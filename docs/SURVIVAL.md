# Survival's enemies: bosses anywhere, one car type more than once, waves

Answers item 14 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(the Survival mode of the port's `docs/CUSTOM_BATTLE.md`: rounds of computer
cars on one arena, Minions on round 5, Dark Tooth and his heads on round 10,
several Minions and Dark Tooths at the end). It builds on `AI_LIVES.md`
(death, respawn, enemy count) and `SIXTEEN_CARS.md` (arrays, registries,
VRAM, load area) and does not repeat them. Static analysis only: the C in
`src/semantic`, checked against the retail disassembly (`bin/SCUS_94306R.EXE`,
`toolchain/mips/bin/mipsel-none-elf-objdump`) and the disc files under
`cdimage/` (read only; the DMD walk and the VRAM numbers come from scratch
scripts that follow `dbscanHierarchy` and `vramPlaceTexture`). Nothing was
run. Addresses are retail.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code, not traced to the end or not seen running.

Car types: Sweet Tooth 0x10, Hammerhead 0x20 .. Shadow 0xD0, Minion 0xE0,
Dark Tooth 0xF0 (`carIndexToType` 8012B044). A car DMD's object ids are
type + 0..15; "+n" below means type + n. "Group n" is the load group that
`sessionLoadArenaAndCars` (8012BAFC) passes to `bitmapParseContainer` and
`assetInstallImage` (1 = first human, then the other humans, then the
opponents in settings order; `vramTexSlotGroup` 80181050 holds it while a
car DMD is installed).

## Short answer

- **Dark Tooth is an ordinary computer car plus four level-8 gates and some
  Hong Kong map data.** The gates are in `roundStart` (route patch),
  `aiCarsCountRemaining` (stage 2), `aiWeaponFireDecide` (all his boss
  weapons: missiles and the head's swoop) and `weaponProjectileUpdate` (a
  missile wobble). The first two also require him to be `aiCars[0]`. On
  another arena today he drives and rams, but fires no boss weapons, his head
  floats unmoved where it was created, and after his death he has no stage
  2. Section 1.5 lists the patches.
- **The "heads" are not cars.** BB.DMD's three roots are the truck (0xF0),
  the head (object 249, `darkToothBossObject`: it hovers over the truck and
  swoops at the nearest human; in stage 2 it sits on the car, whose truck
  object is hidden), and the model of his ricochet shots (asset 2214).
  There is one head per loaded BB.DMD, and all the boss state is global. A
  "head" as its own enemy is a Dark Tooth car started directly in stage 2
  (1.6). CONFIRMED (code), LIKELY (the look).
- **Minion already works as a normal opponent on any arena.** Nothing in the
  game tests level 4 or Minion-as-boss; the boss stage is only the session
  runner reloading with Minion as the one opponent. One catch: MN.DMD
  registers two car world objects (of the 15), so six Minions need 12 slots
  unless the second is skipped (1.7). CONFIRMED.
- **Two cars of one type need one DMD load each and every per-type table
  keyed by load group.** The per-car looks (damage stage, tires, turbo
  flame, freeze) are switch nodes inside the model data, so two cars cannot
  share one loaded model. Section 2 lists 10 tables (writers, readers,
  sizes) and 7 functions that find a car by its type. The change: a table
  `carSlotGroup[15]` filled in `carRegisterModel`, the registries indexed by
  group, and the run-time callers passing the car's group instead of its
  type. LIKELY (design), CONFIRMED (the list).
- **Waves: park a car, do not shrink `aiCarTotal`.** Off = dead + HP 0 +
  wreck state 4 + object word +8 = 0 + range flag off + moved far away (Mr.
  Slam's grab, the tornado and ground fires test distance only). On =
  `AI_LIVES.md` 2.6. Nothing caches `aiCarTotal`; every reader is a loop
  bound read at the time. Keep it at the loaded count (3.4). CONFIRMED.
- **VRAM:** Dark Tooth takes about 77,000-78,600 texels of the lower half.
  With him, the number of other car types (humans included) that fit is 0 on
  Paris, 1 on Swamp, 1-2 on Hong Kong and Amazonia, 2-4 on Los Angeles, New
  York and Antarctica, 4-10 on Moscow, Holland, Cyburbia and Roof Tops
  (4.1). More Dark Tooths cost no VRAM. LIKELY (replay).
- **Slot stride:** any car loaded after a Dark Tooth (another Dark Tooth
  included) overwrites his texture slots 20..30. Fix: stride 32 in four
  functions (VAs in 4.2), or have the loader skip one group number after
  each Dark Tooth. CONFIRMED.
- **Memory:** BB.DMD is 61,660 bytes (the heads are inside it; there is no
  other file). Every Dark Tooth, Minion or duplicate is one more DMD copy.
  With the car DMDs moved above 0x80200000 (`SIXTEEN_CARS.md` 3.5) this
  stops mattering. CONFIRMED.

## 1. Dark Tooth and Minion as ordinary computer opponents

### 1.1 What BB.DMD holds (CONFIRMED, walk of `DMD\BB.DMD` with the rules of `dbscanHierarchy` 8014ABF8)

Each id goes to `worldLoadObject` (800D1DB0, `world_load_object.c`).

| Root | Id | Count | `worldLoadObject` case | Effect |
|---|---|---|---|---|
| 0 | 240 (+0) | 1 | `carRegisterModel(dbInitCsForAModel(obj), 240)` | the truck: one car world object, owner 0xF0. Unlike 16..224, case 240 does **not** set `worldLoadParentObject` |
| 0 | 241 (+1) | 1 | none | the damage-stage switch (op 9), found later by `carChoiceRegisterGroup` through the model |
| 0 | 242 (+2) | 8 | `carPartRegisterDarkTooth` (80114AA8) | type-5 nodes with part index 1..4 at +36 -> `carPartsDarkTooth[4]` (wheels; spun by `carPartsSpinDarkTooth`) |
| 0 | 243 (+3) | 1 | `carBodyObjectRegister` | `carBodyObjects[15]` (posed at the wheels' ground height each frame by `carBodyPoseUpdate`; LIKELY the shadow) |
| 0 | 244 (+4) | 2 | `objRegistryAdd` | nothing stored (no case for +4) |
| 0 | 246 (+6) | 1 | `objRegistryAdd` | `carTurboObjects[15]` |
| 0 | 248 (+8) | 2 | `clutAnimRegister(obj, 2)` | CLUT animation, id 248 |
| 0 | 2301 | 1 | `weaponStoreProjectileModel` | `weaponSpiralMissileModel` (80180A5C). Sweet Tooth's ST.DMD stores the same id: the last loaded wins |
| 1 | 249 (+9) | 1 | `weaponAssetBind(obj, 249)` (80109038 case 0xF9), then `worldLoadParentObject = obj` | **the head**: a new scene object (`csAlloc`, `csListAdd`), model = this node, word +8 = 1 (shown), pointer in `darkToothBossObject` (80180F40) |
| 1 | 247 (+7) | 1 | `carFreezeBuild(247, worldLoadParentObject)` | `carFreezeObjects[15]`, linked into the **head's** model (its parent is 249, since 240 does not set the parent) |
| 2 | 2214 | 1 | `weaponAssetBind(obj, 0x8A6)` | `weaponRicochetModelAlt` (80180F28): the model of ricochet shots (weapon kind 6) fired by a car of type 0xF0 (`weaponFireSelected` 80104330) |

There is no +5 (wreck model): `carWreckModels[15]` stays 0, and Dark
Tooth's death never asks for it (1.2). The file is 61,660 bytes; the car
DMDs hold one root, BB.DMD three.

### 1.2 A Dark Tooth fight in retail (CONFIRMED unless tagged)

| Step | Where | What |
|---|---|---|
| intro | `sessionRun` (8012BE5C, `session_runner.cpp`) level 8, first win | text screens, opponent slot 0 = 14, opponents = 1, `sessionLoadArenaAndCars` + `sessionBindCars` (he becomes `aiCars[0]`), `audioSetDarkToothMusic` (music mode 11, `game_mode_audio.cpp`). There is no in-engine cut-scene or camera hook: only text, the reload and the music flag |
| each round | `roundStart` (8011A024) | level 8 and `aiCars[0].type == 0xF0`: `aiRoutePatchBossLevel` (800F6080, `ai_route_setup.c`): rewires Hong Kong route nodes 0..117 and the route transition tables |
| stage 1, every frame | `aiWeaponFireDecide` (80103CA0, `weapon_tick_init.c`) | level 8 and type 0xF0: machine guns off, `aiDarkToothBossUpdate(car)` (8010FD44, `ai_dark_tooth_boss.c`): a type-61 missile (`weaponFireDarkToothMissile` 800D9D38, 5 damage, spiral model) every 10..75 updates from alternate sides; the head hovers 240 above the truck, sways, and every 100..200 updates swoops at the nearest human (`humanCarNearest`, within 4,000), hits for 7 and sets the car burning. `aiWeaponsUpdate` (800D3BB8) runs the weapon tick for type 0xF0 even out of range |
| stage 1 death | `carApplyDamage` (800E5628, `car_damage.c`) 800E5C60.. | type 0xF0 always takes the "heavy" path: debris burst of his own model (30 pieces), **wreck state 3** (not 4), explosion, and his object's word +8 = 0 (hidden) |
| | `carDestroyed` (8011BE10) 8011C160 | wreck timer 360 (others 120) and `carApplyDamage(own id, 15, 0, 2, 0)` |
| wait | `aiCarsCountRemaining` (8011C248, `scene_init.c`) | no AI car has HP > 0, level 8, `aiCars[0]` is 0xF0, `aiBossDefeatedFlag` 0: count stays 1; spin-outs while the wreck timer >= 121 |
| stage 2 | same, wreck timer <= 0 | `aiBossDefeatedFlag` (8018102C) = 1, `carStatsDarkToothStage2(&aiCars[0].stats)` (800FF5BC, `arena_setup.c`: max HP and HP 110, new tuning), `carBuildCollisionBox`, dead = 0, `roundOverFlag` = 0, `rtScheduleEvent(3, 0)` (the round ends at once), position clamped into Hong Kong's playfield |
| "But wait..." | `sessionRun`, `dark_tooth_reveal` | text, then a new round **without** `sessionBindCars`: the AI record keeps its stage-2 state; `roundStart` restores the humans |
| stage 2, every frame | `aiDarkToothBossUpdate` | hover height -24: the head sits on the car. The truck object stays hidden (nothing shows it again), so the head is what is seen. `aiCarDamageUpdate` (HP 110 > 40 %) resets the damage stage and shows the body object. The truck's word +8 = 0 also takes it out of car-car collisions (`hdpPassRun`); projectiles still hit it (they skip only word +8 = 0 **and** dead). LIKELY (the look) |
| stage 2 death | `carApplyDamage` | `aiBossDefeated()`: the debris is the head's model (10 pieces), the head hidden |
| win | `worldUpdate` 8011B5AC.. | `aiBossDefeatedFlag`: the head's word +8 = 0, event 3 after 210 (others 420) |

`arenaSetup` (8011995C) clears `aiBossDefeatedFlag` at every
`sessionBindCars`.

### 1.3 Tied to level 8 or to `aiCars[0]` (CONFIRMED)

| VA (test) | Function, file | Test | On another arena today | Patch for Survival |
|---|---|---|---|---|
| 8011A054, 8011A064 | `roundStart`, `scene_init.c` | level 8 and `aiCars[0].type == 0xF0` -> `aiRoutePatchBossLevel` | skipped (right: the patch is Hong Kong route data) | keep the level test; on Hong Kong drop the `aiCars[0]` part only if the port wants the boss routes with Dark Tooth at another index |
| 8011C2B4, 8011C2C4 | `aiCarsCountRemaining`, `scene_init.c` | level 8, `aiCars[0]` is 0xF0, flag 0 | no stage 2: he dies as a normal car | replace with a per-car stage loop (1.6) |
| 80103F6C, 80103F84 | `aiWeaponFireDecide`, `weapon_tick_init.c` | level 8 and type 0xF0 -> `aiDarkToothBossUpdate` | no missiles, no swoop; machine guns stay on; the head is never moved | test type 0xF0 only, with per-car boss state (1.6) |
| 800DB348, 800DB360 | `weaponProjectileUpdate`, `projectile_hit.c` | level 8 and the missile's owner is `carObjectFromOwner(240)` | a type-61 missile wobbles the other way. Cosmetic | drop the level test; compare with the shooter's own object |
| 8011B610 | `worldUpdate` round end | `aiBossDefeatedFlag` -> hide the head, 210 ticks | (flag never set) | per car (hide each head at its stage-2 death) |
| `carStatsDarkToothStage2` | `aiCarsCountRemaining` | `aiCars[0]` | | call it with the right car |

Other level checks that apply to **any** car on that arena (not Dark Tooth
logic; listed so the port knows they come with the arena): Hong Kong:
`worldUpdate` 8011A72C track vehicles, `aiRollTakeRoute` 800E9AAC (always
take the route), `aiMovementUpdate` 800E91CC (the special-manoeuvre timer
`(8 - level) * 60`), the special choice in `ai_steps.c` (level 8 or type
0x20), `carNearestRoutePoint` 800EF4C4 (routes 4..7 read as 24),
`worldMovingSurfaceCount` 8011ED08, `carDestroyed` 8011C0B0 (sank below z
-40), lightning (`weaponFireSelected` 80104540: levels 1, 2, 8). Paris:
`aiCarDamageUpdate` (`aiCarsRemaining < 3` opens the tower links,
`AI_LIVES.md` 1.1).

### 1.4 Dark Tooth code keyed only on type 0xF0 (CONFIRMED)

These run on every arena. "HK data" = written for Hong Kong's node numbers or
coordinates.

| VA | Function, file | What | On another arena |
|---|---|---|---|
| 800EB104, 800F6DB4 | `aiFollowRoute` (`ai_steps.c`), `aiAdvanceRouteNode` (`ai_route_nodes.c`) -> `aiDarkToothJumpSelect` (800F6DDC, `car_restart.c`) | on route nodes 2, 40, 11, 44 and inside y -8799..-5601, x >= -5279 (retail's test, CONFIRMED at 800F6E40..800F6EC4): jump flags. `aiFollowRoute` then spins him out (12 or 13), gives him speed and sets the goal node to 0x75 (117) or 0x70 (112) | **HK data.** Wherever those node numbers fall in that box he jumps toward nodes 112/117, which do not exist on arenas with fewer nodes (Moscow 33, Swamp 8, Cyburbia 10, Roof Tops 9: reads past the route table). Patch: return early unless level 8 |
| 800F7F4C, 800F7FE8 | `aiFollowRouteTarget` (800F7D54, `screen_pause_menu.c`) | on nodes 66/68/70/72 with route group < 4 and the target's >= 5: `worldWarpStart(1)`, route 24; at node 0 with route >= 4 and target < 4: `worldWarpStart(2)`, route 3 | **HK data** (Hong Kong's warps and route groups). Elsewhere he can be teleported through that arena's warp 1, or warp 2, which only Hong Kong sets up (`worldWarpsSetup` 80115310; on others its positions are stale or 0). Patch: level 8 only |
| 800E0A88 | `carContactApplyObject` (800E06CC, `car_object_contact.c`) | any contact entry not handled above it: all four probes on it -> `worldWarpStart(2)` (stage 1), two probes -> spin out | same warp-2 problem if such an entry exists on the arena (LIKELY rare: on Hong Kong these are the boss's markers). Patch: level 8 only |
| 80115688 | `worldWarpStart` (8011566C, `fx_debris_splash.c`) | type 0xF0 never takes warp 0 | harmless |
| 800E81E8 | `aiCarDrive` (800E7EDC, `ai_driver.c`) | target in close range and slower: top speed 25, spin out (11) or turn toward it when the target's y >= -9759 | HK constant in a harmless place: nearly always true elsewhere |
| 800E9C24, 800EA424.. | `aiSetRouteAimPoint`, `aiFreeSteer` (`ai_steps.c`), `aiShouldChaseDirect` 800EBE1C | lane offsets, body spin to aim, an extra chase test | generic, works anywhere |
| 800E5998 | `carApplyDamage` | takes damage after the round is won | harmless |
| 800E5C60, 800E5D08 | `carApplyDamage` | death: always a debris burst, wreck state **3** | **bug outside the boss logic:** wreck state 3 is never followed by 4, so `carWreckUpdate` (800E75F4) keeps calling `carApplyDamage(own id, 15, ...)` once the timer is out and the target far (or after 22 s), and each call bursts again: endless debris and explosions. Patch: after the final death set wreck state +0x501 = 4 (or do it in the per-car stage code). LIKELY (not run) |
| 8011C160 | `carDestroyed` | wreck timer 360 and a self-hit | fine |
| 800E6E2C.. | `carCrashStart` (`car_crash.c`) | a car he rides over is not frozen | fine |
| 800E0F9C, 800E10A8, 800E14FC | `carContactResolve` | a burning car does not set him alight; HK tram damage | fine |
| 800D73CC | `projectileHitTest` (800D6FD8, `weapon_slam_swing.c`) | a larger hit size for type 0xF0 | fine |
| 80104330 | `weaponFireSelected` | his ricochet shots use the 2214 model | fine |
| `weaponFireSpecial` 0xF0 case | `weapon_basic.c` | **no special weapon** (empty case) | fine |
| 800D3DBC | `aiCarSetupProfile` (800D3C7C, `ai_route_nodes.c`) | weights `aiDarkToothWeights9/10`, fire chance 100 | fine |
| `carStatsDarkTooth` 800FF4BC, `weaponBlockInit` 800FDE84, `carDefaultTopSpeed`, `carWheelsSetStepHeight` | | stats (300 HP), weapon capacity, step 0xA0 | fine |
| 800FCD48 | `hudRadarPlotCar` | blip colour | fine |
| | `worldUpdate`, `carSoundUpdate` (8011DB74) | no engine sound for 0xF0 (no case) | fine |

### 1.5 What breaks on another arena, and the patches

With BB.DMD loaded as one opponent on any arena and no patch (LIKELY,
from the tables above):
1. the head is created shown at its initial position (`weaponAssetBind`
   sets word +8 = 1) and never moved: a head standing still somewhere on
   the map, most likely at the origin;
2. no boss missiles or swoop, machine guns instead;
3. Hong Kong jump and warp code can fire on matching node numbers;
4. at his death no stage 2, and the endless burst of 1.4.

Patches (each a small C change or a hook):

| # | Where | Change |
|---|---|---|
| 1 | `aiWeaponFireDecide` 80103F6C | type 0xF0 only (no level); the boss update per car (1.6) |
| 2 | `aiCarsCountRemaining` 8011C2B4.. | per-car stage loop (1.6); no `rtScheduleEvent(3, 0)`, no Hong Kong clamps (or place him on a start node with `aiPlaceOnRouteNode`) |
| 3 | `aiDarkToothJumpSelect` 800F6DDC | clear the two flags and return unless level 8 |
| 4 | `aiFollowRouteTarget` 800F7F4C, 800F7FE8 | the two warp branches only on level 8 |
| 5 | `carContactApplyObject` 800E0A88 | the type-0xF0 tail only on level 8 |
| 6 | `carApplyDamage` 800E5C60.. | wreck state 4 after a final death (stage 2 dead, or no stage 2) |
| 7 | `weaponProjectileUpdate` 800DB348 | drop the level test (cosmetic) |
| 8 | `roundStart` 8011A054 | leave as is |

Then he is a normal opponent with his boss weapons, on any arena. The
music flag (`audioSetDarkToothMusic` 800D3850) is the port's choice.

### 1.6 Several Dark Tooths, and "heads" as enemies

All boss state is global, one copy (CONFIRMED, `names.tsv`):

| VA | Name | What |
|---|---|---|
| 80180B4C..80180B58 | `aiDarkToothShotDelay`, `aiDarkToothShotTimer`, `aiDarkToothFireSide`, `aiDarkToothSwoopTimer`, `aiDarkToothYawStep`, `aiDarkToothPitchStep`, `aiDarkToothSwoopTarget` | missile and swoop timers, head sway, the human swooped at |
| 80180F18..80180F24 | `darkToothSwoopDelay`, `darkToothSwooping`, `darkToothHoverHeight`, `darkToothSwoopTarget` | swoop state |
| 80180F40 | `darkToothBossObject` | the head (last BB.DMD installed wins; earlier heads stay in the world list, shown and unmoved) |
| 80183F64 | `darkToothSwoopVectors[6]` | swoop push |
| 8018102C | `aiBossDefeatedFlag` | stage 2 (readers: `aiBossDefeated` 8011C3EC <- `carContactApplyObject` x3, `carApplyDamage`, `aiDarkToothBossUpdate`; `worldUpdate`) |
| 80180B84 | `carPartsSpinAngleDarkTooth` | wheel spin angle (per type, 2.2) |

Two Dark Tooths calling `aiDarkToothBossUpdate` share one head and run its
timers twice per frame. For several Dark Tooths:
- a per-car boss record (the fields above, the stage, and the head
  pointer), indexed by the AI index;
- the head of each car: take `darkToothBossObject` right after each BB.DMD
  is installed (or store it by `vramTexSlotGroup` in `weaponAssetBind`
  case 0xF9), and keep it with that car;
- `aiDarkToothBossUpdate(boss)`: read and write the car's record instead
  of the globals (a C change of one function; the shot owner and the
  swoop's attacker are already the car's);
- `aiBossDefeated()`: called with the car, or replaced at its 5 call sites;
- stage change per car, in the port's `aiCarsCountRemaining`: for each
  type-0xF0 car with HP <= 0 and stage 0: count it as remaining; when its
  wreck timer +0x522 <= 0: stage 1, `carStatsDarkToothStage2(&car->stats)`,
  `carBuildCollisionBox(car, 0)`, dead +0x06C = 0. The truck stays hidden
  (it already is) and the head comes down to -24 by itself. At the stage-2
  death: hide that car's head (word +8 = 0) and set wreck state 4.

**A "head" as its own enemy** (the port's rounds 10 and 15): a Dark Tooth
car started in stage 2. After `aiCarStart`/`carBindAiSlot`: stage 1 in its
record, `carStatsDarkToothStage2`, `carBuildCollisionBox`, its truck object
hidden (`csSetDrawEnable(obj, 0)`: it stays a target for projectiles, since
it is not dead), its head shown. It needs its own AI record, its own car
world object and its own BB.DMD copy (the head model is in BB.DMD; root 0,
the truck, could be skipped only with a cut-down image the port builds).
LIKELY (design; the pieces are CONFIRMED).

Count for round 15 as planned (3 Dark Tooths + 9 heads): 12 AI records
(retail has 9: `SIXTEEN_CARS.md` 1), 12 car world objects plus the humans
(retail limit 15), 12 BB.DMD copies (740 KB: needs the moved load area).

### 1.7 Minion (CONFIRMED unless tagged)

MN.DMD (32,736 bytes, one root): 224 (+0) **twice**, 225 damage switch,
226 parts x12 (`carPartsMinion[6]`), 227 body object, 229 wreck model,
230 turbo, 231 freeze, 232 CLUT animations x3. The second 224 is a node
inside the damage switch; `carRegisterModel` registers it as a second car
world object with owner 0xE0. It is never bound (kind 2, drawing off,
`carFromSlot` returns 0 for it), so it is harmless today, but it takes one
of the 15 slots, and a "first free slot of this type" binder would pick it
for a second Minion. Skip a second registration from the same group
(2.5).

In retail Minion is a computer car only in tournament level 4's second
stage: `sessionRun` sets opponent slot 0 = 13, opponents = 1, reloads and
rebinds (`TOURNAMENT_FLOW.md` 2). The random picker never picks 13
(`UNLOCKS.md` 1). The only test of level 4 in game code is in
`sessionRun` (grep of the C and of `gameGetLevel` compares in the
disassembly). So the "boss" is only the text and the reload.

What his AI uses, all keyed by type and level-free:

| What | Where |
|---|---|
| profile: weights `aiMinionWeights9/10`, fire chance 100 | `aiCarSetupProfile` 800D3C7C |
| special: three coloured missiles (10 each) and a freeze missile (7) | `weaponFireSpecial` (80105A7C, `weapon_basic.c`) case 0xE0; the AI fires it through `aiWeaponFireDecide` (selected >= 8) |
| stats (210 HP), step height 0x58, top speed | `carStatsMinion` 800FF3B8, `ai_car_start.c` tables |
| six spinning wheels | `carPartsSpinMinion` 801145E8 via `carPartsAnimate` |
| no burning ground effect for an AI Minion | `car_wheels.c` (type 0xE0) |
| engine sound 0x41, radar colour | `carSoundUpdate`, `worldUpdate`, `hudRadarPlotCar` |

So Minion works as a normal opponent on every arena as it is. Two or more
need section 2 (CONFIRMED code, LIKELY not run). His special has no global
state (no single shared object as Twister's tornado or Shadow's strike), so
six Minions firing at once only fill the shared projectile pool (16
records, `COOP_LIVES.md` 1.2). LIKELY.

## 2. Two or more cars of one type

### 2.1 Why one DMD per car (CONFIRMED)

The per-car looks are state inside the loaded model: the damage stage is the
current-state byte (+7) of the model's switch node (`carSetDamageStage`),
the tire state the same in the tire switches, the turbo flame an animation
step (`animSetStep`), and the freeze look a 264-byte record linked into the
model (`carFreezeBuild` writes its address into the parent node). Two cars
drawing one loaded model would always look the same. So each car needs its
own copy of its DMD (as `sessionLoadArenaAndCars` already does: one load
per car, relocated where it lands). Textures are free (VRAM reuse by key,
`SIXTEEN_CARS.md` 4.2).

What then goes wrong is that each copy registers into the same per-type
entries, and the binders and some run-time code find "the car of type t".

### 2.2 Tables keyed by type (CONFIRMED)

"Key" = how the index is made today. Sizes in entries. Callers are `jal`
sites in retail.

| Table (VA) | Size | Key | Written by | Read by | Change |
|---|---|---|---|---|---|
| `carSlotOwners` (80187A98) | 15 x s32 | holds the type | `carRegisterModel` (8011961C) for each +0 id | `carFindSlotByOwner` (8011D8F4) <- `carBindAiByType` (8011A54C) <- `sessionBindCars` 8012BE28; `carBindHumanByType` (8011A29C, own loop) <- 8012BDE0; `carObjectFromOwner` (8011D15C, 2.3); `carBindAiSlot` / `carBindHumanSlot` (type to start the car); `arenaSetup` (`carChoiceRegisterGroup` per slot) | keep the type; add `carSlotGroup[15]` (2.5); bind by group |
| `carBodyObjects` (801873D0) | 15 x ptr | `id >> 4` | `carBodyObjectRegister` (80114EEC) +3; cleared by `carPartsReset` (80114380) | `carBodyObjectHide` (80114E88: `carApplyDamage` 800E5BA4, `carGroundEffects` 800ED764, `weaponFireSpecial` 801067B0 (fixed 0xA0), `weaponInvisibilityUpdate` 8010DED8, `weaponTornadoUpdate` 8010EDD4 (fixed 160), `humanCarDamageUpdate` 8011BAEC, `aiCarDamageUpdate` 8011BDA0); `carBodyObjectShow` (80114E10: 800ED8B0, 8010DF28, 8010EDE4, 8011B970, 8011BCA8); `carBodyPoseUpdate` (80114F24: `humanCarTick` 800DCCC4, `aiCarDrive` 800E85A4) | index by group; the callers pass the car's group |
| `carWreckModels` (8018740C) | 15 x ptr | `id >> 4` | `carWreckModelRegister` (801152A8) +5; `carPartsReset` | `carWreckModelGet` (801152E0) <- `carApplyDamage` 800E5DE4 | by group |
| `carDamageStageObjects` (8018F4E4) | 16 x ptr | `id >> 4` | `carChoiceRegisterGroup` (801246A8) <- `arenaSetup` 801199B0, once per car slot (the root model's first child, the +1 switch) | `carSetDamageStage` (80124718: `carApplyDamage` 800E5E78, `humanCarDamageUpdate` 8011B95C/8011B9F0/8011BA74, `aiCarDamageUpdate` 8011BC94/8011BCFC/8011BD54) | by group (or simply by slot: it is already registered per slot) |
| `carTireSwitchObjects` (8018F524) | [16][3] ptr | the switch node's own id (+2) `>> 4` | `carRegisterTireSwitch` (8012478C) +2 (op 9 nodes; Hammerhead's and the others' +2 parts go to `carPartRegister*` instead) | `uaswToggleCarTireState` (80124800) <- `carPartsAnimate` 80114C3C | by group |
| `carPartsTireTurnAccum` (80187344) | 15 x s32 (run-time) | `kind >> 4` | `carPartsAnimate` (80114B70) <- `humanCarTick` 800DCC00, `aiCarDrive` 800E83DC | same | by group |
| `carTurboObjects` (8018F5E4) | 16 x ptr | `id >> 4` for +6 | `objRegistryAdd` (80123F78); cleared by `objRegistriesReset` | `objRegistrySetStep(type + 6, 0, on)` (80124384) <- `humanCarReadInput` 800DEC80, `aiInputFinalize` 800EC384, `weaponHomingLeapUpdate` 8010F6E0/8010F8D8/8010F948 (fixed 0x66, Grasshopper); `carFreezeResetAll` (loop 16) | by group; the turbo callers pass the group; the leap passes the leaping car's group |
| `D_8018F624` | 16 x ptr | +7 | `objRegistryAdd` | `objRegistrySetStep` | none: +7 ids go to `carFreezeBuild`, never to `objRegistryAdd`, so this table is never written (LIKELY unused) |
| `carFreezeObjects` (8018F950) | 16 x 264 bytes | `id >> 4` | `carFreezeBuild` (80124C40) +7 (also links the record into the parent model) | `carFreezeSetState` (80124C00: `projectileHitTest` 800D7254/800D7570, `humanCarTick` 800DCA44, `aiCarDrive` 800E814C); `carFreezeResetAll` (80124B80: `roundStart` 8011A044, `demoRespawnAiCars` 8011F024; loop 16) | by group (N x 264 bytes) |
| part sets: `carPartsHammerhead[4]` 801872FC, `carPartsMrSlam[4]` 8018730C, `carPartsMinion[6]` 8018731C, `carPartsDarkTooth[4]` 80187334, `carAxelObjects[2]` 80180F50; spin angles 80180B78..80180B88 | one set per type | the type | `carPartRegisterHammerhead/MrSlam/Minion/DarkTooth/Axel` (80114xxx) +2 | the `carPartsSpin*` functions via `carPartsAnimate` | per group: one set and one angle per group |

Also by type, without stored objects: `hudRadarPlotCar` (colour),
`carSoundUpdate` (engine sound id per type: two cars of one type play and
stop the **same** sound id; `worldUpdate` stops it when one of them is out
of range; LIKELY audible), the CLUT animations (`clutAnimRegister(obj, 2)`
for +8, `clutAnimStartFadeOut(type + 8)` for the human invisibility fade:
the CLUT is shared in VRAM anyway), and the weapon models stored by asset
id (2301 spiral missile, 2214, 2308/2322, 2309, 2321, 2323, 2324, 2325,
2326, 2327): a second copy only replaces the pointer with identical model
data, except where a new object is allocated per install: 249 (the Dark
Tooth head, 1.6) and 2325 (`weaponAssetBind` 0x915, Shadow's strike object:
the first one is left behind, hidden).

### 2.3 Run-time code that finds a car by its type (CONFIRMED)

`carObjectFromOwner(type)` (8011D15C) returns the **first** slot of that
type. Its users:

| Caller | VA | Key passed | With two cars of the type |
|---|---|---|---|
| `carWreckUpdate` (800E75F4, `car_crash.c`) | 800E7BE4 | the wreck's own type | breaking up the second car's wreck damages the first car's object instead. Fix: use `aiCarObjects[index]` |
| `carSpinOutByObject` (800F4A58) | | `spin_target` (the type of the car Mr. Slam holds; 192 for Mr. Slam himself) | 11 calls from `weaponSlamSwingUpdate` / `weaponSlamGrabUpdate` |
| `carSpinOutById` (800F5D20) | | same, and the tornado victim's type | `humanCarTick`, `aiCarDrive`, `weaponSlamRelease`, `weaponSlamGrabUpdate`, `weaponTornadoUpdate` x2, `carDestroyed` x2 |
| `carDamageByObject` (800F4B74) | | `spin_target` | `weaponSlamSwingUpdate` x2, `weaponSlamGrabUpdate` |
| `carCrushContactUpdate` (800F39D0, `proximity_update.c`) | | `crush_owner` (the type of the car being ridden, set in `carCrashStart`) | `humanCarTick`, `aiMovementUpdate` |
| `weaponProjectileUpdate` | 800DB35C | 240 | 1.3 |

Fix for all: store the slot (or the object id +0x118) instead of the type
in `spin_target` (written in `screen_card_draw.c` 228/267 and the slam
code) and `crush_owner` (`car_crash.c` 111), and look the object up by slot.

Single shared specials (one object or state for the type, CONFIRMED):
Twister's tornado (`weaponTornadoState`, hides `carBodyObjects[160 >> 4]`),
Shadow's strike (`weaponStrikeState`), Grasshopper's leap
(`homingLeapState`, turbo object 0x66), Mr. Slam's arm and claw
(`slamArmRegister`/`slamClawRegister`, one angle), and Dark Tooth (1.6).
Two cars of these types firing their specials at once share them. Minion
has none.

### 2.4 Binders (CONFIRMED)

`carBindAiByType` takes the first slot of the type; a second AI of that type
binds again to the same slot without `aiCarTotal++` (`SIXTEEN_CARS.md`
1.3). `carBindHumanByType` likewise. With `carSlotGroup`, bind by group:
human i = group i + 1, opponent i = group humans + i + 1 (the order of
`sessionLoadArenaAndCars`), and `aiCarTotal++` once per bound AI.

### 2.5 The change, step by step (LIKELY; each step is a small C change)

1. New `carSlotGroup[15]` (or N); `carRegisterModel` stores
   `vramTexSlotGroup` in it, and skips an id whose group already has a slot
   (Minion's second 224). Set `vramTexSlotGroup = 0` before the level DMD
   walk (`worldLoadStartObjects` does not set it; the level holds no car
   roots, so this is only hygiene).
2. Grow the per-type tables of 2.2 to N groups (N = 1 + cars; 12 today,
   17 for 16 cars) and move them where there is room (they are packed
   against their neighbours, `SIXTEEN_CARS.md` 1.1).
3. Registration functions: index by `vramTexSlotGroup` instead of
   `id >> 4` (`carBodyObjectRegister`, `carWreckModelRegister`,
   `carRegisterTireSwitch`, `objRegistryAdd` for +6, `carFreezeBuild`,
   the five `carPartRegister*`); `carChoiceRegisterGroup` from `arenaSetup`
   with `carSlotGroup[i]`.
4. Run-time functions: take the group instead of the type
   (`carBodyObjectHide/Show`, `carBodyPoseUpdate`, `carWreckModelGet`,
   `carSetDamageStage`, `uaswToggleCarTireState`/`carPartsAnimate`, the
   `type + 6` path of `objRegistrySetStep`, `carFreezeSetState`); the
   callers listed in 2.2 compute it as `carSlotGroup[slot]`, slot =
   `aiCars[i]` +0x006 (`aiCarObjectSlots[i]`) or
   `humanCarObjectSlots[player]`. The car type itself stays in the records
   (stats, specials, sounds and AI all switch on it).
5. `carFreezeResetAll`: loop N.
6. Section 2.3 (lookups by type) and 2.4 (binders).
7. The pickers: allow repeats (`carIsSelectable`, `SIXTEEN_CARS.md` 1.2).

Per car this costs one DMD copy (2.1; section 4.3) and, for a duplicate,
no VRAM.

## 3. Waves: turning a loaded computer car off and on

### 3.1 Off ("parked")

Do it after `sessionBindCars` (which binds and starts every loaded
opponent) for the cars of later waves, and at the end of a wave for any car
the port wants out. Per AI car i:

| Step | Field | Stops | Conf. |
|---|---|---|---|
| 1 | dead +0x06C = 1, **then** HP +0x5A0 = 0 | the count (`aiCarsCountRemaining`, `aiCarsCountAlive`: HP > 0), radar, lock-on, target panel, lightning, taser, ricochet, homing leap, Axel's push (all test HP > 0 or dead, `AI_LIVES.md` 1.3). Dead first, so `aiCarDamageUpdate` does not run `carDestroyed` (no explosion, sound, wreck) | CONFIRMED |
| 2 | wreck state +0x501 = 4 | `aiCarShouldUpdate` returns 0: no `aiCarDrive`, no physics (+0x59C = 0), skipped by the AI pack logic and by `areaDamage` | CONFIRMED |
| 3 | `csSetDrawEnable(aiCarObjects[i], 0)` (word +8 = 0) | drawing, car-car collision (`hdpPassRun`), projectiles and mines (skip word +8 = 0 and dead) | CONFIRMED |
| 4 | in range +0x034 = 0 | engine sound (`worldUpdate` stops it), gun loop (`soundStopSharedLoop`), damage (`carApplyDamage` returns) | CONFIRMED |
| 5 | position +0x73C far away (for example z = -1,000,000) | Mr. Slam's grab, the tornado and ground fires, which test only distance (`AI_LIVES.md` 1.3) | CONFIRMED (the tests), LIKELY (the value is safe: distances there are abs sums of s32) |
| 6 | `carBodyObjectHide` (its group), the freeze look and turbo flame off, burning +0x086 = 0 and the flame effect off (weapons +0x84C `active` = 0) | leftovers on the map | CONFIRMED |
| 7 | the port's own "parked" flag | its count rule (3.3) | |

Nothing global is touched. A Dark Tooth also needs its head hidden (word
+8 = 0) and its boss update skipped (it is called from the weapon tick,
which no longer runs once the car is not updated). CONFIRMED.

### 3.2 On

`AI_LIVES.md` 2.6, steps 1-6 (`aiCarStart(&aiCars[i], type, 1)` puts it
on a start node with full health and clears dead; then the damage stage,
the body object, wreck state 0, word +8 = 1), with the group of 2.5
instead of the type once section 2 is done, and the start-node picker of
`AI_LIVES.md` 2.5 (every start entry is taken after the match start). For
a Dark Tooth also reset its boss record (stage 0 or, for a head, stage 1 as
in 1.6) and show its head. The same frame it drives, is drawn, counted and
shown on the radar. LIKELY (not run).

`carBindAiSlot` (8011A5CC) is a second way in: it binds AI record i to any
loaded car slot and starts it. The port can use it to have one AI record
drive a different loaded car in a later wave (9 records, up to 15 loaded
cars): park the old slot (3.1, then `carSlotKinds` = 2 and
`carSlotIndices` = -1 for it), bind the new one with `carBindAiSlot`
directly (not `carBindAiByType`, which raises `aiCarTotal` for a free
slot). LIKELY.

### 3.3 Round end between waves (CONFIRMED)

The round is won when `aiCarsRemaining <= 0` (`worldUpdate` 8011B5AC). A
parked car has HP 0, so the last car of a wave ends the round. Two ways:
- one round for the whole run: count the parked cars of later waves in the
  port's `aiCarsCountRemaining` (as `AI_LIVES.md` 3.2 counts a car with a
  life left) until the last wave, and start the next wave from the port's
  hook before the AI loop (where `demoRespawnAiCars` is called). The HUD's
  "Enemies: N" (`aiCarsCountAlive`) then shows the cars alive now; give it
  the same rule if it should show the wave's total;
- one round per wave: in modes 1/2 the runner starts a new round without
  `sessionBindCars` (AI records keep their state, as after a lost life:
  `session_runner.cpp`), so the parked cars stay parked. In modes 3/4 it
  calls `sessionBindCars` every round, which rebinds and restarts **every**
  loaded opponent: park again after it.

Paris: `aiCarDamageUpdate` opens the tower links when fewer than 3 cars are
counted and a dead car exists, so with parked cars it happens as soon as a
wave is small. It only opens routes and is idempotent. CONFIRMED.

### 3.4 `aiCarTotal` (CONFIRMED, every access to 8018101C / gp+0x648)

| Function | Access | Use |
|---|---|---|
| `carTablesClear` 80119848 | write 0 | arena load |
| `carBindAiByType` 8011A54C | read, `++` | first bind of a slot |
| `roundStart` 8011A024 | 3 reads | attract mode only (`rand() % total`, loops) |
| `worldUpdate` 8011A700 | 8 reads | AI pack loops, the AI update loop, the gun-loop stop loop |
| `carsPhysicsUpdate` 8011E220 | 8 reads | contact and collision loops |
| `aiCarsCountRemaining`, `aiCarsCountAlive`, `aiCarNearestAlive`, `aiApplyDifficulty`, `aiCarObject` | reads | loops, a dead read |
| `aiCarCount` 8011D0E0 | getter, 14 callers | the weapon, lock-on, slam, tornado, fire and pedestrian loops; `aiCarSetupProfile` (Challenge with one AI: fire chance 100, at bind time) |
| `viewWatchNextAiCar`, `demoWatchNextAiCar`, `demoRespawnAiCars` | reads | view 7 (unreferenced) and attract mode |

No function keeps a copy across frames; the HUD, radar and sound loops read
it each frame. It may change during a match without anything going stale.
But lowering it does not take a car out: its object stays drawn and solid
(the hit pass, `carPairsUpdate` and the slot loops go by world objects and
`carWorldObjectCount`), only the parked car's updates stop, and the cars to
remove would have to be the highest indices. **Recommendation: keep
`aiCarTotal` at the loaded count and park cars (3.1).** CONFIRMED (readers),
LIKELY (recommendation).

### 3.5 Limits for the planned rounds

| Limit | Today | Planned rounds |
|---|---|---|
| AI records | 9 (`aiCars`) | rounds 13 (12 enemies) and 15 (12) need `SIXTEEN_CARS.md` 1.7 |
| car world objects | 15, humans included (`carRegisterModel`); Minion takes 2 unless 2.5 step 1 | 6 Minions = 12 slots as is; 3 Dark Tooths + 9 heads + 4 humans = 16 |
| loaded cars | every car the run uses is loaded at the start; with record reuse (3.2) the run can use up to 15 cars with 9 records | |
| VRAM, memory | section 4 | |

## 4. Dark Tooth's VRAM and memory

### 4.1 VRAM next to other cars (LIKELY, replay of `vramPlaceTexture` on the disc files)

His textures (`SIXTEEN_CARS.md` 4.3): 31 images, 77,056 own texels, 11
own 256-colour and 15 own 16-colour CLUTs. Placed after the level, they
take 77,056..78,592 texels of the lower half (the placement leaves small
gaps). A second or third Dark Tooth reuses them all (reuse by key).

"Others" = how many more different car types fit with him, humans
included, before the first "Relocatable texture not loaded"; types added
largest first (the worst order) or smallest first; Dark Tooth loaded after
them (as retail) or first. "Rows freed" = with `bitmapInit` reserving
(0,0,640,240) instead of (0,0,640,256) (`SIXTEEN_CARS.md` 4.6 option 2).

| Level | TMS | Free lower half | Others, largest first | Smallest first | DT first, largest first | Rows freed: largest / smallest / DT first |
|---|---|---|---|---|---|---|
| 1 Los Angeles | FREEWAY | 114,112 | 2 (CLUT) | 4 (pixels) | 2 (CLUT) | 3 / 4 / 3 |
| 2 Moscow | DISH | 177,472 | 8 (pixels) | 8 (CLUT) | 7 (pixels) | 10 / 11 / 10 |
| 3 Paris | PARIS | 86,176 | **0** (pixels) | 0 (pixels) | 0 (pixels) | 0 / 0 / 0 |
| 3 Paris, 2P | PARIS2 | 89,760 | **0** (pixels) | 1 (pixels) | 0 (pixels) | 0 / 1 / 0 |
| 4 Amazonia | ISLANDS | 140,288 | 1 (CLUT) | 1 (CLUT) | 1 (CLUT) | 6 / 7 / 5 |
| 5 New York | ROOF | 117,120 | 2 (CLUT) | 4 (pixels) | 2 (CLUT) | 3 / 4 / 3 |
| 5 New York, 2P | ROOF2 | 136,880 | 3 (CLUT) | 5 (CLUT) | 2 (CLUT) | 5 / 6 / 5 |
| 6 Antarctica | GLACIER | 142,144 | 2 (CLUT) | 4 (CLUT) | 2 (CLUT) | 6 / 7 / 6 |
| 6 Antarctica, 2P | GLACIER2 | 143,680 | 2 (CLUT) | 4 (CLUT) | 2 (CLUT) | 6 / 7 / 6 |
| 7 Holland | DEN | 167,168 | 7 (pixels) | 4 (CLUT) | 5 (pixels) | 9 / 10 / 8 |
| 8 Hong Kong | HKONG | 116,640 | 1 (CLUT) | 2 (CLUT) | 1 (CLUT) | 3 / 4 / 2 |
| 10 Suicide Swamp | SWAMP | 95,104 | 1 (pixels) | 1 (pixels) | 1 (pixels) | 1 / 1 / 1 |
| 11 Cyburbia | BURB | 176,192 | 8 (pixels) | 9 (pixels) | 7 (pixels) | 10 / 11 / 10 |
| 12 Roof Tops | SROOF | 186,432 | 9 (pixels) | 10 (CLUT) | 8 (pixels) | 11 / 12 / 11 |

So:
- Paris and Swamp cannot hold Dark Tooth beside even one human car type
  (beyond what freeing CLUT rows gives); Survival with Dark Tooth there
  needs more VRAM (`SIXTEEN_CARS.md` 4.6 option 6) or another arena;
- every car type of the whole run is loaded at the start, so the run's
  distinct types (humans + all waves + Dark Tooth) must fit at once;
- retail itself is at the edge: on Hong Kong two large human cars plus Dark
  Tooth (the co-op boss stage) fail on CLUT rows in the largest-first order
  (the same finding as `SIXTEEN_CARS.md` 4.4's "Dark Tooth + MG + IR").

### 4.2 The slot stride (CONFIRMED)

Slots are `group * 20 + image`, and an image index can be 0..31. Car TMS
files are all parsed before any car DMD is installed (4.1 of
`SIXTEEN_CARS.md`), so a car parsed after a Dark Tooth at group n
overwrites slots `(n+1)*20 + 0..10` = his `n*20 + 20..30`; when his DMD is
then remapped, his images 20..30 take that car's textures. This includes a
second Dark Tooth (its images 0..10 land on the first one's 20..30: wrong
images, since the slot content differs). Cars parsed before him are not
affected. Retail always loads him last.

The four places (the `group * 20` multiply is the first shift-add-shift in
each; the second one in each function is the 20-byte slot record size):

| Function | VA | File | Group multiply |
|---|---|---|---|
| `modelRemapTextures` | 80120744 | `world_collapse_warps.c` | 80120794..8012079C |
| `vramPlaceTexture` | 801209E8 | `screen_deathmatch_result.c` | 80120A38..80120A50 |
| `vramUploadClut` | 80120D3C | `scene_init.c` | 80120D84..80120D8C |
| `bitmapAddRecord` | 80121764 | `bitmap_record_append.c` | 80121788..80121790 |

No other function indexes `vramTexSlots` (gp-relative access census).

Fixes:
1. **Stride 32** in the four (C: `* 20` -> `* 32` at the `idx`/`slot`/`n`
   lines). The slot table (packet buffer 0 + 0x10004) has room for 1,599
   20-byte slots, so up to 49 groups at stride 32. Simplest and covers every
   order.
2. **No code change in the four:** in the port's
   `sessionLoadArenaAndCars`, advance the group number by 2 after each Dark
   Tooth, in both the TMS pass and the DMD pass (the group is only a
   counter there). Costs one group number per Dark Tooth (79 groups fit at
   stride 20). The registries of section 2 then use these group numbers
   too.

### 4.3 Memory (CONFIRMED sizes)

BB.DMD is 61,660 bytes and holds the truck, the head and the ricochet
model; the heads have no file of their own. MN.DMD is 32,736. Every car is
one DMD copy, duplicates included (2.1). BB.TMS is only read during the
load (into 0x80019C40, before the level DMD).

Room in the retail load area (737,300 bytes, `SIXTEEN_CARS.md` 3.3) after
the level DMD, one large human car (39,168) and Dark Tooth, and with three
Dark Tooths:

| Level | After the level | + 1 human + 1 DT: left (more large cars) | + 1 human + 3 DT: left (more large cars) |
|---|---|---|---|
| 1 Los Angeles | 315,188 | 214,360 (5) | 91,040 (2) |
| 2 Moscow | 313,392 | 212,564 (5) | 89,244 (2) |
| 3 Paris | 330,248 | 229,420 (5) | 106,100 (2) |
| 4 Amazonia | 264,104 | 163,276 (4) | 39,956 (1) |
| 5 New York | 266,712 | 165,884 (4) | 42,564 (1) |
| 6 Antarctica | 427,760 | 326,932 (8) | 203,612 (5) |
| 7 Holland | 439,224 | 338,396 (8) | 215,076 (5) |
| 8 Hong Kong | 320,904 | 220,076 (5) | 96,756 (2) |
| 10 Suicide Swamp | 375,240 | 274,412 (7) | 151,092 (3) |
| 11 Cyburbia | 198,520 | 97,692 (2) | does not fit |
| 12 Roof Tops | 253,284 | 152,456 (3) | 29,136 (0) |

With the car DMDs loaded above 0x80200000 (`SIXTEEN_CARS.md` 3.5), the
planned run (for example 4 humans, 3 Dark Tooths, 9 heads as Dark Tooth
cars, 6 Minions and about 12 other cars) is about 1.6 MB of DMDs, which the
port's 8 MB holds. The other pools still apply (128 coordinate systems:
each Dark Tooth adds his car object, his head and his parts; 16 projectile
records; `SIXTEEN_CARS.md` 1.6). LIKELY.

## 5. Notes

- `carChoiceRegisterGroup` only registers a root whose first child is a
  switch node, so Minion's second 224 object (2.2, 1.7) does not overwrite
  the damage-stage entry. CONFIRMED.
- `aiCarStart` for a Dark Tooth reloads stage-1 stats (`carStatsLoad` ->
  `carStatsDarkTooth`), so a respawn or a wave restart always gives the
  truck; a head must be set to stage 2 after it (1.6). CONFIRMED.
- Dark Tooth's swoop and missiles target humans only (`humanCarNearest`,
  `weaponFindTargetAhead/Behind`), as every AI does outside attract mode.
  CONFIRMED.
- The Dark Tooth music (`audioDarkToothMusicFlag` 80180A05) is cleared by
  `audioClearDarkToothMusic` at session teardown; it alternates tracks in
  `worldUpdate` while set. CONFIRMED.
