# A player's Dark Tooth (`TM2_PORT_DT_PLAYER`, PC port request 36)

Retail lets only the computer drive Dark Tooth: he is the Survival / tournament boss, run by
`aiDarkToothBossUpdate` (`SURVIVAL.md` 1). This port option lets a human record drive him (car-select index 14,
type 0xF0) and plays him as close to the boss fight as a player's car can be. The fan mod in the port's
`references/mods/` was read as a reference only (the research: what it changes, what breaks for a human 0xF0);
none of its code is used: everything below is our own C.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CAR_SPECIALS --port-define TM2_PORT_DARK_TOOTH \
    --port-define TM2_PORT_DT_PLAYER [other port defines] --output work/pv/P4
```

`TM2_PORT_DT_PLAYER` needs `TM2_PORT_DARK_TOOTH` (and so `TM2_PORT_CAR_SPECIALS`) and `TM2_PORT_CAR_GROUPS`
(`#error` otherwise, `include/semantic/dt_player.h`): a player's Dark Tooth uses his car key's own head and boss
words (`CAR_GROUPS.md` 11). It forces `TM2_TEX_SLOT_STRIDE` 32 (`players.h`). Without a human Dark Tooth and with
`portCarOptions` bit 8 clear, every changed line runs retail's logic (`CAR_GROUPS.md` 11's retail rules included).

## 1. What a player's Dark Tooth does, next to the boss

| | the boss (retail, `aiDarkToothBossUpdate`, stage 1) | a player's Dark Tooth |
|---|---|---|
| machine guns | off (the boss update replaces them) | **off**: the machine-gun button fires the boss's missiles |
| missiles | `weaponFireDarkToothMissile`: type 61, 5 damage, the spiral model (2301, in BB.DMD), turn rate 99 or 20 by `rand()`, speed x5, the pink trail and sparks; every `rand() / 500 + 10` updates (10..75); sides alternating; front points (+-120, 272, 104) toward the target ahead, rear points (+-72, -224, 56) when its AI target is behind; pitch 170, heading + target bearing; no sound per shot | the same missile and damage (`portCarDtPlayerGuns`), while the button is held every `portDtPlayer.fire_rate` updates (0 = **10**, the boss's quickest); sides alternating (the record's `gun_side`); the boss's **front** points, pitch 170, his heading; target: the nearest car ahead (`weaponFindTargetAhead`, player mode); no sound per shot (as the boss); not while frozen or dead |
| missile wobble | the yaw wobble of rate-20 missiles: level 8 (or `portCarOptions` bit 6) | **on every arena** (`portCarDtPlayerShooter` in `weaponProjectileUpdate`) |
| special | none (`weaponFireSpecial` case 0xF0: nothing, the ammo is spent) | **the head swoop** (`portCarDtPlayerSwoop`): at the nearest car, **human or AI**, alive and drawn, within the boss's range (Chebyshev 500 x 8 = **4000** world units of the truck); **refunded** when there is none (or the head is still swooping) |
| swoop | every 100..200 updates (`rand()`) at the nearest **human** within 4000, sound 61 thirty updates before, 66 at the start; the head blends 70 toward the car with a bob (`rsin`), turns to it, hits within 160: **7 damage**, the car set **burning** (unless shielded, in water or fire) with sound 6; the world stops it; it ends after 40 updates or when the car is 4000 away | the boss's swoop code (`portCarDtPlayerSwoopStep`) with that car: 7 damage (x2 from him, see 1.1), burning, sound 6, 40 updates, 4000; sound 66 at the start (no 61: the player chose the moment) |
| head | hovers 240 above the truck (its frame), **blended** 40 per update; sways: pitch +-100 per update between 683 and -682, yaw + the yaw step (150); stage 2: -24 | his own head (his key's `darkToothBossObject`, asset 249) **placed exactly** 240 above the truck each update (`portCarDtPlayerPlace`, no blend), the boss's sway; after a swoop it comes back as the boss's does (blend 40) until within 16 world units, then exact again; hidden while he is dead, cloaked (car object hidden) or parked |
| stage 2 / boss controller | level 8 (`aiCars[0]`) or bit 6: the head on the car, HP 110 | **none**: `aiDarkToothBossUpdate` stays the computer's (check 14) |
| death | the burst path with wreck state **3**: `carWreckUpdate` bursts the wreck again and again (`SURVIVAL.md` 1.4) | one burst of his model, explosion, wreck state **4** (`portCarDarkToothWreck` returns 4 for a human): a normal car's final wreck |
| specials count / recharge | AI: unlimited | `portDtPlayer.max_specials` / `recharge` (0 = **5 / 360**: Sweet Tooth's; retail's 0xF0 row is 5 / 700) |
| missile hit size against him | +25 (DISTANCE units, 200 world) for an AI 0xF0 | the same +25 for a human 0xF0 (`projectileHitTest`) |
| camera | (no camera) | chase (views 2, 7) -830 back / 362 up, view 3 -830 / 618, split 1 -450 / 592, split 2 -1518 / 670 (`viewCameraOffsetsInit`); other full-screen views keep -1200 / 800 |
| engine sound | none (no 0xF0 case in `carSoundUpdate`) | **Minion's engine, 0x41** (`portDtPlayer.engine_sound` 0; > 0 another id, < 0 none as retail) |
| Hong Kong boss markers (`carContactApplyObject` type-240 tail) | warp 2 and spin-outs for the boss | **skipped for a human** (retail would call `carSpinOut(car, 0, ..)`, the AI layout, on a human record) |

### 1.1 Retail facts that stay (decision 1: vanilla)

A human Dark Tooth has 300 HP (+20 as every human: 320 in play) and mass 100 (`carStatsDarkTooth`): `carApplyDamage` **doubles every damage a human
with mass 100 deals** (only Dark Tooth) and his knock-back is 0. So his missiles do 10, his swoop 14 (+ burning).
`roof_height` stays retail's 106 (his top collision point; the cameras come from `viewCameraOffsetsInit`, not from
it, except the spectator view 7). A human's projectiles test a radius of 15 (owner id < 50) against 8 for the AI's
(`weaponProjectileUpdate`): his missiles hit a little more easily than the boss's, as every player missile does. The
ice block of a frozen Dark Tooth is drawn under his head's node (retail, `SURVIVAL.md` 1.1): with the head over the
truck it shows up there (cosmetic).

### 1.2 What is game state (rollback, lockstep)

- **Hover:** the head's position is a function of the car (position, `rot_m`) only: recomputed each update.
- **Sway:** the head's `rot[1]` / `rot[2]` and the key's pitch and yaw steps (`aiDarkToothPitchStep`,
  `aiDarkToothYawStep`) accumulate, as the boss's.
- **Swoop and the way back:** the head's position, `darkToothSwooping` (0 hover, 1 swoop, 2 back),
  `aiDarkToothSwoopTimer`, `aiDarkToothSwoopTarget` (human i, or 0x100 + AI j) and `darkToothSwoopVectors` of his
  key, and the burning it sets: simulation state, as the boss's. The first Dark Tooth's are the retail globals
  (80180B4C.., 80180F18.., 80183F64), a duplicate's `portCarSpecials.dark_tooth[d]`; the head object is in the
  coordinate-system pool. A rollback that saves the game RAM and the port sections has them.
- **Missiles:** the record's `gun_cooldown` and `gun_side`; `weaponFireDarkToothMissile` draws `rand()` (turn
  rate) as the boss's does. The swoop's `carApplyDamage` may draw `rand()` (a spin-out at 8+ damage: 14 >= 8), as
  the boss's does. The head placement draws none (check 14).

## 2. Car select

### 2.1 Index 14

Retail's `carIndexToType` already maps index 14 to 0xF0 and `carFileCode` / `carDmdFileName` / `carTmsFileName`
to `DMD\BB.DMD`, `TMS\BB.TMS`; `carDisplayName(14)` is "Dark Tooth", `carTypeName(0xF0)` "DARK TOOTH" (the
target panel), the radar blip is olive (`hudRadarPlotCar` 0xF0). The carousel (CARSEL.DMD's select node 9005: +6
14 children, +7 the choice, +16 the child pointers) wraps at 14, so 14 was unreachable.

With the define:
- `carIsSelectable` (8012CC10): 14 only with `portCarOptions` bit 8 (`TM2_PORT_CAR_OPT_DT_PLAYER`, 0x100, read live;
  before the repeat test, as Sweet Tooth's and Minion's unlocks). Two players on Dark Tooth need bit 0 (repeats).
- `screenCarSelect` (801307EC): after CARSEL is loaded and before the first draw (the draw binds the carousel's
  model once), `portCarSelExtend(carousel)` gives the carousel scene object (+4) a **copy of node 9005 with 15
  children** in `portDtPlayer.node` (bit 8 set and CARSEL's node as expected; rebuilt at every car select entry,
  CARSEL is reloaded each time). The step commands (`sceneCmdChoiceStepRun`: `% node[6]`, R1's random pick
  included), the choice getters / setters and the name label then work on 0..14 unchanged. Bit 8 clear: CARSEL's
  node, and a start car left at 14 goes back to 13. The "info" return (`carSelectStartCar`) wraps at the
  carousel's count (`portCarSelCount()`).
- Child 14 is `portDtPlayer.carsel_model` when the port sets it, else an **empty node** (a hidden group: the walker
  skips it). So by default the carousel shows nothing for Dark Tooth and **the port draws his model** (2.2).

### 2.2 Drawing him on the carousel: why the game cannot (the result)

Tried first, as asked: install BB.DMD in the shell (callback-free: `modelRelocate` + a node lookup, no
`worldLoadObject`) and point child 14 at its truck. It does not fit **on the PS1's VRAM**:

- The shell keeps CARSEL.TMS resident: 52 images, 287,744 VRAM halfwords + 10,064 of CLUTs, placed at their own
  coordinates (group 0); with the frame buffers (640 x 256, as `bitmapInit` reserves) and the font / system areas,
  **49,328 halfwords** of VRAM are free (counted on the disc's files, scratch model).
- BB.TMS needs **79,616 halfwords of images + 3,136 of CLUTs** (31 images): 33,000 more than all the free VRAM, before
  any fragmentation. Every carousel car's textures stay in VRAM (the carousel turns through all 14), so nothing can be
  evicted for him.
- The shell also keeps no texture bookkeeping: `bitmapParseContainer` of a car group would place through
  `vramPlaceTexture` with the last game load's usage bitmap / slot table / records (stale, and in packet buffer 0,
  which the shell draws from) and `bitmapFindRecord` could reuse a stale arena image by key.
- RAM would fit (CARSEL.DMD ends at 0x80034CD0; 79,264 bytes to the picture buffer at 0x80048270 hold BB.DMD's 61,660),
  and the framing would need a per-index offset (his truck is about twice a carousel car).

So the variant gives the index, the gating, the guards and an empty child; the port draws him (it already draws
his portrait and name). **Plug-in:** `portDtPlayer.carsel_model` = a model node the port has installed and
textured itself (e.g. in a PC-side texture bank of its GPU, `TEXTURE_BANKS.md` 6.8), relocated, of the kind the
carousel's other children are (the walker draws `child[node[7]]`); it is read at each car select entry.

## 3. The port's plug-in points

| what | where | default |
|---|---|---|
| offer Dark Tooth in car select | `portCarOptions` bit 8 (0x100), read live by `carIsSelectable` and at each car select entry | clear: not offered (retail) |
| his bio picture ("info" on index 14) | `portDtPlayer.bio_picture`: a TIM file name for `screenLoadImageFile` (as `carBioPictureFiles`, e.g. `BIOS\BBINFO.TIM` if the port ships one), shown as the other cars' | 0: no picture (info returns at once) |
| his portrait and name on the carousel | the port's own drawing (2.2); or `portDtPlayer.carsel_model` | empty child |
| his name in the game | retail: `carDisplayName` "Dark Tooth", `carTypeName` "DARK TOOTH" | retail |
| his ending movie (tournament win) | `portDtPlayer.ending_movie` / `ending_frames` / `ending_wide` (`moviePlayEnding`'s arguments) | 0: none; YOUWON.STR still plays |
| missile interval | `portDtPlayer.fire_rate` (updates) | 0 = 10 |
| special recharge / count | `portDtPlayer.recharge`, `max_specials` (read by `weaponBlockInit` at each start / restart) | 0 = 360 / 5 |
| engine sound | `portDtPlayer.engine_sound` (a sound id; with `TM2_PORT_CAR_SOUND` his own instance) | 0 = Minion's 0x41; < 0 = none |
| tournament password | none for him (as Sweet Tooth and Minion: the password's car field is 4 bits) | |

## 4. What changed

### 4.1 Game functions (14; + 4 for the stride without `TM2_PORT_CARS`)

| VA | function | unit | change under the define |
|---|---|---|---|
| 8012CC10 | `carIsSelectable` | `car_is_selectable.c` | 14 only with bit 8 |
| 801307EC | `screenCarSelect` | `screen_car_select.c` | `portCarSelExtend` after the load; start car `% portCarSelCount()`; info picture `portCarBioFile(i)` (none: no picture) |
| 80136D98 | `screenEnding` | `screen_ending_setup.c` | index >= 14: the port's movie or none (no `endingMovieTable[14]` = NULL) |
| 8012BE5C | `sessionRun` | `session_runner.cpp` | no password line for 14 (`(u8)(car - 12) >= 3`) |
| 800FF764 | `viewCameraOffsetsInit` | `ai_car_start.c` | 0xF0 camera offsets |
| 80105A7C | `weaponFireSpecial` | `weapon_basic.c` | human 0xF0: `portCarDtPlayerSwoop`, refund when 0 |
| 8010388C | `weaponTick` | `weapon_tick.c` | human 0xF0: `portCarDtPlayerGuns` takes the machine-gun request |
| 800DC88C | `humanCarTick` | `car_tick.c` | after `weaponTick`: `portCarDtPlayerHead` for a 0xF0 |
| 800E06CC | `carContactApplyObject` | `car_object_contact.c` | the type-240 tail skips humans |
| 800D6FD8 | `projectileHitTest` | `weapon_slam_swing.c` | +25 for a human 0xF0 |
| 800DB110 | `weaponProjectileUpdate` | `projectile_hit.c` | the wobble also for `portCarDtPlayerShooter(owner)` |
| 8011DB74 | `carSoundUpdate` | `screen_card_draw.c` | human 0xF0: `TM2_DT_PLAYER_ENGINE()` (`portDtPlayer.engine_sound`, 0 = Minion's 0x41, < 0 none) |
| 800FDCAC | `weaponBlockInit` | `ai_route_setup.c` | human 0xF0: recharge / max specials |
| 800E5628 | `carApplyDamage` | `car_damage.c` | after a won round only the computer boss takes damage (request 41 item A, section 9) |
| 80121764, 801209E8, 80120D3C, 80120744 | `bitmapAddRecord`, `vramPlaceTexture`, `vramUploadClut`, `modelRemapTextures` | | `TM2_TEX_SLOT_STRIDE` 32 (already 32 with `TM2_PORT_CARS`) |

Unchanged: `carIndexToType`, `carFileCode`, `carDisplayName`, `carTypeName`, `carBindHumanByType`,
`carStatsDarkTooth`, `carPartRegisterDarkTooth` / `carPartsSpinDarkTooth` / `carPartsAnimate` (his wheels already
turn per key), `aiDarkToothBossUpdate`, `carApplyDamage` apart from the won-round test (its wreck state comes from
`portCarDarkToothWreck`),
`carDestroyed` (its timer only delays a self-hit that finds wreck 4), `settingsSetHumanCar` (`s8`: 14 fits),
`hudRadarPlotCar`.

### 4.2 Port functions (`.port_text`, `port_variant_arrays.c`; `include/semantic/dt_player.h`)

| function | groups + specials + DT + DT_PLAYER | every port define (with `TM2_PORT_HAZARDS`) + DT_PLAYER | use |
|---|---|---|---|
| `void portCarDtPlayerHead(void *human)` | 0x80544B54 | 0x80585470 | `humanCarTick`: show / hide, hover, swoop, way back |
| `void portCarDtPlayerGuns(void *human)` | 0x80545174 | 0x80585A98 | `weaponTick`: the missiles |
| `int portCarDtPlayerSwoop(void *human)` | 0x80544CA4 | 0x805855C0 | `weaponFireSpecial`: start the swoop (0 = refund) |
| `int portCarDtPlayerShooter(int owner)` | 0x805452DC | 0x80585C00 | the wobble test |
| `void portCarSelExtend(void *carousel)` | 0x80545380 | 0x80585CA4 | `screenCarSelect`: the 15-child node |
| `int portCarSelCount(void)` | 0x80545484 | 0x80585DA8 | the carousel's count (14 / 15) |
| `const char *portCarBioFile(int index)` | 0x805454A4 | 0x80585DC8 | the info picture of an index |

Static helpers: `portCarDtPlayerPlace` (the hover and sway, no `rand()`), `portCarDtPlayerSwoopStep` (one swoop
update), `portDtTarget`, `portDtTargetLive`. Changed: `portCarDarkToothWreck` (a human: 4); with
`TM2_PORT_CAR_SOUND`, `portCarSoundFrame` (a human Dark Tooth with an engine is a candidate for his engine
instance; without this his engine never played with `portCarOptions` bit 4); with `TM2_PORT_CAR_SWAP`,
`portCarResetLook` (bit 4 clear: a human Dark Tooth's engine id is stopped as the other cars' are) and
`TM2_PORT_CAR_SWAP`, `portCarModelOn` (a human Dark Tooth's head shown with his car, not only an AI's).

### 4.3 Storage: `portDtPlayer` (148 bytes, `.port_bss`, every variant, after `portCarSoundLoops`)

Address: 0x80542F54 with groups + specials + DT + DT_PLAYER, 0x8057F464 with every port define (read it from the
build's `symbols.tsv`); in the default variant it is there but unused.

| offset | field | written by | meaning |
|---|---|---|---|
| +0x00 | `fire_rate` | port | updates between two missiles; 0 = 10 |
| +0x04 | `recharge` | port | his `recharge_period`; 0 = 360 |
| +0x08 | `max_specials` | port | 0 = 5 |
| +0x0C | `engine_sound` | port | his engine sound id; 0 = Minion's 0x41 (decision 3); < 0 = none (retail) |
| +0x10 | `bio_picture` | port | index 14's info TIM (char *); 0 = none |
| +0x14 | `ending_movie` | port | index 14's ending movie (char *); 0 = none |
| +0x18 | `ending_frames` | port | its frames |
| +0x1C | `ending_wide` | port | its wide flag |
| +0x20 | `carsel_model` | port | the carousel's child 14; 0 = the empty node |
| +0x24 | `carsel_count` | C | 15 while the carousel is extended, else 0 |
| +0x28 | `blank[28]` | C | the empty node (type 1, +21 = 2) |
| +0x44 | `node[80]` | C | the extended select node (16-byte header, 15 child pointers) |

No other storage: his head and swoop words are his key's `CAR_GROUPS.md` 11 state (a human never runs the boss
update, so they are free), the missile timer and side are the record's `gun_cooldown` / `gun_side`.

## 5. Interactions

- **Keys (`CAR_GROUPS.md` 11):** humans load first, so the first Dark Tooth of a match is usually a player's: key
  0xF0 = the retail globals; a second one (another player, or the AI boss on level 8) a duplicate key with
  `portCarSpecials.dark_tooth[d]`. Each has its own head, timers and swoop. **Two player Dark Tooths work** (bit 0 for
  the repeat); each fires, swoops and dies on his own key.
- **Level 8 with a player Dark Tooth:** the two Dark Tooths go head to head (decision 2, checked in 7.2): the
  tournament boss still runs (a second Dark Tooth, a duplicate key; its stage-2 rule reads `aiCars[0]`'s key); it
  swoops at the nearest human, which may be him.
- **`TM2_PORT_SPECIAL_AS`:** 0xF0 stays refused as a lender; a player Dark Tooth that borrows a special fires it
  (the type switch sees the borrowed type), his own swoop otherwise. A borrowed special's recharge / count
  (`portCarSpecialAsBlockInit`) overrides `portDtPlayer`'s.
- **`TM2_PORT_CAR_SOUND`:** the swoop sound 66 is his instance's (`portCarSoundPlay(66, .., slot)`), the engine
  (Minion's 0x41 by default) his slot's: `portCarSoundFrame` counts him as a car with an engine (7.3).
- **`TM2_PORT_CAR_SWAP` / `TM2_PORT_CAR_DROPIN`:** a record swapped onto a Dark Tooth gets the camera, block and
  head (`portCarApplyType` runs `viewCameraOffsetsInit` / `weaponBlockInit`, `portCarModelOn`); a removed record's
  swoop target ends as the boss's (`portDropInSpecials` by human index; an AI target is re-checked each update).
- **VRAM (`SURVIVAL.md` 4.1):** his 31 images (11 256-colour and 20 16-colour CLUTs) load before the opponents. Paris (and Paris 2P) and
  Swamp cannot hold him beside another car type without texture banks (`portCarOptions` bit 5) or rows 240..255
  (bit 6 / bit 7); Cyburbia and Roof Tops are tight. Use banks, or refuse those arenas.
- **Memory:** one BB.DMD (61,660 bytes) per Dark Tooth, as for the AI ones (`CAR_GROUPS.md` 11.5).

## 6. Status and proofs

- **Built and proved** (`tools/build_source_only.py` + `verify_reloc.py` per configuration, the variant and its
  placement check as `build.py` links them; RAM end 0x80C00000). The retail images keep their hashes: hybrid
  `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`.

| configuration | variant sha256 (P4) | placement (limits 2) | `port_changed` | port sections end |
  |---|---|---|---|---|
  | no port define (default variant; `portDtPlayer` added to `.port_bss`) | `dfa2f404…` | `b95e6fed…` | none | 0x805431B0 |
  | groups + specials + Dark Tooth (no `TM2_PORT_DT_PLAYER`: as before) | `5dd84081…` | `3547b737…` | 63 | 0x80545CE8 |
  | groups + specials + Dark Tooth + `TM2_PORT_DT_PLAYER` | `949b189c…` | `78c51f7d…` | 73 (63 + 13 - 7 already changed + the 4 stride functions) | 0x80547088 |
  | the full car stack (groups, `CARS=16`, swap, banks, specials, Dark Tooth, sound, CLUTs, drop-in, special-as) + `TM2_PORT_DT_PLAYER` | `69b2df98…` | `9842513a…` | 152 | 0x8058D308 |
  | every port define (+ far LOD, far distance, SPU48) + `TM2_PORT_DT_PLAYER` | `0fe214a8…` | `b614e022…` | 192 | 0x8058F1D8 |
  | the same + `TM2_PORT_HUMANS=4` | `0fe214a8…` (identical: 4 is the variant's default) | `b614e022…` | 192 | 0x8058F1D8 |

  Every one passes `verify_reloc.py` against E0 and the placement check. `TM2_PORT_DT_PLAYER` without
  `TM2_PORT_DARK_TOOTH` stops at the `#error` (checked). The full `build.py --source-only --sdk-from-lib --relocatable --port-variant`
  (`work/build-20261014-162750-198430`) keeps the three retail hashes and publishes the default variant `dfa2f404…`
  (`out/TM2_SOURCE_PORT.EXE`, port sections end 0x805431B0).

  **After the decisions of 2026-10-14** (section 7: Minion's engine by default, the engine with bit 4, the swap's
  engine stop; on top of request 38) the proofs again, all passing `verify_reloc.py` against E0, the placement check
  and `check_car_groups.py --expect-functions` (check 14 included):

  | configuration | variant sha256 (P4) | placement (limits 2) | `port_changed` | port sections end |
  |---|---|---|---|---|
  | groups + specials + Dark Tooth + `TM2_PORT_DT_PLAYER` | `1ac458b7…` | `117ad7c9…` | 73 (73 expected) | 0x80547098 |
  | the full car stack + `TM2_PORT_DT_PLAYER` | `9bdd7a78…` | `6433714a…` | 152 (152 expected) | 0x8058D398 |
  | every port define (car stack, far LOD, far distance, SPU48, `TM2_PORT_HAZARDS`) + `TM2_PORT_DT_PLAYER` | `25db1f2d…` | `ce4b6b44…` | 200 (152 + the other defines' 48) | 0x8058F4BC |

  The full build (`work/build-20261014-192519-936605`) keeps hybrid `9219f7ba…`, source-only `d755c86e…`,
  `--sdk-from-lib` `f557c1e3…`, and the default variant `274d0ae4…` (unchanged: every change is under the define).

- **`tools/check_car_groups.py` check 14** (new; with `--expect-functions` `EXPECTED_DT_PLAYER`, + `DT_PLAYER_STRIDE`
  without `TM2_PORT_CARS`): the 9 port functions in `.port_text`, none calling `rand()`; the hooks; **the boss
  controller**: `aiDarkToothBossUpdate`'s only caller is `aiWeaponFireDecide`, whose only caller is `weaponTick` (its
  AI branch), neither is pointed to, and nothing the port functions reach is either of them; **the head placement**
  (`portCarDtPlayerPlace`) reaches only `mathMulMatVec`, `mathBlendValue` (and `timerGetFrameRate` below it),
  `RotMatrixYXZ`: no `rand()`; `portDtPlayer` is used by the port code and `screenEnding`, `carSoundUpdate`,
  `weaponBlockInit` only. Check 8 (the Dark Tooth state used only inside selections) passes with the new functions
  (they select his key). All OK in the four `TM2_PORT_DT_PLAYER` builds (`--expect-functions`: 73 of 73, 152 of 152; with the far / SPU48 defines the extra functions are theirs). Negative control: on the groups + specials + Dark Tooth build without the define, `--negative-control` runs check 14 and it fails (no port functions, no hooks, no placement function, no storage users).
- **Retail logic** with no human Dark Tooth and bit 8 clear, by construction: every new test is on a human record
  of type 0xF0, on index 14 (unreachable with bit 8 clear), on an owner that is a human Dark Tooth's object, or on
  `portDtPlayer` words read only for those; the stride 32 only moves where groups' slots are in the slot table.
- **Not runtime-tested.** Port tests: (1) bit 8 clear: car select and a match frame by frame equal to the variant
  without the define; (2) bit 8: left / right wrap 0..14, R1 lands on 14 sometimes, info on 14 with and without
  `bio_picture`, the name label, two players (bit 0); (3) Challenge Match as Dark Tooth against 1..9 cars on each
  arena (VRAM, 5): the head over the truck in every view and split, swaying, hidden at death / cloak, back at the
  restart; the button fires every 10 updates from alternating front corners, missiles home and wobble, 10 damage;
  the special swoops at AI and human cars within 4000, 14 damage, burning, sound 66; refunded with nobody near;
  recharge 360, 5 at most; his death: one burst, wreck 4, no repeated bursts; (4) Hong Kong: drive him over the boss
  markers: no warp, no spin-out; (5) tournament to level 8 as Dark Tooth: the boss fight with two Dark Tooths, the
  win: no NULL movie, no password line; (6) rollback mid-swoop: restore, compare.

## 7. Decisions (2026-10-14)

The user's answers to the open questions 1, 4, 6, 7 and 8 (section 8), and what each one needed.

| # | question | decision | in the C |
|---|---|---|---|
| 1 | balance | **vanilla**: x2 damage, 300 HP, mass 100, as retail would give a human Dark Tooth | nothing to change (7.1) |
| 2 | tournament level 8 as Dark Tooth | **the two Dark Tooths go head to head**: the final boss fight still happens (no skip as level 4's Minion skip) | nothing to change (7.2) |
| 3 | engine sound | **Minion's engine (0x41)** by default; still overridable | `portDtPlayer.engine_sound` 0 = 0x41 (7.3) |
| 4 | ending | **"You Won" only** for now (YOUWON.STR); the movie, portrait and bio plug-in points stay for later | nothing to change (the current default) |
| 5 | unlock | **the port's**: beat the game with every character, Sweet Tooth and Minion included; then the Vanilla Plus option "Playable Dark Tooth" becomes selectable and sets `portCarOptions` bit 8 while on | nothing to change: the decomp side is only bit 8 (7.4) |

### 7.1 Decision 1: vanilla damage, HP and mass (checked)

Nothing in the variant changes them for a human Dark Tooth (read in the C, every define):
- **300 HP, mass 100:** `carStatsLoad` 0xF0 → `carStatsDarkTooth` (`car_stats.c`), unchanged by every port define;
  as every human he gets retail's +20 maximum HP at the start (`humanCarPlaceAtStart`, `car_object_contact.c`):
  **320** in play. The swap's `portCarApplyType` loads the same stats (and the same +20). No port function writes `stats.mass`; the only other writer is
  retail's shield cheat in `humanCarTick` (`car_tick.c`: on 100, off 25, any car, as retail).
- **x2:** `carApplyDamage` (`car_damage.c`) doubles the amount when the attacker is a human with `stats.mass == 100`
  (retail; no `#ifdef` around it), so his missiles do 10, his swoop 14 (+ burning), his rams twice a car's. Knock-back
  against him: `target * mass / 100` = 0 (retail).
- `TM2_PORT_DT_PLAYER` changes only his recharge / count (`weaponBlockInit`, `portDtPlayer.recharge` /
  `max_specials`, 360 / 5 by default: the special is now the swoop), not damage, HP or mass.

### 7.2 Decision 2: level 8 head to head (checked statically)

With a human on index 14 (type 0xF0), the level-8 session (`sessionRun`, `session_runner.cpp`) runs retail's rules,
which never look at the human's car:
- **First win** (`dark_tooth_stage` 0): the intro text (`strDarkToothIntro1..3`), `sessionSetOpponentCar(.., 0, 14)`
  (a plain store, no `carIsSelectable` test: bit 0 and bit 8 do not matter), one opponent, `sessionLoadArenaAndCars`
  + `sessionBindCars`, `audioSetDarkToothMusic` (music mode 11). No password line (`dark_tooth_stage` set; and
  `(u8)(car - 12) >= 3` for index 14 at every level).
- **The binding:** each car is a load group with its own BB.DMD install (one per car, `SURVIVAL.md` 2.1), humans
  first: the human's group 1 registers its root first, so **his key is 0xF0** (the retail globals); the boss's group
  (humans + 1) finds 0xF0 taken and gets **an overflow key** (0x100, the next free one; `portCarRegisterRoot`).
  `sessionBindCars` binds by group (`portCarBindGroupOf` → `portCarSlotOfGroup`; `TM2_PORT_DT_PLAYER` needs
  `TM2_PORT_CAR_GROUPS`), so the boss is `aiCars[0]` on **his own** car slot. (Retail's search by type would have
  bound him to the human's slot, the first 0xF0: the define's `#error` rules that build out.)
- **Two heads:** `weaponAssetBind` 0xF9 runs inside the load key's selection (33-2a), so each install's `csAlloc`ed
  head goes to its own key: the human's in `darkToothBossObject` (80180F40), the boss's in
  `portCarSpecials.dark_tooth[0].head`. Two scene objects, two sets of the 7 scalars, the swoop words, the vectors
  and the stage.
- **The boss controller:** `aiWeaponFireDecide` (AI records only, from `weaponTick`'s AI branch) selects
  `aiCars[0]`'s key around `aiDarkToothBossUpdate` on level 8: the boss sways, fires and swoops on its own state,
  at the nearest **human** (him, or another player). The human's state is used only by `portCarDtPlayerHead` /
  `portCarDtPlayerSwoop`, which select **his** key (`portCarKey(h->slot, 0xF0)` = 0xF0). Nested selections work:
  a boss swoop that hits him runs `carApplyDamage`, which selects the human's key around its Dark Tooth block and
  restores the boss's (`portCarSpecialSelect` exchanges per part and nests). `check_car_groups.py` check 8 proves
  every use of the Dark Tooth state is inside a selection; check 14 that the port functions never reach
  `aiDarkToothBossUpdate` / `aiWeaponFireDecide`.
- **Stage 2 and "But wait...":** `aiCarsCountRemaining` tests `aiCars[0].type == 0xF0` on level 8 with every AI
  down and runs retail's stage change on **`aiCars[0]`'s key** (`TM2_CAR_KEY(aiCars[0].slot, ..)`): the boss's stage
  flag, `carStatsDarkToothStage2` (110 HP), alive, `rtScheduleEvent(3, 0)`. Result 3 → `dark_tooth_reveal`: the
  reveal text, the next round with the boss's head on its car (its hover -24, its own stage). The human's stage
  (`aiBossDefeatedFlag`, key 0xF0) stays 0: only `aiCarsCountRemaining` (aiCars[0]'s key), bit 6's loop (AI only)
  and the port's `portCarDarkToothStage` write a stage. The round end (`worldUpdate` → `portCarDarkToothRoundEnd`)
  reads and hides `aiCars[0]`'s head. The boss's final death (`carApplyDamage`, its key: `aiBossDefeated()` 1)
  bursts the boss's head and hides it; `portCarDarkToothWreck` keeps retail's 3 for the tournament boss and gives the
  human 4. Final win: "Congratulations" / "Meet Calypso", then `screenEnding` (decision 4).
- **What they do share** (as two cars of one type, all retail limits or cosmetic): the swoop sounds 61 / 66 (with
  `portCarOptions` bit 4 clear one request each: his swoop sound is dropped while the boss's plays, and the other
  way round; bit 4 gives each his instance), the olive radar colour. `roundIsWon`'s exception was shared too (13 in
  section 8); request 41 item A keeps it for the computer boss only (section 9).

Nothing merged them, so no fix was needed.

### 7.3 Decision 3: Minion's engine

Retail's per-type engine table (`carSoundUpdate` 8011DB74 and `worldUpdate`'s copy): Minion (0xE0) is **0x41**
(the others 0x24..0x3C even; Sweet Tooth 0x3C; Dark Tooth none). It is in `SCREEN\UACORE.VAB`, loaded for every
match, so it plays whatever cars are loaded. Now:
- `carSoundUpdate` (human 0xF0): `TM2_DT_PLAYER_ENGINE()` (`dt_player.h`): `portDtPlayer.engine_sound` when > 0,
  **0x41 when 0** (`TM2_DT_PLAYER_ENGINE_SOUND`), none (-1, retail) when < 0.
- `TM2_PORT_CAR_SOUND` (bit 4): `portCarSoundFrame` now counts a human Dark Tooth with an engine as a candidate, so
  his engine plays as his own instance (before, a type without a table engine was never allowed, so the port's id
  was silent with bit 4). 0x41's list is already sized as an engine's (Minion's).
- `TM2_PORT_CAR_SWAP`, bit 4 clear: `portCarResetLook` stops his engine id as the other cars' (`soundStop(0x41)`).
- Bit 4 clear, a Minion in the same match: one 0x41 request for both (two cars of one type, `CAR_GROUPS.md` 12.1).

### 7.4 Decision 5: the unlock recipe (the port's side)

The decomp's only switch is **`portCarOptions` bit 8** (`TM2_PORT_CAR_OPT_DT_PLAYER`, 0x100), read live by
`carIsSelectable` and at each car select entry. Everything else is the port's:

1. **What counts as "beating the game" with a car:** a won 1-player tournament (settings mode 1) on **Medium or
   Hard**: retail plays the ending only there (Easy stops after level 4 with `SHELLDB2\STOP.TIM`; `UNLOCKS.md` 3).
   The hook: `screenEnding` (80136D98) runs, the car = `settings.human_car[0]` (+8, car-select index 0..13). A
   2-player tournament ends with `SHELLDB2\COWIN.TIM` and no `screenEnding`: if the port counts it, the hook is the
   final win in `sessionRun` (level 8, `dark_tooth_reveal` set, result 3), both humans' cars (+8, +9).
2. **What the port stores:** one bit per car index 0..13 (14 bits) in its profile. Retail keeps nothing: no
   memory-card code, no ending record (`TOURNAMENT_PASSWORDS.md`, `UNLOCKS.md` 3).
3. **When all 14 bits are set:** the Vanilla Plus option "Playable Dark Tooth" becomes selectable; while it is on the
   port sets bit 8 (and clears it when off). It may change between matches (read live: the next car select sees it).
4. **Sweet Tooth and Minion first need their car-select codes** (Up, L1, Triangle, Right / L1, Up, Down, Left), which
   retail keeps in RAM only (`unlockSweetTooth` 80180D04, `unlockMinion` 80180D05; `UNLOCKS.md` 1): the port's profile
   can store and restore those two flags too.
5. **Passwords:** retail's password holds the car in 4 bits (0..11), the level and the Dark Tooth stage
   (`TOURNAMENT_PASSWORDS.md`); none is shown for 12, 13 and (with the define) 14, so Sweet Tooth's, Minion's and
   Dark Tooth's tournaments cannot be resumed by password. The other 12 can be resumed at any level, even level 8:
   whether a password-resumed win counts is the port's choice (it can tell by tracking whether the run started at
   level 1 in its own state).
6. A Dark Tooth tournament win is not needed (he is the reward), but the port may record it (index 14) for a gallery.

No C change: bit 8 already gates everything (section 2.1).

## 8. Questions for the port

From the research plan (the answered ones are above: the special = the swoop, the button = the missiles, phase 1
only, car select tried in the game first). Questions 1, 4, 6, 7 and 8 are answered by the decisions of 2026-10-14
(section 7); the others are still open.

1. ~~**Balance**~~ **answered** (7, decision 1): vanilla.
2. **Recharge / count:** 360 / 5 (Sweet Tooth's) chosen as the default for the swoop (7 x 2 + burning, refunded
   when nothing is near); retail's 0xF0 row is 700 / 5. Keep?
3. **Head-only variant** (the mod's second index: the head alone on the car, the truck hidden): wanted later? (The
   per-key stage words make it a small change: hover -24 and `csSetDrawEnable(truck, 0)`.)
4. ~~**Tournament level 8 as Dark Tooth**~~ **answered** (7, decision 2): the boss fight stays, head to head.
5. **`roof_height`:** retail 106 kept (his top collision point; the cameras do not use it). 200 as the mod?
6. ~~**Engine sound**~~ **answered** (7, decision 3): Minion's 0x41 by default.
7. ~~**Ending**~~ **answered** (7, decision 4): YOUWON only for now.
8. ~~**Unlock**~~ **answered** (7, decision 5): the port's progress unlock, then a Vanilla Plus option sets bit 8.
9. **Rear missiles:** the boss fires from its rear points when its target is behind; a player fires forward only.
   Rear fire on a button combination (`weap->rear_fire`, as the specials), or when reversing?
10. **After a swoop** the head comes back blended (as the boss's) for up to ~11 updates instead of snapping to the
    hover: keep, or snap (one line: `portCarDtPlayerPlace(h, head, 1)`)?
11. **Projectile radius:** his missiles test radius 15 (a player's) where the boss's test 8: keep (retail's rule for
    player missiles)?
12. **Carousel:** the port draws him (2.2), or hands the game a model node (`carsel_model`) it textures in a bank?
13. ~~**After a won round**~~ **answered** (request 41 item A, section 9): a player's Dark Tooth is treated like
    every other car (no damage after the round is won); the computer boss keeps retail's exception.

## 9. After a won round (PC port request 41 item A, 2026-10-16)

**Retail (CONFIRMED).** `carApplyDamage` (800E5628, `car_damage.c`) returns before any damage when the target is not
active or when the round is won, except for a car of type 0xF0:
`if (!active || (roundIsWon() && car_type != 0xF0)) return;` (`car_damage.c:208` on this branch, the `#else`).
The exception exists for the boss: after the tournament's level-8 round is won, the boss must still take the hits
that finish its stages. With `TM2_PORT_DT_PLAYER` a player's car can be 0xF0 too, so retail's rule let him be hurt,
even wrecked (no life lost: `carDestroyed` returns for a human after the win), while the end delay runs (question 13).

**The user's decision:** a player's Dark Tooth is treated like everyone else; the computer boss keeps retail.

**Built (under `TM2_PORT_DT_PLAYER`, no bit: a player's Dark Tooth exists only with this define, and the rule is the
user's default):** the test becomes `roundIsWon() && (car_type != 0xF0 || human)` (`car_damage.c:206`), `human` being
the target's side as `carFindById` returned it. So after a won round:

| target | retail | with `TM2_PORT_DT_PLAYER` |
|---|---|---|
| any car but 0xF0 | no damage | no damage (unchanged) |
| the computer boss (AI 0xF0, level 8 or `portCarOptions` bit 6) | takes damage | takes damage (unchanged) |
| a player's Dark Tooth (human 0xF0) | takes damage | **no damage**, like every other car |

The flash (`lightFlickerStart`) comes after the test, so a hit after the win shows nothing on him, as on any car.
No `rand()` changes beyond the hits that no longer happen (a hit >= 8 on him after the win drew the spin roll).
`carApplyDamage` is now one of the functions the define changes (`tools/check_car_groups.py` `EXPECTED_DT_PLAYER`,
check 14's `--expect-functions`). Without the define the function is retail's (the retail builds are unchanged).

## 10. Request 42 (2026-10-16): the ordinary Dark Tooth, the name labels, the carousel for mod cars

The full answers are in [`CUSTOM_TOURNAMENT_ANSWERS.md`](CUSTOM_TOURNAMENT_ANSWERS.md) 1 and 5; this section keeps what
concerns Dark Tooth and car select.

### 10.1 The ordinary Dark Tooth (`portCarOptions` bit 10, `TM2_PORT_DARK_TOOTH`)

What bit 6 turns on, piece by piece (the rows "missile wobble", "head" and "stage 2" of the request): phase 1 = the
boss update on every arena (missiles, the swoop, his head placed over the truck, no machine guns) and the missile
wobble, plus CLUT rows 240..255 for room; boss only = the per-car stage loop / stage 2 and wreck 4 after stage 2;
level-8 rules = Hong Kong's jump zones, warps and boss markers (bit 6 turns them off elsewhere). The head over the
truck is not a piece of its own: it is the hover branch of `aiDarkToothBossUpdate`, whose only stage read is the
hover height (-24 in stage 2, else 240).

Bit 10 (0x400, read live) makes every **computer** Dark Tooth ordinary unless the port marks his car slot as the boss
(`portCarDarkToothRole(slot, 1)`, `portCarDtBossMask`): phase 1 on every arena, no stage loop, no stage 2, no level-8
tail (no retail stage 2 for an ordinary `aiCars[0]`, no Hong Kong route patch, jump zones, warps or markers), wreck
state 4 at his death with his head hidden, and the user's "treat them all the same" (request 41 item A): no damage
after a won round and the 120-tick wreck timer of every car (no 360, no self-hit). A player's Dark Tooth is never
ordinary (this document's rules). Game functions: `aiWeaponFireDecide`, `aiCarsCountRemaining`, `roundStart`,
`carContactApplyObject`, `aiDarkToothJumpSelect`, `aiFollowRouteTarget`, `weaponProjectileUpdate`, `carApplyDamage`,
`carDestroyed`; port: `portCarDarkToothOrdinary`, `portCarDarkToothRole`, `portCarDarkToothHeadShow`, and
`portCarDarkToothWreck`, `portCarDarkToothStages`, `portCarClutsReset` changed. **Tournament:** leave bit 10 clear there,
or mark `aiCars[0]`'s slot as the boss; else the level-8 boss is ordinary too (no "But wait...").

### 10.2 The name labels (a bug of request 36, fixed)

`menuCopyChoiceRun` (8013724C, `menu_render_dispatch.c:16`) copies the carousel's choice into CARSEL's name labels 9006
(0x800347EC) and 9007 (0x80034C94) every frame without a bound. Both have **14** children (name images), so with bit 8
and Dark Tooth highlighted (index 14) the label walk read the word after the child table as a child pointer
(0x0000FF00 for 9006, 0x2330FF00 for 9007) and walked garbage (`label_a` is drawn from the start, `label_b` after a
select). Request 36's `portCarSelExtend` extended only the carousel. Now (`portCarSelExtend(carousel, label_a,
label_b)`, `port_variant_arrays.c:8476`, `screen_car_select.c:376`) the carousel and both labels get copies with the same
count (`portCarSelCopy`); the labels' extra children are the empty node, and the port draws his name as before. The
nodes moved from `portDtPlayer.node` / `blank` (kept, unused, for the layout) to `portCarSel` (`car_numbers.h`).
`portDtPlayer.carsel_count` is still written. Related: `pauseMenu` showed a password for index 14 (retail encodes
16 * (14 + 4), past the 4-bit field); with `TM2_PORT_DT_PLAYER` it shows none for any car past 11
(`screen_pause_menu.c:592`, `TM2_PAUSE_PASSWORD_CAR`). His ending is now played by `portCarEnding`.

### 10.3 Drawing a car's in-game model on the carousel (mod cars, Dark Tooth, Sweet Tooth in the AI select)

The renderer has nothing against it (`CUSTOM_TOURNAMENT_ANSWERS.md` 5). The install must not use the match's loaders.
The recipe for the port (one car at a time, loaded when the highlight changes; the C gives the hook
`portCarSelDraw` and the swap `portCarSelSetChild`):

1. `DrawSync(0)` (the previous frame may still texture from the spot).
2. Copy the car's DMD (a fresh copy each time: relocation marks nodes 0xB1) into free RAM: the shell's
   0x80034DD0..0x80048270 (78,000 bytes after CARSEL.DMD; the info screen's picture buffer starts there), or port RAM.
3. Upload each TIM of its TMS with `LoadImage` at the port's own spots: the free 128 x 256 block at (832,256) (two
   texture-page columns; an image must not cross a 64-unit page column) and the CLUT rows 40..63 at x 768..1023. Do
   **not** use `vramPlaceTexture` / `bitmapParseContainer(tms, n)` / `bitmapInit` in the shell (stale usage bitmap and
   slot table, records appended into an OT, the port's bank and CLUT-row latches reset).
4. Fill a private slot table `{GetTPage(mode, 0, x, y), x, y, clut_x, clut_y}` per image; save `vramTexSlots` and
   `vramTexSlotGroup`, point them at it, call `modelRelocate(dmd, dmd + 0x7FFE63C0)` (as `assetInstallImage`, but
   without `dbscanHierarchy` / `worldLoadObject`, which register match slots, keys and coordinate systems), restore
   both. With texture banks, put the bank bits in the private `page` values (`portTexBanks.live` is stale in the
   shell).
5. Find the root in the PSXD header's root list (+0x18 count, +0x1C...; node id 0x10..0xF0 at node +2; Minion's file
   has two roots) and `portCarSelSetChild(index, root)` (optionally under a port type 4 / 12 node for the framing:
   Dark Tooth's truck is about twice a carousel car).
6. When the highlight leaves, `portCarSelSetChild(index, 0)`; the next load reuses the same RAM and VRAM. Nothing to
   free: no registry was touched.

**Built (request 71, 2026-10-26):** the recipe is now one helper, `portCarSelLoad(k, index, dmd, dmd_len, tms, tms_len)`
(4 load slots, one per split-screen player; with texture banks each in bank 124 + k, so Dark Tooth's BB.TMS fits;
without, slot 0 in the free block above): `CAR_PROFILES.md` 5-7.

Every retail car's TMS fits the free block (at most 14,592 + 832 halfwords); Dark Tooth's BB.TMS (79,616) does not:
he needs a PC-side bank (2.2). In the AI select (Sweet Tooth, `CUSTOM_TOURNAMENT_ANSWERS.md` 2) the free VRAM differs
(AICARSEL.TMS's own layout): the port must pick its spots there from AICARSEL's rectangles.

## 11. Request 46 (2026-10-18): the head of a Dark Tooth nobody drives

`PLAYTEST_ANSWERS.md` 7, `CAR_GROUPS.md` 19. A computer Dark Tooth that is loaded but not driven (parked, a spare,
vacant, or out of play: dead in wreck state 4, as the port's Survival keeps the later waves) has his head hidden each
frame (`portCarDarkToothParked`, `TM2_PORT_DARK_TOOTH`): not drawn and out of the hit pass. When a computer record
drives the car again (`aiCarStart`, `portCarStartAi`, `portCarSetGroup`) the head comes back placed over the truck
(240 up in the car's frame, -24 in stage 2), the swoop cleared. A **player's** Dark Tooth is not touched: his head stays
`portCarDtPlayerHead`'s (shown and hidden each update, section 1). Retail: the head is made shown at the world origin
(`weaponAssetBind` 0xF9) and only the boss update moves it; retail loads BB.DMD only for level 8's boss, who drives
from the first frame, so it is never left standing there.

## 12. Request 118 (2026-11-18): a player's Dark Tooth in his second phase (built; `portDtPlayer.phase2`)

The port's switch is `portDtPlayer.phase2` (+0x28, taken from the unused bytes, so the struct keeps its 148 bytes).
Bit h set means human h's Dark Tooth plays his second phase from every start, restart and respawn. It is game state:
the port sets it with the cars, the same on every machine. With it clear, everything is request 36's phase 1.

1. **The start** (`portCarDtPlayerStart`). It runs from `humanCarPlaceAtStart` (every start, restart and respawn)
   and from the swap's `portCarApplyType`, right after the stats load and before a human's +20.
   - It sets his key's stage flag (`aiBossDefeatedFlag`, selected) and applies stage 2's stats, the boss's
     `carStatsDarkToothStage2`.
   - **Numbers:** 110 HP, so **130** in play with retail's +20 for humans. **Mass 50**, so **no x2**: that doubling is
     for mass 100 only. The knock-back he takes is half, as for any car of mass 51 or more.
   - **The chassis:** width 50, axles +-40, ride height 50, roof 60. `carBuildCollisionBox` follows in both callers.
   - `portCarDtPlayerHead` sets the flag again every update from the switch, so an `arenaSetup` clear cannot drop it.
2. **Car-car collisions.** Retail hides the boss's truck by its object's word +8, which also takes the car out of the
   hit pass. Nobody can bump the stage-2 boss; only his own probe points hit other cars.
   - **Here the car object stays drawn-enabled**, and its model (+0x11C) becomes `portDtPhase2Tree`: a kind-1
     sphere (id 240, as the truck's root) over one kind-8 hit box. A hit-only node draws nothing.
   - **The box** is the head's own box from BB.DMD (`80028460`) at the stage-2 hover: x -55..56, y -84..75,
     z -24..117 in the car's frame.
   - **The head object leaves the hit pass** (+0xC 0), so other cars meet the car's box, with car-car responses on
     both sides. In retail they hit the head as a plain object.
   - **The truck's model is kept per car slot** (`portDtPhase2Truck`) and put back when the phase is off, or when a
     computer driver takes the car in a swap.
   - **No shadow**, as the stage-2 boss: the blob is a node of the truck.
   - **Weapon hits are unchanged**: a cube around the car, with the +25 every 0xF0 gets, as the stage-2 boss.
3. **His weapons:**
   - **Missiles:** as request 36's, from the boss's front points, as the boss fires them in stage 2.
   - **The swoop:** as the boss's, by the user's choice (2026-11-18). The head leaves the car, and while it is away
     nothing is drawn where the car is, exactly as the stage-2 boss.
4. **His death:** already right once the flag is 1. `carApplyDamage`'s Dark Tooth block takes the stage-2 path: the
   head's debris (10 pieces), the head hidden. `portCarDarkToothWreck` gives a human wreck state 4: no repeated
   bursts, and no stage change after it (the stage loops are for computer cars only).
5. **The camera:** in phase 2, `viewCameraOffsetsInit` keeps a car's offsets, not request 36's truck offsets.
   - `roof_height` comes from stage 2's stats (60).
   - The hit size +25 stays, as noted in 2.
6. **Car select:** `portCarSelLoad(k, 14 | TM2_CARSEL_DT_PHASE2, ...)` (0x10000, `car_numbers.h`) shows the head
   alone. It is the request-109 transform node without the truck, at `TM2_PORT_CARSEL_PHASE2_Z` (-90 from the
   truck's origin: 26 over the ground, as in a match). `check_custom.py` check 10 loads BB that way too.
7. **Level 8:** the human's key is 0xF0 and the boss's an overflow key (7.2), so their stage flags stay apart. The
   boss's stage 2 and the reveal run on `aiCars[0]`'s key as before; his own flag is the switch's.

Checked: `check_custom.py` check 29 (`portCarDtPlayerStart` in LinkMips: the flag, the stats, the tree's sphere and
six planes against the head's box, the truck back with the bit clear, other types untouched), and check 10's phase-2
carousel load. **Not tested in game.**

