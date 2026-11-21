"""Resolve config/US/rollback_regions.tsv against a link (PC port request 48 stage 3, docs/ONLINE_ROLLBACK.md 12.4).

    py -3.14 tools/rollback_regions.py LINK [--regions TSV] [--out FILE]

LINK is a build_source_only.py / build.py output directory (report.json, symbols.tsv, reloc_plan.json, the ELF).
Writes LINK/rollback_regions.tsv (or --out): one row per region, sorted by address:

    address  size  name  kind  snapshot  hash  reason

`range` rows become the link's section ranges (game.static: from the item at retail 801809D4 to the end of the game's
BSS; port.bss / port.data: the port variant's sections), `symbol` rows the symbol's address and size (the tsv's size,
or "next": up to the next symbol of the link), `fixed` rows their lo-hi range, `field` rows one row per element of
an array symbol (request 54: the size column is SYMBOL:STRIDE:LO-HI, the bytes [LO, HI) of every STRIDE-byte element
of SYMBOL's extent up to the next symbol; written NAME[i] in the output), `ptr` rows one row per word of an array of
pointers (request 57: the size column is SYMBOL:WORDS:BYTES, WORDS a count or "next"; written NAME[i]:BYTES at the
word's address with size 0, as the row covers no static byte: at run time a non-zero word names BYTES at the address it
holds, which take the row's snapshot and hash over every static row; a reader that skips size-0 rows ignores them),
and the `dmd` row a run-time list (address and size 0: portDmdMutableList). Exit status 0 = every row resolved.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT  # noqa: E402

REGIONS = ROOT / 'config' / 'US' / 'rollback_regions.tsv'
GAME_STATIC_RETAIL = 0x801809D4
KINDS = {'range', 'symbol', 'symbol?', 'fixed', 'field', 'ptr', 'dmd'}


def load_rows(path=REGIONS):
    rows, problems = [], []
    header = None
    for n, line in enumerate(Path(path).read_text(encoding='utf-8').splitlines(), 1):
        if not line.strip() or line.startswith('#'):
            continue
        p = line.split('\t')
        if header is None:
            header = p
            continue
        if len(p) != len(header):
            problems.append('%s:%d: %d columns (want %d)' % (Path(path).name, n, len(p), len(header)))
            continue
        r = dict(zip(header, p))
        r['line'] = n
        if r['kind'] not in KINDS:
            problems.append('%s:%d: kind %r' % (Path(path).name, n, r['kind']))
        if r['snapshot'] not in ('yes', 'no') or r['hash'] not in ('yes', 'no'):
            problems.append('%s:%d: snapshot / hash must be yes or no' % (Path(path).name, n))
        if r['hash'] == 'yes' and r['snapshot'] == 'no':
            problems.append('%s:%d: %s hashed but not snapshotted' % (Path(path).name, n, r['name']))
        if not r['reason'].strip():
            problems.append('%s:%d: %s has no reason' % (Path(path).name, n, r['name']))
        rows.append(r)
    names = [r['name'] for r in rows]
    for d in sorted({x for x in names if names.count(x) > 1}):
        problems.append('%s listed twice' % d)
    return rows, problems


def link_symbols(out):
    """{name: address} of the link's symbols.tsv and the sorted address list (for "next")."""
    syms = {}
    for line in (Path(out) / 'symbols.tsv').read_text(encoding='ascii').splitlines():
        p = line.split('\t')
        if len(p) == 4 and p[0] != 'va' and not line.startswith('#'):
            try:
                syms.setdefault(p[2], int(p[0], 16))
            except ValueError:
                pass
    return syms, sorted(set(syms.values()))


def game_static(out):
    """(lo, hi) of the game's .sdata/.sbss/.bss in the link: the first plan item at or after retail 801809D4 to the
    end of the last item."""
    from reloc_image import Link
    link = Link(out)
    starts = [it['c_start'] for it in link.items if it['r_start'] >= GAME_STATIC_RETAIL and it['c_size']]
    return min(starts), link.bss_end_c


def resolve(out, rows):
    """[(address, size, row)] and problems."""
    import bisect
    out = Path(out)
    report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
    secs = {s['name']: s for s in (report.get('port_variant') or {}).get('sections', [])}
    syms, addrs = link_symbols(out)
    res, problems = [], []
    static = None
    for r in rows:
        k = r['kind']
        if k == 'range':
            if r['name'] == 'game.static':
                if static is None:
                    static = game_static(out)
                res.append((static[0], static[1] - static[0], r))
            elif r['name'] in ('port.bss', 'port.data'):
                s = secs.get('.port_bss' if r['name'] == 'port.bss' else '.port_data')
                if s is None:
                    if report.get('port_variant'):
                        problems.append('%s: no %s section' % (r['name'], r['name']))
                    continue
                res.append((int(s['address'], 16), s['size'], r))
            else:
                problems.append('unknown range %s' % r['name'])
        elif k in ('symbol', 'symbol?'):
            a = syms.get(r['name'])
            if a is None:
                if k == 'symbol':
                    problems.append('symbol %s not in the link' % r['name'])
                continue
            if r['size'] == 'next':
                i = bisect.bisect_right(addrs, a)
                if i >= len(addrs):
                    problems.append('%s: no next symbol' % r['name'])
                    continue
                size = addrs[i] - a
            else:
                size = int(r['size'], 0)
            if size <= 0:
                problems.append('%s: size %d' % (r['name'], size))
                continue
            res.append((a, size, r))
        elif k == 'fixed':
            try:
                lo, hi = (int(x, 16) for x in r['size'].split('-'))
            except ValueError:
                problems.append('%s: fixed range %r (want lo-hi)' % (r['name'], r['size']))
                continue
            res.append((lo, hi - lo, r))
        elif k == 'field':
            # request 54: SYMBOL:STRIDE:LO-HI, the bytes [LO, HI) of every STRIDE-byte element of SYMBOL (its extent up
            # to the next symbol), e.g. the scene objects' fields that each drawn view rewrites
            try:
                sname, stride, span = r['size'].split(':')
                stride = int(stride, 0)
                lo, hi = (int(x, 0) for x in span.split('-'))
            except ValueError:
                problems.append('%s: field %r (want SYMBOL:STRIDE:LO-HI)' % (r['name'], r['size']))
                continue
            a = syms.get(sname)
            if a is None or not 0 <= lo < hi <= stride:
                problems.append('%s: field %r does not resolve' % (r['name'], r['size']))
                continue
            # (request 59: a label inside the first element is a field's alias, not the array's end: weaponArcs' +4,
            # weaponArcsCountField; the extent runs to the first symbol at or past one element)
            i = bisect.bisect_left(addrs, a + stride)
            if i >= len(addrs):
                problems.append('%s: %s holds no whole element' % (r['name'], sname))
                continue
            for e in range((addrs[i] - a) // stride):
                res.append((a + e * stride + lo, hi - lo, dict(r, name='%s[%d]' % (r['name'], e))))
        elif k == 'ptr':
            # request 57: SYMBOL:WORDS:BYTES, the objects an array of pointers names (the cameras' scene objects); the
            # row covers no static byte (size 0 here: the pointee is known at run time only), the output gives BYTES
            try:
                sname, words, nbytes = r['size'].split(':')
                nbytes = int(nbytes, 0)
            except ValueError:
                problems.append('%s: ptr %r (want SYMBOL:WORDS:BYTES)' % (r['name'], r['size']))
                continue
            a = syms.get(sname)
            if a is None or nbytes <= 0:
                problems.append('%s: ptr %r does not resolve' % (r['name'], r['size']))
                continue
            if words == 'next':
                i = bisect.bisect_right(addrs, a)
                n = (addrs[i] - a) // 4 if i < len(addrs) else 0
            else:
                n = int(words, 0)
            if n <= 0:
                problems.append('%s: %s holds no pointer word' % (r['name'], sname))
                continue
            for e in range(n):
                res.append((a + 4 * e, 0, dict(r, name='%s[%d]:0x%X' % (r['name'], e, nbytes))))
        elif k == 'dmd':
            res.append((0, 0, r))
    res.sort(key=lambda x: (x[0], -x[1]))
    # symbol rows must not overlap each other (a range may contain them)
    sym = [x for x in res if x[2]['kind'] in ('symbol', 'symbol?', 'fixed', 'field')]
    for (a1, s1, r1), (a2, s2, r2) in zip(sym, sym[1:]):
        if a2 < a1 + s1:
            problems.append('%s (%08X+%X) overlaps %s (%08X)' % (r1['name'], a1, s1, r2['name'], a2))
    return res, problems


def covering(res, addr):
    """The most specific row holding addr (a symbol / fixed row before a range), or None."""
    best = None
    for a, s, r in res:
        if a <= addr < a + s:
            if best is None or (best[2]['kind'] == 'range' and r['kind'] != 'range'):
                best = (a, s, r)
    return best


def write(out, res, path):
    lines = ['# Generated by tools/rollback_regions.py from config/US/rollback_regions.tsv for this build: the regions a',
             '# rollback snapshot copies (snapshot yes) and the state hash covers (hash yes). A symbol row inside a range',
             '# overrides the range for its bytes ("no" = a hole). dmd: the list portDmdMutableList returns at run time.',
             '# ptr (size 0): address = a pointer word; NAME[i]:BYTES = the bytes it names at run time (when non-zero),',
             '# which take the row\'s snapshot / hash over every other row.',
             'address\tsize\tname\tkind\tsnapshot\thash\treason']
    for a, s, r in res:
        lines.append('%08X\t0x%X\t%s\t%s\t%s\t%s\t%s' % (a, s, r['name'], r['kind'].rstrip('?'),
                                                         r['snapshot'], r['hash'], r['reason']))
    tmp = Path(str(path) + '.tmp')
    tmp.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    tmp.replace(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', help='link directory')
    ap.add_argument('--regions', default=str(REGIONS), help='the hand-kept list (default config/US/rollback_regions.tsv)')
    ap.add_argument('--out', dest='dest', help='output file (default LINK/rollback_regions.tsv)')
    a = ap.parse_args()
    out = Path(a.out) if Path(a.out).is_absolute() else ROOT / a.out
    rows, problems = load_rows(a.regions)
    res, p2 = resolve(out, rows)
    problems += p2
    dest = Path(a.dest) if a.dest else out / 'rollback_regions.tsv'
    write(out, res, dest)
    print('rollback_regions: %d rows resolved -> %s' % (len(res), dest))
    for p in problems:
        print('  ' + p)
    sys.exit(1 if problems else 0)


if __name__ == '__main__':
    main()
