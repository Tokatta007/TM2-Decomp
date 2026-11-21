# Far distance: 32,000 world units across the biggest arenas

Answer to the PC port's request 31 ("Farthest" shows one side of the biggest arena from the other;
online uses it for everyone). Sources: the emitter's and the walker's asm (`model_emitter.c`,
`engine_render_scene.c`), their C forms in `reference/`, the effect drawers' C, and the level files
`DMD\*.DMD` on the USA disc (read with `tools/dmd_lod_dump.py`'s reader). Tags: **CONFIRMED** = read
in code or counted in the data; **GUESS** = inferred, not run.

Units. A **world unit** is what `renderFarDistance` counts (4,800 in retail). Positions and model
coordinates are world units << the level's coordinate shift, which is 3 on every arena
(`renderWorldShift`, DMD header +0x10; CONFIRMED for all 14 level files). So 32,000 world units are
256,000 position units. The sky domes use shift 0 or 1 (opcode 12).

## Summary

- **The limit is the GTE, not the order table** (CONFIRMED). The emitter projects every vertex
  with RTPT/RTPS in the mesh's coordinate units. SZ is 16 bits (0..65,535) and the projection
  divides IR1/IR2, which saturate at ±32,767. At shift 3 that is **8,191 world units of depth**
  and **4,095 units sideways**. Past them a vertex projects to the wrong place and its depth
  saturates: everything past 8,191 lands in one OT slot (2,303). That is what the port saw "above
  about 8,000": at depth 8,000 the screen edge in mode 0 is 4,620 units sideways.
- **The order table is not the limit** (CONFIRMED). The model path does not use AVSZ3/AVSZ4 or ZSF3/ZSF4.
  The emitter takes the largest SZ of a polygon, shifts it right by the mesh's coordinate shift
  (`srav`, the "OTZ shift") to world units, then compresses it (linear to 1,024, then slopes 1/4,
  1/8, 1/32). It drops a polygon only when `otz - 1 >= 4095`, which is past **40,959** world
  units. 32,000 maps to slot 3,816. So no bigger OT and no other ZSF is needed. The cost is
  resolution: one slot per 32 world units past 16,384.
- **The change, implemented: a GTE range pass per mesh** (section 2). It runs in the port variant
  with `--port-define TM2_PORT_FAR_DISTANCE` while `portFarDistance` is non-zero. Between the
  walker and the emitter, each queued mesh whose bounding sphere does not fit ±32,767 GTE units
  gets its rotation scaled by 2^-k and its shift lowered by k. A projection does not change when
  the vertex and the translation are scaled together, so the screen position is right. The
  emitter's `z = SZ >> shift` stays in world units, so the OT mapping is unchanged. Meshes that
  fit (all of retail's 4,800 range) are left exactly as retail queued them.
- **No s32 overflow in the distance tests up to 32,767** (CONFIRMED, section 1). The LOD distance is
  unsigned (3 × 32,767² < 2³²). The far cull's `far << shift` is 524,272 at most. The walker's
  plane products overflow only past 65,535 world units from the eye.
- **Caps** (section 3). These are raised under the same define, each with its own setting.
  - `portMeshListCap`: up to 2,048 meshes per view in `portMeshList`. The walker asm is
    unchanged: its counter starts at 280 − cap.
  - `portPacketBufferBytes`: up to 390,320 bytes per frame buffer in `portPacketBuffers` (4 x retail's, **a buffer
    per view** since request 47 revised: views 1-4 and the mirror, two parities each; section 3).
  - The matrix slots (200 a view, 198 for transform nodes): raised by their own define, `TM2_PORT_RENDER_SLOTS`
    (request 47, section 3 "Matrix slots"), which links the walker's C form.
  - The emitter already ends at the frame record's end (+0x0C), not at a constant.
- **Other things drawn by distance** (section 4):
  - Fog: there is none (no DPCS; DQA/DQB/far colour are never set).
  - The sky is always at the back of the OT (its object's word 0 = 0xFFF is the emitter's OTZ
    override). It needs nothing.
  - The effect sprites (`fxSpritesDraw`, `fxImpactParticlesDraw`, `streaksDraw`) are culled at
    about 4,000 units. Under the define their cull follows the far distance, and they sort with
    the emitter's mapping.
  - Debris wraps past 4,095 units (16-bit vertices). Under the define it is skipped there
    instead.
  - Lightning arcs are a short-range weapon and stay as retail.
- **Not runtime-tested.** The variant links and passes its static proofs with and without the
  define. Nothing has run (section 7).

## 1. The limits (with numbers)

| limit | where | value | world units at shift 3 | at 32,000 |
|---|---|---|---|---|
| GTE SZ (depth), 16 bits | RTPT/RTPS in the emitter `sub_800CDC54` (800CDC54, asm) | 0..65,535 | 8,191 deep | wrong (saturates) → **fixed by the range pass** |
| GTE IR1/IR2 (the projection's x, y) | same; saturate at ±32,767 (sf=1, lm=0) | ±32,767 | 4,095 sideways (the mode-0 screen edge from depth 7,090) | wrong → **fixed by the range pass** |
| projection divide `H / SZ` | same; saturates when SZ < H/2 (H = 91..457, `viewModeTable` +0x08) | – | near plane (17 units at H 277) | the pass never scales a mesh that comes near the eye |
| emitter OT mapping (`z = maxSZ >> shift`, compressed) | 800CE00C / 800CE198 / 800CE298 / 800CE470 (`srav`), knees 1024/4096/16384; `renderCompressDepth` 8014D368 is the C copy | `otz - 1 < 4095` | 40,959 | slot 3,816, fits |
| AVSZ3/AVSZ4, ZSF3/ZSF4 | not used by the model path (the emitter takes the largest SZ) | – | – | – |
| far cull, opcodes 0/1 | 801528C0 / 801529C8: `depth < -r - (far << shift)` | s32 | `far << 3` = 262,136 at 32,767 (shift 4: 524,272) | fits |
| walker plane tests | `renderMulTransMatVec` 8015338C: 3 products with trapping `add`, ≤ 4,096·\|c\| | 2³¹ | \|c\| < 524,287 coordinate units = 65,535 world units | fits; the same nodes are tested in retail whatever the far distance |
| LOD distance d2, opcode 2 | 80152A14: per axis `(pos + t) >> shift` into IR (16 bits), `SQR`, sum, `sltu` | per axis ±32,767; sum < 2³² | per axis 32,767 | fits (the arenas reach about 16,500 units on an axis, sphere-padded) |
| mesh queue | `slti $20,$19,280` at 801528EC; `renderMeshList` 280 × 32 bytes | 280 per view | – | **raised: `portMeshListCap`** |
| matrix slots | opcode 5 `slti …,198` (80152C18), `renderScene` `< 200` | 198/200 | – | not by this define (the far distance adds level meshes, which mostly share their root's slot; objects are never far-culled); **raised by `TM2_PORT_RENDER_SLOTS`** (request 47, section 3) |
| packet buffer | `rtDoubleBufferInit` 800D1A84: `+ 0x17CF0` into each frame record's end | 97,520 bytes a frame, all views | – | **raised: `portPacketBufferBytes`** |
| near list | emitter, 200 entries (71 in the scratchpad) | – | polygons nearer than 70 (20 split) | unchanged |
| effect sprites, sparks, streaks | `z > 32000` / `za >= 32001`, OT index `z >> 3` | 32,000 position units | 4,000 | **follows the far distance** (section 4) |
| debris vertices | `fxDebrisDraw` 8014C19C: model vertex + offset in an SVECTOR | ±32,767 position units | 4,095 | **skipped past it** (section 4) |

**The arenas** (CONFIRMED, the level files' meshes walked through opcodes 4, 5, 12; boxes padded by
each mesh's bounding sphere, so upper bounds): Hong Kong x 16,548 × y 13,583, Los Angeles 14,322 ×
13,669, Amazonia 12,526 × 12,187, Antarctica 11,059 × 11,359, New York 9,014 × 9,014. The sky domes
(object id 2) reach 11,700 (Los Angeles, Paris, Hong Kong), 14,600 (Antarctica) and 8,200
(New York) world units from their centre. The port's route-point figures (31,800 for Hong Kong) are larger than these
boxes' diagonals: if they are in another unit, 32,000 world units still covers every arena.

**Why retail never hit the GTE limit** (CONFIRMED). With far = 4,800 a mesh is at most 4,800 + its
radius deep. Sideways, the widest view at 4,800 deep reaches 2,770 units, inside 4,095.

## 2. The change: the GTE range pass (implemented)

`portFarDistanceScale` (`src/semantic/port_variant_arrays.c`) runs in `portFarDistanceEmit`. That
is `renderScene`'s emitter call under the define, after every walk and after the far LOD's
`portFarLodEnd`. For each queued mesh (32-byte entry: node, translation, shift, object, slot, key):

1. **Bound.** `big = |node centre + translation| + radius` and `near = |…| - radius`, in the
   mesh's coordinate units (GTE units before scaling). The bounding sphere is the one the walker
   culls with: node +28..+36 and +40.
2. **k.** `k = 0` when `big <= 32,767`: every coordinate of the mesh then fits SZ and IR exactly,
   and the entry is not touched. Otherwise k is the smallest value with `big >> k <= 32,767`,
   capped by the mesh's shift (at most 3).
3. **Near guard.** k is lowered while `near >> k < 1,024`. So a mesh around the camera (a big
   ground piece, the sky dome) keeps retail's projection. The projection divide saturates below
   SZ = H/2 (229 at most), and the near pass (`renderNearPolys`, otz < 70) gets only unscaled
   meshes: a scaled mesh starts at least 256 world units away.
4. **Scale.** The entry gets:
   - a copy of its rotation slot with the 9 elements divided by 2^k (rounded since request 61; shifted right
     before). The emitter computes the translation from that matrix (`TR = R·t`, split >> 15 / & 0x7FFF), so the
     translation scales too;
   - (request 61) the translation corrected for that rounding (`portFarTranslate`, "Precision" below) and, for a
     mesh with sprites or beams, a copy of the mesh with their sizes divided by 2^k (`portFarSpriteMesh`, "Sprites
     and beams" below);
   - the same light matrix at the same index;
   - shift − k, so the emitter's `z = SZ >> shift` stays in world units;
   - a translation key of its own: `0x40000000 | k << 28 | the entry's index` (request 61; `key ^ k << 28`
     before). The emitter caches the rotated translation per key, and each scaled entry's translation is corrected
     for its own mesh centre. **The port reads k of a queued mesh** as `(key >> 28) & 3` when bit 30 of the key
     (draw entry +0x1C) is set; an entry without it was left as retail queued it (k = 0).

   The copies live in `portFarState` (512 slots: the view's at most 201 retail slots copied once,
   then one per (slot, k)). The emitter then reads `portFarState.rot` / `.light` instead of
   `renderMatrixSlots` / `renderObjectMatrices`. A view with no far mesh passes the retail arrays.

**Precision (request 61: the far buildings' wiggle, CONFIRMED by a model of the emitter's arithmetic).** The
scaled rotation keeps 12 − k fraction bits. The emitter projects `R'·(t + v)` where `t` is the mesh's translation
(the camera to the mesh frame's origin, which for a level mesh is the level's origin) and `v` the vertex. The
rounding error `D = R − 2^k R'` therefore multiplies the **whole distance** `t + v`, not the mesh's size: on screen
about `0.1 × 2^k` px (k = 3: up to 1.2 px, median 0.37 px; k = 1: up to 0.2 px). It changes as the camera turns
(the truncated elements change) and jumps when k changes, so at 60 FPS with the port's subpixel positions the far
buildings and trees wiggle. Before request 61 the elements were truncated (`>> k`), which also biased them.

**The fix (request 61, `portFarTranslate`).** The elements are rounded, and the entry's translation is moved so
that the **mesh centre** `c` projects exactly: `R'·t_s = R'·t + D·(t + c) / 2^k`, i.e.
`t_s = t + R^T·D·(t + c) / |R|²` (one Newton step: `R'^-1 ≈ 2^k R^T / |R|²`; `|R|²` is the rows' mean squared
length, 4096² for a rotation; all 32-bit, `D·(t + c)` split into its high and low 12 bits). What is left is
`D·(v − c) / 2^k`, the mesh's own extent instead of its distance. A matrix with an element past ±8,192 (an object
scaled more than 2 x) keeps its translation. Each scaled entry gets a key of its own (above), because entries that
share a key share the translation in the emitter's cache but not the centre.

| k (distance) | before request 61: median / 99% / max px | request 61: median / 99% / max px |
|---|---|---|
| 1 (5,000) | 0.07 / 0.16 / 0.21 | 0.03 / 0.04 / 0.05 |
| 2 (10,000) | 0.17 / 0.40 / 0.55 | 0.03 / 0.04 / 0.05 |
| 3 (20,000) | 0.37 / 0.89 / 1.17 | 0.03 / 0.04 / 0.05 |
| 3 (30,000) | 0.36 / 0.87 / 1.05 | 0.02 / 0.03 / 0.03 |

(A model of the emitter's integer arithmetic: the split translation, RTPS, against the exact projection; H 277,
random rotations, mesh radius 60 or 400 world units, 2,000 meshes each, the worst of 8 vertices; the residual is the
GTE's own integer IR. `tools/check_custom.py` check 7 runs the compiled pass in a MIPS interpreter and measures the
same: 0.035 px worst on its 20 entries, 0.78 px for the pass before request 61.) No hysteresis on k is needed: with
the error near 0 at every k, a change of k moves a vertex by under 0.1 px.

**Sprites and beams (request 61, `portFarSpriteMesh`).** These are mesh primitive types 0 (point sprite, also
types 2, 5..7) and 1 (beam). The emitter (asm, kept) sizes them from the scaled depth: a sprite's half sizes
`hw, hh = S16(prim, 6 / 8) × H / SZ` (clamped 1..500), a beam's half width `q = S16(prim, 6) × H / SZ1` (clamped
1..2w). In a scaled mesh SZ is 2^k too small, so before request 61 they were 2^k too large, and flipped size when k
changed. Now a mesh that holds any is drawn from a copy (`portFarSprites`, rebuilt for every view): its node's first
44 bytes with the primitives pointer (+0x0C) moved to a copy of the primitives in which the sprites' +6 / +8 and the
beams' +6 are `round(w / 2^k)` (a beam's at least 1, so that its clamp 1..2w keeps a width). `w' × H / SZ_s` then
equals retail's `w × H / (SZ_s × 2^k)` up to that rounding. The beam's +8 is its half length in vertex units, which
the scaled matrix already scales, and stays. The emitter's clamps are retail's: 500 for a sprite; the beam's 2w only
binds below SZ = H / 2, which a scaled mesh (at least `FAR_GTE_NEAR` = 1,024 GTE units deep) never reaches. A mesh
drawn twice at one k in a view shares one copy (a 512-slot hash of (node, k)); at two k it has two. The copies are
made afresh for every view, so a primitive the game changes is copied as it is now. Out of room (131,072 bytes a
view; the biggest level holds 69,000 bytes of such meshes, `SWAMP`) a mesh keeps retail's sizes and
`portFarSprites.full` counts it. The near pass may get a primitive of a copy (a deferred one): the copy lives until
the next view's pass, after that view's near pass. Since the addendum below a copy's head is 48 bytes: the node's
first 44, then (+44) the source mesh's primitives pointer, which `portFarSpriteSource(mesh, prim)` follows to the
source primitive (the unrounded sizes).

**Precise corners (request 61 addendum, `TM2_PORT_C_EMITTER`; `PLAYTEST_ANSWERS.md` 4.3 item 1; the port's request 65 is
this, PR #86: the ends are the C emitter's exact fixed-point projection, not the GTE's float FIFO, so they do not
depend on the port's float GTE and sit within 0.2 px of the exact projection; `PLAYTEST_ANSWERS.md` 4.3).** What the
user's pickup video showed flickering is the label beam and the beams' integer width and tilt (q clamped to 1, s / c
truncated), not the sizes above. With the define the variant links the emitter's C form
(`reference/model_emitter_800CDC54.c`, integer outputs the asm's: `check_custom.py` check 8), and with the port's
`portPreciseEmit` its beam and point-sprite branches also put the four corners of each drawn quad, in 1/16 pixel, in
the port's precise ring (`PORT_PRECISE_PUT`'s format; `include/semantic/port_c_emitter.h`): the ends projected
exactly (the translation with the split's fraction, as the port's GTE keeps it), the beam's direction from their
difference, the half sizes `w H / (Z 2^k)` with k = (key >> 28) & 3 for an entry with key bit 30 and w the source's
(through the copy's +44), not clamped to 1, the x ones times the anamorphic factor `portPreciseEmit / 4096`. So a
scaled mesh's sprites and beams get their exact sizes in the port's precise modes whatever the copy's rounding, and
the integer corners (wobble PS1) stay as the asm made them.

**The data (CONFIRMED, every mesh of the disc's level files).** Paris has 164 meshes with sprites or beams (449
sprites, 100 beams; 35,524 bytes), all of radius 133 world units or less: the pickups are single-beam meshes
(radius 6 to 11, half widths 20 to 60, half lengths 16 to 59; two per pickup, one under an animation), the most
common sprite is 40 x 40 (292); the beams 80 x 200 and 96 x 120..144 look like the trees' and lamps' camera-facing
quads (GUESS from their sizes).
Such a small mesh is scaled only past about 3,960 world units. There its sprites are a few pixels: at H 277 a beam
of half width 96 is 0.8 px (1 px after the clamp) where the old pass drew 1.7 (still 1); at a larger H, or for the
larger sprites (Moscow's 344-wide, Los Angeles' 570, Amazonia's 619, Antarctica's 4,000 to 6,000 in meshes of radius
1,400 to 3,200), the old pass drew 2 to 8 times retail's size, and switched as the mesh's k changed with the
distance: the pulsing pickups and (GUESS: if the trees are those beams) the trees' size flips. The far buildings'
wiggle is the translation error above (CONFIRMED arithmetic).

**What the pass does not fix:**
- **A mesh that is near and far at once.** This is a huge piece reaching from the camera past
  8,191 units (Hong Kong has meshes of radius 6,792). It keeps retail's projection, so its far end
  can still be wrong. Splitting such a mesh is not possible in the pass.
- **The mirror** (view mode 4) keeps its 500.

## 3. Caps (implemented)

| cap | retail | setting (0 = retail) | storage | how |
|---|---|---|---|---|
| meshes per view | 280 (`slti` at 801528EC; `renderMeshList` 8,960 bytes) | `portMeshListCap` (1..2,048) | `portMeshList` 2,048 × 32 bytes | `portFarDistanceMeshListBegin`, right after `renderInitState`: queue pointer ctx+0x20 = `portMeshList`, counter ctx+0x3C = 280 − cap. The walker's `slti …,280` then allows `cap` meshes. `renderScene` subtracts the bias from the final counter. |
| packet bytes per frame buffer | 0x17CF0 = 97,520 (`rtDoubleBufferInit`), shared by every view of the frame | `portPacketBufferBytes` (≤ 390,320) | `portPacketBuffers[5 views][2 parities]` of 390,400 bytes, 3,904,000 in all (`PORT_PACKET_BUFFER_FACTOR` 4 × retail's 97,600, 80 bytes of slack; `PORT_PACKET_VIEWS` 5: views 1-4 = 0..3, the mirror = `PORT_PACKET_MIRROR` 4. Request 47 revised; 8 × per view pair in request 47, 4 × shared from request 33-1, 8 × shared at first) | `portPacketBuffersApplyView(record, view, parity)` points one frame record's start (+4) and end (+0x0C) at its view's own buffer. `portPacketBuffersApply` at the top of each `rtMainLoop` frame does the six retail records (views 1-2 and the mirror); the port calls `portPacketBuffersApplyView` (or request 47's `portPacketBuffersApplySet(records, 4, 1)`, now a wrapper) for its records of views 3-4. `rtMainLoop` starts view 2's and the mirror's cursor through `portPacketBufferCursor` (its own start; retail's `= view 1's cursor` while the views still share retail's buffer). Setting the bytes back to 0 restores retail's buffers at the next frame. |
| matrix slots per view | 200 (`renderScene` `*count < 200`; slot 200 is one past the arrays, `RETAIL_BUGS.md` 13), 198 for a transform node's own slot (walker `slti 198`, 80152C18) | `--port-define TM2_PORT_RENDER_SLOTS=N` (build time, 201..2,048; bare = 400) | the five slot arrays (`render_slots.h`) with N + 1 slots each, in `.port_bss` (tsv group `renderslots+`) | the variant links the walker's C form with the limit N − 2; `renderScene` counts to N. Section "Matrix slots" below. |
| the emitter's end check | – | – | – | **Already reads the record's end** (CONFIRMED): `lw $17,12($4)` 800CDC94 → ctx+0x3C, tested before each primitive (`sltu` at 800CDF1C). `fxSpritesDraw`, `fxImpactParticlesDraw`, `streaksDraw`, `weaponArcsDraw` and `fxDebrisDraw` also test the record's +0x0C. Nothing to change. |

- **Why a counter bias for the mesh cap.** The cap is an immediate in the hand-written walker,
  which stays byte-for-byte retail (RULES.md). The walker writes entries through the pointer it
  keeps at ctx+0x20 and only compares the counter at ctx+0x3C. No other code reads that counter
  between `renderInitState` and the emitter (the emitter then reuses +0x3C for the packet end).
  The C walker in `reference/` (`S->end >= 280`, signed) behaves the same.
- **Why per frame for the packet buffers.** `rtDoubleBufferInit` runs once at boot, before the
  port's settings are known. Switching between frames is safe: the frame the GPU is drawing keeps
  its packets where they are. Only the next frame's cursor and end move. The records stay in
  retail storage at 8018118C (6 × 0x80). A port with more frame records calls
  `portPacketBuffersApplyView(record, view, parity)` for each (request 47 revised).
- **Why a buffer per view (request 47 revised).** One rule for every player count: each view (and the mirror) has
  4 x retail's to itself, whatever the others draw. The port's worst measurements (`RENDER_ANSWERS.md` R2.4) were
  387 KB for views 3-4 together (4 players, Antarctica, near scale 4: about 194 KB a view, half of 4 x) and 179 KB for
  view 1 and the mirror together. `rtMainLoop`'s view 2 and mirror now start at their
  own buffer's start instead of going on after view 1 (`RT_PACKET_CURSOR`, `portPacketBufferCursor`).
- **The order in which things drop** is unchanged when a cap is still reached. The level comes
  first and the objects last (FAR_LOD.md 3), so the cars still go first. Size the caps from the
  measurements (section 6).
- **Not raised:** the near list (200) and the walker stack (200). The far distance adds level meshes, not
  transforms; objects are never far-culled, so their number does not change. The matrix slots are raised by their
  own define (below).

### Matrix slots (request 47, `TM2_PORT_RENDER_SLOTS`)

The port measured up to 185 slots in a 4-player view with ordinary cars (194 estimated with Minions), against 198
(`RENDER_ANSWERS.md` R2.4). Every place that knows the limit (CONFIRMED, the C, the asm and the relocation table
`config/US/retail_relocs.tsv`):

| what | where | retail | with `TM2_PORT_RENDER_SLOTS=N` |
|---|---|---|---|
| object / world-model / sky slot counter | `renderScene` (`engine_render_scene.c`, `*count < 200` three times; twice more in `portVisual` bit 2's own-car walk) | 200 | `TM2_RENDER_SLOT_LIMIT` = N |
| a transform node's own slot | walker opcode 5, `slti $18,$23,198` (80152C18, asm) | 198 | `RENDER_WALKER_NODE_SLOTS` = N − 2, in the C walker |
| the slot arrays | `renderSlotMatrixPtrs` 801ABA50 (16 B a slot, RenderState +0x04), `renderMatrixSlots` 801AEB18 (+0x18), `renderObjectMatrices` 801F8AB8 (+0x1C), `renderFrustumMatricesA` 801B2E0C (+0x44), `renderFrustumMatricesB` 801B470C (+0x48), 32 B a slot | 200 (220 for the pointers) | N + 1 slots each in `.port_bss`, after `portRenderStats`; every import of them is redirected (they are reached only at their bases) |
| the scratchpad RenderState | `renderInitState` writes the arrays' addresses to +0x04 / +0x18 / +0x1C / +0x44 / +0x48 each frame; `renderCtx` only points at 0x1F800000 | – | nothing to move: the pointers follow the arrays |
| the emitter (asm) | reads the rotation and light matrices by a mesh's slot (draw entry +0x18) from its 4th / 5th arguments | no limit | unchanged |
| the near pass (asm) | no slot use (its 200 is the near list) | – | unchanged |
| `csUpdMat`, `renderDrawModel`, `renderDrawObjectUnculled`, `lightAddSources`, `dbscanInitStacks`, `renderGetMatrixSlots` | an array base or slot 0 | – | unchanged (relocations) |
| the GTE range pass | `portFarState`: `PORT_FAR_SRC_SLOTS` = N + 2 rows, N + 312 copies | 201 / 512 | sized by the same limit (tsv expression) |
| the render counter | `portRenderStats` cap bits 0x1 / 0x2 | 198 / 200 | N − 2 / N (`port_render_stats.h` takes them from `render_slots.h`) |

**The walker.** The 198 is an immediate in the hand-written walker, which stays asm in the byte-matching builds
(`RULES.md`). The `--all-c` flavour cannot be combined with `--port-variant` (`build_source_only.py` refuses it) and
its reference copy of the unit has an old `renderScene` without the port's hooks, so the variant does not swap the
whole unit: `src/semantic/engine_render_scene.c` keeps its C and, under the define, `#include`s only the walker part
of `reference/renderer_walker_engine_render_scene.c` (`TM2_RENDER_WALKER_ONLY`; the reference file keeps its line
numbers and compiles to the same object as before, so the all-C build is unchanged). The variant's three walker
functions are then **the all-C build's code byte for byte except the one immediate** (`slti $2,$9,398` for N = 400;
with the limits forced to retail by the placement link, identical), the copy the all-C flavour proves statically and
that matches the retail asm on random inputs (`SOURCE_BUILD.md` "All-C flavour"). `renderInitNodeHandlers`
stores the C walker's entry in every table slot (the C walker dispatches with a `switch` and never reads the table),
as the all-C link resolves the asm's labels. The C walker keeps its registers on the stack instead of the scratchpad
words 0x40 and 0x4C..0x7B; every reader of those words (`renderScene`, the emitter: 0x40, 0x4C, 0x60..0x74, 0x88)
writes them first (CONFIRMED: the emitter's first access to every scratchpad word it reads before writing is one of
those). Retail, the default variant and every variant without the define keep the asm.

## 4. Other things drawn by distance

| what | retail | under the define (portFarDistance ≠ 0) |
|---|---|---|
| fog / depth cue | **none** (CONFIRMED): the emitter lights with NCCS and never DPCS; `SetDQA`/`SetDQB`/`SetFarColor` (8014A280/8014A28C/8014A1C8) have no callers | nothing to do |
| sky dome (object id 2, `viewSetFollowObj` 80156344) | follows the camera; drawn in the far pass with the far distance (it surrounds the eye, so it is never culled); **always at OT slot 4,095**: its object's word 0 is set to 0xFFF, and the emitter uses an object's word 0 as the OTZ override (`lw $19,0($17)` 800CDED8 → ctx+0x80) | nothing: far geometry (slots ≤ 3,816) is drawn over it. Its dome (radius 8,100..14,600 units, shift 0/1) fits the GTE and keeps k = 0 (the eye is inside it) |
| backdrops (fixed OTZ 4,090..4,095 in the mesh flags; object id 3 objects with word 0 = 0xFFE) | at the back | nothing: still behind every far polygon |
| `fxSpritesDraw` (80110BE4, 150 effect sprites), `fxImpactParticlesDraw` (80112F6C, 20 sparks), `streaksDraw` (800D8230, 40 tracers) | culled past camera z 32,000 position units (4,000 world units); OT index `z >> 3`, linear | cull at `portFarDistance << 3`. OT index = the emitter's mapping of `z >> 3` (`portFarDistanceFxOtz`), at most 4,094 |
| `fxDebrisDraw` (8014C19C, the chunks of `debrisDraw`) | 16-bit vertices: past 4,095 units a piece wraps across the screen | a piece whose offsets pass ±30,000 position units is skipped (`portFarDistanceDebrisFits`) |
| `weaponArcsDraw` (lightning) | segments with `z/4 < 4091` (2,045 units), drawn at OT 0 | unchanged (a weapon effect between nearby cars) |
| LOD bands | model data (FAR_LOD.md) | use `TM2_PORT_FAR_LOD` with it, or the arena still ends at the bands' ends |

**The effect sprites' sort** (CONFIRMED arithmetic, effect GUESS). Retail sorts a sprite at
`z >> 3` while the geometry at the same depth sorts at the compressed index. At 2,000 units a
sprite is in slot 2,000 and the geometry in slot 1,268. The OT draws from slot 4,095 down, so a
sprite past 1,024 units is painted before, and so under, geometry that lies behind it. With the
define on, a sprite sorts in the same slot as geometry at its depth. This changes what retail
shows between 1,024 and 4,000 units. It is a fix if the arithmetic is the whole story, which only a
run can confirm.

## 5. What the port sets

**Build:** add `--port-define TM2_PORT_FAR_DISTANCE`, best with `TM2_PORT_FAR_LOD`:

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_HOOKS --port-define TM2_PORT_FAR_LOD --port-define TM2_PORT_FAR_DISTANCE --output work/pv/P4
```

It changes 6 functions, which the placement check accepts as `port_changed`:
- `renderScene`;
- `rtMainLoop`;
- `fxSpritesDraw`;
- `fxImpactParticlesDraw`;
- `streaksDraw`;
- `fxDebrisDraw`.

With `TM2_PORT_FAR_LOD` too, add `worldLoadStartObjects` and `assetInstallImage`. The define adds
11 functions to `.port_text`:
- `portFarDistanceFar`, `portFarDistanceMeshListBegin`, `portFarDistanceEmit`;
- `portFarDistanceFxLimit`, `portFarDistanceFxOtz`, `portFarDistanceDebrisFits`;
- `portPacketBuffersApply`, `portPacketBuffersApplyView`, `portPacketBuffersApplySet`, `portPacketBufferCursor`
  (request 47 revised);
- 4 static helpers: `portFarIsqrt`, `portFarLength`, `portFarDistanceScale`, `portFarCompress`; request 61 adds
  `portFarTranslate` and `portFarSpriteMesh` (static, called by `portFarDistanceScale`), its addendum
  `portFarSpriteSource` (called by the C emitter's precise corners, `TM2_PORT_C_EMITTER`).

**Precise corners (request 61 addendum):** add `--port-define TM2_PORT_C_EMITTER` too; it changes `sub_800CDC54`
(the C emitter) and adds its static helpers `portPrecise*` to `.port_text`. The port then sets `portPreciseEmit`
(`.port_bss`, after `portFarSprites`) every frame: 0 with wobble PS1 (nothing written), else `round(4096 / k)` with k
its widescreen stretch (4096 without), and takes the ring entries as it takes the effect sprites'
(`PLAYTEST_ANSWERS.md` 4.3 item 1).

**Settings** (`.port_bss`, group `one`; addresses of the 4-player variant, unchanged by request 47 (every
configuration of `CAR_GROUPS.md` 13.11 has them there), from `out/TM2_SOURCE_PORT/symbols.tsv`. They move with `--port-limits` and with storage added before
them, so read them from the map):

| variable | address | value for Farthest |
|---|---|---|
| `portFarDistance` | 0x802091DC | 32,000 (any 1..32,767; 0 = retail 4,800) |
| `portMeshListCap` | 0x802091E0 | 2,048, or what the measurements need (0 = 280) |
| `portPacketBufferBytes` | 0x802091E4 | 390,320, or what they need (0 = 97,520; request 47 revised: per view; the worst pair measured 387 KB for two views) |
| `portMirrorFar` | (request 46: after `portCarDtParkMask`, read the map) | the rear-view mirror's far distance, e.g. `portFarDistance` (0 = retail: 500 for the level and the world models, 1,000 for the objects) |
| `portMeshList` | 0x802091E8 | storage (65,536 bytes) |
| `portFarState` | 0x802191E8 | storage (34,392 bytes) |
| `portFarSprites` | (request 61: after `portDmdImages`, read the map) | storage (139,280 bytes: `struct PortFarSprites`, the view's sprite and beam mesh copies; `.copies` / `.full` count this view's copies and the meshes left at retail's sizes) |
| `portPacketBuffers` | 0x80221840 | storage: `[view][parity]` of 390,400 bytes, 3,904,000 in all (request 47 revised; 3,123,200 in request 47; 780,800 from request 33-1; 1,561,600 before). With `TM2_PORT_RENDER_SLOTS` `portFarState` grows and this moves (0x802250C8 at N = 400) |
| `portPacketBuffersApplyView` | `.port_text` (read the map) | the port calls it each frame for each of its records of views 3-4 (view 2 / 3, parity 0 / 1); `portPacketBuffersApplySet(records, 4, 1)` does the same |
| `portPacketBufferCursor` | `.port_text` (read the map) | a record's packet cursor after the view before it (its own start with a buffer of its own) |

- **The mirror (request 46 item 9, `PLAYTEST_ANSWERS.md` 15).** View mode 4 never used the far distance: its level
  pass is cut at 500 units and its objects at 1,000 (`renderScene`). `portMirrorFar` (1..32,767) replaces both through
  `portFarDistanceMirror` (`.port_text`); its packets came out of the same frame buffer after view 0's, and have a
  buffer of their own since request 47 revised.
- **Setting the variables.** All three can change at any time. They act from the next view (the
  far distance and the mesh cap) or the next frame (the packet bytes). They must be zero at boot,
  like the rest of `.port_bss`.
- **Retail's own far variable.** Write `portFarDistance` rather than `renderFarDistanceDefault`
  (the port's current 8,000). The default still serves the menus' model draws.
- **Layout.** The storage is in every variant, with or without the define, so the variant's layout
  does not depend on it. `.port_bss` is now 80200610 + 0x19F3F0 (it grew by 1,661,600 bytes), so
  `.port_text` starts at 0x8039FA00 (was 0x80209FA0) and the default variant's EXE changed
  (`11100afc…`), not its code.

## 6. What to measure

**Measured (the port, 2026-10-13).** Packet bytes per frame (all views of the frame, far distance on):

| players | arena | bytes per frame |
|---|---|---|
| 4 (16 cars) | Antarctica (two runs) | 303 KB, 262 KB |
| 4 | New York | 254 KB |
| 4 | Hong Kong | 194 KB |
| 2 | (several) | 91 KB .. 170 KB |

The worst, 303 KB (16 cars, 4 players, Antarctica), is 78% of 4 x retail's buffer, so the storage was cut from
8 x to **4 x retail's** (`PORT_PACKET_BUFFER_FACTOR` in `port_variant_arrays.c`; 2 x 390,400 bytes, saving
780,800 bytes of port RAM); `portPacketBufferBytes` now goes up to **390,320** (larger values act as 390,320).
A port that needs more raises the factor (a rebuild; the layout of `.port_bss` after `portPacketBuffers` moves).

**Request 47 (the port, 2026-10-18, `portRenderStats`):** 4 players on Antarctica at Farthest with near scale 4 used
377 KB for views 1-2 and 387 KB for views 3-4 (97-99% of 4 x), the views 3-4 in a buffer of their own. The factor is
8 again, and there is a set of buffers per view pair: `portPacketBuffers[2 sets][2 parities]`, 780,720 usable bytes
each (section 3; `PLAYTEST_ANSWERS.md` 19).

**Request 47 revised (2026-10-19):** a 4 x buffer per view (views 1-4 and the mirror, two parities each:
`portPacketBuffers[5][2]`, 390,320 usable bytes each), the same for every player count; the worst pair measured 387 KB for
two views, about half of 4 x a view (`PLAYTEST_ANSWERS.md` 23).

On Hong Kong (`HKONG`), Antarctica (`GLACIER`) and Amazonia (`ISLANDS`), at the far corners, looking
across, with 1, 2 and 4 views:
- **Frame time:** ticks per frame (`GetCurTics` around the loop, or `rtFieldCounter`). The loop
  waits for 15 or 20 ticks.
- **Meshes per view:** `renderCurrentOt` after `renderScene` is the number queued. It hitting
  `portMeshListCap` means meshes were dropped.
- **Packet bytes:** the frame record's cursor (+8) minus its start (+4) after the view (since request 47 revised
  each view's own; with retail's buffer after the last view). It near `portPacketBufferBytes` means the emitter
  stopped early.
- **Matrix slots** (0x1F800008, ≤ 200) and **near list** (0x1F800038 after the emitter, ≤ 200).
- **Built (the render capacity counter):** with this define `renderScene` records the slots, the meshes and the
  packet bytes of every view, their high-water marks and the caps reached in `portRenderStats`
  (`RENDER_ANSWERS.md` R2.4, which also gives the worst-case test).
- **How many meshes the range pass scaled.** Count the entries whose key has bits 28..29 set
  (temporary instrumentation).
- **Visual checks:** far geometry against the sky; sprites and beams on far meshes (section 2,
  "does not fix"); effect sprites between 1,024 and 4,000 units against walls behind them
  (section 4).

## 7. Status

- **Built and proved** (`SOURCE_BUILD.md` "Port variant"; objects of `work/build-20261012-141309-487038`).
  The retail images keep their hashes: hybrid `9219f7ba…`, source-only `d755c86e…`, with
  `--sdk-from-lib` `f557c1e3…`. The variant passes its proof against E0 and the placement check
  (limits forced to 2) in three configurations:
  - without the define: `11100afc…`, published as `out/TM2_SOURCE_PORT.EXE`;
  - with `TM2_PORT_FAR_DISTANCE`: `ad8f8d5c…`;
  - with `TM2_PORT_FAR_LOD` and `TM2_PORT_FAR_DISTANCE`: `4a2e7d98…`.

  The placement check's `port_changed` functions are exactly the ones listed in section 5.
- **Request 33-1 (2026-10-13):** the packet buffers cut to 4 x retail's after the port's measurements (section 6);
  proved with the configurations of `CAR_GROUPS.md` 13.11.
- **Request 47 (2026-10-18):** the packet buffers 8 x retail's, a set per view pair; `TM2_PORT_RENDER_SLOTS` (the C
  walker and the slot arrays in `.port_bss`). Proofs and addresses in `PLAYTEST_ANSWERS.md` 22.
- **Request 47 revised (2026-10-19):** a 4 x packet buffer per view (`portPacketBuffersApplyView`,
  `portPacketBufferCursor`); proofs and addresses in `PLAYTEST_ANSWERS.md` 26.
- **Request 61 (2026-10-25):** the sprites' and beams' sizes in scaled meshes, the translation corrected for the
  scaled matrix's rounding, rounded elements, the entry's own key (section 2); `tools/check_custom.py` check 7 runs
  the compiled pass in a MIPS interpreter (the projection within 0.1 px, the sizes, the copies, the stores; the pass
  before request 61 fails it). Full build `work/build-20261025-065021-192570` (retail hashes unchanged; the default
  variant `f6f673ef…`, moved by `portFarSprites` only); links `work/pv/r61_{rb,port,allh16}_{P4,P2}` prove against
  its E0 and pass the placement check; `tools/check_all.py` 27/27. In the 4-player port link (`r61_port_P4`):
  `portFarSprites` 0x808ACA6C (after `portDmdImages` 0x808AC968), `portFarTranslate` 0x808D0548,
  `portFarSpriteMesh` 0x808D07B0, `portFarDistanceScale` 0x808D0A6C, `portFarDistanceEmit` 0x808D0F08 (read the
  port's own map). No game function changes for it.
- **Not runtime-tested.** The range pass is checked by reading (the emitter's use of the slot, the key and the
  shift) and, since request 61, by running its compiled code in check 7's interpreter with the emitter as a stub. So are the mesh cap's counter bias (that no other code reads
  ctx+0x3C during the walks) and the packet-buffer switch.
