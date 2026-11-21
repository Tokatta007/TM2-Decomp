# Online play: what rollback needs from the game

Answers request 44 of the port's `docs/DECOMP_REQUESTS.md` (2026-10-17, "questions first"); the support was built from
request 48 on (sections 10-12 and later).
The port's side: `docs/ONLINE_PLAN.md` (the plan), `src/net/rollback.h` (the engine's `save / load / advance /
checksum` callbacks, `RbInput` of 8 bytes per slot), `docs/HARDENING.md` 3-4 (items 9-26), `src/vplus/interp.c`
(the in-between frames) and `src/vplus/four.c` (views 3-4).

Background docs here: `MAIN_LOOP.md` (the loop's 22 calls), `RENDER_ANSWERS.md` R3 (what each call writes),
`RAND_CALLERS.md` (208 GAME_STATE and 90 VISUAL `rand()` sites), `HUMANS16_PLAN.md` (H2's input words, 2.5's
determinism rules, section 7's hash list), `AI_VS_AI.md` 6 (the AI update throttle), `SPU_VOICES.md`,
`CAR_GROUPS.md` 12 (car sounds).

File:line refers to this repository's `src/semantic/*.c` as of branch `decomp-docs-43` (f8a7667).
Confidence tags as elsewhere: CONFIRMED (read in the C), LIKELY (read in part), CHECK (to verify).

**Built so far:** stage 1 (`FIXED_TICKS`, `SEED`, `HOLD`, `PAUSE_PORT`; request 48): section 10, with the user's
decisions (20 steps a second in every mode, co-op included; retail's AI throttle). Stage 2 (`ONCE_PER_STEP`,
`ANIM_IN_STEP`, `VISUAL_RAND`: drawing independence): section 11, with the answers to request 48's questions (a) and
(b) (11.6: `csObjectPool` +0x14 has no logic reader; `debrisStartBurst` writes no packets, no scratch OT needed).
Stage 3 (`STEP_SPLIT` + `portRunStep`, `SOUND_GATE` + `portSoundReconcile`, `CLUT_SHADOW`, `rollback_regions.tsv`,
`portDmdMutableList`: re-runs): section 12, with question (c) (12.4: the port's high RAM listed as "no"). **Request 48
is complete.**
Request 51 (section 13): a car's world collision differed with what each machine drew. The cause is the level's
animation frames (hit volumes on DISH, Moscow) stepped by the drawing; stage 2's bit 3 `ANIM_IN_STEP` fixes it, and the
port had not set it (its options word was 0x1). No new bit; 11.6 (a) stands.
Requests 52 and 53 (section 14): with bit 3 set, a machine that joins mid-match still parts on DISH because the port's
own animator wrapper (interp.c `visual_rand`) draws the step's animation `rand()` from a host-side seed that is not in
the state: the port passes it through under bit 3 (14.4); nothing in the decomp's drawing steps an animation. Request
53's rollback pieces are request 48's stage 3, mapped in 14.6; check 8 now also proves every static symbol in a row.
Requests 54 and 55 (section 15): `portDmdMutableList` now walks every model file loaded for the match (the level
file's every root, every installed car file: `portDmdImages`), so the list is fixed per load; the drawing's, the
interrupt clock's and the sound side's words are out of the hash (`field` rows for the scene objects' per-view
fields; check 9); the follow object is the level's camera-attached model, the same everywhere;
`portRollbackEndDue()` peeks whether the next step ends the round (every end is a scheduled event).
Requests 56 and 57 (sections 16, 17): more words out of the hash, no code: an effect sprite's owner pointer (a stack
address in a re-run), each machine's pads and sound library words, the iteration's buffer flip (check 9 now counts
`rtMainLoop`'s own stores), and the cameras' scene objects, whose matrix only `viewProc` (the drawing) writes: a new
row kind `ptr` names them through their pointer arrays.
Request 59 (section 20): an arc's end pointer (a stack address in a re-run) out of the hash; a seeker's target is
always a car object's position written by the step (what can still part machines, and how the port can tell);
`portClutShadowCapture` empties the palette shadow first, so a round never uploads an earlier round's rows.

**The short version.**

1. A step is one iteration of `rtMainLoop` (`rt_main.c:121-225`). Its updates and its draws are interleaved, and
   four "draw" functions do update work. A port bit can reorder the iteration into "update everything, then draw".
   A `portRunStep()` that runs only the update half is then a small function (section 8).
2. **The step does not run at a fixed rate.** `SetFrameStart` measures real time (`timer_frame.c:66-80`), and the
   result (`timerFrameTicks` 3..5, `timerFrameRate`) scales the physics, the AI and the timers throughout. One slow
   frame on one machine is a desync. This is the first thing to fix (bit `FIXED_TICKS`).
3. **Inputs** already come through one path for humans with H2's `INPUT_ALL` (`portHumanPad` / `portHumanSteer`).
   Only meta reads are left in a step: the pause menu's START, the attract demo's any button, and the view hotkeys,
   which change the local view and the mirror.
4. **State** is the game's `.sdata` + BSS (about 0.5 MB), small parts of `.data`, `.port_bss`, and 1-3 KB of mutable
   model-node fields in the level's model data. There is no heap in battle, and no scratchpad or GTE state lives
   across steps.
5. **Draw-dependent logic.** The mines' and flames' ticks, the tasers' arcs (their damage too), and the screen flash
   run once per *drawn view*: twice with split screen or the rear-view mirror. Animations step inside
   `renderScene`. Draw-time `rand()` shares the game's stream. The AI throttle is *not* camera-dependent (it
   depends on distances only), so the port's `HARDENING.md` 18 can be closed for determinism.
6. **Loads are not deterministic.** There are three `srand(GetCurTics())` sites, `rand()` during the load,
   `timerFrameCount` counts since boot, and modes 3/4 carry state across rounds. So a machine that joins late
   needs a full snapshot (compressed, a few hundred KB), not a diff against its own fresh load.
7. **Clocks.** `GetCurTics` reaches the logic only through the seeds and the frame-time measurement. The CD reaches
   it only through the Dark Tooth music's track-ended flag. The memory card is never read in a battle.

---------------------------------------------------------------------------------------------------------

## 1. One step

### 1.1 Where a step begins and ends

`rtMainLoop` (`rt_main.c:93`). Before the loop: `worldUpdate()` once (`:109`), `animEnable()` (`:114`), and
`rtFrameTime` = 15 in game mode 2 (co-op), else 20 (`:115-118`). One iteration (`:121-225`):

| line | call | kind | in `portRunStep`? |
|---|---|---|---|
| 122 | `rtUpdateScheduledEvents()`: counts the 4 event timers down by `timerGetFrameTicks()`; one that expires ends the loop (`:257-278`) | **state** | yes, first |
| 123 | `GetCurTics()` (result unused) | none | no |
| 127 | `portPacketBuffersApply` (FAR_DISTANCE) | draw | no |
| 129-134 | buffer flip, `ClearOTagR` | draw | no |
| 135 | `padUpdate()`: `padDecode` + latch (`controls.c:154`, `:224-236`; repeat mode 1 in battle, set at `:100`) | **input** | yes (or the port writes the words) |
| 136-139 | `targetActorsUpdate`, `fxRecordsUpdate`, `debrisUpdate`, `lightFlickerUpdate` | state | yes |
| 140 | `viewProc(0)` | draw (its outputs are read by the sound listener and `debrisStartBurst` during the update: RENDER_ANSWERS R3) | camera only, see 1.3 |
| 141 | `streaksDraw(buf, 0)` | **mixed**: `weaponDropsUpdate` 80109D58, `weaponFlameUpdate` 8010A7A0 (damage, ignition, `rand()`), `weaponArcsDraw` (taser count, `carApplyDamage`, `rand()`), `fxSheetDisplay` (`fxSheetTimer`) | the ticks only |
| 142 | `bulletsUpdate` | state | yes |
| 143-156 | per arena: Moscow, LA, Paris, levels 5 and 7 events | state (draw nothing: RENDER_ANSWERS R3) | yes |
| 157 | `fxSpritesUpdate` | state (effects; see 4.5) | yes |
| 158-159 | `fxSpritesDraw`, `debrisDraw` | draw | no |
| 160 | `renderFlipFrame` (re-arms the animation guard) | **step bookkeeping** | yes |
| 161 | `renderScene(buf, 0)` | draw, **plus `animUpdateAll`** at its end (`engine_render_scene.c:263`) | `animUpdateAll` only (4.4) |
| 162 | `worldUpdate()` (`world_update.c`) | **state**: cars, AI, weapons, pickups, sound (`soundUpdate`/`soundFlush` `:552-553`), the radar plots | yes |
| 164-188 | view 1 (split) or the mirror: `viewProc(1)`, `streaksDraw(.., 1)` (**the ticks again**), `debrisDraw`, `fxSpritesDraw`, `renderScene` (`animUpdateAll` again; guarded) | draw + ticks | no (ticks: see 4.2) |
| 190-193 | `DrawSync(0)`; `VSync(0)` until `rtFrameTime` has passed (`timerTicsToFrameRate`, `timer_frame.c:92`) | pacing | no |
| 194 | `rtUpdateFieldCounter(timerGetFrameTicks())`: `rtFieldCounter`, read by nothing in battle (CHECK closed, 14.6: its getter `rtGetFieldCounter` 800D19D4 has no caller and no pointer; the other access is its reset `game_misc_accessors.c:30`) | state (dead) | yes |
| 195 | `SetFrameStart()`: **measures the frame**, sets `timerFrameTicks` / `timerFrameRate` / `timerLastFrameDelta`, `timerFrameCount++` (`timer_frame.c:66-80`) | **state** | yes, with fixed ticks (bit 0) |
| 196 | `start = GetCurTics()` | pacing | no |
| 197 | `clutAnimUpdate()`: 10 palette records at `clutAnimRunning` (80197FE8, 1,060 bytes each), tick and `LoadImage` | **state + VRAM** | yes (its upload: 1.3) |
| 198-200, 208-223 | `PutDrawEnv` / `PutDispEnv` / `DrawOTag` | present | no |
| 201-219 | HUDs: three once-per-step side effects: the radar swap 800FBDFC, `hudMessageUpdate` 800FBC44, the `hudBlinkWeaponMarker` 800FB588 counter (RENDER_ANSWERS R3) | draw + 3 ticks | the 3 ticks |
| 224 | `pauseMenu()` (`screen_pause_menu.c:554`) | meta, blocking | no (6) |

**The step boundary for the port** is the top of the iteration (`:122`). Everything from `rtUpdateScheduledEvents`
to the HUD's ticks is one step, and the state at `:122` is "the state at the start of step f". When the loop
returns (an event fired at `:122`), the round is over. The port sees that as `rtMainLoop` returning to
`sessionRun` (`session_runner.cpp:151`), and every machine returns at the same step because the event timers are
state.

**The order is retail's, not "update then draw".** View 0 is drawn *between* the updates (`:140-161`, before
`worldUpdate`). View 1 and the mirror are drawn after it. With the step pulled out (8.2), the draws run after the
whole step: view 0 shows its state one world step fresher than in retail. This is visual only. RENDER_ANSWERS R3
(Recommended split, item 6) already says so, and interp.c's poses already allow for it.

### 1.2 What a step touches outside RAM

**GPU (VRAM, the OT, packets).**

| where | what | in a re-run |
|---|---|---|
| `clutAnimUpdate` 80148B1C (`engine_clut_anim.c`), `rt_main.c:197` | `LoadImage` of each running palette record (cycle, sweep, fade) | skip the upload; after the batch, upload every record from RAM once (bit `CLUT_SHADOW`, 8.1) |
| `debrisStartBurst` 8014BAD4 (`engine_clut_anim.c:1108`; called from `:952`, `:1102`, from the update) | `renderDrawObjectUnculled(obj)` (`:1146` → `engine_anim_hit_volumes.c:344`): walks the object's model with the camera that `viewProc` last set and adds its polygons to the burst (`lightAddSources`, `renderCtx->ot`). It is the one render-from-update. It can also reach `animUpdateOne` | the burst's captured polygons depend on the local camera, so they are presentation (bursts have no collision: RAND_CALLERS "Entries to review"). In a re-run, let it run into a scratch OT, or keep it as is and exclude `fxDebris`' burst slots (4 x 0x228 at 801A3E70) from the hash. Its `rand()` calls (`:1130-1135`) are on the shared stream and run once per burst on every machine: deterministic. CHECK closed (11.6 (b)): it writes no packets and no OT entries, so 8 re-runs add 0 bytes |
| `fxSheetStart` 800DC45C | fills `fxSheetPolys` and sets the timer (doubled when split: `weapon_fire_specials.c` comment before 800DC45C) | RAM only; the timer is presentation (4.2) |
| attract demo only: `worldUpdate`'s blinking "DEMO" text `fontPrintCenteredXY` (`world_update.c:326-328`) | font prims | never online |
| the HUD's radar plots `hudRadarPlotCar` / `portHumanRadarPlot` (`world_update.c:161`, `:212`, and the plot sites `:207`, `:211/218`, `:415-431`) | the radar's back buffer, in RAM; swapped by the HUD (800FBDFC) | **the swap must run once per step** (with the step), or the re-runs pile N steps of blips into one buffer |

No `DrawSync`, `VSync`, `ClearOTag`, `DrawOTag` or `PutDrawEnv` is reached from the update calls. The arena events
draw nothing (RENDER_ANSWERS R3). CONFIRMED for the calls in rt_main and the events. LIKELY for the whole
`worldUpdate` tree: `DrawSync` and `LoadImage` appear in no unit it reaches apart from the two above.

**SPU (sound).** Logic never talks to the SPU directly. Everything goes through the sound manager object
(`soundManagerObject`) and its wrappers in **`command_wrappers.cpp:20-33`**: `soundStartPlayerLoop` 800D3244,
`soundStopPlayerLoop` 800D328C, `soundStartSharedLoop` 800D32BC, `soundStopSharedLoop` 800D3304, `soundPlay`
800D3334, `soundPlayVariant` 800D3398, `soundPlayVariantEx` 800D33FC, `soundStop` 800D3464, `soundStopInstance`
800D3490, `soundFlush` 800D34E8. On top of these come `soundUpdate` (`timer_frame.c:106`, `taskMgrRunFrame`) and
`soundFlush`, called once per step at `world_update.c:552-553` (the request queue is run and flushed there), plus
the port's car-sound layer (`portCarEngineUpdate`, `portCarFlameSound`, `portCarSoundLoops`: CAR_GROUPS 12).

- **Read back by the logic?** `soundIsActive(id)` (`weapon_fire_specials.c:127`) is *not* SPU status. It returns
  game-side flags `soundActiveIdNN` that the logic sets itself (`:159`). Its readers (`weapon_fire_specials.c:381`:
  stop sound 51 when no projectile 51 is left; `screen_card_draw.c:151`: the engine loop; the port's car-sound code
  `port_variant_arrays.c:3129`, `:3197`, `:3625`) only decide whether to stop a sound. These flags are in BSS, so
  they are snapshotted with the state and stay deterministic. No logic waits for a sound to finish. CONFIRMED for
  the readers listed, LIKELY that there are no others.
- **Camera-dependent sound parameters:** `PadChkMtap()` is a misnamed accessor of `sndListenerDist`
  (`audio_hud_helpers.c:15`), and `soundGetListenerPan()`, `soundLocateForView0/View` and `viewGetSplitMode()`
  choose the sound's volume and pan (e.g. `car_destroyed.c:366-375`, `car_parts_spin.c:184-190`,
  `fx_debris_splash.c:460-468`, `car_crash.c:100-106`, `ai_link_node_pick.c:316-323`, the `HIT_SOUND` macro
  `projectile_hit.c:90`). All of them reach only the sound's parameters, not state. CONFIRMED for these sites.
- **The sound manager's own memory** (`sndObjPool`, the task lists, the instance pool: SPU_VOICES.md, CAR_GROUPS
  12) is presentation. It must be **left out of the snapshot and never restored**. Restoring it would cut off
  sounds that are playing now or replay old ones.
- **In a re-run (bit `SOUND_GATE`, 8.1):** while `portResimulating` is set, the wrappers above and
  `soundUpdate`/`soundFlush` return at once (0, or -1 where a caller tests the result: CHECK the callers of the
  return values; most discard them; closed in 12.2: every caller in the C discards it, the wrappers return 0). The first run's sounds stay. A one-shot that only a corrected re-run would have
  started is lost, and a one-shot of a mispredicted step stays (the usual rollback compromise; 20 Hz and short
  windows keep both rare). **Loops** (engine, turbo, gun, burn, flame: `soundStart*Loop` / `soundStop*Loop` and the
  port's `portCarSoundLoops` bits) heal themselves on the next live step: the engine update re-issues its sound
  every step (`screen_card_draw.c:147-152`), and the stop sites test state every step (`world_update.c:548-550`,
  `weapon_fire_specials.c:381`). The one loop that could stick is a loop started in a mispredicted step whose stop
  site only fires on a transition. The port reconciles those after the batch: for each loop bit in
  `portCarSoundLoops` / the shared loop 9, compare it with the state's cause and stop or start it (8.3).

**CD.** No CD read in a battle step. The music is CD audio / XA, run by the music manager and `cdAudio*`
(`engine_sound_lists.c:675-681`), and the logic reads it in one place: `worldUpdate` → `if
(audioGetDarkToothMusic() && audioTakeTrackEnded()) audioAlternateMusicTrack();` (`world_update.c:545-546`;
`misc_one_liners.c:390-396`; `weapon_special_missiles.c:96-111`). The effect is music only (`audioAltMusicMode`
10/11 and the serial volume), but it consumes a flag set by the CD player and toggles a byte in RAM. In a re-run,
skip the test (it is consumed by the live run only), and keep `audioAltMusicMode` out of the snapshot (or never
restore it). CONFIRMED.

**VSync / timers in a step.** No `VSync` and no `GetRCnt` in the update path. There are `GetCurTics()` calls with the
result discarded (`rt_main.c:123`, `camera.c:52/78`, `car_tick.c:101-103`, `world_update.c:547/557`: leftover
profiling, harmless). The clocks the logic does use are `timerGetFrameTicks` / `timerGetFrameRate` (measured: 4.6)
and `GsGetWorkBase_0()` = `timerFrameCount` (`timer_frame.c:87`; parity tests `ai_steps.c:421`,
`car_contact_resolve.c:362`, `car_crash.c:367`, `car_motion.c:361`, `car_spin.c:147`, `proximity_update.c:76`,
`car_wheel_sway.c:189/201`). `timerFrameCount` is incremented only by `SetFrameStart` (`timer_frame.c:79`) and
zeroed only at boot (`InitTimer` `:46`, from `shellInitMachine` once). So it counts battle steps since power-on,
attract demos included (5).

**Interrupts.** `portRunStep` itself waits on nothing. Whatever the port's runtime delivers between calls (the
VBlank handlers, the root-counter event `SysClkIntHandler` `timer_frame.c:29`, the BIOS pad buffers) is the
port's. It should deliver none of them inside a re-run batch. CHECK on the port side (`runtime/core.c`
`exception()`).

### 1.3 What to skip or defer in a re-run (summary)

| item | live step | re-run (`portResimulating`) |
|---|---|---|
| updates (1.1 "yes" rows) | run | run |
| `viewProc` before the update (sound listener, `debrisStartBurst`'s camera) | the real camera (interp.c restores it already) | leave the last live camera in place; its readers are presentation |
| sound wrappers, `soundUpdate`/`soundFlush` | run | return at once |
| `audioTakeTrackEnded` test | run | skip |
| `clutAnimUpdate`'s `LoadImage` | run | skip; one upload of all 10 records after the batch |
| `debrisStartBurst`'s render | run | run as is (11.6 (b): no packets, no OT entries) |
| radar plots + swap | in the step | in the step (the plots are cheap), or skip both |
| HUD message timer, weapon marker blink | in the step | in the step (they are state-like timers, but render-only: exclude them from the hash) |
| draws, present, HUD drawing, pause | per frame | never |

---------------------------------------------------------------------------------------------------------

## 2. The inputs

### 2.1 Every read of the controls in a battle

The BIOS pad buffers (`padRecvBufferPort1/2`) are filled by the port's runtime (`runtime/hle.c` `update_pads`,
VBlank) and decoded by `padDecode` (`shared_pad.c:38-55`) into `padButtonsDecoded` (pad 0 low half, pad 1 high).
`padUpdate` (`controls.c:154`) latches them (repeat mode 1 in battle: `rt_main.c:100`, so the `GetCurTics`-based
repeat at `controls.c:237-271` is not used in battle). The readers in a step:

| site | reads | kind | with H2 `INPUT_ALL` (bit 0) |
|---|---|---|---|
| `car_input.c:58/60` `ACTION(a)` in `humanCarReadInput` / `carReadMoveCodes` (all car controls, move codes, weapons) | `portHumanAction` → `padCheckAction` or `portHumanPad[h]` (`port_variant_arrays.c:7016`) | **state** | the word: covered |
| `car_tick.c:40-41` cheats `CHEAT_COMBO` / `CHEAT_ANY_LATCHED` in `humanCarTick` | `portHumanCombo` / `portHumanAnyLatched` (`port_variant_arrays.c:7037`, `:7063`) | state (off online: bit 3 NO_CHEATS) | covered |
| `car_records.c:190` `portHumanCombo(1, ..)` (extra views unlock) and the view hotkeys (`car->key_*`, `:160-187`, derived from `ACTION`) in `carViewHotkeys` | the word | **presentation**, but it toggles globals: `viewExtraViewsUnlocked`, `rtToggleRearView` (`:182`), `hudToggleRadar/WeaponList` | covered for the read; the *effect* is local (4.2, 2.3) |
| analog steering: `portHumanSteer[h]` (`port_variant_arrays.c:966`), read by the port's hook around `carThrottleSteer` (ANALOG_STEERING.md) | the port's | state | covered (the port writes it per step) |
| `world_update.c:348` attract demo: `padAnyPressed(0)` → `rtScheduleEvent(1, 0)` | pad 0 directly | meta (ends the demo) | **not covered**; never online (no demo in a match) |
| `screen_pause_menu.c:575-578` `pauseMenu`: `padCheckAction(8, 0)` / `(8, 1)` with split | pads 0/1 directly; also writes `padActionCache` | meta (blocks the loop) | **not covered**: online needs its own pause (6) |
| `padUpdate` itself (`rt_main.c:135`) | the BIOS buffers | writes `padButtonsLatched`, `padActionCache` | the latch is state only for pads 0/1 in retail mode |
| AI cars | **never read a pad**: their "brain" is `aiCarUpdate` 8011C4A0 (`world_update.c:450-458`, `ai_steps.c`, `ai_driver.c`), deterministic from state and the shared `rand()` | state | n/a |

Not in a step (menus, run outside `rtMainLoop`): `movieSkipCheckDelayed` (`ai_route_helpers.c:216-227`),
`menuPasswordCursorRun` (`car_destroyed.c:219-275`), `padWaitConfirmOrTimeout` (`car_parts_spin.c:245-272`),
`sessionRun`'s waits (`session_runner.cpp:110-111`). These are the round-end and between-round screens. Online,
the port runs them per machine or skips them; they are not simulation.

### 2.2 One place where the port writes every slot's input

**Yes, with H2's `INPUT_ALL` (`portHumanOptions` bit 0).** Every human's car reads only `portHumanPad[h]` (the 16-bit
decoded half word), `portHumanSteer[h]` (0..4096) and its own bindings (`portHumanBindings[h]`, built from
`portHumanLayout[h]`). The port writes those for `h < humanCarTotal` before the step (HUMANS16 7, "Common setup").
AI slots need no input. A slot that is an AI car on every machine is the neutral input (`RbInput` all zero) and is
ignored.

`RbInput` (8 bytes) fits: `portHumanPad` (2 bytes) + `portHumanSteer` (2 bytes) + 4 spare bytes (e.g. a "pause
request" or "leave" bit, a layout change).

What still reads a pad directly in a step, and the fix for each:

1. **`padUpdate`** still decodes pads 0/1 into `padButtonsDecoded` / `padButtonsLatched` / `padActionCache`. With
   `INPUT_ALL` no game-state reader uses them (CHECK closed: the addendum and 14.6, `padActionCache` has no reader but
   `padCheckAction`'s unreachable branch). In
   `portRunStep`, call `padUpdate` only on a live step (or not at all) and **exclude** these three words and the
   repeat counters (`padRepeat*`, `controls.c:240-271`) from the hash. Restoring them does no harm.
2. **Pause** (`screen_pause_menu.c:575`): under bit `PAUSE_PORT` (8.1) `pauseMenu` returns at once; the port opens
   its own online menu (no hold), or requests a hold for everyone (6).
3. **Attract demo** (`world_update.c:348`): never online.
4. **Hotkeys** (`car_records.c:160-195`): with H1, `carViewHotkeys` returns at once for a remote human. A local
   human's hotkeys change only presentation, **except the rear-view mirror** (`rtToggleRearView`), which doubles the
   per-view ticks (4.2). With `ONCE_PER_STEP` (bit 2) the mirror no longer matters. Until then, the online build
   must turn the mirror off.

Write the words **before `portRunStep`**, never in the middle of a step: `humanCarReadInput` runs inside
`worldUpdate`, but the cheats and hotkeys read the same words later in the same step.

### 2.3 Input-derived state that lives across steps (in the snapshot)

`carViewHotkeyReleased[]` (per human), the move-code buffers in each human record (`carReadMoveCodes`), the cheat
progress (in `humanCarTick`'s record fields), `viewExtraViewsUnlocked`, and `padActionCache`. All of them are in
BSS / `.port_bss` and are covered by the region list (3.4). The presentation ones (`carViewHotkeyReleased`, the
camera block, `view`/`view_last`) are already on HUMANS16 7's "never hashed" list.

---------------------------------------------------------------------------------------------------------

## 3. The state

### 3.1 RAM regions

- **No heap in battle.** Nothing calls `malloc` or `InitHeap` during a battle. Arena assets go into fixed load areas
  at arena load (`asset_pools.c`, `bitmap_storage_setup.c`, the level model / DMD buffers), and the pools are fixed
  arrays in BSS (`csObjectPool` 128 x 0x128 at 8019A950, projectiles, effects, drops).
- **`.sdata` + `.sbss` + `.bss`: `0x801809D4 .. 0x801FAD48`** (0x7A374 = **~489 KB**) in the retail-layout build.
  Most of a step's state is here: the car records, `aiCars`, the object pool, the pools, `rtScheduledEvents` (at
  the start of this range: `D_801809D4`), the `rand()` seed, the HUD and sound state (to be excluded, 3.3).
  The port variant moves arrays (`port_variant_arrays.tsv`) and adds `.port_bss` (0x80200010.., size per build in
  `symbols.tsv` / the json). Take both ranges from the build's map, not from these numbers.
- **`.data` parts that a step writes:** the timer globals (`timerFrameTicks`, `timerFrameRate`, `timerFrameCount`,
  `timerLastFrameDelta`, `timerFrameStart`, `timerPrevFrameStart`, `timerTicsAccum`: `#pragma tm2_absolute`,
  `timer_frame.c:4-26`) and the sound globals near `soundManagerObject`. These sit in initialized `.data`, outside
  the BSS range. The port needs a list of them from the map. Proposal: the decomp adds a generated
  `rollback_regions.tsv` per build (8.4).
- **`.port_bss`**: all of it (the H2 words are inputs, rewritten each step; harmless to restore).
- **The scratchpad (0x1F800000):** used by the renderer (`renderWalkModel`, asm) and by math helpers as scratch
  within one call. **Nothing a later step reads is kept there.** Not in the snapshot (the port's interp.c already
  saves and restores it around its redraws for its own reasons).
- **GTE registers:** the logic sets the rotation / translation matrices before each use (`RotMatrixYXZ`,
  `SetRotMatrix` / `SetTransMatrix` sequences) and **assumes nothing from a previous step**. Not in the snapshot.
  The one cross-boundary use is the projection set by `viewProc`, read by `debrisStartBurst`'s render
  (presentation, 1.2).
- **Root counters, `VSync(-1)`, `GetRCnt`:** only through `GetCurTics` (`timer_frame.c:59`), and that only in
  `SetFrameStart` and the seeds (7).

### 3.2 The level's model data: what changes in a battle

The level's DMD (model) data is loaded once per arena (≤ 539 KB) and is mostly fixed: geometry, textures (VRAM),
routes and AI nodes, collision. A step writes these node fields:

| node type | mutable bytes | written by |
|---|---|---|
| 3 (animation) | `[+6, +0x12)` | `animUpdateAll` / `animUpdateOne` (cell, timer, `done[2]`) |
| 9 (select) | `+7` | tire switches (`uaswToggleCarTireState` 80124800), damage stages, frozen model |
| 1 | `+0x14` | |
| 4 | `[+4, +0x13)` | |
| 5 (matrix) | `[+4, +0x27)` | car body nodes, part spinners, Mr. Slam's arm, the Eiffel tower, splash nodes, collapsing ice (MAIN_LOOP "Where object positions reach the GTE") |

That is **about 1-3 KB** spread over the level's model. The port can snapshot a precomputed list of
`(address, length)` fields made by walking the loaded DMD after the arena loads. The fallback is the whole level
DMD (≤ 539 KB). Car models are loaded per car group (CAR_GROUPS) and have the same node types, so the same walk
covers them. A drop-in changes them, which changes the list: re-walk after every `portCarStartHuman` /
`portCarRemove`.

### 3.3 Left out of the snapshot or the hash

| what | snapshot | hash | why |
|---|---|---|---|
| sound manager object, pools, instance table, listener (`sndListener*`), `audioAltMusicMode`, the music manager | **no** (never restore) | no | presentation; restoring cuts or replays sounds |
| OTs and packet buffers (`renderView0Frames` records' OT/packets, `renderPacketBuffer0`, the port's HI_OT / HI_PACKETS) | no | no | rebuilt every frame |
| camera globals (`viewEyePositions`, `viewMatrices`, `viewCameraObjects`, `cameraPrevPos`, `viewCameraLockFrames`), each human record's camera block (+0x006, +0x00C..+0x08F, +0x10C, +0x110) | yes (harmless) | **no** | local (HUMANS16 6.4) |
| HUD: radar buffers, `hudMessage*`, the weapon marker blink, `hudSmoothedSpeeds`, toggles | yes | no | presentation |
| `padButtons*`, `padActionCache`, `padRepeat*` | yes | no | inputs |
| `csObjectPool` +0x14..+0x1C, +0x20/+0x40/+0x60, +0x124 (`csUpdMat`, written by the draw) | yes | **no** | depend on the views drawn (RENDER_ANSWERS R3 "Restore after the redraws") |
| type-3 animation fields (3.2) and `renderFrameParity` / `renderPrevFrameParity` | yes | no, until bit `ANIM_IN_STEP` (4.4) | culled nodes step or not |
| debris burst slots (801A3E70, 4 x 0x228) | yes | no | captured with the local camera |
| `fxSheetTimer`, `fxSheetPolys` | yes | no | presentation (4.2) |
| `rtFieldCounter` | yes | no | not read |
| `portRenderStats` (the render capacity counter, `RENDER_ANSWERS.md` R2.4) | **no** | **no** | observation only: written by `renderScene` per view drawn, read by nothing in the game |

### 3.4 Proposed snapshot list (retail-layout sizes; take the real ones from the build)

| region | size | per step? |
|---|---:|---|
| `.sdata`/`.sbss`/`.bss` 0x801809D4..0x801FAD48 minus the OT/packet buffers and the sound pools | ≤ 489 KB (less the excluded parts) | yes |
| `.data` timer globals + the logic's sound flags (`soundActiveIdNN` if in `.data`) | < 1 KB | yes |
| `.port_bss` | 0x5984..~0x27000 (per build: e.g. ~27 KB for `TM2_PORT_HUMANS=16`, HUMANS16 3) | yes |
| level + car DMD mutable fields (3.2) | 1-3 KB | yes |
| the port's own registry (`vp_keep_register`, `feats_keep_register`, HARDENING 3) | port | yes |

About **0.5 MB per step**. 8 steps cost 4 MB, and a copy at 20 Hz is cheap. With a dirty-page diff (ONLINE_PLAN 2)
it is much less: most of BSS (the load-time tables) does not change in a step.

---------------------------------------------------------------------------------------------------------

## 4. Determinism: where the logic depends on what this machine draws, or how often

### 4.1 The AI "unseen car" throttle: not camera-dependent

`aiCarShouldUpdate` (`fx_explosions.c:216-236`) skips an AI car's update when `f00C != 0 && f039 == 0` and its
target is more than 4800 away or behind it (`abs(f024) > 1024`). `f00C` is the director's skip counter
(`world_update.c:450-458`; AI_VS_AI.md 6). **It reads no camera, view or visibility flag.** It is deterministic from
the state. The on-screen flag `+0x124` (`csUpdMat`) has no game reader (RENDER_ANSWERS R3). So the port's
`HARDENING.md` 18 is not a desync. It is a gameplay choice: retail on, the port's `vp_ai_vs_ai`-style off. Online,
every machine must make the same choice (part of the ruleset the host sends). CONFIRMED.

### 4.2 Per-view ticks (the port's HARDENING 19)

`streaksDraw` is called once per drawn view: view 0 (`rt_main.c:141`) and view 1 / the mirror (`:172`, `:184`).
The port's views 3-4 call it with `streaks_draw_only` (`four.c:404-447`). Each call runs:

| tick | state it changes | 1 view → 2 views |
|---|---|---|
| `weaponDropsUpdate` 80109D58 (`fx_sprite_spawn.c`) | 25 `weaponDrops` timers (mine fuses, drop animations), object release | **2x speed** |
| `weaponFlameUpdate` 8010A7A0 | flame damage, ignition, spawns, GAME_STATE `rand()` | **2x damage and `rand()` calls** |
| `weaponArcsDraw` (tasers, lightning) | taser count, `carApplyDamage` (GAME_STATE `rand()` at 800E59C4), VISUAL `rand()` jitter | **2x damage**; SUDDEN_DEATH.md: lightning credit too |
| `fxSheetDisplay` 800DC590 | `fxSheetTimer` countdown (the start doubles it when split: `fxSheetStart`) | balanced by the doubling; presentation |

The mirror is toggled by a local hotkey (`car_records.c:182`), so in retail two machines with the same match can
differ just because one player looks back.

**Bit `ONCE_PER_STEP`:** `streaksDraw` gets a "draw only" flag (as the port's `four.c` already does by hook). The
step runs the three ticks **exactly once**, in canonical order (after `debrisUpdate`, where view 0's call stood),
whatever the number of views. `fxSheetStart`'s split doubling goes away under the bit (the countdown is once per
step). This changes 2-player split from retail: mines and flames tick at the single-view rate. Online that is the
only consistent rule. Question 2 asks whether offline split should keep retail's rate.

### 4.3 `rand()` from drawing code

| site | runs | per view? | fix |
|---|---|---|---|
| animators `animStepRandom` 80151BC4, `animStepWeightedRandom` 80151CB8, `animPickPeriod` 80151DF4 (`rand()` at 80151C00, 80151D30, 80151E24, 80151EA0) | `animUpdateAll` at the end of `renderScene`; `animUpdateOne` from the walker for nodes that pass culling | once per step (guarded by the parity), **but only for culled-in nodes** | the port's visual stream (`interp.c:511` `visual_rand`) already; bit `VISUAL_RAND` makes it the decomp's (8.1) |
| `weaponArcsDraw` jitter (VISUAL) | per view | yes | visual stream, plus `ONCE_PER_STEP` for its GAME_STATE part |
| radar `rand() % 5` (`world_update.c:211/218`, `:167`, `:415/421/431`) | in `worldUpdate`, per human pair | **no**: H2 runs the loops for the same pairs on every machine (HUMANS16 2.5.1) | none (keep on the shared stream: the count is deterministic) |
| the port's `four_radars` (HARDENING 17): the game's `rand()` per radar per car for views 3-4 | per local view | **yes** | port: its own stream, or H2's loops (it is the port's code) |
| VISUAL sites reached from the update (particles, smoke, sparks: `mathJitterPos` 8010BDE8 and the emitters) | once per event, same on every machine | no | none needed for determinism: they stay on the shared stream (moving them is an option, not a requirement) |

The rule: a VISUAL site may stay on the shared stream if **its call count is the same on every machine**. Only
the draw-time ones (animators, arcs) and the port's per-view radars must move. RAND_CALLERS lists them all.

### 4.4 Animations step inside the draw

`renderFlipFrame` (`rt_main.c:160`) re-arms the guard. `animUpdateAll` (end of `renderScene`,
`engine_render_scene.c:263`; `engine_anim_hit_volumes.c:489-510`) steps every registered animation once per step.
`animUpdateOne` (`:539-555`) steps a node the walker reaches. `debrisStartBurst` can reach it from the update. In
`portRunStep` (no draw) the registered animations would **not step**, so the re-run state differs from the first
run's in the type-3 fields.

Are they game state? Type-3 nodes are texture and model-cell animations (tire flicker, signs, water). Type-9
selects (damage stages, frozen model) are set by the logic, not by the animator. No game reader of a type-3 cell
was found (LIKELY). Two options:

- **(a) Presentation.** Leave the animators in the draw and exclude type-3 fields and the parity from the hash.
  Restoring them on a rollback makes a texture animation jump back a few cells (invisible in practice).
- **(b) Bit `ANIM_IN_STEP`.** `portRunStep` calls `animUpdateAll` itself, after `renderFlipFrame`, on the visual
  stream. `renderScene`'s call is skipped under the bit. `animUpdateOne` stays culling-dependent (presentation).

(a) is enough for determinism. (b) makes the snapshot hashable as a whole. The recommendation is (b) for the
registered list (cheap) and (a) for `animUpdateOne`.

### 4.5 `fxSpritesUpdate` / `fxSpritesDraw`

`fxSpritesUpdate` (`rt_main.c:157`) moves the 150 effect sprites once per step. `fxSpritesDraw` writes only the
follow sprites' `pos = *src` (RENDER_ANSWERS R3), with no `rand()`. Both are deterministic. The sprites are
presentation (no collision), so hash them or not as the port prefers. No change needed.

### 4.6 Frame-rate dependence: the big one

`SetFrameStart` (`timer_frame.c:66-80`) measures the real time since the last frame (`GetCurTics`, 70,618 tics per
VBlank). It sets `timerFrameTicks` = round(VBlanks), **clamped 3..5**, and `timerFrameRate` = 60 / ticks. Then:

- `timerGetFrameTicks()` scales speeds, timers and thresholds in the physics and the AI: e.g. `car_control.c:92-118`,
  `:185-188`, `:312-313`; `car_motion.c:298`, `:422-463`, `:535-546`; `car_contacts.c:425-439`, `:489-590`;
  `car_contact_resolve.c:217-319`; `ai_steps.c` (16 sites); `ai_driver.c:56-98`; `car_crash.c:29` (with `rand()`),
  `:331`; `car_rollover.c:85-116`; the event timers `rt_main.c:265`; the demo timers `world_update.c:336-337`.
- `timerGetFrameRate()`: `humanCarTick` / the AI car tick **rescale speed, steer and yaw rate whenever the rate
  changes** (`car_tick.c:70-75`, `ai_driver.c:48-55`), and `mathBlendAngle` / `mathBlendValue` scale every blend
  (`engine_math_near_pass.c:164`, `:175`).

So **a frame that takes 4 VBlanks on one machine (a hitch, a slow re-run batch, a pause) changes that machine's
physics for the next step**: a desync. In retail it also happens once after every pause. `start` (`rt_main.c:196`)
was taken before `pauseMenu` blocked, so the next `SetFrameStart` measures the pause and clamps to 5. Co-op (game
mode 2) runs at `rtFrameTime` 15 (`rt_main.c:115-116`): 4 VBlanks per step, ticks 4, 15 steps a second.

**Bit `FIXED_TICKS`:** `SetFrameStart` (or its call site at `rt_main.c:195`) sets `timerFrameTicks` =
`portStepTicks` (3; 4 in mode 2 if co-op online keeps 15 Hz), `timerFrameRate` = 60 / ticks,
`timerLastFrameDelta` = ticks x 70,618 (`timerGetFrameUsec` has no caller), and `timerFrameCount++`, with no
`GetCurTics`. `timerFrameStart` / `timerPrevFrameStart` are no longer used. The port's own pacing decides when a
step runs. Whether the port's emulated root counter already makes the measurement exact (always 3) is question 1:
even so, the bit is needed, because re-runs happen back to back.

### 4.7 Split-screen and camera reads in the logic

- `viewGetSplitMode()` in the logic: the regen rule (`car_tick.c:45`) is already fixed by H1 to test the game mode
  (`REGEN_SPLIT()` = mode 2 or 4). The sound sites (1.2) and `fxSheetStart`'s doubling (4.2) are presentation.
  `pauseMenu`'s pad 1 (`screen_pause_menu.c:578`) is meta. No other logic reader was found (LIKELY; the
  per-view list in HUMANS16 1.3 is the full census of view-indexed code).
- The sound listener (`sndListenerTransform` 8015DBE8, from `viewProc`'s outputs) reaches only sound parameters
  (1.2).
- The cameras write only the human records' camera block and the view globals (HUMANS16 2.5.2, 6.4: not hashed).
  The reader check of 2.5.2 (H1) stands.
- HUD hotkeys write HUD globals only (`hudToggleRadar/WeaponList`). The port's own HUD writes (HARDENING 20) are
  the port's.

### 4.8 Summary: the bits

| # | dependence | fix (bit, 8.1) |
|---|---|---|
| 1 | measured frame time | `FIXED_TICKS` |
| 2 | mines, flames, arcs, flash per drawn view; the mirror hotkey | `ONCE_PER_STEP` |
| 3 | animators stepped by the draw, culling-dependent `rand()` | `ANIM_IN_STEP` + `VISUAL_RAND` |
| 4 | arc jitter `rand()` per view | `VISUAL_RAND` |
| 5 | AI throttle | none (deterministic); the host's ruleset decides on/off |
| 6 | radar `rand()` | none for the game (H2); the port's `four_radars` |
| 7 | sound / camera / listener | presentation: excluded from the snapshot (sound) or the hash (camera) |

---------------------------------------------------------------------------------------------------------

## 5. Joining a match in progress

**An arena load is not deterministic between two machines.**

- **Seeds from the clock:** `srand(GetCurTics())` at boot / the shell (`ua_init.c:114`), at arena setup
  (`arena_setup.c:331`, end of `arenaSetup` 8011995C, from `sessionBindCars` `screen_card_layout.c:195`), and at
  round start (`scene_init.c:66`, `roundStart` 8011A024 ← `roundStartMusic` `game_mode_audio.cpp:35` ←
  `sessionRun`'s round loop `session_runner.cpp:145`). The first draw after the round seed is GAME_STATE
  (`8011A0D0`: `gp+0x5D8 = rand() % car count`; and the attract demo's `viewWatchedAiCar` `scene_init.c:85`).
- **`rand()` during the load, before any reseed:** `targetTimedEventInit`, `targetPlayerEventInit` and
  `targetActorPlace` draw from the stream that the *previous* seed left. That stream depends on the shell's seed
  and on everything drawn since (menus, the attract demo).
- **`timerFrameCount`** counts battle steps since power-on (`timer_frame.c:79`; zeroed only at boot `:46`), and the
  logic reads its parity (1.2).
- **The first step's ticks** come from the load's duration: `start` (`rt_main.c:102`) and `timerFrameStart`
  (last set at the previous battle's end) make the first `SetFrameStart` clamp to 5. Without `FIXED_TICKS`, the
  first steps differ.
- **Rounds carry state:** in modes 3/4 (and the port's battle modes) the world state carries across rounds:
  pickups, events, the collapse, scores. A drop-in history (`portCarStartHuman` / `portCarRemove`) changes the DMD
  and csObjectPool placement (which object slot a car got, which car group was loaded).
- **What is deterministic:** the memory and VRAM layout of a load (fixed load areas, fixed load order by settings
  and car list, no heap). Two machines loading the same arena with the same car groups place geometry, textures and
  routes at the same addresses.

**So:** a joining machine loads the arena itself (the large fixed data, VRAM, the routes) and then receives **the
full snapshot** of 3.4 (the per-step regions, about 0.5 MB raw, much less compressed: most of BSS is zero or
load-time tables), not a diff against its own fresh load. A diff against a fresh load would carry most of BSS
anyway, because of the seeds, the event tables and the round history. ONLINE_PLAN's "a few hundred KB" holds for
the compressed full snapshot.

To make **fresh starts** deterministic (every machine starting a new match together, no transfer), use bit `SEED`:
the host sends one seed per match. The port calls `srand(seed)` **before the load** (before
`sessionLoadArenaAndCars`, so `targetTimedEventInit` and friends draw the same numbers), and the three
`srand(GetCurTics())` sites use `portSeedFor(site)` (seed, or seed + round number at `roundStart`). It also sets
`timerFrameCount` = 0 at match start, and uses `FIXED_TICKS`. With those, a lockstep start needs no state transfer,
and the state hash checks it from step 0.

---------------------------------------------------------------------------------------------------------

## 6. Pausing everyone (holding the simulation)

**Retail's mechanism:** `pauseMenu` (`screen_pause_menu.c:554-687`) runs at the very end of the iteration
(`rt_main.c:224`) and blocks in its own `VSync(0)` loop (`:602-685`). The world is held because the loop does not
return. But it changes things: the music is paused and the serial volume zeroed (`:584-586`, restored `:629-631`),
`screenLostALife` dims the shown buffer (`:587`), sounds 28/29 play, the pad repeat mode switches (`:601`, `:634`),
`padActionCache` is written, quit schedules event 2 (`:643`), and **the next step runs with ticks 5** (4.6).

**The cleanest hold is between steps, at the top of the iteration** (`rt_main.c:122`, before
`rtUpdateScheduledEvents`): nothing of the step has run, and the state is exactly "the start of step f".

**Bit `HOLD` (a port flag `portStepHold`, in `.port_bss`, not part of the snapshot):** while it is set, the iteration
skips the whole step (no `rtUpdateScheduledEvents`, no `padUpdate`, no updates, no `SetFrameStart`
catch-up, no `clutAnimUpdate`) and still draws and presents the current state. interp.c redraws the last pose (t =
1), and the HUD draws without its 3 ticks. With `FIXED_TICKS` the step after the hold has normal ticks. The sound
keeps playing its loops; the port can duck it with the sound manager's volume (presentation). No menu opens; the
port draws its own "Host left: moving the server..." text over it.

With `portRunStep` (8.2), the hold is simply "the port does not call it". The bit is needed only while the retail
loop drives the steps (the first online build, lockstep). `rtScheduleEvent` is not a hold: it ends the loop.

The round-over freeze is not a hold either. `world_update.c:520-540` schedules event 3 after 210 or 420 ticks
(`rtScheduleEvent`), and the cars keep driving until it fires. Online it is ordinary simulation (deterministic
with `FIXED_TICKS`). The round ends on the same step everywhere.

---------------------------------------------------------------------------------------------------------

## 7. Clocks, the CD and the memory card in a battle

| source | where | effect | fix |
|---|---|---|---|
| `GetCurTics` → `srand` | `ua_init.c:114`, `arena_setup.c:331`, `scene_init.c:66` | the shared stream | `SEED` (5) |
| `GetCurTics` → `SetFrameStart` | `timer_frame.c:68`, `rt_main.c:195` | ticks / rate (4.6) | `FIXED_TICKS` |
| `GetCurTics` → pacing | `rt_main.c:102`, `:193`, `:196` | when a step runs | the port's pacing |
| `GetCurTics` → pad repeat | `controls.c:240-271` | not in battle (repeat mode 1) | none |
| `GetCurTics` (discarded) | `rt_main.c:123`, `camera.c:52/78`, `car_tick.c:101-103`, `world_update.c:547/557` | none | none |
| `timerFrameCount` (`GsGetWorkBase_0`) | parity tests (1.2) | state, counted since boot | 0 at match start (`SEED`), then in the snapshot |
| CD audio track-ended flag | `world_update.c:545` → `misc_one_liners.c:390` | music only (`audioAltMusicMode`) | live step only; not snapshotted |
| CD reads, XA position | none in a step | | |
| memory card | **none in a battle** (no `_card_*`/`InitCARD`/`bu00` reference in the battle code; saves happen in the shell) | | the port's own profile writes (HARDENING 44) are the port's |

---------------------------------------------------------------------------------------------------------

## 8. Recommended design

One define, **`TM2_PORT_ROLLBACK`**, adds one word `portRollbackOptions` in `.port_bss` (zero = retail, as always)
and a few port functions. With the define off, or with the word zero, the build is unchanged.

### 8.1 The bits

| bit | name | what changes | where |
|---|---|---|---|
| 0 | `FIXED_TICKS` | `SetFrameStart`: ticks = `portStepTicks` (default 3), rate = 60 / ticks, delta fixed, `timerFrameCount++`, no clock | `timer_frame.c:66` |
| 1 | `SEED` | the three `srand(GetCurTics())` → `srand(portSeedFor(site))`; `portSeedLoad()` before the load; `timerFrameCount` = `portStepCount0` at match start | `ua_init.c:114`, `arena_setup.c:331`, `scene_init.c:66`, `session_runner.cpp` before `sessionLoadArenaAndCars` |
| 2 | `ONCE_PER_STEP` | `streaksDraw` draws only; `portStepEffects()` runs `weaponDropsUpdate`, `weaponFlameUpdate`, the arcs' tick and damage, and the flash countdown once per step; `fxSheetStart` no split doubling | `streaksDraw` 800D8230, `weaponArcsDraw`, `fxSheetDisplay` 800DC590, `fxSheetStart` 800DC45C |
| 3 | `ANIM_IN_STEP` | `animUpdateAll` runs in the step after `renderFlipFrame`; `renderScene`'s call skipped | `engine_render_scene.c:263` |
| 4 | `VISUAL_RAND` | the animators' and the arcs' jitter `rand()` draw from `portVisualRand()` (its own seed, not in the hash) | 80151C00, 80151D30, 80151E24, 80151EA0, the arc jitter sites (RAND_CALLERS) |
| 5 | `SOUND_GATE` | while `portResimulating`: the sound wrappers, `soundUpdate`/`soundFlush` and the CD track test do nothing | `command_wrappers.cpp:20-33`, `timer_frame.c:106`, `world_update.c:545-553` |
| 6 | `CLUT_SHADOW` | while `portResimulating`: `clutAnimUpdate` steps its records but skips `LoadImage`; `portClutAnimUpload()` uploads all 10 from RAM after a batch | `engine_clut_anim.c` 80148B1C |
| 7 | `HOLD` | `portStepHold` set: the iteration draws only (6) | `rt_main.c:122` |
| 8 | `STEP_SPLIT` | `rtMainLoop` runs `portStepBody()` then the draws (8.2): view 0 is drawn after the world step | `rt_main.c:121-225` |
| 9 | `PAUSE_PORT` | `pauseMenu` returns at once (the port's online menu) | `screen_pause_menu.c:573` |

`portResimulating` is a separate `.port_bss` flag that the port sets around a re-run batch. It is not a bit of
the options word.

### 8.2 `portRunStep` and the split loop

```c
/* port_rollback.c (a --port-unit): one simulation step, no drawing. The port writes portHumanPad/Steer first. */
void portStepBody(void)                 /* the update half of rtMainLoop's iteration, in retail's order */
{
    if (rtUpdateScheduledEvents()) { portStepEnded = rtFiredEventValue; return; } /* the round is over */
    if (!(portHumanOptions & TM2_HUMAN_OPT_INPUT_ALL)) padUpdate();
    targetActorsUpdate(); fxRecordsUpdate(); debrisUpdate(); lightFlickerUpdate();
    portStepEffects();                  /* bit 2: drops, flames, arcs, flash: once */
    bulletsUpdate();
    portLevelEvents();                  /* rt_main.c:143-156 */
    fxSpritesUpdate();
    renderFlipFrame();
    if (portRollbackOptions & ANIM_IN_STEP) animUpdateAll();
    worldUpdate();
    rtUpdateFieldCounter(timerGetFrameTicks());
    SetFrameStart();                    /* bit 0: fixed */
    clutAnimUpdate();                   /* bit 6: no upload while re-running */
    portHudTicks();                     /* radar swap 800FBDFC, hudMessageUpdate 800FBC44, blink 800FB588 */
}
int portRunStep(void) { portStepEnded = 0; portStepBody(); return portStepEnded; } /* != 0: rtMainLoop's result */
```

Under `STEP_SPLIT`, `rtMainLoop`'s iteration becomes: `if (!portStepHold) portStepBody();` (leave with the event's
value if one fired), then the draw half: flip, `viewProc(0)`, `streaksDraw` (draw only), `fxSpritesDraw`,
`debrisDraw`, `renderScene`, view 1 / mirror, `DrawSync`, the pacing wait, present, the HUDs (their 3 ticks
skipped: done in the step), and `pauseMenu` (`PAUSE_PORT`). The port's rollback engine then calls `portRunStep()`
as `advance` (with `portResimulating` set for re-runs) from its frame hook: from the top of the iteration, or
from the pacing wait where interp.c already lives. The retail loop keeps the rendering, and the port drives the
steps. The port's `four.c` / interp.c hooks keep working, because the draw calls stay the same functions.

`save` / `load` copy the regions of 3.4 (from `rollback_regions.tsv`, 8.4) plus the DMD field list, and leave out
3.3's "no" rows. `checksum` hashes the same regions minus 3.3's "no" hash rows (HUMANS16 7's list is the start).

### 8.3 Port functions (the C side)

`portRunStep()`, `portStepBody()`, `portStepEffects()`, `portHudTicks()`, `portLevelEvents()`;
`portClutAnimUpload()`; `portSeedLoad()` / `portSeedFor(site)`; `portVisualRand()`; `portSoundReconcile()`
(after a batch: stop the loops whose cause is gone, `portCarSoundLoops` and shared loop 9); `portDmdMutableList(out,
max)` (walks the loaded level and car models into `(addr, len)` pairs, 3.2); storage `portRollbackOptions`,
`portResimulating`, `portStepHold`, `portStepTicks`, `portStepEnded`, `portSeed`, `portVisualSeed`.

### 8.4 Build outputs

`rollback_regions.tsv` per build: each region (name, address, size, snapshot y/n, hash y/n). It is made from the
map and a hand-kept list of excluded symbols (sound, OT/packets, camera, HUD, pads). `tools/check_all.py` checks
that every BSS symbol is in exactly one row.

### 8.5 Effort

| piece | size | notes |
|---|---|---|
| `FIXED_TICKS` | small (one function) | do first: needed for lockstep too |
| `SEED` (+ `timerFrameCount`) | small (4 sites + one port call) | |
| `HOLD`, `PAUSE_PORT` | small | |
| `ONCE_PER_STEP` | medium | `streaksDraw` and `weaponArcsDraw` split into tick and draw halves; `weaponArcsDraw`'s damage and credit (SUDDEN_DEATH) |
| `STEP_SPLIT` + `portRunStep` | medium | `rtMainLoop` under the bit; `portStepBody` in a port unit; interp.c / four.c hook addresses re-checked by the port |
| `SOUND_GATE` + `portSoundReconcile` | medium | 13 wrappers + the CD test; the reconcile needs CAR_GROUPS 12's loop bits |
| `CLUT_SHADOW` | small | |
| `ANIM_IN_STEP`, `VISUAL_RAND` | small to medium | 4 animator sites + the arc jitter; the port's interp.c visual stream moves into the C |
| `portDmdMutableList` | medium | a DMD walk by node type (FILE_FORMATS.md) |
| `rollback_regions.tsv` + check | medium | build tooling |
| the reader census (3.3's "no hash" rows truly unread by the logic) | medium (research) | HUMANS16 2.5.2's open CHECK, `padActionCache`, `csObjectPool` +0x14 readers; **done**: 11.6 (a), 13.5, 14.6 |

### 8.6 Risks

1. **Unknown state outside the list.** A global written by a step and missed by the region list desyncs on the
   first rollback. Mitigation: snapshot all of BSS and `.port_bss` first (0.5 MB is affordable), and narrow it
   later. The port's T12-style test (re-simulated frames hash as the first run) catches the rest.
2. **Render-side writes read by the logic.** `csUpdMat`'s +0x14 (`wpos`) readers were not traced (RENDER_ANSWERS
   R3, LIKELY none). If the logic reads it, it depends on the views drawn. The check is in 8.5's last row.
   **Closed** (11.6 (a), 13.5, 14.6): no game logic reads +0x14 or +0x124; the drawing's animation stepping was the
   real case (bit 3, 13).
3. **`debrisStartBurst` renders from the update.** It uses the current OT and packets. 8 re-runs with many bursts
   could overflow the frame's packet buffer. Scratch OT in re-runs, or check the buffer sizes. **Closed** (11.6 (b)):
   the premise was wrong, the capture writes no packets and no OT entries.
4. **Retail differences under the bits:** `ONCE_PER_STEP` changes split-screen mine and flame timing, and
   `STEP_SPLIT` draws view 0 one step fresher. Online-only by default (question 2).
5. **Sounds of mispredicted steps** (1.2): accepted. Loops that only stop on a transition need the reconcile.
6. **The port's own state** (HARDENING 3, 9-15): outside this list. The kill cam's registry must be complete.
7. **Interrupt-time guest code** delivered during a re-run batch (the port's runtime) would run the sound sequencer
   or the pads in the middle of a re-run.
8. **Co-op at 15 Hz:** `rtFrameTime` 15 in mode 2. The rollback engine's 20 Hz assumption, or 20 Hz everywhere
   online (ticks 3), is a gameplay change for co-op. **Decided (2026-10-19): 20 Hz everywhere** under `FIXED_TICKS`
   (section 10).

---------------------------------------------------------------------------------------------------------

## 9. Questions for the port

1. **Your root counter.** Does the runtime's `GetRCnt(0xF2000002)` / `timerTicsAccum` advance with emulated VBlanks
   (then retail's ticks are always 3 unless a step overruns) or with real time? Either way `FIXED_TICKS` is proposed.
   Do you want it as a decomp bit, or will you hook `SetFrameStart` 80155214 yourself?
2. **Per-view ticks offline:** under `ONCE_PER_STEP`, should offline split screen keep retail's double rate, with
   the bit online only, or should one rule hold everywhere (simpler; the mirror bug goes too)?
3. **Who drives the steps:** `STEP_SPLIT` (the C loop calls `portStepBody`, the port calls `portRunStep` for
   re-runs) or the port's own loop calling `portRunStep()` and the draw half (`portDrawFrame()`) separately? The
   second needs one more port function and replaces `rtMainLoop` for online battles.
4. **Sounds in re-runs:** is "mute re-runs + reconcile loops" acceptable, or do you want the engine to queue the
   re-run's sound requests and play the ones the first run did not (by sound id, step and car)? That is more work
   and rarely audible at 20 Hz.
5. **Animations:** option (a) presentation-only, or (b) `ANIM_IN_STEP` (4.4)?
6. **Join:** OK to always send the full compressed snapshot (5), with the seed and `timerFrameCount` set by the host
   at match start for lockstep starts?
7. **Co-op online at 15 or 20 steps a second** (8.6 item 8)?
8. **Snapshot granularity:** whole BSS + `.port_bss` first (simple, 0.5 MB), or the generated region list from the
   start?
9. **The AI throttle:** retail (deterministic) or off online, as part of the host's ruleset (4.1)?

## Addendum: findings from the per-question research passes

These came back from the separate research passes after the sections above were drafted. They settle three of
the open CHECK items and add one hazard.

- **VRAM read-back inside a step (new rollback hazard).** `clutAnimStartSweep` (`engine_clut_anim.c:126`,
  StoreImage at `:146-147`) and `clutAnimStartFadeOut` (`:153`, StoreImage at `:168-169`) read a palette row back
  from VRAM into `a->cur` / `a->orig` and compute the fade from it. They are reached from the step: the flame
  (`weapon_flame_update.c:142/144`), ground fires (`fx_ground_fires.c:152/154`) and invisibility
  (`ai_link_node_pick.c:289/291`). During a re-run, VRAM still holds the palettes uploaded by the discarded
  timeline, so the read-back gives a different value. **`CLUT_SHADOW`** must therefore also make
  `gpuStoreImageLine` (`engine_fileio_debris.c:417`) read from a RAM shadow of the palette rows, not only skip the
  uploads during re-runs. Snapshot `clutAnimRunning` (10 x 1060 bytes) with the state.
- **Animations in hit trees: the arenas affected.** No type-3 node on the disc starts with its "always step" byte
  (+0x12) set, so `animUpdateAll`'s list starts empty and every animation advances only when the renderer draws it.
  Four animation nodes in DISH (Moscow; ids 0, 440, 3000, 440) and one in FREEWAY (Los Angeles; id 440) hold hit
  volumes inside their frames, so on those arenas collision can differ between machines that draw differently,
  and a re-run that does not draw never advances them. This is the concrete case for **`ANIM_IN_STEP`**.
- **`padActionCache` (CHECK closed).** It is only written (`controls.c:146`). Its only reader is an unreachable
  branch (`controls.c:148-149`), and `padSetActionCache` (`engine_misc_accessors.c:158`) has no caller. No snapshot
  or hash needed.
- **`csObjectPool` +0x124 and the other `csUpdMat` fields (CHECK closed for +0x124).** The on-screen flag is written
  by `csUpdMat` and read only by `csUpdMat` and `renderScene` (a scan of the whole EXE, RENDER_ANSWERS R2.3). The
  logic does not read it; leave it out of the hash. The +0x14 field remains a CHECK (closed since: 11.6 (a), 14.6).
- **The AI throttle is deterministic.** Confirmed separately: `aiCarShouldUpdate` (`fx_explosions.c:217-237`) uses
  only the skip counter (+0x00C), +0x039, `target_dist` (+0x028) and the bearing `f024`, all game state written by
  the director and the AI step. The port's `HARDENING.md` 18 can be closed; "AI always updates" is a gameplay
  option, not a netcode fix.
- **Retail `rand()` on the radar is deterministic.** The radar's `rand() % 5` sites run once per step inside
  `worldUpdate` and depend only on game state. The view-dependent radar `rand()` is the port's own code
  (`four.c`, the port's `HARDENING.md` 17). `RAND_CALLERS.md` point 2 overstates this and should be corrected.
- **Other per-view work in `streaksDraw`.** Besides the arcs: `weaponDropsUpdate` (mine and remote-bomb fuses and
  the owner-immunity window run twice as fast with 2 views), `weaponDropCheckHit` (the hit test runs twice) and
  `weaponFlameUpdate` (the flame lasts half as long, up to double damage, double `rand()`). The rear-view mirror is
  a local hotkey, so one player turning it on changes the simulation for everyone. **`ONCE_PER_STEP`** moves all of
  these into the update phase.

---------------------------------------------------------------------------------------------------------

## 10. Stage 1 as built (request 48, 2026-10-19)

The port's answers to section 9 came with request 48 (`DECOMP_REQUESTS.md` 48). The decisions of the user:

- **Step rate: 20 steps a second in every mode, co-op (game mode 2) included.** With `FIXED_TICKS` the ticks are 3
  always (`portStepTicks` 0); retail's co-op rule (`rtFrameTime` 15, 4 ticks, 15 steps a second) is not used under
  the bit. This overrules the port's provisional 15 (answer 7). `portStepTicks` stays a port word (3..5) should a
  ruleset want another rate.
- **The AI throttle: retail's** (`aiCarShouldUpdate`'s update skipping stays; it is deterministic, 4.1). "Always
  update every bot" may become a Vanilla Plus option later; no code now.

Stage 1 builds the first four bits of 8.5's order: `FIXED_TICKS`, `SEED` (+ `timerFrameCount`), `HOLD` and
`PAUSE_PORT`, behind `-DTM2_PORT_ROLLBACK` (`include/semantic/port_rollback.h`). With the define off the build is
unchanged; with the define on and every word zero, every hook passes and the game is retail's (the same calls, the
same `rand()` calls in the same order: `tools/check_rollback.py` check 2). Each bit acts alone.

### 10.1 The words (`.port_bss`, every variant)

Defined in every variant after the matrix slots (`port_variant_arrays.c`, tsv rows), so the layout does not depend
on the define. Addresses: in the variant without port defines and with `TM2_PORT_ROLLBACK` alone (the same: the layout
does not depend on the define) `portRollbackOptions` 8084C6D8, `portResimulating` 8084C6DC, `portStepHold` 8084C6E0,
`portStepTicks` 8084C6E4, `portRollbackSeed` 8084C6E8, `portRollbackFrameCount` 8084C6EC, `portRollbackQuit`
8084C6F0, `portRollbackHolding` 8084C6F4, `portRollbackReseeds` 8084C6F8 (then request 49's `portStartPlaced`
8084C6FC, 80 bytes); with every port define and `TM2_PORT_HUMANS=16` (+ `TM2_PORT_RENDER_SLOTS=400`) they start at
808A8614 in the same order. Other builds: the build's `symbols.tsv`.

| word | written by | meaning |
|---|---|---|
| `portRollbackOptions` | the port | the bits (8.1's numbering): 0 `FIXED_TICKS` 0x001, 1 `SEED` 0x002, 7 `HOLD` 0x080, 9 `PAUSE_PORT` 0x200 (`TM2_RB_STAGE1` = 0x283); 2-6 and 8 reserved for the later stages (2-4 since built: section 11; 5, 6, 8: section 12). Read live |
| `portResimulating` | the port | the re-run flag of the later stages; stage 1 only stores it (the port reads it in its `hle_SetFrameStart` hooks: answer 3) |
| `portStepHold` | the port | `HOLD`: non-zero = the iterations draw only |
| `portStepTicks` | the port | `FIXED_TICKS`: the ticks of a step; 0 = 3 (the user's decision above); 3..5 as is, other values clamped to 3..5 |
| `portRollbackSeed` | the port (the host's) | `SEED`: the match seed |
| `portRollbackFrameCount` | the port (the host's) | `SEED`: `timerFrameCount` at the start of each load |
| `portRollbackQuit` | the port; cleared by the C | `PAUSE_PORT`: non-zero = quit at the top of the next step |
| `portRollbackHolding` | the C | this iteration's `HOLD` latch (1 = drawing only) |
| `portRollbackReseeds` | the C | the `srand` calls since the load's seed (part of the seed mix; match state, in the snapshot) |

### 10.2 The bits

**Bit 0 `FIXED_TICKS`.** `SetFrameStart` (80155214, `timer_frame.c`) calls `portRollbackFixedFrame()` after its clock
read: with the bit it sets `timerFrameTicks` = ticks, `timerFrameRate` = `timerBaseRate` (60) / ticks,
`timerLastFrameDelta` = ticks x 70,618, `timerFrameCount` + 1, and returns before the measurement (the clock value
read is discarded; `timerFrameStart` / `timerPrevFrameStart` are not touched). The port's hook stays out of
`SetFrameStart` (answer 1); `SetFrameStart` is still called every step, so the port's `hle_SetFrameStart` hooks
still see it. **The first step of each round:** `rtMainLoop` calls `portRollbackLoopEnter()` at its entry, before
its first `worldUpdate`, which sets the same ticks and rate (retail's first step uses the last measurement: 5 after
a load or a pause), and `portRollbackFrameTime()` after it set `rtFrameTime`: the pacing becomes 60 / ticks (20).
So every step of a match, the first included, runs at 3 ticks on every machine, whatever the frame took.

**Bit 1 `SEED`.** The three `srand(GetCurTics())` sites take `srand(portRollbackSeedFor(site))` (`RB_SEED` in the
source; retail's call without the define): `uaInit` (site 1, the shell), `arenaSetup`'s end (site 2, from
`sessionBindCars`), `roundStart` (site 3, every round). `portRollbackSeedFor(site)` returns
`portRollbackSeed + 0x9E3779B9 x portRollbackReseeds + 0x10001 x site` and counts the call; without the bit it
returns `GetCurTics()` (retail). **The load's seed:** `sessionLoadArenaAndCars` (both of its forms) starts with
`portRollbackPreload()`: with the bit, `portRollbackReseeds` = 0, `srand(portRollbackSeedFor(0))` and
`timerFrameCount = portRollbackFrameCount`. So every `rand()` of the load (`targetTimedEventInit`,
`targetPlayerEventInit`, `targetActorPlace`, `worldLoadStartObjects` under `arenaResetSystems`, the arena loaders),
of `sessionBindCars` (after `arenaSetup`'s seed: the cars' starts) and of every round (after `roundStart`'s seed)
comes from the match seed. Every load of the match does this, the Minion and Dark Tooth reloads too (each reload
restarts the mix: deterministic on every machine).

**`timerFrameCount`: set from the host.** The C writes `timerFrameCount = portRollbackFrameCount` at the start of
each load (not "match-relative" by itself): the host sends the value with the seed (0 is the natural choice), and
`SetFrameStart` counts on from there.

**Bit 7 `HOLD`.** At the top of each iteration `rtMainLoop` calls `portRollbackTop()` instead of going straight to
`rtUpdateScheduledEvents` (`RB_EVENTS`). It latches `portRollbackHolding` = (bit 7 and `portStepHold`) once for the
whole iteration (a flag the port changes during the pacing wait cannot split a step). A held iteration skips every
update and still draws: each update call in the iteration is `RB_STEP(call)` = "unless `portRollbackHeld()`":
`rtUpdateScheduledEvents`, `padUpdate` (no pad latch change), `targetActorsUpdate`, `fxRecordsUpdate`, `debrisUpdate`,
`lightFlickerUpdate`, `bulletsUpdate`, the level events (Moscow, LA, Paris, levels 5 and 7), `fxSpritesUpdate`,
`renderFlipFrame` (the animation guard is not re-armed), `worldUpdate` (the cars, AI, weapons, sound queue, radar
plots), `rtUpdateFieldCounter`, `SetFrameStart` (`timerFrameTicks` and `timerFrameCount` unchanged) and
`clutAnimUpdate`. Inside the draws: `streaksDraw` skips its four ticks (the flash countdown `fxSheetDisplay`,
`weaponDropsUpdate`, `weaponFlameUpdate`, `weaponArcsDraw`: the bullet streaks still draw; mines' sprites, arcs and
the flash do not while held), and `animUpdateAll` / `animUpdateOne` return (no animation step, no animator
`rand()`). Still run: `viewProc` (cameras), `fxSpritesDraw`, `debrisDraw`, `renderScene`, the HUDs, present, the
pacing, `pauseMenu`. Without `FIXED_TICKS` a held iteration moves the measurement's start (`ResetUpdateRate`), so the
step after a hold measures one frame (no catch-up to 5); with it there is no measurement at all. Presentation that
still moves while held (not game state, outside the hash: 3.3): the HUD's radar swap (the blips vanish while held),
`hudMessageUpdate`'s timer and the weapon marker blink, the cameras' view globals.

**Bit 9 `PAUSE_PORT`.** `pauseMenu` (80129528) returns at its entry (`portRollbackPauseOff()`): no pad read, no
music pause, no blocking loop. **The quit protocol:** the port's online menu sends "quit" in the input frame of a
step f; every machine writes `portRollbackQuit` = 1 before running step f. `portRollbackTop()` at the top of step f
(not held) clears it and calls `rtScheduleEvent(2, 0)`; `rtUpdateScheduledEvents` right after counts it below 0 and
`rtMainLoop` returns 2 in that same step on every machine, which `sessionRun` treats as retail's QUIT in modes 1-4 (`quit`,
the weapons reset; with HUMANS16 H5's rules bit, `portRoundResults` gets result 2). Mode 0 has no pause menu in
retail and ends only on result 1: the port should not send quit there. Retail's "BATTLE ABORTED" text and its
wait are not shown (the port draws its own). The word is read only with the bit; a quit while held waits for the
first step that runs.

### 10.3 The port's calls and their timing

The port calls no function in stage 1; it writes words:

| when | what the port writes |
|---|---|
| before the match's first load (before `sessionRun` reaches `sessionLoadArenaAndCars`) | `portRollbackOptions` (0x283 for stage 1), `portRollbackSeed` and `portRollbackFrameCount` (the host's, the same on every machine), `portStepTicks` (0 = 3) |
| before every step (the exchanged input frame, H2's `portHumanPad` / `portHumanSteer` too) | `portStepHold` (0 to run the step, non-zero to hold), `portRollbackQuit` when the frame says quit |
| around re-run batches (later stages) | `portResimulating` |

Read back: `portRollbackHolding`, `portRollbackReseeds`, `timerFrameCount` / `timerFrameTicks` (8.4's region list
later). Port functions (`.port_text`, `functions.tsv` kind `port`): `portRollbackFixedFrame` 8084C844, `portRollbackLoopEnter`
8084C898, `portRollbackFrameTime` 8084C8D4, `portRollbackTop` 8084C93C, `portRollbackHeld` 8084C9D4,
`portRollbackSeedFor` 8084C9E4, `portRollbackPreload` 8084CA54, `portRollbackPauseOff` 8084CAA4 (`TM2_PORT_ROLLBACK`
alone; 808BD434.. with every define).

### 10.4 Checks and proofs

`tools/check_rollback.py LINK --expect-functions [--e0 E0] [--negative-control]`, run by `tools/check_all.py` for
every link with the define: (1) the port functions in `.port_text`, none reaching `rand()`; `portRollbackSeedFor`
reads the options and the seed and calls `GetCurTics` (the retail path); the load seed writes `timerFrameCount`;
(2) no new `rand()` / `srand()`: the changed functions and request 49's call them as often as in the port variant without port defines (`--base`), the port
functions not at all (the load seed's one `srand` is the only new one); (3) the seed sites: `uaInit`, `arenaSetup`,
`roundStart` no longer call `GetCurTics`, each `srand` takes `portRollbackSeedFor`'s result, and the load seed
dominates every other call of `sessionLoadArenaAndCars`; (4) the gates by flow graph (each guarded call dominated
by its gate and unreachable from the gate's "held" / "on" edge): the step calls of `rtMainLoop`, the event countdown,
the first step's ticks before the first `worldUpdate`, `streaksDraw`'s ticks, the animators, every store of the frame
timer in `SetFrameStart`, every call of `pauseMenu`; (5) the latch: `portStepHold` read only by `portRollbackTop`,
`portRollbackHolding` written only by `portRollbackTop` / `portRollbackLoopEnter`, `portRollbackQuit` used only by
`portRollbackTop`, no game function reading a port word directly; (6) with `TM2_PORT_CARS`, request 49's marks
(`CAR_GROUPS.md` 7.10); (7) the changed functions: exactly the 10 below with the define alone. A link without the
define fails checks 1-5 (`--negative-control`).

Changed game functions (10): `SetFrameStart`, `rtMainLoop`, `uaInit`, `arenaSetup`, `roundStart`,
`sessionLoadArenaAndCars`, `pauseMenu`, `streaksDraw`, `animUpdateAll`, `animUpdateOne`. Port functions: 8 + 2 static
helpers (`portRollbackTicks`, `portRollbackSetTicks`).

Proofs (on the objects of `work/build-20261019-052155-850291`, `build_source_only.py` + `verify_reloc.py` against
its E0, each with its placement link, limits forced to 2): no define `bb5f7769…` (P2 `de7ded35…`, unchanged);
`TM2_PORT_ROLLBACK` `87847b56…` (port_changed: the 10 functions; P2 `43434d5b…`); `TM2_PORT_CARS=16` +
`TM2_PORT_ROLLBACK` `33a7f9ae…`; the full car stack `337338ee…`; every port define + `TM2_PORT_HUMANS=16` +
`TM2_PORT_RENDER_SLOTS=400` + `TM2_PORT_ROLLBACK` `7a27f998…` (port end 0x808BF4FC). `tools/check_all.py` passes on
all ten links (proofs, placement, humans, car groups, hazards, balance, custom, rollback).

### 10.5 What stage 1 does not do

The later bits (`ONCE_PER_STEP`, `ANIM_IN_STEP`, `VISUAL_RAND` (built since: section 11), `STEP_SPLIT` +
`portRunStep`, `SOUND_GATE`, `CLUT_SHADOW`) and `rollback_regions.tsv` / `portDmdMutableList` are not built; their bits
are reserved and do nothing. So in stage 1 drawing still feeds the simulation where 4.2-4.4 say (the per-view ticks, the culled
animators' `rand()`): two machines must draw the same views (the lockstep milestone's two instances of one
Challenge Match do), and the mirror must stay off.

### 10.6 Fresh starts and the records' history

A fresh start without a state transfer (every machine loads with the same seed) also needs equal car records:
records are not cleared by a load (`carTablesClear` clears the tables, not the records), and two reads at a
round's start see a record's previous life (`CAR_GROUPS.md` 7.10 (b): the human start height from the previous
car's ride height, `RETAIL_BUGS.md` 35; an AI car's stale stuck counters). The port's lockstep milestone sends the
host's full state at step 0, which covers them; a start without it should begin from zeroed (or equal) records.

---------------------------------------------------------------------------------------------------------

## 11. Stage 2 as built: drawing independence (request 48, 2026-10-19)

Stage 2 builds the next three bits of 8.5's order: `ONCE_PER_STEP`, `ANIM_IN_STEP` and `VISUAL_RAND`. With them the
simulation no longer depends on what this machine draws: how many views (split screen, the port's views 3-4), the
rear-view mirror, what the camera sees (culling), or how many frames are drawn per step. Same rules as stage 1: behind
`-DTM2_PORT_ROLLBACK`, each bit read live from `portRollbackOptions` (0 = retail), retail's text without the define
(the retail images are unchanged), every bit acts alone. `TM2_RB_STAGE2` = 0x29F is stage 1's four bits plus these
three. The port's answer 2 (one rule everywhere, offline too once its baselines are re-recorded) and answer 5
(option (b), `ANIM_IN_STEP`) are what is built.

### 11.1 The word (`.port_bss`, every variant)

One new word, defined in every variant after request 49's `portStartPlaced` (nothing before it moves):
`portRollbackVisualSeed` 8084C74C (the variant without port defines and `TM2_PORT_ROLLBACK` alone; 808A8688 with every
port define, `TM2_PORT_HUMANS=16` and `TM2_PORT_RENDER_SLOTS=400`). It is the state of the **visual stream**,
`portRollbackVisualRand()`: retail's `rand()` formula (seed x 1103515245 + 12345, bits 16..30) on its own word. The
draws that must leave the game's stream alone take it. Any value works; the port may seed it. It is presentation:
leave it out of the hash and never restore it (each machine's stream advances with its own drawing).

### 11.2 The bits

**Bit 2 `ONCE_PER_STEP` (0x004).** `streaksDraw` (800D8230) ran four ticks once per drawn view (4.2). Under the bit:

- `streaksDraw` skips `weaponDropsUpdate` and `weaponFlameUpdate` in every view (`RB_VIEW_TICK`, gate
  `portRollbackViewTickOff`: held, or the bit), and its two mixed functions only draw (`RB_VIEW_DRAW`, gate
  `portRollbackViewDrawOff`: skipped only when held *without* the bit, as in stage 1; with the bit they run, draw
  only, held or not, so the flash and the arcs stay on screen during a hold).
- `rtMainLoop` calls `portRollbackStepEffects()` right after view 0's `streaksDraw` (`RB_STEP`: not while held). It
  runs the four ticks once per step in retail's order: `fxSheetDisplay(0)` (the flash countdown and fade, when
  `fxSheetTimer` > 0), `weaponDropsUpdate` (mine and remote-bomb fuses, the drop blend, `weaponDropCheckHit`),
  `weaponFlameUpdate` (damage, ignition, `rand()`), `weaponArcsDraw(0, 0)` (the tick half of the arcs).
- **The split functions.** `fxSheetDisplay` (800DC590) and `weaponArcsDraw` (801084D8) ask `portRollbackPass(ctx ==
  0)`: without the bit `TM2_RB_PASS_BOTH` (retail: tick and draw), with it `TM2_RB_PASS_TICK` for the step's call
  (OT / context 0) and `TM2_RB_PASS_DRAW` for a view's call. `fxSheetDisplay`: the tick counts down and fades, the
  drawing adds the quad (`AddPrim`, as retail does per view). `weaponArcsDraw`: the tick sets `weaponArcsActive`, runs
  the taser retry (`TM2_PORT_BALANCE` bit 16), places the free end (and stores it in `weaponArcs[i].to`, as retail),
  draws the two `rand() < 512` effect rolls (smoke, explosion: game stream), counts down and applies the damage at 39
  and 0; it computes no points and draws nothing. The drawing builds the 16 points and the segments from the arc's
  state with **no state written**: the free end goes to a local pointer, and the jitter (the end point's 3 draws and
  the 14 points' 42) comes from the visual stream. So a view's call touches neither the game's state nor its `rand()`
  stream: any number of views, the mirror and the port's views 3-4 (whose `four.c` hook `streaks_draw_only` is no
  longer needed under the bit) draw without effect on the simulation.
- `fxSheetStart` (800DC45C) no longer doubles the flash time when split (`portRollbackOncePerStep`): the countdown runs
  once per step.

What changes against retail with the bit (both online and, per answer 2, offline): split screen's mines, bombs,
flames, arcs and flash run at the single-view rate (retail ran them twice as fast, double flame damage and double
`rand()`); the mirror no longer speeds them up; the arcs' jitter differs (another stream); view 0 draws the arcs and
the flash before the step's tick (as retail: the tick ran after the drawing in the same call), view 1 / the mirror
after it (retail: after their own second tick). An arc's last step (count 1 -> 0) is drawn by view 0 only.

**Bit 3 `ANIM_IN_STEP` (0x008).** The answer (b) of question 5. Under the bit:

- `animUpdateAll` (80151700, end of `renderScene`) and `animUpdateOne` (801518E8, from the walker and so from
  `debrisStartBurst`'s `renderDrawObjectUnculled` too) step nothing (gate `portRollbackAnimDrawOff`: held, or the
  bit). Drawing, culling and bursts no longer advance an animation.
- `rtMainLoop` calls `portRollbackAnimStep()` right after `renderFlipFrame` (`RB_STEP`), where view 0's `renderScene`
  stepped them in retail. It steps the registered list (`animUpdateAll`'s loop, with `animListEnabled`), then walks the
  trees the renderer would reach **without the camera**: the level model (`csGetRoot()`, +0x11C: its animation frames
  hold the hit volumes of DISH and FREEWAY, the addendum above; request 51 met DISH's online: 13), the world models
  (`renderWorldModels`), the follow object (`viewGetFollowObj(0)`, shared by every view: `viewSetFollowObj` at the
  load) and every object of the scene
  list with `visible` (+0x08) set (not the on-screen flag +0x124, which is culling). The walk
  (`portRollbackAnimWalk`, `.port_text`, engine_anim_hit_volumes.c) follows `renderWalkModel`'s node types
  (reference/renderer_walker_engine_render_scene.c): groups, offsets, transforms and scales walk every child unless
  hidden (+0x15, +0x13, +0x27 bit 1, as the walker); an animation (type 3) is stepped as `animUpdateOne` does unless it
  is listed (+0x12), then its current frame is walked; a select (type 9) walks its current step; where the walker
  chooses by the camera it walks every branch: every LOD band (type 2) and both zone children (type 11). The parity
  guard (`done[]`, re-armed by `renderFlipFrame`) steps a node shared by several objects once per step.
- **Which `rand()`:** the animators stepped by the step are simulation (the hit volumes) and the same on every machine,
  so their `rand()` stays the game's stream (`portRollbackAnimVisual` is 0 with this bit; see `VISUAL_RAND`).

Choices (not in 8.1): every LOD band and both zones are walked (the camera's choice would make the step depend on the
view); objects count by `visible`, not by culling; the follow object of view 0 only (both views share its model).
Retail difference: animations off screen now run (signs, water and the like animate whether seen or not), and
animations nested in LOD bands step in every band. A burst no longer steps the animations of the model it captures
(research B below).

**Bit 4 `VISUAL_RAND` (0x010).** Draw-time `rand()` from the visual stream (`RB_ANIM_RAND`, `ARC_RAND`; retail's
`rand()` call stays at each site for the other case, so the census of check 2 is unchanged):

- the arcs' jitter (the end point and the 14 inner points: 6 sites in `weaponArcsDraw`) in every pass. With
  `ONCE_PER_STEP` the drawing pass takes the visual stream anyway; this bit adds the retail combined pass (no
  `ONCE_PER_STEP`) and the tick-less case is moot (the tick pass computes no points);
- the four animator sites (`animStepRandom` 80151BC4, `animStepWeightedRandom` 80151CB8, `animPickPeriod` 80151DF4 x 2)
  **only while the renderer steps them** (`ANIM_IN_STEP` clear: `portRollbackAnimVisual` = bit 4 and not bit 3).

**A deviation from 8.1, with the reason.** 8.1 / 4.4 put the step's animators on the visual stream too. That is not
safe: the visual stream advances with each machine's drawing (the arcs' jitter per view), so an animation stepped by
the step and drawing from it would pick different frames on different machines, and on DISH and FREEWAY a frame holds
hit volumes. So with `ANIM_IN_STEP` the animators stay on the game's stream (their call count is the same everywhere:
the walk does not depend on the camera), and `VISUAL_RAND` moves them only when the renderer steps them (`ANIM_IN_STEP`
off), where it keeps the culled animators' draws out of the game's stream (4.3's purpose). With `TM2_RB_STAGE2` both
bits are set: the arcs' jitter is visual, the animators are game state. `animControl`'s step-now (mode 1, from the
logic's `animSetStep` triggers) follows the same rule.

**Each bit alone** (the port sets 0x29F): `ONCE_PER_STEP` alone makes the drops, flames, arcs and flash independent of
the views (the arcs' drawing pass is visual already); the animators still step in the draw. `ANIM_IN_STEP` alone makes
the animations and their `rand()` independent of the views and the culling; the arcs still tick per view.
`VISUAL_RAND` alone only keeps the draw-time draws (arc jitter, culled animators) off the game's stream; the ticks
stay per view. Full drawing independence needs bits 2 and 3; bit 4 adds nothing to determinism once both are set
(the remaining visual draws, the arcs' jitter in the drawing pass, already use the visual stream), but keeps the
arcs' combined pass off the game's stream in builds or modes where bit 2 is clear.

### 11.3 What the port does

| when | what |
|---|---|
| before the match (with stage 1's words) | `portRollbackOptions` \|= 0x01C (`TM2_RB_STAGE2` = 0x29F); optionally `portRollbackVisualSeed` (any value; per machine is fine) |
| its views 3-4 | keep calling `streaksDraw`, `fxSpritesDraw`, `debrisDraw`, `renderScene` for the extra views: under bits 2 and 3 they do no simulation work (the `streaks_draw_only` hook can go; it does no harm) |
| the snapshot / hash | `portRollbackVisualSeed` out of both; the animation nodes' fields (type-3 +0x0C frame, +0x0E elapsed, +0x06 period, +0x10 parity flags) are now state stepped by the step: in the snapshot with the level's model data (`portDmdMutableList`, a later stage) and hashable |
| offline (answer 2) | the same bits when the port's baselines are re-recorded |

Nothing else: no new call. `portRollbackStepEffects` and `portRollbackAnimStep` are called by `rtMainLoop`; under the
later `STEP_SPLIT` they move into `portStepBody` at the same places (after `lightFlickerUpdate` / view 0's
`streaksDraw`, after `renderFlipFrame`).

### 11.4 Functions

Changed game functions (16 with the define alone; stage 1's 10 plus 6): `SetFrameStart`, `rtMainLoop`, `uaInit`,
`arenaSetup`, `roundStart`, `sessionLoadArenaAndCars`, `pauseMenu`, `streaksDraw`, `animUpdateAll`, `animUpdateOne`
(stage 1, the last four changed again), and `weaponArcsDraw`, `fxSheetDisplay`, `fxSheetStart`, `animStepRandom`,
`animStepWeightedRandom`, `animPickPeriod`. Not changed: `weaponDropsUpdate`, `weaponFlameUpdate` (called from a new
place only), `renderScene`, the walker.

Port functions (`.port_text`, `functions.tsv` kind `port`; with `TM2_PORT_ROLLBACK` alone `portRollbackOncePerStep` 8084CABC, `portRollbackPass` 8084CAD4, `portRollbackViewTickOff` 8084CB08, `portRollbackViewDrawOff` 8084CB40, `portRollbackStepEffects` 8084CB70, `portRollbackAnimDrawOff` 8084CBD4, `portRollbackAnimVisual` 8084CC0C, `portRollbackArcVisual` 8084CC28, `portRollbackVisualRand` 8084CC58, `portRollbackAnimStep` 8084D0CC; with every define, `TM2_PORT_HUMANS=16` and `TM2_PORT_RENDER_SLOTS=400` from 808BD6AC in the same order (`portRollbackStepEffects` 808BD760), `portRollbackAnimStep` 808BF7F8 (other builds: the build's `symbols.tsv`)): in `port_variant_arrays.c` `portRollbackOncePerStep`,
`portRollbackPass`, `portRollbackViewTickOff`, `portRollbackViewDrawOff`, `portRollbackStepEffects`,
`portRollbackAnimDrawOff`, `portRollbackAnimVisual`, `portRollbackArcVisual`, `portRollbackVisualRand`; in
`engine_anim_hit_volumes.c` `portRollbackAnimStep` and the static `portRollbackAnimWalk` / `portRollbackAnimStepOne`.
None calls `rand()`; `portRollbackStepEffects` and `portRollbackAnimStep` run the game's ticks and animators, which do.

### 11.5 Checks and proofs

`tools/check_rollback.py` covers stage 2 (run by `check_all.py` for every link with the define):

- check 1: the gates in `.port_text` reach no `rand()`; the two step runners call none themselves,
  `portRollbackStepEffects` calls exactly the four ticks, `portRollbackAnimStep` reads the level, the list and the
  objects and reaches the four animators;
- check 2: the rand census against the variant without port defines now includes the 6 new functions (each keeps its
  `rand()` sites);
- check 4: by flow graph, `portRollbackViewTickOff` guards `weaponDropsUpdate` / `weaponFlameUpdate` in `streaksDraw`,
  `portRollbackViewDrawOff` guards `fxSheetDisplay` / `weaponArcsDraw`, `portRollbackAnimDrawOff` guards the animators
  in `animUpdateAll` / `animUpdateOne`, `portRollbackHeld` guards the two new step calls in `rtMainLoop`;
  `portRollbackPass` dominates every other call of `fxSheetDisplay` and `weaponArcsDraw`; `weaponArcsDraw` has 6
  visual-stream sites after `portRollbackArcVisual` and keeps its 8 `rand()` sites; each animator has one
  `portRollbackAnimVisual` + visual site per `rand()` site; `fxSheetStart` calls `portRollbackOncePerStep`;
- check 5: `portRollbackVisualSeed` is used only by `portRollbackVisualRand`;
- check 7: exactly the 16 functions above with the define alone.

**Negative controls.** A link without the define fails checks 1-5 (`--negative-control`, as stage 1). A link with
one hook removed fails: `streaksDraw` built with a plain `weaponDropsUpdate(ctx, view)` instead of `RB_VIEW_TICK(...)` (link `work/pv/r48b_neg_P4`, the source restored afterwards) fails check 4 ("weaponDropsUpdate at 800D8458 reachable without portRollbackViewTickOff"); the other checks pass, so the failure is the missing hook.

Proofs (`verify_reloc.py` against E0 of `work/build-20261019-155430-640378`, each with its placement link, limits forced to 2): no define `bb5f7769…` (P2 `de7ded35…`, both unchanged: the new word is BSS); `TM2_PORT_ROLLBACK` `9782368a…` (port_changed: the 16 functions; P2 `62250bf5…`; port end 0x8084D2F0); `TM2_PORT_CARS=16` + `TM2_PORT_ROLLBACK` `7694ffff…` (P2 `e48df042…`); the full car stack without the define `337338ee…` (unchanged); every port define + `TM2_PORT_HUMANS=16` + `TM2_PORT_RENDER_SLOTS=400` + `TM2_PORT_ROLLBACK` `7e9901a3…` (P2 `96291538…`; port end 0x808BFBB0, RAM end 0x80C00000). `tools/check_all.py` on the full build and the ten links: 32 checks, 0 failed (retail hybrid `9219f7ba…`, source-only `d755c86e…`, sdk-lib `f557c1e3…` unchanged; proofs, placement, humans, car groups, hazards, balance, custom, rollback); `--negative-control` on the link without the define: fails as expected.

### 11.6 Request 48's extra questions

**(a) The readers of `csObjectPool` +0x14 (8.5's last row, 8.6 risk 2): no game logic reads it.** CONFIRMED for the C,
LIKELY overall (an m2c sweep backs the grep). What it is: `csUpdMat` (`engine_clut_anim.c:511-519`) writes `wpos` =
the object's position relative to the camera in its own frame (`parent->wpos + pos`, rotated), only when `visible`;
`viewProc` writes the root's +0x14 = minus the camera position per view (`view_system.c:272-280`;
`viewSetFixedCamera` `:435-459`); `csReset` zeroes it. `renderScene` calls `csUpdMat` for every listed object per drawn
view, so the field is per-view and rewritten before every read. Its readers: `csUpdMat` itself (the parent's, for the
child chain), `renderWalkModel` (the walk's start translation, reference walker `:597-599`), `fxDebrisDraw`
(`engine_fileio_debris.c:600-624`), the eye copies through `viewGetEyeTrans` (`fxImpactParticlesDraw`, `fxSpritesDraw`,
`streaksDraw`, `weaponArcsDraw`, `viewLoadGte`), and `sndListenerTransform` (sound pan and volume only,
`engine_sound_lists.c:212-217`). Not readers: the hit detection (it reads +0xA8 `pos` and +0x80 `rotm`,
reference/hdp_walker `:805-806`, `:1095-1121`), `struct TM2Object`'s users (nothing at +0x14), the `visible` (+0x08)
users. So the field, +0x20..+0x7F and +0x124 are presentation: out of the hash, not restored (the next draw rewrites
them before reading). 8.6 risk 2 is closed. Re-checked by request 51 (13.5): `hdpPassRun` reads only the root's
+0x11C; the world collision that differed there came from the animation frames (bit 3), not from +0x14.

**(b) `debrisStartBurst` across 8 re-runs: no scratch OT is needed.** The premise of 1.2 / 1.3 / 8.6 risk 3 was wrong:
`renderDrawObjectUnculled` (`engine_anim_hit_volumes.c:344-357`) writes **no packets and no OT entries**. It runs
`renderInitState`, `csUpdMat`, `renderWalkModel` and `lightAddSources`, not the emitter (`sub_800CDC54`) or
`renderNearPolys` (which `renderDrawModel` calls). "`renderCtx->ot`" is the RenderState +0x3C mesh-queue count
(`end`), misnamed. The walker fills scratch arrays only (the mesh list, at most 280 entries; the matrix slots; the
node stack), which `renderInitState` resets before every `renderScene`; `lightAddSources` copies at most 16 entries
(16 x 20 bytes) into the burst slot (`lightCurrentSet`, +0xE8: exactly the 0x228 slot). The capture does not depend on
the camera either (LIKELY: the burst object's parent `debrisRootObject` is never drawn, so its `wpos` stays 0 and the
captured positions are world coordinates; LOD forced, node culling off). The packets of the debris are spent by
`fxDebrisDraw` (from `debrisDraw`, once per drawn view), bounded by its `pktEnd` test and by the 4 live slots, not by
how many bursts started. So 8 re-runs add **0 bytes** to the 390,320-byte per-view packet buffers (request 47), leave
nothing in the OT (no ghost polygons), and the cursor and the OT are reset each frame anyway (`rt_main.c:153-155`).
What a discarded burst leaves is state: the burst slots (4 x 0x228 at 801A3E70), `lightCurrentSet` and the `rand()`
draws, all restored with the snapshot. "Run as is". With `ANIM_IN_STEP` the burst's walk steps no animation either
(`animUpdateOne` is gated), which closes the last CHECK of that path.

### 11.7 Open notes

- `ANIM_IN_STEP` walks the whole level tree every step (and up to 8 times in a re-run batch). The cost is a tree walk
  without matrices (no GTE); measure it on the port. If it shows, a later stage can collect the type-3 nodes once per
  load into a list (with the drop-in unloads pruning it, as `portDropIn` prunes `animNodeList`).
- The animation nodes are now simulation state and live in the level's and the cars' model data (DMD), outside BSS:
  the snapshot must include them (`portDmdMutableList`, built in stage 3: 12.2); until then a rollback restores BSS but not the
  frames, and on DISH / FREEWAY the hit volumes would then differ after a rollback. Lockstep without rollback is not
  affected.
- `weaponArcs[i].to` keeps retail's store of a stack address (`free_end`) for untargeted arcs in the tick pass. It is
  deterministic (the tick runs at one call depth on every machine) but meaningless; leave the field out of the hash
  when `target` < 0, or hash it.
- Request 48's (c) (the port's high-RAM drawing scratch as "no" rows in `rollback_regions.tsv`): answered in 12.4
  (yes, listed as "no").

---------------------------------------------------------------------------------------------------------

## 12. Stage 3 as built: re-runs (request 48, 2026-10-19)

Stage 3 builds the rest of 8.5's order: `STEP_SPLIT` + `portRunStep`, `SOUND_GATE` + `portSoundReconcile`,
`CLUT_SHADOW`, then `rollback_regions.tsv` and `portDmdMutableList`. With it **request 48 is complete**: the port can
save, load and re-run steps. Same rules as stages 1 and 2: behind `-DTM2_PORT_ROLLBACK`, each bit read live from
`portRollbackOptions` (0 = retail), retail's text without the define (the retail images are unchanged), every bit acts
alone. `TM2_RB_STAGE3` = 0x3FF is every bit (stages 1 and 2 and these three).

### 12.1 The words (`.port_bss`, every variant)

Defined in every variant after `portRollbackVisualSeed` (nothing before it moves):

| word | written by | meaning |
|---|---|---|
| `portRollbackSplitting` | the C (`portRollbackTop`) | this iteration's `STEP_SPLIT` latch: 1 = the step ran at the top (`portStepBody`), the iteration's own update calls are skipped |
| `portRollbackEnded` | the C (`portRunStep`) | 1 = a re-run fired an event (the round is over): `rtMainLoop` leaves at its next top with `rtFiredEventValue` |
| `portClutShadow` | the C | `CLUT_SHADOW`'s palette rows (`struct TM2PortClutShadow`, 0x4188 bytes: `rows`, `overflow`, 32 rows of {x, y, count, 256 colours}); game state |

`portResimulating` (stage 1's word) is now read: the port sets it around a batch of re-runs.

Addresses: with `TM2_PORT_ROLLBACK` alone (and in the variant without port defines: the layout does not depend on the define) `portRollbackSplitting` 8084C750, `portRollbackEnded` 8084C754, `portClutShadow` 8084C758; with every port define, `TM2_PORT_HUMANS=16` and `TM2_PORT_RENDER_SLOTS=400` `portRollbackSplitting` 808A868C, `portRollbackEnded` 808A8690, `portClutShadow` 808A8694. The larger `.port_bss` moves `.port_text` (by 0x4190 with the define alone): the port functions of stages 1 and 2 have new addresses (e.g. `portRollbackFixedFrame` 808509D4, `portRollbackAnimStep` 80852040); other builds: the build's `symbols.tsv`.

### 12.2 The bits

**Bit 8 `STEP_SPLIT` (0x100): the whole step first, then the drawing.** At the top of each iteration
`portRollbackTop` latches the bit (`portRollbackSplitting`) with `HOLD`; `rtMainLoop`'s `RB_EVENTS()` then calls
`portRollbackLoopStep()` instead of `rtUpdateScheduledEvents()`, which runs **`portStepBody()`** (unless held) and
returns 1 when the round is over (the loop returns `rtFiredEventValue`, as retail). `portRollbackHeld()` (the
`RB_STEP` gate of every update call in the iteration) is 1 for a split iteration, so the rest of the iteration only
draws: the buffer flip, `viewProc(0)`, `streaksDraw`, `fxSpritesDraw`, `debrisDraw`, `renderScene(0)`, view 1 or the
mirror, `DrawSync`, the pacing wait, present, the HUDs and `pauseMenu`. The drawing gates read the `HOLD` latch alone:
a split iteration draws as an unheld one (with bits 2 and 3 its drawing does no simulation work; without them
`streaksDraw` and the animators still tick per view, as retail).

`portStepBody()` (`rt_main.c`, `.port_text`) is the update half in retail's order: `rtUpdateScheduledEvents` (1 =
the round is over: return), `padUpdate` (live steps only: not while `portResimulating`; the inputs come from H2's
words, `INPUT_ALL`), `targetActorsUpdate`, `fxRecordsUpdate`, `debrisUpdate`, `lightFlickerUpdate`,
`portRollbackStepEffects` (bit 2), `bulletsUpdate`, the level events (Moscow, LA, Paris, levels 5 and 7),
`fxSpritesUpdate`, `renderFlipFrame`, `portRollbackAnimStep` (bit 3), **`portRollbackHudStep`** (the radar swap,
below), `worldUpdate`, `rtUpdateFieldCounter`, `SetFrameStart` (bit 0: fixed ticks), `clutAnimUpdate` (bit 6). 8.2's
sketch had a `portLevelEvents` and a `portHudTicks`; the level events are inline, and of the HUD's three ticks only the
radar swap moved (choice below).

**`portRunStep()`**: the port's re-run (its rollback engine's `advance`). It runs `portStepBody()` whatever this
iteration's `HOLD` latch says (the latch is cleared for the call and put back), and returns 1 when an event fired: the
port stops the batch, and `rtMainLoop` leaves at its next top (`portRollbackEnded`). The port calls it **between
iterations** of `rtMainLoop` with `STEP_SPLIT` set, from its frame hook (the top of the iteration or the pacing wait,
where interp.c lives), with `portResimulating` = 1 around the batch (answer 3: the port reads the flag in its
`hle_SetFrameStart` hooks, which `portStepBody`'s `SetFrameStart` call still reaches). The live step stays the C
loop's: after a batch the loop's next iteration runs the next live step.

**What moves against retail with the bit** (presentation only): view 0 is drawn after the world step (one step
fresher than retail: 1.1); the step sees the previous frame's camera (`viewProc` runs in the drawing now; its readers
in the step are the sound listener and `debrisStartBurst`'s capture: 1.3, 11.6 (b)); `SetFrameStart` and
`clutAnimUpdate` run before the frame's drawing instead of after the pacing wait (the palette changes one frame
earlier; without `FIXED_TICKS` the measurement is still one step apart).

**The radar swap (choice).** Retail's `hudDisplayRadar` swaps a radar's buffers after drawing it, once per drawn HUD.
`worldUpdate` plots the blips into the back buffer. Re-runs (no drawing) would pile the blips of N steps into one
buffer, and the port's in-between frames (several drawn frames per step) would show an empty radar after the first.
With the bit, `portRollbackHudStep()` swaps every HUD's radar in the step, before `worldUpdate`'s plots, and
`hudDisplayRadar` leaves the swap (`portRollbackRadarInStep()`). A held iteration (no step) keeps the blips on screen
(stage 1's hold made them vanish). `hudMessageUpdate`'s timer and the weapon marker blink stay in the HUD draws: they
are presentation timers that run per drawn HUD, and a re-run (no drawing) does not touch them, so nothing piles up; 8.2
moved them only for symmetry.

**Bit 5 `SOUND_GATE` (0x020).** While `portResimulating` is set with the bit (`portRollbackSoundOff()`):

- the 10 wrappers of `command_wrappers.cpp` (`soundStartPlayerLoop` 800D3244 .. `soundFlush` 800D34E8) return 0 at
  once (`RB_SOUND`: every caller in the C discards the result; the census of 1.2's CHECK found no reader);
- `soundUpdate` (800D34C0, `taskMgrRunFrame`) runs no sound frame;
- `worldUpdate`'s CD test `audioGetDarkToothMusic() && audioTakeTrackEnded()` is skipped (the live step takes the
  track-ended flag);
- with `TM2_PORT_CAR_SOUND`, the port's instance merge (`portSoundPlayInstance`, which writes the sound manager's
  requests directly) plays nothing.

The first run's sounds keep playing. **The sound side's flags are left out of the snapshot** (`rollback_regions.tsv`
"no"): `soundTurboLoopActiveP1/P2`, `soundGunLoopActiveP1/P2`, `soundLoop32ActiveP1/P2`, `soundSharedLoopActive`,
`soundActiveId21..65` (1.2 called them game-side; they are set by `soundMgrPlay` / `soundMgrPlaySided` and cleared by
the stops, so they say what plays), `portCarSoundLoops`, the request pool and the instances. With that, every loop
whose stop or start site tests the state each step heals at the next live step: the turbo, gun and burn loops of the
humans, the computer cars' shared gun loop 9 (`weapon_tick.c`: stopped every step it does not fire; `world_update.c`
for dead cars), the engines (re-issued every step), the flame 47, the rocket 51, the ricochet 18 and the specials'
sounds (`soundIsActive` tests every step: `weapon_flame_update.c:152`, `weapon_fire_specials.c:384`,
`weapon_ricochet.c:220`, `world_update.c:482`).

**`portSoundReconcile()`** (the port calls it after a batch, `portResimulating` cleared): the sounds whose stop site
fires only on a transition. Those are the projectiles' flight sounds 10, 11, 12 and 15 (`soundPlayVariant(id, ..,
slot + 1)` at launch, `soundStopInstance(id, slot + 1)` when the projectile hits or runs out: `projectile_hit.c:292`,
`:314`). For each request in those ids' lists whose projectile slot (instance - 1) is not live (`fxRecords[i]` life
> 0) with that type, it calls `soundStopInstance`. So a missile that the corrected steps never fired, or that hit
earlier, falls silent. Accepted (8.6 risk 5): a one-shot of a mispredicted step plays, a one-shot (or a flight sound)
that only the corrected steps start is lost. Twister's tornado 0x35 is stopped on her death only (a transition); it is
a one-shot of bounded length, not reconciled.

**Bit 6 `CLUT_SHADOW` (0x040).** The palette animations reach VRAM only through `RB_CLUT_LOAD` / `RB_CLUT_STORE`
(`engine_clut_anim.c`: the three `clutAnimStart*` read-backs, the three `clutAnimStep*` uploads). With the bit:

- `portClutLoadLine` (the upload) writes the row into `portClutShadow` (by x, y) and to VRAM unless re-running;
- `portClutStoreLine` (the read-back: the addendum's hazard) copies the row from the shadow when it holds it, else
  reads VRAM and seeds the shadow;
- `portClutShadowCapture()` (called by `rtMainLoop`'s entry, `portRollbackLoopEnter`) reads every definition's row
  (`clutAnimDefs[0..clutAnimLastIndex]`) from VRAM: the round's palettes after the load, the same on every machine.
  So no read-back of the round depends on VRAM, live or re-run;
- `portClutAnimUpload()` (the port, after a batch, and after it loads a snapshot without re-running: a join, a
  rollback to the present) uploads every shadow row to VRAM.

The shadow is game state (a fade-out's colours come from it): in the snapshot and the hash. 32 rows (the definitions
of a level use a few; `overflow` counts the rows that found no room: they read VRAM as retail).

**`portDmdMutableList(out, max)`** (3.2): the models' node fields a step may write, as (address, bytes) pairs, sorted
and merged (a node shared by several objects once). It walks every node reachable from the level model
(`csGetRoot()`), the world models (`renderWorldModels`) and every allocated object's model (`csObjectPool[0 ..
csPoolCount)`: the cars, projectiles, pickups), through every child (every LOD band, animation frame, select child,
both zone children), and lists per node type (FILE_FORMATS.md 2.4), a little wider than 3.2's table (the flag bytes
too; restoring an unchanged byte costs nothing): group `[+0x14, +0x18)`, animation `[+0x06, +0x14)` (period, frame,
elapsed, parity flags, the listed flag, the chained frame: `ANIM_IN_STEP`'s state), offset `[+0x04, +0x14)`, transform
`[+0x04, +0x28)`, hit box `[+0x04, +0x0C)` (the disabled byte), select `[+0x06, +0x0C)`, zones `[+0x04, +0x08)`.
Returns the pair count, or minus the raw pairs needed when `max` is too small. The port calls it after each load and
after each of its calls that changes the models (a drop-in, `portCarStartHuman` / `portCarRemove`, a car swap); the
list is the same on every machine that loaded the same match. It reads the models and writes only `out` (any buffer:
the port's high RAM).

### 12.3 What the port does

| when | what |
|---|---|
| before the match | `portRollbackOptions` = 0x3FF (`TM2_RB_STAGE3`; 0x29F and these three) |
| after each load, and after a model file's load or unload of its own (15.1: not after other model changes) | `portDmdMutableList(buf, max)` (retry with a larger buffer on a negative result) |
| save / checksum | the regions of `out/TM2_SOURCE_PORT/rollback_regions.tsv` (or the link's): every "snapshot yes" row, minus the "no" rows inside them, plus the DMD list; the hash over the "hash yes" rows (12.4) |
| a rollback | load the snapshot; `portResimulating` = 1; `portRunStep()` per step to re-run (stop on 1); `portResimulating` = 0; `portSoundReconcile()`; `portClutAnimUpload()` |
| a join | load the full snapshot; `portClutAnimUpload()` |
| its re-runs' timing | between iterations of `rtMainLoop` (the top, or the pacing wait), never inside one; no interrupt-time guest code (the sound sequencer, the pads) during a batch (8.6 risk 7) |
| inputs | H2's `INPUT_ALL` (`portHumanPad` / `portHumanSteer`) for every human, written before each step, re-runs included |

### 12.4 `rollback_regions.tsv` (and request 48's question (c))

`config/US/rollback_regions.tsv` is the hand-kept list; `tools/rollback_regions.py LINK` resolves it against a
build's symbols and sections into `LINK/rollback_regions.tsv` (address, size, name, kind, snapshot, hash, reason),
and `build.py --port-variant` publishes `out/TM2_SOURCE_PORT/rollback_regions.tsv` with the other maps. Rows:

- three ranges, snapshotted and hashed: `game.static` (the game's `.sdata` / `.sbss` / `.bss`), `port.bss`,
  `port.data`;
- the `.data` words a step writes: the timer globals (`timerFrameTicks`, `timerFrameRate`, `timerFrameCount` hashed;
  `timerLastFrameDelta`, `timerFrameStart`, `timerPrevFrameStart` snapshotted only);
- "no" rows, holes in the ranges or `.data` left alone, each with its reason: the sound side (the manager's pools and
  flags, the listener, the CD player), the OTs and packet buffers, the port's settings and latches
  (`portRollbackOptions`, `portResimulating`, `portStepHold`, ...), the visual stream;
- "yes, not hashed" rows: the cameras, the HUD, the pads, the debris bursts, the flash (3.3);
- `dmd`: the run-time list (12.2);
- fixed ranges: the scratchpad, and **the port's high RAM, answering request 48's (c): yes, listed as "no"** with the
  reason "the port's drawing scratch, never game state": `port.interp_ots` 0x80C00000..0x80C80000 (interp.c's OTs),
  `port.precise_ring` 0x80C80000..0x80DC0000, `port.views34` 0x80DC0000..0x81000000 (views 3-4's OTs and frame
  records, four.c). They lie above the port variant's RAM end (`--port-ram-end`, default 0x80C00000), outside every
  section of the build. The boundaries are the request's; the port adjusts the rows if its layout changes.

The port's own state outside the build (HARDENING 3, `vp_keep_register`, `feats_keep_register`) is the port's list.

### 12.5 Functions

Changed game functions (35 with the define alone): stages 1-2's 16 (`rtMainLoop` changes again) and
`soundStartPlayerLoop`, `soundStopPlayerLoop`, `soundStartSharedLoop`, `soundStopSharedLoop`, `soundPlay`,
`soundPlayVariant`, `soundPlayVariantEx`, `soundStop`, `soundStopInstance`, `soundFlush` (`command_wrappers.cpp`),
`soundUpdate` (`timer_frame.c`), `worldUpdate`, `hudDisplayRadar`, `clutAnimStartSweep`, `clutAnimStartFadeOut`,
`clutAnimStartCycle`, `clutAnimStepSweep`, `clutAnimStepFade`, `clutAnimStepCycle`. With `TM2_PORT_CAR_SOUND` the
port's static `portSoundPlayInstance` is gated too.

Port functions (`.port_text`, `functions.tsv` kind `port`; with `TM2_PORT_ROLLBACK` alone, then with every define, `TM2_PORT_HUMANS=16` and `TM2_PORT_RENDER_SLOTS=400`): `portRollbackHudStep` 80850E90 / 808C1A90; `portRollbackRadarInStep` 80850EDC / 808C1ADC; `portRollbackSoundOff` 80850EEC / 808C1AEC; `portSoundReconcile` 80850F1C / 808C1B1C; `portClutLoadLine` 808511C4 / 808C1DC4; `portClutStoreLine` 8085129C / 808C1E9C; `portClutShadowCapture` 80851398 / 808C1F98; `portClutAnimUpload` 80851490 / 808C2090; `portDmdMutableList` 808517B8 / 808C23B8; `portStepBody` 808519E0 / 808C3E6C; `portRunStep` 80851B40 / 808C3FCC; `portRollbackLoopStep` 80851B98 / 808C4024; in `port_variant_arrays.c` the first nine and the static `portClutRow`, `portClutCopy`, `portDmdAdd`, `portDmdWalk`, in `rt_main.c` the last three. None calls `rand()`; `portStepBody` (and so `portRunStep`, `portRollbackLoopStep`) runs the game's step, which does.

### 12.6 Checks and proofs

`tools/check_rollback.py` covers stage 3 (run by `check_all.py` for every link with the define):

- check 1: `portStepBody`, `portRunStep`, `portRollbackLoopStep` in `.port_text`, calling no `rand()` themselves;
  `portStepBody` calls its 23 calls in that order and nothing else (but `gameGetLevel`, `timerGetFrameTicks`), and
  reads `portResimulating`; the other two call `portStepBody` only; the helpers call what they wrap;
- check 4: `rtMainLoop` calls `portRollbackLoopStep` only on `portRollbackTop`'s non-zero edge; every call of the 10
  wrappers and `soundUpdate` (jal, the virtual `jalr`, a tail jump) is guarded by `portRollbackSoundOff`;
  `worldUpdate`'s two CD calls are dominated by it; `hudDisplayRadar`'s swap is guarded by `portRollbackRadarInStep`;
  the six `clutAnim*` functions call `portClutStoreLine` / `portClutLoadLine` as often as the base calls the gpu line
  functions, and the gpu functions no more; `portRollbackLoopEnter` calls `portClutShadowCapture`;
- check 5: the split latch, the pending end, `portResimulating` (no C writer) and `portClutShadow`'s users;
- check 7: exactly the 35 functions with the define alone;
- **check 8 (new): `rollback_regions.tsv`.** Every row resolves in the link (no two symbol rows overlap, every row has a
  reason), and every static word a step stores to (a LO16 / GPREL16 store in a function `portStepBody` reaches through
  the call graph) lies in a row. It prints the stores into "no" rows (by design: the reasons).

**Negative controls.** A link without the define fails checks 1-5, the stage 3 parts included (`--negative-control`
on `work/pv/r48c_none_P4`: the split step, every sound wrapper, the radar swap and the palette rows FAIL as expected).
Check 8 with `--regions` pointing at a copy of the list without the `timerFrameCount` row fails on the link with the
define ("not covered: timerFrameCount (stored by SetFrameStart, portRollbackFixedFrame)"); the first run of check 8
also found a real gap, `viewAxisMatrixTemplate` (`.data`, stored by `viewLoadMode` from `carSetViewMode` in a step:
the camera), now a row.

Proofs (`verify_reloc.py` against E0 of `work/build-20261019-175955-566422`, each with its placement link, limits
forced to 2): no define `7259869b9568…` (P2 `de7ded357cd2…`; the image moved from stage 2's `bb5f7769…` only because the larger
`.port_bss` moves `.port_text`: the same 25 functions differ from E0, P2 unchanged); `TM2_PORT_ROLLBACK` `a279dbb41991…`
(port_changed: the 35 functions; P2 `1526018b5738…`; port end 0x80852290); `TM2_PORT_CARS=16` + `TM2_PORT_ROLLBACK`
`e31618fce6e5…` (P2 `a9d58e15d0fb…`); the full car stack without the define `4c619d5a9fb2…` (P2 `a54847420793…`); every port define +
`TM2_PORT_HUMANS=16` + `TM2_PORT_RENDER_SLOTS=400` + `TM2_PORT_ROLLBACK` `7b1538b5b119…` (P2 `67ebab9c65ed…`; port end
0x808C4B60, RAM end 0x80C00000). `tools/check_all.py` on the full build and the ten links: 32 checks, 0 failed (retail hybrid `9219f7ba…`, source-only `d755c86e…`, sdk-lib `f557c1e3…` unchanged; proofs, placement, humans, car groups, hazards, balance, custom, rollback).

### 12.7 Open notes

- **Rollback needs bits 2, 3 and 8 together** (and `INPUT_ALL`): `portRunStep` re-runs only the step, so without
  `ONCE_PER_STEP` and `ANIM_IN_STEP` the per-view ticks and the animators (still in the drawing) would not re-run.
  Each bit still acts alone (8 alone only reorders the iteration).
- **The census of check 8 sees direct stores only.** Stores through pointers (the records, the pools: inside the
  ranges anyway) and calls through pointers (the task manager, the C++ virtuals, the object step functions in
  tables) are not followed; whole-range snapshots cover them (8.6 risk 1). The port's T12-style test (re-run hashes
  equal the first run's) remains the final proof.
- `fxSpritesDraw` writes the follow sprites' position (RENDER_ANSWERS R3) in the drawing; re-runs do not, so the
  sprites (presentation, no collision) may differ after a rollback. Hash them or not as the port prefers (4.5).
- `CLUT_SHADOW` after a drop-in or a car swap: the port reloads palettes outside a step; it calls
  `portClutShadowCapture()` after the reload (the rows of the new definitions are read from VRAM otherwise, on first
  use).
- `portDmdMutableList` walked the models attached to objects; a model the logic swapped in later was not listed until
  the next walk. **Closed by request 54 (15.1):** it walks every model file loaded for the match.
- The HUD message timer and the marker blink stay per drawn HUD (12.2): they run at the drawing's rate, as retail.
- 1.1's sketch called `portStepBody` from `portRunStep` with the port's own `portStepEnded`; it is `portRollbackEnded`
  here and the result is `rtFiredEventValue`, as retail.

---------------------------------------------------------------------------------------------------------

## 13. Request 51: a car's collision with the world depends on what each machine drew (2026-10-21)

The port's rejoin test (`net_lobby.py --players 3 --quit-last 82 --rejoin 70`, Moscow) parted at step 1297: on the
host only (it was spectating another car), computer car 4's contact block held contacts against the world object
(`csObjectPool[0]`) with a wall's normal; on the clients the same records were empty. The request suspected the
world object's words +0x14..+0x50 (they differ between the machines in every dump). **They are not the cause.** The
cause is the one the addendum above predicted for DISH (Moscow): **the level's animation frames hold hit volumes,
and without `ANIM_IN_STEP` an animation advances only when a view draws it.** The port runs with
`portRollbackOptions` = **0x00000001** (`FIXED_TICKS` only: in all 22 RAM dumps, host and client; the port's only
write of the word is `port_config.c`'s bit 0), so stage 2's bits are off. **No new bit is needed: bit 3 `ANIM_IN_STEP` (11.2) is the fix; the port sets it (13.3).**

Addresses: retail / the port's build of these dumps (`build/decomp-wc/out/TM2_SOURCE_PORT/symbols.tsv`, the link
whose `portRollbackOptions` is 808A8654 and `csObjectPool` 805FCA00, as in the dumps).

### 13.1 (1) Where the probes take the world's collision from (CONFIRMED)

| step | function (retail / port build) | what it reads |
|---|---|---|
| 1 | `carsPhysicsUpdate` 8011E220 / 8011FB90 (`car_physics.c`) | for each updated car: `hdpCollideObject(car object, contact + 0x18, 1, saved matrix, saved position)`; the point records start at contact + 0x1C (AI record +0xF4, 0x20 each) |
| 2 | `hdpCollideObject` 8014E66C / 80150BDC (`engine_hit_detection.c`) | sets the walker state (scratchpad), calls `hdpPassRun`, then the plane test, then `animControl` for every hit leaf of type 8 that names an animation (a hit can trigger an animation, in the step) |
| 3 | `hdpPassRun` 8014FAB0 / 80152020 (hand-written asm) | clears the point records (+0x00, +0x14, +0x18 = 0, +0x1C = -1, +0x1A / +0x1E = 0); **the world:** `csGetRoot()` (`csObjectPool[0]`) **+0x11C, its model only**, walked from the car object's position (+0xA8); the root's address goes to the walker state (+0xAC) as "the object hit". **No other word of the root is read** (not +0x04, +0x14..+0x7F, +0x80, +0xA8). Then every scene object with +0x08 and +0x0C set (not the car itself): its +0x80 rotation matrix, +0xA8 position, +0x11C model |
| 4 | `hdpWalk` 8014EA60 / 80150FD0 (asm; the kinds `hdpKind0..13`) | the model's nodes. Kind 1 group (sphere of the probe; skipped when +0x15 bit 0 is set), 2 LOD (the band by **the probe's own** distance: no camera), **3 animation: the child `frames[+0x0C]`, the animation's current frame**, 4 offset, 5 transform, 8 leaf, **9 select: the child `[+0x07]`**, 11 zones (box tests of the probe), 12 scale |
| 5 | `hdpLeafTestPoints` 8014F168 / 801516D8 (and `...Mark` / `...SingleBox`) | a leaf hit fills the point record: +0x00 = 1, +0x04..+0x0C the face normal (through the parent transforms), +0x10 the depth, +0x14 the object (the root), +0x18 the leaf's +0x08, +0x1C / +0x1E the tag of the nearest tagged ancestor |
| 6 | `carTestContactPoints` 800E05F0 / 800E0CEC, `carContactResolve`, `carContactsCombine` | the records: push, damage, sounds |

**What drawing writes in that path: the animation nodes' state** (type 3: +0x0C frame, +0x0E elapsed, +0x10 the
parity flags) in the level's model data (DMD, below 0x80080000 on Moscow). The renderer (`renderWalkModel` 801526CC,
type 3) calls `animUpdateOne` for each animation it reaches after culling, and an animation's step can trigger its
child (`animControl`, +0x1C / +0x13), whose frames hold the volumes. The renderer writes nothing else the walker
reads (it writes no node; the select's +0x07 is set by the logic). The same holds for the projectiles' test
(`hdpTraverse` 8014FEEC / 8015245C, from `hdpQueryPoint`: its kinds 3 and 9 read the same bytes).

### 13.2 Evidence from the dumps (`build/kc-hunt4`: `lobby_port-dev` = the host, `lobby_port-dev2` = a client)

- **Bits:** `portRollbackOptions` (808A8654) = 0x00000001 in every RAM dump (steps 1280-1300, both machines).
- **The car records** (`state.txt.cars`, 74,404 bytes a step: 16 human records, 16 AI records, the sprites) are
  equal up to step 1296. At 1297 only AI record 4 differs: its point records 0, 1, 4, 5 (+0xF4, +0x114, +0x174,
  +0x194) and +0x818 / +0x828. Host, record 0: `00000000 00000000 FFFFF000 00000000 0000001B 805FCA00 00000000
  01BB0000` (normal word +0x08 = -0x1000, depth 27, the object `csObjectPool[0]`, tag 0x1BB at +0x1E); client:
  `0 0 0 00001000 18 0 0 0000FFFF` (no object, tag -1). The car is at (33, 92, 839) on both.
- **The world's hit tree** (`csObjectPool[0]` +0x11C = 0x8007584C, walked in the dump): 3,390 nodes, 17 animations
  (type 3); at step 1296, 12 of them differ between the host and the client (frame, elapsed or parity), and 4 hold
  hit volumes in their frames: ids 0, 440, 3000, 440 (the addendum's DISH list). The rest of the level's model
  data is equal.
- **The host's tag 0x1BB is group 0x80074A94** (tag 443), whose only leaf is 0x80074A28 (a wall face). It lies in
  frames 1-5 of **animation 0x80074E3C** (id 440, weighted random, 6 frames, +0x12 = 2: only its parent steps it);
  frame 0 is empty. Its parent is **animation 0x80074E94** (id 3000, a 2-frame loop, period 802 ticks, trigger
  frame 0, child frame 1: when it wraps, `animControl(0x80074E3C, 1)` steps the child once,
  `animStepWeightedRandom`, one `rand()`), which the renderer steps when it draws it:

| step | 0x80074E94 frame / elapsed, host | client | 0x80074E3C frame, host | client |
|---|---|---|---|---|
| 1280 | 1 / 681 | 1 / 123 | 1 (wall in) | 0 (no wall) |
| 1290 | 1 / 711 | 1 / 123 | 1 | 0 |
| 1296 | 1 / 729 | 1 / 123 | 1 | 0 |
| 1298 | 0 / 3 (wrapped) | 1 / 123 | 5 | 0 |
| 1300 | 0 / 9 | 1 / 123 | 5 | 0 |

  The host's spectator camera draws the animation (+3 ticks a step); the client's own camera has not drawn it for
  a while (elapsed frozen at 123). So the wall's volume was present on the host and absent on the client for the
  whole window; car 4 reached it at 1297. On the host the wrap at 1297/1298 also drew a `rand()` from the game's
  stream that the clients did not draw (a second way the machines part). The pair 0x8006F0BC (id 0) / 0x8006F08C
  (id 440) is the same kind (host elapsed 3..60, client frozen at 126; child frame 1 vs 2).
- **The world object's +0x14..+0x74 differ** (and +0x14..+0x30 of 20 other objects): `wpos` and the view matrices
  that `viewProc` / `csUpdMat` write per drawn view. 13.1 step 3: the hit pass does not read them.

### 13.3 (2) The fix: bit 3 `ANIM_IN_STEP` (built in stage 2), set by the port

Under bit 3 the renderer steps no animation (`animUpdateAll` 80151700 / 80153C70 and `animUpdateOne` 801518E8 /
80153E68 return), and `portRollbackAnimStep` (.port_text, 808BFF58 in this build), called by `rtMainLoop` right after
`renderFlipFrame` (`RB_STEP`), steps the registered list and walks **the level's whole tree from `csGetRoot()`
+0x11C without the camera** (every LOD band, both zone children, the current frame of each animation, the current
step of each select), then the world models, the follow object and every visible object. 0x80074E94 is in the
level's current branch (the hit pass reaches its frames, so the step's walk reaches it too), so it advances 3 ticks
every step on every machine, its wrap steps 0x80074E3C at the same step everywhere, and the `rand()` of that step
stays on the game's stream (bit 3 keeps the animators on it: 11.2, "which `rand()`"), the same call on every
machine. The hit volumes are then a function of the step alone. **A new bit would duplicate bit 3**, so none is
built (retail and every variant unchanged; no new state words: the frames are the `dmd` row of
`rollback_regions.tsv`).

- **`VISUAL_RAND` alone is not a fix:** without bit 3 it moves the drawn animators' `rand()` to the visual stream,
  so 0x80074E3C's frame would come from each machine's own stream: still different.
- **What the port sets, online, on every machine before the match:** `portRollbackOptions |= 0x01C` (bits 2, 3, 4:
  `TM2_RB_STAGE2` with stage 1's bits), at least 0x008. Bit 2 (`ONCE_PER_STEP`) is the other drawing dependence
  (4.2: mines, flames, arcs, flash per drawn view; a player's mirror hotkey or split screen changes them); the host
  spectating with one view and a client with one view happen to agree, a mirror does not. Offline: the port's
  answer 2 (when its baselines are re-recorded); the retail differences are in 11.2 (off-screen animations run).
- **The hash:** the port's state hash covers the car records (and the cars dump), not the level's animation nodes,
  so this case surfaced only when a car touched the volume, at 1297; the frames had parted before 1280 (the
  first dump). Hash the `dmd` rows (`portDmdMutableList`, 12.2), or at least the level's type-3 nodes
  (+0x0C..+0x11), to see it at the first step.
- **The join:** the host's full state at join carries the frames (the level's DMD is in the 16 MB image); with bit 3
  every machine steps them the same from there.

### 13.4 (3) Other game logic that reads what a view's drawing computed

**(a) Every RAM region that differs between the host and the client at step 1296** (16 MB each; names from the
link above), classified:

| region | what | reader | class |
|---|---|---|---|
| level DMD 0x80026C8C..0x8007B11C, 152 bytes in 16 runs | 17 animation nodes and nothing else (frame, elapsed, parity): the 12 of the level's tree (13.2) and 5 of the level file's other models or branches (0x800753EC, 0x80075570, 0x8007708C, 0x80078A9C, 0x8007B108) | the hit pass, `hdpTraverse` (the level's); the model walkers | **state: bit 3** |
| `portCarDmdArea` +0x3D6C4 | a car model's animation frame (5 vs 1) | the hit pass walks car models (objects with +0x08 and +0x0C) | **state: bit 3** (the step walks every visible object's model) |
| `portCarGroups.freeze` (key 3's choice record, +0x2C and +0xA4) | the frozen-car model's two option lists (type-3 nodes: frame 4 vs 0, parity) | the model walkers | **state: bit 3** where the record is in a visible object's tree (LIKELY); presentation otherwise |
| `csObjectPool` +0x14..+0x7F of 21 objects (the root's to +0x74) | `wpos` and the per-view matrices (`viewProc`, `csUpdMat`) | the renderer, the effect drawers, the sound listener (11.6 (a)) | presentation |
| `csObjectPool` +0x124 of 5 objects | the on-screen flag (`csUpdMat`) | `csUpdMat` and `renderScene` only (addendum, R2.3) | presentation |
| `csObjectPool[11]` +0xA8 (id 4095, the sky) | placed at view 0's camera (`viewSetFollowObjPos`) | the renderer; +0x0C = 0, so not in the hit pass | presentation |
| `csObjectPool[98]` +0x80..+0xB0 (`viewCameraObjects[0]`) | the view's camera object (no model) | the hit pass walks it (+0x08 and +0x0C set) with model 0: the node at address 0, whose byte (0xB8) is no kind, so nothing; the result does not depend on the camera (LIKELY: true while RAM[0]'s low byte is 13 or more, retail's case). **Corrected in 17.3:** camera objects are never on `csListHead`'s list, so the hit pass does not walk them; `viewProc` alone reads them | presentation |
| `humanCars` +0x04..+0x43, +0x10C (humans 0 and 2) | the human records' camera block | the cameras (4.7; the port leaves them out of its hash) | presentation |
| `viewEyePositions`, `viewCameraLockFrames`, `renderView0Frames` / `FrameB`, `renderCurrentOt`, `renderWalkerSaveRegs`, `renderOtView1Buf0` / `1`, `renderMatrixSlots`, `renderSlotMatrixPtrs`, `renderObjectMatrices`, `renderFrustumMatricesA` / `B`, `portMeshList`, `portPacketBuffers`, `portRenderStats`, `portVisualState`, 0x801FF218.. (the render buffer after BSS) | drawing scratch | the renderer | presentation |
| `hudPlayerRecords`, `hudRadars`, `hudMessage*`, `fontCursorX` | the HUDs | the HUD drawers (12.7: the message timer runs per drawn HUD) | presentation |
| `soundInstPool`, `soundOverflowInsts`, `sndListenerRelPos` / `Pan`, `portSndObjSlots`, `soundVoiceInUse`, libsnd's voice records, register shadows and queues (`D_801A5398`, `D_801A5200`, `D_801A5878`, `D_801A5AF4`, `D_801B2D66`), `portSpuExt` | the sound side (listener-dependent volumes, voices) | sound only (1.2) | presentation ("no" rows; `SOUND_GATE`) |
| `portHumanLocal` | this machine's human | the port | per machine by design |
| 0x80DD2010..0x80DF6A2C | the port's high RAM (views 3-4's OTs and frame records) | the port's drawing | presentation (12.4: "no") |

Nothing else differs: the game's other BSS, the pools, the AI and car records, the projectiles and the rest of
`.port_bss` are equal at 1296. So at that step the animation nodes are the only state that drawing changed.

**(b) The drawing's other writes, from the earlier passes** (the census of 4.x, 11 and 12, re-checked here):

| drawing-time write | read by the logic? | bit / status |
|---|---|---|
| animation frames (level, world models, objects' models), the animators' `rand()` | **yes**: the car probes, the projectiles' test, the debris capture | bit 3 (`ANIM_IN_STEP`) |
| per-view ticks in `streaksDraw` (drops, flames, arcs, flash) | yes (damage, fuses, `rand()`) | bit 2 (`ONCE_PER_STEP`) |
| the arcs' jitter `rand()` | the game's stream | bit 2's drawing pass, bit 4 |
| `csObjectPool` +0x14..+0x7F, +0x124 | no (11.6 (a), addendum) | presentation |
| the follow sprites' `pos` in `fxSpritesDraw` | no (no collision) | presentation (12.7) |
| the HUD radar swap; the message timer and the marker blink | the swap: the radar `worldUpdate` reads | bit 8 (`STEP_SPLIT`) moves the swap into the step; the timers are presentation |
| `debrisStartBurst`'s capture (`renderDrawObjectUnculled`) | its burst slots (state) | deterministic (11.6 (b)); bit 3 stops it stepping animations |
| the GTE and the scratchpad left by the renderer | no: the hit pass loads its own matrices (`ctc2` per object and per type-5 node) and sets its scratchpad state; kinds 1 and 2 use only the GTE's IR / MAC, loaded each time | none needed (CONFIRMED for `hdpPassRun` / `hdpWalk`) |
| the sound listener (`viewProc`) | sound only | presentation |

So with bits 2 and 3 set (and 4 for the arcs' combined pass), no game logic reads what a view's drawing computed
(LIKELY overall: (a) is one run's evidence, (b) the earlier censuses).

### 13.5 Request 48's answer 11.6 (a), re-checked

11.6 (a) said that `csObjectPool` +0x14 has no game-logic reader. **It stands for the probe path**: `hdpPassRun`
reads the root's +0x11C and nothing else of it, and the other objects' +0x80 / +0xA8 / +0x11C (13.1); the
+0x14..+0x74 differences in the dumps are that presentation, written per view. What the stage 2 text understated is
the reach of bit 3 for the port: 11.2 cites "the hit volumes of DISH and FREEWAY" (Moscow and Los Angeles; the
addendum's five nodes) and 11.3 lists 0x01C for the port; request 51 is that case on Moscow with the bits not set.
Without bit 3 the level's collision is per machine on those two arenas (and on any car or object model with volumes
in animation frames).

### 13.6 Checks

No code, header layout or table change (comments and docs only): the retail images (hybrid 9219f7ba..., source-only
d755c86e..., sdk-lib f557c1e3...) and every port variant are unchanged; the package hashes are refreshed
(`build.py --verify-package`). The evidence scripts read the dumps only (the level's tree from `csObjectPool[0]`
+0x11C with the kinds of `hdpWalk`; the car dump's 74,404-byte steps).

### 13.7 Open notes

- **For the user:** a narrower bit (step in the step only the animations whose frames hold hit volumes, leave signs
  and water to the renderer as retail) was not built. It would still change retail on DISH and FREEWAY, needs
  `VISUAL_RAND` for the animators left in the drawing (or their `rand()` parts the streams), and costs a per-load
  classification; bit 3 already gives one rule. If offline should keep retail's off-screen animations while online
  uses bit 3, that is the port's choice of bits per mode, no decomp change.
- Without bit 3 the host's extra `rand()` at an animation's wrap (13.2) parts the game's stream too; the port's
  hash would show it only through the cars.
- `hdpPassRun` walks model-less scene objects (the view cameras) from the node at address 0 (13.4); harmless while
  that byte is not a node kind (retail's behaviour; the port's RAM[0] is the same on every machine). **Corrected in
  17.3:** the view cameras are not on the list `hdpPassRun` walks.

---------------------------------------------------------------------------------------------------------

## 14. Requests 52 and 53 (2026-10-22)

### 14.1 Request 52: the cause (CONFIRMED): the animators draw from a stream that lives outside the state

The port's rejoin test on Moscow with `portRollbackOptions` = 0x1D on every machine (`build/kc-hunt7`:
`lobby_port-dev` the host, `-dev2` machine 2, `-dev4` the machine back, state put in place at step 651). **Nothing in
the decomp's drawing steps an animation with bit 3 set** (14.3), and the joiner's level was the host's (14.2). What
differs is **which `rand()` stream the step's animators draw from**: the port wraps the three animator functions of
the decomp's build (`src/overrides.json`: `80151BC4` `animStepRandom`, `80151CB8` `animStepWeightedRandom`,
`80151DF4` `animPickPeriod`, mapped by `addrmap.tsv` to 80154180 / 80154294 / 801543F0 in this build) with
`src/vplus/interp.c` `visual_rand`: it puts its own `visual_seed` (a host C static) into the BIOS `rand()` seed at
0x80009010 for the call and takes it back after. That predates bit 3 (RAND_CALLERS 2: "visual only"). With bit 3 the
same functions are called **by the step** (`portRollbackAnimStep` and the triggers from the logic), and their draws
pick periods and frames that are simulation (the hit volumes). `visual_seed` is reset per machine at its own round
start (`interp_round_start`, 0x2545F491), advances with every animator call, and is **not in the state transfer**
(not guest RAM, not a `KEEP` block of `vp_keep_register`), the snapshot or the hash. The host and machine 2 ran the
same calls from the same round start, so their two streams agree; the machine back took the host's RAM at 651 but
kept its own `visual_seed`, and every draw after that differs.

Evidence (all three machines' dumps of each step):

| item | host and machine 2 | the machine back | reading |
|---|---|---|---|
| the game's seed (0x80009010) | 0xA8FA4D06 at 700, 0xF3B81B4A at 900, 0x01E4420E at 1000 | the same | the animators' draws are not on the game's stream: the wrapper swaps them away |
| the level's model data (0x80010000..0x80080000) at 700 | equal (host = machine 2 in every dump) | 3 words differ: 0x80074E48 (0x80074E3C +0x0C, the frame), 0x80074E98 (0x80074E94 +0x06, the period), 0x80077098 (0x8007708C +0x0C) | exactly the values picked by a `rand()` since the join; every elapsed counter, flag and select is equal |
| 0x80074E94 (id 3000, loop, 2 frames, on / off periods 600..901, child 0x80074E3C, child frame 1, trigger 0) at 650 | frame 1, elapsed 558, period 647 | (the host's state at 651) | it wraps at step 680 on all three (elapsed 60 at 700 everywhere) |
| the wrap at step 680: `animControl(0x80074E3C, 1)` (`animStepWeightedRandom`: one draw), then `animPickPeriod(.., 0)` (one draw) | 0x80074E3C frame 1, period 837 | frame 3, period 743 | the two draws differ, nothing else |
| step 950 (0x80074E94 in frame 0) | elapsed 810 (period 837) | frame 1, elapsed 66 (it passed 743) | the request's "wraps earlier": its period was drawn smaller |
| 0x8007708C (id 0, random, 12 frames, period 1..2: a draw every step; a model outside the level's tree) | frame 10 at 700 | frame 3 | the stream differs from the first steps after the join |
| step 1992 (0x80074E94 wraps again on all three) | 0x80074E3C frame 4 since 1190: tag 0x1CB | frame 5 since 1639: tag 0x1C9 | the request's contact tags: 0x80074E3C's frames 1-5 are tags 0x1BB, 0x1BC, 0x1BE, 0x1CB, 0x1C9 (a wall each; frame 0 empty) |

The other differing nodes of the later dumps (0x8006ED5C, 0x8006EEB4, 0x800749D4, 0x80074C14, 0x80074D50: loops with
fixed periods, no `rand()`) all lie in 0x80074E3C's frames or 0x80074E94's frame 1: the step walks a frame's subtree
only while it is current (11.2), so they follow their parent's frame. They are a consequence, not a second cause.

Why "one run in four": the animation parts on every rejoin (the joiner's stream is always its own); the cars part
only when one touches a wall that one machine has and another has not (LIKELY). The port's hash (the car records)
sees it only then; the `dmd` rows (12.4) would show it at the first wrap after the join (13.3's advice).

### 14.2 The alternate hypothesis: the state transfer (REJECTED, CONFIRMED)

The port's join sends `killcam_state_save` (`src/vplus/killcam.c`): the whole guest RAM (`GUEST_RAM_BYTES`, so the
level's and the cars' model data with every animation node), the scratchpad, the CPU, the timers and events, the
`KEEP` blocks, then VRAM, the texture banks and the SPU RAM (`net/game_tm2.c` `devices_save`). The joiner loads all of
it (`killcam_state_load`, at its battle's step). So the animation nodes come from the host, not from the joiner's own
load: at step 700 every elapsed counter, parity flag, select and the game's seed equal the host's (14.1), and only the
values drawn from `rand()` after 651 differ. `portDmdMutableList` is not needed for a join (the full RAM covers it);
it is the list for a rollback snapshot (12.2). **What the transfer misses is host-side state the step uses:
`visual_seed`** (interp.c). The other interp.c wrappers of the step's functions (`interp_AnimUpdateAll`,
`interp_AnimUpdateOne`, the HUD ticks, the arcs, the flash) only skip calls while redrawing and hold no stream.

### 14.3 (1) What in the drawing still steps, wraps or triggers an animation with bit 3 set: nothing (CONFIRMED)

Every writer of an animation node's state, by the retail image's call graph (every `jal`; no data word and no
`lui` / `addiu` pair holds the address of an animation function, so there is no call through a pointer):

| function | callers | with bit 3 |
|---|---|---|
| `animStepLoop` 801519DC, `animStepPingPong` 80151ACC, `animStepRandom` 80151BC4, `animStepWeightedRandom` 80151CB8 | `animUpdateAll` 80151700, `animUpdateOne` 801518E8, `animControl` 80151F90 (and `portRollbackAnimStep`) | the first two return (`portRollbackAnimDrawOff`); `animControl` below |
| `animUpdateAll` 80151700 | `renderScene` (80152630) | returns |
| `animUpdateOne` 801518E8 | the walker `renderWalkModel` (80152FEC), so `renderDrawModel` and `debrisStartBurst`'s `renderDrawObjectUnculled` | returns |
| `animControl` 80151F90 | `hdpCollideObject` 8014E66C (the hit leaves that name an animation; from `carsPhysicsUpdate` 8011E220 and the ricochet's 8010CC54 in `worldUpdate`), `hdpTraverse`'s kind 8 (80150020; `hdpQueryPoint` from `bulletsMove`, `fxRecordsUpdate`'s 800DB110, `aiDarkToothBossUpdate`), `animSetStep`, the four animators (a frame's child) | the step only |
| `animSetStep` 80151F14 | `sub_80124384`, `sub_80124B80`, `sub_80124C00`, reached from `targetActorsUpdate`, `worldUpdate`, `bulletsUpdate`, `fxRecordsUpdate`, the level events (Moscow's, LA's, the dish, the Eiffel tower) and the load (`arenaSetup`, `eiffelTowerReset`, `sessionBindCars`) | the step and the load only |
| `animPickPeriod` 80151DF4 | the animators (no other caller) | with them |

None is reached from `viewProc`, `streaksDraw`'s drawing pass, `fxSpritesDraw`, `debrisDraw`, the HUDs or the
present: the camera has no world collision through `hdp*`, LOD selection and the zones only choose what the walker
draws (they write no node), and the walker writes no node except through `animUpdateOne`. So with bit 3 an
animation's step, wrap and child trigger run in the step only, with the same calls in the same order on every
machine. **No decomp change is needed** and none is made.

### 14.4 (2) The fix: the port's wrapper passes through under bit 3

In `src/vplus/interp.c`, `visual_rand` (used by `interp_AnimStepRandom`, `interp_AnimStepWeightedRandom`,
`interp_AnimPickPeriod`): **when the guest's `portRollbackOptions` has bit 3 (0x008), call the original straight**
(`fn(c); return;`), without swapping 0x80009010. With bit 3 the animators are stepped by the step on every machine
alike and their draws belong to the game's stream, which is in RAM (the join, the kill cam's snapshots, a rollback
snapshot and the hash all carry it). That is 11.2's rule ("which `rand()`": the step's animators stay on the game's
stream). Keep the swap when bit 3 is clear (offline with retail's drawing-driven animations, as today).

Alternatives, not recommended: dropping the three overrides in builds with `TM2_PORT_ROLLBACK` and setting bit 4
offline gives the same offline behaviour from the decomp's own stream (`portRollbackVisualRand`, `.port_bss`) but
changes the offline stream (the port's baselines). Registering `visual_seed` as a `KEEP` block would fix the join
only: not a rollback (a loaded snapshot would keep the seed of the discarded steps), nor a kill cam's replay (it
re-runs steps through the swap, then puts the RAM back but not the seed: LIKELY a desync of that machine whenever it
shows a kill cam online with bit 3). A decomp-side workaround (a new bit giving the step's animators a `.port_bss`
stream of their own, out of the swap's reach) was not built: it duplicates the one-line port fix and changes bit 3's
`rand()` order for every user of the bit.

Order of the port's work: the wrapper first (it is what parts the machine back), then hash the `dmd` rows, or at
least the level's type-3 nodes (+0x06..+0x13), so a difference shows at its step. The decomp's builds are unchanged.

### 14.5 (3) DISH's animations and every node kind

The four nodes of the addendum with hit volumes:

| node | kind | `rand()` in its step | in the dumps |
|---|---|---|---|
| 0x80074E94, id 3000 | loop (mode 0), periods 600..901 | `animPickPeriod` at each frame change (max - min >= 2) | parts at 680 (14.1) |
| 0x80074E3C, id 440 | weighted random (mode 3), listed flag 2 (only its parent steps it) | one per trigger | parts at 680 |
| 0x8006F0BC, id 0 | loop, on 150..151, off 0..1 | none (max - min < 2) | equal on all three in every dump |
| 0x8006F08C, id 440 | random (mode 2), listed flag 2 | one per step-now; its parent triggers it with child frame 0 (a reset: no draw) | equal (frame 0) |

Every kind: loop and ping-pong draw only through `animPickPeriod` (when a period range is 2 or more), random and
weighted random once per frame pick, the select (type 9) never (`animSetStep` / `animControl` from the logic). Under
bit 3 all of them run in the step (14.3) and draw from whatever stream 0x80009010 holds, so the port's fix (14.4)
covers every animation of every arena and car model, not only DISH's. FREEWAY's id 440 (Los Angeles) is the same
kind as 0x80074E3C.

### 14.6 Request 53: what it asks for, mapped to what exists (request 48 stages 2 and 3)

Request 53's draft asks for the rollback pieces of 8.x. They were built by request 48 (sections 11 and 12) and its
8.x questions answered there (11.6, 12.7). The names and bits, from `port_rollback.h` (request 53's bit numbers are
the header's):

| request 53 | built | where |
|---|---|---|
| (1) `STEP_SPLIT`, `portRunStep()` / `portStepBody()`, callable from the port's frame hook any number of times | bit 8 `TM2_RB_STEP_SPLIT` 0x100; `portStepBody` (the update half in retail's order), `portRunStep` (the re-run: `portStepBody` whatever the iteration's latch), `portRollbackLoopStep` (the loop's top) | 12.2; `rt_main.c` |
| (1) `portStepEnded` for a round's end inside a re-run | **`portRunStep()` returns 1** (stop the batch) and sets the `.port_bss` word **`portRollbackEnded`**; `rtMainLoop` leaves at its next top with `rtFiredEventValue`, as retail. There is no `portStepEnded` (8.2's sketch name) | 12.1, 12.2, 12.7 |
| (2) `SOUND_GATE` (bit 5) and `portResimulating` | bit 5 `TM2_RB_SOUND_GATE` 0x020; `portResimulating` (`.port_bss`; the port writes it around a batch, no C writer) | 12.2 |
| (2) `portSoundReconcile()` after a batch (CAR_GROUPS 12's loop bits) | `portSoundReconcile()` stops the projectiles' flight sounds 10, 11, 12, 15 whose projectile is gone. The loops (CAR_GROUPS 12's bits, the shared gun loop 9, the engines) need no reconcile: their flags are "no" rows and every start / stop site tests the state each step, so they heal at the next live step | 12.2 |
| (3) `CLUT_SHADOW` (bit 6), `portClutAnimUpload()` | bit 6 `TM2_RB_CLUT_SHADOW` 0x040; `portClutShadow` (game state: the read-backs come from it), `portClutShadowCapture()` (at the loop's entry; the port after a palette reload), `portClutAnimUpload()` (after a batch, a join, a loaded snapshot) | 12.2 |
| (4) `rollback_regions.tsv` per build (name, address, size, snapshot, hash) | `config/US/rollback_regions.tsv` (hand-kept, a reason per row), `tools/rollback_regions.py LINK`, published as `out/TM2_SOURCE_PORT/rollback_regions.tsv` (address, size, name, kind, snapshot, hash, reason) | 12.4 |
| (4) the check that every BSS symbol is in one row | check 8 proved the rows resolve and every static word the step stores to lies in a row; **added now**: every symbol of the static sections (the game's `.sdata` / `.sbss` / `.bss`, `.port_bss`, `.port_data`; bounds from the link) lies in a row (14.7). "One row": the ranges hold the symbols, a symbol row inside a range overrides it (a "no" hole), symbol rows never overlap | `tools/check_rollback.py` check 8 |
| (4) `portDmdMutableList()` | `portDmdMutableList(out, max)`: (address, bytes) pairs of every mutable node field reachable from the level, the world models and every allocated object's model (all LOD bands, frames, select children, zones), sorted and merged; negative = the pairs needed | 12.2 |
| (5) the census: `csUpdMat` +0x14 | no game reader: 11.6 (a) (CONFIRMED for the C, an m2c sweep), re-checked for the probe path by request 51 (13.5) | presentation |
| (5) `csObjectPool` +0x14 | the same field (`csUpdMat` writes `wpos` there) | presentation |
| (5) `csObjectPool` +0x124 (the on-screen flag) | read by `csUpdMat` and `renderScene` only (the addendum, RENDER_ANSWERS R2.3; `renderScene`'s two tests are the only C reads; no port unit reads it) | presentation |
| (5) `padActionCache` | **CONFIRMED** (the addendum, and again here): the retail image references it only from `padClearActionCache` 80150CA4 and `padCheckAction` 80150CFC (three sites: the store, the clear, the read in the unreachable `else if (port == 1)` branch); `padSetActionCache` 80150DD4 has no caller; no port unit reads it | input scratch; in `game.static`'s snapshot (harmless), the hash may leave it out (3.3) |
| (5) 8.6 risk 3: `debrisStartBurst` in a re-run | closed: it writes no packets and no OT entries (11.6 (b)); with bit 3 its walk steps no animation | run as is |
| section 9's answers (2026-10-19) | built as answered: online only, 20 steps a second (`FIXED_TICKS`, ticks 3), the port's pause (`PAUSE_PORT`) | 10, 11, 12 |

The other open items of 1.x and 8.x are closed in place: `rtFieldCounter` (1.1: its getter `rtGetFieldCounter`
800D19D4 has no caller and no pointer), the sound wrappers' results (1.2: every caller discards them, 12.2), the
debris packets (1.2, 1.3, 8.6 risk 3), the census row of 8.5 and 8.6 risk 2. HUMANS16 2.5.2 is finished there. What
stays the port's: interrupt delivery during a batch (1.2, 8.6 risk 7) and its own registry (8.6 risk 6).

**For the port's rollback (request 53's purpose):** `portRollbackOptions` = 0x3FF (12.3), bits 2, 3 and 8 together
(12.7), the snapshot = the "yes" rows plus the `dmd` list, and **14.4's wrapper fix first**: in a re-run
(`portRunStep`) the step's animators draw too, and through the swap they would draw from `visual_seed`, which a loaded
snapshot does not restore, so a re-run would pick other frames than the first run (the kill cam's re-run check would
fail on DISH and FREEWAY, and on any model with random animations).

### 14.7 Checks

- `tools/check_rollback.py` check 8, extended: on `work/pv/r50b_port_P4` (every port define with `TM2_PORT_ROLLBACK`)
  "100 rows resolved; the step (879 functions from portStepBody) stores to 1254 static words, 0 outside every row;
  929 symbols of the static sections, 0 outside every row -> ok". **Negative control:** a copy of the list without
  the `port.data` row fails the new part alone (the store census still finds 0 uncovered): "3 outside every row ->
  FAIL" (`D_8017E15C`, `carViewHotkeyReleased`, `viewModeTable`); without `port.bss` both parts fail (20 stored words,
  122 symbols).
- No game code, header layout or table change (comments, docs and the check only): the retail images and every port
  variant are unchanged; `tools/check_all.py` passes; the package hashes are refreshed (`build.py --verify-package`).
- The evidence reads the dumps only (the RAM files of `build/kc-hunt7`, the level's tree from `csObjectPool[0]` +0x11C
  with `hdpWalk`'s kinds) and the port's sources read-only (`src/overrides.json`, `src/vplus/interp.c`,
  `src/vplus/killcam.c`, `src/vplus/fixes.c`, `src/net/game_tm2.c`).

### 14.8 Open notes

- **For the user / the port:** 14.4 is a port change (one test in `visual_rand`). Until it lands, online with bit 3
  every machine that joins mid-match (and LIKELY one that shows a kill cam) runs its own animation draws.
- Offline the wrapper may stay as it is (bit 3 clear: retail's drawing-driven animations, kept off the game's stream).
  If the port moves offline to bit 3 too (answer 2), the same pass-through applies.
- The hash: add the `dmd` rows (or the level's type-3 nodes) to the port's state hash; this case would then show at
  step 680, not 1992.

---------------------------------------------------------------------------------------------------------

## 15. Requests 54 and 55 (2026-10-22): the node list, the hash rows, the follow object, a round's end

The port's re-run check (`src/net/rb_state.c`, `TM2PC_TEST_RERUN=K,EVERY`, `portRollbackOptions` 0x17D) found three
things (54) and asked for a peek at the round's end (55). Behind `TM2_PORT_ROLLBACK` as before: the retail images are
unchanged (hybrid 9219f7ba..., source-only d755c86e..., sdk-lib f557c1e3...). Addresses: the port's build (every
define of its `tools/decomp/wc.py` plus `TM2_PORT_HOOKS` and `TM2_PORT_FAR_LOD`, without its overlay units:
`work/pv/r54_port_P4`); other builds: the build's `symbols.tsv`.

### 15.1 Request 54 (1): the node list covers every model file loaded for the match (CONFIRMED, built)

**The cause.** Stage 3's `portDmdMutableList` walked what the objects held at the call: the level model
(`csGetRoot()` +0x11C), the world models and every allocated object's model. A model of the level file that no
object held yet was missing. The level file is more than the arena: FREEWAY.DMD has 6 roots, the arena (root 0, id 1,
1,409 nodes), the camera-attached model (root 1, id 2) and four roots of models the logic hands to objects when it
makes them (effects, weapons, pickups). Node 0x80078EEC (a loop, period from `animPickPeriod`, its random child
0x80075B04) is in root 2 (0x8007D54C, 142 nodes), not in the arena's tree: an object made between steps 50 and 54
took that model, and only then did the walk reach it.

**The design: a walk of the files, not of the objects.** A model can only come from a loaded file (the game has no
heap: FILE_FORMATS.md 0), and the files are few and fixed per load:

- the **level file** at 0x80019C40 (`sessionLoadArenaAndCars`, never relocated): every root of its header;
- every **installed file**: `assetInstallImage` (the match's cars and spare groups, a drop-in's
  `portCarLoadGroupLive`) now notes `(base, group)` in **`portDmdImages`** (`.port_bss`, every variant, after
  `portHumanVacant`; `struct TM2PortDmdImages`: `base[64]` by load group, `overflow`) through `portDmdNoteImage`;
  `portRollbackPreload` (the load's start) clears it, `portCarUnloadGroup` clears its group.

`portDmdMutableList` walks every root of each file as `assetInstallImage` reaches them (root word + base -
0x80019C40; a zero root stops), through every child as before (all LOD bands, animation frames, select children, both
zone children), with the same per-type fields, then sorts and merges. The list depends on the loads alone: **the same
for every step of the match**, whatever object holds which model; the walk of the live objects is gone.

Evidence:

- **The disc's 29 DMD files** (`tools/dmd_lod_dump.py`'s reader, the walk modelled in Python): every node that a
  chained pointer names (animation +0x1C, select +0x0C, hit box +0x0C; 0..10 per file) is also a child in the same
  file's tree, so walking the children reaches every node a step can step or trigger. FREEWAY's 0x80078EEC and
  0x80075B04 are in the list from the load on.
- **A RAM dump of the port's build** (`build/kc-hunt7`, Moscow, step 1000): the old walk reached 4,181 nodes, the
  files' walk (DISH.DMD at 0x80019C40 and 8 car files at 0x8063EC84..0x806781D0) 4,239. Every node of the old walk is
  in the files' walk but 16: the projectiles' trail nodes in `fxRecordTrailSegments` (`.port_bss`), which the
  `port.bss` range snapshots whole (as the car groups' freeze records in `portCarGroups`); 74 nodes of the files
  (models no object held at that step) were missing from the old list.
- Sizes: the raw pairs (before the merge; a shared subtree is walked once per parent) are at most 11,093 for a level
  (DEN; ISLANDS 10,959, ROOF 4,761, DISH 1,336, FREEWAY 1,396) plus 34..103 per car file; the merged list 542..1,478
  pairs per level plus 32..76 per car. A buffer of 16,384 pairs (128 KiB) holds any match of 16 cars.

**What the port does:** `portDmdMutableList(buf, max)` after the match's load and after each
`portCarLoadGroupLive` / `portCarUnloadGroup` (a file in or out); **nothing else changes the list**: not a car swap,
`portCarStartHuman` / `portCarRemove` / the hand-overs (they use loaded files), objects made or freed, the steps (12.3
asked for a re-query after those; no longer needed). A negative result: retry with a larger buffer. The ranges the
request offered as an alternative are not needed: the list is exact and about 1% of the level's bytes (the whole
0x80010000..0x80080000 area of the test setting is 448 KiB a state).

**The other path for nodes:** nodes outside the files (the projectiles' trails, the freeze records, the car-number
and hidden groups of the port's car code) are all in `.port_bss` or the game's BSS, inside the `port.bss` /
`game.static` ranges.

### 15.2 Request 54 (2): the hash rows hold game state only (CONFIRMED, tsv changed)

**Which of the listed words the step reads** (the static census of the port's link: every function that names the
word, reached from `portStepBody` (the step), from the drawing roots of `rtMainLoop` (the drawing), or from neither):

| words | written by | the step reads it? | row now |
|---|---|---|---|
| `portMeshList`, `portFarState` | `portFarDistanceMeshListBegin` / `Emit` / `Scale` (`renderScene`, per drawn view) | no | snapshot no, hash no (`symbol?`: `TM2_PORT_FAR_DISTANCE`) |
| `renderOtView1Buf0` / `1`, `renderOtView2Buf0` / `1` | `ClearOTagR` and the renderer, through the frame records | no (the debris capture writes no OT entry, 11.6 (b)) | no, no |
| `renderObjectMatrices`, `renderMatrixSlots`, `renderSlotMatrixPtrs`, `renderFrustumMatricesA` / `B`, `renderWalkerSaveRegs` | `renderScene` / `renderInitState` per drawn view | only `debrisStartBurst`'s capture (`renderDrawObjectUnculled` -> `renderInitState`), which fills them first; its result (`debrisBursts`) is presentation | no, no |
| `renderCurrentOt`, `renderFarDistance`, `renderSkipNodeCull` (and the drawing's `fxSpritesDrawParity`, `hudOrderingTable`: found by check 9) | `renderScene`; `renderDrawObjectUnculled` sets the three before `renderInitState` reads them | no (set before use) | no, no |
| `csObjectPool` (54 words) | **the per-view fields**: `csUpdMat` writes +0x14..+0x1F (`wpos`) and the matrices +0x20 / +0x40 / +0x60 of each listed object per drawn view (and +0x124, the on-screen flag), `viewProc` the root's +0x14 (RENDER_ANSWERS R2.3; 54 words = two objects' +0x14..+0x7F) | no (11.6 (a), 13.5: the hit pass reads +0x08, +0x0C, +0x80, +0xA8, +0x11C) | new `field` rows `csObjectPool.view` (+0x14..+0x80) and `csObjectPool.onscreen` (+0x124) of every element: snapshot yes, hash no |
| `D_801A5398`, `D_801A5880`, `D_801A5200`, `D_801A5380`, `D_801A5878`, `D_801A5AF4` (libsnd), `D_801B2D50`, `D_801B2D66`, `D_8017E15C` (libspu) | the sound library under the driver | no: the step reaches the sound side only through the 10 wrappers and `soundUpdate` (with them cut, no driver or library function is reachable from `portStepBody`), which `SOUND_GATE` skips in a re-run | no, no |
| the sound manager's `soundLoopKillFrom`, `soundLoopKeepCount`, `soundLoopCandidateCount`, `soundActiveTail`, `soundActiveCount`, `soundKeyedOnCount`, `soundInstFreeCount`, `soundInstSearchStart`, `sndObjDistance`, `sndObjDistanceVolume`, `sndObjVolumeRight` | `soundDriverUpdate` (from `taskMgrRunFrame`, from `soundUpdate`), `soundInstAlloc` / `Free` / `KeyOff`, `soundOn`, `sndObjUpdate` | no (as above); the rows cover the whole driver block (`soundLoopKillFrom`..`soundInstSearchStart`, 12 words, and `sndObjNeedVoices`..`sndObjDistanceDelta`, 10) | no, no |
| `taskMgrRunCurrent` (and `taskMgrCheckCursor`, `taskMgrRunNext`, `taskMgrStopNext`) | `taskMgrRunFrame` (from `soundUpdate`, skipped in a re-run, and `pauseMenu`) | set before each task runs, read only after it: a cursor within one call | no, no |
| `timerTicsAccum` | `SysClkIntHandler` (the root counter interrupt), `InitTimer` | `GetCurTics` (below): real time | no, no |

**Is anything the step does decided by `GetCurTics` with `FIXED_TICKS` on? No (CONFIRMED).** The callers reachable
from `portStepBody`: `SetFrameStart` (the clock read is discarded: `portRollbackFixedFrame` returns before every
store, check 4 "frame measurement"), `humanCarTick` (four calls, results discarded), `viewCarCameraUpdate` (two,
discarded), `worldUpdate` (two, discarded), `padLatchWithRepeat` (only in pad repeat mode 0: `rtMainLoop` sets mode 1
at its entry, and `padUpdate` runs on live steps only). No `VSync` and no `GetRCnt` is reachable from the step. The
other clock users are outside the step: `rtMainLoop`'s pacing (its local `start`), `ResetUpdateRate` (a held
iteration without `FIXED_TICKS`), the three `srand(GetCurTics())` seed sites (the load and each round start: bit 1
`SEED` replaces them; the port's 0x17D leaves bit 1 clear, 15.7). `soundRepeatThrottle` runs on `timerFrameCount`
(state) and draws its `rand()` every call; what it decides is whether sound 0x23 plays. So `timerTicsAccum` is not
state: the port keeping it live across a batch is right.

**Sound "playing" tests deciding gameplay: none (CONFIRMED).** The game's only query of what plays is
`soundIsActive` (the `soundActiveId*` flags, "no" rows since stage 3); its 7 callers
(`weapon_fire_specials.c:384`, `weapon_flame_update.c:152`, `weapon_ricochet.c:220`, `world_update.c:484`, three in
the port's car sound code) each decide only a `soundStop`. Every caller of the 10 wrappers discards the result (12.2).

**More drawing state found by the new check 9** (the drawing's static stores into "hash yes" rows):
`fontCursorX` / `Y`, `hudSplitDivider`, `hudPlayerRecords` (the HUDs), `viewAxisScaleMatrix` (`viewProc`),
`portVisualState` (`TM2_PORT_VISUAL`'s tails): snapshot yes (a step writes or reads some of them), hash no;
`fxSpritesDrawParity`, `hudOrderingTable`: no, no. `soundThrottleLastTime`'s reason is corrected (frame count, not the
clock). **`portDmdImages`**: no, no (load-time bookkeeping).

**Rows changed** (`config/US/rollback_regions.tsv`, 116 -> 183 lines): 54 new "no / no" rows (the OTs, the renderer's
scratch, the far-distance lists, the libsnd / libspu words, the sound driver's 22 words, the task manager's 4
cursors, `timerTicsAccum`, `fxSpritesDrawParity`, `hudOrderingTable`, `portDmdImages`), 6 new "yes / no" symbol rows
and the two `field` rows; the `dmd` row's reason; a new row kind **`field`** (`SYMBOL:STRIDE:LO-HI`, resolved by
`tools/rollback_regions.py` into one row per element, `csObjectPool.view[i]`; 640 rows in the port's link, whose
`csObjectPool` has 320 elements). The resolved list of the port's build has 800 rows.

**The check (new check 9 of `tools/check_rollback.py`).** The drawing is what `rtMainLoop` calls outside the step
(`viewProc`, `streaksDraw`, `fxSpritesDraw`, `debrisDraw`, `renderScene`, the three HUD drawers) and all they reach,
less the edges the bits cut (`streaksDraw`'s per-view ticks and its split functions, whose drawing pass stores
nothing; the animators under `animUpdateAll` / `animUpdateOne`; `hudDisplayRadar`'s swap). (a) Every static word the
drawing stores to lies in a "hash no" row (or in no row); (b) every listed word of the request and every per-view
word of every `csObjectPool` element lies in a "hash no" row. **Negative control:** with request 53's list (`git show
HEAD:config/US/rollback_regions.tsv`) it fails: on `work/pv/r54_rb_P4` "the drawing stores to 10 hashed static words; 34 of the request's 34 words and csObjectPool's per-view fields hashed -> FAIL" (`fontCursorX` / `Y`, `fxSpritesDrawParity`, `hudOrderingTable`, `hudPlayerRecords`, `hudSplitDivider`, `renderCurrentOt`, `renderFarDistance`, `renderSkipNodeCull`, `viewAxisScaleMatrix`). The census sees direct stores (as check 8); the OTs and
`csObjectPool`'s fields are written through pointers, so (b) names them.

### 15.3 Request 54 (3): the follow object is the same on every machine; no node is stepped twice (CONFIRMED)

`viewGetFollowObj(0)` is not the car view 0 follows. It is `viewFollowObjects[0]`, the **camera-attached model** (the
level file's id-2 root, FREEWAY's root 1: the sky), which `worldLoadObject` hands to `viewSetFollowObj` at the load;
the renderer draws it at each view's camera (`viewSetFollowObjPos`). The camera that follows the player is
`viewCameraObjects[i]`. `viewFollowObjects` is written only by `viewSetFollowObj` (at the load) and `viewInit`;
neither the step nor the drawing reaches them (check 5). So every machine walks the same object, the same nodes, in
the same order: the walk's order is the list, the level, the world models, the follow object, then `csListHead`'s
objects with +0x08 set, all state or load-time.

A node reached twice in one step (the follow object's nodes are also in the level file; a model shared by several
objects) is stepped once: `portRollbackAnimStepOne` (and the registered list's loop) steps a node only while its
`done[renderFrameParity]` is clear and then sets it, and `renderFlipFrame` (earlier in the same step) flips the parity
and `done[renderPrevFrameParity]` is cleared at each visit; `done[]` lives in the node (the `dmd` rows) and the parity
words are hashed rows. The first walk that reaches a node steps it, the same walk on every machine. **No change**: the
follow walk stays (dropping it would change nothing, the parity guard already dedups; keeping it keeps stage 2's
order).

### 15.4 Request 55: `portRollbackEndDue()` (built)

`int portRollbackEndDue(void)` (`rt_main.c`, `.port_text`, 808C46E0 in the port's build, 80851F20 with `TM2_PORT_ROLLBACK` alone): **1 when the next `portStepBody` (live or
re-run) returns 1 at once**, the round over. `portStepBody`'s first call, `rtUpdateScheduledEvents`, subtracts
`(short)timerGetFrameTicks()` (the last `SetFrameStart`'s ticks; `FIXED_TICKS`' constant) from each of the
`D_801809D4` pending events and fires one whose time drops below zero. `portRollbackEndDue` makes the same test
without the subtraction or the removal: it calls `timerGetFrameTicks`, reads `rtScheduledEvents` and the count,
stores nothing (check 1). Call it between iterations of `rtMainLoop` (the port's hook), any number of times.

**Why the peek is exact and early enough:**

- **Every round end is a scheduled event.** `rtMainLoop` has one exit, `RB_EVENTS()` non-zero: retail's
  `rtUpdateScheduledEvents`, or with `STEP_SPLIT` `portRollbackLoopStep` (a pending `portRollbackEnded`, or
  `portStepBody`, whose only `return 1` is `rtUpdateScheduledEvents`'). Nothing in the step blocks or leaves
  otherwise: no `VSync`, CD read, movie, `exit` or pad wait is reachable from `portStepBody`; `csAlloc`'s and
  `viewCreate`'s `printf` return.
- **An event fires no earlier than the step after the one that scheduled it, before that step reads an input.**
  `rtUpdateScheduledEvents` runs first in the step, before `padUpdate` and every update. The schedulers with time 0
  fire at the next step's top. So whether step N+1 ends the round depends on the state at the end of step N only
  (the inputs up to N), never on step N+1's inputs.

**Everything that schedules a round end** (the callers of `rtScheduleEvent`, port build):

| scheduler | where | event, delay | predictable before the step? |
|---|---|---|---|
| `humanCarDamageUpdate` (`car_damage_human.c`) | the step | a human wrecked: 1 (4 / 3 in mode 4), 210 or 420 ticks (not with the port's rules, H5) | scheduled in the step (from its inputs); fires 70-140 steps later: `portRollbackEndDue` sees it the step before |
| `worldUpdate` | the step | the last computer car down: 3, 210 / 420; the attract demo's pad press: 1, 0 | as above / the demo only |
| `aiCarsCountRemaining` (`scene_init.c`, from `worldUpdate`) | the step | Dark Tooth's second stage: 3, 0 | fires at the next step's top: the peek after the step that scheduled it |
| `demoRespawnAiCars` (`spawn_tables.c`) | the step | the attract demo's fifth round: 1, 0 | the demo only |
| `portRoundEnd` (H5, `portHumanOptions` bit 1) | the port, between steps | the port's event and delay | the port's own call |
| `portRollbackTop` | the loop's top, before a live step (not `portRunStep`) | `portRollbackQuit` with `PAUSE_PORT`: 2, 0 | the port's own input (`portRollbackQuit`), fires in that same step |
| `pauseMenu` (without `PAUSE_PORT`) | the drawing half | retail's QUIT: 2, 0 | fires at the next step's top; the pause menu is this machine's (the port's: it said so) |

`sessionRun` clears the events when `rtMainLoop` returns (`rtClearScheduledEvents`), so nothing is pending at the
next round's first step.

Other ways the loop is left: none in the step. **`portRollbackEnded`** (a re-run fired an event: `portRunStep`
returned 1) makes the next top leave without a step; the port caused it.

**What the port does (offered as the protocol):**

1. Before each **live** step (the top of the iteration, `portRollbackLoopStep`): if `portRollbackEndDue()` is 1 and
   any input up to the last step run is still a prediction, hold (`portStepHold` / `HOLD`, the drawing only) until
   they are confirmed; a rollback with the corrected inputs re-runs the steps, then ask again (the corrected steps
   may not schedule the end, or schedule it at another step). Once the inputs are confirmed, an answer of 1 is the
   same on every machine: run the step; the loop returns `rtFiredEventValue` with every machine at the same step.
2. In a **batch** (`portRunStep`): the steps re-run on corrected inputs; `portRunStep` returning 1 means the end fired
   in the batch (the same test, at that step). With rule 1 a batch never re-runs past a confirmed end: the end can
   only become due inside the batch at a step whose inputs are confirmed, or the port stops the batch (and holds)
   when `portRollbackEndDue()` is 1 before a step whose predecessors are not all confirmed.
3. `portRollbackQuit` (and the H5 `portRoundEnd`) are inputs of the port's own: send them in the input frame of a
   confirmed step.

### 15.5 Functions and words

Changed game function: **`assetInstallImage`** (`asset_header.c`: `portDmdNoteImage(base, group)` after the
version check): 36 with `TM2_PORT_ROLLBACK` alone (check 7). Port functions (`.port_text`; the port's build):
`portDmdNoteImage` 808C2A2C, `portDmdMutableList` 808C2A70 (reworked), `portRollbackEndDue` 808C46E0; with `TM2_PORT_ROLLBACK` alone 80851AF8, 80851B3C, 80851F20. The stage-3 neighbours moved with `.port_text`: `portStepBody` 808C44C4, `portRunStep` 808C4624, `portRollbackLoopStep` 808C467C, `portRollbackAnimStep` 808C4E50; `assetInstallImage` 801222A8. In `port_variant_arrays.c` the reworked `portDmdMutableList` and the static `portDmdWalkFile`;
`portRollbackPreload` and `portCarUnloadGroup` clear `portDmdImages`. None calls `rand()`.

New word: `portDmdImages` (808AC968 in the port's build, 80850A2C with the define alone, 0x104 bytes), after `portHumanVacant` (nothing before it moves); `.port_text` moves
by the new and changed code (`portRollbackEndDue` 808C46E0 in the port's build, 80851F20 with `TM2_PORT_ROLLBACK` alone).

### 15.6 Checks and proofs

- `tools/check_rollback.py`: check 1 (`portDmdMutableList` reads `portDmdImages` and no live object tree;
  `portDmdNoteImage`; `portRollbackEndDue` calls `timerGetFrameTicks` only, reads the events, stores nothing), check
  4 (`assetInstallImage` notes the file before relocating it: one call dominating `modelRelocate` and
  `dbscanHierarchy`; the writers of `portDmdImages`), check 5 (`portDmdImages`' users; `viewFollowObjects` written at
  the load only), check 7 (36 functions), check 8 (the new rows resolve; the store census still finds no uncovered
  word), **check 9** (15.2). Negative controls: with request 53's list check 9 FAILs (15.2); on the full build's port link without the define (`--negative-control`) checks 1-5 FAIL as expected, the new parts included ("assetInstallImage calls portDmdNoteImage 0 times", `portDmdNoteImage` and `portRollbackEndDue` missing).
- Proofs (`verify_reloc.py` against E0 of `work/build-20261022-161107-135568`, each with its placement link, limits forced to 2): `TM2_PORT_ROLLBACK` alone `6f876ca6c77c…` (P2 `fb93bf9b2559…`; port_changed: the 36 functions; port end 0x80852630); `TM2_PORT_CARS=16` stack with humans (`hs`) `f73d378d1586…` (P2 `ebfaefb2fb46…`); the port's define set (`port`, with `TM2_PORT_CAR_DROPIN`) `db0379f09698…` (P2 `fa570af6a662…`; port end 0x808C5230); every port define + `TM2_PORT_HUMANS=16` + `TM2_PORT_RENDER_SLOTS=400` + `TM2_PORT_ROLLBACK` (`allh16`) `2a33ca19f5ea…` (P2 `22faf69b1a5c…`; port end 0x808C6990, RAM end 0x80C00000). The variant without port defines `bb0166991253…` (moved only by `.port_bss`'s new 0x104 bytes). Retail unchanged: hybrid `9219f7ba…`, source-only `d755c86e…`, sdk-lib `f557c1e3…`. `tools/check_all.py` on the full build `work/build-20261022-161107-135568` and the eight links: 31 checks, 0 failed; `build.py --verify-package` passes.

### 15.7 Open notes

- **For the port:** 0x17D leaves bit 1 `SEED` clear, so `uaInit`, `arenaSetup` and `roundStart` seed `rand()` from
  the clock (`srand(GetCurTics())`), outside the steps but at every load and **every round start**. Unless the port
  sets 0x80009010 itself at those points, the machines' streams part at each round. Bit 1 (with `portRollbackSeed`
  the host's) makes them the same; 10.2.
- The drawing check (9) sees direct stores; a word the drawing writes through a pointer and that is hashed would
  still escape it (the request's list and `csObjectPool`'s fields are named for that reason). The port's re-run
  check remains the final proof; new words it finds go into the tsv and into `DRAW_STATE`.
- `portDmdImages` overflow: a load group of 64 or more (none in the port's limits: `TM2_PORT_DROPIN_GROUPS` is 64,
  groups 1..63) is counted in `overflow` and not listed.
- A drop-in loads between steps; a rollback across it would need the file's bytes in the snapshot (the car DMD area
  `portCarDmdArea` is in `.port_bss`, so with `TM2_PORT_CAR_OPT_HIGH_DMD` it is). The port's design keeps loads out of
  the rollback window.

## 16. Request 56 (2026-10-22): `fxSprites[i]` +0x2C, the owner pointer

**The word.** `fxSprites` (150 effect sprites of 48 bytes, `fx_sprite_spawn.c` `struct TM2FxSprite`) +0x2C is
`owner_pos`: the position pointer `fxSpriteSpawn` (80110918) was given, stored as is (`o->owner_pos = pos`). Only the
spawn writes it, so it is written by the step, not by the drawing.

**Why a re-run stores another value.** Most spawns pass a vector on the caller's stack (the explosions
`fxExplosion6..192`, the smoke bursts, `fxSmokeC6`). The stored pointer is then a stack address, and
`portRunStep` called from the port's hook runs the step deeper in the stack than `rtMainLoop` does: 801FF618 in the
first run, 801FF218 in the re-run (0x400 apart), as the request found. The address is meaningless once the spawn
returns.

**Its readers.**
- `fxSpritesDraw` (world_collapse.c), only for a sprite with the follow flag (+0x01) set: it copies `*owner_pos` into
  the sprite's position. Every follow spawn passes a vector that lasts: `fxSparkBlue(v, 1)` (`mathJitterResult`, a
  static; `ai_link_node_pick.c`) and `fxLightningSpark(actor->position, 1)` (an object's position;
  `weapon_slot_scan.c` `SCAN_SLOT`). Every stack-vector spawn passes follow 0, so the drawing never reads a stack
  address.
- The port's car-reference helpers (`port_variant_arrays.c`: drop-in removal, the record move, the hand-over) test
  whether `owner_pos` points into a car record (`portDropInIn`). A stack address is never inside one, so their result
  is the same whichever stack address is stored.

Nothing else reads the word, so its value never changes the game.

**The row.** `fxSprites.owner	field	fxSprites:48:0x2C-0x30	yes	no` (`config/US/rollback_regions.tsv`, next to the
`csObjectPool` field rows): snapshot yes (a follow sprite's pointer must come back with a state), hash no. With it,
the port can hash the whole table and compare it between machines.

**Not changed:** no code. A follow sprite's position is still copied by the drawing only (section 12's open notes): it has no
collision and no logic reads it, and the row for `fxSprites` positions stays as section 12 left it.

---------------------------------------------------------------------------------------------------------

## 17. Request 57 (2026-10-22): the table's hash between two machines

The port now compares the table's hash rows between machines (`rbs_hash`: the rows, the nodes, `vp_rand`) and diffed
the hashed words of two copies at a step whose hashes differed (`net_lockstep.py`, `TM2PC_NET_HASHDUMP`). Three groups
of words: (1) each machine's own, (2) the iteration's buffer flip, (3) `csObjectPool[107]` +0x80..+0x8B. **All three
are presentation in hashed rows; none is game state that the step reads, so nothing in the step changes: no code, the
rows and check 9 change** (`config/US/rollback_regions.tsv`, a new row kind `ptr` in `tools/rollback_regions.py`,
`tools/check_rollback.py` check 9). Retail and every variant are unchanged (no source file is touched).

Addresses: retail, and the port's build of the dumps (`build/decomp-wc/out/TM2_SOURCE_PORT/symbols.tsv`:
`csObjectPool` 805FCA00) or the port's link `work/pv/r54_port_P4` (`csObjectPool` 805FC9C0); the static reach is that
of check 8 / 9 (the `jal` graph from `portStepBody`, and from the drawing roots).

### 17.1 (1) Each machine's own words (CONFIRMED; rows hash no, snapshot no)

| word (retail; r54_port) | written by | the step reads it? | row |
|---|---|---|---|
| `padRecvBufferPort1` / `Port2` (801B2D80 / 801B2DA4, 0x24 each; 801B5720 / 801B5744) | the pad driver's interrupt (this machine's controllers) | only `padDecode`, through `padLatchWithRepeat` from `padUpdate`, which `portStepBody` calls on **live steps only** (`if (!portResimulating)`); `padAnalogAxis` and `padDecodeCopy` have no caller. The latch it fills (`padButtonsLatched`, `padButtonsDecoded`: yes / no since stage 3) has no game reader with `INPUT_ALL`: the humans' input is `portHumanPad` | no, no |
| `portSpuExt` (`.port_bss`, 0x54, `TM2_PORT_SPU48`: 808AD75C) | libsnd / libspu (voices 24..47: key on / off, reverb, envelope words) | only the sound library: reached from the step through `soundUpdate` -> `taskMgrRunFrame` -> `soundDriverUpdate` -> ... -> `soundOn` -> `sub_8015C7D0` / `sub_801619CC` -> `sub_801586A4` / `sub_8015AD64`; **with the 10 wrappers and `soundUpdate` cut (SOUND_GATE's sites, check 4) no function that names it is reachable** | no, no (`symbol?`) |
| `D_801A5AFC` (8 bytes, after request 54's `D_801A5AF4`; 801A849C) | libsnd (`sub_801586A4`, `sub_80158F54`, `sub_8015A560`) | the same: the sound library only | no, no |

Every caller of the sound wrappers discards their results (15.2), so none of these decides anything in a step.
"Snapshot no" as request 54's libsnd rows: a re-run reaches none of them (SOUND_GATE, live-only `padUpdate`), and
putting them back would only replay this machine's own audio and pad history.

### 17.2 (2) The iteration's buffer flip (CONFIRMED; rows hash no, snapshot no; check 9 extended)

| word | written by | readers | row |
|---|---|---|---|
| `D_80180EE4` (view 0's frame record) | `rtMainLoop` at each iteration's top, outside the step (`buffer = &renderView0Frames; if (D_80180EE4 == buffer) buffer++; ... D_80180EE4 = buffer`), its entry, `rtDoubleBufferInit` | the drawing (`renderGetCurrentFrame` from `hudDisplayRadar`; rtMainLoop's `PutDrawEnv` / `DrawOTag` / HUD parity); **in the step only `rtToggleRearView`** (a local human's rear-view hotkey, `carSetViewMode` / `carViewHotkeys`), which uses its parity to pick `renderBufferRearView`: drawing state again | no, no |
| `renderBufferView1`, `renderBufferRearView` | `rtMainLoop`'s split-screen / mirror drawing (flips them each drawn iteration); `rtToggleRearView` (above) | `rtMainLoop`'s drawing only | no, no |
| `D_80193950` (libgpu's `ctlbuf`, 256 bytes: `gpu_control.c`, the low byte of the last GP1 command of each kind) | `_ctl` (from `PutDispEnv` after the step, `ResetGraph`) | `_getctl`; no step function names it. +0x05 is the display start's low byte: 0x00 / 0x40 (x 0 or 320) by the displayed buffer: the word +0x04 = 0x10600000 / 0x10604000 | no, no |
| `rtFrameTime` (found by the extended check) | `rtMainLoop`'s entry and `portRollbackFrameTime` | `rtMainLoop`'s wait for the frame time only | no, no |

None changes what a step computes. Snapshot no: a state put back keeps the iteration's current parity (the drawing goes
on from its own buffers; a restored older parity would have been harmless too, the frame records are "no" rows).

**Check 9 now covers the iteration's own stores.** Besides `DRAW_ROOTS`, the drawing is (a) every other callee of
`rtMainLoop` that the step does not reach, less the loop's bookkeeping (`DRAW_LOOP_SKIP`: the latch and gates, the
entry's `padSetRepeatMode` / `animEnable`, `pauseMenu`): `DrawOTag`, `PutDispEnv`, `VSync`, `timerTicsToFrameRate`,
two libgpu helpers (`sub_80141938`, `sub_80141AA4`) and, with `TM2_PORT_FAR_DISTANCE`, the packet-buffer helpers (6
in `r54_rb`, 8 in `r54_port`); and (b) the static stores of
`rtMainLoop` itself and of `portRollbackLoopStep` (outside `portStepBody`: `portRollbackEnded`). The request's words
join `DRAW_STATE` (part (b) of the check: they must lie in a "hash no" row; libgpu's and the pads' are written through
pointers or by the interrupt, so the store census cannot see them). `VSync`'s two words (`D_8017CFD4` / `D8`, `.data`)
lie in no row (neither snapshotted nor hashed).

### 17.3 (3) `csObjectPool[107]` +0x80..+0x8B: a camera's rotation matrix, written by the drawing only (CONFIRMED)

**The object.** From the two dumps (`build/test-net/port-dev3`, `-dev4`, `state.txt.hash`, step of the request; the
files were replaced by a later run after they were read): element 107 is **a view's camera object**,
`viewCameraObjects[L]`: model 0 (+0x11C), +0x08 and +0x0C 1 (`csReset`'s), parent (+0x10) = element 108, the next
element (the view's pose object `viewCameraObjectTable[L]`: `viewCreateWithCamera` allocates it right after
`viewCreate` allocated the camera object, and `viewSetParent` links them). rot (+0xA0) = (78, 0, 1009), pos (+0xA8)
= (2001, -5838, 332): equal on both machines. The words that differ are +0x80, +0x88 and +0x8C (+0x84 and +0x90 are
equal): the matrix `m[0][0..1]`, `m[1][1..2]`, `m[2][0..1]`:

| | m00 | m01 | m02 | m10 | m11 | m12 | m20 | m21 | m22 |
|---|---|---|---|---|---|---|---|---|---|
| dev3 | 75 | -4095 | 0 | 4065 | 74 | -489 | 488 | 8 | 4066 |
| dev4 | 88 | -4095 | 0 | 4065 | 87 | -489 | 488 | 10 | 4066 |
| `RotMatrixYXZ` of (78, 0, 1009), rot now | 94 | -4095 | 0 | 4065 | 93 | -489 | 488 | 11 | 4066 |

The pitch terms (m12, m20, m22: x = 78) agree; the yaw terms (cos z in m00, m11, m21) are exactly those of z = 1012
on one machine and z = 1010 on the other, while rot holds 1009 on both. **Each matrix is `RotMatrixYXZ` of an older rot: the rot of the last time
that machine drew the view.**

**The writer.** `viewProc` (80155E80; the drawing, a `DRAW_ROOTS` entry and the port's view drawers `four.c`,
`photo.c`): `RotMatrixYXZ(&v->rot, &v->rotm)` and `RotMatrixYXZ(&v->parent->rot, &v->parent->rotm)` for the view it
draws, then the frame's camera from them. Nothing else writes a camera object's +0x80 (the step's camera code writes
rot and pos: `viewSetRotation` / `viewSetPosition` from `viewCarCameraUpdate`, `carSetViewMode`, the missile camera;
the pose object's matrix too, `viewSetCameraPose`, from its rot). Static evidence (the port's link): of the 17 functions
that name `viewCameraObjects`, only `viewProc` writes through a camera object's matrix; in the drawing set (check
9's, extended), `RotMatrixYXZ` and `viewCopyWords` are called by `viewProc` only.

**The readers: none in the step.** `viewProc` itself (the frame's camera); `viewGetRotMatrix` (80156468) has no
caller. Camera objects are **never on `csListHead`'s list** (`viewCreate` / `viewCreateWithCamera` only `csAlloc` them;
none of `csListAdd`'s callers, the pools, the weapons' objects, the debris, the warps, the actors, the trails,
`dbInitCsForAModel`, the registered world objects and the port's car objects, passes a view's object), and
`hdpPassRun` walks that list, so the hit pass never visits them. **Correction to 13.4's table and 13.7's last note:**
`csObjectPool[98]` there (`viewCameraObjects[0]`) is not walked by the hit pass; its model-0 walk from address 0 does
not happen. So this is not request 51's kind: the step does not read drawing state; the hash holds a word only the
drawing defines.

**Why only a machine that re-ran steps showed it (and why every machine will in general).** On a machine that draws
every iteration, the matrix at a step's end is the previous iteration's (the rot of step N-1); a re-run batch draws
nothing, so at a confirmed step the matrix is the one drawn before the batch (or the snapshot's). In general a camera
object is per machine anyway: the cameras follow the **local** humans (`HUMANS16_PLAN.md` 2.3 and 2.5: views,
cameras and hotkeys by local slot, presentation by design; the existing rows `viewCameraObjects`, `viewEyePositions`,
`viewMatrices`, `cameraPrevPos` are "yes / no" for that reason), and `viewProc` in mode 4 (the rear-view mirror)
copies view 0's camera rot / pos into view 1's camera and its parent. The machines in the test happened to run the
same cameras.

**The rows (a new kind, `ptr`).** The objects are pool elements whose index depends on the load, so a `field` row
(every element) cannot name them, and their other fields (+0x80, +0xA0, +0xA8 of every other object) are state.
`ptr` rows name them through their pointer arrays:

```
viewCameraObjects.obj      ptr  viewCameraObjects:next:0x128      yes  no   the views' camera objects
viewCameraObjectTable.obj  ptr  viewCameraObjectTable:next:0x128  yes  no   their pose objects (the cameras' parents)
viewSharedRoot.obj         ptr  viewSharedRoot:1:0x128            yes  no   the cameras' shared root object
viewFollowObjects.obj      ptr  viewFollowObjects:next:0x128      yes  no   the camera-attached models (the sky),
                                                                            placed at a local view's camera
```

`tools/rollback_regions.py` resolves each into one row per pointer word (`viewCameraObjects.obj[i]:0x128`, address =
the word, size 0: it covers no static byte); at run time a non-zero word names 0x128 bytes at the address it holds,
which are hash no (and stay snapshot yes, as the camera rows: the human records' camera blocks are restored with them).
The whole object, not +0x80..+0x9F alone: rot, pos and the matrices of these objects are all the cameras'. The follow
objects join because `viewSetFollowObjPos` places them at the local view's camera (13.4's `csObjectPool[11]` +0xA8);
their model's nodes stay in the `dmd` row. Element 107 in the dumps is `viewCameraObjects.obj[L]`.

**No step-side fix** (a bit making the step compute the cameras' matrices from rot, so the word would follow rot) was
built: it would make a re-run reproduce this word, but the rot itself is per machine (local cameras, mode 4), so the
objects stay out of the hash either way, and nothing in the game reads the matrix outside `viewProc`, which recomputes
it before use.

### 17.4 The scene objects' other fields, +0x80..+0x127: writers and readers (the audit)

| field | the step writes | the drawing writes | read by | row |
|---|---|---|---|---|
| +0x80..+0x93 the rotation matrix (`rotm`; +0x94..+0x9F its translation, zero from `csReset`) | each kind's update from rot: `RotMatrixYXZ` in `debrisUpdateChunks`, the car body / parts (`car_body_pose.c`, `car_parts_spin.c`), the projectiles (`weapon_fire_missiles.c`, `weapon_ricochet.c`), the tornado, the strike, Dark Tooth's boss, the target actors, the track vehicles, `scene_init.c`'s objects, the pose objects (`viewSetCameraPose`); `csReset` at `csAlloc` | **`viewProc` only: the camera objects and their parents** (17.3) | the hit pass (`hdpPassRun`: every listed object with +0x08 and +0x0C set), `csUpdMat` (the drawing: wpos and the per-view matrices) | state (hash yes); the cameras: `ptr` rows |
| +0xA0..+0xA7 rot | the same updates; the cameras' (`viewSetRotation`, `viewSetCameraPose`) | `viewProc` in mode 4 (view 1's camera and its parent, from view 0's) | the matrix writers above, `viewProc` | state; the cameras: `ptr` |
| +0xA8..+0xB3 pos | the updates; the cameras' (`viewSetPosition`, `viewSetCameraPose`); `viewSetFollowObjPos` (the follow objects, a local view) | `viewProc` in mode 4 (as rot) | the hit pass (the probe's offset), `csUpdMat`, the logic | state; the cameras and follow objects: `ptr` |
| +0xB4..+0x117 the kind's data (the car box `box_count`, `box[6][4]`, ...) | `carBuildCollisionBox` and the kinds' updates | none | the step | state |
| +0x118 id, +0x11C model, +0x120 next | the spawns, `csSetModel`, `csListAdd` (loads and steps) | none | the hit pass, the renderer, `portRollbackAnimStep` | state |
| +0x124 on-screen flag | `debrisStartBurst`'s capture (`renderDrawObjectUnculled` -> `csUpdMat`) | `csUpdMat` per drawn view | `csUpdMat`, `renderScene` | `field` row `csObjectPool.onscreen` (hash no, request 54) |

Evidence that the drawing writes no other object word in +0x80..+0x123: (a) the static census of the drawing set
(check 9's roots and the new ones): its direct stores at offsets 0x80..0x123 off a non-stack base are `viewProc`'s
+0xA0 / +0xA4 (mode 4), the packet emitters (`renderNearPolys`, `sub_800CDC54`: packets), `renderScene`'s +0x88 (its
render context in the scratchpad) and `fxDebrisDraw`'s scratchpad word; its calls of the matrix and copy writers are
`RotMatrixYXZ` / `viewCopyWords` (`viewProc` only), `MulRotMatrix0` / `TransposeMatrix` (`csUpdMat` into +0x20 / +0x40
/ +0x60 and locals, `fxDebrisDraw` into locals, `viewProc` into the root's per-view fields) and `memcpy`
(`PutDispEnv`); (b) the RAM differences of request 51's dumps (13.4 (a)) and this request's: the only differing words
of +0x80..+0x123 were a camera object's (`[98]`, `[107]`) and a follow object's +0xA8 (`[11]`). **No new `field` row
is needed.**

### 17.5 Checks

No source file changed: the retail images (hybrid 9219f7ba..., source-only d755c86e..., sdk-lib f557c1e3...) and every
port variant are unchanged; `PACKAGE.json` is refreshed (`tools/make_package.py`, `build.py --verify-package`).

- `tools/check_rollback.py` on `work/pv/r54_rb_P4`, `r54_port_P4` and `r54_allh16_P4` (`--expect-functions --e0` of
  `work/build-20261022-161107-135568`): every check passes. Check 8: 586 / 972 / 972 rows resolve (the `ptr` rows
  included), 0 step stores outside a row, 0 static symbols outside a row. Check 9: the drawing (120 / 156 / 156
  functions from the 8 roots and 6 / 8 other calls of `rtMainLoop`, its and `portRollbackLoopStep`'s own 8 stores)
  stores to 0 hashed static words; 0 of the 41 / 42 `DRAW_STATE` words hashed; 4 of 4 camera pointer arrays out.
- Negative controls (`r54_rb_P4`, `--regions`): **request 56's list** (`git show HEAD:config/US/rollback_regions.tsv`)
  fails check 9: "the drawing stores to hashed `D_80180EE4`, `renderBufferRearView`, `renderBufferView1`, `rtFrameTime`
  (`rtMainLoop`)", 7 of the request's words hashed (`padRecvBufferPort1` / `2`, `D_801A5AFC`, `D_80180EE4`,
  `renderBufferView1`, `renderBufferRearView`, `D_80193950`; 8 on `r54_port_P4` with `portSpuExt`), "0 of 4 camera
  pointer arrays out". **The new list less `rtFrameTime`'s row** fails on `rtMainLoop`'s own store alone (the store
  census of the iteration is not blind); **less `viewCameraObjects.obj`** fails "viewCameraObjects: no ptr row with
  hash no".

### 17.6 What the port does

1. Regenerate `rollback_regions.tsv` (its `tools/decomp/wc.py` runs `tools/rollback_regions.py`). The new symbol rows
   replace the port's masks of (1) and (2) (`rbs_hash`'s `mask_lo` / `mask_hi`: the same words; `D_80193950`'s row is
   its 0x100 bytes).
2. **Read the `ptr` rows** (kind `ptr`, size 0, name `NAME[i]:BYTES`): at each hash, for each row read the word at its
   address; when it is non-zero, leave `[word, word + BYTES)` out of the hash. Their snapshot is "yes", which the
   `game.static` / `port.bss` ranges already copy (`csObjectPool` lies in one). `tools/recomp/recompile.py`'s
   `rb_spans` skips them today (size 0), so until it reads them the camera objects stay hashed (no wrong hole).
3. Keep the hash's other rows as they are: the scene objects' +0x80..+0x123 are game state for every other object.

### 17.7 Open notes

- **For the user:** a step-side bit that computes the cameras' matrices (`RotMatrixYXZ` of rot for each view's camera
  and parent at the step's end) was not built (17.3): with the `ptr` rows it changes no hash, and the cameras are per
  machine by design. Ask if the port wants the cameras hashed (it would need the cameras of every human run on every
  machine, which `HUMANS16_PLAN.md` 2.5 (2) leaves out).
- `rtRearViewActive` (the mirror on), `rtRearViewAllowed` and `viewSplitScreenMode` are set from local humans' hotkeys
  (`rtToggleRearView`, `carViewHotkeys`; the port's `hud_mirror` setting per machine) and stay in hashed rows; the step
  reads them in `carSetViewMode` / `carViewHotkeys` (local humans only) and, for `viewSplitScreenMode` through
  `viewGetSplitMode`, in about 50 game functions (effects, sounds, camera, damage: split-screen variants). Machines with
  different mirror or split settings part on these words; whether the split-screen reads change game state was not
  audited here (a separate request if the port lets machines differ there). Audited in 18: the rows are "hash no" now,
  and the one game-state read (without `TM2_PORT_HUMANS`) is behind bit 10.
- Check 9 sees direct static stores and the named words; a word the drawing or the loop writes through a pointer and
  that is hashed still escapes it (as `D_80193950`, found by the port). The port's cross-machine hash diff remains the
  final proof; new words it finds go into the tsv and `DRAW_STATE`. Request 58 added the census of stores through
  pointers (19.4).

---------------------------------------------------------------------------------------------------------

## 18. This machine's view settings in the step: the split screen and the rear-view mirror (2026-10-22)

Request 57's open note (17.7): online each machine has its own rear-view mirror and possibly its own split screen.
This section audits every read of those settings that the step reaches and classifies it: **(a)** drawing, sound, HUD
or camera only (each machine's own, fine) or **(b)** game state (anything in the snapshot or the hash: the records,
the objects, `rand()`'s count and order, weapons, damage, the AI, timers, events, effect rates). **Result: in the
port's build every read is (a); the one (b) is `humanCarTick`'s weapon regeneration in builds without
`TM2_PORT_HUMANS`, now behind bit 10 `LOCAL_VIEWS`. What the (a) reads write lay in hashed rows: 11 rows go to "hash
no"** (`config/US/rollback_regions.tsv`), and `tools/check_rollback.py` check 10 proves the audit on every rollback
link. Behind `TM2_PORT_ROLLBACK` as before: the retail images are unchanged (hybrid 9219f7ba..., source-only
d755c86e..., sdk-lib f557c1e3...).

Addresses: retail, and the port's build (every define of its `tools/decomp/wc.py` plus `TM2_PORT_HOOKS` and
`TM2_PORT_FAR_LOD`, without its overlay units: `work/pv/vw_port_P4`; "(rb)": `TM2_PORT_ROLLBACK` alone,
`work/pv/vw_rb_P4`); other builds: the build's `symbols.tsv`. The reach is check 8's (the `jal` graph from
`portStepBody`).

### 18.1 The settings, and who sets them on each machine

| word (retail) | written by | per machine online |
|---|---|---|
| `viewSplitScreenMode` 801809E4 (`viewGetSplitMode` 800D1C18) | `sessionRun` (`viewSetSplitMode`: game modes 2 and 4, 1 on levels 2, 3 and 8, else 2); `carViewHotkeys`' layout toggle (a local human's view key in split screen) | **yes**: the port's `vp_ViewSetSplitMode` (`src/vplus/fixes.c`) gives one view to a machine with one local player and the split to one with two (its Options > Video setting), `four.c`'s mode 2 with 3-4; the hotkey toggle is kept as the setting |
| `rtRearViewActive` 801809DC (`rtGetRearViewActive` 800D1C00) | `rtToggleRearView` (a local human's mirror key, `carSetViewMode`'s view 4), `rtRearViewOff` (`arenaSetup`) | **yes**: a local hotkey; the port's `hud_mirror` (`src/settings.c`: the mirror at a battle's start; split screen's mirrors are the port's own drawing, `vplus/mirrors.c`) |
| `rtRearViewAllowed` 801809E0 | `rtSetRearView` (`arenaSetup`, `carSetViewMode`, `carViewHotkeys`: a local human's view mode) | **yes** (a local human's view) |

Found on the way (the same kind): `viewRenderSplitMode` (`viewSetFollowObj` at the load: the split mode then; read by
`renderScene` only), `viewFov`, `viewModes`, `viewEnvs` (the views the split mode and a local human's view mode
create), `viewExtraViewsUnlocked`, `hudRadarVisible`, `hudWeaponListVisible` and `carViewHotkeyReleased` (written by a
local human's hotkeys only: `carViewHotkeys` returns at once for a remote human, H1). The human records' camera block
(`view` +0x10C included) is the port's mask already (HUMANS16 6.4).

`PadChkMtap` (800D3590) is a misnomer: it returns `sndListenerDist`, the last `sndListenerTransform`'s distance (the
sound's volume), not the multitap; "`PAD_REINIT`" (`fx_sprite_spawn.c`) is the explosion sound's cooldown. Both are
the sound locate below.

### 18.2 The reads the step reaches (the audit)

**The pattern of 40 of the 50 functions** (the sound locate, 38 rows below plus `weaponTick` and the port's `portHumanLocate`): `if (viewGetSplitMode() == 0)
soundLocateForView0(pos); else soundLocateForView(pos, -1);` then `soundPlay(id, PadChkMtap(), soundGetListenerPan(),
pitch)` (in split screen the pan is often 0). `sndListenerTransform` (8015DBE8) writes only the sound listener's words
(hash no, snapshot no since request 54) from the view's camera (each machine's own, split or not); the volume and pan
go to the sound wrappers (SOUND_GATE's 10) or the port's car sound functions, whose results no caller uses (15.2);
where the volume decides whether to play (`fxSplashAt`, `fxLavaSplashAt`: `abs(vol) < 1500`), only the play depends on
it. None of the branches holds a `rand()`, a state store or another call: the code before and after the branch runs on
every machine alike (the `rand() % 3` of `carDestroyed` / `moscowEventsUpdate` comes before it).

The 50 functions of the port's build (two more in builds without `TM2_PORT_HUMANS`, marked (rb)); "reads" counts the calls of `viewGetSplitMode` (split) and `rtGetRearViewActive` (mirror) and the direct loads and stores of the words:

| function | retail | port build | source | reads | what the read selects | class | fix |
|---|---|---|---|---|---|---|---|
| `rtSetRearView` | 800D1B94 | 800D1D48 | rt_main.c | rtRearViewAllowed store | writes `rtRearViewAllowed` (a local human's view mode) | a | row |
| `rtToggleRearView` | 800D1BB8 | 800D1D6C | rt_main.c | rtRearViewActive load, rtRearViewActive store, rtRearViewAllowed load | flips `rtRearViewActive` when allowed; `renderBufferRearView` (hash no) | a | row |
| `viewSetSplitMode` | 800D1C24 | 800D1DD8 | rt_main.c | viewSplitScreenMode store | writes `viewSplitScreenMode` (the split toggle) | a | row |
| `moscowEventsUpdate` | 800D4D58 | 800D50F0 | random_event_level2.c | split x2 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `laEventsUpdate` | 800D5AF0 | 800D5E98 | car_destroyed.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponProjectileUpdate` | 800DB110 | 800DB784 | projectile_hit.c | split x3 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxSheetStart` | 800DC45C | 800DCB58 | weapon_fire_specials.c | split x1 | the screen flash time, doubled in split screen (`fxSheetTimer`, hash no: presentation; bit 2 drops the doubling) | a | none (stage 2 gate stays) |
| `carCarCollision` | 800E184C | 800E20E0 | car_collision.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `carContactsCombine` | 800E2DD0 | 800E369C | car_contacts.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `carCrashStart` | 800E6D14 | 800E7714 | car_crash.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `carSetViewMode` | 800E7380 | 800E7DE8 | weapon_missile_trails.c | mirror x1 | the mirror: a local human's view 4 turns it off and disallows it (`rtToggleRearView`, `rtSetRearView`); the record's camera block | a | rows |
| `carParisSinkZonesUpdate` | 800F0450 | 800F1038 | car_wheel_sway.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `carSuspensionUpdate` | 800F15D0 | 800F21DC | screen_pause_menu.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponSlamSwingUpdate` | 800F4058 | 800F4C88 | weapon_slam_swing.c | split x2 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponSlamGrabUpdate` | 800F4CC0 | 800F5964 | weapon_arcs_draw.c | split x2 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `hudInitPanels` | 800FB0B0 | 800FBF28 | hud_init.c | split x3 | the HUD panels' layout per split mode (from the split toggle) | a | none (HUD rows) |
| `hudRadarPlotCar` | 800FC928 | 800FD7A8 | hud_radar.c | split x2 | a blip's offset on the radar (`hudRadars`, hash no) | a | none |
| `viewChaseCamera` | 80102CC4 | 80103C8C | camera.c | split x2 | the chase camera's offsets and pitch per split layout (the human record's camera block, out of the port's hash: HUMANS16 6.4) | a | none |
| `weaponTick` | 8010388C | 80104854 | weapon_tick.c | split x2 | the gun loop's listener and pan (`soundStartSharedLoop`, `portHumanLoopPlay`) | a | none |
| `weaponFireSelected` | 80103FB8 | 80105028 | weapon_select.c | split x3 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponFireEnergyAttack` | 801046FC | 80105800 | weapon_special.c | split x3 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponLaunchProjectile` | 801052BC | 80106554 | weapon_launch.c | split x6 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponFireSpecial` | 80105A7C | 80106D2C | weapon_basic.c | split x12 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponDropItem` | 80109518 | 8010AD80 | weapon_helpers.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponFlameStart` | 8010A5F0 | 8010C020 | car_restart.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `pickupApply` | 8010AFC8 | 8010CA44 | pickup_apply.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponRicochetUpdate` | 8010CC54 | 8010E740 | weapon_ricochet.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `weaponInvisibilityUpdate` | 8010DE54 | 8010FC3C | ai_link_node_pick.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `fxExplosion12` | 80111538 | 80111EB0 | fx_explosions.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxExplosion24` | 8011160C | 80111F84 | fx_explosions.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxExplosion48` | 801116E0 | 80112058 | fx_explosions.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxExplosion96` | 801117B4 | 8011212C | fx_explosions.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxExplosion192` | 80111888 | 80112200 | fx_explosions.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxSmokeBurstPink` | 80111B20 | 80112498 | fx_sprite_spawn.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxImpactDebris` | 80112C90 | 80113608 | fx_debris_splash.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `worldWarpStart` | 8011566C | 801162D4 | fx_debris_splash.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `worldCollapseUpdate` | 801163B8 | 80117020 | world_collapse.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `fxSplashAt` | 80119174 | 80119DF8 | fx_debris_splash.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `fxLavaSplashAt` | 8011930C | 80119F90 | car_parts_spin.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `carDestroyed` | 8011BE10 | 8011CF40 | car_destroyed.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`), and the volume / pan of the play (pan 0 in split) | a | none |
| `carViewHotkeys` | 8011C530 | 8011D724 | car_records.c | split x2, mirror x1 | split screen: the view key toggles the layout 1 <-> 2 (`viewSetSplitMode`, `viewCreateWithCamera` of views 0 and 1, `hudInitPanels`), the radar / weapon list keys, the latch; one view: the camera view, the mirror key, the extra views combo (`viewExtraViewsUnlocked`); local humans only (H1) | a | rows |
| `carSoundUpdate` | 8011DB74 | 8011EDBC | screen_card_draw.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `targetDamage` | 80125118 | 80126590 | target_executor.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `targetActorsUpdate` | 80126464 | 801278EC | target_actor_update.c | split x2 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `targetActorHit` | 80127228 | 801286C8 | target_actor_events.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `targetTimedEventUpdate` | 801274C0 | 80128960 | target_actor_events.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `targetLevelEventUpdate` | 801277A8 | 80128C48 | target_secondary_events.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `targetPlayerEventsUpdate` | 80127CE0 | 80129180 | target_slot_update.c | split x1 | the sound's listener: view 0, or the nearer view (`soundLocateForView(pos, -1)`) | a | none |
| `portHumanLocate` | - | 808BC7FC | port_variant_arrays.c | split x1 | a remote human's car sound: the listener's view(s) and the pan (0 in split) | a | none |
| `worldHumanCamera` | - | 808C47E8 | world_update.c | split x1 | whether a local human's camera runs (`viewMainPlayer == slot` or split) | a | none |
| `worldUpdate` | 8011A700 | 8011AC90 (rb) | world_update.c | split x1 | (without `TM2_PORT_HUMANS`) whether human i's camera runs (`viewMainPlayer == i` or split) | a | none |
| `humanCarTick` | 800DC88C | 800DCC70 (rb) | car_tick.c | split x1 (portRollbackViewSplit) | (without `TM2_PORT_HUMANS`) **the weapon regeneration**: a human regains a weapon only below its capacity, except in split screen (`REGEN_SPLIT`): ammo (+0x7D6), the pickup message (`hudMessagePost`) | **b** | **bit 10 `LOCAL_VIEWS`: `portRollbackViewSplit()`** (with `TM2_PORT_HUMANS` H1 already tests the game mode) |

Reads of the settings outside the step (the load, a round's start, the drawing, the menus): all (a); the two that
could touch state were checked:

| function | retail | reads | where (not reached from `portStepBody`) |
|---|---|---|---|
| `rtMainLoop` | 800D1318 | rtRearViewActive load, viewSplitScreenMode load | the iteration's drawing of view 1 / the mirror and the HUD drawers (check 9) |
| `rtRearViewOff` | 800D1C0C | rtRearViewActive store | arenaSetup (the round's start: the mirror off) |
| `worldLoadObject` | 800D1DB0 | split x1 | the load: `viewSetFollowObj(sky, split)` (`viewRenderSplitMode`, a row; the second follow object is allocated in every mode) |
| `hudDrawSingleView` | 800F8370 | mirror x1 | the HUD drawer: the mirror's frame |
| `UAdashInitDashboardIcons` | 800FAA0C | split x1 | the HUD's icons at the load |
| `hudDisplayRadar` | 800FBE68 | split x1 | the HUD drawer |
| `hudRadarInit` | 800FC5C4 | split x3 | arenaSetup: the radar layout |
| `hudDrawTargetPanel` | 800FD0D0 | split x2 | the HUD drawer |
| `fxExplosion6` | 80111464 | split x1 | no caller in the step (sound locate, as fxExplosion12) |
| `fxSmokeBurstRed` | 80111C24 | split x1 | no caller in the step (sound locate, as fxSmokeBurstPink) |
| `arenaSetup` | 8011995C | split x2 | the round's start: views 0 and 1 in every mode (one view: view 1 the mirror, mode 4), so the same allocations (check 10 (c)) |
| `humanCarsRestart` | 8011A198 | split x1 | the round's start: `carSetViewMode` of each human in one view only (the camera, the rows) |
| `pauseMenu` | 80129528 | split x1 | the pause menu (meta; `PAUSE_PORT` online) |
| `screenDeathmatchResult` | 8012B2D4 | split x1 | the results screen |

### 18.3 The one game-state reader, and the fix: bit 10 `LOCAL_VIEWS` (0x400)

`humanCarTick` (retail 800DC88C, `car_tick.c`), the weapon pickup's regeneration: a human regains a weapon only while
below its capacity, **except in split screen** (`REGEN_SPLIT()`, retail `viewGetSplitMode() != 0`): the ammo, the held
count and the pickup message differ between a machine with two local players and one with one. With `TM2_PORT_HUMANS`
(the port's build) H1 already tests the game mode (2 or 4: where retail's sessionRun splits), so the port's build reads
no split there. Without it (`TM2_PORT_ROLLBACK` alone, and any build without the humans define), under the rollback
define:

- `REGEN_SPLIT()` = `portRollbackViewSplit() != 0`.
- `portRollbackViewSplit()` (`.port_text`, `port_variant_arrays.c`; vw_rb 80851108, vw_port 808C203C): without the
  bit `viewGetSplitMode()` (retail); with it the match's split: retail's choice for the game mode and level, as
  `sessionRun` makes it (modes 2 and 4: 1 on levels 2, 3 and 8, else 2; other modes 0), the same on every machine (the
  game mode and level are match state). No new word: nothing for the port to set but the bit. Calls no `rand()`.

`TM2_RB_ONLINE` = 0x7FF is stage 3's bits plus this one. In the port's build the bit changes nothing (no site): set it
anyway (the options word is the same on every machine, and other builds need it).

### 18.4 The rows (`config/US/rollback_regions.tsv`): 11 settings, snapshot yes, hash no

`viewSplitScreenMode`, `rtRearViewActive`, `rtRearViewAllowed`, `viewRenderSplitMode`, `viewFov`, `viewModes`,
`viewEnvs`, `viewExtraViewsUnlocked`, `hudRadarVisible`, `hudWeaponListVisible`, `carViewHotkeyReleased` (in
`.port_data`; the `port.data` row's reason no longer names it). Each is written in the step by the view functions of
18.2 only (or at the load) and read by the drawing, the HUD, the hotkeys or the view functions only (check 10).

**Snapshot yes, deliberately:** the hotkey latch (`carViewHotkeyReleased`) and what a toggle writes must come back
together. A re-run replays the local human's inputs, so a toggle in a re-run step happens again; with the setting
restored to the snapshot's value the replay leaves it where the live step did (snapshot no would toggle twice). The
frame records the toggle rewrites (`viewCreate`'s draw environments in `renderView0Frames` ...) are "no" rows: the
re-run's toggle rewrites them before the next drawing.

### 18.5 Checks

`tools/check_rollback.py` check 10 (run by `check_all.py` on every link with the define):

- **(a) taint.** The users are the step's functions that call `viewGetSplitMode`, `rtGetRearViewActive`,
  `viewGetRenderSplitMode`, `PadChkMtap` or `soundGetListenerPan`, or load `viewSplitScreenMode`, `rtRearViewActive`,
  `rtRearViewAllowed`, `viewRenderSplitMode`, `sndListenerDist` or `sndListenerPan` (the listener too: each machine's
  camera), less the view and sound functions of (b). In each, a forward taint over the flow graph (registers, stack
  slots, and control dependence: the nodes a dependent branch or jump table reaches before its immediate
  post-dominator) to a fixed point; a finding is a dependent static store to a "hash yes" row, a dependent store
  through a pointer, a call (or tail call, or call through a pointer) in a dependent region or with a dependent
  argument other than the sound side (`SOUND_SIDE`: the 10 wrappers, the locate, `sndListenerTransform`, the port's
  car sound functions) and the view functions (`VIEW_PRESENT`), and a dependent return value (the function becomes a
  source for its callers: 3 functions do, harmlessly: `fxSplashAt` and `fxLavaSplashAt` leave `soundPlay`'s value in
  `$v0`). `portHumanLocate`'s pointer arguments taint the caller's stack slots.
- **(b) the view and sound side.** The functions of `VIEW_PRESENT` and `SOUND_SIDE` the step reaches, and all they
  reach, call no `rand()` and store static words only in "hash no" rows, less the views' lazy allocations
  (`viewCreate` / `viewCreateWithCamera` -> `csAlloc` and `viewCreate`'s `viewSharedRoot`: only while null).
- **(c)** `arenaSetup` calls `viewCreateWithCamera(.., 0)` and `(.., 1)` on every path to its return (so (b)'s lazy
  allocations never run in a battle: every split mode has views 0 and 1).
- **(d) the gate.** `portRollbackViewSplit` is in `.port_text`, reads `portRollbackOptions`, calls exactly
  `viewGetSplitMode`, `gameGetMode`, `gameGetLevel`; its only caller is `humanCarTick`; without `TM2_PORT_HUMANS`
  `humanCarTick` calls it and no `viewGetSplitMode`, with it neither. Without `TM2_PORT_HUMANS` the check runs again
  with the bit clear (`portRollbackViewSplit` as a source): the findings must be exactly `humanCarTick` (the check
  sees the site it gates).
- **(e)** the 11 settings of 18.4 lie in "hash no" rows.

Also: check 1 adds `portRollbackViewSplit` to the port functions (no `rand()`); checks 2 and 7 count `humanCarTick` as
changed by the define in builds without `TM2_PORT_HUMANS` (37 functions with the define alone).

**Runs** (`--expect-functions --e0` of `work/build-20261022-204108-085848`): every check passes on `work/pv/vw_rb_P4`,
`vw_port_P4` and `vw_allh16_P4`. Check 10: 41 / 40 / 40 users (243 / 242 / 242 call sites of the sources; 3 more
functions through dependent returns), 678 / 681 / 681 dependent nodes, 0 findings; the view and sound side (27 / 33 /
33 roots, 95 / 117 / 117 functions) stores to 0 hashed words (1 lazy store exempt); 11 of 11 settings out of the hash;
on `vw_rb_P4` the bit-clear model finds exactly `humanCarTick` (its ammo store +0x7D6, the held count +0xAF and two
`hudMessagePost` calls in the dependent region). Check 7: 37 functions with the define alone. `tools/check_all.py` on
the full build and the six links: 27 checks, 0 failed (retail hybrid `9219f7ba...`, source-only `d755c86e...`, sdk-lib
`f557c1e3...` unchanged; proofs, placement with the limits forced to 2, humans, car groups, hazards, balance, custom,
rollback). Images: vw_rb P4 `afbd754e...` (P2 `d83b6019...`, port end 0x808526C0), vw_port P4 `456de05b...` (P2
`0a4a1399...`, port end 0x808C52C0), vw_allh16 P4 `ee47e55e...` (P2 `6abeb0ec...`, port end 0x808C6A10); the full
build's variant without port defines `bb016699...` (P2 `de7ded35...`).

**Negative controls.** Request 57's list (`git show HEAD:config/US/rollback_regions.tsv`, `--regions`) fails check 10
on `vw_rb_P4` and `vw_port_P4`: the view side stores to 7 hashed words (`hudRadarVisible`, `hudWeaponListVisible`,
`rtRearViewActive`, `rtRearViewAllowed`, `viewExtraViewsUnlocked`, `viewFov`, `viewSplitScreenMode`) and the 11
settings are hashed (checks 8 and 9 pass with it). Request 57's link without the gate (`work/pv/r54_rb_P4`: the same
source before this change, `TM2_PORT_ROLLBACK` alone) fails check 10 (a) on `humanCarTick` (the same four findings)
and (d) (no `portRollbackViewSplit`, one `viewGetSplitMode` call).

### 18.6 What the port does

1. Set `portRollbackOptions` bit 10 (`TM2_RB_LOCAL_VIEWS`, 0x400; every bit: 0x7FF) on every machine of an online
   match. Nothing else to set.
2. Regenerate `rollback_regions.tsv` (its `tools/decomp/wc.py`): the 11 rows of 18.4 leave the hash. Each machine may
   then keep its own mirror (`hud_mirror`, the hotkey) and its own split screen (one view or two, by its local
   players), and toggle them in a match, without the hashes parting.
3. Keep the human records' camera block out of the hash (as today, HUMANS16 6.4).

### 18.7 Open notes

- **For the port: views 3-4 allocate scene objects on one machine only.** `four.c` (`four_views_create`,
  `vp_ViewSetFollowObj`) calls `viewCreateWithCamera` for views 2-3 and `csAlloc` for their follow objects when the
  machine has 3-4 local players (unless `port_config_on()`, where the port's C does it). On another machine those
  objects do not exist: `csPoolCount` (hashed) differs, and every object allocated after them sits at another index of
  `csObjectPool` (the hash and every stored object pointer differ, and a full pool fails on one machine only). No
  object is allocated in a battle's step (check 10: the step reaches `csAlloc` only through the lazy view paths), so
  the difference comes from the load. Suggestion: online, allocate views 2-3's objects on every machine (or after
  every other load allocation, and leave `csPoolCount` and those slots out of the hash). Not changed here (the port's
  code); not measured.
- The taint is static and path-insensitive: it follows `jal` calls, register and stack data and control dependence; it
  does not follow values stored through pointers by the sound side or the view functions (their static stores are
  checked; their pointer stores are the sound objects, the HUD's primitives, the human records' camera block and the
  hotkey latch, by reading). The cross-machine hash diff stays the final proof.
- A sweep with the camera's own getters as sources (`viewGetEyeTrans`, `viewGetViewMatrix`, `viewGetMode`,
  `viewGetFov`, the projection scales) finds them in the step only in the cameras, the view setters, the sound
  listener and `weaponArcsDraw` (whose drawing pass reads them; its tick pass, the step's call with bit 2, does not:
  stage 2, 11.2). Not part of check 10 (the pass is a run-time value).
- Without bit 2 (`ONCE_PER_STEP`) `fxSheetStart` still doubles the flash in split screen: presentation
  (`fxSheetTimer` hash no) but each machine's flash then lasts its own time; bit 2 is set online.

---------------------------------------------------------------------------------------------------------

## 19. Request 58 (2026-10-23): the words request 57's list did not cover

The port's cross-machine diffs (request 57 (2) and (4)) still found hashed words that are each machine's own:
`hudRadarArrowUpPrims` .. `RightPrims`, `hudMessageTextMode` / `Sprites`, `hudLivesTextMode` / `Sprites`,
`portHumanLocal`, and after a drop-in `csObjectPool[40]` +0xA6..+0xA7 and the 256 bytes from `dc_y`. **None of them
is game state the step reads; the one machine-local input to the step (`portHumanLocal`) feeds presentation only.
No source file changes: 30 rows join `config/US/rollback_regions.tsv`, and `tools/check_rollback.py` gets the census
check 9 lacked (stores through pointers) and a check of `portHumanLocal`'s readers (check 11).** The retail images
(hybrid 9219f7ba..., source-only d755c86e..., sdk-lib f557c1e3...) and every port variant are unchanged.

Addresses: retail, and the port's build (`build/decomp-wc/out/TM2_SOURCE_PORT/symbols.tsv`, whose `csObjectPool` is
805FCA00 as in the port's RAM dumps); the static runs use `work/pv/vw_port_P4` (the same source and defines).

### 19.1 The HUD's words and `portHumanLocal` (CONFIRMED: presentation)

| words | written by | in the step? | row |
|---|---|---|---|
| `hudRadarArrowUpPrims` .. `RightPrims` (retail 80182308.., 0x40 each: 32 bytes per HUD; `.port_bss` in the variants) | `hudDisplayRadar` (800FBE68): `RADAR_ARROW` fills the off-radar arrow's sprite and draw mode for the HUD it draws and links both into this frame's OT (`RADAR_ADD`: the tag word holds the OT of the frame's parity: the request's pointer at +0x20) | **no.** `hudDisplayRadar` is called by the three HUD drawers only. STEP_SPLIT's radar swap (12, stage 3) moved its call of `hudRadarSwapBuffers` into the step (`portRollbackHudStep`); the drawing keeps the rest (`DRAW_PRUNE` cuts only that edge) | no, no |
| `hudMessageTextMode` / `hudMessageTextSprites` (80181E28 / 80181E34: 30 glyphs) and `hudLivesTextMode` / `hudLivesTextSprites` (8018208C / 80182098) | `fontSpritePrintCenteredXY` from `hudDrawSingleView` (the message of `hudMessageCurrent`, the lives line), per drawn HUD | **no.** `hudMessagePost` (from the step: `carApplyDamage`, `portHumanMessage`) writes only `hudMessageCurrent` (yes / no since request 54); the step prints nothing into these | no, no |
| `portHumanLocal` (`.port_bss`, 16 bytes) | the port (per machine, H1); `portHumanCompact` / `portHumanVacateRecord` with the records, between steps | read by the step only through `portHumanSlot`. The 28 step functions that use the map (the table below) are presentation, or reach game code only through `portHumanPadPort`, which returns -1 before it reads the map when INPUT_ALL (portHumanOptions bit 0) is set | **yes**, no |

**`portHumanLocal`'s readers in the step** (`vw_port_P4`, check 11):

| reader | what the map decides | class |
|---|---|---|
| `portHumanSlot`, `portLocalHuman`, `portLocalHumanCount`, `portHumanSide`, `portHumanFeedback` | the map itself (pure; HUMANS16 2.2) | source |
| `carSetViewMode`, `carViewHotkeys`, `worldHumanCamera`, `worldHumanFollow` | the local human's views, camera, follow object, hotkeys (writes: the camera block of the record, the view objects, the settings of 18.4) | presentation |
| `portHumanRadarPlot`, `worldRadarAiAll` | the blips on the local slot's radar (`hudRadars`), the AI blink counter `portHumanRadarBlink` | presentation |
| `portHumanMessage`, `portHumanSound`, `portHumanDenied`, `portHumanLoopPlay`, `portHumanLoopStop` | a local human's own feedback (the HUD message, the full-volume sounds, the denied buzz `soundDeniedPending`, the side loops) | presentation / sound |
| `portCarSoundFrame`, `sndListenerTransform` | the engine cap's ranking, the listener's views | sound |
| `carCrashStart`, `carParisSinkZonesUpdate`, `weaponFireEnergyAttack`, `weaponFireSpecial`, `weaponFlameStart`, `weaponSlamGrabUpdate`, `weaponSlamSwingUpdate` | `TM2_CAR_FEEDBACK`: a full-volume sound for a local human, a positional one otherwise | sound: the taint finds no dependent state write, call or return |
| `portHumanPadPort` -> `portHumanAction` / `Combo` / `AnyLatched` | the pad a local slot 0 / 1 reads **only with INPUT_ALL clear**; with the bit set it returns -1 before `portHumanSlot` (`lw portHumanOptions; andi 1; beqz` -> `j` out with -1) | input: gated (online the port sets INPUT_ALL, HUMANS16 2.4) |

So no step code branches on the map in a way that changes shared state, as long as INPUT_ALL is set. With it clear
(offline, or a misconfigured online match), a human on local slot 0 / 1 reads this machine's pad (retail): that is the
input path, not a desync of the step. Snapshot yes for `portHumanLocal`: it moves with the records it maps
(`portHumanCompact`), which are restored with a state.

Found on the way, the same kind (check 10 (b) and 11 with the pointer census, 19.4): `soundDeniedPending` (the denied
buzz: set through `portHumanDenied` for a local human, played and cleared by `worldUpdate`: no / no),
`portHumanRadarBlink` (yes / no), the human records' camera block (`humanCars.camease` +0x006..+0x007,
`humanCars.camera` +0x00C..+0x08B, `humanCars.view` +0x10C..+0x113: field rows, yes / no; the bytes HUMANS16 2.5.2
reviewed, which the port already masks) and `viewCameraObjectTable` (the pose objects' pointers, lazily allocated
with a view: yes / no, as `viewCameraObjects`).

**Why check 9 did not catch them.** Check 9 (a) counts store instructions that carry a LO16 / GPREL16 relocation.
These words are written (1) through an address formed earlier (`&hudRadarArrowUpPrims[idx]`: `addiu` with the
relocation, `addu` of the index, then `sb` / `sh` / `sw` off the register), and (2) by callees through their arguments
(`fontSpritePrintCenteredXY(.., &hudMessageTextMode.mode, hudMessageTextSprites, ..)`, `SetDrawMode`, `AddPrim`,
`ClearOTagR`). Neither store has a relocation. The words were hashed because they lie in `game.static` /
`port.bss` and no row cut them out; check 9 (b) named only the words the port had reported.

### 19.2 `csObjectPool[40]` +0xA4..+0xA7: a ricochet bomb's rotation padding (CONFIRMED)

**The object.** Element 40 is the first of the ten pooled ricochet bomb objects (`weaponAssetBind` asset 2210: +0x118
= 0x8A2, model = `weaponRicochetModel`, `weaponRicochetPool.slots[0..9]` = elements 40..49). From the port's RAM
dumps (`build/kc-hunt4`, Los Angeles, steps 1280-1300, both machines; `build/kc-hunt7`, Moscow, step 1000): element
40 has +0x118 0x8A2, model 0x80079534 and, in kc-hunt4, rot word +0xA0 = 0x0000FF81, +0xA4 = **0xFFFF0063** (rot z =
99, the padding FFFF), pos (-132, 188, 160): a ricochet fired and spent. It is not a camera, a pose object, the sky or
`viewSharedRoot` (the `ptr` rows), and no drop-in allocates it.

**The layout.** `rot` is `s16[3]` at +0xA0 (pitch, roll, yaw) and **+0xA6..+0xA7 is padding** (`cars.h` `struct
TM2Object`: `pad_0A6`); the request's "high half of +0xA4" is the padding, rot z (+0xA4..+0xA5) agrees.

**The writer: stack garbage, a retail bug (`RETAIL_BUGS.md` 36).** `weaponFireSelected` (kind 6) builds `short
vec[4]` and sets `vec[0..2]` only, then `memcpy(slot->rot, vec, 8)`: the padding takes `vec[3]`, `sp+0x1E` of a frame
whose stores are `sp+0x18`, `+0x1A`, `+0x1C` (retail 80103FB8 and `vw_port_P4` 80105028 alike: no store to
`sp+0x1E`). What lies there is
whatever an earlier call at that depth left: it depends on the machine's history (its drawing, sound and HUD calls, a
re-run's deeper stack), hence FFFF on the host and 0000 on another. `csReset` (`csAlloc`) clears `rot[0..2]`, not the
padding, so the value stays until the slot is fired again. The other 8-byte rotation copies move a padding from object
to object (`fx_sprite_spawn.c` the shields, `weapon_basic.c` the tornado funnel) or are the cameras' (`viewSetRotation`
/ `viewGetRotation`).

**No reader.** `RotMatrixYXZ` (the ricochet update, every object kind) reads three angles; the census of the step's
loads at +0xA6 / +0xA7 (and words at +0xA4) finds the car records, the stats' `hp` and the hit pass's own context, no
scene object.

**The fix: a row, no code.** `csObjectPool.rotpad	field	csObjectPool:0x128:0xA6-0xA8	yes	no` (two bytes per
element). Clearing `vec[3]` behind a bit would make the word equal on every machine, but nothing reads it and the
shields / tornado copies would still carry older paddings; the row covers every object's padding whatever wrote it.

### 19.3 The 256 bytes from `dc_y` (CONFIRMED: scratch)

In the port's build 801F8BD4 is `dc_y` (4 bytes; retail 801F5A8C, alias `D_801F5A8C`): **libpress's MDEC VLC decoder
state** (`MDEC_vlc2`, `ivlc_dct_dc_size`: `DecDCTvlc`, used when a menu or screen decodes a picture). The next 252
bytes are the start of `renderSharedWorkBuffer` (801F8BD8, retail 801F5A90, 0x1810 bytes): `renderInit`'s shared
scratch: the hit pass's node stack (`hdpInit`) and entry stack past the scratchpad's 41 entries (`hdpSetStackBuffer`),
the draw walker's overflow stack (`dbscanInitStacks`), `renderScene`'s near list, `bitmapSetupStorage`'s decode
storage (`renderGetSharedBuffer`). Every use pushes before it pops within one call (`hdpTraverse` sets the depth to 0
first), so no step reads a byte an earlier step or the drawing left. Not the port's symbol, not a debug cursor. Rows:
`dc_y` 4 bytes and `renderSharedWorkBuffer`, no / no. (Why after a drop-in: the machine that showed the drop-in's
screens decoded pictures there; any drawing differs too.)

### 19.4 The checks

**Check 9 (d), the drawing's stores through pointers.** `PtrStores` (`tools/check_rollback.py`) follows, per function,
an address formed from a LO16 / GPREL16 relocation through `addiu` / `addu` / `subu` / `or`, the stack slots
(flow-insensitive), the arguments (`$a0..$a3` and four stack words) and the callees' summaries (which arguments a
function stores through, at which offsets, and what it returns), to a fixed point over the call graph. An indexed
element counts as element 0 plus the store's offset; more than 8 values, or a pointer leaving its symbol, become the
symbol's element 0. A pointer loaded from memory and calls through pointers are not followed (the cameras' objects
are check 9 (c)'s `ptr` rows). Every pointer store of the drawing (the 8 roots, rtMainLoop's other callees, its and
`portRollbackLoopStep`'s own) must land in a "hash no" row or outside every row. It found, beyond the request's words:
`hudOrderTables` (the HUDs' OTs), `hudTargetNameModes` / `Sprites` (the target panel's name), `hudSplitVertLivesMode` /
`Sprites` and `hudSplitHorzLivesMode` / `Sprites` (the split HUDs' lives lines), `fontSpriteTemplates` (the colours
`fontSetColor` / `fontResetColor` set around each print; also from `worldUpdate`: yes / no), `fxSprites` +0x14..+0x1F
(a follow sprite's position, copied by `fxSpritesDraw` from `*owner_pos` per drawn view: field row `fxSprites.pos`,
yes / no; sprites are presentation, 4.5), `renderMeshList` and `TM2_PORT_FAR_LOD`'s `portFarLodTable` /
`portFarLodNear` (marked and put back within each drawn view): all no / no except where noted. **One reviewed
exception** (`DRAW_DEAD_STORES`): `hudDrawTargetPanel` sets the target car's `max_hp` to 1 when it is 0 (retail, a
divide guard in the drawing); every car's stats function sets it above 0 (`car_stats.c`, `carStatsDarkToothStage2`)
and the port's vacate / park paths keep it at 1 or more, so the store never runs in a battle. The request's words
join `DRAW_STATE` (check 9 (b)).

**Check 10 (b)** runs the same pointer census over the view and sound side the step reaches (and, with
`TM2_PORT_HUMANS`, `LOCAL_PRESENT`): it found the human records' camera block (`viewCarCameraUpdate` through
`worldHumanCamera`), `viewCameraObjectTable` (`viewCreateWithCamera`), `portHumanRadarBlink` and `soundDeniedPending`.

**Check 11, this machine's humans** (builds with `TM2_PORT_HUMANS`): (a) check 10's taint (registers, stack slots,
control dependence, dependent returns) from `LOCAL_SOURCES` and loads of `portHumanLocal` in every step function that
uses them, less the presentation (`LOCAL_PRESENT`, `VIEW_PRESENT`, `SOUND_SIDE`): no dependent state write, call or
return; the local presentation reaches no `rand()` and stores (directly or through pointers) only in "hash no" rows.
(b) The INPUT_ALL gate: `portHumanPadPort` loads `portHumanOptions` once, tests bit 0, calls only `portHumanSlot`, and
the call is reachable neither from the start without the test nor from the bit-set edge; with the bit modelled clear
the taint must find exactly `LOCAL_INPUT` (`portHumanAction`, `portHumanCombo`, `portHumanAnyLatched`, `humanCarTick`,
`carReadMoveCodes`, `humanCarReadInput`: the check is not blind). (c) `portHumanLocal`, `portHumanRadarBlink`,
`soundDeniedPending` and the camera block of every human record lie in "hash no" rows.

### 19.5 Runs

`--expect-functions --e0` of `work/build-20261022-204108-085848` (no source changed since):

- `work/pv/vw_rb_P4`, `vw_port_P4`, `vw_allh16_P4`: every check passes. Check 8: 912 / 1526 / 1526 rows resolve, 0 step
  stores outside a row. Check 9: 120 / 156 / 156 drawing functions, 0 hashed static stores, 1504 / 1603 / 1603 pointer
  stores, 0 hashed (1 reviewed on vw_port / vw_allh16; without `TM2_PORT_HUMANS` the target's record comes from a
  loaded pointer), 0 of 51 / 52 / 52 named words hashed. Check 10: the view and sound side 27 / 43 / 43 roots, 95 / 138
  / 138 functions, 469 / 511 / 511 pointer stores, 0 hashed. Check 11 (vw_port, vw_allh16): 7 users (20 call sites),
  385 dependent nodes, 0 findings; the local presentation 12 roots, 100 functions, 153 pointer stores, 0 hashed; the
  gate holds; 4 of 4 per-machine rows out; the bit-clear model finds exactly the six input functions.
- `tools/check_all.py` on the full build and the six links: 27 checks, 0 failed (retail hybrid `9219f7ba...`,
  source-only `d755c86e...`, sdk-lib `f557c1e3...`; proofs vw_rb P4 `afbd754e...`, vw_port P4 `456de05b...`, vw_allh16
  P4 `ee47e55e...`, placement P2, humans, car groups, hazards, balance, custom, rollback).
- **Negative controls** (`--regions`): request 57's list (`git show HEAD:config/US/rollback_regions.tsv`) fails check 9
  on `vw_rb_P4` and `vw_port_P4` (10 request words hashed; pointer stores to 17 / 20 hashed symbols: the HUD arrows,
  texts, OTs, fonts, sprites, ...), check 10 (b) (`viewCameraObjectTable`; on vw_port also the camera block,
  `portHumanRadarBlink`, `soundDeniedPending`) and on vw_port check 11 (c) (`portHumanLocal` and the rest hashed). The
  new list less `hudTargetNameModes` (a word no list names: the census alone) fails check 9 (d) on both links; less
  `humanCars.camera` fails check 10 (b) and 11 (c) on vw_port (64 camera words of 16 records hashed).

### 19.6 What the port does

1. Regenerate `rollback_regions.tsv` (its `tools/decomp/wc.py`). The new symbol rows replace the port's masks of the
   HUD words and `portHumanLocal`; `dc_y` and `renderSharedWorkBuffer` (0x1810 bytes, more than the 256 seen) leave the
   hash and the snapshot.
2. **Field rows of two bytes** (`csObjectPool.rotpad[i]`, 2 bytes at +0xA6 of each element) and others not word-sized:
   `recompile.py`'s `rb_spans` handles byte spans; if `rbs_hash` hashes whole words, it must mask partial words (or
   hash bytes) there, else element 40's padding stays in.
3. Keep INPUT_ALL (portHumanOptions bit 0) set online: it is what makes `portHumanLocal` presentation-only (check 11).
4. The camera block rows (`humanCars.camease`, `.camera`, `.view`) are the bytes the port masks already (HUMANS16 6.4):
   the port may drop its own mask once it reads them from the table.

### 19.7 Open notes

- **For the user:** `weaponFireSelected`'s uninitialised `vec[3]` (RETAIL_BUGS 36) is left as retail: a bit that clears
  it would change one game function for a word nothing reads. Say if the port wants it zeroed anyway (one line behind
  the define and a bit).
- `hudDrawTargetPanel`'s `max_hp = 1` guard is a drawing store into a car record (dead in battles, 19.4). If the port
  ever makes a car with `max_hp` 0 (a new mode's spare record), the drawing would change state on the machines that
  target it; the port's vacate and park paths already avoid it.
- `hudLivesText[7] = lives + '/'` (`hudDrawSingleView`, the split HUDs) writes into a `.data` string through a loaded
  pointer: outside every row (not hashed, not snapshotted), so harmless; the census does not follow loaded pointers.
- The pointer census is static: it follows relocated addresses, arguments and returns, not pointers loaded from
  memory nor calls through pointers (the renderer's node handlers). The port's cross-machine hash diff stays the final
  proof; new words go into the tsv and `DRAW_STATE`.

---------------------------------------------------------------------------------------------------------

## 20. Request 59 (2026-10-23): the arcs' end pointer, a seeker between machines, the palette shadow's old rows

Addresses: retail, and the port's build (`build/decomp-wc/out/TM2_SOURCE_PORT/symbols.tsv`: `csObjectPool` 805FCA00,
`weaponArcs` 80615E00, `fxRecords` 805F1240).

**Summary.** (1) `weaponArcsCountField` +0xC is `weaponArcs[0].to` (+0x10), retail's store of `weaponArcsDraw`'s stack
vector `free_end` for an untargeted arc: a stack address, 0x400 deeper in a re-run, never read back. New row, hash no.
(2) A seeker's `target` is always a car scene object's position (+0xA8), the shooter's own object (Roadkill's
boomerang) or 0; the step writes those positions itself (`humanCarUpdate` / `aiCarUpdate`) and no decomp drawing code
moves a car object, so the decomp gives every machine the same aim point. The parting the port saw needs a read
outside the end-of-step hash: 20.2 narrows it to two windows the port can test; no decomp change yet.
(3) `portClutShadowCapture` now empties the shadow first (`portClutShadowReset`, new): a round's shadow holds only
that round's rows. Retail images unchanged.

### 20.1 `weaponArcsCountField` +0xC: `weaponArcs[0].to` (CONFIRMED: a stack address, hash no)

`weaponArcs` (retail 801847B4, 5 records of 20 bytes; the port's build 80615E00, `TM2_MAX_ARCS` 16) is
`struct GC15Taser { s16 id, source, count, attachment, target, pad; s32 *from; s32 *to; }` (`weapon_arcs_draw.c`).
`weaponArcsCountField` is the label retail's `weaponArcSlotFind` reads `count` through (+4), so +0xC from it is
+0x10 of record 0: `to`, the arc's end point.

- **Writers.** `weaponTaserTarget` (`weapon_slot_target.c`) sets `to` to the victim's `motion.pos` (a car record) or
  0; `weaponLightningScan` (`SCAN_SLOT`) likewise; the port's taser retry (`portBalanceTaserRetry`, BALANCE 12.4)
  when it connects late. And `weaponArcsDraw` (801084D8; the port's build 801099E0), for an untargeted arc
  (`target` < 0), stores `weaponArcs[i].to = free_end`, the address of its local `s32 free_end[3]` (`sp+0x18`:
  `addiu s6, sp, 0x18; sw s6, %lo(weaponArcs+0x10)(at)`). With `ONCE_PER_STEP` only the tick pass (the step's call
  from `portRollbackStepEffects`) stores it, as section 11 noted. A re-run from `portRunStep` runs the step 0x400
  deeper than `rtMainLoop` does (as for `fxSprites.owner`, 16), hence 801FF5E0 / 801FF1E0: the request's 4 of 300
  checks are the steps with an untargeted arc live (a taser shot that missed, a lightning slot without a car).
- **Readers.** `weaponArcsDraw` reads `to` at the top of each record and replaces it by its own `free_end` whenever
  `target` < 0, before any use (with `TM2_PORT_ROLLBACK` its `to` is a local in every pass; retail reads the word it
  just stored, in the same call). The port's car-range helpers (`portDropInIn`: the drop-in, the record move, the
  hand-over) only test whether it points into a car record, which a stack address never does. The port's
  `interp.c` redraw points a car-record `to` at the car's object for a blended frame and puts the record back. So for
  an untargeted arc the word is never read as a point; for a targeted arc it is the target's record position, fixed
  by the hashed `target` id.
- **Row** (`config/US/rollback_regions.tsv`): `weaponArcs.to field weaponArcs:20:0x10-0x14 yes no`, one row per
  record (5 in retail-sized links, 16 in the port's). `tools/rollback_regions.py`'s `field` kind now takes the
  array's extent from the first symbol at or past one element, so a label inside element 0 (here
  `weaponArcsCountField`) no longer ends the array; every existing field row resolves to the same elements as before
  on the rb, port and allh16 links (P4 and P2).
- **No code change.** A static copy instead of the stack address would make the word hashable, but nothing reads it;
  retail's store stays (section 11's choice).

### 20.2 The seeker of stage 7 (`csObjectPool` element 56): what `target` can point at

Element 56 (the port's build 80600AC0) is a projectile record's scene object (`fxRecords[k]` +0x34).
`weaponProjectileSteer` (800DA5B4; the port's build 800DAB1C) turns its `rot` (+0xA0) toward `rec->target` (+0x38)
and rebuilds its matrix (+0x80) with `RotMatrixYXZ`. It is called by `weaponProjectileUpdate` (800DB110) from
`fxRecordsUpdate` (800DC348; the port's build 800DC8D8), the step's third update (`portStepBody`: after `padUpdate` and
`targetActorsUpdate`, long before `worldUpdate`).

**Every writer of `target` (fxRecords +0x38)** (CONFIRMED, every call site):

| writer | value |
|---|---|
| `weaponFireHomingMissile` (800DA044) and its wrappers `weaponFireFreezeMissile` (type 14), `weaponFireSpiralMissile` (0x3D), `weaponFireRoadkillBoomerang` (51), `weaponFireDarkToothMissile` (61); `weaponFireColoredMissile` (800DA448, 0x29; `weaponFireGhostMissile` 0x31); `weaponFireNapalm` / `MegaNapalm` (15) | the caller's `effect` / `shot` |
| `fxRecordInitObject`, `weaponSpawnProjectile` (10: Mr. Grimm's), `weaponFirePowerMissile` (12) | 0 |
| `weaponProjectileSteer`, type 0x29 off by more than 341 | 0 (no more steering) |
| `weaponProjectileSteer` / `weaponProjectileUpdate`, type 51 once 300 away | `rec->victim->pos` (+0x3C: the shooter's own object, `weaponFireSpecial`'s `lookup`) |

The callers' `effect` / `shot` come only from `weaponFindTargetAhead` (80109F94; the port's build 8010B944) /
`weaponFindTargetBehind` (8010A2AC; 8010BC6C), called right before each fire (`weaponLaunchProjectile`,
`weaponFireSpecial`, `weaponFireEnergyAttack`, `aiDarkToothBossUpdate`, the port's `portCarDtPlayerGuns`; never
uninitialised: every fire path runs the search first). The search writes `&obj->pos` (object +0xA8) of the nearest AI
car object (`aiCarObject(i)`, hp > 0) or human car object (`humanCarObject(i)`, not the owner) in the owner's cone,
judged from the owner object's matrix (+0x80) and position, or 0. An AI owner outside the attract demo takes
`AI_AIM_TARGET`: retail `humanCarObject(0)->pos`; with `TM2_PORT_HUMANS` `portAiAimTarget` (the port's build
808BC610): human 0's object, or with `portHumanOptions` bit 2 the bot's own target's object (`aiCarTargets`,
`humanCarNearest` from the record's position). **There is no lock-on and no HUD target box in it**: the HUD's target
panel (`hudDrawTargetPanel`) only reads. (The port's host-side `vp_FindTargetAhead` / `Behind` replace the search when
the port's C block is off: `vplus/fixes.c` `ai_aim` / `team_aim`; they also store object +0xA8 pointers.)

**Is the point the same on every machine?** Yes, as far as the decomp's code goes:
- The objects are `csObjectPool` elements bound at the load (`carBindAiSlot`, the human binding in `scene_init.c`,
  through `carSlotObjects`), in the snapshot and the hash (only +0x14..+0x7F, +0xA6, +0x124 and the camera objects
  are out). `humanCarObjects` / `aiCarObjects` are hashed static words.
- Their pose is the step's: `humanCarUpdate` (8011C400) / `aiCarUpdate` (8011C4A0) copy the record's `rot`, `pos` and
  matrix into the object every step (`worldUpdate`).
- No decomp drawing code writes a car object's pose: a scan of every function the drawing reaches (check 9's set) for
  stores at +0x80..+0xB3 through any pointer finds only `viewProc`'s camera-object copies (mode 4's mirror: `ptr`
  rows) and renderer records (`renderScene`, `renderNearPolys`, `fxDebrisDraw`, `sub_800CDC54`: their own structs,
  not scene objects).
- The steering reads nothing else: `rec` (hashed), the missile's own object (hashed; its +0xA8 stayed equal in the
  request), and `rand()` for types 0x29 / 0x31 (the hashed seed).

**So the parting needs a read the end-of-step hash cannot see.** Two windows fit "every other hashed word equal,
element 56's +0x80..+0xA0 parting and growing, deterministic, each joiner its own":
1. **A car object's position between steps.** `fxRecordsUpdate` reads the target's object +0xA8 *before*
   `worldUpdate` rewrites it in the same step. Whatever moves a car object between two steps on one machine and is not
   put back is overwritten before the hash is taken: the hash stays equal while the seeker has already turned. The
   decomp's drawing does not do it (above). The port's own C does write car objects' poses between steps:
   `vplus/interp.c` (`write_blend`: +0x80, +0xA0, +0xA8 of every pool object for the in-between redraws; it saves
   the pool first and puts it back after each redraw), `killcam.c` and `ui/photo.c` (cameras). If any path runs a step
   (`portRunStep`, a resync's load, a kill cam's end) while a blend is in place, or a put-back is skipped, the next
   step's seekers read each machine's own blend.
2. **A target outside the hash.** The search's `cand == 0` test never fires (`cand` is `obj + 0xA8`; RETAIL_BUGS 37):
   a record counted (below `aiCarTotal` / `humanCarTotal`) without its own object gives the target 0xA8 (low RAM,
   not snapshotted) for a null pointer, or another object's position for a stale one (a camera object: hash no, each
   machine's camera). This needs a level where more records are counted than cars bound, e.g. a tournament level whose
   computer-car count differs from the earlier levels' (the port's `vp_TournamentOpponentCount` sets it per level;
   the request's run predates the port's "the same computer cars every stage").

**What the port can log to tell them apart** (the run is deterministic): at `fxRecordsUpdate`'s entry on each
machine, for the record whose +0x34 is element 56, `target` (+0x38), the 12 bytes there, and which pool element (if
any) holds that address; and at the top of each step, every car object's +0xA8 against its record's `motion.pos`
(humans: record +0x6FC; computer cars with `f_59C` set: +0x73C): a mismatch on one machine is a between-steps write.
If (1): the fix belongs where the port moves objects (put them back before any step); the decomp can also add a pose
shadow under a bit (save every car object's +0x80..+0xB3 at the end of `portStepBody`, put it back at its start: a
no-op in retail order, where nothing writes them in between). If (2): keep an object bound for every counted record
(as the drop-in's vacant records keep a parked dummy), or skip null objects in the search under a bit. Both are an
open decision (20.6).

The state transfer carries every pointer involved (`fxRecords`, `humanCarObjects`, `aiCarObjects`, `csObjectPool`:
static, snapshot yes); such a pointer means the same object on every machine because the pool is at one address and
the binding order is the load's. No `snapshot no` static row holds an object pointer.

### 20.3 `portClutShadow` emptied each round (FIXED, bit 6)

`portClutShadowCapture` (`portRollbackLoopEnter`, `rtMainLoop`'s entry; the port's build 808C2610) found or added a
row per palette definition but never dropped the rows of earlier rounds, and `portClutAnimUpload` (808C2708) writes
every row back: after a re-run a tournament's later level got the earlier levels' palettes. Now the capture first
calls `portClutShadowReset()` (new, `.port_text`, `port_rollback.h`): every word of `portClutShadow` 0 (rows, overflow
and the colours: 4194 words), then reads each definition's row from VRAM as before. Only with `CLUT_SHADOW` (the
capture returns before the reset without the bit); retail and every build without `TM2_PORT_ROLLBACK` unchanged. A
capture the port calls mid-round (after a drop-in's palettes) loses nothing: every shadow row is a definition's
(`clutAnimStart*` / `clutAnimStep*` take `def->x`, `def->y`), and the capture re-read each of them from VRAM before
as well. Two machines now hash the same shadow bytes whatever rounds each played. The port may drop its own clear
(`net/rb_state.c rbs_new_round`) or keep it (harmless).

### 20.4 The checks

`tools/check_rollback.py` check 12 (request 59): (a) every element's `weaponArcs` +0x10 and `fxSprites` +0x2C lies in a
"snapshot yes, hash no" row; (b) the evidence of 20.1: `weaponArcsDraw` has a store relocated to `weaponArcs + 0x10`
whose value register comes from an `addiu` off `$sp`; (c) `portClutShadowReset` is in `.port_text`, calls nothing,
addresses `portClutShadow` and loops over its 4194 words, and `portClutShadowCapture` calls it before every
`gpuStoreImageLine` (dominance). Check 1 lists `portClutShadowReset` among the port functions; check 5 allows it as a
user of `portClutShadow`.

### 20.5 Runs

- Links of this source (`work/pv/r59_{rb,port,allh16}_{P4,P2}`, the defines of section 19's three links): proofs
  against E0 pass, P2 placement passes; P4 rb `3e05e79c...`, port `ec97977f...`, allh16 `992bfd7f...`.
- `tools/check_rollback.py --expect-functions` on the three P4 links: every check passes. Check 12: 5 / 16 / 16
  `weaponArcs.to` elements and 150 `fxSprites.owner` elements in "hash no" rows, one `$sp`-formed store to
  `weaponArcs + 0x10` in `weaponArcsDraw`, `portClutShadowReset` (a down-counter from 4193) before the capture's VRAM
  reads. Check 7: the 37 (rb) game functions as before (the reset is a port function).
- `tools/check_all.py` on the full build (`work/build-20261023-210312-087647`, port image `bb016699...`) and the six
  links: 27 checks, 0 failed (retail hybrid `9219f7ba...`, source-only `d755c86e...`, sdk-lib `f557c1e3...`,
  unchanged).
- **Negative controls:** request 58's list (`git show HEAD:config/US/rollback_regions.tsv`) fails check 12 (a) on
  `r59_rb_P4` and `r59_port_P4` (no `weaponArcs.to` row); a link of request 58's source (`vw_rb_P4`) fails check 1
  (`portClutShadowReset` missing) and check 12 (c) (no call before the capture's reads).

### 20.6 What the port does

1. Regenerate `rollback_regions.tsv`: `weaponArcs.to[i]` (4 bytes at +0x10 of each arc record) leaves the hash, stays in
   the snapshot. The port's resolver of `field` rows must take the array's extent past `weaponArcsCountField` (a
   label inside element 0), as `tools/rollback_regions.py` now does; else it sees no element.
2. Call nothing new: `portClutShadowCapture` (from `rtMainLoop`'s entry, and the port's own calls) empties the shadow
   itself. The clear in `net/rb_state.c rbs_new_round` can go, or stay.
3. For the seeker (20.2): log the target and the car objects' positions at the step's start as described, on the
   stage 7 run; send the result back so the fix lands in the right place.

### 20.7 Open notes

- **For the user (decision):** the seeker's parting is not explained by the decomp's step. Which fix to make depends
  on the port's log (20.2): (1) a between-steps write to car objects (port side, or a decomp pose shadow under a new
  bit), (2) a counted record without an object (bind one, or skip null objects in the search under a bit). Nothing is
  changed until then. **Resolved by request 60** (section 21): window 2's kind (a target outside the hash), but
  reached through a record of the previous level's seeker that the round start revives with its old target, not a
  counted car without an object (bit 11 `ROUND_CLEAN`).
- RETAIL_BUGS 37 (the search's dead null test) is left as retail.
- The pose scan of 20.2 is a static census of the decomp's drawing; the port's own C (`interp.c`, `killcam.c`,
  `photo.c`, `fixes.c`'s target hooks) is outside it.

## 21. Request 60 (2026-10-24): a seeker carried into the next round

Addresses: retail, and the port's build (`build/decomp-wc/out/TM2_SOURCE_PORT/symbols.tsv`: `fxRecords` 805F1240,
`csObjectPool` 805FCA00); the two new port functions at their address in a link of this source with the port's
defines (`work/pv/r60_port_P4`).

**Summary.** (a) Yes: no round reset clears a projectile record. `projectilesReset` (every `carRestart`: each human's
bind at the load and each round start, and each respawn) sets every record's life to -1 and hides its object, and
keeps the type, the target and the rest; `missilesCreateObjects` (the load) gives each record a new object and life
-15 and keeps them too. Life -1 is the first step of the 11-step fade (`fxRecordsUpdate` runs
`weaponProjectileUpdate` while life >= -11), so **every record that ever flew** comes back for the new round's first
11 steps with its old type and target: a freeze missile steers its new (hidden) object at its old target, which after
a level change is a pointer into the re-allocated `csObjectPool`, now another object's. Retail does the same
(RETAIL_BUGS 38; harmless on one machine). The other pools are cleared where it counts (21.2). (b) Fixed under a new
bit 11 `ROUND_CLEAN` (0x800): `roundStart` clears every record (`portRoundCleanProjectiles`), and a seeker whose
target is no counted car object's position stops steering (`portSeekerTargetOk`). Bit clear: retail.

### 21.1 The records at a round change (CONFIRMED)

`fxRecords` (retail 801A5B08, 16 records of 980 bytes; the port's build 805F1240, 48) is the projectile table:
+0x02 type, +0x04 life, +0x06 speed, +0x08 turn rate, +0x0A damage, +0x0C hit, +0x28 origin, +0x34 the record's scene
object, +0x38 target, +0x3C victim (the boomerang's own car), +0x40 / +0x41 / +0x44 / +0x45 the trail's segment, slot,
count and timer, +0x42 owner, +0x46 drop, then the trail's vertices, render object (+0xF0), strip (+0xF4) and
segments (+0x14C).

**Life.** > 0 flying. 0: `fxRecordsUpdate` frees it (`fxRecordFree`: life -1, object +8 = 0, hidden). -1 to -11: the
fade, the trail's last steps: `fxRecordsUpdate` still calls `weaponProjectileUpdate`, which runs the type code at its
top (type 15's spin, type 61's wobble and two `fxSparkPink` sparks, both through `rand()`), **steers whenever
`target` is set** (`weaponProjectileSteer`: the object's `rot` and matrix toward the target, type 0x33's re-target
at `victim`, type 0x29 / 0x31's `rand()` wobble), then `--life` and `fxTrailUpdate` (the trail's slot from the
object's matrix), no move and no hit test. -12 and below: idle (the trail object hidden each step). -15:
`missilesCreateObjects`' idle value. `fxRecordFindFree` takes any record with life <= 0. So "life -12" in the
request's log is a dead record and "life -2" a revived one one step into the fade.

**Who resets what** (every caller, retail and the port's build):

| function | when | fxRecords fields written |
|---|---|---|
| `missilesCreateObjects` (800D90C0; 800D957C) | `arenaResetSystems`, the load (`sessionLoadArenaAndCars`): every level | +0x00 id, +0x04 life = -15, +0x34 a new object (`csAlloc`; +8 = 0, +0xC = 0, +0x118 = 10); after `csPoolInit`, so element numbers are the new level's |
| `missileTrailsInit` | the same, after | the trail strip header (+0xF4..) and the trail object (+0xF0) |
| `projectilesReset` (800D79C4; 800D7D20) | `carRestart`: `carBindHumanSlot` (the bind after the load), `humanCarsRestart` (`roundStart`, every round), `humanCarTick` / `sub_8011E114` (a respawn) | +0x04 life = -1, object +8 = 0; `bulletsClear` |
| `fxRecordFree` | an expiry, a hit | life -1, object +8 = 0 |
| `fxRecordsClear` (800D9198; 800D969C) | **never called** (no caller in retail or any link; no pointer to it) | (`fxRecordFree` on each) |

Nothing writes +0x02 (type), +0x38 (target), +0x3C (victim) or +0x42 (owner) but a fire (`fxRecordInitObject` and the
fire functions, which set every head field) and the steering itself. The sequence of a tournament's next level:
`sessionRun` → `sessionLoadArenaAndCars` (`arenaResetSystems`: `csPoolInit`, ..., `missilesCreateObjects`: life -15,
new objects) → `sessionBindCars` (`carBindHumanSlot` → `carRestart` → `projectilesReset`: life -1) →
`roundStartMusic` → `roundStart` (`humanCarsRestart` → `carRestart` → `projectilesReset`: life -1) → `rtMainLoop`.
The first step's `fxRecordsUpdate` finds every record at -1 with its previous level's type and target: type 14
(freeze missile) record 0 with target 80603D20 steers object element 56 at `csObjectPool` element 99 +0xA8, which in
the new level is another object (in the request's run each machine's own: a camera object, "hash no" except its
pose rows, written per machine), for 11 steps: the parting of the request. A same-level next round does the same
with a target that is still a car object (deterministic: the binding is the load's).

**Retail too.** All of it is retail code (no port define touches these functions). On one machine the revived seeker
only turns a hidden object whose pose the next fire overwrites (`fxRecordInitObject`), so nothing shows, except type
61 (Dark Tooth's missile): 22 pink sparks at the record's object for the first 11 steps of each later round, and the
`rand()` draws they take. Logged as RETAIL_BUGS 38.

### 21.2 The other pools at a round change (CONFIRMED)

A pool is safe when its update skips a record by a field the round's resets clear (stale data in a skipped record is
never read):

| pool | round reset (caller) | what it clears | the update's test | at a round change |
|---|---|---|---|---|
| `fxRecords` (projectiles) | `projectilesReset` (`carRestart`), `missilesCreateObjects` (load) | life -1 / -15, object hidden; **type, target, victim, owner kept** | life >= -11 | **leaks** (21.1) |
| `bulletRecords` (40) | `bulletsClear` (`projectilesReset`, `arenaResetSystems`) | life = 0 | life > 0 | clean |
| `fxSprites` (150) and impact particles (20) | `fxSpritesReset` (`carRestart`, `sessionRun` each round, the load) | frame -1, table 0, id; particles' life 0 | frame >= 0; life > 0 | clean |
| `debrisChunks` (8) / `debrisBursts` (4) | `debrisClear` (`roundStart`), `debrisInit` (load) | active 0, object hidden; bursts zeroed | active | clean |
| `weaponDrops` (mines, drops; 25 / 6 N), `weaponMineObjects`, `weaponRemoteBombObjects` | `weaponDropsReset` (`carRestart`) | timer (+8), kind bytes; object hidden and dropped; every mine / remote-bomb object hidden | timer > 0, else a held object | clean |
| `weaponArcs` (5 / 16) | `weaponsRoundReset` (`roundStart`, `carRestart`) | count -1 (`id`, `target`, `from`, `to` kept) | count > 0 | clean (the kept fields are read only while count > 0) |
| `weaponRicochetPool.shots` (10) | `weaponsRoundReset` | life (+4) 0, object hidden and dropped | life > 0 | clean |
| `weaponShieldPool.records` (4) | `weaponsRoundReset` | timer, owner, source, object | timer > 0 | clean |
| `weaponFlameEmitters` (2) | `weaponsRoundReset` | timer 0 | timer > 0 | clean |
| `groundFires` (30) | `weaponsRoundReset` | kind, count, centre, the five pointers | kind | clean |
| tornado, strike, slam records | `weaponsRoundReset` | timer / active, the objects hidden; the slam bytes | timer / active | clean |

### 21.3 The fix: bit 11 `ROUND_CLEAN` (0x800)

- `portRoundCleanProjectiles()` (`.port_text`, `port_variant_arrays.c`; 808C20C8 in `r60_port_P4`), called by
  `roundStart` (8011AD6C there) after `debrisClear`, before `humanCarsRestart` (its `projectilesReset`) and before the
  round's first step; with the bit it clears every record's fire fields: +0x02..+0x33 (type, life = -15 as the load's,
  speed, rate, damage, hit, +0x10..+0x27, the origin) and +0x38..+0x47 (target, victim, the trail's segment / slot /
  count / timer, owner, drop), and hides the record's object (+8 = 0). It keeps the id (+0x00), the object (+0x34) and
  the trail from +0x48 (the load's `missileTrailsInit`). Every record of the live table (`TM2_PROJECTILES_LIVE`: 16, or
  48 with `portCarPoolsLive`). The round's `projectilesReset` then sets life -1 as in retail: 11 fade steps of a type-0
  record with no target, which only refresh its own trail's slot from its own object (hashed state; no `rand()`, no
  sprite, no turn). Every fire sets the cleared fields again (`fxRecordInitObject` and the fire functions; the trail
  counters come from `fxRecordInitObject` before `missileTrailStart`), and a power-on record has them 0 too.
- `portSeekerTargetOk(target)` (`.port_text`; 808C21AC): with the bit, 1 only when `target` is `obj + 0xA8` of
  `humanCarObject(i)` (i < `humanCarCount()`) or `aiCarObject(i)` (i < `aiCarCount()`), a non-null object; without the
  bit 1. `weaponProjectileUpdate` (800DB784 there) asks it first for every record (before type 15's untargeted test) and
  drops the target when it says 0. Every target the game sets is such an object (20.2's table: the search's car objects,
  `AI_AIM_TARGET` / `portAiAimTarget`, the boomerang's own car, the port's `ai_aim` / `team_aim`), so normal play steers
  as before; a stale pointer (an earlier level's, a car whose object a drop-in replaced: `portHumanVacateRecord`'s
  dummy) and RETAIL_BUGS 37's 0xA8 stop steering. Defence in depth: with the clean nothing stale is left at a round
  start.
- Both are no-ops without the bit; neither calls `rand()`. Retail and every build without `TM2_PORT_ROLLBACK`
  unchanged; with the define and the bit clear the game's logic is retail's (the two calls return at once / 1).
  With the bit the round's first 11 steps lose the fade's effects of the earlier round's records (type 61's sparks and
  their `rand()` draws, the hidden objects' turns): the same on every machine.
- Game functions changed with the define: `roundStart` (already one of them) and `weaponProjectileUpdate` (new: 37
  functions with `TM2_PORT_ROLLBACK` alone, 38 without `TM2_PORT_HUMANS`). Port functions: 36.

### 21.4 The check

`tools/check_rollback.py` check 13: (a) `portRoundCleanProjectiles` is in `.port_text`, calls nothing, and is
**run** (a small MIPS I interpreter of the link's code, `MiniMips`) on records filled with a pattern: with
`ROUND_CLEAN` every live record's +0x02..+0x33 and +0x38..+0x47 end 0 (life -15), the id, object and trail bytes are
untouched, each object's +8 is 0 and nothing else is written; with every other bit set and this one clear it writes
nothing, and the same comparison then fails (the test is not blind); both for `portCarPoolsLive` 0 and 1 (16 / the
link's `fxRecords` entries). (b) `portSeekerTargetOk` is run with stubbed car tables (three humans, one without an
object; two AI cars counted, a third not): 1 for a counted object's +0xA8, 0 for 0xA8, an object's base, +0xAC, a
stale address and the uncounted AI car; without the bit 1 for all, calling nothing. (c) `roundStart` calls the clean
once and the call dominates `humanCarsRestart`; `weaponProjectileUpdate` calls the test once, the call dominates
`weaponProjectileSteer`, and it stores 0 at +0x38. (d) The resets of 21.2's table are called on the round's path
(`roundStart` → `weaponsRoundReset`, `debrisClear`, `humanCarsRestart` → `carRestart` → `weaponDropsReset`,
`weaponsRoundReset`, `projectilesReset` → `bulletsClear`, `fxSpritesReset` → `fxImpactParticlesClear`) and each
addresses its pool. Check 1 lists the two functions; check 7 expects `weaponProjectileUpdate`.

### 21.5 Runs

- Links of this source (`work/pv/r60_{rb,port,allh16}_{P4,P2}`, the defines of section 19's three links): proofs
  against E0 pass, P2 placement passes (no unexpected function); P4 rb `28338e5c...`, port `8a106f22...`, allh16
  `0dbeb1e6...`.
- `tools/check_rollback.py --expect-functions` on the three P4 links: every check passes. Check 13: 16 records (rb),
  16 and 48 records (port, allh16: `portCarPoolsLive` 0 / 1) cleared exactly with the bit, nothing written without it,
  8 target cases each way, both call sites dominated, the 11 reset edges and 14 pool references found. Check 7: 38
  game functions in the rb link (37 + `humanCarTick`), with `weaponProjectileUpdate`.
- `tools/check_all.py` on the full build (`work/build-20261024-210954-894303`, port image `bb016699...`, unchanged:
  the variant without port defines) and the six links: 27 checks, 0 failed (retail hybrid `9219f7ba...`, source-only
  `d755c86e...`, sdk-lib `f557c1e3...`, unchanged).
- **Negative control:** a link of request 59's source (`r59_rb_P4`) fails check 13 (no `portRoundCleanProjectiles` /
  `portSeekerTargetOk`, no call in `roundStart` / `weaponProjectileUpdate`), check 1 (the two functions missing) and
  check 7 (`weaponProjectileUpdate` unchanged). Inside check 13 the bit-clear runs are the model's own control: the
  comparison must find the records uncleaned, and the test must answer 1 without calling anything.

### 21.6 What the port does

1. Set bit 11 (`TM2_RB_ROUND_CLEAN`, 0x800) with the others online: `TM2_RB_ONLINE` is now 0xFFF. No call, no new
   word, no new row (`rollback_regions.tsv` unchanged: `fxRecords` was already snapshot and hash).
2. The resync that repairs the parting now can stay as it is; with the bit the stage 7 run should keep every
   `fxRecords` +0x38 0 at step 3009 (records of type 0, life -2).
3. A port path that rebinds a car's object mid-round (the drop-in) no longer leaves a missile steering at the old
   object (the test drops such a target).

### 21.7 Open notes

- **Decision taken (the user may revisit):** one bit for both parts (the clean and the target test); the test could
  get its own bit if the port wants to measure them apart.
- The clean runs at `roundStart`, not in `projectilesReset`: a respawn's `carRestart` mid-round keeps retail's fade of
  the live missiles (their targets are the round's cars).
- `fxRecordsClear` (retail 800D9198) is dead code in retail; left as it is.
- RETAIL_BUGS 38 is not fixed in retail builds.

## 22. Requests 63, 66 and 67 (2026-10-25): the camera wall, the steering in reverse, the pickup respawn

**Request 63** (`FAR_LOD.md` 10, `portVisual` bit 5): `viewChaseCamera` runs in the step (section 18) and now calls
`portVisualCameraWall`, which reads the level model and writes only the human record's `cam_pos` (the camera block:
"hash no", section 19) and its stack. No `rand()`, no static store (check 8 / 9 census; check 10 (b) covers it under
`viewChaseCamera`). Rows: **`portVisual` gets its own row, snapshot no, hash no** (it was inside the `port.bss`
"yes / yes" range, so two machines with different visual options hashed differently; every bit of it draws only:
the arcs' depth and lift, the walk order, the shadows, the first streak, the camera). The camera's eye may differ
between machines (its own view settings, `LOCAL_VIEWS`) and between a run and a re-run: no step reads it as state.

**Request 66** (`ANALOG_STEERING.md` 2.6): **`portHumanControl[TM2_MAX_HUMANS]`** (`.port_bss`, after
`portPreciseEmit`): snapshot yes, hash yes (its own row, inside `port.bss`). The step reads it (`carThrottleSteer`,
`carRicochetOffObject` through `portHumanSteerKeep`): it changes the cars' motion, so every machine must hold the same
bytes (the port writes it from the match setup, each player's Controls option). No `rand()` added.

**Request 67** (`BALANCE.md` 16, `portBalance` bit 23): game state like every balance bit (latched in
`portBalanceLive` at the round's start); `pickupRespawnTick` keeps its `rand()` count (the rand census of check 2:
unchanged); the pick reads the pickup slots, the pickups' models and the selects, all hashed (`game.static`, the
`dmd` rows).

**Checks:** `check_rollback.py` (rand census, checks 8-10 on the new rows) passes on the port's and the "all" links;
the new checks are `check_custom.py` 9, `check_humans.py` 13, `check_balance.py` G22. Verification: full build `work/build-20261025-192227-415794` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK from `.LIB` `f557c1e3…`, default port variant `1becd00a…`: unchanged); links `work/pv/r63_{rb,port,allh16}_{P4,P2}` (port's `r63_port_P4` `dda33c12…`); `check_all.py` 27/27 (proofs, placement, check_rollback, check_custom 9, check_balance G22, check_humans 13). Not runtime-tested.

## 23. Request 86 (2026-10-30): drawing changes the battle through the stack

Port request 86 (after request 82, decomp 7d0d52c): with request 82 linked, the port's `sixteen` scenario (16 cars,
one view, `portPreciseEmit` 0) is equal to a5c57ef's through frame 229 and differs from frame 230, the match's
first: 946 words of the car records, the cars start in other places; `two`, `koth` and `interp_killcam` stay equal.
The port suspected the scratchpad the asm near pass used to leave, or the stack. Asked: find the read (a step must
never read what the drawing left: each machine draws its own views).

### 23.1 Not the scratchpad (CONFIRMED)

- The asm near pass leaves its register saves and temporaries at 0x00..0x53, 0x58..0x5B and 0x78..0x8B (request
  82's check measured it); the C leaves them as they were.
- **The step's scratchpad readers** (every function `portStepBody` reaches, `r86/stack.py`): the hit walker, the
  renderer walker (through `debrisStartBurst`'s `renderDrawObjectUnculled`) and two libsnd functions (hardware
  registers, not the scratchpad).
  - The hit walker (`hdpPassRun`, `hdpWalk` and the leaf tests: asm in the variant; `r86/hdp16.py` lists every
    scratchpad load and store of its 24 functions): `hdpCollideObject` sets 0x00, 0x04, 0x1C, 0x20, 0xB4 and 0xC4
    before each pass, `hdpPassRun` sets 0x08..0x18, 0xA4..0xC0, and every other word the walker reads (0x24..0x2C,
    0x30..0x64, 0xCC) is stored by the same function earlier on its path. No word is read before the pass writes it.
  - The renderer walker reads its context only after `renderInitState` set it.

### 23.2 The stack (the port's test, 23.4: NOT the cause)

- **What request 82 changed under the step:** `rtMainLoop` calls the drawing (`renderScene` and below) and the step
  (`portRollbackLoopStep` > `portStepBody` > `worldUpdate` > ...) from the same frame. The asm near pass used no
  stack; the C one does: `renderNearPolys` 0xC0 bytes and `portNearReport` 0xC8 below `renderScene`'s 0x58, so the
  words 0x58..0x1E0 below the loop's frame now hold the near pass's locals instead of what the emitter (or an earlier
  drawing call) left there. `worldUpdate`'s frame (0xB8, from 0x30 below the loop's) and its callees' lie on that
  range (`r86/depth.py`: the step reaches 0x11A0 bytes deep, the drawing 0x260).
- **The step reads stack it has not written:** a static pass over the step's 895 functions (`r86/stack.py`,
  `stack_strict.py`: an sp-relative load that a path from the entry reaches with no store of its slot) finds 411
  such loads. Most are out-parameters a callee fills only on success and the caller reads only after testing that
  (e.g. `carCrushContactUpdate`'s `peer_side` from `carFindByObject`, `aiCarDrive`'s `th`): the pass does not see the
  test, so they are not proven reads. The three examined in the start path (`aiPickLinkNode`'s `pos` through
  `aiFollowRouteTarget`, `aiGetTargetCar`'s `th`) are filled on every path. The reading function was not found
  statically; `carFromSlot` (no `*human` for an empty slot) and the AI's route helpers are the likeliest kind.
- 16 cars run code paths the 2-car scenarios do not (more AI cars, the start spacing's all-taken case), which fits
  `sixteen` alone changing.

### 23.3 Built: the step's stack filled before each step (`TM2_PORT_ROLLBACK`)

- **`portStepScrub`** (bytes, 0 = off) and **`portStepScrubFill`** (the word), the port's settings (`.port_bss`,
  snapshot and hash no): with `portStepScrub` > 0, `portStepBody` first calls `portStepStackScrub`, which sets that
  many bytes (words, at most `TM2_RB_SCRUB_MAX` 0x8000) below `portStepBody`'s frame to the fill. Then whatever the
  step reads there is the fill or what the step itself wrote: the same on every machine and in every build,
  whatever was drawn. Pure C: the caller's sp is the address of the function's argument (o32's home slot); the
  function keeps no frame (check_rollback checks both).
- **Size:** `TM2_RB_SCRUB_STEP` 0x1200 covers the step's deepest chain (0x11A0: `worldUpdate` > ... >
  `portVisualCameraWall`'s 0xF00 frame). It must stay inside your stack: below the loop's frame there must be that
  much stack (it is the step's own when the camera wall runs, which it does every step with `portVisual` bit 5).
- **To confirm the cause** (your `compare_builds.py`): `sixteen` on 7d0d52c with `portStepScrub` 0x1200 and fill 0,
  then with fill 0xA5A5A5A5. If the two differ, the step reads stack below its frame (and a per-step word diff
  between them points at the reader: the first record word that differs). With fill 0, a5c57ef and 7d0d52c (and every
  later build) should then give the same `sixteen`. The old baselines cannot come back exactly: they held the asm
  near pass's leftovers.
- **Recommended:** `portStepScrub` 0x1200, fill 0, always (offline too), and new baselines once. For online it is
  needed in any case: the drawing differs between machines.
- **Checked:** `tools/check_rollback.py` check 1 (the step runners): `portStepBody` calls `portStepStackScrub` before anything
  else; it is a frameless `.port_text` leaf; in LinkMips with sp 0x801FF000 it writes exactly the 16 words below sp
  for 64 bytes, and 0x8000 bytes for 0x9000. Not tested in game.

### 23.4 Outcome (2026-10-30, the port's PR #423): a false alarm

- The port's "request 82" build had been synced from this folder while request 85 was half done, so it held part of
  85; `sixteen`'s new start places were 85's (SIXTEEN_CARS.md 5.5). Built from 7d0d52c alone, request 82 gives the
  old baselines exactly.
- With 86 in: the fill off, fill 0 and fill 0xA5A5A5A5 give the same `sixteen` frame for frame, and the drawing
  settings (draw distance Farthest, the C emitter) change nothing. No stack read reaches the battle in these
  scenarios: 23.2's candidates stay unproven, none shown real.
- The port keeps `portStepScrub` 0x1200, fill 0, as a guard for online play, and re-recorded the 12 baselines with
  more cars than start points (85's starts); all 29 scenarios pass.

## 24. Request 88 item 1 (2026-10-30): the scrub and the missing sounds

Port request 88 item 1 (the user's playtest of 2026-10-30): with `portStepScrub` 0x1200, fill 0, some sounds are keyed
off a few VBlanks after their key-on (Warthog's special, `soundPlay(0x29, 0, 0, 99)`, sample B620: keyed off 4
VBlanks after its key-on every time; Mr. Grimm's death scream and others missing). With the scrub off they play to the
end. The port's reading: a step reads a stack word it has not written on the sound path. Asked: find it.

### 24.1 No step reads unwritten stack on the sound path (CONFIRMED)

- **Static** (scratchpad `r88/sndroots2.py`, `r88/fpscan.py` on `work/pv/r87_port_P4`): every function reached
  from the 259 sound, libsnd and libspu functions, the ones only called through a vtable included (`sndObjUpdate`,
  the task methods, `soundMgrPlay`): 343 functions. A load from the function's own frame that a path from the entry
  reaches with no store of that slot: none on the play, request, instance or driver path. The only hits are
  out-parameters filled by the callee (`SsGetMVol`'s, and the sequencer's volume fades `_SsSndCrescendo` /
  `_SsSndDecrescendo` through `SpuVmGetSeqVol`: music volume, and libsnd's, not a step's). Only 2 functions of the
  link keep a frame pointer, and neither reads unwritten stack through it.
- **Dynamic:** retail's code and data from a RAM dump taken in a battle, run in a MIPS interpreter: `soundPlay(0x29, 0, 0, 99)` and then
  4 steps of `soundUpdate`, with sp at 0x801FE700 (the dump's sp at the frame boundary is 0x801FE7F0), every load
  below that sp the run had not written logged, pointers into the stack included. **None.** (The three loads it
  flagged are libsnd's statics at 0x801FA448 `_SsMarkCallback` and 0x801FAD38 / 0x801FAD40 `_svm_okof1/2`, the end of
  retail's BSS, inside the range the run watched: not stack.) The sound's two tones were keyed on (voices 0 and 1) and
  stayed on over the next 3 driver updates while their envelope read non-zero.
- `soundManagerObject` and the CD player object are `main`'s locals (`shell_main.c`): on the stack, but in `main`'s
  frame, above every step frame; the scrub never reaches them. The variant keeps the request pool in port storage
  (request 33-4).

So there is no read to give a value: what the scrub changes, no sound code reads.

### 24.2 What keys a new sound off 4 VBlanks in (CONFIRMED in code)

- A request's life: `soundPlay` (C++ `command`, `soundMgrPlay`) queues it; that step's `soundUpdate`
  (`taskMgrRunFrame`) starts it: task slot 3 is `sndObjUpdate`, which allocates its instances (`sched` 1). **It runs
  once per request**: the task's frame method (slot 5) is `sndObjIsFinished` (voices == 0). The next step's
  `soundDriverUpdate` keys the instances on (`soundOn`) and clears `sched`.
- From then on, at every step (3 VBlanks apart; 4 at the first with the pacing), the driver tests each instance that
  nothing scheduled again: `SpuGetKeyStatus(voice) == 0` -> **keyed off and freed** (`soundInstKeyOff` at 80161394).
- libsnd never sets `_spu_keystat` (only `SpuInit` clears it), so the status is 0 exactly when the voice's
  **envelope register (+0xC, ENVX) reads 0**. On a PS1, a few VBlanks after a key-on it is non-zero unless the sample
  has ended.
- So "keyed off 4 VBlanks after its key-on" is this test, at the first step after the key-on, reading the envelope as
  0. The driver's other key-offs, by call site (retail; `ra` = site + 8): 80161394 the envelope 0 (this one); 801613E4
  a timed sound's timer (types 4 and 9, not Warthog's 12); 80161470 the loop list full (29); 80161528 more instances
  than voices (the lowest priorities); 801615D0 loop candidates past the free voices; and `soundOffOn` (mode 0, a
  retrigger; type 12 is mode 1).

### 24.3 For the port

- **The envelope:** the port's SPU writes it as it mixes, on the audio thread (`psyz-01-spu-voice-end.patch`, and
  `vplus/fixes.c`'s note in `low_explosion`: "moves only as audio is mixed"). If the audio thread has not mixed the
  voice since its key-on when the step after it runs, the envelope still reads 0 and the game takes the sound as
  ended. Nothing on the decomp side reads the scrubbed words, so the scrub can only change when the step runs, not
  what it computes. Please log, with the scrub on and off: at each key-off, the caller (`ra` 8016139C = the envelope
  test), and at `SpuGetKeyStatus` (801622D8) the voice's register 0x1F801C0C + 16 x voice (0x1F802C0C + 16 x
  (voice - 24) for 24..47) and the VBlanks since its key-on. If it is 8016139C with 0 there: the fix is the SPU's (a
  voice keyed on reports its attack's level from the key-on, in emulated time, not when it is next mixed). If it is
  another site, tell me which and I will look at it.
- **The stack's room in the variant:** BSS ends at 0x801FDE98 in the port variant (retail 0x801FAD48; the port's
  arrays moved it 0x3150 up), and its last objects are libsnd's (`_SsMarkCallback`, the sequence table, `_svm_okof1/2`
  at 0x801FDE88 / 0x801FDE90). crt0 puts sp at 0x80200000: 8.5 KB of stack instead of retail's 21 KB. In the game's
  own flow `portStepBody`'s frame is about 0x801FF7D0 (`main` 0x6D8, then the screen, `sessionRun`, `rtMainLoop`,
  `portRollbackLoopStep`), so the scrub's lowest word is about 0x801FE5D0, 0x700 above BSS. Code that runs the step
  deeper (each `guest_call` starts 0x400 lower: the co-op duel's `sessionRun`) leaves less. Zeroing libsnd's
  `_svm_okof` would cancel key-offs, not add them, so it is not this sound fault, but keep the step above 0x801FDE98
  + 0x1200.
- The scrub itself (23.3) is unchanged; turn it back on when 24.3's log shows the cause.

**Corrected by request 89 (section 25):** the cause was the stack, not the envelope. The camera wall
(`portVisualCameraWall`, `portVisual` bit 5) took 0xF00 bytes of frame, and the stack under it reached libsnd's
`_svm_okof1` / `_svm_okof2` and left saved registers there, so `SpuVmFlush` keyed off voices at every flush. 24.1 still
holds (no step reads stack it has not written), and 24.2 is how the driver frees a voice whose key-off came from
there. But "zeroing `_svm_okof` would cancel key-offs, not add them" missed that the stack writes non-zero words over
them, and "0x700 above BSS" measured only the step, not the camera under it. 24.1's run started from a RAM dump and
ran the sound code only, so it could not see a frame of the drawing's.

## 25. Request 89 (2026-10-31): the stack runs into libsnd's BSS

Port request 89 (it follows up request 88 item 1). In the hybrid (`TM2_SOURCE_PORT`, build-20261031-065847-944608),
BSS ends at 0x801FE8E8. libsnd's `_svm_okof1` / `_svm_okof2` are its last words, at 0x801FE8D8 / 0x801FE8E0, so the
stack below 0x80200000 had 5.9 KB. A watch on 0x801FE8D8 in a battle: after `SpuVmFlush` clears it, the camera wall's
functions store there every step (0x4AC, then 0x12B4: saved registers). At the next flush voices 2, 4, 5, 7, 9, 12,
16, 17 and 26..31 are keyed off, so a sound the driver starts on one of them ends 3-4 VBlanks in. Asked: (1) more
stack, (2) the camera wall's stack use, (3) a guard.

### 25.1 More stack: `portStack`, 64 KB of .port_bss (built)

- **Why the room shrank:** the variant's code before the game's data is larger than retail's (the changed functions
  are linked at their compiled size). So the whole of .sdata/.sbss/.bss moves up: by 0x3BA0 in the port's build
  (BSS end 0x801FE8E8), 0x3078 in `r89_port_P4` (0x801FDDC0). Retail's stack had 21 KB down to 0x801FAD48.
- **Built (every variant):**
  - `unsigned int portStack[TM2_PORT_STACK / 4]` (0x10000 bytes, 8-aligned) is the last item of .port_bss, with
    `portStackGuard` (16 bytes) right below it. Both are in `port_variant_arrays.c` and `port_rollback.h`.
  - `build_source_only.py --port-variant` sets crt0's stack word to the end of `portStack`, with 0x80000000 left
    out because crt0 ORs it in. The word is in the table of 2MBYTE.OBJ (linked from the .LIB), at
    `__SN_ENTRY_POINT + 0xB0`; retail's is 0x00200000. The same step sets the EXE header's `s_addr` to that end and
    `s_size` to 0 (`elf_to_psx_exe.convert(stack=...)`; the values are in `reloc_plan.json` `port_variant.stack`).
  - `verify_reloc` puts retail's word and header back before it compares the variant with E0, and fails if they
    are not the stack's end.
  - Example, `r89_port_P4`: `portStack` 0x8095A4F0..0x8096A4F0, `portStackGuard` 0x8095A4E0. Read your own build's
    from `symbols.tsv` (the end = `portStack` + 0x10000).
- **What moves with it:** everything on the stack (`main`'s locals: `soundManagerObject`, the CD player; the
  step's frames; request 86's scrub, which now fills portStack). crt0 still clears only the game's BSS, as before,
  and the port zeroes .port_bss. The heap crt0 passes to `InitHeap` (`top - _stacksize - BSS end`) now has a
  positive size. Retail's was negative, and nothing in the game calls `malloc`.
- **Rollback:** the old stack (0x801Fxxxx up to 0x80200000) was in no region. `portStack` lies inside .port_bss,
  whose `port.bss` range is snapshot and hash yes, so it now has rows of its own, snapshot no and hash no
  (`portStack`, `portStackGuard`). The region tsv resolves them, so a port that copies by `rollback_regions.tsv`
  needs nothing else. A port that copies .port_bss as one block must leave `portStack` out.

### 25.2 The camera wall's stack use (built)

- **Not recursive.** `portVisualCameraWall` walks the tree with a stack of its own: 96 pending nodes of 32 bytes,
  and 2 x 32 spans of 8 bytes. All of it was locals, so its frame was **0xF00** (3,840 bytes). Under it are
  `portCameraWallClip` (0x68), `portCameraWallPlanes` (0x38) and `portCameraWallRot` (0x30). The calls go only one
  way: Clip calls Planes, Planes calls Rot. So the "several frames" were these three, under a frame 0xF00 deep:
  0x1090 below `viewChaseCamera`'s frame. `portCameraWallNear` is a leaf with no frame. The watched stores were the
  three callees' saved registers.
- **Built:** the pending nodes and spans are in `portCameraWallWork` (0xE00 bytes, .port_bss, every variant;
  snapshot and hash no). Each call fills what it reads (a node before it is popped, a span before its count), and
  only one view's camera is computed at a time. The frame is now **0x100**, and the whole chain from
  `viewChaseCamera` is 0x290 deep. The walk and its results are unchanged: the same code on static storage.

### 25.3 The guard (built)

- `portStackGuard[4]` is the 16 bytes right below `portStack`. Only a stack that overflows writes it.
  - After the port zeroes .port_bss it reads 0. Fill it with a pattern of your own before `start` (e.g.
    0xA5A5A5A5) and compare after each step: a different word is an overflow, which then shows in the log instead
    of in the sound.
  - Past the guard lies `portCameraWallWork`, then the port's other arrays.
- **The lowest sp seen:** a static count is only a lower bound. `check_rollback` prints the deepest chain of
  direct calls from `main`: 0xBA0 in `r89_port_P4`, through the sound bank's load. It does not follow calls through
  pointers (the task manager's methods, the interrupts, the port's `guest_call`, each 0x400 lower). For the true
  value, fill `portStack` below crt0's sp with a pattern at boot, and after a match look for the lowest changed
  word. 64 KB leaves room: the old stack failed at about 0x1720 below 0x80200000.

### 25.4 Checked

- `tools/check_rollback.py` check 1 (`stack_check`):
  - `portStack` with the guard right below it, and its end 8-aligned;
  - the EXE's crt0 word and `s_addr` hold that end;
  - `portVisualCameraWall` uses `portCameraWallWork`, and its frame is at most 0x200;
  - `main`'s deepest direct chain is at most half the stack.
  - Negative control: request 88's link (`r88_port_P4`) fails, because the symbols are missing.
- `verify_reloc` passes on the variants (the stack word and the header are undone).
- Check 8 resolves the new rows.
- `tools/check_custom.py` check 9 (the camera wall run in the interpreter) now also allows writes to
  `portCameraWallWork`. The eyes it computes are unchanged (worst 1.0 unit; DISH: 4 brought in, 2 kept).
- check_all: 36 of 36 checks on build-20261031-180148-882425 and the `r89_*` links.
- **Not run in game.** For the port: put the scrub back on (`portStepScrub` 0x1200) and test Warthog's special and
  Mr. Grimm's scream.

## 26. Request 106 (2026-11-05): the node list changed when a pickup was taken

### 26.1 The cause (CONFIRMED in the code and the disc's files)

`portDmdMutableList` walks every model file of the match (15.1). It reads each node's child count to find its
children, and for two node types that count is also what the game changes to show or hide a pickup:

- `pickupSetVisible` (8012493C) writes the count of a pickup's holder node: a group (type 1, +0x14) through
  `pickupModelSetVisible`, an offset node (type 4, +0x12) through `pickupSpriteSetVisible`. `pickupApply` and
  `targetDamage` write 0 when a pickup is taken or shot; `pickupRespawnTick` writes 1 when it comes back. `worldLoadObject` also hides
  pickup 418 at the load in mode 4 (count 0).
- A taken pickup's count is 0, so the walk missed its model's nodes. The host's list, made at the match's start,
  had every pickup; the joiner's, made about 850 steps later from the host's state, missed the taken ones. That
  explains the port's four ranges: same bytes, listed on one machine only.

The holders are the nodes of object id 401..430 (u16 at +0x02, `pickupRegisterObject`'s range) of type 1 or 4.
In the disc's 29 DMD files there are 440 (48 groups, 392 offset nodes), every one with count 1.

No other code in the step writes a count the walk reads:
- `carChoiceRegisterGroup` and the tire and damage switches write a select's chosen child (+0x07), not its count
  (+0x06);
- the port's own count writes are on its own nodes (the car select, the hidden areas' groups), outside the files;
- the hidden areas' patches (`portHiddenAreasBegin` / `End`) are undone inside `renderScene`.

### 26.2 Built: the walk takes at least one child of a holder

In `portDmdWalk`, a holder whose count is 0 is walked as count 1, which is its count in the file (`DMD_KIDS`). The
list again depends on the loaded files alone, whatever pickups are taken: it is the list made at the match's start.
Pickup 418 in mode 4 is now listed too, on every machine, so it is a superset (restoring and hashing unchanged
bytes costs nothing).

**For the port:** the list can be made once after each load again (15.1); the rebuild at every 8th step is no
longer needed. It still costs nothing to keep while testing.

**Checked:** `tools/check_rollback.py` check 1, the request 106 run. `portDmdMutableList` runs in check_custom's
LinkMips on PARIS, HKONG and DISH from the disc. Its list equals a Python model of the walk exactly, and it stays
the same with every pickup holder's count set to 0. Not tested in game.
