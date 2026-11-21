#!/usr/bin/env python3
"""For chosen functions, try every compile configuration and report the closest.

Configurations: {shipped, psyq3.6} x {compiler, assembler} x extern_gp {off,on} x fvolatile {off,on}.
For each target function prints MATCH configurations or the minimum differing-word count.
Read-only (no --apply).

    python tools/match_combo.py --target src/semantic/x.c:8012ABCD [...]
"""
import argparse
import itertools
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(source, compiler, delay, egp, fvol):
    cmd = [sys.executable, '-B', str(ROOT / 'tools/match_unit.py'), '--source', source, '--delay-slots', delay,
           '--extern-gp', egp, '--fvolatile', fvol]
    cmd += ['--compiler', compiler]
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT, timeout=600).stdout
    res = {}
    for line in out.splitlines():
        if line.startswith('MATCH'):
            res[line.split()[1]] = 0
        elif line.startswith('DIFF'):
            m = re.search(r'(\d+) words differ', line)
            size = re.search(r'size (\d+) vs slot (\d+)', line)
            res[line.split()[1]] = int(m.group(1)) if m else 999
            if size and int(size.group(1)) > int(size.group(2)):
                res[line.split()[1]] += 1000  # oversized: unusable in the fixed-address build
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--target', action='append', required=True)
    args = ap.parse_args()
    by_source = {}
    for t in args.target:
        src, va = t.split(':')
        by_source.setdefault(src, []).append(va.upper())
    summary = {}
    for src, vas in by_source.items():
        for compiler, delay, egp, fvol in itertools.product(('shipped', 'psyq3.6'), ('compiler', 'assembler'), ('off', 'on'), ('off', 'on')):
            key = f'{compiler}/{delay}/egp-{egp}/fvol-{fvol}'
            try:
                res = run(src, compiler, delay, egp, fvol)
            except Exception as exc:  # noqa: BLE001
                res = {}
            for va in vas:
                summary.setdefault(va, {})[key] = res.get(va, 999)
    for va, d in summary.items():
        best = min(d.values())
        keys = [k for k, v in d.items() if v == best]
        print(f'{va}: best={best} words ({len(keys)} configs) e.g. {keys[:3]}')
    out = ROOT / 'work/match/combo-last.json'
    out.write_text(json.dumps(summary, indent=1) + '\n')


if __name__ == '__main__':
    main()
