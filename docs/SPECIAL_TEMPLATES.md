# Mod characters' specials: templates and their numbers (request 73)

Port request 73 (2026-10-27): a mod character fires a special of its own choice and sets its numbers. With the
user's addition, the choice includes the regular pickup weapons, so a mod car can have a regular weapon as its
special. This file is the census the request asks for (item 2: every tunable per template, its retail value, where
it is, how it is stored, and the limit beyond which the code breaks), what each template needs from the firing car,
and the AI's use of specials. The build is in section 6.

Sources: the C (`src/semantic`), read 2026-10-27; `docs/CAR_STATS.md` 3, 4, 6, `GUN_GAME_SPECIALS.md` 1-4 and
`BALANCE.md` were checked against it. Tags: **CONFIRMED** = read in the code (the default here); **LIKELY** =
inferred.

**Units.** *Frames* are game updates (20 a second). *Ticks* subtract `timerGetFrameTicks()`, 3 per frame.
*D-units* are the game's distance, `max(|dx|, |dy|, |dz|) >> 3`: one D-unit is 8 position units (one world unit).
Distances not marked D are position units. Angles: 4096 = a full turn.

## 1. Rules every template shares

**Damage** goes through `carApplyDamage` (`car_damage.c:144-249`):
- halved by the target's shield flag (+60), none at all under the energy shield;
- divided by 4 when an AI hits an AI (1..3 becomes 0);
- doubled when the attacker is a human with mass 100 (Dark Tooth). **A mod car with mass 100 doubles every special
  it fires** (request 70's mass override);
- a type-1 hit of 11 or more knocks back by `(240 * amount / 25 + 40) * (100 - mass) / 100`; a hit of 8 or more may
  spin the target; a type-0 hit under 5 never kills;
- a hit on a frozen car shortens the freeze by 6 x damage.

**Stock and recharge** (`weaponBlockInit`, `ai_route_setup.c:304-323`; request 70's profile can already replace
them): `recharge_period` s16 frames, `max_specials` s8.

| type | 0x10 | 0x20 | 0x30 | 0x40 | 0x50 | 0x60 | 0x70 | 0x80 | 0x90 | 0xA0 | 0xB0 | 0xC0 | 0xD0 | 0xE0 | 0xF0 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| recharge | 360 | 1200 | 1100 | 700 | 1400 | 700 | 1600 | 700 | 700 | 360 | 700 | 1000 | 400 | 360 | 700 |
| max held | 5 | 5 | 3 | 5 | 3 | 5 | 5 | 4 | 5 | 3 | 5 | 3 | 5 | 5 | 5 |

**The charge** is taken by `weaponFireSelected` after `weaponFireSpecial` returns (`weapon_select.c:229`; slot 0
never touches the pickup total `held`, :230). A special that cannot fire gives it back with `ammo[0]++`. AI ammo
never runs out (`weaponTick` sets every AI count to 1 each tick, `weapon_tick.c:46-52`).

**The projectile record** (`fxRecords[16]`, 980 bytes each; `TM2Proj`, `projectile_hit.c:24`; `TM2Seeker`,
`car_contact_resolve.c:20`). Most missile numbers are written here by the fire function and read back by the update,
so they can be set right after the fire call:

| off | field | width | limits |
|---|---|---|---|
| +0x02 | type | s16 | the behaviour: `==` tests on 14 (freeze), 0x29 (coloured), 0x31 (ghost), 51 (boomerang), 61 (spiral) |
| +0x04 | life | s16 frames | >= 1 (0 frees the record); the spiral multiplies by 4 (<= 8191), Dark Tooth's by 5 |
| +0x06 | speed | s16 per frame | +5 a frame up to **150 for every type** (`projectile_hit.c:187-191`), 75 for type 61; a start above 150 lasts one frame |
| +0x08 | turn rate | s16 angle | **types 0x29 and 0x31 divide by it** (`rand() % (rate * 4)`, `car_contact_resolve.c:94,109,127`): >= 1; **the spiral's 99 means Dark Tooth's inverted wobble** (`projectile_hit.c:144`): never 99 |
| +0x0A | damage | s16 | 0..32767; the boomerang shifts it left 2 (<= 8191); a negative value skips the impact effects |
| +0x38 | target | ptr | 0 = straight |
| +0x44 | trail segments | s8 | <= 6 (the record holds 7 vertices, LIKELY) |

- **Hit box:** radius 15 for a human's shot, 8 for an AI's (`projectile_hit.c:226-227`), so (r + 1) x 8 units; a
  Dark Tooth target adds 25. Two half-steps are tested per frame: at 150 a half-step (75) is already just over the
  AI box (72): **faster than 150 would tunnel** (LIKELY).
- **Pool:** 16 records (48 with `portCarOptions` bit 3). Warthog takes 3 per use, Minion 4.
- **Lock** (`AIM`, `weaponFindTargetAhead`, `weapon_targets.c:60-69`): ahead, local y 12..8000, |y| >= |x| (90
  degrees), z < 401 for AI targets; behind with rear fire. An AI firer outside attract mode aims at human 0.
- **Launch points** come from the firer's own stats (`gun[]`, `launch_height`, `mount_x`): any car launches any
  missile from its own muzzle.

## 2. The projectile specials (fire-time numbers)

### 0x10 Sweet Tooth: spiral (type 61)

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| damage | 15 | `weapon_basic.c:187` | argument -> +0x0A | 0..32767 |
| life | 50 x 4 = 200 | `weapon_fire_missiles.c:140`, `weapon_special_missiles.c:233` | +0x04 | <= 8191 before x4 |
| speed | 37, cap 75 | `weapon_special_missiles.c:232`, `projectile_hit.c:200` | +0x06 | |
| turn rate | 15 | `weapon_special_missiles.c:231` | +0x08 | not 99 |
| wave | +-284 pitch, period ~14 frames | `projectile_hit.c:143,148` | literal (shared with Dark Tooth) | |
| launch | muzzle + 24 + 40 high | `weapon_basic.c:161-165, 186` | | `BALANCE.md` 3.11: lower = earlier ground hits |
| model | 2301 (ST or BB file) | | | needs that file loaded (request 35's load mask) |

### 0x30 Outlaw 2: taser (an arc record, not a projectile)

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| range | 80 D (640) | `weapon_slot_target.c:83,91,104` | literal | |
| duration | 40 | `:114` | arc +0x04 s16 | **>= 40**: the first hit fires at `count == 39` (`weapon_arcs_draw.c:390`) |
| hits | 10 at count 39, 10 at count 0 | `weapon_arcs_draw.c:390-393` | literal | |
| pool | 5 arcs (16 with bit 3) | `players.h:77` | | |

The arc is stepped by `weaponArcsDraw`, once per drawn view (LIKELY faster in split screen). `BALANCE.md` bit 16
retries an arc that caught nobody.

### 0x40 Warthog: 3 coloured missiles (type 0x29)

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| damage | 6 each | `weapon_basic.c:210,216,222` | argument | **falls 1 a frame to 2** while locked past 300 D (`car_contact_resolve.c:95-98`) |
| count | 3 (centre, +gun x, -gun x) | | code | 1..3 (the three launch points) |
| life | 100 | `weapon_fire_missiles.c:235` | +0x04 | >= 1 |
| speed | 150 | `:236` | +0x06 | cap |
| turn rate | 35 | `:238` | +0x08 | >= 1 (divides) |
| far switch | past 300 D: napalm / freeze look, white / blue trail | `car_contact_resolve.c:95-105` | literal | |

A retail slip: the side missiles' x is set only if the previous missile spawned (`weapon_basic.c:211, 217`).

### 0x50 Mr. Grimm: skull (type 0x2B, straight)

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| damage | 40 | `weapon_basic.c:231` | argument | |
| life | 50 | `weapon_fire_specials.c:355` | +0x04 | |
| speed | 150 | `weapon_special_missiles.c:216` | +0x06 | |
| homing | none (target dropped) | `weapon_fire_specials.c:358` | | |
| model | 2322 (MG file) | | | load mask |

### 0x80 Spectre: ghost (type 0x31, homing, through walls)

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| damage | 12 | `weapon_basic.c:248` | argument | |
| life | 80 | `weapon_special_missiles.c:257` | +0x04 | |
| speed | 75 -> 150 | `:256` | +0x06 | |
| turn rate | 100 | `:255` | +0x08 | >= 1 (divides) |
| walls | ignored unless object kind 500..510 | `projectile_hit.c:252` | literal | |
| model | 2326 (SP file) | | | load mask |

### 0x90 Roadkill: boomerang (type 51)

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| damage | 10 out, x4 = 40 back | `weapon_basic.c:257`; `projectile_hit.c:173` | argument, shifted | <= 8191 |
| life | 50 x 4 = 200 | `weapon_fire_specials.c:296` | +0x04 | <= 8191 |
| speed | 75 out, 50 back | `:291`; `projectile_hit.c:175` | +0x06 | |
| turn rate | 5 out, 30 back | `:290`; `:174` | +0x08 | |
| turn-back | 300 D from the launch | `projectile_hit.c:171`, `car_contact_resolve.c:134` | literal | per-frame (stage 2) |
| catch | D < 20, or hitting its owner | `car_contact_resolve.c:144` | literal | |
| model | 2321 (RK file) | | | load mask |

**Retail problem:** a caught boomerang refunds the charge to the **first human Roadkill** (`projectile_hit.c:234`,
`humanCarGet(j)->type == 144`), not to its thrower, and only a human owner can catch it by hitting it. A borrowing car
never gets its charge back (`RETAIL_BUGS.md` candidate).

### 0xE0 Minion: 3 coloured missiles + freeze missile

The coloured missiles are Warthog's at **10** each (cyan, green, red; `weapon_basic.c:409, 413, 419`). The freeze
missile (type 14):

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| damage | 7 | `weapon_basic.c:422` | argument | |
| life | 50 | `weapon_fire_missiles.c:140` | +0x04 | |
| speed | 50 | `weapon_fire_specials.c:312` | +0x06 | |
| turn rate | 15 (human), 4 (AI) | `:308-311` | +0x08 | |
| freeze | 300 ticks (100 frames) | `weapon_slam_swing.c:78, 90, 123, 136` | literal for every type-14 hit -> `freeze_timer` s16 | <= 32767; `BALANCE.md` bit 17 halves repeats |

## 3. The other specials (most numbers in per-frame code)

### 0x20 Hammerhead: crush on the ram

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| crush hit | 15 | `misc_one_liners.c:52` (`carCrushDamageAmount`) | literal, type 0 | >= 5 to kill |
| ride drain | 1 HP every 4th frame | `proximity_update.c:76` | literal, bypasses `carApplyDamage` | |
| ridden car's freeze | 30 ticks | `car_crash.c:110-112` | literal | |
| charge | 1 per mount (humans) | `car_crash.c:92-96` | | |

Needs `stats.mounts_cars` (+0x02; retail Hammerhead, Axel, Minion, Dark Tooth). The fire button only refunds.

### 0x60 Grasshopper: homing leap

| tunable | retail | where | limit |
|---|---|---|---|
| target range | 400 D (3200) | `weapon_leap_homing_init.cpp:95, 108, 126` | |
| cone | 682 (60 degrees) | `:109, 127` | no angle wrap (LIKELY misses across +-2048) |
| homing from | age 8 frames | `weapon_leap_homing_update.c:90` | |
| hit radius | D < 15 (120) | `:120` | |
| damage | 15, type 2 | `:124` | `BALANCE.md` bit 7: 18 |
| timeout | 101 frames | `:129` | |

Moves the firer's own record; one state (`homingLeapState`) per car key.

### 0x70 Thumper: flame

| tunable | retail | where | stored | limit |
|---|---|---|---|---|
| emitter life | 50 frames | `car_restart.c:299` | s16 | |
| stream length | 15 steps | `weapon_flame_update.c:96-98` | u8, capped `>= 16 -> 15` | <= 15 without changing the cap |
| step | 32 forward, +1 high a step | `:78-105` | literal | |
| hit radius | 20 D (168), every other step | `:109-110` | literal | |
| damage | 1 a frame, type 1 | `:112` | literal | AI vs AI rounds it to 0 |
| burn | 100 frames, -1 HP on 1 in 4 frames while HP >= 6 | `fx_ground_fires.c:163-181` | s16, bypasses `carApplyDamage` | |
| pool | 2 emitters (16 with bit 3) | `players.h:81` | | full: nothing fires, the charge is spent (LIKELY) |

### 0xA0 Twister: tornado

| tunable | retail | where | limit |
|---|---|---|---|
| duration | 83 frames | `weapon_basic.c:277` | s16 |
| catch radius | 35 D (280) | `weapon_tornado.c:151, 192` | |
| catch freeze | 60 ticks | `:160, 200` | |
| catch damage | 4, type 1 | `:162, 202` | up to 7 catches = 28 (`CAR_STATS.md` 6) |
| throw-out | `freeze_timer == 1` | `:176, 216` | never true (`RETAIL_BUGS.md` 21); `BALANCE.md` bit 12 adds one |

**Needs:** the funnel object's model is the **Twister root** (bind 0xA0): another car would show a spinning Twister.
One state per car key. `weaponTornadoUpdate` calls `rand()` while active.

### 0xB0 Axel: shockwave

| tunable | retail | where | limit |
|---|---|---|---|
| radius | 800 (true 3-D distance, `carPairDistance`) | `weapon_basic.c:331` | |
| height window | \|dz\| < 200 | `:332` | |
| damage | 15, type 2, no knock-back | `:338` | |
| spin | kind 7 | `:334` | |
| breakables | D < 100: 15 (human Axel only) | `:342-347` | |

All in the fire code. The ring is node 2324 **inside Axel's body** (request 35 gives a borrower its own ring object
when Axel's file is loaded); the damage needs nothing.

### 0xC0 Mr. Slam: arm and claw

12 on the lift + 10 per slam (1..3 by grab distance); 25 once for a target **heavier than the firer** (not "mass >=
35"); halved on Easy auto-grab (`weapon_slam_swing.c`, `weapon_arcs_draw.c:163-166`, `car_wheel_sway.c`). **Needs** the
arm and claw, nodes 201 / 202 **of Mr. Slam's own model** (their angles are not even stored without them, so the slam
never ends) and the claw record (allocated only for type 0xC0).

### 0xD0 Shadow: strike

| tunable | retail | where | limit |
|---|---|---|---|
| speed | 66 a frame | `weapon_basic.c:400` | |
| life | 100 frames | `:382` | s16 |
| contact | radius 20 (168), 25 damage, type 1 | `strike_update.c:67, 73` | cars only for an AI's strike |
| timeout | explosion, **no damage**, charge taken | `:75-81` | |
| blast | radius 75 D (600); 25 - d/4, 50 - d/4 on a surface polygon; spin 5 | `area_damage.c:56-83` | shared with the remote bomb (radius 100); hits the firer too; `BALANCE.md` bit 10 |

Needs node 2325 (HS file) and one strike state per car key (request 35: an as-entry per borrower).

### 0xF0 Dark Tooth

A human's special is empty (`weapon_basic.c:428`; the port's request 36 gives a player's Dark Tooth the head swoop,
which needs his head, root 249 of `BB.DMD`). The boss controller (missiles of 5, the swoop's 7 + burning) is AI-only.

## 4. The pickup weapons

How each fires today, and its numbers. All but the ricochet and the remote bomb go through
`weaponLaunchProjectile(car, kind, human)` (`weapon_launch.c`), which never reads or changes an ammo count, `held` or
`selected`.

| weapon (kind) | fired by | damage | life | speed | turn |
|---|---|---|---|---|---|
| fire missile (1) | `weaponFireMissile` (`world_object_register.c:317`) | **7** (`weapon_launch.c:120`; 10 with `BALANCE.md` bit 0) | 50 | 150 | 10 |
| homing missile (2) | `weaponFireHomingMissile` (`weapon_fire_missiles.c:128`) | **10** (`:97`; 7 with bit 0) | 50 | 150 | 50 |
| power missile (4) | `weaponFirePowerMissile` (`ai_route_helpers.c:294`) | **15** (`:111`) | 50 | 150 | none (no target) |
| napalm (5; energy kind 102) | `weaponFireNapalm` (`weapon_fire_missiles.c:159`) | **10** (`:106`) | 150 | 30 | 7 |
| Mega Napalm (`weap->mega_napalm`) | `weaponFireMegaNapalm` (`:191`) | **30** (`:104`) | 150 | 75 | 50 |
| freeze missile (energy kind 100) | `weaponFireFreezeMissile` (`weapon_fire_specials.c:301`) | **7** (`:126`) | 50 | 50 | 15 human / 4 AI |

- **Napalm** arcs by the pitch roll (+32 a frame to 1024, `projectile_hit.c:126-133`) and starts a ground fire and a
  burn on a hit. **Freeze:** 300 ticks, as Minion's.
- **Ricochet bomb (6):** inline in `weaponFireSelected` (`weapon_select.c:139-177`; no function of its own): life
  400, 100 a frame, starts 40 high; power `max(7, (400 - life) / 20)` (7..20, u8), doubled at the first bounce
  (`weapon_ricochet.c:106-134`); hit box D < 20 + a spin; pool 10. Dark Tooth's look by `vehicle == 0xF0`.
- **Remote bomb (3):** `weaponDropItem(car, 3, human)` -> `weaponDropWrite` (`weapon_slot_writer.c:78-119`); a
  second press detonates: `areaDamage(pos, owner, 1)`, radius 100 D, 25 - d/4 (50 - d/4 on a surface). 2 bomb
  objects. The place-or-detonate switch is `weap->drop_armed`, shared with the pickup bomb.
- **Machine guns** are not a slot weapon: `UpdateGuns` (`update_guns.c:15`) fires one bullet per call (200 a frame,
  life 12 / 15, power 1 or 3 with Mega Guns, half the hits do damage, `rand() & 1`). A burst does not exist in the
  game.

**As a special** (section 6): a fire case calls the pickup's own fire function with the launcher's muzzle and the
template's damage, then sets life / speed / turn in the record; it never changes `selected` or calls
`weaponFireSelected`. The remote bomb as a special gets its own place-or-detonate state, refunds only `ammo[0]`
(never `held`, which would close the pickup capacity check, `pickup_apply.c:55`) and passes kind 3 explicitly
(`weaponDropWrite(0)` would clear `weaponDrops[-1]`).

## 5. The AI and specials

Which weapon an AI picks is by its **own type** (`aiCarSetupProfile`, `ai_route_nodes.c:42-110`; table 9 with the
target ahead, 10 behind; the EXE's table at 0x80162608, 24 bytes a car). Slot 0 (the special), percent:

| type | 0x10 | 0x20 | 0x30 | 0x40 | 0x50 | 0x60 | 0x70 | 0x80 | 0x90 | 0xA0 | 0xB0 | 0xC0 | 0xD0 | 0xE0 | 0xF0 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ahead / behind | 80/80 | 0/0 | 0/70 | 80/40 | 40/5 | 90/0 | 50/0 | 30/30 | 50/60 | 20/0 | 60/60 | 0/0 | 10/100 | 90/0 | 0/0 |

The melee specials (0x30, 0x70, 0xA0, 0xB0, 0xC0; `carHasMeleeSpecial`, `ai_route_helpers.c:176`) also fire when a
target is within 400 and ahead (`weapon_select.c:85-99`), whatever the table says. **So a car whose own slot-0
weight is 0 (Hammerhead, Mr. Slam, Dark Tooth) almost never fires a ranged special it is given.** Other type reads:
Outlaw's slot toggle (`weapon_tick_init.c:69`), Hammerhead's ram (`ai_steps.c:1434, 1485`), Mr. Slam's stop 120
short (`:1668`), the Twister rule in `aiPickWeaponFromTable9` (`weapon_fire_specials.c:202`), the melee approach
(`ai_steps.c:730, 761, 1247`). Request 35's `portCarSpecialType` already redirects the melee, Outlaw, Hammerhead and
crush reads to a borrowed type; the weight tables, the Twister rule and Mr. Slam's stop stay the car's own.

## 6. The build (request 73, stages 1 and 2)

**Scope** (proposed by the decomp, 2026-10-28; the user can widen it):
- **Templates:** the 10 that work on any car (request 35's set: Sweet Tooth, Outlaw 2, Warthog, Mr. Grimm, Thumper,
  Spectre, Roadkill, Axel, Shadow, Minion) and eight pickup weapons: fire, homing and power missile, napalm, Mega
  Napalm, the freeze missile (stage 1), the ricochet bomb and the remote bomb (stage 2). Hammerhead, Grasshopper,
  Twister, Mr. Slam and Dark Tooth stay their own car's: a mod built on that car keeps it (a refused template is
  ignored). A machine-gun burst does not exist in the game, so it is not offered.
- **Numbers:** stage 1 has the ones set at fire time; stage 2 (6.6) adds the per-frame ones: the taser's hits, the
  flame's damage per frame, the burn, the freeze, Shadow's contact hit and detonation, the boomerang's turn-back. The
  tornado, the leap, the crush and the slam are the refused templates' (not offered).

### 6.1 Switch on

`--port-define TM2_PORT_SPECIAL_PROFILE`; needs `TM2_PORT_CAR_PROFILE` (so `TM2_PORT_CAR_NUMBERS` and `TM2_PORT_CARS`)
and `TM2_PORT_SPECIAL_AS` (so `TM2_PORT_CAR_SPECIALS` and `TM2_PORT_CAR_GROUPS`); `#error` otherwise. The port fills
`portSpecialProfile[m]` (by mod, like request 70's `portCarProfile[m]`) before each load, the same on every machine.
It is read only while the mod's `portCarProfile[m].valid` is set; everything else is retail's.

### 6.2 The record (`car_numbers.h` struct `TM2PortSpecialProfile`, 0x48 bytes)

| off | field | meaning |
|---|---|---|
| +0x00 | `special` (s16) | the template: 0 = the base car's own; a car type of the 10 (0x10, 0x30, 0x40, 0x50, 0x70, 0x80, 0x90, 0xB0, 0xD0, 0xE0); or a pickup: `TM2_SPECIAL_PICKUP_FIRE` 0x101, `_HOMING` 0x102, `_BOMB` 0x103, `_POWER` 0x104, `_NAPALM` 0x105, `_RICOCHET` 0x106, `_MEGA` 0x10A, `_FREEZE` 0x164 |
| +0x02 | `count` (u8) | entries of `num[]` used (at most 16) |
| +0x03 | `ai_ahead` (u8) | a computer car's slot-0 weight, percent, with its target ahead (0 = the template's table; a pickup template: 50) |
| +0x04 | `ai_behind` (u8) | the same with its target behind |
| +0x05 | `fire_chance` (u8) | request 87: a computer driver's fire chance, 1..100 (more is 100), 0 = the template's (section 6.7) |
| +0x08 | `num[16]` | {u8 field, u8 pad, s16 value}: the numbers below |

| field | id | applies to | retail | clamped to |
|---|---|---|---|---|
| `TM2_SPN_DAMAGE` | 1 | each projectile a fire makes; Axel's shockwave; stage 2: each of the taser's two hits, the flame's hit per frame, Shadow's strike on contact | per template (sections 2, 4); taser 10, flame 1, strike 25 | 0..8191 |
| `TM2_SPN_DAMAGE2` | 2 | a Minion's freeze missile (Minion's template or a Minion-based mod) | 7 | 0..8191 |
| `TM2_SPN_LIFE` | 3 | each projectile's record life; Thumper's flame emitter; Shadow's strike; the ricochet | 200 spiral / boomerang, 100 Warthog, 50 most, 80 ghost, 150 napalm; flame 50; strike 100; ricochet 400 | 1..8191 |
| `TM2_SPN_SPEED` | 4 | each projectile's start speed; Shadow's strike speed; the ricochet's | per template; strike 66; ricochet 100 | 1..150 |
| `TM2_SPN_TURN` | 5 | each projectile's turn rate | per template | 1..4095 (the spiral's 99 becomes 98) |
| `TM2_SPN_RANGE` | 6 | the taser's reach (D-units); Axel's radius | 80; 800 | 1..4000 |
| `TM2_SPN_HEIGHT` | 7 | Axel's height window | 200 | 1..4000 |
| `TM2_SPN_DURATION` | 8 | the taser's arc | 40 | 40..400 (its first hit comes at count 39, so a longer arc hits later) |
| `TM2_SPN_FREEZE` | 9 | stage 2: the freeze of every freeze missile the car fires (Minion's, the freeze template, the energy attack) | 300 ticks | 1..6000 |
| `TM2_SPN_BURN` | 10 | stage 2: the burn the car's flame or napalm sets on a car | 100 frames | 1..2000 |
| `TM2_SPN_BLAST` | 11 | stage 2: Shadow's detonation, the damage at its centre (25 more on a surface polygon; falls 1 per 4 D-units) | 25 | 0..8191 |
| `TM2_SPN_BLAST_RADIUS` | 12 | stage 2: its radius, D-units | 75 | 1..400 |
| `TM2_SPN_TURNBACK` | 13 | stage 2: the boomerang's turn-back distance, D-units | 300 | 1..4000 |

The numbers go to **whatever special the car fires**: its template, or its base's own when it has none (a mod on
Spectre with only `num[]` gets a Spectre ghost with its numbers). With a Gun Game level (request 35's
`portCarSpecialAsSet`) they go to that special too.

### 6.3 What the C does

- **The template** is applied as request 35's borrowed special: at the car's first start of the load,
  `weaponBlockInit` (`portCarSpecialAsBlockInit` -> `portSpecialApply`) sets `portCarSpecialAs[slot]` to it (making
  Shadow's strike or Axel's ring object for the slot, as `portCarSpecialAsSet` does), unless the port set a borrowed
  special first or the template's car file is not loaded (Sweet Tooth, Mr. Grimm, Spectre, Roadkill, Axel, Shadow:
  request 35's `portCarSpecialLoad` mask). So the fire switch, the AI's melee and type tests and the crush follow it.
  `max_specials` / `recharge_period` are the template's (section 1; a pickup keeps the base's), then request 70's
  `recharge` / `max_specials` when the profile sets them. `portCarSpecialAsSet(slot, -1, n)` now gives a mod car its
  template back (not its base's special).
- **A pickup template** fires through the level's launcher (`portSpecialFirePickup` -> `weaponLaunchProjectile`: its
  muzzle, aim, sound, retail numbers; no ammo, `held` or `selected` touched); Mega Napalm sets the weapons block's flag
  for the shot and gives it back; the charge is `weaponFireSelected`'s, as any special's.
- **The projectiles' numbers:** `weaponFireSpecial` notes the free projectile records before its switch
  (`portSpecialFireBegin`) and, after it, gives every record that became live its damage, life, speed and turn
  (`portSpecialFireEnd`). A record live before the fire is never touched.
- **The other numbers:** Axel's radius, height window and damage (`weaponFireSpecial` case 0xB0), Shadow's strike life
  and speed (case 0xD0), Thumper's flame life (`weaponFlameStart`), the taser's reach and arc (`weaponTaserTarget`),
  through `TM2_SPECIAL_NUM(slot, field, retail)` (retail's literal without the define).
- **The AI:** `aiCarSetupProfile` takes the template's weight tables and fire chance (a car type), so a mod on
  Hammerhead with Warthog's special fires it as a Warthog would; a pickup template, or a profile with `ai_ahead` /
  `ai_behind`, gets copies of the tables with those slot-0 weights (per mod, `portSpecialState`).

Four game functions change (`weaponFireSpecial`, `weaponFlameStart`, `weaponTaserTarget`, `aiCarSetupProfile`) and
11 port functions are added. Storage (`.port_bss`, after `portHiddenAreas`): `portSpecialProfile` (128 x 0x48 =
9,216 bytes, the port's) and `portSpecialState` (0xC08, the C's), both match state (rollback snapshot and hash yes).

### 6.4 Known limits

- A Warthog or Minion template launches its missiles from the firer's `gun[]` points, as any car's.
- Two AI type reads stay the base's (they test the car's own type, not the special): `aiPickWeaponFromTable9`'s Twister
  rule and Mr. Slam's stop 120 short. Only a Twister- or Mr. Slam-based mod with another template sees them.

- Stage 2's per-frame numbers find the firer from the projectile's, emitter's or strike's owner id, so they apply to
  every shot of that kind the car fires: a mod's freeze length also covers its freeze energy attack, its burn its napalm
  pickup, its `DAMAGE` LIKELY its lightning arcs (they share the arcs' hit code). A remote bomb's blast stays retail's.
- The remote bomb template shares the car's one bomb with the remote bomb pickup (the game keeps one per car): either
  button detonates it; detonating it with the pickup's button spends a pickup bomb, as retail's pickup does.

### 6.5 Checks

`tools/check_custom.py` check 13 (with the define): the 11 port functions in `.port_text`, no `rand()`, their callers;
in the MIPS interpreter, a made-up mod on Spectre: every field clamped to its limit and retail's value when unset or
without a valid profile; only the records a fire made take the numbers (a Minion freeze missile its `DAMAGE2`, the
spiral's 99 -> 98); `weaponBlockInit` applies Warthog's template with Warthog's ammo, or request 70's when set, a pickup
keeps Spectre's, Twister's is refused; `aiCarSetupProfile` takes Warthog's tables, and a pickup template gets slot-0
weight 50 over Spectre's own; the pickup fire calls the launcher with the right kind and the Mega flag. Stage 2:
the lookup by object id, Shadow's blast base (a remote bomb's stays retail's), the boomerang's charge to its thrower
(human and AI), the ricochet template's pool shot (owner, life, speed), the remote bomb template's place-then-detonate;
the stage 2 functions are called only by their hooks (`projectileHitTest`, `weaponFlameUpdate`,
`weaponProjectileSteer`, `weaponProjectileUpdate`, the strike update, `areaDamage`, `weaponArcsDraw`).

A link without the define fails check 13 (the negative control: `work/pv/r73_rb_P4`). Verification: full build
`work/build-20261028-031210-048373` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK from `.LIB` `f557c1e3…`:
unchanged); links `work/pv/r73_{rb,port,allh16,ha,sp}_{P4,P2}` (in `r73_allh16_P4`: `portSpecialProfile` 80954A18,
`portSpecialFireEnd` 8095EA4C); `check_all.py` 36 / 36. Not runtime-tested.

### 6.6 Stage 2 (2026-10-28)

- **The per-frame numbers**, each through a hook that finds the firer and is retail's literal without the define:
  - the taser's two hits (`weaponArcsDraw`, by the arc's `source`, the firer's car slot): `TM2_SPN_DAMAGE`;
  - Thumper's flame (`weaponFlameUpdate`, by the emitter's owner object id): its hit per frame (`DAMAGE`) and the
    burn it sets (`BURN`: the burn ends when `burn_time` reaches 100, so it starts at 100 - the length);
  - the freeze and napalm hits (`projectileHitTest`, by the shot's owner id): `FREEZE` (also through `TM2_PORT_BALANCE`'s
    diminishing returns) and `BURN`;
  - Shadow's strike on contact (the strike update, by its owner id): `DAMAGE`; its detonation (`areaDamage` with big
    0, by the firer's slot): `BLAST` and `BLAST_RADIUS`. A remote bomb's blast (big 1) is retail's;
  - the boomerang's turn-back (`weaponProjectileUpdate`, `weaponProjectileSteer`, by its owner id): `TURNBACK`.
- **The boomerang's charge** goes back to its thrower when caught (`portSpecialBoomerangCaught`, human or AI), not to
  the first human Roadkill.
- **The ricochet bomb template** (0x106) fires as `weaponFireSelected`'s case 6 does (its pool, 40 above the car,
  Dark Tooth's look by the car's own type), with the car's `LIFE` (400) and `SPEED` (100).
- **The remote bomb template** (0x103): a press places the car's bomb (the charge given back, as the pickup's
  placement), the next press detonates it (and spends the charge); `drop_armed` follows, so the pickup's own button
  sees the same bomb.
- Game functions changed by stage 2: `weaponArcsDraw`, `weaponFlameUpdate`, `projectileHitTest`, `areaDamage`,
  `weaponProjectileUpdate`, `weaponProjectileSteer`, and the strike update (`weaponShadowStrikeUpdateOne`, the
  per-car body under `TM2_PORT_CAR_SPECIALS`). Port functions: 7 more (18 in all).
- `tools/check_balance.py` follows: `FREEZE_TICKS` counts as the retail 300 freeze write, and `portSpecialBomb` may ask
  `portBalanceBombRefused` (the template's placement, as the pickup's).
- Verification: full build `work/build-20261028-043048-598393` (retail hashes unchanged); links
  `work/pv/r73b_{rb,port,allh16,ha,sp}_{P4,P2}` (in `r73b_allh16_P4`: `portSpecialProfile` 80954A18,
  `portSpecialNumById` 8095EAD0); `check_all.py` 36 / 36; the negative control (`r73b_rb_P4`) fails check 13. Not
  runtime-tested.

### 6.7 Request 87 (2026-10-30): a computer driver's fire chance per mod

Port request 87 part 2: `aiCarSetupProfile` (800D3C7C) sets `weapons.fire_chance` by type (its template's with
one: Sweet Tooth, Minion and Dark Tooth 100, the others `3 * level + 15 * difficulty + 17` .. `+ 30`), then adds
+45 on levels 5 and 6 and +15 in mode 2, sets 100 in mode 3 with one computer car, and clamps to 100. Built: byte 5
of `portSpecialProfile[m]` (`fire_chance`, was `pad[0]`), when not 0, replaces the type's value in
`portSpecialAiTables` (called after the switch), before the bonuses and the clamp; above 100 it is 100. Match state
(the profile, hashed, as the rest). Checked by reading; not run in game.
