#!/usr/bin/env python3
"""Pointer census for a relocatable build: which stored words are addresses.

    py -3.14 tools/pointer_census.py [--build-dir DIR] [--no-sdk] [--check] [--verbose]

A relocatable build lets functions grow or move and data shift, so every
address the image stores must come from a symbol. This tool reads the retail
image (bin/SCUS_94306R.EXE, read only) and classifies every word of the data
segments (rodata, data, sdata) and of the unlisted inline-data gaps inside the
code segments whose value is an address in the image or in BSS
(800CDC54..801FAD48 inclusive), plus every word a compiled object or a PsyQ
library module relocates. It writes:

  config/US/data_relocs.tsv    address, value, kind, target_symbol, confidence, evidence
  config/US/code_abs_refs.tsv  code-side addresses with no relocation in the
                               source-only build (numbers in compiled objects,
                               and every reference made by code that is still
                               retail bytes there)

Kinds: jump_table (switch table entry, target inside a function), code_ptr
(function start), vtable (code_ptr in a C++ virtual table), string_ptr (start
of a NUL-terminated string), data_ptr (start of a labelled or unlabelled data
object), interior_ptr (inside a function, string or labelled object).

Evidence, strongest first:
  obj-reloc   an R_MIPS_32 relocation in a placed data section of a compiled
              unit object of the hybrid build (gen_link.plan) - exact;
  lib-patch   a 32-bit patch in a PsyQ .LIB module's data section placed by
              tools/psyq_lib.py (needs toolchain/psyq-sdk; --no-sdk skips) - exact;
  lw+jr       the table is loaded by `lw` and jumped through by `jr` in the
              function the entries point into (retail code scan) - exact for
              switch tables;
  code-ref    code builds the table's base address with lui/addiu (hi/lo);
  ida / ida-decl / src-decl
              IDA names (jpt_, off_), IDA's typed pointer declarations, and
              pointer-typed declarations of D_<addr> in src/semantic;
  run=N       N consecutive candidate words (tables);
  target      what the value points at (function start, string, label).

Confidence: certain = obj-reloc, lib-patch or lw+jr; likely = the target is a
function/string/label start (or jump-table target) and the word is part of a
table, code-referenced or declared a pointer; doubtful = anything else.

Read only on the image and the build objects; it compiles, links and runs
nothing. --check regenerates both TSVs in memory and fails if they differ.
"""
import argparse
import bisect
import collections
import csv
import hashlib
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, RETAIL, RETAIL_SHA256, CONFIG, read_layout, load_functions  # noqa: E402
from semantic_elf import read_elf  # noqa: E402

T_ADDR, BSS_END, GP = 0x800CDC54, 0x801FAD48, 0x801809D4
OUT_DATA = CONFIG / 'data_relocs.tsv'
OUT_CODE = CONFIG / 'code_abs_refs.tsv'
IDA_C = ROOT / 'reference' / 'ida' / 'SCUS_94306R.EXE.c'

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16 = 2, 4, 5, 6, 7
LOADS = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26}
LO_OPS = {0x08, 0x09, 0x0D} | LOADS | {0x28, 0x29, 0x2A, 0x2B, 0x2E, 0x32, 0x3A}


def in_image(v):
    return T_ADDR <= v <= BSS_END


def sext(v):
    return v - 0x10000 if v & 0x8000 else v


# ---------------------------------------------------------------- the image
class Image:
    def __init__(self):
        raw = RETAIL.read_bytes()
        if hashlib.sha256(raw).hexdigest() != RETAIL_SHA256:
            raise SystemExit('pointer_census: %s is not the known retail image' % RETAIL)
        self.data = raw[0x800:]
        self.end = T_ADDR + len(self.data)

    def word(self, va):
        if not T_ADDR <= va <= self.end - 4:
            return 0
        return struct.unpack_from('<I', self.data, va - T_ADDR)[0]

    def byte(self, va):
        return self.data[va - T_ADDR] if T_ADDR <= va < self.end else 0

    def string_at(self, va, limit=512):
        """The printable NUL-terminated string starting at va, or None."""
        out = bytearray()
        for a in range(va, min(va + limit, self.end)):
            b = self.byte(a)
            if b == 0:
                return out.decode('latin-1') if out else None
            if not (0x20 <= b < 0x7F or b in (9, 10, 13)):
                return None
            out.append(b)
        return None


# ------------------------------------------------------------ MIPS scanning
def dest_reg(w):
    """GPR an instruction writes (None if none)."""
    op = w >> 26
    if op == 0:
        f = w & 63
        if f in (8, 0x0C, 0x0D, 0x11, 0x13, 0x18, 0x19, 0x1A, 0x1B):
            return None
        return (w >> 11) & 31
    if op == 3:
        return 31
    if op == 1:
        return 31 if (w >> 16) & 31 in (0x10, 0x11) else None
    if 8 <= op <= 0x0F or op in LOADS:
        return (w >> 16) & 31
    if op in (0x10, 0x12) and (w >> 21) & 31 in (0, 2):
        return (w >> 16) & 31
    return None


def branch_target(pc, w):
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    if op in (4, 5, 6, 7) or (op == 1 and rt in (0, 1, 0x10, 0x11)) or (op in (0x10, 0x11, 0x12, 0x13) and rs == 8):
        return (pc + 4 + (sext(w & 0xFFFF) << 2)) & 0xFFFFFFFF
    return None


def scan_code(start, end, word, reloc=None, lo=None, hi=None):
    """Scan [start, end) linearly. reloc(pc) -> set of relocation types at pc
    (objects) or None (retail bytes: nothing is relocated). Returns
    (refs, jump_tables) where refs are dicts with pc, kind, value, extra and
    jump_tables are (table, jr pc). Only references without a relocation are
    reported; [lo, hi) bounds intra-unit branches (default [start, end))."""
    lo, hi = (start if lo is None else lo), (end if hi is None else hi)
    rel = reloc or (lambda pc: ())
    gp_reused = any(dest_reg(word(pc)) == 28 for pc in range(start, end, 4))
    regs, refs, jts = {}, [], []
    for pc in range(start, end, 4):
        w = word(pc)
        r = rel(pc)
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        imm, s = w & 0xFFFF, sext(w & 0xFFFF)
        new = None
        if in_image(w) and R_MIPS_32 not in r:
            refs.append(dict(pc=pc, kind='word', value=w, extra=''))
        elif op in (2, 3):
            if R_MIPS_26 not in r:
                tgt = ((pc + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                refs.append(dict(pc=pc, kind='jal26' if op == 3 else 'j26', value=tgt, extra=''))
        else:
            bt = branch_target(pc, w)
            if bt is not None and not lo <= bt < hi:
                refs.append(dict(pc=pc, kind='branch_xfunc', value=bt, extra='pc-relative branch leaves its function'))
        if op == 0x0F:
            if R_MIPS_HI16 not in r:
                new = ('hi', imm << 16, pc)
        elif op in LO_OPS and rs in regs and R_MIPS_LO16 not in r:
            st = regs[rs]
            off = imm if op == 0x0D else s
            if st[0] == 'hi':
                val = (st[1] + off) & 0xFFFFFFFF
                if in_image(val):
                    refs.append(dict(pc=st[2], kind='hi16_lo16', value=val, extra='lo at %08X' % pc))
                if op in (0x08, 0x09, 0x0D):
                    new = ('addr', val)
            elif st[0] == 'idx':
                val = (st[1] + off) & 0xFFFFFFFF
                if st[2] is not None and in_image(val):
                    refs.append(dict(pc=st[2], kind='hi16_lo16', value=val, extra='indexed, lo at %08X' % pc))
                if op == 0x23:
                    new = ('tbl', val)
        elif op in LO_OPS and rs == 28 and not gp_reused and R_MIPS_GPREL16 not in r:
            refs.append(dict(pc=pc, kind='gprel16', value=(GP + s) & 0xFFFFFFFF, extra=''))
        elif op == 0 and (w & 63) == 0x21:
            for a in (rs, rt):
                st = regs.get(a)
                if st and st[0] == 'hi':
                    new = ('idx', st[1], st[2])
                elif st and st[0] == 'addr':
                    new = ('idx', st[1], None)
        elif op == 0 and (w & 63) == 8:
            st = regs.get(rs)
            if st and st[0] == 'tbl':
                jts.append((st[1], pc))
        d = dest_reg(w)
        if d:
            if new is not None:
                regs[d] = new
            else:
                regs.pop(d, None)
    return refs, jts


# ------------------------------------------------------------------ inputs
def read_tsv(path):
    with open(path, newline='') as f:
        return [r for r in csv.DictReader((l for l in f if not l.startswith('#')), delimiter='\t')]


class Labels:
    """Address -> preferred name; extents of objects/functions for interiors."""

    def __init__(self):
        self.by_addr = {}       # va -> (rank, name)
        self.extent = {}        # va -> end (exclusive) when known

    def add(self, va, name, rank, size=0):
        old = self.by_addr.get(va)
        if old is None or rank < old[0]:
            self.by_addr[va] = (rank, name)
        if size:
            self.extent[va] = max(self.extent.get(va, 0), va + size)

    def finish(self):
        self.keys = sorted(self.by_addr)

    def exact(self, va):
        x = self.by_addr.get(va)
        return x[1] if x else None

    def containing(self, va):
        """(start, name) of the labelled object with a known extent holding va."""
        i = bisect.bisect_right(self.keys, va) - 1
        while i >= 0 and va - self.keys[i] < 0x10000:
            k = self.keys[i]
            if k in self.extent and va < self.extent[k]:
                return k, self.by_addr[k][1]
            if k not in self.extent:
                return None
            i -= 1
        return None

    def nearest(self, va):
        i = bisect.bisect_right(self.keys, va) - 1
        return (self.keys[i], self.by_addr[self.keys[i]][1]) if i >= 0 else None


def ida_pointer_decls():
    """[(start, end_or_None, decl)] for IDA's pointer-typed data declarations."""
    out = []
    if not IDA_C.is_file():
        return out
    rx = re.compile(r'^([A-Za-z_][\w ]*\*+)\s*\(?\*?\s*(\w+_([0-9A-F]{8}))\)?(?:\[(\d*)\])?\s*[=;]')
    started = False
    with open(IDA_C, encoding='latin-1') as f:
        for line in f:
            if line.startswith('// Data declarations'):
                started = True
                continue
            if not started:
                continue
            if line.startswith('//----- ('):
                break
            m = rx.match(line)
            if m:
                va = int(m.group(3), 16)
                n = m.group(4)
                end = va + 4 * int(n) if n else (va + 4 if m.group(4) is None else None)
                out.append((va, end, '%s %s%s' % (m.group(1).strip(), m.group(2), '[%s]' % n if n is not None else '')))
    return out


def src_pointer_decls():
    """[(start, end_or_None, 'file:line')] for pointer-typed D_<addr> declarations in src/semantic."""
    rx = re.compile(r'^\s*(?:extern\s+|static\s+)?(?:const\s+)?[A-Za-z_][\w ]*\*\s*(?:const\s+)?'
                    r'D_([0-9A-Fa-f]{8})\s*((?:\[[^\]]*\])*)\s*[;=]')
    out = []
    for path in sorted((ROOT / 'src' / 'semantic').rglob('*')):
        if path.suffix not in ('.c', '.cpp', '.h'):
            continue
        for no, line in enumerate(path.read_text(encoding='latin-1').splitlines(), 1):
            m = rx.match(line)
            if not m:
                continue
            va = int(m.group(1), 16)
            dims = re.findall(r'\[([^\]]*)\]', m.group(2))
            n = 1
            for d in dims:
                try:
                    n *= int(d, 0)
                except ValueError:
                    n = None
                    break
            out.append((va, None if n is None else va + 4 * n, '%s:%d' % (path.relative_to(ROOT).as_posix(), no)))
    return out


def sdk_function_modules():
    """function VA -> 'LIB:MODULE' from config/US/sdk_modules.tsv."""
    out = {}
    for r in read_tsv(CONFIG / 'sdk_modules.tsv'):
        for va in r['functions'].split():
            out[int(va, 16)] = '%s:%s' % (r['lib'], r['module'])
    return out


def sdk_data_patches():
    """[(address, expr, 'LIB:MODULE section')] for 32-bit patches in placed PsyQ data sections, plus xdefs."""
    import psyq_lib as P
    if not P.SDK.is_dir():
        return None, {}
    img = P.Image()
    _, _, placed, _, _ = P.run_match(img)
    out, xdefs = [], {}
    for p in placed:
        o = p.obj
        for n, sec, off in list(o.xdefs.values()) + list(o.locals):
            if sec in p.base:
                xdefs.setdefault(p.base[sec] + off, n)
        for s in o.sections.values():
            if s.id == p.text.id or s.id not in p.base or not s.data:
                continue
            for off, kind, e in s.patches:
                if kind == 16:
                    out.append((p.base[s.id] + off, o.expr_str(e), '%s:%s %s' % (p.mod.libname, p.mod.name, s.name)))
    return out, xdefs


def object_relocs(elf):
    """{target section index: [(offset, type, symbol index)]} of an ELF object."""
    out = {}
    for s in elf['sections']:
        if s['type'] != 9:
            continue
        lst = out.setdefault(s['info'], [])
        for off in range(0, s['size'], 8):
            ro, ri = struct.unpack_from('<II', s['data'], off)
            lst.append((ro, ri & 0xFF, ri >> 8))
    return out


# ------------------------------------------------------------------ census
def census(build_dir=None, use_sdk=True, log=print):
    from gen_link import plan, last_build_dir
    from build_maximal_hybrid import symbol_map
    img = Image()
    layout = read_layout()
    seg_of = lambda va: next((s['name'] for s in layout if s['start'] <= va < s['end']), None)
    funcs = load_functions()                       # va -> (end, name)
    fstarts = sorted(funcs)

    def func_at(va):
        i = bisect.bisect_right(fstarts, va) - 1
        if i >= 0 and va < funcs[fstarts[i]][0]:
            return fstarts[i]
        return None

    pl = plan(build_dir or last_build_dir())
    log('plan: %d object sections from %s' % (len(pl['sections']), pl['build_dir'].relative_to(ROOT).as_posix()))

    # labels
    labels = Labels()
    for r in read_tsv(CONFIG / 'ida_names.tsv'):
        labels.add(int(r['va'], 16), r['name'], 2)
    for va, (end, name) in funcs.items():
        labels.add(va, name, 2, end - va)
    for name, va in symbol_map().items():
        labels.add(va, name, 3)
    objsyms = {}
    for row in pl['sections']:
        for name, va in row['globals'].items():
            labels.add(va, name, 0)
            objsyms[name] = va
    sdk_patches, sdk_xdefs = (None, {})
    if use_sdk:
        sdk_patches, sdk_xdefs = sdk_data_patches()
        if sdk_patches is None:
            log('toolchain/psyq-sdk missing: no lib-patch evidence')
    for va, name in sdk_xdefs.items():
        labels.add(va, name, 1)
    labels.finish()
    # ELF symbol sizes give object extents (data tables, strings)
    elves = {}
    for row in pl['sections']:
        if row['obj'] not in elves:
            elves[row['obj']] = read_elf(ROOT / row['obj'])
    for row in pl['sections']:
        elf = elves[row['obj']]
        idx = next(s['index'] for s in elf['sections'] if s['name'] == row['section'])
        for sym in elf['symbols']:
            if sym['section_index'] == idx and sym['size'] and sym['name']:
                labels.extent[row['base'] + sym['address']] = row['base'] + sym['address'] + sym['size']

    def sym_for(v):
        name = labels.exact(v)
        if name:
            return name
        f = func_at(v)
        if f is not None:
            return '%s+0x%X' % (funcs[f][1], v - f)
        c = labels.containing(v)
        if c:
            return '%s+0x%X' % (c[1], v - c[0])
        if T_ADDR <= v < BSS_END and img.string_at(v) is not None and img.byte(v - 1) != 0:
            n = labels.nearest(v)
            if n and img.string_at(n[0]) is not None and v - n[0] < 512 and all(img.byte(a) for a in range(n[0], v)):
                return '%s+0x%X' % (n[1], v - n[0])
        return 'D_%08X' % v

    # ---- retail code scan: xrefs, jump tables, refs of retail-only code
    code_segs = [s for s in layout if s['kind'] == 'code']
    owned = set(pl['functions'])
    sdk_mod = sdk_function_modules()
    code_refs_by_base = collections.defaultdict(set)
    jump_tables = {}                                   # table -> (func, jr pc)
    retail_refs = []
    for va in fstarts:
        end, name = funcs[va]
        if not any(s['start'] <= va < s['end'] for s in code_segs):
            continue
        refs, jts = scan_code(va, end, img.word)
        for r in refs:
            if r['kind'] == 'hi16_lo16':
                code_refs_by_base[r['value']].add(va)
            if va not in owned:
                r['owner'] = name
                r['status'] = 'retail_sdk' if va in sdk_mod else 'retail_game'
                r['module'] = sdk_mod.get(va, '')
                retail_refs.append(r)
        for t, jr in jts:
            jump_tables.setdefault(t, (va, jr))

    # ---- data regions: data segments plus the unlisted gaps in code segments
    regions = [(s['name'], s['start'], s['end']) for s in layout if s['kind'] in ('rodata', 'data', 'sdata')]
    for s in code_segs:
        cur = s['start']
        for va in fstarts:
            if va < s['start'] or va >= s['end']:
                continue
            if va > cur:
                regions.append(('text_gap', cur, va))
            cur = max(cur, funcs[va][0])
        if cur < s['end']:
            regions.append(('text_gap', cur, s['end']))
    region_of = lambda va: next((n for n, a, b in regions if a <= va < b), seg_of(va))

    cand = {}                                          # address -> evidence list
    for n, a, b in regions:
        for va in range(a, b, 4):
            if in_image(img.word(va)):
                cand.setdefault(va, [])

    # ---- exact evidence: object relocations (and numbers without one)
    obj_numeric = []
    for row in pl['sections']:
        elf = elves[row['obj']]
        sec = next(s for s in elf['sections'] if s['name'] == row['section'])
        rels = object_relocs(elf).get(sec['index'], [])
        at = collections.defaultdict(set)
        for off, typ, si in rels:
            at[row['base'] + off].add(typ)
        if row['code']:
            base = row['base']
            data = sec['data']
            word = lambda pc, d=data, b=base: struct.unpack_from('<I', d, pc - b)[0]
            lo, hi = row['slot'] if row.get('slot') else (base, base + row['size'])
            refs, _ = scan_code(base, base + row['size'], word, reloc=lambda pc, a=at: a.get(pc, ()), lo=lo, hi=hi)
            for r in refs:
                if r['kind'] == 'branch_xfunc' and lo <= r['value'] < hi:
                    continue
                r.update(owner='%s %s' % (row['unit'], row['section']), status='object_numeric', module='')
                obj_numeric.append(r)
            continue
        if row['nobits']:
            continue
        syms = elf['symbols']
        for off, typ, si in rels:
            va = row['base'] + off
            if typ != R_MIPS_32:
                cand.setdefault(va, []).append('obj-reloc type %d?' % typ)
                continue
            sym = syms[si]
            addend = struct.unpack_from('<I', sec['data'], off)[0]
            if sym['type'] == 3:
                tsec = elf['sections'][sym['section_index']]['name']
                trow = next((x for x in pl['sections'] if x['obj'] == row['obj'] and x['section'] == tsec), None)
                linked = (trow['base'] + addend) & 0xFFFFFFFF if trow else None
                how = '%s+0x%X' % (tsec, addend)
            else:
                s0 = objsyms.get(sym['name'])
                if s0 is None:
                    s0 = next((a for a, (rk, nm) in labels.by_addr.items() if nm == sym['name']), None)
                linked = (s0 + addend) & 0xFFFFFFFF if s0 is not None else None
                how = sym['name'] + ('+0x%X' % addend if addend else '')
            ev = 'obj-reloc %s %s+0x%X -> %s' % (row['unit'], row['section'], off, how)
            if linked is not None and linked != img.word(va):
                ev += ' (object links %08X, retail %08X)' % (linked, img.word(va))
            cand.setdefault(va, []).append(ev)
        for off in range(0, len(sec['data']) - 3, 4):
            v = struct.unpack_from('<I', sec['data'], off)[0]
            va = row['base'] + off
            if in_image(v) and R_MIPS_32 not in at.get(va, ()):
                obj_numeric.append(dict(pc=va, kind='word', value=v, extra='data word without relocation',
                                        owner='%s %s' % (row['unit'], row['section']), status='object_numeric', module=''))
                cand.setdefault(va, []).append('object data holds this number with no relocation (%s)' % row['unit'])
    if sdk_patches:
        for va, expr, where in sdk_patches:
            cand.setdefault(va, []).append('lib-patch %s %s' % (where, expr))

    # ---- jump tables from the code scan: extents
    jt_entry = {}                                      # address -> (table, func, jr)
    for t, (f, jr) in sorted(jump_tables.items()):
        fend = funcs[f][0]
        va = t
        while True:
            v = img.word(va)
            if not f <= v < fend or (va != t and va in jump_tables):
                break
            jt_entry[va] = (t, f, jr)
            cand.setdefault(va, [])
            va += 4

    # ---- declarations
    decls = [(a, b, 'ida-decl ' + d) for a, b, d in ida_pointer_decls()] + \
            [(a, b, 'src-decl ' + d) for a, b, d in src_pointer_decls()]
    ckeys = sorted(cand)
    cset = set(ckeys)

    def run_of(va):
        """(start, entries, stride) of the longest table of candidate words through va
        (strides 4..32; stride 4 preferred)."""
        best = (va, 1, 4)
        for stride in (4, 8, 12, 16, 20, 24, 28, 32):
            a = va
            while a - stride in cset:
                a -= stride
            b = va
            while b + stride in cset:
                b += stride
            n = (b - a) // stride + 1
            if n > best[1] and (stride == 4 or n >= 3):
                best = (a, n, stride)
        return best

    is_fstart = lambda v: v in funcs

    def vtable_of(va):
        """gcc 2.x vtable (8-byte zero header, then {delta/index word, pfn} entries) holding
        the pfn word at va, as (vtable start, entries), or None."""
        if not is_fstart(img.word(va)) or img.word(va - 4) != 0 or region_of(va) != 'rodata':
            return None
        e = va
        while is_fstart(img.word(e - 8)) and img.word(e - 12) == 0:
            e -= 8
        if img.word(e - 8) or img.word(e - 12):
            return None
        n, x = 0, e
        while is_fstart(img.word(x)) and img.word(x - 4) == 0:
            n, x = n + 1, x + 8
        return e - 12, n

    rows = []
    for va in ckeys:
        v = img.word(va)
        ev = list(cand[va])
        exact = any(e.startswith(('obj-reloc', 'lib-patch')) for e in ev)
        rstart, rlen, stride = run_of(va)
        vt = vtable_of(va)
        if vt:
            rstart, rlen, stride = vt[0], vt[1], 8
            ev.append('gcc2 vtable %08X (%d entries, delta/index word 0 before each pfn)' % vt)
        jt = jt_entry.get(va)
        if jt:
            ev.append('lw+jr table %08X used at %08X in %s' % (jt[0], jt[2], funcs[jt[1]][1]))
        # code reference to the table base (nearest base at or below va, from one record before the run)
        base = next((b for b in range(va, rstart - stride - 4, -4) if b in code_refs_by_base), None)
        if base is not None:
            users = sorted(code_refs_by_base[base])
            ev.append('code-ref %08X by %s%s' % (base, funcs[users[0]][1], ' +%d' % (len(users) - 1) if len(users) > 1 else ''))
        lab = labels.nearest(va)
        if lab and lab[0] >= rstart and re.match(r'(jpt_|jtbl_|off_)', lab[1]):
            ev.append('ida %s' % lab[1])
        declared = False
        for a, b, d in decls:
            if a == va or (a <= va and (b is None and a >= rstart or b is not None and va < b)):
                ev.append(d)
                declared = True
        if rlen > 1 and not vt:
            ev.append('run=%d from %08X' % (rlen, rstart) + (' stride %d' % stride if stride != 4 else ''))
        # target
        tf = func_at(v)
        tstr = img.string_at(v)
        name = labels.exact(v)
        if not in_image(v):
            kind, tgood = 'data_ptr', exact
            ev.append('target outside the image (fixed address)')
        elif tf is not None and tf == v:
            kind, tgood = 'code_ptr', True
            ev.append('target function start')
        elif tf is not None:
            kind = 'jump_table' if (jt or any('.text' in e or 'tm2_' in e for e in ev if e.startswith('obj-reloc'))
                                   and region_of(va) == 'rodata') else 'interior_ptr'
            tgood = kind == 'jump_table' or name is not None
            ev.append('target inside %s' % funcs[tf][1])
        elif tstr is not None and (img.byte(v - 1) == 0 or name):
            kind, tgood = 'string_ptr', True
            ev.append('target string %s' % ascii(tstr[:24] + ('...' if len(tstr) > 24 else '')))
        elif (img.byte(v) == 0 and img.byte(v - 1) == 0 and v % 4 == 0 and seg_of(v) in ('rodata', 'sdata')
              and img.string_at(v + 1) is None):
            kind, tgood = 'string_ptr', True
            ev.append("target empty string ''")
        elif name or v in labels.extent:
            kind, tgood = 'data_ptr', True
            ev.append('target label %s' % (name or 'object'))
        else:
            c = labels.containing(v)
            if c or (tstr is not None):
                kind, tgood = 'interior_ptr', False
                ev.append('target inside %s' % (c[1] if c else 'a string'))
            else:
                kind, tgood = 'data_ptr', False
                ev.append('target unlabelled %s' % (seg_of(v) or '?'))
        if kind == 'code_ptr' and (vt or lab and re.search(r'(__vt|_vt\$|vtbl|vtable)', lab[1], re.I)):
            kind = 'vtable'
        if exact or jt:
            conf = 'certain'
        elif tgood and (rlen > 1 or vt or base is not None or declared or any(e.startswith('ida ') for e in ev)):
            conf = 'likely'
        elif rlen > 1 and (base is not None or declared):
            conf = 'likely'                            # unlabelled target, but a member of a code-referenced/declared table
        else:
            conf = 'doubtful'
        if (v & 3) and kind in ('code_ptr', 'jump_table') and not exact:
            conf = 'doubtful'
        rows.append(dict(address=va, value=v, kind=kind, target=sym_for(v), confidence=conf,
                         evidence='; '.join(ev), region=region_of(va)))
    code_rows = sorted(obj_numeric + retail_refs, key=lambda r: (r['pc'], r['kind'], r['value']))
    for r in code_rows:
        r['target'] = sym_for(r['value'])
    return rows, code_rows


# ------------------------------------------------------------------ output
DATA_HEADER = (
    '# Generated by `py -3.14 tools/pointer_census.py`; do not edit. Pointer census of the US\n'
    '# retail image (SCUS_94306) for a relocatable build: every stored word that is an address\n'
    '# (value in 800CDC54..801FAD48) in rodata, data, sdata and the inline-data gaps of the code\n'
    '# segments, plus every word a compiled unit object or a PsyQ library module relocates.\n'
    '# kind: jump_table, code_ptr, vtable, string_ptr, data_ptr, interior_ptr.\n'
    '# confidence: certain (relocated by an object/LIB, or a lw+jr switch table), likely, doubtful.\n'
    '# evidence: see the docstring of tools/pointer_census.py and docs/SOURCE_BUILD.md.\n')
CODE_HEADER = (
    '# Generated by `py -3.14 tools/pointer_census.py`; do not edit. Code-side addresses that no\n'
    '# relocation covers in the source-only build. status: object_numeric = a compiled unit\n'
    '# object encodes the address as a number (asm operand, .word, numeric C); retail_game /\n'
    '# retail_sdk = the code is still retail bytes there (unselected function), so every\n'
    '# reference it makes is a number (retail_sdk: the PsyQ .LIB link will relocate it).\n'
    '# kind: hi16_lo16 (address = the lui; extra names the lo), j26, jal26, gprel16 (gp-relative,\n'
    '# moves with sdata/sbss), branch_xfunc (pc-relative branch into another function), word.\n')


def render(rows, code_rows):
    d = [DATA_HEADER, 'address\tvalue\tkind\ttarget_symbol\tconfidence\tevidence\n']
    for r in rows:
        d.append('%08X\t%08X\t%s\t%s\t%s\t%s\n' % (r['address'], r['value'], r['kind'], r['target'],
                                                     r['confidence'], r['evidence']))
    c = [CODE_HEADER, 'address\tvalue\tkind\ttarget_symbol\towner\tstatus\textra\n']
    for r in code_rows:
        owner = r['owner'] + (' [%s]' % r['module'] if r.get('module') else '')
        c.append('%08X\t%08X\t%s\t%s\t%s\t%s\t%s\n' % (r['pc'], r['value'], r['kind'], r['target'], owner,
                                                         r['status'], r['extra']))
    return ''.join(d), ''.join(c)


def summary(rows, code_rows, verbose=False):
    out = []
    per = collections.Counter((r['region'], r['kind'], r['confidence']) for r in rows)
    regions = sorted({r['region'] for r in rows})
    out.append('data words: %d' % len(rows))
    for reg in regions:
        ks = collections.Counter()
        for (rg, k, c), n in per.items():
            if rg == reg:
                ks['%s/%s' % (k, c)] += n
        out.append('  %-9s %s' % (reg, ', '.join('%s %d' % kv for kv in sorted(ks.items()))))
    doubt = [r for r in rows if r['confidence'] == 'doubtful']
    out.append('doubtful: %d' % len(doubt))
    for r in doubt[:None if verbose else 40]:
        out.append('  %08X -> %08X %s %s | %s' % (r['address'], r['value'], r['kind'], r['target'], r['evidence']))
    cc = collections.Counter((r['status'], r['kind']) for r in code_rows)
    out.append('code-side addresses without relocation: %d' % len(code_rows))
    for (st, k), n in sorted(cc.items()):
        out.append('  %-15s %-13s %d' % (st, k, n))
    for r in code_rows:
        if r['status'] != 'retail_sdk' and (verbose or r['status'] == 'object_numeric'):
            out.append('  %08X %-12s %08X %s  [%s] %s' % (r['pc'], r['kind'], r['value'], r['target'], r['owner'], r['extra']))
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build-dir', type=Path, help='hybrid build directory (default: LAST_BUILD.json)')
    ap.add_argument('--no-sdk', action='store_true', help='skip the PsyQ .LIB evidence (faster)')
    ap.add_argument('--check', action='store_true', help='fail if the TSVs are not up to date')
    ap.add_argument('--verbose', action='store_true')
    a = ap.parse_args()
    rows, code_rows = census(a.build_dir, use_sdk=not a.no_sdk)
    data_txt, code_txt = render(rows, code_rows)
    if a.check:
        bad = [p.name for p, t in ((OUT_DATA, data_txt), (OUT_CODE, code_txt))
               if not p.is_file() or p.read_text(encoding='ascii') != t]
        if bad:
            raise SystemExit('pointer_census: out of date: ' + ', '.join(bad))
        print('pointer_census: up to date')
        return
    OUT_DATA.write_text(data_txt, encoding='ascii', newline='\n')
    OUT_CODE.write_text(code_txt, encoding='ascii', newline='\n')
    print('wrote %s (%d rows), %s (%d rows)' % (OUT_DATA.relative_to(ROOT).as_posix(), len(rows),
                                                OUT_CODE.relative_to(ROOT).as_posix(), len(code_rows)))
    print(summary(rows, code_rows, a.verbose))


if __name__ == '__main__':
    main()
