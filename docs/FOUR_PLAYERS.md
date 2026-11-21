# Three and four players: players 3 and 4 in computer-car records

Answers item 22 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(3-4 players in split screen), part A: sections 1 to 3 (input, what else
treats an AI record differently, what is bounded by two players). Sections 4
and 5 (views and HUD) follow below. The port's plan: players 3 and 4 drive
**computer-car records** (`aiCars`), with a controller in place of the AI's
driving. Static analysis only: the C in `src/semantic`, checked against the
retail disassembly (`bin/SCUS_94306R.EXE`) at the places named. Nothing was
run. Addresses are retail.

Builds on `ANALOG_STEERING.md` (the pad and `carThrottleSteer`),
`AI_VS_AI.md` (the human-only tests), `AI_LIVES.md` (one AI car's death and
respawn), `COOP_LIVES.md` (lives per player, round end), `SIXTEEN_CARS.md`
section 1 (fixed loops) and `GAME_EVENTS.md` (killer bytes, pickups,
energy attacks).

Tags: **CONFIRMED** = seen in code (the C is byte-matched to retail, and
where an instruction VA is given it was read in the disassembly).
**LIKELY** = follows from the code, not seen running.

Words used below: a **pad record** is an `aiCars[i]` entry that the port
gives to player 3 or 4. **Layout** is which record offsets a function uses
(the `human` / `side` argument of the shared helpers). **Rules** is what the
function does differently for a player (sounds, limits, refills).

Blocks of the two records (`include/semantic/cars.h`):

| Block | Human (`humanCars` 80187B10 + i*0x814) | AI (`aiCars` 80188B5C + i*0x854) |
|---|---|---|
| camera and player fields | +0x000..+0x08F (player index +0x000, camera +0x00C..) | none (AI fields +0x000..+0x043 instead) |
| type | +0x090 | +0x030 |
| state (`TM2CarState`) | +0x09C | +0x06C |
| input (`TM2CarInput`, 0x28 bytes) | +0x0D0 | +0x044 |
| stats (`TM2CarStatsBlock`) | +0x14C (HP +0x1F0) | +0x4FC (HP +0x5A0) |
| contact | +0x234 | +0x0D8 |
| route following (`TM2AIDrive`) | none | +0x448 |
| wheels | +0x5A4 | +0x5E4 |
| motion (`TM2CarMotion`) | +0x6DC (position +0x6FC) | +0x71C (position +0x73C) |
| weapons (`TM2CarWeapons`) | +0x7C8 | +0x808 |

## Short answer

- **The AI input block is the same struct as the human one** (`TM2CarInput`,
  0x28 bytes): human +0x0D0, AI +0x044. Every byte a pad writes on a human
  car has the same meaning at the same offset in the AI block, including
  the four bytes the move-code reader keeps (+0x20..+0x27). CONFIRMED.
- **Who fills it on an AI car:** `aiMovementUpdate` (800E8F38) clears it
  (`aiClearControls` 800EC938), the steering functions (`aiFreeSteer`,
  `aiSteerToGoal`, `aiFollowRoute`, `sub_800E9E20`, `aiUpdateStop`) set
  steer, slide, throttle, brake and turbo, `aiInputFinalize` (800EC180)
  masks them and switches the turbo flame, and `aiWeaponFireDecide`
  (80103CA0, inside `weaponTick`) sets fire, guns and the energy request.
  `aiPickWeapon` chooses the weapon and the energy attack. The request's
  800EC384 is inside `aiInputFinalize`: the `jal objRegistrySetStep` that
  turns the turbo flame off. CONFIRMED.
- **Cleanest hook:** in the AI loop of `worldUpdate`, for a pad record call a
  port function in place of `aiCarDrive` (through `aiCarUpdate` 8011C4A0).
  It is `aiCarDrive` without the AI brain and with `humanCarTick`'s order:
  a pad reader (an AI-layout copy of `humanCarReadInput` without the view
  keys), `carThrottleSteer`, the slide code, the shared physics with
  `human = 0`, and a weapon tick that does not refill. Section 1.6.
  LIKELY (the design); CONFIRMED (every piece it calls).
- **The AI path is not just "no pad".** With `human = 0`, `weaponTick`
  refills every weapon, the turbo meter (990) and energy (1000) each frame,
  `weaponFireSelected` fires without ammo checks and at the AI's cooldown,
  `carThrottleSteer` has no slope term and no turbo sound, `carStatsDerive`
  halves the launch speed and multiplies the steering lock by 10, and the
  AI speed factor (75/120/140 %) is applied. The port must split the
  `human` argument of these functions into layout and rules. Section 1.7.
  CONFIRMED.
- **Many other places treat a pad record as a computer car** (section 2).
  The ones that change the game most: computer cars never target it
  (they chase the nearest **human**), its own homing shots steer at player
  1, hits between it and a computer car are divided by 4, it takes no
  damage unless its +0x034 range flag is set, the update throttle and the
  pack director act on it, it counts as an enemy for the round end and the
  "Enemies: N" text, its death schedules no event and a round restart does
  not reset it, its energy shield never ends (and ends player 1's), its
  invisibility never ends, and its shots cannot break breakables (object id
  >= 50). CONFIRMED.
- **Two-player bounds** (section 3): the settings struct (2 human cars, 2
  control layouts, `lives[4]` with [2] clobbered by a clear), the runner
  (lives[0] only, two wins tallies), the round-end test (player 1 only), the
  pause owner (pads 0 and 1), `padCheckAction` (ports 0, 1 and 2 = either),
  the per-player sound loops (P1/P2 flags), the radar pair, and the
  2-entry HUD, view and camera arrays. None of these is read for a pad
  record; each needs a port path. CONFIRMED.

## 1. Input: the AI's own input block

### 1.1 The fields

`struct TM2CarInput` (cars.h), block offset n = human +0x0D0 + n = AI
+0x044 + n. "Human writer" is `humanCarReadInput` (800DE5F8, car_input.c)
unless noted; actions are those of `ANALOG_STEERING.md` 1 and
`OPTIONS_CONTROLS.md`. "AI writer" is the function that sets it on a
computer car (`TM2AI` names in `ai_steps.c` in brackets). CONFIRMED.

| n | Field | Human (+0x0D0 + n) | Human writer | AI (+0x044 + n) | AI writer | Read by |
|---|---|---|---|---|---|---|
| 0x00 | `slide_inc` | +0x0D0 | handbrake (0x1C) with Right | +0x044 (`f044`) | `aiSteerToGoal`, `aiFreeSteer` | `carThrottleSteer`, `carGripSlowdown` |
| 0x01 | `slide_dec` | +0x0D1 | handbrake with Left | +0x045 | same | same |
| 0x02 | `slide` | +0x0D2 | `slide_inc` or `slide_dec` | +0x046 | `aiFreeSteer` | `carThrottleSteer` (limit, step) |
| 0x03 | `turbo` | +0x0D3 | action 0x1D, not frozen, not held by Mr. Slam | +0x047 | `aiFollowRoute`; masked by `aiInputFinalize` | `carThrottleSteer` (boost, fuel -1 a frame), turbo flame |
| 0x04 / 0x05 | `f04` / `view_modifier` | +0x0D4 / +0x0D5 | actions 8 / 9 (Start / Select as view modifiers) | +0x048 / +0x049 | cleared only | `humanCarReadInput` (reverse and drive are cleared while held), `carViewHotkeys` |
| 0x06 | `power_slide` | +0x0D6 | `brake` while `abs(speed) > accel_launch` | +0x04A | not written (stays 0) | `carThrottleSteer`, `humanCarSlideStart` |
| 0x07, 0x09..0x0B, 0x16 | view hotkeys | +0x0D7, +0x0D9..+0x0DB, +0x0E6 | actions 0x31, 0x39..0x3C with 9 held | | not written | `carViewHotkeys` |
| 0x0E | `reverse` | +0x0DE | action 0x1B, not with 8 / 9 | +0x052 (`f052`) | `aiInputFinalize` masks; steering sets | `carThrottleSteer` |
| 0x0F | `steer_dec` | +0x0DF | action 0x17 (Left) | +0x053 (`f053`) | steering functions; `aiInputFinalize` clears it while `hold_dist` (+0x077) | `carThrottleSteer`, `carBodyRealign`, `carRicochetOffObject` |
| 0x10 | `steer_inc` | +0x0E0 | action 0x16 (Right) | +0x054 (`f054`) | same | same |
| 0x11 | `forward` | +0x0E1 | action 0x18 | +0x055 | not written | `carThrottleSteer` (brake halves while held) |
| 0x12 | `throttle` | +0x0E2 | `drive` unless frozen, rolling (+0x0AE), all wheels in the air, held | +0x056 (`f056`) | steering functions; `aiInputFinalize` masks frozen, rolling, held | `carThrottleSteer` |
| 0x13 | `brake` | +0x0E3 | action 0x19, or forward + reverse, or reverse while moving forward | +0x057 (`f057`) | steering functions; `aiInputFinalize` sets it while frozen | `carThrottleSteer` |
| 0x14 | `drive` | +0x0E4 | forward, action 0x1A or reverse | +0x058 | not written | `carThrottleSteer` (slope term, human only) |
| 0x17 / 0x18 | `cycle_next` / `cycle_prev` | +0x0E7 / +0x0E8 | actions 0x21 / 0x20 | +0x05B / +0x05C | cleared only | `weaponHumanCycleInput` 80103C08 (human only) |
| 0x19 | `cycle_armed` | +0x0E9 | `weaponHumanCycleInput` (edge re-arm) | +0x05D | `aiClearControls` sets 1 | same |
| 0x1A | `fire` | +0x0EA | action 0x1F; the rear-fire code also sets it | +0x05E (`f05E`) | `aiWeaponFireDecide`; `aiMovementUpdate` (the ram shot); `sub_800E9E20`, `aiFollowRoute` | `weaponFireSelected` 80103FB8 |
| 0x1B | `fire_guns` | +0x0EB | action 0x1E | +0x05F | `aiWeaponFireDecide` | `weaponTick` -> `UpdateGuns` 80104F70 (consumed) |
| 0x1C | `fire_special` | +0x0EC | `carReadMoveCodes` (a complete code) | +0x060 | `aiWeaponFireDecide` (selected weapon >= 8) | `weaponFireEnergyAttack` 801046FC (consumed) |
| 0x1D | `fire_ready` | +0x0ED | `weaponFireSelected` (re-armed when fire is released, human only) | +0x061 | `weaponFireSelected` sets it every call for the AI | `weaponFireSelected` |
| 0x1E | `fire_held` | +0x0EE | action 0x1F again, after the code reader | +0x062 | cleared only | (no reader found) |
| 0x20..0x27 | move-code state: `armed` +0x20, `count` +0x21, `timer` +0x22, `code` +0x24 (s32) | +0x0F0..+0x0F7 | `carReadMoveCodes` | +0x064..+0x06B | not used (not cleared by `aiClearControls`) | `carReadMoveCodes` |

State bytes the pad reader also writes (state block, human +0x09C + n, AI
+0x06C + n): `f06` "a direction is held" (+0x06; human +0x0A2, AI +0x072,
set by `aiInputFinalize` from the steer bytes), `f19` start flag (+0x19,
human only, read by the slide end) and `f1C` turbo flame on (+0x1C; human
+0x0B8 in `humanCarReadInput`, AI +0x088 `firing` in `aiInputFinalize`).
Weapons-block bytes written from input: `rear_fire` (+0x05), `f0C` energy
attack kind (+0x0C), `selected` (+0x0D). CONFIRMED.

### 1.2 Who fills it on an AI car (CONFIRMED, `aiCarDrive` 800E7EDC, ai_driver.c)

`worldUpdate` (8011A700) -> `aiCarShouldUpdate(i)` (8011B7D8) ->
`aiCarUpdate` (8011C4A0, call at 8011B19C) -> `aiCarDrive`:

| Order | Call | What it does to input or the AI | Pad record |
|---|---|---|---|
| 1 | stats `f03` re-place request -> `carNearestRoutePoint`, `aiPlaceOnRouteNode` | puts the car on a route node | skip |
| 2 | `carStatsDerive` 800FF878, fps rescale, `cruise_cap` | per-frame stats | keep (see 1.7 for `f00`) |
| 3 | freeze countdown, thaw look | | keep |
| 4 | `contact.f0E = 0`, `state.weapon_fired = 0` | | keep |
| 5 | if `f000` and not dead: `aiUpdateTargetRange` 800E8604 (+0x028, **+0x034**, range flags), `aiUpdateGoalBearing` 800E88C8, `aiUpdateTargetBearing` 800EC3A4 (+0x020..+0x024, +0x0D4), `aiTryRouteToSlowTarget` 800E9950, `aiUpdateMood` 800E8A84, `aiWeaponsUpdate` 800D3BB8 | the AI brain: target, mood, route choice, and (while +0x034) `aiPickWeapon` 800D3C2C + `weaponTick(car, 0)` 8010388C, which calls `aiWeaponFireDecide` 80103CA0 | **skip**; replace by the pad reader and a player weapon tick (1.6) |
| 6 | `aiMovementUpdate` 800E8F38 | `aiClearControls` 800EC938; turn-around and give-up timers (`aiStartGiveUp`, `aiMaybeReplanRoute` 800E9504, `aiUpdateGiveUp` 800E9674); stop (`aiUpdateStop` 800EC504); **only while +0x034 and not dead:** the ram shot (fire when lined up), route or free steering (`sub_800E9E20`, `aiFreeSteer` 800EA270, `aiFollowRoute` 800EAB50, `aiSteerToGoal` 800EB714, `aiStartRouteFollow`, `aiChooseNextNode`, `aiUpdateStuck` 800E8DFC, `aiSetSegmentSpeed` 800F6F6C), `aiInputFinalize` 800EC180, `carThrottleSteer(car, 0)` 800DD304; then always the physics: `carUpdateContacts` 800EF5D0, `carWheelsUpdate` 800EC994, `carSpinOutUpdate` 800E6B78, `carParisSinkZonesUpdate` 800F0450, `carCrushContactUpdate` 800F39D0, `carBodySwayUpdate` 800E5134, `carBodyAttitudeUpdate` 800F0944, `carWreckUpdate` 800E75F4 (dead) | **replace**: keep the physics calls, drop everything before them except `carThrottleSteer` |
| 7 | type 0xF0: lock onto the target's heading, `carSpinOut` | Dark Tooth's boss move | skip |
| 8 | `carPartsAnimate` 80114B70, Mr. Slam grab/swing | | keep |
| 9 | if `f000`: `CarRotUpdate` 800DF3EC, `carMoveByVelocity` 800DF594 | heading and position | keep (`aiCarStart` sets `f000` = 1) |
| 10 | turn total `drive.turned` (+0x47E), fall test, `carBodyPoseUpdate` 80114F24 | | keep (turned is harmless) |
| 11 | `restart_pending` -> `aiCarStart` | | skip (the port respawns, `AI_LIVES.md` 2) |
| 12 | `carUpdateSidewaysFlag` 800DFA20, `carSkidSmokeUpdate` 800DE4E8 (`human = 0`) | | keep |

Where the bytes come from (the AI writers in 1.1):
- steering, slide, throttle, brake, reverse: `aiSteerToGoal`, `aiFreeSteer`,
  `sub_800E9E20`, `aiFollowRoute`, `aiUpdateStop`, all inside step 6;
- turbo: `aiFollowRoute` (below a route speed limit) and `aiInputFinalize`;
- fire, guns, energy request: `aiWeaponFireDecide` (step 5), plus the ram
  shot in `aiMovementUpdate` (target nearer than 1000, lined up within 56,
  and weapons +0x83E past `(8 - level) * 60 + 180`) and the track-vehicle shots
  of `aiFollowRoute` (level 8, weapon 6 or 2 with `weaponTick(ai, 0)`);
- the weapon (`weapons.selected` +0x815) and energy kind (`weapons.f0C`
  +0x814): `aiPickWeapon` from the weight tables +0x840 / +0x844 by where
  the target is (+0x0D4); Outlaw's special/weapon-1 switch in
  `aiWeaponFireDecide`;
- rear fire: `aiWeaponFireDecide` writes `weapons.rear_fire` (+0x80D)
  directly when the target is behind.

`aiInputFinalize` (800EC180), in full: clear both steer bytes while
`hold_dist` (+0x077); `f06` (+0x072) = either steer byte; brake |= frozen;
throttle and reverse cleared while frozen or rolling (+0x07E); throttle and
turbo cleared while held by Mr. Slam (+0x089); then the **turbo flame**:
on when `turbo` is set, or (difficulty > 0 and speed > stats +0xC8), unless
the car's `top_speed * 3/4` is below its type's default; it switches
`objRegistrySetStep(type + 6, 0, 1 or 0)` on a change of `firing` (+0x088).
800EC384 is the `jal objRegistrySetStep` of the "off" branch. CONFIRMED.

### 1.3 Who fills it on a human car (CONFIRMED)

`humanCarUpdate` (8011C400) = `humanCarReadInput` (800DE5F8) ->
`carViewHotkeys` (8011C530) -> `humanCarTick` (800DC88C). The input-related
parts:

| Function | Input work | Human-only, not to copy for a pad record |
|---|---|---|
| `humanCarReadInput` 800DE5F8 | all bytes of 1.1 from `padCheckAction(action, car->player)`; `aiClearControls` while HP <= 0 or falling (state +0x1F); then `carReadMoveCodes`; at the end the turbo flame (state +0x1C) on while `turbo` and fuel > 0 | the view keys (actions 8, 9, 0x31, 0x39..0x3C into +0x0D4..+0x0DB, +0x0E6) and their gating of reverse and drive; the start flag `f19` is harmless to keep |
| `carReadMoveCodes` 800DECA8 | direction presses (actions 0x1A, 0x1B, 0x17, 0x16) into `code`; a complete code writes `weapons.f0C` (100..106) and `fire_special`, or rear fire (`weapons.rear_fire` and `fire`) | the cheat codes 0x12233344 (Mega Napalm), 0x21431122 (health), 0x12344321 (god mode, infinite weapons, Mega Guns): keep or drop by the port's rules; they use human offsets (HP +0x1F0, max HP +0x186, damageable +0x1F4, weapons +0x7C8) |
| `carViewHotkeys` 8011C530 | camera views, split toggle, map, HUD toggles, rear view (`carViewHotkeyReleased[player]`) | all of it (a pad record has no camera fields) |
| `humanCarTick` 800DC88C | `carThrottleSteer(car, 1)`, `humanCarSlideUpdate`, physics, `weaponTick(car, 1)` -> `weaponHumanCycleInput` 80103C08 (`cycle_next` / `cycle_prev` edge, `weaponCycleSelection` 801073B4) | the cheat combos (`padComboHeld(2..9, player)`: armour, health, turbo, invulnerable); the pad pause is not here (section 3) |

### 1.4 Energy attacks and rear fire: the human codes and the AI's way

| | Human | AI |
|---|---|---|
| kind | `carReadMoveCodes` writes weapons +0x0C (100 freeze, 101 mines, 102 napalm, 103 shield, 104 jump, 105 invisibility, 106 Minion with action 0x1E) | `aiPickWeapon` writes +0x0C: 100 or 101 (101 only usable in mood 2), else none |
| request | `fire_special` (+0x1C) = 1 when the code completes | `aiWeaponFireDecide`: `fire_special` = 1 when `selected` >= 8 and the fire roll passes |
| rear fire | code 0x342: `weapons.rear_fire` = 1 and `fire` = 1 | `aiWeaponFireDecide` sets `rear_fire` when the target is behind |
| fired by | `weaponFireEnergyAttack` (801046FC) from `weaponTick`, both | same |
| cost check | energy (+0x30) against 400 / 120 / 900, denied sound for humans | the same check; AI energy is refilled to 1000 every `weaponTick` |
| cooldown after | `20 >> 1` = 10 frames | `weaponFireInterval` (80180F44) = `(80 - 4 * level) >> difficulty` |
| jump | code 104 (`carSpinOut(car, human, 7, 1)`) | not as an energy attack: `aiSteerToGoal` hops with `carSpinOut` kind 7 itself |

So a pad record uses the human way: the port's reader runs an AI-layout copy
of `carReadMoveCodes` on the pad's direction presses, keeping its state in
the AI block's +0x064..+0x06B (unused by the AI; clear it at each start,
as `carRestart` clears human +0x0F1, +0x0F2, +0x0F4). It writes weapons
+0x814 (kind), +0x80D (rear fire) and input +0x060 / +0x05E. CONFIRMED (the
offsets); LIKELY (the design).

Two energy attacks have AI-only rules in `weaponFireEnergyAttack` that a pad
record would hit (CONFIRMED, weapon_special.c):
- **101 mines:** `if (!human && AI->f001 != 2) break;` refuses them unless
  the record's mood byte is 2, without costing energy.
- **103 shield:** the shield record's `owner` is written only for a human;
  see section 2 (the shield never ends).

### 1.5 What to skip and what to keep

| Piece | Where | Pad record |
|---|---|---|
| target, distance, bearings, range flags | `aiUpdateTargetRange`, `aiUpdateGoalBearing`, `aiUpdateTargetBearing` | skip; but **force +0x034 = 1** every frame (section 2: damage gate, steering gate, sounds) |
| mood | `aiUpdateMood`; the pack director in `worldUpdate` | skip; leave the record out of the director loop |
| route choice and following | `aiTryRouteToSlowTarget`, `aiFollowRoute`, `aiFreeSteer`, `aiSteerToGoal`, `sub_800E9E20`, `aiStartRouteFollow`, `aiChooseNextNode`, `aiSetSegmentSpeed`, `aiFollowRouteTarget` | skip |
| stuck and give-up recovery | `aiUpdateStuck` (drains 1 HP now and then after 25 stuck updates), `aiStartGiveUp`, `aiUpdateGiveUp`, `aiMaybeReplanRoute`, `aiReplanRoute`, `aiUpdateStop`, the re-place request (stats `f03`) | skip |
| weapon decisions | `aiWeaponsUpdate`, `aiPickWeapon`, `aiWeaponFireDecide`, the ram shot, `carHasMeleeSpecial` auto-fire in `weaponFireSelected` | skip |
| input masks and turbo flame | `aiInputFinalize` | skip; the pad reader does the human masks and the flame |
| AI body settling | `carBodyFollowCar` is called only from the AI steering functions (`aiReplanRoute`, `aiUpdateStop`, `aiFreeSteer`) | **call it yourself**, or the slide code below, as `humanCarTick` does |
| throttle and steering | `carThrottleSteer` | keep, with player rules (1.7) |
| slide | `humanCarSlideUpdate` 800DF770 (human only: `carSlideEndIfSlow` 800DF7E0, `humanCarSlideStep` 800DFD14, `humanCarSlideStart` 800DF8A0 -> `carBodyFollowCar` 800DF990 / `carGripSlowdown` 800DFB28) | port an AI-layout copy (the helpers take `(car, human)` except the two `humanCar...` ones) |
| physics | `carUpdateContacts`, `carWheelsUpdate`, `carSpinOutUpdate`, `carParisSinkZonesUpdate`, `carCrushContactUpdate`, `carBodySwayUpdate`, `carBodyAttitudeUpdate`, `CarRotUpdate`, `carMoveByVelocity`, `carBodyPoseUpdate`, `carUpdateSidewaysFlag`, `carSkidSmokeUpdate`, `carPartsAnimate`, Mr. Slam | keep, `human = 0` |
| collisions, pickups | `carsPhysicsUpdate` 8011E220 (runs for AI cars whose +0x59C "updated" is set) | keep: `worldUpdate` sets +0x59C when the car is updated |
| damage and death | `carApplyDamage`, `aiCarDamageUpdate` 8011BBD4, `carDestroyed` 8011BE10 | keep; change the rules of section 2 |
| weapon tick | `weaponTick(car, 0)` | **replace** (1.6): its AI branch refills and calls `aiWeaponFireDecide` |
| wreck | `carWreckUpdate(car, 0)` | keep while dead; it rewrites +0x034 from the target distance and blows the wreck apart when the target is 4320 away (`AI_LIVES.md` 1.2), so the port's respawn should not wait on it |

### 1.6 The hook (LIKELY, a design built from CONFIRMED pieces)

One port table, `padOwner[i]` = -1 for a computer car, else the local
player (2 or 3) driving `aiCars[i]`.

In the AI loop of `worldUpdate` (8011B168..8011B1D8: `jal aiCarShouldUpdate`
at 8011B168, `jal aiCarUpdate` at 8011B19C):

```
if (padOwner[i] >= 0) {
    if (aiCars[i].stats.wreck_state != 4) {     /* as aiCarShouldUpdate; never throttled */
        aiCars[i].stats.probe_on = 1;           /* +0x59C, "updated this frame" */
        portPadCarUpdate(aiCarObjects[i], &aiCars[i], padOwner[i]);
    } else
        aiCars[i].stats.probe_on = 0;
    aiCars[i].f00C = 0;
} else  /* retail */
```

`portPadCarUpdate` = `aiCarUpdate` (8011C4A0) with `portPadCarDrive` in
place of `aiCarDrive`, then the same copy of position, rotation and matrix
into the object. `portPadCarDrive(car, p)`, in `humanCarTick`'s order:

```
carStatsDerive(&car->stats);                    /* 1.7: stats.f00 = 1 */
fps rescale of speed, steer, yaw_rate (as aiCarDrive)
freeze countdown and thaw (as aiCarDrive)
car->contact.f0E = 0; car->state.weapon_fired = 0;
car->active = 1;                                /* +0x034, section 2 */
padReadInput(car, p);                           /* below */
carThrottleSteer(car, LAYOUT_AI | RULES_PLAYER);/* 1.7 */
padSlideUpdate(car);                            /* AI-layout humanCarSlideUpdate */
carUpdateContacts(car, 0); carWheelsUpdate(car, 0); carSpinOutUpdate(car, 0);
if (gameGetMode() != 4 && gameGetMode() != 2) carParisSinkZonesUpdate(car, 0);
if (car->state.crush_contact) carCrushContactUpdate(car, 0);
carBodySwayUpdate(car, 0);                      /* 1.7: AI divides steer_limit by 10 */
Mr. Slam swing / grab / spin (as humanCarTick, side 0)
carBodyAttitudeUpdate(car, 0);
if (car->state.dead) carWreckUpdate(car, 0);
carPartsAnimate(car->type, car->motion.speed);
if (!car->state.f01) carBodyFollowCar(&car->motion, &car->state);
CarRotUpdate(car, 0); carMoveByVelocity(car, 0);
fall test (as aiCarDrive)
carBodyPoseUpdate(car, 0);
padWeaponTick(car, p);                          /* below */
carUpdateSidewaysFlag(car, 0); carSkidSmokeUpdate(car, 0);
```

`padReadInput(car, p)`: `humanCarReadInput` with the AI block (+0x044) and
AI state (+0x06C), reading the port's pad for player p instead of
`padCheckAction(action, car->player)`:
- copy: the HP / falling clear (`aiClearControls(&car->input)`), turbo
  (0x1D, not frozen +0x079, not held +0x089), forward, reverse, brake
  rules, drive, throttle (not frozen, rolling +0x07E, all in the air +0x078,
  held), steer bytes, `power_slide` from stats `accel_launch`, the
  power-slide clear, the slide bytes, `f06`, cycle (0x21 / 0x20), fire
  (0x1F), guns (0x1E), the code reader, the turbo flame with state +0x088
  and `objRegistrySetStep(type + 6, 0, 1 or 0)`;
- do not copy: the view keys (actions 8, 9, 0x31, 0x39..0x3C) and their
  clearing of reverse and drive; `carViewHotkeys`; the cheat combos of
  `humanCarTick`. Pause is handled outside (section 3);
- do not call `aiClearControls` every frame: it sets `cycle_armed` = 1 and
  would break the edge test of the weapon cycle. Write every byte instead,
  as `humanCarReadInput` does.

`padWeaponTick(car, p)`: `weaponTick` (8010388C) with the AI layout and the
player rules:
- `weaponHumanCycleInput` on the AI block (input +0x05B..+0x05D, weapons
  +0x808), in place of `aiWeaponFireDecide`;
- no refill (the refill runs when `weapons.infinite` is set or `MODE == 0`;
  keep only the cheat case);
- specials recharge and energy +1 per frame as retail;
- `weaponFireSelected` and `weaponFireEnergyAttack` with player rules (1.7);
- the gun loop: `soundStartSharedLoop(9, ..., car->index)` as the AI branch
  (the player loops exist only for players 0 and 1, section 3), and
  `UpdateGuns` with the player cooldown (2, not 5).

### 1.7 Making the record drive and fight like a player's car

Retail code that tells a player from a computer car by the `human` argument
(or by stats `f00`), and does more than pick offsets. CONFIRMED (code); the
change column is LIKELY.

| Rule | Where | Computer car | Player | For a pad record |
|---|---|---|---|---|
| launch speed and steering lock | `carStatsDerive` 800FF878 by stats +0x00 (`f00`: humans 1 from `carRestart`, AI 0 from `aiCarStart`) | `accel_launch` / 2, `steer_limit` x 10 | as stated | set `stats.f00 = 1` after every `aiCarStart` (it is read every frame; `carStatsLoad` only uses it to set `damageable`, which `aiCarStart` already set) |
| body roll | `carBodySwayUpdate` 800E5134, AI side | divides by `steer_limit / 10` (because of the x10) | `steer_limit` | with `f00 = 1`, treat the pad record as a player here, or the roll is 10 times too strong (clamped to `lean_max`) |
| speed factor | `aiCarReset` 80100034 (`carStatsSetSpeedMult(stats, aiDifficultySpeedScale())`, 8011D94C) and `aiApplyDifficulty` 8011D528 at `aiSetDifficulty` | 75 / 120 / 140 % by difficulty | 100 (`humanCarPlaceAtStart`) | `carStatsSetSpeedMult(&car->stats, 100)` after `aiCarStart` and after `aiApplyDifficulty` |
| max HP | `humanCarPlaceAtStart` 800FD6C4 | the type's value | value + 20 | add 20 to `max_hp` (+0x536) and `hp` (+0x5A0) after `aiCarStart` |
| refills | `weaponTick` 8010388C | ammo 1 of each, turbo 990, energy 1000 every tick | only with the cheat | 1.6 |
| fire rules | `weaponFireSelected` 80103FB8 | melee auto-fire at the target; cooldown `weaponFireInterval`; no empty-slot check (fires with 0 ammo and goes negative); `fire_ready` forced, so holding fire repeats | cooldown 20; empty or kind 8 cycles back with the denied sound; fire must be released | player rules |
| energy rules | `weaponFireEnergyAttack` 801046FC | cooldown `weaponFireInterval`; mines need mood 2; no denied sound; shield owner not stored | cooldown 10; denied sound | player rules; store the owner (section 2) |
| guns | `UpdateGuns` 80104F70 | cooldown 5 | cooldown 2 | player rule |
| turbo, burning, slope | `carThrottleSteer` 800DD304 | no turbo sound or pad loop; burning goes out at random by difficulty; slope term only while dead (so a computer car climbs at full speed); no air test before the slope | turbo sound 1 and loop 0 per player 0/1; turbo can put out fire for 50 fuel; slope term on the ground when fast or not driving | player rules, but the sound loop by AI index (section 3) |
| skid smoke | `carSkidSmokeUpdate` 800DE4E8 | smoke only when sliding (`f01`) | also with the turbo flame (`f1C`) | player rule |
| turbo flame | `aiInputFinalize` 800EC180 | on by speed at difficulty > 0 too | on while turbo with fuel | pad reader (1.6) |
| riding regen, hidden heal spot, cheats | `humanCarTick` 800DC88C | none | ammo and HP every 0x169 ticks on a moving surface (5 steps), full HP once at the level's heal spot | copy if wanted (AI layout: stats +0x58E regen clock, +0x590 steps, +0x4FD heal spot used) |

The simplest port change for all of these: give the shared functions a
second flag. Today `human` means both "human offsets" and "player rules";
the port passes `layout` (offsets) and `player` (rules) separately, with
`player = human || padOwner[index] >= 0`. The functions that only use
`human` for offsets need nothing.

## 2. Everything else that treats an AI record as a computer car

"Retail" is what happens to a pad record if nothing is changed. CONFIRMED
(code; VAs are the functions, or the instruction where given); the "change"
column is the port's rule.

### 2.1 Damage

| Behaviour | Where | Retail for a pad record | Change |
|---|---|---|---|
| AI-on-AI damage / 4 | `carApplyDamage` 800E5628, 800E5A4C..800E5A78: `if (!human && !attacker_human) amount /= 4` | a computer car's hit on player 3, and player 3's hit on a computer car or on player 4, are divided by 4 (guns and flame do 0) | treat a pad record as human on both sides: `victim_player`, `attacker_player` from `padOwner` (the attacker comes in as a slot; `carFromSlot` gives the AI index) |
| the 4320 rule | `carApplyDamage` 800E585C: `if (!active) return` (AI victim: +0x034) | no damage, flash or spin-out unless +0x034 is set; it is written only by `aiUpdateTargetRange` (skipped) and `carWreckUpdate` | force +0x034 = 1 every frame (1.6) |
| HP loss needs a car attacker | `carApplyDamage` 800E5B08: AI victim loses HP only when `attacker >= 0` | wall and object hits, crushes and other attacker -1 damage never hurt it | use the human branch for pad records |
| wall and object damage | `carContactRespond` 800E3800 (wall hit: `carApplyDamage(..., -1)` for humans, `aiWallBumpCount` 800E17DC for AI); `carContactResolve` 800E0C28 (object hit damage, humans only) | no wall damage; the AI back-up counter runs instead of the ricochet (`carRicochetOffObject` 800E4754) | player branch |
| burning drain | `groundFiresUpdate` 8010C160 | AI drains only while +0x034 | forced flag covers it |
| energy shield against freeze and fire | `projectileHitTest` 800D6FD8 | the shield stops freeze and fire only on humans | player branch |
| Dark Tooth as attacker doubled | `carApplyDamage`: x2 when a human attacker's stats mass is 100 (the armour cheat) | not for a pad attacker | cheat only; ignore |
| killer and last attacker | stats +0x503 / +0x504 (AI), +0x153 / +0x154 (human): **slots**; `GAME_EVENTS.md` 0 | correct: the slot of a pad record decodes to (AI, index) | the scoreboard maps an AI index with `padOwner` to its player |
| death | `aiCarDamageUpdate` 8011BBD4 -> `carDestroyed(car, 0)` 8011BE10 | dead flag, wreck, `soundStopSharedLoop(9, -1)` (every gun loop); **no event is scheduled**, no life is taken; the wreck spin side comes from +0x020 | handle the death as a player's (section 2.4) |
| debris direction | `carApplyDamage` break-up: from the nearest human (`humanCarNearest`) | looks only | none |

### 2.2 Being targeted, and aiming

| Behaviour | Where | Retail for a pad record | Change |
|---|---|---|---|
| who the computer cars chase | `aiGetTargetCar` 8011CF0C: `aiCarTargets[i]` (801879FC) is -1 outside attract, meaning `humanCarNearest` 8011C8E4 | **no computer car ever targets players 3 and 4**; they never drive at them and never fire at them on purpose | for -1, take the nearest live player over humans and pad records; return the AI record and `*human = 0` for a pad record (the AI code reads both layouts, `AI_VS_AI.md` 5.4) |
| aimed AI shots | `weaponFindTargetAhead` 80109F94 / `weaponFindTargetBehind` 8010A2AC, 80109FD8: AI owner, not attract -> `humanCarObject(0)->pos` | **player 3's own homing, fire, napalm and freeze missiles and its car special steer at player 1**; and computer cars' shots steer at player 1, never at player 3 or 4 | pad owner: run the human branch (nearest live AI car in the cone, then the nearest human, skipping the owner; a pad record is in the AI list and the owner is skipped by `local[1] >= 12`); AI owner: the AI's target (`AI_VS_AI.md` 10 row 2) |
| human missile lock-on, homing leap | `weaponFindTargetAhead` human branch, `weaponHomingLeapInit` 8010EF44 | players 1 and 2 lock onto a pad record as onto any AI car (HP > 0) | fine for versus; a team test for co-op |
| the hop instead of a ram | `aiSteerToGoal` 800EB8A0: `carSpinOut(ai, th, 7, 1)` with the target's human flag | against a pad record (`th` = 0) the computer car really hops; against a human it never did (`AI_VS_AI.md` 5.4) | pass 0 for a pad target, or skip the hop |
| Dark Tooth boss | `aiDarkToothBossUpdate` 8010FD44: swoop target `humanCarNearest`, damage on `humanCarObject` | never swoops on players 3 and 4 | include pad records |
| Mr. Slam grab | `weaponSlamFindTarget` 800F5498: humans within 160 first | a pad record is grabbed after humans | include pad records with humans |
| HUD target panel | `carNearestToHuman` 800EC8A0 -> `aiCarNearestAlive` | (sections 4-5) | |
| the pack director | `worldUpdate` 8011AC18.. (all AI cars not broken up) | moods handed to the pad record; its +0x028 (0 when dead) counts in every car's "closer" rank, and its +0x00C | skip pad records in both loops |
| the update throttle | `aiCarShouldUpdate` 8011B7D8 | skipped when +0x00C, +0x039 clear and +0x028 > 4800 or abs(+0x024) > 1024 (all stale for a pad record) | 1.6: pad records bypass it |
| collapse reroute | `aiCarsRerouteFromSection` 800F2420 (Antarctica) | re-plans the route state and runs `aiUpdateStuck` (HP drain) | skip pad records; `worldSectionDisable` 800F20C8 spins out humans on the section, do the same for pad records |
| level 8 trains | `trackVehiclesShouldStop` 80118B20, `trackVehicleGround` 80118BBC, `trackVehicleDestroy` 80118F38 (humans by count) | trains do not stop for or hit players 3 and 4 | include pad records |

### 2.3 Pickups, meters, effects

| Behaviour | Where | Retail for a pad record | Change |
|---|---|---|---|
| collecting | `carTestContactPoints` 800E05F0 -> `carContactApplyObject` 800E06CC -> `pickupApply(car, 0, id, slot)` 8010AFC8 | collects (the probe runs while +0x59C is set) | none |
| full store | `pickupApply`: the "full" refusal is human only | takes a weapon pickup with a full store, gains nothing, and the pickup is gone | player rule |
| HUD messages and jingle | `pickupApply`: `hudMessagePost` human only; jingle 0x18 when human or +0x034 | no "Weapon" / "Health" / "Turbo" text | the port's HUD (sections 4-5) |
| turbo and energy meters | `weaponTick` refill (`GAME_EVENTS.md` 13.2) | turbo 990 and energy 1000 every frame, ammo never runs out | 1.6 |
| warps | `carContactApplyObject` (materials 694..697): humans need all four wheels on the pad, AI any | warps more easily | player rule |
| object 3600 | `carContactApplyObject`: not consumed by a human, consumed by an AI | (object 3600 not traced) | player rule |
| invisibility | `weaponInvisibilityUpdate` 8010DE54 runs over **humans only** | energy 105 sets weapons +0x80C and timer +0x83C = 120, then nothing: no fade, no hidden object, the timer never runs, so +0x80C stays set and every later 105 is refused (`weap->invisible != 0`) until a respawn | run the same per-frame code for pad records (object `aiCarObject(i)`, AI weapons block) |
| energy shield | `weaponFireEnergyAttack` 801046FC case 103 writes the shield record's `owner` only for humans; `weaponShieldsUpdate` 8010DD30 ends it with `humanCarGet(owner)->state.energy_shield = 0` | the pad record's shield (+0x07A) **never ends** (immune for the rest of the life), and when the bubble expires it clears the shield of whichever human the stale `owner` names | store a tagged owner (player 0..3) and clear the right record |
| freeze look, turbo flame object, burning effect | per type (`carFreezeObjects`, `carTurboObjects`, `type >> 4`) | shared with any other car of the same type (as in retail) | none |
| riding regen, heal spot, cheats | `humanCarTick` | none | 1.7 |

### 2.4 Lives and round end

| Behaviour | Where | Retail for a pad record | Change |
|---|---|---|---|
| death event | `humanCarDamageUpdate` 8011B89C schedules result 1 (modes 1-3) or 3/4 (mode 4); `aiCarDamageUpdate` schedules nothing | the round goes on; nothing counts the life | the port's per-player respawn (`AI_LIVES.md` 2.6 for the record, with the fixes of 1.7 and +0x034) and its life count |
| enemy count, round won | `aiCarsCountRemaining` 8011C248 (HP > 0 over all AI cars) -> `worldUpdate` 8011B5AC..8011B62C | **players 3 and 4 count as enemies**: the round is won only after they die too | skip pad records in both counts (and in `aiCarsCountAlive` 8011E16C, the "Enemies: N" text) |
| round-end test | 8011B5CC: `humanCars[0]` not dead | ignores players 2..4 | "any player still in" (`COOP_LIVES.md` 3.2) |
| Paris tower | `aiCarDamageUpdate` 8011BDE0: `aiCarsRemaining < 3` | pad records counted | follows from the count fix |
| round restart | `roundStart` 8011A024 -> `humanCarsRestart` 8011A198 (human records only) and the global wipes in `carRestart` | at a restart after a death of player 1 or 2 (result 1), players 3 and 4 keep position, HP and weapons, while every projectile, mine, shield record and effect is wiped | restart pad records there too (`aiCarStart` + 1.7 fixes), or use the port's respawn for everyone |
| boss stages | `sessionRun` 8012BE5C reloads with `opponents = 1` for Minion (level 4) and Dark Tooth (level 8); `aiCars[0]` is the boss (`roundStart`, `aiCarsCountRemaining`) | the reload drops the pad records | put pad records after the computer opponents and keep them through the reload |
| lives text | `hudDrawSplit*` print player 0 only (`COOP_LIVES.md` 4) | (sections 4-5) | |

### 2.5 HUD, radar, camera, sound (pointers; sections 4-5 have the detail)

| Behaviour | Where | Retail for a pad record | Change |
|---|---|---|---|
| radar blips | `worldUpdate`: the human pair (0 on 1's radar, 1 on 0's) and AI blips relative to `humanCars[0]` and, in mode 2, `humanCars[1]`, into `hudRadars[0/1]` via `hudRadarPlotCar` 800FC928 | players 3 and 4 show on players 1-2's radars as computer cars (by type colour, HP > 0); there is no radar for them | a radar per view; plot every other car, humans and pad records alike |
| HUD data | `hudDrawSingleView`, `hudDrawSplit*` read `humanCarGet(player)` | none for players 3 and 4 | sections 4-5 |
| camera | `viewCarCameraUpdate(&humanCars[i], i)` 80102BE8 in the human loop; the camera fields are in the human record (+0x000..+0x08F); view 7 (`viewChaseCamera` 80102CC4) follows `aiCars[viewWatchedAiCar]` (80180FAC, `aiGetWatchedCar` 8011D128) with the AI layout | a pad record has no camera fields | a port camera record per pad player; view 7's code shows how to read an AI record (sections 4-5) |
| displayed speed | `hudSmoothSpeed` 8011CBD0 / `carGetDisplaySpeed` 8011CD08 into `hudSmoothedSpeeds[2]` | none | per view |
| engine sound | `worldUpdate`: `carSoundUpdate(slot, car, 0)` only while +0x034 or mode 0, else the type's engine sound is stopped | plays (with +0x034 forced); engine by type, no rpm | none |
| player sound loops | `carSoundUpdate` 8011DB74 (human: shield hum 21 per instance, loop 32 for burning or lava per player 0/1); `carThrottleSteer` (turbo loop 0); `weaponTick` (gun loop 9 per player 0/1) | no shield hum, no burn loop, no turbo sound; gun loop as an AI (shared, by AI index), stopped by `worldUpdate` (8011B6DC) when dead or +0x034 clear | shared loops by AI index (the player loop lists are two, section 3) |
| sound position | `soundLocateForView0` 800D3550 / `soundLocateForView(pos, -1)` 800D3570; pan from `soundGetListenerPan` 800D35A0 | located against the existing views | sections 4-5 (listener per view) |
| denied sound, HUD message | `soundSetDeniedPending` (global), `hudMessagePost` 800FBBC4 | human only | port's own per-player feedback |

### 2.6 The "is a human" tests by object id

A car's object id is human index + 1, or AI index + 50 (`carBindAiSlot`
8011A5CC; `SIXTEEN_CARS.md` 1.5). A pad record's shots carry an id >= 50,
so these owner tests treat them as a computer car's. CONFIRMED.

| Where | Test | Effect for a pad record's shots | Change |
|---|---|---|---|
| `bulletSpawn` 800D7A4C | owner >= 50: life 15, else 12 | longer bullet range | an `isPlayerId(id)` helper (id < 50, or id - 50 has `padOwner >= 0`) in each test |
| `bulletsMove` 800D7E30 | hit radius human 10 / AI 5 | half the gun hit radius | same |
| `weaponProjectileUpdate` 800DB110, `projectileHitTest` 800D6FD8 | radius human 15 / AI 8; breakables (500..) only for owner < 50 | missiles easier to miss; **shots do not hit breakables** | same |
| `weaponApplyHit` 800DC65C, targets 500..510, 519, 520 | `id < 50` -> `targetDamage` 80125118 (which fires the level events) | its shots do not damage these targets | same |
| `weaponFireFreezeMissile` 800D9AE0 | turn rate human 15 / AI 4 | its freeze missile barely turns | same |
| `weaponApplyHit` (the car case), `weaponRicochetUpdate` 8010CC54, `weaponTornadoUpdate` 8010E040, `weaponFlameUpdate`, `areaDamage` 8010B5C4, `groundFiresUpdate`, `carContactApplyObject` | owner < 50 = human | as `SIXTEEN_CARS.md` 1.5 | same |
| `UpdateGuns` 80104F70 | by the `human` argument, not the id | cooldown 5 | 1.7 |

Not found: a test that treats a car as a player by its record address, or a
"local player" flag in the AI record. `carSlotKinds` (80187A20) says 1 (AI)
for a pad record, and `carFromSlot` returns `human = 0`; every rule above
comes from that or from the id. CONFIRMED.

## 3. What is bounded by two players

What breaks if the port keeps the game's two human records and adds
players 3 and 4 as pad records. "Reads 3-4" says whether retail code would
ever see them. CONFIRMED unless marked.

| Bound | Where | Reads 3-4? | What to do |
|---|---|---|---|
| human records | `humanCars[2]` 80187B10, `humanCarObjects[2]` 80180FEC, `humanCarObjectSlots[2]`, `humanCarTypes[2]`, `humanCarTotal` <= 2; the human loop in `worldUpdate` **breaks** at a null object | no | unchanged under this plan |
| fixed humans 0 and 1 in `worldUpdate` 8011A700 | the human pair radar (0 on view 1, 1 on view 0), AI blips for `humanCars[0]` and (mode 2) `[1]`, camera only for `viewMainPlayer` (80180FA8) or in split | no | per-view radar loop over all players; camera for every view (sections 4-5) |
| round-end test | 8011B5CC (`humanCars[0]` dead only) | no | "any player in" over humans and pad records (2.4) |
| `roundStart` 8011A024 | `carSetResetWeapons(0, 0)` and `(0, 1)`; `humanCarsRestart` | no | restart pad records too (2.4) |
| `uaInit` 80119698, `carTablesClear` 80119848 | fixed 2 humans (reset-weapons flag, record id, +0x1F4, `PadSetConfig`, `D_80180FD8[2]` / `D_80180FE0[2]`, `viewCameraObjectTable[0..1]`) | no | none for pad records (their resets are `aiCarStart` and the 1.7 fixes) |
| settings `sessionSettings` 8016475C | `human_car[2]` (+0x08), `control_layout[2]` (+0x26, `padSetLayout` for ports 0 and 1 in `sessionRun` 8012BE5C), `lives[4]` (+0x14) | no | players 3-4's cars go in `opponent_car[]` (they are loaded and bound as opponents by `sessionBindCars` 8012BD94, `carIsSelectable` refuses duplicates, `tournamentOpponentCount` caps the total); their layouts and lives in port fields |
| lives per player | `lives[4]` +0x14..+0x17; the runner decrements only `lives[0]` (8012C260); `settingsClearOpponentCars` 80137864 writes 13 bytes from +0x0A, so it clobbers `lives[0..2]` (`COOP_LIVES.md` 4) | no | lives[2] must be set after any clear; lives[3] is untouched; or keep all four in the port |
| result codes | `humanCarDamageUpdate` 8011B89C: mode 4 gives 3 (player 1 died) or 4 (player 0 died) | no | a "last player standing" rule for 3-4 players in deathmatch, and the port's own result |
| win and loss tallies | `sessionRun`: `wins2`, `wins1`, `wins1b` (two-sided); `screenChallengeResult` 8012B0EC, `screenDeathmatchResult` 8012B2D4 (two players) | no | per-player tallies and a 4-player result screen in the port |
| game over and level-won screens | `sessionRun` result handling (GAME OVER on `lives[0]`; weapon flags `carSetResetWeapons(.., 0/1)`, u8 per human at 80180FA0, room for 4) | no | port |
| pause owner | `pauseMenu` 80129528: Start on pad 0, or pad 1 when split; the menu then reads that pad | no | the port's pause from any pad |
| pad ports | `padCheckAction` 80150CFC: ports 0, 1 and 2 (2 = either pad, via `padButtonHeld` 8014A57C); `padActiveBindings[2]`, `padConfiguredBindings[2]`, `padActionCache[128][2]` (port 2 writes one entry past the row: `[action][2]` is `[action + 1][0]`, LIKELY harmless), `padConfigs[2]`, the repeat state P1 / P2, `padButtonsLatched` (16 bits per pad, `InitPAD2`, no multitap) | no | the pad reader of 1.6 reads the port's own input for players 3 and 4; do not call `padCheckAction` with 3 or 4 (it returns 0 for any other port; `padActionHeld` called directly would index past both binding arrays) |
| view hotkeys | `carViewHotkeyReleased[2]` 80180B9C | no | per player |
| per-player sound loops | `soundStartPlayerLoop` 800D3244 / `soundStopPlayerLoop` 800D328C: two loop lists (player 0 or 1), flags `soundLoop32ActiveP1/P2` (80180A06 / 80180A07), turbo and gun loop getters P1 / P2 | no | pad records use the shared loops by AI index (`soundStartSharedLoop` 800D32BC), as computer cars do |
| sound position | `soundLocateForView0` / `soundLocateForView(pos, -1)` for the existing views | | sections 4-5 |
| HUD, radar, views, cameras | `hudPlayerRecords[2]` 801828F0, `hudOrderTables[2][2][2]` 801828D0, `hudRadars[2]` 801837C8, `hudRadarVisible` per player, `viewCameraObjects` / `viewEnvs` / `viewModes[2]`, `viewCameraObjectTable[2]`, `hudSmoothedSpeeds[2]`, split modes 0..2 | no | sections 4-5 |
| radar blips | `hudRadarPlotCar`: 12 blips per view (`count + 2 >= 25`) | | enough for 11 cars |
| HUD target panel | `carNearestToHuman` 800EC8A0: one player or mode 2 -> nearest AI, else the other human | no | sections 4-5 |
| AI chase target | `aiGetTargetCar` -1 = `humanCarNearest` over the humans only | no | 2.2 |
| aimed AI shots | `weaponFindTargetAhead/Behind`: player 1 only | | 2.2 |
| `carPlayerOneObject` | `bulletsUpdate` 800D87B4 stores `humanCarObject(0)` each frame | | read by the bullet code (LIKELY sound or near-miss); not traced |
| the AI-car count | `aiCars[9]`, `aiCarGet` clamp 9 (8011D0EC), `aiCarTargets[9]`, the 15 slots | | two pad records leave 7 computer cars at most (`SIXTEEN_CARS.md` for more) |
| car types | registries by `type >> 4` (`SIXTEEN_CARS.md` 1.3) | | four players and the opponents must still be 15 distinct types |

Loops that are already fine (by count, both arrays), from `SIXTEEN_CARS.md`
1.2: `carsPhysicsUpdate`, `carPairsUpdate`, the weapon hit and scan loops,
`areaDamage`, `weaponTaserTarget`, `weaponLightningScan`, `weaponTornadoUpdate`,
`weaponSlamFindTarget`. A pad record is in all of them as an AI car.

## 4. Views

Read with `WIDESCREEN.md` (the view-mode table), `MAIN_LOOP.md` (the frame steps) and
`RENDER_ANSWERS.md` R2/R3 (buffers, ticks inside the draw calls). Everything here is static
reading of the C in `src/semantic` and the retail EXE.

### 4.1 How a frame draws its views (CONFIRMED)

`rtMainLoop` 800D1318 (`rt_main.c`) draws at most two 3D views per frame. "View 1" is either
the second split-screen view or the rear-view mirror, never both.

| Step | Call | View 0 | View 1 (split) or mirror (1P) |
|---|---|---|---|
| frame record | flip the pair, `ClearOTagR(ot, 4096)`, reset the packet cursor (+8 = +4) | `D_80180EE4` → `renderView0Frames` 8018118C / `renderView0FrameB` 8018120C | `renderBufferView1` 80180EE8 → 8018128C / 8018130C; `renderBufferRearView` 80180EEC → 8018138C / 8018140C. The cursor is **copied from view 0's record** (`field_8 = D_80180EE4->field_8`), not reset |
| camera | `viewProc(i)` 80155E80 | before the update calls | after `worldUpdate` |
| effects | `streaksDraw(rec, i)` 800D8230, `fxSpritesDraw(rec, i)` 80110BE4, `debrisDraw(rec)` 8014B484 | yes | yes (split and mirror call them in a different order; harmless) |
| scene | `renderScene(rec, i)` 80152284 | yes, after `renderFlipFrame` | yes |
| world | `worldUpdate` 8011A700 | runs **between** view 0's draw and view 1's draw | – |
| present | `PutDrawEnv`, `PutDispEnv` (view 0 only), `DrawOTag(ot + 4095)`, then that view's HUD | view 0, HUD 0 | view 1, HUD 1; the mirror after the HUD |

So the order is: updates, view 0 draw, `worldUpdate`, view 1 draw, wait, present view 0 + HUD 0,
present view 1 + HUD 1. View 1 shows one world step newer than view 0 (RENDER_ANSWERS R3).

**Views are drawn one after the other through shared globals.** `viewProc(i)` writes the one
camera record (`csGetRoot()`), the one axis matrix `viewAxisScaleMatrix` 801A5188 and the GTE
H/OFX/OFY. `renderScene` then reads them. A view's `viewProc` must be followed by that view's
draws before the next `viewProc`. (CONFIRMED)

**What `viewProc(i)` does** (`view_system.c`): builds `RotMatrixYXZ` for the camera object
`viewCameraObjects[i]` and its parent, composes them with the axis matrix of the view's mode, writes
eye and view matrix to the camera record and saves copies in `viewEyePositions[i]` /
`viewMatrices[i]`, rotates the cull planes m40/m60 into the record, and sets H = row +0x08 and
OFX/OFY = (w/2, h/2) of the row. Mode 4 (mirror) first copies view 0's camera into view 1 and turns
it 180° (hard-coded `viewCameraObjects[1]` ← `[0]`).

**What `renderScene(rec, view)` does with `view`:** `viewGetMode(view) == 4` sets the mirror's
far distances (500 / 1000) and the back-face flip; `viewGetFollowObj(view)` is the view's own copy
of the camera-attached (sky) model; `viewGetRenderSplitMode()` picks the near-polygon OTZ (20 in
split screen, 70 otherwise). Everything else comes from the camera record. (CONFIRMED)

### 4.2 Creating a view: `viewCreate`, the split mode and who calls them (CONFIRMED)

- **`viewSplitScreenMode`** 801809E4: 0 single, 1 vertical (left/right), 2 horizontal (top/bottom).
  Getter `viewGetSplitMode` 800D1C18, setter `viewSetSplitMode` 800D1C24 (`rt_main.c`). The session
  runner sets it before the match.
- **`arenaSetup` 8011995C** (`arena_setup.c`) creates the views through `viewCreateWithCamera`
  80119CA0 (`ai_route_helpers.c`): split 1 → modes (5, view 0) + (6, view 1); split 2 → (2, 0) +
  (3, 1); else (1, 0), `rtSetRearView(1)` and (4, 1) on the mirror records. Any other split value
  falls into the 1P branch.
- **`carViewHotkeys` 8011C530** (`car_records.c`) toggles 1 ↔ 2 in a match and recreates both views,
  then `hudInitPanels`. It does not rerun `hudRadarInit`.
- **`viewCreate(env, mode, i)` 80155888**: refuses `i >= 2` (prints `strViewOverMax` "Trying to create a
  view greater than MAX"; retail `slti v0,s8,2` at 801558A4). It stores `viewModes[i]` and
  `viewEnvs[i]`, writes both frame records' DRAWENV with `SetDefDrawEnv(x, y, w, h)` and
  `(x + disp_w, y, w, h)` (clip = view rectangle, draw offset = (x, y)), the DISPENVs for a 320x240
  display, `isbg = 1` (each view clears its own rectangle to `viewBgColor` 801AB944 on `PutDrawEnv`),
  allocates `viewSharedRoot` and `viewCameraObjects[i]` once, then `viewLoadMode(mode, i)` 80155B30
  copies the row's cull planes into the camera object (m40 = a[0..2], m60 = b[0..2]).
- **`viewCreateWithCamera`** then allocates the camera's parent object `viewCameraObjectTable[i]`
  80181024 and parents the view to it (`viewSetParent`).

### 4.3 The split modes' numbers (CONFIRMED, from the EXE)

The full table is in WIDESCREEN.md section 2. Values relevant to splitting, read from 8017EC44:

| Mode | Use | x, y, w, h | OFX, OFY | H (+0x08 = +0x0C) | side planes a[0]/a[1] | vertical planes b[0]/b[1] | nominal FOV (+0x00/+0x04) |
|---|---|---|---|---|---|---|---|
| 1 | 1P | 0,0,320,240 | 160,120 | 228 | (∓3355, 2349, 0) 35.0° | (0, 1904, ∓3626) 27.7° | 70, 55 |
| 2 | split 2 top | 0,0,320,120 | 160,60 | 228 | (∓3454, 2200, 0) 32.5° | (0, 1765, ∓3695) 25.5° | 70, 55 |
| 3 | split 2 bottom | 0,120,320,120 | 160,60 | 228 | as 2 | as 2 | 70, 55 |
| 5 | split 1 left | 0,0,160,240 | 80,120 | 138 | (∓3547, 2048, 0) 30.0° | (0, 2681, ∓3096) 40.9° | 60, 47 |
| 6 | split 1 right | 160,0,160,240 | 80,120 | 138 | as 5 | as 5 | 60, 47 |
| 0 | unused | 0,0,320,240 | 160,120 | 277 | (∓3547, 2048, 0) 30.0° | (0, 1627, ∓3758) 23.4° | 60, 47 |

All rows: display 320x240, axis {4096, 3690, 3690}, near a[2] = (0, 4095, 0), far b[2] = (0, -4095, 0).
OFX/OFY are relative to the view's draw offset (DRAWENV ofs = x, y). The plane vectors are
(4096·cos α, 4096·sin α) for a half-angle α; the side planes are nominal atan(halfW / H), the
vertical ones atan(halfH / H) except modes 2/3, whose vertical planes (25.5°) are much looser than
their true 14.7°.

### 4.4 Everything sized for two views

| Array | VA | Size | What follows it |
|---|---|---|---|
| `viewFollowObjects` (sky copy per view) | 80180DDC | 2 pointers | `viewFov` 80180DE4 |
| `viewCameraObjectTable` (camera parents) | 80181024 | 2 pointers | `aiBossDefeatedFlag` 8018102C |
| `viewCameraObjects` | 80181110 | 2 pointers | `viewEnvs` |
| `viewEnvs` | 80181118 | 2 pointers | `viewModes` |
| `viewModes` | 80181120 | 2 ints | `sndObjNeedVoices` 80181128 |
| `viewEyePositions` | 801A51A8 | 2 × 12 | `viewMatrices` |
| `viewMatrices` | 801A51C0 | 2 × 32 | unnamed `D_801A5200` |
| frame records (view 0 pair, view 1 pair, mirror pair) | 8018118C | 6 × 0x80 | `eiffelTowerDebrisPos` 8018148C |
| frame slots `D_80180EE4`, `renderBufferView1`, `renderBufferRearView` | 80180EE4 | 3 pointers | `worldLoadShift` 80180EF0 |
| ordering tables `renderOtView1Buf0/1`, `renderOtView2Buf0/1` | 801E5A8C.. | 4 × 4096 entries | view 1 and the mirror **share** `renderOtView2Buf0/1` |
| view-mode table `viewModeTable` | 8017EC44 | 9 rows × 96 | ends at 8017EFA4; 16 rows in the port variant (`SOURCE_BUILD.md`, request 26) |

Loops bounded by 2 (CONFIRMED): `viewInit` 8015580C, `viewCycleMode` 8015576C (unused), `viewSetBgColor`
8015659C, the `i >= 2` guard in `viewCreate`. Single-slot assumptions: `viewSetFollowObj` 80156344
makes only `[1]` as a copy of `[0]`; `viewProc`'s mode-4 branch hard-codes views 0 → 1;
`rtMainLoop` has exactly one "second view" branch.

Values tested against the split mode with no default case (a new mode value breaks them, CONFIRMED):

| Where | Test | With a new mode 3 |
|---|---|---|
| `viewChaseCamera` 80102CC4 (`camera.c`) | `switch (viewGetSplitMode())` twice: camera offset (`cam_chase`/`cam_split1`/`cam_split2`) and the pitch bias (-159/-193/-102) | **offset not written** (cam_goal keeps last frame's value and then gets `+= pos`, so it runs away) and **pitch uninitialised**. Add a case |
| `rtMainLoop` HUD switch | 0 / 1 / 2 | no HUD and no view-1 present |
| `arenaSetup` | 1 / 2 / else | 1P views and the mirror |
| `carViewHotkeys` | toggles 1 ↔ 2 | the view key would switch a 4-view game to 2 views. Disable it |
| `hudInitPanels`, `hudRadarInit`, `hudRadarPlotCar`, `hudDisplayRadar`, `hudDrawTargetPanel` | `== 0`, `== 1`, `== 2` | fall into the horizontal-split or 1P layouts |
| `fxSheetStart` 800DC45C | `!= 0` doubles the flash timer | stays doubled (see 4.7) |
| `renderScene` near OTZ (`viewRenderSplitMode`, set at load by `viewSetFollowObj(p, viewGetSplitMode())` in `world_load_object.c`) | `!= 0` → 20 | 20, fine |

### 4.5 A third and fourth view: quarters

**New view-mode rows.** A quarter is 160x120, the 1P aspect. Two choices (both follow from the
existing rows; values computed, LIKELY right):

| Row | Use | x, y, w, h | OFX, OFY | H | a[0]/a[1] | b[0]/b[1] | FOV |
|---|---|---|---|---|---|---|---|
| 9 | top-left | 0,0,160,120 | 80,60 | **114** | (∓3355, 2349, 0) | (0, 1904, ∓3626) | 70, 55 |
| 10 | top-right | 160,0,160,120 | 80,60 | 114 | same | same | 70, 55 |
| 11 | bottom-left | 0,120,160,120 | 80,60 | 114 | same | same | 70, 55 |
| 12 | bottom-right | 160,120,160,120 | 80,60 | 114 | same | same | 70, 55 |

- **H = 114** is mode 1 at half size: every angle (35.0° side planes, 27.7° vertical) is the 1P
  one, so the 1P chase offsets and pitch fit. This is the suggested choice.
- **Tighter alternative: H = 138** (the vertical split's H). Then a[0]/a[1] = mode 5's (∓3547,
  2048) (30°) and b[0]/b[1] = mode 0's (0, 1627, ∓3758) (23.4°). It is mode 0 at half size.
- All other fields as mode 1: display 320x240, axis {4096, 3690, 3690}, a[2]/b[2] as every row. Do
  not use row number 4 for anything (the mirror tests).
- **Hor+ 16:9** (WIDESCREEN.md section 6): w 160 → 213, x 160 → 213 for the right column, side
  planes atan(106.5/114) = 43.1° → (∓2991, 2799); H and the vertical planes unchanged.
- The planes are copied into the camera object only at `viewCreate` → `viewLoadMode`, so the rows
  must exist before the views are created.

**What to grow** (the port relocates; all sizes above):

1. `viewModes`, `viewEnvs`, `viewCameraObjects`, `viewCameraObjectTable`, `viewFollowObjects` → 4
   entries; `viewEyePositions` → 4 × 12, `viewMatrices` → 4 × 32. Raise the guard in `viewCreate` and
   the loops in `viewInit` / `viewSetBgColor`.
2. `viewSetFollowObj`: allocate the sky copy for views 2 and 3 too (`csAlloc`, `f0 = 0xFFF`,
   `csSetModel(copy, obj->f11C)`), and set each view's position every step with
   `viewSetFollowObjPos(pos, i)` (worldUpdate does it for the humans).
3. Two more frame-record pairs (0x80 each) with their own DRAWENV/DISPENV, two more slot pointers
   and **four more ordering tables** (4096 entries, 16 KB each, double-buffered: one pair per view).
4. A split-mode value (say 3) with a case in each switch of 4.4.

**Draw loop for four views.** A sketch in retail's order, with view k > 0 drawn after `worldUpdate`
like view 1:

```c
for (k = 1; k < nviews; k++) {
    rec = flip(view k pair); ClearOTagR(rec->ot, 4096);
    viewProc(k);
    rec->cur = prev->cur;             /* continue the frame's one packet buffer */
    streaksDrawOnly(rec, k);          /* see 4.7 */
    fxSpritesDraw(rec, k); debrisDraw(rec); renderScene(rec, k);
    prev = rec;
}
```

The packet cursor must be chained (view k continues from view k-1's cursor). Retail copies from
view 0 only, because it has only one more view. At present: `PutDrawEnv(view k)`, `DrawOTag(view k
OT)`, HUD k, for each view. `PutDispEnv` once.

A port that draws all four views after all updates is cleaner (RENDER_ANSWERS R3 (d) 6): view 0 is
then one step fresher than in retail, which is visual only.

### 4.6 A camera following an AI record

**Retail** (CONFIRMED, `camera.c`, `world_update.c`):

- `worldUpdate` calls `viewCarCameraUpdate(&humanCars[i], i)` 80102BE8 for each human, but in 1P only
  for `viewMainPlayer` 80180FA8 (`if (viewMainPlayer == i || viewGetSplitMode())`).
- `viewCarCameraUpdate` zeroes the parent pose (`viewSetCameraPose(i, …)` 8011D388 on
  `viewCameraObjectTable[i]`), runs `viewChaseCamera` (views 1–4, 7) or `viewFixedCameraTrack` 8010353C
  (views 5, 6), then `viewSetRotation(cam_rot, c->player)` / `viewSetPosition(cam_pos, c->player)`. The
  camera's state lives in the **human** record (+0x006..+0x08B: `cam_ease`, `cam_rot`, `cam_pos`,
  `cam_goal`, `cam_look`, the offsets `cam_chase` / `cam_offset3` / `cam_offset` / `cam_split1` /
  `cam_split2`, `cam_look_offset`; view number +0x10C). `viewCameraOffsetsInit` 800FF764
  (`ai_car_start.c`) fills the offsets per car type at the car restart.
- **View 7** reads the target (`aiGetWatchedCar` 8011D128 = `aiCars[viewWatchedAiCar]`) for position,
  rotation, `state.falling` and the stats `axle_rear`, `ride_height`, `roof_height`. But the camera
  state, the speed lead, the "stay above the car" clamp, the ease rate (`state.f06`, `motion.steer`,
  `stats.steer_limit`) and the look-at rotation (`motion.rot_m`) still come from the watching
  human's own record. With a moving watched car and a still human, view 7 is only a loose chase.
- `viewCameraLockFrames` 80180AE4 and `cameraPrevPos` 80183F14 are single globals shared by every
  chase camera.

**An AI record has no camera fields.** Its +0x006..+0x08B holds index, slot, the AI ranges and
target fields. Its blocks sit elsewhere: state +0x06C (human +0x09C), stats +0x4FC (+0x14C), motion
+0x71C (+0x6DC), weapons +0x808 (+0x7C8). So `viewCarCameraUpdate` cannot take an AI record.

**What a chase camera for an AI-driven player needs** (LIKELY, straightforward):

1. A port-side camera block per extra player with the human layout's camera fields and a view
   number, filled like `viewCameraOffsetsInit` from the AI record's `type` (+0x030) at each
   (re)start.
2. A copy of `viewChaseCamera` that reads **every** car field from the AI record: `motion.rot`,
   `motion.pos`, `motion.rot_m`, `motion.speed`, `motion.steer`, `state` (`falling`, `f1D`, `f20`,
   `all_in_air`, `frozen`, `f06`), `stats.steer_limit`. Use the 1P offset `cam_chase` and pitch bias
   -159 for H = 114 quarters (the split offsets are for 2:1 and 2:3 views). Give it its own lock
   counter.
3. Each step: `viewSetCameraPose(k, zero, zero)`, then `viewSetRotation(rot, k)` /
   `viewSetPosition(pos, k)` with the **view index k** (not the record's `player` field), and
   `viewSetFollowObjPos(aiPos, k)`. Call it after the AI car's update (`aiCarUpdate` 8011C4A0, in
   `worldUpdate`'s AI loop) or after the loop.
4. For a dead or watching player 3/4, the same function with another car's record as the target
   (COOP_LIVES.md 6.2 describes the human-target version).

### 4.7 Buffers and per-view effects

**Per view, reused (CONFIRMED).** `renderScene` calls `renderInitState` 801521D0 at its start and
the emitter `800CDC54` and `renderNearPolys` at its end. So the mesh list (280), the matrix slots and
object matrices (200), the frustum matrices, the walker stack and the near list are filled and
consumed inside one view. More views need no larger caps.

**Per frame, shared (CONFIRMED).** One packet buffer (`renderPacketBuffer0/1`, 0x17CF0 = 97,520
bytes each) holds the primitives of **all** views of a frame. All six frame records point +4/+0xC at
the same buffer; when it is full the emitter silently stops (CAR_SELECT_CAR.md 3). Quarters at
H = 114 see the 1P frustum, so each emits about as many polygons as the 1P view; four views use
about four times the 1P space and twice the 2P space (LIKELY; not measured). Make it 4 × 0x17CF0 or
more (the size immediate is at 800D1A84/88, RENDER_ANSWERS R2.1). OTs are per view (see 4.5).

**Per-view effect calls must not multiply the game ticks.** `streaksDraw` carries update work:
`weaponDropsUpdate`, `weaponFlameUpdate` (damage, ignition, game `rand()`), the simulation half of
`weaponArcsDraw` (a `carApplyDamage` with a game `rand()`) and the `fxSheetTimer` countdown
(RENDER_ANSWERS R3 (d)). Retail already runs them twice per step with split screen or the mirror,
once in 1P without the mirror. Calling `streaksDraw` for four views would run them four times.

- Split it: run the tick half **twice per step** (as retail's 2-player split, which is also what
  `fxSheetStart` compensates for: it doubles `fxSheetTimer` whenever the split mode is non-zero), and
  only the draw half (the tracer loop, taser lines from cached points, the flash prim) for each
  view.
- `fxSheetDisplay` links one of two static polys chosen by `bulletsFrameParity`; views 0 and 1 differ
  only because `bulletsUpdate` toggles the parity between them. Views 2 and 3 would link a prim
  already in another live OT. Copy the prim per view.
- `fxSpritesDraw`, `debrisDraw`, `renderScene` are clean draws (R3 (d)) and can run per view. The
  animators in `renderScene` step at most once per step (guard reset by `renderFlipFrame`), so four
  views do not speed them up. `debrisDraw` sizes sprites with view 0's H (retail quirk), the same
  for all quarters.
- **Per-view hooks** (WORLD_MARKERS.md 8.4): the port's marker hook goes right after each view's
  `fxSpritesDraw`, with that view's record and index. It works for four views unchanged.
- **Sound** listens through `viewGetEyeTrans` / `viewGetViewMatrix` per view (`engine_sound_lists.c`);
  outside this section.

**Draw time.** Each view walks the whole level, every registered model and every object, then
emits. The cost scales with the number of views times the visible polygons, not with pixels: four
quarters cost about four 1P frames of walker and emitter work, against about two for 2P. On PC that
is small. The retail emitter is asm (C reference `reference/model_emitter_800CDC54.c`).
Simulation does not depend on draw time (fixed step of `rtFrameTime` tics).

## 5. HUD

### 5.1 Records, order tables and how a HUD is presented (CONFIRMED)

- **`hudPlayerRecords[2]`** 801828F0, 1900 bytes each (`TM2HudRecord` in `hud_main.c`; `TM2HudPlayer`
  in `hud_split.cpp`). They are static GPU primitives: a top box, the health bar and back, the
  target-panel bar and back, the energy bar and back, 10 weapon markers + modes, 10 big weapon
  icons, 10 ammo-digit pairs, the weapon-list box and frame, the digit font images, the speed
  digits, the turbo icon and its two digits, and in record 0 only the four mirror bars.
  Followed by `hudRadars` 801837C8.
- **`hudOrderTables[2][2][2]`** 801828D0: `[buffer][player]`, 2 entries each. Each HUD call does
  `ClearOTagR(ot, 2)`, `AddPrim`s into entry 0 and `DrawOTag(ot + 1)`. `AddPrim` puts a prim at the
  head, so the last one added draws first (the panel backs are added after their bars).
  `buffer` is `D_80180EE4 == &renderView0Frames`. Followed by `hudPlayerRecords`.
- **Where it draws.** `rtMainLoop` calls the HUD right after that view's `DrawOTag`, under that
  view's `PutDrawEnv`. HUD coordinates are therefore **local to the view** (draw offset = view x,
  y) and clipped to the view rectangle. The split HUDs use the same numbers for both players. The
  divider is drawn by player 0 only, inside view 0's clip, so only its first 3 pixels show
  (x 157..159 or y 117..119; LIKELY what the GPU does).
- **Callers.** `hudDrawSingleView(0, first)` 800F8370 (`hud_main.c`) in single view;
  `hudDrawSplitVertical(player, first)` 800F9084 for split 1; `hudDrawSplitHorizontal(player, first)`
  800F9D48 for split 2 (`hud_split.cpp`). None in the attract demo.
- **Init.** `UAdashInitDashboardIcons` 800FAA0C (`hud_icons.c`, at HUD asset load) parses the icon
  sprites into record 0 only in single view, records 0 and 1 in split (`player = 1` or `2`), then
  `hudInitPanels` 800FB0B0 (`hud_init.c`, both records, boxes by split mode, mirror bars).
  `hudRadarInit` 800FC5C4 (`ai_route_setup.c`) sets up both radars at `arenaSetup`.

Other per-player HUD state, all sized for 2 (CONFIRMED addresses, sizes from the next symbol):

| State | VA | Size | Notes |
|---|---|---|---|
| `hudRadarVisible`, `hudWeaponListVisible` | 80180A6C, 80180A70 | 4 bytes each, u8 per player | bytes 2..3 LIKELY spare: room for 4 |
| `hudMarkerBlinkCounters`, `hudMarkerBlinkPhases` | 80180A74, 80180A7C | 2 ints each | drop-armed blink |
| `carViewHotkeyReleased` | 80180B9C | 2 | view/radar/list hotkeys |
| `hudSmoothedSpeeds` | 8018100C | 2 | `hudSmoothSpeed` per human |
| `hudRadarArrow{Up,Down,Left,Right}Prims` | 80182308.. | 2 × 32 each | off-radar arrows |
| `hudTargetNameModes`, `hudTargetNameSprites` | 80182408, 80182420 | 2 × 12, 2 × 600 | target name text; followed by `hudOrderTables` |
| `hudRadars` | 801837C8 | 2 × 880 | 48 blip lines (two halves), cross, back, frame, flags, count, scale, half; followed by `hudRadarArrowUpSprites` |
| `hudRadarArrow*Sprites` | 80183EA8.. | 2 × 8 each | arrow images (`hudRadarArrowsLoad` 800FD00C) |
| single: `hudMessageCurrent` 80183F00, `hudSplitDivider` 80183EE8, `hudLivesText`, the split lives text modes | – | 1 | message is 1P only; lives text in split is player 0 only |

Radar blips are plotted in `worldUpdate` (`hudRadarPlotCar` 800FC928, `hud_radar.c`): each human gets
the other human; every live AI car goes to radar 0, and to radar 1 only when `gameMode == 2`. Each
plot is gated by `abs(dz) < 400 || rand() % 5 == 0`, a **game-state `rand()`**. Extra radars for
players 3–4 should not draw from the shared stream (or must do so identically on every peer).

### 5.2 What each element shows and where (CONFIRMED)

Coordinates are local to the view. "Energy" is `weapons.energy` (0..1000, the energy attacks);
`hud_main.c` calls its bar `turbo` and `hud_split.cpp` calls it `ammo`. "Turbo" is
`weapons.turbo / 10`; its icon and digits are `lap` / `lap_digits` in `hud_main.c` and `lives_icon`
/ `lives` in `hud_split.cpp` (both misnamed).

| Element | Data | 1P (320x240) | Split 1, vertical (160x240) | Split 2, horizontal (320x120) |
|---|---|---|---|---|
| message | `hudMessageStrs[hudMessageCurrent[0]]`, ticked by `hudMessageUpdate` | text centred (147,30) on box 84..214 x 19..34 | none | none |
| radar (if `hudRadarVisible`) | `hudRadars[p]` | frame 3..82 x 15..66, back 6..79 x 18..63, centre (43,41); arrows (37,12) (37,63) (0,35) (79,35) | back only, 7..80 x 20..65, centre (44,43) | back only, as 1P coords |
| target panel (`hudDrawTargetPanel` 800FD0D0, `car_destroyed.c`) | `carNearestToHuman(p)` 800EC8A0: nearest live AI car; returns early for a human target (2P deathmatch) | name centred x 48, y 79 (radar on) / 49; bar 7..89 (82 px) at y 68..80 / 38..50 | co-op only (`gameMode == 2`), same coords | co-op only; y 17..31 without radar |
| health | `stats.hp * 100 / stats.max_hp`, 70 px, colour `hudHealthLevel` (≥41, ≥13, >0, 0) | 10..80 x 215..225, back 9..81 x 214..226 | 83..153 x 20..30, back 82..154 x 19..31 | 239..309 x 18..28, back 238..310 x 17..29 |
| energy | `weapons.energy / 10` %, 46 px | 271..317 x 150..155 (list on) / 178..183 | 112..158 x 150..155 / 178..183 | 271..317 x 35..40 / 63..68 |
| weapon list (if `hudWeaponListVisible`) | `weapons.ammo[slot] > 0`, `selected` green, `drop_armed` blinks slot 3 | icon (288, 163+8s), digits 272/276; box 273..315 x 160..226, frame 270..318 x 157..229 | icon (128, 163+8s), digits 112/116; box 113..155, frame 110..158 | icon (288, 48+8s), digits 272/276, y 47+8s; box 273..315 x 45..111, frame 270..318 x 42..114 |
| selected weapon (list off) | `ammo[selected]` | big icon (283,191), digits 272/276, 190 | (123,191), 112/116 | (283,76), 272/276, 75 |
| turbo | `weapons.turbo / 10` | icon (221,200), digits 244/248, 199 | icon (11,205), digits 34/38, 204 | icon (11,90), digits 34/38, 89 |
| speed | `carGetDisplaySpeed(p)` 8011CD08 (raw speed; the watched AI car's in view 7) | 3 digits at (216,223) | none | none |
| lives | `gameGetLives(p)`; the text shows lives - 1 (`+47`) | text centred (50,212) | player 0 only, co-op, (11,222) | player 0 only, co-op, (11,107) |
| mirror frame | record 0 `.mirror` | 209..314 x 18..52 when the mirror is on | – | – |
| divider | `hudSplitDivider` | – | 157..163 x 0..240 | 0..320 x 117..123 |

### 5.3 A quarter-screen HUD for players 3–4

**Option A: retail code, a new layout.** A `hudDrawQuarter(player, buffer)` as a copy of
`hudDrawSplitHorizontal` with the x values of the vertical split (160 wide) and the y values of the
horizontal split (120 tall). Coordinates follow from the two retail layouts (LIKELY; tune by eye):

| Element | Quarter (160x120, local) |
|---|---|
| radar (suggest off by default: it is half the quarter's width) | back 6..79 x 18..63, centre (43,41) |
| health | 83..153 x 18..28, back 82..154 x 17..29 |
| energy | 112..158 x 35..40 (list on) / 63..68, back 111..159 |
| weapon list | icon (128, 48+8s), digits 112/116 at 47+8s; box 113..155 x 45..111, frame 110..158 x 42..114 |
| selected weapon | (123,76), digits 112/116, 75 |
| turbo | icon (11,90), digits 34/38, 89 |
| target panel | under the radar 6..90 x 67..81; without the radar use 1P's row y 37..51 (y 17..31 would overlap the health bar at x 82..90) |
| lives | (11,107) |
| dividers | both bars, drawn once after all views with a full-screen DRAWENV (0,0,320,240), as `screenLostALife` 8012940C does |

What it takes:

1. **Four HUD records and OTs:** `hudPlayerRecords[4]`, `hudOrderTables[2][4][2]`, and 4 of each array in
   the table of 5.1. Every primitive is linked into one OT per frame; players 3–4 must have their
   own copies (sharing one prim between two live OTs breaks the chain).
2. **Init:** `UAdashInitDashboardIcons` must fill all four records (its loop count is 1 or 2),
   `hudInitPanels` and `hudRadarInit` loop to 4 with quarter boxes, and the arrow sprites need 4
   entries (`hudRadarArrowsLoad` parses two per arrow).
3. **Data from an AI record** (players 3–4 drive `aiCars[...]`): the HUD functions take
   `humanCarGet(player)`. The stats block (`TM2CarStatsBlock`) and weapons block (`TM2CarWeapons`) have
   the same type in both records, so pass those two pointers instead:

| Value | Human record | AI record |
|---|---|---|
| hp / max_hp | +0x1F0 / +0x186 | **+0x5A0 / +0x536** |
| weapons block | +0x7C8 | **+0x808** |
| energy, turbo | +0x7F8, +0x7FA | +0x838, +0x83A |
| ammo[10], selected, drop_armed | +0x7D6, +0x7D5, +0x7CE | +0x816, +0x815, +0x80E |
| speed | `carGetDisplaySpeed(i)` | `aiCarGetDisplaySpeed(i)` 8011CE14 |
| lives | `gameGetLives(i)` | the port's own count for players 3–4 |

4. **Target panel:** `carNearestToHuman` uses `aiCarNearestAlive(pos)` 8011C98C, which would return the
   player's **own** AI record (distance 0). Exclude the player's own index.
5. **Radar:** plot from the player's position and heading (`motion.rot[2]`) every live car but itself,
   with the player's own `hudRadars[k]`, then swap its halves once per step (RENDER_ANSWERS R3 (d)).
6. **Per-step state:** `hudBlinkWeaponMarker` counts per player; `hudMessageUpdate` is 1P-only.
7. **Hotkeys:** `carViewHotkeys` takes a human record; players 3–4 need the radar/list toggles from the
   port's input, and the split toggle must stay off.

**16:9 anchoring** (WIDESCREEN.md section 5): in a 213-wide quarter, keep the left group (radar, turbo,
lives, target panel) at +0 and move the right group (health, energy, weapon list, box) +53.

**Option B: the port draws its own HUD** for all players, or only for 3–4. Read per step: hp and
max_hp, energy (0..1000), turbo (/10), `ammo[0..9]` with `selected` and `drop_armed`, the display
speed, the lives, the nearest-car target (name `carTypeName(type)`, its hp/max_hp), and radar blips
(own position, heading, every other live car with its type colour from `hudRadarPlotCar`'s table). The
icon images are in the HUD asset (`UAdashInitDashboardIcons`: children at +24 markers, +28 turbo icon,
+36 digit font, +40 big icons), already in VRAM once records 0/1 are initialised. The 1P message line
(`hudMessageCurrent`, `hudMessageStrs`) is global and not shown in split screen. Option B avoids the
four retail record copies and lets the port scale the HUD to the window instead of the 320x240
grid.
