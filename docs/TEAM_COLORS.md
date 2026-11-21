# Team colours on the cars: car palettes in VRAM, and one palette per car

Answers item 19 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(paint each car in its team's colour, also two cars of one type in two
colours, and the same on the car select's spinning car). Builds on
`SIXTEEN_CARS.md` section 4 (placement, slots, free CLUT rows), `SANTA.md`
sections 1.6 and 3 (relocatable assets, tpage/CLUT encoding) and
`SURVIVAL.md` section 2 (one DMD copy per car). Static analysis only: the C
in `src/semantic`, the pure-C references in `reference/`, and small
read-only scratch parsers run on the USA disc (`cdimage/`: `TMS\*.TMS`,
`DMD\*.DMD`, `SHELLDB1\CARSEL.*`, `SHELLDB1\AICARSEL.*`; scripts in the
scratchpad `req19/` folder). Nothing was run in an emulator and no game data
is copied here: only layouts, positions, counts and palette index numbers.
Addresses are retail (USA).

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code or data, not traced to the end or not seen running.

## Short answer

**Built since (request 33-1, `CAR_GROUPS.md` 13):** with `--port-define TM2_PORT_CAR_CLUTS` and
`portCarOptions` bit 7 every car gets its own copy of each CLUT it would share (duplicates' bodies included), so
the port recolours one car by writing its own row: `portCarCluts.clut[group][0]` (or `portTexBanks.body_clut`)
after the load, `LoadImage` of (x, y, 256, 1) with the recoloured palette (recipe in `CAR_GROUPS.md` 13.6), checking
`portCarCluts.shared[group] & 1` (a copy that found no room). With texture banks (bit 5) the same id is in the car's
bank (`TEXTURE_BANKS.md` 6.5 item 6). Ways A-C below remain valid without these options.

- **One palette per car holds the paint.** Every car's TMS has the body
  texture as image 0: a 128x128 8-bit image with its own 256-colour CLUT.
  The paint of all 14 normal cars is in that one CLUT. The other images are
  4-bit (16-colour) parts shared by most cars (undercarriage, shadow,
  flames, smoke) and per-car weapon and effect frames; none of them is paint.
  CONFIRMED (disc), section 1.1.
- **Where:** car images and CLUTs are placed at load by `sub_801209E8`
  (vramPlaceTexture); a car's 256-colour CLUT takes the first free 256-wide
  row, in practice at x 768 of the upper half, just below the level's CLUT
  rows (Antarctica: row 89 for the first car type, 90 for the second ...).
  The position is recorded in the slot `vramTexSlots[n * 20 + image]` of the
  car's load group n. CONFIRMED (code); rows LIKELY (replay).
- **Two cars of one type share everything in VRAM.** The second car finds
  each image by its key (`sub_801218CC` (bitmapFindRecord)) and only copies
  the first car's tpage, x, y and CLUT x/y into its own slots. Nothing is
  uploaded again. CONFIRMED.
- **How the model points at it:** in the car DMD each textured primitive
  holds, in its tpage field, the image's index in the TMS (bits 0..4) and its
  CLUT field is 0. `sub_80120744` (modelRemapTextures), run by
  `assetInstallImage` on that car's own DMD copy, writes the real CLUT id,
  tpage and UV offsets from the car's slots. The emitter copies the
  primitive's texture words into the GPU packet on every draw. CONFIRMED.
- **Own CLUT for one car: one call at load.** Each car already has its own
  DMD copy and its own slots, and the remap reads the CLUT from the slot.
  So after `bitmapParseContainer(carTms, n)` the port only has to upload a
  recoloured copy of the body palette with `sub_80120D3C`
  (vramUploadClut)(256, 1, palette, 0, n): it finds a free row, uploads it
  and writes its x/y into slot n*20 + 0. The remap then points every body
  primitive of that car, and only that car, at the copy. No other change.
  CONFIRMED (code path); not seen running.
- **VRAM room:** one 256-wide CLUT row per (car type, team) pair beyond the
  first. Overwriting the original row instead (when no car of that type
  keeps the original colours) costs nothing. Arenas have 13..18 free rows
  before the cars (`SIXTEEN_CARS.md` 4.4); rows 240..255 under the frame
  buffers give 32 more spots (`SIXTEEN_CARS.md` 4.6 option 2). Section 3.4.
- **Paint entries (section 4):** picked per car by hue, saturation and
  brightness rules on the body palette, checked by recolouring the body
  texture and looking at it. 31 to 141 of the 256 entries per car, covering
  9..64% of the body texels the model uses.
- **Car select:** `CARSEL.TMS` loads its own copies of the 13 body textures
  at fixed spots. Their palettes are **byte-identical** to the in-game
  ones and sit at the fixed rows **(768, 23 + car index)** (Hammerhead 23 ..
  Sweet Tooth 35). Minion's select model uses a different 256x128 texture
  with its own palette at (768, 36). For one car on screen, re-upload the
  recoloured palette to that row (one `LoadImage`); for several cars in one
  frame, patch the CLUT field of that car's select primitives before each
  draw. Section 5. CONFIRMED (data, code); not seen running.

## 1. How a car's textures and CLUTs reach VRAM

### 1.1 What each car loads (CONFIRMED, `TMS\<code>.TMS`)

Every car TMS is a "TXSP" container (`SANTA.md` 1.6): count, key table,
then one TIM per image. All CLUT blocks are one row (16x1 or 256x1); their
VRAM x/y in the file are 0 and ignored. Image 0 is always the body.

| Car (code) | Body image 0 (128x128 8-bit, 256-colour CLUT) key | Other own 256-colour CLUTs | Own 16-colour CLUTs (what) | Shared 16-colour images (keys) |
|---|---|---|---|---|
| Hammerhead (HH) | 0x3BAD | | | 9201, AA9A, 9941, 9BCF |
| Outlaw 2 (OT) | 0x96BC | | 96D6 (blue siren light) | 9201, AA9A, 9941, 9BCF, 9609 (red light) |
| Warthog (WH) | 0xB5E7 | | | 1FCD5, AA9A, 9941, 9BCF |
| Mr. Grimm (MG) | 0x893B | | 13: skull missile and explosion frames (asset 2322), 8A27 (its own shadow blob, node +3) | 9201, 9941, 9BCF |
| Grasshopper (PV) | 0x8C8C | | 1FB2D (asset 309) | 1FCD5, AA9A, 9941, 9BCF |
| Thumper (TH) | 0x8C90 | | | 1FCD5, AA9A, 9941, 9BCF |
| Spectre (SP) | 0xB55C | | 4: special (asset 2326) | 1FCD5, AA9A, 9941, 9BCF |
| Roadkill (RK) | 0x63F3 | | 3: special (asset 2321) | 1FCD5, AA9A, 9941, 9BCF |
| Twister (IR) | 0x9960 | | 4: tornado (asset 2323) | 9201, AA9A, 9941, 9BCF |
| Axel (AX) | 0xFE13 | 2 (asset 2324) | | 9201, AA9A, 9941, 9BCF |
| Mr. Slam (FL) | 0x8AE3 | | | 9201, AA9A, 9941, 9BCF |
| Shadow (HS) | 0x8F01 | | 2 (asset 2325) | 1FCD5, AA9A, 9941, 9BCF |
| Sweet Tooth (ST) | 0xCCF6 | 1 (asset 2301) | | 1FCD5, AA9A, 9941, 9BCF |
| Minion (MN) | 0x7D0A | | 7D15, 7D11 (tires, node +2) | AA9A, 9941, 9BCF |
| Dark Tooth (BB) | 0xEA65 (256x256) | 10 more | 15 | 1FCD5, AA9A, 9941, 9BCF, 9609 |

The shared 16-colour images are the undercarriage (9201 / 1FCD5, used by
the body nodes +1 and +8), the shadow blob (AA9A, node +3), the turbo flame
(9941) and smoke (9BCF, node +6). Keys are the TMS key-table words, as
`bitmapFindRecord` compares them. "Asset" = the weapon model id the
primitives belong to (`SURVIVAL.md` 2.2).

So the paint is always in one CLUT per car, the body CLUT. The only car
whose look is mostly outside it is Axel: his wheel frame is the shared
16-colour undercarriage image 9201 (section 4.2).

### 1.2 Where they go (CONFIRMED code; positions LIKELY, replay)

`sub_8012BAFC` (sessionLoadArenaAndCars) (`shell_main.c`): `bitmapInit`,
level TMS with `bitmapParseContainer(buf, 0)` (fixed spots), then each car
TMS at 0x80019C40 with `bitmapParseContainer(buf, n)`, n = 1 .. (humans
first, then opponents). `sub_8012191C` (bitmapRegisterTim) hands each image
to `vramPlaceTexture(info, index, n)` (`screen_deathmatch_result.c`), which:

1. reuses a known image by key (1.3), or
2. finds a free spot in the lower half for the pixels, `LoadImage`s them,
   then, if the TIM has a CLUT, calls `vramUploadClut(16 or 256, 1, clut,
   index, n)` (`scene_init.c`): the first spot whose top-left texel is free,
   scanning the blocks (0,0), (256,0), (512,0), (768,0), then the lower
   four, row by row, x in steps of the CLUT width. It marks the row,
   `LoadImage`s the CLUT and writes its x/y into the slot;
3. writes tpage (`GetTPage(mode, 0, x, y) & 0x1F`, page bits only), x, y
   into slot `n * 20 + index`.

During a battle x 0..639 of the upper half is reserved (frame buffers), so
the first free 256-wide row is at x 768, below the level's CLUT rows. The
16-colour CLUTs fill the first free 16-texel gaps, usually in the
part-used last level row. Replay on GLACIER (Antarctica) with Outlaw 2,
Spectre, Outlaw 2 again and Roadkill, in that load order:

| Group | Car | Body pixels | Body CLUT | CLUT id (x/16 \| y << 6) | 16-colour CLUTs |
|---|---|---|---|---|---|
| 1 | Outlaw 2 | (448,256) | (768,89) | 0x1670 | (816..896, 88) |
| 2 | Spectre | (448,384) | (768,90) | 0x16B0 | (912..976, 88) |
| 3 | Outlaw 2 | reused | reused (768,89) | 0x1670 | reused |
| 4 | Roadkill | (512,256) | (768,91) | 0x16F0 | reused or new |

The rows therefore depend on the arena and the load order; the port should
read them from the slots (or from the primitives after the install), never
assume them.

### 1.3 Two cars of one type (CONFIRMED)

`vramPlaceTexture` first calls `bitmapFindRecord(bitmapKeyTable[index])`.
The records (`bitmapRecords`, 24 bytes: key, tpage, x, y, CLUT x, CLUT y)
are appended by `sub_80121764` (bitmapAddRecord) for every image of the
level and of every car. On a hit it copies tpage, x, y, CLUT x and CLUT y
into the new car's slot and returns: no pixels, no CLUT uploaded. The
second car of a type thus has its own slots (group n) holding the **same**
CLUT position as the first. (`bitmapFindRecord` returns the first record
with the key, so the first car's placement stays the reference.)

### 1.4 What lives how long (CONFIRMED)

- The palette data in RAM (`TimInfo.clut`) points into the TMS copy at
  0x80019C40. It exists only during that car's `bitmapParseContainer`: the
  next car TMS, then the level DMD, are read over it.
- The usage bitmap and the slot table live in packet buffer 0
  (`sub_801214FC` (bitmapSetupStorage)), the records in an ordering table.
  They are valid from `bitmapInit` to the end of the load; the first frame
  drawn overwrites them. `bitmapDisableTracking` only stops new records.
- So a new CLUT must be **allocated during the load** (or at a spot the
  port picks itself), and the port must keep any position it needs later
  (for example to switch colours between rounds) in its own variables.

## 2. How a car's model points at its CLUTs

### 2.1 Mesh and primitive layout (CONFIRMED, `modelRemapTextures`, the emitter `reference/model_emitter_800CDC54.c`, disc)

A mesh node (type 0, `SANTA.md` 1.5) has the primitive list at +0x0C and
the count at +0x10. Primitive records follow each other; their length
varies:

| Offset | Field |
|---|---|
| +0 | u8: bits 0..6 = texture word count (0 untextured, 3 triangle, 4 quad), bit 7 = texture-window flag |
| +1 | u8 colour word count (1, 3 or 4) |
| +2 | u8 record length in words (next record = this + 4 x this) |
| +3 | u8 GPU packet length |
| +4..+11 | vertex indices (s16 x 3 or 4) |
| +12 | control word: bits 16..18 type (3 triangle, 4 quad, 1 beam, 0 sprite), 19..21 lighting mode, 22..23 packet stride, 28..30 colour word count, bit 31 no backface test |
| +16 | colour words (the first holds the GPU code in its top byte) |
| +16 + 4c | uv0 (low half) + **CLUT id** (high half) |
| +20 + 4c | uv1 + **tpage** (high half) |
| +24 + 4c | uv2 |
| +28 + 4c | uv3 (quads) |
| then | texture-window word (flagged primitives), normal indices (lit primitives) |

c = colour word count, so the CLUT id is the u16 at **+22, +30 or +34**
(c = 1, 3, 4); `modelRemapTextures` computes the same offset as
`16 + ((byte 15 >> 2) & 0x1C)`. On the car DMDs the records are 24 to 56
bytes; 36 bytes is the flat-shaded textured quad (c = 1, no normals).

Encodings (as `SANTA.md` 3.4): CLUT id = x / 16 | y << 6; tpage = x / 64 |
(y / 256) << 4 | abr << 5 | depth << 7.

### 2.2 In the file (CONFIRMED, all 15 car DMDs)

- CLUT id = 0 in every textured primitive (7,604 of them).
- tpage bits 0..4 = **index of the image in the car's TMS**; bits 7..8 =
  depth (1 = 8-bit for image 0, 0 = 4-bit for the parts; checked per
  image); abr bits as wanted.
- UVs relative to the image (0..127 for the body).

Body primitives (tpage index 0) per car, all meshes of the DMD:

| HH | OT | WH | MG | PV | TH | SP | RK | IR | AX | FL | HS | ST | MN |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 316 | 383 | 374 | 206 | 408 | 408 | 326 | 280 | 442 | 213 | 305 | 378 | 319 | 410 |

They are in the body select node (id type + 1: the damage stages), the
tires (+2), the wreck (+5) and the far-LOD body (+8), so a recoloured CLUT
follows the car through damage, wreck and distance.

### 2.3 The remap (CONFIRMED, `world_collapse_warps.c`)

`sub_80120EF4` (assetInstallImage)(buf, n) sets `vramTexSlotGroup` = n and
relocates each root (`sub_80120FEC` (modelRelocate)), which calls
`modelRemapTextures(mesh)` once per mesh node. For each textured primitive:

```c
t = tpage & 0x1F;                    /* image index */
s = &vramTexSlots[vramTexSlotGroup * 20 + t];
clut  = (s->clut_x >> 4 & 0x3F) | (s->clut_y & 0x3FF) << 6;
tpage page bits = s->page;           /* abr and depth kept from the file */
uv += (s->x % 64 * (4, 2 or 1 by depth), s->y & 0xFF)  /* or the window word */
```

The remap writes into the car's own DMD copy (each car loads its own,
`SURVIVAL.md` 2.1), so two cars of one type end up with separate primitive
records that hold the same CLUT id today, because their slots hold the same
CLUT x/y (1.3). The level DMD is not remapped (its words are absolute).

### 2.4 The draw reads the record every time (CONFIRMED, `sub_800CDC54`)

The emitter copies the primitive's texture words (uv0+CLUT, uv1+tpage, uv2,
uv3) from the record into the GPU packet at each draw. A CLUT id written
into a record takes effect on the next draw, and a value written before one
draw call and restored after it affects only that call (used in 5.2).

### 2.5 The CLUT animation on node +8 (LIKELY)

`sub_800D1DB0` (worldLoadObject) sends ids type + 8 (24, 40, .. 248) to
`sub_801489B4` (clutAnimRegister)(mesh, 2). It reads the CLUT id and depth
of the mesh's **first** primitive (after the remap) into a fade definition
keyed (type + 8, 0); a second car of the type is refused
(`clutAnimNewDef` returns 0 for an existing key). The human invisibility
pickup (`weaponInvisibilityUpdate`, 8010DE54) fades that CLUT in VRAM
(`clutAnimStartFadeOut(type + 8, 0, 20)`, then `clutAnimFadeIn`), reading it
with `StoreImage` at the start and writing the saved original back at the
end.

| First primitive of the +8 mesh | Cars |
|---|---|
| body CLUT (image 0) | WH, PV, IR, FL, HS, ST, MN, BB; HH (its first-walked +8 mesh, LIKELY) |
| shared undercarriage CLUT (image 1) | SP, RK, AX |
| untextured (the def gets a junk position in the frame buffer) | OT, MG, TH |

For team colours this means: the fade acts on the CLUT of the first car of
that type installed (humans are installed first) (request 33-1, `CAR_GROUPS.md` 13: with bit 7 the fade is per
car key on the car's own copy, also of the undercarriage), and if the port uploads
another palette into that row while a fade runs, the fade's end restores
the palette it saved. Harmless for separate team rows (2.3 keeps each car
on its own row); worth knowing for way C of section 3.

## 3. Giving one car its own copy of the body CLUT

### 3.1 Way A: at load, through the slot (recommended)

In `sessionLoadArenaAndCars`, right after `bitmapParseContainer(carTms, n)`
for a car whose team needs another colour:

```c
u32 *c = (u32 *)0x80019C40;           /* the car TMS just parsed */
u16 *body = (u16 *)(c + 4 + c[3] + 1 + 5);  /* image 0's CLUT data: header 4
                       words, key table c[3] words, size word, TIM id, flags,
                       CLUT block length, x/y, w/h */
u16 pal[256];
memcpy(pal, body, 512);
teamRecolour(pal, carType, team);     /* section 4 */
if (have (carType, team) row)          /* port's own cache */
    vramTexSlots[n * 20 + 0].clut_x/.clut_y = that row;
else
    vramUploadClut(256, 1, pal, 0, n); /* finds a row, marks it, LoadImage,
                                          writes slot n*20+0 CLUT x/y */
```

`vramUploadClut`'s slot struct puts x at +12 and y at +16, which are the CLUT
fields of the 20-byte slot (`TexPreset` in `world_collapse_warps.c`), so it
moves only the CLUT of that slot; tpage, x and y stay. Then
`assetInstallImage(dmd, n)` remaps that car's body primitives to the new
row. Nothing else reads the slot later, and the record of the image (1.3)
still carries the original CLUT, so a later car of the same type without a
team colour (or with another one) still finds the original. CONFIRMED (code);
not seen running.

It also works for the first car of a type: its body was just uploaded with
the original palette, and the slot is simply moved to the copy. The same
call with index 1 gives a car its own copy of a 16-colour part CLUT
(`vramUploadClut(16, 1, pal, 1, n)`), for example Axel's frame (section
4.2).

### 3.2 Way B: after the install, in the primitives

Walk the car's meshes (from its relocated root) and, in every textured
primitive whose CLUT id equals the car's body CLUT id (keep it from slot
n*20+0 during the load; after the remap the image index in the tpage is
gone, so match on the CLUT id), write the new id. Needs a CLUT row the port has uploaded itself. Use it when the
colour has to change without a reload (for example a team switch between
rounds); writing the old id back undoes it.

### 3.3 Way C: no new row

When no car of a type keeps its original colours (all cars of that type are
on one team), `LoadImage` the recoloured palette over the original row
(slot n*20+0 of the first car of the type). No row is used, no primitive
changes. Two teams with one type then still need way A for one of them.

### 3.4 VRAM room (LIKELY, from `SIXTEEN_CARS.md` 4.4)

Rows needed = number of different (car type, team) pairs, minus one per
type if way C is used for one team of each type. Each body CLUT is a whole
256-wide row (`vramUploadClut` steps x by the CLUT width: x 0, 256, 512,
768 only).

| | Free 256-wide rows at x 768 after the level | Taken by the cars' own CLUTs |
|---|---|---|
| range over the arenas | 13 (Amazonia, Holland, Cyburbia) .. 18 (New York 2P) | 1 per type (Axel 3, Sweet Tooth 2) |

Retail battles have 6 to 10 cars, so a 4-team game of distinct types needs
few or no extra rows with way C; 16 cars of 8 types on 4 teams could need
up to 24 more. The cheap source of more rows is `SIXTEEN_CARS.md` 4.6
option 2: `bitmapInit` reserving (0,0,640,240) instead of (0,0,640,256)
frees rows 240..255 at x 0 and 256, 32 spots, which the CLUT scan finds
first. Option 3 there (16-texel steps) does not help 256-colour rows.

### 3.5 Things to keep in mind

- **Transparency:** 0x0000 is drawn transparent, so a recoloured entry must
  never become 0x0000 (use 0x0421 or any non-zero dark value), and an entry
  that is 0x0000 must stay so. Keep bit 15 (semi-transparency) as it was.
- **Load order and Dark Tooth:** unchanged rules (`SURVIVAL.md` 4.2): with
  stride 20 Dark Tooth must still load last.
- **Fade (2.5):** with two humans of one type, the second human's
  invisibility fades the first human's CLUT (retail does the same, since the
  CLUT is shared there too).
- **Other places a car is drawn with its own colours:** the opponent cards
  (`SIXTEEN_CARS.md` section 6: their own TIMs, palettes at (768, 128 + i))
  and the car select (section 5) are separate images; the HUD and radar use
  colours by type (`hudRadarPlotCar`). None of them uses the battle CLUT.

## 4. Which palette entries are the paint

### 4.1 Method (LIKELY; checked by eye on recoloured images)

Palettes are not sorted: paint shades are spread over the whole CLUT
between unrelated colours, and the body textures are dithered (214 to 254
of the 256 entries appear on each car's model). What worked:

1. **Coverage.** Rasterise the UVs of every body primitive (tpage index 0)
   to know which texels the model shows (9,000 to 14,000 of the 16,384) and
   count each palette index over them.
2. **Classify each entry** by hue H (degrees), saturation S and value V,
   computed from the 5-bit components (c / 31) of the BGR555 entry.
3. **Pick a rule per car**: a hue window plus minimum S and a V range
   (table 4.2). Achromatic paints (white, black) need a V rule instead of a
   hue; the black paints (Hammerhead, Shadow's body) cannot be told apart
   from tires and windows by colour, so for those cars the coloured trim is
   the team colour (flames, purple trim).
4. **Check**: write a copy of the body texture with the picked entries
   recoloured and look at it (`req19/png/paint_<car>.png`). All 14 rules
   below were checked this way.
5. **Recolour** each picked entry: H' = team hue, S' = 0.35 + 0.65 x S
   (so white or grey paint takes the colour too), V' = V; back to 5-bit
   BGR555, bit 15 kept, 0x0000 avoided. Team hues, for example: red 0,
   gold 45, green 120, blue 225. A car whose paint already is the team's
   colour can keep its own palette.

Rules can be tuned per car without code: they only produce a list of
indices, which the port can store as a 256-bit mask per car type.

### 4.2 Per car (LIKELY)

"Share" = picked entries' part of the body texels the model shows.

| Car | Paint (what the rule picks) | Rule: H window, S min, V range | Entries | Share | Notes |
|---|---|---|---|---|---|
| Hammerhead | the flames on the black truck | 345..50, 0.50, 0.35..1 | 141 | 18% | also the red tail lights and a headlight tint; body is black (not separable from tires) |
| Outlaw 2 | the white door and roof panels | any, 0.00, 0.62..1 | 64 | 9% | also small light highlights; the dark body would take the windows and tires with it |
| Warthog | the brown camouflage | 5..45, 0.20, 0.15..1 | 55 | 34% | the red missile on the texture is kept |
| Mr. Grimm | the red bike | 335..20, 0.45, 0.20..1 | 81 | 29% | also the red behind the skull |
| Grasshopper | the green body | 70..170, 0.30, 0.10..1 | 67 | 42% | also the green wheel hubs |
| Thumper | the pink body | 280..345, 0.25, 0.15..1 | 124 | 47% | upper limit 345 keeps the red tail lights |
| Spectre | the blue stripes | 200..265, 0.30, 0.10..1 | 31 | 13% | the body is white: a second colour could be the white (V rule, as Outlaw) |
| Roadkill | the orange rust body | 0..40, 0.45, 0.20..1 | 94 | 45% | grey primer spots stay |
| Twister | the blue body | 195..265, 0.35, 0.10..1 | 45 | 33% | yellow (about 50..70) and magenta trim are candidates for a second colour |
| Axel | his jeans | 195..250, 0.20, 0.10..1 | 76 | 12% | his machine is the shared 16-colour undercarriage image (key 9201), a part CLUT copy (3.1) |
| Mr. Slam | the yellow-tan body | 25..55, 0.30, 0.25..1 | 104 | 34% | also the brown crane arm and wheel hubs; S 0.45 would leave the arm |
| Shadow | the purple trim | 265..330, 0.25, 0.10..1 | 69 | 27% | body is black |
| Sweet Tooth | the red polka dots | 335..15, 0.35, 0.30..1 | 64 | 10% | also the clown's red nose and the ice-cream red |
| Minion | the blue-grey steel | 190..250, 0.08, 0.10..1 | 47 | 64% | low saturation: a strong tint of the whole hull |

Dark Tooth was not analysed: his body is spread over images 0, 5, 6 and 16
(node 248), each with its own CLUT; the same method applies per image.

### 4.3 Index lists (from the rules above, body CLUT of `TMS\<code>.TMS`)

| Car | Paint indices |
|---|---|
| HH | 6,17,26-29,53-54,70-73,76-78,81-83,87,89-91,93-96,98,100-106,109-111,114-115,117-120,122-126,131-138,140-142,144,147-148,150,152-155,157,160-162,165,169,171-172,174-185,188-193,195-206,208,210-217,224-240,244-253 |
| OT | 8,27,35,37-44,84-86,88,99-102,114-117,125,127-128,132,134-136,139-141,162-165,170,182,186,205-207,209-214,220-224,226,231-237,239,251 |
| WH | 4,6,13,15-16,20,27,31,36-37,43-45,51-56,60,68-69,75,84,93,98,100,115,125,127,131-132,135-136,142,144-146,148-149,153-154,157-158,167,171-173,180-183,197,209,234 |
| MG | 6,14,21-22,27-30,36-38,46-48,54,58-59,70-71,75,79-80,82-85,88-91,94,98-100,102-104,106-107,132,137,162-164,166,170-172,175-179,181-194,203,207,210,216,221-222,224-225,229,231,236,242,251-252 |
| PV | 4,37,67,94,105-106,115,120-129,131,135-136,138,144,146,152,154-156,163-164,166-167,169-171,173,175-176,179,181-182,188-196,199,202-205,207-212,214,217,222,227,229,238-239 |
| TH | 0-14,18-20,22,24,28-32,37-38,43-46,48-54,56-58,65-66,68-71,73,75-76,80,84,108,110,112-113,129,134-135,142,152-153,155-156,162-165,167-170,173-179,182-184,196-197,199-205,208-211,213-214,217-218,220-221,223,225-226,229-234,236-244,248,250-253 |
| SP | 95,134,140,143,163-180,185,188,204-207,227,247,251 |
| RK | 25,30-33,48-49,52-54,56-58,61,67,70,74,81-82,94-95,103,105,108-109,114-122,126-127,129-132,134,136,138-139,143-148,150-151,153-159,161-164,166,169,171,173,188-189,192,194,207-208,210,214-222,227-236,241 |
| IR | 0-1,8-12,28-30,41,56-59,75-78,84-86,91,100,103-105,109,117,125,129-130,141,151,163-164,166,169,196,204-205,225,236,241,244 |
| AX | 22,59,92,121,151-152,156,158-164,166-182,184-186,189-195,197-215,218-226,228,231,233-237 |
| FL | 0-1,4-10,12-13,15-16,18-27,30-31,36-37,40,43-45,47-51,54-55,71-72,76-77,83-85,90-92,95-96,100,103-105,131,139-140,144-145,168-169,193-199,202,208,214-225,227-229,232-250 |
| HS | 2,11-21,23,30-31,33-36,42-45,50-53,55,74-75,85,92,104,113,182,189,191-194,196,198-200,202-213,215,229,236,241-250 |
| ST | 13,50-52,55,57-59,61,65,68-71,76-79,86,89,91-93,96-100,105-107,109-111,117-118,122-124,132-133,152-154,156,163-164,167,173,175,177-178,181,186,189-190,195-196,202,204,222,228,240-241 |
| MN | 2,4,6,10-11,19,70,78,83,85,87-93,111,113,116,119-120,133,137-139,151-154,157-160,169-170,176,184-186,191,194,197,202-203,210,212 |
| MN, car select palette (768,36) | 2-3,5,7-8,11-14,54,59-61,68-70,75-76,80-81,83-87,103,106-107,110-111,118,120,124,127,129-132,137-139,142-145,151-153,156-157,165-169,173-174,176-177,180-181,183,195,242-244 |

The first 13 lists apply unchanged to the car select (its palettes are the
same, 5.1). Minion's select palette is a different one; its list was made
with Minion's rule.

## 5. The car select's spinning car

### 5.1 What it draws with (CONFIRMED, disc and code)

`sub_80122750` (sceneLoadTmsDmd) -> `sub_80129010` (shellLoadTmsDmd)
(`screen_card_layout.c`) loads `SHELLDB1\CARSEL.TMS` with
`bitmapParseContainer(buf, 0)`: every image goes to the fixed x/y in its TIM,
CLUT included. `CARSEL.DMD` is then walked in place (`worldLoadStartObjects`):
no relocation, no remap, so its primitives hold absolute tpage and CLUT
words. The shell has no tie to the battle's VRAM layout (the arena load
starts again with `bitmapInit`).

The carousel's select node 9005 (`CAR_SELECT_CAR.md`) has 14 children, one
per car-select index (low-poly models: 59 to 121 body primitives against
206 to 442 in battle). What each child draws with:

| Index | Car | Body image (pixels) | Body CLUT | Body CLUT id | Other CLUTs |
|---|---|---|---|---|---|
| 0 | Hammerhead | (256,256) | (768,23) | 0x05F0 | (768,39) undercarriage 9201, (784,39) shadow AA9A |
| 1 | Outlaw 2 | (192,384) | (768,24) | 0x0630 | same two, plus (800,39) 9609 and (816,39) 96D6 (lights) |
| 2 | Warthog | (192,256) | (768,25) | 0x0670 | (768,39), (784,39) |
| 3 | Mr. Grimm | (128,384) | (768,26) | 0x06B0 | (768,39), (832,39) 8A27 (shadow) |
| 4 | Grasshopper | (128,256) | (768,27) | 0x06F0 | (768,39), (784,39) |
| 5 | Thumper | (64,384) | (768,28) | 0x0730 | (768,39), (784,39) |
| 6 | Spectre | (64,256) | (768,29) | 0x0770 | (768,39), (784,39) |
| 7 | Roadkill | (0,384) | (768,30) | 0x07B0 | (768,39), (784,39) |
| 8 | Twister | (0,256) | (768,31) | 0x07F0 | (768,39), (784,39) |
| 9 | Axel | (960,128) | (768,32) | 0x0830 | (768,39), (784,39) |
| 10 | Mr. Slam | (896,128) | (768,33) | 0x0870 | (768,39), (784,39) |
| 11 | Shadow | (832,128) | (768,34) | 0x08B0 | (768,39), (784,39) |
| 12 | Sweet Tooth | (768,128) | (768,35) | 0x08F0 | (768,39), (784,39) |
| 13 | Minion | 256x128 image at (640,0) (own art, key 0x7D27) | (768,36) | 0x0930 | (768,37), (768,38) (his two other 8-bit images), (784,39) |

So the body CLUT of car-select index i (0..12) is **(768, 23 + i)**, and
its 256 entries and the 128x128 pixels are byte-identical to image 0 of
`TMS\<code>.TMS` (checked for all 13). The undercarriage CLUT (768,39) is
one row shared by every car shown. Rows (768, 0..22) hold the palettes of
CARSEL's 23 other 8-bit images.

The Challenge-mode opponent picker (`sub_80131EEC` (screenAiSelect),
`SHELLDB1\AICARSEL`) has the same layout for its 12 cars (no Sweet Tooth,
no Minion): body CLUT of index i at (768, 23 + i), identical palettes,
undercarriage at (768,35).

### 5.2 Showing another team's colour on a button

**One car on screen (retail car select):** `LoadImage` the recoloured
palette to (768, 23 + i), with i = `sceneObjGetChoiceState(carousel)`; load
the original back to undo. Only that child's primitives use the row, so
nothing else changes colour. The original palette is needed again later:
take it from the car's `TMS\<code>.TMS` (identical), or `StoreImage` the row
once when the screen starts (the CARSEL.TMS copy at 0x80019C40 is
overwritten by CARSEL.DMD right after it is parsed). Upload when the button
is handled, before the scene draws; libgpu queues the `LoadImage` after the
previous frame's `DrawOTag`, so the previous frame still shows the old
colours. When the car changes (Left/Right/R1), re-apply the team palette to
the new car's row, or restore the old row and upload for the new one.
CONFIRMED (code paths); not seen running.

**Several cars in one frame (the port's split-screen car select,
`CAR_SELECT_CAR.md` 3):** the GPU reads the CLUT when it draws the whole
OT at the end of the frame, so one row cannot show two colours in one
frame. Instead give each (car, team) its own row and patch the primitives
per draw:

1. Free rows during car select: (768, 40..63), 24 rows, are not touched by
   any CARSEL image or CLUT (LIKELY: the background and the bio pictures go
   to the frame buffers through `screenDisplayImage`, not seen running).
   AICARSEL leaves the whole lower half from x 640 free.
2. Upload each recoloured palette there once.
3. Before each player's `sceneObjDraw`, write the row's id into the CLUT
   field (2.1) of every primitive of child `node[16 + 4 * car]` whose id is
   `0x05F0 + 0x40 * car` (the body row; 48 | (23 + car) << 6), and restore
   it after the call. The emitter copies the field into the packet during
   the call (2.4), exactly like the shared +7 byte in `CAR_SELECT_CAR.md` 3.

The same lists of section 4.3 give the paint entries, Minion's from its own
select palette.

## 6. Recipe for the port

1. Offline or at start-up: for each car type, the paint mask of section 4
   (and the recolour of 4.1); team colours as hues.
2. Battle load (`sessionLoadArenaAndCars`): after each
   `bitmapParseContainer(carTms, n)`, if car n has a team colour, recolour
   a copy of image 0's palette and either reuse the (type, team) row from
   the port's cache or `vramUploadClut(256, 1, pal, 0, n)` (3.1). Keep the
   rows in the port's own table (the slots are gone after the load).
   Optionally way C for one team per type, and option 2 of
   `SIXTEEN_CARS.md` 4.6 for more rows.
3. Nothing else in battle: `assetInstallImage` remaps each car's own DMD
   copy to its row, damage stages and wreck included.
4. Car select: on the colour button, `LoadImage` the team palette to
   (768, 23 + car) (one car), or patch the child's primitives per draw
   with a row from (768, 40..63) (several cars).

## Not checked

- Nothing was seen running: the slot override, the row choice and the look
  of the recoloured cars in game are from the code and from recoloured
  texture images, not from screenshots.
- The paint rules are judged on the flat texture, not on the lit model; the
  lighting (colour words, NCCS) multiplies the CLUT colour, so dark shades
  may need a brighter V'.
- Dark Tooth's palettes (several images) and Axel's frame image.
- Whether any other code reads a car's CLUT position (only the CLUT
  animation of 2.5 was found; a search for other `StoreImage`/`LoadImage`
  users in battle was not made).
- Discs of other regions.
