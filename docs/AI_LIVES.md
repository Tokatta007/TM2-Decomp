# Lives for computer cars: respawning one AI car

Answers item 13 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(Challenge Match and online play: an AI car with lives left comes back
alone, and the "Enemies" count only goes down when its last life is gone).
It is the AI counterpart of `COOP_LIVES.md` section 2. Static analysis
only: the C in `src/semantic`, checked against the retail disassembly
(`bin/SCUS_94306R.EXE`). Nothing was run. Addresses are retail.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code, not traced to the end or not seen running.

Times are in game ticks: `timerGetFrameTicks()` is 3 per frame at 20 fps,
so 60 ticks are one second (420 = 7 s, as in `COOP_LIVES.md`).

AI car record offsets used below (`aiCars`, 80188B5C + i*0x854,
`struct TM2AIRecord` in `include/semantic/cars.h`):

| Offset | Field |
|---|---|
| +0x000 | `f000` "AI on": set by `aiCarStart`; gates the AI brain and movement integration in `aiCarDrive` |
| +0x001 | behaviour state (mood): 0 start, 1 hunt, 2 attack, 3 flee (`aiUpdateMood`); `aiCarStart` sets `rand() % 3` |
| +0x004 / +0x006 | index in `aiCars` / car object slot |
| +0x00C | frames not updated (`aiCarShouldUpdate`); cleared by `aiCarStart` |
| +0x010 / +0x012 | behaviour clock / **behaviour timer** (state length); `aiCarStart` sets +0x012 = (index + 1) * 60 |
| +0x028 | distance to its target |
| +0x030 | car type 0x10..0xF0 |
| +0x034 | in range of its target (< 4320; `active` in cars.h, `in_range` in ai_steps.c); **an AI car takes damage only while it is set**; +0x035..+0x03D the other range flags |
| +0x044 | controls (`TM2CarInput`) |
| +0x06C | state block: +0x06C **dead**, +0x079 frozen, +0x07A **energy shield**, +0x07C / +0x07D wreck spin flags (f10, f11), +0x07F HUD notice, +0x086 burning, +0x088 turbo flame on (f1C), +0x089 held by Mr. Slam (f1D), +0x08B falling, +0x08D in water, +0x08E in lava, +0x08F in fire (level 5), +0x090 sinking, +0x092 small wreck blast done (f26), +0x098 water wake effect pointer |
| +0x0D8 | contact block |
| +0x448 | drive block (`TM2AIDrive`, route following) |
| +0x4FC | stats block: +0x4FF re-place request (f03), +0x500 damage-stage argument, +0x501 **wreck state** (0..2 smoke, 3 wreck model, 4 broken up), +0x503 killer, +0x504 last attacker, +0x516 freeze timer, +0x518 frozen by missile, +0x51E burn time, +0x522 **wreck timer**, +0x536 max HP, +0x538 damage halved, +0x588 retaliate threshold, +0x598 **restart_pending**, +0x59C updated this frame (probe on), +0x5A0 **HP**, +0x5A4 takes damage, +0x5D0 spawn node |
| +0x71C | motion block: position +0x73C, rotation +0x748 |
| +0x808 | weapons block: +0x80C invisible, +0x812 fire chance (profile), +0x83C invisibility timer, +0x840 / +0x844 weapon weight tables (profile), +0x84C burning effect object, +0x850 Mr. Slam record |

AI car i's world object is `aiCarObjects[i]` (80188B38) = `carSlotObjects[aiCarObjectSlots[i]]`
(set by `carBindAiSlot` 8011A5CC; the object's id +0x118 = slot + 50).
Object word +8 is its draw-and-hit flag (`COOP_LIVES.md` 3.1).

## Short answer

- **Death:** `aiCarDamageUpdate` (8011BBD4) is the only place an AI car
  becomes dead: HP <= 0 and not dead gives `carBodyObjectHide(type)`,
  dead +0x06C = 1 and `carDestroyed(car, 0)` (8011BE10). There is no
  counter: `aiCarsRemaining` and the HUD count are recounted from HP > 0
  every frame. The wreck stays on the map, burning and solid, until it is
  blown apart (target far away, a heavy hit, or 22 s); then its object is
  hidden and the car is no longer updated.
- **`restart_pending` is not a safe way in.** It is only read inside
  `aiCarDrive`, which `worldUpdate` does not call for a broken-up wreck
  (wreck state 4), and it runs after the pose is computed. Call
  `aiCarStart(car, car->type, 1)` (800FFEFC) directly, before the AI loop,
  as the attract demo's `demoRespawnAiCars` (8011EFDC) does.
- **`aiCarStart` is a full per-car reset**: position on a start node,
  heading, full HP, state flags, stats, weapons, route, range flags,
  controls. It touches no global except one start-node "taken" byte. Add:
  object word +8 = 1, wreck state 0 with the undamaged model and the body
  object shown, the freeze look and turbo flame off. Do **not** call
  `carRestart`, `demoRespawnAiCars`, `carBindAiSlot`, `carFreezeResetAll`,
  `debrisClear`, `startNodesClearUsed` or any round reset. Section 2.6 is
  the step list.
- **Enemy count:** two functions count AI cars with HP > 0:
  `aiCarsCountRemaining` (8011C248, into `aiCarsRemaining`: round end and
  Paris) and `aiCarsCountAlive` (8011E16C: the "Enemies: N" message).
  Make both also count a car that is dead but has a life left. Nothing
  else needs to change.
- **Radar, lock-on, AI target picks** all skip HP <= 0 already, or (AI
  against humans) never see AI cars. After the respawn the car is back in
  all of them by itself.

## 1. An AI car's death today (CONFIRMED)

### 1.1 The chain

`worldUpdate` (8011A700) runs, per AI car i: `aiCarShouldUpdate(i)` ->
`aiCarUpdate` (`aiCarDrive` and the object copy), its engine sound, its
radar blip, then `aiCarDamageUpdate(i)`. After the loop:
`aiCarsCountRemaining()` and the round-end test.

| Step | Where | What |
|---|---|---|
| 1 | `carApplyDamage` 800E5628 (`car_damage.c`), the killing blow (HP <= 0, not dead yet) | `carBodyObjectHide(type)`; `hudMessagePost(aiCarsCountAlive(), 0)` ("Enemies: N", 0 = "Enemies Toasted!"); killer +0x503 = attacker; wake effect +0x098 off (word +8 = 0, pointer 0). Then the wreck look: a heavy hit (type 1, >= 6) breaks the model into debris (`debrisBreakModel`), wreck state +0x501 = 4 and **object word +8 = 0**; otherwise `carSpinOut(4)`, `carSetDamageStage(type, 3, ...)` (wreck model), wreck state 3, smoke |
| 1' | other HP losses | falls (`aiCarDrive`: HP = -1 below `worldFallDeathZ`), lava, sinking, burning ground, `aiUpdateStuck`: no step 1, the car keeps its model |
| 2 | `aiCarDamageUpdate` 8011BBD4 (`world_collapse_warps.c`) | HP <= 0 and dead = 0: `carBodyObjectHide(type)` (8011BDA0), **dead +0x06C = 1**, `carDestroyed(car, 0)` (8011BDC4). Only writer of the AI dead flag |
| 3 | same, every frame while HP <= 0 | level 3 and `aiCarsRemaining < 3`: `aiRoutePatchEiffelTower` (800F603C; sets the successor count of 7 route nodes to 2, opening the tower links; idempotent) |
| 4 | `carDestroyed` 8011BE10 (`car_destroyed.c`), AI branch | f11 +0x07D = (target bearing +0x020 <= 0): the wreck's spin side. Mr. Slam (0xC0) lets go of a held car (`carSpinOutById`, record off); a car held by Mr. Slam (f1D) is spun out. Sound 26 (types 0x60, 0x30, 0xA0) or 25; **`soundStopSharedLoop(9, -1)`** (every gun loop); Twister stops sound 0x35 |
| 5 | same | **sank** (level 6 below z 240, or level 8 below z -40): `csSetDrawEnable(object, 0)` (word +8 = 0), level 6 splash, wreck timer 0. **Otherwise:** `fxSmokeA24` at ground level, `carSpinOut(4)`, `fxExplosion48` + `fxExplosion24`, HUD notice +0x07F = 1, wreck timer +0x522 = 120 (Dark Tooth 360, plus `carApplyDamage(own id, 15, 0, 2, 0)`) |
| 6 | same, both paths | wake effect off again; f10 +0x07C = 1; **frozen +0x079 = 1, freeze timer +0x516 = 540**; mounts +0x4FE = 0; `carShakeStart(car, 0, 2, 0, 0)` |
| 7 | `aiCarsCountRemaining` 8011C248 (`scene_init.c`), same frame | `aiCarsRemaining` = number of AI cars with HP > 0. Level 8 boss: when 0 and `aiCars[0]` is Dark Tooth, stage 2 instead (keeps 1) |
| 8 | `worldUpdate` 8011B5AC..8011B62C | `aiCarsRemaining <= 0`, mode != 4, `humanCars[0]` not dead, round not won: round won, `rtScheduleEvent(3, 420)` |

No pickup is dropped (AI cars hold none: `weaponTick` gives them every
weapon, energy 1000 and turbo 990 each tick). No tally, score or kill
counter is changed (`GAME_EVENTS.md`: the killer byte has no reader).
`aiCarTotal`, the slot tables and `aiCarObjects` are not changed.

### 1.2 The wreck afterwards (CONFIRMED, `carWreckUpdate` 800E75F4 via `aiMovementUpdate`)

While dead and wreck state != 4 the car is still updated (when
`aiCarShouldUpdate` lets it through) and `aiMovementUpdate` calls
`carWreckUpdate(car, 0)` each frame:

| Phase | What |
|---|---|
| always | +0x034 = target distance < 4320 (recomputed here, since the AI brain is off); sets burning +0x086 and a flame effect (`fxFlame10`, weapons +0x84C, life 500) unless in the air or in water |
| wreck timer > 0 (2 s) | spins and slows; random explosions and smoke |
| timer < -960 | one small blast (f26 +0x092) |
| timer <= 0 and target >= 4320 away, or timer < -1200 (22 s after death) | **blown apart**: +0x034 = 1, `carApplyDamage(object id, 15, 0, 2, 0)` -> debris, wreck state 4, **object word +8 = 0**; explosion, smoke; flame off, burning 0 |

The frozen flag set at the death counts down in `aiCarDrive` (540 ticks)
and is cleared; when "frozen by missile" +0x518 was set it also thaws the
freeze look (`carFreezeSetState(type, 3)`).

### 1.3 What stops a dead AI car

**From driving** (CONFIRMED):

| Gate | Where | Effect |
|---|---|---|
| dead +0x06C | `aiCarDrive` 800E7EDC: `f000 && !dead` | no `aiUpdateTargetRange`, goal and target bearing, route choice, `aiUpdateMood`, `aiWeaponsUpdate` (no firing); target distance = 0 |
| dead | `aiMovementUpdate`: steering block needs `+0x034 && !dead` | no `aiInputFinalize` / `carThrottleSteer`: the wreck coasts |
| frozen +0x079 | movement and weapons | no steering, firing, energy |
| wreck state 4 | `aiCarShouldUpdate` 8011B7D8 returns 0 | `aiCarDrive` is not called at all, +0x59C = 0 (no ground probe or contacts in `carsPhysicsUpdate`), +0x00C counts up |
| `aiCarsCountRemaining` | | the car is no longer counted (HP <= 0) |

`aiCarShouldUpdate` (outside attract mode) also skips a car whose +0x00C
is not 0 and range flag +0x039 is clear when the target is > 4800 away or
the bearing +0x024 is over 1024; a dead car has target distance 0, so this
part rarely applies to it.

**From being hit** (CONFIRMED):

| Hit | Rule for an AI car | Dead wreck (state 3) | Broken up (state 4) |
|---|---|---|---|
| car-car collision, world hit pass `hdpPassRun` 8014FAB0 | object (+8 & +0xC) != 0 | solid | not solid (word +8 = 0) |
| projectiles, mines, freeze, napalm (`projectileHitTest` 800D6FD8) | skipped only when word +8 = 0 **and** dead | hit (shots stop on the wreck) | skipped |
| damage itself (`carApplyDamage`) | returns unless +0x034 (in range of its target); HP only drops when attacker >= 0 | heavy hits (type 1, >= 6) break it up | |
| `areaDamage` 8010B5C4 | wreck state != 4 | hit and spun | skipped |
| lightning `weaponLightningScan`, taser `weaponTaserTarget` (weapon_slot_scan.c, weapon_slot_target.c) | HP > 0 | skipped | skipped |
| ricochet `weaponRicochetUpdate` | HP > 0 | skipped | skipped |
| Axel / special push (`weaponFireSpecial` 0xB0) | dead = 0 | skipped | skipped |
| Mr. Slam grab `weaponSlamFindTarget` 800F5498, tornado `weaponTornadoUpdate`, ground fires `groundFiresUpdate` | distance only | grabbed, pushed, set burning | same (fire drain needs +0x034 and HP >= 6) |

**From being targeted** (CONFIRMED):

| Who | Rule |
|---|---|
| AI target picks (`aiGetTargetCar` 8011CF0C) | `aiCarTargets[i]` (801879FC) is -1 outside attract mode, so every AI chases the nearest **human**. AI cars never target AI cars in a match. When set (attract mode, or a port AI-vs-AI mode) it has no HP check |
| human missile lock (`weaponFindTargetAhead` 80109F94, `weaponFindTargetBehind` 8010A2AC) | AI loop skips HP <= 0 |
| homing leap (`weaponHomingLeapInit`, weapon_leap_homing_init.cpp) | skips HP <= 0 |
| HUD target panel (`hudDrawTargetPanel` 800FD0D0 via `carNearestToHuman` 800EC8A0) | one player or mode 2: `aiCarNearestAlive` 8011C98C (HP > 0); the name is printed only for HP > 0 |
| radar (`worldUpdate`, `hudRadarPlotCar` 800FC928) | AI blips only for HP > 0, for `humanCars[0]` and, in mode 2, `humanCars[1]` |
| AI pack logic (`worldUpdate`) | skips wreck state 4; the "closer" count skips dead cars. A dead wreck in state 3 can still be picked to change behaviour state (harmless) |
| engine and gun sounds | `worldUpdate` stops the gun loop of a dead or out-of-range car, and its engine sound when out of range |

## 2. Bringing one AI car back

### 2.1 `restart_pending` (CONFIRMED)

`aiCarDrive` ends with: if stats +0x598 (`restart_pending`) is set,
`aiCarStart(car, car->type, 1)` and clear it (800E85C4). Nothing in the
EXE sets it for AI cars (the human copy is in `humanCarTick`). Three
problems for a respawn:
- `aiCarDrive` is only reached through `aiCarShouldUpdate`. A wreck in
  state 4 (most wrecks after a few seconds) is never updated, so the
  request is never read;
- it runs after `carBodyPoseUpdate`, so that frame `aiCarUpdate` copies
  the new position with the old body matrix into the object (one frame;
  LIKELY harmless);
- the object (word +8), the body object and the damage model are not
  touched.

So call `aiCarStart` directly. Its three callers in retail are
`aiCarDrive` (above), `carBindAiSlot` (8011A694, match start) and
`demoRespawnAiCars` (8011F078, the attract demo's respawn of every AI car,
called from `worldUpdate` before the AI loop). The last is the model.

### 2.2 What `aiCarStart(car, type, 1)` does (CONFIRMED, `ai_car_start.c`)

| Step | Effect |
|---|---|
| type +0x030 = type; stats +0x4FC = 0 | the "takes damage" word will be set to 1 |
| `aiCarReset` 80100034 (`ai_route_setup.c`) | motion: position 0, speeds, rotations, spins, body angles 0; rotation and body matrices = identity; drive f448 = 0; **`carStateFlagsClear`** (state block, below); `carSpinStop`; +0x59C = 1; `carPushClear` (contact push); difficulty speed factor; **`carStatsLoad`** (below); top speed; **`carBuildCollisionBox(car, 0)`**; step height; range distances 320 / 160 / 600 / 1000 / 2000; **HP = max HP** |
| `weaponBlockInit(car, weapons, 0)` 800FDCAC | weapons: invisible and timer 0, rear fire, drop toggle, cooldowns, gun side, ammo (specials 2), selected 0, turbo 200, energy 1000, capacity and special recharge for the type, Mr. Slam record re-armed (`weaponSlamStateInit`, reuses its record); **burning effect** (+0x84C) and second effect: `active` = 0 (fire out). The AI profile (weights +0x840/+0x844, fire chance +0x812) is not touched |
| cooldowns 50 / 50 | no shot in the first frames |
| +0x001 = `rand() % 3`, +0x012 = (index + 1) * 60, +0x00C = 0 | new behaviour state; the first change after index + 1 seconds |
| `aiRangeFlagsClear(+0x034)` | +0x034..+0x03D = 0 (in range, near, r1..r4, ...) |
| `aiRouteStateReset(drive)` 80100318 | route nodes, group, away / closer counters, flags, back-up time ... = 0 |
| spawn node +0x5D0 = `startNodePickRandom(startNodeCount)` | takes a free start entry and marks it taken; when all are taken, a random entry, unmarked (`SIXTEEN_CARS.md` 5.2) |
| `aiPlaceOnRouteNode` 800F6518 | walks the route state onto the node (`aiChooseNextNode`, `aiUpdateStuck`, `aiSetSegmentSpeed`, `aiAdvanceRouteNode`, which also clears the turn total +0x47E); target node; **position = node x / y, height = the floor of the node's section**; speed 0; heading = the segment's; wheels reset and set to the node's section and height; `carBodyAttitudeUpdate` |
| `aiClearControls(input)` | controls off |
| `carShakeStart(car, 2, 2, 0, 0)`, +0x000 = 1 | AI on |

`carStateFlagsClear` (800FDC20) clears: dead, frozen, energy shield,
burning, f10, f11, HUD notice, crush flags, f1C (turbo flame on), f1D
(held), falling, in water, sinking, in lava, f26, f27 and others. It does
**not** clear +0x08F (in fire), +0x098 (wake pointer) or the slide bytes.

`carStatsLoad` (800FE15C) clears: freeze timer +0x516, burn time +0x51E,
+0x512, +0x524, damage halving +0x538, re-place request +0x4FF,
restart_pending +0x598, regen clock, mounts; killer +0x503 and last
attacker +0x504 = -1; **wreck state +0x501 = 4** (retail stores 4 at
800FE19C); takes damage +0x5A4 = 1 (since +0x4FC = 0); then the type's
tuning and the retaliate threshold +0x588 (`max HP * pct / 100`).

It is a full per-car reset. The only global it writes is one byte of
`startNodeList`'s taken flags (801B2B46). CONFIRMED.

### 2.3 What it leaves (and the fix)

| Left as it was | Effect | Fix | Conf. |
|---|---|---|---|
| object word +8 (0 after a break-up or a sink) | invisible and not solid | `aiCarObjects[i]->visible = 1` (`demoRespawnAiCars` does this) or `csSetDrawEnable(obj, 1)` (`carBindAiSlot`) | CONFIRMED |
| wreck state +0x501 = 4 (from `carStatsLoad`) | `aiCarShouldUpdate` skips the car until `aiCarDamageUpdate` runs (same frame, after the AI loop): HP > 40 % and state != 0 gives `carSetDamageStage(type, 0, +0x500)`, `carBodyObjectShow(type)`, state 0. So the car is fixed one update late, and its first frame is not driven | either leave it (one frame), or do it at once: `carSetDamageStage(type, 0, aiCars[i].stats.smoke_arg)`, `carBodyObjectShow(type)`, +0x501 = 0, then the same frame's AI loop updates the car and copies its pose to the object | CONFIRMED (code), LIKELY (one-frame look) |
| per-type body object `carBodyObjects[type >> 4]` (hidden at the death) | | shown by the line above | CONFIRMED |
| freeze look `carFreezeObjects[type >> 4]` and +0x518 "frozen by missile" | `carStateFlagsClear` clears frozen, so the thaw in `aiCarDrive` never runs: a car that died frozen by a missile keeps the ice look | `carFreezeSetState(type, 0)` (state 0 = what `carFreezeResetAll` gives every car at a round start) and +0x518 = 0 | LIKELY |
| turbo flame object (`objRegistrySetStep(type + 6, 0, 1)`, `carTurboObjects`) | `aiInputFinalize` turns it off only when +0x088 was 1; `carStateFlagsClear` clears +0x088, so a flame that was on at the death stays on | `objRegistrySetStep(type + 6, 0, 0)` (what `aiInputFinalize` calls) | LIKELY |
| +0x010 behaviour clock, +0x00E | the first `aiUpdateMood` may re-roll at once | +0x010 = 0, +0x00E = 0 | CONFIRMED (not reset), LIKELY (effect) |
| +0x08F in fire, object light env (level 5) | cleared by `carGroundEffects` on the first frame off lava | none needed | CONFIRMED |
| wreck timer +0x522 | read only while dead (and for the level 8 boss) | none needed | CONFIRMED |
| AI profile (weights, fire chance +0x812) | correct as it is | do not call `aiCarSetupProfile` (it is called once by `carBindAiSlot`) | CONFIRMED |

### 2.4 What to call, and what not

| Call | Per car or global | Use it? |
|---|---|---|
| `aiCarStart(&aiCars[i], aiCars[i].type, 1)` 800FFEFC | per car | **yes** |
| `aiCarObjects[i]->visible = 1` | per car | yes |
| `carSetDamageStage(type, 0, arg)` 80124718, `carBodyObjectShow(type)` 80114E10 | per car type | yes (2.3) |
| `carFreezeSetState(type, 0)` 80124C00, `objRegistrySetStep(type + 6, 0, 0)` | per car type | yes (2.3) |
| `aiPlaceOnRouteNode(car)` 800F6518 after setting +0x5D0 | per car | optional, to choose the node (2.5). Retail calls it alone at run time too (`aiCarDrive` re-place request, `aiReplanRoute`) |
| restart_pending +0x598 = 1 | per car | no (2.1) |
| `carRestart` 800FD550 | human records, and **global** wipes (`weaponDropsReset`, `weaponsRoundReset`, `projectilesReset`, `fxSpritesReset`, `hudMessageReset`) | **no** |
| `demoRespawnAiCars` 8011EFDC | every AI car, `carFreezeResetAll`, `debrisClear`, `startNodesClearUsed`, attract targets and spawn tables, ends the demo after 5 rounds | **no** |
| `carBindAiSlot` 8011A5CC, `carBindAiByType` 8011A54C | rebinds the slot tables (and can raise `aiCarTotal`) | **no** |
| `carFreezeResetAll`, `debrisClear`, `startNodesClearUsed`, `aiCarsSetActive` (every AI car), `roundStart`, `humanCarsRestart`, `arenaSetup` | global | **no** |
| `aiCarSetupProfile` 800D3C7C | per car | not needed |

### 2.5 Where it appears: the start node (CONFIRMED)

`aiCarStart` uses `startNodePickRandom(startNodeCount)` (800F636C). The
list (`startNodeList` 801B2B28: 15 node ids, taken bytes at 801B2B46,
count at 801B2B55) has 8 to 15 entries, and the match start has already
taken one per car (AI cars first, then humans; AI cars use the full count
on Paris too). The taken bytes are cleared only by `arenaSetup`. So a
respawn usually finds every entry taken and lands on a random node,
possibly next to a live car. `SIXTEEN_CARS.md` 5.

To choose a node clear of the others (as `COOP_LIVES.md` 2.3):
- for each list entry k, its node n = `startNodeList` id k; node position
  `aiRouteNodes[n]` (801B1DE8, 24 bytes: x s32 +0, y s32 +4, height s16 +8);
- take the entry whose distance (x / y) to every live car (humans
  +0x6FC, AI +0x73C, HP > 0) is largest;
- either mark every other entry taken (801B2B46 + k = 1), call
  `aiCarStart`, then restore the bytes; or call `aiCarStart`, then set
  spawn node +0x5D0 = the chosen node and call `aiPlaceOnRouteNode(car)`
  again (it only reads +0x5D0 and the route table).

The position comes from the node's x / y and the floor height of its
section (`worldSectionFloorHeights[node.route].base`), as at the match
start. Nodes of a collapsed Antarctica section are removed from the list
by `aiRouteNodeRemove` (`startNodesRemove`), so they are not picked.

### 2.6 Step list: one AI car back with full health at a start point

At the death (hook in `aiCarDamageUpdate` 8011BBD4, after `carDestroyed`
at 8011BDC4, inside the `!dead` block):
1. if the car has a life left: take one, and start a port timer for car i
   (for example 420 ticks; the retail wreck burns 120 ticks before it can
   be blown apart). Leave the retail death as it is (wreck, explosion,
   sound).
2. keep counting it as an enemy (section 3).

When the timer ends (in the port's call before the AI loop of
`worldUpdate`, where `demoRespawnAiCars` runs, so the car is updated in
the same frame):
1. note the type `t = aiCars[i].type`;
2. if +0x518 (frozen by missile) is set: `carFreezeSetState(t, 0)`;
   clear +0x518. If +0x088 (turbo flame) is set:
   `objRegistrySetStep(t + 6, 0, 0)`;
3. choose a node (2.5) and call `aiCarStart(&aiCars[i], t, 1)`;
4. `carSetDamageStage(t, 0, aiCars[i].stats.smoke_arg)`,
   `carBodyObjectShow(t)`, wreck state +0x501 = 0;
5. +0x010 = 0, +0x00E = 0;
6. `aiCarObjects[i]->visible = 1`;
7. optional invulnerability: energy shield +0x07A = 1, cleared by the port
   after N frames. Nothing in retail clears an AI car's +0x07A except
   `carStateFlagsClear` (`weaponShieldsUpdate` clears only human records),
   and `carApplyDamage` returns before any damage while it is set. An AI
   energy-shield attack (103) is refused while it is set but still costs
   energy. CONFIRMED;
8. optional: a sound at the spot, as `COOP_LIVES.md` 5.2
   (`soundLocateForView0(pos)` / `soundLocateForView(pos, -1)` then
   `soundPlay(24, ...)`).

The same frame, `aiCarShouldUpdate(i)` returns 1 (+0x00C = 0, state 0),
`aiCarDrive` runs the AI from its new state, `aiCarUpdate` copies the pose
into the object, the radar blip comes back (HP > 0), and
`aiCarsCountRemaining` counts it. LIKELY (not run).

## 3. Enemy count and round end

### 3.1 Writers and readers (CONFIRMED, disassembly scan)

`aiCarsRemaining` (80181020, gp + 0x64C) is accessed at exactly three
places, plus its getter:

| VA | Function | Access |
|---|---|---|
| 8011C258..8011C2F8 | `aiCarsCountRemaining` | **only writer**: 0, then +1 per AI car with HP > 0; level 8 boss: set to 1 while Dark Tooth (`aiCars[0]`, 0xF0) waits for stage 2 |
| 8011B5AC | `worldUpdate` round-end test | `<= 0` (with mode != 4, `humanCars[0]` not dead, round not won) -> round won, event 3 after 420 (210 after the boss) |
| 8011BDE0 | `aiCarDamageUpdate` | level 3: `< 3` while a dead car exists -> `aiRoutePatchEiffelTower` (the Paris event) |
| 8011C23C | `aiCarsGetRemaining` | getter; **no caller** (no `jal`) |

The HUD does not read `aiCarsRemaining`. It uses `aiCarsCountAlive`
(8011E16C), the same HP > 0 count, called from:

| VA | Caller | Use |
|---|---|---|
| 800E5BAC | `carApplyDamage` (killing blow) | `hudMessagePost(count, 0)`: "Enemies: N" now |
| 800FBB74, 800FBB98 | `hudMessageReset` | message = count (round start, `carRestart`) |
| 800FBC00 | `hudMessagePost` | the queued message |
| 800FBC4C, 800FBCC8, 800FBCE4 | `hudMessageUpdate` (each HUD draw) | 0 -> "Enemies Toasted!"; else the default message every 100 frames |

The message index is `hudMessageStrs[count]` (8016286C: 0 "Enemies
Toasted!", 1..12 "Enemies: N"). Neither count is cached: both are
recomputed from HP every time.

### 3.2 Holding a death back

Change the two counting functions, not their readers:
- `aiCarsCountRemaining` 8011C248: count car i when HP > 0 **or** it is
  dead with a life left (respawn pending). Keep the level 8 boss part.
- `aiCarsCountAlive` 8011E16C: the same rule, so "Enemies: N" stays
  equal to the round-end count.

Then:
- the round-end test is right as it is: the round is won only when no car
  has HP > 0 and none has a life left;
- the Paris event opens the tower links when fewer than 3 cars are left
  counting lives, and only while some car is dead (also during a pending
  respawn; it is idempotent);
- the killing-blow message shows the unchanged N while a life is left.
  The port may post its own message there instead;
- radar, lock-on, target panel and the AI's own HP > 0 tests keep using HP,
  so a car waiting to respawn is not targeted. CONFIRMED (code), the rule
  is the port's.

Hook point for taking the life: `aiCarDamageUpdate` 8011BBD4, the
`!dead` block (8011BD84..8011BDC8). It runs once per death; every AI death
passes through it (1.1). Nothing has to be skipped there: let
`carBodyObjectHide`, dead = 1 and `carDestroyed` run, and keep the car
"dead with a life" in the port's table until the respawn.

Exclude Dark Tooth (0xF0) from lives on level 8 (his two-stage boss fight
goes through `aiCarsCountRemaining` and `aiBossDefeatedFlag`).

## 4. The wreck and the radar

| Thing | During the wait | At the respawn | Conf. |
|---|---|---|---|
| world object `aiCarObjects[i]` | visible as a burning wreck (state 3) or hidden (state 4, or sank: word +8 = 0) | word +8 = 1; `aiCarUpdate` moves it to the new pose the same frame | CONFIRMED |
| to remove the wreck early | `csSetDrawEnable(aiCarObjects[i], 0)` hides it and takes it out of the hit pass; or blow it apart as retail does: `carApplyDamage(aiCarObjects[i]->id, 15, 0, 2, 0)` with +0x034 = 1 (debris, explosion, word +8 = 0, state 4) | | CONFIRMED |
| damage model `carDamageStageObjects[type >> 4]` | stage 3 (wreck model) | `carSetDamageStage(type, 0, ...)` | CONFIRMED |
| per-type body object `carBodyObjects[type >> 4]` | removed from the world list (`carBodyObjectHide`) | `carBodyObjectShow(type)` | CONFIRMED |
| burning flame on the wreck (`fxFlame10`, +0x84C) | burning | `weaponBlockInit` sets its `active` 0; burning flag cleared | CONFIRMED |
| debris from a break-up | flies and settles in the shared debris slots | leave it (`debrisClear` is global) | CONFIRMED |
| wake effect +0x098 | turned off at the death | | CONFIRMED |
| radar blip | none (HP <= 0) | back by itself (HP > 0), in `worldUpdate`'s AI loop | CONFIRMED |

Per-type objects are keyed by `type >> 4`, so two cars of the same type
share them (`SIXTEEN_CARS.md` 1.3). That is already true in retail.

## 5. Per-car state a respawn should reset

| State | Where | Reset by `aiCarStart`? | Conf. |
|---|---|---|---|
| frozen +0x079, freeze timer +0x516 | state, stats | yes | CONFIRMED |
| frozen by missile +0x518, freeze look | stats, `carFreezeObjects` | **no**: `carFreezeSetState(type, 0)`, +0x518 = 0 | CONFIRMED / LIKELY |
| burning +0x086, burn time +0x51E, flame effect +0x84C | | yes (flag, time, effect `active` = 0) | CONFIRMED |
| in fire +0x08F (level 5), light env | | no; self-clearing on the first frame | CONFIRMED |
| invisible +0x80C, timer +0x83C | weapons | yes. AI invisibility has no visual (`weaponInvisibilityUpdate` handles humans only) | CONFIRMED |
| energy shield +0x07A | state | yes (cleared). A shield bubble an AI raised (`weaponShieldPool`) keeps following its object until its life ends, then clears a **human** record's flag (the AI owner is never stored) | CONFIRMED (code), LIKELY (effect) |
| damage halving +0x538 | stats | yes | CONFIRMED |
| behaviour state +0x001, timer +0x012 | | yes (`rand() % 3`, (index + 1) * 60) | CONFIRMED |
| behaviour clock +0x010, +0x00E | | **no**: set 0 | CONFIRMED |
| retaliate threshold +0x588, hunt chance +0x580 | stats | +0x588 yes; +0x580 is recomputed by `aiUpdateMood` | CONFIRMED |
| range flags +0x034..+0x03D, distances | | yes | CONFIRMED |
| drive block +0x448 (`TM2AIDrive`) | | yes: `aiRouteStateReset`, then `aiPlaceOnRouteNode` rebuilds nodes, segment, headings, lane, corner, turn total | CONFIRMED |
| killer +0x503, last attacker +0x504 | stats | yes (-1) | CONFIRMED |
| re-place request +0x4FF, restart_pending +0x598 | stats | yes | CONFIRMED |
| contact push, wheels, motion, matrices, collision box | | yes | CONFIRMED |
| turbo flame +0x088 and object | | flag yes, object **no**: `objRegistrySetStep(type + 6, 0, 0)` | LIKELY |
| wreck state +0x501 | | set to 4 (see 2.3) | CONFIRMED |
| Mr. Slam record +0x850 | | re-armed (`weaponSlamStateInit`) | CONFIRMED |
| mines and remote bombs it dropped (`weaponDrops`, 25 records, owner = its object id) | global | not touched at a death or a respawn; they stay armed and keep their owner. Nothing to reset (`weaponDropsReset` is global) | CONFIRMED |
| homing projectiles aimed at it (16 records, `target` +0x38 = a pointer to the target's position, `victim` +0x3C) | global | no; a missile in flight turns to the respawned car's new position | LIKELY |
| homing leap target (`homingLeapState.target_position`), taser victim (`weaponTaserVictim`) | global | no; both point into the record. Short-lived | LIKELY |
| other cars' targets on it | `aiCarTargets[k] == i` | not used outside attract mode (every AI targets the nearest human). A port AI-vs-AI mode should retarget at the death: `aiGetTargetCar` has no HP check | CONFIRMED |
| watched car of view 7 (`viewWatchedAiCar`) | | unchanged; the view follows the record, so it jumps to the new spot | LIKELY |

## 6. Notes

- The AI car is dead only through `aiCarDamageUpdate`, and that runs for
  every AI car every frame, whether it was updated or not. So the death
  hook in 3.2 sees every death, including falls and sinking. CONFIRMED.
- AI cars take HP loss only from attackers >= 0 and while +0x034 is set
  (target within 4320). Right after a respawn far from the humans,
  +0x034 is 0 until the first `aiUpdateTargetRange`; a far-away car is
  not hurt by stray hits until then. CONFIRMED.
- More than one AI car may wait at once; each needs its own timer and its
  own start entry. With 16 cars, the start list needs the larger table of
  `SIXTEEN_CARS.md` 5.3.
- Humans and AI share nothing per car in this path, so the human respawn
  of `COOP_LIVES.md` 2 and this one can run in the same frame.
