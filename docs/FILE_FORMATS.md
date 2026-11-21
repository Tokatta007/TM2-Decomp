# Model (DMD) and texture (TMS) formats, and the PC release's DPC / TPC

PC port request 42, item 4 (`CUSTOM_TOURNAMENT_ANSWERS.md` 4): a written spec of the PlayStation release's model and
texture files for the port's mod converters (from common formats and from Twisted Metal 1's files), as far as the
decomp knows them. Sources: the game's C in `src/semantic`, the pure-C versions of the hand-written asm in
`reference/`, the other docs, and read-only scans of the USA disc (every `DMD\*.DMD` and `TMS\*.TMS`) and of the PC
release's `CARSDB\RK.DPC/.TPC` and `LEVELDB\GLACIER.DPC/.TPC`. No game data is reproduced here.

Every field carries a tag:

- **K** (known): read or written by game code (function, address and source file given) and seen in the disc data.
- **I** (inferred): follows from code or data, but no code path proves the meaning, or it is seen only in data.
- **U** (unknown): no reader found, or the meaning is not established. The entry says how to find out.

All multi-byte values are little-endian; a word is 4 bytes; addresses are the retail USA EXE's.

---

## 0. Where the files live and how they are loaded (K)

| File | Folder | Loaded by | Treatment |
|---|---|---|---|
| Arena model `<STEM>.DMD` (`<STEM>2.DMD` in modes 2 / 4 on levels 3, 5, 6) | `DMD\` | `sessionLoadArenaAndCars` 8012BAFC (`shell_main.c`) reads it to **0x80019C40**; `arenaResetSystems` 80128CE4 -> `worldLoadStartObjects` 800D1C70 (`rt_main.c`) | **never relocated**: its pointers are absolute for 0x80019C40; no texture remap: the primitives hold final tpage / CLUT / UV values |
| Car model `XX.DMD` (HH OT WH MG PV TH SP RK IR AX FL HS ST MN BB) | `DMD\` | the same function, packed after the arena DMD (the port: anywhere, `portCarLoadGroup`), then `assetInstallImage(buf, n)` 80120EF4 (`asset_header.c`), n = the load group 1.. | **relocated** (`modelRelocate` 80120FEC) and **texture-remapped** (`modelRemapTextures` 80120744) |
| Arena textures `<STEM>.TMS` | `TMS\` | `bitmapParseContainer(0x80019C40, 0)` 80121A2C (`bitmap_container_parse.c`) -> `bitmapRegisterTim` 8012191C | every image uploaded at the VRAM x / y of its own TIM header |
| Car textures `XX.TMS` | `TMS\` | `bitmapParseContainer(buf, n)` -> `vramPlaceTexture(info, index, n)` 801209E8 (`screen_deathmatch_result.c`) | the TIM x / y are ignored; the game finds VRAM, uploads, and records the spot in `vramTexSlots[n * stride + index]` |
| Shell screens (`SHELLDB1\CARSEL.TMS/.DMD`, `AICARSEL`, `LEVELSEL`, ...) | `SHELLDB1\`, `SHELLDB2\` | `shellLoadTmsDmd` 80129010 (`screen_card_layout.c`): the TMS with flag 0, the DMD at 0x80019C40 + `worldLoadStartObjects` | as an arena (fixed, not relocated) |

The order in a match (`sessionLoadArenaAndCars`): `bitmapInit` -> the arena TMS (flag 0) -> each car's TMS (flag
n = 1.., humans then computer cars) -> the arena DMD -> `arenaResetSystems` (walks the arena's roots) -> each car's DMD
and `assetInstallImage(.., n)` with the **same n** as its TMS -> `bitmapDisableTracking`. A car DMD's primitives
find their images only through that shared group number n.

RAM (`SANTA.md` 1.3): the arena DMD and every car DMD share 0x80019C40..0x800CDC54 (737,300 bytes) in retail (the
port can move the cars above 2 MB: `portCarOptions` bit 1). The CD reader reads whole 2,048-byte sectors, so up to
2,047 bytes past a file's end are overwritten (`fileioReadFile` 8014D910, `SANTA.md` 1.1).

There is no other container: one DMD and one TMS per arena, car and shell screen.

---

## 1. The readers in the repositories

| Tool | Parses | Notes |
|---|---|---|
| `tools/dmd_lod_dump.py` | `Disc`: a read-only ISO 9660 reader of the MODE2/2352 track. `Dmd`: the PSXD header (magic, origin +0x0C, root count +0x18, roots +0x1C), `children()` per node type (1, 2, 3, 4, 5, 9, 11, 12), the LOD records `{max, min, child}`, `draws()` (reaches a mesh), pickup ids, mesh / primitive cost per file | the reference DMD tree walker in Python; reads node headers only, not mesh or primitive contents |
| `tools/check_car_groups.py` | `dbscan_ids()`: a DMD's callback ids in `dbscanHierarchy` order (`--dmd`, `--key-model`, `--unload-model`); `tms()` in `check_bank_model` and `check_clut_model`: the TMS header, key table, each entry's TIM flags, CLUT and pixel rectangles | host models of the `vramPlaceTexture` / `vramUploadClut` placement |
| `reference/renderer_walker_engine_render_scene.c` | C of `renderWalkModel` 801526CC (the render tree walk) | the node fields used for drawing |
| `reference/model_emitter_800CDC54.c` | C of the primitive emitter `sub_800CDC54` | the primitive record |
| `reference/hdp_walker_engine_hit_detection.c` | C of the collision walk (`hdpTraverse` 8014FEEC) | the node fields used for hit tests |
| port repository: `tools/art/santa.py` | reads PC `GLACIER.DPC` nodes / primitives and `GLACIER.TPC` images, **writes** a PS1 TMS and a one-root relocatable PSXD image | the only existing writer of both formats: a worked example of section 6 |
| port repository: `tools/disc/tim.py`, `extract_images.py` | TIM decode (4 / 8 / 16 / 24 bpp, CLUT), scan of `.TMS` and `.TPC` packs for TIMs, `.TPC` colours as RGB | image extraction only; does not read the TMS key table |

No tool reads vertex / normal arrays or primitives in bulk yet: a full DMD dumper (tree, meshes, primitives) is the
first thing a converter author will want.

---

## 2. DMD ("PSXD" model image)

### 2.1 Header (32 bytes and the root table)

| Off | Type | Field | Tag, evidence |
|---|---|---|---|
| +0x00 | u32 | magic 0x50535844 (bytes `DXSP`) | K: `assetInstallImage` exits (-1) otherwise; `worldLoadStartObjects` reads it and ignores it |
| +0x04 | u32 | version 0x43 | K: the same check |
| +0x08 | u32 | build stamp, equal to the +0x08 word of the paired TMS (e.g. RK 0x3250582C in both) | K that it is read and dropped (`worldLoadStartObjects` reads 0x80019C48 only when its flag is set); I: a time stamp. Any value works |
| +0x0C | u32 | 0x80019C40, the address the pointers were built for | K in data (all 29 files); **not read**: the relocation constant is hard-coded (`delta = base + 0x7FFE63C0` = base - 0x80019C40). Write 0x80019C40 |
| +0x10 | u32 | world coordinate shift (3 in every file) | K: `renderSetWorldShift` 8015166C, `assetInstallShift` / `worldLoadShift`; a **global**: a car file with another value changes the whole scene's shift. Write 3 |
| +0x14 | u32 | pointer to the root table (0x80019C58) | K in data; not read. Write 0x80019C58 |
| +0x18 | u32 | root count | K: both loaders loop `count` times; a zero root pointer stops early |
| +0x1C | u32[count] | root node pointers | K. The first node normally starts right after this table |

On the disc: car files have 1 root (BB.DMD 3), arenas 5..11.

### 2.2 Pointers and relocation (K: `modelRelocate` 80120FEC, `src/semantic/screen_card_draw.c`)

- Every pointer in a DMD is an absolute address: `0x80019C40 + file offset`.
- Arena and shell DMDs are used in place at 0x80019C40: no fixup.
- Car DMDs (and any file installed with `assetInstallImage`) get `delta = load address - 0x80019C40` added to the
  pointers below, **per node, once**: byte +1 of a node is set to **0xB1** when it is relocated, and a node whose byte
  +1 already is 0xB1 is skipped. **A converter must not write 0xB1 into byte +1** of a relocatable file (retail files
  hold 0xFF in meshes, 0x05 / 0x09 / 0xFF elsewhere).
- The relocator walks only through child links (an explicit stack, `modelRelocateStack`):

| Node type | Pointers relocated | Followed |
|---|---|---|
| 0 mesh | +0x04 vertices, +0x08 normals, +0x0C primitives (always all three) | then `modelRemapTextures(node)` |
| 1 group | the children at +0x18 (count u8 +0x14) | yes |
| 2 LOD | the record pointers at +0x14 (count s32 +0x10) and each record's +8 child | the record children |
| 3 animation | the frames at +0x20 (count u8 +0x0B); +0x1C the chained node if non-zero | the frames only (+0x1C is not followed) |
| 4 offset | the children at +0x14 (count u8 +0x12) | yes |
| 5 transform | the children at +0x28 (count u8 +0x26) | yes |
| 7, 8 hit box | +0x0C if non-zero (byte +1 set to 0xB1 only then) | no |
| 9 select | the children at +0x10 (count u8 +0x06); **not** +0x0C | yes |
| 11 zones | +0x08 outside child, +0x0C inside child, the region pointers at +0x10 (count u8 +0x06) | +0x08 and +0x0C only |
| 12 scale | the children at +0x08 (count u16 +0x06) | yes |
| other | nothing | no |

Consequences (K unless marked): every pointer must stay inside the file; a node reachable only through a pointer
that is not relocated (select +0x0C, animation +0x1C, zone regions) is not relocated itself, so it must also be a
child somewhere (retail files do this); a mesh's normal pointer is relocated even when unused, so it must point inside
the file (`SANTA.md` 1.6); two meshes must not share one primitive list in a relocatable file (it would be remapped
twice and its UVs shifted twice). Sharing vertex or normal arrays is fine (arena meshes share pools).

### 2.3 The load-time walk and typed objects (K: `dbscanHierarchy` 8014ABF8, `src/semantic/engine_clut_anim.c`)

After relocation each root is walked by `dbscanHierarchy(root, shift, worldLoadObject)`. For each node it reads the
**object id, u16 at +0x02** (types 0, 1, 2, 3, 4, 5, 8, 9, 11, 13; types 6 and 12 report 0), accumulates translation
and rotation as the renderer does, and for a non-zero id calls `worldLoadObject(id, node, worldPos, parent)`
800D1DB0 (`src/semantic/world_load_object.c`), which hands the node to the subsystem that owns the id. It also runs
`meshSetPrimQuadrants` 8014D4CC on every mesh (rewrites control-word bits 13..15, 2.6) and, for type 3,
`node[13] = 1` when `node[10] == 1` and `animRegister(node)` when `node[18] == 1`. Any type other than 0..6, 8, 9,
11, 12, 13 prints "Bad hierarchy opcode".

"Typed objects" are nodes with a meaningful id. The main ids (K, `worldLoadObject`; the full list is there):

| Id | Meaning |
|---|---|
| 1 | the arena model (attached to the scene root once) |
| 2 | a camera-attached model (view follow object) |
| 3 | a free-standing world model (a new scene object) |
| 16, 32, ..., 240 | the **car root** of car type id / 16 (`carRegisterModel`); must be the file's first root and first callback (all 15 car DMDs: `tools/check_car_groups.py --dmd`) |
| type * 16 + 2 (18, 50, ...) | tire switch; + 3 the body object (`carBodyObjectRegister`); + 5 the wreck model; + 7 the freeze build; + 8 a CLUT animation (`clutAnimRegister(obj, 2)`); + 4 / + 6 per type (20, 22, ...) the LA events |
| 34, 178, 194, 226, 242 | special parts: Hammerhead, Axel, Mr. Slam, Minion, Dark Tooth |
| 249 | Dark Tooth's boss head (`weaponAssetBind`) |
| 401..430, 441..480 | pickups (`PICKUP_LOOKS.md`); the loader writes the pickup index into the node (u16 +0x16 of a group, +0x10 of an offset) |
| 500..520 | targets and pedestrians (514, 516, 517 need a type 9 select node: `SANTA.md` 1.4) |
| 650..692 | explosion, smoke, debris and splash animations; 693 warps |
| 1260, 1290..1293 | dashboard icons, radar arrows |
| 2201..2326 | weapon projectile models and weapon assets |
| 3000..4009 | arena set pieces (Moscow's dish, the Eiffel tower 3201, the collapse 3100, ...) |
| 5000, 5001 | CLUT cycle animations (arena) |
| >= 9000 | shell scene resources (`sceneDeliverResource`; car select's carousel is the select node 9005, its name labels 9006 / 9007) |

A **car** converter must keep the id layout of the car it replaces: the root id = type * 16 and the sub-object ids the
game expects for that type. Which sub-objects each type needs is not tabulated in one place: read the original file
with `dbscan_ids()` (`tools/check_car_groups.py`) and mirror it (I: a missing one is mostly skipped silently; not
checked case by case).

### 2.4 The model tree: node types

Every node starts with **u8 type** (+0) and **u8 flag** (+1, 2.2). Children are pointer lists; the renderer and
dbscan push them in list order and pop them last first. A node's size is its header plus 4 bytes per child. Disc
census (all 29 DMDs, the nodes reachable from the roots): type 0: 9,721; 1: 6,712; 2: 4,508; 3: 436; 4: 3,315; 5: 664;
8: 2,437; 9: 405; 11: 212; 12: 21. Types 6, 7, 10 and 13 do not occur.

Coordinates are s32 positions in **model units**; the renderer turns them into world units with `>> shift` (shift =
header +0x10 = 3, changed per subtree by type 12): K, `renderWalkModel`'s LOD and far tests, `DRAW_DISTANCE.md` 2. Z is
up (I: `SANTA.md` 1.5, pedestrian data). Rotation matrices are s16 3 x 3 with 4096 = 1.0 (K: `DOT3 / 4096`).

**Type 0, mesh (44 bytes)**: a leaf drawn by the emitter. K: walker handler 0, the `Mesh` struct of
`reference/model_emitter_800CDC54.c`, `modelRemapTextures`, `meshSetPrimQuadrants`.

| Off | Type | Field | Tag |
|---|---|---|---|
| +0x02 | u16 | object id (usually 0) | K |
| +0x04 | ptr | vertex array (2.5) | K |
| +0x08 | ptr | normal array (2.5) | K |
| +0x0C | ptr | primitive list (2.6) | K |
| +0x10 | u32 | primitive count | K |
| +0x14 | u32 | 0x00010000 in 9,279 meshes, 0 in 415, 0x0001000N (N 1..3) in 27 | **U**: no reader in the walker, the emitter, dbscan or the remap. Write 0x00010000. To find out: search the EXE for loads at mesh + 0x14 (`lw .., 20(` on a type 0 node) |
| +0x18 | u16 | bit 13 (0x2000) = never sphere-culled (6,745 meshes); bits 14..15 seen (0x8000, 0xA000, 0xC000, 0xE000) | K bit 13 (walker); **U** bits 14..15 |
| +0x1A | u16 | bit 1: wrap every packet in draw-mode packets (dither on via `mode_b`); bit 2: may be deferred to the near-polygon pass (`renderNearPolys`) when close; bits 3..14: a fixed OT z (0 = per polygon) | K (emitter); I the purpose of bit 1 (named "mirror" in the reference C) |
| +0x1C..+0x24 | s32 x 3 | bounding-sphere centre (node frame, model units) | K (walker cull) |
| +0x28 | s32 | bounding-sphere radius | K |

The vertex and normal counts are **not stored**: the arrays are as long as the largest index used (K by absence).

**Type 1, group (24 + 4n)**. K: walker handler 1, dbscan, `hdpTraverse`, `csSphereVisible` 80149344.

| Off | Field |
|---|---|
| +0x02 u16 | id |
| +0x04..+0x0C s32 x 3 | bounding-sphere centre (view culling, the far test, the hit-test sphere) |
| +0x10 s32 | radius |
| +0x14 u8 | child count |
| +0x15 u8 | flags: bit 0 skip in hit tests, bit 1 hidden (not drawn), bit 2 never culled; bit 4 an object root not sphere-tested by `csSphereVisible` |
| +0x16 u16 | secondary id for hit tags (`hdpSetNodeIds`); the pickup index is written here at load |
| +0x18 ptr[n] | children |

An object's root should be a type 1 group with a correct bounding sphere: `csSphereVisible` culls whole objects (cars,
pickups) by the root's sphere when the root is type 1 (K, `DRAW_DISTANCE.md` 3). (Car select's empty node, the port's
hidden group: type 1 with +0x15 = 2, `car_numbers.h`.)

**Type 2, LOD (20 + 4n)**. K: walker handler 2, `tools/dmd_lod_dump.py`, `FAR_LOD.md`.

| Off | Field |
|---|---|
| +0x02 u16 | id |
| +0x04..+0x0C s32 x 3 | the reference point of the distance |
| +0x10 s32 | record count (the walker caps nothing; the port's scan caps at 32) |
| +0x14 ptr[n] | records of 12 bytes: **u32 max, u32 min, ptr child** |

The walker computes d2 = the squared distance in **world** units ((pos + t) >> shift per axis, through the GTE's
16-bit IR registers, so it saturates past +-32767) and draws the child of the **first** record with min <= d2 < max
(unsigned); max 0xFFFFFFFF is unbounded. A retail override forces d2 = 100 in some views (gp + 1008). Cars use bands
such as [0, 200) | [200, 500) | [500, 1000) (distances; stored squared). LOD nodes are also used by the hit pass
(`hdpTestSphereBands`); 834 of them hold only hit boxes.

**Type 3, animation (32 + 4n)**. K: walker handler 3, `animUpdateOne`, `SANTA.md` 4.2.

| Off | Field |
|---|---|
| +0x02 u16 | id; +0x04 u8 secondary id (hit tags) |
| +0x05 u8 | trigger frame (sends +0x13 to the chained node when reached) |
| +0x06 u16 | period in render ticks |
| +0x08 u8 / +0x09 u8 | first / last frame |
| +0x0A u8 | mode (0 loop; others ping-pong / random, `animStep*`); 1 makes dbscan set +0x0D |
| +0x0B u8 | frame count |
| +0x0C u8 | current frame (the child drawn) |
| +0x12 u8 | 0 = advanced while drawn; 1 = registered in the global animation list at load (`animRegister`) |
| +0x13 u8 | the frame value sent to the chained node |
| +0x14..+0x1A | random period limits (`animPickPeriod`) |
| +0x1C ptr | chained node (relocated if non-zero, never walked) |
| +0x20 ptr[n] | the frames (children) |

**Type 4, offset (20 + 4n)**. K: walker, dbscan, hdp. +0x02 id; +0x04..+0x0C s32 translation added to the current
position; +0x10 u16 secondary id (the pickup index is written here); +0x12 u8 child count; +0x13 u8 flags (bit 0 skip
in hit tests, bit 1 hidden); +0x14 children.

**Type 5, transform (40 + 4n)**. K: walker, dbscan, hdp. +0x02 id; +0x04 s16[9] rotation (row-major, 4096 = 1.0);
+0x18..+0x20 s32 translation; +0x24 s16 secondary id; +0x26 u8 child count; +0x27 u8 flags (bit 0 skip hit, bit 1
hidden); +0x28 children. The walker computes the child translation as `t'[k] = sum_j R[k][j] * (t + T)[j] / 4096` and
a new matrix slot (at most 198 per frame); dbscan uses `R * (t + T)` and `transpose(R)` for the child frame.

**Type 8, hit box (80 bytes)**: collision only, never drawn. K: `hdpTraverse` case 8, `hdpTestBox` 80150694
(`src/semantic/engine_anim_hit_volumes.c`). +0x02 id (hit tag); +0x04 u8 plane count (3..6); +0x05 s8 (-2 = the
"consume" flag); +0x06 u8 tag byte; +0x07 u8 (1 = disabled); +0x0B u8 frame sent to +0x0C on a hit; +0x0C ptr the
node `animControl` drives on a hit (relocated if non-zero); +0x10 plane set 1 and +0x30 plane set 2, 32 bytes each:
s16[9] matrix, 2 pad bytes, s32[3] offsets; a point p is inside when every `M * p + offset > 0` for the first `planes`
rows (set 2 holds rows 4..6). I: the exact matrix and offset sign convention beyond "> 0 inside".

**Type 9, select (16 + 4n)**. K: walker handler 9, `SANTA.md` 1.5. +0x02 id; +0x04 u16 instance number; +0x06 u8
child count; **+0x07 u8 the current child (the only one drawn)**; +0x08 u8 frame for +0x0C; +0x09 u8 trigger; +0x0C
ptr chained node (**not relocated**: keep 0 in a relocatable file; retail uses 0); +0x10 children. Car select's
carousel and name labels are type 9 nodes with 14 children (`CAR_SELECT_CAR.md`, `DARK_TOOTH_PLAYER.md` 9).

**Type 11, zones (16 + 4n)**. K: walker handler 11, `hdpTestVolumeGroup`. +0x02 id; +0x04 u8 flags (bit 0 skip hit,
bit 1 hidden); +0x05 u8 secondary id; +0x06 u8 region count; +0x08 ptr the child drawn while the camera is outside
every region; +0x0C ptr the child drawn while it is inside one; +0x10 ptr[n] regions (68 bytes each: u8 plane count
3..6, pad, +0x04 s16[9], pad, s32[3] offsets, +0x24 a second set for rows 4..6, its offsets at +0x38). Regions are
data, not nodes.

**Type 12, scale (8 + 4n)**. K: walker, dbscan. +0x04 u16 the subtree's coordinate shift; +0x06 u16 child count;
+0x08 children. Positions are rescaled by the difference of the shifts. No id (dbscan reports 0).

**Types 6, 7, 10, 13**: K: the renderer skips them. 6 = skip (id 0); 7 is relocated like 8, but dbscan prints "Bad
hierarchy opcode"; 13 = id only (hdp ignores it); 10 = nothing. None on the disc: **do not emit them**.

Named nodes a converter meets (K via the docs): the car root group (id type * 16), the select node of pedestrians
(ids 514 / 516 / 517, `SANTA.md`), car select's carousel select 9005 (`CAR_SELECT_CAR.md`), pickups
(`PICKUP_LOOKS.md`: `group(mesh, anim(2 frames), hit box)`), Dark Tooth's head 249 (`DARK_TOOTH_PLAYER.md`).

### 2.5 Vertex and normal records (K)

- **Vertex**: 8 bytes, `s16 x, s16 y, s16 z, s16 pad` (the emitter loads them with `lwc2 $0 / $1` into GTE V0..V2;
  `meshSetPrimQuadrants` reads x, y as `verts[i * 4]`, `verts[i * 4 + 1]`), in model units of the node's frame (2.4).
  The pad half goes into the GTE's VZ high half and is ignored: write 0.
- **Normal**: 8 bytes, `s16 x, s16 y, s16 z, s16 pad`, unit length 4096 (data: (0, 0, 4096), (-4096, 0, 0)); used by
  NCCS lighting (`lwc2` into V0, then `cop2 0x108041B`).
- Indices are s16: at most 32,767 vertices or normals per array.

### 2.6 Primitive records (K: the emitter `reference/model_emitter_800CDC54.c`, `modelRemapTextures`, the disc census)

Records have variable length and follow each other: `next = this + 4 * byte[2]`. The table is also in
`TEAM_COLORS.md` 2.1.

| Off | Type | Field |
|---|---|---|
| +0x00 | u8 | bits 0..6 = texture word count T (0 untextured, 3 triangle, 4 quad); bit 7 = texture-window flag W |
| +0x01 | u8 | colour word count C (1 = flat, 3 / 4 = per vertex) |
| +0x02 | u8 | record length in words |
| +0x03 | u8 | GPU packet length in words (the `len` field of the POLY_* packet: F3 4, F4 5, G3 6, FT3 7, G4 8, FT4 / GT3 9, GT4 12) |
| +0x04 | s16, s16 | vertex indices v0, v1 (sprites and beams: +0x04 the centre vertex, +0x06 the half width, +0x08 the half height or half length) |
| +0x08 | s16, s16 | v2, v3 (v3 for quads only) |
| +0x0C | u32 | the control word (below) |
| +0x10 | u32[C] | colour words `r, g, b, GPU code` (the first word's top byte is the GP0 command: 0x20 F3, 0x24 FT3, 0x28 F4, 0x2C FT4, 0x30 G3, 0x34 GT3, 0x38 G4, 0x3C GT4; + 2 = semi-transparent; a textured polygon's neutral brightness is 0x80, retail uses 0x7F) |
| +0x10 + 4C | u32[T] | the texture words, exactly the GPU packet's: `uv0 \| CLUT << 16`, `uv1 \| tpage << 16`, `uv2`, `uv3` |
| next | u32 (if W) | the texture-window word (GP0 0xE2 form: offset x / 8 in bits 10..14, offset y / 8 in bits 15..19, the masks in bits 0..9) |
| next | s16[] | normal indices (lit primitives), padded to a word |

The control word (+0x0C):

| Bits | Meaning | Tag |
|---|---|---|
| 0..6 | 0 in all 107,116 disc primitives | K (data); **U** meaning; write 0 |
| 7..12 | a signed OT-z bias added for z < 1024 | K (emitter: `(ctl << 19) >> 26`) |
| 13..15 | the quadrant of v0, rewritten at load by `meshSetPrimQuadrants` | K; any value |
| 16..18 | primitive kind: 3 triangle, 4 quad, 1 camera-facing beam, 0 (and 2, 5..7) point sprite | K |
| 19..21 | lighting: 0 = the colours copied unlit; 1 = one normal for all colours; 3 = per-vertex normals (triangle); 4 = per-vertex (quad); lit colours go through NCCS (the colour word is the material colour) | K (emitter; the data shows only 0, 1, 3, 4) |
| 22..23 | the packet's stride per vertex in words: 1 (F: xy), 2 (FT / G), 3 (GT) | K |
| 24..27 | the word offset from +0x10 to the normal indices = C + T + W | K (emitter: `src + ((ctl >> 24) & 0xF) * 4`; the data agrees for every census row) |
| 28..30 | the colour word count again (= byte +1) | K (`modelRemapTextures` finds the texture words at `+16 + ((byte15 >> 4) & 7) * 4`) |
| 31 | 1 = no backface test (double-sided) | K |

Record length = 4 + C + T + W + ceil(normals / 2) words, where normals = 0 (lighting 0), 1 (1), 3 or 4 (3 / 4). The
census rows all satisfy this (e.g. FT4 unlit 9, FT4 with a window 10, FT4 flat-lit 10, GT4 with 4 colours 12, GT4 with 4
normals 14).

Rules from the emitter (K):

- The vertex order is the PS1 GPU's: triangles v0 v1 v2; quads v0 v1 v2 v3 in "Z" order (v0 top-left, v1 top-right,
  v2 bottom-left, v3 bottom-right of the texture), **not** fan order.
- Backface: NCLIP of the projected v0, v1, v2; with bit 31 clear the polygon is dropped when NCLIP < 0 (normal views;
  the mirror view flips the test). A converter from a counter-clockwise-front format must check the winding on one
  model first (I: the expected world-space winding follows from this test and the projection; not checked on
  screen).
- The polygon's z for sorting = the largest SZ of its vertices >> shift (world units), compressed above 1024
  (`renderCompressDepth` 8014D368); a mesh flag can force a fixed OT z.
- The GPU packet is assembled directly from these words (colours at packet + 4, xy at + 8, texture words at + 12, each
  advancing by the stride), so the GPU code, the packet length and the stride must agree: a converter should emit only
  the combinations of the census (FT3, FT4, GT3, GT4, F4, G3, G4 and their semi-transparent forms; beams and sprites
  with FT4 packets).

### 2.7 How primitives refer to textures

**Arena and shell DMDs (fixed textures)**. K (no remap runs on them; data: FREEWAY tpage values 0x37, 0x11, 0x96,
...): the texture words hold final GPU values. `CLUT = x / 16 | y << 6` of the CLUT's VRAM spot; `tpage = x / 64 |
(y / 256) << 4 | abr << 5 | depth << 7` (depth 0 = 4-bit, 1 = 8-bit, 2 = 15-bit); the UVs are texel coordinates inside
the 256 x 256 page (an 8-bit image's u = (x - page x) * 2 + texel u; 4-bit: * 4). The VRAM spots are those written in
the arena TMS's TIM headers (section 3).

**Car DMDs and anything installed with `assetInstallImage` (relocatable textures)**. K (`modelRemapTextures`
80120744; the data of all 15 car DMDs, `TEAM_COLORS.md` 2.2):

- tpage bits 0..4 = **the index of the image in the TMS loaded with the same group n** (0..19 in retail, 0..31 with
  `TM2_TEX_SLOT_STRIDE` 32 under `TM2_PORT_CARS` or `TM2_PORT_DT_PLAYER`; retail's stride 20 means a group with more
  than 20 images overwrites the next group's slots: Dark Tooth's 31 images, `TEXTURE_BANKS.md` 1.5);
- tpage bits 5..6 = abr (the semi-transparency mode), bits 7..8 = depth; they must match the image's TIM mode;
- the CLUT half is ignored (0 in every car file) and overwritten;
- the UVs are relative to the image's top-left texel; the remap adds `(x % 64) * 4` (4-bit), `* 2` (8-bit) or `* 1`
  (15-bit) to every u and `y & 0xFF` to every v, so `image origin + uv` must stay below 256 (I: images up to 256
  texels wide and high work, as the 128 x 128 car bodies do);
- for a window-flagged record (byte 0 bit 7) the UVs are left alone and the window word's offset fields are set to the
  image's position instead (repeating textures).

The remap runs once per mesh node (guarded by the 0xB1 flag, 2.2). The port's car select draws a mod car's in-game
model with the same remap through its own slot table (`DARK_TOOTH_PLAYER.md` 9.3).

### 2.8 LOD and bounding data (K)

- The bounding spheres of meshes and groups (centre and radius, model units, the node's frame) drive view culling and
  the far-distance test (4,800 world units by default, `DRAW_DISTANCE.md` 2). Wrong spheres make geometry pop out.
- Distance LOD = type 2 nodes with squared world-unit bands; the first matching record is drawn; no blending.
- Collision uses the same tree: group spheres (unless +0x15 bit 0), LOD bands (`hdpTestSphereBands`), zone volumes and
  type 8 boxes; other cars collide against **this model's** hit volumes (`BALANCE.md` 12.10). A car converter must
  keep (or rebuild) the hit boxes; the car's own collision points come from per-type constants, not from its model.

---

## 3. TMS ("PSXT" texture container)

K: `bitmapParseContainer` 80121A2C (`src/semantic/bitmap_container_parse.c`), `shellTextureInit` 8014E558
(`engine_hit_detection.c`, the same format, no caller in the game), the `tms()` readers of
`tools/check_car_groups.py`, the disc data (all 40 TMS files parse to their exact end).

| Off | Type | Field | Tag |
|---|---|---|---|
| +0x00 | u32 | magic 0x50535854 (bytes `TXSP`) | K (exit (-1) otherwise) |
| +0x04 | u32 | version 0x43 | K |
| +0x08 | u32 | build stamp, the same word as the paired DMD's +0x08 | K that it is returned to the caller (`settings->f20` -> `arenaResetSystems(f20)` -> `worldLoadStartObjects(flag)`, where it only decides whether a header word is read and dropped); I: any non-zero value is safe |
| +0x0C | u32 | image count N (0 allowed: `ARROWS.TMS`) | K |
| +0x10 | u32[N] | **image keys** | K: `bitmapSetKeyTable`; below |
| +0x10 + 4N | entries | N x { u32 size, a TIM of `size` bytes } | K: the parser advances `size >> 2` words, so `size` must be a multiple of 4 (TIM block lengths are) |

Each entry's TIM is a standard PlayStation TIM (K: `GsGetTimInfo(entry + 4)`): u32 0x10, u32 flags (bits 0..2 the mode:
0 4-bit, 1 8-bit, 2 15-bit; bit 3 a CLUT present), then with bit 3 a CLUT block (u32 length including its 12-byte
header, u16 x, y, w, h, the colours), then the pixel block (u32 length, u16 x, y, w (in 16-bit VRAM units), h, the
pixels). Colours are the PS1's 15-bit `S BBBBB GGGGG RRRRR`; 0x0000 is transparent, bit 15 marks semi-transparent
texels. Disc census: flags 8 (4-bit + CLUT) 2,362, flags 9 (8-bit + CLUT) 1,362, flags 2 (15-bit, no CLUT) 47.

**Fixed mode (flag 0: arenas, shell screens)**. K, `bitmapRegisterTim`: `LoadImage` of the pixels and the CLUT at the
TIM's own x / y / w / h, the rectangles marked used. Nothing checks for overlaps: the author chooses the VRAM. Reserved:
(0, 0, 640, 256) the frame buffers and (960, 256, 64, 32), (960, 288, 64, 128), (960, 416, 32, 96) (`bitmapInit`
80121550: fonts, HUD); retail arenas use x 640..1023 of rows 0..255 and x 0..~420 of rows 256..511, leaving the rest
for the cars (`SANTA.md` 3.1).

**Relocatable mode (flag n: cars)**. K, `vramPlaceTexture` 801209E8:

1. `bitmapFindRecord(key)`: if an earlier image (the arena's or another car's) has the same key, the image is **not
   uploaded**; the slot takes that image's place and CLUT. Keys are how arenas share effect frames with cars (e.g.
   Roadkill's 0x9F7D..0x9F7F are in every arena TMS: `TEXTURE_BANKS.md` 1.4). **A converter must give new images keys
   that no other file uses**, or reuse a key on purpose to share (the port's `portCarGroupNoReuse` switches the
   sharing off per load group: `TEXTURE_BANKS.md`).
2. Otherwise it scans VRAM rows 256..511 (the first pass only from x 960, then everywhere) in steps of the image's own
   width / height for a free top-left texel (with `portCarOptions` bit 9: a free whole rectangle, `CAR_GROUPS.md`
   13.12), uploads, and for 4 / 8-bit images uploads the CLUT with `vramUploadClut(16 or 256, 1, ...)` 80120D3C into
   the first free 16- or 256-wide spot of one row. So **4-bit images need a 16 x 1 CLUT, 8-bit ones a 256 x 1 CLUT**
   (one palette per image; more palette rows in a TIM are not uploaded on this path).
3. The slot `vramTexSlots[n * stride + index]` gets {the tpage page bits, x, y, CLUT x, CLUT y} for the remap (2.7).
4. A failure prints "Relocatable texture not loaded" and leaves the slot stale (a wrong texture on screen).

Placement (K from the scan; I as advice): VRAM widths are pixels / 4 (4-bit), / 2 (8-bit), x 1 (15-bit); keep sizes that
tile (powers of two such as 8, 16, 32, 64 VRAM units wide, 32..256 high); an image must not cross a 64-unit
texture-page boundary (the remap uses `x % 64`; the scan's own step keeps aligned sizes inside a page;
`check_bank_model` reports "crosses a texture page"). Retail car TMS files hold 5..17 images (BB 31) of 21..167 KB;
after an arena the lower half's free space is about 540 x 256 VRAM units, shared by every car of the match.

---

## 4. The PC release's .DPC / .TPC

Files: `Game Files\CARSDB\XX.DPC/.TPC`, `LEVELDB\<STEM>.DPC/.TPC`, the shell's `SHELLDB1\*.TPC`. Sources: `SANTA.md`
1.2, 2.4, 2.5; the port's `tools\art\santa.py` and `tools\disc\tim.py`; scans of RK and GLACIER.

### 4.1 .TPC (K from data: `SANTA.md` 2.5 and the scans)

| Off | Field |
|---|---|
| +0x00 | magic bytes `TCPM` (u32 0x4D504354) |
| +0x04 | 0x43 |
| +0x08 | stamp (RK.TPC and RK.DPC share it, as on the PS1) |
| +0x0C | image count (RK 9, GLACIER 300) |
| +0x10 | entries: u32 size, TIM; **no key table** |

How it differs from a TMS:

- No keys: images cannot be shared by key (K by absence).
- Every image is **8-bit with one 256 x 1 palette** (RK, GLACIER: all); the PS1's 4-bit images became 8-bit at the
  same pixel size (RK image 1: PS1 16 x 64 VRAM units 4-bit = PC 32 x 64 units 8-bit, both 64 x 64 pixels). Some shell
  images are twice the PS1 resolution (`tim.py`); the car and arena images checked so far have the same size (I for the
  others).
- The palettes are **RGB555 `0RRRRRGGGGGBBBBB`** instead of the PS1's BGR555; the pixel indices are the same.
  Conversion: `ps1 = (c & 0x8000) | ((c & 0x1F) << 10) | (c & 0x3E0) | ((c >> 10) & 0x1F)` (K, checked on 3 shared
  images).
- TIM x / y are 0 for all images (no VRAM placement).
- TIM flags 0x09 or 0x19: **bit 4 (0x10) is U** (PC only; likely "has a transparent colour"). Write 0x09 in a PS1 TIM.
- The image counts and order differ from the PS1 file (RK 9 against 8; 48 of GLACIER's 300 are pixel-identical to PS1
  images). The PC -> PS1 image mapping is **U** (not tabulated; find it by comparing pixels, as `SANTA.md` 2.5 did).

### 4.2 .DPC (I unless marked)

| Off | Field |
|---|---|
| +0x00 | `DCPM`; +0x04 0x43; +0x08 stamp; +0x0C 0x80019C40; +0x10 shift 3 (K data) |
| +0x14 | 0x80019C60 (the root table pointer) |
| +0x18, +0x1C | 0 |
| +0x20 | root count (K data: RK 1, GLACIER 12) |
| +0x24 | root pointers |

- Pointers are absolute for 0x80019C40 as the PS1's are (K data: all followed correctly).
- The node headers (types 1, 2, 3, 4, 5, 8, 9, mesh 0) are byte-compatible with the PS1's (K for the types walked in
  RK and GLACIER: same ids, counts and child offsets; 44-byte meshes with the same fields). The trees differ in
  content (RK: 80 nodes against 99; other mesh splits).
- **The primitive records differ** (layout I, from comparing RK and GLACIER pairs):
  - the same 4-byte header (texture words, colour words, length, packet length) and the vertex indices at +4..+11;
  - +0x0C s16[3] a face normal (unit 4096; beams carry +-4096 here);
  - the PS1 control word's high half moves to +0x12 (bits 16..31 the same but bits 24..27, the normal offset, which
    changes because the record is longer);
  - +0x14 u32 **U** (values 0x2008, 0x2000, 0x4000, ...);
  - +0x18 the colour words with **red and blue swapped** (PS1 `ce 51 02 36` = PC `02 51 ce 36`);
  - then one **u16 u, u16 v pair per vertex** (fan order for quads: the PS1's corners = the PC's corners 0, 1, 3, 2;
    likely);
  - then a **u16 texture id**: 0x4000 | the TPC index in arena files, 0x5000 | the index in car files (RK
    0x5003..0x5008); the meaning of bit 0x1000 and of the u16 after it is **U**;
  - window-flagged records end with a GP0 0xE2 texture-window word (0xE200....) (I);
  - the normal indices after that for lit records (I).
  - So a PS1 FT4 record is 36 bytes where the PC's is 48; `SANTA.md` 2.4 maps one kind field by field.
- **Unknown, and how to find out**: the exact PC primitive layout per kind, the texture-id bits, the +0x14 word and
  TIM flag bit 4 need the PC EXE (`Game Files\TM2.EXE`, not decompiled here): find its loader by the `DCPM` / `TCPM`
  constants and read its primitive loop. A cheaper route: pair every PS1 primitive with its PC twin (same vertex
  indices, same mesh order where the trees match, as for RK) and fit the fields statistically.

---

## 5. Twisted Metal 1's files

**U**. Nothing in the decomp or its tools reads Twisted Metal 1 data, and neither repository has notes on its formats
(nothing of TM1 has been examined). The TM2 engine descends
from it (a "PSXD"-style magic and the same SingleTrac node interpreter are likely), so the first step is to look for
the `DXSP` / `TXSP` magics and the version word in a TM1 model file: if they match with an older version, the node types
of 2.4 are the place to start; if not, it is a reverse-engineering job of its own.

---

## 6. What a converter must produce (checklist)

For a **car** (a relocatable pair installed with group n; the port's `tools/art/santa.py` is a small worked example):

TMS:
- [ ] header `TXSP`, 0x43, stamp (the DMD's), count N (<= 20 retail, <= 32 with `TM2_PORT_CARS`), N keys **unique
      across every arena and car TMS** unless sharing is wanted;
- [ ] each entry: u32 size (a multiple of 4) + TIM; 4-bit with a 16 x 1 CLUT, 8-bit with a 256 x 1 CLUT, or 15-bit
      without a CLUT; PS1 BGR555 colours (0x0000 = transparent); any TIM x / y (ignored);
- [ ] image sizes that tile and fit one texture page (<= 64 VRAM units wide, <= 256 high), in total small enough to
      share the lower half of VRAM with the other cars (and, for car select, to fit the free 128 x 256 block at
      (832, 256): `DARK_TOOTH_PLAYER.md` 9.3).

DMD:
- [ ] header `DXSP`, 0x43, stamp, 0x80019C40, shift 3, 0x80019C58, the root count, the roots; the first node after
      the table;
- [ ] every pointer = 0x80019C40 + file offset, inside the file; byte +1 of every node != 0xB1;
- [ ] the first root a type 1 group with id = car type * 16 and a correct bounding sphere, then the sub-object ids
      the original car file has (dump it with `dbscan_ids()`);
- [ ] only node types 0, 1, 2, 3, 4, 5, 8, 9, 11, 12; every node reachable as a child (nothing only through select
      +0x0C or animation +0x1C);
- [ ] meshes: the 44-byte node, the vertex array (s16 x, y, z, 0), the normal array (s16 x, y, z, 0; length 4096)
      pointer valid even when unused, +0x14 = 0x00010000, the flags, a bounding sphere around the mesh, one primitive
      list per mesh;
- [ ] primitives: header bytes, quads in Z order, a consistent control word (kind, lighting, stride, normal offset,
      colour count, bit 31 for double-sided), the first colour word with the matching GPU code, texture words with
      tpage = image index | abr << 5 | depth << 7, CLUT 0, UVs relative to the image;
- [ ] LOD bands (squared world-unit distances) if wanted; hit boxes (type 8) so other cars and weapons can hit it.

For an **arena** (a fixed pair at 0x80019C40):
- [ ] the same DMD rules, but no relocation and no remap: final tpage / CLUT / UV values in the texture words,
      pointing at the VRAM spots written in the TMS's TIM headers;
- [ ] the TMS images placed by the author outside the reserved rectangles, leaving room for the cars' textures and
      CLUTs (rows 256..511 and free CLUT rows);
- [ ] the DMD's size plus every car's DMD within 737,300 bytes (retail layout), less 2 KB of slack for the sector reads;
- [ ] the level's typed objects (id 1, pickups 4xx, targets 5xx, events 3xxx / 4xxx / 5xxx) as the arena's code
      expects them.

From the **PC files**: TPC -> TMS = add a key table, swap the palettes' red and blue, optionally go back to 4-bit;
DPC -> DMD = keep the nodes, rewrite every primitive into the PS1 record (2.6), map the texture ids to TMS indices
(cars) or to VRAM tpage / CLUT / UV (arenas).
