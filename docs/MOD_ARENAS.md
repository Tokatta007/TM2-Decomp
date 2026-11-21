# Mod arenas: level rules as data (PC port request 114)

The port's `arena-loading` branch plays a mod arena as level 13 (`sessionSettings.level`), with `gameGetLevel`
wrapped to answer 7 (Holland), so every rule read through `gameGetLevel` takes Holland's. Two of those rules are
places on Holland's map. This request makes them data (section 1). Section 2 is the census of the other places that
read the level.

## 1. As built (`TM2_PORT_MOD_ARENA`)

### 1.1 Storage

`portArenaRules` (`.port_bss`, every port variant, 0x1C bytes; `include/semantic/port_mod_arena.h`,
`config/US/port_variant_arrays.tsv`). The port writes it when it loads an arena: all 0 for a retail arena. Nothing in
the game writes it. It is the arena's data, constant through a match, the same on every machine that loaded the
same arena.

| Offset | Field | Meaning |
|---|---|---|
| +0x00 | `heal` | 0 retail (the level's square, carTick's switch); 1 the square below; 2 no spot; other values act as 0 |
| +0x04 | `healX` | the 64 x 64 square's low corner, car `motion.pos[0]` (retail's local `x`) |
| +0x08 | `healZ` | the same, `motion.pos[1]` (retail's local `z`) |
| +0x0C | `warps` | 0 retail (the level's exits); nonzero: `exit[]` |
| +0x10 | `exit[3][2]` (s16) | per warp side 0..2: spawn node, target node. A spawn node below 0 is "no exit" |

The heal spot works as retail's does: a human car that is stopped (`motion.speed == 0`) with both coordinates
inside `[x, x + 64]` and `[z, z + 64]` gets full health, once per car (`heal_spot_used`). For reference, Holland's
spot is (-0x490, -0x1838) = (-1168, -6200).

### 1.2 Code

- `humanCarTick` 800DC88C (`car_tick.c`): the spot comes from `portArenaHealSpot(&x, &z)` (`.port_text`). It returns
  `heal` 1 with the port's corner, or 2, which skips the spot test entirely. Otherwise it returns 0 and the retail
  switch runs.
- `aiWarpSetExitNode` 80115A48 (`fx_debris_splash.c`): with `warps` set, side 0..2 with a spawn node >= 0 sets the
  computer car's `spawn_node` and `target_node` from `exit[side]` and places it with `aiPlaceOnRouteNode`. A spawn
  node below 0, or a side past 2, leaves the car's nodes as they were and doesn't place it, as retail does on a level
  without an exit for that side. Retail's levels use sides 0 and 1, and Hong Kong also uses 2. The side is the
  teleporter's index in `worldWarps` (`aiCarDamageUpdate`'s loop in `world_collapse_warps.c`). It is the same
  numbering the port's `warp1..3` use, if those fill `worldWarps[0..2]`.
- Neither calls `rand()`. With the define absent the retail units are unchanged, and the three retail images stay
  byte-identical.

### 1.3 Checked

`check_custom.py` check 26 runs both functions in LinkMips:

- `portArenaHealSpot`: for `heal` 0, 1 (two corners, one at the s32 extremes), 2 and 3, it checks the return value
  and the corner written, or nothing written.
- `aiWarpSetExitNode` with `portArenaRules` 0, where `gameGetLevel` and `aiPlaceOnRouteNode` are stubs: retail's
  exits for Holland sides 0, 1 and 2, Los Angeles 0 and Hong Kong 2, and no exit on Moscow or on 13.
- `aiWarpSetExitNode` with a table (sides 0 and 2 set, side 1 "none") on levels 7, 13 and 2: each side's nodes, and
  sides 1, 3 and -1 not placed and unchanged.

The check also confirms `portArenaHealSpot` is called only by `humanCarTick`. **Not tested in game.**

## 2. Census: the other readers of the level

The census covers every call of `gameGetLevel`, which answers 7, and every raw read: `sessionGetLevel`,
`sessionSettings.level`, `settings->level`, and the cached copies. It was done by reading the code. Holland's level
events (barns, targets) are in `ARENA_SWITCHES.md` §2 and are not repeated here.

### 2.1 What sees 13 (the raw level)

| # | Where | What 13 does | Port status / suggestion |
|---|---|---|---|
| R1 | `arenaSetup` 8011995C (`arena_setup.c`), from `sessionBindCars` | Its default is `arenaLoadHolland`: Holland's counts (58 route nodes, 7 sections, 8 surfaces), `startNodesDen` (15 start ids up to 47), Holland's sky, `arenaHollandFlag = 1`, `arenaShortChaseRange = 1` | The port wraps `arenaLoadHolland` (ARENA_LOADING.md). The wrap must set all of these, start nodes included: a mod with fewer than 48 route nodes and Holland's start ids would start cars on empty route records |
| R2 | `worldWarpsSetup` 80115310 (`car_damage_human.c`), from `arenaSetup` | No case for 13 and no default: only `active`, `timer` and `src` are reset. Each warp's position, destination and `link` keep the previous arena's values (zeros after boot) | The port's `warp1..3` set the teleporters. They must write `v`, `dst` and `link` for every warp the mod's DMD can trigger (materials 694..697), and zero the rest |
| R3 | `roundStartMusic` 80128DB4 (`game_mode_audio.cpp`), `roundStartMusic(level)` from `sessionRun` | 13 + 1 = CD track 14. `cdAudioPlayTrack` reads `cdAudioTrackToc[14..15]`, which retail never uses: wrong position or silence (LIKELY). The pause menu's resume passes `gameGetLevel() + 1` to `cdAudioResume`, which ignores it | Already on the port's list (ARENA_LOADING.md item 4) |
| R4 | `screenDealCards` 8012A7B8 (`scene_init.c:559`), modes 0..2 when not on the Dark Tooth stage | Loads `SCREEN\SHOWOPS13.TIM`, which is not on the disc. `fileioOpenFile` retries once a frame for up to 300 tries (about 5 s), then the stale work buffer is drawn | **New:** redirect the name in `vp_FileLoad` (to `SHOWOPS7` or the mod's own card), or pass 7 |
| R5 | `levelFileStem` 8012ABD0 (from `settingsLevelDmdFileName` / `TmsFileName`) | "UNKNOWN LEVEL" | The port already rewrites both names |
| R6 | `passwordBuildString` 801292A4 (`scene_init.c:468`), tournament only (mode 1, level >= 2) | `passwordEncode` packs `2 * (level - 1)` into a 3-bit field: 24 overflows into the car bits, so the password is wrong | Keep mods out of the tournament, or show no password above 12 |
| R7 | `sessionRun` 8012BE5C (tournament win) | "Prepare for" `levelDisplayName(14)`, "It's a Secret" (`SECRET_LEVELS.md`) | Cosmetic, tournament only |

### 2.2 What sees 7 (Holland's answer) and is Holland's map

| # | Where | Notes |
|---|---|---|
| H1 | the heal spot (`humanCarTick`) | Section 1 |
| H2 | `aiWarpSetExitNode` | Section 1 |
| H3 | `demoRespawnAiCars` 8011EFDC (`spawn_tables.c`), case 7 `demoSpawnHolland` | Attract demo only: Holland's node numbers and camera positions. Keep `attractDemoLevel` on retail levels |
| H4 | `targetDamage` 80125118, type 510 (Holland's barns) | Disables sections 2 and 4, or 0 and 5, whatever the arena. Not a level read: a mod must not use target type 510 (or the port sets `HOLLAND_BARNS`, `ARENA_SWITCHES.md`) |

### 2.3 Everything else (OK)

The rest of the `gameGetLevel` sites (about 80, in 35 files) and raw reads are OK:

- **Comparisons with other levels:** about 60 sites compare with 1, 2, 3, 5, 6 or 8, so 7 takes the "other level"
  path, as any non-event arena would. This includes the fall heights (`worldFallLimitZ` default -8000), the moving
  surfaces, the weapon slots and lightning (refused, as on Holland).
- **Tuning:** six sites: AI strength `3 * level`, AI waits `(8 - level) * 60`, the weapon interval
  `80 - 4 * 7 = 52`, the split-screen choice, `arenaIsHolland`.
- **Arena events:** 14 sites, all in `ARENA_SWITCHES.md`.
- **Raw reads elsewhere:** the menus are all guarded (the globe table is skipped for 9 and above); `screenCardLevel`
  is write-only; `worldLoadedArena` is never read in C; `tournamentOpponentCount` (default 0) is wrapped by the port.

No fixed-size table indexed by the level is reached with 13. The only out-of-range reads are R3 (the CD table) and
R6 (the bit field).
