"""Static checks of PC port request 42 (custom tournament and mods, docs/CUSTOM_TOURNAMENT_ANSWERS.md).

    py -3.14 tools/check_custom.py work/pv/P4 [--expect-functions] [--e0 E0] [--negative-control]

On a port variant link (build_source_only.py or build.py output directory) the checks of the defines it was built
with:

1. The ordinary Dark Tooth (TM2_PORT_DARK_TOOTH, portCarOptions bit 10, item 1):
   - its port functions (DT_ORD_PORT) are in .port_text and reach no rand() (call graph of R_MIPS_26 relocations);
   - the gate: portCarDarkToothOrdinary returns 0 whenever bit 10 is clear (a walk of its code from the entry: the
     branch on `portCarOptions & 0x400` (lw of portCarOptions, andi 0x400) has a bit-clear edge that reaches
     `jr ra` with $v0 = 0 and no call), so every game test of it is retail's (or bit 6's) logic with the bit clear;
   - its callers are exactly DT_ORD_CALLERS (the gated game functions and the Dark Tooth port functions), the head
     helper's only caller is aiWeaponFireDecide (inside the Dark Tooth's selection), portCarDarkToothRole (the
     port's API) has no caller in the C, and none of them is pointed to.
2. The visual switches (TM2_PORT_VISUAL, portVisual, items 6 and 7):
   - portVisualArcOtz is in .port_text and calls nothing; its only caller is weaponArcsDraw;
   - request 76: portVisualTaserFrom calls only mathMulTransVec and only weaponArcsDraw calls it; weaponArcsDraw run
     on a taser arc (kind 0) whose shooter is turned (a body matrix of a quarter turn): with bit 1 its first segment
     starts at the projection of the light bar (from + body_m (0, TM2_VIS_TASER_SIREN_Y, TM2_VIS_TASER_SIREN_Z)), with
     the bit clear at retail's (from + 30 up); a lightning arc (kind 1) starts at retail's either way;
   - weaponArcsDraw calls the same retail functions as E0's (the retail link) from as many call sites (rand() 8: the
     arc's end and jitter and its sparks; request 88: the jitter is retail's with bit 1 too), so the rand() stream
     is retail's with any bit;
   - bit 0's gate: with every branch on `portVisual & 1` cut at its bit-set edge (an andi 1 of a register loaded
     from portVisual, then beq / bne against zero), no portVisualArcOtz call is reachable from the entry, and the
     AddPrim call is (retail's entry 0 path);
   - the source routes the lift, the jitter and the OT entry through ARC_LIFT / ARC_JIT / ARC_OT, whose port
     definitions test TM2_VIS_TASER_LOW / TM2_VIS_ARC_DEPTH, and no bare `+ 30` or `AddPrim(ot,` is left.
   - request 45 bit 2 (TM2_VIS_OWN_CAR): portVisualOwnCar calls only viewGetMode (and portLocalHuman with
     TM2_PORT_HUMANS), its only caller is renderScene, which calls it once, before the level root's walk (its call
     precedes every renderWalkModel call), and whose object loop tests RENDER_OBJ_WALKED (the source; the
     #else form is retail's `*(s32 *)(obj + 8)`); between the own car's walk and the level root's, renderScene steps
     the object counter (the walker's translation key, ctx+0x34) once more, through the same pointer register as the
     step before the car's walk (HIDDEN_AREAS.md 10: else the level's top-level meshes can share the car's last key
     and the emitter draws them at the car's translation; a link before it, e.g. work/pv/r63_port_P4, fails).
   - request 46 bits 3 and 4 (PLAYTEST_ANSWERS.md 11, 12): the port functions VISUAL_PORT_46 are in .port_text, reach
     no rand() and are called only by VISUAL_CALLERS_46 (renderScene: portVisualWalked after each of its three
     walks of world models and objects, never the level root's or the sky's, and portVisualShadows once, after every
     renderWalkModel and before the emitter call; bulletSpawn: portVisualBulletSpawn; streaksDraw:
     portVisualBulletTail); bulletSpawn and streaksDraw call the retail functions as E0's do (no rand() added); the
     source: the shadows' override pairs are written only under `portVisual & TM2_VIS_SHADOW`, the streak's tail
     returns the record's own point first when `!(portVisual & TM2_VIS_GUN_START)`, and both macros' #else forms
     are retail's (`renderWalkModel(...)`, `e->p1`).
   - request 79 (PICKUP_LOOKS.md 10): the special's pickup look: PICKUP_SPECIAL_PORT in .port_text, no rand(), called
     only by PICKUP_SPECIAL_CALLERS (worldLoadStartObjects: portPickupSpecialBuild once, after its last
     dbscanHierarchy call); in LinkMips, for each of the 14 arena files at their own address with pickupObjects
     filled from the file's pickup nodes and portPickupSpecial.hud = its HUD object (1260): the model is a type-1
     group of 2 (the card: a one-primitive beam mesh, half 32 x 32, whose texture words are the HUD's selected icon
     0's texel box, CLUT and page, v0 top-left; the donor's hit box; request 83: no blinking name), the donor a
     weapon pickup's model, hud cleared; with no HUD object, or no weapon pickup, the model is 0; the level file is
     not changed.
   - request 81 (PICKUP_LOOKS.md 11): ICON_PORT in .port_text, no rand(), portWeaponIconMesh called only by
     portWeaponIcon and portWeaponIcon by nothing (the port's); in LinkMips on the 14 arena files with the mine pool's
     object 0 holding asset 2212 and weaponFreezeMissileModel = asset 2202: the mine's icon is its one sprite's page,
     CLUT, texel box and colour, the freeze's the largest sprite of animation frame f (0, 5, and 16 = 0); with no
     mine object / no freeze model it returns 0.
3. Car numbers (TM2_PORT_CAR_NUMBERS, item 3) and the extended car select (item 5, also TM2_PORT_DT_PLAYER):
   - the port functions (NUMBER_PORT, CARSEL_PORT) are in .port_text and call no rand() themselves (none reaches it
     but portCarEnding, which plays a movie);
   - each is called exactly by NUMBER_CALLERS (sessionLoadArenaAndCars and portHumanCompact only with
     TM2_PORT_CARS / TM2_PORT_CAR_DROPIN) and none is pointed to;
   - portCarSelExtend copies three nodes (portCarSelCopy called 3 times: the carousel and both name labels), and so
     does portAiSelExtend (item 2: Challenge's picker with Sweet Tooth, portCarOptions bit 11);
   - the storage (portSettingsHumanCar 32, portSettingsOpponentCar 32, portModCars 4096, portCarNumbers 208,
     portCarSel 2016, portVisual 4, portCarDtBossMask 8 bytes) is in the link at those sizes.
4. With --expect-functions: the game functions the port's defines changed (report.json port_changed_sections, code)
   include those of the request 42 defines (EXPECTED_VISUAL, EXPECTED_NUMBERS (+ EXPECTED_NUMBERS_CARS with
   TM2_PORT_CARS)) and request 70's (EXPECTED_PROFILE with TM2_PORT_CAR_PROFILE); with only one of them as the port's define they are exactly its list.
5. Request 46's render switches (PLAYTEST_ANSWERS.md 13, 15):
   - TM2_PORT_FAR_LOD (portFarLodNearScale, FAR_LOD.md 8): renderScene reads portFarLodNearScale (beside portFarLod),
     no other game function names it or portFarLodNear; portFarLodBegin and portFarLodEnd (and the scan's
     portFarLodScanLevel) read portFarLodNear, call no rand();
   - TM2_PORT_FAR_DISTANCE (portMirrorFar, FAR_DISTANCE.md 5): renderScene calls portFarDistanceMirror twice (the
     mirror's level and object far distances), its only caller; it calls nothing; no game function names portMirrorFar;
     the source's #else form of RENDER_FAR_MIRROR is retail's constant;
   - TM2_PORT_FAR_DISTANCE (the render capacity counter portRenderStats, RENDER_ANSWERS.md "Measuring the render
     caps"): portRenderStatsWalked and portRenderStatsDrawn are in .port_text, reach no rand(), are called only by
     renderScene, once each (Walked after every renderWalkModel and before the emitter call portFarDistanceEmit,
     Drawn after renderNearPolys); Walked calls only viewGetMode, Drawn nothing; every store of both goes to
     portRenderStats (a must-taint walk of each function's flow graph: a store's base register must hold an address
     formed from portRenderStats, or be $sp) and no game function names portRenderStats.
6. Request 47 (FAR_DISTANCE.md 3, render_slots.h):
   - the packet buffers (every variant; request 47 revised): portPacketBuffers is a 4 x buffer per view (views 1-4
     and the mirror) and parity, 5 x 2 x 390,400 = 3,904,000 bytes; with TM2_PORT_FAR_DISTANCE the four functions
     PACKET_PORT are in .port_text, reach no rand() and are called exactly by PACKET_CALLERS (portPacketBuffersApplyView
     by portPacketBuffersApply and portPacketBuffersApplySet, portPacketBuffersApply by rtMainLoop and ApplySet,
     ApplySet by nobody: the port's, portPacketBufferCursor by rtMainLoop, twice: view 2's and the mirror's cursor);
     portPacketBuffersApplyView is the only function that names portPacketBuffers; the source's #else form of
     RT_PACKET_CURSOR is retail's `((before)->field_8)` and rtMainLoop uses it twice;
   - portFarState is 16 + 8 (N + 1) + 64 (N + 312) bytes for N = TM2_MAX_RENDER_SLOTS (34,392 at retail's 200);
   - TM2_PORT_RENDER_SLOTS (the matrix slot limit L = N - 1): the walker is the C one (renderWalkModel,
     renderPushMatrix, renderMulTransMatVec changed, with renderInitNodeHandlers, whose every relocation names
     renderWalkModel's entry: no asm label is left), renderWalkModel compares with L - 2 and never with retail's 198,
     renderScene compares the slot counter with L (three times, five with TM2_PORT_VISUAL) and never with 200; the
     five slot arrays are in .port_bss with N slots each and no relocation of the link still points into their retail
     storage; the emitter (sub_800CDC54) and the near pass (renderNearPolys) are retail's (not changed); with
     TM2_PORT_FAR_DISTANCE portRenderStatsWalked's cap bits compare with L - 2 and L; with --expect-functions the
     functions the define changes are exactly RENDER_SLOTS_CHANGED when it is the only port define, else include them.

7. Request 61 (FAR_DISTANCE.md 2, with TM2_PORT_FAR_DISTANCE): portFarDistanceEmit runs in a MIPS interpreter
   (check_rollback's MiniMips with div, across the pass's helpers; the emitter sub_800CDC54 a stub that records its
   arguments) on a made-up queue of 20 entries (random rotations, meshes 5,000 to 24,000 world units away, a near one):
   - every scaled entry's vertices, projected the emitter's way (its split translation, RTPS) with the matrix and
     translation the pass hands it, lie within 0.1 px of the exact projection (the translation corrected for the
     scaled matrix's rounding; the pass before request 61 is off by up to about 1 px at k = 3);
   - the scaled matrix is round(R / 2^k), the key bit 30 | k << 28 | the entry's index, the near entry is untouched;
   - a mesh with sprites and beams is drawn from a copy (the node's first 44 bytes, its primitives) whose sprite half
     sizes and beam half widths are round(w / 2^k) (a beam at least 1), the beam's half length and the triangles as
     they were, the source untouched; a mesh without them from its node; the mesh drawn twice at one k shares a copy,
     at another k gets its own;
   - the pass writes only portFarState, portFarSprites, the queue and its stack; no function but portFarDistanceScale
     and portFarSpriteMesh (and since the addendum portFarSpriteSource, which reads it) names portFarSprites.
   A link of the pass before request 61 (e.g. work/pv/r60_port_P4) fails check 7 (the negative control).
   Since the request 61 addendum the copy's head is 48 bytes: the node's first 44, then (+44) the source's primitives.
8. Request 61 addendum (PLAYTEST_ANSWERS.md 4.3 item 1, FAR_DISTANCE.md 2 "Precise corners", with
   TM2_PORT_C_EMITTER; request 82, PLAYTEST_ANSWERS.md 27: also the C near pass, check_near_pass: against E0's asm on
   made-up near lists of 5..120 entries, the same bytes; with portPreciseEmit and portNearDepth, the integer bytes
   unchanged and the ring exactly near_expect's entries): the variant's emitter is the C one (reference/model_emitter_800CDC54.c), and it runs against
   the asm (E0's sub_800CDC54) on a MIPS interpreter with a GTE model (MiniMips with div, cop2, lwc2 / swc2, the link's
   own ratan2 / rsin / rcos and tables; the GTE's RTPS, RTPT, NCLIP, MVMVA and NCCS integer exact, the UNR divide) on
   8 made-up views: disc meshes (PARIS, FREEWAY, one level a view) and made-up beam, sprite, flat and textured
   meshes, near and far, mirrored, fixed-OTZ and OTZ-override entries, shared keys and slots, a slot change under a
   cached translation, the cull flip, a full packet buffer, and with TM2_PORT_FAR_DISTANCE six far copies (48-byte
   head, sizes round(w / 2^k), key bit 30 | k << 28):
   - every byte both write (packets, OT links, the near list and its overflow, the scratchpad state, the context) is
     the same, but the asm's register save area (0x1F800000..2B) and spills (0x8C..0x9F) and the C's stack;
   - with portPreciseEmit 4096 and 3072 the integer bytes are those of the run without it, and the precise ring holds
     four corners (one packet's) per drawn beam and sprite, each group at an exact beam / sprite (its ends or centre
     within 0.2 px, plus 3 x the distance from the centre / (512 Z) near the eye), its beam side step and its half
     sizes within 3 % + 1/8 px of w H / (Z 2^k) (k from the key, the far copy's source sizes; a beam's direction from
     the float ends, in the display's shape; the x sizes times the factor), at least 200 groups; nothing in the ring
     with portPreciseEmit 0.
   A link of the C emitter before the addendum's fixes (its t8 at a slot change and a dropped beam, e.g.
   work/pv/r61b_ce_P4) fails check 8 (the negative control); a link without the define fails it (the asm).

19. Request 102 item 2 (RENDER_ANSWERS.md R10, with TM2_PORT_VISUAL, portVisual bit 10): portVisualShadows in
   LinkMips on made-up draw entries (meshes of quads and triangles, an identity rotation slot): with bit 10 every
   shadow entry gets the override pair {its nearest corner's OT entry - TM2_VIS_SHADOW_LIFT (at least 1), the light
   record's second word}, its car walked or not, over bit 3; bit 3 alone keeps request 46's rule (one farther than the
   car's farthest, or the shadow's own farthest); neither: no entry written; the ranges cleared every time.
18. Request 98 (FAR_LOD.md 14, with TM2_PORT_FAR_LOD and TM2_PORT_FAR_DISTANCE): portFarLodBudgetView in LinkMips
   on made-up render stats: each view's band scale steps up with room, down when 7/8 full, with its holds, between
   portFarLodNearScale and portFarLodBudget; portFarLodBegin scales a record by it.
17. Request 99 (RENDER_ANSWERS.md R9, with TM2_PORT_VISUAL, portVisual bit 9): portVisualMirrorEye / Skip in
   LinkMips on a made-up tilted, turned human car: the mirror's eye on its roof looking back, the car left out of the
   mirror's walk; nothing without the bit or outside car views 2 and 3.
16. Request 93 (RENDER_ANSWERS.md R8, with TM2_PORT_VISUAL, portVisual bit 8): portVisualNearOtz in LinkMips (0 with
   the bit, else the retail threshold), then the link's emitter on made-up views with near polygons at threshold 70 and
   0: at 0 nothing deferred, every polygon the 70 run deferred linked whole at its near entry's OT index; with E0 the
   asm at 0 the same bytes.
15. Request 92 (RENDER_ANSWERS.md R7, with TM2_PORT_VISUAL, portVisual bit 7): portVisualSteadyBegin / End run in
   LinkMips on a made-up tilted Mr. Slam with uneven suspension (links without TM2_PORT_CAR_GROUPS): Begin gives his
   shadow the game's carBodyPoseUpdate pose for even suspension with the step's translation, End the step's matrix
   back, the bit clear nothing.
14. Requests 90 and 91 (RENDER_ANSWERS.md R6.1, FAR_LOD.md 13, with TM2_PORT_FAR_LOD): portFarLodScanLevel in
   LinkMips on DEN, BURB and PARIS lists the records tools/dmd_lod_dump.py does (the extension: extended() and
   fine_field()'s never records, flagged; the near scale: every LOD's records, fine_field()'s fine ones flagged: on
   the disc only Holland's grass field, the beams 8002F50C and the block and patch records above them);
   portFarLodBegin with scale 4 scales them like every band with portFarLod 0, and with portFarLod set draws the
   beams, cells and blocks at every distance and the block and patch quads at none; portFarLodEnd puts the file
   back.
9. Request 63 (FAR_LOD.md 10, with TM2_PORT_VISUAL, portVisual bit 5): the camera-wall functions CAMWALL_PORT are in
   .port_text, called only within CAMWALL_CALLERS (portVisualCameraWall only by viewChaseCamera), call nothing but each
   other, csGetRoot and renderGetWorldShift, and store no static word (no LO16 / GPREL16 store relocation); in check 7's
   interpreter (FarMips) portVisualCameraWall on a made-up level model (a translated wall behind a mark-volume door, a
   rotated wall, a wall under a level change, a distance switch, a sphere away from the segment over a wall that
   crosses it, a switched-off box, a box holding the look-at point) puts the eye where a float model of the rule does
   (within 2 units; at least 5 of the 8 cases move it), leaves it with the bit clear, writes only the eye, its
   stack and portCameraWallWork (request 89: its pending nodes and spans, 0x1000 bytes since request 96) and never the look-at point; and on Moscow's model from the disc (DISH at its own address) four eyes outside
   the outer fence come back inside it, two eyes with no wall between stay. A link without the define fails (the
   negative control: the functions are not there).

10. Request 71 (CAR_PROFILES.md 5-7, with TM2_PORT_DT_PLAYER or TM2_PORT_CAR_NUMBERS): portCarSelLoad runs in check
    7's interpreter across the link (LinkMips: LoadImage and DrawSync stubs that record each upload and its bank word,
    the link's own bytes under unwritten memory) on a made-up 34-child carousel with the disc's SP, BB, MN and RK
    files in slots 0-3 (without texture banks SP then RK in slot 0):
    - it returns 1; every TIM's pixels and its CLUT's first row are uploaded from the file, in the slot's area and
      bank (124 + k; 0 and car select's free block without banks), inside a texture page as the remap addresses it,
      none overlapping; the slot table is {page | bank << 9, x, y, CLUT x, CLUT y} of those uploads;
    - the child is the file's car root (request 74: Dark Tooth's truck in a group with a node holding his head;
      request 109: a transform node, a half turn about z, 240 above it), the slot's index and model are
      set, the other slots
      untouched, vramTexSlots / vramTexSlotGroup / portTexBanks.live restored;
    - every textured primitive of every root (the real modelRelocate / modelRemapTextures) has its image's page bits,
      the bank in tpage bits 9..15 (banked), its CLUT and its UVs moved by the image's place in the page (at least
      100 primitives);
    - banked: portCarSelUnload empties only its own child; a reload frees the slot's old child; a retail index (13)
      returns -2 with no upload; a cut TMS returns -4 and leaves the empty node.
11. Request 70 (CAR_PROFILES.md 1-4, with TM2_PORT_CAR_PROFILE): the profile functions PROFILE_PORT are in .port_text,
    reach no rand(), are called only by PROFILE_CALLERS (the hooked game functions) and pointed to by nothing; in
    LinkMips a made-up mod on Spectre (number 0x25 in load groups 2 and 6 of human record 0's and AI record 1's slots):
    carStatsLoad, carWheelsSetStepHeight, weaponBlockInit and viewCameraOffsetsInit give the profile's fields (both
    records), the refused entries (HP, a wrong width, a derived field) are not written, the wheelbase, the speeds and
    the retaliation budget are derived from the overrides, nothing else changes; portCarProfileTopSpeed, _Radar and
    _Name give the profile's top speed, colour key and name; with valid 0 every one gives the base's (retail's).

12. Request 72 (HIDDEN_AREAS.md 11, with TM2_PORT_HIDDEN_AREAS, portVisual bit 6): the functions HIDDEN_PORT are in
    .port_text, reach no rand(), are called only by HIDDEN_CALLERS (portHiddenAreasBegin and portHiddenAreasEnd by
    renderScene, once each: Begin before every renderWalkModel call, End after every one and before the emitter
    call); in LinkMips with PARIS.DMD and PARIS2.DMD from the disc at their own address (0x80019C40):
    - with the bit, Begin returns 6 and names the place; a walk of the level (the walker's choices: a zone's
      either child, an LOD's first record holding d2, a select's current child) reaches at 50, 600, 2,000 and
      20,000 world units the church's real inside, the walls with the doorway (not the far facade), the door, and
      never the stand-in, while the same walks before Begin are retail's (no inside from outside the zones, the far
      facade past 900, no door past 900);
    - End leaves every byte of the level file as it was before Begin (also with a far record already extended by
      portFarLodBegin: its extended value comes back); drawn[view] holds the words patched;
    - with the bit clear, and with another level (ISLANDS) loaded, Begin patches nothing and returns 0;
    - request 75 (HIDDEN_AREAS.md 12): with the bit the walks also reach the roof's two meshes (portHiddenRoof.built
      2), whose 18 quads, through the turn node, lie at z 832 facing up, behind the towers (local y 764..1,628,
      x -768..768: not over the notch's tiles); with the nave not as listed (its primitive count changed) the church
      is still patched but without the roof (built 0);
    - request 78 (HIDDEN_AREAS.md 12.6; viewGetEyeTrans stubbed): with the eye at z 1,000 Begin lowers every vertex
      of the nave at z 920 (42) to 832 (lowered 1), none is left at 920, the roof's quads still lie at z 832, they
      stay so after End (request 84: the emitter reads them later) and portHiddenAreasDrawn gives back the file's bytes
      (lowered 0); with the eye at z 832, or with a nave vertex already at 832, nothing is lowered; renderScene calls
      Drawn once, after the emitter and renderNearPolys.

13. Request 73 (SPECIAL_TEMPLATES.md 6, with TM2_PORT_SPECIAL_PROFILE): the functions SPECIAL_PORT are in .port_text,
    reach no rand(), are called only by SPECIAL_CALLERS; in LinkMips with a made-up mod on Spectre (as check 11's):
    - portSpecialNum gives each set field clamped to its limit (life 9000 -> 8191, speed 500 -> 150, range 0 -> 1),
      retail's value for an unset field and without a valid profile;
    - portSpecialFireBegin / End: only the records free before and live after take the numbers (damage, life, speed,
      turn; a Minion's freeze missile its DAMAGE2; the spiral's turn 99 -> 98); a record live before is untouched;
    - weaponBlockInit applies the template (Warthog: portCarSpecialAs = 0x40, Warthog's 700 / 5, or request 70's
      recharge / max when the profile sets them; a pickup template keeps Spectre's own 700 / 4; a refused template
      (Twister) leaves the base's);
    - aiCarSetupProfile takes the template's tables (Warthog's), and a pickup template gets copies of the car's own
      with slot-0 weight 50 (or the profile's);
    - portSpecialFirePickup calls weaponLaunchProjectile with the pickup's kind (1, 2, 4, 5, 100), Mega Napalm with the
      flag set for the call and given back.
    - stage 2: portSpecialNumById finds the car by its object id (a stub carFindById), retail's value for an unknown
      id; portSpecialBlast gives Shadow's blast base (and the surface's 25 on top), a remote bomb's retail; a caught
      boomerang gives the charge to its thrower, human or AI; the ricochet template fills a pool shot (owner id, the
      car's life and speed); the remote bomb template places (the charge given back, drop_armed 1) and then detonates
      (drop_armed 0), both through weaponDropItem kind 3. The pickup templates reach rand() only through retail's
      pickup code (the launcher, the drop), as a pickup's fire does.

--negative-control on a build without the defines runs checks 1-3, 5, 6, 7, 8, 9, 10, 11, 12 and 13 as if they were
there (they must fail).
Exit status 0 = pass.
"""
import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from survey_layout import ROOT  # noqa: E402
from reloc_image import Link, AddrMap  # noqa: E402
from check_hazards import Flow, load_functions, jump_tables, call_graph, reaches, sext16  # noqa: E402

R_MIPS_LO16 = 6
DT_ORD_PORT = {'portCarDarkToothOrdinary', 'portCarDarkToothRole', 'portCarDarkToothHeadShow'}
DT_ORD_CALLERS = {
    'portCarDarkToothOrdinary': {'aiWeaponFireDecide', 'aiCarsCountRemaining', 'roundStart', 'carContactApplyObject',
                                 'aiDarkToothJumpSelect', 'aiFollowRouteTarget', 'carApplyDamage', 'carDestroyed',
                                 'portCarDarkToothWreck', 'portCarDarkToothStages', 'portCarDarkToothHeadShow'},
    'portCarDarkToothHeadShow': {'aiWeaponFireDecide'},
    'portCarDarkToothRole': set(),
}
VISUAL_PORT = {'portVisualArcOtz', 'portVisualOwnCar',     # portVisualOwnCar: request 45 item 3 (bit 2)
               'portVisualTaserFrom'}                       # request 76 (bit 1)
# request 46 items 4 and 5 (bits 4 and 3)
VISUAL_PORT_46 = {'portVisualWalked', 'portVisualShadows', 'portVisualBulletSpawn', 'portVisualBulletTail',
                  'portVisualSteadyBegin', 'portVisualSteadyEnd',         # request 92 (bit 7)
                  'portVisualNearOtz',                                    # request 93 (bit 8)
                  'portVisualMirrorEye', 'portVisualMirrorSkip', 'portVisualMirrorHuman'}   # request 99 (bit 9)
VISUAL_CALLERS_46 = {'portVisualWalked': {'renderScene'}, 'portVisualShadows': {'renderScene'},
                     'portVisualSteadyBegin': {'renderScene'}, 'portVisualSteadyEnd': {'renderScene'},
                     'portVisualNearOtz': {'renderScene', 'renderDrawModel'},
                     'portVisualMirrorEye': {'viewProc'}, 'portVisualMirrorSkip': {'renderScene'},
                     'portVisualMirrorHuman': {'portVisualMirrorEye', 'portVisualMirrorSkip'},
                     'portVisualBulletSpawn': {'bulletSpawn'}, 'portVisualBulletTail': {'streaksDraw'}}
EXPECTED_VISUAL = {'weaponArcsDraw', 'renderScene', 'bulletSpawn', 'streaksDraw', 'viewChaseCamera',   # + request 63
                   'worldLoadObject', 'worldLoadStartObjects',                                          # request 79
                   'renderDrawModel',                                                                   # request 93
                   'viewProc'}                                                                          # request 99
# request 79: the special's pickup look
PICKUP_SPECIAL_PORT = {'portPickupSpecialBuild', 'portPickupSpecialBeam', 'portPickupSpecialDonor',
                       'portPickupSpecialMesh'}
PICKUP_SPECIAL_CALLERS = {'portPickupSpecialBuild': {'worldLoadStartObjects'},
                          'portPickupSpecialBeam': {'portPickupSpecialDonor'},
                          'portPickupSpecialDonor': {'portPickupSpecialBuild'},
                          'portPickupSpecialMesh': {'portPickupSpecialBuild'}}
ICON_PORT = {'portWeaponIcon', 'portWeaponIconMesh'}
ICON_CALLERS = {'portWeaponIcon': set(), 'portWeaponIconMesh': {'portWeaponIcon'}}
PICKUP_SPECIAL_LEVELS = ('FREEWAY', 'DISH', 'PARIS', 'PARIS2', 'ISLANDS', 'ROOF', 'ROOF2', 'GLACIER', 'GLACIER2',
                         'DEN', 'HKONG', 'SWAMP', 'BURB', 'SROOF')
RENDER_DEFINES = {'TM2_PORT_FAR_LOD', 'TM2_PORT_FAR_DISTANCE',       # check 5 (request 46)
                  'TM2_PORT_RENDER_SLOTS',                              # check 6 (request 47)
                  'TM2_PORT_C_EMITTER'}                                 # check 8 (request 61 addendum)
# request 47: the matrix slot arrays (render_slots.h; retail address, bytes a slot) and what the define changes
SLOT_ARRAYS = {'renderSlotMatrixPtrs': (0x801ABA50, 16), 'renderMatrixSlots': (0x801AEB18, 32),
               'renderObjectMatrices': (0x801F8AB8, 32), 'renderFrustumMatricesA': (0x801B2E0C, 32),
               'renderFrustumMatricesB': (0x801B470C, 32)}
RETAIL_SLOTS = 200
RENDER_SLOTS_CHANGED = {'renderScene', 'renderWalkModel', 'renderPushMatrix', 'renderMulTransMatVec',
                        'renderInitNodeHandlers'}
PACKET_BUFFER_BYTES = 5 * 2 * 4 * 97600     # views (1-4, the mirror) x parities x 4 x retail's 97,600
# request 47 revised: a packet buffer per view; the functions and their callers (ApplySet: the port's only)
PACKET_CALLERS = {'portPacketBuffersApplyView': {'portPacketBuffersApply', 'portPacketBuffersApplySet'},
                  'portPacketBuffersApply': {'rtMainLoop', 'portPacketBuffersApplySet'},
                  'portPacketBuffersApplySet': set(),
                  'portPacketBufferCursor': {'rtMainLoop'}}
PACKET_PORT = set(PACKET_CALLERS)
CARSEL_PORT = {'portCarSelExtend', 'portCarSelCopy', 'portCarSelCount', 'portCarBioFile', 'portCarSelSetChild',
               'portCarSelNumber', 'portCarEnding', 'portCarSelReset'}
NUMBER_PORT = {'portCarNumberGet', 'portCarNumberSet', 'portCarNumbersClear', 'portCarNumberBase',
               'portCarNumberPickable', 'portCarNumberType', 'portCarNumberName', 'portCarNumberCode',
               'portCarNumberOfGroup', 'portCarNumbersKeep', 'portCarSelDraw', 'portCarNumberMod',
               'portAiSelExtend', 'portAiSelCount'}
NUMBER_CALLERS = {
    'portCarNumberGet': {'sessionGetOpponentCar', 'sessionGetHumanCar', 'settingsGetOpponentCar', 'settingsGetHumanCar',
                         'sessionRun'},
    'portCarNumberSet': {'sessionSetOpponentCar', 'settingsSetOpponentCar', 'settingsSetHumanCar'},
    'portCarNumbersClear': {'settingsClearOpponentCars', 'settingsClearHumanCars'},
    'portCarNumberPickable': {'carIsSelectable'},
    'portCarNumberType': {'carIndexToType'},
    'portCarNumberName': {'carDisplayName'},
    'portCarNumberCode': {'carFileCode'},
    'portCarNumbersKeep': {'shellAttractDemo'},
    'portCarSelDraw': {'sceneObjDraw'},
    'portCarSelNumber': {'screenCarSelect'},
    'portCarSelExtend': {'screenCarSelect'},
    'portCarSelCopy': {'portCarSelExtend', 'portAiSelExtend'},
    'portCarSelReset': {'portCarSelExtend', 'portAiSelExtend'},
    'portAiSelExtend': {'screenAiSelect'},      # item 2: Sweet Tooth in Challenge's picker (bit 11)
    'portAiSelCount': {'screenAiSelect'},
    'portCarEnding': {'screenEnding'},
    'portCarNumberOfGroup': set(),          # the port's API
    'portCarSelSetChild': {'portCarSelLoad'},   # the port's API; request 71's helper
}
NUMBER_CALLERS_CARS = {'portCarNumberGet': {'sessionLoadArenaAndCars'}}
EXPECTED_NUMBERS = {
    'sessionGetOpponentCar', 'sessionGetHumanCar', 'sessionSetOpponentCar', 'settingsGetOpponentCar',
    'settingsClearOpponentCars', 'settingsGetHumanCar', 'settingsClearHumanCars', 'settingsSetOpponentCar',
    'settingsSetHumanCar', 'carIndexToType', 'carFileCode', 'carDisplayName', 'carIsSelectable', 'sessionRun',
    'pauseMenu', 'screenEnding', 'shellAttractDemo', 'sceneObjDraw', 'screenCarSelect',
    'screenAiSelect',   # item 2 (bit 11)
    'passwordBuildString', 'screenLoadCardImages',   # the getters' int result (no sign extension)
}
EXPECTED_NUMBERS_CARS = {'sessionLoadArenaAndCars'}
STORAGE = {'portVisual': 4, 'portCarDtBossMask': 8, 'portSettingsHumanCar': 32, 'portSettingsOpponentCar': 32,
           'portModCars': 4096, 'portCarNumbers': 208, 'portCarSel': 2016,
           # request 46 (every variant)
           'portFarLodNearScale': 4, 'portMirrorFar': 4, 'portFarLodNear': 49156, 'portVisualState': 0x82C,
           'portRenderStats': 0x150,     # the render capacity counter (port_render_stats.h)
           'portPacketBuffers': PACKET_BUFFER_BYTES,    # request 47 revised: a 4 x buffer per view and parity
           'portFarSprites': 0x22010,   # request 61: the range pass's sprite and beam meshes (struct PortFarSprites)
           'portCarSelLoads': 4 * 0x203C0, 'portCarProfile': 128 * 0xA0,   # requests 71 and 70 (car_numbers.h)
           'portHiddenAreas': 0x1D0,                                        # request 72 (port_visual.h; 104)
           'portSpecialProfile': 128 * 0x48, 'portSpecialState': 0xC08}     # request 73 (car_numbers.h)
STATS_PORT = {'portRenderStatsWalked': {'viewGetMode'}, 'portRenderStatsDrawn': set()}   # -> the calls allowed
OUR_DEFINES = {'TM2_PORT_VISUAL', 'TM2_PORT_CAR_NUMBERS'}


def lo_targets(relocs, symbols):
    """instruction address -> symbol name of its LO16 relocation (a load / store / addiu of a global)."""
    by_addr = {}
    for name, s in symbols.items():
        by_addr.setdefault(s['address'], name)
    out = {}
    for p_c, rtype, v_c, item, sym in relocs:
        if rtype == R_MIPS_LO16:
            out[p_c] = by_addr.get(v_c, sym)
    return out


def bit_gate_returns_zero(link, start, size, los, symbol, mask):
    """Walk the function from its entry: follow the code until a conditional branch on (symbol & mask) against zero;
    then from its bit-clear edge to `jr ra`, which must be reached without a call or another conditional branch with
    $v0 = 0 (after the delay slot). Returns (ok, text)."""
    end = start + size
    reg = {}                                    # register -> 'val' (loaded from symbol) / 'bit' (val & mask)
    a = start
    gate = None
    while a < end and a < start + 4 * 64:
        w = link.word(a)
        op, rs, rt, rd, funct = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
        if op == 0x23 and los.get(a) == symbol:            # lw rt, %lo(symbol)(rs)
            reg[rt] = 'val'
        elif op == 0x0C and reg.get(rs) == 'val' and (w & 0xFFFF) == mask:   # andi rt, rs, mask
            reg[rt] = 'bit'
        elif op in (4, 5) and 0 in (rs, rt) and reg.get(rs if rt == 0 else rt) == 'bit':
            t = a + 4 + ((w & 0xFFFF) - (0x10000 if w & 0x8000 else 0)) * 4
            gate = (a, t if op == 4 else a + 8)          # beq: clear -> target; bne: clear -> fall through
            break
        elif op in (1, 2, 3, 4, 5, 6, 7) or (op == 0 and funct in (8, 9)):
            return False, 'a branch, jump or call at %08X before the gate' % a
        else:
            d = rd if op == 0 else rt if (8 <= op <= 0x0F or 0x20 <= op <= 0x26) else None
            if d is not None:
                reg.pop(d, None)
        a += 4
    if gate is None:
        return False, 'no branch on %s & 0x%X' % (symbol, mask)
    v0 = None
    # the gate's delay slot runs on both edges
    seq = [gate[0] + 4]
    a = gate[1]
    steps = 0
    while steps < 32:
        steps += 1
        seq.append(a)
        w = link.word(a)
        op, rs, rt, funct = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 63
        if op == 0 and funct == 8 and rs == 31:          # jr ra: its delay slot, then out
            seq.append(a + 4)
            break
        if op == 4 and rs == 0 and rt == 0:              # b
            seq.append(a + 4)
            a = a + 4 + ((w & 0xFFFF) - (0x10000 if w & 0x8000 else 0)) * 4
            continue
        if op == 2:                                      # j (inside the function)
            seq.append(a + 4)
            a = ((a + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            continue
        if op in (1, 3, 4, 5, 6, 7) or (op == 0 and funct == 9):
            return False, 'the bit-clear path from %08X branches or calls at %08X' % (gate[0], a)
        a += 4
    else:
        return False, 'the bit-clear path from %08X does not return' % gate[0]
    for a in seq:
        w = link.word(a)
        op, rs, rt, rd, funct = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
        if op == 0 and rd == 2 and funct in (0x21, 0x25, 0x20) and rs == 0 and rt == 0:
            v0 = 0                                       # addu / or / add v0, zero, zero
        elif op == 0 and rd == 2 and w != 0:
            v0 = None
        elif op in (8, 9, 0x0D) and rt == 2:             # addi / addiu / ori v0, rs, imm
            v0 = 0 if rs == 0 and (w & 0xFFFF) == 0 else None
        elif op in (0x0C, 0x0F, 0x20, 0x21, 0x23, 0x24, 0x25, 0x0A, 0x0B) and rt == 2:
            v0 = None
    if v0 != 0:
        return False, 'the bit-clear path from %08X returns $v0 not 0' % gate[0]
    return True, 'branch at %08X: bit clear -> return 0 (%d instructions)' % (gate[0], len(seq))


def stores_outside(flow, relocs_at, lo, hi):
    """The stores of a function whose address is not formed from [lo, hi) (or the stack): a must-taint walk of the
    flow graph. A register is tainted by lui / addiu / ori with a HI16 / LO16 relocation into the range, by addiu of
    a tainted register and by addu / add / or of exactly one tainted register (pointer + index); any other write
    clears it, and a call clears the caller-saved registers (after its delay slot). Returns [address]."""
    clobber = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 31}
    call_slots = {a + 4 for a in flow.calls}
    state = {flow.start: frozenset()}
    todo = [flow.start]
    bad = set()

    def step(a, t):
        w = flow.insn[a]
        op, rs, rt, rd, funct = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63
        hit = any(lo <= v < hi for v in relocs_at.get(a, ()))
        t = set(t)
        if op in (0x28, 0x29, 0x2A, 0x2B, 0x2E):                 # sb sh swl sw swr
            if not (rs == 29 or rs in t or hit):
                bad.add(a)
        elif op == 0x0F:                                         # lui
            if hit:
                t.add(rt)
            else:
                t.discard(rt)
        elif op in (8, 9, 0x0D):                                 # addi / addiu / ori
            if rs in t or hit:
                t.add(rt)
            else:
                t.discard(rt)
        elif op == 0 and funct in (0x20, 0x21, 0x25):            # add / addu / or
            if (rs in t) != (rt in t):
                t.add(rd)
            else:
                t.discard(rd)
        elif op == 0 and rd:
            t.discard(rd)
        elif 0x20 <= op <= 0x26 or 0x0A <= op <= 0x0E:           # loads, slti / sltiu / andi / xori
            t.discard(rt)
        if a in call_slots:
            t -= clobber
        t.discard(0)
        return frozenset(t)

    while todo:
        a = todo.pop()
        out = step(a, state[a])
        for s in flow.succ.get(a, ()):
            if not (flow.start <= s < flow.end):
                continue
            new = out if s not in state else state[s] & out
            if new != state.get(s):
                state[s] = new
                todo.append(s)
    return sorted(bad)


def slt_immediates(link, start, size):
    """{immediate: count} of the slti / sltiu instructions of a function."""
    out = {}
    for a in range(start, start + size, 4):
        w = link.word(a)
        if w >> 26 in (0x0A, 0x0B):
            imm = w & 0xFFFF
            imm = imm - 0x10000 if imm & 0x8000 else imm
            out[imm] = out.get(imm, 0) + 1
    return out


def counter_increments(link, start, size):
    """[(address of the store, base register)] of the `lw r,0(B); [nop;] addiu r,r,1; sw r,0(B)` increments of a
    function (a counter kept through a pointer register, as renderScene's `*objects = *objects + 1`)."""
    out = []
    words = [link.word(a) for a in range(start, start + size, 4)]
    for i, w in enumerate(words):
        if w >> 26 != 0x2B or w & 0xFFFF:
            continue
        b, r = (w >> 21) & 31, (w >> 16) & 31
        if i < 1 or words[i - 1] != (0x09 << 26 | r << 21 | r << 16 | 1):
            continue
        loads = [words[j] for j in range(max(0, i - 4), i - 1)]
        if (0x23 << 26 | b << 21 | r << 16) in loads:
            out.append((start + 4 * i, 'r%d' % b))
    return out


def calls_in(flow):
    out = {}
    for a, c in flow.calls.items():
        out[c] = out.get(c, 0) + 1
    return out


# ---- check 7 (request 61, FAR_DISTANCE.md 2): the GTE range pass run in a MIPS interpreter ----
FAR_H = 277                     # the projection plane distance of view mode 0 (viewModeTable +0x08)
FAR_ERR_PX = 0.1                # a scaled mesh's vertices must project within this of the exact projection
FAR_SPRITE_HEAD = 48            # PORT_FAR_SPRITE_HEAD: the node's first 44 bytes, +44 the source's primitives
FAR_SPRITES_BYTES = 0x22010     # struct PortFarSprites
FAR_SCRATCH, FAR_NODES, FAR_LIGHT = 0x1F800000, 0x80E00000, 0x80E80000
FAR_SPRITE_USERS = {'portFarDistanceScale', 'portFarSpriteMesh', 'portFarSpriteSource'}   # (the last: request 61 addendum, read only)


def far_rot(rx, ry, rz):
    """A rotation matrix in 4096 units (row-major), as the slots hold."""
    import math
    cx, sx, cy, sy, cz, sz = math.cos(rx), math.sin(rx), math.cos(ry), math.sin(ry), math.cos(rz), math.sin(rz)
    X = [[1, 0, 0], [0, cx, -sx], [0, sx, cx]]
    Y = [[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]]
    Z = [[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]]
    mm = lambda A, B: [[sum(A[i][k] * B[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
    M = mm(mm(Y, X), Z)
    return [[int(round(M[i][j] * 4096)) for j in range(3)] for i in range(3)]


def far_emit_point(Rp, t, v):
    """The emitter's projection of vertex v of an entry (sub_800CDC54: TR = R'.t split >> 15 / & 0x7FFF, sf=0 << 3 and
    sf=1; RTPS: MAC = TR * 4096 + R'.v, IR = MAC >> 12, SZ = MAC3 >> 12): (sx, sy) unrounded, SZ."""
    hi, lo = [], []
    for x in t:
        a = -x if x < 0 else x
        hi.append(-(a >> 15) if x < 0 else a >> 15)
        lo.append(-(a & 0x7FFF) if x < 0 else a & 0x7FFF)
    tr = [(sum(Rp[i][j] * hi[j] for j in range(3)) << 3) + (sum(Rp[i][j] * lo[j] for j in range(3)) >> 12)
          for i in range(3)]
    mac = [tr[i] * 4096 + sum(Rp[i][j] * v[j] for j in range(3)) for i in range(3)]
    ir = [max(-32768, min(32767, m >> 12)) for m in mac[:2]]
    sz = max(0, min(65535, mac[2] >> 12))
    return (FAR_H * ir[0] / sz, FAR_H * ir[1] / sz, sz) if sz else (0.0, 0.0, 0)


def far_exact_point(R, t, v):
    p = [sum(R[i][j] * (t[j] + v[j]) for j in range(3)) / 4096.0 for i in range(3)]
    return FAR_H * p[0] / p[2], FAR_H * p[1] / p[2], p[2]


class FarMips:
    """check_rollback.MiniMips run across every function of the link (the pass calls its helpers), with div / divu,
    and the emitter (sub_800CDC54) as a stub that records its five arguments."""

    def __init__(self, img):
        from check_rollback import MiniMips
        self.m = MiniMips(img, {})
        self.img = img
        self.emit = []
        base = MiniMips.exec

        def exec_(mm, pc, w):
            op, fn = w >> 26, w & 63
            if op == 0 and fn in (26, 27):          # div, divu (the compiler's divide-by-zero break follows)
                rs, rt = mm.r[(w >> 21) & 31], mm.r[(w >> 16) & 31]
                if fn == 26:
                    a = rs - (1 << 32) if rs & 0x80000000 else rs
                    b = rt - (1 << 32) if rt & 0x80000000 else rt
                else:
                    a, b = rs, rt
                if b:
                    q = abs(a) // abs(b) * (1 if (a < 0) == (b < 0) else -1)
                    mm.lo, mm.hi = q & 0xFFFFFFFF, (a - q * b) & 0xFFFFFFFF
                return None
            if op == 0 and fn in (17, 19):          # mthi, mtlo
                if fn == 17:
                    mm.hi = mm.r[(w >> 21) & 31]
                else:
                    mm.lo = mm.r[(w >> 21) & 31]
                return None
            return base(mm, pc, w)
        self.exec_ = exec_

    def run(self, fn, args=(), steps=3000000):
        m, img = self.m, self.img
        r = m.r
        for i, v in enumerate(args):
            r[4 + i] = v & 0xFFFFFFFF
        r[29], r[31] = 0x801FF000, m.RET
        r[28] = img.link.gp_c       # retail's small data (renderCtx) is $gp-relative
        emitter = img.sizes['sub_800CDC54'][0] if 'sub_800CDC54' in img.sizes else None
        pc, delay = img.sizes[fn][0], None
        for _ in range(steps):
            if pc == m.RET:
                return r[2]
            nxt = self.exec_(m, pc, img.link.word(pc))
            if delay is not None:
                pc, delay = delay, None
                if pc == emitter:
                    self.emit.append((r[4], r[5], r[6], r[7], m.rd(r[29] + 16, 4)))
                    pc = r[31]
                continue
            if nxt is None:
                pc += 4
            else:
                delay, pc = nxt, pc + 4
        raise RuntimeError('%s: no return in %d steps' % (fn, steps))


def far_case(rng, k_hint):
    """A mesh far ahead of the camera: (R, t, c, radius, vertices) in coordinate units (world << 3)."""
    import math
    R = far_rot(rng.uniform(-0.6, 0.6), rng.uniform(-math.pi, math.pi), rng.uniform(-0.1, 0.1))
    dist = {0: 60, 1: 5000, 2: 10000, 3: 24000}[k_hint] * 8
    ang = rng.uniform(-0.4, 0.4)
    pv = [dist * math.sin(ang), rng.uniform(-150, 150) * 8, dist * math.cos(ang)]
    pc = [int(round(sum(R[j][i] * pv[j] for j in range(3)) / 4096.0)) for i in range(3)]     # R^T pv: model frame
    c = [rng.randint(-20000, 20000), rng.randint(-20000, 20000), rng.randint(-3000, 3000)]
    t = [pc[i] - c[i] for i in range(3)]
    radius = 300 if k_hint else 40
    vs = []
    for _ in range(6):
        off = [rng.uniform(-1, 1) for _ in range(3)]
        n = math.sqrt(sum(x * x for x in off)) or 1
        vs.append([max(-32768, min(32767, int(c[i] + off[i] / n * radius * rng.random()))) for i in range(3)])
    return R, t, c, radius, vs


def check_far_sprites(out, link, sizes, names_at, rows, kinds, relocs):
    """Check 7 (request 61): portFarDistanceEmit on a made-up view queue in a MIPS interpreter (the emitter a stub):
    the scaled entries project within FAR_ERR_PX of the exact projection (the translation corrected for the scaled
    matrix's rounding), their sprites' and beams' sizes are divided by 2^k in a copy of the mesh (the beam's half
    length and the triangles kept, a beam at least 1 wide), a mesh drawn twice at one k shares one copy and at two k
    has two, the unscaled entries are untouched, the pass writes only its own state and the queue; and no game
    function names portFarSprites. A link of the pass before request 61 fails (the translation, the sizes)."""
    import random
    from check_rollback import Image
    bad = []
    need = ('portFarDistanceEmit', 'sub_800CDC54')
    sym = lambda n: link.symbols[n]['address'] if n in link.symbols else None
    if any(n not in sizes for n in need) or any(sym(n) is None for n in
                                                ('portFarState', 'renderCtx', 'portFarDistance',
                                                 'renderMatrixSlots', 'renderObjectMatrices', 'renderMeshList')):
        bad.append('the pass or its storage is not in the link')
        print('7. far sprites and translations (request 61) -> FAIL: %s' % bad[0])
        return bad
    for n in ('portFarSpriteMesh', 'portFarTranslate'):
        if n not in sizes:
            bad.append('%s not in the link' % n)
    img = Image(out)
    fm = FarMips(img)
    m = fm.m
    rng = random.Random(61)
    m.wr(sym('portFarDistance'), 4, 32000)
    m.wr(sym('renderCtx'), 4, FAR_SCRATCH)
    nslots = 24                 # a slot of its own for every entry (entry 1 shares entry 0's)
    m.wr(FAR_SCRATCH + 8, 4, nslots - 1)
    # nodes: A sprites + beams + a triangle, B triangles only
    prims_a = [(0, 344, 200), (0, 3, 5), (1, 20, 50), (1, 1, 10), (3, 0, 0), (0, -40, 40)]
    prims_b = [(3, 0, 0), (4, 0, 0)]

    def node(addr, prims, centre, radius):
        pa = addr + 0x40
        m.fill(addr, bytes(0x40))
        m.wr(addr, 1, 0)
        m.wr(addr + 4, 4, FAR_NODES + 0x10000)
        m.wr(addr + 8, 4, FAR_NODES + 0x18000)
        m.wr(addr + 12, 4, pa)
        m.wr(addr + 16, 4, len(prims))
        m.wr(addr + 24, 2, 0x1234)
        for i in range(3):
            m.wr(addr + 28 + 4 * i, 4, centre[i])
        m.wr(addr + 40, 4, radius)
        for j, (t, w, h) in enumerate(prims):
            p = pa + 20 * j
            m.fill(p, bytes((0x83, 3, 5, 9)) + bytes(16))
            m.wr(p + 4, 2, j)
            m.wr(p + 6, 2, w)
            m.wr(p + 8, 2, h)
            m.wr(p + 12, 4, (t << 16) | 0x80400000 | j)
            m.wr(p + 16, 4, 0x00808080 + j)
        return addr

    cases = []
    plan = [('A', 3), ('A', 3), ('A', 1), ('B', 2), ('B', 3), (None, 0)] + [('B', 1 + i % 3) for i in range(14)]
    list_base = sym('renderMeshList')
    for i, (kind, k) in enumerate(plan):
        R, t, c, radius, vs = far_case(rng, k)
        if i == 1:                              # the same mesh as entry 0, the same slot, nearly the same place
            R, t, c, radius, vs = cases[0][1:6]
            t = [t[0] + 8, t[1], t[2]]
        slot = 1 + i
        if i == 1:
            slot = cases[0][6]
        addr = FAR_NODES + 0x1000 * i
        prims = prims_a if kind == 'A' else prims_b
        node(addr, prims, c, radius)
        if i == 1:
            addr = cases[0][0]
        # the slot: the rotation (9 halves), as renderMatrixSlots holds it
        sa = sym('renderMatrixSlots') + 32 * slot
        for j in range(9):
            m.wr(sa + 2 * j, 2, R[j // 3][j % 3])
        e = list_base + 32 * i
        for j, v in enumerate((addr, t[0], t[1], t[2], 3, FAR_LIGHT, slot, 5 + i // 2)):
            m.wr(e + 4 * j, 4, v)
        cases.append((addr, R, t, c, radius, vs, slot, kind, k, list(prims)))
    before = dict(m.mem)
    m.writes.clear()
    fm.run('portFarDistanceEmit', (0x80E90000, len(cases)))
    if len(fm.emit) != 1:
        bad.append('the emitter called %d times (want 1)' % len(fm.emit))
        return bad
    _, lst, count, rot, light = fm.emit[0]
    rd = lambda a, n: m.rd(a, n)
    s16 = lambda a: rd(a, 2) - 0x10000 if rd(a, 2) & 0x8000 else rd(a, 2)
    s32 = lambda a: rd(a, 4) - (1 << 32) if rd(a, 4) & 0x80000000 else rd(a, 4)
    worst, copies = 0.0, {}
    for i, (addr, R, t, c, radius, vs, slot, kind, khint, prims) in enumerate(cases):
        e = lst + 32 * i
        ent = [rd(e + 4 * j, 4) for j in range(8)]
        k = 3 - ent[4]
        if khint == 0:
            if any(rd(e + j, 1) != before.get(e + j, 0) for j in range(32)):
                bad.append('entry %d (near, k = 0) changed' % i)
            continue
        if not 1 <= k <= 3:
            bad.append('entry %d: k = %d (want a scaled mesh)' % (i, k))
            continue
        if ent[7] != (0x40000000 | (k << 28) | i):
            bad.append('entry %d: key %08X (want %08X)' % (i, ent[7], 0x40000000 | (k << 28) | i))
        Rp = [[s16(rot + 32 * ent[6] + 2 * (3 * a + b)) for b in range(3)] for a in range(3)]
        want = [[(R[a][b] + (1 << (k - 1))) >> k for b in range(3)] for a in range(3)]
        if Rp != want:
            bad.append('entry %d: scaled matrix %s (want round(R / 2^%d) %s)' % (i, Rp, k, want))
        ts = [s32(e + 4 + 4 * j) for j in range(3)]
        for v in vs:
            ex, ey, ez = far_exact_point(R, t, v)
            sx, sy, sz = far_emit_point(Rp, ts, v)
            worst = max(worst, ((sx - ex) ** 2 + (sy - ey) ** 2) ** 0.5)
        mesh = ent[0]
        if kind == 'B':
            if mesh != addr:
                bad.append('entry %d: a mesh without sprites drawn from %08X (want its node %08X)' % (i, mesh, addr))
            continue
        if mesh == addr:
            bad.append('entry %d: the sprite mesh is drawn from its own node (sizes not divided by 2^%d)' % (i, k))
            continue
        copies.setdefault((addr, k), set()).add(mesh)
        for j in range(44):
            if j // 4 != 3 and rd(mesh + j, 1) != rd(addr + j, 1):
                bad.append('entry %d: the copy\'s node byte +%d differs' % (i, j))
                break
        if rd(mesh + 44, 4) != rd(addr + 12, 4):
            bad.append('entry %d: the copy\'s +44 is %08X (want the source\'s primitives %08X)' % (
                i, rd(mesh + 44, 4), rd(addr + 12, 4)))
        pa = rd(mesh + 12, 4)
        if pa != mesh + FAR_SPRITE_HEAD:
            bad.append('entry %d: the copy\'s primitives at %08X (want %08X)' % (i, pa, mesh + FAR_SPRITE_HEAD))
            continue
        rnd = lambda x: (x + (1 << (k - 1))) >> k
        for j, (tp, w, h) in enumerate(prims):
            p = pa + 20 * j
            got = (s16(p + 6), s16(p + 8))
            if tp == 1:
                exp = (max(1, rnd(w)) if w > 0 else rnd(w), h)
            elif tp in (3, 4):
                exp = (w, h)
            else:
                exp = (rnd(w), rnd(h))
            if got != exp:
                bad.append('entry %d prim %d (type %d): sizes %s (want %s)' % (i, j, tp, got, exp))
            if any(rd(p + q, 1) != rd(addr + 0x40 + 20 * j + q, 1) for q in range(20) if not 6 <= q < 10):
                bad.append('entry %d prim %d: copied bytes differ outside the sizes' % (i, j))
    if worst > FAR_ERR_PX:
        bad.append('a scaled vertex projects %.2f px from the exact projection (want <= %.2f)' % (worst, FAR_ERR_PX))
    k0, k2 = 3 - rd(lst + 16, 4), 3 - rd(lst + 32 * 2 + 16, 4)
    c0 = copies.get((cases[0][0], k0), set())
    if len(c0) != 1:
        bad.append('the mesh drawn twice at k = %d has %d copies (want 1)' % (k0, len(c0)))
    if k2 != k0 and copies.get((cases[0][0], k2), set()) & c0:
        bad.append('the mesh drawn at k = %d and %d shares a copy' % (k0, k2))
    # writes: the pass's state, the sprite meshes, the queue, the stack
    fs = sym('portFarState')
    fs_end = min(a['address'] for a in link.symbols.values() if a['address'] > fs)
    allowed = [(fs, fs_end),
               (sym('portFarSprites') or 0, (sym('portFarSprites') or 0) + FAR_SPRITES_BYTES),
               (list_base, list_base + 32 * len(cases)), (0x801F0000, 0x801FF000)]
    stray = sorted(a for a in m.writes if not any(lo <= a < hi for lo, hi in allowed))
    if stray:
        bad.append('%d bytes written outside the pass\'s state and the queue (first %08X)' % (len(stray), stray[0]))
    # no game function names portFarSprites
    s = sym('portFarSprites')
    if s is None:
        bad.append('portFarSprites not in the link')
        s = -FAR_SPRITES_BYTES
    users = set()
    for p_c, rtype, v_c, item, sname in relocs:
        if rtype == R_MIPS_LO16 and s <= v_c < s + FAR_SPRITES_BYTES:
            users.add(next((r[2] for r in rows if r[0] <= p_c < r[0] + r[1]), None))
    if not users <= FAR_SPRITE_USERS:
        bad.append('portFarSprites named by %s (want only %s)' % (sorted(map(str, users)), sorted(FAR_SPRITE_USERS)))
    print('7. far sprites and translations (request 61): %d entries, worst scaled vertex %.3f px, %d sprite mesh copies '
          '-> %s' % (len(cases), worst, sum(len(v) for v in copies.values()), 'ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


# ---- check 8 (request 61 addendum, PLAYTEST_ANSWERS.md 4.3 item 1): the C emitter against the asm ----
# the port's precise ring and its cap, read from port_c_emitter.h (request 94: 0x80CB0000, 16,380 entries)
_PCE = (ROOT / 'include/semantic/port_c_emitter.h').read_text(encoding='utf-8')
EMIT_RING = int(re.search(r'#define PORT_PRECISE_RING (0x[0-9A-Fa-f]+)', _PCE).group(1), 16)
EMIT_RING_MAX = int(re.search(r'#define PORT_PRECISE_MAX (\d+)', _PCE).group(1))
EMIT_BASE = 0x80A00000          # the made-up scenes' memory (above every link's port sections)
EMIT_SCRATCH = 0x1F800000
# the scratchpad bytes only the asm writes: its register save area and the spills around ratan2 / rsin / rcos
# (reference/model_emitter_800CDC54.c header); the stack is the C's
EMIT_ASM_ONLY = [(0x1F800000, 0x1F80002C), (0x1F80008C, 0x1F8000A0), (0x801F0000, 0x801FF000)]
EMIT_H = 277
EMIT_PRECISE_PX = 0.2           # a reported end within this of the exact projection
EMIT_SIZE_REL = 0.03            # a reported half size within this (relative, + 1/8 px) of w H / (Z 2^k)


def _s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


def _s32(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


GTE_UNR = [max(0, (0x40000 // (i + 0x100) + 1) // 2 - 0x101) for i in range(0x101)]


class Gte:
    """The PS1 GTE's registers and the commands the emitter uses (RTPS, RTPT, NCLIP, MVMVA, NCCS), integer exact as
    documented (the UNR divide, the saturations; no FLAG bits)."""

    def __init__(self):
        self.d = [0] * 32
        self.c = [0] * 32

    # -- register access (mfc2 / cfc2 read forms)
    def rd_data(self, r):
        d = self.d
        if r in (1, 3, 5, 8, 9, 10, 11):
            return _s16(d[r]) & 0xFFFFFFFF
        if r in (7, 16, 17, 18, 19):
            return d[r] & 0xFFFF
        if r == 15:
            return d[14]
        if r in (28, 29):
            v = 0
            for i in range(3):
                v |= max(0, min(0x1F, _s16(d[9 + i]) >> 7)) << (5 * i)
            return v
        return d[r] & 0xFFFFFFFF

    def wr_data(self, r, v):
        d = self.d
        v &= 0xFFFFFFFF
        if r == 15:
            d[12], d[13], d[14] = d[13], d[14], v
            d[15] = v
        elif r == 28:
            d[28] = v & 0x7FFF
            d[9], d[10], d[11] = (v & 0x1F) << 7, ((v >> 5) & 0x1F) << 7, ((v >> 10) & 0x1F) << 7
        elif r == 29:
            pass
        elif r == 30:
            d[30] = v
            x = v if not v & 0x80000000 else ~v & 0xFFFFFFFF
            d[31] = 32 - x.bit_length()
        elif r == 31:
            pass
        else:
            d[r] = v

    def rd_ctrl(self, r):
        v = self.c[r]
        if r in (4, 12, 20, 26, 27, 29, 30):
            return _s16(v) & 0xFFFFFFFF
        return v & 0xFFFFFFFF

    def wr_ctrl(self, r, v):
        self.c[r] = v & 0xFFFFFFFF

    # -- helpers
    def mat(self, base):
        c = self.c
        e = []
        for i in range(5):
            e += [_s16(c[base + i]), _s16(c[base + i] >> 16)]
        return [e[0:3], e[3:6], e[6:9]]

    def vec(self, i):
        d = self.d
        if i == 3:
            return [_s16(d[9]), _s16(d[10]), _s16(d[11])]
        return [_s16(d[2 * i]), _s16(d[2 * i] >> 16), _s16(d[2 * i + 1])]

    @staticmethod
    def sat(v, lo, hi):
        return lo if v < lo else hi if v > hi else v

    def set_mac_ir(self, macs, lm):
        for i in range(3):
            self.d[25 + i] = macs[i] & 0xFFFFFFFF
            self.d[9 + i] = self.sat(macs[i], 0 if lm else -0x8000, 0x7FFF) & 0xFFFFFFFF

    def divide(self, h, sz):
        if h < sz * 2:
            z = 16 - sz.bit_length()
            n, dd = h << z, sz << z
            u = GTE_UNR[(dd - 0x7FC0) >> 7] + 0x101
            dd = (0x2000080 - dd * u) >> 8
            dd = (0x0000080 + dd * u) >> 8
            return min(0x1FFFF, (n * dd + 0x8000) >> 16)
        return 0x1FFFF

    def rtp(self, v, sf, lm, last):
        c, d = self.c, self.d
        R = self.mat(0)
        tr = [_s32(c[5]), _s32(c[6]), _s32(c[7])]
        V = self.vec(v)
        raw = [(tr[i] << 12) + sum(R[i][j] * V[j] for j in range(3)) for i in range(3)]
        macs = [m >> (12 * sf) for m in raw]
        self.set_mac_ir(macs, lm)
        sz = self.sat(raw[2] >> 12, 0, 0xFFFF)
        d[16], d[17], d[18], d[19] = d[17], d[18], d[19], sz
        n = self.divide(c[26] & 0xFFFF, sz)
        ir1, ir2 = _s16(d[9]), _s16(d[10])
        sx = self.sat((_s32(c[24]) + ir1 * n) >> 16, -0x400, 0x3FF)
        sy = self.sat((_s32(c[25]) + ir2 * n) >> 16, -0x400, 0x3FF)
        self.wr_data(15, (sx & 0xFFFF) | ((sy & 0xFFFF) << 16))
        if last:
            m0 = _s32(c[28]) + _s16(c[27]) * n
            d[24] = m0 & 0xFFFFFFFF
            d[8] = self.sat(m0 >> 12, 0, 0x1000)

    def command(self, w):
        sf, lm, op = (w >> 19) & 1, (w >> 10) & 1, w & 0x3F
        d, c = self.d, self.c
        if op == 0x01:
            self.rtp(0, sf, lm, True)
        elif op == 0x30:
            for v in range(3):
                self.rtp(v, sf, lm, v == 2)
        elif op == 0x06:
            sx = [_s16(d[12 + i]) for i in range(3)]
            sy = [_s16(d[12 + i] >> 16) for i in range(3)]
            m0 = sx[0] * sy[1] + sx[1] * sy[2] + sx[2] * sy[0] - sx[0] * sy[2] - sx[1] * sy[0] - sx[2] * sy[1]
            d[24] = m0 & 0xFFFFFFFF
        elif op == 0x12:
            mx, v, cv = (w >> 17) & 3, (w >> 15) & 3, (w >> 13) & 3
            M = self.mat({0: 0, 1: 8, 2: 16}.get(mx, 0))
            V = self.vec(v)
            t = {0: [_s32(c[5]), _s32(c[6]), _s32(c[7])], 1: [_s32(c[13]), _s32(c[14]), _s32(c[15])],
                 2: [_s32(c[21]), _s32(c[22]), _s32(c[23])]}.get(cv, [0, 0, 0])
            self.set_mac_ir([((t[i] << 12) + sum(M[i][j] * V[j] for j in range(3))) >> (12 * sf) for i in range(3)],
                            lm)
        elif op == 0x1B:
            L, C = self.mat(8), self.mat(16)
            V = self.vec(0)
            self.set_mac_ir([sum(L[i][j] * V[j] for j in range(3)) >> (12 * sf) for i in range(3)], lm)
            IR = self.vec(3)
            bk = [_s32(c[13]), _s32(c[14]), _s32(c[15])]
            self.set_mac_ir([((bk[i] << 12) + sum(C[i][j] * IR[j] for j in range(3))) >> (12 * sf) for i in range(3)],
                            lm)
            IR = self.vec(3)
            rgb = [d[6] & 0xFF, (d[6] >> 8) & 0xFF, (d[6] >> 16) & 0xFF]
            macs = [((rgb[i] * IR[i]) << 4) >> (12 * sf) for i in range(3)]
            self.set_mac_ir(macs, lm)
            col = [self.sat(macs[i] >> 4, 0, 0xFF) for i in range(3)]
            d[20], d[21] = d[21], d[22]
            d[22] = col[0] | col[1] << 8 | col[2] << 16 | (d[6] & 0xFF000000)
        else:
            raise RuntimeError('GTE command %08X not modelled' % w)


class EmitMips:
    """check_rollback.MiniMips with div / divu / mthi / mtlo, the GTE (cop2, lwc2, swc2) and the link's own bytes behind
    the memory (code and tables: ratan2 / rsin / rcos run from the link), across every function of the link."""

    def __init__(self, img):
        from check_rollback import MiniMips
        self.img = img
        link = img.link
        self.gte = Gte()
        outer = self

        class M(MiniMips):
            def rd(self, a, n):
                v = 0
                for k in range(n):
                    b = self.mem.get(a + k)
                    if b is None:
                        b = link.mem(a + k, 1)[0] if 0x80000000 <= a + k < 0x80A00000 else 0
                    v |= b << (8 * k)
                return v
        self.m = M(img, {})
        base = M.exec
        g = self.gte

        def exec_(mm, pc, w):
            op, fn = w >> 26, w & 63
            rs, rt, rd = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
            if op == 0 and fn in (26, 27):
                a, b = mm.r[rs], mm.r[rt]
                if fn == 26:
                    a, b = _s32(a), _s32(b)
                if b == 0:
                    mm.lo, mm.hi = (1 if a < 0 else 0xFFFFFFFF) if fn == 26 else 0xFFFFFFFF, a & 0xFFFFFFFF
                elif fn == 26 and a == -0x80000000 and b == -1:
                    mm.lo, mm.hi = 0x80000000, 0
                else:
                    q = abs(a) // abs(b) * (1 if (a < 0) == (b < 0) else -1)
                    mm.lo, mm.hi = q & 0xFFFFFFFF, (a - q * b) & 0xFFFFFFFF
                return None
            if op == 0 and fn in (17, 19):
                if fn == 17:
                    mm.hi = mm.r[rs]
                else:
                    mm.lo = mm.r[rs]
                return None
            if op == 0 and fn == 13:
                raise RuntimeError('%08X: break' % pc)
            if op == 18:
                if w & 0x02000000:
                    g.command(w & 0x1FFFFFF)
                elif rs == 0:
                    mm.set(rt, g.rd_data(rd))
                elif rs == 2:
                    mm.set(rt, g.rd_ctrl(rd))
                elif rs == 4:
                    g.wr_data(rd, mm.r[rt])
                elif rs == 6:
                    g.wr_ctrl(rd, mm.r[rt])
                else:
                    raise RuntimeError('%08X: COP2 %08X' % (pc, w))
                return None
            if op == 50:
                g.wr_data(rt, mm.rd((mm.r[rs] + sext16(w & 0xFFFF)) & 0xFFFFFFFF, 4))
                return None
            if op == 58:
                mm.wr((mm.r[rs] + sext16(w & 0xFFFF)) & 0xFFFFFFFF, 4, g.rd_data(rt))
                return None
            return base(mm, pc, w)
        self.exec_ = exec_
        self.steps = 0

    def run(self, fn, args, stack_args=(), steps=3000000):
        m, img = self.m, self.img
        r = m.r
        for i, v in enumerate(args):
            r[4 + i] = v & 0xFFFFFFFF
        r[29], r[31] = 0x801FEF00, m.RET
        for i, v in enumerate(stack_args):
            m.wr(r[29] + 16 + 4 * i, 4, v)
        r[28] = img.link.gp_c
        pc, delay = img.sizes[fn][0], None
        word = img.link.word
        for n in range(steps):
            if pc == m.RET:
                self.steps += n
                return r[2]
            nxt = self.exec_(m, pc, word(pc))
            if delay is not None:
                pc, delay = delay, None
                continue
            if nxt is None:
                pc += 4
            else:
                delay, pc = nxt, pc + 4
        import bisect
        starts = sorted(img.names_at)
        at = starts[bisect.bisect_right(starts, pc) - 1]
        raise RuntimeError('%s: no return in %d steps (at %08X in %s)' % (fn, steps, pc, img.names_at[at]))


def emit_rot(rng):
    import math
    return far_rot(rng.uniform(-0.7, 0.7), rng.uniform(-math.pi, math.pi), rng.uniform(-0.3, 0.3))


def emit_disc_meshes(names):
    """{level: (origin, image bytes, [mesh nodes])} from the disc's level files (tools/dmd_lod_dump.py), or {}."""
    try:
        import dmd_lod_dump as L
        disc = L.Disc(L.DEFAULT_BIN)
    except Exception:
        return {}
    out = {}
    for name in names:
        try:
            dm = L.Dmd(name, disc.get('/DMD/%s.DMD' % name))
        except Exception:
            continue
        order, _ = dm.nodes()
        meshes = [p for p in order if dm.type(p) == 0 and dm.u32(dm.off(p) + 16) > 0]
        out[name] = (dm.origin, dm.d, dm, meshes)
    return out


def emit_scene(rng, disc, synth, n_entries, near_frac, precise, k_entries, end_room=None, cull_flip=0, far_copies=(),
               slot_change=0.0):
    """A made-up view for the emitter: (memory writes [(addr, bytes)], gte control regs, args, stack args, entries).
    Entries draw disc meshes (or `synth` meshes) at random places: a fraction near the eye (the near list), some with
    the mirror / fixed-otz / near-deferrable flags and OTZ overrides, keys and slots shared now and then, and
    k_entries of them as the GTE range pass's scaled entries (key bit 30 | k << 28)."""
    import math
    mem = []
    A = EMIT_BASE
    ot, pk = A, A + 0x4000                  # OT (4096 entries), packets
    pk_end = pk + 0x30000 if end_room is None else pk + end_room
    ctx = A + 0x40000
    ents = A + 0x40100
    rot = A + 0x48000                       # 64 slots
    lights = A + 0x49000
    lrec = A + 0x4A000                      # light records (otz override, record*), 32 bytes each
    nodes = A + 0x4C000                     # node header copies, 64 bytes each
    over = A + 0x50000                      # the near list's overflow area
    ot_b = bytearray()
    for i in range(4096):
        ot_b += ((ot + 4 * i + 4) & 0xFFFFFF | 0x00000000).to_bytes(4, 'little')
    mem.append((ot, bytes(ot_b)))
    mem.append((ctx, b''.join(v.to_bytes(4, 'little') for v in (ot, 0x1234, pk, pk_end))))
    # scratchpad: near threshold 70, overflow list, the two draw-mode packets, cull flip
    sp = bytearray(0x400)
    sp[0x40:0x44] = (70).to_bytes(4, 'little')
    sp[0x4C:0x50] = over.to_bytes(4, 'little')
    for i, v in enumerate((0x02000000, 0xE1000600, 0xE1000220, 0x02000000, 0xE1000640, 0xE1000260)):
        sp[0x60 + 4 * i:0x64 + 4 * i] = v.to_bytes(4, 'little')
    sp[0x88:0x8C] = cull_flip.to_bytes(4, 'little')
    mem.append((EMIT_SCRATCH, bytes(sp)))
    slots = []
    for s in range(64):
        R = emit_rot(rng)
        slots.append(R)
        b = b''.join(_u16(R[i // 3][i % 3]) for i in range(9)) + bytes(14)
        mem.append((rot + 32 * s, b))
        L = [[rng.randint(-4096, 4096) for _ in range(3)] for _ in range(3)]
        mem.append((lights + 32 * s, b''.join(_u16(L[i // 3][i % 3]) for i in range(9)) + bytes(14)))
    for j in range(8):
        rec = [rng.randint(0, 255) for _ in range(3)] + [rng.randint(-4096, 4096) for _ in range(9)]
        cm = b''.join(_u16(v) for v in rec[3:]) + bytes(2)
        mem.append((lrec + 0x800 + 32 * j, b''.join(v.to_bytes(4, 'little') for v in rec[:3]) + cm))
    entries, meta = [], []
    pool = []
    for name, (origin, data, dm, meshes) in disc.items():
        pool += [(name, p) for p in meshes]
    key, slot = 100, 0
    prev = None
    for i in range(n_entries):
        if synth and (not pool or rng.random() < 0.5):
            node, centre, radius, flags = synth[rng.randrange(len(synth))]
        else:
            name, p = pool[rng.randrange(len(pool))]
            dm = disc[name][2]
            o = dm.off(p)
            node = p
            centre = [_s32(dm.u32(o + 28 + 4 * j)) for j in range(3)]
            radius = _s32(dm.u32(o + 40))
            flags = None
        # a node header copy of our own: the flags (+0x1A) varied
        hdr = nodes + 64 * i
        if i < len(far_copies):
            node, centre, radius, flags = far_copies[i]
            raw = None
        elif flags is None:
            raw = bytearray(disc[name][1][disc[name][2].off(node):disc[name][2].off(node) + 44])
        else:
            raw = bytearray(synth_hdr(node))
        if raw is None:
            hdr = node                                  # a far copy: drawn as it is (its key below)
            raw = bytearray(synth_hdr(node))
        f = int.from_bytes(raw[0x1A:0x1C], 'little')
        roll = rng.random()
        if roll < 0.15:
            f |= 2                                      # mirrored
        elif roll < 0.25:
            f = (f & 7) | (rng.randint(1, 4000) << 3)   # fixed otz
        if rng.random() < 0.5:
            f |= 4                                      # near-deferrable
        raw[0x1A:0x1C] = (f & 0xFFFF).to_bytes(2, 'little')
        mem.append((hdr, bytes(raw)))
        if prev is not None and rng.random() < 0.15:
            t, sl, ky = prev                            # the same translation, slot and key as the last entry
            if rng.random() < slot_change:
                sl = rng.randrange(64)                  # (a new slot, the cached translation)
        else:
            sl = rng.randrange(64) if rng.random() < 0.8 else slot
            R = slots[sl]
            near = rng.random() < near_frac
            z = rng.uniform(300, 700) if near else math.exp(rng.uniform(math.log(800), math.log(60000)))
            pc = [z * rng.uniform(-0.5, 0.5), z * rng.uniform(-0.4, 0.4), z]
            ct = [int(round(sum(R[j][a] * pc[j] for j in range(3)) / 4096.0)) for a in range(3)]
            t = [ct[a] - centre[a] for a in range(3)]
            key += 1
            ky = key
        if i < len(far_copies):
            ky = 0x40000000 | far_copies[i][3] << 28 | i
        prev = (t, sl, ky)
        override = rng.randint(1, 4000) if rng.random() < 0.08 else 0
        lr = lrec + 0x800 + 32 * rng.randrange(8)
        mem.append((lrec + 8 * i, override.to_bytes(4, 'little') + lr.to_bytes(4, 'little')))
        e = [hdr, t[0], t[1], t[2], 3, lrec + 8 * i, sl, ky]
        entries.append(e)
        meta.append((hdr, node, t, sl, ky))
        mem.append((ents + 32 * i, b''.join((v & 0xFFFFFFFF).to_bytes(4, 'little') for v in e)))
    ctrl = {24: (160 << 16) + 0x8000 * 0, 25: 120 << 16, 26: EMIT_H, 27: -0x1000 & 0xFFFF, 28: 0x1400000}
    return mem, ctrl, (ctx, ents, len(entries), rot), (lights,), meta


def _u16(v):
    return (v & 0xFFFF).to_bytes(2, 'little')


SYNTH_BASE = EMIT_BASE + 0x100000


def synth_meshes(rng):
    """Made-up meshes of beams (type 1), point sprites (types 0, 2, 5) and a few textured / gouraud quads and
    triangles, at SYNTH_BASE: [(node, centre, radius, None)] and their bytes."""
    out, mem = [], []
    for n in range(8):
        node = SYNTH_BASE + 0x2000 * n
        verts, prims = node + 0x40, node + 0x400
        normals = node + 0x300
        span = 300 if n < 6 else 700        # meshes 6, 7: wide, beams and untextured triangles only (the near list
        vs = [[rng.randint(-span, span) for _ in range(3)] for _ in range(24)]   # and the t8 it keeps)
        mem.append((verts, b''.join(_u16(v[0]) + _u16(v[1]) + _u16(v[2]) + bytes(2) for v in vs)))
        mem.append((normals, b''.join(_u16(rng.randint(-4096, 4096)) + _u16(rng.randint(-4096, 4096)) +
                                      _u16(rng.randint(-4096, 4096)) + bytes(2) for _ in range(24))))
        pb = bytearray()
        count = 0
        for j in range(14):
            t = rng.choice((1, 1, 1, 0, 0, 2, 5, 3, 4) if n < 6 else (1, 3, 3))
            if t in (3, 4):
                # an untextured flat polygon: 1 colour, packet POLY_F3 / F4 (stride 4)
                ncol = 1
                words = 4 + ncol
                pkt = 4 if t == 3 else 5
                ctl = (t << 16) | (1 << 22)
                a = rng.randrange(24) | rng.randrange(24) << 16
                b = rng.randrange(24) | rng.randrange(24) << 16
                body = bytes((0x80 if rng.random() < 0.2 else 0, ncol, words, pkt)) + a.to_bytes(4, 'little') + \
                    b.to_bytes(4, 'little') + ctl.to_bytes(4, 'little') + (0x30405060).to_bytes(4, 'little')
            else:
                tex = rng.random() < 0.5
                ncol = 1
                ntex = 4 if tex else 0
                words = 4 + ncol + 1 + ntex
                stride = 8 if tex else 4
                pkt = 9 if tex else 5
                mode = 0 if rng.random() < 0.7 else 1
                ctl = (t << 16) | ((stride // 4) << 22) | (mode << 19) | (1 << 24)
                w = rng.choice((1, 3, 20, 43, 96, 344))
                h = rng.choice((5, 16, 50, 144, 200))
                body = bytes((ntex | (0x80 if rng.random() < 0.2 else 0), ncol, words, pkt)) + \
                    _u16(rng.randrange(24)) + _u16(w) + _u16(h) + _u16(0) + ctl.to_bytes(4, 'little') + \
                    (0x00808080 + j).to_bytes(4, 'little') + _u16(rng.randrange(24)) + _u16(0) + \
                    b''.join(rng.getrandbits(32).to_bytes(4, 'little') for _ in range(ntex))
            assert len(body) == words * 4, (t, len(body), words)
            pb += body
            count += 1
        mem.append((prims, bytes(pb)))
        hdr = bytearray(44)
        hdr[4:8] = verts.to_bytes(4, 'little')
        hdr[8:12] = normals.to_bytes(4, 'little')
        hdr[12:16] = prims.to_bytes(4, 'little')
        hdr[16:20] = count.to_bytes(4, 'little')
        hdr[40:44] = (400).to_bytes(4, 'little')
        mem.append((node, bytes(hdr)))
        SYNTH_HDR[node] = bytes(hdr)
        out.append((node, [0, 0, 0], 400, 0))
    return out, mem


SYNTH_HDR = {}


def synth_hdr(node):
    return SYNTH_HDR[node]


def emit_run(img, scene, disc_loaded, extra=(), precise=0, sym=None):
    """Run img's sub_800CDC54 on a scene; returns (the memory it wrote outside EMIT_ASM_ONLY: {addr: byte}, the ring
    entries, instructions)."""
    mem, ctrl, args, stack, meta = scene
    em = EmitMips(img)
    m = em.m
    for origin, data in disc_loaded:
        for k, b in enumerate(data):
            m.mem[origin + k] = b
    for a, b in list(extra) + list(mem):
        m.fill(a, b)
    if sym and 'portPreciseEmit' in sym:
        m.fill(sym['portPreciseEmit'], (precise & 0xFFFFFFFF).to_bytes(4, 'little'))
    m.fill(EMIT_RING, bytes(4))
    for r, v in ctrl.items():
        em.gte.wr_ctrl(r, v)
    m.writes.clear()
    em.run('sub_800CDC54', args, stack)
    out = {a: m.mem[a] for a in m.writes
           if not any(lo <= a < hi for lo, hi in EMIT_ASM_ONLY) and not EMIT_RING <= a < EMIT_RING + 0x10000 * 16}
    n = m.rd(EMIT_RING, 4)
    ring = [tuple(_s32(m.rd(EMIT_RING + 4 + 16 * i + 4 * j, 4)) for j in range(4)) for i in range(min(n, EMIT_RING_MAX))]
    return out, ring, em.steps, m


def emit_exact(scene_meta, img_mem, rotbase, sym, f):
    """The exact corners of every beam and point sprite of the scene, in float pixels: [(kind, ends or centre, half
    size(s))] for matching the ring's groups."""
    import math
    out = []
    last_key, tr = None, None
    for hdr, node, t, sl, ky in scene_meta:
        rd = lambda a, n: img_mem.rd(a, n)
        R = [[_s16(rd(rotbase + 32 * sl + 2 * (3 * a + b), 2)) for b in range(3)] for a in range(3)]
        if ky != last_key:              # the emitter rotates the translation only when the key changes
            tr = [sum(R[i][j] * t[j] for j in range(3)) / 4096.0 for i in range(3)]
            last_key = ky
        k = ((ky >> 28) & 3) if ky & 0x40000000 else 0
        verts, prims, n = rd(hdr + 4, 4), rd(hdr + 12, 4), rd(hdr + 16, 4)
        src = prims
        if k and sym and sym.get('portFarSprites') is not None:
            data = sym['portFarSprites'] + 0x2010          # struct PortFarSprites: 4 ints, 512 16-byte slots
            if data <= hdr < data + 131072:
                src = rd(hdr + 44, 4)
        p, q = prims, src
        for _ in range(n):
            ctl = rd(p + 12, 4)
            typ = (ctl >> 16) & 7
            if typ not in (3, 4):
                v = verts + _s16(rd(p + 4, 2)) * 8
                vx, vy, vz = _s16(rd(v, 2)), _s16(rd(v + 2, 2)), _s16(rd(v + 4, 2))
                w, h2 = _s16(rd(q + 6, 2)), _s16(rd(q + 8, 2))

                def proj(z):
                    c = [tr[i] + (R[i][0] * vx + R[i][1] * vy + R[i][2] * z) / 4096.0 for i in range(3)]
                    return 160 + EMIT_H * c[0] / c[2], 120 + EMIT_H * c[1] / c[2], c[2]
                if typ == 1:
                    ln = _s16(rd(p + 8, 2))
                    a, b = proj(_s16(vz + ln)), proj(_s16(vz - ln))
                    if a[2] > 0 and b[2] > 0:
                        out.append(('beam', a, b, w * EMIT_H / (a[2] * (1 << k)), w * 2 / (1 << k)))
                else:
                    c = proj(vz)
                    if c[2] > 0:
                        out.append(('sprite', c, None, (w * EMIT_H / (c[2] * (1 << k)), h2 * EMIT_H / (c[2] * (1 << k))),
                                    500))
            p += rd(p + 2, 1) * 4
            q += rd(q + 2, 1) * 4
    return out


def emit_size_err(ex, c, ff):
    """A ring group's side step / half sizes against an exact beam / sprite: [(got, want, tolerance)]."""
    import math
    if ex[0] == 'beam':
        a, b = ex[1], ex[2]
        dx, dy = a[0] - b[0], a[1] - b[1]
        q = min(ex[3], ex[4])
        L = math.hypot(dx, ff * dy)
        s, cc = (q * ff * ff * dy / L, q * dx / L) if L > 0 else (0.0, q)
        got = ((c[0][0] - c[1][0]) / 2, (c[1][1] - c[0][1]) / 2)
        # the direction comes from the two ends' 1/16-pixel places: off by about 1/8 px over the beam's length
        tol = EMIT_SIZE_REL * q + 0.125
        turn = q * min(2.0, 0.15 / max(math.hypot(dx, dy), 1e-6))
        side = math.hypot(got[0] / ff, got[1]) if ff else 0.0      # its length in the display's shape
        return [(got[0], s, tol + turn), (got[1], cc, tol + turn), (side, q, tol)]
    hw, hh = min(ex[3][0], 500) * ff, min(ex[3][1], 500)
    got = ((c[1][0] - c[0][0]) / 2, (c[2][1] - c[0][1]) / 2)
    return [(got[0], hw, EMIT_SIZE_REL * abs(hw) + 0.125), (got[1], hh, EMIT_SIZE_REL * abs(hh) + 0.125)]


def emit_check_ring(ring, exact, f):
    """Each ring group of four (one packet's corners) against the exact beams and sprites that project there (within
    EMIT_PRECISE_PX; of several, the one whose sizes fit best): (worst end px, worst size error, groups matched, bad)."""
    import math
    bad, worst_end, worst_size, matched = [], 0.0, 0.0, 0
    ff = f / 4096.0
    for g in range(0, len(ring) - 3, 4):
        c = [(ring[g + i][1] / 16.0, ring[g + i][2] / 16.0) for i in range(4)]
        if any(ring[g + i][0] != ring[g][0] + i * (ring[g + 1][0] - ring[g][0]) for i in range(4)):
            bad.append('ring group %d: addresses not one packet\'s corners' % (g // 4))
            continue
        e1 = ((c[0][0] + c[1][0]) / 2, (c[0][1] + c[1][1]) / 2)
        e2 = ((c[2][0] + c[3][0]) / 2, (c[2][1] + c[3][1]) / 2)
        mid = ((c[0][0] + c[3][0]) / 2, (c[0][1] + c[3][1]) / 2)
        best, bkey, nearest = None, None, 1e9
        for ex in exact:
            if ex[0] == 'beam':
                dd = max(math.hypot(ex[1][0] - e1[0], ex[1][1] - e1[1]), math.hypot(ex[2][0] - e2[0], ex[2][1] - e2[1]))
            else:
                dd = math.hypot(ex[1][0] - mid[0], ex[1][1] - mid[1])
            nearest = min(nearest, dd)
            # the budget: EMIT_PRECISE_PX, and near the eye the camera coordinates' 1/512 unit (a pixel's distance
            # from the centre over the depth, per unit of depth)
            if dd > EMIT_PRECISE_PX + 3 * (abs(ex[1][0] - 160) + abs(ex[1][1] - 120)) / (512.0 * ex[1][2]):
                continue
            errs = emit_size_err(ex, c, ff)
            key = max(abs(gv - wv) / tol for gv, wv, tol in errs)
            if bkey is None or key < bkey:
                best, bkey, bdd, berr = ex, key, dd, errs
        if best is None:
            bad.append('ring group %d: no beam or sprite projects there (nearest %.2f px; z %d, ends %s %s)' % (
                g // 4, nearest, ring[g][3], e1, e2))
            continue
        matched += 1
        worst_end = max(worst_end, bdd)
        worst_size = max(worst_size, max(abs(gv - wv) for gv, wv, tol in berr))
        if bkey > 1:
            bad.append('%s (ring group %d): %s, want %s (z %d, length %.2f px)' % (
                best[0], g // 4, ', '.join('%.3f' % e[0] for e in berr), ', '.join('%.3f' % e[1] for e in berr),
                ring[g][3], math.hypot(e1[0] - e2[0], e1[1] - e2[1])))
    return worst_end, worst_size, matched, bad


def emit_far_copies(rng, synth_list, smem, data, n):
    """n far copies of synth meshes in portFarSprites' data, as portFarSpriteMesh makes them (the node's first 44
    bytes, +12 the copy's primitives, +44 the source's, the sprites' and beams' sizes round(w / 2^k)):
    [(copy node, k, source node)] and their bytes."""
    out, mem = [], []
    for i in range(n):
        node, centre, radius, _ = synth_list[i % len(synth_list)]
        k = 1 + i % 3
        hdr = bytearray(SYNTH_HDR[node])
        prims, count = int.from_bytes(hdr[12:16], 'little'), int.from_bytes(hdr[16:20], 'little')
        src = dict((a, b) for a, b in smem)[prims]
        cp = bytearray(src)
        p = 0
        rnd = lambda v: (v + (1 << (k - 1))) >> k
        for _ in range(count):
            t = (int.from_bytes(cp[p + 12:p + 16], 'little') >> 16) & 7
            if t not in (3, 4):
                w = _s16(int.from_bytes(cp[p + 6:p + 8], 'little'))
                cp[p + 6:p + 8] = _u16(max(1, rnd(w)) if t == 1 and w > 0 else rnd(w))
                if t != 1:
                    cp[p + 8:p + 10] = _u16(rnd(_s16(int.from_bytes(cp[p + 8:p + 10], 'little'))))
            p += cp[p + 2] * 4
        at = data + 0x1000 * i
        hdr[12:16] = (at + 48).to_bytes(4, 'little')
        mem.append((at, bytes(hdr) + prims.to_bytes(4, 'little') + bytes(cp)))
        SYNTH_HDR[at] = bytes(hdr)
        out.append((at, centre, radius, k))
    return out, mem


NEAR_DEPTH_BYTES = 200 * 16           # portNearDepth (port_c_emitter.h): 200 entries of 4 words
# the asm near pass's register saves and leftovers (0x3C end, 0x44 / 0x48 frustum pointers, 0x50, 0x58, 0x78..0x8B
# the emitter's per-mesh fields and the cull flip): renderInitState, renderScene / renderDrawModel and the emitter set
# each of them again before reading it; the C keeps its state on the stack
NEAR_ASM_ONLY = [(0x1F800000, 0x1F800054), (0x1F800058, 0x1F80005C), (0x1F800078, 0x1F80008C), (0x801F0000, 0x801FF000)]
NEAR_MODEL, NEAR_OT, NEAR_SRC, NEAR_PKT, NEAR_LIST, NEAR_FACES = (0x80A80000, 0x80A81000, 0x80A84000, 0x80A90000,
                                                                  0x80AC0000, 0x80AC8000)


def near_scene(rng, count):
    """Made-up near list (request 82): count faces (triangles and quads, flat / flat-textured / gouraud /
    gouraud-textured, some with the texture-window flag or a mirrored OT index) and their emitted packets. Returns
    the memory to fill and [(face, packet, kind, stride, length, flag, otz)]."""
    mem, ents = [], []
    src, faces = NEAR_SRC, NEAR_FACES
    for k in range(count):
        kind = rng.choice((3, 4))
        T, C, st = rng.choice(((0, 1, 1), (kind, 1, 2), (0, kind, 2), (kind, kind, 3)))
        W = 1 if T and rng.random() < 0.2 else 0
        plen = {(3, 1): 4, (4, 1): 5, (3, 2): 7 if T else 6, (4, 2): 9 if T else 8, (3, 3): 9, (4, 3): 12}[(kind, st)]
        ctl = kind << 16 | st << 22 | (C + T + W) << 24 | C << 28 | rng.randrange(8) << 13
        face = bytes([T | (0x80 if W else 0), C, 4 + C + T + W, plen]) + bytes(8) + ctl.to_bytes(4, 'little')
        face += b''.join(rng.randrange(1 << 32).to_bytes(4, 'little') for _ in range(C + T + W))
        pkt = [rng.randrange(1 << 32) for _ in range(plen + 1)]
        for i in range(kind):           # screen places, -400..400 (s16 each)
            pkt[2 + i * st] = (rng.randrange(-400, 400) & 0xFFFF) | (rng.randrange(-300, 300) & 0xFFFF) << 16
        otz = rng.randrange(1, 70) * (-1 if rng.random() < 0.15 else 1)
        mem.append((faces, face))
        mem.append((src, b''.join(w.to_bytes(4, 'little') for w in pkt)))
        ents.append((faces, src, kind, st * 4, plen + 1, W, otz))
        faces += 64
        src += 4 * (plen + 1) + 8
    scratch = b''.join(f.to_bytes(4, 'little') + p.to_bytes(4, 'little') + bytes(2) + (o & 0xFFFF).to_bytes(2, 'little')
                       for f, p, _k, _s, _l, _w, o in ents[:71])
    over = b''.join(f.to_bytes(4, 'little') + p.to_bytes(4, 'little') + bytes(2) + (o & 0xFFFF).to_bytes(2, 'little')
                    for f, p, _k, _s, _l, _w, o in ents[71:])
    mem += [(0x1F8000A4, scratch), (NEAR_LIST, over),
            (NEAR_MODEL, b''.join(v.to_bytes(4, 'little') for v in (NEAR_OT, 0, NEAR_PKT, NEAR_PKT + 0x28000))),
            (NEAR_OT, b''.join((rng.randrange(1 << 24) | 0x80000000).to_bytes(4, 'little') for _ in range(128))),
            (0x1F800060, b''.join(rng.randrange(1 << 32).to_bytes(4, 'little') for _ in range(3))),
            (0x1F80006C, b''.join(rng.randrange(1 << 32).to_bytes(4, 'little') for _ in range(3))),
            (0x1F800054, rng.randrange(1 << 16).to_bytes(4, 'little'))]
    return mem, ents


def near_run(img, mem, n, precise=0, depths=(), sym=None):
    """renderNearPolys of img on a near scene: (the bytes it wrote outside NEAR_ASM_ONLY, the ring entries)."""
    em = EmitMips(img)
    m = em.m
    for a, b in mem:
        m.fill(a, b)
    if sym and 'portPreciseEmit' in sym:
        m.fill(sym['portPreciseEmit'], (precise & 0xFFFFFFFF).to_bytes(4, 'little'))
        for i, z in enumerate(depths):
            m.fill(sym['portNearDepth'] + 4 * i, (z & 0xFFFFFFFF).to_bytes(4, 'little'))
    m.fill(EMIT_RING, bytes(4))
    m.writes.clear()
    em.run('renderNearPolys', (NEAR_MODEL, NEAR_LIST, n))
    out = {a: m.mem[a] for a in m.writes if not any(lo <= a < hi for lo, hi in NEAR_ASM_ONLY) and
           not EMIT_RING <= a < EMIT_RING + 0x10000 * 16}
    cnt = m.rd(EMIT_RING, 4)
    ring = [(m.rd(EMIT_RING + 4 + 16 * i, 4),) + tuple(_s32(m.rd(EMIT_RING + 8 + 16 * i + 4 * j, 4)) for j in range(3))
            for i in range(min(cnt, EMIT_RING_MAX))]
    return out, ring


def near_expect(mem, ents, depths):
    """The ring entries request 82 asks for, from the packets' corners and the depths (port_c_emitter.h)."""
    words = {}
    for a, b in mem:
        for i in range(0, len(b) - 3, 4):
            words[a + i] = int.from_bytes(b[i:i + 4], 'little')

    def mid_z(a, b):
        if a <= 0 or b <= 0:
            return max(a, b)
        sh = 0
        while a > 0xFFFF or b > 0xFFFF:
            a, b, sh = a >> 1, b >> 1, sh + 1
        return ((a * b // (a + b)) * 2) << sh
    out, pkt = [], NEAR_PKT
    for k, (face, prim, kind, st, ln, flag, otz) in enumerate(ents):
        q = [prim, pkt, pkt + ln * 4, pkt + ln * 8]
        pkt = q[3] + ln * 4 + (24 if flag or otz < 0 else 0)
        z = depths[4 * k:4 * k + 4]
        if k >= 200 or min(z[:kind]) <= 0:
            continue
        pts = []
        for i in range(kind):
            w = words[prim + 8 + i * st]
            pts.append((_s16(w & 0xFFFF) * 16, _s16(w >> 16) * 16, z[i]))
        ends = ((0, 2), (1, 0), (1, 3), (3, 2), (4, 6)) if kind == 4 else ((0, 1), (1, 2), (2, 0))
        if kind == 3:
            pts.append(None)            # (the C's point 3: a triangle has no fourth corner)
        for a, b in ends:
            pts.append(((pts[a][0] + pts[b][0]) // 2, (pts[a][1] + pts[b][1]) // 2, mid_z(pts[a][2], pts[b][2])))
        rows = ((None, 5, 4, 8), (4, 8, 2, 7), (5, 1, 8, 6), (8, 6, 7, 3)) if kind == 4 else \
            ((None, 6, 4), (5, 1, 4), (5, 6, 2), (5, 6, 4))
        for j in range(4):
            for i, pt in enumerate(rows[j]):
                if pt is not None:
                    out.append((q[j] + 8 + i * st,) + pts[pt])
    return out


def check_near_pass(img_c, img_a, sym, rng):
    """Request 82: the C near pass (linked with TM2_PORT_C_EMITTER) against the asm (E0's renderNearPolys) on made-up
    near lists (up to 120 entries: the scratchpad's 71 and the overflow): every byte they write (the new packets, the
    OT links, the model's packet pointer, the scratchpad's texture page) is the same but the asm's register save area;
    with portPreciseEmit set and portNearDepth filled the integer bytes are those of the run without it and the
    precise ring holds exactly the entries near_expect gives (each written vertex word, its place, its depth)."""
    bad = []
    if 'renderNearPolys' not in img_a.sizes or 'renderNearPolys' not in img_c.sizes:
        return ['renderNearPolys not in the links'], 'no near pass'
    ca, sa = img_c.sizes['renderNearPolys'], img_a.sizes['renderNearPolys']
    if ca[1] == sa[1] and all(img_c.link.word(ca[0] + 4 * i) == img_a.link.word(sa[0] + 4 * i) for i in range(ca[1] // 4)):
        bad.append('the link\'s renderNearPolys is the asm (the C near pass is not linked)')
    if 'portNearDepth' not in sym:
        bad.append('portNearDepth not in the link')
        return bad, 'no portNearDepth'
    scenes = entries = 0
    for count in (5, 40, 71, 72, 120):
        mem, ents = near_scene(rng, count)
        oa, ra = near_run(img_a, mem, count)
        oc, rc = near_run(img_c, mem, count, sym=sym)
        diff = sorted(a for a in set(oa) | set(oc) if oa.get(a) != oc.get(a))
        if diff:
            bad.append('near scene of %d: %d bytes differ between the asm and the C (first %s)' % (
                count, len(diff), ', '.join('%08X asm %s C %s' % (a, oa.get(a), oc.get(a)) for a in diff[:3])))
        if rc:
            bad.append('near scene of %d: the ring written with portPreciseEmit 0' % count)
        depths = [rng.randrange(60, 4000) for _ in range(4 * count)]
        if count == 40:
            depths[4 * 3] = 0           # an entry without depths: not reported
            depths[4 * 7 + 1] = 70000   # a far corner (the harmonic mean's scaling)
        op, rp = near_run(img_c, mem, count, 4096, depths, sym)
        nd = sym['portNearDepth']
        d2 = sorted(a for a in set(op) | set(oc) if op.get(a) != oc.get(a) and not nd <= a < nd + NEAR_DEPTH_BYTES)
        if d2:
            bad.append('near scene of %d with portPreciseEmit: %d integer bytes differ (first %08X)' % (count, len(d2), d2[0]))
        want = near_expect(mem, ents, depths)
        if sorted(rp) != sorted(want):
            miss = sorted(set(want) - set(rp))
            extra = sorted(set(rp) - set(want))
            bad.append('near scene of %d: %d ring entries (want %d); missing %s, extra %s' % (
                count, len(rp), len(want), miss[:2], extra[:2]))
        scenes += 1
        entries += len(rp)
    return bad, ('the C near pass against the asm on %d near lists, %d split vertices reported' % (scenes, entries))


def check_c_emitter(out, link, sizes, e0, names, defined=True):
    """Check 8 (request 61 addendum): the C emitter (TM2_PORT_C_EMITTER) against the asm (E0's sub_800CDC54) on a MIPS
    interpreter with a GTE model, on made-up views of disc meshes (PARIS, FREEWAY) and made-up beam / sprite meshes:
    every byte they write (packets, OT links, the near list and its overflow, the scratchpad state, the context) is
    the same but the asm's register save area and spills; with portPreciseEmit set (4096 and 3072) the integer bytes
    are those of the run without it and the precise ring holds four corners per drawn beam and sprite, within
    EMIT_PRECISE_PX of the exact projection, their side steps and half sizes within EMIT_SIZE_REL of w H / (Z 2^k)
    (k from the key, the far copies' source sizes), the x one times the factor."""
    import random
    import time
    from check_rollback import Image
    bad = []
    t0 = time.time()
    if e0 is None or 'sub_800CDC54' not in sizes:
        print('8. the C emitter (request 61 addendum) -> FAIL: no E0 or no emitter')
        return ['no E0']
    img_c, img_a = Image(out), Image(e0)
    sym = {n: s['address'] for n, s in link.symbols.items()}
    ca, sa = img_c.sizes['sub_800CDC54'], img_a.sizes['sub_800CDC54']
    same = ca[1] == sa[1] and all(img_c.link.word(ca[0] + 4 * i) == img_a.link.word(sa[0] + 4 * i)
                                  for i in range(ca[1] // 4))
    if same:
        bad.append('the link\'s sub_800CDC54 is the asm (the C emitter is not linked)')
    if 'portPreciseEmit' not in sym:
        bad.append('portPreciseEmit not in the link')
    far = 'TM2_PORT_FAR_DISTANCE' in names and 'portFarSprites' in sym
    rng = random.Random(61)
    disc = emit_disc_meshes(['PARIS', 'FREEWAY'])
    synth, smem = synth_meshes(rng)
    copies, cmem = ([], [])
    if far:
        copies, cmem = emit_far_copies(rng, synth, smem, sym['portFarSprites'] + 0x2010, 6)
    extra = smem + cmem
    stats = dict(scenes=0, bytes=0, near=0, groups=0, diff=0)
    worst_end = worst_size = 0.0
    plans = [dict(n=60, near=0.3, cull=0), dict(n=60, near=0.3, cull=1), dict(n=40, near=0.1, cull=0, end=0x900),
             dict(n=60, near=0.0, cull=0), dict(n=80, near=0.5, cull=1), dict(n=60, near=0.2, cull=0),
             dict(n=60, near=0.8, cull=0, slots=0.7), dict(n=60, near=0.8, cull=1, slots=0.7)]
    levels = sorted(disc)
    for si, pl in enumerate(plans):
        # one level a scene (every level file is built for the same address); scene 2 made-up meshes only
        lv = {levels[si % len(levels)]: disc[levels[si % len(levels)]]} if levels and si != 2 else {}
        loaded = [(v[0], v[1]) for v in lv.values()]
        scene = emit_scene(rng, lv, synth, pl['n'], pl['near'], 0, 0,
                           end_room=pl.get('end'), cull_flip=pl['cull'], far_copies=copies if far else (),
                           slot_change=pl.get('slots', 0.0))
        oa, ra, na, ma = emit_run(img_a, scene, loaded, extra)
        oc, rc, nc, mc = emit_run(img_c, scene, loaded, extra, sym=sym)
        diff = sorted(a for a in set(oa) | set(oc) if oa.get(a) != oc.get(a))
        stats['scenes'] += 1
        stats['bytes'] += len(oa)
        stats['near'] += ma.rd(EMIT_SCRATCH + 0x38, 4)
        if diff:
            stats['diff'] += 1
            bad.append('scene %d: %d bytes differ between the asm and the C (first %s)' % (
                si, len(diff), ', '.join('%08X asm %s C %s' % (a, oa.get(a), oc.get(a)) for a in diff[:3])))
        if ra or rc:
            bad.append('scene %d: the precise ring written with portPreciseEmit 0' % si)
        if 'portPreciseEmit' not in sym:
            continue
        for f in (4096, 3072):
            op, rp, npp, mp = emit_run(img_c, scene, loaded, extra, precise=f, sym=sym)
            nd = sym.get('portNearDepth')       # request 82: the near entries' corner depths, written only with it
            d2 = sorted(a for a in set(op) | set(oc) if op.get(a) != oc.get(a) and
                        not (nd is not None and nd <= a < nd + NEAR_DEPTH_BYTES))
            if d2:
                bad.append('scene %d, portPreciseEmit %d: %d integer bytes differ from the run without it (first %08X)'
                           % (si, f, len(d2), d2[0]))
            ex = emit_exact(scene[4], mp, scene[2][3], sym if far else None, f)
            we, ws, matched, b2 = emit_check_ring(rp, ex, f)
            drawn = len(rp) // 4
            if len(rp) % 4 or matched != drawn:
                bad.append('scene %d, portPreciseEmit %d: %d ring entries, %d of the groups matched' % (
                    si, f, len(rp), matched))
            bad += ['scene %d, portPreciseEmit %d: %s' % (si, f, b) for b in b2[:4]]
            worst_end, worst_size = max(worst_end, we), max(worst_size, ws)
            stats['groups'] += drawn
    if stats['groups'] < 200 and 'portPreciseEmit' in sym:
        bad.append('only %d beams and sprites drawn with portPreciseEmit (want at least 200)' % stats['groups'])
    nb, nstat = check_near_pass(img_c, img_a, sym, rng)
    bad += nb
    if far and not copies:
        bad.append('no far copies made')
    print('8. the C emitter against the asm (request 61 addendum): %d scenes (%s), %d bytes written, %d near-list '
          'entries, %s; precise corners of %d beams and sprites: worst end %.3f px, worst side step or half size off by %.3f px '
          '(%.0fs); request 82: %s -> %s' % (stats['scenes'], ', '.join(disc) or 'no disc: made-up meshes only', stats['bytes'],
                             stats['near'], 'the bytes differ' if stats['diff'] else 'the same bytes', stats['groups'], worst_end, worst_size, time.time() - t0,
                             nstat, 'ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


# ---- check 9 (request 63, FAR_LOD.md 10): portVisual bit 5, the chase camera's eye kept inside the level ----
CAMWALL_PORT = {'portVisualCameraWall', 'portCameraWallRot', 'portCameraWallPlanes', 'portCameraWallFrac',
                'portCameraWallClip', 'portCameraWallNear', 'portCameraWallSqrt', 'portCameraWallWalk',
                'portCameraWallPull'}
CAMWALL_CALLERS = {'portVisualCameraWall': {'viewChaseCamera'},
                   'portCameraWallRot': {'portCameraWallPlanes', 'portCameraWallWalk'},
                   'portCameraWallPlanes': {'portCameraWallClip', 'portCameraWallWalk'},
                   'portCameraWallFrac': {'portCameraWallClip', 'portCameraWallPull'},
                   'portCameraWallClip': {'portCameraWallWalk'},
                   'portCameraWallNear': {'portCameraWallWalk'},
                   'portCameraWallSqrt': {'portCameraWallPull'},
                   'portCameraWallWalk': {'portVisualCameraWall'},     # request 100: the segment and the probes
                   'portCameraWallPull': {'portVisualCameraWall'}}
CAMWALL_BASE = 0x80A60000       # the made-up level model (above every link's port sections)
CAMWALL_EYE = 0x80A7F000        # the look-at point and the eye handed to the function
CAMWALL_TOL = 2                 # position units


def camwall_expect(look, eye, solids, marks, margin=48, near=64, clear=128, far=(), probes=True):
    """The float model. Request 63: the first point of look..eye inside a solid box (none that holds `look`) that no
    mark box covers; the eye pulled to margin in front of it, at least near from look. Request 100 (`probes`): from
    that eye a probe 2 * clear long each way along each axis against the solids (those holding `look` left out; one
    holding the eye counts from 0) less the marks, the free space h0 back and h1 forward; the eye moved to the middle
    when h0 + h1 < 2 * clear, else off the side nearer than clear. `far`: solids under a distance switch at the origin
    seen only from 800 out (its far record). Boxes: world-space planes [(n, d)] (n unit), inside n.p + d > 0."""
    def inside(planes, q):
        return all(sum(n[k] * q[k] for k in range(3)) + dd > 0 for n, dd in planes)

    def span(planes, a, b):
        t0, t1 = 0.0, 1.0
        fa_in = True
        for n, dd in planes:
            fa = sum(n[k] * a[k] for k in range(3)) + dd
            fb = sum(n[k] * b[k] for k in range(3)) + dd
            if fa > 0:
                if fb > 0:
                    continue
                t1 = min(t1, fa / (fa - fb))
            else:
                fa_in = False
                if fb <= 0:
                    return None
                t0 = max(t0, -fa / (fb - fa))
            if t0 >= t1:
                return None
        return (t0, t1, fa_in)

    def walk(a, b, probe):
        sel = solids + (list(far) if sum((int(a[k]) >> 3) ** 2 for k in range(3)) >= 10000 else [])
        best = 1.0
        ms = [x for x in (span(q, a, b) for q in marks) if x]
        for q in sel:
            x = span(q, a, b)
            if not x or (x[2] and not probe) or (probe and inside(q, look)):
                continue
            t = x[0]
            for _ in range(len(ms) + 1):
                for m0, m1, _in in ms:
                    if m0 <= t < m1:
                        t = m1
            if t <= x[1]:
                best = min(best, t)
        return best
    e = list(eye)
    best = walk(look, eye, False)
    d = [eye[k] - look[k] for k in range(3)]
    length = sum(x * x for x in d) ** 0.5
    if best < 1.0 and length > near:
        t = max(best - margin / length, near / length)
        if t < 1.0:
            e = [look[k] + d[k] * t for k in range(3)]
    if not probes:
        return e
    out = []
    for k in range(3):
        h = []
        for sign in (-1, 1):
            q = list(e)
            q[k] += sign * 2 * clear
            h.append(walk(e, q, True) * 2 * clear)
        if h[0] + h[1] < 2 * clear:
            t = (h[1] - h[0]) / 2
        elif h[0] < clear:
            t = clear - h[0]
        elif h[1] < clear:
            t = h[1] - clear
        else:
            t = 0
        out.append(e[k] + t)
    return out


def mips_load_port(m, link):
    """Put the link's port sections (.port_data, .port_text, .port_rodata: the switch tables a port function's jump
    table reads) into an interpreter's memory."""
    for a, data in link.port_mem:
        if len(data) <= 1 << 20:
            m.fill(a, data)


def check_camera_wall(out, link, sizes, kinds, callers, calls, pointed, relocs, rows, defined=True):
    """Check 9 (request 63): the port functions, their callers, no static store, and portVisualCameraWall on a made-up
    level model and on Moscow's (DISH, from the disc) in check_custom's FarMips."""
    from check_rollback import Image
    bad = []
    missing = sorted(n for n in CAMWALL_PORT if kinds.get(n) != 'port')
    if missing:
        bad.append('not in .port_text: %s' % ' '.join(missing))
    for fn, want in CAMWALL_CALLERS.items():
        got = callers.get(fn, set())
        if fn in kinds and not got <= want:
            bad.append('%s called by %s (want within %s)' % (fn, ' '.join(sorted(got)), ' '.join(sorted(want))))
        if fn in pointed:
            bad.append('%s pointed to' % fn)
    if 'portVisualCameraWall' in kinds and callers.get('portVisualCameraWall') != {'viewChaseCamera'}:
        bad.append('portVisualCameraWall not called by viewChaseCamera')
    for fn in CAMWALL_PORT & set(kinds):
        extra = set(calls.get(fn, ())) - CAMWALL_PORT - {'csGetRoot', 'renderGetWorldShift'}
        if extra:
            bad.append('%s calls %s' % (fn, ' '.join(sorted(extra))))
    # no store to a static word (the census: LO16 / GPREL16 relocations on store instructions) but the camera's own
    # work (request 89; request 100: its count of holds)
    spans = [sizes[f] for f in CAMWALL_PORT if f in sizes]
    for p_c, rtype, v_c, item, sname in relocs:
        if sname == 'portCameraWallWork':
            continue
        if rtype in (6, 7) and any(a <= p_c < a + n for a, n in spans):
            op = link.word(p_c) >> 26
            if op in (40, 41, 43):
                bad.append('static store at %08X (%s)' % (p_c, sname))
    if missing or 'csRootObject' not in link.symbols:
        print('9. camera wall (request 63) -> FAIL: %s' % ('; '.join(bad) or 'csRootObject not in the link'))
        return bad + ['missing']
    fm = FarMips(Image(out))
    m = fm.m
    mips_load_port(m, link)
    sym = lambda n: link.symbols[n]['address']
    B = CAMWALL_BASE

    def s16(v):
        return v & 0xFFFF

    def box(addr, planes_local, nid=0, sel=0, off=0):
        """kind 8 node at addr: planes_local [(row (3 floats, unit), offset)] up to 6 in the node's frame"""
        m.fill(addr, bytes(96))
        m.wr(addr, 1, 8)
        m.wr(addr + 4, 1, len(planes_local))
        m.wr(addr + 5, 1, sel & 0xFF)
        m.wr(addr + 7, 1, off)
        for i, (row, d) in enumerate(planes_local):
            mo, to = (16, 36) if i < 3 else (48, 68)
            r = i % 3
            for c in range(3):
                m.wr(addr + mo + 2 * (3 * r + c), 2, s16(int(round(row[c] * 4096))))
            m.wr(addr + to + 4 * r, 4, int(d) & 0xFFFFFFFF)
        return addr

    def aabb(lo, hi):
        """six planes of an axis box lo..hi (local frame)"""
        return [((1, 0, 0), -lo[0]), ((0, 1, 0), -lo[1]), ((0, 0, 1), -lo[2]),
                ((-1, 0, 0), hi[0]), ((0, -1, 0), hi[1]), ((0, 0, -1), hi[2])]

    def group(addr, kids, flags=4, centre=(0, 0, 0), radius=0):
        m.fill(addr, bytes(24 + 4 * len(kids)))
        m.wr(addr, 1, 1)
        for k in range(3):
            m.wr(addr + 4 + 4 * k, 4, centre[k] & 0xFFFFFFFF)
        m.wr(addr + 16, 4, radius)
        m.wr(addr + 20, 1, len(kids))
        m.wr(addr + 21, 1, flags)
        for i, k in enumerate(kids):
            m.wr(addr + 24 + 4 * i, 4, k)
        return addr
    world_solid, world_mark = [], []
    big = 100000
    # B1: x 1000..1100 under a translation by (1000, 0, 0)
    b1 = box(B + 0x1000, aabb((0, -big, -big), (100, big, big)))
    t4 = B + 0x0200
    m.fill(t4, bytes(24))
    m.wr(t4, 1, 4)
    m.wr(t4 + 4, 4, 1000)
    m.wr(t4 + 18, 1, 1)
    m.wr(t4 + 20, 4, b1)
    world_solid.append(aabb((1000, -big, -big), (1100, big, big)))
    # the door: a mark volume x 900..1200, y -50..50, z -50..50
    door = box(B + 0x1100, aabb((900, -50, -50), (1200, 50, 50)), sel=-2)
    world_mark.append(aabb((900, -50, -50), (1200, 50, 50)))
    # B2 under a rotation: local = R (p - (1550, 0, 0)), R rows (0 1 0) (-1 0 0) (0 0 1): world x 1500..1600
    b2 = box(B + 0x1200, aabb((-big, -50, -big), (big, 50, big)))
    r5 = B + 0x0300
    m.fill(r5, bytes(44))
    m.wr(r5, 1, 5)
    for i, v in enumerate((0, 4096, 0, -4096, 0, 0, 0, 0, 4096)):
        m.wr(r5 + 4 + 2 * i, 2, s16(v))
    m.wr(r5 + 24, 4, 1550)
    m.wr(r5 + 38, 1, 1)
    m.wr(r5 + 40, 4, b2)
    world_solid.append(aabb((1500, -big, -big), (1600, big, big)))
    # B4 under a level change 3 -> 2 (coordinates halved): world x 2200..2300
    b4 = box(B + 0x1300, aabb((1100, -big // 2, -big // 2), (1150, big // 2, big // 2)))
    l12 = B + 0x0400
    m.fill(l12, bytes(12))
    m.wr(l12, 1, 12)
    m.wr(l12 + 4, 2, 2)
    m.wr(l12 + 6, 2, 1)
    m.wr(l12 + 8, 4, b4)
    world_solid.append(aabb((2200, -big, -big), (2300, big, big)))
    # a distance switch whose near record (d2 < 100^2 world) is empty and far one holds B3 (x 300..400): not tested
    b3 = box(B + 0x1400, aabb((300, -big, -big), (400, big, big)))
    empty = group(B + 0x0500, [])
    lod = B + 0x0600
    m.fill(lod, bytes(28))
    m.wr(lod, 1, 2)
    m.wr(lod + 16, 4, 2)
    m.wr(lod + 20, 4, B + 0x0700)
    m.wr(lod + 24, 4, B + 0x0710)
    for a, mx, mn, ch in ((B + 0x0700, 100 * 100, 0, empty), (B + 0x0710, 0xFFFFFFFF, 100 * 100, b3)):
        m.wr(a, 4, mx)
        m.wr(a + 4, 4, mn)
        m.wr(a + 8, 4, ch)
    # a sphere away from the segment (centre y 5000, radius 100) over a box that would cross it (x 500..600)
    b5 = box(B + 0x1500, aabb((500, -big, -big), (600, big, big)))
    far = group(B + 0x0800, [b5], flags=0, centre=(0, 5000, 0), radius=100)
    # a box switched off (+7 = 1) at x 200..250, and one holding the look-at points (skipped)
    off = box(B + 0x1600, aabb((200, -big, -big), (250, big, big)), off=1)
    hold = box(B + 0x1700, aabb((-200, -300, -300), (60, 300, 300)))
    world_solid.append(aabb((-200, -300, -300), (60, 300, 300)))
    # request 100: a tunnel 150 wide (walls y -4100..-4000 and -3850..-3750) and a wall alone (y -5100..-5000), x
    # -3000..-1000, away from everything else
    walls = []
    for i, (y0, y1) in enumerate(((-4100, -4000), (-3850, -3750), (-5100, -5000))):
        walls.append(box(B + 0x1800 + 0x80 * i, aabb((-3000, y0, -big), (-1000, y1, big))))
        world_solid.append(aabb((-3000, y0, -big), (-1000, y1, big)))
    root = group(B + 0x0100, [t4, door, r5, l12, lod, far, off, hold] + walls)
    world_far = [aabb((300, -big, -big), (400, big, big))]
    obj = B + 0x2000
    m.fill(obj, bytes(0x120))
    m.wr(obj + 284, 4, root)
    m.wr(sym('csRootObject'), 4, obj)
    if 'renderWorldShift' in link.symbols:
        m.wr(sym('renderWorldShift'), 4, 3)
    vis = sym('portVisual')
    work = sym('portCameraWallWork')    # request 89: its pending nodes and spans (were its stack)
    cases = [((0, 0, 0), (2000, 0, 0)), ((0, 0, 0), (1300, 300, 0)), ((0, 100, 0), (2400, 100, 0)),
             ((1700, 0, 0), (2400, 0, 0)), ((0, 100, 0), (800, 100, 0)), ((950, 100, 0), (1300, 100, 0)),
             ((0, 0, 0), (-900, 40, 30)), ((40, 0, 0), (1450, -400, 200)),
             ((-1500, -3925, 0), (-2200, -3980, 40)), ((-1500, -4800, 0), (-2300, -4920, 60))]
    worst = 0.0
    moved = 0
    clear = int(re.search(r'#define TM2_VIS_CAMERA_CLEAR\s+(\d+)',
                          (ROOT / 'include/semantic/port_visual.h').read_text(encoding='utf-8')).group(1))

    def run(look, eye, bit):
        m.wr(vis, 4, 0x20 if bit else 0)
        for k in range(3):
            m.wr(CAMWALL_EYE + 4 * k, 4, look[k] & 0xFFFFFFFF)
            m.wr(CAMWALL_EYE + 16 + 4 * k, 4, eye[k] & 0xFFFFFFFF)
        m.writes.clear()
        fm.run('portVisualCameraWall', (CAMWALL_EYE, CAMWALL_EYE + 16))
        stray = sorted(a for a in m.writes if not (0x801F0000 <= a < 0x801FF000 or
                                                     CAMWALL_EYE + 16 <= a < CAMWALL_EYE + 28 or
                                                     work <= a < work + 0x1000))
        if stray:
            bad.append('writes outside the stack, its work and the eye (first %08X)' % stray[0])
        got = [_s32(m.rd(CAMWALL_EYE + 16 + 4 * k, 4)) for k in range(3)]
        if [_s32(m.rd(CAMWALL_EYE + 4 * k, 4)) for k in range(3)] != list(look):
            bad.append('the look-at point changed')
        return got
    for look, eye in cases:
        got = run(look, eye, True)
        want = camwall_expect(look, eye, world_solid, world_mark, clear=clear, far=world_far)
        r63 = camwall_expect(look, eye, world_solid, world_mark, clear=clear, far=world_far, probes=False)
        # request 100: the clearance moves the eye at most `clear` along each axis from request 63's
        if max(abs(got[k] - r63[k]) for k in range(3)) > clear + CAMWALL_TOL:
            bad.append('look %s eye %s: %s, more than the clearance from request 63\'s %s' % (
                look, eye, got, [round(x) for x in r63]))
        err = max(abs(got[k] - want[k]) for k in range(3))
        worst = max(worst, err)
        if got != list(eye):
            moved += 1
        if err > CAMWALL_TOL:
            bad.append('look %s eye %s: %s (want %s)' % (look, eye, got, [round(x) for x in want]))
        if run(look, eye, False) != list(eye):
            bad.append('look %s eye %s: moved with the bit clear' % (look, eye))
    if moved < 5:
        bad.append('only %d of %d made-up cases moved the eye (want at least 5)' % (moved, len(cases)))
    # request 100: in the tunnel the eye goes to the middle; beside the wall alone it is pushed off it sideways,
    # its distance to the look-at point kept (request 96 pulled it most of the way to the car)
    got = run((-1500, -3925, 0), (-2200, -3980, 40), True)
    if abs(got[1] + 3925) > CAMWALL_TOL:
        bad.append('the tunnel: the eye at y %d (want the middle, -3925)' % got[1])
    got = run((-1500, -4800, 0), (-2300, -4920, 60), True)
    if abs(got[1] - (-5000 + clear)) > CAMWALL_TOL or abs(got[0] + 2300) > CAMWALL_TOL:
        bad.append('the wall alone: the eye at %s (want x -2300, y %d)' % (got, -5000 + clear))
    # Moscow's model from the disc, at its own address: a camera spun outside the fence comes back inside
    real = ''
    try:
        import dmd_lod_dump as L
        dmd = L.Dmd('DISH', L.Disc(L.DEFAULT_BIN).get('/DMD/DISH.DMD'))
        m.fill(dmd.origin, dmd.d)
        m.wr(obj + 284, 4, dmd.roots[0])
        n_in = n_out = 0
        for look, eye in (((3000, 0, 700), (3700, 0, 800)), ((0, 3100, 700), (300, 3800, 760)),
                          ((-2200, -2200, 700), (-2700, -2700, 760)), ((-3000, 400, 700), (-3800, 500, 760))):
            got = run(look, eye, True)
            r = max(abs(got[0]), abs(got[1]), (abs(got[0]) + abs(got[1])) * 0.7071)
            if r >= 3432:
                bad.append('DISH: look %s eye %s -> %s still outside the fence' % (look, eye, got))
            else:
                n_out += 1
        # (request 96: eyes more than TM2_VIS_CAMERA_CLEAR from every solid; the first was (0, 0, 700) to (-300, 200,
        # 760), 16 from the side and 40 over the top of the box at x -476..-316, z 520..720: pulled in now)
        for look, eye in (((800, 800, 700), (1100, 600, 760)), ((1500, -500, 700), (1800, -900, 760))):
            got = run(look, eye, True)
            if got != list(eye):
                bad.append('DISH: look %s eye %s moved to %s (no wall within the clearance)' % (look, eye, got))
            else:
                n_in += 1
        real = '; DISH: %d eyes outside the fence brought in, %d inside kept' % (n_out, n_in)
    except (OSError, KeyError, ValueError) as e:
        bad.append('DISH from the disc: %s' % e)
    print('9. camera wall (requests 63, 100): %d made-up cases (%d moved; the tunnel centred, the wall alone '
          'pushed off sideways), worst %.1f units%s -> %s' % (
        len(cases), moved, worst, real, 'ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


# ---- checks 10 and 11 (requests 71 and 70, docs/CAR_PROFILES.md) ----
CARSEL_LOAD_PORT = {'portCarSelLoad', 'portCarSelUnload', 'portCarSelTextures', 'portCarSelPlace', 'portCarSelUpload'}
CARSEL_LOAD_CALLERS = {'portCarSelLoad': set(),            # the port's API
                       'portCarSelUnload': {'portCarSelLoad'},     # and the port's API
                       'portCarSelTextures': {'portCarSelLoad'},
                       'portCarSelPlace': {'portCarSelTextures'},
                       'portCarSelUpload': {'portCarSelTextures'}}
CARSEL_LOAD_BYTES = 0x203C0         # struct TM2PortCarSelLoad
CARSEL_BANK = 124                   # TM2_PORT_CARSEL_BANK
CARSEL_DMD_AT, CARSEL_TMS_AT = 0x80E00000, 0x80E40000
CARSEL_RETAIL_CHILD = 0x80123400    # made-up retail children 0..13 of the test carousel
CARSEL_HEAD_ID, CARSEL_HEAD_Z = 249, 240    # request 74: Dark Tooth's head root, its height over the truck
CARSEL_DT_PHASE2, CARSEL_PHASE2_Z = 0x10000, -90   # request 118: index flag, the head alone (car_numbers.h)
PROFILE_PORT = {'portCarProfileOfSlot', 'portCarProfileSlotOf', 'portCarProfileStats', 'portCarProfileStep',
                'portCarProfileCamera', 'portCarProfileWeapons', 'portCarProfileTopSpeed', 'portCarProfileRadar',
                'portCarProfileName'}
PROFILE_CALLERS = {
    'portCarProfileStats': {'carStatsLoad'},
    'portCarProfileStep': {'carWheelsSetStepHeight'},
    'portCarProfileCamera': {'viewCameraOffsetsInit'},
    'portCarProfileWeapons': {'weaponBlockInit'},
    'portCarProfileTopSpeed': {'aiAdvanceRouteNode', 'aiSetSegmentSpeed', 'aiInputFinalize', 'humanCarTick'},
    'portCarProfileRadar': {'hudRadarPlotCar'},
    'portCarProfileName': {'hudDrawTargetPanel'},
    'portCarProfileSlotOf': {'portCarProfileStats', 'portCarProfileStep'},
    'portCarProfileOfSlot': {'portCarProfileStats', 'portCarProfileStep', 'portCarProfileCamera',
                             'portCarProfileWeapons', 'portCarProfileTopSpeed', 'portCarProfileRadar',
                             'portCarProfileName'},    # and the port's API
}
EXPECTED_PROFILE = {'carStatsLoad', 'carWheelsSetStepHeight', 'viewCameraOffsetsInit', 'weaponBlockInit',
                    'aiAdvanceRouteNode', 'aiSetSegmentSpeed', 'aiInputFinalize', 'humanCarTick', 'hudRadarPlotCar',
                    'worldUpdate', 'hudDrawTargetPanel'}
PROFILE_BYTES = 0xA0                # struct TM2PortCarProfile
# check 13 (request 73): a mod car's special
SPECIAL_PORT = {'portSpecialTemplate', 'portSpecialNum', 'portSpecialFireBegin', 'portSpecialFireEnd',
                'portSpecialFirePickup', 'portSpecialAiType', 'portSpecialAiTables', 'portSpecialOf',
                'portSpecialIsPickup', 'portSpecialApply', 'portSpecialAmmo',
                # stage 2
                'portSpecialSlotOfId', 'portSpecialNumById', 'portSpecialBlast', 'portSpecialBoomerangCaught',
                'portSpecialRicochet', 'portSpecialBomb', 'portSpecialSound'}
SPECIAL_CALLERS = {
    'portSpecialFireBegin': {'weaponFireSpecial'}, 'portSpecialFireEnd': {'weaponFireSpecial'},
    'portSpecialFirePickup': {'weaponFireSpecial'},
    'portSpecialAiType': {'aiCarSetupProfile'}, 'portSpecialAiTables': {'aiCarSetupProfile'},
    'portSpecialNum': {'weaponFireSpecial', 'weaponFlameStart', 'weaponTaserTarget', 'portSpecialFireEnd',
                       'weaponArcsDraw', 'areaDamage', 'portSpecialNumById', 'portSpecialBlast', 'portSpecialRicochet'},
    'portSpecialSlotOfId': {'portSpecialNumById'},
    'portSpecialNumById': {'projectileHitTest', 'weaponFlameUpdate', 'weaponProjectileSteer', 'weaponProjectileUpdate',
                           'weaponShadowStrikeUpdateOne'},
    'portSpecialBlast': {'areaDamage'},
    'portSpecialBoomerangCaught': {'weaponProjectileUpdate'},
    'portSpecialRicochet': {'portSpecialFirePickup'}, 'portSpecialBomb': {'portSpecialFirePickup'},
    'portSpecialSound': {'portSpecialRicochet', 'portSpecialBomb'},
    'portSpecialTemplate': {'portCarSpecialAsSet', 'portSpecialApply', 'portSpecialAiType', 'portSpecialAiTables'},
    'portSpecialApply': {'portCarSpecialAsBlockInit'},
    'portSpecialAmmo': {'portCarSpecialAsBlockInit', 'portCarSpecialAsSet'},
}
EXPECTED_SPECIAL = {'weaponFireSpecial', 'weaponFlameStart', 'weaponTaserTarget', 'aiCarSetupProfile',
                    # stage 2
                    'weaponArcsDraw', 'areaDamage', 'projectileHitTest', 'weaponFlameUpdate', 'weaponProjectileSteer',
                    'weaponProjectileUpdate'}
SPECIAL_BYTES = 0x48                # struct TM2PortSpecialProfile
# check 12 (request 72): portVisual bit 6, the hidden areas
HIDDEN_PORT = {'portHiddenAreasBegin', 'portHiddenAreasEnd', 'portHiddenChurch', 'portHiddenPatch',
               'portHiddenRoofBuild', 'portHiddenRoofMesh', 'portHiddenRoofMove', 'portHiddenAreasDrawn',
               'portHiddenTemple', 'portHiddenZoneIs', 'portHiddenBurb',   # requests 97, 119
               'portHiddenSubway', 'portHiddenKidIs', 'portHiddenRecIs', 'portHiddenOpen',     # request 104
               'portTwoSidedBegin', 'portTwoSidedEnd'}                                         # request 113
HIDDEN_CALLERS = {'portHiddenAreasBegin': {'renderScene'}, 'portHiddenAreasEnd': {'renderScene'},
                  'portHiddenChurch': {'portHiddenAreasBegin'},
                  'portHiddenPatch': {'portHiddenChurch', 'portHiddenTemple', 'portHiddenSubway', 'portHiddenOpen', 'portHiddenBurb'},
                  'portHiddenTemple': {'portHiddenAreasBegin'}, 'portHiddenBurb': {'portHiddenAreasBegin'},
                  'portHiddenZoneIs': {'portHiddenTemple', 'portHiddenSubway', 'portHiddenBurb'},
                  'portHiddenSubway': {'portHiddenAreasBegin'}, 'portHiddenKidIs': {'portHiddenSubway'},
                  'portHiddenRecIs': {'portHiddenSubway'}, 'portHiddenOpen': {'portHiddenSubway'},
                  'portHiddenRoofBuild': {'portHiddenChurch'}, 'portHiddenRoofMesh': {'portHiddenRoofBuild'},
                  'portHiddenRoofMove': {'portHiddenAreasBegin', 'portHiddenAreasDrawn'},
                  'portHiddenAreasDrawn': {'renderScene'},
                  'portTwoSidedBegin': {'renderScene'}, 'portTwoSidedEnd': {'portHiddenAreasDrawn'}}    # request 113
HIDDEN_BYTES = 0x1D0                # struct TM2PortHiddenAreas (request 104: 32 words, the subway's groups)
HIDDEN_LEVEL_AT = 0x80019C40        # the level file's address (its pointers are built for it)
# request 97 (HIDDEN_AREAS.md 14): Hong Kong's temple: the room's zone and its two children (the city, the inside); the
# children of the three doorway zones (the cards, the stand-ins close to the doors) and the passages' stand-ins
HIDDEN_TEMPLE = dict(room=0x80065664, city=0x80055270, inside=0x800655FC,
                     doors=(0x800635D0, 0x80063F50, 0x80063FC8, 0x800644C0, 0x80064538, 0x80064ADC),
                     passages=(0x80048C84, 0x80048D0C, 0x80048D94))
# per Paris file: zone A, the inside's nave, the stand-in, the near / mid / far wall meshes, the door's two far meshes
# (intact, broken), the far wall record (the one portFarLodBegin extends), the door's select node
HIDDEN_CHURCH = {
    'PARIS': dict(place=1, zone=0x8002E5E4, nave=0x8002C524, stand=0x8002C0CC, near=0x8002B8EC, mid=0x8002B674,
                  far=0x8002B5D0, door0=0x8002E5F8, door1=0x8002EB04, far_rec=0x8002B668, door=0x8002EEF0),
    'PARIS2': dict(place=2, zone=0x8002E79C, nave=0x8002C74C, stand=0x8002C2F4, near=0x8002BB14, mid=0x8002B89C,
                   far=0x8002B7F8, door0=0x8002E7B0, door1=0x8002ECBC, far_rec=0x8002B890, door=0x8002F0A8),
}


class LinkMips:
    """FarMips across every function of the link, with Python stubs by function name (their link symbols), and the
    link's own bytes (its .data, .rodata and port sections) under every address the run has not written."""

    def __init__(self, img, stubs):
        self.fm = FarMips(img)
        self.m, self.img, self.stubs = self.fm.m, img, stubs
        m, link = self.m, img.link

        def rd(a, n):
            v = 0
            for k in range(n):
                b = m.mem.get(a + k)
                if b is None:
                    b = link.mem(a + k, 1)[0]
                v |= b << (8 * k)
            return v
        m.rd = rd
        self.starts = {link.symbols[n]['address']: n for n in stubs if n in link.symbols}

    def run(self, fn, args=(), steps=60000000):
        m, img = self.m, self.img
        r = m.r
        r[29], r[31] = 0x801FF000, m.RET
        for i, v in enumerate(args):
            if i < 4:
                r[4 + i] = v & 0xFFFFFFFF
            else:                       # the o32 stack arguments, above the 16-byte home area
                m.wr(r[29] + 4 * i, 4, v & 0xFFFFFFFF)
        r[28] = img.link.gp_c
        pc, delay = img.sizes[fn][0], None
        for _ in range(steps):
            if pc == m.RET:
                return r[2]
            nxt = self.fm.exec_(m, pc, img.link.word(pc))
            if delay is not None:
                pc, delay = delay, None
                if pc in self.starts:
                    m.set(2, self.stubs[self.starts[pc]](m) or 0)
                    pc = r[31]
                continue
            if nxt is None:
                pc += 4
            else:
                delay, pc = nxt, pc + 4
        raise RuntimeError('%s: no return in %d steps' % (fn, steps))


def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def weapon_icon_run(out):
    """Request 81: portWeaponIcon in LinkMips on each arena file (PICKUP_LOOKS.md 11)."""
    import struct
    from check_rollback import Image
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN
    img = Image(out)
    syms = img.link.symbols
    need = ('weaponMineObjects', 'weaponFreezeMissileModel')
    if 'portWeaponIcon' not in img.sizes or any(n not in syms for n in need):
        print('2. request 81 weapon icons in the link -> FAIL: portWeaponIcon / %s missing' % ' / '.join(need))
        return False
    mines, freeze = syms['weaponMineObjects']['address'], syms['weaponFreezeMissileModel']['address']
    obj, res = 0x80A70000, 0x80A70200
    disc = Disc(DEFAULT_BIN)
    bad, done = [], 0
    for name in PICKUP_SPECIAL_LEVELS:
        d = Dmd(name, disc.get('/DMD/%s.DMD' % name)).d

        def w(a):
            return struct.unpack_from('<I', d, a - HIDDEN_LEVEL_AT)[0]

        def b(a):
            return d[a - HIDDEN_LEVEL_AT]

        def h(a):
            return struct.unpack_from('<h', d, a - HIDDEN_LEVEL_AT)[0]
        ids = {}
        for o in range(0, len(d) - 24, 4):
            i = struct.unpack_from('<H', d, o + 2)[0]
            if d[o] == 1 and 1 <= d[o + 20] <= 4 and i in (2202, 2212) and i not in ids:
                ids[i] = HIDDEN_LEVEL_AT + o

        def want(node, frame):
            """The model's largest sprite, walked as portWeaponIcon does, from the file."""
            for _ in range(8):
                t = b(node)
                if t == 0:
                    break
                node = (w(node + 24) if t == 1 else w(w(node + 20) + 8) if t == 2 else
                        w(node + 32 + (frame % b(node + 11)) * 4) if t == 3 else 0)
                if not node:
                    return None
            prim, best = w(node + 12), None
            for _ in range(w(node + 16)):
                if (w(prim + 12) >> 16) & 7 <= 1 and b(prim) & 0x7F == 4 and (best is None or h(prim + 6) > h(best + 6)):
                    best = prim
                prim += b(prim + 2) * 4
            tex = [w(best + 16 + b(best + 1) * 4 + 4 * k) for k in range(4)]
            us, vs = [t & 0xFF for t in tex], [(t >> 8) & 0xFF for t in tex]
            return (tex[1] >> 16, tex[0] >> 16, min(us), min(vs), max(us) - min(us) + 1, max(vs) - min(vs) + 1,
                    w(best + 16))
        if set(ids) != {2202, 2212}:
            bad.append('%s: the file has %s (want 2202 and 2212)' % (name, sorted(ids)))
            continue
        for case in ((0, 0), (1, 0), (1, 5), (1, 16), ('none', 0)):
            lm = LinkMips(img, {})
            m = lm.m
            m.fill(HIDDEN_LEVEL_AT, d)
            m.wr(obj + 0x11C, 4, ids[2212])
            m.wr(mines, 4, 0 if case[0] == 'none' else obj)
            m.wr(freeze, 4, 0 if case[0] == 'none' else ids[2202])
            for k in range(0, 12, 4):
                m.wr(res + k, 4, 0)
            which = 0 if case[0] in (0, 'none') else 1
            rv = s32(lm.run('portWeaponIcon', (which, case[1], res)))
            got = (m.rd(res, 2), m.rd(res + 2, 2), m.rd(res + 4, 1), m.rd(res + 5, 1), m.rd(res + 6, 1),
                   m.rd(res + 7, 1), m.rd(res + 8, 4))
            if case[0] == 'none':
                if rv != 0 or any(got):
                    bad.append('%s: with no models: returned %d, wrote %s' % (name, rv, got))
                continue
            exp = want(ids[2212] if which == 0 else ids[2202], case[1])
            if rv != 1 or got != exp:
                bad.append('%s %s frame %d: returned %d, %s (want 1, %s)' % (
                    name, 'mine' if which == 0 else 'freeze', case[1], rv,
                    '%04X %04X uv %d,%d %dx%d %08X' % got, exp and '%04X %04X uv %d,%d %dx%d %08X' % exp))
            else:
                done += 1
    good = not bad and done == 4 * len(PICKUP_SPECIAL_LEVELS)
    print('2. request 81: the mine\'s and the freeze\'s icons read from the loaded models on %d of %d arenas, 0 '
          'without them -> %s' % (done // 4, len(PICKUP_SPECIAL_LEVELS), 'ok' if good else 'FAIL'))
    for x in bad[:12]:
        print('     ' + x)
    return good


def pickup_special_run(out):
    """Request 79: portPickupSpecialBuild in LinkMips on each arena file (PICKUP_LOOKS.md 10)."""
    import struct
    from check_rollback import Image
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN
    img = Image(out)
    syms = img.link.symbols
    bad = []
    if 'portPickupSpecialBuild' not in img.sizes or 'portPickupSpecial' not in syms or 'pickupObjects' not in syms:
        print('2. request 79 pickup look in the link -> FAIL: portPickupSpecialBuild / portPickupSpecial / '
              'pickupObjects missing')
        return False
    ps, po = syms['portPickupSpecial']['address'], syms['pickupObjects']['address']
    disc = Disc(DEFAULT_BIN)
    weapons = {401, 403, 404, 406, 411, 413, 419}

    def sx(v):
        return v - 0x10000 if v & 0x8000 else v

    built = 0
    for name in PICKUP_SPECIAL_LEVELS:
        dmd = Dmd(name, disc.get('/DMD/%s.DMD' % name)).d
        nodes, hud = [], 0
        for o in range(0, len(dmd) - 28, 4):
            t, i = dmd[o], struct.unpack_from('<H', dmd, o + 2)[0]
            if t == 1 and i == 1260 and dmd[o + 20] >= 5:
                hud = HIDDEN_LEVEL_AT + o
            if t in (1, 4) and 401 <= i <= 430:
                ch = struct.unpack_from('<I', dmd, o + (24 if t == 1 else 20))[0]
                if HIDDEN_LEVEL_AT <= ch < HIDDEN_LEVEL_AT + len(dmd) and len(nodes) < 50:
                    nodes.append((HIDDEN_LEVEL_AT + o, i))
        for case in ('level', 'no hud', 'no weapon'):
            lm = LinkMips(img, {})
            m = lm.m
            m.fill(HIDDEN_LEVEL_AT, dmd)
            for k in range(50):
                m.wr(po + 4 * k, 4, 0)
            for k, (a, i) in enumerate(nodes):
                if case != 'no weapon' or i not in weapons:
                    m.wr(po + 4 * k, 4, a)
            m.wr(ps + 4, 4, 0 if case == 'no hud' else hud)
            lm.run('portPickupSpecialBuild')
            model, donor = m.rd(ps, 4), m.rd(ps + 8, 4)
            if m.rd(ps + 4, 4) != 0:
                bad.append('%s (%s): the HUD object not forgotten' % (name, case))
            if bytes(m.rd(HIDDEN_LEVEL_AT + k, 1) for k in range(len(dmd))) != dmd:
                bad.append('%s (%s): the level file changed' % (name, case))
            if case != 'level':
                if model != 0:
                    bad.append('%s (%s): a model %08X (want 0)' % (name, case, model))
                continue
            if not hud or model != ps + 0x0C:
                bad.append('%s: model %08X, HUD object %08X (want %08X)' % (name, model, hud, ps + 0x0C))
                continue
            built += 1

            def beam(mesh, icon, w, h, rgb):
                """The beam mesh's one primitive against the HUD icon mesh's first."""
                prim = m.rd(mesh + 12, 4)
                words = [m.rd(prim + 4 * k, 4) for k in range(9)]
                ip = m.rd(icon + 12, 4)
                c = m.rd(ip + 1, 1)
                tex = [m.rd(ip + 16 + 4 * c + 4 * k, 4) for k in range(4)]
                us, vs = [t & 0xFF for t in tex], [(t >> 8) & 0xFF for t in tex]
                u0, u1, v0, v1 = min(us), max(us), min(vs), max(vs)
                want = [0x09090104, w << 16, h, 0x15810000, 0x2C000000 | rgb,
                        u0 | v0 << 8 | (tex[0] & 0xFFFF0000), u1 | v0 << 8 | (tex[1] & 0xFFFF0000),
                        u0 | v1 << 8, u1 | v1 << 8]
                vert = m.rd(mesh + 4, 4)
                return (m.rd(mesh, 1) == 0 and m.rd(mesh + 16, 4) == 1 and words == want and
                        sx(m.rd(vert + 4, 2)) == sx(m.rd(mesh + 36, 4) & 0xFFFF))
            g = model
            card, hit = m.rd(g + 24, 4), m.rd(g + 28, 4)
            problems = []
            if m.rd(g, 1) != 1 or m.rd(g + 20, 1) != 2:
                problems.append('the group')
            if not beam(card, m.rd(hud + 40, 4), 32, 32, 0x808080):
                problems.append('the card')
            if hit != m.rd(donor + 24 + 4 * (m.rd(donor + 20, 1) - 1), 4) or m.rd(hit, 1) != 8:
                problems.append('the hit box')
            if not any(m.rd(n + (24 if m.rd(n, 1) == 1 else 20), 4) == donor and i in weapons for n, i in nodes):
                problems.append('the donor %08X is no weapon pickup\'s model' % donor)
            if problems:
                bad.append('%s: %s' % (name, ', '.join(problems)))
    good = not bad and built == len(PICKUP_SPECIAL_LEVELS)
    print('2. request 79: the special\'s pickup look built on %d of %d arenas (card, hit box), none without '
          'the HUD object or a weapon pickup -> %s' % (built, len(PICKUP_SPECIAL_LEVELS), 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


def taser_start(out):
    """Check 2, request 76: weaponArcsDraw's first segment for a taser (kind 0) and a lightning (kind 1) arc, the
    shooter's body matrix a quarter turn (the car facing -x), portVisual bit 1 set and clear. The view: an eye 6000
    back along +y (mathMulTransVec and mathMulMatVec as Python stubs: the GTE's rows times the vector, >> 12)."""
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    if 'portVisual' not in syms or 'weaponArcs' not in syms:
        print('2. the taser start: portVisual / weaponArcs not in the link -> FAIL')
        return False
    hdr = (ROOT / 'include/semantic/port_visual.h').read_text(encoding='utf-8')
    sy = int(re.search(r'#define TM2_VIS_TASER_SIREN_Y \((-?\d+)\)', hdr).group(1))
    sz = int(re.search(r'#define TM2_VIS_TASER_SIREN_Z (\d+)', hdr).group(1))
    S = 0x801E0000
    EYE, MAT, MOTION, TO, CTX, OT, BUF = S, S + 0x40, S + 0x100, S + 0x80, S + 0xC0, S + 0x400, S + 0x1000
    pos = (1000, 2000, 300)
    turn = ((0, -4096, 0), (4096, 0, 0), (0, 0, 4096))     # body_m: local +y (the front) to world -x

    def mul(m, a, b, tr):
        v = [s32(m.rd(a + 4 * k, 4)) for k in range(3)]
        mat = [[cc_s16(m.rd(MOTION + 0x70 + 2 * (3 * r + c), 2)) for c in range(3)] for r in range(3)] if tr else None
        if tr:      # mathMulTransVec: the transpose times the vector
            o = [sum(mat[r][c] * v[r] for r in range(3)) >> 12 for c in range(3)]
        else:       # the view: a camera looking along +y, z up
            o = [v[0], -v[2], v[1]]
        for k in range(3):
            m.wr(b + 4 * k, 4, o[k] & 0xFFFFFFFF)

    def first(bit, kind):
        seed = [7]
        prims = []

        def rand(m):
            seed[0] = (seed[0] * 1103515245 + 12345) & 0xFFFFFFFF
            return (seed[0] >> 16) & 0x7FFF

        def centre(m):
            m.wr(m.r[5], 4, 160)
            m.wr(m.r[6], 4, 120)

        def addprim(m):
            prims.append(tuple(sext16(m.rd(m.r[5] + 8 + 2 * k, 2)) for k in range(4)))

        stubs = {'rand': rand, 'viewGetCentre': centre, 'viewGetProjScaleX': lambda m: 256,
                 'viewGetProjScaleY': lambda m: 256, 'viewGetEyeTrans': lambda m: EYE,
                 'viewGetViewMatrix': lambda m: MAT, 'AddPrim': addprim, 'fxSmokeC12': lambda m: 0,
                 'fxExplosion12': lambda m: 0, 'carApplyDamage': lambda m: 0,
                 'mathMulMatVec': lambda m: mul(m, m.r[5], m.r[6], False),
                 'mathMulTransVec': lambda m: mul(m, m.r[5], m.r[6], True)}
        lm = LinkMips(img, stubs)
        m = lm.m
        m.wr(syms['portVisual']['address'], 4, 2 if bit else 0)
        for k, v in enumerate((0, 6000, 0)):
            m.wr(EYE + 4 * k, 4, v & 0xFFFFFFFF)
        for k, v in enumerate(pos):
            m.wr(MOTION + 0x20 + 4 * k, 4, v & 0xFFFFFFFF)
        for r in range(3):
            for c in range(3):
                m.wr(MOTION + 0x70 + 2 * (3 * r + c), 2, turn[r][c] & 0xFFFF)
        for k, v in enumerate((pos[0] + 300, pos[1] + 400, pos[2])):
            m.wr(TO + 4 * k, 4, v & 0xFFFFFFFF)
        a = syms['weaponArcs']['address']
        for off, v in ((0, 0), (2, 0), (4, 20), (6, kind), (8, 1)):
            m.wr(a + off, 2, v)
        m.wr(a + 12, 4, MOTION + 0x20)
        m.wr(a + 16, 4, TO)
        m.wr(CTX, 4, OT)
        m.wr(CTX + 8, 4, BUF)
        m.wr(CTX + 12, 4, BUF + 0x2000)
        lm.run('weaponArcsDraw', (CTX, 0))
        return prims[0][:2] if prims else None

    def screen(p):      # weaponArcsDraw's projection (C division, toward zero)
        x, y, z = p[0], p[1] + 6000, p[2]
        return 160 + int(x * 256 / y), 120 + int(-z * 256 / y)

    world = [pos[0] + sum(turn[r][0] * (0, sy, sz)[r] for r in range(3)) // 4096,
             pos[1] + sum(turn[r][1] * (0, sy, sz)[r] for r in range(3)) // 4096,
             pos[2] + sum(turn[r][2] * (0, sy, sz)[r] for r in range(3)) // 4096]
    want = {(1, 0): screen(world), (0, 0): screen((pos[0], pos[1], pos[2] + 30)),
            (1, 1): screen((pos[0], pos[1], pos[2] + 30)), (0, 1): screen((pos[0], pos[1], pos[2] + 30))}
    bad = []
    for (bit, kind), w in sorted(want.items()):
        got = first(bit, kind)
        if got != w:
            bad.append('bit %d kind %d: first point %s (want %s)' % (bit, kind, got, w))
    print('2. the taser start (request 76): bit 1 at the light bar %s, else and the lightning at + 30 -> %s' % (
        want[(1, 0)], 'ok' if not bad else 'FAIL: ' + '; '.join(bad)))
    return not bad


def cc_s16(v):
    return v - 0x10000 if v & 0x8000 else v


def carsel_tms(data):
    """The TMS's images as portCarSelTextures reads them: (mode, pixel block offset, w, h, CLUT block offset or
    None)."""
    import struct
    n = struct.unpack_from('<I', data, 12)[0]
    o, out = 16 + 4 * n, []
    for _ in range(n):
        size = struct.unpack_from('<I', data, o)[0]
        t = o + 4
        flags = struct.unpack_from('<I', data, t + 4)[0]
        q, clut = t + 8, None
        if flags & 8:
            clut = q
            q += struct.unpack_from('<I', data, q)[0]
        w, h = struct.unpack_from('<HH', data, q + 8)
        out.append((flags & 7, q, w, h, clut))
        o = t + size
    return out


def carsel_meshes(m, roots):
    """Every mesh node (type 0) reachable from the relocated roots, walked as modelRelocate pushes children."""
    seen, st, out = set(), list(roots), []
    while st:
        p = st.pop()
        if not p or p in seen:
            continue
        seen.add(p)
        t = m.rd(p, 1)
        if t == 0:
            out.append(p)
        elif t in (1, 4, 5, 9, 12):
            cnt = {1: m.rd(p + 20, 1), 4: m.rd(p + 18, 1), 5: m.rd(p + 38, 1), 9: m.rd(p + 6, 1),
                   12: m.rd(p + 6, 2)}[t]
            base = {1: 24, 4: 20, 5: 40, 9: 16, 12: 8}[t]
            st += [m.rd(p + base + 4 * i, 4) for i in range(cnt)]
        elif t == 2:
            for i in range(m.rd(p + 16, 4)):
                st.append(m.rd(m.rd(p + 20 + 4 * i, 4) + 8, 4))
        elif t == 3:
            st += [m.rd(p + 32 + 4 * i, 4) for i in range(m.rd(p + 11, 1))]
        elif t == 11:
            st += [m.rd(p + 8, 4), m.rd(p + 12, 4)]
    return out


def check_carsel_load(out, banks):
    """Check 10 (request 71): portCarSelLoad in LinkMips (LoadImage and DrawSync stubs) on the disc's car files."""
    import struct
    from check_rollback import Image
    bad = []
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    if any(n not in sizes for n in CARSEL_LOAD_PORT) or 'portCarSelLoads' not in syms or 'portCarSel' not in syms:
        print('10. car select models (request 71) -> FAIL: portCarSelLoad or its storage is not in the link')
        return ['missing']
    try:
        from dmd_lod_dump import Disc, DEFAULT_BIN
        disc = Disc(DEFAULT_BIN)
    except OSError:
        print('10. car select models (request 71): no disc image in cdimage/: not run')
        return []
    loads_at, sel_at = syms['portCarSelLoads']['address'], syms['portCarSel']['address']
    bank_word = syms['portVramUploadBank']['address'] if banks else None
    uploads = []

    def load_image(m):
        x, y, w, h = (m.rd(m.r[4] + 2 * k, 2) for k in range(4))
        uploads.append((x, y, w, h, m.r[5], m.rd(bank_word, 4) if bank_word else 0))
        return 0
    lm = LinkMips(img, {'LoadImage': load_image, 'DrawSync': lambda m: 0})
    m = lm.m
    count, retail = 0x22, 14
    blank = sel_at + 0x10
    m.wr(sel_at + 4, 4, count)
    m.wr(sel_at + 0xC, 4, retail)
    m.wr(sel_at + 0x30, 1, 9)
    m.wr(sel_at + 0x30 + 6, 1, count)
    for i in range(count):
        m.wr(sel_at + 0x30 + 16 + 4 * i, 4, CARSEL_RETAIL_CHILD + i if i < retail else blank)
    for k in range(4):
        m.wr(loads_at + CARSEL_LOAD_BYTES * k, 4, 0xFFFFFFFF)
    child = lambda i: m.rd(sel_at + 0x30 + 16 + 4 * i, 4)
    slot_index = lambda k: s32(m.rd(loads_at + CARSEL_LOAD_BYTES * k, 4))
    sentinels = {'vramTexSlots': 0x8099AA00, 'vramTexSlotGroup': 7}
    if banks:
        sentinels['portTexBanks'] = 0       # +0 live: banks are not live in the shell
    for n, v in sentinels.items():
        m.wr(syms[n]['address'], 4, v)
    cases = [(0, 0x20, 'SP'), (1, 0x21, 'BB'), (2, 14, 'MN'), (3, 0x20, 'RK'),
             (1, 0x21 | CARSEL_DT_PHASE2, 'BB')] if banks else \
            [(0, 0x20, 'SP'), (0, 0x21, 'RK')]
    textured = 0
    dt_heads = 0
    files = {}
    for k, flagged, car in cases:
        phase2, index = flagged & CARSEL_DT_PHASE2, flagged & ~CARSEL_DT_PHASE2    # request 118
        dmd, tms = disc.get('/DMD/%s.DMD' % car), disc.get('/TMS/%s.TMS' % car)
        files[car] = (dmd, tms)
        m.fill(CARSEL_DMD_AT, dmd)
        m.fill(CARSEL_TMS_AT, tms)
        del uploads[:]
        prev = [slot_index(j) for j in range(4)]
        rv = s32(lm.run('portCarSelLoad', (k, flagged, CARSEL_DMD_AT, len(dmd), CARSEL_TMS_AT, len(tms))))
        tag = '%s in slot %d as child 0x%X%s' % (car, k, index, ' (phase 2)' if phase2 else '')
        if rv != 1:
            bad.append('%s: returned %d' % (tag, rv))
            continue
        L = loads_at + CARSEL_LOAD_BYTES * k
        buf = L + 0x3C0
        bank = CARSEL_BANK + k if banks else 0
        imgs = carsel_tms(tms)
        area = (0, 0, 1024, 480, 0, 480, 1024, 512) if banks else (832, 256, 960, 512, 768, 40, 1024, 64)
        # the uploads, in portCarSelTextures's order: each image's pixels, then its CLUT's first row
        want_n = len(imgs) + sum(1 for i in imgs if i[4] is not None)
        if len(uploads) != want_n:
            bad.append('%s: %d uploads (want %d)' % (tag, len(uploads), want_n))
            continue
        if any(u[5] != bank for u in uploads):
            bad.append('%s: an upload outside bank %d' % (tag, bank))
        it = iter(uploads)
        px, cl = [], []
        for i, (mode, q, w, h, clut) in enumerate(imgs):
            x, y, uw, uh, data, _ = next(it)
            px.append((x, y, w, h))
            lim = (64, 128, 256)[mode]
            if (uw, uh) != (w, h) or data != CARSEL_TMS_AT + q + 12:
                bad.append('%s: image %d uploaded %dx%d from %08X (want %dx%d from its TIM)' % (tag, i, uw, uh,
                                                                                             data, w, h))
            if not (area[0] <= x and x + w <= area[2] and area[1] <= y and y + h <= area[3]) or \
                    x % 64 + w > lim or (y & 0xFF) + h > 256:
                bad.append('%s: image %d at (%d, %d) %dx%d outside the area or its page' % (tag, i, x, y, w, h))
            for j, a in enumerate(px[:-1]):
                if x < a[0] + a[2] and a[0] < x + w and y < a[1] + a[3] and a[1] < y + h:
                    bad.append('%s: images %d and %d overlap' % (tag, j, i))
            want = [(x >> 6) | ((y >> 8) << 4) | (bank << 9), x, y, 0, 0]
            if clut is not None:
                cx, cy, cw, ch, cdata, _ = next(it)
                if cdata != CARSEL_TMS_AT + clut + 12 or (cw, ch) != ((16 if mode == 0 else 256), 1) or \
                        not (area[4] <= cx and cx + cw <= area[6] and area[5] <= cy < area[7]) or cx % 16:
                    bad.append('%s: image %d CLUT %dx%d at (%d, %d) from %08X' % (tag, i, cw, ch, cx, cy, cdata))
                for a in cl:
                    if a[1] == cy and cx < a[0] + a[2] and a[0] < cx + cw:
                        bad.append('%s: image %d CLUT overlaps another' % (tag, i))
                cl.append((cx, cy, cw))
                want[3:] = [cx, cy]
            sl = [s32(m.rd(L + 0x10 + 20 * i + 4 * f, 4)) for f in range(5)]
            if sl != want:
                bad.append('%s: slot %d = %s (want %s)' % (tag, i, sl, want))
        # the child: the file's car root(s), relocated in the slot
        nroot = struct.unpack_from('<I', dmd, 0x18)[0]
        roots = [struct.unpack_from('<I', dmd, 0x1C + 4 * i)[0] - 0x80019C40 for i in range(nroot)]
        cars = [o for o in roots if struct.unpack_from('<H', dmd, o + 2)[0] in range(0x10, 0xF1, 0x10)]
        heads = [o for o in roots if struct.unpack_from('<H', dmd, o + 2)[0] == CARSEL_HEAD_ID]
        dt_head = len(cars) == 1 and struct.unpack_from('<H', dmd, cars[0] + 2)[0] == 0xF0 and heads
        want_child = buf + cars[0] if len(cars) == 1 and not dt_head else L + 0x390
        if dt_head and phase2:  # request 118: the transform node alone (no truck, no group), the head at -90
            off = buf + ((len(dmd) + 3) & ~3)
            want_child = off
            got = (m.rd(off, 1), [(m.rd(off + 4 + 2 * i, 2) ^ 0x8000) - 0x8000 for i in range(9)],
                   [s32(m.rd(off + 24 + 4 * i, 4)) for i in range(3)], m.rd(off + 38, 1), m.rd(off + 40, 4))
            want = (5, [-4096, 0, 0, 0, -4096, 0, 0, 0, 4096], [0, 0, CARSEL_PHASE2_Z], 1, buf + heads[0])
            if got != want:
                bad.append('%s: the head alone %s (want %s)' % (tag, got, want))
            else:
                dt_heads += 1
        elif dt_head:   # request 74: the group holds the truck and a node with the head 240 above it; request 109:
            # a transform node (5), a half turn about z (the head's face is its -y)
            off = buf + ((len(dmd) + 3) & ~3)
            got = (m.rd(L + 0x390, 1), m.rd(L + 0x390 + 20, 1), m.rd(L + 0x390 + 21, 1), m.rd(L + 0x390 + 24, 4),
                   m.rd(L + 0x390 + 28, 4), m.rd(off, 1), [(m.rd(off + 4 + 2 * i, 2) ^ 0x8000) - 0x8000 for i in range(9)],
                   [s32(m.rd(off + 24 + 4 * i, 4)) for i in range(3)], m.rd(off + 36, 2),
                   m.rd(off + 38, 1), m.rd(off + 39, 1), m.rd(off + 40, 4))
            want = (1, 2, 4, buf + cars[0], off, 5, [-4096, 0, 0, 0, -4096, 0, 0, 0, 4096],
                    [0, 0, CARSEL_HEAD_Z], 0, 1, 0, buf + heads[0])
            if got != want:
                bad.append('%s: the head group %s (want %s)' % (tag, got, want))
            else:
                dt_heads += 1
        if child(index) != want_child or slot_index(k) != index or m.rd(L + 0x3B8, 4) != want_child:
            bad.append('%s: child %08X, slot index %d (want %08X, %d)' % (tag, child(index), slot_index(k),
                                                                          want_child, index))
        for j in range(4):
            if j != k and slot_index(j) != prev[j]:
                bad.append('%s: slot %d changed' % (tag, j))
        for n, v in sentinels.items():
            if m.rd(syms[n]['address'], 4) != v:
                bad.append('%s: %s not restored' % (tag, n))
        # every textured primitive of every root: page bits, bank bits, CLUT and UVs from the slot table
        for node in carsel_meshes(m, [buf + o for o in roots]):
            if m.rd(node + 1, 1) != 0xB1:
                bad.append('%s: mesh %08X not relocated' % (tag, node))
                continue
            p, cnt = m.rd(node + 12, 4), m.rd(node + 16, 4)
            for _ in range(cnt):
                hdr, step = m.rd(p, 1), m.rd(p + 2, 1) * 4
                if hdr & 0x7F:
                    rq = p + 16 + ((m.rd(p + 15, 1) >> 2) & 0x1C)
                    w = rq + 4
                    t = dmd[w + 2 - buf] & 0x1F
                    tp = (struct.unpack_from('<I', dmd, w - buf)[0] >> 23) & 3
                    sl = [s32(m.rd(L + 0x10 + 20 * t + 4 * f, 4)) for f in range(5)]
                    half = m.rd(w + 2, 2)
                    u = sl[1] % 64 * (4, 2, 1, 1)[tp]
                    v = sl[2] & 0xFF
                    good = t < len(imgs) and (half & 0x1F) == (sl[0] & 0x1F) and \
                        half >> 9 == (bank if banks else dmd[w + 3 - buf] >> 1) and \
                        m.rd(rq + 2, 2) == ((sl[3] >> 4) & 0x3F) | ((sl[4] & 0x3FF) << 6)
                    if good and not hdr & 0x80:
                        good = m.rd(rq, 1) == (dmd[rq - buf] + u) & 0xFF and \
                            m.rd(rq + 1, 1) == (dmd[rq + 1 - buf] + v) & 0xFF
                    if not good:
                        bad.append('%s: primitive %08X (image %d) remapped wrong' % (tag, p, t))
                        break
                    textured += 1
                p += step
    # the slot API: an unload empties only its own child; a reload frees the slot's old child; a retail index fails
    if banks and not bad:
        lm.run('portCarSelUnload', (2,))
        if child(14) != blank or slot_index(2) != -1:
            bad.append('portCarSelUnload(2): child 14 %08X, slot index %d' % (child(14), slot_index(2)))
        dmd, tms = files['SP']
        m.fill(CARSEL_DMD_AT, dmd)
        m.fill(CARSEL_TMS_AT, tms)
        rv = s32(lm.run('portCarSelLoad', (3, 0x21, CARSEL_DMD_AT, len(dmd), CARSEL_TMS_AT, len(tms))))
        if rv != 1 or child(0x20) != blank:
            bad.append('slot 3 reloaded as 0x21: %d, its old child 0x20 %08X (want the empty node)' % (rv, child(0x20)))
        mine3 = child(0x21)
        lm.run('portCarSelUnload', (1,))
        if child(0x21) != mine3 or slot_index(1) != -1:
            bad.append('portCarSelUnload(1) changed slot 3\'s child 0x21')
        del uploads[:]
        rv = s32(lm.run('portCarSelLoad', (0, 13, CARSEL_DMD_AT, len(dmd), CARSEL_TMS_AT, len(tms))))
        if rv != -2 or uploads or child(13) != CARSEL_RETAIL_CHILD + 13:
            bad.append('child 13 (a retail car): returned %d, %d uploads' % (rv, len(uploads)))
        rv = s32(lm.run('portCarSelLoad', (0, 0x20, CARSEL_DMD_AT, len(dmd), CARSEL_TMS_AT, 15)))
        if rv != -4 or child(0x20) != blank:
            bad.append('a cut TMS: returned %d, child 0x20 %08X' % (rv, child(0x20)))
    if textured < 100:
        bad.append('only %d textured primitives checked' % textured)
    if dt_heads != sum(1 for c in cases if c[2] == 'BB'):
        bad.append("Dark Tooth's head placed in %d of the BB loads" % dt_heads)
    print('10. car select models (requests 71, 74): %d loads (%s), %d textured primitives remapped through the slot '
          "tables, Dark Tooth's head on his truck %d -> %s" % (len(cases), 'banked' if banks else 'free block',
                                                                textured, dt_heads, 'ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


def check_profile(out):
    """Check 11 (request 70): the profile hooks in LinkMips on a made-up mod on Spectre for human record 0 and AI
    record 1, against the same calls without a valid profile."""
    from check_rollback import Image
    bad = []
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    need = ['carStatsLoad', 'carWheelsSetStepHeight', 'viewCameraOffsetsInit', 'weaponBlockInit',
            'portCarProfileTopSpeed', 'portCarProfileRadar', 'portCarProfileName']
    if any(n not in sizes for n in need) or 'portCarProfile' not in syms:
        print('11. mod car profiles (request 70) -> FAIL: the hooks or portCarProfile are not in the link')
        return ['missing']
    lm = LinkMips(img, {})
    m = lm.m
    hum, ai = syms['humanCars']['address'], syms['aiCars']['address'] + 0x854
    prof = syms['portCarProfile']['address']
    mod, slot_h, slot_a, grp_h, grp_a = 5, 3, 4, 2, 6
    m.wr(syms['portCarGroups']['address'] + 4 * slot_h, 4, grp_h)
    m.wr(syms['portCarGroups']['address'] + 4 * slot_a, 4, grp_a)
    for g in (grp_h, grp_a):
        m.wr(syms['portCarNumbers']['address'] + 2 * g, 2, 0x20 + mod + 1)
    m.wr(syms['portModCars']['address'] + 0x20 * mod, 1, 1)
    m.wr(syms['portModCars']['address'] + 0x20 * mod + 1, 1, 7)       # base Spectre (car 7, type 0x80)
    m.wr(hum + 4, 2, slot_h)
    m.wr(ai + 4, 2, 1)
    m.wr(ai + 6, 2, slot_a)
    stats = [(0x52, 2, 150), (0x3A, 2, 140), (0x48, 2, 90), (0x4E, 2, 70), (0x50, 2, -80), (0x42, 2, 30),
             (0x46, 2, 50), (0xAC, 4, 12), (0x7A, 2, 40), (0x8E, 2, 50),
             (0xA4, 4, 5), (0x52, 4, 1), (0x4A, 2, 9)]             # the last three must be refused
    P = prof + PROFILE_BYTES * mod
    m.wr(P + 1, 1, len(stats))
    m.wr(P + 2, 1, 7)
    m.wr(P + 3, 1, 0x22)                                  # cam[1] (chase) and cam[5] (split mode 2)
    m.wr(P + 4, 2, 0x70)
    m.wr(P + 6, 2, 500)
    m.wr(P + 8, 2, 30)
    m.wr(P + 0xA, 1, 4)
    for i, c in enumerate((10, 200, 30)):
        m.wr(P + 0xB + i, 1, c)
    m.wr(P + 0x10, 4, 0x80E80000)
    cam = lambda v, i: 100 * v + 10 * i - 500
    for v in range(6):
        for i in range(3):
            m.wr(P + 0x14 + 6 * v + 2 * i, 2, cam(v, i) & 0xFFFF)
    for i, (o, w, v) in enumerate(stats):
        m.wr(P + 0x40 + 4 * i, 1, o)
        m.wr(P + 0x41 + 4 * i, 1, w)
        m.wr(P + 0x42 + 4 * i, 2, v & 0xFFFF)
    cam_at = (0x80, 0x44, 0x50, 0x5C, 0x68, 0x74)

    def run_all(valid):
        m.wr(P, 1, valid)
        m.wr(hum + 0x90, 4, 0x80)
        m.wr(ai + 0x30, 4, 0x80)
        res = {}
        for name, rec, st, wh, wp, human in (('human', hum, hum + 0x14C, hum + 0x5A4, hum + 0x7C8, 1),
                                             ('ai', ai, ai + 0x4FC, ai + 0x5E4, ai + 0x808, 0)):
            m.fill(st, bytes(0xE8))
            lm.run('carStatsLoad', (st, 0x80))
            lm.run('carWheelsSetStepHeight', (wh, 0x80))
            lm.run('weaponBlockInit', (rec, wp, human))
            res[name] = (bytes(m.rd(st + i, 1) for i in range(0xE8)), m.rd(wh + 8, 4),
                         bytes(m.rd(wp + i, 1) for i in range(0x30)))
        lm.run('viewCameraOffsetsInit', (hum,))
        res['cam'] = [[s32(m.rd(hum + a + 4 * i, 4)) for i in range(3)] for a in cam_at]
        res['top'] = s32(lm.run('portCarProfileTopSpeed', (slot_a, 0x80)))
        res['radar'] = lm.run('portCarProfileRadar', (slot_a, 0x80))
        res['name'] = lm.run('portCarProfileName', (slot_a, 0x80))
        return res
    base, mine = run_all(0), run_all(1)
    rd = lambda b, o, w: int.from_bytes(b[o:o + w], 'little', signed=True)
    st_b, st_m = base['human'][0], mine['human'][0]
    for o, w, v in stats[:10]:
        for who in ('human', 'ai'):
            if rd(mine[who][0], o, w) != v:
                bad.append('%s stats +0x%02X = %d (want %d)' % (who, o, rd(mine[who][0], o, w), v))
    if rd(st_m, 0xA4, 4) != rd(st_b, 0xA4, 4):
        bad.append('the refused HP field was written')
    if rd(st_m, 0x4A, 2) != 150 or rd(st_m, 0x8C, 2) != 140 * 50 // 100:
        bad.append('derived wheelbase %d / retaliation %d (want 150 / 70)' % (rd(st_m, 0x4A, 2), rd(st_m, 0x8C, 2)))
    if rd(st_m, 0x54, 2) == rd(st_b, 0x54, 2):
        bad.append('the reverse speed was not derived from the top speed')
    changed = {o for o in range(0, 0xE8, 2) if st_m[o:o + 2] != st_b[o:o + 2]}
    derived = {0x4A, 0x54, 0x5A, 0x8C} | set(range(0xB0, 0xD8, 2))   # carStatsSetTopSpeed's speeds, +0xB0..+0xD6
    allowed = {o for o, w, v in stats[:10]} | {0xAE} | derived
    if changed - allowed:
        bad.append('stats changed beyond the profile and the derived fields: %s' %
                   ' '.join(hex(o) for o in sorted(changed - allowed)))
    if (mine['human'][1], mine['ai'][1], base['human'][1]) != (0x70, 0x70, 0x30):
        bad.append('step %d / %d, base %d (want 0x70 / 0x70, 0x30)' % (mine['human'][1], mine['ai'][1],
                                                                        base['human'][1]))
    for who in ('human', 'ai'):
        wb, wm = base[who][2], mine[who][2]
        if (rd(wm, 0x2C, 2), rd(wm, 0x24, 2), rd(wm, 0x0B, 1)) != (500, 30, 4) or \
                (rd(wb, 0x2C, 2), rd(wb, 0x24, 2), rd(wb, 0x0B, 1)) != (700, 24, 4):
            bad.append('%s weapons: recharge %d capacity %d max %d' % (who, rd(wm, 0x2C, 2), rd(wm, 0x24, 2),
                                                                      rd(wm, 0x0B, 1)))
    want = [list(v) for v in base['cam']]
    for v in (1, 5):
        want[v] = [cam(v, i) for i in range(3)]
    if mine['cam'] != want:
        bad.append('camera offsets %s (want %s)' % (mine['cam'], want))
    if (mine['top'], base['top']) != (150, 0x83):
        bad.append('default top speed %d, base %d (want 150, 131)' % (mine['top'], base['top']))
    if (mine['radar'], base['radar']) != (0x1000000 | 10 << 16 | 200 << 8 | 30, 0x80):
        bad.append('radar key %X, base %X' % (mine['radar'], base['radar']))
    if mine['name'] != 0x80E80000 or base['name'] == 0x80E80000:
        bad.append('target name %08X, base %08X' % (mine['name'], base['name']))
    print('11. mod car profiles (request 70): stats, derived fields, step, special, camera, top speed, radar, name, '
          'with and without a valid profile -> %s' % ('ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


def hidden_walk(m, roots, d2, eye_in=False, nodes=None):
    """The meshes a walk of the level reaches with every LOD at squared distance d2 (world units): a zone's outside
    child (its inside one with eye_in; none with its +4 bit 1, as the walker), an LOD's first record with min <= d2 <
    max, a select's current child (node+7), an animation's every frame, every child of the other nodes (a hidden
    group's none); read from the interpreter's memory. `nodes`: a set that gets every node visited."""
    seen = set() if nodes is None else nodes
    st, out = list(roots), set()
    while st:
        p = st.pop()
        if not p or p in seen:
            continue
        seen.add(p)
        t = m.rd(p, 1)
        if t == 0:
            out.add(p)
        elif t in (1, 4, 5, 12):
            if t == 1 and m.rd(p + 21, 1) & 2:
                continue
            cnt = {1: m.rd(p + 20, 1), 4: m.rd(p + 18, 1), 5: m.rd(p + 38, 1), 12: m.rd(p + 6, 2)}[t]
            base = {1: 24, 4: 20, 5: 40, 12: 8}[t]
            st += [m.rd(p + base + 4 * i, 4) for i in range(cnt)]
        elif t == 9:
            k = m.rd(p + 7, 1)
            if k < m.rd(p + 6, 1):
                st.append(m.rd(p + 16 + 4 * k, 4))
        elif t == 2:
            for i in range(min(m.rd(p + 16, 4), 32)):
                r = m.rd(p + 20 + 4 * i, 4)
                if m.rd(r + 4, 4) <= d2 < m.rd(r, 4):
                    st.append(m.rd(r + 8, 4))
                    break
        elif t == 3:
            st += [m.rd(p + 32 + 4 * i, 4) for i in range(m.rd(p + 11, 1))]
        elif t == 11:
            if not m.rd(p + 4, 1) & 2:
                st.append(m.rd(p + (12 if eye_in else 8), 4))
    return out


# request 104 (HIDDEN_AREAS.md 15): Hong Kong's subway: its zone, the four mouth zones' outside children (the box),
# the city's stand-in tunnels, the subway's facade stand-ins, the 8 opened LODs (coarse, finest records)
HIDDEN_SUBWAY = dict(zone=0x80070864, city=0x80065664, sub=0x800707D4, tubes=0x800382A0,
                     mouths=((0x80038B94, 0x800359A0, 0x80038B28), (0x80038CE0, 0x80038BD4, 0x80038C74),
                             (0x80049F64, 0x80049EDC, 0x80049EF4), (0x8004A000, 0x80049F78, 0x80049F90)),
                     stand=(0x80038744, 0x80038300), facades=(0x8006D8CC, 0x8006DB48, 0x8006FF30),
                     opened=((0x8006D5DC, 0x8006B010, 0x8006D5D0), (0x8006ADB4, 0x80067834, 0x8006ADA8),
                             (0x8006FC40, 0x8006DED8, 0x8006FC34), (0x80038284, 0x80035BC4, 0x80038278),
                             (0x80070164, 0x80070000, 0x80070158), (0x80070374, 0x80070210, 0x80070368),
                             (0x800704D0, 0x800703FC, 0x800704C4), (0x8007062C, 0x80070558, 0x80070620)))


# the subway's own parts (portHiddenSubway's subway_parts): the tunnels' 8 transforms, the signs, the 11 pickups
SUBWAY_PARTS = (0x8006ADEC, 0x8006D614, 0x8006D62C, 0x8006FC78, 0x8006FC90, 0x8006FCEC, 0x8006FD18, 0x8006FD30,
                0x80070664) + tuple(0x800706CC + 0x18 * i for i in range(11))


def count_walk(m, root, d2, eye_in, force=None, lods=None):
    """hidden_walk's rules without its de-duplication: how many times each mesh is reached (an instanced mesh once per
    path), `force` {zone: child} in place of the eye's choice, `lods` {LOD: record index, -1 none} in place of the
    distance's."""
    from collections import Counter
    out, st, steps = Counter(), [root], 0
    while st:
        p = st.pop()
        steps += 1
        if not p or steps > 400000:
            continue
        t = m.rd(p, 1)
        if t == 0:
            out[p] += 1
        elif t in (1, 4, 5, 12):
            if t == 1 and m.rd(p + 21, 1) & 2:
                continue
            cnt = {1: m.rd(p + 20, 1), 4: m.rd(p + 18, 1), 5: m.rd(p + 38, 1), 12: m.rd(p + 6, 2)}[t]
            base = {1: 24, 4: 20, 5: 40, 12: 8}[t]
            st += [m.rd(p + base + 4 * i, 4) for i in range(cnt)]
        elif t == 9:
            k = m.rd(p + 7, 1)
            if k < m.rd(p + 6, 1):
                st.append(m.rd(p + 16 + 4 * k, 4))
        elif t == 2:
            if lods and p in lods:
                if lods[p] >= 0:
                    st.append(m.rd(m.rd(p + 20 + 4 * lods[p], 4) + 8, 4))
                continue
            for i in range(min(m.rd(p + 16, 4), 32)):
                r = m.rd(p + 20 + 4 * i, 4)
                if m.rd(r + 4, 4) <= d2 < m.rd(r, 4):
                    st.append(m.rd(r + 8, 4))
                    break
        elif t == 3:
            st += [m.rd(p + 32 + 4 * i, 4) for i in range(m.rd(p + 11, 1))]
        elif t == 11:
            if force and p in force:
                st.append(force[p])
            elif not m.rd(p + 4, 1) & 2:
                st.append(m.rd(p + (12 if eye_in else 8), 4))
    return out


def hidden_subway(lm, m, ha, level):
    """Check 12's subway part (request 104, HIDDEN_AREAS.md 15). Retail first (the eye out of the zone: the city, the
    mouths' box; in it: the subway only); then with bit 6, at distances from 50 to 3,000 and the eye in or out of the
    zones, the walk from the subway's zone reaches: no stand-in (the mouths' box, the city's stand-in tunnels, the
    subway's facades) and no coarse record of an opened LOD; every other mesh of the city (with the mouths' tubes)
    and of the subway at that distance (the opened ones' finest); no mesh more often than in the city with its mouths'
    tubes or in the subway (no surface twice); the temple's inside and the city both. End gives the file back."""
    sw = HIDDEN_SUBWAY
    bad = []
    stand = set()
    for n in list(sw['stand']) + list(sw['facades']) + [z[1] for z in sw['mouths']]:
        for d in (0, 50, 300, 700, 1100, 3000):
            stand |= set(count_walk(m, n, d * d, False))
    # the temple's patches (request 97) as hidden_temple checks them: the doorway zones skipped, the passages' outside
    force = {z: z_in for z, _o, z_in in sw['mouths']}
    force.update({0x80063FB4: 0, 0x80064524: 0, 0x80064B40: 0, 0x80048CE0: 0x80048178, 0x80048D68: 0x80048CF4,
                  0x80048DF0: 0x80048D7C})
    lods = {lod: 1 for lod, _c, _f in sw['opened']}         # the finest record (the second)
    lods.update({0x80038ADC: -1, 0x800386F8: -1})          # the city's stand-in tunnels: never
    pickup = set()                                          # the pickups' models, the city's pickups' too
    for n in SUBWAY_PARTS[9:]:
        pickup |= set(count_walk(m, n, 0, False))
    model = {}
    for d in (50, 300, 700, 1100, 3000):
        city = count_walk(m, sw['zone'], d * d, False)
        sub = count_walk(m, sw['zone'], d * d, True)
        tubes = set(count_walk(m, sw['tubes'], d * d, False))
        if (set(sub) & set(city)) - tubes - pickup or (d <= 700 and not set(city) & stand):
            bad.append('HKONG subway at %d: the retail walk is not what HIDDEN_AREAS.md 15 says: shared %s' % (
                d, sorted('%08X' % n for n in (set(sub) & set(city)) - tubes - pickup)[:6]))
        for e in (False, True):
            want = count_walk(m, 0x80055270, d * d, e, force, lods) + count_walk(m, 0x800655FC, d * d, e, force, lods)
            for n in SUBWAY_PARTS:
                want += count_walk(m, n, d * d, e, force, lods)
            model[d, e] = want
    coarse = set()
    for lod, c, f in sw['opened']:
        coarse |= set(count_walk(m, m.rd(c + 8, 4), 0, False)) - set(count_walk(m, m.rd(f + 8, 4), 0, False))
    rv = s32(lm.run('portHiddenAreasBegin', (3,)))
    for (d, eye_in), want in model.items():
        got = count_walk(m, sw['zone'], d * d, eye_in)
        shown = sorted('%08X' % n for n in set(got) & (stand | coarse))
        diff = sorted('%08X %d/%d' % (n, got[n], want[n]) for n in set(got) | set(want) if got[n] != want[n])
        if shown or diff or not set(got) & set(count_walk(m, 0x800655FC, d * d, eye_in, force, lods)):
            bad.append('HKONG subway at %d (eye %s): stand-ins / coarse %s; against the model (got/want) %s' % (
                d, 'in' if eye_in else 'out', shown[:6], diff[:6]))
    lm.run('portHiddenAreasEnd')
    if rv != 32:
        bad.append('HKONG subway: Begin returned %d (want 32)' % rv)
    # a node not as listed: the temple patched, the subway not
    before = level()
    m.wr(0x800707D4 + 20, 1, 12)
    rv = s32(lm.run('portHiddenAreasBegin', (3,)))
    lm.run('portHiddenAreasEnd')
    m.wr(0x800707D4 + 20, 1, 13)
    if rv != 8 or level() != before:
        bad.append('HKONG subway: with its group changed Begin returned %d (want 8: the temple only)' % rv)
    return bad


def hidden_temple(lm, m, dmd, ha, level):
    """Check 12's Hong Kong part (request 97, HIDDEN_AREAS.md 14): retail's walk is what HIDDEN_AREAS.md 14 says (the
    room's zone: the city outside it, the inside in it; in it the doorway zones' stand-ins); with bit 6 Begin returns
    8 and names the place, and the walk with the eye in or out of every zone, near and far, reaches both the city and
    the inside and none of the doorway zones' children (cards, stand-ins) or the passages' stand-ins; End gives the
    file back."""
    t = HIDDEN_TEMPLE
    hidden = set(t['doors']) | set(t['passages'])
    bad = []
    roots = [t['room']]             # (the walk's zones above the room, the subway's, choose by the eye on their own)
    for eye_in, want, other in ((False, t['city'], t['inside']), (True, t['inside'], t['city'])):
        seen = set()
        hidden_walk(m, roots, 600 * 600, eye_in, seen)
        if want not in seen or other in seen or (eye_in and t['doors'][1] not in seen):
            bad.append('HKONG: the retail walk (eye %s) is not what HIDDEN_AREAS.md 14 says' % ('in' if eye_in else
                                                                                               'out'))
    before = level()
    rv = s32(lm.run('portHiddenAreasBegin', (3,)))
    if rv != 32 or m.rd(ha, 4) != 3 or m.rd(ha + 8 + 12, 4) != 32:
        bad.append('HKONG: Begin returned %d, place %d, drawn[3] %d (want 32: the temple 8, the subway 24; 3, 32)' % (
            rv, m.rd(ha, 4), m.rd(ha + 20, 4)))
    for d in (50, 600, 2000, 20000):
        for eye_in in (False, True):
            seen = set()
            hidden_walk(m, roots, d * d, eye_in, seen)
            shown = sorted('%08X' % x for x in seen & hidden)
            if t['city'] not in seen or t['inside'] not in seen or shown:
                bad.append('HKONG at %d (eye %s the zones): city %s, inside %s, cards / stand-ins walked %s' % (
                    d, 'in' if eye_in else 'out', t['city'] in seen, t['inside'] in seen, shown))
    lm.run('portHiddenAreasEnd')
    if level() != before or m.rd(ha + 4, 4) != 0:
        bad.append('HKONG: the level differs from the file after End')
    bad += hidden_subway(lm, m, ha, level)
    m.wr(t['room'], 1, 2)
    rv = s32(lm.run('portHiddenAreasBegin', (3,)))
    lm.run('portHiddenAreasEnd')
    m.wr(t['room'], 1, 11)
    if rv != 0:
        bad.append('HKONG: with the room\'s zone changed Begin returned %d (want 0)' % rv)
    return bad


def hidden_burb(lm, m, ha, level):
    """Check 12's Cyburbia part (request 119, HIDDEN_AREAS.md 17): retail's walk of the arena's zone 8008B580 is the
    suburb (8008B51C) with the eye in its region and the ring highway (8003EBB4) out of it; with bit 6 Begin returns 2
    and names the place, the walk with the eye in or out, near and far, reaches both; End gives the file back; with
    the zone changed nothing is patched."""
    zone, ring, burb = 0x8008B580, 0x8003EBB4, 0x8008B51C
    bad = []
    for eye_in, want, other in ((False, ring, burb), (True, burb, ring)):
        seen = set()
        hidden_walk(m, [zone], 600 * 600, eye_in, seen)
        if want not in seen or other in seen:
            bad.append('BURB: the retail walk (eye %s) is not what HIDDEN_AREAS.md 17 says' % ('in' if eye_in else 'out'))
    before = level()
    rv = s32(lm.run('portHiddenAreasBegin', (3,)))
    if rv != 2 or m.rd(ha, 4) != 4 or m.rd(ha + 8 + 12, 4) != 2:
        bad.append('BURB: Begin returned %d, place %d, drawn[3] %d (want 2, 4, 2)' % (rv, m.rd(ha, 4), m.rd(ha + 20, 4)))
    for d in (50, 600, 2000, 20000):
        for eye_in in (False, True):
            seen = set()
            hidden_walk(m, [zone], d * d, eye_in, seen)
            if ring not in seen or burb not in seen:
                bad.append('BURB at %d (eye %s the region): ring %s, suburb %s' % (
                    d, 'in' if eye_in else 'out', ring in seen, burb in seen))
    lm.run('portHiddenAreasEnd')
    if level() != before or m.rd(ha + 4, 4) != 0:
        bad.append('BURB: the level differs from the file after End')
    m.wr(zone, 1, 2)
    rv = s32(lm.run('portHiddenAreasBegin', (3,)))
    lm.run('portHiddenAreasEnd')
    m.wr(zone, 1, 11)
    if rv != 0:
        bad.append('BURB: with the zone changed Begin returned %d (want 0)' % rv)
    return bad


def check_hidden_areas(out):
    """Check 12 (request 72): portHiddenAreasBegin / End in LinkMips on the disc's two Paris files and ISLANDS."""
    from check_rollback import Image
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN
    bad = []
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    if any(n not in sizes for n in ('portHiddenAreasBegin', 'portHiddenAreasEnd')) or 'portHiddenAreas' not in syms:
        print('12. hidden areas (request 72) -> FAIL: portHiddenAreasBegin / End or portHiddenAreas not in the link')
        return ['missing']
    try:
        disc = Disc(DEFAULT_BIN)
    except OSError as e:
        print('12. hidden areas (request 72) -> FAIL: the disc image: %s' % e)
        return ['disc']
    vis, ha = syms['portVisual']['address'], syms['portHiddenAreas']['address']
    if 'portHiddenRoof' not in syms:
        print('12. hidden areas (request 72) -> FAIL: portHiddenRoof (request 75) not in the link')
        return ['roof']
    roof = syms['portHiddenRoof']['address']
    roof_meshes = {roof + 0x54, roof + 0x80}

    def sx(v, bits):
        return v - (1 << bits) if v & (1 << (bits - 1)) else v

    def roof_quads(m):
        """The roof meshes' quads through the turn node (portHiddenRoof.turn): their corners in the church's
        frame (the walker's opcode 5: offset + rotation^T q, 4096 = 1)."""
        mir = roof + 0x24
        rot = [sx(m.rd(mir + 4 + 2 * k, 2), 16) for k in range(9)]
        off = [sx(m.rd(mir + 24 + 4 * k, 4), 32) for k in range(3)]
        quads = []
        for mesh in [m.rd(mir + 40 + 4 * i, 4) for i in range(m.rd(mir + 38, 1))]:
            verts, prim = m.rd(mesh + 4, 4), m.rd(mesh + 12, 4)
            for _ in range(m.rd(mesh + 16, 4)):
                a, b = m.rd(prim + 4, 4), m.rd(prim + 8, 4)
                vs = []
                for i in (a & 0xFFFF, a >> 16, b & 0xFFFF, b >> 16):
                    q = [sx(m.rd(verts + 8 * i + 2 * k, 2), 16) for k in range(3)]
                    vs.append([off[c] + sum(rot[3 * r + c] * q[r] for r in range(3)) // 4096 for c in range(3)])
                quads.append(vs)
                prim += m.rd(prim + 2, 1) * 4
        return quads
    for name in ('PARIS', 'PARIS2', 'ISLANDS', 'HKONG', 'BURB'):
        dmd = Dmd(name, disc.get('/DMD/%s.DMD' % name))
        lm = LinkMips(img, {})
        m = lm.m
        m.fill(HIDDEN_LEVEL_AT, dmd.d)

        def level():
            return bytes(m.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(dmd.d)))
        before = level()
        # the bit clear (every other bit set): nothing
        m.wr(vis, 4, 0x3F)
        rv = s32(lm.run('portHiddenAreasBegin', (1,)))
        lm.run('portHiddenAreasEnd')
        if rv != 0 or m.rd(ha + 8 + 4, 4) != 0 or level() != before:
            bad.append('%s: with bit 6 clear Begin returned %d / changed the level' % (name, rv))
        m.wr(vis, 4, 0x40)
        if name == 'HKONG':             # request 97: the temple
            bad += hidden_temple(lm, m, dmd, ha, level)
            continue
        if name == 'BURB':              # request 119: the suburb and the ring
            bad += hidden_burb(lm, m, ha, level)
            continue
        c = HIDDEN_CHURCH.get(name)
        if c is None:                   # another level: nothing listed
            rv = s32(lm.run('portHiddenAreasBegin', (2,)))
            lm.run('portHiddenAreasEnd')
            if rv != 0 or m.rd(ha, 4) != 0 or level() != before:
                bad.append('%s: Begin returned %d, place %d / changed the level' % (name, rv, m.rd(ha, 4)))
            continue
        church = {c['nave'], c['stand'], c['near'], c['mid'], c['far'], c['door0'], c['door1']}
        dists = (50, 600, 2000, 20000)
        retail = {d: hidden_walk(m, dmd.roots, d * d) & church for d in dists}
        if any(c['nave'] in retail[d] for d in dists) or c['far'] not in retail[2000] or c['door0'] in retail[2000]:
            bad.append('%s: the retail walk is not what HIDDEN_AREAS.md 1 says: %s' % (
                name, {d: sorted('%08X' % x for x in v) for d, v in retail.items()}))
        for door in (0, 1):
            m.wr(c['door'] + 7, 1, door)
            before = level()
            rv = s32(lm.run('portHiddenAreasBegin', (3,)))
            if rv != 6 or m.rd(ha, 4) != c['place'] or m.rd(ha + 4, 4) != 6 or m.rd(ha + 8 + 12, 4) != 6:
                bad.append('%s: Begin returned %d, place %d, count %d, drawn[3] %d (want 6, %d, 6, 6)' % (
                    name, rv, m.rd(ha, 4), m.rd(ha + 4, 4), m.rd(ha + 20, 4), c['place']))
            # request 75: the roof, 18 quads at z 832 facing up, behind the towers
            quads = roof_quads(m) if m.rd(roof, 4) == 2 else []
            if len(quads) != 18:
                bad.append('%s: the roof: built %d, %d quads (want 2, 18)' % (name, m.rd(roof, 4), len(quads)))
            for vs in quads:
                (x0, y0, _), (x1, y1, _), (x2, y2, _) = vs[0], vs[1], vs[2]
                up = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0)     # the file's floors give < 0 (they face up)
                if any(v[2] != 832 for v in vs) or up >= 0 or min(v[1] for v in vs) < 764 or \
                        max(v[1] for v in vs) > 1628 or min(v[0] for v in vs) < -768 or max(v[0] for v in vs) > 768:
                    bad.append('%s: a roof quad %s (want z 832, facing up, local y 764..1628, x -768..768)' % (
                        name, vs))
                    break
            for d in dists:
                for eye_in in (False, True):
                    walked = hidden_walk(m, dmd.roots, d * d, eye_in)
                    if roof_meshes - walked:
                        bad.append('%s door %d at %d: the roof is not walked' % (name, door, d))
                    got = walked & church
                    if d < 275:                 # the near door meshes are other nodes
                        got -= {c['door0'], c['door1']}
                        want = {c['nave'], c['near']}
                    else:
                        want = {c['nave'], c['mid'], c['door1'] if door else c['door0']}
                    if got != want:
                        bad.append('%s door %d at %d (eye %s the zones): %s (want %s)' % (
                            name, door, d, 'in' if eye_in else 'out', sorted('%08X' % x for x in got),
                            sorted('%08X' % x for x in want)))
            lm.run('portHiddenAreasEnd')
            if level() != before or m.rd(ha + 4, 4) != 0:
                bad.append('%s door %d: the level differs from the file after End' % (name, door))
        m.wr(c['door'] + 7, 1, 0)
        # request 78: the eye above the roof: the nave's top (z 920) at 832 for the walk, put back by End
        eye_at = 0x80A60000
        le = LinkMips(img, {'viewGetEyeTrans': lambda mm: eye_at})
        le.m.fill(HIDDEN_LEVEL_AT, dmd.d)
        me = le.m

        def nave_z():
            verts, prim = me.rd(c['nave'] + 4, 4), me.rd(c['nave'] + 12, 4)
            zs = {}
            for _ in range(me.rd(c['nave'] + 16, 4)):
                a, b = me.rd(prim + 4, 4), me.rd(prim + 8, 4)
                n = 4 if (me.rd(prim + 12, 4) >> 16) & 7 == 4 else 3
                for i in [a & 0xFFFF, a >> 16, b & 0xFFFF, b >> 16][:n]:
                    zs[verts + 8 * i + 4] = sx(me.rd(verts + 8 * i + 4, 2), 16)
                prim += me.rd(prim + 2, 1) * 4
            return zs
        me.wr(vis, 4, 0x40)
        file_z = nave_z()
        top = [a for a, z in file_z.items() if z == 920]
        file_bytes = bytes(me.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(dmd.d)))
        for eye_z, pre, want in ((1000, None, 1), (832, None, 0), (1000, top[0] if top else None, 0)):
            for k, v in enumerate((0, 0, -eye_z)):
                me.wr(eye_at + 4 * k, 4, v & 0xFFFFFFFF)
            if pre is not None:
                me.wr(pre, 2, 832)
            rv = s32(le.run('portHiddenAreasBegin', (3,)))
            zs = nave_z()
            low = me.rd(roof + 0xB0, 4)
            if want:
                quads = roof_quads(me)
                moved = [a for a in top if zs[a] == 832]
                if rv != 6 or low != 1 or len(top) != 42 or len(moved) != 42 or 920 in zs.values() or                         len(quads) != 18 or any(v[2] != 832 for vs in quads for v in vs):
                    bad.append('%s eye z %d: Begin %d, lowered %d, %d of %d top vertices at 832, 920 left %s, roof '
                               '%d quads at z %s (want 6, 1, 42 of 42, no, 18 at 832)' % (
                                   name, eye_z, rv, low, len(moved), len(top), 920 in zs.values(), len(quads),
                                   sorted({v[2] for vs in quads for v in vs})))
            else:
                changed = [a for a in zs if zs[a] != file_z[a] and a != pre]
                if rv != 6 or low != 0 or changed:
                    bad.append('%s eye z %d%s: Begin %d, lowered %d, %d nave vertices moved (want 6, 0, 0)' % (
                        name, eye_z, ' (a vertex at 832)' if pre else '', rv, low, len(changed)))
            le.run('portHiddenAreasEnd')
            if want and (me.rd(roof + 0xB0, 4) != 1 or 920 in nave_z().values()):
                bad.append('%s eye z %d: the nave\'s top put back by End (request 84: not before the emitter)' % (
                    name, eye_z))
            le.run('portHiddenAreasDrawn')
            if pre is not None:
                me.wr(pre, 2, 920)
            if bytes(me.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(dmd.d))) != file_bytes or                     me.rd(roof + 0xB0, 4) != 0:
                bad.append('%s eye z %d: the level differs from the file after Drawn' % (name, eye_z))
        # request 75: the nave not as listed: the church patched, no roof
        count = m.rd(c['nave'] + 16, 4)
        m.wr(c['nave'] + 16, 4, count - 1)
        rv = s32(lm.run('portHiddenAreasBegin', (4,)))
        walked = hidden_walk(m, dmd.roots, 600 * 600)
        if rv != 6 or m.rd(roof, 4) != 0 or roof_meshes & walked or c['nave'] not in walked:
            bad.append('%s: with the nave changed Begin returned %d, roof built %d, roof walked %s (want 6, 0, no)' % (
                name, rv, m.rd(roof, 4), bool(roof_meshes & walked)))
        lm.run('portHiddenAreasEnd')
        m.wr(c['nave'] + 16, 4, count)
        # a far record extended by portFarLodBegin first: End gives back the extended value, not the file's
        m.wr(c['far_rec'], 4, 0xFFFFFFFF)
        ext = level()
        lm.run('portHiddenAreasBegin', (0,))
        if m.rd(c['far_rec'], 4) != 0:
            bad.append('%s: the far wall record holds %08X while patched (want 0)' % (name, m.rd(c['far_rec'], 4)))
        lm.run('portHiddenAreasEnd')
        if level() != ext:
            bad.append('%s: End did not give back the far-LOD extended record' % name)
    print('12. hidden areas (request 72): Paris\'s church in PARIS and PARIS2 drawn from anywhere (inside, doorway, '
          'door) and put back; the bit clear and ISLANDS untouched; request 75: the roof behind the towers; '
          'request 78: the nave\'s top at the roof with the eye above it; requests 97 / 104: Hong Kong\'s temple and '
          'subway; request 119: Cyburbia\'s suburb and ring both drawn -> %s' % ('ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


FAR_SCAN_LEVELS = ('DEN', 'BURB', 'PARIS')     # request 90: the grass level and two with nested beam details kept


def check_steady_shadow(out, names):
    """Check 15 (request 92, RENDER_ANSWERS.md R7, portVisual bit 7): portVisualSteadyBegin / End in LinkMips on a
    made-up human record (a Mr. Slam, type 0xC0: body object 12 of carBodyObjects; humanCarGet stubbed, humanCarTotal
    1, aiCarTotal 0), tilted (a pitch and roll in motion.rot_m) with its wheels' suspension uneven. The game's own
    carBodyPoseUpdate gives the step's matrix; Begin must give carBodyPoseUpdate's pose of the same car with the
    suspension even (each wheel's pos[2] - h equal) and keep the translation; End must give back the step's matrix
    byte for byte; with the bit clear Begin returns 0 and changes nothing. Only without TM2_PORT_CAR_GROUPS (the
    body table is carBodyObjects); with it the check is skipped."""
    from check_rollback import Image
    if 'TM2_PORT_CAR_GROUPS' in names:
        print('15. steady shadows (request 92): skipped (car groups: the body table is portCarGroups.body) -> ok')
        return []
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    need = ('carBodyObjects', 'humanCarTotal', 'aiCarTotal', 'portVisual')
    if any(n not in sizes for n in ('portVisualSteadyBegin', 'portVisualSteadyEnd', 'carBodyPoseUpdate')) or \
            any(n not in syms for n in need):
        print('15. steady shadows (request 92) -> FAIL: the functions or their storage are not in the link')
        return ['missing']
    rec, body = 0x80A40000, 0x80A42000
    W, M = 0x5A4 + 0x18, 0x6DC          # the human record's wheel[0] and motion
    corners = ((61, -37), (61, 37), (-79, -37), (-79, 37))     # wheel x (forward), y (side)
    rot = (60, -40, 900)                # pitch, roll, yaw (4096 a turn)
    lm = LinkMips(img, {'humanCarGet': lambda m: rec})
    m = lm.m
    for k in range(0, 0x7C8, 4):
        m.wr(rec + k, 4, 0)
    for k in range(0, 0x40, 4):
        m.wr(body + k, 4, 0)
    m.wr(rec + 0x90, 4, 0xC0)
    m.wr(syms['carBodyObjects']['address'] + 4 * 12, 4, body)
    m.wr(syms['humanCarTotal']['address'], 4, 1)
    m.wr(syms['aiCarTotal']['address'], 4, 0)
    # motion: pos, rot, rot_m (RotMatrixYXZ of rot, by the link's own function)
    for i, v in enumerate((5000, -3000, 120)):
        m.wr(rec + M + 0x20 + 4 * i, 4, v)
    for i, v in enumerate(rot):
        m.wr(rec + M + 0x2C + 4 * i, 4, v)
    tmp = 0x80A43000
    for i, v in enumerate(rot + (0,)):
        m.wr(tmp + 2 * i, 2, v)
    lm.run('RotMatrixYXZ', (tmp, rec + M + 0x50))

    def wheels(sus):
        for i, ((x, y), d) in enumerate(zip(corners, sus)):
            w = rec + W + 0x48 * i
            m.wr(w + 0x24, 2, 100)                      # h: the ground
            m.wr(w + 0x3C, 4, 5000 + x)
            m.wr(w + 0x40, 4, -3000 + y)
            m.wr(w + 0x44, 4, 100 + 32 + d)             # pos[2]: ride height + the suspension

    def mat():
        return [m.rd(body + 4 + 4 * i, 4) for i in range(8)]
    bad = []
    wheels((0, 0, 0, 0))
    lm.run('carBodyPoseUpdate', (rec, 1))
    flat = mat()
    wheels((6, -5, -7, 9))
    lm.run('carBodyPoseUpdate', (rec, 1))
    step = mat()
    if step == flat:
        bad.append('the uneven suspension does not change the step\'s pose (the test sees nothing)')
    m.wr(syms['portVisual']['address'], 4, 0)
    if lm.run('portVisualSteadyBegin') != 0 or mat() != step:
        bad.append('bit clear: Begin changed the shadow')
    m.wr(syms['portVisual']['address'], 4, 0x80)
    n = s32(lm.run('portVisualSteadyBegin'))
    got = mat()
    if n != 1 or got[:5] != flat[:5] or got[5:] != step[5:]:
        bad.append('bit 7: Begin %d, matrix %s (want 1, the even-suspension rotation %s and the step\'s '
                   'translation %s)' % (n, ['%08X' % v for v in got], ['%08X' % v for v in flat[:5]],
                                         step[5:]))
    lm.run('portVisualSteadyEnd')
    if mat() != step:
        bad.append('End: the step\'s matrix is not back')
    print('15. steady shadows (request 92): portVisualSteadyBegin poses a tilted Mr. Slam\'s shadow as with even '
          'suspension, End puts the step\'s matrix back, nothing with the bit clear -> %s' % (
              'ok' if not bad else 'FAIL'))
    for b in bad:
        print('     ' + b)
    return bad


def whole_near_ot(m):
    """The order table of an emitter run (emit_scene's, at EMIT_BASE): {packet address: OT entry} of every packet
    linked into it."""
    ot, got = EMIT_BASE, {}
    for i in range(4096):
        end = (ot + 4 * i + 4) & 0xFFFFFF
        p, n = m.rd(ot + 4 * i, 4) & 0xFFFFFF, 0
        while p != end and n < 100000:
            got[0x80000000 | p] = i
            p = m.rd(0x80000000 | p, 4) & 0xFFFFFF
            n += 1
    return got


def check_mirror_car(out):
    """Check 17 (request 99, RENDER_ANSWERS.md R9, portVisual bit 9): portVisualMirrorEye / Skip in LinkMips on a
    made-up human 0 (humanCars[0]: car view +0x10C, roof_height +0x14C + 0x46, motion.pos +0x6FC, motion.rot +0x708,
    motion.rot_m +0x72C from RotMatrixYXZ of a tilted, turned pose; portLocalHuman stubbed: local 0 is human 0; the
    car's object humanCarObjects[0] made up; viewGetMode stubbed 4): with the bit the eye is motion.pos + rot_m^T (0,
    TM2_VIS_MIRROR_BACK, roof + TM2_VIS_MIRROR_UP) and the rotation (pitch, 0, yaw + 2048), Skip names the car; bit
    clear, car view 4 or another view mode: nothing set, no car left out."""
    from check_rollback import Image
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    need = ('humanCars', 'humanCarTotal', 'humanCarObjects', 'portVisual')
    if any(n not in sizes for n in ('portVisualMirrorEye', 'portVisualMirrorSkip')) or any(n not in syms for n in need):
        print('17. the mirror on the car (request 99) -> FAIL: the functions or their storage not in the link')
        return ['missing']
    hdr = (ROOT / 'include/semantic/port_visual.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+\(?(-?0x[0-9A-Fa-f]+|-?\d+)\)?' % n, hdr).group(1), 0)
    bit, up, back, pitch = (num('TM2_VIS_MIRROR_CAR'), num('TM2_VIS_MIRROR_UP'), num('TM2_VIS_MIRROR_BACK'),
                            num('TM2_VIS_MIRROR_PITCH'))
    mode = [4]
    rec, obj, pos, rot, tmp = syms['humanCars']['address'], 0x80A41000, 0x80A42000, 0x80A42100, 0x80A42200
    bad = []

    def run(live, view=2, md=4):
        mode[0] = md
        lm = LinkMips(img, {'portLocalHuman': lambda m: 0 if m.r[4] == 0 else 0xFFFFFFFF,
                            'viewGetMode': lambda m: mode[0]})
        m = lm.m
        for k in range(0, 0x814, 4):
            m.wr(rec + k, 4, 0)
        m.wr(syms['humanCarTotal']['address'], 4, 1)
        m.wr(syms['humanCarObjects']['address'], 4, obj)
        m.wr(syms['portVisual']['address'], 4, live)
        m.wr(rec + 0x10C, 4, view)
        m.wr(rec + 0x14C + 0x46, 2, 75)
        p0, r0 = (4000, -2500, 300), (120, -60, 3000)
        for i in range(3):
            m.wr(rec + 0x6FC + 4 * i, 4, p0[i] & 0xFFFFFFFF)
            m.wr(rec + 0x708 + 4 * i, 4, r0[i] & 0xFFFFFFFF)
            m.wr(tmp + 2 * i, 2, r0[i] & 0xFFFF)
        m.wr(tmp + 6, 2, 0)
        lm.run('RotMatrixYXZ', (tmp, rec + 0x72C))
        for k in range(0, 16, 4):
            m.wr(pos + k, 4, 0x11111111)
            m.wr(rot + k, 4, 0x22222222)
        got = lm.run('portVisualMirrorEye', (1, pos, rot))
        skip = lm.run('portVisualMirrorSkip', (1,))
        R = [[s32(m.rd(rec + 0x72C + 2 * (3 * a + b), 2) << 16) >> 16 for b in range(3)] for a in range(3)]
        v = (0, back, 75 + up)
        want = [p0[i] + sum(R[k][i] * v[k] for k in range(3)) // 4096 for i in range(3)]
        eye = [s32(m.rd(pos + 4 * i, 4)) for i in range(3)]
        angles = [m.rd(rot + 2 * i, 2) for i in range(3)]
        return got, skip, eye, want, angles, (pitch & 0xFFFF, 0, (r0[2] + 2048) & 0xFFF)
    got, skip, eye, want, angles, want_a = run(bit)
    if got != 1 or skip != obj or any(abs(a - b) > 1 for a, b in zip(eye, want)) or angles != list(want_a):
        bad.append('bit 9: set %d, skip %08X, eye %s, rotation %s (want 1, %08X, %s, %s)' % (
            got, skip, eye, angles, obj, want, list(want_a)))
    for name, kw, want_skip in (('bit clear', dict(live=0x1FF), 0), ('car view 4', dict(live=bit, view=4), 0),
                                ('watch view 7', dict(live=bit, view=7), 0), ('not a mirror', dict(live=bit, md=1), 0)):
        got, skip, eye, _, angles, _ = run(**kw)
        if (name != 'not a mirror' and (got != 0 or eye != [0x11111111] * 3)) or skip != want_skip:
            bad.append('%s: set %d, skip %08X, eye %s' % (name, got, skip, ['%08X' % (e & 0xFFFFFFFF) for e in eye]))
    print('17. the mirror on the car (request 99): portVisualMirrorEye puts the eye on a tilted, turned car\'s roof '
          'looking back, Skip leaves the car out; nothing without the bit or outside car views 2 and 3 -> %s' % (
              'ok' if not bad else 'FAIL'))
    for b in bad:
        print('     ' + b)
    return bad


def shadow_otz(z, shift, near, bias=0):
    """The emitter's OT entry of a depth z (portVisualEntryDepth's mapping): near: no bias, at least 1."""
    if near:
        z = max(z, 0)
    o = z >> shift
    if o >= 1024:
        o = ((o - 1024) >> 2) + 1024 if o < 4096 else ((o - 4096) >> 3) + 1792 if o < 16384 else \
            ((o - 16384) >> 5) + 3328
    elif near:
        o = max(o, 1)
    elif o > 0:
        o = max(o + bias, 1)
    return o


def check_shadow_ground(out):
    """Check 19 (request 102 item 2, RENDER_ANSWERS.md R10, portVisual bit 10): portVisualShadows in LinkMips on made-up
    draw entries in portVisualState's ranges (entry: mesh, translation x y z, shift, light record, rotation slot; mesh
    +4 vertices, +12 primitives, +16 count; primitive +2 size in words, +4.. vertex indices, +12 control with the type
    in bits 16..18; the slot an identity rotation, so SZ = vertex z + translation z) against shadow_otz."""
    from check_rollback import Image
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    need = ('portVisualState', 'portVisual', 'renderMatrixSlots')
    if 'portVisualShadows' not in sizes or any(n not in syms for n in need):
        print('19. the shadow over the ground (request 102) -> FAIL: the functions or their storage not in the link')
        return ['missing']
    hdr = (ROOT / 'include/semantic/port_visual.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+\(?(-?0x[0-9A-Fa-f]+|-?\d+)\)?' % n, hdr).group(1), 0)
    bit, bit3, lift = num('TM2_VIS_SHADOW_GROUND'), num('TM2_VIS_SHADOW'), num('TM2_VIS_SHADOW_LIFT')
    vs, slots = syms['portVisualState']['address'], syms['renderMatrixSlots']['address']
    base, k, slot, light = 0x80A40000, 3, 7, 0x1234
    bad = []

    def mesh(m, at, prims):
        """prims: [[z, ...]] (3 or 4 corners); vertices one per corner. Returns the mesh's address."""
        verts, pr = at + 0x100, at + 0x400
        n = 0
        for i, zs in enumerate(prims):
            q = pr + 20 * i
            m.wr(q, 4, 5 << 16)                     # +2: 5 words
            for j, z in enumerate(zs):
                m.wr(verts + 8 * n, 2, (10 * j) & 0xFFFF)
                m.wr(verts + 8 * n + 2, 2, (-7 * j) & 0xFFFF)
                m.wr(verts + 8 * n + 4, 2, z & 0xFFFF)
                m.wr(verts + 8 * n + 6, 2, 0)
                m.wr(q + 4 + 2 * j, 2, n)
                n += 1
            m.wr(q + 12, 4, (len(zs) + 0) << 16 if len(zs) == 4 else 3 << 16)
        for o, v in ((4, verts), (12, pr), (16, len(prims)), (24, 0)):
            m.wr(at + o, 4, v)
        return at

    def entry(m, e, ms, tz, shift):
        for i, v in enumerate((ms, 50, -60, tz, shift, e + 0x40, slot, 0)):
            m.wr(e + 4 * i, 4, v & 0xFFFFFFFF)
        m.wr(e + 0x40, 4, 0)
        m.wr(e + 0x44, 4, light)

    def run(live, car, shadows, car_prims=None):
        lm = LinkMips(img, {})
        m = lm.m
        m.wr(syms['portVisual']['address'], 4, live)
        for i in range(16):
            m.wr(slots + slot * 32 + 2 * i, 2, 0)
        for i in (0, 4, 8):
            m.wr(slots + slot * 32 + 2 * i, 2, 4096)
        for o in range(0, 0x300, 4):
            m.wr(vs + o, 4, 0)
        es = []
        for i, (prims, tz, shift) in enumerate(shadows):
            e = base + 0x80 * i
            entry(m, e, mesh(m, base + 0x10000 + 0x1000 * i, prims), tz, shift)
            es.append(e)
        m.wr(vs + 8 * k, 4, es[0])
        m.wr(vs + 8 * k + 4, 4, es[-1] + 32 if len(es) == 1 else es[0] + 32 * len(es))
        if len(es) > 1:                                 # contiguous entries (32 bytes each)
            for i, (prims, tz, shift) in enumerate(shadows):
                e = base + 32 * i
                entry(m, e, mesh(m, base + 0x10000 + 0x1000 * i, prims), tz, shift)
                m.wr(e + 20, 4, base + 0x800 + 8 * i)
                m.wr(base + 0x800 + 8 * i, 4, 0)
                m.wr(base + 0x804 + 8 * i, 4, light)
            es = [base + 32 * i for i in range(len(shadows))]
            m.wr(vs + 8 * k, 4, es[0])
            m.wr(vs + 8 * k + 4, 4, es[0] + 32 * len(es))
        if car:
            ce = base + 0x4000
            entry(m, ce, mesh(m, base + 0x20000, car_prims), 0, 0)
            m.wr(vs + 0x180 + 8 * k, 4, ce)
            m.wr(vs + 0x180 + 8 * k + 4, 4, ce + 32)
        lights = [m.rd(e + 20, 4) for e in es]
        lm.run('portVisualShadows', ())
        got = []
        for e, l0 in zip(es, lights):
            l1 = m.rd(e + 20, 4)
            got.append(None if l1 == l0 else (s32(m.rd(l1, 4)), m.rd(l1 + 4, 4)))
        cleared = all(m.rd(vs + o, 4) == 0 for o in range(0, 0x300, 4))
        return got, cleared
    a = ([[900, 1000, 1100, 1200], [2000, 3000, 650]], 0, 0)      # nearest 650
    b = ([[5000, 5000, 5000, 5000]], 0, 0)                        # all 5000: entry 1905
    car = [[1500, 1400, 1300, 1200]]                              # farthest 1500: entry 1143
    near = shadow_otz(650, 0, True) - lift
    far3 = max(shadow_otz(1500, 0, False) + 1, shadow_otz(5000, 0, False))
    cases = [('bit 10, the car walked', dict(live=bit, car=True, shadows=[a, b], car_prims=car), [(near, light)] * 2),
             ('bit 10, the car not walked', dict(live=bit, car=False, shadows=[a, b]), [(near, light)] * 2),
             ('bits 3 and 10', dict(live=bit | bit3, car=True, shadows=[a, b], car_prims=car), [(near, light)] * 2),
             ('bit 3 alone', dict(live=bit3, car=True, shadows=[a, b], car_prims=car), [(far3, light)] * 2),
             ('bit 3, the car not walked', dict(live=bit3, car=False, shadows=[a, b]), [None, None]),
             ('neither bit', dict(live=0x3F7, car=True, shadows=[a, b], car_prims=car), [None, None]),
             ('bit 10, a corner by the eye', dict(live=bit, car=False, shadows=[([[1, 900, 950]], 0, 0)]), [(1, light)]),
             ('bit 10, a corner behind the eye', dict(live=bit, car=False, shadows=[([[-40, 900, 950, 990]], 0, 0)]),
              [(1, light)]),
             ('bit 10, shift 2, translated', dict(live=bit, car=False, shadows=[([[300, 900, 950]], 6000, 2)]),
              [(shadow_otz(6300, 2, True) - lift, light)])]
    for name, kw, want in cases:
        got, cleared = run(**kw)
        if got != want or not cleared:
            bad.append('%s: %s (want %s), ranges cleared %s' % (name, got, want, cleared))
    print('19. the shadow over the ground (request 102 item 2): with bit 10 each shadow entry sorted %d in front of its '
          'nearest corner, its car walked or not, over bit 3; bit 3 alone request 46\'s; neither nothing -> %s' % (
              lift, 'ok' if not bad else 'FAIL'))
    for x in bad:
        print('     ' + x)
    return bad


def check_far_lod_budget(out):
    """Check 18 (request 98, FAR_LOD.md 14, TM2_PORT_FAR_LOD with TM2_PORT_FAR_DISTANCE): portFarLodBudgetView in
    LinkMips on made-up render stats (viewGetMode stubbed): with portFarLodNearScale 4 and portFarLodBudget 16 a view
    with room steps 4 -> 6, holds TM2_FAR_LOD_BUDGET_UP views, steps to 8; a full one (7/8 of the meshes) steps down
    and holds TM2_FAR_LOD_BUDGET_DOWN views; never below the floor or above the budget; the mirror its own row; the
    budget 0 nothing; and portFarLodBegin scales a listed record by the view's budget scale, End puts it back."""
    from check_rollback import Image
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    need = ('portFarLodBudget', 'portFarLodBudgetState', 'portFarLodNearScale', 'portRenderStats', 'portFarLodNear',
            'portFarLod')
    if any(n not in sizes for n in ('portFarLodBudgetView', 'portFarLodBegin', 'portFarLodEnd')) or \
            any(n not in syms for n in need):
        print('18. far-LOD budget (request 98) -> FAIL: the functions or their storage not in the link')
        return ['missing']
    hdr = (ROOT / 'include/semantic/port_far_lod.h').read_text(encoding='utf-8')
    num = lambda n: int(re.search(r'#define %s\s+(\d+)' % n, hdr).group(1))
    up, down = num('TM2_FAR_LOD_BUDGET_UP'), num('TM2_FAR_LOD_BUDGET_DOWN')
    mode = [1]
    lm = LinkMips(img, {'viewGetMode': lambda m: mode[0]})
    m = lm.m
    a = lambda n: syms[n]['address']
    st, stats = a('portFarLodBudgetState'), a('portRenderStats')
    for k in range(0, 0x10, 4):
        m.wr(st + k, 4, 0)
    for k in range(0, 0x150, 4):
        m.wr(stats + k, 4, 0)
    m.wr(a('portFarLodNearScale'), 4, 4)
    m.wr(a('portFarLodBudget'), 4, 16)
    m.wr(a('portFarLod'), 4, 0)

    def row(r, meshes, packet, caps=0):
        b = stats + 0x10 + 64 * r
        for off, v in ((0, 1), (0x0C, meshes), (0x14, 2048), (0x18, packet), (0x20, 390320), (0x2C, caps)):
            m.wr(b + off, 4, v)

    def view(v=0, md=1):
        mode[0] = md
        return s32(lm.run('portFarLodBudgetView', (v,)))
    bad = []
    row(0, 100, 10000)
    seq = [view() for _ in range(up + 2)]
    want = [6] * (up + 1) + [8]
    if seq != want:
        bad.append('room: %s (want %s)' % (seq, want))
    row(0, 1900, 10000)
    if view() != 6:
        bad.append('full (7/8 of the meshes): no step down')
    row(0, 100, 10000)
    seq = [view() for _ in range(down + 1)]
    if seq != [6] * down + [8]:
        bad.append('after a step down: %d views at 6, then %d (want %d, then 8)' % (seq.count(6), seq[-1], down))
    row(0, 100, 360000)
    seq = [view() for _ in range(3)]
    if seq[0] != 6:
        bad.append('the packet buffer 7/8 full: %s (want a step down to 6)' % seq)
    for _ in range(3 * down):
        row(0, 2000, 10000, 4)
        view()
    if view() != 4:
        bad.append('below the floor (portFarLodNearScale 4)')
    row(0, 10, 100)
    for _ in range(40 * down):
        if view() == 16:
            break
    if view() != 16:
        bad.append('the budget\'s top (16) not reached with room')
    row(4, 100, 10000)
    if view(1, 4) != 6 or m.rd(st + 4 + 4, 1) != 6:
        bad.append('the mirror: not its own row (4)')
    # portFarLodBegin through the budget's scale: one listed record {100 * 100, 0, child}
    near, rec = a('portFarLodNear'), 0x80A50000
    m.wr(rec, 4, 10000)
    m.wr(rec + 4, 4, 0)
    m.wr(rec + 8, 4, 0x80A50100)
    m.wr(near, 4, 1)
    m.wr(near + 4, 4, rec)
    m.wr(near + 4 + 4 * 4096, 4, 10000)
    m.wr(near + 4 + 8 * 4096, 4, 0)
    sc = view()
    lm.run('portFarLodBegin')
    got = m.rd(rec, 4)
    lm.run('portFarLodEnd')
    if got != 10000 * sc * sc or m.rd(rec, 4) != 10000:
        bad.append('portFarLodBegin at scale %d: max %d (want %d), after End %d' % (sc, got, 10000 * sc * sc,
                                                                                    m.rd(rec, 4)))
    m.wr(a('portFarLodBudget'), 4, 0)
    if view() != 0 or m.rd(st, 4) != 0:
        bad.append('the budget 0: a scale')
    print('18. far-LOD budget (request 98): portFarLodBudgetView steps each view\'s band scale by its previous frame '
          '(up %d / down %d views of hold, floor, top, the mirror\'s row), portFarLodBegin scales by it -> %s' % (
              up, down, 'ok' if not bad else 'FAIL'))
    for b in bad:
        print('     ' + b)
    return bad


def check_whole_near(out, e0):
    """Check 16 (request 93, RENDER_ANSWERS.md R8, portVisual bit 8): portVisualNearOtz in LinkMips gives the retail
    threshold with the bit clear (every other bit set or not) and 0 with it; then the link's emitter (sub_800CDC54) runs
    on made-up views with near polygons (emit_scene, PARIS and made-up meshes) at threshold 70 and at 0: at 0 the near
    list is empty, and every polygon the 70 run deferred (its packet's words) is linked whole into the OT entry its
    near-list entry names (|sort z|); with E0, the asm at 0 writes the same bytes as the link's emitter at 0."""
    import random
    from check_rollback import Image
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    if any(n not in sizes for n in ('portVisualNearOtz', 'sub_800CDC54')) or 'portVisual' not in syms:
        print('16. whole near polygons (request 93) -> FAIL: portVisualNearOtz or the emitter not in the link')
        return ['missing']
    bad = []
    lm = LinkMips(img, {})
    for bits, want in ((0, (70, 20)), (0xFF, (70, 20)), (0x100, (0, 0)), (0x1FF, (0, 0))):
        lm.m.wr(syms['portVisual']['address'], 4, bits)
        got = tuple(s32(lm.run('portVisualNearOtz', (r,))) for r in (70, 20))
        if got != want:
            bad.append('portVisual %X: thresholds %s (want %s)' % (bits, got, want))
    rng = random.Random(93)
    disc = emit_disc_meshes(['PARIS'])
    synth, smem = synth_meshes(rng)
    img_a = Image(e0) if e0 is not None else None
    deferred = whole = 0
    for si, (frac, cull) in enumerate(((0.5, 0), (0.8, 1), (0.3, 0))):
        lv = disc if si != 1 else {}
        loaded = [(v[0], v[1]) for v in lv.values()]
        scene = emit_scene(rng, lv, synth, 60, frac, 0, 0, cull_flip=cull)
        mem = list(scene[0])
        k = [i for i, (a, b) in enumerate(mem) if a == EMIT_SCRATCH][0]
        sp = bytearray(mem[k][1])
        sp[0x40:0x44] = bytes(4)
        mem[k] = (EMIT_SCRATCH, bytes(sp))
        scene0 = (mem,) + tuple(scene[1:])
        _, _, _, m70 = emit_run(img, scene, loaded, smem)
        o0, _, _, m0 = emit_run(img, scene0, loaded, smem)
        n70, n0 = m70.rd(EMIT_SCRATCH + 0x38, 4), m0.rd(EMIT_SCRATCH + 0x38, 4)
        over = int.from_bytes(sp[0x4C:0x50], 'little')
        if n0:
            bad.append('view %d: threshold 0, %d polygons deferred to the near pass' % (si, n0))
        ot0 = whole_near_ot(m0)
        by_words = {}
        for a, e in ot0.items():
            ln = m0.rd(a, 4) >> 24
            by_words.setdefault(tuple(m0.rd(a + 4 + 4 * j, 4) for j in range(ln)), set()).add(e)
        for i in range(n70):
            ent = EMIT_SCRATCH + 0xA4 + 12 * i if i < 71 else over + 12 * (i - 71)
            prim, pkt, sz = m70.rd(ent, 4), m70.rd(ent + 4, 4), s32(m70.rd(ent + 8, 4)) >> 16
            words = tuple(m70.rd(pkt + 4 + 4 * j, 4) for j in range(m70.rd(prim + 3, 1)))
            deferred += 1
            if abs(sz) in by_words.get(words, ()):
                whole += 1
            elif len(bad) < 12:
                bad.append('view %d: near entry %d (packet %08X, sort z %d) not linked whole at OT entry %d at '
                           'threshold 0 (found at %s)' % (si, i, pkt, sz, abs(sz), sorted(by_words.get(words, ()))))
        if img_a is not None:
            oa, _, _, _ = emit_run(img_a, scene0, loaded, smem)
            diff = sorted(a for a in set(oa) | set(o0) if oa.get(a) != o0.get(a))
            if diff:
                bad.append('view %d: threshold 0, the asm and the link\'s emitter differ at %d bytes (first %08X)' % (
                    si, len(diff), diff[0]))
    if deferred < 20:
        bad.append('only %d near polygons in the views (want at least 20: the test sees nothing)' % deferred)
    print('16. whole near polygons (request 93): portVisualNearOtz 0 with bit 8, else retail\'s; the emitter at 0 '
          'defers nothing, %d of %d near polygons linked whole at their entry%s -> %s' % (
              whole, deferred, ', the asm the same bytes' if img_a is not None else '', 'ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


def check_far_lod_scan(out):
    """Check 14 (request 90, RENDER_ANSWERS.md R6.1; request 91, FAR_LOD.md 13): portFarLodScanLevel and
    portFarLodBegin / End in LinkMips on DEN, BURB and PARIS from the disc at 0x80019C40. The records listed for the
    extension and for the near scale are tools/dmd_lod_dump.py's (the extension: extended() and fine_field()'s never
    records, those flagged with bit 1; the near scale: every LOD's records, fine_field()'s fine ones flagged). On DEN:
    with scale 4 and portFarLod 0, Begin scales Holland's grass beams (8002F50C) and the block above them like every
    band; with portFarLod set, the beams, the block's cells and the patch's blocks are drawn at every distance and the
    block's and patch's flat quads at none (the field at its finest level); End puts the file back, twice."""
    from check_rollback import Image
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN
    bad = []
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    need = ('portFarLodTable', 'portFarLodNear', 'portFarLod', 'portFarLodNearScale')
    if any(n not in sizes for n in ('portFarLodScanLevel', 'portFarLodBegin', 'portFarLodEnd')) or \
            any(n not in syms for n in need):
        print('14. far LOD scan (requests 90, 91) -> FAIL: the scan functions or their storage are not in the link')
        return ['missing']
    try:
        disc = Disc(DEFAULT_BIN)
    except OSError as e:
        print('14. far LOD scan (requests 90, 91) -> FAIL: the disc image: %s' % e)
        return ['disc']
    tab, near = syms['portFarLodTable']['address'], syms['portFarLodNear']['address']
    for name in FAR_SCAN_LEVELS:
        dmd = Dmd(name, disc.get('/DMD/%s.DMD' % name))
        if dmd.origin != HIDDEN_LEVEL_AT:
            bad.append('%s: the file is built for %08X' % (name, dmd.origin))
            continue
        lm = LinkMips(img, {})
        m = lm.m
        m.fill(HIDDEN_LEVEL_AT, dmd.d)
        m.wr(syms['portFarLod']['address'], 4, 0)
        m.wr(syms['portFarLodNearScale']['address'], 4, 0)
        lm.run('portFarLodScanLevel')
        ext = [m.rd(tab + 4 + 4 * i, 4) for i in range(m.rd(tab, 4))]
        nr = [m.rd(near + 4 + 4 * i, 4) for i in range(m.rd(near, 4))]
        got_ext, got_near = {a & ~3 for a in ext}, {a & ~3 for a in nr}
        got_never, got_fine = {a & ~3 for a in ext if a & 2}, {a & ~3 for a in nr if a & 2}
        fine, never = dmd.fine_field()
        want_ext = set(dmd.extended()) | never
        order, _ = dmd.nodes()
        want_near = {r[3] for n in order if dmd.type(n) == 2 for r in dmd.records(n)}
        if got_ext != want_ext or got_near != want_near:
            bad.append('%s: listed %d far / %d near records (want %d / %d; %d / %d differ)' % (
                name, len(got_ext), len(got_near), len(want_ext), len(want_near), len(got_ext ^ want_ext),
                len(got_near ^ want_near)))
        if got_fine != fine or got_never != never or any(a & 1 for a in ext + nr):
            bad.append('%s: flagged fine %s / never %s (want %s / %s)' % (
                name, sorted('%08X' % a for a in got_fine), sorted('%08X' % a for a in got_never),
                sorted('%08X' % a for a in fine), sorted('%08X' % a for a in never)))
        kept = dmd.kept_retail()
        if name == 'DEN' and ({'%08X' % n for n in kept} != {'8002F50C'} or len(fine) != 3 or len(never) != 2):
            bad.append('DEN: kept %s, %d fine, %d never (want 8002F50C, the tall grass; 3 and 2)' % (
                sorted('%08X' % n for n in kept), len(fine), len(never)))
        if name != 'DEN' and (kept or fine or never):
            bad.append('%s: kept %s, %d fine, %d never (want none)' % (
                name, sorted('%08X' % n for n in kept), len(fine), len(never)))
        if name == 'DEN':
            before = bytes(m.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(dmd.d)))
            rec = lambda n: [(m.rd(r[3], 4), m.rd(r[3] + 4, 4)) for r in dmd.records(n)]
            inf = 0xFFFFFFFF
            for far, want in ((0, {0x8002F50C: [(19600 * 16, 0)],
                                   0x8002F710: [(100000000 * 16, 360000), (360000, 0)],
                                   0x8002F7C4: [(100000000 * 16, 90000 * 16), (90000 * 16, 0)]}),
                              (1, {0x8002F50C: [(inf, 0)],
                                   0x8002F710: [(0, 360000), (inf, 0)],
                                   0x8002F7C4: [(0, 90000 * 16), (inf, 0)]})):
                m.wr(syms['portFarLod']['address'], 4, far)
                m.wr(syms['portFarLodNearScale']['address'], 4, 4)
                rv = s32(lm.run('portFarLodBegin'))
                got = {n: rec(n) for n in want}
                if rv == 0 or got != want:
                    bad.append('DEN, portFarLod %d: Begin %d, %s (want %s)' % (far, rv, {
                        '%08X' % n: v for n, v in got.items()}, {'%08X' % n: v for n, v in want.items()}))
                lm.run('portFarLodEnd')
                if bytes(m.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(dmd.d))) != before:
                    bad.append('DEN, portFarLod %d: the level differs from the file after End' % far)
    print('14. far LOD scan (requests 90, 91): %s scanned in the link as dmd_lod_dump.py lists them; Holland\'s grass '
          'field scaled with portFarLod 0 and at its finest level at every distance with portFarLod set -> %s' % (
              ', '.join(FAR_SCAN_LEVELS), 'ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


def check_special_profile(out):
    """Check 13 (request 73): the special profile's hooks in LinkMips on a made-up mod on Spectre."""
    from check_rollback import Image
    bad = []
    img = Image(out)
    sizes, syms = img.sizes, img.link.symbols
    need = ['portSpecialNum', 'portSpecialFireBegin', 'portSpecialFireEnd', 'portSpecialFirePickup',
            'portSpecialAiType', 'weaponBlockInit', 'aiCarSetupProfile']
    if any(n not in sizes for n in need) or 'portSpecialProfile' not in syms:
        print('13. mod car specials (request 73) -> FAIL: the hooks or portSpecialProfile are not in the link')
        return ['missing']
    launched, dropped, bomb_out = [], [], [-1]
    ids = {7: (1, 0), 51: (0, 1)}         # made-up object ids: human record 0, AI record 1

    def launch(m):
        launched.append((m.r[5], m.rd(hum + 0x7C8 + 9, 1)))
        return 0

    def find_by_id(m):
        if m.r[4] not in ids:
            return 0
        m.wr(m.r[5], 1, ids[m.r[4]][0])
        m.wr(m.r[6], 4, ids[m.r[4]][1])
        return 1

    def drop(m):
        dropped.append(m.r[5])
        return 0
    obj0 = 0x80E90000                     # human 0's car object (its id at +0x118)
    quiet = lambda m: 0
    lm = LinkMips(img, {'weaponLaunchProjectile': launch, 'carFindById': find_by_id, 'weaponDropItem': drop,
                        'weaponRemoteBombFind': lambda m: bomb_out[0], 'weaponRicochetSlotFind': lambda m: 2,
                        'humanCarObject': lambda m: obj0, 'viewGetSplitMode': quiet, 'soundLocateForView0': quiet,
                        'soundLocateForView': quiet, 'PadChkMtap': quiet, 'soundGetListenerPan': quiet,
                        'soundPlay': quiet, 'soundPlayVariantEx': quiet, 'portBalanceBombRefused': quiet})
    m = lm.m
    hum, ai = syms['humanCars']['address'], syms['aiCars']['address'] + 0x854
    prof, sprof = syms['portCarProfile']['address'], syms['portSpecialProfile']['address']
    as_at, owners = syms['portCarSpecialAs']['address'], syms['carSlotOwners']['address']
    mod, slot_h, slot_a, grp_h, grp_a = 5, 3, 4, 2, 6
    m.wr(syms['portCarGroups']['address'] + 4 * slot_h, 4, grp_h)
    m.wr(syms['portCarGroups']['address'] + 4 * slot_a, 4, grp_a)
    for g in (grp_h, grp_a):
        m.wr(syms['portCarNumbers']['address'] + 2 * g, 2, 0x20 + mod + 1)
    m.wr(syms['portModCars']['address'] + 0x20 * mod, 1, 1)
    m.wr(syms['portModCars']['address'] + 0x20 * mod + 1, 1, 7)       # base Spectre (car 7, type 0x80)
    m.wr(hum + 4, 2, slot_h)
    m.wr(ai + 4, 2, 1)
    m.wr(ai + 6, 2, slot_a)
    m.wr(ai + 0x30, 4, 0x80)
    for sl in (slot_h, slot_a):
        m.wr(owners + 4 * sl, 4, 0x80)
    P, SP = prof + PROFILE_BYTES * mod, sprof + SPECIAL_BYTES * mod
    m.wr(P, 1, 1)
    nums = [(1, 33), (3, 9000), (4, 500), (5, 99), (2, 4), (6, 0), (9, 9000), (10, 40), (11, 60), (12, 0), (13, 120)]
    m.wr(SP + 2, 1, len(nums))
    for i, (f, v) in enumerate(nums):
        m.wr(SP + 8 + 4 * i, 1, f)
        m.wr(SP + 10 + 4 * i, 2, v & 0xFFFF)
    num = lambda f, r: s32(lm.run('portSpecialNum', (slot_h, f, r)))
    got = [num(f, 1234) for f in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)]
    if got != [33, 4, 8191, 150, 99, 1, 1234, 1234, 6000, 40, 60, 1, 120]:
        bad.append('portSpecialNum %s (want 33 4 8191 150 99 1, unset 1234 1234, 6000 40 60 1 120)' % got)
    # stage 2: by object id, the blast, the boomerang's charge
    m.wr(syms['humanCarTotal']['address'], 4, 1)
    m.wr(syms['aiCarTotal']['address'], 4, 2)
    by_id = [s32(lm.run('portSpecialNumById', (i, 1, 25))) for i in (7, 99)]
    if by_id != [33, 25]:
        bad.append('portSpecialNumById %s (want 33 for the mod car\'s id, 25 for an unknown id)' % by_id)
    blast = [s32(lm.run('portSpecialBlast', (slot_h, big, r))) for big, r in ((0, 25), (0, 50), (1, 50))]
    if blast != [60, 85, 50]:
        bad.append('portSpecialBlast %s (want 60 85, a remote bomb 50)' % blast)
    for i, rec_at in ((7, hum), (51, ai)):
        before = m.rd(rec_at + 0x7C8 + 0x0E if rec_at == hum else ai + 0x808 + 0x0E, 2)
        lm.run('portSpecialBoomerangCaught', (i,))
        after = m.rd(rec_at + 0x7C8 + 0x0E if rec_at == hum else ai + 0x808 + 0x0E, 2)
        if after != before + 1:
            bad.append('portSpecialBoomerangCaught(%d): ammo %d -> %d' % (i, before, after))
    m.wr(P, 1, 0)
    if num(1, 15) != 15:
        bad.append('portSpecialNum without a valid profile is not retail\'s')
    m.wr(P, 1, 1)
    # the records a fire makes
    fx = syms['fxRecords']['address']
    rec = lambda i: fx + 980 * i
    for i in range(16):
        m.wr(rec(i) + 4, 2, 50 if i < 4 else 0)
    m.wr(rec(0) + 0x0A, 2, 6)
    lm.run('portSpecialFireBegin')
    for i, (t, d, l, sp, tr) in ((4, (61, 15, 200, 37, 15)), (5, (14, 7, 50, 50, 15))):
        for o, v in ((2, t), (0x0A, d), (4, l), (6, sp), (8, tr)):
            m.wr(rec(i) + o, 2, v)
    lm.run('portSpecialFireEnd', (slot_h, 0xE0))
    rd = lambda i, o: s32(m.rd(rec(i) + o, 2) | (0xFFFF0000 if m.rd(rec(i) + o, 2) & 0x8000 else 0))
    got = [(rd(i, 0x0A), rd(i, 4), rd(i, 6), rd(i, 8)) for i in (4, 5)] + [rd(0, 0x0A)]
    if got != [(33, 8191, 150, 98), (4, 8191, 150, 99), 6]:
        bad.append('the fire\'s records %s (want spiral 33/8191/150/98, freeze 4/8191/150/99, the old one 6)' % got)
    # the template at the start
    m.wr(P + 2, 1, 2)                       # request 70: recharge / max set
    m.wr(P + 6, 2, 500)
    m.wr(P + 0xA, 1, 4)

    def start(template, flags=2):
        m.wr(SP, 2, template)
        m.wr(P + 2, 1, flags)
        m.wr(as_at + 2 * slot_h, 2, 0xFFFF)
        lm.run('weaponBlockInit', (hum, hum + 0x7C8, 1))
        return (s32(m.rd(as_at + 2 * slot_h, 2) | (0xFFFF0000 if m.rd(as_at + 2 * slot_h, 2) & 0x8000 else 0)),
                m.rd(hum + 0x7C8 + 0x2C, 2), m.rd(hum + 0x7C8 + 0x0B, 1))
    m.wr(hum + 0x90, 4, 0x80)
    for template, flags, want in ((0x40, 2, (0x40, 500, 4)), (0x40, 0, (0x40, 700, 5)), (0x102, 0, (0x102, 700, 4)),
                                  (0xA0, 0, (-1, 700, 4))):
        got = start(template, flags)
        if got != want:
            bad.append('weaponBlockInit with template %X, flags %d: as %s (want %s)' % (template, flags, got, want))
    # the AI's tables
    w9 = lambda: m.rd(ai + 0x808 + 0x38, 4)
    m.wr(SP, 2, 0x40)
    if s32(lm.run('portSpecialAiType', (ai,))) != 0x40:
        bad.append('portSpecialAiType with Warthog\'s template is not 0x40')
    lm.run('aiCarSetupProfile', (ai,))
    if w9() != syms['aiWarthogWeights9']['address']:
        bad.append('aiCarSetupProfile with Warthog\'s template: tables %08X (want Warthog\'s)' % w9())
    m.wr(SP, 2, 0x101)
    lm.run('aiCarSetupProfile', (ai,))
    st = syms['portSpecialState']['address'] + 8 + 12 * mod
    own = syms['aiSpectreWeights9']['address']
    if w9() != st or m.rd(st, 1) != 50 or [m.rd(st + i, 1) for i in range(1, 9)] != [m.rd(own + i, 1) for i in range(1, 9)]:
        bad.append('aiCarSetupProfile with a pickup template: tables %08X, slot 0 %d' % (w9(), m.rd(st, 1)))
    # a pickup template's fire
    for v, flag, want in ((0x101, 0, (1, 0)), (0x102, 0, (2, 0)), (0x104, 0, (4, 0)), (0x105, 1, (5, 0)),
                          (0x10A, 0, (5, 1)), (0x164, 0, (100, 0))):
        del launched[:]
        m.wr(hum + 0x7C8 + 9, 1, flag)
        lm.run('portSpecialFirePickup', (hum, 1, v))
        if launched != [want] or m.rd(hum + 0x7C8 + 9, 1) != flag:
            bad.append('portSpecialFirePickup(%X): launched %s, flag after %d (want %s, %d)' % (
                v, launched, m.rd(hum + 0x7C8 + 9, 1), [want], flag))
    # stage 2: the ricochet and the remote bomb as templates
    m.wr(obj0 + 0x118, 4, 7)
    pool = syms['weaponRicochetPool']['address']
    nxt = min(v['address'] for v in syms.values() if v['address'] > pool)
    nric = (nxt - pool) // 52          # objects[N], shots[N]: 52 bytes a ricochet
    shot = pool + 4 * nric + 48 * 2
    m.wr(hum + 0x6FC, 4, 1000)
    lm.run('portSpecialFirePickup', (hum, 1, 0x106))
    got = (s32(m.rd(shot + 2, 2) | (0xFFFF0000 if m.rd(shot + 2, 2) & 0x8000 else 0)), m.rd(shot + 4, 2),
           m.rd(shot + 8 + 4, 4))
    if got != (7, 8191, 150):
        bad.append('the ricochet template: shot 2 owner %d life %d speed %d (want 7, 8191, 150)' % got)
    a0 = lambda: m.rd(hum + 0x7C8 + 0x0E, 2)
    m.wr(hum + 0x7C8 + 0x0E, 2, 3)
    del dropped[:]
    bomb_out[0] = -1
    lm.run('portSpecialFirePickup', (hum, 1, 0x103))
    placed = (a0(), m.rd(hum + 0x7C8 + 6, 1), list(dropped))
    bomb_out[0] = 0
    lm.run('portSpecialFirePickup', (hum, 1, 0x103))
    blown = (a0(), m.rd(hum + 0x7C8 + 6, 1), list(dropped))
    if placed != (4, 1, [3]) or blown != (4, 0, [3, 3]):
        bad.append('the remote bomb template: placed %s, detonated %s (want ammo 4 armed 1, then 4 / 0, kind 3 both)' %
                   (placed, blown))
    print('13. mod car specials (request 73): numbers and limits, the fire\'s records, the template at the start, the AI '
          'tables, the pickup fire; stage 2: by object id, the blast, the boomerang\'s charge, the ricochet and the '
          'remote bomb -> %s' % ('ok' if not bad else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return bad


FALL_RECORDS = {   # request 107: function, record fields (car_tick.c, ai_driver.c; include/semantic/cars.h)
    'humanCarTick': dict(falling=0x0BB, hp=0x1F0, ride=0x18E, fps=0x160, pos_z=0x704, start=None),
    'aiCarDrive': dict(falling=0x08B, hp=0x5A0, ride=0x53E, fps=0x510, pos_z=0x744, start=0x000)}
FALL_CASES = ((116, 150), (116, 196), (116, 197), (15, 90), (15, 94), (15, 95), (32, 60), (116, 1000))


def check_fall_wheels(out):
    """Request 107 (RETAIL_BUGS 40): both fall-death tests (humanCarTick, aiCarDrive) in LinkMips, every call stubbed
    (worldFallLimitZ 0xF00, worldFallDeathZ 80, the frame rate 30): a falling car dies when its body's height less
    its ride height is under 80; a car not yet falling is only marked falling."""
    from check_rollback import Image
    img = Image(out)
    at, death = 0x80A00000, 80
    bad, runs = [], 0
    for fn, f in FALL_RECORDS.items():
        if fn not in img.sizes:
            bad.append('%s not in the link' % fn)
            continue
        stubs = {n: (lambda m: 0) for n in img.sizes if n != fn}
        stubs.update(worldFallLimitZ=lambda m: 0xF00, worldFallDeathZ=lambda m: death, timerGetFrameRate=lambda m: 30)
        for falling in (1, 0):
            for ride, z in FALL_CASES:
                lm = LinkMips(img, stubs)
                m = lm.m
                m.fill(at, bytes(0x900))
                if f['start'] is not None:
                    m.wr(at + f['start'], 1, 1)
                m.wr(at + f['falling'], 1, falling)
                m.wr(at + f['hp'], 4, 100)
                m.wr(at + f['ride'], 2, ride)
                m.wr(at + f['fps'], 2, 30)
                m.wr(at + f['pos_z'], 4, z)
                lm.run(fn, (at,), steps=200000)
                runs += 1
                hp, fl = s32(m.rd(at + f['hp'], 4)), m.rd(at + f['falling'], 1)
                want = (-1 if z - ride < death else 100) if falling else 100
                if hp != want or fl != 1:
                    bad.append('%s falling %d ride %d z %d: hp %d falling %d (want hp %d falling 1)' % (
                        fn, falling, ride, z, hp, fl, want))
    good = not bad
    print('20. request 107: the pit death from the wheels (body height less ride height under 80) in humanCarTick and '
          'aiCarDrive, %d runs -> %s' % (runs, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


POSE_RECORDS = {   # request 108: the record blocks (include/semantic/cars.h), the slot tested
    0: dict(array='humanCars', size=0x814, state=0x09C, input=0x0D0, aim=0x114, stats=0x14C, wheels=0x5A4,
            motion=0x6DC, type=0x090, objects='humanCarObjects', index=1),
    1: dict(array='aiCars', size=0x854, state=0x06C, input=0x044, aim=0x09C, stats=0x4FC, wheels=0x5E4,
            motion=0x71C, type=0x030, objects='aiCarObjects', index=3)}
POSE_SHADOW_UP = (0xA0, 0xB0, 0x40, 0xC0, 0x60)
POSE_BODY = 0x3C8          # struct TM2PortCarGroups .body (3 * 64 + 48 ints + 2)


def check_car_pose(out):
    """Request 108 (CAR_POSE.md): portCarPose in LinkMips on records full of junk (human 1, computer 3; a plain car,
    a raised-shadow car, Dark Tooth, the boss beaten; turbo on and off; four headings): the record still and level
    on the floor, its matrices level (the link's RotMatrixYXZ), the wheels on the floor, the shadow, the car's object
    and the head; the turbo flame turned off once, no rand(), no sound; an index out of range writes nothing. The
    matrix products are Python stubs (no GTE)."""
    import random
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('portCarPose', 'humanCars', 'aiCars', 'humanCarObjects', 'aiCarObjects', 'portCarGroups',
            'darkToothBossObject')
    miss = [n for n in need if n not in syms]
    if miss:
        print('21. request 108 car pose -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    A = lambda n: syms[n]['address']
    OBJ, BODY, HEAD = 0x80A00000, 0x80A00200, 0x80A00400
    rng = random.Random(108)
    bad, runs = [], 0

    def s16(v):
        v &= 0xFFFF
        return v - 0x10000 if v & 0x8000 else v

    def mat(m, a):
        return [[s16(m.rd(a + 2 * (3 * r + c), 2)) for c in range(3)] for r in range(3)]

    def mul(m, tr):
        M, v = mat(m, m.r[4]), [s32(m.rd(m.r[5] + 4 * k, 4)) for k in range(3)]
        o = [sum((M[c][r] if tr else M[r][c]) * v[c] for c in range(3)) >> 12 for r in range(3)]
        for k in range(3):
            m.wr(m.r[6] + 4 * k, 4, o[k] & 0xFFFFFFFF)

    cases = []
    for kind in (0, 1):
        for type_, ride in ((0x10, 15), (0xA0, 26), (0xF0, 116)):
            for beaten in ((0, 1) if kind == 1 and type_ == 0xF0 else (0,)):
                for turbo in (1, 0):
                    for yaw in (0, 1000, -1500, 3000):
                        cases.append((kind, type_, ride, beaten, turbo, yaw))
    for kind, type_, ride, beaten, turbo, yaw in cases:
        f = POSE_RECORDS[kind]
        calls = []
        stubs = {'mathMulMatVec': lambda m: mul(m, False), 'mathMulTransVec': lambda m: mul(m, True),
                 'portCarKey': lambda m: m.r[5], 'portCarSpecialSelect': lambda m: 0,
                 'portCarSpecialRestore': lambda m: 0, 'aiBossDefeated': lambda m, b=beaten: b,
                 'portCarTurboSetStep': lambda m: calls.append(('turbo', m.r[4], m.r[5])),
                 'rand': lambda m: calls.append(('rand',)), 'soundPlay': lambda m: calls.append(('sound',))}
        lm = LinkMips(img, stubs)
        m = lm.m
        rec = A(f['array']) + f['index'] * f['size']
        m.fill(rec, bytes(rng.randrange(256) for _ in range(f['size'])))
        for a in (OBJ, BODY, HEAD):
            m.fill(a, bytes(rng.randrange(256) for _ in range(0x200)))
        m.wr(A(f['objects']) + 4 * f['index'], 4, OBJ)
        m.wr(A('portCarGroups') + POSE_BODY + 4 * (type_ >> 4), 4, BODY)
        m.wr(A('darkToothBossObject'), 4, HEAD)
        st, mo, sb, wh = rec + f['state'], rec + f['motion'], rec + f['stats'], rec + f['wheels']
        m.wr(rec + f['type'], 4, type_)
        m.wr(sb + 0x42, 2, ride)
        m.wr(sb + 0x48, 2, 100)
        m.wr(sb + 0x4E, 2, 120)
        m.wr(sb + 0x50, 2, -110 & 0xFFFF)
        m.wr(st + 0x1C, 1, turbo)
        x, y, floor = rng.randrange(-20000, 20000), rng.randrange(-20000, 20000), rng.choice((260, 190, 130, -500))
        ret = lm.run('portCarPose', (kind, f['index'], x, y, floor, yaw), steps=400000)
        runs += 1
        name = '%s %02X ride %d%s turbo %d yaw %d' % (('human', 'AI')[kind], type_, ride, ' beaten' if beaten else '',
                                                      turbo, yaw)
        p = []
        rot = [s32(m.rd(mo + 0x2C + 4 * k, 4)) for k in range(3)]
        if ret != 1:
            p.append('returned %d' % s32(ret))
        if rot[:2] != [0, 0] or (rot[2] - yaw) % 4096 or not -4096 < rot[2] < 4096:
            p.append('rot %s' % rot)
        if [s32(m.rd(mo + 0x38 + 4 * k, 4)) for k in range(3)] != rot:
            p.append('body_rot')
        pos = [s32(m.rd(mo + 0x20 + 4 * k, 4)) for k in range(3)]
        if pos != [x, y, floor + ride]:
            p.append('pos %s' % pos)
        if any(m.rd(mo + o, 2) for o in (0x00, 0x08, 0x0A, 0x0C, 0x10, 0x12, 0x14, 0x18, 0x1A, 0x1C)) or \
                any(m.rd(mo + o, 4) for o in (0x44, 0x48, 0x4C, 0xE8)):
            p.append('a speed or spin left')
        if kind == 0 and m.rd(rec + 0x8C, 4):
            p.append('speed_change left')
        rm = [m.rd(mo + 0x50 + k, 1) for k in range(18)]
        if [m.rd(mo + 0x70 + k, 1) for k in range(18)] != rm or [m.rd(mo + 0x90 + k, 1) for k in range(18)] != rm:
            p.append('body_m / body_rot_m not rot_m (a tilt left)')
        R = mat(m, mo + 0x50)
        if R[2] != [0, 0, 4096] or R[0][2] or R[1][2] or (yaw == 0 and R != [[4096, 0, 0], [0, 4096, 0], [0, 0, 4096]]):
            p.append('rot_m not level %s' % R)
        for k in range(4):
            e = wh + 0x18 + 0x48 * k
            v = [50 if k & 1 else -50, 120 if k < 2 else -110, -ride]
            corner = [sum(R[c][r] * v[c] for c in range(3)) >> 12 for r in range(3)]
            if [s32(m.rd(e + 0x30 + 4 * j, 4)) for j in range(3)] != corner or \
                    [s32(m.rd(e + 0x3C + 4 * j, 4)) for j in range(3)] != [pos[j] + corner[j] for j in range(3)]:
                p.append('wheel %d corner / pos' % k)
            if pos[2] + corner[2] != floor:
                p.append('wheel %d not on the floor' % k)
            if any(s16(m.rd(e + o, 2)) != floor for o in (0x16, 0x1C, 0x24, 0x26, 0x2A)) or \
                    any(m.rd(e + o, 2) for o in (0x12, 0x14, 0x18, 0x1A, 0x1E, 0x20, 0x22, 0x2C)) or \
                    m.rd(e, 1) != 1 or m.rd(e + 1, 1) or m.rd(e + 6, 1) or m.rd(e + 7, 1):
                p.append('wheel %d fields' % k)
        if any(m.rd(wh + 0x0C + 4 * k, 4) for k in range(3)):
            p.append('wheels move')
        if any(m.rd(st + o, 1) for o in (0x01, 0x02, 0x0B, 0x0C, 0x18, 0x1C, 0x1F)) or m.rd(rec + f['input'] + 3, 1):
            p.append('a state flag left (smoke, turbo, air, falling)')
        if any(m.rd(rec + f['aim'] + o, 4) for o in (0x00, 0x04)) or \
                any(m.rd(rec + f['aim'] + o, 2) for o in (0x0C, 0x0E, 0x16, 0x18, 0x1C, 0x1E, 0x28, 0x2A, 0x30, 0x32)):
            p.append('a tilt or shake left')
        want_calls = [('turbo', type_, 0)] if turbo else []
        if calls != want_calls:
            p.append('calls %s (want %s)' % (calls, want_calls))
        shadow_t = [s32(m.rd(BODY + 0x18 + 4 * k, 4)) for k in range(3)]
        if mat(m, BODY + 4) != R or shadow_t != [x, y, floor + (3 if type_ in POSE_SHADOW_UP else 0)]:
            p.append('shadow %s t %s' % (mat(m, BODY + 4), shadow_t))
        if [s16(m.rd(OBJ + 0xA0 + 2 * k, 2)) for k in range(3)] != [s16(v) for v in rot] or \
                [s32(m.rd(OBJ + 0xA8 + 4 * k, 4)) for k in range(3)] != pos or \
                [m.rd(OBJ + 0x80 + k, 1) for k in range(32)] != [m.rd(mo + 0x70 + k, 1) for k in range(32)]:
            p.append('the car object')
        head = [s32(m.rd(HEAD + 0xA8 + 4 * k, 4)) for k in range(3)]
        if type_ == 0xF0 and head != [x, y, floor + ride + (-24 if beaten else 240)]:
            p.append('the head at %s' % head)
        if p:
            bad.append('%s: %s' % (name, ', '.join(p)))
    for kind, index in ((0, -1), (1, -1), (2, 0)):
        lm = LinkMips(img, {'rand': lambda m: 0})
        m = lm.m
        ret = lm.run('portCarPose', (kind, index, 1, 2, 3, 4), steps=10000)
        runs += 1
        wrote = [a for a in m.mem if not 0x801F0000 <= a < 0x80200000]     # (its own stack frame aside)
        if ret != 0 or wrote:
            bad.append('kind %d index %d: returned %d, %d bytes written' % (kind, index, s32(ret), len(wrote)))
    good = not bad
    print('21. request 108: portCarPose (%d runs): still, level, wheels on the floor, the shadow, the car object, the '
          'head; the turbo off once, no rand() or sound; a bad index writes nothing -> %s' % (
              runs, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


SOUND_RETAIL = (0x25, 0x27, 0x29, 0x2B, 0x2D, 0x2F, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3B,   # request 108's addendum:
                0x3D, 0x0D, 0x42)   # cars 0..14 (Hammerhead .. Shadow, Sweet Tooth, Minion, Dark Tooth): the special's
SOUND_PICKUP = {0x101: 10, 0x102: 11, 0x103: 0x13, 0x104: 12, 0x105: 15, 0x106: 0x12, 0x10A: 15, 0x164: 14}


def check_special_sound(out):
    """Request 108's addendum (CAR_POSE.md "Each car's special sound"): portCarSpecialSound in LinkMips: cars 0..14
    their special's soundPlay code (weapon_basic.c, car_crash.c, weapon_arcs_draw.c, the swoop), out-of-range numbers
    -1; with mod cars (TM2_PORT_CAR_NUMBERS) an unused mod -1, a used one its base's, with a profile
    (TM2_PORT_SPECIAL_PROFILE) its template's (a borrowable type, a pickup) or its base's for a refused one; nothing
    written but its own stack, no sound played. portCarSpecialSoundHeld against the link's sndEffectTable: held = kind
    3 or 8, every code it returns a one-shot (12, 13) or held."""
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('portCarSpecialSound', 'portCarSpecialSoundHeld', 'sndEffectTable', 'soundPlay')
    miss = [n for n in need if n not in syms]
    if miss:
        print('22. request 108 special sound -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    A = lambda n: syms[n]['address']
    mods = 'portModCars' in syms and 'portCarNumberType' in syms
    prof = mods and 'portSpecialProfile' in syms and 'portCarProfile' in syms
    bad, runs = [], 0

    def run(car, setup=None):
        played = []
        lm = LinkMips(img, {'soundPlay': lambda m: played.append(m.r[4]), 'rand': lambda m: played.append('rand')})
        m = lm.m
        if setup:
            setup(m)
        before = set(m.mem)
        ret = s32(lm.run('portCarSpecialSound', (car,), steps=20000))
        wrote = [a for a in m.mem if a not in before and not 0x801F0000 <= a < 0x80200000]
        if played or wrote:
            bad.append('car %#x: played %s, %d bytes written' % (car, played, len(wrote)))
        return ret

    def held(code):
        lm = LinkMips(img, {})
        return s32(lm.run('portCarSpecialSoundHeld', (code,), steps=2000))

    for car, want in enumerate(SOUND_RETAIL):
        runs += 1
        got = run(car)
        if got != want:
            bad.append('car %d: %d (want %d)' % (car, got, want))
    for car in (-1, 15, 0x1F, -0x8000, 0x7FFFFFFF) + (() if mods else (0x20, 0x9F)):
        runs += 1
        if run(car) != -1:
            bad.append('car %#x: not -1' % car)
    if mods:
        MOD, PROF, SPEC = A('portModCars'), (A('portCarProfile') if prof else 0), (A('portSpecialProfile') if prof else 0)

        def mod(m_, base, valid=0, special=0):
            def setup(m):
                m.wr(MOD + 0x20 * m_, 1, 1)
                m.wr(MOD + 0x20 * m_ + 1, 1, base)
                if prof:
                    m.wr(PROF + 0xA0 * m_, 1, valid)
                    m.wr(SPEC + 0x48 * m_, 2, special & 0xFFFF)
            return setup
        runs += 2
        if run(0x20 + 5) != -1 or run(0xA0) != -1:
            bad.append('an unused mod or 0xA0 not -1')
        cases = [(0, b, 0, 0, SOUND_RETAIL[b]) for b in range(15)]       # the base's own
        cases += [(127, 3, 0, 0x101, SOUND_RETAIL[3])]                   # a profile not valid: the base's
        if prof:
            cases += [(9, 0, 1, s, SOUND_PICKUP[s]) for s in sorted(SOUND_PICKUP)]
            cases += [(10, 0, 1, t, SOUND_RETAIL[{0x10: 12, 0xE0: 13}.get(t, (t >> 4) - 2)]) for t in
                      (0x10, 0x30, 0x40, 0x50, 0x70, 0x80, 0x90, 0xB0, 0xD0, 0xE0)]     # TM2_PORT_SPECIAL_AS_MASK
            cases += [(11, 2, 1, t, SOUND_RETAIL[2]) for t in (0x20, 0x60, 0xA0, 0xC0, 0xF0, 0x15, 0x100, 0x107, -1)]
        for m_, base, valid, special, want in cases:
            runs += 1
            got = run(0x20 + m_, mod(m_, base, valid, special))
            if got != want:
                bad.append('mod %d base %d profile %d special %#x: %d (want %d)' % (m_, base, valid, special, got,
                                                                                  want))
    codes = set(SOUND_RETAIL) | set(SOUND_PICKUP.values())
    for code in range(67):
        runs += 1
        kind = s32(img.link.word(A('sndEffectTable') + 20 * code + 12))
        h = held(code)
        if code in codes and (h != (kind in (3, 8)) or kind not in (3, 8, 12, 13)):
            bad.append('code %#x: held %d, the table kind %d' % (code, h, kind))
        elif code not in codes and h:
            bad.append('code %#x held (no special plays it)' % code)
    good = not bad
    print('22. request 108 addendum: portCarSpecialSound (%d runs%s): every car\'s special sound, -1 for no car, '
          'nothing written or played; Held matches the sound table -> %s' % (
              runs, ', mods' + (' and profiles' if prof else '') if mods else '', 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


def check_whole_maps(out):
    """Request 111 (RETAIL_BUGS.md 41): worldCollapseClearDone in LinkMips (gameGetMode, worldSectionDisable and
    worldCollapseUnlink stubbed and recorded): in modes 2 and 4 the six first floes are taken only when the load found
    none of them (GLACIER2.DMD, retail's), never when it found them (the whole GLACIER.DMD); other modes as retail;
    floes already done (the counter) always cleared."""
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('worldCollapseClearDone', 'worldCollapseSections', 'worldCollapseTriggersDone')
    miss = [n for n in need if n not in syms]
    if miss:
        print('23. request 111 whole maps -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    SEC, DONE = syms['worldCollapseSections']['address'], syms['worldCollapseTriggersDone']['address']
    bad, runs = [], 0
    for mode in (1, 2, 3, 4):
        for have in ((), (0, 1, 2, 3, 4, 5), (3,)):
            for done in (0, 3, 6, 9):
                calls = []
                lm = LinkMips(img, {'gameGetMode': lambda m, v=mode: v,
                                    'worldSectionDisable': lambda m: calls.append(('off', s32(m.r[4]))),
                                    'worldCollapseUnlink': lambda m: calls.append(('unlink', s32(m.r[4])))})
                m = lm.m
                for i in range(35):
                    m.wr(SEC + 36 * i + 0x18, 1, 1 if i in have else 0)
                m.wr(DONE, 4, done)
                lm.run('worldCollapseClearDone', (), steps=20000)
                runs += 1
                n = 6 if mode in (2, 4) and done < 6 and not have else done
                want = [c for i in range(n) for c in (('unlink', i), ('off', i))]
                if calls != want or s32(m.rd(DONE, 4)) != n or \
                        any(m.rd(SEC + 36 * i + 0x18, 1) != (0 if i < n else 1 if i in have else 0) for i in range(35)):
                    bad.append('mode %d floes %s done %d: calls %s, counter %d (want %d floes)' % (
                        mode, list(have), done, calls[:4], s32(m.rd(DONE, 4)), n))
    good = not bad
    print('23. request 111: worldCollapseClearDone (%d runs): modes 2 and 4 take the six first floes only when the '
          'file has none of them -> %s' % (runs, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


# Request 112 (RETAIL_BUGS.md 42 and 43): the surfaces portFwyGroundAdd inserts, a copy of portFwyGroundTable
# (arena_load_register.c): (section, its last disc surface, centre, [(radius from, to, angle from, to, h0..h3)]).
FWY_GROUND = [
    (0, 51, (-5376, 2688), [
        (2234, 2391, -597, -461, 96, 96, 41, 44), (2234, 2398, -814, -651, 96, 96, 19, 33), (2234, 2394, -651, -597, 96, 96, 33, 41),
        (2234, 2397, -461, -298, 96, 96, 44, 26), (2234, 2399, -298, -190, 96, 96, 26, 16), (2387, 2482, -624, -461, 36, 44, 71, 80),
        (2389, 2482, -461, -407, 44, 36, 80, 71), (2476, 2567, -570, -461, 80, 80, 116, 116), (2478, 2569, -597, -570, 76, 80, 112, 116),
        (2479, 2570, -461, -434, 80, 76, 116, 112), (2965, 3078, -868, -841, 131, 136, 145, 153), (2973, 3038, -950, -922, 44, 61, 113, 119),
        (2974, 3062, -82, -54, 44, 26, 114, 108), (2974, 3070, -976, -949, 26, 44, 107, 113), (2971, 3092, -163, -136, 131, 126, 146, 138),
        (2975, 3109, -1004, -976, 9, 26, 101, 107), (2975, 3097, -895, -868, 125, 131, 137, 145), (2975, 3099, -54, -27, 26, 9, 108, 102),
        (2975, 3142, -1024, -1003, 0, 9, 96, 101), (2976, 3142, -27, 0, 9, 0, 102, 96), (2990, 3122, -922, -895, 119, 125, 129, 137),
        (2985, 3115, -136, -108, 126, 120, 138, 130), (3012, 3153, -950, -922, 113, 119, 120, 129), (3006, 3145, -109, -81, 120, 114, 130, 122),
        (3031, 3179, -82, -54, 114, 108, 122, 113), (3038, 3190, -976, -950, 107, 113, 112, 120), (3062, 3221, -54, -27, 108, 102, 113, 105),
        (3070, 3233, -1004, -976, 101, 107, 103, 112), (3099, 3270, -27, 0, 102, 96, 105, 96), (3190, 3234, -1004, -976, 135, 112, 135, 112),
        (3179, 3224, -54, -26, 113, 137, 113, 137),
    ]),
    (13, 103, (5376, -2688), [
        (2234, 2391, 1451, 1587, 96, 96, 41, 44), (2234, 2397, 1289, 1451, 96, 96, 24, 41), (2234, 2397, 1587, 1750, 96, 96, 44, 26),
        (2234, 2398, 1750, 1804, 96, 96, 26, 21), (2973, 3038, 1098, 1126, 44, 61, 113, 119), (2974, 3062, 1966, 1994, 44, 26, 114, 108),
        (2974, 3070, 1072, 1099, 26, 44, 107, 113), (2971, 3092, 1885, 1912, 131, 126, 146, 138), (2975, 3109, 1044, 1072, 9, 26, 101, 107),
        (2975, 3097, 1153, 1180, 125, 131, 137, 145), (2975, 3099, 1994, 2021, 26, 9, 108, 102), (2975, 3142, 1024, 1045, 0, 9, 96, 101),
        (2976, 3142, 2021, 2048, 9, 0, 102, 96), (2990, 3122, 1126, 1153, 119, 125, 129, 137), (2985, 3115, 1912, 1940, 126, 120, 138, 130),
        (3012, 3153, 1098, 1126, 113, 119, 120, 129), (3006, 3145, 1939, 1967, 120, 114, 130, 122), (3031, 3179, 1966, 1994, 114, 108, 122, 113),
        (3038, 3190, 1072, 1098, 107, 113, 112, 120), (3062, 3221, 1994, 2021, 108, 102, 113, 105), (3070, 3233, 1044, 1072, 101, 107, 103, 112),
        (3099, 3270, 2021, 2048, 102, 96, 105, 96), (3190, 3234, 1044, 1072, 135, 112, 135, 112), (3179, 3224, 1994, 2022, 113, 137, 113, 137),
    ]),
    (12, 127, (-5376, -2688), [
        (2234, 2391, -1621, -1485, 96, 96, 41, 44), (2234, 2397, -1783, -1621, 96, 96, 24, 41), (2234, 2397, -1485, -1322, 96, 96, 44, 26),
        (2234, 2398, -1322, -1268, 96, 96, 26, 21), (2973, 3038, -1974, -1946, 44, 61, 113, 119), (2974, 3062, -1106, -1078, 44, 26, 114, 108),
        (2974, 3070, -2000, -1973, 26, 44, 107, 113), (2971, 3092, -1187, -1160, 131, 126, 146, 138), (2975, 3109, -2028, -2000, 9, 26, 101, 107),
        (2975, 3097, -1919, -1892, 125, 131, 137, 145), (2975, 3099, -1078, -1051, 26, 9, 108, 102), (2975, 3142, -2048, -2027, 0, 9, 96, 101),
        (2976, 3142, -1051, -1024, 9, 0, 102, 96), (2990, 3122, -1946, -1919, 119, 125, 129, 137), (2985, 3115, -1160, -1132, 126, 120, 138, 130),
        (3012, 3153, -1974, -1946, 113, 119, 120, 129), (3006, 3145, -1133, -1105, 120, 114, 130, 122), (3031, 3179, -1106, -1078, 114, 108, 122, 113),
        (3038, 3190, -2000, -1974, 107, 113, 112, 120), (3062, 3221, -1078, -1051, 108, 102, 113, 105), (3070, 3233, -2028, -2000, 101, 107, 103, 112),
        (3099, 3270, -1051, -1024, 102, 96, 105, 96), (3190, 3234, -2028, -2000, 135, 112, 135, 112), (3179, 3224, -1078, -1050, 113, 137, 113, 137),
    ]),
]
# drawn tops of the corner curves' lips and kerbs that sank (FREEWAY.DMD, PLAYTEST_ANSWERS.md 31): x, y, drawn height
FWY_LIP_POINTS = [
    (-8493, 2946, 112), (-8454, 3086, 121), (-8426, 3111, 122), (-8397, 2938, 83), (-8351, 3270, 129), (-7358, 4012, 38),
    (-7241, 4171, 44), (-7153, 4240, 54), (-7066, 4368, 48), (-6986, 4379, 63), (-6889, 4498, 53), (-6752, 4604, 48),
    (-5944, 5641, 125), (-5846, 5731, 124), (-5759, 5719, 116), (-5649, 5755, 110), (5499, -5789, 102), (5638, -5732, 104),
    (5719, -5700, 104), (5781, -5741, 119), (5853, -5657, 117), (5916, -5695, 127), (6746, -4609, 48), (6917, -4475, 54),
    (7012, -4354, 63), (7102, -4262, 63), (7186, -4164, 62), (7322, -4023, 48), (8369, -3169, 122), (8401, -3117, 120),
    (8421, -3149, 124), (8444, -3152, 126), (-8493, -2946, 111), (-8466, -2973, 111), (-8440, -2999, 112), (-8416, -2996, 111),
    (-8386, -3049, 110), (-8338, -3211, 122), (-7181, -4207, 53), (-7080, -4320, 55), (-6986, -4379, 63), (-6874, -4480, 62),
    (-6699, -4642, 48), (-5921, -5670, 126), (-5846, -5731, 125), (-5768, -5791, 122), (-5706, -5750, 114), (-5621, -5758, 109),
]


def check_fwy_ground(out):
    """Request 112 (RETAIL_BUGS.md 42 and 43): arenaLoadLosAngeles in LinkMips over the disc's FWY.GRP and FWY.RMP
    (the route, GRP and RMP loaders stubbed to write the files): the 225 surface records and the GRP exactly as
    FWY_GROUND inserted after each section's last disc surface gives them (the disc's records unchanged, only
    renumbered; every section's range moved with them); then the link's worldGroundHeightAt on that ground against
    the disc's own: within 12 of the drawn top on every lip point (the disc's at
    least 16 below), the disc's height everywhere else (a grid over the map away from the changed places, the deck,
    by the wall, the gap, the corners' middles)."""
    import math
    import struct
    from check_rollback import Image
    from dmd_lod_dump import Disc, DEFAULT_BIN
    img = Image(out)
    syms = img.link.symbols
    need = ('arenaLoadLosAngeles', 'portFwyGroundAdd', 'worldSurfaces', 'worldSectionGrpFile',
            'worldSectionFirstSurface', 'worldSectionLastSurface', 'worldGroundHeightAt', 'terrainSectionCount')
    miss = [n for n in need if n not in syms]
    if miss:
        print('24. request 112 LA ground -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    A = lambda n: syms[n]['address']
    SURF, GRP, FIRST, LAST = A('worldSurfaces'), A('worldSectionGrpFile'), A('worldSectionFirstSurface'), \
        A('worldSectionLastSurface')
    disc = Disc(DEFAULT_BIN)
    grp, rmp = disc.get('/TERRAIN/FWY.GRP'), disc.get('/TERRAIN/FWY.RMP')
    count = grp[0x801AAFD8 - 0x801A9848]
    bad, runs = [], 0

    def put(m, base, data):
        for k, b in enumerate(data):
            m.wr(base + k, 1, b)

    # the expected layout
    first, last = FIRST - GRP, LAST - GRP
    for sec, at, c, L in FWY_GROUND:
        if grp[last + sec] != at:
            bad.append('section %d ends at %d on the disc (the table says %d)' % (sec, grp[last + sec], at))
    new = []
    for k in range(128):
        new.append(rmp[28 * k:28 * k + 28])
        for sec, at, c, L in FWY_GROUND:
            if at == k:
                new += [struct.pack('<BBhhhhhhhhhii', 2, 1, -1, *e, c[0], c[1]) for e in L]
    want_rmp = b''.join(new) + rmp[28 * len(new):]
    want_grp = bytearray(grp)
    for i in range(count):
        f, l = grp[first + i], grp[last + i]
        if f <= l:
            want_grp[first + i] = f + sum(len(L) for s, at, c, L in FWY_GROUND if at < f)
            want_grp[last + i] = l + sum(len(L) for s, at, c, L in FWY_GROUND if at <= l)

    def grp_load(m):
        m.wr(A('terrainSectionCount'), 4, count)
        put(m, GRP, grp)

    stubs = {'routeLoadFwy': lambda m: None, 'viewSetBgColor': lambda m: None,
             'terrainLoadGrpFwy': grp_load, 'terrainLoadRmpFwy': lambda m: put(m, SURF, rmp)}
    lm = LinkMips(img, stubs)
    m = lm.m
    lm.run('arenaLoadLosAngeles', (), steps=2000000)
    runs += 1
    got_rmp = bytes(m.rd(SURF + k, 1) for k in range(len(rmp)))
    got_grp = bytes(m.rd(GRP + k, 1) for k in range(len(grp)))
    if got_grp != bytes(want_grp):
        diff = [k for k in range(len(grp)) if got_grp[k] != want_grp[k]]
        bad.append('GRP: %d bytes differ (first at +%#x: %d, want %d)' % (len(diff), diff[0], got_grp[diff[0]],
                                                                          want_grp[diff[0]]))
    if got_rmp != want_rmp:
        diff = [k // 28 for k in range(len(rmp)) if got_rmp[k] != want_rmp[k]]
        bad.append('surfaces: %d records differ (first %d: %s, want %s)' % (
            len(set(diff)), diff[0], got_rmp[28 * diff[0]:28 * diff[0] + 28].hex(),
            want_rmp[28 * diff[0]:28 * diff[0] + 28].hex()))

    def memcpy(m):                      # a BIOS call (A0 table) in the link: no BIOS here
        for k in range(m.r[6]):
            m.wr(m.r[4] + k, 1, m.rd(m.r[5] + k, 1))
        return m.r[4]

    def heights(grp_bytes, rmp_bytes, points):
        # SquareRoot0 uses the GTE (LZCS): an integer root instead, the same in both runs
        lm = LinkMips(img, {'memcpy': memcpy, 'sub_8013FC44': lambda m: math.isqrt(s32(m.r[4]) & 0x7FFFFFFF)})
        m = lm.m
        put(m, GRP, grp_bytes)
        put(m, SURF, rmp_bytes)
        res = []
        for p in points:
            for k in range(3):
                m.wr(0x801F8000 + 4 * k, 4, p[k] & 0xFFFFFFFF)
            res.append(s32(lm.run('worldGroundHeightAt', (0x801F8000,), steps=4000000)))
        return res

    def polar(c, r, a, z):
        t = a * 2 * math.pi / 4096
        return (c[0] + int(round(r * math.sin(t))), c[1] + int(round(r * math.cos(t))), z)

    NE = (5376, 2688)
    corners = [c for s, at, c, L in FWY_GROUND if c != NE]
    tops = []
    lips = [(x, y, z + 30) for x, y, z in FWY_LIP_POINTS]
    same = [polar(NE, r, s * a, z) for s in (1, -1) for a in (200, 512, 820) for r in (2300, 2800, 3110, 3140)
            for z in (340, 400)]                                     # the deck, by the wall too
    same += [polar(NE, r, s * a, z) for s in (1, -1) for a in (200, 512, 820) for r in (3160, 3270)
             for z in (460, 600)]                                    # the hedge-wall tops: retail (unreachable)
    same += [polar(NE, r, a, z) for a in (-100, 0, 100) for r in (2600, 3200) for z in (360, 600)]    # the gap
    same += [polar(NE, r, a, 200) for a in (-512, 512) for r in (2900, 3200, 3400)]                    # under it
    same += [polar(c, r, a, z) for c in corners for a in range(0, 4096, 256) for r in (1500, 2700)
             for z in (100, 400)]                                   # inside the corners and their banks' middles
    grid = [(x, y, z) for x in range(-8000, 8001, 1000) for y in range(-5500, 5501, 917) for z in (100, 500)]
    same += [p for p in grid if all(not 2150 <= math.hypot(p[0] - c[0], p[1] - c[1]) <= 3450 for c in corners + [NE])]
    pts = tops + lips + same
    old = heights(grp, rmp, pts)
    newh = heights(got_grp, got_rmp, pts)
    runs += 2 * len(pts)
    for (x, y, z), o, n in zip(FWY_LIP_POINTS, old[len(tops):], newh[len(tops):]):
        if abs(n - z) > 12 or o > z - 16:
            bad.append('lip (%d, %d) drawn %d: %d (the disc %d)' % (x, y, z, n, o))
    k0 = len(tops) + len(lips)
    for p, o, n in zip(same, old[k0:], newh[k0:]):
        if n != o:
            bad.append('%s: %d (the disc %d)' % (p, n, o))
    good = not bad
    print('24. request 112: arenaLoadLosAngeles, portFwyGroundAdd and worldGroundHeightAt (%d runs): %d surfaces '
          'added, the curves\' lips at their drawn height (%d points), the disc\'s ground everywhere else '
          '(%d points) -> %s' % (runs, sum(len(L) for s, at, c, L in FWY_GROUND), len(lips), len(same),
                                 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


# check 25 (requests 113 and 113b, HIDDEN_AREAS.md 16): the faces portTwoSidedBegin makes two-sided, per level file (control word
# addresses, primitive +12; each group is set only whole): the intact state (child 0) of each breakable wall whose faces have no back face
TWO_SIDED = {
    'PARIS': [[0x8002E6B0, 0x8002E6D4, 0x8002E6F8, 0x8002E71C, 0x8002E630, 0x8002E654]],    # the church door 8002EEF0
    'PARIS2': [[0x8002E868, 0x8002E88C, 0x8002E8B0, 0x8002E8D4, 0x8002E7E8, 0x8002E80C]],   # the church door 8002F0A8
    'ISLANDS': [[0x8006942C, 0x80069450, 0x80069474, 0x8006949C],                           # the temple's walls 8006957C
                [0x80069634, 0x80069760]],                                                  # panels 800696A8, 8006B064
    'BURB': [[0x80089E3C]],                                                                 # the panel 8008A158
}


def check_two_sided(out):
    """Request 113 (HIDDEN_AREAS.md 16): portTwoSidedBegin and portHiddenAreasDrawn in LinkMips on the disc's level
    files: with portVisual bit 6 exactly TWO_SIDED's words get bit 31 (the emitter's "no backface cull"), each one a
    standing wall's face (one-sided on the disc, at its select node's child 0), and Begin returns their count; Drawn
    gives the file back byte for byte; bit 6 clear, another level, or one word not the disc's: nothing. 113b: the same
    after the game's meshSetPrimQuadrants has run on those faces' meshes (as dbscan does at load, rewriting bits
    13..15), so that the match's words are the ones tested."""
    from check_rollback import Image
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN
    img = Image(out)
    syms = img.link.symbols
    need = ('portTwoSidedBegin', 'portHiddenAreasDrawn', 'portVisual', 'meshSetPrimQuadrants')
    miss = [n for n in need if n not in syms]
    if miss:
        print('25. request 113 two-sided walls -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    vis = syms['portVisual']['address']
    disc = Disc(DEFAULT_BIN)
    bad, runs = [], 0
    for name in ('PARIS', 'PARIS2', 'ISLANDS', 'BURB', 'HKONG', 'FREEWAY'):
        dm = Dmd(name, disc.get('/DMD/%s.DMD' % name))
        d = dm.d
        groups = TWO_SIDED.get(name, [])
        want = [w for g in groups for w in g]
        meshes = []                                         # 113b: the meshes holding the listed faces
        for nd in dm.nodes()[0]:
            o = dm.off(nd)
            if d[o] != 0:
                continue
            pp = dm.off(dm.u32(o + 0xC))
            for i in range(dm.u32(o + 0x10)):
                if pp + HIDDEN_LEVEL_AT + 12 in want and nd not in meshes:
                    meshes.append(nd)
                pp += 4 * d[pp + 2]
        quads = 0
        for w in want:
            v = int.from_bytes(d[w - HIDDEN_LEVEL_AT:w - HIDDEN_LEVEL_AT + 4], 'little')
            if v >> 31 or (v >> 16) & 7 not in (3, 4):
                bad.append('%s %08X: %08X is not a one-sided polygon\'s control word' % (name, w, v))
        for bit, mutate, loaded in ((0x40, False, False), (0x7FF & ~0x40, False, False), (0x40, True, False),
                                    (0x40, False, True), (0x7FF & ~0x40, False, True), (0x40, True, True)):
            lm = LinkMips(img, {})
            m = lm.m
            m.fill(HIDDEN_LEVEL_AT, d)
            m.wr(vis, 4, bit)
            if loaded:
                for nd in meshes:
                    lm.run('meshSetPrimQuadrants', (nd,), steps=200000)
                    runs += 1
                if not mutate:
                    quads = sum(m.rd(w, 4) != int.from_bytes(d[w - HIDDEN_LEVEL_AT:w - HIDDEN_LEVEL_AT + 4], 'little')
                                for w in want)
            if mutate and want:
                m.wr(want[0], 4, m.rd(want[0], 4) ^ 0x1000)        # the first group then stays as it is
            before = bytes(m.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(d)))
            rv = s32(lm.run('portTwoSidedBegin', (), steps=200000))
            runs += 1
            after = bytes(m.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(d)))
            set_ = sorted(HIDDEN_LEVEL_AT + i - 3 for i in range(3, len(d), 4)
                          if after[i - 3:i + 1] != before[i - 3:i + 1])
            exp = (want if not mutate else [w for g in groups[1:] for w in g]) if bit & 0x40 else []
            if rv != len(exp) or set_ != sorted(exp) or any(
                    int.from_bytes(after[w - HIDDEN_LEVEL_AT:w - HIDDEN_LEVEL_AT + 4], 'little') !=
                    int.from_bytes(before[w - HIDDEN_LEVEL_AT:w - HIDDEN_LEVEL_AT + 4], 'little') | 0x80000000
                    for w in exp):
                bad.append('%s bit %#x%s%s: Begin returned %d, changed %s (want %d: %s)' % (
                    name, bit, ', a word changed' if mutate else '', ', loaded' if loaded else '', rv, ['%08X' % w for w in set_][:8], len(exp),
                    ['%08X' % w for w in exp]))
            lm.run('portHiddenAreasDrawn', (), steps=200000)
            runs += 1
            if bytes(m.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(d))) != before:
                bad.append('%s bit %#x%s%s: the level differs from before Begin after Drawn' % (
                    name, bit, ', a word changed' if mutate else '', ', loaded' if loaded else ''))
        if want and not quads:
            bad.append('%s: meshSetPrimQuadrants left every listed word as on the disc (the 113b case not tested)' % name)
    good = not bad
    print('25. requests 113, 113b: portTwoSidedBegin / portHiddenAreasDrawn (%d runs): the breakable walls\' faces '
          'two-sided with bit 6 (%d faces in 4 files), as on the disc and after meshSetPrimQuadrants, nothing else, the '
          'file back after drawing -> %s' % (
              runs, sum(len(g) for v in TWO_SIDED.values() for g in v), 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


# check 26 (request 114, MOD_ARENAS.md): a mod arena's health spot and warp exits from portArenaRules
# (port_mod_arena.h: +0 heal, +4 healX, +8 healZ, +0xC warps, +0x10 exit[3][2] s16). retail's exits per level and
# side (fx_debris_splash.c aiWarpSetExitNode): (spawn node, target node), None = the car not placed
MOD_WARP_RETAIL = {(7, 0): (0x2E, 9), (7, 1): (0x2F, 0x26), (7, 2): (0x2F, 0x26), (1, 0): (0x4A, 0x35),
                   (8, 2): (0x42, 0x86), (2, 0): None, (13, 0): None}
MOD_AI_AT = 0x801E0000          # a scratch AI record: stats spawn_node +0x5D0 (s32), target_node +0x53C (s16)


def check_mod_arena(out):
    """Request 114 (MOD_ARENAS.md): portArenaHealSpot and aiWarpSetExitNode in LinkMips. With portArenaRules all 0
    both are retail's (the exits per level and side as MOD_WARP_RETAIL; heal 0 = retail's square); heal 1 gives
    (healX, healZ), heal 2 none; warps 1 takes each side's nodes from exit[], a spawn below 0 or a side past 2 places
    nothing and leaves the record as it was."""
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('portArenaHealSpot', 'aiWarpSetExitNode', 'portArenaRules')
    miss = [n for n in need if n not in syms]
    if miss:
        print('26. request 114 mod arena rules -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    rules = syms['portArenaRules']['address']
    bad, runs = [], 0
    out_x, out_z = MOD_AI_AT + 0x800, MOD_AI_AT + 0x804

    def heal(mode, x, z):
        lm = LinkMips(img, {})
        m = lm.m
        m.wr(rules, 4, mode)
        m.wr(rules + 4, 4, x & 0xFFFFFFFF)
        m.wr(rules + 8, 4, z & 0xFFFFFFFF)
        m.wr(out_x, 4, 0x11111111)
        m.wr(out_z, 4, 0x22222222)
        rv = s32(lm.run('portArenaHealSpot', (out_x, out_z), steps=2000))
        return rv, s32(m.rd(out_x, 4)), s32(m.rd(out_z, 4))

    for mode, x, z, want in ((0, 5, 6, (0, 0x11111111, 0x22222222)), (1, -0x490, 0x1838, (1, -0x490, 0x1838)),
                             (1, 0x7FFFFFC0, -0x7FFFFFFF, (1, 0x7FFFFFC0, -0x7FFFFFFF)),
                             (2, 5, 6, (2, 0x11111111, 0x22222222)), (3, 5, 6, (0, 0x11111111, 0x22222222))):
        got = heal(mode, x, z)
        runs += 1
        if got != want:
            bad.append('portArenaHealSpot heal %d (%d, %d): got %s, want %s' % (mode, x, z, got, want))

    def warp(level, side, table=None):
        placed = []
        lm = LinkMips(img, {'gameGetLevel': lambda m: level,
                            'aiPlaceOnRouteNode': lambda m: placed.append(m.r[4]) or 0})
        m = lm.m
        for i in range(0x1C):
            m.wr(rules + i, 1, 0)
        if table is not None:
            m.wr(rules + 0xC, 4, 1)
            for i, (a, b) in enumerate(table):
                m.wr(rules + 0x10 + 4 * i, 2, a & 0xFFFF)
                m.wr(rules + 0x12 + 4 * i, 2, b & 0xFFFF)
        m.wr(MOD_AI_AT + 0x5D0, 4, 0x7777)
        m.wr(MOD_AI_AT + 0x53C, 2, 0x6666)
        lm.run('aiWarpSetExitNode', (MOD_AI_AT, side & 0xFFFF if side >= 0 else side), steps=5000)
        sp, tg = s32(m.rd(MOD_AI_AT + 0x5D0, 4)), m.rd(MOD_AI_AT + 0x53C, 2)
        tg = tg - 0x10000 if tg & 0x8000 else tg
        if not placed:
            return None if (sp, tg) == (0x7777, 0x6666) else ('not placed, but changed', sp, tg)
        if placed != [MOD_AI_AT]:
            return ('placed', placed)
        return (sp, tg)

    for (level, side), want in sorted(MOD_WARP_RETAIL.items()):
        got = warp(level, side)
        runs += 1
        if got != want:
            bad.append('retail level %d side %d: got %s, want %s' % (level, side, got, want))
    table = [(12, 34), (-1, 99), (300, -5)]
    for level in (7, 13, 2):
        for side, want in ((0, (12, 34)), (1, None), (2, (300, -5)), (3, None), (-1, None)):
            got = warp(level, side, table)
            runs += 1
            if got != want:
                bad.append('table, level %d side %d: got %s, want %s' % (level, side, got, want))
    good = not bad
    print('26. request 114: portArenaHealSpot / aiWarpSetExitNode (%d runs): retail\'s spot and exits with '
          'portArenaRules 0, the port\'s spot, none, and exits per side when set -> %s' % (runs, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


# check 27 (request 116): the emitter's backface test on the exact projection (portVisual bit 11)
def cull_strip_meshes():
    """Made-up meshes like Hong Kong's crosswalk (8002A2D0): eight flat strips across a 768-unit road, 96 units deep
    (two of 192), at z 400, as untextured quads (the disc's corner order) and as triangles (each quad's 0-1-2 and its
    3-2-1), at SYNTH_BASE + 0x10000 (after synth_meshes'): [(node, centre, radius, 0)] and their bytes."""
    ys = (-480, -288, -192, -96, 0, 96, 192, 288, 480)
    vs = [(x, y, 400) for y in ys for x in (0, -768)]
    vi = lambda x, y: 2 * ys.index(y) + (0 if x == 0 else 1)
    quads = [((0, -192), (0, -288), (-768, -192), (-768, -288)), ((-768, 288), (-768, 480), (0, 288), (0, 480)),
             ((0, -288), (0, -480), (-768, -288), (-768, -480)), ((-768, -192), (-768, -96), (0, -192), (0, -96)),
             ((0, 0), (0, -96), (-768, 0), (-768, -96)), ((-768, 0), (-768, 96), (0, 0), (0, 96)),
             ((0, 192), (0, 96), (-768, 192), (-768, 96)), ((-768, 192), (-768, 288), (0, 192), (0, 288))]
    out, mem = [], []
    for n, tris in enumerate((False, True)):
        node = SYNTH_BASE + 0x10000 + 0x1000 * n
        verts, normals, prims = node + 0x40, node + 0x200, node + 0x400
        mem.append((verts, b''.join(_u16(v[0]) + _u16(v[1]) + _u16(v[2]) + bytes(2) for v in vs)))
        mem.append((normals, bytes(8 * len(vs))))
        pb, count = bytearray(), 0
        for q in quads:
            ix = [vi(*c) for c in q]
            for corners in (([ix[0], ix[1], ix[2]], [ix[3], ix[2], ix[1]]) if tris else (ix,)):
                t = len(corners)
                c4 = corners + [0] * (4 - t)
                ctl = (t << 16) | (1 << 22)                     # bit 31 clear: the backface test applies
                pb += bytes((0, 1, 5, t + 1)) + (c4[0] | c4[1] << 16).to_bytes(4, 'little') + \
                    (c4[2] | c4[3] << 16).to_bytes(4, 'little') + ctl.to_bytes(4, 'little') + \
                    (0x28000000 + 0x10101 * (count + 1) if t == 4 else 0x20000000 + 0x10101 * (count + 1)).to_bytes(
                        4, 'little')
                count += 1
        mem.append((prims, bytes(pb)))
        hdr = bytearray(44)
        hdr[4:8] = verts.to_bytes(4, 'little')
        hdr[8:12] = normals.to_bytes(4, 'little')
        hdr[12:16] = prims.to_bytes(4, 'little')
        hdr[16:20] = count.to_bytes(4, 'little')
        hdr[40:44] = (500).to_bytes(4, 'little')
        mem.append((node, bytes(hdr)))
        SYNTH_HDR[node] = bytes(hdr)
        out.append((node, [-384, 0, 400], 500, 0))
    return out, mem


def cull_packets(m, flip):
    """The polygons an emitter run (emit_scene's memory) linked into its OT or deferred to its near list: a multiset
    {(where, the packet's words but its tag): count}, where = ('ot', entry) or ('near', sort z)."""
    import collections
    got = collections.Counter()
    for a, e in whole_near_ot(m).items():
        ln = m.rd(a, 4) >> 24
        got[(('ot', e), tuple(m.rd(a + 4 + 4 * j, 4) for j in range(ln)))] += 1
    n = m.rd(EMIT_SCRATCH + 0x38, 4)
    over = m.rd(EMIT_SCRATCH + 0x4C, 4)
    for i in range(n):
        ent = EMIT_SCRATCH + 0xA4 + 12 * i if i < 71 else over + 12 * (i - 71)
        prim, pkt, sz = m.rd(ent, 4), m.rd(ent + 4, 4), s32(m.rd(ent + 8, 4)) >> 16
        got[(('near', sz), tuple(m.rd(pkt + 4 + 4 * j, 4) for j in range(m.rd(prim + 3, 1))))] += 1
    return got


def cull_corners(words):
    """A polygon packet's first three screen corners (x, y), or None when the words are not a polygon."""
    code = words[0] >> 24
    if code & 0xE0 != 0x20:
        return None
    g, t = code & 0x10, code & 4
    out, w = [], 1
    for i in range(3):
        if i and g:
            w += 1
        if w >= len(words):
            return None
        out.append((_s16(words[w] & 0xFFFF), _s16(words[w] >> 16)))
        w += 2 if t else 1
    return out


def cull_exact(scene, m):
    """Every triangle and quad of an emitter scene that the backface test applies to (control bit 31 clear), on the
    exact projection: [(entry, primitive, float corners 0..2 in pixels, camera-space determinant sign, its projected
    area in px^2)]; the translation rotated by the slot of the entry where the key changed, as the emitter does."""
    rotbase, meta = scene[2][3], scene[4]
    out, last_key, tr = [], None, None
    for ei, (hdr, node, t, sl, ky) in enumerate(meta):
        R = [[_s16(m.rd(rotbase + 32 * sl + 2 * (3 * a + b), 2)) for b in range(3)] for a in range(3)]
        if ky != last_key:
            tr = [sum(R[i][j] * t[j] for j in range(3)) / 4096.0 for i in range(3)]
            last_key = ky
        verts, p, n = m.rd(hdr + 4, 4), m.rd(hdr + 12, 4), m.rd(hdr + 16, 4)
        for pi in range(n):
            ctl = m.rd(p + 12, 4)
            if (ctl >> 16) & 7 in (3, 4) and not ctl & 0x80000000:
                a, b = m.rd(p + 4, 4), m.rd(p + 8, 4)
                cs = []
                for ix in (_s16(a & 0xFFFF), _s16(a >> 16), _s16(b & 0xFFFF)):
                    v = [_s16(m.rd(verts + 8 * ix + 2 * k, 2)) for k in range(3)]
                    cs.append([tr[i] + sum(R[i][j] * v[j] for j in range(3)) / 4096.0 for i in range(3)])
                if all(c[2] > 1 for c in cs):
                    e1 = [cs[1][i] - cs[0][i] for i in range(3)]
                    e2 = [cs[2][i] - cs[0][i] for i in range(3)]
                    nn = [e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]]
                    det = sum(cs[0][i] * nn[i] for i in range(3))
                    px = [(160 + EMIT_H * c[0] / c[2], 120 + EMIT_H * c[1] / c[2]) for c in cs]
                    area = (px[1][0] - px[0][0]) * (px[2][1] - px[0][1]) - (px[2][0] - px[0][0]) * (px[1][1] - px[0][1])
                    out.append((ei, pi, px, det, area))
            p += 4 * m.rd(p + 2, 1)
    return out


def cull_reflect(scene):
    """Request 128: an emitter scene as the rear-view mirror draws it: every rotation slot's row 0 negated (viewProc's
    axis matrix with x -4096 for view mode 4), so det(R) < 0 and NCLIP's sign is reversed (run it with cull flip 1)."""
    mem, ctrl, args, stack, meta = scene
    rot, out = args[3], []
    for a, b in mem:
        if rot <= a < rot + 32 * 64 and (a - rot) % 32 == 0 and len(b) == 32:
            v = [_s16(int.from_bytes(b[2 * i:2 * i + 2], 'little')) for i in range(9)]
            b = b''.join(_u16(-x if i < 3 else x) for i, x in enumerate(v)) + b[18:]
        out.append((a, b))
    return out, ctrl, args, stack, meta


def cull_clamp(v):
    """A corner coordinate (float pixels, offset added) clamped as the GTE clamps SX / SY (-1024..1023)."""
    return min(max(v, -1024.0), 1023.0)


def check_precise_cull(out):
    """Check 27 (request 116; portVisual bit 11, TM2_VIS_PRECISE_CULL, with TM2_PORT_C_EMITTER): the
    link's emitter runs on made-up views (emit_scene: PARIS and FREEWAY meshes, crosswalk strips as quads and as
    triangles, at random turns and distances, cull flip 0 and 1) with portVisual 0, with every other bit, and with bit
    11: every other bit writes the bytes of 0; bit 11 draws every polygon 0 drew (the same words at the same OT entry or
    near-list sort z) and more, and each one more is a triangle or quad whose packet corners NCLIP culls (in the
    scene's cull sense) and that faces the eye on the exact projection (float camera space; a projected area within
    CULL_EPS px^2 of 0 is either). At least CULL_MIN more polygons in all (the test sees the case). The bit's value is
    the same in port_c_emitter.h and port_visual.h. Request 128: also mirror views (cull_reflect, cull flip 1: no
    back face drawn, the same rules) and views whose keys are shared across slots (the GTE keeps the first slot's
    translation: the test's camera-space path, its corners and edges halved to 15 and 14 bits: edge-on within
    CULL_EPS_SHARED px^2 there; the game keeps a key to one slot, as the GTE would draw it elsewhere otherwise). The
    exact corners are matched clamped to the GTE's -1024..1023."""
    import random
    from check_rollback import Image
    CULL_EPS, CULL_MIN = 0.02, 50
    CULL_EPS_SHARED = 0.5     # the camera-space path (keys shared across slots): request 116's precision
    bad = []
    hdr_c = (ROOT / 'include' / 'semantic' / 'port_c_emitter.h').read_text(encoding='utf-8')
    hdr_v = (ROOT / 'include' / 'semantic' / 'port_visual.h').read_text(encoding='utf-8')
    bc = re.search(r'#define PORT_PRECISE_CULL_BIT (0x[0-9A-Fa-f]+)', hdr_c)
    bv = re.search(r'#define TM2_VIS_PRECISE_CULL (0x[0-9A-Fa-f]+)', hdr_v)
    if not bc or not bv or int(bc.group(1), 16) != int(bv.group(1), 16):
        bad.append('PORT_PRECISE_CULL_BIT and TM2_VIS_PRECISE_CULL differ or are missing')
        bit = 0x800
    else:
        bit = int(bc.group(1), 16)
    img = Image(out)
    syms = img.link.symbols
    if 'sub_800CDC54' not in img.sizes or 'portVisual' not in syms:
        print('27. request 116 precise backface test -> FAIL: the emitter or portVisual not in the link')
        return False
    pv = syms['portVisual']['address']
    rng = random.Random(116)
    disc = emit_disc_meshes(['PARIS', 'FREEWAY'])
    strips, smem = cull_strip_meshes()
    levels = sorted(disc)
    stats = dict(scenes=0, mirror=0, drawn=0, more=0, sure=0, edge=0, clamped=0)
    views = [(c, False, 0.0) for c in (0, 1, 0, 1, 0, 1)] + [(1, True, 0.0)] * 6 + [(0, False, 0.5), (1, True, 0.5)]
    for si, (cull, mirror, change) in enumerate(views):
        lv = {levels[si % len(levels)]: disc[levels[si % len(levels)]]} if levels and si % 6 < 4 else {}
        loaded = [(v[0], v[1]) for v in lv.values()]
        scene = emit_scene(rng, lv, strips, 80, 0.1, 0, 0, cull_flip=cull, slot_change=change)
        if mirror:
            scene = cull_reflect(scene)
            stats['mirror'] += 1
        runs = {}
        for word in (0, 0xFFFFFFFF & ~bit, bit):
            runs[word] = emit_run(img, scene, loaded, list(smem) + [(pv, word.to_bytes(4, 'little'))])
        o0, m0 = runs[0][0], runs[0][3]
        oo, mb = runs[0xFFFFFFFF & ~bit][0], runs[bit][3]
        diff = sorted(a for a in set(o0) | set(oo) if o0.get(a) != oo.get(a))
        if diff:
            bad.append('scene %d: the other portVisual bits change %d bytes (first %08X)' % (si, len(diff), diff[0]))
        p0, pb = cull_packets(m0, cull), cull_packets(mb, cull)
        lost = p0 - pb
        if lost:
            bad.append('scene %d: %d polygons of the run without the bit not drawn with it' % (si, sum(lost.values())))
        more = pb - p0
        exact = cull_exact(scene, mb)
        stats['scenes'] += 1
        stats['drawn'] += sum(p0.values())
        for (where, words), cnt in more.items():
            cs = cull_corners(words)
            if cs is None:
                continue                                # a draw-mode packet around a flagged polygon
            stats['more'] += cnt
            (x0, y0), (x1, y1), (x2, y2) = cs
            if any(v in (-1024, 1023) for c in cs for v in c):
                stats['clamped'] += cnt         # a corner the GTE clamped (its divide saturated near the eye): the
                continue                        # packet's corners are not the projection's, nothing to match
            mac0 = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
            if (mac0 >= 0) == (cull == 0):
                bad.append('scene %d: a polygon more with the bit that NCLIP keeps (%s, NCLIP %d)' % (si, cs, mac0))
                continue
            gap = lambda c: max(max(abs(cull_clamp(c[2][k][0]) - cs[k][0] - 0.5),
                                    abs(cull_clamp(c[2][k][1]) - cs[k][1] - 0.5)) for k in range(3))
            best = min(exact, key=gap)      # (the GTE clamps a corner off the screen to -1024..1023)
            err = gap(best)
            if err > 1.6:
                bad.append('scene %d: a polygon more with the bit (%s) matches no primitive (nearest %.2f px off)' % (
                    si, cs, err))
                continue
            # every primitive the packet's corners could be (request 128: a small mesh's faces seen edge-on round to
            # the same pixels): the best one that faces the eye or is edge-on
            eps = CULL_EPS_SHARED if change else CULL_EPS
            cands = [c for c in exact if gap(c) <= 1.6]
            ok_c = [c for c in cands if abs(c[4]) <= eps or (c[3] >= 0) == (cull == 0)]
            if ok_c:
                best = min(ok_c, key=gap)
            if abs(best[4]) <= eps:
                stats['edge'] += cnt
            elif ok_c:
                stats['sure'] += cnt
            else:
                bad.append('scene %d: a polygon more with the bit faces away on the exact projection (entry %d prim %d, '
                           'area %.3f px^2)' % (si, best[0], best[1], best[4]))
    if stats['more'] < CULL_MIN:
        bad.append('only %d polygons more with the bit (want at least %d: the test sees nothing)' % (
            stats['more'], CULL_MIN))
    good = not bad
    print('27. request 116 precise backface test (portVisual bit %X): %d views (%d mirror), %d polygons drawn without '
          'it; with it all of them and %d more, each culled by NCLIP on its packet corners and facing the eye on the '
          'exact projection (%d clearly, %d within %.2f px^2 of edge-on, %d with a corner clamped off the screen); the '
          'other bits change nothing -> %s' % (
              bit, stats['scenes'], stats['mirror'], stats['drawn'], stats['more'], stats['sure'], stats['edge'],
              CULL_EPS, stats['clamped'], 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


# check 28 (request 117, PLAYTEST_ANSWERS.md 33): a falling wreck hidden with its splash, or exploding at the floor
WRECK_RECORDS = {   # request 117: the record fields portWreckFall reads (include/semantic/cars.h)
    1: dict(array='humanCars', size=0x814, index=1, number=0x000, slots='humanCarObjectSlots', type=0x090,
            dead=0x09C, falling=0x0BB, timer=0x172, ride=0x18E, pos=0x6FC),
    0: dict(array='aiCars', size=0x854, index=3, number=0x004, slots='aiCarObjectSlots', type=0x030,
            dead=0x06C, falling=0x08B, timer=0x522, ride=0x53E, pos=0x73C)}


def check_wreck_fall(out, wheels, humans):
    """Request 117 (PLAYTEST_ANSWERS.md 33): portWreckFall and portWreckFallBlown in LinkMips, frame by frame on a
    human and a computer record (the effects and sounds stubbed, csSetDrawEnable the link's): on Antarctica a drawn
    wreck is hidden with one splash (a human's sound 34) the frame it passes below 240, its wreck timer at most 0,
    and nothing more after; level 8 below -40 hidden without a splash; on New York a wreck blown up in the air, or
    given nothing (the won round), explodes once when it passes the pit test and never again as it bounces; one
    blown up at the floor (portWreckFallBlown) does not explode again; a live car, a wreck not falling, a hidden or
    parked car, and a slot outside the load are left alone; no rand()."""
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('portWreckFall', 'portWreckFallBlown', 'portWreckFallMark', 'humanCars', 'aiCars', 'humanCarObjectSlots',
            'aiCarObjectSlots', 'carSlotObjects', 'carWorldObjectCount')
    miss = [n for n in need if n not in syms]
    if miss:
        print('28. request 117 wreck fall -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    A = lambda n: syms[n]['address']
    OBJ, SLOT, DEATH = 0x80A00000, 5, 80
    bad, runs = [], 0

    def scenario(human, level, frames, drawn=1, slot=SLOT, count=8, ride=15, timer=120):
        """frames: (dead, falling, z, blown) per frame; blown = portWreckFallBlown after portWreckFall (carDestroyed's
        explosion that frame). Returns the effects per frame and the end state."""
        f = WRECK_RECORDS[human]
        ev = []
        stub = lambda tag: (lambda m: ev.append(tag) or 0)
        stubs = {'gameGetLevel': lambda m: level, 'worldFallDeathZ': lambda m: -800 if level == 8 else DEATH,
                 'fxSplashAt': lambda m: ev.append(('splash', s32(m.rd(m.r[4] + 8, 4)), s32(m.r[5]))) or 0,
                 'fxSmokeA24': stub('smoke'), 'fxExplosion48': stub('boom48'), 'fxExplosion24': stub('boom24'),
                 'carSpinOut': lambda m: ev.append(('spin', m.r[5], m.r[6])) or 0,
                 'soundPlay': lambda m: ev.append(('sound', m.r[4])) or 0,
                 'portHumanSound': lambda m: ev.append(('sound', m.r[5])) or 0,
                 'soundLocateForView0': stub('locate'), 'soundLocateForView': stub('locate'),
                 'PadChkMtap': lambda m: 0, 'soundGetListenerPan': lambda m: 0, 'viewGetSplitMode': lambda m: 0,
                 'rand': stub('rand')}
        lm = LinkMips(img, stubs)
        m = lm.m
        rec = A(f['array']) + f['index'] * f['size']
        m.fill(rec, bytes(f['size']))
        m.wr(rec + f['number'], 2, f['index'])
        m.wr(rec + f['type'], 4, 0x10)
        m.wr(rec + f['ride'], 2, ride)
        m.wr(rec + f['timer'], 2, timer & 0xFFFF)
        m.wr(rec + f['pos'], 4, 1234)
        m.wr(rec + f['pos'] + 4, 4, -567 & 0xFFFFFFFF)
        m.wr(A(f['slots']) + 4 * f['index'], 4, slot & 0xFFFFFFFF)
        m.wr(A('carWorldObjectCount'), 4, count)
        if 0 <= slot < 64:
            m.wr(A('carSlotObjects') + 4 * slot, 4, OBJ)
        m.fill(OBJ, bytes(0x100))
        m.wr(OBJ + 8, 4, drawn)
        if 0 <= slot < 64:
            m.wr(A('portWreckFallMark') + slot, 1, 0)
        out_ = []
        for dead, falling, z, blown in frames:
            m.wr(rec + f['dead'], 1, dead)
            m.wr(rec + f['falling'], 1, falling)
            m.wr(rec + f['pos'] + 8, 4, z & 0xFFFFFFFF)
            del ev[:]
            lm.run('portWreckFall', (rec, human), steps=20000)
            if blown:
                lm.run('portWreckFallBlown', (rec, human), steps=20000)
            out_.append(list(ev))
        t = m.rd(rec + f['timer'], 2)
        return out_, m.rd(OBJ + 8, 1), t - 0x10000 if t & 0x8000 else t

    fz = lambda floor_z, ride: floor_z + (ride if wheels else 0)     # the body height for a wheel height
    boom = ['locate', ('sound', 25), 'smoke', ('spin', None, 4), 'boom48', 'boom24']

    def same(got, want):
        if len(got) != len(want):
            return False
        for g, w in zip(got, want):
            if isinstance(w, tuple) and isinstance(g, tuple):
                if len(g) != len(w) or any(b is not None and a != b for a, b in zip(g, w)):
                    return False
            elif g != w:
                return False
        return True

    for human in (1, 0):
        who = ('AI', 'human')[human]
        splash = [('splash', 240 - 1, 2000)] + ([('sound', 34)] if human else [])
        cases = [
            # Antarctica: killed in the air (blown at 2000, not at the floor), falls past 240: hidden once
            ('6 air kill', 6, [(0, 1, 3000, 0), (1, 1, 2000, 1), (1, 1, 900, 0), (1, 1, 239, 0), (1, 1, 100, 0),
                               (1, 1, 0, 0)], {3: splash}, 0, 0),
            # Antarctica: the won round (carDestroyed did nothing), the wreck falls: hidden once
            ('6 won round', 6, [(0, 1, 3000, 0), (1, 1, 2000, 0), (1, 1, 239, 0), (1, 1, 10, 0)], {2: splash}, 0, 0),
            # Antarctica: hidden already by carDestroyed (not drawn): nothing
            ('6 hidden', 6, [(1, 1, 100, 0), (1, 1, 50, 0)], {}, 0, 120, 0),
            # Antarctica: alive below 240 (about to die there): nothing
            ('6 alive', 6, [(0, 1, 150, 0), (0, 1, 100, 0)], {}, 1, 120),
            # level 8 below -40: hidden, no splash
            ('8 sink', 8, [(1, 1, 0, 0), (1, 1, -41, 0), (1, 1, -60, 0)], {1: []}, 0, 0),
            # New York: killed in the air, the wreck passes the pit test once, bounces back over it and under again
            ('5 air kill', 5, [(0, 1, 5000, 0), (1, 1, 4000, 1), (1, 1, fz(81, 15), 0), (1, 1, fz(79, 15), 0),
                               (1, 1, fz(120, 15), 0), (1, 1, fz(10, 15), 0)], {3: boom}, 1, 120),
            # New York: died at the floor and blown up there by carDestroyed: no second explosion
            ('5 floor death', 5, [(0, 1, fz(70, 15), 1), (1, 1, fz(60, 15), 0), (1, 1, fz(5, 15), 0)], {}, 1, 120),
            # New York: the won round, the wreck falls: one explosion at the floor
            ('5 won round', 5, [(0, 1, 7000, 0), (1, 1, 5000, 0), (1, 1, fz(79, 15), 0), (1, 1, fz(60, 15), 0)],
             {2: boom}, 1, 120),
            # a wreck on a roof, not falling, low: nothing; and a respawn clears the mark
            ('5 not falling', 5, [(1, 0, 50, 0), (1, 0, 20, 0)], {}, 1, 120),
            ('5 respawn', 5, [(1, 1, fz(70, 15), 0), (0, 0, 7000, 0), (0, 1, 5000, 0), (1, 1, 4000, 1),
                              (1, 1, fz(79, 15), 0)], {0: boom, 4: boom}, 1, 120),
            # parked (not drawn, far away): nothing
            ('5 parked', 5, [(1, 1, -1000000, 0), (1, 1, -1000000, 0)], {}, 0, 120, 0),
        ]
        for case in cases:
            name, level, frames, want, vis, timer = case[:6]
            drawn = case[6] if len(case) > 6 else 1
            got, vis_got, timer_got = scenario(human, level, frames, drawn=drawn)
            runs += 1
            for i, g in enumerate(got):
                w = want.get(i, [])
                if not same(g, w) or ('rand' in g):
                    bad.append('%s %s frame %d: %s (want %s)' % (who, name, i, g, w))
            if vis_got != vis or timer_got != timer:
                bad.append('%s %s: drawn %d timer %d (want %d, %d)' % (who, name, vis_got, timer_got, vis, timer))
        for slot, count in ((SLOT, SLOT), (-1, 8), (70, 80)):
            got, vis_got, _ = scenario(human, 6, [(1, 1, 100, 0), (1, 1, 50, 0)], slot=slot, count=count)
            runs += 1
            if any(got) or vis_got != 1:
                bad.append('%s slot %d of %d: %s, drawn %d (want nothing)' % (who, slot, count, got, vis_got))
    good = not bad
    print('28. request 117: portWreckFall / portWreckFallBlown (%d runs, pit test from the %s, human sound by %s): '
          'a wreck hidden with its splash below 240 on Antarctica (-40 on level 8, no splash), one explosion at the '
          'floor elsewhere for a wreck not blown up there, nothing for a live, roof, hidden, parked or foreign car '
          '-> %s' % (runs, 'wheels' if wheels else 'body', 'portHumanSound' if humans else 'soundPlay',
                     'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good



# check 29 (request 118, DARK_TOOTH_PLAYER.md 12): a player's Dark Tooth in his second phase
DT_PHASE2_BOX = ((-55, 56), (-84, 75), (-24, 117))     # the head's box (BB.DMD 80028460) at the stage-2 hover -24


def dt_phase2_inside(tree, base, v):
    """hdpBox on the tree's kind-8 node (the walker's planes: row . v / 4096 + depth > 0, all six)."""
    import struct
    b = tree[28:]
    for mat, dep in ((16, 36), (48, 68)):
        R = struct.unpack_from('<9h', b, mat)
        t = struct.unpack_from('<3i', b, dep)
        for r in range(3):
            if (R[3 * r] * v[0] + R[3 * r + 1] * v[1] + R[3 * r + 2] * v[2]) // 4096 + t[r] <= 0:
                return False
    return True


def check_dt_phase2(out):
    """Request 118 (DARK_TOOTH_PLAYER.md 12): portCarDtPlayerStart in LinkMips on a human record (the key selection
    stubbed). With portDtPlayer.phase2's bit for the human and type 0xF0: his key's stage flag 1, stage 2's stats
    (carStatsDarkToothStage2: max HP 110, mass 50, ride height 50, HP 110), the car object's model the hit-only tree
    and the truck's kept for the slot, the head out of the hit pass; the tree a kind-1 sphere (id 240) over one kind-8
    box whose six planes hold exactly the head's box at -24 (points just inside and outside each face); the bit
    clear again: the flag 0, the truck back, the head in the hit pass, the stats not touched; another type or
    another human's bit: nothing."""
    import struct
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('portCarDtPlayerStart', 'portDtPlayer', 'portDtPhase2Tree', 'portDtPhase2Truck', 'humanCars',
            'carSlotObjects', 'carWorldObjectCount', 'aiBossDefeatedFlag', 'darkToothBossObject')
    miss = [n for n in need if n not in syms]
    if miss:
        print('29. request 118 phase-2 Dark Tooth -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    A = lambda n: syms[n]['address']
    OBJ, HEAD, TRUCK, SLOT, H = 0x80A00000, 0x80A00200, 0x80A00400, 5, 1
    rec = A('humanCars') + H * 0x814
    st = rec + 0x14C
    bad, runs = [], 0
    stubs = {'portCarSpecialSelect': lambda m: 0, 'portCarSpecialRestore': lambda m: 0,
             'portCarKey': lambda m: m.r[5], 'rand': lambda m: bad.append('rand() called') or 0}
    lm = LinkMips(img, stubs)
    m = lm.m
    m.fill(rec, bytes(0x814))
    m.wr(rec, 2, H)
    m.wr(rec + 4, 2, SLOT)
    m.wr(A('carWorldObjectCount'), 4, 8)
    m.wr(A('carSlotObjects') + 4 * SLOT, 4, OBJ)
    m.fill(OBJ, bytes(0x120))
    m.wr(OBJ + 0x11C, 4, TRUCK)
    m.fill(HEAD, bytes(0x120))
    m.wr(HEAD + 0xC, 4, 1)
    m.wr(A('darkToothBossObject'), 4, HEAD)
    m.wr(A('portDtPhase2Truck') + 4 * SLOT, 4, 0)
    tree_at = A('portDtPhase2Tree')
    sentinel = bytes((i * 37 + 11) & 0xFF for i in range(0xE8))

    def stats_reset():
        m.fill(st, sentinel)

    def stats():
        return bytes(m.rd(st + i, 1) for i in range(0xE8))

    def s16(a):
        v = m.rd(a, 2)
        return v - 0x10000 if v & 0x8000 else v

    def run(type_, bits):
        m.wr(rec + 0x90, 4, type_)
        m.wr(A('portDtPlayer') + 0x28, 4, bits)
        stats_reset()
        lm.run('portCarDtPlayerStart', (rec,), steps=200000)
        return (m.rd(A('aiBossDefeatedFlag'), 4), m.rd(OBJ + 0x11C, 4), m.rd(A('portDtPhase2Truck') + 4 * SLOT, 4),
                m.rd(HEAD + 0xC, 4))

    # phase 2 on
    m.wr(A('aiBossDefeatedFlag'), 4, 0)
    got = run(0xF0, 1 << H)
    runs += 1
    if got != (1, tree_at, TRUCK, 0):
        bad.append('phase 2: flag %d, model %08X, kept %08X, head +0xC %d (want 1, %08X, %08X, 0)' % (
            got + (tree_at, TRUCK)))
    want_st = {0x3A: 110, 0x7A: 50, 0x42: 50, 0x46: 60, 0x48: 50, 0x4E: 40, 0x50: -40}
    for off, w in want_st.items():
        if s16(st + off) != w:
            bad.append('phase 2: stats +0x%X = %d (want %d)' % (off, s16(st + off), w))
    if s32(m.rd(st + 0xA4, 4)) != 110:
        bad.append('phase 2: hp %d (want 110)' % s32(m.rd(st + 0xA4, 4)))
    tree = bytes(m.rd(tree_at + i, 1) for i in range(108))
    r = tree
    if (r[0], struct.unpack_from('<H', r, 2)[0], r[20], r[21], struct.unpack_from('<I', r, 24)[0], r[28], r[32],
            struct.unpack_from('<b', r, 33)[0], struct.unpack_from('<I', r, 40)[0]) != \
            (1, 0xF0, 1, 0, tree_at + 28, 8, 6, -1, 0):
        bad.append('the tree: sphere %s / box head %s' % (r[:28].hex(), r[28:44].hex()))
    cx, cy, cz, rad = struct.unpack_from('<4i', r, 4)
    (x0, x1), (y0, y1), (z0, z1) = DT_PHASE2_BOX
    for c in ((x0, y0, z0), (x1, y1, z1), (x0, y1, z0), (x1, y0, z1)):
        if (c[0] - cx) ** 2 + (c[1] - cy) ** 2 + (c[2] - cz) ** 2 > rad * rad:
            bad.append('the sphere (%d, %d, %d) r %d misses the corner %s' % (cx, cy, cz, rad, c))
    mid = ((x0 + x1) // 2, (y0 + y1) // 2, (z0 + z1) // 2)
    for axis, (lo, hi) in enumerate(DT_PHASE2_BOX):
        for v, want in ((lo + 1, True), (lo, False), (hi - 1, True), (hi, False)):
            p = list(mid)
            p[axis] = v
            if dt_phase2_inside(tree, 0, p) != want:
                bad.append('the box: %s %s (want %s)' % (p, 'inside' if not want else 'outside',
                                                          'inside' if want else 'outside'))
    # phase 2 again: nothing more kept (the truck stays the one saved)
    got = run(0xF0, 1 << H)
    runs += 1
    if got != (1, tree_at, TRUCK, 0):
        bad.append('phase 2 twice: %s' % (got,))
    # another human's bit: phase 1 for this one, the truck back
    got = run(0xF0, 1 << (H + 1))
    runs += 1
    if got != (0, TRUCK, 0, 1) or stats() != sentinel:
        bad.append('phase 1 again: flag %d, model %08X, kept %08X, head +0xC %d, stats %s (want 0, %08X, 0, 1, '
                   'untouched)' % (got + ('touched' if stats() != sentinel else 'untouched', TRUCK)))
    # another type: nothing
    m.wr(A('aiBossDefeatedFlag'), 4, 7)
    got = run(0x10, 1 << H)
    runs += 1
    if got != (7, TRUCK, 0, 1) or stats() != sentinel:
        bad.append('type 0x10: %s, stats %s' % (got, 'touched' if stats() != sentinel else 'untouched'))
    good = not bad
    print('29. request 118: portCarDtPlayerStart (%d runs): phase 2 = the stage flag, stage 2\'s stats (110 HP, mass '
          '50, ride 50), the hit-only model (the head\'s box at -24, x %d..%d y %d..%d z %d..%d) for the truck, the '
          'head out of the hit pass; phase 1 puts the truck back; other types untouched -> %s' % (
              runs, x0, x1, y0, y1, z0, z1, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


# check 30 (request 120, PLAYTEST_ANSWERS.md 34): Antarctica's fire missile moved out of the mark that hides it
GLACIER_FIRE = {0x80051B40: (600, -800, 4800), 0x80051B64: (600, -800, 4851)}


def check_glacier_fire(out):
    """Request 120: portGlacierFire in LinkMips on the disc's GLACIER.DMD at 0x80019C40: on level 6 the fire pickup
    (80051B40) and its LOD (80051B64) at (992, 119), z kept, and nothing else of the file changed; a second call, level
    5, or one checked word off: nothing changed; the other levels' files: nothing."""
    import struct
    from check_rollback import Image
    from dmd_lod_dump import Disc, Dmd, DEFAULT_BIN
    img = Image(out)
    if 'portGlacierFire' not in img.sizes:
        print('30. request 120 Antarctica fire missile -> FAIL: portGlacierFire not in the link')
        return False
    disc = Disc(DEFAULT_BIN)
    bad, runs = [], 0

    def load(name, level):
        dmd = Dmd(name, disc.get('/DMD/%s.DMD' % name))
        lm = LinkMips(img, {'gameGetLevel': lambda m: level})
        lm.m.fill(HIDDEN_LEVEL_AT, dmd.d)
        return lm, dmd

    def image(lm, dmd):
        return bytes(lm.m.rd(HIDDEN_LEVEL_AT + i, 1) for i in range(len(dmd.d)))

    lm, dmd = load('GLACIER', 6)
    rv = s32(lm.run('portGlacierFire', (), steps=20000))
    runs += 1
    want = bytearray(dmd.d)
    for a, (x, y, z) in GLACIER_FIRE.items():
        struct.pack_into('<ii', want, a - HIDDEN_LEVEL_AT + 4, 992, 119)
    got = image(lm, dmd)
    if rv != 1 or got != bytes(want):
        diff = [hex(HIDDEN_LEVEL_AT + i) for i in range(len(got)) if got[i] != want[i]][:6]
        bad.append('GLACIER level 6: returned %d, bytes off at %s' % (rv, diff))
    rv = s32(lm.run('portGlacierFire', (), steps=20000))
    runs += 1
    if rv != 0 or image(lm, dmd) != bytes(want):
        bad.append('GLACIER again: returned %d / changed' % rv)
    for level, poke in ((5, None), (6, (0x80051B7C + 44, 0x80051B04)), (6, (0x80051B40 + 2, 402)),
                        (6, (0x80051B64 + 12, 4850))):
        lm, dmd = load('GLACIER', level)
        if poke:
            lm.m.wr(poke[0], 4 if poke[1] > 0xFFFF else 2, poke[1])
        before = image(lm, dmd)
        rv = s32(lm.run('portGlacierFire', (), steps=20000))
        runs += 1
        if rv != 0 or image(lm, dmd) != before:
            bad.append('GLACIER level %d, %s: returned %d / changed' % (level, poke and hex(poke[0]), rv))
    for name in ('GLACIER2', 'PARIS', 'HKONG', 'BURB'):
        lm, dmd = load(name, 6)
        before = image(lm, dmd)
        rv = s32(lm.run('portGlacierFire', (), steps=20000))
        runs += 1
        if rv != 0 or image(lm, dmd) != before:
            bad.append('%s: returned %d / changed' % (name, rv))
    good = not bad
    print('30. request 120: portGlacierFire (%d runs): on GLACIER.DMD level 6 the fire pickup and its LOD at (992, 119), '
          'nothing else; again, level 5, a checked word off, another file: nothing -> %s' % (runs, 'ok' if good else
                                                                                         'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


def check_dt_swoop(out):
    """Request 121 item 1 (PLAYTEST_ANSWERS.md 35): portDtSwoopPick / Pos / Live / Hit in LinkMips on made-up records
    (aiCarCount 4, humanCarNearest 1, the car objects and the effects stubbed). Pick: the boss's aiCarTargets entry,
    a computer car alive and drawn, gives 0x100 + it; -1, himself, past the count, dead, HP 0 or not drawn give the
    nearest player. Pos: the record's motion.pos (+0x73C a computer car's, +0x6FC a player's). Live: 1 for a player,
    a computer car's as Pick's test. Hit: carApplyDamage(its object's id, 7, darkToothSwoopVectors + 3, 0, the
    boss's slot), then (not burning, shielded, in water or in fire) burning 1, burn_time 0, fire_effect the flame's,
    fxSmokeA12 and fxExplosion24 at its position and sound 6, in retail's order; burning already: the damage only."""
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('portDtSwoopPick', 'portDtSwoopPos', 'portDtSwoopLive', 'portDtSwoopHit', 'aiCars', 'humanCars',
            'aiCarTargets', 'darkToothSwoopVectors')
    miss = [n for n in need if n not in syms]
    if miss:
        print('31. request 121 Dark Tooth swoop -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    A = lambda n: syms[n]['address']
    ai, hu, tg = A('aiCars'), A('humanCars'), A('aiCarTargets')
    OBJ = 0x80A00000
    log = []
    stubs = {'aiCarCount': lambda m: 4,
             'humanCarNearest': lambda m: log.append(('nearest', m.r[4])) or 1,
             'aiCarObject': lambda m: OBJ + 0x200 * (m.r[4] & 0xFF),
             'humanCarObject': lambda m: OBJ + 0x2000 + 0x200 * (m.r[4] & 0xFF),
             'carApplyDamage': lambda m: log.append(('damage', m.r[4], m.r[5], m.r[6], m.r[7], m.rd(m.r[29] + 16, 4))),
             'fxFlameBurning': lambda m: log.append(('flame', m.r[4], m.r[5])) or 0x80B00000,
             'fxSmokeA12': lambda m: log.append(('smoke', m.r[4])),
             'fxExplosion24': lambda m: log.append(('boom', m.r[4])),
             'soundPlay': lambda m: log.append(('sound', m.r[4])),
             'rand': lambda m: log.append(('rand',)) or 0}
    bad, runs = [], 0

    def setup(target, dead=0, hp=50, visible=1):
        lm = LinkMips(img, stubs)
        m = lm.m
        m.fill(ai, bytes(4 * 0x854))
        m.fill(hu, bytes(2 * 0x814))
        for j in range(4):
            m.wr(ai + j * 0x854 + 4, 2, j)
            m.wr(ai + j * 0x854 + 0x5A0, 4, 50)
            m.fill(OBJ + 0x200 * j, bytes(0x120))
            m.wr(OBJ + 0x200 * j + 8, 4, 1)
            m.wr(OBJ + 0x200 * j + 0x118, 4, 60 + j)
        for h in range(2):
            m.fill(OBJ + 0x2000 + 0x200 * h, bytes(0x120))
            m.wr(OBJ + 0x2000 + 0x200 * h + 0x118, 4, 1 + h)
        m.wr(ai + 2 * 0x854 + 0x6C, 1, dead)
        m.wr(ai + 2 * 0x854 + 0x5A0, 4, hp)
        m.wr(OBJ + 0x200 * 2 + 8, 4, visible)
        for j in range(4):
            m.wr(tg + 4 * j, 4, -1 & 0xFFFFFFFF)
        m.wr(tg, 4, target & 0xFFFFFFFF)
        return lm

    def s32v(v):
        return v - (1 << 32) if v & 0x80000000 else v

    for target, kw, want in ((2, {}, 0x102), (-1, {}, 1), (0, {}, 1), (5, {}, 1), (2, {'dead': 1}, 1),
                             (2, {'hp': 0}, 1), (2, {'visible': 0}, 1), (3, {}, 0x103)):
        lm = setup(target, **kw)
        log.clear()
        got = s32v(lm.run('portDtSwoopPick', (ai,), steps=200000))
        runs += 1
        if got != want:
            bad.append('Pick, target %d %s: %#x (want %#x)' % (target, kw or '', got, want))
        if want == 1 and ('nearest', ai + 0x73C) not in log:
            bad.append('Pick, target %d %s: humanCarNearest not asked with his position' % (target, kw or ''))
        live = s32v(lm.run('portDtSwoopLive', (0x102,), steps=200000))
        runs += 1
        if live != (1 if target == 2 and not kw else 0 if target == 2 else live):
            bad.append('Live(0x102), %s: %d' % (kw or 'alive', live))
    lm = setup(2)
    for code, want in ((0x102, ai + 2 * 0x854 + 0x73C), (1, hu + 0x814 + 0x6FC), (0, hu + 0x6FC)):
        got = lm.run('portDtSwoopPos', (code,), steps=200000)
        runs += 1
        if got != want:
            bad.append('Pos(%#x) = %08X (want %08X)' % (code, got, want))
    if s32v(lm.run('portDtSwoopLive', (1,), steps=200000)) != 1:
        bad.append('Live(1) for a player is not 1')
    vec = A('darkToothSwoopVectors') + 12
    for code, rec, st, stats, wp, mo, obj_id in ((0x102, ai + 2 * 0x854, 0x6C, 0x4FC, 0x808, 0x71C, 62),
                                                (1, hu + 0x814, 0x9C, 0x14C, 0x7C8, 0x6DC, 2)):
        for burning in (0, 1):
            lm = setup(2)
            m = lm.m
            m.wr(rec + st + 0x1A, 1, burning)
            m.wr(rec + stats + 0x22, 2, 77)
            log.clear()
            lm.run('portDtSwoopHit', (code, 7), steps=200000)
            runs += 1
            pos = rec + mo + 0x20
            want = [('damage', obj_id, 7, vec, 0, 7)]
            if not burning:
                want += [('flame', pos, 1), ('smoke', pos), ('boom', pos), ('sound', 6)]
            if log != want:
                bad.append('Hit(%#x), burning %d: calls %s (want %s)' % (code, burning, log, want))
            if not burning and (m.rd(rec + st + 0x1A, 1) != 1 or m.rd(rec + stats + 0x22, 2) != 0
                                or m.rd(rec + wp + 0x44, 4) != 0x80B00000):
                bad.append('Hit(%#x): burning / burn_time / fire_effect not set' % code)
            if burning and m.rd(rec + stats + 0x22, 2) != 77:
                bad.append('Hit(%#x), burning already: burn_time changed' % code)
    good = not bad
    print('31. request 121: the Dark Tooth swoop (%d runs): his own target (a computer car alive and drawn) or the '
          'nearest player; its position; a computer car that left ends it; the hit\'s damage and burn on either '
          'record, retail\'s order -> %s' % (runs, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


def check_human_wreck(out):
    """Request 126 (PLAYTEST_ANSWERS.md 40, port_human_wreck.h): humanCarDamageUpdate calls portHumanWreckTick first and
    portHumanWreckStart after carDestroyed; carWreckUpdate calls portHumanWreckBreak once and portHumanWreckBurns. In
    LinkMips (humanCarObject, timerGetFrameTicks 3, rand, the effects, carBodyObjectShow and carApplyDamage stubbed)
    on a made-up player record (dead, HP 0, wreck timer 120 as carDestroyed leaves it):
    - Start: a car in play gets carBodyObjectShow(key), its small-blast flag cleared and the mark; a car out (+8 = 0)
      or without an object neither;
    - carWreckUpdate each step, marked: the computer cars' flame at the first step, the small blast (f26) at step 361
      (timer < -960) and the break-up at step 441 (timer < -1200, 22 s at 60): carApplyDamage(its id, 15, 0, 2, 0)
      once, the flame out, the mark cleared; not marked: neither the blast nor the break-up (retail); marked with HP
      back above 0 (a respawn) at step 200: no break-up;
    - Tick: HP above 0 or the car hidden clears the mark and puts the flame out; a marked wreck in play stays marked."""
    from check_rollback import Image
    img = Image(out)
    link = img.link
    rows, kinds = load_functions(out)
    sizes = {r[2]: (r[0], r[1]) for r in rows}
    need = ('portHumanWreckStart', 'portHumanWreckTick', 'portHumanWreckBurns', 'portHumanWreckBreak',
            'portHumanWreckLive')
    miss = [n for n in need if n not in link.symbols] + [n for n in ('humanCarDamageUpdate', 'carWreckUpdate')
                                                         if n not in sizes]
    if miss:
        print('33. request 126 human wreck -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    bad = []
    tables = jump_tables(out)
    names_at = {r[0]: r[2] for r in rows}

    def flow_of(fn):
        return Flow(link, sizes[fn][0], sizes[fn][1], tables, names_at)

    flow = flow_of('humanCarDamageUpdate')
    calls = sorted(flow.calls.items())
    at = lambda fl, name: [a for a, c in sorted(fl.calls.items()) if c == name]
    tick, start, dest = at(flow, 'portHumanWreckTick'), at(flow, 'portHumanWreckStart'), at(flow, 'carDestroyed')
    if len(tick) != 1 or len(start) != 1 or len(dest) != 1:
        bad.append('humanCarDamageUpdate: portHumanWreckTick x%d, portHumanWreckStart x%d, carDestroyed x%d '
                   '(want 1 each)' % (len(tick), len(start), len(dest)))
    else:
        if calls[0][0] != tick[0]:
            bad.append('humanCarDamageUpdate: portHumanWreckTick is not its first call')
        if not (start[0] > dest[0] and start[0] in flow.reach(dest[0] + 8, set())):
            bad.append('portHumanWreckStart not after carDestroyed')
    wflow = flow_of('carWreckUpdate')
    if len(at(wflow, 'portHumanWreckBreak')) != 1 or not at(wflow, 'portHumanWreckBurns'):
        bad.append('carWreckUpdate: portHumanWreckBreak x%d, portHumanWreckBurns x%d (want 1, >= 1)' % (
            len(at(wflow, 'portHumanWreckBreak')), len(at(wflow, 'portHumanWreckBurns'))))

    live = link.symbols['portHumanWreckLive']['address']
    OBJ, REC, FIRE, JIT = 0x80A00000, 0x80A10000, 0x80A40000, 0x80A48000
    obj_of = lambda i: OBJ + 0x200 * i
    rec_of = lambda i: REC + 0x900 * i
    log = []
    state = {'has_obj': True}
    stubs = {'humanCarObject': lambda m: obj_of(m.r[4] & 0xFF) if state['has_obj'] else 0,
             'humanCarGet': lambda m: rec_of(m.r[4] & 0xFF),
             'timerGetFrameTicks': lambda m: 3,
             'rand': lambda m: 0x7FFF,
             'mathJitterPos': lambda m: JIT,
             'carBodyObjectShow': lambda m: log.append(('show', m.r[4])),
             'carApplyDamage': lambda m: log.append(('damage', m.r[4], m.r[5], m.r[6], m.r[7],
                                                     m.rd(m.r[29] + 16, 4))),
             'fxFlame10': lambda m: log.append(('flame', m.r[5])) or FIRE,
             'fxExplosion24': lambda m: log.append(('blast24',)),
             'fxExplosion48': lambda m: log.append(('blast48',)),
             'fxExplosion96': lambda m: log.append(('blast96',)),
             'fxSmokeA48': lambda m: log.append(('smoke48',)),
             'fxSmokeA24': lambda m: None,
             'fxSmokeA12': lambda m: None}
    runs = 0

    def setup(i, visible=1):
        lm = LinkMips(img, stubs)
        m = lm.m
        o, r = obj_of(i), rec_of(i)
        m.fill(r, bytes(0x900))
        m.wr(o + 8, 4, visible)
        m.wr(o + 0x118, 4, 40 + i)              # its object id
        m.wr(r, 2, i)                           # player
        m.wr(r + 0x9C, 1, 1)                    # state.dead
        m.wr(r + 0xC2, 1, 1)                    # state.f26 (left set by an earlier wreck)
        m.wr(r + 0x172, 2, 120)                 # stats.wreck_timer, as carDestroyed leaves it
        m.wr(r + 0x1F0, 4, 0)                   # stats.hp
        m.wr(FIRE + 0x0C, 2, 0)
        return lm

    # Start
    for i in (0, 2):
        for visible, has in ((1, True), (0, True), (1, False)):
            state['has_obj'] = has
            log.clear()
            lm = setup(i, visible)
            lm.run('portHumanWreckStart', (i, 0x1230 + i))
            runs += 1
            ok_car = visible and has
            got = (list(log), lm.m.rd(live + 4 * i, 4), lm.m.rd(rec_of(i) + 0xC2, 1))
            want = ([('show', 0x1230 + i)] if ok_car else [], 1 if ok_car else 0, 0 if ok_car else 1)
            if got != want:
                bad.append('Start human %d visible %d object %s: calls, mark, f26 %s (want %s)' % (
                    i, visible, has, got, want))
    state['has_obj'] = True

    # carWreckUpdate, the wreck's life
    def life(i, marked, steps=600, respawn_at=None):
        log.clear()
        lm = setup(i)
        m = lm.m
        m.wr(rec_of(i) + 0xC2, 1, 0)
        m.wr(live + 4 * i, 4, 1 if marked else 0)
        first_flame = blast = broke = None
        breaks = 0
        for s in range(1, steps + 1):
            if respawn_at == s:
                m.wr(rec_of(i) + 0x1F0, 4, 100)
            n0 = len(log)
            lm.run('carWreckUpdate', (rec_of(i), 1))
            new = log[n0:]
            if first_flame is None and ('flame', 1) in new:
                first_flame = s
            if blast is None and m.rd(rec_of(i) + 0xC2, 1):
                blast = s
            d = [c for c in new if c[0] == 'damage']
            breaks += len(d)
            if d and broke is None:
                broke = (s, d[0])
        return (first_flame, blast, broke, breaks, m.rd(live + 4 * i, 4), m.rd(rec_of(i) + 0xB6, 1),
                m.rd(FIRE + 0x0C, 2) if m.rd(rec_of(i) + 0x80C, 4) == FIRE else None)

    got = life(1, True)
    runs += 1
    want = (1, 361, (441, ('damage', 41, 15, 0, 2, 0)), 1, 0, 0, 0)
    if got != want:
        bad.append('marked wreck: first computer-car flame, small blast, break-up, break-ups, mark, burning, fire '
                   'active %s (want %s)' % (got, want))
    got = life(1, False)
    runs += 1
    if got[0] is not None or got[1] is not None or got[3] != 0:
        bad.append('unmarked wreck (retail): computer-car flame %s, small blast %s, break-ups %d (want none)' % (
            got[0], got[1], got[3]))
    got = life(3, True, respawn_at=200)
    runs += 1
    if got[3] != 0:
        bad.append('marked wreck with HP back at step 200: %d break-ups (want 0)' % got[3])

    # Tick
    for label, hp, visible, want_mark in (('respawn', 100, 1, 0), ('hidden', 0, 0, 0), ('in play', 0, 1, 1)):
        lm = setup(2, visible)
        m = lm.m
        m.wr(live + 8, 4, 1)
        m.wr(rec_of(2) + 0x1F0, 4, hp)
        m.wr(rec_of(2) + 0xB6, 1, 1)
        m.wr(rec_of(2) + 0x80C, 4, FIRE)
        m.wr(FIRE + 0x0C, 2, 300)
        lm.run('portHumanWreckTick', (2,))
        runs += 1
        got = (m.rd(live + 8, 4), m.rd(rec_of(2) + 0xB6, 1), m.rd(FIRE + 0x0C, 2))
        want = (1, 1, 300) if want_mark else (0, 0, 0)
        if got != want:
            bad.append('Tick %s: mark, burning, fire active %s (want %s)' % (label, got, want))

    print('33. request 126: a destroyed player\'s wreck lives a computer car\'s wreck\'s life (%d runs): Tick first '
          'and Start after carDestroyed in humanCarDamageUpdate; carWreckUpdate: the flame, the small blast at step '
          '361, blown apart at step 441 (22 s), retail unmarked, none after a respawn -> %s' % (
              runs, 'ok' if not bad else 'FAIL'))
    for b in bad[:8]:
        print('     ' + b)
    return not bad


def check_team_locks(out):
    """Request 123 item 3 (PLAYTEST_ANSWERS.md 37.3): Grasshopper's leap (weaponHomingLeapInit) and Outlaw's taser
    (weaponTaserTarget) in LinkMips on made-up records (2 players, 3 computer cars on one line, the car objects,
    ratan2, the arc slot, the effects and carSpinOut stubbed), firing from computer car 0 and from player 1, under
    team tables in portCarTeam: each picks the nearest car in reach that is not itself and not of the firer's team
    (both teams nonzero and equal), and with every team 0 retail's nearest car; with no foe in reach, no lock (the
    leap stays inactive, the taser's arc has no target and nobody spins). portTeamMates on its own: equal nonzero
    teams only, slots out of range 0 (the table's size from the link: 15 or 64 slots). portTeamNearestFoe (the player
    at aiCarTargets -1) and aiGetTargetCar at -1: the nearest player (|dx| + |dy|) not of the car's team; every
    player a teammate: retail's humanCarNearest; a target >= 0: its computer car as retail."""
    from check_rollback import Image
    img = Image(out)
    syms = img.link.symbols
    need = ('portTeamMates', 'portCarTeam', 'weaponHomingLeapInit', 'weaponTaserTarget', 'weaponTaserVictim',
            'weaponArcs')
    miss = [n for n in need if n not in syms]
    if miss:
        print('32. request 123 team locks -> FAIL: %s not in the link' % ', '.join(miss))
        return False
    A = lambda n: syms[n]['address']
    team, arcs, victim = A('portCarTeam'), A('weaponArcs'), A('weaponTaserVictim')
    leap = A('homingLeapState') if 'homingLeapState' in syms else 0x80184DD4    # (#pragma tm2_absolute: retail's)
    HU, AI, OBJ = 0x80A00000, 0x80A10000, 0x80A40000
    HSZ, ASZ = 0x814, 0x854
    log = []
    stubs = {'humanCarCount': lambda m: 2,
             'aiCarCount': lambda m: 3,
             'humanCarGet': lambda m: HU + HSZ * s32(m.r[4]),
             'aiCarGet': lambda m: AI + ASZ * s32(m.r[4]),
             'humanCarObject': lambda m: OBJ + 0x200 * s32(m.r[4]),
             'aiCarObject': lambda m: OBJ + 0x1000 + 0x200 * s32(m.r[4]),
             'ratan2': lambda m: 0,
             'lightFlickerStart': lambda m: log.append(('flicker', m.r[4])),
             'weaponArcSlotFind': lambda m: 0,
             'portSpecialNum': lambda m: m.r[6],
             'carSpinOut': lambda m: log.append(('spin', m.r[4], m.r[5])),
             'rand': lambda m: log.append(('rand',)) or 0}
    # the cars on one line: (kind, index, record, object, slot, x); a record's motion.pos is its object's position
    def cars(scale):
        out_ = []
        for h, x in ((0, 800), (1, 1600)):
            out_.append(('h', h, HU + HSZ * h, OBJ + 0x200 * h, h, x // scale, 1 + h, 0x6FC))
        for j, x in ((0, 0), (1, 1200), (2, 2400)):
            out_.append(('a', j, AI + ASZ * j, OBJ + 0x1000 + 0x200 * j, 10 + j, x // scale, 50 + j, 0x73C))
        return out_

    def setup(scale, teams):
        lm = LinkMips(img, stubs)
        m = lm.m
        m.fill(team, bytes(64))
        for kind, i, rec, obj, slot, x, ident, pos in cars(scale):
            m.fill(rec, bytes(ASZ))
            m.fill(obj, bytes(0x120))
            if kind == 'h':
                m.wr(rec + 0, 2, i)
                m.wr(rec + 4, 2, slot)
            else:
                m.wr(rec + 4, 2, i)
                m.wr(rec + 6, 2, slot)
                m.wr(rec + 0x5A0, 4, 50)
            m.wr(rec + pos, 4, x)
            m.wr(obj + 0xA8, 4, x)
            m.wr(obj + 0x118, 4, ident)
            m.wr(team + slot, 1, teams[slot])
        return lm

    def want(scale, teams, firer, limit):
        line = cars(scale)
        f = next(c for c in line if c[:2] == firer)
        best, pick = limit, None
        for c in line:
            if c is f:
                continue
            d = abs(c[5] - f[5]) >> 3
            if d < best and not (teams[f[4]] and teams[f[4]] == teams[c[4]]):
                best, pick = d, c
        return f, pick

    TABLES = ((0, 0, 0, 0, 0), (1, 2, 1, 2, 2), (1, 2, 1, 1, 2), (1, 1, 1, 1, 1), (1, 2, 0, 2, 2),
              (2, 1, 2, 2, 1), (0, 1, 1, 1, 1), (3, 3, 3, 3, 3))
    bad, runs = [], 0
    for tbl in TABLES:
        teams = dict(zip((0, 1, 10, 11, 12), tbl))
        # the leap, from computer car 0 (mode 0) and player 1 (mode 1)
        for firer in (('a', 0), ('h', 1)):
            lm = setup(1, teams)
            m = lm.m
            m.wr(leap, 1, 0)
            m.wr(leap + 0x34, 4, 0xDEAD0000)
            f, pick = want(1, teams, firer, 400)
            log.clear()
            lm.run('weaponHomingLeapInit', (f[2], 1 if firer[0] == 'h' else 0), steps=400000)
            runs += 1
            got = (m.rd(leap, 1), m.rd(leap + 0x34, 4))
            exp = (1, pick[2] + pick[7]) if pick else (0, 0xDEAD0000)
            if got != exp or ('rand',) in log:
                bad.append('leap from %s%d, teams %s: active %d target %08X (want %d %08X)%s' % (
                    firer[0], firer[1], tbl, got[0], got[1], exp[0], exp[1], ', rand()' if ('rand',) in log else ''))
        # the taser (reach 80: the line at a tenth), from computer car 0 and player 1
        for firer in (('a', 0), ('h', 1)):
            lm = setup(10, teams)
            m = lm.m
            m.wr(victim, 4, 0xDEAD0000)
            m.wr(arcs + 8, 2, 0x7777)
            f, pick = want(10, teams, firer, 80)
            log.clear()
            lm.run('weaponTaserTarget', (f[2], 1 if firer[0] == 'h' else 0, 7), steps=400000)
            runs += 1
            spins = [e for e in log if e[0] == 'spin']
            got = (m.rd(victim, 4), s32(m.rd(arcs + 8, 2) | (0xFFFF0000 if m.rd(arcs + 8, 2) & 0x8000 else 0)),
                   len(spins))
            exp = (pick[2], pick[6], 1) if pick else (0xDEAD0000, -1, 0)
            if got != exp:
                bad.append('taser from %s%d, teams %s: victim %08X id %d spins %d (want %08X %d %d)' % (
                    firer[0], firer[1], tbl, got[0], got[1], got[2], exp[0], exp[1], exp[2]))
    nslots = min(a_ for a_ in (s['address'] for s in syms.values()) if a_ > team) - team   # the table's room
    nslots = 64 if nslots >= 64 else 15        # TM2_MAX_CAR_SLOTS: 2 x 32 with TM2_PORT_CARS, retail's 15 (+ alignment)
    lm = setup(1, dict.fromkeys((0, 1, 10, 11, 12), 0))
    m = lm.m
    last = nslots - 1
    for slot, t in ((0, 1), (1, 1), (2, 2), (3, 0), (4, 0), (last, 1)):
        m.wr(team + slot, 1, t)
    for a, b, exp in ((0, 1, 1), (1, 0, 1), (0, 2, 0), (3, 4, 0), (0, 0, 1), (3, 3, 0), (0, last, 1), (-1, 0, 0),
                      (0, -1, 0), (0, nslots, 0), (0, 9999, 0), (9999, 9999, 0)):
        got = s32(lm.run('portTeamMates', (a, b), steps=10000))
        runs += 1
        if got != exp:
            bad.append('portTeamMates(%d, %d) = %d (want %d)' % (a, b, got, exp))
    # the player at -1: computer car k (x 0 / 1200 / 2400), players at x 800 and 1600; retail's |dx| + |dy| < 0x7FFF
    stubs['humanCarNearest'] = lambda m: log.append(('nearest', m.r[4])) or 77
    tg = A('aiCarTargets') if 'aiCarTargets' in syms else None
    hcars = A('humanCars') if 'humanCars' in syms else None
    for tbl in TABLES:
        teams = dict(zip((0, 1, 10, 11, 12), tbl))
        for k in range(3):
            lm = setup(1, teams)
            m = lm.m
            line = cars(1)
            me = line[2 + k]
            foes = [c for c in line[:2] if not (teams[me[4]] and teams[me[4]] == teams[c[4]])]
            best, exp = 0x7FFF, (foes[0][1] if foes else None)
            for c in foes:
                if abs(c[5] - me[5]) < best:
                    best, exp = abs(c[5] - me[5]), c[1]
            log.clear()
            got = s32(lm.run('portTeamNearestFoe', (k,), steps=400000))
            runs += 1
            if exp is None:
                if got != 77 or log != [('nearest', me[2] + me[7])]:
                    bad.append('portTeamNearestFoe(%d), teams %s: %d %s (want humanCarNearest: 77)' % (k, tbl, got, log))
            elif got != exp or log:
                bad.append('portTeamNearestFoe(%d), teams %s: %d (want %d)%s' % (k, tbl, got, exp, ' %s' % log if log else ''))
            if tg is None or hcars is None or 'aiGetTargetCar' not in img.sizes:
                continue
            for target in (-1, 2 - k if k != 1 else 0):
                m.wr(tg + 4 * k, 4, target & 0xFFFFFFFF)
                flag = 0x801FE000
                m.wr(flag, 1, 0x55)
                log.clear()
                got = lm.run('aiGetTargetCar', (k, flag), steps=400000)
                runs += 1
                if target == -1:
                    want_rec = hcars + 0x814 * (exp if exp is not None else 77)
                    want_flag = 1
                else:
                    want_rec = A('aiCars') + ASZ * target
                    want_flag = 0
                if got != want_rec or m.rd(flag, 1) != want_flag:
                    bad.append('aiGetTargetCar(%d) at %d, teams %s: %08X flag %d (want %08X %d)' % (
                        k, target, tbl, got, m.rd(flag, 1), want_rec, want_flag))
    good = not bad
    print('32. request 123: the team locks (%d runs): Grasshopper\'s leap and Outlaw\'s taser pick the nearest car of '
          'another team in reach (every team 0: retail\'s nearest; no foe: no lock); portTeamMates -> %s' % (
              runs, 'ok' if good else 'FAIL'))
    for b in bad[:12]:
        print('     ' + b)
    return good


def check(out, names, e0, defined=True, expect=False, limits=None):
    link = Link(out)
    rows, kinds = load_functions(out)
    tables = jump_tables(out)
    names_at = {r[0]: r[2] for r in rows}
    sizes = {r[2]: (r[0], r[1]) for r in rows}
    calls, callers, pointed = call_graph(link, rows)
    relocs, _ = link.relocations()
    los = lo_targets(relocs, link.symbols)
    ok = True
    dark_tooth = 'TM2_PORT_DARK_TOOTH' in names or not defined
    visual = 'TM2_PORT_VISUAL' in names or not defined
    numbers = 'TM2_PORT_CAR_NUMBERS' in names or not defined
    far_lod = 'TM2_PORT_FAR_LOD' in names or not defined
    far_distance = 'TM2_PORT_FAR_DISTANCE' in names or not defined
    c_emitter = 'TM2_PORT_C_EMITTER' in names or not defined
    render_slots = 'TM2_PORT_RENDER_SLOTS' in names or not defined
    carsel = numbers or 'TM2_PORT_DT_PLAYER' in names
    profile = 'TM2_PORT_CAR_PROFILE' in names or not defined
    hidden = 'TM2_PORT_HIDDEN_AREAS' in names or not defined
    special = 'TM2_PORT_SPECIAL_PROFILE' in names or not defined
    fall = 'TM2_PORT_FALL_WHEELS' in names or not defined
    pose = 'TM2_PORT_CAR_POSE' in names or not defined
    whole = 'TM2_PORT_WHOLE_MAPS' in names or not defined
    fwy_ground = 'TM2_PORT_FWY_GROUND' in names or 'TM2_PORT_FWY_PARAPETS' in names or not defined
    mod_arena = 'TM2_PORT_MOD_ARENA' in names or not defined
    wreck = 'TM2_PORT_WRECK_FALL' in names or not defined
    dt_player = 'TM2_PORT_DT_PLAYER' in names or not defined
    glacier_fire = 'TM2_PORT_GLACIER_FIRE' in names or not defined
    dt_swoop = 'TM2_PORT_DT_SWOOP' in names or not defined
    team_locks = 'TM2_PORT_TEAM_LOCKS' in names or not defined
    human_wreck = 'TM2_PORT_HUMAN_WRECK' in names or not defined

    def port_fns(label, fns, rand_reach=True, allowed_rand=()):
        missing = sorted(n for n in fns if kinds.get(n) != 'port')
        bad = sorted(n for n in fns if n in kinds and n not in allowed_rand and
                     (reaches(calls, n) if rand_reach else 'rand' in calls.get(n, ())))
        direct = sorted(n for n in allowed_rand if n in kinds and 'rand' in calls.get(n, ()))
        good = not missing and not bad and not direct
        print('%s: %d port functions in .port_text (missing %s); reaching rand(): %s -> %s' % (
            label, len(fns) - len(missing), missing or 'none', (bad + direct) or 'none', 'ok' if good else 'FAIL'))
        return good

    def callers_are(label, table):
        bad = []
        for fn, want in table.items():
            got = callers.get(fn, set())
            if fn not in kinds:
                bad.append('%s not in the link' % fn)
            elif got != want:
                bad.append('%s called by %s (want %s)' % (fn, ' '.join(sorted(got)) or 'nothing',
                                                         ' '.join(sorted(want)) or 'nothing'))
            if fn in pointed:
                bad.append('%s pointed to from %s' % (fn, ' '.join(sorted(pointed[fn]))))
        good = not bad
        print('%s: %d functions -> %s' % (label, len(table), 'ok' if good else 'FAIL'))
        for b in bad[:12]:
            print('     ' + b)
        return good

    # 1. the ordinary Dark Tooth
    if dark_tooth:
        ok = port_fns('1. ordinary Dark Tooth', DT_ORD_PORT) and ok
        fn = 'portCarDarkToothOrdinary'
        if fn in sizes:
            good, text = bit_gate_returns_zero(link, sizes[fn][0], sizes[fn][1], los, 'portCarOptions', 0x400)
        else:
            good, text = False, '%s not in the link' % fn
        print('1. bit-10 gate: %s: %s -> %s' % (fn, text, 'ok' if good else 'FAIL'))
        ok = ok and good
        ok = callers_are('1. callers of the ordinary Dark Tooth functions', DT_ORD_CALLERS) and ok

    # 2. the visual switches
    if visual:
        ok = port_fns('2. visual switches', VISUAL_PORT) and ok
        callees = sorted(calls.get('portVisualArcOtz', ()))
        good = 'portVisualArcOtz' in kinds and not callees
        print('2. portVisualArcOtz calls %s -> %s' % (' '.join(callees) or 'nothing', 'ok' if good else 'FAIL'))
        ok = ok and good
        ok = callers_are('2. callers of portVisualArcOtz', {'portVisualArcOtz': {'weaponArcsDraw'}}) and ok
        # request 76 bit 1: the taser from Outlaw 2's light bar
        callees = set(calls.get('portVisualTaserFrom', ()))
        good = 'portVisualTaserFrom' in kinds and callees == {'mathMulTransVec'}
        print('2. portVisualTaserFrom calls %s -> %s' % (' '.join(sorted(callees)) or 'nothing', 'ok' if good else 'FAIL'))
        ok = ok and good
        ok = callers_are('2. callers of portVisualTaserFrom', {'portVisualTaserFrom': {'weaponArcsDraw'}}) and ok
        ok = taser_start(out) and ok
        # request 45 bit 2: the own car first
        callees = set(calls.get('portVisualOwnCar', ()))
        good = 'portVisualOwnCar' in kinds and callees <= {'viewGetMode', 'portLocalHuman'} and 'viewGetMode' in callees
        print('2. portVisualOwnCar calls %s -> %s' % (' '.join(sorted(callees)) or 'nothing', 'ok' if good else 'FAIL'))
        ok = ok and good
        ok = callers_are('2. callers of portVisualOwnCar', {'portVisualOwnCar': {'renderScene'}}) and ok
        if 'renderScene' in sizes:
            fr = Flow(link, sizes['renderScene'][0], sizes['renderScene'][1], tables, names_at)
            own = sorted(a for a, c in fr.calls.items() if c == 'portVisualOwnCar')
            walks = sorted(a for a, c in fr.calls.items() if c == 'renderWalkModel')
            good = len(own) == 1 and walks and own[0] < walks[0]
            print('2. renderScene: %d portVisualOwnCar calls, before the first renderWalkModel: %s -> %s' % (
                len(own), bool(own and walks and own[0] < walks[0]), 'ok' if good else 'FAIL'))
            ok = ok and good
            # HIDDEN_AREAS.md 10: the object counter (the walker's translation key) stepped again between the own
            # car's walk and the level root's, so the level's top-level meshes never share the car's last key
            incs = counter_increments(link, sizes['renderScene'][0], sizes['renderScene'][1])
            before = [b for a, b in incs if walks and a < walks[0]]
            base = before[-1] if before else None
            between = [a for a, b in incs if len(walks) > 1 and walks[0] < a < walks[1] and b == base]
            good = base is not None and len(between) == 1
            print('2. renderScene: the object counter (0(%s)) stepped between the own car\'s walk and the level\'s: '
                  '%d -> %s' % (base or '?', len(between), 'ok' if good else 'FAIL'))
        else:
            good = False
            print('2. renderScene not in the link -> FAIL')
        ok = ok and good
        rs = (ROOT / 'src/semantic/engine_render_scene.c').read_text(encoding='utf-8')
        good = ('if (RENDER_OBJ_WALKED(obj)) {' in rs and '#define RENDER_OBJ_WALKED(obj) (*(s32 *)((obj) + 8) && (obj) != own && (obj) != mirror_own)' in rs
                and '#define RENDER_OBJ_WALKED(obj) (*(s32 *)((obj) + 8))' in rs and rs.count('portVisualOwnCar(view)') == 1)
        print('2. source: renderScene walks the own car first and its loop skips it (RENDER_OBJ_WALKED) -> %s' % (
            'ok' if good else 'FAIL'))
        ok = ok and good
        fn = 'weaponArcsDraw'
        flow = Flow(link, sizes[fn][0], sizes[fn][1], tables, names_at) if fn in sizes else None
        if flow is None:
            print('2. weaponArcsDraw not in the link -> FAIL')
            ok = False
        else:
            mine = {c: n for c, n in calls_in(flow).items() if kinds.get(c) != 'port'}
            if e0 is not None:
                l0 = Link(e0)
                r0, k0 = load_functions(e0)
                s0 = {r[2]: (r[0], r[1]) for r in r0}
                f0 = Flow(l0, s0[fn][0], s0[fn][1], jump_tables(e0), {r[0]: r[2] for r in r0})
                retail = calls_in(f0)
                good = mine == retail
                print('2. weaponArcsDraw calls the retail functions as E0 does (rand() %d, AddPrim %d) -> %s' % (
                    mine.get('rand', 0), mine.get('AddPrim', 0), 'ok' if good else 'FAIL: %s vs E0 %s' % (mine, retail)))
                ok = ok and good
            else:
                print('2. weaponArcsDraw: no E0 link, the retail call census not compared')
            # bit 0's gate: cut every bit-set edge of a branch on portVisual & 1
            bitregs_at = {}
            cut_edges = set()
            for a in sorted(flow.insn):
                w = flow.insn[a]
                op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
                if op == 0x0C and (w & 0xFFFF) == 1:
                    # andi rt, rs, 1: is rs loaded from portVisual within the 8 instructions before?
                    src = any(los.get(b) == 'portVisual' and ((flow.insn.get(b, 0) >> 16) & 31) == rs
                              for b in range(a - 32, a, 4))
                    if src:
                        bitregs_at[a] = rt
            gates = []
            for a, r in bitregs_at.items():
                for b in range(a + 4, min(a + 40, flow.end), 4):
                    w = flow.insn[b]
                    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
                    if op in (4, 5) and {rs, rt} == {r, 0}:
                        t = b + 4 + ((w & 0xFFFF) - (0x10000 if w & 0x8000 else 0)) * 4
                        gates.append((b, t if op == 5 else b + 8))   # bne: set -> target; beq: set -> fall through
                        break
            # the flow graph with the bit-set edges removed
            succ = {k: list(v) for k, v in flow.succ.items()}
            for b, set_edge in gates:
                succ[b + 4] = [s for s in succ.get(b + 4, []) if s != set_edge]
            seen, todo = set(), [flow.start]
            while todo:
                a = todo.pop()
                if a in seen or not flow.start <= a < flow.end:
                    continue
                seen.add(a)
                todo.extend(succ.get(a, ()))
            otz = [a for a, c in flow.calls.items() if c == 'portVisualArcOtz']
            addprim = [a for a, c in flow.calls.items() if c == 'AddPrim']
            leak = [a for a in otz if a in seen]
            good = bool(gates) and bool(otz) and not leak and any(a in seen for a in addprim)
            print('2. bit-0 gate: %d branches on portVisual & 1, %d portVisualArcOtz calls, reachable with the bit clear: '
                  '%s; AddPrim reachable: %s -> %s' % (len(gates), len(otz), ['%08X' % a for a in leak] or 'none',
                                                       any(a in seen for a in addprim), 'ok' if good else 'FAIL'))
            ok = ok and good
        src = (ROOT / 'src/semantic/weapon_arcs_draw.c').read_text(encoding='utf-8')
        port_defs = src.split('#ifdef TM2_PORT_VISUAL', 1)[1].split('#else', 1)[0] if '#ifdef TM2_PORT_VISUAL' in src else ''
        body = src.split('weaponArcsDraw(u8 **ctx, s32 view)', 1)[1] if 'weaponArcsDraw(u8 **ctx, s32 view)' in src else ''
        problems = []
        if 'TM2_VIS_TASER_LOW' not in port_defs or 'TM2_VIS_ARC_DEPTH' not in port_defs:
            problems.append('the port macros do not test the bits')
        if body.count('ARC_LIFT(i)') != 2 or body.count('ARC_JIT(i, j,') != 3 or 'AddPrim(ARC_OT(j)' not in body:
            problems.append('the lift / jitter / OT entry not all through the macros')
        if re.search(r'from\[2\]\s*\+\s*30', body) or 'AddPrim(ot,' in body:
            problems.append("retail's + 30 or AddPrim(ot, left in the body")
        good = not problems
        print('2. source: lift, jitter and OT entry through ARC_LIFT / ARC_JIT / ARC_OT -> %s' % (
            'ok' if good else 'FAIL: ' + '; '.join(problems)))
        ok = ok and good
        # request 46 bits 3 and 4
        ok = port_fns('2. request 46 visual switches', VISUAL_PORT_46) and ok
        ok = callers_are('2. callers of the request 46 visual functions', VISUAL_CALLERS_46) and ok
        if 'renderScene' in sizes:
            fr = Flow(link, sizes['renderScene'][0], sizes['renderScene'][1], tables, names_at)
            walks = sorted(a for a, c in fr.calls.items() if c == 'renderWalkModel')
            noted = sorted(a for a, c in fr.calls.items() if c == 'portVisualWalked')
            shadows = sorted(a for a, c in fr.calls.items() if c == 'portVisualShadows')
            emit = sorted(a for a, c in fr.calls.items() if c in ('sub_800CDC54', 'portFarDistanceEmit'))
            # each noted walk: a portVisualWalked call follows a renderWalkModel call with no other walk between
            pairs = sum(1 for n in noted if any(w < n and not any(w < w2 < n for w2 in walks) for w in walks))
            good = (len(walks) == 5 and len(noted) == 3 and pairs == 3 and len(shadows) == 1 and emit
                    and shadows[0] > walks[-1] and all(shadows[0] < e for e in emit))
            print('2. renderScene: %d walks, %d noted right after a walk (want 3 of 5: own car, world models, objects), '
                  '%d portVisualShadows after every walk and before the emitter -> %s' % (
                      len(walks), pairs, len(shadows), 'ok' if good else 'FAIL'))
        else:
            good = False
            print('2. renderScene not in the link -> FAIL')
        ok = ok and good
        if defined:
            ok = not check_steady_shadow(out, names) and ok     # 15. request 92
            ok = not check_whole_near(out, e0) and ok           # 16. request 93
            ok = not check_mirror_car(out) and ok               # 17. request 99
            ok = not check_shadow_ground(out) and ok            # 19. request 102 item 2
        if e0 is not None:
            l0 = Link(e0)
            r0, _k0 = load_functions(e0)
            s0 = {r[2]: (r[0], r[1]) for r in r0}
            for fn in ('bulletSpawn', 'streaksDraw'):
                if fn not in sizes or fn not in s0:
                    print('2. %s not in the link -> FAIL' % fn)
                    ok = False
                    continue
                f1 = Flow(link, sizes[fn][0], sizes[fn][1], tables, names_at)
                f0 = Flow(l0, s0[fn][0], s0[fn][1], jump_tables(e0), {r[0]: r[2] for r in r0})
                mine = {c: n for c, n in calls_in(f1).items() if kinds.get(c) != 'port'}
                retail = calls_in(f0)
                good = mine == retail
                print('2. %s calls the retail functions as E0 does (rand() %d) -> %s' % (
                    fn, mine.get('rand', 0), 'ok' if good else 'FAIL: %s vs E0 %s' % (mine, retail)))
                ok = ok and good
        pva = (ROOT / 'src/semantic/port_variant_arrays.c').read_text(encoding='utf-8')
        problems = []
        sh = pva.split('void PORT_TEXT portVisualShadows(void)', 1)[1].split('\n}\n', 1)[0] if \
            'void PORT_TEXT portVisualShadows(void)' in pva else ''
        if not sh or sh.find('portVisual & TM2_VIS_SHADOW') < 0 or sh.find('e[5] =') < sh.find('portVisual & TM2_VIS_SHADOW'):
            problems.append('portVisualShadows writes an entry outside `portVisual & TM2_VIS_SHADOW`')
        tl = pva.split('int *PORT_TEXT portVisualBulletTail(int slot, int *tail)', 1)[1].split('\n}\n', 1)[0] if \
            'int *PORT_TEXT portVisualBulletTail(int slot, int *tail)' in pva else ''
        if not re.search(r'if \(!\(portVisual & TM2_VIS_GUN_START\)[^;{]*\)\s*return tail;', tl):
            problems.append('portVisualBulletTail does not return the record\'s point first with bit 4 clear')
        rsrc = (ROOT / 'src/semantic/engine_render_scene.c').read_text(encoding='utf-8')
        if '#define RENDER_WALK(obj, model) renderWalkModel((obj), (model))' not in rsrc:
            problems.append("RENDER_WALK's #else is not retail's walk")
        scd = (ROOT / 'src/semantic/screen_card_draw.c').read_text(encoding='utf-8')
        if '#define STREAK_TAIL e->p1' not in scd or scd.count('STREAK_TAIL[') != 3:
            problems.append("streaksDraw's tail not through STREAK_TAIL (retail e->p1)")
        good = not problems
        print('2. source: bit 3 writes only with the bit, bit 4 returns the record\'s point with the bit clear, the '
              'macros\' retail forms -> %s' % ('ok' if good else 'FAIL: ' + '; '.join(problems)))
        ok = ok and good
        # request 79: the special's pickup look
        ok = port_fns('2. request 79 pickup look', PICKUP_SPECIAL_PORT) and ok
        ok = callers_are('2. callers of the request 79 functions', PICKUP_SPECIAL_CALLERS) and ok
        if 'worldLoadStartObjects' in sizes:
            fw = Flow(link, sizes['worldLoadStartObjects'][0], sizes['worldLoadStartObjects'][1], tables, names_at)
            build = sorted(a for a, c in fw.calls.items() if c == 'portPickupSpecialBuild')
            scans = sorted(a for a, c in fw.calls.items() if c == 'dbscanHierarchy')
            good = len(build) == 1 and scans and build[0] > scans[-1]
            print('2. worldLoadStartObjects: %d portPickupSpecialBuild calls, after its dbscanHierarchy: %s -> %s' % (
                len(build), bool(build and scans and build[0] > scans[-1]), 'ok' if good else 'FAIL'))
        else:
            good = False
            print('2. worldLoadStartObjects not in the link -> FAIL')
        ok = ok and good
        ok = pickup_special_run(out) and ok
        # request 81: the kill feed's mine and freeze icons
        ok = port_fns('2. request 81 weapon icons', ICON_PORT) and ok
        ok = callers_are('2. callers of the request 81 functions', ICON_CALLERS) and ok
        ok = weapon_icon_run(out) and ok

    # 3. car numbers and the extended car select
    if carsel:
        fns = CARSEL_PORT | CARSEL_LOAD_PORT | (NUMBER_PORT if numbers else set())
        ok = port_fns('3. car numbers / car select', fns, allowed_rand=('portCarEnding',)) and ok
        table = {k: set(v) for k, v in NUMBER_CALLERS.items() if k in fns}
        table.update({k: set(v) for k, v in CARSEL_LOAD_CALLERS.items()})
        if profile and 'portCarNumberOfGroup' in table:
            table['portCarNumberOfGroup'] = {'portCarProfileOfSlot'}     # request 70 (and the port's API)
        if numbers and 'TM2_PORT_CARS' in names:
            for k, v in NUMBER_CALLERS_CARS.items():
                table[k] |= v
        if numbers and 'portCarSpecialSound' in kinds:
            table['portCarNumberType'] |= {'portCarSpecialSound'}     # request 108's addendum: a mod's base type
        if numbers:
            table['portCarNumberBase'] = None
            table['portCarNumberMod'] = None
        else:
            # request 36's car select alone: the number functions are not there
            for k in ('portCarSelDraw', 'portCarNumberOfGroup'):
                table.pop(k, None)
            table['portCarSelNumber'] = set()     # screenCarSelect maps through it only with TM2_PORT_CAR_NUMBERS
            table['portCarSelCopy'] = {'portCarSelExtend'}
            table['portCarSelReset'] = {'portCarSelExtend'}
        base = table.pop('portCarNumberBase', 0)
        mod = table.pop('portCarNumberMod', 0)
        ok = callers_are('3. callers of the car number / car select functions', table) and ok
        if numbers:
            bad = sorted(c for c in callers.get('portCarNumberBase', set()) | callers.get('portCarNumberMod', set())
                         if kinds.get(c) != 'port')
            good = not bad and 'portCarNumberBase' not in pointed
            print('3. portCarNumberBase / portCarNumberMod called only by port code (game callers: %s) -> %s' % (
                bad or 'none', 'ok' if good else 'FAIL'))
            ok = ok and good
        for fn in ('portCarSelExtend',) + (('portAiSelExtend',) if numbers else ()):
            if fn in sizes:
                f = Flow(link, sizes[fn][0], sizes[fn][1], tables, names_at)
                n = calls_in(f).get('portCarSelCopy', 0)
            else:
                n = 0
            good = n == 3
            print('3. %s copies %d nodes (the carousel and both name labels) -> %s' % (fn, n, 'ok' if good else 'FAIL'))
            ok = ok and good
    # 5. request 46: the render switches of the far LOD and far distance defines
    if far_lod or far_distance:
        users = {}
        for p_c, rtype, v_c, item, sym in relocs:
            if rtype != R_MIPS_LO16:
                continue
            for name in ('portFarLodNearScale', 'portFarLodNear', 'portMirrorFar'):
                s = link.symbols.get(name)
                if s and s['address'] <= v_c < s['address'] + {'portFarLodNear': 49156}.get(name, 4):
                    fn = next((r[2] for r in rows if r[0] <= p_c < r[0] + r[1]), None)
                    users.setdefault(name, set()).add(fn)
    if far_lod:
        bad = []
        game = sorted(f for f in users.get('portFarLodNearScale', set()) if kinds.get(f) != 'port')
        if game != ['renderScene']:
            bad.append('game functions reading portFarLodNearScale: %s (want renderScene)' % game)
        game = sorted(f for f in users.get('portFarLodNear', set()) if kinds.get(f) != 'port')
        if game:
            bad.append('game functions naming portFarLodNear: %s' % game)
        for fn in ('portFarLodBegin', 'portFarLodEnd'):
            if fn not in users.get('portFarLodNear', set()):
                bad.append('%s does not read portFarLodNear' % fn)
            if fn in kinds and reaches(calls, fn):
                bad.append('%s reaches rand()' % fn)
        good = not bad
        print('5. far LOD near scale (portFarLodNearScale): read by renderScene only, the records by the port -> %s' % (
            'ok' if good else 'FAIL: ' + '; '.join(bad)))
        ok = ok and good
        if defined:
            ok = not check_far_lod_scan(out) and ok     # 14. request 90
    if far_distance:
        bad = []
        if 'portFarDistanceMirror' not in kinds or kinds.get('portFarDistanceMirror') != 'port':
            bad.append('portFarDistanceMirror not in .port_text')
        elif calls.get('portFarDistanceMirror'):
            bad.append('portFarDistanceMirror calls %s' % sorted(calls['portFarDistanceMirror']))
        if callers.get('portFarDistanceMirror', set()) != {'renderScene'}:
            bad.append('portFarDistanceMirror called by %s' % sorted(callers.get('portFarDistanceMirror', set())))
        elif 'renderScene' in sizes:
            fr = Flow(link, sizes['renderScene'][0], sizes['renderScene'][1], tables, names_at)
            n = calls_in(fr).get('portFarDistanceMirror', 0)
            if n != 2:
                bad.append('renderScene calls portFarDistanceMirror %d times (want 2)' % n)
        game = sorted(f for f in users.get('portMirrorFar', set()) if kinds.get(f) != 'port')
        if game:
            bad.append('game functions naming portMirrorFar: %s' % game)
        rs = (ROOT / 'src/semantic/engine_render_scene.c').read_text(encoding='utf-8')
        if '#define RENDER_FAR_MIRROR(retail) (retail)' not in rs or rs.count('RENDER_FAR_MIRROR(500)') != 1 or \
                rs.count('RENDER_FAR_MIRROR(1000)') != 1:
            bad.append("source: the mirror's 500 / 1000 not through RENDER_FAR_MIRROR (retail form (retail))")
        good = not bad
        print('5. mirror far distance (portMirrorFar): renderScene twice through portFarDistanceMirror -> %s' % (
            'ok' if good else 'FAIL: ' + '; '.join(bad)))
        ok = ok and good
        # the render capacity counter
        ok = port_fns('5. render capacity counter', set(STATS_PORT)) and ok
        ok = callers_are('5. callers of the render counter', {fn: {'renderScene'} for fn in STATS_PORT}) and ok
        bad = []
        for fn, allowed in STATS_PORT.items():
            got = set(calls.get(fn, ()))
            if got - allowed:
                bad.append('%s calls %s' % (fn, sorted(got - allowed)))
        if 'renderScene' in sizes:
            fr = Flow(link, sizes['renderScene'][0], sizes['renderScene'][1], tables, names_at)
            at = {}
            for a, c in fr.calls.items():
                at.setdefault(c, []).append(a)
            walked, drawn = at.get('portRenderStatsWalked', []), at.get('portRenderStatsDrawn', [])
            emit, near = at.get('portFarDistanceEmit', []), at.get('renderNearPolys', [])
            walks = at.get('renderWalkModel', [])
            if len(walked) != 1 or len(drawn) != 1:
                bad.append('renderScene calls Walked %d / Drawn %d times (want 1 / 1)' % (len(walked), len(drawn)))
            elif not (emit and near and walks and max(walks) < walked[0] < min(emit) and drawn[0] > max(near)):
                bad.append('renderScene: Walked not between the walks and the emitter, or Drawn not after the near pass')
        else:
            bad.append('renderScene not in the link')
        s = link.symbols.get('portRenderStats')
        if s is None:
            bad.append('portRenderStats not in the link')
        else:
            lo, hi = s['address'], s['address'] + STORAGE['portRenderStats']
            relocs_at = {}
            for p_c, rtype, v_c, item, sym in relocs:
                if rtype in (5, 6, 7):                           # HI16 LO16 GPREL16
                    relocs_at.setdefault(p_c, []).append(v_c)
            for fn in STATS_PORT:
                if fn in sizes:
                    f = Flow(link, sizes[fn][0], sizes[fn][1], tables, names_at)
                    st = stores_outside(f, relocs_at, lo, hi)
                    if st:
                        bad.append('%s stores outside portRenderStats at %s' % (fn, ' '.join('%08X' % a for a in st)))
            users = set()
            for p_c, rtype, v_c, item, sym in relocs:
                if lo <= v_c < hi:
                    fn = next((r[2] for r in rows if r[0] <= p_c < r[0] + r[1]), None)
                    if kinds.get(fn) != 'port':
                        users.add(fn)
            if users:
                bad.append('game functions naming portRenderStats: %s' % sorted(users))
        good = not bad
        print('5. render capacity counter: renderScene after the walks / after the near pass, stores only to '
              'portRenderStats -> %s' % ('ok' if good else 'FAIL: ' + '; '.join(bad)))
        ok = ok and good

    # 6. request 47: the packet buffer sets, the far state's size, the matrix slots
    addrs = sorted({s['address'] for s in link.symbols.values()})

    def room(name):
        s = link.symbols.get(name)
        if s is None:
            return None
        nxt = next((a for a in addrs if a > s['address']), None)
        return None if nxt is None else nxt - s['address']

    n_slots = (limits or {}).get('renderslots', RETAIL_SLOTS)
    if not defined:
        n_slots = 401                           # the negative control: as if built with TM2_PORT_RENDER_SLOTS=400
    limit = n_slots - 1 if n_slots > RETAIL_SLOTS else 200
    bad = []
    want = 16 + 8 * (n_slots + 1) + 64 * (n_slots + 312)
    got = room('portFarState')
    if got is None or not want <= got < want + 16:
        bad.append('portFarState has %s bytes before the next symbol (want %d for %d slots)' % (got, want, n_slots))
    if far_distance:
        if not port_fns('6. packet buffers per view', PACKET_PORT):
            bad.append('the packet buffer functions')
        if not callers_are('6. callers of the packet buffer functions', PACKET_CALLERS):
            bad.append('the packet buffer callers')
        if 'rtMainLoop' in sizes:
            fr = Flow(link, sizes['rtMainLoop'][0], sizes['rtMainLoop'][1], tables, names_at)
            n = calls_in(fr)
            if n.get('portPacketBufferCursor', 0) != 2 or n.get('portPacketBuffersApply', 0) != 1:
                bad.append('rtMainLoop calls portPacketBufferCursor %d times (want 2), portPacketBuffersApply %d (want 1)'
                           % (n.get('portPacketBufferCursor', 0), n.get('portPacketBuffersApply', 0)))
        else:
            bad.append('rtMainLoop not in the link')
        s = link.symbols.get('portPacketBuffers')
        if s is None:
            bad.append('portPacketBuffers not in the link')
        else:
            lo, hi = s['address'], s['address'] + PACKET_BUFFER_BYTES
            named = set()
            for p_c, rtype, v_c, item, sym in relocs:     # (another symbol's address - k, e.g. portCarGroups - 4: not it)
                if rtype in (5, 6, 7) and lo <= v_c < hi and (sym == 'portPacketBuffers' or sym not in link.symbols):
                    named.add(next((r[2] for r in rows if r[0] <= p_c < r[0] + r[1]), None))
            if named != {'portPacketBuffersApplyView'}:
                bad.append('portPacketBuffers named by %s (want portPacketBuffersApplyView only)' % sorted(map(str, named)))
        rm = (ROOT / 'src/semantic/rt_main.c').read_text(encoding='utf-8')
        if '#define RT_PACKET_CURSOR(record, before) ((before)->field_8)' not in rm or \
                rm.count('= RT_PACKET_CURSOR(') != 2 or rm.count('->field_8 = D_80180EE4->field_8') != 0:
            bad.append("source: view 2's and the mirror's cursor not through RT_PACKET_CURSOR (retail form "
                       "((before)->field_8))")
    good = not bad
    print('6. packet buffers (5 views x 2 x 4 x retail, a buffer per view) and portFarState for %d slots -> %s' % (
        n_slots, 'ok' if good else 'FAIL: ' + '; '.join(bad)))
    ok = ok and good
    if render_slots:
        bad = []
        if 'renderWalkModel' in sizes:
            imm = slt_immediates(link, *sizes['renderWalkModel'])
            if imm.get(limit - 2, 0) != 1 or (limit - 2 != 198 and imm.get(198)):
                bad.append('renderWalkModel compares with %s (want %d once, never 198)' % (
                    sorted(imm.items()), limit - 2))
        else:
            bad.append('renderWalkModel not in the link')
        if 'renderScene' in sizes:
            imm = slt_immediates(link, *sizes['renderScene'])
            n = 5 if 'TM2_PORT_VISUAL' in names else 3
            if imm.get(limit, 0) != n or (limit != 200 and imm.get(200)):
                bad.append('renderScene compares with %s (want %d %d times, never 200)' % (sorted(imm.items()), limit, n))
        else:
            bad.append('renderScene not in the link')
        if 'renderInitNodeHandlers' in sizes and 'renderWalkModel' in link.symbols:
            a0, sz = sizes['renderInitNodeHandlers']
            entry = link.symbols['renderWalkModel']['address']
            tgt = sorted({v_c for p_c, rtype, v_c, item, sym in relocs if a0 <= p_c < a0 + sz and rtype in (5, 6)})
            if tgt != [entry]:
                bad.append('renderInitNodeHandlers stores %s (want only renderWalkModel %08X)' % (
                    ' '.join('%08X' % t for t in tgt), entry))
        else:
            bad.append('renderInitNodeHandlers / renderWalkModel not in the link')
        port_lo = int((link.port or {}).get('base', '0'), 16) if link.port else None
        amap = AddrMap.read(out / 'addrmap.tsv')
        for name, (va, eb) in SLOT_ARRAYS.items():
            s = link.symbols.get(name)
            r = room(name)
            if s is None or port_lo is None or s['address'] < port_lo:
                bad.append('%s not in the port storage' % name)
            elif r is None or r < n_slots * eb:
                bad.append('%s has %s bytes (want %d slots of %d)' % (name, r, n_slots, eb))
            # a relocation whose target maps back into the retail storage (addrmap.tsv) but is not the new array
            home = (s['address'], s['address'] + n_slots * eb) if s is not None else (0, 0)
            stale = sorted({p_c for p_c, rtype, v_c, item, sym in relocs if not home[0] <= v_c < home[1] and
                            va <= (amap.c2r(v_c) or 0) < va + RETAIL_SLOTS * eb})
            if stale:
                bad.append('%d relocations still point into the retail %s (first at %08X)' % (len(stale), name, stale[0]))
        pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
        changed = {s['symbol'] for s in pv.get('changed_sections', []) if s['code']}
        for fn in ('sub_800CDC54', 'renderNearPolys'):
            # the C emitter (request 61 addendum) and the C near pass (request 82) with TM2_PORT_C_EMITTER: check 8
            if fn in changed and 'TM2_PORT_C_EMITTER' not in names:
                bad.append('%s changed (the emitter and the near pass stay retail\'s)' % fn)
        if far_distance and 'portRenderStatsWalked' in sizes:
            imm = slt_immediates(link, *sizes['portRenderStatsWalked'])
            if not imm.get(limit - 2) or not imm.get(limit):
                bad.append('portRenderStatsWalked compares with %s (want %d and %d)' % (sorted(imm.items()), limit - 2, limit))
        if expect:
            pc = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
            alone = names - {'TM2_PORT_VARIANT'} == {'TM2_PORT_RENDER_SLOTS'}
            if not RENDER_SLOTS_CHANGED <= pc or (alone and pc != RENDER_SLOTS_CHANGED):
                bad.append('the define changes %s (want %s%s)' % (sorted(pc), sorted(RENDER_SLOTS_CHANGED),
                                                                    ' exactly' if alone else ' among them'))
        good = not bad
        print('6. matrix slots (limit %d, %d slots a view): the C walker, the five arrays in .port_bss -> %s' % (
            limit, n_slots, 'ok' if good else 'FAIL'))
        for b in bad[:12]:
            print('     ' + b)
        ok = ok and good

    # 7. request 61: the GTE range pass's sprites, beams and translations
    if far_distance:
        ok = not check_far_sprites(out, link, sizes, names_at, rows, kinds, relocs) and ok

    # 18. request 98: the far-LOD budget
    if far_distance and 'TM2_PORT_FAR_LOD' in names:
        ok = not check_far_lod_budget(out) and ok

    # 8. request 61 addendum: the C emitter against the asm, its precise corners
    if c_emitter:
        ok = not check_c_emitter(out, link, sizes, e0, names, defined) and ok
        ok = check_precise_cull(out) and ok                 # 27. request 116

    # 9. request 63: the chase camera's eye kept inside the level (portVisual bit 5)
    if visual:
        ok = not check_camera_wall(out, link, sizes, kinds, callers, calls, pointed, relocs, rows, defined) and ok

    # 10. request 71: car select's in-game models (portCarSelLoad) on the disc's car files
    if carsel:
        ok = not check_carsel_load(out, 'TM2_PORT_TEXTURE_BANKS' in names) and ok

    # 11. request 70: a mod car's own numbers
    if profile:
        ok = port_fns('11. mod car profiles', PROFILE_PORT) and ok
        table = dict(PROFILE_CALLERS)
        if 'TM2_PORT_SPECIAL_PROFILE' in names:     # request 73: the special profile's lookups
            table['portCarProfileOfSlot'] = PROFILE_CALLERS['portCarProfileOfSlot'] | {'portSpecialOf', 'portSpecialAmmo'}
        ok = callers_are('11. callers of the profile functions', table) and ok
        ok = not check_profile(out) and ok

    # 13. request 73: a mod car's special
    if special:
        # the pickup templates fire through retail's own pickup code (the launcher, the drop), which reaches rand() as
        # a pickup's fire does; none of the port functions calls it itself
        ok = port_fns('13. mod car specials', SPECIAL_PORT, allowed_rand=('portSpecialFirePickup', 'portSpecialBomb',
                                                                           'portSpecialRicochet')) and ok
        ok = callers_are('13. callers of the special-profile functions', SPECIAL_CALLERS) and ok
        ok = not check_special_profile(out) and ok

    # 12. request 72: the hidden areas (portVisual bit 6)
    if hidden:
        ok = port_fns('12. hidden areas', HIDDEN_PORT) and ok
        ok = callers_are('12. callers of the hidden-area functions', HIDDEN_CALLERS) and ok
        if 'renderScene' in sizes:
            fr = Flow(link, sizes['renderScene'][0], sizes['renderScene'][1], tables, names_at)
            at = lambda fn: sorted(a for a, c in fr.calls.items() if c == fn)
            beg, end, walks = at('portHiddenAreasBegin'), at('portHiddenAreasEnd'), at('renderWalkModel')
            emit = at('portFarDistanceEmit') + at('sub_800CDC54')
            drawn, near = at('portHiddenAreasDrawn'), at('renderNearPolys')
            good = bool(len(beg) == 1 and len(end) == 1 and walks and emit and beg[0] < walks[0] and
                        walks[-1] < end[0] < min(emit) and len(drawn) == 1 and near and
                        drawn[0] > max(emit) and drawn[0] > max(near))
            print('12. renderScene: Begin %d call(s) before the first walk, End %d after the last and before the '
                  'emitter, Drawn %d after the emitter and the near pass -> %s' % (
                      len(beg), len(end), len(drawn), 'ok' if good else 'FAIL'))
        else:
            good = False
            print('12. renderScene not in the link -> FAIL')
        ok = ok and good
        ok = not check_hidden_areas(out) and ok

    if fall:
        ok = check_fall_wheels(out) and ok                  # 20. request 107
    if pose:
        ok = check_car_pose(out) and ok                     # 21. request 108
        ok = check_special_sound(out) and ok                # 22. request 108's addendum
    if whole:
        ok = check_whole_maps(out) and ok                   # 23. request 111
    if fwy_ground:
        ok = check_fwy_ground(out) and ok                   # 24. request 112
    if hidden:
        ok = check_two_sided(out) and ok                    # 25. request 113
    if mod_arena:
        ok = check_mod_arena(out) and ok                    # 26. request 114
        ok = callers_are('26. callers of portArenaHealSpot', {'portArenaHealSpot': {'humanCarTick'}}) and ok
    if wreck:
        # the effects and carSpinOut draw on the game's rand() as carDestroyed's do; neither function calls it itself
        ok = port_fns('28. wreck fall', ('portWreckFall', 'portWreckFallBlown'), allowed_rand=('portWreckFall',)) and ok
        ok = callers_are('28. callers of the wreck-fall functions', {'portWreckFall': {'humanCarTick', 'aiCarDrive'},
                                                                     'portWreckFallBlown': {'carDestroyed'}}) and ok
        ok = check_wreck_fall(out, 'TM2_PORT_FALL_WHEELS' in names or not defined,     # 28. request 117
                              'TM2_PORT_HUMANS' in names or not defined) and ok
    if dt_player:
        ok = port_fns('29. phase-2 Dark Tooth', ('portCarDtPlayerStart',)) and ok
        want = {'humanCarPlaceAtStart'} | ({'portCarApplyType'} if 'portCarApplyType' in kinds else set())
        ok = callers_are('29. callers of portCarDtPlayerStart', {'portCarDtPlayerStart': want}) and ok
        ok = check_dt_phase2(out) and ok                    # 29. request 118
    if glacier_fire:
        ok = port_fns('30. Antarctica fire missile', ('portGlacierFire',)) and ok
        ok = callers_are('30. callers of portGlacierFire', {'portGlacierFire': {'worldLoadStartObjects'}}) and ok
        ok = check_glacier_fire(out) and ok                 # 30. request 120
    if dt_swoop:
        ok = port_fns('31. Dark Tooth swoop', ('portDtSwoopPick', 'portDtSwoopPos', 'portDtSwoopLive',
                                                 'portDtSwoopHit'), allowed_rand=('portDtSwoopHit',)) and ok
        ok = callers_are('31. callers of the swoop functions', {n: {'aiDarkToothBossUpdate'} for n in (
            'portDtSwoopPick', 'portDtSwoopPos', 'portDtSwoopLive', 'portDtSwoopHit')}) and ok
        ok = check_dt_swoop(out) and ok                     # 31. request 121
    if team_locks:
        ok = port_fns('32. team locks', ('portTeamMates', 'portTeamNearestFoe')) and ok
        ok = callers_are('32. callers of the team functions', {
            'portTeamMates': {'weaponHomingLeapInit', 'weaponTaserTarget', 'portTeamNearestFoe'},
            'portTeamNearestFoe': {'aiGetTargetCar'} | ({'portDtSwoopPick'} if dt_swoop else set())}) and ok
        ok = check_team_locks(out) and ok                   # 32. request 123
    if human_wreck:
        ok = port_fns('33. human wreck', ('portHumanWreckStart', 'portHumanWreckTick', 'portHumanWreckBurns',
                                          'portHumanWreckBreak'),
                      allowed_rand=('portHumanWreckBreak',)) and ok     # the break-up's debris and blast: the game's
        ok = callers_are('33. callers of the wreck functions', {'portHumanWreckStart': {'humanCarDamageUpdate'},
                                                                'portHumanWreckTick': {'humanCarDamageUpdate'},
                                                                'portHumanWreckBurns': {'carWreckUpdate'},
                                                                'portHumanWreckBreak': {'carWreckUpdate'}}) and ok
        ok = check_human_wreck(out) and ok                  # 33. request 126

    bad = []
    for name, size in STORAGE.items():
        s = link.symbols.get(name)
        if s is None:
            bad.append('%s missing' % name)
            continue
        nxt = next((a for a in addrs if a > s['address']), None)     # the room up to the next symbol
        if nxt is not None and nxt - s['address'] < size:
            bad.append('%s has %d bytes before the next symbol (want %d)' % (name, nxt - s['address'], size))
    good = not bad
    print('storage: %d words / tables of requests 42-47 at their sizes -> %s' % (len(STORAGE), 'ok' if good else 'FAIL: ' + '; '.join(bad)))
    ok = ok and good

    # 4. the changed game functions
    if expect:
        pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
        changed = {s['symbol'] for s in pv.get('changed_sections', []) if s.get('port_changed') and s['code']}
        want = (EXPECTED_VISUAL if 'TM2_PORT_VISUAL' in names else set())
        want |= (EXPECTED_NUMBERS if 'TM2_PORT_CAR_NUMBERS' in names else set())
        want |= (EXPECTED_NUMBERS_CARS if {'TM2_PORT_CAR_NUMBERS', 'TM2_PORT_CARS'} <= names else set())
        want |= (EXPECTED_PROFILE if 'TM2_PORT_CAR_PROFILE' in names else set())
        want |= (EXPECTED_SPECIAL if 'TM2_PORT_SPECIAL_PROFILE' in names else set())
        alone = names & OUR_DEFINES and not (names - OUR_DEFINES - {'TM2_PORT_VARIANT'}) and len(names & OUR_DEFINES) == 1
        missing = sorted(want - changed)
        extra = sorted(changed - want) if alone else []
        good = not missing and not extra
        print('4. changed game functions: %d; request 42 lists %d (missing %s%s) -> %s' % (
            len(changed), len(want), missing or 'none', '; extra %s' % (extra or 'none') if alone else '',
            'ok' if good else 'FAIL'))
        ok = ok and good
    if not defined:
        print('negative control: %s' % ('FAIL as expected' if not ok else 'passed: the check is blind'))
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out', help='port variant link directory (build_source_only.py --output)')
    ap.add_argument('--expect-functions', action='store_true',
                    help='also compare the functions the port defines changed with the request 42 lists')
    ap.add_argument('--e0', help='the retail relocatable link (default: the latest full build\'s source_reloc_e0)')
    ap.add_argument('--negative-control', action='store_true',
                    help='run checks 1-3 on a build without the defines (they must fail)')
    a = ap.parse_args()
    out = Path(a.out) if Path(a.out).is_absolute() else ROOT / a.out
    pv = json.loads((out / 'report.json').read_text(encoding='utf-8')).get('port_variant') or {}
    names = {d.split('=')[0] for d in pv.get('port_defines') or []}
    if a.e0:
        e0 = Path(a.e0) if Path(a.e0).is_absolute() else ROOT / a.e0
    else:
        builds = sorted(p for p in (ROOT / 'work').glob('build-*') if (p / 'source_reloc_e0' / 'report.json').exists())
        e0 = builds[-1] / 'source_reloc_e0' if builds else None
    relevant = names & (OUR_DEFINES | RENDER_DEFINES | {'TM2_PORT_DARK_TOOTH', 'TM2_PORT_DT_PLAYER',
                                                        'TM2_PORT_DT_SWOOP', 'TM2_PORT_TEAM_LOCKS',
                                                        'TM2_PORT_HUMAN_WRECK'})
    if not relevant and not a.negative_control:
        print('requests 42 / 46 / 47: no TM2_PORT_DARK_TOOTH / _DT_PLAYER / _VISUAL / _CAR_NUMBERS / _FAR_LOD / '
              '_FAR_DISTANCE / _RENDER_SLOTS in %s: nothing to check' % a.out)
        print('CHECK CUSTOM OK')
        sys.exit(0)
    if a.negative_control and relevant:
        sys.exit('check_custom: --negative-control wants a build without the request 42 / 46 / 47 defines')
    ok = check(out, names, e0, defined=not a.negative_control, expect=a.expect_functions and not a.negative_control,
               limits=pv.get('limits'))
    if a.negative_control:
        ok = not ok
    print('CHECK CUSTOM %s' % ('OK' if ok else 'FAILED'))
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
