"""Static checks of the human records: object ids (docs/HUMANS16_PLAN.md 1.4, step H0), the local-player
map (step H1), the inputs (step H2), the sound sides and local feedback (step H3), the storage for 16 humans
(step H4), the rules (step H5), the compaction (step H6), vacant records and the hand-over (request 50).

    py -3.14 tools/check_humans.py [work/pv/<a port variant build> ...]

1. The id macros of include/semantic/players.h, read back from the preprocessor for retail (no
   define), the port variant (4 humans), TM2_MAX_HUMANS 7 (predefined), TM2_PORT_HUMANS 4, 8 and 16:
   TM2_HUMAN_ID(i) is i + 1 for every i < 7 and every count up to 7; the ids are distinct, never 0, 8
   (the world root) or 10 (the missile objects), all below 50 (AI ids); TM2_HUMAN_INDEX_OF_ID inverts
   TM2_HUMAN_ID. TM2_PORT_HUMANS outside 4..16, above TM2_PORT_CARS or without TM2_PORT_CARS, and more
   than 7 humans without TM2_PORT_HUMANS must stop the preprocessor (#error).
2. No game or port C computes a human's object id from its index (`player + 1`, `index + 1`, ...) or
   an index from an id (`owner - 1`, ...) outside the macros: a scan of src/semantic for those
   forms (comments stripped); each known site (SITES) uses the macro the number of times listed.
3. With build directories: portHumanOfId is a port function (functions.tsv kind port) of each.
4. The local map (H1), in the C as the compiler sees it: every built unit that calls a view, camera, HUD or radar
   function with a per-view index (VIEW_CALLEES: viewCarCameraUpdate, viewSetCameraPose / Position / Rotation /
   FollowObjPos, viewCreateWithCamera, viewProc, hudToggleRadar / WeaponList, hudShow*, hudRadarPlotCar,
   hudDisplayRadar, hudDrawTargetPanel, hudBlinkWeapon*, hudDraw{SingleView,SplitVertical,SplitHorizontal},
   portHumanRadarPlot), and every human-record call inside the HUD functions (humanCarGet, carGetDisplaySpeed,
   gameGetLives, carNearestToHuman), is preprocessed with TM2_PORT_HUMANS (TM2_PORT_VARIANT TM2_PORT_CARS=16
   TM2_PORT_HUMANS=4); each call's index argument is on the reviewed list REVIEWED with what it is (a local slot
   from the map, a slot parameter, a fixed slot of a local-only path, the human of a slot, or a human that
   portHumanRadarPlot maps), and where it comes from the map the function body holds that assignment
   (`slot = portHumanSlot(..)`, `human = portLocalHuman(..)`). A call not on the list, or an entry no longer
   found, fails: no view, camera, HUD or radar function is indexed by the human record index outside the map.
   The census with TM2_PORT_HUMANS=16 (H4) must be the same (checks 6 and 8 likewise).
   Negative control: without TM2_PORT_HUMANS the same scan finds the retail human-indexed calls (RETAIL_HUMAN).
5. With build directories built with TM2_PORT_HUMANS: the map's functions (portHumanSlot, portLocalHuman,
   portLocalHumanCount, portHumanRadarPlot, worldHumanCamera, worldHumanFollow) and step H2's (the input functions
   portHumanAction / Combo / AnyLatched, portHumanPadPort, portPadButtonInWord, portHumanSetLayout(s), the cheat
   gates portHumanCheatsOff / CheatCode, portAiAimTarget, worldRadarAiAll) are in .port_text, and no rand()
   is reachable (call graph of R_MIPS_26 relocations; calls through pointers are not seen) from the code that
   runs only for local humans or local slots, nor from H2's functions (LOCAL_ROOTS: those functions, the gated
   camera / follow / radar plot, carViewHotkeys, carSetViewMode, viewCarCameraUpdate, the HUD draw functions and
   the target panel). The game functions H1 and H2 change (H1_CHANGED) are among report.json's port_changed
   sections.
6. The pad reads (H2), in the C as the compiler sees it (as check 4): every call of a pad reader (PAD_CALLEES:
   padCheckAction, padComboHeld, padAnyLatched, padButtonHeld, padActionHeld, padAnyPressed, padAnyDown,
   padLatchPort, ctlpadAny*, padClearActionCache, padGetActiveBinding, padActionIsBound, padSetLayout,
   padLoadLayout) and of the human input functions (portHumanAction, portHumanCombo, portHumanAnyLatched) is on
   the reviewed list PAD_REVIEWED with its port argument: menus, match start, the attract demo and the pad layer
   read this machine's pads; the input functions' retail path reads the port portHumanPadPort gives; the human
   record index reaches a pad only through the input functions (humanCarReadInput, carReadMoveCodes,
   humanCarTick, carViewHotkeys). Negative control: without TM2_PORT_HUMANS the scan finds retail's five pad reads
   by human index (RETAIL_PAD_HUMAN).
7. The cheats are gated (H2, portHumanOptions bit 3): humanCarTick returns on portHumanCheatsOff() before its
   first combo read; carReadMoveCodes tests portHumanCheatCode(car->code) before its switch, and every code of
   that switch with a HUD message is one of the three cheat codes portHumanCheatCode lists.
8. The sound sides and the local feedback (H3), in the C as the compiler sees it, with TM2_PORT_HUMANS and again
   with the car-sound stack (SOUND_DEFINES): every call of the side loops (soundStartPlayerLoop /
   soundStopPlayerLoop, soundMgrPlaySided / StopSided), of hudMessagePost and soundSetDeniedPending, of the H3
   gates (portHumanMessage / Sound / Denied / LoopPlay / LoopStop) and every full-volume sound (soundPlay,
   soundPlayVariant(Ex) with volume and pan 0) is on FEEDBACK_REVIEWED with what it is: a side from portHumanSide,
   a gate called with the human record index, the gated call inside a gate, a full-volume call on the branch
   TM2_CAR_FEEDBACK takes only for a local human (the body holds portHumanFeedback), or reviewed as not a human's
   own feedback / the shell. No sound side, HUD message or denied buzz is indexed by the human record index outside
   the gates (the map decides). Negative control: without TM2_PORT_HUMANS the scan finds retail's (side = the human
   index, messages and cues of every human). Check 5 covers H3's rand() side: the gates, the listener
   (sndListenerTransform), the engine cap (portCarSoundFrame) and every sound call a remote human's positional
   branch makes are roots of its call graph, and the H3 game functions are among the changed ones.
9. The storage (H4), in the C as the compiler sees it, with TM2_PORT_HUMANS 4 and 16 (retail as the control):
   (a) every per-human array of the census (PER_HUMAN: HUMANS16_PLAN.md 1.1 and the H1/H2 storage) is a row of
   config/US/port_variant_arrays.tsv in a humans group, and every declaration or definition of each in the built
   units has the first dimension the group gives (TM2_MAX_HUMANS, TM2_MAX_HUMANS - 2; carResetWeapons' retail u8[2]
   only while 4 humans fit its storage) or none; (b) no write past the settings record with 16 humans:
   settingsCopy's four list loops stop at their list's length (the bytes written stay inside the record's 0x2C,
   which retail's loops would pass at 7 humans), and every other write of the record's per-human fields
   (human_car, lives, control_layout, directly or through settingsSetHumanCar / Lives / ControlLayout) is a
   constant index inside the field, the port table's macro (TM2_SETTINGS_HUMAN_CAR) or a reviewed menu site;
   (c) the static asserts of step H4 (port_variant_arrays.c tm2_h4_*) are compiled at 16 humans.
   With build directories: each humans-group array of report.json has TM2_MAX_HUMANS (- 2) entries in port
   storage (carResetWeapons only past 4 humans), and the port sections end below the port RAM end.
10. The rules (H5, portHumanOptions bit 1), in the C as the compiler sees it, with TM2_PORT_HUMANS 4 and 16 (the
   negative control without it): (a) every round end in the built units (each rtScheduleEvent call, each store to
   roundOverFlag / roundWonFlag) is on ROUND_REVIEWED with what it is: a fixed-human rule behind the bit-1 gate (the
   gate precedes the site in its function), the port's trigger (portRoundEnd), a round start's reset, the boss's
   stage 2, the attract demo, the pause menu's quit; (b) in each rule function (RULE_GATES: humanCarDamageUpdate,
   worldUpdate, sessionRun, carNearestToHuman) every fixed-human read of the rule (RULE_READS: humanCars[0],
   i == 0, humanCarGet(0), lives[0], the 2-player result screens) comes after the gate, so with bit 1 no round end
   and no result is decided on a fixed human index; the split HUDs print gameGetLives of their own human under the
   gate (no gameGetLives(0) left); (c) the H5 port functions (portRoundEnd, portRoundResults, portHumanRespawn,
   portHumanRestartRecord, portHumanNearestCar, the alive / down helpers) read no fixed human index; (d) the
   respawn path: every carRestart call (the in-place restart) is on RESTART_REVIEWED (the round start, the
   binding, humanCarTick's restart_pending, the dead sub_8011E114), restart_pending is written only with 0 or a
   saved value, sub_8011E114 has no caller, and portHumanRespawn restarts through portHumanRestartRecord, which
   makes none of carRestart's match-wide resets; without TM2_PORT_HUMANS the census finds retail's ungated death /
   won rules (negative control). With build directories: the H5 functions are port functions, the H5 game
   functions are among the changed ones, and portHumanRespawn does not reach carRestart (call graph).
11. The compaction (H6; HUMANS16_PLAN.md H6 (d)): portHumanCompact(from, to), preprocessed with the drop-in stack
   (COMPACT_DEFINES) and TM2_PORT_HUMANS 4 and 16, writes index `to` of every per-human array: every row of a humans
   group of config/US/port_variant_arrays.tsv (read from the tsv, so a new row is checked at once: the records, the
   six word arrays, carResetWeapons, sessionPortHumanCars, the map, the input words, tilts, layouts, lives,
   bindings) and the settings record's human_car[0..1] (TM2_SETTINGS_HUMAN_CAR's first part); the settings' lives /
   control_layout stay on purpose (COMPACT_KEPT). Negative control: without TM2_PORT_HUMANS the port's own human
   arrays are not moved. With build directories built with TM2_PORT_HUMANS and TM2_PORT_CAR_DROPIN:
   portHumanCompact is a port function and has a relocation into each of those arrays and into sessionSettings.
   (The id / address holders it rewrites are check_car_groups.py check 12's.)
12. Vacant human records and the hand-over (request 50, HUMANS16_PLAN.md 8), in the C as the compiler sees it with
   TM2_PORT_HUMANS=16 and TM2_PORT_CAR_SWAP: every function of a built game unit that reads humanCarTotal /
   humanCarCount() (a loop over the human records), and sessionBindCars, is on VACANT_REVIEWED: gated (the vacancy
   test portHumanVacant.mask is in its body: worldUpdate, humanCarsRestart, humanCarNearest, worldCollapseUpdate,
   sessionBindCars) or inert by the vacant state (what makes it pass the record by); a new loop fails until reviewed.
   The port side: the counting helpers test the vacancy, portHumanVacateRecord writes the vacant state (VACATE_FIELDS),
   and the hand-over, leave and return call no compaction, restart, rebind, start picker or rand(). With
   TM2_PORT_CAR_DROPIN too: the same gates, and portCarActiveCount / portCarBindCount / portHumanCompact know the
   vacancy. Negative control: TM2_PORT_HUMANS=16 without TM2_PORT_CAR_SWAP has none of the gates (the gate test
   fails on each) and builds no hand-over function; a copy of arena_setup.c without humanCarsRestart's gate (under
   work/check_humans_neg) fails the gate test. With build directories built with both defines: the hand-over
   functions are port functions, the gated functions are changed, and no forbidden callee is reached (call graph;
   the swap blocks' own special endings excepted). Request 50b: portHumanRejoin (a joiner in vacant index h, in place)
   clears the vacancy, starts on a free car only (slot kind 2, index -1), binds h and starts it through
   portHumanRestartRecord, and writes no humanCarTotal and calls no compaction (REJOIN_NEEDS / REJOIN_RENUMBER; with
   the drop-in portCarStartHumanAt hands a vacant index to it); its call graph reaches humanCarPlaceAtStart and none
   of REJOIN_FORBIDDEN; negative control: three broken copies of its body (vacancy kept, humanCarTotal written, a
   driven car taken) fail the test.
13. The turn's sign kept in reverse (request 66, ANALOG_STEERING.md 2.6), with build directories: portHumanSteerKeep is
   a port function called only by carThrottleSteer and carRicochetOffObject, calls nothing and is never pointed to;
   carThrottleSteer runs on check_custom's MIPS interpreter (every callee but mathBlendAngle and the gate a stub) over a
   sweep of speeds (forward, reverse, both sides of accel_launch) and both steering signs on human record 0: with
   portHumanControl[0] = 0 the yaw rate is retail's (proportional to the signed speed below turn_speed, the steering
   flipped by the speed's sign above it), with bit 0 it has the steering's sign at every speed and retail's size, and
   the turn in place is retail's in both; no rand(). Negative control: a link without TM2_PORT_HUMANS (the rollback or
   the define-free one) must show retail's flip with the bit set too (the run sees the difference).

Exit status 0 when every check passes."""
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))

# file -> (function, macro, count): every place that turns a human index into an object id or back.
SITES = [
    ('src/semantic/scene_init.c', 'carBindHumanSlot', 'TM2_HUMAN_ID', 1),
    ('src/semantic/update_guns.c', 'UpdateGuns', 'TM2_HUMAN_ID', 1),
    ('src/semantic/weapon_basic.c', 'weaponFireSpecial', 'TM2_HUMAN_ID', 1),
    ('src/semantic/weapon_helpers.c', 'weaponDropItem', 'TM2_HUMAN_ID', 1),
    ('src/semantic/weapon_special.c', 'weaponFireEnergyAttack', 'TM2_HUMAN_ID', 1),
    ('src/semantic/car_damage_human.c', 'weaponDropCheckHit', 'TM2_HUMAN_INDEX_OF_ID', 1),
    ('src/semantic/port_variant_arrays.c', 'portCarBindSlot', 'TM2_HUMAN_ID', 1),
    ('src/semantic/port_variant_arrays.c', 'portDropInId', 'TM2_HUMAN_ID', 1),
    ('src/semantic/port_variant_arrays.c', 'portHumanCompact', 'TM2_HUMAN_ID', 2),
    ('src/semantic/port_variant_arrays.c', 'portHumanOfId', 'TM2_HUMAN_ID', 1),
    ('src/semantic/port_variant_arrays.c', 'portHumanOfId', 'TM2_HUMAN_INDEX_OF_ID', 1),
    ('src/semantic/port_variant_arrays.c', 'portHumanHandOver', 'TM2_HUMAN_ID', 1),     # request 50
    ('src/semantic/port_variant_arrays.c', 'portHumanLeave', 'TM2_HUMAN_ID', 1),
    ('src/semantic/port_variant_arrays.c', 'portHumanReturn', 'TM2_HUMAN_ID', 1),
]

# forms that compute a human id from an index or an index from an id
FORBIDDEN = [
    (r'\bplayer\s*\)?\s*\+\s*1\b', 'player + 1 (a human id: TM2_HUMAN_ID)'),
    (r'->\s*id\s*=\s*[^;=]*\+\s*1\s*;', 'object id = x + 1 (TM2_HUMAN_ID)'),
    (r'\bkind\s*==\s*0\s*\?\s*\w+\s*\+\s*1\b', 'kind 0 ? index + 1 (TM2_HUMAN_ID)'),
    (r'\b(id|id2)\s*=\s*\w+\s*\+\s*1\s*;', 'id = x + 1 (TM2_HUMAN_ID)'),
    (r'\b\w*(owner|attacker|shooter|hit)\w*\s*-\s*1\b', 'owner/hit - 1 (TM2_HUMAN_INDEX_OF_ID)'),
    (r'\bhumanCar\w*\(\s*[^()]*\bid\w*\s*-\s*1\s*\)', 'humanCar*(id - 1) (TM2_HUMAN_INDEX_OF_ID)'),
]


def strip_comments(text):
    """Comments blanked, line structure kept."""
    def blank(m):
        return re.sub(r'[^\n]', ' ', m.group(0))
    return re.sub(r'/\*.*?\*/|//[^\n]*', blank, text, flags=re.S)


def preprocess(defines, body):
    from semantic_compile import preprocess as cpp
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        src = Path(tmp) / 'humans.c'
        src.write_text('#include "semantic/players.h"\n' + body, encoding='ascii', newline='\n')
        try:
            return cpp(src, Path(tmp) / 'humans.i', defines).decode('ascii', errors='replace'), None
        except SystemExit as e:
            return None, str(e)


def id_table(defines, n):
    """TM2_HUMAN_ID(i) for i < n and TM2_HUMAN_INDEX_OF_ID back, by #if chains (the preprocessor evaluates)."""
    lines = []
    for i in range(n):
        for v in range(0, 100):
            lines.append('#if TM2_HUMAN_ID(%d) == %d\nint id_%d_%d;\n#endif\n' % (i, v, i, v))
            lines.append('#if TM2_HUMAN_INDEX_OF_ID(%d) == %d\nint ix_%d_%d;\n#endif\n' % (v, i, v, i))
    lines.append('int max_humans = TM2_MAX_HUMANS;\n')
    text, err = preprocess(defines, ''.join(lines))
    if text is None:
        return None, None, None, err
    ids = {int(i): int(v) for i, v in re.findall(r'int id_(\d+)_(\d+);', text)}
    inv = {int(v): int(i) for v, i in re.findall(r'int ix_(\d+)_(\d+);', text)}
    mh = int(re.search(r'int max_humans = \(?(\w+)\)?;', text).group(1), 0)
    return ids, inv, mh, None


def check_macros():
    ok = True
    variant = ['TM2_PORT_VARIANT']
    cars = variant + ['TM2_PORT_CARS=16']
    configs = [
        ('retail', [], 2),
        ('variant', variant, 4),
        ('cars16 + humans4', cars + ['TM2_PORT_HUMANS=4'], 4),
        ('7 humans (predefined)', variant + ['TM2_MAX_HUMANS=7'], 7),
        ('cars16 + humans8', cars + ['TM2_PORT_HUMANS=8'], 8),
        ('cars16 + humans16', cars + ['TM2_PORT_HUMANS=16'], 16),
    ]
    for name, defines, n in configs:
        ids, inv, mh, err = id_table(defines, n)
        if ids is None:
            print('FAIL 1 %s: preprocessor stopped: %s' % (name, err.strip()[-300:]))
            ok = False
            continue
        good = mh == n and len(ids) == n and len(set(ids.values())) == n
        good = good and all(v not in (0, 8, 10) and v < 50 for v in ids.values())
        good = good and all(inv.get(v) == i for i, v in ids.items())
        if n <= 7:
            good = good and all(v == i + 1 for i, v in ids.items())
        else:
            good = good and all(ids[i] == i + 1 for i in range(7))
        ok = ok and good
        print('%s 1 %-40s humans %2d ids %s' % ('ok  ' if good else 'FAIL', name, mh,
                                               ' '.join(str(ids[i]) for i in sorted(ids))))
    # must not preprocess
    for name, defines in [
            ('TM2_PORT_HUMANS without TM2_PORT_CARS', ['TM2_PORT_VARIANT', 'TM2_PORT_HUMANS=4']),
            ('TM2_PORT_HUMANS=3', ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16', 'TM2_PORT_HUMANS=3']),
            ('TM2_PORT_HUMANS=17', ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16', 'TM2_PORT_HUMANS=17']),
            ('TM2_PORT_HUMANS=16 with TM2_PORT_CARS=11', ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=11', 'TM2_PORT_HUMANS=16']),
            ('8 humans without TM2_PORT_HUMANS', ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16', 'TM2_MAX_HUMANS=8'])]:
        text, err = preprocess(defines, 'int x;\n')
        good = text is None and '#error' in (err or '')
        ok = ok and good
        print('%s 1 %-40s %s' % ('ok  ' if good else 'FAIL', name, 'stops (#error)' if good else 'does NOT stop'))
    return ok


def function_body(text, name):
    """The body of function `name` (its definition: name(...) then '{', K&R parameter declarations allowed
    between), braces matched; '' if none."""
    m = re.search(r'\b%s\s*\([^;{}]*\)(\s*[^;{}()]+;)*\s*\{' % re.escape(name), text)
    if not m:
        return ''
    depth = 0
    for k in range(m.end() - 1, len(text)):
        if text[k] == '{':
            depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0:
                return text[m.end() - 1:k + 1]
    return ''


def check_sources():
    ok = True
    files = sorted(list((ROOT / 'src/semantic').glob('*.c')) + list((ROOT / 'src/semantic').glob('*.cpp')))
    bad = []
    for f in files:
        text = strip_comments(f.read_text(encoding='utf-8', errors='replace'))
        for pat, why in FORBIDDEN:
            for m in re.finditer(pat, text):
                line = text.count('\n', 0, m.start()) + 1
                bad.append('%s:%d: %s: %s' % (f.relative_to(ROOT).as_posix(), line, why, m.group(0).strip()))
    for b in bad:
        print('FAIL 2 ' + b)
    ok = not bad
    print('%s 2 no human id / index arithmetic outside the macros (%d files scanned)' % ('ok  ' if ok else 'FAIL', len(files)))
    cache = {}
    for rel, func, macro, count in SITES:
        if rel not in cache:
            t = strip_comments((ROOT / rel).read_text(encoding='utf-8', errors='replace'))
            cache[rel] = t
        text = cache[rel]
        body = function_body(text, func) if func else text
        n = len(re.findall(r'\b%s\s*\(' % macro, body))
        good = n == count and bool(body)
        ok = ok and good
        print('%s 2 %-36s %-22s %-22s %d (want %d)' % ('ok  ' if good else 'FAIL', rel.split('/')[-1], func or '(file)',
                                                      macro, n, count))
    return ok


# ---- 11. portHumanCompact moves every per-human array (H6) ----
COMPACT_DEFINES = ['TM2_PORT_VARIANT', 'TM2_PORT_CAR_GROUPS', 'TM2_PORT_CARS=16', 'TM2_PORT_CAR_SWAP',
                   'TM2_PORT_CAR_DROPIN']
# per-human state that is not a tsv row of a humans group but moves with the record too: the settings record's
# human_car[0..1] (TM2_SETTINGS_HUMAN_CAR's first part; sessionPortHumanCars is its rest)
COMPACT_EXTRA = {'human_car': r'\bcars\s*\[\s*to\s*\]\s*=(?!=)'}
# per-human state the compaction leaves on purpose: name -> why
COMPACT_KEPT = {
    'lives': 'settings lives[0..3]: retail\'s runner counter (lives[0] = the team\'s); with portHumanOptions bit 1 '
             '(any match past 2 humans) or bit 5 the lives are portHumanLives, which moves',
    'control_layout': 'settings control_layout[0..1]: the layout of pads 0/1 (padSetLayout), by local slot, not by '
                      'human; every human\'s own layout is portHumanLayout, which moves',
}


def _moved_to(body, name):
    """Writes `name[to ...]... = ` (any further subscripts) in the body; for humanCars the record copy's
    destination (`= humanCars[to]`)."""
    n = 0
    for m in re.finditer(r'\b%s\s*\[\s*to\b[^\]]*\]' % re.escape(name), body):
        k = m.end()
        while True:
            s = body[k:].lstrip()
            if not s.startswith('['):
                break
            k = len(body) - len(s)
            d = 0
            for e in range(k, len(body)):
                d += {'[': 1, ']': -1}.get(body[e], 0)
                if d == 0:
                    break
            k = e + 1
        rest = body[k:].lstrip()
        if rest.startswith('=') and not rest.startswith('=='):
            n += 1
        elif re.search(r'=\s*$', body[:m.start()]) and name == 'humanCars':
            n += 1
    return n


def check_compact():
    """Check 11 (source): with the drop-in stack and TM2_PORT_HUMANS 4 and 16, portHumanCompact's preprocessed body
    writes index `to` of every humans-group array of the tsv (and the settings' human_car)."""
    from semantic_compile import preprocess as cpp
    ok = True
    rows = per_human_rows()
    for n in (4, 16):
        defines = COMPACT_DEFINES + ['TM2_PORT_HUMANS=%d' % n]
        with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
            try:
                text = cpp(ROOT / 'src/semantic/port_variant_arrays.c', Path(tmp) / 'pva.i', defines).decode(
                    'ascii', errors='replace')
            except SystemExit as e:
                print('FAIL 11 port_variant_arrays.c with %d humans: %s' % (n, str(e)[-300:]))
                ok = False
                continue
        text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
        body = function_body(text, 'portHumanCompact')
        missing = []
        for name, row in sorted(rows.items()):
            if name == 'sessionPortHumanCars':
                hit = re.search(r'\bsessionPortHumanCars\s*\[\s*to\s*-\s*2\s*\]\s*=(?!=)', body)
            else:
                hit = _moved_to(body, name)
            if not hit:
                missing.append(name)
        for name, pat in COMPACT_EXTRA.items():
            if not re.search(pat, body):
                missing.append(name)
        good = bool(body) and not missing
        ok = ok and good
        print('%s 11 TM2_PORT_HUMANS=%-2d portHumanCompact writes index `to` of %d/%d per-human arrays (the humans groups '
              'of the tsv%s)%s' % ('ok  ' if good else 'FAIL', n, len(rows) + len(COMPACT_EXTRA) - len(missing),
                                   len(rows) + len(COMPACT_EXTRA), ' + ' + ', '.join(COMPACT_EXTRA),
                                   (': missing %s' % missing) if missing else ''))
    # negative control: without TM2_PORT_HUMANS the H1/H2/H6 moves are absent
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        text = cpp(ROOT / 'src/semantic/port_variant_arrays.c', Path(tmp) / 'pva.i', COMPACT_DEFINES).decode(
            'ascii', errors='replace')
    body = function_body('\n'.join(l for l in text.splitlines() if not l.startswith('#')), 'portHumanCompact')
    absent = [name for name in rows if name.startswith('portHuman') and _moved_to(body, name)]
    good = bool(body) and not absent and _moved_to(body, 'humanCars') > 0
    ok = ok and good
    print('%s 11 negative control (drop-in without TM2_PORT_HUMANS): the port\'s human arrays are not moved%s' % (
        'ok  ' if good else 'FAIL', (': found %s' % absent) if absent else ''))
    for name, why in sorted(COMPACT_KEPT.items()):
        print('     11 kept on purpose: %s' % why)
    return ok


def check_build_compact(out):
    """Check 11 (build, drop-in + TM2_PORT_HUMANS): a relocation of portHumanCompact into every humans-group array
    (moved: its port storage; carResetWeapons at 4 humans: its retail bytes) and into sessionSettings."""
    import json
    from check_hazards import load_functions, owner
    from reloc_image import Link
    out = Path(out)
    pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
    names = {d.split('=')[0] for d in pv.get('port_defines') or []}
    if not {'TM2_PORT_HUMANS', 'TM2_PORT_CAR_DROPIN'} <= names:
        print('     11 %s: not built with both TM2_PORT_HUMANS and TM2_PORT_CAR_DROPIN: skipped' % out.as_posix())
        return True
    rows, kinds = load_functions(out)
    link = Link(out)
    relocs, _ = link.relocations()
    targets = [v_c for p_c, rtype, v_c, item, sym in relocs if owner(rows, p_c) == 'portHumanCompact']
    arrays = {a['name']: a for a in pv.get('arrays', [])}
    spans = {}
    for name, row in per_human_rows().items():
        sym = link.symbols.get(name)
        if sym is None:
            spans[name] = None
            continue
        a = arrays.get(name)
        # humans-2 (sessionPortHumanCars[to - 2]): the compiler folds the -2 into the address
        lo = sym['address'] - (2 if row['group'] == 'humans-2' else 0)
        spans[name] = (lo, sym['address'] + (a['size'] if a else 4))   # not moved: carResetWeapons' 4 bytes
    sym = link.symbols.get('sessionSettings')
    spans['sessionSettings'] = (sym['address'], sym['address'] + 0x2C) if sym else None
    missing = sorted(n for n, s in spans.items() if s is None or not any(s[0] <= t < s[1] for t in targets))
    good = kinds.get('portHumanCompact') == 'port' and not missing
    print('%s 11 %s: portHumanCompact (port function) writes %d/%d per-human arrays + sessionSettings%s' % (
        'ok  ' if good else 'FAIL', out.as_posix(), len(spans) - len(missing), len(spans),
        (': missing %s' % missing) if missing else ''))
    return good


def check_build(out):
    out = Path(out)
    rows = [l.split('\t') for l in (out / 'functions.tsv').read_text(encoding='utf-8').splitlines()
            if l and not l.startswith('#')]
    hit = [r for r in rows if len(r) > 6 and r[4] == 'portHumanOfId']
    good = len(hit) == 1 and hit[0][6] == 'port'
    print('%s 3 %s: portHumanOfId %s' % ('ok  ' if good else 'FAIL', out.as_posix(),
                                        ('at ' + hit[0][0] + ' (port)') if good else 'missing or not a port function'))
    return good


# ---- 4. the local map (H1) in the preprocessed C ----

# callee -> position of its per-view (view / camera / HUD / radar) index argument
VIEW_CALLEES = {
    'viewCarCameraUpdate': 1, 'viewSetCameraPose': 0, 'viewSetPosition': 1, 'viewSetRotation': 1,
    'viewSetFollowObjPos': 1, 'viewCreateWithCamera': 2, 'viewProc': 0,
    'hudToggleRadar': 0, 'hudToggleWeaponList': 0, 'hudShowRadar': 0, 'hudShowWeaponList': 0,
    'hudRadarPlotCar': 3, 'hudDisplayRadar': 1, 'hudDrawTargetPanel': 1, 'hudBlinkWeaponMarker': 0,
    'hudBlinkWeaponDigits': 0, 'hudDrawSingleView': 0, 'hudDrawSplitVertical': 0, 'hudDrawSplitHorizontal': 0,
    'portHumanRadarPlot': 3,
}
# inside the HUD functions: callee -> position of its human-record index argument
HUD_FUNCS = ('hudDrawSingleView', 'hudDrawSplitVertical', 'hudDrawSplitHorizontal', 'hudDrawTargetPanel')
HUMAN_CALLEES = {'humanCarGet': 0, 'carGetDisplaySpeed': 0, 'gameGetLives': 0, 'carNearestToHuman': 0}
HUMANS_DEFINES = ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16', 'TM2_PORT_HUMANS=4']
HUMANS16_DEFINES = ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16', 'TM2_PORT_HUMANS=16']   # H4
NEGATIVE_DEFINES = ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16']

# (unit, function, callee, index argument) -> (what it is, the map assignment the body must hold or None).
# Kinds: 'slot map' a local slot from portHumanSlot; 'human map' the human of a slot from portLocalHuman;
# 'slot param' the function's own slot parameter (its callers are on this list); 'slot fixed' a fixed view / HUD
# slot of a path that runs for this machine's screen only (rtMainLoop's HUDs 0/1, the split views, menus);
# 'human mapped' a human index handed to portHumanRadarPlot, which maps it; 'rule' a reviewed retail rule.
_S = 'slot map'
REVIEWED = {
    ('arena_setup.c', 'arenaSetup', 'viewCreateWithCamera', '0'): ('slot fixed', None),
    ('arena_setup.c', 'arenaSetup', 'viewCreateWithCamera', '1'): ('slot fixed', None),
    ('camera.c', 'viewCarCameraUpdate', 'viewSetCameraPose', 'i'): ('slot param', None),
    ('camera.c', 'viewCarCameraUpdate', 'viewSetPosition', 'i'): ('slot param', None),
    ('camera.c', 'viewCarCameraUpdate', 'viewSetRotation', 'i'): ('slot param', None),
    ('car_destroyed.c', 'hudDrawTargetPanel', 'carNearestToHuman', 'human'): ('human map', 'human=portLocalHuman(i)'),
    ('car_records.c', 'carViewHotkeys', 'hudToggleRadar', 'slot'): (_S, 'slot=portHumanSlot(player)'),
    ('car_records.c', 'carViewHotkeys', 'hudToggleWeaponList', 'slot'): (_S, 'slot=portHumanSlot(player)'),
    ('car_records.c', 'carViewHotkeys', 'viewCreateWithCamera', '0'): ('slot fixed', None),
    ('car_records.c', 'carViewHotkeys', 'viewCreateWithCamera', '1'): ('slot fixed', None),
    ('hud_main.c', 'hudDrawSingleView', 'carGetDisplaySpeed', 'human'): ('human map', 'human=portLocalHuman(player)'),
    ('hud_main.c', 'hudDrawSingleView', 'gameGetLives', 'human'): ('human map', 'human=portLocalHuman(player)'),
    ('hud_main.c', 'hudDrawSingleView', 'humanCarGet', 'human'): ('human map', 'human=portLocalHuman(player)'),
    ('hud_main.c', 'hudDrawSingleView', 'hudBlinkWeaponDigits', 'player'): ('slot param', None),
    ('hud_main.c', 'hudDrawSingleView', 'hudBlinkWeaponMarker', 'player'): ('slot param', None),
    ('hud_main.c', 'hudDrawSingleView', 'hudDisplayRadar', 'player'): ('slot param', None),
    ('hud_main.c', 'hudDrawSingleView', 'hudDrawTargetPanel', 'player'): ('slot param', None),
    # co-op (mode 2) team lives: retail keeps them in lives[0] and prints them on HUD 0 (COOP_LIVES.md 4); with
    # portHumanOptions bit 1 (H5) every HUD prints its own human's
    ('hud_split.cpp', 'hudDrawSplitHorizontal', 'gameGetLives', '(portHumanRules() ? (human) : 0)'): ('human map', 'human=portLocalHuman(player)'),
    ('hud_split.cpp', 'hudDrawSplitHorizontal', 'hudBlinkWeaponDigits', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitHorizontal', 'hudBlinkWeaponMarker', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitHorizontal', 'hudDisplayRadar', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitHorizontal', 'hudDrawTargetPanel', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitHorizontal', 'humanCarGet', 'human'): ('human map', 'human=portLocalHuman(player)'),
    ('hud_split.cpp', 'hudDrawSplitVertical', 'gameGetLives', '(portHumanRules() ? (human) : 0)'): ('human map', 'human=portLocalHuman(player)'),
    ('hud_split.cpp', 'hudDrawSplitVertical', 'hudBlinkWeaponDigits', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitVertical', 'hudBlinkWeaponMarker', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitVertical', 'hudDisplayRadar', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitVertical', 'hudDrawTargetPanel', 'player'): ('slot param', None),
    ('hud_split.cpp', 'hudDrawSplitVertical', 'humanCarGet', 'human'): ('human map', 'human=portLocalHuman(player)'),
    ('menu_password_digits_draw.c', 'menuPasswordDigitsDraw', 'viewProc', '0'): ('slot fixed', None),
    ('menu_slider_draw.cpp', 'menuSliderDraw', 'viewProc', '0'): ('slot fixed', None),
    ('port_variant_arrays.c', 'portHumanRadarPlot', 'hudRadarPlotCar', 'L'): (_S, 'L=portHumanSlot(h)'),
    ('rt_main.c', 'rtMainLoop', 'hudDrawSingleView', '0'): ('slot fixed', None),
    ('rt_main.c', 'rtMainLoop', 'hudDrawSplitHorizontal', '0'): ('slot fixed', None),
    ('rt_main.c', 'rtMainLoop', 'hudDrawSplitHorizontal', '1'): ('slot fixed', None),
    ('rt_main.c', 'rtMainLoop', 'hudDrawSplitVertical', '0'): ('slot fixed', None),
    ('rt_main.c', 'rtMainLoop', 'hudDrawSplitVertical', '1'): ('slot fixed', None),
    ('rt_main.c', 'rtMainLoop', 'viewProc', '0'): ('slot fixed', None),
    ('rt_main.c', 'rtMainLoop', 'viewProc', '1'): ('slot fixed', None),
    ('scene_init.c', 'sceneObjDraw', 'viewProc', '0'): ('slot fixed', None),
    ('weapon_missile_trails.c', 'carSetViewMode', 'viewSetCameraPose', 'slot'): (_S, 'slot=portHumanSlot(human->player)'),
    ('weapon_missile_trails.c', 'carSetViewMode', 'viewSetPosition', 'slot'): (_S, 'slot=portHumanSlot(human->player)'),
    ('weapon_missile_trails.c', 'carSetViewMode', 'viewSetRotation', 'slot'): (_S, 'slot=portHumanSlot(human->player)'),
    ('world_update.c', 'worldHumanCamera', 'viewCarCameraUpdate', 'slot'): (_S, 'slot=portHumanSlot(i)'),
    ('world_update.c', 'worldHumanFollow', 'viewSetFollowObjPos', 'slot'): (_S, 'slot=portHumanSlot(i)'),
    ('world_update.c', 'worldRadarPlotHumans', 'portHumanRadarPlot', 'j'): ('human mapped', None),
    ('world_update.c', 'worldUpdate', 'portHumanRadarPlot', '0'): ('human mapped', None),
    ('world_update.c', 'worldUpdate', 'portHumanRadarPlot', '1'): ('human mapped', None),
    ('world_update.c', 'worldUpdate', 'portHumanRadarPlot', 'j'): ('human mapped', None),
    # H2: portHumanOptions bit 4, the AI blips on every local human's radar (drawing only, no rand())
    ('world_update.c', 'worldRadarAiAll', 'portHumanRadarPlot', 'j'): ('human mapped', None),
    # the ending movie's scene camera (the shell, view 0; found since c_bodies sees inside extern "C" blocks)
    ('screen_ending.cpp', 'sceneRun', 'viewSetPosition', '0'): ('slot fixed', None),
    ('screen_ending.cpp', 'sceneRun', 'viewSetRotation', '0'): ('slot fixed', None),
}
# without TM2_PORT_HUMANS (negative control): the retail calls indexed by the human record index
RETAIL_HUMAN = [
    ('camera.c', 'viewCarCameraUpdate', 'viewSetPosition', 'c->player'),
    ('car_destroyed.c', 'hudDrawTargetPanel', 'carNearestToHuman', 'i'),
    ('car_records.c', 'carViewHotkeys', 'hudToggleRadar', 'player'),
    ('hud_main.c', 'hudDrawSingleView', 'humanCarGet', 'player'),
    ('hud_split.cpp', 'hudDrawSplitVertical', 'humanCarGet', 'player'),
    ('weapon_missile_trails.c', 'carSetViewMode', 'viewSetCameraPose', 'human->player'),
    ('world_update.c', 'worldUpdate', 'viewCarCameraUpdate', 'i'),
    ('world_update.c', 'worldUpdate', 'viewSetFollowObjPos', 'i'),
    ('world_update.c', 'worldRadarPlotHumans', 'hudRadarPlotCar', 'j'),
]
_NOT_NAMES = {'__attribute__', 'section', 'aligned', 'packed', 'if', 'while', 'for', 'switch', 'return', 'sizeof'}


def c_bodies(text):
    """[(name, start, end)] of the function definitions of preprocessed C (bodies by brace depth; the name is the
    last `name(` before a top-level '{' that is not an initialiser; K&R parameter declarations allowed)."""
    out = []
    depth, k, n = 0, 0, len(text)
    last_top = seg_start = 0
    start = name = None
    linkage = 0     # open `extern "C" {` blocks (C++ units): transparent, their functions are top level
    while k < n:
        c = text[k]
        if c in '"\'':
            k += 1
            while k < n and text[k] != c:
                k += 2 if text[k] == '\\' else 1
        elif c == '{' and depth == 0 and re.search(r'extern\s*"C"\s*$', text[last_top:k]):
            linkage += 1
            last_top = seg_start = k + 1
        elif c == '}' and depth == 0 and linkage:
            linkage -= 1
            last_top = seg_start = k + 1
        elif c == '{':
            if depth == 0:
                name = None
                if '=' not in re.sub(r'\([^()]*\)', '', text[last_top:k]):
                    cands = [m.group(1) for m in re.finditer(r'\b([A-Za-z_]\w*)\s*\(', text[seg_start:k])
                             if m.group(1) not in _NOT_NAMES]
                    name = cands[-1] if cands else None
                start = k
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                if name:
                    out.append((name, start, k + 1))
                last_top = seg_start = k + 1
        elif c == ';' and depth == 0:
            last_top = k + 1
        k += 1
    return out


def call_args(text, k):
    """The arguments (whitespace normalised) of the call whose '(' is at text[k]."""
    depth, args, cur = 0, [], []
    for j in range(k, len(text)):
        c = text[j]
        if c == '(':
            depth += 1
            if depth == 1:
                continue
        elif c == ')':
            depth -= 1
            if depth == 0:
                args.append(''.join(cur))
                break
        elif c == ',' and depth == 1:
            args.append(''.join(cur))
            cur = []
            continue
        cur.append(c)
    return [re.sub(r'\s+', ' ', a).strip() for a in args]


def built_units():
    import json
    m = json.loads((ROOT / 'BUILD_MANIFEST.json').read_text(encoding='utf-8'))
    return sorted({u['source'] for u in m['units']} | {'src/semantic/port_variant_arrays.c'})


def map_census(defines):
    """{(unit, function, callee, index argument): count} and {(unit, function): body without blanks}."""
    from semantic_compile import preprocess as cpp
    pat = re.compile(r'\b(%s)\s*\(' % '|'.join(sorted(set(VIEW_CALLEES) | set(HUMAN_CALLEES))))
    sites, bodies = {}, {}
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        for rel in built_units():
            src = ROOT / rel
            if not pat.search(src.read_text(encoding='utf-8', errors='replace')):
                continue
            text = cpp(src, Path(tmp) / (src.stem + '.i'), defines).decode('ascii', errors='replace')
            text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
            for fn, b0, b1 in c_bodies(text):
                bodies[(src.name, fn)] = re.sub(r'\s+', '', text[b0:b1])
                for m in pat.finditer(text, b0, b1):
                    callee = m.group(1)
                    if callee in VIEW_CALLEES:
                        pos = VIEW_CALLEES[callee]
                    elif fn in HUD_FUNCS:
                        pos = HUMAN_CALLEES[callee]
                    else:
                        continue
                    args = call_args(text, m.end() - 1)
                    key = (src.name, fn, callee, args[pos] if pos < len(args) else '?')
                    sites[key] = sites.get(key, 0) + 1
    return sites, bodies


def check_map():
    ok = True
    sites, bodies = map_census(HUMANS_DEFINES)
    same16 = map_census(HUMANS16_DEFINES)[0] == sites      # H4: the same calls with 16 human records
    unreviewed = sorted(k for k in sites if k not in REVIEWED)
    stale = sorted(k for k in REVIEWED if k not in sites)
    for k in unreviewed:
        print('FAIL 4 not reviewed: %s %s() calls %s(.., %s ..) %d x' % (k + (sites[k],)))
    for k in stale:
        print('FAIL 4 reviewed call not found: %s %s() %s(.., %s ..)' % k)
    nomap = sorted(k for k, (kind, need) in REVIEWED.items()
                   if k in sites and need and need not in bodies.get((k[0], k[1]), ''))
    for k in nomap:
        print('FAIL 4 %s %s(): %s(.., %s ..) without `%s` in the body' % (k + (REVIEWED[k][1],)))
    kinds = {}
    for k in sites:
        if k in REVIEWED:
            kinds[REVIEWED[k][0]] = kinds.get(REVIEWED[k][0], 0) + sites[k]
    good = not unreviewed and not stale and not nomap and same16
    ok = ok and good
    print('%s 4 local map, TM2_PORT_HUMANS: %d calls at %d reviewed sites (%s); unreviewed %d, stale %d, without '
          'the map %d; the same at 16 humans: %s' % ('ok  ' if good else 'FAIL', sum(sites.values()), len(sites),
                          ', '.join('%s %d' % kv for kv in sorted(kinds.items())), len(unreviewed), len(stale),
                          len(nomap), 'yes' if same16 else 'NO'))
    neg, _ = map_census(NEGATIVE_DEFINES)
    missing = [k for k in RETAIL_HUMAN if k not in neg]
    good = not missing and any(k not in REVIEWED for k in neg)
    ok = ok and good
    print('%s 4 negative control (no TM2_PORT_HUMANS): the scan finds the %d retail human-indexed calls%s' % (
        'ok  ' if good else 'FAIL', len(RETAIL_HUMAN) - len(missing), (' (missing %s)' % missing) if missing else ''))
    return ok


# ---- 5. builds with TM2_PORT_HUMANS ----
H1_PORT = ['portHumanSlot', 'portLocalHuman', 'portLocalHumanCount', 'portHumanRadarPlot', 'worldHumanCamera',
           'worldHumanFollow',
           # H2: the input functions, the layouts, the cheat gates, the AI aim, the bit-4 AI blips (all pure)
           'portHumanPadPort', 'portPadButtonInWord', 'portHumanAction', 'portHumanCombo', 'portHumanAnyLatched',
           'portHumanSetLayout', 'portHumanSetLayouts', 'portHumanCheatsOff', 'portHumanCheatCode', 'portAiAimTarget',
           'worldRadarAiAll',
           # H3: sound sides and local feedback
           'portHumanSide', 'portHumanFeedback', 'portHumanMessage', 'portHumanSound', 'portHumanDenied',
           'portHumanLoopPlay', 'portHumanLoopStop', 'portHumanLocate',
           # H5: the target panel's search (presentation), the rules gate and the alive tests (pure)
           'portHumanNearestCar', 'portHumanRules', 'portHumanAlive', 'portHumanDown']
# H3: what runs, or plays differently, by locality: the gates above, the listener with views 2..3, the engine cap,
# and everything a remote human's positional branch calls instead of the local human's full-volume call
H3_LOCAL = ['sndListenerTransform', 'portCarSoundFrame', 'soundPlay', 'soundPlayVariant', 'soundPlayVariantEx',
            'soundLocateForView0', 'soundLocateForView', 'PadChkMtap', 'soundGetListenerPan', 'soundStartSharedLoop',
            'soundStopSharedLoop', 'soundStartPlayerLoop', 'soundStopPlayerLoop', 'hudMessagePost',
            'soundSetDeniedPending', 'portCarFlameSound', 'portCarTornadoSound']
LOCAL_ROOTS = H1_PORT + ['carViewHotkeys', 'carSetViewMode', 'viewCarCameraUpdate', 'hudDrawSingleView',
                         'hudDrawSplitVertical', 'hudDrawSplitHorizontal', 'hudDrawTargetPanel',
                         'carNearestToHuman'] + H3_LOCAL
H1_CHANGED = {'worldUpdate', 'viewCarCameraUpdate', 'carSetViewMode', 'carViewHotkeys', 'hudDrawSingleView',
              'hudDrawSplitVertical', 'hudDrawSplitHorizontal', 'hudDrawTargetPanel',
              # H2
              'humanCarReadInput', 'carReadMoveCodes', 'humanCarTick', 'gameGetLives', 'weaponFindTargetAhead',
              'weaponFindTargetBehind', 'sessionRun',
              # H3
              'carThrottleSteer', 'weaponTick', 'carSoundUpdate', 'pickupApply', 'weaponFireSelected',
              'weaponFireEnergyAttack', 'weaponFireSpecial', 'weaponSlamSwingUpdate', 'weaponSlamGrabUpdate',
              'targetActorsUpdate', 'carCrashStart', 'weaponFlameStart', 'carParisSinkZonesUpdate', 'carDestroyed',
              'sndListenerTransform',
              # H5 (worldUpdate, sessionRun, gameGetLives and the split HUDs again)
              'humanCarDamageUpdate', 'carNearestToHuman'}


def check_build_map(out):
    import json
    from check_hazards import load_functions, call_graph
    from reloc_image import Link
    out = Path(out)
    pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
    if 'TM2_PORT_HUMANS' not in {d.split('=')[0] for d in pv.get('port_defines') or []}:
        print('     5 %s: built without TM2_PORT_HUMANS (the map is not compiled): skipped' % out.as_posix())
        return True
    rows, kinds = load_functions(out)
    calls, _, _ = call_graph(Link(out), rows)
    missing = [n for n in H1_PORT if kinds.get(n) != 'port']
    seen, todo, via = set(), [r for r in LOCAL_ROOTS if r in kinds], {}
    while todo:
        fn = todo.pop()
        if fn in seen:
            continue
        seen.add(fn)
        for c in calls.get(fn, ()):
            via.setdefault(c, fn)
            todo.append(c)
    rand_path = []
    if 'rand' in seen:
        n = 'rand'
        while n in via and n not in LOCAL_ROOTS and len(rand_path) < 20:
            rand_path.append(n)
            n = via[n]
        rand_path.append(n)
    changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
    not_changed = sorted(H1_CHANGED - changed)
    good = not missing and 'rand' not in seen and not not_changed
    print('%s 5 %s: map functions in .port_text %d/%d%s; %d functions reachable from the local-only code, rand() %s; '
          'H1/H2/H3 game functions changed %d/%d%s' % (
              'ok  ' if good else 'FAIL', out.as_posix(), len(H1_PORT) - len(missing), len(H1_PORT),
              (' (missing %s)' % missing) if missing else '', len(seen),
              ('REACHED via ' + ' <- '.join(rand_path)) if rand_path else 'not reached',
              len(H1_CHANGED) - len(not_changed), len(H1_CHANGED), (' (unchanged %s)' % not_changed) if not_changed else ''))
    return good


# ---- 6. pad reads (H2) in the preprocessed C ----

# pad readers -> position of their pad-port argument
PAD_CALLEES = {
    'padCheckAction': 1, 'padComboHeld': 1, 'padAnyLatched': 0, 'padButtonHeld': 1, 'padActionHeld': 1,
    'padAnyPressed': 0, 'padAnyDown': 0, 'padLatchPort': 0, 'ctlpadAnyUp': 0, 'ctlpadAnyDown': 0, 'ctlpadAnyLeft': 0,
    'ctlpadAnyRight': 0, 'padClearActionCache': 0, 'padGetActiveBinding': 1, 'padActionIsBound': 1,
    'padSetLayout': 1, 'padLoadLayout': 0,
}
# the human input functions -> position of their human-index argument
HUMAN_INPUT_CALLEES = {'portHumanAction': 1, 'portHumanCombo': 1, 'portHumanAnyLatched': 0}


def pad_census(defines):
    """{(unit, function, callee, port argument): count} of every pad read and human-input call."""
    from semantic_compile import preprocess as cpp
    callees = dict(PAD_CALLEES)
    callees.update(HUMAN_INPUT_CALLEES)
    pat = re.compile(r'\b(%s)\s*\(' % '|'.join(sorted(callees)))
    sites, bodies = {}, {}
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        for rel in built_units():
            src = ROOT / rel
            if not pat.search(src.read_text(encoding='utf-8', errors='replace')):
                continue
            text = cpp(src, Path(tmp) / (src.stem + '.i'), defines).decode('ascii', errors='replace')
            text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
            for fn, b0, b1 in c_bodies(text):
                bodies[(src.name, fn)] = re.sub(r'\s+', '', text[b0:b1])
                for m in pat.finditer(text, b0, b1):
                    callee = m.group(1)
                    args = call_args(text, m.end() - 1)
                    pos = callees[callee]
                    key = (src.name, fn, callee, args[pos] if pos < len(args) else '?')
                    sites[key] = sites.get(key, 0) + 1
    return sites, bodies


# (unit, function, callee, port argument) -> (kind, the assignment the body must hold or None). Kinds: 'menu' a
# shell / menu / pause / scene-command read of this machine's pads (a constant port, 2 = either pad, or a pad
# variable of the screen); 'match start' sessionRun's pad 0/1 layouts and its wait for pad 0's release; 'attract'
# the attract demo's "any button on pad 0"; 'pad layer' the pad library's own port parameter or its two ports
# (shared_pad.c, controls.c, spawn_tables.c); 'input fn' a human input function's retail path, the port from
# portHumanPadPort (local slot 0/1 with bit 0 clear); 'human input' a game read of a human's input through the
# input functions, by the human record index (the only reads by that index).
_M, _P = 'menu', 'pad layer'
PAD_REVIEWED = {
    ('ai_route_helpers.c', 'movieSkipCheckDelayed', 'padCheckAction', '2'): (_M, None),
    ('car_destroyed.c', 'menuPasswordCursorRun', 'padAnyPressed', '0'): (_M, None),
    ('car_destroyed.c', 'menuPasswordCursorRun', 'padCheckAction', '0'): (_M, None),
    ('car_destroyed.c', 'menuPasswordCursorRun', 'padCheckAction', 'j'): (_M, None),
    ('car_parts_spin.c', 'padWaitConfirmOrTimeout', 'padCheckAction', 'p'): (_M, None),
    ('input_commands.cpp', 'sceneCmdPadInputRun', 'padCheckAction', '(s16)c->port'): (_M, None),
    ('input_commands.cpp', 'sceneCmdPadInputRun', 'padCheckAction', '2*(c->port==1)'): (_M, None),
    ('input_commands.cpp', 'sceneCmdPressAnyRun', 'padAnyPressed', '(s16)c->port'): (_M, None),
    ('menu_password_digits_create.c', 'movieSkipCheck', 'padCheckAction', '2'): (_M, None),
    ('screen_ai_select.c', 'screenAiSelect', 'padAnyPressed', '0'): (_M, None),
    ('screen_card_layout.c', 'padWaitAnyButtonOrTimeout', 'padAnyPressed', 'pad'): (_M, None),
    ('screen_pause_menu.c', 'pauseMenu', 'padAnyPressed', 'pad'): (_M, None),
    ('screen_pause_menu.c', 'pauseMenu', 'padCheckAction', '0'): (_M, None),
    ('screen_pause_menu.c', 'pauseMenu', 'padCheckAction', '1'): (_M, None),
    ('screen_pause_menu.c', 'pauseMenu', 'padCheckAction', 'pad'): (_M, None),
    ('session_runner.cpp', 'sessionRun', 'padCheckAction', '0'): ('match start', None),
    ('session_runner.cpp', 'sessionRun', 'padSetLayout', '0'): ('match start', None),
    ('session_runner.cpp', 'sessionRun', 'padSetLayout', '1'): ('match start', None),
    ('world_update.c', 'worldUpdate', 'padAnyPressed', '0'): ('attract', None),
    ('controls.c', 'padActionHeld', 'padButtonHeld', 'port'): (_P, None),
    ('controls.c', 'padActionIsBound', 'padGetActiveBinding', 'port'): (_P, None),
    ('controls.c', 'padAnyPressed', 'padAnyLatched', 'port'): (_P, None),
    ('controls.c', 'padAnyPressedEitherPort', 'padAnyLatched', '2'): (_P, None),
    ('controls.c', 'padCheckAction', 'padActionHeld', '(short)port'): (_P, None),
    ('controls.c', 'padCheckAction', 'padAnyLatched', '(short)port'): (_P, None),
    ('controls.c', 'padFlush', 'padClearActionCache', '0'): (_P, None),
    ('controls.c', 'padInit', 'padLoadLayout', 'port'): (_P, None),
    ('controls.c', 'padLatchWithRepeat', 'padAnyDown', '0'): (_P, None),
    ('controls.c', 'padLatchWithRepeat', 'padAnyDown', '1'): (_P, None),
    ('controls.c', 'padLatchWithRepeat', 'padLatchPort', '0'): (_P, None),
    ('controls.c', 'padLatchWithRepeat', 'padLatchPort', '1'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyDown', 'ctlpadAnyDown', '0'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyDown', 'ctlpadAnyDown', '1'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyLeft', 'ctlpadAnyLeft', '0'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyLeft', 'ctlpadAnyLeft', '1'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyRight', 'ctlpadAnyRight', '0'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyRight', 'ctlpadAnyRight', '1'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyUp', 'ctlpadAnyUp', '0'): (_P, None),
    ('shared_pad.c', 'ctlpadAnyUp', 'ctlpadAnyUp', '1'): (_P, None),
    ('shared_pad.c', 'padButtonHeld', 'padButtonHeld', '0'): (_P, None),
    ('shared_pad.c', 'padButtonHeld', 'padButtonHeld', '1'): (_P, None),
    ('spawn_tables.c', 'padSetLayout', 'padLoadLayout', 'port'): (_P, None),
    ('port_variant_arrays.c', 'portHumanAction', 'padCheckAction', 'port'): ('input fn', 'port=portHumanPadPort(h)'),
    ('port_variant_arrays.c', 'portHumanAnyLatched', 'padAnyLatched', 'port'): ('input fn', 'port=portHumanPadPort(h)'),
    ('port_variant_arrays.c', 'portHumanCombo', 'padComboHeld', '(short)port'): ('input fn', 'port=portHumanPadPort(h)'),
    ('car_input.c', 'carReadMoveCodes', 'portHumanAction', '(s16)player'): ('human input', 'player=car->player'),
    ('car_input.c', 'humanCarReadInput', 'portHumanAction', '(s16)player'): ('human input', 'player=car->player'),
    ('car_records.c', 'carViewHotkeys', 'portHumanCombo', 'player'): ('human input', 'player=car->player'),
    ('car_tick.c', 'humanCarTick', 'portHumanAnyLatched', 'car->player'): ('human input', None),
    ('car_tick.c', 'humanCarTick', 'portHumanCombo', 'car->player'): ('human input', None),
}
# without TM2_PORT_HUMANS (negative control): retail's pad reads by the human record index
RETAIL_PAD_HUMAN = [
    ('car_input.c', 'humanCarReadInput', 'padCheckAction', '(s16)player'),
    ('car_input.c', 'carReadMoveCodes', 'padCheckAction', '(s16)player'),
    ('car_tick.c', 'humanCarTick', 'padComboHeld', 'car->player'),
    ('car_tick.c', 'humanCarTick', 'padAnyLatched', 'car->player'),
    ('car_records.c', 'carViewHotkeys', 'padComboHeld', 'player'),
]
CHEAT_CODES = ('0x12233344', '0x21431122', '0x12344321')


def check_pads():
    ok = True
    sites, bodies = pad_census(HUMANS_DEFINES)
    same16 = pad_census(HUMANS16_DEFINES)[0] == sites      # H4: the same reads with 16 human records
    unreviewed = sorted(k for k in sites if k not in PAD_REVIEWED)
    stale = sorted(k for k in PAD_REVIEWED if k not in sites)
    for k in unreviewed:
        print('FAIL 6 not reviewed: %s %s() calls %s(.., %s ..) %d x' % (k + (sites[k],)))
    for k in stale:
        print('FAIL 6 reviewed call not found: %s %s() %s(.., %s ..)' % k)
    # the human record index reaches a pad only through the input functions
    wrong = sorted(k for k, (kind, need) in PAD_REVIEWED.items()
                   if (kind == 'human input') != (k[2] in HUMAN_INPUT_CALLEES))
    for k in wrong:
        print('FAIL 6 %s %s() %s(.., %s ..): a human-index read must go through the input functions' % k)
    nomap = sorted(k for k, (kind, need) in PAD_REVIEWED.items()
                   if k in sites and need and need not in bodies.get((k[0], k[1]), ''))
    for k in nomap:
        print('FAIL 6 %s %s(): %s(.., %s ..) without `%s` in the body' % (k + (PAD_REVIEWED[k][1],)))
    kinds = {}
    for k in sites:
        if k in PAD_REVIEWED:
            kinds[PAD_REVIEWED[k][0]] = kinds.get(PAD_REVIEWED[k][0], 0) + sites[k]
    good = not unreviewed and not stale and not wrong and not nomap and same16
    ok = ok and good
    print('%s 6 pad reads, TM2_PORT_HUMANS: %d calls at %d reviewed sites (%s); unreviewed %d, stale %d, human '
          'index outside the input functions %d, without the map %d; the same at 16 humans: %s' % (
              'ok  ' if good else 'FAIL', sum(sites.values()), len(sites),
              ', '.join('%s %d' % kv for kv in sorted(kinds.items())), len(unreviewed), len(stale), len(wrong),
              len(nomap), 'yes' if same16 else 'NO'))
    neg, _ = pad_census(NEGATIVE_DEFINES)
    missing = [k for k in RETAIL_PAD_HUMAN if k not in neg]
    good = not missing and not any(k[2] in HUMAN_INPUT_CALLEES for k in neg)
    ok = ok and good
    print('%s 6 negative control (no TM2_PORT_HUMANS): the scan finds the %d retail pad reads by human index%s' % (
        'ok  ' if good else 'FAIL', len(RETAIL_PAD_HUMAN) - len(missing), (' (missing %s)' % missing) if missing else ''))
    # 7. the cheats are gated (portHumanOptions bit 3)
    tick = bodies.get(('car_tick.c', 'humanCarTick'), '')
    gate = tick.find('if(portHumanCheatsOff())return;')
    first = min([i for i in (tick.find('portHumanCombo('), tick.find('portHumanAnyLatched(')) if i >= 0] or [-1])
    g1 = gate >= 0 and first > gate and 'padComboHeld(' not in tick and 'padAnyLatched(' not in tick
    moves = bodies.get(('car_input.c', 'carReadMoveCodes'), '')
    gate2 = moves.find('portHumanCheatCode(car->code)')
    sw = moves.find('switch(car->code)')
    cases = re.findall(r'case(0x[0-9A-Fa-f]+):(.*?)(?=case0x|default:|$)', moves[sw:] if sw >= 0 else '')
    feedback = sorted({c.upper().replace('0X', '0x') for c, body in cases
                       if 'hudMessagePost(' in body or 'portHumanMessage(' in body})   # H3: the message gate
    g2 = 0 <= gate2 < sw and feedback and set(feedback) <= {c.upper().replace('0X', '0x') for c in CHEAT_CODES}
    code = bodies.get(('port_variant_arrays.c', 'portHumanCheatCode'), '')
    off = bodies.get(('port_variant_arrays.c', 'portHumanCheatsOff'), '')
    g3 = all(c in code for c in CHEAT_CODES) and 'portHumanCheatsOff()' in code and '&0x08' in off
    good = g1 and g2 and g3
    ok = ok and good
    print('%s 7 cheats gated: humanCarTick returns on portHumanCheatsOff() before its combos %s; carReadMoveCodes '
          'consumes the cheat codes before its switch %s (codes with HUD messages: %s); portHumanCheatCode lists %s, '
          'bit 3 %s' % ('ok  ' if good else 'FAIL', 'yes' if g1 else 'NO', 'yes' if g2 else 'NO',
                        ', '.join(feedback) or 'none', ', '.join(CHEAT_CODES) if g3 else 'NOT all', 'yes' if g3 else 'NO'))
    return ok


# ---- 8. sound sides and local feedback (H3) in the preprocessed C ----

# callee -> position of the argument the census keys on: a sound side, a human index (the H3 gates), a message,
# or (the soundPlay family) the sound id of a full-volume call (volume and pan both literal 0)
FEEDBACK_CALLEES = {
    'soundStartPlayerLoop': 4, 'soundStopPlayerLoop': 1, 'soundMgrPlaySided': 5, 'soundMgrStopSided': 2,
    'hudMessagePost': 0, 'soundSetDeniedPending': 0,
    'portHumanMessage': 0, 'portHumanSound': 0, 'portHumanDenied': 0, 'portHumanLoopPlay': 4, 'portHumanLoopStop': 1,
    'soundPlay': 0, 'soundPlayVariant': 0, 'soundPlayVariantEx': 0,
}
FEEDBACK_GATES = ('portHumanMessage', 'portHumanSound', 'portHumanDenied', 'portHumanLoopPlay', 'portHumanLoopStop')
# the car stack with the per-car sound (portHumanLoopPlay's instance paths, the Dark Tooth player)
SOUND_DEFINES = ['TM2_PORT_VARIANT', 'TM2_PORT_CAR_GROUPS', 'TM2_PORT_CARS=16', 'TM2_PORT_CAR_SWAP',
                 'TM2_PORT_TEXTURE_BANKS', 'TM2_PORT_CAR_SPECIALS', 'TM2_PORT_DARK_TOOTH', 'TM2_PORT_CAR_SOUND',
                 'TM2_PORT_CAR_CLUTS', 'TM2_PORT_CAR_DROPIN', 'TM2_PORT_SPECIAL_AS', 'TM2_PORT_DT_PLAYER',
                 'TM2_PORT_HUMANS=4']


def _bare(a):
    """An argument without its outer parentheses."""
    while a.startswith('(') and a.endswith(')'):
        depth = 0
        for k, c in enumerate(a):
            depth += c == '('
            depth -= c == ')'
            if depth == 0:
                break
        if k != len(a) - 1:
            break           # the first '(' closes before the end: `(a) + (b)`
        a = a[1:-1].strip()
    return a


def feedback_census(defines):
    """{(unit, function, callee, keyed argument): count} and {(unit, function): body without blanks}."""
    from semantic_compile import preprocess as cpp
    pat = re.compile(r'\b(%s)\s*\(' % '|'.join(sorted(FEEDBACK_CALLEES)))
    sites, bodies = {}, {}
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        for rel in built_units():
            src = ROOT / rel
            if not pat.search(src.read_text(encoding='utf-8', errors='replace')):
                continue
            text = cpp(src, Path(tmp) / (src.stem + '.i'), defines).decode('ascii', errors='replace')
            text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
            for fn, b0, b1 in c_bodies(text):
                bodies[(src.name, fn)] = re.sub(r'\s+', '', text[b0:b1])
                for m in pat.finditer(text, b0, b1):
                    callee = m.group(1)
                    args = [_bare(a) for a in call_args(text, m.end() - 1)]
                    if callee.startswith('soundPlay'):
                        if len(args) < 3 or args[1] != '0' or args[2] != '0':
                            continue        # positional (or the gates' own pass-through): not feedback
                        arg = args[0]
                    else:
                        pos = FEEDBACK_CALLEES[callee]
                        arg = args[pos] if pos < len(args) else '?'
                    key = (src.name, fn, callee, arg)
                    sites[key] = sites.get(key, 0) + 1
    return sites, bodies


# (unit, function, callee, keyed argument) -> (kind, (unit, function) whose body must hold `need`, need) or None.
# Kinds: 'side' a loop by the sound side from portHumanSide; 'wrapper' a pass-through of the side; 'human gate' an
# H3 gate called with the human record index (the gate decides by the map); 'gate' the gated call inside an H3 gate;
# 'local branch' a full-volume call on the branch TM2_CAR_FEEDBACK / TM2_HUMAN_FEEDBACK takes for a local human only
# (the other branch plays positionally, as for a computer car); 'not feedback' a full-volume sound or message that
# is not a human's own feedback (world events, the boss, a car's tumble at full volume for computer cars too, the
# cars-left message, the pending denied buzz a local human set); 'menu' the shell.
_HG, _LB, _NF = 'human gate', 'local branch', 'not feedback'
_PV = 'port_variant_arrays.c'
_HREC = '((struct TM2HumanRecord *)(car))->player'
_HRECC = '((struct TM2HumanRecord *)car)->player'
FEEDBACK_REVIEWED = {
    (_PV, 'portHumanLoopPlay', 'soundStartPlayerLoop', 'side'): ('side', (_PV, 'portHumanLoopPlay'), 'side=portHumanSide(player)'),
    (_PV, 'portHumanLoopStop', 'soundStopPlayerLoop', 'side'): ('side', (_PV, 'portHumanLoopStop'), 'side=portHumanSide(player)'),
    ('command_wrappers.cpp', 'soundStartPlayerLoop', 'soundMgrPlaySided', 'e'): ('wrapper', None, None),
    ('command_wrappers.cpp', 'soundStopPlayerLoop', 'soundMgrStopSided', 'b'): ('wrapper', None, None),
    (_PV, 'portHumanMessage', 'hudMessagePost', 'msg'): ('gate', (_PV, 'portHumanMessage'), 'if(portHumanFeedback(h))'),
    (_PV, 'portHumanDenied', 'soundSetDeniedPending', '1'): ('gate', (_PV, 'portHumanDenied'), 'if(portHumanFeedback(h))'),
    ('car_control.c', 'carThrottleSteer', 'portHumanLoopPlay', _HREC): (_HG, None, None),
    ('car_control.c', 'carThrottleSteer', 'portHumanLoopStop', _HREC): (_HG, None, None),
    ('car_control.c', 'carThrottleSteer', 'portHumanSound', _HREC): (_HG, None, None),
    ('weapon_tick.c', 'weaponTick', 'portHumanLoopPlay', _HRECC): (_HG, None, None),
    ('weapon_tick.c', 'weaponTick', 'portHumanLoopStop', _HRECC): (_HG, None, None),
    ('screen_card_draw.c', 'carSoundUpdate', 'portHumanLoopPlay', 'a1[0]'): (_HG, None, None),
    ('screen_card_draw.c', 'carSoundUpdate', 'portHumanLoopStop', 'id'): (_HG, None, None),
    ('car_destroyed.c', 'carDestroyed', 'portHumanSound', _HREC): (_HG, None, None),
    ('car_input.c', 'carReadMoveCodes', 'portHumanMessage', 'player'): (_HG, ('car_input.c', 'carReadMoveCodes'), 'player=car->player'),
    ('car_input.c', 'carReadMoveCodes', 'portHumanSound', 'player'): (_HG, ('car_input.c', 'carReadMoveCodes'), 'player=car->player'),
    ('car_tick.c', 'humanCarTick', 'portHumanMessage', 'car->player'): (_HG, None, None),
    ('pickup_apply.c', 'pickupApply', 'portHumanDenied', _HRECC): (_HG, None, None),
    ('pickup_apply.c', 'pickupApply', 'portHumanMessage', _HRECC): (_HG, None, None),
    ('target_actor_update.c', 'targetActorsUpdate', 'portHumanMessage', 'i'): (_HG, None, None),
    ('target_actor_update.c', 'targetActorsUpdate', 'portHumanSound', 'i'): (_HG, None, None),
    ('weapon_select.c', 'weaponFireSelected', 'portHumanDenied', _HREC): (_HG, None, None),
    ('weapon_special.c', 'weaponFireEnergyAttack', 'portHumanDenied', _HRECC): (_HG, None, None),
    # full volume only on the local human's branch
    ('car_crash.c', 'carCrashStart', 'soundPlay', '37'): (_LB, ('car_crash.c', 'carCrashStart'), 'portHumanFeedback('),
    ('car_restart.c', 'weaponFlameStart', 'soundPlay', '47'): (_LB, ('car_restart.c', 'weaponFlameStart'), 'portHumanFeedback('),
    ('car_wheel_sway.c', 'carParisSinkZonesUpdate', 'soundPlay', '0x23'): (_LB, ('car_wheel_sway.c', 'carParisSinkZonesUpdate'), 'portHumanFeedback('),
    ('weapon_arcs_draw.c', 'gc15_feedback', 'soundPlay', 'command'): (_LB, ('weapon_arcs_draw.c', 'weaponSlamGrabUpdate'), 'portHumanFeedback('),
    ('weapon_slam_swing.c', 'g16_feedback', 'soundPlay', 'command'): (_LB, ('weapon_slam_swing.c', 'weaponSlamSwingUpdate'), 'portHumanFeedback('),
    ('weapon_special.c', 'weaponFireEnergyAttack', 'soundPlay', '13'): (_LB, ('weapon_special.c', 'weaponFireEnergyAttack'), 'portHumanFeedback('),
    ('weapon_basic.c', 'weaponFireSpecial', 'soundPlayVariantEx', '0x33'): (_LB, ('weapon_basic.c', 'weaponFireSpecial'), 'portHumanFeedback('),
    # not a human's own feedback
    ('ai_dark_tooth_boss.c', 'aiDarkToothBossUpdate', 'soundPlay', '6'): (_NF, None, None),
    ('ai_dark_tooth_boss.c', 'aiDarkToothBossUpdate', 'soundPlay', '61'): (_NF, None, None),
    ('ai_dark_tooth_boss.c', 'aiDarkToothBossUpdate', 'soundPlay', '66'): (_NF, None, None),
    (_PV, 'portCarDtPlayerSwoopStep', 'soundPlay', '6'): (_NF, None, None),     # the boss's head hit, as the boss's
    ('car_destroyed.c', 'carDestroyed', 'soundPlay', '25'): (_NF, None, None),  # a wreck within 1000 of the listener
    ('car_damage.c', 'carApplyDamage', 'hudMessagePost', 'aiCarsCountAlive()'): (_NF, None, None),   # cars left
    ('car_tumble.c', 'carBarrelRollStep', 'soundPlay', '6'): (_NF, None, None),  # full volume for an active AI too
    ('world_update.c', 'worldUpdate', 'soundPlay', '0x3F'): (_NF, None, None),   # the pending buzz, set by the gates
    ('world_update.c', 'worldUpdate', 'soundSetDeniedPending', '0'): (_NF, None, None),
    ('screen_credits.c', 'screenCredits', 'soundPlay', '27'): ('menu', None, None),
    ('screen_history_movie.cpp', 'screenHistoryMovie', 'soundPlay', '27'): ('menu', None, None),
    ('screen_options_audio.c', 'screenOptions', 'soundPlay', '27'): ('menu', None, None),
    ('screen_title_level_select.c', 'screenTitle', 'soundPlay', '27'): ('menu', None, None),
}
for _snd in ('0x27', '0x29', '0x2b', '0x2d', '0x31', '0x35', '0x37', '0x3b', '0x3d', '13'):
    FEEDBACK_REVIEWED[('weapon_basic.c', 'weaponFireSpecial', 'soundPlay', _snd)] = (
        _LB, ('weapon_basic.c', 'weaponFireSpecial'), 'portHumanFeedback(')
# without TM2_PORT_HUMANS (negative control): retail's sides and feedback by the human record index
RETAIL_FEEDBACK_HUMAN = [
    ('car_control.c', 'carThrottleSteer', 'soundStartPlayerLoop', _HREC),
    ('weapon_tick.c', 'weaponTick', 'soundStartPlayerLoop', _HRECC),
    ('screen_card_draw.c', 'carSoundUpdate', 'soundStartPlayerLoop', 'a1[0]'),
    ('car_input.c', 'carReadMoveCodes', 'hudMessagePost', '0x1F'),
    ('car_tick.c', 'humanCarTick', 'hudMessagePost', '0x23'),
    ('pickup_apply.c', 'pickupApply', 'hudMessagePost', '0x24'),
    ('weapon_select.c', 'weaponFireSelected', 'soundSetDeniedPending', '1'),
    ('car_control.c', 'carThrottleSteer', 'soundPlay', '0x3F'),
]


def check_feedback():
    ok = True
    sites, bodies = {}, {}
    per = {}
    for defines in (HUMANS_DEFINES, SOUND_DEFINES):
        s, b = feedback_census(defines)
        per[tuple(defines)] = s
        for k, n in s.items():
            sites[k] = max(sites.get(k, 0), n)
        bodies.update(b)
    # H4: the same calls with 16 human records
    same16 = all(feedback_census([('TM2_PORT_HUMANS=16' if d.startswith('TM2_PORT_HUMANS=') else d) for d in dset])[0]
                 == per[tuple(dset)] for dset in (HUMANS_DEFINES, SOUND_DEFINES))
    unreviewed = sorted(k for k in sites if k not in FEEDBACK_REVIEWED)
    stale = sorted(k for k in FEEDBACK_REVIEWED if k not in sites)
    for k in unreviewed:
        print('FAIL 8 not reviewed: %s %s() calls %s(.., %s ..) %d x' % (k + (sites[k],)))
    for k in stale:
        print('FAIL 8 reviewed call not found: %s %s() %s(.., %s ..)' % k)
    nogate = sorted(k for k, (kind, where, need) in FEEDBACK_REVIEWED.items()
                    if k in sites and need and need not in bodies.get(where, ''))
    for k in nogate:
        print('FAIL 8 %s %s() %s(.., %s ..): `%s` not in %s()' % (k + (FEEDBACK_REVIEWED[k][2],
                                                                   FEEDBACK_REVIEWED[k][1][1])))
    # a human-index argument reaches a side, a message or the denied buzz only through the H3 gates
    wrong = sorted(k for k, (kind, where, need) in FEEDBACK_REVIEWED.items()
                   if (kind == _HG) != (k[2] in FEEDBACK_GATES and k[1] not in FEEDBACK_GATES))
    for k in wrong:
        print('FAIL 8 %s %s() %s(.., %s ..): a human-index side / message / sound must go through the gates' % k)
    kinds = {}
    for k in sites:
        if k in FEEDBACK_REVIEWED:
            kinds[FEEDBACK_REVIEWED[k][0]] = kinds.get(FEEDBACK_REVIEWED[k][0], 0) + sites[k]
    good = not unreviewed and not stale and not nogate and not wrong and same16
    ok = ok and good
    print('%s 8 sound sides and feedback, TM2_PORT_HUMANS (+ the car-sound stack): %d calls at %d reviewed sites (%s); '
          'unreviewed %d, stale %d, without their gate %d, human index outside the gates %d; the same at 16 humans: '
          '%s' % ('ok  ' if good else 'FAIL', sum(sites.values()), len(sites),
                  ', '.join('%s %d' % kv for kv in sorted(kinds.items())), len(unreviewed), len(stale), len(nogate),
                  len(wrong), 'yes' if same16 else 'NO'))
    neg, _ = feedback_census(NEGATIVE_DEFINES)
    missing = [k for k in RETAIL_FEEDBACK_HUMAN if k not in neg]
    good = not missing and not any(k[2] in FEEDBACK_GATES for k in neg)
    ok = ok and good
    print('%s 8 negative control (no TM2_PORT_HUMANS): the scan finds retail\'s sides and feedback by human index '
          '(%d of %d listed)%s' % ('ok  ' if good else 'FAIL', len(RETAIL_FEEDBACK_HUMAN) - len(missing),
                                   len(RETAIL_FEEDBACK_HUMAN), (' (missing %s)' % missing) if missing else ''))
    return ok


# ---- 9. the storage for 16 humans (H4) ----
# HUMANS16_PLAN.md 1.1 and the H1/H2 storage: every array with one entry per human record
PER_HUMAN = ['humanCars', 'humanCarObjects', 'humanCarObjectSlots', 'padConfigs', 'humanCarTypes', 'hudSmoothedSpeeds',
             'carViewHotkeyReleased', 'carResetWeapons', 'sessionPortHumanCars', 'portHumanLocal', 'portHumanPad',
             'portHumanSteer', 'portHumanLayout', 'portHumanLives', 'portHumanBindings']
# what retail's own definitions hold (the variant's #else / below-K branches)
RETAIL_DIM = {'humanCarObjects': 2, 'carResetWeapons': 2}
H4_ASSERTS = ['tm2_h4_human_records', 'tm2_h4_human_words', 'tm2_h4_human_map', 'tm2_h4_bind_group',
              'tm2_h4_reset_weapons', 'tm2_h4_human_cars']
_DECL_WORDS = re.compile(r'^(?:\s*(?:extern|static|const|volatile|unsigned|signed|struct\s+\w+|[A-Za-z_]\w*|\*))+\s*$')
_NOT_TYPES = {'return', 'case', 'else', 'sizeof', 'goto', 'do', 'if', 'while', 'for', 'switch'}


def per_human_rows():
    import gen_link
    return {a['name']: a for a in gen_link.read_port_arrays()
            if a['group'] in ('humans', 'humans-2') or a['group'].startswith('humans>')}


def _expected_dim(row, n):
    """First dimension a declaration may have with n human records (None = unsized), and whether it is moved."""
    g = row['group']
    if g == 'humans-2':
        return n - 2, True
    if g.startswith('humans>'):
        k = int(g.split('>')[1])
        return (n, True) if n > k else (RETAIL_DIM.get(row['name'], 2), False)
    return n, True


def _eval_dim(text):
    t = re.sub(r'\s+', ' ', text).strip()
    if not t:
        return None
    if not re.fullmatch(r'[0-9xXa-fA-F()+\-*/ ]+', t):
        return '?' + t
    try:
        return int(eval(t.replace('/', '//'), {'__builtins__': {}}))
    except Exception:
        return '?' + t


def declaration_census(defines, names):
    """{(unit, name): {first dimensions}} of every declaration / definition of `names` in the built units (the
    preprocessed C; headers count in each unit that includes them)."""
    from semantic_compile import preprocess as cpp
    pat = re.compile(r'\b(%s)\s*\[' % '|'.join(names))
    out = {}
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        for rel in built_units():
            src = ROOT / rel
            if not pat.search(src.read_text(encoding='utf-8', errors='replace')):
                continue
            text = cpp(src, Path(tmp) / (src.stem + '.i'), defines).decode('ascii', errors='replace')
            text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
            for m in pat.finditer(text):
                k = m.start()
                j = max(text.rfind(';', 0, k), text.rfind('{', 0, k), text.rfind('}', 0, k), text.rfind(',', 0, k),
                        text.rfind('(', 0, k), text.rfind('=', 0, k))
                pre = text[j + 1:k]
                if text[j] in ',(=' or not _DECL_WORDS.match(pre) or not pre.strip():
                    continue
                if set(re.findall(r'[A-Za-z_]\w*', pre)) & _NOT_TYPES:
                    continue
                depth, e = 0, m.end() - 1
                for e in range(m.end() - 1, len(text)):
                    depth += {'[': 1, ']': -1}.get(text[e], 0)
                    if depth == 0:
                        break
                out.setdefault((src.name, m.group(1)), set()).add(_eval_dim(text[m.end():e]))
    return out


# settings record fields per human (TM2Settings, settings.h) and their entries; the setters that write them
SETTINGS_FIELDS = {'human_car': 2, 'lives': 4, 'control_layout': 2}
SETTINGS_SETTERS = {'settingsSetHumanCar': ('human_car', 1), 'settingsSetLives': ('lives', 1),
                    'settingsSetControlLayout': ('control_layout', 1)}
# (unit, function, field or setter, index) -> why it stays inside the record
SETTINGS_REVIEWED = {
    ('menu_state_accessors.c', 'settingsSetControlLayout', 'control_layout', 'player'):
        'accessor: its callers are on this list (the controller menus, players 0/1)',
    ('screen_car_select.c', 'screenCarSelect', 'settingsSetHumanCar', 'player'):
        'menu: car select of the menus\' players (1-2 humans; past 1 the accessor uses the port table)',
    ('screen_car_select.c', 'screenCarSelect', 'settingsSetHumanCar', 'flag'):
        'menu: car select of the menus\' players (1-2 humans)',
    ('screen_controller_multiplayer.c', 'screenControllerLayout', 'settingsSetControlLayout',
     'controlsLayoutPlayer'): 'menu: the controller layout screen of pad 0/1',
}
# without the variant (control): retail's settingsSetHumanCar writes human_car[player] itself (the variant goes
# through TM2_SETTINGS_HUMAN_CAR, the port table past human 1)
RETAIL_SETTINGS_WRITES = {('menu_state_accessors.c', 'settingsSetHumanCar', 'human_car', 'player')}


def settings_census(defines):
    """{(unit, function, field or setter, index): count} of the writes of the record's per-human fields with an
    index that is not a constant inside the field, and {(unit, function): body} for settingsCopy."""
    from semantic_compile import preprocess as cpp
    fields = '|'.join(SETTINGS_FIELDS)
    setters = '|'.join(SETTINGS_SETTERS)
    direct = re.compile(r'->\s*(%s)\s*\[' % fields)
    calls = re.compile(r'\b(%s)\s*\(' % setters)
    sites, bodies = {}, {}
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        for rel in built_units():
            src = ROOT / rel
            raw = src.read_text(encoding='utf-8', errors='replace')
            if not re.search(r'\b(%s|%s|settingsCopy)\b' % (fields, setters), raw):
                continue
            text = cpp(src, Path(tmp) / (src.stem + '.i'), defines).decode('ascii', errors='replace')
            text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
            for fn, b0, b1 in c_bodies(text):
                if fn == 'settingsCopy':
                    bodies[(src.name, fn)] = text[b0:b1]
                for m in direct.finditer(text, b0, b1):
                    depth, e = 0, m.end() - 1
                    for e in range(m.end() - 1, b1):
                        depth += {'[': 1, ']': -1}.get(text[e], 0)
                        if depth == 0:
                            break
                    idx = re.sub(r'\s+', ' ', text[m.end():e]).strip()
                    after = text[e + 1:e + 4]
                    before = text[max(b0, m.start() - 40):m.start()]
                    write = (re.match(r'\s*=[^=]', after) or re.search(r'(\+\+|--)\s*\(?\s*\w+\s*$', before)
                             or re.match(r'\s*(\+\+|--|[-+*/|&]=)', after))
                    # the port table's macro: `&(s)->human_car[i]` behind `(i) < 2 ?`
                    macro = m.group(1) == 'human_car' and re.search(r'<\s*2\s*\)?\s*\?\s*&\s*\(?\s*\w+\s*\)?\s*$',
                                                                    before)
                    if macro:
                        continue
                    if not write and not re.search(r'&\s*\(?\s*\w+\s*\)?\s*$', before):
                        continue
                    if re.fullmatch(r'\d+', idx) and int(idx) < SETTINGS_FIELDS[m.group(1)]:
                        continue
                    key = (src.name, fn, m.group(1), idx)
                    sites[key] = sites.get(key, 0) + 1
                for m in calls.finditer(text, b0, b1):
                    if fn == m.group(1):
                        continue
                    field, pos = SETTINGS_SETTERS[m.group(1)]
                    args = call_args(text, m.end() - 1)
                    idx = args[pos] if pos < len(args) else '?'
                    if re.fullmatch(r'\d+', idx) and int(idx) < SETTINGS_FIELDS[field]:
                        continue
                    key = (src.name, fn, m.group(1), idx)
                    sites[key] = sites.get(key, 0) + 1
    return sites, bodies


# settingsCopy's record (menu_state_copy.cpp TM2MenuSelection): list -> (offset, entries, the count it runs to)
COPY_LISTS = {'list8': (0x08, 2, 'count'), 'list10': (0x0A, 10, 'count2'), 'list20': (0x14, 4, 'count'),
              'list38': (0x26, 2, 'count')}
SETTINGS_BYTES = 0x2C   # what sessionSettings owns in RAM (screenCardDrawEnvs at 80164788 follows)


def copy_extent(body, humans, opponents):
    """The highest byte offset settingsCopy writes with `humans` / `opponents` in the record, and the lists whose
    loop is not bounded by its own length."""
    top, unbounded = 0, []
    for lst, (off, n, cnt) in COPY_LISTS.items():
        m = re.search(r'for\s*\(([^;]*);([^;]*);[^)]*\)\s*dest\s*->\s*%s\s*\[' % lst, body)
        if not m:
            return None, [lst]
        cond = re.sub(r'\s+', '', m.group(2))
        bounded = ('sizeof(dest->%s)' % lst) in cond
        if not bounded:
            unbounded.append(lst)
        count = humans if cnt == 'count' else opponents
        last = min(count, n) - 1 if bounded else count - 1
        top = max(top, off + last)
    return top, unbounded


def check_storage():
    ok = True
    rows = per_human_rows()
    missing = [n for n in PER_HUMAN if n not in rows]
    good = not missing
    ok = ok and good
    print('%s 9 per-human arrays of the census in port_variant_arrays.tsv humans groups: %d/%d%s' % (
        'ok  ' if good else 'FAIL', len(PER_HUMAN) - len(missing), len(PER_HUMAN),
        (' (missing %s)' % missing) if missing else ''))
    names = sorted(set(PER_HUMAN) | set(rows))
    for label, defines, n in (('retail', [], 2), ('TM2_PORT_HUMANS=4', HUMANS_DEFINES, 4),
                              ('TM2_PORT_HUMANS=16', HUMANS16_DEFINES, 16)):
        decl = declaration_census(defines, names)
        bad = []
        for (unit, name), dims in sorted(decl.items()):
            if name not in rows:
                continue
            want = 2 if not defines else _expected_dim(rows[name], n)[0]
            if not defines and name == 'sessionPortHumanCars':
                want = None
            for d in dims:
                if d is not None and d != want:
                    bad.append('%s %s[%s] (want %s)' % (unit, name, d, want))
        seen = sorted({name for (unit, name) in decl})
        good = not bad and (bool(defines) or any(d == 2 for dims in decl.values() for d in dims))
        ok = ok and good
        for b in bad:
            print('FAIL 9 %s: %s' % (label, b))
        print('%s 9 %s: %d declarations / definitions of %d per-human arrays in the built units, every first '
              'dimension %s' % ('ok  ' if good else 'FAIL', label, sum(len(v) for v in decl.values()), len(seen),
                                ('as its group gives (%d humans)' % n) if defines else 'retail\'s 2 (control)'))
    # (b) settings: settingsCopy and every other write of the record's per-human fields
    for label, defines, humans in (('TM2_PORT_HUMANS=16', HUMANS16_DEFINES, 16), ('TM2_PORT_HUMANS=4', HUMANS_DEFINES, 4),
                                   ('retail', [], 2)):
        sites, bodies = settings_census(defines)
        body = bodies.get(('menu_state_copy.cpp', 'settingsCopy'), '')
        top, unbounded = copy_extent(body, humans, 15 if defines else 9)
        if defines == HUMANS16_DEFINES:
            good = top is not None and top < SETTINGS_BYTES and not unbounded
            print('%s 9 %s: settingsCopy writes up to record byte +0x%X (the record owns 0x%X; every list loop stops '
                  'at its list\'s length%s)' % ('ok  ' if good else 'FAIL', label, top if top is not None else -1,
                                               SETTINGS_BYTES, '' if not unbounded else ', NOT: %s' % unbounded))
            ok = ok and good
        else:
            good = top is not None and top < SETTINGS_BYTES and len(unbounded) == len(COPY_LISTS)
            print('%s 9 %s: settingsCopy keeps retail\'s loops and writes up to +0x%X with %d humans (inside 0x%X); '
                  'retail\'s loops with 16 humans would reach +0x%X' % (
                      'ok  ' if good else 'FAIL', label, top if top is not None else -1, humans, SETTINGS_BYTES,
                      COPY_LISTS['list38'][0] + 15))
            ok = ok and good
        reviewed = set(SETTINGS_REVIEWED) | (set() if defines else RETAIL_SETTINGS_WRITES)
        unreviewed = sorted(k for k in sites if k not in reviewed)
        stale = sorted(k for k in reviewed if k not in sites)
        for k in unreviewed:
            print('FAIL 9 %s: not reviewed: %s %s() writes %s[%s] %d x' % ((label,) + k + (sites[k],)))
        for k in stale:
            print('FAIL 9 %s: reviewed write not found: %s %s() %s[%s]' % ((label,) + k))
        good = not unreviewed and not stale
        ok = ok and good
        print('%s 9 %s: writes of the settings record\'s per-human fields with a variable index: %d at %d reviewed '
              'sites (menus and accessors); the rest constant inside the field or through TM2_SETTINGS_HUMAN_CAR' % (
                  'ok  ' if good else 'FAIL', label, sum(sites.values()), len(sites)))
    # (c) the static asserts, compiled at 16 humans
    from semantic_compile import preprocess as cpp
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        src = ROOT / 'src/semantic/port_variant_arrays.c'
        try:
            text = cpp(src, Path(tmp) / 'pva.i', HUMANS16_DEFINES).decode('ascii', errors='replace')
        except SystemExit as e:
            text = ''
            print('FAIL 9 port_variant_arrays.c at 16 humans: %s' % str(e)[-300:])
    found = [a for a in H4_ASSERTS if re.search(r'typedef\s+char\s+%s\s*\[' % a, text)]
    good = len(found) == len(H4_ASSERTS)
    ok = ok and good
    print('%s 9 static asserts of the storage compiled at 16 humans: %d/%d (%s)' % (
        'ok  ' if good else 'FAIL', len(found), len(H4_ASSERTS), ', '.join(found)))
    return ok


def check_build_storage(out):
    import json
    out = Path(out)
    pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
    n = (pv.get('limits') or {}).get('humans')
    if not n:
        print('     9 %s: not a port variant build: skipped' % out.as_posix())
        return True
    arrays = {a['name']: a for a in pv.get('arrays', [])}
    bad = []
    for name, row in sorted(per_human_rows().items()):
        want, moved = _expected_dim(row, n)
        a = arrays.get(name)
        if not moved:
            if a is not None:
                bad.append('%s moved (%d entries) with %d humans' % (name, a['entries'], n))
        elif a is None or a['entries'] != want:
            bad.append('%s: %s entries, want %d' % (name, a and a['entries'], want))
    base = int(pv.get('base') or '0', 16) if isinstance(pv.get('base'), str) else (pv.get('base') or 0)
    end = max(int(s['address'], 16) + s['size'] for s in pv['sections'])
    ram_end = int(pv.get('ram_end') or '80C00000', 16) if isinstance(pv.get('ram_end'), str) else (pv.get('ram_end') or 0x80C00000)
    good = not bad and end <= ram_end
    for b in bad:
        print('FAIL 9 %s: %s' % (out.as_posix(), b))
    crw = arrays.get('carResetWeapons')
    print('%s 9 %s: %d humans, every per-human array sized (carResetWeapons %s); port end %08X (RAM end %08X)' % (
        'ok  ' if good else 'FAIL', out.as_posix(), n, ('in port storage, %d entries' % crw['entries']) if crw else
        'in retail storage', end, ram_end))
    return good


# ---- 10. the rules (H5) in the preprocessed C ----
ROUND_PAT = re.compile(r'\brtScheduleEvent\s*\(|\b(roundOverFlag|roundWonFlag)\s*=(?!=)|\bcarRestart\s*\(')
# (unit, function, site) -> (kind, gate the body holds before the site, or None). site: 'event:<value>' (an
# rtScheduleEvent call by its first argument), '<flag>=<value>' (a store), 'restart' (a carRestart call). Kinds:
# 'rule' a fixed-human rule of retail behind the bit-1 gate; 'port' the port's trigger; 'start' a round start's
# reset; 'boss' Dark Tooth's stage 2 (aiCars[0], no human); 'attract' the attract demo; 'quit' the pause menu.
_GD = 'if(portHumanRules())return;'
_GW = '!portHumanRules()&&aiCarsRemaining<=0'
ROUND_REVIEWED = {
    ('car_damage_human.c', 'humanCarDamageUpdate', 'roundOverFlag=1'): ('rule', _GD),
    ('car_damage_human.c', 'humanCarDamageUpdate', 'event:1'): ('rule', _GD),
    ('car_damage_human.c', 'humanCarDamageUpdate', 'event:3'): ('rule', _GD),
    ('car_damage_human.c', 'humanCarDamageUpdate', 'event:4'): ('rule', _GD),
    ('world_update.c', 'worldUpdate', 'roundWonFlag=1'): ('rule', _GW),
    ('world_update.c', 'worldUpdate', 'roundOverFlag=1'): ('rule', _GW),
    ('world_update.c', 'worldUpdate', 'event:3'): ('rule', _GW),
    ('world_update.c', 'worldUpdate', 'event:1'): ('attract', None),         # any button on pad 0 in the demo
    ('port_variant_arrays.c', 'portRoundEnd', 'roundOverFlag=1'): ('port', None),
    ('port_variant_arrays.c', 'portRoundEnd', 'roundWonFlag=1'): ('port', None),
    ('port_variant_arrays.c', 'portRoundEnd', 'event:event'): ('port', None),
    ('scene_init.c', 'roundStart', 'roundWonFlag=0'): ('start', None),
    ('scene_init.c', 'roundStart', 'roundOverFlag=0'): ('start', None),
    ('scene_init.c', 'aiCarsCountRemaining', 'roundOverFlag=0'): ('boss', None),
    ('scene_init.c', 'aiCarsCountRemaining', 'event:3'): ('boss', None),
    ('ua_init.c', 'uaInit', 'roundOverFlag=0'): ('start', None),
    ('ua_init.c', 'uaInit', 'roundWonFlag=0'): ('start', None),
    ('spawn_tables.c', 'demoRespawnAiCars', 'event:1'): ('attract', None),
    ('screen_pause_menu.c', 'pauseMenu', 'event:2'): ('quit', None),
}
# every in-place restart (carRestart) and why it is not a respawn of a human the port's rules leave down
RESTART_REVIEWED = {
    ('arena_setup.c', 'humanCarsRestart', 'restart'): 'round start (roundStart): every human, a new round',
    ('scene_init.c', 'carBindHumanSlot', 'restart'): 'the binding (match / round start, the drop-in with START_RETAIL)',
    ('car_tick.c', 'humanCarTick', 'restart'): 'restart_pending: no writer but 0 / a saved value (checked)',
    ('arena_load_register.c', 'sub_8011E114', 'restart'): 'dead code: no caller (checked)',
}
# rule function -> (the gate as it reads in the body without blanks, the fixed-human reads that must follow it)
RULE_GATES = {
    ('car_damage_human.c', 'humanCarDamageUpdate'): (_GD, ['humanCars[0].alt_sfx', 'if(i==0)', 'roundOverFlag=1',
                                                           'rtScheduleEvent(']),
    ('world_update.c', 'worldUpdate'): (_GW, ['humanCars[0].f_09C', 'roundWonFlag=1']),
    ('session_runner.cpp', 'sessionRun'): ('value=portRoundResults(s,result);if(value>=0){quit=value;continue;}',
                                           ['--s->lives[0]', 'humanCarGet(0)->', 'screenChallengeResult(',
                                            'screenDeathmatchResult(']),
    ('ai_steps.c', 'carNearestToHuman'): ('if(portHumanRules())returnportHumanNearestCar(a,human);',
                                          ['humanCarNearest(']),
}
H5_PORT = ['portHumanRules', 'portHumanAlive', 'portHumanDown', 'portHumanAliveCount', 'portAiAliveCount',
           'portRoundEnd', 'portRoundResults', 'portHumanRespawn', 'portHumanNearestCar']
H5_BODIES = H5_PORT + ['portHumanPresent', 'portAiAlive', 'portHumanRestartRecord']
FIXED_HUMAN = re.compile(r'humanCars\[0\]|humanCarObjects\[0\]|humanCarGet\(0\)|humanCarObject\(0\)|lives\[0\]|'
                         r'H5_HUMAN\(0\)|\b(h|a|i|k|human)==0\b|portHumanLives\[0\]')
MATCH_RESETS = ['weaponDropsReset(', 'weaponsRoundReset(', 'projectilesReset(', 'fxSpritesReset(', 'hudMessageReset(']


def round_census(defines):
    """{(unit, function, site): count} and {(unit, function): body without blanks} of the built units."""
    from semantic_compile import preprocess as cpp
    sites, bodies = {}, {}
    names = ('rtScheduleEvent', 'roundOverFlag', 'roundWonFlag', 'carRestart', 'restart_pending', 'sub_8011E114',
             'portHumanRules', 'portRound')
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        for rel in built_units():
            src = ROOT / rel
            raw = src.read_text(encoding='utf-8', errors='replace')
            if not any(n in raw for n in names):
                continue
            text = cpp(src, Path(tmp) / (src.stem + '.i'), defines).decode('ascii', errors='replace')
            text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
            for fn, b0, b1 in c_bodies(text):
                bodies[(src.name, fn)] = re.sub(r'\s+', '', text[b0:b1])
                for m in ROUND_PAT.finditer(text, b0, b1):
                    if m.group(1):
                        rest = re.match(r'\s*([^;]*);', text[m.end():])
                        site = '%s=%s' % (m.group(1), re.sub(r'\s+', '', rest.group(1)) if rest else '?')
                    elif m.group(0).startswith('carRestart'):
                        site = 'restart'
                    else:
                        site = 'event:' + _bare(call_args(text, m.end() - 1)[0])
                    key = (src.name, fn, site)
                    sites[key] = sites.get(key, 0) + 1
    return sites, bodies


def _after_gate(body, gate, reads):
    """The reads (strings of the blank-free body) that occur before the gate (all of them when the gate is
    missing)."""
    g = body.find(gate)
    if g < 0:
        return [r for r in reads if r in body]
    return [r for r in reads if 0 <= body.find(r) < g]


def check_rules():
    ok = True
    sites, bodies = round_census(HUMANS_DEFINES)
    sites16, _ = round_census(HUMANS16_DEFINES)
    same16 = sites16 == sites
    rounds = {k: n for k, n in sites.items() if k[2] != 'restart'}
    unreviewed = sorted(k for k in rounds if k not in ROUND_REVIEWED)
    stale = sorted(k for k in ROUND_REVIEWED if k not in rounds)
    nogate = sorted(k for k, (kind, gate) in ROUND_REVIEWED.items()
                    if k in rounds and gate and gate not in bodies.get(k[:2], ''))
    for k in unreviewed:
        print('FAIL 10 not reviewed: %s %s() round end %s, %d x' % (k + (rounds[k],)))
    for k in stale:
        print('FAIL 10 reviewed round end not found: %s %s() %s' % k)
    for k in nogate:
        print('FAIL 10 %s %s() %s: no `%s` gate' % (k + (ROUND_REVIEWED[k][1],)))
    kinds = {}
    for k, n in rounds.items():
        if k in ROUND_REVIEWED:
            kinds[ROUND_REVIEWED[k][0]] = kinds.get(ROUND_REVIEWED[k][0], 0) + n
    good = not unreviewed and not stale and not nogate and same16
    ok = ok and good
    print('%s 10 round ends, TM2_PORT_HUMANS: %d at %d reviewed sites (%s); unreviewed %d, stale %d, rules without '
          'the bit-1 gate %d; the same at 16 humans: %s' % (
              'ok  ' if good else 'FAIL', sum(rounds.values()), len(rounds),
              ', '.join('%s %d' % kv for kv in sorted(kinds.items())), len(unreviewed), len(stale), len(nogate),
              'yes' if same16 else 'NO'))
    # (b) every fixed-human read of a rule after its gate; the split HUDs' lives by their own human
    bad = []
    for key, (gate, reads) in RULE_GATES.items():
        body = bodies.get(key, '')
        if gate not in body:
            bad.append('%s %s(): no gate `%s`' % (key + (gate,)))
            continue
        missing = [r for r in reads if r not in body]
        early = _after_gate(body, gate, reads)
        if missing:
            bad.append('%s %s(): rule reads not found %s (update RULE_GATES)' % (key + (missing,)))
        if early:
            bad.append('%s %s(): fixed-human rule reads before the gate: %s' % (key + (early,)))
    for fn in ('hudDrawSplitVertical', 'hudDrawSplitHorizontal'):
        if 'gameGetLives(0)' in bodies.get(('hud_split.cpp', fn), 'gameGetLives(0)'):
            bad.append('hud_split.cpp %s(): gameGetLives(0) (co-op lives on human 0)' % fn)
    # (c) the H5 port functions read no fixed human index
    for fn in H5_BODIES:
        body = bodies.get(('port_variant_arrays.c', fn))
        if body is None:
            bad.append('port_variant_arrays.c %s(): not found' % fn)
            continue
        hits = sorted({m.group(0) for m in FIXED_HUMAN.finditer(body)})
        if hits:
            bad.append('port_variant_arrays.c %s(): fixed human index %s' % (fn, hits))
    for b in bad:
        print('FAIL 10 ' + b)
    good = not bad
    ok = ok and good
    print('%s 10 no round end or result on a fixed human index with bit 1: %d rule functions gated before their '
          'fixed-human reads, the split HUDs\' lives by their own human, %d H5 port functions without a fixed human '
          'index' % ('ok  ' if good else 'FAIL', len(RULE_GATES), len(H5_BODIES)))
    # (d) the respawn path
    restarts = {k: n for k, n in sites.items() if k[2] == 'restart'}
    bad = ['not reviewed: %s %s() carRestart' % k[:2] for k in sorted(restarts) if k not in RESTART_REVIEWED]
    bad += ['reviewed carRestart not found: %s %s()' % k[:2] for k in sorted(RESTART_REVIEWED) if k not in restarts]
    for key, body in bodies.items():
        for m in re.finditer(r'restart_pending=([^;]*);', body):
            if m.group(1) not in ('0', 'restart'):
                bad.append('%s %s(): restart_pending = %s (a writer: carRestart in place)' % (key + (m.group(1),)))
        if 'sub_8011E114(' in body and key[1] != 'sub_8011E114':
            bad.append('%s %s() calls sub_8011E114 (carRestart in place)' % key)
    resp = bodies.get(('port_variant_arrays.c', 'portHumanRespawn'), '')
    rec = bodies.get(('port_variant_arrays.c', 'portHumanRestartRecord'), '')
    if 'portHumanRestartRecord(human,' not in resp or 'carRestart(' in resp or 'carBindHumanSlot(' in resp:
        bad.append('portHumanRespawn(): not through portHumanRestartRecord, or an in-place carRestart / rebind')
    if 'humanCarPlaceAtStart(human)' not in rec or any(r in rec for r in MATCH_RESETS):
        bad.append('portHumanRestartRecord(): no humanCarPlaceAtStart, or a match-wide reset (%s)' %
                   [r for r in MATCH_RESETS if r in rec])
    for b in bad:
        print('FAIL 10 ' + b)
    good = not bad
    ok = ok and good
    print('%s 10 respawn path: %d in-place restarts, all reviewed (%s); restart_pending only cleared / restored; '
          'portHumanRespawn through portHumanRestartRecord (no match-wide reset)' % (
              'ok  ' if good else 'FAIL', sum(restarts.values()), '; '.join(sorted({k[1] for k in restarts}))))
    # negative control: without the define retail's death and won rules are ungated
    neg, nb = round_census(NEGATIVE_DEFINES)
    found = [k for k, (kind, gate) in ROUND_REVIEWED.items() if kind == 'rule' and k in neg]
    good = len(found) == sum(1 for v in ROUND_REVIEWED.values() if v[0] == 'rule') and not any(
        'portHumanRules(' in b for b in nb.values())
    ok = ok and good
    print('%s 10 negative control (no TM2_PORT_HUMANS): retail\'s %d fixed-human round ends, none gated' % (
        'ok  ' if good else 'FAIL', len(found)))
    return ok


def check_build_rules(out):
    import json
    from check_hazards import load_functions, call_graph
    from reloc_image import Link
    out = Path(out)
    pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
    if 'TM2_PORT_HUMANS' not in {d.split('=')[0] for d in pv.get('port_defines') or []}:
        print('     10 %s: built without TM2_PORT_HUMANS: skipped' % out.as_posix())
        return True
    rows, kinds = load_functions(out)
    calls, _, _ = call_graph(Link(out), rows)
    missing = [n for n in H5_PORT if kinds.get(n) != 'port']
    seen, todo = set(), ['portHumanRespawn']
    while todo:
        fn = todo.pop()
        if fn not in seen:
            seen.add(fn)
            todo.extend(calls.get(fn, ()))
    changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
    h5 = {'humanCarDamageUpdate', 'worldUpdate', 'sessionRun', 'carNearestToHuman', 'gameGetLives',
          'hudDrawSplitVertical', 'hudDrawSplitHorizontal'}
    not_changed = sorted(h5 - changed)
    good = not missing and 'carRestart' not in seen and 'humanCarPlaceAtStart' in seen and not not_changed
    print('%s 10 %s: H5 port functions %d/%d%s; portHumanRespawn reaches %d functions, carRestart %s; H5 game '
          'functions changed %d/%d%s' % (
              'ok  ' if good else 'FAIL', out.as_posix(), len(H5_PORT) - len(missing), len(H5_PORT),
              (' (missing %s)' % missing) if missing else '', len(seen),
              'REACHED' if 'carRestart' in seen else 'not reached', len(h5) - len(not_changed), len(h5),
              (' (unchanged %s)' % not_changed) if not_changed else ''))
    return good

# ---- 12. vacant human records and the hand-over (request 50, HUMANS16_PLAN.md 8) ----

HANDOVER_DEFINES = ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16', 'TM2_PORT_HUMANS=16', 'TM2_PORT_CAR_GROUPS',
                    'TM2_PORT_CAR_SWAP']
HANDOVER_DROPIN = HANDOVER_DEFINES + ['TM2_PORT_CAR_DROPIN']
HANDOVER_NEGATIVE = ['TM2_PORT_VARIANT', 'TM2_PORT_CARS=16', 'TM2_PORT_HUMANS=16']   # no TM2_PORT_CAR_SWAP: no gate
HUMAN_LOOP = re.compile(r'\bhumanCarTotal\b|\bhumanCarCount\s*\(\s*\)')
VACANT_TEST = 'portHumanVacant.mask'
_FAR = 'far away (HO_FAR in x, y and z): out of the test'
# (unit, function) -> ('gate', what) = the vacancy test is in the preprocessed body; ('inert', why) = the vacant
# record's state (dead, HP 0, wreck state 4, no damage, no probe, type 0, invisible dummy object, cleared contacts,
# wheels and controls, route -1, remote, far away) makes the loop pass it by. Every function of a built game unit
# that reads humanCarTotal / humanCarCount() is listed (port_variant_arrays.c: VACANT_PORT below).
VACANT_REVIEWED = {
    ('world_update.c', 'worldUpdate'): ('gate', 'not updated: no car update, sound, damage rule, camera, radar'),
    ('arena_setup.c', 'humanCarsRestart'): ('gate', 'not restarted at a round start'),
    ('car_records.c', 'humanCarNearest'): ('gate', 'never the nearest human (AI targets, the boss swoop), and not '
                                                   'the default'),
    ('world_collapse.c', 'worldCollapseUpdate'): ('gate', 'looks at no section (angle test), stands on none'),
    ('screen_card_layout.c', 'sessionBindCars'): ('gate', 'not bound again; the hand-overs\' AI records are'),
    ('world_update.c', 'worldRadarAiAll'): ('inert', 'remote: no radar (TM2_HUMAN_LOCAL < 0)'),
    ('world_update.c', 'worldRadarPlotHumans'): ('inert', 'plotted only with HP > 0; its own radar: remote, none'),
    ('ai_link_node_pick.c', 'weaponInvisibilityUpdate'): ('inert', 'invisible 0'),
    ('ai_steps.c', 'carNearestToHuman'): ('inert', 'through humanCarNearest (gated)'),
    ('area_damage.c', 'areaDamage'): ('inert', 'wreck state 4 and ' + _FAR),
    ('car_physics.c', 'carsPhysicsUpdate'): ('inert', 'probe off (no ground test), contacts cleared (nothing to '
                                                      'combine or answer)'),
    ('car_records.c', 'humanCarCount'): ('inert', 'the count (the records stay: no compaction)'),
    ('car_records.c', 'humanCarFindById'): ('inert', 'every holder of its id was moved to the AI record'),
    ('car_restart.c', 'worldSectionDisable'): ('inert', 'wheels cleared (none moving)'),
    ('fx_debris_splash.c', 'trackVehicleDestroy'): ('inert', 'route -1'),
    ('fx_ground_fires.c', 'groundFiresUpdate'): ('inert', 'type 0'),
    ('global_accessors.c', 'humanCarsSetCount'): ('inert', 'the setter (match start)'),
    ('projectile_hit.c', 'weaponProjectileUpdate'): ('inert', 'type 0'),
    ('screen_card_draw.c', 'weaponSlamFindTarget'): ('inert', _FAR),
    ('target_actor_update.c', 'targetActorsUpdate'): ('inert', _FAR),
    ('track_vehicle_ground.c', 'trackVehicleGround'): ('inert', _FAR),
    ('weapon_leap_homing_init.cpp', 'weaponHomingLeapInit'): ('inert', _FAR),
    ('weapon_ricochet.c', 'weaponRicochetUpdate'): ('inert', 'invisible dummy and dead: skipped'),
    ('weapon_slam_swing.c', 'projectileHitTest'): ('inert', 'invisible dummy and dead: skipped'),
    ('weapon_slot_scan.c', 'weaponLightningScan'): ('inert', _FAR),
    ('weapon_slot_target.c', 'weaponTaserTarget'): ('inert', _FAR),
    ('weapon_targets.c', 'weaponFindTargetAhead'): ('inert', _FAR + ' (8001 ahead; the product stays in 32 bits)'),
    ('weapon_targets.c', 'weaponFindTargetBehind'): ('inert', _FAR + ' (8001 behind)'),
    ('weapon_tornado.c', 'weaponTornadoUpdate'): ('inert', _FAR),
    ('world_object_register.c', 'trackVehiclesShouldStop'): ('inert', 'route -1'),
}
# port functions that count or pick humans: the vacancy test in their body
VACANT_PORT = ['portHumanPresent', 'portHumanVacantCount', 'portHumanFirstPresent', 'portHandOverHumanSlot']
# portHumanVacateRecord: the vacant state (preprocessed, blanks removed)
VACATE_FIELDS = ['hr->state.dead=1;', 'hr->stats.hp=0;', 'hr->stats.wreck_state=4;', 'hr->stats.damageable=0;',
                 'hr->stats.probe_on=0;', 'hr->state.x2C.route=-1;', 'hr->type=0;', 'dummy->visible=0;',
                 'portHumanLocal[h]=-1;', 'portHumanPad[h]=0;', 'humanCarObjects[h]=(int)dummy;',
                 'humanCarObjectSlots[h]=', 'hr->weapons.slam=0;', 'hr->fx=0;', 'portHumanVacant.mask|=1u<<h;']
# what the hand-over / leave / return must not call (a compaction, a restart, a start with rand(), a rebind)
HANDOVER_FORBIDDEN = ['portHumanCompact', 'carRestart', 'aiCarStart', 'carBindAiSlot', 'carBindHumanSlot',
                      'humanCarPlaceAtStart', 'startNodePickRandom', 'rand']
HANDOVER_PORT = ['portHumanHandOver', 'portHumanLeave', 'portHumanReturn', 'portHumanVacantCount',
                 'portHumanFirstPresent', 'portHumanBindAiCount', 'portHumanVacantReset', 'portHumanRejoin',
                 'portHumanFreeGroupOfType']
# request 50b, the joiner in place (portHumanRejoin, preprocessed, blanks removed): what its body must hold, and the
# writes that would renumber humans (humanCarTotal) it must not hold
REJOIN_NEEDS = ['portHumanVacant.mask&=~(1u<<h);', 'portHumanFreeSlot(group)', 'portCarBindSlot(0,h,slot);',
                'portHumanRestartRecord(hr,0);', 'portHumanSetLayout(h,portHumanLayout[h]);']
REJOIN_FREE = ['carSlotKinds[s]!=2', 'carSlotIndices[s]!=-1']     # portHumanFreeSlot: a car nobody drives
REJOIN_RENUMBER = re.compile(r'humanCarTotal(=(?!=)|\+\+|--|\+=|-=)|portHumanCompact\(')
# what the rejoin's call graph must not reach (its start is portHumanRestartRecord's, which draws the heading's one
# rand() as every start: not listed)
REJOIN_FORBIDDEN = ['portHumanCompact', 'carRestart', 'aiCarStart', 'carBindAiSlot', 'carBindHumanSlot']


def rejoin_problems(body, free):
    """What is wrong with a portHumanRejoin body (and its free-slot test body); [] = right."""
    out = ['no `%s`' % n for n in REJOIN_NEEDS if n not in body]
    out += ['free-car test without `%s`' % n for n in REJOIN_FREE if n not in free]
    out += ['renumbers: `%s`' % m.group(0) for m in REJOIN_RENUMBER.finditer(body)]
    return out


def vacant_census(defines, units=None):
    """{(unit, function): body without blanks} of every function of the built units reading humanCarTotal /
    humanCarCount() (port_variant_arrays.c aside), plus sessionBindCars; and port_variant_arrays.c's bodies."""
    from semantic_compile import preprocess as cpp
    found, port = {}, {}
    with tempfile.TemporaryDirectory(dir=ROOT / 'work') as tmp:
        for rel in units or built_units():
            src = ROOT / rel
            raw = src.read_text(encoding='utf-8', errors='replace')
            pva = src.name == 'port_variant_arrays.c'
            if not pva and not HUMAN_LOOP.search(raw) and 'sessionBindCars' not in raw:
                continue
            text = cpp(src, Path(tmp) / (src.stem + '.i'), defines).decode('ascii', errors='replace')
            text = '\n'.join(l for l in text.splitlines() if not l.startswith('#'))
            for fn, b0, b1 in c_bodies(text):
                body = re.sub(r'\s+', '', text[b0:b1])
                if pva:
                    port[fn] = body
                elif HUMAN_LOOP.search(text, b0, b1) or fn == 'sessionBindCars':
                    found[(src.name, fn)] = body
    return found, port


def check_vacant(units=None):
    ok = True
    found, port = vacant_census(HANDOVER_DEFINES, units)
    unreviewed = sorted(k for k in found if k not in VACANT_REVIEWED)
    stale = sorted(k for k in VACANT_REVIEWED if k not in found)
    nogate = sorted(k for k, (kind, _) in VACANT_REVIEWED.items() if kind == 'gate' and k in found
                    and VACANT_TEST not in found[k])
    for k in unreviewed:
        print('FAIL 12 not reviewed: %s %s() reads the human count (a loop over human records)' % k)
    for k in stale:
        print('FAIL 12 reviewed human loop not found: %s %s()' % k)
    for k in nogate:
        print('FAIL 12 %s %s(): no vacancy test (%s)' % (k + (VACANT_TEST,)))
    gates = sum(1 for v in VACANT_REVIEWED.values() if v[0] == 'gate')
    good = not unreviewed and not stale and not nogate
    ok = ok and good
    print('%s 12 vacant human records, TM2_PORT_HUMANS=16 + TM2_PORT_CAR_SWAP: %d game functions over the human '
          'records, all reviewed: %d gated (the vacancy test in the body), %d inert by the vacant state; unreviewed '
          '%d, stale %d, gates missing %d' % ('ok  ' if good else 'FAIL', len(found), gates, len(found) - gates,
                                              len(unreviewed), len(stale), len(nogate)))
    bad = [fn for fn in VACANT_PORT if VACANT_TEST not in port.get(fn, '')]
    vac = port.get('portHumanVacateRecord', '')
    missing = [f for f in VACATE_FIELDS if f not in vac]
    calls = []
    for fn in HANDOVER_PORT:
        body = port.get(fn)
        if body is None:
            calls.append('%s(): not found' % fn)
            continue
        calls += ['%s() calls %s' % (fn, c) for c in HANDOVER_FORBIDDEN if re.search(r'\b%s\(' % c, body)]
    hand = port.get('portHumanHandOver', '')
    for need in ('portCarBindSlot(1,j,slot);', 'portCarApplyType(1,j,hp);', 'aiDifficultySpeedScale()',
                 'portHumanVacateRecord(h);', 'aiRouteStateReset(', 'aiCarTargets[j]=-1;'):
        if need not in hand:
            calls.append('portHumanHandOver(): no %s' % need)
    for p in rejoin_problems(port.get('portHumanRejoin', ''), port.get('portHumanFreeSlot', '')):
        calls.append('portHumanRejoin(): ' + p)
    for b in bad:
        print('FAIL 12 port function %s(): no vacancy test' % b)
    for m in missing:
        print('FAIL 12 portHumanVacateRecord(): no `%s`' % m)
    for c in calls:
        print('FAIL 12 ' + c)
    good = not bad and not missing and not calls
    ok = ok and good
    print('%s 12 port side: %d counting functions test the vacancy, the vacant state has its %d fields, the hand-over, '
          'leave and return bind / retype / vacate without a compaction, restart, rebind or rand() (%d functions)' % (
              'ok  ' if good else 'FAIL', len(VACANT_PORT), len(VACATE_FIELDS), len(HANDOVER_PORT)))
    # with the drop-in too: the same gates, portCarActiveCount and portCarBindCount know the vacancy
    found_d, port_d = vacant_census(HANDOVER_DROPIN, units)
    bad = sorted(k for k, (kind, _) in VACANT_REVIEWED.items() if kind == 'gate' and VACANT_TEST not in
                 found_d.get(k, ''))
    bad += [fn for fn in ('portCarActiveCount', 'portCarBindCount')
            if 'portHumanVacantCount()' not in port_d.get(fn, '') and 'portHumanBindAiCount(' not in
            port_d.get(fn, '')]
    bad += ['portHumanCompact' for _ in [0] if 'portHumanVacant.mask' not in port_d.get('portHumanCompact', '')]
    bad += ['portCarStartHumanAt' for _ in [0] if 'returnportHumanRejoin(index,group,node);' not in
            port_d.get('portCarStartHumanAt', '')]
    good = not bad
    ok = ok and good
    print('%s 12 with TM2_PORT_CAR_DROPIN: the %d gates, portCarActiveCount / portCarBindCount and portHumanCompact '
          'know the vacancy, portCarStartHumanAt takes a vacant index back through portHumanRejoin%s' % ('ok  ' if good else 'FAIL', gates, (': missing %s' % bad) if bad else ''))
    # negative control: without TM2_PORT_CAR_SWAP (no hand-over) the gates are retail's loops (the check above would
    # fail on every one of them) and no port function of the hand-over is built
    neg, port_n = vacant_census(HANDOVER_NEGATIVE, units)
    would_fail = sorted(k for k, (kind, _) in VACANT_REVIEWED.items() if kind == 'gate'
                        and k[1] != 'sessionBindCars' and VACANT_TEST not in neg.get(k, VACANT_TEST))
    leaked = [fn for fn in HANDOVER_PORT if fn in port_n]
    good = len(would_fail) == gates - 1 and not leaked and not any(VACANT_TEST in b for b in neg.values())
    ok = ok and good
    print('%s 12 negative control (TM2_PORT_HUMANS=16 without TM2_PORT_CAR_SWAP): the %d game gates are absent (the '
          'gate test fails on each), no hand-over function is built%s' % (
              'ok  ' if good else 'FAIL', len(would_fail), (': built %s' % leaked) if leaked else ''))
    # negative control 2: humanCarsRestart with its gate removed (a copy under work/) fails the gate test
    neg_dir = ROOT / 'work' / 'check_humans_neg'
    neg_dir.mkdir(parents=True, exist_ok=True)
    text = (ROOT / 'src/semantic/arena_setup.c').read_text(encoding='utf-8', errors='replace')
    cut = re.sub(r'^.*TM2_HUMAN_SKIP_VACANT\(i\);.*\r?\n', '', text, flags=re.M)
    (neg_dir / 'arena_setup.c').write_text(cut, encoding='utf-8', newline='')
    neg2, _ = vacant_census(HANDOVER_DEFINES, ['work/check_humans_neg/arena_setup.c'])
    key = ('arena_setup.c', 'humanCarsRestart')
    good = cut != text and key in neg2 and VACANT_TEST not in neg2[key]
    ok = ok and good
    print('%s 12 negative control: humanCarsRestart without its gate (a copy) fails the gate test' % (
        'ok  ' if good else 'FAIL'))
    # negative control 3 (request 50b): the rejoin test fails on a body that keeps the vacancy, on one that
    # renumbers (humanCarTotal written, as portCarStartHumanAt's new-index path), and on a free test that takes a
    # driven car
    body, free = port.get('portHumanRejoin', ''), port.get('portHumanFreeSlot', '')
    muts = {'vacancy kept': (body.replace('portHumanVacant.mask&=~(1u<<h);', ''), free),
            'renumbering': (body + 'humanCarTotal=h+1;', free),
            'driven car taken': (body, free.replace('carSlotKinds[s]!=2', 'carSlotKinds[s]>2'))}
    caught = [k for k, (b, f) in muts.items() if rejoin_problems(b, f)]
    good = bool(body) and len(caught) == len(muts)
    ok = ok and good
    print('%s 12 negative control: the rejoin test fails on %d/%d broken copies (%s)' % (
        'ok  ' if good else 'FAIL', len(caught), len(muts), ', '.join(caught)))
    return ok


def check_build_vacant(out):
    """Check 12 (build): with TM2_PORT_HUMANS and TM2_PORT_CAR_SWAP the hand-over functions are port functions, the
    gated game functions are changed sections, and no hand-over function reaches rand(), aiCarStart, carRestart,
    portHumanCompact or the start pickers through calls the swap blocks do not make themselves."""
    import json
    from check_hazards import load_functions, call_graph
    from reloc_image import Link
    out = Path(out)
    pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
    names = {d.split('=')[0] for d in pv.get('port_defines') or []}
    if not {'TM2_PORT_HUMANS', 'TM2_PORT_CAR_SWAP'} <= names:
        print('     12 %s: not built with TM2_PORT_HUMANS and TM2_PORT_CAR_SWAP: skipped' % out.as_posix())
        return True
    rows, kinds = load_functions(out)
    calls, _, _ = call_graph(Link(out), rows)
    missing = [n for n in HANDOVER_PORT if kinds.get(n) != 'port']
    changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
    gated = {k[1] for k, v in VACANT_REVIEWED.items() if v[0] == 'gate'}
    unchanged = sorted(gated - changed)
    # the swap blocks' own reach (portCarResetSpecials ends specials as the game does: carSpinOutById and the
    # animation steps may draw rand(), as at the same events in the game) is allowed; the rest must not get there
    allowed = {'portCarResetSpecials', 'portCarResetLook',
               # the game entry points check_car_groups.py check 5 allows the swap (a held car let go spins out, an
               # animation steps: a leap's turbo flame off through animSetStep), as at the same events in the game
               'carSpinOutById', 'objRegistrySetStep', 'carFreezeSetState', 'animSetStep'}
    reached = set()
    for first in ('portHumanHandOver', 'portHumanLeave', 'portHumanReturn'):
        seen, todo = set(), [first]
        while todo:
            fn = todo.pop()
            if fn in seen or fn in allowed:
                continue
            seen.add(fn)
            todo.extend(calls.get(fn, ()))
        reached |= {f for f in HANDOVER_FORBIDDEN if f in seen}
    # the rejoin: a start through portHumanRestartRecord (humanCarPlaceAtStart), no compaction, rebind or restart
    seen, todo = set(), ['portHumanRejoin']
    while todo:
        fn = todo.pop()
        if fn in seen or fn in allowed:
            continue
        seen.add(fn)
        todo.extend(calls.get(fn, ()))
    reached |= {f for f in REJOIN_FORBIDDEN if f in seen}
    if 'humanCarPlaceAtStart' not in seen:
        reached.add('(rejoin without humanCarPlaceAtStart)')
    good = not missing and not unchanged and not reached
    print('%s 12 %s: hand-over functions %d/%d port functions%s; gated game functions changed %d/%d%s; forbidden '
          'callees reached %s' % ('ok  ' if good else 'FAIL', out.as_posix(), len(HANDOVER_PORT) - len(missing),
                                  len(HANDOVER_PORT), (' (missing %s)' % missing) if missing else '',
                                  len(gated) - len(unchanged), len(gated),
                                  (' (unchanged %s)' % unchanged) if unchanged else '', sorted(reached) or 'none'))
    return good


# check 13 (request 66, ANALOG_STEERING.md 2.6): the turn's sign kept in reverse
STEER_NATIVE = {'carThrottleSteer', 'portHumanSteerKeep', 'mathBlendAngle'}
STEER_CALLERS = {'carThrottleSteer', 'carRicochetOffObject'}
H_STATE, H_INPUT, H_STATS, H_MOTION = 0x9C, 0xD0, 0x14C, 0x6DC


def steer_run(out, keep_bits):
    """carThrottleSteer (human = 1) on human record 0 in check_custom's FarMips, every other callee a stub returning 0
    (timerGetFrameTicks 3): the yaw rate for each (speed, steer) of a sweep, portHumanControl[0] = keep_bits."""
    from check_rollback import Image
    from check_custom import FarMips
    img = Image(out)
    fm = FarMips(img)
    m = fm.m
    link = img.link
    sym = lambda n: link.symbols[n]['address']
    rec = sym('humanCars')
    ctl = sym('portHumanControl')
    entry = {img.sizes[n][0]: n for n in img.sizes}
    calls = []

    def run(fn, args, steps=400000):
        r = m.r
        for i, v in enumerate(args):
            r[4 + i] = v & 0xFFFFFFFF
        r[29], r[31] = 0x801FF000, m.RET
        r[28] = link.gp_c
        pc, delay = img.sizes[fn][0], None
        for _ in range(steps):
            if pc == m.RET:
                return r[2]
            nxt = fm.exec_(m, pc, link.word(pc))
            if delay is not None:
                pc, delay = delay, None
                name = entry.get(pc)
                if name is not None and name not in STEER_NATIVE:
                    calls.append(name)
                    r[2] = 3 if name == 'timerGetFrameTicks' else 0
                    pc = r[31]
                continue
            if nxt is None:
                pc += 4
            else:
                delay, pc = nxt, pc + 4
        raise RuntimeError('%s: no return' % fn)

    out_rows = []
    for steer in (300, -300):
        for speed in list(range(-7000, 7001, 500)) + [-421, -419, 419, 421]:
            m.fill(rec, bytes(0x814))
            m.wr(ctl, 1, keep_bits)
            w = lambda off, n, v: m.wr(rec + off, n, v & ((1 << (8 * n)) - 1))
            w(H_STATS + 0x4C, 2, 80)        # turn_speed
            w(H_STATS + 0x60, 2, 40)        # steer_rate_high
            w(H_STATS + 0x62, 2, 60)        # steer_rate_low
            w(H_STATS + 0x64, 2, 300)       # steer_limit
            w(H_STATS + 0x6E, 2, 2)         # handbrake_lock
            w(H_STATS + 0xB0, 4, 100)       # accel_top
            w(H_STATS + 0xB4, 4, 420)       # accel_launch
            w(H_STATS + 0xB8, 4, 9000)      # speed_cap
            w(H_STATS + 0xC8, 4, 6000)      # reverse_cap
            w(H_STATS + 0xCC, 4, 200)       # brake_decel
            w(H_STATE + 0x06, 1, 1)         # f06: a direction held
            w(H_INPUT + (0x10 if steer > 0 else 0x0F), 1, 1)    # steer_inc / steer_dec
            w(H_MOTION + 0x00, 2, steer)
            w(H_MOTION + 0x0A, 2, speed)
            run('carThrottleSteer', (rec, 1))
            y = m.rd(rec + H_MOTION + 0x14, 2)
            out_rows.append((steer, speed, y - 0x10000 if y & 0x8000 else y))
    return out_rows, calls


def check_build_steer(out):
    """Check 13 (build): portHumanSteerKeep is a port function called only by carThrottleSteer and
    carRicochetOffObject, calls nothing; carThrottleSteer run on the MIPS interpreter: with the bit clear the rolling
    yaw rate flips with the speed's sign (retail), with portHumanControl bit 0 it has the steering's sign at every
    speed, the same size as retail's, and the turn in place (|speed| <= accel_launch) is retail's either way; without
    TM2_PORT_HUMANS (the negative control) the bit changes nothing: the run must see retail's flip."""
    import json
    from check_hazards import load_functions, call_graph
    from reloc_image import Link
    out = Path(out)
    pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
    names = {d.split('=')[0] for d in pv.get('port_defines') or []}
    humans = 'TM2_PORT_HUMANS' in names
    link = Link(out)
    if 'portHumanControl' not in link.symbols or 'humanCars' not in link.symbols:
        print('FAIL 13 %s: portHumanControl / humanCars not in the link' % out.as_posix())
        return False
    rows, kinds = load_functions(out)
    calls, callers, pointed = call_graph(link, rows)
    bad = []
    if humans:
        if kinds.get('portHumanSteerKeep') != 'port':
            bad.append('portHumanSteerKeep not a port function')
        if callers.get('portHumanSteerKeep', set()) != STEER_CALLERS:
            bad.append('portHumanSteerKeep called by %s' % sorted(callers.get('portHumanSteerKeep', set())))
        if calls.get('portHumanSteerKeep'):
            bad.append('portHumanSteerKeep calls %s' % sorted(calls['portHumanSteerKeep']))
        if 'portHumanSteerKeep' in pointed:
            bad.append('portHumanSteerKeep pointed to')
    clear, c0 = steer_run(out, 0)
    keep, c1 = steer_run(out, 1)
    if 'rand' in c0 + c1:
        bad.append('carThrottleSteer called rand() in the sweep')
    flips = 0
    for (s, v, y0), (_, _, y1) in zip(clear, keep):
        retail = (v // 32 if v >= 0 else -((-v) // 32)) * s
        retail = int(retail / 80) if abs(v) // 32 < 80 else (s if v >= 0 else -s)
        if abs(v) > 420:
            if y0 != retail:
                bad.append('bit clear, steer %d speed %d: yaw %d (want retail %d)' % (s, v, y0, retail))
            want1 = abs(retail) * (1 if s > 0 else -1) if humans else retail
            if y1 != want1:
                bad.append('bit set, steer %d speed %d: yaw %d (want %d)' % (s, v, y1, want1))
            if v < 0 and y0 != 0:
                flips += (y0 > 0) != (s > 0)
        else:
            if y0 != y1 or (y0 != 0 and (y0 > 0) != (s > 0)):
                bad.append('in place, steer %d speed %d: yaw %d / %d (want the steering\'s sign, both alike)' % (
                    s, v, y0, y1))
    if not flips:
        bad.append('the bit-clear sweep shows no flip in reverse (the run is blind)')
    good = not bad
    print('%s 13 %s: carThrottleSteer sweep %d speeds x 2 steers, retail flips %d in reverse; with bit 0 %s%s' % (
        'ok  ' if good else 'FAIL', out.as_posix(), len(clear) // 2, flips,
        'the steering\'s sign at every speed' if humans else 'unchanged (no TM2_PORT_HUMANS: retail)',
        '' if good else ': ' + '; '.join(bad[:6])))
    return good


def main(argv):
    ok = check_macros()
    ok = check_sources() and ok
    ok = check_map() and ok
    ok = check_pads() and ok
    ok = check_feedback() and ok
    ok = check_storage() and ok
    ok = check_rules() and ok
    ok = check_compact() and ok
    ok = check_vacant() and ok
    for out in argv:
        ok = check_build(out) and ok
        ok = check_build_map(out) and ok
        ok = check_build_storage(out) and ok
        ok = check_build_rules(out) and ok
        ok = check_build_compact(out) and ok
        ok = check_build_vacant(out) and ok
        ok = check_build_steer(out) and ok
    print('HUMANS CHECK %s' % ('OK' if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
