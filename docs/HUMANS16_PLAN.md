# 16 human records, at most 4 of them local: plan

**For the PC port:** a plan ahead of the request (16 online players, at most 4 local per machine). The
questions are in section 5, the port's answers (request 37) in section 6. Steps H0..H6 (section 4) each become a
PR; **H0 is done** (2026-10-14, object ids), **H1 is done** (2026-10-14, the local-player map), **H2 is done**
(2026-10-14, the inputs; also the AI-aim bit of H5 and the two H1 review items), **H3 is done** (2026-10-15, sound
sides and local feedback), **H4 is done** (2026-10-15, the storage: `TM2_PORT_HUMANS` up to 16), **H5 is done** (2026-10-15, the rules behind `portHumanOptions` bit 1, the respawn, the dedicated-server list), **H6 is done** (2026-10-15, the checks collected,
`portHumanCompact` moves the car selection too, `tools/check_all.py`, the port's test checklist in section 7).

Read-only planning pass, 2026-10-14. Tree: branch `decomp-requests-33f` at `22df6c3` (main after PR #42). The
working tree also had another agent's request 33 step 4 edits (`include/semantic/car_sound.h`,
`include/semantic/players.h` `TM2_MAX_SOUND_REQUESTS`, `port_variant_arrays.c` `portCarSoundLoops`); file:line
below are for the committed files unless they say otherwise. Sources: `docs/PER_CAR_PLAN.md` D, `docs/FOUR_HUMANS.md`,
`docs/FOUR_PLAYERS.md` 2-3, `docs/SIXTEEN_CARS.md` 1.5, `docs/CAR_GROUPS.md` 7 and 14, `docs/SOURCE_BUILD.md`
"Port variant", `docs/RAND_CALLERS.md`, `include/semantic/players.h`, `settings.h`, `car_groups.h`, `car_sound.h`,
and the C in `src/semantic`. Nothing was built or run.

Tags: **CONFIRMED** = read in the C. **LIKELY** = follows from the C, not traced to the end. **CHECK** = the
executing agent must verify first.

## 0. The goal in one paragraph

Up to 16 human records (`humanCars[i]`, `i` = **human index** 0..15), any mix with bots up to 16 active cars. At
most 4 humans are **local** (split screen, local slot 0..3); the rest are **remote** (inputs fed by the port). Every
per-human array goes to 16. Everything per local player stays at 4: views, cameras, HUD records/OTs/radars, pads,
sound sides (0/1), frame records. Today `humanCars[i].player == i` doubles as pad port, view, camera, HUD and sound
side. The plan splits that: the record keeps `player = i` (it is also the object id and the record index in ~25
weapon and contact functions, section 1.4), and a new **local map** says which local slot (or none) a human has.
Every view/HUD/pad/sound-side use goes through the map.

---------------------------------------------------------------------------------------------------------

## 1. Census: everything keyed by the human index

### 1.1 Per-human arrays (must reach 16)

| array | where defined | entry | at 4 (variant today) | at 16 | note |
|---|---|---:|---:|---:|---|
| `humanCars` | `port_variant_arrays.c:111` | 0x814 | 8,272 | 33,088 | the records |
| `humanCarObjects` | `:112` | 4 | 16 | 64 | worldUpdate's human loop breaks at the first null (`world_update.c:191`) |
| `humanCarObjectSlots` | `:113` | 4 | 16 | 64 | |
| `padConfigs` | `:114` | 4 | 16 | 64 | per record (`carApplyPadConfig` action map at record +0xF8), not per pad |
| `humanCarTypes` | `:115` | 4 | 16 | 64 | |
| `hudSmoothedSpeeds` | `:116` | 4 | 16 | 64 | per human; no HUD code reads it (only `hudSmoothSpeed`/`hudResetSpeed`, `car_records.c:239/245`) |
| `carViewHotkeyReleased` | `:41` (`.port_data`, all 1) | 4 | 16 | 64 | per human (edge detector for that human's hotkeys) |
| `sessionPortHumanCars` | `:119` | 1 | 2 | 14 | settings, humans 2..N-1 (`settings.h:52`) |
| **`carResetWeapons`** | **retail storage** `timer_state.c:16` (u8[2] in 4 bytes, not moved) | 1 | 4 (fits) | 16 | **must move** to `.port_bss` past 4 humans; writers `carSetResetWeapons` (`timer_frame.c:108`), `car_damage_human.c:329`, `portHumanCompact`; readers `carBindHumanSlot` (`scene_init.c:107`), `humanCarsRestart` (`arena_setup.c:238`), `sub_8011E114` (`arena_load_register.c:722`, dead) |
| `portCarBindGroup` humans | `car_groups.h:55` `TM2_PORT_BIND_HUMANS 16` | 1 | | 16 | exactly 16: add `#if TM2_MAX_HUMANS > TM2_PORT_BIND_HUMANS #error` |
| settings `human_car[2]` (+0x08), `lives[4]` (+0x14), `control_layout[2]` (+0x26) | `settings.h` | 1 | | | cars: port table (done); lives and layouts of humans 2.. / 4..: no storage yet (1.6) |
| `D_80180FD8/FE0[2]` | retail, dead | | | | stay 2 (`FOUR_HUMANS.md` 1.1) |

Record fields that hold the index: `player` (+0x000), `id` (+0x002, `uaInit` `ua_init.c:84`, `humanCarSetId`
`game_misc_accessors.c:248`, read by `humanCarFindById` `car_records.c:332`). Values elsewhere that hold a human
index (all wide enough for 0..15, CONFIRMED by type): shield record `owner` (s16, `weapon_special.c:129`, read by
`weaponShieldsUpdate` `fx_sprite_spawn.c:183`), lightning arc shooter (record index), `aiDarkToothSwoopTarget`
(s16), `weaponSlamFindTarget`'s `id = player` (`screen_card_draw.c:178`), contact/collision `id = player`
(`car_collision.c:90/115`, `car_contact_resolve.c:206`, `car_contacts.c:298`, `car_restart.c:246`: record index for
`humanCarObject(i)` and sound instances).

### 1.2 Per local player (stay at 4; must never be indexed by a human index)

Views: `viewCameraObjectTable`, `viewFollowObjects`, `viewCameraObjects`, `viewEnvs`, `viewModes`,
`viewEyePositions`, `viewMatrices` (all `[TM2_MAX_VIEWS]`). HUDs: `hudPlayerRecords` (1,900), `hudRadars` (0x430 with
15 blips), `hudOrderTables[2][4][2]`, `hudMarkerBlinkCounters/Phases`, `hudTargetNameModes/Sprites`,
`hudRadarArrow*Prims/Sprites` (`[TM2_MAX_HUDS]`); `hudRadarVisible`, `hudWeaponListVisible`, `padLayout` (u8[4],
retail sdata). Pads: `padActiveBindings[2]`, `padConfiguredBindings[2]`, `padActionCache[128][2]`,
`padButtonsLatched` (two 16-bit halves), the repeat state P1/P2. Sound: the side 0/1 lists (`soundMgrPlaySided`
`sound_manager_play.c:69`, `soundMgrStopSided` `:123`; owner +0x608/+0x61C turbo, +0x630/+0x644 gun, +0x66C/+0x680
loop 32) and flags `soundTurboLoopActiveP1/P2`, `soundGunLoopActiveP1/P2`, `soundLoop32ActiveP1/P2`. Frame
records, OTs, packet buffers (port side). Scalars: `viewMainPlayer` (only `uaInit` writes it, = 0), the split mode,
`rtRearViewActive`, `viewExtraViewsUnlocked`.

### 1.3 Every place a human index is used as pad / view / camera / HUD / sound side (the coupling to cut)

| use | site (file:line) | today | 16-human hazard |
|---|---|---|---|
| pad actions | `car_input.c:55` `ACTION(a) = padCheckAction(a, player)` in `humanCarReadInput` (`:95`) and `carReadMoveCodes` (`:235`) | port 0/1 = pad, 2 = **either pad**, 3+ = nothing (`controls.c:137`) | humans 2.. get wrong or no input; remote humans have no pad |
| cheat combos | `car_tick.c:201-242` (`humanCarTick` 800DC88C): `padComboHeld(n, player)`, `padAnyLatched(player)` | port 0 = whole 32-bit latch, other = high half (`shared_pad.c:129/152`) | game-state cheats (armour, HP, turbo, infinite); remote humans need their own word |
| hotkeys | `carViewHotkeys` `car_records.c:104`: `carViewHotkeyReleased[player]` (per human, ok), `hudToggleRadar(player)`/`hudToggleWeaponList(player)` (`:131/136/155/160` → `hudRadarVisible[player]`, u8[4]: **writes past at 4+**), split toggle (global), `rtToggleRearView` (global), `padComboHeld(1, player)` (`:174`, sets global `viewExtraViewsUnlocked`), `carSetViewMode` | | |
| camera | `worldUpdate` `world_update.c:229-230` `if (viewMainPlayer == i || viewGetSplitMode()) viewCarCameraUpdate(&humanCars[i], i)`; `viewCarCameraUpdate` `camera.c:47`: `viewSetCameraPose(i, ..)` (`:56/61`), `viewSetRotation/Position(.., c->player)` (`:67-68`) | | view arrays [4] indexed 4..15 |
| view mode | `carSetViewMode` `weapon_missile_trails.c:395`: `viewSetCameraPose(human->player)` `:411`, `viewSetPosition/Rotation(.., player)` `:444-445`, `rtSetRearView`/`rtToggleRearView` (global). Callers: `carBindHumanSlot` `scene_init.c:109` (every human, every bind), `humanCarsRestart` `arena_setup.c:241` (every human when split 0), `carViewHotkeys` `car_records.c:150`, `car_crash.c:222` (sim path: view 3/4 on ground class 1 → 2), `portCarStartHuman` `port_variant_arrays.c:4554`, attract/watch on `humanCars[0]` (`scene_init.c:72`, `view_watch_ai.c:42/51`, `world_collapse_warps.c:330`) | | same, plus rear-view/global toggles from remote humans |
| sky follow | `world_update.c:244` `viewSetFollowObjPos(target, i)` every human every frame (`view_system.c:345`) | | `viewFollowObjects[4..15]` |
| HUD draw | `rtMainLoop` `rt_main.c:204-216` passes HUD 0/1; `hudDrawSingleView` `hud_main.c:78` / `hudDrawSplitVertical` `hud_split.cpp:112` / `hudDrawSplitHorizontal` `:249`: the HUD index is also `humanCarGet(player)` (`hud_main.c:84`, `hud_split.cpp:119/256`), `carGetDisplaySpeed(player)` (`hud_main.c:186`), `gameGetLives(player)` (`:189`) | HUD slot = human | HUD slot s must show its local human, not human s |
| target panel | `hudDrawTargetPanel(ot, i)` `car_destroyed.c:433`: `carNearestToHuman(i)` (human) and `hudRadarVisible[i]`, `hudTargetNameModes[i]`, `hudPlayerRecords[i]` (HUD) | one index for both | split into (hud, human) |
| radar | `hudRadarPlotCar(type, pos, angle, idx)` `hud_radar.c:110` (idx = HUD). Callers: `worldUpdate` retail pair `world_update.c:207-220` (radars 1/0), variant `worldRadarPlotHumans` `:156` (`hudRadarPlotCar(.., j)` with j a **human**), AI blips `:411-422` (radar 0 for human 0, radar 1 for human 1 in mode 2), variant `:424-433` (`hudRadarPlotCar(.., j)`, j human) | | radars [4] indexed by human |
| speed readout | `hudSmoothSpeed(i)` `world_update.c:226` | per human | fine (array per human) |
| sound sides | turbo: `carThrottleSteer` `car_control.c:181` start, `:21-26` `PAD_REFRESH` stop; gun: `weaponTick` `weapon_tick.c:71` start, `:81-84` stop; loop 32: `carSoundUpdate` `screen_card_draw.c:92` start (`a1[0]` = player), `:94-99` stop | side = `player` | side ≥ 2 leaks one request per frame (`FOUR_HUMANS.md` 3); request 33 step 4 (in progress, `car_sound.h:44-52`) gives humans 2.. slot instances; must become "humans without side 0/1", not "index ≥ 2" |
| pad layout | `sessionRun` `session_runner.cpp:117-118` `padSetLayout(control_layout[0/1], 0/1)` | local pads | fine (local slots 0/1); remote humans need their own layout (1.6) |
| pad config | `PadSetConfig/PadGetConfig(player)` `car_records.c:492/500`, from `uaInit`, `carRestart` `car_restart.c:221`, `portCarStartHuman` | per record | fine (per-human array) |
| non-positional feedback | `hudMessagePost(..)` and `soundPlay(x, 0, 0, ..)` from a human's own actions: `car_input.c` (11: combo jingles/messages), `pickup_apply.c` (6), `weapon_special.c` (7), `weapon_select.c` (3), `car_control.c` (3: turbo start `:183`), `car_tick.c` (2), `weapon_basic.c` (2), `car_destroyed.c` (2), `car_crash.c`, `car_restart.c`, `car_tumble.c`, `car_wheel_sway.c`, `weapon_slam_swing.c`, `weapon_arcs_draw.c`, `target_actor_update.c` (3), `world_update.c:470` denied sound (grep count of `soundPlay(x, 0, 0,` / `hudMessagePost(` in game code; menu/screen files excluded) | heard/shown on the shared screen | a remote player's pickups, combos and denials would show on this machine's HUD and play at full volume |
| engine-voice cap | `portCarSoundFrame` `port_variant_arrays.c:~2452`: nearest to **any** human | | should be nearest to a local human (listener) |

### 1.4 Object ids (human i gets i + 1) and the id ranges

Writers of object +0x118 found in the C (CONFIRMED): `csReset` 0 (default, `engine_clut_anim.c:611`); `csPoolInit`
root = **8** (`engine_clut_anim.c:643`: the world, hit result 8 = "no car"); `missilesCreateObjects` = **10**
(`world_object_register.c:120`, every projectile record's scene object); human i + 1 (`scene_init.c:111`); AI
j + 50 (`car_parts_spin.c:231` = `carBindAiSlot`); track vehicles 100 + i (`arena_load_register.c:671`); the
projectile pool 0x8A2 (`asset_pools.c:49`, `port_variant_arrays.c:2053`); breakables/targets 500..5999 (by the
registries), target actors 6000 + n (`target_actor_lifecycle.c:52`).

Tests on ids: "is a car" `(u32)(id - 1) < 99 && id != 8` (`weaponApplyHit` `weapon_missile_trails.c:337`),
`id < 6000 && id != 8` (`carContactResolve` `car_contact_resolve.c:253`); hit result 8 = nothing
(`car_damage_human.c:51`, `projectile_hit.c:240`, `weapon_bullets.c:276`); "owner is human" `owner < 50` (~15 sites,
`SIXTEEN_CARS.md` 1.5; `projectile_hit.c:207`, `weapon_bullets.c:207/255`, `weapon_missile_trails.c:346`,
`weapon_ricochet.c:113`, `weapon_slam_swing.c:125`, `weapon_tornado.c:225`, `weapon_fire_specials.c:298`, ...).

**Hazards:** human 7 gets id 8 (world: never a car; `weaponApplyHit` and `carContactResolve` ignore it). Human 9
gets id **10**, the missile scene objects' id: `weaponApplyHit(10, ..)` takes the car branch, `carApplyDamage(10)` →
`carFindById(10)` would find human 9, so a hit test that reports a missile object would damage human 9 (LIKELY;
CHECK whether `projectileHitTest`/`hdp` can return a missile object at all). Skip 10 as well.

Writers of a human id (must use the macro): `carBindHumanSlot` `scene_init.c:111`; `portCarBindSlot`
`port_variant_arrays.c:2949`; `portDropInId` `:4028`; `portHumanCompact` `:4270-4271, 4290` (and every owner id it
rewrites); owner ids from the record: `update_guns.c:32` (`player + 1`), `weapon_basic.c:138`, `weapon_helpers.c:92`,
`weapon_special.c:77`. Id → index: `weaponDropCheckHit` `car_damage_human.c:70` (`humanCarGet(owner - 1)`, the only
one); `carFindById` (`car_records.c:411`) works by slot table, any id. Value tests (`hit == owner`,
`b[i].a != -owner`) work for any id.

**Macro (as built in H0, `players.h`):** `TM2_HUMAN_ID(i) = i < 7 ? i + 1 : i < 8 ? i + 2 : i + 3` → 1..7, 9,
11.. (16 humans: 1..7, 9, 11..**18**; the earlier "11..19" counted one human too many);
`TM2_HUMAN_INDEX_OF_ID(id) = id <= 7 ? id - 1 : id <= 9 ? id - 2 : id - 3` (unchecked: for ids known to be a
human's); the checked form is the port function `portHumanOfId(id)` (-1 for 0, 8, 10, AI ids, ids of absent
humans). With at most 7 human records both are `i + 1` / `id - 1` (retail and every variant compile to the same code).
**Max safe human id: 49** (ids 1..49 minus 8 and 10 = 47 humans). `players.h`: `#if TM2_HUMAN_ID(TM2_MAX_HUMANS - 1)
>= 50 #error`; more than 7 records without `TM2_PORT_HUMANS` is an error.

**The id scan (H0, CONFIRMED; was CHECK).** Over the whole retail text (all 1,948 functions):
- every store to +0x118 (17 `sw/sh/sb ..,280(..)`): object ids written are 0 (`missileTrailsInit`, `pickupSlotsReset`,
  `csReset`, `fxDebrisDraw`), 8 (`csPoolInit`'s root), 10 (`missilesCreateObjects` 800D90FC), 0x8A2 (`weaponAssetBind`
  ×2), human `slot + 1` (`carBindHumanSlot` 8011A50C), AI `slot + 50` (`carBindAiSlot` 8011A6C4), track vehicles
  `index + 100` (`worldTrackVehicleAttach` 801179B0), target actors `count + 6000` (`targetActorSpawn` 8012594C); the
  other five are other records' +0x118 (stack spills in `carCarCollision`, `wheelBlockReset`'s s16 at +0x118,
  counters in `pickupRegister` and `fxDebrisDraw`, `SpuVmKeyOn`). No writer of 9 or 11..49.
- every compare of a value loaded from +0x118 (19): constants 6000, 8, 100, 101, 50 and value compares (owner, other
  id); none against 9..49.
- every range test `(x - c) < k` (c 1..10) whose range meets 9..49 (14): 12 switch jump tables (game mode, settings
  etc.) and 2 range tests of a record's +0xC8 (`80118B60`, `8011ED4C`); none on an id.
- the C: every compare of an `id`/`owner`/`hit`/`attacker`-named value against 9..49: none (the hits are car types
  `id >> 4 < 15`, `objRegistryAdd`'s callback ids 16..399, HUD widths).
Hit results: `projectileHitTest` returns only car objects' ids (its loops go over the human and AI car objects),
500 + i (breakables) and 6000 + i (spinners); `hdpQueryPoint` reports the id of whatever scene object it hits, which
can be the root (8) or a missile object (10): so 10 must stay skipped, as planned. Ids 11..49 are free.

### 1.5 Loops and fixed indices

- **By `humanCarTotal`** (fine at 16, arrays only): `humanCarsRestart` (`arena_setup.c:233`), `worldUpdate`'s human
  loop (`world_update.c:190`), `humanCarNearest` (`car_records.c:187`, includes dead humans: retail), `humanCarFindById`,
  `carsPhysicsUpdate` (8 gp sites), every weapon/contact loop on `humanCarCount()` (`SIXTEEN_CARS.md` 1.2),
  `worldRadarPlotHumans` (`world_update.c:156`, **O(N²) with `rand() % 5` per pair**), `portStartNodePick`,
  `portCarSoundFrame`, `portCarRec`, `portCarKeyOfStats` (by `TM2_MAX_HUMANS`), `sessionLoadArenaAndCars`
  (`shell_main.c:298/334/370/391`), `sessionBindCars` (`screen_card_layout.c:205-236`), `sessionRun`'s Minion and
  dead-human loops.
- **By `TM2_MAX_HUMANS`** (variant): `uaInit` (`ua_init.c:81`, `:101`), `carTablesClear` (`:143`), `roundStart`
  (`scene_init.c:62`), `TM2_RESET_WEAPONS_REST` (`session_runner.cpp:71`), `settingsClearHumanCars`.
- **`i < 2` / two sides** (local, stay): `padInit`/`padLoadDefaultBindings` ports 0/1, sound sides, `rtMainLoop`'s
  HUD 0/1, `pauseMenu` pads 0/1, `carViewHotkeys` split modes, `uaInit`'s dead `D_80180FD8` loop.
- **Fixed human 0 / 1** (rules; section 2.6): round won needs `!humanCars[0].dead` (`world_update.c:441`); deathmatch
  result 4 for `i == 0`, 3 otherwise, first death ends the round (`car_damage_human.c:330-343`); `--lives[0]`
  (`session_runner.cpp:167`), `humanCarGet(0)` dead test (`:183`), two-sided tallies; AI radar blips for humans 0/1
  (`world_update.c:411-422`); AI aimed shots at `humanCarObject(0)` (`weapon_targets.c:34`, `:107`);
  `carPlayerOneObject = humanCarObject(0)` (`bulletsUpdate` `misc_one_liners.c:501`, use not traced);
  `carNearestToHuman` (`ai_steps.c:856`: 1 human or co-op → nearest AI, else nearest **human** only, so with bots in
  deathmatch the target panel never shows a bot); HUD lives `gameGetLives(0)` (`hud_split.cpp:123/260`); attract /
  watch view on `humanCars[0]` (`spawn_tables.c`, `view_watch_ai.c`, `world_collapse_warps.c:302-330`).

### 1.6 Settings, progression, saves

- `human_car`: humans 2.. in `sessionPortHumanCars[N-2]` through `TM2_SETTINGS_HUMAN_CAR` (grows with N: 14 entries at 16, H4).
- `lives[4]` (+0x14): `gameGetLives(p)` (`misc_one_liners.c:289`) reads `lives[p]`: p ≥ 4 reads `level` and on.
  Only HUD lives text and the tournament runner use it (modes 1/2, ≤ 2 humans in the menus).
- `control_layout[2]` (+0x26): only local pads 0/1 (`session_runner.cpp:117-118`).
- **`settingsCopy`** (`menu_state_copy.cpp:35`, called by the attract demo `shell_attract_demo.c:18/30` to save and
  restore `sessionSettings`): copies `list8` (human cars), `list20` (lives), `list38` (layouts) `count = humans`
  entries each. With `humans` > 2 it copies source bytes into the same offsets of dest past each list (harmless
  inside the record), but `list38` from +0x26 with **`humans ≥ 7` writes past the 0x2C bytes the record owns in RAM
  into `screenCardDrawEnvs` (80164788)**; `list10` likewise for `opponents > 10` (step 2, not fixed either). CONFIRMED
  (code); clamp in the variant.
- Tournament/progression: modes 1/2 (1-2 local humans from the menus): passwords (`human_car[0]`), `--lives[0]`,
  boss reloads, `screenEnding` per human (16 ending movies if `humans` = 16 at the end: the port sets `humans` back,
  or skips). No per-human progression state. **No memory card** (passwords only; no card code in `src/semantic`).

### 1.7 The input path (what the port feeds)

`padDecode` (`shared_pad.c:38`) packs both BIOS pads into `padButtonsDecoded` (pad 0 low half, pad 1 high half);
`padUpdate` → `padLatchWithRepeat` (`controls.c:224`) copies it to `padButtonsLatched` once per frame (battle uses
repeat mode 1: latch = decoded). Readers: `padButtonHeld(button, port)` (`shared_pad.c:96`, port 2 = either),
`padComboHeld` (`:129`, port 0 compares the **whole** 32-bit word), `padAnyLatched` (`:152`), and
`padCheckAction(action, port)` (`controls.c:137`: action 6 = any button; else `padActiveBindings[port].button[action]`
→ `padButtonHeld`, result cached in `padActionCache`). Bindings per port come from `padSetLayout(layout, port)` →
`padLoadLayout` (`spawn_tables.c:695/704`) → `padActivateBindings`. The port injects its pads (and netplay tapes) at the
decoded word today (`DECOMP_REQUESTS.md` 637-638). A human's car reads input only in `humanCarReadInput`,
`carReadMoveCodes`, `humanCarTick`'s cheats and `carViewHotkeys` (all through `humanCarUpdate`
`car_records.c:74`). `PadSetConfig`'s record map at +0xF8 is separate and per record.

---------------------------------------------------------------------------------------------------------

## 2. The split design

### 2.1 Two indices

- **Human index** `h` (0..`TM2_MAX_HUMANS`-1): the record, `player`, `id` (+0x002), every per-human array, object id
  `TM2_HUMAN_OBJECT_ID(h)`, kill credit, scoring, drop-in/compaction. Unchanged meaning everywhere it is a record index.
- **Local slot** `L` (0..`TM2_MAX_VIEWS`-1, or -1 = remote): view, camera, HUD, radar, pad port (0/1), sound side
  (0/1), hotkey targets, local feedback.

### 2.2 Storage (`.port_bss`, tsv group `one` or `humans`; zero = retail)

```c
/* port_humans.h (new; included by players.h users that need it) */
extern signed char portHumanLocal[TM2_MAX_HUMANS];   /* 0 = default (identity: slot h if h < TM2_MAX_VIEWS, else
                                                         remote); 1..4 = local slot v-1; -1 = remote */
extern unsigned short portHumanPad[TM2_MAX_HUMANS];  /* this frame's buttons, padButtonsDecoded half layout
                                                         (active high), written by the port before each frame */
extern unsigned char portHumanLayout[TM2_MAX_HUMANS];/* control layout 0..3 of each human (padLoadLayout's) */
extern unsigned char portHumanBindings[TM2_MAX_HUMANS][128]; /* action -> button, built from portHumanLayout */
extern unsigned char portHumanLives[TM2_MAX_HUMANS]; /* lives of humans >= 4 (optional, Q11) */
extern int portHumanOptions;                         /* bit 0 INPUT_ALL: every human reads portHumanPad (netplay);
                                                         bit 1 RULES: the port decides round end/results (2.6);
                                                         bit 2 AI_AIM: AI aimed shots at their own target */
#define TM2_HUMAN_LOCAL(h)   /* -> L or -1; identity when the define is off */
int portLocalHuman(int L);   /* -> h or -1 (scan; identity default) */
```

Default (all zero): humans 0..3 are local slots 0..3, humans 4..15 remote. A 1- or 2-human retail match with the
define on therefore runs exactly the retail calls.

### 2.3 What moves to "per local slot"

Everything in 1.2, reached as follows:

- **Cameras:** `worldUpdate` calls `viewCarCameraUpdate(&humanCars[h], L)` only when `L >= 0 && (L == 0 ||
  split)` (retail: `viewMainPlayer == h || split`, `viewMainPlayer` = 0); `viewCarCameraUpdate` passes `L` (not
  `c->player`) to `viewSetRotation/Position`. `viewSetFollowObjPos(target, L)` only for local humans.
- **`carSetViewMode`:** the record part (`view`, `view_last`, `cam_pos/rot`) for every human; `viewSetCameraPose`,
  `viewSetPosition/Rotation` and the global `rtSetRearView`/`rtToggleRearView` only when the human is local, with
  `L`. One gate inside the function covers all its callers (bind, restart, crash, hotkeys, drop-in).
- **Hotkeys:** `carViewHotkeys` returns at once for a remote human (no record-visible state of a remote human comes
  from it except `view`, see 2.5); for a local one `hudToggleRadar(L)`, `hudToggleWeaponList(L)`.
- **HUD:** `rtMainLoop` keeps passing HUD slots; `hudDrawSingleView/SplitVertical/SplitHorizontal(L, buf)` take
  `h = portLocalHuman(L)` for `humanCarGet(h)`, `carGetDisplaySpeed(h)`, lives; `hudDrawTargetPanel(ot, L)` gets a
  `h` for `carNearestToHuman(h)` (a port wrapper `portHudTargetPanel(ot, L, h)` keeps the retail body).
- **Radar:** keep every `rand()` exactly where it is today, keyed by humans (2.5), and only map the target radar:
  `hudRadarPlotCar(.., TM2_HUMAN_HUD(j))`, skipped when j is remote. Retail's pair (≤ 2 humans) plots onto radars
  `TM2_HUMAN_HUD(1)` / `(0)` = 1 / 0 by default.
- **Sound sides:** side = `L` when `L` is 0 or 1, else none: a human without a side plays its turbo/gun/burn loops as
  its car slot's instance (request 33 step 4's mechanism, `car_sound.h:44-52`), positional (`soundLocateForView` /
  listener pan, like the AI's shared gun loop) when remote. Condition in that work: "no side" instead of
  "`player >= 2`".
- **Pads:** local slots 0/1 read pads 0/1 through `padCheckAction(action, L)` (retail); every other human reads its
  `portHumanPad` word (2.4).
- **Local feedback:** HUD messages, non-positional jingles, the denied sound, the turbo start sound from a human's
  own action: only when the human is local (a macro `TM2_HUMAN_FEEDBACK(h)` at each site of 1.3's list).

What stays **per human**: the record (incl. camera fields), `hudSmoothedSpeeds`, `carViewHotkeyReleased`,
`padConfigs`, `carResetWeapons`, ids, kill credit (killer/last attacker are slots), lives (port), layout/bindings of
the human's own inputs (`portHumanLayout/Bindings`).

### 2.4 Inputs of humans without a pad (remote, and local slots 2-3)

Per human, one 16-bit word `portHumanPad[h]` in the decoded half layout (the same bits the port already produces
per pad), written by the port between frames, plus that human's layout `portHumanLayout[h]` (bindings built once by a
port copy of `padLoadLayout` into `portHumanBindings[h]`, `portHumanSetLayout(h, layout)`). Three port functions,
pure (no globals written, no `rand()`):

- `portHumanAction(action, h)`: retail path `padCheckAction(action, L)` when `L` is 0/1 and `INPUT_ALL` clear; else
  action 6 = word != 0, otherwise `portPadButtonInWord(portHumanBindings[h][action], word)` (the switch of
  `padButtonHeld`, factored into a pure helper over a word).
- `portHumanCombo(combo, h)`: retail path `padComboHeld(combo, L)`; else the same 9 compares on the 16-bit word (=
  retail's port-0 result when the other pad is idle).
- `portHumanAnyLatched(h)`: retail path `padAnyLatched(L)`; else word != 0.

Sites: `car_input.c:55` (`ACTION`), `car_tick.c:201-242`, `car_records.c:174`. With `INPUT_ALL` (netplay: local
players' inputs go through the same delayed exchange as remote ones) every human reads its word, local or not.
Analog steering (`ANALOG_STEERING.md`, port overlay) needs per-human analog values the same way (Q2).

### 2.5 Determinism (every machine runs all humans from the same inputs)

On machine A humans 0-1 are local, on B humans 2-5: anything that depends on locality must be presentation only.

1. **`rand()`**: the radar's `rand() % 5` (`world_update.c:211/218`, `:167`, `:415/421/431`, RAND_CALLERS: VISUAL,
   but drawn from the shared stream) must run for the same pairs on every machine. Keep the loops over humans (all
   pairs when > 2 humans; human 0/1 in retail's ≤ 2 path; AI × humans in co-op), gate only `hudRadarPlotCar`.
   No gated function may reach `rand()` (CHECK by call graph, 4.H7).
2. **Record bytes of remote humans that only presentation writes:** `view` (+0x10C), `view_last` (+0x110), the camera
   block (+0x006 `cam_ease`, +0x00C..+0x08F). With cameras and hotkeys for locals only they differ between machines.
   LIKELY no simulation reader (readers found: camera code, `carGetDisplaySpeed` (HUD), `car_crash.c:221` (only
   resets `view`), `humanCarsRestart`'s `view_last`); CHECK the full reader list. The port's state hash / desync check
   should skip them (Q4); the alternative (cameras for every human) changes the shared camera globals
   (`viewCameraLockFrames`, `cameraPrevPos`) seen by the locals.
3. **Globals set from a human's hotkeys** (`viewExtraViewsUnlocked`, split mode, rear view, HUD toggles): local only.
4. Cheats (`humanCarTick`) are game state: they read the human's word on every machine (deterministic), or the port
   turns them off online (Q13).
5. Sound and HUD messages are not hashed (G.7); local-only gating is safe if they reach no `rand()` (point 1).

### 2.6 Rules that fix humans 0/1 (with more than 2 humans or any remote one)

Behind `portHumanOptions` bit 1 (RULES; the C ends nothing, the port decides) and bit 2 (AI_AIM):
- round won (`world_update.c:441`): RULES → never by `humanCars[0]`; the port schedules the end
  (`rtScheduleEvent(3, ..)` from its frame hook, or a port entry `portRoundEnd(result)`);
- `humanCarDamageUpdate` (`car_damage_human.c:330-350`): RULES → no `roundOverFlag`/event at a human death (the port
  respawns: `carRestart` / `portCarStartHuman(.., TM2_DROPIN_START_RETAIL)`-like, lives in `portHumanLives`);
- `weaponFindTargetAhead/Behind` (`weapon_targets.c:34/107`): AI_AIM → `aiGetTargetCar(owner)`'s object;
- `carNearestToHuman` (`ai_steps.c:856`): with bots in deathmatch, nearest of humans and live AI;
- `bulletsUpdate` (`misc_one_liners.c:501`): trace `carPlayerOneObject`'s readers first (CHECK); likely "the first local
  human's object".
Retail ≤ 2 local humans with the bits clear: unchanged.
**As built (H5):** bit 1 as above, `carPlayerOneObject` traced (write-only: nothing to change), the respawn through `portHumanRespawn` (section 4, H5).

---------------------------------------------------------------------------------------------------------

## 3. Memory

| item | at 4 | at 16 | added |
|---|---:|---:|---:|
| `humanCars` (0x814 = 2,068 each) | 8,272 | 33,088 | 24,816 |
| six per-human word arrays (objects, slots, pad configs, types, smoothed speeds, hotkey flags) | 96 | 384 | 288 |
| `carResetWeapons` (moved) | (retail 4) | 16 | 16 |
| `sessionPortHumanCars` | 2 | 14 | 12 |
| `portHumanLocal`, `portHumanLayout`, `portHumanLives` | | 48 | 48 |
| `portHumanPad` | | 32 | 32 |
| `portHumanBindings` (128 each) | | 2,048 | 2,048 |
| `portHumanOptions` | | 4 | 4 |
| **total** | | | **~27.3 KB** of `.port_bss` |

Per-view/HUD storage unchanged (4). Stack: `portCarSoundFrame`'s `slot_of`/`dist` grow with `TM2_MAX_HUMANS` (+96
bytes). The port sections end ~0x80587B98 with every define (RAM end 0x80C00000): room is ample. Everything after
`humanCars` in `.port_bss` moves (re-read `symbols.tsv`). Code: ~2 KB of port functions.

---------------------------------------------------------------------------------------------------------

## 4. Ordered steps (each its own PR)

All under `--port-define TM2_PORT_HUMANS=N` (5..16; needs `TM2_PORT_VARIANT` and `TM2_PORT_CARS`, `#error`
otherwise: 16 humans need the 64 car slots, 32 load groups and the 16 AI records of step 2). `players.h`:
`TM2_PORT_HUMANS` sets `TM2_MAX_HUMANS = N`; `TM2_MAX_VIEWS`/`TM2_MAX_HUDS` stay 4. Steps H1-H3 also build and
mean something at N = 4 (2 local + 2 remote), so they can land before the raise. Retail-equal rule for every step:
with ≤ 4 local humans, the default map, `portHumanOptions` = 0 and no remote human, the game logic and `rand()` calls
are the retail ones (the 11-car comparison stays exact; only addresses and the setup loops differ, as today).

**H0. Macros and the id fix** (`TM2_PORT_HUMANS` define introduced, nothing raised yet). **DONE 2026-10-14**
(branch `decomp-humans-h0`; "As built" below the list).
- `players.h`: `TM2_PORT_HUMANS` → `TM2_MAX_HUMANS`; `TM2_HUMAN_OBJECT_ID`, `TM2_HUMAN_INDEX_OF_ID`; the new id
  `#error`; `#error` unless `TM2_PORT_CARS`; `#if TM2_MAX_HUMANS > TM2_PORT_BIND_HUMANS #error`.
- Functions: `carBindHumanSlot` (`scene_init.c:96`), `UpdateGuns` (`update_guns.c:32`), `weapon_basic.c:138`,
  `weapon_helpers.c:92`, `weaponFireSpecial`-family `weapon_special.c:77`, `weaponDropCheckHit`
  (`car_damage_human.c:70`), `portCarBindSlot` (`port_variant_arrays.c:2936`), `portDropInId` (`:4026`),
  `portHumanCompact` (`:4261`). Export `int portHumanOfId(int id)` (-1 if not a human) for the port's kill feed.
- Hazard: all owner-id producers must change together (a projectile from `player + 1` with the bind's id differing
  would not be found by `carFindById`); the static check lists every `player + 1`. CHECK the id scan of 1.4.
- Retail-equal: identical for humans 0..6.
- **As built:** `players.h`: `TM2_HUMAN_ID` / `TM2_HUMAN_INDEX_OF_ID` (1.4); `-DTM2_PORT_HUMANS=N` sets
  `TM2_MAX_HUMANS` = N, `#error` without `TM2_PORT_CARS`, outside 4..16, and **above 4 until H1-H4** (views, HUDs,
  pads, sound sides and `carResetWeapons` are still per local player or retail-sized); more than 7 records without the
  define is an `#error`; the id `#error` (last id >= 50); `#if TM2_MAX_HUMANS > TM2_PORT_BIND_HUMANS #error` in
  `port_variant_arrays.c`. Sites: `carBindHumanSlot`, `UpdateGuns`, `weaponFireSpecial` (`weapon_basic.c:141`),
  `weaponDropItem` (`weapon_helpers.c:92`), `weaponFireEnergyAttack` (`weapon_special.c:77`), `weaponDropCheckHit`
  (the inverse), `portCarBindSlot`, `portDropInId`, `portHumanCompact` (both ids). `portHumanOfId` is in **every** port
  variant (the kill feed needs it with 1-4 players too). No game function changes at N <= 7 (the proofs: the variants'
  game code is byte-for-byte that of the same defines without `TM2_PORT_HUMANS`; retail images unchanged). The ids past
  7 records are checked by `tools/check_humans.py` (macros read back from the preprocessor at 2, 4, 7, 8 and 16
  records, `TM2_MAX_HUMANS` predefined for 8/16; the `#error` cases; no other `player + 1` / `owner - 1` in the C)
  and a build of every unit with `TM2_MAX_HUMANS=16` predefined.
- **H0 proofs** (`verify_reloc.py` vs E0, placement check with `--port-limits 2`): default variant `21427706…`
  (port end 0x80543120; was `22139e4b…`: only `portHumanOfId` added, no game function); `TM2_PORT_CARS=16` with and
  without `TM2_PORT_HUMANS=4` both `148d4f6c…`; the car stack + `TM2_PORT_HUMANS=4` `b7308882…`; every port define
  with and without `TM2_PORT_HUMANS=4` both `58c5c034…` (port end 0x8058DDA8); every define + `TM2_PORT_HUMANS=4
  TM2_MAX_HUMANS=16` builds and proves (`5fdc8bb7…`, port end 0x805940A8; not runnable before H1-H4). Retail
  images unchanged: hybrid `9219f7ba…`, source-only `d755c86e…`, `--sdk-from-lib` `f557c1e3…`.

**H1. The local map: cameras, views, HUD, radar** (`portHumanLocal`, `portLocalHuman`). **DONE 2026-10-14** (branch `decomp-humans-h1`; "As built" below).
- Functions: `worldUpdate` (`world_update.c:174`: camera test `:229`, follow `:244`, retail radar pair `:207-220`, AI
  blips `:411-433`), `worldRadarPlotHumans` (`:156`), `viewCarCameraUpdate` (`camera.c:47`), `carSetViewMode`
  (`weapon_missile_trails.c:395`), `humanCarsRestart` (`arena_setup.c:233`, via the gate), `carViewHotkeys`
  (`car_records.c:104`), `hudDrawSingleView` (`hud_main.c:78`), `hudDrawSplitVertical`/`Horizontal`
  (`hud_split.cpp:112/249`), `hudDrawTargetPanel` (`car_destroyed.c:433`, via a port wrapper taking (L, h)).
  `portHumanCompact` moves `portHumanLocal[from]` to `to` (then the port's view/HUD/pad remap of `CAR_GROUPS.md` 14.5
  is no longer needed: views and HUDs follow the slot, not the index).
- Hazards: `hudDrawSingleView` etc. are byte-matched retail functions: keep the retail body for the default map
  (`#if` around the actor lookup only); `viewMainPlayer` stays 0 (meaning "slot 0"); the rand rule of 2.5.1;
  remote humans' record camera bytes diverge (Q4).
- **As built (2026-10-14, branch `decomp-humans-h1`; everything under `--port-define TM2_PORT_HUMANS`):**
  - **API** (`include/semantic/port_humans.h`). Storage `signed char portHumanLocal[TM2_MAX_HUMANS]`, `.port_bss`
    right after `portArenaLive`, in **every** variant (tsv group `humans`, layout `new`), so the layout does not
    depend on the define. Entry per human record: **0** = default (slot h for h < `TM2_MAX_VIEWS`, remote past it:
    humans 0..3 on slots 0..3 = retail); **1..4** = local slot 0..3; **< 0** (or > 4) = remote. Addresses: see
    `symbols.tsv` / `port_arrays.tsv` of the build ("H1 proofs" below gives them for each configuration). Port
    functions (`.port_text`, only with the define; all pure, no `rand()`): `portHumanSlot(h)` (slot or -1; no
    presence test, the binding uses it before `humanCarTotal` counts h), `portLocalHuman(L)` (the first present
    human, index < `humanCarTotal`, on slot L, or -1), `portLocalHumanCount()` (slots 0..3 with a present human:
    the views and HUDs this machine shows; the port picks its split mode from it), `portHumanRadarPlot(type, delta,
    angle, h)` (`hudRadarPlotCar` on h's slot, nothing for a remote h). Macros `TM2_HUMAN_LOCAL(h)` /
    `TM2_LOCAL_HUMAN(L)` (identity without the define).
  - **How the port sets local slots:** write `portHumanLocal[h]` for every human before the frame that needs it
    (between frames; at match setup, and before `portCarStartHuman` for a drop-in: the bind's `carSetViewMode`
    already uses it). Keep the slots unique (with two humans on a slot both cameras write it; the HUD shows the first).
    A machine with remote humans sets them `-1` (or leaves humans >= 4 at 0). `portHumanCompact(from, to)` moves the
    slot with the record (written explicitly, `slot + 1` or -1) and sets the vacated index to -1, so
    `CAR_GROUPS.md` 14.5's view/HUD remap is no longer needed with the define (the port's pad remap still is
    until H2). The split mode (how many views) stays the port's: `portLocalHumanCount()`.
  - **Functions** (8 game functions change, 2 static helpers and 4 port functions in `.port_text`):
    `worldUpdate` (camera through `worldHumanCamera(i)`: slot L >= 0 and (`viewMainPlayer == L` or split), then
    `viewCarCameraUpdate(rec, L)`; sky follow through `worldHumanFollow(target, i)`, local only; the 5 radar plots
    through `portHumanRadarPlot`; every `abs() < 400 || rand() % 5` test stays exactly where it was, for every
    human pair and AI x human: the plot alone is gated); `worldRadarPlotHumans` (port code already; its plot);
    `viewCarCameraUpdate` (`viewSetRotation/Position(.., i)`, i = the slot, instead of `c->player`);
    `carSetViewMode` (the record part, `view`, `view_last` and the camera fields, for every human; `viewSetCameraPose`,
    `viewSetPosition/Rotation` on the slot and the rear-view calls only for a local human; `humanCarsRestart`,
    `carBindHumanSlot`, `car_crash.c`, the attract/watch calls and `portCarStartHuman` all go through this gate);
    `carViewHotkeys` (returns for a remote human; `hudToggleRadar/WeaponList(slot)`, so no more writes past the
    u8[4] `hudRadarVisible`/`hudWeaponListVisible`); `hudDrawSingleView`, `hudDrawSplitVertical`,
    `hudDrawSplitHorizontal` (`human = portLocalHuman(player)`, no HUD when -1; `humanCarGet`, `carGetDisplaySpeed`,
    `gameGetLives` of that human; records, OTs, radar, blink state stay by slot; co-op team lives stay `lives[0]` on
    HUD 0, retail's rule); `hudDrawTargetPanel` (slot i, `carNearestToHuman(portLocalHuman(i))`).
    `worldUpdate`'s frame does not change (0xB0 with `TM2_PORT_CARS=16`; the car stack's 0xB8 is older than H1,
    `FX_SPRITE_STACK_POINTER.md`): the camera and follow gates are out-of-line helpers. Only `hudDrawSingleView`
    grows (0x78 to 0x80), outside the simulation (`rtMainLoop`, after `worldUpdate`).
  - **Camera-only record bytes** (remote humans' differ between machines; the hash and rollback may skip them):
    +0x006 `cam_ease`, +0x00C..+0x07F (`cam_rot`, `cam_pos`, `cam_goal`, `cam_look`, the chase / split offsets),
    +0x080 `cam_look_offset`, +0x10C `view`, +0x110 `view_last`; plus the per-human `carViewHotkeyReleased[h]` (only
    `carViewHotkeys` reads it, and it no longer runs for remote humans). **Not camera: +0x08C..+0x08F is
    `speed_change`** (`humanCarTick` `car_tick.c:79` writes `speed - old_speed` every tick, `carBodySwayUpdate`
    `ai_route_nodes.c:222` reads it later in the same call: simulation, but a per-tick temporary, so skipping it in
    the hash and the snapshot is harmless; the exact camera range is +0x00C..+0x08B). The unnamed bytes +0x008..+0x00B,
    +0x012..+0x013, +0x020..+0x02B are padding in every C view of the record (no reader found).
  - **Reader check 2.5.2 (finished; C census over every struct view of the 0x814-byte record, the `cam_*`, `view`,
    `view_last`, `speed_change` names and raw offsets):** no simulation reader of the camera bytes, `view` or
    `view_last`. Readers: the camera (`camera.c`), `carSetViewMode` (`view` = mode test for the camera copy),
    `carViewHotkeys`, `carGetDisplaySpeed` (HUD: view 7 shows the watched AI's speed), `humanCarsRestart` (`view_last`
    → `carSetViewMode`), `car_crash.c:221` (view 3/4 on ground class 1 → `carSetViewMode(2)`: camera only),
    `carApplyDamage` `car_damage.c:159` (`dead = view`: a dead read kept for matching, never used), the attract demo
    (`demoWatchNextAiCar`, `worldUpdate`'s `f_014/f_018` follow target), `viewCameraOffsetsInit` / `carRestart`
    (writes). The shared camera globals `viewCameraLockFrames` and `cameraPrevPos` are read only in `camera.c`.
    **One locality-dependent simulation read found outside those bytes:** `humanCarTick` `car_tick.c:153` (repair-zone
    regeneration gives ammo beyond capacity when `viewGetSplitMode() != 0`). In retail the split mode is 1/2 exactly
    in game modes 2 and 4 (`sessionRun`, the hotkey toggles only 1 ↔ 2), but if the port sets the split mode per
    machine from its local players, two machines differ: a desync. Not changed in H1 (open question; the
    retail-equal fix is `gameGetMode() == 2 || gameGetMode() == 4` under the define). Every other split-mode reader
    is sound placement, HUD layout, camera offsets or view creation.
  - **Determinism:** `tools/check_humans.py` check 5: no `rand()` is reachable from the local-only code (the map
    functions, the gated camera / follow / plot, `carViewHotkeys`, `carSetViewMode`, `viewCarCameraUpdate`, the HUD
    draw functions and the target panel; jal call graph). Nothing local-only is written into the simulation: the
    gated code writes only the camera bytes above, view / HUD / radar records and the local globals (rear view, split
    toggle, `viewExtraViewsUnlocked`, HUD toggles).
  - **Retail-equal:** with `portHumanLocal` all zero every map call returns retail's index (humans 0..3), so the
    1-4 local human paths make the retail calls with the retail arguments. Check 4 lists every call with a view /
    HUD / radar index (67 calls, 49 sites) against the reviewed list; without the define the same scan finds the 9
    retail human-indexed calls (negative control).
  - **H1 proofs** (`verify_reloc.py` vs E0 of `work/build-20261014-192519-936605`, placement check with
    `--port-limits 2`; all pass): default variant `274d0ae4…` (unchanged: `portHumanLocal` is NOLOAD storage
    nobody references; port end 0x805431C0, `portHumanLocal` at 0x80542FF4); `TM2_PORT_CARS=16` `b7fb2bfc…`;
    `TM2_PORT_CARS=16 TM2_PORT_HUMANS=4` `6ce103dd…` (port end 0x80581D50; `portHumanLocal` 0x8057F4A4,
    `portHumanSlot` 0x8058198C, `portLocalHuman` 0x805819E0, `portLocalHumanCount` 0x80581A68); against the same
    build without the define exactly the 8 H1 functions differ in the placement check (at 4 limits 4 of them, the
    others already differ by the moved arrays); the car stack (+ `TM2_PORT_DT_PLAYER`) + `TM2_PORT_HUMANS=4`
    `77ddfb27…` (port end 0x8058D608); every port define + `TM2_PORT_HUMANS=4` `845e0492…` (port end 0x8058F73C,
    `portHumanLocal` 0x8057F504, `portHumanSlot` 0x8058D9F0); every define + `TM2_PORT_HUMANS=4
    TM2_MAX_HUMANS=16` builds and proves (`4b1b0dbb…`, port end 0x80595A5C). `tools/check_humans.py` checks 1-5 pass
    on all of them. Addresses move with every later `.port_bss` / `.port_text` change: read `symbols.tsv`.

**H2. Inputs** (`portHumanPad`, `portHumanLayout`, `portHumanBindings`, bit 0). **DONE 2026-10-14** (branch `decomp-humans-h2`; "As built" below).
- Port functions `portHumanAction`, `portHumanCombo`, `portHumanAnyLatched`, `portHumanSetLayout`,
  `portPadButtonInWord`; sites `car_input.c:55` (`humanCarReadInput` `:95`, `carReadMoveCodes` `:235`),
  `humanCarTick` (`car_tick.c:36`, combos `:201-242`), `carViewHotkeys` (`car_records.c:174`).
- `portHumanCompact` moves the word, layout and bindings with the record.
- Hazards: `padCheckAction` caches into `padActionCache` (retail path only); combos on port 0 compare the 32-bit
  latch (retail path keeps it); the port must write the words before `humanCarUpdate` runs (`worldUpdate` may run
  twice per frame: one word per frame, as the latch). Replaces the port's own overlay for pads 3-4.
- **As built (2026-10-14, branch `decomp-humans-h2`; everything under `--port-define TM2_PORT_HUMANS`):**
  - **Who reads what.** A human on local slot 0 or 1 reads pad 0 / 1 with retail's calls (`padCheckAction`,
    `padComboHeld`, `padAnyLatched` with the slot as the port: slot 0 combos still compare the whole 32-bit latch),
    unless `portHumanOptions` bit 0 (INPUT_ALL) is set. Every other human (local slots 2-3, remote humans, and every
    human with INPUT_ALL) reads its own input word through its own binding table. Offline (bit 0 clear) slots 0/1
    keep the pads and slots 2-3 read words the port fills from pads 3-4 (this replaces the port's overlay of
    `humanCarReadInput` / `padCheckAction` for players 3-4); online the port sets bit 0, so local humans go through
    the same delayed exchange as remote ones and no human reads a local pad.
  - **Storage** (`include/semantic/port_humans.h`; `.port_bss` right after `portHumanLocal`, defined in **every**
    variant so the layout does not depend on the define; tsv rows group `one` / `humans`, layout `new`):
    - `int portHumanOptions`, 0 = retail. Bit 0 `TM2_HUMAN_OPT_INPUT_ALL` every human reads its word; bit 1
      `TM2_HUMAN_OPT_RULES` reserved for H5; bit 2 `TM2_HUMAN_OPT_AI_AIM` a bot's aimed shot at its own target;
      bit 3 `TM2_HUMAN_OPT_NO_CHEATS` cheats off; bit 4 `TM2_HUMAN_OPT_RADAR_AI` AI blips on every local human's radar
      in every game mode; bit 5 `TM2_HUMAN_OPT_LIVES` `gameGetLives` reads `portHumanLives` for every human.
    - `unsigned short portHumanPad[N]`: **the input word**, this frame's buttons of human h in `padButtonsDecoded`'s
      16-bit half layout, active high: bit 0 L2, 1 R2, 2 L1, 3 R1, 4 triangle, 5 circle, 6 cross, 7 square,
      8 select, 9 L3, 10 R3, 11 start, 12 up, 13 right, 14 down, 15 left (the bits the port already produces per pad).
    - `short portHumanSteer[N]`: the stick tilt 0..4096 (4096 = full, or a digital press). The C never reads it;
      the port's analog hook around `carThrottleSteer` (scaling `stats.steer_limit`) reads it for every human.
    - `unsigned char portHumanLayout[N]`: the control layout 0..3 (`padLayoutTable`'s columns), filled by the port
      from the match setup, **for every human** (humans 0/1 too: with bit 0 they read words through it).
    - `unsigned char portHumanBindings[N][128]`: action -> button (1..14, `padButtonHeld`'s numbering), built by
      `portHumanSetLayout(h, layout)` exactly as `padInit` + `padSetLayout` build a pad's active table (cleared,
      `padDefaultBindings`' 22 pairs, the layout's 43 rows). `sessionRun` builds every human's table at the match
      start (`portHumanSetLayouts()`, right after retail's `padSetLayout` of pads 0/1), `portCarStartHuman` at a
      drop-in (from `portHumanLayout[i]`, which the port writes before the call). The port may overwrite single
      entries afterwards (its per-player buttons; wrap `portHumanSetLayout` as it wraps `padSetLayout` today), the
      same on every machine.
    - `unsigned char portHumanLives[N]`: lives per human. `gameGetLives(p)` reads it for p >= 4 always (the settings
      hold 4) and for every p with bit 5; with bit 5 clear humans 0..3 read settings `lives[]` (retail; co-op team
      lives stay `lives[0]`). The runner's `--lives[0]` is unchanged (rules: H5).
    - `unsigned int portHumanRadarBlink`: bit 4's blink clock (written by the C, presentation only).
  - **Port functions** (`.port_text`, only with the define; none calls `rand()`, check 5): `portPadButtonInWord(b, w)`
    (`padButtonHeld`'s switch on one word); `portHumanAction(action, h)` (pad path `padCheckAction(action, slot)`;
    word path action 6 = word != 0, else `portPadButtonInWord(portHumanBindings[h][action], word)`, actions >= 128
    0); `portHumanCombo(combo, h)` (pad path `padComboHeld(combo, slot)`; word path the nine compares on the 16-bit
    word = retail's pad-0 answer with the other pad idle); `portHumanAnyLatched(h)` (pad path `padAnyLatched(slot)`;
    word != 0); `portHumanSetLayout(h, layout)` (layout >= 4 ignored, as `padSetLayout`), `portHumanSetLayouts()`;
    `portHumanCheatsOff()`, `portHumanCheatCode(code)`; `portAiAimTarget(owner)`; static `portHumanPadPort(h)`
    (slot 0/1 with bit 0 clear, else -1) and `worldRadarAiAll()` (world_update.c).
  - **Game functions** (7 new + 2 of H1 again): `humanCarReadInput` and `carReadMoveCodes` (the `ACTION` macro is
    `portHumanAction((u8)a, (s16)player)`; `carReadMoveCodes` also consumes a completed cheat code with no effect when
    the cheats are off); `humanCarTick` (the cheat combos through `portHumanCombo` / `portHumanAnyLatched`, the whole
    cheat block skipped when `portHumanCheatsOff()`; the repair-zone ammo test, see below); `carViewHotkeys` (combo 1
    through `portHumanCombo`); `gameGetLives`; `weaponFindTargetAhead` / `weaponFindTargetBehind` (an AI owner's shot
    at `portAiAimTarget(owner)`: human 0's object with bit 2 clear (retail), else the object of `aiGetTargetCar`'s car,
    an AI's or the nearest human's, or none (the shot goes straight) without one: nothing assumes human 0 exists);
    `sessionRun` (`portHumanSetLayouts()`); `worldUpdate` (`worldRadarAiAll()` after the AI loop). `portHumanCompact`
    moves word, tilt, layout, bindings and lives with the record (the vacated word and tilt cleared), so the port's pad
    remap of `CAR_GROUPS.md` 14.5 is gone too.
  - **Cheats** (answer 13): bit 3 turns off `humanCarTick`'s combos (armour, HP, turbo, infinite weapons; one
    `return` before the block) and `carReadMoveCodes`' three cheat codes 0x12233344 (Mega Napalm), 0x21431122
    (health), 0x12344321 (god mode / infinite weapons / Mega Guns): consumed (count and code cleared) with no effect,
    message or jingle. The special moves 100..106 and rear fire stay. Check 7: the gate precedes the first combo read,
    and every code of the move switch that posts a HUD message is one of the three.
  - **Repair-zone ammo (H1 review, `car_tick.c`):** retail gives ammo beyond the capacity while `viewGetSplitMode() !=
    0`. Retail's split mode is non-zero exactly in game modes 2 and 4 (`sessionRun` sets 1/2 from the mode right
    before `gameSetMode`, the only other writer, `carViewHotkeys`, toggles 1 <-> 2 while it is non-zero, and the attract
    demo runs mode 0 with split 0), so the define tests `gameGetMode() == 2 || gameGetMode() == 4`: equal to retail
    in every retail situation, and the same on every machine whatever split mode the port picks per machine. (A port
    match that runs split screen in another game mode now follows the mode, as retail would.)
  - **AI radar blips (H1 review):** retail plots each live AI car on human 0's radar, and in co-op (mode 2) on the
    others' (variant: humans 2.. too), each behind its own `abs(dz) < 400 || rand() % 5 == 0`. Outside mode 2 humans
    1..3 get no AI blips, so "every local human" would differ from retail for humans 1..3 with the default map: it is
    behind bit 4. With the bit, outside mode 2, `worldRadarAiAll()` (after the AI loop) plots every live AI car on
    each local human 1.. with `abs(dz) < 400 || (portHumanRadarBlink + i) % 5 == 0` (the clock steps once per
    `worldUpdate`): no `rand()`, every retail plot and draw unchanged, drawing only. Mode 2 and human 0 keep retail's.
  - **Lockstep and rollback.** Inputs are the only per-frame values: the port writes, **before each simulation
    frame** (before `rtMainLoop`'s `padUpdate` / `worldUpdate`; the match's first `worldUpdate`, before the loop,
    sees the words as set then), for every human h < `humanCarTotal`: `portHumanPad[h]` and `portHumanSteer[h]` of
    that frame (online: the exchanged inputs of frame F, local players' included; offline: slots 2-3 from pads 3-4).
    One word per frame, as the latch. Per match (before `sessionRun`, or before `portCarStartHuman` for a drop-in):
    `portHumanLayout[h]`, `portHumanLives[h]`, `portHumanOptions` (online: bits 0 and 3 at least), then any binding
    overrides after the match start. For **rollback**: the words and tilts are inputs (rewrite them for each
    re-simulated frame, no need to snapshot); `portHumanBindings`, `portHumanLayout`, `portHumanLives` and
    `portHumanOptions` are match state (snapshot them, or keep them constant within the match);
    `portHumanRadarBlink` is presentation (skip it). With bit 0 clear a human on slot 0/1 reads a local pad: only
    for offline play.
  - **Retail-equal:** with `portHumanOptions` 0 and the default map, humans 0 and 1 make exactly retail's pad calls
    (`padCheckAction(a, 0/1)`, `padComboHeld(n, 0/1)`, `padAnyLatched(0/1)`), so 1-2 local humans are retail. Humans 2
    and 3 read their words (answer 5): retail's own reads for them were broken (human 2 "either pad" through port 0's
    configured table, human 3 nothing; `FOUR_HUMANS.md` 3), and the port's overlay already fed them their own buttons
    as port 0 with the other half idle, which is what the word path computes. All other changes are behind bits.
  - **Checks** (`tools/check_humans.py`): check 6 lists every pad read (137 calls, 50 sites) with its port argument
    against a reviewed list: menus, match start, the attract demo and the pad layer read this machine's pads, the
    input functions' retail path the port of `portHumanPadPort`, and the human record index reaches a pad only
    through `portHumanAction` / `Combo` / `AnyLatched` (negative control: without the define the scan finds retail's
    5 human-indexed reads). Check 7: the cheat gates. Check 5 now also covers H2's port functions (in `.port_text`,
    no `rand()` reachable) and H2's game functions (changed). `c_bodies` now sees functions inside `extern "C"`
    blocks (C++ units), which added `screen_ending.cpp`'s `sceneRun` view calls (slot 0, the shell) to check 4.
  - **H2 proofs** (`verify_reloc.py` vs E0 of `work/build-20261014-212753-204592`, placement check with
    `--port-limits 2`; all pass): default variant `5e54710d…` (was `274d0ae4…`: the new `.port_bss` storage moves
    `.port_text`, so `portHumanOfId` moved and its one reference with it; no game function changes; port end
    0x805433E0); `TM2_PORT_CARS=16` `8ab3396c…`; `TM2_PORT_CARS=16 TM2_PORT_HUMANS=4` `f3dfa2f1…` (port end 0x8058279C);
    against the same build without the define exactly the 15 H1 + H2 game functions differ in the placement check
    (`carReadMoveCodes`, `carSetViewMode`, `carViewHotkeys`, `gameGetLives`, the three HUD draws,
    `hudDrawTargetPanel`, `humanCarReadInput`, `humanCarTick`, `sessionRun`, `viewCarCameraUpdate`,
    `weaponFindTargetAhead`, `weaponFindTargetBehind`, `worldUpdate`); the car stack (+ `TM2_PORT_DT_PLAYER`) +
    `TM2_PORT_HUMANS=4` `f765f616…` (port end 0x8058E124); every port define + `TM2_PORT_HUMANS=4` `77f0f366…`
    (port end 0x8059024C); every define + `TM2_PORT_HUMANS=4 TM2_MAX_HUMANS=16` builds and proves (`def687ee…`, port
    end 0x80596BAC). `tools/check_humans.py` checks 1-7 pass on all of them.
  - **Addresses** (read `symbols.tsv` of the build: every later `.port_bss` / `.port_text` change moves them).
    `TM2_PORT_CARS=16 TM2_PORT_HUMANS=4`: `portHumanLocal` 0x8057F4A4, `portHumanOptions` 0x8057F4A8,
    `portHumanPad` 0x8057F4AC, `portHumanSteer` 0x8057F4B4, `portHumanLayout` 0x8057F4BC, `portHumanLives`
    0x8057F4C0, `portHumanBindings` 0x8057F4C4, `portHumanRadarBlink` 0x8057F6C4; `portPadButtonInWord` 0x80581D88,
    `portHumanAction` 0x80581E60, `portHumanCombo` 0x80581F10, `portHumanAnyLatched` 0x80582020,
    `portHumanSetLayout` 0x80582084, `portHumanSetLayouts` 0x8058215C, `portHumanCheatsOff` 0x805821A4,
    `portHumanCheatCode` 0x805821BC, `portAiAimTarget` 0x80582224. Every port define + `TM2_PORT_HUMANS=4`:
    `portHumanLocal` 0x8057F504, `portHumanOptions` 0x8057F508, `portHumanPad` 0x8057F50C, `portHumanSteer`
    0x8057F514, `portHumanLayout` 0x8057F51C, `portHumanLives` 0x8057F520, `portHumanBindings` 0x8057F524,
    `portHumanRadarBlink` 0x8057F724; `portHumanAction` 0x8058DF94, `portHumanSetLayout` 0x8058E1B8,
    `portAiAimTarget` 0x8058E358.

**H3. Sound sides and local feedback.** **DONE 2026-10-15** (branch `decomp-humans-h3`; "As built" below).
- Turbo/gun/loop 32 by `L` (0/1) instead of `player`: `carThrottleSteer` (`car_control.c:21-26, 181`), `weaponTick`
  (`weapon_tick.c:71-84`), `carSoundUpdate` (`screen_card_draw.c:31`, `:92-99`); humans without a side through
  request 33 step 4's slot instances (coordinate: that work is in progress), remote ones positional.
- `TM2_HUMAN_FEEDBACK(h)` at the ~45 non-positional feedback sites of 1.3 (list them with a census script:
  `hudMessagePost`, `soundPlay(.., 0, 0, ..)`, `soundSetDeniedPending` reached from a human branch).
- `portCarSoundFrame`'s cap: distance to local humans.
- Hazards: sound must stay rand-free when gated (RAND_CALLERS: `sound_repeat_throttle` 800D3B34 is VISUAL but on the
  shared stream: CHECK it is not reached from a gated call); retail sides 0/1 unchanged for the default map.
- **As built (2026-10-15, branch `decomp-humans-h3`; everything under `--port-define TM2_PORT_HUMANS`):**
  - **API** (`include/semantic/port_humans.h`; port functions in `.port_text`, only with the define; none writes
    simulation state, none reaches `rand()`, check 5): `portHumanSide(h)` (h's sound side: its local slot when that
    is 0 or 1, else -1), `portHumanFeedback(h)` (1 when h is local: `portHumanSlot(h) >= 0`), `portHumanMessage(h,
    msg, prio)` / `portHumanSound(h, snd, vol, pan, pitch)` / `portHumanDenied(h)` (`hudMessagePost` / `soundPlay` /
    `soundSetDeniedPending(1)` only when h is local), `portHumanLoopPlay` / `portHumanLoopStop` (the loops by side;
    with the define these are H3's, with `TM2_PORT_CAR_SOUND` alone request 33-4's), static `portHumanLocate` (a
    remote human's car located for the listener). Game-side macros (retail's own calls without the define, so the
    units include `port_humans.h` always): `TM2_HUMAN_FEEDBACK(h)`, `TM2_CAR_FEEDBACK(human, h)` (a human record
    whose human is local), `TM2_CAR_POSITIONAL(human, ai)` (the computer cars' positional branch, a remote human's
    too), `TM2_HUMAN_MESSAGE`, `TM2_HUMAN_SOUND`, `TM2_HUMAN_DENIED`; `TM2_PORT_SHARED_GUN(slot)` = `0x100 + slot`
    (a remote human's instance in the shared gun loop; the computer cars use their AI index 0..15). No new storage:
    `portCarSoundLoops` (every variant) gets bit 0x10 `TM2_PORT_LOOP_SHARED_GUN` (`car_sound.h`).
  - **Sound sides.** `carThrottleSteer` (turbo 0), `weaponTick` (gun 9) and `carSoundUpdate` (burn / lava 32) call
    `portHumanLoopPlay` / `portHumanLoopStop` whenever `TM2_PORT_HUMANS` or `TM2_PORT_CAR_SOUND` is defined. A human
    with a side makes retail's `soundStartPlayerLoop(.., side)` / flag test + `soundStopPlayerLoop(.., side)` (the
    default map gives humans 0 and 1 sides 0 and 1: retail's calls). A human without a side:
    - a **remote** human's **gun**: the computer cars' shared gun loop (`soundStartSharedLoop(9, distance, pan, 99,
      0x100 + slot)`, located at its car with weaponTick's rule for computer cars: pan only in one view), stopped
      per car (bit 0x10) and by retail's stop-all (`carDestroyed`, round end) as theirs;
    - with `TM2_PORT_CAR_SOUND`: request 33-4's instance slot + 1 in the id's own list: a **local** human's (slot
      2-3) at the caller's values (full volume: the player's own loop), a **remote** human's turbo at the distance
      and pan of its car, its burn loop at `carSoundUpdate`'s values (positional already);
    - without `TM2_PORT_CAR_SOUND` turbo and burn have no list for a human without a side: not played (retail leaked
      a sound request every frame for a human of index 2 or more, `FOUR_HUMANS.md` 3).
    The stop first makes the side's retail calls, then stops whatever the slot started without a side (instance,
    shared gun), so a human whose slot changes leaves nothing playing. This answers `CAR_GROUPS.md` 16.7
    question 1 (positional, as computer cars; not silent).
  - **Local feedback** (request 37 answer 7; census: `tools/check_humans.py` check 8 lists every full-volume sound,
    HUD message, denied buzz and side call of the match code with what it is). Dropped for a remote human:
    - HUD messages: `carReadMoveCodes` (8: the move / cheat messages), `humanCarTick` (2: repair zone),
      `pickupApply` (11: ammo / health / turbo pickups, "full"), `targetActorsUpdate` (the 10th run-over);
    - jingles and cues: `carReadMoveCodes` (3 × 0x18), `carThrottleSteer` (turbo launch 1, turbo empty / burning
      0x3F ×2), `carDestroyed` (34, level 6 splash), `targetActorsUpdate` (3 run-over, 24 the 10th);
    - the denied buzz (`soundSetDeniedPending`): `weaponFireSelected` (3), `weaponFireEnergyAttack` (6),
      `pickupApply` (full); `worldUpdate` plays a pending buzz once per frame as retail.
    Played **positionally** for a remote human (retail's computer-car branch: located at the car, distance and pan)
    instead of at full volume: the special-fire sounds of `weaponFireSpecial` (`SOUND` macro: 0x27, 0x29, 0x2B,
    0x2D, 0x31, 0x37, 0x3B, 0x3D; Roadkill's boomerang 0x33; the Twister's tornado 0x35; Mega Napalm's 13),
    `weaponFireEnergyAttack`'s freeze missile (13), `weaponFlameStart` (Thumper's flame 47; with
    `TM2_PORT_CAR_SOUND` the human's own slot), `carCrashStart` (Hammerhead's crush 37: full volume when a local
    human is either car), the slam swing / grab cues (`weaponSlamSwingUpdate`, `weaponSlamGrabUpdate`), and the
    Paris lamps' cue (`carParisSinkZonesUpdate`: `soundRepeatThrottle`, which draws `rand()`, is called exactly as
    retail; only the branch after it differs).
    Kept as retail (not a human's own feedback, reviewed in check 8): `carApplyDamage`'s cars-left message (match
    state), `carBarrelRollStep`'s tumble (6: full volume for an active computer car too), `carDestroyed`'s wreck (25:
    full volume within 1000 of the listener), the boss's and the Dark Tooth player's swoop sounds (the boss's rule),
    the skid / shield hum (21, positional, instance = human index), the pickup sound (0x18, positional), menus.
  - **Listeners.** `sndListenerTransform` (8015DBE8, engine code) with view -1 (split screen) also takes views 2..3
    when a local human is on them (`portLocalHuman(v) >= 0`), keeping the nearest as for views 0/1 (which stay
    retail's). Views are slots since H1, so the listeners are the local humans' views (at most 4). No simulation
    code reads the listener values (`PadChkMtap` / `soundGetListenerPan` feed only sound calls; the one comparison,
    `carDestroyed`'s `< 1000`, picks a volume).
  - **Engine-voice cap** (answer 12): `portCarSoundFrame` ranks the cars by their distance to the nearest **local**
    human (remote humans' cars stay candidates; with no local human the playing engines rank first, then record order
    decides).
  - **Functions** (17 game functions change, all only where a human's sound / message is played; plus 7 port
    functions and 1 static helper in `.port_text`): `carThrottleSteer`, `weaponTick`, `carSoundUpdate` (sides),
    `carReadMoveCodes`, `humanCarTick`, `pickupApply`, `targetActorsUpdate`, `weaponFireSelected`,
    `weaponFireEnergyAttack`, `carDestroyed` (gated feedback), `weaponFireSpecial`, `weaponFlameStart`,
    `carCrashStart`, `weaponSlamSwingUpdate`, `weaponSlamGrabUpdate`, `carParisSinkZonesUpdate` (local or positional),
    `sndListenerTransform` (listeners); port `portCarSoundFrame` (the cap), `portHumanLoopPlay` / `Stop` (moved
    into H3's section with the define; 33-4's list sizing shared as `portHumanLoopList`). `humanCarTick`,
    `carReadMoveCodes`, `carSoundUpdate`, `weaponTick`, `carThrottleSteer`, `weaponFireSpecial`, `carDestroyed`
    were already changed by H2 or the car stack in some configurations.
  - **Determinism.** Nothing H3 gates writes simulation state: the HUD message state (`hudMessageCurrent`, read only
    by the HUD), the denied flag (read only by `worldUpdate`'s buzz), sound requests and the listener values. The
    gated / positional branches reach no `rand()` (check 5: the gates, `sndListenerTransform`, `portCarSoundFrame`
    and every sound call the positional branches make are roots). `soundRepeatThrottle` (`RAND_CALLERS.md`
    800D3B34, VISUAL on the shared stream) is called the same number of times on every machine.
  - **Retail-equal:** with the default map humans 0 and 1 have sides 0 and 1 and every human 0..3 is local, so
    every gate passes and every branch is retail's; the listener adds views 2..3 only with a present local human
    on them (3-4 local humans, which retail never had). Humans 2-3 with `TM2_PORT_CAR_SOUND` keep 33-4's per-car
    loops (retail leaked); without it their turbo / burn loops are silent (retail leaked).
  - **Checks** (`tools/check_humans.py`): check 8 (new) lists every sound-side call, HUD message, denied buzz, H3 gate
    call and full-volume sound of the built units (preprocessed with `TM2_PORT_HUMANS`, and again with the car-sound
    stack: 84 calls at 53 reviewed sites) and requires the human record index to reach them only through the gates
    (negative control: without the define it finds retail's sides and cues by human index); check 7 accepts the
    gated message; check 5's roots now include H3's gates, `sndListenerTransform`, `portCarSoundFrame` and every
    sound call of the positional branches (170 / 186 functions reachable, `rand()` not reached), and its list of
    changed game functions has the H3 ones (30). `tools/check_car_groups.py --expect-functions` passes on the car
    stack + `TM2_PORT_HUMANS=4` (check 9: the side loops and their flags still only from port functions).
  - **H3 proofs** (`verify_reloc.py` vs E0 of `work/build-20261014-233834-387312`, placement check with
    `--port-limits 2`; all pass): default variant `5e54710d…` (unchanged: no H3 code without the define; port end
    0x805433E0); `TM2_PORT_CARS=16` `8ab3396c…` (unchanged); `TM2_PORT_CARS=16 TM2_PORT_HUMANS=4` `2ec4d314…` (port end
    0x80582B9C; `portHumanSide` 0x8058230C, `portHumanFeedback` 0x80582340, `portHumanMessage` 0x80582364,
    `portHumanSound` 0x805823A8, `portHumanDenied` 0x80582400, `portHumanLoopPlay` 0x805824CC, `portHumanLoopStop`
    0x805825BC; the placement check's changed functions are the H1 + H2 + H3 ones plus the car limits'; at 2 limits
    `sndListenerTransform` compiles to retail's: its views 2..3 loop is empty); the car stack (+ `TM2_PORT_DT_PLAYER`)
    + `TM2_PORT_HUMANS=4` `c0c68da6…` (port end 0x8058E3F4); every port define + `TM2_PORT_HUMANS=4` `a6c78e74…` (port
    end 0x8059052C; `portHumanSide` 0x8058E230, `portHumanLoopPlay` 0x8058E3F0, `portHumanLoopStop` 0x8058E574,
    `portCarSoundFrame` 0x805877AC); every define + `TM2_PORT_HUMANS=4 TM2_MAX_HUMANS=16` builds and proves
    (`1ae6f425…`, port end 0x80596E8C). `tools/check_humans.py` checks 1-8 pass on all of them (check 5 on the
    4-limit builds). Addresses move with every later `.port_bss` / `.port_text` change: read `symbols.tsv`.
  - **Full build** `build.py --source-only --sdk-from-lib --relocatable --port-variant` (`work/build-20261015-013829-357938`): retail images unchanged (hybrid `9219f7ba…`, source-only `d755c86e…`, `--sdk-from-lib` = E0 `f557c1e3…`); published default variant `5e54710d…` (port end 0x805433E0); `tools/make_package.py` run.

**H4. Raise to N** (the storage and settings step). **DONE 2026-10-15** (branch `decomp-humans-h4`; "As built" below).
- `TM2_PORT_HUMANS=16`: every `[TM2_MAX_HUMANS]` array grows (`config/US/port_variant_arrays.tsv`); move
  `carResetWeapons` to `.port_bss` (tsv row, `timer_state.c:16` drops its definition in the variant, as
  `car_object_lookup.c` does for `humanCarObjects`); `TM2_MAX_LOADED_CARS` comes from `TM2_PORT_CARS_LOADED` (32).
- Settings: `settingsCopy` (`menu_state_copy.cpp:35`) clamps `list8`/`list38` to 2 and `list20` to 4 (and `list10`
  to 10); `gameGetLives` (`misc_one_liners.c:289`) reads `portHumanLives[p]` for p ≥ 4; `TM2_SETTINGS_HUMAN_CAR`
  table grows; `humans` ≤ `TM2_MAX_HUMANS` checked where the port builds settings (run-time rule, plus
  `humans + opponents ≤ TM2_PORT_CARS`).
- Drop-in: `portCarStartHuman` already bounds by `TM2_MAX_HUMANS`; `portCarBindGroup` humans = 16 (assert).
- Hazards: `humanCars` grows by 24 KB, every later `.port_bss` symbol moves; `uaInit`/`carTablesClear`/`roundStart`
  loops now touch 16 records (setup only); `TM2_MAX_HUMANS + TM2_MAX_AI_CARS` stack arrays in
  `portCarSoundFrame`.
- **As built (2026-10-15, branch `decomp-humans-h4`; everything under `--port-define TM2_PORT_HUMANS=N`):**
  - **The raise.** `players.h` accepts `TM2_PORT_HUMANS` 4..16 (`#error` outside, without `TM2_PORT_CARS`, or above
    `TM2_PORT_CARS`: every human record is an active car; answer 14: 16 records exactly, 16 cars in all in any mix).
    New `TM2_VARIANT_HUMANS 4` (the variant's record count before H4: what retail's `carResetWeapons` bytes and the
    settings' `lives[4]` hold). With N = 4, and without the define, every image is H3's (proofs below).
  - **Storage** (`.port_bss` / `.port_data`, `config/US/port_variant_arrays.tsv`; every row already sized by its
    group, now checked): `humanCars[N]` (0x814 each: 33,088 bytes at 16), `humanCarObjects`, `humanCarObjectSlots`,
    `padConfigs`, `humanCarTypes`, `hudSmoothedSpeeds`, `carViewHotkeyReleased` (words, 64 bytes each at 16),
    `sessionPortHumanCars[N - 2]` (14), and H1/H2's `portHumanLocal` (16), `portHumanPad` (32), `portHumanSteer` (32),
    `portHumanLayout` (16), `portHumanLives` (16), `portHumanBindings` (2,048). Two changes:
    - **`carResetWeapons` moves** past 4 humans: new tsv group kind `NAME>K` (`gen_link.port_entries`: the limit when
      above K, else 0 = not moved), row `carResetWeapons 80180FA0 1 2 humans>4 bss linear sbss`; `timer_state.c` keeps
      retail's `u8[2]` (with its 2 bytes of padding: humans 2..3) unless the variant has more than 4 records, then
      `port_variant_arrays.c` defines `unsigned char carResetWeapons[TM2_MAX_HUMANS]` (`#pragma tm2_absolute` in
      `players.h` for that case). Writers / readers unchanged (`carSetResetWeapons`, `uaInit`, `roundStart`,
      `sessionRun`'s `TM2_RESET_WEAPONS_REST`, `humanCarDamageUpdate`, `carBindHumanSlot`, `humanCarsRestart`,
      `sub_8011E114`, `portCarStartHuman`, `portHumanCompact`): they index by the symbol, which now has 16 bytes.
    - **`carViewHotkeyReleased`** starts 1 for every human (`[0 ... N - 1] = 1`; it had 1 only for the first 4, so a
      human 4.. given a local slot would have needed one release before its first hotkey).
  - **Settings** (1.6): human_car of humans 2.. in `sessionPortHumanCars` (N - 2), lives and layouts of every human
    in `portHumanLives` / `portHumanLayout` (H2: `gameGetLives` reads `portHumanLives` past human 3), the record keeps
    retail's `human_car[2]`, `lives[4]`, `control_layout[2]`. **`settingsCopy`** (`menu_state_copy.cpp`, the attract
    demo's save / restore): with more than 4 records each list loop also stops at its list's own length
    (`TM2_COPY_ROOM`: `list8` 2, `list10` 10, `list20` 4, `list38` 2), so with 16 humans it writes up to record byte
    +0x27; retail's loops would reach +0x35 (past the 0x2C the record owns: `screenCardDrawEnvs`, and past the
    attract demo's 48-byte stack copy). Up to 4 humans: retail's loops (they stay inside the record, +0x29 at most).
    The copy does not save the port tables (`sessionPortHumanCars`, `sessionPortOpponentCars`, `portHumanLives`,
    `portHumanLayout`): the attract demo writes none of the human ones, but `settingsClearOpponentCars` resets
    `sessionPortOpponentCars` (open question below).
  - **Static asserts** (`port_variant_arrays.c`, after the bind-table `#error`): `TM2_PORT_BIND_HUMANS` is exactly 16
    and the AI part of `portCarBindGroup` holds `TM2_MAX_AI_CARS`; the last human id < 50 and N <= the sound slots;
    `typedef char tm2_h4_*[...]` on the sizes of the records, the six word arrays, the map / input / layout / lives /
    bindings arrays, `portCarBindGroup` (>= humans + AI), `carResetWeapons` (moved: N) and `sessionPortHumanCars`.
  - **Reviewed at 16, no change needed:** the setup loops (`uaInit`, `carTablesClear`, `roundStart`: by
    `TM2_MAX_HUMANS`), `sessionRun`'s Minion / dead-human / reset loops, `sessionLoadArenaAndCars` and `sessionBindCars`
    (by the settings' `humans`, cars through `TM2_SETTINGS_HUMAN_CAR`; groups human i = i + 1, opponents after),
    `carBindHumanSlot` (id `TM2_HUMAN_ID`), the `humanCarTotal` loops (arrays only), `portCarStartHuman` (index <
    `TM2_MAX_HUMANS`, `portCarActiveCount() < TM2_PORT_CARS`), `portHumanCompact` (moves every per-human array incl.
    `carResetWeapons`; `check_car_groups.py`'s holder check passes with it moved), the radars (`TM2_MAX_RADAR_BLIPS` =
    `TM2_PORT_CARS - 1` = every other car; `hudRadarPlotCar`'s capacity test), HUDs by slot (H1). Kill credit is by
    world-object slot (`stats.killer` / `last_attacker`, s8, slots < 64); there is no per-human score table in the C.
    `portCarSoundFrame`'s stack arrays are `[TM2_MAX_HUMANS + TM2_MAX_AI_CARS]` (32 entries).
  - **Checks** (`tools/check_humans.py`): check 1 now builds the ids with `TM2_PORT_HUMANS` 8 and 16 and requires the
    `#error` for 17, for 3, without `TM2_PORT_CARS` and for 16 humans with `TM2_PORT_CARS=11`; checks 4, 6 and 8 run
    their census again with `TM2_PORT_HUMANS=16` and require the same calls; **check 9 (new)**: (a) the 15 per-human
    arrays of the census are tsv rows of a humans group, and every declaration / definition in the built units has
    the first dimension its group gives (80 at 16 humans, 79 at 4; control: retail's 36 all 2); (b) settingsCopy's
    extent at 16 humans (+0x27 < 0x2C, every loop bounded) and at 4 / retail (retail's loops, inside the record), and
    every write of the record's `human_car` / `lives` / `control_layout` with a variable index (direct or through the
    setters) is the port-table macro or one of 4 reviewed menu / accessor sites (retail adds `settingsSetHumanCar`'s
    own write: control); (c) the 6 `tm2_h4_*` asserts preprocess at 16; with a build directory: every humans-group
    array has its entries in report.json (`carResetWeapons` in port storage only past 4) and the port end is below
    the RAM end. `tools/check_car_groups.py --expect-functions` passes on the car stack + `TM2_PORT_HUMANS=16`.
  - **H4 proofs** (`verify_reloc.py` vs E0 of `work/build-20261015-013829-357938`, placement check with
    `--port-limits 2`; all pass). Unchanged from H3: default variant `5e54710d…` (port end 0x805433E0),
    `TM2_PORT_CARS=16` `8ab3396c…`, `TM2_PORT_CARS=16 TM2_PORT_HUMANS=4` `2ec4d314…` (0x80582B9C), the car stack +
    `TM2_PORT_HUMANS=4` `c0c68da6…` (0x8058E3F4), every port define + `TM2_PORT_HUMANS=4` `a6c78e74…` (0x8059052C).
    New: `TM2_PORT_CARS=16 TM2_PORT_HUMANS=16` `bcdb3cc5…` (port end **0x8058946C**; its placement image is the
    4-human one's, `7447a112…`: at 2 limits nothing differs); the car stack (+ `TM2_PORT_DT_PLAYER`) +
    `TM2_PORT_HUMANS=16` `bba4a51e…` (port end **0x80594D64**); every port define + `TM2_PORT_HUMANS=16` `84e9d387…`
    (port end **0x80596E9C**, 6.4 MB below the port RAM end 0x80C00000; 26,992 bytes more than at 4 humans).
    `tools/check_humans.py` checks 1-9 pass on all of them.
  - **Addresses** (every port define + `TM2_PORT_HUMANS=16`; read `symbols.tsv`, they move with every later change):
    `carViewHotkeyReleased` 0x80200000 (`.port_data`), `humanCars` 0x802006A0, `humanCarObjects` 0x802087E0,
    `humanCarObjectSlots` 0x80208820, `padConfigs` 0x80208860, `humanCarTypes` 0x802088A0, `hudSmoothedSpeeds`
    0x802088E0, `carResetWeapons` 0x80208920, `sessionPortHumanCars` 0x80208930, `portCarBindGroup` 0x8053CEBC,
    `portHumanLocal` 0x80585730, `portHumanOptions` 0x80585740, `portHumanPad` 0x80585744, `portHumanSteer`
    0x80585764, `portHumanLayout` 0x80585784, `portHumanLives` 0x80585794, `portHumanBindings` 0x805857A4,
    `portHumanRadarBlink` 0x80585FA4; `portHumanCompact` 0x8059272C, `portCarStartHuman` 0x80593330,
    `portHumanOfId` 0x805943AC. `TM2_PORT_CARS=16 TM2_PORT_HUMANS=16`: `humanCars` 0x80200640, `carResetWeapons`
    0x802088C0, `portHumanLocal` 0x805856D0, `portHumanBindings` 0x80585744, `portHumanOfId`
    0x805883F4.
  - **What the port sets for 16 humans:** build with `--port-define TM2_PORT_HUMANS=16` (and `TM2_PORT_CARS=16`);
    keep `settings.humans` <= 16 and `humans + opponents` <= 16 active cars where it builds the settings (run-time
    rule; nothing in the C clamps `humanCarsSetCount`); fill per match `sessionPortHumanCars[h - 2]` (cars of humans
    2..), `portHumanLayout[h]`, `portHumanLives[h]`, `portHumanOptions`; `portHumanLocal[h]` for every human (humans
    4..15 default to remote, 0..3 to slots 0..3: a machine whose local players are humans 5 and 9 sets them 1 and 2
    and the others -1); per frame `portHumanPad[h]` / `portHumanSteer[h]` for every h < `humanCarTotal`. Drop-in /
    leave: `portCarStartHuman` (index = `humanCarTotal`, up to 16) and `portHumanCompact`, as at 4. The kill feed:
    `portHumanOfId` (ids 1..7, 9, 11..18).
  - **Full build** `build.py --source-only --sdk-from-lib --relocatable --port-variant` (`work/build-20261015-041454-120457`): retail images unchanged (hybrid `9219f7ba…`, source-only `d755c86e…`, `--sdk-from-lib` = E0 `f557c1e3…`); published default variant `5e54710d…` (port end 0x805433E0); `tools/make_package.py` run.
  - **Left for H5 / H6:** H5 the rules (`portHumanOptions` bit 1: round won on `humanCars[0]`, the first human death
    ending the round, `--lives[0]`, `carNearestToHuman`, `carPlayerOneObject`; with more than 2 humans the port must
    set bit 1 once it exists); H6 the port's runtime tests of section 4 (16 humans for a match, 4 local + 12 remote,
    humans 7..9's ids, two machines in sync). Open: the attract demo's `settingsClearOpponentCars` resets
    `sessionPortOpponentCars` and `settingsCopy` does not restore it (request 32's table, any human count; harmless
    unless the port runs the attract demo between matches with more than 9 opponents set); the `worldRadarPlotHumans`
    pairs are O(N^2) with a `rand()` each (240 at 16 humans; deterministic, kept for the shared stream).

**H5. Rules** (`portHumanOptions` bits 1-2; bit 2, AI aim, done in H2): `worldUpdate` round won (`:441`), `humanCarDamageUpdate`
(`car_damage_human.c:299`), `weaponFindTargetAhead/Behind` (`weapon_targets.c:24/97`), `carNearestToHuman`
(`ai_steps.c:856`), `bulletsUpdate` (after tracing `carPlayerOneObject`). Retail with the bits clear. **DONE 2026-10-15**
(branch `decomp-humans-h5`; "As built" below).
- **As built (2026-10-15, branch `decomp-humans-h5`; everything under `--port-define TM2_PORT_HUMANS`, `portHumanOptions`
  bit 1 `TM2_HUMAN_OPT_RULES`; bit 1 clear = retail, the port's equality tests at <= 4 humans unchanged):**
  - **What bit 1 turns off** (request 37 answer 8: the port's battle modes own the rules). The gate is
    `portHumanRules()` (bit 1 outside the attract demo, which has no humans and keeps retail's rules):
    - `humanCarDamageUpdate` (8011B89C): a human's death still marks it dead, sets its `carResetWeapons` flag and runs
      `carDestroyed` (the wreck), then returns: no `roundOverFlag`, no `rtScheduleEvent(1/3/4, 210/420)`, so no
      "first death ends the round" and no deathmatch result by `i == 0` / `humanCars[0].alt_sfx`.
    - `worldUpdate` (8011A700): the won test (`aiCarsRemaining <= 0 && gameMode != 4 && !humanCars[0].dead`) is
      skipped: human 0's survival never ends the round.
    - `sessionRun` (8012BE5C): after each round `portRoundResults(s, result)` takes the result (below): no `--lives[0]`,
      no `humanCarGet(0)` test, no 2-player result screens or tallies (`screenChallengeResult`,
      `screenDeathmatchResult`); a tournament's won round (modes 1/2, event 3) keeps retail's level progression (boss
      introductions, next level, endings: by car types and the level, not by human).
    - `carNearestToHuman` (800EC8A0, the target panel only): the nearest of the other live humans and the live
      computer cars (retail: with two humans the nearest human only, so with bots in a battle the panel never showed
      one); one human or co-op as retail (the nearest live computer car); nothing within 0x7FFF: no panel.
    - The split HUDs (`hudDrawSplitVertical` / `Horizontal`) in co-op print each slot's own human's lives (retail: the
      team's `lives[0]` on HUD 0 only), and `gameGetLives` reads `portHumanLives` for every human (as bit 5).
  - **API** (`include/semantic/port_humans.h`; `.port_text`, only with the define; constants and `portRound` in every
    variant):
    - `int portHumanRules(void)`; `int portHumanAlive(int h)` (present: h < `humanCarTotal` and bound to a car; not
      dead; HP > 0); `int portHumanDown(int h)` (present and destroyed: with bit 1 it waits for the port);
      `int portHumanAliveCount(void)`; `int portAiAliveCount(void)` (computer cars alive, vacant drop-in records not
      counted). All pure.
    - **Round over:** `int portRoundEnd(int event, int delay, int winner, int flags)`: retail's round end
      (`roundOverFlag` = 1, `rtScheduleEvent(event, delay)`: `rtMainLoop` returns `event` to `sessionRun` after
      `delay` ticks, the cars drive on meanwhile, as after a retail death); flag `TM2_ROUND_FREEZE` (1) also does the
      won round's stop (`roundWonFlag`: no car but Dark Tooth takes damage, a human's wreck is ignored; the computer
      cars' gun loops stop). Returns 1, or 0 when a round end is pending already (`roundOverFlag`), the event is not
      positive, or in the attract demo. Works with bit 1 clear too. Events (`TM2_ROUND_*`): 1 lost, 2 quit, 3 won,
      4 retail deathmatch's "player 1 lost", **5 `TM2_ROUND_GAME_OVER`**, 6 and up the port's own.
    - **Result:** `struct TM2PortRound portRound` (`.port_bss` every variant, 16 bytes, tsv group `one`): `event`,
      `delay`, `winner` of the last `portRoundEnd` (the C never reads `winner`: a human index, a team, -1, the port's
      choice), `start_node` (internal, 0 between frames). Match state: the rollback snapshot keeps it.
      `int portRoundResults(struct TM2Settings *s, int result)`: `sessionRun`'s handling with bit 1, and **the hook the
      port wraps for its results screen** (it may call the C's part after its screen). Returns -1 = retail's handling
      (bit 1 clear, mode 0, a tournament's won round), 1 = the session ends, 0 = the next round: quit (2) ends the
      session; game over (5) shows retail's GAME OVER and ends it; a tournament (modes 1/2) plays the round again
      (no `lives[0]`: the lives are `portHumanLives`, the port's) with the down humans' weapons reset; a battle
      (modes 3/4) binds the cars again (`sessionBindCars`, as retail every round) and resets every human's weapons,
      without a result screen. Every human index by loop, none fixed.
    - **Respawn:** with bit 1 a dead human stays down (its wreck; nothing restarts it: no event, no round restart),
      `portHumanDown(h)` = 1, until the port calls `int portHumanRespawn(int h, int node, int flags)` (between frames,
      on every machine at the same frame): a drop-in style start of the present human h on **its own car** (the
      record keeps its index, car, slot, object id): fresh stats and full HP (`carStatsLoad`, as every start), fresh
      weapons (`TM2_HUMAN_RESPAWN_KEEP_WEAPONS`: retail's lost-a-life rule, 2 specials, full energy, the rest kept;
      the port sets its loadout after the call), state flags cleared (dead, burning, frozen ...), the car drawn and
      its body shown at damage stage 0, `hudResetSpeed`, at route node `node` (an `aiRouteNodes` index the port picks)
      or, with `node` < 0, the start farthest from every live car (`portStartNodePick`'s rule, no `rand()`). The
      heading draws one `rand()`, as every retail start (simulation: same on every machine). Not `carRestart`: none of
      its match-wide resets (projectiles, drops, specials, effect sprites, HUD messages stay). The view mode stays
      (the port's watch view sets it back with `carSetViewMode`); `stats.killer` / `last_attacker` are cleared by
      `carStatsLoad` (read them before). Returns h, or `TM2_HUMAN_ERR_BUSY` (-9, attract demo),
      `TM2_HUMAN_ERR_RECORD` (-1, no present human h), `TM2_HUMAN_ERR_NODE` (-11, past `aiRouteNodeCount`). Any
      present human may be restarted (alive too).
    - **Drop-in start place** (`TM2_PORT_CAR_DROPIN`): `portCarStartHumanAt(index, group, flags, node)` = 
      `portCarStartHuman` at route node `node` (< 0 = its farthest free start; `TM2_DROPIN_ERR_NODE` -11);
      `portCarStartHuman(index, group, flags)` is now `portCarStartHumanAt(.., -1)`. Both forcings go through
      `portRound.start_node`, read first by `portStartNodePick` (`TM2_PORT_CARS`), with `portCarOptions` bit 2 set
      for the call. A respawn is not a drop-in: `portCarStartHuman` takes a new record (index `humanCarTotal`) on a
      free car, so restarting a down human through it would need `portCarRemove` first (which ends its projectiles
      and moves the last human into its index).
  - **Functions** (7 game functions change, all only under the define: `humanCarDamageUpdate`, `carNearestToHuman`,
    and again `worldUpdate`, `sessionRun`, `gameGetLives`, `hudDrawSplitVertical`, `hudDrawSplitHorizontal`; port:
    9 functions + 3 static helpers in `.port_text` (`portHumanPresent`, `portAiAlive`, `portHumanRestartRecord`),
    `portStartNodePick` (the forced start), `portCarStartHuman` / new `portCarStartHumanAt`, `portAiAimTarget` (below)).
  - **Human 0 not local, alive or present** (answer 9). On these paths nothing reads a fixed human index with bit 1
    (check 10). Traced: `carPlayerOneObject` (`bulletsUpdate` 800D87B4 writes `humanCarObject(0)` every frame) is
    **write-only**: its only references in retail are the two `$gp` stores (`bulletsUpdate`, the setter
    `carSetPlayerOneObject` 800D6EE8, itself uncalled); with no human the store writes 0 (`humanCarObjects[0]`).
    `portAiAimTarget` with bit 2 clear (retail's aim at human 0) returns 0 (no target: the shot goes straight) when
    there is no human 0 object, instead of an address off a null pointer.
  - **Dedicated server (later step, not a rework):** what a match with no human (or human 0 absent) still needs:
    1. `humanCarObject(0)` / `humanCars[0]` users outside the rules: `weaponFindTargetAhead/Behind` with bit 2 clear
       (now no target without human 0; a server sets bit 2), `humanCarFindById`'s fallback to record 0 for an unknown
       id (`car_records.c` 8011D07C: callers would write into record 0; needs a "none"), the attract / watch views on
       `humanCars[0]` (`demoWatchNextAiCar` `world_collapse_warps.c`, `spawn_tables.c` camera presets,
       `view_watch_ai.c`), the AI blips on human 0's radar (`worldUpdate`: `rand() % 5` per AI car keyed on
       `humanCars[0]`'s position: deterministic on every machine even when the record is stale, but a server should
       keep the draws as they are and skip only the plot).
    2. `aiGetTargetCar` / `humanCarNearest` with no humans: `humanCarNearest` returns 0 for an empty loop, so a bot
       with target -1 chases record 0's stale position; a server gives every bot an AI target (`aiCarTargets`, the
       port's team modes) or the C gets a "no human" answer there. `carNearestToHuman` runs only for a local human.
    3. The round rules: bit 1 hands them to the port (nothing reads `humanCars[0]` on the rules path with the bit);
       with the bit clear `worldUpdate`'s won test and `sessionRun`'s `humanCarGet(0)` test (<= 2 humans) still do.
    4. A spectator camera for view slot 0 without a human (`viewMainPlayer` = slot 0; `worldHumanCamera` runs only for
       a local human; view 7 `viewWatchedAiCar` follows a computer car, a human-less one would need its own record).
    5. `portCarRemove` refuses the last human (`TM2_DROPIN_ERR_LAST`); a server with no player needs 0 humans allowed
       (and `humanCarsSetCount(0)` at the match start: the loops by `humanCarTotal` already allow it).
  - **Determinism.** `portRoundEnd` and `portHumanRespawn` write the simulation: the port calls them between frames
    from state every machine has (its rules read the records, `portHumanAlive/Down`, its own tables), on every machine
    at the same frame. The respawn's `rand()` (the heading; a farthest start with no live car: retail's random entry)
    is drawn on every machine alike. `portHumanNearestCar` is presentation (the target panel): no `rand()` (check 5).
    `portRound` is match state (snapshot it); `start_node` is 0 between frames.
  - **Checks** (`tools/check_humans.py` check 10, new): (a) every round end in the built units (`rtScheduleEvent` call,
    store to `roundOverFlag` / `roundWonFlag`; 24 at 19 sites) is on a reviewed list: a fixed-human rule behind the bit-1 gate,
    the port's trigger, a round start's reset, the boss's stage 2, the attract demo, the pause quit; (b) in the 4 rule
    functions every fixed-human read of the rule (`humanCars[0]`, `i == 0`, `humanCarGet(0)`, `lives[0]`, the result
    screens, `humanCarNearest`) comes after the gate, the split HUDs have no `gameGetLives(0)`; (c) the 12 H5 port
    functions read no fixed human index; (d) the respawn path: the 4 in-place restarts (`carRestart`) are the round
    start, the binding, `humanCarTick`'s `restart_pending` (written only 0 / restored) and the dead `sub_8011E114` (no
    caller); `portHumanRespawn` goes through `portHumanRestartRecord`, which makes none of carRestart's match-wide
    resets; the same census at 16 humans; negative control without the define (retail's 7 ungated fixed-human round
    ends). With a build: the H5 port functions are port functions, `portHumanRespawn` does not reach `carRestart`
    (call graph), the H5 game functions changed. Check 4's two co-op lives entries became "human map"; check 5's
    roots include `carNearestToHuman` / `portHumanNearestCar`.
  - **H5 proofs** (`verify_reloc.py` vs E0 of `work/build-20261015-060537-106377`, placement check with
    `--port-limits 2`; all pass): default variant `65330f25…` (was `26154d79…`: `portRound` is new `.port_bss` storage, so `.port_text` moves; no game function changes; port end 0x80543400); `TM2_PORT_CARS=16 TM2_PORT_HUMANS=4` `7c92512f…` (port end 0x805835BC; the placement image `4676a53f…` changes exactly the H1-H5 game functions plus the car limits', `humanCarDamageUpdate` and `carNearestToHuman` among them); the car stack (+ `TM2_PORT_DT_PLAYER`) + `TM2_PORT_HUMANS=16` `4805aeb2…` (port end 0x80595824); every port define (with `TM2_PORT_RAM_BALANCE`) + `TM2_PORT_HUMANS=16` `f5d13fc3…` (port end **0x80597B2C**, 6.4 MB below the port RAM end). `tools/check_humans.py` checks 1-10 pass on all of them; `tools/check_car_groups.py --expect-functions` passes on the car stack + `TM2_PORT_HUMANS=16`.
  - **Addresses** (every port define + `TM2_PORT_HUMANS=16`; read `symbols.tsv`, they move with every later change):
    `portHumanOptions` 0x80585740, `portHumanLives` 0x80585794, `portRound` 0x80585FB8; `portStartNodePick` 0x8058A280, `portCarStartHuman` 0x80593380, `portCarStartHumanAt` 0x805933A0, `portHumanRules` 0x80595168, `portHumanAlive` 0x805951DC, `portHumanDown` 0x80595254, `portHumanAliveCount` 0x805952D4, `portAiAliveCount` 0x805953E8, `portRoundEnd` 0x80595460, `portRoundResults` 0x80595524, `portHumanRespawn` 0x8059578C, `portHumanNearestCar` 0x805958DC. `TM2_PORT_CARS=16 TM2_PORT_HUMANS=4`: `portHumanOptions` 0x8057F4A8, `portRound` 0x8057F6D8, `portRoundEnd` 0x80582A50, `portRoundResults` 0x80582B14, `portHumanRespawn` 0x80582D7C.
  - **Full build** `build.py --source-only --sdk-from-lib --relocatable --port-variant` (`work/build-20261015-075238-489764`): retail images unchanged (hybrid `9219f7ba…`, source-only `d755c86e…`, `--sdk-from-lib` = E0 `f557c1e3…`); published default variant `65330f25…` (port end 0x80543400); `tools/make_package.py` run.
  - **Open:** (1) the port's results screen: `portRoundResults` is the hook (wrap it), or a later step lets the C call
    a port callback there; (2) `restart_pending` (stats +0x9C) has no writer but makes `humanCarTick` call `carRestart`
    in place when set: left as retail (the port must not set it); (3) with bit 1 a co-op tournament's "GAME OVER" comes
    only from the port (`TM2_ROUND_GAME_OVER`): the C no longer counts lives there.

**H6. Proofs and tools** (with each step, collected here). **DONE 2026-10-15** (branch `decomp-humans-h6`; "As built" below).
- `tools/check_car_groups.py` check 13: (a) every caller of `viewSetCameraPose`, `viewSetPosition`,
  `viewSetRotation`, `viewSetFollowObjPos`, `hudToggleRadar`, `hudToggleWeaponList`, `hudRadarPlotCar`,
  `hudDrawTargetPanel`, `padCheckAction`, `padComboHeld`, `padAnyLatched`, `soundStartPlayerLoop`,
  `soundStopPlayerLoop` is on a reviewed list with its index kind (slot or human); (b) every id producer of 1.4 uses
  the macro; (c) the locality-gated calls reach no `rand()` caller (call graph as check 12); (d) `portHumanCompact`
  writes every per-human array (the 21 + the new ones).
- `verify_reloc.py` against E0 per configuration, `--expect-functions` lists; retail images keep their hashes.
- Port tests: 1 local + 15 bots (retail-equal path); 2 local split + 2 remote; 4 local + 12 remote (all HUDs,
  radars show every other car, no write past the view arrays); a remote human's turbo/guns/burn for a minute (no
  sound-pool leak); a remote combo/pickup shows nothing locally; human 7, 8, 9 shoot and get shot (ids 9, 11, 12;
  kill credit right; missiles never damage human 9); compaction of a local human (its view/HUD/pad stay on its
  slot); two machines with different local sets stay in sync (hash without the camera block).
- **As built (2026-10-15, branch `decomp-humans-h6`):**
  - **Where each item is checked** (the plan's "check_car_groups.py check 13" is taken: `check_car_groups.py` check
    13 is the special-as check of request 35, so the items stay in the tools that grew with H0-H5):
    | item | check |
    |---|---|
    | (a) every caller of the view / camera / HUD / radar functions with its index kind | `check_humans.py` check 4 (`viewSetCameraPose`, `viewSetPosition`, `viewSetRotation`, `viewSetFollowObjPos`, `hudToggleRadar`, `hudToggleWeaponList`, `hudRadarPlotCar`, `hudDrawTargetPanel` and the rest of `VIEW_CALLEES`; 4 and 16 humans; negative control) |
    | (a) the pad readers (`padCheckAction`, `padComboHeld`, `padAnyLatched`, ...) | `check_humans.py` check 6 |
    | (a) the side loops (`soundStartPlayerLoop`, `soundStopPlayerLoop`) and the local feedback | `check_humans.py` check 8 (and `check_car_groups.py` check 9: the side loops' flags only from port functions) |
    | (b) every id producer uses the macro | `check_humans.py` checks 1 (the macros at 2..16 records, the `#error`s), 2 (no `player + 1` / `owner - 1` outside the macros; the 11 sites) and 3 (`portHumanOfId` in every port variant) |
    | (c) the locality-gated calls reach no `rand()` | `check_humans.py` check 5 (jal call graph from the local-only roots: the map, H2's input functions, H3's gates, the listener, the engine cap, the target panel) |
    | (d) `portHumanCompact` writes every per-human array | **`check_humans.py` check 11 (new)**; the id / address holders it rewrites (projectiles, arcs, drops, shields, ricochets, bullets, sprites, specials) stay `check_car_groups.py` check 12's |
    | the rules (H5) | `check_humans.py` check 10 |
    | the storage at 16 (H4) | `check_humans.py` check 9 |
  - **(d): one array was not moved.** `portHumanCompact` moved the records, the six word arrays, `carResetWeapons` and
    (H1 / H2) the map, words, tilts, layouts, lives and bindings, but not the human's **car selection**:
    `sessionPortHumanCars` (tsv group `humans-2`) and, below it, the settings record's `human_car[0..1]`
    (`TM2_SETTINGS_HUMAN_CAR`). After a compaction the moved human's index pointed at the leaver's car, which
    `sessionLoadArenaAndCars` reads on a later load of the match (a tournament's next level) and the Minion
    introduction test of `sessionRun` reads every round. **Fixed** (with `TM2_PORT_HUMANS`; the drop-in define gives
    the function): the car selection moves with the record (`human_car[]` for indices 0..1, `sessionPortHumanCars[i -
    2]` past them; with the placement check's 2 records the record alone). Kept on purpose (check 11 prints why):
    the settings' `lives[0..3]` (retail's runner counter, `lives[0]` the team's; with bit 1 or 5 the lives are
    `portHumanLives`, which moves) and `control_layout[0..1]` (pads 0/1's layouts by slot; each human's own is
    `portHumanLayout`, which moves). `CAR_GROUPS.md` 14.5's "the port remaps `sessionPortHumanCars` / `human_car[]`"
    is not needed with the define.
  - **Check 11** (`tools/check_humans.py`): preprocessed with the drop-in stack and `TM2_PORT_HUMANS` 4 and 16,
    `portHumanCompact`'s body writes index `to` of every row of a humans group of the tsv (read from the tsv: a new
    per-human row is checked at once; 15 rows) and of `human_car` (16/16); negative control: without `TM2_PORT_HUMANS`
    the port's own human arrays are not moved. With a build (drop-in + `TM2_PORT_HUMANS`): `portHumanCompact` is a
    port function with a relocation into each of the 15 arrays and into `sessionSettings`. Before the fix it failed:
    source 14/16 (`sessionPortHumanCars`, `human_car`), and on H5's `h5_allh16` / `h5_carsh16` links 14/16 + no
    `sessionSettings`.
  - **`tools/check_all.py`** (new; `SOURCE_BUILD.md` "Static checks of the port options"): given a full build (default:
    the latest `work/build-*`), link directories or `work/pv/<tag>` (its `_P4` and `_P2`), runs every static check
    that applies to each link's port defines in parallel and prints one summary: the retail images' sha256 (a full
    build), `verify_reloc.py` against E0 (every link), the placement check (limits-2 links), `check_humans.py`
    (every limits-4 port variant; source checks once), `check_car_groups.py --expect-functions` (a car define),
    `check_hazards.py` / `check_balance.py --expect-functions` (their define), `--models` the disc model checks.
    `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE`, `TM2_PORT_SPU48` have no tool of their own (proof and placement).
  - **H6 proofs** (`verify_reloc.py` vs E0 of `work/build-20261015-185449-548387`, placement check with
    `--port-limits 2`; `tools/check_all.py` on all of them: 14 checks OK): default variant `0c28aa83…` (unchanged: no
    drop-in, no H6 code; port end 0x80543470); the car stack (+ `TM2_PORT_DT_PLAYER`) + `TM2_PORT_HUMANS=16`
    `ececb680…` (port end 0x80595914; placement `d5e42c0b…`); every port define (with `TM2_PORT_BALANCE`) +
    `TM2_PORT_HUMANS=16` `41447845…` (port end **0x8059E3FC**, 6.4 MB below the port RAM end; placement `9b383602…`).
    Only `portHumanCompact` changes (0x660 bytes; at 0x80597EFC with every define, 0x805909AC with the car stack).
  - **Port tests:** rewritten as the checklist of section 7 (T1-T13: setup values, what to observe and hash).
  - **Full build** `build.py --source-only --sdk-from-lib --relocatable --port-variant` (`work/build-20261015-223109-608696`): retail images unchanged (hybrid `9219f7ba…`, source-only `d755c86e…`, `--sdk-from-lib` = E0 `f557c1e3…`); published default variant `0c28aa83…` (port end 0x80543470); `tools/check_all.py --models` on it: 8 checks OK; `tools/make_package.py` run.

---------------------------------------------------------------------------------------------------------

## 5. Questions for the port

1. **Netcode model:** lockstep (every machine simulates all humans from exchanged inputs) or state sync from the
   server? The determinism rules of 2.5 assume lockstep.
2. **Remote input format:** the 16-bit decoded button word + the human's layout (0..3), evaluated in the C (2.4)? Or
   pre-resolved actions? And per-human analog steering/throttle values (your overlay's analog hook): same table?
3. **Local players online:** should local humans also read `portHumanPad` (bit 0, input delay equal for all), or
   their pads directly?
4. **Remote humans' camera/view bytes** (+0x006..+0x08F, +0x10C, +0x110) differ per machine with cameras for locals
   only: excluded from your state hash, or should the C run every human's camera (changes the shared camera globals)?
5. **Pads of local slots 2-3:** through `portHumanPad` (replacing your overlay's pad 3-4 read)? Slots 0/1 stay on
   pads 0/1 unless bit 0.
6. **Object ids** 1..7, 9, 11..19 (skip 8 = world, 10 = missile objects): OK? Your kill feed / `victim_of` should map
   id → player with `portHumanOfId` (or the macro) instead of `id - 1`.
7. **Remote feedback:** drop a remote human's HUD messages, jingles and denied sounds locally (default), or play them
   positionally?
8. **Rules with > 2 humans:** the port decides round end and results (bit 1), and respawns dead humans itself (how:
   `carRestart` in place, or a drop-in style start)? Lives in `portHumanLives` or your own tables?
9. **Dedicated server:** still out of scope (human 0 = host, always present)? `humanCarObject(0)`, `aiGetTargetCar`
   with no humans, and the round rules would need a spectator design later.
10. **AI aim** (`weaponFindTargetAhead/Behind` steer every bot's aimed shot at human 0): fix under bit 2 (the bot's own
    target)? It changes which car is aimed at, not `rand()`.
11. **Lives and layouts of humans past the record's fields:** in the C (`portHumanLives`, `portHumanLayout`) or yours?
12. **Engine-voice cap:** nearest to local humans (the listeners) instead of any human?
13. **Cheat combos online** (armour, HP, turbo, infinite weapons from `humanCarTick`): allowed (deterministic from the
    words) or off?
14. **Maximum:** 16 human records exactly, or headroom (ids allow 47)?
15. **Ordering:** H1-H3 at N = 4 first (2 local + 2 remote), then H4 raise; or H4 first?

---------------------------------------------------------------------------------------------------------

## 6. The port's answers (request 37, 2026-10-14)

From the port's `DECOMP_REQUESTS.md` request 37 (the user confirmed 9, 13 and 14). What each changes in the plan:

1. **Netcode: lockstep**, input delay first, then rollback; state sync only as a fallback or resync. 2.5 holds as
   written, plus one rule from rollback: the simulation saves and restores cleanly, and nothing local-only feeds back
   into it.
2. **Remote input:** the 16-bit decoded word + the human's layout (0..3), evaluated in the C (2.4), so combos and
   cheats see the same bits everywhere. **New:** `short portHumanSteer[TM2_MAX_HUMANS]`, the stick tilt 0..4096
   (4096 = full or digital), read by the port's analog hook around `carThrottleSteer` for every human (H2 storage).
3. **Local players online** read `portHumanPad` too (bit 0, the same delay as remote ones); offline, pads directly.
4. **Remote cameras:** not run; the port's hash and rollback snapshot skip +0x006, +0x00C..+0x08F, +0x10C, +0x110.
   The reader check of 2.5.2 must be finished (any simulation reader of those bytes is a desync): H1.
5. **Local slots 2-3** through `portHumanPad`, replacing the overlay's pad 3-4 read; slots 0/1 on pads 0/1 unless bit 0.
6. **Object ids** 1..7, 9, 11.. : OK (16 humans end at 18, see 1.4). The kill feed (by object slots today) uses
   `portHumanOfId` wherever it turns an id into a player. **Done in H0.**
7. **Remote feedback** dropped locally (the default of 2.3): no HUD messages, jingles or denied sounds of a remote
   human; positional sounds (gun, engine, turbo of a remote car) stay, as for computer cars.
8. **Rules with more than 2 humans:** the port decides round end and results (bit 1; its battle modes already do for
   1-4 players). Respawn: a drop-in style start (`portCarStartHuman`, start place picked by the port, full health,
   its loadout), not `carRestart` in place. Lives in `portHumanLives`.
9. **Dedicated server:** not now, but someday. Human 0 = host for now. Nothing new may assume human 0 is local,
   alive or present (H0's `portHumanOfId` checks presence, not index 0). What a server with no human needs, as a later
   step (not a rework): `humanCarObject(0)` users (`weaponFindTargetAhead/Behind`, `carPlayerOneObject` in
   `bulletsUpdate`, `humanCarFindById`'s fallback to record 0, the attract/watch views on `humanCars[0]`) need a
   "no human" path; `aiGetTargetCar` / `carNearestToHuman` with no humans; the round rules of 2.6 without
   `humanCars[0]` (bit 1 already hands them to the port); a spectator camera for view slot 0 (`viewMainPlayer` = slot,
   no human).
10. **AI aim:** fixed under bit 2: a bot's aimed shot goes at `aiGetTargetCar` (the port's team modes set
    `aiCarTargets`). H5.
11. **Lives and layouts** of every human in the C (`portHumanLives`, `portHumanLayout`), filled by the port from the
    match setup; `gameGetLives` reads them (H2 / H4).
12. **Engine-voice cap:** nearest to the local humans (the listeners). H3.
13. **Cheats online: never.** Off whenever any human is remote; locally they keep working. **New:** a bit in
    `portHumanOptions` turns the cheat combos of `humanCarTick` off (one gate; H2).
14. **Maximum: exactly 16 human records**, 16 cars in all in any mix; a joining human takes a bot's slot (the bot
    removed, the human dropping in with their own car: request 33's drop-in, not a takeover), and a leaving human may
    be replaced by a bot: records come and go in both directions mid-match (`TM2_PORT_HUMANS` max 16).
15. **Ordering:** H1-H3 at N = 4 first (2 local + 2 remote, testable on one machine with two instances), then H4;
    H0 any time (done).

---------------------------------------------------------------------------------------------------------

## 7. The port's runtime tests (H6 checklist)

Section 4 H6's "Port tests" as a checklist for the port (the decomp never runs the game: these are the port's to
run). Build: `--port-define` the car stack (`TM2_PORT_CAR_GROUPS TM2_PORT_CARS=16 TM2_PORT_CAR_SWAP
TM2_PORT_TEXTURE_BANKS TM2_PORT_CAR_SPECIALS TM2_PORT_DARK_TOOTH TM2_PORT_CAR_SOUND TM2_PORT_CAR_CLUTS
TM2_PORT_CAR_DROPIN TM2_PORT_SPECIAL_AS TM2_PORT_DT_PLAYER`) plus `TM2_PORT_HUMANS=16` (the "B" build); the "A" build
is the same without `TM2_PORT_HUMANS`. Addresses: the build's `symbols.tsv` (they move with every change).
`tools/check_all.py <link>` passes on both before a run.

**Common setup** (between frames; `port_humans.h`, `car_dropin.h`, `settings.h`):
- per match: `sessionSettings.humans` / `opponents` (`humans + opponents <= 16`), the cars (`human_car[0..1]`,
  `sessionPortHumanCars[h - 2]`), `portHumanLayout[h]` (0..3), `portHumanLives[h]`, `portHumanOptions`, then
  `portHumanLocal[h]` for every human (0 = default: humans 0..3 on slots 0..3, 4.. remote; 1..4 = slot 0..3;
  -1 = remote);
- per simulation frame, before `rtMainLoop`'s `padUpdate` / `worldUpdate`: `portHumanPad[h]` (the 16-bit word,
  `padButtonsDecoded`'s half layout, active high) and `portHumanSteer[h]` (0..4096) for every h < `humanCarTotal`;
- online options: `portHumanOptions = 0x0B` (bit 0 INPUT_ALL, bit 1 RULES, bit 3 NO_CHEATS) + bit 2 (AI_AIM) for
  team modes; offline with up to 2 humans: 0 (retail); offline 3-4 humans: bit 1 (the rules past 2 humans);
- **the hash** of a frame (two machines, or A against B): the simulation state, i.e. `humanCars[0..humanCarTotal)`
  **without** each record's +0x006..+0x007, +0x00C..+0x08F (camera block; +0x08C..+0x08F `speed_change` is a
  per-tick temporary, skipping it is harmless), +0x10C..+0x113 (`view`, `view_last`); `aiCars`, the car slot and
  bind tables, the projectile / arc / drop / shield / ricochet / bullet pools, the special states, `portRound`,
  `portHumanBindings` / `Layout` / `Lives` / `Options`, the `rand()` seed. Never hashed (per machine:
  presentation or inputs): `portHumanLocal`, `portHumanRadarBlink`, `carViewHotkeyReleased`, `hudSmoothedSpeeds`,
  views, HUDs, radars, sound state and listeners, `portHumanPad` / `Steer` (inputs: rewritten each frame).

| # | scenario | setup (beyond the common one) | observe / hash | pass when |
|---|---|---|---|---|
| T1 | **1 local + 15 bots, retail-equal path** | A and B; `humans` 1, `opponents` 15; `portHumanOptions` 0; `portHumanLocal` all 0; the same recorded pad stream on pad 0 | per-frame hash of A and B over a whole round (bots fight, the human dies once, `carRestart`), `rand()` call count per frame | hashes and call counts equal frame for frame (the define changes nothing with bits 0 and the default map); the port's retail 11-car comparison (`humans` 1, `opponents` 10, no `TM2_PORT_CARS`) stays exact as before |
| T2 | **2 local split, retail rules** | A and B; `humans` 2 (modes 2 co-op and 4 deathmatch), options 0, pads 0/1 | hash A = B; the first death ends the round (retail), the result screens of 2 players, `lives[0]` counting down | as T1; humans 0/1 read pads 0/1 (`padCheckAction(a, 0/1)`), turbo / gun / burn on sound sides 0/1 |
| T3 | **3-4 local offline** | B; `humans` 4; options bit 1 (rules); `portHumanLocal` 0 (default); words of slots 2-3 from pads 3-4 (`portHumanPad[2..3]`), slots 0/1 on pads 0/1 | 4 views / HUDs (`portLocalHumanCount()` 4), each HUD the human of its slot, radars with 3 humans + bots; slots 2-3's turbo / burn loops play (with `TM2_PORT_CAR_SOUND`: the slot's instance) | no write past the 4-entry view / HUD arrays (canary words after `hudRadarVisible` / `hudWeaponListVisible` and the view tables unchanged); a death leaves the human down (`portHumanDown`) until `portHumanRespawn` |
| T4 | **2 local split + 2 remote** (one PC, two instances, lockstep) | B; `humans` 4; options 0x0B; X: `portHumanLocal = {1, 2, -1, -1}`, Y: `{-1, -1, 1, 2}`; every word exchanged and written on both for frame F | hash X = Y every frame; X shows humans 0/1, Y humans 2/3 on their slots 0/1; remote cars' guns / turbo / burn positional | equal hashes for 10 minutes of battle (respawns through `portHumanRespawn`, rounds through `portRoundEnd`) |
| T5 | **4 local + 12 remote** | B; `humans` 16, `opponents` 0; options 0x0B + bit 4 (AI blips everywhere, no effect without bots); X: humans 0..3 `{1, 2, 3, 4}`, 4..15 -1; Y (non-identity map): humans 5, 9, 12, 15 = 1..4, the rest -1 | 4 HUDs and 4 radars on each; each radar plots the other 15 cars (`TM2_MAX_RADAR_BLIPS` 15); `sndListenerTransform` takes views 2..3; Y's HUD of slot 0 shows human 5 (`portLocalHuman(0)` = 5) | hash X = Y; canaries after every 4-entry view / HUD / radar array unchanged; no HUD, camera or hotkey for a remote human (`carViewHotkeys` returns) |
| T6 | **a remote human's loops for a minute** | T4's X; human 2 (remote) holds turbo, fires the machine gun, drives through lava / fire for 60 s; then dies and the round ends | the sound request pool (`sndObjPool` in use) and the instance pool, `portCarSoundLoops[slot]` bits | pool use bounded (no growth per frame), its gun in the shared loop (instance 0x100 + slot, bit 0x10), turbo / burn as its car's instance; after its death / the round end nothing of it plays (bits cleared) |
| T7 | **remote feedback** | T4; remote human 2 performs a special move code, a cheat code (bit 3 set, and once clear offline), picks up ammo when full, fires with an empty weapon | locally (X): `hudMessageCurrent` unchanged, no full-volume jingle or denied buzz; its special-fire sound positional at its car; on Y (its own machine) the messages and cues play | nothing local for a remote human; with bit 3 the cheat code is consumed with no effect on both machines (hash equal) |
| T8 | **humans 7, 8, 9: ids** | B; `humans` 10, `opponents` 6; options 0x0B; humans 7, 8, 9 (object ids 9, 11, 12) fire every weapon (homing / fire / power missiles, napalm, mines, freeze, specials, rear fire) at each other and at bots, and get shot | `stats.killer` / `last_attacker` of the victims through `portHumanOfId` (9 -> 7, 11 -> 8, 12 -> 9, 8 and 10 -> -1); projectile owners = `TM2_HUMAN_ID(h)` | kill credit right for every weapon; no human hits itself with its own missile; human 9 takes damage from others' missiles (its id is 12, not the missile objects' 10) |
| T9 | **compaction of a local human** | B; `humans` 4, options 0x0B; X: `portHumanLocal = {-1, 1, -1, 2}` (humans 1 and 3 local on slots 0 and 1); human 3 fires missiles, then `portCarRemove(0, 1)` (human 1 leaves) | after the call: `humanCarTotal` 3; human 3 is now index 1: `portHumanLocal[1]` = 2, `portHumanLocal[3]` = -1, `portHumanPad` / `Steer` / `Layout` / `Lives` / `Bindings`, `carResetWeapons`, `humanCarTypes`, `padConfigs`, `hudSmoothedSpeeds`, `carViewHotkeyReleased` and its car selection (`sessionPortHumanCars[3 - 2]` -> `human_car[1]`) at index 1; car object id 2; its missiles' owner 2; `portHumanOfId(2)` = 1, `portHumanOfId(4)` = -1 | its view, HUD and pad stay on slot 1 (no remap by the port), its missiles still credit it; the port remaps only its own net-player table; Y does the same call at the same frame: hash equal |
| T10 | **drop-in and leave at 16** | B; `humans` 12, `opponents` 4; options 0x0B; a bot leaves (`portCarRemove(1, i)`), a human drops in: write `portHumanLocal[12]`, `portHumanLayout[12]`, `portHumanLives[12]`, `sessionPortHumanCars[10]` first, then `portCarStartHumanAt(12, group, flags, node)` (node -1: the farthest free start); repeat to 16 humans; then one more | the new human's bindings built from its layout, its view mode set (local: its slot's camera), id `TM2_HUMAN_ID(12)` = 15; the 17th start refused (`TM2_DROPIN_ERR_FULL` / `_INDEX`), removing the last human refused (`TM2_DROPIN_ERR_LAST`) | hash X = Y through every join / leave |
| T11 | **rules past 2 humans** | B; `humans` 6 battle (mode 4) and co-op tournament (mode 2), options 0x0B; human 0 dies first, then human 0 leaves (`portCarRemove(0, 0)`) | with bit 1: a death -> `portHumanDown(h)` 1, no round end; the port ends the round with `portRoundEnd(event, delay, winner, flags)` and sees `portRoundResults` (wrapped) take it; `portHumanRespawn(h, -1, 0)` restarts h on its own car (index, slot, id kept; other cars' projectiles kept); co-op GAME OVER only through `portRoundEnd(TM2_ROUND_GAME_OVER, ..)`; the target panel shows bots too | nothing ends or is decided on human 0 (dead or absent); hash X = Y |
| T12 | **two machines, different local sets** | B; `humans` 16 (or 10 + 6 bots); options 0x0B; X local humans 0, 1; Y local 2, 3, 4; Z local 11 (any third instance); joins, leaves, respawns, round ends as in T9-T11 | per-frame hash (common setup) on every machine; rollback: snapshot the hashed state + `portHumanBindings` / `Layout` / `Lives` / `Options`, `portRound`; re-simulate 8 frames with the words rewritten | equal hashes for 30 minutes; the re-simulated frames hash as the first run |
| T13 | **attract demo between matches** | B; after a 16-human match go back to the title and let the demo run, then start a 4-human match | `settingsCopy` writes stay inside the 0x2C-byte record (canary after it); `portRoundEnd` / `portHumanRespawn` / drop-in refused in the demo (0 / `TM2_HUMAN_ERR_BUSY` / `TM2_DROPIN_ERR_BUSY`) | no corruption; the next match starts with the port's tables as the port set them (re-write `sessionPortOpponentCars`: the demo clears it, H4 "Open") |

**H5 rules to keep in the tests:** set bit 1 whenever more than 2 humans play or any human is remote; the port's
rules read `portHumanAlive` / `portHumanDown` / `portHumanAliveCount` / `portAiAliveCount`, end rounds with
`portRoundEnd` and respawn with `portHumanRespawn` (between frames, on every machine at the same frame:
both write the simulation); `portRound` is match state; never set `restart_pending` (stats +0x9C).

## 8. Request 50: a leaving player's car to a computer driver (hand-over, vacant records)

Request 50 (2026-10-20): online with `TM2_PORT_HUMANS=16`, when a player leaves mid-match from a spot the host fills
with computer cars ("CPU / OPEN"), a computer driver takes the car over; human records keep their indices on every
machine. Built: **(a)**, the hand-over, plus the leave without a successor and the rejoin. Everything is behind the
two defines the port already builds with, `TM2_PORT_HUMANS` and `TM2_PORT_CAR_SWAP` (`port_humans.h`
`TM2_HUMAN_HANDOVER`); it does **not** need `TM2_PORT_CAR_DROPIN` (8.5). Static proofs only (8.7).

### 8.1 API (`port_humans.h`; `.port_text`; between frames, on every machine at the same step)

| function | does | returns |
|---|---|---|
| `int portHumanHandOver(int h, int j)` | **(a)** human h's car, as it is, to AI record j (-1 = a free one: the lowest vacant AI record with `TM2_PORT_CAR_DROPIN`, else a new record `aiCarTotal`); a computer car from the next frame; human h left vacant | j, or an error |
| `int portHumanLeave(int h)` | h leaves with no successor: its car parked (a free car of its group, as `portCarRemove` parks it), its projectiles / mines / arcs / shields / ricochets / bullets removed, every reference cleared; h vacant. **(b)** with `TM2_PORT_CAR_DROPIN`: then `portCarStartAi(-1, group, 0)` starts a new computer car of that car's group at the farthest free start (fresh HP and weapons) | h, or an error |
| `int portHumanReturn(int h, int j)` | the rejoin: vacant human h takes AI record j's car back as it is; AI record j leaves the match (8.4) | h, or an error |
| `int portHumanRejoin(int h, int group, int node)` | request 50b: a joining player takes vacant record h back **in place** (no other record moves, `humanCarTotal` unchanged) on the free car of load group `group`, as a fresh start at route node `node` (< 0: the farthest free start) (8.4) | h, or an error |
| `int portHumanFreeGroupOfType(int type)` | request 50b: a free (parked) car of car type `type`: its load group, lowest first | group, 0 = none |
| `int portHumanVacantCount(void)` | the vacant human records | count |

Errors (negative): `TM2_HUMAN_ERR_RECORD` -1 (no human h bound to a car / h not vacant for the return),
`TM2_HUMAN_ERR_FULL` -3 (the vacant slot, `TM2_MAX_CAR_SLOTS - 1`, holds a car: never with 16 cars),
`TM2_HUMAN_ERR_INDEX` -4 (j not a free AI record / not one that may leave), `TM2_HUMAN_ERR_LAST` -5 (the last
present human), `TM2_HUMAN_ERR_BUSY` -9 (attract demo), `TM2_HUMAN_ERR_DOWN` -13 (the car is destroyed: hand over
after `portHumanRespawn`, or use `portHumanLeave`). State: `portHumanVacant` (`.port_bss`, every variant, the last
port word; `struct TM2PortHumanVacant`, 0x14C bytes since request 50b): `mask` (bit h = record h vacant), `handed` /
`left` / `returned` / `rejoined` counts of this load, `ai[h]` (the AI record + 1 that took h's car, 0 = none), the dummy object. Cleared
at every load (`carTablesClear` -> `portCarGroupsReset`). Match state: in the rollback snapshot and hash
(`port.bss`; `config/US/rollback_regions.tsv` row `portHumanVacant`).

Addresses (the port's define list, `work/pv/r50_port_P4/functions.tsv`; they move with every change, read
`symbols.tsv`):

| function | the port's defines (`r50_port_P4`) | humans + cars + swap only (`r50_hs_P4`) |
|---|---|---|
| `portHumanHandOver` | 0x808B947C | 0x8089A804 |
| `portHumanLeave` | 0x808B9800 | 0x8089AB88 |
| `portHumanReturn` | 0x808B9948 | 0x8089ACD0 |
| `portHumanVacantCount` | 0x808B8A10 | 0x80899D98 |
| `portHumanFirstPresent` | 0x808B8A80 | 0x80899E08 |
| `portHumanBindAiCount` | 0x808B8AE8 | 0x80899E70 |
| `portHumanVacantReset` | 0x808B89CC | 0x80899D54 |
| `portHumanVacant` (storage) | 0x808AC81C | 0x808956C8 (default variant: 0x808508E0) |
| game: `worldUpdate` / `humanCarsRestart` / `humanCarNearest` / `worldCollapseUpdate` / `sessionBindCars` | 0x8011B578 / 0x8011AF08 / 0x8011DAF8 / 0x80117020 / 0x8012D418 | 0x8011B694 / 0x8011B024 / 0x8011DB38 / 0x80117168 / 0x8012D1A8 |

### 8.2 The hand-over (`portHumanHandOver`)

Human h on slot s (its car: object, model, CLUT / team colours, group, key, every reference by slot all stay):
1. what the car runs or holds ends as in a swap (`portCarResetSpecials(s)`, `portCarResetLook(s)`: a special in
   progress, Mr. Slam's claw on it, a crush, freeze ice, turbo flame, fire, invisibility, engine; the next frame's
   AI update starts its engine again); its loops stop (`portDropInSlotSound`: the human's turbo / gun / burn and a
   remote human's shared gun loop);
2. the AI record j cleared (as `carTablesClear` leaves a new one; a claw record it held is kept for a car that brings
   none), `aiCarTotal` + 1 for a new record; the car's blocks copied from the human record: state, controls, aim,
   **stats (HP, killer, last attacker, timers), contacts, wheels, motion (place, heading, speed, route node), weapons
   (ammo, specials, selected weapon, energy, turbo, claw record)**; the wreck effect pointer (human +0xCC -> AI
   `state.x2C.fx`);
3. the AI's own fields as `aiCarStart` sets them, without its start: index, type, started (`f000` 1), `f001` = j % 3
   (`aiCarStart` draws `rand() % 3`: the hand-over draws nothing), `f012` = (j + 1) x 60, the range thresholds
   320 / 160 / 600 / 1000 / 2000 and flags (`aiRangeFlagsClear`), route state (`aiRouteStateReset`: it heads for the
   route from where it is), controls cleared, gun and fire cooldown 50, chase patience 0; every other AI field
   (stuck / wall-bump / back-up counters, target bearing and distance, spun) 0;
4. the binding: `portCarUnbind(0, h)`, `portCarBindSlot(1, j, s)` (object id j + 50), `portCarBindGroup` of AI j =
   the car's group (a rebind keeps it there, 8.3), of h = 0;
5. the AI difficulty: `carStatsSetSpeedMult(aiDifficultySpeedScale())`, `portCarApplyType(1, j, hp)`: the AI stats
   of the type (max HP without the human's +20, top speed, wheels, collision box), **HP kept** (clamped to the AI
   maximum: a human at 120 of 120 drives on at 100 of 100), weapons kept (`weaponBlockInit` for an AI with every
   count put back), weights and fire chance (`aiCarSetupProfile`); `stats.f00` 0; target = the nearest human
   (`aiCarTargets[j]` -1);
6. what named the car by its driver: object id `TM2_HUMAN_ID(h)` -> j + 50 for its projectiles, arcs (shooter and
   target), mines / drops, ricochets and bullets in flight (they hit others, not their own car, and credit AI j),
   arc end points and following effect sprites moved to the AI record, the energy shields it carried end, leaps and
   Dark Tooth swoops aimed at h end; by slot (kill credits, taser shooter, dropper, homing targets) nothing changes;
7. human h vacant (8.3).

The team: the port's team table is by human index; the car keeps its slot, so its paint (`portCarCluts`) and the
port's slot -> team map stay; the port maps AI j to h's team (`portHumanVacant.ai[h]` = j + 1) for its own team
rules (`portHumanOptions` bit 2's `aiGetTargetCar` targets by `aiCarTargets`: give AI j a target of another team).

### 8.3 Vacant human records: what reads them (`tools/check_humans.py` check 12)

A vacant record: dead, HP 0, wreck state 4, `damageable` 0, probe off, route -1, type 0 (`humanCarTypes[h]` 0),
contacts, wheels and controls cleared, claw, fire and wreck effects dropped, still at (-80000, -80000, -80000);
its object the dummy in `portHumanVacant` (not in the scene list: never drawn, never in a hit pass; also far
away, `visible` 0); its slot `TM2_MAX_CAR_SLOTS - 1` (past `carWorldObjectCount`; its object is the dummy, so
`carObjectId` and the like find one); remote (`portHumanLocal[h]` -1), input word 0. `humanCarTotal` is unchanged.

Five game functions test the vacancy (`TM2_HUMAN_SKIP_VACANT` / `TM2_HUMAN_VACANT`, nothing without the defines):
`worldUpdate` (the human loop: no update, car sound, damage rule, camera, follow, radar plot), `humanCarsRestart`
(no restart at a round start), `humanCarNearest` (never the nearest human: AI targets, `carNearestToHuman`, the boss
swoop; its default is the first present human, `portHumanFirstPresent`), `worldCollapseUpdate` (Hong Kong's
sections: no look or stand test), `sessionBindCars` (not bound again; the AI records hand-overs added are, each on
its group: `portHumanBindAiCount`, and with the drop-in `portCarBindCount`). The other 25 game functions over the
human records pass a vacant one by its state (check 12's reviewed list says how: far away for every distance or
angle test, the bound on the matrix product in `weaponFindTargetAhead` / `Behind`, invisible and dead for the
ricochet and hit tests, wreck state 4, type 0, route -1, cleared wheels, remote for the radars). The port side:
`portHumanAlive` / `Down` / `AliveCount` (not present), `portHumanRespawn` (refused), `portHumanNearestCar`, the
start picker (HP 0), `portCarActiveCount` (with the drop-in: not counted), `portHumanCompact` (with the drop-in: the
vacancy moves with the record). Rules: with `portHumanOptions` bit 1 (online) nothing decides on a vacant record;
without it, retail's won test reads human 0 (a vacant human 0 is dead: no won round), so hand over with bit 1.
HUD, camera, views, sound listener: local humans only (a vacant record is remote). Kill credit, lives, names and
the player list are the port's tables by human index: the port keeps h's row (or shows it as "left").

### 8.4 The rejoin (`portHumanReturn`; a vacant record taken back)

A player joining a match in progress takes a vacant record back with `portHumanReturn(h, j)`: AI j's car (the one
`portHumanVacant.ai[h]` names, or any AI record) back to human h as it is: the blocks copied back, the human's own
fields as a drop-in start sets them without its place (the uaInit flags, pad configuration 1, the bindings of
`portHumanLayout[h]`, controls, cheat state, camera offsets, view mode 2), the human stats of the type at speed 100
(max HP + 20, HP kept), object id `TM2_HUMAN_ID(h)` for its things in the air; the vacancy cleared. AI record j
leaves the match: the last record (j = `aiCarTotal - 1`: cleared, `aiCarTotal` - 1) without the drop-in; any record
with `TM2_PORT_CAR_DROPIN` (vacant, as `portCarRemove(1, j)` leaves one). A fresh start after it:
`portHumanRespawn(h, node, 0)`. Write `portHumanLocal[h]`, `portHumanLayout[h]`, `portHumanLives[h]` first.
With several hand-overs, take the records back in the reverse order (the last AI record first) or build with the
drop-in.

**A joiner on a free car (request 50b, `portHumanRejoin(h, group, node)`).** A player who joins a match in progress
can also take vacant record h back on a **free** car: a loaded car nobody drives (slot kind 2, index -1), e.g. the car
`portHumanLeave` parked, a car `portCarRemove` parked, or (with the drop-in) a car `portCarLoadGroupLive` loaded.
`portHumanFreeGroupOfType(type)` finds one of a car type (the leaver's own group is
`portCarGroups.slot_group[slot]` of its old slot; the port may also keep it). Record h is reused in place: nobody
is renumbered and `humanCarTotal` does not change. Steps:
1. checks: not the attract demo; h vacant (`TM2_HUMAN_ERR_RECORD` otherwise); `node` below `aiRouteNodeCount`
   (`TM2_HUMAN_ERR_NODE`); the group's first slot free (`portHumanFreeSlot`: a valid car slot below the vacant slot,
   kind 2, index -1; with the drop-in not unloaded) (`TM2_HUMAN_ERR_INDEX`); with the drop-in, fewer than
   `TM2_PORT_CARS` cars driving (`TM2_HUMAN_ERR_FULL`);
2. the record cleared (the vacant state goes), `player` / `f002` = h, type = the car's; the vacancy bit and
   `portHumanVacant.ai[h]` cleared;
3. the per-record part of `uaInit` as `portCarStartHumanAt` does it (reset weapons, damageable, pad configuration 1),
   h's bindings from `portHumanLayout[h]`, `hudResetSpeed(h)`;
4. the binding: `portCarBindSlot(0, h, slot)` (object id `TM2_HUMAN_ID(h)`, `humanCarTypes[h]`), `portCarBindGroup`
   of h = the group (a later rebind keeps h on that car);
5. the start `portHumanRespawn` makes (`portHumanRestartRecord`: fresh stats, full HP with the human +20, fresh
   weapons, state flags cleared, at route node `node` or, for `node` < 0, the start farthest from every live car);
   none of `carRestart`'s match-wide resets; view mode 2 (outside the demo); the car drawn, body shown, damage
   stage 0 (`portCarModelOn`).

Write `portHumanLocal[h]` (its slot on this machine, or -1), `portHumanLayout[h]` and `portHumanLives[h]` before the
call. As every start (`portHumanRespawn`, `portCarStartHumanAt`), the start draws one `rand()` for the heading: the
same on every machine at the same step; no new `rand()` call site. With `TM2_PORT_CAR_DROPIN`,
`portCarStartHumanAt(h, group, flags, node)` with a vacant `h` (below `humanCarTotal`) does the same through
`portHumanRejoin` (its flags do not apply there: no retail restart, no random start); index `humanCarTotal` is a
new record as before.

### 8.5 Request 50 (c): `TM2_PORT_CAR_DROPIN`

Neither (a) nor the leave / return need it: they use the swap's blocks and the drop-in's reference blocks, which
`TM2_HUMAN_HANDOVER` now builds without the drop-in (`port_variant_arrays.c`: the declarations up to `portDropInRefs`
and `portDropInSlotSound` are shared; the rest stays the drop-in's). Only (b)'s new computer car needs it
(`portCarStartAi`). What `TM2_PORT_CAR_DROPIN` changes in a match that never calls it (`CAR_GROUPS.md` 14): five game
functions: `sessionBindCars` (the settings' counts while `portCarDropIn.live` is 0), `sessionLoadArenaAndCars`
(`portCarLoadDone` records where the DMD area ends), `portStartNodePick` (reads a flag that is 0), `csAlloc` (an
empty free list first: bump allocation as retail), `bitmapSetupStorage` (the load's VRAM bitmap, slot table and image
records in `portCarLiveTex` instead of packet buffer 0 and the view-1 ordering table: the same bytes written and
read, in another place; retail's copies were overwritten by the first frames); no `rand()` added or moved; the
storage (`portCarDropIn`, `portCarLiveTex`, `portCarUnload`) exists in every variant already, so only `.port_text`
grows. The 29 test matches: the simulation is the same step for step; what differs is memory the game no longer
uses after the load (packet buffer 0 and the OT keep frame data only). For rollback: `portCarLiveTex` (0x42088
bytes) is in `port.bss`, so in the snapshot and hash whether the define is on or not (zero without it); with the
define it holds the load's bookkeeping (deterministic; the port may add a "no" row once it uses no mid-match load).

### 8.6 Changes and storage

- Game functions (5, all `TM2_HUMAN_HANDOVER`): `worldUpdate`, `humanCarsRestart`, `humanCarNearest`,
  `worldCollapseUpdate`, `sessionBindCars`. Port: 7 exported (`portHumanHandOver`, `portHumanLeave`,
  `portHumanReturn`, `portHumanVacantCount`, `portHumanFirstPresent`, `portHumanBindAiCount`, `portHumanVacantReset`;
  request 50b: `portHumanRejoin`, `portHumanFreeGroupOfType`) + 9 static helpers (50b: `portHumanFreeSlot`); with
  the drop-in `portCarStartHumanAt` hands a vacant index to `portHumanRejoin`; `portHumanPresent` tests the vacancy; with the drop-in `portHumanCompact`,
  `portCarActiveCount`, `portCarBindCount` too. `portDropInSlotSound` moved up (shared).
- Storage: `portHumanVacant` (0x14C bytes since 50b, `.port_bss` last, every variant; tsv row, rollback row).
- No `rand()` call of their own, no `aiCarStart` / `carRestart` / `carBindAiSlot` / `portHumanCompact` in the three
  calls (check 12, call graph on the link); as in the swap and `portCarRemove`, ending a special can reach the game's
  `carSpinOutById` / `animSetStep` (a leap's turbo flame off), which may draw `rand()` as at the same events in the
  game: the same on every machine.

### 8.7 Status and proofs

Built and proved (static), `work/build-20261020-151724-925058` (`build.py --source-only --sdk-from-lib --relocatable
--port-variant`) and `tools/build_source_only.py` links on its objects, each proved against its E0 (`verify_reloc.py`)
with the placement check (limits forced to 2) passing. Retail images unchanged: hybrid `9219f7ba…`, source-only
`d755c86e…`, `--sdk-from-lib` `f557c1e3…` (`BUILD_MANIFEST.json` unchanged).

| configuration | P4 sha256 | P2 sha256 | `port_changed` (code) | port end |
|---|---|---|---|---|
| no port define (default variant; `portHumanVacant` added, only `.port_text` moves) | `6379a24d…` | | 0 | 0x80850BF0 |
| `TM2_PORT_HUMANS=16` `TM2_PORT_CARS=16` `TM2_PORT_CAR_GROUPS` `TM2_PORT_CAR_SWAP` (`r50_hs`) | `6255d4dc…` | `8128d229…` | 154 | 0x8089B57C |
| the port's defines (`tools/decomp/wc.py` list + `TM2_PORT_HOOKS`, `TM2_PORT_FAR_LOD`; `r50_port`) | `d6ec416a…` | `0b38eb3f…` | 269 | 0x808BFEE0 |
| every port define with `TM2_PORT_CAR_DROPIN` and `TM2_PORT_HUMANS=16` (`r50_allh16`) | `0fb0d9d2…` | `7e6bd7eb…` | 280 | 0x808C62E0 |

`tools/check_humans.py` check 12 (source: 30 human loops reviewed, 5 gated, 25 inert; the port side; with the drop-in;
two negative controls: without `TM2_PORT_CAR_SWAP` no gate is built, and a copy of `arena_setup.c` without
`humanCarsRestart`'s gate fails) and its build part on each link; `tools/check_all.py` on the full build and the
three link pairs: 27/27 checks pass.

**Request 50b (the joiner in place).** `work/build-20261020-165801-222267` (retail images unchanged: `9219f7ba…`,
`d755c86e…`, `f557c1e3…`; the default variant still `6379a24d…`, port end 0x80850BF0) and links on its objects, each
proved against its E0 with the placement check passing:

| configuration | P4 sha256 | P2 sha256 | port end | `portHumanRejoin` | `portHumanFreeGroupOfType` |
|---|---|---|---|---|---|
| humans + cars + groups + swap (`r50b_hs`) | `cfb4616f…` | `1f5a1672…` | 0x8089B95C | 0x8089B26C | 0x8089B18C |
| the port's defines (`r50b_port`) | `f3b896ee…` | `497066f2…` | 0x808C02A0 | 0x808B9ED4 | 0x808B9DF4 |
| every define with the drop-in (`r50b_allh16`) | `75bc6c41…` | `7f1df4b8…` | 0x808C6730 | 0x808BFBD4 | (`portCarStartHumanAt` 0x808B902C) |

The other request-50 functions in the port's build moved by a few bytes: `portHumanHandOver` 0x808B9484,
`portHumanLeave` 0x808B9808, `portHumanReturn` 0x808B9950, `portHumanVacantCount` 0x808B8A18,
`portHumanFirstPresent` 0x808B8A88, `portHumanBindAiCount` 0x808B8AF0, `portHumanVacantReset` 0x808B89CC;
`portHumanVacant` 0x808AC81C. Check 12 adds the rejoin (its body clears the vacancy, takes a free car only, binds h
and starts it through `portHumanRestartRecord`, writes no `humanCarTotal`; on the links its call graph reaches
`humanCarPlaceAtStart` and no compaction, rebind or restart; with the drop-in `portCarStartHumanAt` hands a vacant
index to it) and a third negative control (three broken copies of its body fail). `tools/check_all.py`: 27/27.

Not runtime-tested (the port's: T14 below).

| # | scenario | pass when |
|---|---|---|
| T14 | **a player leaves, a bot takes over** (B with the port's defines; `humans` 6, options 0x0B; X and Y): at step F human 2 (alive, with missiles in the air, a mine down, 60 % HP) leaves: both call `portHumanHandOver(2, -1)` (returns the new AI index) between steps; then human 4 leaves with `portHumanLeave(4)`; later a player rejoins: `portHumanReturn(2, j)` (j the last AI record) and `portHumanRespawn(2, -1, 0)` | hash X = Y every step; AI j drives off from human 2's place with its HP and weapons, its missiles credit j and do not hit it; no camera / HUD / radar for human 2 or 4; `portHumanAliveCount` 4; the next round (`portRoundEnd`, mode 4 rebind) binds AI j on that car and skips humans 2 and 4; the return gives human 2 its car back |
| T15 | **a joiner in a vacant slot on a free car** (as T14): human 4 left with `portHumanLeave(4)` (its car parked); a new player joins: write `portHumanLocal[4]`, `portHumanLayout[4]`, `portHumanLives[4]`, then `portHumanRejoin(4, portHumanFreeGroupOfType(type), -1)` on every machine at the same step | hash X = Y; `humanCarTotal` and every other human index unchanged; human 4 starts fresh at the farthest free start (full HP, fresh weapons), has its view / HUD where local, is counted by `portHumanAliveCount`; `portHumanVacant.mask` bit 4 clear; a second rejoin of 4 refused (`TM2_HUMAN_ERR_RECORD`), a rejoin on a driven car refused (`TM2_HUMAN_ERR_INDEX`) |
