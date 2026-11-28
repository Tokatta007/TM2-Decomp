# Hidden areas: interiors and underground parts seen from anywhere

The user's goal (after request 64, `FAR_LOD.md` 11): Paris's church and Amazonia's underground tunnel should be "just
another part of the level": seen whenever the camera looks at them through their openings, without showing through
walls, roofs or the ground. Today the church's inside is drawn only once the eye is practically past the doorway, and
the tunnel only near it. This file says what hides them (disc data), lists every such place on the disc, explains why
drawing them always breaks under the PS1 renderer, and designs the fix. **Analysis only, except section 10** (a port
bug found in the user's tunnel video, fixed under `portVisual` bit 2) **and section 11** (request 72: Paris's church
drawn from anywhere, built under `portVisual` bit 6 and `TM2_PORT_HIDDEN_AREAS`).

Sources: the level files `DMD\*.DMD` on the USA disc, walked with the scratchpad DMD walkers of requests 61/63 (`mw.py`,
`zones.py`, `sim.py`, built on `tools/dmd_lod_dump.py`), the walker's opcode 2 / 11 handlers (`FAR_LOD.md` 1, 6), the
emitter's OT mapping (`DRAW_DISTANCE.md` 7) and the port's source (read only). Coordinates are **position units** (world
units x 8, z up) unless marked "world". Tags: **CONFIRMED** = read in the data or code; **LIKELY**; **GUESS**.

## Summary

- **Paris's church is hidden by two nested region switches (opcode 11), not by an LOD band** (CONFIRMED). Zone
  `8002E5E4` (region: 228 world units in front of the door, as wide as the church, up to the doorway's height) and,
  inside it, zone `8002E58C` (the same box from the door plane on). Outside the first box only the outer walls are
  drawn; in it a 27-polygon stand-in of the inside is added; the real inside (159 polygons: the nave, the teleporter,
  the pickup spots) appears only when the eye crosses the door plane, and the outer walls go. The church has **no roof
  and no floor outside the inside child**: from above its walls (z 1,472) it is an empty box.
- **Amazonia's tunnel is hidden by short LOD bands** (CONFIRMED, `FAR_LOD.md` 11): tunnel `[0,300)/[300,1200)`, ramps
  `[0,300)/[300,600)/[600,1000)`; its one zone (`800692BC`) only adds the ramp end walls while the eye is inside.
- **Update 2026-10-25, corrected (section 10): "the sky and the upper level in the tunnel" is a port bug of
  `portVisual` bit 2 (the own car walked first), CONFIRMED in the code and matched to the user's video.** The level's
  top-level meshes then share the translation key of the car's last part, and the emitter, which recomputes the
  translation only when the key changes, draws the tunnel and ramps at the car's translation: as if seen from a few
  hundred units behind and above the level's origin (the tunnel's middle), an open trough from above. The mirror and
  retail cannot show it. Fixed in `renderScene` (one more object number after the own car). **Section 9.2's reading
  (the eye above the roof, view 4) was wrong:** the user drove in the default view, and the camera did not move
  (section 10.1). The hole theory of the next bullet stays unsupported.
- **(Superseded, see above) "The sky and the upper level in the tunnel" is LIKELY a hole problem, not a sort or a cap problem:** the detailed
  tunnel meshes are flagged for the near pass (mesh `+26 & 4`), which in retail cuts polygons nearer than 70 units so
  they stay under the GPU's size limit. With the port's default `wobble = none` the near pass is switched off
  (`vplus/precise.c`: threshold 0) while the size limit is kept (`runtime/gpu.c` `too_big`): big near polygons of the
  tube are **dropped**, and the sky (always drawn first) and front-facing upper-level polygons show through the holes,
  flickering as the camera moves. The caps (2,048 meshes, 390,320 bytes) are LIKELY not reached there (the counts
  below; `portRenderStats` will tell).
- **Survey:** 212 zone nodes on the disc; besides the church and the tunnel, the enclosed places hidden this way are
  New York's rooftop interior and the rooms inside its real block versions, Roof Tops' upper hall, Hong Kong's portal
  cards, and some roofed pieces with short bands (Holland, Amazonia, Antarctica, Swamp). Most other zones are not
  interiors (two versions of a part, cells, walls) and must not be touched (section 3).
- **Why "draw them always" glitches:** no depth buffer, one OT slot per polygon from its **deepest vertex** (1 world
  unit a slot near, 4 / 8 / 32 farther), so a large outer polygon whose far corner is deeper than an inner polygon is
  drawn first and the inner one paints over it. Measured on the data: the church's nave painted over its own side wall
  (slots 653 vs 671), the tunnel over the ground above it (782 vs 789) (section 4).
- **Recommendation: (a) a depth buffer in the port + a decomp "hidden areas" bit** (section 5): the port already has a
  depth per vertex for about 83% of the world's polygons (the precise path); with a depth test and the size limit
  lifted for polygons that have depth, the decomp's bit draws the listed interiors at every distance (zones forced to
  their inside / both children, bands unbounded) and the depth test does the hiding. Portals (b) work on the PS1
  renderer but only roughly; stopping the band extension (c) hides the problem and does not reach the goal.
- **Built for the church (request 72, section 11):** `portVisual` bit 6 with `--port-define TM2_PORT_HIDDEN_AREAS`
  (needs `TM2_PORT_VISUAL`). For each view's walks the church's zone draws its outer walls and its real inside
  together, the walls keep the facade with the doorway at every distance (past 900 world units retail swaps in a
  facade with no doorway), and the door is drawn at every distance (retail drops it past 900 / 1,000). PARIS and
  PARIS2; every word is put back after the walks. The ceiling is not made two-sided (the user: try without it first).
- **Captures asked from the port** (section 7): the three Amazonia spots and the church doorway, each with
  `TM2PC_NO_FAR_LOD`, `TM2PC_NEAR_SCALE=1` and `wobble = ps1` toggled one at a time, plus `portRenderStats` and the
  GPU's dropped-polygon count. The results and the next asks: section 9.

## 1. Paris: the church (CONFIRMED in `PARIS.DMD`, root 0)

**Where:** at the end opposite the Eiffel Tower (the tower stands at (0, -6,144), `ARENA_SWITCHES.md` 12). The building
spans x -768..768, y 7,296..8,924, z 192..1,472 (world: 192 wide, 204 long, the floor at z 24, the walls 184 high). The
front (the facade) is the plane y = 7,296, facing -y, toward the tower.

**The doorway:** a hole in the facade at x -256..256, z 192..832 (world 64 wide, 80 high). The door is a level-model
select node (opcode 9) `8002EEF0` in the doorway, outside the zones: child 0 the intact door (LOD `8002E740`: `[0,275)`
4 polygons, `[275,900)` 2), child 1 the broken door (LOD `8002EEB0`: `[0,275)` 11, `[275,1000)` 7). Past 900 / 1,000
world units the door is not drawn at all.

**The switches** (opcode 11, `renderWalkModel`: the eye in the node's frame against the regions; inside draws child +12,
outside child +8):

| node | region (position units) | outside (+8) draws | inside (+12) draws |
|---|---|---|---|
| `8002E5E4` (zone A) | `8002E5A0`, 6 planes: x -1,006..1,012, **y 5,472..9,205**, z 192..832 | the outer walls: LOD `8002C074` = `[0,275)` 52, `[275,900)` 16, `[900,16626)` 3 polygons (the facade and the two side walls up to z 1,472; no roof, no back wall) | zone B |
| `8002E58C` (zone B) | `8002E548`, 6 planes: the same box from **y 7,296** (the door plane) to 9,205 | the outer walls (`8002C074` again) + `8002C0CC`, 27 polygons: a plain box of the inside (floor z 192, ceiling z 920, walls), the stand-in | the inside, 14 meshes, 159 polygons: the nave `8002C524` (106), the teleporter pad LOD `8002DD64` (`[400,4000)` 12 polygons, `[0,400)` 4 small parts), 4 one-polygon windows / doors (`8002DE28`..`8002DF64`), two select nodes `8002E2EC` / `8002E4E0` with a `[0,700)` LOD each (LIKELY the weapon pickup spots); **no outer walls** |

**What the player sees** (the regions are tests of the eye, the camera, not the car):
- eye more than 228 world units in front of the door (y < 5,472), or off to the side (|x| > 126 world), or above the
  doorway's top (z > 832: 80 world units over the floor): the outer walls only. Through the open doorway the eye sees
  **nothing of the inside**: the far walls are back faces (one-sided, facing out), there is no floor; whatever was drawn
  before shows (the sky / ground behind);
- eye in front of the door within 228 units, low and centred: the 27-polygon stand-in (bare walls and floor, no
  teleporter, no pickups);
- eye past the door plane: the real inside, the outer walls dropped (they would be back faces from inside anyway).

This is what the user describes ("can't see inside until you're practically past the doorway"): the chase camera
trails the car, so the eye crosses y 7,296 (world 912) only once the car is well inside.

**Not a band:** the outer walls reach 16,626 (always), the inside has no band of its own (its pieces have `[0,400)` /
`[0,700)` details). The door's bands do not matter for the inside.

**The missing roof** (CONFIRMED): no polygon of the level model lies over x -768..768, y 7,488..8,832 above z 920 except
the walls' tops; the inside's ceiling (z 920) faces down. In retail, from a high eye (the tower's deck, z 1,344..3,001)
the church is an open box with nothing inside. Drawn always, the inside would be seen through the open top (correctly
by depth: nothing covers it). Section 5 offers a fix (the ceiling drawn two-sided).

## 2. Amazonia: the tunnel (CONFIRMED in `ISLANDS.DMD`; details in `FAR_LOD.md` 11)

| node | what | records (world units) | polygons |
|---|---|---|---|
| `8007B6A8`, `8007C970`, `8007D698` | the three tunnel sections (x -200..200, y -2,800..2,800, floor z -600, roof z -200) | `[0,300)` detailed (meshes `8007AB30`, `8007B858`, `8007CB20`, near-pass flag `+26 & 4` set), `[300,1200)` coarse (`8007A9B8`, `8007B6E0`, `8007C9A8`, flag clear) | 80 / 120 / 80; 8 each coarse |
| `8007E700`, `8007F748` | the two ramps to the surface (y +-2,800..4,400) | `[0,300)` 72 (flag 4), `[300,600)` 7, `[600,1000)` 4 big quads (z -600..200) | |
| zone `800692BC` (show) | regions = the tunnel and both ramps (x +-400, z -600..200), reached twice | outside: nothing; inside: `80069020`, the ramp end walls (8 polygons, y +-3,600 / +-4,400) | 8 |
| LOD `80069308` | holds the zone in `[0,400)`; `[400,500)` = `800676F0`, a hut over the ramp's exit and the same 8 end walls | | 70 |

So the tunnel is hidden by distance: past 1,200 (tunnel) / 1,000 (ramps) world units retail draws none of it. From the
surface the ground (facing up) hides it; from inside, the ground's underside is a back face. Nothing else hides one
level from the other.

**"The sky / the upper level inside the tunnel"** (mechanism CONFIRMED in code and data; **the match with the user's
frames is not supported by the port's stills**: no holes at A1 / A2 in any run, and the user's video shows the chase
eye above the roof instead; section 9):
- the detailed tube is near-pass material: its meshes have flag 4 (3,880 of the 10,119 mesh visits of a walk of ISLANDS over every branch have it; the coarse tunnel and ramp meshes do not). In
  retail a polygon with flag 4 nearer than OT slot 70 (20 in split screen) is deferred to `renderNearPolys`, which cuts
  it at screen midpoints (`RENDER_ANSWERS.md`, `DRAW_DISTANCE.md` 7). In a tube 50 world units wide every wall is near;
- the port, with `wobble = none` (its default), sets that threshold to 0 while the emitter runs (`src/vplus/precise.c`,
  `vp_ModelEmit`), so nothing is cut: every polygon goes whole to the OT;
- the port's GPU keeps the PS1's size limit for every polygon, precise ones included (`src/runtime/gpu.c` `polygon`:
  a triangle with two vertices more than 1,023 pixels apart across or 511 down is skipped, `gpu_culled_prims++`), and
  the GTE clamps SX / SY to -1,024..1,023: a wall quad beside the eye, or one reaching behind it, is over the limit and
  is **not drawn**;
- behind the hole: the sky dome (OT slot 4,095, drawn first) and any upper-level polygon facing the eye (walls, hut
  sides, double-sided plants; the ground faces up and is culled). As the camera moves, polygons cross the limit: the
  flicker.
- **Not the caps:** at Farthest about 1,860 level meshes are selected before the frustum anywhere in the arena (the
  painter model, `sim.py`); after the frustum the queue stays under `portMeshListCap` 2,048, and the selected 4,100-5,100
  polygons (before the frustum) would fill at most about two thirds of the 390,320-byte buffer. `portRenderStats` at the spot settles it (section 7).
- **Not Farthest itself:** near the eye the detailed band is chosen at any draw distance, so `TM2PC_NO_FAR_LOD` and
  `TM2PC_NEAR_SCALE=1` should not change it; `wobble = ps1` (the near pass back) should.

## 3. Survey: every zone and every short-band covered piece on the disc

**Zones** (opcode 11; 212 nodes, `FAR_LOD.md` 6). "Polygons" are of the whole child, every record. Kinds: *show*
(outside draws nothing), *hide* (inside draws nothing), *swap* (both draw), *empty* (neither draws polygons).

| level | zones | what they are | hidden area? |
|---|---|---|---|
| FREEWAY (Los Angeles) | 22 empty | children hold hit volumes only (1 flagged off) | no |
| GLACIER / GLACIER2 (Antarctica) | 12 / 9 empty | flagged off in the file (`node+4 & 2`) | no |
| PARIS / PARIS2 | 12 empty, 7 swaps | church `8002E5E4` / `8002E58C` (section 1); Eiffel deck `80034A54` / `8003816C` (eye on the deck: the tower's 12 meshes, 269 / 204 polygons, replaced by the deck alone, 114 / 106); `8003C074` / `8003DC40` (flag 1; eye above z 744 / 640: 4 -> 16 and 6 -> 8 meshes of raised parts, a detail swap by height); `8006EF1C` (flag 1; eye at z 640..2,400 over the arena: 325 vs 379 meshes, two versions of the streets) | **church: yes**; the rest no |
| ISLANDS (Amazonia) | 1 show | ramp end walls (section 2) | **yes** (with the tunnel's bands) |
| ROOF / ROOF2 (New York) | 33 / 32 hide, 3 show, 13 / 10 swaps | building blocks as **a one-mesh shell outside / the real block inside its region** (`80030DF8` 45 vs 139 polygons, `8003B130` 104 vs 595, `8004FA88` 94 vs 1,807, `80069508` 71 vs 2,183, `800765BC` 43 vs 1,003, `8006F7C0`, `80071FF8`, `800754F0`, `80076484`, ...); facades / walls hidden while the eye is in a box beside them (the 33 hides, 6-184 polygons each); and **a rooftop interior** at x 4,000..9,080, y -4,000..1,080, z 6,200..7,400: swap `8004FA10` (outside 452 polygons, inside 1,355), shows `80047998` (387), `8004F94C` (859), `8004FC8C` (4), and hide `8007C014` (the street plane, 128 polygons, gone while inside) | **the rooftop interior: yes** (GUESS: rooms entered from the roofs; openings not checked); **the shells: the real blocks hold interiors too** (below), so `in` (the real block from anywhere, the shell dropped) is a candidate, an expensive one |
| HKONG (Hong Kong) | 3 show, 9 swaps | two whole-arena swaps (`80070864`: the subway, eye at z -160..400, 447 vs 62 meshes; `80065664`: the temple room,
the eye in it walks the inside only, 327 vs 136: section 14); four buildings as a 12-polygon box outside / 291 polygons inside their region (`80038B94`, `80038CE0`, `80049F64`, `8004A000`); **portal cards** `80063FB4`, `80064524`, `80064B40`: outside, a 4-polygon picture across a passage mouth (x or y = +-3,481), inside, the 53-58 polygons beyond; the centre's 73-polygon structure shown only from the three passages (`80048CE0`, `80048D68`, `80048DF0`) | **the passages behind the cards: yes** (retail already uses portal impostors); the arena swaps: no |
| SWAMP | 5 show, 1 hide | cells: big parts of the arena (55-216 meshes, 198-2,265 polygons, e.g. `8005D854`, `8003B2EC`) drawn only while the eye is in the columns of space that can see them; `8002AD54` hides 16 polygons (the ground's edge skirt, section 18) | not interiors (visibility cells, CONFIRMED by request 129); all drawn with bit 6 (section 18) |
| BURB (Cyburbia) | 1 swap | `8008B580`: the suburb (in its region) or the ring highway around it (out), two areas that share no surface (request 119 corrected this: not two versions; section 17) | yes, request 119 |
| SROOF (Roof Tops) | 7 show, 1 hide, 9 swaps | **an upper hall** at x 5,120..12,400, y 3,600..6,800, z 7,400..8,000 (the second region a corridor): swap `8007ED00` (outside 4,863 polygons, inside 782) + hide `8007EC3C` (120 polygons gone while inside) + 8 swaps of 12 / 10 vs 58 polygons at z 8,000..9,200 (its roof parts, GUESS); walls drawn only from one side (`8002DE34`, `8002E314`, `800392B4`, `800396F8`, 18-21 polygons; `800536C4`, `80054A10`, `8005609C` on the arena's edges, 93-109) | **the hall: yes** (GUESS); the walls: safe to draw always |
| DISH (Moscow), DEN (Holland) | 0 | | |

**Short-band pieces under other geometry** (a drawing LOD record ending before 4,800 world units whose meshes lie
under a horizontal polygon of another part of the level model within 200 world units above, at 7 of 9 sample points;
scratchpad `ha/covered.py`; the list then reviewed by hand):

| level | node, records (world) | where | what (GUESS unless marked) |
|---|---|---|---|
| ISLANDS | `8007B6A8`, `8007C970`, `8007D698` `[0,300)/[300,1200)` | the tunnel | the tunnel (CONFIRMED, section 2); the ramps are open above and so not in this list |
| ISLANDS | `8005AC98` `[0,500)` 134, `[500,800)` 18; `800746DC` `[0,350)` 6 meshes | y 13,400..14,600, z 200..600 | a roofed structure at the arena's north end and small pieces inside it |
| DEN | `80054538` `[0,200)` 62, `[200,600)` 25; `80056954` 113 / 50; `80047B00` `[0,200)` 36 | x 3,458..4,539, y 3,461..4,369 (and y -4,539..-3,641), z 0..430 | roofed interiors; LIKELY the barns' insides (the barns hold teleporters, `ARENA_SWITCHES.md` 14); not checked against the barn objects |
| GLACIER | `80055744` `[0,500)` 19 | x 13,900..16,295, z 3,960 | a floor under a roof |
| SWAMP | `8003A680` `[0,75)` 196, `[75,150)` 52, `[150,600)` 16 | x -4,215..-3,588, z -56..56 | a roofed shelter (inside the show cells `8003B2EC` / `80042568`) |
| PARIS | `8002DD64`, `8002E2B8` and parts | inside the church | already behind the church's zones (section 1) |
| PARIS | `8003E848` `[400,4000)` 12 + 4 parts `[0,400)` | x +-280, y -6,424..-5,863 | the same shape as the church's teleporter pad, under the Eiffel Tower's deck: covered, not enclosed (no) |
| ROOF (New York) | inside block `80069508`'s real version: 16 walls `[0,200)` of 16-20 polygons (`80056E50`..`80058C2C`, `80059050`..`8005AA9C`), `80062FD4` `[0,500)`; a teleporter-pad shape `8006AB58` `[400,4000)` + 4 parts | x -6,400..-4,000, y -3,600..-800, two storeys z 6,400..7,200 and 7,200..8,000; the pad at (-4,900, -1,900, 6,400) | the rooms of a building, with a teleporter (GUESS from the pad's shape) |
| ROOF | inside the rooftop interior: `8004B51C` (`[0,220)` 362, `[220,1000)` 41), `8004F888`, `80043DC0` (86 / 23), `80045F68` (202 / 42), `80047918` (152 / 31), `8004161C`, `80042C90` | x 4,000..8,600, y -3,600..800, z 6,360..7,160 | the interior's rooms (the zones of the zone table) |
| ROOF | `8002F804`, `8002EF44` (`[0,500)` 96 meshes, 816 / 528 polygons), `8002F508`, `8007F47C` (`[0,900)` 229) | x 0..3,200, y 0..5,600, z 7,040..7,520 | under the roofs of block `80030DF8` (its real version) |
| SROOF (Roof Tops) | five `[0,120)` pieces of 24 polygons (`80077CF4`..`80078C04`), their `[120,1000)` 8-polygon bands (`8007C02C`..`8007C66C`), `8007776C`, `80077924` (`[0,400)` 10) | x 5,600..9,600, y 5,950..6,250, z 7,400..7,700 | the upper hall's corridor (inside zone `8007ED00`) |

Left out on review (covered but open to the side, not interiors): Los Angeles's ground under the freeway (`80072F04`),
Hong Kong's streets under the raised parts, Moscow's four 2-polygon pieces, Paris's arcade strips. Cyburbia has none.
(The level model is the file's largest root: root 0, or root 1 for New York, Cyburbia and Roof Tops.)

## 4. Why drawing them always glitches on the PS1 renderer

**The rule** (CONFIRMED, `DRAW_DISTANCE.md` 7, `RENDER_ANSWERS.md` R1): there is no depth buffer. Each polygon goes into
one OT slot, `z = max(SZ of its vertices) >> shift` (its **deepest** vertex), plus a per-polygon bias from the mesh
data, compressed: 1 world unit a slot below 1,024, 4 to 4,096, 8 to 16,384, 32 beyond. The OT is drawn from the deepest
slot to the nearest; inside one slot, in link order. Some meshes take a fixed slot (the sky 4,095, backdrops
4,090..4,095). Back faces are culled per polygon (NCLIP) unless the polygon is double-sided.

**What makes the levels look right anyway:** occluded parts are left out by the data: bands that end before the part
can be seen wrongly, zones that switch the inside off while the eye is outside, one-sided polygons whose backs vanish,
and small polygons where a deep corner cannot reach far behind another part. Drawing a hidden area always removes the
first two of these, so its polygons compete with the outer ones in the OT.

**Example 1, the church drawn always** (painter model `sim.py`, retail bands, the inside added from outside; eye at
(-2,400, 3,400, 2,400), yaw 25, pitch -20, 500 world units in front and high): at one sample the nearest surface is the
church's own side wall (`8002B674`, at 597 world units there), but that wall quad runs the building's length: its
deepest corner is at 672, slot **671**. The nave polygon behind it (`8002C524`, 611..654) gets slot **653**, is drawn
later and paints over the wall: the inside shows through the outer wall. From behind and above, 21 of 826 samples of
the inside are wrong this way; from the side, level with it, none. From any eye above the walls' top the inside is
seen over them (the missing roof, section 1).

**Example 2, the tunnel under the ground** (retail bands; eye at (5,994, 4,565, 1,135), yaw -138, pitch -10, about
1,000 world units away): the coarse tunnel section `8007C9A8` (8 big polygons) spans 701..783 world units at one
sample, slot **782**; the ground polygon above it (`8002DBCC`) is the nearest surface there, at 651, but its deepest
corner is at 790, slot **789**. The ground is drawn first, the tunnel's roof over it: the tunnel shows through the
ground. In a 24-camera sweep at 600-1,100 world units (eyes 100-300 world units up, looking at the tunnel) this appears in 10 views (1-26 samples each); `FAR_LOD.md` 11
counts 87 of 81,000 samples at 800..4,000 with retail's bands. At Farthest the same fault reaches farther and the
coarser slots make ties: ramp `8007E83C` (deepest 1,360, slot 1,107) over the ground `80048904` (deepest 1,372, slot
1,111, the ground nearer at 1,275).

**Why no OT trick fixes it in general:** the slot is per polygon from one vertex. A big outer polygon (the ground, a
wall) and a small inner one (the tube, the nave) overlap on screen with the inner one behind; whenever the outer one's
deepest corner is deeper than the inner one's, the order is wrong. Sorting the inside "behind" the outside (a slot
offset) works only from directions where the inside is truly behind every outer polygon on screen; from above the
church or across the tunnel it is not. The coarse far slots (4 / 8 / 32 units) make it worse at a distance, and a
depth sort inside one slot does not exist (link order). With hundreds of polygons of the inside visible at once
(church 159, tunnel up to 424, New York's interior 2,600), a few wrong pixels per frame at changing places are the
flicker the user reports from above the tunnel.

## 5. The fix

### (a) Depth-buffered drawing in the port + a "hidden areas" bit in the decomp (recommended)

**Idea:** let a depth test do the occlusion, and make the decomp draw the hidden areas whenever the walker's frustum
test keeps them. Then the church's inside is visible exactly where a line of sight through the doorway exists, the
tunnel where the ramp mouths are in view, and nowhere through walls or ground.

**Port side** (`NATIVE_RENDERER.md`; can be done on today's MIPS renderer, before the native stages):
1. **The depth is already there.** With `wobble = none` the precise path gives each vertex of about 83% of a 1-player
   battle's polygons its float position and depth (the GTE shadow table, `gte_precise_lookup`; PSY-Z
   `psyz-12-precise-vertices.patch` draws `(x*w, y*w, 0, w)`). The C emitter's beams and sprites have a depth per
   corner in the precise ring (`PORT_PRECISE_PUT`'s format, `port_c_emitter.h`; `TM2_PORT_C_EMITTER`). Write a real
   depth (from w) instead of 0 and enable a depth test for the views' 3D order tables (`port_ot_is_world`).
2. **Rules per polygon:**
   - opaque, with depth: test and write;
   - semi-transparent: test, no write, still in OT order;
   - no precise depth (2D, refused projections): draw as today, in OT order, without the test (or with the depth
     of its OT slot, the inverse of the compression, as a constant);
   - the sky and backdrops (slots 4,090..4,095) and the far pass's background: no write;
   - coplanar overlays (decals, shadows: the mesh data's OT bias, the shadows' override pair, arcs' OT depth): a
     tolerance, i.e. pass when `new <= stored + e` with e of about 1-2 world units scaled to the depth, so a later
     polygon of the same plane wins as in the painter's order. The bias can be recovered by the port alone: expected
     slot `compress(max depth >> shift)` against the slot the polygon was linked at (`DRAW_DISTANCE.md` 7);
3. **Lift the size limit for polygons that have depth** (they are clipped by the GPU at w = 0 and drawn as they are):
   this also fixes the tunnel's holes of section 2 without the near pass. Keep the limit for integer-only polygons.
4. **The range pass** (`portFarDistanceScale`, `FAR_DISTANCE.md` 2): a mesh scaled by 2^k is projected from scaled
   vertices, so its recorded depth is the true depth / 2^k (the projection itself is exact). The depth test needs the
   true depth: either the port multiplies by 2^k (k is in the mesh entry's key, bits 28..29 with bit 30, but the GPU
   stage sees only packets), or the decomp provides it (below).

**Decomp side** (proposed here; **built for Paris's church by request 72, section 11**, in code rather than a table;
under a port define such as `TM2_PORT_HIDDEN_AREAS`, `portVisual` bit 6
`TM2_VIS_HIDDEN_AREAS`, 0 = retail, drawing only):
1. **The list:** `config/US/hidden_areas.tsv`, made by a tool (`tools/hidden_areas.py`, the scratchpad survey made
   permanent) and reviewed: level file, node, kind, policy, polygons, description. Policies:
   - `in`: a zone draws its inside child from anywhere (the zone's +8 pointer replaced by +12 for the view's walk);
   - `both`: a zone draws both children (+8 pointed at a port group node holding both);
   - `open`: an LOD record's band becomes `[min, unbounded)` (its first record, so the detailed piece is the one
     drawn; no coarse/detailed seams);
   - `twosided`: listed polygons drawn double-sided (ctl bit 31) for the view (the church's ceiling as its roof).

   The first entries: Paris `8002E5E4` both, `8002E58C` in, ceiling polygons of `8002C524` (z 920) twosided; Amazonia
   `8007B6A8`, `8007C970`, `8007D698`, `8007E700`, `8007F748` open (424 polygons in all), `800692BC` in (the end walls).
   Candidates for review: New York's rooftop interior (`8004FA10` in, `80047998`, `8004F94C`, `8004FC8C` in), Roof
   Tops' hall (`8007ED00` in and its 8 roof swaps, the corridor pieces open), New York's real blocks (`80069508`,
   `80030DF8`, `8003B130`, `8004FA88`, ... in: 600-2,200 polygons each, the cost to weigh), Hong Kong's cards
   (`80063FB4`, `80064524`, `80064B40` in, which also drops the cards), Holland / Amazonia / Antarctica / Swamp's roofed
   pieces (open). **Not** listed: swaps that are two versions of the same part (Cyburbia's arena, Paris `8006EF1C`, the
   Hong Kong arena swaps, the Eiffel deck): both would draw the same surfaces twice (z-fighting); where one version is
   the real one with an inside (New York's blocks) the policy is `in`, never `both`;
2. **The mechanism:** as `portFarLodBegin` / `portFarLodEnd` (`FAR_LOD.md` 4, 8): patch the listed nodes in memory for
   the length of each view's walk in `renderScene` and put them back before anything else reads them (the hit walker
   reads the same records and zones: collisions stay retail). Node addresses come from the level file's load address,
   like the far-LOD scan's (`assetInstallImage`); levels only, no car files;
3. **Depth for the port** where the GPU stage cannot get it: for range-pass meshes (k > 0), the C emitter
   (`TM2_PORT_C_EMITTER`) can put the true depth of each scaled mesh's polygons into the precise ring (the ring's depth
   field exists), or `portFarDistanceEmit` can record each scaled mesh's packet range and k in a small port table;
4. **Checks** like `check_custom.py`'s: only `renderScene` reads the bit, the patches are undone by the end of the walk
   (the MIPS interpreter on a made-up tree and on Paris / Amazonia from the disc), no `rand()`, no game-state store.

**Interaction with the rest:**
- The bit requires the depth buffer: with `wobble = ps1` / `smooth` (no depth per vertex) the port leaves it off, and
  the near pass stays as retail's.
- `portFarLod` / `portFarLodNearScale`: independent (LOD records only); an `open` record is not extended again.
- Capacity: the listed pieces add at most a few hundred polygons a view (church 159, tunnel 424; New York's interior
  2,600 if accepted), well inside 2,048 meshes and 390,320 bytes.
- The precise path (`PORT_PRECISE_PUT`, the shadow table) stays as it is; it only gains a consumer.
- A native renderer (`NATIVE_RENDERER.md` stage 3) would carry the depth per vertex directly from the emitter (no shadow
  table, the true depth for scaled meshes from the start); the decomp bit is the same.

### (b) Portals on the PS1 renderer (possible, limited)

Treat the doorway and the ramp mouths as portals:
- per view, project the opening's corners (the church: x +-256, z 192..832 at y 7,296; the tunnel: the two ramp
  mouths at the surface); if any is in front of the eye and on screen, and the eye is on the outer side, draw the
  inside;
- clip the inside to the opening's screen rectangle (a drawing-area packet before the inside's polygons and one
  restoring the view's area after them, both linked into the OT around them);
- link the inside's polygons into slots behind the opening's frame (deeper than the deepest slot of the facade
  polygons around the doorway), so the frame paints over them.

Limits: the clip is a screen rectangle, not the opening's shape (the facade around it must cover the rest, and it does
only where it is nearer in the OT); a slot offset breaks wherever the inside is not behind every outer polygon on
screen (looking down into the roofless church, or along the ground over the tunnel); a portal seen through another
portal is not handled; cars inside are objects drawn at their own slots; each opening is hand-placed per level; the
tunnel's inside-out case (the upper level seen from the tunnel through the mouth) needs the reverse portal. Hong Kong's
portal cards show that retail used the cheap form of this. It would give "visible through the door from in front",
not "like any other part of the level".

### (c) Not extending the underground bands (the band-aid of `FAR_LOD.md` 11 options 1-2)

It puts Amazonia back to retail: the tunnel invisible past 1,200 world units, retail's own sort faults within 1,200
(section 4, example 2), the hut and end walls back to their 400..500 band. It does nothing for the church (zones, not
bands), nothing for the holes inside the tunnel (section 2), and it reaches the goal backwards: the hidden area stays
hidden instead of being seen. It is a reasonable stopgap for Farthest only until (a) exists.

**Recommendation:** (a). Order: the port's depth test and size-limit rule for precise polygons (which also answers the
in-tunnel holes), then the decomp's list and bit with Paris and Amazonia, then the reviewed candidates.

## 6. Notes

- The zone's region is tested against the eye in the node's frame; the walker never tests the car. A chase camera
  behind the car is what keeps the church's inside off until the car is through the door.
- The region boxes are 6-plane boxes (`hdpBox`'s format); Swamp's are columns without a height limit (the survey's z
  ranges there are not meaningful).
- `RENDER_ANSWERS.md` R2.2's count "No zone has a null child" is about pointers: 22 show zones have an outside child
  that draws nothing (an empty group), as listed here.

## 7. For the port: captures asked

Four places, each captured at Farthest (the user's setting) as a frame plus numbers, then again with **one** switch
changed at a time:

| spot | where (eye positions in position units, yaw in 1/4096 turns, 0 = +y; `TM2PC_PHOTO_AT=x,y,z,yaw` places the photo camera there in the car record's units (+0x6FC), assumed to be these: check on the Eiffel Tower at (0, -6,144)) |
|---|---|
| A. in the tunnel | eye at (0, 0, -450), looking along +y (yaw 0); also near a wall (x 150) |
| B. above it | eye at about (0, -9,000, 3,200) (yaw 0, looking down at the middle section) and at the user's place |
| C. the ramp over the lava | the user's place (the hut / ramp pieces seen over the lava) |
| D. Paris's church from outside, door destroyed | eye at (0, 4,000, 400), yaw 0 (outside zone A); (0, 6,000, 400) (in zone A: the stand-in); (0, 7,600, 400) (past the door plane: the inside); and the same as played, driving in |

Runs per spot:
1. as played (Farthest, `wobble = none`);
2. `TM2PC_NO_FAR_LOD=1` (`src/vplus/port_config.c`: `portFarLod` 0);
3. `TM2PC_NEAR_SCALE=1` (`src/vplus/fixes.c`: `portFarLodNearScale` 1, retail bands);
4. `wobble = ps1` (the near pass back: `vplus/precise.c` keeps the threshold);
5. optionally `TM2PC_RETAIL_CAPS=1` (retail's 280 meshes / 97,520 bytes) to see what the raised caps already fix.

With each: `TM2PC_RENDER_STATS=1` (the `portRenderStats` row of the view: meshes / cap, packet bytes / cap, caps
bits); the GPU's dropped polygons for the frame (`gpu_culled_prims` in `runtime/gpu.c`, today counted and never printed:
a temporary print of its change per frame is enough); the eye's position (the view's eye, `viewEyePositions`, or the
camera of the human record +0x14) so the zone state can be computed from the regions above.

What each result proves:
- **A:** holes that go away with run 4 and a non-zero dropped count in run 1 = the size limit with the near pass off
  (section 2): fixed by (a) step 3 or by keeping the near pass. If they stay in run 4 and caps bit 0x4 / 0x8 is set,
  a cap; if neither, a sort fault (send the frame and the eye for the model).
- **B:** the tunnel over the ground in run 1 but not in run 2 = the extension (`FAR_LOD.md` 11 (b)); still there in run
  2 near the tunnel = retail's own fault (example 2 of section 4). Either way the fix is (a).
- **C:** gone in run 2 = the extended ramp / hut records (`FAR_LOD.md` 11).
- **D:** the three positions should show nothing / the stand-in / the inside exactly at the regions' borders: that
  confirms section 1 at run time (no draw-distance switch should change it).

## 8. Files

- This file; links from `FAR_LOD.md` 11 and `NATIVE_RENDERER.md` 6.
- Scratchpad tools (not in the repository): `ha/church.py`, `ha/survey.py`, `ha/zsum.py`, `ha/covered.py`,
  `ha/church_sim.py`, `ha/example2.py`, `ha/flag4.py` (on `r63/mw.py`, `zones.py`, `sim.py`); for section 9
  `ha/tun_hv.py` (the tunnel's hit volumes as planes, on `r63/hv.py`), `ha/above.py` (render polygons over the
  tunnel), `ha/camclamp.py` (a float model of `portVisualCameraWall` on Amazonia's volumes), `ha/hut.py` (the ramp,
  hut and north-temple LOD records); the video frames in the scratchpad's `vid/a`, `vid/b`.
- `RETAIL_BUGS.md` 39 (the chase camera leaving the level) names the tunnel case (view 4, from the code: not seen;
  the video is section 10's bug).
- Section 11: `ha4/church2.py` (the church's zones, LOD records and door in PARIS / PARIS2), `ha4/doorway.py` (which
  meshes cover the doorway), `ha4/cop.py` (coplanar pairs and OT bias fields of the walls, inside, stand-in and door).
- Section 10: `ha3/lods.py` (every LOD node's point and bands), `ha3/paths.py` (the tunnel meshes' paths),
  `ha3/rr.py` (a z-buffer render of a level from an eye, on `r63/sim.py`), `ha3/toplevel.py` (top-level-key meshes per
  level), `ha3/queue.py` (the level walk's queue order and keys for an eye), `ha3/dis_rs.py` (`renderScene`'s calls and stores); frames `vid/a3` (full resolution, 1,920 x 1,080, and a
  4 fps sheet).

## 9. The port's captures (2026-10-25): what they show

Sources: the port's `build/captures/hidden_areas/RESULTS.md`, `results_rest.txt` and the 30 pictures there (five runs
at each of A1, A2, B, D1, D2, D3: 1 as played, 2 `TM2PC_NO_FAR_LOD`, 3 `TM2PC_NEAR_SCALE=1`, 4 `wobble = ps1`,
5 `TM2PC_RETAIL_CAPS`), and two playtest videos, a (15.5 s, in the tunnel) and b (9.8 s, the road over the lava), read frame by frame.

### 9.1 What the stills show and rule out

| spot | result | what it proves |
|---|---|---|
| A1, A2 (in the tunnel, the centre and 50 units from the right wall) | the tunnel whole in all five runs; 16 polygons over the size limit dropped in runs 1-4, 0 in run 5 | **the size-limit / near-pass theory (section 2) is not supported at these places**: the 16 dropped are dropped with the near pass on too (run 4), so they are not the tube's near walls (no hole in any picture); they belong to meshes that retail's 280-mesh cap leaves out (run 5: 0), i.e. queued late (LIKELY the upper level's polygons, drawn before the tube and painted over by it). Not the caps either (the tube is whole with the 280 cap reached) |
| B (asked z 3,200, placed at (0, -9,000, 2,600), yaw 0, pitch 300) | the arena from above, no tunnel through the ground in runs 1-4; **run 5 shows it**: with the 280-mesh cap reached most of the ground (lava, plaza) is not drawn, the clear colour (brown) fills the frame and the tunnel's floor, end walls and its health pickup show as a grey strip from the image's middle up to the ring | B **was over the tunnel's span**: not above it (the eye is 6,200 units south of the tunnel's end) but looking along it: the centre ray (pitch 300 = 26° down, if positive is down) meets the ground at about y -3,770 (the south ramp), the tunnel (y -2,800..2,800) lies 2..14 degrees above the centre, where run 5 draws it. So from this eye (325 world units up, along the axis) the painter's order is right at every draw distance, which does not test section 4's faults (they were found at 600-1,100 units from eyes 100-300 up, looking across). Run 5 shows something else: **the level walk queues the tunnel before the ground**, so whenever a mesh cap is reached the ground goes first and the tunnel shows through it |
| D1 (y 4,000), D2 (y 6,000) | the facade with the door **intact** (destroyed was asked) | nothing about the inside: the intact door closes the doorway. D1 outside zone A, D2 in it (the stand-in would be behind the door) |
| D3 (y 7,600, past the door plane) | the real inside: the hall, the two statues, the painting | **zone B confirmed at run time**: the inside child is drawn once the eye is past y 7,296, the same in every run (no draw-distance switch changes it, as section 1 says). Runs 1-3 drop 21 polygons, run 4 15: there the near pass matters for 6 polygons (none visibly missing) |

- **The render stats are stale** in photo mode (the battle frame's before the pause: 447 meshes in Amazonia, 74 in
  Paris at every spot): they say nothing about the spots. The dropped counts are fresh (they differ by spot and run).
- **The caps:** run 5 is retail's 280 meshes; at 2,048 (the port's) the stale battle-frame count, 447, is far below.
  Not the user's cause unless live stats say otherwise (ask (v)).

### 9.2 The user's video a: the chase eye is above the tunnel's roof (measured) -- WRONG, see section 10

**Correction (2026-10-25, section 10):** the user drove in the **default view** (view 2), not view 4, and the eye did
not leave the tube: the car is the same size and place on screen in the glitched and the normal frames, and the
picture switches on and off within a quarter of a second. What the frames show is the tunnel **drawn at the wrong
place** (from about the level's origin), not seen from a wrong eye. The car-size step below also took the car as 260-290
units wide; against the tube in a normal frame it is about 30 (section 10.1), so the "1,200 behind, 600 above" and the
view-4 conclusion do not follow (and retail forces view 2 on covered ground, 10.2). The rest of this subsection (the hit-volume table, the view geometry from the code) is
data and code and stays; the request 63 option (9.3) is not the fix for the video.

Frames `vid/a/f_2.5.png` (near the ramp's start), `f_12.5.png` (in the tunnel); the rear-view mirror (top right, a
camera at the car) shows the tube correctly, walls and roof; the main view shows:
- the tube as an **open trough seen from above**: the floor and, near the eye, the inner faces of both walls with
  their top edges (f_2.5); no roof. The roof's polygons face down and are culled from above; the walls' inner faces
  face the axis, so they are front faces from an eye above the tube and between its walls (seen steeply, thin, in
  f_12.5);
- a **flat brown plane** up to the horizon: **not a mesh, the clear colour** (the same brown fills run 5 of spot B where
  the cap left the ground out). The ground over the tunnel (the lava, `8002B814` etc. at z 0, faces up) and the plaza
  tops (z 200) are back faces from an eye under them; nothing else is there, so the clear colour shows. The lava far
  away is a red line at the horizon (edge on: the eye is at about the lava's height);
- **the upper level from below**: the plaza slabs at z 200 seen from under them, and the big dark grey wall ahead
  (f_12.5) is LIKELY the far ramp's end wall / ramp walls (`80069020`, `8007E73C`: up to z 200, facing the tunnel,
  drawn while the eye is in zone `800692BC`'s regions, x +-400, z -600..200).

**The eye's height from f_2.5** (full-resolution frame; the walls' floor and top edges converge to one point; tube
400 wide and 400 high, render floor z -600, roof z -200): at the frame's bottom row the floor's edges give
(200 + x)/h = (200 - x)/h = 0.30, the top edges 200/(h - 400) = 0.65: **the eye is centred and about 650-700 above
the floor, z of about +50..+100**: 250-300 above the roof's polygons, about at the lava's height. With the 1-player
projection (vertical half angle 27.7 degrees, `FOUR_PLAYERS.md`), the car's rear is 29 degrees under the eye:
**about 1,200-1,300 units behind, about 600 above the car** (car about 260-290 wide, about 230 px of 1,920). Video b
has the same geometry (the car 31 degrees under the eye, the same size).

**Which view** (`viewChaseCamera` 80102CC4, `camera.c`; offsets from `viewCameraOffsetsInit` 800FF764, position
units, car space: back, up): view 2 (`cam_chase`) 280, 112; view 3 (`cam_offset3`) 480, 168; **views 1 / 4
(`cam_offset`) 1,200, 800** (eased at 90%). Only the last fits the frames: LIKELY the user plays in view 4 (the key
cycles 2, 3, 4; `carSetViewMode(h, 2)` starts in view 2). The port should log the human record's `view` (+0x10C) to
confirm.

**The numbers in the tunnel** (CONFIRMED in `ISLANDS.DMD`; `ha/tun_hv.py`, `ha/above.py`):

| | render polygons | hit volumes (`hv.py`) |
|---|---|---|
| floor | z -600 | `8007F8AC` z -758..-598 |
| walls | x +-200, facing in | `8007F85C` / `8007F94C` x -500..-180 / 180..500, z -758..-42 |
| roof | z -200, facing down | **`8007F8FC` x +-500 (bevelled), y +-2,801, z -202..-42**: the roof is a hit volume |
| ground above | lava / ground pieces at z 0, facing up (`8002B814`, `8002C1DC`, ... `[0,200)` records); plaza z 200 around the ramps | `8006D9D0` / `8006DA20` (x beyond +-200, z <= 0), plaza boxes z 0..200 |
| covered ramp, \|y\| 2,801..3,623 | roof parallel to the ramp, 395 above it | **`8007FA3C` / `8007FB7C`**: underside z = -202 + 0.487 (\|y\| - 2,801), meets the plaza (z 200) at \|y\| 3,623 |
| ramp | floor z = -597 + 0.498 (\|y\| - 2,800) (26.5 degrees), open from \|y\| 3,623 | floors `8007F99C` / `8007FADC`, side walls `8007F9EC`, `8007FA8C`, `8007FB2C`, `8007FBCC` x 180..500, up to z 200 |

So the tube is 360 wide (400 between the render walls) and 396 high between the volumes; the ground over it is 200 above
the roof's polygons. The chase eye's goal on level floor (the car's point about R above the floor, R a few dozen
units: GUESS 60): **view 2 z = -428, view 3 -372: inside, 230-170 under the roof; view 4 z = +260: 460 above the
roof's polygons, above the ground**. Moving: `viewChaseCamera` leads by the speed (`speed / 32` along the heading,
which the ease mostly cancels), eases at 40..70% kept (90% in view 4), turns its goal with all of the car's heading and
a third of its pitch, and only keeps the eye at or above the car's height (on the ground) and 40 units behind it in
car space. **It has no roof, wall or indoor test, and no lowered mode** (CONFIRMED: those are all its height rules).
So:
- **view 4: the eye is above the roof whenever the car is in the tunnel** (and over the covered ramps); during the
  ease (90% kept) it lags higher or lower along the car's path, around the ground's height, which is what the video
  measures (the clear colour under it means it was under the lava's plane in those frames);
- views 2 / 3: never above the roof on level floor or on the ramps (descending, view 2's eye is 16 + R above the ramp
  behind and 374 - R under the covered ramp's roof; climbing, 206 + R above and 182 - R under); **sideways out through
  a wall** when the car is turned against the tube: the goal is `280 sin(angle)` (view 3: 480) to the side, out of the
  render wall (200) at 46 degrees from the axis with the car centred (view 3: 25), at about 17 (10) with the car's
  centre 120 from the axis (GUESS for a car against the wall). From outside a wall the near wall is a back face and the
  ground faces up: the sky and the upper level's front faces show, flickering as the eye crosses the wall (a wall
  bounce, a spin, reversing).

**Retail:** the eye's position is retail's code and offsets, and at Original the detailed tube (`[0,300)` world units,
2,400 position units) and the coarse sections are drawn from 1,200 behind: **retail at Original shows the same picture
in view 4** (LIKELY; code and data CONFIRMED, not seen on a PS1). The extended bands and the near scale change only the
far pieces. Listed with `RETAIL_BUGS.md` 39.

### 9.3 Request 63's option covers it (no new code needed for Amazonia)

(Correction, section 10: the option covers a camera outside the tube, which video a is not; the video's cause is
bit 2's translation key, fixed in 10.5. What follows stays true of the camera.)

`portVisualCameraWall` (`port_variant_arrays.c`, `FAR_LOD.md` 10) tests every level-model hit box with `+7 != 1` that
is not a mark (`+5 == -2`), the rule the cars use (`engine_hit_detection.c` 1901). The tunnel's volumes have `+7 = 2`,
`+5 = 0..2`: **solids for the cars and for the camera**, the roof included. The port already ships the option
(`vplus.c`: `VP_CAMERA_WALLS`, "Camera inside the arena", `portVisual |= 0x20`). A float model of the clamp on
Amazonia's volumes (`ha/camclamp.py`; every select child, no LOD / zone choice; margin 48, at least 64 from the look-at
point; R 60):

| car | view | goal eye (x, y, z) | clamped eye | stopped by |
|---|---|---|---|---|
| tunnel centre, heading +y | 4 | (0, -1,200, 260) | (0, -444, -228): 28 under the roof's polygons | roof `8007F8FC` |
| the same | 2 / 3 | z -428 / -372 | unchanged | - |
| x 100, heading 40 degrees toward -x | 2 / 3 / 4 | (280, -214, -428) / (409, -368, -372) / (871, -919, 260) | (151, -61, -498) / (151, -60, -502) / (154, -64, -460) | wall `8007F94C` |
| ramp y 3,200, climbing | 4 | (0, 1,891, 268) | (0, 2,909, -189) | covered ramp's roof `8007FB7C` |
| ramp y 3,200, descending | 3 / 4 | (0, 3,649, -98) / (0, 4,263, 637) | (0, 3,537, -153) / (0, 3,544, 2) | `800673F0` (a box at the ramp's end, x +-210, \|y\| 3,580..4,420, z -600..890, under a select: which child is current in play was not checked) |

So with the option on, view 4 in the tunnel puts the eye about 450 behind and 330 above the car, under the roof, and
the sideways cases come back inside the walls. **Limits** (from `FAR_LOD.md` 10): only hit volumes of the level model
(decoration and `csList` objects are not walls); a box holding the look-at point is skipped (the look-at point is the
car's point + 40 ahead, inside the tube: not an issue here). The volumes' faces lie inside the polygons (walls 180
against 200, roof -202 against -200), so the clamped eye ends at least 48 inside the drawn tube.

**If a listed hidden area's roof were not a hit volume** (none found in Amazonia; New York's rooftop interior and Roof
Tops' hall have solids at their roof heights, 23 each near the regions, `hv.py ROOF / SROOF -v`, roles not reviewed):
an extension of the same bit, designed, not built:
- **data:** per listed area, its interior as one or a few convex boxes in `hdpBox`'s plane format (made by
  `tools/hidden_areas.py` from the area's render polygons: the tube's floor, wall and roof planes; reviewed), a
  `const` table in the port text (level file, node of the frame it is in, planes): read only, built at build time, no
  load-time store;
- **test:** at the end of `portVisualCameraWall`, for the current level's entries: if the look-at point is inside an
  interior box and the (already clamped) eye is not, clip the segment to the box (Cyrus-Beck, the existing
  `portCameraWallClip`) and put the eye at its exit minus the margin, at least `TM2_VIS_CAMERA_NEAR` from the look-at
  point, written at the constant offsets as today. The zone regions (`800692BC`'s, x +-400, z -600..200) are not used:
  they reach to z 200, above the roof;
- **rules:** the same bit 5 (no new option), the same reads (the level model, `renderGetWorldShift`, the table), the
  same single store (the human record's `cam_pos`, "hash no"); no `rand()`, no static store, no hdp state: check 10 (b)
  and check 8 / 9 unchanged, `check_custom.py` check 9 gains a case (a box interior with no roof volume: the eye
  comes back under the roof; a look-at point outside every box: unchanged).

### 9.4 "The tunnel through the ground from above", and Paris's church when driving in

- **Tunnel through the ground:** three causes, in likelihood: (1) the eye under the ground's plane (view 4 in the
  tunnel or the covered ramps, or views 2 / 3 out through a ramp's side wall below z 200): the ground and plaza tops
  are back faces from below, so the tube and the ramp show with the clear colour around them (video a is this);
  the option fixes it (the ramps' side walls and covered roofs are volumes, the model's rows above). **Correction: video
  a is not this but section 10's bug (the tunnel drawn at the car's translation)**; (2) retail's
  painter faults from a low eye on the surface (section 4, example 2; a chase eye is 112-800 above the car: grazing
  views, the worst case), not fixed by the option; (3) a mesh cap reached (spot B run 5: the tunnel is queued before
  the ground): not at 2,048 on the stale numbers, live stats needed. On the surface the eye is never under the
  ground on its own: `viewChaseCamera` keeps it at or above the car's height when the car is on the ground.
- **Paris's church:** the zones test the eye. View 2's eye trails 280 behind and 112 up, view 3's 480 / 168, so the
  real inside appears only when the car is that far (plus the ease's lag) past the door plane, 17-30% into the 1,628-
  deep hall; before, the eye in zone A sees the stand-in through the doorway. **View 4:** the eye is about 800 above
  the car (z about 1,000 with the floor at 192), above both regions' top (z 832): **the inside is never drawn in view
  4**, even with the car inside; the eye looks down into the roofless, floorless box of the outer walls with the car in
  it (LIKELY; not captured). The camera option does not change this (the doorway is open, no volume on the segment). The
  fix stays section 5 (a) (the inside drawn from anywhere), or, short of it, a hidden-areas policy that tests a listed
  zone against the look-at point as well as the eye (drawing only, `renderScene`'s walk; with `both` for zone B while
  only the look-at point is past the door, so the facade stays).

### 9.5 The user's video b: the grey block in the distance (spot C)

Frames `vid/b/f_7.5.png` (and `f_3.5`, the 1 fps sheet): on the road over the lava a grey, flat-topped, sloped-sided
block on a stone base stands near the horizon, right of the centre, left of the volcano; it is there in every frame.
- **Not the ramp pieces:** the coarse ramp records (`8007D6F4` / `8007E73C`, `[600,1000)`, 4 quads) reach only up to
  z 200, the plaza's top; they cannot stand above the ground.
- **Not LIKELY the hut over the ramp's exit** (`80069308`, centre (0, 0, 120): `[0,400)` the 100-polygon hut
  `80068174` + the zone, `[400,500)` `800676F0` (70, with the 8 end walls down to z -200), `[500,2000)` `8006745C`
  (16); x +-200, \|y\| 3,600..4,400, z 200..498): it is drawn in every record (so not a Farthest-only object), and at
  the block's angular width (about 0.21 radians, 213 px of 1,920) the 400 x 800 hut would be about 3,000 units away
  (detailed, as retail draws it); its height-to-width ratio (about 0.5) is twice the block's (about 0.21).
- **LIKELY the north-end temple** `8005AC98` (x +-600, y 13,400..14,600, z 200..600; `[0,500)` 134 polygons,
  `[500,800)` 18), the same shape as the far building at the top of spot B's pictures: about 7,000-7,500 units (900
  world) away at that width, ratio about 0.27. **Retail draws nothing of it past 800 world units**; at Farthest the
  near scale (x4: detailed to 2,000) and the extended last record draw it from everywhere. That is Farthest working as
  meant (a real building seen from far), unless the user means another object.
- **The camera's place (GUESS):** the volcano (a backdrop: a direction) is 13 degrees right of the view's centre, the
  block 1 degree right; spot B (yaw 0) puts the volcano 7 degrees left of +y, so the block's bearing is about 19
  degrees left of +y, and at about 7,250 units from the temple the eye is near **(+2,300, +7,100)**, looking north,
  slightly west. The port's camera position for this frame (or the user's place in photo mode) would settle it.

### 9.6 Next asks for the port (in this order)

(Superseded for video a by 10.6: rebuild with the fix first. Asks 3-5 stay useful.)

1. **The user retries the tunnel and the ramp with "Camera inside the arena" on, then off** (`VP_CAMERA_WALLS`,
   `portVisual` bit 5), in the view they play (and in view 2 too). The roof, walls, covered ramp roofs and ramp side
   walls are hit volumes (9.3), so with the option on the sky / flat brown / upper level from below and the tunnel
   through the ground should go; if they do, it is the camera (and retail's, view 4). If not, send the frame and the
   eye.
2. **A moving chase-camera capture through the tunnel** (`TM2PC_PAD1_SCRIPT`: drive from the south plaza down the
   ramp, through the tunnel, out of the north ramp; views 2 and 4; option off and on), logging per frame the human
   record's `view` (+0x10C), `cam_pos` (+0x14), `cam_look` (+0x38) and the car's position (+0x6FC), with a flag when the
   eye is outside the tube: \|x\| > 200 or z > -200 for \|y\| < 2,801; on the covered ramps z > -202 + 0.487
   (\|y\| - 2,801) for 2,801 < \|y\| < 3,623; \|x\| > 200 and z < 200 in the ramp's cut. Expected: view 4 flagged
   throughout with the option off, never with it on.
3. **D1 and D2 again with the door destroyed** (shoot it first, or a scripted drive), and once driving in at views 2
   and 4 (9.4: the stand-in until the eye passes the door plane; in view 4 never the inside).
4. **The user's place for spot C** (the photo camera at the place of video b's frame 7.5, or the car position there),
   to confirm the north temple (9.5).
5. **Render stats from live battle frames** (`TM2PC_RENDER_STATS=1` during the scripted drive of ask 2, not photo
   mode): meshes and bytes against 2,048 / 390,320 in the tunnel and over it, for cause (3) of 9.4.

The **no-wobble near pass** theory (section 2): not supported by the stills (A1 / A2 whole with the near pass off, the
16 dropped polygons unrelated to it). Still possible while moving (a chase eye can come closer to a wall than A2's 50
units, and it looks down 14-20 degrees more), but nothing in the video needs it: every symptom there follows from the
eye's position. Ask 2 with `wobble = ps1` as a third run would close it.

## 10. The tunnel video, the real cause: `portVisual` bit 2's shared translation key (CONFIRMED in code; fixed; also request 69)

Port request 69 (2026-10-25) answers section 9.6's asks 1-2: the port's scripted drive (player 1 from (0, -5,000,
260), yaw 0, Up held, `build/captures/tunnel_drive/v2_on.txt`, `v2_off.txt`, `v4_*.txt`) logged the human record each
step: **the eye is inside the tube every step**, option on or off (car z -584, eye 280 behind and 112 above, z -472,
\|x\| < 150); and the user, with "Camera inside the arena" on, sees the tunnel right for its first half and the glitch
again **from about the health pickup (y 0) on**. This section answers both.

Video a was driven in the **default view** (the port's single-player view, mode 2,
`cam_chase`). The mirror (a camera at the car) shows the tube whole the whole time; the main view shows only a floor
strip with low walls, the clear colour, the sky and the upper level's slabs. Two candidates were asked for, (A) the
camera and (B) the far-LOD bands; both are ruled out with numbers below, and the cause is a third one, (C).

### 10.1 What the frames show (`vid/a3`: full resolution, 1,920 x 1,080, and a 4 fps sheet over the 15.5 s)

- **The picture switches:** paused at 0-2 s glitched; driving 2.0-2.75 s glitched, 3.0-3.25 s whole, 3.5-3.75 s
  glitched, 4.0-14.0 s whole (the tube with its frieze, roof and far end), from 14.5 s glitched. The car moves
  steadily; nothing about its place changes in a quarter of a second.
- **The camera does not move:** the car is 243 px wide and at the same screen place in the whole frames (5.5 s) and
  the glitched ones (2.5 s, 12.5 s). In a whole frame the floor's edges at the car's row are about 3,350 px apart
  (the floor / frieze lines extrapolated from their vanishing point at about (935, 440)): **the car is about 1/14 of
  the tube's 400** (about 29 units; section 9.2 took 260-290, hence its far eye). Request 69's trace gives the eye
  itself: 128 above the floor, 280 behind the car, inside the tube.
- **The glitched picture is the tube drawn elsewhere:** at 12.5 s the strip's edges converge to (954, 207), about 380
  px above the far lava line, and its half width over its drop below that point is 0.15, which for a 400-wide floor
  needs an eye about 1,300 above it. The car beside it is the same 243 px as in the whole frames. Both cannot hold for one eye:
  **the tube is drawn from another place than the car.** The rest of the glitched frames is right for the real eye under
  the ground: the clear colour where the ground and plaza tops are back faces, the plaza slabs from below, the zone's
  end wall at y 3,600 about 440 away (930 px wide for 400 units), the sky.

### 10.2 (A) the camera: ruled out

Request 69's trace settles it: the eye is inside the tube at every step, option on or off. Section 9.2's view-4 reading
rested on the car-size step (a 260-290 wide car) and on the strip read as seen from the real eye. With the car about
29 wide the camera-to-car geometry is the same in the whole and the glitched frames (size and place to the pixel), and
an eased chase eye (40..98% kept per step) cannot jump about 1,000 units and back within
0.25 s, twice. Section 9.2's code reading already gave views 2 / 3 no way above the roof on the level floor or the
ramps (the goal 112 / 168 above the car's point; descending, 374 - R under the covered ramp's roof); the speed lead
(`speed / 32` along the heading, before the ease) moves the goal along the tube, not through the roof. A frame-by-frame
simulation of `viewChaseCamera` was not needed: nothing in the frames asks for a moved eye.

**Why view 4 never puts the eye above the roof there (request 69 update; CONFIRMED in code):** the port forced view 4
(+0x10C = 4): on the plaza the eye was 1,160 behind / 800 above, and down the ramp it came in to view 2's place (280 /
112) by step 490, with the camera option off too. That is retail's own rule in `carUpdateContacts` (800EF5D0,
`src/semantic/car_crash.c`): for a human car, **when the front-left wheel's ground kind has class 1**
(`worldSectionGroundClass[kind] == 1`, the class the covered ground carries; the tunnel's and the covered ramps'
kinds LIKELY, from the trace) **and the view is 3 or 4, it calls `carSetViewMode(car, 2)`**: the view itself becomes 2
(the far offsets are no longer used; the ease then brings the eye in). So section 9.2's "view 4 above the roof" cannot
happen in the tube in retail or the port; `RETAIL_BUGS.md` 39's tunnel paragraph is withdrawn with it. Pressing the
view key there switches to 3 / 4 for at most one step.

### 10.3 (B) the far-LOD bands: ruled out (CONFIRMED in the data)

The walker's opcode 2 measures **the eye to the LOD node's own point** (node +4..+12 plus the walker's translation,
`>> shift`, i.e. world units, squared with the GTE's SQR; `FAR_LOD.md` 1), not a sphere. The tunnel and ramp pieces
(`ha3/lods.py`):

| LOD node | point (position units) | bands (world units) | the eye in its own piece is at most |
|---|---|---|---|
| `8007B6A8`, `8007D698` (end sections) | (0, -+2,000, -400) | `[0,300)` 80 polygons, `[300,1,200)` 8 | about 1,000 from the point (125 world) |
| `8007C970` (middle) | (0, 0, -400) | `[0,300)` 120, `[300,1,200)` 8 | 1,232 (154 world) |
| `8007E700`, `8007F748` (ramps) | (0, -+3,600, -200) | `[0,300)` 72, `[300,600)` 7, `[600,1,000)` 4 | about 830 (104 world) on the covered part |
| `80069308` (the hut, the zone) | (0, -+4,000, 320) | `[0,400)` the zone, `[400,500)` hut + end walls (70), `[500,2,000)` hut (16) | |

- **Every band starts at 0** (min 0, and `0 x s^2 = 0`), so the near scale (x4 at Farthest: `[0,1,200)` /
  `[1,200,4,800)`) opens no gap at the eye. The detailed band is chosen in the tube at every setting (retail, near
  scale 4, far LOD), and each section's floor, walls and roof are **one mesh** (`8007AB30`: floor quads 200 x 200, walls
  in three rows, roof quads; `8007E9D0` the same for the ramp): no band choice can keep the floor and drop the walls.
- **Main view against mirror:** `portFarLodBegin` runs in every `renderScene`, the mirror's too: the same records.
  The mirror's eye (at the car) and the main eye (280 behind it) are both inside the zone's regions (x +-400,
  z -600..200): the zone chooses the same child.

### 10.4 (C) the cause: the own car, walked first, shares its translation key with the level (CONFIRMED in code)

- **The key.** Each queued mesh carries a translation key, draw entry +0x1C = the walker's `w.obj`
  (`reference/renderer_walker_engine_render_scene.c`): a model's walk starts it at the object counter (`w.obj =
  S->objects`, ctx +0x34), and every offset (4), transform (5) or scale (12) node takes a new number (`w.obj =
  ++S->objects`). `renderScene` steps the counter before each object's walk; **the level root's walk takes no step**:
  in retail it is the first walk (counter 0), and its top-level meshes share key 0 with the world models, which is right
  (one object, one translation).
- **The emitter's cache.** `sub_800CDC54` (`reference/model_emitter_800CDC54.c`, the `trans_id` test) recomputes the
  GTE translation (`TR = R_slot (x, y, z)`, split >> 15 / & 0x7FFF) **only when an entry's key differs from the
  previous entry's**. The rotation is reloaded by slot; the translation is not.
- **Bit 2** (request 45 item 3, `portVisualOwnCar`; set always by the port, `vplus.c`; never in the mirror, mode 4)
  walks the view's own car **before** the level: `*objects + 1`, then the car's model, whose transform nodes take new
  numbers. The level root's walk then starts at the counter as the car left it: **the key of the car part numbered
  last**. When that part's mesh is also the car's last queued entry (it depends on which of the car's parts pass the
  frustum and their LOD, so it changes from frame to frame) and the level's first queued meshes are top-level ones (no
  4 / 5 / 12 above them), those level meshes get no translation of their own: they are drawn with the level's rotation
  and **the car part's translation**, until an entry with another key comes.
- **What that draws:** the level's vertices are in the level's frame; with the car part's camera-space translation
  they appear as seen from `eye - car part` in level coordinates, about 280 behind and 100-130 above the level's origin
  for the default view heading +y (request 69's offsets; the car part's own origin moves it by a few dozen): **over the middle of Amazonia's tunnel, at about the lava's height**, looking along it.
  The tunnel and ramp LOD nodes are top-level children walked first (spot B run 5 in 9.1: "the level walk queues the
  tunnel before the ground"), so they make the run. A z-buffer render of the five detailed tunnel / ramp meshes from
  (0, -136, 34), pitch +3 (the car climbing), and from (0, -130, 50), pitch -6 (`ha3/rr.py`; `ha3/k1.png`, `k2.png`)
  gives the glitched frames' picture: an open trough from above (the roof faces down and is culled), its walls as thin
  rims, the far ramp rising to a vanishing point above the horizon, about 120 px wide at its far end at 1,920 px (the
  frame: 125-150). Everything walked after the run (the ground, the plaza, the zone's end walls, the sky, the other
  cars) is drawn right, from the real eye under the ground. It also explains why 9.2 measured the same "centred eye
  650-700 above the floor" in both videos: the level's origin does not move with the car, and the tube is centred on
  x = 0.
- **The switch at the health pickup (request 69).** What breaks the run is any entry with another key before the
  tunnel. A model of the level walk's queue (`ha3/queue.py`: the walker's stack order, children pushed in order and
  popped last first, the key counter of nodes 4 / 5 / 12, the bands at Farthest, the zones, a frustum cull of each
  mesh's and group's sphere; eye on the axis at z -472, heading +y, pitch -12) gives the level's first entries:

  | eye y | first level entries (node, key; K = the level's start key) |
  |---|---|
  | -1,000 | `8006A048`, `80069F80` (the pickup at (0, 1,200, -545), under an offset: K + 1), `8006B874` (the **health pickup** at (0, 0, -551), LOD `8007F7E4`, under an offset: K + 2), then `8007E9D0`, `8007CB20`, `8007B858` (ramp and tunnel, top-level: K) |
  | +1,000 | `8006A048`, `80069F80` (K + 1), then the ramp and tunnel (K): the health pickup is behind the eye and culled |

  The pickups are the root's last children, so they are queued first; they are sprite / beam meshes (no polygons) under
  offset nodes, which the game LIKELY hides when the pickup is taken (the offset node's skip flag, +19 & 2; GUESS, not
  traced). Each gives the emitter a new
  key, so the tunnel's first entry after it gets its own translation again. With the pickup at y 1,200 taken (or out of
  view), **the health pickup at y 0 is the last such entry ahead: once the eye passes it, the tunnel is the first
  entry**, and whenever the car's last queued part has key K the whole tube is drawn at the car's translation. That is
  the user's "right for the first half, glitched from the health pickup on". In video a the glitch comes back at 12.5 s
  with "Ricochets Loaded" on the screen: the pickup ahead had just been taken (LIKELY the one at y 1,200). The start
  (the south ramp, 2.0-3.75 s, flickering) depends on the same two conditions (the pickups in the frustum from the
  ramp's pitch, the car's part order). No band, zone or record choice changes between y -1,000 and y +1,000 (10.3).
- **Why the mirror, retail and the stills do not show it:** the mirror has no own-car walk (`portVisualOwnCar` returns
  0 for mode 4); retail walks the level first, right after the emitter's cache is reset (`S->trans_id = -1`); the
  stills A1 / A2 (photo mode) had another camera and car-part order (not reproduced).
- **Elsewhere:** every arena has top-level-key meshes (`ha3/toplevel.py`: `ISLANDS` 444 of 670 meshes, `PARIS` 205 of
  735, `DISH` 1,404 of 1,519, `HKONG` 262 of 732, `FREEWAY` 445 of 729), so level pieces drawn at the car's place can
  appear in any arena whenever its first queued meshes are top-level ones. LIKELY also video b's grey block (9.5): a
  top-level piece drawn displaced rather than the north temple. Not checked frame by frame.

### 10.5 The fix (built; drawing only)

`renderScene` (`src/semantic/engine_render_scene.c`, under `TM2_PORT_VISUAL`, inside bit 2's `if (own)`): after the own
car's walk, **one more object number** (`*objects = *objects + 1;`), so the level root's walk starts at a key no entry
has used, as retail's level does after the cache reset. Nothing else reads the counter (the walker only, as a key
source; the port's stats do not). The later keys of the view shift by one; keys are only compared with the previous
entry's for equality, so nothing else changes. Retail and every build without the define are byte-identical (the code
is inside the define and the bit); with bit 2 clear nothing runs.

- **Check:** `tools/check_custom.py` check 2 (with `TM2_PORT_VISUAL`): in `renderScene`'s code the counter is stepped
  (`lw r,0(B); addiu r,r,1; sw r,0(B)`, through the same pointer register as the step before the own car's walk)
  exactly once between the own car's `renderWalkModel` call and the level root's. **Negative control:** the request 63
  link `work/pv/r63_port_P4` has only the slot counter's step there and fails it.
- **State:** drawing only: the render context's counter is reset by `renderInitState` every view, the queue and the
  emitter's cache are per view; `check_rollback.py` unchanged (9 / 10 pass).
- Verification: full build `work/build-20261026-033321-835047` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK from
  `.LIB` `f557c1e3…`, default port variant `1becd00a…`: unchanged); links `work/pv/ha3_{rb,port,allh16}_{P4,P2}` (the
  port's `ha3_port_P4` `e321e1bc…`; `renderScene` 801543CC, now 1,496 bytes; `portVisual` 8088C1F8); `check_all.py`
  27 / 27 (proofs, placement with `--port-limits 2`, check_rollback, check_custom with the new check 2 line, check_humans);
  the negative control `work/pv/r63_port_P4` fails the new line. Not runtime-tested.

### 10.6 For the port and the user

1. **Rebuild on the new decomp** (the port variant's `renderScene` changed; read addresses from the link's
   `symbols.tsv`) and drive video a's route again in the default view: the tunnel should stay whole all the way,
   ramps included.
2. **A one-switch confirmation on the old build:** clear `portVisual` bit 2 (`portVisual &= ~4`); the glitch should
   go (and bit 2's own purpose with it: the own car is then walked last, with the other objects).
3. **To see the mechanism at run time** (request 69's tools; one line a step, view 1 after its `renderScene` walk,
   before the emitter): the mesh queue is `portMeshList` (32-byte entries: +0x00 node, +0x14 object, +0x18 slot,
   +0x1C translation key); the own car's entries are the first ones (log the queue length right after the own car's
   walk, or count the entries whose +0x14 is the own car's object). Log: the key of the own car's last entry, the
   level's start key (the walk's first top-level entry's key), and node + key of the level's first four entries. On the
   old build the glitched steps are those where the car's last key equals the level's first entry's key; on the fixed
   build that never happens. Drive as in the trace, health pickup present and taken.
4. Video b's grey block (9.5): look again after the rebuild; if it is gone it was this bug.
5. The view-4 question is closed (10.2: `carUpdateContacts` forces view 2 on covered ground). To confirm the ground
   kind: log the human record's front-left wheel kind and `worldSectionGroundClass[kind]` per step in the drive.

## 11. Request 72: Paris's church drawn from anywhere (built; `portVisual` bit 6, `TM2_PORT_HIDDEN_AREAS`)

Port request 72 (2026-10-26) takes section 5 (a): the port builds the depth buffer (its Video > Hidden surfaces, a
depth test for the views' 3D order tables, the size limit lifted for polygons with depth) and sets `portVisual` bit 6
only while that test is on; the decomp draws the church's inside from anywhere. **The user's rule (2026-10-26):
nothing hidden.** Whatever is in the camera's line of sight is drawn: with the church's door intact it is a wall,
once it is blown up the doorway shows the inside from any angle and distance that sees through it. Only real
two-version swaps stay as they are. The church first; Amazonia's tunnel and the other places of section 3 follow
under the same bit (11.5).

### 11.1 What bit 6 changes (CONFIRMED in the data; `ha4/church2.py`, `ha4/doorway.py`)

`renderScene` calls `portHiddenAreasBegin(view)` before its walks (after `portFarLodBegin`) and
`portHiddenAreasEnd()` after them (before `portFarLodEnd`), the window `portFarLod` already uses: nothing but the
walker reads the level model in between, and the hit walker, which reads the same zones and records, sees the file's
values again (collisions stay retail). With the bit, Begin checks the church's nodes at the level file's fixed
addresses (the file is built for 0x80019C40: zone, children, LOD node, record and door pointers; another level
loaded, nothing is patched) and patches six words for the walks:

| word | PARIS | PARIS2 | file value | with bit 6 | why |
|---|---|---|---|---|---|
| zone A outside child (+8) | `8002E5EC` | `8002E7A4` | the outer walls `8002C0B0` | `portHiddenAreas.group` | the zone draws both |
| zone A inside child (+12) | `8002E5F0` | `8002E7A8` | zone B `8002E58C` | the same group | |
| walls far record max | `8002B668` | `8002B890` | [900,16,626) | 0 (never chosen) | its facade `8002B5D0` has **no doorway**: one polygon covers the hole (x -256..256, z 192..832 at y 7,296) |
| walls mid record max | `8002B8E0` | `8002BB08` | [275,900) | [275, unbounded) | the facade with the doorway (16 polygons) at every distance |
| intact door far record max | `8002E66C` | `8002E824` | [275,900) | [275, unbounded) | past 900 retail draws no door: the doorway would show the inside |
| broken door far record max | `8002EC6C` | `8002EE24` | [275,1,000) | [275, unbounded) | the broken door's frame at every distance |

The group (`portHiddenAreas.group`, opcode 1, never culled: +21 bit 2) holds two children: the outer walls
(`8002C0B0`; PARIS2 `8002C2D8`) and the real inside (`8002E510`: the nave `8002C524` with 106 polygons, the
teleporter pad, the windows, the two pickup-spot selects; PARIS2 `8002E6CC`). So whatever the eye, the walker draws
the walls and the inside, and **never zone B**: its 27-polygon stand-in (`8002C0CC`; PARIS2 `8002C2F4`) is not drawn,
so it is never drawn over the nave it copies (92 coplanar pairs with it). This is section 5 (a)'s "zone A both, zone B in"
with one node.

Not changed:
- **The ceiling stays one-sided** (the user, 2026-10-27: try without it first). From above the walls (the Eiffel
  Tower's deck, z 1,344..3,001) the church is an open box and the inside is now seen through the top: correct by
  depth (nothing covers it), but not the original building's look. **Request 75 adds a roof instead (section 12):**
  the two-sided ceiling offered here would sit 96 above the surface cars drive on, and over the notch's floor.
- **The inside's own LOD bands** (the pad `[400,4000)`, its parts `[0,400)`, the pickup-spot details `[0,700)`):
  distance detail as everywhere else, not hiding (Farthest's near scale stretches them like any band).
- **Objects** (pickups, breakables, cars inside): the object loop draws them whatever the zones, in retail too; with
  the depth test they now hide behind the walls instead of painting over them.
- **The door's select node** (`8002EEF0`; PARIS2 `8002F0A8`; intact / broken is game state): untouched; its near
  records `[0,275)` are retail's.
- With the bit clear, or with the bit on another level, `renderScene` draws as retail (Begin returns 0). Retail and
  every build without the define are byte-identical.

Cost per view, when the church is in the frustum: the inside's 159 polygons (from outside the zones too), the
16-polygon facade instead of the 6-polygon far one past 900, and the door (2 / 7 polygons) past 900 / 1,000.

### 11.2 For the port's depth test (request 72 item 3; `ha4/cop.py` on both files)

- **Reading the state:** `portHiddenAreas` (`port_visual.h` struct `TM2PortHiddenAreas`, 0x88 bytes, `.port_bss`,
  its address in the link's `symbols.tsv`): after `renderScene(db, view)`, `drawn[view & 7]` is the number of words
  that view's walks patched (6 = the church drawn from anywhere, 0 = drawn as retail); `place` is 1 (PARIS), 2
  (PARIS2) or 0. The bit is read at each `renderScene` call, so the port may also change it between views (e.g.
  clear it for the mirror if its depth test is off there).
- **The walls and the inside's walls share planes, facing opposite ways** (the inside's wall is the back of the
  outer wall: 72, 84 and 24 coplanar pairs with the mid, near and far wall records). Both are one-sided: only one of each pair
  faces any eye, so equal depths there never meet unless back faces are drawn.
- **Coplanar overlays in the inside** (same plane, same facing, overlapping): the two pickup-spot pads (`8002E038`,
  `8002E388`) and their `[0,700)` detail (`8002E18C`, 8 of its 10 polygons double-sided) on the nave's floor; the
  teleporter pad's four detail meshes (`8002D928`, `8002D9E8`, `8002DAE8`, `8002DC30`, `[0,400)`) on each other; a
  window (`8002DF64`) on a nave wall; in PARIS a window `8002DE28` under an overlay `8002DE90` (OT bias field +7).
  In retail they win by the painter's order: 43 of the nave's 106 polygons carry an OT bias of +30 (the emitter's
  field, prim+12 bits 7..12, `DRAW_DISTANCE.md` 7), so they sort farther, and equal slots go in link order. The depth
  test must keep that: section 5 (a) rule 2's tolerance (a later polygon passes at `new <= stored + e`), or the
  overlays flicker. The pad's far mesh `8002D60C` (`[400,4000)`) is another record: never drawn with its parts.
- **The door** lies in the facade's plane (y 7,296) but inside the drawn facade's doorway hole: no overlap. (The far
  facade, which would overlap it, is never drawn with the bit.)
- **Drawn twice:** nothing: the stand-in is not walked (11.1), and the walls are walked once (the group, not also
  zone B's copy of them).

### 11.3 Checks

`tools/check_custom.py` check 12 (with the define): the four port functions are in `.port_text`, reach no `rand()`,
are called only by `renderScene` (Begin, End) and each other; in `renderScene` Begin comes before every
`renderWalkModel` call and End after every one and before the emitter; and in the MIPS interpreter across the link,
with PARIS.DMD and PARIS2.DMD from the disc at 0x80019C40:
- with the bit, Begin returns 6 and names the place; a walk of the level (the walker's choices: the zone's child for
  an eye outside or inside the regions, each LOD's record for the distance, the door's current child) at 50, 600,
  2,000 and 20,000 world units reaches the nave, the near or mid walls (never the far facade), the door past 275
  (intact or broken), and never the stand-in; before Begin the same walks are retail's (no inside from outside, the
  far facade and no door at 2,000);
- End leaves every byte of the level file as it was (also with the far record already extended by `portFarLodBegin`:
  its extended value comes back);
- with the bit clear, and on ISLANDS, Begin returns 0 and changes nothing.

A link without the define fails check 12 (the negative control: `work/pv/r72_rb_P4`). Verification: full build
`work/build-20261027-182223-339240` (hybrid `9219f7ba…`, source-only `d755c86e…`, SDK from `.LIB` `f557c1e3…`:
unchanged); links `work/pv/r72_{rb,port,allh16,ha}_{P4,P2}` (the port's `r72_port_P4`: `renderScene` 801543CC,
`portHiddenAreas` 80954990, `portHiddenAreasBegin` 8096C024); `check_all.py` 31 / 31. Not runtime-tested.

### 11.4 For the port

1. Rebuild on this decomp with `--port-define TM2_PORT_HIDDEN_AREAS` (needs `TM2_PORT_VISUAL`) and set bit 6
   (`TM2_VIS_HIDDEN_AREAS`, 0x40) while the depth test is on.
2. The request's test: the photo hooks (request 68's D1-D3: outside zone A, in zone A, past the door plane) with the
   door intact (a wall from every spot) and destroyed (the inside through the doorway from every spot, D1 included,
   and from far away), and a drive into the church in views 2 and 4 (view 4's eye, above the regions, now sees the
   inside through the open top: 11.1).
3. If the overlays of 11.2 flicker: the depth tolerance. The open top: request 75 built a roof (section 12).

### 11.5 Next under the same bit (the user's rule)

In section 3's order, each a reviewed patch list like the church's: Amazonia's tunnel (`8007B6A8`, `8007C970`,
`8007D698`, `8007E700`, `8007F748`: the detailed records unbounded; zone `800692BC` in; the hut's `[400,500)`
record), New York's rooftop interior and the rooms of its real blocks (`in`, never `both`: one version each), Roof
Tops' upper hall and corridor, Hong Kong's passages behind the portal cards (`in`, which drops the cards), and the
roofed pieces with short bands (Holland's barns, Amazonia's north temple, Antarctica, Swamp's shelter). Real
two-version swaps (Cyburbia's arena, Paris `8006EF1C`, Hong Kong's arena swaps, the Eiffel deck) stay as they are.

## 12. Request 75: the church's roof (built; `portVisual` bit 6, `TM2_PORT_HIDDEN_AREAS`)

Port request 75 (2026-10-28, the user's playtest note 11): on top of the church "it's mainly 4 square tiles as the
roof", they vanish at some angles, mostly close up, and the ground shows; the user wants them always there. The
request offered section 11.1's two-sided ceiling and asked what the 4 tiles are if not those.

### 12.1 What is on top of the church (CONFIRMED in PARIS and PARIS2; `ha4/roof.py`, `ha4/under.py`, `ha4/hvp.py`)

Position units; y as the level's (the church's own frame is offset y 7,296):

- **The surface cars drive on is the collision's, not a polygon:** hit volume `8002EF58` (PARIS2 `8002F110`) is a
  slab z 536..824 over the whole church (x -768..768, y 7,296..8,928). The towers' bodies (`8002F098`, `8002F0E8`, x +-240..544,
  y 7,296..8,064) and a block behind the notch (`8002F048`, x -256..256, y 7,856..8,160) are solid from z 832 up;
  above z 1,472 everything over the church is solid (`8002F1D8`, PARIS2 `8002F390`: no flying out over the walls).
- **The 4 tiles are the notch's floor:** between the two front towers (x -256..256, y 7,296..7,872) the walls' near
  mesh (`8002B8EC`, record `[0,275)`; PARIS2 `8002BB14`) has 4 quads at z 832 facing up, 256 x 288 each (32 x 36
  world units); its mid mesh (`8002B674`, `[275,900)`; PARIS2 `8002B89C`) has one quad over the same area. They are
  the only polygons of the church's outside that face up.
- **Behind the towers (y 8,064..8,928) there is no polygon at all over the slab.** The nave's ceiling (`8002C524`, 30
  quads at z 920) faces down and lies 96 above the slab's top: from above it is a back face. Retail draws nothing
  there (the zones keep the inside away from an eye that high); with bit 6 the eye sees down through the open top
  into the nave, or, past its walls, the ground: "the ground shows".
- **Why the tiles vanish close up** (LIKELY, port side): the 4 tiles' mesh is flagged for the near pass (mesh +26
  bit 2), which in retail cuts polygons nearer than OT 70 (20 in split screen) so they stay under the GPU's size
  limit. With `wobble = none` the port turns the near pass off (`vplus/precise.c`), so a tile with a corner close to
  the eye is drawn whole or dropped, as section 1's tunnel holes. Nothing in the walls' LOD drops them: both the near
  and the mid record have the notch's floor. The port's near-plane handling decides it; the roof below has the same
  flag (it is the nave's header), so the same applies to it.

### 12.2 The roof (built)

Not the two-sided ceiling: at z 920 it would sit 96 (12 world units) above the slab cars drive on, so a car on it
would look sunk into it; and its front rows lie over the notch (88 above its tiles) and inside the towers.

Instead, with bit 6, `portHiddenAreasBegin` (`portHiddenRoofBuild`, called by `portHiddenChurch`) builds in
`portHiddenRoof` (`.port_bss`, 0xAC bytes, `port_visual.h` struct `TM2PortHiddenRoof`, after `portSpecialState`):
- **two mesh headers** copied from the nave's (`8002C524`; PARIS2 `8002C74C`), one over its primitives 13..24 (the
  ceiling's back two rows, y 8,348..8,924) and one over 31..36 (the row in front, y 8,060..8,348): 18 quads, the
  file's own primitive records (nothing copied but the 44-byte headers). Each is checked to be a quad with its first
  vertex at z 920, and the nave to have 106 primitives; otherwise no roof (the rest of the church patch stays);
- **a turn node** (opcode 5): half a turn about y (rotation diag(-1, 1, -1), offset z 1,752), so the quads lie at
  z 832 facing up (x mirrored onto itself: the tiles are symmetric about x 0). A proper rotation: the walker's
  NCLIP sees the up-facing winding (a mirror in z alone would not: a flat polygon's winding seen from above stays).
  From inside the nave the roof is a back face, and the nave's own ceiling stays: inside, nothing changes;
- **a group** (opcode 1, never culled) holding `portHiddenAreas.group` (the walls and the inside) and the turn node:
  zone A's two children point at it instead.

The roof is at the notch floor's height (832, 8 above the slab), where the walls' upper parts start (the side walls
and towers run z 832..1,472), so it meets them and does not overlap the notch's tiles (y 7,872 against 8,060). The
middle strip behind the notch (x -256..256, y 7,872..8,060, the solid block's front) and the towers' insides stay
open: the towers are shells with no back wall, so from the roof the towers' insides show the backs of the facade
(one-sided walls, not drawn). The towers' tops (z 1,472) have no polygon either.

`portHiddenRoof.built` is 2 after a view's Begin that built it, 0 otherwise (the bit clear, another level, the nave not
as listed). Cost when the church is in view: 18 quads, one more matrix slot (the turn), one more object number.

### 12.3 Checks

`tools/check_custom.py` check 12, extended: the two new functions are in `.port_text`, reach no `rand()`, are called
only by the church patch (`portHiddenRoofMesh` by `portHiddenRoofBuild`); in both Paris files, with the bit, the
walks at every distance, eye in or out, door intact or broken, reach both roof meshes, and their 18 quads, through the
turn node, lie at z 832 facing up within x -768..768 and the church's local y 764..1,628 (level y 8,060..8,924); with
the nave's primitive count changed, the church is patched (6 words) but no roof is built or walked. A link without
the change fails it (no `portHiddenRoof`).

### 12.4 For the port

1. Nothing new to switch on: a build with `TM2_PORT_HIDDEN_AREAS` and bit 6 set has the roof.
2. Test: the photo hooks above the church (the Eiffel deck, view 4) and a car parked on the roof behind the towers and
   in the notch, the camera close to the roof and at the edge looking back. Expect the roof (the ceiling's texture,
   turned) from y 8,060 back and the notch's tiles in front.
3. If tiles still vanish close up, in the notch or on the roof: the near plane (12.1's last item), the port's side.
4. Left open, ask if wanted: the towers' open backs and tops, and the strip behind the notch (a few more quads from
   the same file through other turn nodes, or the walls' upper parts two-sided).

### 12.5 Request 76 item 1: "a little wall" (answered by request 78: 12.6)

Port request 76 (2026-10-28): from on top of the church the user sees "a little wall poking through the edge of the
building", a car drives through it, and it was not there before request 75. The port's tests: a car put in the notch
(0, 7,600) sits at z 848; one put behind the towers ((0, 8,400) or (0, 8,500), z 850..1,100) ends at z 16 on a
water-like floor, no church around it.

- **Where the roof is** (CONFIRMED; check 12 and the walker's opcode 5, `reference/renderer_walker_engine_render_scene.c`:
  the child's point is the rotation's transpose times it plus the offset, as check 12 models it): x -768..768 (the
  side walls' x, the whole facade's width; the towers are x +-240..544 inside it), y 8,060..8,924, z 832. The side
  walls end at y 8,832 and the church has **no outside back wall**; the inside's back wall is at y 8,924. So the
  roof's last 92 (y 8,832..8,924, 11.5 world units) lie past the end of the outside walls, over the back of the
  inside. Nothing of it is outside x +-768.
- **Seen from the notch:** the notch's back wall (y 7,872, z 832..1,472) and the towers' inner walls stand between the
  notch and the roof, so from in the notch the roof is hidden; from above z 1,472, or from beside the towers
  (x 544..768), it is seen, and from car height it is nearly edge-on.
- **12.1's collision reading is in question:** a car placed behind the towers falls, so the slab `8002EF58`
  (z 536..824) is LIKELY not what holds a car there (hit volumes may be the camera's walls, request 63's, more than the
  cars' floor). If nothing holds a car behind the towers, the roof draws a surface that is not there: a car on it
  sinks through, and a car's camera low over it sees it as a thin plate.
- **Options once the picture shows what it is:** (a) no roof (bit 6 as request 72 built it); (b) the roof without its
  back row (y 8,060..8,636; the rest open as before); (c) keep it. The tiles are the file's own and 288 deep, so the
  roof cannot end at y 8,832 without new polygons.

### 12.6 Request 78: the lip is the nave's walls (built; the restore came too early, fixed by request 84: 12.7)

Port request 78 (2026-10-29, the user's picture of request 76's wall): Thumper on the roof near the towers, a tall
wall of facade texture standing out of the roof's edge, close by; the car drives through it. Wanted: the roof, not
the wall.

- **What it is** (CONFIRMED, `ha4/walls.py` over both Paris files: every polygon of the church reaching above z 832):
  the nave's own walls. The inside's mesh (`8002C524`; PARIS2 `8002C74C`) has its side walls (x +-768) and its back
  wall (y 8,924) from z 512 up to its ceiling at **z 920**, facing in. The roof (12.2) is at z 832, so their top 88
  (11 world units) stands above it all round, the inner face toward a car on the roof: from close by a wall, from
  farther a thin lip "poking through the edge". Nothing hits it (the walls are drawing only; the car's floor there is
  the collision's). It was there before request 75 too, but with no roof the eye saw down into the nave, so the
  walls read as its sides. Nothing else of the church stands up out of the roof: the outer side walls (x +-768,
  z 832..1,472) face out (from the roof, back faces), the towers face out.
- **The fix:** for the walk of a view whose eye is above the roof (world z > 832: `-viewGetEyeTrans(view)[2]`, the
  eye translation being minus the eye's position), `portHiddenAreasBegin` moves every vertex of the nave's primitives
  at z 920 to z 832 (`portHiddenRoofMove`: 42 vertices, the ceiling's and the walls' top corners; the vertex array is
  shared only with the stand-in `8002C0CC`, which bit 6 never walks) and sets the roof's turn offset to 2 x 832, so
  the roof, built from the same vertices, stays at 832. The walls then end at the roof; the ceiling, now at 832, faces
  down, so from above it is a back face. `portHiddenAreasDrawn` (request 84; `portHiddenAreasEnd` as first built)
  puts the 42 back (`portHiddenRoof.lowered` 1 while they are moved). A nave with a vertex already at 832 is not moved (the move could not be undone exactly).
- **Why nothing else changes:** from an eye above z 832 nothing of the nave between 832 and 920 can be seen except
  through the roof's own openings (the strip behind the notch, the towers' open insides), and through those the eye
  looks down, below 832. Inside the church (floor z 192, ceiling 920) the eye stays below 832 in play (the chase
  camera sits well under 640 above the car), so the inside is as before. An eye inside the nave above z 832 would see
  the ceiling at 832; not expected (LIKELY).
- **Split screen and the mirror:** each view's walk decides from its own eye; the vertices are put back after each.
- **Checked:** `tools/check_custom.py` check 12: `portHiddenRoofMove` in `.port_text`, called only by Begin and End;
  in both Paris files with the eye at z 1,000 the 42 top vertices are at 832, none left at 920, the roof's 18 quads
  still at z 832 facing up, and End gives back the file's bytes; at z 832, or with a nave vertex already at 832,
  nothing moves. Not tested in game.

### 12.7 Request 84: "the wall is still there": the vertices were put back before they were drawn (fixed)

Port request 84 (2026-10-29, the user's playtest of request 78, decomp 2d5b290): the wall still stands round the
roof's edges behind the towers. Asked: what else stands up there, and is `portHiddenRoof.lowered` set.

- **The cause** (CONFIRMED in `renderScene`): the walk only queues meshes; the emitter (`portFarDistanceEmit` /
  `sub_800CDC54`) and `renderNearPolys` read their vertices afterwards. Request 78 put the nave's vertices back in
  `portHiddenAreasEnd`, which runs after the walks but **before** the emitter (rightly for the 6 node words, which
  only the walk reads). So the emitter drew the walls whole, up to z 920, and the roof, whose turn offset had been
  moved to 2 x 832 for the lowered vertices, at 1,664 - 920 = **744**, 88 below the roof's height: the walls stood up
  176 above it. `lowered` was 1 during the walk and 0 again by the time the view was drawn. check 12 ran Begin and End
  but not the emitter, so it did not see this.
- **The fix:** the vertices stay moved until the view is drawn. A new call, `portHiddenAreasDrawn()`, after
  `renderNearPolys` in `renderScene` (under `TM2_PORT_HIDDEN_AREAS`, when Begin patched), moves them back and clears
  `lowered`; End still puts the node words back before the emitter. Nothing else reads mesh vertices in between
  (the hit walk uses hit volumes; the next view's Begin comes after this view's Drawn).
- **Nothing else stands up out of the roof** (CONFIRMED, every church polygon on the side planes and above z 832,
  `ha4/side.py`, `ha4/walls.py`): the outer side walls' upper parts (x +-768, z 832..1,472) face out, as their lower
  parts do: from the roof they are back faces, not drawn (if the port draws back faces in Depth mode, they would show
  as tall facade walls round the roof); the towers (x +-240..544, z 832..1,472) face out and are open at the back; the
  notch's back wall (y 7,872) faces the notch. The nave's walls were the only front faces above the roof.
- **Checked:** check 12 now also asserts that after End the 42 are still at 832 (`lowered` 1) and that
  `portHiddenAreasDrawn` gives back the file's bytes; and that `renderScene` calls Drawn once, after the emitter and
  the near pass. A request 81 link fails it. Not tested in game.

## 13. Request 88 item 5 (2026-10-30): Amazonia's temple, "the wall in the way out" (answered, nothing built)

The user: inside Amazonia's temple, looking out of the opening to leave it, the opening shows a texture from far away
and is see-through only up close; asked: see-through always, as Paris's church (request 72, bit 6).

**What is there** (CONFIRMED in `ISLANDS.DMD`; scratchpad `r88/temple.py`, `hole.py`, `lods.py`, `hvroom.py`):
- The north temple (section 3: `8005AC98` and the outer shell `8005C4F0`, LOD reference points (0, 14,000, 400) /
  (0, 14,200, 480)): a room at x +-600, y 13,800..14,600, z 200..600, reached by one passage at x -200..200, y
  13,400..13,800, z 200..400 from the front slope (y 13,400). Every LOD band of the temple reaches 500 / 800 world
  units, so from inside it the near records (134 / 146 polygons, both with the passage's opening) are always the
  ones drawn: no band switch happens inside.
- **The passage holds a breakable wall**: select node `8006957C` (opcode 9, id 504, a target: `PICKUP_LOOKS.md` 5),
  child 0 (whole) = mesh `800693F4` (4 polygons: 2 in the front slope's plane facing out, 2 across the passage's
  inner end at y 13,800 facing into the room) and **hit volume `800694F4`, which fills the passage**; child 1
  (destroyed) = nothing. The file starts it whole (+7 = 0); shooting it switches it.
- So the "texture" seen from inside is the wall's inside face, and the wall is solid while it stands: the room's only
  way out is closed (a map of the hit volumes at z 260: the passage solid only through the select's child 0). It is
  see-through up close because its mesh has no near-pass flag (+26 & 4) and the eye's near plane then drops the
  polygon (R5.2: the same corners behind the eye as the ground).

**Decision (the user, 2026-10-31): keep it a wall.** Request 72's rule: an intact door is a wall. Bit 6 changes
nothing here; destroying the wall opens the passage (both faces and the hit volume go). For the port: the up-close
loss is the renderer's near-plane drop (`RENDER_ANSWERS.md` R5.2), not the data.

## 14. Request 97 (2026-11-02 playtest): Hong Kong's temple doorways (built; `portVisual` bit 6, `TM2_PORT_HIDDEN_AREAS`)

### 14.1 What is there (CONFIRMED in HKONG.DMD, built for 0x80019C40)

- **The room is a zone:** `80065664`, region x +-3,487, y +-3,487, z 864..3,760.
  - Eye in the room: only the temple's inside (`800655FC`).
  - Eye outside it: only the city (`80055270`).
  - The real city is never walked while the eye is in the room.
- **Each doorway is a zone of the inside, with a card in it:**

| doorway zone | region | outside (+8): most of the room | inside (+12): close to the door |
|---|---|---|---|
| `80063FB4` south (y -3,481) | x +-768, y -3,487..-1,923, z 699..2,025 | `800635D0`: the card, 4 quads, a picture of the city | `80063F50`: a 53-polygon stand-in |
| `80064524` west (x -3,481) | x -3,487..-1,920, y +-768 | `80063FC8`: the card | `800644C0`: 53 polygons |
| `80064B40` east (x +3,481) | x 1,920..3,487, y +-768 | `80064538`: the card | `80064ADC`: 58 polygons |

- **The flat picture:** the card shows until the eye is within about 196 world units of the door. Even then retail
  draws a stand-in, never the city.
- **The other direction:** the city's three passages to the doors are zones (`80048CE0`, `80048D68`, `80048DF0`)
  that draw a 73-polygon stand-in of the inside (`80048190`) close to the doors, and empty groups otherwise.

### 14.2 Built: `portHiddenTemple` (with bit 6, after the two Paris files)

`portHiddenZoneIs` checks every node first (another level: nothing is patched). Then, for the walk, 8 words
(`TM2_VIS_HIDDEN_PATCHES`):

- **The room:** its zone's two children both become `portHiddenAreas.group` (opcode 1, never culled), which holds
  the city and the inside. So both are drawn, wherever the eye is.
- **The doorways:** the three doorway zones get +4 bit 1, which the walker skips. No card, no stand-in: the real city
  through the doorway.
- **The passages:** the three passage zones draw their empty outside group close to the doors too, so no stand-in
  covers the real inside.
- **Place:** `portHiddenAreas.place` is `TM2_VIS_HIDDEN_HKONG` (3).
- **Collisions:** End puts every word back before anything else reads them, so the hit walker stays retail.
- **Cost:** in the room, the city at the cost of standing just outside it. The doorway tunnel (`80045354`, 6
  polygons) is reached from both sides and so drawn twice, which is harmless with the depth test.
- **The subway's zone `80070864`** is left alone.
- **Checked:** `check_custom.py` check 12's Hong Kong part. The retail walk from the room's zone: the city outside,
  the inside and the south stand-in inside. With the bit, Begin returns 8 and names the place. Walks near and far,
  with the eye in or out of the zones, reach the city and the inside and none of the cards, stand-ins or the
  passages' stand-ins. End gives the file back. With the room's zone changed, nothing is patched. `hidden_walk` now
  skips a zone with +4 bit 1, as the walker does. Not tested in game.

## 15. Request 104 (2026-11-04 playtest): Hong Kong's subway, parts that pop in late (built; `portVisual` bit 6, `TM2_PORT_HIDDEN_AREAS`)

The user: as the tunnel opens up outside, border effects pop in late, and the overhead lights, billboards and signs
in the tunnel show up late. Every part below was found in HKONG.DMD (position units, z up, the street at z 400; LOD
bands in world units). No video was needed.

### 15.1 The subway's zone (CONFIRMED in the code and the file)

- **The swap:** zone `80070864` (opcode 11, root 1), one region: x ±12,684, y -7,900..3,100, z -160..400. The eye
  inside it walks the subway's version `800707D4` only; outside, the temple's zone `80065664` (the city
  `80055270`) only. The street is at z 400 everywhere, so in practice the eye's height decides.
- **Where it flips:** the eye crosses z 400 with the floor under it about 290 (the chase eye 112 over the car): at
  |x| about 2,900 and 6,700 on each corridor, some 1,500 before each tube mouth.
- **The layout:** two corridors (south y -7,600..-6,800, north y 2,000..2,800). Each has a middle tunnel (|x| <
  1,120), enclosed sloping tubes (|x| 1,120..4,320 and 5,280..8,480), and a bridge over a cross street at |x|
  4,320..5,280, where "the tunnel opens up": 8 tube mouths. Corner and end tunnels join the corridors at |x|
  8,480..12,080.
- **Shared by both versions:** only the sloping tubes, group `800382A0` (8 instances).
- **Only in the subway version:** the corner (`8006ADD0`), end (`8006D5F8`) and middle (`8006FC5C`) tunnels; the 4
  hanging signs (`80070664`); 11 pickups (`800706CC`..`800707BC`); stand-ins of the far mouths' facades (`8006D8CC`,
  `8006DB48`, `8006FF30`). It lacks the bridges, the cross streets, the mouth walls and the whole city: from below
  the street, only the facades and the sky. **The "border" pop-in.**
- **Only in the city version:** four mouth zones (`80038B94`, `80038CE0`, `80049F64`, `8004A000`): close to each
  bridge the real tubes and two stand-in tunnels (`80038744` [0,1400), `80038300` [0,1200)); farther, a 12-polygon
  box. The city has no deep tunnels and no signs. **The signs at x 6,880 (south) and 7,680 (north) sit right at the
  flip,** so driving down from a bridge they appear only when the eye drops below 400.
- `800673E4` (in the subway version) holds only type-8 nodes, which the renderer skips: nothing drawn.

### 15.2 The short bands

| Set | Finest | Coarse | Past it |
|---|---|---|---|
| Signs `80070164`, `80070374`, `800704D0`, `8007062C` | [0,200) | 1 polygon [200,900) | nothing |
| Ends `8006D5DC` | [0,600) | [600,1000) | nothing |
| Corners `8006ADB4` | [0,600) | [600,1000) | nothing |
| Middles `8006FC40` | [0,600) | [600,900) | nothing |
| Sloping tubes `80038284` | [0,600) | [600,900) | nothing |

Inside each, the sections switch detail at 200..400 to a coarse record that runs to 10,000 (16-24 ceiling tiles
against 1: LIKELY the "overhead lights"); those have a coarse version at every distance, so they change detail but do
not pop in. `portFarLodNearScale` scales them as every band.

### 15.3 Built: `portHiddenSubway`, under bit 6 after the temple (request 97)

`portHiddenAreasBegin` calls it once the temple is patched. Every node is checked first (a node not as listed: the
temple only). Then, for the walk (24 words; `TM2_VIS_HIDDEN_PATCHES` is now 32, `portHiddenAreas` 0x1D0 bytes):

- **The subway's zone** draws `portHiddenAreas.subway` wherever the eye is: the temple's zone (the city, with the
  temple's own patches) and `subway_parts`, the subway's own parts: its corner, end and middle tunnels (8
  transforms), its signs and its pickups. Not its tubes (the city's mouths draw them) and not its facades.
- **The four mouth zones** draw their inside child (the real tubes) from anywhere: no box.
- **The city's two stand-in tunnels** never (their records' max 0): the real tunnels are drawn.
- **The 8 short bands open,** as Amazonia's (FAR_LOD.md 11): finest at every distance, the coarse record never.
- No node is walked twice, so no surface is drawn twice and no animation stepped twice. End puts every word back;
  the hit walk never sees the patches.
- **Needs the port's depth test** (as the temple): from the street the subway's tunnels are drawn under it, and from
  below the street the city above.

### 15.4 The cost (one view, before the frustum: upper bounds)

| Eye | Retail | Built |
|---|---|---|
| In the middle tunnel | 476 | about 2,030 (the city 1,437, the subway's parts 169, the open bands) |
| On a bridge | 1,484 | about 1,920 |
| On the street, 3,000 away | 1,236 | about 1,720 |

Inside the tunnel it costs what the street view above it already costs; outside it adds 55..470.

### 15.5 To test

- The teleport (`feat_hooks.c`) moves only the car: LIKELY the eye stays above z 400 (the camera eases toward the
  car from where it was), so the walker keeps picking the city. Log the eye (`TM2PC_CAM_TRACE`, `h+0x1C`) after it:
  z 400 or more confirms. Either write the camera's eye and look-at too, or teleport into an outer tube, e.g.
  (7,600, -7,200, 100) facing -x, and drive up onto the bridge.
- Compare the frames where the eye crosses z 400, with bit 6 on and off. A pop left at another eye height is one of
  15.2's inner switches: then a video of that frame with `TM2PC_NEAR_SCALE=1`.

**Checked:** `check_custom.py` check 12's subway part. The retail walk: the city (with the mouths' box) out of the
zone, the subway in it, sharing only the tubes and the pickups' models. With the bit, Begin returns 32 (the temple
8, the subway 24); walks at 50..3,000, the eye in or out of the zones, reach none of the stand-ins or opened coarse
records, and each mesh exactly as often as a walk of the city (the temple's patches, the mouths' tubes, the opened
bands' finest) plus the subway's own parts: no surface twice. End gives the file back; with the subway's group changed only the temple is
patched. Not tested in game.

## 16. Request 113 (2026-11-11 online Versus): the church's door wall, invisible from inside (built; `portVisual` bit 6, `TM2_PORT_HIDDEN_AREAS`)

**What it is (CONFIRMED, disc).** The wall a car spawned inside Paris's church saw through and then hit is the church
door: target 503 (`targetRegister` gives it a counter of 8), the select node `8002EEF0` in the doorway
(`8002F0A8` in PARIS2), section 1 of this file. Child 0 is the standing door; child 1 the broken one, which the target
switches to when it is destroyed. The standing door is 4 faces in one plane (y 7,296, x -256..256, z 192..832; a
2-face LOD past 275), all one-sided and facing out, with no back faces. The emitter (`sub_800CDC54`) culls a face whose
control word (primitive +12) has bit 31 clear when it faces away (`NCLIP`). So from inside the door was culled: the
avenue showed through, and the door's hit volume still stopped the car until it was shot.

**The other breakable walls a car can be behind** (every level file's targets 500..520, their standing state's
faces checked for a back face or a two-sided flag):

| Level | Target, select | Standing state | Two-sided now |
|---|---|---|---|
| PARIS, PARIS2 | 503, `8002EEF0` / `8002F0A8` | the church door, 4 + 2 faces | yes |
| ISLANDS (Amazonia) | 504, `8006957C` | the temple's two breakable walls (y 13,450 and 13,800), 2 panels each (section 13) | yes |
| ISLANDS | 504, `800696A8` / `8006B064` | a lone 200 x 200 panel each at (-6,250, -400) and (-6,250, 400) | yes |
| BURB | 502, `8008A158` | a lone 320 x 240 panel at (22,752, 8,352) | yes |

Everything else breakable is a closed block (crates, pillars, the ice towers, the buildings), already has back faces
(the Roof Tops windows: one-sided front and back layers), or is two-sided on the disc (the Los Angeles fences).

**Built.** With `portVisual` bit 6, `portTwoSidedBegin` (called by `renderScene` right after
`portHiddenAreasBegin`) sets bit 31 on those faces' control words, so the emitter draws them from both sides; a
level's group is set only when every one of its words holds its disc value outside bits 13..15, with bit 31 clear
(that file loaded; see 113b below). `portHiddenAreasDrawn`
(after the emitter and the near pass, which read the words) puts each one back. The words are the level file's,
which the rollback hash covers, so they are only ever changed inside one view's drawing, outside the step, as
request 78's vertices are. Draw state only: no hit volume, no target, nothing in the rollback state changes. From
behind, the face shows its texture mirrored.

**Checked:** `check_custom.py` check 25 runs `portTwoSidedBegin` and `portHiddenAreasDrawn` in LinkMips on the disc's
files: exactly the listed words get bit 31 with bit 6 (each a one-sided polygon's control word on the disc), nothing
with bit 6 clear, on another level, or when one word of a group is not the disc's; Drawn gives the file back byte
for byte. **Not tested in game.**

**113b (2026-11-11): the door was still not drawn from inside in the match. Fixed.** The port found the PARIS words
in the match differing from the disc in bits 13..15 (e.g. `8002E6B0`: `15844000` on the disc, `15840000` in the
match), so the first build's exact compare skipped the group. CONFIRMED cause, the game's own: `dbscan`
(`8014ABF8`), which walks the level when it loads, calls `meshSetPrimQuadrants` (`8014D4CC`) on every mesh, and that
rewrites each primitive's bits 13..15 (byte 13 bits 5..7) with the quadrant of its first vertex (reversed order for
primitives 2, 4 and 6). Recomputing it from the disc gives exactly the port's six PARIS values; the ISLANDS and BURB
words change the same way (e.g. `80069450`: `15842000` to `15846000`; `80089E3C`: `15844000` to `15840000`).

- `portTwoSidedBegin` now compares each word without bits 13..15 (`PORT_TWO_SIDED_KEY` `0xFFFF1FFF`; bit 31 kept in
  the compare, so it must be clear) and sets bit 31 on the word as it is, quadrant bits included.
- `portTwoSidedEnd` (in `portHiddenAreasDrawn`) clears bit 31 on a listed word that matches its key with bit 31
  set, which gives back exactly what Begin found.
- Check 25 now also runs the game's `meshSetPrimQuadrants` (in LinkMips) on the meshes holding the listed faces
  before Begin, in each case (bit 6 on, off, one word changed), and fails if that leaves every listed word as on the
  disc. **Not tested in game.**

## 17. Request 119 (2026-11-18 playtest): Cyburbia's tunnels, the far end shut until close (built; `portVisual` bit 6, `TM2_PORT_HIDDEN_AREAS`)

Asked from the user's single-player take (49:28-49:31, Farthest): driving into a highway tunnel, its far opening
shows tunnel or wall instead of the outside until the car is near it. Section 3's survey was wrong about this zone.

**The zone (CONFIRMED, `BURB.DMD`).** `8008B580` (opcode 11) has one 5-plane region, `8008B53C`: x -400..29,200,
y -1,200..15,600, z under 1,600.
- **Eye in the region:** the walk draws the suburb only, `8008B51C`. That is two `[0, 1800)` halves (`80073538`,
  `8008A468`) and the hit volumes.
- **Eye outside it:** the walk draws the ring highway around the arena only, `8003EBB4`.
  - Six LODs: the corners `[0, 1800)` and the middle pieces `8003E56C` / `8003EB9C` `[0, 1400)`.
  - It holds the roofed corners, the north and south ring tunnels with their portal facades, and the pillars.
- **No shared surface:** no surface of the suburb lies outside the region, and no stand-in or cap exists on either side.
- **The four curved ramp tunnels** that join the suburb to the ring are drawn always, outside the zone (`800921D8`).
  - Each forks at a suburb-side mouth: (9,600 / 19,200, 14,400) in the north, y 0 in the south.
  - It meets the ring near (8,424 / 10,776, 15,965), or y -1,565 in the south.
  - The region's edge (y 15,600 / -1,200) runs inside them, about 45 units before the ring end.
- **So the swap was seen:**
  - From in a tunnel, the ring past its far opening was missing until the eye crossed that line; only the tunnel and
    the short always-drawn ring piece between the branches showed.
  - From the ring, the suburb was never drawn, through the tunnels or over the ring's walls.
- **The bands at the mouths:**
  - The tunnel sections' coarse records are the same open tube with fewer segments; none is capped or closed.
  - At the port's Farthest scales no band in or at the mouths ends short.

**The patch (bit 6, `portHiddenBurb`, after the Hong Kong checks; place `TM2_VIS_HIDDEN_BURB` 4).**
- **Checked first:** the zone, its two children (both opcode 1) and its region word.
- **Then:** both child words point to `portHiddenAreas.group`, a group of the ring and the suburb (unused on BURB),
  so either side is drawn from anywhere. 2 words, put back by `portHiddenAreasEnd`.
- **Nothing drawn twice:** no mesh is shared at the same place and no polygon is coplanar with an overlapping one of
  the other side. Five instanced props are reached from both, always at different positions; their animation steps
  once per frame.
- **Drawing only:** the hit volumes are untouched.
- **Checked:** `check_custom.py` check 12's Cyburbia part.
  - Retail's walk is the suburb in the region and the ring out of it.
  - With the bit, the walk reaches both, near and far, eye in or out.
  - End gives the file back, and a changed zone patches nothing.

**Cost (a model of the walk, Farthest, the bands unbounded).** Polygons before the frustum, and in a rough 100°
view:

| Eye | Retail, before the frustum | Patched, before the frustum | Retail -> patched, 100° view |
|---|---|---|---|
| In a ramp tunnel, inside the region | 7,814 | 10,618 | ~200 -> ~580 |
| In a ramp tunnel, past the edge | 3,796 | 10,598 | 576 -> 601 |
| On the north ring, looking south over the suburb (the worst) | ~3,700 | ~10,450 | ~1,500 -> ~5,190 |

- **Meshes:** at most about 1,050, under the 2,048 cap.
- **Packets:** the ring view's packet bytes (the 390,320-byte cap) are the risk: watch `portRenderStats` there.
- **Depth:** it relies on the port's depth test, since each side is now behind the other's walls.

**Test spot (position units, z up; the four forks are mirrors).**
- **Into the tunnel:** eye (9,200, 15,124, 136), in the west branch of the north-west ramp tunnel, looking toward the
  ring-side opening (8,771, 15,651). The rest of the ring now shows past the opening.
- **The reverse:** eye (8,771, 15,700, 136), looking back to the fork (9,600, 14,400). The suburb now shows through
  it.
- **Which tunnel (LIKELY):** these ramp tunnels are the ones meant. The north and south ring tunnels (x
  11,520..17,280) lie wholly in the ring: from them only the suburb beyond the ring's walls was missing, which the
  same patch fixes.

## 18. Request 129 (2026-11-27 playtest): Suicide Swamp, parts that pop in (built; `portVisual` bit 6, `TM2_PORT_HIDDEN_AREAS`)

Asked from a player's report: driving Swamp, parts of the arena only appear once the eye is in the right column of
space. Section 3's reading (visibility cells, not interiors) holds; this section adds what each zone holds and how
bit 6 now draws them all.

**The zones (CONFIRMED, `SWAMP.DMD`, the level model root `800670A4`).** Five "show" zones and one "hide" zone, all
flag 0:

| Zone | Its child (walked with the eye in a region) | Outside child | Regions (columns, position units) |
|---|---|---|---|
| `8002F23C` | `8002F104`, 97 meshes, 1,074 primitives (every record) | empty group `80027328` | 4: around x -5,100..5,400, y -5,000..900, z under 1,600 |
| `8003B2EC` | `8003B280`, 99 / 1,417 (holds the roofed shelter `8003A680`) | empty `8002F25C` | 1: x -5,800..1,700, y -4,600..2,800, z under 1,600 |
| `80042568` | `800424FC`, 49 / 702 | empty `8003B300` | 1: x -5,200..-2,400, y 2,000..6,000, z under 3,200 |
| `80045974` | `800458C8`, 73 / 834 | empty `80042680` | 2: x -2,100..9,300, y -4,800..1,100, z under 3,200 |
| `8005D854` | `8005D7EC`, 212 / 3,609 | empty `8004598C` | 1: x -3,500..10,500, y -4,800..-1,400, z under 5,760 |
| `8002AD54` (hide, in `8002F104`) | empty `8002ACF8` | `8002ACD8`: 2 meshes, 16 quads and 5 beams | 1: x 100..3,000, y -4,500..-1,600, z under 3,200 |

- **The cells** (the five children) are the arena's big parts. Outside its columns a cell draws nothing, so it popped
  in when the eye crossed into one. The regions overlap (from most of the arena the eye is in two or three), and the
  arena's drivable ground lies in their union.
- **The hide zone's 16 polygons are real geometry (CONFIRMED):** a stretch of the ground's edge skirt, walls from
  z 0 down to -400 under the ground's edge, with ground on one side only. They are the same kind of skirt that every
  cell draws always (`80029E50`, `8002A14C`, `8002A748`, `800369B0`, `8003F204`, ...). Retail leaves this stretch out
  while the eye is near it, LIKELY because the PS1's one-slot-per-polygon sort drew the walls below the ground over
  it from close by (section 4). They stand in for nothing: no polygon of the level is coplanar with them. Their own
  bands (`8002AB04` 200..500 world units, `8002ACC0` 0..600) are kept. Under the port's depth test the parts under
  the ground stay hidden.
- **The shelter's short bands** (`8003A680`, section 3) need nothing more: its last record is one the port's far LOD
  extends at Farthest (`FAR_LOD.md`), and it lies in cell `8003B2EC`, which is now walked from anywhere.

**The patch (bit 6, `portHiddenSwamp`, after Cyburbia's; place `TM2_VIS_HIDDEN_SWAMP` 5).**
- **Checked first:** each zone (opcode 11, flag 0, its two children, its first region word), its children are groups,
  and the outside groups (the hide zone's inside one) are empty. Another file patches nothing.
- **Then 6 words:** each show zone's outside word points to its cell, and the hide zone's inside word to the skirt.
  So every cell and the whole skirt are walked wherever the eye is. `portHiddenAreasEnd` puts them back.
- **Drawing only:** the hit volumes and the hit walk are untouched (they read the file after End).

**Drawn twice?** No mesh is reached twice. The cells' grounds do overlap a little, and that is retail's own:
- **Where:** each cell's ground runs a little into its neighbour's, coplanar. That gives 993 overlapping pairs of
  polygons between cells (or a cell and the always-drawn parts). Most are not the same texture.
- **Retail draws them too:** every pair's two cells are drawn together where their regions overlap, and 477 of the
  993 pairs are drawn together by retail somewhere at its own bands (a 200-unit grid of eyes).
- **With the bit:** they are drawn together from more places. The port's depth rule for coplanar layers (section 11.2:
  a later polygon passes at `new <= stored + e`, in OT order) settles them as at those overlaps today. If seams
  flicker in the Swamp, that rule is the place to look; the same seams can already be seen today from the spots
  where two cells' regions overlap.

**Cost (a model of the walk: the level root, before the frustum).** Primitives (beams and sprites included) / meshes,
retail -> patched, at retail's bands and at Farthest (the far LOD's extended records unbounded):

| Eye (position units, z the ground + 120) | Retail bands | Farthest |
|---|---|---|
| (3,500, -4,000), the worst at Farthest | 984 / 94 -> 1,014 / 110 | 1,187 / 134 -> 1,493 / 184 |
| (-4,000, 1,500), the worst at retail's bands | 818 / 81 -> 1,058 / 114 | 857 / 89 -> 1,293 / 155 |
| over the whole arena (a 500-unit grid) | a median of +110 primitives, 120 meshes at most | a median of +501, 186 meshes at most |

- **The 2,048-mesh cap:** at most 186 meshes, far under it.
- **The packets:** at most about 1,500 primitives, under 80 KB even all as GT4. The 390,320-byte cap is not at risk.
- **To watch:** `portRenderStats`. Swamp stays one of the lightest arenas.

**Test spots (position units, z up; "look" is the point to face, and atan2 is its angle from +x toward +y).** The
cell named was not drawn from the spot before and is now:

| Cell that popped in | Eye | Look toward (atan2) | Retail -> patched at Farthest |
|---|---|---|---|
| `8002F23C` | (2,400, -3,500, 120) | (0, -1,555) (141°) | 865 -> 1,312 primitives |
| `8003B2EC` (the shelter's cell) | (-3,800, 2,900, 157) | (-4,029, 457) (-95°) | 625 -> 1,142 |
| `80042568` | (-3,600, 1,900, 138) | (-3,801, 4,353) (95°) | 694 -> 1,148 |
| `80045974` | (-800, -1,700, 120) | (1,568, -3,283) (-34°) | 689 -> 1,025 |
| `8005D854` | (-500, -1,250, 120) | (5,502, -3,812) (-23°) | 719 -> 1,023 |

- Each spot lies just outside the named cell's columns (within 300 units of them for the first four), on the ground
  and inside other cells' regions. With bit 6 clear, driving toward the look point shows the cell appear at the edge
  of its column; with the bit it is there from the start.

**Cyburbia (the same report).** Nothing else of it is switched by the eye: its one zone is request 119's, and it has
no short band under other geometry (section 3). With bit 6 on, no Cyburbia cell should pop in. The report's "no
clouds and sky" was found port-side (the Vulkan renderer only).

**Checked:** `tools/check_custom.py` check 12's Swamp part.
- **Retail's walk:** with the eye out of every zone no cell is reached; with the eye in all of them every cell is
  reached but not the skirt.
- **With the bit:** Begin returns 6 and place 5. The walk reaches every cell and the skirt, eye in or out, at 50, 300,
  2,000 and 20,000 world units (the skirt's two meshes at their own bands).
- **End** gives the file back. With a cell's zone or the hide zone changed, nothing is patched.
