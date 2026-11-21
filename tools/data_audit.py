#!/usr/bin/env python3
"""Audit the data that maintained C units define, embed or describe against retail.

    py -3.14 tools/data_audit.py [image-report.json] [--verbose] [--linked-only]

Defaults to the image named by LAST_BUILD.json. Read-only: it reads the build's
linked unit ELFs (work/build-*/psx/unit-NNNN/unit.elf, matched to the report by
hash), the compiled objects (work/build-*/compile/*/), bin/SCUS_94306R.EXE and
the sources; it compiles, links and writes nothing.

Code bytes are match_status.py's job. This tool checks everything else:

 1. Every allocated section of every unit object is classified (code, placed
    data, metadata). An allocated data section that is not placed would make
    the builder skip the unit, so any such section is reported.
 2. Every placed data section (.tm2_<addr> from a section attribute or
    `#pragma tm2_rdata`, or .data/.sdata/.bss/.rodata placed through a D_<addr>
    symbol) is compared with retail at its linked address:
      - inside the file image: linked bytes (relocations resolved) == retail;
      - beyond the file image (retail BSS, zeroed at boot): C must be all zero;
      - NOBITS (C zero-initialised) inside the file image: retail must be zero.
    Words carrying a code relocation (switch jump tables, function pointers)
    are reported as case -> target offsets relative to the target function.
 3. Code sections carrying R_MIPS_32 relocations (a jump table emitted inside
    .text) are listed; their bytes are part of the code comparison.
 4. Every symbol named after a retail address (D_X, aX, sub_X, func_X) in a
    linked unit sits at that address, and so does every global named in
    config/US/names.tsv (readable names given to retail symbols).
 5. Comments that quote retail strings next to a declaration
    (`extern char D_X[]; /* "text" */`, `char *D_X; /* "text" */`,
    `char *D_X[]; /* { "A", "B", ... } */`) match the C string in retail.
    All sources under src/semantic are scanned, linked or not.

C files under src/semantic that the build does not link (data-only units such
as font_state.c) are compiled and placed the same way in a system temp folder
(deleted afterwards) and audited too, unless --linked-only.

A unit whose source changed since the build is audited from the newest
work/match/<unit>-* run of the current source when one exists (else flagged
STALE and audited from the build's object).
"""
import csv
import hashlib
import json
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).parent))
from semantic_elf import read_elf  # noqa: E402
from symbol_names import renamed_addresses  # noqa: E402

ADDRNAME = re.compile(r'^(?:D_|a|sub_|func_)([0-9A-Fa-f]{8})$')
SECTION_HEX = re.compile(r'^\.tm2_(?:string_|init_)?([0-9A-Fa-f]{8})$')
R_MIPS_32 = 2


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Retail:
    def __init__(self):
        self.raw = (ROOT / 'bin/SCUS_94306R.EXE').read_bytes()
        self.load = struct.unpack_from('<I', self.raw, 0x18)[0]
        self.end = self.load + len(self.raw) - 0x800

    def inside(self, addr, n=1):
        return self.load <= addr and addr + n <= self.end

    def read(self, addr, n):
        o = addr - self.load + 0x800
        return self.raw[o:o + n]

    def word(self, addr):
        return struct.unpack_from('<I', self.raw, addr - self.load + 0x800)[0] if self.inside(addr, 4) else None

    def cstring(self, addr, limit=512):
        if not self.inside(addr):
            return None
        b = self.read(addr, min(limit, self.end - addr))
        return b.split(b'\0', 1)[0]


def symbol_map():
    out = {'_gp': 0x801809D4}
    for r in csv.DictReader((ROOT / 'config/US/ida_names.tsv').open(), delimiter='\t'):
        out[r['name']] = int(r['va'], 16)
    for path in (ROOT / 'config/US/undefined_syms_auto.txt', ROOT / 'config/US/undefined_syms_extra.txt'):
        for line in path.read_text().splitlines():
            m = re.match(r'\s*([A-Za-z_.$][\w.$]*)\s*=\s*0x([0-9A-Fa-f]+)\s*;', line)
            if m:
                out[m.group(1)] = int(m.group(2), 16)
    out.update(renamed_addresses())  # config/US/names.tsv: readable names given to retail symbols
    return out


def address_of(name, syms):
    if name in syms:
        return syms[name]
    m = ADDRNAME.match(name)
    return int(m.group(1), 16) if m else None


def relocations(elf):
    """{target section index: [(offset, type, symbol)]} from the SHT_REL sections."""
    out = {}
    for s in elf['sections']:
        if s['type'] != 9:
            continue
        for off in range(0, s['size'], 8):
            r_off, info = struct.unpack_from('<II', s['data'], off)
            out.setdefault(s['info'], []).append((r_off, info & 0xFF, info >> 8))
    return out


def parse_ld(path):
    """unit.ld -> {input section name: (output section name, base)}."""
    text = Path(path).read_text(encoding='ascii')
    return {m.group(3): (m.group(1), int(m.group(2), 16))
            for m in re.finditer(r'(\.s\d+) 0x([0-9A-Fa-f]+) : \{ \*\(([^)]+)\) \}', text)}


def hexdiff(c, r, start=0, width=16):
    """First differing window of c (C) vs r (retail) as two short hex/ascii lines."""
    n = min(len(c), len(r))
    first = next((i for i in range(n) if c[i] != r[i]), n)
    lo = max(0, first - first % 4)
    hi = min(max(len(c), len(r)), lo + width)

    def show(b):
        seg = b[lo:hi]
        asc = ''.join(chr(x) if 32 <= x < 127 else '.' for x in seg)
        return seg.hex(' ') + '  |' + asc + '|'
    return lo, show(c), show(r)


def c_unescape(s):
    out = bytearray()
    i = 0
    simple = {'n': 10, 't': 9, 'r': 13, '0': 0, '\\': 92, '"': 34, "'": 39, 'a': 7, 'b': 8, 'f': 12, 'v': 11}
    while i < len(s):
        ch = s[i]
        if ch == '\\' and i + 1 < len(s):
            nx = s[i + 1]
            if nx == 'x':
                m = re.match(r'[0-9A-Fa-f]{1,2}', s[i + 2:])
                out.append(int(m.group(0), 16))
                i += 2 + len(m.group(0))
                continue
            if nx in '01234567' and re.match(r'[0-7]{2,3}', s[i + 1:]):
                m = re.match(r'[0-7]{1,3}', s[i + 1:])
                out.append(int(m.group(0), 8) & 0xFF)
                i += 1 + len(m.group(0))
                continue
            if nx in simple:
                out.append(simple[nx])
                i += 2
                continue
            # retail paths like "SHELLDB1\CARSEL.TIM" are quoted with one backslash
            out.append(92)
            i += 1
            continue
        out += ch.encode('latin-1', errors='replace')
        i += 1
    return bytes(out)


class Audit:
    def __init__(self, report_path):
        self.report_path = Path(report_path)
        self.report = json.loads(self.report_path.read_text(encoding='utf-8'))
        self.psx = self.report_path.parent
        self.retail = Retail()
        self.syms = symbol_map()
        self.renamed = renamed_addresses()
        self.problems = []   # (kind, unit, what, address, detail lines)
        self.notes = []
        self.counts = dict(units=0, sections_data=0, sections_code_tables=0, tables=0, table_words=0,
                           comment_strings=0, address_names=0, stale=0, fresh=0, unlinked=0, common=0)

    def problem(self, kind, unit, what, address, *detail):
        self.problems.append((kind, unit, what, address, list(detail)))

    # -- units -------------------------------------------------------------
    def unit_artifacts(self):
        elves = {}
        for p in sorted(self.psx.glob('unit-*/unit.elf')):
            elves[sha(p)] = p
        for u in self.report['units']:
            src = ROOT / u['source']
            obj = ROOT / u['object']
            elf = elves.get(u['linked_elf_sha256'])
            if elf is None:
                self.notes.append('no linked ELF found for ' + u['source'])
                continue
            stale = not src.exists() or sha(src) != u['source_sha256']
            if stale:
                fresh = self.fresh_match(src)
                if fresh:
                    self.counts['fresh'] += 1
                    self.notes.append('%s changed since the build: audited from %s' % (u['source'], fresh[1].parent.relative_to(ROOT).as_posix()))
                    obj, elf = fresh
                else:
                    self.counts['stale'] += 1
                    self.notes.append('STALE %s changed since the build and has no work/match run of the current source; audited from the build object' % u['source'])
            yield u['source'], obj, elf

    def fresh_match(self, src):
        if not src.exists():
            return None
        want = sha(src)
        for d in sorted((ROOT / 'work/match').glob(src.stem + '-*'), reverse=True):
            mj = d / 'match.json'
            if not mj.exists() or not re.fullmatch(re.escape(src.stem) + r'-\d{8}-\d{6}-\d+', d.name):
                continue
            try:
                if json.loads(mj.read_text())['source_sha256'] != want:
                    continue
            except (ValueError, KeyError):
                continue
            obj = d / 'compile/candidate.o'
            if not obj.exists():
                obj = d / 'compile/source.o'
            if obj.exists() and (d / 'unit.elf').exists():
                return obj, d / 'unit.elf'
        return None

    def audit_unit(self, source, obj_path, elf_path):
        self.counts['units'] += 1
        obj = read_elf(obj_path)
        linked = read_elf(elf_path)
        ld = parse_ld(Path(elf_path).parent / 'unit.ld')
        out_by_name = {s['name']: s for s in linked['sections']}
        relocs = relocations(obj)
        sec_by_index = {s['index']: s for s in obj['sections']}
        for s in obj['sections']:
            if not (s['flags'] & 2) or not s['size'] or s['name'].startswith('.MIPS'):
                continue
            code = bool(s['flags'] & 4)
            if s['name'] not in ld:
                self.problem('UNPLACED', source, s['name'], None, 'allocated section of %d bytes not placed (the builder skips this unit)' % s['size'])
                continue
            out_name, base = ld[s['name']]
            out = out_by_name.get(out_name)
            if out is None or out['address'] != base:
                self.problem('LINK', source, s['name'], base, 'linked output section %s missing or moved' % out_name)
                continue
            rel32 = [r for r in relocs.get(s['index'], []) if r[1] == R_MIPS_32]
            if code:
                if rel32:
                    self.counts['sections_code_tables'] += 1
                    self.notes.append('%s %s: %d R_MIPS_32 words inside code (inline table, covered by the code comparison)' % (source, s['name'], len(rel32)))
                continue
            self.counts['sections_data'] += 1
            self.audit_data(source, obj, s, out, base, rel32, sec_by_index, ld, out_by_name)
        # tentative definitions (COMMON): zero storage the builder cannot place
        for y in obj['symbols']:
            if y['section_index'] != 0xFFF2:
                continue
            self.counts['common'] += 1
            a = address_of(y['name'], self.syms)
            if a is None:
                self.problem('COMMON', source, y['name'], None, 'tentative definition with no retail address')
            elif self.retail.inside(a, y['size']) and any(self.retail.read(a, y['size'])):
                self.problem('COMMON', source, y['name'], a, 'C zero-initialises (tentative definition); retail ' + self.retail.read(a, min(16, y['size'])).hex(' '))
        # 4. address-named symbols sit at their address
        #    (and names given to retail symbols in config/US/names.tsv sit at their row's VA)
        for y in linked['symbols']:
            m = ADDRNAME.match(y['name'])
            if y['section_index'] == 0 or y['section_index'] >= 0xFF00:
                continue
            if not m:
                va = self.renamed.get(y['name'])
                if va is not None and y['binding'] in (1, 2):
                    self.counts['address_names'] += 1
                    if va != y['address']:
                        self.problem('SYMBOL', source, y['name'], y['address'], 'linked at %08X, names.tsv says %08X' % (y['address'], va))
                continue
            self.counts['address_names'] += 1
            if int(m.group(1), 16) != y['address']:
                self.problem('SYMBOL', source, y['name'], y['address'], 'linked at %08X, name says %s' % (y['address'], m.group(1).upper()))

    def symbol_at(self, obj, sec_index, offset):
        best = None
        for y in obj['symbols']:
            if y['section_index'] == sec_index and y['name'] and y['type'] != 3 and y['address'] <= offset:
                if best is None or y['address'] > best['address']:
                    best = y
        return '%s+0x%X' % (best['name'], offset - best['address']) if best else '+0x%X' % offset

    def audit_data(self, source, obj, s, out, base, rel32, sec_by_index, ld, out_by_name):
        size = s['size']
        name = s['name']
        written = bool(SECTION_HEX.match(name))
        tag = name + ('' if written else ' (linked, not written to the image)')
        nobits = s['type'] == 8
        c = bytes(size) if nobits else out['data']
        r_in = self.retail.inside(base, size)
        if not r_in and self.retail.inside(base):
            # straddles the end of the file image: the part beyond it is retail BSS
            cut = self.retail.end - base
            if any(c[cut:]):
                i = cut + next(i for i, b in enumerate(c[cut:]) if b)
                self.problem('DATA', source, tag, base, 'C initialises retail BSS (zero at boot) with nonzero bytes at %s' % self.symbol_at(obj, s['index'], i))
            c, size = c[:cut], cut
            r_in = True
        if not r_in:
            # retail BSS: zero at boot
            if any(c):
                i = next(i for i, b in enumerate(c) if b)
                self.problem('DATA', source, tag, base, 'C initialises retail BSS (zero at boot) with nonzero bytes at %s' % self.symbol_at(obj, s['index'], i),
                             'C      ' + c[i - i % 4:i - i % 4 + 16].hex(' '))
            return
        r = self.retail.read(base, size)
        # code relocations (switch tables, function pointers): compare as offsets in the target function
        table_words = {}
        for off, _, symi in rel32:
            sym = obj['symbols'][symi]
            tsec = sec_by_index.get(sym['section_index'])
            if tsec is not None and tsec['flags'] & 4 and tsec['name'] in ld:
                table_words[off] = (tsec['name'], ld[tsec['name']][1], out_by_name[ld[tsec['name']][0]]['size'])
        if table_words:
            self.counts['tables'] += 1
            self.counts['table_words'] += len(table_words)
        if c == r:
            return
        diffs = [i for i in range(0, size, 4) if c[i:i + 4] != r[i:i + 4]]
        tw = [i for i in diffs if i in table_words]
        other = [i for i in diffs if i not in table_words]
        if tw:
            by_target = {}
            for i in tw:
                by_target.setdefault(table_words[i][:2], []).append(i)
            for (tname, tbase), offs in by_target.items():
                tcode = out_by_name[ld[tname][0]]['data']
                same_code = tcode == self.retail.read(tbase, len(tcode))
                lines = []
                for i in offs[:8]:
                    cw = struct.unpack_from('<I', c, i)[0]
                    rw = struct.unpack_from('<I', r, i)[0]
                    lines.append('word %s (%08X): C -> +0x%X, retail -> +0x%X' % (self.symbol_at(obj, s['index'], i), base + i, cw - tbase, rw - tbase))
                if len(offs) > 8:
                    lines.append('... %d more words' % (len(offs) - 8))
                # shape: which entries share a target, and their order (meaningful even when code differs)
                idx = sorted(table_words)
                idx = [i for i in idx if table_words[i][0] == tname]
                cv = [struct.unpack_from('<I', c, i)[0] for i in idx]
                rv = [struct.unpack_from('<I', r, i)[0] for i in idx]
                shape = (lambda v: [sorted(set(v)).index(x) for x in v])
                same_shape = shape(cv) == shape(rv)
                kind = 'TABLE' if same_code or not same_shape else 'TABLE?'
                head = ('function at %08X is byte-identical: the C case labels disagree with retail' % tbase if same_code else
                        'function at %08X differs from retail; table shape %s' % (tbase, 'matches (offsets differ only because the code does)' if same_shape else 'DIFFERS (case grouping/order)'))
                self.problem(kind, source, tag, base, head, *lines)
        if other:
            i = other[0]
            lo, cl, rl = hexdiff(c, r)
            syms = sorted({self.symbol_at(obj, s['index'], j).split('+')[0] for j in other})
            self.problem('DATA', source, tag, base, '%d of %d words differ (%s); first at %s (%08X)%s' % (
                len(other), (size + 3) // 4, ', '.join(syms[:6]), self.symbol_at(obj, s['index'], i), base + i,
                ' [C zero-initialised (NOBITS)]' if nobits else ''),
                'C      %08X: %s' % (base + lo, cl), 'retail %08X: %s' % (base + lo, rl))

    # -- 5. comments quoting retail strings ---------------------------------
    DECL = re.compile(r'^(?P<decl>[^/]*?\b(?P<name>[A-Za-z_]\w*)\s*(?P<arr>\[[^\]]*\])?\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?(?:=[^;]*)?;)\s*(?://|/\*)\s*(?P<comment>.*?)\s*(?:\*/)?\s*$')
    STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')

    def audit_comments(self):
        files = [p for p in sorted((ROOT / 'src/semantic').rglob('*')) if p.suffix in ('.c', '.cpp', '.h', '.hpp')
                 and not any(part.startswith('tmp') for part in p.relative_to(ROOT / 'src/semantic').parts)]
        for p in files:
            rel = p.relative_to(ROOT).as_posix()
            for ln, line in enumerate(p.read_text(encoding='utf-8', errors='replace').splitlines(), 1):
                m = self.DECL.match(line)
                if not m or '"' not in m.group('comment'):
                    continue
                comment = m.group('comment')
                strings = self.STRING.findall(comment)
                if not strings:
                    continue
                name = m.group('name')
                decl = m.group('decl')
                # only comments that open with the quoted value: `"text" ...` or `{ "a", "b", ... }`
                # (not prose that merely quotes a word, e.g. `/* ... a "parent" type */`)
                body = comment.strip()
                is_list = body.startswith('{')
                if not (is_list or body.startswith('"')):
                    continue
                addr = address_of(name, self.syms)
                if addr is None:
                    continue
                self.counts['comment_strings'] += 1
                pointer = bool(re.search(r'\*\s*' + re.escape(name) + r'\b', decl))
                where = '%s:%d' % (rel, ln)
                if is_list or (pointer and m.group('arr')):
                    for k, text in enumerate(strings):
                        ptr = self.retail.word(addr + 4 * k)
                        self.compare_string(where, name, addr + 4 * k, ptr, c_unescape(text), '[%d]' % k)
                elif pointer:
                    self.compare_string(where, name, addr, self.retail.word(addr), c_unescape(strings[0]), '')
                else:
                    self.compare_string(where, name, addr, addr, c_unescape(strings[0]), '')

    def compare_string(self, where, name, slot, addr, want, suffix):
        got = self.retail.cstring(addr) if addr is not None else None
        if got == want:
            return
        hint = ''
        if got is not None:
            for delta in range(-64, 65, 4):   # the same text a few words away: a wrong address in the name
                if delta and self.retail.cstring(addr + delta) == want:
                    hint = '  (that text is at %08X)' % (addr + delta)
                    break
        self.problem('COMMENT', where, name + suffix, addr if addr is not None else slot,
                     'comment  "%s"' % want.decode('latin-1').encode('unicode_escape').decode(),
                     'retail   %s%s' % ('"%s"' % got.decode('latin-1').encode('unicode_escape').decode() if got is not None else '(outside the file image)', hint))

    # -- sources the build does not link (data-only units) --------------------
    def compile_unlinked(self, tmp):
        """Compile and link the src/semantic C files the build does not link (mostly
        data-only units) into tmp, exactly as the builder would place them."""
        from semantic_compile import AS, CC1, CPP, MASPSX, MIPS
        linked = {u['source'] for u in self.report['units']}
        sources = [p for p in sorted((ROOT / 'src/semantic').glob('*.c')) if p.relative_to(ROOT).as_posix() not in linked]
        cflags = ['-EL', '-nostdinc', '-undef', '-D__GNUC__=2', '-D__OPTIMIZE__', '-Dmips', '-D__mips__', '-D__mips',
                  '-Dpsx', '-D__psx__', '-D__psx', '-D__EXTENSIONS__', '-D_MIPSEL', '-D__CHAR_UNSIGNED__',
                  '-D_LANGUAGE_C', '-DLANGUAGE_C', '-DINCLUDE_ASM_USE_MACRO_INC=1', '-I' + str(ROOT / 'include'),
                  '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'toolchain/psyq330/include')]
        for src in sources:
            rel = src.relative_to(ROOT).as_posix()
            d = Path(tmp) / src.stem
            d.mkdir()
            env = dict(__import__('os').environ, TMP=str(d), TEMP=str(d), TMPDIR=str(d))
            steps = [[CPP, *cflags, src, '-o', d / 's.i'],
                     [CC1, '-O2', '-quiet', '-fno-builtin', '-G8', '-fno-delayed-branch', d / 's.i', '-o', d / 's.s'],
                     None,
                     [AS, '-EL', '-I' + str(ROOT / 'include'), '-I' + str(ROOT), '-march=r3000', '-mtune=r3000',
                      '-no-pad-sections', '-O1', '-G0', '-o', d / 's.o', d / 's.ms']]
            ok = True
            for step in steps:
                if step is None:
                    q = subprocess.run([sys.executable, str(MASPSX), '--aspsx-version=2.34', '--expand-div', '--dont-force-G0',
                                        '-G8', str(d / 's.s')], capture_output=True, stdin=subprocess.DEVNULL, timeout=60)
                    (d / 's.ms').write_bytes(q.stdout)
                else:
                    q = subprocess.run(list(map(str, step)), cwd=d, env=env, capture_output=True, stdin=subprocess.DEVNULL, timeout=60)
                if q.returncode:
                    self.notes.append('UNLINKED %s does not compile (%s); not audited' % (rel, (q.stdout + q.stderr).decode(errors='replace').strip()[-160:]))
                    ok = False
                    break
            if not ok:
                continue
            obj = read_elf(d / 's.o')
            bases, imports = {}, []
            for y in obj['symbols']:
                if y['binding'] not in (1, 2) or not y['name']:
                    continue
                if y['section_index'] == 0:
                    imports.append(y['name'])
                    continue
                if y['section_index'] >= 0xFF00:
                    continue
                sec = obj['sections'][y['section_index']]['name']
                a = address_of(y['name'], self.syms)
                if a is None:
                    m = SECTION_HEX.match(sec) or re.match(r'^\.text_([0-9A-Fa-f]{8})$', sec)
                    a = int(m.group(1), 16) + y['address'] if m else None
                if a is not None:
                    bases.setdefault(sec, a - y['address'])
            for s in obj['sections']:
                m = SECTION_HEX.match(s['name']) or re.match(r'^\.text_([0-9A-Fa-f]{8})$', s['name'])
                if m and s['flags'] & 2 and s['size'] and s['name'] not in bases:
                    bases[s['name']] = int(m.group(1), 16)
            missing = [n for n in imports if address_of(n, self.syms) is None and n != '_gp']
            if missing:
                self.notes.append('UNLINKED %s has unresolved imports %s; not audited' % (rel, missing[:5]))
                continue
            placed = ' '.join('.s%d 0x%08X : { *(%s) }' % (n, b, s) for n, (s, b) in enumerate(sorted(bases.items())))
            defs = ' '.join('%s = 0x%08X;' % (n, address_of(n, self.syms)) for n in sorted(set(imports)) if n != '_gp') + ' _gp = 0x%08X;' % self.syms['_gp']
            (d / 'unit.ld').write_text('SECTIONS { ' + placed + ' /DISCARD/ : { *(.reginfo .pdr .comment .gnu.attributes .mdebug*) } }\n' + defs + '\n', encoding='ascii')
            q = subprocess.run([str(MIPS / 'mipsel-none-elf-ld.exe'), '-EL', '-T', str(d / 'unit.ld'), '-o', str(d / 'unit.elf'), str(d / 's.o')],
                               capture_output=True, timeout=30)
            if q.returncode:
                self.notes.append('UNLINKED %s does not link (%s); not audited' % (rel, (q.stdout + q.stderr).decode(errors='replace').strip()[-160:]))
                continue
            self.counts['unlinked'] += 1
            yield rel + ' [not linked by the build]', d / 's.o', d / 'unit.elf'

    def run(self, verbose, unlinked=True):
        for source, obj, elf in self.unit_artifacts():
            self.audit_unit(source, obj, elf)
        if unlinked:
            with tempfile.TemporaryDirectory(prefix='tm2-data-audit-') as tmp:
                for source, obj, elf in self.compile_unlinked(tmp):
                    self.audit_unit(source, obj, elf)
        self.audit_comments()
        for n in self.notes if verbose else [n for n in self.notes if n.startswith(('STALE', 'UNLINKED')) or 'changed since' in n]:
            print('note:', n)
        for kind, unit, what, addr, detail in self.problems:
            print('%-7s %s  %s  %s' % (kind, '%08X' % addr if addr is not None else '--------', unit, what))
            for d in detail:
                print('          ' + d)
        c = self.counts
        kinds = {}
        for k, *_ in self.problems:
            kinds[k] = kinds.get(k, 0) + 1
        print('summary: %d units, %d data sections (%d with code-pointer tables, %d table words), %d code sections with inline tables, '
              '%d tentative definitions, %d address-named symbols, %d quoted-string comments; %d of the units are sources the build does not link; '
              '%d stale, %d audited from a fresh match run' % (
                  c['units'], c['sections_data'], c['tables'], c['table_words'], c['sections_code_tables'], c['common'],
                  c['address_names'], c['comment_strings'], c['unlinked'], c['stale'], c['fresh']))
        print('mismatches: %d%s' % (len(self.problems), (' (' + ', '.join('%s %d' % kv for kv in sorted(kinds.items())) + ')') if kinds else ''))
        return 1 if self.problems else 0


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if args:
        path = Path(args[0])
    else:
        last = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))
        path = ROOT / Path(last['executable']).parent / 'image-report.json'
    sys.exit(Audit(path).run('--verbose' in sys.argv, '--linked-only' not in sys.argv))


if __name__ == '__main__':
    main()
