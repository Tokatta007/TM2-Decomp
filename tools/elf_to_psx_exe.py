#!/usr/bin/env python3
"""Convert a linked MIPS ELF into a PS-X EXE.

Allocated PROGBITS sections are written into a zero-filled buffer that runs
from t_addr (config/US/exe_header.json) to ALIGN(end of the last PROGBITS
section, file_alignment). NOBITS (NOLOAD) sections are not written: inside the
image they stay zero, past it they are cleared by crt0. Sections must not
overlap, must not start below t_addr, and must carry no file data if NOBITS.
pc0 is the ELF entry point and must equal the header's pc0 address (and the
pc0 symbol, when the ELF defines it). A relocatable link (--relocated) takes
pc0 from the ELF entry and lets t_size follow the link. Only header fields listed in
exe_header.json are written; every other header byte is zero.
port_base (port variant): allocated sections at or above it (.port_data, .port_bss at the port
base, above the PS1's 2 MB) are not part of the PS-X EXE; they are returned in `port_sections`.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from semantic_elf import read_elf

ROOT = Path(__file__).resolve().parent.parent
SHT_PROGBITS, SHT_NOBITS = 1, 8
STACK_WORD, STACK_RETAIL = 0xB0, 0x00200000  # crt0 (2MBYTE): sp = table[1] | 0x80000000, table at __SN_ENTRY_POINT + 0xAC
SHF_ALLOC, SHF_EXECINSTR = 2, 4


def convert(elf_path, out_path, header_path=ROOT / 'config/US/exe_header.json', allow_size_change=False,
            nobits_aliases=(), relocated=False, port_base=None, stack=None):
    """relocated=True (relocatable link): t_size and pc0 follow the link (pc0 = the ELF entry, which must
    equal the pc0 symbol); everything else is checked as for a fixed link.
    stack (port variant, PC port request 89): (symbol, bytes) of the stack array; crt0's stack word (2MBYTE's table,
    __SN_ENTRY_POINT + 0xB0, retail 0x00200000: crt0 ORs in 0x80000000) and the header's s_addr are set to its end."""
    allow_size_change = allow_size_change or relocated
    cfg = json.loads(Path(header_path).read_text(encoding='ascii'))
    t_addr = int(cfg['t_addr'], 16)
    align = int(cfg['file_alignment'], 16)
    header_size = int(cfg['header_size'], 16)
    pc0 = int(cfg['pc0']['address'], 16)
    if cfg.get('other_nonzero_header_bytes'):
        raise SystemExit('elf_to_psx_exe: other_nonzero_header_bytes not supported')

    elf = read_elf(elf_path)
    alloc = [s for s in elf['sections'] if s['flags'] & SHF_ALLOC and s['size']]
    port = [s for s in alloc if port_base is not None and s['address'] >= port_base]
    alloc = [s for s in alloc if s not in port]
    for s in alloc:
        if s['type'] not in (SHT_PROGBITS, SHT_NOBITS):
            raise SystemExit('elf_to_psx_exe: unsupported allocated section %s (type %d)' % (s['name'], s['type']))
        if s['address'] < t_addr or s['address'] + s['size'] > 0x100000000:
            raise SystemExit('elf_to_psx_exe: section %s at %08X is out of range' % (s['name'], s['address']))
        if s['type'] == SHT_NOBITS and s['data']:
            raise SystemExit('elf_to_psx_exe: NOBITS section %s carries data' % s['name'])
    ordered = sorted(alloc, key=lambda s: (s['address'], -s['size']))
    # nobits_aliases: {inner: outer} NOBITS sections allowed to lie entirely inside another NOBITS section
    # (the same zero-initialised storage claimed by two units); every other overlap is an error.
    by_name = {s['name']: s for s in ordered}
    for inner, outer in dict(nobits_aliases).items():
        i, o = by_name.get(inner), by_name.get(outer)
        if not i or not o or i['type'] != SHT_NOBITS or o['type'] != SHT_NOBITS or not (
                o['address'] <= i['address'] and i['address'] + i['size'] <= o['address'] + o['size']):
            raise SystemExit('elf_to_psx_exe: bad NOBITS alias %s in %s' % (inner, outer))
    prev = None
    for s in ordered:
        if s['name'] in nobits_aliases:
            continue
        if prev and prev['address'] + prev['size'] > s['address']:
            raise SystemExit('elf_to_psx_exe: sections %s and %s overlap' % (prev['name'], s['name']))
        prev = s

    progbits = [s for s in ordered if s['type'] == SHT_PROGBITS]
    if not progbits:
        raise SystemExit('elf_to_psx_exe: no loadable sections')
    end = max(s['address'] + s['size'] for s in progbits)
    t_size = (end - t_addr + align - 1) // align * align
    if t_size != int(cfg['t_size'], 16) and not allow_size_change:
        raise SystemExit('elf_to_psx_exe: t_size 0x%X differs from exe_header.json 0x%X' % (t_size, int(cfg['t_size'], 16)))
    image = bytearray(t_size)
    for s in progbits:
        off = s['address'] - t_addr
        image[off:off + s['size']] = s['data']
    # NOBITS inside the image must be zero: guaranteed since nothing else overlaps them.
    for s in ordered:
        if s['type'] == SHT_NOBITS and s['address'] < t_addr + t_size:
            off = s['address'] - t_addr
            if any(image[off:off + s['size']]):
                raise SystemExit('elf_to_psx_exe: NOBITS section %s is not zero in the image' % s['name'])

    if relocated:
        pc0 = elf['entry']
    if elf['entry'] != pc0:
        raise SystemExit('elf_to_psx_exe: ELF entry %08X != pc0 %08X' % (elf['entry'], pc0))
    named = [x for x in elf['symbols'] if x['name'] == cfg['pc0']['symbol']]
    if named and named[0]['address'] != pc0:
        raise SystemExit('elf_to_psx_exe: symbol %s is at %08X, not pc0' % (cfg['pc0']['symbol'], named[0]['address']))
    if not any(s['flags'] & SHF_EXECINSTR and s['address'] <= pc0 < s['address'] + s['size'] for s in progbits):
        raise SystemExit('elf_to_psx_exe: pc0 is not inside an executable section')

    s_addr = int(cfg['s_addr'], 16)
    stack_info = None
    if stack:
        def sym(n):
            m = [x for x in elf['symbols'] if x['name'] == n]
            if len(m) != 1:
                raise SystemExit('elf_to_psx_exe: %d symbols %s (the stack)' % (len(m), n))
            return m[0]
        name, nbytes = stack
        st, entry = sym(name), sym('__SN_ENTRY_POINT')
        top, word = st['address'] + nbytes, entry['address'] + STACK_WORD
        off = word - t_addr
        if nbytes < 0x1000 or top & 7 or not 0x80200000 <= st['address'] < top <= 0x80C00000:
            raise SystemExit('elf_to_psx_exe: stack %s at %08X + 0x%X is no port RAM stack' % (name, st['address'], nbytes))
        if not 0 <= off < t_size - 4 or struct.unpack_from('<4I', image, off - 4) != (STACK_RETAIL,) * 4:
            raise SystemExit('elf_to_psx_exe: crt0\'s stack table is not at %08X (__SN_ENTRY_POINT + 0x%X)' % (word, STACK_WORD))
        struct.pack_into('<I', image, off, top & 0x7FFFFFFF)
        s_addr = top
        stack_info = dict(symbol=name, top='%08X' % top, bytes=nbytes, crt0_word='%08X' % word,
                          retail_word='%08X' % STACK_RETAIL)

    header = bytearray(header_size)
    header[0:8] = cfg['magic'].encode('ascii')
    struct.pack_into('<10I', header, 0x10, pc0, int(cfg['gp0'], 16), t_addr, t_size, 0, 0, 0, 0,
                     s_addr, 0 if stack else int(cfg['s_size'], 16))
    region = cfg['region'].encode('ascii')
    roff = int(cfg['region_offset'], 16)
    header[roff:roff + len(region)] = region
    Path(out_path).write_bytes(bytes(header) + bytes(image))
    return dict(t_addr=t_addr, t_size=t_size, pc0=pc0, sections=len(alloc), stack=stack_info,
                port_sections=[dict(name=s['name'], address=s['address'], size=s['size'], nobits=s['type'] == SHT_NOBITS,
                                    data=None if s['type'] == SHT_NOBITS else bytes(s['data'])) for s in port])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('elf', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--header', type=Path, default=ROOT / 'config/US/exe_header.json')
    parser.add_argument('--allow-size-change', action='store_true', help='accept a t_size different from exe_header.json')
    parser.add_argument('--relocated', action='store_true',
                        help='relocatable link: t_size and pc0 follow the ELF (implies --allow-size-change)')
    args = parser.parse_args()
    info = convert(args.elf, args.output, args.header, args.allow_size_change, relocated=args.relocated)
    print('PS-X EXE %s: pc0 %08X t_addr %08X t_size 0x%X' % (args.output, info['pc0'], info['t_addr'], info['t_size']))


if __name__ == '__main__':
    main()
