#!/usr/bin/env python3
"""Plan and generate the M2 ("nonmatching") source-only link.

Every compiled unit object of a hybrid build (work/build-*/selection.json, as
written by build.py) is linked at its retail address; retail bytes from
split/bin/ (tools/split_retail.py) fill everything the objects do not own.

Placement of an object's allocated sections follows the hybrid
(tools/build_maximal_hybrid.py):
  1. selected function identity: the section holding a selection function's
     symbol starts at VA - symbol offset;
  2. a global symbol whose name resolves through symbol_map() (ida_names.tsv,
     undefined_syms_*.txt, or a D_/sub_/func_<hex> name) fixes its section;
  3. `.tm2_<addr>` (also `.tm2_string_`/`.tm2_init_`) sections sit at <addr>.
All rules must agree; every allocated section except .MIPS.abiflags must be
placed. Sections at/above the BSS start (80180EE4) become NOLOAD output
sections and their contents must be zero.

A code section owns the retail slots of the functions it holds (slots come
from ida_funcs + extra_funcs - not_funcs). The section must lie inside the
union of those slots; when it is shorter, the rest of the slot (the "tail") is
left unwritten, so tools/elf_to_psx_exe.py fills it with zero. Data sections
own exactly their own bytes. Retail segments are cut into `.incbin` / `.space`
pieces around these holes.

symbols.ld resolves every import of every object:
  - defined by some object        -> nothing assigned, ASSERT(name == address)
  - alias of an object symbol     -> name = sym;            (same address)
  - inside a placed section       -> name = sym + 0xOFF;     (interior reference)
  - otherwise                     -> name = 0xADDRESS;       (absolute, retail)
plus `_gp` and the entry symbol. A name both defined by an object and assigned
in the script is an error.

Port variant (relocatable only, plan(port=...); docs/SOURCE_BUILD.md "Port variant"): the sections
.port_data / .port_bss of src/semantic/port_variant_arrays.c are not placed at a retail address but at the
port base (default 0x80200000); every import whose retail address lies inside the retail storage of an array
of config/US/port_variant_arrays.tsv resolves to the new storage (name = array + offset). The retail storage
itself becomes plain retail bytes (no unit defines it any more). Port code (docs/SOURCE_BUILD.md "Port code"):
any object of the variant may also contribute .port_text (functions) and .port_rodata (their constants and jump
tables); the four port sections are laid out in the order of PORT_SECTIONS from the port base, each object's part
at an offset fixed by port_plan (and asserted by the linker script).
"""
import bisect
import csv
import fnmatch
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, CONFIG, read_layout, load_functions
from semantic_elf import read_elf, read_relocs
from build_maximal_hybrid import symbol_map, address_for, SECTION_HEX
from symbol_names import load_renames

SHF_ALLOC, SHF_EXECINSTR = 2, 4
SHT_NOBITS = 8
GP = 0x801809D4
BSS_START = 0x80180EE4
IGNORED_ALLOC = ('.MIPS.abiflags',)
NOBITS_KINDS = ('bss_in_file', 'bss')
# Port variant sections, in link order at the port base (docs/SOURCE_BUILD.md "Port variant", "Port code"):
# storage first (.port_data at the base, as before port code existed), then the port's code and constants.
PORT_SECTIONS = ('.port_data', '.port_bss', '.port_text', '.port_rodata')
PORT_DATA_SECTIONS = ('.port_data', '.port_bss')  # the arrays of config/US/port_variant_arrays.tsv
PORT_ALIGN = 16  # every port output section after the first starts 16-aligned
PORT_BASE = 0x80200000
# The port sections must end below the port RAM end: the port's emulated RAM is 16 MB (to 0x81000000); the decomp's
# port memory may use 0x80200000..0x80BFFFFF (12 MB) and the port's own high block (in-between frames' OTs and
# packets, the subpixel ring, ...; formerly 0x80600000..0x807FFFFF, which capped the port sections at 0x80600000)
# moves to 0x80C00000..0x80FFFFFF. The PORT chooses the end: everything the decomp places at the port base must
# stay below it, and the port's own high RAM must start at or above it. build_source_only.py / build.py
# --port-ram-end ADDR set it (recorded in report.json, out/TM2_SOURCE_PORT.json and LAST_PORT_BUILD.json as
# port_ram_end); the default is 0x80C00000 (the port's answer to request 33, 2026-10-13).
DEFAULT_PORT_RAM_END = 0x80C00000
PORT_RAM_END = DEFAULT_PORT_RAM_END  # (old name: the default)
PORT_RAM_TOP = 0x81000000            # the port's 16 MB


class LinkError(SystemExit):
    pass


def fail(msg):
    raise LinkError('gen_link: ' + msg)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def last_build_dir():
    exe = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))['executable']
    return (ROOT / exe).parent.parent


# The limits of include/semantic/players.h as retail has them (the port variant reads its own back from the
# preprocessor: build_source_only.port_limits). Keys are the groups of config/US/port_variant_arrays.tsv.
RETAIL_LIMITS = dict(humans=2, views=2, huds=2, viewmodes=9, ai=9, slots=15, carsloaded=11, radarblips=12,
                     projectiles=16, coordsys=128, soundinsts=35, sndreqs=35, arcs=5, shields=4, remotebombs=2,
                     ricochets=10,
                     flames=2, slamrecords=4, worldmodels=20, clutdefs=20, clutanims=10,
                     dmdarea=0x200800,  # dmdarea: bytes of the car DMD area (no retail storage; its default)
                     spuvoices=24,  # SPU voices (spu_voices.h TM2_SPU_VOICES; 48 with TM2_PORT_SPU48, request 34)
                     drops=25, mines=23,  # drop records / mine objects (raised by TM2_PORT_BALANCE, request 40)
                     fxsprites=150,  # effect sprites (raised by TM2_PORT_CARS, request 102)
                     renderslots=200)  # matrix slots per view (render_slots.h; TM2_PORT_RENDER_SLOTS, request 47)


def port_entry_bytes(expr, limits):
    """entry_bytes of a tsv row: a number, or an expression of the limits (e.g. 0x70+0x40*radarblips)."""
    if re.fullmatch(r'\s*(0[xX][0-9A-Fa-f]+|\d+)\s*', expr):
        return int(expr, 0)
    if not re.fullmatch(r'[\w\s+*()-]+', expr):
        fail('port_variant_arrays.tsv: bad entry_bytes expression %r' % expr)
    try:
        return int(eval(expr, {'__builtins__': {}}, dict(limits)))
    except Exception as e:  # unknown limit name, ...
        fail('port_variant_arrays.tsv: entry_bytes %r: %s' % (expr, e))


def read_port_arrays(path=None):
    """config/US/port_variant_arrays.tsv rows (ints parsed; retail_va None for port-only storage). entry_bytes is
    the retail entry size (the expression at RETAIL_LIMITS); entry_expr keeps the tsv text for other limits."""
    rows = []
    for r in read_tsv(path or CONFIG / 'port_variant_arrays.tsv'):
        rows.append(dict(name=r['name'], retail_va=None if r['retail_va'] == '-' else int(r['retail_va'], 16),
                         entry_bytes=port_entry_bytes(r['entry_bytes'], RETAIL_LIMITS), entry_expr=r['entry_bytes'],
                         retail_entries=int(r['retail_entries'], 0),
                         group=r['group'], section=r['section'], layout=r['layout'], retail_section=r['retail_section'],
                         note=r['note']))
    return rows


def port_entries(arr, limits):
    """Entries of a port array for the limits (RETAIL_LIMITS keys: humans, views, huds, viewmodes, ai, slots,
    radarblips, projectiles, coordsys, soundinsts, sndreqs, carsloaded); `slotpairs` is slots squared (the [slot][slot] pair
    tables); `groups` is carsloaded + 1 (load groups 0..carsloaded);
    `humans-2` / `ai-9` are what exceeds retail's human_car[2] / opponent slots 0..8; `NAME+` is the limit NAME
    when it is above retail's, else 0 (the array is not moved: retail's storage stays in use); `NAME>K` is the limit
    NAME when it is above K, else 0 (retail's storage has room for K, e.g. carResetWeapons' 4 bytes: humans>4);
    group `one` (a fixed-size
    block, e.g. the PC port's settings) is one entry whatever the limits; `one:NAME+` is such a block only while
    the limit NAME is raised (0 entries, not defined, otherwise)."""
    g = arr['group']
    if g.startswith('one:'):
        return 1 if port_entries(dict(arr, group=g[4:]), limits) > 0 else 0
    if g == 'humans-2':
        return limits['humans'] - 2
    if g == 'ai-9':
        return limits['ai'] - 9
    if g == 'slotpairs':
        return limits['slots'] ** 2
    if g == 'groups':  # load groups 0 (the level) .. carsloaded
        return limits['carsloaded'] + 1
    if g == 'one':
        return 1
    m = re.fullmatch(r'(\w+)>(\d+)', g)
    if m:  # moved only past K entries: retail's storage (not moved) while it has room (carResetWeapons: humans>4)
        if m.group(1) not in limits:
            fail('port_variant_arrays.tsv: %s: unknown group %s' % (arr['name'], g))
        return limits[m.group(1)] if limits[m.group(1)] > int(m.group(2)) else 0
    if g.endswith('+'):  # moved only when raised: retail's storage (not moved) while the limit is retail's
        g = g[:-1]
        if g not in limits:
            fail('port_variant_arrays.tsv: %s: unknown group %s+' % (arr['name'], g))
        return limits[g] if limits[g] > RETAIL_LIMITS[g] else 0
    if g not in limits:
        fail('port_variant_arrays.tsv: %s: unknown group %s' % (arr['name'], g))
    return limits[g]


def port_plan(port, port_rows, port_defined, defined, syms):
    """Check the port object against config/US/port_variant_arrays.tsv; returns (plan info, moved arrays)."""
    base, limits = port['base'], port['limits']
    ram_end = port.get('ram_end') or DEFAULT_PORT_RAM_END
    if ram_end % 16 or not BSS_START < ram_end <= PORT_RAM_TOP:
        fail('port RAM end %08X must be 16-aligned, above the retail image and at most %08X' % (ram_end, PORT_RAM_TOP))
    if base % 16 or not BSS_START < base or base >= ram_end:
        fail('port base %08X must be 16-aligned and inside the port RAM (above the retail image, below the port RAM '
             'end %08X)' % (base, ram_end))
    # Several objects may contribute to each port section: the variant's own unit (port_variant_arrays.c)
    # first, then the build's units (recompiled by the variant) in build order, then the port's units
    # (--port-unit) in command-line order. Each input starts at its own alignment; each output section after
    # the first at PORT_ALIGN. Every address is fixed here and asserted by the linker script.
    rank = {'port_variant': 0, 'port_unit': 2}
    port_rows.sort(key=lambda r: (PORT_SECTIONS.index(r['section']), rank.get(r.get('kind'), 1), r['unit_index']))
    addresses, sizes_out, inputs, at = {}, {}, [], base
    for i, sec in enumerate(PORT_SECTIONS):
        if i:
            at = (at + PORT_ALIGN - 1) // PORT_ALIGN * PORT_ALIGN
        addresses[sec], cur = at, 0
        for row in (r for r in port_rows if r['section'] == sec):
            align = max(row['align'], 1)
            if align > PORT_ALIGN:
                fail('%s: %s is %d-aligned (port sections allow up to %d)' % (row['unit'], sec, align, PORT_ALIGN))
            cur = (cur + align - 1) // align * align
            row['offset'] = cur
            inputs.append(dict(unit=row['unit'], obj=row['obj'], section=sec, offset=cur, size=row['size'],
                               align=align))
            cur += row['size']
        sizes_out[sec] = cur
        at += cur
    # symbol sizes: up to the next symbol (or the section end); gcc 2.7 emits no .size
    sizes = {}
    for row in port_rows:
        offs = sorted((o, n) for n, o in row['globals'].items())
        for i, (o, n) in enumerate(offs):
            sizes[n] = (offs[i + 1][0] if i + 1 < len(offs) else row['size']) - o
    arrays, moved = [], []
    want = set()
    for a in port['arrays']:
        n = port_entries(a, limits)
        if n <= 0:
            continue  # e.g. sessionPortHumanCars with two humans
        want.add(a['name'])
        d = port_defined.get(a['name'])
        if d is None:
            fail('port variant: %s (config/US/port_variant_arrays.tsv) is not defined in .port_data/.port_bss' % a['name'])
        sec = '.port_data' if a['section'] == 'data' else '.port_bss'
        if d['section']['section'] != sec:
            fail('port variant: %s is in %s, the table says %s' % (a['name'], d['section']['section'], sec))
        eb = port_entry_bytes(a.get('entry_expr', str(a['entry_bytes'])), limits)
        size = eb * n
        if sizes[a['name']] < size or sizes[a['name']] - size >= 4 + max(d['section']['align'], 4):
            fail('port variant: %s is 0x%X bytes, expected 0x%X (%d x 0x%X)' % (a['name'], sizes[a['name']], size, n,
                                                                              eb))
        if a['retail_va'] is not None:
            if address_for(a['name'], syms) not in (None, a['retail_va']):
                fail('port variant: %s is at %08X in the retail names, the table says %08X' % (
                    a['name'], address_for(a['name'], syms), a['retail_va']))
            if eb != a['entry_bytes']:
                layout = 'relayout'  # the entry itself changed (e.g. hudRadars with more blips): only the start maps
            else:
                layout = a['layout'] if n != a['retail_entries'] else 'linear'
            moved.append(dict(name=a['name'], retail_va=a['retail_va'], layout=layout,
                              retail_end=a['retail_va'] + a['entry_bytes'] * a['retail_entries']))
        else:
            layout = 'new'
        arrays.append(dict(name=a['name'], section=sec, offset=d['section']['offset'] + d['offset'], size=size, entries=n,
                           entry_bytes=eb, group=a['group'], layout=layout,
                           retail_va=None if a['retail_va'] is None else '%08X' % a['retail_va'],
                           retail_size=a['entry_bytes'] * a['retail_entries'], retail_section=a['retail_section'],
                           note=a['note']))
    extra = sorted(n for n, d in port_defined.items() if d['section']['section'] in PORT_DATA_SECTIONS and n not in want)
    if extra:
        fail('port variant: .port_data/.port_bss define %s, which config/US/port_variant_arrays.tsv does not list' % ', '.join(extra))
    # retail code reaches the four radar-arrow sprite arrays as one [4][2] block (hudRadarArrowsLoad): with
    # two HUDs they must stay back to back, in retail order
    if limits['huds'] == 2:
        seq = ['hudRadarArrow%sSprites' % k for k in ('Up', 'Down', 'Left', 'Right')]
        offs = [port_defined[n]['section']['offset'] + port_defined[n]['offset'] for n in seq if n in port_defined]
        if len(offs) == 4 and any(b - a != 16 for a, b in zip(offs, offs[1:])):
            fail('port variant: the radar-arrow sprite arrays must be back to back (Up, Down, Left, Right)')
    for m in moved:
        for n, d in defined.items():
            if m['retail_va'] <= d['address'] < m['retail_end']:
                fail('%s still defines %s inside the retail storage of moved array %s' % (d['unit'], n, m['name']))
    info = dict(base=base, ram_end=ram_end, limits=dict(limits),
                sections=[dict(name=s, address='%08X' % addresses[s], size=sizes_out[s],
                               align=max([r['align'] for r in port_rows if r['section'] == s] or [1]),
                               nobits=s == '.port_bss') for s in PORT_SECTIONS if sizes_out[s]],
                addresses=addresses, sizes=sizes_out, inputs=inputs, arrays=arrays, redirects=[])
    return info, moved


def plan(build_dir, sdk_from_lib=False, work_dir=None, selection=None, all_c=None, port=None, sdk_keep_c=None):
    """Return the link plan for the objects of `build_dir`. No files are written, except with sdk_from_lib:
    the converted PsyQ modules (work/sdk_lib/) and the filtered unit objects (work_dir/sdk_units/).

    `selection` replaces build_dir/selection.json (the all-C flavour swaps unit objects). `all_c` maps a unit
    source to the names of its code sections that are replaced C (pure-C versions of hand-written asm,
    docs/SOURCE_BUILD.md "All-C flavour"): such a section may be larger or smaller than its retail slots
    (only a relocatable link can place it), gets no zero-filled tail, and imports of addresses inside it
    other than its entry (labels of the retail asm) resolve to its entry symbol. A data section named in
    `all_c` (a jump table of a replaced function, port variant) is placed as usual and marked `excluded`.

    `port` (port variant): dict(base=address, limits={'humans','views','huds'}, arrays=read_port_arrays()).
    The port sections (PORT_SECTIONS) are collected in plan['port'] instead of being placed, imports inside
    a moved array's retail storage are redirected to the new storage, and a unit that still defines retail
    storage of a moved array is an error.

    `sdk_keep_c` (with sdk_from_lib): {(lib, module), ...} of config/US/sdk_c_modules.tsv whose functions stay the
    C units' (sdk_c_override): those modules are not linked from the .LIB (port variant, e.g. TM2_PORT_SPU48)."""
    build_dir = Path(build_dir).resolve()
    if selection is None:
        selection = json.loads((build_dir / 'selection.json').read_text(encoding='utf-8'))
    all_c = all_c or {}
    syms = symbol_map()
    slots = load_functions()  # va -> (end, name)
    warnings, problems = [], []

    by_source = {}
    for va, f in selection['functions'].items():
        if f['kind'] != 'maintained':
            fail('selection function %s is %s, not maintained' % (va, f['kind']))
        by_source.setdefault(f['source'], []).append((int(va, 16), f['symbol']))
    missing_slots = sorted(va for fs in by_source.values() for va, _ in fs if va not in slots)
    if missing_slots:
        fail('selection functions without a slot: ' + ' '.join('%08X' % v for v in missing_slots))
    unselected = sorted(set(slots) - {va for fs in by_source.values() for va, _ in fs})

    sections, defined, imports, discards = [], {}, {}, []
    port_rows, port_defined = [], {}
    for index, (source, unit) in enumerate(selection['units'].items()):
        obj = ROOT / unit['object']
        if sha(obj) != unit['object_sha256']:
            fail('stale object %s (sha differs from selection.json)' % unit['object'])
        if (ROOT / source).is_file() and sha(ROOT / source) != unit['source_sha256']:
            warnings.append('source changed since the hybrid build (object reused as-is): ' + source)
        elf = read_elf(obj)
        secs = elf['sections']
        bases, why = {}, {}

        def setbase(sec_name, base, reason):
            old = bases.setdefault(sec_name, base)
            why.setdefault(sec_name, reason)
            if old != base:
                fail('%s: %s placed at %08X by %s but %08X by %s' % (source, sec_name, old, why[sec_name], base, reason))

        globals_ = [s for s in elf['symbols'] if s['name'] and s['binding'] in (1, 2)]
        port_idx = {sec['index'] for sec in secs if sec['name'] in PORT_SECTIONS and sec['size']}
        if port_idx and port is None:
            fail('%s has port sections (%s): link it only as the port variant' % (
                source, ', '.join(sorted({secs[i]['name'] for i in port_idx}))))
        for va, name in by_source.get(source, []):
            m = [s for s in globals_ if s['name'] == name and 0 < s['section_index'] < 0xff00]
            if len(m) != 1:
                fail('%s: selection symbol %s not defined exactly once' % (source, name))
            setbase(secs[m[0]['section_index']]['name'], va - m[0]['address'], 'function %08X' % va)
        for s in globals_:
            if 0 < s['section_index'] < 0xff00 and s['section_index'] not in port_idx:
                addr = address_for(s['name'], syms)
                if addr is not None:
                    setbase(secs[s['section_index']]['name'], addr - s['address'], 'symbol ' + s['name'])
        for sec in secs:
            if sec['flags'] & SHF_ALLOC and sec['size']:
                m = SECTION_HEX.match(sec['name'])
                if m:
                    setbase(sec['name'], int(m.group(1), 16), 'section name')
        unit_secs = {}
        for sec in secs:
            if sec['type'] not in (1, SHT_NOBITS) or sec['name'] in IGNORED_ALLOC:
                continue
            has_syms = any(s['section_index'] == sec['index'] for s in globals_)
            if not (sec['flags'] & SHF_ALLOC) or (not sec['size'] and not has_syms):
                if any(fnmatch.fnmatchcase(sec['name'], d) for d in DISCARD):
                    continue
                if sec['size'] or has_syms:
                    fail('%s: unexpected non-allocated section %s (%d bytes)' % (source, sec['name'], sec['size']))
                discards.append((unit['object'], sec['name']))  # empty placeholder section, e.g. a declared-only slot
                continue
            if sec['index'] in port_idx:
                if sec['name'] in unit_secs:
                    fail('%s: duplicate section name %s' % (source, sec['name']))
                data = b'\0' * sec['size'] if sec['type'] == SHT_NOBITS else sec['data']
                row = dict(unit=source, unit_index=index, obj=unit['object'], section=sec['name'], base=None,
                           size=sec['size'], align=sec['alignment'], code=sec['name'] == '.port_text',
                           nobits=sec['name'] == '.port_bss', kind=unit.get('kind'),
                           placed_by='port base', functions=[], globals={}, port=True,
                           data_sha256=hashlib.sha256(data).hexdigest(), _data=data)
                if row['code'] != bool(sec['flags'] & SHF_EXECINSTR):
                    fail('%s: %s is %sexecutable' % (source, sec['name'], '' if sec['flags'] & SHF_EXECINSTR else 'not '))
                if row['nobits'] and any(data):
                    fail('%s: .port_bss has non-zero contents' % source)
                unit_secs[sec['name']] = row
                port_rows.append(row)
                continue
            if sec['name'] not in bases:
                fail('%s: unplaceable allocated section %s (%d bytes)' % (source, sec['name'], sec['size']))
            if sec['name'] in unit_secs:
                fail('%s: duplicate section name %s' % (source, sec['name']))
            base = bases[sec['name']]
            nobits_in = sec['type'] == SHT_NOBITS
            data = b'\0' * sec['size'] if nobits_in else sec['data']
            row = dict(unit=source, unit_index=index, obj=unit['object'], section=sec['name'], base=base,
                       size=sec['size'], align=sec['alignment'], code=bool(sec['flags'] & SHF_EXECINSTR),
                       nobits=base >= BSS_START, placed_by=why[sec['name']], functions=[], globals={},
                       data_sha256=hashlib.sha256(data).hexdigest())
            if base < BSS_START < base + sec['size']:
                fail('%s: %s straddles the BSS start' % (source, sec['name']))
            if row['nobits'] and any(data):
                fail('%s: %s lies in BSS (%08X) but has non-zero contents' % (source, sec['name'], base))
            if base % max(sec['alignment'], 1):
                fail('%s: %s at %08X violates its alignment %d' % (source, sec['name'], base, sec['alignment']))
            row['_data'] = data
            unit_secs[sec['name']] = row
            sections.append(row)
        for s in globals_:
            if s['section_index'] == 0:
                imports.setdefault(s['name'], []).append(source)
            elif s['section_index'] < 0xff00:
                row = unit_secs.get(secs[s['section_index']]['name'])
                if row is None:
                    fail('%s: global %s in an unplaced section' % (source, s['name']))
                if row.get('port'):
                    if s['name'] in defined or s['name'] in port_defined:
                        fail('port symbol %s defined twice (%s)' % (s['name'], source))
                    port_defined[s['name']] = dict(unit=source, section=row, offset=s['address'])
                    row['globals'][s['name']] = s['address']
                    continue
                if s['name'] in defined or s['name'] in port_defined:
                    fail('symbol %s defined by both %s and %s' % (
                        s['name'], (defined.get(s['name']) or port_defined[s['name']])['unit'], source))
                addr = row['base'] + s['address']
                defined[s['name']] = dict(unit=source, address=addr, section=row)
                row['globals'][s['name']] = addr
            else:
                fail('%s: unsupported special symbol %s (shndx %04X)' % (source, s['name'], s['section_index']))
        for va, name in by_source.get(source, []):
            row = defined[name]['section']
            if defined[name]['address'] != va:
                fail('%s: %s lands at %08X, not %08X' % (source, name, defined[name]['address'], va))
            row['functions'].append(va)

    functions = {int(va, 16): f['symbol'] for va, f in selection['functions'].items()}
    link_objects = sorted({u['object'] for u in selection['units'].values()})
    sdk = None
    if sdk_from_lib:
        sdk = apply_sdk_from_lib(sections, defined, imports, discards, syms, slots, work_dir, warnings, problems,
                                 keep_c=sdk_keep_c)
        for va in sdk['dropped_functions']:
            functions.pop(int(va, 16))
        link_objects = sorted({sdk['object_map'].get(o, o) for o in link_objects} | set(sdk['lib_objects']))

    # Holes: code sections own their functions' whole slots, data sections their own bytes.
    holes, tails = [], []
    for row in sections:
        start, end = row['base'], row['base'] + row['size']
        if row['code'] and row.get('lib'):
            lo, hi = row['slot']  # a PsyQ module: its slots plus the unlisted gaps inside it (crt0's stack table)
            if end < hi:
                fs = sorted(row['functions'])
                tails.append(dict(function='%08X' % fs[-1], symbol=slots[fs[-1]][1], unit=row['unit'],
                                  section=row['section'], start='%08X' % end, end='%08X' % hi, bytes=hi - end))
            holes.append((lo, hi, row))
        elif row['code']:
            if not row['functions']:
                fail('%s: code section %s holds no selection function' % (row['unit'], row['section']))
            fs = sorted(row['functions'])
            spans = [(va, slots[va][0]) for va in fs]
            for (a0, a1), (b0, b1) in zip(spans, spans[1:]):
                if a1 != b0:
                    fail('%s: %s functions %08X/%08X are not contiguous slots' % (row['unit'], row['section'], a0, b0))
            lo, hi = spans[0][0], spans[-1][1]
            replaced = row['section'] in all_c.get(row['unit'], ())
            if start < lo or (end > hi and not replaced):
                fail('%s: %s [%08X,%08X) exceeds its slots [%08X,%08X)' % (row['unit'], row['section'], start, end, lo, hi))
            row['slot'] = (lo, hi)
            if replaced:
                if start != lo:
                    fail('%s: replaced C section %s does not start at its slot %08X' % (row['unit'], row['section'], lo))
                row['all_c'] = True  # sized by the compiler; no tail
            elif end < hi:
                tails.append(dict(function='%08X' % fs[-1], symbol=slots[fs[-1]][1], unit=row['unit'],
                                  section=row['section'], start='%08X' % end, end='%08X' % hi, bytes=hi - end))
            holes.append((lo, hi, row))
        else:
            if row['section'] in all_c.get(row['unit'], ()):
                row['excluded'] = True  # port variant: jump table of a replaced function (its words point into it)
            holes.append((start, end, row))
    # Two units may claim the same zero-initialised BSS storage (one defines a struct that contains the
    # other's variable). Both sections are NOLOAD and all zero, so the inner one is linked as an alias of the
    # outer storage (reported); any other overlap is an error.
    holes.sort(key=lambda h: (h[0], -h[1]))
    kept, bss_aliases = [], []
    for h in holes:
        prev = kept[-1] if kept else None
        if prev and prev[1] > h[0]:
            a, b = prev[2], h[2]
            if a['nobits'] and b['nobits'] and h[1] <= prev[1] and a['unit'] != b['unit']:
                b['alias_of'] = out_name(a)
                bss_aliases.append(dict(outer=dict(unit=a['unit'], section=a['section'], start='%08X' % prev[0], end='%08X' % prev[1]),
                                        inner=dict(unit=b['unit'], section=b['section'], start='%08X' % h[0], end='%08X' % h[1])))
                problems.append('BSS storage claimed twice: %s %s [%08X,%08X) contains %s %s [%08X,%08X)' % (
                    a['unit'], a['section'], prev[0], prev[1], b['unit'], b['section'], h[0], h[1]))
                continue
            fail('overlap: %s %s [%08X,%08X) and %s %s [%08X,%08X)' % (a['unit'], a['section'], prev[0], prev[1],
                                                                      b['unit'], b['section'], h[0], h[1]))
        kept.append(h)
    holes = kept

    # Retail pieces around the holes, per segment.
    layout = read_layout()
    pieces = []
    for seg in layout:
        cur = seg['start']
        inside = [h for h in holes if h[0] < seg['end'] and h[1] > seg['start']]
        for lo, hi, row in inside:
            if lo < seg['start'] or hi > seg['end']:
                fail('%s %s [%08X,%08X) crosses the %s segment boundary' % (row['unit'], row['section'], lo, hi, seg['name']))
            if lo > cur:
                pieces.append(dict(segment=seg['name'], kind=seg['kind'], start=cur, end=lo))
            cur = hi
        if cur < seg['end']:
            pieces.append(dict(segment=seg['name'], kind=seg['kind'], start=cur, end=seg['end']))
    first, last = layout[0]['start'], layout[-1]['end']
    for lo, hi, row in holes:
        if lo < first or hi > last:
            fail('%s %s outside the layout' % (row['unit'], row['section']))

    # Symbol resolution.
    starts = [h[0] for h in holes]

    def owner(addr):
        i = bisect.bisect_right(starts, addr) - 1
        if i >= 0 and holes[i][0] <= addr < holes[i][1]:
            return holes[i][2]
        return None

    port_info, moved = None, []
    if port is not None:
        if sdk:
            for row in port_rows:
                row['obj'] = sdk['object_map'].get(row['obj'], row['obj'])
        port_info, moved = port_plan(port, port_rows, port_defined, defined, syms)
    elif port_rows:
        fail('port sections without --port-variant')

    def moved_array(addr):
        for m in moved:
            if m['retail_va'] <= addr < m['retail_end']:
                return m
        return None

    header = json.loads((CONFIG / 'exe_header.json').read_text(encoding='ascii'))
    entry = header['pc0']['symbol']
    needed = dict(imports)
    needed.setdefault(entry, ['<entry>'])
    if '_gp' in defined:
        fail('_gp is defined by %s' % defined['_gp']['unit'])
    assignments, asserts, kinds = [], [], {'object': 0, 'alias': 0, 'interior': 0, 'absolute': 0, 'gp': 0}
    if port is not None:
        kinds.update(port=0, port_redirect=0)
    interior, into_tail, unresolved, mismatched, allc_labels = [], [], [], [], []
    for name in sorted(needed):
        if name == '_gp':
            kinds['gp'] += 1
            continue
        if name in port_defined:
            kinds['port'] += 1  # resolved by the linker: the port object defines it
            continue
        addr = address_for(name, syms)
        if name == entry:
            addr = int(header['pc0']['address'], 16)
        m = moved_array(addr) if addr is not None else None
        if m is not None:
            if name in defined:
                fail('%s defines %s inside the retail storage of moved array %s' % (defined[name]['unit'], name, m['name']))
            off = addr - m['retail_va']
            if off and m.get('layout') == 'relayout':
                fail('%s (%08X) is inside %s, whose entries change layout in the variant: only its start maps '
                     '(use the array in the C instead)' % (name, addr, m['name']))
            assignments.append((name, m['name'] if not off else '%s + 0x%X' % (m['name'], off)))
            port_info['redirects'].append(dict(name=name, retail='%08X' % addr, array=m['name'], offset=off,
                                               users=needed[name]))
            kinds['port_redirect'] += 1
            continue
        if name in defined:
            kinds['object'] += 1
            if addr is not None and addr != defined[name]['address']:
                mismatched.append(dict(name=name, map='%08X' % addr, object='%08X' % defined[name]['address'],
                                       unit=defined[name]['unit']))
            asserts.append((name, defined[name]['address'] if addr is None else addr))
            continue
        if addr is None:
            unresolved.append(dict(name=name, units=needed[name]))
            continue
        row = owner(addr)
        rhs = '0x%08X' % addr
        if row is not None and row.get('all_c') and addr != row['base']:
            # a label inside retail hand-written asm whose function is replaced C: the C has no such
            # point, so the name stands for the replaced function's entry (its users only store it).
            at_start = [n for n, a in row['globals'].items() if a == row['base']]
            if not at_start:
                fail('%s %s: replaced C section has no global at its start' % (row['unit'], row['section']))
            rhs = sorted(at_start)[0]
            kinds['interior'] += 1
            allc_labels.append(dict(name=name, address='%08X' % addr, expr=rhs, unit=row['unit'],
                                    section=row['section'], users=needed[name]))
        elif row is not None:
            end = row['base'] + row['size']
            if addr >= end:
                into_tail.append(dict(name=name, address='%08X' % addr, unit=row['unit'], section=row['section'],
                                      users=needed[name]))
                kinds['absolute'] += 1
            else:
                below = [(a, n) for n, a in row['globals'].items() if a <= addr]
                if below:
                    a, n = max(below)
                    rhs = n if a == addr else '%s + 0x%X' % (n, addr - a)
                else:
                    rhs = 'ADDR(%s) + 0x%X' % (out_name(row), addr - row['base'])
                if below and a == addr:
                    kinds['alias'] += 1
                else:
                    kinds['interior'] += 1
                    interior.append(dict(name=name, address='%08X' % addr, expr=rhs, unit=row['unit'],
                                         section=row['section'], users=needed[name]))
        else:
            kinds['absolute'] += 1
        assignments.append((name, rhs))
        asserts.append((name, addr))
    if unresolved:
        fail('unresolved imports: ' + ', '.join('%s (%s)' % (u['name'], u['units'][0]) for u in unresolved[:20]))
    if mismatched:
        fail('object symbols away from their mapped address: ' + json.dumps(mismatched[:20]))
    clash = sorted({n for n, _ in assignments} & set(defined))
    if clash:
        fail('names both defined by an object and assigned in symbols.ld: ' + ', '.join(clash))
    if into_tail:
        problems.append('imports pointing into zero-filled slot tails: %d' % len(into_tail))

    for m in moved:  # no unit may keep a section inside a moved array's retail storage
        for row in sections:
            if row['base'] < m['retail_end'] and m['retail_va'] < row['base'] + row['size']:
                fail('%s %s [%08X,+0x%X) overlaps the retail storage of moved array %s' % (
                    row['unit'], row['section'], row['base'], row['size'], m['name']))
    if port_info is not None:
        port_info['rows'] = port_rows
    return dict(build_dir=build_dir, selection=selection, sections=sections, discards=discards, pieces=pieces, tails=tails,
                port=port_info,
                assignments=assignments, asserts=asserts, symbol_kinds=kinds, interior=interior,
                into_tail=into_tail, bss_aliases=bss_aliases, warnings=warnings, problems=problems, entry=entry,
                all_c_labels=allc_labels,
                functions=functions, unselected_functions=unselected, link_objects=link_objects, sdk_lib=sdk)


# ---------------------------------------------------------------------------------------------------------
# --sdk-from-lib (docs/SOURCE_BUILD.md, "SDK from the PsyQ libraries")
#
# Every module that config/US/sdk_modules.tsv marks exact in PsyQ 3.3 is converted from the local .LIB
# (tools/lnk2elf.py, into work/sdk_lib/) and linked in place of the C sections of its functions: its .text
# at the module's retail start, each data section at the address sdk_modules.tsv located it (C data
# sections lying inside it are dropped too). A data section that is located but not equal to retail (libetc
# INTR .rdata, the rcsid string) or that overlaps a kept C section only partly is externalized: the module
# refers to it as D_<retail address> and its bytes stay with the retail piece / C unit. Unlocated data that
# no patch refers to (the CD-register .data of CDROM/C_0xx) is left out. The names a module imports
# resolve to the addresses its patches hold in retail (sectstart/sectend become D_<address> labels); the C
# objects keep importing sub_<VA>/IDA names, which symbols.ld aliases to the Sony names. Unit objects that
# lose sections are linked as filtered copies (work dir): the dropped sections emptied and renamed to a discarded name (.pdr, ...),
# their globals made undefined so that they resolve to the module's symbols.

FILTER_DISCARD_NAMES = (b'.pdr', b'.reginfo', b'.comment', b'.text', b'.data', b'.bss')


def filter_object(src, dst, drop):
    """Copy ELF `src` to `dst` with the sections named in `drop` emptied (non-alloc, size 0, renamed to a
    discarded name), their relocation sections emptied and their global symbols made undefined. Fails if
    a kept section refers to a local symbol of a dropped section. Returns the names made undefined."""
    raw = bytearray(Path(src).read_bytes())
    elf = read_elf(src)
    secs = elf['sections']
    shoff, = struct.unpack_from('<I', raw, 32)
    shstrndx, = struct.unpack_from('<H', raw, 50)
    names = secs[shstrndx]['data']
    pos = -1
    for cand in FILTER_DISCARD_NAMES:  # any section name that the linker scripts discard (*(name) in DISCARD)
        pos = names.find(cand + b'\0')
        if pos >= 0:
            break
    if pos < 0:
        fail('filter_object: %s has no discarded section name to reuse' % src)
    drop_idx = {s['index'] for s in secs if s['name'] in drop}
    if len(drop_idx) != len(drop):
        fail('filter_object: %s lacks sections %s' % (src, sorted(set(drop) - {s['name'] for s in secs})))
    rels = read_relocs(elf)
    for s in secs:
        if s['index'] in drop_idx or not s['flags'] & SHF_ALLOC:
            continue
        for off, rtype, sym in rels.get(s['index'], []):
            if sym['binding'] == 0 and sym['section_index'] in drop_idx:
                fail('filter_object: %s %s+0x%X refers to a local symbol of dropped section %s' % (
                    src, s['name'], off, secs[sym['section_index']]['name']))
    for s in secs:
        h = shoff + 40 * s['index']
        if s['index'] in drop_idx:
            struct.pack_into('<IIII', raw, h, pos, 1, 0, 0)           # name, PROGBITS, no flags, addr
            struct.pack_into('<I', raw, h + 20, 0)                    # size
        elif s['type'] == 9 and s['info'] in drop_idx:
            struct.pack_into('<I', raw, h + 20, 0)
    undefined = []
    symtab = next(s for s in secs if s['type'] == 2)
    for i, sym in enumerate(elf['symbols']):
        if sym['binding'] in (1, 2) and sym['section_index'] in drop_idx:
            o = symtab['offset'] + 16 * i
            struct.pack_into('<IIBBH', raw, o + 4, 0, 0, (sym['binding'] << 4) | 0, 0, 0)
            undefined.append(sym['name'])
    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    Path(dst).write_bytes(bytes(raw))
    return undefined


def read_sdk_c_modules(path=None):
    """config/US/sdk_c_modules.tsv: per port define, the exact PsyQ modules that --sdk-from-lib leaves to the C
    units while that define is on (docs/SPU_VOICES.md 4). Returns {define: [(lib, module), ...]}."""
    out = {}
    for r in read_tsv(path or CONFIG / 'sdk_c_modules.tsv'):
        out.setdefault(r['define'], []).append((r['lib'], r['module']))
    return out


def sdk_c_override(defines):
    """The (lib, module) set whose code stays C for these -D defines (names; `NAME=VALUE` counts as NAME)."""
    names = {d.split('=')[0] for d in defines}
    keep = set()
    for define, mods in read_sdk_c_modules().items():
        if define in names:
            keep.update(mods)
    return keep


def apply_sdk_from_lib(sections, defined, imports, discards, syms, slots, work_dir, warnings, problems, keep_c=None):
    import lnk2elf
    import psyq_lib
    if work_dir is None:
        fail('--sdk-from-lib needs an output directory for the filtered unit objects')
    work_dir = Path(work_dir)
    img = psyq_lib.Image()
    mods = lnk2elf.exact_modules()
    kept_c = []
    if keep_c:
        # per-module override (sdk_c_modules.tsv): these modules' functions stay the C units' sections
        known = {(m['lib'], m['module']) for m in mods}
        unknown = sorted(set(keep_c) - known)
        if unknown:
            fail('sdk_c_modules.tsv names modules that are not exact PsyQ 3.3 modules: ' +
                 ', '.join('%s:%s' % k for k in unknown))
        kept_c = [m for m in mods if (m['lib'], m['module']) in keep_c]
        mods = [m for m in mods if (m['lib'], m['module']) not in keep_c]
        have = {va for row in sections if row['code'] for va in row['functions']}
        missing = sorted(va for m in kept_c for va in m['functions'] if va not in have)
        if missing:
            fail('sdk_c_modules.tsv: functions of modules left to C have no C section: ' +
                 ' '.join('%08X' % v for v in missing))
    covered = {}
    for m in mods:
        for va in m['functions']:
            if va in covered:
                fail('function %08X in two exact modules' % va)
            covered[va] = m
    drop = []
    for row in sections:
        if row['code'] and row['functions']:
            inside = [va for va in row['functions'] if va in covered]
            if inside and len(inside) != len(row['functions']):
                fail('%s %s holds library functions %s and others %s' % (
                    row['unit'], row['section'], ' '.join('%08X' % v for v in inside),
                    ' '.join('%08X' % v for v in row['functions'] if v not in covered)))
            if inside:
                drop.append(row)
    dropped_ids = {id(r) for r in drop}

    def kept_overlaps(lo, hi):
        return [r for r in sections if id(r) not in dropped_ids and r['base'] < hi and lo < r['base'] + r['size']]

    lib_rows, report, collisions, externalized, omitted, extra = [], [], [], [], [], {}
    for m in mods:
        mod = lnk2elf.load_module(m['lib'], m['module'])
        obj = mod.obj
        key = '%s:%s' % (m['lib'], m['module'])
        text = obj.section('.text')
        if text.size != m['size']:
            fail('%s: .text is 0x%X bytes, sdk_modules.tsv says 0x%X' % (key, text.size, m['size']))
        refs = lnk2elf.referenced_sections(obj)
        bases = {text.id: m['start']}
        extern, omit, linked = {}, [], []
        for s in obj.sections.values():
            if not s.size or s.id == text.id:
                continue
            addr, status = m['data'].get(s.name, (None, 'not in sdk_modules.tsv'))
            if addr is None:
                if s.id in refs or any(sec == s.id for _, sec, _ in obj.xdefs.values()):
                    fail('%s: %s is referenced but has no retail address (%s)' % (key, s.name, status))
                omit.append(s.name)
                omitted.append(dict(module=key, section=s.name, size=s.size, status=status))
                continue
            bases[s.id] = addr
            ov = kept_overlaps(addr, addr + s.size)
            partial = [r for r in ov if r['code'] or r['base'] < addr or r['base'] + r['size'] > addr + s.size]
            if not (status.startswith('eq') or status == 'bss') or partial:
                extern[s.name] = 'D_%08X' % addr
                why = ('data differs from retail (%s)' % status if not (status.startswith('eq') or status == 'bss') else
                       'overlaps kept sections only partly: ' + ', '.join('%s %s' % (r['unit'], r['section']) for r in partial))
                externalized.append(dict(module=key, section=s.name, address='%08X' % addr, size=s.size, reason=why))
                continue
            for r in ov:
                drop.append(r)
                dropped_ids.add(id(r))
            linked.append(s)
        # names the module imports: their retail values
        seen = lnk2elf.observe(obj, bases, img.word)
        names = {term[1] for s, off, kind, term, k, e in lnk2elf.patches(obj) if term and term[0] == 'name'}
        rename = {}
        for n in sorted(names):
            vals = seen.get(n, [])
            if len(vals) > 1:
                fail('%s: patches of %s hold different retail values %s' % (key, n, ' '.join('%08X' % v for v in vals)))
            v = vals[0] if vals else None
            if n.startswith(('sectstart(', 'sectend(')):
                if v is None:
                    fail('%s: no retail value for %s' % (key, n))
                rename[n] = 'D_%08X' % v
                continue
            known = address_for(n, syms) if n not in extra else extra[n]
            if v is None and known is None:
                fail('%s: import %s has no retail value and no known address' % (key, n))
            if v is not None and known is not None and v != known:
                rename[n] = 'D_%08X' % v
                collisions.append('%s imports %s: its patches hold %08X, the retail names say %08X; linked as D_%08X'
                                  % (key, n, v, known, v))
            elif v is not None and known is None:
                extra[n] = v
        path, info = lnk2elf.cached_convert(mod, extern, omit, rename)
        rel = path.relative_to(ROOT).as_posix()
        elf = read_elf(path)
        unit = 'psyq %s' % key
        base_by_elf = {lnk2elf.elf_name(obj.sections[sid].name): a for sid, a in bases.items()}
        for sec in elf['sections']:
            if not sec['flags'] & SHF_ALLOC:
                continue
            base = base_by_elf[sec['name']]
            code = bool(sec['flags'] & SHF_EXECINSTR)
            nobits = sec['type'] == SHT_NOBITS
            data = b'\0' * sec['size'] if nobits else sec['data']
            row = dict(unit=unit, unit_index=-1, obj=rel, section=sec['name'], base=base, size=sec['size'],
                       align=sec['alignment'], code=code, nobits=base >= BSS_START, placed_by='sdk_modules.tsv',
                       functions=[], globals={}, data_sha256=hashlib.sha256(data).hexdigest(), lib=key, _data=data)
            if base % max(sec['alignment'], 1):
                fail('%s: %s at %08X violates its alignment %d' % (key, sec['name'], base, sec['alignment']))
            if base < BSS_START < base + sec['size']:
                fail('%s: %s straddles the BSS start' % (key, sec['name']))
            if row['nobits'] and any(data):
                fail('%s: %s lies in BSS but has contents' % (key, sec['name']))
            if code:
                fs = sorted(m['functions'])
                end = base + sec['size']
                if fs[0] != base:
                    fail('%s: the first function %08X is not the module start %08X' % (key, fs[0], base))
                stray = [va for va in slots if base <= va < end and va not in fs]
                if stray:
                    fail('%s: functions %s lie inside the module but are not in sdk_modules.tsv' % (
                        key, ' '.join('%08X' % v for v in stray)))
                hi = max(end, slots[fs[-1]][0])
                row['functions'] = fs
                row['slot'] = (base, hi)
                row['function_ends'] = {va: (fs[i + 1] if i + 1 < len(fs) else hi) for i, va in enumerate(fs)}
            lib_rows.append(row)
        for sym in elf['symbols']:
            if sym['binding'] in (1, 2) and 0 < sym['section_index'] < 0xff00:
                sec = elf['sections'][sym['section_index']]
                row = next(r for r in lib_rows if r['obj'] == rel and r['section'] == sec['name'])
                row['globals'][sym['name']] = row['base'] + sym['address']
            elif sym['binding'] in (1, 2) and sym['section_index'] == 0:
                imports.setdefault(sym['name'], []).append(unit)
        report.append(dict(module=key, object=rel, text='%08X' % m['start'], text_bytes=text.size,
                           functions=len(m['functions']), data_linked=['%s@%08X' % (s.name, bases[s.id]) for s in linked],
                           externalized=sorted(extern), omitted=omit, renamed=rename, relocations=info['relocations']))

    # drop the C sections, make their names resolvable by address
    dropped_functions, dropped_sections = {}, []
    by_obj = {}
    for row in drop:
        sections.remove(row)
        by_obj.setdefault(row['obj'], []).append(row)
        dropped_sections.append(dict(unit=row['unit'], section=row['section'], base='%08X' % row['base'], size=row['size']))
        for name, addr in row['globals'].items():
            if defined.get(name, {}).get('section') is row:
                del defined[name]
            known = extra.get(name, address_for(name, syms))
            if known is None:
                extra[name] = addr
            elif known != addr:
                fail('dropped %s %s defines %s at %08X, the retail names say %08X' % (row['unit'], row['section'], name, addr, known))
        for va in row['functions']:
            dropped_functions['%08X' % va] = slots[va][1]
    for name, addr in extra.items():
        syms.setdefault(name, addr)
    object_map = {}
    for i, (objpath, rows) in enumerate(sorted(by_obj.items())):
        dst = work_dir / 'sdk_units' / ('%03d_%s' % (i, Path(objpath).name))
        undefined = filter_object(ROOT / objpath, dst, {r['section'] for r in rows})
        new = dst.resolve().relative_to(ROOT).as_posix()
        object_map[objpath] = new
        unit = rows[0]['unit']
        for name in undefined:
            imports.setdefault(name, []).append(unit)
    for row in sections:
        row['obj'] = object_map.get(row['obj'], row['obj'])
    discards[:] = [(object_map.get(o, o), s) for o, s in discards]
    for row in lib_rows:
        for name, addr in row['globals'].items():
            if name in defined:
                collisions.append('%s defines %s, also defined by %s' % (row['lib'], name, defined[name]['unit']))
                fail('symbol %s defined by both %s and %s' % (name, row['lib'], defined[name]['unit']))
            known = address_for(name, syms)
            if known is not None and known != addr:
                collisions.append('%s defines %s at %08X; the retail names put %s at %08X' % (row['lib'], name, addr, name, known))
            defined[name] = dict(unit=row['unit'], address=addr, section=row)
        sections.append(row)
    for c in collisions:
        warnings.append('sdk-from-lib: ' + c)
    nfun = sum(len(m['functions']) for m in mods)
    fbytes = sum(slots[va][0] - va for m in mods for va in m['functions'])
    c_override = [dict(module='%s:%s' % (m['lib'], m['module']), text='%08X' % m['start'],
                       functions=['%08X' % va for va in sorted(m['functions'])]) for m in kept_c]
    return dict(modules=report, lib_objects=sorted({r['obj'] for r in lib_rows}), object_map=object_map,
                c_override=c_override,
                dropped_functions=dropped_functions, dropped_sections=dropped_sections, externalized=externalized,
                omitted=omitted, collisions=collisions, resolved_imports={n: '%08X' % a for n, a in sorted(extra.items())},
                coverage=dict(modules=len(mods), functions=nfun, function_bytes=fbytes,
                              c_override_modules=len(kept_c),
                              c_override_functions=sum(len(m['functions']) for m in kept_c),
                              text_bytes=sum(m['size'] for m in mods),
                              data_sections_linked=sum(len(r['data_linked']) for r in report),
                              data_sections_externalized=len(externalized), data_sections_omitted=len(omitted),
                              c_sections_dropped=len(dropped_sections), c_functions_dropped=len(dropped_functions)))


def out_name(row):
    return '.c.%08X' % row['base']


def piece_name(p):
    return '.r.%08X' % p['start']


PIECE_FLAGS = {'code': '"ax",@progbits', 'rodata': '"a",@progbits', 'data': '"aw",@progbits',
               'sdata': '"aw",@progbits', 'bss_in_file': '"aw",@nobits', 'bss': '"aw",@nobits'}


def segment_asm(seg_name, pieces, split_row):
    lines = ['# Generated by tools/gen_link.py: retail pieces of segment %s' % seg_name]
    for p in pieces:
        size = p['end'] - p['start']
        lines += ['\t.section %s,%s' % (piece_name(p), PIECE_FLAGS[p['kind']]), '\t.p2align 0']
        if p['kind'] in NOBITS_KINDS:
            lines.append('\t.space 0x%X' % size)
        else:
            seg_start = int(split_row['start'], 16)
            lines.append('\t.incbin "split/%s", 0x%X, 0x%X' % (split_row['file'], p['start'] - seg_start, size))
    return '\n'.join(lines) + '\n'


DISCARD = ('.MIPS.abiflags', '.reginfo', '.MIPS.options', '.pdr', '.comment', '.gnu.attributes', '.mdebug*',
           '.debug*', '.line', '.note*', '.text', '.data', '.bss', '.rodata', '.sdata', '.sbss')


def linker_script(p, piece_objs):
    """Main script: one output section per object section and per retail piece, sorted by address."""
    rows = []
    for row in p['sections']:
        rows.append((row['base'], out_name(row), row['nobits'], 'KEEP(%s(%s))' % (row['obj'], row['section'])))
    for piece in p['pieces']:
        obj = piece_objs[piece['segment']]
        rows.append((piece['start'], piece_name(piece), piece['kind'] in NOBITS_KINDS,
                     'KEEP(%s(%s))' % (obj, piece_name(piece))))
    rows.sort()
    out = ['/* Generated by tools/gen_link.py (M2 nonmatching source-only link). */',
           'OUTPUT_FORMAT("elf32-littlemips")', 'OUTPUT_ARCH(mips)', 'ENTRY(%s)' % p['entry'],
           'INCLUDE symbols.ld', '', 'SECTIONS', '{']
    for addr, name, nobits, spec in rows:
        out.append('  %s 0x%08X%s : { %s }' % (name, addr, ' (NOLOAD)' if nobits else '', spec))
    out.append('  .rel.dyn : { *(.rel.dyn) }')
    out.append('  /DISCARD/ : {')
    for obj, sec in p['discards']:
        out.append('    %s(%s)' % (obj, sec))
    out.append('    %s' % ' '.join('*(%s)' % s for s in DISCARD))
    out += ['  }', '}', '', 'ASSERT(SIZEOF(.rel.dyn) == 0, "dynamic relocations in a static image")']
    for row in p['sections']:
        out.append('ASSERT(ADDR(%s) == 0x%08X && SIZEOF(%s) == 0x%X, "%s %s misplaced")'
                   % (out_name(row), row['base'], out_name(row), row['size'], row['unit'], row['section']))
    for piece in p['pieces']:
        out.append('ASSERT(ADDR(%s) == 0x%08X && SIZEOF(%s) == 0x%X, "retail piece %08X misplaced")'
                   % (piece_name(piece), piece['start'], piece_name(piece), piece['end'] - piece['start'], piece['start']))
    for va, name in sorted(p['functions'].items()):
        out.append('ASSERT(%s == 0x%08X, "function %s is not at %08X")' % (name, va, name, va))
    return '\n'.join(out) + '\n'


def symbols_script(p):
    out = ['/* Generated by tools/gen_link.py: imports of the unit objects. */',
           '_gp = 0x%08X;' % GP]
    for name, rhs in p['assignments']:
        out.append('%s = %s;' % (name, rhs))
    out.append('')
    out.append('ASSERT(_gp == 0x%08X, "_gp")' % GP)
    for name, addr in p['asserts']:
        out.append('ASSERT(%s == 0x%08X, "%s")' % (name, addr, name))
    return '\n'.join(out) + '\n'


def plan_json(p):
    def row_json(r):
        return {k: ('%08X' % v if k in ('base', 'ram_end') else v) for k, v in r.items() if not k.startswith('_')} | dict(
            functions=['%08X' % v for v in sorted(r['functions'])],
            globals={n: '%08X' % a for n, a in sorted(r['globals'].items())},
            slot=['%08X' % v for v in r['slot']] if 'slot' in r else None, out=out_name(r))
    return dict(schema='tm2-source-link-plan-v1', build_dir=p['build_dir'].relative_to(ROOT).as_posix(),
                sections=[row_json(r) for r in sorted(p['sections'], key=lambda r: r['base'])],
                pieces=[dict(segment=x['segment'], kind=x['kind'], start='%08X' % x['start'], end='%08X' % x['end'],
                             out=piece_name(x)) for x in p['pieces']],
                tails=p['tails'], bss_aliases=p['bss_aliases'], symbol_kinds=p['symbol_kinds'], interior=p['interior'], into_tail=p['into_tail'],
                assignments=[dict(name=n, expr=e) for n, e in p['assignments']],
                warnings=p['warnings'], problems=p['problems'],
                unselected_functions=['%08X' % v for v in p['unselected_functions']],
                link_objects=p['link_objects'], sdk_lib=p['sdk_lib'])


# ---------------------------------------------------------------------------------------------------------
# Relocatable flavour (docs/SOURCE_BUILD.md, "Relocatable link").
#
# Same objects and retail pieces as plan(), but no output section is pinned to a retail address except the
# first one (t_addr). Every item (C section, zero-filled slot tail, retail piece, optional shift padding) is
# laid out sequentially in retail order with its alignment; a C section free to have its compiled size
# pushes everything after it. Retail pieces become generated .s files: a global label at every image/BSS
# address an object imports, `.incbin` for plain bytes and `.word target+addend` for every
# config/US/data_relocs.tsv slot they hold. symbols.ld keeps only out-of-image absolute addresses
# (hardware, scratchpad, the 0x80010000-area buffers); _gp is the start of sdata as laid out.

def read_tsv(path):
    lines = [l for l in Path(path).read_text(encoding='utf-8').splitlines() if l and not l.startswith('#')]
    return list(csv.DictReader(lines, delimiter='\t'))


def read_data_relocs(path=CONFIG / 'data_relocs.tsv'):
    return [dict(address=int(r['address'], 16), value=int(r['value'], 16), kind=r['kind'],
                 target_symbol=r['target_symbol'], confidence=r['confidence']) for r in read_tsv(path)]


def read_code_abs_refs(path=CONFIG / 'code_abs_refs.tsv'):
    return [dict(address=int(r['address'], 16), value=int(r['value'], 16), kind=r['kind'],
                 target_symbol=r['target_symbol'], owner=r['owner'], status=r['status']) for r in read_tsv(path)]


def split_symbol(expr):
    """'sub_X+0x34' -> ('sub_X', 0x34)."""
    m = re.match(r'^([A-Za-z_.$][\w.$]*)\s*(?:([+-])\s*(0x[0-9A-Fa-f]+|\d+))?$', expr.strip())
    if not m:
        raise ValueError('bad symbol expression ' + expr)
    off = int(m.group(3), 0) if m.group(3) else 0
    return m.group(1), -off if m.group(2) == '-' else off


def parse_shift(text):
    """'800D1318:16' -> (0x800D1318, 16): insert 16 bytes of padding before retail address 800D1318."""
    m = re.match(r'^([0-9A-Fa-f]{8}):(0x[0-9A-Fa-f]+|\d+)$', text or '')
    if not m:
        raise LinkError('gen_link: --shift must be ADDR:N (hex address, byte count), got %r' % text)
    addr, n = int(m.group(1), 16), int(m.group(2), 0)
    if n <= 0 or n % 4:
        fail('--shift %s: the padding must be a positive multiple of 4' % text)
    return addr, n


def lowbit_align(addr, cap=4):
    a = 1
    while a < cap and not addr & a:
        a <<= 1
    return a


def reloc_plan(p, shift=None, corrupt=()):
    """Relocatable layout of plan() `p`: ordered items, labels, data words and script symbols."""
    layout = read_layout()
    t_addr, bss_end = layout[0]['start'], layout[-1]['end']
    segs = [(s['start'], s['end'], s) for s in layout]

    def seg_of(addr):
        for lo, hi, s in segs:
            if lo <= addr < hi:
                return s
        return layout[-1]

    items = []
    for row in p['sections']:
        start, end = row['base'], row['base'] + row['size']
        if row.get('all_c'):
            end = row['slot'][1]  # retail extent; the output section has the compiled size
        seg = seg_of(start)
        align = max(row['align'], 1)
        if row['code']:
            align = max(align, 4)
            if row['slot'][0] != start:
                fail('%s %s starts after its first slot (%08X)' % (row['unit'], row['section'], row['slot'][0]))
        items.append(dict(type='c', name=out_name(row), r_start=start, r_end=end, nobits=row['nobits'],
                          code=row['code'], align=align, seg=seg['name'], seg_kind=seg['kind'], obj=row['obj'],
                          section=row['section'], unit=row['unit'], alias_of=row.get('alias_of'),
                          functions=[['%08X' % va, '%08X' % row.get('function_ends', {}).get(va, load_functions_cache()[va][0]),
                                      load_functions_cache()[va][1]] for va in sorted(row['functions'])],
                          globals=row['globals'], **({'lib': row['lib']} if row.get('lib') else {}),
                          **({'all_c': True, 'compiled_size': row['size']} if row.get('all_c') else {}),
                          **({'excluded': True} if row.get('excluded') else {})))
        if row['code'] and end < row['slot'][1]:
            items.append(dict(type='tail', name='.t.%08X' % end, r_start=end, r_end=row['slot'][1], nobits=False,
                              code=True, align=1, seg=seg['name'], seg_kind=seg['kind'],
                              function='%08X' % sorted(row['functions'])[-1], unit=row['unit']))
    for piece in p['pieces']:
        items.append(dict(type='r', name=piece_name(piece), r_start=piece['start'], r_end=piece['end'],
                          nobits=piece['kind'] in NOBITS_KINDS, code=piece['kind'] == 'code',
                          align=lowbit_align(piece['start']), seg=piece['segment'], seg_kind=piece['kind']))

    if shift:
        addr, n = shift
        if not t_addr < addr < bss_end:
            fail('--shift address %08X outside the image (%08X..%08X)' % (addr, t_addr, bss_end))
        for it in list(items):
            if it['r_start'] < addr < it['r_end']:
                if it['type'] != 'r':
                    fail('--shift %08X falls inside %s (%s); choose an item boundary' % (addr, it['name'], it.get('unit', 'tail')))
                second = dict(it, name=piece_name(dict(start=addr)), r_start=addr, align=lowbit_align(addr))
                it['r_end'] = addr
                items.append(second)
        nxt = min((it for it in items if it['r_start'] >= addr), key=lambda it: (it['r_start'], it['r_end']))
        items.append(dict(type='aux', name='.x.%08X' % addr, r_start=addr, r_end=addr, size=n, nobits=nxt['nobits'],
                          code=nxt['code'], align=1, seg=nxt['seg'], seg_kind=nxt['seg_kind']))
    order = {'aux': 0}
    items.sort(key=lambda it: (it['r_start'], order.get(it['type'], 1), it['r_end'] - it['r_start'] > 0))
    for a, b in zip(items, items[1:]):
        if not b.get('alias_of') and not a.get('alias_of') and a['r_end'] != b['r_start'] and b['type'] != 'aux' and a['type'] != 'aux':
            fail('items do not tile: %s ends %08X, %s starts %08X' % (a['name'], a['r_end'], b['name'], b['r_start']))
    if items[0]['r_start'] != t_addr or max(it['r_end'] for it in items) != bss_end:
        fail('items do not cover %08X..%08X' % (t_addr, bss_end))

    generated = [it for it in items if it['type'] in ('r', 'tail')]
    gen_starts = [it['r_start'] for it in generated]
    c_items = sorted((it for it in items if it['type'] == 'c' and it['r_end'] > it['r_start'] and not it.get('alias_of')),
                     key=lambda it: it['r_start'])
    c_starts = [it['r_start'] for it in c_items]

    def gen_container(addr):
        i = bisect.bisect_right(gen_starts, addr) - 1
        while i >= 0 and generated[i]['r_start'] == generated[i]['r_end']:
            i -= 1
        if i >= 0 and generated[i]['r_start'] <= addr < generated[i]['r_end']:
            return generated[i]
        return None

    def c_container(addr):
        i = bisect.bisect_right(c_starts, addr) - 1
        if i >= 0 and c_items[i]['r_start'] <= addr < c_items[i]['r_end']:
            return c_items[i]
        return None

    last = items[-1]
    defined_names = set()
    for it in items:
        defined_names |= set(it.get('globals', ()))
    taken = defined_names | {n for n, _ in p['assignments']}
    ida_at = {}
    for r in csv.DictReader((CONFIG / 'ida_names.tsv').open(), delimiter='\t'):
        if r['code'] == '0' and re.match(r'^[A-Za-z_]\w*$', r['name']):
            ida_at.setdefault(int(r['va'], 16), r['name'])
    for r in load_renames():  # config/US/names.tsv: the readable name wins over the IDA name
        if r['va'] in ida_at:
            ida_at[r['va']] = r['new']
    labels = {}  # item name -> {offset: [names]}
    helpers = {}  # helper symbol -> linker-script expression (targets inside C sections without a global)
    end_symbols = []  # names at the very end of the layout (BSS end) defined by the script

    def add_label(it, addr, name):
        labels.setdefault(it['name'], {}).setdefault(addr - it['r_start'], [])
        if name not in labels[it['name']][addr - it['r_start']]:
            labels[it['name']][addr - it['r_start']].append(name)

    # Absolute imports inside the image/BSS become labels in the retail pieces (or the BSS-end symbol).
    assignments, relabelled, kept_absolute = [], [], []
    for name, rhs in p['assignments']:
        if not rhs.startswith('0x'):
            assignments.append((name, rhs))
            continue
        addr = int(rhs, 16)
        if addr < t_addr or addr > bss_end:
            assignments.append((name, rhs))
            kept_absolute.append(dict(name=name, address='%08X' % addr))
            continue
        it = gen_container(addr)
        if it is None and addr == bss_end:
            end_symbols.append(name)
            relabelled.append(dict(name=name, address='%08X' % addr, item='<end>'))
            continue
        if it is None:
            c = c_container(addr)
            fail('absolute import %s (%08X) is not inside a retail piece (%s)' % (name, addr, c and c['name']))
        add_label(it, addr, name)
        relabelled.append(dict(name=name, address='%08X' % addr, item=it['name']))

    def target_expr(value):
        """A symbol expression for the image address `value`."""
        c = c_container(value)
        if c is not None:
            below = [(a, n) for n, a in c['globals'].items() if a <= value]
            if below:
                a, n = max(below)
                return n, value - a
            h = '__reloc_%08X' % value
            helpers[h] = 'ADDR(%s) + 0x%X' % (c['name'], value - c['r_start'])
            return h, 0
        it = gen_container(value)
        if it is None:
            if value == bss_end:
                if not end_symbols:
                    end_symbols.append('__image_bss_end')
                return end_symbols[0], 0
            fail('pointer target %08X is in no item' % value)
        names = labels.get(it['name'], {}).get(value - it['r_start'])
        if names:
            return names[0], 0
        name = ida_at.get(value)  # the retail (IDA) name of the target, else D_<addr>
        if not name or name in taken:
            name = 'D_%08X' % value
        if name in taken:
            name = 'R_%08X' % value
        taken.add(name)
        add_label(it, value, name)
        return name, 0

    words = {}  # item name -> {offset: (symbol, addend, slot)}
    slots, corrupted = [], []
    corrupt = set(corrupt)
    for slot in read_data_relocs():
        addr = slot['address']
        it = gen_container(addr)
        if it is None:
            c = c_container(addr)
            if c is None:
                fail('data_relocs slot %08X is in no item' % addr)
            slots.append(dict(slot, item=c['name'], via='object'))
            continue
        if it['nobits'] or it['type'] != 'r':
            fail('data_relocs slot %08X lies in %s (%s)' % (addr, it['name'], it['type']))
        if addr % 4 or addr + 4 > it['r_end']:
            fail('data_relocs slot %08X is unaligned or crosses the end of %s' % (addr, it['name']))
        sym, addend = target_expr(slot['value'])
        slots.append(dict(slot, item=it['name'], via='generated', symbol=sym, addend=addend))
        if addr in corrupt:
            corrupted.append('%08X' % addr)
            continue  # negative control: keep the raw retail word, no relocation
        words.setdefault(it['name'], {})[addr - it['r_start']] = (sym, addend, slot)
    missing = corrupt - {int(a, 16) for a in corrupted}
    if missing:
        fail('--corrupt-slot addresses that are not generated data_relocs slots: ' + ' '.join('%08X' % a for a in sorted(missing)))
    for name, offs in words.items():
        for off in offs:
            for lo in labels.get(name, {}):
                if off < lo < off + 4:
                    fail('label inside the data_relocs word %s+0x%X' % (name, off))

    # pc-relative branches that leave their section (code_abs_refs.tsv): the linker script asserts that the
    # distance is still the retail one (the two sections must stay adjacent).
    by_unit_sec = {(it.get('unit'), it.get('section')): it for it in items if it['type'] == 'c'}
    branch_asserts = []
    for ref in read_code_abs_refs():
        if ref['kind'] != 'branch_xfunc':
            fail('code_abs_refs: unsupported kind %s at %08X' % (ref['kind'], ref['address']))
        unit, sec = ref['owner'].split(' ')
        it = by_unit_sec.get((unit, sec))
        lib_it = c_container(ref['address'])
        if it is None and lib_it is not None and lib_it.get('lib') and lib_it['r_start'] <= ref['value'] < lib_it['r_end']:
            continue  # --sdk-from-lib: the branch is inside one PsyQ module section now; it moves with it
        if it is None:
            fail('code_abs_refs owner %s not linked' % ref['owner'])
        sym, addend = split_symbol(ref['target_symbol'])
        if sym not in defined_names:
            sym, addend = target_expr(ref['value'])
        branch_asserts.append(dict(item=it['name'], offset=ref['address'] - it['r_start'], symbol=sym, addend=addend,
                                   distance=ref['value'] - ref['address'], address='%08X' % ref['address']))

    sdata_start = next(s['start'] for s in layout if s['kind'] == 'sdata')
    gp_item = min((it for it in items if it['r_start'] >= sdata_start and it['type'] != 'aux'),
                  key=lambda it: (it['r_start'], it['r_end'] - it['r_start'] > 0))
    if gp_item['r_start'] != sdata_start:
        fail('no item starts at the sdata start %08X' % sdata_start)
    return dict(items=items, labels=labels, words=words, helpers=helpers, end_symbols=end_symbols, last=last['name'],
                assignments=assignments, relabelled=relabelled, kept_absolute=kept_absolute, slots=slots,
                corrupted=corrupted, branch_asserts=branch_asserts, gp_item=gp_item['name'],
                shift=None if not shift else dict(address='%08X' % shift[0], bytes=shift[1]), entry=p['entry'],
                t_addr=t_addr, bss_end=bss_end)


_FUNCS = None


def load_functions_cache():
    global _FUNCS
    if _FUNCS is None:
        _FUNCS = load_functions()
    return _FUNCS


def reloc_segment_asm(seg_name, rp, split_row):
    """Generated items (retail pieces, slot tails, shift padding) of one segment."""
    lines = ['# Generated by tools/gen_link.py --relocatable: segment %s' % seg_name]
    for it in rp['items']:
        if it['seg'] != seg_name or it['type'] not in ('r', 'tail', 'aux'):
            continue
        flags = '"ax",@progbits' if it['code'] else ('"aw",@nobits' if it['nobits'] else PIECE_FLAGS.get(it['seg_kind'], '"aw",@progbits'))
        if it['nobits']:
            flags = '"aw",@nobits'
        lines += ['\t.section %s,%s' % (it['name'], flags), '\t.align 0', '\t.p2align 0']
        size = it['size'] if it['type'] == 'aux' else it['r_end'] - it['r_start']
        events = {}
        for off, names in rp['labels'].get(it['name'], {}).items():
            events.setdefault(int(off), [[], None])[0].extend(names)
        for off, w in rp['words'].get(it['name'], {}).items():
            events.setdefault(int(off), [[], None])[1] = w
        if it['name'] == rp['last']:
            events.setdefault(size, [[], None])[0].extend(rp['end_symbols'])
        cur = 0

        def fill(a, b):
            if b <= a:
                return
            if it['type'] == 'r' and not it['nobits']:
                seg_start = int(split_row['start'], 16)
                lines.append('\t.incbin "split/%s", 0x%X, 0x%X' % (split_row['file'], it['r_start'] + a - seg_start, b - a))
            else:
                lines.append('\t.space 0x%X' % (b - a))
        for off in sorted(events):
            names, word = events[off]
            if off < cur:
                fail('overlapping events in %s at +0x%X' % (it['name'], off))
            fill(cur, off)
            cur = off
            for n in names:
                lines += ['\t.globl %s' % n, '%s:' % n]
            if word:
                sym, addend, slot = word
                lines.append('\t.word %s%s\t# %08X -> %08X %s' % (sym, ' + 0x%X' % addend if addend else '', slot['address'],
                                                              slot['value'], slot['kind']))
                cur = off + 4
        fill(cur, size)
    return '\n'.join(lines) + '\n'


def reloc_linker_script(p, rp, piece_objs):
    out = ['/* Generated by tools/gen_link.py --relocatable: sequential layout in retail order. */',
           'OUTPUT_FORMAT("elf32-littlemips")', 'OUTPUT_ARCH(mips)', 'ENTRY(%s)' % rp['entry'], '', 'SECTIONS', '{',
           '  . = 0x%08X;' % rp['t_addr']]
    for it in rp['items']:
        if it['type'] == 'c':
            spec = 'KEEP(%s(%s))' % (it['obj'], it['section'])
        else:
            spec = 'KEEP(%s(%s))' % (piece_objs[it['seg']], it['name'])
        noload = ' (NOLOAD)' if it['nobits'] else ''
        if it.get('alias_of'):
            outer = next(o for o in rp['items'] if o['name'] == it['alias_of'])
            out.append('  %s ADDR(%s) + 0x%X%s : { %s }' % (it['name'], outer['name'], it['r_start'] - outer['r_start'], noload, spec))
            out.append('  . = ADDR(%s) + SIZEOF(%s);' % (outer['name'], outer['name']))
            continue
        out.append('  %s ALIGN(%d)%s : { %s }' % (it['name'], it['align'], noload, spec))
    port = p.get('port')
    if port:
        # port variant: the grown arrays at the port base, outside the PS-X EXE image (elf_to_psx_exe skips them);
        # .port_bss is NOLOAD and must be zeroed by the port (crt0 clears only the retail BSS); then the port's
        # code and its constants (port_text.bin, port_rodata.bin)
        out.append('  . = 0x%08X;' % port['base'])
        for i, sec in enumerate(PORT_SECTIONS):
            spec = ' '.join('KEEP(%s(%s))' % (inp['obj'], sec) for inp in port['inputs'] if inp['section'] == sec)
            out.append('  %s %s%s : { %s }' % (sec, '.' if not i else 'ALIGN(%d)' % PORT_ALIGN,
                                              ' (NOLOAD)' if sec == '.port_bss' else '', spec))
    out.append('  .rel.dyn : { *(.rel.dyn) }')
    out.append('  /DISCARD/ : {')
    for obj, sec in p['discards']:
        out.append('    %s(%s)' % (obj, sec))
    out.append('    %s' % ' '.join('*(%s)' % s for s in DISCARD))
    out += ['  }', '}', '', 'INCLUDE symbols.ld', '',
            'ASSERT(SIZEOF(.rel.dyn) == 0, "dynamic relocations in a static image")',
            'ASSERT(ADDR(%s) == 0x%08X, "the image must start at t_addr")' % (rp['items'][0]['name'], rp['t_addr'])]
    for it in rp['items']:
        size = it['size'] if it['type'] == 'aux' else it['r_end'] - it['r_start']
        if it['type'] != 'c' or not it['code']:
            out.append('ASSERT(SIZEOF(%s) == 0x%X, "%s size")' % (it['name'], size, it['name']))
    for b in rp['branch_asserts']:
        out.append('ASSERT(%s + 0x%X - (ADDR(%s) + 0x%X) == 0x%X, "pc-relative branch at retail %s: keep its sections adjacent")'
                   % (b['symbol'], b['addend'], b['item'], b['offset'], b['distance'], b['address']))
    if port:
        last = rp['items'][-1]['name']
        out.append('ASSERT(ADDR(%s) + SIZEOF(%s) <= 0x%08X, "port variant: the image reaches the port base")' % (last, last, port['base']))
        end = max(port['addresses'][s] + port['sizes'][s] for s in PORT_SECTIONS)
        ram_end = port.get('ram_end') or DEFAULT_PORT_RAM_END
        if end > ram_end:
            fail('port variant: the port sections end at %08X, past the port RAM end (%08X, --port-ram-end)' % (
                end, ram_end))
        for sec in PORT_SECTIONS:
            if port['sizes'][sec]:  # (ld drops an empty output section)
                out.append('ASSERT(ADDR(%s) == 0x%08X && SIZEOF(%s) == 0x%X, "port variant: %s misplaced")' % (
                    sec, port['addresses'][sec], sec, port['sizes'][sec], sec))
        for inp in port['inputs']:  # each object's part where gen_link put it (the maps rely on these offsets)
            syms = [(n, o) for r in port['rows'] if r['obj'] == inp['obj'] and r['section'] == inp['section']
                    for n, o in sorted(r['globals'].items(), key=lambda kv: kv[1])][:1]
            for n, o in syms:
                out.append('ASSERT(%s == 0x%08X, "port variant: %s (%s) misplaced")' % (
                    n, port['addresses'][inp['section']] + inp['offset'] + o, n, inp['unit']))
        for a in port['arrays']:
            out.append('ASSERT(%s == ADDR(%s) + 0x%X, "port variant: %s misplaced")' % (a['name'], a['section'], a['offset'], a['name']))
    return '\n'.join(out) + '\n'


def reloc_symbols_script(p, rp):
    out = ['/* Generated by tools/gen_link.py --relocatable: imports of the unit objects. */',
           '_gp = ADDR(%s);' % rp['gp_item']]
    for name, rhs in rp['assignments']:
        out.append('%s = %s;' % (name, rhs))
    for name, rhs in sorted(rp['helpers'].items()):
        out.append('%s = %s;' % (name, rhs))
    last = next(it for it in rp['items'] if it['name'] == rp['last'])
    if last['type'] not in ('r', 'tail', 'aux'):
        for name in rp['end_symbols']:
            out.append('%s = ADDR(%s) + SIZEOF(%s);' % (name, last['name'], last['name']))
    out.append('')
    if 'D_801809D4' in {n for names in rp['labels'].values() for ns in names.values() for n in ns} | {n for n, _ in rp['assignments']}:
        out.append('ASSERT(_gp == D_801809D4, "crt0 loads gp from D_801809D4: it must be the start of sdata")')
    return '\n'.join(out) + '\n'


def reloc_plan_json(p, rp):
    def item_json(it):
        j = {k: v for k, v in it.items() if k not in ('globals',)}
        j['r_start'], j['r_end'] = '%08X' % it['r_start'], '%08X' % it['r_end']
        return j
    return dict(schema='tm2-reloc-link-plan-v1', build_dir=p['build_dir'].relative_to(ROOT).as_posix(),
                shift=rp['shift'], corrupted=rp['corrupted'], t_addr='%08X' % rp['t_addr'], bss_end='%08X' % rp['bss_end'],
                gp_item=rp['gp_item'], entry=rp['entry'], end_symbols=rp['end_symbols'],
                items=[item_json(it) for it in rp['items']],
                slots=[dict(address='%08X' % s['address'], value='%08X' % s['value'], kind=s['kind'],
                            target_symbol=s['target_symbol'], confidence=s['confidence'], item=s['item'], via=s['via'],
                            symbol=s.get('symbol'), addend=s.get('addend')) for s in rp['slots']],
                branch_asserts=rp['branch_asserts'], helpers=rp['helpers'], relabelled=rp['relabelled'],
                kept_absolute=rp['kept_absolute'],
                labels={n: {('%X' % int(o)): v for o, v in offs.items()} for n, offs in rp['labels'].items()},
                port=None if not p.get('port') else {k: ('%08X' % v if k in ('base', 'ram_end') else
                                                         {s: '%08X' % a for s, a in v.items()} if k == 'addresses' else v)
                                                      for k, v in p['port'].items() if k != 'rows'})


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build-dir', type=Path, help='hybrid build directory (default: LAST_BUILD.json)')
    a = ap.parse_args()
    pl = plan(a.build_dir or last_build_dir())
    j = plan_json(pl)
    print(json.dumps(dict(sections=len(j['sections']), pieces=len(j['pieces']), tails=len(j['tails']),
                          symbol_kinds=j['symbol_kinds'], interior=len(j['interior']), into_tail=len(j['into_tail']),
                          warnings=j['warnings'], problems=j['problems']), indent=2))
