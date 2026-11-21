#!/usr/bin/env python3
"""Try alternative source texts for one function and report which byte-match.

    python tools/match_try.py --source src/semantic/x.c --va 8012CBAC --old 'OLD TEXT' --new 'VARIANT 1' --new 'VARIANT 2'

Each variant replaces the unique OLD TEXT in the source, the unit is checked with
match_unit.py (unit policy, no --apply), and the original file is restored.
Prints MATCH/DIFF (+ differing word count) per variant; with --keep the first
matching variant is left in place.
"""
import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--source', required=True)
    ap.add_argument('--va', required=True)
    ap.add_argument('--old', required=True)
    ap.add_argument('--new', action='append', required=True)
    ap.add_argument('--keep', action='store_true')
    ap.add_argument('--unit-arg', action='append', default=[], help='extra match_unit.py argument, e.g. --unit-arg=--extern-gp --unit-arg=on')
    args = ap.parse_args()
    path = ROOT / args.source
    original = path.read_text(encoding='utf-8')
    if original.count(args.old) != 1:
        raise SystemExit('OLD TEXT must occur exactly once')
    kept = None
    try:
        for i, variant in enumerate(args.new):
            path.write_text(original.replace(args.old, variant), encoding='utf-8')
            out = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/match_unit.py'), '--source', args.source, *args.unit_arg],
                                 capture_output=True, text=True, cwd=ROOT).stdout
            line = next((l for l in out.splitlines() if l[7:15].upper() == args.va.upper()), None)
            print(f'[{i}] {line or out.strip().splitlines()[-1] if out.strip() else "compile failed"}')
            if line and line.startswith('MATCH') and args.keep:
                kept = variant
                break
    finally:
        path.write_text(original.replace(args.old, kept) if kept else original, encoding='utf-8')
    if kept:
        print('kept variant:', kept)


if __name__ == '__main__':
    main()
