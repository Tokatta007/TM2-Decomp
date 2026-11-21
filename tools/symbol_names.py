#!/usr/bin/env python3
"""Readable names given to retail symbols: config/US/names.tsv.

Columns: va, old, new, evidence. `old` is the retail (IDA) name, kept in
config/US/ida_names.tsv / ida_funcs.tsv as the reference; `new` is the name the
C sources use. Every tool that resolves names to addresses (symbol_map() in
build_maximal_hybrid.py and data_audit.py, the linkers' label tables) reads this
file too, so a renamed symbol resolves to its VA without changing ida_names.tsv.
Lines starting with '#' are comments. tools/rename_symbol.py writes the rows.
"""
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAMES_TSV = ROOT / 'config/US/names.tsv'
COLUMNS = ('va', 'old', 'new', 'evidence')


def load_renames(path=NAMES_TSV):
    """[{va: int, old, new, evidence}] in file order ([] when the file does not exist)."""
    if not path.exists():
        return []
    rows = []
    with path.open(newline='', encoding='utf-8') as f:
        lines = [line for line in f if line.strip() and not line.startswith('#')]
    for r in csv.DictReader(lines, delimiter='\t'):
        rows.append(dict(va=int(r['va'], 16), old=r['old'], new=r['new'], evidence=r.get('evidence') or ''))
    return rows


def renamed_addresses(path=NAMES_TSV):
    """{new name: va} for every row of names.tsv."""
    return {r['new']: r['va'] for r in load_renames(path)}
