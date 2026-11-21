# Tournament state, passwords and saves

Answers the "tournament state, password and memory-card save code" item of
the PC port's `docs/DECOMP_REQUESTS.md` (section 3). All functions named
here are byte-identical C in `src/semantic` unless marked otherwise.

## No memory-card saves

The retail executable has no memory-card code: no `bu00:` / `bu10:` device
paths, no save-file product code, no card BIOS calls. Tournament progress
is carried only by passwords. Mid-tournament saves and profiles in the port
are new features; the state to store is the settings struct below.

## The tournament state: settings struct `D_8016475C` (sessionSettings) (48 bytes)

Everything the shell and the session runner need lives in one struct; the
attract mode saves and restores it as a 48-byte block
(`shell_attract_demo.c`, `sub_80137954` (settingsCopy)). Accessors:
`menu_state_accessors.c` (8013775C..80137948), `game_setting_accessors.c`
(8012CB84..8012CC04), `game_settings_extra.c`.

| Offset | Type | Meaning | Set by |
|---|---|---|---|
| +0 | s32 | game mode: 0 attract/demo, 1 tournament, 3 challenge | single-player menu, attract cycle |
| +4 | u8 | number of human players | menus |
| +5 | u8 | number of computer opponents in the arena | tournament: `sub_8012AFE8(level)` (tournamentOpponentCount) minus +4; 1 for the Dark Tooth stage |
| +6 | u8 | humans who have picked a car (car select progress) | car select |
| +7 | u8 | opponents picked (AI select progress) | AI select |
| +8 + i | s8 | car-select index of human player i (0..14, see below) | car select, password |
| +10 + i | s8 | car-select index of opponent slot i (-1 = empty) | AI select, random options |
| +20 + i | u8 | per-player byte (read by `sub_8012CB9C` (sessionGetLives)) | |
| +24 | s32 | level / arena (tournament 1..8) | menus, password |
| +29 | u8 | Dark Tooth stage of level 8 (also the password flag bit) | password, tournament |
| +36 | u8 | difficulty: 0 Easy, 1 Medium, 2 Hard (applied with `sub_8011D5D0` (aiSetDifficulty), which sets `D_80180FB8` (aiDifficulty) and rescales every AI car) | options, password rule below |
| +37, +38 + i | u8 | other menu state (not needed by the tournament) | |

Car-select index to car type (`sub_8012B044` (carIndexToType), `tournament_tables.c`): 0 Hammerhead (0x20),
1 Outlaw 2, 2 Warthog, 3 Mr. Grimm, 4 Grasshopper, 5 Thumper, 6 Spectre,
7 Roadkill, 8 Twister, 9 Axel, 10 Mr. Slam, 11 Shadow (0xD0), 12 Sweet
Tooth (0x10), 13 Minion (0xE0), 14 Dark Tooth (0xF0).

Opponents per tournament level (`sub_8012AFE8` (tournamentOpponentCount), `tournament_tables.c`): levels 1, 3, 4, 5: 7; level 2: 6; level 6: 8; level 7: 10;
level 8: 9 (then 1, Dark Tooth, when +29 is set).

## Password format

A password is 6 symbols, each one of 5 buttons. It encodes a 16-bit code
(`sub_8012CE04` (passwordEncode) encodes, `sub_8012CE7C` (passwordDecode) decodes, `password_codec.c`):

| Bits | Contents |
|---|---|
| 0 | always 0 |
| 1..3 | level - 1 (valid levels 2..8) |
| 4..7 | car-select index + 4 (valid indices 0..11: the 12 regular cars) |
| 8 | always 0 |
| 9 | Dark Tooth stage flag (only valid with level 8) |
| 10..13 | check value: (car index + 1) x (level - 1) mod 13 |

Showing a password (`sub_801292A4` (passwordBuildString), in `session_runner.cpp`): the code from
the current level (+24), player 1's car index (+8) and the flag (+29) is
written in base 5 (`sub_8012CF94(code, 10, 5)` (passwordRebase)) as six digits (`"%06d"`),
and each digit becomes a font glyph: 0 `_`, 1 `<`, 2 `@`, 3 `>`, 4 `=` (the
game font draws these as the controller button symbols). The largest valid
code (13054) needs 6 base-5 digits.

Entering a password (`screenVehicleChoice` (screenSinglePlayerMenu) 8012E908, `screen_single.c`):
the keypad digits (`sub_80137690` (menuKeypadGetValue)) are read back as base 5
(`sub_8012CF94(value, 5, 10)` (passwordRebase)) and decoded. A valid code sets tournament
mode (+0 = 1), the level (+24), player 1's car (+8), the level-8 flag
(+29) and the opponent count (+5), then starts the tournament; an invalid
one prints "invalid password" and returns to the menu.

What a password does not carry: the difficulty, the number of lives or any
progress inside a level. Easy only covers the first four levels: the
tournament's level advance (`screenScrollText` 80136BB0,
`menu_scroll_text.c`) ends an Easy tournament after level 4 with the
`SHELLDB2\STOP.TIM` screen, and entering a password for level 5 or later
on Easy raises the difficulty to Medium ("difficulty set to medium for
this level"). Medium and Hard are kept.

## For the port

- A mid-tournament save is the 48-byte settings struct (it already holds the
  difficulty at +36) plus whatever the port adds (profile, unlocks).
- Restoring it and running the tournament path is enough to resume at the
  start of a level, exactly like a password does.
- Passwords stay compatible if the port keeps `sub_8012CE04` (passwordEncode) /
  `sub_8012CE7C` (passwordDecode); the 12-car limit (4 bits, index + 4) is why Sweet Tooth,
  Minion and Dark Tooth cannot appear in a password.
