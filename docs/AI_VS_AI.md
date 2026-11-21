# Computer cars fighting each other

Answers item 17 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(teams and spectating: computer cars get each other as targets through
`aiCarTargets` and hunt each other, but their health drops slowly). It
builds on `AI_LIVES.md` (the 4320 rule, `aiGetTargetCar`). Static
analysis only: the C in `src/semantic`, checked against the retail
disassembly (`bin/SCUS_94306R.EXE`, `mipsel-none-elf-objdump`) at the
places named below. Nothing was run. Addresses are retail.

Tags: **CONFIRMED** = seen in code (and, where a VA is given for an
instruction, in the retail disassembly). **LIKELY** = follows from the
code, not seen running.

AI car record offsets used below (`aiCars` 80188B5C + i*0x854, `struct
TM2AIRecord` in `include/semantic/cars.h`; `struct TM2AI` in `ai_steps.c`
names the same bytes):

| Offset | Field |
|---|---|
| +0x001 | mood (`aiUpdateMood`): 0 start, 1 hunt, 2 and 3 the other two (see 7.3) |
| +0x00A / +0x00C | AI cars nearer their targets than this one / skip counter (`worldUpdate` director, `aiCarShouldUpdate`) |
| +0x016..+0x01E | distance thresholds: near 320, r1 160, r2 600, r3 1000, far 2000 (set by `aiCarReset`, `ai_route_setup.c`) |
| +0x020 | bearing of the target relative to the car's heading (`target_angle`) |
| +0x022 / +0x024 | bearing to the target / that bearing relative to the target's own heading (0 = the target faces this car) |
| +0x028 | distance to the target (`target_dist`) |
| +0x034 | **in range of its target** (`active` in cars.h, `in_range` in ai_steps.c) |
| +0x037..+0x03A | range flags r1..r4 (target nearer than 160 / 600 / 1000 / 2000) |
| +0x0D4 | where the target is: 0 ahead, 1 behind, 2 right, 3 left |
| +0x044 | controls: +0x05E fire, +0x05F guns, +0x060 special |
| +0x4FC | stats: +0x576 mass, +0x588 retaliate budget, +0x5A0 HP, +0x5A4 takes damage |
| +0x808 | weapons: +0x80D rear fire, +0x812 **fire chance**, +0x814 energy action, +0x815 selected weapon, +0x82E fire cooldown, +0x840 / +0x844 weapon weight tables |

## Short answer

Computer cars do fire at each other about as often as at a player. Their
fire decision, weapon choice, energy use and driving read only the
target's position, heading, speed, mass and route section, through
`aiGetTargetCar` (8011CF0C), which handles a human and an AI target alike.
What is different is what happens to the shots:

1. **AI-on-AI damage is divided by 4** (`carApplyDamage` 800E5628, at
   800E5A4C..800E5A78). Integer division: machine guns (power 1, mega 3)
   and Thumper's flame (1) do **no** damage at all to a computer car when a
   computer car fires them; a homing missile does 2 instead of 10.
   CONFIRMED.
2. **Every aimed shot of a computer car steers at player 1** (`humanCars[0]`),
   not at its target (`weaponFindTargetAhead` 80109F94 / `weaponFindTargetBehind`
   8010A2AC, outside the attract demo). That is the fire, homing, napalm and
   freeze missiles and the specials of Sweet Tooth, Warthog, Mr. Grimm,
   Spectre, Roadkill and Minion. Against another computer car they curve
   away towards the player (or the player's wreck or spectator spot).
   CONFIRMED.
3. **The 4320 rule is the victim's own range flag**: a computer car takes
   damage only while it is within 4320 of *its own* target (+0x034), as
   last computed by its own AI. The attacker does not matter. With every
   car hunting the nearest enemy this is almost always true for a car that
   is being shot at close range, so it is a smaller factor. CONFIRMED.
4. **Update throttle**: outside the attract demo, a computer car with 3 or
   more cars nearer their targets than it is, that is not within 1000 of
   its target, and is either more than 4800 away or behind its target's
   back (|+0x024| > 1024), is not updated at all that frame (no driving, no
   firing). It was a "the player can't see it" saving; chasing another car
   from behind is exactly that position. CONFIRMED (code), LIKELY (how
   much it matters).
5. **The pack director** in `worldUpdate` limits how many cars hunt at once
   (it counts all cars, whoever they hunt). LIKELY to keep some cars out of
   the fight in a big team battle.

The retail attract demo (where computer cars fight) is not a model to copy:
there nobody loses HP at all (section 9).

The changes, in order of effect, are in section 10.

## 1. Every check, by function

"Human-only condition" is what makes the code behave differently when the
target, victim or attacker is a human. "Symmetric" means none.

| VA | Function (file) | Human-only condition | Effect on AI vs AI | Tag |
|---|---|---|---|---|
| 800E5A4C | `carApplyDamage` (car_damage.c) | `if (!human && !attacker_human) amount /= 4` | damage / 4 (truncated) | CONFIRMED |
| 800E585C | `carApplyDamage` | AI victim: `active = victim +0x034`; `!active` returns | no damage, no flash, no impact effect, no spin-out unless the victim is within 4320 of its own target | CONFIRMED |
| 800E5B08 | `carApplyDamage` | AI victim loses HP only when `attacker >= 0` | car attackers are >= 0: no effect | CONFIRMED |
| 800E5ACC | `carApplyDamage` | `gameIsAttractDemo()`: no HP loss for anyone | attract only | CONFIRMED |
| 80109FD8 | `weaponFindTargetAhead` (weapon_targets.c) | AI owner and not attract: target = `humanCarObject(0)->pos`, return | all aimed AI shots steer at player 1 | CONFIRMED |
| 8010A2F0 | `weaponFindTargetBehind` | same for rear shots | same | CONFIRMED |
| 8010FD44 | `aiDarkToothBossUpdate` (ai_dark_tooth_boss.c) | swoop target `humanCarNearest`, swoop damage and fire on `humanCarObject`, shots through the two finders above | Dark Tooth's boss moves only ever hit humans | CONFIRMED |
| 8011B7D8 | `aiCarShouldUpdate` (fx_explosions.c) | not attract: skip counter +0x00C, +0x039, +0x028 > 4800, abs(+0x024) > 1024 | a throttled car is not updated that frame | CONFIRMED |
| 8011AC18.. | `worldUpdate` director (world_update.c) | not attract | moods handed out over all cars together | CONFIRMED (code), LIKELY (effect) |
| 800EB8A0 | `aiSteerToGoal` (ai_steps.c) | `carSpinOut(ai, th, 7, 1)`: the AI's own record with the *target's* human flag | vs a human the AI record is written through human offsets; vs an AI the car really hops | CONFIRMED (code), LIKELY (effect) |
| 80103CA0 | `aiWeaponFireDecide` (weapon_tick_init.c) | none | symmetric | CONFIRMED |
| 800D3BB8 | `aiWeaponsUpdate`, `aiPickWeapon` 800D3C2C, tables 800D4124 / 800D420C | none | symmetric | CONFIRMED |
| 80103FB8 | `weaponFireSelected` (weapon_select.c) | none (melee auto-fire reads the target's slot for human or AI) | symmetric | CONFIRMED |
| 800D3C7C | `aiCarSetupProfile` (ai_route_nodes.c) | none | symmetric | CONFIRMED |
| 800E8604..800EC504 | `aiUpdateTargetRange`, `aiUpdateTargetBearing`, `aiUpdateMood`, `aiTryRouteToTarget`, `aiTryRouteToSlowTarget`, `aiShouldChaseDirect`, `aiFreeSteer`, `aiFollowSpeed`, `sub_800E9E20` (ai_steps.c), `aiFollowRouteTarget` 800F7D54 | `th` only picks the record layout; the fields read (position, heading, speed, mass, stats +0xB8/+0xBC/+0xC8, route section) mean the same in both | symmetric | CONFIRMED |
| 800D6FD8 | `projectileHitTest` (weapon_slam_swing.c) | humans are tested before AI cars; only human shots hit breakables (owner < 50) | an AI shot hits AI cars normally | CONFIRMED |
| 800D7E30, 800DB110 | `bulletsMove`, `weaponProjectileUpdate` | hit radius by *owner*: human bullets 10 / AI 5, human projectiles 15 / AI 8 | the same against players | CONFIRMED |
| 800D9AE0 | `weaponFireFreezeMissile` | turn rate by owner: human 15 / AI 4 | the same against players | CONFIRMED |
| 80104F70 | `UpdateGuns` (update_guns.c) | gun cooldown by owner: human 2 / AI 5 | the same against players | CONFIRMED |
| 8010C160 | `groundFiresUpdate` (fx_ground_fires.c) | burning AI loses HP only while +0x034 | same gate as damage | CONFIRMED |
| 800F5498 | `weaponSlamFindTarget` (screen_card_draw.c) | humans within 160 are grabbed before AI cars | Mr. Slam prefers a human in reach | CONFIRMED |
| 800E5628 (break-up) | `carApplyDamage` | debris of a broken-up wreck flies from the nearest human (`humanCarNearest`) | looks only | CONFIRMED |
| 8011A700 | `worldUpdate` radar, sounds | radar relative to humans; engine sound and gun loop only while +0x034 | looks and sound only | CONFIRMED |

Not found anywhere in the AI: a line-of-sight test, a check of the target's
object id against 50, a check of the target's invisibility, or a loop over
human cars only, other than the rows above.

## 2. The damage gate (the 4320 rule)

`carApplyDamage(target, amount, dir, type, attacker)` (800E5628), for an AI
victim (CONFIRMED, 800E585C `lbu t1,52(s4)`):

```
active = victim->active;            /* +0x034 */
... knock-back (type 1, amount >= 11) and the energy shield come first ...
if (!active || (roundIsWon() && type != 0xF0)) return;
```

So:

- It is the **victim's** flag. The attacker's distance, kind or target do
  not enter.
- The flag means "this car is within 4320 of **its own** target" (the car
  in `aiCarTargets[victim]`, or the nearest human when that is -1), **or**
  it is driving a route towards its target (+0x455 set and +0x468 > 0).
  `aiUpdateTargetRange` (800E8604, `slti 4320` at 800E8754) writes it once
  per AI update of the victim; for a wreck, `carWreckUpdate` (800E75F4)
  rewrites it from the plain distance.
- It is as old as the victim's last AI update: a car skipped by the
  throttle (section 6) keeps its old value.
- When the gate closes, the hit shows nothing: no red/white flash
  (`lightFlickerStart`), no impact debris, no spin-out. Only a heavy blow's
  knock-back and the shield's "put out the fire" still happen.
- Burning (`groundFiresUpdate`) also drains HP only while the flag is set.

In retail every AI targets the nearest human, so the rule reads "a computer
car can't be hurt unless a player is within 4320 of it". With teams, if
every car hunts the nearest car of another team, a car being shot from
under 4320 away has an enemy (the shooter) within 4320, so its own target
is nearer still and the gate is open. It closes when targets are sticky or
chosen another way (a car shot by a third car while it chases someone far
away), and when the shooter is far away (homing missiles, long napalm).

## 3. AI-on-AI damage is divided by 4

`carApplyDamage` (CONFIRMED, 800E5A4C..800E5A78; C in `car_damage.c`):

```
attacker_human = 0; carFromSlot(attacker, &attacker_human);   /* 800E5748 */
...
if (st->shield) amount /= 2;                 /* damage-halving stat */
if (!human && !attacker_human) amount /= 4;  /* AI victim, AI (or no) attacker */
victim HP -= amount  (AI victim: only when attacker >= 0; also the retaliate budget +0x588)
```

The cut comes after the "amount <= 0 returns" test, so a hit of 1..3 still
flashes and still counts as the last attacker, but takes 0 HP. Ram hits
(type 0) under 5 cannot kill, after the cut.

Damage of the AI's weapons (the last argument of the launch functions, or
the `carApplyDamage` amount) and what an AI victim takes from an AI:

| Weapon | Damage | AI on AI |
|---|---|---|
| machine guns / mega guns (`UpdateGuns`, bullet power; half of the hits apply, `rand() & 1`) | 1 / 3 | **0 / 0** |
| Thumper flame (`weaponFlameUpdate`, 1 per hit) | 1 | **0** (burning still drains, see 2) |
| fire missile (kind 1) | 7 | 1 |
| homing missile (kind 2) | 10 | 2 |
| napalm (kind 5) / mega napalm | 10 / 30 | 2 / 7 |
| power missile (kind 4) | 15 | 3 |
| freeze missile (energy 100) | 7 | 1 |
| Sweet Tooth spiral, Axel push, homing leap | 15 | 3 |
| Warthog / Minion coloured missiles | 6 / 10 each | 1 / 2 |
| Mr. Grimm | 40 | 10 |
| Spectre / Roadkill | 12 / 10 | 3 / 2 |
| taser arcs | 10 | 2 |
| Shadow strike | 25 | 6 |
| area blast (`areaDamage`) | 50 or 25, minus distance | / 4 |
| car-car ram (`carCarCollision` 800E184C, `car_collision.c`) | speed based | / 4 |

A computer car hit by a human, and a human hit by a computer car, take the
full amount. In retail the cut only applied to crossfire (an AI hitting an
AI it was not after), which kept computer cars from killing each other
while chasing the player.

## 4. Aim and homing: every AI shot steers at player 1

`weaponFindTargetAhead` (80109F94) and `weaponFindTargetBehind` (8010A2AC)
give a launched projectile its target: a pointer to a car object's position
(object +0xA8), stored in the projectile (+0x38) and followed each frame by
`weaponProjectileSteer` (800DA5B4) at the projectile's turn rate.

```
if (owner is an AI && !gameIsAttractDemo()) {   /* 80109FD0..80109FFC */
    *out = humanCarObject(0)->pos;              /* player 1, always */
    return;
}
/* otherwise (a human owner, or the attract demo): the nearest live AI car
 * in the owner's cone (local y >= 12, |y| >= |x|, y < 8001, z < 401 ahead;
 * y < -11 behind), and only when none, the nearest human in the cone */
```

CONFIRMED (disassembly: `jal gameIsAttractDemo`, then `jal 8011CB50`
with a0 = 0, `addiu v0,v0,168`). It is player 1, not the nearest human and
not the AI's target (in two-player games too).

Callers with an AI owner, all aiming at player 1:

| Shot | Function | Notes |
|---|---|---|
| fire missile (1), homing (2), napalm (5), freeze (energy 100), napalm energy (102) | `weaponLaunchProjectile` 801052BC | power missile (4) has no target, flies straight |
| Sweet Tooth (0x10), Warthog (0x40), Mr. Grimm (0x50), Spectre (0x80), Roadkill (0x90), Minion (0xE0) specials | `weaponFireSpecial` 80105A7C | |
| Dark Tooth boss shots | `aiDarkToothBossUpdate` 8010FD44 | |
| energy 106 | `weaponFireEnergyAttack` 801046FC | AI never picks it |

A steering shot turns towards player 1 from the moment it is fired. With
the target a few hundred units away and player 1 elsewhere, most of them
turn away before they arrive. Machine-gun bullets and the power missile do
not steer and fly along the car's heading; Taser (Outlaw), homing leap
(Grasshopper), flame (Thumper), tornado (Twister), Axel's push and Mr. Slam
pick the nearest car in reach themselves (any car, AI or human, no team
test) and are not affected.

## 5. The fire decision and the rest of the attack AI (all symmetric)

### 5.1 `aiWeaponFireDecide` (80103CA0, weapon_tick_init.c)

Called each AI update from `weaponTick` (8010388C) via `aiWeaponsUpdate`
(800D3BB8), which runs only while +0x034 is set (or for Dark Tooth). The
attacker therefore fires only within 4320 of its own target. CONFIRMED.

| Step | Rule |
|---|---|
| guns | abs(+0x020) < 136 (about 12 degrees), +0x028 < 4000, no weapon launched this frame -> guns (+0x05F); `UpdateGuns` fires every 6th call (cooldown 5) |
| cone width | n = 4, or 1 when the target is nearer than 2000 (r4 is always set with r2 / r3); forward cone abs(+0x020) < 113 * n: 10 degrees near, 40 degrees far |
| forward roll | `rand() / 328 < fire chance (+0x812)` -> fire the selected weapon (+0x05E) when +0x815 < 8, else the energy action (+0x060); Outlaw switches between special and weapon 1 by r2. A failed roll clears fire and reloads the cooldown +0x82E from `weaponFireInterval` when it has run out |
| rear roll | +0x0D4 == 1 (target behind, abs(+0x020) >= 1537) and abs(+0x020) > ((180 - 10n) << 12) / 360: the same roll, with rear fire (+0x80D) |
| Dark Tooth | level 8: guns off, `aiDarkToothBossUpdate` |

No line-of-sight test, no lead on the target's speed: the guns fire along
the heading (the "aim" block is body sway), and steering shots chase the
target pointer's current position.

`weaponFireSelected` (80103FB8) then applies the cooldown
`weaponFireInterval` (80180F44) = (80 - 4 * level) >> difficulty, and for
cars with a melee special (`carHasMeleeSpecial`) forces the special when
the target (human or AI, by its slot) is within 400 and 227 of the heading.

### 5.2 Weapon choice and energy

`aiPickWeapon` (800D3C2C), while the cooldown is under 2: target ahead
(+0x0D4 0) -> 9-entry table (+0x840), target behind (1) -> 10-entry table
(+0x844), left or right -> keep the last pick. A pick of 8 or 9 is an
energy action: 100 freeze missile, 101 mine (only in mood 2), else none.
`weaponTick` refills every AI weapon each frame (ammo 1, energy 1000,
turbo 990). `sub_800E9E20` (route chase, slowed down) picks a special by
level and car. All read only distance and bearing. CONFIRMED.

### 5.3 `aiCarSetupProfile` (800D3C7C): the fire chance

`+0x812 = 3 * level + 15 * difficulty + base`, base 17 (Hammerhead, Outlaw,
Mr. Grimm, Grasshopper, Thumper, Spectre, Twister, Axel), 20 (Mr. Slam), 22
(Warthog), 30 (Roadkill, Shadow); 100 for Sweet Tooth, Minion and Dark
Tooth. +45 on levels 5 and 6, +15 in game mode 2, 100 in mode 3 with a
single AI car, at most 100. It picks the weight tables by car type. Nothing
depends on the target. CONFIRMED.

### 5.4 Driving at the target, lead, ramming

| Function | What it reads from the target | Tag |
|---|---|---|
| `aiUpdateTargetRange` 800E8604 | position: distance, height, range flags | CONFIRMED |
| `aiUpdateTargetBearing` 800EC3A4 | position, heading: +0x020/+0x022/+0x024, +0x0D4 | CONFIRMED |
| `aiFollowRouteTarget` 800F7D54 | route section, position, heading, speed: in moods 0/1 within 2000 the aim point is ahead of the target by half the distance plus speed * 1600 / top speed (the only "lead") | CONFIRMED |
| `aiFollowSpeed` 800EBFF8 | heading, speed: match the target's speed when following it | CONFIRMED |
| `aiShouldChaseDirect` 800EBB74, `aiTryRouteToTarget` 800E97B0, `aiTryRouteToSlowTarget` 800E9950 | route section, height, speed and stats +0xB8 / +0xBC / +0xC8 | CONFIRMED |
| `aiSteerToGoal` 800EB714 | mass: target heavier, ahead (abs(+0x020) < 227, abs(+0x024) < 113) and within 600 -> hop (`carSpinOut` kind 7), within 1200 -> pass | CONFIRMED |
| `aiFreeSteer` 800EA270 | mass: a lighter target is followed at its speed now and then | CONFIRMED |

The human target's route section (`state.x2C.route`) and an AI's
(`drive.route` +0x46A) are both the world section under the car
(`carUpdateContacts`; the human's only when all four wheels agree). The
stats fields are the same struct in both records.

The one real difference is the hop in `aiSteerToGoal`: retail passes the
target's human flag with the AI's own record (800EB8A0, `a1 = lbu 32(sp)`,
the flag from `aiGetTargetCar`). Against a player the AI record is read and
written through human offsets (state +0x9C, wheels +0x5A4: the AI's aim and
stats bytes), so in retail the hop never really happened; against an AI
the AI car really hops instead of ramming a heavier target. LIKELY a small
effect (fewer rams).

## 6. The update throttle (`aiCarShouldUpdate` 8011B7D8)

CONFIRMED (disassembly matches the C):

```
if (!gameAttractDemoMode) {
    if (wreck state == 4) return 0;
    if (+0x00C != 0 && !r3 /* +0x039, target nearer than 1000 */) {
        if (+0x028 > 4800) return 0;
        if (abs(+0x024) > 1024) return 0;   /* not in front of its target */
    }
}
return 1;
```

`worldUpdate` (8011A700) sets +0x00C: in the director loop it is cleared
when fewer than 3 live AI cars are nearer their targets than this one,
else counted up; the car with the highest count is cleared again (it gets
its turn); an updated car is cleared, a skipped one counted up. A skipped
car gets no `aiCarUpdate` (8011C4A0, call at 8011B19C): no AI, no driving
or physics integration, no firing, and its +0x034 stays as it was.

With one player and 5 computer cars this only touched the farthest cars
out of the player's sight. In a team battle with many cars, a car chasing
another from behind (abs(+0x024) > 1024 means it is behind its target's
back) between 1000 and 4320 away is skipped for most frames whenever 3
other cars are closer to their own targets. LIKELY one reason some cars
"fire little".

## 7. Moods and the pack director

### 7.1 The director (`worldUpdate`, not attract; from 8011AC18)

Over all AI cars that are not broken up, by +0x028 (distance to each car's
own target): when 4 or more are hunting (mood 1) or 4 or more are within
4800, the hunting or mood-3 car with the lowest hunt chance (+0x580) that
has been in its mood 241+ ticks goes to mood 2 or 3 (coin flip); when none
is within 4800, the nearest car in mood 2/3 past half its time goes to mood
0 for 600 ticks; when none hunts, the car with the highest hunt chance
hunts for 3600 ticks. CONFIRMED (code).

It was made to keep about 3 cars on the one player. In a team battle it
counts every car together, so with 4 or more cars within 4800 of their
targets it keeps pushing one car a frame out of hunting. LIKELY.

### 7.2 `aiUpdateMood` (800E8A84)

Per car: rolls the next mood from health and the profile chances when its
time is up; mood 0 turns to mood 2 when the target is close and facing it
(abs(+0x024) small); mood 0/1 turns to 2 when the retaliate budget
(+0x588, lowered by every hit with an attacker) goes below 0. Symmetric.

### 7.3 What the moods do

Only mood 1 drives straight at the target (`aiShouldChaseDirect`) and
routes towards it (`aiStartRouteFollow` -> `aiTryRouteToTarget`); moods 0
and 1 aim the route choice at a point ahead of the target
(`aiFollowRouteTarget`); mood 2 picks the route node *farthest* from the
target's position (`aiPickLinkNode` 800F7B00) and is the only mood that
lays mines; mood 3 takes a random next node (`aiChooseNextNode`
800F763C). Firing does not depend on the mood. CONFIRMED (code); the names
"attack" and "flee" in older docs are LIKELY loose.

## 8. Other places where a human is special

| Where | What | Matters for AI vs AI? |
|---|---|---|
| `aiGetTargetCar` 8011CF0C | -1 = the nearest human (`humanCarNearest`, alive or not); an AI index has no HP or dead test | the port must retarget at a death (`AI_LIVES.md`) |
| `aiCarTargets` 801879FC | 9 words (up to `carSlotKinds` 80187A20); reset to -1 by `arenaSetup` 8011995C and `uaInit` | there is no way to point at one human |
| `projectileHitTest` 800D6FD8 | the energy shield stops freeze and fire only on humans; humans tested first | small |
| `weaponShieldsUpdate`, `weaponInvisibilityUpdate` | human records only (`AI_LIVES.md`) | AI invisibility has no look, and no car ignores an invisible target |
| radar, HUD target panel, engine and gun sounds | humans only / +0x034 | no |

## 9. The attract demo (where retail AI fights AI)

`gameAttractDemoMode` (80180FBC, `gameIsAttractDemo` 8011EDC4). Set up by
`roundStart` (8011A024) and `demoRespawnAiCars` (8011EFDC), CONFIRMED:

| Thing | Attract demo | A match |
|---|---|---|
| targets | `aiCarTargets[0..1] = 3`, `[2..3] = 1` (cars 4+ keep -1: the hidden human car) | all -1 (nearest human) |
| HP loss | **none for anyone** (`carApplyDamage` 800E5ACC); hits only flash and spin | normal, AI on AI / 4 |
| missile targets | nearest live AI car in the cone, then a human | AI shots: player 1 |
| fire chance | 100 for every car | profile (17..100) |
| HP | max minus `rand() & 63` per round (smoke variety) | max |
| update throttle, director | off (every car updated every frame) | on |
| mood | 1 (hunt) at the start (`roundStart`) | `rand() % 3` |
| route aim point | `aiFollowRoute`: only when +0x47A < 2 | always |
| rounds | respawned every 600 ticks (10 s), 5 rounds, then the title | |

The demo only had to look busy: nobody can die, so the /4 cut never shows,
and the missile finder looks at AI cars (but at any AI car in the cone, a
teammate too, not the car's target).

## 10. What to change for AI vs AI

In order of effect. "AI-vs-AI mode" is the port's teams/spectating mode;
in the original modes keep retail.

| # | Where | Change | Tag |
|---|---|---|---|
| 1 | `carApplyDamage` 800E5628, the `/4` (800E5A4C..800E5A78) | skip it when the attacker is an AI car of another team (keep it for retail modes, and for friendly fire if wanted). Do not cut by a smaller divisor: guns and flame need 1 to stay 1 | CONFIRMED |
| 2 | `weaponFindTargetAhead` 80109F94 / `weaponFindTargetBehind` 8010A2AC, the "AI owner, not attract" branch | give the AI's own target instead of player 1: `t = aiCarTargets[owner]; *out = (t == -1) ? humanCarObject(humanCarNearest(aiCars[owner].pos, -1))->pos : aiCarObject(t)->pos;` (object +0xA8, as retail). For the behind finder the same (retail does not check the cone for this branch either). The attract branch (nearest AI in the cone) would also work but can lock onto teammates | CONFIRMED |
| 3 | `aiCarShouldUpdate` 8011B7D8 | in AI-vs-AI mode return 1 except for wreck state 4 (keep that test: a broken-up wreck must stay off) | CONFIRMED |
| 4 | `carApplyDamage` gate 800E585C (and `groundFiresUpdate`'s +0x034 test) | with nearest-enemy targeting it can stay. If targets are sticky or chosen otherwise, open it for car attackers: `active = victim +0x034 || (aiVsAi && attacker >= 0)` | CONFIRMED (rule), LIKELY (need) |
| 5 | `worldUpdate` director (from 8011AC18) | run it per target (count only cars whose target is the same car), or skip it as the attract demo does | LIKELY |
| 6 | `aiSteerToGoal` 800EB8A0 | optional: to match retail behaviour against players, skip the hop when the target is an AI (`th == 0`); retail never really hops | LIKELY |
| 7 | `aiDarkToothBossUpdate` 8010FD44 | Dark Tooth as an opponent of computer cars needs its swoop target from `aiGetTargetCar` instead of `humanCarNearest` / `humanCarObject` | CONFIRMED (code) |

Not to change, for "as hard as against the players": `aiWeaponFireDecide`,
the fire chance, `weaponFireInterval`, the weapon tables, the AI's smaller
hit radius (bullets 5, projectiles 8), its slower freeze missile (turn 4)
and gun cooldown (5). They are already the same against a player and
against a computer car.

Also needed by the port, not part of the retail AI:

- Team tests are nowhere in the retail code: `projectileHitTest`, the
  finders, the taser, homing leap, Mr. Slam, the tornado, Axel and area
  blasts hit any car. Friendly fire off has to be done in
  `carApplyDamage` (or the hit tests) by the port.
- `aiGetTargetCar` has no HP test: retarget when a target dies, before
  the next `aiCarDrive`.
