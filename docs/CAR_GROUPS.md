# Car groups: two or more cars of one type, and more cars

Answer to the PC port's request 32, **step 1** ("Duplicates", sections 1-6), **step 2** ("More
cars", section 7), **step 3** ("Car swap", section 8) and **step 4** ("Texture banks", section 9) of
`CAR_GROUPS_PLAN.md`, and request 33 **step 2a** ("Per-car specials", section 10, `PER_CAR_PLAN.md` C.2) and **step 2e**
("Dark Tooth per car", section 11), **step 3** ("Per-car engine sound", section 12) and **step 1** ("Per-car CLUTs",
section 13), **step 4** ("The rest", section 16), and **online with bots, players dropping in** (section 14): a match may hold
several cars of one type, each with its own looks and its own binding, (step 2) up to 16 active cars
with every limit a macro, (step 3) a car may change driver during the match (the Juggernaut), and
(step 4) every car may have its textures in a VRAM bank of its own, (33-2a) every car's special has a
state of its own, and (33-2e) every Dark Tooth has its own head and boss state (up to 16), optionally a boss on
any arena, (33-3) every car's engine is a voice of its own, (33-1) without banks every car may have its own
CLUTs (team colours, invisibility), and (drop-in) a car and its driver may leave the match for good, a record may start
mid-match on a free car, and a car may be loaded mid-match from port buffers. Sources: the C units
listed below, `SURVIVAL.md` 2.2-2.5 (the tables and the code that finds a car by its type), and the
car files `DMD\*.DMD` on the USA disc. Tags: **CONFIRMED** = read in code, counted in the data or
proved by the build; **GUESS** = inferred, not run.

## Summary

- **The define:** `--port-define TM2_PORT_CAR_GROUPS` on a port variant build
  (`SOURCE_BUILD.md` "Port variant", "Port code"). Without it every changed line is the retail C
  (each change is `#ifdef TM2_PORT_CAR_GROUPS ... #else <retail> #endif`, or a macro whose
  `#else` is the retail expression).
- **What retail does wrong with duplicates** (CONFIRMED, `SURVIVAL.md` 2.2-2.4). The per-car looks
  (body, wreck, damage stage, tire switches, turbo flame, spinning parts, freeze record) live in
  tables indexed by the car type (`id >> 4`). A second car of a type overwrites the first one's
  entries, the binders bind both cars to the first car's slot, and lookups by type
  (`carObjectFromOwner`: Mr. Slam's grab, crushing, spin-outs) find the first car.
- **The change:** a **car key** per loaded car DMD, with the layout of the retail value (`key >> 4`
  is the table index). Every reader and writer of those tables uses new tables indexed by key
  (`portCarGroups`, 32 entries), and the code that stores or passes "which car" passes the key.
  The binders bind each car to the slot of its own load group.
- **Keys** (`CAR_GROUPS_PLAN.md` D1). Assigned when a car DMD's root registers
  (`carRegisterModel`):
  - the **first** car of a type in the load order (humans first, then opponents) gets
    `key = type` (0x10..0xF0);
  - every **further** car of that type gets an overflow key `(16 + d) << 4` (0x100, 0x110, ...,
    0x1F0), d counting up in load order;
  - the roots of one DMD share their group's key (Minion's two 224 roots).
- **Equal to retail by construction** with no duplicate type (CONFIRMED by reading): every key
  equals the type, so every index, every object and every stored value is the retail one,
  including the values kept in the car records (`stats.crush_owner`, the slam state's
  `spin_target`). No `rand()` call is added or removed; with `portCarOptions` = 0 the pickers are
  retail too.
- **Not per car** (left as retail, `CAR_GROUPS_PLAN.md` 1.10 and 6.Q4-Q6): the engine sound per
  type (per car with request 33-3, section 12), the single shared specials (Twister's tornado, Shadow's strike, Grasshopper's leap state,
  Mr. Slam's arm and claw, the Dark Tooth head), and the CLUT (one per type in VRAM: a duplicate's
  invisibility fade also fades its twin; per car with step 4's texture banks, section 9).
- **Status:** built and proved statically (sections 6 and 7.9). Not run.
- **Step 2** (`TM2_PORT_CARS=16`, section 7) raised step 1's registries: 64 car slots and 48 key
  indices (32 loaded cars, Minions' second roots, up to 32 overflow keys), so `portCarGroups` is now
  16,520 bytes and the offsets of section 3 are those of the larger struct.
- **Step 3** (`TM2_PORT_CAR_SWAP`, section 8): `portCarSetGroup(human, index, group, hp)` moves a
  record to another loaded car between frames; port code plus one line of `sessionBindCars`.
- **Request 33 step 2a** (`TM2_PORT_CAR_SPECIALS`, section 10): Mr. Slam's arm and claw, Twister's tornado,
  Shadow's strike, Axel's shockwave and Grasshopper's leap per car key (the first car of a type keeps the retail
  global, duplicates use `portCarSpecials`); the claw-record pool without retail's overflow. Retail-equal with
  no duplicate, no run-time bit. Dark Tooth's boss state: 33-2e, section 11.
- **Request 33 step 2e** (`TM2_PORT_DARK_TOOTH`, section 11): Dark Tooth's head, timers, swoop state and stage per
  car (a sixth part of 33-2a's selection), retail-equal with one Dark Tooth; `portCarOptions` bit 6 adds
  `SURVIVAL.md` 1.5/1.6 (boss weapons and a per-car stage 2 on any arena).
- **Request 33 step 3** (`TM2_PORT_CAR_SOUND`, section 12): with `portCarOptions` bit 4 every car plays its engine
  (and Thumper's flame, Dark Tooth's swoops) as its own sound instance; no cap by default (`portEngineVoices` = N
  keeps the N nearest to a human); engine voices are the first to drop when the SPU's 24 run out.
- **Request 33 step 4** (section 16, `TM2_PORT_CAR_SOUND` and `TM2_PORT_CARS`): the human loops (turbo, guns,
  burn/lava) of players 2.. per car instead of leaking a sound request every frame; with bit 4 Twister's tornado
  sound per car and every special-fire sound's list room for 4; the sound request pool 35 -> 64 with `TM2_PORT_CARS`.
- **Request 33 step 1** (`TM2_PORT_CAR_CLUTS`, section 13): with `portCarOptions` bit 7 (no banks) every CLUT a car
  would share is copied after the car TMS files (rows 240..255 first, shared with bit 6), the fade per car, each
  group's CLUT ids published in `portCarCluts` for team paint; bit 7 is ignored while banks are live. Bit 9
  (13.12): the CLUT scan tests every texel of a spot (retail: the top-left one), rows 240..255 last.
- **Request 33 drop-in** (`TM2_PORT_CAR_DROPIN`, section 14): `portCarRemove` takes a car and its driver out for good
  (its projectiles, mines and drops too; every reference cleared; an AI record left vacant, humans compacted),
  `portCarStartHuman` / `portCarStartAi` start a record on a free car as a respawn, `portCarLoadGroupLive` loads a car
  between frames from buffers the port hands over (with or without banks), `portCarUnloadGroup` gives a group nobody
  drives back for good (DMD bytes, slots, key, group number, scene objects, per-key state, CLUT copies, bank, image
  records, sound), and `portCarRoom` / `portCarCanLoad` tell the port what is free (14.7).
- **Step 4** (`TM2_PORT_TEXTURE_BANKS`, section 9; `TEXTURE_BANKS.md` 6): with `portCarOptions` bit 5 each
  load group's textures and CLUTs go to a VRAM bank of its own, named in tpage bits 9..15 of its
  primitives; the invisibility fade per car; `portVramUploadBank` tells the port's GPU the bank of each
  upload.

## 1. Keys and slots

A **load group** is the `n` of `assetInstallImage(base, n)` in `sessionLoadArenaAndCars`: human
i's DMD is group i + 1, opponent i's is group humans + i + 1 (`vramTexSlotGroup` while the file's
nodes register). A **slot** is a `carRegisterModel` entry (`carSlotObjects`, `carSlotOwners`, ...):
one per car root, so Minion's second root takes a slot too (D2: the slot numbers stored in the car
records, +0x004 human / +0x006 AI, stay retail).

| when | what happens (port function) |
|---|---|
| `carTablesClear` (every load, before the level and the cars) | `portCarGroupsReset`: slots, groups, keys and every object pointer forgotten. The tire accumulators, spin angles and the freeze records of key indices 1..15 are kept, as retail keeps its own (retail never clears them); the duplicates' freeze records (16..31) are zeroed |
| a car root registers (`carRegisterModel`) | `portCarRegisterRoot(slot, type)`: the slot's group is `vramTexSlotGroup`; the group's first root fixes its key (the type, or the next overflow key when another group already has the type's key; after 16 overflow keys, the type again = retail sharing) |
| the DMD's other nodes register (`carBodyObjectRegister`, `carWreckModelRegister`, `carRegisterTireSwitch`, `objRegistryAdd` for +6, `carFreezeBuild`, `carPartRegister*`) | the entry goes to `portCarLoadKey() >> 4`, the key of the group being installed; 0 (no root yet) falls back to the retail index |
| `arenaSetup` | each slot's damage-stage switch is registered under the slot's key (`TM2_CAR_KEY(i, type)`); Minion's two slots share it, the last one wins (retail order) |
| `sessionBindCars` | human i binds to the first slot of group i + 1, opponent i to the first slot of group humans + i + 1 (`portCarSlotOfGroup`); if that group has no slot of the expected type, the retail search by type |

The root registers before every other node of its file (CONFIRMED: `dbscanHierarchy` calls a node
back when it pops it, after pushing its children, so a root's callback is the first of its tree;
on the disc each of the 15 car files has a car root as its first root and first callback,
`tools/check_car_groups.py --dmd`). The level DMD (no car roots) registers before any car root,
so its nodes keep the retail indices.

`TM2_CAR_KEY(slot, type)` (`include/semantic/car_groups.h`) is `portCarKey(slot, type)`: the slot's
key, or `type` if the slot has none (out of range, never registered). Without the define it is
`(type)`.

## 2. What changed (47 functions)

All in the port variant with the define only. The list is checked mechanically
(`tools/check_car_groups.py --expect-functions`, section 6).

**Registration and reset (15):**

| VA | function | unit | change |
|---|---|---|---|
| 80119848 | `carTablesClear` | `ua_init.c` | `portCarGroupsReset()` first |
| 8011961C | `carRegisterModel` | `ai_route_helpers.c` | `portCarRegisterRoot(slot, id)` per new slot |
| 80114EEC | `carBodyObjectRegister` | `misc_one_liners.c` | `portCarGroups.body[key >> 4]`; retail's `id >> 4 < 15` bound stays (Dark Tooth's 243 is not registered, as retail) |
| 801152A8 | `carWreckModelRegister` | `misc_one_liners.c` | `wreck[key >> 4]`, same bound |
| 8012478C | `carRegisterTireSwitch` | `car_switch_registry.c` | `tire[key >> 4][slot]` |
| 80123F78 | `objRegistryAdd` | `object_registry.c` | turbo (+6, ids below 256): `turbo[key >> 4]`; ids 256..399 and the +7 path keep retail's tables |
| 80124384 | `objRegistrySetStep` | `object_registry.c` | the `type + 6` path reads `portCarGroups.turbo[id >> 4]` (the first car of the type, = retail's object) |
| 80124C40 | `carFreezeBuild` | `car_freeze_build.c` | the record `freeze[key >> 4]` (the model's pointer to its record points into `.port_bss`) |
| 801149B8, 80114A08, 80114A58, 80114AA8, 80114AF8 | `carPartRegisterHammerhead/Minion/MrSlam/DarkTooth/Axel` | `arena_load_register.c` | `parts[key >> 4][k - 1]`, retail bounds 4/6/4/4/2; fallback key 0x20/0xE0/0xC0/0xF0/0xB0 |
| 8011995C | `arenaSetup` | `arena_setup.c` | `carChoiceRegisterGroup(obj, TM2_CAR_KEY(i, carSlotOwners[i]))` (its jump table `.tm2_800CF2B0` moves with it) |
| 801246A8 | `carChoiceRegisterGroup` | `car_switch_registry.c` | `damage_stage[key >> 4]` |

**Readers (10):**

| VA | function | unit | change |
|---|---|---|---|
| 80114E88, 80114E10 | `carBodyObjectHide`, `carBodyObjectShow` | `ai_route_helpers.c` | `portCarGroups.body`, index < 32 |
| 80114F24 | `carBodyPoseUpdate` | `car_body_pose.c` | body of `TM2_CAR_KEY(rec->slot, rec->type)`; the height test stays by type |
| 801152E0 | `carWreckModelGet` | `small_game_helpers.cpp` | `portCarGroups.wreck` |
| 80124718 | `carSetDamageStage` | `car_switch_registry.c` | `portCarGroups.damage_stage`, index < 32 |
| 80124800 | `uaswToggleCarTireState` | `car_switch_registry.c` | `portCarGroups.tire`, index < 32 |
| 80114B70 | `carPartsAnimate` | `car_parts_spin.c` | type of the key (`portCarKeyType`) 0x20/0xB0/0xC0/0xE0/0xF0 → `portCarPartsSpin(key, amount)`; else `tire_accum[key >> 4]` and `uaswToggleCarTireState` as retail |
| 80124C00 | `carFreezeSetState` | `car_freeze_dispatch.c` | `portCarGroups.freeze`, index < 32 |
| 80124B80 | `carFreezeResetAll` | `car_freeze_dispatch.c` | all 32 freeze records and turbo objects |
| 8011D15C | `carObjectFromOwner` | `car_records.c` | compares the slot's key (`portCarGroups.slot_key`) instead of its type; this re-keys `carSpinOutByObject`, `carSpinOutById`, `carDamageByObject`, `carCrushContactUpdate`, `weaponSlamCarryTarget` and the Dark Tooth test of `weaponProjectileUpdate` (`carObjectFromOwner(240)`: the first Dark Tooth, as retail) |

**Callers that pass "which car" (18):** the type stays wherever it is compared with a constant;
only the value that names the car becomes its key.

| VA | function | unit | change |
|---|---|---|---|
| 800E5628 | `carApplyDamage` | `car_damage.c` | hide, wreck model, damage stage by the hit car's key |
| 800ED128 | `carGroundEffects` | `car_wheels.c` | hide/show (sinking) by key |
| 80105A7C | `weaponFireSpecial` | `weapon_basic.c` | Twister's body hide by the firer's key |
| 8010DE54 | `weaponInvisibilityUpdate` | `ai_link_node_pick.c` | body hide/show by key; the CLUT fade stays by type |
| 8010E040 | `weaponTornadoUpdate` | `weapon_tornado.c` | victims' spin-outs by key; Twister's body by the key of `weaponTornadoState.object` (`portCarKeyOfObject`, else 160) |
| 8010F520 | `weaponHomingLeapUpdate` | `weapon_leap_homing_update.c` | turbo flame of the leaping car (`homingLeapState.owner_id` = its slot) |
| 8011B89C | `humanCarDamageUpdate` | `car_damage_human.c` | damage stage, show, hide by key |
| 8011BBD4 | `aiCarDamageUpdate` | `world_collapse_warps.c` | the same for AI cars |
| 800DC88C | `humanCarTick` | `car_tick.c` | freeze state and parts by key (its jump table `.tm2_800CEAE0` moves with it) |
| 800E7EDC | `aiCarDrive` | `ai_driver.c` | the same |
| 800DE5F8 | `humanCarReadInput` | `car_input.c` | turbo flame: `portCarTurboSetStep(key, v)` instead of `objRegistrySetStep(type + 6, 0, v)` |
| 800EC180 | `aiInputFinalize` | `ai_steps.c` | the same |
| 800D6FD8 | `projectileHitTest` | `weapon_slam_swing.c` | the freeze missile's freeze state by key |
| 800F4058 | `weaponSlamSwingUpdate` | `weapon_slam_swing.c` | Mr. Slam's own spin-out (`192`) by the slammer's key |
| 800F5498 | `weaponSlamFindTarget` | `screen_card_draw.c` | `spin_target` = the grabbed car's key |
| 800E6D14 | `carCrashStart` | `car_crash.c` | `crush_owner` = the ridden car's key |
| 800E75F4 | `carWreckUpdate` | `car_crash.c` | the wreck's own object by key |
| 8011BE10 | `carDestroyed` | `car_destroyed.c` | its spin-out by key (the stored `spin_target` passes through) |

**Binders and pickers (4):**

| VA | function | unit | change |
|---|---|---|---|
| 8012BD94 | `sessionBindCars` | `screen_card_layout.c` | passes each car's load group |
| 8011A29C | `carBindHumanByType` | `world_object_register.c` | new 3rd parameter `group`; the group's slot, else the retail search |
| 8011A54C | `carBindAiByType` | `ai_route_helpers.c` | the same; `aiCarTotal++` once per newly bound AI (kind 2), as retail; a rebind in later rounds finds kind 1 and does not count again |
| 8012CC10 | `carIsSelectable` | `car_is_selectable.c` | `portCarOptions & 1`: skip the two "already taken" loops (the unlock tests stay). This reaches `screenCarSelect`, `screenAiSelect` and `tournamentPickOpponents` (whose `rand() % 13` loop then accepts repeats) |

Not changed: `worldLoadObject` (the registration functions read the group themselves), the
functions that only pass a stored key on (`weaponSlamGrabUpdate`, `weaponSlamRelease`,
`carSpinOut*`, `carDamageByObject`, `carCrushContactUpdate`, `weaponSlamCarryTarget`),
`carPartsSpinHammerhead/Minion/MrSlam/DarkTooth/Axel` (no longer called; `carPartsAnimate` uses
`portCarPartsSpin`), `carPartsReset` and `objRegistriesReset` (they still clear the unused retail
tables).

## 3. For the port: API and variables

**Build:** add `--port-define TM2_PORT_CAR_GROUPS` (works with `TM2_PORT_FAR_LOD`,
`TM2_PORT_FAR_DISTANCE` and the port's `TM2_PORT_HOOKS`; it shares no function with the first two):

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_HOOKS --port-define TM2_PORT_CAR_GROUPS --output work/pv/P4
```

**Storage** (`.port_bss`, group `one`, defined in every variant so the layout does not depend on the
define; addresses of the default 4-player variant, `out/TM2_SOURCE_PORT/symbols.tsv`; they move
with `--port-limits` and with storage added before them, so read them from the map):

| variable | address | size | meaning |
|---|---|---|---|
| `portCarGroups` | 0x8039EC40 | 16,520 bytes | `struct TM2PortCarGroups` (`include/semantic/car_groups.h`) |
| `portCarOptions` | 0x803A2CC8 | 4 | bit 0: the car pickers allow repeated cars; bits 1-3: step 2 (section 7.3); 0 = retail |

`struct TM2PortCarGroups` (offsets in bytes):

| offset | field | contents |
|---|---|---|
| 0x000 | `slot_group[64]` | load group of each car slot |
| 0x100 | `slot_key[64]` | key of each car slot (0 = none) |
| 0x200 | `group_key[64]` | key of each load group (0 = none yet) |
| 0x300 | `key_type[48]` | car type of each key index |
| 0x3C0 | `dup_count`, `pad` | overflow keys handed out in this load |
| 0x3C8 | `body[48]` | body object per key index (retail `carBodyObjects`) |
| 0x488 | `wreck[48]` | wreck model (`carWreckModels`) |
| 0x548 | `damage_stage[48]` | damage-stage switch (`carDamageStageObjects`) |
| 0x608 | `tire[48][3]` | tire switches (`carTireSwitchObjects`) |
| 0x848 | `tire_accum[48]` | tire turn accumulator (`carPartsTireTurnAccum`) |
| 0x908 | `turbo[48]` | turbo flame object (`carTurboObjects`) |
| 0x9C8 | `parts[48][6]` | spinning parts (`carPartsHammerhead/Minion/MrSlam/DarkTooth`, `carAxelObjects`) |
| 0xE48 | `part_angle[48]` | their spin angle (`carPartsSpinAngle*`) |
| 0xF08 | `freeze[48][264]` | freeze records (`carFreezeObjects`) |

(Before step 2: 32 slots and 32 key indices, 10,888 bytes; `TM2_CAR_SLOT_CAP` and `TM2_CAR_KEYS` in
`car_groups.h`.)

**Port functions** (`.port_text`, `port_variant_arrays.c`; only with the define; addresses of the
build of step 1 with this define alone, before step 2 grew the storage: they move with it, so read them
from the build's map, `functions.tsv`):

| function | address | use |
|---|---|---|
| `portCarGroupsReset()` | 0x803A2490 | called by `carTablesClear` |
| `portCarRegisterRoot(slot, type)` | 0x803A25A8 | called by `carRegisterModel` |
| `portCarLoadKey()` | 0x803A26AC | key of the group being installed (0 before its root) |
| `portCarKey(slot, type)` | 0x803A26E4 | `TM2_CAR_KEY`: the slot's key, else `type` |
| `portCarKeyType(key)` | 0x803A2718 | the car type of a key |
| `portCarKeyOfObject(object)` | 0x803A2754 | key of the slot holding a world object, else 0 |
| `portCarSlotOfGroup(group)` | 0x803A27CC | first slot of a load group, else -1 |
| `portCarTurboSetStep(key, value)` | 0x803A282C | turbo flame on/off (retail `objRegistrySetStep(type + 6, 0, value)`) |
| `portCarPartsSpin(key, amount)` | 0x803A2868 | the `carPartsSpin*` body for the key's parts and angle |

**Reading a car's key from the port** (e.g. for its own HUD or for step 3's swaps):
`portCarGroups.slot_key[slot]` with the record's slot (human +0x004, AI +0x006). The type of a key:
`key < 0x100 ? key : key_type[key >> 4]`.

## 4. Limits

- **Slots:** retail's 15 (`carRegisterModel`): cars + Minions' second roots; with step 2's
  `TM2_PORT_CARS` 64 (`TM2_MAX_CAR_SLOTS`). The registries hold 64 slots and 48 key indices: index 0
  unused, 1..15 the first car of each type, 16..47 overflow keys (32 loaded cars have at most 31
  duplicates; keys up to 0x2F0).
- **Shared, not per car** (retail behaviour, documented in `CAR_GROUPS_PLAN.md` 1.10):
  - **engine sound** (`carSoundUpdate`, `worldUpdate`): one sound id per type; two cars of a type
    play and stop the same sound (6.Q4: deferred);
  - **single specials**: Twister's tornado (`weaponTornadoState`), Shadow's strike
    (`weaponStrikeState`, object 2325: the last install wins), Grasshopper's leap state
    (`homingLeapState`), Mr. Slam's arm and claw (`slamArmRegister`/`slamClawRegister`: the last
    loaded Mr. Slam's objects and one angle), the Dark Tooth head (249). Two cars of these types
    firing their specials at once share them (6.Q5). Per car with request 33-2a (section 10), except the
    Dark Tooth head;
  - **CLUTs**: one CLUT per type in VRAM; the invisibility fade of a duplicate also fades its twin
    (6.Q6). The body hide/show itself is per car. With step 4's banks (section 9, `portCarOptions`
    bit 5) each car has its own copy and its own fade.
- **Dark Tooth not loaded last** still overwrites the texture slots of the next groups (`SURVIVAL.md`
  4.2): unchanged.
- **Pools sized for one car of a type** (retail): Mr. Slam's claw records (4 + an overflow,
  `RETAIL_BUGS.md` 17: five Mr. Slams in one load reach it), arcs (5), shields (4), remote bombs (2),
  ricochets (10), Thumper flames (2). Step 2 raises them (`TM2_PORT_CARS`, `portCarOptions` bit 3,
  section 7).
- **Dark Tooth's body** is not registered (retail's `< 15` bound on the type index), so Dark Tooth
  has no body pose or body hide, as retail.
- **The binders** still take the type from `carIndexToType`; a group whose slot holds another type
  (a load order that does not follow `sessionLoadArenaAndCars`) falls back to the retail search.
- **Persistent state:** the spin angles, tire accumulators and freeze records of key indices 1..15
  persist across matches like retail's; the overflow keys' accumulators and angles persist too
  (they start where the last car with that key left them, as a first car's do in retail).

## 5. Test checklist for the port

1. **Equality, no duplicates** (`portCarOptions` = 0, ≤ 11 distinct cars): frame-by-frame against the
   variant built without the define (or against E0 with the variant's address map). Car records,
   `rand()` calls and the drawn scene must match. Expected differences: the addresses of the new
   storage and the freeze record pointer inside each car model copy (`carFreezeBuild`'s
   `output[8]`, which now points into `.port_bss`).
2. **Two of a type** (set `portCarOptions` = 1, pick e.g. two Mr. Slams or human Outlaw + AI Outlaw):
   - both cars appear, each driven by its own record (`carSlotIndices`, `aiCarTotal` counts both);
   - damage looks (`carSetDamageStage`) and the body hide on death are per car;
   - the turbo flame shows only on the car that boosts;
   - freezing one (freeze missile) shows the ice only on that one;
   - spinning parts (Hammerhead, Axel, Mr. Slam, Minion, Dark Tooth) and the tire switches turn per
     car;
   - Mr. Slam grabbing the second car of a type moves that car, not the first (`spin_target`);
   - riding/crushing a duplicate (`crush_owner`) follows the ridden car;
   - the wreck that breaks up is the one destroyed (`carWreckUpdate`);
   - Twister's tornado: the firing Twister's body hides/shows (the tornado itself is shared).
3. **Minion twice** (two Minion roots each: 4 slots): bindings, parts and damage per car.
4. **Many duplicates**: up to the slot limit (15 slots; 64 with `TM2_PORT_CARS`), all overflow keys
   distinct (`portCarGroups.group_key`), no key ≥ 0x300.
5. **Pickers:** with bit 0 set the car select and AI select screens offer taken cars; tournaments
   may draw repeats. With bit 0 clear the pickers are retail.
6. **Rounds and modes 3/4:** `sessionBindCars` again each round (rebinding finds kind 1, no second
   `aiCarTotal++`); `carFreezeResetAll` at round start clears every car's ice and turbo flame.
7. **Shared limits** (expected, not bugs): engine sound per type, the shared specials, the CLUT fade.

## 6. Status and proofs

(Step 1 as built; step 2's build and proofs, with the larger registries: section 7.9.)

- **Built and proved** (`build.py --source-only --sdk-from-lib --relocatable --port-variant`, `work/build-20261012-152607-394949`; the define builds use the same objects). The retail images keep their hashes: hybrid
  `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` `f557c1e3…`. The variant passes its
  proof against E0 (`verify_reloc.py`) and the placement check (limits forced to 2) in these
  configurations:
  - without the define: `106c0f21…`, published as `out/TM2_SOURCE_PORT.EXE` (was `11100afc…`:
    only `.port_bss` grew, by 10,892 bytes, to 80200610 + 0x1A1E7C, so `.port_text` now starts at
    0x803A2490; the code is unchanged, still the 25 functions of request 25);
  - with `TM2_PORT_CAR_GROUPS`: `dfdca515…`;
  - with `TM2_PORT_CAR_GROUPS`, `TM2_PORT_FAR_LOD` and `TM2_PORT_FAR_DISTANCE`: `e74af59f…`.

  The placement check's `port_changed` functions are exactly the 47 of section 2 (with the other
  two defines, theirs too); the changed jump tables are `humanCarTick`'s (800CEAE0) and
  `arenaSetup`'s (800CF2B0), every word pointing into its changed function.
- **`tools/check_car_groups.py`** (the plan's static checks, 1.11):
  - `check_car_groups.py <link dir>`: every relocation into a retail per-type table (retail address and size mapped through
    `addrmap.tsv`) is in an allowed function: `carPartsReset`, `objRegistriesReset` (they clear
    the unused retail tables), `carPartsSpin*` (no longer called), `objRegistryAdd` /
    `objRegistrySetStep` (the +7 path and ids 256..399). With the define this holds; on the
    default variant the check fails with 19 more readers (negative control);
  - `--expect-functions`: the define's `port_changed` code sections equal the 47 functions;
  - `--dmd`: the 15 car DMDs on the disc register their car root first;
  - `--key-model`: a host model of `portCarRegisterRoot` (11 or 15 distinct cars: keys = types;
    16 Minions: 0xE0, 0x100..0x1E0; mixed duplicates; since step 2 also 32 loaded cars and 32
    Minions: 0xE0, 0x100..0x2E0): every index below 48, every group one key.
  - Since step 2 check 1 runs on builds with `TM2_PORT_CAR_GROUPS` (or with `--negative-control`),
    and `--expect-functions` compares with the union of the step 1 and step 2 lists for the defines
    of the build.
- `tools/data_audit.py`: 0 mismatches.
- **Not runtime-tested.** The equality with no duplicates holds by reading (every key equals the
  type); the duplicate behaviour is the port's to check (section 5).

## 7. Step 2: more cars (`TM2_PORT_CARS`)

Request 32 step 2 (`CAR_GROUPS_PLAN.md` 2), with the port's answers to the plan's section 6 and to
request 33 (`DECOMP_REQUESTS.md` 32/33: Q3, Q7-Q9, Q11-Q14 of the first list; Q8-Q10, Q13-Q15 of the
`PER_CAR_PLAN.md` list). Everything is in the port variant behind `--port-define TM2_PORT_CARS=16`
(any N in 11..16); without it every changed line is the retail C (`#ifdef TM2_PORT_CARS ... #else
<retail>`, or a limit macro that expands to the retail constant). Pair it with
`TM2_PORT_CAR_GROUPS` (section 1-6): without step 1 a repeated type binds to the first car of the
type, as retail, so at most 15 distinct cars (+ Minion's second root) are useful.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CARS=16 --output work/pv/P4
```

### 7.1 Limits (`include/semantic/players.h`)

Every limit is a macro (players.h style: predefinable, `TM2_RETAIL_*` names what retail holds,
`--port-limits 2` forces the retail value); none is derived from the human count, so humans and
computer cars can each reach their own limit (request 33: 16 humans later, or 1 human + 15 bots).
**Active cars** (humans + bound computer cars) <= `TM2_PORT_CARS` is a run-time rule the port keeps
where it builds the settings (and in its drop-in code), not an array size.

| macro | retail | `TM2_PORT_CARS=16` | sizes | bytes per unit | at 16 |
|---|---:|---:|---|---:|---:|
| `TM2_PORT_CARS` | - | 16 | active cars (the rule above); the derived limits | | |
| `TM2_PORT_CARS_LOADED` | - | 32 | loaded cars / load groups (16 active + the Juggernaut's Minion + spares + joins, Q14) | | |
| `TM2_MAX_LOADED_CARS` | 11 | 32 | `portCarSource` (per load group) and the texture-slot check | 16 per group | 528 |
| `TM2_MAX_AI_CARS` | 9 | 16 | `aiCars` (0x854), `aiCarObjects`, `aiCarObjectSlots`, `aiCarTargets`; opponent slots (`sessionPortOpponentCars` holds 9..15) | 2,144 + 1 | 34,311 |
| `TM2_MAX_HUMANS` | 2 | 4 (unchanged) | the human records (request 25); more humans are a later request: `#error` past 7 (object id 8, `SIXTEEN_CARS.md` 1.5) | 2,068 + 24 | |
| `TM2_MAX_CAR_SLOTS` | 15 | 64 | the four slot tables (`carSlot*`); the five pair tables [slots][slots] | 16 per slot + 34 per pair | 1,024 + 139,264 |
| `TM2_TEX_SLOT_STRIDE` | 20 | 32 | texture slots per load group (`vramTexSlots`, in packet buffer 0 while loading: up to 49 groups at 32) | (20 per slot, load time) | |
| `TM2_PORT_CAR_DMD_AREA_BYTES` | - | 2 MiB + 2 KB | `portCarDmdArea` (default; every variant) | | 2,099,200 |
| `TM2_MAX_RADAR_BLIPS` | 12 | 15 | radar lines per HUD record (`hudRadars` entry 0x70 + 64 per blip) | 64 per HUD | 0x430 per HUD |
| `TM2_MAX_PROJECTILES` | 16 | 48 | `fxRecords` (980 bytes; each takes 2 coordinate systems) | 980 | 47,040 |
| `TM2_MAX_COORD_SYSTEMS` | 128 | 256 | `csObjectPool` (`csAlloc`) | 296 | 75,776 |
| `TM2_MAX_SOUND_INSTANCES` | 35 | 64 | `soundInstPool` (0x84) and `portSoundSortList` | 136 | 8,704 |
| `TM2_MAX_SOUND_REQUESTS` | 35 | 64 | `sndObjPool` and its objects `portSndObjSlots` (retail: main's stack); request 33 step 4, section 16 | 124 | 7,936 |
| `TM2_MAX_ARCS` | 5 | 16 | `weaponArcs` (taser, lightning) | 20 | 320 |
| `TM2_MAX_SHIELDS` | 4 | 16 | `weaponShieldPool` { objects[N]; records[N] } | 16 | 256 |
| `TM2_MAX_REMOTE_BOMBS` | 2 | 16 | `weaponRemoteBombObjects` | 4 | 64 |
| `TM2_MAX_RICOCHETS` | 10 | 32 | `weaponRicochetPool` { objects[N]; shots[N] } | 52 | 1,664 |
| `TM2_MAX_FLAME_EMITTERS` | 2 | 16 | `weaponFlameEmitters` (Thumper) | 32 | 512 |
| `TM2_MAX_SLAM_RECORDS` | 4 | 17 | `weaponSlamRecords` (Mr. Slam's claw records; the allocator hands out one more than its wrap point) | 18 | 306 |
| `TM2_MAX_WORLD_MODELS` | 20 | 40 | `renderWorldModels` (car bodies, level models) | 4 | 160 |
| `TM2_MAX_CLUT_ANIM_DEFS` | 20 | 40 | `clutAnimDefs` | 20 | 800 |
| `TM2_MAX_CLUT_ANIMS` | 10 | 20 | `clutAnimRunning` | 1,060 | 21,200 |
| `TM2_MAX_FX_SPRITES` | 150 | 400 | `fxSprites` (25 per car; request 102, BALANCE.md 23.1) | 48 | 19,200 |
| `TM2_CAR_SLOT_CAP`, `TM2_CAR_KEYS` (`car_groups.h`) | - | 64, 48 | step 1's registries (fixed, every variant) | 12 per slot, 328 per key | 16,520 |

Weapon slots (25) stay (Q11; the effect sprites are raised since request 102); mines (23), drops (25), ground fires (30),
bullets and the 4 shield-state/strike singletons are not raised (the port measures, Q9). Object ids:
AI j gets j + 50 (50..65), humans i + 1 (1..4).

Where the arrays live:

- **Every variant** (retail sizes without the define, so the default variant's code is unchanged:
  only addresses move, Q7): the car arrays of `SIXTEEN_CARS.md` 1.1 (`aiCars` first: its address is
  the same with and without the define), `fxRecords`, `csObjectPool`, `soundInstPool`, and the new
  port storage below. `symbols.tsv` names each moved array with its retail name, and `addrmap.tsv`
  maps every retail address in them (rows `port`; with the define the pair tables, `hudRadars`,
  `weaponShieldPool` and `weaponRicochetPool` change layout, rows `port_relayout`: only their start
  maps).
- **Only when raised** (tsv groups `name+`): the weapon and render pools of Q9 (`weaponArcs` ..
  `clutAnimRunning`), and since request 33 step 4 the sound request pool (`sndObjPool`, `portSndObjSlots`). Without the define they stay at their retail addresses, so the default variant
  is unchanged there too.

### 7.2 Storage (`.port_bss`; `config/US/port_variant_arrays.tsv`)

Addresses of the published default variant (`out/TM2_SOURCE_PORT/symbols.tsv`) and of the build with
`TM2_PORT_CARS=16` alone; they move with the limits and with storage added before them, so read them
from the build's `symbols.tsv`.

| variable | default variant | size | `TM2_PORT_CARS=16` | size | contents |
|---|---|---:|---|---:|---|
| `portCarOptions` | 0x803A2CC8 | 4 | 0x803A2CC8 | 4 | step 1 + 2 switches (7.3) |
| `portCarGroups` | 0x8039EC40 | 16,520 | 0x8039EC40 | 16,520 | step 1 registries (section 3), 64 slots / 48 keys |
| `aiCars` | 0x803A2CCC | 19,188 | 0x803A2CCC | 34,112 | AI car records (0x854 each); first car array: same address with and without the define |
| `aiCarObjects` | 0x803A77C0 | 36 | 0x803AB20C | 64 | world object per AI car |
| `aiCarObjectSlots` | 0x803A77E4 | 36 | 0x803AB24C | 64 | car slot per AI car |
| `aiCarTargets` | 0x803A7808 | 36 | 0x803AB28C | 64 | AI target per AI car |
| `carSlotKinds` | 0x803A782C | 60 | 0x803AB2CC | 256 | per car slot: 0 human, 1 AI, 2 loaded |
| `carSlotIndices` | 0x803A7868 | 60 | 0x803AB3CC | 256 | per car slot: record index |
| `carSlotOwners` | 0x803A78A4 | 60 | 0x803AB4CC | 256 | per car slot: type |
| `carSlotObjects` | 0x803A78E0 | 60 | 0x803AB5CC | 256 | per car slot: world object |
| `sessionPortOpponentCars` | - | - | 0x803AB6CC | 7 | opponent slots 9..15 (Q8) |
| `portCarsPerArena` | 0x803A791C | 4 | 0x803AB6D4 | 4 | cars per arena, 0 = retail |
| `portCarPoolsLive` | 0x803A7920 | 4 | 0x803AB6D8 | 4 | bit 3 latched at the load |
| `portCarSource` | 0x803A7924 | 224 | 0x803AB6DC | 528 | per load group: port buffers (mods, Q15) |
| `fxRecords` | 0x803A7A04 | 15,680 | 0x803AB8EC | 47,040 | projectiles (980 bytes) |
| `csObjectPool` | 0x803AB744 | 37,888 | 0x803B70AC | 75,776 | coordinate systems (0x128) |
| `soundInstPool` | 0x803B4B44 | 4,620 | 0x803C98AC | 8,448 | sound instances (0x84) |
| `portSoundSortList` | 0x803B5D50 | 140 | 0x803CB9AC | 256 | the sound driver's priority list |
| `weaponArcs` | - | - | 0x803CBAAC | 320 | arcs (only when raised) |
| `weaponShieldPool` | - | - | 0x803CBBEC | 256 | shields (only when raised) |
| `weaponRemoteBombObjects` | - | - | 0x803CBCEC | 64 | remote bombs (only when raised) |
| `weaponRicochetPool` | - | - | 0x803CBD2C | 1,664 | ricochets (only when raised) |
| `weaponFlameEmitters` | - | - | 0x803CC3AC | 512 | flame emitters (only when raised) |
| `weaponSlamRecords` | - | - | 0x803CC5AC | 306 | claw records (only when raised) |
| `renderWorldModels` | - | - | 0x803CC6E0 | 160 | world models (only when raised) |
| `clutAnimDefs` | - | - | 0x803CC780 | 800 | CLUT animation definitions (only when raised) |
| `clutAnimRunning` | - | - | 0x803CCAA0 | 21,200 | running CLUT animations (only when raised) |
| `carPairVectors` | 0x803B5DDC | 2,700 | 0x803D1D70 | 49,152 | pair table [slot][slot][3] |
| `carPairHeadingDeltas` | 0x803B6868 | 900 | 0x803DDD70 | 16,384 | pair table |
| `carPairDirections` | 0x803B6BEC | 2,700 | 0x803E1D70 | 49,152 | pair table [slot][slot][3] |
| `carPairHeadings` | 0x803B7678 | 450 | 0x803EDD70 | 8,192 | pair table (s16) |
| `carPairDistances` | 0x803B783C | 900 | 0x803EFD70 | 16,384 | pair table |
| `hudRadars` | 0x805B83C0 | 3,520 | 0x805F4570 | 4,288 | radar per HUD (0x370; 0x430 with 15 blips) |
| `portCarDmdArea` | 0x803B7BC0 | 2,099,200 | 0x803F3D70 | 2,099,200 | car DMD area (2 MiB + 2 KB; must end below 0x80600000) |
| `portCarBindGroup` | 0x805B9180 | 48 | 0x805F5630 | 48 | step 3's bindings (section 8), after `hudRadars` |

`portCarDmdArea` ends at 0x805B83C0 (default variant) / 0x805F4570 (`TM2_PORT_CARS=16`) / 0x805F4570 (every port define).

**`.port_bss` end** (before step 3; step 3 adds `portCarBindGroup`, 48 bytes, and its code: section 8.8
has the ends with it): default variant 0x805B9180 (`.port_text` ends 0x805B92FC); with
`TM2_PORT_CARS=16` 0x805F5630 (`.port_text` to 0x805F5D50); with every port define (`TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE`,
`TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`) the port sections end at 0x805F72C0: 36,160 bytes below
0x80600000 (Q13/Q14). The linker now refuses port sections past 0x80600000 (`gen_link.PORT_RAM_END`).
The DMD area is 2 MiB (Q14); with 1 MiB every end above is 1 MiB lower. The biggest blocks are the DMD
area and request 31's packet buffers (1,561,600 bytes; 780,800 since request 33-1, `FAR_DISTANCE.md` 6); a
larger DMD area or more pools would pass 0x80600000 (the port moves its own RAM up then, Q14; the port RAM end
is 0x80C00000 since 33-2e).

**Measured by the port (2026-10-13), DMD area:** one car DMD takes 25 to 39 KB (the largest 39,168 bytes);
16 cars took 530 to 562 KB of the 2 MiB. Dark Tooth (61,660 bytes per copy, section 11.5) was not measured.
The area stays 2 MiB (room for 16 Dark Tooths plus 16 large cars, 11.5, and for mid-match loads).

### 7.3 Run-time switches and the port's tables (all 0 = retail)

| what | meaning |
|---|---|
| `portCarOptions` bit 1 (`TM2_PORT_CAR_OPT_HIGH_DMD`, `port_cars.h`) | the car DMDs are packed (by their actual size) from the start of `portCarDmdArea` instead of after the level DMD at 0x80019C40; the level and the TMS files stay low |
| bit 2 (`OPT_FAR_STARTS`) | `startNodePickRandom` uses `portStartNodePick`: the round's first car as retail (one `rand()`), then the untaken start farthest (smallest \|dx\|+\|dy\|, largest) from the taken ones; all taken: the start farthest from the live cars placed this round (HP > 0; request 49, 7.10), ties to the entry picked the fewest times this round, not marked |
| bit 3 (`OPT_POOLS`) | the weapon pools (projectiles, arcs, shields, remote bombs, ricochets, flames, claw records) use their raised size, and the sound allocator may use all `TM2_MAX_SOUND_INSTANCES`. Latched by `carTablesClear` at each load (`portCarPoolsLive`), before the level and car DMDs create the pools' objects: set it before the load and leave it for the match |
| `portCarsPerArena` | > 0: `tournamentOpponentCount` returns it (cars per arena, humans included; the menus subtract the humans) |
| `sessionPortOpponentCars[TM2_MAX_AI_CARS - 9]` | **the opponent table (Q8):** car-select indices of opponent slots 9..15; the record keeps slots 0..8 (its slot 9 and `lives[]` untouched). Read and written through the settings accessors (`TM2_SETTINGS_OPPONENT_CAR`); `settingsClearOpponentCars` sets it to -1. The port fills it before `sessionBindCars` |
| `portCarSource[group]` | per load group (1..32; human i = i + 1, opponent i = humans + i + 1): `{tms, tms_size, dmd, dmd_size}`; a non-zero pointer makes `portCarLoadGroup` copy that buffer instead of reading the disc file (mod cars, mid-match data, Q15) |

Rules for the port: humans + opponents <= `TM2_PORT_CARS` (and opponents <= `TM2_MAX_AI_CARS`,
humans <= `TM2_MAX_HUMANS`: the arrays' bounds); with more than 11 cars set bits 1-3 (the retail load
area holds 7-14 cars, `SIXTEEN_CARS.md` 3.3; the retail pools are sized for 11 cars). Bits 0-3 may be
set separately (D3, Q3).

**Port functions** (`.port_text`, `port_variant_arrays.c`, only with the define): `portStartNodePick(count)`,
`portCarLoadGroup(dst, group, car, dmd, end)` (and the static `portStartGap`); addresses in the build's
`functions.tsv` (kind `port`).

### 7.4 What changed (71 functions with `TM2_PORT_CARS`; 75 since request 33 step 4; 78 since request 49)

Checked mechanically: `tools/check_car_groups.py <link> --expect-functions` (`EXPECTED_CARS`).

| group | functions | change |
|---|---|---|
| limits (constants, strides) | `uaInit`, `carTablesClear`, `carRegisterModel`, `arenaSetup`, `aiCarsSetActive`, `aiCarGet`, `carPairsUpdate`, `carPairVector`, `carPairHeadingDelta`, `carPairDirection`, `carPairHeading`, `carPairDistance`, `screenAiSelect` | 9 / 15 / [15] -> `TM2_MAX_AI_CARS` / `TM2_MAX_CAR_SLOTS`; `carTablesClear` also latches `portCarPoolsLive` |
| settings (Q8) | `sessionGetOpponentCar`, `sessionSetOpponentCar`, `settingsGetOpponentCar`, `settingsSetOpponentCar`, `settingsClearOpponentCars`, `sessionBindCars` | opponent slots 9.. in `sessionPortOpponentCars` |
| load | `sessionLoadArenaAndCars` | every car file through `portCarLoadGroup`; bit 1: DMDs in `portCarDmdArea` |
| texture-slot stride (Q10) | `vramPlaceTexture`, `vramUploadClut`, `bitmapAddRecord`, `modelRemapTextures` | `group * 20` -> `group * TM2_TEX_SLOT_STRIDE` (32): a group with more than 20 images (Dark Tooth, a mod car) no longer overwrites the next group's slots |
| pickers, arena | `tournamentOpponentCount` (its jump table 800CFC38, shared with `carIndexToType`'s, keeps its words), `tournamentPickOpponents`, `startNodePickRandom` | `portCarsPerArena`; past 13 - humans distinct cars a repeat is taken (retail would loop forever); bit 2 |
| placed cars (request 49, 7.10) | `startNodesClearUsed`, `humanCarPlaceAtStart`, `aiCarStart` | `portStartPlacedClear` at the round's clear; `portStartPlacedMark` before (0) and after (1) each car's start pick |
| card screen (Q9) | `sub_8012A910` (`SsInitHot` in `session_runner.cpp`) | more than 9 opponents: the card screen is skipped (the port draws its own line-up) |
| radar | `hudRadarInit`, `hudRadarPlotCar`, `hudDisplayRadar`, `hudRadarSwapBuffers` | record of 0x70 + 64 x blips bytes, cap `2 * TM2_MAX_RADAR_BLIPS + 1` |
| projectiles | `projectilesReset`, `fxRecordGet`, `fxRecordsClear`, `missileSetTrailColor`, `fxRecordFindFree`, `missilesCreateObjects`, `missileTrailsInit`, `fxRecordsUpdate` | loops by `TM2_POOL_LIVE(PROJECTILES)`, bounds by the maximum |
| coordinate systems | `csAlloc` | 256 |
| sound requests (request 33 step 4, section 16.3) | `sub_8012C8DC` (main), `sndPoolReset`, `sndPoolResetAll`, `sndPoolAlloc` | 64 request objects in `portSndObjSlots` (retail: 35 on main's stack); the allocator hands out the first 35 while bit 3 is clear |
| sound | `soundDriverInit`, `soundDriverReset`, `soundInstAlloc`, `soundDriverUpdate` | 64 instances; the allocator hands out the first 35 while bit 3 is clear; the priority list is `portSoundSortList` with bit 3 (or more than 35 active), else the scratchpad as retail (it holds 35 pointers before the hit-detection state at 0x1F8000B0) |
| weapon pools | `weaponArcSlotFind`, `weaponArcsDraw`, `weaponShieldSlotFind`, `weaponShieldsUpdate`, `weaponFireEnergyAttack`, `weaponBombObjectFind`, `weaponDropWrite`, `weaponDropsReset`, `weaponRicochetSlotFind`, `weaponRicochetUpdate`, `weaponFireSelected`, `weaponFlameSlotFind`, `weaponFlameUpdate`, `weaponSlamStateAlloc`, `weaponAssetBind`, `weaponObjectsDetach`, `weaponsRoundReset` | allocators and the load-time object creation by `TM2_POOL_LIVE`, the other loops by the maximum (idle entries are skipped or null) |
| world models, CLUT animations | `renderAddWorldModel`, `renderRemoveWorldModel`, `renderClearModelLists`, `renderScene`, `clutAnimInit`, `clutAnimNewDef`, `clutAnimFindRunning`, `clutAnimFreeSlot`, `clutAnimUpdate` | 40 / 40 / 20 (not switched: an 11-car match never fills retail's) |

Changed jump tables: `arenaSetup`'s (800CF2B0), `weaponFireSelected`'s (800CF10C),
`weaponFireEnergyAttack`'s (800CF134) and `tournamentOpponentCount`'s (800CFC38), every changed word
pointing into its changed function. Two tools learnt shapes they had not met: a rodata section holding
the jump tables of two functions of which only one changes (`build_source_only.changed_sections`,
`verify_reloc.py`: the other function's words are checked as ordinary pointer slots), and a changed
function in a plain `.text` section (`weapon_flame_update.c`).

### 7.5 The opponent-card screen (Q9)

The card screen holds 9 cards (`screenCardQuads` [9] + the template, `screenCardSlotLayouts` for 1..9,
`SIXTEEN_CARS.md` 6.2). With `TM2_PORT_CARS`, `sub_8012A910` returns at once when
`sessionSettings.opponents > 9`: no card is drawn and nothing is written past the 9 cards. With 9 or
fewer it runs as retail.

### 7.6 Retail-equal with the define on (<= 11 cars, `portCarOptions` = 0, `portCarsPerArena` = 0)

Same game logic and `rand()` calls as the variant without the define: every loop over cars runs to the
counts; the pickers and starts are retail's; every weapon pool hands out retail's number of entries and
creates retail's number of objects at the load (so the coordinate-system allocation order is retail's);
the sound allocator hands out the first 35 instances and sorts in the scratchpad. Expected
differences:

- addresses: every array after `aiCars` (the pools, the slot and pair tables) is at another address
  than in the define-off build (both are in port storage), so pointers into them differ by a delta
  (coordinate-system objects held in car records, as in shifted builds, `RELOCATIONS.md` 4);
- the setup loops clear AI records 9..15 and slot entries 15..63 (not compared);
- the pair tables have a row stride of 64; `carPairsUpdate` writes the same values for the same pairs;
- during the load the texture slots sit at `group * 32 + image` in packet buffer 0 (overwritten by the
  first frame, as retail's);
- Mr. Slam's fifth claw record: retail's allocator hands out 5 records from a 4-record table, the 5th
  over the first object pointers of `weaponRicochetPool` (a retail overflow). With the define and bit 3
  clear the 5th record is still placed there (now the moved table's start), so the overflow behaves as
  retail's; with bit 3 set the records stay in their own table.

### 7.7 Mods, drop-in, loaded cars (request 33, Q10, Q15)

What step 2 does to keep these open:

- **One load path:** every car TMS and DMD goes through `portCarLoadGroup(dst, group, car, dmd, end)`:
  the port can hand any group a buffer (`portCarSource`), so a mod car (or a car joining mid-match,
  Q15: no CD read during a battle) needs no C change there. A buffer that does not fit below `end`
  (the EXE start for the low area, the area's end for `portCarDmdArea`) is not copied (the disc file is
  read instead). Files are packed by their actual size (no 64 KB per car).
- **Texture slots** by stride 32 (Q10): any image count 0..31 per group.
- **Car-select indices** stay signed bytes in the settings and the opponent table, so indices from 0x20
  (mod cars, Q10) fit; every per-index lookup on that path is a bounded switch (`carIndexToType`,
  `carFileCode`, `carDisplayName`, `carTmsFileName`/`carDmdFileName` default to -1 / "unknown"), so
  such an index cannot overrun a 15-entry table; the port's mod table (index - 0x20 -> base type,
  buffers) is a later step.
- **Limits by loaded cars:** groups, slots, keys and the DMD area are sized for 32 loaded cars, not 16
  active (spares, the Juggernaut's Minion, joins).

What is **load-time only** (not yet callable mid-match): the install of a car DMD (`assetInstallImage`:
VRAM placement and the texture slots use packet buffer 0 and the VRAM bitmap, which exist only while
loading, `SIXTEEN_CARS.md` 4.1); `carRegisterModel` appends slots and `portCarRegisterRoot` hands out
keys in load order; `carTablesClear`/`portCarGroupsReset` are the only reset. Slots, keys and groups
are **not freed or reused** within a load (a car that leaves keeps its slot, key and group; a mid-match
load would need a free list and an install that keeps its own VRAM state: a later step). No new code
reads `humanCars[0]` (Q13). **Since request 33's drop-in (section 14):** removal, mid-match starts and mid-match loads
are built (`TM2_PORT_CAR_DROPIN`); since its unloading step (14.7) a group nobody drives can be given back, and its
DMD bytes, slots, key, group number and scene objects are reused by later mid-match loads.

### 7.8 Tests for the port

1. Equality (section 7.6) against the variant without `TM2_PORT_CARS` at <= 11 cars, options 0.
2. 16 cars (`portCarsPerArena` = 16 or settings opponents up to 15 with `sessionPortOpponentCars`),
   bits 1-3 set: the load fits (`portCarDmdArea`), every car binds (with `TM2_PORT_CAR_GROUPS` for
   repeats), starts are spread (bit 2), 15 blips per radar, 16 Outlaws tasering / 16 shields / 16 remote
   bombs, 32 ricochets, 48 projectiles at once.
3. Tournament with more than 9 opponents: no card screen, no overflow.
4. A mod/buffer car: `portCarSource[group]` set for a group: the buffer is installed.

### 7.9 Status and proofs

- **Built and proved** (`build.py --source-only --sdk-from-lib --relocatable --port-variant`, `work/build-20261012-211023-769730`; then
  `tools/build_source_only.py` + `verify_reloc.py` per configuration, on the objects of the identical
  build before it, `work/build-20261012-190320-916674`: the variant and its placement check, as `build.py` links them). The retail
  images keep their hashes: hybrid `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` (= E0)
  `f557c1e3…`. `tools/data_audit.py`: 0 mismatches. Every configuration passes its proof against E0 and
  the placement check (humans/views/HUDs forced to 2, every other limit to retail's):

  | configuration | variant sha256 | `port_changed` (placement check) | `.port_bss` / port end |
  |---|---|---|---|
  | no port define (published `out/TM2_SOURCE_PORT.EXE`) | `24245f1e…` | none: still the 25 functions of request 25 (code unchanged; only the moved arrays' addresses) | 80200610 + 3,902,320 / 0x805B92FC |
  | `TM2_PORT_CARS=16` | `94c832df…` | 12 (the run-time and loading changes: `bitmapAddRecord`, `carTablesClear`, `modelRemapTextures`, `sessionLoadArenaAndCars`, `soundDriverUpdate`, `soundInstAlloc`, `startNodePickRandom`, `sub_8012A910`, `tournamentOpponentCount`, `tournamentPickOpponents`, `vramPlaceTexture`, `vramUploadClut`); 71 in the full link | 80200610 + 4,149,280 / 0x805F5D50 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CARS=16` | `e8aa2c11…` | 114 in the full link (47 + 71, 4 shared) | same / 0x805F6250 |
  | + `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` | `e85c071c…` | 121 (+ the 7 of requests 30/31) | same / 0x805F72C0 |

  Changed jump tables with `TM2_PORT_CARS`: 800CF2B0 (`arenaSetup`), 800CF10C (`weaponFireSelected`),
  800CF134 (`weaponFireEnergyAttack`), 800CFC38 (`tournamentOpponentCount`; `carIndexToType`'s words in
  the same section keep their retail targets), and request 25's 800CFCA8; with step 1 also 800CEAE0.
- **`tools/check_car_groups.py`:** `--expect-functions` equals `EXPECTED_CARS` (71) with
  `TM2_PORT_CARS`, the union (114) with both defines; check 1 (no retail per-type table in changed code)
  passes with `TM2_PORT_CAR_GROUPS` and fails on the default variant with `--negative-control`; `--dmd`
  and `--key-model` (now with 32 loaded cars and 32 Minions: keys up to 0x2E0) pass.
- **`config/US/retail_relocs.tsv`, `move_hazards.tsv`** regenerated (`tools/export_retail_relocs.py`,
  groups `cars` and `pools`): 38 new rows, none high; the medium one (`weaponRicochetUpdate` copies 12
  bytes inside a shot) and the stored pointers (`missileTrailsInit` keeps `&fxRecords[i] + 0x14C/0xF4`
  in its records and objects; stack copies in `worldUpdate`, `carPairsUpdate`, `weaponRicochetUpdate`)
  are inside the moved arrays or in changed functions; `soundInstPool` is retail `.data` holding zeros
  (`.port_bss` is zero); `weaponRemoteBombObjects` sits in the $gp area but retail reaches it with
  hi/lo only.
- **Not runtime-tested.** Retail equality at <= 11 cars holds by reading (7.6); 16 cars are the port's
  to run (7.8).

### 7.10 The start picker counts only the cars placed this round (request 49, 2026-10-19)

**The bug (found by the port integrating request 47).** With more cars than start points (`sixteen`: 16 cars, the
arena's list shorter), `portStartNodePick`'s last case ("all taken": the entry farthest from the live cars, HP > 0)
runs for every car past the list at the round's start. It read every car record with HP > 0, including the cars
not placed yet this round, whose records still held their last match's (or the attract demo's, or a previous
build's memory) position and HP. The picks then depended on the memory layout and the history of the records:
the same match started the first car past the list on node 6 in one build and on nodes 55, 21, 64 in the next,
whose `.port_bss` moved. Each build is deterministic, so online (one build everywhere) was not at risk.

**The fix (`TM2_PORT_CARS`, only with `portCarOptions` bit 2; retail's picker unchanged with the bit clear).**

- `portStartPlaced` (`.port_bss`, every variant, after the rollback words; `port_cars.h` struct
  `TM2PortStartPlaced`, 80 bytes): `car[64]` (human h at h, AI car a at 16 + a: 1 = its start ran since
  `startNodesClearUsed`) and `picks[16]` (the picks of each start-list entry since then, every case).
- `startNodesClearUsed` calls `portStartPlacedClear()` (a new round's or a new arena's starts: no car placed).
  The arena loaders' start lists (`startNodesFwy` and the others) and `arenaSetup` (same arena) clear through it,
  and so does the attract demo's `demoRespawnAiCars`.
- `humanCarPlaceAtStart` and `aiCarStart` call `portStartPlacedMark(car, human, 0)` before their
  `startNodePickRandom` (a car being re-placed is not counted from its old position: `humanCarsRestart` at
  `roundStart` re-places the humans that `sessionBindCars` placed) and `portStartPlacedMark(car, human, 1)` after it.
  Every start goes through these two (round starts, AI restarts after a death, the H5 respawns, the drop-ins
  `portCarStartHuman` / `carBindAiSlot`).
- `portStartNodePick`'s "all taken" case counts only the humans and AI cars with `car[]` set and HP > 0. At a round's
  start every entry then has a car on it (all gaps 0); the tie goes to the entry picked the fewest times this round,
  then the lower entry, so the cars past the list spread over the entries in list order (one more car per entry)
  instead of all taking entry 0. Mid-match (H5 respawns and drop-ins: every car placed since the round's start) it
  is the start farthest from the live cars, as before. With no counted car, a random entry as retail (one `rand()`).
  No other case changed; `rand()` calls are unchanged (one in the round's first pick, one when nobody is counted).
- Proofs: `tools/check_car_groups.py --expect-functions` (`EXPECTED_CARS` + the three functions) and
  `tools/check_rollback.py` check 6 with `TM2_PORT_CARS` (the clear, the two marks around each pick, the picker
  reads `portStartPlaced`, no `rand()` in the two port functions).

**(b) Does anything else read a car record before its car is placed in a round?** Traced from `sessionBindCars`
(`arenaSetup`, then the humans' `carRestart` and the AI cars' `carBindAiSlot` -> `aiCarStart`) and `roundStart`
(`humanCarsRestart`) to the first step:

| reader | reads | before its car is placed? |
|---|---|---|
| `startNodePickRandom` (retail, bit 2 clear) | the start list's flags only | no record |
| `portStartNodePick`, untaken case | the taken entries' nodes | no record |
| `portStartNodePick`, all taken | other cars' position and HP | **yes: fixed above** |
| `humanCarPlaceAtStart` | **its own** `stats.ride_height`: `pos[2] = ride_height + node height` is computed before `carStatsLoad` sets the ride height of the new type | yes, its own record: the record's previous car's ride height (0 for a record never used since boot). `RETAIL_BUGS.md` 35 |
| `aiCarStart` -> `aiPlaceOnRouteNode` -> `aiUpdateStuck` | **its own** stuck counters (+0x474, +0x524 `stuck_time`, +0x03D): `aiCarReset` / `aiRouteStateReset` do not clear them, and a stale `stuck_time` >= 24 with +0x474 set takes 1 HP when `timerFrameCount & 5` == 1 | yes, its own record; only after a death while stuck (LIKELY rare); deterministic from the record |
| `aiCarReset` | its own `ride_height` for `pos[2]` | overwritten by `aiPlaceOnRouteNode`: no effect |
| `humanCarsRestart`, `carRestart` | the record's `type` (+0x90) and `carResetWeapons[]` | written by the binder this load: not stale |
| the AI's targets (`aiCarTargets`, `aiGetTargetCar`) | other cars | only in steps (`worldUpdate`), after every placement |
| the radar (`hudRadarPlotCar`, `portHumanRadarPlot`) | other cars | only in `worldUpdate` |
| `carPairsUpdate` (the pairs table) | every slot's position and its previous heading | only in `worldUpdate`; its first call of a round turns the previous round's headings into `carPairHeadingDeltas` (one step's stale delta, deterministic from the state) |
| the port's drop-in starts (`portCarStartHuman`, `portCarStartHumanAt`, `portHumanRespawn`) | through `humanCarPlaceAtStart` / `carBindAiSlot`: as above | the forced "all taken" case now counts only placed cars |
| `rtMainLoop`'s first `worldUpdate` (before the first step) | every car | after `roundStart`: every car is placed |

Nothing between the placements runs a step, so no other reader sees an unplaced car. The two own-record reads
make a round's start depend on the record's history (the previous match's car in that record): each build is
deterministic, and online the host's state at step 0 (the port's lockstep milestone) or a full snapshot covers
them; a fresh start without a state transfer (`TM2_PORT_ROLLBACK` bit `SEED`) should also start from zeroed or
equal records (ONLINE_ROLLBACK.md 10.6).

## 8. Step 3: the car swap (`TM2_PORT_CAR_SWAP`)

Request 32 step 3 (`CAR_GROUPS_PLAN.md` 3, as updated by `PER_CAR_PLAN.md` B and the port's answers
G.3-G.5, Q14, Q15): a record (human i or AI j) leaves its car and drives another loaded car; the
record that drove that car, if any, takes the first one's car. Everything is behind
`--port-define TM2_PORT_CAR_SWAP`, which needs `TM2_PORT_CAR_GROUPS` (`#error` otherwise: the swap
re-binds per-car keys; with type keys a human Minion and the Juggernaut's Minion would be one car).
It works with and without `TM2_PORT_CARS`.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CAR_SWAP [--port-define TM2_PORT_CARS=16] \
    --output work/pv/P4
```

### 8.1 What changes

- **Port code only** (`.port_text`, `port_variant_arrays.c`): 11 exported functions (8.2) and a few
  static helpers.
- **One game function**, `sessionBindCars` 8012BD94 (`screen_card_layout.c`), already changed by step 1:
  under the define it passes each record's group and type through `portCarBindGroupOf` /
  `portCarBindTypeOf`, so a record that was swapped binds to its new car again when modes 3/4 rebind
  every round (G.3). No other game function changes (`port_changed` is the same set as without the
  define; `tools/check_car_groups.py --expect-functions`).
- **Storage** (every variant, `.port_bss`, after `hudRadars`, so nothing before it moves):
  `portCarBindGroup` (48 bytes, `signed char`): the load group each record was swapped to, 0 = the
  settings' group; human i at [i], AI j at [16 + j]. `portCarSetGroup` writes it, `portCarGroupsReset`
  (every load) clears it, `sessionBindCars` reads it. The port may also clear an entry itself.
- **Retail behaviour when nothing is called** (define on, no swap): the table is all 0, both functions
  return their argument, `sessionBindCars` binds exactly as step 1, and no other game code runs differently
  (no swap function calls `rand()` itself either: check 5 of `check_car_groups.py`). The frame-by-frame comparison
  against the build without the define stays equal; the expected differences are the port sections'
  addresses (`.port_text` grows) and nothing else.

### 8.2 API

Addresses in three builds of the 4-player variant (`work/pv/r32c_*_P4/functions.tsv`). They move with
the storage and the other defines: read them from the build's `functions.tsv` (kind `port`) /
`symbols.tsv`. `kind`: 0 human, 1 AI; `index` into
`humanCars` / `aiCars`; records must be bound (index < `humanCarTotal` / `aiCarTotal`).

| function | groups + swap | + `TM2_PORT_CARS=16` | every port define | use |
|---|---|---|---|---|
| `int portCarSetGroup(int human, int index, int group, int hp)` | 0x805BAC34 | 0x805F768C | 0x805F869C | **the swap** (below) |
| `int portCarParkAi(int index)` | 0x805BAF00 | 0x805F7958 | 0x805F8968 | `SURVIVAL.md` 3.1 for AI record j: specials and look reset, dead then HP 0 (no explosion), wreck state 4, out of range, car not drawn and out of the hit pass, body hidden, record and car far below; stays bound to its car. Returns 0 if no such record |
| `int portCarBindGroupOf(int ai, int index, int group)` | 0x805BAABC | 0x805F7514 | 0x805F8524 | `sessionBindCars`: the record's `portCarBindGroup` entry if that group has a car, else `group` |
| `int portCarBindTypeOf(int ai, int index, int type)` | 0x805BAB10 | 0x805F7568 | 0x805F8578 | `sessionBindCars`: the type of that group's car, else `type` |
| `void portCarResetSpecials(int slot)` | 0x805B9AD0 | 0x805F6524 | 0x805F7534 | end every special state holding the car in `slot` (8.3) |
| `void portCarResetLook(int slot)` | 0x805B9E1C | 0x805F6870 | 0x805F7880 | freeze, turbo flame, engine sound of the car; freeze, fire, invisibility of its driver (8.3) |
| `void portCarUnbind(int kind, int index)` | 0x805B9FB4 | 0x805F6A08 | 0x805F7A18 | the record leaves its slot (slot kind 2, index -1) |
| `void portCarBindSlot(int kind, int index, int slot)` | 0x805BA05C | 0x805F6AB0 | 0x805F7AC0 | bind as `carBindHumanSlot` / `carBindAiSlot` without the restart: object and slot tables, record slot, slot kind/index, object id (i + 1 / j + 50), `humanCarTypes` |
| `int portCarApplyType(int kind, int index, int hp)` | 0x805BA150 | 0x805F6BA4 | 0x805F7BB4 | the record takes the type of its slot's car (8.4) |
| `void portCarParkSlot(int slot)` | 0x805BA6BC | 0x805F7110 | 0x805F8120 | a car nobody drives: not drawn, body hidden, far below, slot kind 2 |
| `void portCarRekeyReferences(int slot_a, int slot_b)` | 0x805BA794 | 0x805F71E8 | 0x805F81F8 | references by slot or object exchanged a <-> b; b < 0 clears a's (8.5) |

`portCarBindGroup`: 0x805B9180 (default variant and groups + swap), 0x805F5630 (with `TM2_PORT_CARS=16`,
also with every port define). The 7 shared blocks are the pieces `PER_CAR_PLAN.md` B.2 names, with a slot
or a kind/index pair in place of `TM2PortCarRef`; request 33's removal and mid-match start (B.6) are built
from them (not in this step).

**The swap.** `int portCarSetGroup(int human, int index, int group, int hp)` (human != 0: human
record `index`, else AI record `index`; `group` = a load group as `sessionLoadArenaAndCars` numbers
them: human i = i + 1, opponent i = humans + i + 1; `hp < 0` = scale, `hp >= 0` = set). Returns 1;
0 (nothing changed) when the record or the group's car does not exist, or the record already drives
it. Call it between frames: from the port's frame hook before the car updates (where it calls
`demoRespawnAiCars`' slot), never from inside a car update or a damage callback. Several swaps in one
frame are fine (each reads the tables the previous one wrote).

Sequence (record R on slot a, group g's first slot b, O = the record on b if any):

1. `portCarResetSpecials(a)`, `portCarResetSpecials(b)`, `portCarResetLook(a)`, `portCarResetLook(b)`
   (8.3), while the bindings are still the old ones.
2. Mr. Slam's claw records: with O, R and O exchange their `weapons.slam` pointers (a claw record goes
   with the car type that uses it); without O, a record that becomes Mr. Slam takes an idle record's
   claw record, else `weaponBlockInit` allocates one (retail's allocator).
3. The two cars exchange places: matrix, angles and position of each world object (+0x80..+0xB3) go to
   the other, so each car appears exactly where its new driver's car was; without O the new car takes R's
   car's place.
4. `portCarUnbind` R (and O), `portCarBindSlot(R, b)` and `portCarBindSlot(O, a)`; without O,
   `portCarParkSlot(a)`.
5. `portCarApplyType(R, hp)` and `portCarApplyType(O, -1)` (8.4).
6. Each car shown as its driver is (alive: drawn, body shown, damage stage from the HP percentage and the
   record's stage field set to it; parked: not drawn, body hidden; dead: drawn as the old car was, body
   hidden, stage 2).
7. `portCarRekeyReferences(a, b)` (8.5); `portCarBindGroup` of R = g, of O = a's group.

**What is kept and what is reset** (Q14, G.4, G.5):

| | kept | reset / new |
|---|---|---|
| position, motion, route, AI targets, view | yes (the record's) | the cars exchange places (3) |
| HP | scaled to the new maximum (`hp < 0`, at least 1 while alive) or set (`hp >= 0`, 1..max); a dead record keeps its HP | |
| stats | killer, last attacker (slots, remapped in 7), shield, damageable, restart pending, regen clock, the unnamed per-life fields +0x03, +0x16, +0x28 | the type's tuning (`carStatsLoad`), at the record's own speed multiplier; human `max_hp` + 20; freeze and burn timers; the riding flag `mounts_cars` |
| weapons | pickups `ammo[1..9]`, selected weapon, energy, turbo, held, drop armed, rear fire; specials `ammo[0]` clamped to the new `max_specials` | recharge period, capacity, `max_specials` of the new type (`weaponBlockInit`); invisibility |
| specials | | every special state of both cars ended (8.3) |
| looks | | freeze ice, turbo flame, fire, invisibility fade, engine sound of both cars (8.3) |
| camera / AI | | human: camera offsets of the type (`viewCameraOffsetsInit`); AI: weights and fire chance (`aiCarSetupProfile`, no `rand()`) |
| object ids, `humanCarTypes` | ids follow the record (human i + 1, AI j + 50) | `humanCarTypes[i]` = the new type (the port's HUD) |

### 8.3 Specials and looks (`portCarResetSpecials(slot)`, `portCarResetLook(slot)`)

Each state is ended the way the game ends it itself. Until request 33-2 the specials are retail's single
states, so a state is ended only when it belongs to this car. With `TM2_PORT_CAR_SPECIALS` (section 10) the
function selects the slot's key around the tornado, strike, leap and shockwave blocks, so it ends that car's
own state (duplicates included), and also puts a Mr. Slam's arm and claw back to rest:

| type | state | ended when | how |
|---|---|---|---|
| Mr. Slam | claw records (`weapons.slam`) of every record | it holds this car (`spin_target` = the slot's key), or it is the driver's own | as `carDestroyed`: `carSpinOutById(spin_target, slammer's heading, 0)`, `active` 0, `spin_target` -1, `spin_pending` 0 |
| any | `stats.crush_owner` of every record | = the slot's key | `state.crush_contact` = 0 |
| Twister | `weaponTornadoState` | its object is this car's | `weaponTornadoUpdate`'s end block: timer 0, tornado and funnel off, the car shown (if alive) and its body; sound 0x35 stopped |
| Shadow | `weaponStrikeState` | its weapons block is the driver's | timer and active 0, animation 2325 step 0, strike object off; `ammo[0]` - 1 as the strike's own end (it refunds the special at fire) |
| Grasshopper | `homingLeapState` | its owner slot is this one | active 0, turbo flame off |
| Thumper | `weaponFlameEmitters` | an emitter's object is this car's | its timer 0 (the flame sound stops with the last emitter) |
| Axel | shockwave animation 2324 | the car is an Axel | step 0 |
| Dark Tooth | boss controller | the car is Dark Tooth | shot and swoop counters 0, swoop ended; the head (one 249 object) hidden when its car is parked, shown when a live AI drives it. The stage flag `aiBossDefeatedFlag` is the round rule's and stays. With `TM2_PORT_DARK_TOOTH` (11.3): that car's own controller and head, and its stage back to 1 |

`portCarResetLook`: `carFreezeSetState(key, 0)`, `portCarTurboSetStep(key, 0)`, the type's engine
sound stopped (the new driver's restarts it next frame); the driver's freeze state (only while alive: a
wreck stays frozen), fire (`burning`, `burn_time`, `fire_effect->active`), invisibility (the CLUT fade
back when it ran, body and object shown).

### 8.4 Type change (`portCarApplyType(kind, index, hp)`)

The type is the slot's (`carSlotOwners`), so call it after the binding. It sets what
`humanCarPlaceAtStart` / `aiCarReset` / `weaponBlockInit` set at a start, keeping where the record is
and what it does: `carStatsSetSpeedMult` (the record's own multiplier), `carStatsLoad`, human
`max_hp + 20` / AI `carStatsSetTopSpeed`, the kept fields of 8.2 put back, HP, the ride-height
difference into `motion.pos[2]`, `carWheelsSetStepHeight`, `carBuildCollisionBox`, `weaponBlockInit`
with the kept weapons put back, `viewCameraOffsetsInit` (human) or `aiCarSetupProfile` (AI). It is also
the place for a mod car's stats (`PER_CAR_PLAN.md` E).

### 8.5 References (`portCarRekeyReferences(a, b)`)

State that names a car **by slot or by object** across frames, exchanged a <-> b (with b < 0: cleared
for a, for request 33's removal):

| holder | field | stores |
|---|---|---|
| every record | `stats.killer` (+0x07), `stats.last_attacker` (+0x08) | slot (the kill credit stays the player's) |
| `weaponArcs` | +0x02 | the taser shooter's slot |
| `weaponDrops` (25) | +0x06 | the dropper's slot (mines, remote bomb) |
| projectiles (`fxRecords`) | +0x38 `target` | a car object's position (+0xA8): homing missiles follow the record to its new car (G.5) |
| projectiles | +0x3C `victim` | Roadkill's boomerang: the shooter's car (it flies back to the new one) |
| `weaponShieldPool` records | +0x04 | the energy shield's car |

Not touched, because they follow the record by themselves: object ids (projectile and ricochet owners,
the leap's target, the strike's owner, the drops' +0x04), record addresses (the tornado's `anchor` and
`flags`, the leap's pointers, burning sprites, the arcs' end points), record indices (`aiCarTargets`,
`contact.last`). The contact records' `other` slots (+0x14 of the contact block and of its combined
hit) are consumed within a frame and left. There is no persistent lock-on besides the projectiles'
target (the HUD target and `aiGetTargetCar` are recomputed each frame); `carSetViewMode` and the camera
cache no car object. `carPairHeadingDeltas` sees one frame's heading jump.

### 8.6 The Juggernaut in calls

Load one extra opponent, Minion (car-select index 13), as opponent m: its AI record is m, its load group
`gM = humans + m + 1`.

1. After every `sessionBindCars` (the first one, and each round of modes 3/4): `portCarParkAi(m)`. The
   bindings of earlier swaps survive the rebind (`portCarBindGroup`), so record m is on whichever car it
   was given; `sessionBindCars` restarts it there and the call parks it again. The rebind restarts every
   car with the full HP of its type: give the Juggernaut its HP again after it (step 2's note).
2. Player X becomes the Juggernaut: `portCarSetGroup(1, X, gM, hp)`. X takes the Minion at X's place;
   the parked record m takes X's car and stays parked (hidden, far below). `hp` = the Juggernaut's HP,
   clamped to Minion's maximum (+20); for more, set `stats.max_hp` and `stats.hp` after the call (human
   record +0x186 and +0x1F0, AI record +0x536 and +0x5A0).
3. The Juggernaut changes from Y to X: **first** `portCarSetGroup(1, Y, Y + 1, -1)` (Y gets its own
   car back from record m, which takes the Minion and stays parked), **then** step 2 for X. In the
   other order X would take the Minion from Y and Y would get X's car (a two-way swap).
4. Y and X keep their pickups, energy and turbo; specials count clamped; kill credits by slot are
   remapped, so a kill made just before the swap still counts for its player.

An AI record can be the Juggernaut the same way (`portCarSetGroup(0, j, gM, hp)`), and two active
records can trade cars (`portCarSetGroup(1, X, group of Z's car, -1)`: Z gets X's car).

### 8.7 Limits

- **Shared specials stay shared** until request 33-2 (per-key special state; built for all but Dark Tooth in
  section 10, `TM2_PORT_CAR_SPECIALS`): without it, a Juggernaut that is a
  second car of a type with a single special (Mr. Slam's arm and claw angles, Twister's tornado,
  Shadow's strike, Grasshopper's leap, the Dark Tooth head) shares it with the other car of the type;
  `carStatsLoad` puts the one Mr. Slam arm and claw back to rest when a record becomes Mr. Slam (a
  second Mr. Slam's arm in mid-swing jumps). The CLUT (and the invisibility fade) is per type until
  request 33-1, or per car with step 4's banks (section 9); the engine sound per type until 33-3. Team
  colours: the player now drives the new group's car, whose body CLUT is its type's (repainting it
  repaints every car of the type until 33-1; with banks it is that car's own, `TEXTURE_BANKS.md` 6.5).
- **Dead records:** a swap of a dead record is allowed (its HP stays, its wreck timer runs on the new
  car, which shows damage stage 2 with the body hidden); swapping at the respawn gives the cleanest look.
- **Parked records** (`SURVIVAL.md` 3.1) stay parked when they get a car; the port starts one again
  with `aiCarStart(&aiCars[j], carSlotOwners[aiCars[j].slot], 1)` (the slot's type), as
  `SURVIVAL.md` 3.2 describes, and the car's draw flag and body as there.
- **Claw records:** with `portCarOptions` bit 3 clear the pool is retail's (4 + the overflow record);
  a swap moves claw records with the type instead of allocating, so it adds no allocation except for a
  record that becomes Mr. Slam on a car nobody drove while no idle record holds one.
- **Not runtime-tested** (static proofs only, 8.8). Port scenarios: `PER_CAR_PLAN.md` B.5 and
  `CAR_GROUPS_PLAN.md` 3.5 (swap with turbo on, frozen, burning, invisible, mid-slam, mid-tornado,
  mid-leap, mid-strike; a homing missile in flight; kill credit after a swap; swap back; repeated swaps
  across rounds in modes 3/4; human <-> parked, AI <-> parked, human <-> human).

### 8.8 Status and proofs

- **Built and proved** (`build.py --source-only --sdk-from-lib --relocatable --port-variant`,
  `work/build-20261012-221455-680707`; then `tools/build_source_only.py` + `verify_reloc.py` per configuration, on the objects of
  the identical build before it, `work/build-20261012-211023-769730`). The retail images keep their
  hashes: hybrid `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`.
  `tools/data_audit.py`: 0 mismatches. Every configuration passes its proof against E0 and the placement check (humans/views/HUDs
  forced to 2, every other limit to retail's):

  | configuration | variant sha256 | `port_changed` | port end |
  |---|---|---|---|
  | no port define (published until step 4; now `ae34528c…`, section 9) | `b3388570…` | none (the 25 functions of request 25; `.port_bss` + 48 bytes) | 0x805B932C |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SWAP` | `b66d45fc…` | 47, step 1's set | 0x805BB1A0 |
  | + `TM2_PORT_CARS=16` | `46ced43a…` | 114, steps 1 and 2 | 0x805F7BF8 |
  | + `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` (every port define) | `a5b7872c…` | 121 | 0x805F8C70: 29,584 bytes below 0x80600000 |

  The swap code is about 6.5 KB of `.port_text` (6,488 bytes with groups alone, 6,568 with
  `TM2_PORT_CARS=16`). Without
  `TM2_PORT_CAR_GROUPS` the define stops the build (`#error` in `car_groups.h`, checked with the
  preprocessor).
- **`tools/check_car_groups.py`** (check 5 new): with `TM2_PORT_CAR_SWAP` the 11 functions are in
  `.port_text`, `sessionBindCars` calls `portCarBindGroupOf`/`portCarBindTypeOf`, and no port function the
  swap reaches calls `rand()` (of the 20 game functions it calls, `carSpinOutById`, `objRegistrySetStep`,
  `carFreezeSetState` and `animSetStep` can reach `rand()` inside the game, as at the same events there: a
  held car let go spins out, a random animation steps); `--expect-functions` treats the define as adding
  no game function (47 / 114 / 121 as above); checks 1, `--dmd`, `--key-model` pass as before.
- **CHECK items of the plans that step 3 relies on** (all read in code):
  - `PER_CAR_PLAN.md` A.2 row 2 (is `weapons.slam` cleared between loads?): **yes**: `carTablesClear`
    zeroes it in every record each load (human +0x810, AI +0x850 = weapons +0x48) and resets the claw
    cursor, so no record keeps a claw record from an earlier match; no extra clear is needed;
  - B.3 step 7 (credits): `stats.killer` (+0x07) and `stats.last_attacker` (+0x08) hold **slots** in human
    and AI records alike (one stats block; `carApplyDamage` stores the attacker's slot, `ai_steps.c`
    compares `last_attacker` with the AI's own slot); `carStatsLoad` resets both to -1, so the type change
    keeps them and the fixer remaps them;
  - B.7.3 (`s16` stores of keys): `spin_target` is `s16`, `crush_owner` `s32`: keys up to 0x2F0 fit;
  - `CAR_GROUPS_PLAN.md` 3.3 (`carSetViewMode` 800E7380 caches no object): **confirmed**: it copies
    positions into the record's camera fields; the camera objects are per view; view 7 reads the watched AI
    record each frame;
  - `CAR_GROUPS_PLAN.md` 1.7 (`homingLeapState.owner_id` is the leaping car's slot, the +0x20 of the tornado
    view): confirmed in both views (the leap is ended by slot);
  - B.3 step 1 (Minion's second root): `portCarSlotOfGroup` returns a group's first slot, the root that
    registered first, as the binders use it;
  - G.5 (lock-ons): the only persistent lock is a projectile's target (+0x38, a car object's position);
    the HUD target and `aiGetTargetCar` are recomputed every frame. The census of holders (8.5) found four
    the plan did not list: the taser arcs' shooter slot, the drops' dropper slot, the shields' source car
    and Thumper's flame emitters' car (ended with the specials);
  - `PER_CAR_PLAN.md` A.3 (`soundPlay`'s vtable slot): not needed, the swap stops engine sounds with
    `soundStop` (every request of the id).
- **Not runtime-tested.** Retail equality with no swap holds by construction (8.1); the swap scenarios are
  the port's (8.7).

## 9. Step 4: texture banks (`TM2_PORT_TEXTURE_BANKS`)

Request 32 step 4 (`CAR_GROUPS_PLAN.md` 4, `TEXTURE_BANKS.md` 5) with request 33's wishes (nothing shared
between two cars of a type, mid-match loads of any type, mod cars: `PER_CAR_PLAN.md` B.7, C.1, E, G.1,
G.10). The full description, the API with addresses and the PSY-Z protocol are in **`TEXTURE_BANKS.md` 6
("As built")**; this section is the summary for the car-group side.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_TEXTURE_BANKS [--port-define TM2_PORT_CARS=16] \
    [--port-define TM2_PORT_CAR_SWAP] --output work/pv/P4
```

### 9.1 What it does

- Needs `TM2_PORT_CAR_GROUPS` (`#error` otherwise). Run-time switch `portCarOptions` bit 5 (0x20),
  latched at each load by `bitmapInit`; clear = retail logic (`TEXTURE_BANKS.md` 6.6).
- With bit 5: each car load group n gets a bank (the lowest free one: n in a normal load); its images and
  CLUTs are placed there with retail's scan in a fresh bank; no image is shared with another car (a
  duplicate has its own copy); an image the arena's TMS holds by key keeps the arena's coordinates in bank
  0 (retail look, arena animations followed). The slot's tpage bits 9..15 hold the bank;
  `modelRemapTextures` copies them into each primitive; the port's GPU samples bank `attr >> 9`.
- Uploads: `portVramUploadBank` holds the bank of the `LoadImage`/`StoreImage` in progress (set before,
  reset to 0 after the `DrawSync`): `vramPlaceTexture`, `vramUploadClut`, `gpuLoadImageLine`,
  `gpuStoreImageLine`, `portTexLoadImage`.
- The invisibility fade is one CLUT animation per car key (id `key + 8`: retail's `type + 8` for the first
  car of a type) in the bank of the CLUT it fades: one car's invisibility no longer fades its twin (6.Q6
  answered with banks on). `portCarResetLook` (step 3) ends the car's own fade.
- Wreck, body (shadow) and debris already go by key since step 1 (`carWreckModelGet(key)`,
  `carBodyObjectShow(key)`); their meshes are the group's own DMD copy, remapped with its bank: nothing more
  to change (`TEXTURE_BANKS.md` 5 item 4).
- `portCarGroupNoReuse[group]` (the port's, u8 per load group): the group neither takes an image by key
  nor gives one (mod cars, `PER_CAR_PLAN.md` E.1 / G.10), with or without banks.
- Per group, published after the load: bank (`portTexBanks.group_bank`), body CLUT id and its bank
  (`body_clut`, `body_bank`) for team paint.
- Mid-match: `portTexBankAlloc(group)` / `portTexBankFree(group)`; banked placement needs no VRAM bitmap
  (`TEXTURE_BANKS.md` 6.7 for what the later mid-match step still needs).

### 9.2 What changed

8 game functions (`tools/check_car_groups.py --expect-functions`, `EXPECTED_BANKS`): `bitmapInit`,
`vramPlaceTexture`, `vramUploadClut`, `modelRemapTextures`, `clutAnimRegister`, `gpuLoadImageLine`,
`gpuStoreImageLine`, `weaponInvisibilityUpdate` (also step 1's; `vramPlaceTexture`, `vramUploadClut` and
`modelRemapTextures` are also step 2's). 11 port functions (`BANKS_PORT`), table in `TEXTURE_BANKS.md`
6.4. Storage (every variant, after `portCarBindGroup`): `portTexBanks` (1,232 bytes), `portVramUploadBank`
(4), `portCarGroupNoReuse` (64).

### 9.3 Duplicates and request 33-1

With banks on, every car (duplicates included) has its own pixels and CLUTs, its own fade and its own body
CLUT for team paint, so `PER_CAR_PLAN.md` C.1 (per-car CLUTs) is only the fallback for matches with bit 5
clear; recommendation and the pieces C.1 would reuse: `TEXTURE_BANKS.md` 6.7.

### 9.4 Status and proofs

Built and proved statically; the builds, hashes, port ends and checks are in `TEXTURE_BANKS.md` 6.9.
Port sections with every port define end at 0x805F9850 (26,544 bytes below 0x80600000). Not runtime-tested:
the port's tests are in `TEXTURE_BANKS.md` 6.10.

## 10. Request 33 step 2a: per-car specials (`TM2_PORT_CAR_SPECIALS`)

Request 33 step 2 (`PER_CAR_PLAN.md` C.2, decisions D5-D7, the port's answers G.2/G.9) for every special with
a single retail state **except Dark Tooth** (33-2e: section 11): Mr. Slam's arm and claw, Twister's tornado,
Shadow's strike, Axel's shockwave and Grasshopper's leap (`PER_CAR_PLAN.md` A.2 rows 1-5 and 7), plus the claw
records' overflow (row 2). Behind `--port-define TM2_PORT_CAR_SPECIALS`, which needs `TM2_PORT_CAR_GROUPS`
(`#error` otherwise); works with and without `TM2_PORT_CARS`, `TM2_PORT_CAR_SWAP` and `TM2_PORT_TEXTURE_BANKS`.
Without the define every changed line is the retail C.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CAR_SPECIALS [other port defines] --output work/pv/P4
```

### 10.1 How it works

- **Storage per car key (D5).** The first car of a type (key = type) keeps the retail global at its retail
  address; a duplicate (overflow key `(16 + d) << 4`, `CAR_GROUPS.md` 1) has entry d of `portCarSpecials`
  (`.port_bss`, 32 entries = every overflow key of step 2's 48 key indices).
- **Selection instead of rewritten accessors.** The game code that reads and writes the state stays retail's.
  It runs in the context of one car: `portCarSpecialSelect(key)` exchanges the retail storage of the key's
  special (chosen by the key's type) with the key's entry, so the retail globals hold that car's state;
  `portCarSpecialRestore(token)` exchanges it back. `portCarSpecials.sel[part]` remembers which entry is in the
  retail storage, so selections nest; a key below 0x100 selects the first car's own state. Between selections
  the retail globals always hold the first car's state and each duplicate's state is in its entry.
- **Retail-equal without duplicates, no run-time bit needed (D7 confirmed).** Every key is a type, so every
  selection is a no-op (no exchange): retail's code on retail's storage, at the retail addresses, in retail's
  order; no `rand()` call is added, removed or moved (the selection functions call none:
  `check_car_groups.py` check 8). The only differences from the define-off build are the extra calls and
  the port sections' addresses. The claw-record allocator hands out retail's records with retail's cursor
  for the first four allocations (10.4), which is all a match without duplicates (and without swaps of more
  than four records onto Mr. Slam) uses.
- **Per-frame updates without a car (D6).** `weaponTornadoUpdate`, `weaponHomingLeapUpdate` and
  `weaponShadowStrikeUpdate` keep their retail call sites in `worldUpdate`. Each is now a small loop at the
  retail address: retail's body (moved to `.port_text` as `...One`) for the first car, then, for each
  overflow key of the type handed out in this load (`portCarGroups.key_type`), in key order, select, body,
  restore (`portCarSpecialNext`). An inactive state returns at the body's first test, as retail.

### 10.2 What is per car now

| special | state (retail VA, bytes) | registered by (per load key) | used inside a selection by | fixed for duplicates |
|---|---|---|---|---|
| Mr. Slam (0xC0) | `slamArmObject` 80180F74, `slamClawObject` 80180F78, `slamArmAngle` 80180F7C, `slamClawAngle` 80180F80 (16) | `slamArmRegister`, `slamClawRegister` (`portCarLoadKey()`) | `weaponSlamSwingUpdate` / `weaponSlamGrabUpdate` (selected by `humanCarTick` / `aiCarDrive` with the slammer's key; `weaponSlamCarryTarget` lifts the held car by that arm), `carStatsLoad` 0xC0 (the record's key, `portCarKeyOfStats`) | each Mr. Slam's crane moves; a second slam no longer jumps the first's arm; the held car follows its own slammer's arm; a duplicate's (re)start resets its own arm |
| Twister (0xA0) | `weaponTornadoState` 80184DC0 (0x14: timer, kind, angle, anchor, flags, object), `weaponTornadoObject` 80180F38 (asset 2323), `weaponTornadoFunnelObject` 80180F3C (root 160) | `weaponAssetBind` 0x913 / 0xA0 | `weaponFireSpecial` 0xA0 (the firer's key), `weaponTornadoUpdate` (loop) | a second Twister fires while the first spins; each tornado and funnel is its own car's objects, so the funnel draws **its own car's model** (DMD copy, damage stage, and with texture banks its bank) |
| Shadow (0xD0) | `weaponStrikeState` 80184E10..80184E74 (0x64: state, `weaponStrikeCornerOffsets`, `weaponStrikeOwner`, strike object), `objRegistryId2325` 80181068 (its animation) | `weaponAssetBind` 0x915, `objRegistryAdd` 2325 (`portCarSpecialRegister`) | `weaponFireSpecial` 0xD0, `weaponShadowStrikeUpdate` (loop) | a human Shadow's second press detonates **its own** strike, never another Shadow's; the AI's fire is no longer refused while another strike runs; each strike animates its own object |
| Axel (0xB0) | `objRegistryId2324` 80181064 (shockwave animation, AX.DMD asset 2324) | `objRegistryAdd` 2324 | `weaponFireSpecial` 0xB0 (`objRegistrySetStep(0x914, ..)`) | the shockwave animates on the firing Axel's own model (and bank) |
| Grasshopper (0x60) | `homingLeapState` 80184DD4 (0x3C, inside the 0x50 block at 80184DC0) | (none) | `weaponFireSpecial` 0x60 → `weaponHomingLeapInit`, `weaponHomingLeapUpdate` (loop) | a second leap no longer overwrites the first mid-air; each leap's turbo flame is its car's (step 1) |
| Mr. Slam's claw records | `weaponSlamRecords` (retail 80184818, 4 x 18; raised table with `TM2_PORT_CARS`), cursor `weaponSlamStateCount` 80180AF0 | `weaponSlamStateAlloc` | `weaponsRoundReset` (via `portCarClawRecord`) | records past the table come from `portCarSpecials.claw` (32): never the overlapping fifth record over `weaponRicochetPool` (`RETAIL_BUGS.md` 17), no sharing before 4 + 32 (or 17 + 32) allocations |

Not changed, and why:

- **Thumper's flame emitters**: a pool whose emitters hold their car's object (per car already); step 2 raised
  it to 16 with `portCarOptions` bit 3 (G.9). With bit 3 clear the retail 2 stay (a third Thumper flaming at
  once gets none, as retail). The flame sound 47 is one id (one loop while any emitter runs: acceptable,
  `PER_CAR_PLAN.md` A.2 row 8).
- **Weapon models by asset id** (row 12: identical data), the tornado's scratch vectors (used within one call).
- **Dark Tooth** (rows 6, 14): 33-2e, section 11 (`TM2_PORT_DARK_TOOTH`).
- `worldUpdate`, `weaponObjectsDetach`, `carPartsReset`: unchanged. `carPartsReset` and `weaponObjectsDetach`
  clear the first cars' objects at each load; the duplicates' entries are cleared just before, by
  `portCarSpecialsReset` from `portCarGroupsReset` (`carTablesClear`, the first call of `arenaResetSystems`).

### 10.3 What changed (13 game functions, 8 port functions, 3 moved bodies)

| VA | function | unit | change under the define |
|---|---|---|---|
| 80105A7C | `weaponFireSpecial` | `weapon_basic.c` | the firer's key selected around the type switch (also step 1's) |
| 8010E040 | `weaponTornadoUpdate` | `weapon_tornado.c` | loop (D6); body `weaponTornadoUpdateOne` in `.port_text` (also step 1's) |
| 8010F520 | `weaponHomingLeapUpdate` | `weapon_leap_homing_update.c` | loop; body `weaponHomingLeapUpdateOne` (also step 1's) |
| 8010F9C0 | `weaponShadowStrikeUpdate` | `strike_update.c` | loop; body `weaponShadowStrikeUpdateOne` |
| 800DC88C | `humanCarTick` | `car_tick.c` | the car's key selected around `weaponSlamSwingUpdate` / `weaponSlamGrabUpdate` (also step 1's) |
| 800E7EDC | `aiCarDrive` | `ai_driver.c` | the same (also step 1's) |
| 80115188, 801151D0 | `slamArmRegister`, `slamClawRegister` | `arena_load_register.c` | the load key selected around the registration |
| 80109038 | `weaponAssetBind` | `asset_pools.c` | the load key selected around the switch (also step 2's) |
| 80123F78 | `objRegistryAdd` | `object_registry.c` | ids 2324 / 2325: `portCarSpecialRegister(id, obj)` (also steps 1 and 2's) |
| 800FE15C | `carStatsLoad` | `car_stats.c` | 0xC0: the arm and claw rest angles set with the record's key selected (`portCarKeyOfStats`) |
| 80108CF0 | `weaponsRoundReset` | `round_reset.c` | the claw-record loop through `portCarClawRecord`; then `portCarSpecialsRoundReset()` (also step 2's) |
| 8010EE64 | `weaponSlamStateAlloc` | `weapon_tables.c` | the pool of 10.4 (also step 2's) |

`weaponSlamSwingUpdate`, `weaponSlamGrabUpdate`, `weaponSlamCarryTarget`, `weaponHomingLeapInit` (C++), the slam
getters/setters and `objRegistrySetStep` are unchanged: they run inside their callers' selections. The plan's
per-key accessors (`portSlamArmGet/Set`, a 6th parameter for `weaponSlamCarryTarget`, the setters moved out of
`carStatsLoad`) were not needed. Retail bodies are moved to `.port_text` only for the three loops (D6); the
other selections are in place, so the big bodies stay in the EXE (a first attempt with wrappers for every
function put 18 KB of bodies in `.port_text` and passed 0x80600000 with every define).

Port functions (`.port_text`, `port_variant_arrays.c`; `include/semantic/car_specials.h`):

| function | groups + specials | every port define | use |
|---|---|---|---|
| `int portCarSpecialSelect(int key)` | 0x805BBB88 | 0x805F9608 | the key's special state into the retail storage; returns a token (0: the key's type has none) |
| `void portCarSpecialRestore(int token)` | 0x805BBC18 | 0x805F9698 | undo that selection (selections nest) |
| `int portCarSpecialNext(int type, int *index)` | 0x805BBC48 | 0x805F96C8 | the D6 loops: select the next duplicate key of a type, 0 when none |
| `void portCarSpecialRegister(int id, void *object)` | 0x805BBCC8 | 0x805F9748 | `objRegistryAdd` 2324 / 2325 by the load key |
| `int portCarKeyOfStats(void *stats)` | 0x805BBD34 | 0x805F97B4 | `carStatsLoad`: the key of the record holding a stats block, 0 none |
| `void *portCarClawRecord(int index)` | 0x805BBDC8 | 0x805F9848 | claw record i of the pool (10.4) |
| `void portCarSpecialsReset(void)` | 0x805BBE24 | 0x805F98A4 | `portCarGroupsReset` (each load): every entry and selection cleared |
| `void portCarSpecialsRoundReset(void)` | 0x805BBE78 | 0x805F98F8 | `weaponsRoundReset`: the duplicates' part |
| static `weaponTornadoUpdateOne` | 0x805BC490 | 0x805FBF5C | retail's body of `weaponTornadoUpdate` (`weapon_tornado.c`) |
| static `weaponHomingLeapUpdateOne` | 0x805BBFD8 | 0x805FBAA4 | retail's body of `weaponHomingLeapUpdate` |
| static `weaponShadowStrikeUpdateOne` | 0x805BD2FC | 0x805FCDC8 | retail's body of `weaponShadowStrikeUpdate` (`strike_update.c`) |

With `TM2_PORT_CAR_SWAP`, `portCarResetSpecials(slot)` (8.3) now selects the slot's key around its tornado,
strike, leap and shockwave blocks (so it ends **that car's own** state, duplicates included) and puts a Mr.
Slam's arm and claw back to rest. `portCarRekeyReferences` needs nothing new: the per-key states name their
own car (ended by the reset: the tornado's object, anchor and flags, the leap's owner slot, the strike's
weapons block) or other cars by object id / record pointer, which follow the records (8.5: the leap's target,
the strike's owner id).

### 10.4 Claw records (`weaponSlamStateAlloc`)

Retail hands out 5 records from a 4-record table (the fifth over `weaponRicochetPool`'s first object pointers)
and wraps to 0, so a fifth Mr. Slam record overflows and a sixth shares record 0 (`RETAIL_BUGS.md` 17). Step 2
raised the table to 17 behind `portCarOptions` bit 3 and kept retail's overflow with bit 3 clear (7.6). Under
`TM2_PORT_CAR_SPECIALS` the pool is `weaponSlamRecords` (retail's 4 at the retail address, or the raised table
of `TM2_MAX_SLAM_RECORDS`, **whatever bit 3**) followed by the 32 records of `portCarSpecials.claw`; the cursor
counts up to 4 + 32 - 1 (17 + 32 - 1) and then wraps. The first four allocations return retail's records with
retail's cursor; a match needs a fifth only with five Mr. Slam records (duplicates, or swaps onto a Mr. Slam
nobody held a record for), so no run-time bit is used. `weaponsRoundReset` resets the records it handed out
through `portCarClawRecord(i)`. Records are only ever allocated by Mr. Slam records (`weaponBlockInit` 0xC0,
`carRestart`, `aiCarStart`, `carCarCollision` with a record already holding one, `weaponFireSpecial` 0xC0),
once per record and load (`carTablesClear` zeroes `weapons.slam`).

### 10.5 Update order and the swap

- Fire: `weaponFireSelected` → `weaponFireSpecial` selects the firer's key for the type switch.
- Car ticks (each car in retail's order): `humanCarTick` / `aiCarDrive` select the slammer's key around its
  slam updates.
- `worldUpdate` (retail's order): tornado (first Twister, then duplicates by key), leap (likewise),
  strike (likewise). Two tornadoes catching one car: the later in key order sets its position last.
- Round reset (`roundStart`, `carRestart`): retail's part, then every duplicate's (`portCarSpecialsRoundReset`:
  leap active and age, tornado timer and objects off, strike active and timer, each duplicate Mr. Slam's arm
  and claw to rest).
- Load: `portCarGroupsReset` clears every entry and the selections; registration by `portCarLoadKey()`.
- Swap (`TM2_PORT_CAR_SWAP`): `portCarSetGroup` → `portCarResetSpecials(a)`, `(b)` end both cars' own states;
  `portCarApplyType` → `carStatsLoad` resets the arm of the record's new car when it is a Mr. Slam. Call the
  swap between frames (8.2), never inside a selection.

### 10.6 Storage

`portCarSpecials` (`.port_bss`, every variant, after `portCarGroupNoReuse`, so nothing before it moves;
`config/US/port_variant_arrays.tsv` group `one`; 7,392 bytes = 0x1CE0; written by the C only):
0x805B96C4 in the default variant (`out/TM2_SOURCE_PORT/symbols.tsv`) and with groups + specials; 0x805F5B74 with `TM2_PORT_CARS=16` (every port define). It moves with storage added before it: read it from the build's `symbols.tsv`. The retail globals keep their retail addresses (mapped through `addrmap.tsv` in a shifted build).

| offset | field | contents |
|---|---|---|
| 0x000 | `sel[5]` | per part (0 Mr. Slam, 1 Twister, 2 Shadow, 3 Axel, 4 Grasshopper): the entry now in the retail storage, d + 1; 0 = the first car's (always 0 between selections) |
| 0x020 | `slam[32]` | arm, claw (objects), arm angle, claw angle |
| 0x220 | `tornado[32]` | state (0x14 bytes of 80184DC0), tornado object, funnel object |
| 0x5A0 | `strike[32]` | the 0x64-byte block of 80184E10, the 2325 animation object |
| 0x12A0 | `axel_anim[32]` | the 2324 animation object |
| 0x1320 | `leap[32]` | the 0x3C bytes of 80184DD4 |
| 0x1AA0 | `claw[32]` | claw records past `weaponSlamRecords` (18 bytes) |

Entry d belongs to key `(16 + d) << 4`; the port reads a duplicate's state there (not while the game runs a
selection: never inside a frame callback the game calls from a special update).

### 10.7 Tests for the port

1. Equality with no duplicates (`portCarOptions` 0, <= 11 distinct cars): frame by frame against the variant
   without the define. Car records, `rand()` calls and the drawn scene must match; the special state stays at
   its retail addresses.
2. Two (or more) of each type, at once: two Mr. Slams grabbing at once (each crane moves, each held car follows
   its own arm, a second slam does not jump the first); two Twisters spinning at once (each funnel shows its own
   car's model and damage stage, with banks its own colours); human Shadow + AI Shadow striking at once (the
   human's second press detonates only its own strike); two Axels (each shockwave on its own car); two
   Grasshoppers leaping at once (both land, each turbo flame its own); five or more Mr. Slams (no claw record
   over the ricochet pool: ricochets still work).
3. Rounds (modes 3/4) and restarts mid-special: every car's tornado, strike, leap and arm reset.
4. With `TM2_PORT_CAR_SWAP`: swap mid-slam, mid-tornado, mid-strike, mid-leap on a duplicate: only that car's
   special ends.

### 10.8 Status and proofs

- **Built and proved** (`tools/build_source_only.py` + `verify_reloc.py` per configuration, the variant and its
  placement check as `build.py` links them; then the full `build.py --source-only --sdk-from-lib --relocatable
  --port-variant`, `work/build-20261013-020430-499980`). The retail images keep their hashes: hybrid `9219f7ba…`, source-only
  `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`. Every configuration passes its proof against E0 and
  the placement check (humans/views/HUDs forced to 2, every other limit to retail's):

  | configuration | variant sha256 | `port_changed` (full link) | port sections end |
  |---|---|---|---|
  | no port define (published `out/TM2_SOURCE_PORT.EXE`) | `9e0c6fdb…` | none (code unchanged; `.port_bss` + 7,392 bytes) | 0x805BB52C |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SPECIALS` | `d192d87f…` | 54 (step 1's 47 + 7) | 0x805BD814 |
  | + `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`, `TM2_PORT_TEXTURE_BANKS` | `134fc0f4…` | 123 | 0x805FC2D4 |
  | + `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` (every port define) | `eb40dc76…` | 129 | 0x805FD344: 11,452 bytes below 0x80600000 |

  `TM2_PORT_CAR_SPECIALS` adds about 7.4 KB of storage and 3.3 KB of port code plus the three moved bodies
  (5.8 KB). Without `TM2_PORT_CAR_GROUPS` the define stops the build (`#error` in `car_specials.h`).
- **`tools/check_car_groups.py`** (check 8 new): with the define the 8 selection functions and the 3 moved
  bodies are in `.port_text` and none of them calls `rand()`; every function that relocates into the retail
  special state (`SPECIAL_STATE`: 7 ranges) is the mechanism (`portCarSpecialExchange`,
  `portCarSpecialRegister`), the first cars' own path (`weaponsRoundReset`, `weaponObjectsDetach`,
  `carPartsReset`), dead (`weaponHomingLeapState`; `sub_80114DD0` as a caller), or reached only through
  selecting callers (19-20 functions, all ok in the three define builds). `objRegistrySetStep` counts only for
  calls with `$a0` 0x914 / 0x915 or an unreadable `$a0`; the target-event callers that pass a target record's
  id are listed (`SPECIALS_OTHER_IDS`). Negative control: on a build without the define (`--negative-control`)
  check 8 fails with 14 unselected paths on the default variant (e.g. `weaponTornadoUpdate <- worldUpdate <- rtMainLoop`).
  `--expect-functions` equals the union of the steps' lists (`EXPECTED_SPECIALS`, 13); checks 1, 5, 6,
  `--dmd`, `--key-model` pass as before.
- **Not runtime-tested.** Retail equality with no duplicates holds by construction (10.1); the duplicate
  scenarios are the port's (10.7).

## 11. Request 33 step 2e: Dark Tooth per car (`TM2_PORT_DARK_TOOTH`)

Request 33 step 2e (`PER_CAR_PLAN.md` A.2 rows 6 and 14, C.2 33-2e, the port's answer G.6). Goal: up to 16 Dark
Tooths in one match, each with its own head and boss state; with one Dark Tooth, retail behaviour exactly.
Behind `--port-define TM2_PORT_DARK_TOOTH`, which needs `TM2_PORT_CAR_SPECIALS` (`#error` otherwise; that one
needs `TM2_PORT_CAR_GROUPS`). A separate define (not part of `TM2_PORT_CAR_SPECIALS`) so a build with only
33-2a stays as it was and the port can test the two steps apart. Works with and without `TM2_PORT_CARS`,
`TM2_PORT_CAR_SWAP` and `TM2_PORT_TEXTURE_BANKS`. Without the define every changed line is the retail C.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CAR_SPECIALS --port-define TM2_PORT_DARK_TOOTH \
    [other port defines] --output work/pv/P4
```

### 11.1 What is per car now

The selection mechanism of 10.1 with a sixth part (`portCarSpecialPart(0xF0)`): the first Dark Tooth (key 0xF0)
keeps the retail globals at their retail addresses, a duplicate (key `(16 + d) << 4`) has
`portCarSpecials.dark_tooth[d]`, and `portCarSpecialSelect(key)` exchanges the two for the time of a call.

| state (retail VA, bytes) | what | registered / used inside a selection by |
|---|---|---|
| `aiDarkToothShotDelay` .. `aiDarkToothSwoopTarget` 80180B4C (7 x s16) | missile delay and timer, firing side, swoop timer, head yaw and pitch steps, the human swooped at | `aiDarkToothBossUpdate` (selected by `aiWeaponFireDecide` with the car's key) |
| `darkToothSwoopDelay`, `darkToothSwooping`, `darkToothHoverHeight`, `darkToothSwoopTarget` 80180F18 (0x10) | swoop state | the same |
| `darkToothBossObject` 80180F40 (4) | **the head** (asset 249: a scene object `csAlloc`ed per BB.DMD install) | `weaponAssetBind` 0xF9 (already selected by the load key since 33-2a: each install's head goes to its own car), `aiDarkToothBossUpdate`, `aiDarkToothBossModel` / `aiDarkToothBossObject` (via `carApplyDamage`) |
| `darkToothSwoopVectors` 80183F64 (24) | the swoop's push | `aiDarkToothBossUpdate` |
| `aiBossDefeatedFlag` 8018102C (4) | **the stage**: 1 = stage 2 (the head on the car) | `aiBossDefeated()` from `aiDarkToothBossUpdate` (hover height), `carApplyDamage` (the debris of a final death, the head hidden), `carContactApplyObject` (through `portCarBossDefeated(key)`); `aiCarsCountRemaining` (retail's rule, aiCars[0]'s key; bit 6's loop), `worldUpdate` (`portCarDarkToothRoundEnd`) |

So each Dark Tooth fires its own missiles on its own timer, sways and swoops its own head at the human nearest
to **it**, has its own stage (its head comes down onto its own car in stage 2) and its own final death (its own
head's debris, its own head hidden). Retail: one head (the last BB.DMD's), the others standing still where they
were created, the timers run once per Dark Tooth per frame, one stage for all (`SURVIVAL.md` 1.6).

Retail's rules kept (bit 6 clear):
- the boss update runs on level 8 only (for every Dark Tooth there, each on its own state);
- the stage-2 change is retail's: level 8, all AI cars down, `aiCars[0]` is a Dark Tooth: `aiCars[0]`'s own
  stage (its key selected), the round ended at once, Hong Kong's position clamps; other Dark Tooths have no
  stage 2 (they die as normal cars, with retail's endless debris bursts of wreck state 3, `SURVIVAL.md` 1.4);
- the round end (`worldUpdate`): the shorter delay (210) and the head hidden when `aiCars[0]`'s car is in stage 2;
- `arenaSetup` (each `sessionBindCars`) clears the stage: the first car's (retail) and every duplicate's
  (`portCarDarkToothClearStages`);
- the scalars keep their values across loads and rounds for the first Dark Tooth (retail never resets them); a
  duplicate starts each load from retail's initial values (0, 10, 0, 10, 150, 100, 0; the rest 0).

### 11.2 `portCarOptions` bit 6 (`TM2_PORT_CAR_OPT_BOSS_ANYWHERE`, 0x40): Dark Tooth as a boss anywhere

`SURVIVAL.md` 1.5 and 1.6 (the port's answer G.6), taken from the port's overlay (`portConfig.boss_anywhere`,
`decomp/patches/0001-port-hooks.patch`), now per car. Read live each frame, except by `bitmapInit` (read at each
load). Clear = the retail rules above. Bits 0-3 are step 2's, 4 is engine sound's (33-3, section 12), 5 is the
texture banks'.

| # (`SURVIVAL.md` 1.5) | function | with bit 6 |
|---|---|---|
| 1 | `aiWeaponFireDecide` | the boss update (missiles, swoop, head) for every AI Dark Tooth on every arena, machine guns off (retail: level 8 only) |
| 2 | `aiCarsCountRemaining` → `portCarDarkToothStages` | **per-car stage loop** (1.6): every AI Dark Tooth except the tournament boss (level 8 and `aiCars[0]`, which keeps retail's code) that is down (HP <= 0, not parked: wreck state != 4) and in stage 1 **counts as remaining** (the round does not end while it waits), spins out while its wreck timer >= 121, and at 0 enters stage 2 where it is: stage 1 → 2, `carStatsDarkToothStage2` (HP 110), `carBuildCollisionBox`, alive. No `rtScheduleEvent(3, 0)` (the round goes on), no Hong Kong clamps. The truck stays hidden (its death hid it); the head comes down onto the car (hover -24). This runs every frame, also while other AI cars are alive (retail's stage check runs only when all are down) |
| 3 | `aiDarkToothJumpSelect` | Hong Kong's jump zones only on level 8 (elsewhere: both flags cleared, return) |
| 4 | `aiFollowRouteTarget` | Hong Kong's warps 1 and 2 only on level 8 |
| 5 | `carContactApplyObject` | the type-0xF0 tail (the boss markers: warp 2, spin-outs) only on level 8 |
| 6 | `carApplyDamage` → `portCarDarkToothWreck` | a Dark Tooth's **final** death (in stage 2) sets wreck state 4 (out of play) instead of 3, so `carWreckUpdate` does not burst the wreck again and again; not for the tournament boss (retail's 3) |
| 7 | `weaponProjectileUpdate` → `portCarDarkToothShooter` | the type-61 missile wobble on every arena (with or without bit 6 the test is now "any Dark Tooth's car object", retail: the first Dark Tooth's; the same with one) |
| — | `vramUploadClut` | VRAM rows 240..255 (x 0..639; the port's answer F.1: nothing draws there) give CLUTs more room, so Dark Tooth fits beside more car types without banks (`SURVIVAL.md` 4.1 "rows freed"): a CLUT upload that finds no room in the upper half takes a spot there before the lower half. **Changed by request 33-1 (section 13.3):** until then `bitmapInit` reserved only rows 0..239 with bit 6; now one allocator owns the rows for bit 6 and 33-1's CLUT copies, and every car keeps its retail row |
| 8 | `roundStart` | unchanged: the Hong Kong route patch needs level 8 and `aiCars[0]` a Dark Tooth (retail) |

The music (`audioSetDarkToothMusic`) stays the port's choice. What bit 6 does **not** do: start a Dark Tooth in
stage 2 ("a head as its own enemy", `SURVIVAL.md` 1.6) — the port does that with `portCarDarkToothStage(slot, 1)`
plus `carStatsDarkToothStage2`, `carBuildCollisionBox` and hiding the truck (`csSetDrawEnable(obj, 0)`) after the
car starts; a human Dark Tooth gets no boss update and no stage 2 (as retail: both are AI code).

**Challenge Match and stage 2.** Challenge Match leaves bit 6 off, so a Dark Tooth there stays in stage 1 for the
whole match (retail's stage-2 change needs level 8 with `aiCars[0]` a Dark Tooth: the tournament boss). Stage 2
comes only with bit 6 (Survival: the per-car stage loop) or as the retail level-8 boss. A "stage 2 as a separate
car" (the head as its own opponent) could later use the mod-car mechanism (`PER_CAR_PLAN.md` E: a car of its own
load group whose files the port hands over) instead of the stage flag.

### 11.3 What changed (10 game functions, 7 port functions; 11 and 10 since request 33-1, section 13)

| VA | function | unit | change under the define |
|---|---|---|---|
| 80103CA0 | `aiWeaponFireDecide` | `weapon_tick_init.c` | the car's key selected around `aiDarkToothBossUpdate`; level 8 or bit 6 |
| 8011C248 | `aiCarsCountRemaining` | `scene_init.c` | bit 6: `+= portCarDarkToothStages()`; retail's tail with `aiCars[0]`'s key selected |
| 8011A700 | `worldUpdate` | `world_update.c` | the round end through `portCarDarkToothRoundEnd()` |
| 800E5628 | `carApplyDamage` | `car_damage.c` | the dying Dark Tooth's key selected around the final-death block; wreck state from `portCarDarkToothWreck` (also step 1's) |
| 800E06CC | `carContactApplyObject` | `car_object_contact.c` | `portCarBossDefeated(key)` at the 3 tests; bit 6: the tail only on level 8 |
| 8011995C | `arenaSetup` | `arena_setup.c` | `portCarDarkToothClearStages()` after retail's clear (also step 1/2's) |
| 800DB110 | `weaponProjectileUpdate` | `projectile_hit.c` | the wobble test: `portCarDarkToothShooter(p->owner)`, level 8 or bit 6 |
| 800F6DDC | `aiDarkToothJumpSelect` | `car_restart.c` | bit 6: return unless level 8 |
| 800F7D54 | `aiFollowRouteTarget` | `screen_pause_menu.c` | bit 6: the two warp branches only on level 8 |
| 80121550 | `bitmapInit` | `bitmap_init.c` | `portCarClutsReset()`: latch bit 6 for the rows 240..255 allocator (also step 4's; until request 33-1: bit 6 → `bitmapMarkRect(0, 0, 640, 240)`) |
| 80120D3C | `vramUploadClut` | `scene_init.c` | since request 33-1: bit 6, after the upper half: `portClutRowsUpload` (section 13.3) |

Unchanged: `aiDarkToothBossUpdate` (runs inside its caller's selection), `aiBossDefeated`,
`aiDarkToothBossModel` / `aiDarkToothBossObject`, `weaponAssetBind` (33-2a's selection by load key now covers
0xF9), `weaponObjectsDetach` (clears the first car's head at each load; the duplicates' entries are cleared by
`portCarSpecialsReset` just before), `roundStart`.

Port functions (`.port_text`, `port_variant_arrays.c`; `include/semantic/car_specials.h`):

| function | groups + specials + DT | every port define | use |
|---|---|---|---|
| `int portCarBossDefeated(int key)` | 0x805BC8D4 | 0x805FA354 | `aiBossDefeated()` of the key's car |
| `int portCarDarkToothWreck(void *ai)` | 0x805BC910 | 0x805FA390 | `carApplyDamage`: 3, or 4 after a final death with bit 6 |
| `int portCarDarkToothStages(void)` | 0x805BC988 | 0x805FA408 | bit 6: the per-car stage loop; returns the Dark Tooths waiting |
| `int portCarDarkToothRoundEnd(void)` | 0x805BCAE8 | 0x805FA568 | `worldUpdate`: `aiCars[0]`'s stage (its head hidden when set) |
| `void portCarDarkToothClearStages(void)` | 0x805BCB5C | 0x805FA5DC | `arenaSetup`: the duplicates' stages 0 |
| `int portCarDarkToothShooter(int owner)` | 0x805BCB80 | 0x805FA600 | the missile wobble: is `owner` a Dark Tooth's car object id |
| `int portCarDarkToothStage(int slot, int stage)` | 0x805BCC4C | 0x805FA6CC | **for the port**: read (stage < 0) or set the stage of the Dark Tooth in car slot `slot`; returns the old stage, -1 if the slot holds none. Between frames only |

Static helpers: `portSwapHalves` (the 7 scalars) and, with `TM2_PORT_CAR_SWAP`, `portCarDarkToothHead(key)`.

**Swap (`TM2_PORT_CAR_SWAP`).** `portCarResetSpecials(slot)` (8.3) now resets the Dark Tooth block **inside** the
slot key's selection, so it ends that car's own controller (shot and swoop timers 0, swoop ended), and its stage
goes back to 1 (the record that drives it next gets stage-1 stats from `portCarApplyType`). `portCarResetLook`,
`portCarParkSlot` and `portCarParkAi` show or hide **that car's** head (`portCarDarkToothHead(key)`), not the
first Dark Tooth's. Without the define the swap keeps step 3's behaviour.

### 11.4 Storage

`portCarSpecials` (10.6) grows by 32 entries of 0x40 bytes at +0x1CE0 (now 9,440 bytes = 0x24E0, every variant,
so the default variant's later `.port_bss` moves by 2 KB); `sel[5]` is the Dark Tooth part (the struct's
`sel[6]` took one word of the padding: offsets 0x000..0x1CDF are unchanged). Address: 0x805B96C4 with groups +
specials (+ DT) and in the default variant; 0x805F5B74 with every port define (read it from the build's
`symbols.tsv`).

| offset in an entry | field | retail storage |
|---|---|---|
| +0x00 | `scalars[7]` (s16) | `aiDarkToothShotDelay` .. `aiDarkToothSwoopTarget` 80180B4C |
| +0x10 | `swoop[4]` | `darkToothSwoopDelay` block 80180F18 |
| +0x20 | `head` | `darkToothBossObject` 80180F40 |
| +0x24 | `vectors[6]` | `darkToothSwoopVectors` 80183F64 |
| +0x3C | `stage` | `aiBossDefeatedFlag` 8018102C |

Entry d is key `(16 + d) << 4`; `portCarSpecials.dark_tooth[d].head` is that Dark Tooth's head object. The port
reads them between frames (never inside a callback the game makes from a selection).

### 11.5 16 Dark Tooths: what it takes, and what still limits it

- **Defines:** `TM2_PORT_CARS=16` is needed besides the three: 16 AI records (retail 9) and the texture-slot
  stride 32 (`SURVIVAL.md` 4.2: with stride 20 a second Dark Tooth's images 0..10 land on the first one's slots
  20..30). 16 Dark Tooths + 1 human = 17 loaded cars (of 32), 17 car slots (of 64), 15 overflow keys (of 32).
- **Memory:** one BB.DMD copy per Dark Tooth (61,660 bytes; the head and the ricochet model are inside it): 16 =
  986,560 bytes. That needs `portCarOptions` bit 1 (the 2 MiB DMD area, `portCarDmdArea`): 16 Dark Tooths and
  16 large cars (39,168 each) fit (1.61 MB). In retail's load area after the level only 2-5 fit (`SURVIVAL.md`
  4.3). Per Dark Tooth the port storage adds nothing beyond the fixed 2 KB of `portCarSpecials.dark_tooth`.
- **VRAM:** without banks the duplicates reuse the first Dark Tooth's images and CLUTs by key (no VRAM, one look
  for all). With texture banks (bit 5) every Dark Tooth has its own bank (about 80,000 texels with its CLUTs;
  16 banks of the 127). Banks are PC-side pages of the port's GPU (`TEXTURE_BANKS.md` 6.8), so they cost no game
  RAM. Next to other car types without banks: `SURVIVAL.md` 4.1 (bit 6's free rows 240..255 help).
- **Coordinate systems:** each Dark Tooth takes its car object, its head (`weaponAssetBind` 0xF9 `csAlloc`) and
  its parts; 128 in retail, `TM2_MAX_COORD_SYSTEMS` 256 with `TM2_PORT_CARS=16` (GUESS: enough; the port
  measures).
- **Projectiles:** each Dark Tooth fires a missile every 10..75 updates. 16 share the projectile records: 16 in
  retail (with bit 3 clear a full pool refuses the shot, as retail), 48 with `portCarOptions` bit 3. Use bit 3.
- **CLUT animations:** BB.DMD registers two (+8, id 248). Without banks the duplicates' fades are refused (one per
  type, as retail: their bodies still hide and show); with banks one definition per car: 16 Dark Tooths need
  `clutAnimDefs` 40 / `clutAnimRunning` 20 (bit 3 with `TM2_PORT_CARS`).
- **Sounds:** the swoop sounds 61 / 66 and the burning sound 6 are one id each (same-id requests merge): 16 heads
  swooping at once sound as one. Dark Tooth has no engine sound.
- **Targets:** every head swoops at the human nearest to its own car (`humanCarNearest`), missiles target humans
  (`weaponFindTargetAhead/Behind`), as retail: 16 Dark Tooths all hunt the humans.
- **Hong Kong:** on level 8 every Dark Tooth runs the Hong Kong jump and warp code (map data valid there); only
  `aiCars[0]` has retail's stage-2 reveal unless bit 6 gives the others the per-car loop.
- **Waves / AI lives** (`AI_LIVES.md`): a Dark Tooth waiting for its stage 2 is dead with HP 0; if the port's
  respawn restarts it (`aiCarStart` reloads stage-1 stats) before its wreck timer runs out, it comes back as a
  truck. Park it (wreck state 4) or let the stage loop finish first (GUESS: not run).

### 11.6 Status and proofs

- **Built and proved** (`tools/build_source_only.py` + `verify_reloc.py` per configuration, the variant and its
  placement check as `build.py` links them, with the default port RAM end 0x80C00000; then the full
  `build.py --source-only --sdk-from-lib --relocatable --port-variant`, `work/build-20261013-040710-824661`). The retail images keep their
  hashes: hybrid `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`. Every
  configuration passes its proof against E0 and the placement check:

  | configuration | variant sha256 | `port_changed` | port sections end (RAM end 0x80C00000) |
  |---|---|---|---|
  | no port define (`out/TM2_SOURCE_PORT.EXE` as published at 33-2e; now 12.9) | `a4045ae8…` | none (code unchanged; `.port_bss` + 2,048 bytes) | 0x805BBD30 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SPECIALS` (33-2a only) | `7e688c7a…` | 54 (as before) | 0x805BE014 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SPECIALS` + `TM2_PORT_DARK_TOOTH` | `39715ec0…` | 62 (54 + 8: the 10 of 11.3 less `carApplyDamage`, `arenaSetup`) | 0x805BE528 |
  | + `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`, `TM2_PORT_TEXTURE_BANKS` | `7a9f0ed5…` | 129 | 0x805FD048 |
  | + `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` (every port define) | `6a961fbe…` | 136 | 0x805FE0B8 (also below the old 0x80600000, by 8,008 bytes) |

  `TM2_PORT_DARK_TOOTH` adds 2 KB of storage (in every variant) and about 1.2 KB of port code.
- **`tools/check_car_groups.py`** (check 8 extended): with `TM2_PORT_DARK_TOOTH` the Dark Tooth state
  (`DARK_TOOTH_STATE`: 5 ranges, 11.1) is checked with the specials' state: every function that relocates into it
  is the mechanism (`portCarSpecialExchange`), the first car's path (`arenaSetup`, `weaponObjectsDetach`) or reached
  only through selecting callers (`aiDarkToothBossUpdate`, `aiBossDefeated`, `aiDarkToothBossModel` / `Object`; the
  selecting `aiCarsCountRemaining`, `aiWeaponFireDecide`, `carApplyDamage`, `weaponAssetBind` and the port functions);
  the 7 port functions are in `.port_text` and call no `rand()`; `--expect-functions` adds `EXPECTED_DARK_TOOTH` (10).
  All OK in the three Dark Tooth builds. Negative control: on the 33-2a-only build (`--negative-control`) the Dark
  Tooth state fails with 5 unselected paths (`aiBossDefeated <- carContactApplyObject <- ...`, `aiCarsCountRemaining`,
  `worldUpdate`, `aiDarkToothBossModel`, `aiDarkToothBossObject`). Limitation (as for 33-2a): the check works per
  function, so a function that selects anywhere counts as selecting.
- **Retail equality with one Dark Tooth** holds by construction: its key is 0xF0, every selection is a no-op, the
  wobble test equals retail's with one Dark Tooth (type-61 missiles come only from `aiDarkToothBossUpdate`), the
  round-end and stage reads are the retail global; with bit 6 clear no level test changes. The only differences
  from the define-off build are the extra calls and the port sections' addresses.
- **Not runtime-tested.** Port tests: (1) one Dark Tooth, bit 6 clear: the level-8 boss fight frame by frame
  against the variant without the define; (2) two to sixteen Dark Tooths on Hong Kong and on another arena with
  bit 6 (each head over its own truck, each firing on its own timer, each its own stage 2 and final death, the
  round ending after the last); (3) bit 6 on Paris/Swamp without banks (rows 240..255) and with banks;
  (4) with `TM2_PORT_CAR_SWAP`: swap a record off a Dark Tooth mid-swoop and in stage 2 (that car's controller and
  stage reset, the other Dark Tooths untouched).

## 12. Request 33 step 3: per-car engine sound (`TM2_PORT_CAR_SOUND`)

Request 33 step 3 (`PER_CAR_PLAN.md` A.2 row 11, A.3, C.3; the port's answer G.7 and the user's change of
2026-10-13: **no cap by default**, engines below the effects when the voices run out). Goal: every car's engine is
its own voice (pan, pitch and distance its own; one car stopping does not stop another of its type); the same
for Thumper's flame sound and, with `TM2_PORT_DARK_TOOTH`, Dark Tooth's swoop sounds. Behind
`--port-define TM2_PORT_CAR_SOUND`, which needs `TM2_PORT_CAR_GROUPS` (`#error` otherwise), and the run-time
`portCarOptions` bit 4 (`TM2_PORT_CAR_OPT_ENGINE_SOUND`, 0x10, read live at each `worldUpdate`). Bit 4 clear:
the retail calls, word for word (made by the port functions). Works with and without every other port define.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CAR_SOUND [other port defines] --output work/pv/P4
```

### 12.1 What retail does (CONFIRMED by reading)

- `carSoundUpdate` (8011DB74) picks the engine id from the car type (0x24..0x3C even, Sweet Tooth 0x3C, Minion
  0x41; Dark Tooth none) and every update calls `soundPlay(id, distance, pan, pitch)` (instance 0) while the car
  is alive, else `soundIsActive(id)` → `soundStop(id)`; `worldUpdate` stops the id of an AI out of range.
- The sound manager (`soundMgrPlay` 80128890) keeps one request list per id (`soundManagerObject + id * 20 +
  0xCC`); `soundMgrInitLists` gives the engine ids, the flame (47) and swoop 66 lists **1** request (61: 3). With
  the list full a new request of a merging kind (0..9: engines are kind 1, the flame kind 3) is merged into the
  request of the same instance id, else into the farthest one (`sndListMergeById`); kinds 12/13 (the swoops)
  are dropped while the list is full.
- So two cars of a type are one request: its distance, pan and pitch jump between the cars every update, and
  one car dying or leaving range stops the other's engine. The flame: a second Thumper's flame merges into the
  first one's request. The swoops: a second Dark Tooth's warning is dropped while the first one plays.

### 12.2 What changes with bit 4

- **Instances.** Car slot s plays its sounds as **instance s + 1** of the id (`soundPlayVariant`, the request's
  `+0x2C`): engine, flame (47) and, with `TM2_PORT_DARK_TOOTH`, the swoops (61, 66) from each Dark Tooth's own
  controller. Instance 0 (retail's) is never used, so a request always belongs to one car.
- **Merge by instance first** (`portSoundPlayInstance`). Before a new request is made, a request of the same
  instance in the id's list is updated in place exactly as `sndListMergeById` does (distance, pan, pitch, extra
  word; queued to start; the id's active flag set), so each car keeps one request however often it plays. The
  re-queue is skipped when the request is already in this frame's pending queue (the queue holds 45 and
  `worldUpdate` runs twice per frame: 20 engines queued twice would fill it and push out effects).
  Only kinds 0..9 merge; others (the swoops) are submitted as retail would.
- **List sizes** (set at each `worldUpdate` while bit 4 is on; back to retail's when it goes off): engine lists
  2 × (cap, or humans + AI records with no cap) (room for requests that are still stopping), flame 2 ×
  `TM2_MAX_FLAME_EMITTERS`, swoops `TM2_PORT_SWOOP_VOICES` (4, with `TM2_PORT_DARK_TOOTH`).
- **Stops by instance.** A car's engine stops when it dies, leaves range (AI), loses its place under the cap,
  or its slot's sound id changes (a swap): `soundStopInstance(id, s + 1)`. A slot no record drives any more
  (unbound or parked by the swap) is stopped by `portCarSoundFrame`. The flame stops per car when that car's
  last emitter ends (`portCarFlameSoundEnd`); when no emitter runs, sound 47 is stopped as retail
  (`portCarFlameSoundIdle`). `soundIsActive` stays one flag per id (retail's); with bit 4 the code tracks
  playing instances in `portCarSound` instead.
- **Request pool.** Sound requests come from a fixed pool of 35 (`sndObjPool`, `shell_main.c`'s stack; not
  raised by `TM2_PORT_CARS`). A **new** engine instance is refused while 12 or fewer are free
  (`TM2_PORT_SOUND_POOL_RESERVE`), so engines never take the last requests from the effects; merging needs no
  request.
- **Bit 4 turned off mid-match:** every instance this code started is stopped, the lists get retail's sizes,
  and the next update plays retail's instance-0 requests.

### 12.3 The cap: `portEngineVoices`

`portEngineVoices` (.port_bss word, **0 = no cap, the default**: every car's engine plays). With N > 0 only the
engines of the N cars nearest to any human play, recomputed at each `worldUpdate` (`portCarSoundFrame`):
candidates are the cars that have an engine to play (a human, or an AI in range / in mode 0; alive; a type with
an engine); distance = Chebyshev distance to the nearest human car (every human, so every view); a car that is
playing ranks at 3/4 of its distance (no swapping every frame at the edge); ties by record order (humans
first). A car past N is stopped (the driver's normal stop; no fade). Values above 64 mean no cap. With
`TM2_PORT_HUMANS` (`HUMANS16_PLAN.md` H3, request 37 answer 12) the distance is to the nearest **local** human
(the listeners: humans with a local slot on this machine); a remote human's car is still a candidate like any car.

### 12.4 Voices: the 24-voice limit and who drops (CONFIRMED by reading; counts from `SCREEN\UACORE.VAB`)

- The PS1 SPU has **24 voices**; the game's driver (`soundDriverInit`, `soundVoiceLimit` = 24,
  `soundVoiceInUse[24]`) hands them out; music is CD audio (no SPU voice). Each request sounds every tone of its
  VAB program, one voice per tone: engines **1 voice** each except 0x24 (type 0x20) **2** and 0x36 (Axel)
  **3**; the flame (47) 3; swoops 61: 2, 66: 4. So 16 single-tone engines (+ the humans') take 16-20 of the 24
  voices and leave **4-8 for everything else**; 16 Axels would want 48.
- **Stealing** (`soundDriverUpdate` 80161318, game code): each frame the instances refreshed by their request
  are sorted (`soundSortInstances`) and every one past position 24 is killed at once (keyed off, freed; its
  request re-allocates next frame and is sorted again). Instances that played on without a refresh keep a
  voice only if fewer than 24 were refreshed. A killed instance's voice becomes free only after the SPU's
  release ends (`soundVoiceAlloc` checks `SpuGetKeyStatus`), so a new effect may start a frame or two late.
- **Retail's order:** a quicksort by the request's distance (+0x70), then a second quicksort by the tone's
  VagAtr priority (+0x02, read by `SsUtGetVagAtr` from the VAB). Every one of the VAB's 124 tones has priority
  0, and the quicksort is not stable, so retail's victim is close to arbitrary: an explosion can drop while a
  far engine keeps its voice.
- **With bit 4** `soundSortInstances` calls `portCarSoundSortInstances` (game-side code, no SDK change): a stable
  sort with every non-engine instance first (by VagAtr priority, then distance), then the engine instances by
  distance. So when more than 24 are wanted, **the farthest engines drop first**, effects only when the
  effects alone need more than 24. Without a cap and with 16+ engines the far engines come and go as effects
  start and end (each frame the tail is cut).
- **More voices: done by request 34** (`SPU_VOICES.md`). The voice count was fixed in three places: the
  hardware (24), libsnd/libspu (the voice tables sized 24, 24-bit masks; their exact modules linked from the
  `.LIB`), and the game's driver (`soundVoiceLimit`, `soundVoiceInUse[24]`, `1 << voice` masks). Now:
  - the port's SPU has 48 voices (voices 24..47 in a second register block at 0x1F802C00);
  - `--port-define TM2_PORT_SPU48` builds libspu and libsnd from the C with 48 voices (`SpuSetVoiceAttr`,
    `SpuGetKeyStatus`, `SpuSetNoiseVoice`, `SsUtKeyOnV`, `SsUtKeyOffV`, `SsUtSetVVol` and the voice manager;
    a libspu mask with bit 31 set names voices 24..47; the `.LIB` modules are replaced by the C through
    `config/US/sdk_c_modules.tsv`);
  - the driver has `soundVoiceInUse[48]` and `soundVoiceLimit` **44** (`TM2_SOUND_VOICES`): voices 44..47 are
    left to the port's menu sounds (`TM2_PORT_SOUND_VOICES_RESERVED`, default 4; 0 gives the driver all 48).
  - The sort and the cap of this section (bit 4, `portEngineVoices`) work on `soundVoiceLimit`, so with the
    define the farthest engines drop only past 44 voices: 16 single-tone engines leave 28 voices for effects,
    and 16 Axels (48 wanted) still lose some. Without the define everything above holds (24).

### 12.5 What changed (5 game functions, + 1 with `TM2_PORT_DARK_TOOTH`; 8 port functions)

| VA | function | unit | change under the define |
|---|---|---|---|
| 8011DB74 | `carSoundUpdate` | `screen_card_draw.c` | the engine block → `portCarEngineUpdate(slot, snd, !dead, distance, pan, pitch)` (slot: human +0x004, AI +0x006) |
| 8011A700 | `worldUpdate` | `world_update.c` | `portCarSoundFrame()` first; the out-of-range stop → `portCarEngineUpdate(slot, sound, 0, ...)` |
| 8010A5F0 | `weaponFlameStart` | `car_restart.c` | `soundPlay(47, ...)` → `portCarFlameSound(slot, distance, pan)` |
| 8010A7A0 | `weaponFlameUpdate` | `weapon_flame_update.c` | an emitter's end → `portCarFlameSoundEnd(car object)`; the final stop → `portCarFlameSoundIdle(active)` |
| 801618C8 | `soundSortInstances` | `engine_sound_driver.c` | bit 4: `portCarSoundSortInstances` (engines last) |
| 8010FD44 | `aiDarkToothBossUpdate` | `ai_dark_tooth_boss.c` | with `TM2_PORT_DARK_TOOTH` too: the swoop sounds → `portCarSoundPlay(61/66, 0, 0, 99, boss->slot)` |

Unchanged: `soundMgrPlay` (the merge-first is done before it, in the port code), `sndListSubmit`,
`sndListMergeById`, `soundMgrInitLists` (the port code sets the list sizes), the SDK. The swap's
`portCarResetLook` (with `TM2_PORT_CAR_SWAP`) stops the slot's own instances (`portCarSoundStopSlot`) instead of
the type's id while bit 4 is on.

Port functions (`.port_text`, `port_variant_arrays.c`; `include/semantic/car_sound.h`):

| function | groups + sound | every port define | use |
|---|---|---|---|
| `void portCarSoundFrame(void)` | 0x805BC668 | 0x805FAD5C | `worldUpdate`, first: latch bit 4 (off: stop every started instance, retail list sizes), list sizes, the cars allowed (all, or the nearest N), orphan slots stopped |
| `void portCarEngineUpdate(int slot, int snd, int on, int vol, int pan, int pitch)` | 0x805BCACC | 0x805FB1E8 | `carSoundUpdate` / `worldUpdate`: the slot's engine instance; bit 4 clear = retail's calls |
| `void portCarSoundPlay(int snd, int vol, int pan, int pitch, int slot)` | 0x805BCC40 | 0x805FB35C | `soundPlay` as the slot's instance (the swoops); bit 4 clear = `soundPlay` |
| `void portCarFlameSound(int slot, int vol, int pan)` | 0x805BCC8C | 0x805FB3A8 | `weaponFlameStart`: sound 47 per car |
| `void portCarFlameSoundEnd(void *model)` | 0x805BCCFC | 0x805FB418 | `weaponFlameUpdate`: an emitter ended; the car's flame stops when it has no other |
| `void portCarFlameSoundIdle(int active)` | 0x805BCE04 | 0x805FB520 | `weaponFlameUpdate`: retail's stop of 47 when no emitter runs |
| `void portCarSoundStopSlot(int slot)` | 0x805BCE8C | 0x805FB5A8 | **for the port** (and the swap): stop the engine and flame instances of car slot `slot`; between frames |
| `int portCarSoundSortInstances(void **a, int lo, int hi)` | 0x805BCF10 | 0x805FB62C | `soundSortInstances`: engines last; 0 while bit 4 is not live |

Static helpers: `portCarSoundOfType`, `portSoundIsEngine`, `portSoundPlayInstance`, `portCarSoundOff`,
`portSoundDist`.

### 12.6 Storage and words

| word | size | groups + sound | every port define | |
|---|---|---|---|---|
| `portCarSound` | 208 | 0x805BBBA4 | 0x805F8054 | state (12.6); also in the default variant at 0x805BBBA4 |
| `portEngineVoices` | 4 | 0x805BBC74 | 0x805F8124 | the cap: 0 = none (default), N > 0 = the N nearest; read at each `worldUpdate`; default variant 0x805BBC74 |
| `portCarOptions` bit 4 (0x10) | - | - | - | per-car sound instances, read at each `worldUpdate` |

`portCarSound` layout: +0x00 `live` (bit 4 as applied), +0x04 `voices` (cap in use, 0 = none), +0x08
`candidates`, +0x0C `allowed`, +0x10 `engine[64]` (the sound id slot s plays as instance s + 1, 0 = none),
+0x50 `allow[64]`, +0x90 `flame[64]`. Defined in every variant after `portCarSpecials` (the default variant's
`.port_bss` grows by 212 bytes). Read the addresses from the build's `symbols.tsv`.

### 12.7 What is still shared

- `soundIsActive` per id (one flag; with bit 4 the port code no longer relies on it for these ids).
- Since request 33 step 4 (section 16) the human loops of players 2.. and Twister's tornado sound are per car, the
  special-fire sounds' lists hold 4 with bit 4, and the request pool is raised with `TM2_PORT_CARS`. What is still
  shared after that: section 16.5.
- The 24 voices (12.4; 44 for the game with `TM2_PORT_SPU48`, `SPU_VOICES.md`).

### 12.8 Tests for the port

(1) Bit 4 clear: a match frame by frame equal to the variant without the define (the calls are retail's).
(2) Bit 4, two cars of one type: two engines, each with its own pan and pitch; one dies: the other keeps
playing. (3) 16 cars, `portEngineVoices` 0: count voices; fire weapons near: the explosions keep their voices,
the farthest engines drop. (4) `portEngineVoices` 8: only the 8 nearest engines. (5) Two Thumpers flaming at once:
two flame sounds, each stopping with its own flame. (6) With `TM2_PORT_DARK_TOOTH`: two Dark Tooths' swoop
warnings at once. (7) Toggle bit 4 mid-match: no stuck engine. (8) With the swap: the swapped car's engine
follows the new type.

### 12.9 Status and proofs

- **Built and proved** (`tools/build_source_only.py` + `verify_reloc.py` per configuration, the variant and its
  placement check as `build.py` links them, port RAM end 0x80C00000; then the full
  `build.py --source-only --sdk-from-lib --relocatable --port-variant`, `work/build-20261013-060652-752747`). The retail images keep their
  hashes: hybrid `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`. Every
  configuration passes its proof against E0 and the placement check:

  | configuration | variant sha256 | `port_changed` | port sections end (RAM end 0x80C00000) |
  |---|---|---|---|
  | no port define (published `out/TM2_SOURCE_PORT.EXE`) | `8cb2b843…` | none (code unchanged; `.port_bss` + 208 bytes) | 0x805BBE00 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SOUND` | `1bb62c41…` | 52 (step 1's 47 + the 5 of 12.5) | 0x805BD200 |
  | `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`, `TM2_PORT_TEXTURE_BANKS`, `TM2_PORT_CAR_SPECIALS`, `TM2_PORT_DARK_TOOTH` + `TM2_PORT_CAR_SOUND` | `564015c0…` | 133 (129 + `carSoundUpdate`, `weaponFlameStart`, `soundSortInstances`, `aiDarkToothBossUpdate`; `worldUpdate`, `weaponFlameUpdate` were already changed) | 0x805FE068 |
  | + `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` (every port define) | `3173659a…` | 140 | 0x805FF0D8 (also below the old 0x80600000, by 3,880 bytes) |

  `TM2_PORT_CAR_SOUND` adds 212 bytes of storage (every variant) and about 3.9 KB of port code (the step-2e configurations end 4,128 bytes later).
- **`tools/check_car_groups.py` check 9:** in every build with the define no game function calls `soundPlay`,
  `soundPlayVariant`, `soundStop`, `soundStopInstance` or `soundIsActive` with a per-car sound id (the 14 engines,
  47; 61 and 66 with `TM2_PORT_DARK_TOOTH`), and the five (six) changed functions make no such call with a
  non-constant id (except `carSoundUpdate`'s sound-21 calls by player index); every hook is called; the 8 port
  functions are in `.port_text` and call no `rand()`; `--expect-functions` matches (52 / 133 / 140 with the far
  defines). All OK. Negative control (`--negative-control` on a groups build without the define): check 9 fails
  with the 8 retail calls (`carSoundUpdate` soundPlay / soundIsActive / soundStop by variable id, `worldUpdate`'s
  stop, `weaponFlameStart`'s soundPlay(47), `weaponFlameUpdate`'s soundIsActive(47) / soundStop(47)).
- **Retail behaviour with bit 4 clear** holds by construction: every changed call site calls a port function
  whose bit-4-clear path is the retail call with the retail arguments (`soundSortInstances` falls through to
  retail's two sorts). With bit 4 set and no duplicate the requests differ from retail (instance s + 1, list
  sizes, the sort order): accepted by the port (G.7, sound state is not compared frame by frame).
- **Not runtime-tested** (12.8).

## 13. Request 33 step 1: per-car CLUTs without texture banks (`TM2_PORT_CAR_CLUTS`)

Request 33 step 1 (`PER_CAR_PLAN.md` A.2 rows 9 and 10, C.1; the port's answer G.1). Goal, without the texture
banks of step 4: **team colours on cars of one type** (4 teams of 4 of one car) and **an invisibility fade per car**.
Every CLUT a car would share with another car or with the arena gets a copy of its own; the pixels stay shared. Behind
`--port-define TM2_PORT_CAR_CLUTS`, which needs `TM2_PORT_CAR_GROUPS` (`#error` otherwise), and the run-time
`portCarOptions` **bit 7** (`TM2_PORT_CAR_OPT_CAR_CLUTS`, 0x80), latched at each load by `bitmapInit`. Bit 7 clear:
retail's logic (13.7). Works with and without every other port define.

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CAR_CLUTS [other port defines] --output work/pv/P4
```

### 13.1 How it works

- **Notes during the parse.** `vramPlaceTexture` writes each slot as retail (reuse by key: the first record with
  the key, from the arena or an earlier car). After writing slot (group, image) it calls `portCarClutNote`: an
  image with a CLUT (4- or 8-bit) placed here is the group's own; one taken from **another car** or from the
  **arena** is noted for a copy (a CLUT of an earlier image of the same TMS stays the group's own).
- **The copy pass after the last car TMS.** `sessionLoadArenaAndCars` calls `portCarClutsCopyAll` right after the
  car TMS loop, before the level DMD and the car DMDs: every original (the arena's images, the first car of each key)
  is placed by then, so **no copy can move or block a retail placement**, and the copies are in the slots before
  `assetInstallImage` remaps each car's DMD (`modelRemapTextures` takes the CLUT from the slot). Each copy is read
  back from VRAM (`gpuStoreImageLine`) and written to its own spot (`gpuLoadImageLine`): it is the row as the arena
  or the first car uploaded it, so an arena palette (Roadkill's and Dark Tooth's effect frames on Suicide Swamp,
  Cyburbia, Roof Tops) looks as in retail. No arena CLUT animation (objects 5000 / 5001 / 5100 of the 14 level
  DMDs) acts on a CLUT a car takes from the arena (checked on the disc), so a copy loses nothing.
- **Order (when room is short):** phase 0 the bodies (image 0, team paint), phase 1 the other 256-colour CLUTs
  taken from a car (Axel's two parts, Sweet Tooth's, Dark Tooth's), phase 2 the 16-colour ones taken from a car
  (undercarriage, shadow, flame, lights, effect frames), phase 3 those taken from the arena; groups in load order.
- **Where:** first VRAM rows 240..255 (13.3), then the rest of VRAM in retail's CLUT scan order (upper half,
  then lower half) with **every texel** of the spot tested in the usage bitmap (retail tests the top-left one).
  A copy that finds no room leaves that car on the shared row (retail), counted in `portCarCluts.failed`, and
  flagged in `portCarCluts.shared` (13.5) so the port knows team paint or the fade would also act on another car.
- **Fade per car.** With bit 7 live a car's +8 CLUT animation (the invisibility fade) is one definition per car
  key, id `key + 8` (= `type + 8` for the first car of a type), exactly as with banks: `clutAnimRegister` keys it,
  `weaponInvisibilityUpdate` and the swap's `portCarResetLook` use `portTexFadeId(slot, type)`. The definition
  reads the CLUT from the car's own first primitive, which the remap pointed at the car's copy, so one car's
  invisibility fades only that car (and, for Spectre, Roadkill and Axel, whose fade CLUT is the undercarriage
  (image 1), only that car's undercarriage copy; Axel's key 9201 comes from the arena on New York and Hong
  Kong and is copied too; Spectre's and Roadkill's key 1FCD5 is in no arena). Outlaw 2, Mr. Grimm and Thumper keep retail's junk row (untextured first primitive,
  `TEAM_COLORS.md` 2.5).
- **Reused from step 4** (`TEXTURE_BANKS.md` 6.7): the image bookkeeping of `portTexBanks` (level record count,
  each group's record range, `body_clut`), `portTexPlaceBegin`, `portTexFindReuse` (so `portCarGroupNoReuse`
  works without banks too), `portTexNoteSlot`, `portTexFadeId` and the per-key fade in `clutAnimRegister`. They
  are compiled with either define (internal macro `TM2_PORT_TEX_SHARED`, `car_cluts.h`); without
  `TM2_PORT_TEXTURE_BANKS` no bank is ever live.

### 13.2 Texture banks (bit 5) and bit 7

With `TM2_PORT_TEXTURE_BANKS` and bit 5 latched, every car already has its own bank with its own copy of every
CLUT (`TEXTURE_BANKS.md` 6), so **bit 7 is ignored** for that load (`portCarCluts.live` = 0: no note, no copy).
Bit 7 is for loads without banks (bit 5 clear, or a build without the banks define). In both modes the port finds
each group's body CLUT at `portTexBanks.body_clut[g]` in bank `portTexBanks.body_bank[g]` (bank 0 without
banks: the copy pass writes the copy's id there), and the fade is per car key.

### 13.3 VRAM rows 240..255: one allocator for bit 7 and bit 6

Rows 240..255 of x 0..639 lie under the 320x240 frame buffers (nothing draws there: G.1). `bitmapInit` now
reserves (0, 0, 640, 256) in the usage bitmap in **every** case again (retail), so retail's scans never take these
rows; **one allocator** (`portClutRowsAlloc`, state in `portCarCluts.rows`) hands them out to both users:

- **bit 7** (this step): the copies, first;
- **bit 6** (Dark Tooth anywhere, section 11.2, `TM2_PORT_DARK_TOOTH`): a CLUT upload that finds no room in the
  upper half takes a spot there **before** the scan goes on to the lower half (`vramUploadClut` →
  `portClutRowsUpload`). Until now bit 6 made `bitmapInit` reserve only rows 0..239, so retail's scan put the
  first 256-colour CLUTs of every car into rows 240..255 (and could take (512, 240..255), whose 256 texels reach
  x 767). Now every car keeps its retail row and only what has no room in the upper half moves there: the same 32
  256-colour spots, plus 16-colour ones, never two users on one spot.

Spots: 256-colour at x 0 and 256 (32), from row 240 down; 16-colour at x 512..639 first (128 spots, where no
256-colour spot fits), then from row 255 up, x 496 down, so the two kinds meet in the middle. Who took what is
not recorded per spot; both users only take, nothing is freed during a load. With bit 6 and bit 7 both set the
Dark Tooth uploads come first (they happen during the parse; the copies after it).

Found on the way (model, 13.6): retail's CLUT scan tests only the top-left texel of a spot, so once the upper
half's x 768 column is full, a 256-colour CLUT lands at (768, 256..) in the **lower half over texels already
taken** (bitmapInit's font area and car pixels at x 960..1023). The model shows it from the 9th different type on
Amazonia, Holland and Cyburbia (largest first), the 11th on Moscow and Roof Tops (and with 15 of Mr. Grimm, Axel or
Sweet Tooth plus a Dark Tooth on Amazonia, Holland, Cyburbia with bit 6 clear), so `SIXTEEN_CARS.md` 4.4's "all 14
fit" there meant "placed, but CLUTs overwrite texels" (`RETAIL_BUGS.md` 27). Bit 6's fallback comes before that
lower-half scan, which avoids it on those arenas while rows 240..255 last. The fix for the originals is
`portCarOptions` bit 9 (13.12); with bit 9 clear their scan is retail's.

### 13.4 What changed (6 game functions with bit 9's `vramUploadClut`; it also changes with `TM2_PORT_DARK_TOOTH`)

| VA | function | unit | change under the define |
|---|---|---|---|
| 80121550 | `bitmapInit` | `bitmap_init.c` | `portCarClutsReset()` (latch bits 7 and 6, clear the tables; after `portTexBanksReset`); `bitmapMarkRect(0, 0, 640, 256)` again with `TM2_PORT_DARK_TOOTH` |
| 801209E8 | `vramPlaceTexture` | `screen_deathmatch_result.c` | step 4's bookkeeping (`portTexPlaceBegin`, `portTexFindReuse`, `portTexNoteSlot`) also without banks; `portCarClutNote(row, col, img->mode, known)` after writing the slot (reuse branch: the record; placed: 0) |
| 801489B4 | `clutAnimRegister` | `engine_clut_anim.c` | a car's +8 fade keyed `portCarLoadKey() + 8` while banks **or** the copies are live |
| 8010DE54 | `weaponInvisibilityUpdate` | `ai_link_node_pick.c` | `portTexFadeId(slot, type)` (also step 1's) |
| 8012BAFC | `sessionLoadArenaAndCars` | `shell_main.c` | `portCarClutsCopyAll()` after the car TMS loop (both versions: with and without `TM2_PORT_CARS`) |
| 80120D3C | `vramUploadClut` | `scene_init.c` | the spot test is `portClutSpotFree(x, y, w)` (bit 9: every texel; clear: retail's `bitmapIsFreeXY(x, y)`), and after both halves `portClutRowsLast(w, h, img, slot, bank)` (bit 9, 13.12); **with `TM2_PORT_DARK_TOOTH`** also, between the upper and the lower half, `portClutRowsUpload(w, h, img, slot, bank)` (bit 6) |

With `TM2_PORT_DARK_TOOTH` (section 11) `bitmapInit` keeps a change (the reset call) and `vramUploadClut` is new
in its list; `bitmapMarkRect(0, 0, 640, 240)` is gone. `portCarResetLook` (`TM2_PORT_CAR_SWAP`) ends the car's own
fade through `portTexFadeId` with this define too. Unchanged: `modelRemapTextures` (it already reads the CLUT from
the slot), `gpuLoadImageLine` / `gpuStoreImageLine`, the emitter, `bitmapAddRecord`.

Port functions (`.port_text`, `port_variant_arrays.c`; `include/semantic/car_cluts.h`):

| function | groups + CLUTs | every port define | use |
|---|---|---|---|
| `void portCarClutsReset(void)` | 0x804FF688 | 0x80540A18 | `bitmapInit`: latch bit 7 (ignored while banks are live) and bit 6, clear the tables |
| `int portClutRowsAlloc(int w, int *x, int *y)` | 0x804FF77C | 0x80540B50 | a 16- or 256-texel spot in rows 240..255, 0 = full |
| `int portClutRowsUpload(int w, int h, void *data, int slot, int bank)` | - | 0x80540CD4 (groups + specials + Dark Tooth: 0x805001FC) | `vramUploadClut` (`TM2_PORT_DARK_TOOTH`, bit 6) |
| `void portCarClutNote(int group, int image, int mode, int *known)` | 0x804FF900 | 0x80540DA4 | `vramPlaceTexture`: note the image's CLUT, and one to copy |
| `int portClutSpotFree(int x, int y, int w)` | 0x80544868 (13.12's build) | 0x805974E8 (13.12's build) | `vramUploadClut`'s spot test: bit 9 latched, all w texels free (and x + w <= 1024); else `bitmapIsFreeXY(x, y)` |
| `int portClutRowsLast(int w, int h, void *data, int slot, int bank)` | 0x80544910 (13.12's build) | 0x80597590 (13.12's build) | `vramUploadClut` with bit 9, after both halves: a bank-0 one-row CLUT to rows 240..255, the slot's CLUT x / y, the upload; 0 = none |
| `void portCarClutsCopyAll(void)` | 0x80500264 | 0x80541688 | `sessionLoadArenaAndCars`: the copy pass, then the table |
| `int portCarClutsCopyGroup(int group)` | 0x805002E4 | 0x80541708 | **API** for a group parsed later (a mid-match load, after its `bitmapParseContainer`, before its `assetInstallImage`): its copies from rows 240..255 only (the usage bitmap is gone after the load); returns the copies made |

Static helpers: `portCarClutAliased`, `portCarClutScan`, `portCarClutCopyOne`, `portCarClutCopyPhase`,
`portCarClutsPublish`. Step 4's bookkeeping functions (`portTexBanksReset`, `portTexPlaceBegin`,
`portTexFindReuse`, `portTexNoteSlot`, `portTexFadeId`) are in `.port_text` with this define too.

### 13.5 The table for the port: `portCarCluts`

`struct TM2PortCarCluts` (`include/semantic/car_cluts.h`; 7,120 bytes, `.port_bss`, every variant, after
`portEngineVoices`). Written by the C at the load; the port reads it after the load. Addresses from each build's
`symbols.tsv`: 0x804FD278 in the default variant and with `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_CLUTS` (or + specials +
Dark Tooth), 0x80539728 with `TM2_PORT_CARS=16` (the big and the every-define configurations of 13.11).

| offset | field | contents |
|---|---|---|
| 0x000 | `live` | 1 = copies this load (bit 7 latched, banks not live) |
| 0x004 | `rows_live` | 1 = the copies use rows 240..255, 2 = bit 6's fallback does, 4 = bit 9 latched (whole-spot scan; its last resort uses them) |
| 0x008 | `copies`, 0x00C `failed` | copies made / copies that found no room this load |
| 0x010 | `rows[16][2]` | rows 240..255: used 16-texel units (unit u = x 16u..16u+15: bit u & 31 of word u >> 5) |
| 0x090 | `pend16[64]`, 0x190 `pend256[64]`, 0x290 `arena[64]` | per load group, bit = image: copies still to make (16 / 256 colours), images taken from the arena |
| 0x390 | `clut[64][32]` (u16) | **the CLUT id of each group's image after the load** (x / 16 \| y << 6), 0 = no CLUT |
| 0x1390 | `state[64][32]` (u8) | 0 none, 1 own (uploaded for this group), 2 own copy, 3 still shared (no room); all 0 when bit 7 was clear |
| 0x1B90 | `shared[64]` (u8) | per group: 1 = its body CLUT is also another car's, 2 = another of its CLUTs is also another car's, 4 = one of its CLUTs is still the arena's |

Load group g: human i = group i + 1, opponent i = humans + i + 1 (as `portTexBanks`); image = index in the car's TMS
(0 = body; Dark Tooth's body is images 0, 5, 6 and 16, `TEAM_COLORS.md` 4.2). The first car of a type keeps its
retail row (state 1); its twins have copies (state 2), so the first car's row is no longer anybody else's.

### 13.6 Team colours: the recipe for the port

After the load (`sessionLoadArenaAndCars` returned; the slots and the usage bitmap die with the first frame):

1. For each car's load group g with a team colour: `id = portCarCluts.clut[g][0]` (or
   `portTexBanks.body_clut[g]`, the same id). If `portCarCluts.shared[g] & 1`, the row is also another car's
   (a copy failed): painting it paints both (choose the same team for both, or skip).
2. The row: `x = (id & 0x3F) * 16`, `y = id >> 6`, 256 entries (`state` 1 or 2).
3. The palette: read the row back (`StoreImage` of (x, y, 256, 1), right after the load: every copy equals the type's
   palette) or take image 0's CLUT from `TMS\<code>.TMS`; recolour the paint entries (`TEAM_COLORS.md` 4: keep
   0x0000 and bit 15).
4. Write it: `LoadImage(&(RECT){x, y, 256, 1}, palette)` and `DrawSync(0)` (bank 0; with banks live use
   `portTexLoadImage(portTexBanks.body_bank[g], ...)`, `TEXTURE_BANKS.md` 6.5 item 6). Every primitive of that car
   (body, tires, wreck, far LOD) uses that row; no other car does.
5. Between fades: a fade saves the palette at its start and writes it back at its end (`TEAM_COLORS.md` 2.5), so
   repaint when the car is not invisible, or repaint again after its fade ends.

Other parts (Axel's machine, Dark Tooth's images 5, 6, 16) the same way with `clut[g][image]` and
`shared[g] & 2`. The car select keeps its own rows (`TEAM_COLORS.md` 5).

### 13.7 Retail-equal with the define on and bit 7 clear

`portCarClutNote` and `portCarClutsCopyAll` return at once, `portTexFindReuse` returns `bitmapFindRecord`'s record
(no no-reuse flag set), the fade id is `type + 8`, `bitmapInit` reserves retail's rectangle: every slot, record,
upload and VRAM write is retail's. Expected differences from the variant without the define: the port storage it
writes (`portTexBanks` bookkeeping, `portCarCluts` cleared at each load) and the addresses of `.port_text`. No
`rand()` call is added or removed. With bit 7 and no duplicate type the copies still happen for the shared parts
(undercarriage, shadow, flame, effect frames: every car type shares some), so the VRAM differs from retail's.

### 13.8 Room per arena (`tools/check_car_groups.py --clut-model`, check 11)

Host model of the load as the C runs it (the disc's 14 arena TMS files and 15 car TMS files; stride 32). "rows" /
"16s": whole free 256-texel rows and free 16-texel spots of VRAM after the level, besides rows 240..255 (32 + 128).
"one": 16 cars of one type, for each of the 14 normal types: types whose 15 body copies all fit, and the type with
the fewest copies made / needed (all CLUTs). "4x4": 4 teams of 4 cars (4 types, `TEAM_SETS`, 6 sets): sets whose
copies all fit. "mix": as many different types as fit (largest first), filled up to 16 with duplicates: body copies
and all copies made / needed. "dt0" / "dt": 15 of one type plus a Dark Tooth (loaded last) with bit 6 clear / set:
types whose 14 body copies fit, the most originals failing (with bit 7 clear too: retail's room), and the rows
240..255 spots bit 6 took.

| arena (TMS) | rows | 16s | one: types whose bodies all fit (least complete type: copies made / needed) | 4x4 | mix | dt0: bodies, originals failing | dt: bodies, originals failing, bit-6 spots |
|---|---|---|---|---|---|---|---|
| 1 Los Angeles (FREEWAY) | 17 | 7461 | 14/14 (fewest: HH 76/76) | 6/6 | 12 types: bodies 4/4, all 85/85 | 14/14, 0 | 14/14, 0, 0 |
| 2 Moscow (DISH) | 271 | 11268 | 14/14 (fewest: HH 75/75) | 6/6 | 14 types: bodies 2/2, all 78/78 | 14/14, 0 | 14/14, 0, 0 |
| 3 Paris (PARIS) | 15 | 5554 | 14/14 (fewest: AX 104/106) | 6/6 | 8 types: bodies 8/8, all 97/97 | 14/14, 6 | 14/14, 6, 0 |
| 3 Paris 2P (PARIS2) | 15 | 5816 | 14/14 (fewest: AX 105/106) | 6/6 | 8 types: bodies 8/8, all 97/97 | 14/14, 5 | 14/14, 5, 0 |
| 4 Amazonia (ISLANDS) | 269 | 8929 | 14/14 (fewest: HH 75/75) | 6/6 | 12 types: bodies 4/4, all 84/84 | 14/14, 0 | 14/14, 0, 2 |
| 5 New York (ROOF) | 113 | 7564 | 14/14 (fewest: HH 76/76) | 6/6 | 12 types: bodies 4/4, all 85/85 | 14/14, 0 | 14/14, 0, 0 |
| 5 New York 2P (ROOF2) | 274 | 8673 | 14/14 (fewest: HH 75/75) | 6/6 | 13 types: bodies 3/3, all 81/81 | 14/14, 0 | 14/14, 0, 0 |
| 6 Antarctica (GLACIER) | 271 | 9181 | 14/14 (fewest: HH 75/75) | 6/6 | 10 types: bodies 6/6, all 90/90 | 14/14, 0 | 14/14, 0, 0 |
| 6 Antarctica 2P (GLACIER2) | 272 | 9246 | 14/14 (fewest: HH 75/75) | 6/6 | 11 types: bodies 5/5, all 86/86 | 14/14, 0 | 14/14, 0, 0 |
| 7 Holland (DEN) | 269 | 10647 | 14/14 (fewest: HH 75/75) | 6/6 | 14 types: bodies 2/2, all 78/78 | 14/14, 0 | 14/14, 0, 2 |
| 8 Hong Kong (HKONG) | 14 | 7330 | 14/14 (fewest: AX 105/107) | 6/6 | 9 types: bodies 7/7, all 96/96 | 14/14, 1 | 14/14, 0, 1 |
| 10 Suicide Swamp (SWAMP) | 15 | 6065 | 14/14 (fewest: AX 103/105) | 6/6 | 9 types: bodies 7/7, all 94/94 | 14/14, 0 | 14/14, 0, 0 |
| 11 Cyburbia (BURB) | 269 | 11236 | 14/14 (fewest: HH 76/76) | 6/6 | 14 types: bodies 2/2, all 79/79 | 14/14, 0 | 14/14, 0, 2 |
| 12 Roof Tops (SROOF) | 527 | 11778 | 14/14 (fewest: HH 75/75) | 6/6 | 14 types: bodies 2/2, all 78/78 | 14/14, 0 | 14/14, 0, 0 |

So: **16 cars of any one type and every 4 x 4 set tried get their own body CLUTs on every arena**, and all their
other CLUTs except 1-2 of Axel's 256-colour parts on Paris, Hong Kong and Suicide Swamp (those stay shared and are
flagged 2). With bit 6 and a Dark Tooth, every body still fits; Axel then leaves 7-12 parts shared. Bit 6's fallback is what
lets every Dark Tooth CLUT fit on Hong Kong (with bit 6 clear one original CLUT fails there beside some types; none with it). The binding
limit stays retail's pixel room (Paris: Dark Tooth's own images do not fit beside 15 others, with or without the
copies). The model also checks that no copy overlaps anything placed or another copy, every copy is in VRAM on a
16-texel boundary, rows 240..255 are only taken through the allocator and within x 0..639, and every original keeps
the position it has with bit 7 clear: all OK. Negative control: with the copy scan testing only the top-left texel
(as retail) the model fails (copies over the font area on Paris and Suicide Swamp).

### 13.9 Limits

- **CLUT animation tables:** one fade definition per car as with banks: more than about 11 cars need
  `TM2_PORT_CARS` (`clutAnimDefs` 40 / `clutAnimRunning` 20, `portCarOptions` bit 3); a car whose definition does
  not fit has no fade (its body still hides and shows).
- **Slot stride 20** (without `TM2_PORT_CARS`): a Dark Tooth that is not the last group shares its slots 20..30
  with the next group (`SIXTEEN_CARS.md` 4.5); those images get no copy (`state` 0).
- **Mid-match loads:** `portCarClutsCopyGroup(g)` uses rows 240..255 only. A copy is in the shared VRAM, so a car
  removed for good leaves its copies there (they are not freed; the next load starts over).
- **Arena animations** would not follow a copy; none acts on a car CLUT on the disc (13.1).

### 13.10 Tests for the port

(1) Bit 7 clear: frame by frame (VRAM included) equal to the variant without the define. (2) Bit 7, two cars of one
type: invisibility on one, the other keeps its colours; Axel on New York / Hong Kong: invisible Axel, the other
cars' undercarriages unchanged. (3) 4 teams of 4 of one car type on Hong Kong and Paris: 4 colours per the recipe
(13.6), `shared` all 0. (4) 16 Axels on Paris: `failed` > 0, `shared[g] & 2` set for the cars whose part copy
failed, bodies all paintable. (5) Bit 6 + bit 7 with a Dark Tooth on Amazonia: no glitch in rows 240..255.
(6) Bits 5 and 7 together: banks only (`portCarCluts.live` 0).

### 13.11 Status and proofs

- **Built and proved** (`tools/build_source_only.py` + `verify_reloc.py` per configuration, the variant and its
  placement check as `build.py` links them, port RAM end 0x80C00000; objects of the last full build; then the full
  `build.py --source-only --sdk-from-lib --relocatable --port-variant`, `work/build-20261013-081556-006124`). The retail images keep their
  hashes: hybrid `9219f7ba…`, source-only `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`. Every
  configuration passes its proof against E0 and the placement check:

  | configuration | variant sha256 | `port_changed` | port sections end |
  |---|---|---|---|
  | no port define (published `out/TM2_SOURCE_PORT.EXE`) | `2704e16e…` | none (code unchanged; `.port_bss` + 7,120 bytes, packet buffers - 780,800) | 0x804FEFD0 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_CLUTS` | `d8a3b85d…` | 51 (step 1's 47 + `bitmapInit`, `vramPlaceTexture`, `clutAnimRegister`, `sessionLoadArenaAndCars`) | 0x80500500 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SPECIALS` + `TM2_PORT_DARK_TOOTH` (bit 6 on the shared allocator) | `635b5b6d…` | 63 (62 + `vramUploadClut`) | 0x80501B08 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_TEXTURE_BANKS` (unchanged set) | `34f49c63…` | 54 | 0x804FFB90 |
  | `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`, `TM2_PORT_CAR_SPECIALS`, `TM2_PORT_DARK_TOOTH`, `TM2_PORT_CAR_SOUND` + `TM2_PORT_CAR_CLUTS` | `66668db0…` | 131 | 0x80541C18 |
  | every port define (+ `TM2_PORT_TEXTURE_BANKS`, `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE`) | `3d9f19a7…` | 140 | 0x80543048 |

  The ends are about 780 KB lower than at 33-3 because request 31's packet buffers were cut to 4 x retail's in the
  same change (`FAR_DISTANCE.md` 6). The CLUT code is about 3.3 KB of `.port_text`; the storage 7,120 bytes.
- **`tools/check_car_groups.py`** checks 10 and 11 (new): every configuration with the define or with
  `TM2_PORT_DARK_TOOTH` passes check 10 (port functions, hooks, the rows 0..255 reservation of `bitmapInit`, only
  `clutAnimRegister` reads `portCarCluts` among the game functions) and `--expect-functions` (51 / 63 / 131 / 140
  with the far defines); negative control: check 10 forced on the groups + banks build fails (5 functions and 3
  hooks missing). Check 11 (`--clut-model`) passes on all 14 arenas (13.8) and fails its negative control.
  Checks 1-9 still pass in every configuration.
- **Retail behaviour with bit 7 clear** holds by construction (13.7).
- **Not runtime-tested** (13.10).

### 13.12 `portCarOptions` bit 9: the CLUT and image scans test the whole spot (request 33-1 follow-up, request 41 item B)

PR #38's question 1 (13.3, `RETAIL_BUGS.md` 27): retail's `vramUploadClut` takes a CLUT spot when its top-left
texel is free, so with 12-14 different car types a 256-colour original lands in the lower half on texels already in
use. With `TM2_PORT_CAR_CLUTS` and `portCarOptions` **bit 9** (`TM2_PORT_CAR_OPT_CLUT_SCAN`, 0x200; latched by
`bitmapInit` into `portCarCluts.rows_live` bit 2 (value 4); independent of bit 7, and also latched while banks are
live, where it changes nothing: a bank's scan tests whole rectangles already and bank 0 keeps only the arena):

- **The test:** `vramUploadClut` calls `portClutSpotFree(x, y, w)` for each candidate in retail's order (upper half,
  then lower half; x in steps of the CLUT width): every one of the w texels (16 or 256) must be free in the usage
  bitmap. A CLUT therefore never overwrites used texels. Up to the first CLUT retail would have put over used texels
  every placement is retail's; from there on CLUTs take later spots.
- **No whole spot in either half:** `portClutRowsLast` takes a spot in VRAM rows 240..255 from the allocator of 13.3
  (256-colour at x 0 / 256 from row 240 down, 16-colour from x 512.. then from row 255 up), writes the slot's CLUT
  x / y and uploads. The originals take these rows during the parse, so before bit 7's copies (which then have fewer
  spots); with bit 6 the rows were already tried between the halves, so this second try finds nothing new.
- **When the rows are full too** (an original that cannot be placed): retail's failure. `vramUploadClut` returns 0,
  `vramPlaceTexture` prints "Relocatable texture not loaded" and returns 0, and the slot keeps the CLUT x / y it had
  (that image draws with a wrong palette; with bit 7 live `portCarClutNote` records it as the group's own with that
  id). Nothing else stops the load. No modelled load gets there: in every load of 13.8 and `SIXTEEN_CARS.md` 4.4
  with bit 9, rows 240..255 had room for every CLUT the halves could not take; the limit stays the pixels (Paris,
  Swamp, Los Angeles, New York, Hong Kong at 12). A texture bank (bit 5, `TEXTURE_BANKS.md`) is the way past it.
- **Bit 9 clear:** `portClutSpotFree` is `bitmapIsFreeXY(x, y)` and `portClutRowsLast` returns 0 at once: every
  test, placement, upload and failure is retail's. The variant without the define has retail's `vramUploadClut`
  (with `TM2_PORT_DARK_TOOTH` alone: 33-2e's version).
- **Images (request 41 item B, 2026-10-16).** `vramPlaceTexture`'s image scan had the same one-texel test
  (`RETAIL_BUGS.md` 27: the model shows image rectangles over earlier ones on most arenas, on Suicide Swamp already
  with Mr. Grimm and Twister). With bit 9 its scan runs **four passes instead of two** (`portImagePasses`): passes 1
  and 2 are retail's two (the x 960.. column first, then all; x in steps of the width, y of the height, lower half)
  with `portImageSpotFree(x, y, w, h, pass)` testing **every texel of the w x h rectangle** free and inside VRAM
  (x + w <= 1024, y + h <= 512); only when neither finds one, passes 3 and 4 are **retail's two passes with retail's
  top-left test** (the clean fallback: the image loads where retail would put it, so bit 9 never makes an image fail
  that retail places; its CLUT still goes through `vramUploadClut`'s whole-spot test). Bit 9 clear: two passes,
  retail's test. A banked group (bit 5) keeps `portTexBankIsFree` (whole rectangles in its bank already).
  `screen_deathmatch_result.c` (`IMAGE_PASSES`, `IMAGE_SPOT_FREE`, `IMAGE_PASS_START`), `port_variant_arrays.c`
  (`portImagePasses`, `portImageSpotFree`, after `portClutRowsLast`).

**Room (check 11, `tools/check_car_groups.py --clut-model`, rerun with item B).** Distinct types, bits 6 and 7
clear, largest first / smallest first. Retail: placed / clean (before the first CLUT over used texels) / img (before
the first CLUT **or image** over used texels, new with item B); bit 9: the CLUT and image scans (`SIXTEEN_CARS.md`
4.4 has the whole table):

| arena | retail placed / clean / img | bit 9 (rows 240..255 spots it took) |
|---|---|---|
| Moscow (DISH) | 14 / 10 / 10, 14 / 11 / 11 | 14 (4), 14 (3) |
| Amazonia (ISLANDS) | 12 (pixels) / 8 / 4, 14 / 11 / 11 | 14 (6), 14 (5) |
| Holland (DEN) | 14 / 8 / 8, 14 / 11 / 11 | 14 (6), 14 (5) |
| Cyburbia (BURB) | 14 / 8 / 8, 14 / 11 / 11 | 14 (6), 14 (5) |
| Roof Tops (SROOF) | 14 / 10 / 4, 14 / 11 / 11 | 14 (4), 14 (3) |
| New York 2P (ROOF2) | 13 (CLUT) / 13 / 4, 13 / 13 / 13 | 14 (1), 14 (1) |
| Antarctica (GLACIER) | 10 (CLUT) / 10 / 10, 14 / 11 / 11 | 14 (4), 14 (3) |
| Antarctica 2P (GLACIER2) | 11 (CLUT) / 11 / 11, 12 (CLUT) / 12 / 12 | 14 (3), 14 (2) |
| Hong Kong (HKONG) | 9 (CLUT) / 9 / 2, 11 (CLUT) / 11 / 11 | 12 (pixels; 3), 12 (pixels; 2) |
| Los Angeles (FREEWAY) | 12 (pixels) / 12 / 9, 12 / 12 / 10 | **11** (pixels), 12 (pixels) |
| New York (ROOF) | 12 (pixels) / 12 / 12, 12 / 12 / 10 | 12 (pixels), 12 (pixels) |
| Paris (both files) | 8-9 (pixels), clean, img the same | unchanged |
| Suicide Swamp (SWAMP) | 9 (pixels) / 9 / **1**, 10 / 10 / 10 | 9 (pixels), 10 (pixels) |

The image overlaps come early on some arenas in the largest-first order (Swamp from the 2nd type: Twister's image 6,
8 x 64 at (616, 448), over an earlier 16 x 32 at (608, 480); Hong Kong from the 3rd; Amazonia, New York 2P and Roof
Tops from the 5th) and with Mr. Grimm + Twister + Dark Tooth on 9 arenas. With bit 9 the counts are those of the CLUT
scan alone except Los Angeles largest first, 12 -> 11 (retail's 12th type fit only by overlapping images: its img
count is 9); no CLUT or image lands on used texels. The fallback passes ran on Los Angeles, Hong Kong, New York,
Paris and Swamp (pixel-bound loads), and placed nothing over used texels except on Paris 2P with Mr. Grimm + Twister +
Dark Tooth, a load that fails for pixels anyway (retail fails it too). Check 11 fails if a bit-9 load puts a CLUT, or
an image outside the fallback, over used texels.

Dark Tooth after Mr. Grimm and Twister: retail overwrites on Holland and Cyburbia and fails (CLUT) on Amazonia and
Hong Kong; with bit 9 it fits on all four. The 13.8 scenarios with bit 9 set (a second table of check 11): every one
passes the copy checks and has no CLUT over used texels; "mix" now loads 14 types on Amazonia, New York 2P and both
Antarctica files (12, 13, 10, 11 with bit 9 clear) and 12 on Hong Kong (9), the bodies of the duplicates all fit,
Hong Kong's one original that failed beside a Dark Tooth with bit 6 clear fits, and the originals take up to 6
spots of rows 240..255. Check 11 also fails if any bit-9 load puts a CLUT over used texels, and needs the negative
control: with bit 9 clear, 11 of the 28 arena / order runs have one.

**Proofs** (`tools/build_source_only.py --build-dir last --relocatable --sdk-from-lib --port-variant`, the P4 link and
its placement link with the limits forced to 2, both against E0 of `work/build-20261016-014135-215616`;
`tools/check_all.py` on each, with `--models` on the last; then the full
`build.py --source-only --sdk-from-lib --relocatable --port-variant`, `work/build-20261016-025646-467274`: hybrid `9219f7ba…`,
source-only `d755c86e…`, `--sdk-from-lib` `f557c1e3…`, default variant unchanged):

| configuration | variant sha256 | `port_changed` | port sections end |
|---|---|---|---|
| no port define | `2bcf231c…` (unchanged) | none | 0x80543D90 |
| `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_CLUTS` | `596b5d2e…` | 52 (51 + `vramUploadClut`) | 0x80545450 |
| every port define + `TM2_PORT_HUMANS=16` | `a2a9f498…` | 232 (unchanged: `vramUploadClut` already changed) | 0x805A1CFC |

Check 10 now also wants `portClutSpotFree` and `portClutRowsLast` in `.port_text` and both called by `vramUploadClut`
with this define (`EXPECTED_CLUTS` gains `vramUploadClut`). Not runtime-tested: (1) bit 9 clear: VRAM equal to the
variant without the define; (2) bit 9, 14 different types on Holland: the font and every car intact, `vramTexSlots`
CLUTs never on a used texel; (3) bits 9 + 7 + 6 together on Amazonia with a Dark Tooth.

**Request 41 item B proofs** (2026-10-16, `tools/check_all.py work/pv/r41_<tag> --e0
work/build-20261016-025646-467274/source_reloc_e0 --models`; the full build `work/build-20261016-063045-826803` keeps the retail images):
`vramPlaceTexture` was already changed by `TM2_PORT_CAR_CLUTS`, so no new function changes; check 10 also wants
`portImagePasses` and `portImageSpotFree` in `.port_text` and called by `vramPlaceTexture`. Car stack + `TM2_PORT_BALANCE`
(with `TM2_PORT_CAR_CLUTS` and `TM2_PORT_DT_PLAYER`): P4 `cf5db5d27047`, P2 `6154382d2f87`; every port define +
`TM2_PORT_HUMANS=16` + `TM2_PORT_BALANCE`: P4 `88b86163cd49`, P2 `2d3fe4c3d995` (`BALANCE.md` 10.2); both proofs,
the placement check and `check_car_groups.py --expect-functions` pass; check 11 (`--clut-model`) passes with the image
model. Not runtime-tested: (4) bit 9 on Suicide Swamp with Mr. Grimm and Twister: Twister's image 6 no longer over
the earlier image (VRAM dump), and Los Angeles with 12 types largest first: the 12th type fails for pixels as
modelled (retail placed it over other images).

## 14. Request 33: online with bots, players dropping in (`TM2_PORT_CAR_DROPIN`)

Request 33 "Online with bots, players dropping in" and "load a joining player's car on demand" (`PER_CAR_PLAN.md`
B.6 / B.7 as revised, D item 11, E, and the port's answers G.11-G.15). A server starts as one player and its bots; a
player who joins does not take over a bot's car: one bot is removed for good and the player starts fresh, as a
respawn, on a car of the type they picked (a free loaded car of that type, or a car loaded mid-match from buffers the
port hands over); a player who leaves is removed the same way, and a bot may take the free place. Behind
`--port-define TM2_PORT_CAR_DROPIN`, which needs `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS` and `TM2_PORT_CAR_SWAP`
(`#error` otherwise: cars by load group and key, the DMD area and the start picker, the swap's reset/bind/park blocks).
Works with and without `TM2_PORT_TEXTURE_BANKS`, `TM2_PORT_CAR_SPECIALS`, `TM2_PORT_DARK_TOOTH`, `TM2_PORT_CAR_SOUND`,
`TM2_PORT_CAR_CLUTS`, `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE`; what each adds is said where it matters (14.6).

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_CAR_GROUPS --port-define TM2_PORT_CARS=16 --port-define TM2_PORT_CAR_SWAP \
    --port-define TM2_PORT_CAR_DROPIN [other port defines] --output work/pv/P4
```

**Retail behaviour while nothing is called.** The functions run only when the port calls them. The game changes are
five: `sessionBindCars` and `sessionLoadArenaAndCars` (already changed by steps 1/2), `startNodePickRandom`'s port
picker (port code), `bitmapSetupStorage` and `csAlloc` (new in the list: it takes an object an unload gave back first,
and the list is empty until `portCarUnloadGroup` runs, so it bump-allocates as retail). Until a drop-in function runs in a load
(`portCarDropIn.live`), `sessionBindCars` binds exactly as before; `portCarLoadDone` only records where the DMD area
ends; `portStartNodePick` reads a flag that is 0; `bitmapSetupStorage` puts the load's texture bookkeeping (VRAM usage
bitmap, slot table, image records) in port storage instead of packet buffer 0 and an ordering table, which changes
where it is, not what is written or read (retail's copies were simply overwritten by the first frames). No `rand()`
call is added or moved; the 11-car comparison stays equal. Expected differences from the variant without the define:
the port sections' addresses, `portCarDropIn` / `portCarLiveTex` in `.port_bss` (every variant), and packet buffer 0 /
the view-1 ordering table no longer holding the load's bookkeeping before the first frames.

### 14.1 API

Port code (`.port_text`, `port_variant_arrays.c`; `include/semantic/car_dropin.h`). Call every function **between
frames** (the port's frame hook before the car updates, where it would call `portCarSetGroup`), never from a game
callback, and not in the attract demo (refused). `kind`: 0 human, 1 AI. Addresses: read them from the build's
`functions.tsv` (kind `port`); in the configurations of 14.9:

| function | full car stack + drop-in | every port define + `TM2_PORT_HUMANS=16` | use |
|---|---|---|---|
| `int portCarRemove(int kind, int index)` | 0x80588BBC | 0x80598FEC | a record and its car leave for good (14.3); 1 or an error |
| `int portCarStartHuman(int index, int group, int flags)` | 0x805891A4 | 0x805995D4 | a new human (index = `humanCarTotal` or -1) on the free car of `group`, as a respawn; the human index or an error (`portCarStartHumanAt`: 0x805891C4 / 0x805995F4, at a route node) |
| `int portCarStartAi(int index, int group, int flags)` | 0x8058946C | 0x805998B0 | an AI record (a vacant index, `aiCarTotal`, or -1 = the lowest vacant else a new one) on the free car of `group`; the AI index or an error |
| `int portCarLoadGroupLive(const void *tms, int tms_size, const void *dmd, int dmd_size, int flags)` | 0x8058AB4C | 0x8059AF90 | a new load group from port buffers (14.6) into what unloads gave back; its car parked and free; the group or an error (flags 0) |
| `int portCarUnloadGroup(int group)` | 0x8058C2FC | 0x8059C9C8 | a group nobody drives given back whole (14.7); 1 or an error |
| `int portCarRoom(struct TM2PortCarRoom *room)` | 0x8058B0B0 | 0x8059B4F4 | what is free now (14.7); returns the largest DMD block |
| `int portCarCanLoad(const void *tms, int tms_size, const void *dmd, int dmd_size)` | 0x8058B074 | 0x8059B4B8 | 0 = `portCarLoadGroupLive` would load these files now (VRAM aside), else its error; changes nothing |
| `int portCarFreeGroupOfType(int type)` | 0x80588FE8 | 0x80599418 | the lowest load group whose car of type `type` (0x10..0xF0) has no driver, 0 = none |
| `int portCarGroupFree(int group)` | 0x80588FB0 | 0x805993E0 | 1 = the group's car has no driver (and was not unloaded) |
| `int portCarActiveCount(void)` | 0x80588E60 | 0x80599290 | humans + AI records driving a car (not vacant, not parked): the starts refuse at `TM2_PORT_CARS` |
| `void portHumanCompact(int from, int to)` | 0x805886E8 | 0x8059897C | human record `from` moves to index `to` (14.5); called by `portCarRemove`, exported for the port's own reordering |
| `int portCarBindCount(int ai, int count)` | 0x805897A0 | 0x80599BE4 | hook: `sessionBindCars`'s record counts |
| `int portCarBindSkip(int ai, int index)` | 0x805897DC | 0x80599C20 | hook: `sessionBindCars` skips a vacant AI record |
| `void portCarLoadDone(unsigned char *next, unsigned int end, int groups)` | 0x805898A0 | 0x80599CE4 | hook: `sessionLoadArenaAndCars`, the DMD area's free end and the next group |
| `void portCarLiveStorage(void)` | 0x80589820 | 0x80599C64 | hook: `bitmapSetupStorage`, the texture bookkeeping in `portCarLiveTex` |
| `void *portCsTake(void)` | 0x80589CF8 | 0x8059A13C | hook: `csAlloc`, an object unloads gave back (0 = none) |

(The configurations of 14.9: "full car stack" = `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`,
`TM2_PORT_CAR_DROPIN`, `TM2_PORT_TEXTURE_BANKS`, `TM2_PORT_CAR_CLUTS`, `TM2_PORT_CAR_SPECIALS`, `TM2_PORT_DARK_TOOTH`,
`TM2_PORT_CAR_SOUND`.)

Error codes (`car_dropin.h`): -1 `TM2_DROPIN_ERR_RECORD` (no such bound record), -2 `ERR_GROUP` (no car in that group,
or it has a driver, or it was unloaded), -3 `ERR_FULL` (`TM2_PORT_CARS` cars drive already), -4 `ERR_INDEX` (a human
must start at `humanCarTotal`; an AI at a vacant index or `aiCarTotal`; past the arrays), -5 `ERR_LAST` (the last
human cannot leave), -6 `ERR_NOLOAD` (no group number, car slot, overflow key, DMD space, coordinate systems,
animation or CLUT-animation entries, image records left; or 64 car roots took the vacant records' slot), -7 `ERR_VRAM`
(the textures do not fit: nothing was installed, the bookkeeping is as before), -8 `ERR_BUFFER` (a bad or misaligned
TMS / DMD), -9 `ERR_BUSY` (a driver on the group; before the match load; attract demo), -10 `ERR_TYPE` (a second car of
a type whose special state is single in this build: Twister, Axel, Mr. Slam, Shadow without `TM2_PORT_CAR_SPECIALS`,
Dark Tooth without `TM2_PORT_DARK_TOOTH`; an unload: another loaded car of such a type), -11 `ERR_NODE`
(`portCarStartHumanAt`), -12 `ERR_SHARED` (an unload: another group has the key, 14.7).

Flags of the starts: `TM2_DROPIN_START_RETAIL` (1, humans) runs retail's `carBindHumanSlot`, whose `carRestart` also
resets the whole match's projectiles, drops, specials' round state, effect sprites and HUD message, as a lost-a-life
respawn does in retail; without it (the default) only the record is reset. `TM2_DROPIN_START_RANDOM` (2) uses the
start picker of the build (retail's random pick, or the farthest-free one with `portCarOptions` bit 2) instead of
the farthest-from-live-cars rule.

### 14.2 Call sequences

**A player joins** (one bot leaves for them; the type the player picked is T, their team / net slot is the port's):

1. Pick the bot to drop: an AI record j (`aiCars[j]`, not vacant: `portCarDropIn.vacant[j]` 0).
2. `portCarRemove(1, j)`: the bot and its car leave (14.3); the car becomes a free car of its type.
3. A car of type T: `g = portCarFreeGroupOfType(T)` (the bot's car if it was a T, a car removed earlier, a spare);
   if 0, `g = portCarLoadGroupLive(tms, tms_size, dmd, dmd_size, 0)` with T's files (or a mod car's) the port read in
   the background (14.6); a negative result: the pick cannot be served on this arena now (grey it out).
4. `i = portCarStartHuman(-1, g, 0)`: the new human is `humanCarTotal - 1`, at the start farthest from the live cars,
   fresh weapons and full HP; map the net player to human i; give it a local view if it is local (14.5).
5. Team paint: repaint its body CLUT (`portTexBanks.body_clut[g]` / `body_bank[g]`, `TEXTURE_BANKS.md` 6.5;
   `CAR_GROUPS.md` 13.6 without banks).

**A player leaves:** `portCarRemove(0, i)`; if `i` was not the last human, the last human is now human i: remap the
port's tables keyed by human index (14.5). Then optionally a bot takes the place: `portCarStartAi(-1, g, 0)` with the
leaver's group (free now) or any free group.

**Bot fill at the match start** (server alone with 15 bots): load the match with the server's human and 15 opponents as
usual (settings); nothing of this API is needed until someone joins. To offer types not in the match without loading
on demand, load spares after the match load with `portCarLoadGroupLive` (each a free car) before the first frame.

**Rounds (modes 3/4 rebind every round):** nothing to redo. Once a drop-in function ran in this load
(`portCarDropIn.live`), `sessionBindCars` binds the records there are (`humanCarTotal` humans, `aiCarTotal` AI
records), each on its own group (`portCarBindGroup`, written by the starts and the compaction), and skips vacant AI
records. The port keeps `sessionSettings` as it wants it for its menus; the binding no longer reads its counts.

**A long online session** (many joins and leaves in one match load; 14.7): every leave frees a car, and a car nobody
will drive again is given back, so the next join's load reuses its memory:

1. **Leave:** `portCarRemove(kind, i)` (14.3). The car is now a free car of its type (`portCarGroupFree(g)` = 1).
2. **Keep or give back:** keep it while its type may be picked again soon (a later join of that type takes it with no
   load); else `portCarUnloadGroup(g)` (1 = done). The port's policy, e.g. keep at most one free car per type and give
   back the oldest, or give back every free car of a type nobody picked for a minute. Giving back is refused while a
   record drives it (`ERR_BUSY`) and, in builds without per-key specials, while another car of its single-special type
   is loaded (`ERR_TYPE`, 14.7).
3. **Join of type T:** `g = portCarFreeGroupOfType(T)`; if 0, decide with `portCarCanLoad(tms, tms_size, dmd, dmd_size)`
   (0 = it would load now; else the error `portCarLoadGroupLive` would return, nothing changed) or `portCarRoom(&room)`
   (what is free, struct `TM2PortCarRoom`); when it cannot load, give back a free car (2.) and ask again: the largest
   DMD block (`room.dmd_largest`) grows when the freed copy borders a hole or the tail; then
   `g = portCarLoadGroupLive(...)` (it takes the first free DMD range that holds it, the lowest free slots, key and group
   number) and `portCarStartHuman(-1, g, 0)` as above. `ERR_VRAM` (no banks) also changes nothing.
4. **Every peer** makes the same calls at the same frame (the order of loads and unloads decides the addresses, slots,
   keys and group numbers: the port's net state names cars by group number, which every peer computes alike).
5. **The next match load** resets everything (the area, the slots, keys and groups, the free lists).

### 14.3 What `portCarRemove(kind, index)` does (the census)

Order: the specials and look of the car (`portCarResetSpecials`, `portCarResetLook`: Mr. Slam claws holding it let go,
riders on it lose contact, its tornado / strike / leap / flames / shockwave / Dark Tooth controller end, ice, turbo,
fire, invisibility fade, engine and flame sounds stop), then everything it has out (G.11, removed at once, each the
way the game removes one), then every reference to it, then the car parked, then the record.

| holder (retail VA, entries) | field (form) | done |
|---|---|---|
| projectiles `fxRecords` (801A5B08, 16 / 48) | +0x42 owner (object id); +0x3C boomerang's return object | freed (`fxRecordFree`; homing / boomerang sound instances 10, 11, 12, 15 stopped) |
| | +0x38 target (pointer to a car object's position) | cleared: the missile flies on straight (`portCarRekeyReferences(slot, -1)`) |
| arcs `weaponArcs` (801847B4, 5 / 16) | +0x08 target id, +0x02 shooter slot, +0x0C / +0x10 record positions | ended (count 0) |
| drops `weaponDrops` (80184B04, 25): mines, remote bombs | +0x04 dropper id | ended (status, kind, held 0, object hidden and dropped); the record's `drop_armed` 0 |
| | +0x06 dropper slot | cleared (`portCarRekeyReferences`) |
| shields `weaponShieldPool` (80184A68, 4 / 16) | +0x04 car object | ended (timer 0, shield object hidden, `energy_shield` 0) |
| ricochets `weaponRicochetPool` (80184860, 10 / 32) | shot +0x02 owner id | ended (life 0, object hidden) |
| bullets `bulletRecords` (80181498, 40) | +0x24 owner id | freed (`bulletFree`) |
| effect sprites `fxSprites` (80185144, 150) | +0x2C pointer into the record (burning flame, while +0x01 follows) | stop following, loop ended |
| flame emitters `weaponFlameEmitters` (80184774, 2 / 16) | +0x1C car object | ended (`portCarResetSpecials`) |
| every record's `stats.killer` / `last_attacker` (+0x07 / +0x08) | slot | -1 (`portCarRekeyReferences`) |
| claw records (`weapons.slam`) | `spin_target` (key) | let go as `carDestroyed` does (`portCarResetSpecials`) |
| `stats.crush_owner` of riders on it | key | contact ended (`portCarResetSpecials`) |
| the car it rides (its own `crush_owner`) | the ridden car's `state.crush_peer` | 0 (else that car could not roll or collide) |
| its wake effect (human +0xCC / AI `state.x2C.fx` +0x98) | object | hidden, pointer 0 |
| `aiCarTargets` (801879FC) | AI index | -1 |
| `viewWatchedAiCar` (80180FAC, view 7) | AI index | 0 |
| tornado / strike / leap states, every key (`portCarSpecialNext`) | record pointers, object ids | a leap aimed at it ends (its turbo flame off); its own states ended above |
| Dark Tooth swoop (`aiDarkToothSwoopTarget` 80180B58), every Dark Tooth | human index | swoop ended |

Not touched (recomputed each frame or within one frame, CONFIRMED by reading): the HUD target panel
(`carNearestToHuman`), `aiGetTargetCar`, sound locate, the pair tables, the taser / lightning victim globals, the
contact records' `other` slots and `contact.last` (repeat-hit detection only).

The car: `portCarParkSlot` (not drawn, out of the hit pass, body hidden, far below, Dark Tooth's head hidden; slot kind
2, index -1: a free car of its type).

**AI record: vacant** (`portDropInVacate`): dead, HP 0 (no explosion), wreck state 4, out of range, type 0, killer
and last attacker -1, contact block and velocities zeroed, far below; `max_hp` kept above 0. Its object and slot
fields name the **vacant slot** `TM2_MAX_CAR_SLOTS - 1` (63), whose object is `portCarDropIn.dummy`: a zeroed scene
object (never drawn: not in the scene list, `visible` 0; far below; no collision box). `aiCarTotal` is not lowered.
`portCarDropIn.vacant[j]` = 1. Why not slot -1 or the old car (the census of 14.4): `carContactsCombine`,
`groundFiresUpdate` and `projectileHitTest` read `aiCarObjects[j]` every frame unguarded (a null object crashes),
`carObjectId` reads `carSlotObjects[slot]` (slot -1 reads before the table), and keeping the old car would keep it
from being free. The record's type 0 keeps `aiCarsCountRemaining`'s level-8 rule (`aiCars[0]` a Dark Tooth) from
reviving it.

**Human record:** humans stay contiguous (`worldUpdate`'s human loop stops at the first null object). If i is not the
last human, `portHumanCompact(last, i)` (14.5); then `humanCarTotal` - 1. The last human cannot leave
(`ERR_LAST`): a dedicated server with no human needs the round-end rule of Q10 first; the C itself adds no new
`humanCars[0]` dependence (G.13).

### 14.4 Vacant AI records: every reader (census; `tools/check_car_groups.py` check 12 keeps the list)

Every game function that reads `aiCarTotal`, `aiCarObjects`, `aiCarObjectSlots` or `aiCarTargets` (the link's
relocations) is listed in `DROPIN_AI_REVIEWED` with its verdict; a new reader fails the check until reviewed.

| function | why a vacant record is safe |
|---|---|
| `aiApplyDifficulty` | stats only (difficulty change) |
| `aiCarCount` | getter; its callers (weapons, lock-ons, slam, tornado, fires, targets) test HP > 0 or distance (far) |
| `aiCarNearestAlive` | HP > 0 |
| `aiCarObject` | getter: the dummy object (visible 0, far below, no collision box) for a vacant record |
| `aiCarObjectSlot` | getter (no caller in the link) |
| `aiCarsCountAlive` | HP > 0 |
| `aiCarsCountRemaining` | HP > 0; type 0 keeps the level-8 aiCars[0] Dark Tooth rule off |
| `aiGetTargetCar` | aiCarTargets[j] = -1 (nearest human); an AI target on a removed record is reset to -1 |
| `arenaSetup` | clears aiCarTargets |
| `carBindAiByType` | sessionBindCars skips vacant records once drop-in is live |
| `carBindAiSlot` | the binding itself (portCarStartAi) |
| `carDestroyed` | only for a car dying now (a vacant record is dead already: no second carDestroyed) |
| `carObjectId` | carSlotObjects[63] = the dummy: id 0; reached only through contacts, which the dummy never makes |
| `carRestoreNodeState` | no caller in the link |
| `carSaveNodeState` | no caller in the link |
| `carsPhysicsUpdate` | hdpCollideObject only when updated; carContactsCombine reads the dummy object (no points), the contact block is zeroed (no respond, no push timer) |
| `carTablesClear` | load reset |
| `demoRespawnAiCars` | attract demo only (drop-in is refused there) |
| `demoWatchNextAiCar` | attract demo only |
| `roundStart` | attract demo branches only |
| `uaInit` | clears aiCarTargets |
| `viewWatchNextAiCar` | view 7 (no caller in the link); viewWatchedAiCar is moved off a removed record |
| `worldUpdate` | pack loops skip wreck state 4, the closer loop skips dead; aiCarShouldUpdate is 0 (no drive, no physics); carSoundUpdate only while active; the out-of-range branch: type 0 has no engine id, slot 63 stops only its own instance; radar needs HP > 0; aiCarDamageUpdate: dead, max_hp kept > 0 |

### 14.5 `portHumanCompact(from, to)` and what the port remaps

A plain copy of the 0x814-byte record (no field points into the record itself: CONFIRMED by the census), then:
`player` and `id` (+0x000, +0x002) = to; `humanCarObjects`, `humanCarObjectSlots`, `humanCarTypes`, `padConfigs`,
`hudSmoothedSpeeds`, `carViewHotkeyReleased`, `carResetWeapons` moved; `carSlotIndices[slot]` = to; the car object's
id = to + 1; `portCarBindGroup` = the car's group; every holder by object id moved from + 1 -> to + 1 (projectile
owners, arcs, drops, ricochets, bullets, the strike's owner, the leap's target), by address into the record moved by
the record distance (arcs' end points, effect sprites following it, the tornado's anchor / flags, the strike's weapons
block, the leap's pointers, the Dark Tooth swoop target and its position), the shields' holder index; the old place
left unbound (object 0, claw record / fire / wake pointers 0).

**What the port remaps** (keyed by human index, not touched by the C): its net player <-> human table; the local view
of each human (D.3: `viewMainPlayer`, the view and camera of a local player, `viewFollowObjects` follows from
`worldUpdate` each frame by index); HUD state per human index (`hudPlayerRecords`, radar, target name, marker blink:
per HUD, rebuilt each frame from the human of that HUD, so a moved human's HUD shows on its new index's HUD; the
port's own HUD); the pad bindings of local humans (`padConfigs` moves with the record; the pad port a local human
reads is `player`, so a moved local human reads pad `to`: rebind the pad or give local humans the lowest indices);
`sessionPortHumanCars` / `human_car[]` in the settings (not read by the binding once drop-in is live; with
`TM2_PORT_HUMANS` the C moves them too, `HUMANS16_PLAN.md` H6); `lives[]`, the
results and any per-player tallies the port keeps; the side 0/1 sound loops (turbo, gun, loop 32 by `player` 0/1,
`PER_CAR_PLAN.md` D item 5).

### 14.6 Mid-match car loading (`portCarLoadGroupLive`, B.7)

The B.7.1 blockers and how they are solved:

| blocker | solution |
|---|---|
| the VRAM usage bitmap, the slot table and the image records live in packet buffer 0 / an ordering table and die with the first frames | `bitmapSetupStorage` puts them in `portCarLiveTex` (64 KB bitmap, 2,048 slots = 64 groups x 32, 4,096 records), so they live through the match; retail's sizes were 1,599 slots and ~682 records before spilling into the next ordering table |
| the TMS load buffer 0x80019C40 is the live level DMD | the TMS is parsed where the port has it (read only, word aligned); nothing is copied to 0x80019C40 |
| `portCarLoadGroup` falls back to a CD read | not used mid-match: no CD or file read is reachable (check 12) |
| the DMD area's end was a local of the load | `portCarLoadDone` (called by `sessionLoadArenaAndCars`) records it; each live load appends (word aligned) |
| load-time resets (`arenaResetSystems`, `carTablesClear`, `bitmapInit`, ...) | not called: check 12 proves none is reachable from `portCarLoadGroupLive` |
| `carChoiceRegisterGroup` for the new slot (arenaSetup's per-slot work) | called for each new slot (Minion: two) |
| the new key's freeze record | zeroed before the install (the key is fresh: a type no group holds, or the next overflow key) |
| `renderSetWorldShift` in `assetInstallImage` | the match's shift put back after the install |
| `exit(-1)` on a bad header | the TMS and DMD are checked first (`ERR_BUFFER`) |
| pools that fail badly when full (`csAlloc` returns 0 and is dereferenced, `clutAnimLastIndex` runs past its table) | room checked first (`ERR_NOLOAD`): 24 coordinate systems, 16 animation entries, 4 CLUT-animation definitions, the records, a slot below the vacant slot, an overflow key |
| a second Twister / Axel / Mr. Slam / Shadow / Dark Tooth binds the live car's special objects to its own copy | refused (`ERR_TYPE`) unless `TM2_PORT_CAR_SPECIALS` (and `TM2_PORT_DARK_TOOTH` for Dark Tooth) make the state per key; the first car of a type in the match binds the retail globals, which nobody else uses |

Textures:
- **Banks live** (`portCarOptions` bit 5 latched at the load): a fresh bank (`portTexBankAlloc`) for the group; its
  images and CLUTs placed there with retail's scan; arena keys keep the arena's images in bank 0 (the image records
  are alive now, so `portCarGroupNoReuse` is not needed; the port may still set it for a mod car).
- **No banks:** bank 0, retail's scan over the usage bitmap as it is after the load and earlier live loads; an image
  whose key is loaded already (arena, first car of the type) is taken by key as at the load, so a type already in
  the match costs no VRAM (`portCarGroupNoReuse[g]`, set by the port before the call, stops that). Before the parse
  the bitmap, the record count and the rows 240..255 allocator are saved and the group's slots marked unwritten; if
  any image or CLUT finds no room they are restored and `ERR_VRAM` returned (the pixels uploaded before the failing
  one lie in VRAM nobody uses; nothing is installed). With `portCarOptions` bit 7 (per-car CLUTs) the group's CLUT
  copies are made right after its parse (rows 240..255, then free VRAM: the bitmap is alive), before the install.
- How many types fit without banks: `SIXTEEN_CARS.md` 4.4 / B.7.3 per arena; with banks no arena limit.

Model: the DMD is copied into the DMD area (`portCarDmdArea` with bit 1, else retail's low area up to the EXE): the
first free range that holds it (an unload gave it back), else the tail (14.7); and installed as a load group
(`assetInstallImage`) with the lowest free group number: the root registers on the first free slot(s) (Minion: two
consecutive; `carWorldObjectCount` is set to that slot for the install and back to the larger count after it) and
takes the lowest free key, the per-key registries fill, `TM2_PORT_FAR_LOD`'s scan appends the car's LOD records
(global, de-duplicated, 2,048: safe between frames, `portFarLodBegin/End` restore them inside `renderScene`; an unload
removes them; `TM2_PORT_FAR_DISTANCE` has no hook in the install). The new car is parked and free; start a record on it.

**`portCarUnloadGroup(group)`** (a group nobody drives) gives everything of it back: 14.7.

### 14.7 Unloading (`portCarUnloadGroup`), what is free (`portCarRoom`), limits

Request 33 (the port's request 37 answer 14: bots and humans joining and leaving over a long online session). Before
this step an unload gave back only the group's texture bank, so every mid-match load used up DMD bytes, a slot, an
overflow key, a group number and up to 24 coordinate systems for the rest of the match. Now a group nobody drives is
given back whole, and the next live load reuses what it gave back. All of it is port code except `csAlloc`.

**`int portCarUnloadGroup(int group)`**, between frames. Refused, with nothing changed: `ERR_GROUP` (no such group, the
level's group 0, already unloaded), `ERR_BUSY` (a record drives one of its cars: human, AI, a parked Juggernaut record;
attract demo), `ERR_TYPE` (a build without per-key specials, `TM2_PORT_CAR_SPECIALS` for Twister / Axel / Mr. Slam /
Shadow, `TM2_PORT_DARK_TOOTH` for Dark Tooth: another loaded car of its type shares the type's single special objects
and nodes, which the last install of the type bound; give that one back first, or keep both), -12 `ERR_SHARED`
(another group holds its key: more duplicates of a type than the 32 overflow keys, retail sharing). Any group may go:
a car of the match start, a spare (request 35's), a live-loaded one, a removed car. What it does, in order:

| what | given back / cleared |
|---|---|
| the cars (each slot of the group) | `portCarResetSpecials`, `portCarResetLook` (claws holding it, riders, its tornado / strike / leap / flames / shockwave / Dark Tooth controller, ice, turbo, fire, fade), its sound instances (engine, flame, tornado, the turbo / gun / burn loops of players 2.., Dark Tooth's swoops 61 / 66, a remote human's shared gun loop; `portCarRemove` stops these too now), `portCarParkSlot`, `portCarRekeyReferences(slot, -1)` (kill credits, attackers, taser and drop slots, homing targets) |
| its scene objects | the cars' root objects, and the single special objects its install bound: Twister's tornado and funnel, Shadow's strike object, Dark Tooth's head; for the first car of a type retail's words (`weaponTornadoObject`, `weaponTornadoFunnelObject`, `weaponStrikeState.object`, `darkToothBossObject`) when they draw its copy, else 0 as before the type's first install; for a duplicate its `portCarSpecials` entry, which goes back to `portCarSpecialsReset`'s values. Each object leaves the drawn-object list and goes on `csAlloc`'s free list (`portCarUnload.cs_free`) |
| words that keep a node of its DMD copy | the shared projectile models (`weaponSpiralMissileModel` 2301, `weaponBoomerangModel` 2321, `weaponGrimmSpecialModel` 2322, `weaponGhostMissileModel` 2326, `weaponRicochetModelAlt` 2214: the same node of another loaded copy that has it, else 0, which the fire code tests), the special nodes `objRegistryId2324` / `2325`, `slamArmObject` / `slamClawObject` (0: no other car of the type shares them, `ERR_TYPE` above), `worldLoadParentObject` |
| other objects drawing its copy | another car's projectile or ricochet shot using one of those models: the same node of another copy, else a neutral model (the homing missile's / the plain ricochet's) and the shot ended; an object a second install of a type leaked (retail's "last install wins" without per-key specials): given back |
| words naming its objects | Thumper's flame emitters, the energy shields' source car, projectiles' homing targets (a position inside the object) and the boomerang's return object (a tornado it fired was ended by `portCarResetSpecials`) |
| request 35 | a slot that borrows a special whose lender's objects or nodes are gone (`portCarSpecialAsLoaded` fails) fires its own again (`portCarSpecialAsSet(slot, -1, -1)`); the as-entry objects that draw the copy, and the as-entries of the group's own slots, are given back (the next `portCarSpecialAsSet` makes new ones from the lender of the day) |
| lists pointing into its copy | the animation list (`animNodeList`: its entries removed, the order of the rest kept, so the others' per-frame `rand()` order is unchanged), the world models (`renderWorldModels`: the shadows), the far-LOD records (`TM2_PORT_FAR_LOD`) |
| textures | its +8 fade definitions (`clutAnimDefs` of id key + 8, while banks or CLUT copies are live; a running fade of it ends, the last definition moves into the hole); its CLUT copies (33-1: a spot in rows 240..255 back to the allocator, one in VRAM proper cleared in the usage bitmap; the width of each copy is noted in `portCarUnload.clut256`); its image records (below); its texture bank |
| image records | one per image of its TMS (noted at the match load's TMS pass and at each live load: `portCarUnload.rec_first` / `rec_end`). A banked group's all go (its bank goes); a bank-0 group's go when another usable record has the same key, while the only record of a key stays: bank 0's VRAM is never given back, it is a cache by key, so a later car of that type takes its pixels by key as at the load and costs no VRAM. With `TM2_PORT_TEX_SHARED` a `portCarGroupNoReuse` group's records all go (their pixels stay in VRAM: see the limits). The table closes up and every range follows |
| per-key registries | body, wreck, damage stage, tire switches, turbo, parts and the freeze record of its key zeroed (as for a key no car of the load has); an overflow key free again (`key_type` 0): the next duplicate takes the lowest free one (`portCarRegisterRoot`: the same order as before when nothing was unloaded) |
| slots | object = `portCarDropIn.dummy` (the zeroed object the vacant AI records use: not drawn, out of the hit pass, far below, id 0), type 0, kind 2, index -1, group `TM2_PORT_SLOT_FREE` (-1), key 0, `portCarSpecialAs` -1, `portBalanceRideTop` 0; `carWorldObjectCount` drops over free slots at its end. A live load takes the first run of free slots (Minion two) |
| group | `group_key` 0, `portCarBindGroup` entries naming it 0, `portCarDropIn.group_live` 2 (its number free: the next live load takes the lowest free number), `group_dmd` 0 |
| DMD bytes | back to the area's free list (below) |

What is **not** reset (kept as for any key or slot no car of the load has): the tire accumulators and spin angles of
the key (retail keeps them across loads; a new car with the key starts where the last one left them), the slot's
`carPair*` rows (rewritten every frame), a CLUT fade that is not per key (no banks, no copies: one per type, shared,
as retail).

**The DMD area's allocator.** `portCarUnload.range_start` / `range_size` (66 ranges, by address, 4-aligned, coalesced)
below the tail `portCarDropIn.dmd_next` .. `dmd_end`; a range that reaches the tail becomes tail again. A load takes the
first range that holds its file (first fit), else the tail. Every car DMD of the match start is a range of its own
(noted by `portCarLoadGroup`), so a car of the match start can be given back too. Nothing in the area can move: the
registries, the scene objects, the animation list and the models of every other car point into their copies (and
retail's DMD format is relocated once, at the install), so there is no compaction; the next match load resets the area.
**Fragmentation** (`tools/check_car_groups.py --unload-model`, the disc's 15 car files, 24,940 (Hammerhead) ..
39,168 (Roadkill) bytes, Dark Tooth 61,660; 2 MiB area): 20,000 random leave + join events with 16 cars, with 8..32
and with 32 never had a load refused (at most 13 / 22 / 22 holes); with 16..40 cars of which half are Dark Tooth (about
1.9 MB live) 4,701 of 15,117 loads were refused although enough bytes were free in total (the largest block was
27,840 bytes at one of them): first fit strands bytes in holes smaller than the next file once the area is nearly
full. So: the 2 MiB area holds every match within the slot and key limits below as long as the live DMDs stay well
under it (32 normal cars are at most 1.25 MB); a load that does not fit is refused with `ERR_NOLOAD` and changes
nothing; the port reads `room.dmd_largest` (and `dmd_free`, `dmd_holes`) and may give back free cars to make room. The
low area (bit 1 clear) works the same with retail's room up to the EXE.

**Coordinate systems.** `csAlloc` takes the last object an unload gave back first (a stack, `portCarUnload.cs_free`,
512 entries), else the pool's next one as retail; with nothing given back the list is empty and it is retail's bump
allocator, so the match load and every retail allocation are unchanged. The list is dropped when the pool restarts
(`csPoolInit`: `csPoolCount` below the count when the list last grew) and at each load. A given-back object is unlinked
from the drawn-object list (`csDrawListHead`), not drawn, model 0; `csAlloc` resets it (`csReset`) when it hands it
out again. A live load checks the pool's rest plus the list for its 24.

**`int portCarRoom(struct TM2PortCarRoom *room)`** fills (returns `dmd_largest`): `dmd_free` (holes + tail),
`dmd_largest`, `dmd_holes`, `slots_free` (below the vacant slot 63, given back or never used), `slots_run` (the longest
run: Minion's file needs 2), `keys_free` (of the 32 overflow keys; a type no loaded car has needs none), `groups_free`,
`cs_free`, `anim_free` (of 100; an install takes up to 16), `clut_defs_free` (up to 4), `records_free` (of 4,096; one
per image), `banks_free` (banks live), `rows16_free` / `rows256_free` (rows 240..255), `groups_loaded`, `unloaded`.
**`int portCarCanLoad(tms, tms_size, dmd, dmd_size)`**: 0 when `portCarLoadGroupLive` would load these files now, else
the error it would return (`ERR_BUSY`, `ERR_BUFFER`, `ERR_NOLOAD`, `ERR_TYPE`); VRAM is found out only by the parse
(`ERR_VRAM` from the load, which changes nothing either). Neither changes anything.

**Census: what can meet a slot or a key that was given back** (`tools/check_car_groups.py` check 12 keeps both lists:
a new reader fails until reviewed). The free slot looks like a slot no car of the load has: kind 2 (no driver: every
binder and every loop by record skips it), type 0 (no search by type finds it), key 0, the dummy object (never drawn,
out of the hit pass, no model: `carChoiceRegisterGroup` does nothing with it). Every entry of a free key is 0, as for a
key no car of the load has, and no slot has the key any more, so only a stored key could reach it: the stored keys
(`spin_target`, `crush_owner`) are let go by `portCarResetSpecials` first. The readers and their verdicts:

| reader | what it meets |
|---|---|
| `arenaSetup` (slot) | carChoiceRegisterGroup(dummy, key 0) for a free slot: the dummy has no model, nothing registered |
| `carBindAiByType` (slot) | searches by type (never 0) and by group (portCarSlotOfGroup: a free slot has group -1) |
| `carBindAiSlot` (slot) | binds the slot it is given (a group's slot from the binders or portCarStartAi: never a free one) |
| `carBindHumanByType` (slot) | searches by type (never 0) and by group (portCarSlotOfGroup: a free slot has group -1) |
| `carBindHumanSlot` (slot) | binds the slot it is given (a group's slot from the binders or portCarStartHuman) |
| `carDestroyed` (slot) | the dying record's own slot (bound: an unload refuses a driven group) |
| `carFindById` (slot) | id 0 of the dummy: kind 2 is never "found" (as a spare car's object); index -1 unused then |
| `carFindByObject` (slot) | the dummy: kind 2 is never "found" |
| `carFindSlotByOwner` (slot) | searches by type (never 0) |
| `carFromSlot` (slot) | kind 2 / index -1: returns 0; its callers skip 0 (weaponFireSpecial's shockwave loop) or pass a slot of this frame's contacts or attackers (the dummy collides with nothing; credits were cleared) |
| `carObjectCount` (slot) | getter; its caller weaponFireSpecial skips carFromSlot 0 |
| `carObjectFromOwner` (slot) | by key (a free slot has key 0, never searched); no match returns carSlotObjects[count]: a free slot past the trimmed count holds the dummy, as retail's one-past entry |
| `carObjectId` (slot) | a bound record's slot |
| `carPairsUpdate` (slot) | every pair of slots: the dummy's position (far below), recomputed each frame, never in range |
| `carRegisterModel` (slot) | writes the slot a live load takes (the count is set to the free run for the install) |
| `carTablesClear` (slot) | load reset |
| `sessionBindCars` (slot) | the binders above; reads sessionPortOpponentCars through its base minus the 9 record slots (TM2_SETTINGS_OPPONENT_CAR), which the link places in the slot tables: no slot is read |
| `sessionGetOpponentCar` (slot) | reads sessionPortOpponentCars through its base minus the 9 record slots (TM2_SETTINGS_OPPONENT_CAR), which the link places in the slot tables: no slot is read |
| `sessionLoadArenaAndCars` (slot) | reads sessionPortOpponentCars through its base minus the 9 record slots (TM2_SETTINGS_OPPONENT_CAR), which the link places in the slot tables: no slot is read |
| `sessionSetOpponentCar` (slot) | reads sessionPortOpponentCars through its base minus the 9 record slots (TM2_SETTINGS_OPPONENT_CAR), which the link places in the slot tables: no slot is read |
| `settingsGetOpponentCar` (slot) | reads sessionPortOpponentCars through its base minus the 9 record slots (TM2_SETTINGS_OPPONENT_CAR), which the link places in the slot tables: no slot is read |
| `settingsSetOpponentCar` (slot) | reads sessionPortOpponentCars through its base minus the 9 record slots (TM2_SETTINGS_OPPONENT_CAR), which the link places in the slot tables: no slot is read |
| `carBodyObjectHide` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |
| `carBodyObjectRegister` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carBodyObjectShow` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |
| `carBodyPoseUpdate` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |
| `carChoiceRegisterGroup` (key) | arenaSetup and the live load: a slot's key (a free slot passes 0 with the dummy, which has no model: nothing written) |
| `carFreezeBuild` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carFreezeResetAll` (key) | steps all 48 keys' freeze records and turbo objects: a zeroed record and a null turbo are what every unused key holds (animSetStep is null-safe) |
| `carFreezeSetState` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |
| `carObjectFromOwner` (key) | compares slot keys (a free slot has key 0) |
| `carPartRegisterAxel` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carPartRegisterDarkTooth` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carPartRegisterHammerhead` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carPartRegisterMinion` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carPartRegisterMrSlam` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carPartsAnimate` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |
| `carRegisterTireSwitch` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `carSetDamageStage` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |
| `carWreckModelGet` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |
| `carWreckModelRegister` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `objRegistryAdd` (key) | registers under the key of the group being installed (portCarLoadKey): a key handed out at that install |
| `objRegistrySetStep` (key) | the type + 6 path reads turbo[type >> 4]: 0 when the type's first car is gone, as for a type not in the match |
| `uaswToggleCarTireState` (key) | a bound record's or a slot's key (no slot has a given-back key; stored keys were let go): never a free key; a zero entry is a key no car has, which retail meets for every type not in the match (null-safe) |

**Limits and what is still not possible:**

- **No compaction** of the DMD area (see the allocator); the area's room is the port's to watch (`portCarRoom`).
- **Bank 0's VRAM is never given back** (without banks): an image stays where the first car of its type put it, as a
  cache by key (a later car of the type takes it); so VRAM grows only with types never loaded before in the match. A
  `portCarGroupNoReuse` car (a mod car) without banks leaves its pixels in VRAM for the rest of the match (its records
  go: nobody may take them). With banks every group's bank is freed.
- **Single special state without per-key specials:** a car of Twister, Axel, Mr. Slam, Shadow (and Dark Tooth without
  `TM2_PORT_DARK_TOOTH`) can be given back only while no other loaded car has its type (`ERR_TYPE`).
- **Keys shared past 32 duplicates** (retail sharing) cannot be given back (`ERR_SHARED`).
- **Humans:** at most `TM2_MAX_HUMANS` (4 in the variant; 16 with `TM2_PORT_HUMANS`, `HUMANS16_PLAN.md`); total active
  cars <= `TM2_PORT_CARS` (16); AI records <= `TM2_MAX_AI_CARS` (16), vacant ones included (a vacant record is
  reused first by `portCarStartAi(-1, ...)`).
- **At once:** 63 car slots (64 car roots at the load take the vacant records' slot: AI removal is then refused,
  `ERR_NOLOAD`), 63 load groups, 15 type keys + 32 overflow keys, `TM2_MAX_COORD_SYSTEMS` coordinate systems, 100
  animations, `TM2_MAX_CLUT_ANIM_DEFS` CLUT animation definitions, 4,096 image records: now limits on what is loaded at
  the same time, not on what a match loads over its life.
- **The last human cannot leave** (a dedicated server needs the round-end rule first, Q10).
- **The attract demo** is refused (it drives and respawns every AI record itself).
- **Determinism:** the starts call `rand()` as a respawn does (the start heading, `aiCarStart`'s `rand() % 3`, the
  start picker only when no car is alive); the loads call none; an unload calls none itself (ending a Mr. Slam claw that
  holds a car spins it out as the game does: `carSpinOutById`; its animations leave the list, so their random frames
  stop). Every peer must make the same calls at the same frame, in the same order.
- **Not runtime-tested** (static proofs and the host model only, 14.9).

### 14.8 CHECK items of `PER_CAR_PLAN.md` B.6 / B.7, answered

- **The render shared buffer is free between frames** (B.7.1): CONFIRMED. `renderSharedWorkBuffer` (retail
  801F5A90, about 6 KB) is used by `renderInit` (the hit-detection stack, the dbscan overflow stack), `renderDrawModel`,
  `renderInitState` and `renderScene`'s near list: scratch inside a frame, nothing kept across frames, so the install's
  `dbscanHierarchy` may use it (and `renderMatrixSlots`) between frames. **`modelRelocateStack` is not that buffer**:
  `bitmapSetupStorage` stores the buffer's address *through* the pointer (`lw v1, modelRelocateStack; sw v0, 0(v1)`),
  and the pointer is 0 in retail (nothing assigns it), so `modelRelocate` and `weaponSlamFindTarget` keep their
  traversal stack at address 0 (low RAM): at the load and in every Mr. Slam grab in retail, and now also in a mid-match
  install. Kept as retail (the port maps guest RAM; the plan's text and `config/US/names.tsv`'s note were wrong).
- **Far-LOD / far-distance bookkeeping in `assetInstallImage` is append-only** (B.7.1): CONFIRMED. `portFarLodScan`
  (`TM2_PORT_FAR_LOD`) appends each root's LOD records to one global table (de-duplicated, 2,048; past it a record keeps
  its retail max); only `portFarLodScanLevel` (the match load) resets it; `portFarLodBegin` / `End` extend and restore
  the records inside `renderScene`, so a scan between frames is safe. `TM2_PORT_FAR_DISTANCE` has no hook in the install.
- **Vacant AI records are safe in every loop over `aiCarTotal`**: CONFIRMED by reading every reader (14.4) and kept by
  check 12. Three per-frame readers dereference `aiCarObjects[j]` unguarded (`carContactsCombine`, `groundFiresUpdate`,
  `projectileHitTest`), `carObjectId` indexes `carSlotObjects` by the record's slot, `aiCarDamageUpdate` divides by
  `max_hp`: hence the dummy object on slot 63 and `max_hp` > 0; `sessionBindCars` (modes 3/4) would rebind a vacant
  record and `carBindAiByType` would raise `aiCarTotal` for a free slot: hence the skip; type 0 keeps
  `aiCarsCountRemaining`'s level-8 rule from reviving `aiCars[0]`; the attract demo drives every record: refused.
- **The projectile pools' owner fields** (B.6.3): CONFIRMED. `fxRecords` +0x42 owner = object id (written in
  `weapon_fire_missiles.c`, four missile kinds, and `weapon_fire_specials.c`; `weaponApplyHit` turns it into a slot
  through `carFindById`), +0x38 target = a car object's position, +0x3C the boomerang's return object; ricochet shots +0x02
  owner id; bullets +0x24 owner id; drops +0x04 dropper id (+0x06 its slot); arcs +0x00 shooter id (lightning: record
  index), +0x02 shooter slot, +0x08 victim id, +0x0C / +0x10 record positions; shields +0x02 human index, +0x04 car
  object; flame emitters +0x1C car object. All handled (14.3).
- **The key stores** (B.7.3): `spin_target` is s16, `crush_owner` s32 (step 3): keys up to 0x2F0 fit.
- **A new key's state**: the overflow keys' special state (`portCarSpecials`, Dark Tooth's initial values included) is
  reset at the load for all 32 and untouched until a car of that key installs; a type key the match never held binds
  the retail globals, which no other car uses; the freeze record is zeroed before the install; the engine sound is per
  slot (`portCarSound`), so a new slot needs nothing.
- **The live install's `rand()`**: none in the install path for a car DMD (CONFIRMED by reading; check 12 lists what the
  static graph reaches: worldLoadObject's level-object cases and the animation steppers stored as function pointers).

### 14.9 Status and proofs

**Storage** (`.port_bss`, every variant, after `portCarCluts`, so nothing before them moves; tsv group `one`):

| variable | size | default variant | `TM2_PORT_CARS=16` (the full stacks of 14.9) | contents |
|---|---:|---|---|---|
| `portCarDropIn` | 936 | 0x804FEE48 | 0x8053B2F8 | `struct TM2PortCarDropIn` (`car_dropin.h`): +0x000 `live`, +0x004 `removed`, +0x008 `started`, +0x00C `loaded`, +0x010 `vacant[32]` (AI record j vacant), +0x030 `group_live[64]` (1 loaded live, 2 unloaded), +0x070 `group_dmd[64]`, +0x170 `group_size[64]`, +0x270 `dmd_next`, +0x274 `dmd_end`, +0x278 `next_group`, +0x27C `start_live`, +0x280 `dummy` (the vacant records' scene object, 0x128 bytes) |
| `portCarLiveTex` | 270,472 | 0x804FF1F0 | 0x8053B6A0 | the usage bitmap (65,540), the slot table (40,960), the image records (98,304), the bitmap snapshot (65,540), the rows 240..255 snapshot (128); internal |
| `portCarUnload` (since the unloading step, 14.7; defined last of the port storage) | 2,344 | 0x805432A8 | 0x80581688 (full car stack; 0x8058D644 every define + `TM2_PORT_HUMANS=16`) | `struct TM2PortCarUnload` (`car_dropin.h`): +0x000 `unloaded`, +0x004 `cs_count`, +0x008 `cs_high`, +0x00C `range_count`, +0x010 `range_start[66]`, +0x118 `range_size[66]`, +0x220 `cs_free[512]` (u16), +0x620 `clut256[64]`, +0x720 `rec_first[64]`, +0x820 `rec_end[64]`, +0x920 `rec_group`; read through `portCarRoom` |

The port reads `portCarDropIn` between frames (which AI records are vacant, which groups were loaded live); it writes
nothing there. Read the addresses from the build's `symbols.tsv`.

- **Built and proved** (`tools/build_source_only.py` + `verify_reloc.py` per configuration, the variant and its
  placement check as `build.py` links them, port RAM end 0x80C00000; then the full
  `build.py --source-only --sdk-from-lib --relocatable --port-variant`, `work/build-20261013-165534-786000`). The retail images keep their
  hashes: hybrid `9219f7baâ€¦`, source-only `d755c86eâ€¦`, with `--sdk-from-lib` (= E0) `f557c1e3â€¦`. Every configuration
  passes its proof against E0 and the placement check:

  | configuration | variant sha256 | `port_changed` | port sections end (RAM end 0x80C00000) |
  |---|---|---|---|
  | no port define (published `out/TM2_SOURCE_PORT.EXE`) | `451fc414â€¦` | none (code unchanged; `.port_bss` + 271,408 bytes) | 0x80541400 |
  | `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP` + `TM2_PORT_CAR_DROPIN` (the minimum) | `5b04da77â€¦` | 115 (114 + `bitmapSetupStorage`) | 0x805821C0 |
  | the full car stack (`TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`, `TM2_PORT_CAR_SPECIALS`, `TM2_PORT_DARK_TOOTH`, `TM2_PORT_CAR_SOUND`, `TM2_PORT_CAR_CLUTS`) + `TM2_PORT_CAR_DROPIN` | `0bd315e6â€¦` | 132 (131 + 1) | 0x80586748 |
  | every port define (+ `TM2_PORT_TEXTURE_BANKS`, `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE`) | `44481597â€¦` | 141 (140 + 1) | 0x80587B98 (6,784,104 bytes below 0x80C00000) |

  `TM2_PORT_CAR_DROPIN` adds 271,408 bytes of storage (every variant) and about 10 KB of port code. Without
  `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS` or `TM2_PORT_CAR_SWAP` the define stops the build (`#error` in
  `car_dropin.h`, checked with the compiler).
- **`tools/check_car_groups.py` check 12** (new): in the three drop-in builds the 13 functions are in `.port_text`; the
  hooks are called (`bitmapSetupStorage` -> `portCarLiveStorage`, `sessionLoadArenaAndCars` -> `portCarLoadDone`,
  `sessionBindCars` -> `portCarBindCount` / `portCarBindSkip`, `portCarGroupsReset` -> `portCarDropInReset`,
  `portStartNodePick` reads `portCarDropIn`); every holder of the census (15: projectiles, arcs, drops, shields,
  ricochets, bullets, effect sprites, flame emitters, AI targets, the watched AI, tornado, leap, strike, the Dark Tooth
  swoop target and flag) is written by the code `portCarRemove` reaches, which calls the blocks for the record fields;
  `portHumanCompact` writes every per-human array and id / address holder (21); the call-graph self-test passes
  (the match load reaches `arenaResetSystems` and, through `dbscanHierarchy`'s function pointer, `worldLoadObject`);
  `portCarLoadGroupLive` reaches 161 / 182 / 195 functions and none of the 41 load-time-only or CD functions, and of
  `rand()`'s callers only the level-object cases of `worldLoadObject` and the animation steppers (never run for a car
  DMD by the install); the 23 game functions that read `aiCarTotal` / `aiCarObjects` / `aiCarObjectSlots` /
  `aiCarTargets` are the reviewed ones (14.4). `--expect-functions` adds `EXPECTED_DROPIN` (115 / 132 / 141). Checks
  1-10, `--dmd` and `--key-model` pass as before. Informational lines: `portCarStartHuman` reaches the match-wide
  resets only through `carBindHumanSlot` (`TM2_DROPIN_START_RETAIL`).
- **Retail behaviour with nothing called** holds by construction (14, first paragraph): every hook returns the retail
  value while `portCarDropIn.live` is 0; `bitmapSetupStorage` moves where the bookkeeping is, not what is done.
- **Unloading (14.7, 2026-10-15)**: built and proved the same way on the objects of `work/build-20261015-223109-608696`
  (E0 = its `source_reloc_e0`), then the full build `work/build-20261016-014135-215616` (published `out/TM2_SOURCE_PORT.EXE` = `2bcf231c…`). The retail images keep their hashes (hybrid `9219f7ba…`,
  source-only `d755c86e…`, `--sdk-from-lib` `f557c1e3…`). Every configuration passes its proof and the placement check
  (`tools/check_all.py`: 19 checks, 0 failed, with `--models`):

  | configuration | variant sha256 (P4) | placement (P2) | port_changed (P4) | port sections end |
  |---|---|---|---|---|
  | no port define (published `out/TM2_SOURCE_PORT.EXE`) | `2bcf231c…` | `b95e6fed…` (unchanged) | none | 0x80543D90 |
  | `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`, `TM2_PORT_CAR_DROPIN` | `84ac806d…` | `8955114f…` | 119 | 0x80588F30 |
  | + `TM2_PORT_TEXTURE_BANKS`, `TM2_PORT_CAR_CLUTS`, `TM2_PORT_CAR_SPECIALS`, `TM2_PORT_DARK_TOOTH`, `TM2_PORT_CAR_SOUND` | `c552d89c…` | `cf504db1…` | 140 | 0x8058E478 |
  | every port define + `TM2_PORT_HUMANS=16` | `7dd37869…` | `dfc8d380…` | 232 | 0x805A1B4C (6,677,684 bytes below 0x80C00000) |

  The unloading code (31 port functions) is 11,768 bytes of `.port_text` (every define); `portCarUnload` adds 2,344 bytes of `.port_bss` in every variant.
  `tools/check_car_groups.py` check 12 (extended): `csAlloc` calls `portCsTake`, `portCarLoadGroup` notes each car file
  (`portDropInNoteLoad`), `portCarRegisterRoot` takes the lowest free key (`portCarFreeDupKey`), `portCarRemove` stops
  the slot's sound; the 37 / 44 / 47 holders of `UNLOAD_HOLDERS` (by define) are all written by the port code
  `portCarUnloadGroup` reaches, which calls the shared blocks; it reaches no load-time-only function and no CD read;
  the 22 game readers of the slot tables and the 22 of the per-key registries are the reviewed ones (14.7). Check 8 (per-key
  specials) passes: the unload reads the single special state inside the key's selection. Check 15 (`--unload-model`,
  new) passes (14.7). `--expect-functions` adds `csAlloc` (`EXPECTED_DROPIN`).
  Port tests to add: (9) a long session: 1 human + 15 bots, 200 join / leave cycles of random types (Dark Tooth among
  them) with the leavers' cars given back: `portCarRoom` stays flat (DMD free, slots, keys, coordinate systems,
  animation entries, CLUT definitions, image records), no stale object drawn, no crash; (10) unload while another car's
  missile with that car's model is in flight (Sweet Tooth's spiral, Roadkill's boomerang): it takes another copy's node
  or ends; (11) Gun Game: a borrower of a Shadow / Axel special whose lender is unloaded fires its own again; (12) unload
  a car of the match start, then load another type into its hole; (13) `ERR_TYPE` / `ERR_BUSY` refusals change nothing.
- **Not runtime-tested.** Port tests: (1) the 11-car comparison with the define on and nothing called (expected
  differences above); (2) 1 human + 15 bots, a second player joins on a type in the match (a bot leaves, the player
  spawns far from everyone, team colour), leaves again, a bot fills the place; (3) a join on a type not in the match,
  with banks and without (Paris: refused with `ERR_VRAM` when it does not fit; a type already loaded costs no VRAM); (4)
  a leave mid-special (Mr. Slam holding a car, a tornado, a strike, a leap, burning, frozen, invisible, mines and a
  remote bomb out, missiles homing on it, a boomerang out, a shield up): everything of it gone at once, no orphan
  explosion, no kill credit to the gone car; (5) human 1 of 3 leaves: human 2 continues as human 1 (its pad, view,
  HUD remapped by the port), its projectiles still credit it; (6) modes 3/4 across rounds: vacant records stay vacant,
  joined players rebind to their cars; (7) loads until `ERR_NOLOAD` (slots, keys, DMD space); (8) the attract demo
  refuses.

## 15. Request 35: any car fires another car's special (`TM2_PORT_SPECIAL_AS`)

Gun Game's special levels: `portCarSpecialAs[slot]` (the borrowed type, -1 own) set by `portCarSpecialAsSet(slot,
type, ammo)` between frames; Shadow's strike and Axel's shockwave get an as-entry per car slot in the 33-2a selection
mechanism (section 10, values 33..96); `portCarSpecialLoad` loads missing car files as parked spare groups (needs
`TM2_PORT_CARS` and `portCarOptions` bit 1); the swap's `portCarResetSpecials` ends a slot's borrowed special and
resets it to its own. 10 types borrowable, Hammerhead, Grasshopper, Twister, Mr. Slam and Dark Tooth refused. All of
it, with the API, addresses, costs and proofs: `GUN_GAME_SPECIALS.md` 4 ("As built"); `tools/check_car_groups.py`
check 13.

## 16. Request 33 step 4: the rest (`TM2_PORT_CAR_SOUND`, `TM2_PORT_CARS`)

Request 33 step 4 (`PER_CAR_PLAN.md` C.4, D item 5): what was still shared between two cars of one type after
33-1, 2a, 2e, 3 and request 35, re-checked against the census (`PER_CAR_PLAN.md` A.2; a fresh read of every sound
call and every per-type global written during a match). Most of the census was already done: the pools of C.4
(`renderWorldModels` 20 -> 40, `weaponFlameEmitters` 2 -> 16, `clutAnimDefs` / `clutAnimRunning`, the claw records,
arcs, shields, remote bombs, ricochets) by step 2 (section 7.1), the fade per car by 33-1 (section 13), the
Dark Tooth wobble by 33-2e. Left were sounds and the sound request pool. No new define: the sound changes are under
`TM2_PORT_CAR_SOUND` (needs `TM2_PORT_CAR_GROUPS`), the pool under `TM2_PORT_CARS`.

### 16.1 The human loops by side (players 2..; CONFIRMED by reading)

Retail plays a human's turbo loop (sound 0, `carThrottleSteer`), gun loop (9, `weaponTick`) and burn/lava loop (32,
`carSoundUpdate`) with `soundStartPlayerLoop(id, .., player)` -> `soundMgrPlaySided`: one fixed list per side and
id (side 0 or 1, max 1 each), flags `soundTurboLoopActiveP1/P2`, `soundGunLoopActiveP1/P2`,
`soundLoop32ActiveP1/P2`. For a player of 2 or more the request is taken from the pool and set up, then neither
listed nor freed (`FOUR_HUMANS.md` 3): every frame of turbo, guns or burning by human 3 or 4 loses one of the 35
requests, and all game sound stops within about half a second. With `TM2_PORT_CAR_SOUND`:

- players 0 and 1 make retail's calls, word for word (`portHumanLoopPlay` / `portHumanLoopStop` call
  `soundStartPlayerLoop` and, as retail, `soundStopPlayerLoop` only when the side's flag is set);
- a player of 2 or more plays the loop as **instance slot + 1** of the id in the id's own list (`soundMgrPlay`, the
  lists of 0, 9 and 32 are not used by retail; they get room for `2 * (TM2_MAX_HUMANS - 2)`), merged by instance
  (33-3's `portSoundPlayInstance`), and stops it with `soundStopInstance` when it was started
  (`portCarSoundLoops[slot]`, bits 0..2).

This does **not** depend on `portCarOptions` bit 4: retail has no path for these players (only a leak). So with the
define, bit 4 clear and three or four humans, the build differs from the variant without the define in exactly
this (expected; with two humans it is equal). A 4-human port build needs `TM2_PORT_CAR_SOUND` (and so
`TM2_PORT_CAR_GROUPS`) to keep its sound. Up to 16 human records (`PER_CAR_PLAN.md` D): the instance is the car
slot, so it holds for any human index. A remote human's turbo / gun / burn (16.7 question 1): answered by request 37
answer 7 and built in `HUMANS16_PLAN.md` H3 (with `TM2_PORT_HUMANS` the sides follow the local slot, a remote human's
loops play positionally, as a computer car's).

### 16.2 Twister's tornado sound (0x35) and the special-fire sounds (bit 4)

- Retail: `weaponFireSpecial` plays 0x35 (`soundPlay`, kind 12: a new request, dropped while the list is full;
  list size **1**) and `carDestroyed` stops it for **every** Twister (`soundStop(0x35)`) when any Twister dies. With
  33-2a each Twister has its own tornado, so a second Twister's tornado was silent while the first one's sound
  played, and one Twister dying cut off the other's. With bit 4: the Twister's own instance (slot + 1,
  `portCarTornadoSound`), stopped per car (`portCarTornadoSoundStop` from `carDestroyed`, from the swap's
  `portCarResetSpecials` and from request 35's `portCarSpecialAsEndOwn`; bit 0x08 of `portCarSoundLoops`). Bit 4
  clear: retail's `soundPlay(0x35, ..)` / `soundStop(0x35)` from the port functions.
- The special-fire one-shots (`weaponFireSpecial`, `weaponArcsDraw`, `weaponSlamSwingUpdate`: 0x27, 0x29, 0x2B, 0x2D,
  0x31, 0x35, 0x37, 0x39, 0x3B, 0x3D; kind 12) are dropped while their list is full; retail's lists hold 1 (0x29,
  0x2D, 0x35, 0x39), 2 (0x27, 0x3B) or 3. So two Grasshoppers leaping, or two Mr. Slams swinging, within the
  sound's length: the second is silent. With bit 4 `portCarSoundFrame` gives each of these lists room for
  `TM2_PORT_SPECIAL_VOICES` (4); bit 4 off restores retail's sizes. No call changes (they stay one-shots by id).

### 16.3 The sound request pool (`TM2_PORT_CARS`)

Retail: 35 request objects of 120 bytes on `main`'s stack (`sub_8012C8DC`), their pointers in `sndObjPool`
(8017F1EC, .data), `sndPoolReset` / `sndPoolResetAll` count 35, `sndPoolAlloc` searches them. Every playing sound
holds one (16 engines with bit 4 hold 16). With `TM2_PORT_CARS=N`: `TM2_MAX_SOUND_REQUESTS` = 4 N (64 for 16,
`players.h`), the pointer table moves to `.port_bss` (tsv `sndObjPool`, group `sndreqs+`, linear) and the objects
are `portSndObjSlots` (`.port_bss`, 120 bytes each; main builds them there instead of on its stack). As the sound
instances (step 2): the allocator hands out the first `TM2_SOUND_REQUESTS_LIVE` = 64 while `portCarOptions` bit 3 is
set (read at each allocation), else 35 (a full retail-sized pool refuses as retail). The engine reserve of 33-3
(`TM2_PORT_SOUND_POOL_RESERVE` 12) counts the free requests within that live size. Without the define nothing
moves (the default variant's `sndObjPool` stays at its retail address).

### 16.4 What changed

| VA | function | unit | change |
|---|---|---|---|
| 800DD304 | `carThrottleSteer` | `car_control.c` | (`TM2_PORT_CAR_SOUND`) turbo loop -> `portHumanLoopPlay(0, .., player, slot)`, the stops -> `portHumanLoopStop(0, player, slot)` |
| 8010388C | `weaponTick` | `weapon_tick.c` | (`TM2_PORT_CAR_SOUND`) gun loop of a human -> `portHumanLoopPlay(9, ..)` / `portHumanLoopStop(9, ..)` (the AI's shared gun loop unchanged) |
| 8011DB74 | `carSoundUpdate` | `screen_card_draw.c` | (`TM2_PORT_CAR_SOUND`, already 33-3's) burn/lava loop -> `portHumanLoopPlay(32, ..)` / `portHumanLoopStop(32, ..)` |
| 80105A7C | `weaponFireSpecial` | `weapon_basic.c` | (`TM2_PORT_CAR_SOUND`) Twister: `portCarTornadoSound(slot, vol, pan)` (the AI's locate / `PadChkMtap` / pan as retail's `SOUND` macro) |
| 8011BE10 | `carDestroyed` | `car_destroyed.c` | (`TM2_PORT_CAR_SOUND`) a Twister's death: `portCarTornadoSoundStop(slot)` |
| 8012C8DC | `main` (`sub_8012C8DC`) | `shell_main.c` | (`TM2_PORT_CARS`) the request objects in `portSndObjSlots` |
| 8015F0EC, 8015F194 | `sndPoolReset`, `sndPoolAlloc` | `engine_sound_lists.c` | (`TM2_PORT_CARS`) `TM2_MAX_SOUND_REQUESTS`; the live size |
| 8015F134 | `sndPoolResetAll` | `engine_misc_accessors.c` | (`TM2_PORT_CARS`) `TM2_MAX_SOUND_REQUESTS` |

Without the defines every line is the retail C (`#ifdef ... #else <retail>`, or a macro equal to retail's constant).
Port functions (`.port_text`, `port_variant_arrays.c`, declared in `car_sound.h`): `portHumanLoopPlay(snd, vol,
pan, pitch, player, slot)`, `portHumanLoopStop(snd, player, slot)`, `portCarTornadoSound(slot, vol, pan)`,
`portCarTornadoSoundStop(slot)`; static `portSndPoolFree`, `portSpecialSoundLists`, `portLoopBit`. 33-3's
`portCarSoundFrame` / `portCarSoundOff` set and restore the special-fire lists and stop tornado instances.

Storage: `portCarSoundLoops` (64 bytes, `.port_bss`, every variant, after `portSpecialAs`): per car slot bit 0 turbo,
1 gun, 2 burn/lava (players 2..), 3 tornado (bit 4) started as the slot's instance. With `TM2_PORT_CARS`:
`sndObjPool` (4 x 64) and `portSndObjSlots` (120 x 64), defined last. Addresses: the build's `symbols.tsv`.

### 16.5 Still shared (and why)

| what | where | why it stays |
|---|---|---|
| the AI's gun loop (9) | `soundStartSharedLoop(9, .., AI index)` -> the generic list (max 1) | one voice for every computer car's guns, the nearest wins (merge by instance, else the farther replaced): retail's design for any line-up, not per type; `carDestroyed` and the round end stop all of them (`soundStopSharedLoop(9, -1)`, retail) |
| the skid / shield hum (21) | `carSoundUpdate`, instance = player index, list max 1 | per human already; one voice for all humans (nearest wins), as retail with two |
| sound 6 (impact / crash; earlier notes called it "the burning sound") | `fxImpactDebris`, `carTumble`, the swoop hit | a one-shot effect by id (list 2, dropped when full), not per type |
| the other one-shots (horn 3, 1, 7, 25, 26, 34, ...) | many | effects by id; merged or dropped as retail, the same with any line-up |
| Roadkill's boomerang (0x33) | `weaponFireSpecial`, instance = projectile slot, list 1 | already by instance; a bigger list would leave a returned boomerang's loop playing until the last one ends (`weapon_fire_specials.c` stops 51 only when none flies) |
| `soundIsActive` per id | `weapon_fire_specials.c` | one flag per id; the per-car paths do not rely on it |
| the invisibility fade with bit 7 (and banks) off | `clutAnimStartFadeOut(type + 8, ..)` | a twin's fade fades the shared CLUT row, and a second start while the first runs saves the faded palette (the row can stay dark): fixed by 33-1's bit 7 (one fade per car key); with bit 7 off it is retail's sharing (the guard `PER_CAR_PLAN.md` C.4 calls optional was not added: it would act only where bit 7 is off) |
| weapon pools not raised (mines 23, drops 25, ground fires 30, effect sprites 150, bullets) | | not per type; the port measures (Q9, Q11) |
| the 24 SPU voices | | 44 for the game with `TM2_PORT_SPU48` |

### 16.6 Status and proofs

- **Built and proved** (`tools/build_source_only.py --relocatable --sdk-from-lib --port-variant` per configuration,
  `verify_reloc.py` against E0, and the placement check with `--port-limits 2` as `build.py` links it; port RAM end
  0x80C00000). Every configuration passes its proof and the placement check:

  | configuration | variant sha256 | `port_changed` | port sections end |
  |---|---|---|---|
  | no port define | `22139e4b…` | none (code unchanged; `.port_bss` + 64 bytes: `portCarSoundLoops`) | 0x805430E0 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SOUND` | `d50c0696…` | 54 (33-3's 52 + `carThrottleSteer`, `weaponTick`; `weaponFireSpecial`, `carDestroyed` were already step 1's) | 0x805449B0 |
  | `TM2_PORT_CARS=16` | `618d1c87…` | 75 (step 2's 71 + the 4 of the request pool) | 0x80581A30 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CARS=16` | `ec8bc30e…` | 118 | 0x80581F30 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CARS=16` + `TM2_PORT_CAR_SOUND` | `d2b9733d…` | 124 | 0x80583330 |
  | the car stack: GROUPS, CARS=16, SWAP, TEXTURE_BANKS, SPECIALS, DARK_TOOTH, SOUND, CLUTS, DROPIN, SPU48, SPECIAL_AS | `01300d01…` | 181 (148 + SPU48's 33) | 0x8058CCF8 |
  | + `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` (every port define) | `3e266318…` | 188 | 0x8058DD68 |

  The car-stack and all-defines builds include the request 35 fix of `portCarSpecialAsReset` (`GUN_GAME_SPECIALS.md`
  4.4). The full `build.py --source-only --sdk-from-lib --relocatable --port-variant`
  (`work/build-20261014-072110-299158`) keeps the retail images' hashes: hybrid `9219f7ba…`, source-only
  `d755c86e…`, with `--sdk-from-lib` (= E0) `f557c1e3…`; it publishes the default variant `22139e4b…`
  (`out/TM2_SOURCE_PORT.EXE`, port sections end 0x805430E0).
- **`tools/check_car_groups.py` check 9, extended:** 0x35 is a per-car id; `soundStartPlayerLoop`,
  `soundStopPlayerLoop` and the six loop-flag getters may be called only by port functions; the hooks of the four new
  callers; 12 port functions in `.port_text`, none calling `rand()`. `--expect-functions` matches in every
  configuration above (`EXPECTED_SOUND` + 4, `EXPECTED_CARS` + 4; the SPU48 / far functions are reported as
  belonging to the other defines). Negative control (`--negative-control` on GROUPS + CARS=16 without the sound
  define): check 9 fails with the retail calls, now including `carThrottleSteer` / `weaponTick` / `carSoundUpdate` ->
  `soundStartPlayerLoop`, `soundStopPlayerLoop` and the flag getters, `weaponFireSpecial` -> `soundPlay(53)` and
  `carDestroyed` -> `soundStop(53)`.
- **Retail behaviour with the options off** by construction: players 0 and 1 make retail's calls (the port functions
  call `soundStartPlayerLoop` / test the side's flag / `soundStopPlayerLoop` as the code did); the tornado and the
  special lists are retail's while bit 4 is clear; the request pool hands out 35 while `portCarOptions` bit 3 is clear.
  The one difference with every bit clear is players 2.. (16.1), where retail leaked.
- **Not runtime-tested.** Port tests: (1) 2 humans, bit 4 clear: frame by frame equal to the variant without the
  define; (2) 4 humans, turbo and guns held by players 3 and 4 for a minute: sound keeps working (it died in about half
  a second before), each player's loop stops when released; (3) bit 4, two Twisters: both tornado sounds, one Twister
  dies, the other's keeps playing; two Grasshoppers leaping together: both sounds; (4) `TM2_PORT_CARS`, bit 3: 16
  engines + effects, `sndPoolFreeCount` never 0; bit 3 clear: at most 35 requests as retail.

### 16.7 Open questions for the port

1. ~~Remote humans (online, up to 16 human records): their turbo / gun / burn loops are "local feedback" sounds
   played at the listener (volume 0, pan 0). Should a human with no local view play them at all?~~ **Answered**
   (request 37 answer 7: positional sounds of a remote car stay, as for computer cars; its own feedback is not
   heard), built in `HUMANS16_PLAN.md` H3 under `TM2_PORT_HUMANS`: the side of `soundStartPlayerLoop` is the human's
   local slot when it is 0 or 1 (`portHumanSide`); a remote human's **gun** loop goes into the computer cars'
   shared gun loop (`soundStartSharedLoop(9, distance, pan, 99, 0x100 + slot)`, positional, the nearest gun of all
   computer cars and remote humans heard, stopped per car and by `carDestroyed`'s stop-all as theirs); its **turbo**
   loop (with `TM2_PORT_CAR_SOUND`) plays as instance slot + 1 at the distance and pan of its car (retail plays no
   turbo for a computer car; the answer keeps a remote car's turbo positional), its **burn** loop as instance
   slot + 1 at `carSoundUpdate`'s positional values. A local human on slot 2-3 (no side) keeps 33-4's instance at the
   caller's full volume. Without `TM2_PORT_CAR_SOUND` a human without a side plays no turbo / burn loop (retail leaked
   a request each frame) and a remote human's gun still uses the shared loop. Without `TM2_PORT_HUMANS` nothing
   changes (players 2.. as above).
2. `TM2_PORT_SPECIAL_VOICES` (4) and the tornado: enough, or per number of loaded cars of the type?
3. The AI's shared gun loop (one voice for all computer cars): keep retail's, or one per car with bit 4 (as the
   engines; more voices)?

## 17. Request 36: a player's Dark Tooth (`TM2_PORT_DT_PLAYER`)

Moved to its own document: [`DARK_TOOTH_PLAYER.md`](DARK_TOOTH_PLAYER.md). In short: with `--port-define
TM2_PORT_DT_PLAYER` (needs `TM2_PORT_DARK_TOOTH` and `TM2_PORT_CAR_GROUPS`) a human record can drive Dark Tooth
(car-select index 14, offered with `portCarOptions` bit 8): the machine-gun button fires the boss's missiles, the special
is the boss's head swoop at the nearest car, his own head (his car key's section 11 state) hovers and sways over the
truck, no boss controller, a normal final wreck (`portCarDarkToothWreck` returns 4 for a human). Bits 0-7 of
`portCarOptions` are the steps above; bit 8 is request 36's; bit 9 is request 33-1's follow-up (13.12). `tools/check_car_groups.py` check 14.

## 18. Request 42: car numbers for 128 mod drivers (`TM2_PORT_CAR_NUMBERS`), the ordinary Dark Tooth (bit 10)

Details: [`CUSTOM_TOURNAMENT_ANSWERS.md`](CUSTOM_TOURNAMENT_ANSWERS.md) 1-3, 5 and `PER_CAR_PLAN.md` E.5. In short:

- `--port-define TM2_PORT_CAR_NUMBERS` (needs the port variant; works with or without the car defines): car numbers
  are 16 bits (0..14 retail, 0x20 + m mod m for m 0..127, `portModCars[m]`), kept for sessionSettings in
  `portSettingsHumanCar[16]` / `portSettingsOpponentCar[16]`; the record's bytes keep the mod's base car, so the bind
  (`carIndexToType`), the load's disc fallback and every byte reader see a retail car, and keys / types in a match stay
  the base's (a mod is a duplicate of its base). The getters return an int; names, plates, passwords, the ending,
  `carIsSelectable` and the attract demo handle mod numbers; `portCarLoadGroup` records the number per load group
  (`portCarNumberOfGroup`). Car select's carousel index is the number (0x20 + mods children); Challenge's picker offers
  Sweet Tooth with `portCarOptions` bit 11 while he is unlocked. 22 game functions change (with `TM2_PORT_CARS` also
  `sessionLoadArenaAndCars`); `tools/check_custom.py` check 3.
- `portCarOptions` bit 10 (`TM2_PORT_DARK_TOOTH`): the ordinary Dark Tooth (phase 1, no boss parts), per-slot boss role
  `portCarDarkToothRole` / `portCarDtBossMask` (`DARK_TOOTH_PLAYER.md` 10.1); `roundStart` and `carDestroyed` join
  `EXPECTED_DARK_TOOTH` (`tools/check_car_groups.py`); `tools/check_custom.py` check 1 proves the bit-clear gate.
- `portCarOptions` bits: 0-3 request 32, 4 sound, 5 banks, 6 boss anywhere, 7 CLUT copies, 8 a player's Dark Tooth,
  9 the whole-spot CLUT scan, **10 the ordinary Dark Tooth, 11 Sweet Tooth in Challenge's picker** (request 42).

## 19. Request 46: a parked Dark Tooth's head (`TM2_PORT_DARK_TOOTH`)

The whole answer is `PLAYTEST_ANSWERS.md` 7. In short: of a car's parts, Dark Tooth's head (249, `weaponAssetBind`
0xF9) is the only one that is a scene object of its own made **shown** (+8 = 1, +0xC = 1, at the world origin), and
only a live Dark Tooth's boss update (or request 36's player code) moves it. Every other separate object (Twister's
tornado and funnel, Shadow's strike, request 35's as-strike and Axel's ring) is made hidden and shown only by its
car's running special, which the park paths (`portCarResetSpecials`) and the round resets end; the parts of the car
models (`carPartRegister*`, Mr. Slam's arm and claw, turbo, freeze, damage stages) go with their car object, and Dark
Tooth's freeze block with his head. A Dark Tooth loaded but not driven (the port's Survival puts him out of play with
its own writes before the first frame) left the head standing at the origin, drawn and solid.

Built: `portCarDarkToothParked` (from `aiCarsCountRemaining`, every frame) hides the head of every Dark Tooth car
nobody drives (slot kind 2, or an AI record dead in wreck state 4) and shows a head it hid again, placed over the
truck, when a computer record drives the car; `portCarDarkToothParkHead(slot)` is the hide (used by `portCarParkSlot`,
`portCarParkAi`, `portCarModelOn` too); `portCarDtParkMask[2]` (8 bytes, `.port_bss` after `portCarSel`, cleared each
load) remembers the heads. Each slot's head is its key's (33-2e selection). A hidden head is out of the hit pass
(`hdpPassRun` skips `(+8 & +0xC) == 0`). No game function is added to `EXPECTED_DARK_TOOTH`; check 16 of
`tools/check_car_groups.py` keeps the hook, the park paths and the mask's users. Retail never loads BB.DMD without him
driving (level 8), so the head never stands there in retail.

## 20. Request 50: a leaving player's car to a computer driver (`TM2_PORT_HUMANS` + `TM2_PORT_CAR_SWAP`)

The hand-over (`portHumanHandOver(h, j)`: human h's car, as it is, to a free AI record; human h vacant at its index,
no compaction), the leave without a successor (`portHumanLeave(h)`: the car parked as `portCarRemove` parks it) and
the rejoins (`portHumanReturn(h, j)`, and request 50b's `portHumanRejoin(h, group, node)` on a free car, which
`portCarStartHumanAt` also uses for a vacant index with the drop-in) are documented in `HUMANS16_PLAN.md` 8. They are built from this file's swap
blocks (section 8) and section 14's reference blocks, which `port_variant_arrays.c` now also builds without
`TM2_PORT_CAR_DROPIN` when both `TM2_PORT_HUMANS` and `TM2_PORT_CAR_SWAP` are defined (the declarations up to
`portDropInRefs`, and `portDropInSlotSound`, moved up); the drop-in itself is unchanged except that, with both, it
knows vacant human records (`portCarActiveCount` does not count them, `portCarBindCount` binds the hand-overs' AI
records, `portHumanCompact` moves a record's vacancy). Request 50 (c), what the drop-in define changes for a match
that never calls it: section 14's opening paragraph and `HUMANS16_PLAN.md` 8.5.
