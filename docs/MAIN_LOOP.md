# Main loop: update vs draw (for 60 fps)

Answers item 1 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`.
`rtMainLoop` (800D1318) is in C in `src/semantic/rt_main.c` (close match);
`viewProc` (80155E80) is in C in `src/semantic/view_system.c`.

## One iteration of `rtMainLoop`

The simulation runs once per loop iteration. The frame time is `D_801809E8` (rtFrameTime):
20 tics normally, 15 when `sub_8011EDAC() == 2` (gameGetMode).
`sub_8015531C()` (timerGetFrameTicks) returns ticks per frame (`D_8017EBF8` (timerFrameTicks), normally 3, which means 20 fps).

| # | Call | Kind | What it does |
|---|---|---|---|
| 1 | flip view-0 buffer, `ClearOTagR` | draw | picks the other of `D_8018118C[0..1]` (renderView0Frames), clears its ordering table |
| 2 | `sub_80150E00` (padUpdate) | **update** | pad poll (`controls.c`) |
| 3 | `sub_80126464` (targetActorsUpdate) | **update** | target actors (`target_actor_update.c`) |
| 4 | `sub_800DC348` (fxRecordsUpdate) | **update** | ticks the 16 effect records at `D_801A5B08` (fxRecords) (`weapon_fire_specials.c`) |
| 5 | `sub_8014B45C` (debrisUpdate) | **update** | debris/chunk physics: `sub_8014B940` (debrisUpdateChunks) (8 chunks), `sub_8014BF3C` (debrisUpdateBursts) (4 bursts); angles += spin, `RotMatrixYXZ`, position += velocity. In phase 2, `debrisUpdateBursts` adds the spin and rebuilds the matrix 4 times per step (see RENDER_ANSWERS.md R3) |
| 6 | `sub_80114B48` (lightFlickerUpdate) | **update** | light flicker timers (`misc_one_liners.c` → `sub_80114D08` (lightFlickerTick)) |
| 7 | `viewProc(0)` | draw | builds view 0's camera matrices from the camera object (see below) |
| 8 | `sub_800D8230(buf, 0)` (streaksDraw) | **mixed** | draws the 40 streak slots, FX sheet and tasers. It also calls `sub_80109D58` (weaponDropsUpdate), which **ticks** 25 timed-blend slots (state), and `sub_8010A7A0` (weaponFlameUpdate), which is update work: damage, ignition, spawns and `rand()`. `weaponArcsDraw` ticks the taser count, calls `carApplyDamage` (GAME_STATE `rand()` at 800E59C4) and calls VISUAL `rand()`. `fxSheetDisplay` counts down `fxSheetTimer` (see RENDER_ANSWERS.md R3) |
| 9 | `update_bullets` (bulletsUpdate) (800D87B4) | **update** | toggles `D_80180A38` (bulletsFrameParity), moves the 40 projectiles (`sub_800D7E30` (bulletsMove)), caches player 0's object |
| 10 | per arena (`sub_80129AF8()` (gameGetLevel) = level at settings +24) | **update** | level 2: `sub_800D4D58` (moscowEventsUpdate), `sub_800D6188` (moscowDishUpdate) (random event 0); level 1: `sub_800D5AF0` (laEventsUpdate) (random events 1–2), `sub_801274C0` (targetTimedEventUpdate); level 3: `sub_800D6800` (eiffelTowerUpdate); level 5: `sub_801277A8` (targetLevelEventUpdate); level 7: `sub_80127CE0` (targetPlayerEventsUpdate) |
| 11 | `sub_80110A40` (fxSpritesUpdate) | **update** | moves the 150 effect sprites at `D_80185144` (fxSprites) |
| 12 | `sub_80110BE4(buf, 0)` (fxSpritesDraw) | draw | draws those effect sprites |
| 13 | `sub_8014B484(buf)` (debrisDraw) | draw | draws the 4 debris bursts (`sub_8014C19C` (fxDebrisDraw)) |
| 14 | `sub_80152194` (renderFlipFrame) | step bookkeeping | flips the renderer's double-buffer flag (gp+996/+1000) and stores ticks per frame. It re-arms the animation guard, so it must run once per step, not per redraw (see RENDER_ANSWERS.md R3) |
| 15 | `sub_80152284(buf, 0)` (renderScene) | draw | the 3D scene: level and every object model (see below) |
| 16 | `sub_8011A700` (worldUpdate) | **update** | the world step: cars, AI, physics, camera targets (`world_update.c`) |
| 17 | view 1 (split screen) or rear-view mirror | draw | same draw calls for the second buffer: `viewProc(1)`, 800D8230, 8014B484, 80110BE4, 80152284. It runs after `worldUpdate` (#16), so it draws a state one world step newer than view 0 (see RENDER_ANSWERS.md R3) |
| 18 | `DrawSync`, `VSync` until the frame time has passed | pacing | |
| 19 | `rtUpdateFieldCounter`, `SetFrameStart`, `sub_80148B1C` (clutAnimUpdate) | timing / **update** | `sub_80148B1C` walks 1060-byte records at `D_80197FE8` (clutAnimRunning) by type 1/2/3. These are palette cycle, sweep and fade animations (10 records x 1060 bytes) uploaded to VRAM. Run it once per step (see RENDER_ANSWERS.md R3) |
| 20 | `PutDrawEnv`, `PutDispEnv`, `DrawOTag` | present | view 0, then view 1 / mirror |
| 21 | HUD: `sub_800F8370` (hudDrawSingleView) (1 view) / `800F9084` / `800F9D48` (split layouts) | draw + 3 once-per-step side effects | checked. The side effects are the radar swap 800FBDFC, `hudMessageUpdate` 800FBC44 and the `hudBlinkWeaponMarker` 800FB588 counter. No `rand()`, sound or pad calls (see RENDER_ANSWERS.md R3) |
| 22 | `sub_80129528` (pauseMenu) | input + draw | the pause menu (`screen_pause_menu.c` on the follow-up branch). It is a blocking loop that draws into the displayed buffer, not into the OT (see RENDER_ANSWERS.md R3) |

`sub_8011A700` (worldUpdate) is also called once before the loop. So retail's order is:
update everything except the world, then draw, then update the world.
Rotated by one call, that is a clean "update all, then draw all".

## Can update and draw be separated? Yes, with three caveats

The port can keep retail's 20 Hz iteration exactly as it is. For in-between
frames it re-runs only the draw calls (#1, 7, 12, 13, 15, 17, 20, 21) with
interpolated transforms. `renderFlipFrame` (#14) is step bookkeeping and
runs once per step, not per redraw (see RENDER_ANSWERS.md R3).
These things must not run twice:

1. **`sub_800D8230` (streaksDraw) mixes the two.** Its calls to `sub_80109D58` (weaponDropsUpdate) (ticks the
   blend slots) and `sub_8010A7A0` (weaponFlameUpdate) must run once per 20 Hz step. Split it in
   the port: run the ticks in the update phase and the draw part every frame.
   More ticks (see RENDER_ANSWERS.md R3):
   - `weaponArcsDraw` ticks the taser count, calls `carApplyDamage` (GAME_STATE `rand()` at 800E59C4) and calls VISUAL `rand()`;
   - `fxSheetDisplay` counts down `fxSheetTimer`;
   - all these ticks already run twice per step with split screen or the mirror.
2. **Shared `rand()`.** Some draw-side calls use `rand()` (the VISUAL entries in
   `docs/rand_callers.tsv`). Re-running them would advance the game's random
   stream and desync both the simulation and the tapes. Give visual callers
   their own stream, as the port already plans.
3. **HUD and pause.** Both are now checked (see RENDER_ANSWERS.md R3). The HUD
   has three once-per-step side effects (#21); its drawing can be redone.
   `sub_80129528` (pauseMenu) is a blocking loop that draws into the displayed
   buffer. Run it once per step, after the last presented frame.

## Where the camera matrix is set each frame

`viewProc(i)` (80155E80) works from the camera object of view `i`
(`D_80181110[i]` (viewCameraObjects); view mode `D_80181120[i]` (viewModes), 4 = rear view).

1. **Rotation matrices.** It builds the rotation matrices from the objects'
   angles: `RotMatrixYXZ(obj+160 → obj+128)` for the camera object and for its
   parent object (`*(obj+16)`).
2. **Camera matrix.** It composes those with the per-mode offset (table at
   `D_8017EC6C`, 48-byte rows) using `MulRotMatrix0` (80149970), and writes the
   result into the current camera record (`sub_801495F8()` (csGetRoot), words +5..+31).
   The translation is `-(camera position at +168/+172/+176)`, rotated.
3. **Saved copies.** It saves copies per view at `D_801A51A8[3*i]` (viewEyePositions) (translation)
   and `D_801A51C0[8*i]` (viewMatrices) (matrix).
4. **Projection.** It sets the projection: `sub_8014A274(H from D_8017EC4C)`
   and `sub_8014A25C(screen centre from D_8017EC5C/60 / 2)`. This is
   probably where widescreen (topic 5) hooks in.

For interpolation: before the extra `viewProc(i)` calls, put interpolated
angles (+160..+164) and position (+168..+176) on the camera object and its
parent, then restore them afterwards.

## Where object positions reach the GTE

All objects use one layout:

| Offset | Contents |
|---|---|
| +128 | 3×3 rotation matrix (`MATRIX`, short) |
| +160 / +162 / +164 | angles |
| +168 / +172 / +176 | world position |
| +32 / +64 / +96 | matrices the renderer fills in |
| +16 | parent |

- **Game code** sets the angles and position. For cars this happens in
  `sub_8011A700` (worldUpdate)'s car update; projectiles and debris do it in their update
  functions. Game code then calls `RotMatrixYXZ(obj+160, obj+128)`.
  This is not always so (see RENDER_ANSWERS.md R3):
  - car objects copy `body_m` with the body tilt into +128;
  - ricochet shots build it from yaw only;
  - several drawn transforms are type-5 model nodes written directly: car body
    nodes, part spinners, Mr. Slam's arm, the Eiffel tower, splash nodes and
    collapsing ice.
- **The renderer** `sub_80152284` (renderScene) (80152284) → `sub_801526CC` (renderWalkModel) (801526CC) is
  hand-written assembly. It uses the scratchpad at 0x1F800000 and dispatches
  model node types through a table.
  - At draw time it composes each object's matrix with its parent and the
    camera (`sub_8015338C` (renderMulTransMatVec), `sub_80152664` (renderPushMatrix)).
  - It loads the result into the GTE rotation and translation registers
    (`ctc2 $0..$4`, around 80152C5C).
  - It walks the level model (the camera record's +284) and every registered
    object (`D_801A4E1C` (renderWorldModels) list, objects added with `sub_80149444` (csListAdd) /
    `sub_80149604` (csSetDrawEnable)).
- **Bullets, streaks, effect sprites and particles** are drawn by
  `sub_800D8230` (streaksDraw), `sub_80110BE4` (fxSpritesDraw) and `sub_80112F6C` (fxImpactParticlesDraw) from their own position
  fields (60-, 48- and 36-byte records), using the per-view camera matrix.
  The 60-byte records are the machine-gun bullets (`bulletRecords`).
  `fxImpactParticlesDraw` is called from `fxSpritesDraw`.
  Missiles are csList objects drawn by `renderScene`.
  Debris chunks are objects; `debrisDraw` draws only the bursts
  (see RENDER_ANSWERS.md R3).

So for interpolation, every drawn transform comes from fields that game code
writes once per step: object +160..+176, +128 (not always rebuilt from +160;
see above), the type-5 model nodes, and the record positions of the effect
systems. A 60 fps renderer needs to:

1. **Save the previous step.** Keep each drawn object's previous-step
   +160..+176, and the previous positions of projectiles, sprites and debris.
   Only bullets already keep one (`bulletsMove` 800D7E30 saves the last
   position). Missiles keep no previous position (see RENDER_ANSWERS.md R3).
2. **Swap in interpolated values.** For each in-between frame, write the
   interpolated position and angles into those fields. Rebuild +128 with
   `RotMatrixYXZ`, interpolating angles through `BoundAngle` (mathBoundAngle).
3. **Draw and restore.** Run the draw calls, then put the real values back.

The HUD, `sub_80148B1C` (clutAnimUpdate) and `sub_8010A7A0` (weaponFlameUpdate)
are now checked. No arena event (#10) writes into the ordering table; the
Eiffel flash goes through `fxSheetStart` / `fxSheetDisplay` (see
RENDER_ANSWERS.md R3).
