#!/usr/bin/env python3
"""Update notes (Markdown or text) to the decomp's new symbol and file names.

    py -3 port_rename_notes.py --dry-run docs/*.md notes.txt      # list what would change
    py -3 port_rename_notes.py docs/ notes.txt                    # rewrite in place
    py -3 port_rename_notes.py --annotate docs/                   # new name, old one kept in brackets
    py -3 port_rename_notes.py --names X/names.tsv --files X/file_renames.tsv notes/

Stand-alone: it needs only Python 3.8+ and the two tables of the TM2 decomp repository,
config/US/names.tsv (va, old, new, evidence) and config/US/file_renames.tsv (old_path, new_path,
reason). By default they are looked up next to this script (../config/US/); pass --names / --files
to use copies elsewhere. A directory argument means every *.md and *.txt file under it (--ext
changes the list).

What is rewritten (whole words, case-sensitive):
  - address names of a renamed symbol: sub_8011BE10, func_8011BE10, D_8016475C, and Ghidra's
    FUN_8011BE10 / DAT_8016475C (hex in either case) -> the new name (carDestroyed, sessionSettings);
  - the old (IDA) names of names.tsv: update_bullets -> bulletsUpdate, screenCheckMidGameOptions ->
    sessionRun, ...;
  - old file paths and names: src/semantic/game_code10.c (also with backslashes and inside longer
    paths), semantic/game_code10.c, game_code10.c, and the bare stem game_code10 -> screen_pause_menu.
    func_<VA>.c files are matched only as file names, so a func_<VA> symbol in the notes still gets
    the symbol name, and reference/m2c/func_<VA>.c paths are left alone.
Addresses themselves (8011BE10, 0x8011BE10) are never touched: the VA is the stable key, keep it in
the notes next to the name. Names that are not in the tables are left as they are. Every name is
replaced in one pass, so a replacement is never renamed again.

--annotate writes `new (old)` instead of `new` for symbols, so text searches for the old name still
hit. Text already in the decomp docs' style, `sub_8011BE10` (carDestroyed), becomes `carDestroyed`
(without --annotate) or stays as it is (with it). Files are read and written as bytes (UTF-8), so
line endings stay as they were. Files that are not UTF-8 are skipped with a warning.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DEFAULT_NAMES = HERE.parent / 'config/US/names.tsv'
DEFAULT_FILES = HERE.parent / 'config/US/file_renames.tsv'
ADDR = re.compile(r'^(?:sub|func|D)_([0-9A-Fa-f]{8})$')
SYMBOL_STEM = re.compile(r'^(?:func|sub)_[0-9A-Fa-f]{8}$')


def read_tsv(path):
    with open(path, newline='', encoding='utf-8') as f:
        lines = [line for line in f if line.strip() and not line.startswith('#')]
    return list(csv.DictReader(lines, delimiter='\t'))


def load_symbols(path):
    """({VA upper hex: new}, {old non-address name: new})."""
    by_va, by_old = {}, {}
    for r in read_tsv(path):
        va = r['va'].strip().upper()
        by_va[va] = r['new']
        if not ADDR.match(r['old']) and r['old'] != r['new']:
            by_old[r['old']] = r['new']
    return by_va, by_old


def load_files(path):
    """{old literal: new literal}: full paths (/ and \\), semantic/<name>, file names, stems."""
    out = {}
    for r in read_tsv(path):
        old, new = r['old_path'].strip(), r['new_path'].strip()
        out[old] = new
        out[old.replace('/', '\\')] = new.replace('/', '\\')
        parent = old.rsplit('/', 2)
        if len(parent) == 3:
            out[parent[1] + '/' + parent[2]] = new.rsplit('/', 2)[1] + '/' + new.rsplit('/', 1)[1]
        oname, nname = old.rsplit('/', 1)[-1], new.rsplit('/', 1)[-1]
        out[oname] = nname
        ostem, nstem = oname.rsplit('.', 1)[0], nname.rsplit('.', 1)[0]
        if not SYMBOL_STEM.match(ostem):
            out[ostem] = nstem
    return out


def build(by_va, by_old, files):
    """One regex: file literals first (longest first), then old names, then address names."""
    parts = []
    for k in sorted(files, key=len, reverse=True):
        if '/' in k or '\\' in k:
            parts.append(r'(?<![\w$.-])' + re.escape(k) + r'(?![\w])')
        else:
            parts.append(r'(?<![\w$./\\-])' + re.escape(k) + r'(?![\w])')
    file_alt = '|'.join(parts) or r'(?!)'
    old_alt = '|'.join(re.escape(k) for k in sorted(by_old, key=len, reverse=True)) or r'(?!)'
    # an optional ` (name)` after a symbol: the decomp docs write `sub_8011BE10` (carDestroyed)
    tail = r'(?P<tail>(?P<tick>`?) \((?P<tailname>[A-Za-z_][\w$]*)\))?'
    # symbols: not inside a path (reference/m2c/func_X.c) and not a file name (func_X.c)
    return re.compile(r'(?P<file>%s)|(?<![\w$/\\])(?:(?P<old>%s)|(?P<addr>(?:sub|func|D|FUN|DAT)_(?P<va>[0-9A-Fa-f]{8})))'
                      r'(?![\w$])(?!\.(?:c|cpp|h|hpp|s)\b)%s' % (file_alt, old_alt, tail))


def rewrite(text, pattern, by_va, by_old, files, annotate, changes):
    def sub(m):
        if m.group('file'):
            old = m.group('file')
            new = files[old]
            kind = 'file'
        elif m.group('old'):
            old = m.group('old')
            new = by_old[old]
            kind = 'symbol'
        else:
            old = m.group('addr')
            new = by_va.get(m.group('va').upper())
            kind = 'symbol'
            if new is None:
                return m.group(0)
        line = text.count('\n', 0, m.start()) + 1
        tail = m.group('tail') if kind == 'symbol' else None
        if tail and m.group('tailname') == new:
            # already written as `old (new)`: keep it with --annotate, else just `new`
            if annotate:
                return m.group(0)
            changes.append((line, old + tail, new + m.group('tick')))
            return new + m.group('tick')
        new_text = '%s (%s)' % (new, old) if kind == 'symbol' and annotate else new
        changes.append((line, old, new_text))
        return new_text + (tail or '')
    return pattern.sub(sub, text)


def expand(paths, exts):
    out = []
    for p in paths:
        p = Path(p)
        if p.is_dir():
            out += sorted(q for q in p.rglob('*') if q.is_file() and q.suffix.lower() in exts)
        elif p.is_file():
            out.append(p)
        else:
            print('warning: %s does not exist' % p, file=sys.stderr)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('paths', nargs='+', help='Markdown/text files or directories')
    ap.add_argument('--names', type=Path, default=DEFAULT_NAMES, help='names.tsv (default: %(default)s)')
    ap.add_argument('--files', type=Path, default=DEFAULT_FILES, help='file_renames.tsv (default: %(default)s)')
    ap.add_argument('--dry-run', action='store_true', help='list the changes, write nothing')
    ap.add_argument('--annotate', action='store_true', help='write "new (old)" for symbols')
    ap.add_argument('--ext', default='.md,.txt', help='extensions for directory arguments (default: %(default)s)')
    a = ap.parse_args()
    for p in (a.names, a.files):
        if not p.is_file():
            raise SystemExit('port_rename_notes: %s not found (pass --names / --files)' % p)
    by_va, by_old = load_symbols(a.names)
    files = load_files(a.files)
    pattern = build(by_va, by_old, files)
    exts = {e.strip().lower() if e.strip().startswith('.') else '.' + e.strip().lower()
            for e in a.ext.split(',') if e.strip()}
    total = edited = 0
    for path in expand(a.paths, exts):
        data = path.read_bytes()
        try:
            text = data.decode('utf-8')
        except UnicodeDecodeError:
            print('warning: %s is not UTF-8, skipped' % path, file=sys.stderr)
            continue
        changes = []
        new = rewrite(text, pattern, by_va, by_old, files, a.annotate, changes)
        if new == text:
            continue
        edited += 1
        total += len(changes)
        print('%s: %d' % (path, len(changes)))
        if a.dry_run:
            for line, old, repl in changes:
                print('  %d: %s -> %s' % (line, old, repl))
        else:
            path.write_bytes(new.encode('utf-8'))
    print('%d replacements in %d files%s' % (total, edited, ' (dry run, nothing written)' if a.dry_run else ''))


if __name__ == '__main__':
    main()
