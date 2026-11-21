# Between tournament levels, and after the last life

Answers item 8 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`.
The settings struct `D_8016475C` (sessionSettings) is described in `TOURNAMENT_PASSWORDS.md`
and `MATCH_START.md`. Addresses are retail (`SCUS_94306R.EXE`). Every
function named here is C in `src/semantic`.

Tags: CONFIRMED = read in the C and checked against the retail
disassembly; LIKELY = follows from the code, not traced to the end.

## 1. The screen loop of a tournament

Shell screens are frame handlers: each returns the next handler. A
tournament level is this loop (modes 1 and 2):

| Step | Function | What it does |
|---|---|---|
| 1 | `sub_801334C0` (screenPickOpponents) (`screen_pick_opponents.c`) | `sub_8012CD08()` (tournamentPickOpponents): random opponents into slots +10.., or Dark Tooth (14) in slot 0 when +29 is set. Returns the globe |
| 2 | `sub_801334E8` (screenWorldGlobe) (`screen_controller_multiplayer.c`) | the globe (skipped when Map, +37, is 0, or level >= 9). Returns `screenScrollText` |
| 3 | `screenScrollText` 80136BB0 (`menu_scroll_text.c`) | calls the session runner, then picks the next screen from its result |
| 4 | `screenCheckMidGameOptions` (sessionRun) 8012BE5C (`session_runner.cpp`) | the whole level: password on the loading screen, load, every round, the boss stages, lives, the end text |

The entry points into step 1 are car select (`screen_car_select.c`), the
password item (`screen_single.c`) and `screenScrollText` itself. CONFIRMED.

There is no separate password screen. From level 2 on (mode 1 only, player
1 not Sweet Tooth or Minion, not the Dark Tooth stage) the runner prints the
password at the bottom of the "LOADING" screen of the next level
(`sub_801292A4` (passwordBuildString) at 8012BF70). The pause menu shows it too (80129724). CONFIRMED.

## 2. Level won

### Inside the runner (8012BE5C)

`rtMainLoop` (call at 8012C0F8) returns 3 when no computer car is left
(`MATCH_START.md` section 3). The runner then:

| Level | What happens | Retail |
|---|---|---|
| 1-3, 5-7 | "CONGRATULATIONS! / Prepare for / *next level name*", wait 300 frames, up to 600 or a button (`sub_801290CC` (padWaitConfirmOrTimeout)). Weapon flags 0 (kept). Session ends, result 3 | text 8012C6DC |
| 4, first win | Minion intro text. Opponent slot 0 = 13 (Minion), opponents +5 = 1, reload (`sub_8012BAFC` (sessionLoadArenaAndCars), `sub_8012BD94` (sessionBindCars)), next round in the same session. Skipped when a human drives Minion | 8012C3A8, reload 8012C3F8 |
| 4, Minion beaten | Medium/Hard: "CONGRATULATIONS! / Prepare for / New York", weapon flags 0. Easy: no text, weapon flags 1. Session ends, result 3 | 8012C48C |
| 8, first win | Dark Tooth intro text. Slot 0 = 14, +5 = 1, reload, `sub_800D3850()` (audioSetDarkToothMusic) | 8012C534, reload 8012C584 |
| 8, Dark Tooth head beaten | "But wait..." text, another round, no reload | 8012C600 |
| 8, last win | "CONGRATULATIONS! / You are the winner...", weapon flags 1. Session ends, result 3 | 8012C664 |

On exit the runner clears the intro flags +28, +29, +30 and calls
`sub_80128F3C()` (sessionTeardown) (8012C89C). CONFIRMED.

### After the runner (`screenScrollText`)

| Case | Retail | Next screen |
|---|---|---|
| Easy and level 4 | 80136C88 | `SHELLDB2\STOP.TIM`, wait 300..600 frames, title `sub_8012DF84` (screenTitle) |
| level < 8 | 80136CBC | level +24 = level + 1; opponents +5 = `sub_8012AFE8(level)` (tournamentOpponentCount) - humans (+4); returns `sub_801334C0` (screenPickOpponents) |
| level 8, mode 1 | 80136D20 | `sub_80136D98` (screenEnding), the ending (`UNLOCKS.md`) |
| level 8, mode 2 | 80136D30 | `SHELLDB2\COWIN.TIM`, wait, title |

### The hook: 80136CFC

**The one place for CONTINUE / SAVE / QUIT is in `screenScrollText` at
80136CFC**: after `sub_80137918` (settingsSetOpponents) (jal at 80136CF4) has stored the next
level's opponent count, and before `return sub_801334C0` (screenPickOpponents) (80136CFC..80136D04).
Every won level 1-7 (1-3 on Easy), in 1 and 2 players, passes here and
nowhere else. CONFIRMED.

State valid at 80136CFC:

| State | Value |
|---|---|
| +0 mode | 1 or 2 |
| +4 humans, +8 / +9 cars | unchanged |
| +24 level | **the next level** (2..8) |
| +5 opponents | the next level's count (table minus humans) |
| +10.. opponent slots | stale: the level just won (or Minion in slot 0 after level 4). `sub_801334C0` (screenPickOpponents) re-picks them |
| +20 lives | the lives left (+20 is the only counter, see section 4) |
| +28..+30 intro flags | 0 |
| +36 difficulty, +37 Map, +38 / +39 controls | unchanged |
| `D_80180FA0[0..1]` (carResetWeapons) weapon flags | 0: the human cars keep their pickup weapons into the next level |
| screen | the last frame of the end text ("Prepare for ..."); the shell scene is not set up yet, so the port draws its own menu |

What each choice should do there:
- **CONTINUE:** return `sub_801334C0` (screenPickOpponents) (retail).
- **SAVE:** store the settings (and, if wanted, lives and weapons, see
  section 3), then show the menu again.
- **QUIT:** do what retail does on a quit: level +24 = 1,
  `sub_80119834(1, 0)` (carSetResetWeapons) and `sub_80119834(1, 1)` (weapon flags 1, so the
  next tournament starts with fresh weapons), return `sub_8012DF84` (screenTitle).

## 3. Game over, and restarting a level

### Losing a life

A human car's death schedules result 1 (`sub_8011B89C` (humanCarDamageUpdate), `car_damage_human.c`).
The runner decrements +20 (8012C260..8012C26C). If lives remain, it sets
the dead player's weapon flag and loops: the next round starts at once in
the same arena. No reload, no new opponents, no text. Round start
(`sub_80128DB4` (roundStartMusic) -> `sub_8011A024` (roundStart) -> `sub_8011A198` (humanCarsRestart)) restarts each human
car on its start node with full health; the dead car loses its weapons
(flag 1), a surviving co-op partner keeps them (flag 0). CONFIRMED. The
computer cars are not reset by the runner, so those already destroyed stay
out. LIKELY.

### The last life

When +20 reaches 0: "GAME OVER" (font 6, at 159, 80) over the darkened
last frame (`sub_8012B5FC` (screenShowMessageLines) at 8012C2B4, `screenLostALife` 8012940C). There is
no wait: the session ends with result 1 and weapon flags 1, 1.
`screenScrollText` sets level +24 = 1 and returns the title screen
`sub_8012DF84` (screenTitle) (80136C38..80136C48). The text stays up while the title
loads. LIKELY.

The pause menu's QUIT goes the same way (result 2, 80136C20).

**Hook for RESTART / QUIT:** `screenScrollText` right after the runner
returns (80136BE4, result in `v0`), when the result is 1 and the mode is 1
or 2. Result 2 there is a quit from the pause menu, not a game over. State
at that point: +24 still holds the lost level, +20 = 0, +5 and the slots
hold the stage that was lost (1 and Minion/Dark Tooth if lost at a boss),
intro flags 0, weapon flags 1, 1. CONFIRMED.

### What a restart needs

Setting the struct the password way and returning `sub_801334C0` (screenPickOpponents) is
enough, if all of these are set (the password path in `screen_single.c`
does the same):

| Field | Set to | Why |
|---|---|---|
| +4 humans, +8 / +9 cars | as before | |
| +6, +7 pick progress | 0 | |
| +10.. opponent slots | `sub_80137864(s)` (settingsClearOpponentCars) (0xFF) | `sub_8012CD08` (tournamentPickOpponents) refuses cars already in the slots; stale picks steer the draw. Also clears +20..+22, so set lives after it |
| +5 opponents | `sub_8012AFE8(level)` (tournamentOpponentCount) - humans | it is 1 after a boss stage |
| +20 lives | 3 (1 player), 6 (co-op, also +21) | it is 0 after a game over |
| +24 level | the level | |
| +29 | 0 (level start; bosses included) | |
| +0 mode | 1 or 2 | |
| `D_80180FA0` (carResetWeapons) | already 1, 1 | fresh weapons, as at a tournament start |

Nothing else needs resetting. The loader resets the arena on each load
(`sub_8012BAFC` (sessionLoadArenaAndCars) -> `sub_80128CE4` (arenaResetSystems) -> `sub_80119848` (carTablesClear) sets `D_80181018` (arenaLoadedId) = -1,
so `sub_8011995C` (arenaSetup) redoes the arena setup), health is set at each round
start, and the opponents are picked again. CONFIRMED.

### Everything carried from one level to the next

| State | Where | Carried how | Notes |
|---|---|---|---|
| lives | settings +20 | never reset between levels | CONFIRMED |
| pickup weapons | human car record `D_80187B10[i]` (humanCars) weapons block +0x7C8 (counts at +0x10..+0x20) | weapon flags `D_80180FA0[i]` (carResetWeapons) = 0 after a won level, so `sub_800FD550` (carRestart) skips the block reset (`sub_800FDCAC` (weaponBlockInit)) | specials (+0x0E) are set to 2 and +0x30 to 1000 anyway. CONFIRMED |
| car, humans, difficulty, Map, controls | settings | unchanged | CONFIRMED |
| opponents | settings +10.. | the old picks only exclude cars from the new draw | CONFIRMED |
| level 8 boss stage | +29 | cleared at runner exit | CONFIRMED |

Not carried: health (refilled at each round start, `sub_800FD6C4` (humanCarPlaceAtStart)), the
arena, the opponents' state. There is no score. The two per-player counters
`D_80180FD8` / `D_80180FE0` are zeroed only at boot (`uaInit`), but nothing
calls their accessors (8011E078..8011E0F8). LIKELY unused.

So a save made at 80136CFC and loaded the password way gives 3 lives and no
pickup weapons, where CONTINUE would have kept both. To keep them, save
+20 and the weapon counts; on load set +20 after `sub_80137864` (settingsClearOpponentCars), write the
counts into `D_80187B10[i]` (humanCars) +0x7C8 +0x10.. and set `D_80180FA0[i]` (carResetWeapons) = 0
before returning `sub_801334C0` (screenPickOpponents). LIKELY (not traced through every loader
init).

## 4. Lives

| Where | Value | Function |
|---|---|---|
| 1-player TOURNAMENT | +20 = 3 | `screenVehicleChoice` (screenSinglePlayerMenu) 8012E908 (`screen_single.c`) |
| password | +20 = 3 | same function |
| CHALLENGE MATCH | +20 = 1 | same function |
| 2-player co-op tournament | +20 = +21 = 6 | `sub_8012F5F8` (screenMultiPlayer) (`screen_controller_multiplayer.c`) |
| 2-player Deathmatch | +20 = +21 = 1 | `sub_8012F5F8` (screenMultiPlayer) |

`sub_80137820` (settingsSetLives) clamps to 6. Its only callers are these two menus. The
runner decrements +20 only. Nothing else writes it, so lives carry across
all levels of a tournament, and a password restarts them at 3. CONFIRMED.

## 5. Quit from the pause menu

The pause menu `sub_80129528` (pauseMenu) (`screen_pause_menu.c`): QUIT prints "BATTLE
ABORTED", waits 60 frames, schedules `sub_800D181C(2, 0)` (rtScheduleEvent) (80129A24). The
runner sets weapon flags 1, 1 and ends with result 2. `screenScrollText`
sets level +24 = 1 and returns the **title screen** `sub_8012DF84` (screenTitle)
(`SHELLDB1\TITLE`). Same for Challenge and Deathmatch (their runner exit
also leads to the title). CONFIRMED.

## 6. 2-player co-op tournament (mode 2)

Same loop, same hooks. Differences:

| What | Co-op | Where |
|---|---|---|
| lives | one shared pool of 6 at +20: either player's death costs one, and the round restarts for both | runner; HUD shows +20 on player 1's view only (`hud_split.cpp`) |
| opponents | the level's table minus 2 | `sub_8012F5F8` (screenMultiPlayer), `screenScrollText` |
| password | not shown (loading screen or pause) | mode 1 only |
| split screen | `sub_800D1C24(1)` (viewSetSplitMode) on levels 2, 3, 8, else `(2)` | runner |
| Minion stage | skipped if either human drives Minion | runner |
| pause | either pad | `sub_80129528` (pauseMenu) |
| Easy | also stops after level 4 (`STOP.TIM`) | `screenScrollText` |
| end | `COWIN.TIM`, then the title (no ending movie) | 80136D30 |

CONFIRMED.
