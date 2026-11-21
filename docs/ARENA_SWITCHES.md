# Arena hazards and destructibles: off switches

PC port request 38 (2026-10-14). The port's custom battle game types have two switches, **Arena hazards** (the
automatic ones) and **Arena destructibles** (the ones players set off), both on by default. "Off" means the arena
plays as if the thing never happens, consistently: no invisible wall left, no hole without its effect, AI routes as
the arena is. The events patch sections, surfaces and AI routes and draw from `rand()`, so the switches are gates
inside the retail functions, under `--port-define TM2_PORT_HAZARDS`.

Sources: the C in `src/semantic`, `GAME_EVENTS.md` 1.4 / 9.4 / 14 / 15, `PICKUP_LOOKS.md` 1 and 5, `RAND_CALLERS.md`,
the USA disc (`DMD\*.DMD` object ids and select-node children, `TERRAIN\*.GRP` section tables, read with
`tools/dmd_lod_dump.py`). Addresses are retail. Tags: **CONFIRMED** = read in the code or the disc data; **LIKELY**
= follows from them, not seen running. Nothing was run in an emulator.

Level ids (`gameGetLevel`): 1 Los Angeles, 2 Moscow, 3 Paris, 4 Amazonia, 5 New York, 6 Antarctica, 7 Holland,
8 Hong Kong, 10 Suicide Swamp, 11 Cyburbia, 12 Roof Tops.

Contents: 1 short answer, 2 census, 3 what "off" does, 4 as built, 5 pickups, 6 `rand()`, 7 retail bugs found,
8 per-arena switches, 9 verification, 10 questions for the port.

## 1. Short answer

- **Hazards (bit 0), 4 items:** Antarctica's collapsing ice, lava burn (ground class 2: Amazonia and one Los
  Angeles section), the fire volumes (material 666: Los Angeles 2, Moscow 3 after its bridge events) and Hong
  Kong's two trains.
- **Destructibles (bit 1), 8 items:** the Los Angeles quake and timed target 508, Moscow's four bridge events and
  its dish, the Eiffel Tower (and its "fewer than 3 AI cars" route patch), New York's 519 -> 509 event, Holland's
  two barns (target 510) and the Hong Kong trains as targets 506 / 507.
- **Corrections to the port's list:** the "hazard object kinds 100 / 101" of `carContactResolve` are the trains
  (world object id 100 + i). Moscow's events and Los Angeles' events are **set off by remote bombs**
  (destructibles, not hazards; only LA event 1, the quake, exists on the disc). Drowning (Hong Kong's harbour) is
  a map edge like a fall: not switched (Q2). The Antarctica Santa is a pedestrian (516) that respawns
  (`SANTA.md`): not a destructible. The ordinary breakable props (targets 500..505, 520, on every arena) stay
  breakable, by the user's decision (Q1, 2026-10-14); item `PROPS` stays in the per-item mask but no menu offers it.
- **Trains off: removed**, not parked inside the arena (section 3).
- **11 gates + the latch**, each one `#ifdef TM2_PORT_HAZARDS` call of a `.port_text` helper; every gate sits
  before the first `rand()` of its event. With both words zero, retail's logic and `rand()` order
  (`tools/check_hazards.py`).

## 2. Census

### 2.1 Where each arena's code runs (CONFIRMED)

| Per frame | Level | Call site |
|---|---|---|
| `moscowEventsUpdate` 800D4D58, `moscowDishUpdate` 800D6188 | 2 | `rtMainLoop` (`rt_main.c`) |
| `laEventsUpdate` 800D5AF0, `targetTimedEventUpdate` 801274C0 | 1 | `rtMainLoop` |
| `eiffelTowerUpdate` 800D6800 | 3 | `rtMainLoop` |
| `targetLevelEventUpdate` 801277A8 (New York) | 5 | `rtMainLoop` |
| `targetPlayerEventsUpdate` 80127CE0 (Holland) | 7 | `rtMainLoop` |
| `worldTrackVehiclesUpdate` 80117B40 | 8 | `worldUpdate` 8011A700, first |
| `worldCollapseUpdate` 801163B8 | 6 | `worldUpdate`, last |
| `carGroundEffects` 800ED128 (lava, water) | all | per car (`car_wheels.c`) |

Remote-bomb hooks (detonation in `weaponDropWrite`, a car driving into a bomb in `weaponDropCheckHit`):
`moscowEventsTrigger` 800D4504 (level 2), `laEventsTrigger` 800D5584 (level 1), `eiffelTowerBombCheck` 800D6698
(level 3). Arena load (`arenaSetup` 8011995C, only when the arena changes) and `uaInit` reset the events; nothing
re-arms them at `roundStart` 8011A024: **a destroyed arena stays destroyed until it is loaded again**.

### 2.2 The census table

H = hazard (bit 0), D = destructible (bit 1). "Item" is the bit number in `portArenaItems` / `portArenaLive`
(`include/semantic/port_hazards.h`).

| Item | What | Level | Trigger | Effects | Gate |
|---|---|---|---|---|---|
| 0 `ICE` (H) | collapsing ice | 6 | `worldCollapseUpdate` 801163B8: timer from -720; after 361 ticks the first active section of the stage a **human** has all four wheels on, after 781 one a human faces, after 1081 also `(rand() & 3) == 1`. AI cars never trigger it | route nodes of the section removed (`aiRouteRemoveSectionNodes`), AI cars on it re-route, neighbour links cut; 480 ticks of rumble (sound 31), shaking, explosions and debris; then the platform falls: object off, section and linked sections at height -80, no floor; players on it fall into the sea | G1 |
| 1 `LAVA` (H) | lava burn | 4, 1 (section 14) | `carGroundEffects`: all four wheels on one ground-class-2 section, not on a moving surface, wheel 0 depth >= 8 | splash (`fxLavaSplashAt`, object 691, sound, pad), wheel effects, fire timer 0; every 61 ticks **HP -= 3** if the car burns (human: damageable; AI: +0x3A, never type 0xE0) | G2 |
| 2 `FIRE` (H) | fire volumes, material 666 | 1 (2 static), 2 (3, in the bridge events' after state) | `carContactApplyObject` 800E06CC: any car touching a 666 volume, not burning, no energy shield, not on turbo >= 50 | `burning = 1`, `fxFlameBurning`; burning costs humans 1 HP with `(rand() & 3) == 0` per frame down to 5 HP, ends after 100 frames (`groundFiresUpdate`) | G3 |
| 3 `TRAINS` (H) | the two trains | 8 | always: `worldTrackVehiclesUpdate` moves both round an oval every frame | drivable roof surfaces (the last 2 records of HKONG.RMP) carry riders; contact (`carContactResolve` 800E0C28, object id 100 / 101): spin-outs, speed set, damage to humans only; trains hidden outside the station window | G4, G6 (+ G5 for 506/507) |
| 8 `LA_QUAKE` (D) | Los Angeles quake, event 1 | 1 | a remote bomb on section 6 or within 800 of the lane anchors: `laEventStart` 800D58BC, once | **`worldSectionDisable(6)`** at once (height 32000, surfaces reset, route nodes removed, no floor, humans on it spin out); then lanes: debris, sounds 4 / 19, explosions, flash; `objRegistrySetStep(4005, 1, 1)`: **pickup 403 appears** | G7 |
| 9 `LA_TIMED` (D) | timed target 508 | 1 | 508 destroyed (12 HP; `targetDamage` 80125118) | `targetTimedEventUpdate`: sparks, flash at 50, debris, 508 to its after state (no hit volume), sounds at 110. No sections, no pickups | G5 |
| 10 `MOSCOW_BRIDGES` (D) | four bridge events (4000 #1..#4) | 2 | a remote bomb on the event's section or within 800 of one of its 4 lanes: `moscowEventStart` 800D4AAC, once per event | **`worldSectionDisable(section)`** at once; `moscowEventsUpdate`: lane debris, sounds, flash; after state: **fire volumes 666** (#1..#3), **pickups 430, 403, 404** | G8 |
| 11 `MOSCOW_DISH` (D) | the dish (3005 / 3006) | 2 | the first weapon hit on 3005 / 3006 (`weaponApplyHit` -> `moscowDishDamage` 800D6140 -> `moscowDishStartDestroy` 800D5F94) | `moscowDishUpdate`: sparks on dish and satellites, debris, dish hidden, flash; `objRegistrySetStep(3005, 0, 1)`: the dish's 3 hit volumes and the **440 bonus carrier** vanish; **lightning is then denied to humans on Moscow** (`weapon_select.c`). No sections | G9 |
| 12 `EIFFEL` (D) | the Eiffel Tower | 3 | a remote bomb within 1200 (x and y) of (0, -6144): `eiffelTowerStartFall` 800D6710, once (`GAME_EVENTS.md` 15) | `eiffelTowerUpdate`: tilt, sparks, debris; when down `eiffelTowerSetFallenSurfaces` (sections 13 / 15 get the fallen deck) and `aiRoutePatchEiffelTower` 800F603C (7 nodes get a second successor onto nodes 107..115 on the deck), flash, sounds. **Also** `aiCarDamageUpdate` 8011BBD4 calls `aiRoutePatchEiffelTower` every frame on level 3 while fewer than 3 AI cars remain, standing tower or not (routes only: AI then heads for the floorless section 15) | G10, G11 |
| 13 `NY_EVENT` (D) | New York 519 -> 509 | 5 | 519 (inside prop 505's after state) destroyed, then 509 (inside 519's) | `targetLevelEventUpdate`: sparks, flash, explosions, 519 / 509 to their after states (509's 440 bonus spot goes). No sections | G5 |
| 14 `HOLLAND_BARNS` (D) | the two barns (510 #1 / #2) | 7 | 510 destroyed | at once `worldSectionDisable(2)` + `(4)` (barn 1) or `(0)` + `(5)` (barn 2); `targetPlayerEventsUpdate`: debris, flash, sounds; the barn's intact state goes with its **teleporter (695 / 694)**, the 3600 animation and **6 pickups** (411, 403, 403 / 417, 413, 404) | G5 |
| 15 `TRAIN_TARGETS` (D) | trains as targets 506 / 507 | 8 | 25 damage (weapons, ramming, splash, Dark Tooth contact) | explosions, `trackVehicleDestroy` 80118F38 (hidden, riders spin out, roof surface to (24000, 24000)); the next update re-places it at the start of its oval. Nothing permanent | G5 |
| 16 `PROPS` | breakable props 500..505, 520 | all | destroyed by weapons, ramming, touch (500 / 520), splash | after state, often revealing pickups (`PICKUP_LOOKS.md` 5). No sections, routes or `rand()` of their own | G5 (not offered by the menus, by decision: Q1) |

Not switched, so the census is complete: deep water (Hong Kong section 23, the harbour: a map edge, Q2), water
(splash, puts fire out), New York section 7 (a fountain), falls, teleporters (warps 694..697, every level; Holland's
two vanish with the barns), material 3600 / 3002 / 3301, Swamp's 600 volumes, Paris 7777 (never switched: unused),
the flame markers 5100 (cosmetic), pedestrians and the Santa, the Dark Tooth boss markers.

### 2.3 Disc facts (CONFIRMED)

- Ground class (GRP +0x125E) 2 = lava: Amazonia sections 2-5, 7, 8, 11, 12, 16-19, 21, 22; **Los Angeles section
  14** (a 768 x 576 box at (2304, -5664), floor flag set). Moscow's GRP sets class 2 on index 14, past its 12
  sections: unused. What LA section 14 looks like in game is for the port to see (section 9.3).
- FREEWAY has one quake lane node (4006, lane index 2): see section 7.
- DISH: four events with all four lanes each; the fire volumes and pickups under the events' after states.
- HKONG: the trains are the target nodes 506 / 507 (children 3501 / 3502), attached by `worldLoadObject` as scene
  objects (`dbInitCsForAModel`, `worldTrackVehicleAttach`: object id 100 + i).

## 3. What "off" does

| Item | Off |
|---|---|
| `ICE` | `worldCollapseUpdate` never runs: the timer stays, no section is chosen, routes, objects and sections stay as loaded. Kept: the 2-player file's layout (`worldCollapseClearDone` disables sections 0..5 at load in modes 2 / 4, GLACIER2 has only 19 of the 25 floes: that is the file, not the hazard; Q4) |
| `LAVA` | only `st->hp -= 3` is skipped: splash, wheel effects, sounds and the fire-timer reset stay (the look). No `rand()` on that line |
| `FIRE` | the 666 contact is consumed as retail (`ret = 1`, `active = 0`, so it is not tested again each frame) but sets nothing alight. The volume is no wall in retail either, so nothing invisible is left |
| `TRAINS` | **removed**: every frame on level 8, in place of `worldTrackVehiclesUpdate`, `portArenaTrainsPark` hides both trains (object +8 = 0: not drawn, and the car and weapon hit passes skip an object whose +8 & +0xC is 0, `hdpPassRun` 8014FC50..8014FC64), moves them to (24000, 24000, -64), sets busy (retail's destroyed state) and turn / move 0 (no carry), parks their roof surface records at (24000, 24000) with size 0 (as `trackVehicleDestroy`) and their targets' positions with them (splash cannot reach them); targets 506 / 507 take no damage. Why not parked inside the arena: a parked train keeps a live roof surface, a hit volume and a target, and the station-window logic would show it again. AI routes never knew the trains, so nothing else changes. G6 (a train contact treated as a plain object) is unreachable while they are hidden; it stays as a guard for a port whose hit pass would not test +8 |
| `LA_QUAKE` | `laEventStart` returns before its `worldSectionDisable(6)` and its `rand()` calls; `laEventsUpdate` then never runs a lane |
| `LA_TIMED`, `NY_EVENT`, `HOLLAND_BARNS`, `TRAIN_TARGETS`, (`PROPS`) | `targetDamage` returns 0 at its entry for that target: no counter change, no effect, no `rand()`. Weapons still stop on it (`weaponApplyHit` still reports a hit, as on a wall, which it really is). No caller reads `targetDamage`'s result |
| `MOSCOW_BRIDGES` | `moscowEventStart` returns before its `rand()` and its `worldSectionDisable` |
| `MOSCOW_DISH` | `moscowDishDamage` returns before the countdown: the dish stays, its hit volumes and the 440 carrier stay, lightning stays allowed on Moscow |
| `EIFFEL` | `eiffelTowerStartFall` returns (both bomb sites), and `aiCarDamageUpdate` skips the "fewer than 3 AI cars" `aiRoutePatchEiffelTower`: routes stay the standing tower's. With the item on (retail), retail's rule stays, standing tower or not (Q5) |

Hazards bit only: Moscow's events still run, but their fire volumes no longer burn. Destructibles bit only: the
trains run and hit as retail but cannot be destroyed.

## 4. As built

### 4.1 Storage (`.port_bss`, every variant; `config/US/port_variant_arrays.tsv`, after `portDtPlayer`)

| Name | Type | Written by | Meaning |
|---|---|---|---|
| `portArenaOptions` | `int` | the port | bit 0 hazards off (items 0..3), bit 1 destructibles off (items 8..15); 0 = retail |
| `portArenaItems` | `unsigned int` | the port | more items off, bit = item number (section 2.2; `TM2_ARENA_BIT(item)`); 0 = none. Item 16 `PROPS` only here: not offered by the menus, by decision (Q1) |
| `portArenaLive` | `unsigned int` | the C | the items off this round: `portArenaItems`, plus 0x0000000F with bit 0, plus 0x0000FF00 with bit 1; copied by `roundStart` (before its `srand`). Every gate reads only this word, so the switches cannot change mid-round |

The port should change the two words only at match setup: a destroyed arena stays destroyed until it is loaded again
(Q6). Item numbers: 0 `ICE`, 1 `LAVA`, 2 `FIRE`, 3 `TRAINS`, 8 `LA_QUAKE`, 9 `LA_TIMED`, 10 `MOSCOW_BRIDGES`,
11 `MOSCOW_DISH`, 12 `EIFFEL`, 13 `NY_EVENT`, 14 `HOLLAND_BARNS`, 15 `TRAIN_TARGETS`, 16 `PROPS` (4..7 and 17..31
free).

### 4.2 Port functions (`.port_text`, `port_variant_arrays.c`; none calls `rand()`)

| Function | Called by | Does |
|---|---|---|
| `portArenaLatch()` | `roundStart` | `portArenaLive` from the two words |
| `portArenaOff(item)` | the gates | `(portArenaLive >> item) & 1` |
| `portArenaTargetImmune(type)` | `targetDamage` | 1 when the target is switched off: 500..505 / 520 `PROPS`; 506 / 507 on level 8 `TRAINS` or `TRAIN_TARGETS`; 508 on level 1 `LA_TIMED`; 509 / 519 on level 5 `NY_EVENT`; 510 on level 7 `HOLLAND_BARNS` |
| `portArenaTrainsPark()` | `worldUpdate` (level 8) | with `TRAINS` off removes both trains (section 3) and returns 1; else 0 (retail update) |

### 4.3 Gates (12 game functions change)

| # | Function (retail VA) | File | Change under `TM2_PORT_HAZARDS` |
|---|---|---|---|
| G0 | `roundStart` 8011A024 | `scene_init.c` | first statement `portArenaLatch()` |
| G1 | `worldUpdate` 8011A700 | `world_update.c` | `worldCollapseUpdate()` only if `!portArenaOff(ICE)` |
| G2 | `carGroundEffects` 800ED128 | `car_wheels.c` | `st->hp -= 3` only if `!portArenaOff(LAVA)` |
| G3 | `carContactApplyObject` 800E06CC | `car_object_contact.c` | the 666 ignition only if `!portArenaOff(FIRE)` |
| G4 | `worldUpdate` 8011A700 | `world_update.c` | `worldTrackVehiclesUpdate()` only if `!portArenaTrainsPark()` |
| G5 | `targetDamage` 80125118 | `target_executor.c` | entry: `if (portArenaTargetImmune(type)) return 0;` |
| G6 | `carContactResolve` 800E0C28 | `car_contact_resolve.c` | the train branch (object id 100 / 101) only if `!portArenaOff(TRAINS)`; else the plain-object arm |
| G7 | `laEventStart` 800D58BC | `weapon_missile_trails.c` | entry: return if `portArenaOff(LA_QUAKE)` |
| G8 | `moscowEventStart` 800D4AAC | `weapon_bullets.c` | entry: return if `portArenaOff(MOSCOW_BRIDGES)` |
| G9 | `moscowDishDamage` 800D6140 | `arena_load_register.c` | entry: return if `portArenaOff(MOSCOW_DISH)` (its only path to `moscowDishStartDestroy`) |
| G10 | `eiffelTowerStartFall` 800D6710 | `weapon_fire_specials.c` | entry: return if `portArenaOff(EIFFEL)` |
| G11 | `aiCarDamageUpdate` 8011BBD4 | `world_collapse_warps.c` | the AI-count `aiRoutePatchEiffelTower()` only if `!portArenaOff(EIFFEL)` |
| - | `laEventsUpdate` 800D5AF0 | `car_destroyed.c` | the lane-object store skipped for a null object only (section 7, not a switch) |

Each gate is a call and a branch, with no new locals (frame sizes of the big functions unchanged in shape). The
retail units without the define are unchanged: the retail builds stay byte-identical.

## 5. Pickups that depend on an event

| Event | Appear only after it | Vanish with it |
|---|---|---|
| LA quake | 403 (1) | - |
| Moscow bridges | 430, 403, 404 (and the 3 fire volumes) | - |
| Moscow dish | - | the 440 carrier with 443, 444, 446, 457, 459 (unregistered bonus spots); lightning use on Moscow |
| Holland barns | - | 411, 403, 403, 417, 413, 404 + teleporters 694 / 695 |
| New York 509 | (its 440 spot appears when 505 / 519 break) | the 440 spot |
| props | many (`PICKUP_LOOKS.md` 5) | - |

Chosen: **as on the intact arena**. With an item off the after-event pickups never show; they stay registered and
hidden, as retail already has them while the object is whole, so the respawn pick is unchanged. Moving them into
the open would change the arena.

## 6. `rand()` with a bit set (the stream differs only here)

| Off | Calls that no longer happen |
|---|---|
| `ICE` | `worldCollapseUpdate` (5 sites), `worldCollapseExplosions` (12), `worldCollapseDebris` (7), their emitters and `fxSpawnPooledBlast`; the AI re-route picks (`aiCarsRerouteFromSection` -> `aiChooseNextNode` / `aiUpdateStuck`); `carSpinOut` of humans on a falling section |
| `LAVA` | none (only the HP line is skipped) |
| `FIRE` | the burn ticks: `groundFiresUpdate`'s `rand() & 3` per burning human per frame (2 sites), the flame emitters |
| `TRAINS` | `carContactResolve` `rand() % 682` (2 sites) per train contact; `targetDamage` for 506 / 507 (3x `rand() & 1`, jitter, `fxImpactDebris`); `carSpinOut` in `trackVehicleDestroy` |
| `LA_QUAKE` | `laEventStart` (6), `laEventsUpdate` (3) + `mathJitterPos` + emitters + `debrisBurstAt` (`debrisStartBurst`), spin-outs on section 6 |
| `LA_TIMED`, `NY_EVENT`, `HOLLAND_BARNS`, `TRAIN_TARGETS` | `targetDamage`'s effects (`mathJitterPos`, emitters, `rand() & 1` for 505..507); `targetTimedEventUpdate` / `targetLevelEventUpdate` / `targetPlayerEventsUpdate` (2 each) + jitter + emitters + debris; Holland's spin-outs |
| `MOSCOW_BRIDGES` | `moscowEventStart` (6), `moscowEventsUpdate` (6) + jitter + emitters + debris, spin-outs |
| `MOSCOW_DISH` | `moscowDishStartDestroy` (10), `moscowDishUpdate` (4) + jitter + emitters + debris (lightning use stays allowed and draws as usual) |
| `EIFFEL` | `eiffelTowerStartFall` (1), `eiffelTowerUpdate` (4) + jitter + emitters + 3 `debrisBurstAt`; G11 draws none itself, but the AI's node choices differ afterwards |
| `PROPS` | `targetDamage`'s effects for the props |

Unchanged: everything at load (`targetTimedEventInit`, `targetPlayerEventInit` draw 6 each), `roundStart`'s `srand`.
The gates and the port functions draw nothing: with both words zero the stream is retail's. Many listed sites are
VISUAL in `RAND_CALLERS.md`; if the port moves VISUAL sites to a local stream, only the GAME_STATE ones above change
the shared stream. Online, both machines must use the same words (part of the ruleset hash).

## 7. Retail bugs found (`RETAIL_BUGS.md` 18 and 19)

- **LA quake lane 1 has no object** (`RETAIL_BUGS.md` 18): FREEWAY has one lane node (index 2), so lane 1's
  pointer stays 0 and `laEventsUpdate` writes a 0 byte to address 0x12 (`sb zero,18(v0)` at 800D5CAC) each time
  the quake runs; lane 1's effects use the anchor (0, 0, 0). With `TM2_PORT_HAZARDS` the store is skipped for a
  null object only, whatever the switches: every other case writes as retail, no `rand()` changes.
- **`arenaIsHolland()` is always 0** (`RETAIL_BUGS.md` 19): `arenaSetup` clears the flag right after
  `arenaLoadHolland` sets it, so `aiShouldChaseDirect`'s Holland branch never runs. Kept (retail AI).
- `trackVehiclesShouldStop` (80118B20) decides the trains' visibility, not a stop: they never stop. The "sound
  506 / 507" of `track_vehicles.c`'s header is the target position update (`targetSetTrackVehiclePos`); the update
  plays no sound.

## 8. Per-arena switches later

Each item lives on one level except `FIRE` (Los Angeles + Moscow), `LAVA` (Amazonia + LA section 14) and `PROPS`.
A per-arena menu sets bits of `portArenaItems` (and leaves `portArenaOptions` 0); no gate moves. Recipe:

1. Pick the item (section 2.2) and set `portArenaItems |= 1u << item` at match setup (before the round starts).
2. For an item on more than one level that should be split (e.g. LA's lava apart from Amazonia's), take a free item
   number (4..7 for a hazard, 17..31 otherwise), add it to `port_hazards.h`, and make the gate ask for it by level
   (the gate's call is `portArenaOff(item)`; a helper `portArenaOffHere(a, b)` choosing by `gameGetLevel()` keeps
   it one call). Add the new number to the latch masks if a menu bit should include it.
3. A new event (an item not in the census) needs a new gate: add it to `tools/check_hazards.py` `GATES` (and its
   start function to `CALLERS`) so the check proves the gate sits before the event's `rand()`.

Worth their own switch: Antarctica's ice (changes the arena the most), Amazonia's lava, the Hong Kong trains apart
from their destruction (`TRAINS` vs `TRAIN_TARGETS`), the Paris tower, the Moscow dish (it also controls lightning on
Moscow), Moscow's bridges, the LA quake, Holland's barns (they also remove two teleporters and 6 pickups), New York's
event. Not props (all arenas): they stay breakable by decision (Q1). Deep water would need a way out of the harbour, not just a gate (Q2).

## 9. Verification

### 9.1 Static (done)

- Retail builds byte-identical: hybrid `9219f7ba…`, source-only `d755c86e…`, source-only with the SDK from `.LIB`
  `f557c1e3…` (full default build, section 9.4).
- Port variant proofs (`tools/verify_reloc.py` vs E0) and the placement check (limits forced to 2) pass with no
  define, `TM2_PORT_HAZARDS` alone, the car stack + `TM2_PORT_HAZARDS`, and every port define +
  `TM2_PORT_HAZARDS` (section 9.4).
- `tools/check_hazards.py OUT --expect-functions` on each build with the define: the 4 port functions in
  `.port_text` reach no `rand()`; every gate present, dominating its event's protected sites (calls, `rand()`,
  the lava HP store) and cutting them on its off edge while its on edge still reaches them; the latch dominates
  `srand`; the events' start functions have only the gated callers and are never pointed to; the changed game
  functions are exactly the 12 above (with `TM2_PORT_HAZARDS` alone). `--negative-control` on a build without the
  define fails as it must.
- Code reading: `hdpPassRun` (8014FAB0) skips scene objects with `(+8 & +0xC) == 0` for every car (humans and AI)
  and the weapons' pass does the same (`weapons.h`): a hidden train cannot be hit (Q8 of the plan, resolved).
  No caller of `targetDamage` reads its result.

### 9.2 Proof results

Car stack = `TM2_PORT_CAR_GROUPS TM2_PORT_CARS=16 TM2_PORT_CAR_SWAP TM2_PORT_TEXTURE_BANKS TM2_PORT_CAR_SPECIALS
TM2_PORT_DARK_TOOTH TM2_PORT_CAR_SOUND TM2_PORT_CAR_CLUTS TM2_PORT_CAR_DROPIN TM2_PORT_SPECIAL_AS TM2_PORT_DT_PLAYER`;
all = the car stack + `TM2_PORT_FAR_LOD TM2_PORT_FAR_DISTANCE TM2_PORT_SPU48 TM2_PORT_HUMANS=4`. Every row: both
proofs and the placement check pass; `check_hazards.py --expect-functions` (exactly the 12 functions with the define
alone) and `check_car_groups.py --expect-functions` pass on the builds with the define; `check_hazards.py
--negative-control` fails as it must on a build without it. Port end of the P4 link (port RAM end 80C00000).

| Defines | P4 sha | P2 (placement) sha | port end |
|---|---|---|---|
| none | `274d0ae4ab31` | `b95e6fed7439` (as before: request 38 adds no code without the define) | 805431C0 |
| `TM2_PORT_HAZARDS` | `c6bc54e42c0f` | `7c4ca86894d6` | 80543414 |
| car stack + `TM2_PORT_HAZARDS` | `9c7bf5d6c92a` | `90ec24aa1c19` | 8058D57C |
| all port defines + `TM2_PORT_HAZARDS` | `0c2603037efb` | `fc1c969d4866` | 8058F44C |

### 9.3 In game (for the port)

1. All bits clear: a recorded session gives the same game-state hashes and `rand()` stream as without the define.
2. Antarctica, bit 0: stand on every floe for minutes: no rumble, shake or fall; AI routes intact. 2 players: the
   six missing floes still missing.
3. Amazonia, bit 0: sit in lava 30 s: splash and wheel effects, HP unchanged (human and AI). Los Angeles section 14:
   see what it is and that it no longer burns.
4. Los Angeles, bit 0: drive through both fire volumes: no fire. Moscow, bit 0 only: after a bridge event its fires
   do not ignite.
5. Hong Kong, bit 0: no train drawn, no collision anywhere on the oval (the station window included), no rideable
   roof, weapons and splash at the old spots hit nothing. Bit 1 only: the trains run, hit as retail, cannot be
   destroyed.
6. Los Angeles / Moscow / Paris, bit 1: remote bombs on the trigger spots: no event, sections intact (drive over LA
   section 6 and Moscow's bridges), the after-event pickups never show.
7. Moscow, bit 1: shoot the dish: nothing; lightning works all round.
8. Paris, bit 1: kill AI cars down to 2 and fewer: the AI never heads for nodes 107..115.
9. New York and Holland, bit 1: 519 / 509 and both 510s absorb shots; the barns' teleporters and pickups work.
10. Props unchanged with both bits.
11. Online: two machines with the same words agree; a different word must be refused by the ruleset hash.

### 9.4 Builds

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261014-*`): hybrid
`9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB` / E0
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `274d0ae4�` (its `.port_bss` grew by the three words; code as before), placement check
OK. Proof helper: `pv.py TAG DEFINES` (P4 and P2 links, `verify_reloc.py` against E0, placement).

## 10. Questions for the port

- **Q1. Props** (500..505, 520 on every arena, many hiding pickups): **answered (the user, 2026-10-14):** the
  ordinary breakable props are **not switched**: they stay breakable; only the big hazards and destructibles listed
  (items 0..3, 8..15) are switched. Item 16 `PROPS` stays in the per-item mask (`portArenaItems`) as documented,
  **not offered by the menus, by decision**. No code change.
- **Q2. Drowning** (Hong Kong section 23): kept as a map edge. Turning it off needs a way out of the water.
- **Q3. Trains off:** removed (done). With only destructibles off the trains run and still spin you out but cannot
  be destroyed: is that what the menu means?
- **Q4. Antarctica in modes 2 / 4:** the six pre-collapsed floes are kept with hazards off (the 2-player file's
  layout). OK?
- **Q5. Eiffel AI patch with destructibles on:** retail's rule kept (the patch also applies while the tower stands,
  sending AI towards the floorless section 15). Want it only once the tower is down?
- **Q6. When the words may change:** at match setup only (they are latched per round; a destroyed arena stays
  destroyed until it is loaded again).
- **Q7. LA quake's write to 0x12:** skipped under the define for a null lane object (section 7). Without the define,
  how does the port map it?
- **Q8. LA section 14** (lava, class 2): what is it in game? It is switched with Amazonia's lava (item `LAVA`); split
  it if it should differ (section 8).
