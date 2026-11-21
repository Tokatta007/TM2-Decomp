#!/usr/bin/env python3
"""Split the retail executable into per-segment binaries for the source-only build.

Verifies bin/SCUS_94306R.EXE against the known sha256, then writes, for every
file-backed segment of config/US/layout.tsv that holds initialised bytes,

  split/bin/<name>_<start>.bin

plus split/SPLIT.json (retail hash, layout hash, per-segment size and sha256).
bss_in_file is checked to be all zero and recorded without a file; bss is not
in the file at all. split/ holds retail bytes and is gitignored: never commit it.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT, RETAIL, RETAIL_SHA256, CONFIG, HEADER_SIZE, read_layout

SPLIT = ROOT / 'split'
SPLIT_JSON = SPLIT / 'SPLIT.json'
FILE_KINDS = ('code', 'rodata', 'data', 'sdata')


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def bin_name(seg):
    return 'bin/%s_%08X.bin' % (seg['name'], seg['start'])


def split():
    raw = RETAIL.read_bytes()
    if sha256(raw) != RETAIL_SHA256:
        raise SystemExit('split_retail: %s sha256 mismatch (expected %s)' % (RETAIL, RETAIL_SHA256))
    header = json.loads((CONFIG / 'exe_header.json').read_text(encoding='ascii'))
    t_addr = int(header['t_addr'], 16)
    image = raw[HEADER_SIZE:]
    file_end = t_addr + len(image)
    layout = read_layout()
    (SPLIT / 'bin').mkdir(parents=True, exist_ok=True)
    rows = []
    for seg in layout:
        start, end, kind = seg['start'], seg['end'], seg['kind']
        row = dict(name=seg['name'], start='%08X' % start, end='%08X' % end, kind=kind, size=end - start)
        if kind == 'bss':
            if start < file_end:
                raise SystemExit('split_retail: bss segment %s starts inside the file' % seg['name'])
            row.update(file=None)
        else:
            if end > file_end:
                raise SystemExit('split_retail: segment %s runs past the file image' % seg['name'])
            data = image[start - t_addr:end - t_addr]
            row.update(sha256=sha256(data))
            if kind == 'bss_in_file':
                if any(data):
                    raise SystemExit('split_retail: %s is not all zero' % seg['name'])
                row.update(file=None, zero=True)
            elif kind in FILE_KINDS:
                path = SPLIT / bin_name(seg)
                path.write_bytes(data)
                row.update(file=bin_name(seg))
            else:
                raise SystemExit('split_retail: unknown kind ' + kind)
        rows.append(row)
    manifest = dict(schema='tm2-split-v1', retail=RETAIL.relative_to(ROOT).as_posix(), retail_sha256=RETAIL_SHA256,
                    layout_sha256=sha256((CONFIG / 'layout.tsv').read_bytes()), t_addr=header['t_addr'],
                    segments=rows, note='Retail bytes: gitignored, never commit.')
    SPLIT_JSON.write_text(json.dumps(manifest, indent=2) + '\n', encoding='ascii', newline='\n')
    return manifest


def check():
    """Return the SPLIT.json manifest if split/ is current for this layout, else None."""
    if not SPLIT_JSON.is_file():
        return None
    manifest = json.loads(SPLIT_JSON.read_text(encoding='ascii'))
    if manifest.get('layout_sha256') != sha256((CONFIG / 'layout.tsv').read_bytes()):
        return None
    for row in manifest['segments']:
        if row.get('file'):
            path = SPLIT / row['file']
            if not path.is_file() or sha256(path.read_bytes()) != row['sha256']:
                return None
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--check', action='store_true', help='only verify existing split/ files')
    args = parser.parse_args()
    manifest = check() if args.check else split()
    if manifest is None:
        raise SystemExit('split_retail: split/ is missing or stale; run tools/split_retail.py')
    for row in manifest['segments']:
        print('%-14s %s-%s %8X %-11s %s' % (row['name'], row['start'], row['end'], row['size'], row['kind'],
                                           row.get('file') or ('zero' if row.get('zero') else 'not in file')))


if __name__ == '__main__':
    main()
