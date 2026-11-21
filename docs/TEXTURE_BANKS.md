# Texture banks: every car's textures in its own VRAM

Answers item 20 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(give each car a VRAM-sized "bank" of its own, so that 16 cars fit on every
arena, every car can be preloaded, and each car can carry its own team paint).
Builds on, and does not repeat: `TEAM_COLORS.md` (what each car TMS holds,
palette rows, `modelRemapTextures`, the CLUT animation on node +8),
`SIXTEEN_CARS.md` 3 and 4 (the load function, the placement algorithm, the
slot table, VRAM room per arena), `SURVIVAL.md` 2 and 4 (one DMD copy per
car, tables keyed by type, Dark Tooth's slot stride), `SANTA.md` 1 (how a
DMD is loaded and relocated) and `RENDER_ANSWERS.md` R1 (the emitter, the
near pass, debris and sprite drawers).

Static analysis only: the C in `src/semantic`, the pure-C references in
`reference/` (walker, emitter, near pass), a `jal` census of the retail EXE
(`bin/SCUS_94306R.EXE`), and read-only parsers run on the USA disc
(`cdimage/`; scripts in the scratchpad `req20/`). Nothing was run. Addresses
are retail (USA).

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code or data, not traced to the end or not seen running.

**Built (request 32 step 4, `TM2_PORT_TEXTURE_BANKS`): section 6 "As built"** has what was
implemented, the port's API and words, the PSY-Z protocol step by step, the limits and the
proofs. Sections 1-5 are the analysis it follows.

## Short answer

- **Uploads are easy to attribute.** Every car texture and CLUT reaches VRAM
  through two `LoadImage` calls: `vramPlaceTexture` (pixels, jal 80120B98)
  and `vramUploadClut` (its CLUT, jal 80120E4C). Both get the load group n
  as an argument (1 = first human ... humans, then opponents); the arena
  (group 0) goes through the two `LoadImage` calls of `bitmapRegisterTim`
  (80121988, 801219E0) at the TIMs' fixed positions. Nothing else uploads
  car textures during a match. CONFIRMED (section 1).
- **A car does not always own its images today.** `vramPlaceTexture` first
  looks the image's key up in the records of the level and of the cars
  loaded before; on a hit it only copies the position. Besides the six part
  images shared between cars, the level TMS of every arena holds 7 images
  of Roadkill and Dark Tooth by key, and some arenas hold the shared turbo
  flame, shadow or undercarriage. On Suicide Swamp, Cyburbia and Roof Tops
  those 7 level copies have **different palettes** from the car's own, and
  retail draws the level's. CONFIRMED (disc, section 1.4).
- **Draw side: the cleanest tag is written at load, into the primitives.**
  `modelRemapTextures` writes each car primitive's CLUT and tpage page bits
  from that car's slots, into that car's own DMD copy. It never touches
  tpage bits 9..15, which are 0 in every textured primitive of all 15 car
  DMDs and all 14 level DMDs. If the port's remap also writes the group
  there, every GPU packet built from a car primitive carries its bank: the
  emitter, the near pass and the debris drawer copy the texture words as
  whole words, and put only the low byte of the tpage into the DR_MODE
  packets. Everything else (arena, HUD, effect sprites, shell) has 0 there.
  No hook on the draw path is needed. CONFIRMED (section 2.4).
- **The object pointer is not a usable tag.** The car's shadow node is
  drawn as a world model under the root object; the freeze ice is a level
  mesh hung under the car; projectiles, debris chunks and debris bursts use
  car meshes under other objects or none. The **mesh address** works (each
  car's DMD copy is one contiguous range, recorded at load), and so does the
  tpage tag; the object does not. Section 2.
- **Shared part images: a copy in every car bank.** Their content is
  identical in all cars (checked), so each bank gets its own; where the
  arena's TMS has the key, copy the arena's rectangle (pixels and CLUT) into
  the car bank at the arena's coordinates, which keeps the retail look on
  the three arenas with different palettes. Section 3.
- **Other VRAM readers during a match:** only the CLUT animations
  (`StoreImage` of one CLUT row at the start of a fade, cycle or sweep;
  `LoadImage` of the row each step). The car one is the invisibility fade on
  the +8 node: it must go to the bank of the car it was registered for. No
  `MoveImage` touches textures (only frame-buffer copies), no `ClearImage`,
  no `DR_MOVE`/`DR_LOAD`. The HUD has no car pictures. CONFIRMED (section 4).
- **CLUT sharing today:** two cars of one type share every CLUT (reuse by
  key); car CLUTs are shared with the arena only for the keys of 1.4; the
  six part images are shared by all cars that have them. With banks nothing
  is shared, and team paint becomes "recolour the body CLUT in place in that
  car's bank" (`TEAM_COLORS.md` way C, per car, no extra rows).

## 1. Upload side

### 1.1 The order during `sessionLoadArenaAndCars` (8012BAFC, `shell_main.c`; CONFIRMED)

| Step | Call (jal VA) | VRAM effect | Belongs to |
|---|---|---|---|
| 1 | `screenDisplayToDisplay(1, 0)` 8012BB28 -> `MoveImage` 80121D80 | copies one 320x240 frame buffer to the other (loading screen) | frame buffers |
| 2 | `bitmapInit` 8012BB30 | bookkeeping only (`SIXTEEN_CARS.md` 4.1) | - |
| 3 | `bitmapParseContainer(levelTms, 0)` 8012BB58 -> `bitmapRegisterTim(tim, 0, i)` -> `LoadImage` **80121988** (pixels), **801219E0** (CLUT) at the TIM's own x/y; `bitmapAddRecord` 80121A08 | every level image | arena (group 0) |
| 4 | `fontPrintCenteredXY` "Loading" | draws into the frame buffer | frame buffers |
| 5 | per human i, then per opponent j: `bitmapParseContainer(carTms, n)` 8012BBC8 / 8012BC28 -> `bitmapRegisterTim(tim, n, i)` -> `vramPlaceTexture(info, i, n)` 80121954 -> `LoadImage` **80120B98** (pixels) and `vramUploadClut(16 or 256, 1, clut, i, n)` 80120C18 -> `LoadImage` **80120E4C** (CLUT); then `bitmapAddRecord` | the car's images not found by key | car group n |
| 6 | `screenDisplayToDisplay(0, 1)` 8012BC4C | frame buffer copy back | frame buffers |
| 7 | level DMD, `arenaResetSystems` -> `clutAnimInit` 80128D30, `worldLoadStartObjects` -> `worldLoadObject` | ids 5000/5001 start CLUT cycles: `clutAnimStartCycle` -> `gpuStoreImageLine` (`StoreImage` 8014DD54) of a level CLUT row; 5100 registers sweeps | arena |
| 8 | per car, same order and same n: car DMD at 0x80019C40 + off, `assetInstallImage(buf, n)` 8012BCD8 / 8012BD40 -> `modelRelocate` -> `modelRemapTextures` 801210A8 (primitives) -> `worldLoadObject` (+8 id: `clutAnimRegister(obj, 2)`, a definition only, no VRAM access) | none (RAM only) | car group n |
| 9 | `bitmapDisableTracking` | - | - |

So: arena first (all of it), then the cars' textures by group 1..N, then the
arena model, then the cars' models by group 1..N. Groups are a plain counter:
n = 1 + human index for humans, n = 1 + humans + opponent index for
opponents, the same numbers in steps 5 and 8. CONFIRMED.

### 1.2 How each upload tells whose it is (CONFIRMED)

| Upload | Group | Image | Other identification |
|---|---|---|---|
| `bitmapRegisterTim` `LoadImage` 80121988 / 801219E0 | its 2nd argument `flag` is 0 (the arena). `vramTexSlotGroup` is **stale** here (it is not set for group 0; it keeps the last car's n from the previous load) | 3rd argument = index in the TMS | rect = the TIM's own x/y; key = `bitmapKeyTable[index]` (`bitmapSetKeyTable`, the container's key list) |
| `vramPlaceTexture` `LoadImage` 80120B98 | 3rd argument (`row`) = n; also stored into `vramTexSlotGroup` (80181050) at 80120A4C, so the global is right from here on | 2nd argument (`col`) | slot `n * 20 + col` (80120A38..80120A50); key `bitmapKeyTable[col]`; car type = `settings->human_car[n - 1]` or `opponent_car[n - 1 - humans]` |
| `vramUploadClut` `LoadImage` 80120E4C | 5th argument (`row`) = n | 4th (`col`) | slot `n * 20 + col` (80120D84..80120D8C); width 16 or 256 = 4-bit or 8-bit image |
| `modelRemapTextures` (RAM, not VRAM) | `vramTexSlotGroup`, set by `assetInstallImage` at 80120F1C | tpage bits 0..4 of each primitive | the DMD copy at `buf` = 0x80019C40 + off, size = file size |

The TMS pointer does not identify anything: every TMS (level and cars) is
read into the same buffer 0x80019C40. The DMD pointer does: each car's DMD
copy is a separate range (`SIXTEEN_CARS.md` 3.1-3.2).

`vramPlaceTexture` returns early, with **no upload**, when
`bitmapFindRecord(key)` finds an earlier image with the key (level or
car); the slot then holds that image's position. `bitmapAddRecord` still
appends a record for the new image (with the reused position). CONFIRMED.

### 1.3 Every other `LoadImage` / `StoreImage` / `MoveImage` caller in the EXE (CONFIRMED, `jal` census)

| Function | jal | When | What | Bank |
|---|---|---|---|---|
| `fontInit` (80113F30) | `LoadImage` 8011413C, `Load4BitClut` 80114178 (-> `LoadImage` 8014DCB4) | `shellInitMachine` 80128C18, `sessionRun` 8012BEC8 (before the arena load) | font images and CLUTs at x 960 (reserved by `bitmapInit`) | arena |
| `screenLoadCardImages` (8012A614) | `LoadImage` 8012A730 (art), 8012A784 (CLUT) | opponent-card screen, before the arena load (`SIXTEEN_CARS.md` 6) | `SCREEN\<code>PLATE.TIM` of opponent i (car `sessionGetOpponentCar(i)`), at the card's spot | none: overwritten by the arena load |
| `screenDisplayImage` (80121C8C) | `LoadImage` 80121D08 | shell screens, card background `SHOWOPS<level>.TIM`, title, credits | full-screen pictures into a display buffer | frame buffers |
| `screenDisplayToDisplay` (80121D38) | `MoveImage` 80121D80 | load screen (1.1), `pauseMenu`, `screenLostALife`, shell screens | frame buffer to frame buffer | frame buffers |
| `moviePlayEnding` (8012B898) | `MoveImage` 8012BA90 | movies (title, endings, history) | frame buffers | frame buffers |
| `movieStrCallback` (801380D8) | `LoadImage` 80138120 | movies | decoded slices | frame buffers |
| `gpuLoadImageLine` (8014DCEC) | `LoadImage` 8014DD10 | `clutAnimStepSweep/Fade/Cycle` every frame of a running CLUT animation | one CLUT row | arena, or a car for the +8 fade (section 4.1) |
| `gpuStoreImageLine` (8014DD30) | `StoreImage` 8014DD54 | `clutAnimStartSweep/FadeOut/Cycle` | reads one CLUT row (the original, to restore later) | same |
| `shellLoadTmsDmd` (80129010) | `bitmapParseContainer(buf, 0)` 80129084 | every shell screen via `sceneLoadTmsDmd` (CARSEL, AICARSEL, LEVELSEL, ...) | fixed TIM spots (group 0 path) | shell |
| `LoadTPage`, `LoadClut` | `LoadImage` 80143CFC, 80143D74 | only `FntLoad` (debug font, no game caller found) | - | - |
| `screenRamToVram` (80121D98), `timIntoVRAM` (8014E4C8) via `shellTextureInit` (8014E558) | `LoadImage` 80121DE4, 8014E500/8014E540 | no `jal` caller (LIKELY dead) | - | - |

`ClearImage` has no game caller, and `SetDrawMove` / `SetDrawLoad` are not
linked, so no VRAM copy is queued in an OT (LIKELY: a hand-built GP0 0x80
packet was not searched for).

### 1.4 Images a car takes from the arena or from another car (CONFIRMED, disc)

Reuse by key (1.2) makes a car's primitives point at VRAM it did not
upload:

| Key (car image) | In which car TMS | Also in the level TMS of | Level copy identical to the car's? |
|---|---|---|---|
| 0x9941 turbo flame | all 15 | Los Angeles | yes |
| 0xAA9A shadow | 14 (not Mr. Grimm) | Paris, Paris 2P, Hong Kong, Cyburbia | yes |
| 0x9201 undercarriage | HH, OT, MG, IR, AX, FL | New York, Hong Kong | yes |
| 0x9F7D, 0x9F7E, 0x9F7F (Roadkill images 7, 6, 5; asset 2321, his special) | RK | **every arena** | yes, except **Suicide Swamp, Cyburbia, Roof Tops**: same pixels, 15 of 16 CLUT entries differ |
| 0x9BDE, 0x9BE0, 0x9BE2, 0x9BE4 (Dark Tooth images 22..25) | BB | **every arena** | yes, except **Suicide Swamp, Cyburbia, Roof Tops**: same pixels, 8..14 CLUT entries differ |
| 0x9BCF smoke, 0x1FCD5 undercarriage, 0x9609 red light | 15 / 8 / 2 cars | none | - |

All 32x32 or 64x32 4-bit images. Between cars, all six shared keys have one
content (checked across every car that holds them). So in retail:

- the first car of a type uploads; every later car of that type, and every
  later car holding a shared part, points at the first one's VRAM;
- a car whose image is in the level TMS points at the arena's copy, and on
  the three arenas above Roadkill's special and Dark Tooth's four images are
  drawn with the arena's palette (these keys look like generic effect frames
  that the arenas recolour; LIKELY).

### 1.5 Dark Tooth's 31 images (CONFIRMED, `SURVIVAL.md` 4.2)

The slot stride is 20 and Dark Tooth's images go to 30, so a car parsed after
him overwrites his slots 20..30 before his DMD is remapped. Banks do not
change this: the slot table is shared bookkeeping, not VRAM. Since the port
replaces `vramPlaceTexture` for banks anyway (section 3), the simplest fix is
stride 32 in the four functions listed in `SURVIVAL.md` 4.2
(`vramPlaceTexture`, `vramUploadClut`, `bitmapAddRecord`,
`modelRemapTextures`); the slot table holds 49 groups at that stride.

## 2. Draw side

### 2.1 How a car's primitives reach the packet buffer (CONFIRMED, `reference/renderer_walker_engine_render_scene.c`, `reference/model_emitter_800CDC54.c`)

`renderScene` (80152284) does not emit per object. It walks every model into
one mesh queue, then calls the emitter once:

1. `renderWalkModel(obj, model)` (801526CC, asm in retail) for: the level
   model under the root object (`csGetRoot()`, model at +284); the 20
   `renderWorldModels` entries, **also under the root object**; the view's
   follow object; then every object of the cs list with +8 set (drawable)
   and +292 set.
2. Each mesh node (type 0) that passes the culling appends a 32-byte entry
   to `renderMeshList` (801AC810, up to 280): **[0] mesh node pointer**,
   [1..3] translation, [4] shift, **[5] the object passed to the walk**,
   [6] matrix slot, [7] object number.
3. `sub_800CDC54(db, renderMeshList, count, ...)` (the emitter) runs over
   the entries in order. For each primitive record of the mesh (+0x0C list,
   +0x10 count) it writes one GPU packet at the cursor; the texture words
   (uv0+CLUT, uv1+tpage, uv2, uv3) are **copied as whole words from the
   record** (`W32(dst) = W32(src)`). Near polygons are deferred with
   `{record pointer, packet pointer, tpage, sort z}`.
4. `renderNearPolys` splits the deferred packets; the new pieces take the
   CLUT halfword from vertex 0 and the **tpage halfword from vertex 1 of the
   parent packet** (whole halfword).

`renderDrawModel` (80151378: car select, menus, endings) and
`renderDrawObjectUnculled` (801514B8: debris bursts) use the same walker.

### 2.2 What draws car textures, and what identifies the car there

| What | How it is drawn | Mesh comes from | Object at emit time ([5]) | Tag by object? | Tag by mesh address / tpage bits? |
|---|---|---|---|---|---|
| Car body, damage stages (+1), tires (+2), wreck (+5), turbo flame and smoke (+6), far LOD (+8), parts (Hammerhead, Mr. Slam arm/claw, Axel, Minion, Dark Tooth) | car cs object (`dbInitCsForAModel` in `worldLoadObject` id type+0) in the cs list | the car's DMD copy | the car object | yes | yes |
| Shadow (+3, `carBodyObjects`, posed by `carBodyPoseUpdate`) | `renderWorldModels` (added by `carBodyObjectShow`) | car DMD (one per type today, `SURVIVAL.md` 2.2) | **the root object** | **no** | yes |
| Freeze ice (+7, `carFreezeBuild`) | node records in BSS (`carFreezeObjects` 8018F950) linked into the car model; their children are the level's object 692 | **level DMD** (arena textures) | the car object | **no** (would pick the car bank for arena texels) | yes (it is arena: bank 0) |
| Debris chunks (`debrisBreakModel` 8014B504) | 8 own cs objects (`debrisChunks`), model = a child of the wreck model | the wreck of `carWreckModelGet(type)` = **the first car of that type** | the chunk object | no | yes (but it is the first car's bank: fix with per-group wrecks, `SURVIVAL.md` 2.5) |
| Debris bursts (`debrisStartBurst` 8014BAD4, drawn by `fxDebrisDraw` 8014C19C) | `renderDrawObjectUnculled(obj)` walks the car object once; `lightAddSources` keeps up to 16 mesh pointers in the burst slot; each frame `fxDebrisDraw` emits them with libgte | the car's own model (or `aiDarkToothBossModel()`) | none (not the emitter) | no | yes: texture words copied as words (`*d = *tex++`), only the low byte of the tpage goes to the DR_MODE word (`((u8 *)pkt)[4] = uv`). Mesh source LIKELY (field names in `lightAddSources` are generic) |
| Projectiles and specials (asset ids 2301, 2308, 2309, 2321..2327, 2214, 249, 309) | `weaponStoreProjectileModel` / `weaponAssetBind` keep **one model pointer per id**; fired projectiles are cs objects with that model | the DMD of the **last installed car** holding the id (2301 is in ST and BB) | the projectile object | no | yes; consistent, because the coordinates in the mesh are that DMD's own (section 2.3) |
| Effect sprites (explosions, smoke, sparks, flames: `fxSpritesDraw`, `fxImpactParticlesDraw`) | 2D POLY_FT4 built from sprite rects | level objects 650..662, 670..673, 687 (`explodeStore*Animation`) | - | - | arena: no car DMD holds an id below 2000 except the car ids, 249 and 309 (CONFIRMED, id census of all 15 car DMDs) |
| HUD (weapon icons, digits, radar arrows, armor icon) | 2D sprites | level objects 1260, 1290..1293, 669; fonts | - | - | arena |
| Car select carousel (`sceneObjDraw` -> `renderDrawModel`) | shell scene | CARSEL.DMD / AICARSEL.DMD, never remapped, absolute words (`TEAM_COLORS.md` 5) | the scene instance | - | shell (bank 0) |

### 2.3 Why the mesh (or what the remap wrote into it) is the right key

A primitive's CLUT and tpage page bits are written once, by
`modelRemapTextures`, from the slots of the group whose DMD copy holds the
primitive (`assetInstallImage(buf, n)` sets the group, then remaps every mesh
of `buf`). So "which VRAM coordinates this primitive samples" and "which DMD
copy it lives in" are decided by the same code at the same time. Any draw
path that uses that primitive (car object, world model, chunk, burst,
projectile) samples that group's coordinates, whatever object it is drawn
under. Tagging by object breaks in four rows of 2.2; tagging by mesh never
disagrees with the coordinates. CONFIRMED (code paths).

### 2.4 Proposed scheme: the bank in tpage bits 9..15, written by the remap

In the port's `modelRemapTextures` (the C in `world_collapse_warps.c`), next
to the page-bits write:

```c
((UVTpWord *)w)->tpage = vramTexSlots[n].page;      /* retail: bits 0..4 */
((UVTpWord *)w)->rest  = bank;                      /* new: bits 9..15 = vramTexSlotGroup */
```

and in the port's GPU (PSY-Z): for a textured polygon, `bank = attr >> 9`,
texpage = `attr & 0x1FF`; sample VRAM bank `bank` (0 = the normal VRAM).

Why it holds (all CONFIRMED unless tagged):

- **Free bits.** The remap's bit field is 5 bits (`UVTpWord.tpage`); abr,
  depth and bits 9..15 (`rest`) are kept from the file. Bits 9..15 are 0 in
  all 7,604+ textured primitives of the 15 car DMDs and in all primitives of
  the 14 level DMDs, CARSEL, AICARSEL and LEVELSEL (`req20/bits.py`). The CLUT
  halfword's spare bit 15 is **not** usable: `clutAnimRegister` computes the
  CLUT row as `clut >> 6` without a mask.
- **Every copy keeps the halfword:** the emitter (word copies), the near
  pass (tpage halfword from vertex 1 into every piece), `fxDebrisDraw` (word
  copies).
- **No DR_MODE gets the bank:** the emitter (`cur[4] = tp`), the near pass
  (`((u8 *)d)[4] = SPAD_TPAGE`) and `fxDebrisDraw` (`((u8 *)pkt)[4] = uv`)
  store only the low byte into the E1 word. (The E1 word never had bit 8
  either; retail behaviour.)
- **Readers of the field:** `clutAnimRegister` takes `(tpage >> 7) & 3` (depth
  only). No other code reads a car primitive's texture words (the sprite
  parsers read level objects only, 2.2). LIKELY complete: the census covered
  every reader of mesh primitive lists found in the C.
- **Everything else is bank 0:** level primitives are absolute and never
  remapped; packets built at run time use `GetTPage` (bits 0..8 only),
  `SetDrawMode`, or tpage values parsed from level objects.
- **Room:** 7 bits = 128 banks (bit 11 is texture-disable on real hardware;
  the port's GPU must just not honour it, the game never enables it, LIKELY).

Nothing has to be hooked at draw time, split screen and the mirror redraw
work unchanged, and the near pass and PGXP-style shadows are unaffected.

### 2.5 Alternatives (if the port does not want to touch the remap)

1. **Mesh address ranges.** Record `[0x80019C40 + off, + size)` per group in
   `sessionLoadArenaAndCars` (step 8) and the level range
   `[0x80019C40, + level size)`. Look up entry [0] of each `renderMeshList`
   entry before the emitter (the reference C emitter can take it per entry),
   the record pointer of each near-list entry in `renderNearPolys`, and each
   burst piece's mesh in `fxDebrisDraw`. Needs three hooks and a packet
   range per mesh; gives the same answer as 2.4.
2. **Packet address ranges.** The emitter only writes the cursor back at the
   end (`ctx->cur`), so per-entry ranges need an emitter change anyway; the
   near pass appends its pieces after all entries. Not recommended.
3. **Object pointer.** Wrong for the shadow, the freeze ice, debris and
   projectiles (2.2). Not recommended.

## 3. Placement in banks (proposal)

### 3.1 What goes where

| Bank | Holds | Written by |
|---|---|---|
| 0 | arena TMS, fonts, frame buffers, card art, shell screens, movies, everything 2D | everything that is not a car group (1.3) |
| n (1..N) | car group n's images and CLUTs | the port's `vramPlaceTexture` / `vramUploadClut` for group n; the +8 CLUT fade of group n; team paint |

### 3.2 Per-bank placement that keeps the retail look

In the port's `vramPlaceTexture(img, col, n)`:

1. **Usage bitmap per bank,** each a copy of the bitmap right after the level
   TMS (`bitmapInit` + level). A car's spots then never coincide with an
   arena spot, and a single car always fits (largest: Dark Tooth, 79,616
   texels, 26 CLUTs; the lower half alone is 262,144). LIKELY.
2. **Look up the level's records only** (records added with group 0). On a
   hit, keep the retail slot (the level's coordinates) and copy that
   rectangle and its CLUT row from bank 0 into bank n (port-side copy). This
   reproduces the retail look for the keys of 1.4, including the three
   arenas whose palettes differ.
3. **Ignore records of other cars.** Place and upload the image into bank n,
   CLUT included. Two cars of one type then each have their own copy, as do
   the six shared parts (identical content, 1.4); cost 3,584 texels per
   bank.
4. Write the slot as retail (stride 32, 1.5), and record group n, bank n,
   and the DMD range for the port.

The six shared part images therefore live in every car bank that uses them
(or are copied from the arena where the arena has the key). A common "parts
bank" is not needed: nothing outside a car's own meshes samples them (the
effect sprites use level frames, 2.2).

### 3.3 Consequences

- **16 cars on every arena:** each bank has a whole VRAM; the per-arena
  limits of `SIXTEEN_CARS.md` 4.4 and `SURVIVAL.md` 4.1 no longer apply.
- **Preloading** (a random Juggernaut, waves): any number of groups, each
  with its DMD copy and its bank; groups that are not on the field cost only
  memory.
- **Team paint per car, duplicates included:** recolour the body CLUT at
  slot n*20+0 in bank n (`TEAM_COLORS.md` 4 for the entries). No new rows,
  no primitive change.
- **Load-time bookkeeping** (slots, records) still dies with the first frame
  (`TEAM_COLORS.md` 1.4): keep group -> bank, the body CLUT position and the
  DMD range in port variables.

## 4. Other readers and writers of car VRAM

### 4.1 CLUT animations (CONFIRMED; `engine_clut_anim.c`)

| Animation | Started by | Reads | Writes | Whose CLUT |
|---|---|---|---|---|
| cycle (types 0, 3) | `clutAnimRegister` of level ids 5000, 5001, at load | `StoreImage` row (8014DD54) | `LoadImage` row each step | arena |
| sweep (type 1) | `weaponFlameUpdate` 8010AD6C, `groundFiresUpdate` 8010CA54, keyed (5100, k) | same | same | arena (level flame markers) |
| fade (type 2) | `weaponInvisibilityUpdate` 8010DE54: `clutAnimStartFadeOut(type + 8, 0, 20)` at 8010DECC, `clutAnimFadeIn` at 8010DF1C | same | same | **car**: the CLUT of the first primitive of the first-installed car of that type's +8 mesh (`TEAM_COLORS.md` 2.5) |

For banks: give the definition the group (`vramTexSlotGroup` is n while
`worldLoadObject` runs inside `assetInstallImage(buf, n)`), key it by group
instead of by type (today a second car of the type is refused by
`clutAnimNewDef`), route its `StoreImage`/`LoadImage` lines to bank n, and
pass the invisible car's group from `weaponInvisibilityUpdate`. For Outlaw 2,
Mr. Grimm and Thumper the first +8 primitive is untextured and the definition
points into the frame buffer (retail fades junk there); route those to bank
0 or skip them.

### 4.2 Nothing else (CONFIRMED by the census of 1.3)

- `StoreImage`: only the CLUT animations. No screenshot or card capture
  reads VRAM; `screenCardSettle` copies a primitive, not pixels.
- `MoveImage`: frame-buffer copies only (load screen, pause menu, lost-a-life,
  movies).
- HUD: no car pictures. The target panel prints the name (`carTypeName`) and
  a bar; the radar plots coloured blips by type (`hudRadarPlotCar`); the
  sprites come from level objects (2.2).
- Opponent cards and car select: their own images, before the arena load or
  in the shell (bank 0), never the battle CLUTs (`SIXTEEN_CARS.md` 6.3,
  `TEAM_COLORS.md` 5).

### 4.3 CLUT sharing today (CONFIRMED)

| Between | Shared? |
|---|---|
| two cars of one type | everything: the second car's slots copy the first's positions (`TEAM_COLORS.md` 1.3) |
| different cars | the six part images (first car that has the key uploads) |
| a car and the arena | the keys of 1.4 (0x9941 on Los Angeles; 0xAA9A on Paris, Paris 2P, Hong Kong, Cyburbia; 0x9201 on New York, Hong Kong; Roadkill's 3 and Dark Tooth's 4 effect images on every arena) |
| body CLUTs | never with the arena or another type; all 256-colour body palettes differ (`SIXTEEN_CARS.md` 4.6 item 4) |

## 5. Recipe for the port

1. `sessionLoadArenaAndCars`: bank 0 as today for the level TMS; per car
   group n, allocate bank n, place with 3.2, slot stride 32; keep
   group -> {bank, DMD range, body CLUT position}.
2. `modelRemapTextures`: also write `bank` into tpage bits 9..15 (2.4).
3. PSY-Z: textured polygons sample bank `attr >> 9`, texpage `attr & 0x1FF`;
   CPU-side `LoadImage`/`StoreImage` go to bank 0 unless the caller is one of
   the bank-aware paths (3.2 uploads, the +8 fade, team paint).
4. CLUT fade per group (4.1); per-group wreck and shadow registries
   (`SURVIVAL.md` 2.5) so duplicates' debris and shadows use their own bank.
5. Team paint: recolour in place in the car's bank (3.3).

## 6. As built (request 32 step 4, `TM2_PORT_TEXTURE_BANKS`)

Built on 2026-10-12 as request 32 step 4 (`CAR_GROUPS_PLAN.md` 4, `CAR_GROUPS.md` 9), with the port's answers
of `PER_CAR_PLAN.md` G (G.1, G.10, G.15). Sections 1-5 above are the analysis it follows; where the build
differs from the proposal of 3.2 and 4.1 it says why (6.2). Tags as above; nothing was run.

### 6.1 Summary

- **Define:** `--port-define TM2_PORT_TEXTURE_BANKS` on a port variant build; needs `TM2_PORT_CAR_GROUPS`
  (`#error` otherwise: the fades and the banks follow load groups and car keys). Works with and without
  `TM2_PORT_CARS` and `TM2_PORT_CAR_SWAP`. Without the define every changed line is the retail C
  (`#ifdef TM2_PORT_TEXTURE_BANKS ... #else <retail> #endif`).
- **Run-time switch:** `portCarOptions` bit 5 (`TM2_PORT_CAR_OPT_TEXTURE_BANKS` = 0x20,
  `include/semantic/tex_banks.h`), **latched at each load** by `bitmapInit` (the first call of
  `sessionLoadArenaAndCars`). Bit 4 stays free for request 33-3 (engine sound, `PER_CAR_PLAN.md` D7).
  With the bit clear the define runs retail's logic (6.6).
- **With the bit set** every car load group n gets a bank of its own (the lowest free bank, so group n
  = bank n in a normal load) on its first image. Everything of the group goes there: pixels and CLUTs,
  placed with retail's scan in a fresh bank. Nothing is shared between cars any more (no reuse by key
  from another car, so two cars of one type have two copies); an image whose key the **arena's** TMS
  holds keeps the arena's coordinates in **bank 0**, exactly as retail (1.4).
- **Tag:** the slot records the bank in its tpage word bits 9..15; `modelRemapTextures` writes them
  into each primitive's tpage bits 9..15 (2.4); every packet built from a car primitive carries it.
- **Uploads:** the game says the bank of each CPU-side `LoadImage` / `StoreImage` in a port word,
  `portVramUploadBank`, set just before the call and reset to 0 after its `DrawSync` (6.4).
- **Fades:** the invisibility fade is one definition per car key (= per car), and its `StoreImage` /
  `LoadImage` lines go to the bank of the CLUT it fades (4.1).
- **No reuse:** `portCarGroupNoReuse[group]` (the port sets it, e.g. for a mod car): the group neither
  takes an image by key nor gives one, banks or not (`PER_CAR_PLAN.md` E.1, G.10).
- **Mid-match:** a group's bank is allocated and freed on its own (`portTexBankAlloc` /
  `portTexBankFree`); banked placement needs no VRAM bitmap (6.7).

### 6.2 Decisions (and where they differ from 3.2 / 4.1)

1. **Arena keys stay in bank 0 instead of a copy in the car's bank** (3.2 item 2 proposed copying the
   arena's rectangle and CLUT into bank n). The slot keeps the record's tpage (no bank bits), so the
   primitive samples the arena's live copy in VRAM. Same look as retail on every arena (Suicide Swamp,
   Cyburbia and Roof Tops' own palettes included), no copy for PSY-Z to do, and an arena CLUT that
   animates is followed (the open question of "Not checked"). Cost: these images (Roadkill's 3 and
   Dark Tooth's 4 effect frames everywhere; turbo flame, shadow, undercarriage on 1-2 arenas each) are
   shared with the arena and with every car that has the key, as in retail.
2. **A fresh bank holds only `bitmapInit`'s reservations** (frame buffers (0,0,640,256), fonts at x 960)
   instead of a copy of the level's VRAM bitmap (3.2 item 1). One group fills a bank, so nothing else
   has to be avoided; the coordinates stay in the areas retail uses for car textures (lower half for
   pixels, x 640.. of the upper half for 256-colour CLUTs); no 64 KB bitmap per bank is needed (the port
   sections have about 29 KB left with every define, `CAR_GROUPS.md` 8.8), and a mid-match load needs
   no VRAM bitmap at all. The test is on whole rectangles (retail tests the top-left texel only), in a
   list of the rectangles taken in the bank being filled (`portTexBanks.rects`, 72 entries).
3. **The bank is per slot, not per group:** a banked group's slots hold its bank, its arena-key slots
   bank 0. `modelRemapTextures` copies the slot's bits, so the tag always equals the VRAM the slot's
   coordinates point into (2.3).
4. **The fade's bank comes from the primitive** it was registered on (its tpage bits 9..15 after the
   remap), carried to the line helpers in `def->y` bits 9..15 (a CLUT row is below 512, so those bits
   are free). An untextured first primitive (Outlaw 2, Mr. Grimm, Thumper, 2.5 of `TEAM_COLORS.md`)
   keeps bank 0: retail's junk row in the frame buffer, never a row of another car's bank.
5. **The fade id per car is the car's key + 8** (`CAR_GROUPS.md` 1: the first car of a type has key =
   type, so its id is retail's type + 8; a duplicate gets 0x108, 0x118, ...), only while banks are live.
6. **No-reuse works both ways** (records of a flagged group are never handed to another group): with
   banks off a flagged mod car placed first would otherwise give its textures to a later real car of
   the base type.

### 6.3 What changed (8 game functions, 11 port functions)

| VA | function | unit | change under the define |
|---|---|---|---|
| 80121550 | `bitmapInit` | `bitmap_init.c` | `portTexBanksReset()` first: latch bit 5, free every bank |
| 801209E8 | `vramPlaceTexture` | `screen_deathmatch_result.c` | `portTexPlaceBegin(row)` (the group's bank, allocated at its first image while live); `portTexFindReuse(key, row, bank)` instead of `bitmapFindRecord` (banked: level records only; no-reuse flags); in a bank the scan tests/marks with `portTexBankIsFree/Mark`; `portVramUploadBank` around the `LoadImage`; slot tpage `\| bank << 9`; `portTexNoteSlot` publishes image 0's CLUT |
| 80120D3C | `vramUploadClut` | `scene_init.c` | the group's bank (`portTexBankOfGroup`); in a bank the same scan with `portTexBankIsFree/Mark`; `portVramUploadBank` around the `LoadImage` |
| 80120744 | `modelRemapTextures` | `world_collapse_warps.c` | while live, `rest` (tpage bits 9..15) = the slot's tpage >> 9 |
| 801489B4 | `clutAnimRegister` | `engine_clut_anim.c` | while live: a type-2 (+8) definition is keyed `portCarLoadKey() + 8`; `def->y` carries the textured primitive's bank in bits 9..15 |
| 8014DCEC | `gpuLoadImageLine` | `engine_fileio_debris.c` | `portVramUploadBank = (y >> 9) & 0x7F`, `y &= 0x1FF`; reset after the `DrawSync` |
| 8014DD30 | `gpuStoreImageLine` | `engine_fileio_debris.c` | the same around `StoreImage` |
| 8010DE54 | `weaponInvisibilityUpdate` | `ai_link_node_pick.c` | fade out / in by `portTexFadeId(st->slot, st->type)` (step 1 already changed it) |

Port code (`port_variant_arrays.c`, `.port_text`): `portTexBanksReset`, `portTexPlaceBegin`,
`portTexFindReuse`, `portTexBankIsFree`, `portTexBankMark`, `portTexBankAlloc`, `portTexBankFree`,
`portTexBankOfGroup`, `portTexNoteSlot`, `portTexFadeId`, `portTexLoadImage`; with `TM2_PORT_CAR_SWAP`,
`portCarResetLook` ends the fade by `portTexFadeId` too. Not changed (checked): `sessionLoadArenaAndCars`
(banks are allocated lazily by the placement), `bitmapAddRecord` (it copies the slot's tpage, bank bits
included), `bitmapRegisterTim`, the emitter, the near pass and `fxDebrisDraw` (2.4), the per-group wreck,
body (shadow) and debris lookups: since step 1 they are by key (`carWreckModelGet(key)` in
`carApplyDamage`'s `debrisBreakModel`, `carBodyObjectShow(key)` for the shadow world model), and the models
are the group's own DMD copy, remapped with its bank, so item 4 of section 5 needs no code.

### 6.4 For the port: words, functions, addresses

Storage (`.port_bss`, every variant, after `portCarBindGroup`, so nothing before it moves; rows in
`config/US/port_variant_arrays.tsv`, group `one`). Addresses from each build's `symbols.tsv`
(`work/pv/r32d_*_P4`; they move with the storage before them and the other defines: read them from the
map):

| variable | default variant | groups + banks | groups + cars=16 + swap + banks | every port define | size | contents |
|---|---|---|---|---|---:|---|
| `portTexBanks` | 0x805B91B0 | 0x805B91B0 | 0x805F5660 | 0x805F5660 | 1,232 | `struct TM2PortTexBanks` (below) |
| `portVramUploadBank` | 0x805B9680 | 0x805B9680 | 0x805F5B30 | 0x805F5B30 | 4 | bank of the CPU upload in progress, 0 = VRAM |
| `portCarGroupNoReuse` | 0x805B9684 | 0x805B9684 | 0x805F5B34 | 0x805F5B34 | 64 | per load group (u8), the port's: 1 = never share an image by key |
| `portCarOptions` | 0x803A2CC8 | 0x803A2CC8 | 0x803A2CC8 | 0x803A2CC8 | 4 | bit 5 = banks for the next load |

`struct TM2PortTexBanks` (`include/semantic/tex_banks.h`; written by the C, read by the port):

| offset | field | contents |
|---|---|---|
| 0x000 | `live` | bit 5 as latched by this load's `bitmapInit` |
| 0x004 | `level_records` | image records of the arena's TMS (-1 until the first car image) |
| 0x008 | `cur_bank`, 0x00C `rect_count`, 0x010 `rects[72][4]` | the bank being filled and its rectangles (x, y, w, h as s16) |
| 0x250 | `group_bank[64]` (u8) | bank of each load group (human i = group i + 1, opponent i = humans + i + 1), 0 = none |
| 0x290 | `bank_group[128]` (u8) | load group owning each bank, 0 = free |
| 0x310 | `body_clut[64]` (u16) | CLUT id (x / 16 \| y << 6) of the group's image 0 (the body, `TEAM_COLORS.md` 1.1) |
| 0x390 | `body_bank[64]` (u8) | the bank of that CLUT |
| 0x3D0 | `rec_first[64]`, 0x450 `rec_end[64]` (s16) | the group's image records (for the no-reuse flag) |

Port functions (`.port_text`, only with the define; addresses from `functions.tsv`, kind `port`):

| function | groups + banks | + cars=16 + swap | every port define | use |
|---|---|---|---|---|
| `void portTexBanksReset(void)` | 0x805B9BD0 | 0x805F7FA8 | 0x805F8FB8 | called by `bitmapInit`: latch bit 5, free every bank |
| `int portTexPlaceBegin(int group)` | 0x805B9FA8 | 0x805F8380 | 0x805F9390 | called by `vramPlaceTexture`: the group's bank (allocated at its first image while live), record range, level record count |
| `int *portTexFindReuse(int key, int group, int bank)` | 0x805BA08C | 0x805F8464 | 0x805F9474 | the record to share by key (retail `bitmapFindRecord` rules unless banked / no-reuse), 0 = place |
| `int portTexBankIsFree(int bank, int x, int y, int w, int h)` | 0x805B9C7C | 0x805F8054 | 0x805F9064 | whole-rectangle test in the bank being filled |
| `void portTexBankMark(int bank, int x, int y, int w, int h)` | 0x805B9D78 | 0x805F8150 | 0x805F9160 | take a rectangle in the bank being filled |
| `int portTexBankAlloc(int group)` | 0x805B9DE0 | 0x805F81B8 | 0x805F91C8 | **API**: a fresh bank for a group (lowest free; 0 = none / banks not live) |
| `void portTexBankFree(int group)` | 0x805B9F08 | 0x805F82E0 | 0x805F92F0 | **API**: give the group's bank back (mid-match removal) |
| `int portTexBankOfGroup(int group)` | 0x805B9F88 | 0x805F8360 | 0x805F9370 | **API**: the group's bank, 0 = none |
| `void portTexNoteSlot(int group, int image)` | 0x805BA19C | 0x805F8574 | 0x805F9584 | publishes image 0's CLUT id and bank (`body_clut`, `body_bank`) |
| `int portTexFadeId(int slot, int type)` | 0x805BA218 | 0x805F85EC | 0x805F95FC | the car's fade id: key + 8 while live, else type + 8 |
| `void portTexLoadImage(int bank, void *rect, void *data)` | 0x805BA250 | 0x805F8624 | 0x805F9634 | **API**: `LoadImage` into a bank (team paint), word set and reset around it |

### 6.5 The PSY-Z protocol, step by step

1. **Addresses.** Read `portVramUploadBank`, `portTexBanks`, `portCarOptions` and `portCarGroupNoReuse`
   from the build's `symbols.tsv` (6.4).
2. **Before a load** (`sessionLoadArenaAndCars`): set `portCarOptions |= 0x20` for a banked match, and
   `portCarGroupNoReuse[group]` for the groups that must not share by key (mod cars). `bitmapInit`
   latches bit 5 and frees every bank; changing the bit during the match has no effect until the next
   load.
3. **CPU uploads.** At the entry of `LoadImage(rect, data)` and `StoreImage(rect, data)` read the 32-bit
   word at `portVramUploadBank`: `b = word & 0x7F`. `b == 0`: the PS1's VRAM, as today. `b != 0`: the
   rectangle (x 0..1023, y 0..511) is written into / read from bank `b`. Latch `b` with the transfer
   when it is queued: the game resets the word to 0 only after its `DrawSync(0)`, but a queue that runs
   later must not read the word again. Every other VRAM access is bank 0: `MoveImage`, `ClearImage`,
   `LoadImage`/`StoreImage` with the word 0, all drawing (the frame buffers are only in bank 0).
   The game's bank-aware calls are exactly: `vramPlaceTexture` and `vramUploadClut` (car pixels and
   CLUTs at the load), `gpuLoadImageLine` / `gpuStoreImageLine` (the CLUT animations: a car's fade in its
   bank), and `portTexLoadImage` (for the port's own uploads); `tools/check_car_groups.py` check 6 proves
   that every store of a bank is followed by the upload and a reset to 0.
4. **Drawing.** For every textured polygon command (GP0 0x24..0x27, 0x2C..0x2F, 0x34..0x37, 0x3C..0x3F)
   the attribute halfword of its second texture word (uv1 + tpage) gives `bank = (attr >> 9) & 0x7F` and
   `texpage = attr & 0x1FF`; sample the texels **and the CLUT** from bank `bank`. Bit 11 (= bank bit 2)
   is not "texture disable" for these polygons. When a polygon's texpage updates the draw-mode state
   (GPUSTAT / E1, used by later sprites and rectangles), take bits 0..8 only. Sprites, rectangles, lines,
   `DR_MODE`/E1 packets and untextured polygons are bank 0 (the game never puts a bank there: the
   emitter, the near pass and `fxDebrisDraw` store only the low byte of the tpage into E1, 2.4).
5. **Bank lifetime.** `portTexBanks.bank_group[b] != 0` = bank `b` in use. A load frees every bank
   (`bitmapInit`); `portTexBankFree` frees one mid-match. PSY-Z may allocate a bank's storage at its first
   upload and drop it when its owner returns to 0; texels outside the uploaded rectangles are undefined
   and never sampled (a fresh bank is placed into as if empty). Group n gets bank n in a normal load,
   but read `group_bank` rather than assume it.
6. **Team paint.** After the load (the slots die with the first frame, `TEAM_COLORS.md` 1.4):
   group g's body CLUT is `portTexBanks.body_clut[g]` in bank `body_bank[g]`; recolour it in place
   (`TEAM_COLORS.md` 4 for the entries) with `portTexLoadImage(bank, &rect, palette)` (rect = (id & 0x3F)
   * 16, id >> 6, 256, 1) or with a direct write into that bank. Every car has its own body CLUT when
   banked, duplicates included (way C per car, no extra rows). Do it between fades (a fade restores the
   palette it saved, `TEAM_COLORS.md` 2.5).
7. **Texture replacement / caches** keyed by VRAM position must include the bank.

### 6.6 Retail-equal with the define on and bit 5 clear

Every changed function then runs retail's logic: `portTexPlaceBegin` returns 0 (no bank),
`portTexFindReuse` returns `bitmapFindRecord`'s record (the first with the key) while no
`portCarGroupNoReuse` flag is set, the scans use the VRAM bitmap, the slots get no bank bits, the remap
writes nothing more, `clutAnimRegister` keeps retail's id and row, the line helpers and the uploads store
0 into `portVramUploadBank`, the fades use type + 8. Expected differences against the variant without
the define: only port storage (`portTexBanks` bookkeeping, the word written with 0) and the addresses of
`.port_text`. No `rand()` call is added or removed (none of the new code calls it).

### 6.7 Duplicates, per-car CLUTs (`PER_CAR_PLAN.md` C.1), mid-match loads and mods

- **Duplicates.** With banks on, each duplicate group has its own bank with its own copy of the pixels
  and of every CLUT, so everything C.1 wanted per car comes for free: the invisibility fade fades only
  that car (one definition per key, in its bank), team paint recolours only that car (6.5 item 6), and
  the shared part images (undercarriage, smoke, red light, and turbo flame / shadow except on the arenas
  that hold them) are per car too. What stays shared is what the arena holds (6.2 item 1), as retail.
- **C.1 is the no-banks fallback** (built 2026-10-13: `CAR_GROUPS.md` 13, `TM2_PORT_CAR_CLUTS`, `portCarOptions` bit 7,
  ignored while banks are live; it reuses the pieces named here). Recommendation: build C.1 only if the port needs per-car CLUTs with
  bit 5 clear (a mode that must keep the PS1's VRAM, e.g. a frame-exact comparison of VRAM with retail);
  otherwise skip it. If it is built, reuse step 4's pieces: `portTexFindReuse` (its "no car-to-car reuse"
  branch is C.1's reuse rule minus the CLUT copy), the per-key fade (`clutAnimRegister`'s keying and
  `portTexFadeId`, gated by C.1's own bit as well) and `body_clut`; only the CLUT copier with the rows
  240..255 allocator (G.1) is new.
- **Mid-match loads** (`PER_CAR_PLAN.md` B.7.4 item 2): banked placement needs only port storage, so of
  B.7.1's load-time-only storage the VRAM bitmap is no longer needed. Still needed by the later mid-match
  step: the slot table (`vramTexSlots`, in packet buffer 0 during a match) and `modelRelocateStack` as port
  blocks (`bitmapSetupStorage`), and the arena's image records for the arena keys (they live in an
  ordering table). Simplest: set `portCarGroupNoReuse` for a joining group, which then reads no record at
  all (its arena-key images are placed in its own bank: same pixels; on Suicide Swamp, Cyburbia and Roof
  Tops Roadkill's and Dark Tooth's effect frames then show the car's palette instead of the arena's).
  Sequence for one group g: `portTexBankAlloc(g)` (or let the first `vramPlaceTexture` do it),
  `bitmapParseContainer(tms, g)`, `assetInstallImage(dmd, g)`; tracking is off mid-match, so no record is
  appended. To drop a car for good: hide it, end its fade (`portCarResetLook`), then `portTexBankFree(g)`.
  **Built (request 33, `CAR_GROUPS.md` 14.6):** `portCarLoadGroupLive` / `portCarUnloadGroup` with
  `TM2_PORT_CAR_DROPIN`; the slot table, the usage bitmap and the image records are kept in port storage
  (`bitmapSetupStorage`), so no-reuse is not needed; `modelRelocateStack` needs nothing (it is 0 in retail: the
  stack is at address 0, `PER_CAR_PLAN.md` B.7).
- **Mods** (`PER_CAR_PLAN.md` E.1, G.10): set `portCarGroupNoReuse[g]` for a mod group. With banks it
  only stops the arena-key sharing (car-to-car sharing is off anyway); without banks it keeps the mod's
  images and the base car's apart in both directions.

### 6.8 Limits

- **127 banks, 63 groups** (`TM2_PORT_TEX_GROUPS` 64 = `TM2_CAR_SLOT_CAP`); a group without a free bank
  uses bank 0 with retail's rules.
- **One bank is filled at a time:** `vramUploadClut` / `vramPlaceTexture` can add to a bank only until the
  next bank is allocated (e.g. `TEAM_COLORS.md` way A right after the group's parse works; later it fails
  and returns 0). Repaint in place instead (6.5 item 6).
- **Slot stride:** without `TM2_PORT_CARS` the stride is retail's 20, so a Dark Tooth that is not the last
  group still lends images 20..30 to the next group's slots (`SURVIVAL.md` 4.2), now with that group's
  bank. Use banks with `TM2_PORT_CARS` (stride 32) or load Dark Tooth last.
- **CLUT animation tables:** a banked match has one fade definition per car (retail: one per type), so
  with more than about 11 cars use `TM2_PORT_CARS` (`clutAnimDefs` 40, `clutAnimRunning` 20). A car whose
  definition does not fit has no fade (its body still hides and shows).
- **Freed banks:** the primitives of a freed group still name the bank; free only cars that are no longer
  drawn.
- **Arena-key images** are shared with the arena as in retail: Axel's fade (undercarriage key 0x9201) acts
  on the arena's undercarriage on New York and Hong Kong (retail does the same). Spectre's and Roadkill's
  undercarriage is key 0x1FCD5, which no arena has. Per-car CLUTs (33-1, bit 7) copy it.
- **The port's memory:** a full bank is 1 MiB (1024 x 512 x 16 bits); 33 banks are 33 MiB. A car uses
  about 11,000 texels (Dark Tooth 80,000), so a sparse store (pages of 64 x 256, the tpage unit) is
  enough. **The port's answer (2026-10-13):** banks are PC-side pages the port's GPU allocates on demand;
  they take no game RAM (nothing in the port sections below `--port-ram-end`), so 16 Dark Tooths in 16 banks
  (request 33-2e, `CAR_GROUPS.md` 11) cost only the port's own memory.

### 6.9 Proofs (static)

- **Built and proved** (`build.py --source-only --sdk-from-lib --relocatable --port-variant`,
  `work/build-20261012-235044-734697`; then `tools/build_source_only.py` + `verify_reloc.py` per configuration
  on the objects of that build or of the identical one before it, `work/build-20261012-221455-680707`: the
  variant and its placement check, as `build.py` links them). The retail images keep their hashes: hybrid
  `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`. `tools/data_audit.py`: 0
  mismatches. Every configuration passes its proof against E0 and the placement check (humans/views/HUDs
  forced to 2, every other limit to retail's):

  | configuration | variant sha256 | `port_changed` | port end |
  |---|---|---|---|
  | no port define (published `out/TM2_SOURCE_PORT.EXE`) | `ae34528c…` | none (the 25 functions of request 25; `.port_bss` + 1,300 bytes) | 0x805B9850 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_TEXTURE_BANKS` | `0ab2b781…` | 54: step 1's 47 + 7 (`weaponInvisibilityUpdate` is in both) | 0x805BA410 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CARS=16` + `TM2_PORT_CAR_SWAP` + `TM2_PORT_TEXTURE_BANKS` | `ab367ebb…` | 118: steps 1-2's 114 + `bitmapInit`, `clutAnimRegister`, `gpuLoadImageLine`, `gpuStoreImageLine` | 0x805F87E0 |
  | + `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` (every port define) | `be85f039…` | 125 (+ the 7 of requests 30/31) | 0x805F9850: 26,544 bytes below 0x80600000 |

  The bank code is about 2.1 KB of `.port_text` (2,104 bytes); the storage 1,300 bytes. No jump table
  changed beyond those of steps 1-2. Without `TM2_PORT_CAR_GROUPS` the define stops the build (`#error` in
  `tex_banks.h`, checked with the preprocessor).
- **`tools/check_car_groups.py`** (checks 6 and 7 new): `--expect-functions` adds `EXPECTED_BANKS` with the
  define (54 / 118 / 125 as above); check 6: the 11 port functions are in `.port_text`, the one tpage write
  of the remap is followed by the bank write and `modelRemapTextures` reads `portTexBanks.live`, the
  functions that store `portVramUploadBank` are exactly `vramPlaceTexture`, `vramUploadClut`,
  `gpuLoadImageLine`, `gpuStoreImageLine` and `portTexLoadImage`, each in the order bank store, upload,
  store of 0; check 7 (`--bank-model`): all 15 cars placed in a bank of their own on each of the 14 arenas
  (210 banks), nothing overlapping, nothing outside VRAM or across a texture page, the largest bank Dark
  Tooth's (81,664 texels with its CLUTs); checks 1, 5, `--dmd`, `--key-model` pass as before.
- **Census claims this step relies on, re-checked** (disc, read-only, scratch `r32d/bits_lvl.py`): tpage bits
  9..15 and CLUT bit 15 are 0 in every textured primitive of the 15 car DMDs (7,604 primitives), of the 14
  arena DMDs and of CARSEL/AICARSEL; the arena DMD is loaded by `worldLoadStartObjects` and never remapped;
  `assetInstallImage` (the only caller of `modelRelocate`) runs only for car groups; the only reader of a
  car primitive's tpage halfword is `clutAnimRegister` (`(mode >> 7) & 3`), the others build packets from
  level sprite frames (`fxImpactParticlesDraw`, HUD, bullets); the emitter and the near pass copy the whole
  halfword into the polygon and only its low byte into E1.

### 6.10 Tests for the port

1. **Equality, bit 5 clear:** frame-by-frame against the variant without the define (or E0 with the
   variant's map); expected differences: port storage and `.port_text` addresses only (6.6).
2. **One car per bank:** set bit 5, load 2-11 different cars: each group's `group_bank` is set, every car
   looks as in retail (compare screenshots with bit 5 clear), the arena-key images (Roadkill's special,
   Dark Tooth's effect frames on Suicide Swamp / Cyburbia / Roof Tops) keep the arena's palettes.
3. **Duplicates** (with `portCarOptions` bit 0): two humans on one type: invisibility on one fades only
   that car; repaint one car's body CLUT (6.5 item 6): only that car changes.
4. **16 cars** with `TM2_PORT_CARS=16` (bits 1-3 and 5): Paris and Suicide Swamp with 15 different types
   plus a duplicate (retail runs out of VRAM there: "Relocatable texture not loaded" must not appear).
5. **Debris, wrecks, shadows, projectiles** of a duplicate draw with its own textures (bank tag on the
   chunk and burst packets).
6. **Upload word:** log every `LoadImage`/`StoreImage` with a non-zero `portVramUploadBank` during a load
   and a fade: car pixels and CLUTs at the load, the fade's line every frame of it; never a frame-buffer
   copy.
7. **Mod car:** a group with `portCarGroupNoReuse` set and a TMS that reuses its base car's keys shows its
   own textures, with and without banks; the base car loaded after it shows the base car's.

## Not checked

- Nothing was run; the bank scheme, the copy of arena rectangles and the
  fade routing are from code paths only.
- A hand-built GP0 VRAM-copy packet in game code (only the libgpu entry
  points were searched).
- Whether the arenas animate any of the CLUTs that cars take from them by
  key (1.4); if one did, the car bank's copy would not follow.
- Which field of the burst slot `lightAddSources` fills (2.2, LIKELY the mesh
  pointer).
- Discs of other regions.
