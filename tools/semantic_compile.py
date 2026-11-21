#!/usr/bin/env python3
"""Compile selected TM2 C sources without rewriting compiler/assembler output.

Creates isolated objects and source/tool/preprocessed-input hashes. This is a
compile check only; unresolved imports, linkage and semantics remain separate.
Common-symbol visibility is checked after assembly. Use --preserve-common for
tentative global definitions; the legacy-compatible mode fails closed if the
stock wrapper changes a common symbol's binding.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from semantic_elf import read_elf

ROOT = Path(__file__).resolve().parents[1]
MIPS = ROOT / 'toolchain/mips/bin'
CPP = MIPS / 'mipsel-none-elf-cpp.exe'
CC1 = ROOT / 'toolchain/gcc-2.7.2-psx/cc1.exe'
AS = MIPS / 'mipsel-none-elf-as.exe'
NM = MIPS / 'mipsel-none-elf-nm.exe'
MASPSX = ROOT / 'tools/maspsx/maspsx.py'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def header_hashes():
    roots = (ROOT / 'include', ROOT / 'toolchain/psyq330/include')
    return {p.relative_to(ROOT).as_posix(): sha(p) for base in roots
            for p in sorted(base.rglob('*')) if p.is_file() and p.suffix.lower() in ('.h', '.inc')}


def common_linkage_errors(assembly, elf):
    globals_=set(re.findall(r'(?m)^\s*\.(?:globl|global)\s+([^,\s]+)',assembly))
    locals_=set(re.findall(r'(?m)^\s*\.local\s+([^,\s]+)',assembly))
    errors=[]
    for directive,name in re.findall(r'(?m)^\s*\.(comm|lcomm)\s+([^,\s]+)\s*,',assembly):
        expected=0 if name in locals_ or (directive=='lcomm' and name not in globals_) else 1
        found=[s for s in elf['symbols'] if s['name']==name]
        # An unreferenced assembler-local label may legitimately be discarded.
        if not found and expected==0:continue
        if len(found)!=1 or found[0]['binding']!=expected:
            errors.append(dict(symbol=name,expected_binding=expected,actual_bindings=[s['binding'] for s in found]))
    return errors


PORT_TEXT = '.port_text'
PORT_RODATA_DIRECTIVE = '\t.section\t.port_rodata,"a",@progbits'


def port_rodata(text, port_functions=()):
    """Port variant (docs/SOURCE_BUILD.md "Port variant", port code): the read-only data a port function emits
    inside its body (switch jump tables: `.rdata` between `.ent F` and `.end F`) goes to .port_rodata, not to
    .rdata (which no link places). A function is a port function when it is named in `port_functions` (the
    PsyQ path, before its sections are inserted) or its code section is .port_text* (cc1's own `.section`).
    Text without port functions is returned unchanged."""
    if PORT_TEXT not in text and not port_functions:
        return text
    out, code_sec, func = [], '.text', None
    for line in text.splitlines(keepends=True):
        m = re.match(r'^\s*\.section\s+([^\s,]+)\s*(?:,\s*"([^"]*)")?', line)
        if m and (m.group(1).startswith(PORT_TEXT) or 'x' in (m.group(2) or '')):
            code_sec = m.group(1)
        elif re.match(r'^\s*\.text\s*$', line):
            code_sec = '.text'
        m = re.match(r'^\s*\.ent\s+([\w$.]+)', line)
        if m:
            func = m.group(1) if (m.group(1) in port_functions or code_sec.startswith(PORT_TEXT)) else None
        elif re.match(r'^\s*\.end\s', line):
            func = None
        elif func and re.match(r'^\s*\.rdata\s*$', line):
            line = PORT_RODATA_DIRECTIVE + '\n'
        out.append(line)
    return ''.join(out)


def cpp_flags(defines=()):
    """Preprocessor flags of every unit compile; `defines` (e.g. ['TM2_PORT_VARIANT', 'TM2_MAX_HUMANS=2'])
    are appended as -D options (the port variant, docs/SOURCE_BUILD.md)."""
    return ['-EL', '-nostdinc', '-undef', '-D__GNUC__=2', '-D__OPTIMIZE__',
            '-Dmips', '-D__mips__', '-D__mips', '-Dpsx', '-D__psx__', '-D__psx',
            '-D__EXTENSIONS__', '-D_MIPSEL', '-D__CHAR_UNSIGNED__',
            '-D_LANGUAGE_C', '-DLANGUAGE_C', '-DINCLUDE_ASM_USE_MACRO_INC=1',
            '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
            '-I' + str(ROOT / 'toolchain/psyq330/include')] + ['-D' + d for d in defines]


def preprocess(source, out_path, defines=()):
    """Only the cpp step of compile_source (same flags): write the .i to out_path; returns its bytes."""
    cplusplus = source.suffix == '.cpp'
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run([str(CPP), *cpp_flags(defines), *(['-x', 'c', '-D__cplusplus=1'] if cplusplus else []),
                           str(source), '-o', str(out_path)], cwd=out_path.parent, capture_output=True,
                          stdin=subprocess.DEVNULL, timeout=60)
    if proc.returncode:
        raise SystemExit('cpp failed on %s: %s' % (source, proc.stderr.decode(errors='replace')[-1500:]))
    return out_path.read_bytes()


def compile_source(source, output, delay_slots='auto', preserve_common=False, extern_gp=False, fvolatile=False,
                   defines=()):
    output.mkdir(parents=True)
    pp, asm, expanded, obj = [output / name for name in ('source.i', 'source.s', 'source.ms', 'source.o')]
    original_hash = sha(source)
    row = {'source': source.relative_to(ROOT).as_posix(), 'source_sha256': original_hash,
           'compiled': False, 'linked': False, 'runtime_validated': False, 'steps': [], 'preserve_common':preserve_common}
    if defines:
        row['defines'] = list(defines)
    flags = cpp_flags(defines)
    ccflags = ['-O2', '-quiet', '-fno-builtin', '-G8'] + (['-fvolatile'] if fvolatile else [])
    fill = delay_slots == 'compiler' or (delay_slots == 'auto' and 'cc: fill' in source.read_text(encoding='utf-8-sig'))
    if not fill:
        ccflags.append('-fno-delayed-branch')
    cplusplus = source.suffix == '.cpp'
    commands = [
        ('cpp', [CPP, *flags, *(['-x', 'c', '-D__cplusplus=1'] if cplusplus else []), source, '-o', pp]),
        ('cc1', [CC1, *ccflags, pp, '-o', asm]),
        ('maspsx', [sys.executable, MASPSX, '--aspsx-version=2.34', '--expand-div',
                    '--dont-force-G0', *(['--use-comm-section'] if preserve_common else []),
                    *(['--extern-gp'] if extern_gp else []), '-G8', asm]),
        ('as', [AS, '-EL', '-I' + str(ROOT / 'include'), '-I' + str(ROOT),
                '-march=r3000', '-mtune=r3000', '-no-pad-sections', '-O1', '-G0', '-o', obj, expanded]),
    ]
    if cplusplus:
        # C++ units: only preprocess here; PsyQ CC1PLPSX compiles them (match_unit.compile_candidate).
        commands = commands[:1]
    env = dict(os.environ, TMP=str(output) + os.sep, TEMP=str(output) + os.sep, TMPDIR=str(output) + os.sep)
    for stage, command in commands:
        command = list(map(str, command))
        try:
            proc = subprocess.run(command, cwd=output, env=env, capture_output=True,
                                  stdin=subprocess.DEVNULL, timeout=60)
        except subprocess.TimeoutExpired:
            row['failed_stage'] = stage
            row['error'] = '60-second compiler stage timeout'
            break
        (output / (stage + '.stdout')).write_bytes(proc.stdout)
        (output / (stage + '.stderr')).write_bytes(proc.stderr)
        row['steps'].append({'stage': stage, 'command': command, 'exit_code': proc.returncode})
        if proc.returncode:
            row['failed_stage'] = stage
            break
        if stage == 'cpp':
            text = pp.read_text(encoding='utf-8', errors='replace')
            # Fallback markers and unresolved machine-register placeholders are
            # not candidate C implementations, even if a compiler accepts them.
            if re.search(r'\b(?:INCLUDE_ASM|M2C_ERROR|saved_reg_\w+)\b|\.text\.discard', text):
                row['failed_stage'] = 'source-selection'
                row['error'] = 'ASM fallback, M2C error or unresolved saved-register placeholder'
                break
        if stage == 'cc1' and defines:
            # port variant only: jump tables of .port_text functions to .port_rodata (no-op without them)
            text = asm.read_text(encoding='ascii', errors='replace')
            fixed = port_rodata(text)
            if fixed != text:
                asm.write_text(fixed, encoding='ascii', newline='\n')
        if stage == 'maspsx':
            expanded.write_bytes(proc.stdout)  # Exact output; no definition stripping.
    else:
        row['compiled'] = pp.is_file() if cplusplus else obj.is_file()
    row['cplusplus'] = cplusplus
    row['source_unchanged'] = sha(source) == original_hash
    row['compiled'] = row['compiled'] and row['source_unchanged']
    row['artifacts'] = {
        p.name: {'path': p.relative_to(ROOT).as_posix(), 'sha256': sha(p)}
        for p in (pp, asm, expanded, obj) if p.exists()
    }
    if row['compiled'] and cplusplus:
        return row  # object comes from the PsyQ C++ compiler (match_unit.compile_candidate)
    if row['compiled']:
        row['common_linkage_errors']=common_linkage_errors(asm.read_text(),read_elf(obj))
        if row['common_linkage_errors']:
            row['compiled']=False;row['failed_stage']='data-linkage'
            row['error']='Common storage binding changed; use supported preserve-common mode or explicit source ownership'
    if row['compiled']:
        proc = subprocess.run([str(NM), '-u', str(obj)], capture_output=True, timeout=30)
        (output / 'undefined.txt').write_bytes(proc.stdout + proc.stderr)
        row['undefined_symbols'] = [line.split()[-1] for line in proc.stdout.decode('ascii', errors='replace').splitlines() if line.strip()]
        row['symbol_audit_ok'] = proc.returncode == 0
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    select = parser.add_mutually_exclusive_group(required=True)
    select.add_argument('--source', type=Path, action='append')
    select.add_argument('--inventory', type=Path, help='Compile preferred source files from a fresh semantic inventory')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--preserve-common',action='store_true',help='Use stock maspsx common-symbol preservation for source-faithful external storage')
    parser.add_argument('--delay-slots', choices=('auto', 'compiler', 'assembler'), default='auto',
                        help='Semantic scheduling policy; compiler uses ordinary -O2 delay-slot scheduling')
    args = parser.parse_args()
    output = args.output.resolve()
    missing = []
    if args.inventory:
        index = json.loads(args.inventory.read_text(encoding='utf-8'))
        for relative, expected in index['inputs'].items():
            if sha(ROOT / relative) != expected:
                parser.error('Inventory input changed: ' + relative)
        sources = []
        for row in index['functions']:
            facts = row.get('semantic_source') or row['historical_matching_source'] or row['generated_draft']
            if facts['present']:
                path = (ROOT / facts['path']).resolve()
                if sha(path) != facts['sha256']:
                    parser.error('Inventory source changed: ' + facts['path'])
                sources.append(path)
            else:
                missing.append({'va': row['va'], 'label': row['label'], 'reason': 'no candidate source'})
        sources = list(dict.fromkeys(sources))
    else:
        sources = [p.resolve() for p in args.source]
    if not output.is_relative_to(ROOT / 'work') or output.exists():
        parser.error('Choose a new output directory beneath this project work/')
    if len(set(sources)) != len(sources) or any(not p.is_file() or p.suffix != '.c' or not p.is_relative_to(ROOT / 'src') for p in sources):
        parser.error('Choose distinct existing .c sources beneath this project src/')
    output.mkdir(parents=True)
    headers = header_hashes()
    report = {'schema': 'tm2-semantic-compile-v1', 'started_utc': datetime.now(timezone.utc).isoformat(),
              'linked': False, 'runtime_validated': False,
              'header_sha256': headers,
              'delay_slot_policy': args.delay_slots, 'preserve_common':args.preserve_common,
              'missing_sources': missing,
              'inventory_sha256': sha(args.inventory) if args.inventory else None,
              'tools': {str(p): sha(p) for p in (Path(__file__), CPP, CC1, AS, NM, MASPSX)}, 'results': []}
    for i, source in enumerate(sources):
        row = compile_source(source, output / ('%04d_%s' % (i, source.stem)), args.delay_slots, args.preserve_common)
        report['results'].append(row)
        if not args.inventory or not row['compiled'] or (i + 1) % 100 == 0:
            print('%d/%d' % (i + 1, len(sources)), row['source'], 'COMPILED' if row['compiled'] else 'FAILED: ' + row.get('failed_stage', 'changed source'), flush=True)
    report['finished_utc'] = datetime.now(timezone.utc).isoformat()
    report['compiled_count'] = sum(r['compiled'] for r in report['results'])
    report['total'] = len(sources)
    report['headers_unchanged'] = headers == header_hashes()
    (output / 'compile-report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('%s/%s compiled; not linked or runtime validated' % (report['compiled_count'], report['total']))
    return 0 if report['compiled_count'] == report['total'] and report['headers_unchanged'] else 1


if __name__ == '__main__':
    sys.exit(main())
