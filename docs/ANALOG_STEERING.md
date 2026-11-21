# Analog steering and analog throttle

Answers item 18 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(analog steering in Controls: a stick tilted a little turns a little; and
whether gas and brake could take trigger values the same way). Static
analysis only: the C in `src/semantic`, checked against the retail
disassembly (`bin/SCUS_94306R.EXE`, load address 800CDC54). Nothing was run.
Addresses are retail.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code, not traced to the end or not seen running.

Frame rate: `timerGetFrameTicks()` (8015531C) is the vsync ticks per frame,
`T`, normally 3 (clamped 3..5); `timerGetFrameRate()` (8015532C) is `60 / T`
(20, 15 or 12 fps). Numbers below are for T = 3 (20 fps) unless a formula
is given. Angles: 4096 = 360 degrees.

Human car record offsets used below (`struct TM2HumanRecord`,
`include/semantic/cars.h`):

| Offset | Block |
|---|---|
| +0x09C | state flags (`TM2CarState`) |
| +0x0D0 | input flags (`TM2CarInput`), written by `humanCarReadInput` |
| +0x14C | stats (`TM2CarStatsBlock`, `docs/CAR_STATS.md`) |
| +0x6DC | motion (`TM2CarMotion`): +0x6DC `steer`, +0x6E6 `speed`, +0x6F0 `yaw_rate` |

The AI record has the same blocks at other offsets (input +0x044, state
+0x06C, stats +0x4FC, motion +0x71C).

## Short answer

- The pad is read in `humanCarReadInput` (800DE5F8, `car_input.c`; 800DEC80
  is the turbo `objRegistrySetStep` call inside it). Left and right become
  two on/off bytes: `input.steer_dec` (+0x0DF) and `input.steer_inc`
  (+0x0E0). CONFIRMED.
- The turn is made in `carThrottleSteer` (800DD304, `car_control.c`), called
  once a frame from `humanCarTick` (800DC88C, `car_tick.c`) right after
  `carStatsDerive`. While a direction is held, `motion.steer` (+0x6DC, s16)
  moves towards +/-limit by a step per frame (this is the ramp-up). When
  nothing is held it falls back to 0 (divided by T each frame on the ground).
  Then `motion.yaw_rate` (+0x6F0) is derived from `steer` and the speed, and
  `CarRotUpdate` (800DF3EC) adds `yaw_rate` to the heading `motion.rot[2]`.
  CONFIRMED.
- `motion.steer` is the single steering value. Its unit is heading change per
  frame (4096 = 360 degrees); its range is +/-`stats.steer_limit` (+0x1B0),
  50*T = 150 at 20 fps, three times that (or more) with the handbrake, half in
  the air. Positive = the Right action (0x16). CONFIRMED.
- For proportional steering, write a fraction of the limit as a target and let
  the game's own step move `steer` to it: change the clamp in
  `carThrottleSteer` from `lim` to `lim * stick`. Keep setting the on/off
  bytes from the stick's sign, because other code reads them. Writing `steer`
  directly from outside the function does not work, because the function
  changes `steer` and computes the yaw from it in the same call. Section 5.
  LIKELY (the design); CONFIRMED (the reasons).
- The same `steer` value also drives the chase camera's follow rate
  (`viewChaseCamera`), the body roll in corners (`carBodySwayUpdate`, `aim.lean_roll`),
  the body attitude (`carBodyAttitudeUpdate`), the speed loss when you steer
  past the grip (`carGripSlowdown`), and indirectly (through `yaw_rate`) the
  start of a slide (`humanCarSlideStart`). All of them already scale with
  `steer`, so they follow a partial stick without changes. The wheels do not
  turn visually (no reader). The AI has its own input block and writes its
  own `steer`. Section 4. CONFIRMED.
- Gas and brake can be analog in the same function. Throttle is an on/off
  byte (`input.throttle`, +0x0E2) that adds a speed step per frame up to
  `speed_cap`; brake (`input.brake`, +0x0E3) takes off `brake_decel` per
  frame. For a trigger, scale the step and the cap in `carThrottleSteer`, and
  the brake step. Section 6. LIKELY (the design); CONFIRMED (the path).

## 1. From the pad to the input bytes (CONFIRMED, `humanCarReadInput` 800DE5F8)

`padCheckAction(action, player)` (80150CFC) answers "is the button bound to
this action held" (`OPTIONS_CONTROLS.md`). The stock bindings
(`padLayoutTable` 80162F64, buttons as in `OPTIONS_CONTROLS.md`, layouts 0/1/2/3):

| Action | Buttons | Meaning |
|---|---|---|
| 0x16 | Right in all four | steer right |
| 0x17 | Left in all four | steer left |
| 0x18 | Square / R2 / Cross / Cross | accelerate |
| 0x19 | Circle / R1 / R2 / R2 | brake |
| 0x1A | Up | accelerate (D-pad) |
| 0x1B | Down | reverse |
| 0x1C | Cross / L2 / L2 / L2 | handbrake turn |
| 0x1D | Triangle / L1 / Triangle / Triangle | turbo |

The bytes it writes (human +0x0D0 + n = `TM2CarInput` field n):

| Byte | Field | Value |
|---|---|---|
| +0x0E0 | `steer_inc` | action 0x16 (Right) |
| +0x0DF | `steer_dec` | action 0x17 (Left) |
| +0x0A2 | `state.f06` ("a direction is held") | `steer_inc || steer_dec` (`car->steering` in car_input.c) |
| +0x0D2 | `slide` | handbrake (0x1C) together with a direction |
| +0x0D0 / +0x0D1 | `slide_inc` / `slide_dec` | handbrake with Right / with Left |
| +0x0E1 | `forward` | action 0x18 |
| +0x0DE | `reverse` | action 0x1B, cleared while a view key (+0x0D4/+0x0D5) is held |
| +0x0E3 | `brake` | action 0x19, or `forward` and `reverse` together, or `reverse` while `speed > 0` |
| +0x0E4 | `drive` | `forward`, action 0x1A or `reverse` (cleared with the view keys) |
| +0x0E2 | `throttle` | `drive`, unless frozen (+0x0A9), rolling over (+0x0AE), all wheels in the air (+0x0A8) or held by Mr. Slam (+0x0B9) |
| +0x0D6 | `power_slide` | `brake` while `|speed| > stats.accel_launch` (+0x200) |
| +0x0D3 | `turbo` | action 0x1D, unless frozen or held |

Then: if `power_slide` is set and a direction is held, `brake` and
`forward` are cleared. So brake + steer while moving is a power slide, not a
stop. (cars.h calls `power_slide` "braking above the slide speed"; the
threshold read is +0x200 = stats +0xB4, `accel_launch`, the "moving at all"
speed.)

The special-move and cheat-code reader `carReadMoveCodes` (800DECA8, same
file) calls `padCheckAction` for 0x16, 0x17, 0x1A and 0x1B itself. It does
not read the bytes above. A stick used for analog steering must therefore
still answer `padCheckAction(0x16/0x17)` as a D-pad press past some
threshold, or the codes stop working with the stick. CONFIRMED.

`state.f06` also counts as "the player did something" for the start flag
`f_0B5` in the same function (with `f_23E` between 31 and 180).

## 2. Steering in `carThrottleSteer` (CONFIRMED, 800DD304, `car_control.c`)

`carThrottleSteer(car, human)` is shared by human and AI cars; the `human`
argument picks the block offsets. `speed = |motion.speed|`;
`maxsp = max(speed, stats.speed_cap)`.

### 2.1 The limit `lim`

| Case | `lim` |
|---|---|
| normal | `steer_limit` |
| normal, some wheel in the air (`state.wheel_in_air`) | `steer_limit / 2` |
| `slide` or `power_slide` | `steer_limit * handbrake_lock` (3, or 2 for Hammerhead, Warthog, Mr. Slam) |
| the same, wheel in the air | that / 4 |
| `slide` and `power_slide` together, on the ground | that + that * 2 * speed / maxsp (up to 3x at top speed) |

If the car is frozen (`state.frozen`), both steering bytes are cleared first.

### 2.2 The step while a direction is held (the ramp-up)

```
if (speed > T*1520/100*32)              /* 1440 at T=3, "80 mph" */
    step = steer_rate_high;
else
    step = steer_rate_low - speed*(steer_rate_low - steer_rate_high) / (T*1520/100*32);
if (!state.f01 && pressing against the current sign of steer)   /* counter-steer, not sliding */
    step *= 2;
if (speed > steer_boost_speed && (power_slide || slide)) {
    step *= handbrake_lock;
    if (slide && power_slide) step += steer_rate_high;
}
steer_inc: steer += step, then steer = min(steer, lim)
steer_dec: steer -= step, then steer = max(steer, -lim)
```

`steer_inc` wins when both bytes are set.

### 2.3 Release (nothing held)

| Condition | New `steer` |
|---|---|
| `|steer| < steer_rate_low` | 0 |
| some wheel in the air | `steer - steer/16` |
| otherwise | `steer / T` (a third of it at 20 fps: back to centre in 2-3 frames) |

### 2.4 From `steer` to the turn

```
if (speed > accel_launch) {               /* moving */
    if (speed/32 < turn_speed)
        yaw_rate = motion.speed/32 * steer / turn_speed;   /* signed speed */
    else {
        yaw_rate = steer;
        if (motion.speed < 0) yaw_rate = -yaw_rate;
    }
} else if (state.f06)                     /* about stopped, a direction held */
    yaw_rate = mathBlendAngle(yaw_rate, steer/2, 0x5F);
else
    yaw_rate = 0;
```

`yaw_rate` is `motion.spin[2]` (+0x6F0). `CarRotUpdate` (800DF3EC, `car_motion.c`)
adds it to `motion.rot[2]` every frame, and `carBodyFollowCar` (800DF990) copies it to the body
unless the car is sliding. So the heading turns by `yaw_rate` units per
frame. Below `turn_speed` (stats +0x4C, 65..105) the turn is proportional
to speed. speed/32 at top speed is 52..80 at 20 fps, so most cars stay below
it; only fast cars near top speed (Mr. Grimm, Grasshopper, Spectre, Twister)
pass it.

### 2.5 The tuning values (`carStatsDerive` 800FF878, `hud_radar.c`; humans, mult 100)

| Field | Offset (stats / human) | Formula | At 20 fps |
|---|---|---|---|
| `steer_rate_high` | +0x60 / +0x1AC | `steer_high` (2) * 60 / fps = 2T | 6 |
| `steer_rate_low` | +0x62 / +0x1AE | 2 * `steer_low` * mult/100 * 60/fps = 2T * `steer_low` | 18..48 (`steer_low` 3..8) |
| `steer_limit` | +0x64 / +0x1B0 | `steer_rate_high` * `steer_lock` (25); x10 for AI | 150 (13.2 degrees per frame) |
| `steer_limit_half` | +0x66 / +0x1B2 | `steer_limit` / 2 | 75 |
| `steer_tilt_max` | +0x70 / +0x1BC | 5 * `steer_rate_low` | 90..240 |
| `handbrake_lock` | +0x6E / +0x1BA | base stat | 2 or 3 |
| `turn_speed` | +0x4C / +0x198 | base stat (not frame-scaled) | 65..105 |
| `steer_boost_speed` | +0xBC / +0x208 | 10 * `accel_launch` | |

`carStatsDerive` runs at the top of every `humanCarTick`, so these are
rebuilt each frame from the base stats. When the frame rate changes,
`humanCarTick` also rescales `steer`, `speed` and `yaw_rate` by old/new fps.

What this means at 20 fps: above 1440 speed units a held direction reaches
full lock in 150/6 = 25 frames (1.25 s), or half that when counter-steering.
At rest it takes 3 to 8 frames. Letting go centres the wheel in 2-3 frames.

### 2.6 Request 66: the turn's sign kept in reverse (`portHumanControl` bit 0, implemented)

**The switch the user feels (CONFIRMED in the code).** In `carThrottleSteer` (800DD304) the yaw rate has three
cases (2.4): rolling below `turn_speed`, `yaw_rate = motion.speed / 32 * steer / turn_speed` (the **signed** speed);
rolling at or above it, `steer`, negated when `motion.speed < 0`; and at `|speed| <= accel_launch` with a direction
held (`state.f06`), the turn in place, eased towards `steer / 2`, **never** flipped. Going from forward into reverse
with the same direction held, the car slows through `accel_launch` (turn in place: the nose turns towards the held
side), then rolls back past `-accel_launch` (the sign flips: the nose turns the other way). That is the "controls go
backwards" moment.

**Other sign changes found** (C grep and the functions below, CONFIRMED):
- `carRicochetOffObject` (800E4754, `car_object_contact.c`): a human bouncing off an object with a direction held
  gets the deflection target `rot +- 512` from the held side, the sides swapped while `state.reversing`
  (the same convention as the rolling yaw: retail's real-car reverse);
- not the slide: `carBodyRealign` (800DFDFC), `humanCarSlideStart`, `carSlideDrag`, `carGripSlowdown` and
  `carUpdateSidewaysFlag` use `|vel|` or forward-only thresholds and the held side, never the speed's sign; the input
  (`humanCarReadInput`) does not swap the steering bytes in reverse; `carContactResolve`'s `+-steer_limit / 2` comes
  from the hit's side, `carSpinOut`'s spins from the yaw rate it finds.

**The bit.** `portHumanControl` (`unsigned char [TM2_MAX_HUMANS]`, `.port_bss` after `portPreciseEmit`, every
variant; `port_humans.h`; acts with `TM2_PORT_HUMANS`), bit 0 `TM2_HUMAN_CTL_STEER_KEEP`: human h's turn keeps its
sign in reverse, Left turns the nose left on screen at every speed. The port sets it per human record from that
player's Controls option, before the match (every machine the same: it is match state, `rollback_regions.tsv`
snapshot and hash yes, `ONLINE_ROLLBACK.md` 22). 0 = retail.

**How** (`portHumanSteerKeep(car, human)`, `.port_text`: 1 for a human record whose bit is set, else 0; the AI
records, `human` = 0, always 0):
- `carThrottleSteer`: the rolling yaw takes the speed's size, not its sign (`YAW_SPEED` = `speed`, i.e.
  `|motion.speed|`, for `motion.speed`), in both rolling cases. The size is retail's; the sign is the steering's, the
  turn in place's, so the in-place turn and the rolling turn agree through `accel_launch` in both directions (no flip
  at the transition);
- `carRicochetOffObject`: the deflection target as going forward while reversing (`RICOCHET_REVERSING`).
Without `TM2_PORT_HUMANS` both macros are retail's text. `rand()`: none added (check_rollback's census).

**Check:** `tools/check_humans.py` check 13 (every humans link): `portHumanSteerKeep` is a port function called only
by the two, calls nothing; `carThrottleSteer` runs on a MIPS interpreter (FarMips; every callee a stub but
`mathBlendAngle` and the gate) on human record 0 over 33 speeds (forward, reverse, both sides of `accel_launch`) and
both steering signs: bit clear = retail's yaw (the flip in reverse seen), bit set = the steering's sign at every speed
and retail's size, the turn in place unchanged; no `rand()`. Negative control: a link without `TM2_PORT_HUMANS` (the
rollback one, the define-free one) must show retail's flip with the bit set too, which it does.

`portHumanCompact` (the drop-in's compaction) moves the byte with the record (`check_humans.py` check 11 found it
missing in the first build: every humans-group array must move).

**For the port:** write `portHumanControl[h] = reverse_unflipped ? 1 : 0` for each human record when the match is
set up (and for a joiner before its first step); keep it identical on every machine (part of the match setup, like the
layouts). The record index is the human's (`humanCars` index), not the local slot.

**Port's build** (`work/pv/r63_port_P4`): `portHumanSteerKeep` 808E4FBC, `portHumanControl`
808CEA80 (`TM2_MAX_HUMANS` bytes), `carThrottleSteer` 800DDD10 and
`carRicochetOffObject` 800E5280 (changed). Verification: full build `work/build-20261025-192227-415794` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK from `.LIB` `f557c1e3…`, default port variant `1becd00a…`: unchanged); links `work/pv/r63_{rb,port,allh16}_{P4,P2}` (port's `r63_port_P4` `dda33c12…`); `check_all.py` 27/27 (proofs, placement, check_rollback, check_custom 9, check_balance G22, check_humans 13). Not runtime-tested.

## 3. Handbrake, reversing, in the air, standing still (CONFIRMED)

- **Handbrake (`slide`, action 0x1C + a direction)**: the limit is
  `handbrake_lock` times larger, and above `steer_boost_speed` the step too.
  With brake held as well (`power_slide`), the limit grows with speed up to
  3x again. `humanCarSlideStart` (800DF8A0) detaches the body (state `f01`,
  slide) when `power_slide` is set, or when `|speed| > slide_cap` (+0x20C)
  and `|yaw_rate| > steer_limit_half`. While sliding, `carBodyRealign`
  (800DFDFC) turns the body back towards the travel direction at a rate
  that depends on which of `steer_inc` / `steer_dec` is held (a term of 0, 2T
  or 5T degrees, averaged with a speed term) and on turbo.
  `carGripSlowdown` (800DFB28) uses `steer_limit * handbrake_lock` as the
  grip while `slide_inc` or `slide_dec` is set.
- **Reversing**: the same `steer`; `yaw_rate` flips sign with the speed
  (2.4), so the car turns like a real car in reverse. Nothing else changes.
- **In the air**: `lim` is halved (quartered when sliding), the release is
  slow (`steer/16` per frame), and `yaw_rate` is still computed, so the car
  still turns in the air.
- **Standing still** (`speed <= accel_launch`): `yaw_rate` eases towards
  `steer/2` while `state.f06` is set. This is the turn in place. It is not
  flipped for reverse.
- **Collisions**: `carContactResolve` (800E0C28) sets `steer = +/-steer_limit/2`
  on some car hits (with a random body kick); `carWheelsUpdate` (800EC994) halves `steer` every
  frame while sinking; `carRicochetOffObject` (800E4754) and
  `carContactPushUpdate` (800E405C) read `state.f06` (and the first also the
  steering bytes) to pick the bounce side, and to end a wall push early
  (`carPushClear`) or stop its turn while a direction is held.

## 4. Who else reads the steering (CONFIRMED, C grep + disassembly scan)

Scan: every C reference to `motion.steer`, `steer_inc`/`steer_dec`,
`slide_inc`/`slide_dec` and state `f06`, plus raw `lh/sh 1756(..)`,
`lbu 223/224/162(..)` in the retail code.

| Reader | VA | Reads | Effect with a partial stick |
|---|---|---|---|
| `viewChaseCamera` (camera.c) | 80102CC4 | `state.f06`, `|steer| / steer_limit` | follow rate `70 - 30*|steer|/steer_limit` (clamped 40..98) while a direction is held, 40 otherwise; lower = tighter follow. A held small tilt keeps the camera loose (~65), like the first frames of a digital turn |
| `carBodySwayUpdate` (ai_route_nodes.c) | 800E5134 | `steer / steer_limit` (AI: /10 of it) | body roll target `aim.lean_roll` = steer * lean_max / steer_limit * |speed| / reverse_cap, clamped to `lean_max`; proportional by itself |
| `carBodyAttitudeUpdate` (car_rollover.c) | 800F0944 | `|steer| > steer_tilt_max`, `|yaw_rate| <= steer_rate_high` | snaps or eases body pitch/roll to the wheels; a gentle stick stays under the threshold |
| `carGripSlowdown` (car_motion.c) | 800DFB28 | `|steer|` against the grip | above `slide_cap` the car loses `brake_decel` speed per frame when `|steer|` is over a grip that falls from `steer_limit` to `steer_tilt_max` at top speed. A gentle stick keeps the speed |
| `humanCarSlideStart` | 800DF8A0 | `yaw_rate`, `power_slide` | slide starts above `steer_limit_half` yaw at speed |
| `carBodyRealign` | 800DFDFC | `steer_inc`, `steer_dec` | on/off only |
| `carRicochetOffObject` | 800E4754 | `state.f06`, `steer_inc`, `steer_dec` | on/off only |
| `carContactPushUpdate` | 800E405C | `state.f06` | on/off only |
| `carPartsAnimate` | 80114B70 | `motion.speed` only | no front-wheel steering visual exists |
| `aiFreeSteer`, `aiCarDrive` | 800EA270, 800E7EDC | the AI's own `steer` (+0x71C) | AI only; it blends and halves its own `steer` directly (800EA53C, 800EA960, 800EAA48) |

The AI's input block (+0x044) is written by the AI code (`aiInputFinalize`
800EC180 and `aiClearControls`), never by the pad. A change made only on the
`human` side of `carThrottleSteer` does not touch computer cars.

## 5. Analog steering: what to write and where

### 5.1 The value

A fraction `a` of full lock, signed, with Right positive (the sign of
`steer_inc`). In `carThrottleSteer` the target is `lim * a`. `lim` already
holds the air and handbrake cases and is in the current frame rate's units,
so the same fraction stays right at 20, 15 or 12 fps and with the handbrake.
Keep it per player in the port, for example `s16 portSteer[player]` in
-4096..4096, filled from the stick after the deadzone and response curve.
The car record has no spare byte for it.

### 5.2 The cleanest hook: replace the clamp, keep the on/off bytes (LIKELY)

1. In `humanCarReadInput`, keep setting `steer_inc` / `steer_dec`,
   `slide_inc` / `slide_dec`, `slide` and `state.f06` from the stick's sign
   (stick past the deadzone = that direction held). The readers in section 4
   need them, and the counter-steer doubling and the turn in place need them.
2. In `carThrottleSteer`, human side only, replace the clamp with a clamp to
   the target:

```
if (in->steer_inc || in->steer_dec) {
    /* step as retail */
    target = human ? lim * portSteerAbs(car) / 4096 : lim;    /* 0..lim */
    if (in->steer_inc) {
        mot->steer += step;
        if (target < mot->steer) mot->steer = target;
    } else {
        mot->steer -= step;
        if (mot->steer < -target) mot->steer = -target;
    }
}
```

At full tilt (`|a| = 4096`) this is the retail code, so a D-pad or keyboard
drives exactly as before. With a partial tilt, `steer` ramps at the game's
speed-dependent rate up to the partial lock, counter-steer is still twice as
fast, and the yaw, body roll, camera and grip loss follow from `steer`.
Easing the stick back snaps `steer` down to the new target, as retail snaps
it down to `lim` when you land or let go of the handbrake. The retail release
(`steer / T`) is nearly as fast. For a softer return, use
`steer = target + (steer - target) / T` when `|steer| > target`.

### 5.3 Why not write `motion.steer` directly (CONFIRMED)

- Before `carThrottleSteer`, a written `steer` is changed again: stepped and
  clamped if a byte is set, divided by T if none is set.
- After it, the yaw for this frame has already been computed from the old
  value, so the turn lags a frame. The next call then steps or decays it
  again.
- Writing it in place of the step inside the function gives instant lock
  with no ramp. At 20 fps the retail ramp at speed is 25 frames, so an
  instant lock would feel much twitchier than the D-pad, and the counter-steer
  and handbrake step rules would be lost.
- `yaw_rate` cannot be written instead either: slides, collisions
  (`carContactPushUpdate`, spins) and the turn in place write it too.

So the hook is a small change inside `carThrottleSteer` (5.2), gated on
`human`.

## 6. Analog gas and brake (triggers)

### 6.1 The retail path (CONFIRMED, `carThrottleSteer` after the steering)

- If frozen: `brake = 1`, `throttle = 0`.
- **Turbo** (`input.turbo` with fuel): `throttle = 0x7F` (only tested for
  non-zero), `maxsp = speed_cap * 4/3` clamped to 140..180 "mph",
  `accel = 4 * accel_launch`. Otherwise `accel = accel_launch`, and if
  `speed > speed_cap` the throttle is cleared.
- **Brake** (`input.brake`): unless all wheels are in the air, `speed` moves
  towards 0 by `brake_decel` (half if accelerate/reverse is held in the same
  direction); `state.accelerating = 0`.
- **Throttle** (`input.throttle`): `state.accelerating = 1`; reversing when
  `reverse` is held and `speed <= 0`. If below the cap (`maxsp` forward,
  `-reverse_cap` backward) and not sliding (`state.f01`):
  `step = accel - (accel - accel_top) * speed / maxsp`, so 5x `accel_top` at
  rest, tapering to `accel_top` at the cap. There is no wheel check here: it
  also accelerates in the air.
- **Coast** (no throttle, on the ground): `speed` drops by
  `max(accel_top, (brake_decel - accel_top) * speed / maxsp / 2)` a frame.
- **Slope**: on the ground, when fast or not driving, `speed += ` the slope
  term from `body_rot[0]` and `stats.accel`.

Values (`carStatsDerive`, humans, 20 fps): `accel_top` = 3*`accel`,
`accel_launch` = 15*`accel`, `brake_decel` = 3*`accel`*`brake`,
`speed_cap` = top_speed*19*T/100*32 (1664 for Sweet Tooth),
`reverse_cap` the same with 3/4 of the top speed. All are rebuilt every frame,
so writing them from outside does not hold. `accel_launch` is also the
"moving at all" threshold in many places (power slide, yaw, rollover,
crashes), so it must not be scaled anyway.

### 6.2 Partial throttle (LIKELY, a design)

Keep `forward` / `throttle` / `drive` on/off from the trigger past a small
threshold, so the reading code (brake-while-reversing, power slide, the
slope gate on `drive`) stays as it is. Add a per-player fraction `t`
(0..4096) and, in the throttle branch, human side only, unless turbo:

```
cap  = maxsp * t / 4096;            /* forward cap; reverse: reverse_cap * t / 4096 */
step = step * t / 4096;             /* after the retail taper, which uses maxsp */
if (speed > cap) fl->accelerating = 0;  /* let the coast drag bring it down */
```

Scaling only the step would still reach full top speed with a light
trigger, and scaling only the cap would give full acceleration up to a lower
limit. Both together feel like a pedal. With `t = 4096` it is the retail code.
Turbo should stay full (`throttle = 0x7F` and its own `maxsp`/`accel`).

### 6.3 Partial brake (LIKELY, a design)

In the brake branch, `step = brake_decel * b / 4096` for the trigger's
fraction `b`. Leave the brake from Down-while-moving-forward and from
accelerate+reverse at full. Because any brake while moving with a direction
held turns into a power slide (section 1), use a threshold before the trigger
sets the brake byte, or a light brake in a corner will slide.

## 7. Notes

- `carThrottleSteer` is shared with the AI; every port change above is
  behind `human`. The AI's limits are different (`steer_limit` x10,
  `accel_launch` /2).
- The analog value is per player and needs no save/state field. If the port
  syncs input online, sync the fractions with the pad bits.
- With more than two humans (`SIXTEEN_CARS.md`) the arrays need one entry
  per human player.
- `cars.h` field names: `TM2CarStatsBlock` offsets in this doc are within
  the stats block; human-record offsets are stats + 0x14C.
