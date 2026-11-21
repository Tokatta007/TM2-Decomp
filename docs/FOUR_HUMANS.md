# Four human records

Answers item 23 of the port's `docs/DECOMP_REQUESTS.md`: real human records for players 3 and 4
(instead of pad-driven computer-car records, `FOUR_PLAYERS.md`), by rebasing `humanCars` and the
arrays that go with it to a larger area. Static analysis only: the C in `src/semantic`, the retail
disassembly (`bin/SCUS_94306R.EXE`, `toolchain/mips/bin/mipsel-none-elf-objdump`), the
relocations of the source-only build's unit objects (`work/build-20261007-034623-844435/compile/*/source.o`,
`objdump -r`), its symbol table (`TM2_SOURCE.elf`, same data addresses as retail) and
`config/US/data_relocs.tsv`. Nothing was built or run. Addresses are retail.

Builds on `FOUR_PLAYERS.md` (sections 2 to 5), `SIXTEEN_CARS.md` (1.1, 1.2, 2.2), `AI_VS_AI.md` and
`COOP_LIVES.md`. Where those documents already give the detail, this one points to them.

**Implemented as the port variant (request 25):** `docs/SOURCE_BUILD.md`, "Port variant", builds the
data-structure part of section 7 (items 1, 2 and 5, and the radar part of 3) behind
`include/semantic/players.h`. Section 8 below says what it covers and what is left to the port.

Tags: **CONFIRMED** = seen in code (the C is byte-matched to retail; an instruction VA was read in
the disassembly). **LIKELY** = follows from the code, not seen running.

Words used below: **index** is a human's place in `humanCars` (0..3). It is also the record's
`player` field (+0x000, written by `carBindHumanSlot`), and the game uses that one number as the pad
port, the sound side, the view, the camera, the HUD record and the hotkey slot. **[2] / [3]** is what
an index of 2 or 3 reads in retail's two-entry array: the bytes that follow it in memory.

## Short answer

- **Nothing points into the human arrays from data, and no code compares a record address with a
  constant.** `data_relocs.tsv` has no word whose value lies in any array of this document. Every
  use is a `lui`/`addiu` (or `lui` + load/store) pair, or a gp-relative load/store. Moving an array
  means adding the move to those instructions, with their addends. CONFIRMED.
- **Almost nothing can grow in place.** The sdata/sbss arrays are packed against each other, and
  `humanCars` ends where `aiCarObjects` begins. Only `carResetWeapons` (u8, 4 bytes, 2 used),
  `hudRadarVisible`, `hudWeaponListVisible` and `padLayout` (u8 in 4 bytes each) already have room
  for 4. Everything else must move (section 1). CONFIRMED.
- **In retail, index 2 or 3 does damage at once.** In `worldUpdate`'s human loop, `hudSmoothSpeed(2)`
  writes the speed into `humanCarTotal` (the loop's own bound). `viewSetFollowObjPos(pos, 2)` uses
  `viewFov` (70 or 60) as an object pointer. `viewSetRotation`/`viewSetPosition` for player 2 write the
  camera into view 0's frame record. `carViewHotkeys` for player 2 writes `padLayout`. So the human
  arrays, the camera/view arrays and the hotkey array must all move together (sections 1 and 3).
  CONFIRMED (the aliases); LIKELY (the effects).
- **Constant bounds of 2 are few:** `uaInit` (two loops), `carTablesClear` (a count-down from
  `0x814`), `roundStart` and `sessionRun` (`carSetResetWeapons` for 0 and 1 only), the radar pair
  and the round-end test in `worldUpdate`, the result codes 3/4 of `humanCarDamageUpdate`, the
  `padCheckAction` ports, the two sound sides. Everything else loops on `humanCarTotal`. Section 2.
  CONFIRMED.
- **Object ids 3 and 4 already work.** Every "is a human" test on an id is `id < 50` (or
  `(u32)(id - 1) < 99 && id != 8` for "is a car"). No test uses `id <= 2` or `id == 1/2` to mean a
  human. One function turns an id back into an index (`owner - 1` in `weaponDropCheckHit`), which
  works for 4 records. Section 4. CONFIRMED.
- **Settings:** with `humans = 4`, every `i < humans` loop over `human_car[i]` (+0x08) reads cars 3
  and 4 from +0x0A/+0x0B, which is `opponent_car[0..1]`, while the opponent loops read the same bytes
  as opponents. Section 5 lists both sets of readers and two ways out. CONFIRMED.

## 1. What moves together with `humanCars`

### 1.1 Arrays indexed by the human index

"Sites" are instruction VAs (the `%lo` instruction; its `lui` comes just before it). "gp" marks a
gp-relative access (`gp` = 801809D4). All of the sdata/sbss ones are in `bss_in_file`
(80180EE4..80181454) or sdata and are set by code, not by initialised data. CONFIRMED (addresses,
sizes from the next symbol, sites from the disassembly; the counts equal the `R_MIPS_HI16`/`LO16`/
`GPREL16` relocations of the unit objects).

| VA | Name | Size | Sites | [2] / [3] in retail | Room for 4? |
|---|---|---|---|---|---|
| 80187B10 | `humanCars` | 2 x 0x814 | 195 pairs in 34 functions (`SIXTEEN_CARS.md` 2.2) | `aiCarObjects` (80188B38, 0x24) then `aiCars[0]` | no: must move (4 x 0x814 = 0x2050) |
| 80180FEC | `humanCarObjects` | 2 x ptr | 8011A458 `carBindHumanSlot`, 8011A788 `worldUpdate`, 8011CB54 `humanCarObject`, 8011D9A4 `carSaveNodeState`, 8011DA2C `carRestoreNodeState`, 8011E2EC `carsPhysicsUpdate` | `humanCarObjectSlots[0]`, `[1]` | no |
| 80180FF4 | `humanCarObjectSlots` | 2 x s32 | 8011A46C `carBindHumanSlot`, 8011A940 `worldUpdate`, 8011BE68 `carDestroyed`, 8011D298 `carObjectId`, 8011D8C0 `humanCarObjectSlot` | `padConfigs[0]`, `[1]` | no |
| 80180FFC | `padConfigs` | 2 x s32 | 8011D628 `PadSetConfig`, 8011D680 `PadGetConfig` | `humanCarTypes[0]`, `[1]` | no |
| 80181004 | `humanCarTypes` | 2 x s32 | 801197C0 `uaInit` (gp, `[0] = 0x90`), 8011A2B8 `carBindHumanByType`, 8011A328 `humanCarGetType`, 8011A48C `carBindHumanSlot` | `hudSmoothedSpeeds[0]`, `[1]` | no |
| 8018100C | `hudSmoothedSpeeds` | 2 x s32 | 8011CBB8 `hudResetSpeed`, 8011CC7C `hudSmoothSpeed` | **`humanCarTotal`**, `arenaLoadedId` | no |
| 80180FA0 | `carResetWeapons` | u8, 4 bytes | 8011983C `carSetResetWeapons`, 8011A1CC `humanCarsRestart`, 8011A4CC `carBindHumanSlot`, 8011BB18 `humanCarDamageUpdate`, 8011E140 `sub_8011E114` | bytes 80180FA2/3, never referenced | **yes** |
| 80180B9C | `carViewHotkeyReleased` | 2 x s32 | 10 sites in `carViewHotkeys` (8011C59C..8011C850) | `padLayout` (4 bytes), `bitmapTrackingEnabled` | no |
| 80181024 | `viewCameraObjectTable` | 2 x ptr | 801197AC/801197B0 `uaInit` (gp, [0] and [0]+4), 801198F4/801198F8 `carTablesClear` (gp, same), 80119CBC `viewCreateWithCamera`, 8011D394 `viewSetCameraPose`, 8011D440 `viewGetCameraPose`, 8011D4F0 `viewGetCameraObject` | `aiBossDefeatedFlag`, `arenaShortChaseRange` (u8 + 3 bytes) | no |
| 80180FD8 | `D_80180FD8` | 2 x s32 | 8011977C `uaInit` (zeroes it); 8011E098, 8011E0B4, 8011E0E0 in three accessors | `D_80180FE0[0]`, `[1]` | no, but see below |
| 80180FE0 | `D_80180FE0` | 2 x s32 | 80119774 `uaInit`; 8011E07C, 8011E0FC in two accessors | `carWorldObjectCount`, `humanCarObjects[0]` | no, but see below |

`D_80180FD8` / `D_80180FE0` need not move. Their only live user is `uaInit`, which zeroes [0] and
[1]. The five accessors (`sub_8011E078`, `sub_8011E094`, `sub_8011E0B0`, `sub_8011E0DC`,
`sub_8011E0F8`) are never called (no `jal`, no word in data) and neither is `sub_8011E114`. CONFIRMED.

Scalars that do **not** have to move: `humanCarTotal` (80181014; 16 gp sites: `humanCarsRestart`,
`humanCarsSetCount`, `worldUpdate` x2, `humanCarNearest`, `humanCarCount`, `humanCarFindById`,
`carsPhysicsUpdate` x8, plus one in `humanCarsRestart`'s loop), `carWorldObjectCount`, `aiCarTotal`,
`viewMainPlayer` (80180FA8). They only move if the port wants them away from a moved neighbour; once
`hudSmoothedSpeeds` moves, `humanCarTotal` is safe where it is.

The car **pair tables** (`carPairVectors` 8018D674 .. `carPairDistances` 8018F0D4) and the **slot
tables** (`carSlotKinds/Indices/Owners/Objects`, 80187A20..80187B10) are indexed by world-object
slot, not by human. A slot is one per car type (`SIXTEEN_CARS.md` 1.3), so 4 humans and up to 9
computer cars use at most 13 of the 15 slots. They need not move. CONFIRMED.

The record itself does not change. Its fields that hold the index: `player` (+0x000, =
index, from `carBindHumanSlot`), `id` (+0x002, = index, from `uaInit` and `humanCarSetId`; read by
`humanCarFindById`), `slot` (the world-object slot), and the action-to-button map at +0x0F8
(`carApplyPadConfig`, per record). CONFIRMED.

### 1.2 Per-player arrays reached through the same index

These are per **local player** (`SIXTEEN_CARS.md` 1.4), but the game reaches them with the human
index, so 4 human records on one machine reach entries 2 and 3. `FOUR_PLAYERS.md` 4.4 and 5.1 list
them for the views and the HUD (and request 24 asks for their relocations). Only what [2]/[3] hit is
added here. CONFIRMED (addresses).

| VA | Name | Size | [2] / [3] in retail | Reached per frame for every human? |
|---|---|---|---|---|
| 80180DDC | `viewFollowObjects` | 2 x ptr | `viewFov` (an int, 60 or 70), `viewRenderSplitMode` (0..2) | **yes**: `viewSetFollowObjPos(target, i)` at the end of each human's step (8011AA50) |
| 80181110 | `viewCameraObjects` | 2 x ptr | `viewEnvs[0]`, `viewEnvs[1]` (frame-record pointers) | yes in split screen: `viewCarCameraUpdate` -> `viewSetRotation`/`viewSetPosition(.., c->player)` |
| 80181118 / 80181120 | `viewEnvs` / `viewModes` | 2 each | `viewModes[0..1]` / `sndObjNeedVoices`, `sndObjUseVolumeScale` | no (only `viewCreate`, which refuses `i >= 2`) |
| 80180A6C / 80180A70 | `hudRadarVisible` / `hudWeaponListVisible` | u8, 4 bytes each | spare bytes | no (HUD and hotkeys) |
| 80180A74 / 80180A7C | `hudMarkerBlinkCounters` / `hudMarkerBlinkPhases` | 2 x s32 each | `hudMarkerBlinkPhases[0..1]` / unnamed sdata up to `hudLivesText` 80180AC4 | no |
| 801828D0 | `hudOrderTables[2][2][2]` | 32 bytes | `[0][2]` = `[1][0]` (the other buffer's player 0); `[1][2]` = `hudPlayerRecords[0]` | no |
| 801828F0 | `hudPlayerRecords` | 2 x 1,900 | `hudRadars` (801837C8) and on | no |
| 801837C8 | `hudRadars` | 2 x 880 | `hudRadarArrowUpSprites` 80183EA8 .. | no |
| 80180BA4 | `padLayout` | u8, 4 bytes | spare bytes | no |
| 801A4AEC | `padActionCache[128][2]` | 256 | `[a][2]` = `[a + 1][0]` | yes (`padCheckAction`, section 3) |
| 801A4BEC | `padActiveBindings[2]` | 2 x 128 | `[2]` = `padConfiguredBindings[0]` | yes (`padActionHeld`, section 3) |
| 801A4CEC | `padConfiguredBindings[2]` | 2 x 128 | `[2]` = `renderWalkerSaveRegs` 801A4DEC | no (only `padSetLayout`) |
| 8016475C | `sessionSettings` | 0x2C in RAM (`screenCardDrawEnvs` at 80164788) | `human_car[2..3]` = `opponent_car[0..1]`; `control_layout[2..3]` = `cd_volume`, `sfx_volume` | no (section 5) |

## 2. Every loop or test bounded by two humans

CONFIRMED (VAs read in the disassembly); the "with 4" column is LIKELY unless it says otherwise.

| VA | Function | Test | With 4 human records |
|---|---|---|---|
| 801196EC..80119764 (`slti 2` 8011975C) | `uaInit` | `for i < 2`: `carSetResetWeapons(1, i)`, record +0x1F4 = 1, `id = i`, +0x096..+0x09B = 0 | records 2-3 keep the previous session's `id`, damageable flag and these bytes; raise to 4 |
| 80119780..801197A4 (`slti 2` 801197A0) | `uaInit` | `for i < 2`: `PadSetConfig(1, i)`, `D_80180FD8[i] = D_80180FE0[i] = 0` | `padConfigs[2..3]` not set (0 from BSS); raise the loop for `PadSetConfig` only |
| 801197AC, 801197B0; 801198F4, 801198F8 | `uaInit`, `carTablesClear` | `viewCameraObjectTable[0] = [1] = 0` (two gp stores each) | per view, with the views (`FOUR_PLAYERS.md` 4.4) |
| 801198AC..801198C0 | `carTablesClear` | `humanCars[i].f_810 = 0` by a count-down: `li v0,0x814`, store at `humanCars + v0 + 0x810`, `v0 -= 0x814` while `v0 >= 0` | the count is the start offset `(2 - 1) * 0x814`: becomes `0x183C` for 4 |
| 8011A098, 8011A0A4 | `roundStart` | `carSetResetWeapons(0, 0)`, `(0, 1)` after `humanCarsRestart` | flags 2-3 are never cleared: once player 3 or 4 has died, every later restart resets their weapons even after a won round; add (0, 2) and (0, 3) |
| 8012C214..8012C858 (16 calls) | `sessionRun` | `carSetResetWeapons(.., 0)` and `(.., 1)` in pairs; 8012C2D8: `humanCarGet(0)->stats.hp <= 0` picks which of the two to reset after result 1 | the same for 2-3; the "who died" test needs a loop |
| 8012C260 | `sessionRun` | `--lives[0]` on result 1 (co-op and tournament) | one shared life count; `COOP_LIVES.md` 4 |
| 8011A7B0 (`bnez s1`), 8011A878 (`bne s1,1`) | `worldUpdate` | radar pair: when `i == 0` plot human 0 on radar 1 relative to `humanCars[1]`; when `i == 1` plot human 1 on radar 0 relative to `humanCars[0]` (the other record is reached as `humanCars + 0xF10` / `+0x6FC + 0x81C`, constant offsets) | humans 2-3 are on no radar, and 0/1 never see them; a per-view loop over all other humans |
| 8011B3E0..8011B4AC; 8011B4B0..8011B4B8 (`gameMode == 2`); 8011B4C0..8011B57C | `worldUpdate` (AI loop) | AI blips relative to `humanCars[0]` into radar 0; and only in mode 2 relative to `humanCars[1]` into radar 1 | radars 2-3 get no blips; also mode 4 (deathmatch with AI) gives radar 1 none |
| 8011B5AC..8011B5E8 | `worldUpdate` | round won: `aiCarsRemaining <= 0 && gameMode != 4 && !humanCars[0].state.dead (+0x09C, lbu at 8011B5D0) && !roundWonFlag` | only player 1's death blocks the win; "any human alive" over all (`COOP_LIVES.md` 3.2) |
| 8011BB24..8011BBB4 | `humanCarDamageUpdate` | at a death: `roundOverFlag = 1`; mode 4: `i == 0` (`bnez s1` 8011BB3C) -> event 4, any other i -> event 3; other modes -> event 1 | in deathmatch the **first** death ends the round and `sessionRun` credits player 2 (event 4) or player 1 (event 3); a death of player 3 or 4 counts as a win for player 1. Needs a "last one standing" rule and a result per winner |
| 8012C784, 8012C834 (the shared `jal`s) | `sessionRun` | mode 3: result 1 -> `screenChallengeResult(s, 0, wins2, ++wins1)`, 3 -> `(s, 1, ++wins2, wins1)`; mode 4: result 4 -> `screenDeathmatchResult(s, 2, wins2, ++wins1b)`, 3 -> `(s, 1, ++wins2, wins1b)` | tallies are stack locals (`wins1`, `wins2`, `wins1b`), two-sided; four tallies and a 4-player result screen in the port |
| before 8012C3CC | `sessionRun` | Minion intro: `human_car[0] == 13`, or `humans == 2 && human_car[1] == 13` | cars 3-4 not checked |
| 800EC8C0..800EC8E0 | `carNearestToHuman` | `humanCarCount() == 1 || gameGetMode() == 2` -> nearest live AI car; else -> `humanCarNearest(pos, a)` (the other human) | works for 4 (it is a nearest-of-all over `humanCarTotal`); in deathmatch with 4 the panel shows the nearest other human |
| 8011C8E4 | `humanCarNearest(pos, exclude)` | loop over `humanCarTotal`, `|dx| + |dy| < best` (start 0x7FFF), skip `exclude`; no HP test; returns 0 when nothing is nearer than 0x7FFF | works for 4; note it picks dead humans too, and it is what every computer car chases (`aiGetTargetCar` 8011CF0C with `exclude = -1`) |
| 80109FD8..80109FEC; 8010A2F0..8010A304 | `weaponFindTargetAhead` / `weaponFindTargetBehind` | AI owner and not attract: target = `humanCarObject(0)->pos` | computer cars' aimed shots steer at player 1 only (`AI_VS_AI.md` 4); with 4 humans, use the shooter's own target (`aiGetTargetCar`) |
| 800D87D4 | `bulletsUpdate` | `carPlayerOneObject = humanCarObject(0)` | player 1 only (use not traced, `FOUR_PLAYERS.md` 3) |
| 8011DAF8, 8011DB48; `world_collapse_warps.c` 301..321 | `viewWatchNextAiCar`, `viewStopWatchingAi` | view 7 / watch mode on `humanCars[0]` only | single-player feature; leave |
| 801295A0, 801295C4 | `pauseMenu` | Start on pad 0, or pad 1 | port's pause |

Loops already bounded by `humanCarTotal` (nothing to change but the arrays): `humanCarsRestart`
8011A198, `worldUpdate`'s human loop 8011A760..8011AA6C, `humanCarNearest`, `humanCarFindById`
8011D07C, `carsPhysicsUpdate` 8011E220, and every function in `SIXTEEN_CARS.md` 1.2 "Weapons,
damage, lock-on" (they loop on `humanCarCount()`). CONFIRMED.

There is **no** test of `humanCarTotal <= 2` (or `humans <= 2`) anywhere: `humanCarsSetCount`
(8011A268, called at 8012C054 with `settings->humans`) stores any value. The limit is only the
two-record array and the menus, which set `humans` to 1 (`screenSinglePlayerMenu`, attract) or 2
(`screenControllerMultiplayer`). CONFIRMED.

`worldUpdate`'s human loop also **breaks** at a null `humanCarObjects[i]` (8011A79C). In retail,
`humanCarObjects[2]` is `humanCarObjectSlots[0]`: the loop stops at i = 2 only if human 0 is in
slot 0, and otherwise passes a small integer as the object to `humanCarUpdate`. CONFIRMED (the
alias); LIKELY (the effect).

## 3. Where a human's index picks something fixed

| What | Where | Index 2 in retail | Index 3 in retail | For 4 records |
|---|---|---|---|---|
| pad reads | `humanCarReadInput` 800DE5F8, `carReadMoveCodes` 800DECA8: `padCheckAction(action, car->player)` 80150CFC | port 2 = **either pad**: `padActionHeld(action, 2)` reads `padActiveBindings[2]` = `padConfiguredBindings[0]` (port 0's configured layout) and `padButtonHeld(button, 2)` ORs both pads; the result is cached in `padActionCache[action][2]` = `[action + 1][0]` | `padCheckAction` returns 0: no input at all | the port's own pad read for 2-3 (or 4-entry binding arrays and a port-aware `padButtonHeld`; but port 2 must keep meaning "either" for the menus: `movieSkipCheck` 8012D03C, `movieSkipCheckDelayed` 8012B854) |
| cheat combos | `humanCarTick` 800DD070..800DD270 `padComboHeld(n, player)` 8014A6A8; `padAnyLatched(player)` 800DD0E0; `carViewHotkeys` 8011C870 `padComboHeld(1, player)` | `padComboHeld`: any port other than 0 reads pad 2's half; `padAnyLatched(2)`: both pads | same as 2 | port input |
| pad layout | `sessionRun` 8012C030 / 8012C03C: `padSetLayout(control_layout[0], 0)`, `(control_layout[1], 1)`; settings +0x26 `control_layout[2]` | `control_layout[2]` = +0x28 `cd_volume`; `padSetLayout(x, 2)` would write `padLayout[2]` (spare) but then `padLoadLayout(2)` writes `padConfiguredBindings[2]` = `renderWalkerSaveRegs` and `padActivateBindings(2)` overwrites `padConfiguredBindings[0]` | `control_layout[3]` = +0x29 `sfx_volume`; port 3 likewise past the end | never call `padSetLayout` with 2-3 in retail; layouts for 2-3 in port fields |
| pad config per record | `PadSetConfig(config, player)` 8011D600 (from `uaInit`), `PadGetConfig(player)` 8011D660 (from `carRestart` 800FD550) | `padConfigs[2]` = `humanCarTypes[0]`: `carRestart` passes the car type (e.g. 0x90) to `carApplyPadConfig`, whose default case is configuration 1 | `padConfigs[3]` = `humanCarTypes[1]` | move `padConfigs` with the human arrays |
| sound loops P1/P2 | `soundStartPlayerLoop(type, .., side)` 800D3244 -> `soundMgrPlaySided` 8012854C; stop 800D328C -> `soundMgrStopSided` 80128740. Callers: turbo loop 0 `carThrottleSteer` 800DDD08 (every frame of turbo), gun loop 9 `weaponTick` 80103AAC (every frame of fire), loop 32 burn/lava `carSoundUpdate` 8011DD5C; stops at 800DDE00..800DDF04, 80103B5C/80103B9C, 8011DD88/8011DDB0 test `player == 0` / `== 1` | side 2: the sound object is taken from the pool (`sndPoolAlloc`, 35 objects) and set up, then neither listed nor freed; the stop does nothing | same | LIKELY: every frame of turbo or guns by player 3-4 **leaks one sound object**, so all sound stops after about half a second. Give 2-3 their own lists, or the shared loops by a unique instance (`soundStartSharedLoop` 800D32BC, as computer cars do) |
| loop flags | `soundLoop32ActiveP1/P2`, `soundTurboLoopActiveP1/P2`, `soundGunLoopActiveP1/P2` (80180A06..80180A0B, bytes) | none (scalars, by side 0/1 only) | | port flags |
| shield hum | `carSoundUpdate` `soundPlayVariant(21, .., id)` with `id = player` | instance 2 is also computer car 2's (AI uses its index +0x004) | 3 likewise | already shared in retail for 0/1; harmless |
| HUD records, OTs | `rtMainLoop` draws HUD 0 and 1 only; `hudDrawSplit*(player, ..)` index `hudPlayerRecords[player]`, `hudOrderTables[buf][player]`, `hudRadars[player]` | records: `hudRadars`; OT: the other buffer's player 0 entry, or `hudPlayerRecords[0]` | beyond | never called for 2-3 in retail; port HUD (`FOUR_PLAYERS.md` 5.3) |
| HUD state | `hudToggleRadar` 800FCFA4, `hudToggleWeaponList` (from `carViewHotkeys`), `hudBlinkWeaponMarker` 800FB590 | `hudRadarVisible[2]` spare; blink counter [2] = `hudMarkerBlinkPhases[0]` | spare; phases [1] | the two flag bytes are fine; move the blink arrays |
| hotkeys | `carViewHotkeys` 8011C530: `carViewHotkeyReleased[player]` | writes `padLayout[0..3]` as one word (1 = port 0 layout 1, port 1 layout 0) when no key is held: takes effect at the next `padLoadLayout` | writes `bitmapTrackingEnabled` | move; and disable the split toggle for 4 views |
| displayed speed | `hudSmoothSpeed(i)` 8011CBD0 (every frame, `worldUpdate` 8011A968); `hudResetSpeed(i)` 8011CBB0 (`humanCarsRestart` 8011A1E8) | **`humanCarTotal`**: the smoothed speed becomes the human count mid-loop; the restart loop zeroes it and stops | `arenaLoadedId`: the next `arenaSetup` reloads the arena | move `hudSmoothedSpeeds` |
| camera | `viewCarCameraUpdate(&humanCars[i], i)` 80102BE8 (in split screen for every human, else for `viewMainPlayer`); `carSetViewMode` 800E7380 (`carBindHumanSlot`); both call `viewSetCameraPose(player, ..)` 8011D388 | `viewCameraObjectTable[2]` = `aiBossDefeatedFlag`: 0 (skipped) or, after Dark Tooth falls, 1 -> pose written at address 1 + 0xA8.. | `arenaShortChaseRange`: nonzero on some arenas -> written as a pointer | views and camera parents for 2-3 (`FOUR_PLAYERS.md` 4.5) |
| camera object | `viewSetRotation` 801564D8 / `viewSetPosition` (`.., c->player`) | `viewCameraObjects[2]` = `viewEnvs[0]`: the camera's rotation and position are copied into **view 0's frame record** at +0xA0/+0xA8 | `viewEnvs[1]` | same |
| sky follow object | `viewSetFollowObjPos(target, i)` 801563BC, every human, every frame | `viewFollowObjects[2]` = `viewFov` (60 or 70): 12 bytes written at 70 + 0xA8, low RAM | `viewRenderSplitMode` (0: skipped; 1 or 2: low RAM) | same |
| lives | `gameGetLives(p)` 80129B20 -> `sessionSettings.lives[p]` (+0x14, 4 bytes) | `lives[2]`, fine; but `settingsClearOpponentCars` 80137864 writes 0xFF into +0x0A..+0x16, i.e. `lives[0..2]` | `lives[3]`, untouched | as `COOP_LIVES.md` 4: set all four after any clear; the runner's decrement of `lives[0]` only (8012C260) |
| win tallies | `sessionRun` locals `wins1`, `wins2`, `wins1b` | none | none | port |
| reset-weapons flag | `carResetWeapons[i]` (section 1.1) | spare byte | spare byte | fine in place; but see `uaInit` / `roundStart` in section 2 |
| start position | `humanCarPlaceAtStart` 800FD6C4 | random start node, not by index | | fine (`SIXTEEN_CARS.md` 5) |

CONFIRMED (code paths and aliases); the "leak" and the effects of the wild writes are LIKELY.

## 4. Object ids

A human's world object gets id `index + 1` (`carBindHumanSlot`, `carSlotObjects[id]->id = slot + 1`),
a computer car's `index + 50` (`carBindAiSlot` 8011A5CC). Human ids 3 and 4 therefore sit between 2
and 50. Every test found (CONFIRMED):

| Kind of test | Where (VA of the test) | Ids 3-4 |
|---|---|---|
| owner `< 50` = player (range, hit radius, turn rate, breakables, targets) | `projectileHitTest` 800D7618, 800D77C4; `bulletSpawn` 800D7D70; `bulletsMove` 800D7F98; `weaponFireFreezeMissile` 800D9B0C; `weaponProjectileUpdate` 800DB8C0; `weaponApplyHit` 800DC808; `carContactApplyObject` 800E08A8; `weaponFlameUpdate` 8010AAE8; `areaDamage` 8010BB80, 8010BBB4, 8010BD5C, 8010BD94; `groundFiresUpdate` 8010C424; `weaponRicochetUpdate` 8010CEA0; `weaponTornadoUpdate` 8010E950 | human, as wanted |
| hit object is a car: `(u32)(id - 1) < 99 && id != 8` | `weaponApplyHit` 800DC778 | car; id 8 is the only gap (`SIXTEEN_CARS.md` 1.5), reached at the 8th human |
| `id < 6000 && id != 8` | `carContactResolve` 800E0E9C, 800E0EB0 | car |
| `owner == 1` | `bulletSpawn`: a separate branch that sets life 12, as the `< 50` branch does | no difference |
| `hit == owner`, `S16(obj, 10) == -owner` | `bulletsMove` | by value, any id |
| id back to an index: `humanCarGet(owner - 1)` | `weaponDropCheckHit` 80109A9C (kind 19, the remote bomb: clears `drop_armed`, `held--`, `ammo[3]--`) | index 2-3: needs the 4 records |
| id search | `carFindById` 8011D2C8 (slot loop by `carWorldObjectCount`), `humanCarFindById` 8011D07C (record `id`, +0x002, = index) | fine |

Found by a scan of every `slti`/`sltiu` against 49..51 and 2..3 in the EXE, every `(x - 1) < 2 or 3`
range test, and every compare that follows a load of object +0x118. The other hits are car types
(`slti 49` / `113` switches in `hudRadarPlotCar`, `worldUpdate`, `carSoundUpdate`), pool sizes (50
pickups, 50 targets) and shake kinds. No test reads "id <= 2", "id < 3" or "id == 2" as "a human".
CONFIRMED.

The killer and last-attacker bytes (stats +0x153 / +0x154 on a human record) hold world-object
**slots**, not ids (`GAME_EVENTS.md` 0), so they are fine too.

**Since `HUMANS16_PLAN.md` step H0 (2026-10-14):** every writer of a human's id and the one reader that turns
an id back into an index use `players.h`'s `TM2_HUMAN_ID(i)` / `TM2_HUMAN_INDEX_OF_ID(id)` (`carBindHumanSlot`,
`UpdateGuns`, `weaponFireSpecial`, `weaponDropItem`, `weaponFireEnergyAttack`, `weaponDropCheckHit`, and the port's
`portCarBindSlot`, `portDropInId`, `portHumanCompact`). With up to 7 human records they are `i + 1` / `id - 1`, so
the 4-human variant above is unchanged; past 7 (`TM2_PORT_HUMANS` 8..16, since step H4) the ids skip 8 and 10. The port
turns an id into a player with `portHumanOfId(id)` (every port variant), never `id - 1`. `tools/check_humans.py`.

## 5. The slot binding and the settings

### 5.1 `carBindHumanSlot` (8011A3FC, scene_init.c) and `carBindHumanByType` (8011A29C)

`sessionBindCars(settings)` (8012BD94, screen_card_layout.c): `arenaSetup(level)`, then for `i <
settings->humans`: `carBindHumanByType(i, carIndexToType(settings, settings->human_car[i]))`, then
for `i < settings->opponents`: `carBindAiByType(i, carIndexToType(settings, settings->opponent_car[i]))`.

`carBindHumanByType(i, type)`: the first slot whose owner is `type` (loop on `carWorldObjectCount`),
`humanCarTypes[i] = type`, then `carBindHumanSlot(i, slot, type)`:

1. `humanCarObjects[i] = carSlotObjects[slot]`, `humanCarObjectSlots[i] = slot`,
   `carSlotIndices[slot] = i`, `humanCars[i].slot = slot`, `humanCarTypes[i] = type`,
   `humanCars[i].player = i`;
2. `carRestart(&humanCars[i], carSlotOwners[slot], carResetWeapons[i])` (800FD550: start node, stats,
   weapons, `PadGetConfig(player)`, `humanCarObject(player)`);
3. not attract: `carSetViewMode(rec, 2)` (-> `viewSetCameraPose(player, ..)`);
4. `carSlotKinds[slot] = 0` (human), object id = `i + 1`, `csSetDrawEnable(object, 1)`.

Nothing in it is bounded by 2. With moved arrays it binds human 2 and 3 as it binds 0 and 1. The
per-index pieces it touches are those of section 1 (objects, slots, types, reset flag, pad config,
camera parent). CONFIRMED.

`humanCarsSetCount(settings->humans)` (8012C054) must be 4, and `sessionLoadArenaAndCars` (8012BAFC)
loads one TMS and one DMD per human from `human_car[i]` (load groups 1..humans, then the opponents).
CONFIRMED.

### 5.2 Where cars 3 and 4 would go in the settings

`sessionSettings` (8016475C): `human_car[2]` at +0x08, `opponent_car[10]` at +0x0A, `lives[4]` at
+0x14, `control_layout[2]` at +0x26. In RAM only 0x2C bytes belong to it (`screenCardDrawEnvs` starts
at 80164788); `settingsCopy` copies 42 bytes. No word in data points at it; code uses `&sessionSettings`
(lui/addiu) or the pointer passed to `sessionRun`. CONFIRMED.

Readers of `human_car[i]` for `i < humans` (they would read cars 3-4 from +0x0A/+0x0B):

| VA | Function | Use |
|---|---|---|
| 8012BAFC | `sessionLoadArenaAndCars` | TMS and DMD files of each human (two loops) |
| 8012BD94 | `sessionBindCars` | bind each human |
| 8012CC10 | `carIsSelectable` | refuses a car already in a human slot (via `settingsGetHumanCar` 8013788C) |
| 801307EC | `screenCarSelect` | clears and fills `human_car[player]` for `player < humans` |
| 80136D98 | `screenEnding` | the ending movie of each human's car |
| 80137954 | `settingsCopy` | `list8` (human cars), `list20` (lives), `list38` (control layouts) by `humans` |
| (several) | `sessionRun`, `passwordBuildString`, `pauseMenu` | `human_car[0]` (and `[1]` in the Minion test) |

Readers and writers of +0x0A on as opponents:

| VA | Function | Use |
|---|---|---|
| 8012BAFC, 8012BD94 | `sessionLoadArenaAndCars`, `sessionBindCars` | `opponent_car[i]` for `i < opponents` |
| 8012A614 | `screenLoadCardImages` | card TIMs (`sessionGetOpponentCar` 8012CBAC) |
| 8012CC10 | `carIsSelectable` | via `settingsGetOpponentCar` 8013783C |
| 8012CD08 | `tournamentPickOpponents` | writes `opponent_car[0..]` (or slot 0 = 14 for Dark Tooth) |
| 80131EEC | `screenAiSelect` | clears slots `picked..8`, writes the picked ones |
| 801307EC | `screenCarSelect` | clears all 10 opponent slots |
| 80137864 / 801378B4 | `settingsClearOpponentCars` / `settingsClearHumanCars` | 13 bytes from +0x0A / +0x08 (into `lives`) |
| 8012CBC4 | `sessionSetOpponentCar` from `sessionRun` | boss reloads: `opponent_car[0] = 13` (Minion) or `14` (Dark Tooth), `opponents = 1` |
| 80137954 | `settingsCopy` | `list10` by `opponents` |

With retail's struct and `humans = 4`, cars 3-4 sit in `opponent_car[0..1]`, the opponent loops read
them as opponents too, `carBindAiByType` finds the same slot again (first slot of that type,
`carSlotKinds` now 0, so no `aiCarTotal++`) and rebinds it as computer car 0 or 1 (kind 1, id 50/51).
CONFIRMED (the code path); LIKELY (the outcome).

Two ways out (LIKELY, design):

- **Shift the opponent list.** Keep cars 3-4 at +0x0A/+0x0B and make every opponent reader above use
  `opponent_car[i + humans - 2]` when `humans > 2` (at most 8 opponents then). The boss reloads in
  `sessionRun` must write the boss after the human cars, and the clears must keep +0x0A/+0x0B.
- **A port field.** Keep `human_car[2]` for players 1-2 and store cars 3-4 in a port table; change
  only the human readers (`sessionLoadArenaAndCars`, `sessionBindCars`, `carIsSelectable`, the car
  select screen, `screenEnding`, `settingsCopy`) to take index 2-3 from it. The opponent side stays
  untouched. This is the smaller change.

Either way `control_layout[2..3]` (would be `cd_volume`/`sfx_volume`) and the lives clears
(`COOP_LIVES.md` 4) need port fields or care. `tournamentOpponentCount` (8012AFE8) counts humans in
its total; `tournamentPickOpponents` hangs when `opponents > 13 - humans` (`SIXTEEN_CARS.md` 1.2).

## 6. What makes a rebase unsafe

| Hazard | Found | Where |
|---|---|---|
| pointer to a record (or any array of this document) stored in data | **none** (`data_relocs.tsv`: no value in 801879FC..8018F458, 80180F94..80181034, 80180A6C..80180A84, 80180B9C..80180BA8, 801828D0..80183EE8, 801A4AEC..801A4DEC, 8016475C..80164788) | CONFIRMED |
| record address compared with a constant | **none**: no `beq`/`bne`/`slt(u)`/`subu` uses a register loaded from these bases | CONFIRMED (scan of every base `addiu`, 9 instructions on) |
| addends: the `%lo` of a pair often carries a field offset or record 1 | `humanCars` + 0x14 .. + 0xF18 (record 1 is reached as `humanCars + 0x814 + off`, e.g. 8011A884 `+0xA04`, 8011A898 `+0xF10`); `viewCameraObjectTable + 4` (gp, 801197B0, 801198F8); base + immediate through a register (`addiu s5,s4,1788`, `lw a3,3856(s4)`) | the move must keep `symbol + addend`, which a delta does; no addend leaves its array (largest `0xF18 < 0x1028`) | CONFIRMED |
| gp-relative accesses | `humanCarTotal` (16), `humanCarTypes[0]` (801197C0), `viewCameraObjectTable[0]/[1]` (4) | a recompiler that emits C just uses the new address; a patch in place cannot (gp reaches 801789D4..801889D3 only) |
| a size or end computed by hand | `carTablesClear` 801198AC `li v0,0x814` (start of the last record, count-down); `uaInit` `slti 2` (8011975C, 801197A0); `padInitDriver` 80150E3C `memset(padActiveBindings, 0, 0x100)` and `memset(padConfiguredBindings, 0, 0x100)` (`li a2,0x100` at 80150E54, 80150E68: sizeof the 2-entry arrays); `viewCreate` `slti 2` 801558A4 | change with the size |
| memcpy/memset over a whole array or across neighbours | `padInitDriver` (above); `padActivateBindings` 80150FC0 copies one 128-byte entry; `settingsClearOpponentCars` / `settingsClearHumanCars` write 13 bytes across `opponent_car`/`human_car` into `lives`; `settingsCopy` by the counts | the two settings clears are the only cross-field writes; nothing copies the human records as a block (`mathCopyWords` / `memcpy` users copy fields: matrices, positions) |
| code reaching a neighbour through an index past the end | `padCheckAction(.., 2)` -> `padActiveBindings[2]` = `padConfiguredBindings[0]` and `padActionCache[a][2]` = `[a + 1][0]` (retail, by design or by luck: menus use port 2); every index-2/3 alias of sections 1 and 3 when 4 humans run on unmoved arrays | move `padActiveBindings` and `padConfiguredBindings` together (or keep the `[2]` = configured-[0] meaning); move all of section 1.1 together |
| runtime pointers into the records | effect lists and sound objects keep pointers into a record at run time (e.g. `fxSmokeA24(humanCars[i].smoke)`, positions for the sound locate) | safe when the move is made in the recompiled image before it runs; not safe if arrays were moved during a match |
| other modules reaching the records | none directly: outside 80119698..80120664 every user goes through `humanCarGet`, `humanCarObject`, `humanCarGetPos/Rot`, `carFromSlot`, `aiGetTargetCar` (`SIXTEEN_CARS.md` 2.1; no getter address in data) | the getters' own pairs are in the 34 functions |

So a rebase is safe when it moves, as one block or each by its own delta, every array of section 1.1
(except the dead `D_80180FD8`/`D_80180FE0`) and the per-view arrays of section 1.2 that a human loop
reaches (`viewFollowObjects`, `viewCameraObjects` with `viewEnvs`/`viewModes`,
`viewCameraObjectTable`), and fixes the hand-written bounds above. CONFIRMED for what is listed; that
nothing else exists is CONFIRMED for data words and address compares, and LIKELY for index-past-end
reads in code not traced here (HUD and views are covered by `FOUR_PLAYERS.md`).

## 7. The minimal change list for 4 human records (LIKELY, design)

1. Move and size for 4: `humanCars`, `humanCarObjects`, `humanCarObjectSlots`, `humanCarTypes`,
   `padConfigs`, `hudSmoothedSpeeds`, `carViewHotkeyReleased`, `viewCameraObjectTable`,
   `viewFollowObjects`, `viewCameraObjects`, `viewEnvs`, `viewModes` (and the rest of
   `FOUR_PLAYERS.md` 4.4 / 5.1 for four views and HUDs). `carResetWeapons`, `hudRadarVisible`,
   `hudWeaponListVisible`, `padLayout` already have 4 bytes.
2. Bounds: `uaInit` loops to 4 (or a port init for 2-3); `carTablesClear` start offset `0x183C`;
   `roundStart` and `sessionRun` reset flags for 2-3; `viewCreate`'s guard and the view loops.
3. Fixed pairs: the radar pair and the AI blips per view (`worldUpdate`); the round-end test over all
   humans; a deathmatch rule for 3-4 (results 3/4 of `humanCarDamageUpdate`); win tallies per player;
   `weaponFindTargetAhead/Behind` aiming at the shooter's target, not player 1.
4. Input and sound: the port's pad read for players 3-4 (not `padCheckAction(.., 2)`, which is "either
   pad", nor 3, which is nothing); no `padSetLayout` for ports 2-3; sound loops for 2-3 on their own
   lists or the shared loops (side 2-3 leaks pool objects).
5. Settings: cars 3-4 in a port table (or the shifted opponent list), their layouts and lives in port
   fields (section 5.2).
6. Ids, pair and slot tables: nothing to change.

## 8. The port variant (request 25)

`build.py --source-only --relocatable --port-variant` builds this document's change list as a separate,
relocatable variant (`-DTM2_PORT_VARIANT`; the retail builds are unchanged). `docs/SOURCE_BUILD.md`,
"Port variant", has the details, the proofs and the outputs (`out/TM2_SOURCE_PORT.*`).

| Section 7 item | In the variant |
|---|---|
| 1. Move and size for 4 | **Done.** Every array of 1.1 (except the dead `D_80180FD8`/`D_80180FE0`) and the per-view and per-HUD arrays of `FOUR_PLAYERS.md` 4.4 / 5.1, sized by `TM2_MAX_HUMANS/VIEWS/HUDS`, at the port base (default 0x80200000, `.port_bss`/`.port_data`). The frame records, OTs and pad arrays are not moved. |
| 2. Bounds | **Done:** `uaInit`, `carTablesClear` (count-down from `(N - 1) * 0x814`), `roundStart`, the `sessionRun` reset pairs, `viewCreate`'s guard and the view loops. |
| 3. Fixed pairs | **Radar only:** the human radar pair and the co-op AI blips for N humans. Not done (rules, the port's side): the round-won test, deathmatch results 3/4, the win tallies, `weaponFindTargetAhead/Behind`. |
| 4. Input and sound | **Not done** (the port's side): pads 3-4, `padSetLayout`, the per-player sound loops (sides 2-3 still leak pool objects). |
| 5. Settings | **Cars done:** cars 3-4 in the port table `sessionPortHumanCars`, read and written through the settings accessors, `sessionLoadArenaAndCars` and `sessionBindCars`; `opponent_car` is no longer aliased. Not done: layouts and lives of players 3-4. |
| 6. Ids, pair and slot tables | Nothing to change. |

With one or two humans the variant follows retail's game logic, `rand()` calls included. Two things
differ: the array addresses, and the setup loops, which also set the entries of players, views and
HUDs 3-4.

**Since `HUMANS16_PLAN.md` step H1 (2026-10-14, `--port-define TM2_PORT_HUMANS`):** section 3's "camera",
"camera object", "sky follow object", "HUD records", "HUD state" and "hotkeys" rows no longer use the human index:
a local-player map (`portHumanLocal`, `include/semantic/port_humans.h`) gives each human a local slot (0..3) or
none (remote). Cameras, the sky follow, the view calls of `carSetViewMode`, the hotkeys and their HUD toggles, the
radar blips and the HUD's car, speed, lives and target panel follow the slot; a remote human has none of them on
this machine. The default map (all zero) is retail's identity for humans 0..3. Sound sides still go by index
until step H3.

**Since step H2 (2026-10-14):** section 3's "pad reads" and "cheat combos" rows no longer use the human index as a
pad port: a human on local slot 0 or 1 reads pad 0 / 1 (retail's calls), every other human (local slots 2-3,
remote humans, and everyone with `portHumanOptions` bit 0) its own input word `portHumanPad[h]` with its own
layout (`portHumanLayout` / `portHumanBindings`), which replaces the port's own read of pads 3-4
(`HUMANS16_PLAN.md` H2 "As built").

**Since step H3 (2026-10-15):** section 3's sound rows: the turbo / gun / burn loops go by the human's sound side
(local slot 0 or 1), not its index; humans without a side no longer leak a sound request each frame
(`HUMANS16_PLAN.md` H3 "As built").

**Since step H4 (2026-10-15, `--port-define TM2_PORT_HUMANS=N`, N 4..16):** the variant can hold up to 16 human
records (at most 4 of them local). Every per-human array of section 1.1 is `[TM2_MAX_HUMANS]` in port storage;
`carResetWeapons`, which used the 2 bytes of padding after retail's u8[2] for players 3-4, moves to `.port_bss`
past 4 humans; section 5's settings: cars 3.. stay in `sessionPortHumanCars` (N - 2 entries), the layouts and lives
of every human are the port's `portHumanLayout` / `portHumanLives` (H2), and `settingsCopy` (the attract demo's
save / restore) copies only the record's own entries past 4 humans: retail's loops run to the human count past
each list and from 7 humans on wrote past the record (into `screenCardDrawEnvs`). With N = 4 (and without the
define) nothing above changes: the images are H3's (`HUMANS16_PLAN.md` H4 "As built").
