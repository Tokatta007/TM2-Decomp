#!/usr/bin/env python3
"""Survey the retail SCUS_94306 image layout for the source-only build.

Derives every segment boundary from evidence in the retail executable and the
function lists, asserts that the segments tile the address space from t_addr
to the end of BSS with no gaps or overlaps, and writes:

  config/US/layout.tsv       name, start, end, kind (one row per segment)
  config/US/exe_header.json  PS-X EXE header fields the build must reproduce

Evidence used (see docs/SOURCE_BUILD.md):
  * header: t_addr, t_size, pc0, s_addr, region string
  * start (pc0): lui/addiu pairs give the BSS clear range and the gp value
  * function lists (ida_funcs + extra_funcs - not_funcs): code ranges
  * gp-relative loads/stores/addiu in code: small-data range
  * the file image beyond the last initialised byte is zero padding to 0x800

Reads bin/SCUS_94306R.EXE (retail, never committed). Writes only the two
config files above (or checks them with --check).
"""
import argparse
import csv
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RETAIL = ROOT / 'bin/SCUS_94306R.EXE'
RETAIL_SHA256 = '12864f090a02c94b8b6e6d21d40119c12e59fd5a7327db4f80052c7e674e1bac'
CONFIG = ROOT / 'config/US'
HEADER_SIZE = 0x800
FILE_ALIGN = 0x800
GP_REG = 28


def fail(message):
    raise SystemExit('survey_layout: ' + message)


def load_functions():
    """Function start -> (end, name) from ida_funcs + extra_funcs - not_funcs."""
    not_funcs = {int(r['va'], 16) for r in csv.DictReader((CONFIG / 'not_funcs.tsv').open(), delimiter='\t')}
    funcs = {}
    for name in ('ida_funcs.tsv', 'extra_funcs.tsv'):
        for r in csv.DictReader((CONFIG / name).open(), delimiter='\t'):
            va = int(r['va'], 16)
            if va in not_funcs:
                continue
            funcs[va] = (va + 4 * int(r['words']), r['name'])
    return funcs


MAX_CODE_GAP = 0x100  # unlisted words between functions (inline data, padding) stay in the code segment


def merge_ranges(funcs, gaps):
    """Merge function ranges separated by < MAX_CODE_GAP bytes; return [start, end, count] list.

    Small gaps that were bridged are appended to `gaps` as (start, end).
    """
    out = []
    for start in sorted(funcs):
        end = funcs[start][0]
        if out and start < out[-1][1]:
            fail('overlapping functions at %08X' % start)
        if out and start - out[-1][1] < MAX_CODE_GAP:
            if start > out[-1][1]:
                gaps.append((out[-1][1], start))
            out[-1][1] = end
            out[-1][2] += 1
        else:
            out.append([start, end, 1])
    return out


def decode_start(image, t_addr, pc0):
    """Return (bss_start, bss_end, gp) from the crt0 lui/addiu pairs at pc0."""
    pending = {}
    found = {}
    for i in range(64):
        word = struct.unpack_from('<I', image, pc0 - t_addr + 4 * i)[0]
        op, rs, rt, imm = word >> 26, (word >> 21) & 31, (word >> 16) & 31, word & 0xFFFF
        if op == 0x0F:  # lui
            pending[rt] = imm << 16
        elif op == 0x09 and rs == rt and rt in pending:  # addiu r,r,lo
            value = (pending.pop(rt) + (imm - 0x10000 if imm & 0x8000 else imm)) & 0xFFFFFFFF
            found.setdefault(rt, []).append(value)
    # crt0: v0 = BSS start, v1 = BSS end (the sw zero loop), gp = $28
    try:
        return found[2][0], found[3][0], found[GP_REG][0]
    except (KeyError, IndexError):
        fail('could not decode crt0 constants at pc0 %08X' % pc0)


def gp_references(image, t_addr, code_ranges, gp):
    """Targets of gp-relative loads/stores/addiu in all code ranges."""
    mem_ops = set(range(0x20, 0x27)) | set(range(0x28, 0x2F)) | {0x32, 0x3A}  # lb..lwr, sb..swr, lwc2, swc2
    targets = []
    for start, end, _ in code_ranges:
        for va in range(start, end, 4):
            word = struct.unpack_from('<I', image, va - t_addr)[0]
            op, rs, imm = word >> 26, (word >> 21) & 31, word & 0xFFFF
            if rs != GP_REG or not (op in mem_ops or op == 0x09):
                continue
            if op == 0x09 and ((word >> 16) & 31) == GP_REG:
                continue  # gp setup itself
            targets.append((gp + (imm - 0x10000 if imm & 0x8000 else imm)) & 0xFFFFFFFF)
    return targets


def survey():
    raw = RETAIL.read_bytes()
    if hashlib.sha256(raw).hexdigest() != RETAIL_SHA256:
        fail('retail EXE sha256 mismatch')
    header = raw[:HEADER_SIZE]
    image = raw[HEADER_SIZE:]
    if header[:8] != b'PS-X EXE':
        fail('bad magic')
    (pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size,
     s_addr, s_size) = struct.unpack_from('<10I', header, 0x10)
    if t_size != len(image) or t_size % FILE_ALIGN:
        fail('t_size %X does not match the file image' % t_size)
    region = header[0x4C:HEADER_SIZE].split(b'\0', 1)[0]
    # Every non-zero header byte must be accounted for by a known field.
    known = set(range(0, 8)) | set(range(0x10, 0x14)) | set(range(0x18, 0x20)) | set(range(0x30, 0x34)) \
        | set(range(0x4C, 0x4C + len(region)))
    other = {'0x%03X' % i: header[i] for i in range(HEADER_SIZE) if header[i] and i not in known}
    if other or gp0 or d_addr or d_size or b_addr or b_size or s_size:
        fail('unexpected non-zero header fields: %r' % other)
    file_end = t_addr + t_size

    funcs = load_functions()
    gaps = []
    code = merge_ranges(funcs, gaps)
    if pc0 not in funcs:
        fail('pc0 %08X is not a listed function' % pc0)
    bss_start, bss_end, gp = decode_start(image, t_addr, pc0)

    # Code: expect exactly two merged ranges, the first at t_addr.
    if len(code) != 2 or code[0][0] != t_addr:
        fail('expected two code ranges starting at t_addr, got %s' %
             ['%08X-%08X' % (a, b) for a, b, _ in code])
    (emit_start, emit_end, emit_n), (text_start, text_end, text_n) = code
    rodata_start, rodata_end = emit_end, text_start

    # rodata between the model emitter and the main code: check it holds
    # jump-table entries into code and strings, and no instructions of listed
    # functions (guaranteed by the merge above).
    words = struct.unpack_from('<%dI' % ((rodata_end - rodata_start) // 4), image, rodata_start - t_addr)
    jump_entries = sum(1 for w in words if text_start <= w < text_end)

    # Small data: gp-relative references.
    refs = gp_references(image, t_addr, code, gp)
    if not refs:
        fail('no gp-relative references found')
    gp_min, gp_max = min(refs), max(refs)
    if gp_min < gp:
        fail('gp-relative access below gp (%08X)' % gp_min)
    if not gp < bss_start < bss_end:
        fail('crt0 constants out of order')
    refs_sdata = sum(1 for r in refs if gp <= r < bss_start)
    refs_sbss = sum(1 for r in refs if bss_start <= r < bss_end)

    # BSS start inside the file image: the tail must be zero, and it must be
    # exactly the 0x800 file-alignment padding after the last initialised byte.
    if not bss_start <= file_end <= bss_end:
        fail('file end %08X not inside BSS' % file_end)
    tail = image[bss_start - t_addr:]
    if any(tail):
        fail('non-zero bytes in the file image after BSS start')
    aligned = t_addr + ((bss_start - t_addr + FILE_ALIGN - 1) // FILE_ALIGN) * FILE_ALIGN
    if aligned != file_end:
        fail('file end is not ALIGN(bss_start - t_addr, 0x800)')

    segments = [
        ('model_emitter', emit_start, emit_end, 'code'),
        ('rodata', rodata_start, rodata_end, 'rodata'),
        ('text', text_start, text_end, 'code'),
        ('data', text_end, gp, 'data'),
        ('sdata', gp, bss_start, 'sdata'),
        ('bss_in_file', bss_start, file_end, 'bss_in_file'),
        ('bss', file_end, bss_end, 'bss'),
    ]
    # Tiling assertion: contiguous, non-empty, word aligned, t_addr..bss_end.
    cursor = t_addr
    for name, start, end, _ in segments:
        if start != cursor or end <= start or start % 4 or end % 4:
            fail('segment %s %08X-%08X breaks the tiling at %08X' % (name, start, end, cursor))
        cursor = end
    if cursor != bss_end:
        fail('segments end at %08X, BSS ends at %08X' % (cursor, bss_end))
    in_file = [s for s in segments if s[1] < file_end]
    if in_file[-1][2] != file_end:
        fail('file image end does not fall on a segment boundary')

    evidence = dict(
        pc0=pc0, t_addr=t_addr, t_size=t_size, s_addr=s_addr, file_end=file_end,
        bss_start=bss_start, bss_end=bss_end, gp=gp,
        emit_funcs=emit_n, text_funcs=text_n, rodata_jump_entries=jump_entries,
        gp_refs=len(refs), gp_min=gp_min, gp_max=gp_max, gp_refs_sdata=refs_sdata,
        gp_refs_sbss=refs_sbss, pc0_name=funcs[pc0][1],
        last_func=max(funcs), code_gaps=gaps, last_func_name=funcs[max(funcs)][1],
    )
    exe_header = dict(
        magic='PS-X EXE',
        pc0=dict(symbol=funcs[pc0][1], address='0x%08X' % pc0),
        gp0='0x%08X' % gp0,
        t_addr='0x%08X' % t_addr,
        t_size='0x%X' % t_size,
        s_addr='0x%08X' % s_addr,
        s_size='0x%X' % s_size,
        region=region.decode('ascii'),
        region_offset='0x%X' % 0x4C,
        header_size='0x%X' % HEADER_SIZE,
        file_alignment='0x%X' % FILE_ALIGN,
        other_nonzero_header_bytes=other,
        note='All other header bytes are zero. t_size is ALIGN(end of last loadable '
             'section - t_addr, file_alignment). gp (0x%08X) is not in the header; crt0 '
             'loads it.' % gp,
    )
    return segments, exe_header, evidence


def layout_text(segments, ev):
    lines = [
        '# TM2 US (SCUS_94306) retail memory layout. Generated by tools/survey_layout.py; do not edit.',
        '# Segments tile [t_addr, bss_end) exactly; see docs/SOURCE_BUILD.md for the evidence.',
        '# model_emitter: t_addr 0x%08X (header) .. end of sub_800CDC54 (%d function(s), function lists)'
        % (ev['t_addr'], ev['emit_funcs']),
        '# rodata: gap between the model emitter and the next listed function; %d words are'
        ' addresses inside text (switch jump tables); first label is a string (aInternalErrorW)'
        % ev['rodata_jump_entries'],
        '# text: first function after rodata (rtMainLoop) .. end of the last listed function'
        ' (%s @ %08X); %d functions' % (ev['last_func_name'], ev['last_func'], ev['text_funcs']),
        '#   %d unlisted gaps inside text (%d bytes total, largest %d; inline data/padding, e.g. crt0 stack'
        ' table at 80138838): %s' % (len(ev['code_gaps']), sum(b - a for a, b in ev['code_gaps']),
                                     max(b - a for a, b in ev['code_gaps']),
                                     ' '.join('%08X-%08X' % g for g in ev['code_gaps'])),
        '# data: end of text .. gp; no listed function inside (not_funcs.tsv drops IDA false positives)',
        '# sdata: gp 0x%08X (crt0 lui/addiu $28 at pc0) .. BSS start 0x%08X (crt0 clear-loop start);'
        ' %d gp-relative refs, %d in sdata, %d in sbss, range %08X..%08X'
        % (ev['gp'], ev['bss_start'], ev['gp_refs'], ev['gp_refs_sdata'], ev['gp_refs_sbss'], ev['gp_min'], ev['gp_max']),
        '# bss_in_file: BSS start .. file end 0x%08X = t_addr + t_size; all zero, exactly the'
        ' 0x800 file-alignment padding' % ev['file_end'],
        '# bss: file end .. BSS end 0x%08X (crt0 clear-loop end); not in the file' % ev['bss_end'],
        'name\tstart\tend\tkind',
    ]
    for name, start, end, kind in segments:
        lines.append('%s\t%08X\t%08X\t%s' % (name, start, end, kind))
    return '\n'.join(lines) + '\n'


def read_layout(path=CONFIG / 'layout.tsv'):
    """Parse layout.tsv into a list of dicts with int start/end."""
    rows = [line for line in path.read_text(encoding='ascii').splitlines() if line and not line.startswith('#')]
    out = []
    for row in csv.DictReader(rows, delimiter='\t'):
        out.append(dict(name=row['name'], start=int(row['start'], 16), end=int(row['end'], 16), kind=row['kind']))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--check', action='store_true', help='verify the committed files instead of writing them')
    args = parser.parse_args()
    segments, exe_header, ev = survey()
    layout = layout_text(segments, ev)
    header_json = json.dumps(exe_header, indent=2) + '\n'
    targets = {CONFIG / 'layout.tsv': layout, CONFIG / 'exe_header.json': header_json}
    if args.check:
        stale = [p.name for p, text in targets.items() if not p.is_file() or p.read_text(encoding='ascii') != text]
        if stale:
            fail('stale: ' + ', '.join(stale))
        print('layout OK')
        return
    for path, text in targets.items():
        path.write_text(text, encoding='ascii', newline='\n')
    print('%-14s %-8s %-8s %-8s %s' % ('name', 'start', 'end', 'size', 'kind'))
    for name, start, end, kind in segments:
        print('%-14s %08X %08X %8X %s' % (name, start, end, end - start, kind))
    print('gp refs: %d (%d sdata, %d sbss), range %08X..%08X'
          % (ev['gp_refs'], ev['gp_refs_sdata'], ev['gp_refs_sbss'], ev['gp_min'], ev['gp_max']))


if __name__ == '__main__':
    sys.exit(main())
