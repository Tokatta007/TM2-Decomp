# Far LOD: everything visible at a distance

Answer to the PC port's request 30 ("Draw distance: Farthest": no pop-in of the arena, pickups and
cars). Sources: the walker's asm and its C form, the hit walker, and the 29 model files
`DMD\*.DMD` on the USA disc, dumped with `tools/dmd_lod_dump.py` (section 7). Tags: **CONFIRMED** =
read in code or counted in the data; **GUESS** = inferred, not run.

## Summary

- **What pops in is the model data, not a far cull** (CONFIRMED). Every one of the 3,674 drawing
  LOD nodes on the disc ends at a finite distance. Past it, retail draws nothing for that node:
  - level nodes: 1,425 end before 2,400 world units and 50 more before 4,800. The other 2,009
    end at 10,000 or 16,627, beyond the 4,800 far cull, so they never pop. Cyburbia's arena
    ends by 1,800, except 27 meshes;
  - car bodies end at 1,000 or less; only a few parts of Dark Tooth (`BB`) and `HS` reach
    1,500 to 3,000. Past that the car draws **nothing**;
  - 237 LOD nodes have a pickup below them. The pickup's own node has one record `[0, R)`, with
    R = 70 to 3,000 (Paris 375/425, Antarctica 400/600, Amazonia 400). Pickups without an LOD
    (Los Angeles, Moscow, Swamp, Hong Kong) are cut only by the level far distance (4,800,
    `DRAW_DISTANCE.md` 2).
- **Why `d2 = 100` made models vanish** (counts CONFIRMED, effect GUESS). The data is not the
  cause: every LOD set but one has a record holding 100. The cause is capacity. With the
  override every model in the whole arena draws its most detailed record out to the far
  distance. Amazonia's file then holds 3,999 meshes and 67,267 primitives. Its far bands hold
  249 and 1,262. Retail has room for **280 meshes per view** and a **97,520-byte packet
  buffer** per frame. The walker queues the level first and the object list (cars, the player's
  included) last, so the objects are the first thing dropped (section 3).
- **The switch, implemented:** `portFarLod`, an `int` in `.port_bss`
  (`config/US/port_variant_arrays.tsv`, group `one`). It takes effect only in a variant built
  with `--port-define TM2_PORT_FAR_LOD`. At 0 everything is retail. At any other value, while
  `renderScene` walks a view, every LOD node keeps drawing its farthest band past that band's
  end. Split LODs are handled: only the last band of a set stays. Hit detection keeps the retail
  records (section 4).
- **Value 2 (highest detail for objects) is not implemented** (section 5). It is correct for the
  data, but with the retail caps it overflows the mesh list and the packet buffer, and that drops
  the cars, the same failure as `d2 = 100`. It becomes safe once the port raises the caps.
- **Request 46 item 6: the near bands farther** (section 8, implemented): `portFarLodNearScale` 2..16 puts every
  LOD band at that many times its distance while a view is walked, so the sharp near bands (Hong Kong's detailed
  building textures) are kept farther.
- **Opcode 11 does not hide far geometry** (region boxes CONFIRMED, purpose GUESS). Its regions
  touch the geometry they switch, so from far away the camera is outside them. They look like
  occlusion zones: a part hidden while the camera is in a box beside it (above a roof, outside a
  tunnel). In Cyburbia, Hong Kong, Paris, New York and Roof Tops, zones swap between two drawing children,
  most likely two versions of the same part. A "keep both" option would double geometry and
  break the painter's sort. It is not implemented (section 6).

## 1. Opcode 2 (CONFIRMED)

The handler is in `renderWalkModel`'s asm, `src/semantic/engine_render_scene.c:726` (80152A14)
to `:769`. The C form is in `reference/renderer_walker_engine_render_scene.c:665`:

```
d2 = SQR((node.pos + t) >> shift)                 // world units squared, unsigned (GTE SQR)
if (ctx+0x24) d2 = 100                            // gp+1008 renderUnculledObject: renderInitState :119
for each record {max, min, child} in file order:  // count node+16, pointers node+20
    if (min <= d2 < max) { push child; break; }
```

- **Past the last record nothing is drawn.** If no record matches, the node simply ends.
- **The override** `gp+1008` (`renderUnculledObject`, 80180DC4) is set only by
  `renderDrawObjectUnculled` (`engine_anim_hit_volumes.c:343`, the preview draw) and cleared
  again after it. No other code reads `ctx+0x24`.
- **Other readers of the same records.** The hit walker (`hdpKind2` 8014EA60, C form
  `reference/hdp_walker_engine_hit_detection.c:509`; `hdpTestSphereBands`) reads the bands too,
  with the probe's distance. `dbscanHierarchy` (`engine_clut_anim.c:1201`) walks every record
  at load. So **the records must not be changed in memory** for good: that would change
  collisions.

## 2. What the data holds (CONFIRMED, `tools/dmd_lod_dump.py --summary`)

| | files | LOD nodes | drawing | top finite | no record at d2=100 | extended by the switch | LOD nodes over pickups | zone nodes |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| levels | 14 | 4,300 | 3,484 | 3,484 | 184 | 3,324 (at most 809, DISH) | 237 | 212 |
| cars | 15 | 208 | 190 | 190 | 52 | 138 (at most 13 per file) | 0 | 0 |

- **Non-drawing LOD nodes.** 834 of them hold only hit volumes (opcode 8): bands for collision,
  nothing to see. No drawing LOD node has an empty record.
- **Top distance of the drawing nodes:**

| file | top <1000 | <2400 | <4800 | >=4800 (mostly 10,000 or 16,627) | meshes / prims at d2 = 0 | past every record, retail | past every record, switch on |
|---|---:|---:|---:|---:|---|---|---|
| FREEWAY (Los Angeles) | 30 | 60 | 2 | 18 | 386 / 10,124 | 88 / 511 | 286 / 1,397 |
| DISH (Moscow) | 28 | 2 | 0 | 784 | 582 / 3,173 | 56 / 382 | 122 / 853 |
| PARIS | 78 | 33 | 1 | 152 | 397 / 6,304 | 56 / 299 | 252 / 747 |
| ISLANDS (Amazonia) | 53 | 11 | 9 | 122 | 3,999 / 67,267 | 52 / 414 | 249 / 1,262 |
| ROOF (New York) | 71 | 16 | 0 | 175 | 158 / 1,954 | 75 / 928 | 158 / 1,269 |
| GLACIER (Antarctica) | 69 | 8 | 20 | 80 | 548 / 8,273 | 57 / 360 | 356 / 1,561 |
| DEN (Holland) | 18 | 7 | 0 | 126 | 5,332 / 16,368 | 54 / 422 | 237 / 1,115 |
| HKONG (Hong Kong) | 30 | 34 | 1 | 150 | 555 / 12,885 | 60 / 327 | 200 / 848 |
| SWAMP | 96 | 24 | 0 | 0 | 140 / 1,411 | 42 / 229 | 91 / 380 |
| BURB (Cyburbia) | 218 | 156 | 0 | 2 | 439 / 11,276 | 27 / 240 | 439 / 3,775 |
| SROOF (Roof Tops) | 39 | 86 | 1 | 32 | 301 / 5,931 | 30 / 326 | 45 / 541 |
| ST (Sweet Tooth) | 6 | 3 | 0 | 0 | 2 / 70 | 0 / 0 | 2 / 18 |
| AX | 13 | 5 | 0 | 0 | 3 / 135 | 0 / 0 | 3 / 14 |
| BB (Dark Tooth) | 5 | 4 | 2 | 0 | 19 / 407 | 10 / 162 | 16 / 207 |

  The first four columns count drawing LOD nodes by the distance their farthest record ends at.
  The cost columns count the whole file, every root, with zones on their outside child,
  animations on their current frame and selects on their current child. They are upper bounds:
  frustum culling and the level far cull keep only part of it.
- **Split LODs.** Most car files and some level parts keep each band in a node of its own, under
  one group, e.g. Axel `8001FF64`: `[0,150) | [150,300) | [300,1000)`. Other sets overlap, e.g.
  Moscow `8007711C`: `[0,200) | [0,100) | [100,300)`, a detail part over two bands. So "the last
  record" of one node is not always the end of the model.
- **Pickups.** Each is its own single-record LOD node `[0, R)` holding the pickup (1-2 meshes).
  In Cyburbia they also sit under LODs of the arena part (500 < 1000 < 1800) and under its zone.
  The switch extends all 237.
- **Cars** (`ST`, `RK`...): the body bands are `[0,200) | [200,500) | [500,1000)`, in one node or
  split over three, so the car draws nothing past 1,000 units. With the switch, the 500-1000 band stays, which is about
  18 primitives for Sweet Tooth.

## 3. Why the port's `d2 = 100` made models vanish (counts CONFIRMED, effect GUESS)

- **It is not the data.** Every LOD set has a record holding d2 = 100, except one (Swamp, a
  `[200, 2000)` part). The 184 + 52 nodes without such a record are far bands of split sets
  whose near band holds 100.
- **It is capacity.** The override draws the nearest record of everything that survives the
  frustum and the 4,800 far cull. Compare the "d2 = 0" and "switch on" columns above: 3 to 15
  times the primitives of the far bands on most levels, 53 times on Amazonia. Retail has room for:
  - 280 queued meshes per view (`slti $20,$19,280`, `engine_render_scene.c:649`, 801528EC;
    the queue `renderMeshList` is 280 x 32 bytes);
  - 97,520 bytes of packets per frame, shared by both views (`DRAW_DISTANCE.md` 9). That is
    about 2,000 textured polygons. The emitter stops when it is full.
- **Why the cars go first.** `renderScene` walks the level model and world models (group 1)
  before the object list (group 3, `engine_render_scene.c:204`). The meshes past the cap are
  the last ones, so the cars, the player's included, vanish first.
- **What follows.** Keeping detail at a distance needs bigger caps whatever rule picks the
  record. The switch below keeps the cheapest record (the farthest band).

## 4. The switch: `portFarLod` (implemented)

**Values:** 0 = retail. Any other value = past every record, an LOD node draws its farthest band.
There is no separate value 2 (section 5).

**Rule** (`portFarLodNode`, `src/semantic/port_variant_arrays.c:290`):
1. A node's farthest record (largest max, the first of equal ones) is **extended** to an
   unbounded max when:
   - its max is finite;
   - it reaches a mesh;
   - **no drawing record of its set starts at or beyond that max**.
2. A **set** is either:
   - the LOD children of a node whose children are all walked (opcodes 1, 4, 5, 12); or
   - a single LOD node: a root, or a child of an LOD, animation, select or zone node.

So in `[0,200) | [200,500) | [500,1000)` only the 500-1000 node is extended. In
`[0,200) | [0,100) | [100,300)` the `[0,200)` detail and the `[100,300)` base both stay. An
extended record behaves as "past every record, draw this one": its min is unchanged, and no
other record of the node reaches beyond its old max.

**How:**

| step | where | what |
|---|---|---|
| load, level | `worldLoadStartObjects`, `rt_main.c:391` | `portFarLodScanLevel()`: forgets the previous match, scans the roots at 0x80019C5C (`port_variant_arrays.c:341`). The shell's `shellLoadTmsDmd` calls it too, so a shell screen's models replace the match's list |
| load, car files | `assetInstallImage`, `asset_header.c:40` | `portFarLodScan(object)` after `modelRelocate` (`:329`) |
| each view | `renderScene`, `engine_render_scene.c:150` / `:218` | if `portFarLod != 0`: `portFarLodBegin()` writes 0xFFFFFFFF into each listed record's max (`:355`); after the object list, before the emitter, `portFarLodEnd()` puts the retail max back (`:373`) |

- **At 0, retail is unchanged.** `renderScene` loads `portFarLod` and branches past both calls.
  The loads only fill a table (`portFarLodTable`, 2,048 entries; a level needs at most 809 and a
  car file 13).
- **Hit detection** and `dbscanHierarchy` never see an extended record: they run outside
  `renderScene`. `renderDrawModel` (menus, ending) and `renderDrawObjectUnculled` are not
  wrapped.
- **Stale memory.** The model area is reused between matches. `portFarLodBegin` extends a
  record only while its max still holds the value seen at the scan. `portFarLodEnd` restores
  only the records it extended (bit 0 of the table pointer).
- **Why not in the walker.** A per-node rule in opcode 2 ("no match → draw the last record")
  would draw all three bands of a split set at once past 1,000: the node cannot see its
  siblings. It would also need new code in the hand-written walker asm, and the walker's
  interior labels (the node handler table, `renderInitNodeHandlers`) cannot move in the
  variant. Patching at load would change collisions (section 1). The per-view patch needs
  neither.
- **Cost (GUESS until measured).** The extended bands are the cheapest records. The "past every
  record, switch on" column is what now draws beyond the old ends, in the whole file. Retail's
  caps still apply. Cyburbia, the worst case (439 meshes / 3,775 primitives in the file), may
  reach the 280-mesh cap at a far viewpoint. When it does, the cars are dropped first
  (section 3). **The port should raise the mesh cap and the packet buffer before shipping
  Farthest**, or at least measure `renderCtx->end` (0x1F80003C) on Cyburbia, Amazonia and
  Antarctica.

## 5. Value 2: highest detail for objects (not implemented)

**What "objects" are in the data:**
- **Cars and weapons.** Everything in the car files (`AX`, `BB`, ... two-letter stems): the car
  root (id = type x 16), its parts, and the projectile/special assets (ids 2xxx, 249, 309). At
  run time these are the object-list models (group 3), walked with `ctx+0x28 = 1`.
- **Pickups.** In the level file: nodes with id 401..430 (441..480 for the rotating bonus), kind 1
  or 4 (`PICKUP_LOOKS.md` 1). They have **no LOD of their own beyond the single `[0, R)` record**,
  so for them value 1 already is full detail at any distance.
- So value 2 would change only the car files: draw what retail draws at d2 = 0.

**Why it is not implemented:**
- **Cost.** It is correct for the data: every car set covers d2 = 0. But it multiplies the
  objects' primitives at a distance: Dark Tooth 407 instead of 207, `AX` 135 instead of 14,
  Sweet Tooth 70 instead of 18 (d2 = 0 against the switch's far bands). With 8 cars (Antarctica
  tournament) and projectiles that is roughly +500 to +1,500 primitives.
- **Who pays.** The packet buffer holds about 2,000 polygons for both views. The objects are
  queued last, so they are what gets dropped: the `d2 = 100` failure again, only for cars.
- **When it becomes safe.** Once the port enlarges `renderMeshList` (280), the packet buffers
  (0x17CF0 each) and the emitter's end check. Then it can be added cheaply: a second table of
  "near" records for car-file roots (from `portFarLodScan`'s caller, `assetInstallImage`), and
  in `portFarLodBegin` each car set's d2 = 0 record stretched to `[0, far²)`.

## 6. Opcode 11: camera regions (mechanism and data CONFIRMED, purpose GUESS; no option)

**Mechanism.** The handler is `engine_render_scene.c:1132` (80153048), C form
`reference/renderer_walker_engine_render_scene.c:736`:
- -t, the camera in the node's frame, is tested against each region (count node+6, pointers
  node+16).
- A region is a box of up to 6 planes: rows at +4, offsets at +24; for type > 3 a second set at
  +36 / +56.
- Camera inside any region: the walker draws child +12. Otherwise it draws child +8. Flag
  node+4 bit 1 skips the node.

**What the disc's 212 zone nodes do:**

| level | zones | hide (out draws, in empty) | show (in draws) | swap (both draw) | nothing to see |
|---|---:|---:|---:|---:|---:|
| FREEWAY | 22 | 0 | 0 | 0 | 22 |
| GLACIER / GLACIER2 | 12 / 9 | 0 | 0 | 0 | 12 / 9 |
| PARIS / PARIS2 | 19 / 19 | 0 | 0 | 7 | 12 |
| ROOF / ROOF2 | 49 / 45 | 33 / 32 | 3 | 13 / 10 | 0 |
| HKONG | 12 | 0 | 3 | 9 | 0 |
| SWAMP | 6 | 1 | 5 | 0 | 0 |
| BURB | 1 | 0 | 0 | 1 (306 vs 644 meshes: two versions of the arena) | 0 |
| SROOF | 17 | 1 | 7 | 9 | 0 |
| ISLANDS | 1 | 0 | 1 | 0 | 0 |

**Findings:**
- **The regions touch what they switch.** Of the 201 pairs of a region and a drawing child
  with a bounding sphere, 198 boxes touch or overlap that sphere. The other 3 (New York) lie at
  most 298 coordinate units (37 world units) from it. For example, New York `8002F710` hides a
  part centred at z 7120 (radius 3666) while the camera is in a box beside it (z 7600..9600).
- **They are not distance culling.** From far away the camera is outside these boxes, so a hide
  zone draws its piece. Nothing seen from a distance is hidden by opcode 11. That the boxes stand
  for occlusion (the part cannot be seen from inside the box) is a GUESS from their placement.
- **Keeping both children is not safe:**
  - swaps would draw two versions of the same geometry (Cyburbia: the whole arena, 306 + 644
    meshes at d2 = 0);
  - hide zones would draw interiors through roofs. The PS1 path has no depth buffer, only the
    ordering table's coarse sort, so occluded polygons can show through;
  - New York's zones hold 2,280 meshes / 34,074 primitives on their outside children (d2 = 0),
    about 8 times the mesh cap.

  **No option is added.** Pickups under zones (Hong Kong 47, Cyburbia 48, Roof Tops 12, New
  York 2) follow the same occlusion design.

## 7. For the port

**Build:** add `--port-define TM2_PORT_FAR_LOD` to the variant build (`tools/build_source_only.py
... --port-variant --port-define TM2_PORT_HOOKS --port-define TM2_PORT_FAR_LOD`). It changes three
functions, which the placement check accepts as `port_changed`:
- `renderScene`;
- `worldLoadStartObjects`;
- `assetInstallImage`.

It also adds 11 `.port_text` functions (`portFarLod*`, `port_variant_arrays.c`).

**Set it:** write `portFarLod` from portConfig: 1 at Farthest, 0 otherwise. Its address is
**0x80205F94** in the 4-player variant (`.port_bss`, after the HUD arrays; `portFarLodTable`
follows at 0x80205F98). Take it from `out/TM2_SOURCE_PORT/symbols.tsv` or the json: it moves with
`--port-limits` and with any storage added before it. It is defined in every variant build, with or
without the define; without it nothing reads it.
- `portFarLodTable` (16,388 bytes) is in every variant too, so `.port_bss` grew by 0x4000 and
  `.port_text` now starts at 0x80209FA0 (was 0x80205FA0): the default variant's EXE changed
  (`worldUpdate`'s call to `worldRadarPlotHumans`), not its code.
  Request 31's storage (`FAR_DISTANCE.md` 5) follows it: `.port_text` is now at 0x8039FA00.
- It can change at any time. It acts from the next view drawn.
- A match loaded while it was 0 is scanned anyway, so turning it on mid-match works.
- `portFarLodTable` must be zero like the rest of `.port_bss`.

**Also do:**
- raise `renderFarDistanceDefault` (`DRAW_DISTANCE.md` 12) so that the arena past 4,800 is not
  cut by the far cull;
- raise the caps of section 3.

**Not tested at run time:** the variant links and passes its static proofs (`SOURCE_BUILD.md`
"Port variant"); nothing here has been run. The equivalence of the C rule and the tool's rule
was checked only by reading.

**The tool:**
```
py -3.14 tools/dmd_lod_dump.py --summary              # one line per file + totals
py -3.14 tools/dmd_lod_dump.py --files ST,PARIS       # every LOD node: records, flags, `ext`
```
It reads the disc image in `cdimage/` and writes nothing, unless `--out` is given.

## 9. Request 62: Moscow at Farthest, the ground and the fence gone with the camera outside the fence (no code)

**Asked:** the camera stood outside Moscow's outer fence for about 6 frames (60 FPS) after the car spun against it;
the ground near the camera and the fence vanished (the clear colour showed), the skyline and the far fence stayed.

**(a) Not the region switches** (CONFIRMED). Moscow's level file (`DISH`) has **no opcode-11 node** (section 6's
table; `tools/dmd_lod_dump.py --summary`: zones 0). Its tree: 1,143 groups, 814 LOD nodes, 58 offsets, 40
animations, 11 selects, 84 hit volumes, 1,560 meshes. What retail's opcode 11 does with an eye outside every region
(other levels): it draws child +8, the "outside" child (`renderWalkModel`, C form
`reference/renderer_walker_engine_render_scene.c` 759); there is no "none" case except a zone whose +8 is empty (the
"show" zones, 22 on the disc, none in Moscow).

**(b) Not the far features either** (CONFIRMED by the data):
- the GTE range pass never scales a Moscow level mesh: from any eye within 1,300 x 1,200 units of the centre
  (well outside the fence) every level mesh's far point is under 3,450 world units, below the 4,096 where k = 1
  starts;
- `portFarLod` only extends the farthest band of a node, and `portFarLodNearScale` scales all bands of a set alike;
  none of Moscow's 814 LOD nodes has a gap at d2 = 0 (every set starts at 0), so a near piece always has a band.

**What it is: retail's level and camera** (geometry CONFIRMED from the file, the match with the video GUESS).
- Moscow's ground ends at the outer walls: a 40-unit grid of the level's horizontal polygons (all LOD bands) has no
  ground cell outside them, only walls on the boundary.
- The outer walls are one-sided: every wall polygon at the four extremes has the emitter's back-face cull on
  (`ctl` bit 31 clear: NCLIP) and faces into the arena (`z` is up: the ground at z 80, the walls' centres at
  112..282). From outside, the near fence is a back face and is dropped; under and around the camera there is no
  ground; the far fence and the sky dome (root 1) face the eye and stay.
- The chase camera has no wall test: `viewChaseCamera` (80102CC4, `camera.c`) eases toward its goal behind the car
  and only keeps it at or above the car's height and 40 units above the car in car space. A car spun against the
  outer fence can put it outside, in retail too: there for about 2 steps (20 a second), what the port shows for 6
  frames at 60 FPS. Retail's emitter, walker and level data draw the same picture there. Listed as
  `RETAIL_BUGS.md` 39.

**Offered, not built (needs the user's decision):** a port bit for the chase camera that keeps the eye inside the
arena: test the segment from the car to the camera goal against the level's walls (the hdp volumes the cars collide
with, `worldGroundHeightAt` / `hdpCollideObject`) and stop the camera in front of the wall, 0 = retail. The camera
fields are presentation (`rollback_regions.tsv`: the human cars' camera rows are hash no), but `viewChaseCamera`
runs with the game's update, so it would need the check 8 / 9 census like any step-reachable change. A drawing-only
alternative (draw the outer walls' back faces while the eye is outside) would still show the void under the camera.

## 8. Request 46 item 6: the near bands farther (`portFarLodNearScale`, implemented)

**Why** (`PLAYTEST_ANSWERS.md` 13.1): Hong Kong's buildings switch from a detailed near band (78 quads, 0.29 texels a
model unit, detail pages 141 / 147) to a box with one low-resolution picture a side (12 quads, 0.10) at 200..600
units; over the arena's 171 multi-band LOD nodes the near band has 2.4 times the texel density (median). The user sees
the blurry band until about a quarter of a block away.

**The setting:** `portFarLodNearScale`, an `int` in `.port_bss` (every variant; tsv row after `portCarDtParkMask`),
read live by `renderScene`; acts only with `--port-define TM2_PORT_FAR_LOD`.

| value | effect while `renderScene` walks a view |
|---|---|
| 0, 1 | retail bands |
| 2..16 (larger acts as 16) | every LOD record's `[min, max)` is `[min x s^2, max x s^2)` (d2 is squared distance): each band ends s times farther, the sharp band of a Hong Kong building at 600 (s = 2) or 1,200 (s = 4) instead of 300; an unbounded max stays unbounded, a product past 32 bits saturates at 0xFFFFFFFE |

**How:**
- The scans (`portFarLodScanLevel`, `portFarLodScan`; unchanged callers) also list **every** record of every LOD node
  with its retail band (`portFarLodNear`: count, 4,096 record pointers, maxes, mins; the disc needs at most 1,582 for a
  level (DISH) plus 33 per car file; `portFarLodNearNote` in `portFarLodNode`, once per record).
- `portFarLodBegin`: with s > 1, every listed record still holding the band seen at the scan gets the scaled band
  (bit 0 of its pointer marks it); then, with `portFarLod` set, the far extension as before, accepting a farthest
  record's max either retail or scaled. `portFarLodEnd` puts the retail bands back, then the retail maxes, before
  anything else reads them (hit detection keeps retail's bands).
- `renderScene`: `farLod = portFarLod | portFarLodNearScale` (its only change for this).
- Scaling all records of a node alike keeps every set's order, split sets included (all siblings scale alike), so the
  rule of section 4 still picks the same farthest bands.
- With `TM2_PORT_CAR_DROPIN` an unloaded car's records leave the list with it (`portCarUnloadGroup`).

**Cost:** the near bands are the expensive ones (Hong Kong: 12,885 primitives at d2 = 0 against 327 past every record,
whole file); s = 4 keeps them to 4 times their distance, which on Hong Kong, Paris or Amazonia can mean several
thousand more primitives at a far viewpoint: set `portMeshListCap` and `portPacketBufferBytes` to their maximum first
(`FAR_DISTANCE.md` 5). The near scale works with or without `portFarLod`.

**Checks:** `tools/check_custom.py` check 5 (with `TM2_PORT_FAR_LOD`): the only game function reading
`portFarLodNearScale` is `renderScene`, no game function names `portFarLodNear`, `portFarLodBegin` / `portFarLodEnd`
read it and reach no `rand()`; `tools/check_car_groups.py` check 12 lists it as an unload holder; the proof and the
placement check (`SOURCE_BUILD.md`). Not run.

## 10. Request 63: the chase camera kept inside the level (`portVisual` bit 5, implemented)

**Asked:** section 9's offer (`RETAIL_BUGS.md` 39): `viewChaseCamera` keeps its eye inside the level, every arena,
every view, drawing only.

**The bit.** `portVisual` bit 5, `TM2_VIS_CAMERA_WALL` (`port_visual.h`, `--port-define TM2_PORT_VISUAL`; 0 =
retail). A visual bit, not a `LOCAL_VIEWS` one: the camera is each machine's own and nothing in the step reads it
back as state (`ONLINE_ROLLBACK.md` 18: the human records' camera block is "hash no"). `portVisual` itself now has
its own row, snapshot and hash no (it was inside the `port.bss` range: each machine's drawing settings were hashed;
`ONLINE_ROLLBACK.md` 22).

**Where:** `viewChaseCamera` (80102CC4, `camera.c`) calls `portVisualCameraWall(c->cam_look, c->cam_pos)` right after
the look-at point is built and before the yaw and pitch are taken from `look - eye`, so the clamped eye also aims the
camera. All chase views (1-4 and the spectator view 7, which looks at the watched AI car) and every split-screen view
go through it. The fixed views 5 and 6 (`viewFixedCameraTrack`) do not move the eye.

**The wall data: the cars' own hit volumes** (CONFIRMED in the code and the disc data). The level model's hdp kind-8
leaves are convex boxes of 3 to 6 planes (`hdpBox`: two rotation matrices with offsets, inside when every plane value
is > 0). On Moscow (`DISH`, 84 volumes) the outer fence is a ring of eight 3-plane wedges past radius 3,432 position
units (unbounded in height) and the plateau beyond the bowl is 5-plane walls below z 600: a camera outside the fence
is inside a solid. Other levels use "mark" volumes (n[5] = -2: 15 + 36 on Los Angeles, 31 on Paris, 54 on New York,
32 on Hong Kong, ...), which in the cars' pass free the points they hold from the solids walked after them (passages
through solids). The render polygons are not used: one-sided and not closed, they cannot say inside from outside.

**The test** (`portVisualCameraWall` and six helpers `portCameraWall*`, `.port_text`, pure C, no GTE): the segment
from the look-at point to the eye is walked down the level model (`csGetRoot()` +0x11C, the tree `hdpPassRun` walks
first) as `hdpWalk` walks a probe: translations (kind 4), rotations (5, in `hdpRotVecSplit`'s split precision), level
changes (12), the current frame of an animation (3, the step's frame: `ANIM_IN_STEP` keeps it the same on every
machine) and of a select (9); a distance switch (2, the 16-bit IR distance as the GTE takes it) and a zone (11) choose
by the look-at point, i.e. the bands and regions the car itself collides with; a bounding sphere (1) is entered when it
can touch the segment (the segment's own sphere, conservative). Every box gives the segment's span inside it
(Cyrus-Beck, in 1/4096ths). A box holding the look-at point is skipped (a carved space the camera target stands in);
the mark volumes' spans are cut out of the solids' (`hdpWalk`'s order is not kept: a mark frees every solid of the
level model, not only those walked after it). The eye moves to the first point of a solid span no mark covers, minus
`TM2_VIS_CAMERA_MARGIN` (48 position units, 6 world units), at least `TM2_VIS_CAMERA_NEAR` (64) from the look-at
point; nothing moves when the segment crosses no solid. Because `cam_pos` is eased from where it was, the next step
starts from the clamped eye, so the camera slides along the wall instead of popping.

**State.** It reads the level model and `renderWorldShift` (via `renderGetWorldShift`, the walker's start level),
calls nothing else, writes only the eye (the human record's `cam_pos`, hash no) and its stack (3.7 KB: 96 pending
nodes, 32 solid and 32 mark spans; deeper pushes are dropped, which can only miss a wall; at most 16,384 nodes a walk). No hdp state, scratchpad,
contact record, `hdpTraverse`'s stack, `animControl` or `rand()`. `check_rollback.py` check 10 (b) covers it as part
of `viewChaseCamera` (VIEW_PRESENT: no `rand()`, stores only in "hash no" rows, through pointers too); check 8 / 9
find no static store in it. (A first version wrote the eye in a loop, `eye[i]`; check 10 (b) and 11 failed on it:
the pointer census cannot place an indexed store through the argument inside the camera block's "hash no" row, so it
counted a store into the hashed human record. The eye is now written at constant offsets, +0x14..+0x1F of the record:
the census itself is the negative control of the store side.)

**Check:** `tools/check_custom.py` check 9 (with `TM2_PORT_VISUAL`): the functions, their callers and callees, no
static store, and `portVisualCameraWall` run in the MIPS interpreter (FarMips) on a made-up level model (a translated
wall behind a mark-volume door, a rotated wall, a wall under a level change, a distance switch, a sphere away from the
segment over a wall that crosses it, a switched-off box, a box holding the look-at point: 8 cases, the eye within 2
units of a float model of the rule, unmoved with the bit clear, only the eye and the stack written) and on Moscow's
model loaded from the disc at its own address (four eyes outside the outer fence come back inside it, two with no
wall between stay). Negative control: a link without `TM2_PORT_VISUAL` (no function; e.g. `work/pv/r63_rb_P4`).

**For the port:** set `portVisual |= 0x20` (Vanilla Plus visual option; per machine, any value online). Not
runtime-tested. Limits: a wall that is not a hit volume (decoration the cars drive through) does not stop the camera;
a volume inside a sub-object (csListHead objects: doors, platforms) is not tested, only the level model.

**Port's build** (`work/pv/r63_port_P4`, read the addresses from its `symbols.tsv`): `portVisualCameraWall`
808E4794 (helpers `portCameraWallRot` 808E41F0 .. `portCameraWallSqrt`
808E473C), `viewChaseCamera` 80103EF4 (changed), `portVisual` 8088C1F8.
Verification: full build `work/build-20261025-192227-415794` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK from `.LIB` `f557c1e3…`, default port variant `1becd00a…`: unchanged); links `work/pv/r63_{rb,port,allh16}_{P4,P2}` (port's `r63_port_P4` `dda33c12…`); `check_all.py` 27/27 (proofs, placement, check_rollback, check_custom 9, check_balance G22, check_humans 13). Not runtime-tested.

## 11. Request 64: Amazonia's underground at Farthest (analysis; no code)

**Asked:** at Farthest, in Amazonia's underground tunnel the upper level shows through (the sky, the arena above,
flickering); from above, the tunnel shows through the ground; at the lava a piece of the underground ramp is drawn over
the lava. Do the region switches (opcode 11) hide the levels, and do `portFarLod` / the far pass break them; or is it
the order table or the near pass; a fix; and does retail at Original show it.

**The data** (`ISLANDS.DMD`, CONFIRMED; position units, z up; scratchpad tools `mw.py`, `zones.py`, `sim.py`):
- The tunnel runs along y at x -200..200, floor z -600, roof z -200 (the ground is at about 0..200): three sections
  (y -2,800..-1,200, -1,200..1,200, 1,200..2,800) and two ramps up to the surface at y +-(2,800..4,400), each an LOD
  node of its own: tunnel `8007B6A8`, `8007C970`, `8007D698` = `[0,300)` detailed (80-120 polygons), `[300,1200)`
  coarse (8); ramps `8007E700`, `8007F748` = `[0,300)` (72), `[300,600)` (7), `[600,1000)` (4 big quads whose side
  walls run from z -600 underground to z 200). Retail draws none of it past 1,200 (tunnel) / 1,000 (ramps) world
  units. The tube's polygons are one-sided and face inward.
- **One zone (opcode 11, `800692BC`, a "show" zone, reached twice: one per ramp end)**: its four regions are the
  tunnel and the two ramps (x +-400, z -600..200); inside, it draws `80069020`, 8 polygons, the ramp end walls at
  y +-3,600 / +-4,400 (z -200..200); outside, nothing. It sits in the `[0,400)` record of LOD `80069308`, whose other
  record `[400,500)` draws `800676F0` (70 polygons: a hut over the ramp's exit, z 200..498, and the same 8 end walls).
  Nothing in Amazonia hides the upper level from inside the tunnel or the tunnel from above: retail relies on the
  bands' short ranges, the back-face cull (the ground faces up, the tube inward) and the painter's sort.

**(a) The zones are not broken** (CONFIRMED in the code). `portFarLod` and `portFarLodNearScale` patch only LOD
records (opcode 2); the walker's zone test and its children are untouched, the range pass (`portFarDistanceScale`)
works on queued meshes only, after the walk, and never adds a node the walk did not queue. What the Farthest settings
change for these nodes (the port sets `portFarLod` 1 and `portFarLodNearScale` 4 at Farthest, `port_config.c` /
`fixes.c`):
- every tunnel and ramp LOD's farthest band is extended (section 4's rule: finite, drawing, nothing after it), so
  the coarse tunnel (8 polygons a section) and the coarse ramp (the 4 quads with the underground walls) are drawn
  from anywhere in the arena, not only within 1,200 / 1,000;
- the hut LOD `80069308`'s `[400,500)` record is extended too: past 400 x 4 = 1,600 the hut and its 8 end walls,
  which go down to z -200 at the ramp's ends, are drawn from everywhere (retail: only from 400 to 500);
- with the near scale every band is 4 times as far: the detailed tunnel is drawn to 1,200, the coarse one from there.

**(b) The order table and the near pass.** A painter's-order model of the walker's selection (bands, extension,
scale, zones, selects, the far cull; the emitter's max-SZ >> 3 entry, compressed; the back-face cull) and a coarse
raster (80 x 60 samples) over 40 camera placements (in the tunnel, on the ramps, above at 800..4,000 and at
1,500..16,000 units) compared the order table's winner with the nearest surface per sample (`sim.py`, `sweep.py`):
- inside the tunnel and on the ramps every sample is the tunnel's own polygon in retail and at Farthest alike: the
  model shows no upper level through the tunnel. The sky and the arena above showing there needs polygons that are
  not drawn, which the model does not have: a full mesh queue (2,048 at the port's cap) or packet buffer, or the near
  pass's 200-entry list (polygons nearer than 70 world units: in the tunnel most of its own walls). GUESS: of these
  only the queue and the buffer grow with Farthest (about 1,900 level meshes queued before the frustum against 440 in
  retail at the same place); the port's render counter (`portRenderStats`, `RENDER_ANSWERS.md`) at that place would
  show whether a cap is reached;
- from above, the tunnel over the ground: 87 samples of 81,000 at 800..4,000 units with retail's bands, 0 with
  Farthest's; at 1,500..16,000 units 6 (retail), 24 with the extension alone, 5 with Farthest. So retail shows the
  coarse tunnel through the ground too (within its 1,200 band), and the extension adds the same faults at every
  distance (the coarse pieces' big polygons sort by their farthest corner, behind the ground's);
- the ramp over the lava: matches the extended ramp and hut records (the 4-quad ramp and the hut's end walls reach
  below the ground at the ramp's ends), drawn from distances retail never draws them.
The model is coarse (no frustum, no near pass, no OT bias or override, every sample a ray) and was not run against
the user's frames; its use here is the comparison between the settings, not a picture.

**Retail at Original:** the same tunnel and ramp pieces are drawn only within 1,200 / 1,000 world units (the hut only
at 400..500), the far distance is 4,800; inside 1,200 the same sort faults exist (LIKELY: the model's 87 samples; the
retail band shows the coarse tunnel from above up to 1,200). So "the tunnel through the ground from above" is retail
near the tunnel and new farther away; the hut / ramp pieces "over the lava" from far away are new (the extension);
"the upper level in the tunnel" is not explained by the data or the sort (see (b)).

**(c) No fix built** (decision for the user). Options, each under `portFarLod` (0 = retail):
1. **Do not extend the underground pieces' bands:** leave out of the extension every LOD record whose meshes all lie
   below the ground at their place (on Amazonia the five tunnel / ramp nodes and the hut's `[400,500)`); the test needs
   a ground height per place (the hit volumes', as section 10 walks them) at scan time, or a reviewed list of nodes
   per level file. Without the extension the tunnel is drawn as in retail (to 1,200 x the near scale).
2. **Do not scale them either** (`portFarLodNearScale` leaves those records at retail's bands).
3. Measure first: the port's frame captures at the three places, with `portRenderStats` (caps) and with
   `TM2PC_NO_FAR_LOD=1` / `TM2PC_NEAR_SCALE=1` (the port's test switches) tell which setting brings each artefact.
   The request offers captures; the in-tunnel one decides between a cap and a sort fault.

**Follow-up: `HIDDEN_AREAS.md`.** The user's goal is the tunnel (and Paris's church) seen from anywhere like any
other part of the level, without showing through walls or the ground. That file finds a third cause for the sky in the
tunnel (the near pass off with `wobble = none` while the GPU size limit stays: big near polygons of the tube dropped),
surveys every hidden area on the disc, explains why drawing them always breaks the painter's sort, and recommends a
depth buffer in the port plus a decomp list and bit that draws them; option 1 above is a stopgap at most. Its section 7
replaces option 3's capture list.

## 12. Request 90: Holland's tall grass kept at retail's distance (implemented)

At Farthest, Holland's grass field drew its tall-grass beams on whole 100 x 100 blocks out to about 600 units from the
eye, in block-shaped patches (RENDER_ANSWERS.md R6.1). The beams' LOD `8002F50C` has one band, `[0, 140)`. Section 8's
scale put it at `[0, 560)`, and section 4's switch then extended it to unbounded because it is its node's top record,
so the beams filled every cell drawn. The cells only exist within the block LOD's near band (`[0, 150)`, scaled to
`[0, 600)`), whose far band is the block's one flat quad. So the grass ended at the block switch, a tile edge.

- **The rule** (`portFarLodScan`): a beam-only LOD is left out of both lists, the near scale and the extension, and
  keeps its retail band. Beam-only means every mesh the walker can reach below it holds only camera-facing beams
  (primitive kind 1), as `portFarLodBeams` tests. It applies only when the nearest LOD above it reached it through a
  record that has a coarser version: another drawing record of that LOD starting at or beyond the record's max
  (`portFarLodCoarser`; `portFarLodWalk` carries the flag down and each LOD sets it again).
- **What it changes on the disc:** only `8002F50C` (DEN; its 384 cell instances share it). `tools/dmd_lod_dump.py`
  `kept_retail()` gives the same answer for every DMD. The other 230 beam-only LODs (BURB 47, PARIS / PARIS2 42 each,
  ISLANDS 33, GLACIER2 22, GLACIER 18, SWAMP 18, HKONG 2, SROOF 1, DEN's other 2) are either under no LOD or under a
  single-band one, which has no coarser version. They keep Farthest as before.
- **Effect:** Holland's tall grass ends about 140 from the eye in 25-unit cell steps, as in retail, at every draw
  distance. The ground, the blocks and the patches keep the scale and the extension.
- **Checked:** `check_custom.py` check 14 runs `portFarLodScanLevel` in LinkMips on DEN, BURB and PARIS from the disc.
  The listed records must equal `dmd_lod_dump.py`'s. With scale 4 and `portFarLod` set, `portFarLodBegin` must leave
  the beams at `[0, 140)` while the block above them goes to `[0, 600)` / `[600, ..)`, and `portFarLodEnd` must give
  back the file. Negative control: request 89's link (`r89_port_P4`) fails, with the beams unbounded.

## 13. Request 91 item 1: Holland's grass field at its finest level at every distance (implemented)

The user, after request 90: the point of Farthest was the tall grass showing everywhere it is drawn, not appearing as
the car comes near. So with `portFarLod` set the decomp now keeps Holland's whole grass field at its finest level:
every 25-unit cell with its 8 beams, as near the car, at every distance.

- **The field** (DEN, `RENDER_ANSWERS.md` R6.1):
  - It is 6 patches, each 4 blocks, each 16 cells: 384 cells.
  - The patch LOD `8002F7C4` has `[300, 10000)` for the patch quad and `[0, 300)` for its blocks. The block LOD
    `8002F710` has `[150, ..)` for the block quad and `[0, 150)` for its cells. The beam LOD `8002F50C` has
    `[0, 140)` for 8 beams.
  - The whole field at its finest is 384 ground quads and **3,072 beams**. That is about what Farthest drew before
    request 90 (the cells out to 600, the beams on all of them; the field is about 600 across). The cost is small.
- **The rule** (`portFarLodScan`). Request 90's beam LODs are those reached through a record with a coarser version;
  on the disc only `8002F50C`. Two kinds of record are flagged:
  - **fine:** that LOD's records, and on the walk's path to it every LOD record with a coarser version (the
    patch's `[0, 300)`, the block's `[0, 150)`). Bit 1 of the record pointer in `portFarLodNear`.
  - **off:** those records' coarser versions (the patch quad, the block quad). Bit 1 in `portFarLodTable`, listed
    there when not already.
- **`portFarLodBegin`:**
  - With `portFarLod` set, a fine record is `[0, ..)` and an off record gets max 0, so it draws at no distance.
  - With `portFarLod` 0, every flagged record is scaled like any band (`portFarLodNearScale`). At Far the beams then
    reach 140 x the scale, in cell steps inside the cells' own band, so no block rectangle shows.
  - `portFarLodEnd` puts back every value; the flags stay for the next view. Hit detection keeps retail's records.
- **On the disc:** `tools/dmd_lod_dump.py` `fine_field()` flags 3 fine records (`8002F500`, `8002F704`, `8002F7B8`)
  and 2 off records (`8002F2D4`, `8002F334`) in DEN, and none in any other DMD. No record is both.
- **Not the decomp's:** a far beam's look. The beams are camera-facing quads of world size, so they shrink with
  distance like any polygon. The emitter clamps a far beam to at least 1-2 pixels, and the beam texture's
  transparent texels are 0x0000 (black). If far grass looks darker than near grass, LIKELY the port's filtering or
  edge blending takes those black texels in (request 90's note). The data's texture, colour 0x7F unlit and height
  are the same at every distance.
- **Checked:** `check_custom.py` check 14 runs the real code in LinkMips on DEN, BURB and PARIS.
  - The scan's lists and flags equal `dmd_lod_dump.py`'s.
  - On DEN with scale 4: `portFarLod` 0 scales the three LODs; `portFarLod` 1 gives the beams `[0, ..)`, the cells
    and blocks `[0, ..)`, and the block and patch quads max 0.
  - `portFarLodEnd` gives back the file both times.
  - Negative control: request 90's link (`r90_port_P4`) fails (no flags, beams `[0, 140)`).
  - Not tested in game.

## 14. Request 98 (2026-11-02 playtest at Farthest, Holland): the fence that changes, the dark wedges (budget built)

### 14.1 The fence (CONFIRMED in DEN)

- **Layout:** a rectangle of 200 segments (25 world units each) and 4 corners, x -1,569..6,600, y -6,569..5,600.
- **A four-level LOD chain, with a different texture page / CLUT at each level** (bands in world units, retail):

| LOD (instances) | fine record (draws) | coarse record (draws) |
|---|---|---|
| `8003A424` (24) | `8003A418` [0, 500): 2 x `8003A3A8` | `80039EA8` [500, 800): 1 quad, page 8F |
| `8003A3A8` (50) | `8003A39C` [0, 300): `8003A364` | `80039F08` [300, 1000): 1 quad, page 8F |
| `8003A364` (50) | `8003A358` [0, 200): 4 x `8003A2B0` | `80039F90` [200, 10000): 2 quads, page 90 |
| `8003A2B0` (200) | `8003A2A4` [0, 100): 18 polygons, the 3D rails, page 8A | `80039FEC` [100, 10000): 1 two-sided quad, page 8A |
| corners `80039E04` (4) | `80039DF8` [0, 100): 21 polygons | `80039AD8` [100, 800): 1 quad |

- **Why it still swaps at Farthest:** the scale (`portFarLodNearScale` 4) only moves the swaps out to 400, 800,
  1,200 and 2,000 world units from each node's point. The distance is to each segment, so the swap line travels with
  the car: the 3D rails about 16 segments away.
- **The finest level everywhere:**
  - The whole fence at its finest is 204 meshes, 3,684 polygons and 144 KB of packets. In view: up to 113 meshes,
    2,040 polygons and 80 KB. Now: up to 54 meshes and 16.5 KB.
  - On 40 sampled eye points, DEN at Farthest peaks at 1,665 meshes and 358 KB. With the fine fence: 1,781 meshes
    and 489 KB. 26 of 320 views pass the 390,320-byte buffer (upper bounds: every polygon counted, no back-face
    culling).
  - Finest everything, per level: fits every view on DISH, PARIS, PARIS2, ROOF, ROOF2, GLACIER2, SWAMP, BURB and
    SROOF. Passes the bytes in a few views on FREEWAY (6), GLACIER (7) and HKONG (16). DEN: 87 views over 2,048
    meshes. ISLANDS: impossible (up to 2.5 MB).
  - So a fixed rule can't choose; the packet buffer binds. A per-view budget can.

### 14.2 Built: `portFarLodBudget` (`.port_bss`, every variant; acts with `TM2_PORT_FAR_LOD` and `TM2_PORT_FAR_DISTANCE`)

- **Off / on:** 0 / 1 = off (retail, or `portFarLodNearScale` as set). 2..16 = the budget's top.
- **What it does:** `renderScene` calls `portFarLodBudgetView(view)` before `portFarLodBegin`. It picks the view's
  band scale (`portFarLodNearFactor` uses it until the next view) from the view's previous frame in
  `portRenderStats` (rows: views 0..3, the mirror).
  - **Down** one step at once when that frame used more than 7/8 of the mesh queue or of the packet buffer (or
    reached either). Then it holds `TM2_FAR_LOD_BUDGET_DOWN` (240) views before going up again.
  - **Up** one step when both were under half and the hold has passed: `TM2_FAR_LOD_BUDGET_UP` (8) views after a
    step up.
  - **Steps:** 2, 3, 4, 6, 8, 12, 16, between `portFarLodNearScale` (the floor) and `portFarLodBudget`.
  - **At 16** the fine bands reach 16 times their distance: Holland's 3D rails out to 1,600 world units, the whole
    arena.
- **State:** `portFarLodBudgetState` (0x10 bytes: the walked view's scale, per row its scale and hold). Drawing only:
  `rollback_regions.tsv` snapshot and hash no for both words.
- **The port's side:** at Farthest, set `portFarLodBudget` 16 with `portFarLodNearScale` 4 as now. Each view then
  draws its finest records as far out as its own last frame allows.
- **Checked:** `check_custom.py` check 18. A LinkMips run on made-up stats: room 4 -> 6, the hold, 8; 7/8 of the
  meshes and of the packet buffer step down; the hold after a step down; the floor; the top; the mirror's own row;
  the budget 0. `portFarLodBegin` scales a listed record by the view's budget scale, and End puts it back. Not tested
  in game.

### 14.3 The dark beams in the tall grass (answered; nothing built)

- **The beams with an end behind the eye** (mesh `8002F394`, 8 beams, ends at z 0 and 56, LOD `8002F50C` record
  `8002F500`, all 384 cells):
  - Measured over 4,000 chase views of the field: about 0.67 a frame, as the port measured.
  - All of them land just below the screen's bottom edge. About 0.22 a frame keep their GTE corners (no precise
    ones) and reach at most the bottom 15 pixels.
  - So they don't make long wedges across the field.
- **LIKELY the coplanar base (`RENDER_ANSWERS.md` R5.3):**
  - A flat base, colour (48, 69, 9), sorted at the fixed entry 4092, lies under the field: x 3,184..6,416,
    y -1,816..3,216.
  - Its meshes: `80038620` (one 3,232 x 5,032 quad, LOD `80038990`), `8003866C` (LOD `800388A4`), and `800386B8` /
    `80038704` (LOD `800387F0`).
  - The 384 grass ground quads (`8002F340`, entry 4090) lie in the same plane. Under a depth test the base's big
    triangles win in long halves that swing as the camera turns, and the base is a little darker: the wedges.
- **Test it on the port's side:** hide the meshes sorted at entry 4092 (or order coplanar layers by their entry):
  the wedges should go. Dropping the beams with an end nearer than 115 should change nothing.
- **Not built:** giving the base's records max 0 would leave about 27 world units of the field's south edge
  (y -1,816..-1,600) without ground. A depth order of the port's own (the layers' OT entries) is the clean fix.

## 15. Request 96 (2026-11-02 playtest, Hong Kong's subway): the camera beside a wall (`portVisual` bit 5, built)

### 15.1 Why

- **What bit 5 tests (section 10):** only the look-at-to-eye segment, with the eye kept 48 position units before the
  first solid along it.
- **Driving along a wall:** the segment meets the wall at a shallow angle θ, so the eye stays only 48 sin θ off it,
  close to 0, or isn't moved at all when the segment doesn't cross the wall.
- **Why that breaks the view:** at 1 player's world shift 3, the GTE's projection saturates nearer than H / 2 = 114
  of its units, about 127 position units. The near view's corners poke past the one-sided wall, and its nearby
  polygons project wrongly.
- **The data is clean** (checked in HKONG.DMD):
  - The subway is 800 wide and 400 high. Its walls are separate solid volumes, flat at the straight parts and chords
    at the curve, and neighbouring slabs share their end planes exactly.
  - The hit faces lie on the render walls, or 3.5-16 units in front of them at the curve. No mark is carved
    through a solid there.
- **What the decomp can't fix:** a wall polygon that runs alongside and behind the camera has a corner behind the eye
  whatever the margin. The port's renderer must clip those in 3D (`RENDER_ANSWERS.md` R5.2).

### 15.2 Built: the clearance `TM2_VIS_CAMERA_CLEAR` 128 (position units, 16 world units)

- **Solids grown:** `portCameraWallClip` takes a clearance `c` (every plane value + `c`). Each solid is grown by it,
  so an eye outside a grown box is at least `c` from the box in every direction. The bounding-sphere test widens by
  `c`.
- **The car within the clearance of a wall** (the grown box holds the look-at point): that box is tested as it is,
  and the eye is kept off it along the segment, as before.
- **Marks shrunk:** each mark is shrunk by `c`, so in a carved passage the eye keeps that far from its sides.
- **Units:** `c` is carried through the level changes (opcode 12) like the points.
- **Kept as before:** the 48 margin along the segment and the 64 floor.
- **Work area:** `portCameraWallWork` grows 0xE00 -> 0x1000 (`TM2_PORT_CAMERA_WORK`: the pending nodes carry `c`).
- **Checked:** `check_custom.py` check 9.
  - The float model grows and shrinks the boxes too; the made-up cases match within 1 unit.
  - Moscow: the 4 eyes outside the fence come back in.
  - The first "kept" case, (0, 0, 700) to (-300, 200, 760), was 16 units from the side and 40 over the top of the
    box at x -476..-316, z 520..720. It is pulled in now, so it was replaced by (800, 800, 700) to (1100, 600, 760),
  - Not tested in game.
- **Replaced by section 16** (request 100): growing the solids pulled the eye onto the car beside every bank and wall.

## 16. Request 100 (2026-11-03, the port testing request 96): the clearance without the pull (`portVisual` bit 5, built)

### 16.1 Why

- **Request 96's grown solids act along the segment.** Driving along a bank or wall farther than 128 from the car,
  the eye (112 over the car) is inside the grown solid while the look-at point is not. The segment enters the grown
  solid near its start, so the eye is pulled to the 64 floor.
- **Measured by the port** (LA, 2 players, player 2's eye-to-look-at distance): request 96 median 65, request 63
  median 112, bit 5 off median 290.

### 16.2 Built: probes from the eye

- **The segment is request 63's again,** byte for byte: real solids, real marks, a solid holding the look-at point
  skipped, the 48 margin and the 64 floor. `portCameraWallClip` loses its `c`. The walk is now
  `portCameraWallWalk(node, a, b, probe)` and the pull `portCameraWallPull`.
- **Then the clearance, from the pulled eye:**
  - Six probes, each `2 * TM2_VIS_CAMERA_CLEAR` (256) long, ±x, ±y, ±z, against the real solids less the marks.
  - On each axis, the free space back (`h0`) and forward (`h1`) set the move:
    - **Narrower than 256** (`h0 + h1 < 256`, a tunnel): the eye goes to the middle.
    - **Otherwise:** the eye moves off whichever side is nearer than 128, out to 128 from it.
  - A wall moves the eye sideways and a floor moves it up.
  - A move is never more than 128 on an axis and stays inside the free space the probes found, so the eye is never
    pulled toward the car by more than the clearance.
- **Probe rules:**
  - A solid that holds the eye counts from 0. A mark that holds the eye cuts it, so the eye in a carved passage
    keeps off the passage's sides.
  - The solids that hold the look-at point (skipped by the segment) are skipped by the probes too. The segment's
    walk lists them in the work (`hold`, up to 8).
  - The probes choose distance switches and zones by the eye.
- **Work area:** `portCameraWallWork` stays 0x1000. The pending nodes lost `c` (0xE24 used with the holds).
- **Cost:** 7 walks per view instead of 1. The probes are short, so the bounding spheres cut most of the tree.
- **The port's overlay** (`cur.c = 0`) can go: this build's segment is request 63's.
- **Checked:** `check_custom.py` check 9.
  - The float model does the same probes. The made-up cases match within 2 units, and no eye is more than 128 per
    axis from request 63's.
  - Two new cases. A tunnel 150 wide: the eye 20 from one wall goes to the middle. A wall alone, 200 from the car
    and 80 from the eye: the eye moves 48 sideways to 128 from the wall, its distance to the car kept (request 96
    pulled it most of the way in).
  - Moscow: the 4 eyes outside the fence come back in; the 2 clear eyes stay put.
  - Not tested in game. To test, as the request says: eyes driving along LA's canal banks and Hong Kong's subway.
