#!/usr/bin/env python3
"""Count linked functions whose image bytes equal retail (slot = retail bytes, tail zero-padded).

    python tools/match_status.py [image-report.json] [--list]
Defaults to the image named by LAST_BUILD.json. Read-only.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load_exe(path):
    data = Path(path).read_bytes()
    return int.from_bytes(data[0x18:0x1C], 'little'), data[0x800:]


def status(report_path):
    r = json.loads(Path(report_path).read_text(encoding='utf-8'))
    base_i, img = load_exe(ROOT / r['image'])
    retail = 'bin/SCUS_94306R.EXE'
    base_r, ret = load_exe(ROOT / retail)
    same, diff = [], []
    for u in r['units']:
        for f in u['functions']:
            va = int(f['va'], 16)
            a = img[va - base_i: va - base_i + f['retail_slot']]
            b = ret[va - base_r: va - base_r + f['retail_slot']]
            (same if a == b else diff).append((f['va'], u['source']))
    return same, diff


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if args:
        path = args[0]
    else:
        last = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))
        path = ROOT / Path(last['executable']).parent / 'image-report.json'
    same, diff = status(path)
    print(f'identical {len(same)}/{len(same) + len(diff)}')
    # Placed data (jump tables, .tm2_<addr> sections) that differs from retail:
    # identical code with a wrong table still misbehaves at run time.
    for u in json.loads(Path(path).read_text(encoding='utf-8'))['units']:
        for d in u.get('data_changed') or []:
            print('DATA', d['address'], d['bytes'], u['source'])
    if '--list' in sys.argv:
        for va, src in diff:
            print('DIFF', va, src)


if __name__ == '__main__':
    main()
