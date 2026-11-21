# Sixteen cars in a match, and more than 2 MB

Answers sections 1-3 of item 12 in the port's `docs/DECOMP_REQUESTS.md`
(16 cars for Challenge Match and 16-player online; 9 opponents on every
arena as a first step). Static analysis only: the C in `src/semantic`,
checked against the retail disassembly (`bin/SCUS_94306R.EXE`, disassembled
with `toolchain/mips/bin/mipsel-none-elf-objdump`) and the disc
(`cdimage/`, read only). Nothing was run. Addresses are retail. Memory
layout: `config/US/layout.tsv` and the symbol files
`config/US/undefined_syms_*.txt`.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code, not traced to the end or not seen running.

The census was made two ways: every `lui`/`addiu`/load/store pair in the EXE
that lands in a car array (an `addu` of an index between them is followed),
and every `slti`/`sltiu`/`li` with 2, 3, 9, 10, 11, 15 or 16 in a function
that touches a car array, a car count or a car getter. The hits were then
read in the C.

## Short answer

- **Three counts, four fixed sizes.** Loops over cars use the counts
  `humanCarTotal` (80181014, gp+0x640), `aiCarTotal` (8018101C, gp+0x648)
  and `carWorldObjectCount` (80180FE8). The arrays are fixed at 2 human
  records, 9 AI records, 15 car world objects and 15 x 15 car pairs. Only a
  few loops use a constant: 2 (humans, in `uaInit` and `roundStart`), 9 (AI
  tables, in `uaInit`, `carTablesClear`, `arenaSetup`, `aiCarsSetActive`,
  and the clamp in `aiCarGet`) and 15 (`carRegisterModel`,
  `carTablesClear`). CONFIRMED.
- **The real limit is 15 car types.** The car world objects are registered
  and found **by car type** (0x10..0xF0), not by car. So are the body,
  wreck, damage-stage, tire, turbo and freeze objects (tables of 15 or 16,
  index `type >> 4`). The game only ever has one car of each type: every
  picker refuses a car already taken. A 16th car must share a type with
  another one, and then both bind to the same world object. 16 cars need
  these registries keyed by car (the load group n) instead of by type.
  CONFIRMED.
- **Records:** found through the getters in 72 functions, and by direct
  address in 50, all in one block of the EXE (80119698..80120664, the
  car-record module from `uaInit` to the attract-mode spawns; the slot and
  pair tables add 16 more functions in 8011961C..8011EBE0). No pointer to a car
  array is stored in data, and none is gp-relative. In the C every use goes
  through the symbols `humanCars`, `aiCars` etc. (absolute symbols in
  `config/US/undefined_syms_auto.txt`), so moving an array is a change of
  one symbol plus the constants listed in section 1. CONFIRMED.
- **Load area:** base 0x80019C40 is an immediate in 9 places of
  `sessionLoadArenaAndCars`. There is no end constant and no size check;
  the end is the EXE start 0x800CDC54 (737,300 bytes). The level DMD is not
  relocated (its pointers are absolute, read from fixed addresses by
  `worldLoadStartObjects`); each car DMD is relocated by
  `assetInstallImage` and can go anywhere. CONFIRMED.
- **Easiest move:** leave the level DMD at 0x80019C40 and load only the car
  DMDs from 0x80200000 (two `lui s6,0x8001` sites and one `addu`, section
  3.5). Nothing else in the game reads the car DMDs by address. Nothing
  else may move: 0x80010000..0x80019020 holds the sound bank header that
  libsnd keeps using, and the stack top is 0x80200000. CONFIRMED.
- **9 opponents on every arena:** besides memory, the opponent count comes
  from one table, `tournamentOpponentCount` (8012AFE8), which caps every
  mode by arena (6 to 10 cars, humans included). The start points, the VRAM
  and the 9 opponent cards (sections 4-6) are the other limits. The
  records, objects and pair tables already hold 2 + 9 = 11 cars.
  CONFIRMED.
- **VRAM (section 4):**
  - **Where car textures go:** only in the lower half of VRAM. A second car
    of the same type costs nothing, because duplicate images are reused.
  - **Room per arena:** 8 different normal car types fit on every arena.
    All 14 are *placed* on Moscow, Holland, Cyburbia and Roof Tops, but
    retail's CLUT scan tests only one texel, so from the 9th-12th type on a
    256-colour CLUT lands on texels already in use (4.4,
    `RETAIL_BUGS.md` 27). With the port's `portCarOptions` bit 9
    (`TM2_PORT_CAR_CLUTS`) all 14 fit cleanly there and on Amazonia,
    Antarctica and New York 2P too.
  - **Dark Tooth:** he alone needs as much room as about 7 normal cars.
  - **Texture slot table:** it steps by 20 per car, but Dark Tooth has 31
    images, so he must load last (retail does). LIKELY (a replay of the
    placement code on the disc files).
- **Start points (section 5):** each arena has 8 to 15. When all are taken,
  extra cars share a random one; the humans end up sharing, because they
  pick last. The doc gives a farthest-free picker and extra points per
  arena. CONFIRMED.
- **Opponent cards (section 6):** they only appear in tournaments.
  - **Arrays that need to grow:** the sprite array (9 cards + 1 template),
    the 9 x 9 layout table (the settings struct follows it) and the card
    palettes. The palettes sit at (768, 128+i) and would run into the card
    art above 9 cards.
  - **For 16 cards:** the doc suggests a 4 x 4 layout. CONFIRMED.

## 1. Census

### 1.1 The arrays and what follows them

All are BSS (zero at boot) unless the address is in sdata (gp-relative,
801809D4..80180EE4 and the sbss after it). "Next" is the symbol right after
the array: what a larger array in place would overwrite. Every array below
is packed against its neighbour, so none can grow in place.

| VA | Name | Size | Indexed by | Next |
|---|---|---|---|---|
| 801879FC | `aiCarTargets` | 9 x s32 | AI index; value = AI index, or -1 = nearest human | `carSlotKinds` 80187A20 |
| 80187A20 | `carSlotKinds` | 15 x s32 (0 human, 1 AI, 2 loaded but free) | car object slot | `carSlotIndices` 80187A5C |
| 80187A5C | `carSlotIndices` | 15 x s32 (index in its record array, -1 free) | slot | `carSlotOwners` 80187A98 |
| 80187A98 | `carSlotOwners` | 15 x s32 (the car **type**) | slot | `carSlotObjects` 80187AD4 |
| 80187AD4 | `carSlotObjects` | 15 x world object pointer | slot | `humanCars` 80187B10 |
| 80187B10 | `humanCars` | 2 x 0x814 | human index | `aiCarObjects` 80188B38 |
| 80188B38 | `aiCarObjects` | 9 x object pointer | AI index | `aiCars` 80188B5C |
| 80188B5C | `aiCars` | 9 x 0x854 (ends 8018D650) | AI index | `aiCarObjectSlots` 8018D650 |
| 8018D650 | `aiCarObjectSlots` | 9 x s32 | AI index | `carPairVectors` 8018D674 |
| 8018D674 | `carPairVectors` | [15][15][3] s32, 2,700 bytes | slot, slot | `carPairHeadingDeltas` |
| 8018E100 | `carPairHeadingDeltas` | [15][15] s32 | slot, slot | `carPairDirections` |
| 8018E484 | `carPairDirections` | [15][15][3] s32 | slot, slot | `carPairHeadings` |
| 8018EF10 | `carPairHeadings` | [15][15] s16 (+2 pad) | slot, slot | `carPairDistances` |
| 8018F0D4 | `carPairDistances` | [15][15] s32 | slot, slot | `screenTimInfo` 8018F458 |
| 80180FE8 | `carWorldObjectCount` | s32 (sdata) | | `humanCarObjects` |
| 80180FEC | `humanCarObjects` | 2 x object pointer (sdata) | human index | `humanCarObjectSlots` |
| 80180FF4 | `humanCarObjectSlots` | 2 x s32 (sdata) | human index | `padConfigs` 80180FFC |
| 80180FFC | `padConfigs` | 2 x s32 (sdata) | player | `humanCarTypes` 80181004 |
| 80181004 | `humanCarTypes` | 2 x s32 (sdata) | player | `hudSmoothedSpeeds` 8018100C |
| 8018100C | `hudSmoothedSpeeds` | 2 x s32 (sdata) | player | `humanCarTotal` 80181014 |
| 80181014 / 8018101C / 80181020 | `humanCarTotal` / `aiCarTotal` / `aiCarsRemaining` | s32 (sdata) | | |
| 80181024 | `viewCameraObjectTable` | 2 x pointer (sdata) | view | `aiBossDefeatedFlag` 8018102C |
| 80180FA0 | `carResetWeapons` | u8 per human (room for 4) | player | `viewWatchingAi` 80180FA4 |

Arrays keyed by car type (index `type >> 4`, 1..15), section 1.3:

| VA | Name | Size | Next |
|---|---|---|---|
| 80187344 | `carPartsTireTurnAccum` | 15 x s32 | `lightFlickerTimers` 80187380 |
| 801873D0 | `carBodyObjects` | 15 x pointer | `carWreckModels` |
| 8018740C | `carWreckModels` | 15 x pointer | `worldCollapseSections` 80187448 |
| 8018F4E4 | `carDamageStageObjects` | 16 x pointer | `carTireSwitchObjects` |
| 8018F524 | `carTireSwitchObjects` | [16][3] pointer | `carTurboObjects` |
| 8018F5E4 | `carTurboObjects` | 16 x pointer | `D_8018F624` |
| 8018F624 | `D_8018F624` (second per-type anim object) | 16 x pointer | `objRegPedestrians` 8018F664 |
| 8018F950 | `carFreezeObjects` | 16 x 264 bytes | (inside to 801909D0) |
| 801872FC.. | `carPartsHammerhead`, `carPartsMrSlam`, `carPartsMinion`, `carPartsDarkTooth`, `carAxelObjects` (80180F50) | one set each | |

Arrays per local player or view (2), section 1.4:

| VA | Name | Size |
|---|---|---|
| 801828F0 | `hudPlayerRecords` | 2 x 1,900 bytes of HUD primitives |
| 801828D0 | `hudOrderTables` | [2][2][2] |
| 801837C8 | `hudRadars` | 2 x 880 bytes (blip lines) |
| 80180A6C | `hudRadarVisible` (and `hudWeaponListVisible` after it) | per player |
| 80181110 / 80181118 / 80181120 | `viewCameraObjects` / `viewEnvs` / `viewModes` | 2 each |
| 80180B9C | `carViewHotkeyReleased` | 2 x s32 |
| 801A4BEC / 801A4CEC | `padActiveBindings` / `padConfiguredBindings` | 2 x 0x80 |
| 80180FD8 / 80180FE0 | `D_80180FD8[2]`, `D_80180FE0[2]` (per-player counters, zeroed by `uaInit`) | 2 each |
| 80180A06 / 80180A07 | `soundLoop32ActiveP1` / `P2` (engine loop per player) | 1 byte each |
| 8016475C | `sessionSettings`: `human_car[2]` +0x08, `opponent_car[10]` +0x0A, `lives[4]` +0x14 | 48 bytes; next 80164788 |

### 1.2 Loops, fixed indexes and bounds

"Count" = bounded by `humanCarTotal`, `aiCarTotal` or `carWorldObjectCount`
(directly or through `humanCarCount` 8011CEDC, `aiCarCount` 8011D0E0,
`carObjectCount` 8011CED0). Such a loop needs nothing but larger arrays.
"Fixed" = a constant in the code.

**Setup and tables** (all CONFIRMED)

| VA | Function | File | Bounds | How | For 16 cars |
|---|---|---|---|---|---|
| 8011961C | `carRegisterModel` | ai_route_helpers.c | registers each car world object into the 4 slot tables | fixed: `carWorldObjectCount < 15` (`slti 15` 80119628); a 16th is dropped silently | raise to N; grow the 4 slot tables |
| 80119848 | `carTablesClear` | ua_init.c | clears slot kinds/indices; `aiCarObjectSlots`; `humanCars[i].f_810`; `aiCars[i].f_850` | fixed 15 (8011987C), 9 (801198A0), 2, 9 | new sizes |
| 80119698 | `uaInit` | ua_init.c | per-human init (reset-weapons flag, record id = i, +0x1F4, PadSetConfig, `D_80180FD8/FE0`), `viewCameraObjectTable[0..1]`, `aiCarTargets` | fixed 2 (8011975C, 801197A0), 9 (8011980C) | new sizes |
| 8011995C | `arenaSetup` | arena_setup.c | `carChoiceRegisterGroup` per slot; `aiCarTargets` = -1 | count; fixed 9 (80119AA4) | new size |
| 8011A024 | `roundStart` | scene_init.c | `carSetResetWeapons(0, 0)` and `(0, 1)`; AI loop; `aiCars[0].type` boss test | fixed humans 0 and 1; count | loop over humans |
| 8011A198 | `humanCarsRestart` | arena_setup.c | restart every human | count | none |
| 8011A268 | `humanCarsSetCount` | global_accessors.c | sets `humanCarTotal` (from settings +4 in `sessionRun` 8012C054, 1 in `uaInit`) | | none |
| 8011A29C / 8011A54C | `carBindHumanByType` / `carBindAiByType` | world_object_register.c / ai_route_helpers.c | finds the slot whose owner is the car **type** (first match) | count | key by car, not type (1.3) |
| 8011A3FC | `carBindHumanSlot` | scene_init.c | human i -> slot; object id = i + 1 | by index | ids (1.5) |
| 8011A5CC | `carBindAiSlot` | car_parts_spin.c | AI i -> slot; object id = i + 50; `aiCarTotal++` in the caller | by index | ids (1.5) |
| 8011A3A0 | `aiCarsSetActive` | misc_one_liners.c | byte +0 of `aiCars[0..8]` | fixed 9 (loop offset 8 x 0x854 down to 0) | new size |
| 8011D0EC | `aiCarGet` | car_records.c | returns `&aiCars[i]`, **i >= 9 gives car 0** | fixed (`sltiu 9` 8011D0EC) | raise the clamp |
| 8011CEE8 | `humanCarGet` | car_records.c | `&humanCars[i]`, no check | | none |
| 8011CB50 / 8011CB70 | `humanCarObject` / `aiCarObject` | car_object_lookup.c | object of human i / AI i; the AI one compares with `aiCarTotal` but only for a dead read | none | none |
| 8011CFD0 | `carFromSlot` | car_records.c | slot -> record via kind and index | by slot | none |
| 8011D15C, 8011D1CC, 8011D2C8, 8011D8F4 | `carObjectFromOwner`, `carFindByObject`, `carFindById`, `carFindSlotByOwner` | car_records.c, arena_setup.c | search the slots | count | none |
| 8011D280 | `carObjectId` | car_records.c | id of human/AI i's object | by index | none |
| 8011D98C / 8011DA1C | `carSaveNodeState` / `carRestoreNodeState` | game_misc_accessors.c | one car by index | by index | none |
| 8012BD94 | `sessionBindCars` | screen_card_layout.c | binds settings +4 humans and +5 opponents (`human_car[i]`, `opponent_car[i]`) | settings counts | settings arrays (1.4) |

**Per frame** (CONFIRMED)

| VA | Function | File | Bounds | How | For 16 cars |
|---|---|---|---|---|---|
| 8011A700 | `worldUpdate` | world_update.c | human update, taunt sound and radar between `humanCars[0]` and `[1]`, sound, damage, HUD speed, camera target; AI update; AI pack logic (nested i, j over AI cars); AI radar blips for `humanCars[0]` and, in split screen, `[1]`; round end | counts; **fixed** humans 0 and 1 for the radar pair and the round-end test (`humanCars[0]` dead only) | radar per local player; round end over all humans |
| 8011A700 | (same) | | AI pack: `closer < 3`, `hunting >= 4 or near_count >= 4` pick one AI to attack | fixed thresholds | LIKELY tune: with 15 AI more of them qualify |
| 8011B89C / 8011BBD4 | `humanCarDamageUpdate` / `aiCarDamageUpdate` | car_damage_human.c / world_collapse_warps.c | one car by index; Paris event when `aiCarsRemaining < 3` | by index | none |
| 8011B7D8 | `aiCarShouldUpdate` | fx_explosions.c | one AI car | by index | none |
| 8011C248 | `aiCarsCountRemaining` | scene_init.c | alive AI into `aiCarsRemaining`; `aiCars[0]` boss test | count | none |
| 8011E16C | `aiCarsCountAlive` | arena_setup.c | alive AI (HUD "Enemies: N") | count | none |
| 8011C8E4 / 8011C98C | `humanCarNearest` / `aiCarNearestAlive` | car_records.c | nearest car | count | none |
| 8011CF0C | `aiGetTargetCar` | car_records.c | AI i's target: `aiCars[aiCarTargets[i]]`, or nearest human when -1 | by index | none |
| 8011D528 | `aiApplyDifficulty` | car_records.c | speed factor of every AI | count | none |
| 8011E220 | `carsPhysicsUpdate` | car_physics.c | contacts, collision, response for every car (6 contact points each, inside the record) | count | none |
| 8011E7D8 | `carPairsUpdate` | screen_deathmatch_result.c | every ordered pair of slots into the 5 pair tables | count, but tables are [15][15] | tables [N][N] |
| 8011EB18, 8011EB4C, 8011EB78, 8011EBAC, 8011EBE0 | `carPairVector`, `carPairHeadingDelta`, `carPairDirection`, `carPairHeading`, `carPairDistance` | game_misc_accessors.c, misc_one_liners.c | read one pair | **fixed row stride 15** (`i * 15` as shift-subtract) | new stride |
| 8011CBD0 / 8011CD08 | `hudSmoothSpeed` / `carGetDisplaySpeed` | car_records.c | `hudSmoothedSpeeds[player]` | by player | per local player |
| 8011DB74 | `carSoundUpdate` | screen_card_draw.c | engine/skid sounds per car; the looping sound 32 is per player 0/1 (`soundLoop32ActiveP1/P2`) | by slot; fixed players 0/1 | per local player |
| 800FC928 | `hudRadarPlotCar` | hud_radar.c | one blip into `hudRadars[view]`; refused once `count + 2 >= 25` (12 blips) | fixed 25 | LIKELY raise the blip buffer (more cars than 12 drop off the radar) |
| 8011DAF8 / 8011DB48 | `viewWatchNextAiCar` / `viewStopWatchingAi` | view_watch_ai.c | view 7 cycles AI cars | count | none |

**Weapons, damage, lock-on, world objects** (CONFIRMED): these loop with
`humanCarCount()` / `aiCarCount()` and fetch cars with `humanCarGet`,
`aiCarGet`, `humanCarObject`, `aiCarObject`; nothing to change but the
`aiCarGet` clamp.

| VA | Function | Uses |
|---|---|---|
| 800D6FD8 | `projectileHitTest` | humans and AI by count |
| 800DB110 | `weaponProjectileUpdate` | humans by count |
| 800DC65C | `weaponApplyHit` | `carFindById`; ids (1.5) |
| 800E0C28 | `carContactResolve` | `carFindByObject`; id 8 test (1.5) |
| 800E5628 | `carApplyDamage` | `carFromSlot`, `carFindById`, `aiCarsCountAlive` |
| 800EC8A0 | `carNearestToHuman` | humans by count |
| 800F20C8 / 800F2420 | `worldSectionDisable` / `aiCarsRerouteFromSection` | humans / AI by count |
| 800F5498 / 800F59A4 | `weaponSlamFindTarget` / `weaponSlamCarryTarget` | both by count |
| 80103FB8 | `weaponFireSelected` | `carPairDistance`, `carPairHeading`, `aiGetTargetCar` (lock-on by slot) |
| 80105A7C | `weaponFireSpecial` | `carObjectCount` loop + `carFromSlot`, `carPairDistance` |
| 801075C8 / 80107B00 | `weaponTaserTarget` / `weaponLightningScan` | both by count |
| 80109F94 / 8010A2AC | `weaponFindTargetAhead` / `weaponFindTargetBehind` | lock-on, both by count |
| 8010B5C4 | `areaDamage` | both by count, `carObjectId` |
| 8010C160 | `groundFiresUpdate` | both by count |
| 8010CC54 | `weaponRicochetUpdate` | both by count |
| 8010DE54 | `weaponInvisibilityUpdate` | humans by count |
| 8010E040 | `weaponTornadoUpdate` | both by count |
| 8010EF44 | `weaponHomingLeapInit` | both by count |
| 801163B8 | `worldCollapseUpdate` | humans by count |
| 80118B20, 80118BBC, 80118F38 | `trackVehiclesShouldStop`, `trackVehicleGround`, `trackVehicleDestroy` | humans by count |
| 80126464 | `targetActorsUpdate` | both by count (pedestrians) |
| 800E7EDC.. | AI driving (`aiCarDrive`, `aiMovementUpdate`, 13 more) | `aiGetTargetCar` only |

**Attract mode only** (CONFIRMED): `demoRespawnAiCars` (8011EFDC,
spawn_tables.c) sets `aiCarTargets[0..3]` and picks one of 4 spawn sets
(`demoSpawnSlotsUsed[4]`); the eight `demoSpawn<Arena>` functions
(8011F284..801203EC) write fixed spawn and target nodes into `aiCars[0..3]`
and `humanCars[0]`. `shellAttractDemo` uses 1 human and 4 opponents. These
are the only "spawn tables" in the EXE; normal starts use the start nodes
(section 5). `demoWatchNextAiCar` (8011EDD0) moves the camera of
`humanCars[0]`. Outside attract mode `aiCarTargets` stays -1, so every AI
car hunts the nearest human; AI-against-AI only happens in the demo.

**Pickers and settings** (CONFIRMED)

| VA | Function | File | Bounds | How | For 16 cars |
|---|---|---|---|---|---|
| 8012AFE8 | `tournamentOpponentCount` | tournament_tables.c | cars per arena, humans included: LA 7, Moscow 6, Paris 7, Amazonia 7, New York 7, Antarctica 8, Holland 10, Hong Kong 9, others 0 | table | section 3.6 |
| 8012CC10 | `carIsSelectable` | car_is_selectable.c | refuses a car already in a human or opponent slot | settings counts | allow repeats only after 1.3 |
| 8012CD08 | `tournamentPickOpponents` | tournament_pick_opponents.c | random distinct cars 0..12 | settings +5 | **loops forever** if +5 > 13 - humans |
| 80131EEC | `screenAiSelect` | screen_ai_select.c | picks until +7 = +5; clears slots `+7..8` | fixed 9 (`slot < 9`) | new size |
| 8016475C | `sessionSettings` | settings.h | `opponent_car[10]` then `lives[4]`; `settingsClearOpponentCars` writes 13 bytes | fixed | move to a larger struct |

### 1.3 Car types are the real limit of 15

The car DMDs each hold one root whose object id is the car type (16, 32 ..
240; the disc's car DMDs have count 1, Dark Tooth's BB.DMD 3). Its parts
use ids type + 1..15 (for example type + 6 is the turbo object in
`objRegistryAdd`). `worldLoadObject` (800D1DB0) sends the 15 type ids to
`carRegisterModel`, which stores the type in `carSlotOwners`. CONFIRMED.

`carBindHumanByType` and `carBindAiByType` look the type up with
`carFindSlotByOwner` (8011D8F4), which returns the **first** slot with that
type. Two cars of one type therefore bind to the same world object; the
second AI is bound again without `aiCarTotal++` (the `carSlotKinds != 2`
branch at 8011A54C). CONFIRMED.

These registries are also one entry per type, filled by each DMD as it is
installed, so a second DMD of the same type overwrites the first car's
entries (LIKELY: the first car would lose its body/wreck/damage looks):

| VA | Function | Table | Key |
|---|---|---|---|
| 80114EEC | `carBodyObjectRegister` | `carBodyObjects[15]` | `id >> 4 < 15` |
| 801152A8 | `carWreckModelRegister` | `carWreckModels[15]` | `id >> 4 < 15` |
| 80114B70 | `carPartsAnimate` | `carPartsTireTurnAccum[15]` | `kind >> 4 < 15` |
| 801246A8 / 80124718 | `carChoiceRegisterGroup` / `carSetDamageStage` | `carDamageStageObjects[16]` | `id >> 4 < 16` |
| 8012478C / 80124800 | `carRegisterTireSwitch` / `uaswToggleCarTireState` | `carTireSwitchObjects[16][3]` | group < 16 |
| 80123F78 / 80124384 | `objRegistryAdd` / `objRegistrySetStep` | `carTurboObjects[16]`, `D_8018F624[16]` | `id >> 4` |
| 80124C40 / 80124B80 | `carFreezeBuild` / `carFreezeResetAll` | `carFreezeObjects[16]` | `id >> 4 < 16`; reset loops 16 |
| 80114380 | `carPartsReset` | `carBodyObjects`, `carWreckModels` | loop 15 |
| (several) | Hammerhead, Axel, Mr. Slam, Minion, Dark Tooth part tables | one set per car | |
| 800FC928 | `hudRadarPlotCar` | blip colour by type | |

The game never needs more: there are 15 types, the card and select
screens refuse a car already taken, and `carRegisterModel` stops at 15. A
16-car match needs at least one repeated type, so the port must key these
by car. The natural key is the load group n that `sessionLoadArenaAndCars`
already passes to `bitmapParseContainer` and `assetInstallImage` (1 = first
human, then the AI cars; `vramTexSlotGroup` holds it during the install).
LIKELY (design suggestion).

### 1.4 Per-player arrays (2)

The HUD, views, cameras, pads, rear view, hotkeys, displayed speed and the
engine loop sound are per **local** player: `hudPlayerRecords[2]`,
`hudRadars[2]`, `viewCameraObjects[2]`, `viewCameraObjectTable[2]`,
`padConfigs[2]`, `padActiveBindings[2]`, `carViewHotkeyReleased[2]`,
`hudSmoothedSpeeds[2]`, `soundLoop32ActiveP1/P2` (table 1.1). For online
play each guest still shows at most two, so these can stay at 2 as long as
the remote humans' records do not use their player index to reach them.
`humanCars[i].player` (+0x00) is that index (`carBindHumanSlot`).
CONFIRMED for the arrays; the online use is a suggestion.

Arrays that do grow with the number of human cars: `humanCars`,
`humanCarObjects`, `humanCarObjectSlots`, `humanCarTypes`,
`carResetWeapons` (room for 4), `sessionSettings.human_car[2]` and
`lives[4]`, and the fixed humans 0/1 in `worldUpdate`, `roundStart` and
`uaInit`.

### 1.5 Object ids

| Car | Id written to object +0x118 | By | Conf. |
|---|---|---|---|
| human i | i + 1 | `carBindHumanSlot` | CONFIRMED |
| AI i | i + 50 | `carBindAiSlot` (8011A5CC) | CONFIRMED |

Readers: an owner id below 50 is a human (`weaponApplyHit` 800DC808,
`weaponRicochetUpdate`, `weaponTornadoUpdate`; also `slti 50` in
`projectileHitTest`, `bulletSpawn`, `bulletsMove`,
`weaponFireFreezeMissile`, `weaponProjectileUpdate`,
`carContactApplyObject`, `weaponFlameUpdate`, `areaDamage`,
`groundFiresUpdate`, LIKELY the same test). A hit object is a car when its
id is 1..99 and **not 8** (`weaponApplyHit` 800DC778:
`(u32)(type - 1) < 99 && type != 8`; `carContactResolve`: `id < 6000 &&
id != 8`). CONFIRMED.

So 16 AI cars (ids 50..65) fit as they are. Up to 49 humans fit below 50,
but **human 8 would get id 8**, which those two tests do not treat as a
car: give that car another id (for example skip 8). CONFIRMED (the tests);
what id 8 is reserved for was not traced.

### 1.6 Pools that more cars fill (LIKELY)

Not sized by the car count, but shared by all cars:
- coordinate systems: 128 (`csObjectPool` 8019A950, `csAlloc` 8014952C,
  "too many coordinate systems"); each car takes at least one, more with
  parts;
- sound instances: 35 (`soundInstFreeCount`, 35 x 120-byte `sndObjPool`
  slots built in `main`);
- radar blips: 12 per view (1.2);
- weapon pools: 4 shield records, 16 projectile records, 25 weapon-slot
  records, 150 effect sprites (`docs/COOP_LIVES.md` 1.2);
- the per-frame pair update is n x n (`carPairsUpdate`): 256 pairs at 16
  cars against 121 today.

### 1.7 What a 16-car build needs (summary)

1. New arrays above 0x80200000 (or anywhere free) for `humanCars`, `aiCars`,
   `aiCarObjects`, `aiCarObjectSlots`, `aiCarTargets`, the four slot tables,
   the five pair tables, and the sdata human arrays
   (`humanCarObjects`, `humanCarObjectSlots`, `humanCarTypes`,
   `carResetWeapons`).
2. The constants: 15 in `carRegisterModel` and `carTablesClear`; 9 in
   `uaInit`, `carTablesClear`, `arenaSetup`, `aiCarsSetActive`, `aiCarGet`,
   `screenAiSelect`; 2 in `uaInit`, `carTablesClear`, `roundStart`; stride
   15 in the five pair getters and `carPairsUpdate`.
3. The fixed humans 0/1 in `worldUpdate` (radar pair, taunt, round end).
4. Registries by car instead of by type (1.3), or a rule of 16 distinct
   types (impossible with 15).
5. Id 8 (1.5); the settings struct (opponent list of 10); the pickers
   (`tournamentPickOpponents` hangs past 13 - humans).
6. Sections 4-6: VRAM, start points, cards.

## 2. Getters or direct addresses

### 2.1 Getter calls (`jal`, CONFIRMED)

| Getter | VA | Call sites | Functions |
|---|---|---|---|
| `humanCarGet` | 8011CEE8 | 75 | 37 |
| `humanCarObject` | 8011CB50 | 41 | 27 |
| `aiCarObject` | 8011CB70 | 35 | 23 |
| `aiCarGet` | 8011D0EC | 36 | 24 |
| `aiGetTargetCar` | 8011CF0C | 15 | 15 |
| `aiGetWatchedCar` | 8011D128 | 6 | 3 |
| `carFromSlot` | 8011CFD0 | 4 | 4 |
| `humanCarFindById` | 8011D07C | 1 | 1 |
| `aiCarsGetBase` | 8011A390 | 0 | 0 |

No getter address appears in data (no function-pointer use). The getters
take an index and return `base + index * size`; none caches a pointer.

### 2.2 Direct addresses (CONFIRMED)

Every direct use is a `lui`/`addiu` (or `lui` + load/store) pair, often
with an index added between them. There are no gp-relative uses of the
record arrays (they are outside gp's reach), no numeric addresses in the C,
and no word in the EXE's data that points into 801879FC..8018F458.

| Array | Uses | Functions |
|---|---|---|
| `humanCars` | 195 | 34 |
| `aiCars` | 423 | 32 |
| `aiCarObjects` | 8 | 7 |
| `humanCarObjects` (sdata, gp) | 6 | 6 |
| slot tables, `aiCarTargets`, `aiCarObjectSlots`, pair tables | 69 | 25 |
| any of the above | | 66 |

Per function (uses of `humanCars` / `aiCars` / the object arrays):

| VA | Function | humanCars | aiCars | objects |
|---|---|---|---|---|
| 80119698 | `uaInit` | 8 | | |
| 80119848 | `carTablesClear` | 1 | 1 | |
| 8011A024 | `roundStart` | 1 | 2 | |
| 8011A198 | `humanCarsRestart` | 3 | | |
| 8011A274 | `humanCarSetId` | 1 | | |
| 8011A340 | `humanCarSetEternalLife` | 1 | | |
| 8011A390 | `aiCarsGetBase` | | 1 | |
| 8011A3A0 | `aiCarsSetActive` | | 1 | |
| 8011A3C4 | `humanCarSetInfiniteWeapons` | 1 | | |
| 8011A3FC | `carBindHumanSlot` | 4 | | 1 |
| 8011A5CC | `carBindAiSlot` | | 4 | 1 |
| 8011A700 | `worldUpdate` | 17 | 62 | 2 |
| 8011B7D8 | `aiCarShouldUpdate` | | 5 | |
| 8011B89C | `humanCarDamageUpdate` | 28 | | |
| 8011BBD4 | `aiCarDamageUpdate` | | 19 | |
| 8011C248 | `aiCarsCountRemaining` | | 10 | |
| 8011C8E4 | `humanCarNearest` | 2 | | |
| 8011C98C | `aiCarNearestAlive` | | 3 | |
| 8011CA40 | `humanCarGetPos` | 3 | | |
| 8011CA98 | `aiCarGetPos` | | 3 | |
| 8011CAF8 | `humanCarGetRot` | 3 | | |
| 8011CB50 | `humanCarObject` | | | 1 |
| 8011CB70 | `aiCarObject` | | | 2 |
| 8011CBD0 | `hudSmoothSpeed` | 1 | | |
| 8011CD08 | `carGetDisplaySpeed` | 2 | 1 | |
| 8011CE14 | `aiCarGetDisplaySpeed` | | 1 | |
| 8011CEE8 | `humanCarGet` | 1 | | |
| 8011CF0C | `aiGetTargetCar` | 1 | 2 | |
| 8011CFD0 | `carFromSlot` | 1 | 1 | |
| 8011D07C | `humanCarFindById` | 3 | | |
| 8011D0EC | `aiCarGet` | | 1 | |
| 8011D128 | `aiGetWatchedCar` | | 1 | |
| 8011D528 | `aiApplyDifficulty` | | 1 | |
| 8011D98C | `carSaveNodeState` | 1 | | 2 |
| 8011DA1C | `carRestoreNodeState` | 1 | 1 | 2 |
| 8011DAF8 | `viewWatchNextAiCar` | 1 | | |
| 8011DB48 | `viewStopWatchingAi` | 1 | | |
| 8011E16C | `aiCarsCountAlive` | | 1 | |
| 8011E220 | `carsPhysicsUpdate` | 26 | 30 | 2 |
| 8011E7D8 | `carPairsUpdate` | 2 | 2 | |
| 8011EDD0 | `demoWatchNextAiCar` | 13 | 9 | |
| 8011EFDC | `demoRespawnAiCars` | | 5 | 1 |
| 8011F284..801203EC | the eight `demoSpawn<Arena>` | 7..9 each | 32 each | |

The slot tables are used directly by the binders, the `carFind*`
functions, `carRegisterModel`, `carTablesClear`, `arenaSetup`,
`carDestroyed` (8011BE10), `carObjectId`, `carPairsUpdate` and the pair
getters (section 1.2).

So: the records are **not** reached only through the getters. But the
direct users are 50 functions of one module (80119698..80120664; with the
slot and pair tables 66 functions in 8011961C..80120664), and the rest of
the game (weapons, AI, HUD, physics helpers: 72 functions, none of them a
direct user) uses the getters. In the decomp's C both kinds use
the same symbols, so for a source build moving an array means changing its
symbol in `config/US/undefined_syms_auto.txt` (`humanCars = 0x80187B10;`,
`aiCars = 0x80188B5C;` ...) or its `.tm2_<va>` section (`humanCarObjects`,
`aiCarObjects` are defined in `car_object_lookup.c`), the declared sizes
(`include/semantic/cars.h` `humanCars[2]` / `aiCars[9]`, and the local
`extern` declarations of `humanCars`/`aiCars` in 15 files), and the
constants of section 1.7. A binary patch would need the `lui`/`addiu`
pairs of those 66 functions.

## 3. `sessionLoadArenaAndCars` and the load area

### 3.1 The function (8012BAFC, shell_main.c, CONFIRMED)

| Step | VA | What |
|---|---|---|
| 1 | 8012BB30 | `bitmapInit` (VRAM bookkeeping) |
| 2 | 8012BB40..8012BB58 | level `.TMS` to **0x80019C40**; `bitmapParseContainer(buf, 0)` (fixed VRAM spots) |
| 3 | 8012BBB0..8012BBC8 / 8012BC10..8012BC28 | each human's, then each opponent's car `.TMS` to **0x80019C40**; `bitmapParseContainer(buf, n)`, n = 1, 2, ... (textures placed by `vramPlaceTexture`) |
| 4 | 8012BC5C..8012BC64 | level `.DMD` to **0x80019C40**; `size` = file size (s3) |
| 5 | 8012BC70 | `arenaResetSystems(settings->f20)` -> `worldLoadStartObjects` walks the level roots in place |
| 6 | 8012BC90 | `addu s5,s5,s3`: `off` = level size |
| 7 | 8012BCA0 / 8012BCA4 | `lui s6,0x8001; ori s6,s6,0x9C40`: base for the humans' car DMDs |
| 8 | 8012BCC0..8012BCDC | `buf = base + off`; load `carDmdFileName`; `assetInstallImage(buf, n)`; `off += size` |
| 9 | 8012BD08 / 8012BD0C | the same base again for the opponents' car DMDs, 8012BD28..8012BD44 |
| 10 | 8012BD60 | `bitmapDisableTracking` |

The base is an immediate in 9 places here (`lui 0x8001` + `ori 0x9C40`).
The end is not set anywhere: there is no limit, no size check and no
comparison with the EXE start. The space ends where the EXE text starts,
0x800CDC54 (`config/US/exe_header.json`), 737,300 bytes after the base.

`fileioLoadFileIntoRam` (8014DA30) -> `fileioReadFile` (8014D910) reads
whole 2,048-byte sectors with `CdRead` straight into the buffer, so up to
2,047 bytes after each file are overwritten too. Each next file is read
after that, so only the last car's tail matters: in a full load it would
run into the EXE's first function (`sub_800CDC54`, the model emitter). LIKELY.

### 3.2 How the images are packed (CONFIRMED)

Every DMD on the disc (level and car) was built for address 0x80019C40:
header word +0x0C is 0x80019C40 and its pointers are absolute.

- **Level DMD:** loaded at 0x80019C40 and **never relocated**.
  `worldLoadStartObjects` (800D1C70, rt_main.c) reads the shift at
  0x80019C50, the count at 0x80019C58 and the roots from 0x80019C5C as
  fixed addresses (7 `lui 0x8001` sites, 800D1C78..800D1CF4) and walks
  each root with `dbscanHierarchy(root, shift, worldLoadObject)`.
- **Car DMD:** loaded at `base + off`. `assetInstallImage` (80120EF4,
  asset_header.c) checks "PSXD" and version 0x43, sets
  `vramTexSlotGroup = n` and the world shift, then for each root:
  `object = entry + delta` with `delta = buf + 0x7FFE63C0` (= `buf -
  0x80019C40`, mod 2^32), `modelRelocate(object, delta)` (80120FEC; adds
  `delta` to every internal pointer and marks nodes with 0xB1),
  `dbscanHierarchy(object, shift, worldLoadObject)`. The constant
  0x7FFE63C0 (80120EF8) is the files' build address, not the load area:
  keep it when moving the area.
- The relocation is plain 32-bit addition, so a car image works at any
  address, including above 0x80200000.
- `assetInstallImage` stops early on a zero object, which after relocation
  only happens at the original base. No DMD on the disc has a zero root
  entry (checked: cars have 1 root, BB.DMD 3, levels 5..11), so this does
  not matter.

### 3.3 The limit today (CONFIRMED sizes, from the disc)

The level DMD plus every car DMD must fit in 737,300 bytes. Car DMDs: RK
39,168, HS 38,484, PV 36,312, IR 35,876, TH 35,124, SP 34,040, WH 33,620,
AX 33,204, OT 32,932, MN 32,736, MG 32,308, ST 29,516, FL 27,032, HH
24,940 (Dark Tooth's BB 61,660). Worst case (largest cars):

| Level | DMD | Size | Cars today (table) | Free with 10 cars | Free with 11 cars | Most cars that fit |
|---|---|---|---|---|---|---|
| 1 Los Angeles | FREEWAY | 422,112 | 7 | -36,308 | -68,616 | 9 |
| 2 Moscow | DISH | 423,908 | 6 | -38,104 | -70,412 | 9 |
| 3 Paris | PARIS / PARIS2 | 407,052 / 406,060 | 7 | -21,248 | -53,556 | 9 |
| 4 Amazonia | ISLANDS | 473,196 | 7 | -87,392 | -119,700 | 7 |
| 5 New York | ROOF / ROOF2 | 470,588 / 434,908 | 7 | -84,784 | -117,092 | 7 (8 in 2P) |
| 6 Antarctica | GLACIER / GLACIER2 | 309,540 / 261,124 | 8 | +76,264 | +43,956 | 12 (14 in 2P) |
| 7 Holland | DEN | 298,076 | 10 | +87,728 | +55,420 | 13 |
| 8 Hong Kong | HKONG | 416,396 | 9 | -30,592 | -62,900 | 9 |
| 10 Swamp | SWAMP | 362,060 | 0 | +23,744 | -8,564 | 10 |
| 11 Cyburbia | BURB | 538,780 | 0 | -152,976 | -185,284 | 5 |
| 12 Roof Tops | SROOF | 484,016 | 0 | -98,212 | -130,520 | 7 |

With the retail layout 10 or 11 cars fit only on Antarctica and Holland
(and 10 on Swamp). The level DMD alone always fits: the largest,
BURB.DMD, ends at 0x8009D4DC.

### 3.4 What else uses low RAM (CONFIRMED unless marked)

| Range | What | Notes |
|---|---|---|
| 0x80010000..0x80019020 | `SCREEN\UACORE.VAB` header (67 programs: 0x20 + 0x800 + 67 x 512 + 0x200 = 36,896 bytes) | loaded at boot by `shellLoadUaCore` (80128C7C). `SsVabOpen` (801461C0) calls `SsVabOpenHead(addr, -1)`, which stores the address in libsnd's `_svm_vab_vh` (`D_801AB88C`, store at 801463F0); libsnd reads the header through it (`SpuVmVSetUp`) when it sets up voices. **Must stay.** This is why the load area starts at 0x80019C40 |
| 0x80019C40.. | level and car TMS (one at a time, only during the load), level DMD, car DMDs | the TMS copies are overwritten by the level DMD at once, so nothing keeps pointers into them |
| 0x80019C40.. | front-end scene TMS/DMD (`shellLoadTmsDmd` 80129010, 3 sites) | front end only |
| 0x80048270.. | front-end background images (`screenLoadImageFile` 80121C64; `main`, the menu screens) | front end only; called by no in-battle code (`sessionRun` does not reach it) |
| 0x800CDC54.. | EXE text, data, BSS to 0x801FAD48 | |
| 0x801FAD48..0x80200000 | stack (about 21 KB) | crt0 (`start` 8013878C) sets sp = 0x200000 (word at 80138838) OR 0x80000000 = 0x80200000 |

Checks for "below the EXE": none found. The game has no `malloc` (the only
heap call is one `delete` at 801373CC; `InitHeap` gets a negative size
because BSS + stack exceed 2 MB, so the heap is unused). Every
`lui 0x8000` compare in game code is the INT_MIN guard of a division, not a
pointer test. The 0x00FFFFFF / 0xFF000000 masks are ordering-table tags in
libgpu, the HUD radar and `explodeMakeScreenRed`, on primitives in BSS. No
code compares a pointer with 0x800CDC54 or masks a DMD pointer.
CONFIRMED (scan of every `lui` constant in the EXE).

Hardware side: `CdRead` DMAs the file straight into the destination, so
the port's CD DMA must write above 2 MB; on a real 2 MB machine
0x80200000 is a mirror of 0x80000000 (the BIOS area). The renderer reads
the meshes with the CPU and builds packets in BSS, and the textures go to
VRAM from the TMS copy at 0x80019C40, so the GPU never DMAs from a DMD.
LIKELY.

### 3.5 Moving the load area above 0x80200000

**Recommended: move only the car DMDs.** The level stays at 0x80019C40,
where its absolute pointers and `worldLoadStartObjects` expect it, and it
always fits (3.3). The cars are relocated anyway.

- Assembly: at 8012BCA0 and 8012BD08 load 0x80200000 into s6
  (`lui s6,0x8020` + `ori s6,s6,0`), and at 8012BC90 do not add the level
  size (make it a `nop`, so `off` starts at 0).
- C (`shell_main.c`): give the two car loops their own base, for example
  `#define CAR_DMD_BASE 0x80200000`, set `off = 0` after the level load,
  and use `(u8 *)CAR_DMD_BASE + off` in both loops.
- Nothing else changes: `assetInstallImage` relocates by `buf -
  0x80019C40`, and no other code reads a car DMD by address. The TMS
  loads stay at 0x80019C40.
- Needs real RAM at 0x80200000.. in the port (no mirror) and CD DMA there.
  16 cars take at most about 0.6 MB (16 x 39,168).

Moving the level DMD too would need: the 9 + 7 base constants
(`sessionLoadArenaAndCars`, `worldLoadStartObjects`), a relocation of the
level image (each root through `modelRelocate(root, newbase -
0x80019C40)` before `worldLoadStartObjects` walks it, as
`assetInstallImage` does for cars), and keeping 0x80010000..0x80019020
intact. LIKELY to work; no reason to do it.

### 3.6 First step: 9 opponents on every arena

With the car DMDs moved, memory no longer limits the count. The records
and tables already hold 2 humans + 9 AI (11 cars, 11 of 15 slots). What
else limits the opponents per arena today:

| Limit | Where | Today | For 9 opponents | Conf. |
|---|---|---|---|---|
| opponents per arena | `tournamentOpponentCount` (8012AFE8): total cars per level, minus humans, stored in settings +5 by `screenSinglePlayerMenu` (8012F37C, 8012F4A8), `screenMultiPlayer` (8012FCA8), `screenLevelSelect` (80130730, Challenge mode 3) and `screenScrollText` (80136CDC, next tournament level) | 6..10 cars; 0 for levels 9-12 (`SECRET_LEVELS.md`) | return 9 + humans (or set settings +5 = 9 after the call) | CONFIRMED |
| Challenge opponent picks | `screenAiSelect` (80131EEC): picks while +7 < +5, clears slots +7..8 | 9 slots | fits | CONFIRMED |
| distinct cars | `carIsSelectable` (8012CC10) | 12 regular cars + Sweet Tooth and Minion when unlocked | 2 + 9 = 11 fits even locked | CONFIRMED |
| random picks | `tournamentPickOpponents` (8012CD08): cars 0..12, distinct | | 9 <= 13 - 2 | CONFIRMED |
| settings opponent list | `opponent_car[10]` at +0x0A | 10 | fits | CONFIRMED |
| AI records, objects, targets | `aiCars[9]` etc., `aiCarGet` clamp 9 | 9 | fits | CONFIRMED |
| car world objects | `carRegisterModel` < 15 | 15 | 11 fits | CONFIRMED |
| memory | 3.3 | 5..14 cars by arena | car DMDs moved (3.5) | CONFIRMED |
| start points | `startNodePickRandom`, up to 15 nodes per arena | section 5 | section 5 | |
| car textures in VRAM | `vramPlaceTexture`, groups 1..11 | section 4 | section 4 | |
| opponent cards | `screenCardHalfSize`, 9 card quads | section 6 | 9 fits | |
| radar | 12 blips per view | 10 other cars at most | fits | LIKELY |
| AI routes on Roof Tops, round end in mode 4 | `SECRET_LEVELS.md` | | unchanged | CONFIRMED there |

So 9 opponents on every arena needs: the car DMDs moved (3.5), settings
+5 = 9 (the `tournamentOpponentCount` table), and whatever sections 4-6
find for VRAM and start points. Nothing in the car arrays.

## 4. VRAM: car textures

Static analysis plus a replay of the placement code on the disc files
(`cdimage/`, read only; scratch scripts `replay.py`, `run*.py` in the
scratchpad `req12/` folder). The replay follows the C exactly (top-left
test, scan order, reuse by key); it is not a capture, so its numbers are
LIKELY. Everything about the code is CONFIRMED unless tagged.

### 4.1 How a car's textures reach VRAM (CONFIRMED)

`sessionLoadArenaAndCars` (8012BAFC, `shell_main.c`) does it in two passes:

| Step | Call | What happens |
|---|---|---|
| 1 | `bitmapInit` (80121550, `bitmap_init.c`) | clears the VRAM-usage bitmap (1 bit per VRAM texel), reserves (0,0,640,256) (both frame buffers), (960,256,64,32), (960,288,64,128), (960,416,32,96) (fonts: `fontInit` puts the font images at x 960, y 256.. and their CLUTs at (960, 511-i)) |
| 2 | `bitmapParseContainer(levelTms, 0)` (80121A2C) | every level image goes to the VRAM x/y written in its TIM; pixels and CLUT are marked used |
| 3 | `bitmapParseContainer(carTms, n)`, n = 1, 2, .. (humans first, then opponents) | `bitmapRegisterTim` (8012191C) hands each image to `vramPlaceTexture(info, index, n)` (801209E8, `screen_deathmatch_result.c`) |
| 4 | level DMD, then each car DMD at 0x80019C40 + off, `assetInstallImage(buf, n)` (80120EF4, `asset_header.c`) | sets `vramTexSlotGroup` (80181050) = n; `modelRelocate` (80120FEC) calls `modelRemapTextures` (80120744, `world_collapse_warps.c`) on every mesh node |
| 5 | `bitmapDisableTracking` (801215EC) | bookkeeping ends |

All car TMS files are parsed before any car DMD is installed, so the
slot table must hold every car's slots at once.

`modelRemapTextures` reads, for each textured primitive, the image index
from the low 5 bits of the primitive's tpage field (0..31), takes slot
`group * 20 + index`, and rewrites the CLUT word, the tpage and adds the
image's x/y inside the page to the UVs (or to the texture-window word for
primitives with the flag bit).

Bookkeeping storage (`bitmapSetupStorage` 801214FC):

| Table | Where | Entry | Room |
|---|---|---|---|
| usage bitmap `bitmapWords` (80181038) | packet buffer 0 (`D_801B600C`) +0 | 1 bit per texel, 0x4000 words | all of VRAM |
| slots `vramTexSlots` (80181034) | packet buffer 0 + 0x10004 | 20 bytes: tpage, x, y, CLUT x, CLUT y; index `group * 20 + image` | (0x17CF0 - 0x10004) / 20 = 1,599 slots |
| image records `bitmapRecords` | `renderGetOrderingTable()` = OT `D_801E5A8C` | 24 bytes: key, tpage, x, y, CLUT x, CLUT y | 682 in that 16 KB OT; more spill into the next OTs (801E9A8C..), which are idle while loading (LIKELY harmless) |

(`bitmap_record_append.c` calls the last two record words width and
height; they are read from TimInfo +16/+18, which are the CLUT x/y. So a
reused record carries the right CLUT. CONFIRMED from the struct layout.)

### 4.2 The placement algorithm (CONFIRMED)

`vramPlaceTexture(img, index, group)`:

1. If `bitmapFindRecord(key)` (801218CC) finds an image with the same key
   (from the level or an earlier car), copy its tpage, x, y and CLUT into
   the slot and stop. **The same car type twice costs no VRAM**, and the
   six part images shared by most cars (keys 0x9941, 0x9BCF, 0xAA9A,
   0x9201, 0x1FCD5, 0x9609; 3,584 texels in all) are placed once.
2. Otherwise look for a free w x h spot (w in VRAM halfwords) in the lower
   half only (y 256..511). Pass 1: the column block x 960..1215 (only
   (992..1023, 416..511) is not reserved). Pass 2: the blocks at x 0, 256,
   512, 768. Inside a block it walks x in steps of w and, for each x, y in
   steps of h (column by column). A spot counts as free if its **top-left
   texel** is free (`bitmapIsFreeXY` 801216A8); the rectangle is then
   marked and uploaded.
3. If the image has a CLUT, `vramUploadClut(16 or 256, 1, clut, index,
   group)` (80120D3C, `scene_init.c`) scans blocks (0,0), (256,0),
   (512,0), (768,0), then the lower four, row by row, x in steps of the
   CLUT width (so a 256-colour CLUT can only start at x 0, 256, 512, 768),
   again testing only the top-left texel. So a 256-colour CLUT in the
   lower half can land on texels already in use (the font area, car
   pixels): 4.4, `RETAIL_BUGS.md` 27.
4. On failure it prints "Relocatable texture not loaded" and returns 0;
   the slot keeps stale values and the texture draws wrong. Nothing stops
   the load.

During a level the upper-half x 0..639 holds the frame buffers and x
640..1023 the level's own images and CLUTs, so in practice car CLUTs land
in the few free rows at x 768 of the upper half, and once those are gone,
in rows of the lower half (where each 256-colour CLUT takes a 256-wide row
from the image area).

### 4.3 What each car takes (CONFIRMED, from `TMS\<code>.TMS`)

Every car has one 8-bit 128x128-texel body image (64 x 128 VRAM
halfwords, 8,192 texels, with a 256-colour CLUT) plus small 4-bit parts.
"Own" excludes the six images shared by key.

| Car (id, code) | Images | All texels | Own texels | Own 256-colour CLUTs | Own 16-colour CLUTs |
|---|---|---|---|---|---|
| Hammerhead (0, HH) | 5 | 10,496 | 8,192 | 1 | 0 |
| Outlaw 2 (1, OT) | 7 | 11,008 | 8,448 | 1 | 1 |
| Warthog (2, WH) | 5 | 10,496 | 8,192 | 1 | 0 |
| Mr. Grimm (3, MG) | 17 | 14,592 | 12,800 | 1 | 13 |
| Grasshopper (4, PV) | 6 | 11,008 | 8,704 | 1 | 1 |
| Thumper (5, TH) | 5 | 10,496 | 8,192 | 1 | 0 |
| Spectre (6, SP) | 9 | 10,944 | 8,640 | 1 | 4 |
| Roadkill (7, RK) | 8 | 11,264 | 8,960 | 1 | 3 |
| Twister (8, IR) | 9 | 12,544 | 10,240 | 1 | 4 |
| Axel (9, AX) | 7 | 11,520 | 9,216 | 3 | 0 |
| Mr. Slam (10, FL) | 5 | 10,496 | 8,192 | 1 | 0 |
| Shadow (11, HS) | 7 | 11,520 | 9,216 | 1 | 2 |
| Sweet Tooth (12, ST) | 6 | 11,520 | 9,216 | 2 | 0 |
| Minion (13, MN) | 6 | 10,752 | 9,472 | 1 | 2 |
| Dark Tooth (14, BB) | **31** | 79,616 | 77,056 | 11 | 15 |

A "texel" here is one VRAM halfword; a 256x256 block is 65,536.

### 4.4 What is free during a level (LIKELY, replay)

After the level TMS alone (no cars). "Free CLUT rows" = rows at x 768 of
the upper half whose 256 texels are all free (where car 256-colour CLUTs
go first). The type columns replay `vramPlaceTexture` with distinct
car types added largest first (Mr. Grimm, Twister, Axel, Shadow, Sweet
Tooth, Roadkill, Outlaw 2, Grasshopper, Spectre, Minion, Hammerhead,
Warthog, Thumper, Mr. Slam) and smallest first, and give how many types
fit before the first failure and why.

**Corrected (request 33-1 follow-up).** The first version of this table
counted a type as fitting when every image and CLUT was *placed*. But
retail's CLUT scan tests only the top-left texel of a spot (4.2 step 3,
`RETAIL_BUGS.md` 27): once the upper half's CLUT rows are gone, a
256-colour CLUT can start on a free texel of the lower half and run over
texels already in use (the font area at x 960.., car pixels). So each
type column now gives three numbers: **placed** (retail's count, as
before) / **clean** (types loaded before the first CLUT that overwrites
used texels: what really fits in retail) / **bit 9** (with the port's
`portCarOptions` bit 9, `TM2_PORT_CAR_CLUTS`: every texel of a CLUT spot
tested, rows 240..255 as the last resort; never over used texels; the
number in brackets after it is the spots of rows 240..255 it took).
**Request 41 item B (2026-10-16):** the image scan has the same one-texel
test, so the retail cells give a fourth number in brackets, **img** (types
loaded before the first CLUT *or image* over used texels), and bit 9 now
also tests the whole image rectangle (retail's scan as the fallback when
no rectangle is wholly free; `CAR_GROUPS.md` 13.12); the bit 9 numbers
are with both scans. The
reason after a count is what failed first. The last column gives retail /
bit 9 for Dark Tooth after Mr. Grimm and Twister. Host model:
`tools/check_car_groups.py --clut-model` (check 11, the "Types per
arena" table), bits 6 and 7 clear. Its "placed" counts and retail Dark
Tooth column equal the first replay's, except Holland and Cyburbia's Dark
Tooth case, which the first replay called ok.

| Level | TMS | Lower half free from x | Free texels, lower half | Free CLUT rows (x 768) | Largest first: placed / clean (img) / bit 9 | Smallest first: placed / clean (img) / bit 9 | Dark Tooth + MG + IR: retail / bit 9 |
|---|---|---|---|---|---|---|---|
| 1 Los Angeles | FREEWAY | 528 | 114,112 | 17 (y 167..183) | 12 / 12 (**9**) / **11** (pixels) | 12 / 12 (**10**) / 12 (pixels) | ok / ok |
| 2 Moscow | DISH | 280 | 177,472 | 15 (73..87) | 14 / **10** (10) / 14 (4) | 14 / **11** (11) / 14 (3) | ok / ok |
| 3 Paris | PARIS | 636 | 86,176 | 15 (161..175) | **8** / 8 (8) / 8 (pixels) | 9 / 9 (9) / 9 (pixels) | fails (pixels) / fails (pixels) |
| 3 Paris, 2P | PARIS2 | 624 | 89,760 | 15 (161..175) | **8** / 8 (8) / 8 (pixels) | 9 / 9 (9) / 9 (pixels) | fails (pixels) / fails (pixels) |
| 4 Amazonia | ISLANDS | 424 | 140,288 | 13 (75..87) | 12 (pixels) / **8** (**4**) / 14 (6) | 14 / **11** (11) / 14 (5) | fails (CLUT) / ok |
| 5 New York | ROOF | 516 | 117,120 | 17 (87..103) | 12 / 12 (12) / 12 (pixels) | 12 / 12 (**10**) / 12 (pixels) | ok / ok |
| 5 New York, 2P | ROOF2 | 440 | 136,880 | 18 (78..95) | 13 (CLUT) / 13 (**4**) / 14 (1) | 13 (CLUT) / 13 (13) / 14 (1) | ok / ok |
| 6 Antarctica | GLACIER | 420 | 142,144 | 15 (89..103) | 10 (CLUT) / 10 (10) / 14 (4) | 14 / **11** (11) / 14 (3) | ok / ok |
| 6 Antarctica, 2P | GLACIER2 | 412 | 143,680 | 16 (88..103) | 11 (CLUT) / 11 (11) / 14 (3) | 12 (CLUT) / 12 (12) / 14 (2) | ok / ok |
| 7 Holland | DEN | 320 | 167,168 | 13 (67..79) | 14 / **8** (8) / 14 (6) | 14 / **11** (11) / 14 (5) | **overwrites** / ok |
| 8 Hong Kong | HKONG | 518 | 116,640 | 14 (146..159) | 9 (CLUT) / 9 (**2**) / 12 (pixels; 3) | 11 (CLUT) / 11 (11) / 12 (pixels; 2) | fails (CLUT) / ok |
| 10 Suicide Swamp | SWAMP | 604 | 95,104 | 15 (113..127) | **9** / 9 (**1**) / 9 (pixels) | 10 / 10 (10) / 10 (pixels) | fails (pixels) / fails (pixels) |
| 11 Cyburbia | BURB | 284 | 176,192 | 13 (43..55) | 14 / **8** (8) / 14 (6) | 14 / **11** (11) / 14 (5) | **overwrites** / ok |
| 12 Roof Tops | SROOF | 244 | 186,432 | 15 (41..55) | 14 / **10** (**4**) / 14 (4) | 14 / **11** (11) / 14 (3) | ok / ok |

So in retail the binding limit is the lower-half image area (Paris,
Swamp, then Los Angeles, New York, Hong Kong) or the 256-colour CLUT rows:
on Antarctica and Hong Kong the scan fails, on Moscow, Amazonia, Holland,
Cyburbia and Roof Tops it "succeeds" by overwriting from the 9th-12th
type on. **Without overwriting, retail fits 8 types on every arena and
14 nowhere.** With bit 9 all 14 fit cleanly on Moscow, Amazonia, New
York 2P, both Antarctica files, Holland, Cyburbia and Roof Tops (taking 1
to 6 of the 32 256-colour spots of rows 240..255, which bit 7's copies
and bit 6 then share), Hong Kong goes from 9 to 12, and Paris, Swamp, Los
Angeles and New York stay pixel-bound. Up to the first CLUT that retail
would put over used texels (or fail to place) every placement with bit 9
is retail's; from there on the CLUTs take other spots. With bit 9 a 256-colour CLUT no
longer takes a row of the lower-half image area by mistake, which is why
Amazonia (largest first) and Hong Kong also gain pixel room.

**Images (request 41 item B).** Counting image overlaps too (the "img"
number), retail's clean count drops further where an image rectangle
runs over an earlier one: from the 2nd type on Swamp (largest first),
the 3rd on Hong Kong, the 5th on Amazonia, New York 2P and Roof Tops, the
10th-11th on Los Angeles and New York (smallest first); Mr. Grimm +
Twister + Dark Tooth already overlap on 9 arenas. So **without
overwriting, retail fits 1-4 types on the worst arenas** (largest first),
and the Dark Tooth column's "ok" is for CLUTs only. With bit 9 the image
scan tests whole rectangles too: no image or CLUT lands on used texels
(outside the fallback, which only pixel-bound loads reach), and the bit 9
numbers stay as with the CLUT scan alone except Los Angeles largest first
(12 -> 11: retail's 12th type fit only by overlapping images).

The image scan (4.2 step 2) has the same one-texel test, and the model
shows images overlapping earlier ones on most arenas too (Suicide Swamp
already with Mr. Grimm and Twister), so a retail load can also lose
texels there; bit 9 does not change the image scan (`RETAIL_BUGS.md`
27).
Retail itself can hit this: a Challenge game with 9 or more different
large cars on Paris or Swamp would print "Relocatable texture not loaded"
(LIKELY, not seen running). No image is ever placed across x 1024 (pass 1
tests x up to 1215, but those spots are always taken).

### 4.5 What 16 cars need

Duplicates are free (4.2), and there are only 15 car types, so 16 cars
always share at least one type.

| Case | Texels (lower half) | 256-colour CLUT rows | 16-colour CLUTs |
|---|---|---|---|
| Typical: 16 cars of about 9 different normal types (estimate from 4.3) | about 85,000 | about 10 | about 20 |
| All 14 normal types | 131,264 (2.0 blocks of 256x256) | 17 | 30 own + 6 shared |
| All 15 types (with Dark Tooth) | 208,320 (3.2 blocks) | 28 (27 different palettes: Dark Tooth and Sweet Tooth share one) | 51 |

Compared with 4.4: all 14 normal types are placed on Moscow, Holland,
Cyburbia and Roof Tops as is, but only by overwriting texels (clean: 8-11
types); with `portCarOptions` bit 9 they, Amazonia, Antarctica and New
York 2P take all 14 cleanly; Paris and Swamp cannot hold more than 8-9
different types in the lower half whatever is done with CLUTs. Dark Tooth plus 13 others fits nowhere.

Two non-VRAM limits found on the way (CONFIRMED):

- **Slot stride 20 vs 31 images.** Slots are `group * 20 + image`, but an
  image index can be 0..31. Dark Tooth's 31 images write slots
  `n*20 + 20..30`, which are the next car's slots 0..10; the next car's
  TMS (parsed before any DMD) overwrites them, so Dark Tooth's DMD would
  remap images 20..30 to the next car's textures. Retail always loads him
  last (sole opponent), so it never shows. For 16 cars either load him
  last or change the stride to 32 in the four places that use it:
  `vramPlaceTexture`, `vramUploadClut`, `bitmapAddRecord` (80121764) and
  `modelRemapTextures`. The slot table has room (17 groups x 32 = 544 of
  1,599).
- **Records.** One record per image, reused or not: level (189..327) plus
  the cars. 16 cars of 17 images or fewer stay under 682; 16 Dark Tooths
  (823) would spill into the next OT, which is idle during loading
  (LIKELY harmless).

### 4.6 Options

1. **Shared textures for duplicate types: already there** (reuse by key,
   4.2). The port only has to keep the number of *different* types per
   arena within 4.4 (8 always fits).
2. **CLUT room under the frame buffers.** `bitmapInit` reserves
   (0,0,640,256), but the frame buffers are 320x240 (draw/display
   environments and `screenClearScreen` use 240 lines), and no level image
   or CLUT touches rows 240..255 (checked on all 14 level TMS files).
   Reserving (0,0,640,240) instead frees 16 rows x 640: 32 256-colour CLUT
   spots at x 0 and 256, found first by the CLUT scan. Replay: Antarctica
   (both files), Amazonia and New York 2P then take all 14 types; Hong
   Kong goes from 9 to 12; Paris, Swamp, Los Angeles and New York stay
   pixel-bound. LIKELY (nothing else seen drawing there).
3. **CLUT packing.** Step 16 instead of the CLUT width in
   `vramUploadClut` lets a 256-colour CLUT start in a part-used row (any
   x that is a multiple of 16). Helps CLUT-bound arenas; does nothing for
   pixels.
4. **CLUT sharing by content** saves almost nothing: across all 15 cars
   only 4 palettes are duplicated under different keys (three 16-colour
   ones between Mr. Grimm and Roadkill, one 256-colour one between Dark
   Tooth and Sweet Tooth); the 256-colour body palettes are all different.
5. **Pixel room (Paris, Swamp)** can only come from outside the 1024x512
   VRAM, or from limiting the number of different types there.
6. **A larger VRAM in the port's GPU emulation** (note only): the game's
   primitives address VRAM through the tpage word (x / 64 in 4 bits, y in
   1 bit: 0 or 256) and the CLUT word (x / 16 in 6 bits, y in 9 bits), so
   anything above 1024x512 cannot be named by the retail primitive
   formats. It would need an extended encoding in the port's GPU (for
   example a spare tpage bit as a bank select; the 2 MB-VRAM GPU variants
   used tpage bit 11 for a second y bit) plus changes in
   `vramPlaceTexture`, `vramUploadClut`, `GetTPage` callers and
   `modelRemapTextures`.

## 5. Start points

### 5.1 The start lists (CONFIRMED, `ai_start_nodes.c`)

Each arena's route loader (`routeLoad<Track>`, `ai_route_loaders.c`) sets
the route-node count, calls its `startNodes<Track>` and copies
`TERRAIN\<track>.PTS` into `aiRouteNodes` (`D_801B1DE8`, 24 bytes: x s32,
y s32, height s16, next[6] s16, count s8, section s8).

| Level | List function | Start nodes | Route nodes | Closest two starts | Median gap to nearest start |
|---|---|---|---|---|---|
| 1 Los Angeles | `startNodesFwy` 80101AB4 | 10 | 79 | 1,634 | 4,517 |
| 2 Moscow | `startNodesDish` 80101C38 | 10 | 33 | 1,131 | 1,555 |
| 3 Paris | `startNodesParis` 8010210C | 11 (humans: first 9) | 118 | 1,723 | 2,575 |
| 4 Amazonia | `startNodesLava` 80101F7C | 11 | 94 | 1,060 | 3,000 |
| 5 New York | `startNodesRoof` 8010244C | 10 | 101 | 2,126 | 4,118 |
| 6 Antarctica | `startNodesIce` 80101DBC | 15 | 79 | 1,173 | 1,460 |
| 7 Holland | `startNodesDen` 801025D0 | 15 | 58 | 762 | 2,093 |
| 8 Hong Kong | `startNodesHkong` 80102298 | 14 | 140 | 1,600 | 4,494 |
| 10 Suicide Swamp | `startNodesSwamp` 8010278C | 8 (every node) | 8 | 1,718 | 2,285 |
| 11 Cyburbia | `startNodesCyb` 801028F4 | 10 (every node) | 10 | 2,642 | 4,640 |
| 12 Roof Tops | `startNodesSroof` 80102A74 | 9 (every node) | 9 | 3,056 | 3,419 |

Distances are in map units on x/y (from the PTS files). For scale, a
ram (`weaponSlamFindTarget` 800F5498) hits a car within 160 units of a
point 60..120 units ahead of the axle.

Storage: `startNodeList` (801B2B28): node ids s16[15], then taken bytes
u8[15] at 801B2B46, count at 801B2B55 (`startNodeCount`). It is followed
directly by `moscowDishEmitters` (801B2B58), so it cannot grow in place.

### 5.2 How a spawn picks a node (CONFIRMED)

`startNodePickRandom(count)` (800F636C): start at `rand() % count`, take
the first entry not taken (wrapping once), mark it taken, return its node
id. If all are taken, return a random entry's node **without marking**, so
any number of cars can land on the same node.

| Caller | When | Count passed | Placement |
|---|---|---|---|
| `aiCarStart` (800FFEFC, `ai_car_start.c`) | `carBindAiSlot` (8011A5CC) for each opponent in `sessionBindCars` (8012BD94), every round; also when an AI's `stats.restart_pending` (+0x9C of the stats block) is set (`aiCarDrive` 800E7EDC) | `startNodeCount` | stores the node in `stats.spawn_node`, then `aiPlaceOnRouteNode` (800F6518, `ai_route_setup.c`): position = node x/y, height = the floor of the node's section, heading from the route state |
| `humanCarPlaceAtStart` (800FD6C4, `car_object_contact.c`) | `roundStart` -> `humanCarsRestart`, and every human restart (`carRestart`, `restart_pending` in `humanCarTick` 800DC88C) | 9 on level 3, else `startNodeCount` | node x/y, height + ride height, heading toward a random successor, then `worldSectionAt` for the wheels |

Order in a round: `sessionBindCars` -> `arenaSetup` (8011995C) clears the
taken bytes (`startNodesClearUsed` 801019DC; via the route loader on a new
arena), the AIs pick, then `roundStart` places the humans. So with more
cars than nodes it is the humans (and later restarts) that share.
`aiRouteNodeRemove` (800F2308) also deletes a node from the list
(`startNodesRemove` 80101A04) when a section goes away (Antarctica
collapse), so the count can drop during a match.

`aiCarStart` sets `+0x012 = (index + 1) * 60`. That field is the AI's
behaviour timer (`state_time` in `ai_steps.c`): the first behaviour change
of AI i comes i+1 seconds after the start. It is not a spawn delay
(LIKELY).

The attract demo uses its own tables (`spawn_tables.c`), not these.

### 5.3 Ways to space extra cars

**A. A distance-aware picker (simplest, all arenas).** Replace
`startNodePickRandom` with: among untaken entries, pick the one whose node
is farthest from every car already placed this round (AI positions are set
as each is bound, humans after); when all are taken, pick the entry
farthest from all cars and add an offset (C). Same signature, two callers.
`COOP_LIVES.md` 2.3 gives the same idea for restarts.

**B. Route nodes as extra start points (main arenas).** Route nodes are on
the AI's driving graph, so they lie on drivable ground. A greedy
farthest-point pick over all route nodes, starting from the retail list,
reaches 16 points without lowering the list's own smallest gap (LIKELY:
picked by distance only, not checked against collapsing floors, the
Eiffel Tower, warps or water):

| Level | Nodes to add (in order) | Smallest gap of the 16 |
|---|---|---|
| 1 Los Angeles | 2, 31, 71, 12, 18, 65 | 1,634 |
| 2 Moscow | 31, 18, 0, 20, 22, 26 | 1,131 |
| 3 Paris | 52, 3, 8, 49, 33 | 1,723 |
| 4 Amazonia | 77, 17, 92, 23, 29 | 1,060 |
| 5 New York | 41, 35, 48, 83, 79, 15 | 2,126 |
| 6 Antarctica | 69 | 1,173 |
| 7 Holland | 55 | 762 |
| 8 Hong Kong | 70, 38 | 1,600 |

The list needs a new home with 16+ entries (5.1); the human Paris limit
of 9 (nodes 0..72, so 91 and 96 are AI-only; the reason was not traced)
should stay. Hooks: the `startNodes<Track>` functions (or a port table
used instead of `startNodeList`), `startNodePickRandom`,
`startNodesClearUsed`, `startNodesRemove`.

**C. Offsets from a node along its heading (secret levels, overflow).**
On Swamp, Cyburbia and Roof Tops every route node is already a start, so B
adds nothing. Placing a second car part way to a successor works on Swamp
and Cyburbia (flat; route edges 1,718..4,712 and 4,640..14,400 long;
using edge midpoints gives 16 points at least 859 / 2,583 apart). On Roof
Tops edges cross between roofs (height steps of 240..320), so a midpoint
can be over a gap: check the point with `worldSectionAt` (800EFF34) and
fall back to a short offset (a few hundred units) along the heading.
Hooks:
- humans: in `humanCarPlaceAtStart`, after `pos` and `rot[2]` are set and
  before `wheelBlockSetSection(worldSectionAt(pos))`, move `pos` along the
  heading; the section and height are then derived from the new spot.
- AIs: after `aiPlaceOnRouteNode` (called from `aiCarStart`), move
  `motion.pos` and redo `wheelBlockSetSection` / `wheelBlockSetHeight`
  with `worldSectionAt`; retail takes the section from the node, which
  would be wrong after a move. The route state (node, target) can stay.

**D. A short spawn delay.** Two existing hooks re-place a car later:
setting `stats.restart_pending` makes the next `aiCarDrive` call
`aiCarStart` again (a fresh pick and full reset), and `stats.f03` makes
it re-place the car at `carNearestRoutePoint` (`aiCarDrive`, first lines).
A delay would hold extra car k hidden (`aiCarObjects[k]->visible = 0`, as
`demoRespawnAiCars` 8011EFDC does the opposite) for k x N frames, then set
`restart_pending` while the picker of A chooses a spot clear of every car.
More moving parts than A to C (the car still exists, and collides, while
it waits) (LIKELY).

Recommended: A everywhere, B for the main arenas, C for the three secret
levels and as the overflow when every entry is taken.

### 5.4 Built: A (requests 32 and 49)

`portCarOptions` bit 2 (`OPT_FAR_STARTS`): `startNodePickRandom` uses
`portStartNodePick`, the farthest-free picker of A, counting the live cars
placed this round when every entry is taken (`CAR_GROUPS.md` 7.4, 7.10).

### 5.5 Built: C as the overflow, along the route's links (request 85)

Port request 85 (2026-10-29, the user's 16-car team deathmatch on Suicide
Swamp): Swamp's 8 route nodes are its 8 start points, so with 16 cars every
start got two cars on top of each other. Asked: a start place past the
start points, a clear point along the route's links (at least 700 from
every placed car), on the ground there, facing along the link, and a way to
place the car there.

**Built** (with `TM2_PORT_CARS` and `OPT_FAR_STARTS`; `port_cars.h`):
- **When:** in `portStartNodePick`'s "all taken" case (also a respawn's and
  a drop-in's, which take that case), after it has chosen the entry farthest
  from the live placed cars: if that entry's node lies within 700 (straight
  line) of such a car, `portStartLinkFind` looks along every link of the
  route (each node to each of its `next[]`) at `|dx| + |dy|` / 256 points
  (2..16 parts, the ends excluded) for the one farthest from those cars
  (the first of equals). If it is 700 or more away, the pick returns the
  link's first node and leaves the point in `portStartLink`; else the node
  as before. A point whose ground (the same probe as below) lies more than
  160 under both ends is over a gap (Roof Tops' links cross between roofs)
  and refused: the node as before.
- **Where the car goes:** right after the node placement,
  `portStartLinkPlace` gives x and y, the ground height there
  (`worldGroundHeightAt`, probed down from 256 above the link's higher end)
  and
  the heading along the link (`ratan2` as `humanCarPlaceAtStart` turns to a
  next node):
  - a human (`humanCarPlaceAtStart`): x, y, ride height + ground, the
    heading; the section and wheel height are then taken from there by the
    code that follows (as from a node);
  - an AI car (`aiCarStart`, after `aiPlaceOnRouteNode`): x, y, the section
    at the point (`worldSectionAt`) and its floor base as `aiPlaceOnRouteNode`
    does, the heading (also the drive's), the wheels on that section at the
    ground height, and its next node the link's second (its node is the
    link's first).
  The point is taken once; every pick clears it first.
- **No `rand()`** is added (the human's random successor is still drawn,
  then overridden); with the bit clear, or a clear node, retail's or
  request 49's placement.
- **Distances:** straight line in 1/16 units (each axis clamped), the same
  live-car set as request 49 (placed this round, HP > 0).
- **On Swamp** (LIKELY, from 5.3 C's edge lengths 1,718..4,712): the links'
  middles are 859 or more from their ends, so with a car on every node the
  next eight go near the links' middles; past that the farthest sample may be
  under 700 and the car takes the node as before.

**Checked:** `tools/check_rollback.py` check 6: both starts call
`portStartLinkPlace` once, after their pick; it reaches no `rand()`; run in
check_custom's `LinkMips` on a made-up ring route (8 nodes 2,400 from the
centre, all starts taken): with an AI car on every node the pick returns
the link's first node and the farthest sample (>= 700), and
`portStartLinkPlace` gives that point, the (stubbed) ground height, the
heading along the link and the second node, once; with cars also at the
links' middles, with one car, or with the ground 2,000 below the route
(a gap), no link start. Not tested in game.

## 6. Opponent-card screen

### 6.1 When it runs (CONFIRMED)

`sessionRun` (8012BE5C, `session_runner.cpp`) calls 8012A910 (called
`SsInitHot` in that file; `SDK_SOURCES.md` shows it is game code) before
the arena load, only in modes 0..2 (attract, tournament, two-player
tournament) and not on the Dark Tooth stage. Challenge (3) and Deathmatch
(4) have no card screen. 8012A910 runs:

1. `screenInitCardSprites` (80129D34, `screen_deathmatch_result.c`):
   two 320x240 buffers at x 0 and 320; one POLY_FT4 per opponent.
2. `screenLoadCardImages` (8012A614, `scene_init.c`): per opponent,
   `SCREEN\<code>PLATE.TIM` into `fileioWorkBuffer2`, uploaded to that
   card's VRAM spot (taken back from the quad's tpage, UV and CLUT).
3. `screenDealCards` (8012A7B8, `scene_init.c`): loads
   `SCREEN\SHOWOPS<level>.TIM` (320x240 15-bit) and deals the cards one
   by one: a card snaps in every 11 frames (`screenDrawCard(i, 1, 0)`,
   sound 3, a random roll of 360 +- 0..6), then glides toward its slot
   (`mathBlendValue(.., 85)`); `screenCardSettle` (8012A014) copies the
   moving quad into the card. Each frame `screenDrawCard` (8012A0E0) draws
   the background with `screenDisplayImage`, the settled cards 0..i-1 and
   the moving card (template quad), into a 2-entry OT.

Only opponents get cards; the human players have none in retail.

### 6.2 Arrays and limits (CONFIRMED)

| What | Where | Size | Limit / what follows |
|---|---|---|---|
| `screenCardQuads` | 8019108C | 10 POLY_FT4 x 40 bytes: cards 0..8, **[9] = the moving-card template** | ends at 8019121C, where `screenCardOts` starts |
| template index 9 | `screenInitCardSprites`, `screenCardSettle`, `screenDrawCard` | literal 9 | a 10th card would be the template |
| `screenCardOts` | 8019121C | 2 OTs x 2 words | fine for any count (all cards go in one entry) |
| `screenCardSlotLayouts` | 80164618 | 9 counts x {u16 x[9], u16 y[9]} = 324 bytes, indexed `[opponents - 1]` by `screenCardSlotPos` (80129C7C) | ends at 8016475C = `sessionSettings`: 10+ opponents would read the settings as positions |
| `screenCardHalfSize` | 80129BC8 | switch on the opponent count | 10+ fall to `default`: 80 x 40, the size for 1-2 cards |
| opponent cars | settings +10..+19 | 10 bytes | +20 is the lives array |
| card art | `SCREEN\<code>PLATE.TIM` | 14 files, 8,736 bytes each: 8-bit 128x64 (64x64 VRAM), 256-colour CLUT | **no BBPLATE.TIM**: Dark Tooth has no card (he only appears on the Dark Tooth stage, where the screen is skipped) |

Card sizes by count (half width x half height, full card 2x): 1..2 80x40,
3 60x30, 4 50x25, 5..6 46x23, 7..9 38x19.

Slot positions (centre x, y on the 320x240 screen; from the EXE at
80164618):

| Count | Positions |
|---|---|
| 1 | (160,90) |
| 2 | (160,60) (160,160) |
| 3 | (88,70) (247,70) (160,160) |
| 4 | (88,60) (247,60) (88,160) (247,160) |
| 5 | (88,50) (247,50) (160,110) (88,160) (247,160) |
| 6 | (86,50) (249,50) (90,110) (245,110) (86,160) (249,160) |
| 7 | (59,40) (159,40) (259,40) (159,100) (59,160) (159,160) (259,160) |
| 8 | (59,40) (159,40) (259,40) (69,100) (249,100) (59,160) (159,160) (259,160) |
| 9 | 3 x 3 grid: x 59, 159, 259; y 40, 100, 160 |

### 6.3 Card art in VRAM (CONFIRMED)

`screenInitCardSprites` gives card i:

- tpage `10 + i / 4` with the 8-bit bit (value + 128): page x = 640 +
  64 * (i / 4), y 0; v = (i % 4) * 64. Four cards per 64x256 page.
- CLUT (768, 128 + i), one 256-entry row per card.

So 9 cards use pages at x 640, 704, 768 (only the first 64 rows of the
third) and CLUT rows 128..136 at x 768..1023. The TIMs' own VRAM x/y
(pixels (640,0), CLUT (0,480)) are ignored. This VRAM is used only during
the deal; `bitmapInit` in the arena load that follows starts VRAM over.

### 6.4 What has to grow for 16 cards (humans included)

Up to 16 cards (16 cars in all) fit the art scheme: 4 pages, x 640..895.
What has to change:

1. **Card count.** Every loop and table uses `sessionGetOpponents`. Use
   `cards = humans + opponents` in `screenInitCardSprites`,
   `screenLoadCardImages`, `screenDealCards`, `screenCardHalfSize` and
   `screenCardSlotPos`.
2. **Card source.** `screenLoadCardImages` takes the car from
   `sessionGetOpponentCar(i)`; human cards need `sessionGetHumanCar(j)`
   (settings +8+j). Opponents past 10 need a new settings array (+10..+19
   holds 10). Dark Tooth needs a fallback card (no file).
3. **Quads.** `screenCardQuads` moved to a larger array (16 cards + the
   template), and the literal 9 replaced by the new template index.
4. **CLUTs.** With more than 9 cards, CLUT rows 128.. at x 768 run into
   the art of cards 10 and 14 (page x 768 and 832, rows 128..191). Put the
   card CLUTs elsewhere, e.g. (768, 256 + i) in the lower half, which holds
   only dead menu data at that point (LIKELY), and set the quad's CLUT
   word to match.
5. **Layouts and sizes.** A new table for 10..16 cards. For 16 a 4 x 4
   grid with half size about 30 x 15 (cards 60 x 30) at x 40, 120, 200,
   280 and y 35, 90, 145, 200 fits 320 x 240; humans could take the first
   row or a marked colour. The 128x64 art scales down fine (the 9-card
   size is already 76 x 38).
6. **Time.** 11 frames per card plus the glide: 16 cards take about 3
   seconds, against about 1.7 for 9.
