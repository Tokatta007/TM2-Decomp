# Renames: what changed for the PC port

For the PC port team. The decomp now uses readable names for most
functions and globals, and its source files are named after what they
hold. This page says what was renamed, what did not change, and how to
bring the port's notes up to date. Nothing in the game changed: the
images built from the C are the same bytes as before.

## What was renamed

**Symbols.** `config/US/names.tsv` lists every renamed function and global,
2,100+ rows with four columns:

| Column | What it holds |
|---|---|
| `va` | The retail address (`SCUS_94306R.EXE`). |
| `old` | The name before: `sub_X` / `D_X`, or the IDA name (`update_bullets`, `screenVehicleChoice`). |
| `new` | The name the C uses now (`bulletsUpdate`, `screenSinglePlayerMenu`). |
| `evidence` | Where the purpose was confirmed (a doc, a caller, a string). |

Names IDA already had and that were right (`rtMainLoop`, `viewProc`,
`UpdateGuns`, `uaInit`, ...) were kept and have no row.

**Files.** `config/US/file_renames.tsv` lists every renamed source file,
118 rows: `old_path`, `new_path`, `reason`. The batch names
(`game_code10.c`, `small_helpers8.c`, `mid_helpers4.c`, `lib_r152.c`,
`func_8010FD44.c`, `game_movie_*.c`, ...) are gone. A file that holds
unrelated functions is named after its main content and was not split, so
some functions still sit in a file named after something else. The table at
the end of `docs/CODE_MAP.md` maps old to new, and its section 3 says what
else each mixed file holds. The authority for "which file is VA X in" is
still `config/semantic/selection-template.json` (`functions[VA].source`).

## What did not change

- **Addresses.** VAs never change. They are the stable key: every name, old
  or new, maps to one VA. Keep the VA next to the name in the port's notes
  (`carDestroyed` (8011BE10)), then a later rename costs nothing.
- **`config/US/ida_names.tsv`** and `ida_funcs.tsv` still hold the IDA
  names. They are never edited, so tools that look names up there (call
  traces, replay hooks) work as before.
- Code and data bytes, section names (`.text_<VA>`, `.tm2_<VA>`), the build
  hashes, and the struct layouts in `include/semantic/`.

## Updating the port's notes

`tools/port_rename_notes.py` rewrites Markdown and text files to the new
names. It is a single script that needs only Python 3 and the two TSV
files above; copy all three into the port repository if that is easier.

```sh
py -3 tools/port_rename_notes.py --dry-run ../port/docs     # list the changes
py -3 tools/port_rename_notes.py ../port/docs              # rewrite in place
py -3 tools/port_rename_notes.py --annotate notes.md                    # write "new (old)"
py -3 port_rename_notes.py --names names.tsv --files file_renames.tsv notes/   # copied elsewhere
```

What it changes, as whole words:

- `sub_X`, `func_X`, `D_X` and Ghidra's `FUN_X` / `DAT_X` of a renamed
  symbol become the new name. The hex may be upper or lower case.
- The old IDA names of `names.tsv` become the new names.
- Old file paths, file names and bare file stems become the new ones
  (`src/semantic/game_code10.c`, `game_code10.c`, `game_code10`;
  backslash paths too).
- Text in the decomp docs' style, `` `sub_8011BE10` (carDestroyed) ``,
  becomes `` `carDestroyed` ``.

It leaves addresses (`8011BE10`, `0x8011BE10`), names that are not in the
tables, and `reference/m2c/func_X.c` paths alone. `--dry-run` prints each
change with its line number and writes nothing. `--annotate` keeps the old
name in brackets after the new one, so a search for the old name still
finds the line. Line endings are kept.

A dry run over the port's `docs/` on 2026-10-06 found 20 changes in 5 files
(`DECOMP_REQUESTS.md`, `OPTIONS.md`, `PROFILES.md`, `RECOMP_INVENTORY.md`,
`RUNTIME.md`). Nothing was written there.

For code in the port that names decomp symbols (hooks, patch tables), look
the old name up in `names.tsv` and use the new one, or better, key the hook
by VA. `config/US/signatures.tsv` gives each hooked VA's prototype (return
type, parameters, varargs, struct definitions); see `docs/SIGNATURES.md`.

## The renames that matter most for the port

From `names.tsv`; the file is the one that holds the function now.
"global" marks a variable.

### Session runner and match flow

| VA | Old name | New name | File |
|---|---|---|---|
| 8012BE5C | `sub_8012BE5C` | `sessionRun` | `session_runner.cpp` |
| 8012BAFC | `sub_8012BAFC` | `sessionLoadArenaAndCars` | `shell_main.c` |
| 8012BD94 | `sub_8012BD94` | `sessionBindCars` | `screen_card_layout.c` |
| 8011A024 | `sub_8011A024` | `roundStart` | `scene_init.c` |
| 80128F3C | `sub_80128F3C` | `sessionTeardown` | `misc_one_liners.c` |
| 8011E214 | `sub_8011E214` | `roundIsWon` | `global_accessors.c` |
| 8011EDB8 | `sub_8011EDB8` | `roundIsOver` | `global_accessors.c` |
| 8012940C | `sub_8012940C` | `screenLostALife` | `car_parts_spin.c` |

IDA called the session runner `screenCheckMidGameOptions`; its row has
`old` = `sub_8012BE5C`.

### Shell menus and car select

| VA | Old name | New name | File |
|---|---|---|---|
| 8012E908 | `screenVehicleChoice` | `screenSinglePlayerMenu` | `screen_single.c` |
| 801307EC | `sub_801307EC` | `screenCarSelect` | `screen_car_select.c` |
| 801334E8 | `sub_801334E8` | `screenWorldGlobe` | `screen_controller_multiplayer.c` |
| 8016303C | `D_8016303C` | `shellScene` | global |
| 800CFF14 | `D_800CFF14` | `carSelectCodeTable` | global |

`screenVehicleChoice` was misleading: it is the single-player mode menu.
The real car select is `screenCarSelect`.

### Pause menu

| VA | Old name | New name | File |
|---|---|---|---|
| 80129528 | `sub_80129528` | `pauseMenu` | `screen_pause_menu.c` |
| 800CF66C | `D_800CF66C` | `pauseMenuLabels` | global |

### Damage, death and respawn hooks

| VA | Old name | New name | File |
|---|---|---|---|
| 8011B89C | `sub_8011B89C` | `humanCarDamageUpdate` | `car_damage_human.c` |
| 8011BBD4 | `sub_8011BBD4` | `aiCarDamageUpdate` | `world_collapse_warps.c` |
| 8011BE10 | `sub_8011BE10` | `carDestroyed` | `car_destroyed.c` |
| 800F4B74 | `sub_800F4B74` | `carDamageByObject` | `weapon_special_missiles.c` |
| 800FD550 | `sub_800FD550` | `carRestart` | `car_restart.c` |
| 800FD6C4 | `sub_800FD6C4` | `humanCarPlaceAtStart` | `car_object_contact.c` |

### Settings accessors

| VA | Old name | New name | File |
|---|---|---|---|
| 8016475C | `D_8016475C` | `sessionSettings` | global |
| 8012A940 | `sub_8012A940` | `settingsInitDefaults` | `arena_load_register.c` |
| 8012CB90 | `sub_8012CB90` | `sessionGetLevel` | `game_setting_accessors.c` |
| 8012CB9C | `sub_8012CB9C` | `sessionGetLives` | `game_setting_accessors.c` |
| 8012CBD4 | `sub_8012CBD4` | `sessionGetHumanCar` | `game_setting_accessors.c` |
| 8012CC04 | `sub_8012CC04` | `sessionGetMode` | `game_setting_accessors.c` |
| 80137808 | `sub_80137808` | `settingsGetLevel` | `menu_state_accessors.c` |
| 80137820 | `sub_80137820` | `settingsSetLives` | `game_settings_extra.c` |
| 8013788C | `sub_8013788C` | `settingsGetHumanCar` | `game_settings_extra.c` |
| 801378A4 | `sub_801378A4` | `settingsSetHumanCar` | `menu_state_accessors.c` |
| 80137924 | `sub_80137924` | `settingsGetHumans` | `menu_state_accessors.c` |
| 80137930 | `sub_80137930` | `settingsSetHumans` | `menu_state_accessors.c` |
| 8013793C | `sub_8013793C` | `settingsGetMode` | `menu_state_accessors.c` |
| 80137948 | `sub_80137948` | `settingsSetMode` | `menu_state_accessors.c` |
| 80137954 | `sub_80137954` | `settingsCopy` | `menu_state_copy.cpp` |

### View and camera

| VA | Old name | New name | File |
|---|---|---|---|
| 800D1C18 | `sub_800D1C18` | `viewGetSplitMode` | `rt_main.c` |
| 800D1C24 | `sub_800D1C24` | `viewSetSplitMode` | `rt_main.c` |
| 80102BE8 | `sub_80102BE8` | `viewCarCameraUpdate` | `camera.c` |
| 80102CC4 | `sub_80102CC4` | `viewChaseCamera` | `camera.c` |
| 8011D388 | `sub_8011D388` | `viewSetCameraPose` | `car_records.c` |
| 8011D4EC | `sub_8011D4EC` | `viewGetCameraObject` | `game_misc_accessors.c` |
| 80155DB8 | `sub_80155DB8` | `viewSetFov` | `engine_misc_accessors.c` |
| 8011DAF8 | `sub_8011DAF8` | `viewWatchNextAiCar` | `view_watch_ai.c` |
| 80180FA8 | `D_80180FA8` | `viewMainPlayer` | global |
| 80181110 | `D_80181110` | `viewCameraObjects` | global |
| 80181118 | `D_80181118` | `viewEnvs` | global |
| 80181120 | `D_80181120` | `viewModes` | global |

### Pad and controls

| VA | Old name | New name | File |
|---|---|---|---|
| 8014A2A4 | `sub_8014A2A4` | `padStart` | `engine_clut_anim.c` |
| 8014A31C | `sub_8014A31C` | `padDecode` | `shared_pad.c` |
| 8014A4F0 | `sub_8014A4F0` | `padGetLatched` | `engine_misc_accessors.c` |
| 8014A57C | `sub_8014A57C` | `padButtonHeld` | `shared_pad.c` |
| 80150E00 | `sub_80150E00` | `padUpdate` | `controls.c` |
| 80150CFC | `sub_80150CFC` | `padCheckAction` | `controls.c` |
| 80150F70 | `sub_80150F70` | `padBindAction` | `controls.c` |
| 80150FC0 | `sub_80150FC0` | `padActivateBindings` | `controls.c` |
| 80151138 | `sub_80151138` | `padLatchWithRepeat` | `controls.c` |
| 80120664 | `sub_80120664` | `padLoadLayout` | `spawn_tables.c` |
| 801206F4 | `sub_801206F4` | `padSetLayout` | `spawn_tables.c` |
| 801A4BEC | `D_801A4BEC` | `padActiveBindings` | global |
| 801A4CEC | `D_801A4CEC` | `padConfiguredBindings` | global |
| 80162F64 | `D_80162F64` | `padLayoutTable` | global |

### HUD

| VA | Old name | New name | File |
|---|---|---|---|
| 800F8370 | `sub_800F8370` | `hudDrawSingleView` | `hud_main.c` |
| 800F9084 | `sub_800F9084` | `hudDrawSplitVertical` | `hud_split.cpp` |
| 800F9D48 | `sub_800F9D48` | `hudDrawSplitHorizontal` | `hud_split.cpp` |
| 800FB0B0 | `sub_800FB0B0` | `hudInitPanels` | `hud_init.c` |
| 800FBBC4 | `sub_800FBBC4` | `hudMessagePost` | `audio_hud_helpers.c` |
| 800FBE68 | `sub_800FBE68` | `hudDisplayRadar` | `hud_radar.c` |
| 800FD0D0 | `sub_800FD0D0` | `hudDrawTargetPanel` | `car_destroyed.c` |
| 8014E194 | `sub_8014E194` | `hudDrawNumber` | `packet_digits.c` |

### Timing

| VA | Old name | New name | File |
|---|---|---|---|
| 8015531C | `sub_8015531C` | `timerGetFrameTicks` | `timer_frame.c` |

`docs/CODE_MAP.md` has the full subsystem map with every important function
and global, and where each one lives.
