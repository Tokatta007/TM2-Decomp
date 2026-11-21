#!/usr/bin/env python3
"""Static proofs for relocatable source-only links (tools/build_source_only.py --relocatable).

    py -3.14 tools/verify_reloc.py OUT                     # E0: zero shift; identity map, sha == expected
    py -3.14 tools/verify_reloc.py OUT --e0 E0_OUT         # E1: un-relocating OUT must give E0 exactly
    py -3.14 tools/verify_reloc.py OUT --e0 E0_OUT --identity-map --expect-fail   # negative control
    py -3.14 tools/verify_reloc.py --experiments --build-dir last   # E0, E1, E1b + negative controls

Checks (every one must pass):
  relocs      every relocation field of every linked input section (unit objects and generated retail
              pieces) holds exactly what its record says (R_MIPS_32/26/HI16/LO16/GPREL16, HI16/LO16 paired)
  map         addrmap.tsv tiles retail 800CDC54..801FAD48 and the candidate image without overlap;
              E0: it is the identity
  unrelocate  E1 only: copy every mapped candidate range to its retail position and rewrite every relocated
              field with c2r(address); the result must equal the E0 image byte for byte (header: pc0 maps
              to E0's pc0, t_size from the layout)
  sha         E0 only: the image is expected_source_only_sha256 (BUILD_MANIFEST.json), i.e. the fixed
              nonmatching source-only build (expected_source_only_sdk_lib_sha256 for a --sdk-from-lib link)
  slots       every config/US/data_relocs.tsv slot is an R_MIPS_32 relocation site, its candidate word is
              r2c(retail value), and pointer_slots.tsv resolves to it through symbols.tsv
  hi16        no `lui %hi(X)` is used by a load/store/addiu whose immediate is not %lo(X) (a numeric low
              half would keep the retail address when X moves)
  unrelocated no data word outside a relocation site holds an image/BSS address (retail coordinates)
  branches    each code_abs_refs.tsv pc-relative branch still reaches its retail target
  header      gp is the start of sdata, the BSS end symbol D_801FAD48 is the end of the layout, the entry
              `start` maps to retail pc0

All-C flavour (build_source_only.py --relocatable --all-c; reloc_plan.json has "all_c"): --e0 must be the
normal relocatable E0 of the same objects. `unrelocate` then compares everything except the retail slots of
the replaced C sections (pure-C versions of the hand-written asm), whose bytes cannot equal the asm; their
own relocations are still checked by `relocs`, and pointers into them from the rest of the image are
un-relocated by name (reloc_image.canonicalize). The excluded ranges are listed in reloc_verify.json.

Port variant (build_source_only.py --relocatable --port-variant; docs/SOURCE_BUILD.md "Port variant"): --e0 is
the normal relocatable E0 of the same objects. The functions the variant changes (and their jump tables) are
excluded like replaced all-C sections; a jump-table slot inside them must be an R_MIPS_32 relocation into a
changed function. Every other reference to a moved array un-relocates through the addrmap.tsv `port` rows to its
retail address, so the rest of the image must equal E0; `map` also checks that the port rows do not overlap.
Port code (.port_text/.port_rodata, and .port_data): every relocation of every object's part is checked by `relocs`
(e.g. the jump tables of port functions) and its code by `hi16`; it has no retail counterpart, so it is never
un-relocated (calls into it come only from changed functions, which are excluded).

The un-relocate check alone cannot see a pointer the build forgot to relocate (its bytes are unchanged), so
`slots` and `unrelocated` make static data strict; `--experiments` shows both negative controls failing.
Writes reloc_verify.json into OUT. Exit status 1 when a check fails (0 with --expect-fail).
"""
import argparse
import hashlib
from datetime import datetime, timezone
import json
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, read_layout
from semantic_elf import read_elf, read_relocs
import reloc_image
from reloc_image import Link, AddrMap, canonicalize, h8, R_MIPS_32, R_MIPS_HI16, R_MIPS_LO16
from gen_link import read_data_relocs, read_code_abs_refs, read_tsv

LIMIT = 30
MEM_OPS = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2A, 0x2B, 0x2E, 0x32, 0x3A, 0x08, 0x09}
WRITES_RT = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}


def hi16_holes(link):
    """`lui $r,%hi(X)` followed (before $r is rewritten) by a memory op / addiu on $r without a LO16."""
    holes, cache = [], {}
    for it in link.items + link.port_items:  # (port variant: the port's code too)
        if it['type'] not in ('c', 'port') or not it['code']:
            continue
        if it['obj'] not in cache:
            elf = read_elf(ROOT / it['obj'])
            cache[it['obj']] = (elf, read_relocs(elf))
        elf, rels = cache[it['obj']]
        sec = next(s for s in elf['sections'] if s['name'] == it['section'])
        rows = rels.get(sec['index'], [])
        d, at = sec['data'], {o: t for o, t, _ in rows}
        for o, t, sym in rows:
            if t != R_MIPS_HI16:
                continue
            rt = (struct.unpack_from('<I', d, o)[0] >> 16) & 31
            p, stop = o + 4, None
            while p + 4 <= len(d) and p < o + 4 * 24:
                w = struct.unpack_from('<I', d, p)[0]
                op, rs, rtt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
                if op in MEM_OPS and rs == rt and at.get(p) != R_MIPS_LO16:
                    holes.append('%s %s+0x%X: %08X uses %%hi(%s) in $%d without %%lo' % (
                        it['unit'], it['section'], p, w, sym['name'], rt))
                if (op in WRITES_RT and rtt == rt) or (op == 0 and ((w >> 11) & 31) == rt and (w & 0x3F) != 8):
                    break
                if stop is not None and p >= stop:
                    break
                if op == 2 or (op == 0 and (w & 0x3F) == 8):
                    stop = p + 4
                p += 4
    return holes


def check_map(amap, link, t_addr, bss_end):
    problems = []
    r_rows = sorted((r for r in amap.rows if r[1] > r[0]), key=lambda r: r[0])
    if r_rows[0][0] != t_addr or r_rows[-1][1] != bss_end:
        problems.append('retail rows cover %s..%s, not %s..%s' % (h8(r_rows[0][0]), h8(r_rows[-1][1]), h8(t_addr), h8(bss_end)))
    for a, b in zip(r_rows, r_rows[1:]):
        if a[1] != b[0]:
            problems.append('retail rows do not tile at %s / %s' % (h8(a[1]), h8(b[0])))
    c_rows = sorted((r for r in amap.rows if r[3] > r[2]), key=lambda r: r[2])
    if c_rows[0][2] != t_addr or c_rows[-1][3] != link.bss_end_c:
        problems.append('candidate rows cover %s..%s, not %s..%s' % (h8(c_rows[0][2]), h8(c_rows[-1][3]), h8(t_addr), h8(link.bss_end_c)))
    for a, b in zip(c_rows, c_rows[1:]):
        if a[3] != b[2]:
            problems.append('candidate rows do not tile at %s / %s' % (h8(a[3]), h8(b[2])))
    # port variant: the moved arrays' rows lie outside the tiling; they must not overlap each other or the image
    port = sorted(amap.port_rows, key=lambda r: r[2])
    for a, b in zip(port, port[1:]):
        if a[3] > b[2]:
            problems.append('port rows %s and %s overlap' % (a[5], b[5]))
    for r in port:
        if r[2] < link.bss_end_c:
            problems.append('port row %s (%s) lies inside the image' % (r[5], h8(r[2])))
    moved = sorted((r for r in port if r[1] > r[0]), key=lambda r: r[0])
    for a, b in zip(moved, moved[1:]):
        if a[1] > b[0]:
            problems.append('port rows %s and %s claim the same retail storage' % (a[5], b[5]))
    return problems


def verify(out, e0=None, identity_map=False):
    out = Path(out) if Path(out).is_absolute() else ROOT / out
    link = Link(out)
    layout = read_layout()
    t_addr, bss_end = layout[0]['start'], layout[-1]['end']
    amap = AddrMap.read(out / 'addrmap.tsv')
    if identity_map:
        amap = AddrMap.identity(t_addr, bss_end, amap.header)
    relocs, rproblems = link.relocations()
    canon, cproblems, covered = canonicalize(link, amap, relocs)
    stack = (link.plan.get('port_variant') or {}).get('stack')
    if stack:  # PC port request 89: crt0's stack word holds portStack's end; E0 has 2MBYTE's (retail) word
        r = amap.c2r(int(stack['crt0_word'], 16))
        canon = bytearray(canon)
        if r is None or struct.unpack_from('<I', canon, r - t_addr)[0] != int(stack['top'], 16) & 0x7FFFFFFF:
            cproblems.append('port variant: crt0\'s stack word %s does not hold portStack\'s end %s' % (
                stack['crt0_word'], stack['top']))
        else:
            struct.pack_into('<I', canon, r - t_addr, int(stack['retail_word'], 16))
    checks = {}
    checks['relocs'] = rproblems + [p for p in cproblems if p.startswith('relocation ') and 'the image holds' in p]
    checks['map'] = check_map(AddrMap.read(out / 'addrmap.tsv'), link, t_addr, bss_end)
    other = [p for p in cproblems if p not in checks['relocs']]
    replaced = reloc_image.replaced_ranges(link)
    if replaced and e0 is None:
        raise SystemExit('verify_reloc: %s is an all-C link; prove it against the normal relocatable E0 (--e0)' % out)

    if e0 is None:
        manifest = json.loads((ROOT / 'BUILD_MANIFEST.json').read_text(encoding='utf-8'))
        key = 'expected_source_only_sdk_lib_sha256' if link.plan.get('sdk_from_lib') else 'expected_source_only_sha256'
        expected = manifest.get(key)
        digest = hashlib.sha256(link.exe['raw']).hexdigest()
        checks['sha'] = [] if digest == expected else ['sha256 %s != %s %s' % (digest, key, expected)]
        if not AddrMap.read(out / 'addrmap.tsv').is_identity():
            checks['map'].append('E0: the address map is not the identity')
        checks['unrelocate'] = other + ([] if canon == link.exe['image'] else
                                        ['E0: the canonical image differs from the image itself'])
    else:
        e0 = Path(e0) if Path(e0).is_absolute() else ROOT / e0
        ref = reloc_image.exe_image(e0 / 'TM2_SOURCE.EXE')
        diffs = []
        if len(canon) != len(ref['image']):
            diffs.append('canonical image is 0x%X bytes, E0 0x%X' % (len(canon), len(ref['image'])))
        n = min(len(canon), len(ref['image']))
        skip = bytearray(n)  # all-C: the replaced retail slots
        for a, b in replaced:
            lo, hi = max(a - t_addr, 0), max(min(b - t_addr, n), 0)
            skip[lo:hi] = bytes([1]) * max(hi - lo, 0)
        i = 0
        while i < n:
            if skip[i]:
                i += 1
            elif canon[i] != ref['image'][i]:
                a = t_addr + (i & ~3)
                cov = covered.get(a)
                diffs.append('%s: un-relocated %s, E0 %s%s' % (
                    h8(a), canon[i & ~3:(i & ~3) + 4].hex(), ref['image'][i & ~3:(i & ~3) + 4].hex(),
                    ' (relocation %s -> %s)' % (reloc_image.RELOC_NAMES[cov[0]], h8(cov[1])) if cov else ''))
                i = (i & ~3) + 4
            else:
                i += 1
        if amap.c2r(link.entry_c) != ref['pc0']:
            diffs.append('entry %s maps to %s, E0 pc0 %s' % (h8(link.entry_c), h8(amap.c2r(link.entry_c) or 0), h8(ref['pc0'])))
        hdr_c, hdr_r = bytearray(link.exe['raw'][:0x800]), bytearray(ref['raw'][:0x800])
        for h in (hdr_c, hdr_r):
            h[0x10:0x14] = b'\0' * 4
            h[0x1C:0x20] = b'\0' * 4
        if stack:  # request 89: s_addr is portStack's end, s_size 0
            if struct.unpack_from('<2I', hdr_c, 0x30) != (int(stack['top'], 16), 0):
                diffs.append('header s_addr/s_size are not portStack\'s end')
            hdr_c[0x30:0x38] = hdr_r[0x30:0x38]
        if hdr_c != hdr_r:
            diffs.append('header differs outside pc0/t_size')
        checks['unrelocate'] = other + diffs

    # typed pointer slots
    slot_problems = []
    syms = {}
    for r in read_tsv(out / 'symbols.tsv'):
        syms.setdefault(r['name'], int(r['va'], 16))
    reloc_at = {p_c: (t, v) for p_c, t, v, it, name in relocs}
    pslots = {int(r['r_addr'], 16): r for r in read_tsv(out / 'pointer_slots.tsv')}
    replaced_c = [(it['c_start'], it['c_end']) for it in link.items if it.get('all_c')]
    for s in read_data_relocs():
        c = amap.r2c(s['address'])
        want = amap.r2c(s['value'])
        if c is not None and any(a <= s['address'] < b for a, b in replaced):
            # port variant: a jump table of a function the variant changed; its words point into that function's
            # new code, which has no retail address: the word must be a relocation into a changed function. A
            # section holding the tables of several functions keeps the other functions' words: those must be
            # ordinary slots (checked below)
            rec = reloc_at.get(c)
            if rec is not None and rec[0] == R_MIPS_32 and any(a <= rec[1] < b for a, b in replaced_c):
                continue
            if want is None or rec is None or rec[0] != R_MIPS_32 or link.word(c) != want:
                slot_problems.append('slot %s (%s, excluded jump table) is not a relocation into a changed function'
                                     % (h8(s['address']), s['kind']))
                continue
        if c is None or want is None:
            slot_problems.append('slot %s -> %s does not map' % (h8(s['address']), h8(s['value'])))
            continue
        rec = reloc_at.get(c)
        if rec is None or rec[0] != R_MIPS_32:
            slot_problems.append('slot %s (%s) is not an R_MIPS_32 relocation site' % (h8(s['address']), s['kind']))
        word = link.word(c)
        if word != want:
            slot_problems.append('slot %s holds %s, expected r2c(%s) = %s' % (h8(s['address']), h8(word), h8(s['value']), h8(want)))
        ps = pslots.get(s['address'])
        if ps is None or ps['target_symbol'] not in syms or (syms[ps['target_symbol']] + int(ps['addend'], 16)) & 0xFFFFFFFF != word:
            slot_problems.append('pointer_slots.tsv row for %s does not resolve to %s' % (h8(s['address']), h8(word)))
    checks['slots'] = slot_problems

    checks['hi16'] = hi16_holes(link)

    # unrelocated image addresses in data (retail coordinates of the canonical image)
    slot_addrs = {s['address'] for s in read_data_relocs()}
    unrel = []
    for r0, r1, c0, c1, kind, owner in amap.rows:
        if kind in ('bss', 'aux') or r1 <= r0 or kind == 'code_entry' or (kind == 'code_linear' and not owner.startswith('retail')):
            continue
        for a in range((r0 + 3) & ~3, r1 - 3, 4):
            if a in covered or a - t_addr + 4 > len(canon) or any(lo <= a < hi for lo, hi in replaced):
                continue
            w = struct.unpack_from('<I', canon, a - t_addr)[0]
            if t_addr <= w < bss_end:
                unrel.append('%s (%s) holds %s with no relocation%s' % (h8(a), owner, h8(w), ' [data_relocs slot]' if a in slot_addrs else ''))
    checks['unrelocated'] = unrel

    br = []
    for ref in read_code_abs_refs():
        c = amap.r2c(ref['address'])
        w = link.word(c)
        target = (c + 4 + (((w & 0xFFFF) ^ 0x8000) - 0x8000) * 4) & 0xFFFFFFFF
        if amap.c2r(target) != ref['value']:
            br.append('branch at %s (c %s) reaches %s (retail %s), expected %s' % (
                h8(ref['address']), h8(c), h8(target), h8(amap.c2r(target) or 0), h8(ref['value'])))
    checks['branches'] = br

    hdr = []
    first_sdata = next(s['start'] for s in layout if s['kind'] == 'sdata')
    if link.gp_c != amap.r2c(first_sdata):
        hdr.append('gp %s is not r2c(sdata start) %s' % (h8(link.gp_c), h8(amap.r2c(first_sdata) or 0)))
    for name, want in (('D_801FAD48', link.bss_end_c), ('D_80180EE4', amap.r2c(0x80180EE4)), ('D_801809D4', link.gp_c),
                       ('start', link.entry_c)):
        got = link.symbols.get(name, {}).get('address')
        if got != want:
            hdr.append('%s is %s, expected %s' % (name, h8(got or 0), h8(want or 0)))
    if amap.c2r(link.entry_c) != 0x8013878C:
        hdr.append('entry does not map to retail start 8013878C')
    checks['header'] = hdr

    ok = not any(checks.values())
    result = dict(schema='tm2-reloc-verify-v1', out=out.relative_to(ROOT).as_posix(),
                  e0=None if e0 is None else Path(e0).relative_to(ROOT).as_posix(), identity_map=identity_map,
                  ok=ok, relocations=len(relocs), slots=len(read_data_relocs()),
                  shift=link.plan.get('shift'), corrupted=link.plan.get('corrupted'),
                  all_c_excluded=['%s..%s' % (h8(a), h8(b)) for a, b in replaced] or None,
                  sha256=hashlib.sha256(link.exe['raw']).hexdigest(), t_size='0x%X' % link.exe['t_size'],
                  gp_c=h8(link.gp_c), bss_end_c=h8(link.bss_end_c), entry_c=h8(link.entry_c),
                  failed=[k for k, v in checks.items() if v],
                  checks={k: dict(ok=not v, count=len(v), first=v[:LIMIT]) for k, v in checks.items()})
    name = 'reloc_verify%s.json' % ('_identity_map' if identity_map else '')
    (out / name).write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def show(label, res, expect_fail=False):
    good = res['ok'] != expect_fail
    print('%s %s: %s%s' % ('OK  ' if good else 'FAIL', label, 'all checks pass' if res['ok'] else 'failed ' + ', '.join(res['failed']),
                           ' (expected to fail)' if expect_fail else ''))
    for k in res['failed']:
        for line in res['checks'][k]['first'][:3]:
            print('       %s: %s' % (k, line))
    return good


def experiments(build_dir, out_dir):
    """E0, E1 (800D1318:16), E1b (801625B8:16), and the negative controls."""
    out_dir.mkdir(parents=True, exist_ok=False)
    py = [sys.executable, '-B', str(ROOT / 'tools/build_source_only.py'), '--build-dir', build_dir, '--relocatable']
    runs = dict(E0=[], E1=['--shift', '800D1318:16'], E1b=['--shift', '801625B8:16'],
                E1_corrupt=['--shift', '800D1318:16', '--corrupt-slot', '80162858'])
    for name, extra in runs.items():
        subprocess.run(py + extra + ['--output', (out_dir / name).relative_to(ROOT).as_posix()], cwd=ROOT, check=True)
    results, good = {}, True
    for label, kw, expect_fail in (('E0', dict(out=out_dir / 'E0'), False),
                                   ('E1', dict(out=out_dir / 'E1', e0=out_dir / 'E0'), False),
                                   ('E1b', dict(out=out_dir / 'E1b', e0=out_dir / 'E0'), False),
                                   ('E1 with an identity map', dict(out=out_dir / 'E1', e0=out_dir / 'E0', identity_map=True), True),
                                   ('E1 with slot 80162858 left unrelocated', dict(out=out_dir / 'E1_corrupt', e0=out_dir / 'E0'), True)):
        res = verify(**kw)
        results[label] = dict(ok=res['ok'], expected_ok=not expect_fail, failed=res['failed'], sha256=res['sha256'],
                              t_size=res['t_size'], gp_c=res['gp_c'], bss_end_c=res['bss_end_c'], entry_c=res['entry_c'],
                              checks={k: v['count'] for k, v in res['checks'].items()},
                              first={k: res['checks'][k]['first'][:3] for k in res['failed']})
        good &= show(label, res, expect_fail)
    summary = dict(schema='tm2-reloc-experiments-v1', build_dir=build_dir, out=out_dir.relative_to(ROOT).as_posix(),
                   ok=good, results=results)
    (out_dir / 'experiments.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
    print('EXPERIMENTS %s -> %s' % ('OK' if good else 'FAILED', (out_dir / 'experiments.json').relative_to(ROOT).as_posix()))
    return good


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', nargs='?', help='relocatable link output directory')
    ap.add_argument('--e0', help='zero-shift relocatable link of the same objects (E1 proof)')
    ap.add_argument('--identity-map', action='store_true', help='negative control: ignore addrmap.tsv, use the identity')
    ap.add_argument('--expect-fail', action='store_true', help='exit 0 only if a check fails')
    ap.add_argument('--experiments', action='store_true', help='build and prove E0/E1/E1b plus the negative controls')
    ap.add_argument('--build-dir', default='last', help='with --experiments: hybrid build whose objects are linked')
    a = ap.parse_args()
    if a.experiments:
        import gen_link
        b = gen_link.last_build_dir() if a.build_dir == 'last' else (ROOT / a.build_dir).resolve()
        out = ROOT / 'work' / datetime.now(timezone.utc).strftime('reloc-%Y%m%d-%H%M%S-%f')
        sys.exit(0 if experiments(b.relative_to(ROOT).as_posix(), out) else 1)
    if not a.out:
        ap.error('OUT is required')
    res = verify(a.out, a.e0, a.identity_map)
    good = show(a.out, res, a.expect_fail)
    print('RELOC VERIFY %s' % ('OK' if res['ok'] else 'FAILED'))
    sys.exit(0 if good else 1)


if __name__ == '__main__':
    main()
