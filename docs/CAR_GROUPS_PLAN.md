# Port request 32: implementation plan (duplicates, 16 cars, car swap, texture banks)

**Status: step 1 (`TM2_PORT_CAR_GROUPS`) is done**, with the default decisions D1-D3: see `CAR_GROUPS.md`
(what it does, the API and addresses, the tests). Deviations from this plan, all retail-preserving:
`carBodyObjectRegister`/`carWreckModelRegister` keep retail's `id >> 4 < 15` bound on the type (Dark Tooth's
body stays unregistered, as retail); `objRegistryAdd`/`objRegistrySetStep` keep retail's table for turbo-like
ids 256..399 (not cars); the binders also fall back to the type search when the group's slot holds another
type; the root-first check is structural plus `tools/check_car_groups.py --dmd`.

**Step 2 (`TM2_PORT_CARS`) is done** too: see `CAR_GROUPS.md` 7 (limits, storage, functions, proofs). It follows
section 2 with the port's answers to section 6 and to request 33 (`DECOMP_REQUESTS.md` 32/33), which change these
defaults: the AI-record limit is its own macro (`TM2_MAX_AI_CARS` = `TM2_PORT_CARS`, not `N - humans`; humans + AI
<= N is a run-time rule), loaded cars are a separate limit (`TM2_PORT_CARS_LOADED` = 32: slots 64, keys 48, so step
1's registries grew), the DMD area is 2 MiB (a macro), every car file goes through one port function
(`portCarLoadGroup`, buffers from the port for mod cars), the texture-slot stride is 32, the pools of Q11 and Q9 are
raised (the weapon pools only while `portCarOptions` bit 3 is set at the load, so a match with the bit clear keeps
retail's sizes), the card screen is skipped with more than 9 opponents, and `tournamentOpponentCount`'s jump table
shares its section with `carIndexToType`'s (the tools now accept such a table).

**Step 3 (`TM2_PORT_CAR_SWAP`) is done**: see `CAR_GROUPS.md` 8 (API, swap sequence, what is kept and reset,
the Juggernaut in calls, proofs).

**Step 4 (`TM2_PORT_TEXTURE_BANKS`) is done**: see `CAR_GROUPS.md` 9 and `TEXTURE_BANKS.md` 6 ("As built": API,
words, the PSY-Z protocol, limits, proofs). It follows section 4 below with these changes: the run-time switch is
`portCarOptions` bit 5 (latched by `bitmapInit`); `sessionLoadArenaAndCars` is not changed (the placement allocates
a group's bank at its first image); an image the arena holds by key keeps bank 0 (no copy into the car's bank);
the stride stays step 2's macro; the CLUT animations take their bank from the registered primitive's tpage bits,
and the invisibility fade is keyed by car key; a per-group no-reuse flag (`portCarGroupNoReuse`) for mod cars; 8
game functions, 11 port functions.

Read-only planning pass, 2026-10-12, tree at `01dcd5f` (main, after PR #28). Sources: `docs/SURVIVAL.md`
2.2-2.5, 3.1; `docs/SIXTEEN_CARS.md` 1.1-1.7, 3.5, 5, 6.2; `docs/TEXTURE_BANKS.md` 2.4, 5;
`docs/FOUR_HUMANS.md`; `docs/SOURCE_BUILD.md` "Port variant" and "Port code"; `docs/FAR_LOD.md` 7 (the pattern
for a decomp-side port option); `include/semantic/players.h`, `settings.h`, `cars.h`;
`config/US/port_variant_arrays.tsv`; `src/semantic/port_variant_arrays.c`; `config/US/retail_relocs.tsv`
(a census of the car arrays was run against it, see 2.6); the C units listed below. Every VA is retail;
`file:line` is the function's definition line (or the call line) at `01dcd5f`.

Three PRs (steps 1, 2, 3), each usable alone; step 4 is an outline.

---------------------------------------------------------------------------------------------------------

## 0. Decisions this plan makes (the port can overrule them; questions in section 6)

**D1. A per-model "car key" instead of the bare load group.** `SURVIVAL.md` 2.5 indexes the per-type tables
by load group. This plan indexes them by a *key* with the same layout as the retail value (`key >> 4` is the
table index, `key + 6` the turbo id), assigned when a car DMD's root registers:

- the **first** car of each type in the load order (humans first, then opponents, = group order) gets
  `key = type` (0x10..0xF0);
- every **further** car of a type already loaded gets an overflow key `(16 + d) << 4`, d = 0, 1, ...
  (index 16..31);
- all roots of one DMD (Minion's two 224 roots) share their group's key.

Why: with no duplicate in the match every key equals the type, so every table index, every lookup and every
value the game stores (`stats.crush_owner` in the car record, `spin_target` in the slam states) is
**byte-for-byte what retail stores**. The port's frame-by-frame equality test (≤ 11 cars, no duplicates)
then holds by construction, also for fields inside the car records. With pure group indexing,
`crush_owner`/`spin_target` would hold group numbers and differ from the define-off build. The key is still
"per car" (one per loaded DMD), which is what 2.5 asks; the tables have 32 entries (index 0 unused, 1..15
first cars, 16..31 duplicates; a 16-car match has at most 15 duplicates).

**D2. Minion's second root keeps its slot.** 2.5 step 1 suggests skipping it. Retail registers it (slot k+1,
kind 2, drawn off), and every later car's slot number (car record +0x004 / +0x006) depends on it; skipping it
would change those record bytes whenever a Minion is loaded before other cars. Kept, so slots per match =
cars + Minions (≤ 32 for 16 cars).

**D3. Run-time switches, all 0 = retail.** Anything that changes game logic or `rand()` order in a match
that retail could also play is behind a port word (`portCarOptions`, step 1), not only behind the define:
repeats in the pickers (bit 0), car DMDs above the image (bit 1, step 2), farthest-free start picker (bit 2,
step 2). So "define on, switches 0, ≤ 11 distinct cars" = retail logic, `rand()` calls included.

**D4. Step 1 adds new port tables and never moves the retail ones.** The 13 per-type tables of 2.2 stay where
they are (and keep their retail users in the define-off build); under the define every reader and writer uses
`portCarGroups` (new, `.port_bss`, group `one`). No retail symbol is redirected, so step 1 needs no tool
change and no hazard regeneration, and a static check proves that no changed function still touches a
retail per-type table (verification 1.9).

---------------------------------------------------------------------------------------------------------

## 1. Step 1: duplicates (`TM2_PORT_CAR_GROUPS`)

### 1.1 Defines and combinations

| define | needs | effect |
|---|---|---|
| `TM2_PORT_CAR_GROUPS` | `TM2_PORT_VARIANT` (`#error` otherwise, in `car_groups.h`) | keys (D1), per-key tables, binders by group, pickers may allow repeats (switch) |

Works with or without `TM2_PORT_CARS` (step 2) and `TM2_PORT_FAR_LOD` (no shared function: FAR_LOD changes
`assetInstallImage`, `renderScene`, `worldLoadStartObjects`; step 1 touches none of them).

### 1.2 New header `include/semantic/car_groups.h`

Plain C types (included by units with their own `u8`/`s32` typedefs, like `settings.h`):

```c
#ifndef TM2_SEMANTIC_CAR_GROUPS_H
#define TM2_SEMANTIC_CAR_GROUPS_H
#define TM2_CAR_KEYS 32          /* key index 0 unused, 1..15 first car of each type, 16..31 duplicates */
#define TM2_CAR_SLOT_CAP 32      /* >= every TM2_MAX_CAR_SLOTS (step 2) */
struct TM2PortCarGroups {
    int  slot_group[TM2_CAR_SLOT_CAP];    /* load group n of each car slot (vramTexSlotGroup at registration) */
    int  slot_key[TM2_CAR_SLOT_CAP];      /* key of each slot */
    int  group_key[TM2_CAR_SLOT_CAP];     /* key of each load group (0 = none yet); groups 1..16 used */
    int  key_type[TM2_CAR_KEYS];          /* car type of each key index (for dispatch on duplicates) */
    int  dup_count;                       /* overflow keys handed out in this load */
    int  pad;
    void *body[TM2_CAR_KEYS];             /* = carBodyObjects (801873D0) per key */
    void *wreck[TM2_CAR_KEYS];            /* = carWreckModels (8018740C) */
    void *damage_stage[TM2_CAR_KEYS];     /* = carDamageStageObjects (8018F4E4) */
    void *tire[TM2_CAR_KEYS][3];          /* = carTireSwitchObjects (8018F524) */
    int  tire_accum[TM2_CAR_KEYS];        /* = carPartsTireTurnAccum (80187344), never cleared (as retail) */
    void *turbo[TM2_CAR_KEYS];            /* = carTurboObjects (8018F5E4) */
    void *parts[TM2_CAR_KEYS][6];         /* = carPartsHammerhead[4]/Minion[6]/MrSlam[4]/DarkTooth[4]/carAxelObjects[2] */
    int  part_angle[TM2_CAR_KEYS];        /* = carPartsSpinAngle* (80180B78..88), never cleared (as retail) */
    unsigned char freeze[TM2_CAR_KEYS][264]; /* = carFreezeObjects (8018F950), 264-byte TM2Choice records */
};
typedef char tm2_port_car_groups_size[(sizeof(struct TM2PortCarGroups) == PUT_EXACT_SIZE) ? 1 : -1];
#ifdef TM2_PORT_CAR_GROUPS
extern struct TM2PortCarGroups portCarGroups;
extern int portCarOptions;               /* bit 0: pickers allow repeats; bits 1-2: step 2 */
#pragma tm2_absolute portCarGroups
#pragma tm2_absolute portCarOptions
int  portCarKey(int slot, int type);         /* slot's key; `type` if the slot has none (fallback = retail) */
int  portCarKeyType(int key);                /* key < 0x100 ? key : key_type[key >> 4] */
int  portCarLoadKey(void);                   /* key of group vramTexSlotGroup (0 if its root has not registered) */
int  portCarKeyOfObject(void *object);       /* key of the slot holding this world object, else 0 */
int  portCarSlotOfGroup(int group);          /* first slot of the group, -1 if none */
void portCarRegisterRoot(int slot, int type);/* carRegisterModel: group/key bookkeeping */
void portCarGroupsReset(void);               /* carTablesClear: per-load reset (1.5) */
void portCarTurboSetStep(int key, int value);/* animSetStep(turbo[key >> 4], value) */
void portCarPartsSpin(int key, int amount);  /* the carPartsSpin* body for this key's part set and angle */
#define TM2_CAR_KEY(slot, type) portCarKey((slot), (type))
#else
#define TM2_CAR_KEY(slot, type) (type)
#endif
#endif
```

- The pragma is required: under extern-gp a 4-byte `extern int` is $gp-addressed, and a $gp access to the
  port base cannot link (`R_MIPS_GPREL16` overflow). Every port name a game unit reads must have it.
- The `#else` branch expands to the retail expression, so every changed call site is the retail C without the
  define.
- Fill `PUT_EXACT_SIZE` (about 0x2A90; compute it, it must equal the tsv `entry_bytes`).

### 1.3 Storage (`config/US/port_variant_arrays.tsv`, `src/semantic/port_variant_arrays.c`)

Rows (appended after `portFarLodTable`; defined in **every** variant, as FAR_LOD's storage, so the variant's
layout does not depend on the option):

```
portCarGroups	-	<sizeof, hex>	0	one	bss	new	-	per-car registries of duplicate cars: keys per slot/group, body, wreck, damage-stage, tire, turbo, part, freeze objects per key (request 32 step 1; used only with --port-define TM2_PORT_CAR_GROUPS)
portCarOptions	-	4	0	one	bss	new	-	port switches for request 32: bit 0 pickers allow repeated cars, bit 1 car DMDs above the image (step 2), bit 2 farthest-free start picker (step 2); 0 = retail
```

In `port_variant_arrays.c` (outside `#ifdef TM2_PORT_CAR_GROUPS`): `#include "semantic/car_groups.h"`,
`struct TM2PortCarGroups portCarGroups PORT_BSS = {0};`, `int portCarOptions PORT_BSS = 0;`.
Zero is a correct initial state (retail's tables are BSS, the spin angles are sdata 0: checked, words at
80180B78..80180B88 are 0 in the EXE).

### 1.4 Port functions (`.port_text`, in `port_variant_arrays.c` under `#ifdef TM2_PORT_CAR_GROUPS`)

Same recipe as FAR_LOD's functions (`PORT_TEXT`, GCC 2.7.2, absolute addressing). Externs they use:
`vramTexSlotGroup` (sdata 80181050), `carWorldObjectCount` (80180FE8), `carSlotObjects`, `animSetStep`,
`RotMatrixX`, `mathCopyWords`.

- `portCarGroupsReset()`: zero `slot_group`, `slot_key`, `group_key`, `key_type`, `dup_count`; zero the
  **pointer** tables `body`, `wreck`, `damage_stage`, `tire`, `turbo`, `parts` (retail clears their
  counterparts each load: `carPartsReset` 80114380, `objRegistriesReset` 80124A00); zero `freeze[16..31]`
  only. Do **not** touch `tire_accum`, `part_angle`, `freeze[1..15]`: retail never clears them, and keeping
  them gives the same persistence across matches as retail for first cars. `freeze[16..31]` are zeroed
  because `carFreezeResetAll` calls `animSetStep` on every record; a stale duplicate record from an older
  match would point into a DMD area since reused, which retail never does (retail's stale records are type
  records only, kept equal here).
- `portCarRegisterRoot(slot, type)`: `g = vramTexSlotGroup`; `slot_group[slot] = g`; if `group_key[g] == 0`:
  if some other group already has `group_key[h] == type` (an earlier car of this type), then
  `key = (16 + dup_count++) << 4` (clamp: if `dup_count == 16`, use `type`, i.e. retail sharing), else
  `key = type`; `group_key[g] = key`; `key_type[key >> 4] = type`. `slot_key[slot] = group_key[g]`.
- `portCarLoadKey()`: `group_key[vramTexSlotGroup]` (0 if the root has not registered yet).
- `portCarKey(slot, type)`: `(u32)slot < 32 && slot_key[slot] ? slot_key[slot] : type`.
- `portCarKeyOfObject(o)`: first `i < carWorldObjectCount` with `carSlotObjects[i] == o` → `slot_key[i]`; 0.
- `portCarSlotOfGroup(g)`: first `i < carWorldObjectCount` with `slot_group[i] == g`, else -1.
- `portCarTurboSetStep(key, v)`: `animSetStep(turbo[key >> 4], v)` (with `key >> 4 < 32`). Equivalent to
  retail's `objRegistrySetStep(type + 6, 0, v)`: the ids `type + 6` match no case of that switch.
- `portCarPartsSpin(key, a)`: the `ROTATE_OBJECTS` body of `car_parts_spin.c:30` with
  `part_angle[key >> 4]` and `parts[key >> 4][0..5]` (null entries skipped, as retail's loop skips them).
  Check first that `carPartsSpinAxel` (`world_fall_limit.c`, 80114898..) is the same pattern as the four
  in `car_parts_spin.c` (`angle -= a / 7`, identity, `RotMatrixX(-angle)`, copy 5 words to `obj + 4`); if it
  differs, give Axel its own branch.

### 1.5 Game functions: registration and reset

All changes are `#ifdef TM2_PORT_CAR_GROUPS ... #else <retail line(s)> #endif`.

| VA | function | file:line | change |
|---|---|---|---|
| 80119848 | `carTablesClear` | `ua_init.c:124` | add `portCarGroupsReset();` (runs first in `arenaResetSystems` 80128CE4, before `carPartsReset` / `objRegistriesReset` / the DMD walks; also from `uaInit`) |
| 8011961C | `carRegisterModel` | `ai_route_helpers.c:333` | inside the `if`, before `carWorldObjectCount++`: `portCarRegisterRoot(carWorldObjectCount, id);` (`carSlotOwners` keeps the type: `carBindHumanSlot`/`carBindAiSlot` start the car with it) |
| 80114EEC | `carBodyObjectRegister` | `misc_one_liners.c:421` | `k = portCarLoadKey(); if (!k) k = id; if (*object == 5 && (k >> 4) < 32) portCarGroups.body[k >> 4] = object;` |
| 801152A8 | `carWreckModelRegister` | `misc_one_liners.c:429` | same, `type 1`, `wreck[]` |
| 8012478C | `carRegisterTireSwitch` | `car_switch_registry.c:53` | group = `(portCarLoadKey() ? portCarLoadKey() : *(u16 *)(object + 2)) >> 4`, bound 32, `tire[group][slot]` |
| 80123F78 | `objRegistryAdd` | `object_registry.c:17` | the `((id - 6) & 15) == 0` line: `portCarGroups.turbo[(portCarLoadKey() ? portCarLoadKey() : id) >> 4] = object;` (keep the `+7` line retail: `D_8018F624` is never written, SURVIVAL 2.2) |
| 80124384 | `objRegistrySetStep` | `object_registry.c:56` | the `((id - 6) & 15) == 0` line reads `portCarGroups.turbo[id >> 4]` (fallback for any caller still passing `type + 6`; `id < 400` keeps `>> 4` ≤ 24) |
| 80124C40 | `carFreezeBuild` | `car_freeze_build.c:70` | `index = (portCarLoadKey() ? portCarLoadKey() : id) >> 4`, bound 32, records in `portCarGroups.freeze` (cast to `TM2Choice[32]`; the `BUILD_SIDE` macro uses `carFreezeObjects[index]`: make it a macro parameter or `#define` the table name) |
| 801149B8, 80114A08, 80114A58, 80114AA8, 80114AF8 | `carPartRegisterHammerhead/Minion/MrSlam/DarkTooth/Axel` | `arena_load_register.c:538/550/562/574/586` | keep each bound (4, 6, 4, 4, 2); store into `portCarGroups.parts[portCarLoadKey() >> 4][k - 1]` (fallback key = the type constant 0x20/0xE0/0xC0/0xF0/0xB0) |
| 8011995C | `arenaSetup` | `arena_setup.c:243` (call at :250) | `carChoiceRegisterGroup(carSlotObjects[i], TM2_CAR_KEY(i, carSlotOwners[i]));` (both Minion slots → same key, same "last wins" order as retail) |
| 801246A8 | `carChoiceRegisterGroup` | `car_switch_registry.c:14` | `index < 32`, `portCarGroups.damage_stage[index]` |

`worldLoadObject` (800D1DB0, big switch with a jump table) is **not** changed: the registration functions
take the key from `vramTexSlotGroup` themselves.

Pre-condition to verify statically (1.9): `dbscanHierarchy` calls back a car DMD's root before its children
(retail already relies on it: `worldLoadParentObject` is set at the root case and read by the +7 case). The
fallbacks (`portCarLoadKey() == 0` → the retail index) keep retail behaviour if it ever does not.

### 1.6 Game functions: run-time readers

| VA | function | file:line | change |
|---|---|---|---|
| 80114E88 / 80114E10 | `carBodyObjectHide` / `carBodyObjectShow` | `ai_route_helpers.c:114/125` | table `portCarGroups.body`, `i = id >> 4`, add `if (i >= 32) return;` |
| 80114F24 | `carBodyPoseUpdate` | `car_body_pose.c:26` | `slot = TM2_CAR_KEY(rec->slot, rec->type) >> 4` (human +0x004, AI +0x006), table `portCarGroups.body`; `id` stays the type (height +3 test) |
| 801152E0 | `carWreckModelGet` | `small_game_helpers.cpp:20` (C++: `extern "C"` declarations in the header) | `portCarGroups.wreck[id >> 4]` |
| 80124718 | `carSetDamageStage` | `car_switch_registry.c:33` | `index < 32`, `portCarGroups.damage_stage` |
| 80124800 | `uaswToggleCarTireState` | `car_switch_registry.c:69` | `group < 32`, `portCarGroups.tire` |
| 80114B70 | `carPartsAnimate` | `car_parts_spin.c:69` | `t = portCarKeyType(kind)`; if `t` is 0x20/0xB0/0xC0/0xE0/0xF0 → `portCarPartsSpin(kind, amount)`; else `k = kind >> 4; if (k < 32)` accumulate in `portCarGroups.tire_accum[k]`, toggle `uaswToggleCarTireState(k, amount)` as retail |
| 80124C00 | `carFreezeSetState` | `car_freeze_dispatch.c:27` | `index < 32`, `portCarGroups.freeze` |
| 80124B80 | `carFreezeResetAll` | `car_freeze_dispatch.c:11` | loop 32 over `portCarGroups.freeze[i]` and `portCarGroups.turbo[i]` |
| 8011D15C | `carObjectFromOwner` | `car_records.c:361` | compare `portCarGroups.slot_key[i]` instead of `carSlotOwners[i]` (keep retail's "return `carSlotObjects[i]` past the end"). This one change re-keys `carSpinOutByObject` 800F4A58, `carSpinOutById` 800F5D20, `carDamageByObject` 800F4B74, `carCrushContactUpdate` 800F39D0, `weaponSlamCarryTarget` 800F59A4 and the Dark Tooth test in `weaponProjectileUpdate` (800DB35C, `carObjectFromOwner(240)`: the first Dark Tooth, as today) |

### 1.7 Game functions: callers that pass a type as a car identity

In every row the type stays where it is compared with a constant; only the value passed as "which car" becomes
the key. Local struct views without a `slot` field (`car_damage_human.c` `HumanCar2068`, `world_collapse_warps.c`
`AICar`, check `car_input.c` `Car`, `ai_steps.c` `TM2AI`) get one (human +0x004, AI +0x006, `s16`; the size
asserts must still hold).

| VA | function | file:line (calls) | change |
|---|---|---|---|
| 800E5628 | `carApplyDamage` | `car_damage.c:94` (type at :152/:168; calls :226 hide, :278 wreck, :286 damage stage) | new local `car_key = TM2_CAR_KEY(<rec>->slot, car_type)` in both branches; the three calls use it; the `0xF0` tests keep `car_type` |
| 800ED128 | `carGroundEffects` | `car_wheels.c:296` (type at :313/:323; :450 hide, :474 show) | local key, the two calls |
| 80105A7C | `weaponFireSpecial` | `weapon_basic.c:113` (:258 `carBodyObjectHide(0xa0)`) | `carBodyObjectHide(TM2_CAR_KEY(otherid, 0xa0))` (`otherid` = the firer's slot, :135/:141). Its `switch (vehicle)` jump table changes with it (allowed: words into the changed function) |
| 8010DE54 | `weaponInvisibilityUpdate` | `ai_link_node_pick.c:267` (:285, :291) | `carBodyObjectHide/Show(TM2_CAR_KEY(st->slot, st->type))`; `clutAnimStartFadeOut/FadeIn(st->type + 8, ..)` stay by type (the CLUT is shared in VRAM, 6.Q6) |
| 8010E040 | `weaponTornadoUpdate` | `weapon_tornado.c:102` (:167, :202 `carSpinOutById(x->type, ..)`; :235/:237 `carBodyObjectHide/Show(160)`) | :167/:202 → `TM2_CAR_KEY(x->slot, x->type)`; :235/:237 → `k = portCarKeyOfObject(weaponTornadoState.object); if (!k) k = 160;` (`weaponTornadoState.object` = the firer's object, set at `weapon_basic.c:254`) |
| 8010F520 | `weaponHomingLeapUpdate` | `weapon_leap_homing_update.c:47` (:82, :91, :96 `objRegistrySetStep(0x66, 0, v)`) | `portCarTurboSetStep(TM2_CAR_KEY(H.source, 0x60), v)`; `H.source` (+0x20) = `homingLeapState.owner_id` = the leaping car's slot (`weapon_leap_homing_init.cpp:72/82`): confirm the two struct views agree on +0x20 |
| 8011B89C | `humanCarDamageUpdate` | `car_damage_human.c:280` (:287, :288, :295, :302, :308) | key of `humanCars[i]` for the 5 calls |
| 8011BBD4 | `aiCarDamageUpdate` | `world_collapse_warps.c:239` (:246, :247, :252, :257, :262) | key of `aiCars[i]` |
| 800DC88C | `humanCarTick` | `car_tick.c:34` (:61 freeze, :96 parts) | key of `car` |
| 800E7EDC | `aiCarDrive` | `ai_driver.c:29` (:58 freeze, :95 parts) | key of `car` |
| 800DE5F8 | `humanCarReadInput` | `car_input.c:92` (:170, :174 `objRegistrySetStep(car->type + 6, 0, v)`) | `portCarTurboSetStep(TM2_CAR_KEY(car->slot, car->type), v)` under the define (`#else`: retail line). Do not pass `key + 6` to `objRegistrySetStep`: overflow keys ≥ 25 << 4 would hit its `id < 400` test and its 500..510 cases |
| 800EC180 | `aiInputFinalize` | `ai_steps.c:992` (:1021, :1025) | same |
| 800D6FD8 | `projectileHitTest` | `weapon_slam_swing.c:44` (:65, :99 `carFreezeSetState(x->type, 1)`) | key of the hit car |
| 800F4058 | `weaponSlamSwingUpdate` | `weapon_slam_swing.c:175` (:251, :254, :288 `carSpinOutByObject(192, ..)`) | `TM2_CAR_KEY(id, 192)` (`id` = the slammer's slot, :203) |
| 800F5498 | `weaponSlamFindTarget` | `screen_card_draw.c:142` (:228, :267 `rec->spin_target = x->type`) | `= TM2_CAR_KEY(x->slot, x->type)` (keys ≤ 0x1F0 fit the `s16`) |
| 800E6D14 | `carCrashStart` | `car_crash.c:30` (:111 `st->crush_owner = okind`) | `= TM2_CAR_KEY(oid2, okind)` (`oid2` = the other car's slot, :73/:80); the `kind == 32` / `!= 240` tests stay by type |
| 800E75F4 | `carWreckUpdate` | `car_crash.c:248` (:396 `carObjectFromOwner(kind)`) | `TM2_CAR_KEY(AI(car)->slot, kind)` (or `aiCarObjects[index]`, SURVIVAL 2.3; the key form keeps the code shape) |
| 8011BE10 | `carDestroyed` | `car_destroyed.c:299` (:343 `carSpinOutById(mode, ..)`) | key of the destroyed car; the `mode == 0xC0/0x60/..` tests stay by type |

Not changed (they only pass a stored key through): `weaponSlamGrabUpdate` (`weapon_arcs_draw.c`),
`weaponSlamRelease` 800F4C34 (`world_object_register.c:137`), `carSpinOut*`, `carDamageByObject`,
`carCrushContactUpdate`, `weaponSlamCarryTarget`. `weapon_slam_swing_draft.c` is not a built unit (not in
`BUILD_MANIFEST.json`): leave it.

### 1.8 Binders and pickers

| VA | function | file:line | change |
|---|---|---|---|
| 8012BD94 | `sessionBindCars` | `screen_card_layout.c:181` | humans: `carBindHumanByType(i, type, i + 1)`; opponents: `carBindAiByType(i, type, settings->humans + i + 1)` (the group numbers of `sessionLoadArenaAndCars`) |
| 8011A29C | `carBindHumanByType` | `world_object_register.c:422` | new 3rd parameter `group` (under the define; prototype in the shared header); `i = portCarSlotOfGroup(group)`, fallback: retail type search; then the retail body (`humanCarTypes[view] = id`, dead read, `carBindHumanSlot(view, i, id)`) |
| 8011A54C | `carBindAiByType` | `ai_route_helpers.c:390` | same with `group`; the retail `carSlotKinds[i] != 2` / `aiCarTotal++` logic unchanged: every newly bound AI slot is kind 2, so `aiCarTotal++` happens once per bound AI (SURVIVAL 2.4); a rebind in later rounds (modes 3/4) finds kind 1 and does not count again, as retail |
| 8012CC10 | `carIsSelectable` | `car_is_selectable.c:12` | `if (portCarOptions & 1)` skip the two "already taken" loops (unlock tests stay) |

`tournamentPickOpponents` (8012CD08) needs no change: with bit 0 set its `rand() % 13` loop accepts repeats
(and can no longer hang past 13 - humans). `screenCarSelect` / `screenAiSelect` reach `carIsSelectable`.

No change to `carFindSlotByOwner` (8011D8F4): its only caller is `carBindAiByType`'s fallback.

### 1.9 What stays retail-equivalent (define on, `portCarOptions` = 0, no duplicate type)

- Every key equals the type → the same table indices, the same objects, the same `crush_owner` /
  `spin_target` values, the same binder slots, `aiCarTotal` the same. No new `rand()` call anywhere.
- Persistent state (spin angles, tire accumulators, freeze records 1..15) persists exactly as retail's.
- Differences that remain: the tables' addresses (port storage), and the pointer that `carFreezeBuild` writes
  into the car model (`output[8] = &record`, inside the car DMD copy) now points into `.port_bss`. Car records
  are unaffected.
- With a duplicate type, retail is not a reference (retail binds both to one slot).

### 1.10 Hazards (step 1)

- **$gp:** only new names; all under `#pragma tm2_absolute` (1.2). No retail global moves. `vramTexSlotGroup`,
  `carWorldObjectCount` stay sdata (read through $gp by the game units, absolutely by the port unit).
- **Jump tables:** `weaponFireSpecial` (switch on the type) and any other changed function with a switch get
  a changed table; `build_source_only` accepts it when all words point into the changed function. Check
  `changed_jump_tables` in `out/TM2_SOURCE_PORT.json`. `worldLoadObject` and `tournamentOpponentCount` are not
  touched in step 1.
- **Hand asm / relocations:** none of the changed functions is hand asm. No retail data relocation targets
  these tables (`SIXTEEN_CARS.md` 2.2). `move_hazards.tsv` does not cover them and is not needed (D4).
- **Shared specials with duplicates (known limits, not fixed):** Twister's tornado, Shadow's strike,
  Grasshopper's leap state, Mr. Slam's arm and claw (`slamArmRegister`/`slamClawRegister`: the last loaded
  Mr. Slam's objects and one angle), the Dark Tooth head (249) and Shadow's 2325 object (last install wins),
  the invisibility CLUT fade (one CLUT per type in VRAM: a duplicate's fade also fades the other), the engine
  sound id per type (6.Q4). SURVIVAL 2.3 lists them.
- **Dark Tooth not loaded last** overwrites his texture slots 20..30 (SURVIVAL 4.2): unchanged by step 1.
- **Key exhaustion:** > 16 overflow keys cannot happen with ≤ 16 cars; the clamp in `portCarRegisterRoot`
  falls back to retail sharing.

### 1.11 Verification (step 1)

1. `py -3.14 build.py --source-only --sdk-from-lib --relocatable --port-variant` (no port define): hybrid,
   source-only, E0 hashes unchanged (compare with `LAST_BUILD.json`, `LAST_SOURCE_BUILD.json`,
   `out/TM2_SOURCE_E0.json` before the change); `tools/data_audit.py` 0 mismatches; the variant's
   `changed_functions` the same 25 (+ FAR_LOD's when given); only `.port_bss` grows.
2. Same with `--port-define TM2_PORT_CAR_GROUPS`: proof vs E0 passes; placement check passes;
   `port_changed_functions` equals exactly the list of 1.5-1.8 (write the expected VA list into the PR and
   diff it against the json with a small script); `changed_jump_tables` ⊆ {`weaponFireSpecial`'s, and any
   other table of a listed function}.
3. Combinations: `TM2_PORT_CAR_GROUPS` + `TM2_PORT_FAR_LOD`; + `TM2_PORT_HOOKS` with the port's overlay if
   available.
4. **No retail per-type table in changed code:** in the define link, list every relocation whose symbol is one
   of `carBodyObjects carWreckModels carDamageStageObjects carTireSwitchObjects carPartsTireTurnAccum
   carTurboObjects D_8018F624 carFreezeObjects carPartsHammerhead carPartsMrSlam carPartsMinion
   carPartsDarkTooth carAxelObjects carPartsSpinAngle*`; the referencing functions must be ⊆ {`carPartsReset`,
   `objRegistriesReset`, `carPartsSpinHammerhead/Minion/MrSlam/DarkTooth/Axel` (dead under the define),
   `objRegistryAdd`/`objRegistrySetStep` (+7 path only)}.
5. **Root-first walk:** for the 15 car DMDs on the disc (`cdimage/`), walk each with the rules of
   `dbscanHierarchy` (the FAR_LOD tool `tools/dmd_lod_dump.py` has a walker) and check that the first callback
   id is the root type and that every id 17..255 comes after it.
6. **Key allocation, host-side:** a 20-line Python model of `portCarRegisterRoot` over load orders (all-distinct
   11 cars: keys == types; 16 Minions: keys 0xE0, 0x100..0x1E0; 4 humans + 12 mixed) → all indices < 32, no
   key ≥ 0x200.
7. Port side: frame-by-frame equality vs the define-off variant, ≤ 11 cars, `portCarOptions = 0`; then its
   duplicate scenarios.

Size: about **45 game functions** in ~30 units, **9 port functions**, 1 header, 2 tsv rows. No tool change.

---------------------------------------------------------------------------------------------------------

## 2. Step 2: more cars (`TM2_PORT_CARS=16`)

**Done** (2026-10-12), with the changes listed at the top of this file; as built: `CAR_GROUPS.md` 7.

### 2.1 Defines and combinations

| define | needs | effect |
|---|---|---|
| `TM2_PORT_CARS=N` (11 ≤ N ≤ 16) | `TM2_PORT_VARIANT` | `TM2_MAX_AI_CARS = N - TM2_MAX_HUMANS` (12 with 4 humans), `TM2_MAX_CAR_SLOTS = 2 N` (cars + Minions' second roots, D2; 32 for 16); constants, settings, DMD area, start picker |

- **Without step 1** it builds and runs, but every car must be of a different type (15 types): at most 15 cars
  (+ Minion's extra slot); a 16th car would duplicate a type and bind to the first one's slot (retail
  behaviour). Document; recommend the port pairs it with `TM2_PORT_CAR_GROUPS`.
- `players.h`: `#if TM2_PORT_CARS - TM2_MAX_HUMANS < 9 || TM2_PORT_CARS > 16 → #error`; and a static assert
  `TM2_MAX_HUMANS < 8` (human i gets object id i + 1; id 8 is "not a car" in `weaponApplyHit` 800DC778 and
  `carContactResolve`, SIXTEEN 1.5: nothing to do while humans ≤ 7; AI ids 50..61 are fine).

### 2.2 Limits (`include/semantic/players.h`)

```c
#define TM2_RETAIL_AI_CARS 9
#define TM2_RETAIL_CAR_SLOTS 15
#ifdef TM2_PORT_VARIANT
# if defined(TM2_PORT_CARS)
#  ifndef TM2_MAX_AI_CARS
#   define TM2_MAX_AI_CARS (TM2_PORT_CARS - TM2_MAX_HUMANS)
#  endif
#  ifndef TM2_MAX_CAR_SLOTS
#   define TM2_MAX_CAR_SLOTS (2 * TM2_PORT_CARS)
#  endif
# endif
#endif
#ifndef TM2_MAX_AI_CARS
# define TM2_MAX_AI_CARS TM2_RETAIL_AI_CARS
#endif
#ifndef TM2_MAX_CAR_SLOTS
# define TM2_MAX_CAR_SLOTS TM2_RETAIL_CAR_SLOTS
#endif
```

and in the `#ifdef TM2_PORT_VARIANT` pragma block: `#pragma tm2_absolute` for `aiCars aiCarObjects
aiCarObjectSlots aiCarTargets carSlotKinds carSlotIndices carSlotOwners carSlotObjects carPairVectors
carPairHeadingDeltas carPairDirections carPairHeadings carPairDistances sessionPortOpponentCars
portCarsPerArena portCarDmdArea portCarOptions`. (None of the car arrays is $gp-addressed in retail: the
census below found only hi16/lo16; the pragmas make sure no sized `extern` turns into one.)

### 2.3 Storage: moved arrays (`port_variant_arrays.tsv`)

The arrays move in **every** variant, sized by the limits (retail sizes without `TM2_PORT_CARS`), as the human
arrays of request 25 did. Rows:

```
aiCars	80188B5C	0x854	9	ai	bss	linear	bss	AI car records (TM2AIRecord, 0x854 each)
aiCarObjects	80188B38	4	9	ai	bss	linear	bss	world object of each AI car (car_object_lookup.c defines it in retail)
aiCarObjectSlots	8018D650	4	9	ai	bss	linear	bss	object slot of each AI car
aiCarTargets	801879FC	4	9	ai	bss	linear	bss	AI target (AI index, -1 = nearest human)
carSlotKinds	80187A20	4	15	slots	bss	linear	bss	0 human, 1 AI, 2 loaded unbound
carSlotIndices	80187A5C	4	15	slots	bss	linear	bss	index in its record array, -1 free
carSlotOwners	80187A98	4	15	slots	bss	linear	bss	car type of each slot
carSlotObjects	80187AD4	4	15	slots	bss	linear	bss	world object of each slot
carPairVectors	8018D674	0xC	225	slotpairs	bss	relayout	bss	[slot][slot][3]: the row stride changes, only the start maps
carPairHeadingDeltas	8018E100	4	225	slotpairs	bss	relayout	bss	[slot][slot]
carPairDirections	8018E484	0xC	225	slotpairs	bss	relayout	bss	[slot][slot][3]
carPairHeadings	8018EF10	2	225	slotpairs	bss	relayout	bss	[slot][slot] s16 (retail 450 bytes + 2 pad)
carPairDistances	8018F0D4	4	225	slotpairs	bss	relayout	bss	[slot][slot]
sessionPortOpponentCars	-	1	0	ai-9	bss	new	-	car-select index of opponents 10..N (slots >= 9; settings.h)
portCarsPerArena	-	4	0	one	bss	new	-	cars per arena, humans included, the port sets from portConfig; 0 = retail table (tournamentOpponentCount)
portCarDmdArea	-	0x100800	0	one	bss	new	-	car DMD load area (16 x 64 KB + one sector of CD overrun); used when portCarOptions bit 1 is set
```

Order in `port_variant_arrays.c`: put `aiCars` **first** among these (its address is then the same with and
without `TM2_PORT_CARS`, which keeps pointers into AI records equal between the two builds the port
compares), pair tables and `portCarDmdArea` last.

`car_object_lookup.c:12`: `aiCarObjects` becomes `extern u32 aiCarObjects[TM2_MAX_AI_CARS];` under
`TM2_PORT_VARIANT` (as `humanCarObjects` at :7-10); its `.tm2_80188B38` section is then a "lost section
holding only moved arrays" (allowed).

### 2.4 Tool changes (needed: new groups)

| file | change |
|---|---|
| `tools/gen_link.py` `port_entries` (:102) | `'ai-9'` → `limits['ai'] - 9`; `'slotpairs'` → `limits['slots'] ** 2` |
| `tools/build_source_only.py` `port_defines` (:333) | map `'ai': 'TM2_MAX_AI_CARS'`, `'slots': 'TM2_MAX_CAR_SLOTS'` |
| `tools/build_source_only.py` `port_limits` (:378) | read `TM2_MAX_AI_CARS`, `TM2_MAX_CAR_SLOTS` back; return `ai`, `slots`. It must see the port's `--port-define`s (it gets `defines` from `port_defines(.., extra)`: check `TM2_PORT_CARS=16` is in that list) |
| `tools/build_source_only.py` (:766-768) | `--port-limits N` also forces `ai=9, slots=15` (the placement check: "only the arrays move") |
| `build.py` (:413-442, :495-549) | placement-check wording and the limits it records; the expected $gp set stays the 5 functions of request 25 (none of the car arrays is $gp-reached) |
| `tools/reloc_image.py` | confirm `relayout` (start-only) is generic, not keyed on `hudOrderTables` |
| `tools/export_retail_relocs.py` `ARRAYS` (:47) | add the 13 car arrays (group `cars`), regenerate `config/US/retail_relocs.tsv` and `move_hazards.tsv` (both "do not edit", generated from an E0 link), and read the new hazard rows |

Census already run for this plan (all relocations into the 13 arrays in `retail_relocs.tsv`): **no
neighbour or outside-symbol relocation** (every reloc's symbol is the array that holds its target), no $gp
access. Addends: `aiCars` 67 distinct (all inside 9 records: linear is fine), `aiCarTargets` 0..0xC
(`demoRespawnAiCars`), `carPairVectors` 0/4/8 and `carPairDirections` 0/4 (component offsets of pair [0][0],
only in `carPairsUpdate`/`carPairVector`/`carPairDirection`, which step 2 changes anyway). Users of the slot
and pair tables: `aiCarObject aiCarObjectSlot aiGetTargetCar arenaSetup carBindAiByType carBindAiSlot
carBindHumanByType carBindHumanSlot carDestroyed carFindById carFindByObject carFindSlotByOwner carFromSlot
carObjectFromOwner carObjectId carPair* carRegisterModel carRestoreNodeState carSaveNodeState carTablesClear
carsPhysicsUpdate demoRespawnAiCars uaInit worldUpdate`. "End address" hits were only the next array's own
symbol (adjacent arrays), not one-past-end forms.

### 2.5 Game functions (constants, settings, load, start points)

| VA | function | file:line | change |
|---|---|---|---|
| 8011961C | `carRegisterModel` | `ai_route_helpers.c:333` | `< 15` → `< TM2_MAX_CAR_SLOTS` |
| 80119848 | `carTablesClear` | `ua_init.c:124` | `15` → `TM2_MAX_CAR_SLOTS`; both `9` → `TM2_MAX_AI_CARS`; `extern` sizes at :30/:41-44 |
| 80119698 | `uaInit` | `ua_init.c:65` | `aiCarTargets` loop `9` → `TM2_MAX_AI_CARS` (:120) |
| 8011995C | `arenaSetup` | `arena_setup.c:243` | `aiCarTargets` loop `9` (:263-264) |
| 8011A3A0 | `aiCarsSetActive` | `misc_one_liners.c:189` | `8 * sizeof` → `(TM2_MAX_AI_CARS - 1) * sizeof` |
| 8011D0EC | `aiCarGet` | `car_records.c:347` | clamp `9` → `TM2_MAX_AI_CARS` |
| 8011E7D8 | `carPairsUpdate` | `screen_deathmatch_result.c:33` | declarations :12-16 `[][15]` → `[][TM2_MAX_CAR_SLOTS]` |
| 8011EB18, 8011EB4C | `carPairVector`, `carPairHeadingDelta` | `game_misc_accessors.c:373/379` (decls :197-198) | stride via the declaration |
| 8011EB78, 8011EBAC, 8011EBE0 | `carPairDirection`, `carPairHeading`, `carPairDistance` | `misc_one_liners.c:312/318/324` (decls :210-212) | same |
| 80131EEC | `screenAiSelect` | `screen_ai_select.c:114` (:136 `slot < 9`) | `TM2_MAX_AI_CARS` |
| 8012CBAC, 8012CBC4 | `sessionGetOpponentCar`, `sessionSetOpponentCar` | `game_setting_accessors.c:23/38` | slots ≥ 9 → `sessionPortOpponentCars[slot - 9]` (new `TM2_SETTINGS_OPPONENT_CAR(s, i)` lvalue macro in `settings.h`, like `TM2_SETTINGS_HUMAN_CAR`) |
| 8013783C, 80137854 | `settingsGetOpponentCar`, `settingsSetOpponentCar` | `game_settings_extra.c:24`, `menu_state_accessors.c:24` | same |
| 80137864 | `settingsClearOpponentCars` | `game_settings_extra.c:31` | also set the port table to -1 (the 13-byte retail loop unchanged) |
| 8012BD94 | `sessionBindCars` | `screen_card_layout.c:181` | `TM2_SETTINGS_OPPONENT_CAR(settings, i)` |
| 8012BAFC | `sessionLoadArenaAndCars` | `shell_main.c:265` | (a) opponents through `TM2_SETTINGS_OPPONENT_CAR` (:290, :308); (b) SIXTEEN 3.5: `if (portCarOptions & 2) { base = portCarDmdArea; off = 0; } else { base = 0x80019C40; off = level size; }` after the level load, both car loops use `base + off`. The TMS loads and the level DMD stay at 0x80019C40; `assetInstallImage`'s 0x7FFE63C0 stays |
| 8012AFE8 | `tournamentOpponentCount` | `tournament_tables.c:12` | first line: `if (portCarsPerArena > 0) return portCarsPerArena;` (the callers subtract the humans: `screen_single.c:225/240`, `screen_controller_multiplayer.c:210`, `screen_title_level_select.c:359`, `menu_scroll_text.c:45`; check each before relying on it). Its jump table (`.tm2_800CFC38`) changes with it |
| 8012CD08 | `tournamentPickOpponents` | `tournament_pick_opponents.c:11` | no change with repeats on; with repeats off and `opponents > 13 - humans` it hangs (retail): allow repeats for the slots past 13 - humans |
| 800F636C | `startNodePickRandom` | `ai_start_nodes.c:15` | `if (portCarOptions & 4) return portStartNodePick(count);` (port function, 2.6) |

`extern` declarations with a size (`[9]`, `[15]`, `[][15]`) in `car_physics.c:79/83`, `game_misc_accessors.c:235`,
`misc_one_liners.c:35`, `spawn_tables.c:20/22`, `world_update.c:81/86`, `cars.h:90`, `ua_init.c:30/41-44` →
the limit macros (code unchanged except where a stride or a loop bound comes from them).

Already done by request 25 (the variant): the per-human arrays at 4 (`humanCars`, `humanCarObjects`,
`humanCarObjectSlots`, `humanCarTypes`; `carResetWeapons` has 4 bytes), `uaInit`/`carTablesClear`/`roundStart`
human loops, `worldUpdate`'s radar for every human pair (taunt included). **Left:** the round-end test
`!humanCars[0].f_09C` (`world_update.c:425`): a game rule, the port's (6.Q10).

### 2.6 Port function `portStartNodePick(count)` (SIXTEEN 5.3 A)

In `port_variant_arrays.c` under `#ifdef TM2_PORT_CARS` (`.port_text`):
- untaken entries exist: if none taken yet, retail's first pick (`rand() % count`, first free from there);
  else the untaken entry whose node (`aiRouteNodes[id].x/.z`, 24-byte records at `D_801B1DE8`) has the
  largest minimum distance (|dx| + |dz|, no overflow) to the taken entries' nodes (= the cars placed since
  `startNodesClearUsed`); mark it taken;
- all taken: the entry with the largest minimum distance to the current positions of the live cars
  (humans and AI by count, skipping HP ≤ 0 and parked ones), not marked (as retail).
- Paris keeps its human limit (the callers pass 9).

### 2.7 Retail-equivalent with the define on (≤ 11 distinct cars, `portCarOptions` = 0, `portCarsPerArena` = 0)

Same logic and `rand()` calls. Differences: array addresses (both builds have the arrays in port storage;
`aiCars` at the same address if placed first), the clearing/active loops touch records 9..11 (not compared:
the define-off build has none), `aiCarGet(9..11)` returns those records instead of record 0 (only reachable
by out-of-range callers). Pair-table values for a kind-2 slot (Minion's second root) read `aiCars[-1].pos`,
i.e. whatever precedes `aiCars` in port storage (retail reads into `humanCars[1]`; already different since
request 25; never used).

### 2.8 Hazards (step 2) and limits that more cars hit

- **Opponent cards** (tournament only, SIXTEEN 6.2): `screenCardQuads` holds 9 cards + the template;
  `screenLoadCardImages` (8012A614, `scene_init.c:365`) and the card layout code index by opponent count, so
  10+ opponents in a tournament write past the array and read `sessionSettings` as positions. Must be
  decided before shipping > 9 opponents in tournaments (6.Q9).
- **Radar:** 12 blips per view (`hudRadarPlotCar` 800FC928, `count + 2 >= 25`); 15 other cars drop off.
- **Pools:** 128 coordinate systems (`csAlloc`), 35 sound instances, 16 projectiles, 25 weapon slots, 150
  effect sprites (SIXTEEN 1.6). Not changed here.
- **AI pack logic** in `worldUpdate` (`closer < 3`, `>= 4`): more AIs qualify (tuning, not a bug).
- **Paris** opens the tower links when `aiCarsRemaining < 3`; **Dark Tooth** gates need `aiCars[0]` (SURVIVAL 1.3).
- **Memory:** without bit 1 the car DMDs still share 0x80019C40..0x800CDC54 (SIXTEEN 3.3: 7-14 cars per arena).
  With bit 1, the area is real RAM the port maps above 2 MB (it is in `.port_bss`, zeroed with it). CD DMA
  writes up to 2,047 bytes past the last file (the 0x800 slack).
- **VRAM:** 8 different normal types fit on every arena, 14 on four arenas (SIXTEEN 4.5); step 4.
- **Pointers in the port's comparisons:** with bit 1 the object pointers in car records differ by the area
  delta (as in shifted builds, `RELOCATIONS.md` 4).
- **rand():** none added with the switches at 0; `portStartNodePick` calls `rand()` once per first pick only.

### 2.9 Verification (step 2)

1. Default variant (no define): arrays moved at retail sizes; proof vs E0 passes; `changed_functions` the
   same as before (the moves change no code); placement check passes with the 5 $gp functions only.
2. `--port-define TM2_PORT_CARS=16` (with and without `TM2_PORT_CAR_GROUPS`): proof passes;
   `port_changed_functions` = the list of 2.5 (+ step 1's); `changed_jump_tables` ⊆ {800CFC38,
   step 1's}; the json's `limits` show `ai 12, slots 32`; `port_arrays.tsv` sizes: `aiCars` 12 x 0x854,
   pair tables 32 x 32 entries.
3. Placement check (`--port-limits 2` → humans 2, ai 9, slots 15): un-relocates to E0 except the $gp
   functions and the `port_changed` set.
4. Negative control as in SOURCE_BUILD "Proofs" 3: swap the `port` rows of two slot tables in the placement
   check's `addrmap.tsv`; `unrelocate` must fail.
5. Regenerated `move_hazards.tsv`: read every new `high`/`medium` row for the car arrays; each must be in a
   changed function or explained.
6. Retail hashes unchanged; `tools/data_audit.py` 0.
7. Port: equality vs define-off at ≤ 11 cars with switches 0; its 16-car scenarios with bits 0-2 and
   `portCarsPerArena = 16`.

Size: about **25 game functions** in ~15 units + ~15 `extern` declarations, 1 port function, 16 tsv rows, 5
tool files.

---------------------------------------------------------------------------------------------------------

## 3. Step 3: a car that changes type (`TM2_PORT_CAR_SWAP`)

**Done** (2026-10-12), as built: `CAR_GROUPS.md` 8, following `PER_CAR_PLAN.md` B (this section updated for
per-car state) and the port's answers G.3-G.5 and Q14/Q15. The one game change is `sessionBindCars` (already
step 1's), which binds a swapped record to its new car through `portCarBindGroup`; everything else is port
code. 3.4's first two hazards are fixed: homing projectiles follow the record (G.5) and kill credits by slot are
remapped.

### 3.1 Defines

`TM2_PORT_CAR_SWAP` requires `TM2_PORT_CAR_GROUPS` (`#error` otherwise: the swap re-binds per-model keys;
with type keys two cars of one type, e.g. a human Minion and the Juggernaut's Minion, would collide).
Independent of `TM2_PORT_CARS`. Everything lives in `.port_text` (`port_variant_arrays.c`); no game function
changes, so the define-off build is not affected at all and the define-on build differs only by the new port
code (`port_changed_functions` empty).

### 3.2 Model

A car is a record (human i or AI j) bound to a slot (its loaded DMD copy, its key, its group). The swap
exchanges the slots of two records, or moves a record to a free (kind 2) slot. The Juggernaut: the match
loads one extra opponent, Minion, in group m, parks its AI record right after `sessionBindCars` (SURVIVAL 3.1,
and again after every `sessionBindCars` in modes 3/4, 3.3). Player X becomes the Juggernaut:
`portCarSetGroup(1, X, m, -1)` (X takes the Minion slot, the parked AI record takes X's car and stays
parked). The old Juggernaut Y: `portCarSetGroup(1, Y, Y's own group, -1)` (Y takes its own car back from the
parked record, which takes the Minion back).

### 3.3 `int portCarSetGroup(int human, int index, int group, int hp)`

Returns 1 on success. `hp < 0`: scale (`hp * new_max / old_max`, at least 1 while alive); else set (clamped to
the new max). Call it from the port's per-frame hook before the car updates (where it calls
`demoRespawnAiCars`' slot, SURVIVAL 3.3), not inside a car update.

1. `rec` = `&humanCars[index]` or `&aiCars[index]`; `sa = rec->slot`; `sb = portCarSlotOfGroup(group)`. Fail if
   `sb < 0` or `sb == sa`. `other` = the record bound to `sb` (`carSlotKinds[sb]` 0/1, `carSlotIndices[sb]`), or
   none (kind 2).
2. **Release what the old car holds** (for `rec`, and for `other` if it is not parked):
   - Mr. Slam (type 0xC0) with `weapons.slam->active`: as `carDestroyed` 8011BE10 :336-341
     (`carSpinOutById(spin_target, rot[2], 0)`, `active = 0`, `spin_target = -1`, `spin_pending = 0`);
   - Twister: if `weaponTornadoState.timer > 0 && weaponTornadoState.object == carSlotObjects[sa]` → end it as
     `weaponTornadoUpdate`'s end block (`weapon_tornado.c:226-240`);
   - Shadow: `weaponStrikeState.s.active` and its owner is this car → off as `weapon_basic.c:323-329`;
   - Grasshopper: `homingLeapState.active` and `owner_id == sa` → `active = 0`, `portCarTurboSetStep(key_a, 0)`;
   - any car whose `stats.crush_owner` is `key_a`/`key_b` → `state.crush_contact = 0`; any active slam state
     whose `spin_target` is `key_a`/`key_b` → released as above (keys belong to slots, not records).
3. **Reset the look on the old model:** freeze (`state.frozen = 0`, `stats.freeze_timer = 0`,
   `carFreezeSetState(key_a, 0)`), fire (`state.burning = 0`, `weapons.fire_effect->active = 0` if set), turbo
   flame (`portCarTurboSetStep(key_a, 0)`), invisibility (`weapons.invisible`: `clutAnimFadeIn(type_a + 8, 0)`,
   `carBodyObjectShow(key_a)`, object `visible = 1`, `invisible = 0`, `invis_timer = 0`), engine sound (stop the
   type's engine id: the table of `carSoundUpdate` 8011DB74, `screen_card_draw.c:100-116`).
4. **Re-bind the tables** (raw stores, as `carBindHumanSlot` 8011A3FC / `carBindAiSlot` 8011A5CC do, without
   their `carRestart`/`aiCarStart`):
   - human i ← sb: `humanCarObjects[i]`, `humanCarObjectSlots[i]`, `humanCars[i].slot`, `humanCarTypes[i] = type_b`,
     `carSlotIndices[sb] = i`, `carSlotKinds[sb] = 0`, `carSlotObjects[sb]->id = i + 1`;
   - AI j ← sb: `aiCarObjects[j]`, `aiCarObjectSlots[j]`, `aiCars[j].slot` (+0x006), `carSlotIndices[sb] = j`,
     kind 1, id `j + 50`;
   - `other` ← sa the same way; with no `other`: `carSlotKinds[sa] = 2`, `carSlotIndices[sa] = -1`.
5. **Type change of `rec`** (type_a → type_b = `carSlotOwners[sb]`): keep `motion` (pos, rot, speeds, matrices),
   route state, `aiCarTargets`; `r0 = stats.ride_height`; `rec->type = type_b`; human: `carStatsSetSpeedMult
   (&stats, 100)`, `carStatsLoad(&stats, type_b)` (800FE15C), `max_hp += 20` (as `humanCarPlaceAtStart` 800FD6C4);
   AI: `carStatsSetSpeedMult(&stats, aiDifficultySpeedScale())`, `carStatsLoad`, `carStatsSetTopSpeed(&stats,
   stats.top_speed)` (as `aiCarReset` 80100034); HP per the argument; `motion.pos[2] += stats.ride_height - r0`;
   `carWheelsSetStepHeight(&wheels, type_b)` (800FF638); `carBuildCollisionBox(rec, human)` (800FDFC4);
   weapons: save `ammo[1..9]`, `selected`, call `weaponBlockInit(rec, &weapons, human)` (800FDCAC: per-type
   `recharge_period`, `capacity`, `max_specials`, `ammo[0] = 2`, Mr. Slam's slam state), restore the saved
   pickups (6.Q14); human: `viewCameraOffsetsInit(rec)` (800FF764, camera offsets per type); AI:
   `aiCarSetupProfile(rec)` (800D3C7C, weights and fire chance per type; no `rand()`).
6. **New model on:** damage stage from the HP percentage (> 40 → 0, > 12 → 1, > 0 → 2) with
   `carSetDamageStage(key_b, stage, smoke_arg)`, and the record's stage field (human `dmg_state`, AI `smoke`,
   the names of `car_damage_human.c` / `world_collapse_warps.c`) set to it so the damage update does not fire
   again; `carBodyObjectShow(key_b)` unless dead; `csSetDrawEnable(obj_b, 1)`, `obj_b->visible = 1`,
   `obj_b->light = lightGetEnv(0)`; copy `motion.pos`/`rot` into `obj_b` so this frame's pair tables and hit
   tests see it in place.
7. **`other`** (if any) gets type_a the same way (5-6) if it is active; if it is parked it only gets
   `type = type_a` and stays parked: dead, HP 0, wreck state 4, `csSetDrawEnable(obj_a, 0)`,
   `carBodyObjectHide(key_a)`, position far away (SURVIVAL 3.1). When the port unparks it later it must start it
   with `aiCarStart(&aiCars[j], carSlotOwners[aiCars[j].slot], 1)` (the slot's type, not a remembered one).
8. With no `other`, the old model is parked the same way (draw off, body hidden, object far away).

What does **not** need a reset: the HUD has no car picture in retail (`TEXTURE_BANKS.md` 4.2, CONFIRMED); the
port's own HUD reads `humanCarTypes[i]` (set in 4). Pair tables, radar and sound positions are recomputed each
frame. The camera reads the record (`camera.c`; confirm `carSetViewMode` 800E7380 caches no object).
`aiCarTotal` unchanged (the parked Minion record stays counted; the port's round-end rule must ignore parked
cars, SURVIVAL 3.3).

### 3.4 Hazards (step 3)

- Homing missiles and lock-ons that hold an object pointer keep following the old object (now parked far away).
- Kills/attacker ids follow object ids: the human's id moves with it (i + 1), so scoring stays with the player.
- The swapped-in model's tire/damage switch states are whatever its last driver left.
- Shared specials (1.10): a Juggernaut that is a second Mr. Slam/Twister shares the arm/tornado.
- Calling it during a car's own update (e.g. from a damage callback) would change the record under the
  caller: only from the frame hook.

### 3.5 Verification (step 3)

Static: define-off and step-1-only images unchanged; with `TM2_PORT_CAR_SWAP` the only differences are new
`.port_text` functions (`port_functions` in the json), `port_changed_functions` empty, proofs pass. Runtime
is the port's (Juggernaut scenarios: swap with turbo on, frozen, burning, invisible, mid-slam, mid-tornado;
swap back; repeated swaps; human ↔ parked, AI ↔ parked, human ↔ human).

Size: **2-3 port functions** (~250 lines C), 0 game functions, 0-1 tsv row.

---------------------------------------------------------------------------------------------------------

## 4. Step 4 (outline): texture banks (`TEXTURE_BANKS.md` 5) — done, see `CAR_GROUPS.md` 9

Behind `TM2_PORT_TEXTURE_BANKS` (needs step 2 for its value, works with step 1 for duplicates' own CLUTs):
- `sessionLoadArenaAndCars` (8012BAFC): bank 0 for the level TMS; per car group n, a fresh placement in bank n
  (no reuse by key across groups; the arena's rectangles copied where the arena holds the key, 3.2), slot
  stride 32 (the Dark Tooth stride fix of SURVIVAL 4.2: four functions there).
- `modelRemapTextures` (80120744, `world_collapse_warps.c`): also write the bank into tpage bits 9..15
  (`UVTpWord.rest = vramTexSlotGroup`).
- `vramPlaceTexture` / `vramUploadClut`: per-bank bookkeeping (port side for the actual VRAM).
- CLUT animations: `clutAnimRegister` per group; the invisibility fade per car (fixes 1.10's shared fade).
- The port: PSY-Z samples bank `attr >> 9`, `LoadImage`/`StoreImage` of the bank-aware paths go to the bank.
Size: ~6-8 game functions + port GPU work. Not planned in detail here.

---------------------------------------------------------------------------------------------------------

## 5. Working notes for the executing agent

- Work only in the port variant; every change under its define with the retail C in the `#else`.
- Build and tool files are CRLF (`build.py`, `tools/match_unit.py`); Python writes with `newline='\n'`; Bash
  heredocs here collapse backslash escapes (write edit scripts with the Write tool).
- Pure C (RULES.md): no asm; `volatile` only for hardware/ISR/timing.
- Record the expected `port_changed_functions` list in the PR and compare it mechanically.
- Docs: a short `docs/CAR_GROUPS.md` (or a section in SURVIVAL.md) per step with the port-facing facts:
  storage names and how to find their addresses (`symbols.tsv`), `portCarOptions` bits, `portCarsPerArena`,
  `portCarSetGroup` contract; add the defines to the table in `SOURCE_BUILD.md` "Decomp-side port options".

---------------------------------------------------------------------------------------------------------

## 6. Questions for the port

- **Q1 (D1).** Keys (= type for the first car of a type, overflow 16..31 for duplicates) instead of the bare
  load group of SURVIVAL 2.5. Same per-car tables, retail values in records. Accept?
- **Q2 (D2).** Keep Minion's second root registered (retail slots; up to 32 slots for 16 cars) rather than
  skipping it?
- **Q3 (D3).** `portCarOptions` bits (0 repeats in pickers, 1 high DMD area, 2 farthest-free starts), all 0 =
  retail: OK, and does the port want any of them tied to the define instead?
- **Q4.** Engine sound per car for duplicates is **not simple**: `carSoundUpdate` plays/stops/tests the
  engine by sound id per type (`soundPlay`/`soundIsActive`/`soundStop`, no instance form of `soundIsActive`),
  and `worldUpdate` stops it by id. Two cars of a type share one engine sound. Leave it (port mixes), or a
  later request (instance ids through `soundPlayVariant`/`soundStopInstance`)?
- **Q5.** Single shared specials with duplicates (tornado, strike, leap, Mr. Slam's arm/claw, Dark Tooth's
  head): needed for the port's modes? Each is its own small change later.
- **Q6.** Duplicates share each CLUT, so one car's invisibility fade fades its twin until step 4. Acceptable
  until then?
- **Q7.** Step 2 moves the car arrays in every variant build (retail sizes without the define), changing the
  default variant's image (addresses only). OK, or only with `TM2_PORT_CARS` (needs conditional tsv rows)?
- **Q8.** Opponent slots ≥ 9 in a port table (record bytes stay retail; slot 9 of the record unused) rather
  than using the record's `opponent_car[9]`?
- **Q9.** Tournaments with > 9 opponents: the card screen overflows. Skip the card screen, show the first 9,
  or the port draws its own?
- **Q10.** Round end on `humanCars[0]` only (`world_update.c:425`) and the other 4-player rules of SOURCE_BUILD
  "Not in the variant": the port's, as before?
- **Q11.** Radar 12 blips per view, 128 coordinate systems, 35 sound instances: raise any of them in step 2?
- **Q12.** 16-player online with remote humans as human records would need `TM2_MAX_HUMANS` > 7 (object id 8)
  and every per-human array: not in this request; confirm remote players are AI records driven by the net.
- **Q13.** Car DMD area: a 1 MiB block in `.port_bss` (linker-placed, in every variant) or a fixed address
  outside the port sections?
- **Q14.** Swap: keep pickup weapons and reset specials (planned), or reset everything / keep everything? HP:
  scale by default?
- **Q15.** Swap semantics when the target group's car is driven by an active car: allowed (two-way swap,
  planned) or refused?
