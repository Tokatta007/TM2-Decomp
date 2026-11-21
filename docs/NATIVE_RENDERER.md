# The renderer as native PC code (PC port request 47 item 4)

The port asked whether the renderer (`renderScene`, the model tree walker, the model emitter, the near pass) could run
as host C instead of recompiled MIPS inside the emulated RAM, reading the game's objects and models from the emulated
RAM and writing its primitives without PS1-sized limits. This is research: **no code was written for it.** File:line
refers to branch `decomp-requests-47b`. Tags: CONFIRMED (read in the code or measured), LIKELY, GUESS.

## Summary

- **Yes, it is feasible, and the C to start from exists.** The four hand-written asm routines have pure-C forms in
  `reference/`. They are **not byte-exact** (no plain C can reproduce SingleTrac's asm; they are 0x5CC bytes larger in
  all), but they are **proven equivalent statically** by the all-C build (`SOURCE_BUILD.md` "All-C flavour": every byte
  outside the 18 replaced sections equals E0). The walker and the near pass also matched the retail asm on random
  inputs; **the emitter (and the hdp walker) have never been run.** Everything else the renderer calls is
  byte-identical C in `src/`.
- **What host C needs**: a GTE layer (the C forms use raw `cop2` inline asm: 17 / 7 / 9 asm statements) mapped to the
  port's `gte.c`; 32-bit guest addresses instead of C pointers (the host is 64-bit, the structures hold PS1 pointers);
  the scratchpad as `M.scratch`; and, to drop the fixed limits, host-side queues and a packet path the port's GPU code
  takes without the PS1's 24-bit OT links.
- **Effort (GUESS):** about 3-5 weeks of port work in four stages, plus about a week on the decomp side (a GTE macro
  header and a structure header the all-C build proves unchanged, a runtime test of the emitter's C). Stage 1 (walker,
  emitter, near pass as host C, byte-compared with the MIPS output each frame) is the bulk and the proof; the effect
  drawers are stage 2; the limits and float precision come last.
- **Biggest risks:** the emitter's C was never run; the drawers that change game state while drawing (`weaponArcsDraw`
  calls `rand()` and `carApplyDamage`, `streaksDraw` calls the flame and drop updates); every port hook that today wraps
  a MIPS function (redraws, views 3-4, precise vertices, the near threshold, the counters) has to be carried over.

## 1. The state of the C forms (CONFIRMED)

| Routine | retail (asm in `src/`) | C form | bytes, slot -> C | Proof |
|---|---|---|---|---|
| walker: `renderWalkModel`, `renderPushMatrix`, `renderMulTransMatVec` | `engine_render_scene.c` | `reference/renderer_walker_engine_render_scene.c` (822 lines with the 17 byte-identical functions of the unit; the walker about 360) | 0xCC0 + 0x68 + 0xD4 -> 0xAC0 + 0x88 + 0x128 | all-C static proof; **differential emulator test with random model trees** (with the near pass); **linked in the port variant** by `TM2_PORT_RENDER_SLOTS` (request 47, the all-C bytes but one immediate) |
| emitter `sub_800CDC54` | `model_emitter.c` | `reference/model_emitter_800CDC54.c` (458 lines) | 0xCBC -> 0xD38 | all-C static proof only: **"Not runtime-verified"** (its header) |
| near pass: `renderNearPolys`, `renderNearSplitTri`, `renderNearLinkPrims`, `renderNearSplitQuad` | `engine_math_near_pass.c` | `reference/near_pass_engine_math_near_pass.c` (568 lines; the pass about 270) | 0x7A0 + 0x54 + 0x204 + 0x84 -> 0x8B8 + 0xA8 + 0x220 + 0xE0 | all-C static proof; **differential emulator test with random polygon lists** |
| hdp walker: `hdpWalk` .. `hdpPassRun` (10 functions) | `engine_hit_detection.c` | `reference/hdp_walker_engine_hit_detection.c` (1,349 lines) | 0x12DC in all -> +1,272 | all-C static proof only; not run. **Collision, not drawing**: a native renderer does not need it (keep it MIPS) |

- "Exact" here means: the all-C link (`build.py --source-only --relocatable --all-c`) proves that replacing the 18
  asm sections by these C objects changes nothing else in the image (`verify_reloc.py --e0`: every other byte equal to
  E0 once un-relocated, the relocations into the replaced code resolved by name). It does **not** prove the C does what
  the asm does; that is what the random-input tests are for, and they exist only for the walker and the near pass.
  The all-C flavour itself has never been run (`SOURCE_BUILD.md` lists what a comparison with retail would have to
  leave out: the scratchpad save areas, the two handler tables).
- Known differences from the asm (all deliberate, in the files' headers): the C keeps its saved registers on its stack
  (the asm used scratchpad words: emitter 0x00..0x2B and 0x8C..0x9F, near pass 0x00..0x37 / 0x40 / 0x4C, walker 0x40 and
  0x4C..0x7B); the walker dispatches node types with a `switch` (the asm's handler table holds only tokens); t8/t9 and
  a2/a3 are not preserved; the emitter's raw `div` is `HWDIV()` (R3000 results, no trap); the asm's trapping `add`s are
  plain adds (retail would crash where the C wraps).
- The rest of the renderer is byte-identical C in `src/` (`functions.tsv` form `c`, identical 1): `renderScene`
  (992 bytes), `renderInitState` (180), `renderFlipFrame` (60), `renderDrawModel` (320), `csUpdMat` (348), `viewProc`
  (1,080), `animUpdateAll` / `animUpdateOne` (304 / 244), `sub_8014215C` = `SetDrawMode` (88).

## 2. What compiling them as host C needs

### 2.1 The GTE

The C forms reach the GTE with inline asm, which only a MIPS compiler accepts (the emitter has `#error This unit
requires the PSX GTE` off MIPS):

| unit | asm statements | GTE commands | transfers |
|---|---|---|---|
| emitter | 17 | RTPT 0x280030 (x2), RTPS 0x180001 (x2), NCLIP 0x1400006, NCCS 0x108041B, MVMVA 0x41E012 / 0x49E012 (the translation, split >> 15 / & 0x7FFF) | ctc2 12, lwc2 14, mtc2 13, mfc2 21, swc2 7, cfc2 1 |
| walker | 7 (macros `gte_ldR`, `gte_ldV0`, `gte_ldIR`, `gte_rtv0`, `gte_sqr0`, `gte_stIR`, `gte_stMAC`) | MVMVA 0x486012 (child matrices), SQR 0xA00428 (LOD distance) | ctc2 5, mtc2 5, mfc2 7 |
| near pass | 0 (pure integer C: screen-space midpoints of xy / uv / rgb) | none | none |
| hdp walker | 9 | MVMVA x3, SQR | (not needed) |

The way to make one source serve both: a header of GTE macros, `TM2_GTE_CTC2(reg, value)`, `TM2_GTE_MTC2`,
`TM2_GTE_MFC2`, `TM2_GTE_CFC2`, `TM2_GTE_LWC2(reg, address)`, `TM2_GTE_SWC2(reg, address)`, `TM2_GTE_CMD(op)`:

- on MIPS (the all-C build) they expand to the same inline asm, so the all-C objects stay byte-identical (provable: the
  build compares the objects);
- on the host they call the port's runtime: `gte_write_ctrl(c, reg, v)`, `gte_write_data`, `gte_read_data`,
  `gte_read_ctrl`, `gte_command(c, op)`, and **`swc2` as `gte_store_data(c, ea, reg)`**, which is what keeps the port's
  precise (subpixel) vertices working: `gte.c` records the float of each SXY stored at a guest address
  (`RENDER_PLAN.md` 4). Using the port's `gte.c` (on `M.cpu.gte_*`) also shares the GTE state with the MIPS code around
  it: `viewProc` and `csUpdMat` (still MIPS, or host C later) load the camera and the slot matrices through libgte
  (`SetRotMatrix`, `SetTransMatrix`, `MulRotMatrix0`, `SetGeomOffset`), and the emitter reads them from the GTE.
- The asm's hazard `nop`s become nothing on the host.

### 2.2 Pointers into the emulated RAM

- The C forms use real C pointers in their structures (`Mesh.verts`, `DrawEntry.mesh`, `RenderState` words 0x04 /
  0x18 / 0x20, `WalkEntry.node`, the OT links). On a 64-bit host these are 8 bytes, so every structure that overlays
  game memory changes layout. The host form must keep **32-bit guest addresses** (`u32`) in every structure and
  convert at each dereference: `G(a) = M.ram + (a & 0x00FFFFFF)` (the port's guest RAM is 16 MB, contiguous,
  `GUEST_RAM_BYTES`; `mem_direct` in `psx_cpu.h`) and the scratchpad `M.scratch + (a - 0x1F800000)`.
- That is the bulk of the work: mechanical, but on every load and store of about 1,000 lines of walker, emitter and
  near pass (and of each drawer in stage 2). A macro set (`TM2_P(T)` = `T *` on MIPS, `u32` on the host;
  `TM2_LD32(a, off)` / `TM2_ST32`) would let one source compile both ways, at the cost of readability.
- Byte order and alignment are the same on x86-64 / ARM64 (little-endian, the game's records are aligned).
- The data the renderer reads lives at addresses that change per build (the port variant's `.port_bss`, the slot
  arrays with `TM2_PORT_RENDER_SLOTS`): the host code must take them from the build's `symbols.tsv` (as the port
  already does with `port_sym`), or from the RenderState words that `renderInitState` writes each frame (the walker
  and the emitter already read the arrays from there).

### 2.3 The scratchpad (0x1F800000, `RenderState`)

- `renderInitState` fills it each frame; the walker, the emitter and the near pass share it (the slot counter +0x08,
  the mesh queue +0x20, +0x3C, the near threshold +0x40, the overflow list +0x4C, the draw modes +0x60 / +0x6C, the
  cull flip +0x88, the walker's node stack from +0x7C (32 entries), the near list from +0xA4 (71 entries)).
- For stage 1 keep it **in `M.scratch`**, so the host code and the MIPS code see the same words and the frame compare
  can include it (masking the save areas of section 1). Later the host walker's stack and the near list can move to
  host arrays without the 32 / 71 / 200 limits (the walker's overflow stack is `renderSharedWorkBuffer`, 200 entries).

### 2.4 The order tables and the packets

- The emitter and the near pass write PS1 GPU primitives (the GP0 command words the port's PSY-Z path consumes) and
  link them into a 4,096-entry OT with 24-bit links (`ADD_PRIM` / `LINK` keep the entry's top byte, the packet length
  in the packet's top byte). The port's `gpu_dma` walks those lists in guest RAM.
- Stage 1: **keep the same format in guest RAM** (the frame records' buffers, `portPacketBuffers`). Then the port's GPU
  path, its precise vertices (keyed by the vertex words' guest addresses), its OT logs and the byte compare all work
  unchanged.
- Without fixed limits (stage 3): a host packet arena (growing vector) and a host OT, submitted to PSY-Z directly
  (`Psyz_GpuWriteGP0` per primitive in OT order) instead of through `gpu_dma`. Then there is no 24-bit or 16 MB
  limit, and the OT could get more entries than 4,096 (finer sorting at long distances). The precise vertices would
  then come straight from the emitter's floats (no shadow table lookup by address).

### 2.5 Fixed-point precision

- Stage 1 must stay **integer-exact** (the GTE's own arithmetic through `gte.c`, the C's `/ 4096` toward zero, 32-bit
  wrap: compile with `-fwrapv` or keep the arithmetic in `int32_t` / `uint32_t`, `HWDIV` for the raw divides,
  arithmetic right shifts of negative values, which every host compiler does).
- Later the host form can compute in float where it helps (projection, the translation split, the near pass's
  midpoints, which are integer screen midpoints today and the cause of the near-camera warping), as long as the
  integer path stays for the compare and for `wobble = ps1`.

### 2.6 The matrix slots and queues

- Today's limits are already raised inside the MIPS: matrix slots to `TM2_PORT_RENDER_SLOTS` (up to 2,048), meshes to
  `portMeshListCap` (2,048), packets to `portPacketBufferBytes` (390,320 a view). On the host these become vectors:
  the slot arrays (`renderSlotMatrixPtrs` 16 B, `renderMatrixSlots`, `renderObjectMatrices`,
  `renderFrustumMatricesA/B` 32 B a slot), the mesh queue (32-byte draw entries), the near list, the walker's stack.
- `csUpdMat` (byte-identical C, 348 bytes) writes the slot it is given; `renderScene` hands out the slot numbers.
  Both would move with the walker (renderScene and csUpdMat as host C too), or the slot arrays stay in guest RAM.

## 3. The other drawers

All are **byte-identical C** in `src/` already (sizes are the MIPS bytes, `out/TM2_SOURCE_E0/functions.tsv`). None of
the effect drawers uses the GTE: they project in integer C (`mathMulMatVec` + the view's projection scales); only
`fxDebrisDraw` uses libgte.

| drawer | bytes | unit | GTE / libgte | game state while drawing | native? |
|---|---:|---|---|---|---|
| `renderScene` + `renderInitState` + `renderFlipFrame` + `renderDrawModel` (+ `renderDrawObjectUnculled` 152) | 992 + 180 + 60 + 320 | `engine_render_scene.c`, `engine_anim_hit_volumes.c` | `SetDrawMode` | `animUpdateAll` (advances animations: game state) | stage 1 (with the walker) |
| `csUpdMat` | 348 | `engine_clut_anim.c` | `MulRotMatrix0`, `SetRotMatrix`, `TransposeMatrix` | writes the slot matrices | stage 1 or keep MIPS |
| `viewProc` | 1,080 | `view_system.c` | `RotMatrixYXZ`, `SetRotMatrix`, `SetTransMatrix`, `SetGeomOffset`, `MulRotMatrix0` | the view's matrices (the port wraps it: `ui_ViewProc`) | keep MIPS (cheap, hooked) |
| the sky | - | (a model: the camera-follow object, walked by `renderScene` at 801523E8) | through the walker / emitter | - | comes with stage 1 |
| `fxSpritesDraw` | 1,840 | `world_collapse.c` | none (`mathMulMatVec`, `ratan2` / `rsin` / `rcos`) | calls `fxImpactParticlesDraw` | stage 2 |
| `fxImpactParticlesDraw` | 992 | `car_damage_human.c` | none | - | stage 2 |
| `streaksDraw` | 1,308 | `screen_card_draw.c` | none | **calls `weaponDropsUpdate` (300), `weaponFlameUpdate` (1,636: the flame) and `weaponArcsDraw`** | stage 2, split update from drawing (the port's `four_streaks_draw_only` already does) |
| `weaponArcsDraw` (taser / lightning arcs) | 2,072 | `weapon_arcs_draw.c` | none | **`rand()` (8 call sites), `carApplyDamage`, `fxExplosion12`, `fxSmokeC12`** | stage 2, only with its `rand()` calls in retail order (or keep MIPS) |
| `debrisDraw` -> `fxDebrisDraw` | 128 + 4,556 | `engine_clut_anim.c`, `engine_fileio_debris.c` | `RotTransPers4`, `NormalColorCol`, `MulRotMatrix0`, `SetRotMatrix`, `SetTransMatrix`, `SetLightMatrix`, `SetColorMatrix`, `SetBackColor`, `TransposeMatrix` | - | stage 2 (needs host libgte calls on `gte.c`) |
| HUD: `hudDrawSingleView` 3,348, `hudDrawSplitVertical` / `Horizontal` 3,268 each (C++), `hudDisplayRadar` 1,884, `hudRadarPlotCar` 1,636, `hudDrawTargetPanel` 1,152, `hudDrawNumber` 820 | ~15,400 | `hud_main.c`, `hud_split.cpp`, `hud_radar.c`, `car_destroyed.c`, `packet_digits.c` | none (2D `AddPrim`, `setXY`) | `hudMessageUpdate` (timers) | **not needed**: 2D, fixed counts, their own buffers |
| fonts: `fontPrint` 692, `fontSpritePrint` 816, `fontSpritePrintCenteredXY` 492, `fontStringWidth` 348, the XY wrappers | ~2,500 | `font_print.c`, `font_buffer.c`, `font_measure.c` | none | - | not needed |

## 4. A staged plan

**Stage 0 (decomp, about a week, GUESS).**

1. `include/semantic/gte_ops.h`: the GTE macros of 2.1, MIPS forms = today's inline asm. Rewrite the three C forms on
   them; the all-C build must give **the same objects** (a byte compare of the 3 units, the all-C proof unchanged).
2. `include/semantic/render_types.h`: the shared structures with their offsets checked by `static_assert`-style
   typedefs (as `rt_buffers.h` does): `RenderState` (scratchpad), the draw context / frame record, `DrawEntry` /
   `WalkMesh`, `Mesh`, the node formats (types 0-5, 9, 11, 12), `WalkEntry`, `NearEntry`, the light records, the object
   words the walker reads (+20..+31 translation, +32 / +64 / +96 matrices), with the pointer fields behind a
   `TM2_P(T)` macro.
3. A runtime test of the emitter's C (the same kind as the walker's: random meshes through the asm and the C,
   compared word for word: packets, OT, scratchpad outside the save areas). It is the one renderer routine whose C was
   never run.

**Stage 1 (port, about 1.5-2 weeks).** Host C for the walker, the emitter, the near pass, `renderScene`,
`renderInitState`, `renderDrawModel` (and `csUpdMat`), behind a port switch (e.g. `[render] native = off | compare |
on`):

- reading objects / models from `M.ram`, the scratchpad in `M.scratch`, the GTE through `gte.c`, the packets and OT in
  the frame record's guest buffers (as today);
- `compare`: run the MIPS `renderScene` and the host one on the same input each frame (save and restore what the
  first changes: the packet buffer from the record's cursor, the OT, the scratchpad, the slot arrays, the mesh queue,
  the animation state that `animUpdateAll` advances; or run the host one into a second record) and compare the packet
  bytes, the OT, the record's cursor and the scratchpad outside the save areas. Any difference is a bug in the host
  form (or in the emitter's C);
- the port's hooks around `renderScene` / the emitter carried over: `vplus/precise.c` (the near threshold at
  scratchpad +0x40), `render_stats.c`, the redraws (`interp.c`), views 3-4 (`port_views.c`), `portRenderStats`,
  `portFarDistanceEmit` (the GTE range pass for far meshes, FAR_DISTANCE.md 2), the far LOD scale, `portVisual` bits
  2-3 (the own car first, the shadows' override pair).

**Stage 2 (port, about a week).** The effect drawers of section 3 (`fxSpritesDraw`, `fxImpactParticlesDraw`,
`streaksDraw` drawing only, `fxDebrisDraw` with host libgte wrappers on `gte.c`); `weaponArcsDraw` only if its `rand()`
and damage calls stay in retail's order, else keep it MIPS. Same compare switch. The HUD and fonts stay MIPS.

**Stage 3 (port, about 1-2 weeks).** The limits and the precision: host vectors for the slots, the mesh queue, the
near list and the walker's stack; a host packet arena and OT submitted to PSY-Z directly (2.4); float projection and
near-pass midpoints in the precise mode (the integer path kept for `wobble = ps1` and the compare).

## 5. Risks

- **The emitter's C has never run** (section 1). Stage 0's test, or stage 1's compare, is its first runtime check.
- **Game state inside drawing**: `animUpdateAll` in `renderScene`, `weaponFlameUpdate` / `weaponDropsUpdate` in
  `streaksDraw`, `rand()` and `carApplyDamage` in `weaponArcsDraw`. A native drawer must make the same calls in the
  same order (the `rand()` stream, online rollback and the state hash depend on it), or leave them in MIPS.
- **The port's hooks** wrap the MIPS functions today (`src/overrides.json`, `guest_call` of 80152284 in the redraws, the
  near threshold wrap, the far-distance and counter calls of the port variant). The native path must do each of them,
  and the MIPS path must stay for `native = off` and for the compare.
- **Per-build addresses**: the port variant's storage moves with the defines; the host code must take every address
  from the build's maps or from the RenderState words.
- **64-bit layouts**: any structure with a C pointer that overlays game memory is wrong on the host (2.2).
- **Retail edge cases**: the asm's trapping adds (retail crashes where C wraps), the raw `div` results (`HWDIV`), the
  scratchpad save areas (a compare must mask them).
- **Payoff**: the limits are already raised inside the MIPS (2.6), so the gain is mainly precision (floats straight
  from the emitter), speed (no recompiled-MIPS overhead on the heaviest loops), and no PS1-sized buffers at all. A
  smaller first step with most of the precision gain: only the emitter and the near pass as host C (the walker stays
  MIPS: it is linked as C already with `TM2_PORT_RENDER_SLOTS`).

## 6. What the decomp can provide

- The GTE macro header and the three C forms rewritten on it, proven unchanged by the all-C build (stage 0.1).
- The structure header with checked offsets (stage 0.2), the node types and the scratchpad map of section 2.3.
- The emitter's runtime test (stage 0.3).
- On request: host-compilable copies of the byte-identical C the renderer calls (`renderScene`, `renderInitState`,
  `renderDrawModel`, `csUpdMat`, the effect drawers) with the pointer macros, and a list of every global each one reads
  or writes (for the compare's save / restore).
- For the hidden areas (`HIDDEN_AREAS.md` 5 (a)): a reviewed list of interior / underground nodes per level
  (`config/US/hidden_areas.tsv`) and a drawing bit that patches them for each view's walk (zones to their inside or both
  children, bands unbounded), and the true depth of range-pass meshes; the occlusion itself needs the port's depth test
  (on the precise path's per-vertex depth today, or stage 3's host depth), with the size limit lifted for polygons that
  have depth.
