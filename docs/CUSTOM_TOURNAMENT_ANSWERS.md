# Custom Tournament and mods: answers and what was built (PC port request 42, 2026-10-16)

The port's Custom Tournament (port `docs/CUSTOM_TOURNAMENT.md`) and its modding design (port `docs/MODDING.md`)
asked seven questions. This page answers each, names what was built (file:line on this branch) and how to switch it
on. Everything is behind a port define or a bit of a port word; with no define (and every new bit and word 0) the
game's logic is retail's, and the retail builds are byte-identical.

| Item | Answer in short | Built | Switch |
|---|---|---|---|
| 1 Ordinary Dark Tooth | bit 6 is eight pieces; the ordinary Dark Tooth is phase 1 without the boss parts; several in one match work | yes | `portCarOptions` bit 10 (0x400), `TM2_PORT_DARK_TOOTH`; per-slot boss role |
| 2 Sweet Tooth as a computer opponent | everything is there (retail fields him in tournaments); only Challenge's picker leaves him out | the picker (the user's decision) | `portCarOptions` bit 11 (0x800), `TM2_PORT_CAR_NUMBERS` |
| 3 128 mod drivers | 16-bit car numbers in port tables, the settings bytes keep the base car | yes | `TM2_PORT_CAR_NUMBERS` |
| 4 File formats | written spec | [`FILE_FORMATS.md`](FILE_FORMATS.md) | - |
| 5 Car select's turning car for mod cars | nothing in the draw stops it; the install is the port's; the name labels had a real bug | yes (label fix, N children, hooks) | `TM2_PORT_DT_PLAYER` bit 8 / `TM2_PORT_CAR_NUMBERS` |
| 6 Arcs behind walls | link each segment at its own depth | yes | `portVisual` bit 0, `TM2_PORT_VISUAL` |
| 7 The taser's start lower | start + 25, less jitter at the start | yes | `portVisual` bit 1, `TM2_PORT_VISUAL` |

New words and bits (all in `.port_bss`, every variant, after `portBalanceGunObjects`; `config/US/port_variant_arrays.tsv`):

| Name | Size | Meaning |
|---|---|---|
| `portCarOptions` bit 10 `TM2_PORT_CAR_OPT_DT_ORDINARY` 0x400 | (existing word) | computer Dark Tooths are ordinary (item 1); read live |
| `portCarOptions` bit 11 `TM2_PORT_CAR_OPT_AI_SWEET_TOOTH` 0x800 | (existing word) | Challenge's picker offers Sweet Tooth while unlocked (item 2); read when the screen opens |
| `portVisual` | 4 | bit 0 arc depth, bit 1 taser lower (items 6, 7); 0 = retail; read live |
| `portCarDtBossMask` | 8 | per car slot: the Dark Tooths that keep the boss parts under bit 10 (port-set, never cleared) |
| `portSettingsHumanCar`, `portSettingsOpponentCar` | 32 + 32 | sessionSettings' 16-bit car numbers (16 each) |
| `portModCars` | 4096 | 128 mod cars of 32 bytes (`car_numbers.h` `struct TM2PortModCar`) |
| `portCarNumbers` | 208 | the car number of each load group; the numbers kept around the attract demo |
| `portCarSel` | 2016 | car select's (and the AI select's) extended carousel and name-label copies, the mod count |

Headers: `include/semantic/car_specials.h` (bit 10), `include/semantic/car_numbers.h` (new: items 2, 3, 5),
`include/semantic/port_visual.h` (new: items 6, 7). Checks: `tools/check_custom.py` (new; run by `tools/check_all.py`
for links with `TM2_PORT_DARK_TOOTH`, `_DT_PLAYER`, `_VISUAL` or `_CAR_NUMBERS`), `tools/check_car_groups.py`
(updated function lists).

---

## 1. Dark Tooth as an ordinary opponent (`portCarOptions` bit 10)

### 1.1 What bit 6 turns on, piece by piece

The test is `TM2_PORT_BOSS_ANYWHERE` (`car_specials.h`). Retail's own gate for these pieces is "level 8" (some also
"`aiCars[0]` is 0xF0").

| # | Piece | Where | Kind |
|---|---|---|---|
| a | the boss update on any arena, machine guns off (`aiDarkToothBossUpdate`: missiles, the swoop, the head over the truck) | `aiWeaponFireDecide` 80103CA0, `weapon_tick_init.c:106` | **phase 1** |
| b | the per-car stage loop: a dead Dark Tooth in stage 1 counts as remaining, spins while its wreck timer >= 121, enters stage 2 at 0 (HP 110) | `aiCarsCountRemaining` 8011C248 `scene_init.c:150` -> `portCarDarkToothStages` | **boss only** |
| c | wreck state 4 after a death in stage 2 | `carApplyDamage` 800E5628 `car_damage.c` -> `portCarDarkToothWreck` | **boss only** |
| d | Hong Kong's jump zones only on level 8 | `aiDarkToothJumpSelect` 800F6DDC, `car_restart.c:172` | level-8 rule |
| e | Hong Kong's warps 1 / 2 only on level 8 | `aiFollowRouteTarget` 800F7D54, `screen_pause_menu.c:12` (`TM2_HK_WARPS`) | level-8 rule |
| f | the type-240 boss markers' tail (warp 2, spin-outs) only on level 8 | `carContactApplyObject` 800E06CC, `car_object_contact.c:287` | level-8 rule |
| g | the type-61 missile wobble on every arena | `weaponProjectileUpdate` 800DB110, `projectile_hit.c:144` | **phase 1** (looks) |
| h | VRAM rows 240..255 for bank-0 CLUTs (room for Dark Tooth beside more cars) | `portCarClutsReset` `port_variant_arrays.c:4389` | room |

Not tied to bit 6: the level-8 `aiCars[0]` boss (`roundStart`'s route patch, retail's stage-2 tail, the round end,
"But wait..."), and everything keyed only on type 0xF0 (the truck, the head made per BB.DMD install, the weights, the
ricochet model, the +25 hit size, `carDestroyed`'s 360 timer). The controller `aiDarkToothBossUpdate`
(`ai_dark_tooth_boss.c:60-172`) needs no split: its only stage read is the hover height (`:101-104`), it writes no
stage or flag, and it has no `aiCars[0]` assumption (each Dark Tooth's state is selected per car key).

### 1.2 Built (`TM2_PORT_DARK_TOOTH`, bit 10, `car_specials.h`)

`portCarDarkToothOrdinary(slot)` (`port_variant_arrays.c:1941`) is 1 for a car slot that holds a **computer** Dark
Tooth (an AI record of type 0xF0) while bit 10 is set and the slot is not in the boss mask; it returns 0 at once with
bit 10 clear, so every game test of it is retail's (or bit 6's) logic then (`tools/check_custom.py` proves the
bit-clear path returns 0). An ordinary Dark Tooth:

| Piece | Where (game) | With bit 10 |
|---|---|---|
| phase 1 on every arena | `weapon_tick_init.c:106-113` | the boss update runs (missiles, swoop, head over the truck, no guns); `portCarDarkToothHeadShow` (`:1975`) shows his head while his car object is drawn (after an AI-lives start, hidden while cloaked) |
| missile wobble | `projectile_hit.c:144/147` | with bit 10 too (`TM2_PORT_DT_ORDINARY_ON`) |
| CLUT rows 240..255 | `port_variant_arrays.c:4389` | latched with bit 10 too |
| no stage loop / stage 2 | `portCarDarkToothStages` (`:1838`) | skipped |
| no level-8 stage 2 when he is `aiCars[0]` | `scene_init.c:158` | returns |
| no Hong Kong boss route patch | `roundStart` 8011A024, `scene_init.c:69` (`TM2_DT_BOSS_SLOT`) | not patched |
| no jump zones, warps, boss markers | `car_restart.c:172`, `screen_pause_menu.c:12`, `car_object_contact.c:290` | on no arena |
| death | `portCarDarkToothWreck` (`:1810`) | wreck state 4 (one burst of the truck, as any car), head hidden, swoop ended |
| after a won round | `carApplyDamage`, `car_damage.c:32-36, 215` (`TM2_WON_SPARED`) | no damage, as any car (the user's "treat them all the same", request 41 item A) |
| wreck timer | `carDestroyed` 8011BE10, `car_destroyed.c:14, 403` | 120 and no self-hit, as any car (retail's 0xF0: 360 + a 15-point self-hit) |

Counting: `aiCarsCountRemaining` counts HP > 0 only, so he drops out of `aiCarsRemaining` at his death (one kill, one
elimination); the killer credit is `carApplyDamage`'s as for any car; no `rand()` call is added or moved.

**Several Dark Tooths, one of them the boss.** Each Dark Tooth already has his own head, timers and stage (request
33-2e's per-key state). The role is per **car slot**: `portCarDarkToothRole(slot, boss)` (`:1956`, the port's API,
between frames) sets bit `slot` of `portCarDtBossMask` (1 = keeps the boss parts: bit 6's fight, retail's level 8;
0 = ordinary; < 0 reads). The C never clears the mask. So: bit 6 + bit 10 + the boss's slot marked = one boss with
stage 2 and any number of ordinary Dark Tooths. Limits: one BB.DMD each (`portCarOptions` bit 1 for many), the same
look without bit 7 / banks, the pools (bit 3), and Hong Kong's routes for an ordinary Dark Tooth on level 8 (not
patched; how well he drives there is untested).

**Tournament caveat.** With bit 10 set and no mask entry, the level-8 tournament boss (`aiCars[0]`) is ordinary too:
no stage 2, no "But wait...", no Hong Kong route patch, the round ends at his first death. The port must either
leave bit 10 clear in the tournament or mark `aiCars[0]`'s slot as boss (`portCarDarkToothRole(aiCars[0].slot, 1)`).

## 2. Sweet Tooth as a computer opponent

**Everything is present** (no crash, no "never fires"). Retail already fields a computer Sweet Tooth: the tournament
picker `tournamentPickOpponents` (8012CD08, `tournament_pick_opponents.c:11`) forces `unlockSweetTooth` and rolls cars
0..12, and the attract demo uses it.

| Need | Where | State |
|---|---|---|
| AI profile, weights, fire chance | `aiCarSetupProfile` 800D3C7C `case 0x10` (`ai_route_nodes.c:42-44`): `aiSweetToothWeights9/10` (80162608 / 80162614: 80 % special), fire chance 100 | present |
| the special's AI use | `weaponFireSpecial` 80105A7C `case 0x10` (`weapon_basic.c:184-191`): the spiral (15 damage) | present |
| stats, top speed, step height, radar, engine sound | `carStatsSweetTooth` 800FE62C and the per-type switches | present |
| load | `carIndexToType(12)` = 0x10, `carFileCode(12)` = "ST" | present |

Behaviour to expect: fire chance always 100 (no difficulty scaling) and about 80 % of his picks are the spiral, so he
is more aggressive than cars 0..11; like every computer shot, his spirals home on player 1 (`AI_VS_AI.md` 4).

**What leaves him out** is Challenge's opponent picker `screenAiSelect` (80131EEC): AICARSEL.DMD's carousel (node
9005) and its name labels (9006 / 9007) hold the 12 regular cars, and the bio wrap is `% 12`. For the port's own
Challenge Match UI no code is needed: `settingsSetOpponentCar(&sessionSettings, i, 12)` is a supported state.

**Built (the user's decision: excluded only while locked).** With `TM2_PORT_CAR_NUMBERS` and `portCarOptions` bit 11
(`TM2_PORT_CAR_OPT_AI_SWEET_TOOTH`, `car_numbers.h`), `portAiSelExtend` (`port_variant_arrays.c:8580`, called at
`screen_ai_select.c:326`) gives the carousel and both labels 13-child copies **while `unlockSweetTooth` is set**; child
12 is the empty node until the port puts a model there (`portCarSelSetChild(12, model)`), the labels' child 12 is
empty (the port draws his name). The bio wrap goes over the carousel's count (`TM2_AI_SEL_PREV`, `:340`; 13 when
extended), `carBioPictureFiles[12]` is his bio, the stored opponent is the carousel index (12 = Sweet Tooth),
`carIsSelectable` accepts 12 only while he is unlocked. Locked, or the bit clear: AICARSEL's own nodes, retail exactly
(a start car left at 12 by an extended picker goes back to 11). Minion (13) stays out, as in retail.

Can the AI select borrow car select's Sweet Tooth model? **No** (checked on the disc files): AICARSEL.DMD replaces
CARSEL.DMD at 0x80019C40, so CARSEL's child 12 (node 0x800324F0) is gone from RAM, and its textures are overwritten
in VRAM: his body image (CARSEL.TMS key 0xCCF6, 8-bit, (768,128)) lies under AICARSEL's image 3 (key 0xFE13), his
256-colour CLUT row (768,35) under AICARSEL's 16-colour CLUTs (768..847,35). The port loads a copy as for a mod car
(`DARK_TOOTH_PLAYER.md` 10.3's recipe). The AI select's labels have 12 children, as many as its carousel, so retail's
AI select has no label bug; the 13-child copies keep them equal.

Optional balance knob (not built): `case 0x10`'s fire chance 100 could scale with the difficulty like the others.

## 3. 128 mod drivers (`TM2_PORT_CAR_NUMBERS`)

**Finding.** A car number (car-select index) matters only before the match: menus, the load, the bind, between
rounds, the pause screen and the ending. In a match every car is its type or key. The settings record (48 bytes,
`lives[]` right after the opponent bytes, 13-byte clears) cannot widen, so the numbers live in port tables.

**Built** (`car_numbers.h`; PER_CAR_PLAN.md E.5; CAR_GROUPS.md 18):

- Numbers: 0..14 retail, `0x20 + m` mod m (0x20..0x9F, `portModCars[m]`: used, base car, name, file code, bio
  picture, ending movie), -1 none.
- Storage: `portSettingsHumanCar[16]`, `portSettingsOpponentCar[16]` (sessionSettings' numbers); the record's byte
  keeps the **base** car (`portCarNumberBase`, `port_variant_arrays.c:8664`), so every byte reader sees a retail car.
- Setters (`settingsSetHumanCar` / `settingsSetOpponentCar` `menu_state_accessors.c:26/31`, `sessionSetOpponentCar`
  `game_setting_accessors.c:47`) write both (`portCarNumberSet`, `:8692`); the clears (`game_settings_extra.c:45/73`)
  both (`portCarNumbersClear`).
- Getters (`settingsGetHumanCar`, `sessionGetHumanCar`, `settingsGetOpponentCar`, `sessionGetOpponentCar`:
  `game_settings_extra.c:27/53`, `game_setting_accessors.c:26/36`) return an **int** (`TM2CarNumber`, `settings.h`;
  no sign extension): the table's mod number when its base is the byte, else the byte (`portCarNumberGet`, `:8676`).
  Without a mod number every getter returns retail's value.
- The mod-number bugs fixed: the bind gets the base's type (`carIndexToType` `tournament_tables.c:111` ->
  `portCarNumberType`); names / plates (`carDisplayName`, `carFileCode` `car_parts_spin.c:422/450`: the mod's or the
  base's); passwords never with a number >= 12 (`sessionRun` `session_runner.cpp:117`, `pauseMenu`
  `screen_pause_menu.c:592`); `screenEnding` reads `endingMovieTable` only below 14 and plays a mod's movie
  (`screen_ending_setup.c:42` -> `portCarEnding`, `:8613`); sign extension (the getters, `screenEnding`'s
  `(signed char)` casts, `sessionLoadArenaAndCars`' `(s8)(u8)`); `carIsSelectable` compares ints and refuses numbers
  that are no car (`car_is_selectable.c:36`, `portCarNumberPickable`).
- The load (`TM2_PORT_CARS`): `sessionLoadArenaAndCars` hands `portCarLoadGroup` the number (`shell_main.c:308-348`);
  it keeps it per group (`port_variant_arrays.c:1385`, read with `portCarNumberOfGroup(group)`) and loads the base's
  disc files when the port set no buffer. Keys and types stay the base's (a mod registers as a duplicate of its base).
- `shellAttractDemo` keeps the numbers around its demo match (`shell_attract_demo.c:20/35`); `portHumanCompact` moves a
  human's number (`:5141`).
- Kept on the bytes (the base car): the Minion boss test (`sessionRun`, a mod built on Minion counts as Minion), the
  load's disc fallback, `portCarSpecialLoadPlan`, `settingsCopy`.
- Port side (not decomp): store mods by string id, assign numbers per session; drop `(int8_t)` / `& 0xFF` / `% 15` on
  the getters' results (`ui/options.c`, `vplus/fixes.c`, `profile/profile_hooks.c`, `vplus/four.c`).

## 4. File formats

[`FILE_FORMATS.md`](FILE_FORMATS.md): DMD header, relocation, the load walk and typed objects, every node type, vertex /
normal / primitive records, texture references, TMS (keys, fixed and relocatable placement), the PC release's TPC and
DPC differences, Twisted Metal 1 (unknown), a converter checklist; the readers in `tools/` named; unknowns marked U.

## 5. Car select's turning car for mod cars

**Nothing in the draw stops it.** The carousel is CARSEL's select node 9005, drawn by `sceneObjDraw` (80122124,
`scene_init.c:353`; `renderDrawModel` at `:378`), which walks only `child[node[7]]` with the carousel's position, spin,
camera, lights, OT and packets. A relocated in-game car root in that slot is drawn the same way; no car record, world or
match state is needed. VRAM is not the obstacle for normal cars (a free 128 x 256 block at (832,256) and CLUT rows 40..63
at x 768); Dark Tooth's BB.TMS (79,616 halfwords) does not fit. The install recipe: `DARK_TOOTH_PLAYER.md` 10.3.

**The bug (fixed).** `menuCopyChoiceRun` (8013724C) copies the carousel index into the name labels 9006 / 9007 every
frame. CARSEL's labels have 14 children, so at index 14 (a player's Dark Tooth, `portCarOptions` bit 8) their walk read
the word after the child table (0x0000FF00 for 9006, 0x2330FF00 for 9007) as a node pointer. `portCarSelExtend`
(`port_variant_arrays.c:8476`, called at `screen_car_select.c:376`) now copies the carousel **and both labels** with
the same count (`portCarSelCopy`, `:8437`); the labels' extra children are the empty node.

**Built for mod cars** (`TM2_PORT_CAR_NUMBERS`): the carousel index **is** the car number. With `portCarSel.mods` > 0
the three copies have `0x20 + mods` children: 0..13 retail's, 14 Dark Tooth (bit 8) or empty, 15..0x1F empty and never
accepted, `0x20 + m` mod m (empty until the port sets it). Up to 160 children (the node's count byte allows 255).
- `portCarSelSetChild(index, model)` (`:8536`): swap one extra child (the in-game model loaded on demand; 0 = empty).
- `portCarSelDraw(node)` (`:8800`), called by `sceneObjDraw` before each draw of the extended carousel
  (`scene_init.c:372`): the port wraps it to load the highlighted car (the index changes during the slide-out).
- `portCarSelNumber(index)` (`:8551`) maps the picked index to the stored number before `settingsSetHumanCar`
  (`screen_car_select.c:392`; the identity for every car).
- `portCarBioFile` (`:8515`) gives a mod's bio picture; `carIsSelectable` refuses the empty children.

## 6. Arcs behind walls (`portVisual` bit 0)

Retail links every arc segment at OT entry 0 (drawn last, over everything): `weaponArcsDraw` (801084D8). Both weapons
use this code (kind 0 the taser, 1 the lightning). With `TM2_PORT_VISUAL` and bit 0, `ARC_OT(j)`
(`weapon_arcs_draw.c:248`, used at `:366`) links segment j at `portVisualArcOtz(z_a, z_b)` (`port_variant_arrays.c:8401`):
the nearer end's camera depth >> 3, compressed as the level emitter compresses SZ (`renderCompressDepth` 8014D368), 4
entries in front (`TM2_VIS_ARC_BIAS`, `port_visual.h`), clamped to 1..4094. A level polygon is sorted by its farthest
corner, so the arc sorts in front of the polygons it touches and behind walls nearer than it. No depth buffer: a big
wall polygon that reaches farther than the arc can still sort behind it. Unknown: whether far nodes with
`portFarDistanceScale` keep SZ>>3-compatible indices (arcs are culled at camera z 16364 anyway). No `rand()` call is
added or moved; the packets and their count are unchanged.

## 7. The taser's start (`portVisual` bit 1)

**Since request 76 (7.4) bit 1 starts the taser at Outlaw 2's light bar;** the + 25 below was request 42's first cut.

Retail: the start point is the shooter's position + 30 with no jitter (`weapon_arcs_draw.c:306`), points 1..14 +30 plus
the slope (`:314`) with `(rand() & 0x3f) - 32` per axis (`:315-317`). With bit 1, for the taser only (kind 0; the
lightning keeps 30): `ARC_LIFT` = 25 (`TM2_VIS_TASER_LIFT`: Outlaw 2's launch height 17 + 8, the muzzle) at both lines,
and `ARC_JIT` scales the first jittered points: point 1 +-8, point 2 +-16, then +-32. The `rand()` calls are retail's
(count and order: 8 sites); only the values they feed are scaled. Bits 0 and 1 are independent, for the user to judge in
game. The port's `interp_WeaponArcsDraw` needs no change.

### 7.4 Request 76: from the siren (built)

Port request 76 item 2 (2026-10-28): with bit 1 on or off the port's pictures of Outlaw 2 firing were the same, and the
arc's start is ahead of and above her light bar. The ask: the start at the siren, for the player's and the computer's
Outlaw 2, under the same bit.

- **The bit did reach the arc** (CONFIRMED, link run, `tools/check_custom.py` check 2): `weaponArcs[i].attachment` is
  the arc's kind (+6: 0 taser, 1 lightning; `weaponTaserTarget` stores 0, its `SLOT.attachment` is the `to` pointer at
  +16), so `ARC_TASER_LOW` holds for her shot. But its effect was too small to see: + 25 for + 30 is 5 position
  units (under one world unit), and the scaled jitter moves points 1 and 2 by a pixel or two at a chase camera's
  distance. If the port's frames are byte-identical with the bit on and off, read `portVisual` back during the
  match: bit 1 must be set when the arc draws.
- **Why it looked ahead of and above the bar** (CONFIRMED, `OT.DMD`): `from` is the shooter's `motion.pos`
  (`weaponTaserTarget`'s origin), the car's centre; retail adds 30 straight up. Outlaw 2's model has three LODs; the
  near one (LOD record `[0, 200)`; `8001BC30` the body, the bar `8001DE08`.. animated, 12 frames) has its roof at
  z 16 and the bar at y -18..-12, z 17..20 (+y is the car's front: `car_collision.c` takes a front hit when `vel[1] > 0`; the
  animated parts behind y -58 are LIKELY the turbo flames). So the old start was 15 ahead of the bar's centre and 10 above its top (5 with + 25). The far LODs are
  bigger (`[200, 500)`: bar top 32; `[500, 1000)`: 37); the near one is the one a chase camera sees.
- **Built:** with bit 1 a taser arc starts at `portVisualTaserFrom(from, at)`: `from` plus the car's body matrix
  (`motion.body_m`, 0x50 bytes after `pos`: the heading with the tilt) applied to (0, `TM2_VIS_TASER_SIREN_Y` -15,
  `TM2_VIS_TASER_SIREN_Z` 20), as the guns turn their muzzle (`mathMulTransVec`). The lift over it is 0, and the
  points 1..14 run from that start to the target (retail runs them from the centre + 30). It is read at each draw, so
  the start stays on the bar as she drives and turns; human and computer alike (both records have the motion block).
  The jitter scaling stayed (request 88 took it out: 7.5). The lightning (kind 1) keeps retail's start.
- **Borrowed tasers** (special-as, gun game) start at the same place on the borrowing car's body: about its roof,
  not a siren.
- **LIKELY, not seen in game:** that `body_m` is the matrix the body is drawn with (it is `rot` plus the tilt,
  `carMotionUpdate`; `rot_m` alone differs only by the tilt), and that the drawn car's origin is `motion.pos`.
- **Checked:** check 2 runs `weaponArcsDraw` in the link with a turned shooter: with the bit the first segment starts
  at the light bar's projection, without it, and for the lightning either way, at + 30; `portVisualTaserFrom` calls
  only `mathMulTransVec` and only `weaponArcsDraw` calls it; the retail call census of `weaponArcsDraw` is unchanged.

## Proofs (2026-10-16)

Port variant links (`build_source_only.py --relocatable --sdk-from-lib --port-variant`, E0 = the retail relocatable
link), each with its placement link (limits forced to 2); `tools/check_all.py` passed on all (proof, placement,
humans, car groups, hazards, balance, custom):

| Config | Defines | P4 image | P2 image | Changed game functions |
|---|---|---|---|---|
| none | - | c1108ee8 | b95e6fed | 0 |
| vis | `TM2_PORT_VISUAL` | bfb89095 | f2de9cd4 | 1 (`weaponArcsDraw`) |
| num | `TM2_PORT_CAR_NUMBERS` | b4ca015b | 919f5e6f | 22 (exactly check_custom's list) |
| dt | `_CAR_GROUPS`, `_CAR_SPECIALS`, `_DARK_TOOTH` | 8c91c219 | 1815122f | 64 (check_car_groups' list) |
| dtp | dt + `_DT_PLAYER` | b20d5b73 | 4ff7e2cc | 75 |
| cars | the 11 car defines + `_VISUAL` + `_CAR_NUMBERS` | 253e6337 | c0a8323f | 118 |
| allh16 | every port define, `TM2_PORT_HUMANS=16` | c3942a37 | 7b0de813 | 159 |

The full default build (`work/build-20261016-210450-578481`): hybrid 9219f7ba, source-only d755c86e, `--sdk-from-lib`
f557c1e3 (retail, unchanged); default variant c1108ee8, placement b95e6fed. No new retail bug was found (the label walk
past 13 and the pause menu's password for car 14 were request 36's, reachable only with `TM2_PORT_DT_PLAYER`).

## Open questions

1. Item 1: should `roundStart` keep Hong Kong's boss route patch when an ordinary Dark Tooth is `aiCars[0]` on level 8
   (built: no patch)? Untested how he drives there.
2. Item 3: mod car profiles (stats, weights, camera per key, `PER_CAR_PLAN.md` E.2) are still the base type's; the
   numbers give the port the group -> mod mapping it needs to add them.
3. Item 5: the port's own VRAM spots / slot table for the in-game model in car select (10.3) are untested in the game.
4. Items 6 / 7: the bias (4) and the lift (25) are first values for the user to judge.

### 7.5 Request 88 item 2 (2026-10-30): retail's jitter with bit 1 (built)

The user: the start at her light bar is right, the smaller jitter at points 1 and 2 is not wanted. With bit 1 only the
start changes now: `ARC_FROM` (`portVisualTaserFrom`) and `ARC_LIFT` (0); `ARC_JIT` is retail's at every point (+-32;
requests 42-87 shifted points 1 and 2 to +-8 and +-16). `weapon_arcs_draw.c` only; the `rand()` calls are retail's as
before. Checked: `tools/check_custom.py` check 2 (the start run in the link, the call census against E0). Not tested
in game.
