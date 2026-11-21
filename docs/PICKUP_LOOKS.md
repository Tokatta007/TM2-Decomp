# What a pickup looks like on the map, and how to change it

Answers item 15 of "Next, in this order" in the port's `docs/DECOMP_REQUESTS.md`
(a Challenge Match game type turns weapons off; their pickups then give a
weapon that is on, through the port's `vp_pickup_id`, but on the map they
still show the old weapon). Static analysis only: the C in `src/semantic`,
checked against the retail disassembly (`bin/SCUS_94306R.EXE`), and the
level files read straight from the disc (`cdimage/`, `DMD\*.DMD`), walked
with the same opcodes as `dbscanHierarchy` (8014ABF8). Nothing was run.
Addresses are retail.

Tags: **CONFIRMED** = seen in code or data. **LIKELY** = follows from the
code, not traced to the end or not seen running.

Pickup ids (`GAME_EVENTS.md` 3.2 and 13.1):

| Id | Pickup | `pickupApply` does |
|---|---|---|
| 401 | Fire missile | weapon 1 + ammo |
| 403 | Homing missile | weapon 2 + ammo |
| 404 | Power missile | weapon 4 + ammo |
| 406 | Remote bomb | weapon 3 + ammo |
| 411 | Napalm | weapon 5 + ammo |
| 413 | Ricochet | weapon 6 + ammo |
| 419 | Lightning | weapon 7 + ammo |
| 417 | full health | HP = max (only if HP > 0) |
| 418 | one third health | HP += max / 3, capped (only if HP > 0) |
| 430 | turbo | weapons +0x32 += 100, capped at 999 |
| 441..480 | the same + 40 | the same, but never taken off the map |

## Short answer

- **The look is data, not code.** Each pickup on the map is one DMD node
  (kind 1 "group" or kind 4 "offset") whose id (+2) is the pickup id. The
  node has exactly one child: the pickup's 3D model (a small subtree with a
  mesh, a spinning animation and a hit box). **All pickups of one kind on a
  level share the same model subtree** (one pointer, many parents). No code
  picks a model by kind: the renderer just draws the child. CONFIRMED.
- **The act comes from the same node.** The car's hit test (`hdpTraverse`,
  8014FEEC) tags a hit with the node's id (+2) and its slot index (+22 or
  +16); the car contact code passes those to `pickupApply(car, human, id,
  slot)` (8010AFC8). `pickupSlots[slot].kind` is used only by the respawn
  timer. CONFIRMED.
- **"Visible" is the child count.** `pickupSetVisible` (8012493C) writes 0
  (taken) or 1 (back) into the node's child count (kind 1: +20, kind 4:
  +18). With 0 children the node is neither drawn nor hit. CONFIRMED.
- **To make a pickup look and act as kind B:** write B into node +2, point
  the node's child (kind 1: +24, kind 4: +20) at the shared model of a kind-B
  pickup on the same level, and write B into `pickupSlots[slot].kind`
  (80183F7C + 16 * slot). Do it after the level's objects are loaded
  (section 6.1), and again for a slot while it is hidden (after it is taken)
  to re-roll it for its next respawn (section 6.2).
- **Health and turbo** are pickups like the weapons (417/418/430); 417 and
  418 share one model on every level. Turbo has no model on New York
  (ROOF, ROOF2), remote bomb none on Moscow and Suicide Swamp, lightning only
  on Los Angeles, Moscow and Hong Kong (section 7).
- **Destroyed cars drop nothing** in retail. No code makes a pickup at run
  time: `pickupRegister` has one caller, the level loader. What exists are
  pickups **hidden inside destructible objects** (targets 500..520, 4000,
  4005), which appear when the object is blown up (section 5). A port drop
  needs new nodes (section 8). CONFIRMED.

## 1. The pickup objects in the level file (CONFIRMED)

Every pickup is a node of the level DMD (loaded to 0x80019C40,
`SANTA.md` 1.2). Layouts used by the loader, the renderer
(`reference/renderer_walker_engine_render_scene.c`) and the hit walker
(`hdpTraverse`, `engine_hit_detection.c`):

| Offset | Kind 1 "group" (bounding sphere) | Kind 4 "offset" (position) |
|---|---|---|
| +0x00 | 1 | 4 |
| +0x02 | **id** (u16): 401..430, 441..480 | **id** |
| +0x04 | sphere centre x, y, z (s32, local) | **position** x, y, z (s32, added to the parent's) |
| +0x10 | sphere radius (s32) | **slot index** (u16, written by the loader) |
| +0x12 | | **child count** (u8) = visible flag |
| +0x13 | | flags (bit 1: renderer skips, bit 0: hit walker skips) |
| +0x14 | **child count** (u8) = visible flag | **child pointer[0]** = the model |
| +0x15 | flags (bit 1: renderer skips, bit 2: no cull, bit 0: hit walker skips) | |
| +0x16 | **slot index** (u16, written by the loader) | |
| +0x18 | **child pointer[0]** = the model | |

A kind-1 pickup sits under a kind-4 node (id 0) that gives its position;
its sphere is about (0, 0, 52..59), radius 86..91. A kind-4 pickup carries
its own position. Both kinds' models are drawn at the pickup's position, so
a model taken from either kind works under either kind. LIKELY (all models
are local: their root is a kind-1 group with id 0).

Every pickup node on the disc has exactly one child. The model subtree is
`1:0( mesh, anim 3:0 (2 frames), hit box 8:0 )` (health:
`1:0( anim, mesh, hit box )`), one per kind per level:

| Level (DMD) | Pickups | Models (kind: pointer, number of pickups using it) |
|---|---|---|
| Los Angeles (FREEWAY) | 36 | 401 8003F280 x5, 403 8003F0E0 x7, 404 80043FA4 x2, 406 8003EF3C x3, 411 8003F44C x5, 413 80042AA0 x2, **417 + 418 8003F620** x1 + x4, 419 800424D0 x3, 430 8003EC04 x4 |
| Moscow (DISH) | 14 | 401 800755E8 x2, 403 80028A40 x3, 404 8002C4CC x1, 411 800752F8 x2, 413 800757CC x1, 418 8007518C x2, 419 80075464 x1, 430 80026CF8 x2 |
| Paris (PARIS) | 42 | 401 8002B51C x8, 403 8002E924 x9, 404 80030270 x5, 406 800306EC x3, 411 80030E38 x5, 413 8002F414 x5, 418 8002EAC8 x4, 430 8003E428 x3 |
| Paris 2P (PARIS2) | 42 | 401 8002B744, 403 8002EADC, 404 80030428, 406 800308A4, 411 80030FF0, 413 8002F5CC, 418 8002EC80, 430 8003E180 |
| Amazonia (ISLANDS) | 36 | 401 8006B760, 403 8006B388, 404 8006B51C, 406 8006B1F4, 411 8006A59C, 413 8006B9A4, 417 + 418 8006A0E8, 430 80069B3C |
| New York (ROOF) | 30 | 401 8007FE20, 403 80062884, 404 80078044, 406 800801A4, 411 800629F0, 413 8007FF8C, 417 + 418 80077A50; **no turbo** |
| New York 2P (ROOF2) | 23 | 401 80077218, 403 800770AC, 404 8006F36C, 406 800776F0, 411 800774F0, 413 80077384, 418 8006ED78; **no turbo** |
| Antarctica (GLACIER) | 37 | 401 80026B2C, 403 80027350, 404 80030FEC, 406 8003EFA8, 411 8003117C, 413 8002B440, 417 + 418 80030E1C, 430 8002B5DC |
| Antarctica 2P (GLACIER2) | 30 | 401 800250B4, 403 800258D8, 404 8002F258, 406 8003A904, 411 8002F3E8, 413 800295D4, 417 + 418 8002F088, 430 800297DC |
| Holland (DEN) | 25 | 401 8003B5C8, 403 8003B060, 404 8003B3DC, 406 80056D00, 411 80047214, 413 80052ED0, 417 + 418 8003B238, 430 80056B78 |
| Hong Kong (HKONG) | 47 | 401 800535AC, 403 80053734, 404 800540A4, 406 800532D4, 411 80053440, 413 80054678, 417 + 418 80053164, 419 800538B8, 430 80053A3C |
| Suicide Swamp (SWAMP) | 12 | 401 80063A28, 403 80067018, 404 80064540, 411 800642AC, 413 80062EDC, 418 80066E90, 430 80064014 |
| Cyburbia (BURB) | 48 | 401 8003E858, 403 80053B20, 404 800374E0 (x4) and **80067BC8 (x1: a hit box only, an invisible pickup)**, 406 800542CC, 411 8003E4E0, 413 8003BFE8, 417 80084540, 430 8003734C |
| Roof Tops (SROOF) | 18 | 401 80057B38, 403 800372BC, 404 80058C30, 406 8005839C, 411 800448A8, 413 80037744, 417 + 418 800581F8, 430 80036DC0 |

The pointers are RAM addresses once the DMD is at 0x80019C40 (the file's
pointers are built for that address). The table is from the disc files;
read them at run time from the nodes (section 6.1) rather than hard-coding
them. Counts are 401..430 nodes; `SECRET_LEVELS.md` 3 has the per-kind split
for Swamp, Cyburbia and Roof Tops (its Swamp total, 13, should be 12).

The +40 pickups (441..480) are only on Los Angeles (441, 444, 453, 459) and
Moscow (443, 444, 446, 451, 457, 459). They hang under an animation node with
id 440 that steps through them (frame 0 is empty): a rotating "bonus" spot.
The loader has no case for 440 or 441..480, so they are not registered,
never hidden and never respawned. On Moscow's first 440 carrier the hit box
is beside the 44x node, not under it, so touching it gives id 440, which
`pickupApply` ignores (jingle only). What drives the 440 animation (its
node +18 is 2: neither the loader nor the renderer advances it) was not
traced. LIKELY.

## 2. Loading: `worldLoadObject` and `pickupRegister` (CONFIRMED)

`sessionLoadArenaAndCars` (8012BAFC, `shell_main.c`) reads the level DMD
into RAM, then `arenaResetSystems` (80128CE4, `sound_manager_reap.c`) runs
`objRegistriesReset` (clears `pickupObjects`), `pickupSlotsReset` (8010AE04)
and `worldLoadStartObjects` (800D1C70, `rt_main.c`). That walks each root
with `dbscanHierarchy` (8014ABF8, `engine_clut_anim.c`), which calls
`worldLoadObject(id, node, pos)` (800D1DB0, `world_load_object.c`) for every
node with a non-zero id; `id` is the node's u16 +2 and `pos` the summed
offsets down to it (a kind-4 node includes its own).

For ids 401, 403, 404, 406, 411, 413, 417, 418, 419, 430:

1. Mode 4 and id 418: the node's child count is set to 0 (hidden for the
   whole match) and nothing else; it gets no slot.
2. Otherwise: slot = `worldLoadPickupIndex` (80180EF8, starts at 0); slot is
   written into the node (kind 1: +22, kind 4: +16); `pickupRegister(pos,
   id, slot)` (8010AE58, `ai_route_helpers.c`); `worldLoadPickupIndex`++;
   `pickupRegisterObject(node, id)` (801248C8, `car_switch_registry.c`).

So slot n of `pickupSlots` and `pickupObjects[n]` are the same pickup, in
the order the walk meets them. The DMD in RAM is a fresh copy for each match
(the file is read again by `sessionLoadArenaAndCars`); a round restart (a
human's lost life) does not reload it or reset pickups (`COOP_LIVES.md`).

### 2.1 `pickupSlots` (80183F7C, 16 bytes each)

| Offset | Field | Written by | Read by |
|---|---|---|---|
| +0x0 | **kind** (s16, the id) | `pickupRegister` | `pickupRespawnTick` only (health test, `pickupSetVisible` id) |
| +0x2 | **active** (s16): 1 registered or back, 0 taken | `pickupRegister` (1), `pickupRespawnTick` (1), `pickupApply` (0), `pickupSlotsReset` (0) | `pickupRegister` (refuses a used slot) |
| +0x4 | x, y, z (s32) | `pickupRegister` | **nobody** |

Found by searching the disassembly for every 80183F7C..80183F88 operand
(and the gp range): 9 instructions, all in 8010AE04..8010B4E4. So **the AI
does not look for pickups through this table** (`SECRET_LEVELS.md` 3 says it
does; it does not): computer cars collect pickups only by driving over them.
`pickupObjects` (8018F6BC) likewise has only its three users
(`pickupRegisterObject`, `pickupSetVisible`, `objRegistriesReset`).

The three C files that use the table declare it three ways (`TM2Mark` in
`ai_route_helpers.c`, `Pick16` in `fx_sprite_spawn.c`, `TM2PickupSlot` in
`pickup_apply.c`); the layout above is the real one.

**Size:** `pickupSlotsReset` clears 50 slots, and the ammo table
`pickupAmmoTable` (8018429C) starts right after slot 49 (80183F7C + 50 * 16).
`pickupRegister` accepts slot < 51, so a 51st pickup would write over the
ammo table; `pickupObjects` has 50 entries (`pickupRegisterObject` checks
< 50). Retail's largest level has 48 (Cyburbia). A port that adds pickups
must stop at 50.

### 2.2 `pickupObjects` and the "pickup display objects" (`car_switch_registry.c`)

`pickupObjects[50]` (8018F6BC) holds the node pointer of each slot.
`pickupRegisterObject` stores it at the index read back from the node
(+22 or +16). `pickupSetVisible(id, slot, value)` (8012493C) does, for
id 401..430 only: kind 1 -> `pickupModelSetVisible` (801249D8: node +20 =
value), kind 4 -> `pickupSpriteSetVisible` (801249EC: node +18 = value).

The names are misleading: neither is a sprite. +20 and +18 are the child
counts of a kind-1 and a kind-4 node; both kinds hold the same 3D models.
Better names would be "pickupGroupSetChildCount" and
"pickupOffsetSetChildCount". `pickupSetVisible` also reads the node's kind
before it checks slot < 50 and never checks for a null entry.

## 3. Drawing and hitting (CONFIRMED)

**Drawing:** the level model is the kind-1 root (id 1) set by
`worldLoadObject` case 1. The renderer walks it every frame: a kind-1 node
is culled by its sphere, then its `+20` children are pushed; a kind-4 node
adds its +4 position and pushes its `+18` children. The pickup's model
subtree is drawn like any other part of the level. Its animation node (kind
3, node +18 = 0) is advanced by `animUpdateOne` (801518E8), which advances a
node once per frame buffer however many parents draw it, so sharing one model
among many pickups is safe. LIKELY (from its name and notes; not
re-checked).

**Hitting:** the car's contact probes go through the hit walker (the car
pass `hdpPassRun` 8014FAB0 uses the hand-written walker; `hdpTraverse`
8014FEEC has the same node rules in C):
- kind 1: when the probe point is inside the sphere and the flags allow it,
  the ids of the hit become (node +2, node +22) through `hdpSetNodeIds`
  (8014FE0C), and the children are pushed with them;
- kind 4: (node +2, node +16), position subtracted, children pushed;
- the model's hit box (kind 8, id 0) is tested by the hand-written leaf test
  at 8014F168; with its own id 0 it keeps the inherited pair and stores
  them in the contact probe: probe +0x1C (`surface`) = slot index, probe
  +0x1E (`material`) = pickup id (8014F638..8014F648).

`carContactApplyObject` (800E06CC, `car_object_contact.c`) then, for
material 401..480: clears the probe and calls `pickupApply(car, human,
material, surface)` at 800E081C.

So the hit box travels with the model: give a pickup another kind's model
and it also gets that model's hit box. Projectiles and bullets fly through
401..440 (`projectile_hit.c`, `weapon_bullets.c`, `weapon_ricochet.c`), so a
remapped kind inside that range changes nothing for shots.

## 4. Collecting and coming back (CONFIRMED)

**`pickupApply` (8010AFC8, `pickup_apply.c`)**, entry a0 = car record, a1 =
1 human / 0 AI, a2 = id, a3 = slot:
- id >= 441: "keep" (not taken off the map), id -= 40.
- A **human** with a full weapon bay and a weapon id: "full" message, the
  pickup stays (not hidden). Health and turbo are always taken. AI cars
  always take it.
- The effect by id (the table at the top), the HUD message (humans), the
  jingle (sound 24).
- Not "keep": `pickupSetVisible(id, slot, 0)` (8010B4D0) and slot active = 0
  (8010B4E4).

**`pickupRespawnTick` (8010AED0, `fx_sprite_spawn.c`)**, called once per
frame by `worldUpdate` (8011B63C): counts frames to `pickupRespawnPeriod`
(80180AE8, 200 in retail; the port sets it per game type), then picks
**n = rand() % pickupRegisteredCount** (80180AEC). If `pickupSlots[n].kind`
is 417 or 418 it goes on only half the time (rand() < 16384). Then slot n
active = 1 and **`pickupSetVisible(kind, n, 1)` at 8010AFA8**. It does not
look at whether slot n was taken: a pick of a pickup still on the map is
wasted. Request 67 (`BALANCE.md` 16): with `portBalance` bit 23 the same one draw picks among the
pickups that are taken (model hidden, slot active 0) and can show (not behind a select a target or level event
keeps whole, section 5): `rand() % that count`; health keeps its half chance; none: nothing comes back.

`pickupSetVisible` has three callers, and only this one passes value 1:

| Call | Value | Who |
|---|---|---|
| 8010AFA8 | **1** | `pickupRespawnTick`, slot in a1 |
| 8010B4D0 | 0 | `pickupApply` |
| 80125260 | 0 | `targetDamage` (ids 500..520: out of range, does nothing) |

So **`pickupSetVisible` with a2 = 1 is exactly "slot a1 comes back"**.

## 5. Health, turbo, and "drops" (CONFIRMED)

- **Health and turbo** are ordinary pickup nodes (417, 418, 430), loaded,
  hidden and respawned like the weapons. 417 and 418 use the same model on
  every level that has both; only the effect differs. In mode 4 the 418s are
  hidden at load and get no slot (section 2).
- **Destroyed cars drop nothing.** `carDestroyed` (8011BE10,
  `car_destroyed.c`) makes sound, smoke and the wreck timer; "drop its held
  pickup" in its comment and in `names.tsv` means Mr. Slam letting go of a
  held car. `aiCarDamageUpdate` and `carApplyDamage` make no pickup either
  (`AI_LIVES.md` 1.1). `weaponDrops` (80184B04) are mines and remote bombs.
  `pickupRegister` has one caller (800D2DE8, the loader) and
  `pickupRegisterObject` one (800D2E00).
- **Pickups hidden in destructible objects.** Many pickups are inside the
  "destroyed" state of a select node (kind 9) whose id is a target
  (500..520) or a level event (4000 Moscow, 4005 Los Angeles). The select
  node draws and hits only its current child (+7); the object's code switches
  it when the object is blown up (`targetDamage` 80125118 for targets, the
  level-event code for 4000 / 4005), and the pickup appears (LIKELY for each
  object type; the tree shape is CONFIRMED). They are
  registered at load like the others (they count for the respawn pick even
  while their object is whole). Examples: Antarctica 5 under target 502 and 1
  under 504, Paris 10 under 502 and 6 under 503, Roof Tops 14 of its 18 under
  502, Holland 6 under 510 (state 0: shown until the event) and 5 under 501,
  Cyburbia 5 under 502. Changing their kind works the same way.

## 6. Making a pickup of kind A look and act as kind B

The port's hooks wrap whole functions (`F_<VA>_orig`), so the hook points
below are function entries and returns.

### 6.1 At load (all pickups of a match)

**Where:** after `worldLoadStartObjects` (800D1C70) returns, that is after
the original `arenaResetSystems` (80128CE4) or `sessionLoadArenaAndCars`
(8012BAFC) returns. At that moment every model of the level is known.
Doing it inside `worldLoadObject` is too early: the model of kind B may come
later in the walk.

**Steps:**
1. Build the model table: for n = 0..49, node = `pickupObjects[n]`
   (8018F6BC + 4n); skip null. kind = node[0]; id = u16 node+2; child =
   u32 at node+24 (kind 1) or node+20 (kind 4). Keep the first child per id
   whose first byte is not 8 (skips Cyburbia's hit-box-only pickup). 417 and
   418 may fill each other's entry.
2. For each slot n to change from A to B, when the table has B:
   - `*(u16 *)(node + 2) = B` (the act: the hit returns it);
   - child pointer (kind 1: node+24, kind 4: node+20) = model of B (the look
     and the hit box);
   - `*(s16 *)(0x80183F7C + 16n) = B` (the respawn: health test and the
     `pickupSetVisible` range check).
   Leave the child count (+20 / +18) alone: it is the visible state.
3. When B has no model on this level: either keep the old look (and let
   `vp_pickup_id` still change the act), or take the model of another kind
   that is on (closest look), or hide the pickup (section 6.3).

Keep B within 401..430: `pickupSetVisible` ignores other ids (the pickup
would never hide or come back). To make one permanent (never taken), use
B + 40 in node +2 only (the slot kind stays B).

**Mode 4's 418s** get no slot and stay hidden. To keep them as another kind,
hook `worldLoadObject` (800D1DB0) entry: when a0 = 418 and the game mode is
4, write the new id to node +2 and to a0 before calling the original (it
then registers it as that kind); then swap its model in the step above.

### 6.2 At respawn (a new kind each time it comes back)

Two equivalent places; use one:
- **On collection (simplest):** after the original `pickupApply`
  (8010AFC8) returns, if node `pickupObjects[a3]` has child count 0 (it was
  really taken), re-roll slot a3 as in 6.1 step 2. It is hidden, so the
  change is not seen; it comes back as the new kind. Note the "full bay"
  case leaves the pickup on the map (count still 1): do not change it then.
- **On respawn:** hook `pickupSetVisible` (8012493C) entry; when a2 = 1,
  a1 is the slot coming back: re-roll it (6.1 step 2) and set a0 = the new
  kind before calling the original. Only the respawn passes 1 (section 4).
  The 417/418 "half as often" test has already used the old kind.

With either, `vp_pickup_id` can stay as a fallback for the port's own ids
(400 special, 402 nothing), but for kinds remapped this way the id arriving
at `pickupApply` is already the new one.

### 6.3 Turning a kind off completely

Hide it: child count 0 (kind 1 +20, kind 4 +18) and slot kind = 0 (outside
401..430, so the respawn's `pickupSetVisible` does nothing). The respawn
still draws the slot in `rand() % pickupRegisteredCount` and wastes that
pick, so pickups come back more slowly; to keep the rate, scale
`pickupRespawnPeriod` by (registered - hidden) / registered, or remap the
pickup to a kind that is on instead (6.1).

### 6.4 Fields, in one place

| What | Address / offset | Effect |
|---|---|---|
| node id | node +0x02 (u16) | what the car gets (`pickupApply` a2) |
| node model | kind 1 +0x18, kind 4 +0x14 (pointer) | what is drawn and what is hit |
| node visible | kind 1 +0x14, kind 4 +0x12 (u8 child count, 0 or 1) | taken / on the map |
| node slot | kind 1 +0x16, kind 4 +0x10 (u16) | `pickupApply` a3 |
| node position | kind 4 +0x04..+0x0C; kind 1: its parent's | where |
| slot kind | 80183F7C + 16n, s16 | respawn: health halving, `pickupSetVisible` id |
| slot active | 80183F7E + 16n, s16 | bookkeeping only |
| node of slot n | `pickupObjects` 8018F6BC + 4n | |
| slot count | `pickupRegisteredCount` 80180AEC | respawn pick range |
| respawn period | `pickupRespawnPeriod` 80180AE8 (200) | frames between picks |
| ammo per pickup | `pickupAmmoTable` 8018429C + 4 * weapon | |

## 7. Which looks each level has (CONFIRMED, from the disc)

| Level | Missing models |
|---|---|
| Los Angeles, Hong Kong | none |
| Moscow | remote bomb 406 (health only as 418's model) |
| Paris, Paris 2P | lightning 419 |
| Amazonia, Antarctica (both), Holland, Roof Tops | lightning 419 |
| New York | turbo 430, lightning 419 |
| New York 2P | turbo 430, lightning 419 (health only as 418's model) |
| Suicide Swamp | remote bomb 406, lightning 419 |
| Cyburbia | lightning 419 (health only as 417's model) |

Every level has a health model. Lightning (weapon 7) only works on levels
1, 2 and 8 (`GAME_EVENTS.md` 3.2), the three levels that have its model.

## 8. Drops from destroyed cars (a port feature)

Retail has no mechanism for them (section 5). Ways to add one, from the
data above (LIKELY, not tried):
- **New nodes:** allocate a kind-4 node (24 bytes: kind 4, id B, position,
  slot, child count 1, flags 0, child = the level's model of B) and hang it
  where the renderer and the car hit walk see it: under the level root
  (the kind-1 node with id 1 set by `worldLoadObject` case 1), for example
  by replacing one root child pointer with a new kind-1 group (sphere large
  enough, no-cull flag +21 bit 2) whose children are the old child and the
  drop nodes. Give it a free slot index (< 50) and fill `pickupObjects` and
  `pickupSlots` (kind, active) yourself; leave it out of
  `pickupRegisteredCount` so the respawn timer does not bring it back. Its
  `pickupApply` hides it like any other pickup.
- **Moving a registered pickup:** changing a kind-4 pickup's position is
  not enough: its parent groups are culled by their own spheres (an area of
  the level), so far from home it is neither drawn nor hit.
- The car's death is seen in `carDestroyed` (8011BE10, a0 = car record, a1
  = human) for humans and AI cars (`AI_LIVES.md` 1.1).

## 9. Notes

- `GAME_EVENTS.md` 13.1 guessed that 441..480 are "placed or bonus
  pickups": they are the rotating bonus spots of Los Angeles and Moscow
  (section 1). `SECRET_LEVELS.md` 3's "The AI uses the same slots" is not
  so (section 2.1).
- One pickup on Cyburbia (a 404, node 80067C18) has no model, only a
  hit box: an invisible Power missile. Swapping its model makes it visible.
- A remapped pickup keeps its parent's place in the level tree, so a pickup
  inside a destructible object still appears only when the object is
  destroyed.

## 10. A look for "a special" (request 79, built; `TM2_PORT_VISUAL`; request 83: no blinking name)

Port request 79 (2026-10-29, the user's playtest): Challenge Match's "Special pickups" (a turned-off weapon's pickups
may give a special) and Gun Game (every weapon pickup refills the level's weapon, a special or a pickup weapon) keep
the old weapon's look, so the player cannot tell. Asked: a pickup look for "a special" in the shared-model way of
section 6, on every arena; the port points those pickups at it.

**What the game has** (CONFIRMED, from the 14 arena files): no pickup model for a special. But every arena's HUD
object (1260, `UAdashInitDashboardIcons`) holds the special's icons as textured quads, icon 0 of each list:
- +40, the selected weapon's big icon: the blue SPECIAL globe, 32 x 32 texels on every arena;
- +24, the weapon list's names: the word SPECIAL, 31 x 8 (drawn green, (0, 140, 10), when selected).
They are in VRAM for the whole match (the HUD draws them), on the arena's own pages. Every weapon pickup model on the
disc has one shape (section 1): a group of a card (one upright camera-facing beam, kind 1, the weapon's picture), a
blink (an animation of two frames: an empty group and the weapon's name, another beam) and a hit box.

**The look** (`portPickupSpecial`, `port_visual.h` struct `TM2PortPickupSpecial`, `.port_bss`, 0x108 bytes):
- `worldLoadObject`'s case 1260 notes the HUD object (`portPickupSpecial.hud`); at the end of
  `worldLoadStartObjects` (after every object of the level, as section 6.1 asks) `portPickupSpecialBuild` takes the
  first weapon pickup of that shape in `pickupObjects` (401, 403, 404, 406, 411, 413 or 419) as the donor;
- its own group (the donor's header and sphere) holds: **the card**, the globe (one FT4 beam, half 32 x 32, colour
  128 as the HUD's, at the donor's card's centre); **the blink**, a copy of the donor's animation (its period) whose
  frames are the donor's empty group and **the name**, the word SPECIAL (half 40 x 10, green as the HUD's, at the
  donor's name's centre); and **the donor's hit box** (shared);
- the texture words are the icon's texel box, CLUT and page, with v0 top-left as the weapons' cards; nothing of the
  level file is written. Without a HUD object or a donor, `model` is 0 (no arena on the disc).

**For the port:**
1. After `worldLoadStartObjects` returns (where section 6.1 remaps), read `portPickupSpecial.model`; when it is not
   0, point a pickup at it as for any look: kind 1 node +0x18, kind 4 node +0x14 = `model`. The act stays the port's
   (`vp_pickup_id`'s PICKUP_SPECIAL 400, or the node id as section 6.1 says); keep the slot kind in 401..430.
2. Point the pickups again when one is re-rolled to or from a special (section 6.2). To give a pickup its own look
   back, point it at its kind's model (section 6.1's table).
3. The look is load state: the same on every machine (level data only), not changed by a step: snapshot and hash
   no. A new load rebuilds it, so read it after each load.

**Checked:** `tools/check_custom.py` check 2: the four functions in `.port_text`, no `rand()`, called only from
`worldLoadStartObjects` (once, after its last `dbscanHierarchy`); run in the link on all 14 arena files with
`pickupObjects` filled from the file: the model's group, card, blink and hit box as above, the texture words the
HUD icons', the donor a weapon pickup's model, the level file unchanged; with no HUD object, or no weapon pickup,
no model. Not tested in game: the look's size (the card 64 x 64 against the weapons' 40 x 100) is a first guess.

## 11. A mine's and the freeze shot's pictures for the kill feed (request 81, built; `TM2_PORT_VISUAL`)

Port request 81 (2026-10-29): the port's kill feed shows a kill's weapon as its HUD icon, but mines and the freeze
have none (MINE, FROZE as words). Asked: where their pictures are in VRAM during a match, or a function that gives
them, and whether the freeze shot is a model or sprites.

**What they are** (CONFIRMED, all 14 arena files): both are **sprites in the level's own models**, not 3D meshes, on
each arena's own texture pages (so the VRAM spot differs per arena: no fixed rectangle):
- **the mine**, asset 2212 (0x8A4; its pool `weaponMineObjects`, each object's model at +0x11C): a group, a group, an
  LOD of one band [0, 600) and a mesh of **one point sprite**, half 20 x 10, its texture **64 x 32 texels**, colour
  `2E7F7F7F` (semi-transparent) on a page with abr 2 (**subtractive**: the mine darkens what is behind it). E.g.
  Paris: page `00CE`, CLUT `2270`, u 0..63, v 224..255;
- **the freeze shot**, asset 2202 (`weaponFreezeMissileModel`; the freeze missile pickup and energy attack 106 both
  draw it): a group, a group and an **animation of 16 frames** (period 1) of meshes of 10 point sprites each: 9 small
  ones (half 40, 32 x 32 texels: the trail, along -y) and **the head** (half 60, 32 x 32 texels), colour `2EFF2000`
  (blue, semi-transparent; Swamp, Cyburbia and Roof Tops `2EB31F00`) on a page with abr 1 (**additive**). The frames
  step the head through different 32 x 32 cells (a pulsing glow). E.g. Paris frame 0: page `00B7`, CLUT `2130`,
  u 0..31, v 192..223.

**`portWeaponIcon(which, frame, out)`** (`port_visual.h`): `which` `TM2_ICON_MINE` (0) or `TM2_ICON_FREEZE` (1);
fills `struct TM2PortIcon` (12 bytes: `tpage`, `clut`, `u`, `v`, `w`, `h`, `colour`) with the model's largest sprite
(the mine's one sprite; the freeze shot's head in animation frame `frame`, modulo 16) and returns 1, or 0 when the
model is not loaded (out untouched). It reads the models as loaded for this match (the mine pool's object 0, the
freeze model), so call it after the level's load. Reads only, no `rand()`, no storage.

**Drawing it** (the port's): a textured quad over (u, v, w, h) of page `tpage` with `clut`. As the game draws them
they blend into what is behind (the mine subtracts, the freeze adds blue), which on a HUD panel would hardly show:
draw them **opaque** (texel 0 stays transparent), or opaque on a light backing for the mine. Which freeze frame
reads best was not looked at (the textures were not rendered here): frame 0 is a fair start; stepping `frame` with
time animates the icon as the shot does.

**Checked:** `tools/check_custom.py` check 2: in the link on all 14 arena files, the mine's icon and the freeze's
for frames 0, 5 and 16 equal the models' sprites read from the file; without the models it returns 0. Not tested in
game.

### 10.1 Request 83: the globe alone

Port request 83 (2026-10-29, the user's playtest of request 79): the look works, but the HUD's globe already reads
SPECIAL, so the blinking green word over it showed two words overlapping. Asked: the look without the blink, or the
word under the globe, whichever keeps the code cleanest. **Built: without the blink.** `portPickupSpecial.model` is
now a group of two children, the globe card and the donor's hit box; the animation copy, the name's mesh and the
weapon list's icon are gone (`TM2PortPickupSpecial` 0x84 bytes, was 0x108; `model` still at +0). The donor test asks
only for a card first and a hit box last. Nothing changes for the port: it points pickups at `model` as before.
