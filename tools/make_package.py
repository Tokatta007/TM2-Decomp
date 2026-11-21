#!/usr/bin/env python3
"""Regenerate PACKAGE.json, the file list that `build.py --verify-package` checks.

    py -3.14 tools/make_package.py           # rewrite PACKAGE.json for the current tree
    py -3.14 tools/make_package.py --check   # exit 1 if PACKAGE.json is stale (writes nothing)

PACKAGE.json lists every file of the offline build package with its size and
sha256. The package is what `build.cmd` / `build.py` (hybrid and --source-only)
and the matching tools need, plus the documentation:

  - root: build.cmd, build.py, BUILD_MANIFEST.json, README.md, THIRD_PARTY.md
  - every file under config/, docs/, include/, src/, tools/

Files are taken from git (tracked files, plus untracked files that .gitignore
does not exclude, so a new tool is picked up before it is committed); the
hashes are of the working-tree bytes, which is what --verify-package compares.
Never listed: bin/, cdimage/, split/, toolchain/, work/ (retail bytes,
third-party toolchains and build outputs are supplied locally), reference/
(reference material, not a build input), and files that change on every build
(LAST_BUILD.json, LAST_SOURCE_BUILD.json, LAST_RELOC_BUILD.json, PACKAGE.json
itself).

Run it after changing any packaged file (sources, tools, docs, the manifest),
i.e. before committing, so that `build.py --verify-package` passes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / 'PACKAGE.json'
ROOT_FILES = ('build.cmd', 'build.py', 'BUILD_MANIFEST.json', 'README.md', 'THIRD_PARTY.md')
DIRS = ('config/', 'docs/', 'include/', 'src/', 'tools/')
NEVER = ('bin/', 'cdimage/', 'split/', 'toolchain/', 'work/', 'reference/')
PURPOSE = ('File list of the TM2 decomp build (hybrid and source-only PS-X EXE), for build.py --verify-package. '
           'bin/, toolchain/ and split/ are supplied locally (README "Building") and are not listed.')


def git(*args):
    proc = subprocess.run(['git', *args], cwd=ROOT, capture_output=True, text=True, encoding='utf-8')
    if proc.returncode:
        raise SystemExit('make_package: git %s failed: %s' % (' '.join(args), proc.stderr.strip()))
    return proc.stdout


def candidates():
    names = set(git('ls-files', '-z').split('\0')) | set(git('ls-files', '-z', '--others', '--exclude-standard').split('\0'))
    names.discard('')
    picked = sorted(n for n in names
                    if (n in ROOT_FILES or n.startswith(DIRS)) and not n.startswith(NEVER)
                    and '__pycache__/' not in n and not n.endswith('.pyc'))
    untracked = set(git('ls-files', '-z', '--others', '--exclude-standard').split('\0'))
    return picked, sorted(n for n in picked if n in untracked)


def package():
    names, untracked = candidates()
    files = {}
    for name in names:
        path = ROOT / name
        if not path.is_file():
            continue  # tracked but deleted in the working tree: not part of the package
        data = path.read_bytes()
        files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    missing = [n for n in ROOT_FILES if n not in files]
    if missing:
        raise SystemExit('make_package: missing root files: ' + ', '.join(missing))
    return dict(base_commit=git('rev-parse', 'HEAD').strip(), purpose=PURPOSE, generator='tools/make_package.py',
                files=files), untracked


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--check', action='store_true', help='compare with PACKAGE.json instead of writing it')
    args = ap.parse_args()
    new, untracked = package()
    if args.check:
        old = json.loads(PACKAGE.read_text(encoding='utf-8'))['files']
        diff = sorted(set(old) ^ set(new['files']) | {n for n in old if n in new['files'] and old[n] != new['files'][n]})
        for n in diff:
            print('%s %s' % ('only in PACKAGE.json:' if n not in new['files'] else
                             'not in PACKAGE.json:' if n not in old else 'differs:', n))
        print('PACKAGE.json %s (%d files, %d differences)' % ('stale' if diff else 'up to date', len(new['files']), len(diff)))
        sys.exit(1 if diff else 0)
    PACKAGE.write_text(json.dumps(new, indent=2) + '\n', encoding='utf-8', newline='\n')
    for n in untracked:
        print('untracked, included:', n)
    print('PACKAGE.json written: %d files, %d bytes' % (len(new['files']), sum(r['bytes'] for r in new['files'].values())))


if __name__ == '__main__':
    main()
