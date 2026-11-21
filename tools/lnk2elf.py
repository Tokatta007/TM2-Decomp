#!/usr/bin/env python3
"""Convert PsyQ LNK modules (from a .LIB archive or a lone .OBJ) into ELF32 MIPS relocatable objects.

    py -3.14 tools/lnk2elf.py LIB MODULE -o OUT.o [--version 33]   one module
    py -3.14 tools/lnk2elf.py --exact                               every module marked exact in
                                                                    config/US/sdk_modules.tsv -> work/sdk_lib/
                                                                    (prints a coverage report)

The converted objects are linked by the source-only build with --sdk-from-lib (tools/gen_link.py,
docs/SOURCE_BUILD.md "SDK from the PsyQ libraries"). They contain Sony's code, so they are only ever
written under work/ (gitignored), never into a tracked file.

Conversion (the LNK format is described in tools/psyq_lib.py):
  sections   .text -> .psyq.text ("ax"), .rdata -> .psyq.rdata ("a"), .data/.sdata/.ctors/.dtors ->
             .psyq.<name> ("aw"), .bss/.sbss -> .psyq.<name> (NOBITS). Empty sections are left out.
             Alignment is 4: the PsyQ linker packed TM2's modules at 4-byte boundaries although every
             LNK section declares 8 (e.g. LIBC C14 sits at 80138858).
  symbols    XDEF -> global FUNC/OBJECT; LNK locals -> local NOTYPE; every name a patch refers to (XREF,
             referenced XBSS commons, sectstart/sectend of an output section) -> undefined global. The
             caller may rename them (gen_link renames sectstart/sectend to the D_<retail address> label).
             XBSS commons are not allocated here: the retail link allocated them, so they resolve to
             their retail address like any other import.
  patches    word32 (16) -> R_MIPS_32, jump26 (74) -> R_MIPS_26, hi16 (82) -> R_MIPS_HI16, lo16 (84) ->
             R_MIPS_LO16, gprel16 (100) -> R_MIPS_GPREL16. The LNK linker overwrites the field, so the
             in-place addend is written fresh: the constant part of the expression. A HI16 is paired by
             ld with the next LO16 of the same symbol in the relocation table; the table is ordered so
             that every HI16 is followed by a LO16 whose low half agrees (an error if none exists).
             Expressions: constants, symbols, sectbase(own section) (the module's piece), sums with a
             constant, and differences. **The subtraction operands are stored reversed**: `- a b` in
             the prefix stream means b - a (libcd ISO9660 +0x6F8: `sectbase(.bss) + 0x600 - 0x24`,
             retail 80191AE8 = 8019150C + 0x5DC). Anything else (sectstart/sectend arithmetic beyond a
             constant, products, section-relative 16-bit patches (30)) is refused with an error.
  extern     a section may be externalized: it is not emitted and every reference to it becomes a
             reference to a named undefined symbol (gen_link uses D_<retail address>), so its bytes
             stay with the retail pieces / C units (libetc INTR .rdata, whose rcsid differs).
"""
import argparse
import csv
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import psyq_lib as P

ROOT = P.ROOT
CONFIG = ROOT / 'config' / 'US'
SDK_TSV = CONFIG / 'sdk_modules.tsv'
CACHE = ROOT / 'work' / 'sdk_lib'
CONVERTER_SHA = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()[:12]

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_REL, SHT_NOBITS = 1, 2, 3, 9, 8
SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR, SHF_INFO_LINK = 1, 2, 4, 0x40
STB_LOCAL, STB_GLOBAL = 0, 1
STT_NOTYPE, STT_OBJECT, STT_FUNC, STT_SECTION = 0, 1, 2, 3
R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16 = 2, 4, 5, 6, 7
RTYPE = {16: R_MIPS_32, 74: R_MIPS_26, 82: R_MIPS_HI16, 84: R_MIPS_LO16, 100: R_MIPS_GPREL16}
EF_MIPS_NOREORDER = 1  # what the unit objects carry
SECTION_KIND = {'.text': (SHF_ALLOC | SHF_EXECINSTR, SHT_PROGBITS), '.rdata': (SHF_ALLOC, SHT_PROGBITS),
                '.data': (SHF_ALLOC | SHF_WRITE, SHT_PROGBITS), '.sdata': (SHF_ALLOC | SHF_WRITE, SHT_PROGBITS),
                '.ctors': (SHF_ALLOC | SHF_WRITE, SHT_PROGBITS), '.dtors': (SHF_ALLOC | SHF_WRITE, SHT_PROGBITS),
                '.bss': (SHF_ALLOC | SHF_WRITE, SHT_NOBITS), '.sbss': (SHF_ALLOC | SHF_WRITE, SHT_NOBITS)}
M32 = 0xFFFFFFFF


class ConvertError(SystemExit):
    pass


def fail(msg):
    raise ConvertError('lnk2elf: ' + msg)


def elf_name(lnk_name):
    return '.psyq' + lnk_name


# ---------------------------------------------------------------------------------------------------------
# config/US/sdk_modules.tsv and the libraries

DATA_ITEM = re.compile(r'(\.\w+)(?:@([0-9A-Fa-f]{8})|\((0x[0-9a-fA-F]+)\)):(.*?)(?=\s+\.\w+(?:@|\()|$)')


def read_sdk_modules(path=SDK_TSV):
    """Rows of sdk_modules.tsv: lib, module, version, start, size, match, functions [VA], data {name: (addr|None, status)}."""
    lines = [l for l in Path(path).read_text(encoding='utf-8').splitlines() if l and not l.startswith('#')]
    out = []
    for r in csv.DictReader(lines, delimiter='\t'):
        data = {}
        for m in DATA_ITEM.finditer(r['data']):
            data[m.group(1)] = (int(m.group(2), 16) if m.group(2) else None, m.group(4).strip())
        out.append(dict(lib=r['lib'], module=r['module'], version=r['version'], start=int(r['retail_text_start'], 16),
                        size=int(r['text_size'], 16), match=r['match'],
                        functions=[int(v, 16) for v in r['functions'].split()], data=data))
    return out


def exact_modules(version='33'):
    """Modules whose .text is exact in `version` (every exact module is exact in 3.3)."""
    return [r for r in read_sdk_modules() if r['match'].startswith('exact') and version in r['version'].split('+')]


_LIBS = {}


def lib_path(lib, version='33'):
    d = P.SDK / ('psyq%s' % version) / 'LIB'
    return d / (lib if lib.upper().endswith('.OBJ') else lib + '.LIB')


def load_module(lib, module, version='33'):
    path = lib_path(lib, version)
    if not path.is_file():
        fail('%s is missing: the PsyQ %s libraries must be supplied locally in toolchain/psyq-sdk/ (docs/SDK_SOURCES.md)'
             % (path.relative_to(ROOT), version))
    if path not in _LIBS:
        _LIBS[path] = {m.name.upper(): m for m in P.read_lib(path)}
    m = _LIBS[path].get(module.upper())
    if m is None:
        fail('%s has no module %s' % (path.name, module))
    return m


# ---------------------------------------------------------------------------------------------------------
# expressions

def evaluate(obj, e):
    """(term, constant): term None, ('sec', section id) or ('name', symbol name)."""
    t = e[0]
    if t == 'const':
        return None, e[1]
    if t == 'base':
        return ('sec', e[1]), 0
    if t == 'sym':
        if e[1] in obj.xdefs:
            _, sec, off = obj.xdefs[e[1]]
            return ('sec', sec), off
        return ('name', obj.symname(e[1])), 0
    if t in ('sect', 'send'):
        s = obj.sections.get(e[1])
        nm = s.name if s else '%#x' % e[1]
        return ('name', '%s(%s)' % ('sectstart' if t == 'sect' else 'sectend', nm)), 0
    if t == '+':
        a, b = evaluate(obj, e[1]), evaluate(obj, e[2])
        if a[0] is not None and b[0] is not None:
            fail('%s: unsupported expression %s (sum of two relocatable terms)' % (obj.name, obj.expr_str(e)))
        return a[0] or b[0], (a[1] + b[1]) & M32
    if t == '-':
        left, right = evaluate(obj, e[1]), evaluate(obj, e[2])  # stored reversed: value = right - left
        if left[0] is not None:
            fail('%s: unsupported expression %s (subtracts a relocatable term)' % (obj.name, obj.expr_str(e)))
        return right[0], (right[1] - left[1]) & M32
    fail('%s: unsupported expression %s' % (obj.name, obj.expr_str(e)))


def patches(obj):
    """[(section, offset, kind, term, constant)] of every patch, kinds checked."""
    out = []
    for s in obj.sections.values():
        for off, kind, e in s.patches:
            if kind not in RTYPE:
                fail('%s: unsupported patch kind %d (%s) at %s+%#x' % (obj.name, kind, P.PATCH_NAME.get(kind, '?'), s.name, off))
            term, k = evaluate(obj, e)
            out.append((s, off, kind, term, k, e))
    return out


def observe(obj, bases, word):
    """Values of the named terms, read back from the retail image: {name: [addresses]}.

    `bases` maps LNK section ids to their retail addresses (text, located data); `word(va)` reads retail."""
    seen = {}
    for s in obj.sections.values():
        if s.id not in bases:
            continue
        rows = s.patches
        for i, (off, kind, e) in enumerate(rows):
            term, k = evaluate(obj, e)
            if term is None or term[0] != 'name':
                continue
            addr = bases[s.id] + off
            w = word(addr)
            if kind == 84:
                continue
            if kind == 82:
                lo = next((o2 for o2, k2, e2 in rows[i + 1:] if k2 == 84 and e2 == e), None)
                if lo is None:
                    continue
                val = (((w & 0xFFFF) << 16) + P.sext16(word(bases[s.id] + lo) & 0xFFFF)) & M32
            elif kind == 74:
                val = ((addr + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
            elif kind == 16:
                val = w
            else:
                val = (P.GP + P.sext16(w & 0xFFFF)) & M32
            seen.setdefault(term[1], set()).add((val - k) & M32)
    return {n: sorted(v) for n, v in seen.items()}


# ---------------------------------------------------------------------------------------------------------
# ELF writer

class Strtab:
    def __init__(self):
        self.data, self.index = bytearray(b'\0'), {'': 0}

    def add(self, s):
        if s not in self.index:
            self.index[s] = len(self.data)
            self.data += s.encode('latin-1') + b'\0'
        return self.index[s]


def order_hilo(rels, what):
    """Order a section's relocations (offset order) so that ld pairs every HI16 with a LO16 of the same
    symbol whose low half equals the HI16's constant. rels: [(offset, rtype, symidx, constant)]."""
    rels = sorted(rels, key=lambda r: r[0])

    def partner(table, i):
        sym = table[i][2]
        return next((j for j in range(i + 1, len(table)) if table[j][1] == R_MIPS_LO16 and table[j][2] == sym), None)

    out = [r for r in rels]
    for hi in [r for r in rels if r[1] == R_MIPS_HI16]:
        i = out.index(hi)
        j = partner(out, i)
        if j is not None and (out[j][3] - hi[3]) & 0xFFFF == 0:
            continue
        cands = [r for r in out if r[1] == R_MIPS_LO16 and r[2] == hi[2] and (r[3] - hi[3]) & 0xFFFF == 0]
        if not cands:
            fail('%s: HI16 at +%#x has no LO16 of the same target (%#x)' % (what, hi[0], hi[3]))
        after = [r for r in cands if r[0] > hi[0]]
        target = (after or cands)[0]
        out.pop(i)
        out.insert(out.index(target), hi)
    for i, r in enumerate(out):  # every HI16 now pairs as ld pairs it
        if r[1] == R_MIPS_HI16:
            j = partner(out, i)
            if j is None or (out[j][3] - r[3]) & 0xFFFF:
                fail('%s: could not pair HI16 at +%#x' % (what, r[0]))
    return out


def convert(mod, out_path, extern=None, omit=(), rename=None):
    """Write `mod` (psyq_lib.LibModule) as an ELF object. extern: {LNK section name: symbol name};
    omit: LNK section names to leave out (they must be unreferenced); rename: {import name: new name}.
    Returns a description: sections, globals, imports, relocation counts."""
    obj = mod.obj
    extern, rename, omit = dict(extern or {}), dict(rename or {}), set(omit)
    what = '%s:%s' % (mod.lib, mod.name)
    ps = patches(obj)
    by_name = {s.name: s for s in obj.sections.values()}
    for n in list(extern) + list(omit):
        if n not in by_name:
            fail('%s: no section %s to externalize/omit' % (what, n))
    emitted = [s for s in obj.sections.values() if s.size and s.name not in extern and s.name not in omit]
    for s in emitted:
        if s.name not in SECTION_KIND:
            fail('%s: unsupported section %s (%d bytes)' % (what, s.name, s.size))
    emitted_ids = {s.id for s in emitted}
    for s, off, kind, term, k, e in ps:
        if s.id not in emitted_ids:
            fail('%s: patch in omitted/externalized section %s' % (what, s.name))
        if term and term[0] == 'sec':
            t = obj.sections[term[1]]
            if t.id not in emitted_ids and t.name not in extern:
                fail('%s: %s+%#x refers to section %s, which is %s' % (what, s.name, off, t.name,
                                                                      'omitted' if t.name in omit else 'empty'))
    for n, sec, off in obj.xdefs.values():
        if sec not in emitted_ids:
            fail('%s: XDEF %s lies in section %s, which is not emitted' % (what, n, obj.sections[sec].name))

    # section header table: null, (section, rel) pairs, symtab, strtab, shstrtab
    shstr, strtab = Strtab(), Strtab()
    shdrs = [dict(name='', type=0, flags=0, data=b'', size=0, link=0, info=0, align=0, entsize=0)]
    sec_index = {}
    for s in emitted:
        flags, typ = SECTION_KIND[s.name]
        sec_index[s.id] = len(shdrs)
        shdrs.append(dict(name=elf_name(s.name), type=typ, flags=flags, data=b'' if typ == SHT_NOBITS else bytes(s.data),
                          size=s.size, link=0, info=0, align=min(max(s.align, 1), 4), entsize=0, lnk=s))
    # symbols: null, section symbols, locals, then globals
    syms = [(0, 0, 0, 0, 0)]  # (name, value, size, info, shndx)
    sec_sym = {}
    for s in emitted:
        sec_sym[s.id] = len(syms)
        syms.append((0, 0, 0, (STB_LOCAL << 4) | STT_SECTION, sec_index[s.id]))
    for n, sec, off in obj.locals:
        if sec in sec_index and n:
            syms.append((strtab.add(n), off, 0, (STB_LOCAL << 4) | STT_NOTYPE, sec_index[sec]))
    first_global = len(syms)
    starts = {}
    for n, sec, off in list(obj.xdefs.values()) + obj.locals:
        starts.setdefault(sec, set()).add(off)
    globals_ = {}
    for n, sec, off in sorted(obj.xdefs.values(), key=lambda x: (sec_index.get(x[1], 0), x[2], x[0])):
        if n in globals_:
            fail('%s: XDEF %s twice' % (what, n))
        s = obj.sections[sec]
        nxt = min([o for o in starts.get(sec, ()) if o > off] + [s.size])
        typ = STT_FUNC if s.name == '.text' else STT_OBJECT
        globals_[n] = len(syms)
        syms.append((strtab.add(n), off, nxt - off, (STB_GLOBAL << 4) | typ, sec_index[sec]))
    imports = {}

    def import_sym(name):
        name = rename.get(name, name)
        if name in globals_:
            return globals_[name]
        if name not in imports:
            imports[name] = len(syms)
            syms.append((strtab.add(name), 0, 0, (STB_GLOBAL << 4) | STT_NOTYPE, 0))
        return imports[name]

    # relocations
    rels = {}
    counts = {}
    for s, off, kind, term, k, e in ps:
        h = shdrs[sec_index[s.id]]
        data = bytearray(h['data']) if not isinstance(h['data'], bytearray) else h['data']
        h['data'] = data
        w, = struct.unpack_from('<I', data, off & ~3)
        if off % 4:
            fail('%s: unaligned patch at %s+%#x' % (what, s.name, off))
        if term is None:
            nw = P.patch_apply(kind, w, k)
            if kind == 100:
                fail('%s: constant gprel16 at %s+%#x' % (what, s.name, off))
            struct.pack_into('<I', data, off, nw)
            counts['constant'] = counts.get('constant', 0) + 1
            continue
        if term[0] == 'sec' and obj.sections[term[1]].name in extern:
            symidx = import_sym(extern[obj.sections[term[1]].name])
        elif term[0] == 'sec':
            symidx = sec_sym[term[1]]
        else:
            if term[1].startswith(('sectstart(', 'sectend(')) and term[1] not in rename:
                fail('%s: %s needs a name (rename it to the address label)' % (what, term[1]))
            symidx = import_sym(term[1])
        rtype = RTYPE[kind]
        if rtype == R_MIPS_32:
            nw = k
        elif rtype == R_MIPS_26:
            if k % 4:
                fail('%s: jump target constant %#x not a multiple of 4 at %s+%#x' % (what, k, s.name, off))
            nw = (w & 0xFC000000) | ((k >> 2) & 0x03FFFFFF)
        elif rtype == R_MIPS_HI16:
            nw = (w & 0xFFFF0000) | (((k + 0x8000) >> 16) & 0xFFFF)
        elif rtype == R_MIPS_LO16:
            nw = (w & 0xFFFF0000) | (k & 0xFFFF)
        else:  # GPREL16 against a symbol: in-place addend
            if not -0x8000 <= P.sext16(k & 0xFFFF) == (k if k < 0x80000000 else k - 0x100000000):
                fail('%s: gprel16 constant %#x out of range' % (what, k))
            nw = (w & 0xFFFF0000) | (k & 0xFFFF)
        struct.pack_into('<I', data, off, nw)
        rels.setdefault(s.id, []).append((off, rtype, symidx, k))
        counts[P.PATCH_NAME[kind]] = counts.get(P.PATCH_NAME[kind], 0) + 1

    rel_shdrs = []
    for s in emitted:
        if s.id in rels:
            table = order_hilo(rels[s.id], '%s %s' % (what, s.name))
            blob = b''.join(struct.pack('<II', o, (si << 8) | t) for o, t, si, _ in table)
            rel_shdrs.append(dict(name='.rel' + elf_name(s.name), type=SHT_REL, flags=SHF_INFO_LINK, data=blob,
                                  size=len(blob), link=None, info=sec_index[s.id], align=4, entsize=8))
    shdrs += rel_shdrs
    symtab_index = len(shdrs)
    sym_blob = b''.join(struct.pack('<IIIBBH', n, v, sz, info, 0, shn) for n, v, sz, info, shn in syms)
    shdrs.append(dict(name='.symtab', type=SHT_SYMTAB, flags=0, data=sym_blob, size=len(sym_blob),
                      link=symtab_index + 1, info=first_global, align=4, entsize=16))
    shdrs.append(dict(name='.strtab', type=SHT_STRTAB, flags=0, data=bytes(strtab.data), size=len(strtab.data),
                      link=0, info=0, align=1, entsize=0))
    for h in rel_shdrs:
        h['link'] = symtab_index
    shdrs.append(dict(name='.shstrtab', type=SHT_STRTAB, flags=0, data=None, size=0, link=0, info=0, align=1, entsize=0))
    for h in shdrs:
        h['name_off'] = shstr.add(h['name'])
    shdrs[-1]['data'] = bytes(shstr.data)
    shdrs[-1]['size'] = len(shstr.data)

    body = bytearray(52)
    for h in shdrs[1:]:
        if h['type'] == SHT_NOBITS:
            h['offset'] = len(body)
            continue
        al = max(h['align'], 1)
        body += b'\0' * ((-len(body)) % al)
        h['offset'] = len(body)
        body += bytes(h['data'])
    body += b'\0' * ((-len(body)) % 4)
    shoff = len(body)
    for h in shdrs:
        body += struct.pack('<10I', h['name_off'], h['type'], h['flags'], 0, h.get('offset', 0), h['size'],
                            h['link'], h['info'], h['align'], h['entsize'])
    ident = b'\x7fELF\x01\x01\x01' + b'\0' * 9
    struct.pack_into('<16sHHIIIIIHHHHHH', body, 0, ident, 1, 8, 1, 0, 0, shoff, EF_MIPS_NOREORDER, 52, 0, 0, 40,
                     len(shdrs), len(shdrs) - 1)
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    tmp = out_path.with_suffix('.tmp')
    tmp.write_bytes(bytes(body))
    tmp.replace(out_path)
    return dict(module=what, sections=[dict(lnk=s.name, elf=elf_name(s.name), size=s.size,
                                            nobits=SECTION_KIND[s.name][1] == SHT_NOBITS) for s in emitted],
                globals={n: dict(section=elf_name(obj.sections[sec].name), offset=off)
                         for n, sec, off in obj.xdefs.values()},
                imports=sorted(imports), relocations=counts, externalized=extern, omitted=sorted(omit))


def referenced_sections(obj):
    """LNK section ids that some patch of the module refers to."""
    return {term[1] for s, off, kind, term, k, e in patches(obj) if term and term[0] == 'sec'}


def cached_convert(mod, extern=None, omit=(), rename=None, version='33'):
    """Convert into work/sdk_lib/ (gitignored), keyed by the converter, the module bytes and the options."""
    key = hashlib.sha256(json.dumps(dict(c=CONVERTER_SHA, raw=hashlib.sha256(mod.raw).hexdigest(), v=version,
                                         extern=sorted((extern or {}).items()), omit=sorted(omit),
                                         rename=sorted((rename or {}).items())), sort_keys=True).encode()).hexdigest()[:16]
    stem = '%s_%s' % (Path(mod.lib).stem.upper(), mod.name.upper())
    out = CACHE / ('%s_%s.o' % (stem, key))
    info_path = out.with_suffix('.json')
    if out.is_file() and info_path.is_file():
        return out, json.loads(info_path.read_text(encoding='utf-8'))
    info = convert(mod, out, extern, omit, rename)
    info_path.write_text(json.dumps(info, indent=1) + '\n', encoding='utf-8')
    return out, info


# ---------------------------------------------------------------------------------------------------------
# CLI

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('lib', nargs='?', help='LIB name (LIBETC) or path, or a .OBJ')
    ap.add_argument('module', nargs='?')
    ap.add_argument('-o', '--output', help='ELF object to write (must be under work/)')
    ap.add_argument('--version', default='33')
    ap.add_argument('--exact', action='store_true', help='convert every exact module of sdk_modules.tsv into work/sdk_lib/')
    a = ap.parse_args()
    if a.exact:
        n, nbytes = 0, 0
        for r in exact_modules(a.version):
            mod = load_module(r['lib'], r['module'], a.version)
            ref = referenced_sections(mod.obj)
            omit = [s.name for s in mod.obj.sections.values()
                    if s.size and s.name != '.text' and r['data'].get(s.name, (None,))[0] is None and s.id not in ref]
            names = {term[1] for s, off, kind, term, k, e in patches(mod.obj) if term and term[0] == "name"}
            rename = {nm: '__psyq_' + nm.replace('(', '_').replace(')', '').replace('.', '') for nm in names
                      if nm.startswith(('sectstart(', 'sectend('))}
            out, info = cached_convert(mod, omit=omit, rename=rename, version=a.version)
            n += 1
            nbytes += r['size']
            print('%-10s %-9s %s  %s' % (r['lib'], r['module'], out.relative_to(ROOT).as_posix(), json.dumps(info['relocations'])))
        print('%d modules, %d text bytes' % (n, nbytes))
        return
    if not (a.lib and a.module and a.output):
        ap.error('LIB MODULE -o OUT, or --exact')
    out = Path(a.output).resolve()
    if not out.is_relative_to((ROOT / 'work').resolve()):
        ap.error('the output must be under work/ (it holds Sony code; never commit it)')
    lib = Path(a.lib)
    if lib.is_file():
        mod = P.read_lib(lib)
        mod = next((m for m in mod if m.name.upper() == a.module.upper()), None) or fail('no module ' + a.module)
    else:
        mod = load_module(a.lib, a.module, a.version)
    names = {term[1] for s, off, kind, term, k, e in patches(mod.obj) if term and term[0] == "name"}
    rename = {nm: '__psyq_' + nm.replace('(', '_').replace(')', '').replace('.', '') for nm in names
              if nm.startswith(('sectstart(', 'sectend('))}
    print(json.dumps(convert(mod, out, rename=rename), indent=1))


if __name__ == '__main__':
    main()
