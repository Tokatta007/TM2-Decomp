"""Small strict ELF32/MIPS reader for semantic artifact audits (no rewriting)."""
from pathlib import Path
import struct


def read_elf(path):
    raw = Path(path).read_bytes()
    if len(raw) < 52 or raw[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('Expected ELF32 little-endian version 1')
    hdr = struct.unpack_from('<HHIIIIIHHHHHH', raw, 16)
    if hdr[1] != 8 or hdr[2] != 1:
        raise ValueError('Expected MIPS ELF version 1')
    shoff, shentsize, shnum, shstrndx = hdr[5], hdr[10], hdr[11], hdr[12]
    if shentsize != 40 or not shnum or shstrndx >= shnum or shoff + shnum * 40 > len(raw):
        raise ValueError('Invalid/unsupported ELF section table')

    def span(offset, size):
        if offset < 0 or size < 0 or offset + size > len(raw):
            raise ValueError('ELF file range out of bounds')
        return raw[offset:offset + size]

    sections = []
    for i in range(shnum):
        words = struct.unpack_from('<10I', raw, shoff + i * 40)
        keys = ('name_offset', 'type', 'flags', 'address', 'offset', 'size', 'link', 'info', 'alignment', 'entry_size')
        section = dict(zip(keys, words))
        section['index'] = i
        section['data'] = b'' if section['type'] == 8 else span(section['offset'], section['size'])
        sections.append(section)
    names = sections[shstrndx]['data']

    def string(table, offset):
        if offset >= len(table):
            raise ValueError('ELF string offset out of bounds')
        end = table.find(b'\0', offset)
        if end < 0:
            raise ValueError('Unterminated ELF string')
        return table[offset:end].decode('utf-8')

    for section in sections:
        section['name'] = string(names, section['name_offset'])
    symbols = []
    for section in sections:
        if section['type'] != 2:
            continue
        if section['entry_size'] != 16 or section['size'] % 16 or section['link'] >= shnum:
            raise ValueError('Invalid ELF symbol table')
        strings = sections[section['link']]['data']
        for offset in range(0, section['size'], 16):
            name, value, size, info, other, index = struct.unpack_from('<IIIBBH', section['data'], offset)
            symbols.append(dict(name=string(strings, name), address=value, size=size,
                                binding=info >> 4, type=info & 15, section_index=index))
    return dict(entry=hdr[3], sections=sections, symbols=symbols)


def global_symbol(elf, name):
    matches = [s for s in elf['symbols'] if s['name'] == name and s['binding'] in (1, 2)]
    if len(matches) != 1:
        raise ValueError('Expected unique global symbol: ' + name)
    return matches[0]


def read_relocs(elf):
    """REL relocations of an ELF from read_elf(): {target section index: [(offset, type, symbol), ...]}.

    `symbol` is the read_elf() symbol dict (index 0 = the null symbol). RELA sections are refused."""
    out = {}
    syms = elf['symbols']
    for section in elf['sections']:
        if section['type'] == 4:
            raise ValueError('RELA sections are not supported: ' + section['name'])
        if section['type'] != 9:
            continue
        if section['entry_size'] != 8 or section['size'] % 8:
            raise ValueError('Invalid REL section ' + section['name'])
        rows = out.setdefault(section['info'], [])
        for offset in range(0, section['size'], 8):
            r_offset, info = struct.unpack_from('<II', section['data'], offset)
            rows.append((r_offset, info & 0xff, syms[info >> 8]))
    return out
