# Draw distance: object culling and far clip

Answers the "object culling and far clip" item of the PC port's
`docs/DECOMP_REQUESTS.md` (section 2). Findings come from the Hex-Rays
export, the disassembly and the decompiled C; each is tagged CONFIRMED or
GUESS. Independently re-checked: the far value 4800 at `D_80180DAC` (renderFarDistanceDefault) (file
offset 0xB3958), the unused setter at `8015165C` writing both `D_80180DAC` (renderFarDistanceDefault)
and `D_80180DA8` (renderFarDistance), and the node test at `801528C0` (far from scratchpad
+0x10, shifted by the level's coordinate shift).

Retail image: load address 0x800CDC54, file offset = VA - 0x800CDC54 + 0x800.
`$gp` = 0x801809D4 (from tools/build_maximal_hybrid.py), so `N(gp)` = 0x801809D4 + N.
Scratchpad 0x1F800000 is the render context ("ctx"); `*(gp+1016)` = 0x1F800000.

Tags: **CONFIRMED** = seen in code and understood; **GUESS** = inferred.

---

## 0. Summary

| # | What | Where | Constant | Lives in |
|---|------|-------|----------|----------|
| 1 | Far-distance cull of model nodes (world/arena model, 20 world-space models, camera-attached model) | handler opcodes 0 and 1 of the model walker `sub_801526CC` (renderWalkModel) (tests at 801528C0..801528DC and 801529C8..801529E4) | **4800** world units (`0x12C0`) | data: `D_80180DAC` (renderFarDistanceDefault) (gp+984) default, copied to `D_80180DA8` (renderFarDistance) (gp+980) and ctx+0x10 each pass |
| 2 | Rear-view-mirror far distance (view mode 4) | `sub_80152284` (renderScene) 801522CC / 80152450 | **500** (world) / **1000** (objects, unused) | immediates `li v0,0x1F4` / `li v0,0x3E8` |
| 3 | Object list (cars, pickups, props): coarse 5-plane sphere test, **no far test** | `sub_801491E8` (csUpdMat) → `sub_80149344` (csSphereVisible) | none (radius from model root node) | – |
| 4 | Per-node culling disabled for the object list | `sub_80152284` (renderScene) 80152468..80152484 | `gp+1012` is always 0, so ctx+0x28 = 1 | data / dead setter 801516C0 |
| 5 | LOD selection by squared distance | model opcode 2, 80152A14 | per-LOD `[min,max)` squared distances | **model data** (DMD) |
| 6 | Camera-in-region switch (zone visibility) | model opcode 11, 80153048 | region planes/offsets | **model data** |
| 7 | OTZ compression, near drop, OT range | `sub_800CDC54` (primitive emitter) | 1024/4096/16384 knees, `otz-1 in (0,4095)` | immediates |
| 8 | OT size | main loop `sub_800D1318` | 4096 entries (ClearOTagR(ot,4096), DrawOTag(ot+16380)) | immediates + BSS |
| 9 | Capacity caps that drop distant objects silently | `sub_801526CC` (renderWalkModel), `sub_80152284` (renderScene), `sub_800CDC54` | 280 meshes, 198/200 matrices, 0x17CF0-byte packet buffer | immediates |
| 10 | Particles/tracers: camera z < 32001 | `sub_80110BE4` (fxSpritesDraw), `sub_80112F6C` (fxImpactParticlesDraw), `sub_800D8230` (streaksDraw) | 32001, OT index z>>3 | immediates |
| 11 | GTE limits (SZ 0..65535, IR ±32767) | hardware | effective far ≈ 65535 >> shift | – |
| 12 | Fog / depth cue | not used | DQA/DQB only set in InitGeom; SetDQA/SetDQB (8014A280/8014A28C) and SetFarColor have no callers | – |

The **single most important knob is `D_80180DAC` (renderFarDistanceDefault) = 4800** (file offset 0xB3958).

---

## 1. Model draw pipeline (CONFIRMED)

Per view, `sub_800D1318` (main game loop) calls `viewProc(i)` 80155E80, then the effect drawers,
then **`sub_80152284(ot_record, view)` (renderScene)**, which:

1. `gp+980 = (viewMode(view) == 4) ? 500 : gp+984;` then `sub_801521D0()` (renderInitState) fills ctx (ctx+0x10 = gp+980 = far,
   ctx+0x24 = gp+1008 = LOD override, ctx+0x28 = gp+1004 = cull-disable, ctx+0x38 = gp+992 = world shift, …).
2. **Group 1** (culling ON, far = 500/4800): the root camera object's model (`*(sub_801495F8()+284)` (csGetRoot),
   which is the **level/arena model**: DMD model id 1 is attached to it in `sub_800D1DB0` (worldLoadObject)), and the up to 20
   models in `dword_801A4E1C[20]` (added by `sub_801512F8` (renderAddWorldModel), removed by `sub_80151338` (renderRemoveWorldModel)).
3. **Group 2** (culling ON, far = gp+984): the camera-follow object `sub_801563A0(view)` (viewGetFollowObj) (DMD model id 2,
   registered by `sub_80156344` (viewSetFollowObj); GUESS: sky/background shell), via `sub_801491E8(obj, slot, 1)` (csUpdMat).
4. **Group 3** (object list from `sub_80149438` (csListHead), linked through +288): `gp+980 = mode4 ? 1000 : gp+984`,
   `if (gp+1012 == 0) gp+1004 = 1; ctx+0x28 = gp+1004;` — i.e. **per-node culling disabled** (see §4).
   Each object: `sub_801491E8(obj, slot, gp+1012)` (csUpdMat) sets `obj+292` (visible) via `sub_80149344` (csSphereVisible); if visible,
   `sub_801526CC(obj, model)` (renderWalkModel).
5. `sub_800CDC54(ot_record, dword_801AC810, count=gp+1840, matrices…)` emits the primitives, then
   `sub_80154690` (renderNearPolys) (deferred/near list), `sub_80151700` (animUpdateAll) (texture animators).

`sub_801526CC` (renderWalkModel) (801526CC, 816 words, hand-written asm) is a model-hierarchy interpreter: opcode byte at
node+0, jump table at ctx+0x2C filled by `sub_80153460` (renderInitNodeHandlers) (13 handlers). Relevant opcodes:

| op | handler | meaning |
|----|---------|---------|
| 0 | 8015282C | mesh leaf: sphere cull (centre node+28..+36, radius node+40, flag node+24 & 0x2000 = never cull), then append a 32-byte draw entry to `dword_801AC810` |
| 1 | 80152928 | group: sphere cull (centre node+4..+12, radius node+16; node+21 bit1 = hidden, bit2 = never cull), recurse into node+20 children |
| 2 | 80152A14 | LOD by squared distance (§5) |
| 3 | 80152FAC | callback `sub_801518E8` (animUpdateOne) then child select by node+12 |
| 4 | 80152ABC | translate |
| 5 | 80152B18 | rotate+translate (allocates a matrix slot, cap 198) |
| 9 | 80152F84 | switch by node+7 |
| 11 | 80153048 | camera-in-region switch (§6) |
| 12 | 801532D8 | change coordinate shift (t6) to node+4, rescale translation |

## 2. Far-distance test (CONFIRMED)

In handlers 0 and 1 (identical code; opcode 0 at 80152840..801528E0, opcode 1 at 80152948..801529E8):

```
if (ctx->cullOff /*ctx+0x28*/) visible = 1; else {
  c = node.centre + t(current translation, camera-relative, coord units)
  r = node.radius
  d = dot(columns of plane matrix A, c)    // sub_8015338C (renderMulTransMatVec), A = obj+64 (view m40)
  if (d0 < -r || d1 < -r || d2 < -r) cull;  // left, right, near
  d = dot(columns of plane matrix B, c)    // B = obj+96 (view m60)
  if (d0 < -r || d1 < -r) cull;             // top, bottom
  if (d2 < -r - (ctx->far /*ctx+0x10*/ << t6)) cull;   // FAR: b[2] = (0,-4095,0) so d2 = -depth
}
```

So a node is dropped when **depth > far·2^shift + radius** (coord units) i.e. **depth_world > far + radius_world**.
The plane normals come from the view-mode table 8017EC44: `a[0..2]` (+0x30) → m40, `b[0..2]` (+0x48) → m60,
rotated into world space by `viewProc`. For all 8 modes a[2] = (0,4095,0) (near) and b[2] = (0,-4095,0) (far
direction); side planes match the FOV (e.g. mode 0: ±(3547,2048) = 60° horizontal).

* `t6` = coordinate shift (fraction bits). Default = `gp+992` = `D_80180DB4` (renderWorldShift) = **3**, set per level by
  `sub_8015166C(DMD header word)` (renderSetWorldShift) (called from `sub_800D1C70` (worldLoadStartObjects) with `dword_80019C50`, and from `sub_80120EF4` (assetInstallImage)).
  (IDA names 8015166C `uaSetDifficulty` (renderSetWorldShift); that name is wrong — it sets the world shift.) Opcode 12 can change it per subtree.
* far value: `D_80180DAC` (renderFarDistanceDefault) (gp+984) initialised data **0x12C0 = 4800**, file offset **0xB3958**. `D_80180DA8` (renderFarDistance)
  (gp+980, also 4800 in the file, offset 0xB3954) is the per-pass working copy. A setter at **8015165C**
  (`sw a0,984(gp); sw a0,980(gp); jr ra`, not in the IDA function list) has **no callers** — the value is never changed at run time.
* Mirror (view mode 4: 101×30 window at 211,20, projection −4096): world/attached models use **500**
  (immediate `li v0,0x1F4` at **801522CC**, file 0x84E78); the object pass uses **1000** (`li v0,0x3E8` at
  **80152450**, file 0x84FFC), but that value is never read because object node culling is off.

**Port change:** raise `D_80180DAC` (renderFarDistanceDefault) (and `D_80180DA8` (renderFarDistance)), or patch the test at 801528C0–801528DC / 801529C8–801529E4
(e.g. treat ctx+0x10 as "infinite"). Optionally scale the mirror's 500. Side effects: more meshes → caps of §9,
GTE depth range of §11, OTZ resolution of §7.

## 3. Object list: coarse test only, no far (CONFIRMED)

`sub_801491E8(obj, matSlot, force)` (csUpdMat) (801491E8): `obj+292 = sub_80149344(obj, worldPos - cam)` (csSphereVisible); `force != 0` forces visible.
`sub_80149344` (csSphereVisible) (80149344): only if the model root is opcode 1 and its flag bit 4 is clear; tests the sphere (root radius
node+16) against the 3 planes of view m40 and the first 2 planes of view m60 — **the far plane is not tested**.
Cars, weapons, pickups, props in the object list are therefore never distance-culled; only off-screen culled.
No change needed for draw distance (but see §9 caps).

## 4. Per-node culling switch (CONFIRMED)

`gp+1012` (`D_80180DC8` (renderNodeCullEnable)) is zeroed by `sub_8015155C` (renderInit) and only set to 1 by **801516C0** (`li v0,1; sw v0,1012(gp)`),
which has no callers. Hence in group 3 `gp+1004 = 1 → ctx+0x28 = 1` and opcodes 0/1 skip all tests (also in
`sub_80151378` (renderDrawModel), menu/ending model draw). In groups 1–2 ctx+0x28 = 0, so the level model **is** culled.
`sub_801514B8` (renderDrawObjectUnculled) (preview draw) sets gp+1004 = gp+1008 = 1 (no culling, forced nearest LOD).

## 5. LOD by squared distance (CONFIRMED mechanism; thresholds are model data)

Opcode 2 (80152A14):
```
v = (node.pos + t) >> t6            // world units, written to GTE IR1..3 (16-bit, wraps beyond ±32767)
d2 = SQR(v) summed (cop2 0xA00428, sf=0)
if (ctx+0x24 /*gp+1008*/) d2 = 100   // force nearest LOD
for each child k (count node+16, ptrs node+20...): if (d2 >= rec[1] && d2 < rec[0]) draw rec[2]
```
Distance thresholds live in the level/car model files (DMD), not the EXE. If the farthest LOD of a level piece has a finite
max, it disappears past that distance regardless of §2 — **GUESS** that some level pieces do this (needs a DMD dump).
**Port change:** when walking LOD records, treat the last (largest max) record as unbounded, or scale d2 down.
Note the 16-bit IR load: per-axis distance must stay below 32768 world units or it wraps.
`FAR_LOD.md` (request 30) has the DMD dump of every LOD node on the disc and the port variant's
far-LOD switch (`portFarLod`).

## 6. Camera-region switch (CONFIRMED mechanism; GUESS on purpose)

Opcode 11 (80153048): for each region (count node+6, ptr list at node+16) transforms −camera into the region's frame
(3×3 at +4, offsets at +0x14..+0x1C, optional 2nd set at +0x24 for up to 6 planes; plane count in byte 0) and checks all
`> 0`. Camera inside any region → draw child node+12, else child node+8 (flag node+4 bit1 = hidden). GUESS: interior/exterior
or occlusion zones authored in the level data; could hide distant geometry when the camera is in a zone. Data-driven; a port
could force both children if far geometry is missing in specific zones.
`FAR_LOD.md` 6: the regions touch the geometry they switch (occlusion zones and two-version swaps), so
nothing seen from a distance is hidden; forcing both children is not safe.

## 7. Primitive emitter OTZ mapping and near/far drop (CONFIRMED)

`sub_800CDC54` (800CDC54, first function of the EXE, asm). Each draw entry: [0] mesh, [1..3] translation, [4] shift,
[5] light, [6] matrix slot, [7] …. Translation is rotated in two halves (>>15 / &0x7FFF) to keep precision. For each
polygon, z = max(SZ of verts) >> shift (world units), then:

```
z < 1024:          otz = z + bias(prim+12 bits 7..12, signed)   (if <= 0 → 1)
1024 <= z < 4096:  otz = ((z-1024)>>2) + 1024       // 800CE010..800CE04C (4 copies)
4096 <= z < 16384: otz = ((z-4096)>>3) + 1792
z >= 16384:        otz = ((z-16384)>>5) + 3328
drop if otz-1 < 0 (near / behind) or otz-1 >= 4095 (beyond OT)   // slti …,4095 at 800CE314, 800CE4EC, 800CE5BC
```
So the OT itself reaches **z ≈ 40 959 world units**; it is not the binding limit while far = 4800 (otz 1880).
Backface: NCLIP sign vs ctx+0x88 (mirror flips). Sprite half-size clamped to 1..500 px (800CE6xx) — GUESS: fine.
Near-camera list: if `otz < ctx+0x40` (20 when `sub_80156860()!=0` (viewGetRenderSplitMode), else 70) and prim flag 4, up to 200 polys are
deferred to `sub_80154690` (renderNearPolys) (GUESS: near subdivision/semi-transparent pass).
Fog: lighting uses NCCS/NCLIP/RTPT only; no DPCS, so there is no fog fade to adjust.

**Port change:** none needed up to ~40 000 units. Beyond that, extend the log segments or use a bigger OT (see §8).
`FAR_DISTANCE.md` (request 31): the OT mapping reaches 40,959 units (32,000 → slot 3,816), but the GTE does not
(§11); the port variant's GTE range pass (`TM2_PORT_FAR_DISTANCE`) scales far meshes so their SZ stays in range.

## 8. Ordering table (CONFIRMED)

`sub_800D1318`: `sub_80141938(ot, 4096)` (ClearOTagR) and `sub_80141A30(ot + 16380)` (DrawOTag from the last entry) for
each player buffer. OTs: 801E5A8C / 801E9A8C (P1 double buffer), 801EDA8C / 801F1A8C (P2 / mirror), 0x4000 bytes each,
set up by `rtDoubleBufferInit` 800D1A84 (`src/semantic/rt_buffer_init.c`). Particle/tracer code indexes with `z>>3`
directly. Bigger OT: change the 4096 / 16380 immediates in the main loop, the 4095 bounds in §7, and the BSS arrays.

## 9. Capacity caps (CONFIRMED)

* **280 mesh draw entries per view**: opcode 0 `slti s4,s3,280` at **801528EC** (ctx+0x3C counter);
  `dword_801AC810[2240]` = 280×32 bytes. Extra meshes are dropped silently.
* **Matrix slots**: opcode 5 `slti s2,s7,198` at **80152C18**; `sub_80152284` (renderScene) `sltiu …,200` (80152354 etc.);
  `dword_801F8AB8` / `dword_801AEB18` = 200 × 32 bytes.
* **Packet buffer**: 0x17CF0 (97 520) bytes per frame buffer (`D_801B600C` (renderPacketBuffer0), `D_801CDD4C` (renderPacketBuffer1)), shared by both views;
  `sub_800CDC54` stops when `cur >= end` (ctx+0x3C), other drawers check `cur + size < end`.
* Object pool: 128 objects (`sub_8014952C` (csAlloc)); deferred list 200; 20 world-space models.

A longer far distance mostly adds level-model meshes in group 1, so **the 280 cap and packet buffer** are the first things
to raise in a port (BSS arrays are fixed-size in the EXE; a port can allocate larger ones and change the immediates).

## 10. Particles, tracers, sprites (CONFIRMED tests; units GUESS)

* `sub_80110BE4` (fxSpritesDraw) (150 particles): draw if `8 <= z < 32001` (camera-space z from `sub_80153FE4` (mathMulMatVec)), OT index `z>>3`.
* `sub_80112F6C` (fxImpactParticlesDraw): `-32 <= z < 32001`. `sub_800D8230` (streaksDraw) (40 tracers): both ends `z > 0`, `z < 32001`, OT index `z>>3`;
  screen bounds `-2w < x < 4w`, `-2h < y < 4h`.
* Correction (`WORLD_MARKERS.md`): these z values are in position units, not world units. 32001 is about 4000 world units, slightly short of the 4800 model far, so far sprites disappear a little before the geometry around them.
  Raising 32001 needs the `z>>3` OT index clamped to 4095.

## 11. GTE range (CONFIRMED hardware behaviour, impact GUESS)

SZ saturates at 65535 in coordinate units, i.e. 65535 >> shift world units (**8191 at shift 3**). The 4800 default stays
inside it. IR1/IR2, which the projection divides, saturate at ±32767: 4,095 units sideways at shift 3 (the mode-0
screen edge from 7,090 deep). `FAR_DISTANCE.md` 2 has the port variant's per-mesh fix (request 31). A port that pushes far beyond ~8000 units needs its GTE emulation to stop clamping SZ and IR1/IR2 (±32767),
otherwise distant polygons project wrongly and all sort to the same OTZ. The view-mode projection H (+0x08, 277/228/91/138/457)
is unaffected.

## 12. Recommended port changes

1. Replace the far value: `D_80180DAC` (renderFarDistanceDefault) (gp+984; file 0xB3958) — for example 4800 → 16000 — or remove the far test in
   opcodes 0/1. Optionally the mirror 500 at 801522CC.
2. Raise the 280-mesh / 200-matrix caps and the packet buffer (0x17CF0) together.
3. If going past ~8000 units at shift 3: unclamped GTE SZ/IR, and keep the OTZ mapping (good to ~40 000) or enlarge the OT.
4. Check level DMD data for finite top LOD ranges (opcode 2) and region switches (opcode 11). These are data limits
   the EXE does not control.
5. Objects (cars and pickups) have no far cull. Nothing to change there.
6. Request 31 (`FAR_DISTANCE.md`): in the port variant, `--port-define TM2_PORT_FAR_DISTANCE` with `portFarDistance`
   (up to 32,767), `portMeshListCap` (up to 2,048) and `portPacketBufferBytes` (up to 390,320 since request 33-1) does 1-3 without
   touching the asm: a GTE range pass per mesh, the walker's counter biased for a bigger queue, the frame records
   pointed at bigger buffers; the effect sprites' 32001 cull (§10) follows the far distance.
