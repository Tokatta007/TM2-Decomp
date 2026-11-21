#!/usr/bin/env python3
"""Byte-match check for one maintained C unit against retail.

Compiles the unit with its manifest policy (tools/semantic_compile.py), derives
exports/imports from the object, links it at the retail addresses exactly as
tools/build_maximal_hybrid.py does, and compares every selected function with
the retail bytes over the whole retail slot (IDA words * 4). A function matches
when its linked symbol is exactly slot-sized and every byte equals retail.

    python tools/match_unit.py --source src/semantic/area_damage.c
    python tools/match_unit.py --source src/semantic/area_damage.c --apply   # refresh template exports

--apply rewrites this unit's entry in config/semantic/selection-template.json
(object, hashes, exports, imports) so the normal build uses the new C. It never
touches other units. Outputs go to a new work/match/<unit>-<timestamp>/.
"""
import argparse
import os
import csv
import json
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from semantic_compile import MIPS, ROOT, compile_source, sha  # noqa: E402
from semantic_elf import global_symbol, read_elf  # noqa: E402
from build_maximal_hybrid import SECTION_HEX, address_for, symbol_map  # noqa: E402

TEMPLATE = ROOT / 'config/semantic/selection-template.json'
MANIFEST = ROOT / 'BUILD_MANIFEST.json'
OBJDUMP = MIPS / 'mipsel-none-elf-objdump.exe'


def retail_image():
    data = (ROOT / 'bin/SCUS_94306R.EXE').read_bytes()
    load = int.from_bytes(data[24:28], 'little')
    return data, load


def exports_imports(obj):
    elf = read_elf(obj)
    exports, imports = [], []
    for s in elf['symbols']:
        if s['binding'] not in (1, 2) or not s['name']:
            continue
        if s['section_index'] == 0:
            imports.append(s['name'])
            continue
        if s['section_index'] >= 0xFF00:
            continue
        sec = elf['sections'][s['section_index']]
        exports.append(dict(name=s['name'], size=s['size'], offset=s['address'], section=sec['name'],
                            storage='section', section_size=sec['size'], code=bool(sec['flags'] & 4)))
    return exports, sorted(set(imports))


def disasm(blob: bytes, vma: int) -> list[str]:
    tmp = Path(subprocess.run(['cmd', '/c', 'echo', '%TEMP%'], capture_output=True, text=True).stdout.strip())
    path = tmp / f'match_{vma:08X}.bin'
    path.write_bytes(blob)
    out = subprocess.run([str(OBJDUMP), '-D', '-b', 'binary', '-m', 'mips:3000', '-EL',
                          f'--adjust-vma=0x{vma:08X}', str(path)], capture_output=True, text=True).stdout
    path.unlink()
    return [l.strip() for l in out.splitlines() if l.strip()[:2] == '80' and ':' in l]


CANDIDATES = ROOT / 'toolchain/psyq-candidates'
DOSBOX = CANDIDATES / 'dosbox-x/bin/x64/Release/dosbox-x.exe'
CANDIDATE_ASPSX = {'psyq3.3': '2.21', 'psyq3.5': '2.34', 'psyq3.6': '2.34', 'gcc2.6.3': '2.21', 'gcc2.5.7': '2.21'}
# Native (non-DOS) cc1 builds: GCC 2.6.3 and 2.5.7 for the PSX, built from source in toolchain/build-gcc263, build-gcc257.
NATIVE_CC1 = {'gcc2.6.3': ROOT / 'toolchain/gcc-2.6.3-psx/cc1.exe',
              'gcc2.5.7': ROOT / 'toolchain/gcc-2.5.7-psx/cc1.exe'}
ASM_POSTS = ('gcc263',)
SECTION_ATTR = re.compile(r'__attribute__\s*\(\(\s*section\s*\(\s*"([^"]+)"\s*\)\s*\)\)')


def gcc263_frames(text):
    """asm_post 'gcc263': emulate the retail libsnd compiler (GCC 2.6.3) on maspsx
    output: register saves keep their scheduled slots but are stored $31 first,
    then in descending register order; the epilogue's `addu $sp,$sp,N` moves into
    the `j $31` delay slot (with a nop before `j` when it follows `lw $31`)."""
    L = text.replace('\r\n', '\n').split('\n'); out = []; i = 0; saved = None
    while i < len(L):
        l = L[i]
        m = re.match(r'^\.mask\s+0x([0-9a-f]+)', l)
        if m:
            mask = int(m.group(1), 16); saved = set(r for r in range(32) if mask >> r & 1); out.append(l); i += 1; continue
        if saved and re.match(r'^subu\s+\$sp,\$sp,\d+$', l):
            j = i + 1; body = []; saves = {}
            while j < len(L) and not L[j].startswith('.end'):
                s = re.match(r'^sw\s+\$(\d+),(\d+)\(\$sp\)$', L[j])
                if s and int(s.group(1)) in saved and int(s.group(1)) not in saves:
                    saves[int(s.group(1))] = L[j]; body.append(None)
                else:
                    body.append(L[j])
                j += 1
            out.append(l)
            order = [saves[r] for r in sorted(saves, reverse=True)]
            for b in body:
                out.append(order.pop(0) if b is None else b)
            i = j; saved = None; continue
        out.append(l); i += 1
    L = out; out = []; i = 0
    while i < len(L):
        if re.match(r'^addu\s+\$sp,\$sp,\d+$', L[i]) and i + 2 < len(L) and re.match(r'^j\s+\$31$', L[i + 1]) and L[i + 2].startswith('nop'):
            prev = [x for x in out if x.strip() and not x.startswith('#')][-1]
            if re.match(r'^lw\s+\$31,', prev):
                out.append('nop')
            out.append(L[i + 1]); out.append(L[i]); i += 3; continue
        out.append(L[i]); i += 1
    # GCC 2.6.3 also schedules `lw $31` above the last body instruction, so no
    # load-delay nop: `I; lw $31; nop; j $31; addu $sp` -> `lw $31; I; j $31; addu $sp`.
    def code(k):  # index of the next non-comment, non-blank line at or after k
        while k < len(out) and (not out[k].strip() or out[k].lstrip().startswith('#')):
            k += 1
        return k
    def prev_code(k):
        k -= 1
        while k >= 0 and (not out[k].strip() or out[k].lstrip().startswith('#')):
            k -= 1
        return k
    branchy = re.compile(r'^\s*(b\w*|j\w*)\s')
    loads = re.compile(r'^\s*(lb|lbu|lh|lhu|lw|lwl|lwr)\s')
    k = 0
    while k < len(out):
        a = code(k)
        if a >= len(out):
            break
        b = code(a + 1); c = code(b + 1); d = code(c + 1); e = code(d + 1)
        if e < len(out) and re.match(r'^\s*lw\s+\$31,\d+\(\$sp\)\s*$', out[b]) and out[c].strip().startswith('nop') \
                and re.match(r'^\s*j\s+\$31\s*$', out[d]) and re.match(r'^\s*addu\s+\$sp,\$sp,\d+\s*$', out[e]):
            I = out[a]; p = prev_code(a)
            ok = (not I.strip().endswith(':') and not I.lstrip().startswith(('.', 'nop')) and not branchy.match(I)
                  and not loads.match(I) and '$31' not in I and '$sp' not in I and '$29' not in I
                  and (p < 0 or not branchy.match(out[p]))
                  and not any(out[x].strip().endswith(':') for x in range(a + 1, b)))
            if ok:
                lw = out[b]
                del out[c]
                out[b] = I
                out[a] = lw
        k = a + 1
    return '\n'.join(out)


def compile_candidate(compiler, compile_dir, delay, aspsx=None, extern_gp=False, fvolatile=False, cplusplus=False, gp_size=8, asm_post=None, opt=None):
    """Recompile compile_dir/source.i with an original PsyQ CC1PSX (DOS, via DOSBox-X),
    or with a native cc1 from NATIVE_CC1 (e.g. 'gcc2.6.3').

    Old cc1 releases reject the function section attribute, so it is removed from
    the .i and the equivalent `.section` directive is re-inserted in front of each
    symbol in the assembly; placement is unchanged, code generation is cc1's own.
    Then the same maspsx + GNU as steps as tools/semantic_compile.py.
    """
    from semantic_compile import AS, MASPSX
    text = (compile_dir / 'source.i').read_text(encoding='utf-8', errors='replace')
    sections = {}
    for m in SECTION_ATTR.finditer(text):
        after = re.match(r'\s*\**\s*([A-Za-z_]\w*)', text[m.end():])      # u32 __attribute__ *f(...)
        before = re.search(r'([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*$', text[:m.start()])  # u32 D_x[4] __attribute__
        rest = text[m.end():].lstrip(' *' + chr(9) + chr(10) + chr(13))  # attribute may precede the name on its own line
        name = after.group(1) if after and rest[len(after.group(1)):].lstrip().startswith('(') else (before.group(1) if before else None)
        if name:
            sections[name] = m.group(1)
    # `#pragma tm2_symbol MANGLED NAME` (C++ members, e.g. constructors): emit the
    # compiled symbol under its retail name and place it in .text_<address>.
    pragma = re.compile(r'^[ \t]*#[ \t]*pragma[ \t]+tm2_symbol[ \t]+(\S+)[ \t]+(\S+)[ \t]*$', re.M)
    renames = pragma.findall(text)
    text = pragma.sub('', text)
    # `#pragma tm2_rdata .tm2_ADDR`: the compiler's read-only data (switch jump
    # tables) goes to that section, which the linkers place at ADDR.
    rdata_pragma = re.compile(r'^[ \t]*#[ \t]*pragma[ \t]+tm2_rdata[ \t]+(\S+)[ \t]*$', re.M)
    rdata_sections = rdata_pragma.findall(text)
    text = rdata_pragma.sub('', text)
    # `#pragma tm2_absolute NAME`: a small extern that retail addresses absolutely
    # rather than through $gp (its `.extern NAME,size` is dropped before maspsx).
    absolute_pragma = re.compile(r'^[ \t]*#[ \t]*pragma[ \t]+tm2_absolute[ \t]+(\S+)[ \t]*$', re.M)
    absolute = absolute_pragma.findall(text)
    text = absolute_pragma.sub('', text)
    pragma_sections = set()
    for _, new in renames:
        m = re.match(r'(?:sub|func)_([0-9A-Fa-f]{8})$', new)
        if m and new not in sections:
            sections[new] = '.text_' + m.group(1).upper()
            pragma_sections.add(new)
    run = compile_dir / 'psyq'
    run.mkdir()
    cc1 = 'CC1PLPSX.EXE' if cplusplus else 'CC1PSX.EXE'
    src_name = 'IN.II' if cplusplus else 'IN.I'
    if compiler in NATIVE_CC1 and cplusplus:
        raise SystemExit(f'{compiler} has no C++ front end')
    (run / src_name).write_text(SECTION_ATTR.sub('', text), encoding='ascii', errors='replace')
    flags = os.environ.get('TM2_CC1_FLAGS', f'-quiet {opt or "-O2"} -fno-builtin -G{gp_size}') + ('' if delay == 'compiler' else ' -fno-delayed-branch') + (' -fvolatile' if fvolatile else '')
    if compiler in NATIVE_CC1:
        # Native Windows cc1 (no DOSBox): same input, flags and OUT.S as the DOS compilers.
        r = subprocess.run([str(NATIVE_CC1[compiler]), *flags.split(), src_name, '-o', 'OUT.S'], cwd=run,
                           capture_output=True, stdin=subprocess.DEVNULL, timeout=120)
        (run / 'CC1.ERR').write_bytes(r.stderr)
    else:
        shutil.copy(CANDIDATES / compiler / compiler / cc1, run / cc1)
        (run / 'RUN.BAT').write_text(f'{cc1} {flags} {src_name} -o OUT.S\r\n', encoding='ascii')
        subprocess.run([str(DOSBOX), '-silent', '-nogui', '-nomenu', '-set', 'dosbox memsize=64',
                        '-c', f'mount c "{run}"', '-c', 'c:', '-c', 'RUN.BAT', '-c', 'exit'],
                       capture_output=True, stdin=subprocess.DEVNULL, timeout=120)
    asm = run / 'OUT.S'
    if not asm.exists() or '.end' not in asm.read_text(errors='replace'):
        raise SystemExit(f'{compiler} cc1 failed (no complete OUT.S in {run})')
    lines, placed = [], set()
    out = asm.read_text(errors='replace')
    # port variant: functions in .port_text keep their jump tables in .port_rodata (before the tm2_rdata
    # handling below, which then sees only the game's tables; no-op without port functions)
    port_functions = {n for n, s in sections.items() if s.startswith('.port_text')}
    if port_functions:
        from semantic_compile import port_rodata
        out = port_rodata(out, port_functions)
    for old, new in renames:
        out = re.sub(r'(?<![\w$.])' + re.escape(old) + r'(?![\w$.])', new, out)
        if new in pragma_sections and not re.search(r'\.ent\s+' + re.escape(new) + r'\b', out):
            sections.pop(new, None)  # an import (e.g. a base constructor), not placed here
    for name in absolute:
        out = re.sub(r'(?m)^[ \t]*\.extern[ \t]+' + re.escape(name) + r'[ \t]*,.*\n', '', out)
    if rdata_sections:
        # (retail places the table at its address with word alignment)
        out = re.sub(r'(?m)^[ \t]*\.rdata[ \t]*\n[ \t]*\.align[ \t]+3[ \t]*$', '\t.rdata\n\t.align\t2', out)
        # ... but a later table still starts 8-aligned in absolute terms: pad it by hand.
        m = re.match(r'\.tm2_([0-9A-Fa-f]{8})$', rdata_sections[0])
        if m:
            address, first, inside, padded = int(m.group(1), 16), True, False, []
            for line in out.splitlines():
                if re.match(r'^[ \t]*\.rdata[ \t]*$', line):
                    if not first and address % 8:
                        line += '\n\t.space\t4'
                        address += 4
                    first, inside = False, True
                elif re.match(r'^[ \t]*\.(text|data|section|bss)\b', line):
                    inside = False
                elif inside and re.match(r'^[ \t]*\.word[ \t]', line):
                    address += 4
                padded.append(line)
            out = '\n'.join(padded) + '\n'
        out = re.sub(r'(?m)^[ \t]*\.rdata[ \t]*$', '\t.section\t' + rdata_sections[0] + ',"a"', out)
    # Switch section right before each body: `.ent name` for code, `name:` for data.
    # (Some cc1 releases emit every .globl at the top of the file, so .globl is no anchor.)
    current = None  # code section of the function being emitted
    for line in out.splitlines():
        m = re.match(r'\s*\.ent\s+(\w+)', line) or re.match(r'^([A-Za-z_]\w*):', line)
        if m and m.group(1) in sections and m.group(1) not in placed:
            sec = sections[m.group(1)]
            code = sec.startswith(('.text', '.port_text'))
            if code == line.lstrip().startswith('.ent'):
                lines.append(f'\t.section\t{sec},{"\"ax\"" if code else "\"aw\""}')
                lines.append('\t.align\t2')
                placed.add(m.group(1))
                if code:
                    current = sec
        if re.match(r'\s*\.end\s', line):
            lines.append(line)
            current = None
            continue
        if current and re.match(r'^\s*\.text\s*$', line):
            # back from an in-function jump table: return to the function's own section
            line = f'\t.section\t{current},"ax"'
        lines.append(line)
    fixed = compile_dir / 'candidate.s'
    fixed.write_text('\n'.join(lines) + '\n', encoding='ascii')
    ms = subprocess.run([sys.executable, str(MASPSX), f'--aspsx-version={aspsx or CANDIDATE_ASPSX[compiler]}',
                         '--expand-div', '--dont-force-G0', *(['--extern-gp'] if extern_gp else []), f'-G{gp_size}', str(fixed)], capture_output=True, stdin=subprocess.DEVNULL, timeout=120)
    if asm_post == 'gcc263':
        ms.stdout = gcc263_frames(ms.stdout.decode('ascii', errors='replace')).encode('ascii')
    elif asm_post:
        raise SystemExit('unknown asm_post ' + asm_post)
    (compile_dir / 'candidate.ms').write_bytes(ms.stdout)
    obj = compile_dir / 'candidate.o'
    q = subprocess.run([str(AS), '-EL', '-I' + str(ROOT / 'include'), '-I' + str(ROOT), '-march=r3000', '-mtune=r3000',
                        '-no-pad-sections', '-O1', '-G0', '-o', str(obj), str(compile_dir / 'candidate.ms')], capture_output=True, stdin=subprocess.DEVNULL, timeout=120)
    if ms.returncode or q.returncode:
        raise SystemExit('candidate assemble failed: ' + (ms.stderr + q.stderr).decode(errors='replace')[-1500:])
    missing = sorted(set(sections) - placed)
    return obj, missing


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--source', type=Path, required=True)
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--delay-slots', choices=('compiler', 'assembler'), help='override the manifest policy (recorded with --apply)')
    ap.add_argument('--compiler', choices=['shipped'] + sorted(CANDIDATE_ASPSX), help='override the manifest compiler (default: manifest compiler of the unit)')
    ap.add_argument('--extern-gp', choices=('on', 'off'), help='override manifest extern_gp (gp-relative small externs)')
    ap.add_argument('--fvolatile', choices=('on', 'off'), help='override manifest fvolatile (cc1 -fvolatile)')
    ap.add_argument('--gp-size', type=int, help='override manifest gp_size (cc1/maspsx -G, default 8; PsyQ libraries use 0)')
    ap.add_argument('--opt', help='override manifest opt (cc1 optimisation flag, default -O2)')
    ap.add_argument('--asm-post', choices=('none',) + ASM_POSTS, help='override manifest asm_post (rewrite of the maspsx output)')
    ap.add_argument('--aspsx', help='maspsx --aspsx-version for --compiler (default per compiler)')
    ap.add_argument('--as-cpp', action='store_true', help='probe: compile a C unit as C++ (CC1PLPSX) inside extern "C" { }; never with --apply')
    ap.add_argument('--context', type=int, default=6, help='instructions to show around the first difference')
    ap.add_argument('--define', action='append', default=[],
                    help='probe: extra -D for the preprocessor (e.g. TM2_PORT_VARIANT, docs/SOURCE_BUILD.md "Port variant"); '
                         'never with --apply')
    args = ap.parse_args()
    if args.define and args.apply:
        ap.error('--define is a probe; it cannot be combined with --apply')
    source = (ROOT / args.source).resolve()
    rel = source.relative_to(ROOT).as_posix()
    manifest = {u['source']: u for u in json.loads(MANIFEST.read_text(encoding='utf-8'))['units']}
    if rel not in manifest:
        raise SystemExit(f'{rel} is not a manifest unit')
    policy = manifest[rel]
    template = json.loads(TEMPLATE.read_text(encoding='utf-8'))
    funcs = sorted((va.upper(), f) for va, f in template['functions'].items() if f.get('source') == rel)
    words = {r['va'].upper(): int(r['words']) for name in ('extra_funcs.tsv', 'ida_funcs.tsv')
             for r in csv.DictReader((ROOT / 'config/US' / name).open(), delimiter='\t')}

    out = ROOT / 'work/match' / (source.stem + '-' + datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S-%f'))
    delay = args.delay_slots or policy['delay_slots']
    egp = (args.extern_gp == 'on') if args.extern_gp else bool(policy.get('extern_gp'))
    fvol = (args.fvolatile == 'on') if args.fvolatile else bool(policy.get('fvolatile'))
    gsize = args.gp_size if args.gp_size is not None else policy.get('gp_size', 8)
    opt = args.opt or policy.get('opt')
    asm_post = policy.get('asm_post') if args.asm_post is None else (None if args.asm_post == 'none' else args.asm_post)
    row = compile_source(source, out / 'compile', delay, policy['preserve_common'], egp, fvol, defines=args.define)
    if not row['compiled']:
        print(json.dumps({k: row.get(k) for k in ('failed_stage', 'error')}))
        print((out / 'compile' / 'cc1.stderr').read_text(errors='replace')[-2000:] if (out / 'compile' / 'cc1.stderr').exists() else '')
        raise SystemExit(1)
    obj = out / 'compile' / 'source.o'
    compiler = policy.get('compiler') if args.compiler is None else (None if args.compiler == 'shipped' else args.compiler)
    cplusplus = source.suffix == '.cpp' or args.as_cpp
    if args.as_cpp and args.apply:
        raise SystemExit('--as-cpp is a probe; convert the source to .cpp to apply')
    if args.as_cpp and source.suffix != '.cpp':
        pp = out / 'compile' / 'source.i'
        body = pp.read_text(encoding='utf-8', errors='replace')
        pp.write_text('extern "C" {' + chr(10) + body + chr(10) + '}' + chr(10), encoding='utf-8')
    if cplusplus and not compiler:
        raise SystemExit('C++ units need a PsyQ compiler (--compiler psyq3.6 or the manifest compiler)')
    if compiler:
        obj, missing = compile_candidate(compiler, out / 'compile', delay, args.aspsx, egp, fvol, cplusplus, gsize, asm_post, opt)
        if missing:
            print('warning: no section placed for', missing)
    exports, imports = exports_imports(obj)
    by_name = {e['name']: e for e in exports}

    # Link exactly like build_maximal_hybrid: functions at their VA, other exports by name/section.
    syms = symbol_map()
    bases = {}
    for va, f in funcs:
        e = by_name.get(f['symbol'])
        if not e:
            raise SystemExit(f'{f["symbol"]} not exported by {rel}')
        bases.setdefault(e['section'], int(va, 16) - e['offset'])
    for e in exports:
        a = address_for(e['name'], syms)
        if a is None:
            m = SECTION_HEX.match(e['section'])
            a = int(m.group(1), 16) + e['offset'] if m else None
        if a is not None:
            bases.setdefault(e['section'], a - e['offset'])
    for sec in read_elf(obj)['sections']:  # e.g. a jump table moved to its retail address (#pragma tm2_rdata)
        m = SECTION_HEX.match(sec['name'])
        if m and sec['flags'] & 2 and sec['size'] and sec['name'] not in bases:
            bases[sec['name']] = int(m.group(1), 16)
    unresolved =[n for n in imports if address_for(n, syms) is None]
    if unresolved:
        raise SystemExit(f'unresolved imports: {unresolved}')
    script = out / 'unit.ld'
    placed = ' '.join('.s%d 0x%08X : { *(%s) }' % (n, b, s) for n, (s, b) in enumerate(sorted(bases.items())))
    defs = ' '.join('%s = 0x%08X;' % (n, address_for(n, syms)) for n in imports if n != '_gp') + ' _gp = 0x%08X;' % syms['_gp']
    script.write_text('SECTIONS { ' + placed + ' /DISCARD/ : { *(.reginfo .pdr .comment .gnu.attributes .mdebug*) } }\n' + defs + '\n', encoding='ascii')
    elfpath = out / 'unit.elf'
    q = subprocess.run([str(MIPS / 'mipsel-none-elf-ld.exe'), '-EL', '-T', str(script), '-o', str(elfpath), str(obj)], capture_output=True)
    if q.returncode:
        raise SystemExit('link failed: ' + (q.stdout + q.stderr).decode(errors='replace')[-1500:])
    elf = read_elf(elfpath)
    retail, load = retail_image()

    results = []
    for va, f in funcs:
        addr, slot = int(va, 16), 4 * words[va]
        sym = global_symbol(elf, f['symbol'])
        sec = elf['sections'][sym['section_index']]
        start = sym['address'] - sec['address']
        cand = sec['data'][start:start + sym['size']]
        ret = retail[addr - load + 0x800: addr - load + 0x800 + slot]
        # Retail slots may end in alignment padding: a function matches when its bytes
        # equal the retail prefix and the rest of the slot is zero (nop) padding.
        match = cand == ret[:len(cand)] and len(cand) > 0 and not any(ret[len(cand):])
        res = dict(va=va, symbol=f['symbol'], size=sym['size'], slot=slot, match=match,
                   padding=slot - len(cand) if match else None)
        if not match:
            n = min(len(cand), len(ret))
            first = next((i for i in range(0, n, 4) if cand[i:i + 4] != ret[i:i + 4]), n)
            res['first_diff'] = f'{addr + first:08X}'
            res['differing_words'] = sum(1 for i in range(0, n, 4) if cand[i:i + 4] != ret[i:i + 4]) + abs(len(cand) - len(ret)) // 4
            lo = max(0, first - 4 * args.context)
            hi = first + 4 * args.context
            res['retail'] = disasm(ret[lo:hi], addr + lo)
            res['candidate'] = disasm(cand[lo:hi], addr + lo)
        results.append(res)

    for r in results:
        if r['match']:
            print(f"MATCH  {r['va']} {r['symbol']} ({r['slot']} bytes)")
        else:
            print(f"DIFF   {r['va']} {r['symbol']} size {r['size']} vs slot {r['slot']}, {r['differing_words']} words differ, first at {r['first_diff']}")
            for a, b in zip(r['retail'] + [''] * 20, r['candidate'] + [''] * 20):
                if not a and not b:
                    break
                mark = '  ' if a.split(':', 1)[-1:] == b.split(':', 1)[-1:] else '>>'
                print(f'   {mark} R {a[:60]:60} | C {b[:60]}')
    (out / 'match.json').write_text(json.dumps(dict(source=rel, source_sha256=sha(source), results=results), indent=1) + '\n')

    if args.apply:
        unit = template['units'][rel]
        unit.update(object=obj.relative_to(ROOT).as_posix(), object_sha256=sha(obj), source_sha256=sha(source),
                    exports=exports, code_exports=[e['name'] for e in exports if e['code']], imports=imports)
        TEMPLATE.write_text(json.dumps(template, indent=1) + '\n', encoding='utf-8')
        print('template updated for', rel)
        if delay != policy['delay_slots'] or compiler != policy.get('compiler') or egp != bool(policy.get('extern_gp')) or fvol != bool(policy.get('fvolatile')) or gsize != policy.get('gp_size', 8) or asm_post != policy.get('asm_post') or opt != policy.get('opt'):
            m = json.loads(MANIFEST.read_text(encoding='utf-8'))
            u = next(u for u in m['units'] if u['source'] == rel)
            u['delay_slots'] = delay
            if fvol:
                u['fvolatile'] = True
            else:
                u.pop('fvolatile', None)
            if egp:
                u['extern_gp'] = True
            else:
                u.pop('extern_gp', None)
            if gsize != 8:
                u['gp_size'] = gsize
            else:
                u.pop('gp_size', None)
            if asm_post:
                u['asm_post'] = asm_post
            else:
                u.pop('asm_post', None)
            if opt and opt != '-O2':
                u['opt'] = opt
            else:
                u.pop('opt', None)
            if compiler:
                u['compiler'] = compiler
            else:
                u.pop('compiler', None)
            MANIFEST.write_text(json.dumps(m, indent=2) + '\n', encoding='utf-8')
            print('manifest ->', delay, compiler or 'shipped cc1')
    print('all match' if all(r['match'] for r in results) else 'not matching', '->', out.relative_to(ROOT))


if __name__ == '__main__':
    main()
