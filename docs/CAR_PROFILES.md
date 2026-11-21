# Mod characters: their own numbers, and car select's turning car

Answers the PC port's requests 70 and 71 (port repo `docs/DECOMP_REQUESTS.md`, asked 2026-10-26):

- **Request 70:** a mod character's own stats, size, special numbers, radar colour and target name. This is
  `PER_CAR_PLAN.md` E.2's `portCarProfile`, built as `TM2_PORT_CAR_PROFILE` (sections 1-4).
- **Request 71:** the in-game model of a mod car (or Dark Tooth, or Sweet Tooth in the AI select) on car select's
  carousel. This is `DARK_TOOTH_PLAYER.md` 10.3's recipe as one helper, `portCarSelLoad` (sections 5-7).

Both build on request 42 (`CUSTOM_TOURNAMENT_ANSWERS.md`, `car_numbers.h`): mod m has the car number 0x20 + m, is
described in `portModCars[m]`, and registers in a match as a duplicate of its base car.

**Tags:** **CONFIRMED** = seen in code or data. **LIKELY** = follows from the code, not seen running. Static proofs
only: the port variant builds and proofs of section 8. Nothing was run in the game.

**Request 73** (the special itself: a template of another driver or a pickup weapon, and its numbers) is in
`SPECIAL_TEMPLATES.md`.

## 1. Request 70 in short

- **Define:** `--port-define TM2_PORT_CAR_PROFILE`. It needs `TM2_PORT_CAR_NUMBERS` and `TM2_PORT_CARS`, because the
  car number of each load group comes from `portCarLoadGroup`. Without the define, every changed line is the
  retail C.
- **Storage:** `portCarProfile[128]` (`.port_bss`, 0xA0 bytes each, indexed by mod m). The port fills it before each
  load, from the mod's `[character]` section, the same on every machine. `valid` 0 means the base car's numbers:
  every hook then runs retail's code.
- **How a record finds its profile:**
  1. The record's car slot (+0x004 human, +0x006 AI) gives a load group (`portCarGroups.slot_group`).
  2. The load group gives the car number it was loaded with (`portCarNumberOfGroup`).
  3. A used mod's number gives `portCarProfile[number - 0x20]`, if `valid` is set.

  This holds for humans and computer cars alike, and after a car swap (`portCarApplyType` reloads the stats of the
  slot's car).
- **What a profile replaces** (every field it leaves out stays the base's):
  1. **Stats:** up to 24 stats-block fields (section 2). This covers top speed, acceleration, braking, turning,
     mass, armor and the AI's numbers. The hit box follows from the same fields: `carBuildCollisionBox` builds it
     from the track width, the axles, the ride height and the roof height.
  2. **The step height:** `carWheelsSetStepHeight`.
  3. **The camera offsets:** `viewCameraOffsetsInit`, any of its six vectors.
  4. **The special's numbers:** `weaponBlockInit`: recharge period, pickup capacity and max specials.
  5. **The default top speed:** the speed the AI's cruise levels are cut from, and the speed given back after a
     freeze or the shield cheat (`carDefaultTopSpeed` callers). It is the profile's top-speed entry.
  6. **The radar blip's colour:** `hudRadarPlotCar`.
  7. **The target panel's name:** `hudDrawTargetPanel`.
- **What stays the base type's:**
  - The special's behaviour: its code, the AI's special code, and the Dark Tooth and Minion rules (E.2's last
    point).
  - The AI's weight tables (`aiCarSetupProfile`).
  - The engine sound.
  - `carHasMeleeSpecial`.

  Section 4 lists these.

## 2. The profile (`car_numbers.h` struct `TM2PortCarProfile`, 0xA0 bytes)

| Offset | Field | Meaning |
|---|---|---|
| +0x00 | `u8 valid` | 1 = consult this profile; 0 = the base car's everything |
| +0x01 | `u8 stat_count` | entries of `stat[]` used (at most 24) |
| +0x02 | `u8 flags` | bit 0 `TM2_PORT_PROFILE_STEP`, bit 1 `_SPECIAL`, bit 2 `_RADAR` |
| +0x03 | `u8 cam_mask` | bit v: `cam[v]` replaces that camera offset |
| +0x04 | `s16 step` | the highest climbable step (flag bit 0; > 0). Retail: 0x28 Mr. Grimm .. 0xA0 Dark Tooth |
| +0x06 | `s16 recharge` | frames per special (flag bit 1; > 0, else the base's). Retail 360..1600 (`CAR_STATS.md` 3) |
| +0x08 | `s16 capacity` | pickup capacity (flag bit 1; > 0). 24 for every retail car |
| +0x0A | `s8 max_specials` | specials held at most (flag bit 1; > 0). Retail 3..5 |
| +0x0B | `u8 radar[3]` | the blip's r, g, b (flag bit 2) |
| +0x10 | `const char *name` | the target panel's name (0 = `carTypeName` of the base). The HUD's font, as for the retail names |
| +0x14 | `s16 cam[6][3]` | camera offsets in car space (x, y, z), by `cam_mask` bit: 0 look-at point (+0x080), 1 chase / views 2 and 7 (+0x044), 2 view 3 (+0x050), 3 the other full-screen views (+0x05C), 4 split mode 1 (+0x068), 5 split mode 2 (+0x074); the human record offsets in brackets. Retail values: `viewCameraOffsetsInit` (`ai_car_start.c`) |
| +0x38 | `int reserved[2]` | 0 |
| +0x40 | `stat[24]` | `{u8 offset, u8 width, s16 value}`: one stats-block field each, applied in order |

### 2.1 The stats fields a profile may set

`stat[i]` writes `value` into the stats block (human +0x14C, AI +0x4FC) at `offset`, with the field's own width:
1 byte, 2 bytes, or 4 bytes (sign-extended). It is applied in `carStatsLoad`, after the base type's init function
and before the derived fields. An entry whose `(offset, width)` is not in this list is skipped. Meanings and the
retail values per car are in `CAR_STATS.md` 2.

| Group | Fields: offset (width) |
|---|---|
| Handling | top speed +0x52 (2), acceleration +0xAC (4), brake +0x5C (2), turning speed +0x4C (2), low-speed steering +0x6C (2), high-speed steering +0x5E (2), steering lock +0x68 (2), handbrake lock +0x6E (2), slide speed +0x56 (2) |
| Body | mass +0x7A (2), max HP / armor +0x3A (2), mounts cars +0x02 (1), unknown +0x0E (1), pitch travel +0x34 (2), body lean +0x6A (2), rollover threshold +0x3E (2) |
| Size (the hit box) | track width +0x48 (2), front axle +0x4E (2), rear axle +0x50 (2), ride height +0x42 (2), roof height +0x46 (2) |
| Mounts | gun x, y, z +0xD8, +0xDA, +0xDC (2 each); launch height +0x44 (2); centre mount x +0xE0 (2); second mount y, z +0xE2, +0xE4 (2 each) |
| AI | chase patience +0x24 (2), turning range +0x2A (2), hunt at full / zero HP +0x7C / +0x7E (2), attack at full / zero HP +0x80 / +0x82 (2), hunt / attack / flee times +0x86 / +0x88 / +0x8A (2), retaliation % +0x8E (2), pass chance +0x90 (2) |

Notes:
- **Derived fields are computed from the overrides** by `carStatsLoad` (`CAR_STATS.md` 2):
  - the wheelbase (+0x4A) = |front axle| + |rear axle|;
  - every speed of the top speed (`carStatsSetTopSpeed`): reverse, 2/3 and the scaled speeds;
  - the retaliation budget (+0x8C) = max HP × retaliation % / 100;
  - +0x84 from the hunt chance at full HP.

  So a mod sets the base numbers, never the derived ones. The per-life fields (+0x00, +0x14, +0x3C shield, +0xA4
  HP, +0xA8, the timers `carStatsLoad` clears) are refused.
- **Max HP:** a human gets +20 on top, as for every retail car (`humanCarPlaceAtStart`). So +0x3A is the table's
  value, the one `CAR_STATS.md` lists.
- **Rollover threshold:** retail's +0x3E equals the track width, except Sweet Tooth (2/3) and Mr. Grimm (3×). A wider
  mod should set both.
- **Ride height** also places the car on the ground at a start: the AI's z = node z + ride height, a human's from
  its start.
- **"Wheelbase"** in MODDING.md 4.2 means the two axle offsets: the wheelbase field itself is derived.
- **Limits:** the C takes the port's values as they are. MODDING.md 4.2 says the port keeps them within limits. These
  values are known to break things:
  - a top speed of 0, or one far above the retail range (92..141, `CAR_STATS.md` 2): the turbo's cap is 140..180;
  - a mass of 100, which is Dark Tooth's: a human with it does double damage (`carApplyDamage`);
  - a mass above 100: the spin-out formula `(100 - mass)` goes negative.

### 2.2 The port's side

- Fill `portCarProfile[m]` from `mods/characters/<id>/mod.ini [character]` before the load whose settings hold
  number 0x20 + m. Do it on every machine alike: the profile is match state (section 3).
- Clear `valid` for skins: a skin keeps its base's numbers (MODDING.md 5).
- `portCarProfileOfSlot(slot)` is also the port's API. It returns the profile a car slot uses (0 = the base's), for
  the port's kill feed, results and Car Info.

## 3. Request 70: what changed, and online

| VA | Function | Unit | Change under the define |
|---|---|---|---|
| 800FE15C | `carStatsLoad` | `car_stats.c` | `portCarProfileStats(s)` after the type's init function, before the derived fields |
| 800FF638 | `carWheelsSetStepHeight` | `ai_car_start.c` | `portCarProfileStep(w)` at the end |
| 800FF764 | `viewCameraOffsetsInit` | `ai_car_start.c` | `portCarProfileCamera(human)` at the end (after a player's Dark Tooth's) |
| 800FDCAC | `weaponBlockInit` | `ai_route_setup.c` | `portCarProfileWeapons(car, rec, alt)` after the type's numbers and a player's Dark Tooth's, before a borrowed special's (`TM2_PORT_SPECIAL_AS`) |
| 800F68D8 | `aiAdvanceRouteNode` | `ai_route_nodes.c` | the five cruise levels from `TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, type)` |
| 800F6F6C | `aiSetSegmentSpeed` | `car_tumble.c` | the four cruise speeds likewise |
| 800EC180 | `aiInputFinalize` | `ai_steps.c` | the fire test's top speed likewise |
| 800DC88C | `humanCarTick` | `car_tick.c` | the shield cheat's speed restore likewise |
| 800FC928 | `hudRadarPlotCar` | `hud_radar.c` | a type with a slot in bits 16..23 asks `portCarProfileRadar`; a colour key draws the profile's colour |
| 8011A700 | `worldUpdate` | `world_update.c` | the blips' type carries the car's slot + 1 in bits 16..23 (`WORLD_RADAR_AI` / `WORLD_RADAR_HUMAN`: no call and no local, so its frame stays retail's) |
| 800FD0D0 | `hudDrawTargetPanel` | `car_destroyed.c` | the name from `portCarProfileName(slot, type)` |

**Port functions** (`.port_text`, `port_variant_arrays.c`):
- `portCarProfileOfSlot`
- `portCarProfileStats`, `portCarProfileStep`, `portCarProfileCamera`, `portCarProfileWeapons`
- `portCarProfileTopSpeed`, `portCarProfileRadar`, `portCarProfileName`

None of them calls `rand()` or keeps state.

**Not changed:**
- `carBuildCollisionBox`: it reads the stats, so the hit box follows them.
- `carDefaultTopSpeed` itself: its callers pass the slot.
- `portCarApplyType`: the swap reloads through the hooks above.

**Online (rollback):** `portCarProfile` is match state. `rollback_regions.tsv` marks it snapshot yes and hash yes: it
is read in the step (`carStatsLoad` at a respawn, the AI's speeds). The port writes it before the load, the same on
every machine, from the server's mod list. A machine with another profile shows up in the hash at once. Every hook
reads only it and the slot tables, which are already state.

## 4. Request 70: limits

- **The special's behaviour, its damage and its speed stay the base's.** `weaponFireSpecial` and the special
  updates switch on the base type. Per-template damage and speed (MODDING.md 4.3) would need a table per special
  template. That is a later request, as E.2 and the port's MODDING.md 4.3 say.
- **The AI reads some numbers by type, not from the stats:**
  - the weights `ai*Weights9/10` and the fire chance (`aiCarSetupProfile` 800D3C7C);
  - `carHasMeleeSpecial`;
  - the Dark Tooth gates (type 0xF0);
  - the route code's `ai->type != 0xF0` tests.

  A mod drives with its base's AI personality, but its stats-block AI numbers (section 2.1, "AI") are its own.
- **Engine sound:** by the base type (`CAR_GROUPS.md` 12). The port's own sound bank (MODDING.md 11) would hook
  `portCarEngineUpdate`'s sound choice later.
- **The target panel's name** is drawn with the HUD font (`fontSpritePrintCenteredXY`, font 2). It has the
  characters of the retail names; use upper case letters, digits and spaces.
- **The kill feed and results** are the port's. `carDisplayName` already gives the mod's name (request 42).

## 5. Request 71 in short: `portCarSelLoad`

```c
int  portCarSelLoad(int k, int index, const void *dmd, int dmd_len, const void *tms, int tms_len);
void portCarSelUnload(int k);
```

- **What it does:** it installs one car's model files in load slot k and makes the model the extended carousel's
  child `index`. The child is 14 for Dark Tooth and 0x20 + m for mod m in car select, 12 for Sweet Tooth in the AI
  select. Slot k's previous model is freed first. The draw (`sceneObjDraw` → `renderDrawModel`) then shows it as a
  retail child: the carousel's position, spin (`o1176`), slides, R1 drop, camera, three lights, OT and packets.
- **When to call it:** between frames, or from the port's wrap of `portCarSelDraw` (called before each draw of the
  extended carousel), while car select or the AI select runs. That is after `portCarSelExtend` / `portAiSelExtend`,
  which reset every slot at each screen's start.
- **Its arguments:**
  - `dmd` / `tms` are the car's files in guest RAM: the mod's `carselect.dmd` / `.tms` (MODDING.md 4.1), or its
    `car.dmd` / `car.tms`, Dark Tooth's `BB.DMD` / `BB.TMS`, or a retail car's files.
  - The DMD is copied into the slot (`portCarSelLoads[k].dmd`, 128 KB). The port may also write it there directly
    and pass that address.
  - The TMS is read in place: it can be a temporary buffer.
- **Return value:** 1, or an error, after which slot k is empty and the child is the empty node. The errors:

  | Error | Value | Cause |
  |---|---|---|
  | `TM2_CARSEL_LOAD_SLOT` | -1 | bad k |
  | `_INDEX` | -2 | not an extra child of this screen's carousel |
  | `_DMD` | -3 | not PSXD version 0x43, larger than 128 KB, or no root |
  | `_TMS` | -4 | not PSXT, cut short, more than 32 images, or a TIM that is not 4- or 8-bit with a CLUT or 15-bit without one |
  | `_VRAM` | -5 | the images do not fit |
- **`portCarSelUnload(k)`** puts slot k's child back to the empty node, unless another slot's model was set there
  since.
- **Fields the port can read:** `portCarSelLoads[k].index` is the child slot k's model is on (-1 none) and `.model`
  the node it set.

**Split screen (the request's question): 4 slots, each its own.** `TM2_PORT_CARSEL_LOADS` = 4: one slot per player, so
4 different mod cars can be drawn at once.
- **With `TM2_PORT_TEXTURE_BANKS`:** each slot has its own VRAM bank (124 + k) and its own DMD copy. Use slot p for
  player p.
- **Two players on the same mod:** each gets the same child: load it once and draw both from it. Loading it into
  both slots also works, and the later load is drawn.
- **Without texture banks:** only slot 0 exists (car select's one free VRAM block), so beyond one mod car fall back to
  the base's car.

The port's split-screen draw (`CAR_SELECT_CAR.md` 3) then sets each player's choice byte, or points each player's
engine object at the child, as it does for retail cars.

## 6. Request 71: how it installs a model (`port_variant_arrays.c`)

1. **`DrawSync(0)`:** the previous frame may still sample the slot's VRAM.
2. **The DMD is copied** into the slot. It is a fresh copy each time: relocation marks nodes 0xB1. If the port writes
   into the slot itself, it must rewrite the file before each load.
3. **The TMS's images go to VRAM** at the slot's own spots (`portCarSelTextures`, `portCarSelPlace`). The area:

   | Build | Pixels | CLUTs |
   |---|---|---|
   | `TM2_PORT_TEXTURE_BANKS` | bank 124 + k, x 0..1023, y 0..479 | rows 480..511 |
   | without banks | slot 0 only: car select's free block at x 832..959, y 256..511 (`DARK_TOOTH_PLAYER.md` 10.3) | rows 40..63, x 768..1023 |

   - **Placement:** each image goes to the first free spot on a grid of its own size (as `vramPlaceTexture` scans).
     The image stays inside one texture page as `modelRemapTextures` addresses it: a row of 256 texels is 64 VRAM
     units at 4-bit, 128 at 8-bit and 256 at 15-bit, and `(y & 0xFF) + h <= 256`.
   - **CLUTs:** each image's CLUT is its first row, 16 colours (4-bit) or 256 (8-bit), as retail uploads. The CLUTs
     are packed along the CLUT rows.
   - **The uploads:** banked, they go through `portTexLoadImage`: the bank word is set around `LoadImage`
     (`TEXTURE_BANKS.md` 6.5), and the port's GPU writes them into that bank.
4. **A private texture slot table:** `portCarSelLoads[k].slots`, `{page bits | bank << 9, x, y, CLUT x, CLUT y}` per
   image.
5. **Relocation:** `vramTexSlots` points at the private table and `vramTexSlotGroup` is set to 0. With banks,
   `portTexBanks.live` is set to 1, because `modelRemapTextures` writes tpage bits 9..15 only while banks are live.
   Then every root in the PSXD root list is relocated (`modelRelocate`, delta = buffer + 0x7FFE63C0, as
   `assetInstallImage`), and all three are restored.
   - This leaves out `renderSetWorldShift`, `dbscanHierarchy` and `worldLoadObject`. Those register match slots,
     keys, coordinate systems and CLUT animations. The world shift of every car file and CARSEL is 3 (CONFIRMED,
     disc).
   - **Nothing to free:** no registry, VRAM bitmap or load group is touched.
6. **The child:** the car roots (ids 0x10..0xF0, low nibble 0) become the child.
   - Every retail car file has one (CONFIRMED, disc).
   - Dark Tooth's `BB.DMD` has three roots: 240 (the truck), 249 (the head, which only the boss update places over
     the truck) and 2214 (his ricochet bomb's look). **Request 74:** the child is a group of the truck and an offset
     node (opcode 4, after the file in the slot's buffer) holding the head **240 above the truck in the truck's
     frame** (`TM2_PORT_CARSEL_HEAD_Z`: the boss's hover height, `ai_dark_tooth_boss.c`; the file's units are
     position units, shift 3), so the head turns with the truck under the carousel's spin and lights, without the
     boss's sway. **Request 109:** the node is a transform node (opcode 5) turned a half turn about z: the head's
     face is its -y (the boss's swoop turns it to `ratan2(head - target)`), a truck's front its +y, so request 74's
     unturned head faced backwards and its bulk (its sphere centred 133 along its +y) hung over the truck's front. The
     height stays 240: node offsets count as the vertices do (the file's own wheel nodes sit at x +-72, its truck's
     vertices at z down to -155 for a ride height of 116); the shift scales only the LOD and far tests. In a match the
     head has no fixed facing: the boss update turns it 150 a frame and rocks it +-683, and the turned head is how it
     sits facing forward. Each load slot has its own (split-screen car select: every slot showing him has his head). No other
     car file has a separate part (every other retail file is one root; Sweet Tooth's in the AI select too). The
     group's sphere is the truck's and the group is never culled, so the head is not clipped by it.
   - Several car roots go under a group node (type 1, the first root's sphere, not culled).
   - A file with no car root: its first root.

**The carousel's framing (LIKELY):**
- CARSEL's children are type-1 groups of the cars' own size: the camera is 560 ahead and 200 above the car.
- An in-game model draws at its in-game size. That is the same size for the 14 cars, about twice it for Dark Tooth's
  truck.
- The C does not scale it. A type 12 node changes only the walker's units (the translation and the OT shift, not
  the vertices). A type 5 matrix would scale the light matrix too, so a scaled car would be lit darker.
- If Dark Tooth needs to be smaller, the port can draw his child from a camera further back, or ask for a scaled
  carousel model.

## 7. Request 71: the storage and the port's addresses

`portCarSelLoads[4]` (`.port_bss`, `car_numbers.h` struct `TM2PortCarSelLoad`, 0x203C0 bytes each):

| Offset | Field | Contents |
|---|---|---|
| +0x000 | `index` | the child it is set on, -1 none |
| +0x004 | `bank` | its VRAM bank (124 + k), 0 = the PS1's VRAM |
| +0x008 | `images` | images placed |
| +0x00C | `roots` | car roots in the child |
| +0x010 | `slots[32][5]` | the private texture slot table |
| +0x290 | `rects[32][4]` | the pixel rectangles taken |
| +0x390 | `group[40]` | the group node over several roots |
| +0x3B8 | `model` | the node set as the child, 0 none |
| +0x3C0 | `dmd[0x20000]` | the DMD copy |

- **Online:** shell presentation only, so `rollback_regions.tsv` marks it snapshot and hash no.
- **Defined in every variant**, after `portHumanControl`, followed by `portCarProfile`.
- **Addresses:** read them from each build's `symbols.tsv` and `functions.tsv` (kind `port`). They move with the
  defines.

## 8. Addresses and proofs

**Addresses** in the two links of the proofs. They move with the defines, so read them from each build's
`symbols.tsv`:

| Symbol | Every port define + mods (`r70_allh16_P4`) | The port's set without mods (`r70_port_P4`) |
|---|---|---|
| `portCarSelLoads` (storage) | 0x808CEA90 | 0x808CEA90 |
| `portCarProfile` (storage) | 0x8094F990 | 0x8094F990 |
| `portCarSelLoad` | 0x8096C930 | 0x8096B894 |
| `portCarSelUnload` | 0x8096C894 | 0x8096B7F8 |
| `portCarProfileOfSlot` | 0x8096D528 | (needs `TM2_PORT_CAR_PROFILE`) |

With every define the port sections now end at 0x80972000, well below `PORT_RAM_END` (0x80C00000). The new storage takes 0x85F00
bytes: 528 KB of car select models and 20 KB of profiles.

**Proofs (2026-10-27).** Nothing was run in the game.
- **Retail builds:** unchanged (source-only `d755c86e…`, sdk-lib `f557c1e3…`, `BUILD_MANIFEST.json` untouched). Every
  changed game line is under its define.
- **Port links:** four links, each P4 and P2 (`--port-limits 2`). Every one passes `verify_reloc` against E0.
  1. `TM2_PORT_ROLLBACK` alone.
  2. The port's usual define set without mods: `TM2_PORT_DT_PLAYER` and texture banks.
  3. Every define with `TM2_PORT_CAR_NUMBERS` and `TM2_PORT_CAR_PROFILE`.
  4. A set without texture banks (car groups, cars 16, specials, Dark Tooth, the player's Dark Tooth, numbers,
     profile).
- **`tools/check_all.py`:** 32 checks, 0 failed, including `check_rollback` (the new rows), `check_humans` and
  `check_custom` checks 10 and 11:
  - **Check 10:** the real `portCarSelLoad`, `modelRelocate` and `modelRemapTextures` run on the disc's SP, BB
    (31 images), MN and RK. That is 2,521 textured primitives banked and 1,006 in the free block, each with its
    image's page bits, its bank bits, its CLUT and its UVs.
  - **Check 11:** the profile hooks on a made-up mod on Spectre, with and without a valid profile.
- **Negative control:** a link without the defines fails checks 10 and 11.
