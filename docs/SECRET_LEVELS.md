# Secret levels with computer players

Answers item 6 of the PC port's `docs/DECOMP_REQUESTS.md`. Can Suicide Swamp,
Cyburbia and Roof Tops be offered in any custom match, including 1 Player with
bots? How the codes unlock them is in `UNLOCKS.md`. How to start a match is in
`MATCH_START.md`.

The work was static analysis only: retail code (IDA pseudo-C and disassembly
cross-checked) and the files read straight from the retail disc image
(`cdimage/Twisted Metal 2 (USA) (Track 01).bin`). Nothing was run in an
emulator. **Confirmed** means the code or the disc data says so directly.
**Inferred** means it is reasoned from the code and has not been seen running.

## Short answer

- **Loading works.** The engine loads and runs all three levels in any mode.
- **Spawning works.** Spawn points and pickups are on the disc.
- **Computer cars can be added.** Nothing stops it. In retail, though, these
  levels never had computer cars: 2-player Deathmatch has 0 opponents. Their
  AI route data is a **stub that was never finished**. One byte is wrong in
  every route node, and Roof Tops' route table points at nodes that do not exist.
- **What the port has to supply** (details in section 6):
  - fix the route bytes in 27 nodes;
  - give Roof Tops a route table, or make its AI chase directly;
  - set the opponent count itself;
  - choose a mode that does not move on to "the next level".

## 1. Level ids and files

The level (arena) is settings +24 (`sub_80137814` (settingsSetLevel), `sub_80129AF8` (gameGetLevel)). The file
names are built by `sub_8012A998` (settingsLevelDmdFileName) (DMD) and `sub_8012AA5C` (settingsLevelTmsFileName) (TMS), using the
stem from `sub_8012ABD0` (levelFileStem). The arena setup `sub_8011995C` (arenaSetup) (`arena_setup.c`)
calls one loader per arena. Each loader loads TERRAIN `.PTS`, `.GRP` and
`.RMP`.

| Id | Name (`sub_8012ACB8` (levelDisplayName)) | DMD / TMS stem | TERRAIN stem | Arena loader |
|---|---|---|---|---|
| 1 | Los Angeles : Quake Zone Rumble | FREEWAY | FWY | `sub_80119D08` (arenaLoadLosAngeles) |
| 2 | Moscow : Suicide Slide | DISH | DISH | `sub_80119D4C` (arenaLoadMoscow) |
| 3 | Paris : Monumental Disaster | PARIS (PARIS2 in 2P) | PARIS | `sub_80119E24` (arenaLoadParis) |
| 4 | Amazonia : Fire Walk | ISLANDS | LAVA | `sub_80119DDC` (arenaLoadAmazonia) |
| 5 | New York : The Big Leap | ROOF (ROOF2 in 2P) | ROOF | `sub_80119EB4` (arenaLoadNewYork) |
| 6 | Antarctica : The Drop Zone | GLACIER (GLACIER2 in 2P) | ICE | `sub_80119D90` (arenaLoadAntarctica) |
| 7 | Holland : Field of Screams | DEN | DEN | `func_80119EFC` (arenaLoadHolland) (switch default) |
| 8 | Hong Kong : Hong Kong Krunch | HKONG | HKONG | `sub_80119E6C` (arenaLoadHongKong) |
| 9 | "It's a Secret" | "UNKNOWN LEVEL" (no file) | (DEN, switch default) | - |
| 10 | Florida : Suicide Swamp | SWAMP | SWAMP | `sub_80119F4C` (arenaLoadSuicideSwamp) |
| 11 | Los Angeles : Cyburbia | BURB | CYB | `sub_80119F94` (arenaLoadCyburbia) |
| 12 | Los Angeles : Roof Tops | SROOF | SROOF | `sub_80119FDC` (arenaLoadRoofTops) |

- **The "2" files** are used only in modes 2 and 4, on levels 3, 5 and 6 (`sub_8012A998` (settingsLevelDmdFileName)). The secret levels have a single file set, already used for both 1 and 2 players. (Confirmed.)
- **Level 9 would crash.** It has no files, so never select it.
- **Decomp note:** in `car_parts_spin.c`, the `case` labels of `sub_8012ABD0` (levelFileStem) and `sub_8012ACB8` (levelDisplayName) do not match retail. Retail's jump tables are not in order, and those tables are the ones that get linked. The table above follows retail (IDA plus the jump table at 800CFB58). (Confirmed.)

All per-level files exist on the disc for the secret levels (confirmed):

| File | Swamp | Cyburbia | Roof Tops | Normal levels |
|---|---|---|---|---|
| `DMD\<stem>.DMD` (models + objects) | 362,060 | 538,780 | 484,016 | 298,076 .. 473,196 |
| `TMS\<stem>.TMS` (textures) | 506,248 | 342,392 | 318,584 | 337,008 .. 526,552 |
| `TERRAIN\*.PTS` (route nodes, 3360 bytes) | yes | yes | yes | yes |
| `TERRAIN\*.GRP` (sections + route tables, 6036 bytes) | yes | yes | yes | yes |
| `TERRAIN\*.RMP` (surfaces, 6300 bytes) | yes | yes | yes | yes |

The secret levels do not lack any kind of file.

## 2. Spawning

**Normal matches do not use `spawn_tables.c`.** Its 8 tables (`sub_8011F284` (demoSpawnFreeway) .. `sub_801203EC` (demoSpawnHongKong), levels 1..8) belong to `sub_8011EFDC` (demoRespawnAiCars). That function only runs in "battle display mode" `D_80180FBC` (gameAttractDemoMode). `sub_8011ED88` (gameSetMode) sets that mode only for mode 0, the attract demo. (Confirmed.) If the secret levels were used in attract mode, the switch has no case for them. The cars would keep their start-list spawns, and the intro camera would stay on the cars. (Inferred: harmless.)

**Real spawns use a per-level start list.** Every match spawns from the list `D_801B2B28` (startNodeList) (`ai_start_nodes.c`). It holds up to 15 route-node ids, and the count is at `D_801B2B55` (startNodeCount). The route loader for each level fills it.

- **Picking:** `sub_800F636C(count)` (startNodePickRandom) picks an unused id at random. If all ids are used, it picks any id at random, so two cars can share a spot.
- **Humans:** `sub_800FD6C4` (humanCarPlaceAtStart) (`car_object_contact.c`) places them. It uses the node's x, y and z, and heads towards a random successor.
- **Computer cars:** `sub_800FFEFC` (aiCarStart) stores the node in +0x5D0, and `sub_800F6518` (aiPlaceOnRouteNode) (`ai_route_setup.c`) places them.

| Level | Start list (`ai_start_nodes.c`) | Start points |
|---|---|---|
| Swamp | `func_8010278C` (startNodesSwamp): nodes 0..7 | 8 |
| Cyburbia | `func_801028F4` (startNodesCyb): nodes 0..9 | 10 |
| Roof Tops | `func_80102A74` (startNodesSroof): nodes 0..8 | 9 |
| Normal levels | hand-picked subsets | 10..15 |

On the secret levels, every route node is a start point. With more cars than start points, cars share spots. Example: 9 or more cars on Swamp. (Confirmed: code. Effect: inferred.)

**Teleporters:** there are none on the secret levels. `sub_80115310` (worldWarpsSetup) has no case for 10..12. Moscow has none either. (Confirmed.)

## 3. Pickups

Pickups are objects 401..430 in the level DMD. The object loader `sub_800D1DB0` (worldLoadObject) (`world_load_object.c`) registers each one in a pickup slot (`sub_8010AE58` (pickupRegister), slot table `D_80183F7C` (pickupSlots), at most 50). Nothing in the EXE is per level. The DMD object trees were walked straight from the disc, with the same opcodes as `sub_8014ABF8` (dbscanHierarchy). (Confirmed.)

| Level | Weapons (401,403,404,406,411,413,419) | Full health 417 | 1/3 health 418 | Turbo 430 | Total |
|---|---|---|---|---|---|
| Swamp | 10 | 0 | 1 | 1 | 12 |
| Cyburbia | 42 | 1 | 0 | 5 | 48 |
| Roof Tops | 14 | 1 | 1 | 2 | 18 |
| Freeway (for comparison) | 27 | 1 | 4 | 4 | 36 |
| Hong Kong (for comparison) | 33 | 1 | 5 | 8 | 47 |

- **Effects** are applied by `sub_8010AFC8` (pickupApply) (`pickup_apply.c`): 417 refills health, 418 adds a third and 430 adds 100 turbo.
- **Deathmatch skips 418.** In mode 4 the loader skips object 418, so in retail Deathmatch Swamp has **no health at all**. In any other mode its one 418 appears.
- **Cyburbia is near the limit.** Its 48 pickups are close to the 50-slot limit, but still fit.
- **The AI does not use the slots.** (Corrected; see PICKUP_LOOKS.md 2.1.) Computer cars do not look for pickups through the slot table. They collect pickups only by driving over them. The x, y, z stored in each slot are never read.
- **Swamp total.** The table above first said 13 for Swamp; the parts add up to 12 (see PICKUP_LOOKS.md 1).

## 4. AI route data: where the stub is

### What a route node is

`D_801B1DE8` (aiRouteNodes) holds 140 nodes of 24 bytes. The PTS file is copied into it, and the count goes in `D_80180AD8` (aiRouteNodeCount) (`ai_route_loaders.c`). Layout:

| Offset | Field |
|---|---|
| +0 | x |
| +4 | y |
| +8 | z (s16) |
| +0x0A | 6 successor ids |
| +0x16 | successor count |
| +0x17 | **route byte** |

**The route byte is a section number.** The GRP file holds up to 35 boxes, called "sections". `sub_800EFF34` (worldSectionAt) (`car_wheel_sway.c`) gives the section that contains a point. That section is the "route" of a car: +0xC8 for a human, +0x46A for an AI car.

**The GRP also holds tables indexed [section][section].** At `D_801A9B9C` (aiRouteTransitionTables) (GRP+0x354) there are three 35x35 tables:
- next node;
- next section;
- link node.

Per section, the GRP also holds the floor height `D_801A985C` (worldSectionFloorHeights).

| Level | Nodes (`D_80180AD8` (aiRouteNodeCount)) | Sections (`D_80180AD4` (terrainSectionCount)) | Route byte in the nodes |
|---|---|---|---|
| Normal levels | 33..140 | 7..35 | the section that contains the node |
| Swamp | 8 | 2 (0 = a box, 1 = whole map) | **6 in every node** |
| Cyburbia | 10 | 2 (0 = a box, 1 = whole map) | **6 in every node** |
| Roof Tops | 9 | 14 (one per roof / ramp, 13 = whole map) | **6 in every node** |

Swamp and Cyburbia do not even have a section 6. The data shows the files are stubs (confirmed):

- **A copy of Cyburbia:** SWAMP.PTS still holds Cyburbia's nodes 8 and 9 after its own 8 nodes, and SROOF.PTS still holds Cyburbia's node 9.
- **Template value:** 6 is Holland's "whole map" section. Holland is the only level where 6 is the right value.
- **Nodes are in other sections:** retail's own section test puts the nodes in other sections:
  - Swamp: 0,0,0,0,0,1,1,1;
  - Cyburbia: 1,1,1,1,1,1,0,0,0,1;
  - Roof Tops: 11,10,8,8,8,8,0,3,6.

  On Roof Tops, each of those sections has a floor height equal to that node's z. So these are the intended values.
- **Bad node ids on Roof Tops:** the next-node table in SROOF.GRP names nodes up to 48, for example 12, 13, 14, 17, 25, 43 and 46. Only nodes 0..8 exist, and the rest of the node array is zero (x = y = z = 0). The table was made for a bigger node set that is not on the disc.
- **No link tables:** the link-node table (`D_801AA52E` (aiRouteLinkTargetTable)) is all zero on all three levels.

### What the AI does with it

Every effect below is inferred from the code. None has been seen running, because retail never runs these levels with computer cars.

1. **Route walking:** between attacks the AI walks the node graph:
   - `sub_800EAB50` (aiFollowRoute) (`ai_steps.c`) calls `sub_800E971C` (aiStartRouteFollow), which sets f448;
   - at each node, `sub_800F7B00` (aiPickLinkNode) (`ai_link_node_pick.c`) picks the successor nearest to an aim point from `sub_800F7D54` (aiFollowRouteTarget) (`screen_pause_menu.c`);
   - if the AI's node section equals the target's section, the aim point is the target;
   - otherwise the next-node table picks a node to aim at.
   - **Swamp and Cyburbia:** the node section (6) never equals the target's section (0 or 1). Row 6 of the table is all 0, so two AI cars in three (personality `ai[1] < 2`) aim at **node 0** instead of their target. The third kind aims at the target, but picks the farthest node.
   - **Roof Tops:** row 6 says node 17 (or 2 and 4 for some targets). Node 17 is empty, so most AI cars aim at **(0, 0)**, one corner of the map.
   - **Holland works:** there, nodes and targets are both in section 6, so the AI aims at the target.
2. **Spawn height:** `sub_800F6518` (aiPlaceOnRouteNode) puts a computer car at the floor height of the section in its node's route byte. It does not use the node's z (+0x744 = `D_801A985C[route]` (worldSectionFloorHeights)).
   - **Swamp and Cyburbia:** section 6 does not exist, so its data is zero and the floor is 0. The nodes are at z 0 or 56, so this is close enough.
   - **Roof Tops:** section 6's floor is 7200, but nodes 2..5 are at 8000. Those cars start 800 below the roof top. The next ground lookup may then find the "whole map" section (floor 0), and the car would fall to its death: below z 6000 on this level, `sub_8011EC0C` (worldFallLimitZ), which applies to AI cars as well (`ai_driver.c`). Humans are not affected, because `sub_800FD6C4` (humanCarPlaceAtStart) uses the node's own z.
3. **Stuck recovery:** a stuck AI car is moved to the nearest node of its own section (`sub_800EF48C` (carNearestRoutePoint), `car_wheels.c`). No node has the car's section, so it always returns node 0. Every stuck car on a secret level jumps to node 0.
4. **Following the target's route:** the "follow the route to the target" mode (`sub_800E97B0` (aiTryRouteToTarget), f455) needs the link table, which is all zero. It never starts. This does not matter: `sub_800E9A98` (aiRollTakeRoute) turns this mode on only on levels 3, 5 and 8, so Freeway and the other levels never use it either. (Confirmed.)

## 5. Other per-level code checked

Each switch or comparison on `sub_80129AF8()` (gameGetLevel) in `src/semantic` was checked for levels 10..12.

| Code | Levels 10..12 | Status |
|---|---|---|
| Fall-death height `sub_8011EC0C` (worldFallLimitZ) (humans and AI) | 12 uses 6000 (like New York). 10 and 11 use the default -8000 | confirmed; Roof Tops is ready for AI falls |
| Hidden health-refill square in `car_tick.c` (humans only) | has entries for 10, 11, 12 | confirmed |
| Per-level scripted objects (`weapon_slot_scan.c`, `target_actor_*`, `rt_main.c` events, `ua_init.c`) | no case: nothing scripted | confirmed |
| Tournament opponent count `sub_8012AFE8` (tournamentOpponentCount) | returns **0** | confirmed |
| AI scaling by level number | see below | confirmed formulas, effect inferred |
| Loading the arena from the session (`sub_8012BAFC` (sessionLoadArenaAndCars)) | generic | confirmed |

**AI scaling.** Several AI values grow with the level number. Levels 10..12 extend the tournament's level 1..8 scale. So computer cars there act like an even harder level than Hong Kong:
- **Strength:** byte +0x812 = `level*3 + difficulty*15 + base` (`sub_800D3C7C` (aiCarSetupProfile), `ai_route_nodes.c`), capped at 100;
- **Special timer:** the special-weapon interval `D_80180F44 = (80 - 4*level) >> difficulty` (weaponFireInterval) (`sub_80108CF0` (weaponsRoundReset), `round_reset.c`). It is 40 on level 10 and 32 on level 12;
- **Wait times:** two timers in `ai_steps.c`, `(8 - level)*60 + 180` and `(8 - level)*60 + 120` ticks. The second is 0 from level 10 on, and the first from level 11 on. At 0 or below, the AI does not wait at all.

The port may want to pass 8 to these three formulas on the secret levels.

## 6. What the port has to supply

1. **Fix the node route bytes.** After the PTS load (`sub_80102814` (routeLoadSwamp) Swamp, `sub_80102994` (routeLoadCyb) Cyburbia, `sub_80102B08` (routeLoadSroof) Roof Tops) and the GRP load, set byte +0x17 of each node:

   | Level | Route byte for nodes 0.. |
   |---|---|
   | Swamp | 0,0,0,0,0,1,1,1 |
   | Cyburbia | 1,1,1,1,1,1,0,0,0,1 |
   | Roof Tops | 11,10,8,8,8,8,0,3,6 |

   Or compute them with `sub_800EFF34(x, y, z)` (worldSectionAt) for each node. This fixes the spawn height (section 4, point 2) and stuck recovery (point 3).
   - **Swamp and Cyburbia** need nothing more. Rows 0 and 1 of their next-node table are -1, so after the fix the AI always aims straight at its target. That suits these open arenas. (Inferred.)
2. **Roof Tops needs a route table**, or direct chasing. Its next-node table (`D_801A9B9C[0]` (aiRouteTransitionTables), GRP+0x354) uses node ids that do not exist. Choose one:
   - **Simplest:** after the GRP load, write -1 to every entry that is 9 or more. Those AI cars then aim straight at the target. They may drive off the edges and fall below 6000.
   - **Better:** author a 9-node next-node / next-section table for the 14 sections, maybe with more nodes on the ramps. This is new data; there is nothing on the disc to recover.
3. **Opponents.** Set settings +5 yourself. `sub_8012AFE8` (tournamentOpponentCount) gives 0 for these levels. Mode 2 subtracts the humans from it, so it would wrap to 254 opponents.
4. **Mode.**
   - **1 Player with bots:** use challenge (mode 3). It plays rounds on one level and only counts wins.
   - **Avoid tournament modes 1 and 2.** On a win they announce `sub_8012ACB8(level + 1)` (levelDisplayName) and move on to the next level. After Roof Tops that is level 13, whose files are named "UNKNOWN LEVEL".
   - **2 players plus bots in mode 4:** the round never ends when all computer cars die. The "all opponents dead" check in `world_update.c` skips mode 4, so the port needs its own end rule. (Inferred from the condition.)
5. **Car count.** Start points limit the cars: 8 on Swamp, 10 on Cyburbia, 9 on Roof Tops. With more cars, two cars can start on the same spot.

   Memory may limit them too, if the port keeps the retail RAM layout. `sub_8012BAFC` (sessionLoadArenaAndCars) packs the level DMD and then every car's DMD from 0x80019C40. The space up to the EXE start, 0x800CDC54, is 737,300 bytes. (Inferred limit: Hong Kong's tournament, 9 cars in the worst case, fills 735,156 of it.)

   | Level | Level DMD | Cars that fit (largest car files) |
   |---|---|---|
   | Swamp | 362,060 | 10 |
   | Cyburbia | 538,780 | **5** |
   | Roof Tops | 484,016 | 7 |

   VRAM was not checked: car textures are uploaded per slot (`sub_80121A2C` (bitmapParseContainer)). The level TMS sizes are within the range of the tournament levels (Paris's 526,552 is the largest).
6. **Optional:** pass 8 instead of the level to the three AI scaling formulas (section 5), so that computer cars are not harder than on Hong Kong.

No new pickups, spawn tables or level files are needed.
