# Vanilla Plus balancing

PC port request 40 (2026-10-15), which supersedes request 39's numbers (ram balance; PR #53, never merged, whose
`docs/RAM_BALANCE.md` this document replaces), request 41 (2026-10-16, round 2: the user's answers; section 12), round 3 (2026-10-16: the user's decisions on
round 2; section 13) and request 45 (2026-10-17 playtest: bit 8 split per car, the ricochet's bounce off its owner;
section 14). The user's Vanilla Plus list is a set of gates in the retail functions
under `--port-define TM2_PORT_BALANCE`, one bit per rule in one word, `portBalance` (0 = retail). The port sets all of
them in Vanilla Plus and always online (one ruleset).

Sources: the C in `src/semantic` (file:line references are to this branch), `CAR_STATS.md`, `GAME_EVENTS.md` 2,
`GUN_GAME_SPECIALS.md`, `DARK_TOOTH_PLAYER.md`, `CAR_GROUPS.md`, `RETAIL_BUGS.md`. Addresses are retail. Tags:
**CONFIRMED** = read in the code; **LIKELY** = follows from it, not seen running; **OPEN** = needs a runtime look or a
decision. Nothing was run in an emulator.

Contents: 1 short answer, 2 the word, 3 the rules (3.1-3.12), 4 the gates, 5 `rand()`, 6 interplay, 7 rounding,
8 request 39 folded in, 9 retail notes and future bits, 10 verification, 11 questions, 12 round 2 (request 41: bits
14-19, the corrections, the answers), 13 round 3 (bit 14 removed, bit 16's one hit, bit 20 the machine guns, the
freeze kept on ice, the shadow), 14 request 45 (bits 21 and 22: Mr. Grimm's and Thumper's recharges; bit 18 also
lets the ricochet pass through its owner's car).

## 1. Short answer

- **One word, `portBalance`** (`.port_bss`, every variant), one rule per bit, 0 = retail; Vanilla Plus and online
  set **`0xDFFFFFF`** (`TM2_BAL_ALL`, bits 0-24, 26 and 27, bits 9 and 14 reserved; request 40 had `0x3FFF`, request
  41 `0xFFFFF`, round 3 `0x1FFFFF`, request 45 `0x7FFFFF`, request 67 `0xFFFFFF` (bit 23: section 16), request 77 adds
  bit 24: section 17, request 91 bits 26 and 27: sections 19, 20). The port shows each bit as its own option.
  `roundStart` copies it into `portBalanceLive` before its `srand`
  (`portBalanceLatch`), so a change acts from the next round and both machines run one value per round. Put the
  word in the online ruleset hash.
- **The define is `TM2_PORT_BALANCE`.** Request 39's `TM2_PORT_RAM_BALANCE` is **retired, not aliased**: the
  header stops the build with an `#error` that names the new define (its words are gone and its bit 0 changed
  meaning, so an alias would silently mean something else). Request 39's three gates are now under
  `TM2_PORT_BALANCE`: G1 (no mount) is bits 4, 5, 11, G2 (ram factors) bits 3-5 and 6-7, G3 (the crush word) is
  gone: the mount is retail's 15 again, and Hammerhead's +5 on top is a new gate in the ride.
- **Word zero: retail logic and retail `rand()` order.** No port function calls `rand()`; without the define the
  retail builds are byte-identical (section 10).
- **Implemented:** items 1-6, 7a-7c, 8a-8c, 9, 11, request 39's borrowing-Hammerhead rule (bit 11), 7d with the
  user's option c (bit 12: the catching as retail, the cars still caught thrown out as the tornado ends, 3.7), and 8d
  as a first cut behind bit 13 (kept as is; awaits the port's runtime look). The follow-up (2026-10-15) also applied
  bit 10's surface cap to remote bombs (3.12); bit 9 is **reserved, no effect** (3.11: Sweet Tooth's spiral is fully
  retail; the early-death analysis found one clear mechanism but no small fix that removes it).
- **Round 2 (request 41, section 12):** Hammerhead's recharge 700 (bit 3); bit 12's throw takes the humans by their
  caught flag; six new bits, each its own rule: 14 Grasshopper no hit before the hop, 15 Twister fires nothing while
  her tornado runs, 16 Outlaw 2's taser second chance, 17 freeze diminishing returns, 18 no ricochet self-hit at the
  launch, 19 Grasshopper's free energy jump. Machine guns: no bit (the tracer and the damage are one test, 12.7).
- **Round 3 (section 13):** bit 14 is **reserved, no effect** (Grasshopper's leap is request 40's again: 8a-8c and
  bit 13 as they were, no homing pause); bit 16's late connect now gives the connect's lift and **one** hit of 10;
  **bit 20** (new) takes the machine guns' 50 % coin flip away: every bullet that connects counts, at half its power,
  the half point carried per target car (retail's average exactly); bit 17 stays on ice freezes only; the shadow of a
  mod body (13.5, no code).
- **Request 45 (section 14):** bit 8 is now **Outlaw 2's recharge only**; **bit 21** Mr. Grimm's (1200), **bit 22**
  Thumper's (1500). Bit 18 (the ricochet) now also makes the shot **pass through its owner's car** until it has left
  his box: the hit pass tests every car's hit box, the owner's included, as a wall (a bounce: heading reflected,
  power doubled, the flicker, the push-out), which with request 41's bit 18 carried the shot in front of a fast car;
  retail has the same bounce (14.2).
- **Mines at 16 x 5 = 80: yes** (3.3): 96 drop records (mines in the first 80, remote bombs in the last 16), 80 mine
  objects, the coordinate-system pool 256 -> 320; about 22 KB of port RAM in all, no VRAM.

## 2. The word

### 2.1 Bits (`include/semantic/port_balance.h`, `TM2_BAL_*`)

| bit | mask | item | name | rule |
|---|---|---|---|---|
| 0 | 0x0001 | 1 | `TM2_BAL_MISSILES` | fire missile 7 -> 10, homing missile 10 -> 7 |
| 1 | 0x0002 | 2 | `TM2_BAL_SHIELD` | a shield cast while shielded (or with the shield pool full) is refused: denied sound, no cost |
| 2 | 0x0004 | 3 | `TM2_BAL_MINES` | mines permanent, 5 per driver (a 6th removes that driver's oldest), own range of drop records, refused with no cost when one cannot be placed |
| 3 | 0x0008 | 4 | `TM2_BAL_HAMMERHEAD` | ram damage dealt x1.2; crush 15 at the mount + 5 once on top (20); recharge 1200 -> **700** (request 41 item 1; 40 built 900) |
| 4 | 0x0010 | 5 | `TM2_BAL_AXEL` | never mounts (no crush); ram damage dealt x1.1 |
| 5 | 0x0020 | 6 | `TM2_BAL_MINION` | never mounts (no crush); ram damage dealt x1.1; recharge 360 -> 600 |
| 6 | 0x0040 | 7a-7c | `TM2_BAL_TWISTER` | no mine / remote-bomb placement while her tornado runs; recharge 360 -> 600; no ram damage taken while it runs |
| 7 | 0x0080 | 8a-8b, 121-3 | `TM2_BAL_GRASSHOPPER` | leap damage 15 -> 20 (18 until request 121: the PC version's 20, section 25); no ram damage taken while leaping (8c, the touch, is bit 25 since request 88) |
| 8 | 0x0100 | 9 | `TM2_BAL_RECHARGE` (= `TM2_BAL_OUTLAW2_RECHARGE`) | Outlaw 2's recharge 1100 -> 900 (request 45: Outlaw 2 only; Mr. Grimm's is bit 21, Thumper's bit 22) |
| 9 | 0x0200 | 121-4 | `TM2_BAL_GUN_RANGE` | every machine-gun bullet flies 25 % further: its life 12 -> 15 frames (a player's), 15 -> 19 (a computer car's), every car (section 25). Until request 121 bit 9 was item 10's, **reserved, no effect** (Sweet Tooth's spiral is retail, 3.11) |
| 10 | 0x0400 | 11 | `TM2_BAL_SHADOW` | the raised-surface splash fix: Shadow's detonation min(retail, 25) + 3 (28 max), a remote bomb's min(retail, 25) |
| 11 | 0x0800 | 39 Q1 | `TM2_BAL_HH_BORROW` | a Hammerhead borrowing another special does not mount (needs `TM2_PORT_SPECIAL_AS`; ignored without it) |
| 12 | 0x1000 | 7d | `TM2_BAL_TWISTER_THROW` | option c: her catching as retail (up to 7 catches, 28); when the tornado ends, every car caught at the end is thrown out (retail's intended throw): humans by their caught flag `state.f16`, computer cars by her catch box (request 41 item 5, 12.9) |
| 13 | 0x2000 | 8d | `TM2_BAL_LEAP_LANDING` | Grasshopper's clean landing: set down on the ground, the hop after the hit about half as long (first cut, kept as is: **awaits the port's runtime look**) |
| 14 | 0x4000 | 121-5 | `TM2_BAL_THUMPER_LOCK` | a Thumper fires no special while a flame of his own still burns: denied sound, no cost (section 25). Until request 121 bit 14 was **reserved, no effect** (round 3, 13.1: request 41's "no hit before the hop", removed) |
| 15 | 0x8000 | 41-4, 46-3 | `TM2_BAL_TWISTER_LOCK` | while her tornado runs a Twister fires nothing but her **shield**: every pickup weapon and her special, the machine guns, every energy attack except the shield (103), a remote bomb's detonation; refused with the denied sound, no cost; humans and computer cars (12.3). Request 46 item 3 (15): the shield is let through (request 41 locked it too) |
| 16 | 0x10000 | 41-6 | `TM2_BAL_TASER` | Outlaw 2's taser: a shot that connected to no one may still connect on a later tick of its 40 (the nearest car in range, retail's test); a late connect lifts the victim as a connect at the shot does (`carSpinOut` kind 6, no damage) and deals **one** hit of 10, retail's last tick (round 3, 13.2; request 41 built 2 x 10) |
| 17 | 0x20000 | 41-7 | `TM2_BAL_FREEZE_DR` | freeze diminishing returns: an ice freeze of a car frozen, or unfrozen for at most 20 frames, lasts half its previous one (300, 150, 75, 37, 18, 15 ticks: floor 15); the chain resets after 20 frames unfrozen; per car (12.5) |
| 18 | 0x40000 | 41-9 | `TM2_BAL_RICOCHET` | a ricochet bomb cannot hit its owner before it has once been outside his box (retail: after 2 ticks, a human's; 4, an AI's) (12.6); request 45: until then it also passes through his car's hit box (no bounce, no doubled power, no flicker, no push-out) (14.2) |
| 19 | 0x80000 | 41-10 | `TM2_BAL_FREE_JUMP` | Grasshopper's energy jump (attack 104) costs no energy; its cooldown kept (12.8) |
| 20 | 0x100000 | R3-3 | `TM2_BAL_GUNS` | the machine guns: no 50 % coin flip per connecting bullet; every bullet that connects counts at half its power (a normal bullet 1/2, Mega Guns and Minion's rear gun 3/2), the half point carried per target car: 0, 1, 0, 1 ... (1, 2, 1, 2 ...), retail's average exactly (13.3) |
| 21 | 0x200000 | 45-1 | `TM2_BAL_GRIMM_RECHARGE` | Mr. Grimm's recharge 1400 -> 1200 (bit 8 before request 45) (14.1) |
| 22 | 0x400000 | 45-1 | `TM2_BAL_THUMPER_RECHARGE` | Thumper's recharge 1600 -> 1500 (bit 8 before request 45) (14.1) |
| 24 | 0x1000000 | 77 | `TM2_BAL_HH_MOVE` | Hammerhead's brakes 1 -> 2 and slide speed 25 -> 60 (17) |
| 25 | 0x2000000 | 8c, 88 | `TM2_BAL_LEAP_TOUCH` | Grasshopper touching her leap's target counts as the hit (bit 7's until request 88; **not in `TM2_BAL_ALL`**: the user's, off) (18) |
| 26 | 0x4000000 | 91-2 | `TM2_BAL_SLAM_LOCK` | while his arm and claw special runs, Mr. Slam fires nothing but the machine guns and the shield (19) |
| 27 | 0x8000000 | 91-3 | `TM2_BAL_AI_UNJAM` | a computer car jammed by other cars backs out and goes round the pile (20) |

Bits 12 and 13 are separate from 6 and 7 because they change the feel most (7d the throw, 8d the look); fold them in
later if wanted (the code is the same). Request 41's rules each have a bit of their own (the port shows every bit as
an option); bit 15 overlaps bit 6's 7a (with bit 15 set, 7a's refusals never trigger: the lock refuses first).
`TM2_BAL_ALL` = `0xDFFFFFF` (since request 91; `0x1FFFFFF` since request 77) includes reserved bits 9 and 14 (no
effect). Machine guns: request 41 built no bit (12.7);
round 3's bit 20 removes the coin flip (13.3). Request 45 split bit 8 into bits 8, 21, 22 (one per car, 14.1).

The word is full. Since request 124 a second word, `portBalance2` (`TM2_BAL2_*`, `TM2_BAL2_ALL` = `0xB`), holds
the newer rules, latched with this one (section 26.1).

### 2.2 Storage (`.port_bss`, every variant; `config/US/port_variant_arrays.tsv`, after `portHumanRadarBlink`)

| Name | Type | Written by | Meaning |
|---|---|---|---|
| `portBalance` | `int` | the port | the bits above, 0 = retail |
| `portBalanceLive` | `int` | the C | this round's copy (`roundStart`, before its `srand`); every gate reads it |
| `portBalanceDropSeq[TM2_MAX_DROPS]` | `u32` | the C | per drop record, the placement order of its mine (bit 2: "oldest") |
| `portBalanceDropSeqNext` | `u32` | the C | the last order handed out (never reset: only the order of the live mines matters) |
| `portBalanceRideTop[TM2_MAX_CAR_SLOTS]` | `u8` | the C | per crusher slot, 1 while Hammerhead's +5 on top is owed (bit 3; cleared by the latch) |
| `portBalanceCars[TM2_MAX_CAR_SLOTS]` | `struct TM2PortBalanceCar` (8 bytes; 16 in request 41) | the C | per car slot (cleared by the latch and, with `TM2_PORT_CAR_DROPIN`, when an unload frees the slot): `frozen_at` / `freeze_len` (bit 17: the last frame the car was frozen, `timerFrameCount`; the chain's last ice length in ticks), `gun_half` (bit 20: the half point of gun damage the car still owes, 0 or 1), `guns_refused` (bit 15: the gun button refused last frame). Round 3 removed bit 14's `leap_z0` / `leap_z` / `leap_state` |
| `portBalanceGunObjects` | `u32` | the C | round 3, bit 20: the half point carried for the gun hits that are not cars (targets, the dish, walls), one for all (cleared by the latch) |

They replace request 39's `portRamOptions` / `portRamCrushDamage` / `portRamLive` / `portRamCrushLive` at the same
place; `portRound` (H5) follows them. `portBalanceCars` (request 41) and `portBalanceGunObjects` (round 3) are
defined last, after `portCarUnload`, so nothing before them moves. The port reads the addresses from the link map / `addrmap.tsv` of its build
(section 10.2 lists them for the proof builds).

With `TM2_PORT_BALANCE` and `TM2_PORT_CARS` two retail tables move to `.port_bss` too (tsv groups `drops+` /
`mines+`, moved only when raised, as request 32's pools): `weaponDrops` (80184B04, 25 x 28 bytes -> 6 N) and
`weaponMineObjects` (80184AA8, 23 pointers -> 5 N).

### 2.3 Port functions (`.port_text`, `port_variant_arrays.c`)

None calls `rand()`. Each returns the retail value (or does nothing) when its bit is clear.

| Function | Called by | Does |
|---|---|---|
| `portBalanceLatch()` | `roundStart` | `portBalanceLive = portBalance`; clears `portBalanceRideTop` |
| `portBalanceMissile(kind, d)` | `weaponLaunchProjectile` | bit 0: kind 2 (homing) 7, kind 1 (fire missile) 10; else `d` |
| `portBalanceShieldRefused(shielded)` | `weaponFireEnergyAttack` 103 | bit 1: 1 when shielded or `weaponShieldSlotFind() < 0` |
| `portBalanceMineRefused(car, human)` | `weaponFireEnergyAttack` 101 | 1 when bit 6 and her tornado runs, or bit 2 and the mine cannot be placed |
| `portBalanceBombRefused(car, human)` | `weaponFireSelected` 3 | bit 6: 1 while her tornado runs (placement only) |
| `portBalanceMineSlot(id)` | `weaponDropWrite` 101 | bit 2: removes the driver's oldest at 5, a free mine-range record only with a free mine object, its order noted; else `weaponDropSlotFind()` |
| `portBalanceBombSlot()` | `weaponDropWrite` 3 | bit 2: the bombs' range first, then `weaponDropSlotFind()`; else `weaponDropSlotFind()` |
| `portBalanceMineKeep(kind, timer)` | `weaponDropsUpdate` | bit 2: 1 for a mine at 1 (do not count down) |
| `portBalanceNoMount(slot, type)` | `carCarCollision` | 1 for Axel (bit 4), Minion (bit 5), a borrowing Hammerhead (bit 11) |
| `portBalanceRam(car, human, other, d)` | `carContactRespond` | 0 for a Twister in her tornado (bit 6) or a leaping Grasshopper (bit 7); else x1.2 / x1.1 / x1.1 when Hammerhead / Axel / Minion dealt it (bits 3-5) |
| `portBalanceCrushStart(slot, kind)` | `carCrashStart` | bit 3: a Hammerhead crusher is owed +5 |
| `portBalanceCrushTop(car, human, peer_human, peer_index)` | `carCrushContactUpdate` | the owed +5, once (`carApplyDamage(victim, 5, pair direction, 0, rider)`) |
| `portBalanceCrushEnd(car, human)` | `carCrushContactUpdate` | the ride ended: nothing owed |
| `portBalancePeriod(car, human, period)` | `weaponTick` | the recharge period of the car's special (3.10) |
| `portBalanceLeapDamage(d)` | `weaponHomingLeapUpdate` | bit 7: 20 (18 until request 121) |
| `portBalanceGunLife(life)` | `bulletSpawn` | bit 9 (request 121): the bullet's life x 1.25, rounded (12 -> 15, 15 -> 19) |
| `portBalanceFlameLocked(car, human)` | `weaponFireSelected` (his special) | bit 14 (request 121): 1 = refuse, no cost: a Thumper with a flame of his own burning |
| `portBalanceLeapTouch()` | `weaponHomingLeapUpdate` | bit 25 (bit 7 until request 88): 1 when her contact is a car-to-car record with the leap's target |
| `portBalanceLeapLand()` | `weaponHomingLeapUpdate` | bit 13: at the hit, set down + the hop cut (3.8) |
| `portBalanceSplash(d, big)` | `areaDamage` | bit 10: `min(d, 25) + 3` for Shadow (`big == 0`), `min(d, 25)` for a remote bomb (`big == 1`) |
| `portBalanceTornadoThrow()` | `weaponTornadoUpdate` (its end block) | bit 12: throws out every car caught at the end: humans with `state.f16`, the AI in her catch box (3.7, 12.9) |
| `portBalanceLocked(car, human)` | `weaponFireSelected` (and `portBalanceEnergyLocked`) | bit 15: 1 = refuse, no cost (a Twister whose tornado runs) |
| `portBalanceEnergyLocked(car, human, kind)` | `weaponFireEnergyAttack` | bit 15: as `portBalanceLocked`, but 0 for the shield (`kind` 103, `TM2_BAL_LOCK_FREE_KIND`): request 46 item 3 (15) |
| `portBalanceGunsRefused(car, human, held)` | `weaponTick` | bit 15: the machine guns: 0 fire, 1 refused, 2 refused with the denied sound (a new press) |
| `portBalanceTaserRetry(arc)` | `weaponArcsDraw` | bit 16: an unconnected taser arc searches again (count 39 down to 1); on a find the connect and its lift, no damage: the one hit is the arc's own last tick (12.4, 13.2) |
| `portBalanceFreeze(car, human, len)` | `projectileHitTest` (type 14, four writes) | bit 17: the ice freeze's length (half the chain's last, floor 15) |
| `portBalanceFrozenTick(car, human)` | `humanCarTick`, `aiCarDrive` (each frame frozen) | bit 17: the chain's clock (a new frozen streak after 20 frames starts a new chain) |
| `portBalanceRicochetOwner(shot, retail, dist)` | `weaponRicochetUpdate` (both car loops) | bit 18: the owner may be hit only once the shot has left his box |
| `portBalanceRicochetPass(shot, obj)` | `weaponRicochetUpdate` (after each of its two hit passes) | bit 18 (request 45): 1 = drop the hit when the volume hit is the owner's car and the shot has not yet left his box (no bounce) (14.2) |
| `portBalanceJumpFree(car, human)` | `weaponFireEnergyAttack` 104 | bit 19: 1 for a Grasshopper (no energy taken, none needed) |
| `portBalanceGunHit(hit, power)` | `bulletsMove` (a bullet that connected) | bit 20: the damage to apply now (`power` half points + the target's carried one, the whole points out, the remainder carried); -1 with the bit clear (retail's coin flip and power) (13.3) |

Static helpers: `balType`, `balSlot`, `balTornadoRuns` (her key's tornado state, selected for the test with
`TM2_PORT_CAR_SPECIALS`), `balLeaping` (likewise the leap state), `balMines`, `balMineRecord`, `balCar` (the slot's
`portBalanceCars` entry), `balLeapTouchesTarget` (8c's contact test, bit 7). Round 3 removed `portBalanceLeapArmed`
and `portBalanceLeapHold` (bit 14).

## 3. The rules

### 3.1 Missile swap (bit 0, item 1)

- **CONFIRMED.** `weaponLaunchProjectile` (801052BC, `weapon_launch.c`): case 2 (homing) passes 10 to
  `weaponFireHomingMissile`, case 1 (fire missile) 7 to `weaponFireMissile`. The value is the damage (projectile
  `+0x0A`, applied by `weaponProjectileUpdate`). Turn rates and lives are untouched.
- **Gate:** the two damage arguments go through `portBalanceMissile(2, 10)` / `portBalanceMissile(1, 7)`. Humans and
  computer cars both fire through here.
- **Unchanged:** power missile 15, napalm 10, Mega Napalm 30, freeze 7, every special.
- **`rand()`:** none added. A fire-missile hit (10) now reaches the spin roll (>= 8), a homing hit (7) no longer.

### 3.2 Shield cast while shielded (bit 1, item 2)

- **CONFIRMED.** `weaponFireEnergyAttack` (801046FC, `weapon_special.c`) case 103 takes 400 whenever there are 400,
  but makes a shield only when `energy_shield` is 0 and a shield record is free (4 retail, N with `TM2_PORT_CARS`):
  a recast wastes 400 (`RETAIL_BUGS.md` 23).
- **Gate:** after the energy test, `portBalanceShieldRefused(flags->energy_shield)`: refused with the denied sound
  (`TM2_HUMAN_DENIED`) and no cost; the shared tail clears the request and reloads the cooldown, as every retail
  refusal. It works again once `weaponShieldsUpdate` clears `energy_shield`. A full pool is refused the same way (Q9).
- **Edges:** only humans cast the shield (the AI's energy attacks are 0x64, 0x65, 0x6B). Not enough energy: retail's
  denied path, unchanged.
- **`rand()`:** none.

### 3.3 Mines: permanent, 5 per driver (bit 2, item 3)

Since request 127 the second word's bits 2 and 3 can remove them after a time (27).

**Retail (CONFIRMED):** energy attack 101 (120 energy; the AI only when its `f001 == 2`) -> `weaponDropItem` ->
`weaponDropWrite(101, id, ...)`: a record from `weaponDropSlotFind` (25, shared with remote bombs), an object from
`weaponMineObjectFind` (23), `status = 300` frames, `kind = 20`. `weaponDropsUpdate` counts `status` down and runs
the hit test (`weaponDropCheckHit`: the owner is immune while `status >= 260`, a hit does 15 and frees the record).
Records past the 23rd object keep a stale or null object pointer (`RETAIL_BUGS.md` 22). Every human restart
(`carRestart` -> `weaponDropsReset`, from `roundStart`'s `humanCarsRestart` and a lost life) clears every mine.

**With bit 2:**

| # | Function | Change |
|---|---|---|
| 3a | `weaponDropsUpdate` (80109D58, `fx_sprite_spawn.c`) | `portBalanceMineKeep`: a mine at 1 is not counted down, so it never expires and its hit test keeps running; the owner can hit his own mine after 40 frames, as retail |
| 3b | `weaponDropWrite` (801096F4) case 101 | the record from `portBalanceMineSlot(id)`: this driver's live mines counted (by the record's id `+4`: a human's `TM2_HUMAN_ID`, AI index + 50); at 5 the one with the lowest `portBalanceDropSeq` is removed silently (record and object freed, as the drop-in removes a leaving car's); then the first free record of the mine range, **only when a mine object is free too** (fixes `RETAIL_BUGS.md` 22); its order noted |
| 3c | `weaponDropWrite` case 3 | a remote bomb's record from `portBalanceBombSlot()`: the bombs' range first, then any free record |
| 3d | `weaponFireEnergyAttack` case 101 | `portBalanceMineRefused` before the energy test: when the mine cannot be placed (fewer than 5 of his mines and no free mine record or object) it is refused with the denied sound and **no cost** (the 120 is kept) |

**Ranges:** mines use records `[0, P - R)`, remote bombs `[P - R, P)`, P = the drop records this match
(`TM2_DROPS_LIVE`), R = the remote-bomb objects this match (`TM2_POOL_LIVE(REMOTE_BOMBS)`). With `TM2_PORT_CARS=16`
and the pools live (`portCarOptions` bit 3): 80 + 16. Otherwise retail's sizes: 23 + 2, so bit 2 then caps the mines
at 23 in all (further ones refused with no cost).

**Sizes** (`players.h`; raised only with `TM2_PORT_BALANCE` and `TM2_PORT_CARS`, `--port-limits 2` forces retail's):
`TM2_MAX_DROPS` 6 N (96), `TM2_MAX_MINE_OBJECTS` 5 N (80), `TM2_MAX_COORD_SYSTEMS` 20 N (320, from 16 N). The
allocators use the live sizes (`weaponDropSlotFind`: `TM2_DROPS_LIVE`; `weaponAssetBind`, `weaponMineObjectFind`:
`TM2_MINE_OBJECTS_LIVE`), the loops the maximum (`weaponDropsUpdate`, `weaponRemoteBombFind`, `weaponDropsReset`,
`weaponObjectsDetach`). Every hard-coded 25 / 23 / 24 of the ten functions that use the tables (`retail_relocs.tsv`)
and the port's own `PORT_DROPS` (drop-in) is now a macro. `weaponDropWrite`'s unreachable `weaponDrops[-1]` store
(`RETAIL_BUGS.md` 25) is left out when the table moves.

**Cost at 16 cars:**

| Cost | Amount |
|---|---|
| Tables | `weaponDrops` 96 x 28 = 2,688 B (retail 700) + `weaponMineObjects` 80 x 4 = 320 B (retail 92) + `portBalanceDropSeq` 96 x 4 = 384 B |
| Scene objects | `weaponAssetBind` (asset 0x8A4) makes 80 `csAlloc` objects instead of 23: 57 more x 296 B = 16,872 B, taken from the coordinate-system pool |
| Coordinate-system pool | `csObjectPool` 256 -> 320 entries (+64 x 296 B = 18,944 B): the 57 mine objects plus 7 spare. Nothing else assumes 256: `csAlloc`, the pool, the drop-in's headroom test (`DROPIN_CS_HEADROOM`) and the tsv group `coordsys` all use `TM2_MAX_COORD_SYSTEMS`. A full pool would return 0 to a caller that dereferences it, so the 16-car peak (`csPoolCount`) is worth one runtime look (Q3) |
| VRAM | none: one model and texture, already loaded |
| Draw | up to 80 small models in the scene list (hidden ones are skipped) |
| Per frame | `weaponDropsUpdate` walks 96 records; each live mine calls `projectileHitTest` (about 16 cars + 50 breakables + 6 spinners): about 7k max-abs distance checks at 96 live, negligible on PC |
| Port RAM in all | about 22 KB (2,688 + 320 + 384 + 18,944) |

- **Edges:** 5 per driver times the drivers is at most 5 N, so placement fails only on the range or object limit. A
  car swapped to another type keeps its id and its mines; a record reused by a new driver inherits the old driver's
  mines and quota (rare). Remote bombs as retail (0x7FFF life, one per car through `drop_armed`). Round restart
  clears every mine (retail, unchanged); a drop-in join does not (`portCarStartHuman` skips it, as before).
- **`rand()`:** none added. More live mines mean more hits; each hit's 15 reaches the spin roll.

### 3.4 Hammerhead (bit 3, item 4)

- **x1.2 dealt** (G2, `carContactRespond`): request 39's factor, in `portBalanceRam`.
- **Front ram vs on top (CONFIRMED: two distinct code events).** The **mount** is `carCrashStart` (800E6D14), once per
  contact when a `mounts_cars` car hits with its front moving forward (or its back reversing): a human pays a charge,
  the victim is frozen 30 and spun, one `carApplyDamage(..., 15)`. The **ride** is `carCrushContactUpdate` (800F39D0,
  `proximity_update.c`), every frame while `crush_contact` holds: he is **on top** when his nearest wheel is within
  half the crush range of the victim's centre (sparks, the `(rand() & 15)` explosion, `hp -= 1` every 4th tick).
- **Built (Q2 default):** the mount stays 15 (retail's `carCrushDamageAmount`; request 39's crush word is gone);
  `carCrashStart` marks a Hammerhead crusher (by car type) as owed (`portBalanceCrushStart`); the first frame he is
  on top, `portBalanceCrushTop` applies **+5 once** (`carApplyDamage(victim, 5, pair direction, 0, his slot)`, as the
  mount's call: flash, shields, the AI-vs-AI /4 = 1, kill credit). The flag clears when the ride ends
  (`portBalanceCrushEnd`) or at the next round. So 15 if he slides off without getting on top, 20 if he gets on top.
  The ride drain stays retail.
- **Recharge 1200 -> 700** for humans (3.10; request 41 item 1, as Axel's; request 40 built 900). Computer cars
  never recharge: they have full ammo every tick.
- **Edges:** an AI Hammerhead still mounts for free (retail: only humans pay); with 0 charges a human does a plain ram;
  a borrowing Hammerhead mounts unless bit 11 (3.9), and keeps x1.2 and the +5 (they follow his car type).
- **`rand()`:** x1.2 makes 7 -> 8 reach the spin roll. The +5 itself rolls nothing (5 < 8); if it kills the victim,
  `carApplyDamage`'s wreck path runs as for any kill (its `carSpinOut` draws a `rand()`), deterministic (5).

### 3.5 Axel (bit 4, item 5)

Request 39 unchanged: never mounts (G1 `carCarCollision`: no `carCrashStart`, so no freeze, 15, spin-out, ride or
its 8 wheel-shake `rand()`), ram damage dealt x1.1. Human or computer Axel, borrowing or not.

### 3.6 Minion (bit 5, item 6)

`mounts_cars` is 1 for 0xE0. Built as Axel: never mounts, x1.1 dealt; recharge 360 -> 600 (3.10). Every Minion,
the Juggernaut's included (Q12). The Minion's second root draws only; the record type stays 0xE0.

### 3.7 Twister (bits 6 and 12, item 7)

"Her tornado runs" = her key's `weaponTornadoState.timer > 0` and its `object` is her car object. With
`TM2_PORT_CAR_SPECIALS` the first Twister's state is retail's global and a duplicate's is its `portCarSpecials`
entry, selected for the test (`balTornadoRuns`), so twins are separate. Twister's special cannot be borrowed, and a
Twister borrowing another special never runs a tornado.

- **7a (bit 6). No placement while her tornado runs:** mines (`weaponFireEnergyAttack` 101, AI too) and remote
  bombs (`weaponFireSelected` case 3 with `drop_armed == 0`, before `drop_armed`, `ammo` and `held` change), refused
  with the denied sound and no cost. **Detonating** a bomb placed before stays allowed (Q4).
- **7b (bit 6). Recharge 360 -> 600** (3.10).
- **7c (bit 6). No ram damage taken while it runs** (`portBalanceRam` returns 0: `carApplyDamage(..., 0)` flashes
  and returns before the spin roll). The other car's share stays. LIKELY: her hidden body still collides (the
  object keeps `collides`); a runtime look.
- **7d (bit 12): the throw, option c (the user, 2026-10-15).** The retail bug (`RETAIL_BUGS.md` 21):
  `weapon_tornado.c` tests `freeze_timer == 1` (both loops), which never happens: the catch sets 60, the hit cuts it
  by 6 x 4 = 24, and the car ticks subtract 3 a frame (36, 33, ..., 3, 0). The order per frame is car ticks, then
  `weaponTornadoUpdate`. Of the three options (a: one catch and a throw at the thaw, 4 damage; b: a with more damage
  per catch; c: retail's catching, the throw at the end) the user chose **c**. As built:
  - **The catching is retail's, unchanged.** Both loops, the catch (`frozen`, `freeze_timer = 60`, `carSpinOut`,
    4 damage), the re-catch after each thaw (up to 7 catches, 28), the orbit, the effects and their `rand()` calls,
    and the two dead `freeze_timer == 1` tests stay as they are (`tools/check_balance.py` G14 counts retail's two
    `carSpinOutById` calls in the function).
  - **When.** Once, in the frame her tornado ends, as the last part of its end block: the block runs when the
    timer reaches 0 (`weaponFireSpecial` 0xA0 sets it to 0x53: 83 updates, about 4.2 s at 20 a second), when her HP (`*anchor`) is 0 or less, or when she is frozen
    (the three retail end tests, unchanged). The call `portBalanceTornadoThrow()` comes after the block's own work
    (tornado and funnel objects off, her body shown or hidden, `fxSmokeB24`) and before it clears the humans'
    caught flags (`state.f16`). So the order of that frame is: car ticks, the tornado's loops (catch, orbit), the
    end block, the throw.
  - **Which cars (request 41 item 5: the cars caught at the end).** Humans: every human whose caught flag
    `state.f16` is set, retail's own flag, which this tornado's human loop sets (1) or clears (0) every update by its
    catch test `DISTANCE(her car object, the car's motion.pos) < 35` (max-abs distance >> 3, 280 units; her own car
    never) before the orbit: in the end frame the loops ran just before the end block, so `f16` is "caught this
    frame", and the end block clears it after the throw. Computer cars (no such flag): the same catch test on the
    positions this frame's orbit left (as request 40 built it for both). Humans first (index order), then the
    computer cars (index order). A car frozen by something else (a freeze missile) but caught is thrown too, as
    retail's `freeze_timer == 1` test would throw any car orbited by her at that moment. Another Twister caught
    (twins, or a human Twister) is thrown like any car; her own car never. (Twins: the end block runs inside the
    ending Twister's own update, right after her own loops, so `f16` is hers then.)
  - **How.** Exactly retail's intended throw: `carSpinOutById(the car's key, heading, 0)` with the heading of
    retail's push vector, `ratan2(car x - centre x, car y - centre y)` (`tm2_heading`: a y of 0 counts as 1), the
    centre being the tornado object's position (her position; its z was raised by 104 in the end block, which the
    heading does not use). With `TM2_PORT_CAR_GROUPS` the key is `TM2_CAR_KEY(slot, type)` (retail's loops use the
    same), without it the car type. `carSpinOutById(..., 0)` is a forward spin-out: speed `1330 x ticks / 100 x 32`
    (1248, about 39 units a frame, at 3 ticks), body heading = away from her, contact timer 180, shake, spin direction from
    the heading difference.
  - **Left as is.** No new damage, no change to the freeze: a thrown car keeps the rest of its catch's freeze
    (at most 36 ticks, 12 frames, 54 for AI vs AI), during which retail's frozen car brakes (`car_control.c`), so
    the throw may be shorter than one at the thaw would be: a runtime look (Q7).
  - **Twins (33-2a).** The call runs inside the ending Twister's selection (with `TM2_PORT_CAR_SPECIALS` her body
    `weaponTornadoUpdateOne` runs once per Twister in key order with her state as the retail global), so each
    tornado throws only from its own box when it ends. A car in two boxes is thrown by each tornado that ends.
  - **`rand()`:** no new call site. `carSpinOutById` draws retail's own calls (its `carSpinOut` kind 1, a `rand() &
    7` unless the catch's spin still runs, and `carShakeStart` / `carShakeRandomize`): the calls retail's intended
    throw would draw, once per thrown car, in a fixed order. `portBalanceTornadoThrow` itself calls none; the check
    takes its edge to `carSpinOutById` out of the no-`rand()` proof (as `portBalanceCrushTop`'s).

**The user's tornado questions (answers):**

- **Do several caught cars add damage?** No. Each caught car takes its own 4 per catch. The orbit
  (`weapon_tornado.c`, both loops) is one rotation of every caught car's offset about her (`weaponTornadoRotation`
  {0, 0, -341}): a rigid turn, so the caught cars keep their spacing and orbit together; they are not driven into each
  other. Ram damage comes from each car's own movement (`wheels.move`, its velocity in `carUpdateContacts`), not from
  the teleport, so two caught cars that already touch can trade small rams, and contact with her is a normal ram
  scored by her own speed (7c removes her share). LIKELY, not measured.
- **Does a caught car hitting a wall take damage?** The orbit positions are not checked against walls in
  `weaponTornadoUpdate`. The next frame's car tick runs the usual wall probes and pushes the car out. Wall damage
  (`car_contacts.c`) is for humans only (an AI car bumps) and speed-based (`car_contact_resolve.c`: 1, 3, or up to 3..6
  x speed / cap), and a caught car's own speed is small: about **1 point for a human**, none for the AI. LIKELY.
- **Retail damage:** 4 per catch, up to 7 catches = 28 (`CAR_STATS.md` 6, corrected); AI vs AI 1 per catch.

### 3.8 Grasshopper (bits 7 and 13, item 8)

**CONFIRMED.** Launch: `weaponFireSpecial` 0x60 -> `carSpinOut(kind 7)` (the hop: wheel upward speed `448 x 2/3`,
bounce counters with `rand()` picks) and `weaponHomingLeapInit` (the nearest car within 400 and about 60 degrees
ahead). `weaponHomingLeapUpdate` (8010F520) moves **her** position toward the target's `motion.pos`; a hit when
`DISTANCE < 15` (120 units): 15 damage. Cancel at age 101, when she touches anything (`*cancel` = her
`contact.in_contact`), or when she lost >= 6 HP since the launch.

- **8a (bit 7).** The hit's damage through `portBalanceLeapDamage(15)`: 18. Both >= 8: no `rand()` change.
- **8b (bit 7).** `portBalanceRam` returns 0 while her key's leap is active and hers (`balLeaping`); this also stops
  her own ram damage from setting off the ">= 6 HP lost" cancel.
- **8c (bit 25 since request 88, section 18; bit 7 before).** Before the distance test, when the distance is not yet < 15 and `*cancel` is set,
  `portBalanceLeapTouch()` checks her contact: a car-to-car record (`hit.f01 == 0`) whose `other` car's `motion.pos`
  is the leap's target pointer (exact, no new distance). Then the hit block runs (18, effects, the turbo flame off)
  and the cancel test that follows skips its explosion. A contact with another car or a wall cancels as retail.
  The ram on that contact was already applied by the car tick (the target's share stands, hers is 0 with 8b).
  A touch before age 8 passes the hit's uninitialised direction vector, as a retail early hit does (it is used only
  if the hit kills).
- **8d (bit 13, first cut; kept as is by the user, 2026-10-15: awaits the port's runtime look).** Retail leaves her where the homing put her: at the target's
  origin height, which is the target's ride height above the ground (Hammerhead 66, Minion 27, Dark Tooth 116
  against her 17), so she ends in the air or inside the target. At the hit `portBalanceLeapLand()`:
  - sets her down: `z = worldGroundHeightAt(her pos + 24) + her ride height` (the probe `weaponDropItem` uses; the
    grounded-car relation) when she is above it, moving her wheels with her (as `carUpdateContacts` does);
  - cuts the hop: each wheel's upward speed (`fall` > 0) to a quarter. In `carWheelVerticalMotion` the lift is the
    speed and the fall grows with the square of the air counter, so the time in the air goes as the square root of
    the speed: a quarter is about half as long. Whether that reads smooth (not choppy) on a long leap (the kind-7 hop
    starts at the launch and may be over by the hit) is the runtime question (Q10).
- **Twins:** per key (the 33-2a loops). Grasshopper's special cannot be borrowed.

### 3.9 Borrowing Hammerhead (bit 11, request 39 bit 1)

With `TM2_PORT_SPECIAL_AS`: a Hammerhead whose special is borrowed (`portCarSpecialAs[slot] > 0`, Gun Game) does not
mount, so he gets no free crush (he pays no charge while borrowing). Ignored without `TM2_PORT_SPECIAL_AS`.

### 3.10 Recharges (bit 8, item 9, and the recharge parts of bits 3, 5, 6)

- **The values** (`weaponBlockInit`, `ai_route_setup.c`; CONFIRMED): Hammerhead 1200, Outlaw 2 1100, Mr. Grimm 1400,
  Thumper 1600, Twister 360, Minion 360 (frames, 20 a second); Axel 700 (unchanged).
- **The only reader** is `weaponTick` (8010388C, `weapon_tick.c`): `if (weap->f2E++ >= weap->recharge_period)`.
  The gate is there: `portBalancePeriod(car, human, weap->recharge_period)` returns Hammerhead 700 (bit 3; request 41,
  `TM2_BAL_HH_RECHARGE`), Minion
  600 (bit 5), Twister 600 (bit 6), Outlaw 2 900 (bit 8), Mr. Grimm 1200 (bit 21), Thumper 1500 (bit 22; request 45
  split bit 8 per car, 14.1), by the **special's** type
  (`TM2_SPECIAL_TYPE`: a borrowed special's with `TM2_PORT_SPECIAL_AS`), else the stored value. A human Dark Tooth's
  (`portDtPlayer`) is never touched.
- **Why at the reader:** humans keep their weapons block between rounds and computer cars are set up at bind time,
  before `roundStart`'s latch; a write-site gate would use a stale latch. The reader acts from the round the latch
  changes.
- **Computer cars never recharge** (CONFIRMED): `weaponTick` sets every AI ammo count to 1 (and energy to 1000) every
  tick, so these rules change **human** cars only (and a human with the `infinite` flag already has full ammo).
- **`rand()`:** none.

### 3.11 Sweet Tooth's spiral: early deaths (bit 9, item 10: reserved, no effect)

**The user (2026-10-15, clarified):** the worry is spirals that **die right when fired**, not spirals flying under
the ground. Request 40's first cut (speed cap 100, turn rate 20) and the follow-up's ground check
(`portBalanceSpiralGround`: kill the spiral below the drivable ground) are both **removed**: the ground check could
only add early deaths (a spiral just under the probed surface on a slope dies at once). Bit 9 has **no effect**;
Sweet Tooth's spiral is retail with every bit set. The question asked: why can a spiral die right after launch, and is
there a small safe fix (a launch grace or a higher launch, request 40 item 10)? **Finding: one clear mechanism, but no
small fix that removes it; bit 9 stays reserved** (below).

**Retail (CONFIRMED in the C).**
- **Launch** (`weaponFireSpecial` case 0x10, `weapon_basic.c`): the muzzle is the car's origin + its matrix x (0, 75,
  9) (gun y / z, `CAR_STATS.md` +218 / +220) + 24 + 40 in z. Sweet Tooth's origin is his ride height (32) above the
  road, so a level car launches the spiral **about 105 above the road**, 75 ahead of his origin.
- **Record** (`weaponFireSpiralMissile` 800D9C5C over `weaponFireHomingMissile`): type 61, damage 15, turn rate 15,
  speed 37 (+5 a frame, capped at 75 for type 61), life 50 x 4 = **200** frames (10 s at 20 frames a second). Nothing
  there can end it early.
- **Pitch** (`weaponProjectileUpdate` 800DB110): every frame the pitch is **set** (not added) to
  `284 x sin(life x 300) / 4095` (about +-25 degrees, a period of about 13.7 frames), then `weaponProjectileSteer`
  turns it at most 15 toward the target's pitch, which the next frame overwrites again. A positive pitch points the
  nose down (`mathMulTransVec` of `RotMatrixYXZ`: z step = -speed x sin(pitch); the steer gives a higher target a
  negative pitch). So the spiral's height **does not follow the ground or the target**: it flies a fixed wave whose
  phase is set by the life count.
- **The wave from launch** (no steering; the steer's +-15 changes it by about 5 %): up about 20 in frames 0-2, then
  **down to about 90 below the muzzle at frames 8-9** (about 560 units ahead, 0.4-0.45 s after the shot), back to the
  muzzle's height by frame 13, then on between about 90 below and 74 above it (troughs every ~900 units). Its lowest
  points are therefore **about 15 above a flat road**, for its whole flight.
- **Tests:** `projectileHitTest` (cars, breakables, spinners: a box of (radius + 1) x 8 = **128** units around the
  test point for a human's shot, 72 for a computer car's; its own car excluded, `hit == owner -> 0`) at both
  half-steps, `hdpQueryPoint` (the level's hit volumes, tags 3301, 2, 667, 668, 401..440, 3500..3509 and `-owner`
  ignored) at the step's midpoint. The road's volumes start 0-8 units under the drivable surface at most route points
  (`RETAIL_BUGS.md` 26). There is no self-hit and no timer bug.

So a spiral dies right after launch (a) at its first trough, 0.3-0.5 s out, when the road ahead rises by more than its
~15 units of margin (a climb of more than about 3 % over 560 units, a crest, the car pitched nose-down, a kerb or a
ramp), or (b) within a frame or two at the muzzle when a wall, a prop, a breakable or another car is within reach of
the first test points (128 units for a human's shot: every missile type shares this).

**Offline estimate (CONFIRMED in the files, LIKELY in game).** The offline copy of the hit query (`RETAIL_BUGS.md` 26)
flew the retail spiral (its wave, the +-15 steer toward a target 3000 ahead at car height) from Sweet Tooth at every AI
route point of the 11 distinct level DMDs, toward each neighbouring point, straight along that line: 1198 launches.
- **The margin is thin everywhere:** of the 984 spirals still alive at frame 13, 767 passed within 32 units of a
  volume top in their first trough (757 at 16-31, 10 under 16).
- **Deaths in frames 0-13: 214 (18 %).** 26 at frames 0-3 (the muzzle in or at geometry); 188 at frames 4-13: 60 into
  a surface's top (the point 40 higher is clear: the trough meeting the ground) and 128 into walls or higher geometry
  (mostly the model's straight line leaving the road). By the segment's slope: uphill steeper than 2 %, 34 of 62
  launches die at frames 4-13; flat, 140 of 1074; downhill, 14 of 62.
- **Of the 60 trough deaths only 12 meet ground within 48 units of the launch ground;** 22 meet a surface 48-96
  higher and 26 one more than 96 higher (kerbs, platforms, ramps and steps 300-700 units ahead: Hong Kong, Islands,
  Roof Tops, Freeway).

**The fixes tried on those 60 (same model):**

| fix | still dead at frames 0-13 | dead at 14-20 | alive at frame 40 |
|---|---|---|---|
| retail | 60 | 0 | 0 |
| launch 24 higher | 47 | 7 | 1 |
| launch 40 higher | 41 | 7 | 5 |
| launch 64 higher | 34 | 10 | 7 |
| grace: frames 0-13, a world hit ignored when the point 48 higher is clear | 37 | 12 | 5 |
| the same, 64 higher | 34 | 14 | 6 |

Over all 1198 launches the deaths at frames 0-13 go from 214 to 200 (24 higher), 201 (40 higher) or 178-184 (the
graces), and the spirals alive at frame 40 from 696 to 702-705. Because the wave never climbs, a fix for the first
frames mostly moves the death to the end of the grace or to the next trough while the ground keeps rising; a higher
launch lifts the whole flight (a computer car's spiral then spends more of its wave above the 72-unit car box: its
mean height is about 97 above the road, a target's origin 16-32). A pitch that follows the target (the wave added to a
steered pitch) changes the flight, which the user ruled out, and in the model dies more (108 of 1198 alive at frame
40), as it dives toward the target's car height.

**Decision: bit 9 reserved (no effect), Sweet Tooth fully retail.** The mechanism is clear (the first trough about
15 units over a flat road, and the muzzle's 128-unit car/prop box), but the estimate does not show a small, safe fix
that keeps the spiral alive: a grace or a higher launch of the sizes asked saves 1-3 % of launches in the model, most
for only a few frames, and the model cannot tell how many of the user's early deaths are of this kind. **For the
port's runtime look:** log a spiral's `life` and `hit` / `obj->kind` when it dies. Deaths at `life` 189-195 (frames
4-10) with world hit 8 on rising ground are the trough; deaths at `life` 198-199 (frames 0-1) are the muzzle (a car
id or 500+ is a car or a breakable inside 128 units, hit 8 a wall). If the trough dominates, the mod's higher launch (or the graze
grace above) can come back as bit 9 with these numbers; the bit and its place in `TM2_BAL_ALL` are kept for it.
- **Dark Tooth's** type-61 missiles (rate 20 or 99, life x 5) are untouched as before.
- **`rand()`:** nothing (no code).

### 3.12 The raised-surface splash: Shadow and remote bombs (bit 10, item 11)

- **CONFIRMED.** `areaDamage` (8010B5C4, `area_damage.c`): `25 - d/4` (radius 75, or 100 when `big`), `50 - d/4`
  when the car's `state.on_surface` is set (a raised surface polygon: ramps, roofs). Its three call sites (all
  checked, retail jal relocations too): Shadow's detonation (`big = 0`, `weaponFireSpecial`, `weapon_basic.c`) and
  the remote bomb twice (`big = 1`: the owner's detonation in `weaponDropWrite` case 3, `weapon_slot_writer.c`; a car
  driving into it in `weaponDropCheckHit`, `car_damage_human.c`). Every car's amount goes through one of the four
  `carApplyDamage(..., SPLASH(...))` calls, so the gate covers every caller; nothing else adds the surface bonus.
- **Built:** `portBalanceSplash(d, big)` caps a car's amount at 25 (the off-surface maximum), so the surface case
  can no longer exceed the flat ground's:
  - **Shadow** (`big == 0`, the user's "cap so the max is 25, then +3"): `min(d, 25) + 3`: **28 flat on a surface
    polygon**, `28 - d/4` off it (28 max; retail 50 - d/4 / 25 - d/4).
  - **Remote bomb** (`big == 1`, the user, 2026-10-15): the same cap and **no +3** (the +3 was Shadow's
    compensation for losing the bonus; the bomb's damage was not asked to change): `min(d, 25)`: **25 flat on a
    surface** (anywhere within its radius 100, where retail gave 50 - d/4, 26..50), `25 - d/4` off it (as retail).
    The equivalent rule: on a surface the bomb deals what it deals at its centre on flat ground.
  - The strike's direct hit stays 25 (`strike_update.c`), breakables (25) and spinners as retail.
- **`rand()`:** Shadow: near the centre both >= 8; at the edge `28 - 18 = 10 >= 8` where retail's off-surface 7 was
  not, so the spin roll now runs there. Remote bomb on a surface: 25 >= 8 as retail's 26..50 (rolls as before).

## 4. The gates (game functions that change)

With `TM2_PORT_BALANCE` alone, exactly these 20 functions (`tools/check_balance.py --expect-functions`; G12 and G15
change none; request 41 added `weaponArcsDraw`, `projectileHitTest`, `humanCarTick`, `aiCarDrive`,
`weaponRicochetUpdate`; round 3 `bulletsMove`):

| # | Function (retail VA) | File | Change | Bits |
|---|---|---|---|---|
| G0 | `roundStart` 8011A024 | `scene_init.c` | `portBalanceLatch()` before the `srand` (after request 38's latch) | all |
| G1 | `weaponLaunchProjectile` 801052BC | `weapon_launch.c` | the homing / fire missile damage through `portBalanceMissile` | 0 |
| G2 | `weaponFireEnergyAttack` 801046FC | `weapon_special.c` | 103: `portBalanceShieldRefused` after the energy test; 101: `portBalanceMineRefused` first | 1, 2, 6 |
| G3 | `weaponFireSelected` 80103FB8 | `weapon_select.c` | case 3: `drop_armed == 0 && portBalanceBombRefused` -> denied, return | 6 |
| G4 | `weaponDropWrite` 801096F4 | `weapon_slot_writer.c` | 101: `portBalanceMineSlot(arg1)`; 3 (placement): `portBalanceBombSlot()` | 2 |
| G5 | `weaponDropsUpdate` 80109D58 | `fx_sprite_spawn.c` | `if (!portBalanceMineKeep(kind, t)) timer--` | 2 |
| G6 | `carCarCollision` 800E184C | `car_collision.c` | `st->mounts_cars && !portBalanceNoMount(id2, mode)` | 4, 5, 11 |
| G7 | `carContactRespond` 800E3800 | `car_contacts.c` | the `f01`-clear branch passes `portBalanceRam(car, human, c->hit.other, c->hit.damage)`; the repeat guard keeps the raw value | 3-7 |
| G8 | `carCrashStart` 800E6D14 | `car_crash.c` | `portBalanceCrushStart(id, kind)` before the mount's damage (which is retail's again) | 3 |
| G9 | `carCrushContactUpdate` 800F39D0 | `proximity_update.c` | on top: `portBalanceCrushTop`; ride ended: `portBalanceCrushEnd` | 3 |
| G10 | `weaponTick` 8010388C | `weapon_tick.c` | the period through `portBalancePeriod` | 3, 5, 6, 8, 21, 22 |
| G11 | `weaponHomingLeapUpdate` 8010F520 | `weapon_leap_homing_update.c` | touch, damage, landing (with `TM2_PORT_CAR_SPECIALS` in its body `weaponHomingLeapUpdateOne`) | 7, 13 |
| G12 | `weaponProjectileUpdate` 800DB110 | `projectile_hit.c` | none: the file is retail again (bit 9 reserved); `check_balance.py` proves no `portBalance` call there | (9) |
| G13 | `areaDamage` 8010B5C4 | `area_damage.c` | the four amounts through `SPLASH` = `portBalanceSplash(d, big)` (Shadow and remote bombs) | 10 |
| G14 | `weaponTornadoUpdate` 8010E040 | `weapon_tornado.c` | the end block's last act: `portBalanceTornadoThrow()` (with `TM2_PORT_CAR_SPECIALS` in its body `weaponTornadoUpdateOne`) | 12 |
| G15 | `weaponHomingLeapUpdate` 8010F520 | `weapon_leap_homing_update.c` | none since round 3 (bit 14 reserved): the file is request 40's again (G11 only); `check_balance.py` proves no `portBalanceLeapArmed` / `portBalanceLeapHold` call and neither linked | (14) |
| G16 | `weaponFireSelected` 80103FB8, `weaponFireEnergyAttack` 801046FC, `weaponTick` 8010388C | `weapon_select.c`, `weapon_special.c`, `weapon_tick.c` | `portBalanceLocked` before the cooldown reload (pickups) / `portBalanceEnergyLocked` (request 46: the shield let through) before the energy test (energy attacks): denied, return; `portBalanceGunsRefused` before the gun block takes `fire_guns` | 15 |
| G17 | `weaponArcsDraw` 801084D8 | `weapon_arcs_draw.c` | an arc with no target: `portBalanceTaserRetry(i)` before the free end and the count | 16 |
| G18 | `projectileHitTest` 800D6FD8, `humanCarTick` 800DC88C, `aiCarDrive` 800E7EDC | `weapon_slam_swing.c`, `car_tick.c`, `ai_driver.c` | the four type-14 `freeze_timer = 300` through `portBalanceFreeze`; `portBalanceFrozenTick` at the top of the frozen block | 17 |
| G19 | `weaponRicochetUpdate` 8010CC54 (+ `weaponFireSelected` case 6) | `weapon_ricochet.c` (`weapon_select.c`) | the owner clause `life < 398` / `< 396` through `OWNER_HIT` = `portBalanceRicochetOwner`; the launch clears `left_owner`; request 45: after each `hdpCollideObject` (and its kind filter) `if (portBalanceRicochetPass(&SHOT, hit object)) hit.hit = 0` | 18 |
| G20 | `weaponFireEnergyAttack` 801046FC | `weapon_special.c` | the 0-energy return, case 104's cost test and subtraction through `portBalanceJumpFree` | 19 |
| G21 | `bulletsMove` 800D7E30 | `weapon_bullets.c` | the hit's coin flip and power through `GUN_FLIP` / `GUN_POWER` = `portBalanceGunHit` (the `rand() & 1` only when it returns -1) | 20 |

With `TM2_PORT_CARS` the raised tables also change `weaponDropSlotFind`, `weaponRemoteBombFind`, `weaponDropsReset`,
`weaponObjectsDetach`, `weaponAssetBind`, `weaponMineObjectFind` (sizes only) and the coordinate-system pool's users.

## 5. `rand()`

No `rand()` call site is added; with the word zero the stream is retail's. With bits set it differs only where a rule
makes one of retail's own calls run or not run, the same on every machine for the same word:

- the spin roll in `carApplyDamage` (`amount >= 8 && (rand() & 7) < amount - 7`): bits 0 (10 rolls, 7 does not), 3-5
  (scaled rams crossing 8), 10 (the edge of Shadow's splash), and every hit that no longer happens or newly happens
  (7c, 8b: 0 damage returns before it; 8c: a touch is now a hit; 2: more mines, more hits);
- an Axel / Minion / borrowing Hammerhead that would have mounted skips `carCrashStart`'s 8 wheel-shake calls, the
  crush's spin roll and every riding frame's `(rand() & 15)` (bits 4, 5, 11);
- bit 12's throw calls retail's `carSpinOutById` once per car in her box at the tornado's end: its `carSpinOut` and
  car shake draw the `rand()` calls retail's dead `freeze_timer == 1` throw would draw (a fixed order: humans, then
  the AI by index). `tools/check_balance.py` takes that edge (`portBalanceTornadoThrow` -> `carSpinOutById`) out of
  its no-`rand()` proof and checks it is the function's only call of it;
- Hammerhead's +5 (bit 3) rolls nothing (5 < 8); a kill by it runs `carApplyDamage`'s wreck path (`carSpinOut`, one
  `rand() & 7`) as any kill does. `tools/check_balance.py` takes that one edge (`portBalanceCrushTop` ->
  `carApplyDamage`) out of its no-`rand()` proof and checks the amount is the constant 5;
- 8d moves her and cuts her wheels' speed: the hop's own `rand()` picks were made at the launch; later wheel code may
  take other branches.
- request 41 (12.11): no new call site. Bit 14 moves a leap hit (18: the spin roll) later or drops it; bit 15 skips
  every refused weapon's own calls (a bullet's two coin flips, a launch's); bit 16's late connect runs retail's
  connect calls (`carSpinOut` kind 6: `rand() & 7`; the first 10 through `carApplyDamage`: the spin roll), the calls a
  connect at the shot runs (`check_balance.py` takes `portBalanceTaserRetry`'s two edges out of the no-`rand()`
  proof, each called once, the amount the constant 10); bit 17 changes lengths only; bit 18 skips a self-hit's calls
  (its `carSpinOut` kind 4) and, since request 45, a self-bounce's flicker (`lightFlickerTick` draws `rand() % 3`
  each frame of a running flicker timer: a first bounce off the owner no longer starts one); bit 19 none.
- round 3 (13.6): bit 14 reserved (no calls moved); bit 16's late connect runs only the connect's `carSpinOut` kind 6
  (one `rand() & 7`; `check_balance.py`: `portBalanceTaserRetry`'s one edge out, no `carApplyDamage` call), the arc's
  last-tick hit its spin roll as retail's; **bit 20 removes a retail call**: `bulletsMove`'s second `rand() & 1` (the
  damage coin flip before `weaponApplyHit`, `weapon_bullets.c:300`) is not called for any bullet that connects (a
  car, an object or the level); the first (`rand() & 1` for the smoke puff, line 294) stays.

## 6. Interplay

- **`TM2_PORT_SPECIAL_AS` (request 35):** the ram factors and no-mount follow the **car type** (Axel or Minion
  borrowing still never mount; a car borrowing Axel's shockwave or Minion's volley gets no x1.1); the recharges and
  Shadow's splash follow the **special** (`TM2_SPECIAL_TYPE`, `big == 0`). Twister and Grasshopper
  cannot be borrowed. Bit 11 stops a borrowing Hammerhead's free mount.
- **`TM2_PORT_DT_PLAYER` (request 36):** a human Dark Tooth is untouched: he mounts as retail (15 x2, the ride drain),
  his type-61 missiles are retail, his recharge is `portDtPlayer`'s.
- **`TM2_PORT_CAR_SPECIALS` / twins (33-2a):** the tornado and leap tests read the car's own key's state (a selection
  for the test; the first car of a type is retail's global). Mines are per record id, the +5 per car slot.
- **16 cars / 16 humans:** the drop and mine tables scale with N; ids are unique per record; the denied sound goes
  through `TM2_HUMAN_DENIED` (H3).
- **`TM2_PORT_CAR_DROPIN`:** `portCarRemove` clears a leaving car's drops by id over `TM2_MAX_DROPS` records (its
  `PORT_DROPS`); a join keeps the field's mines.
- **`TM2_PORT_HAZARDS`:** independent; both latches in `roundStart` before the `srand`.

## 7. Rounding (the ram factors, from request 39)

`d` is never negative (sqrt-based, scaled by |angle| / 1024). Round to nearest, half up: `(d * tenths + 5) / 10`.

| d | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Axel / Minion x1.1 | 1 | 2 | 3 | 4 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |
| Hammerhead x1.2 | 1 | 2 | 4 | 5 | 6 | 7 | 8 | 10 | 11 | 12 | 13 | 14 |

"Dealt" is every car-to-car record whose `other` is the car, including the (usually halved) damage a car takes when
it rams into him (39 Q3). The factor is applied before `carApplyDamage`, so the DT-human x2, the shield /2 and the
AI-vs-AI /4 act on the scaled value. Thresholds: >= 3 a bigger flash, >= 5 a ram may kill, >= 8 the spin roll.

How a ram works (CONFIRMED; `GAME_EVENTS.md` 2): `carCarCollision` computes both cars' damage `f x 12 / the other
car's speed cap`; the car aimed more squarely at the other takes half, the other double, each scaled by |angle| /
1024 below 1024. Each gets a contact record (`f01 = 0`) whose `other` is the other car; `carContactsCombine` keeps an
object record only without a ground hit, so the `f01`-clear branch of `carContactRespond` is reached only by these
records (walls, objects, trains have `f01` set).

## 8. Request 39 folded in

| Request 39 | Request 40 |
|---|---|
| define `TM2_PORT_RAM_BALANCE` | `TM2_PORT_BALANCE` (the old define stops the build with an `#error`) |
| `portRamOptions` bit 0 (Axel no mount + x1.1, Hammerhead x1.2) | bits 4 (Axel) and 3 (Hammerhead) |
| `portRamOptions` bit 1 (borrowing Hammerhead) | bit 11 |
| `portRamCrushDamage` (0 = 15) | dropped: the mount is 15, +5 on top (bit 3) |
| `portRamLive`, `portRamCrushLive` | `portBalanceLive` |
| `portRamLatch`, `portRamNoMount`, `portRamDealt`, `portRamCrush` | `portBalanceLatch`, `portBalanceNoMount`, `portBalanceRam`, (retail `carCrushDamageAmount`) |
| `docs/RAM_BALANCE.md`, `tools/check_ram.py` | this document, `tools/check_balance.py` |

## 9. Retail notes and future bits

- `RETAIL_BUGS.md` 20 (`aiFreeSteer`'s dead "Hammerhead always rams" store): left retail; a future bit could fix it
  (AI Hammerhead always rams, more free AI crushes, that `rand()` skipped for him).
- 21 (the tornado throw): bit 12, option c (3.7): the throw at the tornado's end; the dead tests stay. 22 (mines past the 23rd object): fixed under bit 2.
  23 (shield recast cost): bit 1. 24 (an AI shield would never clear) and 25 (`weaponDrops[-1]`): latent, left.
  26 (missiles under the ground through holes in the hit volumes): left retail, documentation only (bit 9 reserved,
  3.11).
- The ride drain (1 HP every 4th tick, unscaled): unchanged. A word for it would gate `carCrushContactUpdate`'s
  `hp -= 1`.

## 10. Verification

### 10.1 Static

- Retail builds byte-identical (10.3).
- Port variant proofs (`tools/verify_reloc.py` vs E0) and the placement check (limits forced to 2, the pools to
  retail's) pass with no define, `TM2_PORT_BALANCE` alone, the car stack + `TM2_PORT_BALANCE`, and every port define
  + `TM2_PORT_HUMANS=16` + `TM2_PORT_BALANCE` (10.2).
- `tools/check_balance.py OUT --expect-functions` on each build with the define (checks in its docstring: the port
  functions in `.port_text`, reaching no `rand()` (the one documented edge aside), called only by their gates, never
  pointed to; gates G0-G21 by flow graph (G12: `weaponProjectileUpdate` free of port calls; G15: the leap free of
  bit 14's calls, neither function linked); the helpers' source without `rand(`; no literal 25 / 23 / 24 sizing the
  tables; exactly the 20 functions with the define alone). `--negative-control` on the build without the define
  fails as it must.

### 10.2 Proof results

Car stack = `TM2_PORT_CAR_GROUPS TM2_PORT_CARS=16 TM2_PORT_CAR_SWAP TM2_PORT_TEXTURE_BANKS TM2_PORT_CAR_SPECIALS
TM2_PORT_DARK_TOOTH TM2_PORT_CAR_SOUND TM2_PORT_CAR_CLUTS TM2_PORT_CAR_DROPIN TM2_PORT_SPECIAL_AS TM2_PORT_DT_PLAYER`
(so with `TM2_PORT_DT_PLAYER` and `TM2_PORT_CAR_CLUTS`: request 41 items A and B); all = the car stack +
`TM2_PORT_FAR_LOD TM2_PORT_FAR_DISTANCE TM2_PORT_SPU48 TM2_PORT_HAZARDS TM2_PORT_HUMANS=16`. Round 3 (2026-10-16),
every row by `tools/check_all.py work/pv/r3_<tag> --e0 work/build-20261016-063045-826803/source_reloc_e0`: both
proofs (`verify_reloc.py`) and the placement check pass; `check_balance.py --expect-functions` passes on the three
builds with the define (exactly the 20 functions with the define alone; the placement link changes exactly those 20
plus the 5 `$gp` users of moved arrays); `--negative-control` fails as it must on the build without it, and the
round-3 checks fail on request 41's link (G15: `portBalanceLeapArmed` / `Hold` called and linked; G21: no
`portBalanceGunHit`; the taser's `carApplyDamage`); on the car-stack and all builds `check_car_groups.py
--expect-functions` passes (check 12 with `portBalanceCars` as an unload holder), on the all build also
`check_hazards.py --expect-functions`, `check_humans.py` on the four; `--models` (the disc models) passes. Port end of
the P4 link (port RAM end 80C00000).

| Defines | P4 sha | P2 (placement) sha | port end |
|---|---|---|---|
| none | `003783cb32c4` | `b95e6fed7439` (as before: no code without the define) | 80543E10 |
| `TM2_PORT_BALANCE` | `03678c4ab720` | `8738597854ad` | 80545590 |
| car stack + `TM2_PORT_BALANCE` | `b9c7bb6bcada` | `3274c69f62f2` | 805984E8 |
| all + `TM2_PORT_HUMANS=16` + `TM2_PORT_BALANCE` | `08aef4395df2` | `c7b3977e5a9b` | 805A293C |

(Request 41's: none `fc67c45181e6`, BALANCE `fefbb9e784a3`, car stack `cf5db5d27047`, all `88b86163cd49`.)

Addresses (P4 links):

| Symbol | none / BALANCE | car stack + BALANCE | all + HUMANS=16 + BALANCE |
|---|---|---|---|
| `portBalance` | 80543218 | 80584C88 | 8058B568 |
| `portBalanceLive` | 8054321C | 80584C8C | 8058B56C |
| `portBalanceDropSeq` | 80543220 (25) | 80584C90 (96) | 8058B570 (96) |
| `portBalanceDropSeqNext` | 80543284 | 80584E10 | 8058B6F0 |
| `portBalanceRideTop` | 80543288 (15) | 80584E14 (64) | 8058B6F4 (64) |
| `portBalanceCars` (8 bytes each; 16 in request 41) | 80543BD0 (15) | 8058768C (64) | 8058DF6C (64) |
| `portBalanceGunObjects` (round 3) | 80543C48 | 8058788C | 8058E16C |
| `weaponDrops` | retail storage | 80317D70 (96 x 28) | 8031DFFC |
| `weaponMineObjects` | retail storage | 803187F0 (80) | 8031EA7C |
| `csObjectPool` | 128 entries | 802F86AC (320 entries) | (320) |

The default variant's `.port_bss` holds `portBalanceCars` (15 x 8 bytes) and `portBalanceGunObjects`, defined last
(round 3: 0x70 bytes less than request 41's 15 x 16), so `.port_text` moved; its code is otherwise as before.

### 10.3 Builds

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261016-170052-021339`, round 3):
hybrid `9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB` / E0
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `003783cb…` (port end 80543E10; placement check OK; request 41 `fc67c451…`). Proof helper: `pv.py TAG DEFINES`
(P4 and P2 links, `verify_reloc.py` against E0, placement); then `tools/check_all.py`.

### 10.4 In game (for the port)

1. **Word 0:** same state hashes and `rand()` stream as without the define (sessions with a tornado, a leap,
   mines, remote bombs, a shield recast, spiral shots, a Shadow detonation on a ramp, rams and crushes).
2. **Bit 0:** fire missile hit 10, homing 7 (log `weaponApplyHit` amounts).
3. **Bit 1:** recast while shielded: denied, energy unchanged; recast after it ends: works.
4. **Bit 2:** a driver's 6th mine removes his oldest; mines never expire; 16 x 5 = 80 visible; remote bombs still
   placeable with 80 mines down; the refusal keeps the energy (retail-size build: the 24th mine); owner immune 40
   frames; round restart clears, a drop-in join does not; `csPoolCount` peak below 320.
5. **Bits 3-5:** Hammerhead ram x1.2; a crush he slides off gives 15, on top 20, the ride drain as retail; Axel and
   Minion never mount, x1.1; a human Dark Tooth unchanged.
6. **Bit 12:** a car caught at the tornado's end flies out along the heading away from her (one `carSpinOutById`
   per car in her box, humans then the AI); the catching (4 per catch, up to 28) unchanged; twins throw from their own
   boxes.
6b. **Bit 6:** mines and remote bombs refused during her tornado (detonation allowed); recharge 600; rams on her during
   the tornado: 0 to her; twins: one Twister's tornado does not refuse the other's mines.
7. **Bit 7:** leap hit 18; nose contact with the target = the hit, no cancel explosion; contact with another car or a
   wall = retail cancel; no ram damage to her while leaping.
8. **Bit 13:** she lands at ground height; the hop after the hit about half as long, smooth (video).
9. **Recharges:** a human's special comes back at 900 / 1200 / 1500 (Outlaw 2, Grimm, Thumper), 600 (Minion,
   Twister), 700 (Hammerhead, request 41); a car borrowing Outlaw 2's gets 900; the AI unaffected.
10. **Bit 9 (reserved):** the spiral is retail with every bit set. Log each spiral's `life` and `hit` / `obj->kind`
    at its death (3.11): how many die at frames 4-10 on rising ground (the first trough) and how many at frames 0-1
    (the muzzle's 128-unit box: a car, a breakable, a wall), to decide whether a launch fix is worth a bit.
11. **Bit 10:** Shadow's detonation on a ramp 28, off it 28 - d/4; a remote bomb on a ramp 25, off it 25 - d/4.
12. **Online:** the same word on two machines -> the same hashes; a different word refused by the ruleset hash.
13. **Bit 14 (reserved since round 3):** the leap with every bit set is request 40's (bits 7, 13): a point-blank
    leap hits at once, as before request 41.
14. **Bit 15:** during her tornado every weapon button, the guns and every energy code buzz once and cost nothing; a
    computer Twister fires nothing either; all works again when it ends.
15. **Bit 16:** a taser fired with no car in range, then a car driving into range within the 40 ticks: connected
    (the arc's end jumps to the car), the car lifted at once as on a connect at the shot, **no damage then**, one 10
    at the arc's end (20 only for a connect at the shot); lightning unchanged.
16. **Bit 17:** freeze missiles in a row on one car: 300, 150, 75 ... ticks; 1 s unfrozen resets to 300.
17. **Bit 18:** a ricochet fired at top speed and with turbo, straight and at 45 degrees: no self-hit at the launch;
    a bounce back into him still hits.
18. **Bit 19:** a human Grasshopper's energy jump: energy unchanged, works at 0, the cooldown as before.
19. **Bit 20:** hold the guns on one car: every bullet that connects flashes it; its HP drops 1 every second normal
    bullet (Mega Guns: 1, 2, 1, 2); over many bullets the same damage as with the bit clear (count bullets and HP on
    a tape with and without); two shooters on one car share its carry; computer cars on computer cars still 0.

## 11. Questions for the port

Built with the user's decisions (2026-10-15) as the defaults; any can change.

- **Q1. Fold 39:** built as one word with no alias; `TM2_PORT_RAM_BALANCE` retired with an `#error` (not an alias).
- **Q2. Hammerhead:** 15 at the mount + 5 once the first frame on top (20 in all); the ride drain retail; recharge 700
  (request 41; 40 built 900)
  for humans (computer cars never recharge). Default as decided.
- **Q3. Mines:** 96 records (80 mines, 16 bombs), 80 objects, coordinate-system pool 320 (measure the 16-car peak
  once); a removed oldest mine vanishes silently (or pop with an effect and no damage?); the owner can hit his own
  mine after 40 frames (retail); the stale-object fix is part of bit 2 (with the word zero retail's behaviour stays,
  as the word-zero rule requires; it could be made unconditional under the define instead, which would change the
  word-zero logic only in that bug's case); without `TM2_PORT_CARS` (or with the pools bit clear) bit 2 caps the
  mines at 23 in all.
- **Q4. Remote bomb during the tornado:** placement refused, detonation of an earlier bomb allowed (default).
- **Q5. Shadow and remote bombs (answered 2026-10-15):** Shadow `min(retail, 25) + 3` (28 flat on a surface,
  `28 - d/4` off it); remote bombs the same cap without the +3 (25 flat on a surface, `25 - d/4` off it), both
  under bit 10 (3.12). Still open: the strike's direct hit 25 -> 28 too (built: no)?
- **Q6. Spiral (answered 2026-10-15, clarified):** flight retail; the worry is spirals dying right when fired, so the
  ground-contact death is removed and bit 9 is **reserved, no effect** (3.11). The cause found: the wave's first
  trough passes about 15 units over a flat road 0.4 s out (it dies on any rise), plus the muzzle's 128-unit box; the
  offline estimate shows no small fix (a grace or a 24-64 higher launch) that keeps most of those spirals alive. Open:
  after the runtime log (10.4 item 10), build the mod's higher launch or the graze grace under bit 9, or leave it.
- **Q7. Twister's throw (7d): option c (answered 2026-10-15),** built (3.7). Runtime look: how far a thrown car
  travels while the rest of its catch's freeze (<= 12 frames, braking) runs; if too short, the throw could also end
  that freeze (not built: the user asked to leave retail's catching as it is).
- **Q8. Recharges at the reader** (as decided): they act from the next round even when weapons carry over.
- **Q9. Shield pool full:** refused with no cost under bit 1 too (default yes).
- **Q10. Grasshopper landing (bit 13):** z = ground + ride height at the hit and the wheels' upward speed / 4: a first
  cut, kept as is (the user, 2026-10-15); **awaits the port's runtime look** (tune, or replace with advancing the air
  counters).
- **Q11.** Bits 12 and 13 separate (built) or folded into 6 and 7?
- **Q12. Minion:** every Minion, the Juggernaut's included (built)?
- **Q13. "Dealt":** every record whose `other` is the car (built, as 39 Q3)?

## 12. Round 2 (PC port request 41, 2026-10-16)

The user's answers to this document (2026-10-15). Same word and rules as request 40: one bit per rule (the port shows
each bit as an option), zero = retail, all on in Vanilla Plus and online, deterministic, no new `rand()` beyond what a
rule says. The 40 rules stay on their bits (the user agrees with them all). Line numbers are this branch's.

### 12.1 The items

| # | Item | Status | Bit |
|---|---|---|---|
| 1 | Hammerhead's recharge 700 (35 s), as Axel's | **built** (`TM2_BAL_HH_RECHARGE`, `portBalancePeriod`) | 3 |
| 2 | Sweet Tooth's spiral retail | **already done, verified**: bit 9 reserved, no effect; `weaponProjectileUpdate` has no `portBalance*` call and no `portBalanceSpiral*` function is linked (`check_balance.py` G12, every proof build) | 9 (reserved) |
| 3 | Grasshopper: no hit before the hop | built (12.2); **removed in round 3** (13.1): bit 14 reserved | 14 (reserved) |
| 4 | Twister: no other weapon at all while her tornado runs | **built** (12.3) | 15 (own bit; bit 6's 7a stays as is) |
| 5 | The throw, option c, the cars caught at the end | **already built, confirmed and refined** (12.9): humans now by `state.f16` | 12 |
| 6 | Outlaw 2's taser: a second chance | **built** (12.4); round 3: a late connect lifts and hits once (13.2) | 16 |
| 7 | Freeze: diminishing returns | **built** (12.5); round 3: ice freezes only, confirmed (13.4) | 17 |
| 8 | Machine guns: damage only from bullets that connect | **answer: they already match** (12.7): no bit; round 3 removes the coin flip (13.3) | 20 (round 3) |
| 9 | Ricochet: no self-hit at launch | **built** (12.6); the other projectiles do not have the problem | 18 |
| 10 | Grasshopper's energy jump free | **built** (12.8) | 19 (own bit) |
| 11 | Car size for collisions and hits (question) | **answered** (12.10) | - |
| A | A player's Dark Tooth after a won round | **built** under `TM2_PORT_DT_PLAYER`, no bit (`DARK_TOOTH_PLAYER.md` 9) | - |
| B | The image scan's one-texel test | **built** under `portCarOptions` bit 9 (`TM2_PORT_CAR_CLUTS`; `CAR_GROUPS.md` 13.12, `SIXTEEN_CARS.md` 4.4) | (car bit 9) |

### 12.2 Grasshopper: no hit before the hop (bit 14, item 3)

**Removed in round 3 (13.1): bit 14 is reserved, no effect; the code below is gone.** The retail analysis stays
valid (the leap's code is request 40's again, so its line numbers are those of `weapon_leap_homing_update.c` at
request 40, where the hit test is line 111 (120 without the define) and the cancel test line 129).

**Retail (CONFIRMED).** `weaponFireSpecial` 0x60 (`weapon_basic.c:235-238`) calls `carSpinOut(car, 7, keep 1)` (the
hop: every wheel's upward speed `fall` = `RATE(448) x 2/3` = 298, about 28 units a frame up at 3 ticks, gravity
growing with each wheel's counter, `carWheelVerticalMotion` `car_wheels.c:876`) and `weaponHomingLeapInit`
(`weapon_leap_homing_init.cpp:64`: the nearest car with `DISTANCE < 400` (3200 units) within about 60 degrees ahead;
`active = 1`, `age = 0`, the homing speed `speed_cap / 32 x 2`, about twice her top speed per frame). Then
`weaponHomingLeapUpdate` (`weapon_leap_homing_update.c`), every update while active:

- the homing moves **her position** (`H.projectile` = her `motion.pos`) toward the target's `motion.pos` (yaw and
  pitch at it), but only from `age` 8 on (`H.age++ >= 8`, line 94), its speed blended from 0 toward the homing speed
  (about 7 % of the gap a frame at 20 fps);
- the hit: `DISTANCE(her, target) < 15` (120 units on **every** axis, z too) -> 15 damage (18 with bit 7), line 115;
- the end: `age >= 101`, her contact byte `contact.in_contact` (`*H.cancel`: any contact, a car, a wall, an object)
  with the cancel explosion, or 6 HP lost since the launch, line 135.

So the hit test runs from the first update (the frame she fires): **point-blank (within 120 on every axis) it hits at
once, before the hop shows** (the request's finding).

**What the leap's state gives to test.** The state (`homingLeapState` 80184DD4, 0x3C bytes; with
`TM2_PORT_CAR_SPECIALS` per key) holds `active`, `age` (updates since the launch), her position pointer, the
target's position pointer, the homing velocity (its forward speed, not a vertical one), her HP pointer and HP at the
launch, the contact byte pointer. It has **no vertical speed and no "on the ground" flag**. Her car record has the hop
in its wheels (each wheel's `touch` (+0x01) set by the spin-out, `fall` (+0x14) the upward speed, `f12` the gravity
counter), but the climb is `fall x ticks / 32` plus a bounce term `f2C` that grows with `f12` (`car_wheels.c:880-890`),
so "descending" is not one field. Her **z** (`motion.pos[2]`) follows the wheels (`carRollover` sets it to the
wheels' mean, `car_rollover.c:216`; LIKELY: the homing's own z step is overwritten by the next car tick), so the rule
follows her z per car slot (`portBalanceCars[slot]`):

- at the leap's first update (`age` 0) her z is noted (`leap_z0`);
- **"left the ground"**: her z is `TM2_BAL_LEAP_AIR` = 16 above it;
- **"descending"**: after that, the first update at which her z is not above the previous update's (past the top of
  the hop): she is **armed** and stays armed for this leap;
- **no hop**: `carSpinOut` does nothing while a spin-out already runs (`car_spin.c:122`), so a Grasshopper hit just
  before her shot does not hop; never up by `age` 8 (`TM2_BAL_LEAP_NO_HOP_AGE`, where the homing starts) = armed, so
  such a leap hits as retail's does instead of never.

`portBalanceLeapArmed()` (`port_variant_arrays.c:8139`) runs at every update before the move
(`weapon_leap_homing_update.c:80`); the touch (bit 7) and the hit need `armed` (lines 113-114); before she is armed a
contact **with the target** does not cancel the leap (`portBalanceLeapHold`, line 135: point-blank she touches it at
the launch, then hops and comes down on it); a contact with anything else, the age 101 timeout and the 6 HP loss end
it as retail. Twins: per slot (each Grasshopper's leap runs in its own selection).

**What happens at a few distances (LIKELY: from the code, not run; the hop's height from the wheel model, about 25-28
units a frame up at first, the top after roughly 0.5-0.8 s, the homing from 0.4 s on at up to about 2x her top
speed).** Distances are world units between the two origins.

| distance | retail | 40 built (bits 7, 13) | 40 + bit 14 |
|---|---|---|---|
| point-blank (< 120 on every axis: nose to nose, side by side) | hit in the frame she fires: 15, before any hop (the hop then plays out over the hit car) | the same, 18; bit 13 sets her down and quarters the hop: she hardly leaves the ground | no hit at once: she hops (her contact with the target ignored), the homing (from 0.4 s) keeps her over it; the hit when she comes down within 120 (or touches it, bit 7), about 0.6-1 s after the shot |
| ~200 | no hit in the first frames (> 120); the hop lifts her more than 120 above the target before the homing starts, so the z test fails while she is up; if her body touches the target first (a bump while she still moves forward, or landing on it off-centre) the contact **cancels** the leap: explosion, no hit, which reads as "she jumps over the enemy" (the user's observation); else the hit as she comes down | a touch with the target is the hit (8c): 18; so the "jumps over, no hit" case becomes a hit | as 40: by the time the homing brings her over the target she is near the top of the hop, so the hit comes on the way down as before; the rule changes little here |
| ~500 | the homing from 0.4 s, at the target after about 1 s; she is usually already past the top of the hop: the hit when within 120 | the same, 18 (a touch counts) | unchanged (armed past the top before she arrives) |

**Can a point-blank hop miss a target that drives away?** Rarely. The homing follows the target's position pointer
(no line of sight, no range limit after the launch) at up to about twice her top speed (`speed_cap / 32 x 2`; cars
move at most about 80 units a frame, 103 with turbo), so a car cannot outrun her on open ground. With bit 14 she
misses when: the leap times out (age 101, about 5 s), she touches something else (a wall, another car: the retail
cancel, now possible while she is up and before she is armed), she loses 6 HP, or the target ends up more than 120
below or above her when she comes down (a ramp, a lower level). The "short window to dodge" is therefore mostly a
delay of about 0.6-1 s, not an escape; if the user wants a real dodge, a later bit could hold the homing while she is
up (not built).

**`rand()`:** none added; the hit (18: the spin roll) moves later or does not happen.

### 12.3 Twister: no weapon at all while her tornado runs (bit 15, item 4)

"Her tornado runs" = `balTornadoRuns` (3.7: her key's `weaponTornadoState.timer > 0` and its object is her car
object; per key with twins). Bit 15 is its own bit (the port prefers it): bit 6 keeps 7a-7c as built; with both set the
lock refuses first, so 7a's refusals never trigger.

| What | Where (gate) | Refused how |
|---|---|---|
| every pickup weapon (kinds 1-7), her special (kind 0), a remote bomb's placement **and detonation** (kind 3, either branch) | `weaponFireSelected` (`weapon_select.c:124`), after the frozen / no-request test, **before** the cooldown reload and the dispatch | `portBalanceLocked`: the denied sound (`TM2_HUMAN_DENIED`, humans), `fire` and `fire_ready` cleared (one sound per press: `fire_ready` re-arms only when the button is released), return: no ammo, no cooldown |
| every energy attack (100-106: missiles, mines, shield, jump, invisibility, the freeze) | `weaponFireEnergyAttack` (`weapon_special.c:97`), after the frozen / no-request test, **before** the energy test | the denied sound (humans), the request cleared (a pad code is one request), return: no energy, no cooldown. **Since request 46 item 3 (15) not the shield (103)**: `portBalanceEnergyLocked` lets it through |
| the machine guns | `weaponTick` (`weapon_tick.c:93`), before the gun block | `portBalanceGunsRefused`: `fire_guns` taken (the gun loop stops in the block's `else`, `UpdateGuns` is not called); the denied sound only on a new press (`portBalanceCars[slot].guns_refused` remembers last frame's refusal; the button is held, so a per-frame sound would buzz) |

Humans and computer cars alike (the AI goes through the same three functions with `MODE` 0; it gets no sound, and
"no cost" for it means not firing: its ammo and energy are refilled every tick anyway). Her special itself is refused
too: retail's `weaponFireSpecial` 0xA0 does nothing while her tornado runs (`if (weaponTornadoState.timer <= 0)`) but
`weaponFireSelected`'s tail still takes the charge (`RETAIL_BUGS.md` 29), so a second press during the tornado no
longer wastes one. Detonation: refused as the request asks ("no weapon input at all"); the bomb stays armed and can
be set off when the tornado ends (at most 83 updates, about 4.2 s). **`rand()`:** a refused weapon's own calls do not
run (bullets' coin flips, a launch's), nothing added.

### 12.4 Outlaw 2's taser: a second chance (bit 16, item 6)

**Retail (CONFIRMED).**
- **Firing:** `weaponFireSelected` kind 0 -> `weaponFireSpecial` 0x30 (`weapon_basic.c:198`) ->
  `weaponTaserTarget(car, human, id)` (801075C8, `weapon_slot_target.c:51`). The state is an **arc record**
  (`weaponArcs`, 20 bytes: shooter id, shooter slot (the damage's source), `count`, kind 0 taser / 1 lightning,
  target object id, a pointer to the shooter's `motion.pos`, a pointer to the victim's), 5 in retail (one per car with
  `TM2_PORT_CARS`), from `weaponArcSlotFind`. The AI fires it as a melee special (`carHasMeleeSpecial`: a target
  within `carPairDistance < 400` and about 20 degrees, `weapon_select.c:85-97`).
- **Choosing:** **once, at the shot**: the nearest car with `DISTANCE(shooter, car) < 80` (640 units on every axis),
  humans first (any HP), then the computer cars alive, not the shooter, strict `<` (ties keep the human); **no angle
  and no line of sight**.
- **Keeping:** locked for the whole arc: `weaponArcsDraw` never re-tests range, sight or HP; the arc ends when its
  count does.
- **The shot's time:** `count = 40` (`weapon_slot_target.c:114`), counted down once per `weaponArcsDraw` call
  (`weapon_arcs_draw.c:325`), which `streaksDraw` runs per view: 40 frames (2 s) in one view, half that with split
  screen or the mirror (`RETAIL_BUGS.md` 15).
- **Damage:** 10 at `count == 39` (the first tick) and 10 at `count == 0` (the last) through `carApplyDamage(target,
  10, 0, 2, source)` (`weapon_arcs_draw.c:326-329`): 20 per connected shot; the connect also runs `carSpinOut(victim,
  kind 6)`.
- **A miss** (no car in range at the shot): the arc is **not** ended: `count = 40`, but id, slot and target are set
  to -1 and the end point to none (`weapon_slot_target.c:115-119`); it runs its 40 ticks with a free end swinging
  around the shooter (`weapon_arcs_draw.c:279-283`), no damage. The function returns 1 (the shot's sound plays).
  **The charge is spent anyway**: `weaponFireSelected`'s tail takes `ammo[0]` for every fire (`weapon_select.c:229`),
  hit or miss (also when no arc record was free).

**Built (the smallest change):** in `weaponArcsDraw`, for an arc with no target (`weapon_arcs_draw.c:276`, before the
free end is computed and the count goes down) `portBalanceTaserRetry(i)` (`port_variant_arrays.c:8201`): for a taser
arc (kind 0; lightning is untouched) with more than one tick left, find the shooter (the car whose `motion.pos` the
arc starts at, since the miss cleared its id and slot) and run retail's target test again from it; on a find connect
the arc as `weaponTaserTarget` connects it (id, slot, target id, the victim's position, `carSpinOut(victim, 6)`), and
apply its **first 10 now** unless retail's own `count == 39` hit is still to come (a connect in the first tick), so a
late connect deals retail's 2 x 10 (the second at `count == 0`). "Not already locked" = the target is -1; a connected
arc is never re-tested. The search runs every tick while unconnected (each call of `weaponArcsDraw`, so per view as
retail's ticks). **`rand()`:** a late connect runs the calls a connect at the shot runs (`carSpinOut` kind 6: one
`rand() & 7`; the 10 through `carApplyDamage`: the spin roll); the search none. **Round 3 changed the late connect
(13.2): the lift without damage, one hit of 10 (the arc's last tick).**

### 12.5 Freeze: diminishing returns (bit 17, item 7)

**Where the freeze length is set (CONFIRMED; units are ticks, `freeze_timer` counts down by `timerGetFrameTicks()`
each frame: 3 at 20 fps).**

| source | function | file:line | length |
|---|---|---|---|
| a freeze projectile (type 14) on a human: the freeze missile pickup (`weaponLaunchProjectile`), the energy freeze (attack 106, `weapon_special.c`), Minion's volley (`weaponFireSpecial` 0xE0, `weapon_basic.c:422`) | `projectileHitTest` 800D6FD8 | `weapon_slam_swing.c:72-77` (not frozen, not shielded: frozen, 300) / `:86-89` (already frozen **or shielded**: the timer back to 300) | 300 (5 s) |
| the same on a computer car | `projectileHitTest` | `weapon_slam_swing.c:117-122` / `:131-135` | 300 |
| Twister's catch | `weaponTornadoUpdate` | `weapon_tornado.c:160` / `:200` (only when not frozen) | 60 |
| Mr. Slam's grab | `weaponSlamFindTarget` 800F5498 | `screen_card_draw.c:244` / `:287` | 60 (+ speed 0, held) |
| Hammerhead's crush (the victim) | `carCrashStart` 800E6D14 | `car_crash.c:111` | 30 |
| a wreck | `carDestroyed` 8011BE10 | `car_destroyed.c:414` | 540 |
| damage while frozen shortens it | `carApplyDamage` | `car_damage.c:322` | -6 x damage |

The thaw: `humanCarTick` (`car_tick.c:79-93`) and `aiCarDrive` (`ai_driver.c:58-72`): `freeze_timer -= ticks`; at <= 0
`frozen = 0` (and the ice look off for a missile freeze). There is no common freeze function and no Dark Tooth freeze.

**Built.** The rule applies to the **ice freezes** (every type-14 projectile: the freeze missile, the energy freeze,
Minion's volley, humans and AI): the four `freeze_timer = 300` writes of `projectileHitTest` go through
`portBalanceFreeze(car, human, 300)` (`port_variant_arrays.c:8269`). The holds of other specials keep retail's
lengths: the tornado's catch (bit 12 / option c keeps her catching as retail; halving it would re-catch the car more
often, and each catch costs 4 HP), Mr. Slam's grab and Hammerhead's crush (part of those moves' timing), and the
wreck. They still count as "frozen" for the chain (below). Per car, in `portBalanceCars[slot]`:

- `frozen_at`: the last frame (`timerFrameCount`, `GsGetWorkBase_0`) the car was frozen, written every frozen frame
  by `portBalanceFrozenTick` (at the top of the thaw block, `car_tick.c:81`, `ai_driver.c:60`) and by each ice
  freeze;
- `freeze_len`: the length of the car's last ice freeze in the current chain (0 = none); a frozen streak that starts
  more than 20 frames after the last frozen frame clears it (a new chain).

An ice freeze of a car that is frozen now or was frozen within the last `TM2_BAL_FREEZE_WINDOW` = 20 frames (1 s),
with an ice freeze already in the chain, lasts `freeze_len / 2`, at least `TM2_BAL_FREEZE_FLOOR` = 15 ticks (5
frames); else the full 300, which starts a chain: 300, 150, 75, 37, 18, 15, 15 ... ticks (5 s, 2.5, 1.25, 0.6, 0.3,
0.25). A re-freeze while frozen sets the timer to the shorter length (retail: back to 300). A shielded human hit by a
freeze projectile is not frozen (retail writes the timer only): the chain is untouched. **`rand()`:** none (lengths
only).

### 12.6 Ricochet: no self-hit at the launch (bit 18, item 9)

**Retail (CONFIRMED).** The launch (`weaponFireSelected` case 6, `weapon_select.c:150-172`): the shot starts at the
car's origin + 40 in z (no forward offset), rolls 100 units a frame along the car's heading (`offset[1] = 100`), life
400, owner = the car's object id. `weaponRicochetUpdate` (8010CC54) moves it first, then tests every car with its own
box: `DISTANCE(shot, car) < 20` (160 units on every axis), the owner allowed once `life < 398` (a human's shot: from
its 3rd update) or `< 396` (an AI's: the 5th) (`weapon_ricochet.c:155`, `:171`). After k updates the gap along the
heading is `100 (k + 1) - k v` for a car moving v units a frame (a top speed of 92-141 "mph" is about 52-80 a frame;
turbo up to about 103): at a human's first owner test (k = 2) the gap is under 160 for v > 70 on an axis-aligned
heading and v > 37 at 45 degrees, so a fast car **blows itself up on the 3rd update** (0.15 s), as the user saw
(`RETAIL_BUGS.md` 28). A car at 100 a frame or more never falls behind the shot, so no fixed time works.

**Built: "has left the owner's box once"** (the cleanest of the three: a time is beaten by speed and heading, a
distance threshold is the box itself). The shot record's byte +0x01 (unused by retail, now `left_owner`) is cleared at
the launch (`weapon_select.c`, beside `bounces = 0`); in both car loops the owner clause goes through `OWNER_HIT` =
`portBalanceRicochetOwner(shot, retail clause, DISTANCE(shot, owner))` (`port_variant_arrays.c:8309`): with the bit,
a test with the owner outside the box (`DISTANCE >= 20`) sets the byte, and the owner can be hit only once it is set.
So the shot cannot hit him until it has been clear of him once; after that, as retail (a bounce off a wall back into
him, or a car that overtook its own shot and slows down, can still be hit). **`rand()`:** a self-hit that no longer
happens skips its `carSpinOut` and explosion.

**Other projectiles (CONFIRMED): not the same problem.** The missiles, the homing and power missiles, the napalm and
Mega Napalm, the freeze missile and the spiral all run through `weaponProjectileUpdate`, whose car test
`projectileHitTest` always skips the owner (`obj->id != owner`, `weapon_slam_swing.c:70` / `:116`; `projectile_hit.c:231`
clears a hit on the owner; the level test ignores the owner's own volume): a fast car can never hit its own
projectile there, at any time. Mines and remote bombs (`weaponDropCheckHit`, `car_damage_human.c`) keep the owner
immune for about 40 frames, but they do not move: a car driving away leaves their 104-unit box in a frame or two.
Only the ricochet has its own test with a short timed owner immunity on a shot as fast as a car. Nothing else changed.

### 12.7 Machine guns: per bullet, one test (item 8: no bit)

(Round 3 adds bit 20, which removes the coin flip: 13.3.)

**Retail (CONFIRMED): the damage comes from the visible bullet's own test.** `UpdateGuns` (80104F70) spawns a bullet
record (`bulletSpawn`, `weapon_bullets.c:191`: 40 records, 200 units a frame, life 12 (human) / 15 (AI)) per shot,
alternating the muzzles. `bulletsMove` moves each bullet and tests four sub-steps of its path with
`projectileHitTest(point, owner, 10 / 5, -1)` (`weapon_bullets.c:263-265`: a box of 88 units for a human's bullet, 48
for an AI's; the owner skipped) and the level at one of them. On a hit the bullet ends (`life = 0`) and, **with a
coin flip**, `if (rand() & 1) weaponApplyHit(..., power 1 (3 with Mega Guns), ...)` (`weapon_bullets.c:287`). The
tracer `streaksDraw` draws is the same record (its previous and current point). There is no ray, cone or lock; the AI
fires the same bullets (`aiWeaponFireDecide` only sets the request).

So **damage never comes from a tracer that missed**, and every damaging hit is a bullet that reached the car. The only
difference: **half of the bullets that connect do no damage** (the coin flip, a retail `rand()` call). Small visual
differences: the bullet's last segment is not drawn on the hit frame, and the 88-unit box can count a hit for a line
drawn just beside the car. **No bit built** (the request: "if they already match, say so"). If the user wants every
connecting bullet to do damage, a bit could skip the flip's test while still calling `rand()` (the stream unchanged);
that doubles the guns' damage (open question 12.12).

### 12.8 Grasshopper's free energy jump (bit 19, item 10)

**Retail (CONFIRMED).** The energy jump is attack 104 (pad code `0x11300000`, `car_input.c`): `weaponFireEnergyAttack`
(`weapon_special.c:169-186`) refuses below 120 energy (denied), else `carSpinOut(car, 7, keep 1)` (the hop), energy
-120, sound 0x10; the shared tail sets the cooldown (`fire_cooldown = 20 >> 1` = 10 for a human,
`weapon_special.c:255`). The function also returns silently at 0 energy before the switch.

**Built:** `portBalanceJumpFree(car, human)` (`port_variant_arrays.c:8322`) = bit 19 and the car is a Grasshopper (car
type 0x60: her passive, also when she borrows another special with `TM2_PORT_SPECIAL_AS`): case 104's test needs 0
instead of 120 and nothing is subtracted; the 0-energy return lets her jump through. The cooldown (the tail) is
kept. **Computer cars:** the AI never casts attack 104 (its energy picks are 100, 101 or none:
`aiPickWeaponFromTable9` / `10`); a computer Grasshopper's ram hop (`ai_steps.c`) is a direct `carSpinOut` that costs
nothing already. So the rule acts for humans; it is written for both. **`rand()`:** none.

### 12.9 The throw (bit 12, item 5): confirmed

As built in 3.7 (option c): no throw at each stun's end (retail's dead `freeze_timer == 1` tests stay dead), the
catching retail's for the whole tornado (up to 7 catches, 28), and **when the tornado ends every car caught at the end
is thrown out** (`portBalanceTornadoThrow`, `port_variant_arrays.c:8080`, the end block's last act,
`weapon_tornado.c:273`). Request 41 asked for "the cars caught at the end (humans by `state.f16`, the AI as you
proposed)": the humans' test now **is** `state.f16` (retail's caught flag, set this frame by her loop: 3.7), the AI's
the catch box on the positions after the orbit (as proposed in 40; the AI records have no such flag). **The throw's
`rand()` is retail's:** the throw is retail's own `carSpinOutById(key, heading, 0)`, whose `carSpinOut` draws the spin
roll (`rand() & 7`) and whose shake draws its picks, the calls retail's intended throw would draw; the port function
calls none (`check_balance.py` takes that one edge out and checks it is called once).

### 12.10 Car size for collisions and weapon hits (item 11: question, no change)

**Per-car constants, not model bounds (CONFIRMED).** `carStatsLoad` (800FE15C, `car_stats.c`) clears the stats block
and calls one init function per car type, which writes constants (`CAR_STATS.md` table: +66 ride height, +68 launch
height, +70 roof height, +72 track width, +74 wheelbase, +78 / +80 front / rear axle, +216..+220 the gun muzzle).
Nothing reads a model's vertex bounds and no size comes from a disc file. There is no radius field.

| system | its size comes from |
|---|---|
| the car's own collision points (6: four corners at (+-track/2, front / rear axle, 24 - ride height), front centre, front roof point at roof height) | stats (`carBuildCollisionBox` 800FDFC4, `car_restart.c:240-274`, at start and restart) |
| what those points hit on **another car** | **the other car's model**: `hdpCollideObject` / `hdpPassRun` test the points against every live object's hit-volume tree (object +0x11C = the model it draws: bounding-sphere nodes, box leaves) |
| the ram response, the crush, the push-out | stats and speeds (`carCarCollision`, `carCrashStart`: axles, wheelbase, ride and roof height; `carContactRespond`) |
| weapon hits on a car | a **cube of a constant radius per weapon** around the car's origin (`projectileHitTest`: (radius + 1) x 8; 15 / 8 projectiles, 10 / 5 bullets, 20 the strike, 12 drops), +25 for type 0xF0 only; the splash 75 / 100 (`areaDamage`) |
| wall and ground contact, rollover | stats (the four wheel probes at (+-track/2, axle, -ride height): `carUpdateContacts`, `carWheelsUpdate`, `car_rollover.c`) |
| camera | constants by car type (`viewCameraOffsetsInit` 800FF764: e.g. chase (0, -280, 112), a few per-type split-screen cases); the watched-AI view adds stats (axle, ride + roof height) |
| shadow | its **shape** is the car model's node 3 (the shadow blob, `TEAM_COLORS.md`); its **height and tilt** come from the wheels (`carBodyPoseUpdate` 80114F24: the mean wheel height, +3 for some types); file:line and what would make the height follow a mod model: 13.5 |

**A mod body (e.g. Twisted Metal 1's Mr. Grimm without the sidecar):** what follows the new model automatically is
only what is read from the model: the hit volumes **other cars** collide with (if the new model has its own sphere /
box nodes; with none, other cars would drive through it), its draw and visibility sphere, and the shadow blob's shape.
Everything else stays the original car's because it is keyed by car type: its own collision points, the wheel and
wall probes, ride and roof height, weapon hit cubes, muzzles, camera, shadow height. **To keep the original hit box:**
keep the original model's hit-volume nodes in the new model's tree (the visible mesh can change freely), or keep the
object's +0x11C on the original model and draw the new mesh separately. A skin that changes only textures changes
nothing of this.

### 12.11 `rand()` (round 2)

No new call site. With bits 14-19 the stream differs only where a rule runs or skips one of retail's own calls
(section 5's last item). `check_balance.py`: the request-41 port functions reach no `rand()` except
`portBalanceTaserRetry` through its two retail calls (`RAND_VIA`, each called once, the amount the constant 10); its
source block has no `rand(`.

### 12.12 Questions for the port (round 2)

- **Q14 (bit 14).** The hit only after the top of the hop; the homing still follows the target while she is up, so a
  point-blank target rarely escapes (12.2). Hold the homing while she is up for a real dodge window (a new bit)?
  **Answered (round 3): neither; bit 14's rule is removed, the leap is request 40's, no homing pause (13.1).**
- **Q15 (bit 15).** Detonation refused during the tornado (as asked); the gun's denied sound once per press (built).
  Keep?
- **Q16 (bit 16).** A late connect deals the full 2 x 10 (the first 10 at the connect). Or only retail's last 10?
  **Answered (round 3): one hit of 10, plus the first hit's lift without its damage (13.2).**
- **Q17 (bit 17).** Ice freezes only (projectile type 14); the tornado's catch, the Slam grab and the crush keep their
  lengths (they count as frozen for the window). Floor 15 ticks (5 frames). Include the others?
  **Answered (round 3): ice freezes only, as built (13.4).**
- **Q18 (item 8).** No bit: the guns' damage is per connecting bullet with a 50 % coin flip. Remove the flip (double
  the gun damage) under a bit? **Answered (round 3): remove it under bit 20, every hit at half damage, the average
  kept (13.3).**
- **Q19 (bit 18).** "Left the box once" (built). A car that overtakes its own shot can be hit by it later (retail-like):
  fine?

## 13. Round 3 (2026-10-16: the user's decisions on round 2)

Same rules as before: one rule per bit, zero = retail, all on in Vanilla Plus and online, deterministic, no new
`rand()` call site. `TM2_BAL_ALL` = `0x1FFFFF` (bits 0-20; 9 and 14 reserved, no effect). Line numbers are this
branch's.

| # | Decision | Status | Bit |
|---|---|---|---|
| 1 | Grasshopper: remove bit 14's rule (no hit before the hop); vanilla leap; keep 40's 8a-8c and bit 13; no homing pause | **done** (13.1) | 14 reserved |
| 2 | Outlaw 2's taser, late connect: one hit of 10 (the second hit's), plus the first hit's lift without its damage | **done** (13.2) | 16 |
| 3 | Machine guns: no coin flip; every connecting bullet does damage, at half, the average kept | **built** (13.3) | 20 (new) |
| 4 | Freeze diminishing returns: ice freezes only (not the tornado's catch, the Slam grab, the crush) | **kept as built** (13.4) | 17 |
| 5 | Shadow of a mod body: confirm; what would make its height follow the model | **answered** (13.5), no code | - |

### 13.1 Bit 14 reserved (Grasshopper's leap is request 40's again)

`weapon_leap_homing_update.c` is back to its request-40 text (the same file as before request 41: bit 7's touch and
18, bit 13's landing at the hit, retail's cancel test at line 129); `portBalanceLeapArmed` and `portBalanceLeapHold`
are deleted, bit 14's fields (`leap_z0`, `leap_z`, `leap_state`) are gone from `struct TM2PortBalanceCar` (16 -> 8
bytes), and `TM2_BAL_LEAP_AIR` / `TM2_BAL_LEAP_NO_HOP_AGE` are gone. `TM2_BAL_LEAP_HOP` (0x4000) stays defined as
**reserved, no effect** (as bit 9): a port that still sets it changes nothing. `balLeapTouchesTarget` stays (bit 7's
touch). No homing pause was built. `check_balance.py` G15 now proves the leap body calls neither function, neither is
linked and the source does not name them. 12.2's distance table's "40 built" column is the behaviour again.

### 13.2 Bit 16: a late connect lifts and hits once

**What retail's two hits do (CONFIRMED).**

- **The connect** (at the shot, `weaponTaserTarget` 801075C8): the arc record is filled and the victim gets
  `carSpinOut(victim, human, 6, 0)` (`weapon_slot_target.c:124` / `:127`). Kind 6 is the spin-out's full strength:
  every wheel's upward speed `RATE(288) x (100 - mass) / 100`, at least `RATE(96)` (`car_spin.c:129`; kind 4, a
  damage spin, is a third of that, `:134`), one of 8 bounce patterns by `rand() & 7` (`:194`), the spin running
  (`s->spin = 1`, `:283`; while it runs further spin-outs do nothing, `:122`). This is **the lift**: the car jumps up.
  It belongs to the connect, not to a damage hit. No sound of its own: the shot's sound plays at the fire, hit or
  miss (`weaponTaserTarget` returns 1).
- **The first hit** (`weaponArcsDraw`, `count == 39`, `weapon_arcs_draw.c:326`) and **the second** (`count == 0`,
  `:328`) are the same call, `carApplyDamage(target, 10, 0, 2, source)`: the red flash (`lightFlickerStart`), the
  spin roll (`amount >= 8`: `(rand() & 7) < 3` -> `carSpinOut` kind 4, which does nothing while the connect's spin
  still runs), the impact debris, the HP (x2 for a human Dark Tooth shooter, /2 for the shield flag, /4 computer on
  computer, nothing under the energy shield), the freeze shortening (`car_damage.c:135-325`). Neither hit lifts the
  car; neither plays a sound.

**Built.** `portBalanceTaserRetry` (`port_variant_arrays.c:8170`) searches from the arc's second tick (`count` 39
down to 1, `:8178-8180`; the shot's own tick, `count` 40, is retail's miss: a connect there would let retail's
`count == 39` hit run too) and on a find connects the arc as `weaponTaserTarget` does, with the lift,
`carSpinOut(victim, human, 6, 0)` (`:8229`), and **no damage** (request 41's `carApplyDamage(target, 10, ...)` at
the connect is gone). The arc's one hit is then its own `count == 0` hit of 10 (`weapon_arcs_draw.c:328`); its
`count == 39` hit is already past. A connect on the last tick (`count` 1) lifts and hits in the same tick. A connect
at the shot is unchanged: lift, 10, 10.

**`rand()`:** a late connect draws the connect's `carSpinOut` (`rand() & 7`) at the connect and the last hit's spin
roll at the end; request 41's first-hit spin roll at the connect is gone. `check_balance.py`:
`portBalanceTaserRetry` calls `carSpinOut` once (its one `RAND_VIA` edge) and `carApplyDamage` never; G17 also
checks `weaponArcsDraw` keeps retail's two `carApplyDamage` calls.

### 13.3 Bit 20: the machine guns without the coin flip

**Retail (CONFIRMED).** `UpdateGuns` (80104F70) spawns bullets with power 1, or 3 with Mega Guns
(`update_guns.c:64-67`; Minion's rear gun always 3, `:79`). `bulletsMove` (800D7E30): a bullet that connects with
anything (`s->hit > 0`: a car, a target, the level) ends, plays its spark, `if (rand() & 1)` a smoke puff
(`weapon_bullets.c:294`), then **`if (rand() & 1)`** `weaponApplyHit(hit, ..., power, ...)` (now line 300 via
`GUN_FLIP`). For a car (`hit` 1..99, not 8) that is `carApplyDamage(car, power, dir, 1, shooter)`
(`weapon_missile_trails.c:349`), which then: x2 for a human Dark Tooth shooter (`car_damage.c:150-151`), nothing
under the energy shield (`:191-202`), the flash and a return for an amount of 0 or less (`:211-213`), no spin roll
(the amounts stay below 8), /2 with the shield flag (`:227-228`), /4 computer on computer (`:229-230`). Other hits:
a target's counter (`targetDamage`, humans only), the Moscow dish, an arena actor (`targetActorHit`, no amount).

**The numbers (HP per connecting bullet; retail = half the bullets do the amount, the others nothing).**

| shooter -> target | amount at the target | retail: per bullet | bit 20 (carry): per bullet | average | "min 1 a hit" |
|---|---|---|---|---|---|
| normal gun (power 1) | 1 | 0 or 1 at random | 0, 1, 0, 1 ... | 0.5 = 0.5 | 1 (x2) |
| Mega Guns / Minion's rear gun (3) | 3 | 0 or 3 | 1, 2, 1, 2 ... | 1.5 = 1.5 | 2 (+33 %) or 1 (-33 %) |
| human Dark Tooth, normal (x2) | 2 | 0 or 2 | 0, 2 ... | 1 = 1 | 2 (x2) |
| human Dark Tooth, Mega (x2) | 6 | 0 or 6 | 2, 4 ... | 3 = 3 | 4 (+33 %) |
| normal on the shield flag (/2) | 1 / 2 = 0 | 0 | 0 | 0 = 0 | 0 |
| Mega on the shield flag (/2) | 3 / 2 = 1 | 0 or 1 | 0, 1 ... (1/2, 2/2) | 0.5 = 0.5 | 1 (x2) |
| computer on computer (/4) | 0 | 0 | 0 | 0 = 0 | 0 |

**The trade-off.** HP are whole numbers and a normal bullet is worth half a point on average, so "every bullet does
some HP damage" and "the same average" cannot both hold: a minimum of 1 a hit doubles the normal guns (and changes
Mega by a third). **Chosen: the half point carried** (exact average, deterministic, no `rand()`): every connecting
bullet adds `power` half points to its target's carry and the whole points are applied (normal 0, 1, 0, 1 ...; Mega
1, 2, 1, 2 ...). Every bullet that connects **counts**: it is applied every time, so the car flashes red on every
hit (`carApplyDamage` flashes before its amount test; retail's lost flips did not even flash), and its half point is
never lost. The reductions inside `carApplyDamage` act on the whole amounts as before, and every row above keeps
retail's average exactly. (If the user prefers "at least 1 HP every hit" over the average, that is a one-line
change in `portBalanceGunHit`: the doubled normal-gun damage above.)

**Built.** `bulletsMove` (`weapon_bullets.c:58-67`, `:300-304`): `GUN_FLIP(s, k)` = `(k = portBalanceGunHit(hit,
power)) >= 0 || (rand() & 1)`, `GUN_POWER(s, k)` = `k >= 0 ? k : power`; without the define the macros are retail's
`rand() & 1` and `power` (the retail build is byte-identical). `portBalanceGunHit` (`port_variant_arrays.c:8305`):
-1 with the bit clear; else the carry is the target car's `portBalanceCars[slot].gun_half` (a car hit found by
`carFindById`, as `weaponApplyHit` finds it), or the one shared `portBalanceGunObjects` for every other hit (targets,
the dish, the level: per object the total can differ from the exact half by one point at most; an arena actor reacts
to the first bullet instead of at random). Per target, not per shooter: two shooters on one car share its carry (the
one who completes a point gets the credit). Both carries are cleared by the latch (each round); with
`TM2_PORT_CAR_DROPIN` an unload clears the freed slot's entry (`port_variant_arrays.c:6757-6760`, also the freeze
chain and the gun refusal; `check_car_groups.py` lists `portBalanceCars` among the unload holders).

**`rand()`: the call that goes away** is `bulletsMove`'s second `rand() & 1`, the damage coin flip before
`weaponApplyHit` (line 300), for every bullet that connects with anything (a car, an object, the level): one call
fewer per connecting bullet. The smoke puff's `rand() & 1` (line 294) stays. `check_balance.py` G21: one
`portBalanceGunHit` call, which dominates exactly one of the two `rand()` calls and every `weaponApplyHit`, with a
path from it to `weaponApplyHit` that skips that `rand()`; the source goes through the macros.

### 13.4 Bit 17 stays on ice freezes only

As built (12.5): the diminishing returns apply to the four type-14 freezes of `projectileHitTest` (the freeze
missile, the energy freeze, Minion's volley); Twister's catch (60, `weapon_tornado.c`), Mr. Slam's grab (60,
`screen_card_draw.c`), Hammerhead's crush (30, `car_crash.c`) and the wreck keep retail's lengths and do not shorten
or start a chain. They still count as "frozen" for the chain's 20-frame window (`portBalanceFrozenTick` runs every
frozen frame, whatever froze the car): an ice freeze within 1 s of the end of a catch, grab or crush, with an ice
freeze earlier in the same chain, is halved. No code change.

### 13.5 The shadow of a mod body (no code)

**Confirmed (CONFIRMED in the code).**

- **The shape is the model's.** The shadow is the car file's object `type + 3` (ids 19, 35, ... 243: node 3, the
  shadow blob, `TEAM_COLORS.md`), registered while the car loads: `worldLoadObject` -> `carBodyObjectRegister(id,
  obj)` (`world_load_object.c:144-146`), which keeps the object (kind 5) as the type's body object
  (`misc_one_liners.c:437-455`; with `TM2_PORT_CAR_GROUPS` per key in `portCarGroups.body`). It is drawn with its own
  mesh, so a mod body's node 3 is its shadow: its outline, size and any offset modelled into it follow the new model
  already.
- **The pose is computed, not modelled.** `carBodyPoseUpdate` (80114F24, `car_body_pose.c:32-82`), each frame from
  `humanCarTick` / `aiCarDrive` (`car_tick.c:152`, `ai_driver.c:152`): the height is the mean of the four wheels'
  ground heights (`:62`), the tilt from the wheel positions (`:63-75`), x / y the car's position (`:76-77`), and
  **`+3` for the types 0x40 Warthog, 0x60 Grasshopper, 0xA0 Twister, 0xB0 Axel, 0xC0 Mr. Slam** (`:78-81`, by car
  type, not by model). The wheel positions come from the type's stats (track width, axles: 12.10).

So a mod body that replaces a car keeps that car's ground offset (+3 or 0) and its wheel footprint for the tilt; the
shadow's height itself is the ground under the wheels, which is right for any model whose blob is modelled at the
same height as the original's.

**To make the height follow the new model too:**

- **Easy (about one table and one gate, recommended):** a per-key byte, e.g. `portCarShadowLift[TM2_CAR_KEYS]`
  (`.port_bss`, set by the port from the mod's description, default retail's 3 / 0 by type), read in
  `carBodyPoseUpdate` instead of the type test under a define; the retail build unchanged, a proof and one gate
  check. The mod author gives the offset.
- **Medium:** derive it from the mesh: at `carBodyObjectRegister` walk node 3's vertices (the DMD walker the unload
  code already has) for its lowest z and set the lift so the blob sits a fixed amount above the ground. It must give
  exactly retail's 3 / 0 for the 15 retail cars (to be checked on the disc models), or it would change retail's look.
- The tilt's footprint follows the stats (track width, axles), like the collision points (12.10): a different
  footprint for a mod needs its stats changed, which is the car-stats question, not the shadow's.

### 13.6 `rand()` (round 3)

No new call site. Bit 14: none (reserved). Bit 16: a late connect draws only the connect's `carSpinOut` at the
connect (request 41 also drew the first hit's spin roll there). Bit 20: **one retail call fewer per connecting
bullet** (13.3: `bulletsMove`'s damage coin flip); the gun hits themselves draw nothing (amounts below 8), a kill
draws the wreck's `carSpinOut` as any kill. Word zero: retail's stream (the `rand()` still runs when
`portBalanceGunHit` returns -1).

### 13.7 Verification

As 10.1, on this branch (proof table in 10.2): retail builds byte-identical; both proofs and the placement check
with no define, `TM2_PORT_BALANCE` alone, the car stack + `TM2_PORT_BALANCE`, all + `TM2_PORT_HUMANS=16`;
`check_balance.py --expect-functions` (20 functions with the define alone, G15 retired, G17 and G21) on the three
builds with the define and `--negative-control` on the one without; `tools/check_all.py` on the four.

## 14. Request 45 (2026-10-17 playtest)

PC port request 45 (`PLAYTEST_2026-10-17.md` in the port: the Vanilla Plus menu's Character Balancing section, item
6 the ricochet). The rendering and car-select questions of the same request are in `PLAYTEST_ANSWERS.md`.

### 14.1 Bit 8 split per car (bits 21 and 22)

The Vanilla Plus menu shows one option per car, so each recharge has its own bit now:

| bit | mask | name | rule |
|---|---|---|---|
| 8 | 0x0100 | `TM2_BAL_RECHARGE` = `TM2_BAL_OUTLAW2_RECHARGE` | Outlaw 2's recharge 1100 -> 900 (only Outlaw 2 since request 45) |
| 21 | 0x200000 | `TM2_BAL_GRIMM_RECHARGE` | Mr. Grimm's recharge 1400 -> 1200 |
| 22 | 0x400000 | `TM2_BAL_THUMPER_RECHARGE` | Thumper's recharge 1600 -> 1500 |

`TM2_BAL_ALL` = `0x7FFFFF` (bits 0-22; 9 and 14 reserved). The gate is the same (`weaponTick` -> `portBalancePeriod`,
G10); only the port function changed: its early-out mask names the three bits and each car's `case` tests its own
bit (`port_variant_arrays.c`, `portBalancePeriod`). **For the port:** a word saved before request 45 with bit 8 set
meant all three; to keep such a setting, set bits 21 and 22 too when reading an old save (or online ruleset) whose
bit 8 is set. `check_balance.py` G10 now also checks the source (each car's case tests its own bit, bit 8's old name
is not used for Mr. Grimm or Thumper) and the header (`TM2_BAL_ALL` is the OR of every bit, 0..22, no two equal).
`rand()`: none.

### 14.2 Ricochet from a fast car: the bounce off its owner (bit 18)

**What the user saw:** fired at top speed (and with turbo), the bomb seems to jump forward with the car and blinks
red, as if it had bounced; with bit 18 it never hits him.

**The shot (CONFIRMED, retail).**

- **Launch** (`weaponFireSelected` case 6, `weapon_select.c:139-176`): the shot object is placed at the car's origin
  + 40 in z (`:160-161`; no forward offset), heading = the car's yaw (or reversed with rear fire), local velocity
  `offset = {0, 100, 0}` (`:168`), life 400, bounces 0, owner = the car's object id, `collides = 0` (`:148`: cars'
  own hit passes skip it, so the shot never pushes a car and no car pushes the shot). **No speed is taken from the
  car**: nothing reads the car's velocity; every update the shot moves exactly 100 units along its heading. The car
  moves first (`humanCarTick`: `carMoveByVelocity` at `car_tick.c:142`, the fire at `:153`), so after its first
  update the shot is 100 units ahead of the car's origin.
- **Per update** (`weaponRicochetUpdate` 8010CC54, `weapon_ricochet.c`, once a world update after
  `carsPhysicsUpdate`, `world_update.c:543` / `:559`): age, move 100 (`:108-116`), then the **hit pass**
  `hdpCollideObject(shot, &hit, 1, matrix, prev)` (`:117`), the kind filter (`:118`: kinds 667-668 and 401-440 are
  ignored), then if `hit.hit`: a target kind 500-520 of a human's shot detonates it (`:126-129`); otherwise **the
  bounce** (`:130-153`): `mathReflectHeading` reflects the heading on the hit normal, `bounces++`, on the **first**
  bounce the power doubles and `lightFlickerStart(shot, 100)` starts the flicker (`lightFlickerTick` 80114D08,
  `fx_explosions.c:155`: for 100 frames the object's light environment is switched at random, 1 frame in 3: the red
  blinking), the shot is pushed out by `depth + 20` along the normal, a second hit pass (`:141`) and, if still
  inside, a step back and a half turn (`:148-153`). Then the ground follow (`:156-160`: z = max(ground + 16, a blend
  toward it)), then the **car tests** (`:162-195`): `DISTANCE(shot, car) < 20` (160 units on every axis) explodes it
  on a car (`weaponApplyHit`, `carSpinOut`, `carKnockBack`, `:167-177`), the owner only once `life < 398` (a
  human's) / `< 396` (an AI's) in retail, through `OWNER_HIT` (`portBalanceRicochetOwner`) with bit 18. At
  `life == 100` the flicker starts again (`:203-204`: the warning before it expires).
- **The hit pass tests the owner's car (CONFIRMED).** `hdpCollideObject` (8014E66C, `engine_hit_detection.c:216`)
  runs `hdpPassRun` (8014FAB0): the level's tree first, then, for the points still unhit, **every object of the
  scene list whose words +8 and +0xC are both non-zero, except the object itself** (8014FC50..8014FC70): every live
  car, **its owner's included** (nothing tests the owner). The shot has one hit point, `(0, 0, 40)` in its own frame
  (`asset_pools.c:87`, `PROJECTILE(.., 40)`: +0xB4 = 1 point, +0xB8 = (0, 0, 40)), so the test point is **40 units
  above the shot**. A hit records the object (the point record's +0x14, `ricochet hit.f18[0]`, 8014F4FC) and the
  kind: a car model is one group whose id is the car type (Axel 176, Outlaw 2 48, ...) holding one hit box with id 0,
  so the kind is the **car type, 16..240** (8014EBA4..8014EBB0 and 8014F634..8014F648; counted in the 15 car DMDs on
  the disc): not filtered, so **a car's hit box is a wall to the ricochet**: the bounce above.
- **The owner's box vs the test point (LIKELY: the box frame read as the car object's, z up, y forward; flat ground,
  30 frames a second).** The boxes (one per car DMD, 6 axis-aligned planes) and the test point's height above the car's
  origin: 80 at the launch, then `56 - ride + (ride + 24) x 0.7^k` after k updates (the ground follow), settling at
  `56 - ride`:

| Car | ride height | box z (up) | box front (y) | test point z after 1 / 2 / 3 updates | settled | inside the box's height from update |
|---|---|---|---|---|---|---|
| Sweet Tooth | 32 | -32..58 | 58 | 63 / 51 / 43 | 24 | 2 |
| Hammerhead | 66 | -66..40 | 65 | 53 / 34 / 21 | -10 | 2 |
| Outlaw 2 | 16 | -16..24 | 61 | 68 / 60 / 54 | 40 | never |
| Warthog | 26 | -26..32 | 61 | 65 / 54 / 47 | 30 | 10 |
| Mr. Grimm | 17 | -16..42 | 54 | 68 / 59 / 53 | 39 | 8 |
| Grasshopper | 17 | -18..34 | 41 | 68 / 59 / 53 | 39 | never |
| Thumper | 14 | -14..18 | 62 | 69 / 61 / 55 | 42 | never |
| Spectre | 16 | -16..17 | 52 | 68 / 60 / 54 | 40 | never |
| Roadkill | 16 | -16..33 | 56 | 68 / 60 / 54 | 40 | never |
| Twister | 12 | -10..19 | 61 | 69 / 62 / 56 | 44 | never |
| Axel | 15 | -16..58 | 39 | 68 / 60 / 54 | 41 | 3 |
| Mr. Slam | 32 | -32..56 | 57 | 63 / 51 / 43 | 24 | 2 |
| Shadow | 20 | -19..30 | 64 | 67 / 58 / 51 | 36 | never |
| Minion | 27 | -28..84 | 126 | 65 / 54 / 46 | 29 | 1 |

  (Dark Tooth's model has 9 boxes reaching 190 ahead.) Lengthwise the shot is 100 ahead after its first update and
  `100 + (k - 1)(100 - v)` after k for a car covering v units an update: it stays ahead of the box's front (39-65)
  unless the car is **faster than its shot** (v > 100: top speed with turbo, `BALANCE.md` 12.6 / `RETAIL_BUGS.md`
  28: about 52-80 an update at top speed, up to about 103 with turbo). Minion's box (126 ahead, 84 up) holds the test
  point at the first update.

**So, with request 41's bit 18 (before this change).** A car at about the shot's speed (top speed + turbo) keeps the
shot 100 units ahead for as long as it keeps that speed, never 160 away: the shot never leaves his box, so it can
never hit him (bit 18 works), and it **looks carried by the car** (it is simply not faster than the car). If the car
is faster than 100 an update it closes in; for a car whose box is tall enough (Axel, Sweet Tooth, Hammerhead, Mr.
Slam, Mr. Grimm, Warthog late, Minion always) the test point enters his box: **a bounce off its owner**: the heading
reflected, the power doubled and the **red flicker** at the first one, and the push-out of `depth + 20` out of his
box; the next update it moves into him again and bounces again, so it is pushed along in front of him until he
slows down or turns. For the low cars (Outlaw 2, Thumper, Spectre, Roadkill, Twister, Grasshopper, Shadow) the test
point passes over the box and the car drives through its shot (no bounce).

**Retail (CONFIRMED mechanism, LIKELY geometry).** The same hit pass, the same bounce: retail has no owner test
there either. But in retail the shot of a car faster than about 70 an update **explodes on its owner at its third
update** (the timed immunity, `RETAIL_BUGS.md` 28), before it can be caught up, so the bounce off a fast owner is not
seen; bit 18 removed that explosion and so made the bounce visible. A **Minion** (box 126 ahead, 84 up) bounces its
own ricochet at the first update in retail too (red flicker, power 14 from the start): `RETAIL_BUGS.md` 31.

**The change (bit 18, request 45; built).** Under the same bit (it is the same mechanism and the same rule: the shot
ignores its owner until it has left him once): after each hit pass and its kind filter (`weapon_ricochet.c:120-124`
and `:144-147`) `if (portBalanceRicochetPass(&SHOT, hit object)) hit.hit = 0;`. `portBalanceRicochetPass`
(`port_variant_arrays.c`, after `portBalanceRicochetOwner`) returns 1 when the bit is set, the shot's `left_owner`
byte is still 0 (it has not yet been outside his box at a car test: `portBalanceRicochetOwner`), and the hit object
is a car (`carFindByObject`) whose object id is the shot's owner. The hit is dropped: **no bounce, no doubled power,
no flicker, no push-out, no step back**; the shot rolls on through him. Once it has been outside his box (160 on an
axis) it is retail's again: it bounces off him and can hit him. Other cars are untouched (they still bounce it, and
its 160 box hits them as retail). Notes:

- A hit pass records one volume per point; if the point was inside the owner's box **and** another object's in the
  same update, the record may hold the owner's and is then dropped (the other object is not bounced off that update;
  the other car is still hit by the car test). The level's tree is walked first and the objects only while a point
  is still unhit (8014FBE4..8014FC48, LIKELY), so a wall hit is not replaced by the owner's box.
- The "jump forward with the car" at about the shot's speed stays: the shot's speed is retail's 100 an update, no
  speed is inherited. Giving it the car's forward speed would be a new rule (a bit of its own), not built: say if
  wanted.
- `rand()`: a self-bounce no longer starts the flicker timer, whose frames draw `rand() % 3` (`lightFlickerTick`); the
  same on every machine for the same word.
- `check_balance.py` G19: two `portBalanceRicochetPass` calls, each after a hit pass, every `mathReflectHeading`
  reachable only through one; the source puts one after each hit pass's kind filter, before `if (hit.hit) {`.
- The port's test (10.4 item 17): fire a ricochet at top speed with turbo as Axel (a tall box) and as Outlaw 2 (a low
  one), straight on: no red flicker at the launch, the shot not pushed; as Minion, parked: no flicker at the launch.
  Then let a shot bounce off a wall back into its owner after it has left him: it bounces off him / hits him as
  retail.

### 14.3 Verification

Request 45 (2026-10-17), E0 = `work/build-20261016-210450-578481/source_reloc_e0`; every row by `pv.py` (P4 and the
placement link P2, `verify_reloc.py` against E0) and `tools/check_all.py work/pv/r45_<tag> --e0 <E0>`: both proofs
and the placement check pass; `check_balance.py --expect-functions` on the three builds with `TM2_PORT_BALANCE`
(exactly the 20 functions with the define alone; G10 with the per-car source / header check, G19 with the
pass-through), `--negative-control` on the build without it fails as it must; G19 fails on request 42's link
(`work/pv/r42_allh16_P4`: no `portBalanceRicochetPass`) and G10 / G19's source checks fail on the previous sources
(negative controls); `check_custom.py --expect-functions` on the builds with `TM2_PORT_VISUAL` (bit 2:
`portVisualOwnCar`'s callees and caller, `renderScene` changed; with `TM2_PORT_VISUAL` alone exactly `weaponArcsDraw`
and `renderScene`), its `--negative-control` on the build without it; `check_car_groups.py`, `check_hazards.py`,
`check_humans.py` on the builds they apply to. Car stack as in 10.2; all = the car stack + `TM2_PORT_FAR_LOD
TM2_PORT_FAR_DISTANCE TM2_PORT_SPU48 TM2_PORT_HAZARDS TM2_PORT_HUMANS=16 TM2_PORT_BALANCE TM2_PORT_VISUAL
TM2_PORT_CAR_NUMBERS`.

| Defines | P4 sha | P2 (placement) sha | port end (P4) |
|---|---|---|---|
| none | `c1108ee81c3d` | `b95e6fed7439` (as before) | 80545710 |
| `TM2_PORT_BALANCE` | `8a4b45332f79` | `de9008d8921e` | 80546F10 |
| `TM2_PORT_VISUAL` | `dabe1cf7a13a` | `e1a97e4531a7` | 80545840 |
| car stack + `TM2_PORT_BALANCE` + `TM2_PORT_VISUAL` | `e4360fbbef2b` | `a7332a8ceb83` | 8059A3B8 |
| all + `TM2_PORT_HUMANS=16` | `bbf38feadade` | `9e51263e0645` | 805A4F7C |

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261017-220541-042954`):
hybrid `9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB`
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `c1108ee81c3d…` (unchanged from request 42: no code without the defines; placement check OK);
`tools/check_all.py` on it passes.

## 15. Request 46 item 3 (2026-10-18 playtest): Twister's shield during her tornado (bit 15 changed)

The user (`PLAYTEST_2026-10-18.md` 7 in the port): while her tornado runs, Twister may use her **shield** (the energy
attack), the one thing she may use. As asked, this is the default of bit 15 now, not a bit of its own.

| | request 41 (old bit 15) | request 46 (bit 15 now) |
|---|---|---|
| pickup weapons, her special, a remote bomb's detonation | refused (denied sound, no cost) | refused (unchanged) |
| the machine guns | refused (the sound once per press) | refused (unchanged) |
| energy attacks 100-102, 104-106 (missiles, mines, jump, invisibility, the freeze) | refused | refused (unchanged) |
| the shield (energy attack 103) | refused | **allowed**: retail's cast (400 energy, a shield of life 60, the cooldown) |

**How (CONFIRMED in the code).** `weaponFireEnergyAttack` (801046FC, `weapon_special.c`) calls
`portBalanceEnergyLocked(car, human, kind)` (new, `port_variant_arrays.c`, after `portBalanceLocked`) instead of
`portBalanceLocked`; it returns 0 when `kind` (the energy attack, `(signed char)weapons.f0C`) is 103
(`TM2_BAL_LOCK_FREE_KIND`, `port_balance.h`), else `portBalanceLocked(car, human)`. `weaponFireSelected` keeps
`portBalanceLocked` (no shield there: the shield is only an energy attack). Humans and computer cars alike (the AI's
shield requests go through the same function). Bit 1 (`TM2_BAL_SHIELD`) still refuses a recast while shielded, and
retail's own 400-energy test still applies; nothing else of the tornado changes (bit 6's rules, bit 12's throw).

**Old behaviour** (for a port setting that wants it): there is no bit for the shield-locked form any more; a build
before request 46 (or a ruleset that must match one) locks the shield too. `TM2_BAL_ALL` is unchanged (`0x7FFFFF`).

**Gates and checks.** One game function changed in this request: `weaponFireEnergyAttack` (it was a G16 gate already;
the changed set of `--expect-functions` is the same 20). `tools/check_balance.py` G16: `portBalanceEnergyLocked` is
the energy gate (called once, dominates the five energy dispatches, none reachable from its refused edge), its code
tests the kind against the immediate 103 and its source returns 0 for `TM2_BAL_LOCK_FREE_KIND` before calling
`portBalanceLocked`; the callers table has `portBalanceLocked` <- `weaponFireSelected`, `portBalanceEnergyLocked` and
`portBalanceEnergyLocked` <- `weaponFireEnergyAttack`. `rand()`: none added (a shield cast calls none).

## 16. Request 67 (2026-10-25): no wasted pickup respawn (bit 23)

**Asked:** `pickupRespawnTick` (8010AED0) draws `rand() % pickupRegisteredCount` and wastes the turn when that
pickup is still on the map or hidden in an unbroken target (`PICKUP_LOOKS.md` 4). Keep the randomness, no wasted
draws: one `rand()` as retail's, `% the count of the slots that are taken and can show`; health keeps its half
chance; nothing to bring back: nothing.

**Bit 23, `TM2_BAL_PICKUP_DRAW` (0x800000)**; `TM2_BAL_ALL` is now **`0xFFFFFF`** (bits 0-23, bits 9 and 14 reserved).
Latched per round like the others (`portBalanceLive`). Every mode.

**How.** `pickupRespawnTick` takes its pick from `portBalancePickupSlot(rand())` (`.port_text`) and returns when it is
-1; the rest is retail's (the health ids 417 / 418 draw the second `rand() < 16384`, a failed one wastes the turn as
in retail; the slot's active flag set; `pickupSetVisible(id, n, 1)`). With the bit clear the function returns
`r % pickupRegisteredCount`: retail. With it:
- **taken** = a map pickup (id 401..430, `pickupSetVisible`'s range) whose slot's active word is 0 and whose model is
  hidden (`pickupObjects[n]` kind 1: child count +20, kind 4: +18 at 0). The model decides: `pickupSlotsReset` leaves
  active 0 for every slot, also the pickups the load shows, so active alone would count them;
- **can show** = the model is reached from the level model (`csGetRoot()` +0x11C) through the current child of every
  select (kind 9: a target or a level event, 500..520, 4000, 4005, keeps its pickups in its other child while whole,
  `PICKUP_LOOKS.md` 5), every child of the other nodes (animations, distance switches, zones, transforms)
  (`portBalancePickupShowable`: a read-only walk, a 384-entry stack, at most 65,536 nodes, a bit per slot);
- the pick is the `(r % n)`-th of the n such slots in slot order; n = 0: -1, nothing comes back.
One `rand()` per respawn tick as retail (also when nothing is eligible), the health one only for a health pick, as
retail: the stream's use per respawn is retail's; which pickup comes back differs. Deterministic: it reads the
pickup table, the models' visible counts and the selects' children, all match state (the `dmd` rows hash the
models' mutable fields).

**Check:** `tools/check_balance.py` G22: `pickupRespawnTick` calls `rand()` twice (as the base) and
`portBalancePickupSlot` once, right after the pick's `rand()`, and no `pickupSetVisible` is reachable without it;
`portBalancePickupSlot` runs on check_custom's MIPS interpreter on a made-up level model and slot table (shown,
hidden, inactive / still active, behind a select's other child, a kind-4 pickup, health): bit set, r = 0..39 picks the
(r % n)-th eligible slot; the select switched, the pickup behind it joins; all back on the map, -1; bit clear,
`r % count`; it writes only its stack. Negative control: a link without `TM2_PORT_BALANCE` (no function) fails;
the made-up table's answers differ from retail's `r % count` (the run is not blind).

**Port's build** (`work/pv/r63_port_P4`): `portBalancePickupSlot` 808E350C,
`portBalancePickupShowable` 808E3224, `portBalancePickupTaken` 808E3470,
`pickupRespawnTick` 8010CBC4 (changed). **The port:** add bit 23 to Vanilla Plus / online
(`portBalance = 0xFFFFFF`). Verification: full build `work/build-20261025-192227-415794` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK from `.LIB` `f557c1e3…`, default port variant `1becd00a…`: unchanged); links `work/pv/r63_{rb,port,allh16}_{P4,P2}` (port's `r63_port_P4` `dda33c12…`); `check_all.py` 27/27 (proofs, placement, check_rollback, check_custom 9, check_balance G22, check_humans 13). Not runtime-tested.

## 17. Request 77 (2026-10-28): Hammerhead's movement (bit 24)

Port request 77 (the user's, after the port's Hammerhead analysis): Hammerhead's **brakes (stats +0x5C, +92) 1 -> 2**
and **slide speed (+0x56, +86) 25 -> 60**, nothing else (maybe 70 for the slide later), for the player's and the
computer's Hammerhead alike and after a car swap, under a new bit 24.

- **Bit 24, `TM2_BAL_HH_MOVE` (0x1000000)**; `TM2_BAL_ALL` is now **`0x1FFFFFF`** (bits 0-24, 9 and 14 reserved).
  The numbers: `TM2_BAL_HH_BRAKE` 2, `TM2_BAL_HH_SLIDE` 60 (`port_balance.h`).
- **The gate:** `carStatsLoad` (800FE15C) calls `portBalanceCarStats(s, type)` after the type's own values
  (`carStatsHammerhead`) and before a mod profile's (`portCarProfileStats`, request 70) and the derived fields
  (`carStatsSetTopSpeed` -> `carStatsDerive`: braking +0xCC = accel x brakes, the grip limit +0xC0 from the slide
  speed). Every load of type 0x20 goes through it: a human's restart (`humanCarPlaceAtStart`), a computer car's bind
  and restart (`aiCarReset`), a swap (`portCarApplyType`). A mod car whose base is Hammerhead gets them too unless its
  profile sets those fields.
- **Read from `portBalance`, not `portBalanceLive`** (the one gate that does): computer cars load their stats at bind
  time, before `roundStart`'s latch (3.10's reason for gating recharge at the reader), so the latched word would
  still hold the round or match before. The port writes `portBalance` at the match's load and not after
  (`four.c`), so every load of a match sees one value, the same on every machine online. Changing the option takes
  effect at the next load of each car (the next round for humans, the next match for computer cars).
- **`rand()`:** none. With the bit clear retail's 1 and 25.
- **Checked:** `tools/check_balance.py` G23: `portBalanceCarStats` called only by `carStatsLoad`, once, before
  `carStatsSetTopSpeed`; `carStatsLoad` run in the link (check_custom's `LinkMips`): Hammerhead with the bit gives 2,
  60 and braking accel x 2; with it clear (and `portBalanceLive` set) 1, 25; Outlaw 2 with the bit keeps 3, 103.
  Not tested in game.

## 18. Request 88 item 3 (2026-10-30): Grasshopper's touch is not a hit (bit 25)

The user does not want "touching her target counts as the hit" (8c); bit 7 keeps 8a (18 damage) and 8b (no ram
damage taken while leaping). Built as the request's second option: **8c has a bit of its own, bit 25 `TM2_BAL_LEAP_TOUCH`
(0x2000000), left out of `TM2_BAL_ALL`** (still `0x1FFFFFF`), so the port's Vanilla Plus / online word does not set
it. `portBalanceLeapTouch` tests bit 25 instead of bit 7; nothing else changes (with bit 25 clear her leap ends on a
touch as retail's: the cancel test, no hit). `rand()`: none. Checked: `tools/check_balance.py` G10's bit census
(every bit but the opt-in ones in `TM2_BAL_ALL`, bit 25 its own) and G11 (the touch called once). Not tested in game.

## 19. Request 91 item 2 (2026-11-01 playtest): Mr. Slam locked while his special runs (bit 26)

The user: while Mr. Slam's arm and claw special runs he kept firing missiles and other weapons, nearly a one-shot
kill. Now, as Twister's tornado (bit 15, 12.3 and 15), **bit 26 `TM2_BAL_SLAM_LOCK` (0x4000000, in `TM2_BAL_ALL`)**:

| While his special runs | |
|---|---|
| pickup weapons, his special again, a remote bomb's detonation | refused: the denied sound (humans), no ammo, no cooldown |
| energy attacks 100-102, 104-106 | refused the same way |
| the shield (energy attack 103) | allowed (retail's cast) |
| the machine guns | allowed (retail's) |

**"His special runs"** (`balSlamRuns`, CONFIRMED in the code): the car's type is 0xC0 and its slam record (weapons
+0x48, `struct TM2CarSlam`) has `active` set. `weaponFireSpecial` 0xC0 sets it at the fire (`weapon_basic.c`), and so
does retail's bump start at the easiest difficulty (`carCarCollision`, `car_collision.c`). The car update
(`ai_driver.c`, and the human's) clears it once neither the grab nor the swing runs. So the lock lasts from the
press until the held car is dropped or thrown.

**How:** `portBalanceLocked` (the gate of `weaponFireSelected`, and through `portBalanceEnergyLocked` of
`weaponFireEnergyAttack`, which lets the shield through) is now `(bit 15 && her tornado runs) || (bit 26 && his
special runs)`. `portBalanceGunsRefused` tests bit 15 only, so his guns fire. No game function changed beyond bit
15's gates. Humans and computer cars alike. `rand()`: none.

**Checked:** `tools/check_balance.py` G16 runs `portBalanceLocked` in LinkMips on made-up human and AI records. It
gives 1 for a Mr. Slam with an active record and bit 26 set; 0 with the record idle, with no record, with only bit 15
set, and for another type. G16's source test confirms the guns' gate does not name bit 26. G10's census confirms
bit 26 is in `TM2_BAL_ALL`. Not tested in game.

## 20. Request 91 item 3 (2026-11-01 playtest): computer cars jammed in a pile (bit 27)

The user (Juggernaut, Hong Kong): several computer cars chasing one target jam in a street, backing up a moment and
ramming forward again for many seconds.

**What retail's AI does (CONFIRMED in the code):**
- A contact with anything sets the car's stop flag (`carRicochetOffObject` 800E4754, `f453`).
  - `aiMovementUpdate` then runs only `aiUpdateStop` 800EC504, which brakes to 0.
  - It backs out for 30 ticks (10 frames) only if the hit turned it round (`f03F`), else not at all.
  - Then it re-plans (`aiReplanRoute` 800E9578).
- The re-plan starts from the nearest route node (`carNearestRoutePoint`), and the greedy pick (`aiPickLinkNode`
  800F7B00: the successor nearest the aim point) gives the same node again. The same ram follows.
- The stuck counter (`aiUpdateStuck` 800E8DFC) counts wall and object bumps only (`drive.f474`, from
  `aiWallBumpCount`; `carCarCollision` clears the wall flag of a car-car contact). A car-car jam resets it at every
  re-plan, so the respawn-when-stuck never comes.
- Near the target the car drives straight at it (`aiShouldChaseDirect` 800EBB74, which needs no wall bump and
  `stuck_time < 6`), whatever is in between.
- There is no try-another-node logic and no steering apart between computer cars.

**Bit 27 `TM2_BAL_AI_UNJAM` (0x8000000, in `TM2_BAL_ALL`):**
- In `aiUpdateStop`, when the car has stopped, before its re-plan: `portBalanceJamStop`.
  - A stop counts when there was no wall bump since the last re-plan (`f474` 0) and the car last hit
    (`contact.last`) is not its own target (`aiGetTargetCar`). Ramming the target stays retail's.
  - `TM2_BAL_JAM_STOPS` (3) counted stops within `TM2_BAL_JAM_WINDOW` (120 frames, 6 s) make a jam.
  - The car then backs out (the gate sets `f03F`, so retail's 30-tick reverse runs) and goes round for
    `TM2_BAL_JAM_AVOID` (100 frames, 5 s).
- Going round:
  - `aiPickLinkNode` leaves out the route node it was heading for at the jam (`drive.node_next`): `portBalanceJamAvoid`.
    If every candidate is left out, the pick is retail's default.
  - `aiShouldChaseDirect` returns 0 (`portBalanceJamRound`), so it follows the route nodes instead of driving at the
    target through the pile.
  - Afterwards it is retail's again: the greedy pick may bring it back the other way round, at the target's far side
    or behind the pile once it has thinned.
- State: `portBalanceJam[TM2_MAX_CAR_SLOTS]` (12 bytes a slot, `.port_bss`, every variant, in `port.bss`'s rollback
  row: snapshot and hash). It is reset in `portBalanceLatch` and when a car leaves its slot.
- Frames are `timerFrameCount` (`GsGetWorkBase_0`, 20 a second). `rand()`: none, so the AI stays deterministic for
  online play.
- Three game functions are changed under `TM2_PORT_BALANCE`: `aiUpdateStop`, `aiPickLinkNode` and
  `aiShouldChaseDirect`.

**Checked:** `tools/check_balance.py` G24.
- Each gate is called once, and `aiReplanRoute` in `aiUpdateStop` is reached only after `portBalanceJamStop`.
- LinkMips runs on a made-up AI record:
  - nothing happens with the bit clear;
  - the 3rd stop by a car makes the jam;
  - the car then avoids its jammed node only, with no direct chase, and both end after 100 frames;
  - ramming the target, stops with a wall bump, and stops farther apart than the window do not count.
- EXPECTED has the three functions.

Not tested in game: the user's next Juggernaut in Hong Kong is the test. The numbers (3 stops, 6 s, 5 s) are
first guesses; say if cars give up too soon or too late.

## 21. Request 95 (2026-11-02 playtest): Roadkill's boomerang comes back without the charge (bit 28)

### 21.1 Why (CONFIRMED in the code)

The fields: `fxRecords`, `projectile_hit.c` struct `TM2Proj`. 1 D = the source's `DISTANCE`: the largest of |dx|,
|dy|, |dz|, >> 3, so 8 position units.

- **Thrown:** `weaponFireRoadkillBoomerang` 800D98E0 calls `weaponFireHomingMissile`, then sets life 200 steps,
  speed 75 (+5 a step, at most 150) and turn rate 5.
- **Turned back** past 300 D from its launch point (`weaponProjectileUpdate` 800DB110 without a target, or
  `weaponProjectileSteer` 800DA5B4 with one):
  - Its target becomes the thrower's live position.
  - Damage x4, turn rate 30, speed 50 again.
  - On the way back a wall hit (value 8) is ignored, so it flies through walls.
- **Caught:** only in `weaponProjectileUpdate`'s two sub-steps. `projectileHitTest` must return the thrower within
  its radius: 15 for a human (< 16 D), 8 for a computer car.
- **The dead zone:** before that, each step `weaponProjectileSteer` frees a returning boomerang within 20 D of its
  thrower, without a charge.
  - So one that ends a step 16-19 D from the car is lost the next step. That is 32 position units of 160.
  - At a closing speed v a straight approach loses it about 32 / v of the time: a fifth head-on at 150, more when
    chasing a car that drives away. Passing beside the car at 16-19 D always loses it.
- **Computer throwers never get it back:** the computer loop of `projectileHitTest` tests `id != owner` first, so it
  never returns the thrower. The charge is lost at the silent removal or at the life's end.
- **The life's end** (life 0) explodes it, no charge.
- **Retail's charge:** +1, not capped, to the first human of type 144 (Roadkill), not the thrower.
- **The port variant's bug** (`TM2_PORT_SPECIAL_PROFILE`): `portSpecialBoomerangCaught` ran in both sub-steps when
  both were within the radius, giving +2. Fixed in this PR for every build of that define, bit or not: once, with
  retail's `flag`.
- No `rand()` on the path.

### 21.2 Built: `TM2_BAL_BOOMERANG` 0x10000000 (bit 28; `TM2_BAL_ALL` now 0x1DFFFFFF)

`portBalanceBoomerangCatch(rec, pt)` (`.port_text`) gives the charge back only when the boomerang comes back to the car
(the user's rule, 2026-11-03):

- **Back at the car:** caught anywhere within `TM2_BAL_BOOMERANG_CATCH`. That is 20 D (160 position units), the
  distance at which retail's steering takes it away, so every boomerang that reaches the car is caught.
- **Everything else is retail's:** a hit (a wall on its way out, a car, anything it explodes on) or its life running
  out gives nothing. On its way back it flies through walls (value 8), as retail.

- **When it counts as caught:**
  - type 51;
  - its target is the thrower's live position (it has turned back);
  - life left;
  - the thrower is alive (`carFindById`, `state.dead`).
- **On a catch:**
  - it is freed (`fxRecordFree`), with no explosion;
  - the thrower gets +1 charge, at most its `max_specials`;
  - a car with nothing selected (`selected >= 8`) selects its specials, as the recharge does.
- **Called from:**
  - `weaponProjectileSteer`, before retail's silent removal (the dead zone);
  - `weaponProjectileUpdate`, each sub-step after the car test (any thrower, either radius).
- **Bit clear:** the function returns 0 at once, and every path is retail's.
- **Game state:** online it is the same on every machine (the latched `portBalanceLive`).
- **Checked:** `check_balance.py` G25.
  - The calls: steer 1, update 1. Steer's `fxRecordFree` can't be reached without the catch call first.
  - A LinkMips run of the catch on made-up records: human and computer throwers, the bit clear, at the catch
    distance, far away, not turned back, a dead thrower, timed out (life 0) at the car, a dead record, the cap,
    nothing selected.
  - G12 now allows only this `portBalance` call in `weaponProjectileUpdate`.
- Not tested in game.

## 22. Request 101 (2026-11-04 playtest): the hunters pile up on Roof Tops (bit 29)

The user (Juggernaut, Roof Tops, 15 computer cars): the hunters gather in piles, backing up and going forward, and
never reach the Juggernaut. The port measured the same spots over and over: about (4300, 8500), (6300, 4100) and
(2800, 7000), and a computer Juggernaut parked at (1085, 9502).

### 22.1 What stops them (CONFIRMED in the code and the disc's data)

- **Roof Tops is SROOF, not ROOF.** Arena 12's loader calls `routeLoadSroof`: 9 route nodes from
  `TERRAIN\SROOF.PTS`, 14 sections in `SROOF.GRP`. `routeLoadRoof` (101 nodes) is New York.
- **Its TM1 route data is broken in three ways.** No other level is like it (a survey of all 11 levels: their nodes
  carry their own sections and their tables name real nodes).
  - Every node is tagged section 6; only node 8 is in it. `carNearestRoutePoint` takes only nodes of the car's own
    section and returns node 0 when none is. So every re-plan of a car off section 6 heads for node 0, at the far
    east corner, and a car marked stuck is respawned there.
  - The next-node table (GRP +0x354) names node 17 for most target sections (TM1's leftovers, up to node 48, 114
    entries past the 9 nodes). Entry 17 is all zero, so a hunting car aims at (0, 0).
  - The graph is one-way west to east: the east roof {0..5}, the west roofs {6, 7}, node 8; the only crossings are
    6 -> 0 and 7 -> 8 -> 4. Nothing leads from the east roof back west.
- **What retail's AI then does:**
  - Near the target it drives straight at it (`aiShouldChaseDirect`), over a gap or into a roof edge.
  - The bump stops it (`aiUpdateStop`), the re-plan sends it toward node 0 again, and the same thing follows.
  - Bit 27's unjam leaves out stops with a wall bump, so it never fires there.
  - Cars blocking each other add to it near the target (LIKELY), but are not the cause.
- **Each spot** (LIKELY: the target's position at each moment is not in the trace):
  - (4300, 8500) and (2800, 7000), the west lower roof: aimed at node 0, east, into the roof's edge, with the
    Juggernaut up on the north roof.
  - (6300, 4100), the east roof's west edge: a direct chase into the gap.
  - The Juggernaut at (1085, 9502) in mood 2: its re-plans go to node 0 too.
- **Also seen:** the Roof Tops loader never sets `arenaShortChaseRange`, so it keeps the previous arena's value.
  Every machine must have played the same arenas in order for the AI to match online.

### 22.2 Built: bit 29 `TM2_BAL_AI_ROUTE` (0x20000000, in `TM2_BAL_ALL`)

- **The nearest node** (`carNearestRoutePoint` -> `portBalanceRouteNearest`): when no node carries the car's
  section, the nearest node of any section (the same |dx| + |dy| + |dz|), not node 0. Re-plans and stuck respawns
  start from where the car is.
- **The graph's way** (`aiFollowRouteTarget` -> `portBalanceRouteNext`), only where retail's table gives none: its
  entry is past the level's nodes, or the target is in the section of the car's node while the car itself is not
  (a mis-tagged node: retail would chase straight).
  - A breadth-first walk of the successor lists from the car's node to the target's nearest node (of the target's
    section, else of any; lower indices first).
  - The car heads for the next node on that way; at the target's node it chases the target, as retail on the same
    section.
  - When the graph cannot reach it, the goal is the reached node nearest the target, and the car keeps to it
    (patrols it) instead of ramming an edge.
- **No direct chase out of reach** (`aiShouldChaseDirect` -> `portBalanceRouteBlocked`): no straight chase when the
  graph reaches neither the target's nearest node nor any node within `TM2_BAL_ROUTE_NEAR` (1,200) of the target.
- **Everywhere else it is retail's:** a level whose tables are good keeps its own choices. (A mis-tagged node, as in
  Ice and Hong Kong, takes the graph's way only while the target is in the node's section and the car is not.)
- **State: none.** The graph is walked on demand from the level's route nodes (at most 140), the same on every
  machine; nothing to snapshot or hash. `rand()`: none.
- **Changed under `TM2_PORT_BALANCE`:** `carNearestRoutePoint`, `aiFollowRouteTarget`, `aiShouldChaseDirect`.

### 22.3 What the decomp cannot do: the east roof back to the west

- TM1's data has no way from the east roof to the west roofs. Whether a car can drive or jump back there was not
  checked against the level's walls, so no link was made up.
- A hunter on the east roof with the Juggernaut on the west keeps to the east node nearest it. For the port's
  targeting (the Juggernaut's hunters, King of the Hill's, the targets'): `portBalanceRouteBlocked(node, pos, sec)`
  says whether the graph can take a computer car to a target. `node` is the car's current node (AI record +0x464,
  `drive.node`), `pos` the target's position and `sec` its section (+0x46A for a computer car, `state.x2C.route` for
  a human). A blocked hunter can be given another target it can reach.

**Checked:** `tools/check_balance.py` G26.
- Each gate is called once.
- A LinkMips run on Roof Tops' 9 nodes, from the disc. From every node to 8 targets (the port's stuck spots and a
  point on each roof), sections 6 and 5: the nearest node, the way and the blocked flag all match a Python model.
- A good table entry stays retail's; the east roof to the west is blocked, the west to the east is not; nothing
  changes with the bit clear.

Not tested in game: the user's next Juggernaut on Roof Tops is the test.

## 23. Request 102 item 3 (2026-11-04 playtest): missile hits with no explosion (bit 30, and the bigger sprite pool)

The user (Hong Kong, 9 opponents): missiles hit with no explosion, picture or sound. The port measured the effect
sprites (`fxSprites`, 150): with 15 opponents on Roof Tops the pool reached 150 once and 120 or more 6 times in 108
counts.

### 23.1 The picture: the pool (CONFIRMED)

- `fxSpriteSpawn` takes the first free of the 150 sprites (`fxSpriteFindFree`) and skips the effect when none is
  free. Retail's 150 serves up to 11 cars.
- **Built with `TM2_PORT_CARS`:** `TM2_MAX_FX_SPRITES` 25 N (`players.h`), 400 for 16 cars (19,200 bytes).
  - `fxSpritesReset`, `fxSpritesUpdate`, `fxSpriteFindFree` and `fxSpritesDraw` loop to it; the port's drop-in code
    (`DROPIN_SPRITES`) too.
  - The array moves to `.port_bss` only when raised (tsv group `fxsprites+`, retail address 80185144). Without
    `TM2_PORT_CARS` it stays retail's.
  - The sprite's id byte (+7, the index) wraps past 255; nothing reads it (LIKELY: the C calls it padding).
  - **For the port:** take the base from `symbols.tsv` (`fxSprites`) and the count from the variant report's
    limits (`fxsprites`); `vplus/interp.c` reads 150 at 80185144 now.

### 23.2 The sound: one cooldown for the whole arena (CONFIRMED)

- `fxExplosion6` .. `fxExplosion192` (and the smoke bursts' sound 8, `fxSmokeBurstPink` / `Red`) play their sound
  only when `fxExplosionCooldown` is 6 or more (it counts up once per step in `fxSpritesUpdate`), then zero it. The
  sound is decided before the sprite spawns, and does not depend on it.
- The cooldown is zeroed even when the sound is then too far to hear (`sndMaxDistance` 5,815). So a far blast
  silences a near one for up to 6 steps; with 16 cars, often.
- **Built: bit 30 `TM2_BAL_BOOM_SOUND` (0x40000000; `TM2_BAL_ALL` now 0x7DFFFFFF):**
  `portBalanceBoomReady(pos, ready)`: retail's test, or an explosion `TM2_BAL_BOOM_APART` (1,000, |dx| + |dy| + |dz|)
  or more from the last one that sounded. One blast's own explosions lie within it, so they still sound once.
- **State:** `portBalanceBoom` (16 bytes, `.port_bss`, every variant): the last sound's position. Match state, hashed;
  cleared by `portBalanceLatch` at each round's start. No `rand()`. It changes which sounds start, and nothing else.
- **Other ways a sound is lost (CONFIRMED; not changed):**
  - Distance: `taskMgrQueueStart` never starts a request farther than `sndMaxDistance`. `fxExplosion96` / `192`
    pass half the distance.
  - Full lists: sound 7's list holds 6, sound 8's holds 4; both are type 13, which `sndListSubmit` drops when the
    list is full.
  - The request pool (`sndPoolAlloc`: 64 with `TM2_PORT_CARS`), the voice limit (44), the start queue (46 a pass;
    past that each new request overwrites the last slot, LIKELY at 16 cars), and the port's `RB_SOUND` mute while it
    re-simulates.
- **Other ways the picture is lost:** `fxSpritesDraw` culls a sprite nearer than 8 or farther than 32,000 (or the
  far-mode limit), off screen, or with the packet buffer full. Views after view 0 skip a sprite's first frame.
- **Both lost, whatever the pool:** `projectileHitTest` spawns no effect or sound at a car hit when the projectile's
  damage is negative (`weaponApplyHit` still runs). Which weapons have negative damage was not traced (LIKELY rare).
- **The car's death sound (25 / 26)** (`carDestroyed`):
  - A human car's is skipped when `roundWonFlag` is set.
  - Both lists hold 1. 26 (modes 0x60, 0x30, 0xA0) is type 13: dropped while an earlier 26 is in its list. 25 is
    type 1: a second death while the first's sound is in the list merges into it without restarting. So two deaths
    close together give one sound.
  - Distance: 26 is heard to about 11,630, 25 to about 17,445.

**Checked:** `tools/check_balance.py` G27: each of the 8 functions asks once; a LinkMips run of
`portBalanceBoomReady`: retail's cooldown with the bit clear; with it, the first sound, near ones only after the
cooldown, at the distance always, the last sound kept, cleared by `portBalanceLatch`. `check_all` proves the pool
build. Not tested in game. To test: 16 cars, missile hits in a crowd, each with its explosion and its sound.

## 24. Request 103 (2026-11-04, the user's decision): Grasshopper's leap pushes its target 300 (bit 31)

### 24.1 Retail's pushes (CONFIRMED in the code)

- **A weapon's push:** `carApplyDamage` of type 1 (a projectile hit) of 11 or more pushes `240 * damage / 25 + 40`,
  less the target's `stats.mass`% (`car_damage.c`), through `carKnockBack` (800E5F24). Spectre's missile (12) gives
  155; 40 damage gives 424.
- **`carKnockBack`:** half again at mass 51 or more; it sets the target's contact push speed and direction and its
  push timer (300 at 40 or more, else 120). `carContactPushUpdate` then moves `motion.pos` along the flat direction
  (x, y of a 4096-long vector) each frame.
- **The leap's hit** (`weaponHomingLeapUpdate`): `carApplyDamage(target, 15 / 18, ..., 2, her slot)`. Type 2 spins
  (`carSpinOut` kind 4) and never pushes. At 18 a type-1 hit would push only 212, so the push needs its own call.
- The spin writes the wheels, spin rate and turn; the push writes only the contact record. Both can be had.

### 24.2 Built: bit 31 `TM2_BAL_LEAP_PUSH_BIT` (0x80000000; `TM2_BAL_ALL` now 0xFDFFFFFF)

- **`portBalanceLeapPush()`**, right after the hit's `carApplyDamage` (its spin unchanged):
  - The target is the leap's target id's car (`carFindById`): human or computer.
  - The push: `TM2_BAL_LEAP_PUSH` (300) less the target's mass%, as a missile's. Mr. Grimm (5) 285, Hammerhead (35)
    195; through `carKnockBack`, so mass 51 or more halves it again (Minion 66: 51), Dark Tooth (100) 0.
  - The direction: from her `motion.pos` to the target's, flat (z 0), normalised by `VectorNormal` as
    `weaponApplyHit` does; straight ahead (+y) if they coincide.
- **Her own movement:** unchanged. Her record is not written; her weight (15) and acceleration (5), the landing (bit
  13) and the touch rule (bit 25, off) are as they were. On a touch hit (bit 25) the push replaces the car-to-car
  push between them.
- **Every Grasshopper:** humans and computer cars fire through the same path; twins (`TM2_PORT_CAR_SPECIALS`) run
  it with their own leap state selected. No car borrows the leap (`SPECIAL_AS` and the templates refuse
  Grasshopper), and Dark Tooth has none.
- **Online:** game state written by the step, the same on every machine. No `rand()` added (retail's spin roll in
  `carApplyDamage` is unchanged). No new state.
- **Tuning:** `TM2_BAL_LEAP_PUSH` (`port_balance.h`, `#ifndef`): build with `--port-define TM2_BAL_LEAP_PUSH=n`.
- **Bit 31 is the sign bit:** read `portBalance` as unsigned (`TM2_BAL_ALL` is 0xFDFFFFFF).

**Checked:** `tools/check_balance.py` G11: the leap calls `portBalanceLeapPush` once, never without the hit. A
LinkMips run (`carFindById` and `carKnockBack` stubbed): the push goes to the target's record, 300 less its mass%,
along her flight, flat, 4096 long, for a computer car and a human; nothing with the bit clear or no car with the id.
Not tested in game. To test: leap into a Mr. Grimm and a Hammerhead; each is thrown about 285 / 195 and still spins.

## 25. Request 121 (2026-11-23): Grasshopper 20, the machine guns' range, Thumper's double flame (bits 7, 9, 14)

The port asked for three Vanilla Plus changes. The word had no free bit (bit 25 is an off option, bits 0..31 named),
so bits 9 and 14, reserved with no effect since request 40's item 10 and round 3, carry the two new rules. Both were
already in `TM2_BAL_ALL` (0xFDFFFFFF, unchanged): Vanilla Plus and online now have them. A build of the port that
set `TM2_BAL_ALL` before gets them with this decomp.

### 25.1 Grasshopper's leap damage 20 (bit 7)

`TM2_BAL_LEAP_DAMAGE` 18 -> 20 (the PC version's). Only the damage: the cone, the homing, the touch (bit 25), the
landing (bit 13) and the push (bit 31) are as they were. Both 18 and 20 are >= 8: `carApplyDamage`'s spin roll is
drawn as before.

### 25.2 The machine guns' range (bit 9, `TM2_BAL_GUN_RANGE`)

- **What the range is:** a bullet's life, in frames of 200 units along its main axis (`bulletSpawn`): 12 for a
  player's (2,400 units), 15 for a computer car's (3,000). `bulletsMove` counts it down; nothing else limits a bullet.
  The guns, Mega Guns (kind 4, power 3) and Minion's rear gun all go through `bulletSpawn`.
- **Built:** `portBalanceGunLife(life)` at the end of `bulletSpawn` (before the port's bullet-trail hook):
  `life x 125 / 100`, rounded: 12 -> 15 (3,000 units, exactly 25 %), 15 -> 19 (3,800, the nearest to 3,750).
  Every car, players and computer cars alike. Bit clear: the life as retail.
- **Other readers:** none of the range. The computer cars decide to fire with their own distance test
  (`aiWeaponFireDecide`: target within 4,000 and about 12 degrees, and the weapons tick only within 4,320): it does
  not read the bullet life and is unchanged (retail already fires at targets its bullets cannot reach). No HUD or
  targeting reads it. The port's bullet trails (`portVisualBulletSpawn`) copy the life after the gate.
- **The bullet pool:** 40 records, unchanged. A longer life keeps a bullet one record-life longer: a player firing
  keeps 5 (4 before), a computer car 4 (3), Minion's front and rear gun 8 (6). When the pool is full a shot is lost
  (the flash and sound still play), as in retail; with 16 cars all firing at once retail can already run out.
- **`rand()`:** none in `bulletSpawn`. Longer bullets hit more often, and each hit draws `bulletsMove`'s own
  (the smoke; the coin flip unless bit 20): only with the bit, the same on every machine.

### 25.3 Thumper's double flame (bit 14, `TM2_BAL_THUMPER_LOCK`)

- **What happens (CONFIRMED in the C):** Thumper's special (`weaponFireSpecial` case 0x70) starts a flame
  (`weaponFlameStart`): a new record of the flame pool, its car object as owner, 50 frames. Nothing compares owners:
  a second press while the first flame burns takes a second record. Both follow his car, so both streams cover the
  same space and each hits the first car in it, once per frame (`weaponFlameUpdate`: strength 1): two flames, twice
  the damage. A player's cooldown is 20 frames against the flame's 50, so any second press 1 to 2.5 s after the first
  doubles it. Retail's pool of 2 shared by every Thumper kept it to two; the port's raised pool (`TM2_PORT_CARS`)
  lets one Thumper run three. A computer Thumper at close range (`weaponFireSelected`'s melee path) refires every
  11-12 frames. Burning does not stack (a burning car is not lit again). Other specials guard this themselves
  (Twister refunds a press while her tornado runs; a second Shadow press detonates); Thumper has nothing.
- **Built:** `portBalanceFlameLocked(car, human)` in `weaponFireSelected`, for the special only (kind 0), right after
  bit 15's lock: 1 when the car's special is Thumper's (`TM2_SPECIAL_TYPE`: a borrower of it too) and a flame record
  whose timer runs has his car object as owner. Refused as bit 15's lock is: the denied sound for a player, no
  cooldown, no charge spent. His pickups and machine guns as retail. Twins and borrowers are told apart by their car
  objects. The press is free again the frame his flame ends.
- **Not chosen:** "a second press waits" would need a pending press per car in the rollback state and a decision on
  when and where it fires; refusing matches bits 15 and 26 and adds no state.
- **Left as retail:** a Thumper's press when the pool is full (another Thumper's flames) still spends the special
  and fires nothing.
- **`rand()`:** the gate draws none; fewer flames means fewer of `weaponFlameUpdate`'s own draws, only with the bit.

**Checked:** `tools/check_balance.py` G28 (`bulletSpawn` calls `portBalanceGunLife` once; a LinkMips run: 12 -> 15,
15 -> 19 with bit 9, unchanged without it) and G29 (`weaponFireSelected`: the refused edge of
`portBalanceFlameLocked` cannot reach `weaponFireSpecial`; a LinkMips run on made-up human and computer records:
refused only for a Thumper with bit 14 and a running flame of his own). G12 and G15 still prove the spiral and the
leap are as they were. Not tested in game. To test: (1) bit 9: a player's bullets reach 3,000 units, a computer car's
3,800 (a target just inside: hit; just outside: not); (2) bit 14: Thumper with 2 specials, two presses 1 s apart:
the second is denied (sound), the special count stays, the target takes one flame's damage; a press after the first
flame ends fires; a computer Thumper at close range fires one flame at a time; (3) bit 7: a leap hit takes 20.

## 26. Request 124 (2026-11-25, Zen): the ricochet bomb trapped by a pickup next to a wall (`portBalance2` bit 0)

### 26.1 The second word

`portBalance`'s 32 bits are all named (bit 25 is an off option), so request 124 starts a second word with the same
rules as the first:

| Name | Written by | Meaning |
|---|---|---|
| `portBalance2` | the port | the switches (`TM2_BAL2_*`), 0 = retail; set it with `portBalance` and put it in the ruleset hash next to it |
| `portBalanceLive2` | `portBalanceLatch` (roundStart) | this round's copy, read by the gates; the port reads it, never writes it |

Both are `.port_bss` in every variant, after `portCarTeam` (`config/US/port_variant_arrays.tsv`). Read their
addresses from the link's `symbols.tsv`. `TM2_BAL2_ALL` (`0xB` since request 127) is the word for Vanilla Plus
and online.

| bit | mask | item | name | rule |
|---|---|---|---|---|
| 0 | 0x0001 | 124 | `TM2_BAL2_PICKUP_PASS` | every shot passes a pickup's hit box, ids 401..480: both of the ricochet bomb's hit checks, the missiles' and the bullets' (26.3) |
| 1 | 0x0002 | 125 | `TM2_BAL2_SWOOP_PASS` | Dark Tooth's swoop passes a pickup's hit box, ids 401..480: the boss's and a player's Dark Tooth's (26.4) |
| 2 | 0x0004 | 127 | `TM2_BAL2_MINE_AGE` | mode A: a mine is removed (silently) `portBalanceMineSeconds` after it was laid (27); not in `TM2_BAL2_ALL` |
| 3 | 0x0008 | 127 | `TM2_BAL2_MINE_ORPHAN` | mode B, the default: a mine is removed `portBalanceMineSeconds` after its owner's car was destroyed (27) |

### 26.2 What happens in retail (CONFIRMED in the C; the same on the PS1)

`weaponRicochetUpdate` (8010CC54, `weapon_ricochet.c`) moves each bomb, then asks `hdpCollideObject` for one hit:

1. **The first check** drops a hit of kind 667-668 or 401-440 (the pickups): the bomb rolls through a pickup.
2. **On a hit** (a wall): the heading is reflected, the bounce counted, and the bomb pushed out of the wall by the
   hit's depth + 20 along its normal.
3. **The push-out check** asks `hdpCollideObject` again and drops only 667-668. If something is still hit there, the
   bomb is moved back by this tick's velocity and turned around (`rot[2] += 2048`).

Step 3 is meant for a corner: pushed out of one wall into another, the bomb goes back the way it came. But a pickup's
hit box counts there. With a pickup within about 20 units plus the bomb's size of a wall, each bounce pushes the bomb
into the pickup, step 3 turns it back into the wall, and the next tick bounces it again: back and forth until its
life (400 ticks, 20 s) runs out. It never gets past the pickup again. It is retail's code, unchanged by the port, so
the PS1 does the same wherever a pickup sits that close to a wall. The bomb's power only doubles on the first
bounce, so nothing else grows while it is trapped.

**The other shots** test a hit once, with no push-out (no second check):

| Shot | Function | Passes |
|---|---|---|
| ricochet bomb, first check | `weaponRicochetUpdate` | 667-668, 401-440 |
| ricochet bomb, push-out check | `weaponRicochetUpdate` | 667-668 only |
| missiles and the other projectiles | `weaponProjectileUpdate` (800DB110, `projectile_hit.c`) | 2, 667-668, 3301, 401-440, 3500-3509 |
| machine-gun bullets | `bulletsMove` (800D7E30, `weapon_bullets.c`) | the owner, 667-668, 3301, 401-440, 3500-3509 |

So the ricochet bomb's push-out is the only check that differs from its own first check.

**441..480** (`PICKUP_LOOKS.md` 1) are the +40 "keep" pickups, only on Los Angeles (441, 444, 453, 459) and Moscow
(443, 444, 446, 451, 457, 459): the rotating bonus spots under an id-440 animation node. Every shot stops at 440, so
in retail these hit boxes are walls to every shot: a bomb bounces off them, and a missile or bullet hitting one ends
there. On Moscow's first carrier the hit box is beside the 44x node and reads 440, which every first check already
passes.

**Also seen, not changed:** Dark Tooth's swoop (`aiDarkToothBossUpdate`, 8010FD44, `ai_dark_tooth_boss.c`)
ends at the first hit of `hdpQueryPoint` with no filter at all, so a pickup's box stops it like a wall does. It is
not a shot's check, so this bit leaves it as retail. Request 125 passes pickups there too, with bit 1 (26.4).

### 26.3 Built: `portBalancePickupPass(kind)` (bit 0, `TM2_BAL2_PICKUP_PASS`)

- **The gate:** `portBalancePickupPass(kind)` is 1 when bit 0 of `portBalanceLive2` is set and `kind` is 401..480,
  else 0. Each caller clears its hit when it is 1, the same way as for its own filter. It reads only and calls no
  `rand()`.
- **Called:**
  - `weaponRicochetUpdate`, after **both** checks (after bit 18's `portBalanceRicochetPass`). The push-out check now
    passes 401..480, so a pickup beside a wall no longer turns the bomb back: it bounces once off the wall and rolls
    away through the pickup, as it rolled in. The first check also passes 441..480.
  - `weaponProjectileUpdate`, after its filter: missiles pass 441..480 too.
  - `bulletsMove`, after its filter (the kind cleared as the filter does): bullets pass 441..480 too.
- **What stays:** a real corner (a wall next to a wall, or a car's box) still turns the bomb back, as retail; only a
  pickup's box stops counting.
- **Bit clear:** retail's filters alone, every shot as before. Online: no new state (the word is latched like
  `portBalance`), the same on every machine; it changes play, so it belongs with the online rules version.
- **`rand()`:** the gate draws none. A bomb that gets away may later hit something it never reached in retail, and
  that hit draws what any hit draws: only with the bit, the same on every machine.

**Checked:** `tools/check_balance.py` G30:
- `weaponRicochetUpdate` calls `portBalancePickupPass` twice, each after an `hdpCollideObject`;
  `weaponProjectileUpdate` and `bulletsMove` once each; `portBalanceLatch` copies the word;
- a LinkMips run of `portBalancePickupPass`: 1 for 401..480 with the bit only (400, 481, 500, 667, 0 and -1 never;
  every other bit of both words set or clear);
- a LinkMips run of `weaponRicochetUpdate` itself on a made-up line, `hdpCollideObject` and the movement stubbed: a
  wall at x >= 1000 and a box at 960..999 beside it, the bomb from 800 at 100 a tick. With the bit clear and a pickup
  box (404) the bomb is trapped at the wall for 30 ticks (Zen's), with the bit it bounces once and rolls away. A 459
  box does the same. A box that is not a pickup traps it with the bit too (a corner).

Not tested in game. To test: Hong Kong, a ricochet bomb rolled through the Power pickup by the wall in Zen's video.
With the bit it bounces off the wall once and comes back through the pickup. On Los Angeles or Moscow, missiles and
bullets fly through a bonus spot.

### 26.4 Request 125: Dark Tooth's swoop passes pickups (bit 1, `TM2_BAL2_SWOOP_PASS`)

**Asked:** the swoop (his head) should pass pickups as every shot does since 26.3. Walls, cars and everything else
still end it. Also asked: any other attack or special that stops at a pickup's box, treated the same way.

**What stops at a pickup (read in the C):** the game tests a point or an object against the world's hit volumes
through two functions only: `hdpCollideObject` and `hdpQueryPoint`. Their callers:

| caller | what | pickups |
|---|---|---|
| `weaponRicochetUpdate` | the ricochet bomb, two checks | passed with bit 0 (26.3) |
| `weaponProjectileUpdate` | missiles, Minion's volley, the specials' shots | passed with bit 0 |
| `bulletsMove` | the guns | passed with bit 0 |
| `aiDarkToothBossUpdate` | the boss's swoop | **no filter: stopped. Passed with bit 1** |
| `portCarDtPlayerSwoopStep` | a player's Dark Tooth's swoop (request 36, `TM2_PORT_DT_PLAYER`) | **the same as the boss: passed with bit 1** |
| `carsPhysicsUpdate` (`car_physics.c`) | the cars' ground and walls | not an attack; a car drives through a pickup to take it |

Every other attack (the flame, the freeze, the strike, the mines' and bombs' contact) uses
`projectileHitTest`, which looks only at cars, breakables and spinners and never at pickups. So the swoop was the
last attack that stopped at a pickup.

**Built:** `portBalanceSwoopHit(res)`, `res` being `hdpQueryPoint`'s result (+0 the hit word, +0x0A the id of the
hit volume). It returns the hit word as retail reads it, or 0 when bit 1 of `portBalanceLive2` is set and the id is
401..480. Both swoops call it in place of retail's `*hdpQueryPoint(...)` (`SWOOP_WORLD` / `DT_SWOOP_WORLD`, the
retail form without `TM2_PORT_BALANCE`). It reads only and calls no `rand()`. A swoop that now goes on can still end
at the car, at 40 updates or out of range, as before.

- **Bit clear:** retail, the swoop ends at a pickup's box. Online: no new state, the same on every machine; it
  changes play (the online rules, `BALANCE_CHANGES.md`).
- **One limit, as for the shots:** `hdpQueryPoint` reports one volume. If the head is inside a pickup's box and a
  wall's at the same update and the pickup's is the one reported, the wall ends the swoop one update later.

**Checked:** `tools/check_balance.py` G31: `aiDarkToothBossUpdate` calls `hdpQueryPoint` once and
`portBalanceSwoopHit` once, after it, and a player's Dark Tooth's swoop the same when it is linked; a LinkMips run
of `portBalanceSwoopHit` on a made-up result: the hit word for every kind with the bit clear, 0 for 401..480 with
it, and the hit word for 400, 481, 500, 667, 3 and -1. `aiDarkToothBossUpdate` joins the functions
`TM2_PORT_BALANCE` changes (check 4).

**Also fixed:** a link with `TM2_PORT_BALANCE` and no other port define did not compile. Two helpers used names
that only other defines declared: `portBalanceLeapPush` used the drop-in's `LEAP_*` names and `portBalanceFlameLocked`
used `weaponFlameEmitters`. The balance block now has its own names for both (the same code with every define).
The full run links this build too (`bal`). That link showed one more gap: check 4's list of the functions
`TM2_PORT_BALANCE` changes lacked `carStatsLoad` (request 77's bit 24, 17), which only a build with no other
port define can show. It is in the list now.

Not tested in game. To test: the Dark Tooth fight on a map with a pickup between him and the car (or a player's
Dark Tooth swooping at a car across a pickup). With the bit the head goes through the pickup and reaches the car.

## 27. Request 127 (2026-11-25, Ryupichu, Tegu, SuperflyKev): mines don't stay forever (`portBalance2` bits 2, 3)

### 27.1 Why they stay

**Retail (CONFIRMED, 3.3):** a mine is not permanent. `weaponDropsUpdate` counts its `status` down from 300 and frees
it at 0. That is 300 calls, one per view each step: 300 steps on one screen, 150 split. What keeps mines all match is
the port's own rule, bit 2 of `portBalance` (request 40, 3.3): a mine's count stops at 1, so it never expires, with 5
per driver. With the Original preset (bit 2 clear) mines already go after retail's 300.

The user (2026-11-25): no "stay" setting, so every preset, the Original included, sets one of bits 2 and 3
below. With `portBalance` bit 2 clear, retail's 300 calls also remove a mine, whichever comes first.

### 27.2 Built: the mines' clocks (bits 2 and 3)

| bit | mode | a mine is removed |
|---|---|---|
| 2 `TM2_BAL2_MINE_AGE` | A | `portBalanceMineSeconds` after it was laid |
| 3 `TM2_BAL2_MINE_ORPHAN` | B, the default (in `TM2_BAL2_ALL`) | `portBalanceMineSeconds` after its owner's car was first seen destroyed, whatever its age. Once started its clock keeps running: the owner coming back does not save the mine |
| both | | whichever comes first |

- **"Removed":** silently, as bit 2's sixth mine and the drop-in's leaving car already remove one. The record and
  its object are freed: status, kind and held 0, the object's word +8 0 and the pointer cleared. No explosion, no
  damage, no sound. The user's call: no puff needed.
- **The seconds:** `portBalanceMineSeconds` (`.port_bss`, set by the port, 0 = 15, above 3600 counts as 3600). It
  is latched at `roundStart` (`portBalanceLatch`) into `portBalanceMineTicks`, the limit in 60ths of a second.
- **Counted in game steps:** each step adds the step's ticks (`timerGetFrameTicks`, 60ths of a second, 3 to 5; with
  `FIXED_TICKS` the port's fixed value). So the time does not depend on the frame rate, and every machine counts the
  same. `portBalanceMineClock` runs at the top of `weaponDropsUpdate`, at most once per step (`timerFrameCount`
  noted in `portBalanceMineStep`). In retail `weaponDropsUpdate` runs once per view, so a split screen would
  otherwise count twice.
- **Every mine:** only one path lays a mine, the energy attack 101 (`weaponFireEnergyAttack` -> `weaponDropItem` ->
  `weaponDropWrite`), for players and computer cars alike. No special and no port code lays one. Each record a mine
  is written to starts its clocks at 0 (`portBalanceMineSlot`, with bit 2 of `portBalance` set or clear). Remote
  bombs (kind 19) are never touched.
- **The owner:** the mine's +4, the dropper's object id. `portBalanceCarDown(id)` is 1 when that car is dead or at
  HP 0, or when no car has the id any more (it left the match).
- **State:** `portBalanceMineAge` and `portBalanceMineOrphan` (per drop record, `TM2_MAX_DROPS`), plus
  `portBalanceMineStep` and `portBalanceMineTicks`. All are `.port_bss`, so they are in the snapshot and the hash.
  No `rand()`.
- **Online:** the same on every machine; it changes play (`BALANCE_CHANGES.md`). Put `portBalanceMineSeconds` in the
  ruleset hash next to `portBalance2`.

**Checked:** `tools/check_balance.py` G32:
- `weaponDropsUpdate` calls `portBalanceMineClock` once, before its loop, and both of `portBalanceMineSlot`'s paths
  start the clocks.
- A LinkMips run of `portBalanceLatch` gives 15 s for 0 or less, the seconds set, and 3600 s at most.
- A LinkMips run of `portBalanceMineClock` on two made-up mines and a remote bomb, at 3 ticks a step:
  - bit 2 removes both mines at the 300th step, not before;
  - bit 3 removes the mine of an owner destroyed at step 50 at step 349, even with the owner back at step 100, and
    never the mine of a live owner;
  - a second call in the same step counts nothing;
  - with neither bit, nothing is removed;
  - a removed record and its object are freed, and the bomb is never touched.

Not tested in game. To test: a battle with mines (Vanilla Plus): a computer car lays mines and is destroyed, and 15
seconds later its mines are gone; a live driver's mines stay. With mode A every mine goes 15 seconds after it was
laid.
