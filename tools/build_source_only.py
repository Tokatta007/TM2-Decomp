#!/usr/bin/env python3
"""Source-only TM2 build (milestone M1: retail segments only, no C units yet).

For every segment of config/US/layout.tsv a .s file is generated: initialised
segments `.incbin` their split/bin/ piece (tools/split_retail.py), BSS segments
are `.space` in a NOBITS section. The objects are assembled with
mipsel-none-elf-as, linked with a generated linker script that places each
segment as its own output section at its retail address, converted with
tools/elf_to_psx_exe.py, and the result is checked against the retail sha256.

With --build-dir (milestone M2, the "nonmatching" flavour) every compiled unit
object of that hybrid build (build.py; `--build-dir last` = LAST_BUILD.json) is
linked at its retail address instead, and the retail segments are cut into
pieces around them (tools/gen_link.py). The result is not expected to equal
retail; tools/verify_image.py compares it with the hybrid and with retail.

With --build-dir and --relocatable the same objects are linked in the relocatable
flavour (tools/gen_link.py reloc_plan): everything is laid out sequentially in
retail order from t_addr, `--shift ADDR:N` inserts N bytes of padding before a
retail address, and addrmap.tsv / symbols.tsv / pointer_slots.tsv are written
next to the EXE (tools/reloc_image.py), with functions.tsv / jumptables.tsv and the port
recompiler's inputs (tools/function_maps.py); prove it with tools/verify_reloc.py.

--relocatable --port-variant links the PORT VARIANT (docs/SOURCE_BUILD.md "Port variant"): every unit whose
preprocessed source changes with -DTM2_PORT_VARIANT (include/semantic/players.h: 4 humans, views and HUDs;
--port-limits 2 forces all three to 2, which only moves the arrays) is recompiled with that define,
src/semantic/port_variant_arrays.c is added, the arrays of config/US/port_variant_arrays.tsv go to .port_data /
.port_bss at --port-base (default 0x80200000, outside the EXE), and the functions whose code the variant
changes are laid out with their compiled size (as all-C sections). Prove it against the normal relocatable E0:
tools/verify_reloc.py OUT --e0 E0_OUT (the changed functions and their jump tables are excluded and listed).
The PC port's own code (docs/SOURCE_BUILD.md "Port code"): --port-define NAME[=VALUE] adds a -D to every unit
the variant compiles (units whose preprocessed source changes with it are recompiled; the functions it changes are
reported as port_changed), --port-unit PATH.c adds a unit compiled with the game's compiler without $gp
(PORT_CODE_RECIPE) into the port sections, and any compiled unit may put functions in .port_text (their jump
tables go to .port_rodata), placed after .port_data/.port_bss and published as port_text.bin / port_rodata.bin.
--port-ram-end ADDR (default 0x80C00000): the port sections must end at or below it; the port chooses it, and its
own high RAM must start at or above it (recorded as port_variant.ram_end in report.json).

--relocatable --all-c also replaces the four units whose SingleTrac hand-written asm stays asm in src/
(model emitter, hdp walker, renderer walker, near pass) by their pure-C copies in reference/, compiled with
the recipe of the unit they replace; the C may outgrow the retail slots and pushes later code. Prove it
against the normal relocatable E0: tools/verify_reloc.py OUT --e0 E0_OUT.

Output goes to a new work/source-<timestamp>/ directory every run (or to --output,
which must not exist yet; `build.py --source-only` uses work/build-<timestamp>/source/).
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

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, RETAIL_SHA256, CONFIG, read_layout
import split_retail
from elf_to_psx_exe import convert
from semantic_elf import read_elf

MIPS = ROOT / 'toolchain/mips/bin'
AS = MIPS / 'mipsel-none-elf-as.exe'
LD = MIPS / 'mipsel-none-elf-ld.exe'
SECTION_FLAGS = {
    'code': '"ax",@progbits',
    'rodata': '"a",@progbits',
    'data': '"aw",@progbits',
    'sdata': '"aw",@progbits',
    'bss_in_file': '"aw",@nobits',
    'bss': '"aw",@nobits',
}
NOBITS_KINDS = ('bss_in_file', 'bss')
# Sections the assembler always emits; they must be empty and are discarded.
DISCARD = ('.reginfo', '.MIPS.options', '.MIPS.abiflags', '.pdr', '.comment', '.gnu.attributes',
           '.mdebug*', '.text', '.data', '.bss')


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def tool_env():
    env = dict(os.environ)
    env['PATH'] = str(MIPS) + os.pathsep + str(Path(os.environ.get('SystemRoot', r'C:\Windows')) / 'System32')
    return env


def run(cmd, log, env):
    proc = subprocess.run([str(c) for c in cmd], capture_output=True, cwd=ROOT, env=env, timeout=120)
    with open(log, 'ab') as f:
        f.write(b'$ ' + ' '.join(str(c) for c in cmd).encode() + b'\n' + proc.stdout + proc.stderr)
    if proc.returncode:
        raise SystemExit('build_source_only: %s failed:\n%s' % (Path(cmd[0]).name, (proc.stdout + proc.stderr).decode(errors='replace')))
    return proc


def section_name(seg):
    return '.seg.' + seg['name']


def segment_asm(seg, split_row):
    size = seg['end'] - seg['start']
    lines = ['# Generated by tools/build_source_only.py: %s %08X-%08X (%s)' % (seg['name'], seg['start'], seg['end'], seg['kind']),
             '\t.section %s,%s' % (section_name(seg), SECTION_FLAGS[seg['kind']]),
             '\t.p2align 2',
             '\t.globl seg_%s_start' % seg['name'],
             'seg_%s_start:' % seg['name']]
    if seg['kind'] in NOBITS_KINDS:
        lines.append('\t.space 0x%X' % size)
    else:
        lines.append('\t.incbin "split/%s"' % split_row['file'])
    lines += ['\t.globl seg_%s_end' % seg['name'], 'seg_%s_end:' % seg['name'], '']
    return '\n'.join(lines)


def linker_script(layout, header):
    out = ['/* Generated by tools/build_source_only.py from config/US/layout.tsv. */',
           'OUTPUT_FORMAT("elf32-littlemips")', 'OUTPUT_ARCH(mips)',
           'ENTRY(%s)' % header['pc0']['symbol'], '', 'SECTIONS', '{']
    for seg in layout:
        noload = ' (NOLOAD)' if seg['kind'] in NOBITS_KINDS else ''
        out.append('  %s 0x%08X%s : { KEEP(*(%s)) }' % (section_name(seg), seg['start'], noload, section_name(seg)))
    out.append('  /DISCARD/ : { %s }' % ' '.join('*(%s)' % s for s in DISCARD))
    out += ['}', '',
            '%s = 0x%08X;' % (header['pc0']['symbol'], int(header['pc0']['address'], 16))]
    for seg in layout:
        out.append('ASSERT(ADDR(%s) == 0x%08X && SIZEOF(%s) == 0x%X, "segment %s misplaced")'
                   % (section_name(seg), seg['start'], section_name(seg), seg['end'] - seg['start'], seg['name']))
    return '\n'.join(out) + '\n'


def check_discarded_empty(objects):
    """.text/.data/.bss are discarded by the script; refuse if any object put bytes there."""
    for obj in objects:
        for s in read_elf(obj)['sections']:
            if s['name'] in ('.text', '.data', '.bss') and s['size']:
                raise SystemExit('build_source_only: %s has %d bytes in %s' % (obj.name, s['size'], s['name']))


def build_nonmatching(build_dir, out, log, env, manifest, sdk_from_lib=False):
    """M2: link every unit object of a hybrid build at its retail address, retail pieces around them."""
    import gen_link
    build_dir = gen_link.last_build_dir() if build_dir == 'last' else (ROOT / build_dir).resolve()
    plan = gen_link.plan(build_dir, sdk_from_lib, out)
    rows = {r['name']: r for r in manifest['segments']}
    piece_objs, objects = {}, []
    for seg in read_layout():
        pieces = [p for p in plan['pieces'] if p['segment'] == seg['name']]
        if not pieces:
            continue
        src = out / ('retail_%s.s' % seg['name'])
        obj = out / ('retail_%s.o' % seg['name'])
        src.write_text(gen_link.segment_asm(seg['name'], pieces, rows[seg['name']]), encoding='ascii', newline='\n')
        run([AS, '-EL', '-march=r3000', '-mabi=32', '-no-pad-sections', '-I', ROOT, '-o', obj, src], log, env)
        piece_objs[seg['name']] = obj.relative_to(ROOT).as_posix()
        objects.append(obj)
    check_discarded_empty(objects)
    unit_objs = sorted({r['obj'] for r in plan['sections']} | set(plan['link_objects']))

    (out / 'symbols.ld').write_text(gen_link.symbols_script(plan), encoding='ascii', newline='\n')
    script = out / 'source_only.ld'
    script.write_text(gen_link.linker_script(plan, piece_objs), encoding='ascii', newline='\n')
    (out / 'link_plan.json').write_text(json.dumps(gen_link.plan_json(plan), indent=1) + '\n', encoding='utf-8')
    elf = out / 'TM2_SOURCE.elf'
    aliases = {gen_link.out_name(r): r['alias_of'] for r in plan['sections'] if r.get('alias_of')}
    # ld's overlap check is only disabled for the reported BSS aliases; elf_to_psx_exe still rejects any other overlap.
    check = ['--no-check-sections'] if aliases else []
    run([LD, '-EL', '-nostdlib', '--orphan-handling=error', '--no-warn-rwx-segments'] + check +
        ['-L', out.relative_to(ROOT).as_posix(), '-Map', out / 'TM2_SOURCE.map', '-T', script.relative_to(ROOT).as_posix(),
         '-o', elf] + [o.relative_to(ROOT).as_posix() for o in objects] + unit_objs, log, env)

    exe = out / 'TM2_SOURCE.EXE'
    info = convert(elf, exe, nobits_aliases=aliases)
    digest = sha256(exe)
    report = dict(schema='tm2-source-only-v1', milestone='M2', flavour='nonmatching',
                  executable=exe.relative_to(ROOT).as_posix(), sha256=digest, retail_sha256=RETAIL_SHA256,
                  matches_retail=digest == RETAIL_SHA256, t_size='0x%X' % info['t_size'],
                  build_dir=build_dir.relative_to(ROOT).as_posix(), c_units=len(plan['selection']['units']),
                  c_sections=len(plan['sections']), retail_pieces=len(plan['pieces']), zero_tails=len(plan['tails']),
                  symbol_kinds=plan['symbol_kinds'], warnings=plan['warnings'], problems=plan['problems'],
                  layout_sha256=manifest['layout_sha256'], sdk_from_lib=sdk_report(plan))
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for w in plan['warnings']:
        print('warning:', w)
    for p in plan['problems']:
        print('problem:', p)
    print('%s sha256 %s (%d unit objects, %d C sections, %d retail pieces, %d zero-filled slot tails%s)' % (
        exe.relative_to(ROOT).as_posix(), digest, report['c_units'], report['c_sections'], report['retail_pieces'],
        report['zero_tails'], sdk_summary(plan)))
    print('verify with: py -3.14 tools/verify_image.py %s' % out.relative_to(ROOT).as_posix())


def sdk_report(plan):
    """Coverage of --sdk-from-lib for report.json (None without it)."""
    sdk = plan.get('sdk_lib')
    if not sdk:
        return None
    return dict(coverage=sdk['coverage'], externalized=sdk['externalized'], omitted=sdk['omitted'],
                collisions=sdk['collisions'], modules=len(sdk['modules']), lib_objects=len(sdk['lib_objects']),
                filtered_unit_objects=len(sdk['object_map']), c_override=sdk.get('c_override', []))


def sdk_summary(plan):
    sdk = plan.get('sdk_lib')
    if not sdk:
        return ''
    c = sdk['coverage']
    return '; --sdk-from-lib: %d PsyQ 3.3 modules, %d functions (%d bytes) from .LIB, %d C sections dropped%s' % (
        c['modules'], c['functions'], c['function_bytes'], c['c_sections_dropped'],
        '; %d modules (%d functions) left to C by sdk_c_modules.tsv' % (c['c_override_modules'], c['c_override_functions'])
        if c.get('c_override_modules') else '')


# All-C flavour (docs/SOURCE_BUILD.md): the units whose SingleTrac hand-written asm stays asm in src/ for the
# byte-matching build, and the full pure-C copies of them kept in reference/.
ALL_C_UNITS = {
    'src/semantic/model_emitter.c': 'reference/model_emitter_800CDC54.c',
    'src/semantic/engine_hit_detection.c': 'reference/hdp_walker_engine_hit_detection.c',
    'src/semantic/engine_render_scene.c': 'reference/renderer_walker_engine_render_scene.c',
    'src/semantic/engine_math_near_pass.c': 'reference/near_pass_engine_math_near_pass.c',
}
SECTION_VA = re.compile(r'^\.(?:text|tm2)_(?:string_|init_)?([0-9A-Fa-f]{8})$')


def compile_unit(source, item, cdir, defines=()):
    """Compile `source` with the BUILD_MANIFEST.json recipe `item` (as build.py does) into cdir; the object.
    `defines`: extra -D options (the port variant)."""
    from semantic_compile import compile_source
    row = compile_source(source, cdir, item['delay_slots'], item['preserve_common'], item.get('extern_gp', False),
                         item.get('fvolatile', False), defines=defines)
    if not row['compiled']:
        raise SystemExit('build_source_only: compile of %s failed (%s): see %s' % (
            source.relative_to(ROOT).as_posix(), row.get('failed_stage', 'unknown'), cdir.relative_to(ROOT).as_posix()))
    cplusplus = source.suffix == '.cpp'
    obj = ROOT / row['artifacts']['source.i' if cplusplus else 'source.o']['path']
    if cplusplus and not item.get('compiler'):
        raise SystemExit('build_source_only: C++ unit %s without a PsyQ compiler' % source.relative_to(ROOT).as_posix())
    if item.get('compiler'):
        from match_unit import compile_candidate
        obj, missing = compile_candidate(item['compiler'], obj.parent, item['delay_slots'], extern_gp=item.get('extern_gp', False),
                                          fvolatile=item.get('fvolatile', False), cplusplus=cplusplus,
                                          gp_size=item.get('gp_size', 8), asm_post=item.get('asm_post'), opt=item.get('opt'))
        if missing:
            raise SystemExit('build_source_only: %s: unplaced sections %s' % (source.relative_to(ROOT).as_posix(), ', '.join(missing)))
    return obj


def object_sections(path):
    """{section: (bytes, relocations, size, code)} of the allocated sections, and {global: (section, offset)}."""
    from semantic_elf import read_relocs
    elf = read_elf(path)
    rels = read_relocs(elf)
    secs = {}
    for s in elf['sections']:
        if s['flags'] & 2 and s['name'] != '.MIPS.abiflags' and s['size']:
            r = sorted((o, t, x['name'] or elf['sections'][x['section_index']]['name']) for o, t, x in rels.get(s['index'], []))
            secs[s['name']] = (s['data'], r, s['size'], bool(s['flags'] & 4))
    globs = {s['name']: (elf['sections'][s['section_index']]['name'], s['address']) for s in elf['symbols']
             if s['name'] and s['binding'] in (1, 2) and 0 < s['section_index'] < 0xff00}
    return secs, globs


def prepare_all_c(build_dir, out):
    """Compile the reference/ pure-C units with the recipe of the src/ units they replace and return
    (selection with their objects swapped in, {unit: replaced code sections}, report dict).

    A section is "replaced" when its bytes or relocations differ from the src/ unit's object: these must be
    exactly the hand-written asm functions; every other section of the reference unit must be the same C
    (same bytes, same relocations), so the rest of the image equals the normal relocatable link."""
    from build_maximal_hybrid import symbol_map, address_for
    recipe = {u['source']: u for u in json.loads((ROOT / 'BUILD_MANIFEST.json').read_text(encoding='utf-8'))['units']}
    selection = json.loads((build_dir / 'selection.json').read_text(encoding='utf-8'))
    syms = symbol_map()
    all_c, units, sections, retail_symbols, renamed = {}, [], [], {}, []
    for src, ref in ALL_C_UNITS.items():
        unit = selection['units'][src]
        cdir = out / 'all_c' / Path(ref).stem
        obj = compile_unit(ROOT / ref, recipe[src], cdir)
        old_secs, old_globs = object_sections(ROOT / unit['object'])
        new_secs, new_globs = object_sections(obj)
        lost = sorted(set(old_secs) - set(new_secs))
        if lost:
            raise SystemExit('build_source_only: %s lacks sections of %s: %s' % (ref, src, ', '.join(lost)))
        replaced = sorted(n for n, s in new_secs.items() if n not in old_secs or s[:2] != old_secs[n][:2])
        bad = [n for n in replaced if not new_secs[n][3] or not SECTION_VA.match(n) or n not in old_secs]
        if bad:
            raise SystemExit('build_source_only: %s: only code sections of retail functions may differ from %s, '
                             'not %s' % (ref, src, ', '.join(bad)))
        # retail addresses of the asm unit's globals in the replaced sections (kind handlers, ...)
        for name, (sec, off) in old_globs.items():
            if sec in replaced:
                retail_symbols[name] = '%08X' % (int(SECTION_VA.match(sec).group(1), 16) + off)
        # selection symbols: the reference C may still spell a function by an older name of the same VA
        for va, f in selection['functions'].items():
            if f['source'] != src or f['symbol'] in new_globs:
                continue
            same = [g for g in new_globs if address_for(g, syms) == int(va, 16) and new_globs[g][1] == 0]
            if len(same) != 1:
                raise SystemExit('build_source_only: %s does not define %s (%s)' % (ref, f['symbol'], va))
            renamed.append(dict(va=va, selection=f['symbol'], reference=same[0]))
            f['symbol'] = same[0]
        unit.update(object=obj.relative_to(ROOT).as_posix(), object_sha256=sha256(obj), all_c_source=ref,
                    all_c_source_sha256=sha256(ROOT / ref))
        all_c[src] = replaced
        for n in replaced:
            va = int(SECTION_VA.match(n).group(1), 16)
            funcs = sorted(v for v, f in selection['functions'].items() if f['source'] == src and int(v, 16) == va)
            sections.append(dict(unit=src, reference=ref, section=n, function=funcs[0] if funcs else None,
                                 symbol=selection['functions'][funcs[0]]['symbol'] if funcs else None,
                                 asm_bytes=old_secs[n][2], c_bytes=new_secs[n][2], delta=new_secs[n][2] - old_secs[n][2]))
        units.append(dict(unit=src, reference=ref, object=unit['object'], replaced_sections=replaced,
                          same_sections=len(new_secs) - len(replaced)))
    info = dict(units=units, sections=sorted(sections, key=lambda s: s['section']), retail_symbols=retail_symbols,
                renamed_selection_symbols=renamed,
                asm_bytes=sum(s['asm_bytes'] for s in sections), c_bytes=sum(s['c_bytes'] for s in sections))
    info['delta'] = info['c_bytes'] - info['asm_bytes']
    return selection, all_c, info


# Port variant (docs/SOURCE_BUILD.md "Port variant"): the unit compiled only by the variant, and its recipe
# (plain data: GCC 2.7.2 path, no PsyQ compiler, no $gp).
PORT_UNIT = 'src/semantic/port_variant_arrays.c'
PORT_UNIT_RECIPE = dict(delay_slots='compiler', preserve_common=False, extern_gp=False, fvolatile=False)
# The units a port adds (--port-unit, docs/SOURCE_BUILD.md "Port code"): the game's compiler and flags (PsyQ 3.6
# CC1PSX, -fvolatile, delay slots by the compiler) but no $gp at all (-G0, no extern_gp): the port storage lies
# above 2 MB, out of $gp's reach, so every access is absolute (lui/%lo). Their default sections become the port
# sections (PORT_UNIT_SECTIONS).
PORT_CODE_RECIPE = dict(compiler='psyq3.6', delay_slots='compiler', preserve_common=False, extern_gp=False,
                        fvolatile=True, gp_size=0)
PORT_UNIT_SECTIONS = {'.text': '.port_text', '.rdata': '.port_rodata', '.rodata': '.port_rodata',
                      '.data': '.port_data', '.bss': '.port_bss'}
OBJCOPY = MIPS / 'mipsel-none-elf-objcopy.exe'


# players.h macro of each limit (the groups of config/US/port_variant_arrays.tsv, gen_link.RETAIL_LIMITS)
PORT_LIMIT_MACROS = {'humans': 'TM2_MAX_HUMANS', 'views': 'TM2_MAX_VIEWS', 'huds': 'TM2_MAX_HUDS',
                     'viewmodes': 'TM2_MAX_VIEW_MODES', 'ai': 'TM2_MAX_AI_CARS', 'slots': 'TM2_MAX_CAR_SLOTS',
                     'carsloaded': 'TM2_MAX_LOADED_CARS', 'dmdarea': 'TM2_PORT_CAR_DMD_AREA_BYTES',
                     'arcs': 'TM2_MAX_ARCS', 'shields': 'TM2_MAX_SHIELDS', 'remotebombs': 'TM2_MAX_REMOTE_BOMBS',
                     'ricochets': 'TM2_MAX_RICOCHETS', 'flames': 'TM2_MAX_FLAME_EMITTERS',
                     'slamrecords': 'TM2_MAX_SLAM_RECORDS', 'worldmodels': 'TM2_MAX_WORLD_MODELS',
                     'clutdefs': 'TM2_MAX_CLUT_ANIM_DEFS', 'clutanims': 'TM2_MAX_CLUT_ANIMS',
                     'radarblips': 'TM2_MAX_RADAR_BLIPS', 'projectiles': 'TM2_MAX_PROJECTILES',
                     'coordsys': 'TM2_MAX_COORD_SYSTEMS', 'soundinsts': 'TM2_MAX_SOUND_INSTANCES',
                     'sndreqs': 'TM2_MAX_SOUND_REQUESTS',
                     'spuvoices': 'TM2_SPU_VOICES',  # (spuvoices: spu_voices.h, request 34)
                     'drops': 'TM2_MAX_DROPS', 'mines': 'TM2_MAX_MINE_OBJECTS',  # (request 40, TM2_PORT_BALANCE)
                     'fxsprites': 'TM2_MAX_FX_SPRITES',  # (request 102, TM2_PORT_CARS)
                     'renderslots': 'TM2_MAX_RENDER_SLOTS'}  # (render_slots.h, request 47, TM2_PORT_RENDER_SLOTS)


def placement_limits(value):
    """--port-limits N (the placement check, N = 2): humans, views and HUDs forced to N, every other limit to
    retail's (view modes 9, AI cars 9, car slots 15, the pools): only the arrays move."""
    import gen_link
    limits = dict(gen_link.RETAIL_LIMITS)
    limits.update(humans=value, views=value, huds=value)
    return limits


def port_defines(limits=None, extra=()):
    """-D options of the port variant: TM2_PORT_VARIANT, plus forced limits ({'humans': 2, ...}) if given, plus
    the port's own defines (--port-define NAME[=VALUE], e.g. TM2_PORT_HOOKS) last."""
    out = ['TM2_PORT_VARIANT']
    for group, value in sorted((limits or {}).items()):
        out.append('%s=%d' % (PORT_LIMIT_MACROS[group], value))
    for d in extra:
        if not re.match(r'^[A-Za-z_]\w*(=.*)?$', d) or d.split('=')[0] in [o.split('=')[0] for o in out]:
            raise SystemExit('build_source_only: bad or repeated --port-define %r' % d)
        out.append(d)
    return out


def port_unit_object(obj, dst, unit):
    """A --port-unit object with its default sections renamed to the port sections (PORT_UNIT_SECTIONS):
    everything a port unit compiles is port code or port storage. Returns dst."""
    elf = read_elf(obj)
    secs = {}
    for s in elf['sections']:
        secs.setdefault(s['name'], []).append(s)
    has_syms = {elf['sections'][x['section_index']]['name'] for x in elf['symbols']
                if x['name'] and 0 < x['section_index'] < 0xff00 and x['binding'] in (1, 2)}
    common = sorted(x['name'] for x in elf['symbols'] if x['section_index'] == 0xFFF2)
    if common:
        raise SystemExit('build_source_only: %s: common symbols %s (give them an initializer, e.g. = {0})' % (
            unit, ', '.join(common)))
    for small in ('.sdata', '.sbss'):
        if any(s['size'] for s in secs.get(small, [])):
            raise SystemExit('build_source_only: %s has %s (port units are compiled without $gp)' % (unit, small))
    renamed = {old: new for old, new in PORT_UNIT_SECTIONS.items()
               if any(s['size'] for s in secs.get(old, [])) or old in has_syms}  # (empty: left to the discarded defaults)
    args = []
    for old, new in renamed.items():
        if new in secs or list(renamed.values()).count(new) > 1:
            raise SystemExit('build_source_only: %s: %s would join another %s: in a port unit leave code, constants '
                             'and initialised data without section attributes, or give every one its port section'
                             % (unit, old, new))
        args += ['--rename-section', '%s=%s' % (old, new)]
    dst.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run([str(OBJCOPY)] + args + [str(obj), str(dst)], capture_output=True, cwd=ROOT, env=tool_env())
    if proc.returncode:
        raise SystemExit('build_source_only: objcopy of %s failed: %s' % (unit, (proc.stdout + proc.stderr).decode(errors='replace')))
    return dst


def port_limits(defines):
    """The limits players.h gives for these defines (read back from the preprocessor, not assumed)."""
    from semantic_compile import preprocess
    import tempfile
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        src = Path(tmp) / 'limits.c'
        src.write_text('#include "semantic/players.h"\n#include "semantic/spu_voices.h"\n'
                       '#include "semantic/render_slots.h"\n' + ''.join(
            'int limit_%s = %s;\n' % (k, m) for k, m in sorted(PORT_LIMIT_MACROS.items())), encoding='ascii', newline='\n')
        text = preprocess(src, Path(tmp) / 'limits.i', defines).decode('ascii', errors='replace')
    out = {}
    for k, expr in re.findall(r'int limit_(\w+) = ([^;]+);', text):
        if not re.fullmatch(r'[\dxXa-fA-F\s()+*-]+', expr):
            raise SystemExit('build_source_only: limit %s did not preprocess to a number: %r' % (k, expr))
        out[k] = int(eval(expr, {'__builtins__': {}}))
    if set(out) != set(PORT_LIMIT_MACROS):
        raise SystemExit('build_source_only: could not read the limits back from players.h: %r' % out)
    return out


def changed_sections(src, old_secs, new_secs, old_globs=None, moved=(), lost=None):
    """Sections of a recompiled unit that differ from `old_secs` (see prepare_port_variant): the names of the
    changed code sections and jump tables. Port sections (gen_link.PORT_SECTIONS) are the port's own storage
    and code: they may be added, and are not compared."""
    import gen_link
    unit_changed = []
    for name in sorted(set(old_secs) | set(new_secs)):
        old, new = old_secs.get(name), new_secs.get(name)
        if name in gen_link.PORT_SECTIONS:
            if old is not None and new is None:
                raise SystemExit('build_source_only: %s: the variant loses port section %s' % (src, name))
            continue
        if new is None:
            held = sorted(g for g, (sec, _) in (old_globs or {}).items() if sec == name)
            if old[3] or not held or set(held) - set(moved) or lost is None:
                raise SystemExit('build_source_only: %s: the variant loses section %s (%s), which holds more than '
                                 'port arrays' % (src, name, ', '.join(held) or 'no globals'))
            lost.append(dict(unit=src, section=name, globals=held))
            continue
        if old is None:
            raise SystemExit('build_source_only: %s: the variant adds section %s (a port function goes to .port_text, '
                             'its constants to .port_rodata)' % (src, name))
        if old[:2] == new[:2]:
            continue
        if new[3]:
            # .text: a unit whose one function has no section attribute (weapon_flame_update.c); its
            # function is found by symbol (prepare_port_variant)
            if not SECTION_VA.match(name) and name != '.text':
                raise SystemExit('build_source_only: %s: changed code section %s is not a retail function' % (src, name))
            unit_changed.append(name)
    for name in sorted(set(old_secs) & set(new_secs)):
        old, new = old_secs[name], new_secs[name]
        if new[3] or name in gen_link.PORT_SECTIONS:
            continue
        targets = {t for _, _, t in new[1]}
        if old[:2] == new[:2] and not targets & set(unit_changed):
            continue
        # A jump table of a changed function (its words point into the new code). One retail rodata section may
        # hold the tables of several functions (tournament_tables.c: .tm2_800CFC38 for tournamentOpponentCount
        # and carIndexToType): every word that differs, and every word into a changed function, must be a
        # relocation into a changed function; the other words stay as they were (verify_reloc checks them as
        # ordinary pointer slots).
        oldr = {o: (t, n) for o, t, n in old[1]}
        newr = {o: (t, n) for o, t, n in new[1]}
        bad = old[2] != new[2] or not SECTION_VA.match(name) or not targets & set(unit_changed)
        for o in range(0, max(len(old[0]), len(new[0])), 4):
            if old[0][o:o + 4] == new[0][o:o + 4] and oldr.get(o) == newr.get(o):
                continue
            if o not in newr or newr[o][1] not in unit_changed:
                bad = True
        if bad:
            raise SystemExit('build_source_only: %s: the variant changes data section %s (only jump tables of '
                             'changed functions may change)' % (src, name))
        unit_changed.append(name)
    return unit_changed


def prepare_port_variant(build_dir, out, defines, extra_defines=(), port_units=()):
    """Compile the port variant: every unit whose preprocessed source differs with `defines` is recompiled with
    them (its BUILD_MANIFEST.json recipe), port_variant_arrays.c is added, and the port's own units
    (`port_units`, --port-unit) with PORT_CODE_RECIPE. Returns (selection, {unit: changed sections}, report dict).

    Per recompiled unit, compared with the build's object: a code section whose bytes or relocations differ is
    "changed" (a function the variant rewrites, laid out with its compiled size); a lost section must be data
    holding only arrays of config/US/port_variant_arrays.tsv (their retail definitions, left out by the variant);
    a data section that differs must be a jump table whose relocations all point into changed code (it is
    excluded from the proof with its function); port sections (.port_data/.port_bss/.port_text/.port_rodata)
    may be added. Anything else is an error.

    `extra_defines` (the port's --port-define, already in `defines`): a unit whose preprocessed source also
    differs between `defines` and `defines` without them is changed by the port; its changed sections are
    attributed by comparing with the unit compiled without them (or with the build's object when the decomp's
    own defines leave the unit alone). They are reported as `port_changed` (build.py's placement check
    accepts them besides the $gp functions)."""
    import gen_link
    from semantic_compile import preprocess
    recipe = {u['source']: u for u in json.loads((ROOT / 'BUILD_MANIFEST.json').read_text(encoding='utf-8'))['units']}
    selection = json.loads((build_dir / 'selection.json').read_text(encoding='utf-8'))
    moved = {a['name'] for a in gen_link.read_port_arrays()}
    vdir = out / 'port_variant'
    decomp_defines = [d for d in defines if d not in extra_defines]
    changed, units, sections, lost = {}, [], [], []
    for index, (src, unit) in enumerate(selection['units'].items()):
        path = ROOT / src
        stem = '%04d_%s' % (index, path.stem)
        base_i = preprocess(path, vdir / 'pp' / (stem + '.i'), ())
        var_i = preprocess(path, vdir / 'pp' / (stem + '.variant.i'), defines)
        if base_i == var_i:
            continue
        obj = compile_unit(path, recipe[src], vdir / 'compile' / stem, defines)
        old_secs, old_globs = object_sections(ROOT / unit['object'])
        new_secs, new_globs = object_sections(obj)
        unit_changed = changed_sections(src, old_secs, new_secs, old_globs, moved, lost)
        port_changed = []
        if extra_defines:
            dec_i = preprocess(path, vdir / 'pp' / (stem + '.decomp.i'), decomp_defines)
            if dec_i != var_i:  # the port's defines change this unit: which sections?
                if dec_i == base_i:
                    ref_secs, ref_globs = old_secs, old_globs
                else:
                    ref = compile_unit(path, recipe[src], vdir / 'compile_decomp' / stem, decomp_defines)
                    ref_secs, ref_globs = object_sections(ref)
                ref_lost = []
                port_changed = changed_sections(src, ref_secs, new_secs, ref_globs, moved, ref_lost)
        unit.update(object=obj.relative_to(ROOT).as_posix(), object_sha256=sha256(obj), source_sha256=sha256(path),
                    port_variant_defines=list(defines))
        units.append(dict(unit=src, object=unit['object'], changed_sections=unit_changed,
                          **({'port_changed_sections': port_changed} if extra_defines else {}),
                          port_sections=sorted(n for n in new_secs if n in gen_link.PORT_SECTIONS)))
        if unit_changed:
            changed[src] = unit_changed
        def section_va(n):
            m = SECTION_VA.match(n)
            if m:
                return int(m.group(1), 16)
            fs = [int(v, 16) for v, f in selection['functions'].items() if f['source'] == src
                  and new_globs.get(f['symbol'], (None,))[0] == n]
            if len(fs) != 1:
                raise SystemExit('build_source_only: %s: changed section %s holds %d retail functions (need 1)' % (
                    src, n, len(fs)))
            return fs[0]
        for n in unit_changed:
            va = section_va(n)
            code = new_secs[n][3]
            if not code:  # a jump table: attribute it to the changed function its words point into
                va = section_va(sorted({t for _, _, t in new_secs[n][1]} & set(unit_changed))[0])
            funcs = sorted(v for v, f in selection['functions'].items() if f['source'] == src and int(v, 16) == va)
            sections.append(dict(unit=src, section=n, code=code, function=funcs[0] if funcs else None,
                                 symbol=selection['functions'][funcs[0]]['symbol'] if funcs else None,
                                 retail_bytes=old_secs[n][2], variant_bytes=new_secs[n][2],
                                 delta=new_secs[n][2] - old_secs[n][2],
                                 **({'port_changed': n in port_changed} if extra_defines else {})))
    # the port storage itself
    path = ROOT / PORT_UNIT
    obj = compile_unit(path, PORT_UNIT_RECIPE, vdir / 'compile' / ('port_' + path.stem), defines)
    selection['units'][PORT_UNIT] = dict(source=PORT_UNIT, kind='port_variant', object=obj.relative_to(ROOT).as_posix(),
                                         object_sha256=sha256(obj), source_sha256=sha256(path),
                                         port_variant_defines=list(defines))
    units.append(dict(unit=PORT_UNIT, object=selection['units'][PORT_UNIT]['object'], changed_sections=[]))
    # the port's own units (--port-unit): port code and storage only
    added = []
    for i, rel in enumerate(port_units):
        path = ROOT / rel
        if rel in selection['units']:
            raise SystemExit('build_source_only: --port-unit %s is already a unit of the build' % rel)
        cdir = vdir / 'compile' / ('portunit_%02d_%s' % (i, path.stem))
        obj = port_unit_object(compile_unit(path, PORT_CODE_RECIPE, cdir, defines), cdir / 'port_unit.o', rel)
        secs = object_sections(obj)[0]
        other = sorted(n for n in secs if n not in gen_link.PORT_SECTIONS)
        if other:
            raise SystemExit('build_source_only: port unit %s has sections %s (only port sections)' % (rel, ', '.join(other)))
        selection['units'][rel] = dict(source=rel, kind='port_unit', object=obj.relative_to(ROOT).as_posix(),
                                       object_sha256=sha256(obj), source_sha256=sha256(path),
                                       port_variant_defines=list(defines), recipe=PORT_CODE_RECIPE)
        units.append(dict(unit=rel, object=selection['units'][rel]['object'], changed_sections=[],
                          port_sections=sorted(secs)))
        added.append(rel)
    info = dict(defines=list(defines), recompiled_units=units, changed_sections=sorted(sections, key=lambda s: s['section']),
                lost_sections=lost, port_unit=PORT_UNIT)
    info.update(port_defines=list(extra_defines), port_units=added,
                port_changed_sections=[s for s in info['changed_sections'] if s.get('port_changed')])
    return selection, changed, info


def build_relocatable(build_dir, out, log, env, manifest, shift=None, corrupt=(), sdk_from_lib=False, all_c=False,
                      port_variant=None):
    """Relocatable flavour: the M2 objects and retail pieces laid out sequentially in retail order (no fixed
    addresses except t_addr), retail pieces as labelled .s with symbolic pointer words; writes addrmap.tsv,
    symbols.tsv and pointer_slots.tsv next to the EXE (tools/reloc_image.py), and functions.tsv, jumptables.tsv,
    functions_ida.tsv, names_ida.tsv and recomp_inventory.json (tools/function_maps.py).

    all_c: the units with SingleTrac hand-written asm are replaced by their pure-C copies in reference/
    (ALL_C_UNITS), compiled here with the same recipe; prove with verify_reloc.py OUT --e0 <normal E0>.

    port_variant: dict(base=address, limits=None or placement_limits(2)) links the port variant
    (prepare_port_variant); prove it with verify_reloc.py OUT --e0 <normal E0>."""
    import gen_link
    import reloc_image
    build_dir = gen_link.last_build_dir() if build_dir == 'last' else (ROOT / build_dir).resolve()
    allc_info = port_info = None
    if port_variant is not None and all_c:
        raise SystemExit('build_source_only: --port-variant and --all-c are separate flavours')
    if port_variant is not None:
        extra = list(port_variant.get('defines') or ())
        defines = port_defines(port_variant.get('limits'), extra)
        limits = port_limits(defines)
        selection, changed, port_info = prepare_port_variant(build_dir, out, defines, extra,
                                                             port_variant.get('units') or ())
        (out / 'port_variant' / 'selection.json').write_text(json.dumps(selection, indent=2) + '\n', encoding='utf-8')
        # per-module override of --sdk-from-lib (config/US/sdk_c_modules.tsv): with e.g. TM2_PORT_SPU48 the
        # libspu/libsnd modules come from the C units (recompiled with the defines), not from the .LIB
        keep_c = gen_link.sdk_c_override(defines) if sdk_from_lib else set()
        port_info['sdk_c_override'] = sorted('%s:%s' % k for k in keep_c)
        plan = gen_link.plan(build_dir, sdk_from_lib, out, selection=selection, all_c=changed,
                             port=dict(base=port_variant['base'], ram_end=port_variant.get('ram_end'), limits=limits,
                                       arrays=gen_link.read_port_arrays()), sdk_keep_c=keep_c)
        if plan['all_c_labels']:
            raise SystemExit('build_source_only: imports point inside functions the variant changes: %s' % ', '.join(
                l['name'] for l in plan['all_c_labels']))
        port_info['limits'] = limits
    elif all_c:
        selection, replaced, allc_info = prepare_all_c(build_dir, out)
        (out / 'all_c' / 'selection.json').write_text(json.dumps(selection, indent=2) + '\n', encoding='utf-8')
        plan = gen_link.plan(build_dir, sdk_from_lib, out, selection=selection, all_c=replaced)
        allc_info['labels'] = plan['all_c_labels']
        for lab in plan['all_c_labels']:  # retail asm labels now standing for a replaced function's entry
            allc_info['retail_symbols'][lab['name']] = lab['address']
    else:
        plan = gen_link.plan(build_dir, sdk_from_lib, out)
    rp = gen_link.reloc_plan(plan, gen_link.parse_shift(shift) if shift else None, corrupt)
    rows = {r['name']: r for r in manifest['segments']}
    piece_objs, objects = {}, []
    for seg in read_layout():
        if not any(it['seg'] == seg['name'] and it['type'] in ('r', 'tail', 'aux') for it in rp['items']):
            continue
        src = out / ('reloc_%s.s' % seg['name'])
        obj = out / ('reloc_%s.o' % seg['name'])
        src.write_text(gen_link.reloc_segment_asm(seg['name'], rp, rows[seg['name']]), encoding='ascii', newline='\n')
        run([AS, '-EL', '-march=r3000', '-mabi=32', '-no-pad-sections', '-I', ROOT, '-o', obj, src], log, env)
        piece_objs[seg['name']] = obj.relative_to(ROOT).as_posix()
        objects.append(obj)
    check_discarded_empty(objects)
    unit_objs = sorted({r['obj'] for r in plan['sections']} | set(plan['link_objects']))

    (out / 'symbols.ld').write_text(gen_link.reloc_symbols_script(plan, rp), encoding='ascii', newline='\n')
    script = out / 'source_only.ld'
    script.write_text(gen_link.reloc_linker_script(plan, rp, piece_objs), encoding='ascii', newline='\n')
    plan_json = gen_link.reloc_plan_json(plan, rp)
    plan_json['piece_objs'] = piece_objs
    plan_json['sdk_from_lib'] = bool(plan.get('sdk_lib'))
    plan_json['all_c'] = allc_info
    plan_json['port_variant'] = port_info
    if port_info is not None:
        plan_json['selection'] = (out / 'port_variant' / 'selection.json').relative_to(ROOT).as_posix()
    (out / 'reloc_plan.json').write_text(json.dumps(plan_json, indent=1) + '\n', encoding='utf-8')
    elf = out / 'TM2_SOURCE.elf'
    aliases = {it['name']: it['alias_of'] for it in rp['items'] if it.get('alias_of')}
    check = ['--no-check-sections'] if aliases else []
    # the object list goes through a response file: the port variant's recompiled objects (long paths under
    # the output directory) pass Windows' 32 KB command-line limit
    rsp = out / 'link_objects.rsp'
    rsp.write_text('\n'.join('"%s"' % str(x) for x in [o.relative_to(ROOT).as_posix() for o in objects] + unit_objs) + '\n',
                   encoding='utf-8', newline='\n')
    run([LD, '-EL', '-nostdlib', '--orphan-handling=error', '--no-warn-rwx-segments'] + check +
        ['-L', out.relative_to(ROOT).as_posix(), '-Map', out / 'TM2_SOURCE.map', '-T', script.relative_to(ROOT).as_posix(),
         '-o', elf, '@' + rsp.relative_to(ROOT).as_posix()], log, env)
    exe = out / 'TM2_SOURCE.EXE'
    info = convert(elf, exe, nobits_aliases=aliases, relocated=True,
                   port_base=port_variant['base'] if port_variant is not None else None,
                   stack=next((a['name'], a['size']) for a in plan['port']['arrays'] if a['name'] == 'portStack')
                   if port_variant is not None else None)
    digest = sha256(exe)
    if port_variant is not None:
        port_info['sections'] = write_port_outputs(out, info['port_sections'], plan['port'])
        port_info['stack'] = info['stack']  # request 89: crt0's sp = the end of portStack (verify_reloc undoes it)
        plan_json['port_variant'] = port_info
        (out / 'reloc_plan.json').write_text(json.dumps(plan_json, indent=1) + '\n', encoding='utf-8')
    maps = reloc_image.build_maps(out)
    import function_maps  # functions.tsv, jumptables.tsv and the port's recompiler inputs, next to addrmap.tsv
    maps['function_maps'] = function_maps.write_maps(out)
    kinds = {}
    for it in rp['items']:
        kinds[it['type']] = kinds.get(it['type'], 0) + 1
    report = dict(schema='tm2-source-only-v1', milestone='M2', flavour='relocatable',
                  executable=exe.relative_to(ROOT).as_posix(), sha256=digest, retail_sha256=RETAIL_SHA256,
                  matches_retail=digest == RETAIL_SHA256, t_size='0x%X' % info['t_size'], pc0='%08X' % info['pc0'],
                  shift=rp['shift'], corrupted_slots=rp['corrupted'],
                  build_dir=build_dir.relative_to(ROOT).as_posix(), c_units=len(plan['selection']['units']),
                  c_sections=len(plan['sections']), retail_pieces=kinds.get('r', 0), zero_tails=kinds.get('tail', 0),
                  labels=sum(len(v) for offs in rp['labels'].values() for v in offs.values()),
                  generated_pointer_words=sum(len(w) for w in rp['words'].values()),
                  object_pointer_slots=sum(1 for s in rp['slots'] if s['via'] == 'object'),
                  absolute_symbols_kept=len(rp['kept_absolute']), helper_symbols=len(rp['helpers']),
                  maps=maps, warnings=plan['warnings'], problems=plan['problems'] + maps['reloc_problems'],
                  layout_sha256=manifest['layout_sha256'], sdk_from_lib=sdk_report(plan),
                  all_c=all_c_report(allc_info, maps), port_variant=port_report(port_info, plan))
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for w in plan['warnings']:
        print('warning:', w)
    if allc_info:
        print('all-C: %d replaced sections, asm %d bytes -> C %d bytes (%+d); %d retail asm labels now name their '
              'function entry' % (len(allc_info['sections']), allc_info['asm_bytes'], allc_info['c_bytes'],
                                  allc_info['delta'], len(allc_info['labels'])))
    if port_info:
        print('port variant (%s): %d units recompiled, %d sections changed, %d arrays moved to %08X (%s); port '
              'sections end %08X, port RAM end %08X' % (
            ' '.join(port_info['defines']), len(port_info['recompiled_units']), len(port_info['changed_sections']),
            len(plan['port']['arrays']), plan['port']['base'],
            ', '.join('%s %s +0x%X' % (s['name'], s['address'], s['size']) for s in port_info['sections']),
            max([int(s['address'], 16) + s['size'] for s in port_info['sections']] or [plan['port']['base']]),
            plan['port']['ram_end']))
        pf = maps['function_maps'].get('port_functions', [])
        if pf or port_info['port_units'] or port_info['port_defines']:
            print('port code: %d port functions, %d port jump tables; %d sections changed by the port\'s defines; '
                  'port units: %s' % (len(pf), len(maps['function_maps'].get('port_jump_tables', [])),
                                      len(port_info['port_changed_sections']), ', '.join(port_info['port_units']) or 'none'))
    for p in report['problems']:
        print('problem:', p)
    print('%s sha256 %s (relocatable%s: t_size %s, pc0 %s, gp %s, BSS end %s; %d labels, %d pointer words, map %s)' % (
        exe.relative_to(ROOT).as_posix(), digest,
        ', shift %s:%d' % (rp['shift']['address'], rp['shift']['bytes']) if rp['shift'] else '',
        report['t_size'], report['pc0'], maps['gp_c'], maps['bss_end_c'], report['labels'],
        report['generated_pointer_words'], 'identity' if maps['identity'] else 'shifted') + sdk_summary(plan))
    if report['problems']:
        raise SystemExit('build_source_only: relocatable link has problems (see report.json)')
    print('verify with: py -3.14 tools/verify_reloc.py %s%s' % (out.relative_to(ROOT).as_posix(),
                                                              ' --e0 <relocatable E0 of the same objects>'
                                                              if all_c or port_variant is not None else ''))


PORT_BINS = {'.port_data': 'port_data.bin', '.port_text': 'port_text.bin', '.port_rodata': 'port_rodata.bin'}


def write_port_outputs(out, port_sections, port):
    """port_data.bin, port_text.bin and port_rodata.bin (the bytes of .port_data / .port_text / .port_rodata;
    always written, 0 bytes when the section is empty) next to the EXE; returns the section list for
    report.json: the four port sections in link order, at the addresses gen_link planned (an empty one: size
    0 at the address it would start at)."""
    linked = {s['name']: s for s in port_sections}
    unknown = sorted(set(linked) - set(port['addresses']))
    if unknown:
        raise SystemExit('build_source_only: sections at the port base that are not port sections: ' + ', '.join(unknown))
    secs = []
    for name, addr in port['addresses'].items():
        s = linked.get(name)
        size = s['size'] if s else 0
        if size != port['sizes'][name] or (s and s['address'] != addr):
            raise SystemExit('build_source_only: %s linked at %08X +0x%X, planned %08X +0x%X' % (
                name, s['address'] if s else 0, size, addr, port['sizes'][name]))
        secs.append(dict(name=name, address='%08X' % addr, size=size, nobits=name == '.port_bss'))
        if name in PORT_BINS:
            (out / PORT_BINS[name]).write_bytes(s['data'] if s else b'')
    return secs


def port_report(info, plan):
    """report.json summary of a port-variant link (None otherwise)."""
    if not info:
        return None
    p = plan['port']
    rep = dict(defines=info['defines'], limits=info['limits'], base='%08X' % p['base'], ram_end='%08X' % p['ram_end'],
               sections=info['sections'],
               arrays=p['arrays'], redirects=p['redirects'], changed_sections=info['changed_sections'],
               lost_sections=info['lost_sections'], recompiled_units=len(info['recompiled_units']),
               sdk_c_override=info.get('sdk_c_override', []))
    # the port's own code (--port-define / --port-unit) and what each object puts in the port sections
    rep.update(port_defines=info['port_defines'], port_units=info['port_units'],
               port_changed_sections=info['port_changed_sections'],
               port_inputs=[dict(i, offset='0x%X' % i['offset']) for i in p['inputs']])
    return rep


def all_c_report(info, maps):
    """report.json summary of an all-C link (None otherwise)."""
    if not info:
        return None
    return dict(units=info['units'], sections=info['sections'], asm_bytes=info['asm_bytes'], c_bytes=info['c_bytes'],
                delta=info['delta'], labels=len(info['labels']), renamed_selection_symbols=info['renamed_selection_symbols'],
                bss_end_c=maps['bss_end_c'], gp_c=maps['gp_c'], entry_c=maps['entry_c'])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--build-dir', help='hybrid build directory whose unit objects are linked (M2); "last" = LAST_BUILD.json')
    parser.add_argument('--output', help='output directory (must not exist; default: a new work/source-<timestamp>/)')
    parser.add_argument('--relocatable', action='store_true',
                        help='with --build-dir: sequential relocatable layout (only t_addr fixed) + addrmap/symbols/pointer_slots')
    parser.add_argument('--shift', help='with --relocatable: ADDR:N inserts N bytes of padding before retail address ADDR')
    parser.add_argument('--sdk-from-lib', action='store_true',
                        help='with --build-dir: link every PsyQ module that config/US/sdk_modules.tsv marks exact from '
                             'the local PsyQ 3.3 .LIB (tools/lnk2elf.py) instead of the C sections of its functions')
    parser.add_argument('--all-c', action='store_true',
                        help='with --relocatable: link the pure-C reference/ versions of the hand-written asm units '
                             '(ALL_C_UNITS) instead of the asm')
    parser.add_argument('--port-variant', action='store_true',
                        help='with --relocatable: the port variant (-DTM2_PORT_VARIANT; docs/SOURCE_BUILD.md "Port variant")')
    parser.add_argument('--port-base', default='80200000',
                        help='with --port-variant: address of .port_data/.port_bss (hex, default 80200000)')
    parser.add_argument('--port-ram-end', default=None, metavar='ADDR',
                        help='with --port-variant: the port sections must end at or below this address (hex, default '
                             '80C00000, tools/gen_link.py DEFAULT_PORT_RAM_END). The PORT chooses it: its own high RAM '
                             'must start at or above it (docs/SOURCE_BUILD.md "Port code")')
    parser.add_argument('--port-limits', type=int, choices=(2,),
                        help='with --port-variant: force TM2_MAX_HUMANS/VIEWS/HUDS to this value and every other limit of players.h '
                             '(view modes, AI cars, car slots, the pools) to retail\'s (2: only the arrays move)')
    parser.add_argument('--port-define', action='append', default=[], metavar='NAME[=VALUE]',
                        help='with --port-variant (repeatable): one more -D for every unit the variant compiles, e.g. '
                             'TM2_PORT_HOOKS (the PC port\'s own changes; docs/SOURCE_BUILD.md "Port code")')
    parser.add_argument('--port-unit', action='append', default=[], metavar='PATH.c',
                        help='with --port-variant (repeatable): a C unit of the port\'s, under this tree, compiled with '
                             'the port code recipe (PsyQ 3.6, -fvolatile, no $gp) into .port_text/.port_rodata/'
                             '.port_data/.port_bss')
    parser.add_argument('--corrupt-slot', action='append', default=[],
                        help='negative control: keep the raw retail word (no relocation) for this data_relocs slot (hex)')
    args = parser.parse_args()
    if args.relocatable and not args.build_dir:
        parser.error('--relocatable needs --build-dir')
    if args.sdk_from_lib and not args.build_dir:
        parser.error('--sdk-from-lib needs --build-dir')
    if (args.shift or args.corrupt_slot or args.all_c or args.port_variant) and not args.relocatable:
        parser.error('--shift/--corrupt-slot/--all-c/--port-variant need --relocatable')
    if (args.port_limits or args.port_define or args.port_unit or args.port_ram_end) and not args.port_variant:
        parser.error('--port-limits/--port-define/--port-unit/--port-ram-end need --port-variant')
    port_units = []
    for u in args.port_unit:
        p = (Path(u) if Path(u).is_absolute() else ROOT / u).resolve()
        if not p.is_file() or not p.is_relative_to(ROOT) or p.suffix not in ('.c', '.cpp'):
            parser.error('--port-unit %s: not a .c/.cpp file under %s' % (u, ROOT))
        rel = p.relative_to(ROOT).as_posix()
        if rel in port_units or rel == PORT_UNIT:
            parser.error('--port-unit %s given twice (or the variant\'s own unit)' % u)
        port_units.append(rel)
    if args.output:
        out = Path(args.output) if Path(args.output).is_absolute() else ROOT / args.output
    else:
        out = ROOT / 'work' / datetime.now(timezone.utc).strftime('source-%Y%m%d-%H%M%S-%f')
    out.mkdir(parents=True, exist_ok=False)
    log = out / 'build.log'
    env = tool_env()

    manifest = split_retail.check()
    if manifest is None:
        print('split/ missing or stale: running split_retail')
        manifest = split_retail.split()
    if args.relocatable:
        port = None
        if args.port_variant:
            port = dict(base=int(args.port_base, 16),
                        ram_end=int(args.port_ram_end, 16) if args.port_ram_end else None,
                        limits=None if not args.port_limits else placement_limits(args.port_limits),
                        defines=args.port_define, units=port_units)
        return build_relocatable(args.build_dir, out, log, env, manifest, args.shift,
                                 [int(a, 16) for a in args.corrupt_slot], args.sdk_from_lib, args.all_c, port)
    if args.build_dir:
        return build_nonmatching(args.build_dir, out, log, env, manifest, args.sdk_from_lib)
    rows = {r['name']: r for r in manifest['segments']}
    layout = read_layout()
    header = json.loads((CONFIG / 'exe_header.json').read_text(encoding='ascii'))

    objects = []
    for seg in layout:
        src = out / ('seg_%s.s' % seg['name'])
        obj = out / ('seg_%s.o' % seg['name'])
        src.write_text(segment_asm(seg, rows[seg['name']]), encoding='ascii', newline='\n')
        run([AS, '-EL', '-march=r3000', '-mabi=32', '-no-pad-sections', '-I', ROOT, '-o', obj, src], log, env)
        objects.append(obj)
    check_discarded_empty(objects)

    script = out / 'source_only.ld'
    script.write_text(linker_script(layout, header), encoding='ascii', newline='\n')
    elf = out / 'TM2_SOURCE.elf'
    run([LD, '-EL', '-nostdlib', '--orphan-handling=error', '--no-warn-rwx-segments',
         '-Map', out / 'TM2_SOURCE.map', '-T', script, '-o', elf] + objects, log, env)

    exe = out / 'TM2_SOURCE.EXE'
    info = convert(elf, exe)
    digest = sha256(exe)
    match = digest == RETAIL_SHA256
    report = dict(schema='tm2-source-only-v1', milestone='M1', executable=exe.relative_to(ROOT).as_posix(),
                  sha256=digest, retail_sha256=RETAIL_SHA256, matches_retail=match,
                  t_size='0x%X' % info['t_size'], c_units=0,
                  segments=[dict(name=s['name'], start='%08X' % s['start'], end='%08X' % s['end'], kind=s['kind'],
                                 source='split/' + rows[s['name']]['file'] if rows[s['name']].get('file') else 'zero')
                            for s in layout],
                  layout_sha256=manifest['layout_sha256'])
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('%s sha256 %s' % (exe.relative_to(ROOT).as_posix(), digest))
    if not match:
        raise SystemExit('build_source_only: executable differs from retail (%s)' % RETAIL_SHA256)
    print('MATCHES RETAIL')


if __name__ == '__main__':
    main()
