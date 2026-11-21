# Request 33 (and request 32 step 3): plan

**For the PC port: the questions are in section F (15), the answers in section G. Request 32 step 2 (16 cars) is built
separately; this plan covers step 3 and request 33.**

Read-only planning pass, 2026-10-12, tree at `15184e7` (main after PR #30, step 1 merged). Sources:
the port's requests 32 (with its answers Q1-Q15) and 33; `docs/CAR_GROUPS_PLAN.md`, `docs/CAR_GROUPS.md`,
`docs/SURVIVAL.md`, `docs/SIXTEEN_CARS.md`, `docs/TEXTURE_BANKS.md`, `docs/TEAM_COLORS.md`,
`docs/FOUR_HUMANS.md`, `docs/SOURCE_BUILD.md` ("Port variant", "Port code"); the C in `src/semantic`
(file:line below are at `15184e7`); `out/TM2_SOURCE_E0/symbols.tsv` for addresses (retail VAs).
Tags: **CONFIRMED** = read in code; **LIKELY** = follows from code, not traced to the end; **CHECK** = the
executing agent must verify before relying on it. Nothing was built or run.

---------------------------------------------------------------------------------------------------------

## 0. Decisions this plan makes (the port can overrule them, section F)

**D5. Per-car special state lives "per key", and the first car of a type keeps the retail storage.**
Every special-weapon state that is one global today becomes one entry per car key (`CAR_GROUPS.md` 1),
with this layout:

- key index 1..15 (the *first* car of each type, key = type): **the retail global itself** (only one key
  per type is < 0x100, so the retail variable is never shared by two keys);
- key index 16..31 (duplicates): a 16-entry port array in `.port_bss` (`portCarSpecials`, one struct).

An accessor `TM2_SPECIAL(field, key)` (macro or port function) picks one or the other. With no duplicate in
the match every special runs on the retail variable, at the retail address, in the retail order: the port's
frame-by-frame comparison stays exact *including* the retail addresses of the special state (no new
"expected difference" as step 1's `freeze` pointer). The same rule is used for the pools that hold
record-visible pointers (Mr. Slam's claw records, 1.2 below): retail entries first, port entries after.

Why per key and not per record: the objects these specials drive are nodes of the car's own DMD (Mr.
Slam's arm 201/claw 202, the tornado 2323 and funnel 160, Shadow's strike 2325, Axel's 2324, Dark Tooth's
head 249). They exist once per loaded DMD = once per key. Driver state (timers, owner pointers) goes with
it, because a key has exactly one driver at a time (the swap of step 3 resets both keys involved).

**D6. Updates that run once per frame without a car argument become loops over active keys, retail key
first.** `weaponTornadoUpdate`, `weaponHomingLeapUpdate`, `weaponShadowStrikeUpdate` are called from
`worldUpdate` (`world_update.c:455-457`). Under the define each keeps its retail call site and becomes
"run the retail body on the state of the type key, then on each overflow key whose state is active, in
key order". With no duplicate only the retail body runs: identical `rand()` sequence.

**D7. Anything that changes VRAM placement, the sound engine's lists, or `rand()` order with no duplicate
present is behind a run-time bit of `portCarOptions`** (as D3): bit 3 per-car CLUTs, bit 4 per-car engine
sound. Per-key specials (D5) need no bit: they are retail-equal by construction.

---------------------------------------------------------------------------------------------------------

## A. Census: state kept once per type (or once globally) that a car's special, weapons or run-time look uses

### A.1 Already per car (verified; nothing more to do)

| What | Where | Status |
|---|---|---|
| body (shadow), wreck, damage stage, tire switches + accumulator, turbo flame, spinning parts + angle, freeze record | `portCarGroups` per key (`CAR_GROUPS.md` 3) | per key since step 1. Verified in code: `carPartsAnimate` (`car_parts_spin.c:70-86`) uses `tire_accum[key >> 4]` and `uaswToggleCarTireState(key >> 4)` → `portCarGroups.tire` (`car_switch_registry.c:81-101`); freeze and parts by key at `car_tick.c:63/102`, `ai_driver.c:60/101` |
| freeze *state* (`state.frozen`, `stats.freeze_timer`) | car records | per record (retail) |
| fire (`state.burning`, `weapons.fire_effect`) | car records; the flame is an `fxSprites` record (`fxFlameBurning`, 150-entry pool, `fx_ground_fires.c:106/124`, `car_contact_resolve.c:263/274`, `weapon_slam_swing.c:79`, `weapon_flame_update.c:117`, `ai_dark_tooth_boss.c:152`) | per record (retail); the pool is shared by everything (A.4) |
| invisibility state (`weapons.invisible`, `invis_timer`) | human records (`weaponInvisibilityUpdate` loops humans only, `ai_link_node_pick.c:268`) | per record; **the CLUT fade is not** (A.2 row 9) |
| Mr. Slam's grab state (`active`, `spin_target`, `spin_pending`, `half_damage`) | an 18-byte claw record per weapons block (`weapons.slam`, +0x48) | per record, **but from a 4(+1)-entry pool** (A.2 row 2) |
| Minion's special, Sweet Tooth/Warthog/Grimm/Spectre/Roadkill missiles, Outlaw's taser | projectile records / arc slots | no type-global state (`SURVIVAL.md` 1.7); only pools (A.4) |
| object ids, killer / last-attacker bytes | object +0x118; human stats +0x07/+0x08 (`cars.h:252-253`) hold **slots** | per car (but see B.4 "credits") |

### A.2 Kept once per type or once globally (each breaks with two cars of the type)

Addresses retail (E0 `symbols.tsv`). "Users" = the functions that read or write it (file:line of the
definition or the use). "Fix" = per key unless stated (D5).

| # | State (VA, size) | Users (file:line) | What breaks with two cars of the type | Per-car fix |
|---|---|---|---|---|
| 1 | **Mr. Slam's arm and claw**: `slamArmObject` 80180F74, `slamClawObject` 80180F78 (ptrs), `slamArmAngle` 80180F7C, `slamClawAngle` 80180F80 (s32) | registered: `worldLoadObject` ids 201/202 (`world_load_object.c:296-297`) → `slamArmRegister` 80115188 (`arena_load_register.c:437`), `slamClawRegister` 801151D0 (`:447`); cleared: `carPartsReset` 80114380 (`fx_debris_splash.c:196-199`); get: `slamArmGetAngle` 80115218, `slamClawGetAngle` 80115224 (`global_accessors.c:198/203`); set (+ `RotMatrixYXZ` into the object): `slamArmSetAngle` 80115230 (`misc_one_liners.c:522`), `slamClawSetAngle` 8011526C (`arena_load_register.c:154`); callers: `weaponSlamSwingUpdate` 800F4058 (`weapon_slam_swing.c:223-338`), `weaponSlamGrabUpdate` 800F4CC0 (`weapon_arcs_draw.c:77-197`), `weaponSlamCarryTarget` 800F59A4 (`car_wheel_sway.c:301/337`, arm angle → the held car's lift), `carStatsLoad` 800FE15C case 0xC0 (`car_stats.c:162-163`), `weaponsRoundReset` 80108CF0 (`round_reset.c:111-112`) | the arm and claw of the **last-loaded** Mr. Slam are the only ones registered: the other Mr. Slam's crane never moves, and both slammers drive (and fight over) one angle pair; a second slam during the first jumps the arm; the held car's height follows the other slammer's arm | per key: objects and angles in `portCarSpecials.slam[16]` for overflow keys (D5). Registration by `portCarLoadKey()`; the six accessors get a key (new port functions `portSlamArmGet/Set(key, ..)`, `portSlamClawGet/Set`); `weaponSlamSwingUpdate`/`GrabUpdate` pass `TM2_CAR_KEY(slot, 0xC0)` of `car`; `weaponSlamCarryTarget` gets the carrier's key as a 6th parameter (its only callers are those two); `carStatsLoad`'s 0xC0 setters move to its two callers `humanCarPlaceAtStart` 800FD6C4 (`car_object_contact.c:394`) and `aiCarReset` 80100034 (`ai_route_setup.c:372`), which know the record (or a port `portCarKeyOfStats(s)`); `weaponsRoundReset` and `carPartsReset` loop over all keys |
| 2 | **Mr. Slam's claw records**: `weaponSlamRecords[4]` 80184818 (18 bytes each), cursor `weaponSlamStateCount` 80180AF0 | `weaponSlamStatesReset` 8010EE58 (`weapon_tables.c:50`, from `carTablesClear` `ua_init.c:141`), `weaponSlamStateAlloc` 8010EE64 (`weapon_tables.c:59`: cursor 0..4, **index 4 is the 5th record = `weaponRicochetPool` 80184860**, then wraps to 0), `weaponSlamStateInit` 8010EEA8 (`pickup_spawn.c:29`: allocates only when `weapons.slam` is null); callers `weaponBlockInit` (`ai_route_setup.c:311`), `carRestart` (`car_restart.c:193`), `aiCarStart` (`ai_car_start.c:297`), `carCarCollision` (`car_collision.c:164`), `weaponFireSpecial` (`weapon_basic.c:319`); reset loop in `weaponsRoundReset` | a 5th Mr. Slam gets the record that overlaps the ricochet pool; a 6th shares record 0 with the first (one grab state for two cars). Also LIKELY: `weapons.slam` is never cleared between loads, so a record kept from an earlier match is not re-allocated and can alias a new allocation | pool of `TM2_PORT_CARS_LOADED` records: indices 0..3 the retail `weaponSlamRecords` (record bytes, i.e. the `weapons.slam` pointers, stay retail), 4.. in port storage; the allocator never hands out the overlapping slot; `carTablesClear` (under the define) clears `weapons.slam` in every record before binding (CHECK: retail-equal only if retail always re-allocates; if retail keeps the pointer across loads, do the clear only when `portCarOptions & 1`) |
| 3 | **Twister's tornado**: `weaponTornadoState` 80184DC0 (0x14 bytes: timer, angle, kind, `anchor` = &firer hp, `flags` = &firer state, `object` = firer's car object), `weaponTornadoObject` 80180F38 (the tornado, asset 2323, `csAlloc`ed per install), `weaponTornadoFunnelObject` 80180F3C (a second object with **the Twister root model**, allocated at root id 160); scratch `weaponTornadoFxPos` 80180F14, `tornadoPushVector` 80183F38, `tornadoRotMatrix` 80183F44, const `weaponTornadoRotation` 80180B44 | allocated: `weaponAssetBind` 80109038 cases 0x913/0xA0 (`asset_pools.c`), from `worldLoadObject` (`world_load_object.c:125-127` root 160, `:190-191` id 2323); fired: `weaponFireSpecial` 80105A7C case 0xA0 (`weapon_basic.c:246-258`); run: `weaponTornadoUpdate` 8010E040 (`weapon_tornado.c:104`, called `world_update.c:455`); reset: `weaponsRoundReset` (`round_reset.c` tornado block), `weaponObjectsDetach` 80108F70 | one tornado for all Twisters: a second Twister cannot fire while the first spins (ammo refunded, `weapon_basic.c:263-265`); the tornado and funnel are the last-loaded Twister's objects, and the funnel draws **the last-loaded Twister's model** (its damage stage and CLUT copy) | per key: state + two object pointers (D5). `weaponAssetBind` 0x913/0xA0 store by `portCarLoadKey()`; `weaponFireSpecial` 0xA0 uses the firer's key; `weaponTornadoUpdate` loops (D6); round reset/detach over all keys. The scratch vectors may stay shared (used within one call) |
| 4 | **Shadow's strike**: `weaponStrikeState` 80184E10 (44 bytes: mode, owner id, active, timer, velocity, delta, previous), `weaponStrikeCornerOffsets` 80184E3C (48), `weaponStrikeOwner` 80184E6C (= the firer's weapons block), strike object 80184E70 (asset 2325, `csAlloc`ed per install: "the last install wins, the first is left behind hidden"); its animation object `objRegistryId2325` 80181068 | `weaponAssetBind` case 0x915 and `objRegistryAdd(2325)` (`world_load_object.c:193-195`, `object_registry.c`); fired/detonated: `weaponFireSpecial` 0xD0 (`weapon_basic.c:327-357`); run: `weaponShadowStrikeUpdate` 8010F9C0 (`strike_update.c:26-80`, `world_update.c:457`); `objRegistrySetStep(0x915, ..)` from both; resets as row 3 | one strike for all Shadows: a human Shadow pressing fire while **another** Shadow's strike runs detonates the other one's (`weapon_basic.c:330`: `active && MODE`); the AI's fire is refused (ammo refunded); one animation object | per key: the whole `TM2StrikeBlock` (0x64 bytes) + the 2325 animation pointer; `portStrikeSetStep(key, v)` replaces `objRegistrySetStep(0x915, 0, v)` in the two functions; update loops (D6) |
| 5 | **Grasshopper's leap**: `homingLeapState` 80184DD4 (0x3C bytes, inside the 0x50 block that starts at `weaponTornadoState`: active, source mode, owner slot, age, target, speed, health ptr, velocity, position/rotation/target ptrs) | `weaponHomingLeapInit` 8010EF44 (`weapon_leap_homing_init.cpp:72-132`, C++), `weaponHomingLeapUpdate` 8010F520 (`weapon_leap_homing_update.c:55`, `world_update.c:456`; reaches it as `weaponTornadoState` + 0x14), `weaponsRoundReset` (clears +0x14 active, +0x22 age), accessor `weaponHomingLeapState` 8010F9B0 (`global_accessors.c:188`, no caller found) | a second Grasshopper's leap overwrites the first one's mid-air (position, target, health pointers now the second car's): the first car's leap stops, its turbo flame stays on | per key (D5), update loops (D6); the turbo flame is already per key (`LEAP_TURBO`, `weapon_leap_homing_update.c:41`) |
| 6 | **Dark Tooth's head and boss state**: `darkToothBossObject` 80180F40 (the head, asset 249, `csAlloc`ed per install), `aiDarkToothShotDelay..SwoopTarget` 80180B4C..80180B58 (7 x s16), `darkToothSwoopDelay`/`Swooping`/`HoverHeight`/`SwoopTarget` 80180F18..80180F24, `darkToothSwoopVectors[6]` 80183F64, `aiBossDefeatedFlag` 8018102C (stage 2) | `weaponAssetBind` case 0xF9 (`asset_pools.c`); `aiDarkToothBossUpdate` 8010FD44 (`ai_dark_tooth_boss.c:53`, from `aiWeaponFireDecide` `weapon_tick_init.c:99`); `aiBossDefeated` 8011C3EC (`car_records.c:63`) ← `carContactApplyObject` (`car_object_contact.c:263/268/274`, the car is the Dark Tooth), `carApplyDamage` (`car_damage.c:271-273`, with `aiDarkToothBossModel` 801108F8 and `aiDarkToothBossObject` 8011090C); `aiCarsCountRemaining` 8011C248 (`scene_init.c:136-140`, `aiCars[0]` only); `worldUpdate` round end (`world_update.c:428-429`); `arenaSetup` clears the flag (`arena_setup.c:323`); `weaponObjectsDetach` (`round_reset.c:137`); `weaponProjectileUpdate` `carObjectFromOwner(240)` (first Dark Tooth, step 1) | `SURVIVAL.md` 1.6: one head (the last BB.DMD's; earlier heads stand still where created), the timers run twice per frame, one stage flag for all | per key: one boss record (the 11 scalars, the vectors, the head pointer, the stage flag); `aiDarkToothBossUpdate(boss)` uses `boss`'s key; `aiBossDefeated`/`Model`/`Object` get a key (the 5 call sites have the car); `aiCarsCountRemaining` and the round end keep retail's rule (the record of `aiCars[0]`) unless `SURVIVAL.md` 1.6's per-car stage loop is wanted (separate bit, F.6) |
| 7 | **Axel's shockwave animation**: `objRegistryId2324` 80181064 (asset 2324 of AX.DMD, last install wins) | `objRegistryAdd(2324)` (`world_load_object.c:197-198`), `objRegistrySetStep(0x914, 0, 1)` in `weaponFireSpecial` 0xB0 (`weapon_basic.c:273`) | the shockwave animates on the last-loaded Axel's model, whichever Axel fires | per key (`portCarSpecials.axel_anim`), `portAxelSetStep(key, 1)` |
| 8 | **Thumper's flame emitters**: `weaponFlameEmitters[2]` 80184774 (32 bytes); sound 47 by id | `weaponFlameSlotFind` 8010A5AC, `weaponFlameStart` 8010A5F0 (`car_restart.c:266`), `weaponFlameUpdate` 8010A7A0 (`weapon_flame_update.c:48`, sound stop `:138-139`), `weaponsRoundReset` | a pool, not per type: a 3rd Thumper flaming at once gets nothing (returns 0); the flame sound 47 is one id (plays/stops once for all) | grow to `TM2_PORT_CARS_LOADED` (retail 2 first, D5 style); sound: acceptable as is (one loop while any emitter runs) |
| 9 | **The invisibility CLUT fade**: one definition per type (`clutAnimRegister(obj, 2)` for ids type+8, `world_load_object.c:156-158`; `clutAnimNewDef` refuses the second car's, `engine_clut_anim.c:69`), keyed `(type + 8, 0)`; started/ended by `weaponInvisibilityUpdate` (`clutAnimStartFadeOut(type + 8, 0, 20)` / `clutAnimFadeIn(type + 8, 0)`); tables `clutAnimDefs[20]` 80197E58, `clutAnimRunning[10]` 80197FE8 (0x424 each), `clutAnimLastIndex` 80180D2C | `clutAnimRegister` 801489B4, `clutAnimNewDef` 80149018, `clutAnimFindDef` 801490B0, `clutAnimFindRunning` 80149120, `clutAnimFreeSlot` 801491A0, `clutAnimStartFadeOut` 80148740, `clutAnimFadeIn` 80148868, `clutAnimUpdate` 80148B1C, `clutAnimInit` 80148660 (`engine_clut_anim.c`) | the CLUT is the first car's (shared in VRAM, `TEAM_COLORS.md` 1.3): one car's invisibility fades its twins; a second fade started on the same row saves an already-faded palette as "original" and restores it faded (LIKELY). For Spectre, Roadkill and Axel the faded CLUT is the **shared undercarriage** (image 1), so their fade also fades the undercarriage of every other car type that has that key (retail, `TEAM_COLORS.md` 2.5) | needs per-car CLUTs first (C.1); then one definition per key (id `key + 8`, so the first car keeps `type + 8`); the pools grown (retail 20/10 first, D5 style) |
| 10 | **Team colours** (the port's): today it overwrites the type's own body CLUT row (way C) | the port; rows from the slots during the load (`TEAM_COLORS.md` 3) | every car of the type takes the colour | per-car CLUTs (C.1) + a published table of each group's CLUT positions (the slots die with the first frame, `TEAM_COLORS.md` 1.4) |
| 11 | **Engine sound**: one sound id per type (0x24..0x3C even, Sweet Tooth 0x3C, Minion 0x41; none for Dark Tooth); each id's request list holds **1** request (`soundMgrInitLists` 801281EC, `sound_manager_init.c:39-73`: these ids fall in the "max 1" branch); `soundPlay` = request with instance 0; same-id requests merge (`sndListMergeById` 8015E19C, `engine_sound_lists.c:295`) | `carSoundUpdate` 8011DB74 (`screen_card_draw.c:28`, engine switch `:104-123`: `soundPlay(snd, vol, pan, pitch)` every frame while in range, else `soundIsActive(snd)` → `soundStop(snd)`); `worldUpdate` (`world_update.c:373-393`: an AI out of range stops its type's id); `soundIsActive` 800D3638 (one flag per id, `weapon_fire_specials.c:123`), `soundStop` 800D3464 (instance -1), `soundMgrPlay` 80128890, `sndListSubmit` 8015E57C | two cars of a type are one voice whose pan and pitch jump between the two cars every frame; one car out of range (or with `st[0]` set) stops the other's engine | per-car instance ids + a list capacity > 1 + "merge by instance first" (C.3); bit 4 |
| 12 | **Weapon models stored by asset id** (`weaponStoreProjectileModel` 800D6F00 `weapon_special_missiles.c:174`: 2201/2202/2208/2301/2321/2322/2326; `weaponRicochetModelAlt` 80180F28 = 2214 of BB) | `worldLoadObject` `:187-188` | the last-installed copy's model; identical data, so only its CLUT/bank differs: Roadkill's boomerang, Grimm's skull etc. are drawn with the last-loaded twin's CLUTs | none needed (the special images are not paint; with C.1 a twin's copy has identical content). With texture banks (step 4) the bank is the last twin's: identical content, fine |
| 13 | **Shadow render list**: `renderWorldModels[20]` 801A4E1C (`controls.c:282` `renderAddWorldModel`, silently drops past 20) | `carBodyObjectShow` (`ai_route_helpers.c:154`) adds each car's body (+3, shadow) object; the level adds 686/689/3103/684/691 objects (`world_load_object.c:237-265`) | not per type, but with 16 cars plus a level's world models the later cars' shadows are dropped (LIKELY on levels with several splash/blast objects) | grow to 48 under `TM2_PORT_CARS` (retail 20 first; the walker `engine_render_scene.c:190` loops the count) |
| 14 | **First Dark Tooth for missile wobble**: `weaponProjectileUpdate` 800DB35C `carObjectFromOwner(240)` | `projectile_hit.c` | a second Dark Tooth's type-61 missiles wobble the other way (cosmetic) | compare with the shooter's own object (`SURVIVAL.md` 1.5 #7); belongs with row 6 |

Also checked, nothing per type to fix: `carHasMeleeSpecial`, `aiCarSetupProfile` weights, `carStats*`,
`weaponBlockInit` constants, `hudRadarPlotCar` colours (read-only per-type data: matter for mods, section E);
`carFreezeSetTemplate` (692, the level's ice template, shared by design); `lightFlickerTimers` (per object);
`weaponArcs` and the ricochet/shield/drop pools (A.4).

### A.3 What "engine sound" needs in detail (CONFIRMED unless tagged)

- `soundPlay(a,b,c,d)` = `command(a,b,c,d,-1,0)` and `soundPlayVariant(a,b,c,d,e)` = `command(..,e,0)`
  (`command_wrappers.cpp:28-29`). In `soundMgrPlay` (`sound_manager_play.c:180-196`) variant -1 builds the
  request with `sndObjSetupFromRecord(obj, rec, 0, 0)`, variant v with `(obj, rec, v, 0)`: **so
  `soundPlay` ≡ `soundPlayVariant(.., 0)`** (LIKELY: `command` is `soundMgrPlay` for list-managed ids;
  CHECK the vtable slot).
- `sndListSubmit` inserts while `count < max` and only **merges by id when the list is full**
  (`engine_sound_lists.c:397-426`). With max 1 an engine request merges every frame into the one request
  (pitch/pan updated). Raising max alone would add a new request every frame until full: per-car engines
  need "merge by instance first, else insert while room".
- `soundStop(id)` = `soundMgrStop(type, -1)` stops every live request of the id and clears the id's active
  flag (`sound_manager_reap.c:12`); `soundStopInstance(id, i)` stops instance i (and clears the same flag).
  There is no per-instance "is active".
- Instance ids already in use by other ids: 21 (skid/shield hum) by player index, 9 (gun loop) by AI
  index, 0x33 (boomerang) by projectile slot. Engine ids use none.

### A.4 Pools (per car already; size-limited, not per type)

`weaponRicochetPool` 10 (80184860), `weaponShieldPool` 4 (80184A68), `weaponMineObjects` 23 (80184AA8),
`weaponRemoteBombObjects` **2** (80180F30), `weaponDrops` 25 (80184B04), `weaponArcs` 5 (801847B4: taser
and lightning), `groundFires` 30, `fxSprites` 150 (80185144), projectiles 16, sound pool 35 (`main`'s stack,
`shell_main.c` `slots[35]`), `csObjectPool` 128, `clutAnimRunning` 10, `renderWorldModels` 20,
`weaponFlameEmitters` 2, `weaponSlamRecords` 4 (sound pool 64 with `TM2_PORT_CARS` since 33-4). Q11 already raises projectiles, coordinate systems, radar
blips and sound instances; this plan adds `weaponSlamRecords`, `weaponFlameEmitters`, `renderWorldModels`
and the CLUT animation tables (they block "16 of one car" outright), and lists the rest for the port to
measure (16 Outlaws tasering at once need more than 5 arcs; 4 shields at once; 2 remote bombs at once).

---------------------------------------------------------------------------------------------------------

## B. Request 32 step 3: the car swap (`TM2_PORT_CAR_SWAP`), updated for per-car state

**Done (B.1-B.5)**, as built: `CAR_GROUPS.md` 8. Differences from the text below, with the port's answers
G.3-G.5 applied: `portCarBindGroup` is a fixed 48-byte table (human i at [i], AI j at [16 + j]) consulted by
`sessionBindCars` (its one game change, a function step 1 already changes); the shared blocks take a **slot**
(the key, object and driver follow from it) and kind/index pairs instead of `TM2PortCarRef`
(`portCarResetSpecials(slot)`, `portCarResetLook(slot)`, `portCarUnbind(kind, index)`,
`portCarBindSlot(kind, index, slot)`, `portCarApplyType(kind, index, hp)`, `portCarParkSlot(slot)`,
`portCarRekeyReferences(slot_a, slot_b)`, b < 0 = clear), plus `portCarParkAi(j)` (`SURVIVAL.md` 3.1, the
Juggernaut recipe's per-round call); the stats keep the per-life fields `carStatsLoad` would reset (killer and
last attacker included); Mr. Slam's claw records move with the type (no allocation in a two-way swap); the
reference fixer also remaps the taser arcs' and the drops' slots and the energy shields' car; Dark Tooth's
stage flag is left to the round rule. The CHECK items step 3 relies on are resolved in `CAR_GROUPS.md` 8.8.
B.6 and B.7 are not built.

### B.1 Defines, storage

- `TM2_PORT_CAR_SWAP` requires `TM2_PORT_CAR_GROUPS` (`#error`). Works with or without step 2 and with or
  without the request-33 defines (each 33 define, when on, makes the swap's reset reach its per-key state
  through the same accessor; without it the accessor is the retail global).
- Port code only (`.port_text`, `port_variant_arrays.c` or a new port unit `src/semantic/port_car_swap.c`
  compiled with the port recipe); **no game function changes** (`port_changed_functions` empty, as planned).
- Storage: `portCarBindGroup[TM2_MAX_HUMANS + TM2_MAX_AI_CARS]` (s8, group override per record, 0 = settings;
  F.3) — only if the port wants swaps to survive `sessionBindCars` (modes 3/4 rebind every round).

### B.2 API

```c
/* a record reference: kind 0 human, 1 AI; index into humanCars / aiCars */
typedef struct { int kind, index; } TM2PortCarRef;

int  portCarSetGroup(int human, int index, int group, int hp);  /* step 3: a record drives another loaded model */
/* B.6 (online with bots) adds portCarRemove, portCarStartHuman/Ai, portCarLoadGroup, portHumanCompact */

/* shared building blocks (step 3 writes them; B.6 reuses them) */
void portCarBindSlot(TM2PortCarRef r, int slot);                /* raw table rebind (B.3 step 4), no restart */
void portCarUnbind(TM2PortCarRef r);                            /* the record leaves its slot: parked/vacant */
int  portCarApplyType(TM2PortCarRef r, int key, int hp);        /* stats/weapons/camera/AI for a key; also the mod hook (E) */
void portCarResetSpecials(int key);                             /* release every special state of a key (extended by C.2) */
void portCarResetLook(int key, TM2PortCarRef r);                /* freeze, fire, turbo, invisibility, engine sound */
void portCarRekeyReferences(int slot_a, int slot_b, int id_old, int id_new, void *rec_old, void *rec_new);
                                                                /* credits, projectile owners, targets, record pointers */
```

Shape: `portCarSetGroup` = unbind both records + `portCarBindSlot` crosswise + `portCarApplyType` +
resets. B.6's removal and mid-match start reuse `portCarUnbind`, `portCarResetSpecials`,
`portCarResetLook` and the reference fixer (in "clear" mode), and retail's own binders
(`carBindHumanSlot`/`carBindAiSlot`) for the respawn, so they are small additions once step 3 exists.

Contract (answers Q14, Q15): pickup weapons kept, specials reset, HP scaled by default (`hp < 0`), set when
`hp >= 0` (clamped to the new max; the Juggernaut); the target group may be driven by an active car (two-way
swap) or parked (the Juggernaut's Minion). Called from the port's per-frame hook before the car updates
(where it runs `demoRespawnAiCars`' slot), never from inside a car update.

### B.3 Steps (`CAR_GROUPS_PLAN.md` 3.3, updated)

1. `rec` = human i / AI j; `sa = rec->slot`, `ka = portCarGroups.slot_key[sa]`, `sb = portCarSlotOfGroup(group)`,
   `kb = slot_key[sb]`; fail if `sb < 0`, `sb == sa`, or `carSlotKinds[sb]` is 2 for a Minion *second* root
   (bind to the group's first slot only). `other` = the record on `sb` (kind 0/1) or none (kind 2).
2. **Release the specials of both keys and both records** (`portCarResetSpecials(ka)`,
   `portCarResetSpecials(kb)`), each as the game ends them itself:
   - Mr. Slam (`weapons.slam` of `rec`/`other` with `active`): as `carDestroyed` (`car_destroyed.c`
     :336-341): `carSpinOutById(spin_target, rot[2], 0)`, `active = 0`, `spin_target = -1`, `spin_pending = 0`;
     **and** any slam record anywhere whose `spin_target` is `ka` or `kb` (a car held by a third Mr. Slam:
     keys belong to models, the held model is changing driver) → released the same way; arm/claw of the key
     back to rest (`-113`/`739`, via the accessor);
   - Twister: state of the key (before C.2: the global, only if `weaponTornadoState.object == carSlotObjects[sa]`)
     ended as `weaponTornadoUpdate`'s end block (`weapon_tornado.c:226-247`, body shown, objects off);
   - Shadow: strike of the key (before C.2: only if `weaponStrikeOwner == &rec->weapons`): `active = 0`,
     `timer = 0`, step 0, object hidden, no explosion;
   - Grasshopper: leap of the key (before C.2: only if `homingLeapState.owner_id == sa`): `active = 0`,
     `portCarTurboSetStep(ka, 0)`;
   - Dark Tooth: boss record of the key reset (stage 0, timers 0), head hidden (word +8 = 0) if the model is
     parked by the swap;
   - Axel: animation of the key step 0;
   - crush: any record whose `stats.crush_owner` is `ka`/`kb` → `state.crush_contact = 0`.
3. **Reset the look** on both models: freeze (`frozen = 0`, `freeze_timer = 0`, `carFreezeSetState(k, 0)`),
   fire (`burning = 0`, `fire_effect->active = 0`), turbo (`portCarTurboSetStep(k, 0)`), invisibility
   (`clutAnimFadeIn(k + 8, 0)` with C.1, else `type + 8`; `carBodyObjectShow(k)`, object visible, flags 0),
   engine sound: with C.3 `soundStopInstance(snd(type), k >> 4)` and the car's active bit cleared; without,
   `soundStop(snd(type))` (a twin re-starts its own next frame: harmless).
4. **Re-bind** as `carBindHumanSlot` 8011A3FC / `carBindAiSlot` 8011A5CC do, without `carRestart`/`aiCarStart`
   (`CAR_GROUPS_PLAN.md` 3.3 step 4: object/slot tables, `humanCarTypes`, `carSlotIndices`, `carSlotKinds`,
   object id `i + 1` / `j + 50`; `other` on `sa`, or `sa` kind 2 / index -1).
5. **Type change** through `portCarApplyType(rec, human, kb, hp)`: keep motion and route state;
   `ride_height` difference applied to `pos[2]`; human: `carStatsSetSpeedMult(100)`, `carStatsLoad`,
   `max_hp += 20`; AI: speed mult by difficulty, `carStatsLoad`, `carStatsSetTopSpeed`; HP scaled
   (`hp * new_max / old_max`, ≥ 1 while alive) or set; `carWheelsSetStepHeight`, `carBuildCollisionBox`;
   weapons: save `ammo[1..9]`, `selected`, `energy`, `turbo` (F.4), `weaponBlockInit` (fresh specials:
   `ammo[0] = 2`, recharge/capacity of the new type, a claw record for Mr. Slam **from the grown pool**,
   A.2 row 2), restore; Mr. Slam arm/claw of `kb` to rest; human: `viewCameraOffsetsInit`; AI:
   `aiCarSetupProfile`.
6. **New model on** (damage stage from HP, record stage field, body shown unless dead, draw on, light env,
   object pos/rot from the record) and 7-8 (`other` the same or parked, `SURVIVAL.md` 3.1) as planned.
7. **New (from the census):**
   - **credits:** `killer`/`last_attacker` (human stats +0x07/+0x08, AI equivalents CHECK) hold **slots**:
     in every record swap the values `sa` ↔ `sb`, so a kill still credits the player and not the model;
   - **homing projectiles and lock-ons** holding `carSlotObjects[sa]` (projectile target object, AI
     `aiCarTargets` hold indices: fine): replace by `carSlotObjects[sb]` and vice versa (default on; F.5);
   - **team colours:** the swapped player now drives group(sb)'s DMD; with C.1 the port repaints that group's
     body CLUT (`portCarCluts[group]`) to the player's team; without C.1 it would repaint the whole type;
   - **`portCarBindGroup`** (if the port wants it): set for `rec` and `other`, consulted by `sessionBindCars`
     under the define (one line in step 1's binder code), so modes 3/4 keep the Juggernaut on its Minion
     after the per-round rebind. Without it the port re-applies its swaps after each `sessionBindCars`
     (as it re-parks waves).

### B.4 Hazards

- Juggernaut + 16 players = **17 loaded cars** (16 + the parked Minion): step 2's sizes must count loaded
  cars, not active ones (D.2 item 1): 17 groups, 17 AI-capable records (16 humans + 1 AI), 34 slots if every
  car is a Minion (two roots each) > `TM2_CAR_SLOT_CAP` 32. Overflow keys: 16 of one type + its Minion =
  16 overflow keys = exactly the 16..31 range (none left).
- A swap during a running CLUT fade: `clutAnimFadeIn` restores over the next frames on the old model's
  CLUT; harmless (the old model is parked or driven by `other`, whose invisibility is reset too).
- Two swaps in one frame touching the same key: do them in sequence (each reads the tables the previous
  one wrote).

### B.5 Verification

Static: define-off and step-1/2 images unchanged; with the define only new `.port_text` functions;
proofs pass. Port runtime: the Juggernaut scenarios of the plan plus: swap while the twin of either model
fires its special (with and without C.2), swap with a car held by a third Mr. Slam, kill credit after a
swap, homing missile in flight at swap, repeated swaps across a round in modes 3/4.

Size: 3-4 port functions (~350 lines), 0 game functions (1 if `portCarBindGroup` is consulted by
`sessionBindCars`).

### B.6 Online with bots: removing a car for good, starting a car mid-match, loading a car mid-match

**Done (2026-10-13): `CAR_GROUPS.md` 14, `TM2_PORT_CAR_DROPIN`.** Built as planned with these differences:
`portCarRemove(kind, index)` (not a `TM2PortCarRef`); a vacant AI record points at a dedicated vacant slot (63) whose
object is a port dummy, so its old car is free at once (slot -1 would crash `carObjectId`; null objects crash three
per-frame readers: the census in `CAR_GROUPS.md` 14.4); `portCarStartHuman(index, group, flags)` /
`portCarStartAi(index, group, flags)` take a load group (`portCarFreeGroupOfType(type)` finds a free one) and by
default reset only the record (`carRestart`'s four match-wide resets are left out unless `TM2_DROPIN_START_RETAIL`);
the start node is the one farthest from the live cars; `sessionBindCars` binds the records there are once a drop-in
function ran (no re-parking after a rebind). The CHECK items are answered in `CAR_GROUPS.md` 14.8.

(Port's corrected request 33, 2026-10-12: a joining player does **not** take over a bot's car. One bot is
removed for good (parked, `SURVIVAL.md` 3.1) and the player spawns fresh with the car and team they
picked; a leaver is removed the same way, and a bot may take the free place, spawned fresh.)

**B.6.1 Model.** Three independent things: a **record** (human i / AI j: who drives), a **car** = a loaded
group (its DMD copy, its slot(s), key, VRAM slots, per-key state), and the binding between them. A join is:
remove a car + record (one bot), find or load a car of the picked type, start a record on it as a respawn.
A leave is: remove the car + record, optionally start a bot on a free car.

**B.6.2 Functions** (port code, `TM2_PORT_CAR_SWAP` or its own `TM2_PORT_CAR_ONLINE` define; the shared
blocks of B.2 reused)

```c
void portCarRemove(TM2PortCarRef r);            /* the record's car leaves the match for good; the car becomes a free car of its type */
int  portCarFreeSlotOfType(int type);           /* a free (parked, unbound) car of this type, its first slot; -1 = none */
int  portCarStartHuman(int slot);               /* new human record i = humanCarTotal on that car, as a respawn; returns i */
int  portCarStartAi(int slot);                  /* a vacant AI record (or aiCarTotal++) on that car, as a respawn; returns j */
int  portCarLoadGroup(int type, const void *tms, int tms_size, const void *dmd, int dmd_size); /* B.6.5: a new car; returns its slot */
void portHumanCompact(int from, int to);        /* same-kind record move: the last human into a leaver's index */
```

**B.6.3 `portCarRemove(r)`** (car slot `s`, key `k`, record r):
1. Specials and look of `k`: `portCarResetSpecials(k)`, `portCarResetLook(k, r)` (B.2; engine instance
   stopped, fade ended, turbo, freeze, fire off, Dark Tooth head hidden).
2. References to this car (cleared, not rebased, because it will not come back):
   - by **key/slot**: slam records whose `spin_target == k` released; `crush_owner == k` cleared;
     `killer`/`last_attacker == s` in every record set to -1 (else a later car on this slot would get the
     credit); homing leap/tornado/strike states that target this car's object ended or retargeted;
   - by **object id** (`i + 1` / `j + 50`, which the next record of that index reuses): owners of live
     projectiles, bullets, ricochet shots, mines and drops (CHECK each pool's owner field) set to "no
     owner" or the projectile removed (F.12); targets of homing projectiles that hold this object: dropped;
   - by **record index**: `aiCarTargets[x] == j` → -1; `viewWatchedAiCar` (attract only);
   - by **record address**: effect follow pointers into the record (burning sprite `fxFlameBurning(motion.pos)`,
     `HumanRecord.fx`, sound locate): the effects ended.
3. The car parked (`SURVIVAL.md` 3.1 on the object and model): draw off, body hidden, object far away
   (z = -1,000,000), collision off; `carSlotKinds[s] = 2`, `carSlotIndices[s] = -1`, and a port flag
   `portCarFree[s] = 1` (it can be reused by `portCarFreeSlotOfType`). Minion's second root stays kind 2
   unflagged.
4. The record:
   - **AI j:** vacant (dead, HP 0, wreck state 4, `active` 0, `slot = -1`, `type = 0` so `worldUpdate`'s
     out-of-range branch finds no engine id (`world_update.c:375`), `aiCarObjects[j]`/`aiCarObjectSlots[j]`
     → a port **dummy object** (hidden, far away) so no loop over `aiCarTotal` reaches a real car through j;
     `portAiVacant[j] = 1` for the port's round rules). `aiCarTotal` unchanged (`SURVIVAL.md` 3.4; loops
     skip it: `carsPhysicsUpdate` tests `updated`, `car_physics.c:111`; CHECK `aiCarDamageUpdate`,
     `aiCarsCountRemaining`/`Alive`, `aiCarNearestAlive`, the weapon and lock-on loops over `aiCarCount()`).
   - **human i:** humans must stay **contiguous** (`worldUpdate`'s human loop breaks at the first null
     `humanCarObjects[i]`, `FOUR_HUMANS.md` 2; every other loop runs to `humanCarTotal`). If i is not the
     last, `portHumanCompact(last, i)`; then `humanCarObjects[last] = 0`, `humanCarTotal--`
     (`humanCarsSetCount`). The host (human 0) never leaves (else the round-end rule on `humanCars[0]`,
     Q10, needs the port's rule first).

**B.6.4 `portHumanCompact(from, to)`** (the one record move left; same kind, so a plain 0x814 copy):
copy the record; `player = to`, `f002 = to`; per-human arrays (`humanCarObjects`, `..Slots`, `..Types`,
`padConfigs`, `hudSmoothedSpeeds`, `carViewHotkeyReleased`, `carResetWeapons`) moved; object id
`TM2_HUMAN_OBJECT_ID(to)` (D.2) and `carSlotIndices[slot] = to`; references by object id (projectile owners)
and by record address (tornado `anchor`/`flags`, leap `health`/`position`/`rotation`/`flags`, strike
`weapon`, the burning sprite's follow pointer, sound locate) rebased by `to - from` records (CHECK every
holder against C.2's structs); the local-view map (D.3) and the port's net-player ↔ human table follow.
Slots, keys, the car and everything by slot/key/object pointer stay.

**B.6.5 Starting a record mid-match (as a respawn).** Retail already has both paths:
- `portCarStartHuman(slot)`: i = `humanCarTotal`; the per-record part of `uaInit` 80119698 for i
  (`carSetResetWeapons(1, i)`, damageable +0x1F4 = 1, `id = i`, +0x096..+0x09B = 0, `PadSetConfig(1, i)`),
  `hudResetSpeed(i)`, `humanCarTotal = i + 1`; then `carBindHumanSlot(i, slot, carSlotOwners[slot])` 8011A3FC:
  binds, `carRestart` (start node through `startNodePickRandom`, fresh stats and weapons: the same as a
  lost-a-life respawn), `carSetViewMode(rec, 2)` (only if i has a local view, D.3: on the server a remote
  player has none), kind 0, object id, draw on. Team colour: the port repaints the car's CLUT row (C.1).
- `portCarStartAi(slot)`: j = a vacant record (`portAiVacant`), else `aiCarTotal++`; `carBindAiSlot(j, slot,
  type)` 8011A5CC (binds and `aiCarStart`: start node, full HP, profile); `aiCarTargets[j] = -1`.
- Both clear `portCarFree[slot]`; with C.3 the engine lists' max is recomputed (`portCarSoundSetup`).
- Start node: step 2's farthest-free picker (`portCarOptions` bit 2) picks the start entry farthest from the
  live cars, which is what a mid-match spawn wants (`SIXTEEN_CARS.md` 5.3).

**B.6.6 Where the joining player's car comes from.**
1. A free car of the picked type (`portCarFreeSlotOfType`): a car removed earlier, or a spare loaded at the
   start (B.7). Nothing to load.
2. Otherwise a **new group** (`portCarLoadGroup`, B.7).
Removed cars are never unloaded: their group, key, slot, DMD memory and VRAM stay, and serve the next
player of that type. Unloading a group (removing its scene objects from the cs list, freeing VRAM, keys and
DMD memory) is possible in principle but touches many load-time-only structures; not recommended (B.7.4).

### B.7 Mid-match car loading vs preloading (feasibility)

**Done (2026-10-13): `CAR_GROUPS.md` 14.6, `portCarLoadGroupLive`.** Full mid-match loads of any type (TMS + DMD from
port buffers): with banks a fresh bank, without banks retail's placement in the usage bitmap kept through the match
(`bitmapSetupStorage` -> port storage), refused cleanly when it does not fit. Unloading frees the bank only (14.7;
since 2026-10-15 everything of the group: see the note below).
Correction to B.7.1 (found on the way): `bitmapSetupStorage` does **not** point `modelRelocateStack` at the shared
render buffer; it stores the buffer's address *through* the pointer, which is 0 in retail (nothing else sets it), so
`modelRelocate` and `weaponSlamFindTarget` keep their traversal stack at address 0 (low RAM), at the load and mid-match
alike (a retail quirk, kept). The render shared buffer is per-frame scratch (hit detection, dbscan overflow, the near
list), free between frames.

**Note (2026-10-15, unloading: `CAR_GROUPS.md` 14.7).** `portCarUnloadGroup` now gives a group nobody drives back whole,
and live loads reuse it: its DMD bytes (a first-fit free list of the DMD area, no compaction: nothing in it may move),
car slots (the first free run), overflow key (the lowest free one), load group number, scene objects (`csAlloc` takes
from a free list first: the one game change, retail while nothing was unloaded), per-key registries and special state,
CLUT copies and rows 240..255 spots, +8 fade definitions, image records, bank and sound instances; every reference to
them is cleared or moved to another loaded copy (the shared projectile models). `portCarRoom` / `portCarCanLoad` report
what is free. B.6.6's "removed cars are never unloaded" and B.7.4's "not recommended" are superseded: over a long
session the port keeps a few free cars per type and gives the rest back. Bank 0's VRAM stays a cache by key (never
freed); a car of a single-special type can be given back in builds without per-key specials only while it is the only
one of its type.

**Note (2026-10-12, request 32 step 4 built: `TEXTURE_BANKS.md` 6.7).** A group's bank is allocated and freed on
its own (`portTexBankAlloc` / `portTexBankFree`), and banked placement needs no VRAM usage bitmap (a fresh bank
per group, its rectangles in port storage). For the full mid-match load of B.7.4 item 2 the slot table and the
relocate stack still need port blocks (`bitmapSetupStorage`), and either the arena's image records are kept or
the joining group gets `portCarGroupNoReuse` (then no record is read).

**B.7.1 What a car load does today, and what of it exists only at load time** (`TEXTURE_BANKS.md` 1.1,
`TEAM_COLORS.md` 1.4, `SURVIVAL.md` 2; CONFIRMED unless tagged)

| Step (function) | Needs | Mid-match status |
|---|---|---|
| TMS read (`fileioLoadFileIntoRam` into 0x80019C40) | the CD, the shared load buffer | **blocking CD read**; the battle music is CD audio/XA (`audioAlternateMusicTrack`), so a data read stops it (LIKELY). The load buffer 0x80019C40 is the start of the **level DMD**, live during a battle: a mid-match read there would overwrite the arena. Must be a port-supplied buffer |
| TMS parse (`bitmapParseContainer(buf, n)` → `bitmapRegisterTim` → `vramPlaceTexture`/`vramUploadClut`/`bitmapAddRecord`) | the **VRAM usage bitmap, the slot table and the image records**: `bitmapSetupStorage` 801214FC (`bitmap_storage_setup.c:12-20`) puts the bitmap and the slots (`vramTexSlots` = bitmap + 0x10004) in **packet buffer 0** and the records in **an ordering table**; the first frame drawn overwrites them; `bitmapTrackingEnabled` off after the load (`bitmapDisableTracking`) | **gone after the load.** Fix (small, under the define): `bitmapSetupStorage` uses `.port_bss` blocks instead (bitmap 64 KB, slots 1,599 × 20 = 32 KB, records ~16 KB), so they survive the match; tracking re-enabled around a mid-match parse. With that the parse runs mid-match unchanged |
| VRAM upload (`LoadImage` 80120B98 / 80120E4C + `DrawSync`) | VRAM free where the bitmap says | works mid-match (the port's GPU takes `LoadImage` between frames); room is the real limit (B.7.3) |
| DMD read into the car area | step 2's `portCarDmdArea` (bit 1) | memory is fine (B.7.3); the read must be a port buffer copied into the area (or read by the port directly there) |
| DMD install (`assetInstallImage(buf, n)` → `modelRelocate` → `modelRemapTextures` + `dbscanHierarchy` → `worldLoadObject`) | `modelRelocateStack` = `renderGetSharedBuffer()` (set by `bitmapSetupStorage`: a render buffer, CHECK it is free between frames), the group's slots (above), `vramTexSlotGroup` | works mid-match once the slots persist and the relocate stack is a port block. The registrations it makes are all run-time safe: `carRegisterModel` appends a slot (`carWorldObjectCount++`, bounded by `TM2_MAX_CAR_SLOTS`), `portCarRegisterRoot` gives a fresh key, `dbInitCsForAModel`/`csAlloc` (pool 128, Q11: 256), `carBodyObjectRegister`, `carFreezeBuild` (zero the new key's freeze record first: `portCarGroupsReset` only runs at load), `clutAnimRegister` (definition tables, C.1), `weaponAssetBind` (per key with C.2; without C.2 a new Twister/Shadow/Dark Tooth install **replaces the live global objects**: a running tornado/strike/head of the same type jumps to the new copy) |
| `carChoiceRegisterGroup` per slot | done by `arenaSetup` 8011995C at bind time | must be called for the new slot(s) (port) |
| `arenaResetSystems`, `carTablesClear`, `carPartsReset`, `objRegistriesReset`, `clutAnimInit`, `weaponObjectsDetach` | load-time resets | must **not** run mid-match (they would wipe the live match) |
| `startNodePickRandom`, binding, `carRestart`/`aiCarStart` | | run-time already (respawns) |
| FAR_LOD/FAR_DISTANCE changes in `assetInstallImage` | per-install LOD bookkeeping | CHECK they are per group and append-only |

**B.7.2 Costs per car** (CONFIRMED sizes, `SURVIVAL.md` 4.3, `SIXTEEN_CARS.md` 3.3, 4.3)
- DMD copy in RAM: 30-40 KB for a normal car (largest 39,168), Minion 32,736, Dark Tooth 61,660. A duplicate
  needs its own DMD copy; textures are shared by key.
- TMS: only in the load buffer during the parse (not kept).
- VRAM: the first car of a type uploads its images (`SIXTEEN_CARS.md` 4.3: normal types a few thousand to
  ~13,000 texels, Dark Tooth 77,056) and its CLUTs (1 row each 256-colour CLUT); a duplicate costs only its
  CLUT copies (C.1).
- Groups/slots/keys: one group, 1 slot (2 for Minion), one key per car ever loaded in the match (removed cars
  keep theirs).

**B.7.3 Preloading: how many distinct types fit per arena** (`SIXTEEN_CARS.md` 4.4 replay, LIKELY; "rows
freed" = with the 32 CLUT spots of rows 240..255, which C.1's allocator provides)

| Arena | Distinct normal types that fit (largest first / smallest first) | With rows 240..255 |
|---|---|---|
| Moscow, Holland, Cyburbia, Roof Tops | all 14 | all 14 |
| Antarctica (1P / 2P), Amazonia, New York 2P | 10-13 (CLUT-bound) | all 14 |
| Hong Kong | 9-11 (CLUT) | 12 |
| Los Angeles, New York | 12 (pixels) | 12 |
| Paris, Paris 2P | 8-9 (pixels) | 8-9 |
| Suicide Swamp | 9-10 (pixels) | 9-10 |
| with Dark Tooth | `SURVIVAL.md` 4.1: 0 (Paris) .. 10 (Roof Tops) others | |

Memory for "every pickable type preloaded + a spare": 14 spare normal DMDs (~0.5 MB) + 16 active cars
(~0.65 MB) ≈ 1.15 MB of DMD area: above Q13's 1 MiB. The port's free RAM below 0x80600000 (after `.port_bss`,
which ends near 0x803A3000 today plus step 2's 1 MiB area) allows a 2 MiB area (F.14). Slots: up to 30
groups + Minion second roots → `TM2_MAX_CAR_SLOTS` ~48; keys: 14 type keys + up to 16-30 overflow keys →
`TM2_CAR_KEYS` 48 (keys up to 0x2F0: no code assumes key < 0x200 except the `objRegistrySetStep` path that
`portCarTurboSetStep` already avoids; CHECK the `s16` stores of `spin_target`/`crush_owner`: fine to 0x7FFF).

**B.7.4 Assessment**

| | Preload (spares loaded at match start) | Mid-match DMD only (type's textures already in VRAM) | Mid-match full load (new type: TMS + DMD) |
|---|---|---|---|
| C changes | none beyond B.6 (spares are parked groups, `portCarFree`) | `bitmapSetupStorage` → persistent port storage (slots at least), `portCarLoadGroup` without TMS, `carChoiceRegisterGroup` for the slot, the new key's freeze record zeroed, C.1's CLUT copy from the port allocator | the same + the persistent usage bitmap and records, tracking on around the parse, a dry-run placement check (`portCarCanPlace(tms)`: replay on a copy of the bitmap) so a pick that does not fit is refused before anything is uploaded |
| VRAM | all offered types at start: limited per arena (B.7.3) | none (a CLUT row per copy) | must fit next to everything loaded: same per-arena limits, but only for the types actually picked |
| Memory | all spares resident | one DMD per join | one DMD per join |
| Timing | load screen only | one frame hook: copy ~40 KB + relocate + register (sub-millisecond on PC; LIKELY) | the same + parse/upload (a few ms) |
| Risks | groups/slots/keys sized for spares | the load-time-only storage (B.7.1) | as left + VRAM fragmentation, "Relocatable texture not loaded" on a bad pick if the dry run is skipped |
| With step 4 (banks) | per-arena limit gone | — | per-arena limit gone (a new bank per group; nothing to place around) |

**Recommendation.**
1. **Now (before step 4): preload textures, load DMD copies mid-match.** At match start the server loads the
   TMS of every type it offers to joiners (the port picks the list per arena from B.7.3: all 14 on the
   roomy arenas, the match's types plus a few on Paris/Swamp) as **spare groups** (one DMD each, parked,
   `portCarFree`). A join takes a free car of its type; if the type's spare is in use, `portCarLoadGroup`
   loads one more **DMD copy only** from a port-supplied buffer (no texture placement: the slots are copied
   from the type's spare group, the CLUT copy comes from C.1's rows 240..255 allocator). This needs only the
   persistent slot table, not the bitmap, and never touches the arena's VRAM.
2. **With step 4: full mid-match loads** of any type, also the ones not preloaded (a fresh bank per group).
   Do not try new-type texture loads mid-match without banks except as an option guarded by the dry run.
3. **Never CD reads mid-match from the C**: the port hands buffers (the same API as mod cars, E.1:
   `portCarSource[group]`).
4. Size the limits for churn: groups/slots/keys = 16 active + spares + loads during the match; the port
   caps the total (F.14) and falls back to "no new type" when full.

Size: B.6 ~6 port functions (~400 lines); B.7 adds `portCarLoadGroup` (+150), `bitmapSetupStorage` (1 game
function under the define), 3-4 storage rows (`portCarFree`, `portAiVacant`, slot-table and relocate-stack
blocks; the bitmap and records only for full loads).

---------------------------------------------------------------------------------------------------------

## C. Request 33 as ordered steps (each its own PR, behind a define, retail-equal with options off)

Recommended order: (32-2 in progress) → 32-3 swap (B) → **33-1 CLUTs** → **33-2a..e specials** → **33-3
engine sound** → **33-4 rest** → (32-4 texture banks). 33-1 before the specials because team colours and
invisibility are the most visible sharing; the swap first because its reset hook (`portCarResetSpecials`)
is where each 33-2 PR plugs in.

Common to every step:
- `#ifdef TM2_PORT_<X>` ... `#else <retail> #endif` in game units; `#error` without `TM2_PORT_CAR_GROUPS`.
- Storage rows in `config/US/port_variant_arrays.tsv` defined in **every** variant (layout independent of
  the define), group `one`; `#pragma tm2_absolute` for every port name a game unit reads.
- Retail globals that are $gp-addressed today (`weaponTornadoObject` 80180F38, `darkToothBossObject`
  80180F40, `slamArmObject` 80180F74.., `aiBossDefeatedFlag`, `objRegistryId2324/5`, the Dark Tooth
  scalars) stay where they are and keep their retail users in the define-off build (D4 of the plan).
- Static check in the style of `tools/check_car_groups.py`: under the define, every relocation into a
  retail per-type global listed in A.2 is in an accessor or an allowed function; the expected
  `port_changed_functions` list is written into the PR and diffed mechanically.
- Port test: frame-by-frame equality against the variant without the define (≤ 11 distinct cars, options
  0); then the step's duplicate scenarios.

### C.1 Step 33-1: per-car CLUTs (`TM2_PORT_CAR_CLUTS`, `portCarOptions` bit 3)

**Done (2026-10-13): `CAR_GROUPS.md` 13.** As built it differs from this plan in: the run-time bit is **bit 7**
(0x80; bits 0-6 were taken); bit 7 is ignored while texture banks are live (bit 5); the reuse branch of
`vramPlaceTexture` only **notes** a CLUT to copy and the copies are made in one pass after the last car TMS
(`portCarClutsCopyAll` from `sessionLoadArenaAndCars`), read back from VRAM, bodies first, so no copy can move or
block a later car's original; CLUTs taken from the **arena** are copied too (the copy is the arena's row, so the
arena look stays; no arena CLUT animation acts on them), which makes Axel's fade on New York / Hong Kong his own;
rows 240..255 have one allocator shared with 33-2e's bit 6 (whose `bitmapInit` change became a fallback in
`vramUploadClut`); the published table is `portCarCluts` (CLUT id and state per group and image, sharing flags
per group) and the body CLUT also in `portTexBanks.body_clut`; the CLUT-animation pools were already raised by
step 2 (`TM2_PORT_CARS`), so the nine `engine_clut_anim.c` helpers needed no change.

**Note (2026-10-12, request 32 step 4 built: `TEXTURE_BANKS.md` 6, `CAR_GROUPS.md` 9).** With texture banks on
(`TM2_PORT_TEXTURE_BANKS`, `portCarOptions` bit 5) every car, duplicates included, has its own copy of its
pixels and CLUTs in its own bank, its own invisibility fade (one definition per car key, in its bank) and its
own body CLUT for team paint (`portTexBanks.body_clut` / `body_bank`). C.1 is then only the **no-banks
fallback**: build it only if the port needs per-car CLUTs with bit 5 clear. If it is built, reuse step 4's
pieces (`TEXTURE_BANKS.md` 6.7): `portTexFindReuse`, the per-key fade keying in `clutAnimRegister` and
`portTexFadeId`, `body_clut`, `portCarGroupNoReuse`; only the CLUT copier and the rows 240..255 allocator are
new. Bit 3 is taken (step 2's pools): C.1 needs another bit (4 is kept for C.3, 5 is the banks').

**What:** each car group gets its own copy of every CLUT it would otherwise take from **another car**
(reuse by key, `TEAM_COLORS.md` 1.3): duplicates get copies of all their CLUTs; different types get their
own copies of the shared part CLUTs (undercarriage, shadow, flame, smoke: 16 texels each). Pixels stay
shared. CLUTs taken from the **arena** (Roadkill's and Dark Tooth's effect images, `TEXTURE_BANKS.md` 1.4)
stay shared, so the arena-palette look of Swamp/Cyburbia/Roof Tops is kept.

**Game functions:**

| VA | function | file:line | change |
|---|---|---|---|
| 801209E8 | `vramPlaceTexture` | `screen_deathmatch_result.c:90` (reuse branch :102-110) | after copying the known position: if bit 3, the image has a CLUT (`img->mode & 8`), and the record is a car's (`known` index ≥ `portCarClutLevelRecords`), `portCarClutCopy(img, col, row)`; on failure the slot keeps the shared position (retail), and the group is flagged shared |
| 8012BAFC | `sessionLoadArenaAndCars` | `shell_main.c` | after the level TMS: `portCarClutLevelRecords = <record count>` (CHECK the counter's name in `bitmap_record_append.c`); after each car's `bitmapParseContainer(buf, n)`: `portCarClutPublish(n)` (copy the group's CLUT ids into `portCarCluts[n][0..31]`) |
| 800D1DB0 | `worldLoadObject` | `world_load_object.c:156-158` (+8 ids) | bit 3 and `portCarLoadKey()` an overflow key: `portClutAnimRegisterAs(obj, key + 8)` (a port copy of `clutAnimRegister`'s body with the id given); else retail `clutAnimRegister(obj, 2)` (CHECK the jump-table change of this big switch; step 1 avoided touching it — alternative: do the id override inside `clutAnimRegister` 801489B4 when `o->id` is a car +8 id and `portCarLoadKey()` is an overflow key) |
| 8010DE54 | `weaponInvisibilityUpdate` | `ai_link_node_pick.c:268` | `clutAnimStartFadeOut/FadeIn(TM2_CAR_KEY(st->slot, st->type) + 8, ..)` |
| 80149018, 801490B0, 80149120, 801491A0, 80148660, 80148B1C, 80148740, 8014889C, 8014868C | `clutAnimNewDef`, `FindDef`, `FindRunning`, `FreeSlot`, `Init`, `Update`, `StartFadeOut`, `StartCycle`, `StartSweep` | `engine_clut_anim.c` | definition i and running j through `CLUT_DEF(i)` / `CLUT_RUN(j)`: `i < 20 ? &clutAnimDefs[i] : &portClutDefs[i - 20]` (D5 style), limits `TM2_CLUT_DEFS` (64) / `TM2_CLUT_RUNNING` (16 + 10) |

**Port functions:** `portCarClutCopy(img, col, row)`: 256-colour CLUTs first into a port allocator over
rows 240..255 (x 0 and 256: 32 spots; `SIXTEEN_CARS.md` 4.6 option 2, without changing `bitmapInit`, so first
cars keep their retail rows), then the general `vramUploadClut(256, 1, img->clut, col, row)` scan;
16-colour CLUTs with the general scan (16-texel gaps), then rows 240..255 x 512..639; mark what it takes in
the usage bitmap. `portCarClutPublish(n)`. `portClutAnimRegisterAs(obj, id)`.

**Storage:** `portCarCluts[TM2_CAR_SLOT_CAP][32]` (u16 CLUT ids per group and image, 2 KB),
`portCarClutShared[TM2_CAR_SLOT_CAP]` (u8: 1 = some copy failed, the car still shares), `portCarClutLevelRecords`,
`portClutDefs[44]` (20 bytes), `portClutRunning[16]` (0x424 bytes: 17 KB).

**VRAM:** per duplicate one 256-wide row for the body (Axel 3, Sweet Tooth 2, Dark Tooth 11 + 15 small),
per car a few 16-texel spots. 16 of one normal type: 15 rows → fits in rows 240..255 alone. 16 Dark Tooths
cannot (165 rows): copies fail, those cars share (flagged); step 4 fixes it.

**Team colours (the port):** read `portCarCluts[group][0]` (body) after the load; repaint each car's own row
(`LoadImage`, `TEAM_COLORS.md` 4 for the entries). The first car of a type keeps the original row; nothing
else uses it once its twins have copies.

**Hazards:** (1) does the port draw anything into VRAM rows 240..255, x 0..639 (its 4-view or widescreen
frame buffers)? (F.1); (2) `vramPlaceTexture`'s CLUT data pointer (`img->clut`) is valid only during the
group's parse (the TMS buffer is reused): copy there, not later; (3) the fade of Outlaw 2, Mr. Grimm and
Thumper points into the frame buffer (untextured first primitive, `TEAM_COLORS.md` 2.5): per key now,
still junk, as retail; (4) a second `StartFadeOut` on a running definition (re-trigger) saves a faded
palette: per-car rows make it a per-car retail bug only.

**Verification:** with bit 3 clear the image's VRAM, slots and primitives equal the define-off build (no
path changes); with bit 3: a host replay of `vramPlaceTexture` (the SURVIVAL/SIXTEEN scratch replays) for
16 Twisters on Amazonia and 4 teams of 4 Roadkills on Hong Kong: every group's body CLUT distinct, nothing
placed over an arena or first-car CLUT, count of rows used. Port: invisibility on one twin only; 4 teams of
one car in 4 colours; Spectre's invisibility no longer fades another car's undercarriage.

Size: ~14 game functions (9 of them the small CLUT-animation helpers), 3 port functions, 5 rows.

### C.2 Step 33-2: per-car specials (`TM2_PORT_CAR_SPECIALS`), five PRs in the port's order

**Done (2026-10-13): 33-2a, as one PR for Mr. Slam, Twister, Shadow + Axel and Grasshopper (the plan's
33-2a..d) plus the claw-record pool; `CAR_GROUPS.md` 10.** Built differently from the sketch below: no per-key
accessors; the retail code runs unchanged inside a *selection* (`portCarSpecialSelect(key)` exchanges the
retail storage of the key's special with the key's `portCarSpecials` entry, `portCarSpecialRestore` puts it
back), so D5 (first car = the retail global) and D6 (loops, first car first) hold with 13 game functions
changed, and no run-time bit is needed (D7). The struct holds 32 entries (every overflow key of step 2).
**Done (2026-10-13): 33-2e (Dark Tooth, rows 6 and 14), `TM2_PORT_DARK_TOOTH`; `CAR_GROUPS.md` 11.** The same
mechanism with a sixth part (the 7 scalars, the swoop block, the head, the swoop vectors and the stage flag, in
`portCarSpecials.dark_tooth[32]`); 10 game functions. The port's answer G.6 is built too: `portCarOptions` bit 6
(`SURVIVAL.md` 1.5/1.6: boss weapons and stage 2 on any arena with a per-car stage loop, the Hong Kong jump/warp
and marker code only on level 8, wreck state 4 after the final death, the missile wobble anywhere, `bitmapInit`
reserving rows 0..239). Retail's level-8 / `aiCars[0]` rules stay while bit 6 is clear.

One define, one storage struct `portCarSpecials` (overflow keys 16..31; D5), filled in PR by PR. Each PR
adds its member, its accessors, its part of `portCarResetSpecials(key)` (B.2) and its scenarios.

```c
struct TM2PortCarSpecials {           /* index = key index - 16 */
    struct { void *arm, *claw; int arm_angle, claw_angle; } slam[16];          /* 33-2a */
    struct { unsigned char state[0x14]; void *tornado, *funnel; } tornado[16];  /* 33-2b */
    struct { unsigned char block[0x64]; void *anim; } strike[16];              /* 33-2c, block = TM2StrikeBlock */
    void *axel_anim[16];                                                         /* 33-2c */
    unsigned char leap[16][0x3C];                                                /* 33-2d */
    struct TM2PortDarkTooth { short s[7]; short swoop_delay, swooping; int hover; void *swoop_target;
                              int swoop_vectors[6]; void *head; int defeated; } dark_tooth[16]; /* 33-2e */
};
```

**33-2a Mr. Slam** (A.2 rows 1-2). Functions: `slamArmRegister`, `slamClawRegister` (by `portCarLoadKey()`),
`carPartsReset` 80114380 (clear all), `weaponsRoundReset` 80108CF0 (all keys to rest), `weaponSlamSwingUpdate`
800F4058, `weaponSlamGrabUpdate` 800F4CC0, `weaponSlamCarryTarget` 800F59A4 (+ carrier key), `carStatsLoad`
800FE15C (drop the 0xC0 setters under the define), `humanCarPlaceAtStart` 800FD6C4 and `aiCarReset` 80100034
(set the rest angles of their key after `carStatsLoad`: CHECK the retail order of the two setters relative to
the rest of `carStatsLoad` stays observable-equal: they only touch the arm objects), `weaponSlamStateAlloc`
8010EE64 (pool: retail 4 then `portSlamRecords[TM2_PORT_CARS_LOADED]`, no overlap slot), `weaponsRoundReset`'s
claw-record loop (count over both parts). Port functions: `portSlamArmGet/Set`, `portSlamClawGet/Set`
(the retail getters/setters stay for the define-off build and as the key < 0x100 path). Hazard: the
accessors are called many times per frame inside the two big update functions; keep the key in a local.

**33-2b Twister** (row 3). Functions: `weaponAssetBind` 80109038 (cases 0x913, 0xA0; its switch's jump
table changes, CHECK words point into it), `weaponFireSpecial` 80105A7C (case 0xA0; already changed by step
1), `weaponTornadoUpdate` 8010E040 (body → `weaponTornadoUpdateOne(state)` in `.port_text` of the same unit,
retail body kept for the define-off build; the exported function loops, D6), `weaponsRoundReset`,
`weaponObjectsDetach` 80108F70. Hazard: `weaponTornadoUpdate` reads the owner three ways
(`weaponTornadoOwner` absolute word, struct member, cast pointer, `weapon_tornado.c:85-90`) to match
retail: the per-state body takes one pointer; keep the retail function untouched outside the define.
Two tornadoes catching one car: the later in key order wins the position (acceptable).

**33-2c Shadow (+ Axel)** (rows 4, 7). Functions: `weaponAssetBind` (0x915), `objRegistryAdd` 80123F78 and
`objRegistrySetStep` 80124384 (ids 2324/2325 by key; both already changed by step 1), `weaponFireSpecial`
(0xB0, 0xD0), `weaponShadowStrikeUpdate` 8010F9C0 (loop, D6), `weaponsRoundReset`, `weaponObjectsDetach`.
Port: `portStrikeSetStep(key, v)`, `portAxelSetStep(key, v)`. The 0xD0 detonation test then reads the
firer's own strike (the "detonates the other Shadow's strike" bug of row 4 goes away).

**33-2d Grasshopper** (row 5). Functions: `weaponHomingLeapInit` 8010EF44 (C++: `extern "C"` port
declarations, `#pragma tm2_absolute`), `weaponHomingLeapUpdate` 8010F520 (loop, D6), `weaponsRoundReset`.
`weaponHomingLeapState` 8010F9B0 has no caller: leave it.

**33-2e Dark Tooth** (rows 6, 14; **done**, `CAR_GROUPS.md` 11, as a selection part, not with accessors). Functions: `weaponAssetBind` (0xF9), `aiDarkToothBossUpdate` 8010FD44
(all 12 globals through a pointer to the key's record: the biggest change; `#define` the names to
`B->field` inside the function under the define), `aiBossDefeated` 8011C3EC → `portAiBossDefeated(key)` at its
5 call sites (`carContactApplyObject` 800E06CC ×3, `carApplyDamage` 800E5628, `worldUpdate`),
`aiDarkToothBossModel`/`Object` → by key (`carApplyDamage`, `worldUpdate`), `aiCarsCountRemaining` 8011C248
(sets the flag of `aiCars[0]`'s key: retail rule), `arenaSetup` 8011995C (clear all), `weaponObjectsDetach`,
`weaponProjectileUpdate` 800DB35C (the shooter's object, cosmetic). The level-8/`aiCars[0]` gates and the
`SURVIVAL.md` 1.5 patches are **not** part of this PR (F.6).

Verification for every 33-2 PR: static (the A.2 globals reached only through accessors under the define;
expected function list); port: two cars of the type firing at once, each special independent; the retail
scenario equality.

Size: 33-2a ~12 functions, 2b ~5, 2c ~6, 2d ~3, 2e ~10; 4-8 port functions each.

### C.3 Step 33-3: per-car engine sound (`TM2_PORT_CAR_SOUND`, `portCarOptions` bit 4)

**Done (2026-10-13): `CAR_GROUPS.md` section 12.** As built it differs from the plan below: the instance is the
car slot + 1 (not the key), the merge by instance is done by the port code before `soundMgrPlay` (unchanged), the
list sizes are set by `portCarSoundFrame` at each `worldUpdate` (no `portCarSoundSetup`), there is **no cap by
default** (`portEngineVoices` 0; N > 0 keeps the N nearest to a human: G.7 as changed by the user), and engine
voices sort after every other sound in `soundSortInstances` so the farthest engines drop first when the 24 SPU
voices run out. Thumper's flame (47) and, with `TM2_PORT_DARK_TOOTH`, Dark Tooth's swoops (61, 66) are per car
the same way.

| VA | function | file:line | change (bit 4 set; clear = retail calls) |
|---|---|---|---|
| 8011DB74 | `carSoundUpdate` | `screen_card_draw.c:104-123` | instance `inst = TM2_CAR_KEY(slot, type) >> 4` (1..31; the record's slot is at `a1` +0x004 human / +0x006 AI, CHECK the `a1` layout used there); `soundPlayVariant(snd, vol, pan, pitch, inst)`; stop: `if (portCarEngineOn[slot]) soundStopInstance(snd, inst)`; keep a per-slot "playing" bit |
| 8011A700 | `worldUpdate` | `world_update.c:373-393` | the out-of-range stop by instance, same bit |
| 80128890 | `soundMgrPlay` | `sound_manager_play.c:180` | for the 14 engine ids: if a request with the same instance is in the list, merge into it (`sndListMergeById` path) before allocating; else insert while `count < max` |
| — | port `portCarSoundSetup()` | from `sessionBindCars` or `arenaSetup` | `sndListSetMax` of the 14 engine lists to the number of loaded cars of that id's type (≥ 1), or back to 1 when bit 4 is clear |

Storage: `portCarEngineOn[TM2_CAR_SLOT_CAP]` (u8). Hazards: more simultaneous voices (16 engines + effects:
the PS1 SPU has 24; the port's mixer F.7); the sound object pool (35, Q11) is used per request, not per
frame (requests are released after merging), so 16 engines take 16 of it; `soundIsActive(snd)` stays
per id (other readers? only `carSoundUpdate`/`worldUpdate` for these ids, CONFIRMED by grep). Verification:
bit 4 clear = retail; set with no duplicate: the same requests (instance `type >> 4` instead of 0: audible
equal, request bytes differ: expected difference in the port's comparison, or use instance 0 for keys < 0x100
if the port compares sound state).

### C.4 Step 33-4: the rest

**Done (2026-10-14): `CAR_GROUPS.md` section 16.** The pools below were already raised by step 2
(`CAR_GROUPS.md` 7.1: `renderWorldModels` 40, `weaponFlameEmitters` 16, arcs, shields, remote bombs, ricochets, claw
records, the CLUT animation tables). Step 4 added, under `TM2_PORT_CAR_SOUND`: the human loops (turbo, guns,
burn/lava) of players 2.. as instance slot + 1 (D item 5: no more leaked requests), with bit 4 Twister's tornado
sound per car and room for 4 in every special-fire sound's list; under `TM2_PORT_CARS`: the sound request pool
35 -> 4 N (64). The `clutAnimStartFadeOut` re-trigger guard was not added (33-1's bit 7 makes the fade per car; the
guard would only act with bit 7 off). What stays shared and why: `CAR_GROUPS.md` 16.5.

- `renderWorldModels` 20 → 48 (`renderAddWorldModel` 801512F8, `renderRemoveWorldModel` 80151338,
  `controls.c:282-300`; the clear in `engine_anim_hit_volumes.c:377`; the walker loop
  `engine_render_scene.c:190`, which FAR_LOD/FAR_DISTANCE already change: coordinate). Retail 20 first, port 28
  after (D5 style), under `TM2_PORT_CARS`.
- `weaponFlameEmitters` 2 → `TM2_PORT_CARS_LOADED` (`weaponFlameSlotFind` 8010A5AC, `weaponFlameUpdate`
  8010A7A0 loop, `weaponsRoundReset` loop).
- Pools of A.4 the port asks for after measuring (arcs 5, shields 4, remote bombs 2, ricochet 10).
- `clutAnimStartFadeOut` re-trigger guard (optional, bit 3).

---------------------------------------------------------------------------------------------------------

## D. 16 humans later: what blocks `TM2_MAX_HUMANS` = 16 (keep these macros now)

1. **Step 2's formula `TM2_MAX_AI_CARS = TM2_PORT_CARS - TM2_MAX_HUMANS`** gives 0 AI records at 16 humans,
   and none for the Juggernaut's parked Minion. Recommend: `TM2_PORT_CARS` = active cars (16),
   `TM2_PORT_CARS_LOADED` = loaded cars (17 for the Juggernaut alone; ~30-32 with online spares and joins, B.7.3), `TM2_MAX_AI_CARS` =
   `TM2_PORT_CARS_LOADED` (or a separate macro), independent of humans; the run-time rule
   `humans + opponents ≤ loaded` checked where the settings are built. `TM2_MAX_CAR_SLOTS = 2 *
   TM2_PORT_CARS_LOADED` (34), `TM2_CAR_SLOT_CAP` ≥ it; `TM2_CAR_KEYS` stays 32 (17 loaded → ≤ 16 overflow keys).
2. **Object id 8** (`weaponApplyHit` 800DC778 `(u32)(id-1) < 99 && id != 8`; `carContactResolve`
   `car_contact_resolve.c:253`; hit result 8 = "no car" in `car_damage_human.c:51`, `projectile_hit.c:230`,
   `weapon_bullets.c:276`, `engine_hit_detection.c:242`). Human 7 gets id 8. Add now:
   `TM2_HUMAN_OBJECT_ID(i)` (`i < 7 ? i + 1 : i + 2`, ≤ 17 < 50) in `carBindHumanSlot` 8011A3FC and the swap,
   and `TM2_HUMAN_INDEX_OF_ID(id)` for the one id→index site, `weaponDropCheckHit` 80109A9C
   (`car_damage_human.c:70`: `humanCarGet(owner - 1)`). Ids < 50 = human stays valid up to 48 humans.
3. **`humanCars[i].player` is also the pad port, view, camera and HUD index** (`carBindHumanSlot` sets
   `player = i`): `viewSetFollowObjPos(target, i)` for every human every frame (`viewFollowObjects`
   [TM2_MAX_VIEWS]), `viewCarCameraUpdate`/`viewSetCameraPose(player)`, `carSetViewMode`, `padCheckAction(..,
   player)` (`car_input.c`), `padComboHeld`, `carViewHotkeys`, `hudSmoothSpeed(i)`,
   `hudRadarPlotCar(.., j)` in the variant's N-human radar loop (`world_update.c`, `j < humanCarTotal` →
   `hudRadars[j]` sized by HUDs). With 16 humans and 4 local views these index past the view/HUD arrays.
   Needed: a per-human "local view" (-1 = remote) consulted by every view/HUD/pad use; keep `TM2_MAX_VIEWS`/
   `TM2_MAX_HUDS` at 4 and never index them with a human index. **Steps 2-4 must not add new uses of a
   human index as a view index.**
4. **Arrays not yet sized by `TM2_MAX_HUMANS`**: `carResetWeapons` (4 bytes, not moved), `hudRadarVisible`,
   `hudWeaponListVisible`, `padLayout` (4 bytes: per local player, fine if 3 holds), `sessionSettings.lives[4]`
   and `control_layout[2]`, `sessionPortHumanCars[TM2_MAX_HUMANS - 2]` (ok), `padActionCache`/
   `padActiveBindings`/`padConfiguredBindings` (per pad port: local only).
5. **Done (request 33-4, `CAR_GROUPS.md` 16.1, with `TM2_PORT_CAR_SOUND`): players 2.. play these loops as
   instance slot + 1.** Was: **Sound loops by side 0/1** (`soundMgrPlaySided`/`StopSided` `sound_manager_play.c:66-170`: turbo, gun,
   loop 32; flags `soundLoop32ActiveP1/P2` etc.): a human ≥ 2 leaks one pool object per frame
   (`FOUR_HUMANS.md` 3). Needs per-human instances (as the AI's gun loop) before more than 2 humans make
   sound.
6. **Fixed-player rules** (port's, Q10): round end on `humanCars[0]` (`world_update.c:425`), deathmatch
   results 3/4 (`humanCarDamageUpdate`), two-sided tallies, `lives[0]` decrement, `weaponFindTargetAhead/
   Behind` aiming at player 1, `bulletsUpdate`'s `humanCarObject(0)`.
7. **Per-human records**: 16 × 0x814 = 33 KB (fine in `.port_bss`); the variant's `#if TM2_MAX_HUMANS > 2`
   paths already loop to `humanCarTotal`; `carTablesClear`'s count-down from `(N-1) * 0x814` (fine).
8. **Loops that grow O(N²)** with `rand()` inside (the variant's radar pair loop: `rand() % 5` per pair):
   game-state-neutral but they change `rand()` order with the human count; online requires every machine
   to run them identically (they do, same code), but the port's "visual stream" split (request 1) should
   take them.
9. **Settings and pickers**: the human car indices are u8 car-select indices (D.10 of E applies).
10. **Static assert** `TM2_MAX_HUMANS < 8` (step-2 plan 2.1) stays until item 2 is done; then `≤ 48`.

11. **One total of 16, either mix (16 humans, or 1 human + 15 bots, players joining and leaving).** Records,
    active cars and loaded cars are separate limits:
    - human records `TM2_MAX_HUMANS` = 16 and AI records `TM2_MAX_AI_CARS` = 16 (+1 for a parked
      Juggernaut Minion), **each** able to reach 16, neither derived from the other (item 1); storage
      16 × 0x814 + 17 × 0x854 ≈ 70 KB of `.port_bss`;
    - active cars ≤ `TM2_PORT_CARS` (16): a run-time rule (`humans + bound bots ≤ 16`) where the port builds
      the settings and in `portCarStartHuman`/`portCarStartAi` (a join first removes a bot), not an array size;
    - loaded cars/groups/slots/keys by `TM2_PORT_CARS_LOADED`: 16 active + spares + cars loaded during the
      match + a parked Juggernaut Minion (B.7.3: about 30-32 groups, ~48 slots, `TM2_CAR_KEYS` 48); removed
      cars stay loaded as free cars;
    - `aiCarTotal` counts AI records ever bound (vacant ones included, B.6.3); `humanCarTotal` counts
      contiguous humans (B.6.3 step 4, `portHumanCompact`); a joining human is always index `humanCarTotal`;
    - per-human arrays (`humanCarObjects`, `..Slots`, `..Types`, `padConfigs`, `hudSmoothedSpeeds`,
      `carViewHotkeyReleased`, `carResetWeapons`, `sessionPortHumanCars`) at 16; per-view/HUD arrays stay at
      the local maximum (4) behind the human → local-view map (item 3);
    - settings: `humans` up to 16 with the cars of humans 3..16 in `sessionPortHumanCars[14]` (already
      `[TM2_MAX_HUMANS - 2]`), opponents ≥ 9 in step 2's `sessionPortOpponentCars`; the 0xFF clears and
      `settingsCopy` by counts (`FOUR_HUMANS.md` 5.2) bounded by the macros;
    - object ids: humans 1..17 skipping 8 (item 2), bots 50..66;
    - sound: per-human turbo/gun/loop-32 instances (item 5) — with 16 humans the side-0/1 lists leak at the
      3rd human;
    - the human index changes when a lower-index player leaves (`portHumanCompact`): every port-side table
      keyed by human index (its HUD, net player ↔ human map, local-view map) must follow;
    - human 0 (the host) is never removed while the round-end rule reads `humanCars[0]` (Q10);
    - mid-match starts use retail's respawn path (`carBindHumanSlot`/`carRestart`, `carBindAiSlot`/
      `aiCarStart`) plus the per-record part of `uaInit`; `carSetViewMode` only for humans with a local view.

What steps 2-4 should do now to not make this harder: use `TM2_MAX_HUMANS`, `TM2_MAX_AI_CARS`,
`TM2_PORT_CARS(_LOADED)`, `TM2_MAX_CAR_SLOTS`, `TM2_CAR_KEYS` everywhere (no 4, 9, 12, 15, 16, 32 literals);
never use a human index for a view/HUD/pad array; route new per-human ids through the item-2 macros; key
every new per-car table by key or slot, not by human index.

---------------------------------------------------------------------------------------------------------

## E. Mod cars: what the C needs, and what steps 2-4 must not make harder

**E.1 Model and textures from port data, in its own group, keyed as a duplicate of its base type.**
- `sessionLoadArenaAndCars` loads per group by file name (`carTmsFileName`/`carDmdFileName(settings, index)`,
  `shell_main.c:280-310`). Step 2 rewrites the two loops anyway (high DMD area): make the per-group load
  **one port function** `portCarLoadGroup(n, car, pass)` (pass = TMS or DMD) that reads the disc file, or a
  port-provided buffer `{ptr, size}` from a table `portCarSource[group]` when set. Then a mod only fills the
  table.
- The mod's DMD must use its base type's ids (type + 0..15, and the base's special asset ids 2321..2326,
  201/202, 249) because `worldLoadObject` dispatches by id; the root registers with the base type and
  gets a key like a duplicate (step 1 already allows it). The port's mod packer must enforce this.
- **Texture keys**: `vramPlaceTexture` reuses any image whose TMS key is already loaded (arena or car).
  A mod TMS reusing its base car's keys would silently draw the base car's textures. Either the packer
  gives mod images unique keys, or a per-group "no reuse" flag (`portCarGroupNoReuse[n]`) in the
  `vramPlaceTexture` change of C.1 (the same branch). Recommend the flag (the port controls it; banks of
  step 4 need the same "place, don't reuse" path).
- **Image count**: slots are `group * 20 + image`; a mod with more than 20 images (Dark Tooth has 31)
  corrupts the next group (`SURVIVAL.md` 4.2). Do the **stride-32** fix (four functions) in step 2 or 4
  under the define, not later.
- Per-car CLUT copies (C.1) work for mods unchanged (they act on whatever the group parses).

**E.2 Per-car stats, weapons, camera, AI.**
Everything below is a switch or table **by type** today: `carStatsLoad` 800FE15C (`carStats*` per type),
`carDefaultTopSpeed`, `carWheelsSetStepHeight` 800FF638, `carBuildCollisionBox` (from stats),
`weaponBlockInit` 800FDCAC (`ai_route_setup.c:265-325`: recharge, capacity, max specials), `viewCameraOffsetsInit`
800FF764, `aiCarSetupProfile` 800D3C7C (weights `ai*Weights9/10` 80162608.., fire chance), engine sound id
(A.2 row 11), radar colour (`hudRadarPlotCar`), `carHasMeleeSpecial` 8011D854, `carDisplayName` 8012AEC4 /
target panel name. Recommend:
- a per-key **profile** table `portCarProfile[TM2_CAR_KEYS]` (valid flag; stats block overrides or a whole
  `TM2CarStatsBlock` image; recharge/capacity/max specials; step height; top speed; camera offsets; AI weight
  table pointers and fire chance; engine sound id; radar colour; name pointer), consulted by one helper
  `portCarApplyType(rec, human, key, hp)` (B.2) — **write the swap's type change as that helper now**, and
  have `humanCarPlaceAtStart`/`aiCarReset`/`aiCarStart` call it under a later mod define instead of the
  scattered per-type calls; with no valid profile it runs exactly the retail calls.
- the **behaviour** (special, AI special code, Dark Tooth gates, `carHasMeleeSpecial`) stays by the base
  type (`key_type`), as the port proposes.

**E.3 Per-car names.** `carDisplayName(index)` and the target panel (`carTypeName`) are by type; give the
profile a name pointer and route the target panel through it (the port draws the kill feed and results).

**E.4 What in steps 2-4 would make mods harder (avoid now):**
1. **Car identity as a car-select index (u8 0..14)** in settings (`human_car`, `opponent_car`,
   `sessionPortHumanCars`, step 2's `sessionPortOpponentCars`) and `carIndexToType`: reserve a range (e.g.
   ≥ 0x20 = mod car `index - 0x20` in a port table, base type from it) and never index a 15-entry table
   with a settings value without a bound.
2. **Hard-coding "first car of a type = key type"** in new code: always go through `TM2_CAR_KEY`/
   `portCarKeyType`, so a mod (or a duplicate) on the type key works the same.
3. **Static per-type CLUT lists** (e.g. "copy images 0 and 1 for Spectre"): C.1 avoids them (it copies what
   the group reuses).
4. **Per-type stats called from many places**: concentrate in `portCarApplyType` (E.2).
5. **The 20-image slot stride** (E.1).
6. **Assuming `DMD\XX.DMD` sizes** for the high DMD area (step 2's 64 KB per car in `portCarDmdArea`): size
   by the actual file/buffer (`off += size`), and bound the area (a mod DMD may be larger: Dark Tooth is
   61,660 bytes, a mod could exceed 64 KB).

**E.5 Built (request 42, 2026-10-16): 16-bit car numbers, `TM2_PORT_CAR_NUMBERS` (`car_numbers.h`).** E.4.1's range
is now built: numbers 0..14 retail, 0x20 + m mod m (m 0..127, so 0x20..0x9F), -1 none. A mod is described by the port
in `portModCars[m]` (used, base car 0..14, name, file code, bio picture, ending movie). sessionSettings' numbers live in
`portSettingsHumanCar[16]` / `portSettingsOpponentCar[16]`; the record's bytes keep the **base** car (the mirror), so
the 48-byte record, its 13-byte clears and `settingsCopy` stay retail's and every byte reader sees a retail car. The
setters write both, the clears both, the getters return an int (`TM2CarNumber`: the table's mod number when its base
is the byte, else the byte). The numbers reach: `carIndexToType` (a mod: its base's type: the bind), `carDisplayName`
/ `carFileCode` (the mod's name / code or the base's: names, plates), `carIsSelectable` (int compares, numbers that are
no car refused), the passwords (none past 11), `screenEnding` (the mod's movie; `endingMovieTable` only below 14),
`sessionLoadArenaAndCars` with `TM2_PORT_CARS` (the number to `portCarLoadGroup`, which records it per group and falls
back to the base's disc files), `shellAttractDemo` (keeps the numbers), `portHumanCompact`. Keys never collide with
numbers: a mod registers as a duplicate of its base type (E.1). `CUSTOM_TOURNAMENT_ANSWERS.md` 3 lists the functions
with file:line; the port stores mods by string id and assigns numbers per session.

**E.6 Built (requests 70 and 71, 2026-10-26).** E.2's profile is `TM2_PORT_CAR_PROFILE` (`portCarProfile[m]` per mod:
stats overrides, step, special numbers, camera offsets, radar colour, target name; the behaviour and the AI's weight
tables stay the base's), and the carousel's in-game model is `portCarSelLoad` (`CAR_PROFILES.md`).

---------------------------------------------------------------------------------------------------------

## F. Questions for the port

1. **VRAM rows 240..255 (x 0..639):** does the port's renderer (4-view split, widescreen, in-between frames)
   ever write there? C.1 puts per-car 256-colour CLUT copies there first (32 spots) and keeps every first
   car on its retail row.
2. **D5 (retail storage for the type key, port arrays for duplicates):** accept? It keeps the special state
   and the claw-record pointers in the car records byte-equal to retail in your comparisons.
3. **Swaps across rounds (modes 3/4 rebind every round):** a port table `portCarBindGroup` consulted by
   `sessionBindCars` (one line), or does the port re-apply its swaps after each rebind?
4. **Swap and weapons:** keep `energy` (energy-attack meter) and `turbo` with the pickups? `ammo[0]`
   (specials) reset to the fresh 2, or keep the count clamped to the new type's `max_specials`?
5. **Swap and homing projectiles/lock-ons** on the swapped car: retarget them to the player's new object
   (default in B.3), or let them follow the old model?
6. **Dark Tooth per car (33-2e):** per-car head and boss state only (retail level-8/`aiCars[0]` rules kept),
   or also `SURVIVAL.md` 1.5/1.6 (boss weapons and stage 2 on any arena, per-car stage loop) behind another
   `portCarOptions` bit?
7. **Engine voices:** 16 engines at once: does the port's SPU emulation have more than 24 voices, or should
   the C cap concurrent engine requests (nearest N)? And does the port compare sound-request state frame by
   frame (then C.3 uses instance 0 for type keys, so the requests are byte-equal)?
8. **Loaded vs active cars:** `TM2_PORT_CARS` = 16 active and `TM2_PORT_CARS_LOADED` = 17 (Juggernaut's
   parked Minion; more for Survival preloads?) — what maximum of loaded cars does the port want?
9. **Pools of A.4** (arcs 5, shields 4, remote bombs 2, ricochet 10, flame emitters 2, claw records 4,
   `renderWorldModels` 20, CLUT animations 20/10): raise all with `TM2_PORT_CARS`, or only the ones this
   plan marks as blocking (claw records, flame emitters, `renderWorldModels`, CLUT animations)?
10. **Mods:** unique texture keys from the packer, or the per-group "no reuse" flag (E.1)? And the settings
    index range for mod cars (E.4 item 1)?
11. **Removed car's projectiles:** when a car leaves, its live projectiles, mines and drops: removed at once,
    or left flying with no owner (no kill credit)?
12. **Spares at match start:** which types does the server offer to joiners per arena (B.7.3 limits: all 14
    on Moscow/Holland/Cyburbia/Roof Tops, 12 on Los Angeles/New York/Hong Kong, 8-9 on Paris/Swamp), and is
    "a type not offered cannot be picked mid-match" acceptable until step 4?
13. **Human index changes on leave** (humans must stay contiguous): acceptable that the last human moves
    into the leaver's index (the port remaps its net-player table), or should the C allow holes in the human
    array (a change to every `humanCarTotal` loop and the null-object `break` in `worldUpdate`)? And the host
    (human 0) never leaves?
14. **Loaded-car budget:** the maximum number of groups per match (16 active + spares + mid-match loads +
    the Juggernaut's Minion), and a 2 MiB DMD area below 0x80600000 instead of Q13's 1 MiB?
15. **Mid-match data:** the port hands the DMD (and, after step 4, TMS) as a buffer (no CD read from the C
    while the battle runs): OK?

## G. The port's answers (2026-10-12, port repo docs/DECOMP_REQUESTS.md request 33)

Copied as given; they override the plan where they differ. Already applied in step 2 (CAR_GROUPS.md
section 7): 8, 9, 10 (stride 32, DMD packed by size, bounded per-type tables), 13, 14.
The port also prefers loading a joining player's car on demand (B.7) over preloading.

**The port's answers to `PER_CAR_PLAN.md` section F** (2026-10-12; go ahead with the plan as written where
not said otherwise):
1. **VRAM rows 240..255 (x 0..639):** free. Every view (1-4, the split screens, the in-between frames, the
   kill cam and photo mode) draws with the game's own draw areas, 320x240; the port's HD backgrounds load
   exactly (0 or 320, 0, 320, 240). Already used: the port's overlay frees these rows for Survival with Dark
   Tooth (`bitmapInit` reserving 240 rows when `portConfig.boss_anywhere`, decomp/ overlay), tested
   without a glitch. Let C.1's allocator own them; the port drops its own `bitmapInit` change then.
2. **D5 (retail storage for type keys, port arrays for duplicates):** accepted.
3. **Swaps across rounds:** a port table `portCarBindGroup` consulted by `sessionBindCars` (the C keeps the
   swap through a rebind; nothing for the port to re-apply).
4. **Swap and weapons:** keep `energy` and `turbo` with the pickups; `ammo[0]` kept, clamped to the new
   type's `max_specials`. A mode that wants otherwise sets it after the swap.
5. **Homing projectiles and lock-ons on a swapped car:** retargeted to the new object (the default).
6. **Dark Tooth per car:** yes, also `SURVIVAL.md` 1.5/1.6 behind a `portCarOptions` bit. The port's overlay
   already carries 1.5 for Survival behind `portConfig.boss_anywhere` (`weapon_tick_init.c`, `scene_init.c`
   per-car stage 2, `car_restart.c`, `screen_pause_menu.c`, `car_object_contact.c`, `car_damage.c` wreck
   state 4, `projectile_hit.c`; see the port's `decomp/patches/0001-port-hooks.patch`): take what is
   useful; the port moves to your bit and drops its version when 33-2e lands.
7. **Engine voices:** the port's SPU has 24 voices, as the PS1. Cap concurrent engine requests in the C
   (the nearest N to any human, N in a port word, default 8). [Changed by the user 2026-10-13: default 0 = no
   cap, engines below the effects when voices run out; `CAR_GROUPS.md` 12.] Sound state is not compared frame by frame
   (the port's state hash skips the sound pool and libsnd voices: they follow real time), so C.3 need not
   keep the requests byte-equal for that.
8. **Loaded cars:** `TM2_PORT_CARS_LOADED` = 32 (16 active + the Juggernaut's Minion + spares + joins).
9. **Pools of A.4:** raise all with `TM2_PORT_CARS`, about one per car where a car can hold one (16 of one
   car is the target: 16 Outlaws tasering, 16 shields, 16 remote bombs): arcs 16, shields 16, remote
   bombs 16, ricochet 32, flame emitters 16, claw records 16, `renderWorldModels` 40, CLUT animations
   doubled. The port measures the rest.
10. **Mods:** the per-group "no reuse" flag (the port controls it). Mod cars in settings: indices from
    0x20 (`index - 0x20` into a port table naming the base type), every 15-entry table bounded. Please do
    the stride-32 fix in step 2, and size the DMD area by the actual file (no 64 KB per car).
11. **A removed car's projectiles, mines and drops:** removed at once (no orphan owners).
12. **Spares at the match start:** every type that fits on the arena (B.7.3). A type not offered cannot be
    picked mid-match until step 4: acceptable (the port greys it out in the joiner's menu).
13. **Human index on leave:** compaction is fine (the port remaps its net-player table). Human 0 never leaves
    for now (the host leaving ends the match). Later the port may want a server with no local player
    (dedicated): keep that possible (no new code that needs `humanCars[0]` beyond the round-end test, Q10).
14. **Loaded-car budget:** 32 groups (slots and keys sized to fit, with Minions' second roots), and a 2 MiB
    DMD area, linker-placed, ending below 0x80600000. Report its end; if a later need passes 0x80600000
    the port moves its own high RAM up.
15. **Mid-match data:** yes, the port hands the TMS and DMD as buffers; no CD read from the C during a
    battle.
