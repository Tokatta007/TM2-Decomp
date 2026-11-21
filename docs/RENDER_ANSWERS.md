# Render answers: near polygons, screen-position writers, buffers and caps

Answers questions R1 and R2 of the PC port's request 16 (`docs/DECOMP_REQUESTS.md` item 16,
`docs/RENDER_PLAN.md` section 7). R3 is answered further down in this file, and R4 (request 80, one-frame
glitches) at the end.

How this was checked: static analysis only. Sources were the C in `src/semantic` and `reference/`,
the retail disassembly (`bin/SCUS_94306R.EXE`, `mipsel-none-elf-objdump`, load address 0x800CDC54,
gp = 0x801809D4), and the level files `DMD\*.DMD` read from the USA disc image. Nothing was built
or run. Read this with `WIDESCREEN.md`, `DRAW_DISTANCE.md` and `MAIN_LOOP.md`.

Tags: **CONFIRMED** = seen in code or data and understood. **LIKELY** = inferred; check it before
relying on it.

"ctx" means the renderer record in the scratchpad at 0x1F800000 (`RenderState` in
`engine_render_scene.c`). Names are the current ones from `config/US/names.tsv`.

## R1. Near polygons, and every other write of screen positions into primitives

### R1.1 Short answer

- **The near pass works only in integer screen space (CONFIRMED).** `renderNearPolys` (80154690)
  never touches the GTE. It reads the primitive words that the emitter `sub_800CDC54` already
  wrote: the packed screen XY, UV and RGB. It splits each polygon into 4 by averaging those words
  at the edge midpoints, `(a + b) >> 1` per field. Nothing is reprojected with RTPS, and no 3D
  midpoints are computed.
- **It is a texture-distortion fix, not a depth fix (CONFIRMED mechanism, LIKELY purpose).** All 4
  pieces go into the same OT entry as the parent polygon, and no depth is kept per vertex. The
  subdivision only shortens the affine texture spans of big polygons close to the camera.
- **With perspective-correct texturing the subdivision is no longer needed.** A port that
  replaces the pass must still link the original primitive, with its draw-mode wrappers, because
  the emitter never links a deferred primitive itself (R1.3).
- **Only two code paths put GTE screen positions into primitives:**
  - the model emitter `sub_800CDC54`, which writes polygons with `swc2`, and sprites and beams with
    `mfc2` or `swc2` followed by `sh`;
  - `fxDebrisDraw` (8014C19C), through libgte `RotTransPers` / `RotTransPers3` / `RotTransPers4`.
- **Four drawers project in software** (C, integer divides, the same view tables): tracers,
  lightning, effect sprites and impact particles.
- **Every other primitive XY write is fixed 2D:** the HUD, the mirror frame, the radar, text and
  the full-screen quads.
- No HUD element is placed at a projected world position (CONFIRMED: no caller of the projection
  helpers or of the GTE projection outside the list in R1.4).

### R1.2 How `renderNearPolys` builds the vertices (CONFIRMED, asm 80154690..80155108; C in `reference/near_pass_engine_math_near_pass.c`, tested on random inputs)

**Input.**
- The emitter defers a polygon to the near list when all of these hold:
  - its OTZ is below ctx+0x40 (70, or 20 in split screen; set at 801525D0/801525E4, and 70 at
    8015146C in `renderDrawModel`);
  - the mesh flag `+0x1A & 4` is set;
  - the list holds fewer than 200 entries (`slti 200` at 800CE6C8).
- Entries are 12 bytes: `{face record*, primitive*, t8 halfword, sort key halfword}`. The emitter
  writes them at 800CE6E8 / 800CE720.
- The first 71 entries live in the scratchpad at 0x1F8000A4 (`slti 71` at 800CE6D0). The rest go
  in `renderSharedWorkBuffer` (801F5A90). The near pass switches lists after entry 70
  (`li 0x46` at 80154DD4).
- A negative sort key means the mesh is mirrored: the pieces get the second draw mode (mode B).

**Per polygon** (the main loop at 801546E8):

1. The three new primitives are placed at the packet cursor (frame record +8). The pass stops when
   `end < q3 + 160` (80154744..8015474C). End is frame record +12, kept at scratchpad +0x3C.
2. **Screen XY** (the primitive word at +8, then one vertex stride further for each vertex; the
   stride comes from face word +12 bits 22..23):
   - x is the low halfword and y the high halfword. Each half is averaged as an *unsigned* 16-bit
     value: `srl 16` / `andi 0xFFFF`, add, `sra 1`.
   - Triangle: 80154764..801547CC. Quad: 801549B0..80154A80. The quad's five midpoints (edges 0-2,
     0-1, 1-3, 2-3, and the centre) are kept at scratchpad +0x78..+0x88.
3. **UV** (only if face word 0 & 0x7F, the texture flag): u and v bytes are averaged separately.
   - Triangle: 801547EC..8015488C. Quad: 80154AA0..80154BD0.
   - The CLUT halfword is copied from vertex 0 and the texture page from vertex 1, as the GPU
     packet layout requires. The texture page is also left in scratchpad +0x54.
4. **Colour** (gouraud only: face byte +1 ≥ 2, or face word +12 bits 19..21 ≥ 2): R, G and B
   bytes are averaged separately.
   - Triangle: 801548C4..80154998. Quad: 80154C04..80154DA4.
   - Retail quirk: the blue of the quad's 1-3 and 2-3 midpoints comes from vertices 1+2 and 3+1.
   - Flat faces copy colour word +4 into the three new primitives (80154DB0..80154DC0).
5. **Stores:**
   - `renderNearSplitTri` (80154E30..80154E80) and `renderNearSplitQuad` (80155088..80155108)
     write the fields with plain `sw` into the three new primitives. Then they overwrite vertices
     1..2 (triangle) or 1..3 (quad) of the original primitive.
   - The triangle pieces are `(m12, v1, m01)`, `(m12, m20, v2)`, `(m12, m20, m01)`, and the
     original becomes `(v0, m20, m01)`.
   - The quad pieces are `(m02, c, v2, m23)`, `(m01, v1, c, m13)`, `(c, m13, m23, v3)`, and the
     original becomes `(v0, m01, m02, c)`.
6. **Depth:** `renderNearLinkPrims` (80154E84..80155084) links all 4 pieces into `ot[otz]`, the
   parent's entry. A face with flag bit 7, or a mirrored one, gets a DR_MODE packet before and
   after. These packets are copied from scratchpad +0x60 / +0x6C. For flag bit 7, the texture
   window and texture page are patched from the face record.

**Notes for the port.**

| Topic | What retail does | Consequence |
|---|---|---|
| Rounding | Floor of the integer average; the midpoints are whole pixels | Half-pixel error at 1x. At high resolution the pieces snap to the 320x240 grid. |
| Negative coordinates | The unsigned halfword average is right only modulo 2048. Example: x = -1 and x = 1 give 0x8000, which is 0 after the GPU's 11-bit sign extension | A GPU path that reads x/y as full s16 gets ±32768 errors on near polygons that cross the left or top edge. The port's `gpu.c` / PSY-Z must sign-extend bits 0..10, or a C replacement must average signed values. |
| GTE saturation | Vertices with SX/SY clamped to ±1024 average to wrong points; vertices behind the camera are not fixed either | The subdivision does not help clipping. |
| PGXP | Midpoints are built from words read back from the packet buffer (`lw`), then written with `sw` | A shadow-vertex scheme keyed on guest addresses must propagate through this pass. It can average the two parent shadows (better: interpolate x/w, y/w, 1/w), or replace the pass with C (the reference C is a drop-in). |
| Skipping it | The emitter does not link deferred primitives | A replacement must still call the linking, with mode wrappers, for the single original primitive. |

### R1.3 The model emitter `sub_800CDC54`: every screen-position write (CONFIRMED)

Primitive kind is face word +12 bits 16..18. The packet cursor `cur` is in a register. The end
check reads scratchpad +0x3C at 800CDF0C. The cursor is written back to frame record +8 at the end.

| Kind | GTE | Screen XY write | Other vertex words |
|---|---|---|---|
| 3 triangle | RTPT 800CDF90, NCLIP 800CDF9C (sign vs ctx+0x88) | `swc2 $12/$13/$14` at **800CDFC8, 800CDFD0, 800CDFD8** | SZ1..3 read by `mfc2` 800CDFDC..E4, only for the OTZ (max SZ >> shift); not kept |
| 4 quad | RTPT 800CE0EC on v0..v2, NCLIP 800CE0F8, RTPS 800CE144 on v3 | `swc2 $12/$13/$14` at **800CE124, 800CE12C, 800CE134**; v3 `swc2 $14` at **800CE14C** | SZ0..3 `mfc2` 800CE150..5C, only for the OTZ |
| 1 beam (camera-facing strip) | RTPT 800CE258 on the point at z ± length | `mfc2 $12/$13` at 800CE260/64 into registers. Half width = `+6 · H / SZ1` (H read back with `cfc2 $26` at 800CE218), clamped 1..2·w. The 4 corners are stored with **`sh` at 800CE3DC..800CE420** (x ± s, y ∓ c, rotated by `ratan2` of the strip) | – |
| other: point sprite | RTPS 800CE454 | `swc2 $14` into **scratchpad +0x54** at 800CE464. Half sizes `+6 · H / SZ` and `+8 · H / SZ` (`cfc2 $26` at 800CE43C), clamped 1..500 (800CE538/800CE550). The 4 corners are stored with **`sh` at 800CE570..800CE5B0** | – |
| all | – | – | Colours: NCCS result `swc2 $22` at 800CE648, or plain word copies. UVs: word copies. Near-list entry: 800CE6E8, 800CE720 |

`renderScene` (80152284, at 8015260C) and `renderDrawModel` (80151378, at 8015147C; menus, car
select, endings) call the emitter, then the near pass (80152628 / 80151494).

### R1.4 Every other code that writes screen positions into GPU primitives

**A. 3D-projected, in battle.** These are the sites a high-resolution or subpixel path must handle.
All are per view and per frame, called from `rtMainLoop` (see `MAIN_LOOP.md`).

| Function | VA | File | Projection | XY stores | Primitive | OT index |
|---|---|---|---|---|---|---|
| `fxDebrisDraw` | 8014C19C (called by `debrisDraw` 8014B484 at 8014B4C8) | `engine_fileio_debris.c` | GTE through libgte. Triangles: `RotTransPers3` 801404F0 (call 8014C6BC; its `swc2 $12..$14` at 80140520..28 write straight into the packet). Quads: `RotTransPers4` 8014D680 (call 8014CAA0; `swc2` 8014D6AC..B4, 8014D6D8). Sprites: `RotTransPers` 801404C4 (call 8014CCE0; `swc2 $14` 801404D4) | Sprite path *edits the XY after the GTE wrote it*: `sw` SXY at 8014CE24, then `lhu`/`sub`/`sh` at 8014CE34, 8014CE44 and the corners at 8014CE60..8014CEB8. Half size = `+6/+8 · viewGetProjH(0) / SZ` (8014CD38/8014CD90, **view 0's H, even in view 1**), clamped 1..500 | debris mesh triangles/quads, sprite quad | `renderCompressDepth` 8014D368 of max SZ >> shift; drop unless `otz-1 < 4095` (8014CD24, 8014CEC8) |
| `streaksDraw` | 800D8230 | `screen_card_draw.c` | Software: `mathMulMatVec` (camera matrix `viewGetViewMatrix`, eye `viewGetEyeTrans`), `x = a·H/z + cx` (divides 800D84D8..800D85B0) | LINE_G2: `sh` 800D8670..800D867C. Wide streak (POLY_G3): `sh` 800D86BC..800D86FC; the width is `(y0-y1)/12+1`, `(x0-x1)/12` in screen pixels | LINE_G2 (bullet tracer), POLY_G3 | `za >> 3` |
| `weaponArcsDraw` | 801084D8 (called from `streaksDraw`) | `weapon_arcs_draw.c` | Software, as above (divides 801089B8, 80108A04) | LINE_F2: `sh` 80108C60..80108C84 | LINE_F2 (lightning segments) | `ot[0]` (on top) |
| `fxSpritesDraw` | 80110BE4 | `world_collapse.c` | Software (divides 80110DC4..80110FE4, 8011113C/84 for the type-1 tail point) | POLY_FT4: type 1 (rotated) `sh` 8011121C..80111254; other types (square) `sh` 80111260..80111290. Half size = `w · scale · H / z >> 3`, clamped 1..512, the same in x and y | 150 effect sprites | `z >> 3` (depth codes 1 and 8 adjust it) |
| `fxImpactParticlesDraw` | 80112F6C (called from `fxSpritesDraw`) | `car_damage_human.c` | Software (divides 80113098..8011311C) | POLY_FT4: `sh` 801132DC..801132FC. Half size = `H·10/z` | 20 impact particles | `z >> 3` |

**B. The emitter itself** (R1.3) and the near pass (R1.2).

**C. Fixed 2D screen positions** (they ignore the projection; the list and coordinates are in
`WIDESCREEN.md` §5). Primitive XY writes happen in:

- 1P HUD `hudDrawSingleView` 800F8370;
- split HUDs `hudDrawSplitVertical` 800F9084 and `hudDrawSplitHorizontal` 800F9D48;
- `hudInitPanels` 800FB0B0;
- the mirror frame `uadashInitRearViewMirror` 800FB8AC / `UAdashDrawRearViewMirror` 800FBB0C;
- the radar `hudRadarInit` 800FC5C4, `hudRadarPlotCar` 800FC928, `hudDisplayRadar` 800FBE68 (map
  coordinates, not projected);
- `hudDrawTargetPanel` 800FD0D0, `hudDrawNumber` 8014E194, `fontSpritePrint` 80113C00;
- the dashboard icons `UAdashInitDashboardIcons` 800FAA0C, with 800FADDC / 800FAEEC still
  unchecked (R3).

The HUD has its own 2-entry OTs (`hudOrderTables`) and `DrawOTag` calls. Full-screen quads:

| Function | VA | XY stores |
|---|---|---|
| `fxSheetStart` (flash) | 800DC45C | two POLY_F4 at `fxSheetPolys` 80181DF8: `sh` 800DC4D8..800DC550 (0/320, 0/240). Linked each frame by `fxSheetDisplay` 800DC590 from `streaksDraw` |
| `explodeMakeScreenRed` | 8011335C | POLY_F4, `sh` 801133D8..801133F4, linked inline into the frame OT |
| `screenLostALife` | 8012940C | stack POLY_F4 `sh` 80129490..801294B0, drawn with `DrawPrim` under `SetDefDrawEnv(0,0,320,240)` |

Shell-only 2D (menus, cards, `screenDrawCard` 8012A0E0, passwords, sliders) is outside the match
and not listed.

**D. Nothing else (CONFIRMED by scan).** In the whole EXE:

- The only `swc2 $12/$13/$14` and `mfc2 *, $12..$15` are in the emitter and in the libgte
  functions `RotTransPers` 801404C4, `RotTransPers3` 801404F0, `RotTransPers4` 8014D680 and
  `ReadSXSYfifo` 8014D62C. The only caller of the three projection functions is `fxDebrisDraw`.
  `ReadSXSYfifo` and `SetSXSYfifo` have no callers.
- The view-mode table `8017EC44` is read only by the view accessors. The projection helpers
  `viewGetCentre` 80156614, `viewGetProjScaleX` 801555FC, `viewGetProjScaleY` 80155634 and
  `viewGetProjH` 801555C4 are called only by the five drawers in A. `viewGetViewMatrix` 801566B0
  is also called by `viewLoadGte` 8014965C and the sound listener 8015DBE8 (no primitives).
- The only functions with inline OT linking (the 0x00FFFFFF mask) outside libgpu are
  `hudDisplayRadar`, `explodeMakeScreenRed` and `eiffelTowerStartFall` (a mask, not an OT). The
  emitter and the near pass link with byte stores.
- Lens flares: none (as `WIDESCREEN.md` §5 says).

### R1.5 Notes for widescreen and high resolution

- **Positions follow the camera.** Every 3D path takes its x through the camera matrix, which
  includes the axis-X scale of the view-mode row (`viewProc` builds `viewGetViewMatrix` from
  it). An anamorphic squeeze of that scale therefore moves all projected positions correctly.
- **Sizes do not follow the camera (CONFIRMED).** Sprite and particle sizes are computed in
  screen pixels from H and z only, the same in x and y. This applies to:
  - emitter point sprites and beams;
  - `fxSpritesDraw`, `fxImpactParticlesDraw`, the debris sprites;
  - the `streaksDraw` triangle width.

  Under the anamorphic plan (RENDER_PLAN §3) these come out wider by the stretch factor (×1.33 at
  16:9). The fix is to scale the x half-size by the same factor at each listed `sh` site, or in C
  overrides of the four C drawers.
- **Subpixel.** The emitter and debris paths get integer SX/SY from the GTE (a PGXP-style GTE
  shadow covers them). The four software drawers divide in C with integer results, so a port can
  replace them (they are C in `src/semantic`) and keep fractions. The near pass is covered in R1.2.
- **Depth for perspective-correct UVs.** No path stores per-vertex depth in the primitive. The
  emitter and debris read SZ0..3 only for the OTZ. The port must capture SZ (GTE registers 16..19)
  next to each SXY store listed above.

## R2. Buffers, caps, LOD data and the visible flag

### R2.1 The per-frame buffers: where each address and size is formed (CONFIRMED; all in BSS, 80181454..801FAD48, no file bytes)

| Buffer | Address, size | Every instruction that forms the address (lui / addiu pair) | Size and cap checks |
|---|---|---|---|
| Mesh draw list `renderMeshList` | 801AC810, 280 × 32 = 0x2300 | `renderInitState` 80152230/80152234 (→ ctx+0x20). `renderScene` 801525F4/801525F8 (emitter a1). `renderDrawModel` 80151450/80151454 (emitter a1). `renderDrawObjectUnculled` 80151514/80151518 (`lightAddSources` 8014D3F4 a0) | `slti s4,s3,280` at **801528EC**. The count is ctx+0x3C (read 801528E4, stored 8015291C), and it becomes the emitter's count (renderScene passes ctx+0x3C). The walker cursor is ctx+0x20, stored back at the walker's end |
| Matrix slots `renderMatrixSlots` (rotation, emitter a3) | 801AEB18, 200 × 32 | `renderInitState` 80152210/80152214 (→ ctx+0x18). `renderScene` 801525FC/80152600. `renderDrawModel` 8015145C/80151460. `renderDrawObjectUnculled` 8015151C/80151520. `renderInit` 80151580/80151584 (`dbscanInitStacks`: the load-time matrix stack). `renderGetMatrixSlots` 80151618/8015161C (no direct callers) | see the slot counter below |
| Object / light matrices `renderObjectMatrices` (emitter 5th argument) | 801F8AB8, 200 × 32 | `renderInitState` 80152224/80152228 (→ ctx+0x1C). `renderScene` 8015230C/80152310, 80152388/8015238C, 80152410/80152414, 8015249C/801524A0, 80152604/80152608. `renderDrawModel` 801513D4/801513D8. `renderDrawObjectUnculled` 801514EC/801514F0 | – |
| Frustum matrices A (side and near planes per slot) | 801B2E0C, 200 × 32 | `renderInitState` 80152250/80152254 (→ ctx+0x44) | – |
| Frustum matrices B (top, bottom, far) | 801B470C, 200 × 32 | `renderInitState` 80152270/80152274 (→ ctx+0x48) | – |
| Slot matrix pointers `renderSlotMatrixPtrs` | 801ABA50, 16 bytes per slot (0xDC0 reserved) | `renderInitState` 801521F4/801521F8 (→ ctx+0x04) | – |
| Walker save area `renderWalkerSaveRegs` | 801A4DEC, 40 bytes | `renderInitState` (→ ctx+0x0C) | – |
| Shared work buffer `renderSharedWorkBuffer` | 801F5A90, 0x3028 up to 801F8AB8 | `renderInitState` 801521D4/801521D8 (→ ctx+0x00). `renderScene` 80152584/80152588 (→ ctx+0x4C) and 80152620/80152624 (near pass a1). `renderDrawModel` 80151464/80151468. `renderInit` 80151564/80151568 (`hdpInit`, `hdpSetStackBuffer`: **collision uses it every frame**). `renderGetSharedBuffer` 80151608/8015160C (`bitmapSetupStorage` at load) | Shared by four users at different times: the walker's node stack (entries 32..199 × 28 bytes; `renderPushMatrix` caps at 32/200, 80152664/8015268C, pop at 801527AC); the near list (entries 71..199 × 12); the hdp stack; the load-time relocation stack |
| Packet buffers `renderPacketBuffer0/1` | 801B600C and 801CDD4C, **0x17CF0 each** | `rtDoubleBufferInit` 800D1A94/800D1A98 and 800D1A9C/800D1AA0; `renderGetPacketBuffer` 800D1A64/800D1A68 (callers: `bitmapSetupStorage` 80121518 and `shellInitMachine` 80128BA0, which borrow it as load-time storage). **Size 0x17CF0**: `lui a0,1` / `ori a0,0x7CF0` at **800D1A84/800D1A88**, added once per frame record (800D1AC4, 800D1AC8) | End = record +12 (start + 0x17CF0). Cursor = record +8, reset at the frame start (`rtMainLoop`, `buffer->field_8 = field_4`). View 1 and the mirror continue from view 0's cursor, so **one 0x17CF0 buffer holds all the 3D of a frame, for both views** |
| Ordering tables `renderOtView1Buf0/1`, `renderOtView2Buf0/1` | 801E5A8C, 801E9A8C, 801EDA8C, 801F1A8C; 4096 entries × 4 = 0x4000 each | `rtDoubleBufferInit` 800D1AA4/800D1AA8, 800D1ABC/800D1AC0, 800D1AE0/800D1AE4, 800D1AEC/800D1AF0; `renderGetOrderingTable` 800D1A74/800D1A78 (caller `bitmapSetupStorage` 80121534, `shellInitMachine` 80128BA8) | `ClearOTagR(ot, 0x1000)` at 800D1400, 800D1554, 800D15D4; `DrawOTag(ot + 16380)` at 800D16A8, 800D1748, 800D1798, 800D17DC; emitter drop `slti 4095` at 800CE314, 800CE4EC, 800CE5BC; debris `sltiu 4095` at 8014CD24, 8014CEC8; arcs `slti 4091` at 80108BC8/80108BD0; sprites, particles and tracers stay below 32001 >> 3 = 4000 (800D8400, 800D843C, 80110D78, 80113068) |

There are no data words pointing at any of these buffers (`config/US/data_relocs.tsv` lists none).
Only the instructions above form their addresses.

**The frame records** are 0x80 bytes each:

| Records | Address | Fields |
|---|---|---|
| `renderView0Frames` / `renderView0FrameB` | 8018118C / 8018120C | +0 OT, +4 packet start, +8 packet cursor, +0xC packet end, +0x10 DRAWENV, +0x6C DISPENV |
| `renderView1Frames` / `renderView1FrameB` | 8018128C / 8018130C | same layout |
| `renderMirrorFrames` / `renderMirrorFrameB` | 8018138C / 8018140C | same layout |

`rtDoubleBufferInit` (800D1A84..800D1B6C) is the only code that writes +0, +4 and +0xC.

**The matrix-slot counter** is ctx+0x08. Every per-slot array is indexed by it: `S->m18`, `S->m1C`,
`S->m44`, `S->m48` (×32) and `S->m04` (×16).

| Where | Check |
|---|---|
| walker opcode 5 (transform node) | `slti s2,s7,198` at **80152C18**, then `count++` (80152C24/80152C28) |
| `renderScene`, the 20 world models | `sltiu v0,v0,200` at **80152354** |
| `renderScene`, the camera-follow (sky) model | **801523E8** |
| `renderScene`, each listed object | **801524BC** |

- Each renderScene check is `if (count < 200) count++`, so the index can reach **200**, one past
  the 200-entry arrays. The walker then copies 32 bytes into `renderMatrixSlots[200]` (=
  `worldSurfaces` 801B0418), and `csUpdMat` writes into `renderObjectMatrices[200]` (= 801FA3B8,
  `timerTicsAccum`).
- **LIKELY a latent retail overflow.** It needs 199 or more slots in one view: transform nodes
  plus objects. A port that enlarges the arrays should make all of them ≥ cap + 1 and raise the
  four checks together.

**Other per-view caps:**

| Cap | Where |
|---|---|
| Near list 200 entries (71 in the scratchpad) | 800CE6C8 / 800CE6D0, near pass 80154DD4 |
| 20 world-model slots | `renderWorldModels` 801A4E1C; `slti 20` at 801523B0, 801515EC |
| 128 scene objects | `csAlloc` |
| Walker node stack 200 | 8015268C |

**The scratchpad is full.** The walker's first 32 stack entries live at 0x1F80007C..0x1F8003FC.
The emitter reuses +0x38/+0x3C as near count and packet end, and +0xA0/+0xA4 as the near list. The
near pass keeps its quad midpoints at +0x78..+0x88, which overwrites ctx+0x88, the cull flip that
the callers set again before each emitter call. Larger caps should therefore grow the RAM arrays,
not the scratchpad parts.

**To move a buffer above 2 MB:**

1. Patch every lui/addiu pair in the table, or override the few C functions that form the
   addresses. `renderInitState`, `renderScene`, `renderDrawModel`, `renderDrawObjectUnculled`,
   `renderInit`, the getters and `rtDoubleBufferInit` are all byte-identical C in `src/semantic`.
2. Raise the immediates 280 (801528EC), 198 (80152C18), 200 (80152354, 801523E8, 801524BC) and
   0x17CF0 (800D1A84/88).
3. The asm walker and emitter only use the ctx pointers and the counts. They need no address
   patch, only the two cap immediates above.

### R2.2 Far distance and LOD ranges per arena

**Far distance (CONFIRMED).** There is no per-arena far value.

- `renderFarDistanceDefault` (`D_80180DAC`, gp+984) is 4800 in the file.
- Its only writer is `renderSetFarDistance` 8015165C, which has no callers.
- Readers copy it to `renderFarDistance` (gp+980) at 801522D0/D8, 801523BC/C4, 80152454/5C,
  80151378/94 and 801514B8/D8. `renderInitState` copies it into ctx+0x10 at 801521E4.
- The mirror uses immediates (500 at 801522CC, 1000 at 80152450).
- Per arena, only the **world shift** varies: DMD header +0x10 → `renderSetWorldShift` 8015166C
  (from `worldLoadStartObjects` 800D1C70 at 0x80019C50, and from `assetInstallImage` 80120EF4).
  **It is 3 in every DMD on the disc.** Opcode-12 nodes change the shift inside subtrees: FREEWAY
  uses 1 and 2; PARIS, DEN, SROOF use 0; ISLANDS, ROOF, GLACIER, HKONG use 1; SWAMP uses 3 and 4.

**LOD band format (CONFIRMED; walker opcode 2 at 80152A14; `dbscanHierarchy` 8014ABF8).**

- The node holds a position at +4..+12, a band count at +16, and band pointers from +20.
- Each band is `{max², min², child}` (u32, u32, pointer).
- The squared distance is `SQR((pos + t) >> shift)`, in world units², from the camera to the LOD
  point. It uses the GTE with 16-bit IR inputs, so each axis wraps beyond 32767 world units.
- The first band with `min² ≤ d² < max²` is drawn. If none matches, nothing is drawn.
- **Threshold values above about 16600 are overflowed 32-bit squares (CONFIRMED arithmetic).** The
  authors wrote "infinite" ranges; squared, they wrapped:

  | Stored max | Wrapped from |
  |---|---|
  | 16627 (0x107A4000) | 10^7 |
  | 37551 (0x540BE400) | 10^5 |
  | 38924 | 120000 |
  | 43300 | 10^8 |
  | 59729 | 10^6 |
  | 61091 | 1.5·10^6 |

  All of them are far beyond the GTE's 8191-unit limit at shift 3, so they act as unbounded.

**Per arena** (the level model, node id 1). "Outer LOD" = an opcode-2 node with no LOD above it.
"Top" = its largest band max, in world units.

| Arena (file) | Outer LODs | Top < 4800 (meant to vanish) | Top 4800..8000 | Top 10000 | Top ≥ 16627 (unbounded) | Largest finite top that matters | Opcode-11 zone nodes |
|---|---|---|---|---|---|---|---|
| Los Angeles (FREEWAY) | 162 | 153 | 0 | 2 | 7 | 10000 | 22 (1 hidden by flag) |
| Moscow (DISH) | 74 | 8 | **16 at 5000** | 50 | 0 | **5000** | 0 |
| Paris (PARIS / PARIS2) | 182 / 182 | 108 | 0 | 2 | 72 | 10000 | 19 (4 hidden) |
| Amazonia (ISLANDS) | 197 | 105 | 0 | 91 | 1 | 10000 | 1 (4 zones) |
| New York (ROOF / ROOF2) | 252 / 248 | 150 / 163 | 0 | 102 / 85 | 0 | 10000 | 49 / 45 |
| Antarctica (GLACIER / GLACIER2) | 126 / 108 | 93 / 84 | 0 | 1 | 32 / 23 | 10000 | 12 / 9 (all hidden by flag) |
| Holland (DEN) | 95 | 14 | **1 at 5000** | 80 | 0 | **5000** | 0 |
| Hong Kong (HKONG) | 115 | 82 | 0 | 4 | 29 | 10000 | 12 |
| Suicide Swamp (SWAMP) | 125 | 125 (largest 2000) | 0 | 0 | 0 | 2000 | 6 |
| Cyburbia (BURB) | 12 | 12 (largest 1800) | 0 | 0 | 0 | 1800 | 1 |
| Roof Tops (SROOF) | 51 | 51 (largest 3500) | 0 | 0 | 0 | 3500 | 17 |

How to read the table:

- **Moscow and Holland are the only arenas where raising the far distance above 5000 shows LOD
  pop-out** (16 + 1 pieces end at 5000).
- Everything else is either meant to vanish well inside 4800 (small details), or reaches 10000 or
  more, which is beyond the 8191 GTE limit at shift 3.
- Swamp, Cyburbia and Roof Tops have few or short LOD nodes. Most of their level is plain
  group/mesh nodes, so the far test alone limits them.
- Opcode-11 zone nodes swap a subtree (child +12 when the camera is inside a zone, +8 otherwise).
  No zone has a null child. Flag `node+4 & 2` (hidden) is set in the file data for all 12/9
  GLACIER zones, 4 PARIS zones and 1 FREEWAY zone. Whether game code clears it at run time was not
  checked.

**Car models** (AX, BB, … 15 files) have LOD tops of 150..3000. Cars are objects, which are never
far-culled. **LIKELY** the LOD-switched parts of a car seen beyond its top band (1000 for most
cars, 2000..3000 for some) draw nothing. This is a data limit that a longer draw distance will expose.

The script used (a DMD walker following `dbscanHierarchy`) is not part of the repository. The
values come from `DMD\*.DMD` on the USA disc, image origin 0x80019C40.

### R2.3 Who reads an object's visible flag (CONFIRMED)

Two different flags are both called "visible". They must not be confused.

| Flag | Struct and offset | Writers | Readers | Meaning |
|---|---|---|---|---|
| **On-screen flag** | `TM2Obj.visible`, **+0x124 (+292)**, the 0x128-byte scene object (`csObjectPool`, `engine_clut_anim.c`) | `csUpdMat` 801491E8 only: `sw` at 80149268 (the `csSphereVisible` 80149344 result) and 80149274 (forced 1) | `csUpdMat` itself (80149278: only then does it update `wpos` +0x14, `m20`, and m40/m60 when forced) and `renderScene` 801524F0 (walk the model or not). **Nothing else** | This frame's 5-plane sphere test against the view being drawn. Rewritten for each view |
| **Active flag** | `TM2Object.visible` / `TM2SceneObject.visible` (`cars.h`, `weapons.h`), **+0x008** of the same object | game code (weapons, pools, debris, warps, restarts…) | `renderScene` (`lw 8(s0)` gate before `csUpdMat`); the hit pass in `engine_hit_detection.c` (`(+8 & +0xC) != 0`); weapon code (e.g. `weapon_ricochet.c`) | Gameplay "exists / drawn" switch. It does not depend on the view |

- `csUpdMat` callers: `renderScene` (8015241C for the follow model, forced; 801524E8 for list
  objects, with `renderNodeCullEnable` = 0, so it really tests), `renderDrawModel` (801513E4,
  shell) and `renderDrawObjectUnculled` (801514F4, debris burst setup).
- **Scan:** a scan of every `lw/sw/lh/lb …,292(reg)` in the EXE finds no other read of +0x124.
  The remaining hits are stack frames, a car-record vector in `carUpdateContacts` 800EF85C, and a
  menu record.
- **Collision** (`hdp`) reads +8, +0xC, +0x80 and +0xA8, never +0x124, `wpos` or `m20`.
- **AI targeting** uses its own 45° cone around the car (`WIDESCREEN.md` §4 row 11).
- **Sound** uses the view matrix, not the flag.
- **So the +292 flag is render-only.** Changing culling, the far distance or the side planes
  cannot change gameplay or the simulation (good for netcode determinism).
- Side effect inside rendering: while an object is off screen, its camera-relative `wpos` and
  `m20` are stale. Only the renderer reads them.

### R2.4 Measuring the render caps (`portRenderStats`, `TM2_PORT_FAR_DISTANCE`)

A read-only counter so the port can see how close each view gets to the per-view limits before deciding to raise
the matrix-slot limit (R2.1: 200 per view, 198 for transform nodes at 80152C18), the mesh queue (280 or
`portMeshListCap`, `FAR_DISTANCE.md` 3) or the packet buffer (97,520 bytes or `portPacketBufferBytes`).

**Build:** `--port-define TM2_PORT_FAR_DISTANCE` (the define that owns the mesh and packet caps; with its settings
at 0 the caps are retail's, so it measures retail's limits too). The storage is in every variant; without the
define nothing writes it (it stays zero).

**How it is written** (`renderScene`, `engine_render_scene.c`; port code in `port_variant_arrays.c`, header
`include/semantic/port_render_stats.h`):
- `portRenderStatsWalked(db, view)` after every walk (the level, the world models, the sky, the objects) and
  before the emitter: the slot counter ctx+0x08 and the mesh count `renderCurrentOt` (the walker's counter ctx+0x3C
  minus the port's bias). It must run there: the emitter saves a register over ctx+0x08 (`sw $18,8($8)`
  800CDC60) and reuses ctx+0x3C as the packet end. The walker (hand-written asm) is not touched: the counter reads
  what it leaves in the scratchpad.
- `portRenderStatsDrawn(db)` after the emitter and the near pass, before `animUpdateAll`: the frame record's
  cursor (+8), start (+4) and end (+0x0C).
- Row: view mode 4 (the rear-view mirror) is row 4; otherwise `renderScene`'s view argument 0..3 is its row; any
  other view is not recorded. Retail calls `renderScene(.., 1)` for both the split screen's second view and the
  mirror; the mode tells them apart.
- Observation only: it reads the renderer's counters and the frame record and writes only `portRenderStats`
  (`tools/check_custom.py` check 5), calls no `rand()`, changes nothing drawn. **Not game state:** leave it out of
  the rollback snapshot and the state hash (`ONLINE_ROLLBACK.md` 3.3).

**Layout** (`struct TM2PortRenderStats`, 0x150 bytes, `.port_bss` after `portVisualState`; all fields `int`):

| offset | field | meaning |
|---|---|---|
| +0x00 | `reset` | the port writes non-zero: the next recorded view zeroes every row, then this word |
| +0x04 | `row` | internal: the row being drawn (-1 none) |
| +0x10 + 0x40 × row | `view[5]` | rows 0..3 = views 0..3, row 4 = the mirror |

Each row (`struct TM2PortRenderView`, 0x40 bytes):

| offset | field | meaning |
|---|---|---|
| +0x00 | `frames` | views recorded in this row since the reset |
| +0x04 / +0x08 | `slots` / `slots_max` | matrix slots this frame (ctx+0x08 after the walks, 0..200) / high-water mark |
| +0x0C / +0x10 | `meshes` / `meshes_max` | meshes queued this frame / high-water mark |
| +0x14 | `mesh_cap` | the queue's cap this frame: 280, or `portMeshListCap` clamped to 1..2,048 |
| +0x18 / +0x1C | `packet` / `packet_max` | bytes of the frame's packet buffer used after this view (cursor - start). The views of a frame share one buffer, so this counts every view drawn so far this frame and their effects (streaks, sprites, debris); the last view's value is the frame's total / high-water mark |
| +0x20 | `packet_cap` | the buffer's size (end - start): 97,520, or `portPacketBufferBytes` |
| +0x24 / +0x28 | `packet_scene` / `packet_scene_max` | bytes this view's emitter and near pass added (its level and objects) / high-water mark |
| +0x2C | `caps` | caps reached this frame (bits below) |
| +0x30 | `caps_seen` | every cap bit reached since the reset |
| +0x34 | `capped` | frames (views of this row) with any cap reached |
| +0x38 | `first_capped` | `frames` of the first such view (1-based), 0 = none yet |
| +0x3C | `walk_cursor` | internal: the packet cursor after the walks |

Cap bits: 0x1 `slots >= 198` (a transform node, walker opcode 5, would get no slot of its own: its children
misplaced); 0x2 `slots == 200` (later objects share slot 200, one past the arrays: R2.1's latent overflow);
0x4 `meshes == mesh_cap` (the queue is full: any later mesh of the view was dropped, or the queue was exactly
full); 0x8 `packet > packet_cap - 80` (the emitter stopped early, or its next primitive would have).

**Addresses** (from `out/TM2_SOURCE_PORT/symbols.tsv`; they move with storage added before them and with
`--port-limits`, so read the map of the build the port runs):

| build | `portRenderStats` | row 0 (view 0) | row 4 (mirror) |
|---|---|---|---|
| default variant (`out/TM2_SOURCE_PORT`, no define; nothing writes it) and `TM2_PORT_FAR_DISTANCE` alone | 0x80551D88 | 0x80551D98 | 0x80551E98 |
| the car stack + `TM2_PORT_FAR_LOD TM2_PORT_FAR_DISTANCE TM2_PORT_SPU48 TM2_PORT_HAZARDS TM2_PORT_HUMANS=16 TM2_PORT_BALANCE TM2_PORT_VISUAL TM2_PORT_CAR_NUMBERS` | 0x8059C2AC | 0x8059C2BC | 0x8059C3BC |

Row r is at `portRenderStats + 0x10 + 0x40 * r`.

**The worst-case test.** Build with the full set (`TM2_PORT_FAR_LOD TM2_PORT_FAR_DISTANCE`, the car stack,
`TM2_PORT_HUMANS=16`), then:
1. 16 cars, draw distance Farthest (`portFarDistance` 32,000, `portFarLod` 1), `portFarLodNearScale` 4, the caps at
   the values the port ships (`portMeshListCap`, `portPacketBufferBytes`), and once more with them at 0 (retail's
   280 / 97,520) to see what retail's caps would drop.
2. Hong Kong (HKONG) and Antarctica (GLACIER), at the far corners looking across the arena; Antarctica with Axel
   and with several Minions (the most transform nodes: MN 7 per car).
3. 1 player with the rear-view mirror on (row 4), 2-player split screen, 4 views.
4. Write `reset` = 1 at the start of each run, read the rows at the end: `slots_max` against 198, `meshes_max`
   against `mesh_cap`, the last view's `packet_max` against `packet_cap`, `caps_seen` / `capped` / `first_capped`
   for any frame that reached a cap.
Raise the matrix-slot limit only if `slots_max` comes near 198 (section 3.2 of `PLAYTEST_ANSWERS.md` estimates
about 110 before the own car, about 194 with 15 Minions).

## R3. Interpolation (60 fps): effect records, direct matrix writers, snaps, and state changed while drawing

This answers R3 of request 16 (`docs/RENDER_PLAN.md` section 7 in the port). It covers four things: the drawn effect records, objects whose rotation matrix is not rebuilt from their angles, every discontinuous move (where the blend must snap), and every state change made in the draw path. The answer comes from static reading of `src/semantic` and the Hex-Rays export `reference/ida/SCUS_94306R.EXE.c`; nothing was run.

CONFIRMED means the code was read. LIKELY means it was inferred and not traced to the end. The VAs are retail; names are the current ones in `config/US/names.tsv`. Object offsets use the standard object (`TM2Obj`, `csObjectPool`, 0x128 bytes): +0x08 visible, +0x10 parent, +0x80 (128) matrix, +0xA0 (160) `s16 rot[3]`, +0xA8 (168) `s32 pos[3]`.

### Summary

1. **Most effects are already standard objects.** Missiles and special projectiles, their trail strips, debris chunks, mines and remote bombs, ricochet shots, shields, the tornado, Shadow strike and wake objects are all csList objects drawn by `renderScene`. Blending their +0xA0/+0xA8 covers them. Only five record systems are drawn by hand: effect sprites, impact particles, bullet streaks, debris bursts and taser/lightning arcs. Section (a) gives their layouts.
2. **Only the machine-gun bullets (and ricochet shots and Shadow strike) keep a previous position.** Every other system needs a port-side `prev` array, filled before the step's update, and reset on spawn.
3. **Car objects do not use +0xA0 for their matrix.** `humanCarUpdate` / `aiCarUpdate` copy the car's `body_m` (heading plus suspension pitch/roll, lean and shake) into +0x80. Rebuilding +0x80 from +0xA0 loses the body tilt. Several other drawn transforms are type-5 model nodes written directly (car body nodes, rotor/part spinners, Mr. Slam's arm, the Eiffel tower, splash nodes, collapsing ice). See (b).
4. **There is no kill cam, intro fly-in or winner camera in the 3D loop.** The cuts are: round start (`rtMainLoop` entry), car placement (`humanCarPlaceAtStart`, `aiPlaceOnRouteNode`), teleporters (`worldWarpsUpdate`), view mode changes (`carSetViewMode`), the fixed camera's yaw jump, attract-demo respawn and watch switches, and slot (re)allocation in every pool. See (c).
5. **`streaksDraw` (800D8230) is mostly update work.** Its callees tick mine fuses, the flamethrower, taser damage and the screen flash, and call the shared `rand()`, including one GAME_STATE site. `renderFlipFrame`, `clutAnimUpdate`, the HUD radar swap, the HUD message timer and the weapon-marker blink are also once-per-step. `pauseMenu` is a blocking loop. The arena events draw nothing. See (d).
6. **New finding about retail's order.** In split screen and with the rear-view mirror, view 1 is drawn *after* `worldUpdate` (rt_main.c), so view 1 shows a state one world step newer than view 0. In the same modes the `streaksDraw` ticks already run twice per step in retail. (CONFIRMED, `rt_main.c`.)

### (a) Effect records

#### Conventions (CONFIRMED)

- World positions are `s32 {x, y, z}`, with z up. Gravity always acts on `vel[2]`.
- **Hand-made draws project the same way.** Sprites, impact particles, streaks and arcs all compute `v = pos + viewGetEyeTrans(view)`, then `mathMulMatVec(viewGetViewMatrix(view), v)`, then divide by z.
  - Those are the per-view camera copies `viewEyePositions` (D_801A51A8) and `viewMatrices` (D_801A51C0) that `viewProc` writes.
  - So an interpolated camera placed before the extra `viewProc(i)` calls reaches these draws automatically.
  - `fxDebrisDraw` uses the camera record `csGetRoot()` instead, which `viewProc` also writes.
- **Order within a step.** `fxSpritesUpdate` (#11) runs right before `fxSpritesDraw` (#12). `bulletsUpdate` (#9) runs after `streaksDraw` (#8). Anything spawned in `worldUpdate` (#16) is first drawn by view 0 in the next iteration.

#### Summary table

| System | Pool | Count x size | Position | Prev pos? | Update (once per step unless noted) | Draw | Blend |
|---|---|---|---|---|---|---|---|
| Effect sprites (explosions, smoke, sparks, flames) | `fxSprites` 80185144 | 150 x 48 | +20 `s32[3]`; follow sprites copy `*src` (+44) | no | `fxSpritesUpdate` 80110A40 (#11) | `fxSpritesDraw` 80110BE4, per view | port prev; follow sprites from the interpolated car |
| Impact particles | `fxImpactParticles` 80184E74 | 20 x 36 | +0 `s32[3]` | no | `fxImpactParticlesUpdate` 80112E58 (inside #11) | `fxImpactParticlesDraw` 80112F6C (from `fxSpritesDraw`) | port prev, lerp |
| Bullets = the "40 streak slots" | `bulletRecords` 80181498 | 40 x 60 | +0x00 | **yes, +0x0C** | `bulletsMove` 800D7E30 via `bulletsUpdate` 800D87B4 (#9) | tracer loop of `streaksDraw` 800D8230, per view | head = lerp(prev, pos), tail = head - (pos - prev) |
| Missiles and special projectiles | `fxRecords` 801A5B08 | 16 x 980 | missile object (+0x34) +0xA8 | no | `fxRecordsUpdate` 800DC348 → `weaponProjectileUpdate` 800DB110 (#4) | `renderScene` (object) | standard object blend |
| Missile trails | inside `fxRecords` (+0x48, +0xF0, +0x14C) | 16 x (21 verts, 18 segs) | trail object +0xA8 (static); `s16` verts relative to origin +0x28 | no | `fxTrailUpdate` 800D9238 | `renderScene` (strip model) | optional: rewrite the head slot from the interpolated missile |
| Debris chunks | `debrisChunks` 801A3D50 | 8 x 36 | object (+0x14) +0xA8 | no | `debrisUpdateChunks` 8014B940 (#5) | `renderScene` | standard object blend |
| Debris bursts | `debrisBursts` 801A3E70 | 4 x 552 | `off[4]` +0x44, `rot` +0xB8 → `m` +0xC8; pieces +0xE8 | no | `debrisUpdateBursts` 8014BF3C (#5) | `debrisDraw` 8014B484 → `fxDebrisDraw` 8014C19C, per view | lerp off and rot, rebuild m, restore |
| Taser and lightning arcs | `weaponArcs` 801847B4 | 5 x 20 | pointers to car `motion.pos` or a fixed point | n/a | **none: the draw ticks** | `weaponArcsDraw` 801084D8 (in `streaksDraw`), per view | split the tick out; endpoints from the interpolated cars |
| Flamethrower emitters | `weaponFlameEmitters` 80184774 | 2 x 32 | from the car object | n/a | `weaponFlameUpdate` 8010A7A0, **inside `streaksDraw`** | through fxSprites | once per step |
| Mines and remote bombs | `weaponDrops` 80184B04 | 25 x 28 | object (+24) +0xA8 (z eased) | no | `weaponDropsUpdate` 80109D58, **inside `streaksDraw`** | `renderScene` | once per step; standard object blend |
| Ground fires | `groundFires` 801842C4 | 30 x 40 | +8 (static) | n/a | `groundFiresUpdate` 8010C160 (in `worldUpdate`) | none (5 static fxSprites each) | nothing |
| Skid/turbo smoke | none | – | – | – | `carSkidSmokeUpdate` 800DE4E8 spawns `fxSmokeC6` | through fxSprites | nothing |
| Water wake | `fxWakeObjects` 801879E8 | 5 pointers | object +0xA8 = car pos, z - 16 | no | `carGroundEffects` 800ED128 | `renderScene` | standard blend, or derive from the car |
| Screen flash | `fxSheetPolys` 80181DF8, `fxSheetTimer` 80180A60 | 2 POLY_F4 | screen | n/a | **`fxSheetDisplay` counts down in the draw** | `fxSheetDisplay` 800DC590 (in `streaksDraw`) | tick once per step |
| Eiffel tower | `eiffelTowerState` 801AB810 | 1 | model node +24..+32; `rot` +0x28 → node +4 | no | `eiffelTowerUpdate` 800D6800 (#10) | `renderScene` (model node) | blend rot and node offset, rebuild node +4 |
| Ricochet shots | `weaponRicochetPool` 80184860 | 10 objects + 10 x 48 | object +0xA8 | **yes, shot +0x20** | `weaponRicochetUpdate` 8010CC54 | `renderScene` | see (b): matrix is yaw-only |
| Shadow strike | `weaponStrikeState` 80184E10 | 1 | object +0xA8 | **yes, `previous[3]`** | `weaponShadowStrikeUpdate` 8010F9C0 | `renderScene` | standard object blend |

#### 1. Effect sprites `fxSprites` (80185144) — explosions, smoke, sparks, flames

The pool is 150 x 48 bytes (80185144..80186184). The struct is `TM2FxSprite` in `fx_sprite_spawn.c`; `Fx48` in `world_collapse.c` has better names on the draw side. (CONFIRMED)

| Off | Type | Field | Notes |
|---|---|---|---|
| +0 | u8 | type | 1 = rotated quad aimed along `zofs` (flames) |
| +1 | u8 | follow | non-zero: **the draw copies `*src` into `pos` on every draw** |
| +2 | u8 | tinted | |
| +3..+5 | u8 | r, g, b | tint |
| +6 | s8 | depth | OT bias (`fxSpriteSpawn`'s `count` argument) |
| +7 | u8 | slot index | set once by `fxSpritesReset` 80112820; a stable id |
| +8 | u8 | scale | the spawn "kind"; kind >= 96 lifts spawn z by 120 |
| +9 | u8 | rate | ticks per animation cell |
| +10 | s8 | frame | tick counter; **< 0 = free** |
| +11 | s8 | frames | end tick = rate x cells |
| +12 | s16 | loops | loops left; 0 ends the effect after this pass |
| +14 | s16 | zofs | aim point z offset (type 1) |
| +16 | ptr | table | 8-byte cells {u, v, w, h, tpage, clut} |
| +20 | s32[3] | pos | world position |
| +32 | s32[3] | vel | per-step velocity (smoke rises through `vel[2]`) |
| +44 | s32* | src | followed position (only used when follow != 0) |

- **Spawn.** `fxSpriteSpawn` 80110918 (`fx_sprite_spawn.c`) takes a free slot from `fxSpriteFindFree` 80110BA8 and sets frame = 0, vel = 0, pos = *pos.
- **Spawn wrappers.** Every effect below is a wrapper that calls `fxSpriteSpawn` and then sets vel, tint, type, zofs or loops:
  - explosions: `fxExplosion6/12/24/48/96/192` (`fx_explosions.c`) and `fxExplosionB12/B24/B48/B96/B96Fast`, `fxExplosionQuiet8` (`effect_bindings/`);
  - smoke: `fxSmokeA6..A128`, `fxSmokeB6..B48`, `fxSmokeC6..C48`, `fxSmokePuffBlue`, `fxSmokeBurstPink`, `fxSmokeBurstRed`;
  - sparks: `fxSparkOrange/Blue/Pink`, `fxLightningSpark` 80112600;
  - flames: `fxFlame3`, `fxFlameBurning` 80111D84, `fxFlame10`, `fxFlamePair`, `fxGroundFlame` 80111F48, `fxFlameStreamParticle` 80111FB0;
  - other: `fxBulletImpact` 8011270C.
- **Update.** `fxSpritesUpdate` 80110A40 (`car_restart.c`) runs once per step: frame++, pos += vel, then loop or free. It also calls `fxImpactParticlesUpdate` and counts `fxExplosionCooldown`.
- **Draw.** `fxSpritesDraw` 80110BE4 (`world_collapse.c`) runs per view. It calls `fxImpactParticlesDraw` first. The animation cell is `frame / rate`. It uses no `rand()`.
- **Draw-side writes.** For follow sprites it sets `pos = *src`. It also toggles `fxSpritesDrawParity` (80180B60), which has no reader. In view > 0 it skips records with frame 1 and loops 0.
- **Follow sprites.** Only `fxFlameBurning(car motion.pos, 1)` on burning cars, and the Dark Tooth swoop target, use follow. `src` points at the car record's `motion.pos`, not at object +0xA8.
- **Blend.**
  - Keep `prev[150][3]` = pos before `fxSpritesUpdate`. Set prev = pos when the slot was free before the update or was (re)spawned this step.
  - For follow sprites, draw at the interpolated owner car position instead.
  - Do not advance the animation cell on redraws.

#### 2. Impact particles `fxImpactParticles` (80184E74)

The pool is 20 x 36 bytes. The struct is `TM2ImpactParticle` (`fx_sprite_spawn.c`), also called `Particle36`. (CONFIRMED)

| Off | Type | Field | Notes |
|---|---|---|---|
| +0 | s32[3] | pos | world |
| +12 | s32[3] | vel | |
| +24 | u8 | frame | 0..3, cycles each step |
| +28 | s32 | life | starts at 40; **<= 0 = free** |
| +32 | ptr | anim | `fxDebrisFramesA` or `B` |

- **Spawn.** `fxImpactDebris(size, pos)` 80112C90 (`fx_debris_splash.c`), through `fxImpactParticleFindFree` 80112C54. It uses `rand()` for the velocities.
- **Update.** `fxImpactParticlesUpdate` 80112E58 runs from `fxSpritesUpdate`: pos += vel, vel.z -= 4, bounce or die at the ground, life--.
- **Draw.** `fxImpactParticlesDraw` 80112F6C (`car_damage_human.c`) writes no state.
- **Blend.** Keep a port prev (prev = pos on spawn) and lerp.

#### 3. Bullets and streaks `bulletRecords` (80181498)

The "40 streak slots" that `streaksDraw` draws are the machine-gun bullet records. The pool is 40 x 60 bytes; the struct is `TM2Shot60` (`weapon_bullets.c`). (CONFIRMED)

| Off | Type | Field | Notes |
|---|---|---|---|
| +0x00 | s32[3] | pos | head |
| +0x0C | s32[3] | prev | **previous-step position**, copied by `bulletsMove` before the move |
| +0x18 | s32[3] | vel | constant |
| +0x24 | s32 | owner | |
| +0x28 | s32 | life | 12 or 15 at spawn; **<= 0 = free** |
| +0x2C | s32 | power | |
| +0x30 | s32 | hit | |
| +0x34 | u8 | kind | 0 = thin line, otherwise a widened gouraud quad |

- **Spawn.** `bulletSpawn` 800D7A4C sets pos = prev = start. It is called from `update_guns.c` inside `worldUpdate` (LIKELY).
- **Update.** `bulletsMove` 800D7E30, called by `bulletsUpdate` 800D87B4, does life--, prev = pos, pos += vel, then 4 sub-step hit tests. `bulletsUpdate` also toggles `bulletsFrameParity` (80180A38).
- **Draw.** The tracer loop in `streaksDraw` (`screen_card_draw.c`) draws the segment prev→pos and writes no record state. Retail draws view 0 *before* `bulletsUpdate`, so view 0 shows the previous step's segment.
- **Blend.** Use `head = lerp(prev, pos, t)` and `tail = head - (pos - prev)`. A bullet spawned this step has prev == pos and stays a dot.

#### 4. Missiles and special projectiles `fxRecords` (801A5B08), with their trails

The pool is 16 x 980 bytes. Struct views: `TM2Proj` (`projectile_hit.c`), `TM2Projectile` (`weapons.h`), `TM2Trail` (`ai_route_nodes.c`), `TM2Fx980` (`weapon_missile_trails.c`). (CONFIRMED)

| Off | Type | Field | Notes |
|---|---|---|---|
| +0x00 | s8 | id | record index |
| +0x02 | s16 | type | 10 fire, 11 homing, 12 power, 14 freeze, 15 napalm, 49 ghost, 61 spiral, ... |
| +0x04 | s16 | life | > 0 flying; below 0 the trail lingers to -11; **-1 = freed** (`fxRecordFree` 800D914C) |
| +0x06 | s16 | speed | |
| +0x28 | s32[3] | origin | launch point; trail verts are relative to it |
| +0x34 | obj* | missile object | standard csList object |
| +0x38 | s32* | target | homing target position |
| +0x40 / +0x41 | s8 | trail segment / vertex slot | |
| +0x48 | SVECTOR[21] | trail verts | 7 slots x 3, `s16`, relative to origin |
| +0xF0 | obj* | trail object | csList object whose model is the strip header at +0xF4 |
| +0x14C | 18 x 36 | trail segments | quads, UVs (`missileTrailSetUVs` 800D8CEC) |

- **Objects.** `missilesCreateObjects` 800D90C0 and `missileTrailsInit` 800D87F0 create both objects at load with `csAlloc` + `csListAdd`. `renderScene` draws them.
- **Spawn.** The `weaponFire*` functions (800D9840..800DA448) and `weaponSpawnProjectile` 800D9F40 get a slot from `fxRecordFindFree` 800D91DC and call `fxRecordInitObject` 800D96F8.
  - Its parameters are misnamed: argument 2 becomes `obj->rot` and argument 3 becomes `obj->pos` and origin.
  - `missileTrailStart` 800D8AD4 collapses the trail at the launch point.
- **Update.** `weaponProjectileUpdate` 800DB110 steers (rot plus `RotMatrixYXZ`), moves `pos += matrix * (0, speed, 0)`, runs hit tests (which can snap the position to the ground), and calls `fxTrailUpdate` 800D9238. `fxTrailUpdate` rewrites the current slot's 3 verts every step and advances the slot every second step.
- **No previous position is kept.** The old position is only a local in `weaponProjectileUpdate`.
- **Blend.**
  - Use the standard object blend for the missile object. Snap on allocation and when the record is freed.
  - For the trail head, either accept 20 Hz or rewrite the 3 verts at slot +0x41 from the interpolated missile on redraws: `pos + matrix * {(0,0,8), (8,0,-8), (-8,0,-8)} - origin`. Restore them afterwards.

#### 5. Debris chunks `debrisChunks` (801A3D50)

The pool is 8 x 36 bytes. The struct is `TM2Shadow` in `engine_clut_anim.c`; the name is a leftover, these are the chunks. (CONFIRMED)

| Off | Type | Field |
|---|---|---|
| +0x00 | s32[3] | vel (gravity after age 10) |
| +0x0C | s16[3] | spin (added to obj rot each step) |
| +0x14 | TM2Obj* | object (standard layout) |
| +0x18 / +0x1C / +0x20 | s32 | active / age / life |

- **Spawn.** `debrisBreakModel` 8014B504 places the object at the part's position with rot = 0.
- **Update.** `debrisUpdateChunks` 8014B940 does rot += spin, `RotMatrixYXZ`, pos += vel, and hides the object at its end.
- **Draw.** `renderScene`.
- **Blend.** Standard object blend. The spin is at most about 80 per step. Snap on spawn.

#### 6. Debris bursts `debrisBursts` (801A3E70)

The pool is 4 x 0x228 bytes. Struct views: `TM2FxSlot` (update, `engine_clut_anim.c`) and `TM2Debris` (draw, `engine_fileio_debris.c`). (CONFIRMED unless tagged)

| Off | Type | Field |
|---|---|---|
| +0x000 | s32 | active |
| +0x004 | s32 | count (pieces captured) |
| +0x008 / +0x00C / +0x010 | s32 | age / end / phase-1 length |
| +0x014 | s32[4][3] | vel per corner group (phase 2) |
| +0x044 | s32[4][3] | **off: the moving part** |
| +0x074 | s32[4][3] | wpos (written, not read by the draw) |
| +0x0A4 | s32[3] | phase-1 drift |
| +0x0B0 / +0x0B8 / +0x0C0 | s16[3] | spin2 / rot / spin1 |
| +0x0C8 | MATRIX | m = `RotMatrixYXZ(rot)` |
| +0x0E8 | 20 bytes x up to 16 | pieces {model*, x, y, z, shift} |

- **Start.** `debrisBurstAt` 8014B820 moves the shared `debrisBurstObject` (801810EC) and calls `debrisStartBurst` 8014BAD4. That renders the object once (`renderDrawObjectUnculled`), and `lightAddSources` 8014D3F4 captures the pieces into +0xE8 (LIKELY: the asm renderer was not traced).
- **Update.** `debrisUpdateBursts` 8014BF3C.
  - Phase 1: off += drift, rot += spin1.
  - Phase 2: off[j] += vel[j], vel.z -= 1. **rot += spin2 and the matrix rebuild run inside the 4-group loop, so 4 times per step** (matches retail).
- **Draw.** `debrisDraw` 8014B484 → `fxDebrisDraw` 8014C19C. Corner c is placed at `m * (piece + camT + off[0] + ... + off[c])`, a cumulative sum. It writes only packets.
- **Blend.** Keep prev `off[4][3]` and `rot`. On redraws write the lerped values, rebuild `m`, draw, then restore. Snap on start.

#### 7. Taser and lightning arcs `weaponArcs` (801847B4)

The pool is 5 x 20 bytes. The draw-side names are `GC15Taser` (`weapon_arcs_draw.c`); the writer-side names are `TM2SlotRecord` (`weapon_slot_scan.c`). (CONFIRMED)

| Off | Type | Field |
|---|---|---|
| +0 | s16 | id (slot owner) |
| +2 | s16 | source (shooter) |
| +4 | s16 | count: 40 at start, **<= 0 free, decremented in the draw** |
| +6 | s16 | 1 = lightning, 0 = taser |
| +8 | s16 | target, -1 = none |
| +12 | s32* | from: the shooter's `motion.pos`, or a fixed point (`weaponLightningScanOffset`, `lightningScanOriginX..Z`) |
| +16 | s32* | to: the victim's `motion.pos`; with no target the draw points it at a stack local |

- **Spawn.** `weaponTaserTarget` 801075C8 and `weaponLightningScan` 80107B00, both through `weaponArcSlotFind` 80107574.
- **There is no update function.** `weaponArcsDraw` 801084D8 does it all, per view:
  - it jitters 16 points with `rand()`;
  - it spawns sparks with 1/64 chances;
  - it does count--;
  - it calls `carApplyDamage` at counts 39 and 0;
  - it sets `weaponArcsActive`.
- **Blend.** Move the tick into the update phase (see (d)). Cache the jittered points per step and draw from/to at the interpolated car positions.

#### 8. Flamethrower, mines and remote bombs, the screen flash, ground fires, smoke, wake

- **Flamethrower emitters `weaponFlameEmitters` (80184774).** The pool is 2 x 32 bytes (`Emit32` / `TM2Emitter_A7A0`).
  - Fields: +0 timer (50 down to 0), +2 stream length (up to 15), +4 offset, +16 world point (for sound), +28 the car object.
  - `weaponFlameStart` 8010A5F0 starts it. `weaponFlameUpdate` 8010A7A0 runs **from `streaksDraw`**. It walks the stream, spawns `fxFlameStreamParticle` sprites and does hit tests. It draws nothing itself.
  - (CONFIRMED)
- **Mines and remote bombs `weaponDrops` (80184B04).** The pool is 25 x 28 bytes (`Blend28`).
  - Fields: +0 kind (19 remote bomb, 20 mine), +8 timer, +10 settle height, +20 current height, +24 a pooled object.
  - `weaponDropsUpdate` 80109D58 runs **from `streaksDraw`**. It eases `obj->pos[2]`, does timer-- and runs `weaponDropCheckHit` 80109A9C. Allocation is `weaponDropWrite` 801096F4 (LIKELY).
  - (CONFIRMED)
- **Screen flash `fxSheet`.**
  - `fxSheetStart` 800DC45C fills 2 POLY_F4 over 0,0..320,240 and sets `fxSheetTimer`. It doubles the timer in split screen, but not for the mirror.
  - `fxSheetDisplay` 800DC590 links `fxSheetPolys[bulletsFrameParity]`, does timer-- and fades the colour below 8.
  - (CONFIRMED)
- **Ground fires `groundFires` (801842C4).** The pool is 30 x 40 bytes. Each fire is a static centre plus 5 `fxGroundFlame` sprite pointers.
  - `groundFireSpawn` 8010C020 starts one. `groundFiresUpdate` 8010C160 runs in `worldUpdate` and ends a fire by setting the sprites' loops to 0.
  - The fires have no draw of their own.
  - (CONFIRMED)
- **Skid and turbo smoke.** `carSkidSmokeUpdate` 800DE4E8 (`car_input.c`) keeps no records; it spawns `fxSmokeC6` sprites. (CONFIRMED)
- **Water wake `fxWakeObjects` (801879E8).** These are 5 object pointers. `carGroundEffects` 800ED128 sets pos = car pos (z - 16), rot = {0, 0, yaw} and rebuilds the matrix. (CONFIRMED)
- **Other object-based movers.** These all use the standard object blend:
  - Shields: `weaponShieldsUpdate` 8010DD30 copies the car pose, z - 64.
  - Tornado: `weaponTornadoUpdate` 8010E040.
  - Shadow strike: `weaponShadowStrikeUpdate` 8010F9C0, which keeps `previous[3]`.
  - Ricochet shots: `weaponRicochetUpdate` 8010CC54, prev at +0x20. Their matrix is special; see (b).
  - Dark Tooth boss: `aiDarkToothBossUpdate` 8010FD44.
  - (CONFIRMED; the homing leap 8010F520, pooled blasts and lava splashes are LIKELY.)

### (b) Rotation matrices written directly

There are two kinds of drawn transform (CONFIRMED, `reference/renderer_walker_engine_render_scene.c`):

- **csList objects.** These have +0x80 matrix, +0xA0 rot and +0xA8 pos. `csUpdMat` 801491E8 composes them with the parent at draw time.
- **Type-5 transform nodes inside model data.** These have a rotation at node +4 and an offset at node +24..+32. They have no angles and no object position, so **blending objects does not move them**.

The table lists every transform that is not simply "game code sets +0xA0, then `RotMatrixYXZ(+0xA0 → +0x80)`".

| VA | Name | File | Object | What it writes | Rebuilding from angles works? | Advice |
|---|---|---|---|---|---|---|
| 800DF3EC, 8011C400, 8011C4A0 | `CarRotUpdate`, `humanCarUpdate`, `aiCarUpdate` | car_motion.c, car_records.c | every car object | `body_m` = `RotMatrixYXZ(rot + tilt)`. The tilt is X += aim pitch +0x30, wheel pitch +0x1C, lean pitch +0x16; Y += shake +0x32, wheel roll +0x1E, lean roll +0x18 (aim block: human +0x114, AI +0x9C). The update then copies +0xA0 = `motion.rot`, +0xA8 = `motion.pos` and **+0x80 = `body_m`** (`mathCopyWords`, 8 words) | **No.** +0xA0 has no tilt | Blend rot + tilt sums (mod 4096) and rebuild, or nlerp/slerp the two +0x80 matrices. Snap on placement or warp |
| 80114F24 | `carBodyPoseUpdate` | car_body_pose.c | type-5 car body node `carBodyObjects[type>>4]`, a separate world model | node +4 = `RotMatrixYXZ` of pitch and roll from the wheel heights (`ratan2`) and yaw = `motion.rot[2]`; node t = car x, y and the mean wheel height | no angles stored | Save prev/cur matrix and t (nlerp, lerp), or hook it to keep the 3 angles. The node is shared per car type |
| 801144F8, 801145E8, 801146D8, 801147C8, 801148B8 (from `carPartsAnimate` 80114B70) | `carPartsSpinHammerhead` / `Minion` / `MrSlam` / `DarkTooth` / `Axel` | car_parts_spin.c | type-5 part nodes (rotors, Axel's wheels; ids 34/226/194/242/178) | `RotMatrixX(-angle)` copied (5 words) into node +4. The angle is a per-type global (`carPartsSpinAngle*` 80180B78..80180B88) that changes by speed/7 per step | angle is a global scalar | Lerp the global (unbounded `s32`), redo `RotMatrixX` and the copy |
| 80115188, 801151D0, 80115230, 8011526C | `slamArmRegister`, `slamClawRegister`, `slamArmSetAngle`, `slamClawSetAngle` | arena_load_register.c, misc_one_liners.c | Mr. Slam arm and claw nodes (ids 201/202) | node +4 from `slamArmAngle` / `slamClawAngle` | scalar global | Lerp the scalar and rebuild |
| 800D6800, 800D6710, 800D6E4C | `eiffelTowerUpdate`, `eiffelTowerStartFall`, `eiffelTowerReset` | car_object_contact.c, weapon_fire_specials.c, arena_setup.c | Eiffel tower node (id 3201) | node +4 = `RotMatrixYXZ(eiffelTowerState.rot)`; node offset +28 += 5, +32 += speed | angles are in `eiffelTowerState` (801AB810), not in an object | Blend state rot and node +24..+32, then rebuild |
| 80116B4C, 80119174, 8011930C, 80119468, 80119530 | `worldCollapseMoveSection`, `fxSplashAt`, `fxLavaSplashAt`, `fxSpawnPooledBlast`, `fxTargetHitAt` | world_collapse_warps.c, fx_debris_splash.c, car_parts_spin.c, fx_explosions.c, arena_load_register.c | collapsing ice sections and round-robin splash/blast nodes | node position written directly (kind 4: node +4; kind 5: node +0x18) | no object | Key by node; lerp the node offset; snap on reuse |
| 8010CC54 | `weaponRicochetUpdate` | weapon_ricochet.c | 10 ricochet shot objects | +0x80 = `RotMatrixYXZ({0, 0, rot[2]})`, built **before** the move; a bounce does rot[2] += 2048 later in the step; rot[0] holds the car's pitch but is ignored | **No** (pitch ignored; +0x80 lags +0xA0 by a step after a bounce) | Rebuild yaw-only from the yaw +0x80 was built from; snap on the 180° bounce |
| 80126464 | `targetActorsUpdate` | target_actor_update.c | walking pedestrians | rot[2] = `ratan2(goal)`, then `RotMatrixYXZ`; at a goal flip rot[2] += 0x800 *after* the build | mostly (one step 180° off after a flip) | Blend angles; snap when \|Δyaw\| >= 1024 |
| 8014BF3C | `debrisUpdateBursts` | engine_clut_anim.c | burst slots (not objects) | slot `m` from slot `rot`, rebuilt 4x per step in phase 2 | yes, in the slot | See (a) 6 |
| 80155E80 | `viewProc` (draw) | view_system.c | camera object and its parent | `RotMatrixYXZ(+0xA0 → +0x80)` for both on every draw. In mode 4 (mirror) it first copies view 0's camera and parent pose into view 1, then adds 0x800 to yaw and subtracts 56 from pitch | yes | Blend view 0's angles before both `viewProc` calls |

**Checked and plain (angles → matrix each step, so blending the angles works; CONFIRMED):**

- Missiles: `fxRecordInitObject` 800D96F8, `weaponProjectileSteer` 800DA5B4, `weaponProjectileUpdate` 800DB110. rot[0] and rot[2] wrap, so use the shortest arc.
- Debris chunks: `debrisUpdateChunks` 8014B940.
- Shields 8010DD30. These carry no car tilt, as in retail.
- Tornado 8010E040. It also turns the victim car by up to 341 per step.
- Shadow strike 8010F9C0. It stores its ground pitch and roll back into +0xA0.
- Dark Tooth boss 8010FD44.
- Track vehicles: `worldTrackVehiclePlace` 801179CC, `trackVehicleGround` 80118BBC.
- Water wake 800ED128.
- Camera pose: `viewSetCameraPose` 8011D388.
- Shell and menu scene objects: 80122124, 8012D184, 8012D908, 8012A0E0. They build their matrix at draw time and are outside `rtMainLoop`.

**Not drawn matrices (CONFIRMED):**

- These write only local matrices: `viewChaseCamera` 80102CC4, `carCarCollision`, `carContactRespond`, `carBodyAttitudeUpdate` 800F0944, the AI route helpers, `targetActorHit` 80127228, and `weaponHomingLeapUpdate` 8010F520.
- Turrets and guns (`update_guns.c`) only read `motion.rot_m` to place muzzles. There is no turret matrix.
- Car pitch and roll code (`car_rollover.c`, `car_tumble.c`, `car_crash.c`, `car_spin.c`, `car_wheel_sway.c`) writes only `motion` and tilt fields, which reach the screen through `CarRotUpdate`.

**Animation and selection nodes.**

- Type-3 animation nodes and type-9 select nodes pick a child by index and have no matrix. They cover the tire switches (`uaswToggleCarTireState` 80124800), the car damage stages and the frozen-car model.
- Do not interpolate them; they change at 20 Hz. (CONFIRMED)

**Renderer outputs are derived state.**

- `csUpdMat` 801491E8 writes object +0x14..+0x1C (world position), +0x20, +0x40, +0x60 and +0x124 (cull result) on every draw. It never writes +0x80.
- These fields are recomputed on each draw. After the last redraw they hold whatever that redraw used (see (d), "Restore after the redraws").
- (CONFIRMED)

**Simulation readers of +0x80** (why restore matters; LIKELY read-only): `targetActorsUpdate`, `trackVehicleGround`, `weapon_targets.c`, `weapon_missile_trails.c`, `weaponFlameUpdate`, `fxTrailUpdate`, `debrisBreakModel`, `eiffelTowerDebrisPoint`, and the ricochet collision.

### (c) Camera cuts and teleports (where the blend must snap)

#### Rules

1. **Snap everything when `rtMainLoop` 800D1318 starts.** Every round, match start and lost life passes through it.
   - The runner first calls `roundStart` 8011A024. That runs `srand` at 8011A03C, `carFreezeResetAll`, `debrisClear`, `humanCarsRestart` 8011A198 and `weaponsRoundReset`.
   - At load, `arenaSetup` 8011995C and `arenaResetSystems` 80128CE4 also run first.
   - (CONFIRMED)
2. **Snap any transform that was not drawn last step.** Visibility is +0x08 (`csSetDrawEnable` 80149604, plus many direct stores), `renderAddWorldModel` / `renderRemoveWorldModel` 801512F8 / 80151338, or a record's active flag. Test +0x08 per object each step rather than hooking the function. (CONFIRMED)
3. **Snap a pool slot when it is allocated.** A slot freed and reused in the same step otherwise looks continuous. The allocators are listed below. (CONFIRMED)
4. **Distance backstop.** Positions are world units; one step = 3 ticks.
   - A car moves at most about 103 units per step (human) and about 145 (hard AI with turbo). See `carMoveByVelocity` 800DF594 and `docs/GAME_EVENTS.md` section 7.
   - The shortest teleport is 560 units (Hong Kong warp 0, straight up).
   - Suggested limits: **400** for cars and objects attached to cars, **512** for camera position. Projectiles need their own limit or rule 3 only.
   - Angles are a poor signal: spin-outs and the chase camera turn several hundred units per step. Always blend angles by the shortest arc, `((cur - prev + 2048) & 4095) - 2048`.
   - Scale the limits if `timerGetFrameTicks` (8015531C) changes.
   - (CONFIRMED speeds, suggested limits)

**Timing caveat for cars (CONFIRMED).** `humanCarUpdate` / `aiCarUpdate` copy the car record into the object. A move made after that copy reaches the object only in the next step's copy. The warps at the end of `worldUpdate` are such a move. The camera has the same lag, because `viewCarCameraUpdate` reads `cam_pos` / `cam_rot` early in `worldUpdate`. So keep a snap flag raised by a hook for two captures. Moves made inside `aiCarDrive` / `humanCarTick`, and the attract respawn before the AI loop, are copied in the same step.

#### Events

| Event | VA | Name | File | What jumps | Hook | Conf. |
|---|---|---|---|---|---|---|
| Round loop entry (rounds, match start, lost life) | 800D1318 | `rtMainLoop` | rt_main.c | everything | snap all at entry, before its first `worldUpdate` | CONFIRMED |
| Human car placed at start | 800FD6C4 | `humanCarPlaceAtStart` (from `carRestart` 800FD550) | car_object_contact.c | car pose; **`cam_pos` = car, `cam_rot` = (0, 0, heading)** | wrap; snap car and that player's camera (also the port's lives respawn, `COOP_LIVES.md`) | CONFIRMED |
| Any AI placement | 800F6518 | `aiPlaceOnRouteNode` | ai_route_setup.c | AI pose and wheels | **wrap; snap that AI car.** It covers `aiCarStart` 800FFEFC, the stuck re-place `aiReplanRoute` 800E9578, the `aiCarDrive` re-place flag, warp exits and attract respawns | CONFIRMED |
| Teleporter move (warp frame 10) | 801157D4 | `worldWarpsUpdate` | world_collapse_warps.c | car `motion.pos` → destination; human: `cam_pos` / `cam_rot` cut; AI: also `aiWarpSetExitNode` 80115A48; the warp effect object moves | wrap; before it, note records with timer == 9; after it, snap their car (record +32), the camera and the warp object (+36). Runs last in `worldUpdate` (timing caveat) | CONFIRMED |
| Warp effect starts | 8011566C | `worldWarpStart` | fx_debris_splash.c | warp object shown at the entry | rule 2 | CONFIRMED |
| Warp pairs per arena | 80115310 | `worldWarpsSetup` | car_damage_human.c | data: levels 1, 3, 4, 5, 6, 7, 8 (three on Hong Kong); none on Moscow or the secret levels | – | CONFIRMED |
| Camera view mode set | 800E7380 | `carSetViewMode` | weapon_missile_trails.c | `cam_pos` / `cam_rot` reset to the car (modes 1, 5, 6 always; 2, 3, 4 when re-selected); view 7 jumps to the watched AI car | **wrap; snap that player's camera.** Callers: `carViewHotkeys` 8011C530, `humanCarsRestart`, `carBindHumanSlot` 8011A3FC, `roundStart`, `demoWatchNextAiCar`, and the forced switch to view 2 on ground class 1 in `car_crash.c` | CONFIRMED |
| Fixed camera (views 5, 6) turns at once | 8010353C | `viewFixedCameraTrack` | camera.c | `cam_rot` jumps to the target when the yaw error is over 512 | snap camera rotation on a yaw step > 512 | CONFIRMED |
| Split layout toggled | 8011C530 | `carViewHotkeys` → `viewSetSplitMode`, `viewCreateWithCamera` 80119CA0 | car_records.c | viewport, projection and chase offset change; camera parent reset to `viewSharedRoot` | snap both views | CONFIRMED |
| Rear-view mirror toggled | 800D1BB8 | `rtToggleRearView` | rt_main.c | view 1 appears; `viewProc(1)` copies view 0's pose and turns it 180° | nothing, if view 0 is blended before both `viewProc` calls | CONFIRMED |
| Attract demo respawn (every 601 ticks) | 8011EFDC | `demoRespawnAiCars` | spawn_tables.c | every AI car; `debrisClear`; may move player 0's camera | wrap; snap all cars, debris, camera 0 | CONFIRMED |
| Attract demo watch switch (every 301 ticks) | 8011EDD0 | `demoWatchNextAiCar` | world_collapse_warps.c | camera to the new car; the sky follow object (`viewSetFollowObjPos` 801563BC) moves | wrap; snap camera 0 and the follow object | CONFIRMED |
| Hong Kong train re-placed | 80118F38 → 801179CC | `trackVehicleDestroy` → `worldTrackVehiclePlace` | fx_debris_splash.c, car_parts_spin.c | train hidden, then put back at its start | wrap the place; snap the train | CONFIRMED |
| Hong Kong train station stop | 80117B40 | `worldTrackVehiclesUpdate` | track_vehicles.c | a hidden train is snapped to x 6400 or 3200 and shown | rule 2 | CONFIRMED |
| Hong Kong trains on the oval | 80117B40 | same | track_vehicles.c | **no wrap** (closed oval, 45.6 units per step); headings jump between fixed values at corners | shortest-arc blend only | CONFIRMED |
| Pedestrian respawn | 801259A4 → 80125A18 | `targetActorRespawn` → `targetActorPlace` | target_actor_lifecycle.c, target_actor_init.c | pedestrian to a random start row | wrap the place; snap the actor | CONFIRMED |
| Pedestrian spawned | 80125888 | `targetActorSpawn` | target_actor_lifecycle.c | new object | rule 2 | CONFIRMED |
| Eiffel tower falls | 800D6800 | `eiffelTowerUpdate` | car_object_contact.c | pops up 200 units in one step, then tilts | blending is harmless; interpolate the node itself, see (b) | CONFIRMED |
| Building and dish collapses, LA quake, target events | 800D4D58, 800D6188, 800D5AF0, 801274C0, 801277A8, 80127CE0 | `moscowEventsUpdate`, `moscowDishUpdate`, `laEventsUpdate`, `targetTimedEventUpdate`, `targetLevelEventUpdate`, `targetPlayerEventsUpdate` | random_event_level2.c etc. | **no object moves**: a node is hidden, a debris burst starts, and `objRegistrySetStep` 80124384 switches to the wrecked child | burst slot snap (rule 3) | CONFIRMED |
| Antarctica collapsing ice | 801163B8, 80116B4C | `worldCollapseUpdate`, `worldCollapseMoveSection` | world_collapse.c, world_collapse_warps.c | random shake up to 40 per step, then a fall of up to 96 per step; continuous | blend the nodes (see (b)) | CONFIRMED |

**Pool allocations (rule 3).**

| Pool | Allocator | Notes | Conf. |
|---|---|---|---|
| bullets | `bulletSpawn` 800D7A4C / `bulletFindFree` 800D876C | – | CONFIRMED |
| missiles | `fxRecordFindFree` 800D91DC | the object jumps from the last missile's end to the launch point | CONFIRMED (allocator), LIKELY (every spawner) |
| missile trails | `missileTrailStart` 800D8AD4 | – | LIKELY |
| effect sprites | `fxSpriteSpawn` 80110918 / `fxSpriteFindFree` 80110BA8 | – | CONFIRMED |
| impact particles | `fxImpactDebris` 80112C90 | – | CONFIRMED |
| debris chunks | `debrisBreakModel` 8014B504 | – | CONFIRMED |
| debris bursts | `debrisBurstAt` 8014B820 / `debrisStartBurst` 8014BAD4 | the shared `debrisBurstObject` moves to each new burst | CONFIRMED |
| mines and remote bombs | `weaponDropWrite` 801096F4 | pooled objects | LIKELY |
| splash, lava, blast and target-hit nodes | 80119174, 8011930C, 80119468, 80119530 | round-robin reuse | CONFIRMED |
| water wake | `fxGetFreeWakeObject` 801195C0 | – | LIKELY |
| shields | – | snap with their car | CONFIRMED |
| ricochet, strike, claw objects | – | rule 2 | LIKELY |

**Key blend history by object pointer, not by list slot.** `carBodyObjectShow` 80114E10 removes and re-adds a world model, which can change its index in `renderWorldModels` (801A4E1C). (CONFIRMED)

**Checked, no snap needed (CONFIRMED):**

- Car destroyed (`carDestroyed` 8011BE10): no move; sunk cars are hidden.
- Falling off the map: no move.
- Riding a train: continuous.
- Pickups reappearing (`pickupRespawnTick` 8010AED0): a fixed node is shown again.
- Car freeze and damage-stage switches: they change the child model only.
- Dark Tooth body: eased, continuous.
- The chase camera (`viewChaseCamera` 80102CC4): eased.
- `viewCarCameraUpdate` 80102BE8's per-step `viewSetCameraPose(i, 0, 0)`: no change.

**Camera object substitution (CONFIRMED).**

- `viewCameraObjects[i]` (80181110) is allocated once by `viewCreate` 80155888 and is not repointed until `viewInit` 8015580C.
- The parent (+0x10) changes on a second `viewCreateWithCamera` (split toggle), from `viewCameraObjectTable[i]` back to `viewSharedRoot`. Both parents have identity poses, so the change is visually a no-op. Snap anyway when the parent pointer changes.
- Spectating does not repoint anything. Views 5 and 7 write another car's pose into `cam_pos` / `cam_rot` (`aiGetWatchedCar` 8011D128).

**Not in the 3D loop (CONFIRMED):**

- **No death, kill or winner camera.** The camera keeps chasing the wreck until the round ends (420 or 210 ticks).
- **No intro fly-in.** Boss and level intros are text screens shown by the runner.
- **Shell scenes are separate loops.** Title, car select and ending (`sceneRun` 80122B34) are placed with `sceneObjSetPosition` 80123E1C. Leave them at their native rate.

**Pause (CONFIRMED).** `pauseMenu` 80129528 blocks inside the step with its own `VSync` loop.

- Hold the last frame while paused.
- Reset the port's step accumulator on resume.
- Retail's `SetFrameStart` 80155214 clamps the next step to 5 ticks.

### (d) State changed inside the draw, HUD and pause paths

#### `streaksDraw` 800D8230 (screen_card_draw.c) and its callees: update work, run once per step

The call order is `fxSheetDisplay` (if `fxSheetTimer > 0`), `weaponDropsUpdate`, `weaponFlameUpdate`, `weaponArcsDraw`, then the tracer loop. Only the tracer loop and the `AddPrim` calls are draw.

**Retail already runs these ticks twice per step** when split screen or the rear-view mirror is on: `streaksDraw(…, 1)` runs, and the callees ignore the view argument. Only `fxSheetStart` compensates, and only for split screen. To keep retail timing, run one tick per retail `streaksDraw` call (1 or 2 per step), independent of the redraw count. (CONFIRMED)

| VA | Name | File | State changed | If run 3x | Advice | Conf. |
|---|---|---|---|---|---|---|
| 80109D58 | `weaponDropsUpdate` | fx_sprite_spawn.c | `weaponDrops[25]`: timer--, obj z eased, object released at 0 | fuses and drop animation 3x fast | update phase | CONFIRMED |
| 80109A9C | `weaponDropCheckHit` (from the above) | car_damage_human.c | mine: `fxExplosion24`, `weaponApplyHit`; remote bomb: `fxExplosion48`, `moscowEventsTrigger` / `laEventsTrigger` / `eiffelTowerBombCheck`, `areaDamage`, held/ammo--; `projectileHitTest` 800D6FD8 → `carSpinOut` | owner-skip window 3x shorter; hits land on other steps | update phase | CONFIRMED |
| 8010A7A0 | `weaponFlameUpdate` | weapon_flame_update.c | emitter timer--/length++; `weaponApplyHit`; ignition (`burning`, `burn_time`, `fxFlameBurning`); flame markers (`worldFlameMarkerGet`, `fxFlamePair`, `fxExplosion48`, `clutAnimStartSweep`); `soundStop(47)`; `rand()` at 8010A948, plus 80111FBC..80112044 in `fxFlameStreamParticle` and 8010BE14..8010BFB0 in `mathJitterPos` | flame lasts 1/3, 3x damage, 3x shared `rand()` | update phase | CONFIRMED |
| 801084D8 | `weaponArcsDraw` (`draw_tasers`) | weapon_arcs_draw.c | `weaponArcs[i].count--`; `carApplyDamage(target, 10, 0, 2, source)` at counts 39 and 0, which has a **GAME_STATE** `rand()` at 800E59C4; VISUAL `rand()` at 80108768, 801087A4, 801087E0, 801088D4, 801088F4, 8010890C, 80108A9C, 80108AC4 (about 45 per live arc); 1/64 spark spawns; `weaponArcsActive` (80180B3C, no reader found) | tasers last 1/3, damage moves earlier, stream desync, 3x sparks | split: count, damage, spark gates and jittered world points once per step; the redraws only project cached points | CONFIRMED |
| 801084D8 | `weaponArcsDraw` | weapon_arcs_draw.c | `.to` = a stack address when there is no target | harmless here; never dereference `.to` elsewhere | cache the free end per step | CONFIRMED |
| 80110918 and wrappers | `fxSpriteSpawn`, `fxExplosion*` (reached from the three above) | fx_sprite_spawn.c, fx_explosions.c | `fxSprites` slots, `fxExplosionCooldown`, `soundPlay(7/8)` | 3x particles, pool exhaustion, extra sounds | through their callers, in the update phase; keep them before `fxSpritesUpdate` as retail does | CONFIRMED |
| 800DC590 | `fxSheetDisplay` | weapon_special_missiles.c | `--fxSheetTimer`; fades `fxSheetPolys` r, g, b by `(c*3+128)>>2` below 8 | flash 1/3 as long | count and fade in the update; redraws only link the prim | CONFIRMED |
| 800DC590 | `fxSheetDisplay` | weapon_special_missiles.c | links the static `fxSheetPolys[bulletsFrameParity]`; view 0 and view 1 get different polys only because `bulletsUpdate` toggles the parity between them | if both views link the same prim into live OTs, the chain breaks | copy the prim per view and per redraw | CONFIRMED (code); corruption is reasoning |

The tracer loop (`screen_card_draw.c`, about lines 463–551) only reads `bulletRecords` and writes primitives. It is clean (CONFIRMED). Missile trails are not drawn here (see (a) 4).

#### `renderFlipFrame`, `renderScene` and the animators

| VA | Name | File | State changed | If run 3x | Advice | Conf. |
|---|---|---|---|---|---|---|
| 80152194 | `renderFlipFrame` | engine_render_scene.c | `renderPrevFrameParity` (80180DBC) = `renderFrameParity` (80180DB8), which is then flipped; `renderFrameTicks` = ticks per frame | re-arms every animation's done flag, so animations and their `rand()` run 3x | **once per step; it is update bookkeeping, not draw** | CONFIRMED |
| 80151700 / 801518E8 | `animUpdateAll` (end of `renderScene`) / `animUpdateOne` (walker type-3 node, 80152FEC) | engine_anim_hit_volumes.c | `done[prev] = 0`; steps only if `done[cur] == 0`, then sets it | self-guarded: safe to redraw while the parity is unchanged | keep `renderFlipFrame` once per step; animations stay at 20 Hz | CONFIRMED |
| 80151BC4 / 80151CB8 / 80151DF4 | `animStepRandom` / `animStepWeightedRandom` / `animPickPeriod` | engine_anim_hit_volumes.c | VISUAL `rand()` at 80151C00, 80151D30, 80151E24, 80151EA0 | at most once per step, but only for nodes that pass culling, so an interpolated camera can change which run | private visual stream | CONFIRMED (guard), LIKELY (culling effect) |
| 80152284 | `renderScene` | engine_render_scene.c | `renderFarDistance` (set and restored), `renderSkipNodeCull`, scratchpad, `renderCurrentOt` | rebuilt on each call | harmless | CONFIRMED |
| 801491E8 | `csUpdMat` | engine_clut_anim.c | object +0x14..+0x1C, +0x20, +0x40, +0x60, +0x124 | recomputed; keeps the last redraw's values | see the restore table below | CONFIRMED (writes) |

`debrisStartBurst` 8014BAD4 renders an object (`renderDrawObjectUnculled`) from the *update* phase, using whatever camera `viewProc` last set. It can also reach `animUpdateOne`. Make sure the real step camera is in place when the updates run. (LIKELY: the walker is asm.)

#### HUD (#21)

`hudDrawSingleView` 800F8370 (hud_main.c) runs once per frame. `hudDrawSplitVertical` 800F9084 and `hudDrawSplitHorizontal` 800F9D48 (hud_split.cpp) run once per player. The `first` argument only picks one of `hudOrderTables[buffer][player]`. The HUD tree has **no `rand()`, sound, SPU or pad calls**. The four radar `rand()` sites (8011A828, 8011A8F0, 8011B460, 8011B530) are in `worldUpdate`. (CONFIRMED)

| VA | Name | File | State changed | If run 3x | Advice | Conf. |
|---|---|---|---|---|---|---|
| 800FBDFC | `hudRadarSwapBuffers` (end of `hudDisplayRadar` 800FBE68) | ai_route_helpers.c (caller hud_radar.c) | `hudRadars[idx]` (801837C8): blip count +0x360 = 0, line-buffer half +0x36C flipped, arrow flags +0x35C..+0x35F = 0 | blips are plotted once per step (`hudRadarPlotCar`), so redraws 2 and 3 show an empty radar | swap only on the last redraw of a step (and only while the radar is shown, as retail) | CONFIRMED |
| 800FBC44 | `hudMessageUpdate` (single view only) | hud_state.c | `hudMessageCurrent[5]` (80183F00): [4] countdown; at 0 it promotes the pending message and sets [4] = 100 and [1] = `aiCarsCountAlive()` | messages expire 3x fast; lower-priority messages get in sooner | once per step (update phase, or the first redraw) | CONFIRMED |
| 800FB588 | `hudBlinkWeaponMarker` | hud_numeric.cpp | `hudMarkerBlinkCounters[player]++`, `hudMarkerBlinkPhases` toggled every 5th call | drop-weapon marker blinks 3x fast | count only on the first redraw | CONFIRMED |
| 800FD0D0 | `hudDrawTargetPanel` | car_destroyed.c | sets the target car's stats +58 to 1 if it is 0 (LIKELY max HP) | idempotent | harmless | CONFIRMED (write) |
| 800F9084 / 800F9D48 | split HUDs | hud_split.cpp | `fontSetColor(2, …)` not always reset | same end state | harmless | CONFIRMED |

Clean: `UAdashDrawRearViewMirror` 800FBB0C, `hudDrawNumber` 8014E194, `hudPrintNumber3` 800FB768, `hudBlinkWeaponDigits` 800FB684, `UAdashHealthColor` 800FBD04, `hudDisplayRadar` (apart from the swap), the font print functions, and every getter they call. (CONFIRMED)

#### `clutAnimUpdate` (#19), `pauseMenu` (#22), the arena events (#10)

- **`clutAnimUpdate` 80148B1C (engine_clut_anim.c).**
  - It walks the 10 `clutAnimRunning` records (80197FE8, 1060 bytes each): tick++, then a step by type, and clears `active` at the end of a sweep or fade.
  - The step types: `clutAnimStepCycle` 80148ED8 for types 0 and 3 (rotate the palette by one entry), `clutAnimStepSweep` 80148BEC for type 1 (a white band, started by `clutAnimStartSweep` from the flamethrower), and `clutAnimStepFade` 80148D7C for type 2.
  - It uploads the palette with `LoadImage`. The VRAM keeps it across redraws.
  - **Once per step.** It is not a redraw call. (CONFIRMED)
- **`pauseMenu` 80129528 (screen_pause_menu.c).** It returns at once unless START is held; then it updates `padActionCache` with the same value each call. With START held:
  - it blocks in its own `VSync` loop and draws straight into the displayed buffer with `DrawPrim` / `MoveImage`, not into the OT;
  - it mutes and restores music, plays sounds 28/29, sets the pad repeat mode, and on quit calls `rtScheduleEvent(2, 0)`;
  - `screenLostALife` 8012940C dims the shown buffer;
  - the pad drain loops (`while (padAnyPressed) padUpdate();`) have no `VSync`, so on PC `padUpdate` must poll host input.
  - **Run it once per step, after the step's last presented frame.** It has no `rand()`. (CONFIRMED)
- **Arena events (#10).** They draw nothing. None of them or their callees calls `AddPrim`, `DrawPrim`, `DrawOTag`, `LoadImage` or `MoveImage`.
  - Their flash is `fxSheetStart` 800DC45C, which only fills `fxSheetPolys` and sets the timer; `fxSheetDisplay` draws it.
  - Their `rand()` sites run once per step if they stay in the update.
  - Order note: retail runs view 0's `streaksDraw` before #10, so an event's flash first shows in view 0 one step later. A port that moves all draws after all updates shows it one step earlier.
  - (CONFIRMED)
- **`explodeMakeScreenRed` 8011335C.** It adds a full-screen quad to the current OT from outside the draw path, but it has no caller in the Hex-Rays export or in `src`. It is dead code. (CONFIRMED)

#### Restore after the redraws

These are not retail bugs. They matter because the port swaps interpolated values in for the redraws.

| VA | Name | Written | Why it matters | Advice | Conf. |
|---|---|---|---|---|---|
| 80155E80 | `viewProc` | `viewEyePositions`, `viewMatrices`, the camera record (`csGetRoot`), +0x80 of the camera and parent, GTE projection; in mode 4, view 1's pose | the sound listener (`sndListenerTransform` 8015DBE8, through `viewGetEyeTrans` / `viewGetViewMatrix`) and `debrisStartBurst` read them during the update | make the last redraw of each step t = 1, or rerun `viewProc` with the real camera before the update | CONFIRMED (writes, sound reader) |
| 801491E8 | `csUpdMat` | object +0x14..+0x1C, +0x20/+0x40/+0x60, +0x124 | game readers of +0x14 were not traced; no game reader of +0x124 (+292) was found | last redraw at t = 1, or restore | CONFIRMED (writes), LIKELY (readers) |
| 80110BE4 | `fxSpritesDraw` | follow sprites' `pos = *src` | keeps the interpolated value; no other reader found | last redraw at t = 1 | CONFIRMED (write), LIKELY (no reader) |
| 800D1318 #1 | buffer flip and `ClearOTagR` | `D_80180EE4`, packet pointer reset | each redraw needs a cleared OT and a reset packet pointer, or the packet buffer overflows | reset per redraw | CONFIRMED |

#### Clean draw functions (CONFIRMED)

These write only primitives, the OT, the packet pointer, the scratchpad, GTE registers or values recomputed on every call:

- `viewProc` (no counters or `rand()`);
- `fxSpritesDraw` (apart from the follow copy and the unread parity);
- `fxImpactParticlesDraw` (it never decrements `life`);
- `debrisDraw` and `fxDebrisDraw`;
- the `streaksDraw` tracer loop;
- `renderScene`, `renderInitState` 801521D0, `renderWalkModel` 801526CC (apart from the guarded `animUpdateOne`), `renderPushMatrix`, `renderMulTransMatVec`.

The model emitter 800CDC54 and `renderNearPolys` 80154690 are LIKELY clean too: they are asm and were not audited store by store.

#### Recommended split

1. **Update, once per step:**
   - every retail update call;
   - `renderFlipFrame` and `clutAnimUpdate`;
   - `hudMessageUpdate` and the `hudBlinkWeaponMarker` counter;
   - from `streaksDraw`: `weaponDropsUpdate`, `weaponFlameUpdate`, the simulation half of `weaponArcsDraw`, and the `fxSheetTimer` countdown and fade. Run these once per retail `streaksDraw` call, which is twice when split screen or the mirror is on.
2. **Draw, 3 times per step:**
   - buffer flip and `ClearOTagR`, then `viewProc`;
   - the tracer loop, taser lines from cached points, and a per-view copy of the flash prim;
   - `fxSpritesDraw` (with the impact particles), `debrisDraw` and `renderScene`;
   - `DrawOTag`, then the HUD, with the radar swap only on the last redraw.
3. **After the last presented frame, once per step:** `pauseMenu`.
4. **Last redraw at t = 1**, or restore `viewProc`'s outputs, the `csUpdMat` fields and the follow-sprite positions.
5. **Private visual `rand()` stream** for the animator sites and the arc jitter.
6. **View 1 draws a newer state than view 0.** In split screen and mirror mode, retail draws view 1 after `worldUpdate`. A port that draws both views after all updates makes view 0 one world step fresher than retail. That is visual only, but the tape test should not compare the draw order.

### Corrections for the decomp docs

**docs/MAIN_LOOP.md**

1. **#8 `streaksDraw`.** `weaponFlameUpdate` is now read and is update work: damage, ignition, spawns and `rand()`. Add three more ticks to caveat 1:
   - `weaponArcsDraw` ticks the taser count, calls `carApplyDamage` (GAME_STATE `rand()` at 800E59C4) and calls VISUAL `rand()`;
   - `fxSheetDisplay` counts down `fxSheetTimer`;
   - all these ticks already run twice per step with split screen or the mirror.
2. **#14 `renderFlipFrame`.** It is step bookkeeping. It must run once per step, not per redraw, because it re-arms the animation guard. Remove it from the "re-run on in-between frames" list.
3. **#17 view 1 / mirror.** It runs after `worldUpdate` (#16), so it draws a state one world step newer than view 0.
4. **#19 `clutAnimUpdate`.** It is now identified: palette cycle, sweep and fade animations (10 records x 1060 bytes) uploaded to VRAM. Run it once per step.
5. **#21 HUD.** It is checked. It has three once-per-step side effects: the radar swap 800FBDFC, `hudMessageUpdate` 800FBC44 and the `hudBlinkWeaponMarker` 800FB588 counter. It has no `rand()`, sound or pad calls.
6. **#22 `pauseMenu`.** It is a blocking loop that draws into the displayed buffer, not into the OT.
7. **"Where object positions reach the GTE".**
   - "+128 is rebuilt from +160" is wrong for car objects, which copy `body_m` with the body tilt, and for ricochet shots (yaw only).
   - Several drawn transforms are type-5 model nodes written directly: car body nodes, part spinners, Mr. Slam's arm, the Eiffel tower, splash nodes and collapsing ice.
8. **The effect-record paragraph.**
   - The "projectiles" with 60-byte records are the machine-gun bullets (`bulletRecords`).
   - Missiles are csList objects drawn by `renderScene` and keep no previous position; only `bulletsMove` saves one.
   - `fxImpactParticlesDraw` is called from `fxSpritesDraw`.
   - Debris chunks are objects; only the bursts are drawn by `debrisDraw`.
9. **#5 `debrisUpdate`.** In phase 2, `debrisUpdateBursts` adds the spin and rebuilds the matrix 4 times per step.
10. **The closing "Not yet checked" paragraph.** It can go:
    - the HUD, `clutAnimUpdate` and `weaponFlameUpdate` are now checked;
    - no arena event writes into the OT, and the Eiffel flash goes through `fxSheetStart` / `fxSheetDisplay`.

**docs/WIDESCREEN.md**

1. **Section 5, "800FADDC / 800FAEEC (dashboard icon helpers)", and the table row "800FADDC / 800FAEEC not checked".** These are not functions. They are loop branches inside `UAdashInitDashboardIcons` 800FAA0C (hud_icons.c), which is 425 words long and spans 800FAA0C..800FB0B0 in `ida_funcs.tsv`. It is load-time init (`world_load_object.c` case 1260). Nothing more needs checking there.
2. **The "Explosion red tint" row (`explodeMakeScreenRed` 8011335C).** The function has no caller in retail, so it needs no widescreen change.
3. **The full-screen flash (`fxSheetStart`).** No change is needed for widescreen. For 60 fps, the polys must be copied per view and per redraw, because `fxSheetDisplay` links one static prim chosen by `bulletsFrameParity`.

### Uncertain

- **The burst piece list (+0xE8).** That `lightAddSources` fills it during `renderDrawObjectUnculled` is inferred; the asm renderer was not traced.
- **Model nodes.** The Eiffel tower and car body nodes were not checked against the model format. Which car types have a `carBodyObjects` node is not known.
- **Missile spawners.** Whether every `weaponFire*` spawner goes through `fxRecordFindFree` (only the allocators and a sample were read).
- **Activation paths not read.** Mines, remote bombs, ricochet shots, strike and claw objects; rule 2 should cover them.
- **Possible unread readers.**
  - Game readers of object +0x14..+0x1C (`csUpdMat`) were not traced.
  - `weaponArcsActive` and `fxSpritesDrawParity` readers were searched only as gp-relative accesses.
- **Never-written flags.** `stats.restart_pending` and the AI re-place flag `stats.f03` have no writer found, so they are probably unused.
- **Asm draw code.** The asm parts (`renderWalkModel`, the model emitter 800CDC54, `renderNearPolys`) were judged from the reference C, not store by store.

## R4. Request 80: one-frame glitches on Amazonia, Holland and Antarctica (2026-10-29)

Port request 80 (the user's playtest: 60 FPS or uncapped, Hidden surfaces = Depth, Wobble none, draw distance
Farthest): short glitches, a frame or a few each. The question: where could the game's data cause them? The
answer is from the level files on the disc and the C. The scripts are in the session's scratch folder `r80/`, with
their output in `den2.txt`, `isl2.txt` and `gla2.txt`. Nothing was run in the game. Positions are level position
units; LOD distances are world units (position >> 3, the square root of the stored squared bands). Addresses are the
level file's at its load address 0x80019C40.

### R4.0 What items 1-3 share: coplanar layers kept apart only by the order table (CONFIRMED in the data)

These levels draw two meshes in one plane, overlapping, with different materials, in the same frame. Retail never
shows them fighting because the order table puts one behind the other:
- **a fixed OT z** in the mesh flags (+0x1A bits 3..14): Holland's ground is a flat-colour base at OT 4092 under a
  textured mesh at 4090 / 4091;
- **a per-primitive OT bias** (control word bits 7..12, signed): Amazonia's lava has +10 / +31 under stone at 0;
  Antarctica's ice has +20 / +30 / +31;
- **equal sort z** (bias 0 on both): insertion order decides.

**With a depth test both layers have the same depth, so the order is lost.** Which one wins at a pixel then depends
on rounding:
- the in-between frames' vertices;
- the near pass's split vertices: screen-space midpoints with no depth of their own, `MIDXY` in
  `reference/near_pass_engine_math_near_pass.c`;
- the range pass's k.

The winner changes from frame to frame, in patches shaped like the tiles. These are the coplanar pairs that can draw
in the same frame, found with sampled cameras:

| Level | Pairs of materials | Overlap (pos²) | With different sort hints |
|---|---|---|---|
| DEN | 1,018 | 1.58e9 | nearly all (the fixed OT differs) |
| ISLANDS | 578 | 2.0e7 | 1.2e7 |
| GLACIER | 2,196 | 1.28e8 | 2.2e7 (the rest bias 0 on both) |

**For the port:** draw such pairs in retail's order, not by depth. Options: offset the depth by the polygon's OT z
(the fixed OT or the bias), or draw the level's coplanar pairs with the depth test off. Not a CLUT animation
(CONFIRMED): `clutAnimStartCycle` stops at the first 0x0000 entry, and a type-3 cycle rotates entries 0..7 only
(`engine_clut_anim.c`). The lava's CLUT 1036 entry 15 (transparent) is neither used by the lava tile nor moved.

### R4.1 Amazonia (ISLANDS): red squares on the ground, black squares on the lava

- **The lava grid** (CONFIRMED): z 0, page 0x13, CLUT 1036, cycled by object 5000.
  - LOD `8007A8D4` (reference point (0, 3,200, 0)): `[2,200, 10,000)` draws `800796F8` (70 quads); `[0, 2,200)`
    draws 800 x 800 blocks.
  - Each block is LOD `800785D4`: `[200, 10,000)` draws `80078300`, 1 quad with bias +10; `[0, 200)` draws
    `8007835C`, 16 quads of 200 x 200 with bias +31, flagged for the near pass.
- **The stone floor in the same plane** (CONFIRMED): CLUT 1037 (beige), bias 0, about x -100..400, y 7,200..7,900.
  - Its LOD `80064298`: `[0, 300)` draws `8006343C` (near pass), `[300, 600)` draws `80062E60`, `[600, 10,000)`
    draws `80062B6C`.
  - About 136 polygons overlap the lava, about 1.0e6 pos².
- **The red squares on the ground** (LIKELY): the lava's 200 and 800 squares showing through the stone under the
  depth test (R4.0).
- **The black squares on the lava: no black in the data.**
  - The flame glows lie flat on the lava at z 0: anim `8002AA18` and the other flame groups, 187 x 188 additive
    quads, page 0x32, CLUTs 107C..10B2. They fight the lava under depth too, but they are additive, so they would
    flash red or orange, not black.
  - Their frame 14 (`8002A9C8`, CLUT 10B2, nearly black) would show as a dark blob for one tick only if drawn opaque
    or with the wrong blend mode (LIKELY the port's, if this is it).
  - The near-pass meshes (`8007835C`, `8006343C`) are where squares near the car would appear (their split
    vertices, R4.0).

### R4.2 Holland (DEN): black squares in the tall grass

- **The grass field** (CONFIRMED): patch `8002F7E0`, instanced at x 3,200 / 4,800 and y -1,600 / 0 / 1,600 by
  `8002F88C`. Patch LOD `8002F7C4`: `[300, 10,000)` draws one quad (`8002F280`); `[0, 300)` draws 4 blocks. Block
  LOD `8002F710`: `[150, ..)` draws one quad (`8002F2E0`); `[0, 150)` draws 16 cells of 200 x 200. A cell is the
  ground quad `8002F340` plus LOD `8002F50C` `[0, 140)`: 8 camera-facing beams (half width 50, half length 28,
  opaque FT4, page 0x92, CLUT 03B0, a texture window). 384 cells and 3,072 beams in all. The blade texture is
  transparent above the blades and opaque below; it has no black.
- **Two layers at z 0 under the grass** (CONFIRMED):
  - a flat-colour base (`80038620` / `8003866C` / `800386B8` / `80038704`): F4, rgb (48, 69, 9), fixed OT 4092. Its
    LODs `80038990` -> `800388A4` -> `800387F0` tile it differently (1,600 x 1,652), and its cells are flagged for
    the near pass;
  - the grass ground (`8002F280` / `8002F2E0` / `8002F340`): textured, fully opaque, fixed OT 4090; `8002F340` is
    flagged for the near pass.
  All of Holland's ground is built this way: flat bases (36, 52, 7) (nearly black on screen), (48, 69, 9) and
  (123, 87, 17) at OT 4092, under textures at 4090 / 4091.
- **Cause** (LIKELY): the depth test. The dark base wins in patches where the two tilings meet. Within about 70
  world units of the eye (OT < 70) the near pass also splits both layers in 4, which matches "while driving through
  it".
- **Not the beams:** they are opaque, sorted per beam, never deferred to the near pass, and their width is clamped
  to 1..2w pixels (the emitter).

### R4.3 Antarctica (GLACIER): thin black lines around the trench

- **The trench** (CONFIRMED): floor at about z 4,500 (the ice at 4,800), x -3,360..-1,440, y -3,324..6,648. Its
  segment LODs are each instanced 3 times:
  - `80028E9C`: `[800, 3,000)` draws `800279D4` (6 polygons); its near node `80028E54` has `[0, 200)` 144 polygons
    (`800281D4`), `[200, 400)` 72 (`80027C5C`) and `[400, ..)` 18 (`80027AA4`);
  - `8002A680`: `[800, 3,000)` draws `80028ED4` (6); its near node `8002A638` has `[0, 200)` 195 (`800297C0`),
    `[200, 400)` 99 (`80029174`) and `[400, ..)` 18 (`80028FA4`).
  The rim pieces use CLUT 0170, mostly with bias +31. No animation or select nodes are on these paths, and there is no
  near pass here.
- **The ice tiles' LOD versions overlap in thin strips** (data CONFIRMED, cause LIKELY): their outlines differ by
  about 15..50 position units (e.g. tile `80024AA0`: `[700, ..)` `80024598` against the inner LOD `800249E8`'s
  `80024730`, `80024658` and `800245F8`). Neighbouring tiles at different bands overlap in thin coplanar strips with
  different textures (pages 0x91 / 0x94 / 0x96; CLUT 0670, grey-blue), thousands of polygons at z 4,800. Under the
  depth test (R4.0) these flicker as lines along the tile borders.
- **T-junction cracks** (a guess, not measured): neighbouring segments at different bands (144 / 195 against 72 / 99
  polygons) split their edges differently, so the seams can open into cracks.
- `[800, 3,000)` is a finite band: retail draws nothing past 3,000; Farthest extends it.

### R4.4 Antarctica: a cloud around Mr. Grimm's car for one frame

- **The effect sprites** (CONFIRMED, `fxSpritesDraw` 80110BE4, `world_collapse.c`):
  - size: half size = w x scale x H / z >> 3, clamped to 1..512 pixels; dropped when the camera z is below 8 or above
    32,000, or about 100 pixels off screen;
  - blending: every one is semi-transparent (the frame's page gives the mode);
  - order: OT z >> 3; `depth` 1 puts it 8 slots nearer, 8 puts it at slot 8, over almost everything;
  - records with `follow` copy their position from the source at each draw.
- **No retail cause:** a sprite that large at that distance needs z near 8 for that frame.
- **The pictures:** a rectangle with stepped edges, lighter in 005456 and darker in 005507, drawn over part of his
  name label (so in a near slot, after the label). Light then dark fits an additive then a subtractive smoke or
  explosion frame (LIKELY his ricochet bomb). Which effect it is was not traced.
- **The port's suspects** (LIKELY):
  - interpolating a reused slot (150 slots; `fxSpriteSpawn` takes the first free one, so an in-between frame can
    blend from the slot's previous sprite);
  - a semi-transparent sprite written to the depth buffer, or drawn with the wrong mode, for one frame;
  - `frame / rate` read past `frames` in an in-between frame.

### R4.5 Holland's windmills out of bounds: bigger or moved in some frames

- **The windmills** (CONFIRMED): two, at (3,560, -3,825) and (3,520, 3,640), under select nodes `80047B38` /
  `800569D0`. Their blades are animations `80046724` / `80052C8C` (id 3600): 12 frames, a period of 2 ticks, looping,
  advanced while drawn. Each frame is a mesh swap under its own LOD: `[0, 300)` 32..44 polygons, `[300, 10,000)` 16.
  The frames' LOD reference points differ by 2.5 world units at most.
- **The near and far versions have the same extents**, so a band switch changes only the polygon count, never the
  size. Only the blades' turn changes the outline from frame to frame. Frame 7 of `80052C8C` has 32 + 22 polygons
  instead of 16 + 44. Past 10,000 world units nothing is drawn (Farthest extends that).
- **The data cannot make them grow.** The port's suspects (LIKELY):
  - the range pass (`portFarDistanceScale`): its k is chosen per frame with no hysteresis, and vertices rounded at a
    coarser k move by up to 2^k;
  - in-between frames across the blades' mesh swaps: the blades change mesh every 2 ticks, which is every 4 frames at
    60 FPS. If pairing old and new fails on some swaps (4 of the 12 change the polygon count), that could make "one
    frame in fifteen" (a guess).

### R4.6 Summary

| Item | The game data there | Can the data explain it? | The port's suspect |
|---|---|---|---|
| 1 Amazonia | lava (bias +10 / +31) under stone (bias 0), coplanar; additive flame glows on the lava | the red squares, under depth; no black data | the depth test on coplanar layers; the near pass's split depth; flame frame 14 only if drawn opaque |
| 2 Holland grass | dark flat base (OT 4092) under the textured ground (OT 4090), coplanar, both near-pass | yes, under depth | the depth test; the near pass's split depth |
| 3 Antarctica trench | LOD bands; ice tile versions overlapping in strips | yes, under depth; cracks a guess | the depth test; subpixel cracks |
| 4 Mr. Grimm's cloud | semi-transparent effect sprites, 1..512 pixels | no | slot interpolation; blending or depth on sprites |
| 5 windmills | 12-frame mesh-swap blades; LOD versions of equal size | no | the range pass's k; interpolation across mesh swaps |

## R5. Request 88 item 4 (2026-10-30): the drawing glitches still seen (uncapped, Wobble None, Depth)

Port request 88 item 4: the ground right at the bottom of the screen gone for a moment (Antarctica, New York, Roof
Tops; Paris: the church roof's floor see-through now and then), Holland's black squares in the tall grass (R4.2),
a patch of Amazonia's lava black for moments (beside the long stone path), and Axel's shadow partly covered by the
ground on Roof Tops. Asked: the near pass again, and whether any is the port's.

### R5.1 The near pass is not in any of them (CONFIRMED)

With Wobble None the port sets the emitter's near threshold (scratchpad +0x40) to 0 while it runs
(`src/vplus/precise.c`, `vp_ModelEmit`): no polygon is deferred, `renderNearPolys` gets an empty list and draws
nothing. So neither the near pass nor request 82's C form of it touches these frames. And the near pass would not
help: it works in screen space only (R1.1), it never clips against the eye.

### R5.2 The ground at the bottom of the screen: a corner behind the eye (CONFIRMED in code)

- The emitter (`reference/model_emitter_800CDC54.c`) takes a triangle's or quad's sort depth from its **deepest**
  corner and drops it only when that is 0 or past 4,095. A big ground polygon that runs under and behind the camera
  keeps its front corners' depth and is drawn.
- Its corner behind the eye (or within a unit of it) goes through the GTE as any other: SZ 0, and SX / SY saturated
  (the divide's overflow) and clamped to -1,024..1,023. The polygon's screen shape is then wrong: on the PS1 the GPU's
  size limit skips it (gone for the frame); a renderer without that limit draws a wrong shape. Retail has the same
  data and the same corners; its chase camera, high and behind, rarely has a ground polygon under it.
- **The port's:** only a clip against the near plane in 3D fixes it, and only the port has the camera-space corners
  (its GTE model). The decomp's `portPreciseProject` (beams and sprites) already returns 0 for a corner behind the
  eye; the polygons' precise path is the port's (`gte.c`). Things that put the eye low or into the ground make it
  more frequent: the camera wall (`portVisual` bit 5) pulls the eye in front of walls; wide fields of view.
- Paris's roof floor (request 75: the nave's ceiling turned face up at z 832) is 12 + 6 big polygons: with the eye on
  the roof they are the ones under the camera.

### R5.3 Holland's grass, Amazonia's lava: coplanar layers under the depth test (R4.0)

Unchanged from R4.1 and R4.2: the dark flat base (OT 4092) under the grass texture (OT 4090) in the same plane, and
the lava's tiles (bias +10 / +31) under the stone path (bias 0) in the same plane (LIKELY the spot beside the long stone
path: R4.1's stone floor, `80064298`'s records, x -100..400, y 7,200..7,900). With a depth test their order is lost. The data has no black on
the lava; the lava's black moments are the coplanar pairs (or an additive flame glow, frame 14, drawn opaque). The
port's: offset the depth by the polygon's OT z (its fixed OT or bias), or draw the level's coplanar pairs in OT order
with the depth test off.

### R5.4 Axel's shadow covered by the ground (LIKELY)

A car's shadow is a flat polygon at the ground's height under the car, sorted one OT entry behind the car
(`portVisual` bit 3, PLAYTEST_ANSWERS 12) and always over the ground in retail by the order table. Under a depth test
it is coplanar with the ground, or under it wherever the ground's triangles rise above the shadow's flat plane
(Roof Tops' roofs are tiled, with slopes and seams): those parts are covered. The port's: draw shadows with the depth
test off (as retail's order puts them) or with a bias toward the eye.

### R5.5 Summary

| Item | Cause | Whose |
|---|---|---|
| ground gone at the bottom (Antarctica, New York, Roof Tops; Paris's roof floor) | a ground polygon's corner behind the eye: saturated screen corner; no 3D clip anywhere | the port's (a near-plane clip in its GTE / renderer) |
| Holland's black squares | coplanar layers under depth (R4.2) | the port's depth rule |
| Amazonia's black lava patch | coplanar lava / stone under depth (R4.1) | the port's depth rule |
| Axel's shadow under the ground | shadow coplanar with / under the ground under depth | the port's (shadows without the depth test or with a bias) |

Nothing here is decomp code; nothing was built. The port's depth bias in `gpu_psyz.c` is not visible from here: if it
offsets by the OT slot, the coplanar items should go; the behind-the-eye corners need the clip.

## R6. Request 90 (2026-10-31): Holland's darker grass rectangles, Amazonia's lava slivers

Port request 90 (the user's playtest: uncapped, Wobble None, Hidden surfaces Depth; `docs/PLAYTEST_2026-10-31.md`
items 3 and 5). Two questions, answered from the level files on the disc. Nothing was built, and nothing was run in the
game. The scripts are in the session's scratch folder `r90/` (`tex.py`, `tex2.py`, `rows.py`, `lava.py`,
`lavacol.py`), using `r80/`'s walker. Units are as in R4: positions in level position units, LOD distances in world
units (position >> 3: the grass and the lava are under no scale node, so the shift is 3).

### R6.1 Holland: the three levels draw the same texture, but the far-LOD settings move the tall grass out

- **Material: the same at every level** (CONFIRMED). The patch quad `8002F280`, the block quad `8002F2E0` and the cell
  quad `8002F340` all use:
  - page 0x93 (8-bit), CLUT 02F0, and texture window `E208139C` (the 32 x 32 tile at u 32..63, v 128..159);
  - UVs 0..255, so 8 repeats of the tile across each quad's side;
  - code 0x2C with vertex colour 0x7F7F7F and colour mode 0 (copied, not lit), opaque, fixed OT 4090.

  The tile's mean is (51, 72, 23), with luminance 52..71 from the 10th to the 90th percentile. None of the three levels
  is lighter or darker in the data. Only the scale differs:

  | Level | Quad (world units) | One texture repeat |
  |---|---|---|
  | patch | 200 | 25 |
  | block | 100 | 12.5 |
  | cell | 25 | 3.1 |
- **The tall grass is the fourth level** (CONFIRMED). Each cell has LOD `8002F50C`, reference point (100, 75, 28), whose
  only band is `[0, 140)`. That band draws mesh `8002F394`:
  - 8 camera-facing beams, half width 50, half length 28 about z 28, so every blade's top is at z 56 (flat-topped);
  - page 0x92, CLUT 03B0, window `E200331C`;
  - its top 8 texel rows (of 62) are transparent (0x0000), the next 12 partly (the blade tips), the rest opaque, mean (52, 73, 23): the ground's
    colour again.
- **The distance is the camera's** (CONFIRMED, walker opcode 2): `SQR((ref + t) >> shift)`, with `t` the level root's
  translation, which `viewProc` sets from the view's eye. It is measured to each node's own reference point (the
  patch's, the block's or the cell's centre), not to the car. With the chase camera the eye is behind and above the
  car, so the bands are centred there.
- **What the port changes:**
  - **The range pass's k: nothing.** `portFarDistanceScale` runs after every walk, on the queued meshes, so the level
    is already chosen.
  - **The widescreen x squeeze: nothing.** The band test is a 3D distance; it does not use the screen.
  - **`portFarLodNearScale` and `TM2_PORT_FAR_LOD`: yes** (CONFIRMED, `portFarLodBegin`). At Farthest the port writes
    a scale of 4 (Far: 2; `fixes.c`, request 46 item 6), and `portFarLod` is on. Every band of the scan is put at 4
    times its distance first. Then each set's top record is extended to unbounded. `dmd_lod_dump.py --files DEN`
    marks the beams' only record `ext`: it is the top of its node. So:

    | | Retail | Farthest |
    |---|---|---|
    | patch -> 4 blocks | within 300 | within 1,200 |
    | block -> 16 cells | within 150 | within 600 |
    | a cell's 8 beams | within 140 of the cell | every cell drawn (`[0, 560)`, then unbounded) |
    | where the tall grass ends | about 140 from the eye, in 25-unit cell steps | at each block whose centre is 600 from the eye, in 100-unit block steps |
- **Would retail show them?** Not like this. Retail's tall grass also has an edge, but about 140 units from the eye,
  stepped by 25-unit cells. Block-shaped, flat-topped, staggered patches a few hundred units out are Farthest's: whole
  100 x 100 blocks of beams (128 per block), 7 units high, beside blocks of flat ground. The stagger is each block
  switching at its own distance. The port can see it move with `TM2PC_NEAR_SCALE=1` and `TM2PC_NO_FAR_LOD=1`: the edge
  should come back to about 140 and to cell steps.
- **Darker: not in the data** (the beams' mean colour is the ground's). The likely cause (LIKELY, port side, not
  checked) is the far beams' width. The emitter clamps a beam's half width to 1..2w pixels (R4.2), so far beams are
  1 to 2 pixels wide, and nearly every pixel of one is at a texel edge. Filtering that takes in the transparent
  texels' 0x0000 (black), or blends those edges, darkens a whole block of them. Seen from above at a distance, a block
  of beams also shows the beams' sides instead of the ground's face, with no lighting difference in the data.
- **If the user wants retail's grass edge with Farthest** (not built): the decomp can keep beam-only LOD records out
  of the near scale and the extension. They would keep their retail band, so the tall grass would end at 140 as in
  retail, while everything else keeps Farthest. The rule would be "every mesh the record reaches is beams (kind 1)",
  in `portFarLodNode` / `portFarLodNearNote`. Or the port can lower the near scale on DEN.

### R6.2 Amazonia: the lava's tiles meet at T-junctions, and nothing is under them

- **The main lava grid** (CONFIRMED, CLUT 1036, page 0x13, z 0, faces up; R4.1). Every level is one quad per tile, and
  each tile picks its level by its own distance:

  | Node | Tile | Bands (retail) | At Farthest (x 4) | OT bias |
  |---|---|---|---|---|
  | grid LOD `8007A8D4`, ref (0, 3,200) | whole grid: far mesh `800796F8` | `[2,200, ..)` | `[8,800, ..)` (never in the arena) | +10 / +31 |
  | group LOD `8007873C` | 3,200 square `80078248` | `[2,000, ..)`, else 4 subs | `[8,000, ..)` | +31 |
  | sub LOD `80078688` | 1,600 square `800782A4` | `[500, ..)`, else 4 blocks | `[2,000, ..)` | +31 |
  | block LOD `800785D4` | 800 square `80078300` | `[200, ..)`, else 16 cells | `[800, ..)` | +10 |
  | cells `8007835C` | 200 squares | `[0, 200)` | `[0, 800)` | +31 |
- **T-junctions: yes, at every band boundary** (CONFIRMED in the data). Two neighbouring tiles at different levels share
  an edge on which the finer one has vertices that the coarser one lacks:
  - a 1,600 quad's edge against two 800 blocks has 1 T-vertex, and against cells 7;
  - an 800 quad's edge against cells has 3.

  All T-vertices lie on the 200 grid, exactly on the long edge in world space. The far mesh `800796F8` has them inside
  itself: 12 quads of 3,200, 34 of 1,600 and 24 of 800, with 44 T-vertices. The other lava pools (`8003F8xx` ..
  `80044xxx`, bias 0) also put 800, 600 and 400 quads beside 200 ones. At Farthest the boundaries lie 800 and 2,000
  units out, so the T-junctions are in the far lava. Seen at a low angle, each band boundary is a row of tile edges
  across the screen.
- **Nothing under the lava** (CONFIRMED). At (-8,100, -8,100) every polygon from z -20,000 to +20,000 is one of the
  lava's own levels at z 0, or a sky triangle at z 14,320. A crack therefore shows the clear colour, the flat brown seen in
  HIDDEN_AREAS 9.2's frames (its cause there was corrected in 10, the colour was not). There is no dark base as there is on Holland.
- **Not the texture** (CONFIRMED). CLUT 1036 is reds, (123..205, 0..32, 0). The 64 x 64 box (u, v 0x80..0xBF) uses
  entries 0..14; entry 15, black, is not used. Its mean is (162..170, 11..14, 0) through the 8 steps of the cycle. The
  texels around the box are lava too, so filtering past its edge can't darken it.
- **Why retail barely shows them, and the port can:** retail rounds every corner to a whole PS1 pixel, so a T-vertex is
  at most half a pixel off the long edge: a hairline at 320 x 240, near the eye (retail's boundaries are at 200 and
  500). In the port a crack's width depends on how its corners are computed (LIKELY, not measured):
  - **The range pass:** a far mesh is projected at 2^-k (k up to 3). Each vertex's camera-space x, y, z then comes out
    in steps of 2^k position units, and each mesh has its own rounded matrix, which `portFarTranslate` corrects only at
    the mesh's centre. The coarse tile's edge and the fine tile's T-vertex are in different meshes, so they move
    independently by a few units: a thin wedge, widest at the T-vertex.
  - In-between frames, if those place each mesh's corners separately.

  Both apply only to far meshes, which is where the slivers were seen.
- **Closing them, for the port:**
  - **(a) A base** (simplest): one quad, or a few, in the lava's colour (165, 12, 0), at z just under 0 (e.g. -8
    position units, so the depth test keeps it under the lava), drawn first (OT 4095). It should cover the far
    mesh's extent, x -9,600..9,600 and y -9,600..16,000, plus the other pools if those show cracks. A crack then shows
    lava, as Holland's base does for its ground.
  - **(b) Snapping:** put each T-vertex on its neighbour's edge after projection. The T-vertices are known (the 200
    grid on a coarser neighbour's edge), but this needs the neighbours' levels in the same frame.
  - Lowering the near scale on ISLANDS only moves the boundaries back in.

### R6.3 Summary

| Item | The data | Would retail show it? | The port's |
|---|---|---|---|
| Holland's darker rectangles | three ground levels with the same texture, CLUT and colour, at 2x / 4x scale steps; beams on cells within 140 | the tall grass's edge at about 140 in cell steps; not block rectangles a few hundred out | Farthest's near scale x 4 plus the extension of the beams' only band: beams on every block within 600; darker: LIKELY the far beams' filtering |
| Amazonia's lava slivers | one quad per tile at every level; T-junctions at every band boundary and 44 inside the far mesh; nothing under the lava (the clear colour) | the same T-junctions, at most half a PS1 pixel wide, near the eye | the range pass's per-mesh rounding at far distance; fix with a lava-coloured base under z 0 (a), or snapping (b) |

### R6.4 Built after the answer: the tall grass kept at retail's distance

The user asked for the rectangles to go, so R6.1's offer is built: `FAR_LOD.md` 12. With `TM2_PORT_FAR_LOD`,
Holland's beam LOD `8002F50C` keeps its retail `[0, 140)` at every draw distance, and nothing else on the disc
changes. Amazonia's lava (R6.2) is unchanged: the base or the snapping is the port's.

## R7. Request 92: Mr. Slam's shadow growing and shrinking (Hong Kong), and the steady shadow (portVisual bit 7)

### R7.1 How a car's shadow is made (CONFIRMED in the code)

- **The shadow is a mesh in the car file:** the car's "body object" (`type + 3`, registered by
  `carBodyObjectRegister` 80114EEC). It is a flat, semi-transparent blob of 6-10 quads at z 0, each with an OT bias
  of -2..-7 (Mr. Slam: -5 / -7). It is drawn as a world model in `renderScene`'s world-model pass.
- **No size table, no scaling.** Its size never changes. Each step `carBodyPoseUpdate` 80114F24 (`car_body_pose.c`,
  called from `car_tick.c` and `ai_driver.c`) only places and tilts it:
  - **x, y:** the car's centre (`motion.pos`).
  - **z:** the average of the four wheels' ground heights (`wheel[].h`, the collision surface under each wheel
    from `carWheelOverSurface`), + 3 for types 0x40, 0x60, 0xA0, 0xB0 and 0xC0 (Mr. Slam is one).
  - **tilt:** from two wheel-to-wheel vectors turned by the car's orientation `motion.rot_m`. Their z is the
    difference of each wheel's height **over the ground** (`pos[2] - h`): `pos[2]` holds the suspension (`sway`
    from `wheelSwayUpdate`, the landing bounce `f2C`, `crush`).
- **So on a flat road the tilt is the suspension's play between the wheels.** It changes every step while driving,
  and the blob rocks about its centre only 3 units over the road. Its ends sink into the road and come out.
  Mr. Slam's long wheelbase (front axle 61, rear -79) makes a small pitch move his rear end most: the dark patch
  under his rear grows and shrinks.
- **The cause is the drawing order.** How much of the sunk end shows depends on the order the shadow and the road
  are drawn in.
  - With retail's -5 / -7 bias the shadow normally wins, so retail shows it less.
  - With `portVisual` bit 3 (request 46 item 5: shadows sorted behind their car) the shadow sorts one entry behind
    the car's farthest primitive. Mr. Slam's arm reaches far, so near road polygons can cover his rear's shadow.
  - LIKELY both play a part. Compare with bit 3 off as well as with retail's order.
- **The shadow draws no `rand()`, has no timer and no frame-parity logic**, and nothing in gameplay reads its
  matrix (only the show and hide in `ai_route_helpers.c`).

### R7.2 The port's parts

| | Changes the shadow? |
|---|---|
| far LOD (`TM2_PORT_FAR_LOD`) | no: the ground under the wheels is the collision data (`worldSurfaces`), not the model's LOD records |
| 16 cars / car groups | no: one body object per car key (`portCarGroups.body`), none shared |
| `portVisual` bit 3 | the order only (R7.1) |
| the port's interpolated frames | LIKELY, on the port's side: the shadow's matrix is written into the body object (a model node) once a step. If the port blends the car's pose between steps but not the body object's matrix, the shadow moves in 20 Hz jumps under a smoothly moving car. Blend it too: its translation is the car's x, y; its z and tilt as below |

### R7.3 Built: `portVisual` bit 7 `TM2_VIS_STEADY_SHADOW` (0x80, in `TM2_VIS_ALL`, now 0xFF)

- **Tilt from the car only:** with the bit, every car's shadow is tilted by the car's own orientation
  (`motion.rot_m`) only, without the wheels' suspension (the two vectors' z set to 0). So it follows the road's
  slope (ramps, banks) and no longer rocks.
- **Place, size and height are retail's:** the car's x, y, and the ground under the wheels + 3. The data has no
  size to steady.
- **Drawing only.** The body objects are model nodes, and the rollback hash covers them. So the step still writes
  retail's pose, and `renderScene` re-poses the shadows only for its walks:
  - `portVisualSteadyBegin`, after the far-LOD and hidden-area patches, saves each body object's matrix in
    `portSteadyShadow` (`.port_bss`, `rollback_regions.tsv` snapshot and hash no) and writes the steady pose.
  - `portVisualSteadyEnd`, after the walks, puts every matrix back.
  - Read live per view, like every `portVisual` bit; each machine may set its own. No `rand()`.
- **Checked:** `check_custom.py` check 15 runs Begin and End in LinkMips on a made-up tilted Mr. Slam with uneven
  suspension (links without car groups).
  - Begin gives the pose the game's own `carBodyPoseUpdate` gives the same car with even suspension, and keeps the
    translation.
  - End gives back the step's matrix byte for byte.
  - With the bit clear, nothing changes.
  - Check 2 lists the two functions (called only by `renderScene`). Not tested in game.

## R8. Request 93: the temple wall's sliding halves (Hong Kong), and every polygon whole (portVisual bit 8)

### R8.1 What the near pass does (CONFIRMED in the code)

- **Who defers:** the model emitter (`sub_800CDC54`; C form `reference/model_emitter_800CDC54.c`) sends a triangle,
  quad, beam or sprite to the near list instead of the order table when all three hold:
  - its OT index is below the near threshold (scratchpad `+0x40`);
  - its mesh has flag bit 2 (`+0x1A`, near-deferrable);
  - the list holds fewer than 200.
- **The threshold** is set before each emitter call:
  - `renderScene` 80152284: 70, or 20 in split screen (`viewGetRenderSplitMode`);
  - `renderDrawModel` 80151378 (menus, car select, the ending, `sceneObjDraw`): 70.
- **The cut:** `renderNearPolys` 80154690 (SingleTrac's hand-written asm; C form
  `reference/near_pass_engine_math_near_pass.c`) cuts each listed polygon **once** into four:
  - in screen space, at the edge midpoints (a quad also at its centre);
  - with the corners' UVs and colours averaged (`renderNearSplitTri` 80154E30, `renderNearSplitQuad` 80155088);
  - the four pieces linked at the entry's sort z (`renderNearLinkPrims` 80154E84), the OT index the polygon would have
    had whole.
- **It does not clip or re-project.** Nothing is cut in 3D or against the eye, and a corner behind the eye is
  projected as the GTE gives it, cut or not. On the PS1 its use is the affine texture error, smaller in four pieces,
  and LIKELY the GPU's size limit: a primitive wider than 1023 or taller than 511 pixels is not drawn, and halving
  the edges keeps near polygons under it.
- **Correction (the port, after PR #117):** the C emitter sits at `800CDC54` in the variant, so the port's hook ran in
  matches and the near pass already deferred nothing there. The temple wall's halves came from the port's
  flipped-texture fix (its PSY-Z patch 23). Bit 8 stays: the decomp's own switch, without that hook.
- **So at Wobble None** the averaged UVs are retail's affine error fixed into the pieces (request 93's finding). The
  true UV at a screen midpoint is the perspective one, and it moves away from the average as the camera moves, so the
  pieces' textures slide against each other. Retail shows it as well, under its own affine wobble.

### R8.2 Built: `portVisual` bit 8 `TM2_VIS_WHOLE_NEAR` (0x100, in `TM2_VIS_ALL`, now 0x1FF)

- **No polygon is cut:** with the bit, `renderScene` and `renderDrawModel` set the threshold to 0 through
  `portVisualNearOtz` (`.port_text`). Every drawn polygon has an OT index of at least 1, so the emitter defers none,
  and `renderNearPolys` gets an empty list.
- **Every polygon goes to the order table whole,** in the entry its four pieces had: the emitter's own index, with
  the mesh's fixed index or the entry's override as before, and the mirror's draw modes from the emitter's own path.
  - Only the order inside one OT entry changes: the pieces were linked after the emitter's other polygons (drawn
    before them in that entry); the whole polygon is linked in the emitter's order. With a depth buffer it does not
    matter.
- **Works with both emitters** (the asm and the C one): it is the scratchpad word they both read. So the port's
  `vplus/precise.c` hook on the recompiled `800CDC54` is no longer needed for it.
- **The precise ring:** with nothing deferred, `portNearDepth` and the near pass's split vertices (request 82) stay
  empty. The polygons' corners are the emitter's, as for every far polygon.
- **Which polygons still need cutting: none.** The near pass never clipped anything, so there is no case where its
  cut fixed a polygon. A polygon with a corner behind the eye is as wrong whole as cut. So no perspective-correct
  midpoints were built.
- **The port's side:**
  - Let the graphics card clip (the depth buffer and the precise depths).
  - Do not apply the PS1's 1023 x 511 drop to whole near polygons: up close, a whole wall polygon can be wider than
    the screen.
- Read live per view, like every `portVisual` bit; each machine may set its own. Drawing only, no `rand()`, no
  rollback state.
- **Checked:** `check_custom.py` check 16.
  - `portVisualNearOtz` in LinkMips: 0 with bit 8, else 70 or 20.
  - The link's emitter on three made-up views with near polygons (PARIS and made-up meshes), at threshold 70 and 0.
    At 0 the near list is empty, and every polygon the 70 run deferred is linked whole (its packet's words) at its
    near entry's OT index.
  - The asm at 0 writes the same bytes as the link's emitter at 0.
  - Check 2 lists `portVisualNearOtz` (called only by `renderScene` and `renderDrawModel`), and `renderDrawModel`
    among the functions `TM2_PORT_VISUAL` changes.
  - Not tested in game.

## R9. Request 99: the rear-view mirror looks from too high (`portVisual` bit 9, built)

### R9.1 Retail's mirror (CONFIRMED in the code)

- **Where it exists:** single player only. `arenaSetup` makes view 1 in mode 4.
  - Car views 2 and 3 turn it on (`rtSetRearView(1)`); car view 4 turns it off (`carSetViewMode` 800E7380).
- **How it is made:** `viewProc` 80155E80, mode 4, copies view 0's camera object and its parent, then turns it:
  `rot.vz += 0x800` (half a turn) and `rot.vx -= 56`.
- **So the eye is the chase camera's eye** (`viewChaseCamera` 80102CC4, eased):
  - car view 2: 280 behind and 112 above the car's origin (`cam_chase`);
  - car view 3: 480 behind and 168 above.
  - With a typical roof 32 over the origin, the eye is about 80 over the roof, and close cars pass under it.
- **There is no bumper or in-car view to copy.** The cars' stats have `roof_height` (+0x46: 32 for most; Sweet Tooth
  44, Axel 60, Minion 66, Mr. Slam 75, Dark Tooth 106). Retail's watch view 7 uses it.

### R9.2 Built: bit 9 `TM2_VIS_MIRROR_CAR` (0x200; `TM2_VIS_ALL` now 0x3FF)

- **`viewProc`, after retail's copy and turn:** `portVisualMirrorEye(1, pos, rot)` puts the mirror's eye on the car:
  - position: `motion.pos` + `motion.rot_m`ᵀ (0, `TM2_VIS_MIRROR_BACK` -16, `roof_height` + `TM2_VIS_MIRROR_UP` 8),
    so on the roof, a little behind the centre, tilted with the car;
  - rotation: pitch `TM2_VIS_MIRROR_PITCH` 0 (level), roll 0, yaw `motion.rot[2]` + 0x800 (looking back).
- **The car:** the view's local human, or view 0's for retail's mirror (view 1). Only in car views 2 and 3: view 4
  and the watch view 7 keep retail's.
- **`renderScene`:** `portVisualMirrorSkip(view)` leaves that car out of a mode-4 view's object loop, because the eye
  sits on it.
- **Drawing only:**
  - The camera objects are snapshot yes, hash no (`rollback_regions.tsv`).
  - The human's `cam_pos` / `cam_rot` (step state the chase camera eases from) are not written.
  - No `rand()`.
- **The port's split-screen mirrors** (`vplus/mirrors.c`):
  - They set `viewModes[p] = 13`, so `viewProc`'s mode-4 branch, and this hook, never runs for them.
  - In place of the +0x800 / -56 turn, call `portVisualMirrorEye(p, cam + 0xA8, (short *)(cam + 0xA0))` (it uses
    view p's local human). Leave the parent (`cam + 0x10`) alone: its pose is zero in car views.
  - Save and restore +0x40..+0xB3, not only up to +0xA7, so the position is put back too.
  - `renderScene` already leaves the car out of those views: `viewGetMode` answers 4 for them.
- **Checked:** `check_custom.py` check 17. A LinkMips run on a made-up tilted, turned car: the eye on its roof
  looking back, within 1 unit, and Skip names its object. Nothing with the bit clear, in car view 4 or in the watch
  view 7. Check 2 lists the functions and their callers (`viewProc`, `renderScene`). Not tested in game.

## R10. Request 102: the mirror's red cracks, the shadow under the roof tiles

### R10.1 The red cracks in Amazonia's tunnel, in the mirror only (no decomp change: set bit 8)

- **What the mirror draws (CONFIRMED in the code):**
  - The mirror is view 1 in mode 4, drawn by the same `renderScene` as view 0. Its eye comes from `viewProc` (bit 9:
    `portVisualMirrorEye`); its car is left out.
  - Its projection: H 91 (the main view 228), a 101 x 30 box, x flipped, back faces flipped.
  - Its near-pass threshold (70, or 0 with bit 8), its emitter and its near pass are view 0's.
- **Not the far LOD bands (CONFIRMED):**
  - LOD distances are measured from each view's own eye.
  - The budget (request 98) keeps a row of its own for the mirror, never below `portFarLodNearScale`; it does not
    take view 0's last frame.
  - The tunnel's data (ISLANDS.DMD): section LOD points at y -2000, 0, 2000 and the ramps at ±3600, fine within 300.
    A coarse piece's T-junction seam is at least 2,400 x scale - 1,200 from the eye: 3,600 at Far, 8,400 at Farthest.
    At Farthest every tunnel piece is fine from anywhere in the tunnel.
  - The far range pass (FAR_DISTANCE.md 2) scales only meshes over 32,767; no tunnel mesh is.
  - So no LOD seam lies a few hundred units behind the car, in either view.
- **The near pass (mechanism CONFIRMED, the cause LIKELY):**
  - The emitter hands a polygon to the near pass when its farthest corner is nearer than about 560 (SZ >> 3 < 70)
    and its mesh has flag bit 2. The fine tunnel meshes have it.
  - The near pass's cut pieces get whole-pixel corners (PLAYTEST_ANSWERS 27); their uncut neighbours get the precise
    sub-pixel corners. Where the two meet, the shared edge is off by up to half a pixel: the clear colour, red, shows.
  - That boundary is the tunnel's floor and wall cross seam (every 200) lying 360..560 from the eye: "a few hundred
    units behind the car".
  - In view 0 the same band is 80..280 ahead of the car, behind the car's body and at the screen's bottom edge. In
    the mirror the car is left out and the band is in plain sight.
- **The fix: `portVisual` bit 8 `TM2_VIS_WHOLE_NEAR` (R8.2) for the mirror too.**
  - The port sets bit 8 only at Wobble None (`fixes.c`). With the precise emitter or the depth test on, set it at
    every Wobble setting, or at least around the mirror's `renderScene` call (four.c): bit 8 is read per
    `renderScene` call, so a per-view setting works.
  - Or, port side: build the cut pieces' corners from the precise corner records.
- **To tell (if the cracks stay):** the user's Wobble setting. At None (bit 8 already set), test
  `TM2PC_NEAR_SCALE=4` and `TM2PC_FAR_BUDGET=0`: unchanged cracks rule LOD out for good.
- **A retail seam, every view:** at the tunnel-ramp joins (y ±2800) the fine tunnel's wall vertex is at z -512 and
  the fine ramp's at -513: a gap up to 1 unit wide in both side walls. If it shows: weld it port side, or a
  lava-coloured backing under the tunnel (R6.2 option a).

### R10.2 The shadow under New York's roof tiles (`portVisual` bit 10, built)

- **The port's guess is right (CONFIRMED in the code),** with one correction: a polygon's OT entry is its
  **farthest** corner's, not an average.
  - The emitter keys each triangle or quad by its deepest corner (`model_emitter_800CDC54.c`), compressed past
    1,024, its bias added below 1,024. The near pass links its pieces at the parent's entry; the far range pass
    keeps the entry in world units.
  - Bit 3 (request 46) puts every shadow entry at max(its car's farthest + 1, the shadow's own farthest). Retail puts
    it at its own farthest minus 2..7.
  - The port's depth test: see-through polygons are tested but write no depth. The opaque road writes depth, so a
    road tile drawn after the shadow paints over it. Draw order still decides.
- **The symptom:**
  - A floor tile under the shadow is drawn after it once the tile's farthest corner is nearer than the car's
    farthest primitive + 1: once the car's front has passed the tile's far edge.
  - While the car is on the tile, the tile reaches past its front and is drawn first: the shadow shows. As the tile
    falls behind, the shadow goes under it.
  - Big tiles keep the shadow longer, small ones lose it at once: "under some textures, but not others". ROOF.DMD's
    flat floor quads: about 192 across within 250, about 832 within 1,000, and 1,600 and 3,200 at any distance (the
    car is about 150 long).
  - Retail has the same rule with its 2..7-entry margin; bit 3 took the margin away.
- **Built: bit 10 `TM2_VIS_SHADOW_GROUND` (0x400; `TM2_VIS_ALL` now 0x7FF):**
  - `portVisualShadows`, before the emitter: each shadow draw entry gets the OT entry `TM2_VIS_SHADOW_LIFT` (2)
    nearer than the shadow's own nearest corner (the emitter's mapping of the shallowest SZ, no bias; at least 1),
    through bit 3's override pair. Its car walked or not; over bit 3.
  - Why it holds: a ground polygon under any point of the shadow has a corner at least as far as that point, so its
    entry is at least the shadow's nearest corner's: it is linked farther and drawn first, on every level.
  - The 2 entries cover the shadow's lift and the in-between frames.
- **For the port's depth test only** (as bits 6 and 8):
  - With the depth test, the car's body (opaque, nearer) still hides the shadow wherever it is in front, drawn
    before or after it. Request 46's Sweet Tooth problem stays fixed.
  - Without a depth test, the shadow would be drawn over its car's low parts and anything nearer: no one OT entry
    is both after every tile under the shadow and before every car polygon.
- **The port's pull** (`gpu.c`): a see-through polygon whose entry is nearer than its corners is pulled toward the
  eye by that much, up to 8 entries (retail's -2..-7 shadows). Under bit 10 a shadow's entry is nearer than its
  corners by the shadow's whole depth, so it gets the full 8. Pulled more than a unit or two, it can come in front
  of a bumper or skirt near the ground. Recommended: with bit 10 set, give a see-through polygon whose entry is
  nearer than its corners the fixed 5-entry pull, as bit 3's shadows get now.
- **Other see-through polygons** (smoke, glass, other shadows) nearer than the new entry are drawn before the shadow,
  so their blends come in the other order. LIKELY invisible: the blends nearly commute.
- Drawing only, no `rand()`; one more pass over each shadow's vertices per view.
- **Checked:** `check_custom.py` check 19. A LinkMips run of `portVisualShadows` on made-up shadow and car meshes:
  bit 10's entry is the nearest corner's minus 2 (at least 1; behind the eye, shifted and translated meshes), with
  or without the car, over bit 3; bit 3 alone keeps request 46's; neither writes nothing; the ranges are cleared
  every time. Not tested in game. To test: New York's brown roof, the shadow should stay over every tile.
