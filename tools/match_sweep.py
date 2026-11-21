#!/usr/bin/env python3
"""Byte-match every linked unit under each compiler configuration.

Configurations: shipped cc1 / PsyQ 3.6 CC1PSX x compiler / assembler delay slots.
Writes work/match/sweep-<timestamp>.json with the matching function set per
configuration, the current manifest configuration, and the best configuration
(most matches, never dropping a function that matches under the current one).
Read-only for sources, template and manifest.

    python tools/match_sweep.py [--only unit_stem ...]
"""
import argparse
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIGS = [(None, 'compiler'), (None, 'assembler'), ('psyq3.6', 'compiler'), ('psyq3.6', 'assembler')]


def matches(source, compiler, delay, egp=None, fvol=None):
    """Matching functions for one configuration (None = manifest default)."""
    cmd = [sys.executable, '-B', str(ROOT / 'tools/match_unit.py'), '--source', source, '--delay-slots', delay]
    if fvol is not None:
        cmd += ['--fvolatile', 'on' if fvol else 'off']
    if egp is not None:
        cmd += ['--extern-gp', 'on' if egp else 'off']
    cmd += ['--compiler', compiler or 'shipped']
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT, timeout=600).stdout
    lines = [l for l in out.splitlines() if l.startswith(('MATCH', 'DIFF'))]
    import re
    oversized = [l.split()[1] for l in lines if (m := re.search(r'size (\d+) vs slot (\d+)', l)) and int(m.group(1)) > int(m.group(2))]
    if oversized:
        # The fixed-address build would skip the whole unit: never a valid choice.
        raise RuntimeError('oversized ' + ','.join(oversized))
    return sorted(l.split()[1] for l in lines if l.startswith('MATCH')), len(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--combo', action='store_true', help='all 16 combinations of compiler x delay x extern_gp x fvolatile; skips fully matching units')
    ap.add_argument('--fvolatile', action='store_true', help='compare each unit current configuration with -fvolatile off/on')
    ap.add_argument('--extern-gp', action='store_true', help='compare each unit current configuration with extern_gp off/on')
    args = ap.parse_args()
    manifest = json.loads((ROOT / 'BUILD_MANIFEST.json').read_text(encoding='utf-8'))
    report = json.loads(next(iter(sorted((ROOT / 'work').glob('build-*/psx/image-report.json'), key=lambda p: p.stat().st_mtime)), None).read_text()) \
        if False else None
    last = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))
    image_report = json.loads((ROOT / Path(last['executable']).parent / 'image-report.json').read_text(encoding='utf-8'))
    linked = {u['source'] for u in image_report['units']}
    results = {}
    for unit in manifest['units']:
        src = unit['source']
        if src not in linked or (args.only and Path(src).stem not in args.only):
            continue
        current = (unit.get('compiler'), unit['delay_slots'])
        cur_egp, cur_fvol = bool(unit.get('extern_gp')), bool(unit.get('fvolatile'))
        row = dict(current=list(current) + [cur_egp, cur_fvol], configs={})
        if args.combo:
            import itertools
            m0, n0 = matches(src, current[0], current[1], cur_egp, cur_fvol)
            if len(m0) == n0:
                continue  # already fully matching
            configs = [(c, d, e, f) for c, d in CONFIGS for e, f in itertools.product((False, True), (False, True))]
        elif args.fvolatile:
            configs = [(current[0], current[1], None, False), (current[0], current[1], None, True)]
        elif args.extern_gp:
            configs = [(current[0], current[1], False, None), (current[0], current[1], True, None)]
        else:
            configs = [(c, d, None, None) for c, d in CONFIGS]
        for compiler, delay, egp, fvol in configs:
            key = f'{compiler or "shipped"}/{delay}' + ('/egp' if egp else '') + ('/fvol' if fvol else '')
            try:
                m, n = matches(src, compiler, delay, egp, fvol)
            except Exception as exc:  # noqa: BLE001
                m, n = [], 0
                row.setdefault('errors', []).append(f'{key}: {exc}')
                row.setdefault('invalid', []).append(key)
            row['configs'][key] = m
            row['functions'] = max(row.get('functions', 0), n)
        cur_key = f'{current[0] or "shipped"}/{current[1]}' + ('/egp' if (args.combo and cur_egp) else '') + ('/fvol' if (args.combo and cur_fvol) else '')
        if cur_key not in row['configs']:
            row['configs'][cur_key] = matches(src, current[0], current[1], cur_egp, cur_fvol)[0]
        cur = set(row['configs'][cur_key])
        ok = {k: v for k, v in row['configs'].items() if cur <= set(v) and k not in row.get('invalid', [])}
        best = max(ok, key=lambda k: (len(ok[k]), k == cur_key))
        row['best'] = best
        row['gain'] = len(ok[best]) - len(cur)
        results[src] = row
        print(f"{Path(src).stem}: n={row['functions']} current={len(cur)} best={best} (+{row['gain']})", flush=True)
    out = ROOT / 'work/match' / ('sweep-' + datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S') + '.json')
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(results, indent=1) + '\n', encoding='utf-8')
    total = sum(len(r['configs'][f"{r['current'][0] or 'shipped'}/{r['current'][1]}"]) for r in results.values())
    print(f'units {len(results)}; matching now {total}; available gain {sum(r["gain"] for r in results.values())} -> {out.relative_to(ROOT)}')


if __name__ == '__main__':
    main()
