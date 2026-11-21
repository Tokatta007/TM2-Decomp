#!/usr/bin/env python3
"""Reader for PsyQ / SN Systems LIB archives and LNK object files.

    py -3.14 tools/psyq_lib.py list <LIB>            modules, dates, exports
    py -3.14 tools/psyq_lib.py dump <LIB|OBJ> [module] sections, symbols, patches
    py -3.14 tools/psyq_lib.py dump ... --bytes      ... plus section hex (terminal only)
    py -3.14 tools/psyq_lib.py match [--out TSV]     place the 3.3/3.5 modules in the retail
                                                     image; writes config/US/sdk_modules.tsv
                                                     and prints coverage and per-unit lists

match reads bin/SCUS_94306R.EXE, config/US/{ida_funcs,extra_funcs,not_funcs,
ida_names}.tsv and, for the per-unit lists only, the last build's
image-report.json (LAST_BUILD.json). How a module is placed: its .text must
equal retail with every patch field masked; it is accepted when an exported
name sits at the matching retail address, or the match is unique and the
module is 32+ bytes or patch-free, or it touches an accepted module of the
same library; each module once, no overlaps, SDK block (80138000..) only.
Then every patch is evaluated against retail addresses (own sections, other
placed modules' symbols, IDA names, or the value the other modules agree on)
and the relocated words are compared: 'exact' means every word is equal.
Modules with no masked match whose names appear in retail are reported with a
rough diff: those are TM2's PsyQ 3.4 (Dec 1995) code that neither 3.3 nor 3.5
reproduces.

Written from scratch for this project from observation of the files. The
library files themselves are Sony's and live only in toolchain/psyq-sdk/
(gitignored); this tool never writes their bytes anywhere tracked.

LIB archive (all integers little-endian)
----------------------------------------
    "LIB" u8 version (1)
    repeated until EOF, one entry per module:
        char name[8]            space padded ("SYS", "C112", ...)
        u32  date               DOS date (low half) and time (high half)
        u32  hdr_size           bytes from entry start to the LNK object
        u32  size               total entry size (header + object)
        { u8 len; char name[len]; } ...  exported symbols, ended by len == 0
        LNK object              (size - hdr_size bytes)

LNK object
----------
    "LNK" u8 version (2), then a stream of records, each an opcode byte:

     0 end of object
     2 bytes          u16 n, n bytes into the current section
     6 switch section u16 section id
     8 zeroes         u32 n zero bytes (uninitialised space; .bss/.sbss growth)
    10 patch          u8 kind, u16 offset, expression; the offset counts from the
                      start of the last bytes record of the current section
    12 XDEF           u16 symbol id, u16 section, u32 offset, pstring name
    14 XREF           u16 symbol id, pstring name
    16 section decl   u16 id, u16 group, u8 alignment, pstring name
    18 local symbol   u16 section, u32 offset, pstring name
    20 group decl     u16 id, u8 type, pstring name
    28 file name      u16 file id, pstring name
    46 processor      u8 (7 = R3000)
    48 XBSS           u16 symbol id, u16 section, u32 size, pstring name (common)
    50 SLD line +1    u16 offset
    52 SLD line +b    u16 offset, u8 delta
    54 SLD line +w    u16 offset, u16 delta
    56 SLD set line   u16 offset, u32 line
    58 SLD line+file  u16 offset, u32 line, u16 file id
    60 SLD end        u16 offset
    74 function start u16 section, u32 offset, u16 file, u32 line, u16 frame reg,
                      u32 frame size, u16 return reg, u32 mask, u32 mask offset,
                      pstring name
    76 function end   u16 section, u32 offset, u32 line
    78 block start    u16 section, u32 offset, u32 line
    80 block end      u16 section, u32 offset, u32 line
    82 debug def      u16 section, u32 value, u16 class, u16 type, u32 size, pstring name
    84 debug def2     u16 section, u32 value, u16 class, u16 type, u32 size,
                      u16 ndims, u16 dims[ndims], pstring tag, pstring name

Patch expressions (prefix form):
     0 u32 constant          2 u16 symbol id (XDEF/XREF/XBSS)
     4 u16 section base: where this object's piece of the section lands
    12 u16 section start / 22 u16 section end: the whole linked output section
    32.. binary operators followed by two operands: 32 =, 34 <>, 36 <=, 38 <,
    40 >=, 42 >, 44 +, 46 -, 48 *, 50 /, 52 &, 54 !, 56 ^, 58 <<, 60 >>, 62 %
    (46 stores its operands reversed: `46 a b` is b - a; libcd ISO9660 +0x6F8 holds
    sectbase(.bss) + 0x600 - 0x24 = 80191AE8 in retail)

Patch kinds seen in the PsyQ 3.3/3.5 libraries (the linker overwrites the field;
any value the assembler left in it is not an addend):
    16 word32 (whole word)   74 jump target (low 26 bits, value >> 2)
    82 HI16 (lui; low 16 bits, carry-adjusted)   84 LO16 (low 16 bits)
   100 GPREL16 (low 16 bits, value - gp)   30 16-bit section-relative (libgte CLIP_INI)
"""
import argparse
import csv
import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SDK = ROOT / 'toolchain' / 'psyq-sdk'

PATCH_MASK = {16: 0xFFFFFFFF, 30: 0x0000FFFF, 74: 0x03FFFFFF, 82: 0x0000FFFF, 84: 0x0000FFFF, 100: 0x0000FFFF}
PATCH_NAME = {16: 'word32', 30: 'sect16', 74: 'jump26', 82: 'hi16', 84: 'lo16', 100: 'gprel16'}
BINOPS = {32: '=', 34: '<>', 36: '<=', 38: '<', 40: '>=', 42: '>', 44: '+', 46: '-',
          48: '*', 50: '/', 52: '&', 54: '!', 56: '^', 58: '<<', 60: '>>', 62: '%'}


class Reader:
    def __init__(self, data, pos=0):
        self.d, self.p = data, pos

    def u8(self):
        v = self.d[self.p]
        self.p += 1
        return v

    def u16(self):
        v, = struct.unpack_from('<H', self.d, self.p)
        self.p += 2
        return v

    def u32(self):
        v, = struct.unpack_from('<I', self.d, self.p)
        self.p += 4
        return v

    def pstr(self):
        n = self.u8()
        s = self.d[self.p:self.p + n].decode('latin-1')
        self.p += n
        return s

    def take(self, n):
        b = self.d[self.p:self.p + n]
        self.p += n
        return b


class Section:
    def __init__(self, sid, group, align, name):
        self.id, self.group, self.align, self.name = sid, group, align, name
        self.data = bytearray()     # initialised bytes (zero runs included)
        self.bss = 0                # size of uninitialised space (opcode 8 in bss sections)
        self.patches = []           # (offset, kind, expr)

    @property
    def size(self):
        return len(self.data) if self.data else self.bss


class LnkObject:
    """A parsed LNK object: sections, symbols and patches."""

    def __init__(self, data, name=''):
        self.name = name
        if data[:3] != b'LNK':
            raise ValueError(f'{name}: not an LNK object')
        self.version = data[3]
        self.sections = {}          # id -> Section (declaration order kept)
        self.xdefs = {}             # symbol id -> (name, section id, offset)
        self.xrefs = {}             # symbol id -> name
        self.xbss = {}              # symbol id -> (name, section id, size)
        self.locals = []            # (name, section id, offset)
        self.functions = []         # (name, section id, offset) from debug records
        self.files = {}
        self.unknown = []
        self._parse(Reader(data, 4))

    def _expr(self, r):
        op = r.u8()
        if op == 0:
            return ('const', r.u32())
        if op == 2:
            return ('sym', r.u16())
        if op == 4:
            return ('base', r.u16())
        if op == 12:
            return ('sect', r.u16())
        if op == 22:
            return ('send', r.u16())
        if op in BINOPS:
            a = self._expr(r)
            b = self._expr(r)
            return (BINOPS[op], a, b)
        raise ValueError(f'{self.name}: unknown expression op {op} at {r.p - 1:#x}')

    def _parse(self, r):
        cur = None
        chunk = {}                  # section id -> offset where the last bytes record began
        while True:
            op = r.u8()
            if op == 0:
                break
            elif op == 2:
                n = r.u16()
                chunk[cur.id] = len(cur.data)
                cur.data += r.take(n)
            elif op == 6:
                cur = self.sections[r.u16()]
            elif op == 8:
                n = r.u32()
                if cur.name in ('.bss', '.sbss') or not cur.data and cur.bss:
                    cur.bss += n
                else:
                    cur.data += bytes(n)
            elif op == 10:
                kind = r.u8()
                off = r.u16() + chunk.get(cur.id, 0)   # relative to the last bytes record
                cur.patches.append((off, kind, self._expr(r)))
            elif op == 12:
                sid = r.u16()
                sec = r.u16()
                off = r.u32()
                self.xdefs[sid] = (r.pstr(), sec, off)
            elif op == 14:
                sid = r.u16()
                self.xrefs[sid] = r.pstr()
            elif op == 16:
                sid = r.u16()
                grp = r.u16()
                al = r.u8()
                self.sections[sid] = Section(sid, grp, al, r.pstr())
            elif op == 18:
                sec = r.u16()
                off = r.u32()
                self.locals.append((r.pstr(), sec, off))
            elif op == 20:
                r.u16(); r.u8(); r.pstr()
            elif op == 28:
                fid = r.u16()
                self.files[fid] = r.pstr()
            elif op == 46:
                self.processor = r.u8()
            elif op == 48:
                sid = r.u16()
                sec = r.u16()
                size = r.u32()
                self.xbss[sid] = (r.pstr(), sec, size)
            elif op == 50:
                r.u16()
            elif op == 52:
                r.u16(); r.u8()
            elif op == 54:
                r.u16(); r.u16()
            elif op == 56:
                r.u16(); r.u32()
            elif op == 58:
                r.u16(); r.u32(); r.u16()
            elif op == 60:
                r.u16()
            elif op == 74:
                sec = r.u16(); off = r.u32()
                r.u16(); r.u32(); r.u16(); r.u32(); r.u16(); r.u32(); r.u32()
                self.functions.append((r.pstr(), sec, off))
            elif op in (76, 78, 80):
                r.u16(); r.u32(); r.u32()
            elif op == 82:
                r.u16(); r.u32(); r.u16(); r.u16(); r.u32(); r.pstr()
            elif op == 84:
                r.u16(); r.u32(); r.u16(); r.u16(); r.u32()
                nd = r.u16()
                for _ in range(nd):
                    r.u16()
                r.pstr(); r.pstr()
            else:
                raise ValueError(f'{self.name}: unknown LNK opcode {op} at {r.p - 1:#x}')

    # -- helpers ----------------------------------------------------------
    def section(self, name):
        for s in self.sections.values():
            if s.name == name:
                return s
        return None

    def symname(self, sid):
        if sid in self.xdefs:
            return self.xdefs[sid][0]
        if sid in self.xrefs:
            return self.xrefs[sid]
        if sid in self.xbss:
            return self.xbss[sid][0]
        return f'sym{sid:#x}'

    def expr_str(self, e):
        t = e[0]
        if t == 'const':
            return f'{e[1]:#x}'
        if t == 'sym':
            return self.symname(e[1])
        if t in ('sect', 'base', 'send'):
            s = self.sections.get(e[1])
            nm = s.name if s else f'{e[1]:#x}'
            return {'sect': f'sectstart({nm})', 'base': f'sectbase({nm})', 'send': f'sectend({nm})'}[t]
        if t == '-':   # stored reversed: `- a b` means b - a (libcd ISO9660 +0x6F8, checked against retail)
            return f'({self.expr_str(e[2])} - {self.expr_str(e[1])})'
        return f'({self.expr_str(e[1])} {t} {self.expr_str(e[2])})'

    def linear(self, e):
        """Reduce an expression to (term, constant), or None if it is not of
        that form. term is None (a constant), ('base', section id) for this
        object's own piece of a section (own XDEFs fold into it), or
        ('name', str) for anything the linker resolves globally: XREF/XBSS
        symbols and 'sectstart(.x)' / 'sectend(.x)' of a whole output section."""
        t = e[0]
        if t == 'const':
            return (None, e[1])
        if t == 'base':
            return (('base', e[1]), 0)
        if t == 'sym':
            if e[1] in self.xdefs:
                _, sec, off = self.xdefs[e[1]]
                return (('base', sec), off)
            return (('name', self.symname(e[1])), 0)
        if t in ('sect', 'send'):
            return (('name', self.expr_str(e)), 0)
        if t in ('+', '-'):
            a, b = self.linear(e[1]), self.linear(e[2])
            if a is None or b is None:
                return None
            if t == '+' and (a[0] is None or b[0] is None):
                return (a[0] or b[0], (a[1] + b[1]) & 0xFFFFFFFF)
            if t == '-' and a[0] is None:   # operands stored reversed: `- a b` is b - a
                return (b[0], (b[1] - a[1]) & 0xFFFFFFFF)
        return None

    def mask_words(self, sec):
        """{word offset: mask of bits a patch rewrites} for a section."""
        m = {}
        for off, kind, _ in sec.patches:
            m[off & ~3] = m.get(off & ~3, 0) | PATCH_MASK.get(kind, 0xFFFFFFFF)
        return m


class LibModule:
    def __init__(self, name, date, exports, obj_bytes, lib=''):
        self.name, self.date, self.exports, self.raw, self.lib = name, date, exports, obj_bytes, lib
        self._obj = None

    @property
    def obj(self):
        if self._obj is None:
            self._obj = LnkObject(self.raw, f'{self.lib}:{self.name}')
        return self._obj

    def date_str(self):
        d, t = self.date & 0xFFFF, self.date >> 16
        return (f'{1980 + (d >> 9):04d}-{(d >> 5) & 15:02d}-{d & 31:02d} '
                f'{t >> 11:02d}:{(t >> 5) & 63:02d}:{(t & 31) * 2:02d}')


def read_lib(path):
    path = Path(path)
    data = path.read_bytes()
    if data[:3] != b'LIB':
        if data[:3] == b'LNK':        # a lone .OBJ: one pseudo-module
            o = LibModule(path.stem.upper(), 0, [], data, path.name)
            o.exports = [n for n, _, _ in o.obj.xdefs.values()]
            return [o]
        raise ValueError(f'{path}: not a PsyQ LIB')
    r = Reader(data, 4)
    mods = []
    while r.p < len(data):
        start = r.p
        name = r.take(8).decode('latin-1').rstrip()
        date = r.u32()
        hdr = r.u32()
        size = r.u32()
        exports = []
        while True:
            s = r.pstr()
            if not s:
                break
            exports.append(s)
        mods.append(LibModule(name, date, exports, data[start + hdr:start + size], path.name))
        r.p = start + size
    return mods


# ---------------------------------------------------------------------------
# CLI: list / dump
# ---------------------------------------------------------------------------

def cmd_list(args):
    for m in read_lib(args.lib):
        o = m.obj
        sizes = ' '.join(f'{s.name}={s.size:#x}' for s in o.sections.values() if s.size)
        print(f'{m.name:8} {m.date_str()}  {sizes}')
        for e in m.exports:
            print(f'    {e}')


def cmd_dump(args):
    mods = read_lib(args.lib)
    if args.module:
        mods = [m for m in mods if m.name.upper() == args.module.upper()]
        if not mods:
            sys.exit(f'no module {args.module}')
    for m in mods:
        o = m.obj
        print(f'== {m.lib}:{m.name}  {m.date_str()}  LNK v{o.version}')
        for f in o.files.values():
            print(f'  file {f}')
        for s in o.sections.values():
            print(f'  section {s.id:#06x} {s.name:8} align {s.align} size {s.size:#x}'
                  f'{" (bss)" if s.bss and not s.data else ""}  {len(s.patches)} patches')
        for sid, (n, sec, off) in sorted(o.xdefs.items(), key=lambda kv: (kv[1][1], kv[1][2])):
            print(f'  XDEF {n} = {o.sections[sec].name}+{off:#x}')
        for sid, (n, sec, size) in o.xbss.items():
            print(f'  XBSS {n} size {size:#x} in {o.sections[sec].name}')
        for n, sec, off in o.locals:
            print(f'  local {n} = {o.sections[sec].name}+{off:#x}')
        for n in o.xrefs.values():
            print(f'  XREF {n}')
        for s in o.sections.values():
            for off, kind, e in s.patches:
                print(f'  patch {s.name}+{off:#06x} {PATCH_NAME.get(kind, kind)} {o.expr_str(e)}')
            if args.bytes and s.data:
                for i in range(0, len(s.data), 16):
                    print(f'  {s.name}+{i:04x}: {s.data[i:i + 16].hex(" ")}')




# ---------------------------------------------------------------------------
# match: place library modules in the retail image
# ---------------------------------------------------------------------------

RETAIL = ROOT / 'bin' / 'SCUS_94306R.EXE'
CONFIG = ROOT / 'config' / 'US'
OUT_TSV = CONFIG / 'sdk_modules.tsv'
GP = 0x801809D4
SDK_LO, TEXT_END = 0x80138000, 0x801625B8     # SDK block .. end of retail .text
DATA_LO, DATA_HI = 0x800CE910, 0x801FAD48     # rodata .. end of bss
LIBS = ['LIBAPI', 'LIBC', 'LIBC2', 'LIBCD', 'LIBETC', 'LIBGPU', 'LIBGTE', 'LIBPRESS', 'LIBSN',
        'LIBSND', 'LIBSPU', 'LIBGS', 'LIBMATH', 'LIBCARD', 'LIBTAP', 'LIBCOMB', 'LIBGUN']
OBJS = ['2MBYTE', '8MBYTE', 'MALLOC', 'NONE2', 'NONE3']
DATA_SECTIONS = ('.rdata', '.data', '.sdata', '.sbss', '.bss')


class Image:
    def __init__(self, path=RETAIL):
        d = path.read_bytes()
        self.base, = struct.unpack_from('<I', d, 0x18)
        self.data = d[0x800:]
        self.end = self.base + len(self.data)

    def has(self, va, n=4):
        return self.base <= va and va + n <= self.end

    def word(self, va):
        return struct.unpack_from('<I', self.data, va - self.base)[0]


def load_funcs():
    """{va: (name, bytes)} from ida_funcs.tsv + extra_funcs.tsv minus not_funcs.tsv."""
    funcs = {}
    for name in ('ida_funcs.tsv', 'extra_funcs.tsv'):
        with open(CONFIG / name, newline='') as f:
            for r in csv.DictReader(f, delimiter='\t'):
                funcs.setdefault(int(r['va'], 16), (r['name'], int(r['words']) * 4))
    with open(CONFIG / 'not_funcs.tsv', newline='') as f:
        for r in csv.DictReader(f, delimiter='\t'):
            funcs.pop(int(r['va'], 16), None)
    return funcs


def load_names(funcs):
    """retail name -> VA (ida_names.tsv plus function names)."""
    names = {}
    with open(CONFIG / 'ida_names.tsv', newline='') as f:
        for r in csv.DictReader(f, delimiter='\t'):
            names.setdefault(r['name'], int(r['va'], 16))
    for va, (n, _) in funcs.items():
        names.setdefault(n, va)
    return names


def load_units():
    """{va: unit file name} from the last build's image report (read only)."""
    try:
        last = json.loads((ROOT / 'LAST_BUILD.json').read_text(encoding='utf-8'))
        rep = ROOT / Path(last['executable']).parent / 'image-report.json'
        r = json.loads(rep.read_text(encoding='utf-8'))
    except (OSError, KeyError, ValueError):
        return {}
    return {int(f['va'], 16): Path(u['source']).name for u in r['units'] for f in u['functions']}


def sext16(v):
    return v - 0x10000 if v & 0x8000 else v


def all_modules(versions=('33', '35')):
    out = []
    for ver in versions:
        d = SDK / f'psyq{ver}' / 'LIB'
        for lib in LIBS:
            p = d / f'{lib}.LIB'
            if p.exists():
                for m in read_lib(p):
                    m.ver, m.libname = ver, lib
                    out.append(m)
        for ob in OBJS:
            p = d / f'{ob}.OBJ'
            if p.exists():
                for m in read_lib(p):
                    m.ver, m.libname = ver, ob + '.OBJ'
                    out.append(m)
    return out


def masked_words(obj, sec):
    n = len(sec.data) // 4
    return struct.unpack_from(f'<{n}I', sec.data), obj.mask_words(sec)


def find_masked(img, obj, sec, lo, hi):
    """Every word-aligned retail address in [lo, hi) where a section's bytes
    match with patch fields masked."""
    if len(sec.data) < 8:
        return []
    words, masks = masked_words(obj, sec)
    n = len(words)
    best, run_s, best_s = 0, 0, 0
    for i in range(n + 1):          # longest run of unpatched words = search anchor
        if i == n or masks.get(i * 4):
            if i - run_s > best:
                best, best_s = i - run_s, run_s
            run_s = i + 1
    if best < 2 or not any(words[best_s:best_s + best]):
        return []
    anchor = sec.data[best_s * 4:(best_s + best) * 4]
    hits, data = [], img.data
    pos, endp = max(lo - img.base, 0), min(hi - img.base, len(data))
    while True:
        j = data.find(anchor, pos, endp)
        if j < 0:
            break
        pos = j + 1
        if j % 4:
            continue
        start = img.base + j - best_s * 4
        if start < lo or not img.has(start, n * 4):
            continue
        rw = struct.unpack_from(f'<{n}I', data, start - img.base)
        if all(not ((words[i] ^ rw[i]) & ~masks.get(i * 4, 0) & 0xFFFFFFFF) for i in range(n)):
            hits.append(start)
    return hits


def patch_value(kind, w, addr):
    """Decode the value a patch field holds in a retail word (None if unknown)."""
    if kind == 74:
        return ((addr + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
    if kind == 16:
        return w
    if kind == 100:
        return (GP + sext16(w & 0xFFFF)) & 0xFFFFFFFF
    return None


def patch_apply(kind, w, val):
    """Word w with the patch field replaced by val (None if unknown kind). The
    linker overwrites the field: whatever the assembler left there (often the
    target's offset inside the object) is not an addend."""
    if kind == 74:
        return (w & ~0x03FFFFFF) | ((val >> 2) & 0x03FFFFFF)
    if kind == 16:
        return val & 0xFFFFFFFF
    if kind == 82:
        return (w & 0xFFFF0000) | (((val + 0x8000) >> 16) & 0xFFFF)
    if kind == 84:
        return (w & 0xFFFF0000) | (val & 0xFFFF)
    if kind == 100:
        return (w & 0xFFFF0000) | ((val - GP) & 0xFFFF)
    return None


class Placement:
    """A module's .text placed at a retail address, its data sections located."""

    def __init__(self, mods, start):
        self.mods = mods                  # same (lib, module) from one or more versions
        self.mod = mods[0]
        self.obj = self.mod.obj
        self.text = self.obj.section('.text')
        self.start, self.size = start, self.text.size
        self.end = start + self.size
        self.base = {self.text.id: start}  # section id -> retail address of this piece
        self.conflicts = []
        self.data_status = {}

    key = property(lambda s: (s.mod.libname, s.mod.name))
    vers = property(lambda s: '+'.join(m.ver for m in s.mods))

    def observed(self, img):
        """(term, address, offset) for every text patch whose value can be read
        back from the retail bytes (hi16 paired with the next lo16 of the same
        expression)."""
        o, out, his = self.obj, [], {}
        for off, kind, e in self.text.patches:
            lin = o.linear(e)
            if lin is None or lin[0] is None:
                continue
            addr = self.start + off
            w = img.word(addr)
            if kind == 82:
                his[e] = w
                continue
            if kind == 84:
                if e not in his:
                    continue
                val = (((his[e] & 0xFFFF) << 16) + sext16(w & 0xFFFF)) & 0xFFFFFFFF
            else:
                val = patch_value(kind, w, addr)
                if val is None:
                    continue
            out.append((lin[0], (val - lin[1]) & 0xFFFFFFFF, off))
        return out

    def vote(self, img, gvotes):
        """Data section bases of this module, and global-name votes."""
        votes = {}
        for term, a, off in self.observed(img):
            if term[0] == 'base':
                if term[1] == self.text.id:
                    continue
                v = votes.setdefault(term[1], {})
            else:
                v = gvotes.setdefault(term[1], {})
            v[a] = v.get(a, 0) + 1
        for sid, v in votes.items():
            self.base[sid] = max(v, key=v.get)
            if len(v) > 1:
                self.conflicts.append(f'{self.obj.sections[sid].name} base: ' +
                                      ' '.join(f'{a:08X}' for a in v))

    def locate_data(self, img, taken):
        """Find data sections no text patch refers to by a masked search.
        Returns {section id: candidate addresses} for the ambiguous ones."""
        multi = {}
        for s in self.obj.sections.values():
            if s.id in self.base or s.name not in ('.rdata', '.data', '.sdata') or len(s.data) < 8:
                continue
            hits = [h for h in find_masked(img, self.obj, s, DATA_LO, img.end) if h not in taken]
            if len(hits) == 1:
                self.base[s.id] = hits[0]
                self.data_status[s.id] = 'searched'
            elif hits:
                multi[s.id] = hits
        return multi

    def value(self, e, resolve):
        lin = self.obj.linear(e)
        if lin is None:
            return None
        term, k = lin
        if term is None:
            return k
        a = self.base.get(term[1]) if term[0] == 'base' else resolve(term[1])
        return None if a is None else (a + k) & 0xFFFFFFFF

    def verify(self, img, resolve, sec=None):
        """Relocate a section against retail addresses and compare every word.
        Returns (masked-diff word offsets, relocation mismatches, unresolved)."""
        sec = sec or self.text
        base = self.base[sec.id]
        words, masks = masked_words(self.obj, sec)
        diff = [i * 4 for i, w in enumerate(words)
                if not img.has(base + i * 4)
                or (w ^ img.word(base + i * 4)) & ~masks.get(i * 4, 0) & 0xFFFFFFFF]
        bad, unres = [], 0
        for off, kind, e in sec.patches:
            val = self.value(e, resolve)
            nw = None if val is None else patch_apply(kind, words[off // 4], val)
            if nw is None:
                unres += 1
            elif img.has(base + off) and nw != img.word(base + off):
                bad.append((off, self.obj.expr_str(e)))
        return diff, bad, unres


def fuzzy(w):
    """Instruction word with relocatable fields dropped (for rough diffs)."""
    op = w >> 26
    if op in (2, 3):
        return op << 26
    if op >= 4:
        return w & 0xFFFF0000
    return w


def word_diff(a, b):
    import difflib
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    return sum(max(i2 - i1, j2 - j1) for tag, i1, i2, j1, j2 in sm.get_opcodes() if tag != 'equal')


def text_exports(mod):
    o = mod.obj
    t = o.section('.text')
    return [(n, off) for n, sec, off in o.xdefs.values() if t and sec == t.id]


def func_extents(mod):
    """[(name, offset, size)] of a module's text symbols (exports and locals)."""
    o, t = mod.obj, mod.obj.section('.text')
    syms = sorted({(off, n) for n, sec, off in list(o.xdefs.values()) + o.locals if sec == t.id})
    out = []
    for i, (off, n) in enumerate(syms):
        end = syms[i + 1][0] if i + 1 < len(syms) else t.size
        if end > off:
            out.append((n, off, end - off))
    return out


def run_match(img):
    funcs = load_funcs()
    names = load_names(funcs)
    mods = [m for m in all_modules() if m.obj.section('.text') and m.obj.section('.text').data]

    # 1. every masked hit of every module; versions with identical text merged
    cands = {}            # (lib, module, start) -> [modules]
    for m in mods:
        t = m.obj.section('.text')
        for h in find_masked(img, m.obj, t, img.base, TEXT_END):
            lst = cands.setdefault((m.libname, m.name, h), [])
            if not lst or lst[0].obj.section('.text').size == t.size:
                lst.append(m)
    per_key = {}
    for (lib, name, h) in cands:
        per_key.setdefault((lib, name), []).append(h)

    def anchored(ms, h):
        return any(names.get(n) == h + off for n, off in text_exports(ms[0]))

    # 2. accept, one placement per module, no overlaps, SDK block only; a module
    #    is trusted alone when it is 32+ bytes or has no patches at all (BIOS
    #    call stubs: li t2,table; jr t2; li t1,n)
    placed, used = [], set()

    def size_of(k):
        return cands[k][0].obj.section('.text').size

    def free(h, size):
        return all(h + size <= p.start or h >= p.end for p in placed)

    def accept(k):
        placed.append(Placement(cands[k], k[2]))
        used.add(k[:2])

    order = sorted(cands, key=lambda k: (-size_of(k), k[2]))

    def specific(k):     # big enough, or a patch-free BIOS stub, to trust alone
        return size_of(k) >= 0x20 or not cands[k][0].obj.section('.text').patches

    def adjacent(k):
        nb = [p for p in placed if p.end == k[2] or p.start == k[2] + size_of(k)]
        return any(p.mod.libname == k[0] for p in nb)

    passes = (
        # IDA names anchor the module (below the SDK block IDA names are not
        # trusted: game code there is named SsInitHot)
        lambda k: specific(k) and anchored(cands[k], k[2]),
        # the only place this module matches
        lambda k: specific(k) and len(per_key[k[:2]]) == 1,
        # touches an accepted module of the same library (repeated)
        adjacent,
        # tiny modules anchored by an IDA name (GsGetWorkBase there was really
        # libetc GetVideoMode: hence last)
        lambda k: anchored(cands[k], k[2]),
    )
    for rule in passes:
        changed = True
        while changed:
            changed = False
            for k in order:
                if (k[:2] not in used and k[2] >= SDK_LO and free(k[2], size_of(k))
                        and rule(k)):
                    accept(k)
                    changed = rule is adjacent
                    if changed:
                        break
    # modules too small for the byte search (PUREV is one word) or masked-equal
    # only at a name-anchored address
    for m in mods:
        if (m.libname, m.name) in used:
            continue
        t = m.obj.section('.text')
        for n, off, _ in func_extents(m):
            st = names.get(n, 0) - off
            if SDK_LO <= st and st + t.size <= TEXT_END and free(st, t.size):
                p = Placement([m], st)
                if not p.verify(img, lambda n: None)[0]:
                    k = (m.libname, m.name, st)
                    cands[k] = [x for x in mods if (x.libname, x.name) == k[:2] and
                                x.obj.section('.text').data == t.data] or [m]
                    accept(k)
                    break
    placed.sort(key=lambda p: p.start)

    # 3. data sections and globals from the retail values of the patches
    gvotes = {}
    for p in placed:
        p.vote(img, gvotes)
    taken = {b for p in placed for b in p.base.values()}
    groups = {}
    for p in placed:
        for sid, hits in p.locate_data(img, taken).items():
            groups.setdefault((bytes(p.obj.sections[sid].data), tuple(hits)), []).append((p, sid))
    # identical static tables (libcd's C_0xx modules each carry the same CD
    # register table): the linker lays .data out in module order, so the k-th
    # module gets the k-th copy when the counts agree
    for (_, hits), ps in groups.items():
        if len(ps) == len(hits):
            for (p, sid), h in zip(sorted(ps, key=lambda x: x[0].start), hits):
                p.base[sid] = h
                p.data_status[sid] = 'ordered'
        else:
            for p, sid in ps:
                p.data_status[sid] = f'{len(hits)} candidates'
    symtab = {}
    for p in placed:
        for n, sec, off in p.obj.xdefs.values():
            if sec in p.base:
                symtab.setdefault(n, p.base[sec] + off)
    consensus, conflicts = {}, []
    for n, v in gvotes.items():
        consensus[n] = max(v, key=v.get)
        if len(v) > 1:
            conflicts.append(f'{n}: ' + ' '.join(f'{a:08X}x{c}' for a, c in v.items()))
        known = symtab.get(n, names.get(n))
        if known is not None and known != consensus[n]:
            conflicts.append(f'{n}: code uses {consensus[n]:08X}, defined at {known:08X}')

    def resolve(n):
        for t in (symtab, names, consensus):
            if n in t:
                return t[n]
        return None

    for p in placed:
        p.diff, p.bad_relocs, p.unresolved = p.verify(img, resolve)
        for s in p.obj.sections.values():
            if s.id == p.text.id or s.name not in DATA_SECTIONS or not s.size:
                continue
            if s.id not in p.base:
                p.data_status.setdefault(s.id, 'unlocated')
            elif s.data:
                d, b, u = p.verify(img, resolve, s)
                st = 'eq' if not d and not b else f'diff {len(d)}w/{len(b)}r'
                how = p.data_status.get(s.id)
                p.data_status[s.id] = st + (f' ({how})' if how in ('searched', 'ordered') else '')
            else:
                p.data_status[s.id] = 'bss'

    # 4. modules with no masked match but exported names in retail: 3.4-only code
    starts = sorted(set(funcs) | {p.start for p in placed} | {TEXT_END})
    by_key = {}
    for m in mods:
        by_key.setdefault((m.libname, m.name), []).append(m)
    diffs = []
    for key, ms in by_key.items():
        if key in used:
            continue
        best = None
        for m in ms:
            t = m.obj.section('.text')
            sts = {names[n] - off for n, off, _ in func_extents(m)
                   if n in names and SDK_LO <= names[n] < TEXT_END
                   and not any(p.start <= names[n] < p.end for p in placed)}
            for st in sts:
                if not all(st >= p.end or st < p.start for p in placed):
                    continue
                lim = min([p.start for p in placed if p.start > st] + [st + 2 * t.size + 0x40, TEXT_END])
                ends = [s for s in starts if st < s <= lim] or [lim]
                end = min(ends, key=lambda s: abs(s - st - t.size))
                words, _ = masked_words(m.obj, t)
                nd = word_diff([fuzzy(w) for w in words], [fuzzy(img.word(a)) for a in range(st, end, 4)])
                if best is None or nd < best['diff']:
                    best = dict(diff=nd, ver=m.ver, start=st, end=end, size=t.size, mod=m, key=key)
        if best:
            diffs.append(best)
    # per-function check of the best candidate: exported/local functions whose
    # bytes (patch fields masked) equal retail at their retail name
    for d in diffs:
        m = d['mod']
        t = m.obj.section('.text')
        words, masks = masked_words(m.obj, t)
        eq, tot = [], 0
        for n, off, size in func_extents(m):
            va = names.get(n)
            if va is None:
                continue
            tot += 1
            if img.has(va, size) and all(
                    not ((words[(off + i) // 4] ^ img.word(va + i)) & ~masks.get(off + i, 0) & 0xFFFFFFFF)
                    for i in range(0, size, 4)):
                eq.append(n)
        d['funcs_eq'], d['funcs_named'] = eq, tot
    diffs.sort(key=lambda d: (d['diff'] / max(d['size'], 4), d['start']))
    kept = []
    for d in diffs:
        if all(d['end'] <= k['start'] or d['start'] >= k['end'] for k in kept):
            kept.append(d)
    kept.sort(key=lambda d: d['start'])
    for d in kept:
        d['funcs'] = funcs_in(funcs, d['start'], d['end'])

    # 5. remaining SDK-block functions whose retail name (IDA's _XXXXXXXX
    #    suffix dropped) is a text symbol of some library module: SDK code whose
    #    module boundaries moved between versions (libcd BIOS/CDREAD, libsnd
    #    SSINIT/SSSTART); added to that module's row
    covered = {v for p in placed for v in funcs_in(funcs, p.start, p.end)}
    covered |= {v for d in kept for v in d['funcs']}
    index = {}
    for m in mods:
        for n, off, size in func_extents(m):
            index.setdefault(n, []).append(m)
    rows = {d['key']: d for d in kept}
    for va in sorted(funcs):
        if va in covered or not SDK_LO <= va < TEXT_END:
            continue
        n = re.sub(r'_[0-9A-F]{8}$', '', funcs[va][0])
        if n not in index:
            continue
        m = index[n][0]
        key = (m.libname, m.name)
        if key in used:
            continue
        d = rows.get(key)
        if d is None:
            d = rows[key] = dict(diff=None, ver=m.ver, start=va, end=va, size=m.obj.section('.text').size,
                                 mod=m, key=key, funcs=[], funcs_eq=[], funcs_named=0)
            kept.append(d)
        d['funcs'].append(va)
    kept.sort(key=lambda d: d['start'])
    return funcs, names, placed, kept, conflicts


def funcs_in(funcs, lo, hi):
    return [va for va in sorted(funcs) if lo <= va < hi]


def cmd_match(args):
    img = Image()
    funcs, names, placed, diffs, conflicts = run_match(img)
    rows = []
    for p in placed:
        o = p.obj
        if p.diff:
            st = f'differs {len(p.diff)} words'
        elif p.bad_relocs:
            st = f'masked-equal ({len(p.bad_relocs)} relocs differ)'
        else:
            st = 'exact' + (f' ({p.unresolved} relocs unresolved)' if p.unresolved else '')
        data = []
        for s in o.sections.values():
            if s.id in p.data_status:
                b = p.base.get(s.id)
                data.append(f'{s.name}@{b:08X}:{p.data_status[s.id]}' if b is not None
                            else f'{s.name}({s.size:#x}):{p.data_status[s.id]}')
        rows.append((p.mod.libname, p.mod.name, p.vers, f'{p.start:08X}', f'{p.size:#x}', st,
                     ' '.join(f'{v:08X}' for v in funcs_in(funcs, p.start, p.end)), ' '.join(data)))
    for d in diffs:
        if d['diff'] is None:
            st = 'not found (matched by function names only)'
        else:
            st = (f'differs {d["diff"]} words (closest {d["ver"]}; retail {d["end"] - d["start"]:#x} bytes; '
                  f'{len(d["funcs_eq"])}/{d["funcs_named"]} named functions equal)')
        rows.append((d['key'][0], d['key'][1], 'none', f'{d["start"]:08X}', f'{d["size"]:#x}', st,
                     ' '.join(f'{v:08X}' for v in sorted(d['funcs'])), ''))
    rows.sort(key=lambda r: r[3])
    with open(args.out, 'w', newline='\n') as f:
        f.write('# Generated by `py -3.14 tools/psyq_lib.py match`; do not edit. PsyQ 3.3/3.5 library\n'
                '# modules placed in bin/SCUS_94306R.EXE (no library bytes here: names, addresses, sizes).\n'
                '# version: 33 / 35 / 33+35 = that SDK has a module whose .text equals retail with patch\n'
                '#   fields masked; none = no such module, the row is anchored by exported names (3.4-only).\n'
                '# match: exact = every word equal after relocating the module against retail addresses\n'
                '#   (unresolved = patches against symbols with no known retail address, masked only);\n'
                '#   masked-equal = equal with patch fields masked, some relocated values differ;\n'
                '#   differs N words = rough instruction diff against the closest version (relocatable\n'
                '#   fields ignored); retail bytes = extent assumed for the retail copy.\n'
                '# functions: retail function VAs (ida_funcs + extra_funcs) inside the row\'s range.\n'
                '# data: section@retail address:status; eq = bytes equal after relocation, diff Nw/Nr =\n'
                '#   N words / N relocated words differ, bss = uninitialised, unlocated = not found;\n'
                '#   (searched) = found by a unique byte search rather than through code references;\n'
                '#   (ordered) = one of several identical copies, assigned in module order;\n'
                '#   N candidates = identical copies that could not be assigned.\n')
        f.write('lib\tmodule\tversion\tretail_text_start\ttext_size\tmatch\tfunctions\tdata\n')
        for r in rows:
            f.write('\t'.join(r) + '\n')
    print(f'wrote {args.out} ({len(rows)} modules)')
    summarize(funcs, placed, diffs, conflicts)


def summarize(funcs, placed, diffs, conflicts):
    units = load_units()
    cov = {}
    for p in placed:
        st = 'diff' if p.diff else 'masked' if p.bad_relocs else 'exact'
        for va in funcs_in(funcs, p.start, p.end):
            cov[va] = (st, p.key, p.vers)
    for d in diffs:
        for va in d['funcs']:
            cov.setdefault(va, ('34only', d['key'], 'none'))
    sdk = [va for va in sorted(funcs) if SDK_LO <= va < TEXT_END]

    def nb(vs):
        return sum(funcs[v][1] for v in vs)

    def cls(c):
        return [v for v in sdk if cov.get(v, ('none',))[0] == c]
    print(f'functions {SDK_LO:08X}..{TEXT_END:08X}: {len(sdk)} ({nb(sdk)} bytes)')
    for c in ('exact', 'masked', 'diff', '34only', 'none'):
        print(f'  {c:7}: {len(cls(c)):4} ({nb(cls(c))} bytes)')
    for tag, sel in (('3.3', '33'), ('3.5', '35')):
        vs = [v for v in sdk if cov.get(v, ('',))[0] == 'exact' and sel in cov[v][2]]
        print(f'  exact in {tag}: {len(vs)} ({nb(vs)} bytes)')
    print('  module code below the SDK block:',
          ' '.join(f'{v:08X}:{cov[v][1]}' for v in sorted(cov) if v < SDK_LO) or 'none')
    print(f'placed {len(placed)}: 3.3 only', [p.key for p in placed if p.vers == '33'],
          '3.5 only', [p.key for p in placed if p.vers == '35'])
    print('conflicts:', *conflicts, sep='\n  ')
    for p in placed:
        if p.bad_relocs or p.conflicts:
            print('reloc issues', p.key, p.bad_relocs[:4], p.conflicts)
    # src/semantic units (from the last build's image report) holding SDK code
    byu = {}
    for v in sorted(funcs):
        byu.setdefault(units.get(v, '(no unit)'), []).append(v)
    full, mixed = [], []
    for u, vs in sorted(byu.items()):
        sdkv = [v for v in vs if v in cov]
        if sdkv:
            (full if len(sdkv) == len(vs) else mixed).append((u, sdkv, [v for v in vs if v not in cov]))

    def fmt(vs):
        return ' '.join(f'{v:08X}' + ('' if cov.get(v, ('exact',))[0] == 'exact' else '*') for v in vs)
    print()
    print('units whose every function is SDK code (* = 3.4-only, no 3.3/3.5 module):')
    for u, sdkv, _ in full:
        print(f'  {u} ({len(sdkv)}): {fmt(sdkv)}')
    print()
    print('mixed units: SDK functions | remaining (not in any library):')
    for u, sdkv, rest in mixed:
        print(f'  {u}: SDK ({len(sdkv)}) {fmt(sdkv)}')
        print(f'  {" " * len(u)}  rest ({len(rest)}) ' + ' '.join(f'{v:08X}:{funcs[v][0]}' for v in rest))
    print()
    print(f'SDK-block functions in no library module ({len(cls("none"))}), by unit:')
    nb_u = {}
    for v in cls('none'):
        nb_u.setdefault(units.get(v, '(no unit)'), []).append(v)
    for u, vs in sorted(nb_u.items()):
        print(f'  {u} ({len(vs)}): ' + ' '.join(f'{v:08X}' for v in vs))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    sub = ap.add_subparsers(dest='cmd', required=True)
    p = sub.add_parser('list', help='modules, dates and exports of a LIB')
    p.add_argument('lib')
    p.set_defaults(fn=cmd_list)
    p = sub.add_parser('dump', help='sections, symbols and patches of modules')
    p.add_argument('lib')
    p.add_argument('module', nargs='?')
    p.add_argument('--bytes', action='store_true', help='also hex-dump section bytes')
    p.set_defaults(fn=cmd_dump)
    p = sub.add_parser('match', help='place library modules in the retail image')
    p.add_argument('--out', default=str(OUT_TSV))
    p.set_defaults(fn=cmd_match)
    a = ap.parse_args()
    a.fn(a)


if __name__ == '__main__':
    main()
