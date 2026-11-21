# Car stats, special weapons and Shadow's detonation

For the PC port's Balancing tab (`docs/DECOMP_REQUESTS.md` section 3: car
stat and special-weapon tables, Shadow's special detonation). The stats
code is decompiled in [`src/semantic/car_stats.c`](../src/semantic/car_stats.c)
(byte-identical); its `TM2CarStats` struct uses the field names below.

Verified independently: the car id to name mapping (id list `80162F28`,
name pointers `80162830`), every value in the per-car table against the
disassembly (Hex-Rays merges some paired 16-bit stores into one 32-bit
write; the table uses the real values), the AI chance blend in
`ai_steps.c` (the old names there were reversed and are fixed), and the
Shadow detonation constants (`li a1,0x32` vs `0x19` at `8010B7B4` /
`8010B9D0`). Tags: CONFIRMED (seen in code and understood), LIKELY, GUESS.

Note on "slope": the extra splash damage applies when the target car's
`on_surface` flag is set, meaning at least one wheel is over one of the
level's surface polygons (`D_801B0418` (worldSurfaces): raised plates and ramps, see
`src/semantic/car_wheels.c`), not when the ground is merely steep.

Retail SCUS_94306R.EXE. Stats block = human record +0x14C / AI record +0x4FC (offsets below are into the
stats block, decimal, as in Hex-Rays). "CONFIRMED" = read site seen and its use is clear from code;
"LIKELY" = read site seen, meaning inferred from how it is used; "GUESS" = no clear reader found.

Method: the per-car setters (800FE62C..800FF4BC), the derivation in 800FE15C / 800FE478 / 800FF878, a
register-tracking scan of the whole disassembly for loads through a register set to `record+332` /
`record+1276` (gives the reader lists below), and the existing semantic C (car_motion.c,
car_contacts.c, update_guns.c, weapon_launch.c, weapon_basic.c, ai_steps.c, pickup_apply.c,
area_damage.c, strike_update.c).

## 1. Car type ids -> names (CONFIRMED)

- `dword_80162F28[15] = {0x10,0x20,...,0xF0}`; `sub_8011E1C4(type)` (carTypeToIndex) returns its index (0..14).
- `sub_800FB878(type) = off_80162830[sub_8011E1C4(type)]` (carTypeToIndex) (carTypeName) (the upper-case HUD name table at 0x80162830).

| id | index | name (0x80162830 table) | init fn | notes |
|---|---|---|---|---|
| 0x10 | 0 | SWEETTOOTH | 800FE62C | |
| 0x20 | 1 | HAMMERHEAD | 800FE73C | |
| 0x30 | 2 | OUTLAW 2 | 800FE858 | |
| 0x40 | 3 | WARTHOG | 800FE960 | |
| 0x50 | 4 | MR GRIMM | 800FEA68 | motorcycle (negative body lean, 3x rollover threshold) |
| 0x60 | 5 | GRASSHOPPER | 800FEB8C | |
| 0x70 | 6 | THUMPER | 800FEC8C | |
| 0x80 | 7 | SPECTRE | 800FED98 | |
| 0x90 | 8 | ROAD KILL | 800FEE98 | no own case: falls into the default init in 800FE15C |
| 0xA0 | 9 | TWISTER | 800FEF98 | |
| 0xB0 | 10 | AXEL | 800FF090 | |
| 0xC0 | 11 | MR SLAM | 800FF19C | also sets its two arm-segment rest angles: sub_8011526C (slamClawSetAngle)(739), sub_80115230 (slamArmSetAngle)(-113) |
| 0xD0 | 12 | SHADOW | 800FF2B0 | |
| 0xE0 | 13 | MINION | 800FF3B8 | boss; UpdateGuns also fires a rear gun for 0xE0 |
| 0xF0 | 14 | DARK TOOTH | 800FF4BC | boss |

This fits other code: Mr. Grimm has the lowest HP (65), Warthog the highest of the normal cars (130), and
Minion and Dark Tooth have 210 and 300. Dark Tooth (0xF0) is special-cased in damage (800E5628).
800FE4F0(type) returns the default top speed (+82) per id, and car_tick uses it to restore +82.

## 2. Stats block fields

Derived fields (800FE15C / 800FE478 / 800FF878; `fps` = sub_8015531C (timerGetFrameTicks)/sub_8015532C (timerGetFrameRate), `mult` = +118):
- +74 = |+78| + |+80|; +84 = 3/4·+82; +90 = 2/3·+82; +140 = +58·+142/100; +132 = +124; +20 = 60
- +96 = +94; +98 = +94·+108·mult/100; +100 = +96·+104 (·10 for AI); +102 = +100/2; +112 = 5·+98; +114 = 2·+98; +116 = 682·+106/100
- +176 = +172; +180 = +172·5·mult/100 (/2 for AI); +188 = 10·+180; +204 = +172·+92
- +184/+200/+192/+196/+208 = (+82/+84/+86/+88/+90)·19·fps/100·32·mult/100  (speed units ×32)
- +118 = +120 = speed multiplier %: humans 100 (800FFECC(rec+332,100)); AI = sub_8011D94C (aiDifficultySpeedScale)() = 75 / 120 / 140 by difficulty

### Field table

| off | type | meaning | readers / evidence | conf. |
|---|---|---|---|---|
| +2 | u8 | **Can mount/drive onto other cars on a frontal hit** (Hammerhead, Axel, Minion, Dark Tooth = 1). Hammerhead uses up one special charge per mount. The victim spins out unless the attacker is Dark Tooth | 800E184C car-car collision → sub_800E6D14 (carCrashStart) (sets +48/+50 "riding on" length/height = victim |+78|/|+80| + half its +74, victim +66 + +70 + 24) | CONFIRMED |
| +14 | u8 | unknown (values 0..3) | no reader found | GUESS |
| +20 | s16 | frame rate the speed fields were last scaled for (60) | 800E7EDC rescales velocities when fps changes | CONFIRMED (internal, do not expose) |
| +36 | s16 | **AI chase patience**: how many frames the target can keep pulling away before the AI re-plans its route to it | ai_steps.c `ai->away > ai->f520` (800E8F38, 800E97B0, 800EA270 path) | LIKELY |
| +42 | s16 | **AI turning range**: distance from the next node at which the AI turns toward the following node. Scaled ×½..×2 by turn angle, min 160, then ×2/3 / ×6/5 by difficulty | 800F6F6C → AI +0x484 → 800EB544 (f486), used vs goal_dist | LIKELY |
| +52 | s16 | body pitch travel (cosmetic nose-dive/squat; pitch clamp = atan2(+52, +74)); default 8 | 800E5134, 800F15D0 | LIKELY |
| +58 | s16 | **max HP / armour**. A health pickup (401+16) sets hp(+164) = +58; 401+17 adds +58/3 | 8010AFC8, 800FD0D0 (HUD) | CONFIRMED |
| +60 | s16 | shield active: damage halved | 800E5628 (`v15[30]`) | CONFIRMED (runtime) |
| +62 | s16 | **rollover threshold**: left/right wheel height difference that tips the car over at speed > +208. Equals +72, except 2/3·+72 for Sweet Tooth and 3·+72 for Mr. Grimm | 800F0944, 800EF5D0 | CONFIRMED |
| +66 | s16 | **ride height** (car origin above ground when upright). Contact points use z = 24 − +66; the AI snaps z = node z + +66; projectile +0x46 = +66 − 24 | 800DF594, 800FDFC4, 800E6D14, 801052BC, ai_steps 1742 | CONFIRMED |
| +68 | s16 | **weapon launch height** (muzzle z = car z + +68 + 8) for pickups and center-mounted specials | 801052BC, 80105A7C (0x40/0x50/0x80/0x90/0xE0), 801046FC | CONFIRMED |
| +70 | s16 | roof height (origin height when upside down; top contact point) | 800DF594, 800FDFC4, 800E6D14 | CONFIRMED |
| +72 | s16 | **track width** (left↔right wheel spacing): side contact points ±+72/2; roll angle from the L/R wheel height difference | 800F0944, 800FDFC4, 800DF594 | CONFIRMED |
| +74 | s16 | wheelbase (derived) | 800F0944 pitch, 800E5134 | CONFIRMED |
| +76 | s16 | **turning speed**: yaw rate = steer·speed/(+76·32) below speed +76, full steer above it. Lower = tighter low-speed turning | 800DD304 (`v5[38]`) | CONFIRMED |
| +78 / +80 | s16 | front / rear axle (contact) Y offsets | 800FDFC4, 800EF5D0, 800E6D14, 800F4058 | CONFIRMED |
| +82 | s16 | **top speed** (base, "mph"). Set through 800FE478, which also derives +84 reverse (3/4) and +90 (2/3). AI electrified/frozen: 800FE478(stats,25); restored from 800FE4F0 | +184 used in 800DD304 (throttle cap), 800DFB28, 800E0C28, … | CONFIRMED |
| +84 | s16 | max reverse speed (3/4 top) → +200 | 800DD304 | CONFIRMED |
| +86 | s16 | **slide/drift speed**: above +192 the car loses grip (body drifts, grip slowdown applies) | 800DFB28, 800DFDFC (car_motion.c `slide_speed`) | CONFIRMED |
| +88 | s16 | AI current cruise/target speed (→ +196), set by 800F6F6C from +82 | 800F6F6C, ai_driver.c | CONFIRMED (AI runtime) |
| +90 | s16 | 2/3 top speed → +208: minimum speed for rollovers | 800F0944 | CONFIRMED |
| +92 | s16 | **brake strength**: braking decel +204 = +172·+92 (also the coast-drag basis) | 800DD304, 800DFB28, 800E0208 | CONFIRMED |
| +94 | s16 | steering rate at high speed (all cars 2) → +96 | 800DD304 | CONFIRMED |
| +104 | s16 | max steering-lock factor (25, set in 800FE15C) → +100 | 800DD304 | CONFIRMED |
| +106 | s16 | **body lean in corners** (cosmetic roll, % of 60°). Negative for Mr. Grimm, so the bike leans into the turn | 800E5134 (`+116`) | CONFIRMED |
| +108 | s16 | **low-speed steering rate** multiplier → +98 (steer rate blends +98→+96 as speed rises to 1520) | 800DD304, 800E5FFC | CONFIRMED |
| +110 | s16 | steering-lock multiplier with the handbrake/turbo-turn buttons (default 3; 2 for Hammerhead, Warthog, Mr. Slam) | 800DD304 (`v5[55]`), 800DFB28 (`grip_scale`) | CONFIRMED |
| +118/+120 | s16 | speed multiplier % (see above) | 800FF878 | CONFIRMED |
| +122 | s16 | **mass / weight %**. Car-car impulse split = +122/(+122 + other's +122). Knockback from hits ×(1 − +122/100). Spin-outs ×(100 − +122)%. A lighter car that is hit hard by a heavier one spins out. A **human** attacker with +122 == 100 (Dark Tooth) does **double damage** | 800E184C, 800E2DD0, 800E5628, 800E5FFC, 800E4754, 800E5F24, ai_steps (avoid ramming heavier cars) | CONFIRMED |
| +124/+126 | s16 | AI hunt chance at full / zero health. Formula: `hunt = hp%·(+124) + (100−hp%)·(+126)`. (ai_steps.c now calls these hunt_full / hunt_low; the old names were reversed) | 800E8A84 | CONFIRMED |
| +128/+130 | s16 | AI attack chance at full / zero health (same blend); remainder = flee | 800E8A84 | CONFIRMED |
| +134/+136/+138 | s16 | AI hunt / attack / flee state durations (frames) | 800E8A84 | CONFIRMED |
| +140 | s16 | AI **retaliation budget** = +58·+142/100. Damage the AI takes is subtracted; when it drops below 0, the AI is forced into "attack" | 800E5628 (decrement), 800E8A84 (`f588 < 0`) | CONFIRMED |
| +142 | s16 | % of max HP for +140 | 800FE15C | CONFIRMED |
| +144 | s16 | AI chance (%) to try passing/overtaking | ai_steps.c `rand()%100 < pass_chance` | CONFIRMED |
| +164 | s32 | current HP (runtime) | 800E5628, 8010AFC8, HUD | CONFIRMED |
| +168 | s32 | damageable flag (set to 1 in 800FE15C when byte 0 is 0) | 800E5628 | LIKELY |
| +172 | s32 | **acceleration**. +176 (top-speed accel) and +180 = 5× (launch accel). The accel tapers linearly from +180 at rest to +176 at top speed. Turbo gives 4× accel and 4/3 top speed (capped 140..180). Also scales slope gravity | 800DD304 | CONFIRMED |
| +216/+218/+220 | s16 | **machine-gun muzzle** (x, forward y, z), mirrored left/right on alternate shots; also side mounts for Warthog/Minion specials | UpdateGuns 80104F70, 80105A7C, 801046FC | CONFIRMED |
| +224 | s16 | x of center special mount (always 0) | 80105A7C, 801046FC | CONFIRMED |
| +226/+228 | s16 | y/z of a second mount (unused as far as found) | no reader found | GUESS |

### Per-car values (from the init functions; '-' = not written, default from 800FE15C)

| field | 10 Sweet Tooth | 20 Hammerhead | 30 Outlaw 2 | 40 Warthog | 50 Mr. Grimm | 60 Grasshopper | 70 Thumper | 80 Spectre | 90 Roadkill | A0 Twister | B0 Axel | C0 Mr. Slam | D0 Shadow | E0 Minion | F0 Dark Tooth |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| +2 mount cars | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 1 | 1 |
| +14 ? | 0 | 0 | 2 | 0 | 1 | 3 | 2 | 1 | 0 | 1 | 3 | 2 | 0 | 0 | 0 |
| +36 AI chase patience | 90 | 90 | 40 | 60 | 75 | 60 | 110 | 15 | 25 | 12 | 25 | 180 | 40 | 40 | 25 |
| +42 AI turn range | 600 | 520 | 600 | 440 | 640 | 480 | 560 | 560 | 600 | 640 | 480 | 440 | 560 | 600 | 880 |
| +52 pitch travel | 16 | 24 | 8 | 16 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 16 | 16 | 16 | 24 |
| +58 max HP | 120 | 110 | 100 | 130 | 65 | 90 | 100 | 90 | 100 | 75 | 110 | 120 | 100 | 210 | 300 |
| +62 rollover thr. | 44 | 86 | 52 | 56 | 159 | 73 | 52 | 52 | 52 | 72 | 116 | 74 | 50 | 108 | 173 |
| +66 ride height | 32 | 66 | 16 | 26 | 17 | 17 | 14 | 16 | 16 | 12 | 15 | 32 | 20 | 27 | 116 |
| +68 launch height | 44 | 33 | 17 | 31 | 13 | 16 | 18 | 17 | 19 | 12 | 0 | 55 | 27 | 66 | 56 |
| +70 roof height | 44 | 33 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 60 | 75 | 32 | 66 | 106 |
| +72 track width | 66 | 86 | 52 | 56 | 53 | 73 | 52 | 52 | 52 | 72 | 116 | 74 | 50 | 108 | 173 |
| +76 turning speed | 80 | 100 | 75 | 90 | 65 | 65 | 85 | 70 | 80 | 70 | 100 | 90 | 105 | 75 | 80 |
| +78 front axle | 56 | 64 | 58 | 64 | 50 | 41 | 59 | 48 | 52 | 56 | 56 | 61 | 60 | 126 | 216 |
| +80 rear axle | -33 | -76 | -62 | -61 | -41 | -54 | -75 | -62 | -64 | -70 | -56 | -79 | -105 | -126 | -269 |
| +82 top speed | 92 | 96 | 127 | 101 | 125 | 121 | 111 | 131 | 119 | 141 | 103 | 92 | 113 | 127 | 127 |
| +86 slide speed | 70 | 25 | 103 | 110 | 110 | 90 | 85 | 110 | 75 | 140 | 75 | 80 | 75 | 150 | 75 |
| +92 brake mult | 2 | 1 | 3 | 2 | 3 | 3 | 4 | 2 | 3 | 2 | 3 | 3 | 3 | 3 | 3 |
| +94 hi-speed steer | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 |
| +106 body lean | 75 | 40 | 20 | 15 | -70 | 40 | 60 | 15 | 25 | 20 | 55 | 40 | 35 | 20 | 75 |
| +108 lo-speed steer | 3 | 4 | 6 | 3 | 5 | 5 | 8 | 3 | 7 | 5 | 6 | 7 | 7 | 6 | 7 |
| +110 handbrake lock | 3 | 2 | 3 | 2 | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 2 | 3 | 3 | 3 |
| +122 mass | 30 | 35 | 25 | 30 | 5 | 15 | 25 | 13 | 28 | 14 | 30 | 35 | 25 | 60 | 100 |
| +124 hunt @full hp | 75 | 100 | 20 | 60 | 90 | 50 | 50 | 30 | 75 | 50 | 65 | 100 | 50 | 100 | 100 |
| +126 hunt @0 hp | 75 | 0 | 0 | 0 | 60 | 50 | 10 | 30 | 30 | 50 | 35 | 100 | 50 | 100 | 100 |
| +128 attack @full | 25 | 0 | 80 | 40 | 10 | 50 | 20 | 60 | 15 | 50 | 65 | 0 | 45 | 0 | 0 |
| +130 attack @0 | 25 | 10 | 100 | 100 | 30 | 50 | 40 | 60 | 70 | 50 | 35 | 0 | 45 | 0 | 0 |
| +134 hunt time | 600 | 3600 | 900 | 300 | 300 | 600 | 600 | 900 | 600 | 600 | 600 | 600 | 600 | 600 | 600 |
| +136 attack time | 600 | 300 | 900 | 300 | 1800 | 600 | 1200 | 900 | 600 | 600 | 600 | 600 | 600 | 600 | 600 |
| +138 flee time | 600 | 600 | 300 | 300 | 300 | 600 | 600 | 300 | 600 | 600 | 600 | 600 | 600 | 600 | 600 |
| +142 retaliate % | 100 | 80 | 20 | 100 | 7 | 80 | 80 | 20 | 80 | 20 | 50 | 100 | 40 | 90 | 100 |
| +144 pass chance | 80 | 10 | 70 | 30 | 80 | 80 | 70 | 70 | 50 | 90 | 10 | 10 | 80 | 10 | 0 |
| +172 accel | 5 | 8 | 6 | 6 | 11 | 5 | 5 | 10 | 6 | 9 | 6 | 4 | 6 | 6 | 6 |
| +216 gun x | -36 | -24 | -28 | -25 | -13 | -21 | -27 | -24 | -24 | -9 | -25 | -23 | -28 | -18 | -87 |
| +218 gun y | 75 | 55 | 42 | 68 | 18 | 9 | 60 | 52 | 40 | 37 | 46 | 53 | 66 | 105 | 90 |
| +220 gun z | 9 | -12 | 6 | 11 | 9 | 19 | 8 | 8 | 9 | 8 | 0 | 29 | 8 | 27 | 0 |
| +226 | 32 | 12 | -9 | 10 | -17 | -8 | 3 | -14 | -14 | 6 | -14 | 4 | 4 | 0 | 0 |
| +228 | 44 | 44 | 30 | 33 | 35 | 44 | 30 | 30 | 31 | 32 | 66 | 62 | 36 | 50 | 200 |

Notes: +124..+130 on Outlaw 2, Warthog, Mr. Grimm, Grasshopper, Thumper, Spectre and Roadkill come from
paired stores that Hex-Rays shows as one 32-bit write (e.g. `*(DWORD*)(+124)=20` → +124=20, +126=0); the
disassembly confirms them. Not all the AI chances are set per car (+124..+138 are missing on some), but the
table shows every write.

## 3. Special weapons (per car)

### Specials held and recharge: weapon block init `sub_800FDCAC(rec, weap, isHuman)` (weaponBlockInit). CONFIRMED.
The weapon block is human +0x7C8 / AI +0x808. The per-car values set there:
- `weap+0x0E` = specials held (starts at 2). `weap+0x0B` (s8) = **max specials held**. `weap+0x2C` =
  **special recharge period in frames**: the tick (8010388C) adds 1 special every +0x2C frames, capped at +0x0B.
- `weap+0x24` = total weapon-pickup capacity (24 for every real car; pickups are capped by it, 8010AFC8).

| id | car | max held (+0x0B) | recharge frames (+0x2C) |
|---|---|---|---|
| 0x10 | Sweet Tooth | 5 | 360 |
| 0x20 | Hammerhead | 5 | 1200 |
| 0x30 | Outlaw 2 | 3 | 1100 |
| 0x40 | Warthog | 5 | 700 |
| 0x50 | Mr. Grimm | 3 | 1400 |
| 0x60 | Grasshopper | 5 | 700 |
| 0x70 | Thumper | 5 | 1600 |
| 0x80 | Spectre | 4 | 700 |
| 0x90 | Roadkill | 5 | 700 |
| 0xA0 | Twister | 3 | 360 |
| 0xB0 | Axel | 5 | 700 |
| 0xC0 | Mr. Slam | 3 | 1000 (also arms the slam object, sub_8010EEA8 (weaponSlamStateInit)) |
| 0xD0 | Shadow | 5 | 400 |
| 0xE0 | Minion | 5 | 360 |
| 0xF0 | Dark Tooth | 5 | 700 |

For AI cars (and humans when `weap+0` is set) 8010388C resets every weapon count to 1 each tick before the
recharge step.

### Special fire `sub_80105A7C` (weaponFireSpecial) (weapon_basic.c), switch on car type
Shared setup: the muzzle is at (0, +218, +220), transformed by the car matrix, then +24 high.
- 0x10 rocket (sub_800D9C5C (weaponFireSpiralMissile) …, 15)
- 0x20 Hammerhead: only gives the charge back; the special is the car-mounting ram (+2 / 800E6D14)
- 0x30 helper (801075C8)
- 0x40 three emitters (center +224/+68+8, then ±+216), arg 6 each
- 0x50 Mr. Grimm (forward 0xA0, arg 40)
- 0x60 Grasshopper jump (spin 7 + 8010EF44)
- 0x70 Thumper (8010A5F0)
- 0x80 (arg 12)
- 0x90 (arg 10)
- 0xA0 shield/teleport state D_80184DC0 (weaponTornadoState) (timer 0x53)
- 0xB0 Axel: shock wave. **15 damage** to every car within 800 with |dz| < 200, plus a spin-out (sub_800E5628 (carApplyDamage)(…,15,…))
- 0xC0 Mr. Slam arm
- 0xD0 Shadow (below)
- 0xE0 Minion: center + side emitters, arg 10, plus a +200 high emitter (arg 7)

The trailing integer passed to the emitters (6, 7, 10, 12, 15, 30, 40) is the **damage** (CONFIRMED, request 40:
stored in the projectile record +0x0A, `weapon_fire_missiles.c`, and passed to `weaponApplyHit` by
`weaponProjectileUpdate`, `projectile_hit.c`). The pickup launcher 801052BC (`weaponLaunchProjectile`,
`weapon_launch.c`) passes: fire missile (kind 1) **7**, homing missile (kind 2) **10**, power missile (kind 4)
**15**, napalm (kind 5, energy kind 102) **10**, or **30** for Mega Napalm (`weap->mega_napalm`), freeze missile
(energy kind 100) **7**. (An earlier version of this note said "30 = power missile": 30 is Mega Napalm.) With
`TM2_PORT_BALANCE` bit 0 the fire missile does 10 and the homing missile 7 (`BALANCE.md` 3.1).

Damage application `sub_800E5628(target, amount, dir, type, attacker)` (carApplyDamage): amount ×2 if the attacker is a
human with +122 == 100 (Dark Tooth); halved when the target's shield (+60) is up; /4 for AI-vs-AI;
knockback scaled by the target's +122; sets the last-attacker / killer bytes for kill credit.

## 4. Shadow's special and the slope bonus

**Launch / detonate: `sub_80105A7C` (weaponFireSpecial), case 0xD0 (Hex-Rays export line ~38545; weapon_basic.c line 316).**
- State is the block `D_80184E10` (weaponStrikeState) (byte mode, owner id, `active` +4, `timer` +6, velocity +8, …, corner
  offsets `D_80184E3C` (weaponStrikeCornerOffsets), owner weapon `D_80184E6C` (weaponStrikeOwner), object `D_80184E70` (weaponStrikeObject) = asset 0x915 / 2325, set up in
  the loader that also writes the corners (±40,0,0)/(0,±40,0)).
- 1st press (or any AI press): spends a charge, `active=1`, `timer=100` frames, the object starts at the car
  position facing the car's angles (reversed if `weap+5` is set), speed `velocity[1]=0x42` (66/frame
  forward), sound 0x3B.
- 2nd press by a **human** while it is active = **manual detonation**: `active=timer=0`, hide the object,
  explosion fx `sub_801117B4(obj+0xA8)` (fxExplosion96), then **`sub_8010B5C4(obj+0xA8, owner, 0)` (areaDamage)**: the area damage.
- AI presses always relaunch (no manual detonation for AI).

**Per-frame move: `sub_8010F9C0` (weaponShadowStrikeUpdate) (strike_update.c).** Moves 66 forward along the object's own axes. It
samples the ground (`sub_800EF340` (worldGroundHeightAt)) under the four corners and sets pitch = −atan2(h0−h1, 80) and
roll = atan2(h3−h2, 80), so it climbs and follows slopes. Hit test `sub_800D6FD8(pos, owner, 20, 0x915)` (projectileHitTest):
on a hit (`>=501`, or any hit when the AI fired it) it explodes with a direct **25** damage
(`sub_800DC65C(hit,0,0,25,…)` (weaponApplyHit)). At timeout it moves the object up 80, plays the explosion and gives the charge
back (`--weap+0x0E`).

**Where the slope adds damage: `sub_8010B5C4` (areaDamage) (area_damage.c), the splash damage that manual detonation
calls.** For each car, distance d = max(|dx|,|dy|,|dz|) >> 3. If d < radius (75, or 100 when `big`):
- spin-out `sub_800E5FFC(car, …, 5, 0)` (carSpinOut)
- damage = **`25 − (d>>2)`**, or **`50 − (d>>2)` when the target's state byte +0x0A is set** (human
  record +0xA6 / AI +0x76; retail `lbu v0,166(s0)` at 8010B78C and `lbu v0,118(s0)` at 8010B9A8, then
  `li a1,0x32` vs `li a1,0x19`).
- State +0x0A is `on_surface`, set by the wheel step 800EC994 (car_wheels.c): at least one wheel is over a
  D_801B0418 (worldSurfaces) "surface" polygon (the sloped ramps / raised plates; type 0 rect, 1 triangle, 2 round, with
  per-corner heights).

So a car standing on a ramp/slope surface takes **+25** extra splash damage at every distance: 50 − d/4
instead of 25 − d/4, double at point-blank. The port note says "+24". The constant in retail is 0x32
vs 0x19 (+25). If the note counts from 49, or means something else, check it against this. The
function is shared: 801096F4 and 80109A9C (weapon slot writer, `big=1`, radius 100) call it too, so
a fix in 8010B5C4 also changes those explosions. To fix it for Shadow only, patch the 0xD0 call site,
or add a flag.

## 5. Open items
- +14 (u8 0..3) and +226/+228: no reader found by register tracking or direct-offset scan.
- ~~Emitter trailing argument = damage?~~ Confirmed (section 4 note and section 6; request 40).
- `weap+0x24` capacity is the same (24) for all real cars; only the fallback is 10.

## 6. Special weapon damage (traced to the damage call)

Projectile specials store their damage in the projectile record (+0x0A);
`sub_800DB110` (weaponProjectileUpdate) passes it to `sub_800DC65C` (weaponApplyHit) on a hit (`lhu a3,10(s1)` at
`800DBFA4` / `800DBFD8`), so the trailing integer of each emitter call is
the damage. Frame-based timers assume the game's usual 20 fps (3 ticks per
frame; this rate reproduces measured top and turbo speeds for 13 of 14
cars, 2 and 4 ticks match almost none).

| Car | Max damage, one use on one target | How |
|---|---|---|
| Sweet Tooth | 15 | one spiral missile |
| Hammerhead | +15 | crush, added to the ram damage; costs a charge (Axel, Minion, Dark Tooth crush for free) |
| Outlaw 2 | 20 | 10 at start + 10 after 40 frames, nearest car within 80 |
| Warthog | 18 | 3 missiles x 6; with a lock, each falls 1 per frame to 2 past 300 units |
| Mr. Grimm | 40 | one hit |
| Grasshopper | 15 | on reaching the target mid-leap (homes within 400, about 60 degrees); no self-damage in the special |
| Thumper | 47 + burn | `8010A7A0`: 1 per frame for up to 47 of the flame's 50 frames; first hit ignites (100 frames, 1-in-4 per frame -1 HP, about 25, stops at 5 HP) |
| Spectre | 12 | homing, passes walls |
| Roadkill | 40 | 10 going out, 40 on the return (turns at 300 units) |
| Twister | 28 | `8010E040`: 4 per catch (`li a3,4`); the catch's 60-tick freeze is cut by 6 x 4 = 24 by that same hit (`carApplyDamage`), so a human victim (or any victim of a human Twister) thaws after 36 ticks (12 frames at 20 fps) and is caught again: up to **7 catches = 28** in the tornado's 83 updates. A stats-shielded victim 2 per catch, 6 catches (12); an energy-shielded one 0; computer vs computer 1 per catch (the /4), 5 catches (5). The throw-out at the end of a stun never happens (`freeze_timer == 1` is never true: `RETAIL_BUGS.md` 21). Request 40: `BALANCE.md` 3.7 |
| Axel | 15 | every car within 800 with dz < 200 |
| Mr. Slam | 42 | 12 on the lift + 10 per slam, 1..3 slams by grab distance; cars with mass >= 35 (Hammerhead, Mr. Slam, Minion, Dark Tooth) take one hit of 25. Auto-grab: on Easy only (difficulty `D_80180FB8` (aiDifficulty) = 0), ramming a car with a human Mr. Slam arms the claw for free (`800E184C`, claw byte +11) and every value is halved (6 / 5 / 12, max 21) |
| Shadow | 25 | direct hit 25; detonation splash 25 - d/4 within 75, 50 - d/4 on a surface polygon |
| Minion | 37 | 3 missiles x 10 (falling to 2 at long range) + 7 freeze |
| Dark Tooth | 0 | a human Dark Tooth's special case is empty; his bonus is x2 on all damage he deals |

Damage modifiers (`sub_800E5628` (carApplyDamage)): x2 for a human-driven Dark Tooth; none
while the target holds the energy shield; halved by the shield flag
(stats +60); /4 when attacker and target are both AI (1..3 becomes 0);
ram hits under 5 cannot kill; hits on a frozen car shorten the freeze by
6 x damage. No difficulty scaling.

The table is retail. The PC port's Vanilla Plus rules (request 40, `TM2_PORT_BALANCE`, `docs/BALANCE.md`) change
Hammerhead (+15 at the mount, +5 once on top, ram x1.2), Axel and Minion (no crush, ram x1.1), Twister (no ram damage
taken during the tornado), Grasshopper (18, contact counts), Shadow's detonation (min(retail, 25) + 3) and five
recharge periods. Sweet Tooth's spiral stays retail (`BALANCE.md` 3.11). Request 41 (`BALANCE.md` 12) adds
Grasshopper's no hit before the hop and free energy jump, Twister's weapon lock during the tornado, Outlaw 2's taser
second chance, freeze diminishing returns and the ricochet's launch immunity; Hammerhead's recharge is 700.

### Who else uses Shadow's splash (`sub_8010B5C4` (areaDamage))

Three call sites: Shadow's manual detonation (`80106EA4`, range 75) and
the **Remote Bomb** pickup twice, with `big` = 1 (range 100): when its
owner detonates it (`80109900`, weapon slot kind 19, pickup kind 3) and
when a car drives into it (`80109CA4`). So the +25 on surface polygons
affects remote bombs too. The splash has no owner exclusion (the owner is
only passed for kill credit), so both can damage the car that set them
off. Energy mines (energy attack kind 101, slot kind 20) are separate: a
flat 15 on contact (`80109A9C`, `sub_800DC65C(..., 15, ...)` (weaponApplyHit)).
