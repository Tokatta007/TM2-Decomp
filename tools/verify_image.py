#!/usr/bin/env python3
"""Verify an M2 (nonmatching) source-only image against the hybrid and retail.

    py -3.14 tools/verify_image.py work/source-<timestamp> [--hybrid EXE] [--json OUT]

Byte comparison (whole PS-X EXE files, header included):
  (a) source-only vs the hybrid EXE of the build whose objects were linked
      (link_plan.json build_dir; LAST_BUILD.json is checked to be the same).
      Expected: identical except the zero-filled tails of C functions that are
      shorter than their retail slot (the hybrid keeps retail's tail bytes).
  (b) source-only vs retail (bin/SCUS_94306R.EXE). Expected: differences only
      inside the slots of non-identical ("close") functions, i.e. functions
      whose hybrid slot bytes differ from retail, and inside placed C data
      sections (jump tables, tables) whose bytes differ from retail.
Every differing byte run is attributed to a header field, a function slot
(VA + name + offset), a placed C section, or the nearest data label.

Map checks (TM2_SOURCE.map): every selection function symbol at its VA, every
`.tm2_<X>` input section at X, every `.text_<VA>` at VA, the output sections
tile [t_addr, bss_end) except zero-filled slot tails, no output section
crosses a layout.tsv boundary, retail pieces come from their own segment's
object, _gp = 801809D4, no non-empty code/data input section was discarded.

Exit status 1 when anything unexpected is found.
"""
import argparse
import bisect
import csv
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, CONFIG, read_layout, load_functions
from symbol_names import load_renames

GP = 0x801809D4
HEADER_FIELDS = [(0x00, 8, 'magic'), (0x08, 8, 'reserved_08'), (0x10, 4, 'pc0'), (0x14, 4, 'gp0'),
                 (0x18, 4, 't_addr'), (0x1C, 4, 't_size'), (0x20, 4, 'd_addr'), (0x24, 4, 'd_size'),
                 (0x28, 4, 'b_addr'), (0x2C, 4, 'b_size'), (0x30, 4, 's_addr'), (0x34, 4, 's_size'),
                 (0x38, 0x14, 'reserved_38'), (0x4C, 0x800 - 0x4C, 'region/marker')]
SECTION_ADDR = re.compile(r'^\.(?:text|tm2(?:_string|_init)?)_([0-9A-Fa-f]{8})$')
DISCARD_OK = re.compile(r'^\.(?:reginfo|pdr|MIPS\.abiflags|MIPS\.options|gnu\.attributes|comment|mdebug.*|debug.*|line|note.*)$')


def runs(a, b, base=0):
    """Contiguous differing byte runs of two equal-length buffers as (start, end) offsets."""
    if len(a) != len(b):
        raise SystemExit('verify_image: sizes differ (%d vs %d)' % (len(a), len(b)))
    out, i, n = [], 0, len(a)
    while i < n:
        if a[i] != b[i]:
            j = i
            while j < n and a[j] != b[j]:
                j += 1
            out.append((base + i, base + j))
            i = j
        else:
            i += 1
    return out


class Attributor:
    def __init__(self, plan, t_addr):
        self.t_addr = t_addr
        self.funcs = load_functions()
        self.fstarts = sorted(self.funcs)
        self.secs = sorted(((int(s['base'], 16), int(s['base'], 16) + s['size'], s) for s in plan['sections']),
                           key=lambda x: x[0])
        self.sstarts = [s[0] for s in self.secs]
        labels = {}
        for r in csv.DictReader((CONFIG / 'ida_names.tsv').open(), delimiter='\t'):
            if r['code'] != '1':
                labels[int(r['va'], 16)] = r['name']
        for r in load_renames():  # config/US/names.tsv: the readable name wins over the IDA name
            if r['va'] in labels:
                labels[r['va']] = r['new']
        self.labels = sorted(labels.items())
        self.lstarts = [a for a, _ in self.labels]
        self.layout = read_layout()

    def addr(self, off):
        return self.t_addr + off - 0x800

    def function(self, addr):
        i = bisect.bisect_right(self.fstarts, addr) - 1
        if i >= 0 and addr < self.funcs[self.fstarts[i]][0]:
            return self.fstarts[i]
        return None

    def data_section(self, addr):
        i = bisect.bisect_right(self.sstarts, addr) - 1
        for lo, hi, s in self.secs[max(i - 3, 0):i + 1]:  # sections do not overlap (except one BSS alias)
            if lo <= addr < hi and not s['code']:
                return s
        return None

    def segment(self, addr):
        return next((s['name'] for s in self.layout if s['start'] <= addr < s['end']), '?')

    def describe(self, off):
        if off < 0x800:
            f = next(f for f in HEADER_FIELDS if f[0] <= off < f[0] + f[1])
            return 'header %s+0x%X' % (f[2], off - f[0])
        a = self.addr(off)
        va = self.function(a)
        if va is not None:
            return 'function %08X %s+0x%X' % (va, self.funcs[va][1], a - va)
        s = self.data_section(a)
        if s:
            return 'data %s (%s)+0x%X' % (s['section'], s['unit'], a - int(s['base'], 16))
        i = bisect.bisect_right(self.lstarts, a) - 1
        label = '%s+0x%X' % (self.labels[i][1], a - self.labels[i][0]) if i >= 0 else '?'
        return 'data %s [%s]' % (label, self.segment(a))


def parse_map(path):
    lines = Path(path).read_text(encoding='utf-8', errors='replace').splitlines()
    k = lines.index('Linker script and memory map')
    discarded = []
    pending = None
    for line in lines[:k]:
        m = re.match(r'^ (\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$', line)
        if m:
            discarded.append((m.group(1), int(m.group(3), 16), m.group(4)))
            continue
        m = re.match(r'^ (\S+)$', line)
        if m:
            pending = m.group(1)
            continue
        m = re.match(r'^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$', line)
        if m and pending:
            discarded.append((pending, int(m.group(2), 16), m.group(3)))
            pending = None
    outs, inputs, symbols, assigns = [], [], {}, {}
    cur_out, pend_out, pend_in = None, None, None
    for line in lines[k + 1:]:
        if pend_out:
            m = re.match(r'^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)', line)
            if m:
                cur_out = dict(name=pend_out, address=int(m.group(1), 16), size=int(m.group(2), 16))
                outs.append(cur_out)
            pend_out = None
            continue
        m = re.match(r'^(\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)', line)
        if m:
            cur_out = dict(name=m.group(1), address=int(m.group(2), 16), size=int(m.group(3), 16))
            outs.append(cur_out)
            continue
        m = re.match(r'^(\.\S+)$', line)
        if m:
            pend_out = m.group(1)
            continue
        if pend_in:
            m = re.match(r'^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$', line)
            if m:
                inputs.append(dict(out=cur_out and cur_out['name'], name=pend_in, address=int(m.group(1), 16),
                                   size=int(m.group(2), 16), file=m.group(3)))
            pend_in = None
            continue
        m = re.match(r'^ (\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$', line)
        if m:
            inputs.append(dict(out=cur_out and cur_out['name'], name=m.group(1), address=int(m.group(2), 16),
                               size=int(m.group(3), 16), file=m.group(4)))
            continue
        m = re.match(r'^ (\.\S+)$', line)
        if m:
            pend_in = m.group(1)
            continue
        m = re.match(r'^\s+0x([0-9a-f]+)\s+([A-Za-z_.$][\w.$]*)\s*$', line)
        if m:
            symbols.setdefault(m.group(2), []).append(int(m.group(1), 16))
            continue
        m = re.match(r'^\s+0x([0-9a-f]+)\s+([A-Za-z_.$][\w.$]*) = ', line)
        if m:
            assigns[m.group(2)] = int(m.group(1), 16)
    return dict(outputs=outs, inputs=inputs, symbols=symbols, assigns=assigns, discarded=discarded)


def check_map(mp, plan, selection, layout):
    errors, notes = [], []
    syms = mp['symbols']
    dropped = (plan.get('sdk_lib') or {}).get('dropped_functions', {})
    lib_code = [(int(s['base'], 16), int(s['slot'][1], 16)) for s in plan['sections'] if s.get('lib') and s['code']]
    for va, f in selection['functions'].items():
        if va in dropped:  # --sdk-from-lib: a PsyQ module holds it; the C name, if used, is an alias
            got = mp['assigns'].get(f['symbol'])
            if got not in (None, int(va, 16)) or not any(lo <= int(va, 16) < hi for lo, hi in lib_code):
                errors.append('function %s %s (from the .LIB) at %s in the map' % (va, f['symbol'], got))
            continue
        got = syms.get(f['symbol'])
        if got != [int(va, 16)]:
            errors.append('function %s %s at %s in the map' % (va, f['symbol'], got))
    placed = 0
    for i in mp['inputs']:
        m = SECTION_ADDR.match(i['name'])
        if m and i['size']:
            placed += 1
            if i['address'] != int(m.group(1), 16):
                errors.append('%s from %s at %08X' % (i['name'], i['file'], i['address']))
    notes.append('%d .text_<VA>/.tm2_<X> input sections at their named address' % placed)
    plain = [s for s in plan['sections'] if not SECTION_ADDR.match(s['section'])]
    for s in plain:
        hits = [i for i in mp['inputs'] if i['name'] == s['section'] and i['file'] == s['obj']]
        if len(hits) != 1 or hits[0]['address'] != int(s['base'], 16):
            errors.append('plain section %s of %s not at %s' % (s['section'], s['unit'], s['base']))
        for name, addr in s['globals'].items():
            if syms.get(name) != [int(addr, 16)]:
                errors.append('symbol %s of %s at %s, expected %s' % (name, s['unit'], syms.get(name), addr))
    notes.append('%d plain-named sections (.text/.bss) at their first symbol, every contained symbol checked' % len(plain))
    if mp['assigns'].get('_gp') != GP or syms.get('_gp', [GP]) != [GP]:
        errors.append('_gp is %s' % mp['assigns'].get('_gp'))
    # Tiling: output sections (minus NOBITS aliases) + zero tails cover [first, last) of the layout exactly.
    alias = {s['out'] for s in plan['sections'] if s.get('alias_of')}
    spans = [(o['address'], o['address'] + o['size'], o['name']) for o in mp['outputs']
             if o['size'] and o['name'] not in alias and (o['name'].startswith('.c.') or o['name'].startswith('.r.'))]
    spans += [(int(t['start'], 16), int(t['end'], 16), 'tail ' + t['function']) for t in plan['tails']]
    spans.sort()
    cur = layout[0]['start']
    for lo, hi, name in spans:
        if lo != cur:
            errors.append('tiling: %s at %08X, expected %08X' % (name, lo, cur))
        cur = max(cur, hi)
    if cur != layout[-1]['end']:
        errors.append('tiling ends at %08X, not %08X' % (cur, layout[-1]['end']))
    bounds = [s['start'] for s in layout[1:]]
    for lo, hi, name in spans:
        for b in bounds:
            if lo < b < hi:
                errors.append('%s [%08X,%08X) crosses the layout boundary %08X' % (name, lo, hi, b))
    for i in mp['inputs']:
        if i['out'] and i['out'].startswith('.r.') and i['size']:
            seg = next(s for s in layout if s['start'] <= i['address'] < s['end'])
            if not i['file'].endswith('retail_%s.o' % seg['name']):
                errors.append('retail piece %08X from %s, in segment %s' % (i['address'], i['file'], seg['name']))
    notes.append('%d output spans tile %08X-%08X; no span crosses %s' % (
        len(spans), layout[0]['start'], layout[-1]['end'], ' '.join('%08X' % b for b in bounds)))
    for name, size, f in mp['discarded']:
        if size and not DISCARD_OK.match(name):
            errors.append('discarded non-empty %s (%d bytes) from %s' % (name, size, f))
    return errors, notes


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('source_dir', type=Path)
    ap.add_argument('--hybrid', type=Path, help='hybrid EXE (default: <build_dir>/psx/TM2_MAXIMAL_HYBRID.EXE)')
    ap.add_argument('--retail', type=Path, default=ROOT / 'bin/SCUS_94306R.EXE')
    ap.add_argument('--json', type=Path, help='write the full result here (default: <source_dir>/verify.json)')
    a = ap.parse_args()
    d = a.source_dir if a.source_dir.is_absolute() else ROOT / a.source_dir
    plan = json.loads((d / 'link_plan.json').read_text(encoding='utf-8'))
    build_dir = ROOT / plan['build_dir']
    selection = json.loads((build_dir / 'selection.json').read_text(encoding='utf-8'))
    hybrid_path = a.hybrid or build_dir / 'psx/TM2_MAXIMAL_HYBRID.EXE'
    last = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))
    unexpected, notes = [], []
    if (ROOT / last['executable']).resolve() != hybrid_path.resolve():
        notes.append('note: hybrid %s is not LAST_BUILD.json (%s)' % (hybrid_path, last['executable']))
    src, hyb, ret = (Path(p).read_bytes() for p in (d / 'TM2_SOURCE.EXE', hybrid_path, a.retail))
    t_addr = struct.unpack_from('<I', ret, 0x18)[0]
    at = Attributor(plan, t_addr)
    layout = read_layout()

    def addr_off(addr):
        return addr - t_addr + 0x800

    tails = [(addr_off(int(t['start'], 16)), addr_off(int(t['end'], 16)), t) for t in plan['tails']]
    # Close functions: hybrid slot bytes differ from retail. Changed data: placed C data bytes differ from retail.
    # --sdk-from-lib: PsyQ module sections replace the C sections of their functions. They must equal retail;
    # against the hybrid they may differ only where the C function was close (the module gives retail bytes).
    dropped = (plan.get('sdk_lib') or {}).get('dropped_functions', {})
    lib_spans = [(addr_off(int(s['base'], 16)),
                  min(addr_off(int(s['slot'][1], 16) if s.get('slot') else int(s['base'], 16) + s['size']), len(ret)), s)
                 for s in plan['sections'] if s.get('lib')]
    close, close_replaced = {}, {}
    for va, f in selection['functions'].items():
        lo, hi = addr_off(int(va, 16)), addr_off(at.funcs[int(va, 16)][0])
        if hyb[lo:hi] != ret[lo:hi]:
            (close_replaced if va in dropped else close)[int(va, 16)] = f['symbol']
    changed_data = []
    for s in plan['sections']:
        b = int(s['base'], 16)
        lo, hi = addr_off(b), min(addr_off(b + s['size']), len(ret))
        if not s['code'] and not s.get('lib') and lo < len(ret) and src[lo:hi] != ret[lo:hi]:
            changed_data.append((lo, hi, s))

    def inside(r, spans):
        return next((x for x in spans if x[0] <= r[0] and r[1] <= x[1]), None)

    # (a) vs hybrid
    a_runs = runs(src, hyb)
    a_rows, a_tail_bytes, a_lib_bytes = [], 0, 0
    for r in a_runs:
        t = inside(r, tails)
        lib = inside(r, lib_spans) if not t and src[r[0]:r[1]] == ret[r[0]:r[1]] else None
        row = dict(start=r[0], end=r[1], bytes=r[1] - r[0], where=at.describe(r[0]),
                   cls='zero_tail' if t else 'sdk_lib_retail' if lib else 'UNEXPECTED')
        if t:
            a_tail_bytes += row['bytes']
        elif lib:
            a_lib_bytes += row['bytes']
        else:
            unexpected.append('vs hybrid: %d bytes at file 0x%X (%s)' % (row['bytes'], r[0], row['where']))
        a_rows.append(row)
    tails_differing = sorted({inside(r, tails)[2]['function'] for r in a_runs if inside(r, tails)})

    # (b) vs retail
    close_spans = [(addr_off(va), addr_off(at.funcs[va][0]), va) for va in close]
    b_rows, b_counts = [], {}
    for r in runs(src, ret):
        c = inside(r, close_spans)
        dsec = inside(r, changed_data)
        t = inside(r, tails)
        if any(lo < r[1] and r[0] < hi for lo, hi, _ in lib_spans):
            cls = 'UNEXPECTED'  # a PsyQ module section must equal retail
        elif c:
            cls = 'close_function'
        elif dsec:
            cls = 'placed_data'
        elif t:
            cls = 'zero_tail_of_identical_function'
        else:
            cls = 'UNEXPECTED'
        if cls in ('UNEXPECTED', 'zero_tail_of_identical_function'):
            unexpected.append('vs retail: %s %d bytes at file 0x%X (%s)' % (cls, r[1] - r[0], r[0], at.describe(r[0])))
        b_counts[cls] = b_counts.get(cls, 0) + r[1] - r[0]
        b_rows.append(dict(start=r[0], end=r[1], bytes=r[1] - r[0], where=at.describe(r[0]), cls=cls))
    close_seen = sorted({at.function(at.addr(r['start'])) for r in b_rows if r['cls'] == 'close_function'})
    data_seen = sorted({(inside((r['start'], r['end']), changed_data)[2]['section'],
                         inside((r['start'], r['end']), changed_data)[2]['unit']) for r in b_rows if r['cls'] == 'placed_data'})

    mp = parse_map(d / 'TM2_SOURCE.map')
    map_errors, map_notes = check_map(mp, plan, selection, layout)
    unexpected += ['map: ' + e for e in map_errors]
    if src[:0x800] != ret[:0x800]:
        unexpected.append('header differs from retail')

    result = dict(source=str(d.relative_to(ROOT).as_posix()), hybrid=str(hybrid_path), retail=str(a.retail),
                  vs_hybrid=dict(runs=len(a_runs), bytes=sum(r['bytes'] for r in a_rows), zero_tail_bytes=a_tail_bytes,
                                 tails_planned=len(plan['tails']), tails_differing=tails_differing,
                                 tails_identical=[t['function'] for t in plan['tails'] if t['function'] not in tails_differing],
                                 sdk_lib_retail_bytes=a_lib_bytes,
                                 sdk_lib_close_functions_replaced=['%08X %s' % kv for kv in sorted(close_replaced.items())],
                                 detail=[dict(r, start='0x%X' % r['start'], end='0x%X' % r['end']) for r in a_rows]),
                  vs_retail=dict(runs=len(b_rows), bytes_by_class=b_counts, close_functions_in_hybrid=len(close),
                                 close_functions_differing=['%08X %s' % (v, close[v]) for v in close_seen],
                                 placed_data_differing=['%s (%s)' % x for x in data_seen],
                                 detail=[dict(r, start='0x%X' % r['start'], end='0x%X' % r['end']) for r in b_rows]),
                  map=dict(errors=map_errors, notes=map_notes), problems=plan['problems'], unexpected=unexpected)
    out = a.json or d / 'verify.json'
    out.write_text(json.dumps(result, indent=1) + '\n', encoding='utf-8')

    print('source-only %s' % result['source'])
    print('(a) vs hybrid: %d bytes in %d runs; %d in zero-filled slot tails (%d of %d tails differ: %s)' % (
        result['vs_hybrid']['bytes'], len(a_runs), a_tail_bytes, len(tails_differing), len(plan['tails']),
        ' '.join(tails_differing)))
    if lib_spans:
        print('    --sdk-from-lib: %d PsyQ module sections; %d bytes differ from the hybrid where the module gives retail '
              'bytes (close C functions replaced: %s)' % (len(lib_spans), a_lib_bytes,
                                                         ' '.join('%08X' % v for v in sorted(close_replaced)) or 'none'))
    print('(b) vs retail: %d runs; bytes by class %s' % (len(b_rows), json.dumps(b_counts)))
    print('    close functions (hybrid != retail): %d; differing in the source-only image: %d' % (len(close), len(close_seen)))
    for v in close_seen:
        print('      %08X %s' % (v, close[v]))
    print('    placed C data differing from retail: %d' % len(data_seen))
    for x in data_seen:
        print('      %s (%s)' % x)
    for n in map_notes:
        print('map: ' + n)
    for p in plan['problems']:
        print('problem: ' + p)
    for u in unexpected:
        print('UNEXPECTED: ' + u)
    print('result written to %s' % out.relative_to(ROOT).as_posix() if out.is_relative_to(ROOT) else out)
    if unexpected:
        print('VERIFY FAILED: %d unexpected findings' % len(unexpected))
        sys.exit(1)
    print('VERIFY OK: only the expected differences')


if __name__ == '__main__':
    main()
