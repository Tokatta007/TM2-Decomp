# Playtest answers (PC port playtests, from 2026-10-17)

Answers to findings from playtests of the PC port, each a numbered section. Request 45 asked five
things; items 1 and 2 changed code (`BALANCE.md` 14), item 3 added one visual switch, items 4 and 5 are answers only.

Sources: the C and the hand-written asm in `src/semantic` (file:line refers to this branch, `decomp-requests-45`;
asm by retail address), `reference/` (the C forms of the walker, the emitter and the near pass), and the disc's DMD
files (read-only, `tools/dmd_lod_dump.py` and small scratch scripts). Paths under `src/ui/` or `vplus/` are the
port's (the port repository), read only to see what it changes. Tags: **CONFIRMED** = read in the code or counted in
the data; **LIKELY** = follows from it, not seen running; **GUESS** = plausible, not checked. Nothing was run in an
emulator.

| # | Item | Result |
|---|---|---|
| 1 | Bit 8 split per car | **built**: bit 8 Outlaw 2 (900), **bit 21** Mr. Grimm (1200), **bit 22** Thumper (1500); `TM2_BAL_ALL` `0x7FFFFF` (`BALANCE.md` 14.1) |
| 2 | Ricochet from a fast car | the shot bounces off **its owner's car** (the hit pass treats every car's hit box as a wall, the owner's too); retail has the same bounce, hidden by its self-hit; **built** under bit 18: the shot passes through its owner until it has left his box (`BALANCE.md` 14.2, `RETAIL_BUGS.md` 31) |
| 3 | The player's car disappearing | the two limits that can drop a whole car are the **mesh queue** (280, `portMeshListCap`) and the **packet buffer** (97,520 bytes, `portPacketBufferBytes`); the own car is the **last car walked**, so it goes first. **Built:** `portVisual` bit 2 `TM2_VIS_OWN_CAR` walks the view's own car first (section 3) |
| 4 | Wobble at Farthest | pickups and the Paris trees are **beams** (camera-facing strips) built in the emitter from integer screen positions, an integer angle and an integer width; the parked cars are small polygons whose back-face test is integer. Hook the emitter's beam / sprite branches and NCLIP (section 4) |
| 5 | Car select | the same walker, emitter, projection and OT as a battle (no fixed OT entry, no other near clip); the likely causes are the port's finer sort weakening the models' per-polygon OT bias, and thin double-sided plates (section 5) |

## 1. Bit 8 split per car

See `BALANCE.md` 14.1. `include/semantic/port_balance.h`: `TM2_BAL_RECHARGE` (= `TM2_BAL_OUTLAW2_RECHARGE`, bit 8)
Outlaw 2 only, `TM2_BAL_GRIMM_RECHARGE` 0x200000 (bit 21), `TM2_BAL_THUMPER_RECHARGE` 0x400000 (bit 22);
`portBalancePeriod` (`src/semantic/port_variant_arrays.c`) tests each car's own bit. For an old saved word with bit 8
set, set bits 21 and 22 too (it meant all three).

## 2. Ricochet from a fast car

See `BALANCE.md` 14.2 for the whole analysis (file:line). In short:

- **Launch** (`weapon_select.c:139-176`): the shot at the car's origin + 40 up, the car's heading, a fixed 100 units
  an update (`offset[1] = 100`, `:168`); **no speed is taken from the car**. The shot object has `collides = 0`
  (`:148`): cars never collide with it, so nothing pushes it through car physics.
- **Each update** (`weapon_ricochet.c`): move 100, then the hit pass `hdpCollideObject` (`:117`), which tests the level
  and then **every collidable scene object except the shot, its owner's car included** (`hdpPassRun` 8014FC50..
  8014FC70). A car's hit box reports the car type (16..240) as its kind, which the ricochet does not filter (`:118`),
  so it **bounces**: heading reflected, power doubled and the **red flicker** (`lightFlickerStart(.., 100)`) at the
  first bounce, pushed out by `depth + 20` (`:130-153`). The test point is 40 above the shot (`asset_pools.c:87`).
- **With request 41's bit 18:** a car at about the shot's speed (top speed + turbo) keeps it 100 ahead, never 160
  away, so it never leaves his box and cannot hit him; it looks carried by the car (it is not faster than the car).
  A car faster than 100 an update catches it; if its box is tall enough (Axel, Sweet Tooth, Hammerhead, Mr. Slam, Mr.
  Grimm, Warthog; Minion at once) it bounces off him again and again, pushed along in front with the red flicker.
- **Retail:** the same bounce exists, but a fast car's shot explodes on its owner at its third update first
  (`RETAIL_BUGS.md` 28), so it is not seen; a Minion's shot bounces off him at its first update (`RETAIL_BUGS.md` 31,
  LIKELY).
- **Built (bit 18):** after each hit pass, `portBalanceRicochetPass` drops a hit on the owner's car while the shot has
  not yet left his box (`weapon_ricochet.c:120-124`, `:144-147`): no bounce, no flicker, no push. Not built: giving
  the shot the car's speed (a new rule, if wanted).

## 3. The player's car disappearing (Antarctica, Axel, Farthest, 16 cars, uncapped frame rate)

### 3.1 The walk order: the own car is the last car walked (CONFIRMED)

`renderScene` (80152284, `src/semantic/engine_render_scene.c:166-300`) walks, per view:

1. **the level root** (`:222-226`, matrix slot `*count`, not incremented);
2. **the world models** (`:227-235`): up to `TM2_MAX_WORLD_MODELS` (40 with 16 cars), among them every car's shadow
   (`carBodyObjectShow`, `ai_route_helpers.c:154-155`);
3. **the sky** (the view's follow object, `:239-247`);
4. **the scene list** (`:258-268`): `csListAdd` pushes at the head (`engine_clut_anim.c:577-581`), so the list is in
   **reverse load order**. The level's objects, missiles and trails are added by `arenaResetSystems`
   (`sound_manager_reap.c:17`) before the cars; the cars are added by `dbInitCsForAModel` (`world_load_object.c:116-134`)
   humans first, then opponents (`shell_main.c:401-411`; the port's branch `:343-350`). So the order is: objects added
   at run time (the port's drop-ins, `port_variant_arrays.c:2579`), the **opponents from the last to the first**, the
   **humans in reverse (human 0's car the last car walked)**, then the trails, missiles, level objects and debris.

Every cap below drops **what comes after** the point where it fills, so in one player the own car is the first car
to go.

### 3.2 Every limit that could drop a car

| Limit | Where | What happens when it is hit | Can it hit here? |
|---|---|---|---|
| **Mesh queue**, 280 entries | the walker's opcode 0, `slti $20,$19,280` at 801528EC (`engine_render_scene.c:711`), counter ctx+0x3C | the mesh is not queued; the counter never goes back, so **every later mesh of the view** is dropped (the rest of the object and every object after it). The port's cap: `portFarDistanceMeshListBegin` (`port_variant_arrays.c:500-518`) starts the counter at 280 - cap (cap up to 2,048), the bias taken off at `engine_render_scene.c:278` | **LIKELY with the retail size** (`portMeshListCap` 0): GLACIER's level root has 504 meshes at the nearest bands and 317 in the far bands with the far-LOD switch (`dmd_lod_dump.py --summary`), plus 16 shadows, the sky and the cars before the own car; at Farthest a view can reach 280 before the own car. With the cap at 2,048: no |
| **Packet buffer**, 0x17CF0 = 97,520 bytes a frame for every view | `rtDoubleBufferInit` 800D1A84; the port's `portPacketBufferBytes` up to 390,320 (`port_variant_arrays.c:456-458`, `:733-756`) | the emitter tests the end before each primitive (800CDF1C / 800CDF20, `model_emitter.c:218-224`) and **abandons every remaining mesh of the queue** (exit 800CE8D4); the near pass stops too (`reference/near_pass_engine_math_near_pass.c:335-336`). Before the scene a view also spends packets on `streaksDraw`, `fxSpritesDraw`, `debrisDraw` (`rt_main.c:141-161`), and at Farthest the effect culls follow `portFarDistance` (`portFarDistanceFxLimit`, `port_variant_arrays.c:699-704`) | **LIKELY with the retail size** (the port measured 91-170 KB at 2 players, 303 KB at 4): effect-heavy frames fill it. With 390,320: no |
| OT index range | the emitter: z = the deepest SZ >> shift, compressed at 1024 / 4096 / 16384 (`model_emitter.c:273-320`); a polygon is **dropped** when otz - 1 < 0 or >= 4095 (800CE5B4 / 800CE5BC) | that polygon only; z = 0 (within a unit of the eye) or past 40,960 world units. The range pass's `key ^ (k << 28)` changes only the cache key, the OT index is the same | no: the own car is near and in front |
| Matrix slots, 200 objects / 198 transform nodes | `renderScene`'s `if (*count < 200)` (`:229`, `:241`, `:260`); opcode 5's `slti 198` at 80152C18 | **misplaced, not dropped**: a full opcode 5 gives its children the current slot (car parts with a wrong matrix); past 200 every later object shares slot 200 (drawn with the last-written matrix: the car elsewhere) | **UNLIKELY**: before the own car a view uses about 110 (worst case 15 Minions about 194): the level's transform nodes (35 in GLACIER's root), the 3103 model 1 + 5, 16 shadows x 2, the sky, 15 cars x (1 + their transform nodes: MN 7, FL 6, BB 5, HH 4, AX 3, others 0) |
| Near plane | per polygon only: otz <= 0 dropped (800CE058, `model_emitter.c:309`); no clip (SZ saturates, a polygon partly behind is still drawn). The near list (otz < 70, 20 in split screen, mesh flag +26 & 4, at most 200: 800CE6A8..800CE6CC; past 200 linked normally) | one polygon | no: no car body mesh has flag 4 (Axel only on 13 shockwave-ring meshes) |
| Sphere / frustum | objects skip the walker's node culls (`renderNodeCullEnable` 0 -> ctx+0x28 = 1, `engine_render_scene.c:254-257`; opcodes 0/1 skip their tests at 80152848..80152854); `csSphereVisible` 80149344 (`engine_clut_anim.c:536-566`) tests the root group's radius at the **object origin** (the root's centre +4..+12 ignored), 3 + 2 planes, no far plane, CPU s32 arithmetic (overflows only past about 65,535 world units) | the whole object | no for Axel (centre 0, radius 554); Mr. Grimm's root sphere is off-centre (-1, -260, 57), r 334: an edge cull at the screen side is possible (GUESS) |
| Far cull | opcodes 0/1 (801528C0..801528DC): `depth < -r - (far << shift)`, `far` = `portFarDistanceFar()` 1..32,767 (`port_variant_arrays.c:482-494`) | level nodes and world models (the shadows) | no: objects are never far-culled |
| Request 31's range pass | `portFarDistanceScale` (`port_variant_arrays.c:581-658`) | never skips a mesh: n <= 0, slot >= 201, a sphere that fits, the near guard (k 0) or the 512 copies used up all leave the mesh as retail queued it; k <= the mesh's shift (3) | no: the own car (within 4,095) is never scaled |
| Other counts | world models 40 (`controls.c:283-292`: a full table loses a shadow), the walker's stack 200 (deepest on GLACIER and the cars: 24), the csAlloc pool 256 (allocation only) | | no |

**Not a cap, but a flicker of parts (CONFIRMED data, LIKELY effect, retail too):** Axel's split LOD nodes measure the
distance from their own reference points (e.g. set 8001F5E4: [0, 200) at (0, -1, 37), [200, 500) at (0, -1, 51)), so
at a 30-45 degree camera elevation no band of the set is drawn for camera distances 202.2-204.3 (other sets: 152-154,
about 500). The 1P chase camera sits about 180 away (`ai_car_start.c:264-266`, (0, -1200, 800)) and crosses 150 / 200
when it eases. Single parts (18-47 primitives) vanish for a frame, not the whole car; an uncapped frame rate samples
the gaps more often.

**What to look at in the frame where the car vanishes (for the port;** the render capacity counter
`portRenderStats`, `RENDER_ANSWERS.md` R2.4, records all three per view with `TM2_PORT_FAR_DISTANCE`**):** the mesh counter (ctx+0x3C, minus the bias)
against the cap, the frame record's cursor (+8) - start (+4) against `portPacketBufferBytes`, the slot counter
(ctx+0x08). If both caps are already at their maximum, check the port's redraw of an uncapped frame: each presented
frame must reset the packet cursor (`rt_main.c:132`), clear the OT (`:134`) and go through `renderScene` ->
`renderInitState` (`engine_render_scene.c:113-140`); packets piling up across redraws would cut the tail of the
queue, the own car (GUESS: port code).

**Per-frame state that is not frame-rate independent** (for the uncapped rate): `renderFlipFrame`
(`engine_render_scene.c:106`, called from `rt_main.c:160`: re-arms `animUpdateAll` and the walker's `animUpdateOne`)
should run once per simulation step, as the `streaksDraw` ticks (`RENDER_ANSWERS.md` R3); the mesh / slot / near
counters, the OT and the packet cursor are reset per view.

### 3.3 Built: `portVisual` bit 2, the own car first (`TM2_VIS_OWN_CAR` 0x4)

With `--port-define TM2_PORT_VISUAL` and bit 2 of `portVisual` (`include/semantic/port_visual.h`; read live, drawing
only, no `rand()`, no state):

- `portVisualOwnCar(view)` (`port_variant_arrays.c`, after `portVisualArcOtz`) returns `humanCarObjects[h]` for the
  human of that view (`TM2_LOCAL_HUMAN(view)`: with `TM2_PORT_HUMANS` the local slot's human, else human `view`), or
  0 when the bit is clear, in the mirror view (mode 4), with no human on the view, or when the object is not drawn
  (+8 = 0) or has no model (+0x11C).
- `renderScene` (`engine_render_scene.c:198-220`), after the mesh queue's begin and before the level root: the car is
  walked exactly as the object loop walks it (its own slot `if (*count < 200) *count++`, `csUpdMat` with
  `renderNodeCullEnable`, the loop's depth and node-cull flag ctx+0x28, `renderWalkModel` if +292 is set), then the
  depth and ctx+0x28 restored and one more slot taken (the level copies its matrix to slot `*count`, which would
  otherwise overwrite the car's). The object loop skips that object (`RENDER_OBJ_WALKED`, `:155-161`, `:259`).
- **Effect:** the car is the first thing in the mesh queue and the first object's packets, so a full queue or buffer
  drops far level geometry and the other cars instead. In each OT entry it is linked first (it wins ties: drawn on top
  within the same entry), which is harmless. Two matrix slots more per view (no risk: section 3.2). Animation nodes
  of the own car (Dark Tooth 10, Outlaw 2 1, Axel none) are advanced in the same number, in another order (draw state
  only).
- Bit clear: `portVisualOwnCar` returns 0 at its first test and the walk is retail's (one call and branch more). The
  retail build and the variant without `TM2_PORT_VISUAL` are unchanged; with it `renderScene` is changed
  (`tools/check_custom.py` check 2: `portVisualOwnCar` calls only `viewGetMode` / `portLocalHuman`, is called only by
  `renderScene`, once, before its first `renderWalkModel`; the loop through `RENDER_OBJ_WALKED`).
- **The real fix is the two caps:** set `portMeshListCap` and `portPacketBufferBytes` to their maximum at Farthest (the
  bit only moves what is dropped).

## 4. Wobble at the farthest distance (pickups on level 1, Paris trees, pickups and parked cars)

### 4.1 How they are drawn (CONFIRMED)

All three are nodes of the **level model**, walked from the level root (`renderScene`, `engine_render_scene.c:222-226`)
and emitted by `sub_800CDC54` (through `portFarDistanceEmit`). None has a scene object, `csUpdMat` or an object
matrix slot of its own: they are placed by type-4 offset nodes (walker 80152ABC: integer additions, exact), at shift 3
(no type-12 scale node above them in PARIS or FREEWAY), with the level root's slot (3 parked cars under type-5 nodes,
see below). The emitter rotates the translation with a split `>> 15` / `& 0x7FFF` (800CDD3C..800CDDC4,
`model_emitter.c:88-128`), whose fraction the port already keeps. **So the walker loses no precision for them; the
difference is the primitive type** (face word +12, bits 16..18), counted on the disc:

| Object | In the data | Primitive |
|---|---|---|
| Pickups (FREEWAY 36, PARIS 42) | one mesh with **one type-1 beam** (the icon: half-width w 20, half-length 50 model units) and a 2-frame animation node (frame 0 empty, frame 1 a label beam w 43, len 16) | beams only (CONFIRMED) |
| Paris trees | target 520 (28-30 breakable props): a polygon base and one beam (w 96, len 144) under an LOD band [0, 750); two meshes of 11 beams each (w 96, len 144 / 120) under [0, 750) | beams (data CONFIRMED; that they are the trees LIKELY: 24 x 36 world units, in rows) |
| Parked cars | target 502, 10 instances, 54 x 167 x 43: LOD [0, 150) 12 polygons, [150, 300) 9 quads, [300, 1000) 5 quads; 7 under offset nodes, 3 under type-5 nodes with exact 90-degree matrices | ordinary quads and triangles, 34 of 40 back-face culled (NCLIP), 6 double-sided (data CONFIRMED; that they are the cars LIKELY) |

Farthest also turns on the far-LOD switch (`portFarLod`): retail stops drawing Paris pickups at 375 / 425, the tree
bands at 750 and the parked cars at 1,000; with the switch they are drawn at any distance, where they are a few PS1
pixels across.

### 4.2 Where precision is lost

**A. The beam builder (pickups, trees; CONFIRMED arithmetic, the main cause LIKELY).** The emitter's type-1 branch
800CE210..800CE420 (`model_emitter.c:430-573`):

1. one RTPT projects the beam's ends at z + len and z - len (800CE258, `:449`): **integer** SX / SY (model z is up: a
   vertical strip turned to the camera);
2. its direction is `ratan2(y0 - y1, x0 - x1)` of those integer ends (800CE350, `:517`): for a strip a few pixels
   tall the x difference is 0 or +-1, so the angle jumps between 90 degrees and about 45-80;
3. the half-width `q = w x H / SZ1` is an integer divide (800CE378), clamped to 1..2w;
4. the side step `s = q x sin / 4096`, `c = q x cos / 4096`, truncated (800CE3A8..800CE3D4): with q = 1 off vertical,
   s = 0 and the quad collapses (the billboard blinks out for a frame); with q >= 2 the width steps by 2 pixels;
5. the corners stored with `sh` (800CE3DC..800CE420).

A pickup (100 x 40 model units) is 2,850 / D PS1 pixels tall at D world units (H 228) and is clamped to q = 1 beyond
about 570; a tree about 6,600 / D. The port's quad gives the two ends their subpixel position, but the angle, the
width and the s / c rounding stay the game's integers: the quads change width, tilt and blink as the camera moves.
Ultra-wide makes it worse (more display pixels behind each PS1 x pixel: a 1-pixel width step is about 10 display
pixels at 3440 wide; the widths ignore the anamorphic squeeze, `RENDER_ANSWERS.md` R1.5). The point-sprite branch
(800CE43C..800CE5B0, `:580-682`; clamps 800CE538 / 800CE550) has the same integer sizes without the angle (FREEWAY's
level has 1,054 sprites).

**B. The range pass (request 31; CONFIRMED, minor here).** `portFarDistanceScale` (`port_variant_arrays.c:581`)
scales each matrix element `m >> k` (floor: up to 2^k - 1 units of 1/4096, changing as the camera turns, in the
vertices and in TR = R' t). Two neighbouring meshes with a different k (a small far object and the big ground mesh
it stands on, kept at a lower k by the near guard) move against each other by about H (2^k - 1) 1.7 / 4096: 0.1 PS1
pixel at k = 1, 0.3 at k = 2 (Paris k <= 1, FREEWAY k <= 2). Beams and sprites of a scaled mesh are also 2^k too wide
(q divides by the scaled SZ1), rarely visible (far beams sit at q = 1).

**C. The integer back-face test (parked cars; CONFIRMED arithmetic, LIKELY cause).** NCLIP on integer SX / SY before
any float exists: 800CDF9C (triangles, `model_emitter.c:255`) and 800CE0F8 (quads, v0..v2 only, `:351`); skipped when
MAC0 < 0, drawn when MAC0 = 0. A parked car at 1,000+ units is about 5 x 1 PS1 pixels: its nearly edge-on faces flip
between culled and drawn, and a back face with integer area 0 is drawn from its float corners as a reversed sliver.

**D. Ruled out for these objects (CONFIRMED by the data).** Type-5 transform nodes (`t' = m (t + off)` with CPU
`mult` truncated, 80152B28..80152C14, and the slot product by a truncating MVMVA, 80152C5C..) lose precision only for
non-90-degree matrices; the parked cars' are exact. (The 16 non-90-degree nodes in PARIS (8, 25, 48, 58 degrees;
radii 700-2,900) and FREEWAY's one (target 508, 52 degrees) are large structures with about 1 LSB of slot error and up
to 1 unit of translation error a frame: a separate, small wobble on those only.) Not `csUpdMat` objects
(`engine_clut_anim.c:506`: cars, trains 506 / 507, projectiles only); not `renderNearPolys` (near polygons only); not
the LOD distance (80152A14: switches bands, moves no vertex).

### 4.3 What the port should hook

1. **The emitter's beam and point-sprite branches** (`sub_800CDC54` at 800CE210 and 800CE43C): compute the 4 corners in
   float and queue them precisely, as the port already does for `fxSpritesDraw`: the float ends (the GTE float FIFO),
   the direction from the float difference, the half-width `w x H / (SZ1 x 2^k)` not clamped to 1, its x times the
   anamorphic factor, k = (the entry's key >> 28) & 3. The simplest route is the C emitter
   (`reference/model_emitter_800CDC54.c`) in the variant; keep the integer `sh` stores for a "PS1 look" mode. This
   fixes the pickups, the trees, the label beams and the 2^k widths.

   **Built (request 61 addendum, `TM2_PORT_C_EMITTER`, `include/semantic/port_c_emitter.h`; `FAR_DISTANCE.md` 2
   "Precise corners").** The variant with the define links the C emitter in place of the asm
   (`src/semantic/model_emitter.c` includes the reference copy). Its integer outputs are the asm's: `check_custom.py`
   check 8 runs both on a MIPS interpreter with a GTE model over 8 made-up views of disc meshes (PARIS, FREEWAY) and
   made-up beam / sprite / polygon meshes, about 180,000 bytes of packets, OT links, near list and scratchpad state,
   byte for byte (the check found two t8 differences in the reference copy, now fixed: the light matrix's word 3 kept
   in t8 at a slot change, 800CDD0C, and SXY1 kept when a beam is dropped, 800CE264; both reach the near list's +8
   and a flagged primitive's draw-mode packet). The integer `sh` stores are unchanged (wobble PS1 / smooth). With the
   port's `portPreciseEmit` (`.port_bss`, 0 = off, else the anamorphic x factor in 1/4096) each drawn beam and point
   sprite also puts its four corners into the port's precise ring (0x80C80000, `PORT_PRECISE_PUT`'s format: count,
   then {the vertex word's address in the packet, x and y in 1/16 px, depth}; corners 0..3 in the packet's order):
   - the ends (a beam's two, a sprite's centre) projected exactly: the vertex through the entry's slot and the
     translation with the split's `>> 12` fraction kept (1/256 GTE unit, rounded), divided by the depth unrounded
     (H, OFX, OFY from the GTE); within 0.2 px of the exact projection in check 8 (0.19 worst on 5,046 quads);
   - the beam's direction from the two ends' difference, taken in the display's shape (`L = |(dx, f dy)|`,
     `s = q f (f dy) / L`, `c = q dx / L`; f = 1: retail's `q sin t`, `q cos t`, untruncated);
   - the half width `q = w H / (Z_A 2^k)` (Z_A end A's depth, as retail's SZ1) never clamped to 1, at most 2w / 2^k;
     a sprite's `(w, h) H / (Z 2^k)` at most 500; the x sizes times f; k = (key >> 28) & 3 when the key has bit 30,
     and w, h the source mesh's (a far copy's are rounded w / 2^k: `portFarSpriteSource` reads the copy's new +44);
   - depth `Z_A 2^k` (unscaled GTE units), the same for the four corners.
   Nothing is reported for an end behind the eye or past the GTE's +-1024 px. **The port** sets `portPreciseEmit`
   every frame (`gte_precise_on ? 4096 / k_wide : 0`), adds `TM2_PORT_C_EMITTER` to its defines, and in
   `port_precise_ring` lets these entries past its 4-pixel test where they are meant to differ (the anamorphic x
   sizes, the 2^k sizes of a mesh left uncopied). The ring's cap is the port's 4,000 entries on both sides
   (`PORT_PRECISE_MAX`, `port_c_emitter.h` and the port's `port_hooks.h`): 1,000 quads a frame, which a far view of
   FREEWAY (1,054 level sprites) can pass; the region (0x80C80000..0x80DC0000) has room for 81,919, so raising it is
   one constant on each side.

   Verification: full build `work/build-20261025-180116-645911` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK
   from `.LIB` `f557c1e3…`, unchanged; default port variant `1becd00a…`, changed by the new `portPreciseEmit` word);
   `check_all.py` 27/27 on `work/pv/r61b_{rb,port,allh16}_{P4,P2}` (the port's defines and "all" with
   `TM2_PORT_C_EMITTER`; proofs, placement, check_rollback 9 / 10, check_custom 7 / 8). Port's build (`r61b_port_P4`,
   `185f684c296e…`): `sub_800CDC54` 800CDC54 (0xEA4 bytes, was 0xCBC), `portPreciseBeam` 808E7F10,
   `portPreciseSprite` 808E8228, `portFarSpriteSource` 808D0A80, `portPreciseEmit` 808CEA7C (read them from
   `symbols.tsv`). Negative control: `work/pv/r61b_ce_P4` (the reference C before the t8 fixes) fails check 8.
   **Request 65 (2026-10-25) = this addendum (PR #86).** The port's request 65 asks for the same corners; they are
   built as above, with one difference from its text: the ends are not taken from the GTE's float FIFO but projected
   exactly in fixed point by the C emitter itself (the vertex through the entry's slot, the translation with the
   split's fraction kept to 1/256 GTE unit, divided by the unrounded depth with H, OFX, OFY read from the GTE). For the
   port this means: the corners do not depend on its float GTE (they are the same whatever its precise mode does to
   the FIFO, and the emitter needs no float register access, pure C); they agree with the exact projection within
   0.2 px (check 8), so next to polygons the port draws from its float FIFO a beam or sprite can sit up to about
   0.2 px apart, below its 4-pixel test; the port should take them as given (no second correction from its FIFO).
2. **NCLIP** (the port's GTE): in the precise modes take MAC0's sign from the float FIFO when all three are valid
   (drawing only), or drop a polygon whose float area has the wrong sign though its integer MAC0 = 0.
3. **The range pass:** round the scaled elements (`(m + (1 << (k - 1))) >> k`), or better let the precise path use
   the unscaled slot (record copy -> (slot, k), so the port's GTE computes with `renderMatrixSlots[slot]` / 2^k in
   double).
4. Optional: the non-90-degree type-5 nodes' slot product and translation in float (walker opcode 5, hand-written asm:
   a harder hook, GUESS).

## 5. Car select (Mr. Slam's pole, Mr. Grimm)

### 5.1 The draw path is a battle object's (CONFIRMED)

The carousel's draw `sceneObjDraw` (`scene_init.c:355-385`): `RotMatrixYXZ` (`:369`), `viewProc(0)` (`:375`, view mode
1, H 228), `renderDrawModel` (`engine_anim_hit_volumes.c:318-342`): `renderInitState`, `csUpdMat` into slot 0
(`:327`), `renderWalkModel` (`:328`), the emitter (`:339`), `renderNearPolys`. The same walker and emitter, the same
`csUpdMat` matrix path and the same OT mapping as a battle object (z = the deepest SZ >> 3, compressed above 1,024).
Differences, none of which explains it: one object per call in slot 0; the near threshold 70 even in split screen
(`:338`; the car-select meshes' flags are 0x0002, so nothing goes to the near pass); cull flip 0 (`:337`); node culls
off when `renderNodeCullEnable` is 0 (as a battle's object loop); the emitter called directly (no range pass, no far
LOD: the car is near). **No fixed OT entry** (the meshes' fixed OTZ is 0 and the object's word 0 is 0 for the 3D
items), no other projection, near clip or `renderWorldShift` (CARSEL.DMD's shift is 3, as the arenas').

What the port changes there (port repository): `ui_ViewProc` (`src/ui/options.c:8634`) multiplies H by `f` in split
screen and moves OFX / OFY; `ui_SceneDraw` (`:9044+`) gives each player 1,024 OT entries (`ot + 1024 (n - 1 - p)`)
with a DR_AREA fence per quarter; `vp_emit_fine = 2` (`vplus/precise.c:21-37`) lowers each entry's shift from 3 to 1
(OTZ = SZ >> 1).

### 5.2 Mr. Slam (carousel child 10, node 8003026C)

No transform node, no LOD. The crane's arms are pairs of thin plates 4 units apart (x +-16..20 and +-12..17, 46 long,
y 34..95, z 21..97; meshes 8002FD28 and 8002FF24), all **double-sided** (face word bit 31), several with a
**per-polygon OT bias**, which the emitter adds after the shift for OTZ < 1,024 (`(ctl << 19) >> 26`): 4 end caps at -3
(8002FD28 #8-11), a strip at -2 (8002FF24 #18). Likely causes, in order:

1. **The port's finer sort weakens the bias (LIKELY).** With the shift at 1, a bias of -3 brings a cap forward by 6
   depth units instead of 24, so the caps between the plates swap order with the plates and the body as the car turns.
   Fix: scale the bias by 2^(3 - shift) in `vp_ModelEmit`, or keep shift 3 for polygons with a bias.
2. **Painter's order of two parallel plates (LIKELY):** sorted by their deepest corners 4 units apart, near edge-on
   their depths are within 1-2 units and the visible plate flips. A depth buffer for car select would fix it.
3. **Four players (LIKELY):** H x f makes the 4-unit plates 1-2 PS1 pixels wide; sub-pixel slivers drop out under the
   fill rule: the pole vanishes.
4. GUESS: the tall crane (z up to 127) may cross a player's quarter fence and be clipped.

### 5.3 Mr. Grimm (carousel child 3, node 800272D8)

Nothing Grimm-specific: no transform node, an LOD [0, 22,500) only on the 6-quad base (always in range), 132 of 137
polygons share the bias -1 (no relative effect). It has one **beam** (w 6, len 7: a headlight) drawn by the integer
beam builder of 4.2 A (its width and tilt step as the bike turns), and the smallest body of the cars (radius 59), so
the remaining translation rounding shows most: `csUpdMat`'s `mathMulMatVec` truncates it by up to 1 unit (about 0.85
PS1 pixel at 270 units' depth), and the port's TR fit (`gte_tr_smooth`) covers only that part (GUESS that these are
the visible cause).

**Hooks for car select:** the bias scaling in `vp_ModelEmit`, the beam branch (4.3 item 1), and a per-player depth
or a larger minimum on-screen size in the 4-player quarters.

## 6. Verification

Retail builds byte-identical; the proofs, placement checks and static checks as in `BALANCE.md` 14.3:

Request 45 (2026-10-17), E0 = `work/build-20261016-210450-578481/source_reloc_e0`; every row by `pv.py` (P4 and the
placement link P2, `verify_reloc.py` against E0) and `tools/check_all.py work/pv/r45_<tag> --e0 <E0>`: both proofs
and the placement check pass; `check_balance.py --expect-functions` on the three builds with `TM2_PORT_BALANCE`
(exactly the 20 functions with the define alone; G10 with the per-car source / header check, G19 with the
pass-through), `--negative-control` on the build without it fails as it must; G19 fails on request 42's link
(`work/pv/r42_allh16_P4`: no `portBalanceRicochetPass`) and G10 / G19's source checks fail on the previous sources
(negative controls); `check_custom.py --expect-functions` on the builds with `TM2_PORT_VISUAL` (bit 2:
`portVisualOwnCar`'s callees and caller, `renderScene` changed; with `TM2_PORT_VISUAL` alone exactly `weaponArcsDraw`
and `renderScene`), its `--negative-control` on the build without it; `check_car_groups.py`, `check_hazards.py`,
`check_humans.py` on the builds they apply to. Car stack as in 10.2; all = the car stack + `TM2_PORT_FAR_LOD
TM2_PORT_FAR_DISTANCE TM2_PORT_SPU48 TM2_PORT_HAZARDS TM2_PORT_HUMANS=16 TM2_PORT_BALANCE TM2_PORT_VISUAL
TM2_PORT_CAR_NUMBERS`.

| Defines | P4 sha | P2 (placement) sha | port end (P4) |
|---|---|---|---|
| none | `c1108ee81c3d` | `b95e6fed7439` (as before) | 80545710 |
| `TM2_PORT_BALANCE` | `8a4b45332f79` | `de9008d8921e` | 80546F10 |
| `TM2_PORT_VISUAL` | `dabe1cf7a13a` | `e1a97e4531a7` | 80545840 |
| car stack + `TM2_PORT_BALANCE` + `TM2_PORT_VISUAL` | `e4360fbbef2b` | `a7332a8ceb83` | 8059A3B8 |
| all + `TM2_PORT_HUMANS=16` | `bbf38feadade` | `9e51263e0645` | 805A4F7C |

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261017-220541-042954`):
hybrid `9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB`
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `c1108ee81c3d…` (unchanged from request 42: no code without the defines; placement check OK);
`tools/check_all.py` on it passes.

# Request 46 (the user's second playtest of 2026-10-17)

The port's copy of the notes is `docs/PLAYTEST_2026-10-17b.md` (item 6, and item 2's beams). Request 46 asked two
things: item 1 changed code (`TM2_PORT_DARK_TOOTH`), item 2 is an answer (the corner stores). Same sources and tags as
above; file:line refers to branch `decomp-requests-46`. Nothing was run in an emulator.

| # | Item | Result |
|---|---|---|
| 46-1 | A Dark Tooth head frozen under the bridge (Survival, Los Angeles, not his round) | the head is the only boss part made **shown**, as a scene object of its own at the **world origin**, and only a live Dark Tooth's boss update moves it; the port's Survival puts him out of play before the first frame, so it stood at (0, 0, 0), drawn **and in the hit pass**. **Built:** every Dark Tooth car nobody drives has its head hidden each frame (not drawn, no collision) and shown again over the truck when a computer record drives it (section 7). Retail never shows it: BB.DMD is loaded only for level 8's boss, who drives from the first frame |
| 46-2 | Integer beam / sprite corners | the 8 halfword stores of each branch, at `packet + 8 + i x stride` (stride 4 / 8 / 12 by the face word); tables in section 8 |

## 7. The Dark Tooth head frozen under the bridge (request 46-1)

### 7.1 Which parts exist, and what each does when the car is loaded but not driven (CONFIRMED)

BB.DMD (`SURVIVAL.md` 1.1) gives `worldLoadObject` (`world_load_object.c:98-165`) these parts:

| Part | What it is | Drawn by | At load | When the car is not driven |
|---|---|---|---|---|
| truck (240) | the car's world object (`carRegisterModel`) | its own scene object | hidden: `carRegisterModel` leaves it off until a record binds (`ai_route_helpers.c:360`, `csSetDrawEnable(.., 0)`) | hidden by every park path (`portCarParkSlot`, `portCarParkAi`, the spares' `portCarSpecialLoadDone`; the port's own out-of-play writes `csSetDrawEnable(obj, 0)` too) |
| wheels (242, `carPartRegisterDarkTooth`, `arena_load_register.c:613`) | type-5 nodes of the truck's model (`portCarGroups.parts[key]`) | inside the truck's model | with the truck | with the truck |
| turbo (246), CLUT animation (248) | a node / the CLUT fades of the truck's model | inside the truck's model | with the truck | with the truck |
| body (243) | **none for Dark Tooth**: `carBodyObjectRegister` keeps retail's `id < 15` bound (0xF3 >> 4 = 15) | - | - | - |
| **head (249)** | `weaponAssetBind` case 0xF9 (`asset_pools.c:120-124`): `csAlloc` + `csListAdd`, model = the node, **word +8 = 1** | **a scene object of its own** | **shown**, at the **world origin** | **left shown where it stands** (before this change, unless a park function hid it) |
| freeze block (247) | `carFreezeBuild(247, worldLoadParentObject)`: a switch node linked into the **head's** model (the parent is 249: case 240 does not set it) | inside the head's model | off until he is frozen | with the head |
| ricochet model (2214), spiral missile (2301) | models only (`weaponRicochetModelAlt`, `weaponSpiralMissileModel`) | the projectiles | - | - |

**Where the head stands.** `csAlloc` -> `csReset` (`engine_clut_anim.c:585-618`) zeroes the position, the angles and
the matrices' translations and sets word +8 = 1 (shown) and word +0xC = 1 (collides); `weaponAssetBind` sets the model
and +8 = 1 again and no position, and nothing in the loader places it (the node's own offsets are model-local). So the
head stands at **(0, 0, 0) of the world** until a boss update moves it. Its only movers are `aiDarkToothBossUpdate`
(`ai_dark_tooth_boss.c:118-128`, the hover over the truck) and request 36's player head code (`portCarDtPlayerHead`).
The boss update is called by `aiWeaponFireDecide` (`weapon_tick_init.c:104-116`) from `aiWeaponsUpdate`, which
`aiCarUpdate` runs only for a started, live record (`ai_driver.c:78-85`: `f000 && !dead`), and `aiCarUpdate` itself is
skipped in wreck state 4 (`aiCarShouldUpdate`, `fx_explosions.c:217-224`). On Los Angeles the origin is under the
bridge (the user's sighting; LIKELY, not checked against the level's geometry).

**The cases** (before this change):

| Case | Head |
|---|---|
| the port's Survival, a round before his (`vplus/fixes.c` `vp_survival_start` -> `ai_out_of_play`: HP 0, dead, wreck state 4, out of range, far below, `csSetDrawEnable(car, 0)`), and the custom tournament's waiting bosses (`boss_round_start`, the same function) | **shown at the origin, all match**: the out-of-play writes come before the first frame, the boss update never runs, nothing hides the head. **This is the sighting** |
| `portCarParkAi` (request 32 step 3, `SURVIVAL.md` 3.1) | hidden |
| `portCarParkSlot` (drop-in loads, `portCarRemove`, the car `portCarSetGroup` leaves), `portCarModelOn` of a parked record | hidden |
| request 35's spares (`portCarSpecialLoadDone`, slot kind 2) | would stand at the origin, but Dark Tooth is no spare type (`TM2_PORT_SPECIAL_LOAD_FILES` 0x2B22 has no bit 15) |
| the ordinary Dark Tooth (bit 10) after his death | hidden by `portCarDarkToothWreck`; shown with his car by `portCarDarkToothHeadShow` |
| a bit-6 boss's or the level-8 boss's final death | hidden by `carApplyDamage` (the head's burst) |
| a boss's stage-1 death (wreck state 3, waiting for stage 2) | shown where it was (retail's wait; the stage-2 hover brings it down onto the car) |
| a player's Dark Tooth (request 36) | `portCarDtPlayerHead` shows and hides it each update (dead, cloaked, parked) |

**The other cars' parts** (checked for the same fault: made shown, or left shown by a park):

| Part | Made | Parked |
|---|---|---|
| Twister's tornado (0x913) and funnel (0xA0) | hidden (`asset_pools.c:98-110`; the funnel also +0xC = 0) | hidden by `portCarResetSpecials` (in every park path that can run during a special: `portCarParkAi`, `portCarRemove`, `portCarSetGroup`) and by every round reset (`weaponsRoundReset`, `portCarSpecialsRoundReset`) |
| Shadow's strike object (0x915), request 35's as-strike | hidden (`PROJECTILE`, +0xC = 1) | hidden by `portCarResetSpecials` and the round resets |
| Axel's ring (request 35's as-entry object, `portCarSpecialAsObject`) | hidden, +0xC = 0 | hidden by the round reset and `portCarResetSpecials` |
| Mr. Slam's arm and claw (`slamArmObject`, `slamClawObject`) | nodes of his car's model | with the car; `portCarResetSpecials` puts them back to rest |
| Axel's, Mr. Slam's, Minion's, Hammerhead's parts (`carPartRegister*`) | nodes of the car's model | with the car |
| turbo flames, freeze blocks, damage stages | nodes / switches of the car's model (Dark Tooth's freeze block: of the head's) | with the car (Dark Tooth's: with the head) |
| body objects (`carBodyObjects`, ids 0x13..0xE3: world models drawn in the level's frame, LIKELY the shadow) | added by `carBodyObjectShow` only | `carBodyObjectHide` in every park function; the port's out-of-play writes come before the first `aiCarDamageUpdate` that would show it (wreck state 4 and HP 0 show nothing), and `ai_respawn` shows it itself (LIKELY: not run) |

So the head is the one part shown without a driver: every other separate object starts hidden and is shown only by
its car's running special, which every park path and every round start ends.

### 7.2 Retail (CONFIRMED by the load code; not a retail bug)

Retail loads a car file only for a car of the match (the settings' humans and opponents, `sessionLoadArenaAndCars`),
and Dark Tooth (index 14) only as tournament level 8's boss (`sessionRun`: opponent slot 0 = 14, opponents = 1), never
from a car select (`carIsSelectable`: no index 14 on retail's carousels) or Challenge's picker. On level 8 he is
`aiCars[0]` and drives from the first frame, and `aiWeaponFireDecide` runs his boss update every update, so the head
is over the truck (it starts at the origin and blends there at 40 per update in the first updates: LIKELY a few
frames). A loaded Dark Tooth who is not driving does not happen in retail, so his head is never left standing; were
his file loaded without him (a mod), it would stand at the origin, shown and solid (`SURVIVAL.md` 1.5 #1). No
`RETAIL_BUGS.md` entry: nothing in retail reaches it.

### 7.3 Collision (CONFIRMED)

`hdpPassRun` (8014FAB0, hand-written asm in `engine_hit_detection.c`) walks the moving object's own hierarchy, then
every object of the scene list, and skips one when `(word +8 & word +0xC) == 0` (8014FC50..8014FC64: `lw` +8, `lw`
+0xC, `and`, `beq`) and the mover itself. The head has +8 = 1 and +0xC = 1 (`csReset`; `weaponAssetBind` leaves +0xC),
so **a shown head is in every hit pass** (cars, projectiles; the swoop's own `hdpQueryPoint` passes the head's id to
skip it): the frozen head under the bridge could be hit (through its model's hit nodes, LIKELY). **Hidden (+8 = 0) it
is out of the pass**: `0 & 1 = 0`. The same holds for every part above (a hidden tornado or strike object, +0xC = 1,
is skipped; the funnel and Axel's ring never collide, +0xC = 0) and for a parked car object (`csSetDrawEnable(obj, 0)`
writes +8). So hiding is enough: the change hides with +8 and leaves +0xC as `csReset` made it (shown again = solid
again, as retail's head).

### 7.4 Built: the head hidden with its car (`TM2_PORT_DARK_TOOTH`)

- **`portCarDarkToothParked()`** (new, `.port_text`), called first by `aiCarsCountRemaining` (`scene_init.c`, its
  `TM2_PORT_DARK_TOOTH` form; `worldUpdate` calls it every frame after the car updates, before the scene is drawn).
  For every car slot of type 0xF0 (car slots 0..63):
  - slot kind 2 (parked, spare, vacant, never bound) or an AI record dead in wreck state 4 (out of play:
    `portCarParkAi`, the port's `ai_out_of_play`, an ordinary or bit-6 Dark Tooth's final death): **head hidden**
    (`portCarDarkToothParkHead`) and remembered in `portCarDtParkMask`;
  - a remembered slot whose AI record drives again (started: `aiCarStart` from the port's `ai_respawn`,
    `portCarStartAi`, `portCarSetGroup`): **head shown again**, placed at its hover over the truck (the boss update's
    point: `rot_m x (0, 0, 240)` + the record's position, -24 in stage 2), the swoop cleared, so it does not fly in
    from where it stood; the boss update moves it from there;
  - a player's slot (kind 0): the bit cleared, the head left to request 36's `portCarDtPlayerHead`.
  Each slot's head is read inside its key's selection (33-2e, `portCarSpecialSelect`): every Dark Tooth's own head.
- **`portCarDarkToothParkHead(slot)`** (new): the head of slot `slot`'s key hidden (+8 = 0: not drawn, out of the hit
  pass) and the slot's bit set. With `TM2_PORT_CAR_SWAP` the park functions use it (`DARK_TOOTH_PARK_HEAD`):
  `portCarParkSlot`, `portCarParkAi` and `portCarModelOn` (a parked record), so a car parked there and started by any
  path (the port's own `aiCarStart` too) gets its head back.
- **`portCarDtParkMask[2]`** (new, 8 bytes, `.port_bss` in every variant after `portCarSel`;
  `config/US/port_variant_arrays.tsv`), cleared each load (`portCarSpecialsReset`).
- With no Dark Tooth that nobody drives every test is false: retail's logic (retail's level-8 boss is never in wreck
  state 4: `portCarDarkToothWreck` keeps his 3).
- Game functions changed: none new (`aiCarsCountRemaining` is in `EXPECTED_DARK_TOOTH` already). Port functions: 2
  new; `portCarParkSlot`, `portCarParkAi`, `portCarModelOn` and `portCarSpecialsReset` changed.
- `tools/check_car_groups.py` **check 16** (builds with `TM2_PORT_DARK_TOOTH`): `aiCarsCountRemaining` calls
  `portCarDarkToothParked`, which calls `portCarDarkToothParkHead` and selects; with `TM2_PORT_CAR_SWAP` the park
  paths (`portCarParkSlot`, `portCarParkAi`, `portCarModelOn`) hide through `portCarDarkToothParkHead`;
  `portCarDtParkMask` has only its three users. Check 8 lists both new functions as selecting. Negative control: on
  request 45's link (`work/pv/r45_carsbalv_P4`: the define, without this change) check 16 fails on all three lines.

**For the port:** nothing to call. The Survival flow (`ai_out_of_play` / `ai_respawn`) works as it is: his head is
hidden from the first frame and comes back over him when `ai_respawn` starts him. (The port could use `portCarParkAi`
/ `portCarStartAi` instead of its own writes; both are covered.) A build without `TM2_PORT_DARK_TOOTH` has no hook:
the port's Survival with Dark Tooth needs the define anyway (bit 6).

## 8. The beam and sprite corners (request 46-2, CONFIRMED)

Both branches of `sub_800CDC54` (`model_emitter.c`, hand-written asm; the C form
`reference/model_emitter_800CDC54.c:305-357`) write the four corners into the primitive's packet with `sh`, through
`$11 = $10 + 8` (800CDF24: the packet `$10` + 8 = its first `xy`) and the stride `$12 = ((face word >> 22) & 3) x 4`
(800CDF30..800CDF44: the packet's vertex size). Vertex i's x is at `packet + 8 + i x stride`, its y at `+ 2`:

| Packet (stride) | x0, y0 | x1, y1 | x2, y2 | x3, y3 |
|---|---|---|---|---|
| `POLY_F4` (4) | +8, +10 | +12, +14 | +16, +18 | +20, +22 |
| `POLY_FT4`, `POLY_G4` (8) | +8, +10 | +16, +18 | +24, +26 | +32, +34 |
| `POLY_GT4` (12) | +8, +10 | +20, +22 | +32, +34 | +44, +46 |

**Beam (type 1, 800CE210..800CE438).** RTPT of (x, y, z + len), (x, y, z - len), (x, y, z - len) (800CE258): end A =
SXY0 (`xA`, `yA`, the upper end), end B = SXY1 (`xB`, `yB`); `t = ratan2(yA - yB, xA - xB)` (800CE350); half-width
`q = w x H / SZ1` (800CE378; w = prim +6, H = `cfc2 $26`, SZ1 = end A's depth), clamped to 1..2w (800CE384..800CE39C);
`s = q x rsin(t) / 4096`, `c = q x rcos(t) / 4096` (truncated toward 0, 800CE3A8..800CE3D4). The stores:

| Corner | Value | x store | y store | offset (x, y) |
|---|---|---|---|---|
| 0 | (xA + s, yA - c) | 800CE3DC `sh $17,0($11)` | 800CE3E4 `sh $17,2($11)` | +8, +10 |
| 1 | (xA - s, yA + c) | 800CE3F0 `sh $17,0($18)` | 800CE3F8 `sh $17,2($18)` | +8 + stride, +10 + stride |
| 2 | (xB + s, yB - c) | 800CE404 `sh $17,0($18)` | 800CE40C `sh $17,2($18)` | +8 + 2 stride, +10 + 2 stride |
| 3 | (xB - s, yB + c) | 800CE418 `sh $17,0($18)` | 800CE420 `sh $17,2($18)` | +8 + 3 stride, +10 + 3 stride |

(`$18` = `$11` + stride at 800CE3E8, + stride at 800CE3FC and 800CE410.) Then 800CE424..800CE438 reload the spilled
registers and jump to the shared colour / OT tail at 800CE5C4.

**Point sprite (the other types, 800CE43C..800CE5B0).** RTPS of the vertex (800CE454); SXY2 kept at scratchpad 0x54
(800CE464 `swc2 $14,84($8)`), d = SZ3 (800CE458; d = 0 drops it); half sizes `hw = (prim +6) x H / d` and
`hh = (prim +8) x H / d` (800CE50C, 800CE52C), each clamped to 1..500 (800CE534..800CE560); `x = SX - hw`,
`y = SY - hh` (800CE564..800CE574, from the 0x54 halfwords):

| Corner | Value | x store | y store | offset (x, y) |
|---|---|---|---|---|
| 0 | (SX - hw, SY - hh) | 800CE570 `sh $19,0($11)` | 800CE578 `sh $20,2($11)` | +8, +10 |
| 1 | (SX + hw, SY - hh) | 800CE58C `sh $17,0($2)` | 800CE590 `sh $20,2($2)` | +8 + stride, +10 + stride |
| 2 | (SX - hw, SY + hh) | 800CE59C `sh $19,0($2)` | 800CE5A0 `sh $18,2($2)` | +8 + 2 stride, +10 + 2 stride |
| 3 | (SX + hw, SY + hh) | 800CE5A8 `sh $17,0($2)` | 800CE5B0 `sh $18,2($2)` (the `j`'s delay slot) | +8 + 3 stride, +10 + 3 stride |

(`$2` = `$11` + stride at 800CE588, + stride at 800CE594 and 800CE5A4; the far side is `(SX - hw) + 2 hw`, 800CE57C..
800CE584 and 800CE598.) The sprite's corner order is 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right (a quad
strip); the beam's 0 / 1 are the two sides at end A, 2 / 3 at end B.

**For the port's precise places** (as `PORT_PRECISE_PUT`): the packet is `$10` in both branches (the store base
minus 8), the stride from the face word. After the last store (800CE420 for the beam, 800CE5B0 for the sprite, both
before the tail at 800CE5C4) the port can record or overwrite the four corners: the beam's from the float ends (RTPT's
float FIFO), the direction from their float difference and `w x H / (SZ1 x 2^k)` unclamped; the sprite's from the
float SXY2 and the float half sizes `(+6, +8) x H / (d x 2^k)` (k as in section 4.3). Nothing after the stores reads
the corners back (the tail writes the colours, the UVs, the tag and the OT link from other registers, and a
semi-transparent face's two draw-mode packets after the primitive, 800CE7BC.. / 800CE83C..).

## 9. Verification (request 46)

E0 = `work/build-20261016-210450-578481/source_reloc_e0`; every row by `pv.py` (P4 and the placement link P2,
`verify_reloc.py` against E0) and `tools/check_all.py work/pv/r46_<tag> --e0 <E0>`: both proofs and the placement
check pass; `check_car_groups.py --expect-functions` on the builds with `TM2_PORT_DARK_TOOTH` (check 16 included; the
port_changed set is unchanged: 64 with groups + specials + Dark Tooth); check 16's negative control on request 45's
link (`work/pv/r45_carsbalv_P4`) fails on all three lines. Car stack and "all" as in section 6.

| Defines | P4 sha | P2 (placement) sha | port end (P4) |
|---|---|---|---|
| none | `c1108ee81c3d` (unchanged) | `b95e6fed7439` (unchanged) | 80545710 |
| `TM2_PORT_CAR_GROUPS` | `87909b185c5f` | `c8f26f8321c9` | 80545C10 |
| `TM2_PORT_CAR_GROUPS TM2_PORT_CAR_SPECIALS TM2_PORT_DARK_TOOTH` | `069cbbb75c4f` | `0883ce9d67ee` | 80548738 |
| the same + `TM2_PORT_CAR_SWAP` | `85c331aea4e5` | `db32d05969c2` | 8054A138 |
| car stack + `TM2_PORT_BALANCE` + `TM2_PORT_VISUAL` | `58bc7b0328b8` | `5230039da441` | 8059A6C8 |
| all + `TM2_PORT_HUMANS=16` | `417d9ca74389` | `f20e2f470fcb` | 805A529C |

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261018-061725-740336`):
hybrid `9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB`
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `c1108ee81c3d…` (unchanged: no code without the defines; the 8-byte `portCarDtParkMask`
fits the `.port_bss` alignment, port end 80545710 as before; placement check OK); `tools/check_all.py` on it passes;
`tools/make_package.py` written.


# Request 46 items 3-10 (the user's third playtest, 2026-10-18)

The port's notes are `docs/PLAYTEST_2026-10-18.md` (items 2-4 and 6-10) and `PLAYTEST_2026-10-17b.md`. Same tags as
above; file:line refers to branch `decomp-requests-46b`. Nothing was run in an emulator.

| # | Item | Result |
|---|---|---|
| 46-3 | Twister's shield during her tornado | **built** in bit 15 (`TM2_PORT_BALANCE`): the shield (energy attack 103) is let through, the rest stays locked; the old form locked it too (section 10, `BALANCE.md` 15) |
| 46-4 | Machine-gun bullets start behind a fast car | the bullet starts at **this step's** muzzle, but its streak is first drawn after one more bulletsMove and one more car step: its tail (the shot point) is then one car step behind the muzzle (80 units for Twister, up to 150 with turbo on hard). **Built:** `portVisual` bit 4 draws the first streak's tail from where the shooter's muzzle is now (section 11) |
| 46-5 | Sweet Tooth's shadow over his rear | every car's shadow mesh carries an OT bias of -2..-7 entries in the data (Sweet Tooth -5 / -7), and its polygons sort by their own farthest corner: the strip under his rear sorts in front of the low rear panels. **Built:** `portVisual` bit 3 sorts every car's shadow one entry behind its own car's farthest polygon (section 12) |
| 46-6 | Hong Kong's buildings blurry until close | yes: LOD bands with **lower-resolution textures** (the near bands use 2.4 times the texel density, median, and the detail pages; the switch at 200..600 units). **Built:** `portFarLodNearScale` (`TM2_PORT_FAR_LOD`): every band 2..16 times farther (section 13, `FAR_LOD.md` 8) |
| 46-7 | Cyburbia, 16 cars: sky and ground gray | **not the decomp's VRAM placement:** a replay of the load with 400 random 16-car mixes (retail scans and bit 9, banks off) never puts a car image or CLUT on one of Cyburbia's 249 arena rectangles (nor on any arena's); with banks every car is in its own bank. The other VRAM writers of a match do not touch them either. Leads for the port in section 14 |
| 46-8 | Cyburbia: computer cars driving a short loop | the port's route is complete (127 points, every point reaches every other, every table entry valid); the loop is the route walk's: a target **off the route** makes the car shuttle around the route point nearest it (A-B-A-B-C-B, 180-degree turns), 23 % of the modelled walks; a rule for the port (it picks the targets) in section 16; no code (section 16) |
| 46-9 | Antarctica pop-in, the mirror | per view: mesh queue and matrix slots per view, the packet buffer shared (the mirror gets what view 0 leaves), the mirror's far distances fixed at 500 / 1000 (it ignores `portFarDistance`). **Built:** `portMirrorFar` (`TM2_PORT_FAR_DISTANCE`). The picture moving in its frame: the frame and the mirror's viewport must move together (section 15) |
| 46-10 | Dark Tooth's head under the bridge, again | the fix is PR #66's (`portCarDarkToothParked`); the port's `ai_out_of_play` (HP 0, dead, wreck state 4) is covered by its frame hook (section 17) |

## 10. Twister's shield during her tornado (request 46-3)

**Built** (`TM2_PORT_BALANCE`, bit 15 changed, `BALANCE.md` 15): `weaponFireEnergyAttack` (801046FC,
`weapon_special.c:99-103`) tests `portBalanceEnergyLocked(car, human, kind)` (new, `port_variant_arrays.c`, beside
`portBalanceLocked`), which returns 0 for the shield (`kind` 103, `TM2_BAL_LOCK_FREE_KIND` in `port_balance.h`) and
`portBalanceLocked(car, human)` otherwise. So while her tornado runs a Twister (human or computer) may cast her shield
as retail does (400 energy, a shield of life 60, the cooldown; bit 1's "already shielded" refusal still applies);
every pickup weapon, her special, a remote bomb's detonation, the machine guns and the other energy attacks stay
locked. The old behaviour (request 41: the shield locked too) is what a build before this one does; there is no bit
for it now. `tools/check_balance.py` G16 checks the new gate (domination, the refused edge, the immediate 103 in its
code, the source form); `rand()`: none.

## 11. Machine-gun bullets starting behind a fast car (request 46-4)

### 11.1 Where the start and the streak's points come from (CONFIRMED)

- **The muzzle.** `UpdateGuns` (80104F70, `update_guns.c`) takes the stats' gun offset (+216/+218/+220: x mirrored
  on alternate shots, forward y, z; Twister (A0) x 9, y 37, z 8; `CAR_STATS.md`), turns it with `motion.rot_m` and adds
  `motion.pos`; the direction from `motion.rot` and the aim. It runs from `weaponTick` (`car_tick.c:153`, humans;
  `ai_steps.c:1781` / `weapon_helpers.c:72`, the AI), **after** `CarRotUpdate` and `carMoveByVelocity`
  (`car_tick.c:141-142`): the position and matrix are **this step's**.
- **The bullet.** `bulletSpawn` (800D7A4C, `weapon_bullets.c:201`) sets `pos = prev = start` and `vel` = 200 units a
  step on the direction's largest axis (no share of the car's own speed).
- **The order of a frame** (`rtMainLoop`, `rt_main.c:139-185`): view 0's `streaksDraw`, then `bulletsUpdate` ->
  `bulletsMove` (`prev = pos; pos += vel`, the hit test), view 0's scene, then `worldUpdate` (the cars' step: their
  shots spawn here), then view 1 / the mirror (`streaksDraw` again).
- **The streak** (`streaksDraw` 800D8230, `screen_card_draw.c`) is drawn from the record's `pos` (+0x00, the head) to
  its `prev` (+0x0C, the tail).

So a bullet fired in step N is first drawn in frame N+1 as a zero-length streak at the muzzle of step N (the car is
drawn at step N too: right), and in frame N+2 from `start + vel` back to `start`, while the car is drawn at step N+1:
**the tail lags the muzzle by one car step**. Twister (top speed 141 "mph": `speed_cap` / 32 = 141 x 0.57 = 80
world units a step at 20 frames a second; turbo x 4/3: 107; a hard computer car x 1.4: 112, 150 with turbo) has her
muzzle 37 ahead of her centre and her rear axle 70 behind it: the first streak starts 43..113 units behind her centre,
at or behind her rear. From the second moved frame the tail (`start + vel`) is ahead of the muzzle again (the bullet
gains 200 - 80 a step). Retail (`RETAIL_BUGS.md` 33); a slow car shows no lag.

### 11.2 Built: `portVisual` bit 4 (`TM2_VIS_GUN_START` 0x10, `TM2_PORT_VISUAL`)

- `bulletSpawn` calls `portVisualBulletSpawn(slot)` (new, `.port_text`) at its end: it notes the shooter's
  `motion.pos` (the human whose `TM2_HUMAN_ID` is the owner id, or AI record id - 50), that position now, and the
  bullet's starting life (12 or 15), in `portVisualState.bullets[slot]` (drawing state only; always recorded).
- `streaksDraw` takes the tail through `portVisualBulletTail(i, e->p1)` (new; `STREAK_TAIL`, whose retail form is
  `e->p1`): with bit 4 set and the bullet in its **first moved frame** (life one below its start), the tail is the
  shot point moved by the shooter's own move since the shot (`pos now - pos at the shot`): the streak starts at the
  muzzle where the car is now (the car's turn is not followed: the muzzle's sideways swing is small). Otherwise the
  record's own point. The bullet's hit test, impacts and damage are retail's (`bulletsMove` unchanged); the head is
  retail's.
- Views: view 0 draws before `bulletsMove`, view 1 / the mirror after `worldUpdate`; both read the shooter's current
  position, so each view's tail is at its own drawn car's muzzle.
- **For the port's in-between frames:** if the port interpolates a streak between two steps, take the tail of the
  first moved frame from the same rule (or call `portVisualBulletTail` per drawn step).
- Two game functions change (`bulletSpawn`, `streaksDraw`); `tools/check_custom.py` check 2 (request 46): the callers,
  no `rand()`, the retail call census of both against E0, the macro's retail form.

## 12. Sweet Tooth's shadow over his car (request 46-5)

### 12.1 How a shadow is drawn (CONFIRMED)

- The shadow is the car file's object `type + 3` (node 3, `BALANCE.md` 13.5), a transform node (opcode 5) kept as the
  key's body object (`carBodyObjectRegister`, `misc_one_liners.c`; `portCarGroups.body` per key) and posed each step by
  `carBodyPoseUpdate` (its matrix: the ground under the wheels). It is drawn as a **world model**
  (`carBodyObjectShow` -> `renderAddWorldModel`, `ai_route_helpers.c:139-155`): `renderScene` walks it with the level
  root's matrix in its world-model pass (`engine_render_scene.c:240-250`), before the scene list where the car is.
- Its mesh is a flat semi-transparent blob (GP0 0x2E, FT4) of 6 quads (Sweet Tooth: x -34..34, y -87..62, z 0, two
  columns x three rows).
- The emitter (`sub_800CDC54`, `reference/model_emitter_800CDC54.c:292-300`) sorts every polygon by its **deepest
  vertex** (SZ >> shift, compressed past 1,024) plus, below 1,024, the **signed bias** in its control word (bits 7..12).
  The disc's shadows all carry negative biases (in front), the car bodies 0:

| Car | shadow quads | bias (entries) |
|---|---|---|
| Sweet Tooth (ST) | 6 | -5 x 4, **-7** x 2 (the rear strip, y -87..-38) |
| Hammerhead, Outlaw 2, Spectre, Roadkill, Thumper | 6..8 | -5 / -3 |
| Warthog, Grasshopper, Dark Tooth, Mr. Slam | 6..10 | -5, -7 |
| Mr. Grimm, Twister, Axel, Shadow, Minion | 8..10 | -2..-6 |

  (the bias exists so that the blob wins over the road polygons under the car, whose farthest corner may be near.)
- Sweet Tooth's rear panels (the clown face) are tall quads whose farthest vertex is high or forward; seen from
  behind and a little above, the shadow's rear strip (its deepest corner under his body, minus 7 entries) sorts **in
  front of** the face's lower panels, and being drawn later it darkens them where it projects over them: the bottom
  half of the face. The face's upper panels sort nearer still and stay clean. Retail draws the same
  (`RETAIL_BUGS.md` 32).

### 12.2 Built: `portVisual` bit 3 (`TM2_VIS_SHADOW` 0x8, `TM2_PORT_VISUAL`)

- **Where:** `renderScene`, the cleanest place: it is the only code that knows both which draw entries are the
  shadow's and which are its car's. Each walk of a world model or a scene object goes through `RENDER_WALK` (retail
  form: the plain `renderWalkModel`), which hands the port the draw entries it queued (`portVisualWalked(obj, model,
  first)`: the walker leaves its queue pointer at ctx+0x20); a walk of a car's body object is that key's shadow, a
  walk of a car's scene object (a human's `humanCarObjects` or an AI's `aiCarObjects`) is that key's car (key >> 4,
  `carBodyPoseUpdate`'s index). The own car walked first (bit 2) counts too.
- **Before the emitter** `portVisualShadows()` (both emitter paths, `TM2_PORT_FAR_DISTANCE`'s too): for each key with
  both, the car's farthest OT entry is computed **as the emitter computes it** (per primitive: the deepest vertex's SZ
  from the entry's rotation slot and translation, the translation split as the emitter splits it, >> shift,
  compressed, the bias; a mesh's fixed entry or the object's override word when set), and every shadow draw entry gets
  the entry `max(car's farthest + 1, the shadow's own farthest)` (1..4094) through the emitter's own override: the
  entry's object pointer (word 5, whose words 0 / 1 the emitter reads as the OT override and the light record) is
  replaced by a pair {entry, light record} in `portVisualState.pairs`. The shadow's polygons then share one OT entry
  behind every polygon of its car (a larger entry is drawn earlier), so the car covers it. Then the view's ranges are
  cleared.
- **Not changed:** a shadow whose car was not walked this view (culled, hidden), the arena, other objects, the hit
  pass; the shadow's own primitives and the model data (nothing is written into the models).
- **Trade-off:** the shadow is now sorted at its car's far end instead of its own (minus 2..7). A road polygon under
  the car whose farthest corner is nearer than the car's far end can cover part of the shadow (retail's bias made it
  win there). On the arenas' large ground polygons this is rare; if the port sees it, a smaller rule is possible
  (only the shadow quads that overlap the car's screen box), at more cost.
- **Cost:** one pass over the car's queued meshes per car with a shadow, per view (the car's vertices through one row
  of its matrix); no `rand()`, no game state.
- Game function changed: `renderScene` (already a `TM2_PORT_VISUAL` gate). `tools/check_custom.py` check 2: the four
  new port functions' callers and no `rand()`, three `portVisualWalked` calls right after the object / world-model /
  own-car walks (not the level root's or the sky's), one `portVisualShadows` after every walk and before the emitter,
  the override written only under the bit, `RENDER_WALK`'s retail form.

## 13. Hong Kong's buildings blurry until close (request 46-6)

### 13.1 LOD bands with lower-resolution textures (CONFIRMED, the disc's HKONG.DMD)

Every multi-band LOD node of the level (171 of its 299 LOD nodes) was measured per record (texels per model unit along
the polygons' edges, median, and the texture pages):

- A Hong Kong building is typically `[0, 300)` 78 textured quads at **0.29 texels a unit** on pages 141 / 144 / 145 /
  147, then `[300, 10000)` 12 quads at **0.10** on pages 144 / 145 only (`8006AD48` and its copies), or `[0, 600)` /
  `[600, 1000)` (0.25 / 0.10), `[0, 200)` / `[200, 10000)` (0.31 / 0.08). The far band is a box with one low-detail
  picture per side; the near band has the window and sign details on their own pages (141, 147).
- Over the 171 nodes the near band's density is **2.4 times** the far band's (median; quartiles 1.5 / 4.0). So it is
  not a texture page swap of the same geometry: it is the model's LOD design (the switch at 200..600 units, a quarter
  of a block, as the user saw). Other arenas: Freeway 2.5, Paris 4.0, Antarctica 4.3, Amazonia 4.0, Roof Tops 8.1,
  Swamp 2.1; New York, Moscow and Cyburbia 1.0 (their bands differ in geometry only).

### 13.2 Built: `portFarLodNearScale` (`TM2_PORT_FAR_LOD`, `FAR_LOD.md` 8)

- `portFarLodNearScale` (new `int`, `.port_bss` in every variant): 0 / 1 = retail; 2..16 = while `renderScene` walks a
  view, **every** LOD record's band is at that many times its distance (both bounds: d2 x s x s; an unbounded max
  stays unbounded), so each switch happens s times farther: the sharp band of a Hong Kong building is kept to 600
  (s = 2) or 1,200 (s = 4) instead of 300. A split set (bands in sibling nodes) keeps its order, as all its records
  scale alike. Read live; with `portFarLod` set too, the farthest bands are also extended as before.
- The scan (`portFarLodScanLevel`, `portFarLodScan`) lists every record with its retail band (`portFarLodNear`, 4,096
  records: a level has at most 1,582, a car file 33); `portFarLodBegin` scales the ones still holding the band seen
  at the scan, `portFarLodEnd` puts them back before anything else reads them (hit detection keeps retail's bands, as
  in section 4 of `FAR_LOD.md`). With `TM2_PORT_CAR_DROPIN` an unloaded car's records are dropped from the list.
- `renderScene` calls `portFarLodBegin` when `portFarLod | portFarLodNearScale` is set (its only change).
- **Cost:** the near bands hold several times the primitives (78 against 12 for the building above; the whole HKONG
  file 12,885 at d2 = 0 against 327 past every record): set the mesh cap and the packet buffer to their maximum
  (`FAR_DISTANCE.md` 5) before using 4.
- `tools/check_custom.py` check 5: only `renderScene` (game code) reads `portFarLodNearScale`, only port code names
  `portFarLodNear`, `portFarLodBegin` / `portFarLodEnd` read it and reach no `rand()`; `check_car_groups.py` check 12
  lists `portFarLodNear` as an unload holder.

## 14. Cyburbia with 16 cars: sky and ground gray (request 46-7)

### 14.1 The arena's textures are never overwritten by the cars' placement (CONFIRMED by replay)

- **Cyburbia's VRAM** (BURB.TMS, 249 images, all at fixed places): the upper half x 640..1023 (its 8-bit images at
  (640, 0..255), 4-bit ones at 704..1023), its 256-colour CLUTs at (768, 0..28) and its 16-colour ones at rows 29..42;
  the lower half x 0..283 (columns of 4-bit and 8-bit images, full height). Nothing at rows 240..255 below x 640.
- **Replay** (`check_car_groups.py`'s placement model: `vramPlaceTexture` / `vramUploadClut` with retail's top-left
  tests, or portCarOptions bit 9's whole-spot scans; scratchpad `cyb.py`): 400 random 16-car mixes (7..13 distinct
  types; duplicates share the first car's VRAM, banks off), with bit 9 clear and set, plus the worst orders (14 types
  largest first; Mr. Grimm + Twister + Dark Tooth): **no car image and no car CLUT ever lands on an arena rectangle**.
  The arena protects itself: a car's 256-colour CLUT can only start at x 0 / 256 / 512 / 768, and at 0 / 256 / 768
  every row is used by the arena from its first texel (the top-left test refuses it), so they go to (768, 43..55) and
  then x 512 of the lower half, which only cars use; car images start at x 960 and then at the lower half's free
  columns past 283. The same replay over the 13 other arenas: no arena texel taken either.
- **Retail's overlaps** (`CAR_GROUPS.md` 13.12, `RETAIL_BUGS.md` 27) on Cyburbia are all over **other cars'** texels
  (from the 9th type on), never the arena's.
- **With texture banks** (the port turns them on with a duplicate or more types than the arena holds, so always with
  16 cars): each load group's images and CLUTs go to a bank of its own; bank 0 holds the arena and receives no car
  upload (an image whose key the arena holds keeps the arena's slot, nothing is written).
- **Other VRAM writers of a match** (`TEXTURE_BANKS.md` 4): the arena's own CLUT animations (its objects 5000 / 5001 /
  5100); the invisibility fade (`clutAnimStartFadeOut`, entries to 0 = transparent) on the CLUT of each car's +8 mesh
  (with banks per key, in the car's bank): for Outlaw 2, Mr. Grimm and Thumper the first +8 primitive is untextured and
  the fade's row is junk read past its colour word, at **(16, 0) / (0, 0) / (0, 0)** in the frame buffer, not the
  arena; rows 240..255 (bit 6's and bit 7's allocator, under the frame buffers); the port's team-paint uploads (into the
  car's bank or its own CLUT copy). None of them touches Cyburbia's rectangles.

### 14.2 So where can the gray come from (GUESS, for the port)

The decomp's VRAM for bank 0 is as the TMS put it for the whole match. Since the cars stay fine and the sky and the
ground are arena textures (the 8-bit images at (640, 0..255) with their CLUTs at (768, 0..2)), look at the port's side:
- dump VRAM bank 0 (the renderer's copy) in a gray frame and compare (640..767, 0..255) and (768..1023, 0..2) with
  BURB.TMS: if they match, the texels are there and the renderer's sampling of bank 0 (or of the bank layer, a
  `tpage` bits 9..15 mix-up) is the place to look; if not, log every `LoadImage` / `MoveImage` / `ClearImage` into
  those rectangles (the decomp's own uploads after the load are only the CLUT animations and the port's);
- "a quarter of the time" may be per match (the car mix: banks or not, how many banks the renderer grows; PSY-Z's
  `bank_tex` is re-created in steps of 8 layers) or per frame (a fade or an animation running);
- the arena's 256-colour CLUTs at (768, 0..28) are where an invisibility fade would make texels transparent (entries
  0): only a fade definition pointing there would do that; the decomp's never do (14.1).

### 14.3 How to check free VRAM for a secret arena

`py -3.14 tools/check_car_groups.py --clut-model` prints, per arena (Swamp, Cyburbia and Roof Tops included), the
whole free 256-texel rows and 16-texel spots after the level, how many distinct types load before retail's first
failure / first overlap (largest and smallest first), and the same with bit 9. Cyburbia: 269 rows / 11,236 spots free;
14 types load (largest first: the 9th type's CLUT goes over another car's texels with bit 9 clear; with bit 9 none).
For one mix, the replay `cyb.py BURB <runs>` (scratchpad; it reuses the tool's model) attributes every overlap to the
arena's or a car's rectangle.

## 15. Antarctica pop-in and the rear-view mirror (request 46-9)

### 15.1 What each view has of its own (CONFIRMED)

| Limit | View 0 | Mirror (view 1, mode 4) |
|---|---|---|
| mesh queue (`portMeshListCap`, up to 2,048) | per `renderScene` (`renderInitState` and `portFarDistanceMeshListBegin` each view) | the same cap, its own count |
| matrix slots (200 objects / 198 transform nodes) | per view (`renderInitState`: count 0) | per view |
| packet buffer (`portPacketBufferBytes`, up to 390,320) | the frame's buffer from its start | **shared**: `renderBufferRearView->field_8 = D_80180EE4->field_8` (`rt_main.c:183`): the mirror continues where view 0 stopped; its effects, debris and scene come out of what is left |
| far distance | `portFarDistance` (`RENDER_FAR_DEFAULT`) | **retail's 500 for the level and the world models, 1,000 for the objects** (`engine_render_scene.c`: `viewGetMode(view) == 4`), never `portFarDistance` |
| far LOD (`portFarLod`, the new near scale) | yes (every `renderScene`) | yes |
| GTE range pass | with `portFarDistance` set | the same |
| near pass | `near_otz` 70 (20 split) | the same, mirrored NCLIP (`cull_flip`) |
| effect sprites / streaks | `portFarDistanceFxLimit` | the same limit (it follows `portFarDistance`, not the mirror's 500) |

So in the mirror the level ends at 500 units: everything beyond pops in and out at that edge as the car moves, which
at the farthest distance setting stands out against view 0.

### 15.2 Built: `portMirrorFar` (`TM2_PORT_FAR_DISTANCE`)

`portMirrorFar` (new `int`, `.port_bss` in every variant): 0 = retail (500 / 1,000); 1..32,767 = the mirror's far
distance for the level, the world models and the objects alike. `renderScene` takes both through
`RENDER_FAR_MIRROR(retail)` (retail form: the constant), i.e. `portFarDistanceMirror(500)` / `(1000)` (new,
`.port_text`). Set it with `portFarDistance` (and the packet buffer at its maximum: the mirror's packets come after
view 0's in the same buffer). `tools/check_custom.py` check 5: `renderScene` calls it twice, no other game code names
`portMirrorFar`, the macro's retail form.

### 15.3 What can still drop or move things (for the port)

- **The packet buffer** (shared): with view 0 at 32,000 units, the mirror's scene is the first thing cut; watch the
  frame record's cursor (+8) against its end in a frame where the mirror loses things.
- **The matrix slots** (200 a view, `renderScene`'s `if (*count < 200)` and the walker's opcode 5 `slti 198`, hand
  asm): past 200, every later object is drawn with slot 200's matrix (somewhere else, a "jump" for a frame), not
  dropped. Antarctica with 16 cars: the level's 35 transform nodes, 16 shadows (2 each), the sky, 16 cars (1 + 0..7
  transform nodes each) is about 130; projectiles and parts on screen add one each. Possible in a big fight; not raised
  (the walker's limit is in its hand-written code and three arrays of 201 slots); the port can watch the slot counter
  (scratchpad 0x1F800008) to see whether it hits 200.
- **LOD band gaps** (`PLAYTEST_ANSWERS.md` 3.2, Axel): split bands measured from different reference points leave a
  distance with no band drawn: a part flickers at that distance. Retail data; the near scale (13.2) moves the gaps
  but does not close them.
- **The mirror's picture in its frame** (the user's TV, 16:9): the frame is drawn by the HUD (`uadashInitRearViewMirror`
  800FB8AC, black bars 209..314 x 18..52) and the picture by view mode 4 (x 211, y 20, 101 x 30, its projection centre
  (50, 15) inside it; `WIDESCREEN.md` table and its anchoring section). If the port anchors or scales the HUD's right
  group for 16:9 and not mode 4's x (8017EDE4), or rounds them differently at a high resolution, the picture slides
  against the frame; both must move by the same amount (`WIDESCREEN.md`: +107 with the right group). The decomp keeps
  both fixed: nothing in it moves one without the other.

## 16. Cyburbia: computer cars driving a short loop (request 46-8)

### 16.1 The route data (CONFIRMED)

- **The disc's** (CYB.PTS / CYB.GRP): a stub, `SECRET_LEVELS.md` 4: 10 points, 2 sections, the route byte 6 in every
  point, no links; with it two computer cars in three aim at node 0.
- **The port's** (its `src/vplus/cyburbia_route.inc`, written over the loaded files after `arenaLoadCyburbia`; read
  here, not changed): 127 points, 35 sections (the plaza, 24 street boxes, 10 ring-road pieces), next-node /
  next-section tables for every pair. Checked (scratchpad `cybroute.py`): every point reaches every other point
  (the graph is strongly connected); every section has points; every table entry names an exit point inside its own
  section; every point's section byte equals `worldSectionAt` of its position. **Complete.**

### 16.2 What the AI does with a target its route does not come near (CONFIRMED code, model)

- Between attacks a computer car walks the route: at each point (`aiChooseNextNode` 800F763C) `aiPickLinkNode`
  (800F7B00, `ai_link_node_pick.c`) takes, of the point's successors, the one **nearest the aim point** (moods 0 and 1;
  mood 2 the farthest), where the aim (`aiFollowRouteTarget` 800F7D54) is the next section's exit point, or, once in
  the target's section, **the target itself** (ahead of it by half the distance plus a lead). The previous point may be
  picked again only 3 times in a row (`node_repeat`, +80 to its distance).
- Only mood 1 leaves the route to drive straight at a target (`aiShouldChaseDirect` 800EBB74: same section, the
  target not faster than half its cap, within 2 x the goal distance clamped 320..4,800, not stuck). Moods 0 and 2 walk.
- **The loop:** when the target stands off the route (between the roads, on the plaza's edge, across the ring road),
  the walk reaches the point nearest it and must still move on: it takes the successor nearest the target (B), and
  from B the nearest is the point it came from (A): A-B-A-B (three reversals allowed), then B-C, C-B, B-A ... a cycle
  of 6 legs around the nearest point, each a 180-degree turn, never closer. The model over the port's route (targets on
  an 800-unit grid over the whole map, every start point, moods 0 / 1) ends **23 %** of the walks in such a cycle;
  the most frequent are around the plaza's north and south points and the ring road's corners (legs 1,800..6,600
  units), e.g. `r3-r2-r3-r2-r1-r2` on the east ring for a target at (29920, 5280). Retail's own escape does not help:
  turning 1.5 turns (`turned >= 6145`) or 180 ticks starts a give-up (`aiStartGiveUp`, `aiUpdateGiveUp` 800E9674)
  that re-plans from the nearest route point (`aiReplanRoute` 800E9578), the same point.
- The targets themselves are the port's (spectating: the nearest computer car every 60 VBlanks, `fixes.c`
  `spectate_frame`; "everyone equal" likewise; teams), so a loop lasts as long as the nearest car stays off-route.

### 16.3 A give-up / retarget rule (for the port; no code here)

The target choice is the port's (with its teams), so the rule belongs where it picks: per computer car k, keep the
target it was given and the best `target_dist` (+0x028) seen while chasing it; count the route points the car reaches
(`DRV.node` changes, +0x464) and the steps since the target's HP last dropped. When it has passed **8 route points**
without getting **10 % closer** than its best and without hurting the target, **give it up for 10 seconds**: pick the
nearest other valid target (the port's own enemy test) and do not pick the given-up one again until the time is up
(or it comes within 1,500). Deterministic: no `rand()`, only positions, HP and counters. A decomp-side bit could do
the counting (and write `aiCarTargets`), but it cannot know the port's teams and the port rewrites the targets every
second anyway, so it would fight the port's choice; say if you want the counter in the decomp all the same (a
`portBalance` bit with the state in `portBalanceCars`). A walk-side alternative (no retarget): in the target's own
section, once no successor is nearer the target than the point the car stands on, chase the target straight (as mood
1 does) instead of walking on.

## 17. Dark Tooth's head under the bridge (request 46-10)

The fix is PR #66 (`portCarDarkToothParked`, section 7.4); the third session was before the port took it. The port's
Los Angeles Survival path is covered: `ai_out_of_play` (the port's `fixes.c`) writes HP 0 (+0x5A0), dead (+0x06C,
`state.dead`), wreck state 4 (+0x501, `stats.wreck_state`), out of range and far below; the frame hook
(`aiCarsCountRemaining`, every frame) hides the head of every Dark Tooth slot whose AI record is dead with wreck state
4, from the first frame of the run (`vp_survival_start` puts every car out of play before the first `worldUpdate`
draws), and shows it over the truck when `ai_respawn` starts him. The offsets are the decomp's (`cars.h`: the AI
record's state at +0x06C, stats at +0x4FC, `wreck_state` at stats +0x05). Nothing else for the port to do than to take
the PR (the defines `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CAR_SPECIALS` and `TM2_PORT_DARK_TOOTH`).

## 18. Verification (request 46 items 3-10)

E0 = `work/build-20261018-061725-740336/source_reloc_e0`; every row by `pv.py` (P4 and the placement link P2,
`verify_reloc.py` against E0) and `tools/check_all.py work/pv/r46b_<tag> --e0 <E0>`: both proofs, the placement
check and every applicable check pass (`check_balance.py --expect-functions`: G16 with the shield gate, still the 20
functions; `check_custom.py --expect-functions`: request 46's visual functions and walk order, `bulletSpawn` and
`streaksDraw` against E0, check 5 on the far-LOD and far-distance links, with `TM2_PORT_VISUAL` alone exactly
`weaponArcsDraw`, `renderScene`, `bulletSpawn`, `streaksDraw`; `check_car_groups.py` with `portFarLodNear` as an unload
holder; `check_hazards.py`, `check_humans.py`). Negative controls: `check_balance.py` on request 45's balance link
(`work/pv/r45_bal_P4`) fails G16 (no `portBalanceEnergyLocked`); `check_custom.py` on request 45's visual link and on
request 46's all link (`work/pv/r46_allh16_P4`) fails the request 46 lines of checks 2 and 5; `--negative-control` of
both tools on the new default link fails as it must. Car stack = `TM2_PORT_CAR_GROUPS TM2_PORT_CARS=16
TM2_PORT_CAR_SWAP TM2_PORT_TEXTURE_BANKS TM2_PORT_CAR_SPECIALS TM2_PORT_DARK_TOOTH TM2_PORT_CAR_SOUND TM2_PORT_CAR_CLUTS
TM2_PORT_CAR_DROPIN TM2_PORT_SPECIAL_AS TM2_PORT_DT_PLAYER`; all = the car stack + `TM2_PORT_FAR_LOD
TM2_PORT_FAR_DISTANCE TM2_PORT_SPU48 TM2_PORT_HAZARDS TM2_PORT_HUMANS=16 TM2_PORT_BALANCE TM2_PORT_VISUAL
TM2_PORT_CAR_NUMBERS`.

| Defines | P4 sha | P2 (placement) sha | port end (P4) |
|---|---|---|---|
| none | `5bd13be83c43` | `b95e6fed7439` (unchanged) | 80551F50 |
| `TM2_PORT_BALANCE` | `1de38c7d3015` | `0acb15d40987` | 80553780 |
| `TM2_PORT_VISUAL` | `09424b722eb9` | `8b71a151f582` | 80552A20 |
| `TM2_PORT_FAR_LOD` | `2fe28ecb985c` | `08a014930292` | 80552A70 |
| `TM2_PORT_FAR_DISTANCE` | `075c11769430` | `bc28a1416ee9` | 805527F0 |
| car stack + `TM2_PORT_BALANCE` + `TM2_PORT_VISUAL` + `TM2_PORT_FAR_LOD` | `5e2f7e06fdf9` | `301ca9ea7e83` | 805A85A8 |
| all + `TM2_PORT_HUMANS=16` | `e1eeb1c602fa` | `a258cf48c55f` | 805B292C |

The default variant's image changed (`5bd13be83c43`, was `c1108ee81c3d`): the port sections end 51,264 bytes later (51,256 bytes of storage and their alignment
(`portFarLodNearScale`, `portMirrorFar`, `portFarLodNear` 49,156, `portVisualState` 2,092)), so `.port_text` moved;
its code did not (proof and placement as above: no game function changed without the defines). Port addresses after
`portCarDtParkMask` are new; read them from `out/TM2_SOURCE_PORT/symbols.tsv`.

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261018-183040-699773`):
hybrid `9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB`
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `5bd13be83c43…` (placement link `b95e6fed7439…`, unchanged); `tools/check_all.py` on it passes;
`tools/make_package.py` written. The models of 14 and 16 (`cyb.py`, `cybroute.py`, `lodtex.py`, `shadow.py`) are in the
session's scratchpad; their method is described in each section.

# Request 47 (room for 4 views at the farthest distance, Cyburbia's black shards; 2026-10-18)

The port measured its worst views with `portRenderStats` (`RENDER_ANSWERS.md` R2.4): 4 players on Antarctica at
Farthest with near scale 4 used 377 KB of packets for views 1-2 and 387 KB for views 3-4 (97-99% of 4 x retail's
buffer) and 185 matrix slots in view 4 (198 is the transform nodes' limit). Same tags as above; file:line refers to
branch `decomp-requests-47`. Nothing was run in an emulator.

| # | Item | Result |
|---|---|---|
| 47-1 | More packet room | **built**: `portPacketBuffers` is 8 x retail's per buffer **and a set per view pair** (set 0: views 1-2 and the mirror, rtMainLoop's six records; set 1: views 3-4, the port's records through the new `portPacketBuffersApplySet`), 3,123,200 bytes; `portPacketBufferBytes` up to 780,720 (section 19) |
| 47-2 | More matrix slots | **built**: `--port-define TM2_PORT_RENDER_SLOTS=N` (e.g. 400): `renderScene` counts to N, transform nodes get slots below N - 2, the five slot arrays move to `.port_bss` with N + 1 slots, and the variant links the walker's **C form** (the all-C build's, byte for byte but the one immediate); `portRenderStats`' cap bits follow (section 20) |
| 47-3 | Cyburbia's black shards | **retail data, retail shows them too**: each of the four gates in the plaza wall carries 12 double-sided textured triangles with colour 0 and the **opaque** command (0x24); their STP texels draw black (a ragged strip pattern) on the gate's top, within 450 units of a gate. The only such primitives on the disc's arenas; likely meant to be semi-transparent like the gate's own colour-0 shadow quads. No untextured far models, no hidden geometry; the level's one zone node swaps the whole city for the highway ring by camera position (section 21) |

## 19. More packet room (request 47-1)

### 19.1 Built: 8 x, a set of buffers per view pair

`src/semantic/port_variant_arrays.c`: `PORT_PACKET_BUFFER_FACTOR` 8 (780,800 bytes a buffer, 80 of them slack),
`PORT_PACKET_BUFFER_SETS` 2: `portPacketBuffers[set][parity]`. **Per view pair** rather than 8 x shared: the port's
worst pairs were 377 KB and 387 KB, so one shared 8 x buffer (780 KB) would hold 4 views only at 98%; a set per pair
gives each pair 780,720 bytes (48-50% at the measured worst).

- `portPacketBuffersApply(records, count)` (rtMainLoop, unchanged call) = `portPacketBuffersApplySet(records, count,
  0)`: the six retail frame records (views 1-2 and the mirror) at set 0.
- `portPacketBuffersApplySet(records, count, set)` (new, `.port_text`, `port_far_distance.h`): the port calls it at
  the top of each frame with its own frame records of views 3-4 and set 1 (even record = parity 0, odd = parity 1, as
  rtMainLoop's). It writes each record's start (+4) and end (+0x0C); with `portPacketBufferBytes` back at 0 it points
  them at retail's two buffers again (once), as set 0 does. Another set number does nothing.
- `portFarState.packetSet` is now a bit per set (no size change).
- The emitter and every effect drawer already stop at the record's end (+0x0C, FAR_DISTANCE.md 3), so a set 1 record
  needs nothing else. The port's own trick (views 3-4 drawing into the other parity's buffer) is no longer needed.

### 19.2 What shares the RAM past it (default variant, `work/pv/r47_none_P4`)

`portPacketBuffers` stays at **0x80221840** in the default variant (only `portFarState` before it can grow, with
`TM2_PORT_RENDER_SLOTS`) and now ends at 0x8051C040; it is 3,123,200 bytes (+2,342,400). Everything after it moves up by
2,342,400 bytes, in this order (default variant, `work/pv/r47_none_P4/symbols.tsv`):

| from | what | bytes |
|---|---|---|
| 0x8051C040 | `portCarGroups`, `portCarOptions`, the car arrays (`aiCars` 0x805200CC .. `carSlotObjects`), `portCarsPerArena`, `portCarPoolsLive`, `portCarSource` | 36,292 |
| 0x80524E04 | the pools: `fxRecords`, `csObjectPool`, `soundInstPool`, `portSoundSortList`, the five pair tables | 65,980 |
| 0x80534FC0 | `portCarDmdArea` (the car DMD area) | 2,099,200 |
| 0x807357C0 | `hudRadars`, the texture banks, specials, car sound, CLUTs, drop-in | 22,576 |
| 0x8073AFF0 | `portCarLiveTex` | 270,472 |
| 0x8077D078 | specials-as, Dark Tooth player, hazards, humans, balance, round, unload, visual, car numbers, car select, park mask | 17,112 |
| 0x80781350 | `portFarLodNearScale`, `portMirrorFar`, `portFarLodNear` (49,156), `portVisualState`, `portRenderStats` (0x8078DB88) | 51,600 |
| 0x8078DCE0 | `.port_text` (436 bytes); the port sections end at **0x8078DEA0** | |

The port RAM end is 0x80C00000 (`--port-ram-end`, the default): 4,661,600 bytes left in the default variant. With
the defines the port uses:

| link | port end | left below 0x80C00000 |
|---|---|---|
| default (no define) | 0x8078DEA0 | 4,661,600 |
| `TM2_PORT_FAR_DISTANCE` + `TM2_PORT_RENDER_SLOTS=400` | 0x807A03E0 | 4,586,528 |
| car stack + `TM2_PORT_FAR_DISTANCE` | 0x807DC5A8 | 4,340,312 |
| all port defines + `TM2_PORT_HUMANS=16` + `TM2_PORT_RENDER_SLOTS=400` | 0x8080052C | 4,192,980 |

New addresses (all + HUMANS=16 + RENDER_SLOTS=400, `work/pv/r47_allh16_P4`): `portFarState` 0x8021F474 (48,864
bytes), `portPacketBuffers` 0x8022B354, `portRenderStats` 0x807DB934, then the slot arrays: `renderSlotMatrixPtrs`
0x807DBA84, `renderMatrixSlots` 0x807DD394, `renderObjectMatrices` 0x807E05B4, `renderFrustumMatricesA` 0x807E37D4,
`renderFrustumMatricesB` 0x807E69F4 (12,832 bytes each, 6,416 for the pointers), `portPacketBuffersApplySet`
0x807EBCA0 (`.port_text`). Read them from each build's `symbols.tsv`: anything after `portFarState` moves with the
slot limit, anything after `portPacketBuffers` moved with this request.

## 20. More matrix slots (request 47-2)

### 20.1 Built: `TM2_PORT_RENDER_SLOTS=N`

A new define rather than `TM2_PORT_FAR_DISTANCE`: it changes the walker and moves five arrays, which a port that
only wants the far distance does not need; the two work together (`portRenderStats` is FAR_DISTANCE's). The table
of every place that knows the limit is in `FAR_DISTANCE.md` 3 "Matrix slots"; in short:

- `include/semantic/render_slots.h` (new): `TM2_MAX_RENDER_SLOTS` (array slots: retail 200, N + 1),
  `TM2_RENDER_SLOT_LIMIT` (N; retail 200) and `TM2_RENDER_NODE_SLOT_LIMIT` (N - 2; retail 198). N is 201..2,048; the
  bare define means 400. `--port-limits 2` forces `TM2_MAX_RENDER_SLOTS=200` (placement link).
- `renderScene` (`engine_render_scene.c:230-284`): its five `*count < 200` are `*count < TM2_RENDER_SLOT_LIMIT`
  (retail compiles to the same bytes).
- The walker: under the define `engine_render_scene.c` drops the three asm blocks (`renderPushMatrix`,
  `renderWalkModel`, `renderMulTransMatVec`) and includes the walker part of
  `reference/renderer_walker_engine_render_scene.c` with `RENDER_WALKER_NODE_SLOTS` = N - 2 (the reference's
  `count < 198` is now that macro, 198 in the all-C build; its blank lines 39 / 120 / 454 became the guard, so its
  line numbers and its object are unchanged). `renderInitNodeHandlers` fills the table with the C walker's entry
  (`RENDER_NODE_HANDLER` macros; retail: the asm labels).
- The arrays (`port_variant_arrays.c`, after `portRenderStats`; tsv group `renderslots+`): `renderSlotMatrixPtrs`
  (16 B a slot), `renderMatrixSlots`, `renderObjectMatrices`, `renderFrustumMatricesA`, `renderFrustumMatricesB`
  (32 B), N + 1 slots each, so the last slot `renderScene` hands out is inside them (`RETAIL_BUGS.md` 13 is gone in
  this variant). Every import is redirected by the link (they are reached only at their bases); `renderInitState`
  writes their new addresses to the scratchpad RenderState each frame, and the walker and the emitter read them from
  there or from `renderScene`'s arguments: nothing in the scratchpad or `renderCtx` moves.
- `portFarState` (the GTE range pass) is sized by the same limit: N + 2 source rows, N + 312 copies (tsv expression
  `16+8*(renderslots+1)+64*(renderslots+312)`; 34,392 bytes at retail's 200, 48,864 at N = 400).
- `portRenderStats`: `TM2_RENDER_SLOTS_NODES` / `_MAX` come from `render_slots.h` (N - 2 / N), so cap bits 0x1 / 0x2
  mean the raised limits.
- Not changed: the emitter (`sub_800CDC54`) and the near pass (asm; no slot limit), `csUpdMat` (writes the slot it
  is given), the near list (200) and the walker's stack (200).

### 20.2 Why the C walker can be linked, and how it was checked

`--all-c` cannot be combined with `--port-variant` (`build_source_only.py` refuses it) and its reference unit has an
old `renderScene` without the port's hooks, so the variant does not swap the unit; it includes only the walker's
part. Checked statically:

- the variant's three walker sections are **the all-C build's bytes and relocations except one word**
  (`renderWalkModel` +0x680: `slti $2,$9,198` -> `slti $2,$9,398` for N = 400; identical with the limits forced to
  retail); the reference unit compiled before and after the guard edit gives the same object (20 sections), so the
  all-C flavour is unchanged;
- the C walker keeps its registers on its stack, not in the scratchpad words 0x40 and 0x4C..0x7B; every reader of
  those words writes them first (`renderScene`; the emitter's first access to each scratchpad word it reads is a
  write, except 0x40 / 0x4C / 0x60..0x74 / 0x88, which `renderScene` and `renderDrawModel` set before calling it);
- the C walker is the code the all-C flavour proves and that matched the retail asm on random inputs with
  the near pass (`SOURCE_BUILD.md` "All-C flavour"); the mesh cap's counter bias works the same (`S->end >= 280`);
- `tools/check_custom.py` check 6 (below).

## 21. Cyburbia's black shards (request 47-3)

Read from the disc (`DMD\BURB.DMD`, `TMS\BURB.TMS`, read-only) with `tools/dmd_lod_dump.py`'s reader and small
scripts (`burb.py`: every mesh with its context and the colour its primitives would show, VRAM rebuilt from the TMS;
`burbzone.py`, `burbblack.py`, `blackcensus.py`; session scratchpad).

### 21.1 The gates' black triangles (CONFIRMED data, LIKELY effect)

- The plaza in the middle of the city has a gate in each side of its wall: one mesh (pointer 8006D674, file
  +0x53A34) drawn four times, at world (1800, 997), (1402, 600), (2400, 202), (2797, 600), in the near record
  [0, 450) of its LOD node 8006E1D8 (the far record [450, 1500) is another mesh, without them).
- 12 of its 60 primitives are **double-sided textured triangles (FT3) with colour (0, 0, 0) and the opaque command
  0x24**, 48..80 world units up (the gate's top). Their texels (tpage 0x0010, CLUT 0x08BD, v 135..144) are a ragged
  strip, about 70% transparent (0x0000), the rest with the STP bit set. An opaque command draws STP texels opaque and
  the colour 0 modulates them to black: **ragged black shapes standing over the wall**, 48 triangles in all, up to
  about 250 square world units each. File offsets of the 12 records: +0x53F98, +0x53FB8, +0x53FD8, +0x54040, +0x54060,
  +0x54080, +0x541E4, +0x54228, +0x5426C, +0x5428C, +0x542D0, +0x542F0 (file base 80019C40).
- **Retail shows them**: same data, same GPU rule; from the plaza floor and the streets near a gate (camera within
  450 units of the gate's LOD point and inside the city zone, 21.2). They are not the port's, nor the far distance's
  (the LOD is the gate's own), which fits "the same at the original distance". With `portFarLodNearScale` 4 the
  near band reaches 1,800 units, so the port shows them from four times farther than retail.
- **Likely a data slip**: the same gate's two ground quads use colour 0 with the **semi-transparent** command 0x2E
  (dark shadows), and the other 13 arena files have one opaque textured primitive with colour 0 between them (Moscow;
  Cyburbia has one more, in a select child that is not current at load). `RETAIL_BUGS.md` 34.

### 21.2 What else was looked for (CONFIRMED data)

- **Untextured or black far models:** none. Every LOD record was walked: the level file has 67 untextured primitives in
  all (7 black: 4 of about 1.4 x 1.4 units, and 3 of up to 57 x 30 units in mesh 80089EC4, a child of select node
  502 that is not its current child at load: drawn only if the game switches it), no far band that is untextured.
- **Hidden geometry by camera position:** the level's only zone node (8008B580, under the level root) swaps **the
  whole city** (377 meshes, 1,655 placements) for **the highway ring** around it (76 meshes) by the camera's
  position: the city is drawn while the camera is inside x -50..3650, y -150..1950 and below 200 units (one
  5-plane region), the ring otherwise. A spectator camera above 200 units or past the city's edge would see the ring
  only; it shows no black polygons.
- **Unloaded textures:** 171 textured primitives sample VRAM that `BURB.TMS` does not write (ground pieces with page
  0x0014 whose UV boxes run past their images); most of each is inside an image. Not the shards.

### 21.3 What the port could do

- A load-time patch of the 12 records after Cyburbia's DMD is in RAM (the records at the DMD's load address + the file
  offsets above, or found by walking the level root to the gate mesh): command byte (+0x13 of the colour word, the
  record's byte +0x13) 0x24 -> 0x26 makes them semi-transparent (50% dark, like the gate's shadows), or v1 = v2 = v0
  (+0x04..+0x0A) hides them. Either is a few bytes; the decomp can add it as a `portVisual` bit (Cyburbia's load,
  `arenaLoadCyburbia`) if wanted.
- Culling the whole mesh would also remove the gate (the other 48 primitives); not recommended.

## 22. Verification (request 47)

E0 = `work/build-20261018-202455-774927/source_reloc_e0`; every row by `pv.py` (P4 and the placement link P2,
`verify_reloc.py` against E0) and `tools/check_all.py work/pv/r47_<tag> --e0 <E0>`: both proofs, the placement check
and every applicable check pass (`check_custom.py --expect-functions` with the new check 6, `check_car_groups.py`,
`check_hazards.py`, `check_balance.py`, `check_humans.py`). Car stack and "all" as in section 18; all here also has
`TM2_PORT_RENDER_SLOTS=400`.

| Defines | P4 sha | P2 (placement) sha | port end (P4) |
|---|---|---|---|
| none | `5ad80f2d4bcc` | `c965c9834443` | 8078DEA0 |
| `TM2_PORT_FAR_DISTANCE` + `TM2_PORT_RENDER_SLOTS=400` | `476ea182b6ed` | `af03ba5b96bf` | 807A03E0 |
| car stack + `TM2_PORT_FAR_DISTANCE` | `0e37580a4651` | `ff254855f83c` | 807DC5A8 |
| all + `TM2_PORT_HUMANS=16` + `TM2_PORT_RENDER_SLOTS=400` | `8fefa578c9d7` | `c2cd14cb6657` | 8080052C |

- With `TM2_PORT_RENDER_SLOTS` the changed functions are the request 46 ones plus exactly `renderWalkModel`,
  `renderPushMatrix`, `renderMulTransMatVec`, `renderInitNodeHandlers` (and `renderScene`); the proof (un-relocated
  image equal to E0 outside them) and the placement check (limits forced: the arrays stay, the C walker with 198)
  pass with the C walker linked.
- Before the links: `engine_render_scene.c` compiled with its recipe gives the build's object without the defines
  (retail and the default variant); the reference unit gives its old object; the variant's walker equals the all-C
  walker but the immediate (section 20.2).
- Negative controls: `check_custom.py --negative-control` on the default link fails check 6 (the asm walker's
  `slti 198`, `renderScene`'s 200, the asm labels in `renderInitNodeHandlers`, the arrays at their retail places,
  `portFarState` for 200 slots) as it must; `check_custom.py` on request 46's far-distance link (`work/pv/rmc_fd_P4`,
  4 x buffers) fails check 6 and the storage line (`portPacketBuffers` 780,800 bytes, no `portPacketBuffersApplySet`).
- The default variant's image changed (`5ad80f2d4bcc`, was `0055db6d1473`; placement `c965c9834443`, was
  `b95e6fed7439`): the port sections end 2,342,400 bytes later, so every port address after `portPacketBuffers`
  and `.port_text` moved; no game code changed (25 functions differ from E0 as before, 5 with the limits forced).

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261019-034814-106731`):
hybrid `9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB`
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `5ad80f2d4bcc…` (placement `c965c9834443…`); `tools/check_all.py` on it passes;
`tools/make_package.py` written (560 files). The Cyburbia scripts of section 21 (`burb.py` and its helpers) are in the
session's scratchpad.

# Request 47 revised (a packet buffer per view, the renderer as native code; 2026-10-19)

The port's revised request: one path for every player count (near scale 4 everywhere, no 2 x / 4 x split), so a
packet buffer for every view rather than a set per view pair; and the user's question whether the renderer could run
as native PC code. Items 2 (the matrix slots) and 3 (Cyburbia's black shards) were answered by request 47 (sections 20
and 21, PR #69) and are not repeated. File:line refers to branch `decomp-requests-47b`. Nothing was run in an
emulator.

| # | Item | Result |
|---|---|---|
| 47-1 | A packet buffer per view | **built**: `portPacketBuffers[5][2]` (views 1-4 and the mirror, two parities) of 4 x retail's (390,400 bytes, 3,904,000 in all); `portPacketBuffersApplyView(record, view, parity)` points one frame record at its view's own; `rtMainLoop` starts view 2's and the mirror's cursor at their own buffer (`portPacketBufferCursor`); `portPacketBuffersApplySet` kept as a wrapper (section 23) |
| 47-2 | More matrix slots | answered by request 47: `TM2_PORT_RENDER_SLOTS=N` (section 20) |
| 47-3 | Cyburbia's black shards | answered by request 47: retail data, the plaza gates' 12 opaque colour-0 triangles (section 21) |
| 47-4 | The renderer as native PC code | **research, no code**: feasible; the C forms are proven statically (the all-C build), the walker and the near pass also on random inputs, the emitter never run; GTE macros, 32-bit guest addresses, the scratchpad in `M.scratch`; a staged plan of about 3-5 weeks: [`NATIVE_RENDERER.md`](NATIVE_RENDERER.md) (section 25) |

## 23. A packet buffer per view (request 47-1 revised)

### 23.1 Built

`src/semantic/port_variant_arrays.c`:

- `PORT_PACKET_BUFFER_FACTOR` 4 (the size of each buffer: 4 x retail's 97,600 = 390,400 bytes, 80 of them slack, so
  `portPacketBufferBytes` goes up to **390,320**, the value the port uses), `PORT_PACKET_VIEWS` 5,
  `PORT_PACKET_MIRROR` 4: `int portPacketBuffers[PORT_PACKET_VIEWS][2][PORT_PACKET_BUFFER_BYTES / 4]`. View index 0..3 =
  views 1-4, 4 = the rear-view mirror (the rows of `portRenderStats`). 3,904,000 bytes (request 47: 3,123,200).
- `portPacketBuffersApplyView(record, view, parity)` (new, `.port_text`): one frame record (0x80 bytes) at
  `portPacketBuffers[view][parity]`: start (+4) and end (+0x0C = start + `portPacketBufferBytes` & ~3); with the bytes
  back at 0 it points it at retail's `renderPacketBuffer0/1` (by parity) again, once. It never writes the cursor (+8).
  Another view number does nothing. `portFarState.packetSet` is a bit per (view, parity) now (bit 2 v + p).
- `portPacketBuffersApply(records, count)` (rtMainLoop's call, unchanged): records 0-1 view 1, 2-3 view 2, 4-5 the
  mirror, each through `portPacketBuffersApplyView`.
- `portPacketBuffersApplySet(records, count, set)` (request 47's, kept so the port's call goes on working): set 0 =
  `portPacketBuffersApply`; set 1 = the port's records of views 3-4 (records 0-1 view 3, 2-3 view 4).
- `portPacketBufferCursor(record, before)` (new): the record's packet cursor after the view `before`: its own start
  when its start differs from `before`'s (a buffer of its own), else `before`'s cursor (retail's shared buffer).
- `rt_main.c`: view 2's and the mirror's `record->field_8 = D_80180EE4->field_8` (retail: the frame's packets go on in
  one buffer) is `RT_PACKET_CURSOR(record, D_80180EE4)`: `portPacketBufferCursor` with `TM2_PORT_FAR_DISTANCE`, retail's
  expression otherwise (so the retail and default objects are unchanged). Without it view 2 would have gone on writing
  at view 1's cursor, past its own buffer's end test.
- `include/semantic/port_far_distance.h` declares the four functions; `portRenderStats.packet` (+0x18) is now the
  view's own buffer's bytes (with retail's buffer still the frame's so far).

Every drawer already stops at its record's end (+0x0C: the emitter, the near pass, `fxSpritesDraw`,
`fxImpactParticlesDraw`, `streaksDraw`, `weaponArcsDraw`, `fxDebrisDraw`; FAR_DISTANCE.md 3), so a view's buffer
needs nothing else.

### 23.2 What the port calls

At the top of each frame, as today:

- nothing for views 1-2 and the mirror (`rtMainLoop` calls `portPacketBuffersApply` itself);
- for its own records of views 3-4: `portPacketBuffersApplyView(rec, 2, p)` / `(rec, 3, p)` for each record and parity
  (or request 47's `portPacketBuffersApplySet(records, 4, 1)`, which now does exactly that);
- each view's cursor at the start of its drawing: `rec+8 = rec+4` when its buffer is its own, which is what
  `portPacketBufferCursor(rec, prev)` returns (the port's `draw_view` in `vplus/interp.c` already uses the same rule:
  it goes on from `packets_from` only when the starts are equal). Views 3-4 in `port_views.c` must not go on from
  view 2's cursor any more.

**The 60 FPS redraws** (`vplus/interp.c` `take_rec`): each redrawn view takes the buffers of its **twin record** (the
other parity of the same view: the frame before's, already drawn and shown). With a buffer per view the twin is now
that view's own other buffer, so the rule stays and is now exact: every view's redraw has its own 390 KB, whatever the
other views drew; nothing needs to change. (In request 47 the twin of view 2 was set 0's other buffer, shared with
view 1 and the mirror.) If the port prefers to name the buffer, `portPacketBuffersApplyView(rec, view, 1 - b)` writes
the same start and end as copying the twin's. The redraw's records are restored afterwards, as today.

### 23.3 What lives in RAM past it

`portPacketBuffers` stays at **0x80221840** in the default variant (0x8022B354 with `TM2_PORT_RENDER_SLOTS=400`:
`portFarState` grows before it) and now ends at **0x805DAA40**. It grew by 780,800 bytes, and everything after it
moved up by that much, in the order of section 19.2 (default variant, `work/pv/r47b_none_P4/symbols.tsv`):

| from | what | bytes |
|---|---|---|
| 0x805DAA40 | `portCarGroups`, `portCarOptions`, the car arrays (`aiCars` 0x805DEACC .. `carSlotObjects`), `portCarsPerArena`, `portCarPoolsLive`, `portCarSource` | 36,292 |
| 0x805E3804 | the pools: `fxRecords`, `csObjectPool`, `soundInstPool`, `portSoundSortList`, the five pair tables | 65,980 |
| 0x805F39C0 | `portCarDmdArea` (the car DMD area) | 2,099,200 |
| 0x807F41C0 | `hudRadars`, the texture banks, specials, car sound, CLUTs, drop-in | 22,576 |
| 0x807F99F0 | `portCarLiveTex` | 270,472 |
| 0x8083BA78 | specials-as, Dark Tooth player, hazards, humans, balance, round, unload, visual, car numbers, car select, park mask | 17,112 |
| 0x8083FD50 | `portFarLodNearScale`, `portMirrorFar`, `portFarLodNear` (49,156), `portVisualState`, `portRenderStats` (0x8084C588) | 51,600 |
| 0x8084C6E0 | `.port_text`; the port sections end at **0x8084C8A0** | |

Port ends (P4 links; the port RAM end is 0x80C00000, `--port-ram-end`):

| link | port end | left below 0x80C00000 |
|---|---|---|
| default (no define) | 0x8084C8A0 | 3,880,800 |
| `TM2_PORT_FAR_DISTANCE` | 0x8084D490 | 3,877,744 |
| `TM2_PORT_FAR_DISTANCE` + `TM2_PORT_RENDER_SLOTS=400` | 0x8085EEA0 | 3,805,536 |
| car stack + `TM2_PORT_FAR_DISTANCE` | 0x8089B078 | 3,559,304 |
| all port defines + `TM2_PORT_HUMANS=16` + `TM2_PORT_RENDER_SLOTS=400` | **0x808BEFEC** | 3,411,988 |

Addresses with every define (all + HUMANS=16 + RENDER_SLOTS=400, `work/pv/r47b_allh16_P4`): `portPacketBufferBytes`
0x8020F470, `portFarState` 0x8021F474, `portPacketBuffers` 0x8022B354 (to 0x805E4554), `portCarGroups` 0x805E4554,
`portRenderStats` 0x8089A334, the slot arrays from `renderSlotMatrixPtrs` 0x8089A484; `.port_text`:
`portPacketBuffersApplyView` 0x808AA6A0, `portPacketBuffersApply` 0x808AA7B0, `portPacketBuffersApplySet` 0x808AA824,
`portPacketBufferCursor` 0x808AA8A8. Read them from each build's `symbols.tsv`: everything after `portPacketBuffers`
moved again with this request. The packets stay below 16 MB (24-bit OT links; SOURCE_BUILD.md).

## 24. Items 2 and 3

Answered by request 47 (PR #69): the matrix slots by `TM2_PORT_RENDER_SLOTS=N` (section 20; up to 2,048, the slot
arrays in `.port_bss`, the C walker), Cyburbia's black shards as retail data (section 21, `RETAIL_BUGS.md` 34).

## 25. The renderer as native PC code (request 47-4)

Research only: [`NATIVE_RENDERER.md`](NATIVE_RENDERER.md). In short:

- The C forms (`reference/`) are **not byte-exact** but **proven statically** by the all-C build (replacing the 18 asm
  sections changes no other byte); the walker and the near pass also matched the retail asm **on random
  inputs**; the emitter's C has **never run** (nor the hdp walker's, which is collision and not needed). Everything else
  the renderer and the effect drawers use is byte-identical C.
- Host C needs a GTE macro layer (the C forms use raw `cop2` asm) onto the port's `gte.c` (with `swc2` as
  `gte_store_data` for the precise vertices), 32-bit guest addresses in every structure (the host is 64-bit), the
  scratchpad as `M.scratch`, and, for no fixed limits, host queues and a packet path straight to PSY-Z.
- The effect drawers use no GTE (integer projection) except `fxDebrisDraw` (libgte); `streaksDraw` and
  `weaponArcsDraw` change game state while drawing (`rand()`, damage, the flame and drop updates); the HUD and the
  fonts need nothing (2D).
- Staged plan: decomp stage 0 (GTE macros and a structure header proven by the all-C build, the emitter's runtime
  test), port stage 1 (walker, emitter, near pass, `renderScene` as host C with a per-frame byte compare against the
  MIPS), stage 2 (the effect drawers), stage 3 (no limits, float precision); about 3-5 weeks of port work (GUESS).

## 26. Verification (request 47 revised)

E0 = `work/build-20261019-034814-106731/source_reloc_e0`; every row by `pv.py` (P4 and the placement link P2,
`verify_reloc.py` against E0) and `tools/check_all.py work/pv/r47b_<tag> --e0 <E0>`: both proofs, the placement check
and every applicable check pass (`check_custom.py` with check 6 updated, `check_car_groups.py`, `check_hazards.py`,
`check_balance.py`, `check_humans.py`). Car stack and "all" as in section 18.

| Defines | P4 sha | P2 (placement) sha | port end (P4) |
|---|---|---|---|
| none | `dec9a9d6ccdc` | `de7ded357cd2` | 8084C8A0 |
| `TM2_PORT_FAR_DISTANCE` | `64f62f3d7a02` | `acb5faf860df` | 8084D490 |
| `TM2_PORT_FAR_DISTANCE` + `TM2_PORT_RENDER_SLOTS=400` | `c3bd27090b77` | `10a9a9ba35ba` | 8085EEA0 |
| car stack + `TM2_PORT_FAR_DISTANCE` | `e9e2cbab8246` | `d968273a110b` | 8089B078 |
| all + `TM2_PORT_HUMANS=16` + `TM2_PORT_RENDER_SLOTS=400` | `eb944886dd4c` | `4e1aff62648f` | 808BEFEC |

- The functions the defines change are request 47's (`rtMainLoop` was already among them with
  `TM2_PORT_FAR_DISTANCE`); the default variant changes no game code (its image changed only because the port storage
  after `portPacketBuffers` moved).
- `check_custom.py` check 6 now checks: `portPacketBuffers` 3,904,000 bytes; `portPacketBuffersApplyView`,
  `portPacketBuffersApply`, `portPacketBuffersApplySet`, `portPacketBufferCursor` in `.port_text`, no `rand()`, called
  exactly by (`ApplyView` <- `Apply`, `ApplySet`; `Apply` <- `rtMainLoop`, `ApplySet`; `ApplySet` <- nobody: the
  port's; `Cursor` <- `rtMainLoop`); `rtMainLoop` calls `portPacketBufferCursor` twice and `portPacketBuffersApply`
  once; only `portPacketBuffersApplyView` names `portPacketBuffers`; `RT_PACKET_CURSOR`'s retail form is
  `((before)->field_8)`.
- Negative controls: `--negative-control` on the default link fails check 6 (no packet functions, no cursor calls) as
  it must; `check_custom.py` on request 47's link (`work/pv/r47_fdrs_P4`) fails check 6 (no `ApplyView` / `Cursor`,
  `ApplySet` naming the buffers, `rtMainLoop` without the cursor calls) and the storage line (3,123,200 bytes).

Full build `build.py --source-only --relocatable --port-variant --sdk-from-lib` (`work/build-20261019-052155-850291`):
hybrid `9219f7ba6531477280c4c907c20db0c45f83d433bf9f9d63b076054e829bb10a`, source-only
`d755c86ed01f437f9289d996a0dddffe7c78a6c2de565032143caf2d711acb55`, SDK from `.LIB`
`f557c1e3e8708e706382043eaf420e552d2170237b33152265035b1f8f39a5f1` (all unchanged); default port variant
`out/TM2_SOURCE_PORT.EXE` `dec9a9d6ccdc…` (placement `de7ded357cd2…`); `tools/check_all.py` on it passes (7 checks);
`tools/make_package.py` written.

## 27. Request 82: the near pass's split vertices, with a depth of their own (built; `TM2_PORT_C_EMITTER`)

Port request 82 (2026-10-29, after `RENDER_ANSWERS.md` R4.0): `renderNearSplitTri` / `renderNearSplitQuad` write
`MIDXY` midpoints into new packets with ordinary stores, so the port's depth test (vertex words looked up by their
address, from `swc2` records) finds no record for them, or an unrelated one by value. Asked: each split vertex in
the precise ring with its place and the depth of the screen-space midpoint (the harmonic mean of the corners'), or
the corners' GTE depths if that is all the C has.

**What the C has** (CONFIRMED): the near pass works on the emitted packets alone; no depth survives into it. The
emitter has the corners' depths (the GTE's SZ FIFO after RTPT, and after the fourth vertex's RTPS for a quad) when it
defers a primitive to the near list (`otz` under 70, 20 in split screen, mesh flag bit 2). And in the builds the port
uses the near pass is SingleTrac's hand-written asm; its pure C is the all-C flavour's copy in `reference/`.

**Built (with `TM2_PORT_C_EMITTER`, active only while `portPreciseEmit != 0`):**
- **The C near pass is linked** in the port variant: `engine_math_near_pass.c` includes the near-pass part of
  `reference/near_pass_engine_math_near_pass.c` (`TM2_NEAR_PASS_ONLY`; the unit's other functions stay its own,
  since the reference copy of them predates the port's changes, e.g. `TM2_SPU_VOICES`). Its packets and OT links
  are the asm's (check 8).
- **The emitter keeps the corners' depths:** for near entry n (the list's order: the scratchpad's 71, then the
  overflow; at most 200) `portNearDepth[n * 4 + i]` (`.port_bss`, 3,200 bytes) = corner i's SZ x 2^k (as the ring's
  depths; k the range pass's, 0 for near meshes); a beam's or sprite's entry gets its sort depth four times.
- **The near pass reports every vertex word it writes** (after the screen-place split; uv and colour words are not
  places), as the emitter's ring entries `{word address, x * 16, y * 16, depth}`:
  - a corner copied into a new primitive: its place (the packet's integer pixel, x 16) and its own depth;
  - an edge midpoint: the exact middle of its corners in 1/16 pixel (the packet's `MIDXY` truncates to the pixel)
    and the depth of the screen-space midpoint, `2 za zb / (za + zb)` (1 / z is linear on the screen), scaled down
    first if a depth exceeds 16 bits;
  - a quad's centre: the middle of the 0-2 and 1-3 midpoints, so (the harmonic mean of all four corners).
  - The primitive's first corner keeps its word and its `swc2` record (not reported). Per triangle 11 entries (its
    2 rewritten words and the 3 x 3 of the new ones), per quad 15. An entry whose depths were not kept (0) is not
    reported (the port's lookup as before). The ring's cap (`PORT_PRECISE_MAX`, 4,000) is shared with the beams
    and sprites.
- **Corner places are integer pixels**: the C has only the packet's words. Where the port has a corner's subpixel
  place (its `swc2` record of the original word), it may prefer it, and the midpoints' places from those.

**Checked:** `tools/check_custom.py` check 8, `check_near_pass`: the C near pass against E0's asm on a MIPS
interpreter on made-up near lists of 5, 40, 71, 72 and 120 entries (triangles and quads; flat, flat-textured,
gouraud and gouraud-textured; the texture-window flag; mirrored OT indices): every byte the same but the asm's
register save area; with `portPreciseEmit` 4096 and `portNearDepth` filled (one entry without depths, one corner
past 16 bits) the integer bytes unchanged and the ring exactly the entries above. The emitter's own runs (check 8's
scenes) leave the integer bytes unchanged with the depths kept. Not tested in game.

## 28. Request 110: the freeze in Dark Tooth's phase (Hong Kong, online co-op)

### 28.1 What the emitter was reading (CONFIRMED by reading)

At 800CDEF8 the emitter loads the mesh's primitive pointer (`lw $9, 12($14)`, mesh +0x0C). At 800CE77C it reads each
primitive's header through it (`lw $18, 0($9)`), stepping by the primitive's size. The walk "upward from FDA1F150"
is that loop on a mesh whose +0x0C held `FDA1F150`.

Read as two halfwords, that word is -3760 and -607. Both are inside +-4096, the shape of two rotation-matrix
entries: a 3 x 3 s16 matrix at a node's +0x04 puts its m[1][1] and m[1][2] at +0x0C. So the likeliest picture is a
mesh header (a type 0 node, `FILE_FORMATS.md` 2.4) overwritten by a matrix, not a stale or unloaded model.

The host froze too, after the joiner had left and without taking anyone's state. So the cause is in the shared
simulation, not in the state transfer.

### 28.2 What reaches the draw list in that phase, and what could be stale (CONFIRMED by reading; nothing found)

- **The reload resets every object and table.** The phase is a full reload (`sessionLoadArenaAndCars`), and
  `arenaResetSystems` runs:
  - `csPoolInit`: every drawn object is made again;
  - `carTablesClear` with `portCarGroupsReset`: the body, wreck, damage-stage, turbo, tire and parts tables, for
    every key;
  - `portCarSpecialsReset` / `portCarSpecialAsReset`: the duplicates' special state, and the Sweet Tooth, Roadkill,
    Mr. Grimm and Spectre model words;
  - `missilesCreateObjects` / `missileTrailsInit`: every projectile's object and trail mesh;
  - `debrisInit`, `viewInit`, `clutAnimInit`, and the pickups, targets and level events.
- **Every model the phase's objects take comes from this load:**
  - the level file: projectiles 0x899 / 0x89A / 0x8A0, mines, pickups, effects;
  - Outlaw 2's and Warthog's files;
  - `BB.DMD`: his missile model 2301, his ricochet look 2214, his head 249, his parts.

  `darkToothBossObject` is cleared at the load and made again by asset 0xF9.
- **The remaining suspects are ruled out:**
  - his death in co-op skips the debris burst (`gameGetMode() != 2`);
  - he has no wreck model;
  - the swoop reads car records only;
  - trail meshes are written only up to vertex 20 (counts 6, 3 and 1), never their strip header;
  - the freeze records `portCarGroups.freeze[0..15]` are kept across loads (as retail), but they hold the level
    file's pointers, and the same level file is loaded at the same address.

### 28.3 What writes matrices into model memory during play (the candidates for 28.1)

- `carBodyPoseUpdate` (`car_body_pose.c`): 32 bytes at each car's shadow node +0x04, every step;
- `portCarPartsSpin` (`port_variant_arrays.c`): 20 bytes at each registered spinning part +0x04;
- `portVisualSteadyPose` / `portVisualSteadyEnd` (portVisual bit 7): the same 32 bytes as the first, saved, re-posed
  and put back around each view's walk.

Each writes through a per-key table that the load clears and that registration fills only with transform nodes. No
path into a mesh node was found. If one of them did point at a mesh, its next write would put a matrix over that
mesh's vertex, normal and primitive pointers, as seen.

### 28.4 What would name it (for the port's stopgap)

The port's stopgap already logs the entry. These three steps turn the next log into the cause:

1. **The object.** The draw entry (32 bytes: node, t[3], shift, object, matrix slot, object number) holds the object
   being walked at +0x14. Match it against:
   - `humanCarObjects` and `aiCarObjects`;
   - `fxRecords[i].f34` and `.obj`;
   - `weaponRicochetPool.slots`;
   - `darkToothBossObject`, `weaponTornadoObject` and `weaponStrikeState.object`;
   - the debris chunks, the mines and the pickups.
2. **The file.** Find the mesh address in the level file (0x80019C40, the file's size) or in a car group
   (`portDmdImages.base[1..]`). Its offset, read from the disc file, gives the mesh as the file has it.
3. **The bytes.** Compare the mesh's 44 bytes in RAM with the disc's:
   - a matrix at +0x04..+0x15 points to a writer in 28.3, and a write watch on that header next time names it
     exactly;
   - for anything else, send the 44 bytes and the node's id.

An aside from the watchdog registers: a3 = 0x8A4 is the mine's asset id (2212). If that is the bad mesh's id, the
entry is a mine. Players drop mines, so they appear only in real fights; check this first.

## 29. Request 111: floors that look solid and are not

### 29.1 Antarctica: the floes at the start (CONFIRMED, built: `TM2_PORT_WHOLE_MAPS`, `RETAIL_BUGS.md` 41)

Retail pairs a file with a rule:
- in modes 2 (the co-op tournament) and 4, the load takes `GLACIER2.DMD`, which has no floes 0..5;
- `worldCollapseClearDone` (80116310, from `arenaLoadAntarctica`) then takes the collision of floes 0..5.

Since 2026-11-05 the port loads the whole `GLACIER.DMD` in those modes (`vplus/fixes.c` `whole_map_name`). So floes
0..5 were drawn with no ground under them from the first frame, and both cars fell through drawn ice. It is not the
state transfer: every machine loads the same file and runs the same rule.

Retail's own timing, for the other question:
- **Start.** The collapse cannot start until about 18 s into the level: its timer starts at -720, and the first
  trigger needs 361.
- **The fall.** During the rumble and the fall, the drawn floe and its collision move together each step
  (`worldCollapseMoveSection`, `worldSectionAddHeight`).
- **The release.** `worldCollapseRelease` hides the floe (child count 0) and drops its collision to -80 in the same
  step.

What the state covers:
- **In RAM and in the state:**
  - the timers and flags, 80180F58..68;
  - `worldCollapseSections`, 80187448;
  - the sections (801A9848) and surfaces (801B0418);
  - the node heights and counts.

  Nothing is in the scratchpad or VRAM.
- **Outside the state:**
  - which file was loaded;
  - any copy the port's renderer keeps of node positions (reset it when a state is loaded).

### 29.2 Los Angeles: the deck by the curved freeway (CONFIRMED mechanism; the place not found)

**How the ground works.** Ground is separate from drawing:
- a car stands on the sections and surfaces of `TERRAIN\FWY.GRP` / `FWY.RMP` (`worldSectionAt` 800EFF34,
  `carWheelProbe` 800EDBA8, `carWheelOverSurface` 800EE5F0), never on the drawn `FREEWAY.DMD`;
- a section's floor height applies only with its floor flag (801AAA3D + section);
- model hit boxes are walls only.

**The earthquake.** In play, only the earthquake changes LA's ground:
1. A remote bomb goes off, or is driven into, in section 6 or within 800 of (0, 0, 0) or (0, -42, 240)
   (`laEventsTrigger` 800D5584).
2. `laEventStart` then disables section 6 at once (floor 32000, flag off). Section 6 is the 256 x 1800 strip at
   height 240 through the map's centre (x -128..128, y -880..920).
3. Its drawn piece (node 4006) is hidden 20..83 frames later (`laEventsUpdate`, the store at 800D5CAC).

So in retail the ground goes first, briefly. That strip is not by the curved freeway.

**The disc's drawn flat tops above height 120, against the collision:**
- inside the playable sections every one is solid, except:
  - the curved freeway's parapet tops (height about 440, x 6400..8400, y 3700..5700);
  - a 576 x 768 patch at height 528 (x -192..192, y 816..1392);
- ~~one drawn deck has no surface of its own, south of the boundary; surface 79 has nothing drawn~~. **Wrong
  (corrected for request 112, section 30):** both came from one error in my model walker, which turned transform node
  8006C9CC's matrix the wrong way and scaled its offset. Read as the game does, that node draws the curve's west
  half exactly on surface 79. There is no deck south of the boundary.

**For the port.** When a wheel drops, log:
- the car's position;
- each wheel's section and surface;
- the floor flag (801AAA3D + section);
- section 6's floor height (801A9854 + 12 * 6 + 8): 32000 means the quake ran.

Then compare the position with the places above.

### 29.3 Los Angeles: the far skyline sliding (CONFIRMED data, LIKELY cause)

**What the skyline is.** Flat textured vertical quads (FT4) in the level model: not sprites, not LOD swaps, not a
backdrop (the sky is the camera-attached model, root id 2). They form layers (meshes 8002F028..8002F4E0):

| Distance | Height |
|---|---|
| about 3,400..4,500 | 250..500 |
| 5,000..10,900 | 1,000 |
| 12,000 | a ring of 1,200-high panels |
| 14,800 | |

They sit under a scale node (type 12, 8002F770) with **shift 1**, where the rest of the level has shift 3. Two more
cards are under select node 80036318. Many have rotated or mirrored UV layouts (8002F028's u runs up its vertical
edge).

**Possible causes, likeliest first:**
1. A port projection path that takes shift 3 instead of the draw entry's own shift (entry +0x10, here 1). These
   layers would then move at the wrong rate against each other as the camera moves.
2. The flipped-texture fix (as on Hong Kong's temple wall) misreading the rotated or mirrored panels.
3. True parallax between the layers, which retail shows too (compare with the PS1 or a plain emulator).

Sorting is unlikely: the layers do not cross, and the OT bias acts near the camera only.

## 30. Request 112: ground for the parapet tops in Los Angeles (CONFIRMED; built, then removed: unreachable, `RETAIL_BUGS.md` 42)

**The correction first.** 29.2's "deck south of the boundary" does not exist. Type 5 nodes give a child frame of
`transpose(R)` and an offset in vertex units (`FILE_FORMATS.md` type 5). My walker used `R` and shifted the offset
by the level's shift 3. That put the curve's west half (meshes 8005ECF8, 800603F8, 80061E3C under node 8006C9CC,
rotation [0 1 0; -1 0 0; 0 0 1], offset (8064, -2688)) south of the map, and left surface 79 looking empty. Read
correctly, the west half lies on surface 79, and the whole deck, both halves, is solid. The skyline (29.3) has no
type 4 or 5 node above it, so 29.3 stands.

**What is not solid.** Over the whole level, every drawn flat top inside the playable sections has ground, except:

| Drawn top | Height | Where | Ground |
|---|---|---|---|
| the outer parapets of both halves | 440 | radius 3104..3282 round (5376, 2688), angles 186..838 and -838..-186 | none: the road (0..96) |
| a 576 x 768 patch in the middle | 528 | x -288..288, y 720..1488 | section 6/7 at 240 |
| the tops of two posts and a beam over the road by the curve | 208..248 | x 4169..6583, y 3246..3685 | the road (0) |

The gap between the two halves (angles -186..186) has nothing drawn and no ground: retail's jump.

**Removed on 2026-11-10.** These are the tops of the hedge walls along the tunnel roof's outer edge. Their hit boxes
stand higher than the drawing, so no car can get over them (the user tested it), and they need no ground. The
two surfaces below were in decomp PR #129 and are gone again. The rest of this section is the record of what was
built.

**Built (#129): the parapet tops.**
- Two round surfaces at 440 go in right after section 1's last surface (84); the later surfaces and every section's
  range move up with them.
- The new ground covers radius 3145..3282, starting where the deck's surfaces 79 and 80 end (3144). So the disc's
  ground is unchanged wherever it has some: `worldGroundHeightAt` probes 120 up and still finds the deck by the
  wall, and a wheel's step (40..80) is less than the parapet's 104, so no car on the deck climbs one.
- A car on the inner 40 of a parapet top (radius 3104..3144, over the deck's own edge) stays on the deck at 336.
- Always on with the define: every mode and player count, and the same on every machine. It lives in the surfaces
  and the section table, which are in the rollback state with the rest of the ground. The earthquake changes
  section 6 only.

**Not built:** the 528 patch and the post tops. Neither is asked for, and both are above ground a car only reaches by
a jump. Say if they should be solid too.

**Checked:** `check_custom.py` check 24 runs `arenaLoadLosAngeles` and the link's `worldGroundHeightAt` over the
disc's files. It finds 440 on every parapet top point and the disc's own height at every other point it tests: the
deck, by the wall, the gap, under the curve, past the parapet, and a grid over the whole map.

## 31. Request 112 follow-up: the car sinking into the concrete by a curve (the playtest video)

**What the video shows.** A car on the light concrete lane by the map's wall drives into a curve; the raised
concrete under the orange-and-white barrier rises round it and the car sinks into it, then drives on inside it.
That is not section 30's place (the elevated curve's parapet tops, 440 high): it is at ground level, at the end of one
of the three ground-level corner curves (`RETAIL_BUGS.md` 43). The video does not show which corner; all three have
the same mesh and the same fault.

**What is solid and what was not** (the drawn tops against `worldGroundHeightAt`'s model, a polar grid every 24 of
radius and 6 of angle round each corner):

| Part of a corner curve | Before | After |
|---|---|---|
| the road and the bank (the steep curve) | solid, as drawn | unchanged |
| the outer lip at both ends (radius 2965..3270) | up to 65 under the drawing | follows the drawing |
| the inner kerb (radius 2234..2400) | up to 35 under (48 at the north-west) | follows the drawing |
| the wall face at the lip's outer edge | a wall | a wall (no ground added on steep faces) |

So the bank can be driven up and jumped off, as in retail, and now the lip at its ends and the kerb carry the car at
their drawn height. Points more than 16 under the drawing: 660 / 432 / 444 before, 13 / 17 / 36 after (north-west,
south-east, south-west); about 12 per corner now have ground up to 30 above the drawing, at the lip's outer edge.

**Built** (`TM2_PORT_FWY_GROUND`; `TM2_PORT_FWY_PARAPETS`, section 30's name, still works): `portFwyGroundAdd`
(arena_load_register.c) inserts 79 round surfaces made from the drawn polygons (merged along the curve where the
merged surface stays within 12 of their corners). 207 of the 225 records are used. Every machine
loads the same, and the surfaces and the section table are in the rollback state.

**Elsewhere on the map** (the same comparison over the whole level): the elevated curve's parapet tops (section 30,
built); a raised patch in the middle at 528 and the tops of posts and a beam under the elevated curve (208..248),
reachable only by a jump, not built. Everything else drawn as a top has its ground.

**Checked:** `check_custom.py` check 24 runs the link's `arenaLoadLosAngeles` and `worldGroundHeightAt` on the disc's
files: the 225 records exactly as the table gives them, the parapet tops at 440, 48 lip and kerb points within 12 of
their drawn height (all at least 16 under it on the disc), the disc's height everywhere else it tests.

## 32. Request 115: falling off New York and Antarctica (no change needed: the car dies at the bottom at once)

Asked 2026-11-14 from the 16-player online Versus take (video 12:24-12:32): the car falls off a roof, lands far below,
and "its HP runs down over about 2 s (129, 89, 7, 0)" before "<player> WAS WRECKED".

**What the video shows (CONFIRMED, frames at 2 and 6 per second).**

- The number that reads 129, 121, 61, 21, 0 is the port's **speedometer** ("Your speed, near the weapons"), not
  health. Retail's HUD has no health number.
  - It holds at 129 all through the fall: the car is in the air, so its speed doesn't change.
  - "<player> WAS WRECKED" appears at about 12:30 while it still reads 121, and the health bar empties in the same
    frame.
  - The number then runs down as the wreck stops.
- The time is the fall itself, about 3 s from the roof (12:27) to the bottom (12:30). The camera stays at the roof
  because a falling car locks it (`state.falling`, `camera.c`). The explosion happens thousands of units below the
  camera, a few pixels on screen, so it can look like the car just landed.

**The code (CONFIRMED).** `humanCarTick` (800DC88C) and `aiCarDrive` (800E7EDC):

- once a car is below `worldFallLimitZ` (8011EC0C: 6,000 on New York and Roof Tops, 3,840 on Antarctica), its falling
  flag is set;
- while the flag is set, the car gets HP -1 the frame its height passes `worldFallDeathZ` (80). With
  `TM2_PORT_FALL_WHEELS` (request 107, `RETAIL_BUGS.md` 40) the wheels' height is used, so Dark Tooth too;
- the next damage update destroys it (`humanCarDamageUpdate` 8011B89C, `aiCarDamageUpdate`).

`carDestroyed` then:

- **Antarctica:** below height 240 the car is hidden and splashes (`fxSplashAt`, sound 34). A car dies there with its
  body at most 80 plus its ride height (196 for Dark Tooth), so a fallen car always splashes.
- **New York and Roof Tops:** it explodes where it is.

That is the original's behaviour; nothing in the port changes it.

**Can a fallen car survive? No (CONFIRMED, the disc's `TERRAIN` files).** A car below the fall limit lives only if it
lands on ground at 80 or higher.

| Level (files) | Fall limit | Lowest ground other than the bottom | Ground below the limit |
|---|---|---|---|
| New York (`ROOF.GRP` / `.RMP`, 24 sections, 26 surfaces) | 6,000 | 6,360 (section 7) | only section 23, the 64,000-unit catch-all floor at 0 |
| Antarctica (`ICE`, 35 / 141) | 3,840 | 4,000 (section 33, the east ramp) | only section 34 (catch-all, 0) and section 8 (floor 0, under the south floes' surfaces) |
| Roof Tops (`SROOF`, 14 / 10) | 6,000 | 6,960 (section 5) | only section 13 (catch-all, 0) |

Every section floor and surface corner was read, and none lies between 80 and the fall limit. Antarctica has no
water ground class in its GRP: the sea is the floor at 0. So past the limit a car can only reach the bottom, and
dies there the moment its wheels pass 80. The one car that could live on the bottom in retail was Dark Tooth (ride
height 116, body at 116 over a floor at 0). `TM2_PORT_FALL_WHEELS` already fixed that, and the port's build has it.

**If a car is still seen alive at the bottom:** that is not this path. Note the time, the level and the car. The things to check:

- whether the build has `TM2_PORT_FALL_WHEELS`;
- whether the car's state was restored online with `falling` but a body above the limit;
- whether the round ended during the fall, with the car still in the air (not checked here).

## 33. Request 117: two cars that fell into Antarctica's crevasse bounced on its floor (built: `TM2_PORT_WRECK_FALL`)

Asked 2026-11-16 from the 16-player online Versus take (video 15:38-15:52, Antarctica, the boss round's last two
cars): player 1's car and Krista Sparks's go over the crevasse's edge together, and both wrecks land on the floor and
bounce there: no splash, not hidden.

**The one path that hides a car (CONFIRMED, `carDestroyed` 8011BE10, `car_destroyed.c`).** A car's end is decided
once, when it dies: `humanCarDamageUpdate` / `aiCarDamageUpdate` call `carDestroyed` the frame its HP reaches 0, and
it looks at where the car is then:

- Antarctica (level 6) with the body below 240: drawing off, `fxSplashAt`, a human's sound 34, wreck timer 0;
- level 8 below -40: drawing off;
- anything else: the explosion (smoke, spin out, `fxExplosion48` / `fxExplosion24`, the wreck sound) where it is.

It needs nothing else: human or computer, any cause of death. But a **human's** car gets nothing at all once the round
is won: `carDestroyed` returns at once while `roundWonFlag` is set (retail: the round won, nothing can hurt a human;
the port: `portRoundEnd` with `TM2_ROUND_FREEZE` sets it). Nothing looks at a wreck again: a car that dies in the air
explodes in the air, and its wreck falls on and lies on the floor.

**Why neither car got it (the port's match log).**

- `lives: player 1 is out`, then `2 players: the round is over`, and only after that
  `kill: <player 1> killed KRISTA SPARKS, attack 12`, `lives: player 6 is out`.
- **Krista Sparks (CONFIRMED by the order):** a human's car (player 6 drove the boss), destroyed by the hit after the
  round had ended. `carDestroyed` returned at once: no splash, no hide, not even the explosion. Her wreck fell on.
- **Player 1's car (LIKELY):** it went out before the round end, so `carDestroyed` ran. Had it died of the fall, it
  would have been below 240 and splashed (section 32). It did not splash, so it was destroyed in the air by damage (the
  fight; no kill line was logged for it) and exploded there.
- The explosion near the floor at the replay's end: not explained by these two paths (Krista's car had none); the
  log does not say which effect it is.

Retail has both holes too: a car killed in the air over the crevasse (or a roof's edge) never splashes, and a human
who falls in after winning the round lies on the floor. `RETAIL_BUGS.md` 44.

**Built: `--port-define TM2_PORT_WRECK_FALL`** (`include/semantic/port_wreck_fall.h`; port variant only). After the
fall test, every frame, `humanCarTick` and `aiCarDrive` call `portWreckFall`:

- a wreck still drawn on Antarctica below 240 is hidden exactly as `carDestroyed` hides a car there (the splash, a
  human's sound 34, the wreck timer at most 0); level 8 below -40 likewise, without a splash;
- a falling wreck that passes the fall death's own test (`state.falling` and the height, from the wheels with
  `TM2_PORT_FALL_WHEELS`, under `worldFallDeathZ`) and has not exploded there explodes as `carDestroyed` explodes a
  car: on New York and Roof Tops a wreck that falls to the floor now explodes there, once, whether it blew up in the
  air or got nothing;
- `carDestroyed` records whether its explosion was at the floor (`portWreckFallBlown`), so a car that dies of the
  fall explodes once, as before;
- a live car, a wreck on a roof, a car already hidden and a parked car (not drawn) are left alone.

The mark is one byte per car slot, `portWreckFallMark` (`.port_bss`, match state: snapshot and hash, as the rest of
`.port_bss`). The effects draw on the game's `rand()` as `carDestroyed`'s do; the port code calls none itself. Same
frame on every machine. `check_custom.py` check 28 (frame-by-frame runs of both functions on a human and a computer
record). **Not tested in game.**

## 34. Request 120: Antarctica's fire missile is never picked up (CONFIRMED retail; built: `TM2_PORT_GLACIER_FIRE`)

Asked 2026-11-18 from a single-player playtest. The details are in `RETAIL_BUGS.md` 45.

- **Cause:** an invisible mark volume covers the pickup, not its node layout.
  - The level's hit-only mark `800561E8` (the slab 4800 < z < 6400 over part of the floes) is walked first.
  - It marks every car point inside it, and the fire pickup's leaf then skips those points.
  - The fire missile sits about 200 inside the mark; the power and homing missiles lie outside it.
- **Why a static walk saw it:** at shift 0 the pickup is reached. The level's shift is 3 (GLACIER.DMD +0x10,
  `renderSetWorldShift` -> `hdpSetDifficulty`), and at shift 3 it is not.
- **The suggested relink** (`80051B40` straight under the floe) changes nothing: the pickup is still inside the mark.
- **Built:** `portGlacierFire` (`--port-define TM2_PORT_GLACIER_FIRE`) moves the pickup and its LOD's centre to floe-local
  (992, 119), z kept.
  - This is (600, -800) turned by the floe's 60 degrees, which is LIKELY the spot meant.
  - It lies by the other two, on the same ground (section 5, 4800).
  - It runs at the load, on level 6, before the pickups are registered, so the pickup's respawn spot follows.
  - It is the same on every machine, with no rollback state (the level data).
  - Collected in 1,115 of 1,352 test placements (a model of the walk); the mark itself is left alone, since it shapes
    the collisions over a large area.
- **Floe 3's buried fire pickup** (`8004F0E8`, 4,800 below its floe, inside another mark): left as retail.
- **Checked:** `check_custom.py` check 30 (the patch on the disc's file, and nothing on other files or patched twice).
  **Not tested in game.**

