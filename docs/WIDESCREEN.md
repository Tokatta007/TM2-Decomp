# Widescreen: projection, view culling and screen-space overlays

Answers item 5 of the PC port's `docs/DECOMP_REQUESTS.md` ("Widescreen").
Static analysis only (decompiled C, IDA export, retail EXE data); nothing
here was run. Read with `MAIN_LOOP.md` (viewProc) and `DRAW_DISTANCE.md`
(the model walker and its far test).

Retail image: load address 0x800CDC54, file offset = VA - 0x800CDC54 + 0x800
(so VA 8017ECBC is file 0xB1868). gp = 0x801809D4.

Tags: **CONFIRMED** = seen in code or data and understood. **GUESS** = inferred.

## Short version

- Every 3D view renders into a **320x240** frame (not 512). The shell's 3D
  scenes use 640x480. All projection numbers come from one data table, the
  **view-mode table at `8017EC44`** (9 rows of 96 bytes).
- The projection (GTE H, OFX, OFY) is set **only** in `view_system.c`, from
  that table, every frame in `viewProc`. Nothing else writes H/OFX/OFY
  during a match.
- There is **one** kind of lateral culling: the **side planes** in the same
  table (row +0x30/+0x38). They cull the level model, the sky model and
  objects. The model emitter and the particle drawers have no screen-x test
  that a 16:9 view would hit.
- **For Hor+ 16:9:** keep H. Make the view rectangle 4/3 wider (320 → 427,
  centre 160 → 213). Widen the two side planes. Then move the HUD, which is
  fixed 320x240 coordinates.

## 1. Screen, draw and display environments (CONFIRMED)

`viewCreate` (80155888, `view_system.c`) builds a view from its mode row:

- **Draw environments:** `SetDefDrawEnv(x, y, w, h)` for buffer 0, and
  `(x + disp_w, y, w, h)` for buffer 1. The clip rectangle is the view
  rectangle, and the draw offset is (x, y).
- **Display environments:** `SetDefDispEnv(disp_w, 0, disp_w, disp_h)` and
  `(0, 0, ...)`. So the two frame buffers sit side by side in VRAM at x = 0
  and x = 320.
- **640-wide mode (mode 7):** both buffers draw to the same place
  (interlaced).
- **Clear:** `isbg = 1` clears to the background colour `D_801AB944` (viewBgColor).
  `dtd = 0`.

The PS1 output is 320x240 NTSC, non-interlaced, double buffered.
The HUD and the mirror are drawn with these same draw environments
(`rt_main.c`, lines 194–219). So HUD coordinates are relative to the view's
draw offset, and they are clipped to the view rectangle.

## 2. The view-mode table `8017EC44` (CONFIRMED values, from the EXE)

The layout is `TM2ViewMode` in `view_system.c`. These are its fields:

| Off | Field | Used by |
|---|---|---|
| +0x00 | nominal horizontal FOV (degrees) | copied to `D_80180DE4` (viewFov); its getter `80155DAC` has no callers |
| +0x04 | nominal vertical FOV (degrees) | not read |
| +0x08 | **H** for the GTE, and the x scale of software projection | `sub_8014A274` (ctc2 $26); `801555FC`, `801555C4` |
| +0x0C | y scale of software projection (always = H) | `80155634` |
| +0x10 / +0x14 | display w/h, and VRAM stride between the two buffers | `viewCreate`, `80155B30` |
| +0x18 / +0x1C | **view w/h** (draw clip). **OFX = w/2, OFY = h/2** | `viewCreate`, `viewProc`, `80156614`, `801566F8` |
| +0x20 / +0x24 | view x/y (draw offset in the frame) | `viewCreate` |
| +0x28 | axis scales {X, Y, Z} = {±4096, 3690, 3690} | `viewProc` → camera matrix `D_801A5188` (viewAxisScaleMatrix) |
| +0x30 | 3 cull planes `a[0..2]` (SVECTOR): right, left, near | `80155B30` → view obj m40 |
| +0x48 | 3 cull planes `b[0..2]`: top, bottom, far direction | `80155B30` → view obj m60 |

Plane vectors are (lateral, depth, vertical) in camera space before the
axis matrix (depth is the middle component). Here is each row:

| Mode | Used for | view x,y,w,h | H | side plane a[0]/a[1] | vert plane b[0]/b[1] |
|---|---|---|---|---|---|
| 0 | not used in game | 0,0,320,240 | 277 | (∓3547, 2048) = 30.0° | (1627, ∓3758) |
| **1** | **1 player** | 0,0,320,240 | 228 | (∓3355, 2349) = 35.0° | (1904, ∓3626) |
| **2** | 2P horizontal split, top | 0,0,320,120 | 228 | (∓3454, 2200) = 32.5° | (1765, ∓3695) |
| **3** | 2P horizontal split, bottom | 0,120,320,120 | 228 | same as 2 | same as 2 |
| **4** | **rear-view mirror** (1P view 1) | 211,20,101,30 | 91 | same as 2 | same as 2 |
| **5** | 2P vertical split, left | 0,0,160,240 | 138 | (∓3547, 2048) = 30.0° | (2681, ∓3096) |
| **6** | 2P vertical split, right | 160,0,160,240 | 138 | same as 5 | same as 5 |
| 7 | shell 3D scenes (`801222F0`) | 0,0,640,480 | 457 | (∓3355, 2349) = 35.0° | (1904, ∓3626) |
| 8 | not used (hi-res mirror?) | 234,289,201,107 | 182 | (∓3355, 2349) | (1430, ∓3838) |

- All rows: `a[2] = (0, 4095, 0)` (near), `b[2] = (0, -4095, 0)` (far direction).
- All rows: axis = {4096, 3690, 3690}, except mode 4, which has X = -4096 (the mirror flip).
- Display size is 320x240 for modes 0–6 and 640x480 for modes 7–8.

**Who picks the modes** (CONFIRMED):

- **Round start, `sub_8011995C` (arenaSetup) (`arena_setup.c`):**
  - 1P: `(1, 0)`, plus `(4, 1)` for the mirror.
  - Split 1 (vertical): `(5, 0)` + `(6, 1)`.
  - Split 2 (horizontal): `(2, 0)` + `(3, 1)`.
- **In-game split toggle:** `sub_8011C530` (carViewHotkeys) (`car_records.c`) switches between the two split layouts.
- **Unused:** `sub_8015576C` (viewCycleMode) (cycles modes 0..6) has no callers.

### The 0.9 axis scale (CONFIRMED maths, GUESS on purpose)

`viewProc` loads the camera matrix with these entries: `m[0][0] = axis X`,
`m[1][2] = -3690`, `m[2][1] = 3690`. So camera Y and Z are both scaled by
0.9, and the projection becomes:

```
SX = OFX + H * x / (0.9 * depth)      SY = OFY + H * (-z) / depth
```

- **Horizontal pixels are 1/0.9 = 1.11x wider than vertical ones.** For mode 1 the real horizontal
  half-angle is atan(160·0.9/228) = **32.3°** (FOV 64.5°). Vertically it is
  atan(120/228) = 27.8° (FOV 55.5°, matching the +0x04 value 55).
- **GUESS:** this compensates the PS1's non-square 320-mode pixels (PAR ≈ 0.91 on NTSC).
  On a TV the picture is undistorted. A port that stretches 320x240 to an exact
  4:3 window shows everything 11% too wide, as most emulators do.
- **Fix:** set the axis X entry to ±3690 (instead of ±4096) for square pixels. That gives
  `SX = OFX + H*x/depth`. It changes only X, not SZ, so OTZ and the far test are unaffected.
- **Side planes vs the 0.9:** the planes ignore the 0.9. They are atan(halfW/H) (modes 0,1,5,6,7),
  so they sit about 3° outside the true edge. Modes 2/3/4 are the exception: they use 32.5°.
  That is only 0.2° outside the true edge for 2/3.

## 3. Where H / OFX / OFY are written (CONFIRMED)

H is `ctc2 $26` in `sub_8014A274` (`gte_depth.c`). OFX/OFY are `ctc2 $24/$25`
in `SetGeomOffset` 8014A25C (`gte_setters.c`). The only callers (IDA export and C):

| Where | H | OFX, OFY | When |
|---|---|---|---|
| `InitGeom` (`sdk_libgte.c`) | 1000 | 0, 0 | boot |
| `viewCreate` 80155888 | row +0x08 | w/2, h/2 of the view | view created |
| `sub_80155B30` (viewLoadMode) 80155B30 | row +0x08 | **disp_w/2, disp_h/2** (160,120), even for split and mirror rows | view created / mode cycle |
| **`viewProc` 80155E80** (end) | row +0x08 | **w/2, h/2** of the view | **every frame, per view, before that view is drawn** |
| `sub_801566F8` (viewSetFixedCamera) 801566F8 | **1** | w/2, h/2 | menu objects (`menu_slider_*.cpp`, `menu_timed_command.cpp`, `scene_init.c`) |

- **The last write wins.** `viewProc` runs right before each view's draw calls (see `MAIN_LOOP.md`).
  So the values in effect are **H from +0x08 and the centre (w/2, h/2) of the view rectangle**.
  For the mirror that is (50, 15) inside its 101x30 window.
- **Read-back:** the model emitter reads H back with `cfc2 $26` (800CDC54) to size sprites and beams.
- **No other ctc2 writes:** the renderer's other ctc2 writes are $0–$7 (rotation and translation) and
  $8–$20 (light), never $24–$26.

**Software projection** (not the GTE) uses the same row:

- **Centre:** from `sub_80156614(view)` (viewGetCentre) = (w/2, h/2).
- **Scales:** x scale `801555FC` (+0x08) and y scale `80155634` (+0x0C).
- **Camera:** `viewGetEyeTrans` / `801566B0`, the same matrix as the GTE, including the 0.9.
- **Users:**
  - `800D8230` (tracers, `screen_card_draw.c`)
  - `801084D8` (lightning arcs, `weapon_arcs_draw.c`)
  - `80110BE4` (150 effect sprites, `world_collapse.c`)
  - `80112F6C` (impact particles, `car_damage_human.c`)
  - Debris sprites in `8014C19C` (`engine_fileio_debris.c`) size by `801555C4(0)`, i.e. **view 0's H even
    in view 1**. This is a retail quirk.

So if the table's w and centre change, all of these move with the GTE path.
No second copy of 160/120 needs patching in the 3D draw code.

## 4. Every lateral cull (CONFIRMED unless tagged)

| # | What | Where | Test | Width-dependent? |
|---|---|---|---|---|
| 1 | Level/arena model and the 20 world models, per node | model walker `sub_801526CC` (renderWalkModel), opcodes 0/1 (801528C0.., 801529C8..; `engine_render_scene.c` asm) | sphere vs view m40 columns (a[0] right, a[1] left, a[2] near) and m60 (b[0], b[1] top/bottom, b[2] far) | **yes: a[0], a[1]** |
| 2 | Camera-attached model (DMD id 2, registered by `80156344` from `800D1DB0`; **sky/backdrop shell**, GUESS) | `sub_80152284` (renderScene) group 2, then opcodes 0/1 | same planes (forced visible at the root, but per-node culling is on) | **yes** |
| 3 | Object list (cars, pickups, props) | `sub_801491E8` (csUpdMat) → `sub_80149344` (csSphereVisible) (`engine_clut_anim.c`) | root-sphere radius (node+16) vs the parent's m40 a[0..2] and m60 b[0..1] | **yes: a[0], a[1]** |
| 4 | Per-node culling for objects | off (`D_80180DC8` (renderNodeCullEnable) = 0 → ctx+0x28 = 1) | none | no |
| 5 | Model emitter `800CDC54` (C: `reference/model_emitter_800CDC54.c`) | per polygon | **NCLIP** backface (sign flipped for the mirror, ctx+0x88), OTZ range `otz-1 in [0,4095)`. **No GTE FLAG read, no SX/SY range test.** Sprite half-size 1..500 px, beam width 1..2w | no |
| 6 | Near-polygon pass `80154690` (`engine_math_near_pass.c`) | subdivides polys with otz < 70 (20 in split) at screen-space midpoints | no screen bounds | no |
| 7 | Tracers `800D8230` | both ends: `z > 0`, `z < 32001`, and **`-2cx < x < 4cx`, `-2cy < y < 4cy`** (cx, cy = view centre) | follows the table automatically |
| 8 | Effect sprites `80110BE4`, particles `80112F6C` | `(u32)(sx + 99) < 1099` and the same for sy, i.e. **-99 ≤ sx < 1000** (absolute) | fine up to 1000 px |
| 9 | Lightning `801084D8` | screen x, y in [-4000, 4000]; depth 1..4090 | no |
| 10 | Debris `8014C19C` | GTE RotTransPers, sprite half-size ≤ 500 | no |
| 11 | Weapon auto-target `80109F94` / `8010A2AC` (`weapon_targets.c`) | 45° cone (`|forward| ≥ |side|`) around the **car**, not the screen; range 8000 | no (gameplay, leave it) |

- **GTE and GPU limits (hardware behaviour):**
  - SX/SY saturate at -1024..1023.
  - The GPU drops a primitive whose vertices span more than 1023 px across or 511 px down.
  - At 427 px wide nothing new reaches these limits. A port that emulates them keeps retail
    near-camera artefacts; a port that does not, loses them.
- **GUESS:** with only the view rectangle widened, the pop-in at the sides comes from rows 1–3. They
  stop drawing anything whose bounding sphere is fully outside ±35° (1P). The new edge is at about
  ±40° (true) or ±43° (nominal).

## 5. Screen-space things that will not move by themselves

These are fixed 320x240 (or 160/120 split) coordinates.

| What | Where | Coordinates (x) |
|---|---|---|
| 1P HUD | `func_800F8370` (hudDrawSingleView) (`hud_main.c`) | name text centred 147,30; top box 84..214; health 10..80 (back 9..81); turbo 270..318; weapon list 272..318; selected weapon 283; lap 244/248; score 216; text 50,212 |
| 1P/2P HUD boxes | `sub_800FB0B0` (hudInitPanels) (`hud_init.c`) | top 84..214; map 273..315 / frame 270..318 (1P), 113..158 (vertical split), right column y 42..114 (horizontal split) |
| Rear-view mirror frame | `uadashInitRearViewMirror` 800FB8AC (`hud_init.c`) | black bars 209..314 x 18..52. Must match **mode 4 x = 211, y = 20, 101x30** |
| Split HUDs | `func_800F9084` (hudDrawSplitVertical) (vertical, divider bar 157..163), `func_800F9D48` (hudDrawSplitHorizontal) (horizontal, divider y 117..123, x 0..320) (`hud_split.cpp`) | many fixed x values (11, 34/38, 83..154, 112..159, 270..318, 288) |
| Radar | `sub_800FC5C4` (hudRadarInit) (`ai_route_setup.c`), blips `sub_800FC928` (hudRadarPlotCar) / draw `sub_800FBE68` (hudDisplayRadar) (`hud_radar.c`) | box 3..83 (left anchored); hit arrows at fixed x 0..79 |
| Dashboard icons | `UAdashInitDashboardIcons` 800FAA0C (`hud_icons.c`) | 221,200. 800FADDC / 800FAEEC are not functions: they are loop branches inside `UAdashInitDashboardIcons` (800FAA0C..800FB0B0), which is load-time init (`world_load_object.c` case 1260). Nothing more to check (see RENDER_ANSWERS.md R3) |
| Messages | `sub_800FD0D0` (hudDrawTargetPanel) (`car_destroyed.c`) | centred at x 48 |
| Pause / mid-game menu | `sub_80129528` (pauseMenu) and helpers (`screen_pause_menu.c`), `screenCheckMidGameOptions` (sessionRun) (`session_runner.cpp`) | centred at x 159 |
| Full-screen flash | `sub_800DC45C` (fxSheetStart) sets two POLY_F4 0..320 x 0..240; `sub_800DC590` (fxSheetDisplay) adds them to each view's OT (from `800D8230`). No widescreen change is needed. For 60 fps, copy the polys per view and per redraw: `fxSheetDisplay` links one static prim chosen by `bulletsFrameParity` (see RENDER_ANSWERS.md R3) | 320x240 |
| Explosion red tint | `sub_8011335C` (explodeMakeScreenRed) (`fx_debris_splash.c`) | 0..320 x 0..240. It has no caller in retail, so it needs no change (see RENDER_ANSWERS.md R3) |
| Lost-a-life darken | `sub_8012940C` (screenLostALife) (`car_parts_spin.c`) | quad and `SetDefDrawEnv(0,0,320,240)` |
| Split divider bar | `D_80183EE8` (hudSplitDivider), set in the split HUDs | see above |

- **Sky/backdrop:** there is no 2D backdrop. The background is the clear colour
  (`D_801AB944` (viewBgColor), set by `8015659C`) plus the camera-attached 3D model. It widens
  with the view once its side planes are widened (row 2 above).
- **Lens flares:** none found. No flare or sun code; `grep` and the drawers above show
  only tracers, sprites, particles, lightning and debris.
- **Particles and sprites:** these are 3D-projected with the view centre, so they place
  themselves correctly.

## 6. To go 16:9: change these

Hor+ (same vertical FOV, wider horizontal). The factor is 4/3 (16:9 ÷ 4:3).
For another aspect A, use **halfW' = halfW · A / (4/3)**.

**A. View-mode table (data, 8017EC44)**

The port can patch the bytes, or edit its copy of the table.
H (+0x08/+0x0C) stays the same.

| Mode | Field | VA (file) | Retail → 16:9 |
|---|---|---|---|
| 1 | w (+0x18) | 8017ECBC (B1868) | 320 → **427** (OFX 213) |
| 1 | a[0] / a[1] lateral, depth (+0x30 / +0x38) | 8017ECD4 / 8017ECDC (B1880 / B1888) | (-3355, 2349) / (3355, 2349) → **(-2991, 2799) / (2991, 2799)** (43.1°) |
| 2, 3 | w | 8017ED1C, 8017ED7C (B18C8, B1928) | 320 → **427** |
| 2, 3 | a[0] / a[1] | 8017ED34/3C, 8017ED94/9C (B18E0/E8, B1940/48) | (∓3454, 2200) → **(∓2991, 2799)** |
| 5, 6 | w | 8017EE3C, 8017EE9C (B19E8, B1A48) | 160 → **213** |
| 6 | x (+0x20) | 8017EEA4 (B1A50) | 160 → **213** |
| 5, 6 | a[0] / a[1] | 8017EE54/5C, 8017EEB4/BC (B1A00/08, B1A60/68) | (∓3547, 2048) → **(∓3241, 2505)** (37.7°) |
| 4 (mirror) | x (+0x20) | 8017EDE4 (B1990) | 211 → **318** if the HUD's right group is anchored right (+107); planes unchanged |
| 1..6 | disp_w (+0x10) | 8017ECB4, ... | only if the port emulates VRAM: the second buffer moves to x = disp_w (2×427 = 854 < 1024; check texture pages at x ≥ 640 — GUESS) |
| 7 (shell, optional) | w; a[0]/a[1] | 8017EEFC; 8017EF14/1C | 640 → 853; (∓2994, 2795) |

- **Plane formula:** α = atan(halfW'/H) (nominal, about 3° of margin over the true edge); store
  `(-4096·cos α, 4096·sin α, 0)` and `(+4096·cos α, 4096·sin α, 0)`. Example: 21:9 in 1P gives
  50.8° → (∓2586, 3176).
- **Simplest safe option:** set a[0] and a[1] to (0, 4095, 0), the same as the near plane.
  This disables lateral culling completely. The cost is more draw entries, against the
  280-mesh / 200-matrix caps (see `DRAW_DISTANCE.md` §9).
- **Where the planes go:** they are copied into the view object only in `sub_80155B30` (viewLoadMode) (at view
  creation), so the table must be patched before `viewCreate` runs.

**B. Optional: square pixels.** The axis X at +0x28 (8017ECCC for mode 1,
likewise for the other rows; the mirror's is -4096) controls the 0.9 squeeze.
Use 4096 → **3690** if the port shows the frame at the true 16:9 (square
pixels) and wants retail's TV proportions. Leave it at 4096 to match how
emulators show the game. Initial copy: `D_8017EC24` (viewAxisMatrixTemplate) (written by `80155B30`).

**C. Code with 320/160/240 immediates** (2D only; the port's C):

- `hud_main.c` (`800F8370`), `hud_init.c` (`800FB0B0`, `800FB8AC`), `hud_split.cpp` (`800F9084`, `800F9D48`), `hud_icons.c` (`800FAA0C`), `ai_route_setup.c` (`800FC5C4`) and `hud_radar.c` (`800FC928`, `800FBE68`) (radar), `car_destroyed.c` (`800FD0D0`).
  - Suggested anchoring: **left group** (radar, health 9..81, lives, text at 50) +0. **Centre group** (top box 84..214, text at 147, split divider 157..163) +53. **Right group** (mirror 209..314 and mode 4 x, turbo/weapons/map 244..318, icons 221) +107.
  - Or draw the HUD into a 320x240 layer, centred, with pillarbox margins.
- Full-screen quads: `sub_8012940C` (screenLostALife) (`car_parts_spin.c`): 320 → 427. `sub_800DC45C` (fxSheetStart) needs no change, and `sub_8011335C` (explodeMakeScreenRed) has no caller (see RENDER_ANSWERS.md R3).
- Pause menu `screen_pause_menu.c`: centre 159 → 213.

**D. Nothing to change.** The GTE H, the near plane, the far test, the emitter and its OTZ
mapping, the particle and tracer bounds, the weapon-target cone, and the 3D sky.
They follow the table, or they do not depend on width.

## Not checked

- Whether shell 2D screens (car select, menus: 320x240 or 640x480) should stretch or pillarbox.
- Whether level DMD data has per-node `never cull` flags (node+24 & 0x2000, node+21 bit 2) on
  big pieces. If so, widening is safe for those anyway.
