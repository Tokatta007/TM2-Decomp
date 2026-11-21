#!/usr/bin/env python3
"""Relocation records, address maps and canonicalization for relocatable source-only links.

A relocatable link (tools/gen_link.py --relocatable, written by tools/build_source_only.py --relocatable)
leaves reloc_plan.json, the ELF and the EXE in its output directory. This module

  * reads every relocation of every linked input section (unit objects and the generated retail-piece
    objects) and computes the address it encodes in the candidate ("c") image: R_MIPS_32, R_MIPS_26,
    R_MIPS_HI16/LO16 (paired, as ld pairs them), R_MIPS_GPREL16;
  * builds the address map between retail ("r") and candidate coordinates (AddrMap, addrmap.tsv);
  * canonicalizes a candidate image: every mapped candidate range is copied to its retail position and
    every relocated field is rewritten with c2r(address), i.e. the image is "un-relocated";
  * writes addrmap.tsv, symbols.tsv and pointer_slots.tsv next to the EXE.

Port variant (docs/SOURCE_BUILD.md "Port variant"): the arrays moved to the port base get extra address-map
rows outside the tiling: `port` (the retail entries, linear), `port_new` (entries beyond retail's; candidate
only, r_start == r_end) and `port_relayout` (an array whose entry layout changed: only its start maps). c2r
and r2c look these up first, so a reference to a moved array un-relocates to its retail address and a retail
address of a moved array translates to its new home.

No emulator is involved; everything is static.
"""
import bisect
import csv
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, CONFIG, read_layout, load_functions
from semantic_elf import read_elf, read_relocs
from symbol_names import load_renames

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16 = 2, 4, 5, 6, 7
RELOC_NAMES = {2: 'R_MIPS_32', 4: 'R_MIPS_26', 5: 'R_MIPS_HI16', 6: 'R_MIPS_LO16', 7: 'R_MIPS_GPREL16'}
STT_SECTION = 3
SHN_ABS = 0xFFF1
RETAIL_EXE = ROOT / 'bin/SCUS_94306R.EXE'
M32 = 0xFFFFFFFF
PORT_KINDS = ('port', 'port_new', 'port_relayout')


class RelocError(SystemExit):
    pass


def sext16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


def h8(v):
    return '%08X' % (v & M32)


def exe_image(path):
    """(t_addr, pc0, t_size, image bytes) of a PS-X EXE."""
    raw = Path(path).read_bytes()
    pc0, gp0, t_addr, t_size = struct.unpack_from('<4I', raw, 0x10)
    return dict(raw=raw, pc0=pc0, t_addr=t_addr, t_size=t_size, image=raw[0x800:0x800 + t_size])


class AddrMap:
    """Rows (r_start, r_end, c_start, c_end, kind, owner). Aux rows have r_start == r_end."""

    def __init__(self, rows, header):
        self.port_rows = sorted((r for r in rows if r[4] in PORT_KINDS), key=lambda r: (r[2], r[3]))
        rows = [r for r in rows if r[4] not in PORT_KINDS]
        self.rows = sorted(rows, key=lambda r: (r[2], r[3]))
        self.header = header
        self.c_rows = [r for r in self.rows if r[3] > r[2]]
        self.c_starts = [r[2] for r in self.c_rows]
        self.r_rows = sorted((r for r in self.rows if r[1] > r[0]), key=lambda r: r[0])
        self.r_starts = [r[0] for r in self.r_rows]
        self.c_lo, self.c_hi = self.c_rows[0][2], max(r[3] for r in self.c_rows)
        self.r_lo, self.r_hi = self.r_rows[0][0], max(r[1] for r in self.r_rows)

    @classmethod
    def identity(cls, lo, hi, header=None):
        return cls([(lo, hi, lo, hi, 'identity', 'identity')], header or {})

    def c2r(self, v):
        """Retail address of candidate address v, or None if it has none (inside padding / a resized row)."""
        for r0, r1, c0, c1, kind, _ in self.port_rows:
            if c0 <= v < c1:
                if kind == 'port':
                    return r0 + v - c0
                if kind == 'port_relayout' and v == c0:
                    return r0
                return None
        if v < self.c_lo or v > self.c_hi:
            return v
        if v == self.c_hi:
            return self.r_hi
        i = bisect.bisect_right(self.c_starts, v) - 1
        r0, r1, c0, c1, kind, _ = self.c_rows[i]
        if r0 == r1:  # aux: only its start, as the end of the row before it, has a retail address
            if v == c0 and i > 0 and self.c_rows[i - 1][3] == c0 and self.c_rows[i - 1][1] > self.c_rows[i - 1][0]:
                return self.c_rows[i - 1][1]
            return None
        if r1 - r0 == c1 - c0:
            return r0 + v - c0
        return r0 if v == c0 else None

    def r2c(self, v):
        for r0, r1, c0, c1, kind, _ in self.port_rows:
            if r0 <= v < r1:  # a moved array: its new home
                if kind == 'port':
                    return c0 + v - r0
                return c0 if v == r0 else None
        if v < self.r_lo or v > self.r_hi:
            return v
        if v == self.r_hi:
            return self.c_hi
        i = bisect.bisect_right(self.r_starts, v) - 1
        r0, r1, c0, c1, kind, _ = self.r_rows[i]
        if r1 - r0 == c1 - c0:
            return c0 + v - r0
        return c0 if v == r0 else None

    def is_identity(self):
        return all(r[0] == r[2] and r[1] == r[3] for r in self.rows if r[1] > r[0]) and not any(
            r[0] == r[1] and r[3] > r[2] for r in self.rows) and not self.port_rows

    def write(self, path):
        lines = ['# Generated by tools/reloc_image.py: retail (r) <-> candidate (c) address map of a relocatable link.',
                 '# Header lines are "key<TAB>value"; rows tile retail %s..%s; aux rows (r_start == r_end) exist only '
                 'in the candidate.' % (h8(self.r_lo), h8(self.r_hi))]
        for k in ('gp_r', 'gp_c', 'bss_end_r', 'bss_end_c', 'entry_r', 'entry_c'):
            lines.append('%s\t%s' % (k, h8(self.header[k])))
        lines.append('r_start\tr_end\tc_start\tc_end\tkind\towner')
        for r in sorted(self.rows, key=lambda r: (r[2], r[3])) + self.port_rows:
            lines.append('%s\t%s\t%s\t%s\t%s\t%s' % (h8(r[0]), h8(r[1]), h8(r[2]), h8(r[3]), r[4], r[5]))
        Path(path).write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')

    @classmethod
    def read(cls, path):
        header, rows, cols = {}, [], False
        for line in Path(path).read_text(encoding='ascii').splitlines():
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if parts[0] == 'r_start':
                cols = True
                continue
            if not cols:
                header[parts[0]] = int(parts[1], 16)
            else:
                rows.append((int(parts[0], 16), int(parts[1], 16), int(parts[2], 16), int(parts[3], 16), parts[4], parts[5]))
        return cls(rows, header)


class Link:
    """A relocatable link output directory: plan, ELF, EXE, item placement and relocation records."""

    def __init__(self, out_dir):
        self.dir = Path(out_dir)
        self.plan = json.loads((self.dir / 'reloc_plan.json').read_text(encoding='utf-8'))
        self.elf = read_elf(self.dir / 'TM2_SOURCE.elf')
        self.exe = exe_image(self.dir / 'TM2_SOURCE.EXE')
        self.t_addr = self.exe['t_addr']
        out_secs = {s['name']: s for s in self.elf['sections'] if s['flags'] & 2}
        self.symbols = {}
        for s in self.elf['symbols']:
            if s['name'] and s['binding'] in (1, 2) and s['section_index'] != 0:
                if s['name'] in self.symbols and self.symbols[s['name']]['address'] != s['address']:
                    raise RelocError('reloc_image: symbol %s defined twice' % s['name'])
                self.symbols[s['name']] = s
        self.items = self.plan['items']
        prev_end = self.t_addr
        for it in self.items:
            it['r_start'], it['r_end'] = int(it['r_start'], 16), int(it['r_end'], 16)
            size = it['size'] if it['type'] == 'aux' else it['r_end'] - it['r_start']
            sec = out_secs.get(it['name'])
            if sec is None:
                if size and it['type'] != 'c':
                    raise RelocError('reloc_image: output section %s missing from the ELF' % it['name'])
                it['c_start'] = prev_end  # empty section dropped by ld
                it['c_size'] = 0
            else:
                it['c_start'], it['c_size'] = sec['address'], sec['size']
            it['c_end'] = it['c_start'] + it['c_size']
            if not it.get('alias_of'):
                prev_end = max(prev_end, it['c_end'])
        self.bss_end_c = max(it['c_end'] for it in self.items)
        self.port = self.plan.get('port')
        # port variant: each object's part of the port sections (outside the EXE; no retail counterpart). Their
        # relocations are checked like every other (relocs, hi16) but never un-relocated.
        self.port_items, self.port_mem = [], []
        for inp in (self.port or {}).get('inputs', []):
            c0 = int(self.port['addresses'][inp['section']], 16) + inp['offset']
            sec = out_secs.get(inp['section'])
            if inp['size'] and (sec is None or not sec['address'] <= c0 <= c0 + inp['size'] <= sec['address'] + sec['size']):
                raise RelocError('reloc_image: %s %s is not inside the linked %s' % (inp['unit'], inp['section'], inp['section']))
            self.port_items.append(dict(type='port', name='%s:%s' % (inp['section'], inp['unit']), obj=inp['obj'],
                                        section=inp['section'], unit=inp['unit'], code=inp['section'] == '.port_text',
                                        c_start=c0, c_end=c0 + inp['size'], c_size=inp['size']))
        for s in out_secs.values():
            if self.port and s['type'] != 8 and s['address'] >= int(self.port['base'], 16):
                self.port_mem.append((s['address'], s['data']))
        self.gp_c = self.symbols['_gp']['address']
        self.entry_c = self.elf['entry']
        self._mem = None

    # ---- memory
    def mem(self, addr, n=4):
        off = addr - self.t_addr
        img = self.exe['image']
        if 0 <= off and off + n <= len(img):
            return img[off:off + n]
        for a, data in self.port_mem:  # port variant: .port_data / .port_text / .port_rodata
            if a <= addr and addr + n <= a + len(data):
                return data[addr - a:addr - a + n]
        return b'\0' * n

    def word(self, addr):
        return struct.unpack('<I', self.mem(addr))[0]

    # ---- address map
    def addrmap(self, functions=None, retail=None):
        """Rows per item; code sections split per function (the tail merged into its function's row)."""
        functions = functions or load_functions()
        rows = []
        items = [it for it in self.items if not it.get('alias_of')]
        for it in items:
            kind = segment_kind(it)
            if it['type'] == 'aux':
                rows.append([it['r_start'], it['r_start'], it['c_start'], it['c_end'], 'aux', 'shift padding'])
            elif it['type'] == 'c' and it['code']:
                fs = it['functions']
                for i, (va, end, name) in enumerate(fs):
                    va, end = int(va, 16), int(end, 16)
                    c0 = it['c_start'] + (va - it['r_start'])
                    c1 = it['c_end'] if i == len(fs) - 1 else c0 + (end - va)
                    rows.append([va, min(end, it['r_end']) if i == len(fs) - 1 else end, c0, c1, 'code', '%s %s' % (name, it['unit'])])
            elif it['type'] == 'tail':
                last = rows[-1]
                if last[1] != it['r_start'] or last[3] != it['c_start']:
                    raise RelocError('reloc_image: tail %s is not adjacent to its function' % it['name'])
                last[1], last[3] = it['r_end'], it['c_end']
            elif it['type'] == 'c':
                rows.append([it['r_start'], it['r_end'], it['c_start'], it['c_end'], kind, '%s %s' % (it['unit'], it['section'])])
            else:
                rows.append([it['r_start'], it['r_end'], it['c_start'], it['c_end'],
                             'code_linear' if kind == 'code' else kind, 'retail %s' % it['seg']])
        rows = [r for r in rows if r[1] > r[0] or r[3] > r[2]]
        # alignment gaps the linker inserted are candidate-only
        full, prev = [], None
        for r in sorted(rows, key=lambda r: (r[2], r[3])):
            if prev is not None and r[2] > prev:
                full.append([r[0], r[0], prev, r[2], 'aux', 'alignment padding'])
            full.append(r)
            prev = max(prev or 0, r[3])
        header = dict(gp_r=0x801809D4, gp_c=self.gp_c, bss_end_r=self.plan_bss_end(), bss_end_c=self.bss_end_c,
                      entry_r=None, entry_c=self.entry_c)
        full += self.port_map_rows()
        amap = AddrMap([tuple(r) for r in full], header)
        header['entry_r'] = amap.c2r(self.entry_c)
        return amap

    def port_map_rows(self):
        """Port variant: one row per moved array (port / port_relayout) and one per grown tail (port_new)."""
        rows = []
        for a in (self.port or {}).get('arrays', []):
            c0 = self.symbols[a['name']]['address']
            if a['retail_va'] is None:
                continue  # port-only storage (no retail address): listed in symbols.tsv only
            r0, rsize = int(a['retail_va'], 16), a['retail_size']
            if a['layout'] == 'relayout':
                rows.append([r0, r0 + rsize, c0, c0 + a['size'], 'port_relayout', a['name']])
                continue
            rows.append([r0, r0 + rsize, c0, c0 + rsize, 'port', a['name']])
            if a['size'] > rsize:
                rows.append([r0 + rsize, r0 + rsize, c0 + rsize, c0 + a['size'], 'port_new', a['name']])
        return rows

    def plan_bss_end(self):
        return int(self.plan['bss_end'], 16)

    # ---- relocation records
    def relocations(self):
        """[(P_c, type, V_c, item, symbol name)] for every relocation of every linked input section.

        V_c is the address the field encodes (for HI16/LO16 the full paired address, for GPREL16 the
        target, for R_MIPS_26 the jump target)."""
        cache = {}
        by_obj_sec = {}
        for it in self.items + self.port_items:
            obj = it['obj'] if it['type'] in ('c', 'port') else self.plan['piece_objs'][it['seg']]
            sec = it['section'] if it['type'] in ('c', 'port') else it['name']
            it['_obj'], it['_sec'] = obj, sec
            by_obj_sec[(obj, sec)] = it
        out, problems = [], []
        for it in self.items + self.port_items:
            obj = it['_obj']
            if obj not in cache:
                elf = read_elf(ROOT / obj)
                cache[obj] = (elf, read_relocs(elf))
            elf, rels = cache[obj]
            sec = next((s for s in elf['sections'] if s['name'] == it['_sec']), None)
            if sec is None:
                raise RelocError('reloc_image: %s has no section %s' % (obj, it['_sec']))
            rows = rels.get(sec['index'], [])
            data = sec['data']

            def sym_addr(sym):
                if sym['section_index'] == 0:
                    s = self.symbols.get(sym['name'])
                    if s is None:
                        raise RelocError('reloc_image: %s imports %s, which the link does not define' % (obj, sym['name']))
                    return s['address']
                if sym['section_index'] == SHN_ABS:
                    return sym['address']
                tsec = elf['sections'][sym['section_index']]
                target = by_obj_sec.get((obj, tsec['name']))
                if target is None:
                    raise RelocError('reloc_image: %s %s relocates against unlinked section %s' % (obj, it['_sec'], tsec['name']))
                return target['c_start'] + (0 if sym['type'] == STT_SECTION else sym['address'])

            for i, (off, rtype, sym) in enumerate(rows):
                p_c = it['c_start'] + off
                w = struct.unpack_from('<I', data, off)[0] if not sec['type'] == 8 else 0
                s = sym_addr(sym)
                if rtype == R_MIPS_32:
                    v = (s + w) & M32
                elif rtype == R_MIPS_26:
                    a = (w & 0x3FFFFFF) << 2
                    if sym['binding'] != 0 and a & (1 << 27):  # global: sign-extended addend
                        a -= 1 << 28
                    v = ((p_c + 4) & 0xF0000000) | ((s + a) & 0x0FFFFFFF)  # jump target in the 256 MB region
                elif rtype in (R_MIPS_HI16, R_MIPS_LO16):
                    if rtype == R_MIPS_HI16:
                        lo = next(((o2, s2) for (o2, t2, s2) in rows[i + 1:] if t2 == R_MIPS_LO16 and s2 is sym), None)
                        if lo is None:
                            problems.append('orphan HI16 at %s+0x%X (%s)' % (it['name'], off, sym['name']))
                            continue
                        lo_in = struct.unpack_from('<I', data, lo[0])[0]
                        a = ((w & 0xFFFF) << 16) + sext16(lo_in)
                    else:
                        hi = next(((o2, s2) for (o2, t2, s2) in reversed(rows[:i]) if t2 == R_MIPS_HI16 and s2 is sym), None)
                        a = sext16(w)
                        if hi is not None:
                            hi_in = struct.unpack_from('<I', data, hi[0])[0]
                            a = ((hi_in & 0xFFFF) << 16) + sext16(w)
                    v = (s + a) & M32
                elif rtype == R_MIPS_GPREL16:
                    v = (s + sext16(w)) & M32
                else:
                    raise RelocError('reloc_image: unsupported relocation %d at %s+0x%X' % (rtype, it['name'], off))
                out.append((p_c, rtype, v, it, sym['name']))
        return out, problems


def segment_kind(it):
    return {'code': 'code', 'rodata': 'data', 'data': 'data', 'sdata': 'sdata', 'bss_in_file': 'bss',
            'bss': 'bss'}[it['seg_kind']]


def encode(rtype, word, v, gp):
    if rtype == R_MIPS_32:
        return v & M32
    if rtype == R_MIPS_26:
        return (word & 0xFC000000) | ((v >> 2) & 0x3FFFFFF)
    if rtype == R_MIPS_HI16:
        return (word & 0xFFFF0000) | (((v + 0x8000) >> 16) & 0xFFFF)
    if rtype == R_MIPS_LO16:
        return (word & 0xFFFF0000) | (v & 0xFFFF)
    if rtype == R_MIPS_GPREL16:
        return (word & 0xFFFF0000) | ((v - gp) & 0xFFFF)
    raise RelocError('encode: type %d' % rtype)


def replaced_ranges(link):
    """All-C flavour: retail [start, end) of every replaced C section (pure-C versions of hand-written asm);
    port variant: of every function changed by the variant and of the jump tables of those functions."""
    return sorted((it['r_start'], it['r_end']) for it in link.items if it.get('all_c') or it.get('excluded'))


def canonicalize(link, amap, relocs):
    """Un-relocate the candidate image into retail coordinates.

    Returns (image bytes from t_addr to the retail file end, problems, covered retail word addresses).

    All-C flavour (reloc_plan.json "all_c"): the replaced C sections have no retail counterpart beyond their
    entry. Their rows are left zero in the canonical image and their own relocations are only checked
    (field == record), not un-relocated. A relocation elsewhere whose target has no retail address (a point
    inside a replaced section: a kind handler, a retail asm label standing for the entry) is un-relocated by
    name, through all_c.retail_symbols (the retail address of each global of the replaced asm units and of
    each asm label)."""
    problems = []
    allc = link.plan.get('all_c') or {}
    replaced_items = {it['name'] for it in link.items if it.get('all_c') or it.get('excluded')}
    replaced_r = replaced_ranges(link)
    retail_syms = {n: int(a, 16) for n, a in allc.get('retail_symbols', {}).items()}
    gp_r = amap.c2r(link.gp_c)
    # retail file end: ALIGN(BSS start, 0x800), as for the fixed link
    progbits_end = next(s['start'] for s in read_layout() if s['kind'] in ('bss_in_file', 'bss'))
    r_file_end = link.t_addr + ((progbits_end - link.t_addr + 0x7FF) // 0x800) * 0x800
    out = bytearray(r_file_end - link.t_addr)
    for r0, r1, c0, c1, kind, owner in amap.rows:
        if r1 <= r0 or kind == 'bss' or r0 >= r_file_end:
            continue
        if r1 - r0 != c1 - c0:
            if any(a <= r0 < b for a, b in replaced_r):
                continue  # replaced C: only its entry maps
            problems.append('row %s..%s (%s) changed size: only its entry maps' % (h8(r0), h8(r1), owner))
            continue
        n = min(r1, r_file_end) - r0
        out[r0 - link.t_addr:r0 - link.t_addr + n] = link.mem(c0, n)
    covered = {}
    for p_c, rtype, v, it, name in relocs:
        w = link.word(p_c)
        if encode(rtype, w, v, link.gp_c) != w:
            problems.append('relocation %s at %s (%s+0x%X, %s): the image holds %08X, expected %08X' % (
                RELOC_NAMES[rtype], h8(p_c), it['name'], p_c - it['c_start'], name, w, encode(rtype, w, v, link.gp_c)))
        if it['name'] in replaced_items or it['type'] == 'port':
            continue  # inside replaced C or port code/storage: no retail bytes to compare with
        p_r = amap.c2r(p_c)
        v_r = amap.c2r(v)
        if name in retail_syms and name in link.symbols:  # by name: the label may now stand for an entry
            v_r = (retail_syms[name] + v - link.symbols[name]['address']) & M32
        if p_r is None:
            problems.append('relocation at %s has no retail position' % h8(p_c))
            continue
        if v_r is None:
            problems.append('relocation at %s (%s, %s) targets %s, which has no retail address' % (
                h8(p_c), RELOC_NAMES[rtype], name, h8(v)))
            continue
        covered[p_r] = (rtype, v_r, name)
        if p_r - link.t_addr + 4 > len(out):
            continue
        out[p_r - link.t_addr:p_r - link.t_addr + 4] = struct.pack('<I', encode(rtype, w, v_r, gp_r))
    return bytes(out), problems, covered


def classify_code(amap, canon, retail_image, t_addr):
    """code rows -> code_linear when the un-relocated bytes equal retail over the whole slot, else code_entry."""
    rows = []
    for r in amap.rows:
        r = list(r)
        if r[4] == 'code':
            a, b = r[0] - t_addr, r[1] - t_addr
            same = r[1] - r[0] == r[3] - r[2] and canon[a:b] == retail_image[a:b]
            r[4] = 'code_linear' if same else 'code_entry'
        rows.append(tuple(r))
    return AddrMap(rows + amap.port_rows, amap.header)


def write_outputs(link, amap, relocs):
    """addrmap.tsv, symbols.tsv, pointer_slots.tsv next to the EXE."""
    amap.write(link.dir / 'addrmap.tsv')
    exec_secs = {s['index'] for s in link.elf['sections'] if s['flags'] & 4}
    rows, seen = [], set()
    for name, s in sorted(link.symbols.items(), key=lambda kv: (kv[1]['address'], kv[0])):
        code = 1 if s['section_index'] in exec_secs else 0
        rows.append((s['address'], code, name, s['size']))
        seen.add(name)
    extra = 0
    retail_names = list(csv.DictReader((CONFIG / 'ida_names.tsv').open(), delimiter='\t'))
    code_at = {int(r['va'], 16): r['code'] for r in retail_names}
    # config/US/names.tsv: readable names given to retail symbols are listed too
    retail_names += [dict(va='%08X' % r['va'], name=r['new'],
                          code=code_at.get(r['va'], '1' if r['old'].startswith(('sub_', 'func_')) else '0'))
                     for r in load_renames()]
    for r in retail_names:
        if r['name'] in seen:
            continue
        c = amap.r2c(int(r['va'], 16))
        if c is None:
            continue
        rows.append((c, int(r['code']), r['name'], 0))
        seen.add(r['name'])
        extra += 1
    lines = ['# Generated by tools/reloc_image.py: symbols of the relocatable image (ida_names.tsv format + size).',
             '# Every global of the linked ELF, plus each retail ida_names.tsv name not defined by the link,',
             '# translated with the address map (size 0).', 'va\tcode\tname\tsize']
    lines += ['%s\t%d\t%s\t%d' % (h8(a), c, n, sz) for a, c, n, sz in sorted(rows, key=lambda x: (x[0], x[2]))]
    (link.dir / 'symbols.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')

    by_c = {p_c: (t, v, name) for p_c, t, v, it, name in relocs}
    ordered = sorted((s['address'], n) for n, s in link.symbols.items() if s['section_index'] != SHN_ABS)
    addrs = [a for a, _ in ordered]
    slot_rows = []
    for s in link.plan['slots']:
        rec = by_c.get(amap.r2c(int(s['address'], 16)))
        v = rec[1] if rec else amap.r2c(int(s['value'], 16))
        if s['via'] == 'generated' and s['symbol'] and s['symbol'] in link.symbols:
            sym, addend = s['symbol'], s['addend']
        else:
            from gen_link import split_symbol
            sym, addend = split_symbol(s['target_symbol'])
            if sym not in link.symbols or link.symbols[sym]['address'] + addend != v:
                i = bisect.bisect_right(addrs, v) - 1
                sym = ordered[i][1]
                addend = v - ordered[i][0]
        slot_rows.append((s['address'], 4, sym, addend, s['kind']))
    lines = ['# Generated by tools/reloc_image.py: typed pointer slots (config/US/data_relocs.tsv) of the relocatable',
             '# image. r_addr is retail; the candidate word at r2c(r_addr) must equal symbols.tsv[target_symbol] + addend.',
             'r_addr\twidth\ttarget_symbol\taddend\tkind']
    lines += ['%s\t%d\t%s\t0x%X\t%s' % row for row in slot_rows]
    (link.dir / 'pointer_slots.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')
    return dict(symbols=len(rows), retail_names_added=extra, pointer_slots=len(slot_rows))


def retail_image():
    return exe_image(RETAIL_EXE)


def build_maps(out_dir):
    """Post-link step of a relocatable build: address map (with code classification) and the three TSVs."""
    link = Link(out_dir)
    relocs, problems = link.relocations()
    amap = link.addrmap()
    canon, cproblems, _ = canonicalize(link, amap, relocs)
    retail = retail_image()
    amap = classify_code(amap, canon, retail['image'], link.t_addr)
    info = write_outputs(link, amap, relocs)
    kinds = {}
    for r in amap.rows:
        kinds[r[4]] = kinds.get(r[4], 0) + 1
    return dict(info, relocations=len(relocs), reloc_problems=problems + cproblems, map_rows=len(amap.rows),
                map_kinds=kinds, identity=amap.is_identity(), gp_c=h8(link.gp_c), bss_end_c=h8(link.bss_end_c),
                entry_c=h8(link.entry_c), t_size='0x%X' % link.exe['t_size'])
