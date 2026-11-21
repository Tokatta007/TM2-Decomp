# Sudden Death: soldiers on foot and storm clouds (PC port request 43, answers, 2026-10-17)

The port's request 43 (`docs/DECOMP_REQUESTS.md`) describes a Challenge Match option that ends a match running past
its time limit, in three kinds:

- **Soldiers, then storms.** At the limit every player who is out comes back as a soldier on foot. The soldier walks,
  aims and shoots, and dies in one hit. After each death it comes back with a stronger weapon: machine guns, then
  missiles, then a special (Thumper's flame, Outlaw 2's taser). Its last life is a storm cloud. Later in the match,
  clouds also appear by themselves.
- **Storms by players.** The players who are out come back as clouds straight away.
- **Storms alone.** Nobody steers the clouds. They appear, drift and strike lightning around themselves, and more
  appear as time passes.

This page answers the six questions with file:line on branch `decomp-requests-42` (commit 1a93a33). It covers how
retail handles a player who is out, the clock, scoring and determinism, and it ends with a design, effort estimates,
risks and questions. **Nothing has been built.** Every addition below sits behind `TM2_PORT_SUDDEN_DEATH` and a port
word. With the word at 0 the logic and the `rand()` order stay retail's.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the code but was not traced to the end or seen
running. **GUESS** = an estimate.

| Q | Short answer |
|---|---|
| 1 Pedestrians | Six billboard sprites (`targetActors`, ids 6000 + i). They are hit by a distance cube and have no physics. They cannot be driven or own a weapon. **Recommended:** the soldier is the out player's **own human record**, still bound to its car slot with its real car type. A walker tick replaces `humanCarTick`, the car and its shadow are hidden, and the soldier is drawn as its own sprite object (the Dark Tooth head pattern). |
| 2 Camera | `viewChaseCamera` needs only the record's position, heading and speed. Give the soldier its own offsets with one branch in `viewCameraOffsetsInit`, keyed on a port flag (not on the type). A low view also needs a pitch bias per record. |
| 3 Lightning | Arc kind 1 is the **Lightning pickup** (weapon 7, levels 1, 2, 8). It already strikes from a fixed point in the air to a car. A cloud can reuse it with its own point. Credit is the arc's +0x02 slot, which retail never writes for lightning (stale: retail bug 7). With an owner of -1 the bolt does not hurt computer cars. The arcs **tick in the draw, once per view**. |
| 4 Cloud | Long-lived fx sprites from the `fxSmokeFramesA` sheet (subtractive, so it darkens), drawn in every view. The ground patch is a grid of the port's own quads placed with `worldGroundHeightAt`. The car-shadow mechanism cannot be reused. No new VRAM is needed. |
| 5 Tiers | Machine guns, missiles, the taser and Thumper's flame all work from a human record with no car model. They need the record's `motion` block, its stats muzzle constants, and (for the flame) the car's world object kept at the soldier's position and heading. |
| 6 Limits | A soldier or cloud of an out player uses that player's existing record and slot, so it adds nothing to the 16 cars. A cloud costs about 4 to 8 of the 150 fx sprites and one arc per strike (16 arcs). Soldiers share the 48 projectiles and 40 bullets. RAM is a few hundred bytes. |

---

## 1. Pedestrians, and what a soldier needs

### 1.1 What TM2's pedestrians are (CONFIRMED)

- **Records.** `targetActors` @80190E54 holds 6 records of 60 bytes (`target_actor_state.c:6`), counted by
  `targetActorCount` @80180BB8 (`:4`). Layout (`target_actor_update.c:5-18`):

  | Offset | Field |
  |---|---|
  | +0 | start row |
  | +1 | leg |
  | +2 | state: 0 roaming, 1 thrown, 2 settling |
  | +4 | kind: the DMD id 514 / 516 / 517, -1 = free |
  | +6 | instance |
  | +8 | timer |
  | +10 | speed |
  | +12 | goal[3] |
  | +24 | the thrower's matrix (32 bytes) |
  | +56 | the world object |

- **Kinds.** 514 and 517 walk between two points; 516 stands (`target_actor_update.c:103`, `target_actor_init.c:31`).
  No retail level uses 517. **The breakables 500..510 and 519..520 are not pedestrians.** They are the target pool
  `targetPool[50]` @801909D0, with HP set by `targetRegister` @80124F38 (`target_pool.c:40-101`) and damage from
  `targetDamage` @80125118 (`target_executor.c:70`).
- **Loading.** `worldLoadObject` @800D1DB0 (`world_load_object.c:182-186`) calls `objRegistryAdd` and then
  `targetActorSpawn` @80125888 (`target_actor_lifecycle.c:38-59`). The spawn:
  - runs only when count < 6 and the node is a type-9 select node;
  - allocates with `csAlloc`, then calls `csSetModel(obj, node)` and `csListAdd`;
  - sets +8 = 1 (drawn), +0xC = 0 (no collision), +0x118 = 6000 + i;
  - places the pedestrian with `targetActorPlace` @80125A18 (a random start row from `targetStartPositions`
    @80163DB8, with `rand()` jitter, `target_actor_init.c:34-37`).
- **Look and animation.** A pedestrian is a **billboard sprite**, not a 3D model. Each frame is one camera-facing
  prim, 68 x 66 units (`docs/SANTA.md:17-23`). The select node has three steps: 0 hidden, 1 standing or walking
  loop, 2 hit animation, which chains back to 0. The steps are set by `objRegistrySetStep(kind, inst, step)`
  @80124384 → `animSetStep` @80151F14, which writes child +7 **in the shared node data**
  (`engine_anim_hit_volumes.c:626-633`).
- **Movement.** `targetActorsUpdate` @80126464 moves the pedestrian 7 units per update along its own matrix
  (`target_actor_update.c:104-110`). It turns with `ratan2` + `RotMatrixYXZ` (`:115-116`). The height never follows
  the ground; 201 units above the ground means hidden (`:100-101`).
- **Death.**
  - **Run over:** a car within `dist < 10` (a cube of ±80 units, `:137`, `:161`) throws the pedestrian.
  - **Weapons:** `projectileHitTest` returns 6000 + i (`weapon_slam_swing.c:159-164`), then `weaponApplyHit` calls
    `targetActorHit` (`weapon_missile_trails.c:345-346` → `target_actor_events.c:58-85`).
  - **Splash, Axel, ricochet, slam, tornado and ground fires** each loop over `targetActorsGet()` themselves.
  - In every case the pedestrian is thrown for 42 updates, settles, and comes back about 302 updates after the hit.
    No shooter is recorded.
- **Why 6:** a fixed retail array, and every loop runs to 6.

**Verdict.** A pedestrian has no input, no weapon block, no HP, no camera and no HUD. Making one a player means
writing all of that again. **Only its look is worth reusing.**

### 1.2 What firing needs from the shooter (CONFIRMED)

**Machine guns: `UpdateGuns` @80104F70 (`update_guns.c:14-87`).** Called from `weaponTick` when the record's
`input.fire_guns` bit is set. Fields read:

| Field | Human record | AI record |
|---|---|---|
| player / index | +0x000 | +0x004 |
| type | +0x090 | +0x030 |
| `state.frozen` | +0x0A9 | +0x079 |
| aim pitch / yaw | +0x144 / +0x148 | +0x0CC / +0x0D0 |
| `stats.gun[3]` | +0x224 | +0x5D4 |
| `motion.pos` | +0x6FC | +0x73C |
| `motion.rot` | +0x708 | +0x748 |
| `motion.rot_m` | +0x72C | +0x76C |
| `weapons.gun_side` | +0x7CF | +0x80F |
| `weapons.mega_guns` | +0x7D0 | +0x810 |
| `weapons.gun_cooldown` | +0x7F0 | +0x830 |

- The owner id is `TM2_HUMAN_ID(player)` for a human and index + 50 for an AI (`:32`, `:40`).
- The muzzle is the stats constant `gun[3]`, mirrored by side, rotated by `rot_m`, plus `pos` (`:47-60`). **It does
  not come from the model.**

**Missiles: `weaponLaunchProjectile` @801052BC (`weapon_launch.c:64-146`).** The owner id is the car **object's** id
(`:76-86`). The muzzle is `pos + (0, 0, launch_height + 8)` (`:87-89`). It also reads `ride_height`, `rear_fire` and
`mega_napalm`.

**Homing: `weaponFindTargetAhead` @80109F94 (`weapon_targets.c:32-99`).** It takes the record index and reads the
car object's position and matrix.

**Bullets and projectiles themselves keep only an owner id.**
- `bulletSpawn` @800D7A4C (`weapon_bullets.c:201-243`).
- The 980-byte `fxRecords`: the owner is at +0x42 (`projectile_hit.c:24-42`).

**Credit.** `weaponApplyHit` @800DC65C (`weapon_missile_trails.c:322-371`) turns the owner id into a **slot** with
`carFindById`. An id it cannot find gives -1. `carApplyDamage(target, amount, dir, type, attacker_slot)` @800E5628
(`car_damage.c:113`) then:
- writes `last_attacker` (`:237-238`) and `killer` (`:253-254`) only when attacker >= 0;
- **lowers a computer car's HP only when attacker >= 0** (`:241-246`);
- doubles the damage if the attacker's record has mass 100 (Dark Tooth, `:157-160`);
- quarters AI-versus-AI damage (`:235-236`).

**So a soldier that fires under its player's own car id (that is, its own slot) gets kill credit and does full damage
to computer cars.** Any other owner does no damage to computer cars at all.

### 1.3 How hits find a target (CONFIRMED)

`projectileHitTest(pos, owner, radius, type)` @800D6FD8 (`weapon_slam_swing.c:49-166`) is a fixed scan, in this
order:

1. **Human car objects**, skipped only when the object is hidden and the record is dead (`:61-62`). The hit test is
   `DISTANCE(pos, obj->pos) < radius + 1`; the port adds +25 for a player's Dark Tooth (`:63-66`).
2. **AI car objects.**
3. **Breakables** (500 + i), tested only for an owner < 50.
4. **Pedestrians** (6000 + i).

The radius depends on the **shooter**: bullets 10 (human) or 5 (AI), missiles 15 or 8. In world units that is a cube
of ±88 / 48 / 128 / 72. Car-to-car contact is separate: `hdpPassRun` @8014FAB0 skips objects whose (+8 & +0xC) is 0.

### 1.4 Option A: a "car slot without a car" (a new object, ids 7000 + k)

What it would take:
- **Hittable:**
  - a new loop in `projectileHitTest` and a new id range in `weaponApplyHit`;
  - its own hooks in the six area-effect loops (splash `area_damage.c:95-101`, Axel `weapon_basic.c:349-354`, ground
    fires `fx_ground_fires.c:129-139`, ricochet `weapon_ricochet.c:184-190`, slam `weapon_slam_swing.c:313-319`,
    tornado `weapon_tornado.c:237-242`);
  - a copy of the run-over test.
- **Firing:** its own copies of `UpdateGuns`, `weaponLaunchProjectile` and the target search (they dereference a car
  record). It would still have to fire **under the dead player's id** for credit and AI damage.
- **Camera:** a copy of `viewChaseCamera`, which reads the record (`camera.c:82-230`).
- **HUD:** new sources (`hudDrawSingleView` reads `stats.hp / max_hp`, energy, ammo and turbo from the record,
  `hud_main.c:89-211`).
- **Radar and AI targets:** new paths.

Estimated at 2 to 3 times the work of option B, with more places to forget (GUESS).

### 1.5 Option B: the player's own human record, without car physics (recommended)

The player who is out still owns a human record that is **present and down**. It stays bound to its slot, its object
id is kept, and it is counted in `humanCarTotal` (H5: `portHumanDown` `port_variant_arrays.c:7355`). The soldier
**reuses that record**. The car `type` stays the real car's: `carStatsLoad` (`car_stats.c:186-189`), the per-key
tables, `carRestart`, the sound and `carBodyObjectShow` all index by type, and an unknown type falls back to Roadkill
or reads out of bounds. A port per-human flag (`portSdRole[h]`) marks the soldier.

| Subsystem | What happens with B |
|---|---|
| **Per-frame tick** | `humanCarUpdate` @8011C400 (`car_records.c:77-89`) runs input, hotkeys and `humanCarTick` @800DC88C (`car_tick.c:58-290`), then copies `motion.rot`, `pos` and `body_m` into the car object (+0xA0, +0xA8, +0x80). One gate in `humanCarTick` (or `humanCarUpdate`) runs **`portSdWalkerTick(h)`** instead: read `input` (forward +0x11, reverse +0x0E, steer +0x0F / +0x10, fire); turn in place (`rot[2] += k`); walk `pos += rot_m · (0, v, 0)` with v about 7 to 10 (pedestrians use 7); `z = worldGroundHeightAt(pos + 24)` @800EF340 (`car_wheels.c:924-958`; port example `portBalanceSpiralGround`); walls with `hdpQueryPoint` @80150A74 (as the Dark Tooth swoop does, `port_variant_arrays.c:2157`); write `motion.speed = v * 32` (HUD, camera lead); rebuild `rot_m` / `body_m` with `RotMatrixYXZ`; call `weaponTick` (guns, missiles, specials). Skipped: throttle and steering, slide, wheels, spin-out, Paris sink, crush, sway, attitude, rollover, wreck, `carPartsAnimate`, `carBodyFollowCar`, `carBodyPoseUpdate` (the shadow), skid smoke, regen, heal spots, cheats. No ramps, flips or falls by construction. |
| **Physics pass** | `carsPhysicsUpdate` @8011E220 (`car_physics.c:93-162`): `stats.probe_on` (+0x1EC) = 0 skips the probes. Also clear the record's contact and ground blocks, because `carContactsCombine` and `carContactPushUpdate` still run for every human (`:119-144`). Set the car object's +0xC = 0 so cars drive through it (`hdpPassRun`). |
| **Look** | Hide the car with `csSetDrawEnable(obj, 0)` and `carBodyObjectHide` (the shadow, `ai_route_helpers.c:116-158`). Draw the soldier as **its own cs object** (`csAlloc` + `csSetModel` + `csListAdd`, template `portCarSpecialAsObject` `port_variant_arrays.c:2567-2583`), placed from the record every frame like the Dark Tooth head (`portCarDtPlayerHead` `port_variant_arrays.c:2168`, called at `car_tick.c:156`). The model is **a private copy of a pedestrian select node**, because `animSetStep` writes the shared node's child byte and would otherwise flip the level's own pedestrian (`docs/SANTA.md` 1.6). |
| **Being hit** | The car object stays at the soldier's position, drawn off and **not dead**, so `projectileHitTest`'s human loop still tests it (`weapon_slam_swing.c:61`). A gate beside the Dark Tooth +25 gives a soldier a smaller cube, e.g. `extra = -5` (bullets ±48 instead of ±88: GUESS, tune in play). A gate at the top of `carApplyDamage`'s death branch (`car_damage.c:249`) sends a hit on a soldier to `portSdHit(h, attacker)`. That keeps retail's `killer` / `last_attacker` stores, skips the car death (`carBodyObjectHide`, `debrisBreakModel` with the car's wreck, damage stages, `carDestroyed`'s 540-tick freeze) and puts the soldier down at once (1 HP). The six area effects and Twister already hit car objects, so they reach soldiers with no change. Run-over needs one port test (the pedestrian cube `dist < 10`) in the walker tick, against every live car. |
| **Firing** | `UpdateGuns` and `weaponLaunchProjectile` run as for any human: the owner id is the player's own, so kill credit and AI damage work (1.2). The muzzle uses the car type's stats constants. For a soldier the port overwrites `stats.gun[3]` (+0x224) with a small offset (e.g. (±8, 16, 40): GUESS), `launch_height` (+0x190) with about 40, and `ride_height`. Side effect: a soldier whose car was Dark Tooth (mass 100) does double damage (`car_damage.c:157-160`); gate it or accept it. |
| **Camera, HUD, radar** | Unchanged (sections 2 and 6). The HUD divides by `max_hp` (`hud_main.c:109`), so keep `max_hp` > 0 and set hp = 1. The radar blip needs hp > 0 (`world_update.c:270-281`) and shows the car's colour (a grey unknown-type colour exists if the port wants a soldier colour). |
| **AI** | Computer cars target human cars by record (`aiGetTargetCar`, `carNearestToHuman` → `portHumanNearestCar` under H5), so **they will hunt soldiers**. Whether they should is a question for the user (Q-A). |
| **Down handling** | `humanCarDamageUpdate` @8011B89C (`car_damage_human.c:302-359`) still runs for the record. Its hp <= 0 path (wreck smoke, `carDestroyed`) must be skipped for a soldier; `portSdHit` sets `state.dead` itself. |

**Recommendation: B.** One record, one id, one camera, one HUD. The cost is about ten gates and one port unit.

---

## 2. The camera for a player on foot (CONFIRMED)

- **Path.** `worldUpdate` → `worldHumanCamera` (`world_update.c:166-172`) → `viewCarCameraUpdate` @80102BE8
  (`camera.c:50-79`): views 1-4 and 7 go to `viewChaseCamera` @80102CC4, views 5 and 6 to `viewFixedCameraTrack`.
- **Record offsets.** +0x0C `cam_rot` and +0x14 `cam_pos` are the **camera's**, not the car's. The car's pose is
  `motion` at +0x6DC.
- **What `viewChaseCamera` reads** (`camera.c:82-230`): `motion.rot`, `pos`, `rot_m`, `speed` and the lock bytes
  `falling`, `f1D`, `f20` (`:108-111`). `stats.steer_limit` is read only when `state.f06` is set (`:156-167`). Views
  2 and 3 do not touch the model or the other stats.
- **Offsets.** `viewCameraOffsetsInit` @800FF764 (`ai_car_start.c:253-298`) fills:

  | Field | Value | Used for |
  |---|---|---|
  | `cam_look_offset` | {0, 40, 0} | the look-at point |
  | `cam_chase` | {0, -280, 112} | views 2 and 7 |
  | `cam_offset3` | {0, -480, 168} | view 3 |
  | `cam_offset` | {0, -1200, 800} | other views |
  | `cam_split1` / `cam_split2` | {0, -176, 80} / {0, -240, 80} | split screen |

  Per-type tweaks are at `:270-283`. The request-36 Dark Tooth block (`TM2_PORT_DT_PLAYER`) is at `:284-297`. The
  function is called from `carRestart` (`car_restart.c:218`), `portCarApplyType` (`port_variant_arrays.c:3799`),
  `:5342` and `:7470`.
- **The soldier's offsets.** Add a branch after the DT block: `if (portSdIsSoldier(rec)) { ... }` with, for example,
  `cam_chase` {0, -150, 70}, `cam_split1` / `cam_split2` about {0, -120, 50}, and `cam_look_offset` z about 30
  (pedestrians are about 66 units tall: GUESS, tune in play). Call `viewCameraOffsetsInit(rec)` when the soldier
  starts and again when it turns back into a car or a cloud. The flag must survive `carRestart`, or the soldier
  start must run after it.
- **Pitch.** The pitch bias is a **constant per view, not per record** (`camera.c:200-215`: -159, -204, -227,
  split -193 / -102). For a truly low, close camera add a per-record bias there (one gate, `portSdPitchBias(rec,
  bias)`).
- **A cloud's camera.** For a player cloud, view 3's high offset ({0, -480, 168}) or `cam_offset` already looks down
  from above. The record follows the cloud's position (section 4.4), and the cloud's height is added to z by the
  port.

---

## 3. Lightning

### 3.1 Whose is arc kind 1 (CONFIRMED)

The 20-byte `weaponArcs` records (retail @801847B4, 5 of them; one per car with `TM2_PORT_CARS`, `players.h:77`,
`:157-158`):

| Offset | Field |
|---|---|
| +0x00 | id |
| +0x02 | owner **slot** (the damage source) |
| +0x04 | count (40 at the shot; <= 0 = free) |
| +0x06 | kind: 0 taser, 1 lightning |
| +0x08 | target **object id** (-1 none) |
| +0x0C | `from` pointer |
| +0x10 | `to` pointer |

Layout at `port_variant_arrays.c:7731-7737`. Watch the names: `weapon_arcs_draw.c` calls +0x06 "attachment".

- **Kind 0** is Outlaw 2's taser, created by `weaponTaserTarget` @801075C8 (`weapon_slot_target.c:51-135`).
- **Kind 1** is the **Lightning pickup**: weapon 7, pickup 419. `weaponFireSelected` case 7 (`weapon_select.c:178-196`)
  calls `weaponLightningScan` @80107B00 on levels 1, 2 and 8. That is the `weapon_slot_scan.c:62-81` the request
  asked about: the `SCAN_SLOT` macro and `scan_empty_slot`. It is neither a hazard nor Napalm.
- **The start is a fixed point in the sky.** LA (-4608, -2192, 800); Hong Kong (0, 0, 2560); Moscow the dish offset
  (0, 0, 1064) (`weapon_slot_scan.c:38`, `:146-156`).
- **Targets.** LA strikes every car within `dist < 250`, Hong Kong within `dist < 350`; Moscow strikes cars in four
  track regions. **The caster is included.** Each struck car gets `carSpinOut(.., 6)` and `fxExplosion48`. One extra
  bolt with no target is added, and `fxSheetStart(5, 250, 200, 200)` flashes the screen (`:196`).

### 3.2 How it deals damage (CONFIRMED)

`weaponArcsDraw` @801084D8 (`weapon_arcs_draw.c:261-372`) runs **inside the draw**: `streaksDraw` @800D8230 calls it
(`screen_card_draw.c:503`) once per view. For each live arc it:
- builds a 16-point polyline with `rand()` jitter (`:309-319`);
- spawns smoke and explosion sprites on `rand() < 512` (`:338-341`);
- decrements `count`;
- calls `carApplyDamage(target, 10, 0, 2, owner)` at count 39 and at count 0 (`:343-346`): 20 per bolt.

The arc has no radius: only the target chosen when it was made is damaged. An arc with target < 0 gets a free end
swinging around `from` (`:295-300`).

### 3.3 Can it strike from a cloud? Yes, with three caveats

- **Cloud to car.** Write a free slot (`weaponArcSlotFind`, `weapon_tables.c:30-41`) with:
  - count 40 and kind 1;
  - `from` = a **persistent** `s32[3]` holding the cloud's bolt point (read live every tick, so it may drift);
  - target = the victim's object id and `to` = `&victim.motion.pos`;
  - owner = the credited slot.

  `portBalanceTaserRetry` (`port_variant_arrays.c:8230-8290`) already writes an arc by hand.
- **Cloud to the ground.** With target < 0 the end point is overwritten by the free end. Use a target id that is not
  a car (8, the world) and point `to` at a persistent ground point. `carApplyDamage` then returns early, because
  `carFindById` finds no slot (`car_damage.c:161-162`). Check this against the drop-in clean-up, which ends arcs by
  target or owner (`port_variant_arrays.c:4956-4960`).
- **A small radius** is the port's own loop: every live car within r of the strike point, each getting its own arc
  or one `carApplyDamage`. `areaDamage(center, slot, big)` @8010B5C4 (`area_damage.c:48-103`) is an alternative,
  but its radius (75/100) and damage (25/50 minus distance) are fixed, and it also hits breakables and pedestrians.

### 3.4 Owner semantics and kill credit

- **The owner is a car SLOT** (human record +0x04, AI record +0x06), not an object id. Slot 0 is a real car; only a
  negative value means nobody. A player's cloud passes that player's own slot. `portHumanOfId` (`port_variant_arrays.c:6901`)
  maps an **object id** to a human; for a slot use `carFromSlot(slot, &human)` (`car_records.c:335-349`) and the
  record's `player`.
- **There is no kill-feed hook** in the C. The port's scoring reads `killer` at `carDestroyed` @8011BE10
  (`docs/GAME_EVENTS.md` 1). With a soldier or cloud firing under its player's slot, the existing classification
  credits that player.
- **Owner = nobody (storms alone):**
  - `carApplyDamage(.., -1)` hurts humans only. Computer cars still flash and spin but lose no HP
    (`car_damage.c:241-246`).
  - `carFromSlot(-1)` reads one entry before `carSlotIndices` (`car_damage.c:158` → `car_records.c:337`). In retail
    that entry is `carSlotKinds[14]`; in the port variant it is the last `carSlotKinds` entry
    (`port_variant_arrays.c:777-778`). If the value is >= 0 the code reads `carSlotKinds[-1]`, and the attacker can
    be taken for a human record. Then the Dark Tooth doubling (if that record is a DT) applies and the AI-vs-AI
    quarter is skipped. Retail's wall and train hits (attacker -1) already do this read. LIKELY, not run.
    **Candidate retail bug 31.**
  - **Recommended for no-owner bolts:** the port's strike helper calls `carApplyDamage` through a gate
    (`portSdNoOwner`, set around the call) that (a) treats attacker -1 as "not human" without calling `carFromSlot`,
    and (b) lets a computer car lose HP. Killer stays -1, which `carDestroyed` classifies as "the level". In Deathmatch
    nobody scores, as the port's rules say.

### 3.5 Retail problems the storm runs into

1. **Stale lightning credit** (`RETAIL_BUGS.md` 7, `GAME_EVENTS.md` 9.3): `weaponLightningScan` never writes +0x00
   or +0x02. The bolt's damage goes to whoever last used the slot's taser, or slot 0. Also, `weaponArcSlotFind`'s
   duplicate test by id therefore never works for lightning. The storm writes both fields itself, so it is not
   affected. Fixing the retail pickup is a separate choice (Q-F).
2. **Arcs tick per view, in the draw** (`RETAIL_BUGS.md` 15; `FOUR_PLAYERS.md:699-711`; `RENDER_ANSWERS.md` R3 (d)).
   Count, damage and the jitter `rand()` calls run once per view. Split screen and the mirror halve the bolt's life;
   four local views quarter it. Online, two machines with different local view counts draw `rand()` a different
   number of times and **desync**. This is already true for the taser, flame and drops, so the port must split the
   tick out of `streaksDraw` for netplay anyway (FOUR_PLAYERS' plan: tick twice per step as retail's 2P, draw per
   view). Sudden Death makes bolts frequent, so **it needs that split first**. Alternatively, storm damage can be
   dealt in the simulation by the cloud tick (count independent of views), with the arc only drawn; the jitter
   `rand()` remains a problem until the split is done.

---

## 4. A cloud object

### 4.1 Drawing: long-lived fx sprites (recommended)

- **Pool.** `fxSprites` @80185144, 150 x 48 bytes (not raised by `TM2_PORT_CARS`). `fxSpriteSpawn(pos, scale, rate,
  cells, table, depth, follow)` @80110918 (`fx_sprite_spawn.c:212-246`) takes the first free one; it returns 0 when
  all 150 are busy.
- **Lifetime.** `fxSpritesUpdate` @80110A40 (`car_restart.c:355-379`) runs once per step. A record lives
  `frames × (loops + 1)`; loops = 32767 lasts hours (`WORLD_MARKERS.md:125-126`). `owner_pos` (+44) can follow a
  live `s32[3]`, so a drifting cloud only moves its point.
- **Every view.** `rt_main.c:157-158` (view 0), `:174` (view 1), `:185` (the mirror); the port's views 2-3 use the
  same call. A clean draw (no `rand()`, no ticks).
- **Size.** A 31-texel cell at scale 255 is about 1980 units wide (`WORLD_MARKERS.md:176-188`). A cloud of 4 to 8
  sprites (a core plus puffs) at scale about 128 to 255 gives about 1000 to 2000 units. The half-size clamp is 512 px
  (`world_collapse.c:101-105`).
- **Picture.** `fxSmokeFramesA` @80186F64 (asset 660, 16 cells of 31x31, **subtractive**: it darkens the scene
  behind it) is a dark storm cloud as it is. `fxSmokeFramesB` (average) gives a grey one. They are the same image in
  every level DMD (`WORLD_MARKERS.md:243-258`). **No new VRAM.**
- **Depth.** OT index z >> 3, cull 8 ≤ z ≤ 32000 (`world_collapse.c:90-100`). With `TM2_PORT_FAR_DISTANCE` the far
  mapping applies (`portFarDistanceFxOtz` / `Limit`, `port_variant_arrays.c:699-711`). In retail a sprite beyond
  about 8192 sorts behind near geometry (`WORLD_MARKERS.md:194-222`): fine for a cloud high above.
- **Cost.** One POLY_FT4 per sprite per view, 160 bytes of packet each (`world_collapse.c:125-127`). 8 clouds x 6
  sprites x 4 views is about 31 KB of a 0x17CF0 (97 KB) buffer, or more with `portPacketBufferBytes`. GUESS: fine.
- **Caveat: `carRestart` wipes all fx sprites** (`car_restart.c:212`, via `fxSpritesReset`). The port's own
  respawns skip that wipe (`portHumanRestartRecord`, `port_variant_arrays.c:7446`; `portDropInRestartHuman`
  `:5326`), but a retail lost-a-life restart does it. The cloud tick must check its sprite records every step (frame
  >= 0 and table still smoke A) and respawn the missing ones. That is a deterministic simulation write, since sprites
  are simulation memory.
- **Not suitable:**
  - impact particles: gravity, 40-step life (`fx_sprite_spawn.c:318-341`);
  - world models (`renderWorldModels`, 20 / 40 slots): need a model tree, and the disc has no cloud;
  - a cs object with a borrowed model (shield 0x8A9): one more coordinate system, colour only through a shared CLUT;
  - pickup models: part of the level DMD (`PICKUP_LOOKS.md:33-61`).

### 4.2 The dark patch on the ground

- **The car shadow cannot be reused.** It is node 3 of each car's own model (object `type + 3`), posed by
  `carBodyPoseUpdate` @80114F24 from the wheel heights (`car_body_pose.c:32-82`) and drawn as a world model
  (`carBodyObjectShow`, `ai_route_helpers.c:137-157`). Moving it moves that car's shadow.
- **Recommended:** the port's own quads, linked into each view's OT from a hook after `fxSpritesDraw(rec, view)`
  returns. The frame record gives the OT (+0) and the packet cursor and end (+8 / +0xC). The camera comes from
  `viewGetEyeTrans`, `viewGetViewMatrix`, `viewGetProjScaleX/Y` and `viewGetCentre`; code template in
  `WORLD_MARKERS.md:410-531`.
  - Shape: a 3x3 or 4x4 grid of POLY_FT4 using a `fxSmokeFramesA` cell (subtractive), or flat POLY_F4 with a
    `DR_MODE` abr 2.
  - Ground fit: corners from `worldGroundHeightAt` (+24 start), queried once per step in the simulation and cached.
    Four-corner example: `strike_update.c:51-62`.
  - Depth: `portFarCompress` / `renderCompressDepth` with a small bias toward the camera.
  - Size about the cloud's strike radius, so the patch is the warning.
- **Cheaper alternative:** one flat fx sprite of smoke A at ground height (camera-facing, so it looks right only
  from above). Not recommended for the low soldier camera.

### 4.3 The bolt's look

The arc itself (orange `LINE_F2`, kind 1). `portVisual` bit 0 (request 42) puts each segment at its own depth, so
bolts hide behind walls. Add `fxSheetStart(5, ..)` for the flash; it is a screen-wide flash on every local view,
which is too much for frequent strikes (Q-D). The rumble or thunder is sound 0x40 as the pickup's
(`weapon_select.c:191`).

### 4.4 The cloud's simulation

The state lives in a port record `TM2PortCloud` (`.port_bss`, about 48 bytes each):

| Field | Meaning |
|---|---|
| pos[3] | position, s32 |
| vel[2] | drift |
| height | |
| bolt[3] | the bolt start point, which the arc's `from` points at |
| ground[4] | the cached corner heights |
| owner slot | -1 = none |
| timer | until the next strike |
| radius | |
| sprite indices[8] | |

- **The cloud tick runs once per `worldUpdate`**: drift, keep inside the arena box, count the strike timer, choose
  targets, write arcs, re-spawn missing sprites.
- **A player cloud** reads the player's input word (`portHumanAction` or the record's `input`) to drift, and fires a
  strike on the fire button with a cooldown. The record's `motion.pos` follows the cloud's ground point, so the
  camera and radar work.
- **A cloud with no player** drifts on a schedule (a port PRNG, section 7.4).

---

## 5. Weapon tiers on a soldier

| Tier | Path | Works on a soldier record? |
|---|---|---|
| Machine guns | `weaponTick` → `UpdateGuns` | yes; the muzzle is the stats `gun[3]` (overwrite for the soldier). Retail cooldown 2 ticks. |
| Missiles | `weaponFireSelected` → `weaponLaunchProjectile` | yes; muzzle at `launch_height + 8`; give the record ammo with the port's pickup or weapon setters. Homing `weaponFindTargetAhead` reads the car object's matrix, which follows the soldier. |
| Taser (Outlaw 2, 0x30) | special-as (`portCarSpecialAsSet`, `GUN_GAME_SPECIALS.md` 4.2) → `weaponTaserTarget` | **yes**; it needs only `motion.pos` and the car object's id and position (`weapon_slot_target.c:69-104`). The lift is on the victim (`carSpinOut(.., 6)`) plus the drawn start +30 (`ARC_LIFT`). Range `dist < 80` (±640) from the soldier. |
| Flame (Thumper, 0x70) | special-as → `weaponFlameStart` @8010A5F0 (`car_restart.c:280-326`), `weaponFlameUpdate` @8010A7A0 (`weapon_flame_update.c:55-155`) | **yes, if the car object follows the soldier**: the emitter reads the object's matrix +0x80 (forward = matrix · (0, 32, 0)), pos +0xA8 and id +0x118 every tick. With option B the object gets `body_m` from the walker, so the flame points where the soldier faces. The flame's hit test is `projectileHitTest(pos, own id, 20, -1)`. It also runs per view inside `streaksDraw` (3.5). |
| Storm cloud | section 4 | the record stays down (cloud flag), and the camera follows the cloud |

- **Special-as refuses** (`GUN_GAME_SPECIALS.md:330-336`) 0x20 Hammerhead (crush), 0x60 Grasshopper (the car
  leaps), 0xA0 Twister (draws its model), 0xC0 Mr. Slam (needs his crane nodes) and 0xF0 Dark Tooth (no fired
  special). Error -3 if the special's car file is not loaded; for the taser and the flame nothing extra is needed
  (no model). Error -4 if no coordinate system is left.
- **Muzzles.** Specials fire from `(0, gun_y, gun_z)` rotated, +24 z (`weapon_basic.c:161-165`). With the soldier's
  overwritten `gun[3]` they start at the soldier.
- **Ammo.** The port sets the tier's ammo when the soldier comes back. The HUD shows ammo, the selected weapon and
  energy (`hud_main.c:141-189`).

---

## 6. Sizes and limits with 16 cars

| Item | Limit | Soldier / cloud use |
|---|---|---|
| Car records and slots | 16 cars in any mix (`players.h:89-95`, `:318-320`) | **none extra**: the out player's own record and slot |
| Coordinate systems | 256 (16 N), 320 with `TM2_PORT_BALANCE` (`players.h:144-150`) | 1 per soldier sprite object; none per cloud. Check `csPoolCount` before `csAlloc`. Retail's bump allocator frees only at load, so allocate the soldier object **once per human per match** and reuse it. |
| Projectiles | 48 live (`players.h:141`; `portCarOptions` bit 3) | shared with the cars; missile tier |
| Bullets | 40 (`bulletRecords`, not raised) | shared; 16 cars plus soldiers firing can exhaust them (bullets then do not spawn) |
| Arcs | 16 live (`players.h:158`) | 1 per bolt for 40 ticks (2 s; shorter per view, 3.5). 8 clouds striking every 3 s is about 6 busy. Keep a reserve for tasers. |
| fx sprites | 150 (not raised) | 4-8 per cloud. 8 clouds is 32-64, leaving 86-118 for explosions and smoke. Recommend a cap of 8 clouds. |
| Drops | 96 with BALANCE | none |
| Packet buffer | 0x17CF0 retail, larger with `portPacketBufferBytes` | about 160 B per sprite per view, plus about 16 quads per patch per view |
| VRAM | | none (smoke A is in every level) |
| RAM | port sections end 0x805A293C with every define (`BALANCE.md:631`), limit 0x80C00000 | about 8 clouds x 48 B + per-human state 16 x 16 B + a private pedestrian node copy (about 600 B, `SANTA.md`) ≈ 1.3 KB |

---

## 7. A player who is out, the clock, scoring, determinism

### 7.1 Today (H5, `portHumanOptions` bit 1, CONFIRMED)

- **Death.** `humanCarDamageUpdate` (`car_damage_human.c:302-359`) hides the body, sets dead, runs `carDestroyed`
  (540-tick freeze, `car_destroyed.c:417`) and returns under RULES with no round end (`:334-339`). The record, slot,
  id and object stay; `humanCarTick` runs `carWreckUpdate` (`car_tick.c:132-133`, smoke `rand()`). The camera
  follows the wreck; the HUD shows lives (`hud_split.cpp:110-113`); the radar blip stops (hp <= 0).
- **API** (`port_variant_arrays.c`): `portHumanRules` :7336, `portHumanAlive` :7348, `portHumanDown` :7355,
  `portHumanAliveCount` :7360, `portRoundEnd(event, delay, winner, flags)` :7400, `portRoundResults` :7426 (from
  `sessionRun`, `session_runner.cpp:164-172`).
- **`portHumanRespawn(h, node, flags)`** (`:7499-7525`, via `portHumanRestartRecord` :7450) restarts the record on
  its own car, with fresh stats and HP, without retail's global wipes. One `rand()` for the heading. **This is the
  natural hook for "comes back as a soldier":** a new `portSdSoldierStart(h, node, tier)` = `portHumanRespawn` +
  soldier setup (flag, hp 1, muzzle overrides, camera offsets, car hidden, sprite object shown, the tier's weapons).
  Read `killer` / `last_attacker` before, since `carStatsLoad` clears them.
- **Drop-in** `portCarStartHuman` / `portCarStartHumanAt` (`:5373` / `:5380`) takes a **new** record, so it is not
  for a down human.
- **Lives** are `portHumanLives[h]` (`:968`), which the port fills; the C never decrements them under RULES.

### 7.2 The clock

There is **no time limit** in retail or the port C. `rtMainLoop` runs until a scheduled event fires; `timerFrameCount`
(`timer_frame.c:79`) counts from boot and is never reset per round.

**Recommended:** a `.port_bss` frame counter `portSd.frame`, zeroed in `roundStart` (the latch), ticked once per
`worldUpdate`. It is match state (hash and snapshot).
- The **port** decides the limit (its battle modes already own round end) and calls `portSdStart(kind)` between
  frames at the same frame on every machine.
- The C then uses `frame - start` for the cloud schedule (more clouds as time passes) and for the end of Sudden Death
  (`portRoundEnd(TM2_ROUND_PORT + n, ..)`).
- The port may instead count frames itself. Either way, count simulation frames, not `GetCurTics`.

### 7.3 Scoring

The C keeps no score (`GAME_EVENTS.md:10-13`). The port reads `killer` (a slot, stats +0x07) at `carDestroyed`, maps
it to a player, and applies its rules: a soldier or cloud kill counts in Deathmatch modes and not in lives modes.
**The C needs one helper for the port to tell "killed by a soldier or cloud"** from "killed by the car before it went
down": `portSdKillerRole(slot)` returns the role (car / soldier tier / cloud) **at the time of the killing blow**.
That needs a per-slot byte `portSd.killer_role[victim]`, written at `car_damage.c:254` beside `killer`, because the
role can change in the same frame.

### 7.4 Determinism

- **`rand()`** is the one BIOS stream (`bios_calls.c:55`), seeded at `roundStart` (`scene_init.c:66`). Every call
  advances it (`RAND_CALLERS.md:36-45`). Retail functions the design reuses draw from it: `carSpinOut` (`rand() &
  7`), `carApplyDamage`'s spin roll, `portHumanRespawn`'s heading, `targetActorPlace`'s jitter (not used),
  `weaponArcsDraw`'s jitter (per view: 3.5). These are fine in the simulation in a fixed order.
- **New randomness** (cloud spawn points, drift changes, strike timing, target picks for unowned clouds): use a
  **port PRNG** in `.port_bss` (`portSd.seed`, an LCG or xorshift), seeded from the first `rand()` after
  `roundStart` or set by the port, in the hash and the snapshot. That keeps the retail stream unchanged except where
  retail functions are called. None exists today.
- **Rules** (check tools): no `rand()` reachable from local-only code (`check_humans.py` check 5); port helpers
  reach `rand()` only through whitelisted retail edges (`check_balance.py` `RAND_VIA`); the latch runs before
  `roundStart`'s `srand` (G0); option word 0 means retail's stream.
- **Per-view ticks** (3.5): the arcs and the flame must be split out of `streaksDraw` before netplay with split
  screens on different machines. That is pre-existing work, but Sudden Death depends on it.
- **The soldier sprite** animates through `animStepLoop`, which advances only while drawn. That is visual only; no
  simulation reads it. But with a private node copy the soldier's step must not be read by simulation code either.

---

## 8. Recommended design

### 8.1 Switches

Define `TM2_PORT_SUDDEN_DEATH` (requires `TM2_PORT_HUMANS` with H5's RULES bit; `#error` otherwise). Storage
`portSuddenDeath` (option word, port-set) and `portSdLive` (latched in `roundStart`, `port_hazards.h` pattern).
Bits, 0 = retail:

| Bit | Name | Meaning |
|---|---|---|
| 0 | `SD_SOLDIERS` | `portSdStart` brings out players back as soldiers |
| 1 | `SD_TIER_MISSILES` | the second soldier life has missiles |
| 2 | `SD_TIER_SPECIAL` | the third has a special (special-as; needs `TM2_PORT_SPECIAL_AS`) |
| 3 | `SD_PLAYER_CLOUDS` | the last tier, or (without bit 0) the first, is a player cloud |
| 4 | `SD_AUTO_CLOUDS` | unowned clouds appear on the schedule |
| 5 | `SD_GROUND_PATCH` | draw the clouds' ground patches |
| 6 | `SD_UNOWNED_HITS_AI` | unowned bolts hurt computer cars (the `carApplyDamage` gate, 3.4) |
| 7 | `SD_SOLDIER_RUNOVER` | cars kill soldiers by running them over |
| 8 | `SD_AI_HUNT` | computer cars target soldiers (Q-A) |

The three kinds:
- **Soldiers, then storms** = bits 0 | 1 | 2 | 3 | 4 (+ 5, 6, 7).
- **Storms by players** = bits 3 | 4 (+ 5, 6).
- **Storms alone** = bit 4 (+ 5, 6).

### 8.2 Storage (`.port_bss`, every variant, appended after `portBalanceCars`; rows in `port_variant_arrays.tsv`)

| Name | Size | Meaning |
|---|---|---|
| `portSuddenDeath`, `portSdLive` | 8 | option word and latch |
| `portSd` | about 64 | frame, start frame, kind, seed, next auto-cloud frame, cloud count |
| `portSdHuman[TM2_MAX_HUMANS]` | 16 x 16 | role (car / soldier / cloud), tier, sprite object, cloud index, run-over cooldown, saved muzzle |
| `portSdClouds[8]` | 8 x 48 | the clouds (4.4) |
| `portSdKillerRole[TM2_MAX_CAR_SLOTS]` | 64 | the killer's role at the killing blow (7.3) |
| the soldier's select-node copy | about 600 | built at load from the level's 514 or 516 node, or from a port asset on levels without pedestrians (Q-C) |

### 8.3 Port API (between frames, every machine, same frame)

| Function | What it does |
|---|---|
| `portSdStart(kind)` | at the limit: each `portHumanDown(h)` with no lives becomes a soldier (`portSdSoldierStart`) or a cloud; starts the auto-cloud schedule |
| `portSdSoldierStart(h, node, tier)` | `portHumanRespawn` + soldier setup (7.1) |
| `portSdCloudStart(h or -1, x, y)` | a cloud, owned or not |
| `portSdRole(h)`, `portSdTier(h)`, `portSdKillerRole(slot)` | for the port's HUD, results and scoring |
| `portSdEnd()` | ends Sudden Death: clouds gone, soldiers down |

### 8.4 Game-function gates (each `#ifdef TM2_PORT_SUDDEN_DEATH` calling a `.port_text` helper)

1. `humanCarTick` (`car_tick.c`): a soldier runs `portSdWalkerTick`; a cloud runs `portSdCloudPlayerTick`
   (no car steps).
2. `humanCarDamageUpdate` (`car_damage_human.c:302`): skip the car death for a soldier or cloud.
3. `carApplyDamage` (`car_damage.c:157`, `:241-254`): the soldier's death (`portSdHit`), the killer role byte, the
   unowned-bolt gate (bit 6).
4. `projectileHitTest` (`weapon_slam_swing.c:63-66`): the soldier's smaller cube; a cloud's record is never hit (its
   object is parked high or skipped).
5. `viewCameraOffsetsInit` (`ai_car_start.c:284-297`): soldier and cloud offsets; `viewChaseCamera`
   (`camera.c:200-215`): pitch bias.
6. `worldUpdate` (`world_update.c`, after the car loop): `portSdFrame()`, which ticks the clock, the clouds, the
   strikes and the run-over test.
7. `carsPhysicsUpdate` (`car_physics.c:119-144`): skip the contact blocks of a soldier or cloud record.
8. `carSoundUpdate`: no engine voice for a soldier or cloud (or a footstep: Q-E).
9. The draw hook after `fxSpritesDraw` (`rt_main.c:157/174/185` and the port's views 2-3): `portSdDrawPatches(rec,
   view)`; a clean draw, no `rand()`.
10. `roundStart` (`scene_init.c:58-63`): `portSdLatch`, before `srand`.

Check tool `tools/check_sudden_death.py`, modelled on `check_hazards.py` and `check_balance.py`:
- gate dominance;
- no `rand()` from helpers except through `RAND_VIA` (`carSpinOut`, `carApplyDamage`, `portHumanRespawn`);
- the latch before `srand`;
- the draw hook reaches no simulation write;
- registered in `check_all.py`.

Proofs as usual: none (retail), sd alone, sd + cars + humans16, all.

---

## 9. Retail bugs met (for `RETAIL_BUGS.md`)

| # | Bug | Status |
|---|---|---|
| 7 (known) | Lightning credits a stale slot; its id is never written, so the duplicate test fails for lightning | CONFIRMED (`weapon_slot_scan.c:62-84`) |
| 15 (known) | Arcs, flame and drops tick once per view in `streaksDraw`, with game `rand()` | CONFIRMED; Sudden Death needs the split for netplay |
| 31 (new candidate) | `carApplyDamage(.., -1)` calls `carFromSlot(-1)`, reading `carSlotIndices[-1]` (retail: `carSlotKinds[14]`); if >= 0 it reads `carSlotKinds[-1]` and may treat "nobody" as a human record (DT doubling, no AI-vs-AI quarter) | code CONFIRMED, effect LIKELY, not run |
| note | AI cars lose no HP from attacker -1 (walls, trains, unowned hits) | CONFIRMED, retail rule (`GAME_EVENTS.md:102`) |

---

## 10. Effort (GUESS, in working sessions like the request-40/41 ones)

| Piece | Effort | Notes |
|---|---|---|
| Storage, latch, bits, API skeleton, check tool, proofs | 1 | pattern exists |
| Clock + `portSdStart` + scoring role byte | 0.5 | |
| Soldier walker tick, physics/contacts off, car hidden, hit box, 1-HP death, run-over | 1.5 | biggest risk: leftover car steps pushing the record |
| Soldier look (private select-node copy, sprite object, walk/hit steps) | 1 | levels without pedestrians need a port asset (Q-C) |
| Soldier camera (offsets + pitch bias) | 0.5 | tuning is the port's |
| Weapon tiers (muzzle overrides, ammo, special-as taser/flame) | 0.5 | special-as already built |
| Clouds: sprites, drift, re-spawn after wipes, player control | 1 | |
| Lightning strikes (arc writes, radius loop, unowned gate) | 0.5 | |
| Ground patches (own quads, ground fit, depth) | 1 | the first port-made primitives in a view hook |
| Split of the `streaksDraw` ticks (needed for netplay) | 1 | shared with FOUR_PLAYERS' plan; may already be the port's task |
| **Total** | **about 8-9** | can be split into 3 PRs: soldiers; clouds and lightning; patches and tick split |

---

## 11. Risks

1. **Leftover car behaviour on the soldier record.** Contacts, the wreck update, sound, damage stages and the
   restart pending flag. Each is a gate; the check tool should list every function that reads the record and that
   runs for a soldier.
2. **Hits on a soldier that bypass `carApplyDamage`.** Direct HP writes (lava, water, falls, ground fires, crush,
   `car_wheels.c:385-468`, `fx_ground_fires.c:159-188`) do not run for a soldier because its wheels and contacts are
   off, except ground fires, which test the car position. Decide: a ground fire kills a soldier (gate it into
   `portSdHit`) or ignores it.
3. **Per-view ticks** (3.5): desync and faster bolts until the split is done.
4. **fx sprite starvation:** with 16 cars exploding, clouds may lose puffs; the re-spawn handles it, the look
   flickers. Cap clouds at 8.
5. **Bullets (40, not raised):** 16 cars plus soldiers firing at cooldown 2 can run out; bullets silently do not
   spawn. Raise to 3 N if it shows.
6. **`csAlloc` never frees:** allocate soldier objects once per match (or use the drop-in free list `portCsTake`).
7. **Camera locks:** a soldier must keep `falling`, `f1D` and `f20` clear (`camera.c:108-111`), or the camera
   freezes.
8. **AI targeting of soldiers** and the homing lock (`weaponFindTargetAhead` scans human car objects): soldiers will
   be locked onto and chased unless gated (bit 8).
9. **The soldier's Dark Tooth doubling** (if the player's car was Dark Tooth): gate or accept.

---

## 12. Questions for the user / port

- **Q-A.** Should computer cars hunt soldiers (and lock missiles onto them), or ignore them? (bit 8)
- **Q-B.** The soldier's hit cube: about ±48 units against bullets (GUESS)? And may cars run soldiers over?
- **Q-C.** The soldier's look on levels without pedestrians (2, 4, 7, 9, 10, 12): ship a port sprite asset (the
  Santa-style TMS + PSXD pair, `SANTA.md` 1.6, way A), or use one pedestrian image everywhere?
- **Q-D.** The lightning flash (`fxSheetStart`) on every strike, only on the strike nearest a local player, or never?
- **Q-E.** Sounds: footsteps for soldiers? A thunder per strike (sound 0x40) positioned at the cloud?
- **Q-F.** Fix retail bug 7 (the Lightning pickup's stale credit) under the same define, as a separate bit?
- **Q-G.** Unowned bolts: hurt computer cars (bit 6, recommended) or humans only (retail's attacker -1)?
- **Q-H.** Who owns the clock: the port's frame hook (recommended, it already owns round end) or the C counter?
- **Q-I.** The `streaksDraw` tick split: is it already planned or done in the port for netplay (FOUR_PLAYERS 4.x)?
  Sudden Death online depends on it.
- **Q-J.** Cloud numbers: max clouds (8 suggested), strike interval, radius, damage per strike (retail lightning:
  20 per bolt, plus spin-out), drift speed, and the schedule for auto clouds.
- **Q-K.** A soldier's death by its own weapon (a missile at its feet): the soldier is hit by its own id? (Retail
  skips the owner's own object in `projectileHitTest`, so no.) Area splash, though, could hit it; accept?
