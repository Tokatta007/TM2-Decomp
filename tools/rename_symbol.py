#!/usr/bin/env python3
"""Give a retail function or global a readable name everywhere the build uses it.

    py -3.14 tools/rename_symbol.py sub_8011BE10 carDestroyed --evidence "docs/X.md"
    py -3.14 tools/rename_symbol.py --batch renames.tsv            # TSV: old <TAB> new <TAB> comment
    py -3.14 tools/rename_symbol.py --batch renames.tsv --dry-run  # report only, write nothing

What is renamed (whole word, exact, case-sensitive):
  - src/semantic/**/*.c|.cpp|.h|.hpp (folders named tmp* are skipped) and include/semantic/*.h;
  - config/semantic/selection-template.json: functions[VA].symbol and every unit's exports / code_exports /
    imports names (exact JSON strings, so the file keeps its formatting);
  - config/US/undefined_syms_auto.txt / undefined_syms_extra.txt (`old = 0x...;` lines);
  - docs/*.md and README.md: the old name is kept (VA mentions stay) and the new one is appended once,
    `sub_8011BE10` -> `sub_8011BE10` (carDestroyed); --no-docs leaves the docs alone.

All spellings of the retail symbol are renamed together: for a function at VA X, `sub_X`, `func_X` and its
IDA name; for data, `D_X` and its IDA name.

config/US/ida_names.tsv and ida_funcs.tsv are never changed (they stay the IDA reference that other tools
look names up in). Instead the rename is recorded in config/US/names.tsv (va, old, new, evidence), which
every name -> address resolver of the build reads (tools/symbol_names.py), so the new name links at the
same VA. Symbol names do not change code bytes.

Refused (nothing written for that symbol):
  - a new name that is not a C identifier, is a C/C++ keyword, looks like an address name (D_X, sub_X,
    func_X, aX) or already occurs anywhere (sources, template, symbol files, ida_names.tsv, names.tsv);
    (new may be the IDA name of the same symbol: the C then adopts it; its names.tsv row has old = sub_X / D_X);
  - an old name that occurs nowhere in the sources/template, or whose VA cannot be determined;
  - a symbol used in a `#pragma tm2_symbol` line (the C++ pragma derives the section from the name);
  - a template name that is a C++ mangling of the old name (old__...);
  - a symbol that tools/*.py or build.py mention by name;
  - a symbol used in a file listed with --skip-files (e.g. files other people are editing now).
In a batch every row is checked against the state left by the rows before it; refused rows are reported
and the others applied.
"""
import argparse
import csv
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from symbol_names import COLUMNS, NAMES_TSV, load_renames  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / 'config/US'
TEMPLATE = ROOT / 'config/semantic/selection-template.json'
SYMBOL_FILES = [CONFIG / 'undefined_syms_auto.txt', CONFIG / 'undefined_syms_extra.txt']
IDENT = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')
ADDRNAME = re.compile(r'^(?:D_|a|sub_|func_)?([0-9A-Fa-f]{8})$')
FUNCNAME = re.compile(r'^(?:sub|func)_([0-9A-Fa-f]{8})$')
DATANAME = re.compile(r'^D_([0-9A-Fa-f]{8})$')
KEYWORDS = set('''auto break case char const continue default do double else enum extern float for goto if
inline int long register return short signed sizeof static struct switch typedef union unsigned void volatile
while asm bool catch class delete friend new operator private protected public template this throw try
virtual main'''.split())
FILE_EXT = r'(?!\.(?:c|cpp|h|hpp|s|o|i|inc|tsv|json|md)\b)'


def word(name):
    """Whole-word pattern: no identifier character (or $) on either side, not a file name (name.c).
    The file-name guard only applies when a source file of that name exists (e.g. effect_bindings/func_80111314.c):
    otherwise `name.h` / `name.s` are struct member accesses and must be renamed too."""
    files = {p.name for p in (ROOT / 'src').rglob(name + '.*')}
    guard = FILE_EXT if files else ''
    return re.compile(r'(?<![\w$])' + re.escape(name) + r'(?![\w$])' + guard)


def source_files():
    files = []
    for p in sorted((ROOT / 'src/semantic').rglob('*')):
        if p.is_file() and p.suffix in ('.c', '.cpp', '.h', '.hpp') and \
                not any(part.startswith('tmp') for part in p.relative_to(ROOT / 'src/semantic').parts[:-1]):
            files.append(p)
    files += sorted((ROOT / 'include/semantic').glob('*.h'))
    return files


def doc_files():
    return sorted((ROOT / 'docs').glob('*.md')) + [ROOT / 'README.md']


def tool_files():
    return [p for p in sorted((ROOT / 'tools').glob('*.py')) if p.name != Path(__file__).name] + [ROOT / 'build.py']


class Text:
    """A text file read as bytes-exact str (line endings kept)."""

    def __init__(self, path):
        self.path = path
        self.text = path.read_bytes().decode('utf-8')
        self.original = self.text

    def save(self):
        if self.text != self.original:
            self.path.write_bytes(self.text.encode('utf-8'))
            return True
        return False


def ida_tables():
    names, by_va = {}, {}
    for r in csv.DictReader((CONFIG / 'ida_names.tsv').open(newline=''), delimiter='\t'):
        va = int(r['va'], 16)
        names[r['name']] = (va, r['code'] == '1')
        by_va.setdefault(va, []).append((r['name'], r['code'] == '1'))
    return names, by_va


class Renamer:
    def __init__(self, docs=True, skip_files=(), dry_run=False):
        self.docs = docs
        self.dry_run = dry_run
        self.skip_files = set(skip_files)
        self.sources = {p: Text(p) for p in source_files()}
        self.docs_text = {p: Text(p) for p in doc_files() if p.exists()}
        self.symfiles = {p: Text(p) for p in SYMBOL_FILES}
        self.template = Text(TEMPLATE)
        self.tools = {p: p.read_text(encoding='utf-8', errors='replace') for p in tool_files()}
        self.ida, self.ida_by_va = ida_tables()
        self.renames = load_renames()
        self.applied = []

    # -- lookups --------------------------------------------------------------------------------------
    def resolve(self, old):
        """(va, is_code) of the retail symbol `old` (an address name, an IDA name or a names.tsv name)."""
        m = FUNCNAME.match(old)
        if m:
            return int(m.group(1), 16), True
        m = DATANAME.match(old)
        if m:
            return int(m.group(1), 16), False
        if old in self.ida:
            return self.ida[old]
        for r in self.renames:
            if r['new'] == old:
                return r['va'], r['old'].startswith(('sub_', 'func_')) or self.ida.get(r['old'], (0, False))[1]
        return None, None

    def spellings(self, old, va, code, new=None):
        names = {old}
        if code:
            names |= {'sub_%08X' % va, 'func_%08X' % va}
        else:
            names.add('D_%08X' % va)
        names |= {n for n, c in self.ida_by_va.get(va, ()) if c == code}
        names.discard(new)
        return sorted(names)

    def adopts_ida_name(self, new, va, code):
        """True when `new` is the IDA name of this very symbol (the C then simply uses the IDA name)."""
        return self.ida.get(new) == (va, code)

    def occurrences(self, name):
        """{relative path: count} of whole-word occurrences in sources, template and symbol files."""
        pat = word(name)
        out = {}
        for p, t in list(self.sources.items()) + list(self.symfiles.items()):
            n = len(pat.findall(t.text))
            if n:
                out[p.relative_to(ROOT).as_posix()] = n
        n = self.template.text.count('"%s"' % name)
        if n:
            out[TEMPLATE.relative_to(ROOT).as_posix()] = n
        return out

    # -- checks ---------------------------------------------------------------------------------------
    def check(self, old, new):
        if not IDENT.match(new):
            return 'new name %r is not a C identifier' % new
        if new in KEYWORDS:
            return 'new name %r is a keyword' % new
        if ADDRNAME.match(new):
            return 'new name %r looks like an address name' % new
        va, code = self.resolve(old)
        if va is None:
            return 'no VA known for %r (not an address name, IDA name or names.tsv name)' % old
        adopt = self.adopts_ida_name(new, va, code)
        names = self.spellings(old, va, code, new)
        if new == old:
            return 'old and new name are the same'
        if new in self.ida and not adopt:
            return 'new name %r is an IDA name (at %08X)' % (new, self.ida[new][0])
        for r in self.renames:
            if r['new'] == new:
                return 'new name %r is already used in names.tsv (for %08X)' % (new, r['va'])
        clash = self.occurrences(new)
        if clash and not adopt:
            return 'new name %r already occurs: %s' % (new, ', '.join(sorted(clash)))
        found = {}
        for n in names:
            for path, count in self.occurrences(n).items():
                found[path] = found.get(path, 0) + count
        if not found:
            return 'none of %s occurs in the sources or the template' % '/'.join(names)
        busy = sorted(Path(p).name for p in found if Path(p).name in self.skip_files)
        if busy:
            return 'used in skipped file(s) %s' % ', '.join(busy)
        pragma = re.compile(r'^[ \t]*#[ \t]*pragma[ \t]+tm2_symbol[ \t].*$', re.M)
        for p, t in self.sources.items():
            for line in pragma.findall(t.text):
                if any(word(n).search(line) for n in names):
                    return 'used in a #pragma tm2_symbol line (%s)' % p.relative_to(ROOT).as_posix()
        for n in names:
            if re.search(r'"' + re.escape(n) + r'__[^"]*"', self.template.text):
                return 'the template holds a C++ mangling of %s' % n
        for p, text in self.tools.items():
            for n in names:
                if word(n).search(text):
                    return '%s is named in %s' % (n, p.relative_to(ROOT).as_posix())
        return None

    # -- apply ----------------------------------------------------------------------------------------
    def rename(self, old, new, evidence=''):
        why = self.check(old, new)
        if why:
            return dict(old=old, new=new, refused=why)
        va, code = self.resolve(old)
        adopt = self.adopts_ida_name(new, va, code)
        names = self.spellings(old, va, code, new)
        changed = {}
        for n in names:
            pat = word(n)
            for p, t in self.sources.items():
                t.text, k = pat.subn(new, t.text)
                if k:
                    changed[p.relative_to(ROOT).as_posix()] = changed.get(p.relative_to(ROOT).as_posix(), 0) + k
            for p, t in self.symfiles.items():
                t.text, k = re.subn(r'(?m)^(\s*)' + re.escape(n) + r'(\s*=)', r'\g<1>' + new + r'\g<2>', t.text)
                if k:
                    changed[p.relative_to(ROOT).as_posix()] = changed.get(p.relative_to(ROOT).as_posix(), 0) + k
            k = self.template.text.count('"%s"' % n)
            if k:
                self.template.text = self.template.text.replace('"%s"' % n, '"%s"' % new)
                rel = TEMPLATE.relative_to(ROOT).as_posix()
                changed[rel] = changed.get(rel, 0) + k
        if self.docs:
            for p, t in self.docs_text.items():
                k = annotate_doc(t, names, new)
                if k:
                    changed[p.relative_to(ROOT).as_posix()] = changed.get(p.relative_to(ROOT).as_posix(), 0) + k
        # names.tsv: a rename of an already renamed symbol updates its row
        row = next((r for r in self.renames if r['va'] == va and r['new'] in names), None)
        if row:
            row['new'] = new
            if evidence:
                row['evidence'] = evidence
        else:
            # old = the IDA name; when the C adopts the IDA name itself, old = the address name it replaces
            retail = None if adopt else next((n for n, c in self.ida_by_va.get(va, ()) if c == code), None)
            retail = retail or ('sub_%08X' % va if code else 'D_%08X' % va)
            self.renames.append(dict(va=va, old=retail, new=new, evidence=evidence))
        result = dict(va='%08X' % va, old=old, new=new, spellings=names, files=changed)
        self.applied.append(result)
        return result

    def save(self):
        if self.dry_run:
            return []
        written = []
        for t in list(self.sources.values()) + list(self.symfiles.values()) + [self.template] + \
                list(self.docs_text.values()):
            if t.save():
                written.append(t.path.relative_to(ROOT).as_posix())
        if self.applied:
            json.loads(self.template.text)  # still valid JSON
            write_names(self.renames)
            written.append(NAMES_TSV.relative_to(ROOT).as_posix())
        return written


def annotate_doc(t, names, new):
    """Append ` (new)` after the first mention of each of `names` on a line (after the closing backtick when
    the mention is inside a code span); mentions already followed by `(new)` are left alone."""
    count = 0
    out_lines = []
    for line in t.text.split('\n'):
        inserts = []
        for n in names:
            for m in list(word(n).finditer(line))[:1]:  # the first mention on the line
                pos = m.end()
                if line[:m.start()].count('`') % 2:  # inside a code span: after its closing backtick
                    close = line.find('`', pos)
                    pos = close + 1 if close >= 0 else pos
                if line[pos:].startswith(' (%s)' % new) or line[pos:].startswith('(%s)' % new):
                    continue
                inserts.append(pos)
        for pos in sorted(set(inserts), reverse=True):
            line = line[:pos] + ' (%s)' % new + line[pos:]
            count += 1
        out_lines.append(line)
    t.text = '\n'.join(out_lines)
    return count


def write_names(rows):
    lines = ['# Readable names given to retail symbols (tools/rename_symbol.py). old = the IDA name, which',
             '# config/US/ida_names.tsv keeps; new = the name the C uses. evidence = where the purpose is confirmed.',
             '\t'.join(COLUMNS)]
    for r in sorted(rows, key=lambda r: r['va']):
        lines.append('%08X\t%s\t%s\t%s' % (r['va'], r['old'], r['new'], r['evidence'].replace('\t', ' ')))
    NAMES_TSV.write_bytes(('\n'.join(lines) + '\n').encode('utf-8'))


def read_batch(path):
    rows = []
    for ln, line in enumerate(Path(path).read_text(encoding='utf-8').splitlines(), 1):
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        cols = line.split('\t')
        if len(cols) < 2:
            raise SystemExit('%s:%d: expected old<TAB>new[<TAB>comment]' % (path, ln))
        if cols[0].strip() == 'old' and cols[1].strip() == 'new':
            continue
        rows.append((cols[0].strip(), cols[1].strip(), cols[2].strip() if len(cols) > 2 else ''))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('old', nargs='?')
    ap.add_argument('new', nargs='?')
    ap.add_argument('--evidence', default='', help='where the purpose is confirmed (names.tsv column)')
    ap.add_argument('--batch', type=Path, help='TSV file: old<TAB>new<TAB>comment (comment = evidence)')
    ap.add_argument('--dry-run', action='store_true', help='check and report, write nothing')
    ap.add_argument('--no-docs', action='store_true', help='do not annotate docs/*.md and README.md')
    ap.add_argument('--skip-files', default='', help='comma-separated file names: refuse symbols used in them')
    ap.add_argument('--json', action='store_true', help='print the report as JSON')
    a = ap.parse_args()
    if a.batch:
        if a.old or a.new:
            ap.error('give either OLD NEW or --batch')
        rows = read_batch(a.batch)
    elif a.old and a.new:
        rows = [(a.old, a.new, a.evidence)]
    else:
        ap.error('give OLD NEW or --batch FILE')
    r = Renamer(docs=not a.no_docs, skip_files=[s.strip() for s in a.skip_files.split(',') if s.strip()],
                dry_run=a.dry_run)
    report = [r.rename(old, new, ev) for old, new, ev in rows]
    written = r.save()
    if a.json:
        print(json.dumps(dict(results=report, written=written), indent=1))
    else:
        for x in report:
            if 'refused' in x:
                print('REFUSED  %s -> %s: %s' % (x['old'], x['new'], x['refused']))
            else:
                print('RENAMED  %s %s -> %s (%s) in %d file(s)' % (x['va'], x['old'], x['new'], '/'.join(x['spellings']),
                                                                  len(x['files'])))
        print('%d renamed, %d refused%s' % (sum('refused' not in x for x in report), sum('refused' in x for x in report),
                                           ' (dry run: nothing written)' if a.dry_run else ', %d file(s) written' % len(written)))
    if any('refused' in x for x in report):
        sys.exit(2)


if __name__ == '__main__':
    main()
