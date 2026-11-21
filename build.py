"""Offline Windows TM2 hybrid build. Outputs always use a new work directory.

--source-only additionally links the freshly compiled unit objects into the
source-only image (docs/SOURCE_BUILD.md, nonmatching flavour), verifies it with
tools/verify_image.py and records it in LAST_SOURCE_BUILD.json. LAST_BUILD.json
always describes the hybrid image.

--source-only --relocatable also links the same objects in the relocatable
flavour (no retail address fixed except t_addr): a zero-shift link that must
reproduce the source-only sha (E0), and with --shift ADDR:N a link with N bytes
of padding inserted before retail ADDR, proved against E0 by
tools/verify_reloc.py. The result is recorded in LAST_RELOC_BUILD.json.

--source-only --sdk-from-lib also links the source-only image with every PsyQ
module that config/US/sdk_modules.tsv marks exact taken from the local PsyQ 3.3
.LIB files (tools/lnk2elf.py) instead of the C sections of its functions
(source_sdklib/, checked against expected_source_only_sdk_lib_sha256 and
recorded under "sdk_from_lib" in LAST_SOURCE_BUILD.json); with --relocatable the
relocatable links use the .LIB modules too.

--source-only --relocatable --all-c also links the all-C flavour (source_reloc_allc/): the
units whose SingleTrac hand-written asm stays asm in src/ are replaced by their pure-C
copies in reference/ (compiled with the same recipe), laid out relocatably (with --shift
and --sdk-from-lib when given) and proved against the E0 link with tools/verify_reloc.py:
everything outside the replaced functions must un-relocate to E0. Recorded in
LAST_ALLC_BUILD.json (static only; LAST_BUILD.json and LAST_RELOC_BUILD.json unchanged).

--source-only always links the zero-shift relocatable E0 of the same objects as well (source_reloc_e0/,
with --sdk-from-lib when given), proves it with tools/verify_reloc.py, checks that it is byte-identical to
the fixed-layout source-only image, and publishes it for the PC port at a fixed path: out/TM2_SOURCE_E0.EXE,
out/TM2_SOURCE_E0.json (hashes, build, git commit, every byte range that differs from retail) and the maps
in out/TM2_SOURCE_E0/ (docs/SOURCE_BUILD.md, "For the port: stable build and maps"). out/ holds retail
bytes (the retail pieces of split/), so it is gitignored.

--source-only --relocatable --port-variant also links the PORT VARIANT (docs/SOURCE_BUILD.md "Port variant"):
the units recompiled with -DTM2_PORT_VARIANT (include/semantic/players.h: 4 humans, 4 views, 4 HUDs; the car
arrays and pools at retail's sizes, raised by the port's --port-define TM2_PORT_CARS=N), the grown and moved
arrays in .port_data/.port_bss at --port-base (default 0x80200000, outside the EXE). Two links, both
proved against E0 with tools/verify_reloc.py: source_reloc_port/ (the variant) and source_reloc_port2/ (the
human/view/HUD limits forced to 2 and the others to retail's: only the arrays move, so everything but them and
the functions that addressed them through
$gp must un-relocate to E0). Published to out/TM2_SOURCE_PORT.EXE, out/TM2_SOURCE_PORT.json (the functions that
differ from E0) and out/TM2_SOURCE_PORT/ (maps, port_data.bin, port_text.bin, port_rodata.bin); recorded in
LAST_PORT_BUILD.json. --port-define NAME[=VALUE] and --port-unit PATH.c (repeatable) pass the PC port's own code
to both links (docs/SOURCE_BUILD.md "Port code"). --port-ram-end ADDR (default 0x80C00000): the port sections must
end at or below it; the port chooses it (its own high RAM starts at or above it); recorded as port_ram_end."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools'))
from semantic_compile import compile_source, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify-package', action='store_true', help='Check packaged file hashes and exit')
    parser.add_argument('--source-only', action='store_true',
                        help='After the hybrid build, also link and verify the source-only image (LAST_SOURCE_BUILD.json)')
    parser.add_argument('--relocatable', action='store_true',
                        help='With --source-only: also link the relocatable flavour and prove it (LAST_RELOC_BUILD.json)')
    parser.add_argument('--shift', help='With --relocatable: ADDR:N inserts N bytes of padding before retail ADDR')
    parser.add_argument('--all-c', action='store_true',
                        help='With --relocatable: also link the pure-C versions of the hand-written asm units from '
                             'reference/ (source_reloc_allc/, LAST_ALLC_BUILD.json)')
    parser.add_argument('--port-variant', action='store_true',
                        help='With --relocatable: also link the port variant (4 players, arrays at --port-base; '
                             'out/TM2_SOURCE_PORT.*, LAST_PORT_BUILD.json)')
    parser.add_argument('--port-base', default='80200000',
                        help='With --port-variant: address of the port arrays (hex, default 80200000)')
    parser.add_argument('--port-ram-end', default=None, metavar='ADDR',
                        help='With --port-variant: the port sections must end at or below this address (hex, default '
                             '80C00000); the port chooses it, and its own high RAM starts at or above it')
    parser.add_argument('--port-define', action='append', default=[], metavar='NAME[=VALUE]',
                        help='With --port-variant (repeatable): one more -D for the variant and its placement check, '
                             'e.g. TM2_PORT_HOOKS (docs/SOURCE_BUILD.md "Port code")')
    parser.add_argument('--port-unit', action='append', default=[], metavar='PATH.c',
                        help='With --port-variant (repeatable): a port unit compiled into the port sections '
                             '(.port_text/.port_rodata/.port_data/.port_bss; docs/SOURCE_BUILD.md "Port code")')
    parser.add_argument('--sdk-from-lib', action='store_true',
                        help='With --source-only: also link the exact PsyQ 3.3 modules from the local .LIB files '
                             '(source_sdklib/); with --relocatable the relocatable links use them too')
    args = parser.parse_args()
    if args.relocatable and not args.source_only:
        parser.error('--relocatable needs --source-only')
    if args.shift and not args.relocatable:
        parser.error('--shift needs --relocatable')
    if args.all_c and not args.relocatable:
        parser.error('--all-c needs --relocatable')
    if args.port_variant and not args.relocatable:
        parser.error('--port-variant needs --relocatable')
    if (args.port_define or args.port_unit or args.port_ram_end) and not args.port_variant:
        parser.error('--port-define/--port-unit/--port-ram-end need --port-variant')
    if args.sdk_from_lib and not args.source_only:
        parser.error('--sdk-from-lib needs --source-only')
    if args.verify_package:
        if not (ROOT / 'PACKAGE.json').is_file():
            raise SystemExit('No PACKAGE.json: write one first with tools/make_package.py')
        package = json.loads((ROOT / 'PACKAGE.json').read_text(encoding='utf-8'))
        for name, row in package['files'].items():
            path = ROOT / name
            if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != row['sha256']:
                raise SystemExit('Package file differs: ' + name)
        print('Package integrity OK:', len(package['files']), 'files')
        return

    manifest = json.loads((ROOT / 'BUILD_MANIFEST.json').read_text(encoding='utf-8'))
    selection = json.loads((ROOT / 'config/semantic/selection-template.json').read_text(encoding='utf-8'))
    env = dict(os.environ)
    for key in ('PYTHONHOME', 'PYTHONPATH', 'GCC_EXEC_PREFIX', 'COMPILER_PATH', 'LIBRARY_PATH', 'CPATH', 'C_INCLUDE_PATH'):
        env.pop(key, None)
    env['PYTHONNOUSERSITE'] = '1'
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    env['PATH'] = str(ROOT / 'toolchain/mips/bin') + os.pathsep + str(Path(os.environ['SystemRoot']) / 'System32')
    output = ROOT / 'work' / datetime.now(timezone.utc).strftime('build-%Y%m%d-%H%M%S-%f')
    compile_out = output / 'compile'
    compile_out.mkdir(parents=True)

    for index, item in enumerate(manifest['units']):
        source = ROOT / item['source']
        row = compile_source(source, compile_out / ('%04d_%s' % (index, source.stem)),
                             item['delay_slots'], item['preserve_common'], item.get('extern_gp', False),
                             item.get('fvolatile', False))
        if not row['compiled']:
            raise SystemExit('Compile failed: ' + item['source'] + ' (' + row.get('failed_stage', 'unknown') + ')')
        cplusplus = source.suffix == '.cpp'
        obj = ROOT / row['artifacts']['source.i' if cplusplus else 'source.o']['path']
        if cplusplus and not item.get('compiler'):
            raise SystemExit('Compile failed: ' + item['source'] + ' (C++ unit without a PsyQ compiler)')
        if item.get('compiler'):
            # Units verified to byte-match retail only with an original PsyQ CC1PSX
            # (tools/match_unit.py --compiler) are recompiled with it via DOSBox-X.
            from match_unit import compile_candidate
            obj, missing = compile_candidate(item['compiler'], obj.parent, item['delay_slots'], extern_gp=item.get('extern_gp', False),
                                              fvolatile=item.get('fvolatile', False), cplusplus=cplusplus,
                                              gp_size=item.get('gp_size', 8), asm_post=item.get('asm_post'),
                                              opt=item.get('opt'))
            if missing:
                raise SystemExit('Compile failed: ' + item['source'] + ' (unplaced sections ' + ', '.join(missing) + ')')
        actual = sha(obj)
        unit = selection['units'][item['source']]
        unit['object'] = obj.relative_to(ROOT).as_posix()
        unit['object_sha256'] = actual
        unit['source_sha256'] = sha(source)
        print('%d/%d %s COMPILED' % (index + 1, len(manifest['units']), item['source']), flush=True)

    selection_path = output / 'selection.json'
    selection_path.write_text(json.dumps(selection, indent=2) + '\n', encoding='utf-8')
    image_out = output / 'psx'
    subprocess.run([sys.executable, '-B', str(ROOT / 'tools/build_maximal_hybrid.py'),
                    '--selection', str(selection_path), '--output', str(image_out)],
                   cwd=ROOT, env=env, check=True)
    exe = image_out / 'TM2_MAXIMAL_HYBRID.EXE'
    digest = sha(exe)
    if digest != manifest['expected_image_sha256']:
        raise SystemExit('Executable differs from snapshot: expected %s, got %s' %
                         (manifest['expected_image_sha256'], digest))
    result = dict(executable=str(exe.relative_to(ROOT)), sha256=digest,
                  matches_checkpoint=True, replaced_functions=manifest['replaced_functions'],
                  source_only=False)
    (ROOT / 'LAST_BUILD.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))
    if args.source_only:
        source = source_only(output, result, manifest, env, args.sdk_from_lib)
        e0 = link_e0(output, env, args.sdk_from_lib)
        publish_e0(output, result, source, e0, args.sdk_from_lib)
        if args.relocatable:
            reloc = relocatable(output, result, source, e0, manifest, env, args.shift, args.sdk_from_lib)
            if args.all_c:
                all_c(output, result, reloc, env, args.shift, args.sdk_from_lib)
            if args.port_variant:
                port_variant(output, result, e0, env, args.sdk_from_lib, int(args.port_base, 16),
                             args.port_define, args.port_unit, args.port_ram_end)


def source_only(output, hybrid, manifest, env, sdk_from_lib=False):
    """Link this build's unit objects into the source-only image, verify it, write LAST_SOURCE_BUILD.json."""
    result = link_source_only(output, 'source', [], hybrid, manifest, env, 'expected_source_only_sha256')
    if sdk_from_lib:
        result['sdk_from_lib'] = link_source_only(output, 'source_sdklib', ['--sdk-from-lib'], hybrid, manifest, env,
                                                  'expected_source_only_sdk_lib_sha256')
    (ROOT / 'LAST_SOURCE_BUILD.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))
    return result


def link_source_only(output, name, extra, hybrid, manifest, env, key):
    """One fixed source-only link of this build's objects (build_source_only.py + verify_image.py), checked
    against BUILD_MANIFEST.json[key]."""
    source_out = output / name
    subprocess.run([sys.executable, '-B', str(ROOT / 'tools/build_source_only.py'),
                    '--build-dir', output.relative_to(ROOT).as_posix(),
                    '--output', source_out.relative_to(ROOT).as_posix()] + extra,
                   cwd=ROOT, env=env, check=True)
    verify = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/verify_image.py'),
                             source_out.relative_to(ROOT).as_posix()], cwd=ROOT, env=env)
    report = json.loads((source_out / 'report.json').read_text(encoding='utf-8'))
    checked = json.loads((source_out / 'verify.json').read_text(encoding='utf-8'))
    exe = source_out / 'TM2_SOURCE.EXE'
    digest = sha(exe)
    expected = manifest.get(key)
    result = dict(executable=str(exe.relative_to(ROOT)), sha256=digest, expected_sha256=expected,
                  matches_expected=None if expected is None else digest == expected,
                  matches_retail=report['matches_retail'], flavour=report['flavour'], milestone=report['milestone'],
                  hybrid_executable=hybrid['executable'], hybrid_sha256=hybrid['sha256'],
                  c_units=report['c_units'], c_sections=report['c_sections'], retail_pieces=report['retail_pieces'],
                  zero_tails=report['zero_tails'], verified=verify.returncode == 0 and not checked['unexpected'],
                  vs_hybrid_bytes=checked['vs_hybrid']['bytes'], vs_retail_bytes=checked['vs_retail']['bytes_by_class'],
                  problems=report['problems'], source_only=True)
    if report.get('sdk_from_lib'):
        result.update(sdk_from_lib=True, sdk_lib_coverage=report['sdk_from_lib']['coverage'],
                      sdk_lib_externalized=report['sdk_from_lib']['externalized'],
                      sdk_lib_retail_bytes=checked['vs_hybrid'].get('sdk_lib_retail_bytes'),
                      sdk_lib_close_functions_replaced=checked['vs_hybrid'].get('sdk_lib_close_functions_replaced'))
    if not result['verified']:
        raise SystemExit('Source-only image failed verification: see ' +
                         (source_out / 'verify.json').relative_to(ROOT).as_posix())
    if expected is None:
        print('warning: BUILD_MANIFEST.json has no %s; not checked' % key)
    elif digest != expected:
        raise SystemExit('Source-only executable (%s) differs from snapshot: expected %s, got %s' % (name, expected, digest))
    return result


def reloc_link(output, name, extra, env, sdk_from_lib):
    """One relocatable link of this build's objects into output/name (build_source_only.py --relocatable)."""
    out = output / name
    subprocess.run([sys.executable, '-B', str(ROOT / 'tools/build_source_only.py'),
                    '--build-dir', output.relative_to(ROOT).as_posix(), '--relocatable',
                    '--output', out.relative_to(ROOT).as_posix()] + extra + (['--sdk-from-lib'] if sdk_from_lib else []),
                   cwd=ROOT, env=env, check=True)
    return out


def reloc_prove(out, env, e0=None):
    cmd = [sys.executable, '-B', str(ROOT / 'tools/verify_reloc.py'), out.relative_to(ROOT).as_posix()]
    if e0 is not None:
        cmd += ['--e0', e0.relative_to(ROOT).as_posix()]
    subprocess.run(cmd, cwd=ROOT, env=env)
    return json.loads((out / 'reloc_verify.json').read_text(encoding='utf-8'))


def link_e0(output, env, sdk_from_lib=False):
    """The zero-shift relocatable link (source_reloc_e0/) and its proof; returns (dir, proof)."""
    e0 = reloc_link(output, 'source_reloc_e0', [], env, sdk_from_lib)
    check = reloc_prove(e0, env)
    if not check['ok']:
        raise SystemExit('Relocatable E0 failed its proof: see ' + (e0 / 'reloc_verify.json').relative_to(ROOT).as_posix())
    return e0, check


def git_info():
    def git(*a):
        r = subprocess.run(['git'] + list(a), cwd=ROOT, capture_output=True, text=True)
        return r.stdout.strip() if r.returncode == 0 else None
    status = git('status', '--porcelain', '--untracked-files=no')
    return dict(commit=git('rev-parse', 'HEAD'), branch=git('rev-parse', '--abbrev-ref', 'HEAD'),
                dirty=None if status is None else bool(status),
                note='dirty: tracked files differed from the commit when the build ran')


def publish_e0(output, hybrid, source, e0, sdk_from_lib=False):
    """Copy the E0 image and its maps to out/ (a fixed path for the PC port) with out/TM2_SOURCE_E0.json."""
    import function_maps
    from survey_layout import RETAIL_SHA256
    e0_dir, e0_check = e0
    fixed = source['sdk_from_lib'] if sdk_from_lib else source
    fixed_exe = ROOT / fixed['executable']
    e0_exe = e0_dir / 'TM2_SOURCE.EXE'
    if e0_exe.read_bytes() != fixed_exe.read_bytes():
        raise SystemExit('Relocatable E0 %s is not byte-identical to the fixed-layout image %s' % (e0_exe, fixed_exe))
    diffs, totals = function_maps.retail_differences(e0_dir, fixed_exe.parent / 'verify.json')
    fmaps = json.loads((e0_dir / 'report.json').read_text(encoding='utf-8'))['maps']['function_maps']
    out = ROOT / 'out'
    maps_dir = out / 'TM2_SOURCE_E0'
    maps_dir.mkdir(parents=True, exist_ok=True)

    def put(src, dst):  # replace atomically: a reader sees the old or the new file, never a partial one
        tmp = dst.with_name(dst.name + '.tmp')
        shutil.copyfile(src, tmp)
        os.replace(tmp, dst)
    files = ('addrmap.tsv', 'symbols.tsv', 'pointer_slots.tsv', 'functions.tsv', 'jumptables.tsv',
             'functions_ida.tsv', 'names_ida.tsv', 'recomp_inventory.json')
    for name in files:
        put(e0_dir / name, maps_dir / name)
    put(e0_exe, out / 'TM2_SOURCE_E0.EXE')

    def rel(path):
        return Path(path).relative_to(ROOT).as_posix()
    digest = sha(e0_exe)
    info = dict(schema='tm2-source-e0-v1',
                executable='out/TM2_SOURCE_E0.EXE', sha256=digest, size=e0_exe.stat().st_size,
                flavour='nonmatching' + (' + sdk_from_lib' if sdk_from_lib else ''), sdk_from_lib=sdk_from_lib,
                layout='retail layout: relocatable link with no shift (identity address map)',
                built_utc=datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                build_dir=rel(output), e0_executable=rel(e0_exe),
                e0_proof=dict(ok=e0_check['ok'], checks=sorted(e0_check['checks'])),
                fixed_layout_executable=rel(fixed_exe), fixed_layout_sha256=fixed['sha256'],
                identical_to_fixed_layout=digest == fixed['sha256'],
                expected_sha256=fixed['expected_sha256'], matches_expected=fixed['matches_expected'],
                hybrid_executable=hybrid['executable'].replace('\\', '/'), hybrid_sha256=hybrid['sha256'],
                retail_executable='bin/SCUS_94306R.EXE', retail_sha256=RETAIL_SHA256, matches_retail=digest == RETAIL_SHA256,
                git=git_info(),
                maps={name: rel(maps_dir / name) for name in files},
                functions=dict(total=fmaps['functions'], identical=fmaps['identical'], by_form=fmaps['by_form'],
                               sdk=fmaps['sdk']),
                jump_tables=dict(tables=fmaps['jump_tables'], entries=fmaps['jump_table_entries'],
                                 unresolved_jr=fmaps['unresolved_jr']),
                differences=dict(totals, functions=diffs),
                contains_retail_bytes=True)
    tmp = out / 'TM2_SOURCE_E0.json.tmp'
    tmp.write_text(json.dumps(info, indent=1) + '\n', encoding='utf-8')
    os.replace(tmp, out / 'TM2_SOURCE_E0.json')
    last = json.loads((ROOT / 'LAST_SOURCE_BUILD.json').read_text(encoding='utf-8'))
    last['e0'] = dict(executable=rel(e0_exe), sha256=digest, identical_to_fixed_layout=info['identical_to_fixed_layout'],
                      published='out/TM2_SOURCE_E0.EXE', published_json='out/TM2_SOURCE_E0.json',
                      differences=dict(bytes=totals['bytes'], runs=totals['runs'], places=totals['places'],
                                       functions=totals['owners']))
    (ROOT / 'LAST_SOURCE_BUILD.json').write_text(json.dumps(last, indent=2) + '\n', encoding='utf-8')
    print('published out/TM2_SOURCE_E0.EXE sha256 %s: %d bytes differ from retail in %d places (%d runs) in %d '
          'functions/data items; %d functions, %d jump tables' % (digest, totals['bytes'], totals['places'], totals['runs'],
                                                              totals['owners'], fmaps['functions'], fmaps['jump_tables']))
    return info


def relocatable(output, hybrid, source, e0, manifest, env, shift, sdk_from_lib=False):
    """Relocatable links of this build's objects (E0 = no shift, from link_e0, plus --shift), proved with
    verify_reloc.py. With sdk_from_lib the exact PsyQ modules come from the .LIB and E0 must equal the
    source_sdklib image."""
    if sdk_from_lib:
        source = source['sdk_from_lib']
    e0, e0_check = e0
    final, final_check = e0, e0_check
    if shift:
        final = reloc_link(output, 'source_reloc', ['--shift', shift], env, sdk_from_lib)
        final_check = reloc_prove(final, env, e0)
        if not final_check['ok']:
            raise SystemExit('Relocatable link with --shift %s failed its proof: see %s' % (
                shift, (final / 'reloc_verify.json').relative_to(ROOT).as_posix()))
    report = json.loads((final / 'report.json').read_text(encoding='utf-8'))
    exe = final / 'TM2_SOURCE.EXE'

    def rel(path):
        return str(path.relative_to(ROOT))
    result = dict(executable=rel(exe), sha256=sha(exe), relocated=True, shift=report['shift'],
                  t_size=report['t_size'], pc0=report['pc0'], gp=report['maps']['gp_c'], bss_end=report['maps']['bss_end_c'],
                  addrmap=rel(final / 'addrmap.tsv'), symbols=rel(final / 'symbols.tsv'),
                  pointer_slots=rel(final / 'pointer_slots.tsv'),
                  functions=rel(final / 'functions.tsv'), jumptables=rel(final / 'jumptables.tsv'),
                  map_kinds=report['maps']['map_kinds'], labels=report['labels'],
                  generated_pointer_words=report['generated_pointer_words'],
                  e0_executable=rel(e0 / 'TM2_SOURCE.EXE'), e0_sha256=e0_check['sha256'],
                  e0_matches_source_only=e0_check['sha256'] == source['sha256'],
                  proof=dict(e0=e0_check['ok'], shifted=None if not shift else final_check['ok'],
                             checks=sorted(final_check['checks'])),
                  source_only_executable=source['executable'], source_only_sha256=source['sha256'],
                  hybrid_executable=hybrid['executable'], hybrid_sha256=hybrid['sha256'],
                  sdk_from_lib=sdk_from_lib, source_only=True)
    (ROOT / 'LAST_RELOC_BUILD.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))
    return result


def all_c(output, hybrid, reloc, env, shift, sdk_from_lib=False):
    """All-C relocatable link of this build's objects with the reference/ pure-C versions of the hand-written
    asm units, proved against the E0 link of relocatable(); writes LAST_ALLC_BUILD.json. Static only."""
    out = output / 'source_reloc_allc'
    extra = (['--shift', shift] if shift else []) + (['--sdk-from-lib'] if sdk_from_lib else [])
    subprocess.run([sys.executable, '-B', str(ROOT / 'tools/build_source_only.py'),
                    '--build-dir', output.relative_to(ROOT).as_posix(), '--relocatable', '--all-c',
                    '--output', out.relative_to(ROOT).as_posix()] + extra, cwd=ROOT, env=env, check=True)
    e0 = (ROOT / reloc['e0_executable']).parent
    subprocess.run([sys.executable, '-B', str(ROOT / 'tools/verify_reloc.py'), out.relative_to(ROOT).as_posix(),
                    '--e0', e0.relative_to(ROOT).as_posix()], cwd=ROOT, env=env)
    check = json.loads((out / 'reloc_verify.json').read_text(encoding='utf-8'))
    report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
    exe = out / 'TM2_SOURCE.EXE'

    def rel(path):
        return str(path.relative_to(ROOT))
    allc = report['all_c']
    result = dict(executable=rel(exe), sha256=sha(exe), relocated=True, all_c=True, shift=report['shift'],
                  t_size=report['t_size'], pc0=report['pc0'], gp=report['maps']['gp_c'], bss_end=report['maps']['bss_end_c'],
                  addrmap=rel(out / 'addrmap.tsv'), symbols=rel(out / 'symbols.tsv'),
                  pointer_slots=rel(out / 'pointer_slots.tsv'), functions=rel(out / 'functions.tsv'),
                  jumptables=rel(out / 'jumptables.tsv'), map_kinds=report['maps']['map_kinds'],
                  units=allc['units'], replaced=allc['sections'], asm_bytes=allc['asm_bytes'], c_bytes=allc['c_bytes'],
                  delta=allc['delta'], asm_labels_at_entry=allc['labels'],
                  proof=dict(ok=check['ok'], against_e0=reloc['e0_executable'], e0_sha256=reloc['e0_sha256'],
                             excluded=check['all_c_excluded'], failed=check['failed'], checks=sorted(check['checks'])),
                  hybrid_executable=hybrid['executable'], hybrid_sha256=hybrid['sha256'],
                  sdk_from_lib=sdk_from_lib, source_only=True)
    if not check['ok']:
        raise SystemExit('All-C relocatable link failed its proof: see ' + rel(out / 'reloc_verify.json'))
    (ROOT / 'LAST_ALLC_BUILD.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))
    return result


def gp_functions_of_moved_arrays():
    """Retail functions that reach a moved array through $gp (config/US/retail_relocs.tsv gprel16 rows): in the
    port variant they address it absolutely, so their code changes even with the limits forced to 2."""
    import gen_link
    ranges = [(a['retail_va'], a['retail_va'] + a['entry_bytes'] * a['retail_entries'])
              for a in gen_link.read_port_arrays() if a['retail_va'] is not None]
    out = set()
    for r in gen_link.read_tsv(ROOT / 'config/US/retail_relocs.tsv'):
        if r['kind'] == 'gprel16' and r['target'] and any(lo <= int(r['target'], 16) < hi for lo, hi in ranges):
            out.add(r['function'])
    return out


def port_variant(output, hybrid, e0, env, sdk_from_lib, base, port_defines=(), port_units=(), port_ram_end=None):
    """The port variant (source_reloc_port/) and its placement check (source_reloc_port2/, limits forced to 2),
    both proved against E0; publishes out/TM2_SOURCE_PORT.* and writes LAST_PORT_BUILD.json. Static only.
    port_defines / port_units (the PC port's own code, --port-define / --port-unit) go to both links; the
    placement check then also accepts the functions those defines change (and their jump tables)."""
    e0_dir, e0_check = e0
    links = {}
    port_args = [a for d in port_defines for a in ('--port-define', d)] + [a for u in port_units for a in ('--port-unit', u)]
    if port_ram_end:
        port_args += ['--port-ram-end', port_ram_end]
    for name, extra in (('source_reloc_port', []), ('source_reloc_port2', ['--port-limits', '2'])):
        out = reloc_link(output, name, ['--port-variant', '--port-base', '%08X' % base] + port_args + extra, env,
                         sdk_from_lib)
        check = reloc_prove(out, env, e0_dir)
        report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
        links[name] = (out, check, report)
    out4, check4, rep4 = links['source_reloc_port']
    out2, check2, rep2 = links['source_reloc_port2']

    def funcs(rep):
        return {s['symbol']: s for s in rep['port_variant']['changed_sections'] if s['code']}
    changed4, changed2 = funcs(rep4), funcs(rep2)
    expected2 = gp_functions_of_moved_arrays()
    port2 = {s['symbol'] for s in rep2['port_variant']['changed_sections'] if s.get('port_changed')}
    placement = dict(ok=check2['ok'] and not set(changed2) - expected2 - port2 and expected2 <= set(changed2) and not any(
                         not s['code'] and not s.get('port_changed') for s in rep2['port_variant']['changed_sections']),
                     proof=check2['ok'], failed=check2['failed'], changed_functions=sorted(changed2),
                     expected_gp_functions=sorted(expected2), port_changed_functions=sorted(port2),
                     unexpected=sorted(set(changed2) - expected2 - port2), missing=sorted(expected2 - set(changed2)))
    if not check4['ok']:
        raise SystemExit('Port variant failed its proof against E0: see ' + (out4 / 'reloc_verify.json').relative_to(ROOT).as_posix())
    if not placement['ok']:
        raise SystemExit('Port variant placement check (limits forced to 2) failed: %s' % json.dumps(placement))
    return publish_port(output, hybrid, e0_dir, e0_check, out4, check4, rep4, out2, rep2, placement, changed4, changed2,
                        sdk_from_lib)


def publish_port(output, hybrid, e0_dir, e0_check, out4, check4, rep4, out2, rep2, placement, changed4, changed2,
                 sdk_from_lib):
    """out/TM2_SOURCE_PORT.EXE, out/TM2_SOURCE_PORT.json and out/TM2_SOURCE_PORT/ (maps, port_data.bin,
    port_text.bin, port_rodata.bin)."""
    import function_maps
    from survey_layout import RETAIL_SHA256
    out = ROOT / 'out'
    maps_dir = out / 'TM2_SOURCE_PORT'
    maps_dir.mkdir(parents=True, exist_ok=True)

    def put(src, dst):
        tmp = dst.with_name(dst.name + '.tmp')
        shutil.copyfile(src, tmp)
        os.replace(tmp, dst)

    def rel(path):
        return Path(path).relative_to(ROOT).as_posix()
    files = ('addrmap.tsv', 'symbols.tsv', 'pointer_slots.tsv', 'functions.tsv', 'jumptables.tsv',
             'functions_ida.tsv', 'names_ida.tsv', 'recomp_inventory.json', 'port_data.bin', 'port_text.bin',
             'port_rodata.bin')
    for name in files:
        put(out4 / name, maps_dir / name)
    exe = out4 / 'TM2_SOURCE.EXE'
    syms = {}
    for line in (out4 / 'symbols.tsv').read_text(encoding='ascii').splitlines():
        p = line.split('\t')
        if len(p) == 4 and p[0] != 'va' and not line.startswith('#'):
            syms.setdefault(p[2], p[0])
    lines = ['# Generated by build.py --port-variant: the arrays of the port variant (config/US/port_variant_arrays.tsv)',
             '# at their build address. retail_va/retail_size: their retail storage (now unused); layout: linear = the',
             '# retail entries are the first entries here, relayout = only the start maps, new = no retail storage.',
             'address\tsize\tentries\tentry_bytes\tname\tsection\tretail_va\tretail_size\tlayout\tgroup']
    for a in rep4['port_variant']['arrays']:
        lines.append('%s\t0x%X\t%d\t0x%X\t%s\t%s\t%s\t0x%X\t%s\t%s' % (
            syms[a['name']], a['size'], a['entries'], a['entry_bytes'], a['name'], a['section'], a['retail_va'] or '-',
            a['retail_size'], a['layout'], a['group']))
    tmp = maps_dir / 'port_arrays.tsv.tmp'
    tmp.write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')
    os.replace(tmp, maps_dir / 'port_arrays.tsv')
    # request 48 stage 3 (ONLINE_ROLLBACK.md 12.4): config/US/rollback_regions.tsv resolved for this build
    import rollback_regions
    rr_rows, rr_problems = rollback_regions.load_rows()
    rr_res, rr_more = rollback_regions.resolve(out4, rr_rows)
    if rr_problems or rr_more:
        raise SystemExit('rollback_regions.tsv: %s' % '; '.join(rr_problems + rr_more))
    rollback_regions.write(out4, rr_res, maps_dir / 'rollback_regions.tsv')
    files = files + ('port_arrays.tsv', 'rollback_regions.tsv')
    put(exe, out / 'TM2_SOURCE_PORT.EXE')
    pv = rep4['port_variant']
    fmap = {r['retail_va']: r for r in function_maps_rows(out4 / 'functions.tsv')}

    def why(sym):
        if changed4[sym].get('port_changed'):
            return "the port's own change (--port-define %s)" % ' '.join(pv['port_defines'])
        if sym not in changed2:
            return 'variant code (players.h limits, N-player loops and tests)'
        if changed2[sym]['variant_bytes'] == changed4[sym]['variant_bytes']:
            return 'reaches a moved array through $gp in retail (absolute in the variant)'
        return 'reaches a moved array through $gp in retail, and variant code (players.h limits)'
    changed = []
    for sym, s in sorted(changed4.items(), key=lambda kv: kv[1]['function'] or ''):
        f = fmap.get(s['function'], {})
        changed.append(dict(retail_va=s['function'], name=sym, unit=s['unit'], build_va=f.get('build_va'),
                            retail_size=s['retail_bytes'], build_size=s['variant_bytes'], delta=s['delta'],
                            changed_with_limits_2=sym in changed2, port_changed=bool(s.get('port_changed')),
                            why=why(sym)))
    tables = [dict(section=s['section'], retail_va=s['section'].split('_')[-1], unit=s['unit'], of_function=s['symbol'],
                   of_function_retail_va=s['function'], port_changed=bool(s.get('port_changed')))
              for s in pv['changed_sections'] if not s['code']]
    fm = json.loads((out4 / 'report.json').read_text(encoding='utf-8'))['maps']['function_maps']
    digest = sha(exe)
    info = dict(schema='tm2-source-port-v1',
                executable='out/TM2_SOURCE_PORT.EXE', sha256=digest, size=exe.stat().st_size,
                flavour='port variant (relocatable' + (' + sdk_from_lib' if sdk_from_lib else '') + ')',
                defines=pv['defines'], limits=pv['limits'],
                built_utc=datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'), build_dir=rel(output),
                link_dir=rel(out4), git=git_info(),
                t_size=rep4['t_size'], pc0=rep4['pc0'], gp=rep4['maps']['gp_c'], bss_end=rep4['maps']['bss_end_c'],
                port_base=pv['base'], port_ram_end=pv['ram_end'],
                port_end='%08X' % max(int(s['address'], 16) + s['size'] for s in pv['sections']),
                port_ram_end_meaning='the port sections end at or below port_ram_end (--port-ram-end; the port '
                                     'chooses it): the high RAM of the port must start at or above it',
                port_sections=[dict(s, loaded_by=PORT_LOADED_BY[s['name']]) for s in pv['sections']],
                port_arrays=[dict(a, address=syms.get(a['name'])) for a in pv['arrays']],
                port_defines=pv['port_defines'], port_units=pv['port_units'],
                port_functions=fm.get('port_functions', []), port_jump_tables=fm.get('port_jump_tables', []),
                redirects=pv['redirects'],
                changed_functions=changed, changed_jump_tables=tables,
                proof=dict(ok=check4['ok'], against_e0=rel(e0_dir / 'TM2_SOURCE.EXE'), e0_sha256=e0_check['sha256'],
                           excluded=check4['all_c_excluded'], failed=check4['failed'], checks=sorted(check4['checks'])),
                placement_check=dict(placement, link_dir=rel(out2), sha256=sha(out2 / 'TM2_SOURCE.EXE'),
                                     meaning='with TM2_MAX_HUMANS/VIEWS/HUDS forced to 2 and every other limit '
                                             'of players.h (view modes, AI cars, car slots, the pools) to '
                                             'retail\'s (only the arrays move), the '
                                             'image un-relocates to E0 byte for byte except the functions that '
                                             'reached a moved array through $gp (and those the port\'s own '
                                             'defines change, port_changed_functions)'),
                maps={name: rel(maps_dir / name) for name in files},
                functions=dict(total=fm['functions'], identical_to_retail=fm['identical'], by_form=fm['by_form']),
                jump_tables=dict(tables=fm['jump_tables'], entries=fm['jump_table_entries'],
                                 unresolved_jr=fm['unresolved_jr']),
                e0=dict(executable='out/TM2_SOURCE_E0.EXE', json='out/TM2_SOURCE_E0.json', sha256=e0_check['sha256'],
                        note='the differences of E0 from retail (close functions) are in TM2_SOURCE_E0.json; '
                             'they are in this build too'),
                hybrid_sha256=hybrid['sha256'], retail_sha256=RETAIL_SHA256,
                scope='data structures and loops only (docs/SOURCE_BUILD.md "Port variant"): no pad reading for '
                      'players 3-4, no quarter views, no 4-player HUD drawing, no 4-player rules',
                contains_retail_bytes=True)
    tmp = out / 'TM2_SOURCE_PORT.json.tmp'
    tmp.write_text(json.dumps(info, indent=1) + '\n', encoding='utf-8')
    os.replace(tmp, out / 'TM2_SOURCE_PORT.json')
    last = dict(info, published='out/TM2_SOURCE_PORT.EXE', published_json='out/TM2_SOURCE_PORT.json')
    (ROOT / 'LAST_PORT_BUILD.json').write_text(json.dumps(last, indent=2) + '\n', encoding='utf-8')
    print('published out/TM2_SOURCE_PORT.EXE sha256 %s: port base %s, port sections end %s (port RAM end %s), %d arrays '
          'moved, %d functions differ from E0 (%d of them also with the limits forced to 2 / retail: $gp -> absolute); '
          'placement check (limits 2) OK' % (
              digest, pv['base'], info['port_end'], pv['ram_end'], len(pv['arrays']), len(changed), len(changed2)))
    return info


PORT_LOADED_BY = {
    '.port_data': 'the port: copy out/TM2_SOURCE_PORT/port_data.bin here',
    '.port_bss': 'nobody: NOLOAD, the port must zero it (crt0 clears only the retail BSS)',
    '.port_text': 'the port: its recompiler translates out/TM2_SOURCE_PORT/port_text.bin (the port functions, '
                  'functions.tsv kind port); copy it here if the original code must be readable',
    '.port_rodata': 'the port: copy out/TM2_SOURCE_PORT/port_rodata.bin here (constants and jump tables of the '
                    'port functions)'}


def function_maps_rows(path):
    lines = [l for l in Path(path).read_text(encoding='utf-8').splitlines() if l and not l.startswith('#')]
    import csv
    return list(csv.DictReader(lines, delimiter='\t'))


if __name__ == '__main__':
    main()
