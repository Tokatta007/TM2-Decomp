"""Static checks of the Vanilla Plus balance gates (PC port requests 40 and 41 and round 3, docs/BALANCE.md; replaces
request 39's tools/check_ram.py).

    py -3.14 tools/check_balance.py work/pv/P4 [--expect-functions] [--negative-control]

On a port variant link built with `--port-define TM2_PORT_BALANCE` (build_source_only.py or build.py output
directory):

1. The port functions (PORT_FUNCS, the public portBalance* helpers; their static helpers bal* too) are in .port_text
   (functions.tsv kind port), reach no rand() (call graph of R_MIPS_26 relocations), are called only by their gate
   functions (CALLERS) and are never pointed to. Two exceptions, by design: portBalanceCrushTop (bit 3's +5 on top)
   applies its damage through retail's carApplyDamage, which reaches rand() (the spin roll for amounts >= 8, a kill's
   carSpinOut); the check takes that edge out, proves the rest reaches no rand(), and checks the amount passed is the
   constant 5 (no spin roll). portBalanceTornadoThrow (bit 12) throws through retail's carSpinOutById (its spin-out
   and car shake draw rand(), the calls retail's dead freeze_timer == 1 throw would draw); that edge is taken out
   likewise and the function must call carSpinOutById exactly once. portBalanceTaserRetry (bit 16) connects a late
   taser shot with the connect's lift, retail's carSpinOut (kind 6): that edge out, called once, and no
   carApplyDamage (round 3: the late connect's one hit is weaponArcsDraw's own last-tick hit). So with the word zero
   the rand() stream is retail's, and with bits set no rand() call site is added (only retail's own calls run or
   are skipped, BALANCE.md 5).
2. The gates (flow graph from the instructions, delay slots, jump tables from jumptables.tsv), one per rule:
   - G0 roundStart: the portBalanceLatch call dominates every srand / rand call.
   - G1 weaponLaunchProjectile (bit 0): portBalanceMissile called twice (homing, fire missile).
   - G2 weaponFireEnergyAttack (bits 1, 2, 6): portBalanceShieldRefused's refused edge reaches neither the shield
     pool scan nor anything after the cost (its other edge reaches the scan); portBalanceMineRefused dominates the
     weaponDropItem call, which its refused edge cannot reach and its other edge can.
   - G3 weaponFireSelected (bit 6): portBalanceBombRefused's refused edge cannot reach the weaponDropItem call (the
     placement), its other edge can.
   - G4 weaponDropWrite (bit 2): no direct weaponDropSlotFind call; portBalanceMineSlot dominates the mine object
     search (weaponMineObjectFind), portBalanceBombSlot the bomb object search (weaponBombObjectFind).
   - G5 weaponDropsUpdate (bit 2): portBalanceMineKeep called once; weaponDropCheckHit (the hit test) reachable
     from both of its edges.
   - G6 carCarCollision (bits 4, 5, 11): portBalanceNoMount dominates the carCrashStart call, which its refused edge
     cannot reach and its other edge can.
   - G7 carContactRespond (bits 3-7): portBalanceRam called once, its result carApplyDamage's amount ($a1) at the
     next call, on a path exclusive of the function's other carApplyDamage (the wall branch); in the source in the
     `else` of `if (c->hit.f01)` with the raw repeat guard after it and no store to c->hit.damage.
   - G8 carCrashStart (bit 3): the mount's damage is retail's again (carCrushDamageAmount feeds each carApplyDamage;
     request 39's portRamCrush is gone); portBalanceCrushStart dominates both carApplyDamage calls.
   - G9 carCrushContactUpdate (bit 3): portBalanceCrushTop is reached only through the on-top branch (every path to it
     passes that branch's rand() call); portBalanceCrushEnd called once.
   - G10 weaponTick (bits 3, 5, 6, 8, 21, 22): portBalancePeriod called once and dominates weaponFireSelected;
     request 45 (bit 8 split per car): its source tests Outlaw 2's type with TM2_BAL_OUTLAW2_RECHARGE (bit 8), Mr.
     Grimm's with TM2_BAL_GRIMM_RECHARGE (bit 21), Thumper's with TM2_BAL_THUMPER_RECHARGE (bit 22), each bit in
     its own case only; port_balance.h: TM2_BAL_ALL is the OR of every TM2_BAL_* bit (0..24 and, since request 91,
     26 and 27; one bit each, no two equal) but the options the port leaves off (OPT_IN: request 88's bit 25,
     TM2_BAL_LEAP_TOUCH), which are bits of their own.
   - G11 weaponHomingLeapUpdate (or its body weaponHomingLeapUpdateOne with TM2_PORT_CAR_SPECIALS; bits 7, 13, 25):
     portBalanceLeapDamage feeds the hit's carApplyDamage $a1; portBalanceLeapTouch called once; portBalanceLeapLand
     reachable only after that carApplyDamage.
   - G23 carStatsLoad (bit 24, request 77, BALANCE.md 17): portBalanceCarStats called once, before
     carStatsSetTopSpeed (the derived fields); run in check_custom's MIPS interpreter (LinkMips): carStatsLoad of a
     Hammerhead (0x20) with portBalance bit 24 set gives brakes 2, slide speed 60 and braking +0xCC = accel x 2, with
     the bit clear (and portBalanceLive set: the gate reads portBalance) retail's 1, 25; an Outlaw 2 (0x30) with the
     bit keeps her own 3 and 103.
   - G22 pickupRespawnTick (bit 23, request 67, BALANCE.md 16): it calls rand() as often as the base (2: the pick,
     health's half chance) and portBalancePickupSlot once, with the pick's rand() result in $a0, and every
     pickupSetVisible call comes after it; portBalancePickupSlot runs in check_custom's MIPS interpreter (FarMips) on a
     made-up level model and slot table (pickups shown, hidden, inactive / active, under a select's other child, a
     kind-4 pickup, health): with the bit, for r = 0..39 it returns the (r % n)-th of the n slots that are taken and
     can show, -1 when none is, after the select is switched the one behind it too; with the bit clear r % count
     (retail); it writes nothing but its stack. A link of the source before request 67 fails (no function).
   - G12 weaponProjectileUpdate (BALANCE.md 3.11; bit 9 is request 121's gun range since, G28): Sweet Tooth's
     spiral is retail's: no
     portBalance* call in weaponProjectileUpdate (the first cut's portBalanceSpiral and the follow-up's
     portBalanceSpiralGround are gone, and no portBalanceSpiral* function is linked), weaponProjectileSteer and
     hdpQueryPoint called once each; with the define alone weaponProjectileUpdate is not changed at all (check 4).
   - G13 areaDamage (bit 10): as many portBalanceSplash calls as carApplyDamage calls (the compiler merges each loop's
     surface / off-surface pair: 2), and every carApplyDamage in the source (4) takes SPLASH(...).
   - G14 weaponTornadoUpdate (or its body weaponTornadoUpdateOne with TM2_PORT_CAR_SPECIALS; bit 12):
     portBalanceTornadoThrow called once, reachable only through the end block (every path to it passes the
     block's fxSmokeB24 call); retail's two carSpinOutById calls (the dead freeze_timer == 1 tests) still there.
   Request 41 (BALANCE.md 12):
   - G15 the leap body (round 3 removed request 41's leap hop; bit 14 is request 121's Thumper lock since, G29): no
     portBalanceLeapArmed / portBalanceLeapHold
     call in the leap body, neither function linked, neither named in weapon_leap_homing_update.c.
   - G16 the Twister lock (bit 15): weaponFireSelected: portBalanceLocked guards every dispatch (weaponFireSpecial,
     weaponLaunchProjectile, weaponRicochetSlotFind, weaponLightningScan, weaponDropItem: dominated, none reachable
     from its refused edge, each from the other); weaponFireEnergyAttack: portBalanceEnergyLocked likewise for
     weaponLaunchProjectile, weaponDropItem, weaponShieldSlotFind, carSpinOut, weaponFireFreezeMissile (request 46
     item 3: the gate lets the shield through: its code tests the kind against the immediate 103, its source returns
     0 for TM2_BAL_LOCK_FREE_KIND (103) before portBalanceLocked); weaponTick: portBalanceGunsRefused called once and
     dominates UpdateGuns. Request 91 item 2 (bit 26, BALANCE.md 19, Mr. Slam's lock through the same two gates):
     portBalanceLocked run in check_custom's LinkMips on made-up human and AI records: 1 for a Mr. Slam (0xC0) whose
     slam record is active with bit 26 set, 0 with the record idle, without the record, with only bit 15 set, or
     for another type with the record active; portBalanceGunsRefused's source names neither bit 26 nor balSlamRuns
     (his machine guns stay free).
   - G17 the taser (bit 16): weaponArcsDraw calls portBalanceTaserRetry once (on the no-target branch); the arc's two
     retail carApplyDamage (count 39 and 0) are there and reachable after it.
   - G18 the freeze (bit 17): projectileHitTest calls portBalanceFreeze (the source: all four freeze_timer writes of
     type 14, none left at 300 outside the #else); humanCarTick and aiCarDrive call portBalanceFrozenTick once.
   - G19 the ricochet (bit 18): weaponRicochetUpdate calls portBalanceRicochetOwner in both car loops (the source:
     no owner test left without OWNER_HIT); weaponFireSelected clears the shot's left_owner at the launch. Request
     45: portBalanceRicochetPass called twice (after each hdpCollideObject), every mathReflectHeading (the bounce)
     reachable only through it; the source calls it right after each hit pass's kind filter, before `if (hit.hit)`.
   - G20 the free jump (bit 19): weaponFireEnergyAttack calls portBalanceJumpFree (the 0-energy return, the cost
     test, the subtraction).
   Round 3 (BALANCE.md 13):
   - G21 the machine guns (bit 20): bulletsMove calls portBalanceGunHit once; it dominates exactly one of the
     function's two rand() calls (the damage coin flip; the other, the smoke's, stays before it) and every
     weaponApplyHit; from the call a path reaches weaponApplyHit without that rand() (the bit's path draws none);
     the source routes the flip and both powers through GUN_FLIP / GUN_POWER.
   Request 121 (BALANCE.md 25):
   - G28 the gun range (bit 9): bulletSpawn calls portBalanceGunLife once; run in check_custom's LinkMips: with
     bit 9 a player's life 12 -> 15 and a computer car's 15 -> 19, unchanged with the bit clear (other bits set).
   - G29 the Thumper lock (bit 14): weaponFireSelected: portBalanceFlameLocked's refused edge cannot reach
     weaponFireSpecial, its other edge can; run in LinkMips on made-up human and AI records: 1 for a Thumper (0x70)
     with bit 14 and a flame emitter whose timer runs and whose car object is his; 0 with the timer out, another
     car's flame, the bit clear (every other bit set) or another type.
3. Source: the helpers' block in port_variant_arrays.c has no rand( call (comments stripped); the ten functions that
   size the drop records and mine objects (and the port's PORT_DROPS) use TM2_MAX_DROPS / TM2_MAX_MINE_OBJECTS /
   the live macros, no literal 25 / 23 / 24 left.
4. With --expect-functions: the game functions the port's defines changed (report.json port_changed_sections,
   code) include EXPECTED; with TM2_PORT_BALANCE as the only port define they are exactly EXPECTED.

--negative-control on a build without TM2_PORT_BALANCE runs checks 1-2, which must fail (the check is not blind).
Exit status 0 = pass.
"""
import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT  # noqa: E402
from reloc_image import Link  # noqa: E402
from check_hazards import Flow, load_functions, jump_tables, call_graph, reaches  # noqa: E402

DEFINE = 'TM2_PORT_BALANCE'
LEAP = 'weaponHomingLeapUpdate'
LEAP_ONE = 'weaponHomingLeapUpdateOne'      # its body with TM2_PORT_CAR_SPECIALS (.port_text)
TORNADO = 'weaponTornadoUpdate'
TORNADO_ONE = 'weaponTornadoUpdateOne'      # its body with TM2_PORT_CAR_SPECIALS (.port_text)
BODIES = {LEAP: LEAP_ONE, TORNADO: TORNADO_ONE}
CALLERS = {
    'portBalanceLatch': {'roundStart'},
    'portBalanceMissile': {'weaponLaunchProjectile'},
    'portBalanceShieldRefused': {'weaponFireEnergyAttack'},
    'portBalanceMineRefused': {'weaponFireEnergyAttack'},
    'portBalanceBombRefused': {'weaponFireSelected'},
    'portBalanceMineSlot': {'weaponDropWrite'},
    'portBalanceBombSlot': {'weaponDropWrite'},
    'portBalanceMineKeep': {'weaponDropsUpdate'},
    'portBalanceNoMount': {'carCarCollision'},
    'portBalanceRam': {'carContactRespond'},
    'portBalanceCrushStart': {'carCrashStart'},
    'portBalanceCrushTop': {'carCrushContactUpdate'},
    'portBalanceCrushEnd': {'carCrushContactUpdate'},
    'portBalancePeriod': {'weaponTick'},
    'portBalanceLeapDamage': {LEAP},
    'portBalanceLeapTouch': {LEAP},
    'portBalanceLeapLand': {LEAP},
    'portBalanceLeapPush': {LEAP},      # request 103 (bit 31)
    'portBalanceSplash': {'areaDamage'},
    'portBalanceTornadoThrow': {TORNADO},
    # request 41 (bit 14's portBalanceLeapArmed / portBalanceLeapHold removed in round 3: G15)
    'portBalanceLocked': {'weaponFireSelected', 'portBalanceEnergyLocked'},
    'portBalanceEnergyLocked': {'weaponFireEnergyAttack'},      # request 46 item 3: the shield let through
    'portBalanceGunsRefused': {'weaponTick'},
    'portBalanceTaserRetry': {'weaponArcsDraw'},
    'portBalanceFreeze': {'projectileHitTest'},
    'portBalanceFrozenTick': {'humanCarTick', 'aiCarDrive'},
    'portBalanceRicochetOwner': {'weaponRicochetUpdate'},
    'portBalanceRicochetPass': {'weaponRicochetUpdate'},       # request 45
    'portBalanceJumpFree': {'weaponFireEnergyAttack'},
    # round 3
    'portBalanceGunHit': {'bulletsMove'},
    # request 121 (bits 9, 14)
    'portBalanceGunLife': {'bulletSpawn'},
    'portBalanceFlameLocked': {'weaponFireSelected'},
    # request 67 (bit 23)
    'portBalancePickupSlot': {'pickupRespawnTick'},
    'portBalancePickupShowable': {'portBalancePickupSlot'},
    'portBalancePickupTaken': {'portBalancePickupSlot'},
    # request 77 (bit 24)
    'portBalanceCarStats': {'carStatsLoad'},
    # request 91 item 3 (bit 27)
    'portBalanceJamStop': {'aiUpdateStop'},
    'portBalanceJamAvoid': {'aiPickLinkNode'},
    'portBalanceJamRound': {'aiShouldChaseDirect'},
    # request 95 (bit 28)
    'portBalanceBoomerangCatch': {'weaponProjectileSteer', 'weaponProjectileUpdate'},
    # request 101 (bit 29)
    'portBalanceRouteNearest': {'carNearestRoutePoint'},
    'portBalanceRouteNext': {'aiFollowRouteTarget'},
    'portBalanceRouteBlocked': {'aiShouldChaseDirect'},     # and the port's targeting
    # request 102 item 3 (bit 30)
    'portBalanceBoomReady': {'fxExplosion6', 'fxExplosion12', 'fxExplosion24', 'fxExplosion48', 'fxExplosion96', 'fxExplosion192', 'fxSmokeBurstPink', 'fxSmokeBurstRed'},
    # retail functions the gates route through the helpers now
    'weaponDropSlotFind': {'portBalanceMineSlot', 'portBalanceBombSlot'},
    'carCrushDamageAmount': {'carCrashStart'},
    'carCrashStart': {'carCarCollision'},
}
PORT_FUNCS = {f for f in CALLERS if f.startswith('portBalance')}
STATIC_FUNCS = {'balType', 'balSlot', 'balTornadoRuns', 'balLeaping', 'balMines', 'balMineRecord', 'balCar',
                'balLeapTouchesTarget'}
# a port function's edges to retail functions that reach rand(), allowed by design (doc above)
RAND_VIA = {'portBalanceCrushTop': {'carApplyDamage'}, 'portBalanceTornadoThrow': {'carSpinOutById'},
            'portBalanceTaserRetry': {'carSpinOut'}}
OPT_IN = {'TM2_BAL_LEAP_TOUCH': 1 << 25}     # request 88: bits of their own, not in TM2_BAL_ALL (the port leaves them off)
RETIRED = ('portBalanceLeapArmed', 'portBalanceLeapHold')    # round 3's removed leap hop: must not be linked or called
EXPECTED = {
    'roundStart', 'weaponLaunchProjectile', 'weaponFireEnergyAttack', 'weaponFireSelected', 'weaponDropWrite',
    'weaponDropsUpdate', 'carCarCollision', 'carContactRespond', 'carCrashStart', 'carCrushContactUpdate',
    'weaponTick', LEAP, 'areaDamage', TORNADO,
    # request 41
    'weaponArcsDraw', 'projectileHitTest', 'humanCarTick', 'aiCarDrive', 'weaponRicochetUpdate',
    # round 3
    'bulletsMove',
    # request 67
    'pickupRespawnTick',
    # request 91 item 3
    'aiUpdateStop', 'aiPickLinkNode', 'aiShouldChaseDirect',
    # request 95
    'weaponProjectileSteer', 'weaponProjectileUpdate',
    # request 101
    'carNearestRoutePoint', 'aiFollowRouteTarget',
    # request 121
    'bulletSpawn',
    # request 102 item 3
    'fxExplosion6', 'fxExplosion12', 'fxExplosion24', 'fxExplosion48', 'fxExplosion96', 'fxExplosion192', 'fxSmokeBurstPink', 'fxSmokeBurstRed',
}
# the units / functions that size the drop records and mine objects (BALANCE.md 3.3)
SIZED = [
    ('weapon_tables.c', 'weaponDropSlotFind'), ('ai_route_helpers.c', 'weaponRemoteBombFind'),
    ('ai_route_loaders.c', 'weaponDropsReset'), ('fx_sprite_spawn.c', 'weaponDropsUpdate'),
    ('round_reset.c', 'weaponObjectsDetach'), ('asset_pools.c', 'weaponAssetBind'),
    ('arena_load_register.c', 'weaponMineObjectFind'), ('weapon_slot_writer.c', 'weaponDropWrite'),
    ('car_damage_human.c', 'weaponDropCheckHit'),
]


def writes(w):
    """The register an instruction writes (None for none / $zero)."""
    op = w >> 26
    if op == 0:
        funct = w & 63
        if funct in (8, 12, 13, 24, 25, 26, 27, 17, 19):    # jr, syscall, break, mult/div, mthi/mtlo
            return None
        r = (w >> 11) & 31
    elif op == 3:
        r = 31
    elif 8 <= op <= 15 or 32 <= op <= 38:
        r = (w >> 16) & 31
    elif op == 18:                                        # cop2 moves (mfc2/cfc2 write rt)
        r = (w >> 16) & 31 if ((w >> 21) & 31) in (0, 2) else None
    else:
        r = None
    return r or None


def is_copy(w, holds):
    """A register copy (addu/or rd, rs, $zero) from a register in `holds`: its destination, else None."""
    r = writes(w)
    if w >> 26 == 0 and (w & 63) in (33, 37) and r and (((w >> 21) & 31) in holds and (w >> 16) & 31 == 0
                                                       or (w >> 16) & 31 in holds and (w >> 21) & 31 == 0):
        return r
    return None


def feeds_a1(flow, producer):
    """The first call after `producer` (a jal) and whether $a1 holds the producer's $v0 at that call (straight-line
    code and unconditional jumps inside the function, register copies followed). Returns (address, callee, ok)."""
    holds = {2}
    a = producer + 8
    steps = 0
    while flow.start <= a < flow.end and steps < 64:
        steps += 1
        w = flow.insn[a]
        op = w >> 26
        uncond = None
        if op == 4 and (w >> 21) & 31 == 0 and (w >> 16) & 31 == 0:          # b
            t = w & 0xFFFF
            uncond = a + 4 + ((t - 0x10000 if t & 0x8000 else t) << 2)
        elif op == 2:                                                          # j inside the function
            uncond = ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
        if uncond is not None and flow.start <= uncond < flow.end:
            d = flow.insn[a + 4]
            c = is_copy(d, holds)
            if c:
                holds.add(c)
            elif writes(d):
                holds.discard(writes(d))
            a = uncond
            continue
        if op in (1, 4, 5, 6, 7, 20, 21, 22, 23) or op == 2 or (op == 0 and (w & 63) in (8, 9)):
            return None, 'branch at %08X' % a, False
        if a in flow.calls:
            d = flow.insn[a + 4]
            h = set(holds)
            c = is_copy(d, h)
            if c:
                h.add(c)
            elif writes(d):
                h.discard(writes(d))
            return a, flow.calls[a], 5 in h
        c = is_copy(w, holds)
        if c:
            holds.add(c)
        elif writes(w):
            holds.discard(writes(w))
        a += 4
    return None, 'no call', False


def a1_constant(flow, call, value):
    """True when $a1 is set to the constant `value` (addiu / ori $a1, $zero, value) in the call's delay slot or in
    the straight-line code before it with no other write to $a1 in between."""
    wants = {0x24050000 | (value & 0xFFFF), 0x34050000 | (value & 0xFFFF)}
    if flow.insn.get(call + 4) in wants:
        return True
    if writes(flow.insn.get(call + 4, 0)) == 5:
        return False
    a = call - 4
    while a >= flow.start and call - a <= 64:
        w = flow.insn[a]
        if w in wants:
            return True
        if writes(w) == 5 or a in flow.calls or (w >> 26) in (1, 2, 4, 5, 6, 7):
            return False
        a -= 4
    return False


def c_body(path, name):
    """The braces of C function `name` (first definition in `path`)."""
    text = path.read_text(encoding='utf-8')
    k = None
    for m in re.finditer(r'\b%s\s*\(' % re.escape(name), text):
        depth, e = 0, m.end() - 1
        while e < len(text):                       # the matching ')'
            depth += {'(': 1, ')': -1}.get(text[e], 0)
            if depth == 0:
                break
            e += 1
        brace = text.find('{', e)
        between = strip_comments(text[e + 1:brace]) if brace > 0 else ';'
        if between.strip()[:1] != ';' and not re.search(r'[()}=]', between):   # a definition (old-style too)
            k = brace
            break
    if k is None:
        raise KeyError(name)
    depth = 0
    for e in range(k, len(text)):
        if text[e] == '{':
            depth += 1
        elif text[e] == '}':
            depth -= 1
            if depth == 0:
                return text[k:e + 1]
    raise KeyError(name)


def strip_comments(s):
    return re.sub(r'//[^\n]*', ' ', re.sub(r'/\*.*?\*/', ' ', s, flags=re.S))


def check(out, defined=True):
    link = Link(out)
    rows, kinds = load_functions(out)
    tables = jump_tables(out)
    names_at = {r[0]: r[2] for r in rows}
    sizes = {r[2]: (r[0], r[1]) for r in rows}
    calls, callers, pointed = call_graph(link, rows)
    bodies = {f: (one if one in sizes else f) for f, one in BODIES.items()}
    leap = bodies[LEAP]
    callers_want = {f: {bodies.get(w, w) for w in want} for f, want in CALLERS.items()}
    if 'portSpecialBomb' in sizes:     # request 73 stage 2: the remote bomb as a mod car's special asks it too
        callers_want['portBalanceBombRefused'] = callers_want['portBalanceBombRefused'] | {'portSpecialBomb'}
    results = []

    def flow_of(fn):
        return Flow(link, sizes[fn][0], sizes[fn][1], tables, names_at) if fn in sizes else None

    def report(label, bad):
        print('%s -> %s' % (label, 'ok' if not bad else 'FAIL'))
        for b in bad[:8]:
            print('     ' + b)
        results.append(not bad)

    def gate_calls(flow, gate):
        return sorted(a for a, c in flow.calls.items() if c == gate)

    def sites(flow, names):
        return sorted(a for a, c in flow.calls.items() if c in names)

    def guard(flow, gate, protected, dominate=True, reach_on=True):
        """the gate's single call dominates (optionally) the protected calls; the refused (non-zero) edge reaches
        none of them, the other edge reaches each (optionally)"""
        bad = list(flow.problems)
        g = gate_calls(flow, gate)
        prot = sites(flow, protected)
        if len(g) != 1:
            return bad + ['%d calls of %s (want 1)' % (len(g), gate)]
        if not prot:
            return bad + ['no call of %s' % ' / '.join(sorted(protected))]
        if dominate:
            free = flow.reach(flow.start, set(g))
            bad += ['%s at %08X reachable without %s' % (flow.calls[a], a, gate) for a in prot if a in free]
        edges = flow.off_edge(g[0])
        if edges is None:
            return bad + ['%s at %08X: no branch on its result' % (gate, g[0])]
        off, on = flow.reach(edges[0], set(g)), flow.reach(edges[1], set(g))
        bad += ['%s at %08X reachable from the refused edge' % (flow.calls[a], a) for a in prot if a in off]
        if reach_on:
            bad += ['%s at %08X unreachable from the other edge' % (flow.calls[a], a) for a in prot if a not in on]
        return bad

    # 1. port functions
    missing = sorted(n for n in PORT_FUNCS | STATIC_FUNCS if kinds.get(n) != 'port')
    rand_port = []
    for n in sorted(PORT_FUNCS | STATIC_FUNCS):
        if n not in kinds:
            continue
        cut = {k: set(v) - RAND_VIA.get(k, set()) for k, v in calls.items()}
        if reaches(cut, n):
            rand_port.append(n)
    bad = []
    for fn, want in callers_want.items():
        got = callers.get(fn, set())
        if got != want:
            bad.append('%s called by %s (want %s)' % (fn, ' '.join(sorted(got)) or 'nothing', ' '.join(sorted(want))))
        if fn in pointed:
            bad.append('%s pointed to from %s' % (fn, ' '.join(sorted(pointed[fn]))))
    for n in STATIC_FUNCS:
        if n in pointed:
            bad.append('%s pointed to from %s' % (n, ' '.join(sorted(pointed[n]))))
    flow = flow_of('portBalanceTornadoThrow')
    if flow and len(sites(flow, {'carSpinOutById'})) != 1:
        bad.append('portBalanceTornadoThrow: carSpinOutById x%d (want 1)' % len(sites(flow, {'carSpinOutById'})))
    flow = flow_of('portBalanceTaserRetry')
    if flow:
        cad, spin = sites(flow, {'carApplyDamage'}), sites(flow, {'carSpinOut'})
        if len(cad) != 0 or len(spin) != 1:
            bad.append('portBalanceTaserRetry: carApplyDamage x%d, carSpinOut x%d (want 0 and 1: the lift only)' % (
                len(cad), len(spin)))
    flow = flow_of('portBalanceCrushTop')
    if flow:
        cad = sites(flow, {'carApplyDamage'})
        if len(cad) != 1 or not a1_constant(flow, cad[0], 5):
            bad.append('portBalanceCrushTop: carApplyDamage x%d, amount not the constant 5' % len(cad))
    print('1. port functions: %d of %d in .port_text (missing %s); reaching rand(): %s; callers %s' % (
        len(PORT_FUNCS | STATIC_FUNCS) - len(missing), len(PORT_FUNCS | STATIC_FUNCS), missing or 'none',
        rand_port or 'none', 'ok' if not bad else 'differ'))
    report('   port functions', (['missing ' + ' '.join(missing)] if missing else [])
           + (['reach rand: ' + ' '.join(rand_port)] if rand_port else []) + bad)

    # G0
    flow = flow_of('roundStart')
    g = gate_calls(flow, 'portBalanceLatch')
    prot = sites(flow, {'srand', 'rand'})
    free = flow.reach(flow.start, set(g))
    bad = ([] if g else ['no call of portBalanceLatch']) + ([] if prot else ['no srand'])
    bad += ['%s at %08X reachable without the latch' % (flow.calls[a], a) for a in prot if a in free]
    report('2. G0 latch: roundStart calls portBalanceLatch %dx before %d srand/rand' % (len(g), len(prot)), bad)

    # G1
    flow = flow_of('weaponLaunchProjectile')
    g = gate_calls(flow, 'portBalanceMissile')
    report('2. G1 missiles (bit 0): weaponLaunchProjectile calls portBalanceMissile %dx' % len(g),
           [] if len(g) == 2 else ['%d calls (want 2: homing, fire missile)' % len(g)])

    # G2
    flow = flow_of('weaponFireEnergyAttack')
    bad = guard(flow, 'portBalanceShieldRefused', {'weaponShieldSlotFind'}, dominate=False, reach_on=True)
    bad += guard(flow, 'portBalanceMineRefused', {'weaponDropItem'})
    report('2. G2 energy (bits 1, 2, 6): shield recast and mine refusals guard the pool scan / weaponDropItem', bad)

    # G3
    flow = flow_of('weaponFireSelected')
    bad = guard(flow, 'portBalanceBombRefused', {'weaponDropItem'}, dominate=False)
    report('2. G3 remote bomb (bit 6): portBalanceBombRefused guards the placement', bad)

    # G4
    flow = flow_of('weaponDropWrite')
    bad = ['weaponDropSlotFind called directly at %08X' % a for a in sites(flow, {'weaponDropSlotFind'})]
    for gate, prot in (('portBalanceMineSlot', {'weaponMineObjectFind'}), ('portBalanceBombSlot', {'weaponBombObjectFind'})):
        g = gate_calls(flow, gate)
        p = sites(flow, prot)
        if len(g) != 1 or not p:
            bad.append('%d calls of %s, %d of %s' % (len(g), gate, len(p), ' '.join(prot)))
            continue
        free = flow.reach(flow.start, set(g))
        bad += ['%s at %08X reachable without %s' % (flow.calls[a], a, gate) for a in p if a in free]
    report('2. G4 drop records (bit 2): weaponDropWrite takes its records from portBalanceMineSlot / BombSlot', bad)

    # G5
    flow = flow_of('weaponDropsUpdate')
    g = gate_calls(flow, 'portBalanceMineKeep')
    hit = sites(flow, {'weaponDropCheckHit'})
    bad = [] if len(g) == 1 and hit else ['%d calls of portBalanceMineKeep, %d of weaponDropCheckHit' % (len(g), len(hit))]
    if not bad:
        edges = flow.off_edge(g[0])
        if edges is None:
            bad.append('no branch on portBalanceMineKeep')
        else:
            for e in edges:
                r = flow.reach(e, set(g))
                bad += ['weaponDropCheckHit at %08X unreachable from an edge' % a for a in hit if a not in r]
    report('2. G5 permanent mines (bit 2): portBalanceMineKeep skips only the count-down', bad)

    # G6
    flow = flow_of('carCarCollision')
    report('2. G6 no mount (bits 4, 5, 11): portBalanceNoMount guards carCrashStart',
           guard(flow, 'portBalanceNoMount', {'carCrashStart'}))

    # G7
    flow = flow_of('carContactRespond')
    g = gate_calls(flow, 'portBalanceRam')
    applies = sites(flow, {'carApplyDamage'})
    bad = [] if len(g) == 1 else ['%d calls of portBalanceRam (want 1)' % len(g)]
    fed = None
    for x in g:
        at, name, good = feeds_a1(flow, x)
        if name != 'carApplyDamage' or not good:
            bad.append('portBalanceRam at %08X does not feed carApplyDamage $a1 (%s)' % (x, name))
        else:
            fed = at
    if fed is not None:
        for a in applies:
            if a != fed and (a in flow.reach(fed + 8, set()) or fed in flow.reach(a + 8, set())
                             or g[0] in flow.reach(a + 8, set()) or a in flow.reach(g[0] + 8, set())):
                bad.append('carApplyDamage at %08X shares a path with the scaled one at %08X' % (a, fed))
    body = c_body(ROOT / 'src/semantic/car_contacts.c', 'carContactRespond')
    m_if = body.find('    if (c->hit.f01) {')
    m_else = body.find('\n    } else {', m_if)
    m_deal = body.find('portBalanceRam(car, human, c->hit.other, c->hit.damage)')
    m_guard = body.find('c->f0A = 30 - c->hit.damage * 30 / 12;', m_deal)
    if not (0 <= m_if < m_else < m_deal < m_guard):
        bad.append('source: portBalanceRam not in the else of if (c->hit.f01) before the raw repeat guard')
    if re.search(r'c->hit\.damage\s*[-+*/]?=[^=]', body):
        bad.append('source: carContactRespond stores to c->hit.damage')
    report('2. G7 ram (bits 3-7): portBalanceRam -> carApplyDamage $a1 on the car-to-car branch only', bad)

    # G8
    flow = flow_of('carCrashStart')
    applies = sites(flow, {'carApplyDamage'})
    bad = ['%s called at %08X' % (flow.calls[a], a) for a in sites(flow, {'portRamCrush'})]
    fed = set()
    for x in sites(flow, {'carCrushDamageAmount'}):
        at, name, good = feeds_a1(flow, x)
        if name == 'carApplyDamage' and good:
            fed.add(at)
    if not applies or set(applies) - fed:
        bad.append('carApplyDamage not fed by carCrushDamageAmount at %s' % ' '.join(
            '%08X' % a for a in sorted(set(applies) - fed)))
    g = gate_calls(flow, 'portBalanceCrushStart')
    if len(g) != 1:
        bad.append('%d calls of portBalanceCrushStart (want 1)' % len(g))
    else:
        free = flow.reach(flow.start, set(g))
        bad += ['carApplyDamage at %08X reachable without portBalanceCrushStart' % a for a in applies if a in free]
    report('2. G8 crush (bit 3): the mount keeps retail\'s 15, portBalanceCrushStart before it', bad)

    # G9
    flow = flow_of('carCrushContactUpdate')
    top = gate_calls(flow, 'portBalanceCrushTop')
    end = gate_calls(flow, 'portBalanceCrushEnd')
    rnd = sites(flow, {'rand'})
    bad = [] if len(top) == 1 and len(end) == 1 and len(rnd) == 1 else [
        'calls: CrushTop %d, CrushEnd %d, rand %d (want 1 each)' % (len(top), len(end), len(rnd))]
    if not bad and top[0] in flow.reach(flow.start, set(rnd)):
        bad.append('portBalanceCrushTop reachable without the on-top branch')
    report('2. G9 ride (bit 3): portBalanceCrushTop only on top, portBalanceCrushEnd at the end', bad)

    # G10
    flow = flow_of('weaponTick')
    g = gate_calls(flow, 'portBalancePeriod')
    prot = sites(flow, {'weaponFireSelected'})
    bad = [] if len(g) == 1 and prot else ['%d calls of portBalancePeriod' % len(g)]
    if not bad:
        free = flow.reach(flow.start, set(g))
        bad += ['weaponFireSelected at %08X reachable without portBalancePeriod' % a for a in prot if a in free]
    # request 45: bit 8 split per car (source and header)
    pva_text = (ROOT / 'src/semantic/port_variant_arrays.c').read_text(encoding='utf-8')
    k = pva_text.find('int PORT_TEXT portBalancePeriod(')
    period = strip_comments(pva_text[k:pva_text.find('\n}\n', k)]) if k >= 0 else ''
    if not period:
        bad.append('source: portBalancePeriod not found')
    for typ, bit in (('OUTLAW2', 'OUTLAW2_RECHARGE'), ('GRIMM', 'GRIMM_RECHARGE'), ('THUMPER', 'THUMPER_RECHARGE')):
        m = re.search(r'case TM2_BAL_%s_TYPE:(.*?)break;' % typ, period, re.S)
        if not m or 'BAL(TM2_BAL_%s)' % bit not in m.group(1):
            bad.append('source: portBalancePeriod: the %s case does not test TM2_BAL_%s' % (typ, bit))
        if period.count('TM2_BAL_%s' % bit) != 2:     # the early-out mask and its own case
            bad.append('source: portBalancePeriod names TM2_BAL_%s %d times (want 2)' % (bit, period.count(
                'TM2_BAL_%s' % bit)))
    if re.search(r'TM2_BAL_RECHARGE\b', period):
        bad.append('source: portBalancePeriod still names TM2_BAL_RECHARGE (bit 8 shared)')
    hdr = strip_comments((ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8'))
    vals = {n: int(v, 16) for n, v in re.findall(r'#define (TM2_BAL_[A-Z0-9_]+)\s+(0x[0-9A-Fa-f]+)\b', hdr)}
    bits = {n: v for n, v in vals.items() if n != 'TM2_BAL_ALL' and v and v & (v - 1) == 0 and v < 1 << 32
            and not n.endswith('_TYPE')}
    union = 0
    for v in bits.values():
        union |= v
    if len(set(bits.values())) != len(bits):
        bad.append('port_balance.h: two TM2_BAL_* bits share a value')
    opt_in = {n: bits.pop(n, None) for n in OPT_IN}
    union = 0
    for v in bits.values():
        union |= v
    if vals.get('TM2_BAL_ALL') != union or union != ((1 << 25) - 1) | 63 << 26:
        bad.append('port_balance.h: TM2_BAL_ALL %s, the bits OR to %X (want 0xFDFFFFFF, bits 0..24, 26..31)' % (
            hex(vals.get('TM2_BAL_ALL', 0)), union))
    for n, want in OPT_IN.items():
        if opt_in[n] != want:
            bad.append('port_balance.h: %s %s (want %s, not in TM2_BAL_ALL)' % (n, hex(opt_in[n] or 0), hex(want)))
    if vals.get('TM2_BAL_GRIMM_RECHARGE') != 1 << 21 or vals.get('TM2_BAL_THUMPER_RECHARGE') != 1 << 22 or \
            vals.get('TM2_BAL_RECHARGE') != 1 << 8 or \
            not re.search(r'#define TM2_BAL_OUTLAW2_RECHARGE TM2_BAL_RECHARGE\b', hdr):
        bad.append('port_balance.h: bits 8 / 21 / 22 not Outlaw 2 / Mr. Grimm / Thumper')
    report('2. G10 recharge (bits 3, 5, 6, 8, 21, 22): weaponTick reads the period through portBalancePeriod; '
           'one bit per car', bad)

    # G11
    flow = flow_of(leap)
    bad = []
    if flow is None:
        bad.append('no %s' % leap)
    else:
        dmg, touch, land, push = (gate_calls(flow, n) for n in ('portBalanceLeapDamage', 'portBalanceLeapTouch',
                                                                 'portBalanceLeapLand', 'portBalanceLeapPush'))
        if len(dmg) != 1 or len(touch) != 1 or len(land) != 1 or len(push) != 1:
            bad.append('calls: LeapDamage %d, LeapTouch %d, LeapLand %d, LeapPush %d (want 1 each)' % (
                len(dmg), len(touch), len(land), len(push)))
        else:
            at, name, good = feeds_a1(flow, dmg[0])
            if name != 'carApplyDamage' or not good:
                bad.append('portBalanceLeapDamage does not feed carApplyDamage $a1 (%s)' % name)
            elif land[0] in flow.reach(flow.start, {at}):
                bad.append('portBalanceLeapLand reachable without the hit')
            elif push[0] in flow.reach(flow.start, {at}):
                bad.append('portBalanceLeapPush reachable without the hit')
    bad += leap_push_run(out, link, sizes)
    report('2. G11 leap (bits 7, 13, 31): %s: damage, touch, landing, push; the push 300 less the mass%% along '
           'her flight, flat (MIPS run)' % leap, bad)

    # G12
    flow = flow_of('weaponProjectileUpdate')
    steer = sites(flow, {'weaponProjectileSteer'})
    query = sites(flow, {'hdpQueryPoint'})
    bad = list(flow.problems)
    if len(steer) != 1 or len(query) != 1:
        bad.append('calls: weaponProjectileSteer %d, hdpQueryPoint %d (want 1 each)' % (len(steer), len(query)))
    bad += ['%s called at %08X' % (c, a) for a, c in sorted(flow.calls.items())
            if c and c.startswith('portBalance') and c != 'portBalanceBoomerangCatch']     # request 95: G25's
    bad += ['%s linked (the spiral is retail)' % f for f in sorted(sizes) if f.startswith('portBalanceSpiral')]
    report('2. G12 spiral (retail): weaponProjectileUpdate retail, no portBalance call but bit 28\'s catch',
           bad)

    # G13
    flow = flow_of('areaDamage')
    g = gate_calls(flow, 'portBalanceSplash')
    applies = sites(flow, {'carApplyDamage'})
    body = strip_comments(c_body(ROOT / 'src/semantic/area_damage.c', 'areaDamage'))
    raw = [m for m in re.findall(r'carApplyDamage\(([^;]*)\);', body) if 'SPLASH(' not in m]
    # the source has 4 calls; the compiler may merge the surface / off-surface pair of each loop into one call
    bad = [] if len(g) == len(applies) >= 1 else ['%d portBalanceSplash, %d carApplyDamage (want equal)' % (
        len(g), len(applies))]
    for x in g:
        at, name, good = feeds_a1(flow, x)
        if name == 'carApplyDamage' and not good:
            bad.append('portBalanceSplash at %08X does not feed carApplyDamage $a1' % x)
    bad += ['source: carApplyDamage(%s) without SPLASH' % m for m in raw]
    report('2. G13 Shadow (bit 10): every areaDamage carApplyDamage through portBalanceSplash', bad)

    # G14
    flow = flow_of(bodies[TORNADO])
    bad = []
    if flow is None:
        bad.append('no %s' % bodies[TORNADO])
    else:
        g = gate_calls(flow, 'portBalanceTornadoThrow')
        ends = sites(flow, {'fxSmokeB24'})
        throws = sites(flow, {'carSpinOutById'})
        if len(g) != 1 or len(ends) != 1:
            bad.append('calls: portBalanceTornadoThrow %d, fxSmokeB24 %d (want 1 each)' % (len(g), len(ends)))
        elif g[0] in flow.reach(flow.start, set(ends)):
            bad.append('portBalanceTornadoThrow reachable without the end block (fxSmokeB24)')
        if len(throws) != 2:
            bad.append('%d carSpinOutById calls (want retail 2: the freeze_timer == 1 tests stay)' % len(throws))
    report('2. G14 Twister throw (bit 12): %s: portBalanceTornadoThrow only in the end block, retail throws kept'
           % bodies[TORNADO], bad)

    # G15
    flow = flow_of(leap)
    bad = []
    if flow is None:
        bad.append('no %s' % leap)
    else:
        bad += ['%s called at %08X (the leap hop is gone)' % (flow.calls[a], a) for a in sites(flow, set(RETIRED))]
        if len(sites(flow, {'carApplyDamage'})) != 1:
            bad.append('%d carApplyDamage calls (want the one hit)' % len(sites(flow, {'carApplyDamage'})))
    bad += ['%s linked (the leap hop is gone)' % f for f in RETIRED if f in sizes]
    src = strip_comments((ROOT / 'src/semantic/weapon_leap_homing_update.c').read_text(encoding='utf-8'))
    bad += ['source: weapon_leap_homing_update.c names %s' % f for f in RETIRED if f in src]
    report('2. G15 leap hop (removed): %s: no portBalanceLeapArmed / LeapHold, neither linked' % leap, bad)

    # G16
    bad = []
    flow = flow_of('weaponFireSelected')
    bad += guard(flow, 'portBalanceLocked', {'weaponFireSpecial', 'weaponLaunchProjectile', 'weaponRicochetSlotFind',
                                             'weaponLightningScan', 'weaponDropItem'})
    flow = flow_of('weaponFireEnergyAttack')
    bad += guard(flow, 'portBalanceEnergyLocked', {'weaponLaunchProjectile', 'weaponDropItem', 'weaponShieldSlotFind',
                                                   'carSpinOut', 'weaponFireFreezeMissile'})
    # request 46 item 3: the energy gate lets the shield (kind 103) through: its code tests the kind ($a2) against
    # the immediate 103 and calls portBalanceLocked; the source returns 0 for TM2_BAL_LOCK_FREE_KIND first
    if 'portBalanceEnergyLocked' in sizes:
        a0, n0 = sizes['portBalanceEnergyLocked']
        imms = [link.word(a0 + 4 * k) & 0xFFFF for k in range(n0 // 4)
                if (link.word(a0 + 4 * k) >> 26) in (8, 9, 10, 11, 13, 14)]      # addi(u), slti(u), ori, xori
        if 103 not in imms:
            bad.append('portBalanceEnergyLocked: no test of the kind against 103 (the shield)')
        body = strip_comments(c_body(ROOT / 'src/semantic/port_variant_arrays.c', 'portBalanceEnergyLocked'))
        if not re.search(r'kind\s*!=\s*TM2_BAL_LOCK_FREE_KIND\s*&&\s*portBalanceLocked\s*\(', body):
            bad.append('source: portBalanceEnergyLocked does not let TM2_BAL_LOCK_FREE_KIND through first')
        if not re.search(r'#define\s+TM2_BAL_LOCK_FREE_KIND\s+103\b',
                         (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')):
            bad.append('port_balance.h: TM2_BAL_LOCK_FREE_KIND is not 103 (the shield)')
    flow = flow_of('weaponTick')
    g = gate_calls(flow, 'portBalanceGunsRefused')
    guns = sites(flow, {'UpdateGuns'})
    if len(g) != 1 or not guns:
        bad.append('weaponTick: %d calls of portBalanceGunsRefused, %d of UpdateGuns' % (len(g), len(guns)))
    else:
        bad += ['UpdateGuns at %08X reachable without portBalanceGunsRefused' % a for a in guns
                if a in flow.reach(flow.start, set(g))]
    bad += slam_lock_run(out, link, sizes)
    guns_src = strip_comments(c_body(ROOT / 'src/semantic/port_variant_arrays.c', 'portBalanceGunsRefused'))
    if 'TM2_BAL_SLAM_LOCK' in guns_src or 'balSlamRuns' in guns_src:
        bad.append('source: portBalanceGunsRefused names bit 26 (Mr. Slam keeps his machine guns)')
    report('2. G16 Twister lock (bit 15) and Mr. Slam lock (bit 26): portBalanceLocked guards every weapon dispatch, '
           'portBalanceEnergyLocked every energy dispatch (the shield, 103, let through: request 46), '
           'portBalanceGunsRefused the guns (bit 15 only); the Slam test run', bad)

    # G17
    flow = flow_of('weaponArcsDraw')
    g = gate_calls(flow, 'portBalanceTaserRetry')
    dmg = sites(flow, {'carApplyDamage'})
    bad = [] if len(g) == 1 and len(dmg) == 2 else ['%d calls of portBalanceTaserRetry, %d of carApplyDamage (want 1, 2)'
                                                   % (len(g), len(dmg))]
    if not bad:     # called on the no-target branch, in the loop before the arc's damage (both reachable after it)
        after = flow.reach(g[0] + 8, set())
        bad += ['carApplyDamage at %08X not reachable after portBalanceTaserRetry' % a for a in dmg if a not in after]
    report('2. G17 taser (bit 16): weaponArcsDraw calls portBalanceTaserRetry before the arc damage', bad)

    # G18
    bad = []
    flow = flow_of('projectileHitTest')
    if not gate_calls(flow, 'portBalanceFreeze'):
        bad.append('projectileHitTest does not call portBalanceFreeze')
    # (its #ifdef TM2_PORT_DT_PLAYER doubles an `if (...) {` line, so the text up to the next function, not c_body)
    text = (ROOT / 'src/semantic/weapon_slam_swing.c').read_text(encoding='utf-8')
    k = text.find(') projectileHitTest(')
    body = strip_comments(text[k:text.find('__attribute__((section(".text_', k + 1)])
    n_gate = len(re.findall(r'freeze_timer = portBalanceFreeze\(', body))
    n_raw = len(re.findall(r'freeze_timer = (?:300|FREEZE_TICKS);', body))   # request 73: FREEZE_TICKS is 300 in retail
    if n_gate != 4 or n_raw != 4:
        bad.append('source: %d freeze_timer writes through portBalanceFreeze, %d raw (#else) (want 4 and 4)' % (
            n_gate, n_raw))
    for fn in ('humanCarTick', 'aiCarDrive'):
        k = len(gate_calls(flow_of(fn), 'portBalanceFrozenTick'))
        if k != 1:
            bad.append('%s: %d calls of portBalanceFrozenTick (want 1)' % (fn, k))
    report('2. G18 freeze (bit 17): every ice freeze length through portBalanceFreeze, the frozen clock', bad)

    # G19
    bad = []
    flow = flow_of('weaponRicochetUpdate')
    g = gate_calls(flow, 'portBalanceRicochetOwner')
    if len(g) != 2:
        bad.append('%d calls of portBalanceRicochetOwner (want 2: the human and AI loops)' % len(g))
    body = strip_comments(c_body(ROOT / 'src/semantic/weapon_ricochet.c', 'weaponRicochetUpdate'))
    if re.search(r'SHOT\.owner \|\| SHOT\.life', body) or len(re.findall(r'OWNER_HIT\(', body)) != 2:
        bad.append('source: an owner test without OWNER_HIT')
    sel = strip_comments(c_body(ROOT / 'src/semantic/weapon_select.c', 'weaponFireSelected'))
    if 'left_owner = 0' not in sel:
        bad.append('source: weaponFireSelected does not clear left_owner at the launch')
    # request 45: no bounce off the owner's car box before the shot has left his box
    p = gate_calls(flow, 'portBalanceRicochetPass')
    hits = sites(flow, {'hdpCollideObject'})
    reflect = sites(flow, {'mathReflectHeading'})
    if len(p) != 2 or len(hits) != 2 or not reflect:
        bad.append('calls: portBalanceRicochetPass %d, hdpCollideObject %d, mathReflectHeading %d (want 2, 2, >= 1)'
                   % (len(p), len(hits), len(reflect)))
    else:
        free = flow.reach(flow.start, set(p))
        bad += ['mathReflectHeading at %08X reachable without portBalanceRicochetPass' % a for a in reflect
                if a in free]
        for h in hits:      # each hit pass is followed by a Pass call
            if not any(a in flow.reach(h + 8, set()) for a in p if a > h):
                bad.append('no portBalanceRicochetPass after hdpCollideObject at %08X' % h)
    passes = re.findall(r'hdpCollideObject\((.*?)if \(hit\.hit\) \{', body, re.S)
    if len(passes) != 2 or not all('portBalanceRicochetPass(&SHOT, (void *)hit.f18[0])' in s for s in passes):
        bad.append('source: not every hit pass is followed by portBalanceRicochetPass before `if (hit.hit) {`')
    report('2. G19 ricochet (bit 18): the owner tests through portBalanceRicochetOwner; no bounce off the owner '
           'before he is left (request 45)', bad)

    # G20
    flow = flow_of('weaponFireEnergyAttack')
    g = gate_calls(flow, 'portBalanceJumpFree')
    report('2. G20 free jump (bit 19): weaponFireEnergyAttack calls portBalanceJumpFree %dx' % len(g),
           [] if len(g) >= 2 else ['%d calls (want the 0-energy return, the cost test, the subtraction)' % len(g)])

    # G21
    bad = []
    flow = flow_of('bulletsMove')
    g = gate_calls(flow, 'portBalanceGunHit')
    applies = sites(flow, {'weaponApplyHit'})
    rnd = sites(flow, {'rand'})
    if len(g) != 1 or not applies or len(rnd) != 2:
        bad.append('calls: portBalanceGunHit %d, weaponApplyHit %d, rand %d (want 1, >= 1, retail 2)' % (
            len(g), len(applies), len(rnd)))
    else:
        free = flow.reach(flow.start, set(g))
        flip = [a for a in rnd if a not in free]
        if len(flip) != 1:
            bad.append('%d rand() calls after portBalanceGunHit (want 1: the coin flip)' % len(flip))
        bad += ['weaponApplyHit at %08X reachable without portBalanceGunHit' % a for a in applies if a in free]
        if flip and not any(a in flow.reach(g[0] + 8, set(flip)) for a in applies):
            bad.append('no path from portBalanceGunHit to weaponApplyHit without the coin flip')
    body = strip_comments(c_body(ROOT / 'src/semantic/weapon_bullets.c', 'bulletsMove'))
    if body.count('GUN_FLIP(') != 1 or body.count('GUN_POWER(') != 2 or 's->power, s->vel' in body:
        bad.append('source: the flip / powers not through GUN_FLIP / GUN_POWER')
    report('2. G21 guns (bit 20): bulletsMove: portBalanceGunHit before the coin flip and every weaponApplyHit', bad)

    # G22 (request 67)
    bad = []
    flow = flow_of('pickupRespawnTick')
    g = gate_calls(flow, 'portBalancePickupSlot') if flow else []
    rnd = sites(flow, {'rand'}) if flow else []
    vis = sites(flow, {'pickupSetVisible'}) if flow else []
    if len(g) != 1 or len(rnd) != 2 or not vis:
        bad.append('pickupRespawnTick: portBalancePickupSlot %d, rand %d, pickupSetVisible %d (want 1, 2, >= 1)' % (
            len(g), len(rnd), len(vis)))
    else:
        free = flow.reach(flow.start, set(g))
        bad += ['pickupSetVisible at %08X reachable without portBalancePickupSlot' % a for a in vis if a in free]
        first = min(rnd)
        if first > g[0] or any(first < a < g[0] for a in flow.calls):
            bad.append('the pick\'s rand() is not the call just before portBalancePickupSlot')
    bad += pickup_run(out, link, sizes)
    report('2. G22 pickups (bit 23): pickupRespawnTick\'s one pick through portBalancePickupSlot; the pick among the '
           'taken pickups that can show (MIPS run)', bad)

    # G24 (request 91 item 3)
    bad = []
    for fn, gate in (('aiUpdateStop', 'portBalanceJamStop'), ('aiPickLinkNode', 'portBalanceJamAvoid'),
                     ('aiShouldChaseDirect', 'portBalanceJamRound')):
        flow = flow_of(fn)
        n = len(gate_calls(flow, gate)) if flow else -1
        if n != 1:
            bad.append('%s: %s called %d times (want 1)' % (fn, gate, n))
    flow = flow_of('aiUpdateStop')
    if flow:
        g, rp = gate_calls(flow, 'portBalanceJamStop'), sites(flow, {'aiReplanRoute'})
        if len(g) != 1 or any(a in flow.reach(flow.start, set(g)) for a in rp):
            bad.append('aiUpdateStop: an aiReplanRoute not after portBalanceJamStop')
    bad += jam_run(out, link, sizes)
    report('2. G24 AI unjam (bit 27): aiUpdateStop counts the stops by cars, aiPickLinkNode leaves the jammed node out, '
           'aiShouldChaseDirect no direct chase while going round (MIPS run)', bad)

    # G25 (request 95)
    bad = []
    for fn, want in (('weaponProjectileSteer', 1), ('weaponProjectileUpdate', 1)):
        flow = flow_of(fn)
        n = len(gate_calls(flow, 'portBalanceBoomerangCatch')) if flow else -1
        if n != want:
            bad.append('%s: portBalanceBoomerangCatch called %d times (want %d)' % (fn, n, want))
    flow = flow_of('weaponProjectileSteer')
    if flow:
        g, fr = gate_calls(flow, 'portBalanceBoomerangCatch'), sites(flow, {'fxRecordFree'})
        if len(g) != 1 or len(fr) != 1 or fr[0] in flow.reach(flow.start, set(g)):
            bad.append('weaponProjectileSteer: its fxRecordFree reachable without portBalanceBoomerangCatch')
    bad += boomerang_run(out, link, sizes)
    report('2. G25 boomerang (bit 28): caught within the catch distance on its way back, in the steering, both '
           'sub-steps; nothing at a hit or its life\'s end; the charge once, capped (MIPS run)', bad)

    # G27 (request 102 item 3)
    bad = []
    for fn in ['fxExplosion6', 'fxExplosion12', 'fxExplosion24', 'fxExplosion48', 'fxExplosion96', 'fxExplosion192', 'fxSmokeBurstPink', 'fxSmokeBurstRed']:
        flow = flow_of(fn)
        n = len(gate_calls(flow, 'portBalanceBoomReady')) if flow else -1
        if n != 1:
            bad.append('%s: portBalanceBoomReady called %d times (want 1)' % (fn, n))
    bad += boom_run(out, link, sizes)
    report('2. G27 explosion sound (bit 30): retail\'s cooldown, and with the bit an explosion far from the last one '
           'that sounded; each explosion and smoke burst asks once; cleared at the round\'s start (MIPS run)', bad)

    # G26 (request 101)
    bad = []
    for fn, gate in (('carNearestRoutePoint', 'portBalanceRouteNearest'), ('aiFollowRouteTarget', 'portBalanceRouteNext'),
                     ('aiShouldChaseDirect', 'portBalanceRouteBlocked')):
        flow = flow_of(fn)
        n = len(gate_calls(flow, gate)) if flow else -1
        if n != 1:
            bad.append('%s: %s called %d times (want 1)' % (fn, gate, n))
    bad += route_run(out, link, sizes)
    report('2. G26 AI route (bit 29): the nearest node of any section, the graph\'s way past a bad table entry, no '
           'direct chase out of reach; Roof Tops\' nodes from the disc (MIPS run)', bad)

    # G23 (request 77)
    bad = []
    flow = flow_of('carStatsLoad')
    g = gate_calls(flow, 'portBalanceCarStats') if flow else []
    top = sites(flow, {'carStatsSetTopSpeed'}) if flow else []
    if len(g) != 1 or len(top) != 1 or g[0] > top[0]:
        bad.append('carStatsLoad: portBalanceCarStats %d, carStatsSetTopSpeed %d (want 1 before 1)' % (len(g), len(top)))
    bad += car_stats_run(out, link, sizes)
    report('2. G23 Hammerhead (bit 24): carStatsLoad sets his brakes 2 and slide speed 60 before the derived fields '
           '(MIPS run)', bad)

    # G28 (request 121 item 4)
    flow = flow_of('bulletSpawn')
    n = len(gate_calls(flow, 'portBalanceGunLife')) if flow else -1
    bad = [] if n == 1 else ['bulletSpawn: portBalanceGunLife called %d times (want 1)' % n]
    bad += gun_life_run(out, link, sizes)
    report('2. G28 gun range (bit 9): bulletSpawn\'s life through portBalanceGunLife, 12 -> 15 and 15 -> 19 '
           'with the bit (MIPS run)', bad)

    # G29 (request 121 item 5)
    flow = flow_of('weaponFireSelected')
    bad = guard(flow, 'portBalanceFlameLocked', {'weaponFireSpecial'}, dominate=False) if flow else ['no weaponFireSelected']
    bad += flame_lock_run(out, link, sizes)
    report('2. G29 Thumper lock (bit 14): weaponFireSelected refuses his special while a flame of his burns '
           '(MIPS run)', bad)

    # 3. source
    bad = []
    pva = (ROOT / 'src/semantic/port_variant_arrays.c').read_text(encoding='utf-8')
    i = pva.find('#ifdef TM2_PORT_BALANCE\n/* ---- Vanilla Plus balancing')
    j = pva.find('#endif /* TM2_PORT_BALANCE */', i)
    if i < 0 or j < 0:
        bad.append('helpers block not found')
    elif re.search(r'\brand\s*\(', strip_comments(pva[i:j])):
        bad.append('rand( in the helpers block')
    if not re.search(r'#define PORT_DROPS TM2_MAX_DROPS\b', pva):
        bad.append('port_variant_arrays.c: PORT_DROPS is not TM2_MAX_DROPS')
    sized = re.compile(r'<\s*(?:25|23)\b|\b24\s*\*|POOL_FILL\([^,]+,\s*23\)')
    decl = re.compile(r'\b(?:weaponDrops|weaponMineObjects)\s*\[\s*(?:25|23)\s*\]')
    for unit, fn in SIZED:
        path = ROOT / 'src/semantic' / unit
        b = strip_comments(c_body(path, fn))
        lit = sized.findall(b)
        if lit:
            bad.append('%s %s: retail size literal %s' % (unit, fn, ' '.join(lit)))
        for line in strip_comments(path.read_text(encoding='utf-8')).splitlines():
            if decl.search(line) and '.tm2_' not in line:      # retail storage itself (#else of the raised one)
                bad.append('%s: %s' % (unit, line.strip()))
    report('3. source: helpers without rand(); drop / mine sizes by macro in %d functions' % len(SIZED), bad)
    ok = all(results)
    if not defined:
        print('negative control: %s' % ('FAIL as expected' if not ok else 'passed: the check is blind'))
    return ok


def boomerang_run(out, link, sizes):
    """G25, request 95: portBalanceBoomerangCatch in check_custom's LinkMips on a made-up boomerang record (type +0x02,
    life +0x04, target +0x38, thrower object +0x3C with its position at +0xA8, owner +0x42) and the thrower's record
    in humanCars[0] / aiCars[0] (state.dead +0x9C / +0x6C, weapons +0x7C8 / +0x808: max_specials +0x0B, selected
    +0x0D, ammo[0] +0x0E); carFindById and fxRecordFree stubbed."""
    need = ('humanCars', 'aiCars', 'humanCarTotal', 'aiCarTotal', 'portBalanceLive')
    if 'portBalanceBoomerangCatch' not in sizes or any(n not in link.symbols for n in need):
        return ['portBalanceBoomerangCatch or its storage not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    img = Image(out)
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+(0x[0-9A-Fa-f]+|\d+)' % n, hdr).group(1), 0)
    bit, catch = num('TM2_BAL_BOOMERANG'), num('TM2_BAL_BOOMERANG_CATCH')
    sym = lambda n: link.symbols[n]['address']
    rec, obj, other, pt = 0x80A40000, 0x80A41000, 0x80A42000, 0x80A43000
    who = [1]
    freed = []

    def find(m):
        m.wr(m.r[5], 1, who[0])
        m.wr(m.r[6], 4, 0)
        return 1

    def free(m):
        freed.append(m.r[4] & 0xFFFF)
        m.wr(rec + 4, 2, 0xFFFF)
        return 0
    bad = []

    def run(human=1, live=None, life=50, d=catch * 8 - 1, back=True, dead=0, ammo=1, cap=5, sel=0):
        who[0] = human
        del freed[:]
        lm = LinkMips(img, {'carFindById': find, 'fxRecordFree': free})
        m = lm.m
        car = sym('humanCars') if human else sym('aiCars')
        st, wp = (0x9C, 0x7C8) if human else (0x6C, 0x808)
        for k in range(0, 0x900, 4):
            m.wr(car + k, 4, 0)
        for k in range(0, 0x100, 4):
            m.wr(rec + k, 4, 0)
            m.wr(obj + k, 4, 0)
            m.wr(other + k, 4, 0)
        m.wr(sym('humanCarTotal'), 4, 1)
        m.wr(sym('aiCarTotal'), 4, 1)
        m.wr(car + st, 1, dead)
        m.wr(car + wp + 0x0B, 1, cap)
        m.wr(car + wp + 0x0D, 1, sel)
        m.wr(car + wp + 0x0E, 2, ammo)
        m.wr(sym('portBalanceLive'), 4, bit if live is None else live)
        m.wr(rec, 1, 7)
        m.wr(rec + 2, 2, 51)
        m.wr(rec + 4, 2, life & 0xFFFF)
        m.wr(rec + 0x38, 4, obj + 0xA8 if back else other + 0xA8)
        m.wr(rec + 0x3C, 4, obj)
        m.wr(rec + 0x42, 2, 3 if human else 55)
        for k, v in enumerate((1000, -2000, 50)):
            m.wr(obj + 0xA8 + 4 * k, 4, v)
            m.wr(other + 0xA8 + 4 * k, 4, v)
            m.wr(pt + 4 * k, 4, (v + d if k == 1 else v) & 0xFFFFFFFF)
        got = lm.run('portBalanceBoomerangCatch', (rec, pt))
        return got, m.rd(car + wp + 0x0E, 2), m.rd(car + wp + 0x0D, 1), list(freed)
    cases = [
        ('caught (human)', dict(), (1, 2, 0, [7])),
        ('caught (computer car)', dict(human=0), (1, 2, 0, [7])),
        ('bit 28 clear', dict(live=0), (0, 1, 0, [])),
        ('at the catch distance', dict(d=catch * 8), (0, 1, 0, [])),
        ('not turned back', dict(back=False), (0, 1, 0, [])),
        ('thrower dead', dict(dead=1), (0, 1, 0, [])),
        ('timed out (life 0) at the car', dict(life=0), (0, 1, 0, [])),
        ('a dead record (life -1)', dict(life=-1), (0, 1, 0, [])),
        ('far away', dict(d=3000), (0, 1, 0, [])),
        ('the charge at its cap', dict(ammo=5), (1, 5, 0, [7])),
        ('nothing selected', dict(ammo=0, sel=8), (1, 1, 0, [7])),
    ]
    for name, kw, want in cases:
        got = run(**kw)
        if got != want:
            bad.append('%s: caught %d, charge %d, selected %d, freed %s (want %s)' % ((name,) + got + (want,)))
    return bad


def leap_push_run(out, link, sizes):
    """G11, request 103 (bit 31): portBalanceLeapPush in check_custom's LinkMips: homingLeapState's target id
    (+0x10) and her position pointer (+0x2C) made up, the target an AI record (aiCars[0]: motion.pos +0x6FC... read
    from the struct offsets below, stats.mass) or a human's; carFindById, VectorNormal (GTE) and carKnockBack stubbed: the push goes to the
    target's record, TM2_BAL_LEAP_PUSH less its mass%, the direction her to it, flat, length 4096; nothing with the
    bit clear or no car with the id."""
    need = ('portBalanceLive', 'homingLeapState', 'aiCars', 'humanCars', 'aiCarTotal', 'humanCarTotal')
    if 'portBalanceLeapPush' not in sizes or any(n not in link.symbols for n in need):
        return ['portBalanceLeapPush or its storage not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+(0x[0-9A-Fa-f]+|\d+)' % n, hdr).group(1), 0)
    bit, push = num('TM2_BAL_LEAP_PUSH_BIT'), num('TM2_BAL_LEAP_PUSH')
    a = lambda n: link.symbols[n]['address']
    leap, her = a('homingLeapState'), 0x80A40000
    POS = {1: 0x6DC + 0x20, 0: 0x71C + 0x20}  # motion.pos (cars.h: motion +0x6DC / +0x71C, pos +0x20)
    MASS = {1: 0x14C + 0x7A, 0: 0x4FC + 0x7A}  # stats (+0x14C / +0x4FC) .mass
    s32 = lambda v: v - (1 << 32) if v & 0x80000000 else v
    bad = []
    hdr_cars = (ROOT / 'include/semantic/cars.h').read_text(encoding='utf-8')

    def normal(m):
        v = [s32(m.rd(m.r[4] + 4 * k, 4)) for k in range(3)]
        n = max(1, int(round(sum(x * x for x in v) ** 0.5)))
        for k in range(3):
            m.wr(m.r[5] + 4 * k, 4, int(v[k] * 4096 / n) & 0xFFFFFFFF)
        return 0

    def run(live, human, mass, me, to, found=True):
        got = []
        lm = LinkMips(Image(out), {
            'carFindById': lambda m: (m.wr(m.r[5], 1, human), m.wr(m.r[6], 4, 0), 1 if found else 0)[2],
            'VectorNormal': normal,     # (the GTE's: out = in * 4096 / |in|)
            'carKnockBack': lambda m: got.append((m.r[4], m.r[5], [s32(m.rd(m.r[6] + 4 * k, 4)) for k in range(3)],
                                                  s32(m.r[7])))})
        m = lm.m
        m.wr(a('portBalanceLive'), 4, live)
        m.wr(a('humanCarTotal'), 4, 1)
        m.wr(a('aiCarTotal'), 4, 1)
        m.wr(leap + 0x10, 2, 51 if not human else 1)
        m.wr(leap + 0x2C, 4, her)
        car = a('humanCars') if human else a('aiCars')
        for k in range(3):
            m.wr(her + 4 * k, 4, me[k] & 0xFFFFFFFF)
            m.wr(car + POS[human] + 4 * k, 4, to[k] & 0xFFFFFFFF)
        m.wr(car + MASS[human], 2, mass)
        lm.run('portBalanceLeapPush', ())
        return got, car
    if not re.search(r'\bmass;\s*/\* \+0x7A', hdr_cars):
        bad.append('cars.h: stats.mass not at +0x7A')
    for human, mass in ((0, 5), (0, 35), (1, 10), (0, 60), (0, 100)):
        me, to = (1000, 2000, 300), (1300, 1600, 380)
        got, car = run(bit, human, mass, me, to)
        want = push - push * mass // 100
        if len(got) != 1:
            bad.append('human %d mass %d: carKnockBack called %d times' % (human, mass, len(got)))
            continue
        c, h, d, amt = got[0]
        if c != car or h != human or amt != want or d[2] != 0 or abs(d[0] * 4 + d[1] * 3) > 8 or \
                abs(d[0] - 2458) > 8 or abs(d[1] + 3277) > 8:
            bad.append('human %d mass %d: carKnockBack(%08X, %d, %s, %d) (want %08X, %d, ~[2458, -3277, 0], %d)' % (
                human, mass, c, h, d, amt, car, human, want))
    got, _ = run(bit, 0, 5, (500, 500, 0), (500, 500, 90))
    if len(got) != 1 or got[0][2][1] <= 0 or got[0][2][0] != 0:
        bad.append('the same place: %s (want straight ahead, +y)' % got)
    for name, kw in (('the bit clear', dict(live=0x7DFFFFFF, human=0, mass=5, me=(0, 0, 0), to=(100, 0, 0))),
                     ('no car with the id', dict(live=bit, human=0, mass=5, me=(0, 0, 0), to=(100, 0, 0), found=False))):
        got, _ = run(**kw)
        if got:
            bad.append('%s: pushed %s' % (name, got))
    return bad


def boom_run(out, link, sizes):
    """G27, request 102 item 3: portBalanceBoomReady in check_custom's LinkMips: the bit clear, retail's `ready`; with
    it the first sound, then near ones only when the cooldown has run, far ones (TM2_BAL_BOOM_APART or more) always;
    each played one is the new last; portBalanceLatch clears it."""
    need = ('portBalanceLive', 'portBalanceBoom', 'portBalance')
    if any(n not in sizes for n in ('portBalanceBoomReady', 'portBalanceLatch')) or \
            any(n not in link.symbols for n in need):
        return ['portBalanceBoomReady or its storage not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+(0x[0-9A-Fa-f]+|\d+)' % n, hdr).group(1), 0)
    bit, apart = num('TM2_BAL_BOOM_SOUND'), num('TM2_BAL_BOOM_APART')
    lm = LinkMips(Image(out), {})
    m = lm.m
    a = lambda n: link.symbols[n]['address']
    live, boom, pos = a('portBalanceLive'), a('portBalanceBoom'), 0x80A40000
    s32 = lambda v: v - (1 << 32) if v & 0x80000000 else v
    bad = []

    def ask(p, ready):
        for k in range(3):
            m.wr(pos + 4 * k, 4, p[k] & 0xFFFFFFFF)
        return lm.run('portBalanceBoomReady', (pos, ready))
    last = lambda: [s32(m.rd(boom + 4 * k, 4)) for k in range(4)]
    for k in range(4):
        m.wr(boom + 4 * k, 4, 0)
    m.wr(live, 4, 0)
    if ask((100, 200, 300), 0) != 0 or ask((100, 200, 300), 1) != 1 or any(last()):
        bad.append('the bit clear: not retail\'s cooldown (or the last sound written)')
    m.wr(live, 4, bit)
    p0 = (5000, -3000, 700)
    steps = [(p0, 0, 1, 'the first sound, the cooldown running'),
             ((5100, -3100, 750), 0, 0, 'a near one, the cooldown running'),
             ((p0[0] + apart - 1, p0[1], p0[2]), 0, 0, 'just inside the distance'),
             ((p0[0], p0[1] - apart, p0[2]), 0, 1, 'at the distance, the cooldown running'),
             ((p0[0], p0[1] - apart + 10, p0[2]), 1, 1, 'a near one, the cooldown run'),
             ((-8000, 9000, 0), 0, 1, 'far across the arena')]
    want_last = None
    for p, ready, want, name in steps:
        got = ask(p, ready)
        if got != want:
            bad.append('%s %s (ready %d): %d (want %d)' % (name, p, ready, got, want))
        if want:
            want_last = list(p) + [1]
        if last() != want_last:
            bad.append('%s: the last sound %s (want %s)' % (name, last(), want_last))
    m.wr(a('portBalance'), 4, bit)
    lm.run('portBalanceLatch', ())
    if any(last()):
        bad.append('portBalanceLatch: the last sound kept %s' % last())
    if ask((-8000, 9000, 0), 0) != 1:
        bad.append('after the latch: the first sound refused')
    return bad


def route_model(nodes, frm, pos, sec, near):
    """balRouteWay in Python: (the way's node, blocked)."""
    n = len(nodes)
    gap = lambda i: abs(pos[0] - nodes[i][0]) + abs(pos[1] - nodes[i][1]) + abs(pos[2] - nodes[i][2])

    def nearest(sec):
        for pas in ((0, 1) if sec >= 0 else (1,)):
            best, bi = None, 0
            for i in range(n):
                if pas == 0 and nodes[i][4] != sec:
                    continue
                if best is None or gap(i) < best:
                    best, bi = gap(i), i
            if best is not None:
                return bi
        return 0
    prev = {frm: frm}
    q = [frm]
    for v in q:
        for m in nodes[v][3]:
            if 0 <= m < n and m not in prev:
                prev[m] = v
                q.append(m)
    goal = nearest(sec)
    blocked = False
    if goal not in prev:
        goal = min(prev, key=lambda i: (gap(i), i))
        blocked = gap(goal) >= near
    while goal != frm and prev[goal] != frm:
        goal = prev[goal]
    return goal, blocked


def route_run(out, link, sizes):
    """G26, request 101: portBalanceRouteNearest / Next / Blocked in check_custom's LinkMips on Roof Tops' route nodes
    (TERRAIN/SROOF.PTS from the disc, 9 nodes, every one tagged section 6, the graph one-way west to east) against
    route_model: every node to targets at the port's stuck spots and on each roof, with the bit and without."""
    names = ('portBalanceRouteNearest', 'portBalanceRouteNext', 'portBalanceRouteBlocked')
    need = ('aiRouteNodes', 'aiRouteNodeCount', 'portBalanceLive')
    if any(n not in sizes for n in names) or any(n not in link.symbols for n in need):
        return ['the route functions or their storage not in the link']
    import struct
    from check_rollback import Image
    from check_custom import LinkMips
    try:
        from dmd_lod_dump import Disc, DEFAULT_BIN
        pts = Disc(DEFAULT_BIN).get('/TERRAIN/SROOF.PTS')
    except (OSError, KeyError, ValueError) as e:
        return ['SROOF.PTS from the disc: %s' % e]
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+(0x[0-9A-Fa-f]+|\d+)' % n, hdr).group(1), 0)
    bit, near = num('TM2_BAL_AI_ROUTE'), num('TM2_BAL_ROUTE_NEAR')
    count = 9
    nodes = []
    for i in range(count):
        x, y, z = struct.unpack_from('<iih', pts, 24 * i)
        nx = struct.unpack_from('<4h', pts, 24 * i + 10)
        cnt, rt = struct.unpack_from('<bb', pts, 24 * i + 22)
        nodes.append((x, y, z, list(nx[:cnt]), rt))
    lm = LinkMips(Image(out), {})
    m = lm.m
    a = lambda n: link.symbols[n]['address']
    m.fill(a('aiRouteNodes'), pts[:0xD20])
    m.wr(a('aiRouteNodeCount'), 4, count)
    live, pos = a('portBalanceLive'), 0x80A40000
    s32 = lambda v: v - (1 << 32) if v & 0x80000000 else v
    bad = []

    def put(p):
        for k in range(3):
            m.wr(pos + 4 * k, 4, p[k] & 0xFFFFFFFF)
        return pos
    # the port's stuck spots (request 101) and a point on each roof, at roof height
    spots = [(4300, 8500, 6960), (6300, 4100, 7200), (2800, 7000, 6960), (1085, 9502, 7440), (12000, 2000, 7760),
             (3000, 2000, 7760), (9000, 6000, 8000), (7760, 10040, 7200)]
    m.wr(live, 4, 0)
    if s32(lm.run('portBalanceRouteNearest', (put(spots[0]), 5))) != 5 or \
            s32(lm.run('portBalanceRouteNext', (7, 5, 17, 6, 5, put(spots[1])))) != -2 or \
            lm.run('portBalanceRouteBlocked', (0, put(spots[3]), 6)):
        bad.append('the bit clear: not retail')
    m.wr(live, 4, bit)
    blocked_seen = reached_seen = 0
    for p in spots:
        want = route_model(nodes, 0, p, -1, near)
        got = s32(lm.run('portBalanceRouteNearest', (put(p), 0)))
        wn = min(range(count), key=lambda i: (abs(p[0] - nodes[i][0]) + abs(p[1] - nodes[i][1]) +
                                              abs(p[2] - nodes[i][2]), i))
        if got != wn:
            bad.append('nearest to %s: %d (want %d)' % (p, got, wn))
        for frm in range(count):
            for sec in (6, 5):
                w, blk = route_model(nodes, frm, p, sec, near)
                got = lm.run('portBalanceRouteBlocked', (frm, put(p), sec))
                if got != int(blk):
                    bad.append('blocked from node %d to %s (section %d): %d (want %d)' % (frm, p, sec, got, blk))
                blocked_seen += blk
                reached_seen += not blk
                want = -1 if w == frm and not blk else w
                got = s32(lm.run('portBalanceRouteNext', (frm, 5, 17, 6, sec, put(p))))
                if got != want:
                    bad.append('next from node %d to %s (section %d), the table\'s 17: %d (want %d)' % (
                        frm, p, sec, got, want))
            # a good table entry with the target elsewhere: retail's
            if s32(lm.run('portBalanceRouteNext', (frm, 5, 3, 6, 5, put(p)))) != -2:
                bad.append('a good table entry (3) replaced from node %d' % frm)
            # the target's section on the car's node, the car in another: the graph's
            w, blk = route_model(nodes, frm, p, 6, near)
            want = -1 if w == frm and not blk else w
            if s32(lm.run('portBalanceRouteNext', (frm, 5, -1, 6, 6, put(p)))) != want:
                bad.append('the target\'s section on node %d, the car elsewhere: not the graph\'s way' % frm)
            if s32(lm.run('portBalanceRouteNext', (frm, 6, -1, 6, 6, put(p)))) != -2:
                bad.append('the car in its node\'s section (6): not retail\'s')
    # east roof to the west roofs: out of reach (the graph is one-way west to east)
    if not lm.run('portBalanceRouteBlocked', (4, put((1085, 9502, 7440)), 6)):
        bad.append('east roof (node 4) to the west roof: not blocked')
    if lm.run('portBalanceRouteBlocked', (7, put((9000, 6000, 8000)), 6)):
        bad.append('west roof (node 7) to the east roof: blocked')
    if not blocked_seen or not reached_seen:
        bad.append('the cases: %d blocked, %d reached (want both)' % (blocked_seen, reached_seen))
    return bad


def jam_run(out, link, sizes):
    """G24, request 91 item 3: portBalanceJamStop / Avoid / Round in check_custom's LinkMips on a made-up AI record
    (slot +0x06, index +0x04, contact.last +0x0D8 + 0x0C, drive.f474 +0x474, node_next +0x466), the frame counter
    (GsGetWorkBase_0) and aiGetTargetCar stubbed (a computer car of index 9 as the target)."""
    names = ('portBalanceJamStop', 'portBalanceJamAvoid', 'portBalanceJamRound')
    if any(n not in sizes for n in names) or 'portBalanceJam' not in link.symbols:
        return ['the jam functions or portBalanceJam not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    img = Image(out)
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+(0x[0-9A-Fa-f]+|\d+)' % n, hdr).group(1), 0)
    bit, stops, window, avoid = (num('TM2_BAL_AI_UNJAM'), num('TM2_BAL_JAM_STOPS'), num('TM2_BAL_JAM_WINDOW'),
                                 num('TM2_BAL_JAM_AVOID'))
    ai, tgt, slot = 0x80A40000, 0x80A42000, 3
    clock = [1000]

    def target(m):
        m.wr(m.r[5], 1, 0)
        return tgt
    lm = LinkMips(img, {'GsGetWorkBase_0': lambda m: clock[0], 'aiGetTargetCar': target})
    m = lm.m
    for k in range(0, 0x854, 4):
        m.wr(ai + k, 4, 0)
        m.wr(tgt + k, 4, 0)
    jam = link.symbols['portBalanceJam']['address'] + 12 * slot
    for k in range(0, 12, 4):
        m.wr(jam + k, 4, 0)
    m.wr(ai + 6, 2, slot)
    m.wr(ai + 4, 2, 2)
    m.wr(tgt + 4, 2, 9)
    m.wr(ai + 0x466, 2, 77)
    live = link.symbols['portBalanceLive']['address']
    bad = []

    def stop(last, f474=0, dt=10):
        clock[0] += dt
        m.wr(ai + 0xD8 + 0x0C, 2, last)
        m.wr(ai + 0x474, 2, f474)
        return lm.run('portBalanceJamStop', (ai,))
    m.wr(live, 4, 0)
    if any(stop(5) for _ in range(stops + 1)) or lm.run('portBalanceJamRound', (ai,)):
        bad.append('bit 27 clear: a jam')
    m.wr(live, 4, bit)
    got = [stop(5) for _ in range(stops)]
    if got != [0] * (stops - 1) + [1]:
        bad.append('stops by car 5: %s (want a jam at the %dth)' % (got, stops))
    if not lm.run('portBalanceJamRound', (ai,)) or not lm.run('portBalanceJamAvoid', (ai, 77)) or \
            lm.run('portBalanceJamAvoid', (ai, 78)):
        bad.append('after the jam: not going round node 77 only')
    clock[0] += avoid
    if lm.run('portBalanceJamRound', (ai,)) or lm.run('portBalanceJamAvoid', (ai, 77)):
        bad.append('the way round does not end after TM2_BAL_JAM_AVOID frames')
    if any(stop(9) for _ in range(stops + 1)):
        bad.append('ramming its target (index 9) counted')
    if any(stop(5, f474=1) for _ in range(stops + 1)):
        bad.append('stops with a wall bump counted')
    if any(stop(5, dt=window + 1) for _ in range(stops + 1)):
        bad.append('stops farther apart than the window counted as a jam')
    return bad


def gun_life_run(out, link, sizes):
    """G28, request 121 item 4: portBalanceGunLife in check_custom's LinkMips."""
    if 'portBalanceGunLife' not in sizes or 'portBalanceLive' not in link.symbols:
        return ['portBalanceGunLife or portBalanceLive not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    img = Image(out)
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    bit = int(re.search(r'#define TM2_BAL_GUN_RANGE\s+(0x[0-9A-Fa-f]+)', hdr).group(1), 16)
    bad = []
    for life, live, want in ((12, bit, 15), (15, bit, 19), (12, 0, 12), (15, 0, 15), (12, 0xFFFFFFFF & ~bit, 12),
                             (15, 0xFFFFFFFF & ~bit, 15), (15, 0xFFFFFFFF, 19)):
        lm = LinkMips(img, {})
        lm.m.wr(link.symbols['portBalanceLive']['address'], 4, live)
        got = lm.run('portBalanceGunLife', (life,))
        if got != want:
            bad.append('portBalanceGunLife(%d), bits %X = %d (want %d)' % (life, live, got, want))
    return bad


def flame_lock_run(out, link, sizes):
    """G29, request 121 item 5: portBalanceFlameLocked in check_custom's LinkMips on made-up records (a human's
    player +0x00, slot +0x04, type +0x90; a computer car's index +0x04, slot +0x06, type +0x30), the car objects
    and the special's type (portCarSpecialType) stubbed, two flame emitters (timer +0x00, car object +0x1C)."""
    need = ('portBalanceFlameLocked', 'portBalanceLive', 'weaponFlameEmitters')
    if 'portBalanceFlameLocked' not in sizes or any(n not in link.symbols for n in need[1:]):
        return ['portBalanceFlameLocked, portBalanceLive or weaponFlameEmitters not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    img = Image(out)
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    bit = int(re.search(r'#define TM2_BAL_THUMPER_LOCK\s+(0x[0-9A-Fa-f]+)', hdr).group(1), 16)
    car, objs, em = 0x80A40000, 0x80A50000, link.symbols['weaponFlameEmitters']['address']
    stubs = {'humanCarObject': lambda m: objs + 0x100 * (m.r[4] & 0xFF),
             'aiCarObject': lambda m: objs + 0x8000 + 0x100 * (m.r[4] & 0xFF),
             'portCarSpecialType': lambda m: m.r[5]}
    bad = []
    for human, typ_at, idx_at, slot_at, idx in ((1, 0x90, 0x00, 0x04, 1), (0, 0x30, 0x04, 0x06, 3)):
        mine = objs + (0 if human else 0x8000) + 0x100 * idx
        other = mine + 0x100
        for typ, live, timer, owner, want in ((0x70, bit, 10, mine, 1), (0x70, bit, 0, mine, 0),
                                              (0x70, bit, 10, other, 0), (0x70, 0xFFFFFFFF & ~bit, 10, mine, 0),
                                              (0x60, bit, 10, mine, 0), (0x70, 0xFFFFFFFF, 10, mine, 1)):
            lm = LinkMips(img, stubs)
            m = lm.m
            for k in range(0, 0x900, 4):
                m.wr(car + k, 4, 0)
            for k in range(0, 2 * 32, 4):
                m.wr(em + k, 4, 0)
            m.wr(car + typ_at, 4, typ)
            m.wr(car + idx_at, 2, idx)
            m.wr(car + slot_at, 2, 5)
            m.wr(em + 32, 2, timer)             # emitter 1: the flame being tested
            m.wr(em + 32 + 0x1C, 4, owner)
            m.wr(em + 0x1C, 4, other)           # emitter 0: another car's object, timer 0
            m.wr(link.symbols['portBalanceLive']['address'], 4, live)
            got = lm.run('portBalanceFlameLocked', (car, human))
            if got != want:
                bad.append('portBalanceFlameLocked(%s, type %02X, bits %X, timer %d, %s flame) = %d (want %d)' % (
                    'human' if human else 'AI', typ, live, timer, 'his' if owner == mine else 'another\'s', got,
                    want))
    return bad


def slam_lock_run(out, link, sizes):
    """G16, request 91 item 2: portBalanceLocked in check_custom's LinkMips on made-up records (a human's type at
    +0x90 and slam record pointer at +0x7C8 + 0x48, a computer car's at +0x30 and +0x808 + 0x48)."""
    if 'portBalanceLocked' not in sizes or 'portBalanceLive' not in link.symbols:
        return ['portBalanceLocked or portBalanceLive not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    img = Image(out)
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    bit = lambda n: int(re.search(r'#define %s\s+(0x[0-9A-Fa-f]+)' % n, hdr).group(1), 16)
    slam, twister = bit('TM2_BAL_SLAM_LOCK'), bit('TM2_BAL_TWISTER_LOCK')
    car, rec = 0x80A40000, 0x80A41000
    bad = []
    for human, type_at, slam_at in ((1, 0x90, 0x7C8 + 0x48), (0, 0x30, 0x808 + 0x48)):
        for typ, live, has, active, want in ((0xC0, slam, 1, 1, 1), (0xC0, slam, 1, 0, 0), (0xC0, slam, 0, 1, 0),
                                             (0xC0, twister, 1, 1, 0), (0x30, slam, 1, 1, 0),
                                             (0xC0, slam | twister, 1, 1, 1)):
            lm = LinkMips(img, {})
            m = lm.m
            for k in range(0, 0x900, 4):
                m.wr(car + k, 4, 0)
            for k in range(0, 0x14, 4):
                m.wr(rec + k, 4, 0)
            m.wr(car + type_at, 4, typ)
            m.wr(car + slam_at, 4, rec if has else 0)
            m.wr(rec, 1, active)
            m.wr(link.symbols['portBalanceLive']['address'], 4, live)
            got = lm.run('portBalanceLocked', (car, human))
            if got != want:
                bad.append('portBalanceLocked(%s, type %02X, bits %X, record %s, active %d) = %d (want %d)' % (
                    'human' if human else 'AI', typ, live, 'yes' if has else 'none', active, got, want))
    return bad


def car_stats_run(out, link, sizes):
    """G23: carStatsLoad in check_custom's LinkMips (every function of the link; the frame timers stubbed to 60):
    Hammerhead with portBalance bit 24 set and clear, Outlaw 2 with it set."""
    if 'carStatsLoad' not in sizes or 'portBalance' not in link.symbols or 'portBalanceLive' not in link.symbols:
        return ['carStatsLoad or portBalance not in the link']
    from check_rollback import Image
    from check_custom import LinkMips
    img = Image(out)
    stubs = {n: (lambda m: 60) for n in ('timerGetFrameTicks', 'timerGetFrameRate', 'sub_8015531C', 'sub_8015532C')}
    stats = 0x80A40000
    hdr = (ROOT / 'include/semantic/port_balance.h').read_text(encoding='utf-8')
    bit = int(re.search(r'#define TM2_BAL_HH_MOVE\s+(0x[0-9A-Fa-f]+)', hdr).group(1), 16)
    bad = []
    for typ, word, live, want in ((0x20, bit, 0, (2, 60)), (0x20, 0, bit, (1, 25)), (0x30, bit, 0, (3, 103))):
        lm = LinkMips(img, stubs)
        m = lm.m
        for k in range(0, 0xE8, 4):
            m.wr(stats + k, 4, 0)
        m.wr(stats + 0x76, 2, 100)      # the speed multiplier (carStatsSetSpeedMult), as a human's
        m.wr(stats + 0x78, 2, 100)
        m.wr(link.symbols['portBalance']['address'], 4, word)
        m.wr(link.symbols['portBalanceLive']['address'], 4, live)
        lm.run('carStatsLoad', (stats, typ))
        brake, slide = m.rd(stats + 0x5C, 2), m.rd(stats + 0x56, 2)
        accel, decel = m.rd(stats + 0xAC, 4), m.rd(stats + 0xCC, 4)
        if (brake, slide) != want or decel != accel * brake:
            bad.append('type %02X portBalance %X live %X: brakes %d slide %d, braking %d (accel %d) (want %d %d, '
                       'accel x brakes)' % (typ, word, live, brake, slide, decel, accel, want[0], want[1]))
    return bad


def pickup_run(out, link, sizes):
    """G22: portBalancePickupSlot on a made-up level model and slot table in check_custom's FarMips."""
    bad = []
    need = ('portBalancePickupSlot', 'csGetRoot')
    syms = ('csRootObject', 'pickupSlots', 'pickupObjects', 'pickupRegisteredCount', 'portBalanceLive')
    if any(n not in sizes for n in need) or any(n not in link.symbols for n in syms):
        return ['portBalancePickupSlot or its storage not in the link']
    from check_rollback import Image
    from check_custom import FarMips, mips_load_port
    fm = FarMips(Image(out))
    m = fm.m
    mips_load_port(m, link)       # the switch's jump table in .port_rodata
    sym = lambda n: link.symbols[n]['address']
    base = 0x80A40000
    obj, root, sel, whole = base + 0x3000, base + 0x100, base + 0x200, base + 0x300
    pk = [base + 0x400 + 0x40 * i for i in range(6)]

    def group(addr, kids, nid=0, idx=0):
        m.fill(addr, bytes(24 + 4 * len(kids)))
        m.wr(addr, 1, 1)
        m.wr(addr + 2, 2, nid)
        m.wr(addr + 20, 1, len(kids))
        m.wr(addr + 22, 2, idx)
        for i, k in enumerate(kids):
            m.wr(addr + 24 + 4 * i, 4, k)
    m.fill(obj, bytes(0x120))
    m.wr(obj + 284, 4, root)
    m.wr(sym('csRootObject'), 4, obj)
    group(whole, [])
    ids = [401, 402, 403, 404, 417, 405]
    shown = [0, 1, 0, 0, 0, 0]          # slot 1 on the map
    active = [0, 0, 0, 1, 0, 0]         # slot 3 still active (taken but its slot not cleared: not counted)
    for i in range(6):
        if i == 5:          # a kind-4 pickup node: id +2, visible count +18, index +16
            m.fill(pk[i], bytes(24))
            m.wr(pk[i], 1, 4)
            m.wr(pk[i] + 2, 2, ids[i])
            m.wr(pk[i] + 16, 2, i)
            m.wr(pk[i] + 18, 1, shown[i])
        else:
            group(pk[i], [whole] if shown[i] else [], ids[i], i)
            m.wr(pk[i] + 20, 1, shown[i])
    # the select: child 0 the whole object, child 1 pickup 2 (shown only after the select switches)
    m.fill(sel, bytes(24))
    m.wr(sel, 1, 9)
    m.wr(sel + 6, 1, 2)
    m.wr(sel + 7, 1, 0)
    m.wr(sel + 16, 4, whole)
    m.wr(sel + 20, 4, pk[2])
    group(root, [pk[0], pk[1], sel, pk[3], pk[4], pk[5]])
    for i in range(6):
        a = sym('pickupSlots') + 16 * i
        m.wr(a, 2, ids[i])
        m.wr(a + 2, 2, active[i])
        m.wr(sym('pickupObjects') + 4 * i, 4, pk[i])
    m.wr(sym('pickupRegisteredCount'), 4, 6)
    before = dict(m.mem)

    def call(r):
        m.writes.clear()
        v = fm.run('portBalancePickupSlot', (r,))
        stray = [a for a in m.writes if not 0x801F0000 <= a < 0x801FF000]
        if stray:
            bad.append('portBalancePickupSlot(%d) wrote %d bytes outside its stack (first %08X)' % (r, len(stray),
                                                                                                 min(stray)))
        return v - (1 << 32) if v & 0x80000000 else v
    for live, want in ((0x800000, [0, 4, 5]), (0, None)):
        m.wr(sym('portBalanceLive'), 4, live)
        for r in range(40):
            got = call(r)
            exp = want[r % len(want)] if want else r % 6
            if got != exp:
                bad.append('bit %s, r %d: slot %d (want %d)' % ('set' if live else 'clear', r, got, exp))
                break
    m.wr(sym('portBalanceLive'), 4, 0x800000)
    m.wr(sel + 7, 1, 1)                 # the target blown up: its pickup can show
    for r in range(12):
        if call(r) != [0, 2, 4, 5][r % 4]:
            bad.append('select switched, r %d: slot %d (want %d)' % (r, call(r), [0, 2, 4, 5][r % 4]))
            break
    for i in (0, 2, 4, 5):              # every taken one back on the map: nothing to bring back
        m.wr(pk[i] + (18 if i == 5 else 20), 1, 1)
    if call(7) != -1:
        bad.append('nothing taken: slot %d (want -1)' % call(7))
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', help='port variant link directory (build_source_only.py --output)')
    ap.add_argument('--expect-functions', action='store_true',
                    help='also compare the functions the port defines changed with EXPECTED')
    ap.add_argument('--negative-control', action='store_true',
                    help='run the checks on a build without %s (they must fail)' % DEFINE)
    a = ap.parse_args()
    out = Path(a.out) if Path(a.out).is_absolute() else ROOT / a.out
    report = json.loads((out / 'report.json').read_text(encoding='utf-8'))
    pv = report.get('port_variant') or {}
    names = {d.split('=')[0] for d in pv.get('port_defines') or []}
    defined = DEFINE in names
    if not defined and not a.negative_control:
        print('%s was built without %s (port defines %s): nothing to check; use --negative-control'
              % (a.out, DEFINE, sorted(names)))
        sys.exit(1)
    if not defined:
        print('note: no %s: the checks must fail (negative control)' % DEFINE)
        ok = not check(out, defined=False)
        print('CHECK BALANCE NEGATIVE CONTROL %s' % ('OK' if ok else 'FAILED'))
        sys.exit(0 if ok else 1)
    ok = check(out)
    if a.expect_functions:
        changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
        others = sorted(names - {DEFINE})
        missing, extra = EXPECTED - changed, changed - EXPECTED
        if others:          # with TM2_PORT_CAR_SPECIALS the bodies are weaponHomingLeapUpdateOne / ...TornadoUpdateOne
            missing -= set(BODIES)
        good = not missing and (not extra or bool(others))
        print('4. port_changed functions: %d (expected %d)%s; missing %s; extra %s -> %s' % (
            len(changed), len(EXPECTED), ' with other port defines' if others else '', sorted(missing),
            sorted(extra) if not others else '%d (other defines)' % len(extra), 'ok' if good else 'FAIL'))
        ok = ok and good
    print('CHECK BALANCE %s' % ('OK' if ok else 'FAILED'))
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
