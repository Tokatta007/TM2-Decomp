# Request 35: any car fires another car's special (Gun Game). Census and design

**For the PC port:** built as `TM2_PORT_SPECIAL_AS` (section 4, "As built"): 10 specials can be Gun Game levels (Sweet
Tooth, Outlaw 2, Warthog, Mr. Grimm, Thumper, Spectre, Roadkill, Axel, Shadow, Minion); Hammerhead, Grasshopper,
Twister, Mr. Slam and Dark Tooth are refused. The API is `portCarSpecialAsSet(slot, type, ammo)` between frames and
the load mask `portCarSpecialLoad`; the call sequence is 4.5. Sections 0-3 are the census and the design (read-only
research, before the build).

Read-only research, 2026-10-13, tree at main after PR #38. Tags: **CONFIRMED** = read in code or counted on the
disc; **LIKELY** = inferred from code; **GUESS** = not checked.

Sources: `src/semantic/weapon_basic.c` (`weaponFireSpecial`), `weapon_tornado.c`, `strike_update.c`,
`weapon_leap_homing_update.c`, `weapon_leap_homing_init.cpp`, `weapon_slam_swing.c`, `car_wheel_sway.c`,
`misc_one_liners.c` / `arena_load_register.c` (slam setters), `asset_pools.c` (`weaponAssetBind`),
`world_load_object.c`, `weapon_special_missiles.c`, `weapon_fire_specials.c`, `weapon_fire_missiles.c`,
`car_restart.c` / `weapon_flame_update.c` (flame), `car_crash.c` / `car_collision.c` (Hammerhead),
`ai_route_setup.c` (`weaponBlockInit`), `weapon_tick.c`, `weapon_tick_init.c` (`aiWeaponFireDecide`), `weapon_select.c`,
`ai_route_nodes.c` / `ai_steps.c` / `ai_route_helpers.c` (AI), `hud_icons.c` / `hud_main.c` (HUD),
`ai_dark_tooth_boss.c`, `port_variant_arrays.c` (33-2a mechanism), `shell_main.c` (`sessionLoadArenaAndCars`); docs
`CAR_GROUPS.md` 7, 8.3, 10, 11, 12, 14; `PER_CAR_PLAN.md` A; `SURVIVAL.md` 1.7, 2.2; `CAR_STATS.md` 3, 6;
`TEAM_COLORS.md` 1.1; `TEXTURE_BANKS.md` 2.2; `SIXTEEN_CARS.md` 4.3-4.5. The DMD id census was made with the repo's own
reader (`tools/check_car_groups.py` `dbscan_ids` + `tools/dmd_lod_dump.py`, run read-only from the scratchpad).

## 0. Summary

| type | special | class | load assets of its type? |
|---|---|---|---|
| 0x10 Sweet Tooth | spiral missile | **(a)** | yes: ST.DMD 2301 (also in BB) |
| 0x20 Hammerhead | crush-on-ram (fire only refunds) | **(c)** stats flag + `carCrashStart` type test | no |
| 0x30 Outlaw 2 | taser | **(a)** | no (arcs + the level's lightning 658) |
| 0x40 Warthog | 3 coloured missiles | **(a)** | no (the level's 2201) |
| 0x50 Mr. Grimm | skull missile | **(a)** | yes: MG.DMD 2322 |
| 0x60 Grasshopper | homing leap of the car itself | **(a)** | no (none) |
| 0x70 Thumper | flame | **(a)** | no (the level's flame sprites) |
| 0x80 Spectre | ghost missile | **(a)** | yes: SP.DMD 2326 |
| 0x90 Roadkill | boomerang | **(a)** | yes: RK.DMD 2321 |
| 0xA0 Twister | tornado | **(a)** with one fix: the funnel draws **the Twister's model**; the port variant gives it the firer's | yes: IR.DMD 2323 |
| 0xB0 Axel | shockwave | **(a)** for the effect; the ring animation is **part of Axel's body** (b-visual) | only for the ring (AX.DMD 2324) |
| 0xC0 Mr. Slam | arm and claw | **(b)**: the crane is Mr. Slam's own model, and its state machine stalls without it | leave out |
| 0xD0 Shadow | strike | **(a)** (its animation step is shared: cosmetic) | yes: HS.DMD 2325 |
| 0xE0 Minion | 3 coloured missiles + freeze missile | **(a)** | no (the level's 2201, 2202) |
| 0xF0 Dark Tooth | none for a human; the AI's boss controller (head) | **(c)**: no fired special exists; the boss logic is AI-only, needs a head per car and the level-8 rules | leave out |

So 12 of the 15 can be a Gun Game level: 10 as they are, Twister with the funnel fix, Axel with or without its ring;
Hammerhead needs a small collision change (c). Mr. Slam and Dark Tooth are left out. Sounds need nothing (one VAB).
The HUD icon is not per type. Ammo comes from one table (`weaponBlockInit`). AI touch points: 6 lines.

## 1. Census

### 1.1 What the car DMDs hold (CONFIRMED, disc; DMD tree paths)

Every level DMD (all 14) holds **2201, 2202, 2208** (homing/coloured missile, freeze missile, napalm; `weaponStoreProjectileModel`)
and 2209/2210/2212/2217 (+2216 on some): the pickup weapons. The car DMDs hold:

| file | bytes | special ids (tree path) | used by the load (`worldLoadObject`, world_load_object.c) |
|---|---:|---|---|
| ST | 29,516 | 16 / 17(switch) / **2301** | `weaponStoreProjectileModel` → `weaponSpiralMissileModel` |
| MG | 32,308 | 80 / 81 / 2308 / **2322**(op3) | 2322 → `weaponGrimmSpecialModel`; 2308 has no case (a plain parent node) |
| SP | 34,040 | 128 / 129 / **2326** | → `weaponGhostMissileModel` |
| RK | 39,168 | 144 / 145 / **2321** | → `weaponBoomerangModel` |
| IR | 35,876 | 160 / 161 / **2323**; root **160** | 2323 → `weaponAssetBind` 0x913 (`weaponTornadoObject`, csAlloc); 160 → `weaponAssetBind` 0xA0: the **funnel object, whose model is the Twister root** |
| AX | 33,204 | 176 / **2324**(op9, a direct child of the car root) | `objRegistryAdd(2324)` → `objRegistryId2324` (animation node **inside Axel's car model**) |
| HS | 38,484 | 208 / 209 / 2327 / **2325**(op9) | 2325 → `weaponAssetBind` 0x915 (strike object, csAlloc, model = the node) + `objRegistryAdd(2325)` (its animation) |
| FL | 27,032 | 192 / 193 / (197) / **201** / **202** (op5) | `slamArmRegister` / `slamClawRegister`: nodes of Mr. Slam's own model |
| PV | 36,312 | 96 / 97 / 2309 / 309 | no case for either (no code reads them) |
| BB | 61,660 | roots: 240 (car, 2301 inside), **249** (head, root 1), **2214** (root 2) | 249 → `weaponAssetBind` 0xF9 (head object, **visible = 1**); 2214 → `weaponRicochetModelAlt` (Dark Tooth's ricochet pickup look); 2301 as ST |
| HH, OT, WH, TH, MN | 24,940-35,124 | none | - |

Model pointers stored by id (`weaponStoreProjectileModel`, weapon_special_missiles.c:174) are **never cleared**
between loads (no writer besides that function; `weaponObjectsDetach`, round_reset.c:149, clears only the tornado,
funnel, strike, head and pools). In a match without the type they point into the previous load's DMD area.
`weaponFireSpiralMissile` (weapon_special_missiles.c:233) and `weaponFireDarkToothMissile` (weapon_fire_specials.c:325)
store the spiral model with no null test; Grimm/ghost/boomerang test for 0 only. So **firing a missile special whose
car file is not loaded draws a stale pointer** (hazard H1).

### 1.2 Per special (the fire path is `weaponFireSpecial`, weapon_basic.c:116; switch at :170)

| type | fire code | projectile / object model | textures | sound (all in `SCREEN\UACORE.VAB`) | car body / parts | per-type state | pools |
|---|---|---|---|---|---|---|---|
| 0x10 Sweet Tooth | :171 `weaponFireSpiralMissile(lookup->id, ..., 15)` | 2301 (ST or BB) | ST image 1 + own 256-colour CLUT | 0x3D | none (muzzle from the firer's stats `gun[]`) | none | projectiles 16 (48 with bit 3) |
| 0x20 Hammerhead | :179 `ammo[0]++` and return | - | - | 37 (in `carCrashStart`) | the ram uses both cars' stats | `stats.mounts_cars` (+2; HH, AX, MN, DT = 1); `car_crash.c:87,92` `kind == 32` takes a charge and plays 37 | - |
| 0x30 Outlaw 2 | :185 `weaponTaserTarget` | arcs (`weaponArcs`), level anim 658 | level | 0x27 | none | none (`ai->f458`) | arcs 5 (16) |
| 0x40 Warthog | :192 3 x `weaponFireColoredMissile` | 2201 (level) | level | 0x29 | none (`mount_x`, `launch_height`, `gun[]` of the firer) | none | projectiles |
| 0x50 Mr. Grimm | :213 `weaponFireGrimmSpecial` | 2322 (MG) | MG: 13 16-colour CLUTs (skull frames) | 0x2B | none | none | projectiles |
| 0x60 Grasshopper | :222 `carSpinOut` + `weaponHomingLeapInit` | none (the firer itself leaps; 309/2309 are never read) | - | 0x2D | the firer's own car (generic: position/rotation pointers of the firer's record); turbo flame by the firer's key (`LEAP_TURBO`, weapon_leap_homing_update.c:43 with `TM2_PORT_CAR_GROUPS`; without it the Grasshopper's 0x66) | `homingLeapState` (33-2a part LEAP) | - |
| 0x70 Thumper | :227 `weaponFlameStart` | flame sprites (level 650) | level | 47 (per car with `TM2_PORT_CAR_SOUND` bit 4) | the emitter holds the firer's object | none (pool) | flame emitters **2** (16 with bit 3) |
| 0x80 Spectre | :230 `weaponFireGhostMissile` | 2326 (SP) | SP: 4 16-colour | 0x31 | none | none | projectiles |
| 0x90 Roadkill | :239 `weaponFireRoadkillBoomerang(.., lookup)` | 2321 (RK) | RK: 3 16-colour (images 5..7) | 0x33 by projectile slot | none | none | projectiles |
| 0xA0 Twister | :258 tornado | 2323 tornado object (IR) **and the funnel object = the Twister root model** (the spinning car shown while the firer is hidden) | IR: 4 16-colour | 0x35 | hides the firer (`lookup->visible = 0`), its shadow by key (`carBodyObjectHide(TM2_CAR_KEY(otherid, 0xa0))` = the firer's own key); the funnel must draw the **firer's** model | `weaponTornadoState` + 2 objects (33-2a part TORNADO); `weaponTornadoUpdate` calls `rand()` while active | - |
| 0xB0 Axel | :285 area damage loop (generic) + `objRegistrySetStep(0x914, 0, 1)` | ring = node 2324, a switch **in Axel's car model** | AX: 2 own 256-colour CLUTs | 0x37 | the ring draws only with an Axel car object; the damage, spin-outs, blue smoke and explosions are generic | `objRegistryId2324` (part AXEL) | - |
| 0xC0 Mr. Slam | :329 `weap->slam` record `active` | arm 201 / claw 202 = nodes of Mr. Slam's model | - | (slam sounds in the swing code) | **yes**: `slamArmSetAngle` / `slamClawSetAngle` store the angle **only when the object is non-null** (misc_one_liners.c:532, arena_load_register.c:160), so without the crane `weaponSlamSwingUpdate`'s phases never end (claw angle stays at rest, weapon_slam_swing.c:221-235): the record stays active for good; the lift follows the crane geometry (`weaponSlamCarryTarget`, car_wheel_sway.c:301); claw records are only allocated for 0xC0 records (`weaponBlockInit` :309, `carRestart` :204, `aiCarStart` :296) | arm/claw (part SLAM) + claw record | claw records 4 (17) |
| 0xD0 Shadow | :339 strike / detonate | 2325 strike object (HS); its animation = the same node (`objRegistrySetStep(0x915, ..)`) | HS: 2 16-colour | 0x3B | none (object starts at the firer) | `weaponStrikeState` block (part STRIKE); the animation step lives in the node: shared by every strike drawn with that node | - |
| 0xE0 Minion | :374 3 coloured + `weaponFireFreezeMissile` | 2201, 2202 (level) | level | 13 | none | none | projectiles |
| 0xF0 Dark Tooth | :398 `break` (nothing) | - | - | - | - | the boss controller (`aiDarkToothBossUpdate`, AI-only, `weapon_tick_init.c:104`): head 249 per car, missiles (2301 model), swoops 61/66, level-8 stage rules (`CAR_GROUPS.md` 11) | - |

### 1.3 Sounds (CONFIRMED)

The disc has one bank, `/SCREEN/UACORE.VAB` (556,256 bytes), loaded at boot (`shellLoadUaCore`); every special's
sound id is in it, so **no sound asset is per car**. What is per id: the request list sizes (`soundMgrInitLists`,
`CAR_GROUPS.md` 12.1): many cars firing the same special at once merge into (or are dropped from) that id's list, as
two retail cars of one type do. Only Thumper's flame is per car (instance by slot, bit 4, `portCarFlameSound`).
`portCarResetSpecials` stops 0x35 by id (all tornado sounds; cosmetic).

### 1.4 HUD icon (CONFIRMED)

Not per type. `UAdashInitDashboardIcons` (hud_icons.c:57) parses the **level's** asset 1260 into 10 weapon icons;
slot 0 (the special) is one generic icon; `hud_main.c:126-170` draws slot i with `weapons.ammo[i]`. Nothing in the C
changes; the port may label the level (e.g. `carTypeName(as)`) in its own HUD.

### 1.5 Ammo (CONFIRMED)

`weaponBlockInit` (ai_route_setup.c:266, switch :297-318) sets `max_specials` (+0x0B) and `recharge_period` (+0x2C)
by the record's type (table `CAR_STATS.md` 3; Mr. Slam also allocates its claw record). `weaponTick`
(weapon_tick.c:53-60) adds one special per period up to `max_specials`; for AI records (and `infinite`) every ammo
count is set to 1 each tick (:46-49), so AI ammo never runs out. `carStatsLoad` holds no ammo; it holds
`mounts_cars` (+2, Hammerhead's crush) and Mr. Slam's rest angles.

### 1.6 AI use of specials (CONFIRMED)

| where | what reads the type |
|---|---|
| `weaponFireSelected` (weapon_select.c:80) | `carHasMeleeSpecial(vehicle)` (0x30, 0x70, 0xA0, 0xB0, 0xC0; ai_route_helpers.c:176): an AI fires the special when its target is within 400 and ahead |
| `aiWeaponFireDecide` (weapon_tick_init.c:68) | `ai->type == 0x30`: Outlaw toggles `selected` 0/1 |
| `ai_steps.c:728, 759, 1238` | `carHasMeleeSpecial(ai->type)`: approach/ram behaviour for melee specials |
| `ai_steps.c:1425, 1476` | `ai->type == 0x20`: Hammerhead rams |
| `ai_steps.c:1659` | `ai->type == 0xC0`: Mr. Slam stops 120 short (excluded type) |
| `aiCarSetupProfile` (ai_route_nodes.c:33) | weapon weight tables and `fire_chance` per type (which weapon slot the AI selects, slot 0 = the special); Gun Game forces the selection on the port side anyway |
| `weaponFireSpecial` 0x30/0xA0/0xC0 | `DRV(car)->f458 = 1` for AI (generic) |
| `aiWeaponFireDecide` :104 | Dark Tooth boss update (level 8 or bit 6) |

## 2. Classification and why

- **(a) any car fires it with only its assets loaded:** Sweet Tooth (ST), Outlaw 2, Warthog, Mr. Grimm (MG),
  Grasshopper, Thumper, Spectre (SP), Roadkill (RK), Shadow (HS), Minion; **Twister** (IR) once the funnel takes the
  firer's model (else a Twister spins in the air over a Thumper); **Axel**'s effect (the ring is Axel's body: either
  no ring, or a ring object per firer with node 2324 as its model, 3.4). All muzzle/launch geometry comes from the
  firer's own stats; the per-type states (tornado, strike, leap, Axel) need a per-car entry (3.2), exactly as
  33-2a's duplicates.
- **(b) needs its own car's body:** **Mr. Slam** (the crane is nodes 201/202 of FL's model, its angles are not even
  stored without them, so the slam never ends; the lifted car hangs from the crane's geometry; claw records belong to
  Mr. Slam records). Axel's ring visual only.
- **(c) needs other state:** **Hammerhead** (the special is `stats.mounts_cars` + the `kind == 32` charge and sound in
  `carCrashStart`, car_crash.c:87/92; feasible: 3.6), **Dark Tooth** (a human's special is empty; the AI boss
  controller needs a head per car (one BB.DMD, 61,660 bytes, per car), is AI-only, and is tied to the stage-2 rules;
  leave it out, or define a new "Dark Tooth missile" fire case later: not retail behaviour).

## 3. Design for the port variant

### 3.1 Define, switch, storage

- `--port-define TM2_PORT_SPECIAL_AS`; needs `TM2_PORT_CAR_SPECIALS` (and so `TM2_PORT_CAR_GROUPS`; `#error`
  otherwise); works with and without `TM2_PORT_CARS`, `CAR_SWAP`, `DROPIN`, `DARK_TOOTH`, `CAR_SOUND`, banks.
- `signed char portCarSpecialAs[TM2_CAR_SLOT_CAP]` (64; .port_bss, every variant): per **car slot**, the car type
  whose special that car fires, -1 = its own (all -1 = retail). Per slot because every caller has the slot
  (`weaponFireSpecial`'s `otherid`, the tornado's object, the leap's `owner_id`) and the per-car state must stay with
  the car; the port re-sets it after a swap (`portCarSetGroup`) or a start (`portCarStart*`). The port sets it only
  through `portCarSpecialAsSet` (between frames).
- `unsigned short portCarSpecialLoad` (.port_bss word, every variant): bit `type >> 4` = load that type's special
  assets at the next load (like `portCarOptions`, read by `sessionLoadArenaAndCars`; 0 = retail).
- `struct TM2PortSpecialAs` (.port_bss, every variant; entry s = car slot s):
  `tornado[64] {state[5], tornado, funnel}` (28), `strike[64] {block[25], anim}` (104), `leap[64][15]` (60),
  `axel[64] {anim, ring}` (8): 200 bytes x 64 = **12,800 bytes** (or 7.7 KB with one union per slot plus 4 object
  words, since a slot has one as-type at a time; the separate layout reuses `portCarSpecialExchange` as is).

### 3.2 Selection: as-entries beside 33-2a's duplicate entries

`PS.sel[part]` today: 0 = retail storage, d + 1 = duplicate entry d (1..32). Add 33..96 = as-entry of slot s
(s + 33). `portCarSpecialExchange(part, i)` (port_variant_arrays.c:1367) takes entry i < 32 from `portCarSpecials`,
else from `portSpecialAs[i - 32]` (same layouts). New:

- `int portCarSpecialSelectAs(int slot, int type)`: the slot's as-entry of `type`'s part into the retail storage;
  returns a token for `portCarSpecialRestore` (unchanged). If `type` is the car's own type, it is
  `portCarSpecialSelect(portCarKey(slot, type))` (own state).
- `int portCarSpecialType(int slot, int type)`: `portCarSpecialAs[slot] >= 0 ? it : type` (the AI and collision
  helpers below).
- `int portCarSpecialAsNext(int type, int *slot)`: the per-frame loops: next slot whose as-type is `type` and whose
  own type is not, selected; 0 when none.

### 3.3 `weaponFireSpecial` (weapon_basic.c:116)

- after :147: `as = portCarSpecialAs[otherid]; if (as >= 0) vehicle = as;`
- :168: `special = as >= 0 ? portCarSpecialSelectAs(otherid, as) : portCarSpecialSelect(TM2_CAR_KEY(otherid, vehicle));`
- case 0xA0 (:258-284): unchanged; the as-entry's funnel already carries the firer's model (3.5).
  `carBodyObjectHide(TM2_CAR_KEY(otherid, 0xa0))` is the firer's own key (portCarKey returns the slot's key): fine.
- case 0xB0 (:286): with an as-entry ring object: put it at the firer (`pos`, `rot` from `lookup`), visible, then
  the step; without one (no AX loaded) skip the step: damage/spin/smoke/sound stay.
- case 0x20 (:179): unchanged (the crush is 3.6).
- 0xC0 and 0xF0 are refused by `portCarSpecialAsSet` (a non-Slam record has `weap->slam == 0`, so 0xC0 would only
  set `weapon_fired`; 0xF0 does nothing).

### 3.4 Per-frame updates (D6 loops, unchanged call sites `world_update.c:479-481`)

`weaponTornadoUpdate` (weapon_tornado.c:275), `weaponHomingLeapUpdate` (weapon_leap_homing_update.c:120),
`weaponShadowStrikeUpdate` (strike_update.c:88): after the duplicates' loop, `while ((t = portCarSpecialAsNext(T,
&s)) != 0) { ...One(); portCarSpecialRestore(t); }`. The bodies are unchanged: they read the firer through the state
(tornado `object`/`anchor`/`flags`, leap pointers, strike `weapon`/`owner_id`). The tornado's end block finds the key
with `portCarKeyOfObject(state.object)` (the firer's own key): its shadow is shown/hidden. Order: retail car, then
duplicates by key, then as-entries by slot: deterministic (the tornado calls `rand()` each active frame).
Axel's ring needs no update (the node animates itself; GUESS: step 0 shows the empty child, as on Axel's own car).
`weaponsRoundReset` → `portCarSpecialsRoundReset` (port_variant_arrays.c:1534) also clears every as-entry (leap
active/age, tornado timer + objects off, strike active/timer, ring hidden). `portCarSpecialsReset` (:1508, each load)
zeroes them (objects are re-made, 3.5).

### 3.5 `portCarSpecialAsSet(int slot, int type, int ammo)` (port function, between frames)

1. refuse 0xC0, 0xF0, a type whose assets are not loaded (3.7: its model pointer was not stored in this load), an
   unbound slot; `type == own type` means -1.
2. end the slot's current as-state the way `portCarResetSpecials` (:2251) ends own states (tornado end block with the
   car shown, strike off with its ammo bookkeeping, leap off + turbo off, ring hidden); `portCarResetSpecials` and
   `portCarRemove` also end the as-entry.
3. make the entry's objects the first time (csAlloc + csListAdd, like `weaponAssetBind`; between frames is allowed,
   as drop-in does; check room first): tornado (model = the loaded 2323), **funnel (model = `carSlotObjects[slot]`'s
   model: the firer's own car, collides 0)**, strike (2325 node, `PROJECTILE(.., 80)` defaults and the four corner
   offsets), ring (2324 node, only if AX is loaded). The models come from the first install of the type
   (`weaponTornadoObject->model` etc. after the load, or captured in `weaponAssetBind` / `objRegistryAdd` with the
   load key's type) and the strike/ring animation pointer is that node (shared, hazard H3).
4. set `portCarSpecialAs[slot]`, and the record's `max_specials` / `recharge_period` from the as-type (a 15-entry
   port table equal to `weaponBlockInit`'s, ai_route_setup.c:298-318; -1: the own type's), `ammo[0] = min(ammo,
   max)`, `f2E = 0`; Hammerhead: `stats.mounts_cars = 1` (back to `carStatsLoad`'s value when leaving 0x20).

### 3.6 AI and collision (`portCarSpecialType(slot, type)` in place of the type)

`weapon_select.c:80` (`carHasMeleeSpecial(vehicle)`), `weapon_tick_init.c:68` (`== 0x30`), `ai_steps.c:728, 759,
1238` (`carHasMeleeSpecial`), `ai_steps.c:1425, 1476` (`== 0x20`), `car_crash.c:87, 92` (`kind == 32`: a
Hammerhead-as car pays a charge and plays 37 when it mounts). `aiCarSetupProfile` stays (Gun Game selects the slot);
`ai_steps.c:1659` stays (Mr. Slam excluded).

### 3.7 Loading a type's special assets for a match without that car

Only ST, MG, SP, RK, IR, HS (and AX for the ring) need files. Three ways:

- **A. A parked spare group at the load (recommended).** `sessionLoadArenaAndCars` (shell_main.c:270, the
  `TM2_PORT_CARS` branch): after the opponents' TMS loop (:296-299) and DMD loop (:322-325), one more group per bit of
  `portCarSpecialLoad` whose type no human/opponent has, through `portCarLoadGroup(dst, n, car_index, dmd, end)`
  (`bitmapParseContainer(.., n)` / `assetInstallImage(.., n)`), before `portCarLoadDone`. Its root registers a slot
  and the type's key (it is the first of its type), so the type's single objects go to the retail globals, which no
  driver uses; `sessionBindCars` binds nothing to it; park its slot (`portCarParkSlot`, port_variant_arrays.c:2584, or
  the same three lines without `CAR_SWAP`). No new install code; groups/slots/keys are sized for spares
  (`TM2_PORT_CARS_LOADED` 32). Cost per type: its DMD in the DMD area (RAM) and its whole TMS in VRAM, plus a group, a
  slot, a key and the car object's coordinate systems.
- **B. Mid-match, no C change:** with `TM2_PORT_CAR_DROPIN`, the port calls `portCarLoadGroupLive(tms, .., dmd, ..)`
  for each type right after the load (`CAR_GROUPS.md` 14.6): the car is parked and free (the port must not start a
  record on it: `portCarFreeGroupOfType` would offer it). Same cost as A.
- **C. "Special assets only" install (VRAM saver, later):** the same extra group with a flag that makes
  `worldLoadObject` (world_load_object.c:89) keep only ids 2301, 2321-2326 (skip the car ids 16..255, so no slot, key,
  car object, body, wreck, freeze, parts) and parses only the TMS images the special subtree uses (a per-type key
  list). `portCarLoadKey()` is then 0, so registrations go to the retail globals (free: the type is not in the match).
  Saves the 128x128 body (8,192 texels) and its 256-colour CLUT row per type: what limits Paris/Swamp (8-9 types,
  `SIXTEEN_CARS.md` 4.4). The unparsed body nodes remap to "Relocatable texture not loaded" (printed, not fatal) and
  are never drawn. GUESS: not checked further.

Cost per type (CONFIRMED sizes; VRAM from `SIXTEEN_CARS.md` 4.3; the subset is a GUESS = own texels - 8,192 body):

| type | DMD (RAM) | TMS file | VRAM texels (all / own) | 256-colour CLUTs | 16-colour CLUTs | way C subset (GUESS) |
|---|---:|---:|---|---:|---:|---|
| Sweet Tooth ST | 29,516 | 24,448 | 11,520 / 9,216 | 2 | 0 | ~1,024 + 1 256-colour CLUT |
| Mr. Grimm MG | 32,308 | 30,904 | 14,592 / 12,800 | 1 | 13 | ~4,608 + 13 small CLUTs |
| Spectre SP | 34,040 | 23,032 | 10,944 / 8,640 | 1 | 4 | ~448 + 4 |
| Roadkill RK | 39,168 | 23,600 | 11,264 / 8,960 | 1 | 3 | ~768 + 3 |
| Twister IR | 35,876 | 26,232 | 12,544 / 10,240 | 1 | 4 | ~2,048 + 4 |
| Shadow HS | 38,484 | 24,040 | 11,520 / 9,216 | 1 | 2 | ~1,024 + 2 |
| Axel AX (ring only) | 33,204 | 25,000 | 11,520 / 9,216 | 3 | 0 | ~1,024 + 2 256-colour |

All seven: ~243 KB of DMD area (2 MiB with `portCarOptions` bit 1) and 7 more types in VRAM (with banks: no arena
limit; without, count them with the match's cars against `SIXTEEN_CARS.md` 4.4). A type already in the match costs
nothing. Coordinate systems: the spare's car object and parts (A/B), plus per car 2 (tornado, funnel), 1 (strike),
1 (ring) only for the slots that get that type: 16 cars x 4 = 64 of 256 (`TM2_PORT_CARS`; 128 without).

### 3.8 Functions to change (file:line)

| function | file:line | change |
|---|---|---|
| `weaponFireSpecial` | weapon_basic.c:116 (:147, :168, :286) | as-type; as-entry selection; Axel ring |
| `weaponTornadoUpdate` | weapon_tornado.c:275 | + as loop |
| `weaponHomingLeapUpdate` | weapon_leap_homing_update.c:120 | + as loop |
| `weaponShadowStrikeUpdate` | strike_update.c:88 | + as loop |
| `weaponFireSelected` | weapon_select.c:80 | `carHasMeleeSpecial(portCarSpecialType(owner, vehicle))` |
| `aiWeaponFireDecide` | weapon_tick_init.c:68 | Outlaw toggle by as-type |
| `aiSteps…` (the three functions) | ai_steps.c:728, 759, 1238, 1425, 1476 | as-type |
| `carCrashStart` | car_crash.c:87, 92 | `kind == 32` by as-type |
| `sessionLoadArenaAndCars` | shell_main.c:270 (:296-299, :322-325) | way A extra groups |
| `weaponStoreProjectileModel` users | weapon_special_missiles.c:233, weapon_fire_specials.c:325 | H1: model 0 → not visible (and clear the 7 model words at each load, `carTablesClear` / `portCarGroupsReset`) |
| port | port_variant_arrays.c:1327 (`portCarSpecialPart`), 1367 (`Exchange`), 1420 (`Select`), 1445 (`Next`), 1508/1534 (resets), 2251 (`portCarResetSpecials`), `portCarRemove` | as-entries; new `portCarSpecialAsSet`, `portCarSpecialSelectAs`, `portCarSpecialAsNext`, `portCarSpecialType` |

About 12 game functions and 4-5 new port functions; `check_car_groups.py` check 8 gets the as-entry ranges (every
reader of the special state still only inside a selection) and `EXPECTED_SPECIAL_AS`.

### 3.9 Hazards

- **H0 retail equality.** With `portCarSpecialAs` all -1 and `portCarSpecialLoad` 0: the fire path takes the old
  branch (one extra table read), the new loops find no slot, no csAlloc happens (objects are made lazily in
  `portCarSpecialAsSet`, never at the load), no group is added, the AI helpers return the own type: same code path,
  same `rand()` calls in the same order; only the port sections' addresses differ. `portCarSpecialAsSet` and the
  loads call no `rand()`.
- **H1 stale projectile models.** The model words are never cleared between loads; a missile special whose type's
  DMD is not loaded draws a stale pointer (Sweet Tooth's has no null test). `portCarSpecialAsSet` refuses such a
  type; the define also clears the words at each load and tests 0 in the two unconditional stores.
- **H2 Twister's funnel** is a copy of the Twister model in retail; the as-entry's funnel uses the firer's model
  (its damage stage and CLUT come with it). Without that, every car would turn into a spinning Twister.
- **H3 shared animation nodes.** Strike (2325) and Axel ring (2324) animate the node, not the object: all strikes
  drawn from one HS copy share one animation step; one strike ending (`objRegistrySetStep(0x915, 0, 0)`) freezes the
  others' animation until the next fire; if a real Shadow/Axel is in the match, its own model shares the node with
  the as-entries (Axel: the real Axel shows the ring when another car fires). Cosmetic.
- **H4 pools.** 16 cars on one missile level: projectiles 16 (48 with bit 3), flame emitters 2 (16), arcs 5 (16):
  set `portCarOptions` bit 3 for Gun Game. Sounds merge per id (1.3).
- **H5 level change mid-special.** `portCarSpecialAsSet` ends the old state first (the tornado's victims are
  released by the end block; a strike in flight ends with its ammo bookkeeping). Call between frames, never inside
  a selection.
- **H6 swaps and drop-in.** As is per slot: `portCarSetGroup` / `portCarStart*` / `portCarRemove` end the as-entry;
  the port sets it again for the new record.
- **H7 spare groups (way A/B):** parked and never bound; a BB spare would show its head (visible at install), one
  reason Dark Tooth stays out. With 16 cars + 7 spares: 23 groups of 32, keys within the 16 first-of-type keys.
- **H8 VRAM without banks:** 7 extra types can fail on Paris/Swamp (4.4); use banks, way C, or fewer special
  levels on those arenas.

### 3.10 Verification

Static (as for 33-2a): `build_source_only.py` + `verify_reloc.py` for the define alone with groups + specials, and
with every port define; the retail images keep their hashes; `--expect-functions`; check 8 with the as-entry ranges
and the new port functions free of `rand()`; negative control on a build without the define.
Runtime (port): (1) all -1, `portCarSpecialLoad` 0: frame by frame against the variant without the define (records,
`rand()` count, scene); (2) each of the 12 specials as a level fired by every type (Minion and Dark Tooth as firers
too), with and without the special's own car in the match, with and without banks: projectile textures right,
Twister's funnel showing the firer, the strike moving, the leap landing; (3) 16 cars on the tornado level and the
strike level at once (bit 3); (4) level change mid-tornado, mid-strike, mid-leap; (5) rounds/restarts; (6) a swap and
a drop-in on an as-car; (7) AI: melee approach for taser/flame/tornado/Axel, Outlaw toggle, Hammerhead charge use.

## 4. As built (`TM2_PORT_SPECIAL_AS`, 2026-10-13)

Built as designed in 3, with the scope the user chose: **10 borrowable specials** (Sweet Tooth, Outlaw 2, Warthog,
Mr. Grimm, Thumper, Spectre, Roadkill, Axel, Shadow, Minion). **Refused** by `portCarSpecialAsSet`
(`TM2_SPECIAL_AS_ERR_TYPE`): Hammerhead (0x20, its special is the crush), Grasshopper (0x60, the leap), Twister (0xA0,
the funnel), Mr. Slam (0xC0, the crane), Dark Tooth (0xF0, the boss). So of 3.2-3.8 the tornado and leap as-entries,
the funnel-per-firer (H2), the Hammerhead crush change (3.5 step 4, `mounts_cars`) and the leap / slam borrowing are
**not** built. The borrowed type is in `TM2_PORT_SPECIAL_AS_MASK` (`include/semantic/car_specials.h`, 0x6BBA = bit
`type >> 4` of the 10).

`--port-define TM2_PORT_SPECIAL_AS`; needs `TM2_PORT_CAR_SPECIALS` (`#error` otherwise; so `TM2_PORT_CAR_GROUPS`
too); the load mask (4.4) needs `TM2_PORT_CARS` as well. Works with every other port define.

### 4.1 API (the port calls it between frames, never from a game callback)

Port code in `.port_text` (`src/semantic/port_variant_arrays.c`; declarations in `car_specials.h`). Addresses from the
builds' `symbols.tsv`; read them from your build's `symbols.tsv` / `functions.tsv`:

| function | groups + specials + as | full car stack + as | every port define + as | use |
|---|---|---|---|---|
| `int portCarSpecialAsSet(int slot, int type, int ammo)` | 0x80544674 | 0x80581644 | 0x805834B4 | **the API**: car slot `slot` fires `type`'s special (-1, 0 or its own type = its own); `ammo` specials (-1 keeps the count), capped at the type's `max_specials`; ends the old special. Returns 1 or an error (4.2) |
| `int portCarSpecialType(int slot, int type)` | 0x80543C38 | 0x80580C08 | 0x80582A78 | the type the slot fires (`type` = its own); the AI / crush hooks; the port may call it (e.g. its HUD label) |
| `int portCarSpecialSelectFire(int slot, int *vehicle)` | 0x80543CF0 | 0x80580CC0 | 0x80582B30 | hook: `weaponFireSpecial`'s selection |
| `int portCarSpecialSelectAs(int slot, int type)` | 0x80543C6C | 0x80580C3C | 0x80582AAC | internal: select the slot's as-entry |
| `int portCarSpecialAsNext(int type, int *slot)` | 0x80543D58 | 0x80580D28 | 0x80582B98 | hook: the borrowed strikes' loop (`weaponShadowStrikeUpdate`), the drop-in compaction |
| `void portCarSpecialAsRing(int slot, void *firer)` | 0x80543E00 | 0x80580DD0 | 0x80582C40 | hook: `weaponFireSpecial` 0xB0, the ring at the firer |
| `void portCarSpecialAsEnd(int slot)` | 0x80543E90 | 0x80580E60 | 0x80582CD0 | ends the slot's borrowed strike / ring (the swap's `portCarResetSpecials` calls it) |
| `void portCarSpecialAsBlockInit(void *car, void *weapons, int human)` | 0x80544334 | 0x80581304 | 0x80583174 | hook: `weaponBlockInit` keeps a borrowed `max_specials` / `recharge_period` |
| `void portCarSpecialAsReset(void)` | 0x805449A8 | 0x80581978 | 0x805837E8 | hook: each load (`portCarSpecialsReset`) |
| `int portCarSpecialLoadPlan(void *settings)`, `int portCarSpecialLoadCar(int i)`, `void portCarSpecialLoadDone(void)` | - | 0x80581A40, 0x80581C24, 0x80581C58 | 0x805838B0, 0x80583A94, 0x80583AC8 | hooks: `sessionLoadArenaAndCars`'s spare groups (`TM2_PORT_CARS` only) |

**Storage** (`.port_bss`, every variant, after `portCarLiveTex`; tsv group `one`):

| variable | size | default variant | groups + specials + as | full car stack + as | every port define + as | contents |
|---|---:|---|---|---|---|---|
| `portCarSpecialAs` | 128 | 0x80541278 | 0x80541278 | 0x8057D728 | 0x8057D788 | `short[64]` per car slot: the borrowed type, 0 / -1 = its own (the port reads; `portCarSpecialAsSet` writes) |
| `portCarSpecialLoad` | 4 | 0x805412F8 | 0x805412F8 | 0x8057D7A8 | 0x8057D808 | **the port writes**: bit `type >> 4` = load that type's car file at the next load (4.4); never cleared by the C |
| `portSpecialAs` | 7,192 | 0x805412FC | 0x805412FC | 0x8057D7AC | 0x8057D80C | `struct TM2PortSpecialAs`: per slot the strike as-entry (+0x0000, 104 bytes each: the 0x64-byte strike block + the 2325 node), the Axel as-entry (+0x1A00, 8 each: the 2324 node, the ring object), the load's spare groups (+0x1C00 count, +0x1C04 first group, +0x1C08 car-select indices); internal |

The car slot is the record's `slot` (human +0x004, AI +0x006).

### 4.2 `portCarSpecialAsSet(slot, type, ammo)`

1. Refuses: no record drives `slot` (`-1 TM2_SPECIAL_AS_ERR_SLOT`); a type outside the mask (`-2 ..._ERR_TYPE`: the five
   refused types, or not a multiple of 0x10); a type whose car file is not loaded in this match (`-3
   ..._ERR_NOT_LOADED`: Sweet Tooth without a spiral model, Mr. Grimm / Spectre / Roadkill without their projectile
   model, Shadow without its strike object and node, Axel without its shockwave node: each set only by this load's
   install of that car file, a car of the match or a spare of 4.4); no coordinate system left for the object (`-4
   ..._ERR_ROOM`). A refused call changes nothing.
2. The first time a slot borrows Shadow: a strike object (`csAlloc` + `csListAdd`, `weaponAssetBind`'s 0x915 defaults:
   id 0x8A2, collides, size 80, the four corner offsets in the block) drawing the loaded Shadow's node; Axel: a ring
   object (no collision) drawing the loaded Axel's shockwave node 2324. The nodes are the first car of the type's
   (retail's globals). The objects live until the next load.
3. The old special ends: the slot's borrowed strike (as `strike_update.c` ends it, no blast, the refunded special taken
   back) and ring (hidden), and the car's own states (strike, tornado: car shown again, leap: turbo off, Axel step 0, Mr.
   Slam's swing: a held car let go). Flames, missiles and arcs already out finish by themselves.
4. `portCarSpecialAs[slot] = type` (-1 for its own); `max_specials` and `recharge_period` from `weaponBlockInit`'s
   table for the new type (its own type's when going back), the recharge counter (`f2E`) 0, `ammo[0] = ammo` (or kept
   with -1), capped at `max_specials`. Returns 1 (`TM2_SPECIAL_AS_OK`).

Every respawn / restart (`weaponBlockInit`) keeps the borrowed `max_specials` / `recharge_period`; the special count
itself goes back to 2 as in retail (the port sets it again if Gun Game wants). The HUD needs nothing (slot 0's icon is
generic, the count is `ammo[0]`); sounds need nothing (one VAB).

### 4.3 What the game does with it

- `weaponFireSpecial`: `portCarSpecialSelectFire(otherid, &vehicle)` replaces the 33-2a selection: a borrowing car
  switches on the borrowed type; for Shadow / Axel its own as-entry is selected (the selection mechanism of 33-2a,
  values 33..96 = slot 0..63), so the strike's fire / detonate and the shockwave step act on the borrower's copy. In
  case 0xB0 `portCarSpecialAsRing` puts the borrower's ring at its position and heading first (it stays where it was
  fired; Axel's own ring is part of Axel's body). All muzzle and launch geometry is the firer's own (census 1.2).
- `weaponShadowStrikeUpdate`: after the first Shadow and the duplicates, each slot borrowing Shadow, in slot order
  (`portCarSpecialAsNext`). The strike state is never used outside a selection (check 8).
- The AI and the crush follow the borrowed type (`portCarSpecialType`): `weaponFireSelected` (melee specials:
  Outlaw, Thumper, Axel), `aiWeaponFireDecide` (Outlaw's slot toggle), `aiTryRouteToSlowTarget`, `aiRollTakeRoute`,
  `aiShouldChaseDirect` (melee), `sub_800E9E20` and `aiFreeSteer` (Hammerhead's ram: a borrowing Hammerhead stops
  ramming), `carCrashStart` (`kind == 32`: a borrowing Hammerhead pays no charge and plays no 37; it still mounts cars
  by its stats, like Axel and Minion). Not changed: `aiCarSetupProfile` (the port selects the slot), `ai_steps.c` Mr.
  Slam's stop (`== 0xC0`), and the Dark Tooth boss update (`aiWeaponFireDecide`, by the car's own type: an AI Dark
  Tooth keeps its boss behaviour while it borrows a special).
- Projectile models (H1): `portCarSpecialAsReset` clears the four car projectile model words (spiral 2301, boomerang
  2321, skull 2322, ghost 2326) and the 2324 / 2325 nodes at each load (`carTablesClear`, before the level and car
  files install); `weaponFireSpiralMissile` and `weaponFireDarkToothMissile` test the spiral model for 0 as the other
  three already did (a missile without it keeps the homing missile's model).
- Swap / drop-in: `portCarResetSpecials` (swap both slots, park, `portCarRemove`, `portCarUnloadGroup`) ends the slot's
  as-entry and sets it back to -1: **the port sets the borrowed special again** for the record that drives the slot
  next (after `portCarSetGroup`, `portCarStartHuman` / `portCarStartAi`). The drop-in compaction moves the borrowed
  strikes' weapons pointers and owner ids with the record.
- Rounds: `weaponsRoundReset` (`portCarSpecialsRoundReset`) clears every borrowed strike's active / timer, as retail's,
  and hides the rings. The borrowed type stays (per load).

### 4.4 Loading a type's car file for a match without that car (`portCarSpecialLoad`, way A)

With `TM2_PORT_CARS` and `portCarOptions` bit 1 (the car DMD area), `sessionLoadArenaAndCars` adds one **spare load
group** per bit of `portCarSpecialLoad` whose type needs its car file and that no human or opponent drives: after the
opponents' TMS (before the CLUT copies) and after their DMDs (before `portCarLoadDone`), in car-select order, up to 16
and as long as groups 1..32 last. The spare's root registers a slot and the type's key (the first of its type: its
single objects and models go to retail's globals, which no driver uses), `portCarSpecialLoadDone` parks its slots (not
drawn, out of the hit pass, body hidden, far below the arena, free). No record binds to it; the drop-in's
`portCarFreeGroupOfType` would offer it as a free car (do not start a record on it unless you mean to). Without bit 1,
without `TM2_PORT_CARS`, or with the mask 0 nothing is added (retail). The bits of the types that need no file
(Outlaw 2, Warthog, Thumper, Minion: the level's models) and of the refused types are ignored.

**Fix of 2026-10-14 (found by the port, PR #42):** `portCarSpecialAsReset` cleared the whole `portSpecialAs`,
including the load plan (`load_n`, `load_first`, `load_car`, from +0x1C00). It runs from `portCarSpecialsReset` at the
level's reset, which comes between `sessionLoadArenaAndCars`'s TMS pass (where `portCarSpecialLoadPlan` makes the
plan) and its DMD pass (which reads it again through `portCarSpecialLoadCar`), so the spares' DMDs were never
installed and every borrow of a spare's special (Sweet Tooth's, ...) returned `TM2_SPECIAL_AS_ERR_NOT_LOADED` (-3).
Now the reset clears only the as-entries (+0x0000..+0x1BFF). The plan is still made fresh at the start of every load:
`portCarSpecialLoadPlan` (first thing `sessionLoadArenaAndCars` does for the spares) sets `load_n` and `load_first`
before anything reads them, and `portCarSpecialLoadCar(i)` and `portCarSpecialLoadDone` only look at entries below
`load_n`, so nothing carries over between matches (without `TM2_PORT_CARS` the plan is never made and `load_n` stays 0).
Proved with the request 33-4 builds (`CAR_GROUPS.md` 16.6; the all-defines build has `TM2_PORT_SPECIAL_AS`).

Cost per spare type (census 3.7, CONFIRMED sizes):

| type | bit | DMD in the car DMD area | VRAM: its whole TMS (texels) | 256 / 16-colour CLUTs | coordinate systems |
|---|---|---:|---:|---|---|
| Sweet Tooth (ST) | 0x0002 | 29,516 | 11,520 | 2 / 0 | car object + parts (GUESS ~5) |
| Mr. Grimm (MG) | 0x0020 | 32,308 | 14,592 | 1 / 13 | car object + parts (GUESS ~5) |
| Spectre (SP) | 0x0100 | 34,040 | 10,944 | 1 / 4 | car object + parts (GUESS ~5) |
| Roadkill (RK) | 0x0200 | 39,168 | 11,264 | 1 / 3 | car object + parts (GUESS ~5) |
| Axel (AX) | 0x0800 | 33,204 | 11,520 | 3 / 0 | car object + parts (GUESS ~5) |
| Shadow (HS) | 0x2000 | 38,484 | 11,520 | 1 / 2 | car object + parts (GUESS ~5) |

All six: 206,720 bytes of the 2 MiB DMD area and 71,360 texels of VRAM (with texture banks, bit 5, each spare gets
its own bank: no arena limit; without banks count them with the match's cars against `SIXTEEN_CARS.md` 4.4: Paris and
Swamp may not fit). Plus per borrowing slot one coordinate system for a Shadow strike object and one for an Axel ring
(of `TM2_MAX_COORD_SYSTEMS`, 256 with `TM2_PORT_CARS=16`). A type already in the match costs nothing. A further
possibility (not built): way C, the special subtree only, saves the body's VRAM (3.7).

### 4.5 The port's call sequence (Gun Game)

1. **Before the match load:** `portCarSpecialLoad` = the bits of the gun list's specials among Sweet Tooth, Mr. Grimm,
   Spectre, Roadkill, Axel, Shadow (0x2B22 = all six); `portCarOptions` |= bit 1 (DMD area), and bit 3 (raised pools: 16
   cars firing one missile special), bit 5 (banks) when VRAM is tight.
2. **The load** resets every slot to its own special (`portCarSpecialAs` all -1).
3. **Before the first frame and at each car's level change** (between frames): for a special level of type T,
   `portCarSpecialAsSet(slot, T, ammo)` and the port selects weapon slot 0 (`weapons.selected`) as it does for Gun
   Game; for a weapon level, `portCarSpecialAsSet(slot, -1, 0)` (its own special back, no specials) or leave the
   borrowed one and keep `selected` on the weapon. Check the result: `-3` means the file was not in the load mask.
4. **After a swap or a drop-in start / removal:** set the record's special again (the slot was reset to its own).
5. **After a respawn:** `ammo[0]` is retail's 2 again; set it if Gun Game keeps its own count
   (`portCarSpecialAsSet(slot, T, n)` again is fine: it only ends what is running).

### 4.6 Status and proofs

- **Built and proved** (`tools/build_source_only.py` + `verify_reloc.py` against E0 `work/build-20261013-210155-514511`,
  placement check as `build.py` runs it; port RAM end 0x80C00000):

  | configuration | variant sha256 | `port_changed` | port sections end |
  |---|---|---|---|
  | no port define | `03e5d4c9…` (placement `b95e6fed…`, unchanged) | none (`.port_bss` + 7,324 bytes) | 0x805430A0 |
  | `TM2_PORT_CAR_GROUPS` + `TM2_PORT_CAR_SPECIALS` + `TM2_PORT_SPECIAL_AS` | `5461ea0b…` | 64 | 0x80546264 |
  | the full car stack (`CARS=16`, `CAR_SWAP`, `TEXTURE_BANKS`, `CAR_SPECIALS`, `DARK_TOOTH`, `CAR_SOUND`, `CAR_CLUTS`, `CAR_DROPIN`) + `TM2_PORT_SPECIAL_AS` | `ec56c5cd…` | 142 | 0x80589A68 |
  | every port define (+ `FAR_LOD`, `FAR_DISTANCE`, `SPU48`) + `TM2_PORT_SPECIAL_AS` | `6a79c215…` | 182 | 0x8058B938 (6,768,328 bytes below 0x80C00000) |

  The define changes 13 game functions (`weaponFireSpecial`, `weaponShadowStrikeUpdate`, `weaponBlockInit`,
  `weaponFireSelected`, `aiWeaponFireDecide`, `aiTryRouteToSlowTarget`, `aiRollTakeRoute`, `aiShouldChaseDirect`,
  `sub_800E9E20`, `aiFreeSteer`, `carCrashStart`, `weaponFireSpiralMissile`, `weaponFireDarkToothMissile`; +
  `sessionLoadArenaAndCars` with `TM2_PORT_CARS`, already step 2's) and adds 12 port functions (9 without
  `TM2_PORT_CARS`) and 7 static helpers. The retail images keep their hashes (hybrid `9219f7ba…`, source-only
  `d755c86e…`, `--sdk-from-lib` `f557c1e3…`; full `build.py --source-only --sdk-from-lib --relocatable --port-variant`,
  `work/build-20261014-040241-521117`, published `out/TM2_SOURCE_PORT.EXE` `03e5d4c9…`).
- **`tools/check_car_groups.py` check 13** (new): the port functions in `.port_text` without `rand()`; the hooks (14
  functions with the full stack); no game function touches `portCarSpecialAs` / `portCarSpecialLoad` / `portSpecialAs`;
  the four car projectile model words are written only by `weaponStoreProjectileModel` and the load reset (which
  clears all four), and every game load of one is tested against 0 before use; the mask is the 10 types and none of
  the 5 refused, and `portCarSpecialAsSet` holds it. Check 8 counts the special-as selections as selecting (the
  strike / shockwave state stays inside selections). Negative control: on the groups + specials build without the
  define check 13 fails (no hooks, the spiral model stores unguarded).
- **Retail equality** with every slot -1 and `portCarSpecialLoad` 0 (H0): the fire path selects as before, the strike
  loop finds no slot, no object is made, no group is added, every AI / crush type is the car's own, no `rand()` call is
  added or moved. Differences from the variant without the define: the four model words and the 2324 / 2325 node words
  are 0 after a load until a car file sets them (retail kept the last load's; only those cars' own code reads them),
  and the two spiral stores test for 0.
- **Not runtime-tested.** Port tests: (1) all -1, mask 0: frame by frame against the variant without the define; (2)
  each of the 10 specials fired by every type (Minion and Dark Tooth as firers too), with and without that car in the
  match (mask), with and without banks: the projectile looks, the strike drives and detonates, the ring shows at the
  firer; (3) 16 cars on the strike level and on a missile level (bit 3); (4) a level change mid-strike, mid-tornado of a
  Twister that then borrows, mid-swing of a Mr. Slam; (5) rounds and respawns (max / recharge kept); (6) a swap and a
  drop-in on a borrowing car (it gets its own special back until the port sets it); (7) AI: melee approach for taser,
  flame and shockwave, Outlaw's toggle, a borrowing Hammerhead stops ramming; (8) the refusals (-2 for the five, -3
  without the file).

### 4.7 Open points

- The borrowed ring stays where it was fired (it does not follow the car as Axel's own does); the strike and ring nodes
  are shared with the loaded Shadow / Axel (H3: one strike ending pauses the others' animation; a real Axel in the
  match shows its ring when another car fires).
- An AI Dark Tooth that borrows a special keeps its boss update (by its own type); a human Dark Tooth has none.
- `ammo[0]` returns to 2 at every respawn (retail); if Gun Game wants its own count, the port sets it after respawns.
