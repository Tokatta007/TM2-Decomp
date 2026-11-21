#!/usr/bin/env python3
"""Rewrite the README progress block from the last build.

    python tools/readme_progress.py [image-report.json]

Counts every function in config/US/ida_funcs.tsv plus config/US/extra_funcs.tsv
(code IDA missed between listed functions; deduped by VA): matching (image bytes equal
retail, see match_status.py), close (in C but still differing) and not yet in
C. Sizes are retail slot bytes, KB = 1000 bytes. Replaces the text between the
progress:start / progress:end markers in README.md.
"""
import csv
import datetime
import json
import re
import sys
from pathlib import Path

from match_status import ROOT, status

BAR = 20
PARTS = ' ▏▎▍▌▋▊▉█'


def bar(frac):
    eighths = round(frac * BAR * 8)
    full, rest = divmod(eighths, 8)
    s = '█' * full + (PARTS[rest] if rest else '')
    return s + '░' * (BAR - len(s))


def kb(n):
    return f'{n / 1000:,.1f} KB'


def load_funcs():
    """ida_funcs.tsv + extra_funcs.tsv rows, deduped by VA (ida_funcs wins), minus not_funcs.tsv."""
    funcs = {}
    for name in ('ida_funcs.tsv', 'extra_funcs.tsv'):
        path = ROOT / 'config/US' / name
        if not path.exists():
            continue
        with open(path, newline='') as f:
            for r in csv.DictReader(f, delimiter='\t'):
                funcs.setdefault(int(r['va'], 16), r)
    # not_funcs.tsv: entries of ida_funcs.tsv that are really data.
    path = ROOT / 'config/US/not_funcs.tsv'
    if path.exists():
        with open(path, newline='') as f:
            for r in csv.DictReader(f, delimiter='	'):
                funcs.pop(int(r['va'], 16), None)
    return list(funcs.values())


def main():
    if len(sys.argv) > 1:
        report = Path(sys.argv[1])
    else:
        last = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))
        report = ROOT / Path(last['executable']).parent / 'image-report.json'
    rep = json.loads(report.read_text(encoding='utf-8'))
    slot = {f['va']: f['retail_slot'] for u in rep['units'] for f in u['functions']}
    same, diff = status(report)
    funcs = load_funcs()
    n_all = len(funcs)
    b_all = sum(int(r['words']) * 4 for r in funcs)
    n_ok, b_ok = len(same), sum(slot[v] for v, _ in same)
    n_close, b_close = len(diff), sum(slot[v] for v, _ in diff)
    n_rest, b_rest = n_all - n_ok - n_close, b_all - b_ok - b_close

    units = sorted({Path(src).name for _, src in diff})
    close_what = 'In C, fits its slot, a few instructions still differ'
    if units:
        close_what += ' (' + ', '.join(f'`{u}`' for u in units) + ')'
    sha = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))['sha256']
    today = datetime.date.today().isoformat()

    block = f"""```
Functions  {bar(n_ok / n_all)}  {100 * n_ok / n_all:4.1f} %   {n_ok:,} / {n_all:,} matching
Code       {bar(b_ok / b_all)}  {100 * b_ok / b_all:4.1f} %   {kb(b_ok)} / {kb(b_all)} matching
```

| Status | Functions | Code | What it means |
|---|---:|---:|---|
| ✅ **Matching** | {n_ok:,} | {kb(b_ok)} | C compiles to exactly the retail bytes |
| 🟡 **Close** | {n_close:,} | {kb(b_close)} | {close_what} |
| ⬜ **Not yet in C** | {n_rest:,} | {kb(b_rest)} | Still retail machine code in the build |
"""
    readme = ROOT / 'README.md'
    text = readme.read_text(encoding='utf-8')
    text, n = re.subn(r'(<!-- progress:start -->\n).*?(<!-- progress:end -->)',
                      lambda m: m.group(1) + block + m.group(2), text, flags=re.S)
    if n != 1:
        sys.exit('README.md: progress markers not found')
    text = re.sub(r'Status as of \S+ \(hybrid build `[0-9a-f]+…`\)',
                  f'Status as of {today} (hybrid build `{sha[:8]}…`)', text)
    readme.write_text(text, encoding='utf-8', newline='\n')
    print(f'matching {n_ok}, close {n_close}, not yet in C {n_rest}')


if __name__ == '__main__':
    main()
