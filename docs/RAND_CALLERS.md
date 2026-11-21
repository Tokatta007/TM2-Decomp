# rand() callers: game state or visual only
Every call of the BIOS `rand()` (`80138888`) in the retail executable
(`SCUS_94306R.EXE`), classified for online play: **GAME_STATE** calls must draw
from the random stream every machine shares; **VISUAL** calls only affect
particles, sounds, flashes, HUD or model animation and can move to a
separate, local stream. Requested in `docs/DECOMP_REQUESTS.md` section 1 of
the PC port.

Machine-readable copy: [`rand_callers.tsv`](rand_callers.tsv) (one line per
call site).
## Summary
| | Call sites | Functions |
|---|---:|---:|
| GAME_STATE | 208 | 52 |
| VISUAL | 90 | 27 |
| mixed (both kinds in one function) | | 6 |
| **Total** | **298** | **85** |
## Method and confidence

Each call site was traced from the Hex-Rays export, the decompiled C in
`src/semantic` and the disassembly to where its value goes. A call is VISUAL
only when every use of the value ends in particle / effect emitters
(`sub_8010BDE8` (mathJitterPos) and the `sub_8011xxxx` emitter family), sound, screen flash,
HUD drawing or render-time model animation. Anything that reaches car, AI,
weapon, pickup, debris (`sub_8014B820` (debrisBurstAt)) or level-event state, or any timer
that later gates game logic, is GAME_STATE. Doubtful cases were put under
GAME_STATE; moving a VISUAL call to the shared stream is harmless, the
opposite causes desyncs.

Not verified at runtime. Before relying on a VISUAL entry, a quick check is
to run a recorded online session with that call on the local stream on two
machines and compare game-state hashes.

## Things the port must know

1. **Every call consumes the stream.** A call whose value is discarded still
   advances the generator (for example `800E9924` in the AI route code); it
   is classified by its function's role.
2. **Some VISUAL calls depend on local state and must not stay on the shared
   stream.** The radar blips in `8011A700` (sites `8011A828`, `8011A8F0`,
   `8011B460`, `8011B530`) use `dist < 400 || rand() % 5 == 0`: whether
   `rand()` runs at all depends on the local car's position. Calls made while
   drawing (model animators `80151BC4`, `80151CB8`, `80151DF4`, reached from
   the draw setup at `80152284`) run once per rendered view. Both kinds
   desync the shared stream unless moved to the local one.
3. **`8010BDE8` is a shared helper** (random offset for an effect position).
   All its current callers are effects, so it is VISUAL; a port change that
   uses it for game logic has to revisit this.
4. **Seeding.** `srand(GetCurTics())` runs at boot (`801197D4`), level setup
   (`80119C78`) and round reset (`8011A03C`); the first draw after the round
   reset (`8011A0D0`, picks `gp+0x5D8 = rand() % car count`) is GAME_STATE.
   Online play needs one agreed seed at each of these points.
5. **Front-end picks.** Random car choices for match options and the demo
   match (`8012CD88`, `8012DECC`) are GAME_STATE here; if the online lobby
   syncs the chosen cars instead, they can be local.

## Entries to review

These are GAME_STATE only because nothing proved them cosmetic; each could
move to VISUAL after the check named:

| Function | Sites | Value feeds | Check |
|---|---:|---|---|
| `800E4F30` | 4 | car body bounce amplitudes (car record bounce block +0x28/+0x2A) | nothing outside rendering reads the resulting tilt (+0x30/+0x32) |
| `800E4D8C` | 4 | car tilt / shake start, reaches the body matrix via `CarRotUpdate` | nothing but rendering reads the body matrix (motion block +0x70) |
| `8011F510`, `8011FC6C`, `801203EC` | 12 | spawn-table coin flips writing P1 record +0x14 and `D_80180FD4` (demoFollowPlayerCamera) | these only steer the view target in battle display mode |
| `80116FF8`, `801176C0` | 19 | positions of pooled explosion models (`sub_80119468` (fxSpawnPooledBlast), target event 686) | those models have no collision or damage |
| `8012305C` | 2 | random entry of a four-way choice | the choice is menu UI, not level script |
| `800D4AAC` | 1 (`800D4BD0`) | event record word +0x16 | find its reader |
| `8014BAD4` | 17 | debris object setup (`sub_8014B820` (debrisBurstAt) family) | debris has no collision with cars |

Weakest VISUAL call: `800DA9DC` (homing projectile model flicker, object
+0x11C read as the model word); move it to GAME_STATE if game logic reads
+0x11C.

## By function

### `800D3B34` sound_repeat_throttle (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800D3B48` | VISUAL | random 12-26 frame interval throttling a level sound (sub_800D3334 (soundPlay)); only its own gp+40/44 throttle state |

### `800D4124` sub_800D4124 (aiPickWeaponFromTable9) AI weighted behaviour pick (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800D4134` | GAME_STATE | weighted table pick (table at car+0x840) -> AI state car+0x815/+0x814 |

### `800D420C` ai_pick_weapon_weighted (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800D421C` | GAME_STATE | weighted pick from table rec+0x844 -> weapon/action choice rec+0x814/+0x815 (AI) |

### `800D4AAC` event_debris_setup (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800D4B2C` | GAME_STATE | test choosing debris launch speed stored in event record (later passed to sub_8014B820 (debrisBurstAt)) |
| `800D4B44` | GAME_STATE | debris launch speed stored in event record (sub_8014B820 (debrisBurstAt) arg) |
| `800D4B5C` | GAME_STATE | debris rotation X stored in event record (sub_8014B820 (debrisBurstAt) arg) |
| `800D4B78` | GAME_STATE | debris rotation Y stored in event record (sub_8014B820 (debrisBurstAt) arg) |
| `800D4B9C` | GAME_STATE | frame delay before debris launch (event timer gating sub_8014B820 (debrisBurstAt)) |
| `800D4BD0` | GAME_STATE | event record timer (+0x16) set at event start |

### `800D4D58` sub_800D4D58 (moscowEventsUpdate) level destructible effects tick (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800D4DF8` | VISUAL | gate (rand % f03) for sub_8010BDE8 (mathJitterPos) smoke particle |
| `800D4E68` | VISUAL | pick sub_8011160C (fxExplosion24) vs sub_801116E0 (fxExplosion48) particle |
| `800D4F74` | VISUAL | 1-in-3 sub_801117B4 (fxExplosion96) particle |
| `800D513C` | VISUAL | gate (rand % f03) for sub_8010BDE8 (mathJitterPos) particle |
| `800D51AC` | VISUAL | pick sub_8011160C (fxExplosion24) vs sub_801116E0 (fxExplosion48) particle |
| `800D52F8` | VISUAL | 1-in-3 sub_801117B4 (fxExplosion96) particle |

### `800D58BC` level_event_debris_setup (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800D593C` | GAME_STATE | event timer test: launch-delay halfword that gates sub_8014B820 (debrisBurstAt) debris launch |
| `800D5954` | GAME_STATE | event timer value (launch delay) gating sub_8014B820 (debrisBurstAt) debris launch |
| `800D596C` | GAME_STATE | debris launch vector x passed to sub_8014B820 (debrisBurstAt) |
| `800D5988` | GAME_STATE | debris launch vector y passed to sub_8014B820 (debrisBurstAt) |
| `800D59AC` | GAME_STATE | event frame at which debris launches (+20..) |
| `800D5A1C` | GAME_STATE | event movement direction +/-7 (then sub_800F20C8 (worldSectionDisable) AI node reset) |

### `800D5AF0` event_debris_update (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800D5B88` | VISUAL | rand % rate gates a sub_8010BDE8 (mathJitterPos) smoke/fire particle only |
| `800D5C0C` | VISUAL | picks particle type sub_8011160C (fxExplosion24) vs sub_801116E0 (fxExplosion48) |
| `800D5D1C` | VISUAL | 1-in-3 gate for sub_801117B4 (fxExplosion96) particle |

### `800D5F94` sub_800D5F94 (moscowDishStartDestroy) destructible debris event init (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800D5FE0` | GAME_STATE | debris flight time word_801B2DD6 (sub_8014B820 (debrisBurstAt) arg + phase timer) |
| `800D5FF8` | GAME_STATE | debris launch angle word_801B2DFC (sub_8014B820 (debrisBurstAt)) |
| `800D6018` | GAME_STATE | debris launch angle word_801B2DFE (sub_8014B820 (debrisBurstAt)) |
| `800D6044` | GAME_STATE | event delay word_801B2DD4 (gates debris launch + sub_80124384 (objRegistrySetStep) event) |
| `800D6064` | GAME_STATE | sign of debris spin word_801B2DE8 |
| `800D6078` | GAME_STATE | debris spin word_801B2DE8 (sub_8014B820 (debrisBurstAt)) |
| `800D608C` | GAME_STATE | debris spin word_801B2DE8 (negated branch) |
| `800D60C0` | GAME_STATE | sign of debris spin word_801B2DEA |
| `800D60D4` | GAME_STATE | debris spin word_801B2DEA (sub_8014B820 (debrisBurstAt)) |
| `800D60E8` | GAME_STATE | debris spin word_801B2DEA (negated branch) |

### `800D6188` level_event_explosion_fx_tick (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800D620C` | VISUAL | rand % rate gates only sub_8010BDE8 (mathJitterPos)+sub_8011160C (fxExplosion24)/801116E0 smoke particles |
| `800D6270` | VISUAL | picks particle emitter 8011160C vs 801116E0 |
| `800D62E0` | VISUAL | picks particle emitter 8011160C vs 801116E0 |
| `800D649C` | VISUAL | 1-in-3 gate for sub_801117B4 (fxExplosion96) particle only |

### `800D6710` level_event_start (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800D6754` | GAME_STATE | event delay word_801AB81A gating timeline and sub_8014B820 (debrisBurstAt) debris launch |

### `800D6800` level_event_update (mixed)

| Site | Class | Value feeds |
|---|---|---|
| `800D6AEC` | VISUAL | rand % rate gates sub_8010BDE8 (mathJitterPos) particle bursts |
| `800D6BCC` | VISUAL | picks particle type sub_801116E0 (fxExplosion48) vs sub_801117B4 (fxExplosion96) |
| `800D6C04` | VISUAL | coin flip for extra sub_80111888 (fxExplosion192)/sub_801121D4 (fxSmokeA48) particles |
| `800D6CB4` | GAME_STATE | rotation arg (rand/1024) to sub_8014B820 (debrisBurstAt) debris launch |

### `800D7E30` projectile_collision_tick (mixed)

| Site | Class | Value feeds |
|---|---|---|
| `800D8138` | VISUAL | gates sub_80112094 (fxSmokeA6) impact particle only |
| `800D8170` | GAME_STATE | gates sub_800DC65C (weaponApplyHit) hit/damage application |

### `800D9D38` spawn_projectile_variant (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800D9D64` | GAME_STATE | projectile record field word_801A5B10 = 20 or 99 |

### `800DA5B4` homing_projectile_steer (mixed)

| Site | Class | Value feeds |
|---|---|---|
| `800DA7E4` | GAME_STATE | type-41 projectile yaw jitter (obj+164) |
| `800DA9DC` | VISUAL | projectile model word +0x11C and trail colour (sub_800D8A7C (missileSetTrailColor)) flicker |
| `800DAA54` | GAME_STATE | type-49 projectile yaw jitter (obj+164) |
| `800DABE4` | GAME_STATE | type-41/49 projectile pitch jitter (obj+160) |

### `800DD304` car_speed_turbo_update (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800DDA28` | GAME_STATE | AI car: doubles threshold that clears car flag (+26, turbo/fire state) in car record |

### `800E0C28` car_wall_collision (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E1184` | GAME_STATE | random heading deflection (rand%682) written to car heading target after collision |
| `800E127C` | GAME_STATE | random heading deflection (rand%682) written to car heading target after collision |

### `800E4D8C` sub_800E4D8C (carShakeStart) car tilt shake start (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E4DEC` | GAME_STATE | tilt repeat count (tilt+0x0C); car tilt/bump oscillation block (human car+0x114 / AI car+0x9C) -> body_m matrix in CarRotUpdate; conservative since body matrix may feed wheel/weapon geometry |
| `800E4E00` | GAME_STATE | tilt phase (tilt+0x0E); car tilt/bump oscillation block (human car+0x114 / AI car+0x9C) -> body_m matrix in CarRotUpdate; conservative since body matrix may feed wheel/weapon geometry |
| `800E4E98` | GAME_STATE | tilt X amplitude sign (tilt+0x20); car tilt/bump oscillation block (human car+0x114 / AI car+0x9C) -> body_m matrix in CarRotUpdate; conservative since body matrix may feed wheel/weapon geometry |
| `800E4ED0` | GAME_STATE | tilt Y amplitude sign (tilt+0x22); car tilt/bump oscillation block (human car+0x114 / AI car+0x9C) -> body_m matrix in CarRotUpdate; conservative since body matrix may feed wheel/weapon geometry |

### `800E4F30` car_bounce_init (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E4F6C` | GAME_STATE | car record bounce amplitude (+0x28); probably cosmetic body tilt but stored in car record, readers of +0x30/+0x32 not verified |
| `800E4FDC` | GAME_STATE | car record bounce amplitude (+0x28); probably cosmetic body tilt but stored in car record, readers not verified |
| `800E5050` | GAME_STATE | car record bounce amplitude (+0x2A); probably cosmetic body tilt but stored in car record, readers not verified |
| `800E50C0` | GAME_STATE | car record bounce amplitude (+0x2A); probably cosmetic body tilt but stored in car record, readers not verified |

### `800E5628` apply_damage (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E59C4` | GAME_STATE | heavy-hit chance gates sub_800E5FFC (carSpinOut) car knock/spin physics |

### `800E5FFC` car_hit_reaction_physics (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E6394` | GAME_STATE | case 10 car record knock value (rec+0x6DC/0x71C) |
| `800E63F0` | GAME_STATE | case 10 car record knock value (negative branch) |
| `800E65D4` | GAME_STATE | wheel/suspension offsets in car record |
| `800E65F8` | GAME_STATE | wheel/suspension offsets in car record |
| `800E6644` | GAME_STATE | wheel/suspension offsets in car record |
| `800E6684` | GAME_STATE | rand&7 selects knock type: car angle/velocity/spin changes |

### `800E6D14` sub_800E6D14 (carCrashStart) car-vs-car crash wheel kick (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E6FB8` | GAME_STATE | wheel 0 record field (car wheels block +0x5A4) of this car |
| `800E7018` | GAME_STATE | wheel 1 record field of this car |
| `800E7078` | GAME_STATE | wheel 2 record field of this car |
| `800E70D8` | GAME_STATE | wheel 3 record field of this car |
| `800E7138` | GAME_STATE | wheel 0 record field of other car |
| `800E719C` | GAME_STATE | wheel 1 record field of other car |
| `800E7200` | GAME_STATE | wheel 2 record field of other car |
| `800E7264` | GAME_STATE | wheel 3 record field of other car |

### `800E75F4` car_burning_update (mixed)

| Site | Class | Value feeds |
|---|---|---|
| `800E7770` | VISUAL | 1/16 gate for sub_8010BDE8 (mathJitterPos) smoke particle |
| `800E77B8` | VISUAL | picks particle type sub_80112100 (fxSmokeA12) vs sub_8011216C (fxSmokeA24) |
| `800E7A0C` | GAME_STATE | coin flip increments car counter (+0x16) and sets car flag (+0x13) |
| `800E7A48` | VISUAL | particle X offset (stack only) |
| `800E7A58` | VISUAL | particle Y offset (stack only) |
| `800E7A68` | VISUAL | particle Z offset (stack only) |
| `800E7AB8` | VISUAL | particle type choice sub_801117B4 (fxExplosion96) |
| `800E7ADC` | VISUAL | particle X offset (dead/overwritten) |
| `800E7AEC` | VISUAL | particle Y offset (dead/overwritten) |
| `800E7AFC` | VISUAL | particle Z offset (dead/overwritten) |
| `800E7B70` | VISUAL | particle type sub_8011160C (fxExplosion24) vs sub_801116E0 (fxExplosion48) |

### `800E8A84` ai_mood_state_machine (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E8AD4` | GAME_STATE | AI state_time = attack_time*(rand&3) |
| `800E8C1C` | GAME_STATE | AI mood roll rand%100 |

### `800E97B0` ai_start_route_follow (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E9924` | GAME_STATE | discarded rand in AI route logic (advances shared stream) |

### `800E9A98` ai_decide_take_route (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800E9B04` | GAME_STATE | AI route decision (rand&15)==1 |
| `800E9B20` | GAME_STATE | AI route decision (rand&7)==1 |
| `800E9B8C` | GAME_STATE | AI route decision rand&1 |

### `800E9E20` sub_800E9E20 AI route step (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800EA130` | GAME_STATE | AI state choice ai+0x815 (6 vs 2) |
| `800EA14C` | GAME_STATE | AI state choice ai+0x815 = 5 |
| `800EA170` | GAME_STATE | AI state choice ai+0x815 (1 vs 2) |

### `800EA270` ai_free_steer (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800EA348` | GAME_STATE | AI steering choice (rand&3)==1 |

### `800EAB50` sub_800EAB50 (aiFollowRoute) AI follow route (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800EB4B0` | GAME_STATE | AI state choice ai+0x815 = 2 |

### `800EB714` ai_steer_ram_pass (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800EB90C` | GAME_STATE | AI pass decision rand%100 < pass_chance |

### `800ED128` car_ground_effects (mixed)

| Site | Class | Value feeds |
|---|---|---|
| `800ED72C` | GAME_STATE | deep water sinking: which front wheel level drops (wheel physics) |
| `800ED868` | VISUAL | selects wheel splash effect mode for sub_800ED920 (carWheelsSpawnFx) (particles only) |

### `800ED920` sub_800ED920 (carWheelsSpawnFx) wheel surface particles (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800ED934` | VISUAL | 1-in-5 gate for wheel particle emitters (sub_8011244C (fxSmokeC6)..sub_80112594 (fxSmokeC48)) |

### `800F2778` car_crash_impact (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800F29E4` | VISUAL | gates sub_80112C90 (fxImpactDebris) debris particle only |
| `800F2A00` | VISUAL | gates sub_80112528 (fxSmokeC24) effect sprite only |

### `800F2DF4` sub_800F2DF4 (carTumbleStep) (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800F3060` | VISUAL | gate sub_80112C90 (fxImpactDebris) effect |
| `800F307C` | VISUAL | gate sub_80112528 (fxSmokeC24) effect |

### `800F39D0` proximity_update (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800F3E54` | VISUAL | 1/16 gate for sub_80111538 (fxExplosion12) spark particle |

### `800F3F20` car_spark_emitter (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800F3F40` | VISUAL | spark position x jitter -> sub_800F3FB4 (fxRandomDebrisBurst) particle emitters |
| `800F3F58` | VISUAL | spark position y jitter -> sub_800F3FB4 (fxRandomDebrisBurst) particle emitters |
| `800F3F70` | VISUAL | spark position z jitter -> sub_800F3FB4 (fxRandomDebrisBurst) particle emitters |

### `800F3FB4` contact_sparks (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `800F3FC0` | VISUAL | spark count for sub_80112C90 (fxImpactDebris) |
| `800F3FD4` | VISUAL | particle type choice (sub_80112528 (fxSmokeC24)/80112100/801124BC) |

### `800F4CC0` sub_800F4CC0 (weaponSlamGrabUpdate) car special record step (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800F530C` | GAME_STATE | sets flag f0A in car record (+0x810 / +0x850) |

### `800F636C` sub_800F636C (startNodePickRandom) spawn point pick (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800F6380` | GAME_STATE | start index into spawn table word_801B2B28 |
| `800F64A4` | GAME_STATE | fallback random spawn entry word_801B2B28 |

### `800F6518` ai_pick_route (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800F655C` | GAME_STATE | AI route/path node choice (ai+0x53C path) |

### `800F763C` ai_pick_next_node (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800F76C0` | GAME_STATE | AI next route node choice (ai+0x466) |

### `800FD6C4` car_spawn_init (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800FD7B8` | GAME_STATE | spawn neighbour node choice -> initial car heading (+0x710) |

### `800FFEFC` sub_800FFEFC (aiCarStart) AI car (re)init (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `800FFF6C` | GAME_STATE | AI car field car+0x01 = rand%3 |

### `80103CA0` ai_weapon_fire_decision (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `80103D90` | GAME_STATE | AI fire roll rand/328 < aggression (+0x812) -> fire requests |
| `80103ED4` | GAME_STATE | AI rear-cone fire roll rand/328 < aggression |

### `801084D8` draw_tasers (weaponArcsDraw) (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80108768` | VISUAL | taser bolt end jitter X (draw primitives) |
| `801087A4` | VISUAL | taser bolt end jitter Y (draw primitives) |
| `801087E0` | VISUAL | taser bolt end jitter Z (draw primitives) |
| `801088D4` | VISUAL | taser bolt segment jitter (draw primitives) |
| `801088F4` | VISUAL | taser bolt segment jitter (draw primitives) |
| `8010890C` | VISUAL | taser bolt segment jitter (draw primitives) |
| `80108A9C` | VISUAL | 1/64 gate for sub_801124BC (fxSmokeC12) particle |
| `80108AC4` | VISUAL | 1/64 gate for sub_80111538 (fxExplosion12) particle |

### `8010A7A0` sub_8010A7A0 (weaponFlameUpdate) beam/stream emitter (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `8010A948` | VISUAL | 1-in-50 gate for sub_80112094 (fxSmokeA6) particle |

### `8010AED0` sub_8010AED0 (pickupRespawnTick) pickup respawn tick (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8010AF10` | GAME_STATE | pickup slot choice D_80183F7C (pickupSlots) (sub_8012493C (pickupSetVisible) respawn) |
| `8010AF74` | GAME_STATE | 50% gate for respawning pickup types 417/418 |

### `8010BDE8` random_offset_position (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `8010BE14` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BE28` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BE68` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BEB8` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BECC` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BF0C` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BF5C` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BF70` | VISUAL | random position offset for effect particles (all callers feed emitters) |
| `8010BFB0` | VISUAL | random position offset for effect particles (all callers feed emitters) |

### `8010C160` sub_8010C160 (groundFiresUpdate) burning-car update (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8010CABC` | GAME_STATE | human car hp (+0x1F0) burn damage gate |
| `8010CB7C` | GAME_STATE | AI car hp (+0x5A0) burn damage gate |

### `8010E040` sub_8010E040 (weaponTornadoUpdate) shield volume update (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `8010E158` | VISUAL | 1-in-10 gate for shield particles sub_801124BC (fxSmokeC12)/sub_80111314 (fxSparkOrange) |
| `8010E44C` | VISUAL | gate sub_80112310 (fxSmokeB12) particle (human) |
| `8010E80C` | VISUAL | gate sub_80112310 (fxSmokeB12) particle (AI) |

### `8010FD44` helicopter_boss_update (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8010FDA4` | GAME_STATE | frames until next shot (D_80180B4C (aiDarkToothShotDelay)) gating weapon launch sub_800D9D38 (weaponFireDarkToothMissile) |
| `8011017C` | GAME_STATE | frames until next swoop (D_80180F18 (darkToothSwoopDelay)) gating swoop attack/damage |

### `80111FB0` sub_80111FB0 (fxFlameStreamParticle) particle emitter (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80111FBC` | VISUAL | particle texture pick |
| `80112004` | VISUAL | particle vel x |
| `80112018` | VISUAL | particle vel y |
| `8011202C` | VISUAL | particle vel z gate |
| `80112044` | VISUAL | particle vel z |

### `80112678` effect_binding_80112678 (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `8011268C` | VISUAL | random palette entry for effect sub_80110918 (fxSpriteSpawn) |

### `8011270C` spawn_effect_sprite_random (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80112720` | VISUAL | random sprite frame (rand%6) for sub_80110918 (fxSpriteSpawn) effect |

### `80112C90` spark_debris_emit (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80112DB0` | VISUAL | spark X velocity (effect pool 80184E74) |
| `80112DC4` | VISUAL | spark Y velocity (effect pool) |
| `80112DD8` | VISUAL | spark Z velocity (effect pool) |
| `80112DEC` | VISUAL | spark sprite choice |

### `80114D08` hit_flash_tick (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80114D60` | VISUAL | 1-in-3 lighting table flicker on hit object (+4 = sub_80153E3C (lightGetEnv)) |

### `801163B8` GetNextTriggerPoint (worldCollapseUpdate) (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `801165D4` | GAME_STATE | chooses level trigger point to activate (sub_800F2420 (aiCarsRerouteFromSection)/80116F30) |
| `801169D0` | GAME_STATE | trigger platform motion target -> dword_8018744C -> sub_800F25F4 (worldSectionAddHeight) |
| `801169EC` | GAME_STATE | trigger platform motion target -> sub_800F25F4 (worldSectionAddHeight) |
| `80116A08` | GAME_STATE | trigger platform motion target -> sub_800F25F4 (worldSectionAddHeight) |
| `80116A1C` | GAME_STATE | trigger platform motion target -> sub_800F25F4 (worldSectionAddHeight) |

### `80116FF8` sub_80116FF8 (worldCollapseExplosions) building collapse explosions (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `80117104` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `8011717C` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `801171D4` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `80117284` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `801172DC` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `80117388` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `801173E0` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `8011743C` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `80117494` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `80117538` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `80117590` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |
| `80117638` | GAME_STATE | explosion position/height passed to sub_801176C0 (worldCollapseDebris), which can move a level event object (sub_80119468 (fxSpawnPooledBlast) -> sub_80124384 (objRegistrySetStep) event 686) and itself draws shared rand; conservative |

### `801176C0` ambient_explosion_fx (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `80117710` | GAME_STATE | 1/8 gate for positioned explosion sound emitter (sub_80119468 (fxSpawnPooledBlast), audio only) [review: positions a pooled explosion model via sub_80119468 / target event 686; VISUAL if those models have no collision] |
| `80117728` | GAME_STATE | Z offset of sound emitter position (caller stack vector) [review: positions a pooled explosion model via sub_80119468 (fxSpawnPooledBlast) / target event 686; VISUAL if those models have no collision] |
| `80117820` | GAME_STATE | effect type choice [review: positions a pooled explosion model via sub_80119468 (fxSpawnPooledBlast) / target event 686; VISUAL if those models have no collision] |
| `80117834` | GAME_STATE | effect type choice (sub_801119E4 (fxExplosionB48)) [review: positions a pooled explosion model via sub_80119468 (fxSpawnPooledBlast) / target event 686; VISUAL if those models have no collision] |
| `80117890` | GAME_STATE | effect Z offset [review: positions a pooled explosion model via sub_80119468 (fxSpawnPooledBlast) / target event 686; VISUAL if those models have no collision] |
| `801178B8` | GAME_STATE | particle type sub_80112594 (fxSmokeC48) vs sub_80112528 (fxSmokeC24) [review: positions a pooled explosion model via sub_80119468 (fxSpawnPooledBlast) / target event 686; VISUAL if those models have no collision] |
| `801178FC` | GAME_STATE | effect Z offset for sub_80111A6C (fxExplosionB96Fast) [review: positions a pooled explosion model via sub_80119468 (fxSpawnPooledBlast) / target event 686; VISUAL if those models have no collision] |

### `8011A024` round_reset (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011A0D0` | GAME_STATE | right after srand: gp+1496 = rand % car count (round setup) |

### `8011A700` world_update (mixed)

| Site | Class | Value feeds |
|---|---|---|
| `8011A828` | VISUAL | 1-in-5 radar blip draw (sub_800FC928 (hudRadarPlotCar) HUD, viewport 1) |
| `8011A8F0` | VISUAL | 1-in-5 radar blip draw (sub_800FC928 (hudRadarPlotCar) HUD, viewport 0) |
| `8011AFA0` | GAME_STATE | AI state byte_80188B5D 2 vs 3 and timer word_80188B6E |
| `8011B460` | VISUAL | 1-in-5 radar blip draw (sub_800FC928 (hudRadarPlotCar) HUD) |
| `8011B530` | VISUAL | 1-in-5 radar blip draw (sub_800FC928 (hudRadarPlotCar) HUD, 2nd viewport) |

### `8011EFDC` battle_round_respawn (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011F0A4` | GAME_STATE | subtracts rand&0x3F from AI car hp (D_80188B5C (aiCars)[i].hp) |
| `8011F0CC` | GAME_STATE | spawn slot choice (rand%4) selecting spawn table |

### `8011F284` spawn_table_8011F284 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011F310` | GAME_STATE | spawn table position variant rand&1 |
| `8011F3A0` | GAME_STATE | spawn table position variant rand&1 |
| `8011F434` | GAME_STATE | spawn table position variant rand&1 |
| `8011F4C8` | GAME_STATE | spawn table position variant rand&1 |

### `8011F510` spawn_table_level2 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011F5A0` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); kept with spawn logic, may be camera-only |
| `8011F62C` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `8011F6B8` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `8011F748` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |

### `8011F794` sub_8011F794 (demoSpawnParis) spawn table variant 3 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011F820` | GAME_STATE | spawn table selection (spawn_tables.c) |
| `8011F89C` | GAME_STATE | spawn table selection (spawn_tables.c) |
| `8011F91C` | GAME_STATE | spawn table selection (spawn_tables.c) |
| `8011F9AC` | GAME_STATE | spawn table selection (spawn_tables.c) |

### `8011F9F8` spawn_table_8011F9F8 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011FA84` | GAME_STATE | spawn table position variant rand&1 |
| `8011FB14` | GAME_STATE | spawn table position variant rand&1 |
| `8011FBA0` | GAME_STATE | spawn table position variant rand&1 |
| `8011FC20` | GAME_STATE | spawn table position variant rand&1 |

### `8011FC6C` spawn_table_level5 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011FCFC` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `8011FD90` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `8011FE20` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `8011FEAC` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |

### `8011FEF4` sub_8011FEF4 (demoSpawnAntarctica) spawn table variant 6 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8011FF84` | GAME_STATE | spawn table selection (spawn_tables.c) |
| `80120014` | GAME_STATE | spawn table selection (spawn_tables.c) |
| `8012009C` | GAME_STATE | spawn table selection (spawn_tables.c) |
| `8012012C` | GAME_STATE | spawn table selection (spawn_tables.c) |

### `80120178` spawn_table_80120178 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `80120200` | GAME_STATE | spawn table position variant rand&1 |
| `80120284` | GAME_STATE | spawn table position variant rand&1 |
| `80120314` | GAME_STATE | spawn table position variant rand&1 |
| `801203A0` | GAME_STATE | spawn table position variant rand&1 |

### `801203EC` spawn_table_level8 (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8012047C` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `80120504` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `80120588` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |
| `80120618` | GAME_STATE | coin flip writes player record D_80187B10 (humanCars)[0]+0x14 and D_80180FD4 (demoFollowPlayerCamera) (intro camera); may be camera-only |

### `8012305C` sub_8012305C (sceneCmdChoiceStepRun) TM2FourWay choice randomize (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `80123210` | GAME_STATE | random choice entry choice[7] (target/script selection) |
| `80123284` | GAME_STATE | re-roll choice entry while accept() rejects |

### `80125118` sub_80125118 (targetDamage) damage target (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `801256CC` | VISUAL | pick sub_801116E0 (fxExplosion48) vs sub_801117B4 (fxExplosion96) explosion particle (types 505-507) |

### `80125A18` target_actor_place_random (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `80125A94` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125AB4` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125B00` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125BB8` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125BD8` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125C24` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125CA4` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125D8C` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125DBC` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125E00` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125E80` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125F48` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125F68` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80125FB4` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80126058` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80126088` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `801260CC` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `8012614C` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `8012617C` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `801261C0` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80126278` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `801262AC` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `801262F0` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80126340` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `80126360` | GAME_STATE | target actor start row / position jitter (obj+168/172) |
| `801263AC` | GAME_STATE | target actor start row / position jitter (obj+168/172) |

### `80127398` target_debris_event_init (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `801273FC` | GAME_STATE | sign of debris rotation word_80190DD4 (passed to sub_8014B820 (debrisBurstAt)) |
| `80127410` | GAME_STATE | debris rotation word_80190DD4 (passed to sub_8014B820 (debrisBurstAt)) |
| `80127420` | GAME_STATE | debris rotation word_80190DD4 (negated branch) |
| `80127444` | GAME_STATE | sign of debris rotation word_80190DD6 |
| `80127458` | GAME_STATE | debris rotation word_80190DD6 (passed to sub_8014B820 (debrisBurstAt)) |
| `80127468` | GAME_STATE | debris rotation word_80190DD6 (negated branch) |

### `801274C0` sub_801274C0 (targetTimedEventUpdate) target event fuse effects (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80127570` | VISUAL | gate (rand % counter) for sub_8010BDE8 (mathJitterPos) particle |
| `801275CC` | VISUAL | pick particle pair (801117B4+801121D4 vs 80111888+8011223C) |

### `801277A8` sub_801277A8 (targetLevelEventUpdate) target event fuse effects (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80127880` | VISUAL | gate (rand % counter) for sub_8010BDE8 (mathJitterPos) particle |
| `801278DC` | VISUAL | pick particle pair (80111888+8011223C vs 801117B4+801121D4) |

### `80127AE4` sub_80127AE4 (targetPlayerEventInit) target event object init (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `80127BCC` | GAME_STATE | sign of launch angle word_80190E18 |
| `80127BE0` | GAME_STATE | launch angle word_80190E18 (event object record) |
| `80127BF0` | GAME_STATE | launch angle word_80190E18 (negated) |
| `80127C28` | GAME_STATE | sign of launch angle word_80190E1A |
| `80127C3C` | GAME_STATE | launch angle word_80190E1A |
| `80127C4C` | GAME_STATE | launch angle word_80190E1A (negated) |

### `80127CE0` target_burn_update (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80127EF0` | VISUAL | rand % rate gates sub_8010BDE8 (mathJitterPos) particle |
| `80127F50` | VISUAL | particle type choice |

### `8012A0E0` screenDrawCard (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `8012A22C` | VISUAL | menu card zoom wobble (v19[874]) in front-end draw |

### `8012CD08` random_car_options (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8012CD88` | GAME_STATE | random car/character selection for match options (rand%13) |

### `8012DE7C` demo_match_setup (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8012DECC` | GAME_STATE | random car (rand%13) for attract/demo match config |

### `8014B504` sub_8014B504 (debrisBreakModel) debris launch (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8014B62C` | GAME_STATE | debris piece velocity/time choice |
| `8014B760` | GAME_STATE | debris piece spin field +0x0E |
| `8014B774` | GAME_STATE | debris piece spin field +0x10 |

### `8014BAD4` debris_object_init (GAME_STATE)

| Site | Class | Value feeds |
|---|---|---|
| `8014BB70` | GAME_STATE | debris object piece velocity (object +28) |
| `8014BBC4` | GAME_STATE | debris object piece velocity (+40) |
| `8014BC18` | GAME_STATE | debris object piece velocity (+52) |
| `8014BC6C` | GAME_STATE | debris object piece velocity (+64) |
| `8014BC80` | GAME_STATE | debris object piece velocity jitter (+20) |
| `8014BC98` | GAME_STATE | debris object piece velocity jitter (+24) |
| `8014BCB0` | GAME_STATE | debris object piece velocity jitter (+32) |
| `8014BCC8` | GAME_STATE | debris object piece velocity jitter (+36) |
| `8014BCE0` | GAME_STATE | debris object piece velocity jitter (+44) |
| `8014BCF8` | GAME_STATE | debris object piece velocity jitter (+48) |
| `8014BD10` | GAME_STATE | debris object piece velocity jitter (+56) |
| `8014BD28` | GAME_STATE | debris object piece velocity jitter (+60) |
| `8014BDEC` | GAME_STATE | spin axis choice for debris object |
| `8014BE00` | GAME_STATE | debris spin rate (+178) |
| `8014BE20` | GAME_STATE | debris spin rate (+176) |
| `8014BE44` | GAME_STATE | debris spin rate (+180) |
| `8014BE68` | GAME_STATE | debris spin rate (+178, default case) |

### `80151BC4` sub_80151BC4 (animStepRandom) model animator random frame (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80151C00` | VISUAL | model animator frame choice; updated only from draw setup 80152284 -> sub_80151700 (animUpdateAll) (render path) |

### `80151CB8` sub_80151CB8 (animStepWeightedRandom) model animator weighted frame (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80151D30` | VISUAL | model animator weighted frame choice; render path (sub_80151700 (animUpdateAll)) |

### `80151DF4` sub_80151DF4 (animPickPeriod) model animator random period (VISUAL)

| Site | Class | Value feeds |
|---|---|---|
| `80151E24` | VISUAL | animator frame period (active range); render path |
| `80151EA0` | VISUAL | animator frame period (idle range); render path |
