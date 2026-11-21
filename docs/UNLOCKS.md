# Unlocks, secret levels and winner movies

Answers the three "profiles" items of the PC port's `docs/DECOMP_REQUESTS.md`
(section 3, added 2026-10-04). Traced in retail code (IDA pseudo-C
cross-checked with the disassembly). gp = 0x801809D4.

Short version for profiles: the game itself remembers almost nothing.
Sweet Tooth / Minion stay unlocked in RAM until power-off, secret levels
must be re-entered every time, and won endings are not recorded at all.
Everything a profile stores here is new state for the port.

## Button codes: how the menus read them

Menu screens see the pad as **menu events**, not raw buttons.
`sub_80123430` (sceneCmdPadInputRun) turns one held action per frame into an event (first match
wins: Right, Left, Up, Down, Start, Cross, ...). Actions are looked up
through each port's binding table (`D_801A4BEC` (padActiveBindings), defaults from
`D_8017EBCC` (padDefaultBindings)), so the codes follow the action bindings, not physical
buttons (not checked whether the control-setup screen can rebind them).

| Event | Default button | Event | Default button |
|---|---|---|---|
| 3 | Start | 10 | Right |
| 5 | L1 | 11 | R1 |
| 6 | L2 | 12 | R2 |
| 7 | Up | 13 | Triangle |
| 8 | Down | 14 | Cross |
| 9 | Left | 15 / 16 | Square / Circle |

**Code detector object** (built by `sub_8012DB10(obj, scene, port, table, 2000)`,
run each frame by `sub_8012DCA4` (menuCodeDetectorRun), matcher `sub_8012DB94`). The object is named "camera" in `screen_car_select.c`, which is wrong. How it works:

- **Codes:** it holds 3 codes of 4 events each.
- **Matching:** a matching press advances that code; any other press resets it.
- **Timer:** each match reloads a 60-frame timer. When the timer expires, all progress is cleared.
- **Firing:** the 4th match posts event 17/18/19 (code 1/2/3).
- **Idle:** after 2000 frames without input it posts its "done" event.
- **Pausing:** while the screen pauses the detector, progress is kept and the timer is frozen.

## 1. Sweet Tooth and Minion

| | Sweet Tooth | Minion |
|---|---|---|
| Code at car select (`sub_801307EC` (screenCarSelect)) | **Up, L1, Triangle, Right** | **L1, Up, Down, Left** |
| Unlock flag | `D_80180D04` (unlockSweetTooth) (u8, gp+816) | `D_80180D05` (unlockMinion) (u8, gp+817) |
| Car-select index / type | 12 / 0x10 | 13 / 0xE0 |

- **Code table:** `D_800CFF14` (carSelectCodeTable) = {7,5,13,10, 5,7,8,9, 16,16,16,16}. The third code (Circle ×4) is a dummy, because Circle backs out of the screen.
- **What the code does:** event 17/18 runs `sub_80137744` (shellSetUnlockFlag), which **sets** the flag to 1; it never toggles it. It also plays shell sound 29.
- **Which pad:** the detector reads port 0. For the second human in modes 2 and 4, car select switches it to port 1. The flag is global, not per player.
- **Timing:** Start, Cross, Square, Circle, Left, Right and R1 pause the detector until the carousel animation ends. Up, Down, L1 and Triangle do not. Both codes end on Right/Left, so the last press also turns the carousel.
- **How the carousel is limited:** the carousel has 14 positions. The step objects skip any car for which `sub_8012CC10(car)` (carIsSelectable) returns 0:
  - car 12 needs `D_80180D04` (unlockSweetTooth);
  - car 13 needs `D_80180D05` (unlockMinion);
  - cars already taken by a human or an AI are skipped too.

  The same predicate serves the R1 random car, AI select (`sub_80131EEC` (screenAiSelect)) and the random opponents. Dark Tooth (14) is never offered.
- **Nothing else unlocks them.** The only other writer is the tournament opponent picker `sub_8012CD08` (tournamentPickOpponents). It saves `D_80180D04` (unlockSweetTooth), forces it to 1 while it rolls `rand() % 13`, then restores it. So computer opponents can be Sweet Tooth even when the player has not unlocked him, but never Minion. Winning, beating the Minion boss and passwords never set the flags.
- **Passwords:** they only encode cars 0..11, and the mid-game options never show a password for cars 12 and 13. A Sweet Tooth or Minion tournament therefore cannot be resumed by password (see `TOURNAMENT_PASSWORDS.md`).
- **Lifetime:** RAM only, until power-off or reset. Returning to the title, attract mode (which saves and restores only the 48-byte settings struct), game over and a finished tournament all leave the flags set.
- **Side effect:** at level 4, a human Minion skips the Minion boss intro (`session_runner.cpp`).

**For profiles:** store the two flags and write them back to `D_80180D04` (unlockSweetTooth) and `D_80180D05` (unlockMinion) at boot (or when the profile loads).

## 2. Secret levels

| Level (+24) | Name (`sub_8012ACB8` (levelDisplayName)) | Code at level select |
|---|---|---|
| 10 | Florida: Suicide Swamp | **Up, Down, Right, R1** |
| 11 | Los Angeles: Cyburbia | **Down, Up, L1, R1** |
| 12 | Los Angeles: Roof Tops | **Down, Left, R1, Down** |

- **Where the codes work:** level select `sub_8012FD30` (screenLevelSelect) (code tables chosen at 8012FF18..8012FF60). Only **2-player Deathmatch** (mode 4) uses the real table `D_800CFE70` (secretLevelCodeTable) = {7,8,10,11, 8,7,5,11, 8,9,11,8}, on either pad.
- **Challenge mode** (mode 3) uses the dummy table `D_800CFEA0` (secretLevelCodeTableOther) (Circle ×12), so the codes cannot fire there.
- **What happens on a match:** the secret levels are never in the carousel. A matching code ends the scene with result 4. The screen then sets the level to code index + 10 (`sub_80137814` (settingsSetLevel)), prints "secret level selected" and goes to car select.
- **Timing quirk:** Left/Right slide the level carousel and pause the detector until the slide ends. Progress is kept, so after Suicide Swamp's Right and Roof Tops' Left, wait for the slide before the next press.
- **No unlock state.** Nothing is stored, and the code is needed on every visit. The tournament only goes through levels 1..8. Level name 9 ("It's a Secret") is unused, as far as I can tell.

**For profiles:** "unlocked levels" is new state. The port can add the levels to the carousel or allow them in other modes as a new feature.

## 3. Winner movies

**When an ending plays:**
- Only a won **1-player tournament** (mode 1) on Medium or Hard gets an ending.
- Easy stops after level 4 (`SHELLDB2\STOP.TIM`).
- A 2-player tournament shows only `SHELLDB2\COWIN.TIM`.

**Inside level 8** (`session_runner.cpp`):
1. The first win brings in Dark Tooth, and opponent 0 becomes car 14.
2. The next win shows the "old man" reveal, and another round is played.
3. The final win shows the winner text.

**After that:** `screenScrollText` (80136BB0) returns `sub_80136D98` (screenEnding), the ending player. The credits follow (`sub_80136A3C` (screenCredits), `SHELLDB2\CREDIT01..07.TIM`).

**What `sub_80136D98` (screenEnding) plays:** for each human, with `car` = their car-select index (settings +8):
1. the car's movie from the table `D_801648C4[14]` (endingMovieTable) = {file, frames, wide}, via `sub_8012B898` (moviePlayEnding);
2. then `\MOVIES\YOUWON.STR;1` (118 frames), **except for Grasshopper** (index 4; the test is at 80136E78).

| Index | Car | Movie | Frames | Wide |
|---|---|---|---|---|
| 0 | Hammerhead | `\MOVIES\MONSTERT.STR;1` | 1150 | 1 |
| 1 | Outlaw 2 | `\MOVIES\COPCAR.STR;1` | 931 | 1 |
| 2 | Warthog | `\MOVIES\HUMVEE.STR;1` | 584 | 0 |
| 3 | Mr. Grimm | `\MOVIES\MRGRIM.STR;1` | 1362 | 1 |
| 4 | Grasshopper | `\MOVIES\DUNE.STR;1` | 887 | 1 (no YOUWON) |
| 5 | Thumper | `\MOVIES\IMPALLA.STR;1` | 758 | 1 |
| 6 | Spectre | `\MOVIES\VETTE.STR;1` | 989 | 1 |
| 7 | Roadkill | `\MOVIES\ROADKILL.STR;1` | 1212 | 1 |
| 8 | Twister | `\MOVIES\INDY.STR;1` | 1465 | 1 |
| 9 | Axel | `\MOVIES\AXEL.STR;1` | 1103 | 1 |
| 10 | Mr. Slam | `\MOVIES\FRONTL.STR;1` | 1579 | 1 |
| 11 | Shadow | `\MOVIES\HEARSE.STR;1` | 1253 | 1 |
| 12 | Sweet Tooth | `\MOVIES\SWEET.STR;1` | 1016 | 1 |
| 13 | Minion | `\MOVIES\MINION.STR;1` | 586 | 1 |

**Notes on the table:**
- **Wide:** 1 means a 480×240 display and decode; 0 means 320×240. This is inferred from the display setup in `sub_8012B898` (moviePlayEnding).
- **Skipping:** the car movie can be skipped with Cross or Start on either pad, but only from frame 35 (`sub_8012B82C` (movieSkipCheckDelayed)). YOUWON can be skipped at once.

**For profiles:** the game records no endings. To build an endings gallery, record `settings[8]` when `sub_80136D98` (screenEnding) runs; the gallery can then replay `D_801648C4[car]` (endingMovieTable).
