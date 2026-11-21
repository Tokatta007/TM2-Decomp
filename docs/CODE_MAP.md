# Code map: where things are in the C

A navigation guide for the PC port team, who read this C to patch or hook the
game. It says where each subsystem lives, which functions matter most, and
which globals and structs hold its state. Addresses are retail
(`SCUS_94306R.EXE`). Every function named here is C in `src/semantic`.

Built from `config/semantic/selection-template.json` (which function is in
which file), the evidence column of `config/US/names.tsv`, the file header
comments and the other docs. Areas that are not understood yet are marked
**unclear**. Nothing here was run.

## 1. How to find things

### The name files

| File | What it holds |
|---|---|
| `config/US/names.tsv` | Every readable name given so far (2,100+ rows): `va`, `old` (the IDA or address name), `new` (the name the C uses), `evidence` (where the purpose was confirmed). Functions and globals both. |
| `config/semantic/selection-template.json` | `functions[VA]` gives the symbol and the source file of each of the 1,948 functions. `units[file]` lists what each file exports. This is the authority for "which file is VA X in". |
| `config/US/ida_names.tsv`, `ida_funcs.tsv` | The original IDA names and function sizes. Never edited. Names IDA already had (`rtMainLoop`, `viewProc`, `UpdateGuns`, `uaInit`, ...) are used as is and have no `names.tsv` row. |
| `config/US/file_renames.tsv` | Source files renamed on 2026-10-06: `old_path`, `new_path`, `reason`. The old names are listed again at the end of this file. |
| `config/US/extra_funcs.tsv` | Functions IDA missed. They live in `game_misc_accessors.c`, `engine_misc_accessors.c` and `sdk_extra.c`. |
| `config/US/sdk_modules.tsv` | Which PsyQ library module covers which retail range. |
| `include/semantic/cars.h` | Car record layouts (see the cars section). |
| `include/semantic/weapons.h` | Projectile record, ricochet shots, pooled scene objects. |
| `include/semantic/rt_buffers.h` | The frame record (OT, packet cursor, DRAWENV, DISPENV). |
| `include/semantic/fd44.h` | State of the Dark Tooth boss controller (`ai_dark_tooth_boss.c`). |

### In the C

- Most functions carry their address in the section name:
  `__attribute__((section(".text_8011BE10")))`. Searching for
  `.text_8011BE10` finds the definition. C++ files use `#pragma tm2_symbol`
  instead for some symbols.
- Globals placed at a fixed address use `.tm2_XXXXXXXX` sections. Strings use
  `.tm2_string_XXXXXXXX`.
- Files ending in `_state.c`, `_tables.c`, `_records.c`, plus
  `carved_initializers.c` and `referenced_strings.c`, hold only data.
- `reference/` holds readable C for the hand-written assembly that the build
  keeps as asm: `model_emitter_800CDC54.c`, `renderer_walker_engine_render_scene.c`,
  `near_pass_engine_math_near_pass.c`, `hdp_walker_engine_hit_detection.c`. The matching build does not
  link them; the all-C flavour and the port variant's C emitter do (`SOURCE_BUILD.md`).
- The README progress table lists the files that still hold a "close" (not
  byte-identical) function. Those are `font_init.c`, `screen_pause_menu.c`,
  `car_crash.c`, `car_wheel_sway.c`, `ai_route_setup.c`, `car_destroyed.c`,
  `screen_card_draw.c`, `engine_fileio_debris.c`, `engine_anim_hit_volumes.c`, `engine_sound_object_update.c`,
  `packet_digits.c`, `projectile_hit.c`, `shell_main.c` and some SDK units.

### Old names in the port's notes

The port's notes use three kinds of old name. All of them contain or map to
an address.

| Old name | How to find the new one |
|---|---|
| `sub_8011BE10`, `func_8011BE10` (function) | The VA is in the name. Look up `8011BE10` in `names.tsv` (→ `carDestroyed`). If there is no row, the template's `functions["8011BE10"].symbol` is the name in the C. |
| `D_8016475C` (global) | Look up `8016475C` in `names.tsv` (→ `sessionSettings`). If there is no row, the global still has its `D_` name. |
| IDA names (`update_bullets`, `RicochetOffObject`, `GetNextTriggerPoint`) | Search the `old` column of `names.tsv` (→ `bulletsUpdate`, `carRicochetOffObject`, `worldCollapseUpdate`). Or find the VA in `ida_names.tsv` first. |

From a shell in the repository root:

```sh
grep -i 8011BE10 config/US/names.tsv
grep -P "\tupdate_bullets\t" config/US/names.tsv
```

```powershell
Select-String -Path config\US\names.tsv -Pattern '8011BE10'
```

From Python, `tools/symbol_names.py` reads the file. It has no command
line; import it:

```python
import sys; sys.path.insert(0, 'tools')
from symbol_names import load_renames, renamed_addresses
by_va  = {r['va']: r['new'] for r in load_renames()}   # 0x8011BE10 -> 'carDestroyed'
by_new = renamed_addresses()                             # 'carDestroyed' -> 0x8011BE10
```

`tools/rename_symbol.py` gives new names. It also edits the docs: the old
name stays and the new one is added once in brackets, for example
`` `sub_8011BE10` (carDestroyed) ``. So searching the docs for an old name still
works.

Still unnamed: 127 of the 1,084 functions below `80138000` keep a `sub_` or
`func_` name. Most PsyQ SDK functions keep `sub_` names; their real names are
in the header comment of their unit and in `docs/SDK_SOURCES.md`.

### Some names mislead

| Name in the C | What it really is |
|---|---|
| `sessionRun` (8012BE5C) | The session runner. IDA called it `screenCheckMidGameOptions`; renamed. |
| `screenSinglePlayerMenu` (8012E908) | The single-player mode menu (Tournament, Challenge, Password). IDA called it `screenVehicleChoice`; renamed. The car select is `screenCarSelect`. |
| `movieSkipCheck` and other `movie*` names in `menu_*.c` | The old `game_movie_*` files were shell menu objects (password keypad, slider bar, code detector), not movies; they are now `menu_*.c`. |
| `sound_manager_reap.c` | Also holds `shellInitGraphicsSystem`, `shellLoadUaCore` and `arenaResetSystems`. |
| `soundInstStop` (801611DC) | IDA called it `shellLeaveLevel`; it stops one sound instance. |
| `pickup_spawn.c` | `weaponSlamStateInit` (Mr. Slam claw state). The file header says "pickup spawn"; the two disagree. **Unclear.** |

### The docs

| Doc | One line |
|---|---|
| `AI_LIVES.md` | Lives for computer cars: respawning one AI car alone. |
| `AI_VS_AI.md` | Computer cars fighting each other: targets, damage, missiles. |
| `CAR_SELECT_CAR.md` | Drawing the 3D car more than once (split-screen car select); the scene system and frame records. |
| `CAR_STATS.md` | Car stat tables, special weapons, Shadow's detonation; field names of `TM2CarStats`. |
| `CODE_MAP.md` | This file. |
| `COOP_LIVES.md` | Lives per player in co-op, respawning one car, spectating, HUD lives readout. |
| `DRAW_DISTANCE.md` | Object culling and far clip: far distance, node culling, capacity caps. |
| `GAME_EVENTS.md` | Kills, specials, pedestrians, air time, falls, speed, wins: every hook point for a kill feed. |
| `MAIN_LOOP.md` | One iteration of `rtMainLoop`, update vs draw, where the camera matrix is set (for 60 fps). |
| `MATCH_START.md` | Filling the settings struct, what the session runner does, detecting a round's end. |
| `MOTION_TARGET_UPDATE_RECONSTRUCTION.md` | How the track vehicle function 80118BBC was reconstructed. |
| `OPTIONS_CONTROLS.md` | What the Options, Audio and controller screens store; pad bindings and layouts. |
| `PICKUP_LOOKS.md` | What a pickup looks like on the map, and how to change it; the pickup slots. |
| `PORT_RENAMES.md` | For the PC port: what was renamed (symbols and files) and how to update notes. |
| `RAND_CALLERS.md` | Every `rand()` caller, classified game state or visual only (data in `rand_callers.tsv`). |
| `RENDER_ANSWERS.md` | Near polygons, screen-position writers, buffers and caps, and 60 fps interpolation (R1..R3). |
| `ANALOG_STEERING.md` | Where steering and throttle input become the turn and acceleration; hooks for analog stick and triggers. |
| `TEAM_COLORS.md` | Car textures and palettes in VRAM, per-car palette copies for team colours, the paint entries per car, the car-select carousel. |
| `TEXTURE_BANKS.md` | Per-car VRAM banks: every car texture/palette upload and its group, identifying a car's primitives at draw time (tpage bank bits), shared images. |
| `WORLD_MARKERS.md` | Drawing a marker at a world point: the effect sprite call, why sprites can be invisible, a column/ring, and adding the port's own primitives with the game camera. |
| `FOUR_PLAYERS.md` | 3-4 players: pad-driven AI records (input block, AI-vs-human rules), two-player limits, quarter views and HUDs. |
| `RULES.md` | Source rules for `src/` (pure C, when `volatile` is allowed). |
| `SANTA.md` | The Antarctica Santa: adding the PC's pedestrian 516 #2 to the PS1 level. |
| `SDK_SOURCES.md` | Which PsyQ version TM2 links and where public C for those libraries exists. |
| `SECRET_LEVELS.md` | Offering Suicide Swamp, Cyburbia and Roof Tops in any match, with bots. |
| `SIGNATURES.md` | Machine-readable function signatures for the port (`tools/export_signatures.py`). |
| `SIXTEEN_CARS.md` | Sixteen cars in a match, and more than 2 MB. |
| `SOURCE_BUILD.md` | The source-only build (no retail bytes pasted over). |
| `TOURNAMENT_FLOW.md` | Between tournament levels, after the last life, the pause menu. |
| `TOURNAMENT_PASSWORDS.md` | Tournament state, the password codec, saves; the settings struct. |
| `UNLOCKS.md` | Car and level unlock codes, secret levels, winner movies, credits. |
| `WIDESCREEN.md` | Projection, view culling, view modes and screen-space overlays. |

The port's request list (`docs/DECOMP_REQUESTS.md`) lives in the port
repository, not here.

## 2. Subsystem map

Each table lists the most useful functions. "File" is the file under
`src/semantic/`.

### 2.1 Boot, main loop and timing

`main` (`sub_8012C8DC`) builds the sound manager and music objects on its
stack, shows the logos and intro, then runs the shell (`shellScreenLoop`).
A match runs inside `rtMainLoop`. The loop runs at 20 fps (3 vsync ticks per
frame). `MAIN_LOOP.md` lists every call of one iteration.

| Function | VA | File | Purpose |
|---|---|---|---|
| `sub_8012C8DC` | 8012C8DC | shell_main.c | The game's `main()`. |
| `shellInitMachine` | 80128B74 | shell_machine.c | One-time bring-up: display, graphics, sound, file I/O. |
| `shellInitGraphicsSystem` | 80128B34 | sound_manager_reap.c | Graphics and pad init. |
| `shellLoadUaCore` | 80128C7C | sound_manager_reap.c | Loads UACORE.VAB, then `lightInit` and `uaInit`. |
| `shellScreenLoop` | 80136F0C | menu_callback_chain.c | Calls each returned screen handler until NULL. |
| `rtMainLoop` | 800D1318 | rt_main.c | One match: update, draw, pace, present; returns the fired scheduled event. |
| `worldUpdate` | 8011A700 | world_update.c | The world step: cars, AI, physics, round end, every world subsystem. |
| `rtScheduleEvent` | 800D181C | rt_main.c | Schedules a value that ends `rtMainLoop` after N frames. |
| `rtUpdateScheduledEvents` | 800D18A0 | rt_main.c | Counts scheduled events down; returns the fired one. |
| `rtDoubleBufferInit` | 800D1A84 | rt_buffer_init.c | Sets up the six frame records. |
| `rtToggleRearView` | 800D1BB8 | rt_main.c | Rear-view mirror on/off. |
| `SetFrameStart` | 80155214 | timer_frame.c | Measures the frame, sets ticks per frame and frame rate. |
| `timerGetFrameTicks` | 8015531C | timer_frame.c | Vsync ticks per frame (3 = 20 fps). Many updates scale by it. |
| `timerGetFrameRate` | 8015532C | timer_frame.c | 60 / ticks. |
| `GetCurTics` | 801551E8 | timer_frame.c | Root-counter time. |
| `SysClkIntHandler` | 801553A8 | timer_frame.c | Timer interrupt; adds to `timerTicsAccum`. |

Globals: `rtFrameTime` (801809E8, 20 or 15 tics), `timerFrameTicks`
(8017EBF8), `timerFrameRate` (8017EBFA), `rtScheduledEvents` (8018116C),
`rtFiredEventValue` (801809D8), `timerTicsAccum` (801FA3B8),
`renderView0Frames` (8018118C, see `rt_buffers.h`).

### 2.2 Session runner, match start, round end

The runner takes the settings struct, loads the arena and cars, then runs
`rtMainLoop` once per round and handles each result. See `MATCH_START.md`,
`TOURNAMENT_FLOW.md`, `COOP_LIVES.md`.

| Function | VA | File | Purpose |
|---|---|---|---|
| `screenCheckMidGameOptions` (sessionRun) | 8012BE5C | session_runner.cpp | The session runner. |
| `sessionLoadArenaAndCars` | 8012BAFC | shell_main.c | Loads arena and car files with "LOADING" shown. |
| `sessionBindCars` | 8012BD94 | screen_card_layout.c | `arenaSetup`, then binds humans and opponents to car slots. |
| `arenaSetup` | 8011995C | arena_setup.c | Per-arena setup; calls the `arenaLoad*` loader of the level. |
| `arenaResetSystems` | 80128CE4 | sound_manager_reap.c | Arena reset: car tables, registries, targets, lights, effects. |
| `roundStartMusic` | 80128DB4 | game_mode_audio.cpp | Calls `roundStart`, then picks the level music. |
| `roundStart` | 8011A024 | scene_init.c | Round start: resets and places cars and world state. |
| `humanCarsRestart` | 8011A198 | arena_setup.c | Restarts each human car. |
| `gameSetMode` | 8011ED88 | misc_one_liners.c | Sets `gameMode` and the attract flag. |
| `roundIsOver` / `roundIsWon` | 8011EDB8 / 8011E214 | global_accessors.c | Round-end flags. |
| `aiCarsCountRemaining` | 8011C248 | scene_init.c | Counts AI cars still in play. |
| `screenLostALife` | 8012940C | car_parts_spin.c | "Lost a life" screen. |
| `screenDeathmatchResult` | 8012B2D4 | screen_deathmatch_result.c | Two-player Deathmatch round result. |
| `screenChallengeResult` | 8012B0EC | shell_main.c | Challenge YOU WIN / YOU LOSE and tally. |
| `screenShowMessageLines` | 8012B5FC | shell_main.c | Centred text lines (CONGRATULATIONS, next level). |
| `pauseMenu` | 80129528 | screen_pause_menu.c | The in-match pause menu. |
| `sessionTeardown` | 80128F3C | misc_one_liners.c | Clean-up after the last round. |
| `shellAttractDemo` | 8012DE7C | shell_attract_demo.c | Attract demo: a complete match started from code. |
| `demoRespawnAiCars` | 8011EFDC | spawn_tables.c | Respawns AI cars for the next demo round. |

Globals: `sessionSettings` (8016475C; layout in `MATCH_START.md` and
`TOURNAMENT_PASSWORDS.md`), `gameMode` (80180FC4), `roundOverFlag`
(80180F94), `roundWonFlag` (80180FB0), `aiCarsRemaining` (80181020),
`carResetWeapons` (80180FA0), `gameAttractDemoMode` (80180FBC),
`attractDemoLevel` (80180D0C).

### 2.3 Settings, shell menus, screens and the scene system

Every shell screen builds a scene on the shared scene object `shellScene`
from stack objects, adds transition rows (`sceneAddAction`), runs it
(`sceneRun`) and returns the next screen handler. Scene objects and commands
are C++ classes (g++ 2.x vtables). See `CAR_SELECT_CAR.md` and
`OPTIONS_CONTROLS.md`.

| Function | VA | File | Purpose |
|---|---|---|---|
| `sceneInit` | 801222F0 | scene_init.c | Scene init: buffers, view, lights, handlers. |
| `sceneRun` | 80122B34 | screen_ending.cpp | Runs a scene frame by frame until done. |
| `sceneAddAction` | 80122574 | object_list_helpers.c | Adds an {event, source, type, command} row. |
| `sceneDispatchEvents` | 80122818 | object_list_consumers.cpp | Matches posted events against the rows. |
| `sceneRunCommands` | 80122790 | object_list_consumers.cpp | Runs the running commands each frame. |
| `sceneObjDraw` | 80122124 | scene_init.c | Draw method of every scene object (3D model). |
| `sceneDeliverResource` | 80121E18 | screen_resource_records.c | Hands loaded resources to matching scene objects. |
| `screenTitle` | 8012DF84 | screen_title_level_select.c | Title screen. |
| `screenVehicleChoice` (screenSinglePlayerMenu) | 8012E908 | screen_single.c | Single-player menu (see "Some names mislead"). |
| `screenMultiPlayer` | 8012F5F8 | screen_controller_multiplayer.c | Multi-player menu. |
| `screenLevelSelect` | 8012FD30 | screen_title_level_select.c | Level select (secret level codes). |
| `screenCarSelect` | 801307EC | screen_car_select.c | Car select (car codes). |
| `screenAiSelect` | 80131EEC | screen_ai_select.c | Opponent select. |
| `screenWorldGlobe` | 801334E8 | screen_controller_multiplayer.c | The globe between tournament levels. |
| `screenOptions` / `screenAudio` | 80133C08 / 80135844 | screen_options_audio.c | Options and Audio screens. |
| `screenControllerLayout` | 80135088 | screen_controller_multiplayer.c | Controller layout screen. |
| `screenHistory` | 8013629C | screen_pause_menu.c | History screen. |
| `settingsInitDefaults` | 8012A940 | arena_load_register.c | Default settings at boot. |
| `settingsSet*` / `settingsGet*` | 801377AC.. | menu_state_accessors.c | Settings fields (mode, humans, cars, level, difficulty, layout). |

Globals: `shellScene` (8016303C), `sessionSettings` (8016475C), the menu
object vtables (800D026C..800D036C), `padLayout` (80180BA4).
Menu object classes are in `menu_object_*.c`, `menu_*.c` and `game_movie_*.c`.
Scene commands (move, turn, show, hide, cursor, choice step, exit) are in
`motion_commands.c`, `selection_commands.cpp`, `object_control_helpers.cpp`
and `input_commands.cpp`. **Unclear:** about 10 command constructors in
`object_control_helpers.cpp` are still `sub_`.

### 2.4 Tournament, passwords and unlocks

See `TOURNAMENT_FLOW.md`, `TOURNAMENT_PASSWORDS.md`, `UNLOCKS.md`.

| Function | VA | File | Purpose |
|---|---|---|---|
| `tournamentPickOpponents` | 8012CD08 | tournament_pick_opponents.c | Random opponents (or Dark Tooth) into the settings. |
| `screenPickOpponents` | 801334C0 | screen_pick_opponents.c | Picks opponents, returns the globe screen. |
| `tournamentOpponentCount` | 8012AFE8 | tournament_tables.c | Opponents per level. |
| `carIndexToType` | 8012B044 | tournament_tables.c | Car index to car type. |
| `passwordEncode` / `passwordDecode` | 8012CE04 / 8012CE7C | password_codec.c | The password codec. |
| `passwordRebase` | 8012CF94 | password_rebase.c | Digit base conversion for passwords. |
| `passwordBuildString` | 801292A4 | scene_init.c | Builds the password text. |
| `menuPasswordCursorRun` | 8012D3F0 | car_destroyed.c | Password keypad cursor. |
| `menuKeypadGetValue` | 80137690 | menu_state_copy.cpp | Six keypad digits to an integer. |
| `carIsSelectable` | 8012CC10 | car_is_selectable.c | Car carousel predicate (unlocks). |
| `shellSetUnlockFlag` | 80137744 | menu_state_accessors.c | Sets the Sweet Tooth / Minion flag on a code. |
| `menuCodeDetectorRun` | 8012DCA4 | menu_text_callbacks.c | Runs the code detector each frame. |
| `codeDetectorGetLastCode` | 80137490 | game_settings_extra.c | Last code entered. |
| `screenEnding` | 80136D98 | screen_ending_setup.c | Ending movie player. |
| `screenCredits` | 80136A3C | screen_credits.c | Credits. |

Globals: `unlockSweetTooth` (80180D04), `unlockMinion` (80180D05),
`carSelectCodeTable` (800CFF14), `secretLevelCodeTable` (800CFE70),
`endingMovieTable` (801648C4). **Unclear:** the code detector constructor
and matcher (`sub_8012DB10`, `sub_8012DB94`) are not named yet.

### 2.5 Input, pad and controls

The BIOS pad buffers are decoded into one word per frame (P1 low half, P2
high half). Game code asks for actions, not buttons: an action maps to a
button through a binding table that the controller layout fills. See
`OPTIONS_CONTROLS.md`.

| Function | VA | File | Purpose |
|---|---|---|---|
| `padInit` | 80150B40 | controls.c | Starts the pad driver and loads default bindings. |
| `padStart` | 8014A2A4 | engine_clut_anim.c | `InitPAD2` / `StartPAD2` on the BIOS buffers. |
| `padUpdate` | 80150E00 | controls.c | Per-frame decode and latch with key repeat. |
| `padDecode` | 8014A31C | shared_pad.c | BIOS buffers to the decoded word. |
| `padLatchWithRepeat` | 80151138 | controls.c | Latches presses with repeat delay and interval. |
| `padCheckAction` | 80150CFC | controls.c | Main action query (menus, car input). |
| `padActionHeld` | 801510DC | controls.c | Is the button bound to an action held. |
| `padButtonHeld` | 8014A57C | shared_pad.c | Raw button test (buttons 1..14). |
| `padComboHeld` | 8014A6A8 | shared_pad.c | Exact button combinations (special moves). |
| `padBindAction` / `padActivateBindings` | 80150F70 / 80150FC0 | controls.c | Write configured bindings; copy them to active. |
| `padLoadLayout` / `padSetLayout` | 80120664 / 801206F4 | spawn_tables.c | Controller layout 0..3 per port. |
| `humanCarReadInput` | 800DE5F8 | car_input.c | Pad actions to a human car's input flags. |
| `carReadMoveCodes` | 800DECA8 | car_input.c | Direction codes: special moves and cheat codes. |
| `PadSetConfig` / `carApplyPadConfig` | 8011D600 / 8011D6A4 | car_records.c | Per-player pad configuration 0..3. |
| `padAnalogAxis` | 8014AA24 | engine_misc_accessors.c | Pad 1 analog axis. **Unclear** who uses it. |

Globals: `padButtonsLatched` (80180D34), `padButtonsDecoded` (80180D38),
`padActiveBindings` (801A4BEC), `padConfiguredBindings` (801A4CEC),
`padDefaultBindings` (8017EBCC), `padLayoutTable` (80162F64), `padLayout`
(80180BA4), `padConfigs` (80180FFC), `padRecvBufferPort1/2` (801B2D80,
801B2DA4). Struct: `TM2CarInput` in `cars.h`.

### 2.6 Cars: records

Two human records and nine AI records sit back to back in RAM. The layouts
differ but hold the same blocks at different offsets (table in `cars.h`).
Whether a car is driven by a pad or the AI depends only on its array.

| Function | VA | File | Purpose |
|---|---|---|---|
| `humanCarGet` / `aiCarGet` | 8011CEE8 / 8011D0EC | car_records.c | Record by index. |
| `humanCarCount` / `aiCarCount` | 8011CEDC / 8011D0E0 | car_records.c | Loop bounds. |
| `carFromSlot` | 8011CFD0 | car_records.c | Car object slot to human/AI record. |
| `carFindById` / `carObjectId` | 8011D2C8 / 8011D280 | car_records.c | Object id ↔ (human?, index). |
| `humanCarObject` / `aiCarObject` | 8011CB50 / 8011CB70 | car_object_lookup.c | The car's world object. |
| `carBindHumanSlot` / `carBindAiSlot` | 8011A3FC / 8011A5CC | scene_init.c / car_parts_spin.c | Bind a record to a registered car model. |
| `carRegisterModel` | 8011961C | ai_route_helpers.c | Registers a car model from the level file. |
| `carTablesClear` | 80119848 | ua_init.c | Clears the car tables. |
| `carStatsLoad` | 800FE15C | car_stats.c | Loads a car type's stats (one `carStats<Car>` per car). |
| `carStatsDerive` | 800FF878 | hud_radar.c | Runtime speeds and turning from base stats and fps. |
| `weaponBlockInit` | 800FDCAC | ai_route_setup.c | Weapon block: special recharge and capacity per car. |
| `carTypeToIndex` / `carTypeFromIndex` | 8011E1C4 / 8011E1F8 | misc_one_liners.c / game_misc_accessors.c | Car type id ↔ index. |
| `carDisplayName` / `carFileCode` | 8012AEC4 / 8012ADA0 | car_parts_spin.c | Car name; two-letter file code. |

Globals: `humanCars` (80187B10, 2 × 0x814), `aiCars` (80188B5C, 9 × 0x854),
`humanCarTotal` (80181014), `aiCarTotal` (8018101C), `humanCarObjects`
(80180FEC), `aiCarObjects` (80188B38), `carSlotObjects` / `carSlotKinds` /
`carSlotIndices` / `carSlotOwners` (80187AD4 / 80187A20 / 80187A5C /
80187A98), `humanCarTypes` (80181004), `carTypeIdTable` (80162F28).

Structs (`include/semantic/cars.h`): `TM2HumanRecord` and `TM2AIRecord`
(whole records), built from `TM2CarState`, `TM2CarInput`, `TM2CarAim`,
`TM2CarStatsBlock` (hp at +0xA4), `TM2CarWheels`, `TM2CarMotion`,
`TM2CarWeapons`. `TM2Object` is a world object (matrix +0x80, rot +0xA0,
pos +0xA8, id +0x118). Many fields are still `fXX` or padding.

### 2.7 Cars: physics, motion, wheels, contacts

| Function | VA | File | Purpose |
|---|---|---|---|
| `humanCarUpdate` / `aiCarUpdate` | 8011C400 / 8011C4A0 | car_records.c | Per-car update from `worldUpdate`. |
| `humanCarTick` | 800DC88C | car_tick.c | Per-frame physics and state tick of a human car. |
| `carThrottleSteer` | 800DD304 | car_control.c | Throttle, steering, boost, brake (human and AI). |
| `carsPhysicsUpdate` | 8011E220 | car_physics.c | Per-frame physics over all cars (ground probe, contacts). |
| `carMoveByVelocity` | 800DF594 | car_motion.c | Moves the car by its velocity. |
| `CarRotUpdate` | 800DF3EC | car_motion.c | Car rotation update. |
| `carGripSlowdown` / `carSlideDrag` | 800DFB28 / 800E0208 | car_motion.c | Grip and slide. |
| `carWheelsUpdate` | 800EC994 | car_wheels.c | Wheel contact step (probe, surfaces, moving surfaces). |
| `worldGroundHeightAt` | 800EF340 | car_wheels.c | Ground height under a point. |
| `worldSectionAt` | 800EFF34 | car_wheel_sway.c | Track section containing a point. |
| `carSuspensionUpdate` | 800F15D0 | screen_pause_menu.c | Wheel suspension, body pitch/roll, landing rumble. |
| `carBodyAttitudeUpdate` | 800F0944 | car_rollover.c | Body roll/pitch from wheel heights; starts rolls. |
| `carBarrelRollStep` / `carTumbleStep` | 800F2778 / 800F2DF4 | car_tumble.c | Barrel roll and tumble. |
| `carUpdateContacts` | 800EF5D0 | car_crash.c | Places the four corner contact points. |
| `carContactResolve` | 800E0C28 | car_contact_resolve.c | One contact: wall bounce, car handoff, hazards. |
| `carContactsCombine` / `carContactRespond` | 800E2DD0 / 800E3800 | car_contacts.c | Merges contacts; ricochet, damage, push out. |
| `carCarCollision` | 800E184C | car_collision.c | Car-to-car collision response. |
| `carPairsUpdate` | 8011E7D8 | screen_deathmatch_result.c | Distance and heading for every car pair. |
| `hdpCollideObject` | 8014E66C | engine_hit_detection.c | Hit-detection pass for one object ("hdp", engine code). |
| `hdpQueryPoint` | 80150A74 | engine_anim_hit_volumes.c | Point query against an object's hit tree. |

Globals: `worldSurfaces` (801B0418), `worldSections` (801A9854) and the
section tables after it, `carPairDistances` (8018F0D4), `hdpState`
(80180D68). Struct: `TM2CarMotion`, `TM2CarWheels` in `cars.h`.
`hdpWalk` and its leaf tests are hand-written asm (C copy in
`reference/hdp_walker_engine_hit_detection.c`). Also called from `humanCarTick`: `carSpinOutUpdate` (800E6B78) and
`carParisSinkZonesUpdate` (800F0450).

### 2.8 Cars: damage, death and respawn

See `GAME_EVENTS.md`, `COOP_LIVES.md`.

| Function | VA | File | Purpose |
|---|---|---|---|
| `carApplyDamage` | 800E5628 | car_damage.c | Applies damage to a car. |
| `areaDamage` | 8010B5C4 | area_damage.c | Explosion area damage. |
| `carKnockBack` | 800E5F24 | car_damage.c | Knock-back along a direction. |
| `carSpinOut` | 800E5FFC | car_spin.c | Spin-out / hit reaction by kind. |
| `carCrashStart` | 800E6D14 | car_crash.c | Crash between two cars. |
| `carCrushContactUpdate` | 800F39D0 | proximity_update.c | Car on top drains the car below. |
| `humanCarDamageUpdate` | 8011B89C | car_damage_human.c | Human car health, death, lives. |
| `aiCarDamageUpdate` | 8011BBD4 | world_collapse_warps.c | AI counterpart. |
| `carDestroyed` | 8011BE10 | car_destroyed.c | Destroyed: drop pickup, explode or sink, respawn. |
| `carWreckUpdate` | 800E75F4 | car_crash.c | Wreck smoke, sparks, timer, blow-up. |
| `carRestart` | 800FD550 | car_restart.c | Restarts a car (weapon flags may skip the reset). |
| `humanCarPlaceAtStart` | 800FD6C4 | car_object_contact.c | Places a human car on a random start node. |
| `aiCarStart` / `aiCarReset` | 800FFEFC / 80100034 | ai_car_start.c / ai_route_setup.c | (Re)starts an AI car. |
| `startNodePickRandom` | 800F636C | ai_start_nodes.c | Random unused start node. |
| `worldFallLimitZ` / `worldFallDeathZ` | 8011EC0C / 8011ECC4 | world_fall_limit.c / misc_one_liners.c | Falling off the map. |
| `carSetDamageStage` | 80124718 | car_switch_registry.c | Damage-stage model switch. |
| `debrisBreakModel` | 8014B504 | engine_clut_anim.c | Breaks a car model into flying chunks. |
| `carStatsDarkToothStage2` | 800FF5BC | arena_setup.c | Dark Tooth second stage when its HP reaches 0. |

### 2.9 Cars: AI driver and routes

AI cars drive along per-level route graphs. Routes and start nodes load with
the arena. `worldUpdate` also runs an "AI director" that ranks the AI cars
and hands out attack, hunt and flee states.

| Function | VA | File | Purpose |
|---|---|---|---|
| `aiCarDrive` | 800E7EDC | ai_driver.c | Per-frame AI step (range, goal, mood, movement). |
| `aiUpdateMood` | 800E8A84 | ai_steps.c | Mood state machine (idle, hunt, attack, flee). |
| `aiMovementUpdate` | 800E8F38 | ai_steps.c | Timers, route or free steering, weapons. |
| `aiReplanRoute` | 800E9578 | ai_steps.c | Plans a new route. |
| `aiFollowRoute` | 800EAB50 | ai_steps.c | Follows the route. |
| `aiFreeSteer` / `aiSteerToGoal` | 800EA270 / 800EB714 | ai_steps.c | Steering off-route; ram or pass. |
| `aiChooseNextNode` | 800F763C | ai_route_nodes.c | Next route node. |
| `aiAdvanceRouteNode` | 800F68D8 | ai_route_nodes.c | Node reached: speed, lane, segment. |
| `aiPlaceOnRouteNode` | 800F6518 | ai_route_setup.c | (Re)places an AI car on its node. |
| `aiSetSegmentSpeed` | 800F6F6C | car_tumble.c | Target and cornering speed. |
| `aiCarSetupProfile` | 800D3C7C | ai_route_nodes.c | Behaviour tables by car type, level and difficulty. |
| `aiSetDifficulty` / `aiApplyDifficulty` | 8011D5D0 / 8011D528 | car_records.c | Difficulty (75/120/140 % speed). |
| `aiGetTargetCar` | 8011CF0C | car_records.c | The car an AI car is after. |
| `aiWeaponsUpdate` / `aiWeaponFireDecide` | 800D3BB8 / 80103CA0 | weapon_helpers.c / weapon_tick_init.c | AI weapon choice and firing. |
| `aiDarkToothBossUpdate` | 8010FD44 | ai_dark_tooth_boss.c | Dark Tooth boss on level 8. |
| `routeLoad<Track>` | 80101B58.. | ai_route_loaders.c | Loads `TERRAIN\<track>.PTS` and start nodes. |

Globals: `aiRouteNodes` (801B1DE8), `aiRouteNodeCount` (80180AD8),
`aiRouteTransitionTables` (801A9B9C), `aiRouteSpeedLimitTable` (801AAAC9),
`aiDifficulty` (80180FB8), `aiCarTargets` (801879FC), per-car weapon weights
`ai<Car>Weights9/10` (80162608..80162764), `startNodeList` (801B2B28).
Struct: `TM2AIRecord` in `cars.h` (its +0x0D8 contact area and most route
fields are unnamed). **Unclear:** `sub_800E9E20`. See also `aiInputFinalize` (800EC180).

### 2.10 Weapons, specials, projectiles, pickups

See `GAME_EVENTS.md` and `CAR_STATS.md` (specials per car).

| Function | VA | File | Purpose |
|---|---|---|---|
| `weaponTick` | 8010388C | weapon_tick.c | Per-frame weapon block tick (recharge, fire, guns). |
| `weaponFireSelected` | 80103FB8 | weapon_select.c | Fires the selected weapon. |
| `weaponFireSpecial` | 80105A7C | weapon_basic.c | Special weapon, by car type. |
| `weaponFireEnergyAttack` | 801046FC | weapon_special.c | Energy attacks 100..106 (freeze, mines, napalm, shield, ...). |
| `weaponLaunchProjectile` | 801052BC | weapon_launch.c | Launches pickup projectiles. |
| `UpdateGuns` | 80104F70 | update_guns.c | Machine guns. |
| `weaponProjectileUpdate` | 800DB110 | projectile_hit.c | One projectile: steer, move, hit, expire. |
| `projectileHitTest` | 800D6FD8 | weapon_slam_swing.c | Projectile vs cars, breakables, spinners. |
| `weaponApplyHit` | 800DC65C | weapon_missile_trails.c | Applies a hit by type. |
| `bulletSpawn` / `bulletsMove` | 800D7A4C / 800D7E30 | weapon_bullets.c | Machine-gun bullets. |
| `weaponSpawnProjectile` | 800D9F40 | weapon_fire_specials.c | Base projectile spawn; `weaponFire*` wrap it per type. |
| `weaponDropItem` / `weaponDropsUpdate` | 80109518 / 80109D58 | weapon_helpers.c / fx_sprite_spawn.c | Mines and remote bombs. |
| `weaponRicochetUpdate` | 8010CC54 | weapon_ricochet.c | Ricochet shots. |
| `weaponFlameUpdate`, `groundFiresUpdate` | 8010A7A0, 8010C160 | weapon_flame_update.c, fx_ground_fires.c | Thumper flame, napalm ground fires. |
| `weaponTornadoUpdate` | 8010E040 | weapon_tornado.c | Twister tornado. |
| `weaponShadowStrikeUpdate` | 8010F9C0 | strike_update.c | Shadow ground missile. |
| `weaponSlamGrabUpdate` / `weaponSlamSwingUpdate` | 800F4CC0 / 800F4058 | weapon_arcs_draw.c / weapon_slam_swing.c | Mr. Slam claw. |
| `weaponTaserTarget` / `weaponLightningScan` | 801075C8 / 80107B00 | weapon_slot_target.c / weapon_slot_scan.c | Taser and lightning arcs. |
| `weaponsRoundReset` | 80108CF0 | round_reset.c | Weapon state reset per round. |
| `weaponAssetBind` | 80109038 | asset_pools.c | Binds loaded weapon models to pools. |
| `pickupApply` | 8010AFC8 | pickup_apply.c | Collecting a pickup. |
| `pickupRegister` / `pickupRespawnTick` | 8010AE58 / 8010AED0 | ai_route_helpers.c / fx_sprite_spawn.c | Map pickup slots and respawn. |
| `carContactApplyObject` | 800E06CC | car_object_contact.c | Contact with a pickup or hazard object. |

Globals: `fxRecords` (801A5B08, 16 × 980-byte projectile records,
`TM2Projectile` in `weapons.h`), `bulletRecords` (80181498, 40),
`weaponDrops` (80184B04), `groundFires` (801842C4), `weaponArcs` (801847B4),
`pickupSlots` (80183F7C), `pickupAmmoAmounts` (801842A0),
`weaponFireInterval` (80180F44). Struct: `TM2CarWeapons` in `cars.h`.

### 2.11 World objects, level events, arena loaders

Level files (.DMD) hold typed objects. `worldLoadObject` dispatches each one
by type id into registries; level scripts then drive them.

| Function | VA | File | Purpose |
|---|---|---|---|
| `assetInstallImage` | 80120EF4 | asset_header.c | Validates and relocates a loaded asset image, registers its objects. |
| `worldLoadObject` | 800D1DB0 | world_load_object.c | DMD object loader: one case per object type id. |
| `worldLoadStartObjects` | 800D1C70 | rt_main.c | Walks the start block at 80019C40. |
| `objRegistryAdd` / `objRegistrySetStep` | 80123F78 / 80124384 | object_registry.c | Per-id object registry; animation step. |
| `objRegistriesReset` | 80124A00 | registry_reset.c | Clears all registries. |
| `arenaLoad<Level>` | 80119D08.. | arena_load_register.c, view_watch_ai.c | Terrain loaders per arena. |
| `terrainLoadRmp<Track>` / `terrainLoadGrp<Track>` | 80100488.. | terrain_load.c, terrain_load_presets.c | Surfaces (.RMP) and sections (.GRP). |
| `startNodes<Track>` | 80101AB4.. | ai_start_nodes.c | Start-node lists. |
| `laEventsUpdate` | 800D5AF0 | car_destroyed.c | Los Angeles events. |
| `moscowEventsUpdate` / `moscowDishUpdate` | 800D4D58 / 800D6188 | random_event_level2.c / car_destroyed.c | Moscow events and the dish. |
| `eiffelTowerUpdate` | 800D6800 | car_object_contact.c | Paris tower fall. |
| `worldCollapseUpdate` | 801163B8 | world_collapse.c | Collapsing sections. |
| `worldWarpsSetup` / `worldWarpsUpdate` | 80115310 / 801157D4 | car_damage_human.c / world_collapse_warps.c | Teleporters. |
| `worldTrackVehiclesUpdate` | 80117B40 | track_vehicles.c | The two moving track vehicles. |
| `targetRegister` / `targetDamage` | 80124F38 / 80125118 | target_pool.c / target_executor.c | Breakable targets (types 500..520). |
| `targetActorsUpdate` | 80126464 | target_actor_update.c | Pedestrians. |
| `targetTimedEventUpdate` etc. | 801274C0, 801277A8, 80127CE0 | target_actor_events.c, target_secondary_events.c, target_slot_update.c | Timed level events (levels 1, 5, 7). |
| `levelEventsReset` | 800D42E0 | weapon_fire_specials.c | Clears the level event tables. |

Globals: `worldSections` (801A9854) and its tables, `worldSurfaces`
(801B0418), `laEvents` (801AB438), `moscowEvents` (801AB064),
`moscowDishState` (801B2DCC), `eiffelTowerState` (801AB810),
`worldCollapseSections` (80187448), `worldWarps` (80187934),
`trackVehicleRecords` (80180F84), `targetPool` (801909D0), `targetActors`
(80190E54), the `objReg*` tables (8018F5E4..8018F93C). **Unclear:** the
meaning of most object type ids is only known case by case (see the
comments in `world_load_object.c`).

### 2.12 Camera, views, projection

Each player view has a mode (an index into the view-mode table: screen size,
projection, base matrices), a draw/display environment and a camera object.
`viewProc` builds the frame's camera matrices. See `MAIN_LOOP.md` and
`WIDESCREEN.md`.

| Function | VA | File | Purpose |
|---|---|---|---|
| `viewCarCameraUpdate` | 80102BE8 | camera.c | Per-player camera, by the car's view. |
| `viewChaseCamera` | 80102CC4 | camera.c | Chase camera (views 1..4, 7). |
| `viewFixedCameraTrack` | 8010353C | camera.c | Fixed camera that turns to the car (views 5, 6). |
| `viewProc` | 80155E80 | view_system.c | Builds the view's camera matrices and projection. |
| `viewCreate` / `viewLoadMode` | 80155888 / 80155B30 | view_system.c | Environments and matrices for a mode. |
| `viewInit` | 8015580C | view_system.c | Resets all views. |
| `viewSetFixedCamera` | 801566F8 | view_system.c | Flat camera for menus and car models. |
| `viewSetFov` | 80155DB8 | engine_misc_accessors.c | FOV to view mode. |
| `viewGetCentre` | 80156614 | view_system.c | Screen centre. |
| `viewSetSplitMode` | 800D1C24 | rt_main.c | Split-screen layout. |
| `carSetViewMode` | 800E7380 | weapon_missile_trails.c | Switches a car's camera view. |
| `carViewHotkeys` | 8011C530 | car_records.c | View hotkeys, split toggle. |
| `viewWatchNextAiCar` | 8011DAF8 | view_watch_ai.c | Spectate an AI car (view 7). |
| `viewLoadGte` | 8014965C | engine_clut_anim.c | Loads GTE matrices for a view. |

Globals: `viewModeTable` (8017EC44), `viewModes` (80181120),
`viewCameraObjects` (80181110), `viewMatrices` (801A51C0), `viewEyePositions`
(801A51A8), `viewSplitScreenMode` (801809E4), `viewWatchedAiCar` (80180FAC),
`viewFov` (80180DE4), `cameraPrevPos` (80183F14).

### 2.13 Renderer

SingleTrac engine code in the `engine_*.c` units (retail 80148000..80156000) plus the model emitter
at the start of the executable. The model walker, emitter and near pass are
hand-written assembly. The build keeps them as asm; readable C copies are in
`reference/`. See `DRAW_DISTANCE.md`, `CAR_SELECT_CAR.md`, `WIDESCREEN.md`.

| Function | VA | File | Purpose |
|---|---|---|---|
| `renderScene` | 80152284 | engine_render_scene.c | The 3D scene: level and every object model. |
| `renderWalkModel` | 801526CC | engine_render_scene.c | Model-tree interpreter (asm). |
| `renderInitNodeHandlers` | 80153460 | engine_render_scene.c | Fills the walker's node handler table. |
| `sub_800CDC54` | 800CDC54 | model_emitter.c | Model emitter: projects and lights meshes into the packet buffer (asm; C in `reference/model_emitter_800CDC54.c`). |
| `renderNearPolys` | 80154690 | engine_math_near_pass.c | Near-polygon split pass (asm). |
| `renderFlipFrame` | 80152194 | engine_render_scene.c | Renderer double-buffer flag. |
| `renderDrawModel` | 80151378 | engine_anim_hit_volumes.c | Draws one model alone (menus, car select). |
| `renderInit` | 8015155C | engine_anim_hit_volumes.c | Renderer buffers and lists. |
| `csUpdMat` | 801491E8 | engine_clut_anim.c | Object matrices from the parent. |
| `csSphereVisible` | 80149344 | engine_clut_anim.c | Bounding-sphere frustum test. |
| `csAlloc` / `csListAdd` | 8014952C / 80149444 | engine_clut_anim.c | Object pool (128); drawn list. |
| `renderSetFarDistance` | 8015165C | engine_misc_accessors.c | Far cull distance. |
| `renderEnableNodeCulling` | 801516C0 | engine_misc_accessors.c | Per-node culling. |
| `lightInit`, `lightSetEnvLight`, `lightGetEnv` | 801539DC, 80153510, 80153E3C | engine_render_scene.c | Lighting environments. |
| `lightAddSources` | 8014D3F4 | engine_fileio_debris.c | Point lights. |
| `animUpdateAll` | 80151700 | engine_anim_hit_volumes.c | Model node animations. |
| `clutAnimUpdate` | 80148B1C | engine_clut_anim.c | Palette animations. |
| `renderAddWorldModel` | 801512F8 | controls.c | Extra per-frame world models. |

Globals: `renderFarDistance` / `renderFarDistanceDefault` (80180DA8 /
80180DAC, 4800), `renderNodeCullEnable` (80180DC8), `renderCtx` (80180DCC,
scratchpad render state), `renderWorldShift` (80180DB4), `renderPacketBuffer0/1`
(801B600C / 801CDD4C), `renderOtView1Buf0..renderOtView2Buf1`
(801E5A8C..801F1A8C), `renderView0Frames` (8018118C), `csObjectPool`
(8019A950), `csRootObject` (801810D0), `lightEnvironments` (801A5050),
`renderNodeHandlers` (801B2CD0), `renderObjectMatrices` (801F8AB8).
Struct: `TM2BufferInitRecord` in `rt_buffers.h`. The `dbscan`
hierarchy walker that registers loaded models is `dbscanHierarchy` (8014ABF8).

### 2.14 HUD

| Function | VA | File | Purpose |
|---|---|---|---|
| `hudDrawSingleView` | 800F8370 | hud_main.c | One-view HUD. |
| `hudDrawSplitVertical` / `hudDrawSplitHorizontal` | 800F9084 / 800F9D48 | hud_split.cpp | Split-screen HUDs. |
| `hudInitPanels` | 800FB0B0 | hud_init.c | HUD primitives per view mode; rear-view mirror. |
| `UAdashInitDashboardIcons` | 800FAA0C | hud_icons.c | Dashboard icons. |
| `hudDisplayRadar` / `hudRadarPlotCar` | 800FBE68 / 800FC928 | hud_radar.c | Radar. |
| `hudRadarInit` | 800FC5C4 | ai_route_setup.c | Radar records per view mode. |
| `hudDrawTargetPanel` | 800FD0D0 | car_destroyed.c | Targeted car's name and health bar. |
| `hudMessagePost` / `hudMessageUpdate` | 800FBBC4 / 800FBC44 | audio_hud_helpers.c / hud_state.c | Top-line messages by priority. |
| `UAdashHealthColor` | 800FBD04 | hud_state.c | Health colour. |
| `UAdashDrawRearViewMirror` | 800FBB0C | hud_main.c | Rear-view mirror frame. |
| `hudDrawNumber` | 8014E194 | packet_digits.c | 1–3 digit numbers. |
| `hudSmoothSpeed` / `carGetDisplaySpeed` | 8011CBD0 / 8011CD08 | car_records.c | Speed readout. |
| `fontInit` / `fontPrint` / `fontSpritePrint` | 80113F30 / 80113724 / 80113C00 | font_init.c / font_print.c / font_buffer.c | Text. |

Globals: `hudPlayerRecords` (801828F0, 1900 bytes per player), `hudRadars`
(801837C8), `hudOrderTables` (801828D0), `hudMessageCurrent` (80183F00),
`hudMessageStrs` (8016286C), `hudRadarVisible` / `hudWeaponListVisible`
(80180A6C / 80180A70), `fontDrawModes` (801871E4), `fontSpriteTemplates`
(80187238).

### 2.15 Effects: fx, debris, explosions, particles

| Function | VA | File | Purpose |
|---|---|---|---|
| `fxRecordsUpdate` | 800DC348 | weapon_fire_specials.c | Ticks the 16 effect/projectile records. |
| `fxSpriteSpawn` | 80110918 | fx_sprite_spawn.c | Spawns an effect sprite (pool of 150). |
| `fxSpritesUpdate` / `fxSpritesDraw` | 80110A40 / 80110BE4 | car_restart.c / world_collapse.c | Effect sprites. |
| `fxExplosion6` .. `fxExplosion192` | 80111464.. | fx_explosions.c | Explosions by size. |
| `fxFlame3`, `fxFlameBurning`, ... | 80111D20.. | effect_bindings/ | Flame animations. |
| `fxImpactDebris` | 80112C90 | fx_debris_splash.c | Impact debris particles. |
| `fxImpactParticlesUpdate` / `fxImpactParticlesDraw` | 80112E58 / 80112F6C | fx_sprite_spawn.c / car_damage_human.c | The 20 debris particles. |
| `debrisUpdate` / `debrisDraw` | 8014B45C / 8014B484 | engine_clut_anim.c | Debris chunks and bursts. |
| `debrisBurstAt` | 8014B820 | engine_clut_anim.c | Starts a burst. |
| `fxDebrisDraw` | 8014C19C | engine_fileio_debris.c | Draws debris chunk models. |
| `streaksDraw` | 800D8230 | screen_card_draw.c | Streaks, FX sheet, tasers (also ticks state, see `MAIN_LOOP.md`). |
| `fxTrailUpdate` | 800D9238 | ai_route_nodes.c | Trail effect. |
| `fxSheetStart` / `fxSheetDisplay` | 800DC45C / 800DC590 | weapon_fire_specials.c / weapon_special_missiles.c | Full-screen flash. |
| `explodeMakeScreenRed` | 8011335C | fx_debris_splash.c | Red screen flash. |
| `explodeStore*Animation` | 80112870.. | effect_bindings/ | Bind loaded effect sprite frames. |
| `lightFlickerUpdate` | 80114B48 | misc_one_liners.c | Light flicker timers. |

Globals: `fxSprites` (80185144), `fxImpactParticles` (80184E74),
`fxExplosionFrames` (80186DE4), `fxFlameFrames` (80187144), the smoke and
debris frame tables (80186D64..801870E4), `debrisChunks` (801A3D50),
`debrisBursts` (801A3E70), `fxSheetPolys` (80181DF8). **Unclear:** most of
the 30 one-function files in `effect_bindings/` are still `func_` (asset
binders and small spawners).

### 2.16 Sound

Three layers. The **sound manager** is a C++ object on `main`'s stack
(`soundManagerObject`); game code calls thin wrappers. It submits sound
objects into request lists (**engine**). The **driver** maps sound
instances to the 24 SPU voices through libsnd. CD music is a separate CD
audio player (`musicManagerObject`).

| Function | VA | File | Purpose |
|---|---|---|---|
| `soundPlay` / `soundPlayVariant` / `soundStop` | 800D3334 / 800D3398 / 800D3464 | command_wrappers.cpp | Game-side wrappers. |
| `soundStartPlayerLoop` / `soundStopPlayerLoop` | 800D3244 / 800D328C | command_wrappers.cpp | Per-player loops (turbo, guns). |
| `soundMgrPlay` / `soundMgrStop` | 80128890 / 801289B0 | sound_manager_play.c / sound_manager_reap.c | Manager: allocate and submit sound objects. |
| `soundUpdate` | 800D34C0 | timer_frame.c | Per-frame sound step. |
| `taskMgrRunFrame` | 80160428 | engine_sound_driver.c | Applies start/stop requests, runs the driver. |
| `sndListSubmit` | 8015E57C | engine_sound_lists.c | Adds a request to a list by priority. |
| `sndObjUpdate` | 8015F42C | engine_sound_object_update.c | Voice start, attenuation, pan, doppler. |
| `sndListenerTransform` | 8015DBE8 | engine_sound_lists.c | World position to listener space. |
| `soundDriverUpdate` | 80161318 | engine_sound_driver.c | Retires, sorts and steals voices. |
| `soundOn` / `soundInstUpdate` | 80160F20 / 80160CCC | engine_sound_driver.c | Key on; per-instance pitch and volume. |
| `soundInit` | 80156D08 | engine_math_near_pass.c | Default settings, starts libsnd. |
| `soundVabOpenRetry` | 801283F8 | vab_retry.c | Opens a VAB with retries. |
| `carSoundUpdate` | 8011DB74 | screen_card_draw.c | Engine, skid, horn, surface sounds. |
| `cdAudioPlayTrack` / `cdAudioPlayRange` | 8015E910 / 8015EB04 | engine_sound_lists.c | CD-DA music. |
| `audioAlternateMusicTrack` / `audioFadeOutMusic` | 800D3944 / 800D3A44 | weapon_special_missiles.c | Music switch and fade. |
| `menuVolumeApply` | 80136FF8 | menu_volume_action.cpp | Volume sliders. |

Globals: `soundManagerObject` (80180BC8), `musicManagerObject` (80180BCC),
`sndEffectTable` (801640DC), `sndObjPool` (8017F1EC), `soundInstPool`
(8017F5D0), `soundVoiceInUse` (8017F570), `musicVolume` (801809F8),
`audioSfxVolume` (80180E18), `audioCdVolume` (80180E0C), `cdAudioPlayState`
(80180EA0). **Unclear:** the meaning of most sound ids, and `soundXARepeat`
(8015EBA0, `sound_xa_repeat.c`).

### 2.17 File I/O and resource loading

| Function | VA | File | Purpose |
|---|---|---|---|
| `fileioInit` | 8014DAA4 | engine_fileio_debris.c | `CdInit`. |
| `fileioLoadFileIntoRam` | 8014DA30 | engine_fileio_debris.c | Open, read the whole file, close. |
| `fileioOpenFile` / `fileioReadFile` | 8014D700 / 8014D910 | engine_fileio_debris.c | Search `\NAME;1` and blocking read with retry. |
| `fileioReadFileAsync` | 8014D9D4 | engine_fileio_debris.c | Async `CdRead`. |
| `shellLoadTmsDmd` | 80129010 | screen_card_layout.c | Loads `<name>.TMS` / `.DMD` and installs them. |
| `assetInstallImage` | 80120EF4 | asset_header.c | Installs a loaded asset image. |
| `modelRelocate` | 80120FEC | screen_card_draw.c | Relocates a model's pointers. |
| `timIntoVRAM` / `shellTextureInit` | 8014E4C8 / 8014E558 | engine_hit_detection.c | TIM upload; texture packs. |
| `bitmapParseContainer` | 80121A2C | bitmap_container_parse.c | TXSP texture container. |
| `bitmapInit` / `bitmapMarkRect` | 80121550 / 801215F8 | bitmap_init.c / bitmap_rect_fill.c | VRAM usage bitmap. |
| `vramPlaceTexture` / `vramUploadClut` | 801209E8 / 80120D3C | screen_deathmatch_result.c / scene_init.c | VRAM placement. |
| `modelRemapTextures` | 80120744 | world_collapse_warps.c | Points model primitives at the current textures. |
| `screenLoadImageFile` | 80121C64 | screen_basic.c | Shell images. |
| `levelFileStem`, `settingsLevelDmdFileName`, `carDmdFileName` | 8012ABD0, 8012A998, 8012AB20 | car_parts_spin.c, screen_card_layout.c, arena_load_register.c | File names. |

Globals: `fileioCdFile` (801A4710), `fileioWorkBuffer` (80180D58),
`vramTexSlots` (80181034), `bitmapRecords` (80181040). The level and car
files load at 0x80019C40.

### 2.18 Movies

| Function | VA | File | Purpose |
|---|---|---|---|
| `moviePlay` | 801384C4 | movie_stream.c | Finds the .STR file and runs the decode/display loop. |
| `movieStrInit` / `movieStrCallback` / `movieStrNext` | 80138068 / 801380D8 / 801382A8 | movie_stream.c | MDEC streaming (Sony tutorial shape). |
| `moviePlayEnding` | 8012B898 | shell_main.c | Movie player wrapper (wide flag). |
| `movieBuffersInit` | 80137FD0 | menu_object_setup.c | Movie display buffers. |
| `shellPlayTitleIntro` | 8012DD74 | shell_title_intro.c | Title movie. |
| `screenHistoryMovie` | 801368D8 | screen_history_movie.cpp | History movie. |
| `screenEnding` | 80136D98 | screen_ending_setup.c | Ending movies. |
| `movieSkipCheck` / `movieSkipCheckDelayed` | 8012D024 / 8012B82C | menu_password_digits_create.c / ai_route_helpers.c | Skip tests. |

Globals: `movieDecEnv` (801914B8), `movieRingBuffer` (801810B8),
`movieRgb24` (801810AC), `endingMovieTable` (801648C4).

### 2.19 PsyQ SDK (out of scope)

Sony's libraries (libc, libapi, libetc, libgpu, libgte, libcd, libpress,
libsnd, libspu), from 0x80138000 on. The port does not need them. A source
build can link them from the PsyQ `.LIB` files instead
(`build.cmd --source-only --sdk-from-lib`, `config/US/sdk_modules.tsv`).
See `SDK_SOURCES.md` and `THIRD_PARTY.md`.

Files: `bios_calls.c`, `bios_extra.c`, `cd_control.c`, `cd_toc.c`,
`check_rect.c`, `gpu_*.c`, `gpu_bindings/`, `gte_*.c`, `mdec_transfer.c`,
`mdec_registers.c`, `sdk_*.c`, `lib_r138.c`, `lib_r140.c`,
`sdk_libgpu_font.c`, `sdk_libsnd_vs_vh.c`, `sdk_libsnd_leftovers.c`, `sdk_libspu_leftovers.c`,
`sdk_libsnd_ut.c`, `sdk_unknown_o0.c`. Most functions there keep `sub_` names;
the unit header names them.

Not SDK, although they sit above 0x80138000: the SingleTrac engine in
`engine_clut_anim.c`, `engine_fileio_debris.c`, `engine_hit_detection.c`, `engine_anim_hit_volumes.c`, `engine_render_scene.c`,
`engine_math_near_pass.c`, `engine_sound_lists.c`, `engine_sound_object_update.c`, `engine_sound_driver.c`,
`controls.c`, `shared_pad.c`, `view_system.c`, `timer_frame.c`,
`movie_stream.c` (sections 2.5 to 2.18).

## 3. What the mixed files hold

Many units were made in batches by address range, so one file can hold
unrelated functions. Each is named after its main content (see
`config/US/file_renames.tsv` for the reason of every name, and "Renamed
files" at the end for the old names). This table lists what else is in them.

| File | Main contents |
|---|---|
| `weapon_bullets.c` | Moscow/LA event arming, machine-gun bullets, missile trail UVs. |
| `car_wheel_sway.c` | `worldSectionAt`, wheel sway, Mr. Slam carry, spin-out by object id. |
| `ai_route_setup.c` | Boss route patch, `aiPlaceOnRouteNode`, `hudRadarInit`, `weaponBlockInit`, `aiCarReset`. |
| `car_damage_human.c` | Drop hit test, debris particle draw, warp setup, `humanCarDamageUpdate`. |
| `screen_deathmatch_result.c` | `carPairsUpdate`, `vramPlaceTexture`, opponent card screen, Deathmatch result. |
| `car_destroyed.c` | LA events, Moscow dish, HUD target panel, `carDestroyed`, password cursor. |
| `ai_route_nodes.c` | `aiCarSetupProfile`, trail effect, body sway, AI route node choice. |
| `screen_card_draw.c` | `streaksDraw`, Mr. Slam target, `carSoundUpdate`, `modelRelocate`, card drawing. |
| `car_object_contact.c` | Moscow triggers, Eiffel tower, contact with objects, ricochet off object, `humanCarPlaceAtStart`. |
| `screen_pause_menu.c` | `carSuspensionUpdate`, AI route target, collapse reset, `pauseMenu`, `screenHistory`. |
| `hud_radar.c` | Radar draw and blips, `carStatsDerive`. |
| `car_crash.c` | Car crash, wreck update, contact points. |
| `car_tumble.c` | Barrel roll, tumble, AI segment speed. |
| `world_collapse.c` | Effect sprite draw, collapsing sections and their explosions. |
| `weapon_arcs_draw.c` | Mr. Slam grab, taser/lightning arc draw. |
| `weapon_slam_swing.c` | `projectileHitTest`, Mr. Slam swing. |
| `car_contact_resolve.c` | Homing steering, `carContactResolve`. |
| `weapon_ricochet.c` | Ricochet shots. |
| `world_load_object.c` | `worldLoadObject` (the DMD object loader). |
| `game_misc_accessors.c` | Functions IDA missed (800D..8012): frame-record helpers, HUD toggles, car getters, scene helpers. |
| `engine_misc_accessors.c` | Functions IDA missed (8013..8016): pad helpers, view FOV getters, far distance, timer, bit tables. |
| `misc_one_liners.c` | One-liners: sound locate, event getters, car pair getters, level file names, `sessionTeardown`. |
| `arena_load_register.c` | Slot searches, car part and effect object registration, `arenaLoad*` per level, `settingsInitDefaults`. |
| `ai_route_helpers.c` | Projectile reset, power missile, route node removal, `pickupRegister`, `carRegisterModel`, view creation. |
| `world_object_register.c` | Level event object registration, missile objects, section heights, Mr. Slam release, collapse registration. |
| `weapon_special_missiles.c` | Music switch and fade, AI weapon pick, special missiles, FX sheet draw. |
| `ai_route_loaders.c` | `routeLoad<Track>` (.PTS route files), drop reset. |
| `fx_explosions.c` | `fxExplosion*`, light flicker tick, `aiCarShouldUpdate`. |
| `screen_card_layout.c` | `shellLoadTmsDmd`, wait helpers, card layout, level file names, `sessionBindCars`. |
| `weapon_fire_specials.c` | Sound active flags, AI weapon pick, level event reset, missile spawns, `fxRecordsUpdate`, FX sheet. |
| `ai_car_start.c` | Section edge distance, route edits, section heights, default top speed, camera offsets, `aiCarStart`. |
| `fx_sprite_spawn.c` | Drops, pickup respawn, ground fires, shields, `fxSpriteSpawn`, particles. |
| `car_parts_spin.c` | Car part spinners, track vehicle place, lava splash, `carBindAiSlot`, `screenLostALife`, name tables. |
| `weapon_fire_missiles.c` | Moscow dish destroy, effect record init, homing/napalm/coloured missiles, car shake. |
| `car_restart.c` | Section disable, wheel block reset, `carRestart`, collision box, flame start, `fxSpritesUpdate`. |
| `fx_debris_splash.c` | Impact debris, red screen, car part reset, warp start, collapse release, track vehicle destroy, splash. |
| `scene_init.c` | `roundStart`, `carBindHumanSlot`, AI count, CLUT upload, `sceneObjDraw`, `sceneInit`, password string, cards. |
| `weapon_missile_trails.c` | LA event arm, missile trails, `weaponApplyHit`, car shake, `carSetViewMode`. |
| `ai_link_node_pick.c` | AI link node choice, jitter, invisibility. |
| `world_collapse_warps.c` | Warps update, collapse move and debris, `aiCarDamageUpdate`, demo spectator, texture remap. |
| `arena_setup.c` | Tower reset, bullet slot search, Dark Tooth stage 2, `arenaSetup`, `humanCarsRestart`, music mute. |
| `lib_r138.c`, `lib_r140.c` | PsyQ SDK (libcd, libgte, libgpu, ...). |
| `engine_clut_anim.c` | Engine: palette animation, object pool (`cs*`), pad start/stop, dbscan, debris. |
| `engine_fileio_debris.c` | Engine: debris draw, light sources, file I/O (`fileio*`), CLUT/VRAM line helpers. |
| `engine_hit_detection.c` | Engine: TIM upload, texture packs, hit detection (`hdp*`). |
| `engine_anim_hit_volumes.c` | Engine: hit volumes, model draw helpers, `renderInit`, animation list. |
| `engine_render_scene.c` | Engine: scene renderer, model walker, lighting environments. |
| `engine_math_near_pass.c` | Engine: math, near-polygon pass, view-mode accessors, angle helpers, `soundInit`. |
| `engine_sound_lists.c` | Engine: sound request lists, listener, CD audio player, sound object pool. |
| `engine_sound_driver.c` | Engine: task manager, sound driver (instances, voices). |
| `sdk_libgpu_font.c`, `sdk_libsnd_vs_vh.c`, `sdk_libsnd_leftovers.c`, `sdk_libspu_leftovers.c`, `sdk_libsnd_ut.c`, `sdk_unknown_o0.c` | PsyQ SDK leftovers. |
| `audio_hud_helpers.c` | Listener pan, volumes, HUD message post, armour icon. |
| `view_watch_ai.c` | Holland arena loader, AI spectating. |
| `world_fall_limit.c` | Axel part spin, track vehicle reset, fall limit. |
| `spawn_tables.c` | Attract-demo spawns per level, and controller layouts (`padLoadLayout`, `padSetLayout`). |
| `terrain_load_presets.c`, `terrain_load.c` | `.RMP` / `.GRP` terrain loaders per track. |
| `controls.c` | Pad action layer, plus hdp stack setters and `renderAddWorldModel`. |
| `car_switch_registry.c`, `car_freeze_dispatch.c`, `car_freeze_build.c` | Level switch objects per car group (damage stage, tires, freeze), pickup display objects. |
| `menu_text_callbacks.c`, `menu_password_digit*.c`, `menu_slider_*.cpp`, `menu_timed_command.cpp`, `menu_code_sequences.cpp` | Password keypad, slider bar, code detector, menu text callbacks (the old `game_movie_*` files; not movies). |
| `sound_manager_play.c`, `sound_manager_reap.c`, `sound_manager_init.c` | Sound manager (plus shell init and arena reset in `sound_manager_reap.c`). |
| `fx_wake_object.c`, `target_level_object.c` | Wake object install, pooled object step, target object lookup. |
| `timer_frame.c` | Frame timer, plus `soundUpdate` and `carSetResetWeapons`. |
| `effect_bindings/` | One file per effect asset binder or spawner. |
| `font_color.c`, `weapon_effect_slots.c` | Font colour; ricochet and shield slot searches. |
| `pickup_spawn.c` | `weaponSlamStateInit` (see "Some names mislead"; kept, the right name is **unclear**). |
| `model_emitter.c` | The asm model emitter `sub_800CDC54`. |
| `view_chase_camera_draft.c`, `weapon_slam_swing_draft.c` | Older stand-alone versions of `viewChaseCamera` and `weaponSlamSwingUpdate`. Not in the selection template; the build uses `camera.c` and `weapon_slam_swing.c`. **Unclear** whether they are still needed. |
| `*_state.c`, `*_tables.c`, `*_records.c`, `carved_initializers.c`, `referenced_strings.c` | Data only (globals at their retail addresses). |

## 4. Renamed files

Source files renamed so their names say what they hold (2026-10-06). VAs
and symbols did not change. The full list with the reason for each name is
`config/US/file_renames.tsv`; `tools/port_rename_notes.py` updates notes
that still use the old names (see `PORT_RENAMES.md`).

<!-- renamed-files:begin (generated by tools/rename_file.py from config/US/file_renames.tsv) -->

| Old file | New file |
|---|---|
| `src/semantic/game_code1.c` | `src/semantic/weapon_bullets.c` |
| `src/semantic/game_code2.c` | `src/semantic/car_wheel_sway.c` |
| `src/semantic/game_code3.c` | `src/semantic/ai_route_setup.c` |
| `src/semantic/game_code4.c` | `src/semantic/car_damage_human.c` |
| `src/semantic/game_code5.c` | `src/semantic/screen_deathmatch_result.c` |
| `src/semantic/game_code6.c` | `src/semantic/car_destroyed.c` |
| `src/semantic/game_code7.c` | `src/semantic/ai_route_nodes.c` |
| `src/semantic/game_code8.c` | `src/semantic/screen_card_draw.c` |
| `src/semantic/game_code9.c` | `src/semantic/car_object_contact.c` |
| `src/semantic/game_code10.c` | `src/semantic/screen_pause_menu.c` |
| `src/semantic/game_code11.c` | `src/semantic/hud_radar.c` |
| `src/semantic/game_code12.c` | `src/semantic/car_crash.c` |
| `src/semantic/game_code13.c` | `src/semantic/car_tumble.c` |
| `src/semantic/game_code14.c` | `src/semantic/world_collapse.c` |
| `src/semantic/game_code15.c` | `src/semantic/weapon_arcs_draw.c` |
| `src/semantic/game_code16.c` | `src/semantic/weapon_slam_swing.c` |
| `src/semantic/game_code17.c` | `src/semantic/car_contact_resolve.c` |
| `src/semantic/game_code19.c` | `src/semantic/weapon_ricochet.c` |
| `src/semantic/game_code20.c` | `src/semantic/world_load_object.c` |
| `src/semantic/game_extra1.c` | `src/semantic/game_misc_accessors.c` |
| `src/semantic/game_extra2.c` | `src/semantic/engine_misc_accessors.c` |
| `src/semantic/small_helpers.c` | `src/semantic/misc_one_liners.c` |
| `src/semantic/small_helpers2.c` | `src/semantic/arena_load_register.c` |
| `src/semantic/small_helpers3.c` | `src/semantic/ai_route_helpers.c` |
| `src/semantic/small_helpers4.c` | `src/semantic/world_object_register.c` |
| `src/semantic/small_helpers5.c` | `src/semantic/weapon_special_missiles.c` |
| `src/semantic/small_helpers6.c` | `src/semantic/ai_route_loaders.c` |
| `src/semantic/small_helpers7.c` | `src/semantic/fx_explosions.c` |
| `src/semantic/small_helpers8.c` | `src/semantic/screen_card_layout.c` |
| `src/semantic/small_helpers9.c` | `src/semantic/weapon_fire_specials.c` |
| `src/semantic/small_helpers10.c` | `src/semantic/ai_car_start.c` |
| `src/semantic/small_helpers11.c` | `src/semantic/fx_sprite_spawn.c` |
| `src/semantic/small_helpers12.c` | `src/semantic/car_parts_spin.c` |
| `src/semantic/mid_helpers1.c` | `src/semantic/weapon_fire_missiles.c` |
| `src/semantic/mid_helpers2.c` | `src/semantic/car_restart.c` |
| `src/semantic/mid_helpers3.c` | `src/semantic/fx_debris_splash.c` |
| `src/semantic/mid_helpers4.c` | `src/semantic/scene_init.c` |
| `src/semantic/mid_helpers5.c` | `src/semantic/weapon_missile_trails.c` |
| `src/semantic/mid_helpers6.c` | `src/semantic/ai_link_node_pick.c` |
| `src/semantic/mid_helpers7.c` | `src/semantic/world_collapse_warps.c` |
| `src/semantic/leftovers1.c` | `src/semantic/arena_setup.c` |
| `src/semantic/shell_screens1.c` | `src/semantic/screen_controller_multiplayer.c` |
| `src/semantic/shell_screens2.c` | `src/semantic/screen_title_level_select.c` |
| `src/semantic/shell_screens3.c` | `src/semantic/screen_options_audio.c` |
| `src/semantic/func_800EE950.c` | `src/semantic/world_surface_height.c` |
| `src/semantic/func_800F220C.c` | `src/semantic/world_section_poly_reset.c` |
| `src/semantic/func_8010A7A0.c` | `src/semantic/weapon_flame_update.c` |
| `src/semantic/func_8010C160.c` | `src/semantic/fx_ground_fires.c` |
| `src/semantic/func_8010FD44.c` | `src/semantic/ai_dark_tooth_boss.c` |
| `src/semantic/fd44_object.c` | `src/semantic/ai_dark_tooth_boss_object.c` |
| `src/semantic/fd44_scalars.c` | `src/semantic/ai_dark_tooth_boss_scalars.c` |
| `src/semantic/fd44_state.c` | `src/semantic/ai_dark_tooth_boss_state.c` |
| `src/semantic/fd44_vectors.c` | `src/semantic/ai_dark_tooth_boss_vectors.c` |
| `src/semantic/game_movie_helpers.c` | `src/semantic/menu_text_callbacks.c` |
| `src/semantic/game_movie_record_index.c` | `src/semantic/menu_password_digit_set.c` |
| `src/semantic/game_movie_six_render.c` | `src/semantic/menu_password_digits_draw.c` |
| `src/semantic/game_movie_support.c` | `src/semantic/menu_password_digits_create.c` |
| `src/semantic/game_movie_grid.cpp` | `src/semantic/menu_slider_create.cpp` |
| `src/semantic/game_movie_grid_render.cpp` | `src/semantic/menu_slider_draw.cpp` |
| `src/semantic/movie_grid.hpp` | `src/semantic/menu_slider.hpp` |
| `src/semantic/game_movie_controller.cpp` | `src/semantic/menu_timed_command.cpp` |
| `src/semantic/game_movie_sequence.cpp` | `src/semantic/menu_code_sequences.cpp` |
| `src/semantic/car_choice_builder.c` | `src/semantic/car_freeze_build.c` |
| `src/semantic/car_choice_dispatch.c` | `src/semantic/car_freeze_dispatch.c` |
| `src/semantic/car_choice_registry.c` | `src/semantic/car_switch_registry.c` |
| `src/semantic/car_choice_state.c` | `src/semantic/car_switch_registry_state.c` |
| `src/semantic/shield_update.c` | `src/semantic/weapon_tornado.c` |
| `src/semantic/object_construction.c` | `src/semantic/sound_manager_play.c` |
| `src/semantic/object_shell_sequences.c` | `src/semantic/sound_manager_reap.c` |
| `src/semantic/object_type_initialization.c` | `src/semantic/sound_manager_init.c` |
| `src/semantic/game_option_available.c` | `src/semantic/car_is_selectable.c` |
| `src/semantic/game_option_codec.c` | `src/semantic/password_codec.c` |
| `src/semantic/game_radix_convert.c` | `src/semantic/password_rebase.c` |
| `src/semantic/game_random_options.c` | `src/semantic/tournament_pick_opponents.c` |
| `src/semantic/game_random_transition.c` | `src/semantic/screen_pick_opponents.c` |
| `src/semantic/game_state_cycle.c` | `src/semantic/shell_attract_demo.c` |
| `src/semantic/game_title_setup.c` | `src/semantic/shell_title_intro.c` |
| `src/semantic/menu_media_setup.c` | `src/semantic/screen_ending_setup.c` |
| `src/semantic/menu_intro_setup.cpp` | `src/semantic/screen_history_movie.cpp` |
| `src/semantic/menu_credit_loader.c` | `src/semantic/screen_credits.c` |
| `src/semantic/object_pointer_tables.c` | `src/semantic/fx_wake_object.c` |
| `src/semantic/object_record_lookup.c` | `src/semantic/target_level_object.c` |
| `src/semantic/motion_target_update.c` | `src/semantic/track_vehicle_ground.c` |
| `src/semantic/motion_record_accessors.c` | `src/semantic/track_vehicle_accessors.c` |
| `src/semantic/timer_consumers.c` | `src/semantic/timer_frame.c` |
| `src/semantic/effect_font_views.c` | `src/semantic/font_color.c` |
| `src/semantic/effect_pool.c` | `src/semantic/weapon_effect_slots.c` |
| `src/semantic/homing_init.cpp` | `src/semantic/weapon_leap_homing_init.cpp` |
| `src/semantic/homing_update.c` | `src/semantic/weapon_leap_homing_update.c` |
| `src/semantic/event_object_slots.c` | `src/semantic/eiffel_tower_emitters.c` |
| `src/semantic/actor_lookup.c` | `src/semantic/car_object_lookup.c` |
| `src/semantic/anchor_consumers.c` | `src/semantic/audio_hud_helpers.c` |
| `src/semantic/anchor_state.c` | `src/semantic/audio_hud_state.c` |
| `src/semantic/shared_flags.c` | `src/semantic/view_watch_ai.c` |
| `src/semantic/player_loops.c` | `src/semantic/world_fall_limit.c` |
| `src/semantic/short_list.c` | `src/semantic/ai_start_nodes.c` |
| `src/semantic/short_list_state.c` | `src/semantic/ai_start_nodes_state.c` |
| `src/semantic/track_presets.c` | `src/semantic/terrain_load_presets.c` |
| `src/semantic/aim_update.c` | `src/semantic/view_chase_camera_draft.c` |
| `src/semantic/camera_action_update.c` | `src/semantic/weapon_slam_swing_draft.c` |
| `src/semantic/lib_r148.c` | `src/semantic/engine_clut_anim.c` |
| `src/semantic/lib_r14c.c` | `src/semantic/engine_fileio_debris.c` |
| `src/semantic/lib_r14e.c` | `src/semantic/engine_hit_detection.c` |
| `src/semantic/lib_r150.c` | `src/semantic/engine_anim_hit_volumes.c` |
| `src/semantic/lib_r152.c` | `src/semantic/engine_render_scene.c` |
| `src/semantic/lib_r154.c` | `src/semantic/engine_math_near_pass.c` |
| `src/semantic/lib_r15c_eng.c` | `src/semantic/engine_sound_lists.c` |
| `src/semantic/lib_r15c_f42c.c` | `src/semantic/engine_sound_object_update.c` |
| `src/semantic/lib_r160.c` | `src/semantic/engine_sound_driver.c` |
| `src/semantic/lib_r140_font.c` | `src/semantic/sdk_libgpu_font.c` |
| `src/semantic/lib_r140_snd.c` | `src/semantic/sdk_libsnd_vs_vh.c` |
| `src/semantic/lib_r154_snd.c` | `src/semantic/sdk_libsnd_leftovers.c` |
| `src/semantic/lib_r15c_snd.c` | `src/semantic/sdk_libsnd_ut.c` |
| `src/semantic/lib_r15c.c` | `src/semantic/sdk_libspu_leftovers.c` |
| `src/semantic/lib_r15c_o0.c` | `src/semantic/sdk_unknown_o0.c` |
| `reference/hdp_walker_lib_r14e.c` | `reference/hdp_walker_engine_hit_detection.c` |
| `reference/near_pass_lib_r154.c` | `reference/near_pass_engine_math_near_pass.c` |
| `reference/renderer_walker_lib_r152.c` | `reference/renderer_walker_engine_render_scene.c` |
| `src/semantic/effect_bindings/func_80111314.c` | `src/semantic/effect_bindings/fxSparkOrange.c` |
| `src/semantic/effect_bindings/func_801113F4.c` | `src/semantic/effect_bindings/fxSparkPink.c` |
| `src/semantic/effect_bindings/func_8011195C.c` | `src/semantic/effect_bindings/fxExplosionB12.c` |
| `src/semantic/effect_bindings/func_801119A0.c` | `src/semantic/effect_bindings/fxExplosionB24.c` |
| `src/semantic/effect_bindings/func_801119E4.c` | `src/semantic/effect_bindings/fxExplosionB48.c` |
| `src/semantic/effect_bindings/func_80111A28.c` | `src/semantic/effect_bindings/fxExplosionB96.c` |
| `src/semantic/effect_bindings/func_80111A6C.c` | `src/semantic/effect_bindings/fxExplosionB96Fast.c` |
| `src/semantic/effect_bindings/func_80111AB0.c` | `src/semantic/effect_bindings/fxSmokePuffBlue.c` |
| `src/semantic/effect_bindings/func_80111D20.c` | `src/semantic/effect_bindings/fxFlame3.c` |
| `src/semantic/effect_bindings/func_80111D84.c` | `src/semantic/effect_bindings/fxFlameBurning.c` |
| `src/semantic/effect_bindings/func_80111DEC.c` | `src/semantic/effect_bindings/fxFlame10.c` |
| `src/semantic/effect_bindings/func_80111E54.c` | `src/semantic/effect_bindings/fxFlamePair.c` |
| `src/semantic/effect_bindings/func_80111F48.c` | `src/semantic/effect_bindings/fxGroundFlame.c` |
| `src/semantic/effect_bindings/func_80112094.c` | `src/semantic/effect_bindings/fxSmokeA6.c` |
| `src/semantic/effect_bindings/func_80112100.c` | `src/semantic/effect_bindings/fxSmokeA12.c` |
| `src/semantic/effect_bindings/func_8011216C.c` | `src/semantic/effect_bindings/fxSmokeA24.c` |
| `src/semantic/effect_bindings/func_801121D4.c` | `src/semantic/effect_bindings/fxSmokeA48.c` |
| `src/semantic/effect_bindings/func_8011223C.c` | `src/semantic/effect_bindings/fxSmokeA128.c` |
| `src/semantic/effect_bindings/func_801122A4.c` | `src/semantic/effect_bindings/fxSmokeB6.c` |
| `src/semantic/effect_bindings/func_80112310.c` | `src/semantic/effect_bindings/fxSmokeB12.c` |
| `src/semantic/effect_bindings/func_8011237C.c` | `src/semantic/effect_bindings/fxSmokeB24.c` |
| `src/semantic/effect_bindings/func_801123E4.c` | `src/semantic/effect_bindings/fxSmokeB48.c` |
| `src/semantic/effect_bindings/func_801124BC.c` | `src/semantic/effect_bindings/fxSmokeC12.c` |
| `src/semantic/effect_bindings/func_80112528.c` | `src/semantic/effect_bindings/fxSmokeC24.c` |
| `src/semantic/effect_bindings/func_80112594.c` | `src/semantic/effect_bindings/fxSmokeC48.c` |
| `src/semantic/effect_bindings/func_8011270C.c` | `src/semantic/effect_bindings/fxBulletImpact.c` |
| `src/semantic/effect_bindings/func_801127E0.c` | `src/semantic/effect_bindings/fxExplosionQuiet8.c` |
| `src/semantic/effect_bindings/func_80112928.c` | `src/semantic/effect_bindings/explodeStoreExplosionBAnimation.c` |
| `src/semantic/effect_bindings/func_80112950.c` | `src/semantic/effect_bindings/explodeStoreSmokeAAnimation.c` |
| `src/semantic/effect_bindings/func_80112978.c` | `src/semantic/effect_bindings/explodeStoreSmokeBAnimation.c` |
| `src/semantic/effect_bindings/func_801129A0.c` | `src/semantic/effect_bindings/explodeStoreSmokeCAnimation.c` |
| `src/semantic/effect_bindings/func_801129C8.c` | `src/semantic/effect_bindings/explodeStoreBulletImpactAnimation.c` |
| `src/semantic/effect_bindings/func_801129F0.c` | `src/semantic/effect_bindings/explodeStoreSparkAnimation.c` |

<!-- renamed-files:end -->
