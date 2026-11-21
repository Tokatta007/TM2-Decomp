# Starting a match from code, and detecting its end

Answers item 2 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`.
The settings struct `D_8016475C` (sessionSettings) is described in `TOURNAMENT_PASSWORDS.md`.

- **Session runner:** `screenCheckMidGameOptions` (sessionRun) (8012BE5C) is in C++ in `src/semantic/session_runner.cpp`. 8012C054 is a point inside it, not a separate function.
- **Round end:** 8011B5CC is inside the world step `sub_8011A700` (worldUpdate) (`world_update.c`).
- **A complete example:** the attract demo `shell_attract_demo.c` (80137B00..) starts a match entirely from code.

## 1. Fill the settings struct

| Field | Setter | Notes |
|---|---|---|
| +0 mode | `sub_80137948(s, m)` (settingsSetMode) | 0 attract, 1 tournament, 2 two-player tournament, 3 challenge, 4 two-player Deathmatch |
| +4 humans | `sub_80137930(s, n)` (settingsSetHumans) | |
| +5 computer opponents | `sub_80137918(s, n)` (settingsSetOpponents) | |
| +6 / +7 pick progress | `sub_80137900(s, 0)` (settingsSetCarSelectPlayer), `sub_801378E8(s, 0)` (settingsSetOpponentsPicked) | |
| +8 + i human car (0..14) | `sub_801378A4(s, i, car)` (settingsSetHumanCar) | clear all first with `sub_801378B4(s)` (settingsClearHumanCars) |
| +10 + i opponent car | `sub_80137854(s, i, car)` (settingsSetOpponentCar) | clear all first with `sub_80137864(s)` (settingsClearOpponentCars); `sub_8012CD08()` (tournamentPickOpponents) picks random opponents as in the tournament |
| +20 + i lives | `sub_80137820(s, i, n)` (settingsSetLives) | n < 7; the tournament uses 3, challenge 1 (`screen_single.c`). `sub_80137864` (settingsClearOpponentCars) sets these to 0xFF, so set lives after clearing opponents |
| +24 level / arena | `sub_80137814(s, level)` (settingsSetLevel) | 1..8 tournament arenas, 10..12 secret levels (`UNLOCKS.md`) |
| +28..+30 intro flags | (session_runner) | boss intro, Dark Tooth intro, Dark Tooth reveal; the runner clears them on exit |
| +36 difficulty | | applied by the runner via `sub_8011D5D0` (aiSetDifficulty) |
| +38 + i per-player control choice | | passed to `sub_801206F4(choice, i)` (padSetLayout) (pad map, `spawn_tables.c`) |

`sub_80129AF8()` (gameGetLevel) returns the level at +24, not the mode. This is why
`rtMainLoop`'s per-"mode" event calls are really per-arena scripted events;
`MAIN_LOOP.md` is corrected. `sub_8011EDAC()` (gameGetMode) returns the mode, which the
runner copies to `D_80180FC4` (gameMode) through `sub_8011ED88` (gameSetMode).

The attract demo does this:
1. Saves the 48 bytes.
2. Sets mode 0, the next level, 1 human (pick progress 1), a random car `rand() % 13` and 4 opponents.
3. Clears the opponent slots and calls `sub_8012CD08()` (tournamentPickOpponents).
4. Calls `sub_8012BE5C(D_8016475C)` (sessionRun) (sessionSettings) (screenCheckMidGameOptions (sessionRun)).
5. Restores the 48 bytes.

## 2. What the runner does (8012BE5C)

### Once, before the first round

1. `while (sub_80150CFC(6, 0)) sub_80150E00();` (padUpdate) (padCheckAction) waits for button 6 to be released.
2. Calls `sub_80113F30()` (fontInit).
3. Calls `SsInitHot()` for modes 0–2 unless this is the Dark Tooth stage.
4. Calls `DrawSync`, `VSync` and `screenSetEnv()`.
5. In a tournament from level 2 on, with player 1 not on Sweet Tooth or Minion, it shows the password.
6. Sets the split screen: `sub_800D1C24(0)` (viewSetSplitMode) for one player. In modes 2 and 4 it uses `sub_800D1C24(1)` (one layout) on levels 2, 3 and 8, and `(2)` (the other) elsewhere.
7. Calls `sub_801206F4(+38, 0)` (padSetLayout) and `sub_801206F4(+39, 1)`.
8. Calls `sub_8011ED88(mode)` (gameSetMode), which sets `D_80180FC4` (gameMode) and the attract flag.
9. Calls `sub_8011A268(humans)` (humanCarsSetCount).
10. Calls `sub_8011D5D0(difficulty)` (aiSetDifficulty), which sets `D_80180FB8` (aiDifficulty) and rescales the AI.
11. Calls `sub_8012BAFC(s)` (sessionLoadArenaAndCars), which loads the arena and the cars with "LOADING" on screen.
12. Calls `sub_8012BD94(s)` (sessionBindCars), which binds the humans and opponents to car slots.

### For each round, until `quit`

1. Calls `sub_80128DB4(level)` (roundStartMusic) and `sub_80112820()` (fxSpritesReset), which reset the effect objects.
2. Calls the sound object `sub_801605B4` (taskMgrStopAll) / `nullsub_14` (taskMgrErrorStub) and the fonts `sub_801142B0(6/2/0)` (fontResetColor).
3. Runs `result = rtMainLoop();`.
4. Restores the music and volume, then clears the scheduled events with `sub_800D1858()` (rtClearScheduledEvents).
5. Handles `result` per mode (lives, boss and Dark Tooth stages, wins tally). See section 3.

### After the last round

The runner calls the fonts, clears the three intro flags, calls `sub_80128F3C()` (sessionTeardown)
and returns the last `result`.

## 3. How a round ends

`rtMainLoop` runs until a scheduled event fires (`sub_800D181C(value, delay)` (rtScheduleEvent),
up to 4 events, counted down in tics) and returns that event's value. The
210 / 420-tic delays leave time to watch the explosion; 210 is used when the
dying car's "alt" flag is set.

| Value | Scheduled by | Meaning |
|---|---|---|
| 1 | `sub_8011B89C` (humanCarDamageUpdate) (`car_damage_human.c`, human car damage) when a human car dies, unless mode 4 | lost a life. The runner decrements lives (mode 1/2) and shows GAME OVER at 0 |
| 1 | `world_update.c` in attract mode when a button is pressed; `sub_8011EFDC` (demoRespawnAiCars) after 5 attract cycles | end of the demo |
| 2 | the pause menu `sub_80129528` (pauseMenu) (QUIT, "BATTLE ABORTED") | quit |
| 3 | `world_update.c` when no computer car is left (`D_80181020 <= 0` (aiCarsRemaining)), not in mode 4; `sub_8011C248` (aiCarsCountRemaining) on the Dark Tooth stage | round won |
| 3 / 4 | `sub_8011B89C` (humanCarDamageUpdate) in mode 4 (Deathmatch): human 1 died gives **3**, human 0 died gives **4** | Deathmatch round: 3 means player 1 won, 4 means player 2 won |

**Mode 4 rounds:** the runner never stops a mode 4 session on its own. It
shows the round result (`sub_8012B2D4` (screenDeathmatchResult)), updates the tally and plays another
round until the result is 2 (quit).

## 4. Recipe: the co-op sudden-death duel in Dark Tooth's arena

The duel is the two humans against each other with no computer cars, so it is
Deathmatch (mode 4) on level 8.

1. **Save the settings.** Save the 48 bytes of `D_8016475C` (sessionSettings), as the attract
   demo does, so the tournament state survives.
2. **Fill the settings.** Mode 4; humans 2; opponents 0; clear the opponent
   slots; both human cars at +8 / +9; level 8; intro flags 0.
3. **Run a one-round runner.** Write a port-side copy of the runner (section 2)
   that runs one round and returns:
   - `result == 3`: player 1 won.
   - `result == 4`: player 2 won.
   - `result == 2`: quit.

   The retail runner would keep playing rounds in mode 4.
4. **Restore.** Run the teardown (fonts, `sub_80128F3C()` (sessionTeardown)), then restore the
   saved 48 bytes.

Things to watch:
- **Stale AI record on level 8.** Level 8 runs the Dark Tooth code in
  `world_update.c` whenever computer car record 0 (`D_80188B5C[0]` (aiCars), type at
  +0x30) is Dark Tooth (0xF0). After a co-op tournament that record may
  still hold Dark Tooth. Check that the loader (`sub_8012BAFC` (sessionLoadArenaAndCars)) /
  `sub_8012BD94` (sessionBindCars) resets it with 0 opponents, or clear it before the round.
  Otherwise the duel could end with result 3 from the Dark Tooth branch.
- **Game-over check.** `D_80180FC4` (gameMode) must be 4, so that a human death schedules
  3/4 rather than the life-lost result 1. `sub_8011ED88(4)` (gameSetMode) does this.
- **Unidentified flag.** `sub_800D3850()` (audioSetDarkToothMusic) (sets `D_80180A05` (audioDarkToothMusicFlag), read by
  `sub_800D3844` (audioGetDarkToothMusic)) is called when the Dark Tooth stage starts. It is not
  identified yet; leave it unset for the duel.
