# Car select: drawing the 3D car more than once

Answers item 9 of the PC port's `docs/DECOMP_REQUESTS.md` (a split-screen
car select with a live 3D car for each of 2-4 players). Static analysis
only: the C sources and the retail disassembly (`bin/SCUS_94306R.EXE`).
Nothing was run.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = inferred, not seen
running.

## Short answer

- **The draw function** is `sub_80122124` (sceneObjDraw) (`scene_init.c`), the draw
  method of every shell scene object. It builds the camera with `viewProc(0)`,
  then draws one model with `sub_80151378` (renderDrawModel) (`engine_anim_hit_volumes.c`).
- **Calling it again works.** It can be called again in the same frame with
  another car, angle and position. Each call resets the renderer state and
  emits the model straight into the frame's ordering table (OT). Nothing
  waits for the end of the frame.
- **The catch: the car is chosen by a shared byte.** The carousel model
  is a "select" node in `CARSEL.DMD`, and the car is byte +7 of that
  node. The byte lives in the loaded model data, so every object that
  points at the node shows the same car. Set the byte before each draw,
  or point each player's model at the car's own child node.
- **R1 "random car"** is a chain of 5 script commands on the carousel.
  The parameters are in section 5. The port should run this per player
  in its own code: the scene's running-command list holds only 10 entries.

## 1. How a frame of car select is drawn (CONFIRMED)

Car select (`sub_801307EC` (screenCarSelect), `screen_car_select.c`) builds its objects and
then runs the shell scene `D_8016303C` (shellScene) with `sub_80122B34` (sceneRun)
(`screen_ending.cpp`). Each frame of `sub_80122B34` (sceneRun):

1. **`sub_80152194` (renderFlipFrame)** flips the renderer's frame flag `D_80180DB8` (renderFrameParity) and
   stores the ticks per frame.
2. **Flip the frame.** It switches to the other of the two frame records.
3. **`sub_80122790` (sceneRunCommands)** runs the running commands (moves, turns).
4. **`sub_80122818` (sceneDispatchEvents)** sends the events to the action rows.
5. **`sub_80122984`** draws the frame. IDA calls it `screenCarEnding`, which
   is wrong: it draws every shell scene. It works like this:
   - **Clear the OTs:** `ClearOTagR` on the frame's OT and on the second
     OT (4096 entries each). It also resets the packet cursor
     (frame +8 = frame +4).
   - **Draw the objects:** first the objects in list A (scene +96, 16
     slots), then list B (scene +164). Each one gets `object->draw(frame)`,
     vtable entry 1, with the vtable pointer at object +48.
   - **Show the frame:** `DrawSync`, `VSync`, `PutDrawEnv`, `PutDispEnv`,
     the background picture (`sub_8012201C` (sceneDrawBackground)), then `DrawOTag(ot + 4095)`.

**The frame record** is 128 bytes. Car select uses `D_8018118C` (renderView0Frames) and
`D_8018120C` (renderView0FrameB) (`rt_buffer_init.c`):

| Offset | Field |
|---|---|
| +0x00 | OT, 4096 entries (`801E5A8C` and `801E9A8C`) |
| +0x04 | packet buffer start (`801B600C` and `801CDD4C`) |
| +0x08 | packet cursor |
| +0x0C | packet buffer end (start + 0x17CF0) |
| +0x10 | `DRAWENV` |
| +0x6C | `DISPENV` |

The scene's second frame pair (`D_8018128C` (renderView1Frames)/`D_8018130C` (renderView1FrameB)) has its own OTs
(`801EDA8C`, `801F1A8C`). It shares the same two packet buffers.

**The objects in car select.** The carousel (id 9005), the arrows (9004),
the name labels (9006-9009) and the text counter are in list A. That is
8 of its 16 slots. The "select" and "info" items (9001, 9002) are in
list B. Base scene objects come from `sub_80123E98` (`base_object_init.c`).
Their vtable is `D_800CF4C4` (sceneObjVtbl), so their draw method is `sub_80122124` (sceneObjDraw).

## 2. The draw: `sub_80122124(object, frame)` (sceneObjDraw) (CONFIRMED, checked against the disassembly)

**The scene object** (56 bytes; the accessors are in `base_object_accessors.c`):

| Offset | Field | Set by |
|---|---|---|
| +0 | id (u16), e.g. 9005 | `sub_80123F58` (sceneObjBaseInit) |
| +4 | model node from the DMD | `sub_80123F40` (sceneObjSetModel), through `sub_80121E18` (sceneDeliverResource) at load |
| +8 | visible (u16) | `sub_80123E8C` (sceneObjSetVisible) |
| +10 | 1 = 3D with the scene camera, 0 = flat 2D item (u16) | `sub_80123E74` (sceneObjSetUseCamera) |
| +12 | colour/flag word for 2D items (s16, -1 = keep) | `sub_80123E5C` |
| +16 | 2D scale | `sub_80123E3C` (sceneObjSetScale) |
| +20 | engine object (`TM2Obj`, made on first draw) | |
| +24/+28/+32 | position x, y, z | `sub_80123E1C` (sceneObjSetPosition) / `sub_80123DDC` (sceneObjGetPosition) |
| +36/+40/+44 | rotation a, b, c | `sub_80123E08` (sceneObjSetRotation) / `sub_80123DB0` (sceneObjGetRotation) |

**What the draw does** when +4 and +8 are both set:

1. **First draw only:** it takes an engine object from the 128-object pool
   (`sub_8014952C` (csAlloc)). It gives it lighting environment 0 (`lightGetEnv(0)`)
   and the model (`sub_801495A8` (csSetModel), object +0x11C). The model is never set
   again after this.
2. **Copy the placement.** The position goes to engine object +0xA8/+0xAC/+0xB0.
   The rotation goes in as vx = b, vy = c, vz = a (+0xA0/+0xA2/+0xA4).
   Then `RotMatrixYXZ` builds +0x80.
3. **The camera.** When +10 is set (as for the carousel), it calls
   `viewProc(0)`. Otherwise it uses `sub_801566F8` (viewSetFixedCamera), the flat camera for
   2D items with H = 1.
4. **The model.** It calls `sub_80151378(frame, engine object)` (renderDrawModel).

**`sub_80151378` (renderDrawModel)** (`engine_anim_hit_volumes.c`) draws one model completely:

1. **Reset the state.** Far = `D_80180DAC` (renderFarDistanceDefault) (into `D_80180DA8` (renderFarDistance)), ticks
   (`D_80181108` (renderFrameTicks)), then `sub_801521D0` (renderInitState) fills the render state in the
   scratchpad (0x1F800000, `RenderState` in `engine_render_scene.c`). The matrix slot
   count and the mesh count go back to 0.
2. **No per-node culling.** `D_80180DC0` (renderSkipNodeCull) = 1, so state +0x28 = 1.
3. **`csUpdMat`** (`sub_801491E8` (csUpdMat), `engine_clut_anim.c`) does three things:
   - **Visibility:** a sphere test, but only when the model's root node
     is type 1. The carousel's root is type 9, so it is never culled.
   - **Camera space:** it puts the object in camera space. The parent is
     the root object `D_801810D0` (csRootObject), whose matrices `viewProc` has just
     written.
   - **Light matrix:** light matrix × the object's rotation, into matrix
     slot 0 (`D_801F8AB8` (renderObjectMatrices)).
4. **`sub_801526CC` (renderWalkModel)** (hand-written asm; readable copy in
   `reference/renderer_walker_engine_render_scene.c`) walks the model tree. It puts
   up to 280 mesh entries in `D_801AC810` (renderMeshList) and matrix slots in
   `D_801AEB18` (renderMatrixSlots)/`D_801F8AB8` (renderObjectMatrices) (cap 198).
5. **Draw modes.** `SetDrawMode` packets go into scratchpad +0x60/+0x6C,
   built from the frame's `DRAWENV` (tpage, dtd, dfe, texture window).
6. **`sub_800CDC54`** (asm; readable copy in
   `reference/model_emitter_800CDC54.c`) projects and lights the meshes. It
   links packets into frame->ot (+0), writes them at frame->cur (+8) and
   stops at frame->end (+0xC). At the end it stores the cursor back.
7. **`sub_80154690` (renderNearPolys)** splits the near polygons. It works on the screen
   coordinates that are already projected.

**Where each input comes from:**

| Input | Source |
|---|---|
| **Car model** | The carousel's +4 is a **type 9 "select" node** in `CARSEL.DMD` with id 9005 at node +2. +6 = number of children (14), +7 = the chosen child, +16 = the table of child pointers. The walker (opcode 9) draws child `node[7]` only. `sub_80122078` (sceneObjSetChoiceState) / `sub_801220B0` (sceneObjGetChoiceState) write / read +7, the car-select index (0..13). |
| **Position, rotation** | Scene object +24..+44, as above. Car select sets the carousel to (0, -960, -40), rotation (0, 0, 0). |
| **Camera** | View 0. `sub_801222F0(scene, ..., 0)` (sceneInit) creates it in view mode 1 (320x240, H 228; see `WIDESCREEN.md`). Car select puts the camera at position (0, -1200, 80), rotation (170, 0, 0) (`sub_80137F34` (sceneSetCamera)). `sub_80122B34` (sceneRun) copies that into view 0 once per scene run (`sub_80156528` (viewSetPosition), `sub_801564D8` (viewSetRotation)). `viewProc(0)` rebuilds the matrices **and** H, OFX = 160, OFY = 120 for every 3D object drawn. |
| **Lights** | Environment 0: 3 directional lights and the ambient (`sub_801538A8` (lightSetEnvColor), `sub_801536F4` (lightSetEnvDirection), `sub_80153964` (lightSetEnvAmbient), set by car select). They are fixed in the world. `csUpdMat` turns them into the object's frame on every draw. |
| **OT and packets** | The frame record passed in (section 1). |

**How the model gets bound** (CONFIRMED):

- `sub_80122750` (sceneLoadTmsDmd) → `sub_80129010` (shellLoadTmsDmd) loads `CARSEL.TMS`/`.DMD`.
- `sub_8014ABF8` (dbscanHierarchy) then calls `sub_800D1DB0` (worldLoadObject) for each node.
- Ids of 9000 and up go to `sub_80121E18` (sceneDeliverResource) (`screen_resource_records.c`).
  It gives the node to **every** object in the scene's three lists with
  the same id. It does not stop at the first match.

## 3. Can the draw run again in the same frame? Yes (CONFIRMED)

Each `sub_80122124` (sceneObjDraw) call does everything in one go. Nothing it builds is
kept for later in the frame: the scratchpad state, the mesh list and the
matrix slots are rebuilt at the start of each call and used up before it
returns. So the port can draw N cars by calling it N times between steps
4 and 5 of section 1, or from an extra object's draw method.

**Per-player car (the shared +7 byte).** All carousel objects point at
the same type 9 node. There are three ways round this:

- **(a) Set the byte before each draw:** `node[7] = car[p]`
  (`sub_80122078(obj, car)` (sceneObjSetChoiceState)). This is safe because the tree walk happens
  inside the call.
- **(b) Use the car's own node:** give each player's engine object the
  child node directly, `((u8 **)(node + 16))[car]`, in engine object +0x11C.
  Write it after the first draw has made the object, or make the object
  yourself. That child's root is probably type 1 (LIKELY), so the
  object's sphere test then applies. It uses the view-mode-1 planes.
- **(c) Do not let script commands change the byte.** `TM2FourWay`
  (`sub_8012305C` (sceneCmdChoiceStepRun)) changes +7 itself, so with a shared node the players'
  step commands would fight. Keep each player's car index in port state
  and use (a) or (b).

**Placing each car on screen.** There are three options:

- **World offset (simplest).** Move each car's position (+24/+32) in
  the one camera's view. Perspective changes a little with the offset.
- **Projection centre.** `viewProc(0)` resets OFX/OFY to the view centre
  inside `sub_80122124` (sceneObjDraw), so calling `SetGeomOffset` before the draw has
  no effect. Copy `sub_80122124` (sceneObjDraw) (about 30 lines) and call
  `SetGeomOffset(cx, cy)` after `viewProc(0)`, before `sub_80151378` (renderDrawModel).
  Nothing in `sub_80151378` (renderDrawModel), the emitter or the near pass writes
  OFX/OFY. The emitter only reads H for sprites.
- **A real viewport per player.** There is no per-object clip. The frame
  has one `DRAWENV` (320x240) and one depth-sorted OT, so `DR_AREA`
  packets cannot fence off one car's polygons. A port renderer can
  scissor per draw instead. The scene has a second OT (`other`), drawn
  before the main one when scene +3432 is set, but that gives only 2.

**State a call changes, and whether it matters:**

| State | Changed by | Save/restore? |
|---|---|---|
| Scratchpad 0x1F800000 (render state, saved registers, near list) | `sub_801521D0` (renderInitState), walker, emitter, near pass | No: reset on every call. Matters only if the port's own code keeps data there. |
| GTE rotation, translation, light, colour and back-colour registers | `viewProc`, `csUpdMat`, emitter | No: every 3D and 2D drawer loads its own. |
| GTE H, OFX, OFY | `viewProc` (row values), `sub_801566F8` (viewSetFixedCamera) (H = 1) | Only if the port changes them. Each later object resets them. |
| Root object `D_801810D0` (csRootObject): eye, view, plane matrices; `D_801A51A8[0]` (viewEyePositions), `D_801A51C0[0]` (viewMatrices) | `viewProc(0)` | No: rebuilt from the same view-0 camera on each call. |
| View object `D_80181110[0]` (viewCameraObjects) and its parent: +0x80 | `viewProc(0)` | No. |
| `D_801AC810` (renderMeshList) (meshes, cap 280), `D_801AEB18` (renderMatrixSlots)/`D_801F8AB8` (renderObjectMatrices) (matrix slots, cap 198/200), `D_801ABA50` (renderSlotMatrixPtrs), `D_801B2E0C` (renderFrustumMatricesA), `D_801B470C` (renderFrustumMatricesB), `D_801F5A90` (renderSharedWorkBuffer) | walker and `csUpdMat` | No: used up inside the call. |
| `D_80180DA8` (renderFarDistance), `D_80181108` (renderFrameTicks), `D_80181104` (renderCurrentOt), `D_80180DC0` (renderSkipNodeCull), state +0x28 | `sub_80151378` (renderDrawModel) | No: set again on each call. |
| Frame packet cursor (+8) | emitter, near pass | Do **not** restore: it must move on. All draws in a frame share one 0x17CF0-byte buffer. When it is full, the emitter silently stops. |
| Frame OT entries | emitter | No: the cars are sorted together by depth. They overlap only if the screen places overlap. |
| Model node byte +7 (DMD data) | `sub_80122078` (sceneObjSetChoiceState), `TM2FourWay` | Yes, see above. Set it per player before each draw. |
| Engine object (+0x80..+0xB0, +0x14 world pos, +0x20 matrix, +0x124 visible) | `sub_80122124` (sceneObjDraw), `csUpdMat` | No: use one scene object (one engine object) per player, or rewrite them before each draw. |
| Animated texture nodes (type 3) in the car models | `sub_801518E8` (animUpdateOne) | No: they advance only once per frame (`done[D_80180DB8]` (renderFrameParity)), however many times they are drawn. |
| Object pool (128), `D_801810DC` (csPoolCount) | first draw of each scene object | 1 more object per extra player. The pool is reset per scene by `sub_801495C0` (csPoolInit). |

**Limits to watch for 4 cars (LIKELY):**

- **Packet buffer:** 4 cars use about 4 times the packet space of one.
  Car polygon counts are model data and were not measured. The PC
  renderer can use a bigger buffer.
- **Scene lists:** list A has 8 free slots of 16. 3 more carousels fit,
  but 3 more carousels plus 2 labels each do not.

## 4. Script commands: names vs. what they do (CONFIRMED)

The class names are the decompilation's own (`object_control_helpers.cpp`).
They were first given the wrong way round. Their vtables and run functions
show which one turns and which one moves, and the source now uses the
corrected names: **TM2TurnBy** turns and **TM2MoveBy** moves. Each run is
one scene frame. Run functions are in `motion_commands.c`.

| Constructor | Name | Run (vtable) | What one run does |
|---|---|---|---|
| `sub_801239CC` | TM2TurnBy | `sub_80122DD8` (sceneCmdTurnByRun) (`D_800CF404`) | rotation (a,b,c) += (x,y,z), each % 4096 |
| `sub_80123A20` | TM2MoveBy | `sub_80122CE8` (sceneCmdMoveByRun) (`D_800CF414`) | position (x,y,z) += (x,y,z) |
| `sub_80123A74` | TM2TurnTo | `sub_80122CB4` (sceneCmdTurnToRun) (`D_800CF424`) | rotation = (x,y,z), at once |
| `sub_80123AC8` | TM2MoveTo | `sub_80122C80` (sceneCmdMoveToRun) (`D_800CF434`) | position = (x,y,z), at once |
| `sub_80123B64` | TM2FourWay | `sub_8012305C` (sceneCmdChoiceStepRun) (`D_800CF464`) | choose a child of a type 9 node, shown after this table |

**Timed commands (the "By" pair):**

- A command with N frames applies its step N times.
- On the next run it posts event 2 with itself as the source
  (`sub_801226FC` (scenePostEvent)), takes itself off the running list and reloads N.
- N = -1 means it runs forever.

**TM2FourWay(target, mode, value, accept):**

- **Mode 0:** step by `value`, skipping cars that `accept` rejects.
- **Mode 1:** set the choice to `value`.
- **Mode 2:** random. `+7 = (+7 + rand()) % count`, rolled again while
  `accept` rejects it. Car select passes `sub_8012CC10` (carIsSelectable) (Sweet Tooth and
  Minion locks, cars already taken; see `UNLOCKS.md`).

**Action rows** are `sub_80122574(scene, event, source, kind, command)` (sceneAddAction).
They match on (event, source); a source of 0 means any. Kind 0 adds the
command to the running list (10 slots, `sub_801225F0` (sceneStartCommand)). Kind 1 removes
it. Kind 2 runs it once now.

**Angles.** Rotation a is applied as vz, so it is the turn about the
vertical axis: the car's heading on the turntable. Rotation b is vx,
the pitch. z is up (screen y = OFY - H·z/depth). The camera looks along +y.

## 5. The R1 random-car animation (CONFIRMED rows; the look is LIKELY)

Menu event 11 is R1 (default binding). **Idle state:** the carousel sits
at (0, -960, -40). The spin command `o1176` (TM2TurnBy carousel, 20, 0, 0,
-1) is running: its heading turns +20 per frame (4096 = one full turn,
about 205 frames).

**Step 1, R1 pressed** (event 11, any source):

| Kind | Command | Effect |
|---|---|---|
| run | `o1704` = TM2MenuCallback(counter, carousel, `sub_8012DD38` (menuRandomCarText)) | the text shows "Getting Random Car" |
| run | `o1768` | shell sound 28 |
| remove | code detector | stops the cheat-code reader |
| remove | `o1176` | stops the spin |
| add | `o1112`, `o1144` = TM2TurnBy(arrow, 0, 256, 0, 16) | both arrows flip once (256 × 16 = 4096) about their x axis |
| add | `o1496` = TM2MoveBy(carousel, 0, 0, -320, 10) | the car drops 3200 units in 10 frames, out of view |

**Step 2, when `o1496` ends** (event 2, source `o1496`):

| Kind | Command | Effect |
|---|---|---|
| run | `o1592` = TM2FourWay(carousel, 2, 0, `sub_8012CC10` (carIsSelectable)) | picks a random allowed car |
| run | `o1624`, `o1648` | the name labels 9006/9007 copy the carousel's index into their own type 9 nodes (`sub_8013724C` (menuCopyChoiceRun)) |
| run | `o1272` = TM2MoveTo(carousel, 0, -640, -120) | puts the car further back (320 units) and lower (80) |
| run | `o1304` = TM2TurnTo(carousel, 2048, 0, 0) | heading 2048 (half a turn) |
| add | `o1464` = TM2MoveBy(carousel, 0, -32, 8, 10) | the car moves (0, -32, +8) per frame for 10 frames, back to (0, -960, -40): it comes toward the camera and rises |

**Step 3, when `o1464` ends** (event 2, source `o1464`):

| Kind | Command | Effect |
|---|---|---|
| add | `o1176` | the spin starts again, from heading 2048 |
| add | code detector | cheat codes are read again |
| run | `o1672` = TM2MenuCallback(counter, carousel, `sub_8012DD0C` (menuCarNameText)) | the text shows the new car's name |

**Relative to the camera.** The camera eye is (0, -1200, 80), so the car
goes from 560 ahead and 200 below to 240 ahead and 120 below.

**Per player in the port:** the animation is 11 + 11 scene frames:
each timed command applies N steps, and its end event comes on the next
run. Per player it needs:

```
on R1:     stop spin; flip that player's arrows; dz = -320 for 10 frames
then:      car = random allowed car (separate random stream: rand_callers.tsv
           marks 8012305C GAME_STATE only because it shares rand())
           pos = (0, -640, -120) + player offset; heading = 2048
           pos += (0, -32, 8) for 10 frames
then:      spin heading += 20 per frame; show the car's name
```

**For comparison, Left and Right** (events 9/10) use the same pattern:

- **Slide out:** the car moves ±32 in x for 10 frames (`o1432`/`o1368`).
- **Step:** it steps the car (`o1560`/`o1528`, TM2FourWay mode 0, -1/+1).
- **Jump across:** it jumps to x = ∓320 (`o1208`/`o1240`).
- **Slide in:** it slides back ±32 × 10 (`o1400`/`o1336`).

The arrow flip is `o1112` for Left and `o1144` for Right.

**Why not copy the rows per player.** The scene's running list holds 10
commands. One player's R1 already uses 3, plus the code detector. The
action table holds 128 rows and car select uses 86. Four players' copies
would not fit. The port should run these steps in its own code and
then draw each car as in section 3.
