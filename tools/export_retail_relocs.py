#!/usr/bin/env python3
"""Relocation table of the RETAIL EXE, for moving data in a recompiled retail game (port request 24).

Writes (config/US/):
  retail_relocs.tsv   every relocation in the retail code: hi16 / lo16 / gprel16 / jal26 (j and jal) / word32
  retail_data_ptrs.tsv every data word that holds an address into the image or BSS, with symbol + addend
  move_hazards.tsv    what makes moving the per-player arrays (ARRAYS below) unsafe

Sources:
  * reloc  -- the relocation records of a relocatable source-only link at retail's layout (E0: identity address
              map, tools/reloc_image.py Link.relocations). For a function whose bytes equal retail
              (addrmap.tsv code_linear) every record is at its retail VA and encodes the retail value.
  * disasm -- for the close functions (addrmap.tsv code_entry: compiled C whose bytes differ from retail) the
              records describe the C, not retail; their relocations are derived from the RETAIL disassembly:
              lui -> addiu/ori/load/store pairs by forward data-flow (also through addu), gp-relative operands
              (gp = 801809D4) and j/jal. The same method runs on the identical functions and is compared with
              the records there (agreement % per confidence class, printed and written to the TSV header).

No emulator. Reads bin/SCUS_94306R.EXE (addresses and values only go into the outputs).

    py -3.14 tools/build_source_only.py --build-dir last --sdk-from-lib --relocatable --output work/reloc24/E0
    py -3.14 tools/export_retail_relocs.py --e0 work/reloc24/E0
"""
import argparse
import bisect
import collections
import csv
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, CONFIG, load_functions
from reloc_image import (Link, AddrMap, exe_image, RETAIL_EXE, sext16, h8, M32, STT_SECTION, SHN_ABS,
                         R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16)
from semantic_elf import read_elf, read_relocs
from symbol_names import load_renames

GP = 0x801809D4
SBSS_END = 0x80181164
BSS_START = 0x80180EE4           # highest gp-relative target in retail (docs/SOURCE_BUILD.md)
IMG_LO, IMG_HI = 0x800CDC54, 0x801FAD48
KIND = {R_MIPS_HI16: 'hi16', R_MIPS_LO16: 'lo16', R_MIPS_GPREL16: 'gprel16', R_MIPS_26: 'jal26', R_MIPS_32: 'word32'}

# The arrays of request 24 (current names, config/US/names.tsv). size None = up to the next retail label.
# Sizes: docs/FOUR_PLAYERS.md 3, 4.4, 5.1; docs/SIXTEEN_CARS.md; request 23.
ARRAYS = [
    # group, name, va, size
    ('humans', 'humanCars', 0x80187B10, 2 * 0x814),
    ('humans', 'humanCarObjects', 0x80180FEC, 8),
    ('humans', 'humanCarObjectSlots', 0x80180FF4, None),
    ('humans', 'humanCarTypes', 0x80181004, None),
    ('humans', 'humanCarTotal', 0x80181014, 4),
    ('humans', 'D_80180FD8', 0x80180FD8, None),
    ('humans', 'D_80180FE0', 0x80180FE0, None),
    ('humans', 'carResetWeapons', 0x80180FA0, 4),
    ('humans', 'carViewHotkeyReleased', 0x80180B9C, None),
    ('humans', 'hudSmoothedSpeeds', 0x8018100C, 8),
    ('views', 'viewFollowObjects', 0x80180DDC, 8),
    ('views', 'viewCameraObjectTable', 0x80181024, 8),
    ('views', 'viewCameraObjects', 0x80181110, 8),
    ('views', 'viewEnvs', 0x80181118, 8),
    ('views', 'viewModes', 0x80181120, 8),
    ('views', 'viewEyePositions', 0x801A51A8, 24),
    ('views', 'viewMatrices', 0x801A51C0, 64),
    ('frames', 'D_80180EE4', 0x80180EE4, 4),
    ('frames', 'renderBufferView1', 0x80180EE8, 4),
    ('frames', 'renderBufferRearView', 0x80180EEC, 4),
    ('frames', 'renderView0Frames', 0x8018118C, 0x100),
    ('frames', 'renderView1Frames', 0x8018128C, 0x100),
    ('frames', 'renderMirrorFrames', 0x8018138C, 0x100),
    ('frames', 'renderPacketBuffer0', 0x801B600C, 0x17CF0),
    ('frames', 'renderPacketBuffer1', 0x801CDD4C, 0x17CF0),
    ('ots', 'renderOtView1Buf0', 0x801E5A8C, 0x4000),
    ('ots', 'renderOtView1Buf1', 0x801E9A8C, 0x4000),
    ('ots', 'renderOtView2Buf0', 0x801EDA8C, 0x4000),
    ('ots', 'renderOtView2Buf1', 0x801F1A8C, 0x4000),
    ('hud', 'hudOrderingTable', 0x80180F0C, 4),
    ('hud', 'hudOrderTables', 0x801828D0, 0x20),
    ('hud', 'hudPlayerRecords', 0x801828F0, 2 * 1900),
    ('hud', 'hudRadars', 0x801837C8, 2 * 880),
    ('hud', 'hudRadarVisible', 0x80180A6C, 4),
    ('hud', 'hudWeaponListVisible', 0x80180A70, 4),
    ('hud', 'hudMarkerBlinkCounters', 0x80180A74, 8),
    ('hud', 'hudMarkerBlinkPhases', 0x80180A7C, 8),
    ('hud', 'hudRadarArrowUpPrims', 0x80182308, 64),
    ('hud', 'hudRadarArrowDownPrims', 0x80182348, 64),
    ('hud', 'hudRadarArrowLeftPrims', 0x80182388, 64),
    ('hud', 'hudRadarArrowRightPrims', 0x801823C8, 64),
    ('hud', 'hudTargetNameModes', 0x80182408, 24),
    ('hud', 'hudTargetNameSprites', 0x80182420, 1200),
    ('hud', 'hudRadarArrowUpSprites', 0x80183EA8, 16),
    ('hud', 'hudRadarArrowDownSprites', 0x80183EB8, 16),
    ('hud', 'hudRadarArrowLeftSprites', 0x80183EC8, 16),
    ('hud', 'hudRadarArrowRightSprites', 0x80183ED8, 16),
    # request 32 step 2 (docs/CAR_GROUPS.md "Step 2"): the car arrays and the pools that more cars fill
    ('cars', 'aiCarTargets', 0x801879FC, 9 * 4),
    ('cars', 'carSlotKinds', 0x80187A20, 15 * 4),
    ('cars', 'carSlotIndices', 0x80187A5C, 15 * 4),
    ('cars', 'carSlotOwners', 0x80187A98, 15 * 4),
    ('cars', 'carSlotObjects', 0x80187AD4, 15 * 4),
    ('cars', 'aiCarObjects', 0x80188B38, 9 * 4),
    ('cars', 'aiCars', 0x80188B5C, 9 * 0x854),
    ('cars', 'aiCarObjectSlots', 0x8018D650, 9 * 4),
    ('cars', 'carPairVectors', 0x8018D674, 225 * 12),
    ('cars', 'carPairHeadingDeltas', 0x8018E100, 225 * 4),
    ('cars', 'carPairDirections', 0x8018E484, 225 * 12),
    ('cars', 'carPairHeadings', 0x8018EF10, 452),
    ('cars', 'carPairDistances', 0x8018F0D4, 225 * 4),
    ('pools', 'fxRecords', 0x801A5B08, 16 * 980),
    ('pools', 'csObjectPool', 0x8019A950, 128 * 0x128),
    ('pools', 'soundInstPool', 0x8017F5D0, 35 * 0x84),
    ('pools', 'weaponArcs', 0x801847B4, 5 * 20),
    ('pools', 'weaponShieldPool', 0x80184A68, 64),
    ('pools', 'weaponRemoteBombObjects', 0x80180F30, 8),
    ('pools', 'weaponRicochetPool', 0x80184860, 520),
    ('pools', 'weaponFlameEmitters', 0x80184774, 2 * 32),
    ('pools', 'weaponSlamRecords', 0x80184818, 4 * 18),
    ('pools', 'renderWorldModels', 0x801A4E1C, 20 * 4),
    ('pools', 'clutAnimDefs', 0x80197E58, 20 * 20),
    ('pools', 'clutAnimRunning', 0x80197FE8, 10 * 0x424),
]

# Index past the end / storage shared with a neighbour, from the docs (static analysis, not found by the scan).
DOC_HAZARDS = [
    ('801387A4', '__SN_ENTRY_POINT', '-', 'bss_clear',
     'crt0 zeroes D_80180EE4..D_801FAD48 (sltu loop bound at 801387A4) and calls InitHeap(D_801FAD48 + 4): an array '
     'moved out of BSS (above 2 MB) is not zeroed at boot - the port must clear its new area itself; the heap '
     'still starts at the retail BSS end', 'high', 'docs/SOURCE_BUILD.md (crt0)'),
    ('80112AC4', 'func_80112AB0', 'fxFontUvStorage', 'index_past_end',
     'grutilsParseSpriteRects(0x80187194, 30) into a 10-record (80-byte) bank; records 11+ would overwrite the '
     'font draw modes 801871E4 and templates 80187238 (RETAIL_BUGS.md 6; count capped by the asset, LIKELY <= 10)',
     'medium', 'docs/RETAIL_BUGS.md 6'),
    ('80152354', 'renderScene', 'renderMatrixSlots', 'index_past_end',
     'matrix-slot index can reach 200 (checks at 80152354/801523E8/801524BC, walker 80152C18 < 198): writes '
     'renderMatrixSlots[200] = worldSurfaces 801B0418 and renderObjectMatrices[200] = 801FA3B8 (timerTicsAccum)',
     'medium', 'docs/RETAIL_BUGS.md 13'),
    ('80150CFC', 'padCheckAction', 'padActionCache', 'index_past_end',
     'port 2 writes padActionCache[action][2] = [action+1][0] (one past the row, inside the table); '
     'padActionHeld with port 3/4 would index past padActiveBindings[2]/padConfiguredBindings[2]',
     'low', 'docs/FOUR_PLAYERS.md 3'),
    ('80121764', 'bitmapAddRecord', 'renderOtView1Buf0', 'shared_storage',
     'while loading, the image records bitmapRecords live in renderGetOrderingTable() = renderOtView1Buf0 '
     '(682 x 24 bytes); more records spill into renderOtView1Buf1.. (idle while loading). Moving or resizing '
     'the OTs separately changes where the spill lands',
     'high', 'docs/SIXTEEN_CARS.md 4 (bitmapRecords)'),
    ('801214FC', 'bitmapSetupStorage', 'renderPacketBuffer0', 'shared_storage',
     'packet buffer 0 doubles as load-time storage: usage bitmap bitmapWords at +0 (0x10000 bytes) and '
     'vramTexSlots at +0x10004 (1599 x 20 bytes); keep the buffer >= 0x17CF0 when it moves',
     'medium', 'docs/SIXTEEN_CARS.md 4'),
    ('801E5A8C', '-', 'renderOtView2Buf0', 'shared_storage',
     'view 1 and the rear-view mirror SHARE renderOtView2Buf0/1; a third/fourth view needs its own OTs',
     'medium', 'docs/FOUR_PLAYERS.md 4.4'),
    ('80180EE4', '-', 'D_80180EE4', 'stored_pointer',
     'frame slots hold &renderView0Frames / view1 / mirror at run time; hudDraw* test D_80180EE4 == '
     '&renderView0Frames to pick the HUD OT buffer (both sides relocated: safe if both use the new address)',
     'low', 'docs/FOUR_PLAYERS.md 5.1'),
    ('801828D0', '-', 'hudOrderTables', 'layout',
     'hudOrderTables[2][2][2] is [buffer][player]: growing players changes the buffer stride (index math '
     'in hud_main/hud_split), not just the length', 'medium', 'docs/FOUR_PLAYERS.md 5.1'),
    ('800FD00C', 'hudRadarArrowsLoad', 'hudRadarArrowUpSprites', 'layout',
     'four 16-byte sprite arrays packed back to back (80183EA8..80183EE8), each [player]: no spill '
     '(RETAIL_BUGS.md checked), but each must grow on its own', 'low', 'docs/RETAIL_BUGS.md (not bugs)'),
]

MEMFUNCS = {  # name: (dst arg, src arg or None, size arg, unit bytes)
    'memset': (4, None, 6, 1), 'memcpy': (4, 5, 6, 1), 'memmove': (4, 5, 6, 1), 'bzero': (4, None, 5, 1),
    'bcopy': (5, 4, 6, 1), 'ClearOTag': (4, None, 5, 4), 'ClearOTagR': (4, None, 5, 4), 'strcpy': (4, 5, None, 1),
}

LOADS = {0x20: 1, 0x21: 2, 0x22: 4, 0x23: 4, 0x24: 1, 0x25: 2, 0x26: 4, 0x32: 4}   # lb lh lwl lw lbu lhu lwr lwc2
STORES = {0x28: 1, 0x29: 2, 0x2A: 4, 0x2B: 4, 0x2E: 4, 0x3A: 4}                   # sb sh swl sw swr swc2
CALLEE_SAVED = {16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30}


# ---------------------------------------------------------------- MIPS decoding
def decode(w):
    op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    return op, rs, rt, rd, w & 0x3F, w & 0xFFFF


def dest_reg(w):
    """General register written by w (None if none)."""
    op, rs, rt, rd, fn, imm = decode(w)
    if op == 0:
        if fn in (0x08, 0x0C, 0x0D, 0x11, 0x13, 0x18, 0x19, 0x1A, 0x1B):  # jr syscall break mthi mtlo mult div
            return None
        return rd or None
    if op == 1:
        return 31 if rt in (0x10, 0x11) else None
    if op == 3:
        return 31
    if 8 <= op <= 15 or op in LOADS and op != 0x32:
        return rt or None
    if op in (0x10, 0x12) and rs in (0, 2):  # mfc0/cfc0, mfc2/cfc2
        return rt or None
    return None


def branch_target(va, w):
    op, rs, rt, rd, fn, imm = decode(w)
    if op in (4, 5, 6, 7, 1) or (op in (0x10, 0x11, 0x12, 0x13) and rs == 8):
        return (va + 4 + (sext16(imm) << 2)) & M32
    if op == 2:
        return ((va + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
    return None


def is_delayed(w):
    op, rs, rt, rd, fn, imm = decode(w)
    return (op in (1, 2, 3, 4, 5, 6, 7) or (op == 0 and fn in (8, 9))
            or (op in (0x10, 0x11, 0x12, 0x13) and rs == 8))


class Scan:
    """Forward data-flow over one retail function: lui -> lo pairs, gp-relative operands, j/jal,
    plus the facts the hazard search needs (compares, stores and memory-function arguments)."""

    def __init__(self, start, end, words, labels, gp_ok):
        self.relocs = []      # (va, kind, target, pair_va, cls)
        self.compares = []    # (va, mnemonic, value_a, value_b)
        self.stores = []      # (va, stored value, dest description)
        self.calls = []       # (va, target, {argreg: value})
        regs = {}             # reg -> value tuple
        lui_cross = {}        # lui va -> crossed a label / jump since
        pending = None
        gp_written = False
        for i, w in enumerate(words):
            va = start + 4 * i
            if va in labels and i:
                for k in lui_cross:
                    lui_cross[k] = True
            op, rs, rt, rd, fn, imm = decode(w)
            s = sext16(imm)

            def val(r):
                return ('const', 0) if r == 0 else regs.get(r)

            def lo_use(base_reg, kind_):
                b = regs.get(base_reg)
                if b and b[0] in ('hi', 'hidx'):
                    tgt = (b[1] + (imm if kind_ == 'ori' else s)) & M32
                    if IMG_LO <= tgt <= IMG_HI:
                        cls = 'ori' if kind_ == 'ori' else ('indexed' if b[0] == 'hidx' else
                                                            ('cross' if lui_cross.get(b[2]) else 'adjacent'))
                        self.relocs.append((va, 'lo16', tgt, b[2], cls))
                        if not b[3]:
                            b[3].append((va, tgt, cls))
                        return tgt
                return None

            def is_hi(r):
                return bool(regs.get(r)) and regs[r][0] in ('hi', 'hidx')

            def gp_use():
                if rs == 28 and gp_ok and not gp_written:
                    tgt = (GP + s) & M32
                    self.relocs.append((va, 'gprel16', tgt, None, 'gp'))
                    return tgt
                return None

            newval = None
            d = dest_reg(w)
            if op == 0x0F:  # lui
                hi = imm << 16
                if IMG_LO - 0x8000 <= hi <= IMG_HI + 0x8000:
                    newval = ['hi', hi, va, []]
                    lui_cross[va] = False
                    self.relocs.append([va, 'hi16', None, None, None, newval])
                else:
                    newval = ('const', hi)
            elif op == 0x09:  # addiu
                tgt = lo_use(rs, 'addiu') if (rs != 28 or is_hi(rs)) else gp_use()
                if tgt is not None:
                    newval = ('addr', tgt, va)
                elif rs == 0:
                    newval = ('const', s & M32)
                else:
                    b = regs.get(rs)
                    if b and b[0] == 'addr':
                        newval = ('addr', (b[1] + s) & M32, b[2])
                    elif b and b[0] == 'const':
                        newval = ('const', (b[1] + s) & M32)
            elif op == 0x0D:  # ori
                tgt = lo_use(rs, 'ori')
                if tgt is not None:
                    newval = ('addr', tgt, va)
                elif rs == 0:
                    newval = ('const', imm)
                elif regs.get(rs) and regs[rs][0] == 'const':
                    newval = ('const', regs[rs][1] | imm)
            elif op in LOADS or op in STORES:
                if rs == 28 and not is_hi(rs):
                    tgt = gp_use()
                else:
                    tgt = lo_use(rs, 'mem')
                if op in STORES and op == 0x2B:
                    v = val(rt)
                    if v and v[0] == 'addr':
                        dest = 'gp+0x%X=%s' % (s & 0xFFFF, h8(tgt)) if rs == 28 and tgt is not None else (
                            h8(tgt) if tgt is not None else 'r%d+%d' % (rs, s))
                        self.stores.append((va, v, dest, tgt))
                if op in LOADS and tgt is not None:
                    newval = ('mem', tgt)
            elif op == 0:
                if fn in (0x21, 0x20):  # addu/add
                    a, b = regs.get(rs), regs.get(rt)
                    if rt == 0 and a:
                        newval = a
                    elif rs == 0 and b:
                        newval = b
                    else:
                        his = [x for x in (a, b) if x and x[0] in ('hi', 'hidx')]
                        ads = [x for x in (a, b) if x and x[0] in ('addr', 'aidx')]
                        if his:
                            x = his[0]
                            newval = ['hidx', x[1], x[2], x[3]]
                        elif ads:
                            newval = ('aidx', ads[0][1], ads[0][2])
                elif fn in (0x2A, 0x2B):
                    self.compares.append((va, 'slt' if fn == 0x2A else 'sltu', val(rs), val(rt), rs, rt))
                elif fn == 0x23:
                    a, b = val(rs), val(rt)
                    if (a and a[0] in ('addr', 'aidx')) and (b and b[0] in ('addr', 'aidx')):
                        self.compares.append((va, 'subu', a, b, rs, rt))
                elif fn == 0x25 and rt == 0:  # move = or rd, rs, $0
                    newval = regs.get(rs)
            elif op in (4, 5):
                self.compares.append((va, 'beq' if op == 4 else 'bne', val(rs), val(rt), rs, rt))
            elif op in (2, 3):
                tgt = ((va + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                self.relocs.append((va, 'jal26', tgt, None, 'jump'))
                if op == 3:
                    self.calls.append([va, tgt, None])
            if d == 28:
                gp_written = True
            if d is not None:
                regs.pop(d, None)
                if newval is not None:
                    regs[d] = newval
            # delay-slot effects of the jump before this instruction
            if pending is not None:
                kind_, jva = pending
                pending = None
                if kind_ == 'call':
                    self.calls[-1][2] = {r: regs.get(r) for r in (4, 5, 6, 7)}
                    for r in list(regs):
                        if r not in CALLEE_SAVED:
                            del regs[r]
                    regs[2] = ('ret', jva)
                elif kind_ == 'end':
                    regs.clear()
                for k in lui_cross:
                    lui_cross[k] = True
            if op == 3 or (op == 0 and fn == 9):
                pending = ('call', va)
                if op == 0:
                    self.calls.append([va, None, None])
            elif op == 0 and fn == 8:
                pending = ('end', va)
            elif op == 2 or (op == 4 and rs == 0 and rt == 0):
                pending = ('jump', va)
        # resolve hi16 rows: target and pair from their first lo use
        out = []
        for r in self.relocs:
            if isinstance(r, list):
                uses = r[5][3]
                if not uses:
                    continue  # lui of a non-address (no lo use as base)
                lva, tgt, cls = uses[0]
                out.append((r[0], 'hi16', tgt, lva, cls))
            else:
                out.append(r)
        self.relocs = out


# ---------------------------------------------------------------- names
class Names:
    def __init__(self, symbols_tsv):
        self.renames = {r['old']: r['new'] for r in load_renames()}
        new_at = {r['va']: r['new'] for r in load_renames()}
        by_addr = collections.defaultdict(list)
        for r in csv.DictReader((l for l in Path(symbols_tsv).open(encoding='ascii') if not l.startswith('#')),
                                delimiter='\t'):
            by_addr[int(r['va'], 16)].append((r['name'], int(r['code'])))
        for r in csv.DictReader((CONFIG / 'ida_names.tsv').open(), delimiter='\t'):
            by_addr[int(r['va'], 16)].append((r['name'], int(r['code'])))

        def rank(n):
            if n.startswith(('D_', 'sub_', 'func_', 'jpt_', 'off_', 'loc_', 'a8', 'byte_', 'word_', 'dword_', '.')):
                return 2
            return 1
        self.best = {}
        for a, ns in by_addr.items():
            if a in new_at:
                self.best[a] = new_at[a]
            else:
                self.best[a] = sorted(ns, key=lambda x: (rank(x[0]), x[0]))[0][0]
        for a, n in new_at.items():
            self.best.setdefault(a, n)
        self.addrs = sorted(self.best)

    def name(self, n):
        return self.renames.get(n, n)

    def owner(self, a):
        """Nearest named address at or below a: (name, offset)."""
        i = bisect.bisect_right(self.addrs, a) - 1
        if i < 0:
            return h8(a), 0
        return self.best[self.addrs[i]], a - self.addrs[i]

    def next_after(self, a):
        i = bisect.bisect_right(self.addrs, a)
        return self.addrs[i] if i < len(self.addrs) else None


def symadd(name, addend):
    if addend == 0:
        return name
    return '%s%s0x%X' % (name, '+' if addend > 0 else '-', abs(addend))


# ---------------------------------------------------------------- relocation records of the E0 link
def link_records(link):
    """[(va, kind, symbol name, target, pair_va, sym_addr, item)] from every input section of the E0 link."""
    cache, by_obj_sec, out = {}, {}, []
    for it in link.items:
        obj = it['obj'] if it['type'] == 'c' else link.plan['piece_objs'][it['seg']]
        sec = it['section'] if it['type'] == 'c' else it['name']
        it['_obj'], it['_sec'] = obj, sec
        by_obj_sec[(obj, sec)] = it
    for it in link.items:
        obj = it['_obj']
        if obj not in cache:
            elf = read_elf(ROOT / obj)
            cache[obj] = (elf, read_relocs(elf))
        elf, rels = cache[obj]
        sec = next(s for s in elf['sections'] if s['name'] == it['_sec'])
        rows, data = rels.get(sec['index'], []), sec['data']

        def sym_addr(sym):
            if sym['section_index'] == 0:
                return link.symbols[sym['name']]['address'], sym['name']
            if sym['section_index'] == SHN_ABS:
                return sym['address'], sym['name']
            tsec = elf['sections'][sym['section_index']]
            target = by_obj_sec[(obj, tsec['name'])]
            a = target['c_start'] + (0 if sym['type'] == STT_SECTION else sym['address'])
            return a, (None if sym['type'] == STT_SECTION or not sym['name'] else sym['name'])

        for i, (off, rtype, sym) in enumerate(rows):
            p = it['c_start'] + off
            w = struct.unpack_from('<I', data, off)[0] if sec['type'] != 8 else 0
            s, sname = sym_addr(sym)
            pair = None
            if rtype == R_MIPS_32:
                v = (s + w) & M32
            elif rtype == R_MIPS_26:
                a = (w & 0x3FFFFFF) << 2
                if sym['binding'] != 0 and a & (1 << 27):
                    a -= 1 << 28
                v = ((p + 4) & 0xF0000000) | ((s + a) & 0x0FFFFFFF)
            elif rtype == R_MIPS_HI16:
                lo = next(((o2, s2) for (o2, t2, s2) in rows[i + 1:] if t2 == R_MIPS_LO16 and s2 is sym), None)
                if lo is None:
                    continue
                pair = it['c_start'] + lo[0]
                v = (s + ((w & 0xFFFF) << 16) + sext16(struct.unpack_from('<I', data, lo[0])[0])) & M32
            elif rtype == R_MIPS_LO16:
                hi = next(((o2, s2) for (o2, t2, s2) in reversed(rows[:i]) if t2 == R_MIPS_HI16 and s2 is sym), None)
                a = sext16(w)
                if hi is not None:
                    pair = it['c_start'] + hi[0]
                    a += (struct.unpack_from('<I', data, hi[0])[0] & 0xFFFF) << 16
                v = (s + a) & M32
            elif rtype == R_MIPS_GPREL16:
                v = (s + sext16(w)) & M32
            else:
                continue
            out.append((p, KIND[rtype], sname, v, pair, s, it))
    return out


# ---------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--e0', required=True, help='relocatable link at retail layout (no --shift)')
    ap.add_argument('--out', default=str(CONFIG))
    args = ap.parse_args()
    out_dir = Path(args.out)

    link = Link(args.e0)
    amap = AddrMap.read(Path(args.e0) / 'addrmap.tsv')
    if not amap.is_identity():
        raise SystemExit('export_retail_relocs: %s is not an E0 (identity map) link' % args.e0)
    names = Names(Path(args.e0) / 'symbols.tsv')
    retail = exe_image(RETAIL_EXE)
    img, t_addr = retail['image'], retail['t_addr']

    def rword(a):
        return struct.unpack_from('<I', img, a - t_addr)[0]

    functions = load_functions()
    fstarts = sorted(functions)
    close = {r[0] for r in amap.rows if r[4] == 'code_entry'}
    linear = {r[0] for r in amap.rows if r[4] == 'code_linear'}

    def func_of(a):
        i = bisect.bisect_right(fstarts, a) - 1
        if i >= 0 and a < functions[fstarts[i]][0]:
            return fstarts[i]
        return None

    # arrays
    arrays = []
    for group, nm, va, size in ARRAYS:
        nxt = names.next_after(va)
        if size is None:
            size = nxt - va
        arrays.append(dict(group=group, name=nm, va=va, size=size, end=va + size, next=nxt,
                           next_name=symadd(*names.owner(va + size)), gp=GP <= va < SBSS_END))
    astarts = sorted((a['va'], a) for a in arrays)

    def array_of(t):
        """(array, offset) when start <= t < end (a one-past-end address is the next symbol's start)."""
        for a in arrays:
            if a['va'] <= t < a['end']:
                return a, t - a['va']
        return None, None

    def array_col(t):
        a, off = array_of(t)
        return symadd(a['name'], off) if a else ''

    # jump-table targets are labels too
    jt_targets = set()
    for r in csv.DictReader((l for l in (CONFIG / 'data_relocs.tsv').open() if not l.startswith('#')), delimiter='\t'):
        if r['kind'] == 'jump_table':
            jt_targets.add(int(r['value'], 16))

    # ---- disassembly scan of every retail function
    scans = {}
    asm_gp_units = ('engine_hit_detection', 'engine_render_scene', 'engine_math_near_pass')
    owner_unit = {}
    for it in link.items:
        if it['type'] == 'c' and it.get('code'):
            for va, end, nm in it['functions']:
                owner_unit[int(va, 16)] = it.get('unit', '')
    for fva in fstarts:
        fend = functions[fva][0]
        words = [rword(a) for a in range(fva, fend, 4)]
        labels = set(jt_targets)
        for i, w in enumerate(words):
            t = branch_target(fva + 4 * i, w)
            if t is not None and fva <= t < fend:
                labels.add(t)
        gp_ok = True  # functions that reuse $28 as data (hand-written asm) write it first: gp_written
        scans[fva] = Scan(fva, fend, words, labels, gp_ok)

    # ---- link records in code
    records = link_records(link)
    code_recs = collections.defaultdict(list)
    data_recs = []
    for p, kind, sname, v, pair, s, it in records:
        if it['seg_kind'] == 'code':
            f = func_of(p)
            code_recs[f].append((p, kind, sname, v, pair, s))
        else:
            data_recs.append((p, kind, sname, v, s, it))

    # ---- cross-check on the identical functions
    agree = collections.Counter()
    total = collections.Counter()
    only_rec = collections.Counter()
    only_dis = collections.Counter()
    examples = collections.defaultdict(list)
    for fva in sorted(linear & set(fstarts)):
        recset = {(p, k, v) for p, k, sn, v, pair, s in code_recs.get(fva, []) if k != 'word32'}
        dis = {}
        for va, k, tgt, pair, cls in scans[fva].relocs:
            dis[(va, k, tgt)] = cls
        for key, cls in dis.items():
            total[cls] += 1
            if key in recset:
                agree[cls] += 1
            else:
                only_dis[cls] += 1
                if len(examples['dis_' + cls]) < 5:
                    examples['dis_' + cls].append('%s %s %s' % (h8(key[0]), key[1], h8(key[2])))
        for key in recset - set(dis):
            only_rec[key[1]] += 1
            if len(examples['rec_' + key[1]]) < 5:
                examples['rec_' + key[1]].append('%s %s %s' % (h8(key[0]), key[1], h8(key[2])))
    n_dis = sum(total.values())
    n_rec = sum(len([1 for r in code_recs.get(f, []) if r[1] != 'word32']) for f in linear & set(fstarts))
    n_agree = sum(agree.values())
    conf_of = {}
    for cls in total:
        rate = agree[cls] / total[cls]
        conf_of[cls] = 'high' if rate >= 0.995 else 'medium' if rate >= 0.95 else 'low'

    # ---- retail_relocs.tsv
    rows = []
    for fva in fstarts:
        fname = names.best.get(fva, functions[fva][1])
        if fva in close or fva not in linear:
            for va, k, tgt, pair, cls in sorted(scans[fva].relocs):
                sym, off = names.owner(tgt)
                rows.append((va, k, sym, off, tgt, pair, array_col(tgt), fname, 'disasm', conf_of.get(cls, 'low'), cls))
        else:
            for p, k, sn, v, pair, s in sorted(code_recs.get(fva, []), key=lambda r: (r[0], r[1])):
                if sn is None or sn == '':
                    sym, off0 = names.owner(s)
                    sym, off = sym, v - (s - off0)
                else:
                    sym, off = names.name(sn), (v - s) if k != 'jal26' else 0
                    if k == 'jal26':
                        off = v - s
                rows.append((p, k, sym, off, v, pair, array_col(v), fname, 'reloc', 'certain', ''))
    # unlisted text gaps carry no code references (pointer census); records there would be in code_recs[None]
    for p, k, sn, v, pair, s in code_recs.get(None, []):
        rows.append((p, k, names.name(sn or names.owner(s)[0]), v - s, v, pair, array_col(v), '-', 'reloc', 'certain', 'gap'))
    rows.sort(key=lambda r: (r[0], r[1]))
    kinds = collections.Counter(r[1] for r in rows)
    srcs = collections.Counter(r[8] for r in rows)
    hdr = [
        '# Generated by `py -3.14 tools/export_retail_relocs.py --e0 <relocatable E0 link>`; do not edit.',
        '# Every relocation in the RETAIL code (800CDC54..801625B8), at retail VAs (port request 24, docs/RELOCATIONS.md).',
        '# va: instruction VA. kind: hi16 (lui), lo16 (addiu/ori/load/store using the lui register; the full address',
        '#   is hi+sext(lo)), gprel16 (offset from gp 801809D4), jal26 (j or jal; target), word32 (word in code; none).',
        '# symbol + addend = target: the address the instruction (pair) forms. For source=reloc the symbol is the one',
        '#   the relocation record names (current names.tsv name); for source=disasm the nearest retail label at or',
        '#   below the target. A hi16 and its lo16s form the same target; pair_va: for lo16 the lui it pairs with, for',
        '#   hi16 its first lo16 (one lui may feed several lo16 rows). array: the request-24 array (ARRAYS in the tool)',
        '#   that holds the target (start <= target < end; an end address is the next symbol and is not marked).',
        '# source: reloc = relocation record of the source-only relocatable link at retail layout (E0, identity map),',
        '#   for functions whose bytes equal retail (%d functions); disasm = derived from the retail disassembly for the'
        % len(linear & set(fstarts)),
        '#   %d close functions whose compiled bytes differ (lui->lo forward data-flow, gp operands, j/jal).' % len(close),
        '# confidence: certain (reloc); disasm rows take the agreement of their class with the records on the identical',
        '#   functions: high >= 99.5%, medium >= 95%, else low. method: adjacent / cross (a label or jump between lui',
        '#   and use) / indexed (through addu) / ori / gp / jump.',
        '# Disasm vs reloc on the %d identical functions: %d of %d disasm rows agree (%.2f%%), %d of %d records found'
        % (len(linear & set(fstarts)), n_agree, n_dis, 100.0 * n_agree / max(1, n_dis), n_agree, n_rec),
        '#   (%.2f%%); per class: %s.' % (100.0 * n_agree / max(1, n_rec), ', '.join(
            '%s %d/%d' % (c, agree[c], total[c]) for c in sorted(total))),
        '# Rows: %d (%s); %s.' % (len(rows), ', '.join('%s %d' % kv for kv in sorted(kinds.items())),
                                 ', '.join('%s %d' % kv for kv in sorted(srcs.items()))),
        'va\tkind\tsymbol\taddend\ttarget\tpair_va\tarray\tfunction\tsource\tconfidence\tmethod',
    ]
    lines = hdr + ['%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s' % (
        h8(va), k, sym, ('0x%X' % off if off >= 0 else '-0x%X' % -off), h8(tgt), h8(pair) if pair else '',
        arr, fn, src, conf, m) for va, k, sym, off, tgt, pair, arr, fn, src, conf, m in rows]
    (out_dir / 'retail_relocs.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')

    # ---- retail_data_ptrs.tsv
    census = {}
    for r in csv.DictReader((l for l in (CONFIG / 'data_relocs.tsv').open() if not l.startswith('#')), delimiter='\t'):
        census[int(r['address'], 16)] = r
    slots = {}
    for r in csv.DictReader((l for l in (Path(args.e0) / 'pointer_slots.tsv').open() if not l.startswith('#')),
                            delimiter='\t'):
        slots[int(r['r_addr'], 16)] = r
    rec_words = {p: (sn, v, s, it) for p, k, sn, v, s, it in data_recs if k == 'word32'}
    seg_of = {}
    for it in link.items:
        if it['type'] in ('c', 'r') and not it.get('code'):
            seg_of[(it['r_start'], it['r_end'])] = it['seg_kind']
    seg_ranges = sorted(seg_of)

    def seg(a):
        i = bisect.bisect_right(seg_ranges, (a, M32)) - 1
        if i >= 0 and seg_ranges[i][0] <= a < seg_ranges[i][1]:
            return seg_of[seg_ranges[i]]
        return 'text_gap'
    drows = []
    new_found = 0
    for a in sorted(set(census) | set(rec_words)):
        val = rword(a) if a - t_addr + 4 <= len(img) else None
        c = census.get(a)
        s = slots.get(a)
        if s:
            sym, off = s['target_symbol'], int(s['addend'], 16)
        elif a in rec_words:
            sn, v, sa, it = rec_words[a]
            sym, off = (names.name(sn), v - sa) if sn else names.owner(v)
        else:
            sym, off = names.owner(val)
        tgt = val
        sym = names.name(sym)
        if c is None:
            new_found += 1
        drows.append((a, seg(a), tgt, c['kind'] if c else 'data_ptr', sym, off, array_col(tgt),
                      c['confidence'] if c else 'certain', 'census+reloc' if (c and a in rec_words) else
                      ('census' if c else 'reloc (new)')))
    into_arrays = [r for r in drows if r[6]]
    dk = collections.Counter(r[3] for r in drows)
    hdr = [
        '# Generated by `py -3.14 tools/export_retail_relocs.py --e0 <relocatable E0 link>`; do not edit.',
        '# Every retail data word that holds an address into the image or BSS (800CDC54..801FAD48): rodata, data,',
        '# sdata and the text gaps (config/US/data_relocs.tsv, tools/pointer_census.py) united with every R_MIPS_32',
        '# record of the E0 relocatable link in a data section. BSS holds no initialised words: pointers stored there',
        '# at run time are formed by code (retail_relocs.tsv) - see move_hazards.tsv kind stored_pointer.',
        '# address: retail VA of the word. segment. value: the word (= target). kind: jump_table, vtable, code_ptr,',
        '# string_ptr, data_ptr. symbol + addend = value (symbol as in the E0 pointer_slots.tsv, current names).',
        '# array: the request-24 array holding the target (empty: none). source: census / reloc / both.',
        '# Rows: %d (%s); new since the census: %d; pointing into a request-24 array: %d.' % (
            len(drows), ', '.join('%s %d' % kv for kv in sorted(dk.items())), new_found, len(into_arrays)),
        'address\tsegment\tvalue\tkind\tsymbol\taddend\tarray\tconfidence\tsource',
    ]
    lines = hdr + ['%s\t%s\t%s\t%s\t%s\t0x%X\t%s\t%s\t%s' % (h8(a), sg, h8(t), k, sym, off, arr, conf, src)
                   for a, sg, t, k, sym, off, arr, conf, src in drows]
    (out_dir / 'retail_data_ptrs.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')

    # ---- hazards
    haz = []

    def fn_name(a):
        f = func_of(a)
        return names.best.get(f, functions[f][1]) if f else '-'

    def vdesc(v):
        if v is None:
            return '?'
        if v[0] in ('addr', 'aidx'):
            return ('&' if v[0] == 'addr' else 'idx ') + (array_col(v[1]) or symadd(*names.owner(v[1])))
        if v[0] == 'const':
            return '0x%X' % v[1]
        if v[0] == 'mem':
            return '[%s]' % (array_col(v[1]) or symadd(*names.owner(v[1])))
        return v[0]

    def arr_of_val(v, ends=False):
        if v and v[0] in ('addr', 'aidx'):
            a, off = array_of(v[1])
            if a is None and ends:
                for a2 in arrays:
                    if v[1] == a2['end']:
                        return a2, a2['size']
            return a, off
        return None, None

    # 1. records: the symbol is an array but the target lies at its end or outside it (the C reaches a
    #    neighbour through the array), or the target is in an array but the symbol is a neighbour
    by_name = {a['name']: a for a in arrays}
    for row in rows:
        va, k, sym, off, tgt, pair, arr, fname, src, conf, m = row
        if k == 'jal26':
            continue
        a = by_name.get(sym)
        if a and tgt == a['end']:
            haz.append((va, fname, a['name'], 'end_address',
                        '%s %s = one past the end (%s)' % (k, symadd(sym, off), a['next_name']), 'medium', src))
        elif a and not (a['va'] <= tgt < a['end']):
            a2, o2 = array_of(tgt)
            haz.append((va, fname, a['name'], 'outside_symbol',
                        '%s %s: target %s is %s, outside %s..%s; move them together or rewrite by address' % (
                            k, symadd(sym, off), h8(tgt), array_col(tgt) or symadd(*names.owner(tgt)),
                            h8(a['va']), h8(a['end'])), 'medium' if a2 and a2['group'] == a['group'] else 'high', src))
        elif arr and src == 'reloc' and sym not in by_name:
            sa = tgt - off
            a2, o2 = array_of(tgt)
            if not (a2['va'] <= sa < a2['end']):
                haz.append((va, fname, a2['name'], 'neighbour_symbol',
                            '%s %s reaches %s through another symbol' % (k, symadd(sym, off), arr), 'high', src))

    # 2. shared lui: one hi feeds lo uses into different arrays (or an array and something else)
    for fva, sc in scans.items():
        groups = collections.defaultdict(set)
        for va, k, tgt, pair, cls in sc.relocs:
            if k == 'lo16' and pair:
                a, off = array_of(tgt)
                groups[pair].add(a['name'] if a else ('other:' + names.owner(tgt)[0]))
        for lui, owners in groups.items():
            arr_owners = {o for o in owners if not o.startswith('other:')}
            if arr_owners and len(owners) > 1:
                haz.append((lui, fn_name(lui), ','.join(sorted(arr_owners)), 'shared_hi',
                            'one lui feeds lo16 uses into %s: moving one of them needs its own hi' % ', '.join(sorted(owners)),
                            'medium', 'disasm'))

    # 3. compares / pointer differences involving an array address
    for fva, sc in scans.items():
        for va, mn, a, b, ra, rb in sc.compares:
            aa, ao = arr_of_val(a, True)
            ba, bo = arr_of_val(b, True)
            if not aa and not ba:
                continue
            arrs = sorted({x['name'] for x in (aa, ba) if x})
            same = aa is not None and ba is not None and aa is ba
            if mn == 'subu':
                kind, sev = 'pointer_difference', 'low' if same else 'medium'
            else:
                kind = 'address_compare'
                if b is None or a is None or (a and a[0] in ('ret', 'hi', 'hidx', 'mem')) or (b and b[0] in ('ret', 'hi', 'hidx', 'mem')):
                    sev = 'low'
                elif same:
                    sev = 'low'
                elif (a and a[0] == 'const' and a[1] != 0) or (b and b[0] == 'const' and b[1] != 0):
                    sev = 'high'
                elif (a and a[0] == 'const') or (b and b[0] == 'const'):
                    continue  # compare with zero (NULL test of a pointer register)
                else:
                    sev = 'medium'
            haz.append((va, fn_name(va), ','.join(arrs), kind, '%s %s, %s' % (mn, vdesc(a), vdesc(b)), sev, 'disasm'))

    # 4. memory functions over an array with a constant size
    fn_by_addr = {}
    for nm in MEMFUNCS:
        for a, n in names.best.items():
            if n == nm:
                fn_by_addr[a] = nm
    for r in csv.DictReader((CONFIG / 'ida_names.tsv').open(), delimiter='\t'):
        if r['name'] in MEMFUNCS:
            fn_by_addr[int(r['va'], 16)] = r['name']
    for fva, sc in scans.items():
        for va, tgt, argv in sc.calls:
            if tgt not in fn_by_addr or argv is None:
                continue
            nm = fn_by_addr[tgt]
            dreg, sreg, nreg, unit = MEMFUNCS[nm]
            for role, reg in (('dst', dreg), ('src', sreg)):
                if reg is None:
                    continue
                v = argv.get(reg)
                a, off = arr_of_val(v)
                if a is None:
                    continue
                n = argv.get(nreg) if nreg else None
                if n and n[0] == 'const':
                    nbytes = n[1] * unit
                    over = off + nbytes > a['size']
                    detail = '%s(%s=%s, %d bytes) %s %s' % (
                        nm, role, vdesc(v), nbytes, 'RUNS PAST the end into' if over else 'stays inside;',
                        a['next_name'] if over else 'size by constant (grow it with the array)')
                    sev = 'high' if over else 'medium'
                else:
                    detail = '%s(%s=%s, size %s)' % (nm, role, vdesc(v), vdesc(n) if n else 'n/a')
                    sev = 'low'
                haz.append((va, fn_name(va), a['name'], 'size_by_hand', detail, sev, 'disasm'))

    # 5. pointers into the arrays stored to memory at run time
    for fva, sc in scans.items():
        for va, v, dest, dtgt in sc.stores:
            a, off = arr_of_val(v)
            if a is None:
                continue
            haz.append((va, fn_name(va), a['name'], 'stored_pointer',
                        'stores %s to %s%s' % (vdesc(v), dest, (' (%s)' % array_col(dtgt) if dtgt and array_col(dtgt)
                                                                 else (' (%s)' % symadd(*names.owner(dtgt)) if dtgt else ''))),
                        'low', 'disasm'))

    # 6. gp-relative arrays
    gp_uses, lo_uses, users = collections.Counter(), collections.Counter(), collections.defaultdict(set)
    for row in rows:
        if row[6] and row[1] in ('gprel16', 'lo16'):
            an = array_of(row[4])[0]['name']
            (gp_uses if row[1] == 'gprel16' else lo_uses)[an] += 1
            users[an].add(row[7])
    for a in arrays:
        a['gp_uses'], a['lo_uses'], a['users'] = gp_uses[a['name']], lo_uses[a['name']], len(users[a['name']])
        if a['gp']:
            haz.append((a['va'], '-', a['name'], 'gp_relative',
                        'in %s (gp 801809D4 +0x%X); %d gprel16 and %d lo16 accesses. A signed 16-bit gp offset reaches '
                        '801789D4..801889D3 only: once moved above 2 MB every gprel16 access must become an absolute '
                        'address (lui/addiu, or a constant in the recompiled code)%s' % (
                            'sdata' if a['va'] < BSS_START else 'sbss', a['va'] - GP, a['gp_uses'], a['lo_uses'],
                            '' if a['gp_uses'] else '; this one is only reached through hi/lo pairs'),
                        'high' if a['gp_uses'] else 'low', 'layout'))

    # 6b. initialised (sdata/data) arrays: their retail values must be copied when they move
    for a in arrays:
        if a['va'] < BSS_START:
            vals = [rword(x) for x in range(a['va'], a['end'], 4)]
            haz.append((a['va'], '-', a['name'], 'initialised',
                        'in the file image (not BSS): moving it must copy its initial words %s' % ' '.join(
                            '%08X' % v for v in vals), 'medium', 'layout'))

    # 7. initialised data pointing into the arrays; code pointers / jump tables in data
    for r in into_arrays:
        haz.append((r[0], '-', array_of(r[2])[0]['name'], 'data_pointer',
                    '%s word %s -> %s' % (r[1], h8(r[2]), r[6]), 'medium', 'data'))
    jt = sum(1 for r in drows if r[3] == 'jump_table')
    cp = sum(1 for r in drows if r[3] in ('code_ptr', 'vtable'))
    haz.append((0x800CE910, '-', '-', 'code_pointers_in_data',
                '%d jump-table words (rodata) and %d code/vtable pointers in data (retail_data_ptrs.tsv): they move '
                'with code, not with the arrays; listed for completeness' % (jt, cp), 'info', 'data'))

    # 8. layout: each array, its size and what follows it
    for a in arrays:
        haz.append((a['va'], '-', a['name'], 'layout',
                    '%s, %d bytes to %s, followed by %s; next label after the start %s %s' % (
                        a['group'], a['size'], h8(a['end']), a['next_name'], h8(a['next']) if a['next'] else '-',
                        names.best.get(a['next'], '')),
                    'info', 'layout'))
    for va, fn, arr, kind, detail, sev, src in DOC_HAZARDS:
        haz.append((int(va, 16), fn, arr, kind, detail, sev, src))

    haz.sort(key=lambda r: (r[3] not in ('layout',), r[2], r[0], r[3]))
    hk = collections.Counter(r[3] for r in haz)
    hs = collections.Counter(r[5] for r in haz)
    hdr = [
        '# Generated by `py -3.14 tools/export_retail_relocs.py --e0 <relocatable E0 link>`; do not edit.',
        '# What makes moving the request-24 arrays unsafe (docs/RELOCATIONS.md "Move hazards"). Static analysis of',
        '# the retail code (retail_relocs.tsv + a register data-flow per function) and the docs.',
        '# va: instruction (or array) VA. kind: end_address (a one-past-end address formed in code), outside_symbol,',
        '#   neighbour_symbol (record symbol and target disagree), shared_hi (one lui used for several arrays),',
        '#   address_compare (slt/sltu/beq/bne with an array address), pointer_difference (subu of two addresses),',
        '#   size_by_hand (memset/memcpy/ClearOTag.. over an array with a constant size), stored_pointer (an array',
        '#   address stored to memory at run time), gp_relative, data_pointer, code_pointers_in_data,',
        '#   index_past_end / shared_storage (docs), layout (array, size, next label).',
        '# severity: high (breaks a move unless rewritten), medium (check), low/info (safe if every use moves).',
        '# Rows: %d (%s); severity %s.' % (len(haz), ', '.join('%s %d' % kv for kv in sorted(hk.items())),
                                          ', '.join('%s %d' % kv for kv in sorted(hs.items()))),
        'va\tfunction\tarray\tkind\tdetail\tseverity\tsource',
    ]
    lines = hdr + ['%s\t%s\t%s\t%s\t%s\t%s\t%s' % (h8(va), fn, arr, k, d.replace('\t', ' '), sev, src)
                   for va, fn, arr, k, d, sev, src in haz]
    (out_dir / 'move_hazards.tsv').write_text('\n'.join(lines) + '\n', encoding='ascii', newline='\n')

    # ---- summary
    print('functions: %d identical (reloc), %d close (disasm)' % (len(linear & set(fstarts)), len(close)))
    print('retail_relocs.tsv: %d rows %s %s' % (len(rows), dict(kinds), dict(srcs)))
    print('cross-check: disasm %d rows, %d agree (%.2f%%); records %d, found %.2f%%' % (
        n_dis, n_agree, 100.0 * n_agree / max(1, n_dis), n_rec, 100.0 * n_agree / max(1, n_rec)))
    for c in sorted(total):
        print('  class %-9s %6d/%6d %.2f%% -> %s' % (c, agree[c], total[c], 100.0 * agree[c] / total[c], conf_of[c]))
    print('  records the scan missed: %s' % dict(only_rec))
    for k, v in sorted(examples.items()):
        print('  e.g. %s: %s' % (k, '; '.join(v)))
    print('retail_data_ptrs.tsv: %d rows %s, new %d, into arrays %d' % (len(drows), dict(dk), new_found, len(into_arrays)))
    print('move_hazards.tsv: %d rows %s severity %s' % (len(haz), dict(hk), dict(hs)))
    for a in arrays:
        print('  %-26s %s size 0x%-5X then %-28s lo16 %4d gprel16 %3d functions %3d%s' % (
            a['name'], h8(a['va']), a['size'], a['next_name'], a['lo_uses'], a['gp_uses'], a['users'],
            (' sdata' if a['va'] < BSS_START else ' sbss') if a['gp'] else ''))


if __name__ == '__main__':
    main()
