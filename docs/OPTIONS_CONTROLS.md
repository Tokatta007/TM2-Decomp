# Options, Audio and control setup: what they store

Answers items 3 (Options screen) and 4 (controls) of "Next, in this order" in
the port's `docs/DECOMP_REQUESTS.md`. All functions named here are
byte-identical C:
- `screen_options_audio.c`: Options, Audio
- `screen_controller_multiplayer.c`: controller layout
- `controls.c`: bindings
- `spawn_tables.c`: layout apply
- `menu_state_accessors.c`: setters

## Shell navigation

| Screen | Function | From / to |
|---|---|---|
| Title | `sub_8012DF84` (screenTitle) | start of the shell loop (`sub_80136F0C` (shellScreenLoop)) |
| Options | `sub_80133C08` (screenOptions) (`SHELLDB2\OPTS.TIM`, scene `SHELLDB2\OPTIONS`) | back → title |
| Audio | `sub_80135844` (screenAudio) (`SHELLDB2\AUDIO.TIM`) | from Options item 9004, back → Options |
| Controller layout | `sub_80135088` (screenControllerLayout) (`SHELLDB2\KEYPAD1..4.TIM`) | from Options item 9003 (player 1) / 9009 (player 2), then → Options |
| History | `sub_8013629C` (screenHistory) | from Options item 9005 |

## Options screen (80133C08)

**Entry sound.** On entry it temporarily sets the effects level
`D_80180E18` (audioSfxVolume) to the music volume (`sub_800D362C()` (audioGetMusicVolume), which returns
`D_801809F8` (musicVolume)). It plays sound 27 (`sub_800D3334(27, 0, 0, 99)` (soundPlay)), then
restores the effects level.

**Items:**

| Item | Label | Left/right | Stored in |
|---|---|---|---|
| 9001 | 9006 | ±1 | settings +36 **difficulty** (0 easy, 1 medium, 2 hard): read with `sub_801377D8` (settingsGetDifficulty), written with `sub_801377E4` (settingsSetDifficulty) |
| 9002 | 9007 | ±1 | settings +37: read with `sub_801377C0` (settingsGetWorldMap), written with `sub_801377CC` (settingsSetWorldMap). The only other reader skips the `SHELLDB1\WORLD` globe screen between levels (`sub_801334E8` (screenWorldGlobe)) when it is 0. So it is most likely the "show world map" option |
| 9003 | 9008 | ±1, or opens layout screen | settings +38 **player 1 controller layout** (0..3): `sub_80137798(s, 0)` (settingsGetControlLayout) / `sub_801377AC(s, 0, v)` (settingsSetControlLayout) |
| 9009 | 9010 | ±1, or opens layout screen | settings +39 **player 2 controller layout**: `sub_80137798(s, 1)` (settingsGetControlLayout) / `sub_801377AC(s, 1, v)` (settingsSetControlLayout) |
| 9004 | | | opens Audio |
| 9005 | | | opens History |

**How values move.** The labels are filled from the settings before the
scene runs. Afterwards each one is read back with `sub_801220B0` (sceneObjGetChoiceState) and stored.
`D_80180D1C` (controlsLayoutPlayer) (gp+840) is set to 0 when item 9003 was chosen and 1 otherwise.
This tells the layout screen which player it is editing.

## Audio screen (80135844)

**Rows.** Two sliders: music (9001) and effects (9002), each with label 0x232D and slider 0x232E (`sub_8012D844` (menuSliderCreate)).

**Starting values:**
- It plays CD audio through the music object `D_80180BCC` (musicManagerObject) (vtable slot 2, argument 11).
- It sets the music volume `D_80180E0C` (audioCdVolume) to `sub_800D362C()` (audioGetMusicVolume), clamped to [`D_80180E28` (audioVolumeClampMin), `D_80180E24` (audioVolumeClampMax)], and applies it with `SsSetSerialVol`.
- The music slider starts at `(D_80180E0C + 1) / 8` (audioCdVolume); the effects slider at `D_80180E18 / 7` (audioSfxVolume).

**Applying a change.** Each slider change runs an apply action (`80136FF8`):
- **Music:** `D_80180E0C = slider * 8` (audioCdVolume), capped at `D_80180E14` (audioMusicVolumeCap), then clamped. It is applied with `SsSetSerialVol` and saved to `D_801809F8` (musicVolume) through `sub_800D3620` (audioSetMusicVolume).
- **Effects:** `D_80180E18 = slider * 7` (audioSfxVolume), capped at `D_80180E20` (audioSfxVolumeCap), then clamped. It plays sound 24 as a preview.

**On exit.** It stops the CD audio (vtable slot 3, argument 11) and sets the CD volume to 0.

So the persistent audio settings are `D_801809F8` (musicVolume) (music) and
`D_80180E18` (audioSfxVolume) (effects). They are not part of the 48-byte settings struct.

## Controller layout screen (80135088)

It edits the layout of player `D_80180D1C` (controlsLayoutPlayer). Left/right cycles through the four
`KEYPAD1..4.TIM` pictures. The choice is stored with
`sub_801377AC(s, D_80180D1C, 0..3)` (controlsLayoutPlayer) (settingsSetControlLayout) (settings +38 / +39), and the screen
returns to Options.

## From layout to action bindings (controls)

The shell never edits the binding tables directly. A layout (0..3) is turned
into bindings when a race starts:

1. `screenCheckMidGameOptions` (sessionRun) (8012BE5C) calls `sub_801206F4(settings[38], 0)` (padSetLayout) and `sub_801206F4(settings[39], 1)`.
2. `sub_801206F4(layout, port)` (padSetLayout) (`spawn_tables.c`):
   1. stores the layout in `D_80180BA4[port]` (padLayout);
   2. for each of the 43 rows of `D_80162F64[43][5]` (padLayoutTable) (`{action, button for layout 0, 1, 2, 3}`), calls `sub_80150F70(action, button, port)` (padBindAction), which writes the configured table `D_801A4CEC` (padConfiguredBindings);
   3. calls `sub_80150FC0(port)` (padActivateBindings), which copies `D_801A4CEC[port]` (padConfiguredBindings) (128 bytes) into the active table `D_801A4BEC` (padActiveBindings).
3. During play, `sub_80150CFC(action, port)` (padCheckAction) / `sub_801510DC` (padActionHeld) look up `D_801A4BEC` (padActiveBindings) and ask the pad decoder (`sub_8014A57C` (padButtonHeld)) whether the bound button is held. Button numbers 1..14 are: 1 Select, 2 Start, 3 L1, 4 L2, 5 Up, 6 Down, 7 Left, 8 Right, 9 R1, 10 R2, 11 Triangle, 12 Cross, 13 Square, 14 Circle.

The menus read the same tables (see `UNLOCKS.md`, "how the menus read the
pad"), so in-menu actions follow the layout too.

**For a rebinding page:** write the port's own button for each action with
`sub_80150F70(action, button, port)` (padBindAction) (or into `D_801A4CEC` (padConfiguredBindings) directly), then
call `sub_80150FC0(port)` (padActivateBindings). The 43-row table `D_80162F64` (padLayoutTable) lists every action
the game uses and the four stock layouts. That gives the page its rows and
its "reset to layout N" presets.
