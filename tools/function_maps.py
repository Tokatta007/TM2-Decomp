#!/usr/bin/env python3
"""Function and jump-table maps of a relocatable source-only link, for the PC port's recompiler.

    py -3.14 tools/function_maps.py work/<build>/source_reloc_e0 [more link dirs]

Reads a relocatable link directory (tools/build_source_only.py --relocatable: TM2_SOURCE.EXE,
reloc_plan.json, addrmap.tsv, symbols.tsv) and writes next to addrmap.tsv:

  functions.tsv          every function: build VA, retail VA, size in this build, retail slot size,
                         name, source, kind (game/sdk), form (c/cpp/asm/lib/retail), identical-to-retail
  jumptables.tsv         every switch jump table found in the build's code: build and retail address,
                         entry count, owning function, the jr site, case->target list (build VAs)
  functions_ida.tsv      the port's IDA-style function list (config/US/ida_funcs.tsv format:
                         va words name) at build addresses: input of the port's tools/recomp/inventory.py
  names_ida.tsv          names at build addresses (ida_names.tsv format: va code name) with one
                         jpt_<build jr site> name per jump table, as inventory.py looks them up
  recomp_inventory.json  the subset of the port's docs/recomp_inventory.json that its
                         tools/recomp/recompile.py reads (functions_table, jump_tables, unresolved_jr)

Everything is static: no emulator runs. Jump tables are found the way tools/pointer_census.py finds
them in retail (a lw from a lui/addiu table base indexed by addu, then jr through the loaded word);
a table extends while its words point into the owning function and another table does not start.
"""
import argparse
import csv
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, CONFIG, load_functions  # noqa: E402
from reloc_image import AddrMap, exe_image  # noqa: E402
from symbol_names import load_renames  # noqa: E402
from pointer_census import scan_code  # noqa: E402

ASM_SECTION = re.compile(r'"\s*\.section\s+\.text_([0-9A-Fa-f]{8})\b')


def h8(v):
    return '' if v is None else '%08X' % v


def asm_function_vas():
    """VAs of functions whose src/ code is SingleTrac/PsyQ hand-written asm (file-scope __asm__ .text_<VA>)."""
    out = {}
    for path in sorted((ROOT / 'src').rglob('*')):
        if path.suffix in ('.c', '.cpp', '.h') and path.is_file():
            for m in ASM_SECTION.finditer(path.read_text(encoding='utf-8', errors='replace')):
                out[int(m.group(1), 16)] = path.relative_to(ROOT).as_posix()
    return out


def sdk_function_vas():
    out = set()
    rows = [l for l in (CONFIG / 'sdk_modules.tsv').read_text(encoding='utf-8').splitlines() if l and not l.startswith('#')]
    for r in csv.DictReader(rows, delimiter='\t'):
        out.update(int(v, 16) for v in r['functions'].split())
    return out


def read_symbols(path):
    rows = []
    for line in Path(path).read_text(encoding='ascii').splitlines():
        if not line or line.startswith('#') or line.startswith('va\t'):
            continue
        va, code, name, size = line.split('\t')
        rows.append((int(va, 16), int(code), name, int(size)))
    return rows


def functions(link_dir, amap, plan, image_word):
    """Rows for every retail function, in build order."""
    retail_funcs = load_functions()
    build_dir = ROOT / plan['build_dir']
    sel_path = link_dir / 'all_c' / 'selection.json'
    if plan.get('selection'):  # port variant: the selection with the recompiled objects
        sel_path = ROOT / plan['selection']
    selection = json.loads((sel_path if sel_path.is_file() else build_dir / 'selection.json').read_text(encoding='utf-8'))
    renames = {r['va']: r['new'] for r in load_renames()}
    asm = asm_function_vas()
    sdk = sdk_function_vas()
    allc = {int(s['function'], 16): s for s in ((plan.get('all_c') or {}).get('sections') or [])}
    item_of = {}
    for it in plan['items']:
        if it['type'] == 'c' and it.get('code'):
            for va, end, name in it['functions']:
                item_of[int(va, 16)] = it
    rows = []
    for r0, r1, c0, c1, kind, owner in amap.rows:
        if not kind.startswith('code') or r1 <= r0 or r0 not in retail_funcs:
            continue  # aux padding, data, retail text gaps
        it = item_of.get(r0)
        sel = selection['functions'].get(h8(r0))
        ida_name = retail_funcs[r0][1]
        if it is not None and it.get('lib'):
            source, form = 'psyq ' + it['lib'], 'lib'
            name = next(n for v, e, n in it['functions'] if int(v, 16) == r0)
        elif r0 in allc:
            source, form = allc[r0]['reference'], 'c'
            name = sel['symbol'] if sel else allc[r0]['symbol']
        elif sel is not None:
            source = sel['source']
            form = 'asm' if r0 in asm else ('cpp' if source.endswith('.cpp') else 'c')
            name = sel['symbol']
        else:
            source, form, name = 'retail', 'retail', renames.get(r0, ida_name)
        is_sdk = r0 in sdk or form == 'lib' or Path(source).name.startswith('sdk_')
        rows.append(dict(build_va=c0, retail_va=r0, size=c1 - c0, retail_size=r1 - r0, name=name, ida_name=ida_name,
                         source=source, kind='sdk' if is_sdk else 'game', form=form,
                         identical=kind == 'code_linear'))
    rows.sort(key=lambda f: f['build_va'])
    missing = sorted(set(retail_funcs) - {f['retail_va'] for f in rows})
    if missing:
        raise SystemExit('function_maps: %d retail functions have no code row in %s (%s ...)' % (
            len(missing), link_dir / 'addrmap.tsv', ' '.join(h8(v) for v in missing[:5])))
    return rows


def port_functions(link_dir, plan):
    """Port variant: every function the link holds in .port_text (docs/SOURCE_BUILD.md "Port code"), global or
    static, from each input object's symbols at the offsets gen_link planned (asserted by the linker script).
    Rows as functions() with retail_va None, kind `port`. Returns (rows, {section: (address, bytes)})."""
    from semantic_elf import read_elf
    port = plan.get('port') or {}
    if not port.get('inputs'):
        return [], {}
    elf = read_elf(link_dir / 'TM2_SOURCE.elf')
    base = int(port['base'], 16)
    mem = {s['name']: (s['address'], s['data']) for s in elf['sections']
           if s['flags'] & 2 and s['type'] != 8 and s['address'] >= base and s['size']}
    rows = []
    for inp in port['inputs']:
        if inp['section'] != '.port_text' or not inp['size']:
            continue
        c0 = int(port['addresses'][inp['section']], 16) + inp['offset']
        obj = read_elf(ROOT / inp['obj'])
        idx = {s['index'] for s in obj['sections'] if s['name'] == inp['section']}
        funcs = sorted({(s['address'], s['name'], s['size'], s['binding']) for s in obj['symbols']
                        if s['section_index'] in idx and s['type'] == 2 and s['name']})
        if inp['size'] and (not funcs or funcs[0][0] != 0):
            raise SystemExit('function_maps: %s .port_text does not start with a function' % inp['unit'])
        for i, (off, name, size, binding) in enumerate(funcs):
            end = funcs[i + 1][0] if i + 1 < len(funcs) else inp['size']
            rows.append(dict(build_va=c0 + off, retail_va=None, size=end - off, retail_size=0, name=name, ida_name='',
                             source=inp['unit'], kind='port', form='cpp' if inp['unit'].endswith('.cpp') else 'c',
                             identical=False, static=binding == 0))
    return rows, mem


def bios_call(pc, lo, image_word):
    """jr $t2 with $t2 = 0xA0/0xB0/0xC0 set earlier in the function: a BIOS call stub, not a switch."""
    if (image_word(pc) >> 21) & 31 != 10:
        return False
    for a in range(pc - 4, lo - 4, -4):
        w = image_word(a)
        if w >> 26 in (0x09, 0x0D) and (w >> 16) & 31 == 10:  # addiu/ori $t2, $zero, imm
            return (w >> 21) & 31 == 0 and w & 0xFFFF in (0xA0, 0xB0, 0xC0)
    return False


def jump_tables(funcs, amap, image_word, in_build, c2r=None):
    """(tables, unresolved jr sites) of the build's code. `c2r`: retail address of a build address (default
    amap.c2r; the port variant's gives None inside the port sections)."""
    c2r = c2r or amap.c2r
    found = {}       # table -> (function row, jr site)
    jr_sites = []    # (site, function row, reg)
    consts = set()   # every code constant built with lui + addiu/ori (any function: tables built at run time)
    for f in funcs:
        lo, hi = f['build_va'], f['build_va'] + f['size']
        refs, jts = scan_code(lo, hi, image_word)
        for t, jr in jts:
            if in_build(t):
                found.setdefault(t, (f, jr))
        consts.update(r['value'] for r in refs if r['kind'] == 'hi16_lo16')
        for pc in range(lo, hi, 4):
            w = image_word(pc)
            if w >> 26 == 0 and w & 63 == 8 and (w >> 21) & 31 != 31 and not bios_call(pc, lo, image_word):
                jr_sites.append((pc, f, (w >> 21) & 31))
    tables = []
    for t, (f, jr) in sorted(found.items()):
        lo, hi = f['build_va'], f['build_va'] + f['size']
        targets, a = [], t
        while in_build(a) and lo <= image_word(a) < hi and (a == t or a not in found):
            targets.append(image_word(a))
            a += 4
        if targets:
            tables.append(dict(table=t, retail_table=c2r(t), entries=len(targets), function=f, site=jr,
                               retail_site=c2r(jr), targets=targets))
    with_table = {t['site'] for t in tables}
    unresolved = []
    for site, f, reg in jr_sites:
        if site in with_table:
            continue
        lo, hi = f['build_va'], f['build_va'] + f['size']
        unresolved.append(dict(site=site, retail_site=c2r(site), function=f, reg=reg,
                               runtime_table_targets=sorted(v for v in consts if lo < v < hi and v % 4 == 0)))
    return tables, unresolved


def write_maps(link_dir):
    """Write the five files into link_dir; return a summary dict."""
    link_dir = Path(link_dir)
    amap = AddrMap.read(link_dir / 'addrmap.tsv')
    plan = json.loads((link_dir / 'reloc_plan.json').read_text(encoding='utf-8'))
    exe = exe_image(link_dir / 'TM2_SOURCE.EXE')
    t_addr, img = exe['t_addr'], exe['image']

    pfuncs, pmem = port_functions(link_dir, plan)  # port variant: the port's own code (.port_text)

    def port_at(a):
        for lo, data in pmem.values():
            if lo <= a and a + 4 <= lo + len(data):
                return lo, data
        return None

    def in_build(a):
        return t_addr <= a and a + 4 <= t_addr + len(img) or port_at(a) is not None

    def word(a):
        if t_addr <= a and a + 4 <= t_addr + len(img):
            return struct.unpack_from('<I', img, a - t_addr)[0]
        lo, data = port_at(a)
        return struct.unpack_from('<I', data, a - lo)[0]

    def c2r(a):  # the port sections have no retail address (amap.c2r keeps out-of-image addresses as they are)
        return None if port_at(a) is not None else amap.c2r(a)

    funcs = functions(link_dir, amap, plan, word) + pfuncs
    tables, unresolved = jump_tables(funcs, amap, word, in_build, c2r)

    where = link_dir.relative_to(ROOT).as_posix() if link_dir.is_relative_to(ROOT) else link_dir.as_posix()
    head = ['# Generated by tools/function_maps.py from %s (see docs/SOURCE_BUILD.md, "For the port").' % where]
    lines = head + ['# build_va/size: this image; retail_va/retail_size: the retail slot (addrmap.tsv). kind: game|sdk; '
                    'form: c|cpp|asm (hand-written asm in src/)|lib (PsyQ .LIB module)|retail (retail bytes); '
                    'identical: 1 when the un-relocated bytes equal retail over the whole slot.']
    if pfuncs:
        lines.append('# kind port: the port\'s own code in .port_text (port_text.bin; docs/SOURCE_BUILD.md "Port code"), '
                     'outside the EXE, no retail slot (retail_va empty, retail_size 0).')
    lines.append('build_va\tretail_va\tsize\tretail_size\tname\tsource\tkind\tform\tidentical\tida_name')
    lines += ['%s\t%s\t%d\t%d\t%s\t%s\t%s\t%s\t%d\t%s' % (h8(f['build_va']), h8(f['retail_va']), f['size'], f['retail_size'],
                                                         f['name'], f['source'], f['kind'], f['form'], f['identical'],
                                                         f['ida_name']) for f in funcs]
    (link_dir / 'functions.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')

    lines = head + ['# table/site: this image; retail_*: c2r of addrmap.tsv (empty: no retail address, e.g. inside a '
                    'function whose code differs). targets: case index:build target VA.',
                    'table\tretail_table\tentries\tfunction_va\tfunction_retail_va\tfunction\tsite\tretail_site\ttargets']
    lines += ['%s\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s' % (h8(t['table']), h8(t['retail_table']), t['entries'],
                                                     h8(t['function']['build_va']), h8(t['function']['retail_va']),
                                                     t['function']['name'], h8(t['site']), h8(t['retail_site']),
                                                     ' '.join('%d:%08X' % kv for kv in enumerate(t['targets'])))
              for t in tables]
    (link_dir / 'jumptables.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')

    # The port's tools/recomp formats.
    lines = ['va\twords\tname'] + ['%s\t%d\t%s' % (h8(f['build_va']), f['size'] // 4, f['name']) for f in funcs]
    (link_dir / 'functions_ida.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')
    names = [(va, code, name) for va, code, name, size in read_symbols(link_dir / 'symbols.tsv')
             if not name.startswith('jpt_')]
    names += [(t['table'], 0, 'jpt_%08X' % t['site']) for t in tables]
    lines = ['va\tcode\tname'] + ['%s\t%d\t%s' % (h8(a), c, n) for a, c, n in sorted(names)]
    (link_dir / 'names_ida.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')
    inv = dict(exe=where + '/TM2_SOURCE.EXE',
               generated_by='TM2-Decomp tools/function_maps.py (subset of the port\'s recomp_inventory.json)',
               functions_table=[dict(addr=h8(f['build_va']), name=f['name'], size=f['size'], retail_addr=h8(f['retail_va']),
                                     kind=f['kind'], form=f['form'], identical=f['identical']) for f in funcs],
               jump_tables=[dict(site=h8(t['site']), function=h8(t['function']['build_va']), table=h8(t['table']),
                                 entries=t['entries'], distinct_targets=len(set(t['targets'])), found_by='lw+jr')
                            for t in tables],
               unresolved_jr=[dict(site=h8(u['site']), function=h8(u['function']['build_va']), reg=u['reg'],
                                   candidate_tables=[], runtime_table_targets=[h8(v) for v in u['runtime_table_targets']])
                              for u in unresolved])
    (link_dir / 'recomp_inventory.json').write_text(json.dumps(inv, indent=1) + '\n', encoding='utf-8')
    return dict(functions=len(funcs), identical=sum(f['identical'] for f in funcs),
                by_form={k: sum(1 for f in funcs if f['form'] == k) for k in sorted({f['form'] for f in funcs})},
                sdk=sum(1 for f in funcs if f['kind'] == 'sdk'), jump_tables=len(tables),
                jump_table_entries=sum(t['entries'] for t in tables),
                unresolved_jr=[dict(site=h8(u['site']), function=u['function']['name'],
                                    runtime_table_targets=len(u['runtime_table_targets'])) for u in unresolved],
                **({} if not pfuncs else dict(
                    port_functions=[dict(name=f['name'], address=h8(f['build_va']), size=f['size'], unit=f['source'],
                                         static=f['static']) for f in pfuncs],
                    port_jump_tables=[dict(table=h8(t['table']), entries=t['entries'], function=t['function']['name'],
                                           site=h8(t['site'])) for t in tables if t['function']['kind'] == 'port'])))


def read_functions_tsv(path):
    rows = []
    for line in Path(path).read_text(encoding='ascii').splitlines():
        if not line or line.startswith('#') or line.startswith('build_va	'):
            continue
        p = line.split('	')
        rows.append(dict(build_va=int(p[0], 16), retail_va=int(p[1], 16) if p[1] else None, size=int(p[2]),
                         retail_size=int(p[3]), name=p[4],
                         source=p[5], kind=p[6], form=p[7], identical=p[8] == '1', ida_name=p[9]))
    return rows


PLACE_GAP = 16  # differing runs closer than this (4 instructions) count as one place
DIFF_KIND = {'c': 'close C', 'cpp': 'close C', 'asm': 'asm', 'lib': 'lib', 'retail': 'retail'}


def retail_differences(link_dir, verify_path):
    """Every byte range where the image differs from retail, attributed to a function or data owner.

    link_dir is an identity-map (E0) relocatable link with functions.tsv; verify_path is the
    tools/verify_image.py result of the fixed-layout image that has the same bytes (its vs_retail runs and
    classes are reused). Returns (owners, totals)."""
    link_dir = Path(link_dir)
    amap = AddrMap.read(link_dir / 'addrmap.tsv')
    if not amap.is_identity():
        raise SystemExit('function_maps: retail differences need an identity (E0) map: %s' % link_dir)
    t_addr = exe_image(link_dir / 'TM2_SOURCE.EXE')['t_addr']
    funcs = read_functions_tsv(link_dir / 'functions.tsv')
    fstarts = sorted((f['retail_va'], f) for f in funcs)
    import bisect
    keys = [k for k, _ in fstarts]
    data_rows = sorted((r for r in amap.rows if r[1] > r[0] and not r[4].startswith('code')), key=lambda r: r[0])
    dkeys = [r[0] for r in data_rows]
    checked = json.loads(Path(verify_path).read_text(encoding='utf-8'))
    owners = {}
    for run in checked['vs_retail']['detail']:
        lo, hi = int(run['start'], 16), int(run['end'], 16)
        if lo < 0x800:
            key = ('header', 0)
            owner = dict(va=None, name='PS-X EXE header', source='', kind='header')
        else:
            va = t_addr + lo - 0x800
            i = bisect.bisect_right(keys, va) - 1
            f = fstarts[i][1] if i >= 0 and va < fstarts[i][0] + fstarts[i][1]['retail_size'] else None
            if f is not None:
                key = ('f', f['retail_va'])
                owner = dict(va=h8(f['retail_va']), end=h8(f['retail_va'] + f['retail_size']), name=f['name'],
                             ida_name=f['ida_name'], source=f['source'], kind=DIFF_KIND[f['form']])
            else:
                j = bisect.bisect_right(dkeys, va) - 1
                r = data_rows[j]
                key = ('d', r[0])
                owner = dict(va=h8(r[0]), end=h8(r[1]), name=r[5], source=r[5].split(' ')[0], kind='data')
        o = owners.setdefault(key, dict(owner, verify_class=run['cls'], bytes=0, runs=0, places=[]))
        va0, va1 = t_addr + lo - 0x800, t_addr + hi - 0x800
        o['bytes'] += hi - lo
        o['runs'] += 1
        if o['places'] and va0 - int(o['places'][-1]['end'], 16) < PLACE_GAP:
            pl = o['places'][-1]
            pl['end'] = h8(va1)
            pl['bytes'] += hi - lo
            pl['runs'] += 1
        else:
            o['places'].append(dict(start=h8(va0), end=h8(va1), bytes=hi - lo, runs=1))
    rows = [owners[k] for k in sorted(owners, key=lambda k: (k[0] != 'header', k[1]))]
    totals = dict(bytes=sum(o['bytes'] for o in rows), runs=sum(o['runs'] for o in rows),
                  places=sum(len(o['places']) for o in rows), owners=len(rows),
                  by_kind={k: sum(o['bytes'] for o in rows if o['kind'] == k) for k in sorted({o['kind'] for o in rows})},
                  place_gap=PLACE_GAP)
    return rows, totals


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('link_dirs', nargs='+', type=Path)
    a = ap.parse_args()
    for d in a.link_dirs:
        d = d if d.is_absolute() else ROOT / d
        print(d.relative_to(ROOT).as_posix() if d.is_relative_to(ROOT) else d, json.dumps(write_maps(d)))


if __name__ == '__main__':
    main()
