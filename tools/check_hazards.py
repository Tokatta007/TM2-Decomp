"""Static checks of the arena hazard and destructible switches (PC port request 38, docs/ARENA_SWITCHES.md).

    py -3.14 tools/check_hazards.py work/pv/P4 [--expect-functions] [--negative-control]

On a port variant link built with `--port-define TM2_PORT_HAZARDS` (build_source_only.py or build.py output
directory):

1. The port functions (PORT_FUNCS: portArenaLatch, portArenaOff, portArenaTargetImmune, portArenaTrainsPark) are
   in .port_text (functions.tsv kind port) and reach no rand() (call graph of R_MIPS_26 relocations), so with
   every switch clear the rand() stream is retail's.
2. Every gate is present and guards its event. For each gate (GATES: G0 the latch in roundStart, G1..G11) the
   function calls the gate function, and every protected site of the event in that function
     - is dominated by the gate call (no path from the function's entry reaches it without passing the call), and
     - cannot be reached from the gate's "off" edge (the first branch after the call that tests $v0, the edge it
       takes when the gate returns non-zero) without passing the gate call again, and
     - can be reached from its "on" edge (with the item on, the retail path still leads to it).
   Protected sites: the event's own calls (worldCollapseUpdate, worldTrackVehiclesUpdate, fxFlameBurning,
   aiRoutePatchEiffelTower, the train branch's worldTrackVehiclePos / worldTrackVehicleHeading), every call of an
   entry gate's function after the gate (laEventStart, moscowEventStart, moscowDishDamage, eiffelTowerStartFall,
   targetDamage: all of their rand() calls and effects), every rand() call of the function where the event holds
   them (carContactResolve), and lava's `hp -= 3` (carGroundEffects: the one `addiu rX, rX, -3`, which feeds
   the HP store; the other -3s compare the ground class). G0: portArenaLatch dominates roundStart's srand (the latch is before the seed and every rand()).
   The flow graph is built from the instructions (delay slots, jump tables from jumptables.tsv).
3. The events have no other way in: the callers of their start functions (CALLERS) are exactly the gated
   functions (or a path through them), and none of them is pointed to (a function address in data or code).
4. With --expect-functions: the game functions the port's defines changed (report.json port_changed_sections,
   code) include EXPECTED (the 12 functions request 38 edits); with TM2_PORT_HAZARDS as the only port define
   they are exactly EXPECTED.

--negative-control on a build without TM2_PORT_HAZARDS runs checks 1-3, which must fail (the check is not
blind). Exit status 0 = pass.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT  # noqa: E402
from reloc_image import Link  # noqa: E402

PORT_FUNCS = {'portArenaLatch', 'portArenaOff', 'portArenaTargetImmune', 'portArenaTrainsPark'}
# the game functions request 38 changes (ARENA_SWITCHES.md 4)
EXPECTED = {
    'roundStart', 'worldUpdate', 'carGroundEffects', 'carContactApplyObject', 'targetDamage', 'carContactResolve',
    'laEventStart', 'moscowEventStart', 'moscowDishDamage', 'eiffelTowerStartFall', 'aiCarDamageUpdate',
    'laEventsUpdate',
}
# gate -> (function, gate function, protected sites). Sites: ('call', names) every call of those functions;
# ('after', allowed_before) every call of the function but the gate's, except the getters allowed before it;
# ('rand',) every rand() call of the function; ('addiu', imm) every `addiu rX, rX, imm`; ('dominates', names)
# only dominance (the latch: no off edge).
GATES = [
    ('G0 latch', 'roundStart', 'portArenaLatch', [('dominates', {'srand', 'rand'})]),
    ('G1 ice', 'worldUpdate', 'portArenaOff', [('call', {'worldCollapseUpdate'})]),
    ('G2 lava', 'carGroundEffects', 'portArenaOff', [('addiu', -3)]),
    ('G3 fire', 'carContactApplyObject', 'portArenaOff', [('call', {'fxFlameBurning'})]),
    ('G4 trains', 'worldUpdate', 'portArenaTrainsPark', [('call', {'worldTrackVehiclesUpdate'})]),
    ('G5 targets', 'targetDamage', 'portArenaTargetImmune', [('after', set())]),
    ('G6 train contact', 'carContactResolve', 'portArenaOff',
     [('call', {'worldTrackVehiclePos', 'worldTrackVehicleHeading'}), ('rand',)]),
    ('G7 LA quake', 'laEventStart', 'portArenaOff', [('after', {'laEventGet'})]),
    ('G8 Moscow bridges', 'moscowEventStart', 'portArenaOff', [('after', {'moscowEventGet'})]),
    ('G9 Moscow dish', 'moscowDishDamage', 'portArenaOff', [('after', set())]),
    ('G10 Eiffel Tower', 'eiffelTowerStartFall', 'portArenaOff', [('after', set())]),
    ('G11 Eiffel AI patch', 'aiCarDamageUpdate', 'portArenaOff', [('call', {'aiRoutePatchEiffelTower'})]),
]
# the events' start functions and effects -> their only callers (check 3)
CALLERS = {
    'worldCollapseUpdate': {'worldUpdate'},
    'worldTrackVehiclesUpdate': {'worldUpdate'},
    'laEventStart': {'laEventsTrigger'},
    'moscowEventStart': {'moscowEventsTrigger'},
    'moscowDishStartDestroy': {'moscowDishDamage'},
    'moscowDishDamage': {'weaponApplyHit'},
    'eiffelTowerStartFall': {'eiffelTowerBombCheck'},
    'aiRoutePatchEiffelTower': {'aiCarDamageUpdate', 'eiffelTowerUpdate'},   # eiffelTowerUpdate: only once fallen
    'eiffelTowerSetFallenSurfaces': {'eiffelTowerUpdate'},
    'trackVehicleDestroy': {'targetDamage'},
    'worldSectionDisable': {'laEventStart', 'moscowEventStart', 'targetDamage', 'worldCollapseClearDone'},
}


def load_functions(out):
    rows, kinds = [], {}
    for line in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('build_va'):
            continue
        f = line.split('\t')
        rows.append((int(f[0], 16), int(f[2]), f[4]))
        kinds[f[4]] = f[6]
    rows.sort()
    return rows, kinds


def owner(rows, addr):
    lo, hi = 0, len(rows)
    while lo < hi:
        mid = (lo + hi) // 2
        if rows[mid][0] <= addr:
            lo = mid + 1
        else:
            hi = mid
    if lo and rows[lo - 1][0] <= addr < rows[lo - 1][0] + rows[lo - 1][1]:
        return rows[lo - 1][2]
    return None


def jump_tables(out):
    sites = {}
    for line in (out / 'jumptables.tsv').read_text(encoding='utf-8').splitlines():
        if line.startswith('#') or line.startswith('table'):
            continue
        f = line.split('\t')
        targets = {int(t.split(':')[1], 16) for t in f[8].split()}
        sites[int(f[6], 16)] = targets
    return sites


def sext16(v):
    return v - 0x10000 if v & 0x8000 else v


class Flow:
    """Per-instruction flow graph of one function (delay slots: a branch's successor is its delay slot, whose
    successors are the branch's targets)."""

    def __init__(self, link, start, size, tables, names_at):
        self.start, self.end = start, start + size
        self.insn = {a: link.word(a) for a in range(start, self.end, 4)}
        self.succ = {}
        self.calls = {}         # address -> callee name (jal)
        self.problems = []
        delay = {}
        for a, w in self.insn.items():
            op, rs, rt, funct = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 63
            targets = None
            if op in (4, 5, 6, 7, 20, 21, 22, 23) or (op == 1 and rt in (0, 1, 2, 3)):
                t = a + 4 + (sext16(w & 0xFFFF) << 2)
                if op == 4 and rs == 0 and rt == 0:
                    targets = [t]                       # b
                else:
                    targets = [t, a + 8]
            elif op == 2:
                t = ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                targets = [t] if self.start <= t < self.end else []   # a j out of the function: tail call
            elif op == 3:
                t = ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                self.calls[a] = names_at.get(t, '%08X' % t)
            elif op == 0 and funct == 8:
                if rs == 31:
                    targets = []
                elif a in tables:
                    targets = sorted(tables[a])
                else:
                    self.problems.append('jr $%d at %08X without a jump table' % (rs, a))
                    targets = []
            if targets is not None:
                self.succ[a] = [a + 4]
                delay[a + 4] = [t for t in targets if self.start <= t < self.end]
        for a in self.insn:
            if a in delay:
                self.succ[a] = delay[a]
            elif a not in self.succ:
                self.succ[a] = [a + 4] if a + 4 < self.end else []

    def reach(self, first, cut):
        seen, todo = set(), [first]
        while todo:
            a = todo.pop()
            if a in seen or a in cut or not (self.start <= a < self.end):
                continue
            seen.add(a)
            todo.extend(self.succ.get(a, ()))
        return seen

    def off_edge(self, gate):
        """(off, on): where control goes when the gate call at `gate` returns non-zero / zero: the first conditional
        branch on $v0 after it (beq/bne $v0, $zero). None when not found."""
        for a in range(gate + 8, min(gate + 48, self.end), 4):
            w = self.insn[a]
            op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
            if a in self.calls:
                return None
            if op in (4, 5) and {rs, rt} == {2, 0} and rs != rt:
                t = a + 4 + (sext16(w & 0xFFFF) << 2)
                return (t, a + 8) if op == 5 else (a + 8, t)
            if (op in (4, 5, 6, 7) or op == 1) and 2 not in (rs, rt):
                continue
            if op in (6, 7, 1) and rs == 2:
                return None
        return None


def call_graph(link, rows):
    relocs, _ = link.relocations()
    starts = {r[0]: r[2] for r in rows}
    calls, callers, pointed = {}, {}, {}
    for p_c, rtype, v_c, item, sym in relocs:
        fn = owner(rows, p_c) or 'data@%08X' % p_c
        if rtype == 4:
            callee = owner(rows, v_c) or sym
            if callee != fn:
                calls.setdefault(fn, set()).add(callee)
                callers.setdefault(callee, set()).add(fn)
        elif v_c in starts:
            pointed.setdefault(starts[v_c], set()).add(fn)
    return calls, callers, pointed


def reaches(calls, first, target='rand'):
    seen, todo = set(), [first]
    while todo:
        fn = todo.pop()
        if fn in seen:
            continue
        seen.add(fn)
        todo.extend(calls.get(fn, ()))
    return target in seen


def check(out, defined=True):
    link = Link(out)
    rows, kinds = load_functions(out)
    tables = jump_tables(out)
    names_at = {r[0]: r[2] for r in rows}
    sizes = {r[2]: (r[0], r[1]) for r in rows}
    calls, callers, pointed = call_graph(link, rows)
    ok = True

    # 1. port functions
    missing = sorted(n for n in PORT_FUNCS if kinds.get(n) != 'port')
    rand_port = sorted(n for n in PORT_FUNCS if n in kinds and reaches(calls, n))
    good = not missing and not rand_port
    print('1. port functions: %d in .port_text (missing %s); reaching rand(): %s -> %s' % (
        len(PORT_FUNCS) - len(missing), missing or 'none', rand_port or 'none', 'ok' if good else 'FAIL'))
    ok = good

    # 2. gates
    for gate_name, fn, gate_fn, sites in GATES:
        if fn not in sizes:
            print('2. %s: %s not in the link -> FAIL' % (gate_name, fn))
            ok = False
            continue
        flow = Flow(link, sizes[fn][0], sizes[fn][1], tables, names_at)
        gates = sorted(a for a, c in flow.calls.items() if c == gate_fn)
        bad = list(flow.problems)
        protected = []
        for site in sites:
            kind = site[0]
            if kind in ('call', 'dominates'):
                protected += [(a, c) for a, c in flow.calls.items() if c in site[1]]
                if not any(c in site[1] for c in flow.calls.values()):
                    bad.append('no call of %s' % ' / '.join(sorted(site[1])))
            elif kind == 'rand':
                protected += [(a, c) for a, c in flow.calls.items() if c == 'rand']
            elif kind == 'after':
                first = gates[0] if gates else flow.start
                before = [(a, c) for a, c in flow.calls.items() if a < first and c != gate_fn]
                bad += ['%s called before the gate at %08X' % (c, a) for a, c in before if c not in site[1]]
                protected += [(a, c) for a, c in flow.calls.items() if a > first and c != gate_fn]
            elif kind == 'addiu':
                imm = site[1] & 0xFFFF
                hits = [(a, 'addiu %d' % site[1]) for a, w in flow.insn.items()
                        if w >> 26 == 9 and w & 0xFFFF == imm and (w >> 21) & 31 == (w >> 16) & 31]
                if not hits:
                    bad.append('no addiu %d' % site[1])
                protected += hits
        if not gates:
            bad.append('no call of %s' % gate_fn)
        else:
            free = flow.reach(flow.start, set(gates))
            for a, what in sorted(protected):
                if a in free:
                    bad.append('%s at %08X reachable without the gate' % (what, a))
            if sites[0][0] != 'dominates':
                for g in gates:
                    edges = flow.off_edge(g)
                    if edges is None:
                        bad.append('gate call at %08X: no branch on its result' % g)
                        continue
                    after_off = flow.reach(edges[0], set(gates))
                    for a, what in sorted(protected):
                        if a in after_off:
                            bad.append('%s at %08X reachable from the off edge of %08X' % (what, a, g))
                    # and the gate does not switch the event off for good: with the item on (retail) every
                    # protected site stays reachable
                    after_on = flow.reach(edges[1], set(gates))
                    for a, what in sorted(protected):
                        if a not in after_on:
                            bad.append('%s at %08X unreachable from the on edge of %08X' % (what, a, g))
        rand_sites = sorted(set(c for a, c in protected if c == 'rand' or (c in kinds and reaches(calls, c))))
        good = not bad
        print('2. %s: %s calls %s %dx; %d protected sites%s -> %s' % (
            gate_name, fn, gate_fn, len(gates), len(protected),
            ' (rand() via %s)' % ' '.join(rand_sites) if rand_sites else '', 'ok' if good else 'FAIL'))
        for b in bad[:8]:
            print('     ' + b)
        ok = ok and good

    # 3. no other way in
    bad = []
    for fn, want in CALLERS.items():
        got = callers.get(fn, set())
        if fn not in kinds:
            bad.append('%s not in the link' % fn)
        elif got != want:
            bad.append('%s called by %s (want %s)' % (fn, ' '.join(sorted(got)) or 'nothing', ' '.join(sorted(want))))
        if fn in pointed:
            bad.append('%s pointed to from %s' % (fn, ' '.join(sorted(pointed[fn]))))
    good = not bad
    print('3. event entry points: %d checked -> %s' % (len(CALLERS), 'ok' if good else 'FAIL'))
    for b in bad:
        print('     ' + b)
    ok = ok and good
    if not defined:
        print('negative control: %s' % ('FAIL as expected' if not ok else 'passed: the check is blind'))
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', help='port variant link directory (build_source_only.py --output)')
    ap.add_argument('--expect-functions', action='store_true',
                    help='also compare the functions the port defines changed with EXPECTED')
    ap.add_argument('--negative-control', action='store_true',
                    help='run checks 1-3 on a build without TM2_PORT_HAZARDS (they must fail)')
    a = ap.parse_args()
    out = Path(a.out) if Path(a.out).is_absolute() else ROOT / a.out
    report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
    pv = report.get('port_variant') or {}
    names = {d.split('=')[0] for d in pv.get('port_defines') or []}
    defined = 'TM2_PORT_HAZARDS' in names
    if not defined and not a.negative_control:
        print('%s was built without TM2_PORT_HAZARDS (port defines %s): nothing to check; use --negative-control'
              % (a.out, sorted(names)))
        sys.exit(1)
    if not defined:
        print('note: no TM2_PORT_HAZARDS: checks 1-3 must fail (negative control)')
        ok = not check(out, defined=False)
        print('CHECK HAZARDS NEGATIVE CONTROL %s' % ('OK' if ok else 'FAILED'))
        sys.exit(0 if ok else 1)
    ok = check(out)
    if a.expect_functions:
        changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
        others = sorted(names - {'TM2_PORT_HAZARDS'})
        missing, extra = EXPECTED - changed, changed - EXPECTED
        good = not missing and (not extra or bool(others))
        print('4. port_changed functions: %d (expected %d)%s; missing %s; extra %s -> %s' % (
            len(changed), len(EXPECTED), ' with other port defines' if others else '', sorted(missing),
            sorted(extra) if not others else '%d (other defines)' % len(extra), 'ok' if good else 'FAIL'))
        ok = ok and good
    print('CHECK HAZARDS %s' % ('OK' if ok else 'FAILED'))
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
