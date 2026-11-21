# Game events: kills, specials, pedestrians, air time, falls, speed, wins

Answers item 7 of the PC port's `docs/DECOMP_REQUESTS.md` ("who killed whom,
and with what"), for profile feats and an online kill feed. Static analysis
only: the semantic C in `src/semantic`, with every hook address below
checked in the retail disassembly (`bin/SCUS_94306R.EXE`). Tags:
CONFIRMED (seen in code, use is clear), LIKELY (inferred from how the code
uses it), GUESS.

Short version:
- The victim record keeps the **killer** (byte, car slot) and the **last
  attacker** (byte, car slot). Nothing in the game reads the killer byte: the
  game keeps no score and no kill counts.
- The damage function does not know the **weapon**. The port has to note the
  weapon at the call site (table in section 2).
- Falls, drowning, lava and the Hammerhead crush drain take HP directly and
  record no killer.
- Sections 9 to 15 (added for the portrait unlocks, `PORTRAIT_UNLOCKS.md`
  in the port): which weapon at the killing blow, self-damage, deaths on
  fire, freezes, pickups, the turbo meter, pedestrians by id (and the
  Antarctica Santa), and the Eiffel Tower.

## 0. How cars are identified

Damage code uses three different numbers for a car. Mixing them up gives
wrong kill credit.

| Number | Human car | AI car | Decode |
|---|---|---|---|
| Record | `D_80187B10 + i*0x814` (humanCars), i = player 0..1 | `D_80188B5C + i*0x854` (aiCars), i = 0..8 | `sub_8011CEE8(i)` (humanCarGet) / `sub_8011D0EC(i)` (aiCarGet) |
| Index in its array | record +0x00 (s16, player) | record +0x04 (s16) | |
| **Slot** (the level's car-object entry) | record +0x04 (s16) | record +0x06 (s16) | `D_80187A20[slot]` (carSlotKinds) = 0 human / 1 AI, `D_80187A5C[slot]` (carSlotIndices) = index; `sub_8011CFD0(slot, &human)` (carFromSlot) returns the record |
| **Object id** (world object +0x118) | i + 1 | i + 50 | `sub_8011D2C8(id, &human, &index)` (carFindById); `sub_8011D280(index, human)` (carObjectId) goes back |
| Car type | record +0x90 (s32) | record +0x30 (s32) | 0x10..0xF0, below |

The slot is set by `sub_8011A3FC` (carBindHumanSlot) (human, `scene_init.c`) and
`sub_8011A5CC` (carBindAiSlot) (AI, `car_parts_spin.c`); the object id is written there too
(`slot + 1`, `slot + 50`). Slot 0 is a real car, so an attacker of 0 is not
"nobody"; only -1 (or any negative) is. Other object ids: 500..5999 are
breakables, 6000 + i are pedestrians (section 4).

Car types (`docs/CAR_STATS.md`): 0x10 Sweet Tooth, 0x20 Hammerhead, 0x30
Outlaw 2, 0x40 Warthog, 0x50 Mr. Grimm, 0x60 Grasshopper, 0x70 Thumper,
0x80 Spectre, 0x90 Roadkill, 0xA0 Twister, 0xB0 Axel, 0xC0 Mr. Slam, 0xD0
Shadow, 0xE0 Minion, 0xF0 Dark Tooth. (The car-select index in the settings
struct is a different numbering, see `docs/UNLOCKS.md`.)

Blocks inside a car record (offsets used below):

| Block | Human | AI |
|---|---|---|
| state (flags) | +0x9C | +0x6C |
| stats | +0x14C | +0x4FC |
| motion | +0x6DC | +0x71C |
| weapons | +0x7C8 | +0x808 |

Level numbers (settings +24, `sub_80129AF8` (gameGetLevel)), from the retail jump table of
`sub_8012ACB8` (levelDisplayName) at 800CFB88: 1 Los Angeles, 2 Moscow, 3 Paris, 4 Amazonia,
5 New York, 6 Antarctica, 7 Holland, 8 Hong Kong, 10 Florida (Suicide
Swamp), 11 Cyburbia, 12 Roof Tops. (The `case` labels in
`car_parts_spin.c` name the strings in file order; the retail table maps
them differently.)

## 1. A car is destroyed

### 1.1 The damage function: `sub_800E5628` (carApplyDamage) (`car_damage.c`)

`sub_800E5628(target, amount, dir, type, attacker)` (carApplyDamage):
- `a0` target = victim's **object id**;
- `a1` amount;
- `a2` direction (may be 0);
- `a3` type: 0 = collision or ram, 1 = weapon hit (all hits that go through
  `sub_800DC65C` (weaponApplyHit)), 2 = splash or direct special damage;
- 5th argument (stack, s16) attacker = attacker's **slot**, or -1 for none.

The credit fields, in the stats block (CONFIRMED, stores at 800E5AB8 and
800E5BEC are `sb`):

| Field | Stats offset | Human | AI | Written |
|---|---|---|---|---|
| **Killer** (s8, slot) | +0x07 | +0x153 | +0x503 | at the killing blow, if attacker >= 0 |
| **Last attacker** (s8, slot) | +0x08 | +0x154 | +0x504 | on every hit while HP > 0, if attacker >= 0 |
| HP (s32) | +0xA4 | +0x1F0 | +0x5A0 | |
| Max HP (s16) | +0x3A | +0x186 | +0x536 | |

- **Reset:** both are set to -1 at every (re)start of a car by
  `sub_800FE15C` (carStatsLoad) (`car_stats.c`).
- **Last attacker reader:** the AI step `sub_800E9E20` (`ai_steps.c`) reads it
  on its target. If the target's last attacker is that AI car, it clears it
  to -1. So the last attacker is not reliable for kills.
- **Killer reader:** a scan of every byte load at offset 7 and of the
  absolute addresses found **no reader** of the killer byte.

**How the game itself credits a kill:** it does not. At the killing blow it
calls `sub_800FBBC4(sub_8011E16C(), 0)` (aiCarsCountAlive) (hudMessagePost), which only queues the HUD message
for the number of AI cars still alive ("Enemies: N", 0 = "Enemies
Toasted!"; strings at `D_8016286C` (hudMessageStrs)). `D_80181020` (aiCarsRemaining) (AI cars alive) ends the
round. There is no score and no kill counter per car. CONFIRMED.

Rules inside the damage function that matter for credit (CONFIRMED):
- **Small hits:** a type 0 hit (collision or ram) of less than 5 cannot kill; the HP is given back.
- **AI cars need an attacker:** AI cars lose HP only when attacker >= 0. Hits with attacker -1 (walls, hazards) never hurt them.
- **AI against AI:** damage is /4 when both cars are AI; ×2 when the attacker is a human Dark Tooth.
- **No damage at all:** while the target holds the energy shield (state +0x0E); while `D_80180FBC` (gameAttractDemoMode) (spectator mode) is set; after the round is over (`D_80180FB0` (roundWonFlag), except Dark Tooth).
- **Several hits in one frame:** the killing-blow branch can run more than once in a frame, until the death handler sets `dead`. The last write wins, as in the game.

### 1.2 Hook points

**Killing blow:** 800E5BA4, the start of the `hp <= 0 && !dead` branch
inside `sub_800E5628` (carApplyDamage). The data there:

| Data | Where |
|---|---|
| final amount (after all modifiers) | `s1` |
| victim stats block | `s2` |
| victim state block | `s6` |
| victim car type | `s8` |
| attacker slot (s16) | `sp+120` |
| victim is human | `sp+97` |

The killer byte is stored at 800E5BEC. In the port's C, this is the
`if (killer && attacker >= 0) *killer = attacker;` line.

**Destroyed (exactly once per death):** entry of `sub_8011BE10(car, human)` (carDestroyed)
(`car_destroyed.c`). `a0` is the record and `a1` the human flag. It is called
from:
- `sub_8011B89C(i)` (humanCarDamageUpdate) (human, call at 8011BB1C), right after it sets state
  +0x00 `dead` = 1;
- `sub_8011BBD4(i)` (aiCarDamageUpdate) (AI, call at 8011BDC4).

Both run inside the world step `sub_8011A700` (worldUpdate) (`world_update.c`). At entry
the killer byte is final. For a human, `sub_8011BE10` (carDestroyed) returns early when the
round is already over, but the call still happens.

### 1.3 Classifying a death at `sub_8011BE10` (carDestroyed)

1. Killer >= 0 and it is the victim's own slot: **self kill** (own Ricochet,
   own remote bomb or Shadow splash).
2. Killer >= 0: **killed by** that car, with the attack the port noted at
   the killing blow (section 2).
3. Killer = -1 and state +0x24 `sinking` (human +0xC0, AI +0x90) set:
   **drowned** (deep water).
4. Killer = -1 and state +0x1F `below level` (human +0xBB, AI +0x8B) set:
   **fell off the map** (section 6). Note: sinking also sets +0x1F.
5. Killer = -1 otherwise: **the level** (lava, a human hitting walls and
   hazards, the crush drain, an AI car stuck).

For a "knocked off by" feat, the last-attacker byte can be used as a hint
for cases 3 and 4. It is not cleared at the death.

### 1.4 Deaths that bypass the damage function (no killer recorded)

| Cause | Where | What it does | Conf. |
|---|---|---|---|
| Fall | `sub_800DC88C` (humanCarTick) (`car_tick.c`) store at 800DCC90 (human), `sub_800E7EDC` (aiCarDrive) (`ai_driver.c`) store at 800E8570 (AI) | HP = -1 | CONFIRMED |
| Deep water | `sub_800ED128` (carGroundEffects) (`car_wheels.c`), ground class 4 | sets `sinking` and +0x1F, HP -= ticks each frame | CONFIRMED |
| Lava | `sub_800ED128` (carGroundEffects), ground class 2 | HP -= 3 every 61 ticks while all four wheels are on lava, if the car's damage flag is set and it has no energy shield | CONFIRMED |
| Crush drain | `sub_800F39D0` (carCrushContactUpdate) (`proximity_update.c`) | the car on top drains the car under it: 1 HP every 4th frame | LIKELY (who rides whom) |
| AI stuck | `sub_800E8DFC` (aiUpdateStuck) (`ai_steps.c`) | after 25 stuck frames, -1 HP now and then | CONFIRMED |
| Burning | `func_8010C160` (groundFiresUpdate) | 1 HP in 4 frames, **stops at 5 HP**: cannot kill | CONFIRMED |

Lava, the fire volumes (666), Antarctica's ice and the Hong Kong trains can be switched off by the port (request
38, `--port-define TM2_PORT_HAZARDS`): `ARENA_SWITCHES.md`.

## 2. Which weapon: every damage call site

`sub_800E5628` (carApplyDamage) has no weapon argument. Weapon hits go through
`sub_800DC65C(hit, kind, sub, amount, dir, shooter)` (weaponApplyHit) (`weapon_missile_trails.c`).
For a car hit (object id 1..99), that function turns the **shooter's object
id** into a slot (`owner = human +4 / AI +6`) and calls `sub_800E5628 (carApplyDamage)(hit,
amount, dir, 1, owner)` at 800DC7A4.

Recommendation: before each call site below, the port sets a "current
attack" value. At the killing blow (800E5BA4) it stores (victim, attacker
slot, attack). The call sites are all known and fixed. The machine gun and
the projectiles also carry their own record (owner, type, damage) at the
call.

| Attack | Damage call (VA) | Function (file) | Damage | Attacker passed | Conf. |
|---|---|---|---|---|---|
| Machine gun | 800D81E4 → `sub_800DC65C` (weaponApplyHit) | `sub_800D7E30` (bulletsMove) bullet step (`weapon_bullets.c`); bullets `D_80181498[40]` (bulletRecords), 60 bytes: +0x24 owner object id, +0x2C power, +0x34 kind (0 normal, 4 Mega Guns) | 1 (Mega Guns 3); only half the hits apply (`rand() & 1`) | owner object id | CONFIRMED |
| Every projectile (pickup missiles, Napalm, Freeze, most specials) | 800DC010 → `sub_800DC65C` (weaponApplyHit) | `sub_800DB110` (weaponProjectileUpdate) (`projectile_hit.c`); record in `s1`, section 3 | record +0x0A | record +0x42 (object id) | CONFIRMED |
| Remote bomb (detonated) | 80109900 → `sub_8010B5C4` (areaDamage) → 8010B7DC (human) / 8010B9F8 (AI) | `sub_801096F4` (weaponDropWrite) command 3 (`weapon_slot_writer.c`); slot `D_80184B04[25]` (weaponDrops) kind 19, +4 owner object id, +6 owner slot | 25 - d/4 (50 - d/4 on a raised surface), range 100 | owner slot | CONFIRMED |
| Remote bomb (driven into) | 80109CA4 → `sub_8010B5C4` (areaDamage) | `sub_80109A9C` (weaponDropCheckHit) (`car_damage_human.c`) | same | owner slot (+6) | CONFIRMED |
| Mines (energy attack) | 80109BEC → `sub_800DC65C` (weaponApplyHit) | `sub_80109A9C` (weaponDropCheckHit); slot kind 20 | 15 | owner object id (+4) | CONFIRMED |
| Ricochet | 8010D454 (human), 8010D768 (AI) → `sub_800DC65C` (weaponApplyHit) | `sub_8010CC54` (weaponRicochetUpdate) (`weapon_ricochet.c`); 10 shots at `D_80184888`, 48 bytes: +0x00 power, +0x02 owner object id | power: at least 7, grows with age, doubled after the first bounce | owner object id | CONFIRMED |
| Lightning | 80108B48 / 80108B98 (10 + 10) | `sub_801084D8` (weaponArcsDraw) (`weapon_arcs_draw.c`); slots `D_801847B4[5]` (weaponArcs), +6 = 1 for lightning; set by `sub_80107B00` (weaponLightningScan) (`weapon_slot_scan.c`), levels 1, 2 and 8 only | 10 at the start + 10 after 40 frames | **stale**: the slot's +2 owner is not written for lightning, so it holds the previous taser's owner (or 0). The caster is known at the call 80104558, section 9.3 | CONFIRMED (stale) |
| Ram (car into car) | 800E3A18 | `sub_800E3800` (carContactRespond) (`car_contacts.c`) | contact +0x04 = f × 12 / the *other* car's speed cap (f = relative push speed); the car aimed more squarely at the other (smaller angle between its heading and the bearing to it) takes ÷2 and the other ×2; then each × \|angle\|/1024 below 1024 (a car pointing straight at the other takes almost nothing). Weight sets only the push speeds. Typically 0..24. Computer vs computer ÷4 in carApplyDamage. Request 40 (`BALANCE.md`, absorbing request 39): ×1.2 / ×1.1 / ×1.1 when Hammerhead / Axel / Minion dealt it; none taken by a Twister in her tornado or a leaping Grasshopper | other car's slot (contact +0x14) | CONFIRMED |
| Crush (cars with stats +2: Hammerhead, Axel, Minion, Dark Tooth) | 800E7338 | `sub_800E6D14` (carCrashStart) (`car_crash.c`), from the car collision `sub_800E184C` (carCarCollision) | the mount: 15 once (`sub_8010B59C` (carCrushDamageAmount)); after a non-Dark-Tooth crush the normal ram damage is applied too. The ride: while on top, 1 HP every 4th frame-counter tick taken directly from HP (`sub_800F39D0` (carCrushContactUpdate), not carApplyDamage, no credit). Request 40 (`BALANCE.md`): Axel and Minion never mount; Hammerhead's mount stays 15, +5 once the first frame he is on top | crusher's slot (mount) | CONFIRMED |
| Wall or object (humans only) | 800E3914 | `sub_800E3800` (carContactRespond) | contact +0x04 | -1 | CONFIRMED |
| Hazard objects (kinds 100/101, humans only) | 800E14F0 | `sub_800E0C28` (carContactResolve) (`car_contact_resolve.c`) | by speed | -1 | CONFIRMED |
| Car specials | see section 3 | | | | |
| AI wreck blast, Dark Tooth death blast | 800E7BFC, 8011C198 | `sub_800E75F4` (carWreckUpdate), `sub_8011BE10` (carDestroyed) | 15 to itself | 0 (no effect: the car is already dead) | CONFIRMED |

Splash damage (`sub_8010B5C4` (areaDamage), `area_damage.c`) has no owner exclusion, so
your own remote bomb or Shadow splash can kill you (killer = own slot).
The Ricochet can also hit its owner after its first few ticks.

## 3. Special weapons (each car's own) and projectile types

### 3.1 By car

The special is weapon 0 (`sub_80105A7C` (weaponFireSpecial), `weapon_basic.c`, switch on the
car type). The table also gives the damage path for "special hits" and
"special kills" feats.

| Car | What it fires | Damage path (VA of the damage call) | Damage | Attacker | Conf. |
|---|---|---|---|---|---|
| Sweet Tooth | projectile type 61 (`sub_800D9C5C` (weaponFireSpiralMissile)) | projectile, 800DC010 | 15 | object id | CONFIRMED |
| Hammerhead | no shot; the crush when ramming (+ crush drain) | 800E7338 (crush) | 15 (+ drain, no credit) | slot | CONFIRMED |
| Outlaw 2 | taser `sub_801075C8` (weaponTaserTarget) (`weapon_slot_target.c`), nearest car within 80; slot +6 = 0 | 80108B48 / 80108B98 | 10 + 10 | slot (+2) | CONFIRMED |
| Warthog | 3× projectile type 41 (`sub_800DA448` (weaponFireColoredMissile)) | 800DC010 | 6 each | object id | CONFIRMED |
| Mr. Grimm | projectile type 43 (`sub_800D9A08` (weaponFireGrimmSpecial)) | 800DC010 | 40 | object id | CONFIRMED |
| Grasshopper | leap + homing hit (`sub_8010EF44` (weaponHomingLeapInit) sets it up in `weapon_leap_homing_init.cpp`, `sub_8010F520` (weaponHomingLeapUpdate) hits) | 8010F8C8 | 15 (type 2) | slot (`D_80184DC0` (weaponTornadoState) +0x20) | CONFIRMED |
| Thumper | flame (`sub_8010A5F0` (weaponFlameStart) starts, `func_8010A7A0` (weaponFlameUpdate) steps) | 8010AA00 → `sub_800DC65C` (weaponApplyHit) | 1 per hit, and sets the target on fire (burn cannot kill) | object id | CONFIRMED |
| Spectre | projectile type 49 (`sub_800D9E60` (weaponFireGhostMissile)), passes walls | 800DC010 | 12 | object id | CONFIRMED |
| Roadkill | projectile type 51 (`sub_800D98E0` (weaponFireRoadkillBoomerang)), returns after 300 units | 800DC010 | 10 out, 40 on the way back | object id | CONFIRMED |
| Twister | tornado (`sub_8010E040` (weaponTornadoUpdate), `weapon_tornado.c`) | 8010E41C (human) / 8010E7DC (AI) → `sub_800DC65C` (weaponApplyHit) | 4 per catch | object id | CONFIRMED |
| Axel | shock wave inside `sub_80105A7C` (weaponFireSpecial): every car within 800, \|dz\| < 200 | 801069E0 | 15 (type 2) | slot | CONFIRMED |
| Mr. Slam | claw (`sub_800F4058` (weaponSlamSwingUpdate) / `sub_800F4CC0` (weaponSlamGrabUpdate) → `sub_800F4B74` (carDamageByObject)) | 800F4C10 (calls at 800F4370, 800F4884, 800F52A4) | lift 12, slam 10 each, heavy cars 25 (halved on Easy) | slot | CONFIRMED |
| Shadow | ground missile `sub_8010F9C0` (weaponShadowStrikeUpdate) (`strike_update.c`); 2nd press detonates (`sub_8010B5C4` (areaDamage) at 80106EA4) | 8010FC90 (direct) / 8010B7DC, 8010B9F8 (splash) | 25 direct; splash 25 - d/4 within 75 | object id (direct) / slot (splash) | CONFIRMED |
| Minion | 3× type 41 + 1× type 14 (freeze) | 800DC010 | 10 each + 7 | object id | CONFIRMED |
| Dark Tooth | human: no special (×2 on all his damage). Boss AI on level 8: `func_8010FD44` (aiDarkToothBossUpdate) fires type 61 shots (`sub_800D9D38` (weaponFireDarkToothMissile), 5) and swoops | 800DC010; swoop 80110630 | 5; swoop 7 | object id; swoop: slot | CONFIRMED |

How to count "special kills" in the port:
1. For projectiles, the type identifies the car (types 41, 43, 49, 51, 61;
   type 14 or 41 from a Minion is Minion's).
2. For the other specials, use the call site.
3. Two overlaps: a Minion-special code (energy 106) fires the same types 41
   and 14 as Minion's own special. Type 61 also comes from the Dark Tooth
   boss. Check the attacker's car type and the call site.

### 3.2 Projectile records and types

Record: `D_801A5B08[16]` (fxRecords), 980 bytes (`projectile_hit.c`, `weapon_fire_missiles.c`).

| Offset | Field |
|---|---|
| +0x02 | type (s16) |
| +0x04 | life |
| +0x06 | speed |
| +0x08 | turn rate |
| +0x0A | **damage** (s16) |
| +0x34 | world object |
| +0x38 | target |
| +0x42 | **owner object id** (s16) |

`sub_800DB110` (weaponProjectileUpdate) updates one record and is the hit path for all of them.

| Type | Weapon (retail name) | Emitter | Damage | Fired by | Conf. |
|---|---|---|---|---|---|
| 10 | Fire missile ("Missiles") | `sub_800D9840` (weaponFireMissile) | 7 | weapon 1 (`sub_801052BC` (weaponLaunchProjectile)) | CONFIRMED |
| 11 | Homing missile ("Homers") | `sub_800DA044` (weaponFireHomingMissile) | 10 | weapon 2 | CONFIRMED |
| 12 | Power missile ("Powers") | `sub_800D9BE8` (weaponFirePowerMissile) | 15 | weapon 4 | CONFIRMED |
| 14 | Freeze missile | `sub_800D9AE0` (weaponFireFreezeMissile) | 7 | energy attack 100, Minion's special, energy 106 | CONFIRMED |
| 15 | Napalm | `sub_800DA190` (weaponFireNapalm) (10) / `sub_800DA2EC` (weaponFireMegaNapalm) (30, when weapons +9 is set) | 10 / 30 | weapon 5, energy attack 102 | CONFIRMED (weapons +9 is the Mega Napalm code, section 9.2) |
| 41 | coloured missiles | `sub_800DA448` (weaponFireColoredMissile) | given by caller | Warthog (6), Minion (10), energy 106 (10) | CONFIRMED |
| 43 | Mr. Grimm's special | `sub_800D9A08` (weaponFireGrimmSpecial) | 40 | Mr. Grimm | CONFIRMED |
| 49 | Spectre's ghost missile | `sub_800D9E60` (weaponFireGhostMissile) | 12 | Spectre | CONFIRMED |
| 51 | Roadkill's boomerang | `sub_800D98E0` (weaponFireRoadkillBoomerang) | 10, ×4 on return | Roadkill | CONFIRMED |
| 61 | spiral missile | `sub_800D9C5C` (weaponFireSpiralMissile) (15) / `sub_800D9D38` (weaponFireDarkToothMissile) (5) | 15 / 5 | Sweet Tooth / Dark Tooth boss | CONFIRMED |

The hit test `sub_800D6FD8` (projectileHitTest) (`weapon_slam_swing.c`) applies side effects by type:
- **14 freezes** the target: state +0x0D, timer stats +0x1A = 300.
- **15 sets it on fire.**

Weapon numbers in the weapon block, from the pickup handler `sub_8010AFC8` (pickupApply)
(`pickup_apply.c`) and the HUD strings. The selected weapon is +0x0D; the
count of weapon k is +0x0E + 2k.

| k | Weapon | Pickup id | How it fires (`sub_80103FB8` (weaponFireSelected), `weapon_select.c`) |
|---|---|---|---|
| 0 | car special | | `sub_80105A7C` (weaponFireSpecial) |
| 1 | Fire missile | 401 | type 10 |
| 2 | Homing missile | 403 | type 11 |
| 3 | Remote bomb | 406 | drop or detonate (`sub_80109518` (weaponDropItem)) |
| 4 | Power missile | 404 | type 12 |
| 5 | Napalm | 411 | type 15 |
| 6 | Ricochet | 413 | `sub_8010CC54` (weaponRicochetUpdate) shots |
| 7 | Lightning | 419 | `sub_80107B00` (weaponLightningScan) (levels 1, 2, 8) |

Other pickups: 417 full health, 418 one third health, 430 turbo.

Energy attacks (weapons +0x0C, `sub_801046FC` (weaponFireEnergyAttack), `weapon_special.c`; codes in
`sub_800DECA8` (carReadMoveCodes), `car_input.c`, read as Up=1 Down=2 Left=3 Right=4,
LIKELY):

| Kind | Attack | Code | Energy cost |
|---|---|---|---|
| 100 | Freeze missile | Left, Right, Up | 400 |
| 101 | Mines | Right, Left, Down | 120 |
| 102 | Napalm | Right, Left, Up | 400 |
| 103 | Shield | Up, Up, Right | 400 |
| 104 | Jump | Up, Up, Left | 120 |
| 105 | Invisibility (LIKELY) | Right, Down, Left, Up | 400 |
| 106 | Minion special | secret code | 900 |

The rear-fire code (Left, Right, Down) sets a flag instead of an energy
attack (section 9.2).

**Special hits (not only kills):** hook `sub_800E5628` (carApplyDamage) entry with the
current attack. A hit counts when the target's HP drops. Read stats +0xA4
before and after the call: shields, AI-vs-AI ÷4 and spectator mode can
make it 0.

## 4. A pedestrian is killed

The pedestrians are the "target actors" `D_80190E54[6]` (targetActors) (60 bytes;
`target_actor_*.c`):

| Offset | Field |
|---|---|
| +0x02 | state: 0 walking/standing, 1 thrown, 2 settling |
| +0x04 | kind (0x202 and 0x205 walk between two points) |
| +0x06 | instance |
| +0x38 | world object; its object id is 6000 + index |

They are placed per level (start tables for levels 1, 3, 5, 6, 8, 11) and
come back 300 frames after being hit (`sub_801259A4` (targetActorRespawn)). LIKELY pedestrians:
- they walk back and forth;
- a car touching one throws it along the car's heading;
- the 10th run-over by a human car gives the HUD message "Demented Bonus"
  (message 34), plus 1/3 health and up to 5 Homing missiles to that car.

No other "pedestrian" object was found.

| How | Hook | Who did it |
|---|---|---|
| Run over by a human car | `sub_80126464` (targetActorsUpdate) (`target_actor_update.c`), state store at 80126C10 | `s2` = human index, `s1` = pedestrian record. The bonus counter `D_80180BBC` (targetActorRunOverCount) (global, both humans) is checked at 80126D90 |
| Run over by an AI car | same function, state store at 80126FB0 | `s2` = AI index |
| Hit by a weapon | `sub_800DC65C` (weaponApplyHit) when the hit id >= 6000: call to `sub_80127228(id - 6000, dir)` (targetActorHit) at 800DC76C | the `shooter` argument (6th, object id). The machine gun, projectiles, mines, Thumper's flame and Shadow's missile all hit through here |
| Axel's shock wave, Mr. Slam's claw, splash (Shadow, remote bomb), Twister, Ricochet, burning ground | direct calls to `sub_80127228` (targetActorHit) at 80106D5C, 800F47C4, 8010BDA0, 8010ECF8, 8010D9D8, 8010C800 (`func_8010C160` (groundFiresUpdate)) | not passed; the port knows the car from the calling special (splash: its slot argument) |

`sub_80127228` (targetActorHit) and the run-over both report the kill to the level's target
registry with `sub_80124384(kind, instance, 2)` (objRegistrySetStep). That one call covers all
causes, but without the car.

Quirk: the run-over loops do not re-check the state. Two cars touching the
same pedestrian in one frame both "throw" it, and each human adds to the
bonus counter.

## 5. Air time

There is no per-car air-time counter. The wheel step `sub_800EC994` (carWheelsUpdate)
(`car_wheels.c`) writes two state flags every frame:

| Flag | State offset | Human | AI | Meaning |
|---|---|---|---|---|
| some wheel in the air | +0x0B | +0xA7 | +0x77 | LIKELY |
| all four wheels in the air | +0x0C | +0xA8 | +0x78 | LIKELY |

The step is called at 800DCAB0 (human, `sub_800DC88C` (humanCarTick)) and at 800E944C (AI,
`sub_800E8F38` (aiMovementUpdate)).

**Naming warning.** `car_wheels.c` calls these `any_touch` / `all_touch`
(wheel +0x01 `touch`). The uses say the opposite: the flag means "in the
air".
1. The view hotkeys are locked while +0x0B is set (`sub_8011C530` (carViewHotkeys)).
2. Ground drag applies only while +0x0B is clear (`car_motion.c`).
3. A wheel's `land_z` (+0x2A) tracks the **highest** point while `touch` is
   set.
4. Wheel +0x28 counts ticks only while `touch` is clear. The lava burn
   needs 8 of those ticks, so +0x28 is "ticks on the ground".

The comments in `sub_800EE3A4` (carWheelUpdate) / `sub_800EF0B0` (carWheelVerticalMotion) describing +0x28 as air time
are wrong for the same reason. Please verify at runtime.

Recipe:
- **Take-off:** +0x0C goes 0 → 1.
- **Landing:** +0x0C goes 1 → 0. Count the frames in between (one world
  step per frame, `sub_8011A700` (worldUpdate)).
- **Height:** the motion z (human +0x704, AI +0x744) gives the height. The
  per-wheel `land_z` gives the apex.
- **Cross-check:** compare the car z with `sub_800EF340(pos)` (worldGroundHeightAt), the ground
  height under a point (`car_wheels.c`).

## 6. Falling off the map

Per frame, in the car update (human `sub_800DC88C` (humanCarTick), AI `sub_800E7EDC` (aiCarDrive)):
1. When the car z goes below the level limit `sub_8011EC0C(car, human)` (worldFallLimitZ)
   (`world_fall_limit.c`), state +0x1F is set (human +0xBB, AI +0x8B). It is
   cleared when the car climbs back above the limit.
2. While +0x1F is set and z < `sub_8011ECC4()` (worldFallDeathZ), HP = -1 (stores at 800DCC90
   human, 800E8570 AI). The killer stays -1.

| Level | Limit (`sub_8011EC0C` (worldFallLimitZ)) | Death below (`sub_8011ECC4` (worldFallDeathZ)) |
|---|---|---|
| default | -8000 | 80 |
| 5 New York, 12 Roof Tops | 6000 | 80 |
| 6 Antarctica | 3840 | 80 |
| 8 Hong Kong | -8000; for a human with z < 0 and y < -13920: 0 | -800 |

So on most levels the death comes the frame after the car drops below
-8000. On the rooftop levels it comes as soon as the car is below 80 after
dropping under 6000.

The death then runs through `sub_8011BE10` (carDestroyed) like any other. That function
sinks the car on level 6 when z < 240 and hides it on level 8 when z < -40.

**Hook:** the two HP = -1 stores (the car record is in `s1` for a human,
`s0` for an AI). Or, at `sub_8011BE10` (carDestroyed), test killer = -1 and +0x1F set and
`sinking` clear (section 1.3). CONFIRMED.

## 7. Speed

| Field | Human | AI | Units |
|---|---|---|---|
| Current speed (motion +0x0A, s16, signed: negative = reverse) | +0x6E6 | +0x726 | internal |
| Top speed (stats +0x52, s16) | +0x19E | +0x54E | "mph" |
| Speed cap (stats +0xB8, s32) | +0x204 | +0x5B4 | internal |

- **Converting:** "mph" = internal / 32 × 100 / (ticks × 19), where ticks =
  `sub_8015531C()` (timerGetFrameTicks) (3 at 20 fps). This is the HUD readout `sub_8011CBD0` (hudSmoothSpeed) /
  `sub_8011CD08` (carGetDisplaySpeed) (`car_records.c`), smoothed into `D_8018100C[player]` (hudSmoothedSpeeds).
- **The current speed is rescaled** when the frame time changes
  (`car_tick.c`, `ai_driver.c`).
- **Top speed:** `sub_800FE4F0(type)` (carDefaultTopSpeed) (`ai_car_start.c`) only returns the
  default top speed per car type. `sub_800FE478(stats, mph)` (carStatsSetTopSpeed) sets stats
  +0x52 and derives the cap +0xB8 = mph·19·ticks/100·32·mult/100 (mult 100
  for humans, 75/120/140 for AI by difficulty).
- **Turbo** raises the cap.

**Top speed reached:**
- Turbo counts: `abs(speed) >= S32(stats + 0xB8)`.
- Turbo does not count: compare the "mph" value with
  `sub_800FE4F0(type)` (carDefaultTopSpeed).

Read it after the car update (`sub_8011C400` (humanCarUpdate) / `sub_8011C4A0` (aiCarUpdate), called from
`sub_8011A700` (worldUpdate)). CONFIRMED.

## 8. Wins

The session runner `sub_8012BE5C` (sessionRun) (screenCheckMidGameOptions (sessionRun)) (`session_runner.cpp`) gets the round
result from `rtMainLoop` (call at 8012C0F8). See `MATCH_START.md` section 3.

| Mode | Event | Hook | Data |
|---|---|---|---|
| 1 tournament, 2 co-op tournament | **Tournament won** | `screenScrollText` 80136BB0 (`menu_scroll_text.c`): the level >= 8 path at 80136D0C. 1 player goes to the ending (80136D20, returns `sub_80136D98` (screenEnding)); co-op shows `COWIN.TIM` (80136D30) | `D_8016475C` (sessionSettings) +8 / +9 car-select index of each human, +0x24 difficulty |
| 1, 2 | Easy stop after level 4 (no ending) | 80136C88 (`STOP.TIM`) | not a tournament win |
| 1, 2 | Level won | result 3; the level is settings +24 | |
| 3 challenge | won / lost | `sub_8012B0EC(s, won, wins, losses)` (screenChallengeResult) at 8012C784: a1 = 1 won (result 3), 0 lost (result 1) | |
| 4 two-player deathmatch | **Round winner** | `sub_8012B2D4(s, winner, p1_wins, p2_wins)` (screenDeathmatchResult) at 8012C834: a1 = 1 player 1 won (result 3, human 1 died), 2 player 2 won (result 4, human 0 died) | the loser's killer byte says how |

The deathmatch never ends on its own (only on quit, result 2). "Match won"
is a port rule, for example first to N round wins. The game keeps the tally
in locals of `sub_8012BE5C` (sessionRun) (screenCheckMidGameOptions (sessionRun)) only. CONFIRMED.

## 9. Which weapon at the killing blow

Section 2 lists every damage call. This section says how the port knows the
weapon, the attacker and the victim at each one, and fills the gaps.

### 9.1 One rule for the port

`sub_800E5628` (carApplyDamage) takes the victim (object id, `a0`) and the attacker (slot,
5th argument, at `16(sp)` on entry) but not the weapon. The port keeps a
"current attack" value, sets it just before each call site below and
clears it after. At the killing blow (800E5BA4) it reads the triple
(victim, attacker slot, attack). Every row has all three at hand.
CONFIRMED (the call sites of section 2, checked in retail).

| Attack | Set "current attack" at | Attacker | Victim | How the weapon is known |
|---|---|---|---|---|
| Machine gun | 800D81E4 (`sub_800D7E30` (bulletsMove)) | bullet +0x24 (object id) | the bullet's hit id | bullet +0x34: 0 normal, 4 Mega Guns (cheat, 9.2) |
| Fire / Homing / Power missile | 800DC010 (`sub_800DB110` (weaponProjectileUpdate)) | record +0x42 (object id) | the record's hit id | record +0x02 type 10 / 11 / 12 |
| Napalm (pickup or energy) | 800DC010 | +0x42 | hit id | type 15; pickup or energy from the launch tag (below) |
| Freeze missile | 800DC010 | +0x42 | hit id | type 14; energy 100, Minion's special or code 106 (launch tag) |
| Remote bomb | 80109900 (detonated), 80109CA4 (driven into) | weapon slot +6 (slot) | each car in `sub_8010B5C4` (areaDamage) | the call site |
| Mines | 80109BEC | weapon slot +4 (object id) | hit id | the call site |
| Ricochet | 8010D454 / 8010D768 | shot +0x02 (object id) | the car in the loop | the call site |
| Lightning | 80108B48 / 80108B98 | **not in the slot**: note it at 80104558 (9.3) | slot +8 `target_id` | the call site |
| Car specials | section 3.1 (types 41, 43, 49, 51, 61 at 800DC010; the others by call site) | per row | per row | type or call site |
| Ram / crush | 800E3A18 / 800E7338 | contact +0x14 / the crusher's slot | the other car | the call site |
| Rear fire | any row above | | | the launch had weapons +5 set (9.2) |

**Launch tags.** One record pool (`D_801A5B08[16]` (fxRecords)) serves every
projectile, and a record does not say why it was launched. The port keeps a
16-entry side table indexed by the record slot and fills it where the slot
is known:
- in `sub_801052BC` (weaponLaunchProjectile) (`weapon_launch.c`), after the emitter returns `slot`
  (weapons 1, 2, 4, 5 and energy attacks 100, 102). The call at 801042E4
  (`sub_80103FB8` (weaponFireSelected)) is a selected weapon; the call at 80104880
  (`sub_801046FC` (weaponFireEnergyAttack)) is an energy attack;
- in `sub_801046FC` (weaponFireEnergyAttack) case 106 (Minion code: three type 41 + one type 14);
- in `sub_80105A7C` (weaponFireSpecial) for the car specials.

At 800DC010 the port reads the tag of the record in `s1`. CONFIRMED (call
sites); the side table is the port's own.

### 9.2 Energy attacks, rear fire and the secret codes

The code reader is `sub_800DECA8` (carReadMoveCodes) (`car_input.c`). A complete code writes
the attack kind to weapons +0x0C (human +0x7D4) and raises the request
ctl +0x1C (human +0xEC). `sub_801046FC` (weaponFireEnergyAttack) (`weapon_special.c`) fires it when
the energy (weapons +0x30, s16: 1000 at each start, +1 per frame up to
1000) covers the cost and the cooldown (weapons +0x26) is 0. Codes use
Up = 1, Down = 2, Left = 3, Right = 4 (LIKELY, as in section 3.2). AI cars
choose their attacks in their own code and use the same function.

| Attack | Code | Cost | What it does | Damage path | Conf. |
|---|---|---|---|---|---|
| 100 Freeze | 0x341 (Left, Right, Up) | 400 | `sub_801052BC(car, 100)` (weaponLaunchProjectile): projectile type 14 | 800DC010, 7; the freeze is set in the hit test (section 12) | CONFIRMED |
| 101 Mines | 0x432 (Right, Left, Down) | 120 | `sub_80109518(car, 101)` (weaponDropItem) → `sub_801096F4` (weaponDropWrite) command 101: weapon slot kind 20, life 300 frames. An AI car drops mines only when its record byte +1 is 2 | 80109BEC, 15, and the car is thrown | CONFIRMED |
| 102 Napalm | 0x431 (Right, Left, Up) | 400 | `sub_801052BC(car, 102)` (weaponLaunchProjectile): type 15 | 800DC010, 10 (30 with Mega Napalm); sets the target on fire; a ground fire where it lands | CONFIRMED |
| 103 Shield | 0x114 (Up, Up, Right) | 400 | state +0x0E = 1; the shield record (`D_80184A68` (weaponShieldPool)) lives 60 frames | none; blocks all damage, puts out fire | CONFIRMED |
| 104 Jump | 0x113 (Up, Up, Left) | 120 | `sub_800E5FFC(car, human, 7, 1)` (carSpinOut) | none | CONFIRMED |
| 105 Invisibility | 0x4231 (Right, Down, Left, Up) | 400 | weapons +4 = 1, timer weapons +0x34 = 120; refused while `sub_8011EDB8()` (roundIsOver) is set | none | CONFIRMED (the effect LIKELY) |
| 106 Minion special | 0x1211 (Up, Down, Up, Up) with action 0x1E held | 900 | 3× type 41 (10) + 1× type 14 (7) | 800DC010 | CONFIRMED |
| Rear fire | 0x342 (Left, Right, Down) | none | weapons +5 = 1 and fire request ctl +0x1A = 1: the selected weapon fires at once, backwards, with its own ammo | the weapon's own path | CONFIRMED |

**Rear fire** is not a weapon of its own. The next launch reads and clears
weapons +5: `sub_801052BC` (weaponLaunchProjectile) (missiles, napalm, freeze), `sub_80103FB8` (weaponFireSelected)
case 6 (Ricochet) and `sub_80105A7C` (weaponFireSpecial) (specials). A set flag turns the yaw
by 0x800 and zeroes the pitch. A "rear-fire kill" is a kill whose launch
had weapons +5 set; the port reads it at the launch and keeps it in the tag
(9.1). CONFIRMED.

**Secret codes that change the weapons** (all in `sub_800DECA8` (carReadMoveCodes)):

| Code | Effect | For feats |
|---|---|---|
| 0x12233344 (Up, Down, Down, Left, Left, Left, Right, Right) with Napalm selected and action 0x1F held | **Mega Napalm**: weapons +9 = 1 (napalm uses `sub_800DA2EC` (weaponFireMegaNapalm), 30 damage) and +10 napalms; once per car | napalm kills are still type 15 |
| 0x12344321 with actions 0x1E / 0x1F / 0x20 / 0x21 | toggles stats +0xA8 (0 = takes no damage), weapons +0 (infinite weapons and turbo, 13.2) and sets weapons +8 (**Mega Guns**: bullet kind 4) | the port may want to ignore feats while these are on |
| 0x21431122 | health from the special and other counters | |

### 9.3 Lightning: the caster

The lightning (weapon 7, levels 1, 2 and 8) strikes every car in a fixed
zone, **the caster included**: Los Angeles (-4608, -2192, 800) within 250
(distance >> 3), Hong Kong (0, 0, 2560) within 350, Moscow by track region
(`sub_800EFF34` (worldSectionAt) against `sub_800D44E0(1..4)` (moscowEventGet)). AI cars are struck only
while alive. `sub_80107B00` (weaponLightningScan) (`weapon_slot_scan.c`) fills one slot of
`D_801847B4[5]` (weaponArcs) per car (count 40, +8 `target_id` = object id) and one empty
slot. It never writes the caster. The damage (10 at count 39, 10 at count
0) is applied by the drawing routine `sub_801084D8` (weaponArcsDraw) with the stale +2.
CONFIRMED.

Hook: the call to `sub_80107B00` (weaponLightningScan) at 80104558 (in `sub_80103FB8` (weaponFireSelected); `a0` =
caster record, `a1` = human flag). The port notes every slot whose count is
40 after the call and gives it the caster's slot. At 80108B48 / 80108B98 it
uses that slot, not +2.

### 9.4 Fire and burning (no kills)

Burning never kills: `func_8010C160` (groundFiresUpdate) takes 1 HP in 4 frames and stops at
5 HP (section 1.4). It records no attacker either. The ways a car is set on
fire (CONFIRMED):

| Source | Where | Who did it |
|---|---|---|
| Napalm direct hit (type 15) | `sub_800D6FD8` (projectileHitTest) (`weapon_slam_swing.c`), the type 15/47 branch | the projectile owner (`s5` there, object id). Type 47 has no emitter (GUESS: unused) |
| Napalm ground fire (30 areas `D_801842C4` (groundFires), 100 frames each) | `func_8010C160` (groundFiresUpdate); made by `sub_8010C020` (groundFireSpawn) where a napalm hits or ends | nobody: an area has no owner, and it also lights the napalm's owner |
| Thumper's flame | `func_8010A7A0` (weaponFlameUpdate) | the emitter's car (+0x1C, object +0x118) |
| Hazard object 666 | `sub_800E06CC` (carContactApplyObject) (`car_object_contact.c`) | the level |

A fire goes out after 100 frames (stats +0x22 counts up), with the energy
shield, or (humans only) with the turbo button when the meter is at least
51; that costs 50 (store at 800DD9D0, `sub_800DD304` (carThrottleSteer)).

## 10. Damaging oneself

The game has no self-damage logic. `sub_800E5628` (carApplyDamage) never compares attacker
and victim (CONFIRMED). The only owner checks are in the hit tests, which
skip the owner's car:

| Weapon | Can it hit its owner? | Where |
|---|---|---|
| All projectiles (pickup missiles, napalm, freeze, special types 41/43/49/51/61) | **no**: `sub_800D6FD8` (projectileHitTest) skips object id == owner, and `sub_800DB110` (weaponProjectileUpdate) drops a hit on the owner (Roadkill's boomerang coming back is a catch: +1 special) | `weapon_slam_swing.c`, `projectile_hit.c` |
| Machine gun, Thumper's flame | no (same test) | `weapon_bullets.c`, `weapon_flame_update.c` |
| **Mines** | **yes, after 40 frames**: the hit test skips the owner only while the timer (300, counts down) is >= 260; after that the owner sets it off like any car | `sub_80109A9C` (weaponDropCheckHit) (`car_damage_human.c`) |
| **Remote bomb, driven into** | **yes, after 40 frames** (timer 0x7FFF; the owner is skipped while >= 32727) | same |
| **Remote bomb, detonated**; **Shadow's splash** | **yes**: `sub_8010B5C4` (areaDamage) hits every car within 100 / 75 | `area_damage.c` |
| **Ricochet** | **yes**, after its first 2 frames (4 for an AI owner): the owner is skipped only while life >= 398 / 396 | `sub_8010CC54` (weaponRicochetUpdate) (`weapon_ricochet.c`) |
| **Lightning** | **yes**, when the caster is in the zone (9.3) | `weapon_slot_scan.c` |
| Axel's shock wave | no (skips its own slot) | `sub_80105A7C` (weaponFireSpecial) |
| Twister, Mr. Slam, Grasshopper, Outlaw 2's taser | no (they pick another car) | LIKELY |
| Napalm ground fire | yes, but burning has no attacker and cannot kill (9.4) | |

**How the port recognizes it:** at `sub_800E5628` (carApplyDamage) entry, the attacker slot
(5th argument) equals the victim's slot (human +0x04, AI +0x06 of the record
found from `a0` with `sub_8011D2C8` (carFindById)). For "damaged yourself", also require
that the HP went down (stats +0xA4 before and after: shield, spectator
mode). For lightning, compare with the caster from 9.3, not the slot's +2.

Two false matches to exclude by call site: the AI wreck blast (800E7BFC)
and the Dark Tooth death blast (8011C198) pass attacker 0, which looks like
self-damage when the dying car is slot 0. CONFIRMED.

A self kill leaves killer = own slot (section 1.3, case 1).

## 11. Dying, and dying on fire

**Death:** the entry of `sub_8011BE10(car, human)` (carDestroyed) (section 1.2), once per
death; classify it with section 1.3. CONFIRMED.

**On fire at death:** read the burning byte at that entry. Neither the
damage function nor `sub_8011BE10` (carDestroyed) clears it.

| Field | Human | AI |
|---|---|---|
| burning, state +0x1A (u8) | +0xB6 | +0x86 |
| burn time, stats +0x22 (s16, 0..100) | +0x16E | +0x51E |
| fire effect, weapons +0x44 | +0x80C | +0x84C |

Burning alone cannot kill (section 1.4: the HP stops at 5; for a human it
drains only while stats +0xA8 = 1). So "died while on fire" means another
cause finished the car while it burned. CONFIRMED.

## 12. Freezing

A freeze missile (type 14, 9.2) freezes the car it hits. The hit test
`sub_800D6FD8` (projectileHitTest) does it, before the damage:

| Field | Human | AI | Meaning |
|---|---|---|---|
| frozen, state +0x0D (u8) | +0xA9 | +0x79 | the car cannot steer, fire or use energy |
| freeze timer, stats +0x1A (s16) | +0x166 | +0x516 | 300 at the freeze; minus the frame ticks each frame; minus 6 × damage on each hit |
| frozen by a missile, stats +0x1C (s16) | +0x168 | +0x518 | 1 at a missile freeze; the thaw (`car_tick.c`, timer <= 0) clears it and removes the ice |

The frozen flag is also set for other reasons, with +0x1C left at 0: a
crush (`sub_800E6D14` (carCrashStart), 30) and death (`sub_8011BE10` (carDestroyed), 540: the respawn
hold). So **count freezes at the hooks, not from the flag**.

| Event | Hook | Data |
|---|---|---|
| Human frozen | store at 800D7220 (`sb s7, 169(v0)`) | `v0` = victim record, `s2` = victim index, `s5` = the shooter's **object id** |
| AI frozen | store at 800D755C (`sb s6, 121(s0)`) | `s0` = victim record, `s5` = the shooter's object id |

Both run only for a new freeze. A missile on a car that is already frozen
only resets the timer to 300. A human car with the energy shield is not
frozen; an AI car is (but takes no damage). The shooter is never the
victim (owner skipped). "Freeze 10 enemies" counts the shooter at these
stores; "frozen by enemies 10 times" counts the victim. Decode the
shooter's object id with `sub_8011D2C8` (carFindById) (section 0). CONFIRMED.

## 13. Pickups and the turbo meter

### 13.1 Collecting a pickup

The car's contact test `sub_800E05F0` (carTestContactPoints) (`car_motion.c`) calls
`sub_800E06CC(slot, car, human, apply)` (carContactApplyObject) (`car_object_contact.c`). For ids 401..480
that calls **`sub_8010AFC8(car, human, id, map_slot)` (pickupApply)** (`pickup_apply.c`)
at **800E081C**: `a0` = the car record, `a1` = 1 human / 0 AI, `a2` = the
pickup id, `a3` = the map slot. CONFIRMED.

| Id | Pickup | Effect |
|---|---|---|
| 401, 403, 404, 406, 411, 413, 419 | weapons (section 3.2) | ammo |
| 417 | full health | HP = max, only if HP > 0 |
| 418 | one third health | HP += max / 3 (capped), only if HP > 0 |
| 430 | **turbo ("T")** | weapons +0x32 += 100, capped at 999 |

Ids 441..480 are the same pickups + 40. They work the same but are not
removed from the map. They are the rotating bonus spots on Los Angeles and
Moscow: they hang under an animation node with id 440 that steps through
them. The loader has no case for 440 or 441..480, so they are never
registered and never hidden. On one Moscow carrier the hit box is beside
the 44x node, so touching it gives id 440, which `pickupApply` ignores
(jingle only); likely a retail data bug (see PICKUP_LOOKS.md 1; the earlier
guess here, "placed or bonus pickups", is replaced).

Hook the entry of `sub_8010AFC8` (pickupApply): id 430 (or 470) = turbo collected, 417 /
418 (457 / 458) = health collected, by the car in `a0`. Health and turbo
are always used up, even at full health or a full meter. A **human** car
with a full weapon store does not collect a weapon pickup (the `full` path
skips the removal), so count weapon pickups only when the function reaches
the `sub_8012493C` (pickupSetVisible) removal. AI cars collect pickups too. Pickups come back
at random (`sub_8010AED0` (pickupRespawnTick)). CONFIRMED.

### 13.2 The turbo meter

| Field | Human | AI |
|---|---|---|
| turbo, weapons +0x32 (s16) | **+0x7FA** | +0x83A |

- **Start:** 200, at each (re)start (`sub_800FDCAC` (weaponBlockInit), `ai_route_setup.c`).
- **Maximum:** 999. Only the pickup caps it (1000 or more becomes 999). The
  HUD shows the value / 10 (`hud_main.c`), so 999 shows as 99. No other
  maximum, and no refill over time for humans.
- **Use:** `sub_800DD304` (carThrottleSteer) (`car_control.c`). While the turbo input is held
  (ctl +3, human +0xD3: action 0x1D, not frozen) and the meter is > 0, it
  drops by 1 per frame. Store at **800DDCDC** (`sh v1, 50(s8)`, `s8` = the
  weapons block). At 0 the boost stops.
- **Other use:** 50 to put out a fire (800DD9D0, 9.4).
- **AI cars**, and humans with the infinite-weapons cheat (weapons +0 set):
  `sub_8010388C` (weaponTick) (`weapon_tick.c`) resets the meter to 990 every frame.
  Ignore them.

"Turbo from full to empty" is a port rule. Suggested: arm it when the
meter reaches 999 (only by pickups: eight "T" from 200), give it when the
store at 800DDCDC writes 0 while armed. Whether a pickup on the way
disarms it is the port's choice. CONFIRMED (fields and stores).

## 14. Pedestrians by id, and the Antarctica Santa

### 14.1 Which pedestrian

The pedestrians are level objects 514, 516 and 517 in the level file
(`DMD\<level>.DMD`; node byte 0 = 9, u16 +2 = object id, u16 +4 =
**instance**). `sub_800D1DB0` (worldLoadObject) (`world_load_object.c`) passes each to
`sub_80125888(id, instance)` (targetActorSpawn), which fills `D_80190E54[i]` (targetActors) (+4 kind = the
object id, +6 instance) in file order; the world object gets the id
6000 + i. So a pedestrian is named by (level, kind, instance). Kinds 514
and 517 walk between two points (their start rows hold two points); 516
stands at a random start row (LIKELY). No retail level uses 517. CONFIRMED
(level files parsed from the USA disc in `cdimage/`, read-only).

| Level | File | Pedestrians (kind #instance) |
|---|---|---|
| 1 Los Angeles | FREEWAY | 514 #1, 516 #2 |
| 3 Paris | PARIS (2P: PARIS2) | 514 #1, 514 #2, 516 #3 |
| 5 New York | ROOF (ROOF2) | 516 #1 (fixed spot), 516 #2 |
| 6 Antarctica | GLACIER (GLACIER2) | 514 #1 only |
| 8 Hong Kong | HKONG | 514 #1, 514 #2, 516 #3 |
| 11 Cyburbia | BURB | 516 #1 .. #6 |
| 2, 4, 7, 10, 12 | | none |

The "2" files are the split-screen versions of Paris, New York and
Antarctica, used in modes 2 and 4 (`sub_8012A998` (settingsLevelDmdFileName), `screen_card_layout.c`).
They hold the same pedestrians. Which character each one is was not
identified: the files carry no names.

**Hooks** (who did it: section 4):
- **Any kill, any cause:** `sub_80124384(kind, instance, 2)` (objRegistrySetStep) with kind
  514/516/517 and `a2` = 2; `a0` = kind, `a1` = instance. Calls at 80126C78
  (run over by a human), 80127014 (run over by an AI) and 8012733C
  (`sub_80127228` (targetActorHit): weapons and everything else). CONFIRMED.
- Run over: `sub_80126464` (targetActorsUpdate), `s1` = the record (+4 kind, +6 instance).
- Weapons and the rest: `sub_80127228(index, dir)` (targetActorHit), `a0` = the index into
  `D_80190E54` (targetActors). Only the first hit counts (`armed`); the pedestrian comes
  back 300 frames later (`sub_801259A4` (targetActorRespawn)).

### 14.2 The Antarctica Santa

- **PS1 (USA):** Antarctica has one pedestrian (514 #1), in GLACIER.DMD and
  GLACIER2.DMD. No Santa model or name is in the data or in the EXE (no
  "santa", "claus" or "xmas" strings). CONFIRMED.
- **The PS1 code is ready for a second one.** The start table `D_80163DB8` (targetStartPositions)
  (`target_actor_tables.c`) has, for level 6, two rows for walkers and
  **four rows for a standing pedestrian** (kind 516): (1800, 256, 5336),
  (-4648, 1416, 4816), (13704, -2832, 4016), (1504, 664, 4816). No PS1
  Antarctica object uses them. CONFIRMED.
- **The PC version has him.** Its `LEVELDB\GLACIER.DPC` (same node format;
  the root count is at +0x20) holds 514 #1 **and 516 #2**. The model data of
  that 516 differs in size from every other level's 516 (LA, Paris, New
  York, Hong Kong, Cyburbia), so it is a character of its own. LIKELY the
  Santa (the files carry no names).

To restore him in the port, add the PC 516 #2 node to Antarctica. The PS1
code places him on one of the four rows and treats him like any other
pedestrian (6 slots; Antarctica would use 2). His kill is
`sub_80124384(516, 2, 2)` (objRegistrySetStep) on level 6. Discs of other regions were not
checked.

## 15. The Eiffel Tower (Paris)

The tower is level object 3201 in PARIS.DMD and PARIS2.DMD (stored by
`sub_800D6DE8` (eiffelTowerRegister)). Objects 3202..3204 are its three debris emitters; 3200 is
a registry object switched on when it falls. The state is the record at
`D_801AB810` (eiffelTowerState) (`car_object_contact.c`):

| Offset | Field |
|---|---|
| +0x00 (u8) | active (falling) |
| +0x02 (u8) | **done** (the tower is down) |
| +0x08 / +0x0A | frame count / start delay (50 + rand() / 656) |
| +0x0C | the tower object |
| +0x1C | pivot: (0, -6144, 240) |
| +0x28 | rotation (x reaches 1092 when it is down) |

- **Trigger:** only a **remote bomb** explosion. On level 3 both
  remote-bomb explosions call `sub_800D6698(pos)` (eiffelTowerBombCheck): the detonation (801098DC,
  `sub_801096F4` (weaponDropWrite) command 3) and a car driving into the bomb (80109C8C,
  `sub_80109A9C` (weaponDropCheckHit)). Within 1200 of the pivot in x and in y,
  `sub_800D6710` (eiffelTowerStartFall) starts the fall: **active = 1, stored at 800D673C**. Once
  per round (it checks active and done). Missiles and other weapons cannot
  bring it down: there is no other caller. CONFIRMED.
- **Down:** `sub_800D6800` (eiffelTowerUpdate) (run each frame on level 3 only, call at
  800D14A4) tilts the tower after the delay. When the rotation passes 1092
  it stores **done = 1 at 800D69D8**, flashes the screen, calls
  `sub_80124384(3200, 0, 1)` (objRegistrySetStep) and calls `sub_800F600C` (eiffelTowerSetFallenSurfaces) / `sub_800F603C` (aiRoutePatchEiffelTower).
  These change AI path data (LIKELY: open the routes over the fallen tower;
  `sub_800F603C` (aiRoutePatchEiffelTower) is also called when fewer than 3 AI cars are left in
  Paris, at 8011BDF4). CONFIRMED.
- **Reset:** `sub_800D6E4C` (eiffelTowerReset) when the arena is loaded (`arenaSetup`, call at 80119BD8; only
  when the arena changes, not every round: a fallen tower stays down for the next rounds).
- **Who did it:** not stored. The port takes it at the trigger call site:
  the bomb's owner, weapon slot `D_80184B04[i]` (weaponDrops) +6 (slot) / +4 (object id).
  For a bomb driven into, the game credits nobody; the owner placed it and
  the car in `sub_80109A9C` (weaponDropCheckHit)'s hit set it off (the port chooses). CONFIRMED.

Remote-bomb explosions also run level events in Moscow (`sub_800D4504` (moscowEventsTrigger))
and Los Angeles (`sub_800D5584` (laEventsTrigger)). Those events, the tower, the Moscow dish, the New York and
Holland targets and every other arena hazard and destructible are listed (trigger, effects, `rand()`) in
`ARENA_SWITCHES.md` (request 38), with the port's switches that turn them off (`TM2_PORT_HAZARDS`; the tower's
item also skips the "fewer than 3 AI cars" route patch at 8011BDF4).
