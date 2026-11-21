# The Antarctica Santa: adding the PC's pedestrian 516 #2 to the PS1 level

Answers item 11 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(restore the Antarctica Santa from the PC data). Follows `GAME_EVENTS.md`
section 14.2. Static analysis only: the C in `src/semantic` and the pure-C
references in `reference/`, plus small read-only scratch parsers run on the
USA disc (`cdimage/`, `DMD\GLACIER.DMD`, `TMS\GLACIER.TMS` and others) and on
the PC files (`LEVELDB\GLACIER.DPC`, `GLACIER.TPC`). Nothing was run in an
emulator and no game data is copied here: only layouts, offsets and sizes.
Addresses are retail (USA).

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code or data, not traced to the end or not seen running.

## Short answer

- **He is a sprite, not a 3D model.** PC 516 #2 is a type 9 "select" node
  with the same three steps as every pedestrian: an empty group (hidden), a
  2-frame standing loop and a 3-frame hit animation. Each frame is one mesh
  node with **one camera-facing billboard primitive** (prim type 1, the
  "beam" of the PS1 emitter), 68 x 66 world units, textured with a 64 x 64
  8-bit image. The frames use PC textures 296, 297 and 298 (PC texture ids
  0x4128, 0x4129, 0x412A); nothing in the PS1 files matches them. CONFIRMED.
- **The node format is the PS1's; the primitive format is not.** Select,
  group, animation and mesh nodes are byte-compatible. The PC primitive is
  48 bytes (a texture id and 16-bit UVs instead of tpage/CLUT); the PS1
  record is 36 bytes. Five primitives must be rewritten (section 2.4).
  CONFIRMED.
- **The .TPC is not 16-bit colour.** Its images are 8-bit indexed, like the
  PS1's; only the 256-entry palettes are 16-bit, in RGB555 (red in the high
  bits), where the PS1 uses BGR555. Swap the red and blue fields of each
  palette entry. CONFIRMED.
- **Loading:** the level DMD is read to 0x80019C40 by
  `sub_8012BAFC` (sessionLoadArenaAndCars); its pointers are absolute
  (built for 0x80019C40) and are never relocated. The car DMDs are packed
  after it and relocated. With the 8 cars of the Antarctica tournament there
  are about 140 KB free before the EXE (0x800CDC54). Santa needs 592 bytes.
- **Registering him:** any walk of the node by `sub_8014ABF8`
  (dbscanHierarchy) with `sub_800D1DB0` (worldLoadObject) as the callback
  calls `sub_80125888` (targetActorSpawn)(516, 2, node). The PS1 code then
  places him on one of the four unused level 6 rows and runs him like every
  other pedestrian. Nothing in the EXE needs to change. CONFIRMED.
- **Two ways to add him** (section 1.6). **A (recommended):** ship him as a
  small relocatable asset pair, like a car: a 3-image TMS container parsed
  with group n, and a one-root PSXD image installed with
  `sub_80120EF4` (assetInstallImage)(image, n). The game relocates the
  pointers, finds VRAM for the images and patches tpage/CLUT itself.
  **B:** append the nodes after the level DMD with absolute pointers, upload
  the images at a fixed free VRAM spot, and walk the node once.
- **VRAM:** the GLACIER and GLACIER2 textures use x < 420 of the lower half
  and x 640..1023 of the upper half. With 8 cars, x 768..959 of the lower half
  and CLUT rows 101..103 at x 768 stay free. Fixed spot for B: images at
  (896,384), (928,384), (896,448), tpage 0x9E; CLUTs at (768,101..103).
- **Everything else is already in the EXE:** placement, standing loop, being
  run over or shot, the throw (42 updates), settling, respawn after about 300
  updates, sounds 25, 3 and 669, the 10-pedestrian bonus. His kill is
  `sub_80124384` (objRegistrySetStep)(516, 2, 2) on level 6.

## 1. How a level's DMD gets into RAM (CONFIRMED)

### 1.1 The chain

| Step | Where | What |
|---|---|---|
| 1 | `sub_8012BAFC` (sessionLoadArenaAndCars) (`shell_main.c`) | `sub_80121550` (bitmapInit): clear the VRAM-usage bitmap, reserve the frame buffers |
| 2 | same | load `TMS\<stem>.TMS` (or `<stem>2.TMS`) to 0x80019C40; `sub_80121A2C` (bitmapParseContainer)(buf, **0**): every image to its own fixed VRAM spot |
| 3 | same | each car's TMS to 0x80019C40; bitmapParseContainer(buf, **n**), n = 1.. (humans, then AIs): images placed dynamically by `sub_801209E8` (vramPlaceTexture) |
| 4 | same | load `DMD\<stem>.DMD` to **0x80019C40**; `size` = file size |
| 5 | `sub_80128CE4` (arenaResetSystems) (`sound_manager_reap.c`) | resets the registries, `sub_80125844` (targetActorsReset), then `sub_800D1C70` (worldLoadStartObjects) (`rt_main.c`) walks the level's roots |
| 6 | sessionLoadArenaAndCars | each car's DMD to 0x80019C40 + off (off starts at the level size and grows by each file's size); `sub_80120EF4` (assetInstallImage)(buf, n) (`asset_header.c`) |
| 7 | same | `sub_801215EC` (bitmapDisableTracking): VRAM bookkeeping ends |

The level walk happens once per arena load. Round restarts do not reload or
re-walk the DMD.

File names: `sub_8012ABD0` (levelFileStem) (`car_parts_spin.c`) maps level 6
to "GLACIER" (`aGlacier` at 80180C3C). `sub_8012A998`
(settingsLevelDmdFileName) and `sub_8012AA5C` (settingsLevelTmsFileName)
(`screen_card_layout.c`) build `DMD\GLACIER.DMD` / `TMS\GLACIER.TMS`, or
the "2" files in modes 2 and 4 on levels 3, 5 and 6. Santa must be added to
both GLACIER and GLACIER2.

`sub_8014DA30` (fileioLoadFileIntoRam) returns the file size, but
`sub_8014D910` (fileioReadFile) reads whole 2048-byte sectors: up to 2047
bytes past the file end are overwritten too. Anything appended after a file
must be written after that file's read.

### 1.2 The level DMD file

| Offset | PS1 `.DMD` | PC `.DPC` |
|---|---|---|
| +0x00 | "DXSP" (u32 0x50535844, 'PSXD') | "DCPM" |
| +0x04 | 0x43 (version) | 0x43 |
| +0x08 | build stamp (GLACIER: 0x3254247B, same word as the TMS header) | stamp |
| +0x0C | 0x80019C40 (the address the pointers were built for) | 0x80019C40 |
| +0x10 | world coordinate shift (3) | 3 |
| +0x14 | pointer to the root table (0x80019C58) | 0x80019C60 |
| +0x18 | **root count** (GLACIER: 11) | two zero words |
| +0x1C | **root pointers**, one per root | (+0x20 count 12, +0x24 roots) |

`worldLoadStartObjects` does not use +0x14: it reads the shift at the fixed
address 0x80019C50 (`sub_8015166C` (renderSetWorldShift)), the count at
0x80019C58 and the roots from 0x80019C5C, and calls
`dbscanHierarchy(root, shift, worldLoadObject)` for each. A zero root stops
the loop. The root table cannot grow in place: the first node starts right
after it.

| | GLACIER (all modes but 2 and 4) | GLACIER2 (modes 2, 4) |
|---|---|---|
| DMD size | 309,540 | 261,124 |
| Ends at | 0x80065564 | 0x80059844 |
| Root holding 514 #1 | root 9 = 0x8006501C (header slot +0x40) | root 9 = 0x800592FC (slot +0x40) |
| TMS size / images | 408,856 / 241 | 405,160 / 239 |

### 1.3 Free RAM

The EXE text starts at 0x800CDC54 (`config/US/exe_header.json`), so the
level and car models share 0x80019C40..0x800CDC54 = 737,300 bytes
(`SECRET_LEVELS.md` section 5). The Antarctica tournament has 8 cars
(`sub_8012AFE8` (tournamentOpponentCount), level 6 = 8, humans included).

| Case | Level | 8 largest regular car DMDs | Free |
|---|---|---|---|
| GLACIER | 309,540 | 285,828 (RK, HS, PV, IR, TH, SP, WH, AX) | **141,932** |
| GLACIER2 | 261,124 | 285,828 | 190,348 |

Even with Dark Tooth's 61,660-byte BB.DMD as one of the cars, over 100 KB
stay free. Santa's nodes take 592 bytes (section 1.6). LIKELY (assumes, as
`SECRET_LEVELS.md` does, that nothing else lives in that range during a
battle).

### 1.4 How a node becomes pedestrian 516 #2

1. `sub_8014ABF8` (dbscanHierarchy) (`engine_clut_anim.c`) walks a tree with
   an explicit stack. For each node with a non-zero id (u16 +2) it calls
   `cb(id, node, pos, parent)` after pushing the node's children. Children
   are pushed in list order and popped last-first.
2. `sub_800D1DB0` (worldLoadObject) (`world_load_object.c`), case 514, 516,
   517: `sub_80123F78` (objRegistryAdd)(node, id) stores the node in
   `D_8018F664` (objRegPedestrians)[u16 +4] (7 slots, so instance 2 fits),
   then `sub_80125888` (targetActorSpawn)(id, u16 +4, node).
3. targetActorSpawn (`target_actor_lifecycle.c`) checks `node[0] == 9` and
   that fewer than 6 pedestrians exist (`D_80180BB8` (targetActorCount)). It
   takes record `D_80190E54` (targetActors)[i], allocates a world object
   (`sub_8014952C` (csAlloc), 128 in the pool), gives it the node as its
   model, object id 6000 + i, kind = 516 (+4), instance = 2 (+6), calls
   `sub_80125A18` (targetActorPlace) and sets step 1 (standing).

So the node needs exactly: byte 0 = 9, u16 +2 = 516, u16 +4 = 2, and three
valid children. On level 6 only 514 #1 exists, so Santa gets record 1 and
object id 6001.

### 1.5 Node formats

All nodes start with a type byte. Byte +1 is free in level files (retail
has 0x05, 0x09, 0xFF); `sub_80120FEC` (modelRelocate) sets it to 0xB1 to mark
a node as relocated. Pointers are 32-bit addresses.

| Type | Name | Fields used by dbscan / renderer | Pointers relocated by modelRelocate |
|---|---|---|---|
| 0 | mesh | +2 id, +4 vertex array, +8 normal array, +0x0C primitive list, +0x10 primitive count, +0x14 (0x00010000 in all peds), +0x18 s16 (0x2000 = never cull), +0x1A s16 flags (bit 1 mirror, bit 2 near pass, bits 3..14 fixed OT z), +0x1C/+0x20/+0x24 sphere centre, +0x28 radius. 44 bytes | +4, +8, +0x0C (then `sub_80120744` (modelRemapTextures)) |
| 1 | group | +4..+0x0C centre, +0x10 radius, +0x14 u8 child count, +0x15 flags (bit 1 hidden, bit 2 never cull), +0x18 child pointers. 24 + 4n bytes | children |
| 3 | animation | see section 4.2. +0x0B frame count, +0x1C chained node, +0x20 frame pointers. 32 + 4n bytes | frames and +0x1C |
| 4 | offset | +4..+0x0C translation, +0x12 count, +0x14 children | children |
| 5 | transform | +4 rotation, +0x18 translation, +0x26 count, +0x28 children | children |
| 9 | select | +2 id, **+4 instance**, +6 u8 child count, **+7 u8 current child**, +8 u8 frame for +0x0C, +9 u8 trigger, +0x0C chained node, +0x10 child pointers. 16 + 4n bytes | children only (**not +0x0C**) |
| 2, 11, 12 | LOD, zones, scale | `DRAW_DISTANCE.md` | |

The walker for drawing is `sub_801526CC` (renderWalkModel); its pure-C
version is `reference/renderer_walker_engine_render_scene.c`. A type 9 node
draws only child `node[7]`.

Mesh vertex records are 8 bytes: s16 x, s16 y, s16 z, s16 pad (z is up).
dbscan also runs `sub_8014D4CC` (meshSetPrimQuadrants) on every mesh it
reaches: it rewrites bits 13..15 of each primitive's control word from the
sign of its first vertex. Those bits need no particular value in new data.

### 1.6 Adding Santa: two ways

Santa's PS1 nodes (section 2.4 gives the primitive words), as one 592-byte
image. Offsets are from the start of the image; "P(x)" means a pointer to
offset x.

| Offset | Size | Node | Contents |
|---|---|---|---|
| 0x000 | 32 | PSXD header (way A only) | "DXSP", 0x43, stamp, 0x80019C40, 3, 0x80019C58, count 1, root = P(0x20) |
| 0x020 | 28 | select 516 #2 | 09 05 04 02 02 00 03 00, then 0 (+8, +9), +0x0C = 0, children P(0x3C), P(0x54), P(0x7C) |
| 0x03C | 24 | empty group (step 0) | type 1, all other fields 0 |
| 0x054 | 40 | standing loop (step 1) | 03 09 00 00 00 00 0F 00 00 01 00 02 00 00 00 00 00 00 00 00 0F 00 10 00 0F 00 10 00, +0x1C = 0, frames P(0xA8), P(0xD4) |
| 0x07C | 44 | hit animation (step 2) | 03 09 00 00 00 02 0C 00 00 02 00 03 00 00 00 00 00 00 00 00 0C 00 0D 00 0C 00 0D 00, **+0x1C = P(0x20)** (the select node), frames P(0x100), P(0x12C), P(0x158) |
| 0x0A8 .. 0x183 | 5 x 44 | five meshes | type 0, +1 0xFF, id 0, verts P(0x238), normals P(0x238), prims P(0x184 + 36k), count 1, +0x14 0x00010000, centre (0,0,33) / (0,0,33) / (0,0,33) / (0,0,52) / (0,0,72), radius 47 (48 for the last) |
| 0x184 | 5 x 36 | five primitives | section 2.4 |
| 0x238 | 24 | three vertices | (0,0,34), (0,0,53), (0,0,72) |

The normal pointer is never read for these primitives (colour mode 0), but
it must point inside the image for way A, since modelRelocate adds the delta
to it.

**Way A: a relocatable asset, like a car (recommended).** This is the path
every car takes.

1. Build a TMS container for him: "TXSP" (u32 0x50535854), 0x43, any stamp,
   count 3, three keys that no level or car file uses, then for each image a
   u32 size and a TIM (id 0x10, flags 0x09, a 256x1 CLUT block, a 32x64
   pixel block; its VRAM x/y are ignored). Image 0 = PC texture 296, 1 = 297,
   2 = 298, palettes converted (section 2.6).
2. After the car TMS loop, call `bitmapParseContainer(container, n)` with n =
   the next group (1 + humans + AIs). `sub_801209E8` (vramPlaceTexture) and
   `sub_80120D3C` (vramUploadClut) find free VRAM and record each image in
   `D_80181034` (vramTexSlots)[n * 20 + index].
3. After the car DMD loop, put the image (with its header, pointers built as
   0x80019C40 + offset) anywhere in RAM, for example at 0x80019C40 + off,
   and call `sub_80120EF4` (assetInstallImage)(image, n). It checks the
   magic and version, sets the world shift from +0x10 (keep 3, as the level
   and every car have), relocates each root by `image - 0x80019C40`
   (`sub_80120FEC` (modelRelocate)), rewrites every textured primitive
   (`sub_80120744` (modelRemapTextures)) and walks the root with
   worldLoadObject.

In this form a primitive's tpage field holds the image's **index in the
container** (bits 0..4) and its colour mode (bits 7..8 = 1, 8-bit); the
CLUT half is ignored and UVs are relative to the image (0..63).
modelRemapTextures writes the real CLUT and page and adds the image's offset
inside its page to every UV (x % 64 * 2 for 8-bit, y & 0xFF). The car files
use exactly this: RK.DMD's primitives carry page indices 0..7 = the 8 images
of RK.TMS. CONFIRMED.

Rules this path imposes: no two mesh nodes may share a primitive list (each
mesh is remapped once, so a shared list would be shifted twice); the select
node's +0x0C must stay 0 (not relocated); all pointers must be inside the
image.

**Way B: fixed, appended to the level.** No relocation and no remap happen
for the level, so everything is absolute.

1. Right after the level TMS (step 2 of 1.1, before the car TMS loop),
   `LoadImage` the three images and CLUTs at a fixed free spot and mark them
   with `sub_801215F8` (bitmapMarkRect), so the car images avoid them
   (section 3.3).
2. After the level DMD read, write the image (header not needed) at
   B = 0x80019C40 + size, with pointers B + offset and absolute tpage/CLUT
   words. Add its size to `off` before the car DMD loop.
3. Register it, either:
   - **replace header root slot +0x40** (root 9, the group that already
     holds 514 #1) with a new 32-byte group node (type 1, centre 0, radius 0,
     u8 +0x14 = 2, children P(select), old root 9). The PC file does the
     same: its root 10 is a new group holding the 514 group and Santa's
     group. Children are popped last-first, so 514 #1 is still record 0; or
   - call `dbscanHierarchy(select, worldLoadShift, worldLoadObject)` once
     after `arenaResetSystems` (`D_80180EF0` (worldLoadShift) = 3).

## 2. Santa in the PC file (CONFIRMED)

### 2.1 Where he is

In `GLACIER.DPC` (248,656 bytes) root 10 (header slot +0x4C, 0x80056140) is a
group with two children: the 514 #1 group (same layout as the PS1's root 9)
and a new group for Santa. File offsets (pointer = 0x80019C40 + offset):

| Offset | Node | Notes |
|---|---|---|
| 0x03C4E0 | group, 2 children | centre (0,0,49), radius 70 |
| 0x03C438 | group, 1 child, flags 0x01 | centre (0,0,52), radius 67 |
| **0x03C41C** | **select 516 #2** | bytes 09 05 04 02 02 00 03 00, no chained node |
| 0x03C1E4 | step 0: empty group | |
| 0x03C2B4 | step 1: animation, 2 frames, period 15 | frames 0x03C1FC, 0x03C258 |
| 0x03C3F0 | step 2: animation, 3 frames, period 12, trigger 2, chained to 0x03C41C | frames 0x03C2DC, 0x03C338, 0x03C394 |
| 0x03C4C4 | hidden group (flags 0x02) holding a type 8 node at 0x03C454 | the 514 group has the same pair; type 8 is not drawn and has no id. Not needed |

Frames (one mesh, one primitive each):

| Frame | Mesh | Primitive | Centre vertex (DPC offset) | Half width / half height | PC texture |
|---|---|---|---|---|---|
| stand 0 | 0x03C1FC | 0x03C228 | (0,0,34) at 0x008E0C | 34 / 33 | 0x4128 = 296 |
| stand 1 | 0x03C258 | 0x03C284 | (0,0,34) | 34 / 33 | 0x4129 = 297 |
| hit 0 | 0x03C2DC | 0x03C308 | (0,0,34) | 34 / 33 | 0x412A = 298 |
| hit 1 | 0x03C338 | 0x03C364 | (0,0,53) at 0x008DE4 | 34 / 33 | 298 |
| hit 2 | 0x03C394 | 0x03C3C0 | (0,0,72) at 0x008E9C | 34 / 34 | 298 |

So he stands as a 68 x 66 unit billboard with his feet on the ground and,
when hit, flies up 38 units over three frames, then disappears (section
4.3). The meshes point into the level's shared vertex pool (0x8C9C, 0x8DC4,
0x8E7C + 8 x index) and normal pool (0x88F2C); the PS1 version needs its own
3 vertices.

### 2.2 Compared with other pedestrians

| | Steps | Standing frames | Hit frames | Prims per frame | Texture |
|---|---|---|---|---|---|
| PS1 GLACIER 514 #1 | 3 | 4 (crossed quads, 4 prims) | 10 | 1 beam | 64x64 8-bit at (960,128) / (896,128) |
| PS1 FREEWAY 516 #2 | 3 | 2 | 5 | 1 / 2 beams | 32x64 8-bit, page (704,0) |
| PC GLACIER 514 #1 | 3 | 4 (8 prims) | 10 (2 prims) | doubled | |
| **PC GLACIER 516 #2** | 3 | **2** | **3** | **1 beam** | **64x64 8-bit, own images** |

The PC doubles every PS1 primitive (two records, +0x0C = -4096 and +4096);
Santa's frames have only the +4096 one, so he was made for the PC (LIKELY).
His select, group and animation headers are byte for byte in the PS1
layout.

### 2.3 The billboard primitive on the PS1

`sub_800CDC54` (the primitive emitter, `reference/model_emitter_800CDC54.c`)
treats a primitive whose control word has type 1 (bits 16..18) as a
camera-facing quad: s16 +4 = vertex, s16 +6 = half width, s16 +8 = half
length. It projects vertex z + length and z - length and builds a POLY_FT4
around the screen line between them, width scaled by distance. Corner 0 =
top left, 1 = top right, 2 = bottom left, 3 = bottom right (for an upright
camera). CONFIRMED (code); corner order LIKELY (from the angle maths, not
seen on screen).

### 2.4 PC primitive to PS1 primitive

| PS1 offset | PS1 field | From the PC record (48 bytes) |
|---|---|---|
| +0 | u8 texture words 4, u8 colour words 1, u8 record length 9 (words), u8 packet length 9 | PC: 4, 1, 12, 9 |
| +4 | s16 vertex index | PC +4 (Santa: point at the right one of his 3 vertices) |
| +6 | s16 half width | PC +6 (34) |
| +8 | s16 half length | PC +8 (33, 34 for hit 2) |
| +10 | s16 | PC +10 (1, 0 for hit 2); not read by the emitter |
| +12 | control word | **0x95810000**: as the PS1 514 hit frames (type 1, packet stride 8, colour copy, 1 colour word, bit 31 no backface test). PC +16 holds the same fields in another layout (0x1681....) |
| +16 | GPU code + colour | **0x2C7F7F7F** (POLY_FT4, textured, like every PS1 pedestrian). PC +24 has 0x2CFFFFFF |
| +20 | uv0 + CLUT << 16 | corner (0,0) |
| +24 | uv1 + tpage << 16 | corner (63,0) |
| +28 | uv2 | corner (0,63) |
| +32 | uv3 | corner (63,63) |

PC +0x0C (+-4096), +20 and the texture id at +44 (0x4000 | TPC index) are
dropped. PC UVs are four u16 pairs (u low, v high) at +28..+40 in fan order:
PS1 corners = PC corners 0, 1, 3, 2 (LIKELY, from comparing the 514
records of both versions). All five Santa primitives have PC UVs
(0,0), (63,0), (63,63), (0,63), so on the PS1: (0,0), (63,0), (0,63),
(63,63), plus the image's place in VRAM.

### 2.5 The .TPC texture file

| Offset | Field |
|---|---|
| +0x00 | "TCPM", u32 0x43, stamp |
| +0x0C | image count (GLACIER: 300) |
| +0x10 | images, each: u32 size, then a TIM (id 0x10, flags 0x09 or 0x19, CLUT block 256x1, pixel block). VRAM x/y are all 0. No key table |

A PC primitive's texture id is 0x4000 | index. Checked: id 0x4124 (the PS1
514's hit image) = TPC 292, whose pixels are identical to PS1 GLACIER.TMS
image 9 (key 0xCE88, VRAM (896,128)). 48 of the 300 images are identical
to PS1 ones; the PS1 TMS has no key table entry matching Santa's.

| TPC | Entry at | CLUT data | Pixels (64 x 64 bytes) | Colours used |
|---|---|---|---|---|
| 296 | 0x16CDB0 | 0x16CDC8 | 0x16CFD4 | 241 |
| 297 | 0x16DFD4 | 0x16DFEC | 0x16E1F8 | 232 |
| 298 | 0x16F1F8 | 0x16F210 | 0x16F41C | 243 |

Each image has its own palette; about 3,100 of its 4,096 texels use colour
0x0000, which the PS1 GPU draws as transparent, and no used colour has bit
15 set. **Palette conversion:** PC entry `0RRRRRGGGGGBBBBB` (bit 15 kept) to
PS1 `SBBBBBGGGGGRRRRR`: PS1 = (pc & 0x8000) | ((pc & 0x1F) << 10) |
(pc & 0x3E0) | ((pc >> 10) & 0x1F). Checked on three shared images, where
the PC and PS1 palettes hold the same colours in these two layouts. The
pixel bytes need no change. Bit 4 of the TIM flags (0x19) is a PC-only flag
(LIKELY "has transparent colour"); use 0x09 on the PS1.

Three palettes need three CLUT rows. They can be merged into one 256-colour
palette (682 different colours in all, so with some loss) to use one row.

## 3. Texture on the PS1 side

### 3.1 VRAM during Antarctica (CONFIRMED, from the files and bitmapInit)

| Area (x, y, w, h) | Use |
|---|---|
| (0,0,640,256) | frame buffers (two 320x240 at x 0 and 320), reserved by `sub_80121550` (bitmapInit) |
| (960,256,64,32), (960,288,64,128), (960,416,32,96) | reserved by bitmapInit (fonts/HUD, LIKELY) |
| (640..1023, 0..255) | level images; CLUTs at x 768..1023, rows 0..88 |
| (768..1023, 89..103) | **free** after the level (row 88 partly used); car CLUTs go here first |
| (0..419, 256..511) | level images (GLACIER: 241 images, 163 4-bit, 78 8-bit; GLACIER2 the same up to x 412) |
| (420..959, 256..511) | **free** after the level; car images fill it from x 416 rightwards |

Car images are placed by `sub_801209E8` (vramPlaceTexture)
(`screen_deathmatch_result.c`): first pass from x 960, second pass over the
256-wide blocks from x 0, column by column, rows 256..511 only. A spot is
"free" if its **top-left texel** is free (`sub_801216A8` (bitmapIsFreeXY)).
CLUTs (`sub_80120D3C` (vramUploadClut), `scene_init.c`) take the first free
x (step 16 or 256) and row, scanning (0,0), (256,0), (512,0), (768,0), then
the lower blocks.

### 3.2 What the cars leave

A replay of that algorithm with the level files and the eight largest car
texture files (MG, IR, AX, HS, ST, RK, PV, OT; images shared by key are
reused, as `sub_801218CC` (bitmapFindRecord) does) leaves **x 768..959 of
the lower half free** and car CLUTs in rows 88..100 at x 768; rows 101..103
stay free. With Minion as a ninth car, rows 101.. are taken too. With Dark
Tooth (BB.TMS: 31 images, one 128x256) the lower half is full. LIKELY (a
replay of the code on the disc files, not a capture).

### 3.3 Where to put him

- **Way A:** nothing to choose; vramPlaceTexture finds room after the cars.
  With Dark Tooth in the race it can fail (it prints "Relocatable texture
  not loaded"), and Santa would draw with a wrong texture.
- **Way B:** upload before the car images, mark the rectangles, and the car
  placement steps around them. A spot that holds:

| What | VRAM | Notes |
|---|---|---|
| image 296 (stand 0) | (896,384), 32x64 | |
| image 297 (stand 1) | (928,384), 32x64 | |
| image 298 (hit) | (896,448), 32x64 | the three fill most of one aligned 64x128 cell, so no car image straddles into it (car images are aligned to their own size; only Dark Tooth's 128-wide image could) |
| CLUT 296 / 297 / 298 | (768,101), (768,102), (768,103), 256x1 | whole rows, so car CLUTs skip them |

All three images are in the 8-bit page at (896,256): tpage 0x9E.

### 3.4 Encoding (CONFIRMED, PS1 GPU and the PS1 pedestrian data)

- **tpage** (high half of texture word 1) = x / 64 | (y / 256) << 4 | abr
  << 5 | depth << 7 (depth 0 = 4-bit, 1 = 8-bit, 2 = 15-bit). The PS1 514
  uses 0x8F (8-bit, page (960,0)).
- **CLUT** (high half of texture word 0) = x / 16 | y << 6. The PS1 514 uses
  0x11B0 = (768,70).
- **UV** bytes are texels inside the page: for 8-bit, u = (x - page x) * 2 +
  texel u, v = y - page y + texel v.

Way B words, per image (u0, v0 = the image's corner in the page):

| Image | u0, v0 | Word 0 | Word 1 | Word 2 | Word 3 |
|---|---|---|---|---|---|
| 296 | 0, 128 | 0x19708000 | 0x009E803F | 0x0000BF00 | 0x0000BF3F |
| 297 | 64, 128 | 0x19B08040 | 0x009E807F | 0x0000BF40 | 0x0000BF7F |
| 298 | 0, 192 | 0x19F0C000 | 0x009EC03F | 0x0000FF00 | 0x0000FF3F |

Way A words: word 0 = 0x00000000, word 1 = 0x0080003F | index << 16,
word 2 = 0x00003F00, word 3 = 0x00003F3F (index 0, 1, 2 = images 296, 297,
298); modelRemapTextures fills in the rest.

A 15-bit version is possible (no CLUT, tpage depth 2) but takes 64 x 64
VRAM texels per image, 4 times more, for no gain: the source is already
8-bit.

## 4. The rest of a standing pedestrian (CONFIRMED unless tagged)

### 4.1 Placement

`sub_80125A18` (targetActorPlace) (`target_actor_init.c`): kind 516 is not one
of the walking kinds (514, 517), so on level 6 it picks `rand() % 4` of the
four rows `D_80163DB8` (targetStartPositions) mode6[0..3]:
(1800, 256, 5336), (-4648, 1416, 4816), (13704, -2832, 4016),
(1504, 664, 4816); adds `rand() / 409` (0..80) to x and y; sets the
rotation to 0. It picks again at every respawn. Whether each row lies on the
ground was not checked (the walkers' rows on this level are at z 4816 too).

### 4.2 Steps and animation

`sub_80124384` (objRegistrySetStep)(516, 2, step) calls `sub_80151F14`
(animSetStep), which sets the select node's +7:

| Step | Child | When |
|---|---|---|
| 0 | empty group: not drawn | every update while his z is 201 or more above the ground (`sub_800EF340` (worldGroundHeightAt)); and when the hit animation ends |
| 1 | standing loop | spawn and respawn |
| 2 | hit animation | run over or hit |

Animation node (type 3) fields (`engine_anim_hit_volumes.c`): +5 trigger
frame, u16 +6 period (in render ticks, `D_80181108` (renderFrameTicks)),
+8 first, +9 last, +0x0A mode (0 loop), +0x0B frame count, +0x0C current
frame, +0x12 manual (0 = advanced while drawn), +0x13 frame to send to the
chained node, +0x14..+0x1A random period limits used at frame 0
(`animPickPeriod`), +0x1C chained node. `sub_801519DC` (animStepLoop)
(`animation_frame_update.c`): when the frame reaches the trigger it calls
`sub_80151F90` (animControl)(chained, frame). Santa's hit animation is
chained to his own select node with frame 0, so on its last frame he
switches to step 0 and vanishes; the last frame shows for one render frame.
The PS1 514 and FREEWAY 516 do the same. Animations advance only while
drawn, and step 2 does not rewind the hit animation; retail pedestrians work
the same way.

### 4.3 States (`sub_80126464` (targetActorsUpdate), `target_actor_update.c`)

| State (+2) | What happens |
|---|---|
| 0 standing | a human or AI car within 80 units (largest axis, `>> 3 < 10`) throws him: speed = car speed / 32, direction = the car's matrix |
| 1 thrown | moves speed + 5 per update along that direction; after 42 updates goes to 2 |
| 2 settling | after about 260 more updates `sub_801259A4` (targetActorRespawn): new row, step 1, state 0 |

Weapons: `sub_80127228` (targetActorHit) (`target_actor_events.c`), called by
the weapon and blast code that loops over the 6 records (`sub_80126454`
(targetActorsGet); for example `area_damage.c` within 400 units and
`weapon_basic.c` within 800, largest axis), sets state 1 with speed 25 and the direction
of the hit. Only the first hit counts until he respawns. Collision is by
distance to his position only; the model plays no part.

### 4.4 Sounds, kill and bonus

| Event | Sound / effect |
|---|---|
| run over or hit (kinds other than 517) | `soundPlay(25, ...)` at his position |
| run over by a human | `soundPlay(3, 0, 0, 45)` |
| any hit | `sub_80119530` (fxTargetHitAt): hit effect 40 units up, sound 0x29D |
| 10th pedestrian run over by humans (any kind, `D_80180BBC` (targetActorRunOverCount)) | that car gets up to 5 of weapon slot 2, a third of its max HP, HUD message 34, sound 24 |

No sound is per kind, so Santa has the same voice as the others (PC sound
data not checked). His kill, for the port's event hooks (`GAME_EVENTS.md`
14.1): `objRegistrySetStep(516, 2, 2)` at 80126C78 (human), 80127014 (AI)
or 8012733C (weapons), on level 6.

## 5. Recipe for the port (way A)

1. Offline, from `GLACIER.DPC` and `GLACIER.TPC`: take images 296..298,
   convert their palettes, and build the 3-image TMS container and the
   592-byte PSXD image of section 1.6 (5 primitives per section 2.4, way A
   texture words). Use the same pair for GLACIER2.
2. In `sessionLoadArenaAndCars`, on level 6: after the car TMS loop,
   `bitmapParseContainer(santaTms, n)`; after the car DMD loop,
   `assetInstallImage(santaImage, n)`, n = 1 + humans + AIs.
3. Nothing else: he spawns as record 1 (object 6001), stands on one of the
   four rows, and can be run over, shot, counted and respawned by the retail
   code.

## Not checked

- The PC EXE (whether it uses the same start rows, and any Santa-only code
  or sound).
- The ground height at the four rows (if a row is 201 or more above the
  ground he stays hidden there).
- Discs of other regions.
- On screen: the corner order of section 2.3 (if he shows mirrored, swap
  uv0/uv1 and uv2/uv3).
