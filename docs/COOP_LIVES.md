# Lives per player in co-op, respawning one car, playing on alone

Answers item 10 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(Vanilla Plus "Lives per level": lives per player, one car respawns while
the other plays on, a player out of lives watches). Static analysis only:
the C in `src/semantic`, checked against the retail disassembly
(`bin/SCUS_94306R.EXE`) and the disc (`cdimage/`, read only). Nothing was
run. Addresses are retail.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code, not traced to the end or not seen running.

Human car record offsets used below (`humanCars`, `D_80187B10 + i*0x814`):

| Offset | Field |
|---|---|
| +0x00 | player index (s16); also the view index its camera writes to |
| +0x0C / +0x14 | camera rotation / position |
| +0x90 | car type (0x10..0xF0) |
| +0x9C | state block: +0x9C dead, +0xA9 frozen, +0xAA **energy shield**, +0xB6 burning, +0xC0 sinking |
| +0x10C | camera view (2..7) |
| +0x14C | stats block: +0x166 freeze timer, +0x186 max HP, +0x1EC contact probe on, +0x1F0 HP, +0x1F4 **takes damage** |
| +0x6FC / +0x708 | position / rotation |
| +0x7C8 | weapons block: +0x7CC invisible, +0x7F8 energy, +0x7FA turbo, +0x7FC invisibility timer |

Human i's world object is `D_80180FEC[i]` (humanCarObjects) (= `D_80187AD4[D_80180FF4[i]]` (carSlotObjects) (humanCarObjectSlots)).
Object word +8 is its draw-and-hit flag (section 3.1).

## Short answer

- **Today:** a human death schedules result 1 (`humanCarDamageUpdate`). Any
  scheduled event ends `rtMainLoop`. The runner takes one life from +20 and
  runs the next round. The round start restarts **every human car** and
  wipes **every projectile, mine, fire, shield and effect** in the arena.
  Computer cars, map pickups and arena objects are not reset.
- **One car can come back alone.** Do not let result 1 be scheduled, and
  do not call `carRestart` (it wipes the global weapon pools). Call
  `sub_800FD6C4(car)` (humanCarPlaceAtStart) (start node, full health, clean state) and the
  car's own resets from `carRestart`, then set the shield flag +0xAA
  for a few seconds. Section 2 has the list.
- **A player out of lives:** clear its object's word +8 (not drawn, not
  hit), skip its update, keep AI and homing weapons off it (three loops),
  and fix the round-end test at 8011B5CC, which only looks at player 1.
- **Their half of the screen:** blank, a view of the partner, or one full
  view for the survivor. All three are possible; the full view needs the
  survivor's camera redirected to view 0 when the survivor is player 2.
- **HUD:** the split HUDs print "Lives: N" only for player 1, from +20.
  Each half can print its own (+20 + player). The digits the split code
  calls "lives" are really the turbo meter.
- **Sound:** the title and battles use the same, only sound bank:
  `SCREEN\UACORE.VAB`. Sample 24 is sound id **24** (the pickup jingle).
  Sample 9 is sound id **22** (the invisibility shimmer). Both are in
  sound RAM during every battle.
- **Spectating:** the camera can follow another car (view 7), but only a
  computer car. Following the partner needs a small port-side camera.

## 1. Losing a life today (CONFIRMED)

### 1.1 The chain

| Step | Where | What |
|---|---|---|
| 1 | `humanCarDamageUpdate` 8011B89C (`car_damage_human.c`), called by `worldUpdate` | HP <= 0 and not dead: dead +0x9C = 1 (8011BB0C), `carResetWeapons[i]` = 1 (8011BB18), `sub_8011BE10(car, 1)` (carDestroyed) (8011BB1C), `D_80180F94` (roundOverFlag) = 1 (8011BB2C) |
| 2 | same, 8011BBB4 | `rtScheduleEvent(1, 420)`, or 210 when sinking (+0xC0). Mode 4 schedules 3 or 4 instead |
| 3 | `sub_8011BE10` (carDestroyed) (`car_destroyed.c`) | the death itself: wreck sound and rumble, explosion, or the car hidden if it sank (levels 6 and 8). Frozen +0xA9 = 1 with timer 540, target link cleared |
| 4 | `sub_800D18A0` (rtUpdateScheduledEvents) (`rt_main.c`) | counts every event down by the ticks per frame (3 at 20 fps, so 420 = 7 s). **The first event that fires ends `rtMainLoop`** with its value |
| 5 | runner `screenCheckMidGameOptions` (sessionRun) 8012C260..8012C26C | `--settings[+20]`; at 0 "GAME OVER" (8012C2B4) and the session ends |
| 6 | runner, next loop | `sub_80128DB4` (roundStartMusic) -> `roundStart` -> `humanCarsRestart` |

The round-end test in `worldUpdate` (8011B5CC..8011B62C) also waits on
player 1: it needs `aiCarsRemaining <= 0`, mode != 4,
`humanCars[0]` dead (+0x9C, loaded at 8011B5D0) = 0 and round-over
`D_80180FB0` (roundWonFlag) = 0. Player 2's state is never read there.

### 1.2 What the restart resets

| What | Reset? | By |
|---|---|---|
| Every human car: start node, heading, full HP (max HP = the type's value + 20) | yes | `humanCarsRestart` -> `carRestart` -> `sub_800FD6C4` (humanCarPlaceAtStart) |
| state block (dead, frozen, shield, burning...) | yes | `sub_800FD6C4` (humanCarPlaceAtStart) -> `sub_800FDC20(car + 0x9C)` (carStateFlagsClear) |
| per-life stats (freeze, fire, killer, last attacker, shield halving) | yes | `sub_800FE15C` (carStatsLoad) |
| human pickup weapons | only the dead car's (`carResetWeapons[i]` = 1); a partner keeps them, specials = 2, energy = 1000 | `carRestart` (`sub_800FDCAC` (weaponBlockInit)) |
| invisibility, controls, camera offsets, pad map | yes | `carRestart` |
| displayed speed, camera view (1 player only) | yes | `humanCarsRestart` (`sub_8011CBB0` (hudResetSpeed), `sub_800E7380` (carSetViewMode)) |
| 25 weapon-slot records (mines, remote bombs) and their objects | yes, **global** | `sub_80109440` (weaponDropsReset) (from `carRestart`) |
| fire areas, lightning slots, emitters, 10 effect shots, 4 shield records, the shield volume, strike, held-pickup records | yes, **global** | `sub_80108CF0` (weaponsRoundReset) (from `carRestart` and `roundStart`) |
| the 16 projectile records (missiles, napalm, freeze, specials) | yes, **global** | `sub_800D79C4` (projectilesReset) (from `carRestart`) |
| 150 effect sprites, 20 records at `D_80184E90` (fxImpactParticlesLifeField) | yes, **global** | `sub_80112820` (fxSpritesReset) (from `carRestart` and the runner) |
| "Enemies: N" message | yes | `sub_800FBB6C` (hudMessageReset) |
| the 16 per-group model state objects (LIKELY the car bodies' damage looks) | yes, global | `sub_80124B80` (carFreezeResetAll) (`roundStart`) |
| debris and effect slots | yes, global | `sub_8014B3E0` (debrisClear) (`roundStart`) |
| round over `D_80180FB0` (roundWonFlag), death flag `D_80180F94` (roundOverFlag), `rand` seed | yes | `roundStart` |
| level 8 route patch | when Dark Tooth is AI car 0 | `sub_800F6080` (aiRoutePatchBossLevel) (`roundStart`) |
| computer cars (HP, position, AI state, wrecks) | **no** | nothing touches `aiCars` outside attract mode. LIKELY: destroyed ones stay out |
| map pickups (`D_80183F7C` (pickupSlots), 51) | **no** | they come back on their own timer (`fx_sprite_spawn.c`). LIKELY |
| arena objects, start-node "taken" flags, timers | **no** | only the loader (`arenaSetup`) resets them |

Note: `carRestart` runs the global wipes once per human car, so twice in
co-op.

## 2. Respawning one car without a restart

### 2.1 Stop the restart (CONFIRMED)

The port replaces step 2 of 1.1 for modes 1 and 2:
- do not call `rtScheduleEvent(1, ...)` for a death that still has a life.
  Any event would end the round. Keep the respawn delay in a port counter,
  for example the same 420 / 210 ticks;
- take the life from that player's byte (+20 + i, section 4);
- clear `D_80180F94` (roundOverFlag) again after the respawn (it is set at the death; its
  only reader refuses invisibility at 80104A80).

### 2.2 What to call, and what not

| Call | Per car or global | Use it? |
|---|---|---|
| `sub_800FD6C4(car)` (humanCarPlaceAtStart) | per car | **yes**: start node, heading, state block, motion, matrices, stats, HP = max, wheels, box shape, camera at the car |
| `sub_800FDCAC(car, car + 0x7C8, 1)` (weaponBlockInit) | per car | yes, to clear the pickup weapons (retail does it for a dead car); or set +0x7D6 (specials) = 2 and +0x7F8 (energy) = 1000 only, as for a partner |
| the per-car stores of `carRestart` (800FD550) | per car | yes: +0x90 = type, +0x14C = 1, +0x7CC = 0, +0x7FC = 0, +0xF1 = +0xF2 = 0, +0xF4 = 0, `sub_800FF764(car)` (viewCameraOffsetsInit), +0xC8 = 0, `sub_800E4D8C(car, 2, 2, 0, 1)` (carShakeStart), +0x8C = 0, `sub_800EC938(car + 0xD0)` (aiClearControls), `sub_8011D6A4(car, PadGetConfig(player))` (carApplyPadConfig), +0x155..+0x158 = 0, +0x14D = 0, +0x235 = 1, and the object's light env |
| `sub_8011CBB0(i)` (hudResetSpeed) | per car | yes (displayed speed) |
| `sub_800E7380(car, view)` (carSetViewMode) | per car | yes in one view; skip in split screen, as `humanCarsRestart` does |
| `carRestart` 800FD550 | **global inside** | **no**: it calls `sub_80109440` (weaponDropsReset), `sub_80108CF0` (weaponsRoundReset), `sub_800D79C4` (projectilesReset), `sub_80112820` (fxSpritesReset) and `sub_800FBB6C` (hudMessageReset) |
| `humanCarsRestart`, `roundStart`, `sub_80128DB4` (roundStartMusic) | global | **no** |

After `sub_800FD6C4` (humanCarPlaceAtStart), the next `humanCarDamageUpdate` sees HP above 40 %
and puts the undamaged model back (`sub_80124718(type, 0, ...)` (carSetDamageStage),
`sub_80114E10` (carBodyObjectShow)) and stops the smoke. LIKELY.

`carRestart` sets stats +0 (+0x14C) = 1 before `sub_800FE15C` (carStatsLoad), so the
"takes damage" word +0x1F4 is **not** reset by a restart: the no-damage
cheat survives a life. CONFIRMED.

### 2.3 Where it appears: start nodes (CONFIRMED)

`sub_800FD6C4` (humanCarPlaceAtStart) takes its node from `sub_800F636C(count)` (startNodePickRandom): a list of up to
15 start nodes at `D_801B2B28` (startNodeList) (node index s16 [15], then "taken" bytes
[15] at `D_801B2B46`, count at `D_801B2B55` (startNodeCount); level 3 uses 9). It starts at
a random entry and takes the first one not taken this arena, else a
random one. The taken bytes are cleared only by `arenaSetup`
(`sub_801019DC` (startNodesClearUsed)). After the first round every node may be taken, so
retail restarts land on a random node. That node may be occupied.

To choose a node clear of the others: compute, for each list entry, the
distance from its node (`D_801B1DE8[n]` (aiRouteNodes), 24 bytes: map x and y as s32 at +0 and +4, height s16 at +8) to every
live car; mark every other entry taken (`D_801B2B46[k]` = 1); call
`sub_800FD6C4` (humanCarPlaceAtStart); restore the bytes. The function then has only the chosen
entry to take.

### 2.4 Bring the object back

The car's world object stays visible as a wreck after the death, unless
it sank (`sub_8011BE10` (carDestroyed) cleared word +8 then). Set word +8 = 1
(`sub_80149604(object, 1)` (csSetDrawEnable)) at the respawn. Also set the contact probe
+0x1EC = 1 if the port cleared it (section 3).

## 3. A player out of lives

### 3.1 Taking the car out of play

| Need | How | Conf. |
|---|---|---|
| not drawn | object word +8 = 0 (`sub_80149604(D_80180FEC[i], 0)` (humanCarObjects) (csSetDrawEnable)). `renderScene` skips objects whose +8 is 0 | CONFIRMED |
| not hit by other cars | same flag: the hit pass `sub_8014FAB0` (hdpPassRun) skips an object when (+8 & +0xC) = 0 (8014FC50..8014FC64). The invisibility code uses this too (`sub_8010DE54` (weaponInvisibilityUpdate)) | CONFIRMED |
| no own collision or pickups | +0x1EC = 0: `sub_8011E220` (carsPhysicsUpdate) then skips its probe and pickups | CONFIRMED |
| no update | skip its body in the human loop of `worldUpdate` (`sub_8011C400` (humanCarUpdate), the taunts, `sub_8011DB74` (carSoundUpdate), `humanCarDamageUpdate`). Do not use `D_80180FEC[i]` (humanCarObjects) = 0: the loop **breaks** on it, so player 2 would stop too | CONFIRMED |
| no view hotkeys | already: `carViewHotkeys` returns while dead (+0x9C) is set | CONFIRMED |
| not chased by AI | `sub_8011CF0C` (aiGetTargetCar) (every AI target pick) and `sub_800EC8A0` (carNearestToHuman) call `sub_8011C8E4` (humanCarNearest), the nearest human with no life check. Make it skip out players | CONFIRMED |
| not homed or locked on | the human loops of `weapon_leap_homing_init.cpp` (missile lock) and `weapon_slot_target.c` have no HP test (the AI loops do). Skip out players there | CONFIRMED |
| not hit by splash, lightning, shield push | `areaDamage`, `sub_80107B00` (weaponLightningScan), `sub_8010E040` (weaponTornadoUpdate) go by distance. Damage to a dead car does nothing (dead stays 1), but pushes still move it. Simplest: also move the record and object far below the arena | LIKELY |
| the dead flag | keep +0x9C = 1 | CONFIRMED |

### 3.2 Level won, game over

- **Win when one player remains:** the test at 8011B5CC requires player 1
  alive. Replace `!humanCars[0].dead` with "at least one human not out".
  Retail's reason for the test (no win while dying) still holds per
  player: require the surviving human not dead. CONFIRMED (the test);
  the rule is the port's.
- **Game over when both are out:** schedule result 1 with +20 = 1, so the
  runner's `--lives == 0` gives "GAME OVER" and result 1 as today
  (`TOURNAMENT_FLOW.md` 3). Or call the same text `sub_8012B5FC` (screenShowMessageLines) from the
  port's own runner. CONFIRMED (runner code).
- After either result the runner sets the weapon flags. On a win it keeps
  weapons (flags 0); the out player's record still holds its last weapons
  unless the port resets them (`carResetWeapons[i]` = 1).

### 3.3 Their half of the screen

Split mode is `D_801809E4` (viewSplitScreenMode) (`sub_800D1C18` (viewGetSplitMode) / `sub_800D1C24` (viewSetSplitMode)): 0 one view,
1 vertical (view modes 5 and 6), 2 horizontal (2 and 3). `rtMainLoop`
renders view 1 only when it is not 0. Each car's camera writes to view
`car->player` (`sub_801564D8` (viewSetRotation), `sub_80156528` (viewSetPosition) in `sub_80102BE8` (viewCarCameraUpdate)).

| Option | How | Conf. |
|---|---|---|
| **Blank** | keep the split; in the port's `rtMainLoop` skip `renderScene` (and the HUD) for that view and draw a "OUT" text instead | LIKELY |
| **Watch the partner** | keep the split; give the out player's view a chase camera on the partner (section 6) | LIKELY |
| **One full view** | `sub_800D1C24(0)` (viewSetSplitMode); `sub_80119CA0(sub_800D19E0(), 1, 0)` (viewCreateWithCamera) (renderGetView0Frames); for the mirror `rtSetRearView(1)` and `sub_80119CA0(sub_800D1B84(), 4, 1)` (renderGetMirrorFrames) (as `arenaSetup`); `sub_800FB0B0()` (hudInitPanels) (HUD records). The chase camera then uses its 1-player offsets by itself | CONFIRMED (calls) |

For one full view when the survivor is **player 2** (index 1): view 0 is
the screen, but player 2's camera writes to view 1 (the mirror in one
view). After `sub_80102BE8(&humanCars[1], 1)` (viewCarCameraUpdate) copy its result to view 0
(`sub_801564D8(car + 0x0C, 0)` (viewSetRotation), `sub_80156528(car + 0x14, 0)` (viewSetPosition)), set the
view target with `sub_801563BC(pos, 0)` (viewSetFollowObjPos), call
`hudDrawSingleView(1, ...)` instead of `(0, ...)`, and keep the mirror
off (it would show player 2's camera). `worldUpdate` updates a camera
only for `D_80180FA8` (viewMainPlayer) (player 0) or in split screen: set `D_80180FA8` = 1.
The view toggle hotkey (`carViewHotkeys`) would switch back to a split;
block it. LIKELY.

## 4. HUD lives per player

| HUD | Lives text | Conf. |
|---|---|---|
| one view `hudDrawSingleView(player)` 800F8370 | "Lives: N" at (50, 212) from `sub_80129B20(player)` (gameGetLives) = settings +20 + player. Already per player | CONFIRMED |
| vertical split `hudDrawSplitVertical` 800F9084 | mode 2 and player 0 only: `sub_80129B20(0)` (gameGetLives) at (11, 222), font 2, sprites `D_80182160` (hudSplitVertLivesMode) / `D_8018216C` (hudSplitVertLivesSprites) | CONFIRMED |
| horizontal split `hudDrawSplitHorizontal` 800F9D48 | same, at (11, 107), sprites `D_80182234` (hudSplitHorzLivesMode) / `D_80182240` (hudSplitHorzLivesSprites) | CONFIRMED |

The text is `D_80180AC4` (hudLivesText) ("Lives:  "), with byte 7 = lives + 47: it shows
the **spare** lives (lives - 1). Each player's HUD is drawn with its own
view's draw environment, and both players use the same coordinates (the
health bars do), so the same position works in player 2's half. For
player 2: drop the `player == 0` test and pass `sub_80129B20(player)` (gameGetLives).
The sprite buffer is one per layout; give player 2 its own (on the PS1
the GPU could still be reading the first). LIKELY.

The split code's fields `lives_icon` / `lives` (+0x6B8, +0x6D8) and the
"lap" fields of `hud_main.c` show weapons +0x32 / 10, the **turbo meter**
(`GAME_EVENTS.md` 13.2), not lives. CONFIRMED.

### Where to keep the counts

Settings `D_8016475C` (sessionSettings), 48 bytes:

| Byte | Today | For the port |
|---|---|---|
| +20 | lives; the runner decrements only this | player 1 |
| +21 | set equal to +20 by the 2-player menu (6), never read | player 2 |
| +22 | cleared to 0xFF by `settingsClearOpponentCars` (it clears +10..+22) | player 3 (set after any clear) |
| +23 | not written by the clears; no reader found | player 4 |
| +24 | level (s32) | |

`settingsSetLives(s, i, n)` writes +20 + i and clamps to 6. `sub_8012CB9C` (sessionGetLives)
reads +20 + i. LIKELY (+22/+23 free: no reader found in the C).

## 5. Invulnerability and the respawn sound

### 5.1 What makes a car immune (CONFIRMED)

`sub_800E5628` (carApplyDamage) (`car_damage.c`) in order:

| Flag | Effect | Notes |
|---|---|---|
| energy shield, state +0x0E (human **+0xAA**) | returns before any damage, flash or spin-out; puts out fire | heavy type-1 hits (>= 11) still knock the car back (that comes first). Also blocks freezing (`weapon_slam_swing.c`) and lava |
| `D_80180FBC` (gameAttractDemoMode) (attract mode, `sub_8011EDC4` (gameIsAttractDemo)) | no HP loss for anyone | global; also hides the HUD. Not usable |
| round over `D_80180FB0` (roundWonFlag) (`sub_8011E214` (roundIsWon)) | no damage at all except to Dark Tooth | global |
| "takes damage" stats +0xA8 (human **+0x1F4**) = 0 | no HP loss; the flash, spin-out and knock-back still happen; burning does not drain | the cheat code 0x12344321 toggles it (`car_input.c`); `car_tick.c` also |
| stats +0x3C | halves damage | per car type |

Falls (HP = -1), deep water and the crush drain bypass the damage
function, so they still kill (`GAME_EVENTS.md` 1.4).

**Recommended:** set +0xAA = 1 after `sub_800FD6C4` (humanCarPlaceAtStart) (which clears it) and
clear it after N frames in the port. Two catches: a real energy shield
(attack 103) is refused while +0xAA is set, but still costs 400 energy;
and when a real shield record of that player ends, `sub_8010DD30` (weaponShieldsUpdate) clears
+0xAA.

**With the bubble:** do what attack 103 does (`sub_801046FC` (weaponFireEnergyAttack),
`weapon_special.c`): `slot = sub_80109F54()` (weaponShieldSlotFind); if >= 0: +0xAA = 1,
`D_80184A68.slots[slot]` (weaponShieldPool) word +8 = 1, record `owner` = player,
`lookup` = `sub_8011CB50(player)` (humanCarObject), `life` = 60, `obj` = the slot object.
`sub_8010DD30` (weaponShieldsUpdate) (called each world step) moves the bubble with the car,
and at 0 hides it and clears +0xAA. 60 steps = 3 s at 20 Hz. Only 4
slots exist.

### 5.2 The sound bank (CONFIRMED)

- The disc has **one** sound bank: `/SCREEN/UACORE.VAB` (LBA 1289, 556,256
  bytes). It is the only VAB the game opens: boot `sub_80128C7C` (shellLoadUaCore) loads it
  to 0x80010000 and `sub_801283F8` (soundVabOpenRetry) opens it (`SsVabOpen`, the only
  caller). Nothing closes it. The title has no other bank; its music is
  CD audio.
- Header: 67 programs (0..66), 124 tones, 43 samples (VAGs).
- Sound RAM: the body starts at 0x1010. VAG n (1-based) is at 0x1010 +
  the sum of the earlier sizes. The user's addresses match: 0x025A4 * 8 =
  0x12D20 is **VAG 10** (4,672 bytes), 0x07DD0 * 8 = 0x3EE80 is **VAG 25**
  (5,456 bytes). "Samples 9 and 24" count from 0.
- A sound id is a VAB program: `D_801640DC[id]` (sndEffectTable) (20 bytes per id) holds
  program = id.

| Sample | VAG | Programs that play it | Sound id to use | Used in battle by |
|---|---|---|---|---|
| 9 | 10 | 22 (only tone), 16 (tone 2 of 3), 45 (tone 1 of 8) | **22** | the invisibility shimmer, `sub_8010DE54` (weaponInvisibilityUpdate) (call 8010DFE0) |
| 24 | 25 | 24 (only tone) | **24** | the pickup jingle, `pickupApply` (8010B4BC); secret codes (800DEF98, 800DF324); 10 pedestrians (80126DA4) |

Both are therefore loaded in every battle. To play one:
`sub_800D3334(24, 0, 0, 99)` (soundPlay) (as the secret-code chime does), or with
the pickup's arguments `sub_800D3334(0x18, PadChkMtap(), sub_800D35A0(), 99)` (soundGetListenerPan) (soundPlay).
The two middle arguments are the split-screen side and volume values the
game passes for the car's screen. LIKELY (their meaning).

## 6. Spectating

### 6.1 What the game has (CONFIRMED)

- **View 7** (`sub_80102CC4` (viewChaseCamera) chase camera, `camera.c`) and **view 5**
  (fixed camera, `sub_8010353C` (viewFixedCameraTrack)) follow `sub_8011D128()` (aiGetWatchedCar) =
  `aiCars[D_80180FAC]` (viewWatchedAiCar) instead of the car. `sub_800E7380(car, 7)` (carSetViewMode) switches
  to it. The attract demo uses it: `roundStart` puts player 1 in view 7,
  and `sub_8011EDD0` (demoWatchNextAiCar) steps `D_80180FAC` (viewWatchedAiCar) to the next computer car every
  301 / 601 ticks.
- The target is read with the **computer-car layout** (position +0x73C,
  rotation +0x748, state +0x6C, stats +0x53E/+0x542/+0x54C). A human car
  has position +0x6FC, so view 7 cannot follow the partner as it is.
- In split screen view 7 ignores its own offset and uses the split
  offsets.
- The view's own model (sky) follows `sub_801563BC(pos, view)` (viewSetFollowObjPos); in attract
  mode `worldUpdate` passes the watched car's x, y.
- HUD in view 7: speed from the watched car (`sub_8011CD08` (carGetDisplaySpeed)); everything
  else (health, weapons, turbo, radar, lives) is still the player's own
  record. The attract demo hides the HUD (`rtMainLoop` skips it while
  `sub_8011EDC4()` (gameIsAttractDemo)).

### 6.2 Watching the partner (LIKELY)

A port-side copy of `sub_80102CC4` (viewChaseCamera), with the target taken from
`humanCars[partner]` (position +0x6FC, rotation +0x708, state +0x9C) and
the camera fields of the out player's record (+0x0C, +0x14, +0x2C,
+0x38, offsets +0x44..). Call it for the out player in place of
`sub_80102BE8` (viewCarCameraUpdate); it ends with `sub_801564D8` (viewSetRotation) / `sub_80156528` (viewSetPosition) for the out
player's view. Also call `sub_801563BC(partner pos, out view)` (viewSetFollowObjPos). A
computer car can be watched with the retail view 7 by setting
`D_80180FAC` (viewWatchedAiCar) and `sub_800E7380(car, 7)` (carSetViewMode).

HUD for the watching half: the split HUD reads its numbers from
`sub_8011CEE8(player)` (humanCarGet). To show the partner's numbers, the port passes
the partner's record for the data and the out player's index for the
primitive record `D_801828F0[player]` (hudPlayerRecords) and its OT. Or it draws only a
"SPECTATING" line and the lives.

## 7. Three or four players

| Limit | Where | Conf. |
|---|---|---|
| 2 human records, then `D_80188B38` (aiCarObjects) (AI objects) and `aiCars` right after | `humanCars[2]` | CONFIRMED |
| split modes 0..2; view-mode rows 0..8 (no quarter views) | `viewModeTable` 8017EC44, `arenaSetup` | CONFIRMED |
| 2 views and camera objects (`viewCameraObjects`, `viewModes`, `D_80181024` (viewCameraObjectTable)) | `view_system.c` | CONFIRMED |
| 2 HUD records `D_801828F0[2]` (hudPlayerRecords), OTs `D_801828D0[2][2][2]` (hudOrderTables) | `hud_main.c` | CONFIRMED |
| lives bytes +20..+23 | section 4 | LIKELY |
| `carResetWeapons` (u8 at 80180FA0; the next variable is at 80180FA4) | room for 4 | LIKELY |
| taunts and AI sounds name players 0 and 1 directly | `worldUpdate` | CONFIRMED |

The rules of this file carry over: per-player lives, `sub_800FD6C4` (humanCarPlaceAtStart) per
car, out players hidden by object +8 and skipped in `sub_8011C8E4` (humanCarNearest), the
homing and lock-on loops, and the round-end test written as "any human
still in". The spectator camera of section 6.2 takes any live human as
its target.
