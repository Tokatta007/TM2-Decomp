# Bugs and oddities in the retail game

Bugs and odd code in the **original** Twisted Metal 2 (US, `SCUS_943.06`)
found during the decompilation. The PC port recompiles retail, so it has
all of them. Some may act differently on PC: a write to address 0, a write
past the end of an array into whatever lies next to it, or code that relies
on the PS1 memory layout.

Static analysis only. Each claim was checked against the retail
disassembly (`bin/SCUS_94306R.EXE`, `mipsel-none-elf-objdump`; text loads
at 800CDC54, $gp = 801809D4). Nothing was run. Addresses are retail.
Functions are named by VA first and current name second, because names may
still change.

Tags: **CONFIRMED** = seen in the retail code. **LIKELY** = follows from the
code, but the in-game effect was not seen running.

PC note key:
- **harmless**: same result on PC; nothing to do.
- **check**: may crash or corrupt on PC, depending on how the port maps
  guest memory.
- **fix?**: harmless, but Vanilla Plus may want to fix it.

## Summary

| # | VA | Function | What | Visible? | PC note | Tag |
|---|---|---|---|---|---|---|
| 1 | 80156D08 | soundInit | Passes a NULL pointer to SsGetMVol, which writes 4 bytes at address 0 | no | **check** | CONFIRMED |
| 2 | 800D5AF0 | laEventsUpdate | End-of-event timer kept in a signed byte; late events skip their 20-frame finale | yes, slightly | fix? | CONFIRMED |
| 3 | 800D6188 | moscowDishUpdate | Same signed-byte timer, but dead code here | no | harmless | CONFIRMED |
| 4 | 800D35D0 and others | audioSetSfxVolume, menuVolumeApply, roundStartMusic, audioMuteMusic | The minimum clamp overwrites the maximum clamp | no | harmless | CONFIRMED |
| 5 | 801096F4 | weaponDropWrite | Default case zeroes the low half of a mine object pointer | no (unreachable) | harmless | CONFIRMED code, LIKELY unreachable |
| 6 | 80112AB0 | func_80112AB0 (explodeStoreLightningAnimation) | Asks for 30 UV records into a 10-record bank | no (as far as known) | check | CONFIRMED code, effect unknown |
| 7 | 80107B00 / 801084D8 | weaponLightningScan / weaponArcsDraw | Lightning damage uses a stale owner (last taser user) | no | harmless; feats need the caster | CONFIRMED |
| 8 | 800E9A40 | aiSlowTargetRouteAllowed | Difficulty test computed and thrown away | no | harmless | CONFIRMED |
| 9 | 800F0340 | aiSectionEdgeDistance | Wrong compare for the far z edge | no | harmless | CONFIRMED |
| 10 | 800EC994 | carWheelsUpdate | Wheel 0 tests wheel 1's ground height | no | harmless | CONFIRMED |
| 11 | 80117B40 | worldTrackVehiclesUpdate | Range test `x >= -6400 && x < -6400` never true | maybe | fix? | CONFIRMED |
| 12 | 8014C19C | debris sprite draw | Sprite size uses view 0's projection in view 1 | no | harmless | CONFIRMED |
| 13 | 80152354, 801523E8, 801524BC | renderScene | Matrix-slot index can reach 200, one past the 200-entry arrays | no (as far as known) | check | CONFIRMED code, LIKELY effect |
| 14 | 80154764..80154A80 | renderNearPolys | Near-pass midpoints average screen x/y as unsigned 16-bit; right only after the GPU's 11-bit sign extension | no on PS1 | **check** | CONFIRMED |
| 15 | 800D8230 | streaksDraw | Its update ticks run once per view, so twice per step in split screen and with the mirror | yes, slightly | fix? | CONFIRMED |
| 16 | Moscow DMD (DISH) | level data | One bonus-spot carrier's hit box gives id 440, which `pickupApply` ignores | yes, slightly | fix? | LIKELY |
| 17 | 8010EE64 | weaponSlamStateAlloc | Hands out 5 claw records from a 4-record table; the 5th overlays `weaponRicochetPool`'s first object pointers | no (one Mr. Slam per match) | check | CONFIRMED code, LIKELY unreachable in retail |
| 18 | 800D5CAC (in 800D5AF0) | laEventsUpdate | Los Angeles quake: lane 1 has no object on the disc, so the lane's end writes a 0 byte to address 0x12 | no | **check** | CONFIRMED (code and disc data) |
| 19 | 80119C50 / 8011D514 | arenaSetup / arenaIsHolland | `arenaHollandFlag` is cleared right after `arenaLoadHolland` sets it: `arenaIsHolland()` is always 0 and the AI's Holland rule never runs | no (the AI as shipped) | harmless | CONFIRMED |
| 20 | 800EA328 (in 800EA270) | aiFreeSteer | "Hammerhead always rams": the store of the AI ram mode for Hammerhead (or `f458`) is always overwritten by the next if/else | no (the AI as shipped) | fix? | CONFIRMED |

Cross-references to bugs written up elsewhere are at the end. Candidates
that turned out not to be bugs are listed after that.

## 1. soundInit writes the master volume to address 0

**Where:** 80156D08 `soundInit` (`engine_math_near_pass.c`), calling
80156E08 `SsGetMVol` (`sdk_libsnd_leftovers.c`).

**Code.** At 80156DB0 `soundInit` loads `D_80180E48` (soundMVolResultPtr) (`lw a0,1140(gp)`) and
passes it to `SsGetMVol`. `SsGetMVol` reads the SPU master volume
(`SpuGetCommonAttr`, 8015D7F8), divides each side by 129 and stores the two
halfwords at `a0+0` and `a0+2` (80156E58, 80156E94).

`D_80180E48` (soundMVolResultPtr) is 0 in the EXE image, and this load is the only reference to
it in the whole executable. Nothing ever sets it. So the call writes two
halfwords to address 0x00000000..0x00000003. CONFIRMED.

**In game.** On PS1, address 0 is the start of main RAM (kernel area). The
first words are not used by anything at this point, so nothing happens.
Runs once at boot (`shellInitMachine`).

**PC port: check.** If the port keeps guest RAM in one array, the write
lands at RAM offset 0 and is harmless, as on PS1. If guest pointers ever
become host pointers here, it is a NULL write and crashes. Simplest fix:
skip the `SsGetMVol` call, or point it at a dummy 4-byte variable. The
result is never read.

## 2. LA quake events: end timer in a signed byte

**Where:** 800D5AF0 `laEventsUpdate` (`car_destroyed.c`), level 1 (Los
Angeles). The two events are started by remote bombs
(`laEventsTrigger`).

**Code.** When both debris lanes of an event are done, the event sets its
end time once and retires when the timer reaches it (800D5ECC..800D5F10):

```
if (p[2] == 0) p[2] = timer + 20;     /* sb: stored in a byte */
if (timer >= (s8)p[2]) p[1] = 0;      /* sll/sra 24: read back signed */
```

The timer is a s16 frame counter. CONFIRMED.

**Effect.** The end time only works while `timer + 20 <= 127`, so while the
lanes finish by frame 107. `laEventStart` (800D58BC) gives each lane a start
of 20..83 and a length of 0..63, so lanes can finish as late as frame 146.

| Lanes finish at | `p[2]` read back | Result |
|---|---|---|
| frame <= 107 | 40..127 | the event stays 20 more frames |
| frame 108..235 | negative | the event ends the same frame |
| `timer + 20` a multiple of 256 | 0 | recomputed next frame |

During those 20 frames the "both lanes done" block runs every frame (it
does not check the done flag `p[4]`). It spawns two big explosions, a screen
flash (`fxSheetStart`) and sets object step 0xFA5 each frame. So an early
event ends with about 21 frames of explosions; a late one with a single
frame. LIKELY visible as a shorter finale on some quakes.

**PC port: fix?** Same on PC. A fix stores the end time as s16.

## 3. Moscow dish: the same signed byte, dead here

**Where:** 800D6188 `moscowDishUpdate` (`car_destroyed.c`), level 2.

**Code.** The same pattern at 800D6584..800D65C8: `p[2] = timer + 20` into a
byte, compared as s8. CONFIRMED.

**Effect: none.** The block just before it (800D6504..800D6570) runs on the
same frame as soon as `p[5]` is set and `p[4]` is 0. It sets `p[4] = 1` and
`p[0] = 0`, so the event is already retired. `p[4]` is cleared only by
`moscowDishStartDestroy` (800D5FDC). So the byte timer never decides
anything here, even though the dish burst starts at frame 100..163
(800D604C) and would wrap.

**PC port: harmless.**

## 4. Volume setters: the minimum clamp overwrites the maximum clamp

**Where:** 800D35D0 `audioSetSfxVolume` (`game_misc_accessors.c`); the
same inline in 80136FF8 `menuVolumeApply` (music and effects,
`menu_volume_action.cpp`), 80128DB4 `roundStartMusic`
(`game_mode_audio.cpp`), 80128F64 `audioMuteMusic` (`arena_setup.c`) and
other screens that set `audioCdVolume`.

**Code (800D35D0).**

```
audioSfxVolume = min(v, audioVolumeClampMax);   /* sw at 800D35F8 */
audioSfxVolume = max(v, audioVolumeClampMin);   /* sw at 800D3614, overwrites */
```

The second store uses `v`, not the first result. So only the minimum
applies. CONFIRMED (both stores in 800D35D0 and in 80137084 / 801370AC).

**Effect: none.** Both clamps are 0..127 (`soundInit`), and every caller
already caps the value at 127 (`audioSfxVolumeCap` /
`audioMusicVolumeCap`, slider x7 / x8 at most 127).

**PC port: harmless.** Only matters if a mod feeds a larger value.

## 5. weaponDropWrite: default case damages a mine pointer

**Where:** 801096F4 `weaponDropWrite` (`weapon_slot_writer.c`).

**Code.** Commands 101 (mine) and 3 (remote bomb) are handled. Anything else
does `sh zero,0x4AF0(0x80180000)` (80109A5C) and returns 0. 80184AF0 is
entry 18 of `D_80184AA8[23]` (weaponMineObjects), the mine object pointers (asset 0x8A4). So the
store clears the low 16 bits of mine object 18's pointer. CONFIRMED.

**Reachable?** The only caller is 80109518 `weaponDropItem`, called from
80104680 (weapon 3 case, passes the selected weapon, which is 3) and 801048D0
(passes 0x65 = 101). So the default case never runs. LIKELY dead code.

**PC port: harmless** while unreachable. If a mod adds new drop commands,
remove that store, or mine 18 gets a broken pointer.

## 6. func_80112AB0 (explodeStoreLightningAnimation): 30 records into a 10-record bank

**Where:** 80112AB0 `func_80112AB0` (explodeStoreLightningAnimation) (`font_color.c`), the loader for world
object 658 (`world_load_object.c`). The bank is `fxFontUvStorage`
(80187194, BSS, `effect_font_state.c`).

**Code.** `grutilsParseSpriteRects(source, 0x80187194, 30)` (80112AC4,
`li a2,0x1e`). The parser (8014E008) writes 8 bytes per record and caps the
count at the record count in the asset (+16). CONFIRMED.

The bank holds 10 effect records (80 bytes). Right after them come the
seven font draw modes (801871E4) and the seven font sprite templates
(80187238), set up by `fontInit` (80113F30). The 10 records are what the
lightning effect spawner 80112600 uses (10 frames).

**Effect.** If the object 658 asset has more than 10 sprite rects, records
11+ overwrite the font draw modes, then the font templates. `fontInit` runs
at the start of `sessionRun` (8012BEC8), before the level loads, so the damage
would last the whole match. Retail fonts look right, so the asset most
likely has 10 records or fewer. The asset was not checked. Effect unknown.

**PC port: check.** If the port moves `fxFontUvStorage` or the font
templates apart, an overflow (if any) lands somewhere else. Cap the count
at 10 to be safe.

## 7. Lightning damage uses a stale owner

**Where:** 80107B00 `weaponLightningScan` (`weapon_slot_scan.c`), damage at
80108B48 / 80108B98 in 801084D8 `weaponArcsDraw` (`weapon_arcs_draw.c`).

**Code.** The lightning fills arc slots of `D_801847B4[5]` (`weaponArcs`):
+4, +6, +8, +12, +16. It never writes the +2 owner. Only the Outlaw 2 taser
writes +2 (80107A00, 80107A6C). The damage code reads +2 (`lh 0x47B6`) as
the attacker. So the attacker is the last taser user, or 0. CONFIRMED. See
`docs/GAME_EVENTS.md` 9.3.

**Effect.** The game has no kill counter and no score, so no player sees
it. The wrong attacker only changes the damage rules inside
`carApplyDamage` (for example AI-versus-AI damage /4). LIKELY minor.

**PC port: harmless**, but any feat or kill credit must take the caster from
the call at 80104558, not from +2 (`docs/GAME_EVENTS.md` 9.3).

## 8. aiSlowTargetRouteAllowed: discarded difficulty test

**Where:** 800E9A40 `aiSlowTargetRouteAllowed` (C name `sub_800E9A40` (aiSlowTargetRouteAllowed),
`ai_steps.c`), called by 800E9950 `aiTryRouteToSlowTarget`.

**Code.** It calls `aiGetDifficulty` (8011D5F4). If the result is <= 0 it
also loads byte +55 of the AI record. Then it drops both and returns
`gameGetLevel() == 5 && ai[+56] != 0`. CONFIRMED (800E9A4C..800E9A7C).

It looks like a lost condition, such as `(difficulty > 0 || ai[+55]) &&
...`. As written, difficulty and +55 do not matter.

**Effect.** The "route to a slow target" behaviour on New York (level 5)
does not depend on difficulty. Not visible as a bug.

**PC port: harmless.**

## 9. aiSectionEdgeDistance: wrong compare on the far z edge

**Where:** 800F0340 `aiSectionEdgeDistance` (`ai_car_start.c`), called only
from 800F02D0 (levels 5 and 6, test `< 480`).

**Code.** It returns the distance to the nearest edge of the car's section.
For x it is right. For z, when the car is in the far half:

```
if (dz < r) r = width - dz;     /* should test (width - dz) < r */
```

CONFIRMED (800F042C..800F0444).

**Effect.** Near the far z edge the z distance is usually ignored, so the
result is too large and the caller's "near an edge" test (< 480) fails more
often. Small AI difference on New York and Antarctica. Not visible as a bug.

**PC port: harmless.**

## 10. carWheelsUpdate: wheel 0 uses wheel 1's height

**Where:** 800EC994 `carWheelsUpdate` (`car_wheels.c`).

**Code.** When choosing the surface under the car, each wheel is tested with
`!touch || w[i].h + 10 < w[i].pos.z`. For wheel 0 retail reads wheel 1's `h`
(800ECBC8 `lhu 132(s1)` = w[1].h, against 800ECBCC `lw 92(s1)` = w[0].z).
Wheels 1..3 use their own. CONFIRMED.

**Effect.** A tiny difference in which surface the car counts as under it.
Not visible.

**PC port: harmless.** Keep it, or physics drift from retail.

## 11. Hong Kong track vehicles: a range test that never fires

**Where:** 80117B40 `worldTrackVehiclesUpdate` (`track_vehicles.c`), level 8.

**Code.** While the vehicles are stopped (`trackVehiclesShouldStop`), a
vehicle in the window 3201..6400 snaps to 6400 or 3200. The mirror test for
the negative side is `x >= -6400 && x < -6400` (80117BE4..80117C04: `slti
-6400` twice). It is never true, so a vehicle on that side is never snapped to
-3200 / -6400. CONFIRMED.

**Effect.** A vehicle stopping on the negative-x side is shown where it is,
not at the station stop. LIKELY a small visual offset, if it happens at all.

**PC port: fix?** The intended test is probably `x >= -6400 && x < -3200`.

## 12. Debris sprites use view 0's projection

**Where:** 8014C19C (debris sprites, `engine_fileio_debris.c`).

**Code.** Sprite size comes from `801555C4(0)` (8014CD30 and 8014CD94: `move
a0,zero`), so view 0's H is used in view 1 too. CONFIRMED. See
`docs/WIDESCREEN.md`.

**Effect.** In split screen both views have the same H, so none.

**PC port: harmless**, unless the port gives the two views different
projections (widescreen per view).

## 13. Matrix-slot index can reach 200

**Where:** 80152284 `renderScene` and the asm model walker. Details in
`docs/RENDER_ANSWERS.md` R2.1.

**Code.** The matrix-slot counter (ctx+0x08) indexes several 200-entry
arrays. The walker checks `count < 198` (80152C18). `renderScene` checks
`count < 200` and then counts up (80152354, 801523E8, 801524BC). So the
index can reach 200, one past the arrays. The walker then writes 32 bytes
into `renderMatrixSlots[200]` (= `worldSurfaces` 801B0418), and `csUpdMat`
writes into `renderObjectMatrices[200]` (= 801FA3B8, `timerTicsAccum`).
CONFIRMED code.

**In game.** It needs 199 or more slots in one view. Not seen. LIKELY a
latent overflow.

**PC port: check.** A port that enlarges the arrays should make all of them
at least cap + 1 and raise the four checks together. Done by the port
variant's `TM2_PORT_RENDER_SLOTS=N` (request 47, `FAR_DISTANCE.md` 3 "Matrix
slots"): the five arrays get N + 1 slots, so the last slot `renderScene` hands
out is inside them; without the define the retail overflow stays.

## 14. Near pass: unsigned screen-position midpoints

**Where:** 80154690 `renderNearPolys` (asm). Details in
`docs/RENDER_ANSWERS.md` R1.2.

**Code.** The near pass splits each polygon into 4 by averaging the packed
screen words. Each of x and y is averaged as an unsigned 16-bit value
(`srl 16` / `andi 0xFFFF`, add, `sra 1`). Triangle 80154764..801547CC,
quad 801549B0..80154A80. CONFIRMED.

**In game.** The result is right only modulo 2048. For example x = -1 and
x = 1 give 0x8000. The PS1 GPU keeps only bits 0..10 and sign-extends them,
so 0x8000 becomes 0. Nothing shows on PS1.

**PC port: check.** A GPU path that reads x/y as full s16 gets ±32768
errors on near polygons that cross the left or top edge. Sign-extend bits
0..10, or average signed values in a C replacement.

## 15. streaksDraw ticks twice per step in split screen and mirror mode

**Where:** 800D8230 `streaksDraw` (`screen_card_draw.c`). Details in
`docs/RENDER_ANSWERS.md` R3 (d).

**Code.** `streaksDraw` also does update work: `fxSheetDisplay` counts down
`fxSheetTimer`, `weaponDropsUpdate` ticks fuses and drops,
`weaponFlameUpdate` deals damage, and `weaponArcsDraw` ticks the tasers and
calls `carApplyDamage`. The main loop calls `streaksDraw` once per view.
With split screen or the rear-view mirror it runs twice per step, and the
callees ignore the view argument. Only `fxSheetStart` compensates, and only
for split screen. CONFIRMED.

**In game.** In those modes tasers, fuses and flames run twice as fast.
With the mirror the flash is also half as long.

**PC port: fix?** To keep retail timing, run one tick per retail
`streaksDraw` call. A port may choose to run them once per step instead.

## 16. Moscow: a bonus-spot hit box gives id 440

**Where:** Moscow level data (`DMD\DISH.DMD`). Details in
`docs/PICKUP_LOOKS.md` section 1.

**Data.** The rotating bonus spots (ids 441..480) hang under an animation
node with id 440. On Moscow's first 440 carrier the hit box is beside the
44x node, not under it. Touching it gives id 440. `pickupApply` ignores 440
(jingle only). LIKELY a retail data bug.

**PC port: fix?** The port could move the hit box under the 44x node, or
map 440 to the current frame's pickup.

## 17. weaponSlamStateAlloc: 5 claw records in a 4-record table

**Where:** 8010EE64 `weaponSlamStateAlloc` (`weapon_tables.c`), called by
`weaponSlamStateInit` (8010EEA8) for Mr. Slam's weapon block (`ai_car_start.c`,
`weaponBlockInit`, `car_collision.c`, `car_restart.c`, `weapon_basic.c`, all for
type 0xC0).

**Code.** The record is computed from the cursor before it moves
(`weaponSlamRecords + i * 18`), then `i < 4 ? i + 1 : 0`: cursors 0..4, five
records, but `weaponSlamRecords` (80184818) holds four (72 bytes). The fifth is
80184860 = `weaponRicochetPool.slots[0..4]`, and `weaponSlamStateInit` writes bytes
0..3, 7, 11 and the halfword at +0x10 there: the first ricochet object pointers
get small values. CONFIRMED (code).

**When.** A weapon block keeps its record (`weaponSlamStateInit` allocates only
while `weapons.slam` is 0) and `carTablesClear` resets the cursor every load, so
retail, with one Mr. Slam per match, takes one record per load: LIKELY never
reached. Five or more Mr. Slams in one load (the port's duplicates, request 32)
reach it.

**PC port: check.** The port variant with `TM2_PORT_CARS` and `portCarOptions`
bit 3 gives the claw records their own table of 17 (`CAR_GROUPS.md` 7); with
bit 3 clear (or `TM2_PORT_CAR_GROUPS` alone) the overflow stays as retail's.

## 18. Los Angeles quake: a byte written to address 0x12

**Where:** 800D5AF0 `laEventsUpdate` (`car_destroyed.c`), the store `sb zero,18(v0)` at **800D5CAC**, level 1 (Los
Angeles). Found for PC port request 38 (`docs/ARENA_SWITCHES.md` 7).

**Code.** Each LA event (196-byte record, `laEventGet`) has two lanes; at a lane's start time the update calls
`debrisBurstAt(lane object, ...)` and then clears byte +18 of the lane object (`U8(PTR(p, 36 + j * 4), 18) = 0`).
The lane objects are registered at load by `laEventRegisterObject` (800D54C0) from the DMD nodes 4006..4009, at
the index in the node's +16.

**Data.** `DMD\FREEWAY.DMD` has one event (4005, event 1, the quake) and one lane node: 4006 with index **2**.
Lane 1's object pointer stays 0 (and its anchor (0, 0, 0)). `debrisBurstAt(0, ...)` does nothing, but the store
writes 0 to address 0x00000012 every time the quake runs (once per arena load, when a remote bomb sets it off).
CONFIRMED (code and disc data).

**In game.** Nothing seen: 0x12 is in the kernel's low RAM.

**PC port: check.** With `--port-define TM2_PORT_HAZARDS` (request 38) the store is skipped for a null lane object
only (every other case writes as retail; the switches need not be set). Without the define the port maps the
write as retail does.

## 19. arenaIsHolland is always 0

**Where:** 80119EFC `arenaLoadHolland` (`view_watch_ai.c`) sets `arenaHollandFlag` (80180F9C) to 1 (store at
80119F38); 8011995C `arenaSetup` (`arena_setup.c`) clears it at **80119C50**, right after the arena switch that
called the loader (and `uaInit` clears it at 801196C8). There is no other writer. The only reader is
8011D514 `arenaIsHolland`, called by 800EBB74 `aiShouldChaseDirect` (`ai_steps.c`). Found for request 38.

**Effect.** The flag is never 1 while anything reads it, so `aiShouldChaseDirect`'s Holland branch (chase any car on
the same node within 3200, ignoring height past distance 600) never runs; Holland's AI uses the rule of every
other arena. CONFIRMED.

**PC port: harmless.** Retail AI as shipped. "Fixing" it would change Holland's AI.

## 20. aiFreeSteer: "Hammerhead always rams" is a dead store

**Where:** 800EA270 `aiFreeSteer` (`ai_steps.c`), the store `sb v0,60(s1)` at **800EA328**. Found for PC port
request 39 (`docs/RAM_BALANCE.md` 8, now `docs/BALANCE.md` 9).

**Code.**

```
if (ai->type == 0x20 || ai->f458)            /* Hammerhead (TM2_SPECIAL_TYPE with request 35) */
    ai->f03C = 1;
if (!(ai->f576 < tw) && (rand() & 3) == 1)   /* tw = the target's mass */
    ai->f03C = 1;
else
    ai->f03C = 0;
```

The second statement always writes `f03C` (the AI's ram mode: `aiFollowSpeed` uses its own top speed and the chase
distance is the full distance), so the first store has no effect: the ram mode comes from the mass test and
`rand()` only, for Hammerhead too. An `else` was probably meant. CONFIRMED (the matched C keeps the store, as
retail's compiler did).

**In game.** An AI Hammerhead rams only as often as any other car of his weight; he crushes less than intended.

**PC port: fix?** Left retail, also under `TM2_PORT_BALANCE` (request 40, which absorbed request 39). A fix (`else
if`) makes AI Hammerhead (and `f458` cars) always ram, more free AI crushes, and skips that `rand()` for them (the
stream changes); `BALANCE.md` 9 offers it as a future bit of `portBalance`.

## 21. Twister's tornado never throws a caught car out

**Where:** 8010E040 `weaponTornadoUpdate` (`weapon_tornado.c`), both loops (humans, then computer cars): `if
(freeze_timer == 1) carSpinOutById(type, heading away from her, 0)`. Found for PC port request 40 (`BALANCE.md`
3.7).

**Code.** A catch sets `freeze_timer = 60`, and the same frame's `weaponApplyHit(..., 4, ...)` -> `carApplyDamage`
cuts a frozen car's freeze by 6 x the final damage (24 for 4; 6 for AI vs AI's 1). Each car tick subtracts
`timerGetFrameTicks()` (3 at the game's 20 frames a second; `car_tick.c`, `ai_driver.c`), so the timer takes the values
36, 33, ..., 3, 0: never 1. The tornado update runs after the car ticks, so it sees only multiples of 3. CONFIRMED in
the C (not run).

**In game.** The throw (a spin-out along the heading away from Twister) never happens at 20 or 30 frames a second;
the car thaws, drives off a little, and is caught again while it is still within 280 units: up to 7 catches (28
damage) per tornado (`CAR_STATS.md` 6). It would happen only at 1 tick a frame (60 fps logic). With
`TM2_PORT_CAR_GROUPS` the throw takes the caught car's key; without it, the car type: with two cars of one type
(not reachable in retail) it would throw the wrong one.

**PC port: fix?** `TM2_PORT_BALANCE` bit 12 (the user's option c, `BALANCE.md` 3.7): the catching stays retail's
(the dead tests too), and when the tornado ends every car still in her catch box gets the throw (`carSpinOutById`
along the heading away from her, as the dead test meant). A 1-line test (`freeze_timer <= ticks`) alone would barely
throw (the car is caught again the next frame).

## 22. Mines past the 23rd: a record without an object (stale or null pointer)

**Where:** 801096F4 `weaponDropWrite` (`weapon_slot_writer.c`), case 101 (a mine). Found for PC port request 40
(`BALANCE.md` 3.3).

**Code.** The mine takes a record from `weaponDropSlotFind` (25 records, shared with remote bombs) and then an object
from `weaponMineObjectFind` (23 objects). With a free record but no free object it still opens the record
(`status = 300`, `kind = 20`) and leaves its object pointer `+0x18` as it was: 0 for a record never used, or the
object of the record's last mine, which may now belong to another live mine. CONFIRMED in the C.

**In game.** The 24th and 25th live mines are invisible but hit cars as usual (the hit test uses the record's
position). While the mine sinks to the ground `weaponDropsUpdate` writes its height each frame into that pointer's object (`+0xB0`):
through a null pointer into low RAM, or into another mine's object (moving it); when the mine expires or is hit
it hides that object, so another live mine can vanish while it still hits.

**PC port: fixed under `TM2_PORT_BALANCE` bit 2** (request 40): a mine record is opened only when a mine object is
free (`portBalanceMineSlot`); otherwise the mine is refused and its 120 energy kept. With the word zero the retail
behaviour stays (the word-zero rule: retail logic and `rand()` order).

## 23. A shield cast while shielded costs 400 energy for nothing

**Where:** 801046FC `weaponFireEnergyAttack` (`weapon_special.c`), energy attack 103 (the shield). Found for PC port
request 40 (`BALANCE.md` 3.2).

**Code.** With 400 energy the cast always takes 400; it makes a shield only when `energy_shield` is 0 and
`weaponShieldSlotFind` finds a free record (4 in retail). CONFIRMED in the C.

**In game.** Casting again while the shield is up, or while 4 shields are up, wastes the 400 with no effect and no
denied sound. A design flaw rather than a crash.

**PC port:** `TM2_PORT_BALANCE` bit 1 refuses both casts (denied sound, no cost).

## 24. A computer car's shield would never clear (latent)

**Where:** 8010DD30 `weaponShieldsUpdate` (`fx_sprite_spawn.c`). Found for PC port request 40.

**Code.** At the end of a shield it clears `humanCarGet(record.owner)->state.energy_shield`; the record's owner is
written only for a human caster (`weaponFireEnergyAttack` 103). A computer car's shield would clear human 0's (or the
stale owner's) flag and keep its own set for good. CONFIRMED in the C.

**In game.** Not reachable: the AI's energy attacks are 0x64, 0x65 and 0x6B (`weapon_special_missiles.c`), never the
shield. Left retail.

## 25. weaponDropWrite's default case writes before the table (latent)

**Where:** 801096F4 `weaponDropWrite`, the default case: `weaponDrops[-1].status = 0`, a halfword at **80184AF0**,
which is the low half of `weaponMineObjects[18]`. Found for PC port request 40.

**In game.** Not reachable: the callers pass only kinds 3 and 101. If it were, it would corrupt mine object pointer
18. **PC port:** when `TM2_PORT_BALANCE` with `TM2_PORT_CARS` moves the drop table to port storage, the store would
land before the moved table, so it is left out there (`#if TM2_MAX_DROPS <= TM2_RETAIL_DROPS`); otherwise retail's.

## 26. Missiles can fly on under the ground (holes in the level hit volumes)

**Where:** 800DB110 `weaponProjectileUpdate` (`projectile_hit.c`) and the level DMDs' hit volumes. Found for the
PC port's request 40 follow-up (`BALANCE.md` 3.11).

**Code.** A missile (every type `weaponProjectileUpdate` runs, Sweet Tooth's spiral included) has no ground test of
its own: it dies on a car / breakable / spinner (`projectileHitTest`), on the level's hit volumes (`hdpQueryPoint`,
once a frame at the step's midpoint, tags 3301, 2, 667, 668, 401..440, 3500..3509 ignored), or when its life ends.
The ground the cars drive on (`worldSectionAt` / `worldGroundHeightAt`) is a separate data set. CONFIRMED in the C.

**Data.** An offline copy of the hit query over the 14 level DMDs (files in their load state), at the AI route points
(`TERRAIN\<track>.PTS`, on the drivable ground): below most route points a non-ignored volume starts within 8 units
and is hundreds of units thick, but Hong Kong (26 of 140 points), Paris (9 of 118), Antarctica 2P (12 of 79), Roof
Tops (3 of 9) and Moscow (1 of 33) have points with no volume below them down to z - 6000 or with a "consume" box
that ends the query without a hit, and at 3-15 % of the points elsewhere the volumes' top is 16-300 units below the
road. CONFIRMED in the files (route points as the ground; run-time switches of volumes not modelled).

**In game.** LIKELY: a missile that dips under the road there keeps flying under it until it climbs out or its life
ends; Sweet Tooth's spiral (its pitch set every frame to a wave of about 25 degrees, its lowest points about 90 below
its launch height, so about 15 above a flat road) is the one that dips most.

**PC port: fix?** No: left retail, documentation only. A first follow-up of request 40 made the spiral die below the
drivable ground; it was removed, because the user's worry is spirals dying right after launch, which a ground death
can only add to (on a slope the probed surface can lie just above the spiral). `TM2_PORT_BALANCE` bit 9 is reserved
with no effect; `BALANCE.md` 3.11 has the early-death analysis (the same thin margin over the road: where these
volumes *are* at the road, the wave's first trough meets any rise ahead).

## 27. VRAM scans test only the top-left texel of a spot

**Where:** 80120D3C `vramUploadClut` (`scene_init.c`) and 801209E8 `vramPlaceTexture`
(`screen_deathmatch_result.c`): `if (bitmapIsFreeXY(x, y)) { bitmapMarkRect(x, y, w, h); ... }`. Found for PC port
request 33-1 (`CAR_GROUPS.md` 13.3, PR #38's question 1).

**Code.** Both scans step x by the width (and y by the height for images) and take the first spot whose **top-left**
texel is free in the usage bitmap, then mark and upload the whole w x h rectangle. A spot whose first texel is free but
whose others are not is taken anyway. CONFIRMED in the C.

**In game.** CLUTs: car 256-colour CLUTs fill the free rows at x 768 of the upper half first; once those are gone the
scan moves to the lower half (the image area), where a row of 256 can start on a free texel and run over the font
area (x 960..) or car pixels. The host model shows it from the 9th different type on Amazonia, Holland and Cyburbia
(the largest types first), the 11th on Moscow and Roof Tops, the 12th with the smallest first (`SIXTEEN_CARS.md` 4.4),
and with Dark Tooth after Mr. Grimm and Twister on Holland and Cyburbia: a retail game with 9 or more different large
cars there would hit it, the port's 12-14 types do. LIKELY (model of the disc files, `tools/check_car_groups.py
--clut-model`, not run). Images: the same model shows a car image whose
rectangle overlaps an earlier one on most arenas, on Suicide Swamp already with Mr. Grimm and Twister (Twister's image 6,
8 x 64 at (616, 448)), so some retail loads may lose texels of an earlier image: LIKELY, not seen running.

**PC port: fixed under a bit.** CLUTs: `portCarOptions` bit 9 with `TM2_PORT_CAR_CLUTS` (`CAR_GROUPS.md` 13.12):
`vramUploadClut` tests every texel of the spot (`portClutSpotFree`) and, when neither half has one, takes a spot in
rows 240..255 (`portClutRowsLast`). Images (request 41 item B): the same bit makes `vramPlaceTexture` test the whole
rectangle in two passes (`portImageSpotFree`), retail's two passes as the fallback (`portImagePasses`). Bit 9 clear:
retail. Rerun of the model with images counted (`SIXTEEN_CARS.md` 4.4 "img"): the first image over used texels comes
with the 2nd type on Swamp, the 3rd on Hong Kong, the 5th on Amazonia, New York 2P and Roof Tops (largest first).

## 28. A ricochet bomb hits its own fast car at the launch

**Where:** 8010CC54 `weaponRicochetUpdate` (`weapon_ricochet.c`), the two car loops: `(lookup->id != SHOT.owner ||
SHOT.life < 398)` (a human's shot) / `< 396` (an AI's). Found for PC port request 41 (item 9, the user saw it).

**Code (CONFIRMED).** The shot starts at the car's origin (+40 z, `weaponFireSelected` case 6) and rolls 100 units a
frame; its own box test (`DISTANCE < 20`, 160 units) spares the owner only for 2 updates (human) / 4 (AI). After k
updates the gap is `100 (k + 1) - k v` for a car at v units a frame: at the first owner test it is below 160 for v > 70
on an axis-aligned heading, v > 37 at 45 degrees (a top speed of 92-141 is about 52-80 a frame, turbo up to 103), so a
fast car blows itself up 0.15 s after the shot. The other projectiles skip their owner always (`projectileHitTest`).

**PC port: fixed under a bit.** `TM2_PORT_BALANCE` bit 18 (`BALANCE.md` 12.6): the owner can be hit only once the
shot has been outside his box once (the record's unused byte +0x01).

## 29. Twister's special pressed during her tornado spends a charge for nothing

**Where:** 80105A7C `weaponFireSpecial` case 0xA0 (`weapon_basic.c`) and 80103FB8 `weaponFireSelected`'s tail
(`weapon_select.c`). Found for PC port request 41 (item 4).

**Code (CONFIRMED).** Case 0xA0 starts a tornado only `if (weaponTornadoState.timer <= 0)`; while hers runs it does
nothing. `weaponFireSelected` takes the charge after any dispatch (`weap->ammo[kind]--`), so a second press during the
tornado (about 4.2 s) loses a special. Computer cars are refilled every tick (no effect).

**PC port: fixed under a bit.** `TM2_PORT_BALANCE` bit 15 (`BALANCE.md` 12.3) refuses every weapon during her
tornado, her special included, with no cost.

## 30. A bullet hitting an arena actor passes the last probed surface's target to weaponApplyHit

**Where:** 800D7E30 `bulletsMove` (`weapon_bullets.c`), its hit block. Found for the balance round 3 (`BALANCE.md`
13.3).

**Code (CONFIRMED).** The level object pointer `obj` is set to 0 once per call, then by every bullet's level probe
(`hdpQueryPoint`, at sub-step 2 when the box test found nothing), and never cleared per bullet. A bullet whose hit
comes from the box test (`projectileHitTest`) still passes `S16(obj, 10)` / `S16(obj, 12)` (the kind and instance of
whatever surface an earlier bullet probed this frame) to `weaponApplyHit`. For a car (1..99) or a target (500..5999,
whose kind and instance are re-read from the target table) they are ignored; for an arena actor (6000 + n:
`targetActorHit`) the function goes on to `switch (a)` with the stale kind, so if that surface was a target's (500-510,
519, 520) and the shooter is a human, `targetDamage` hits that target too (3005 / 3006: the Moscow dish).

**Effect (LIKELY).** Rare and small: an actor hit in the same frame after a bullet probed a breakable's surface
damages that breakable again by the bullet's power. Not fixed. With `TM2_PORT_BALANCE` bit 20 every connecting
bullet is applied (no coin flip), so it would happen about twice as often as in retail.

## 31. A ricochet bomb bounces off its own car

**Where:** 8010CC54 `weaponRicochetUpdate` (`weapon_ricochet.c`), its hit pass `hdpCollideObject` -> `hdpPassRun`
(8014FAB0, `engine_hit_detection.c`). Found for PC port request 45 (the user's playtest of 2026-10-17, item 6).

**Code (CONFIRMED).** The shot's hit pass tests the level and then every scene object whose words +8 and +0xC are
set, skipping only the shot itself (8014FC50..8014FC70): its owner's car is tested like any other. A car model's hit
box reports the car type (16..240) as its kind, which the ricochet's filter (667-668, 401-440) does not drop, so the
owner's box is a wall: the heading is reflected, the power doubled and the object's light flicker started (the first
bounce), the shot pushed out by `depth + 20`. The shot's one hit point is 40 units above it (`PROJECTILE(.., 40)`,
`asset_pools.c`).

**Effect (LIKELY: the box frame and the heights, `BALANCE.md` 14.2).** A Minion's box reaches 126 units ahead and 84
up: his shot (100 ahead after its first update, its point 65 up) bounces off him at once: it starts red-flickering
with double power (14). A car faster than its shot (turbo) whose box is tall enough (Axel, Sweet Tooth, Hammerhead,
Mr. Slam, Mr. Grimm) would bounce it too, but in retail the shot explodes on him at its third update first (28), so
that is not seen.

**PC port: fixed under a bit.** `TM2_PORT_BALANCE` bit 18 (`BALANCE.md` 14.2): until the shot has once been outside
its owner's box, a hit on his car is dropped (`portBalanceRicochetPass`): no bounce, no flicker, no push.

## 32. A car's shadow drawn over its own lower body

**Where:** the car files' shadow meshes (object `type + 3`, node 3: `BALANCE.md` 13.5) and the model emitter
800CDC54 (`reference/model_emitter_800CDC54.c:292-300`). Found for PC port request 46 item 5 (the user's playtest of
2026-10-18: Sweet Tooth's clown face half dark in Survival).

**Data and code (CONFIRMED).** Every car's shadow blob is a flat semi-transparent mesh whose polygons carry a
**negative OT bias** in their control words (bits 7..12: -2..-7 entries; Sweet Tooth -5 and, on its rear strip, -7;
the bodies 0), applied below 1,024 units; the emitter sorts each polygon by its deepest vertex plus that bias. The
shadow is a world model drawn by its own sort, not tied to its car's.

**Effect (LIKELY: the geometry, not seen running).** A shadow strip whose deepest corner lies under the car, pulled 5..7
entries forward, sorts in front of the car's low rear (or side) panels whose deepest vertex is a little farther (tall
panels: Sweet Tooth's rear), so the semi-transparent blob is drawn over them where it projects: the lower half of the
panel darkened. The bias exists for the road under the car.

**PC port: fixed under a bit.** `TM2_PORT_VISUAL` bit 3 (`PLAYTEST_ANSWERS.md` 12): each shadow is sorted one entry
behind its own car's farthest polygon.

## 33. A machine-gun bullet's first streak starts a car step behind the muzzle

**Where:** `rtMainLoop` (800D1318, `rt_main.c`): view 0's `streaksDraw` (800D8230), `bulletsUpdate` -> `bulletsMove`
(800D7E30), then `worldUpdate` (the cars' step, `UpdateGuns` 80104F70 spawning at this step's muzzle), then view 1 /
the mirror. Found for PC port request 46 item 4.

**Code (CONFIRMED).** A bullet spawned in step N with `pos = prev = muzzle(N)` is drawn first as a point, then (after
one `bulletsMove` and one more car step) from `muzzle(N) + vel` back to `muzzle(N)` while its shooter is drawn at
step N + 1: the streak's tail is one car step behind the muzzle (bullets get no share of the car's speed).

**Effect (LIKELY).** Seen on the fastest cars: Twister's streaks start 43..113 units behind her centre (80 units a step,
up to 150 for a hard computer car with turbo), at or behind her rear. A drawing matter only: the hit test is the
bullet's own.

**PC port: fixed under a bit.** `TM2_PORT_VISUAL` bit 4 (`PLAYTEST_ANSWERS.md` 11): the first streak's tail is moved
by the shooter's own move since the shot.

## 34. Cyburbia's plaza gates: twelve opaque triangles with colour 0 (drawn as black silhouettes)

**Where:** level data, `DMD\BURB.DMD` on the disc (Cyburbia, a secret level): the gate mesh at file offset +0x53A34
(pointer 8006D674 at the file's base 80019C40), drawn at the four gates in the wall of the central plaza (world
(1800, 997), (1402, 600), (2400, 202), (2797, 600)), in the near record [0, 450) of its LOD node 8006E1D8. Found for
PC port request 47 item 3.

**Data (CONFIRMED, read from the disc).** Of the mesh's 60 primitives, 12 are double-sided textured triangles (FT3)
with the GPU command 0x24 (opaque, modulated) and colour (0, 0, 0): records at file +0x53F98, +0x53FB8, +0x53FD8,
+0x54040, +0x54060, +0x54080, +0x541E4, +0x54228, +0x5426C, +0x5428C, +0x542D0, +0x542F0. Their texels (tpage 0x0010,
CLUT 0x08BD, v 135..144) are a ragged strip pattern: about 70% 0x0000 (transparent) and the rest with the STP bit set.
They stand 48..80 world units up, on the top of the gate. The same gate's two ground quads use colour 0 with the
semi-transparent command 0x2E (dark shadows), and the disc's other arenas have one opaque textured
primitive with colour 0 between them (Moscow; Cyburbia has one more, in a select child that is not current at load).
LIKELY the triangles were meant to be semi-transparent too.

**Effect (LIKELY, from the GPU rules).** An opaque command draws STP texels opaque, and the colour 0 modulates them
to black: ragged black shapes over the gates, seen whenever the camera is within 450 units of a gate's LOD point and
inside the city (the level's zone node: x -50..3650, y -150..1950, below 200 units). Retail shows them as well.

**PC port: fix?** A load-time patch of the twelve records (command 0x24 -> 0x26: half-dark like the shadows; or a
zero-area triangle) after Cyburbia's DMD is loaded. `PLAYTEST_ANSWERS.md` 21.

## 35. A human car starts at its record's previous ride height

**Where:** `humanCarPlaceAtStart` (800FD6C4, `car_object_contact.c`), from `carRestart` (round starts, respawns, the
port's drop-ins). Found for PC port request 49 (b) (`CAR_GROUPS.md` 7.10).

**Code (CONFIRMED).** `human->motion.pos[2] = human->stats.ride_height + node height` is computed before
`carStatsLoad(&human->stats, human->type)` sets the ride height of the record's car (32 .. 66 by type). So the start
height uses the ride height of whatever car the record held before: the same car on a later round, the previous
match's car (or the attract demo's) on a match's first start, 0 on a record never used since boot (BSS). `aiCarReset`
has the same order, but `aiPlaceOnRouteNode` then sets the AI car's height from the section floor: no effect there.

**Effect (LIKELY).** The first step starts the car up to 34 units too high or too low after another car (up to 66
too low from a record never used); the wheels then settle it;
nothing seen in play. It makes a round's first steps depend on the record's history, which matters for online fresh
starts without a state transfer (`ONLINE_ROLLBACK.md` 10.6).

**PC port: not fixed** (retail behaviour; the host's state at step 0 covers it online).

## 36. A ricochet bomb's object gets two bytes of stack garbage

**Where:** `weaponFireSelected` (80103FB8, `weapon_select.c`), weapon kind 6 (the ricochet bomb). Found for PC port
request 58 (`ONLINE_ROLLBACK.md` 19.2).

**Code (CONFIRMED).** The launch fills a local `short vec[4]` with x, 0 and the heading, never `vec[3]`, and copies all
8 bytes into the pooled object's rotation: `memcpy(slot->rot, vec, 8)`. The object's `rot` is `s16[3]` and two bytes
of padding (+0xA6), so the padding takes whatever the stack held at `sp+0x1E` (retail's frame stores `sp+0x18`,
`+0x1A` and `+0x1C` only). `csReset` clears `rot[0..2]`, not the padding.

**Effect (CONFIRMED, harmless).** Nothing reads +0xA6 (`RotMatrixYXZ` and the ricochet's update read the three
angles). The value depends on what ran before on this machine (FFFF on one machine, 0000 on another in the port's
online test), so a byte-wise state hash of the object differs between machines.

**PC port: not fixed** (no reader); the rollback table leaves the padding of every scene object out of the hash
(`csObjectPool.rotpad`).

## 37. The homing search's "no object" test never fires

**Where:** `weaponFindTargetAhead` (80109F94) and `weaponFindTargetBehind` (8010A2AC), `weapon_targets.c`, both passes.
Found for PC port request 59 (`ONLINE_ROLLBACK.md` 20.2).

**Code (CONFIRMED).** Each candidate is `cand = obj->pos` and the loop skips it when `cand == 0`. `pos` is the scene
object's embedded `s32 pos[3]` (+0xA8), so `cand` is the object's address plus 0xA8: it is never 0, and a null object
pointer (`aiCarObject(i)` / `humanCarObject(i)` returning 0) is not skipped. The search then reads a "car" at
0x000000A8..0x000000B3 (the kernel's A0 / B0 call stubs on the PS1) and, if it qualifies, stores the pointer 0xA8 as
the missile's target: the missile steers at whatever those 12 bytes hold.

**Effect (LIKELY, latent).** Retail binds an object to every human and AI record below its counts, so the test is
never needed in play (on the PS1 the stub words are huge coordinates and fail the cone tests anyway). A port path
that leaves a record below the count without an object (the drop-in's vacant records keep a parked dummy object
instead, for this reason) would aim at low RAM, which is not part of a rollback snapshot or hash and may differ
between machines.

**PC port: not fixed** (no retail path reaches it; the port keeps an object on every counted record).

## 38. Every projectile record comes back for 11 steps at each round start, with its old type and target

**Where:** `projectilesReset` (800D79C4, `ai_route_helpers.c`), called by `carRestart` (800FD550) at each bind, each
round start (`roundStart` → `humanCarsRestart`) and each respawn; `missilesCreateObjects` (800D90C0) at each load.
Found for PC port request 60 (`ONLINE_ROLLBACK.md` 21).

**Code (CONFIRMED).** `projectilesReset` sets every record's life to -1 and hides its object; `missilesCreateObjects`
gives each record a new scene object and life -15. Neither touches the type (+0x02), the target (+0x38), the victim
(+0x3C) or the owner (+0x42). Life -1 is the first step of the fade: `fxRecordsUpdate` (800DC348) runs
`weaponProjectileUpdate` (800DB110) for every record with life >= -11, which runs the type's code and steers whenever
the target is set. So every record that ever flew is updated for the first 11 steps of each later round with its old
type and target, even one that died long before (life -12 or below). After a level change the target points into
the re-allocated object pool (another object now). `fxRecordsClear` (800D9198), which would free every record, is
never called.

**Effect (CONFIRMED, mostly invisible).** The revived records do not move or hit anything (life <= 0); a seeker turns
its hidden object, whose pose the next fire overwrites. Visible: a Dark Tooth missile's record (type 61) puts two pink
sparks per step at its object for those 11 steps of every later round, and their `rand()` draws shift the round's
random stream. The steering reads whatever the stale target points at, which in the PC port's online play can be
each machine's own object (a camera): the machines part.

**PC port: fixed online** with `portRollbackOptions` bit 11 `ROUND_CLEAN` (`roundStart` clears every record first;
a seeker steers only at a counted car object). Retail builds keep the retail behaviour.

## 39. The chase camera can leave the arena, and the outer fence disappears from outside

**Where:** `viewChaseCamera` (80102CC4, `camera.c`); Moscow's level file `DMD\DISH.DMD` (and every arena with
one-sided outer walls). Found for PC port request 62 (`FAR_LOD.md` 9).

**Code and data (CONFIRMED).** The chase camera eases toward a goal behind the car and only keeps itself at or above
the car's height and 40 units above the car in car space; nothing tests it against the walls. Moscow's outer walls
are one-sided (back-face culled, facing the arena) and the level has no ground outside them.

**Effect (LIKELY; seen in the PC port at 60 FPS).** When a car spins against the outer fence the camera can stand
outside it for a few steps: the near fence is a back face, under the camera there is nothing, so the clear colour
shows; the far fence and the sky stay.

**Not Amazonia's tunnel (withdrawn 2026-10-25, `HIDDEN_AREAS.md` 10.2):** `carUpdateContacts` switches views 3 / 4
to view 2 whenever the car's front-left wheel is on class-1 (covered) ground, so the far view never reaches the tube;
the PC port's video first read as this (`HIDDEN_AREAS.md` 9.2) was a port bug (`HIDDEN_AREAS.md` 10). The earlier
reading, kept for the record: in view 4 (`cam_offset`:
1,200 behind and 800 above the car) the eye is above the tunnel's roof whenever the car is in it: the roof (facing
down) and the ground (facing up) are culled, so the main view shows the tunnel as an open trough, the clear colour
and the upper level from below. Views 2 and 3 can leave through a side wall when the car is turned across the tube.

**PC port:** retail behaviour. Since request 63 the port can keep the camera inside the level: `portVisual` bit 5
(`TM2_VIS_CAMERA_WALL`, `--port-define TM2_PORT_VISUAL`; the eye pulled in front of the first hit volume between the
look-at point and it; `FAR_LOD.md` 10). 0 = retail.

## 40. A car's pit death is measured from its body, so a tall car lives on a floor that kills the others

**Where:** `humanCarTick` (800DC88C, `car_tick.c`) and `aiCarDrive` (800E7EDC, `ai_driver.c`), with
`worldFallDeathZ` (8011ECC4: -800 on level 8, else 80) and `worldFallLimitZ` (`world_fall_limit.c`). Found for PC
port request 107.

**Code (CONFIRMED).** Once a car is below its level's fall limit its falling flag (`state` +0x1F) is set; while it
is set the car dies (HP -1) only when `motion.pos[2] < worldFallDeathZ()`. `motion.pos[2]` is the body's height:
the ground under the wheels plus the car's `ride_height` (`car_contacts.c`, `car_object_contact.c`). Retail ride
heights are 12 to 66; Dark Tooth's is 116 (`car_stats.c`). So the floor that kills a car is 80 less its ride
height: about 65 for a Twister, -36 for Dark Tooth.

**Effect (LIKELY; seen in the PC port).** On a pit floor between those heights Dark Tooth stays alive while every
other car dies, and a round waiting for his death waits for the clock. Retail never shows it: Dark Tooth fights
only on level 8, whose death height is -800. In the PC port he can drive any arena (Versus and Custom Tournament
boss rounds, a player's Dark Tooth), and Antarctica, New York and Roof Tops have the death height 80.

**PC port:** retail behaviour. With `--port-define TM2_PORT_FALL_WHEELS` both tests read the wheels' height
(`motion.pos[2] - stats.ride_height`), so every car dies at the same floor. A car on such a floor died in retail
too; a falling car now dies when its wheels, not its body, pass the line (a little earlier). The level 6
no-blast death (`car_damage.c`) and the won-round rule (`car_destroyed.c`) are unchanged. `check_custom.py` check
20 runs both tests in a MIPS interpreter.

## 41. Antarctica's first six floes have no ground in modes 2 and 4 (paired with the split-screen file)

**Where:** `worldCollapseClearDone` (80116310, `world_object_register.c`), called by `arenaLoadAntarctica`
(80119D90) after the level's objects are installed; `settingsLevelDmdFileName` (8012A998). Found for PC port
request 111.

**Code (CONFIRMED, disc too).** In game modes 2 (the co-op tournament) and 4 (two players head to head) the load
takes `GLACIER2.DMD` for Antarctica (and other files for Paris and New York), and `worldCollapseClearDone` sets the
triggers-done counter to 6 and disables the collision of floes 0..5 (`worldSectionDisable`: no floor, the surfaces
reset). `GLACIER.DMD` has the floes 0..6, 9..13, 15..19, 21, 22, 24..26, 28..30 (id 3100 nodes); `GLACIER2.DMD` the
same without 0..5. So retail is consistent: the file has no floes 0..5 to draw.

**Effect (seen in the PC port).** A load that takes `GLACIER.DMD` in mode 2 or 4 draws floes 0..5 with no ground
under them from the first frame: cars fall through drawn ice into the water. The PC port loads the whole arenas in
those modes since 2026-11-05 (`vplus/fixes.c` `whole_map_name`); retail never does.

**PC port:** with `--port-define TM2_PORT_WHOLE_MAPS` the six are taken only when the load found none of them
(`worldCollapseRegister` marks each floe found; `worldCollapseReset` clears the marks before the load), so the whole
file's floes stand and break away as in the Challenge, and `GLACIER2.DMD`'s behave as retail. `check_custom.py`
check 23.

## 42. Los Angeles: the curved freeway's outer parapet tops have no ground (unreachable: not fixed)

**Where:** `TERRAIN\FWY.GRP` / `FWY.RMP` against `FREEWAY.DMD` (meshes 8005ECF8, 800603F8, 80061E3C, each drawn
twice: as it is for the east half, and under the transform node 8006C9CC for the west half); loaded by
`arenaLoadLosAngeles` (80119D08). Found for PC port requests 111 and 112.

**Data (CONFIRMED, disc).** The curved freeway is two raised half-rings round (5376, 2688), radius 2232..3144,
angles 182..841 and -841..-182, with a gap between them. Section 1 (the deck, no floor of its own) takes surfaces
79..84: the two deck surfaces at 336 (79 and 80) and a small rise. The drawn outer parapets stand on the deck's
outer edge, and their tops are flat quads at 440, radius 3104..3282, angles 186..838 and -838..-186, 14 quads in all.
No surface covers radius 3145..3282 at either height, so a car that lands on a parapet top has no ground until the
road below (0 to 96). The rest of the curve is solid: every drawn top of the deck has its surface.

**Effect: none in play.** The hedge walls are hit boxes taller than their drawn tops (`hdpTestBox`; every point
tested on the top lies inside one), so no car gets over them: the user tried on 2026-11-10 and could not. A car
would sink through only if it got up there, which it cannot.

**PC port: not fixed** (removed 2026-11-10, after decomp PR #129 had added two round surfaces at 440). `check_custom.py`
check 24 checks that those points keep the disc's own ground.

## 43. Los Angeles: the ends of the three ground-level curves are drawn higher than their ground

**Where:** `TERRAIN\FWY.RMP` against `FREEWAY.DMD` (meshes 80049488, 8004A900, 8004BCF0, 8004D0E0, the same
curve drawn at the north-west, south-west and south-east corners); loaded by `arenaLoadLosAngeles` (80119D08). Found
for the PC port's request 112 follow-up (a playtest video: a car sinking into the concrete beside the curve).

**Data (CONFIRMED, disc).** Each corner is a banked curve round (+-5376, +-2688), radius about 2232..3304. Over most
of the curve the drawn road and bank match the ground (`worldRoundSurfaceHeightAt` on surfaces 38..51, 90..103,
114..127). Two places do not:
- **the outer lip at both ends** (the first and last 180 of each curve's 1024, radius about 2965..3270): drawn
  rising from 96 to about 170 as the bank begins, while the ground there stays near 96 (the flat shoulder,
  surfaces 43/95/119, and the bank's own outer edge). Up to 65 under the drawn top;
- **the inner kerb** (radius 2234..2400, the whole curve): drawn as a curve from 96 down to 16..46, the ground a
  straight slope below it. Up to 35 under (48 at the north-west, whose bank starts at radius 2400, not 2360).

**Effect.** A car driving along the lane by the wall into a curve, or onto its inner kerb, sinks into the drawn
concrete by up to 65 (about half a car) and drives inside it.

**PC port:** with `--port-define TM2_PORT_FWY_GROUND` (`TM2_PORT_FWY_PARAPETS`, its first name, still works),
`portFwyGroundAdd` inserts round surfaces made from the drawn polygons: 31 at the north-west (section 0),
24 at the south-east (13), 24 at the south-west (12), each group right after its section's last surface, so every
section keeps one range. Over the three corners the points where the drawing is more than 16 above the ground drop
from 660 / 432 / 444 to 13 / 17 / 36 (a 24 x 6 polar grid); about 12 points per corner, at the lip's steep outer
edge, now have ground up to 30 above the drawing. `check_custom.py` check 24.

## 44. A car destroyed in the air over a drop is never hidden or blown up where it lands

**Where:** `carDestroyed` (8011BE10, `car_destroyed.c`), called once per death by `humanCarDamageUpdate` (8011B89C)
and `aiCarDamageUpdate` (8011BBD4). Found for PC port request 117.

**Code (CONFIRMED).** `carDestroyed` picks the car's end from its height at the moment it dies: on Antarctica below
240 it hides the car and splashes, on level 8 below -40 it hides it, anywhere else it explodes it where it is. No
later code looks at the wreck's height again. And for a human's car it does nothing at all while `roundWonFlag` is
set (the round won).

**Effect (seen in the PC port).** A car killed by a weapon as it goes over Antarctica's crevasse explodes in the air,
and its wreck falls to the crevasse floor and lies there in view, never hidden; on New York and Roof Tops such a
wreck lies on the floor without the explosion a car that dies of the fall gets. A human who falls in after the round
is won lands on the floor with nothing at all. Retail shows it rarely (a kill at the edge, or a fall after a won
round); the port's online Versus showed both in one boss round.

**PC port:** with `--port-define TM2_PORT_WRECK_FALL`, `portWreckFall` (from `humanCarTick` and `aiCarDrive`, after
the fall test) hides a still-drawn wreck below Antarctica's 240 (level 8: -40) with `carDestroyed`'s splash, and
explodes a falling wreck once when it passes the fall death's test, unless `carDestroyed` already blew it up there
(`portWreckFallBlown`). `PLAYTEST_ANSWERS.md` 33; `check_custom.py` check 28.

## 45. Antarctica: the fire missile on floe 5 can never be picked up

**Where:** `GLACIER.DMD` (built for 0x80019C40), the hit walk `hdpPassRun` / `hdpWalk` (8014FAB0 / 8014EA60;
C copy `reference/hdp_walker_engine_hit_detection.c`). Found for PC port request 120. Also on TASVideos's glitch list.

**Data and code (CONFIRMED, a model of the walk on the disc's tree, at the level's shift 3).**
- **The pickups on ice floe 5** (`80051B7C`, a transform node, id 3100, turned 60 degrees):
  - power 404 (`80051858`) and homing 403 (`80051870`), under group `80051B04`;
  - fire 401 (`80051B40`) at (600, -800, 4800), under its own LOD `80051B64`.
- **Over part of the floes** lies a hit-only mark volume, `800561E8`:
  - six planes, kind -2: the slab 4800 < z < 6400, cut by four side planes;
  - it sits under the hit-only group `80056344`, the level root's last child, which the walk visits first.
- **What the mark does:** `hdpLeafTestPointsMark` sets `hit = 2` on every car point inside it.
  `hdpLeafTestPoints` skips any point already hit, so the fire pickup's leaf (`80026ADC`, id 401) never records a
  point.
- **Where the mark ends:** at y -800 it covers floe-local x below about 800. The fire sits 200 inside it; power and
  homing lie outside.
- **Swept** with a car centre within 90 of each pickup over 8 headings: fire collected 0 / 1,352 times, power and
  homing 1,109 / 1,352 each.
- **The rotation is not the cause:** the LOD node changes neither the transform nor the rotation stack. Relinking the
  pickup straight under the floe leaves it inside the mark (0 / 1,352).
- **A second fire pickup on floe 3** (`8004F0E8`) lies at local z 0, 4,800 below its floe. It is out of reach and
  inside another mark (`8005608C`): never collectable and LIKELY never drawn. Left as retail.

**Effect (seen in the PC port and on the PlayStation):** a car drives straight through the fire missile and gets
nothing.

**PC port:** with `--port-define TM2_PORT_GLACIER_FIRE`, `portGlacierFire` moves the pickup and its LOD's centre to
(992, 119), z kept.
- **When:** from `worldLoadStartObjects`, level 6, before the pickups are registered.
- **Why that spot:** it is (600, -800) turned by the floe's 60 degrees, as if the offset had been written in the
  unturned frame (LIKELY). It puts the pickup by the other two, on the same ground (section 5, 4800).
- **Safety:** every word is checked first.
- **Result in the model:** collected 1,115 / 1,352 times, the other two unchanged.
- `PLAYTEST_ANSWERS.md` 34, `check_custom.py` check 30.

## Cross-references

| Where | What | Doc |
|---|---|---|
| Secret levels (Swamp, Cyburbia, Roof Tops) | AI route data never finished: the route byte is wrong in every node, Roof Tops' route table points at nodes that do not exist, stuck AI cars jump to node 0 | `docs/SECRET_LEVELS.md` sections 4 and 6 |
| Level 9 "It's a Secret" | No files; selecting it would crash | `docs/SECRET_LEVELS.md` section 1 |
| Pedestrian run-over | No state re-check: two cars hitting one pedestrian in the same frame both "throw" it and both add to the bonus counter | `docs/GAME_EVENTS.md` section 4 |
| Several hits in one frame | The killing-blow branch can run more than once per frame; the last write wins | `docs/GAME_EVENTS.md` 1.1 |
| Cheat codes | Infinite weapons, no damage, Mega Guns are retail features, not bugs | `docs/GAME_EVENTS.md` 9.2 |
| AI-versus-AI damage | Damage from a computer car to a computer car is divided by 4 (`carApplyDamage` 800E5A4C). Retail behaviour the port may change (teams) | `docs/AI_VS_AI.md` sections 1 and 3 |
| AI route walk around an off-route target | `aiPickLinkNode` takes the successor nearest the aim; with the target off the route the car shuttles around the route point nearest it (A-B-A-B-C-B, 180-degree turns), retail's give-up re-plans from the same point. Retail logic; seen on the port's Cyburbia route | `docs/PLAYTEST_ANSWERS.md` 16 |
| AI aimed shots | Every aimed shot or homing missile of a computer car steers at player 1, not at its target (`weaponFindTargetAhead` 80109F94 / `weaponFindTargetBehind` 8010A2AC). Retail behaviour the port may change | `docs/AI_VS_AI.md` section 4 |

## Checked and not bugs

| Claim | Finding |
|---|---|
| 800FD00C `hudRadarArrowsLoad` writes `hudRadarArrowUpSprites[0..7]` into the Down/Left/Right arrays | Each of the four arrays (80183EA8, EB8, EC8, ED8) is 16 bytes = two 8-byte records. Each id (0x50A..0x50D) writes its own two records. No spill. Oddity only: both calls parse the same first rect, so the two records are equal. |
| 800D42E0 `levelEventsReset` clears entries -1..3 of `moscowDishEmitterPositions` | It clears word +0 of five 16-byte records at 801B2B58. The positions are at +4 (801B2B5C, `moscowDishSetEmitterPos`). So +0 is a flag of each record, not entry -1. Nothing else reads it. Not a bug; the C names are offset by 4. |
| 8010AE04 `pickupSlotsReset` sets `pickupAmmoAmounts[2] = 1` | The table starts at 8018429C and is indexed by weapon id 0..9. Entries are 2, and entry 3 (remote bomb) is 1. `pickupApply` reads 801842A0..801842B8 per weapon; the remote bomb case reads 801842A8. So a remote bomb pickup gives 1. Intended. The name `pickupAmmoAmounts` (801842A0) is one entry late. |
| `weapon_tick.c` ten-count reset writes one past `ammo[9]` | The C struct declared 9 entries; the block really has 10 (+0x0E..+0x20), matching the pickup table. Fixed: `TM2CarWeapons.ammo[10]`. Decomp naming, not retail. |
| 800D3528 / 800D3530 | IDA split, not a bug. |

## Decomp notes (not retail bugs)

These come from checking every prototype against its definition
(`config/US/signature_conflicts.tsv`, docs/SIGNATURES.md) and the retail code. None
changes the image; each was checked with `tools/match_unit.py`.

### Arguments the callee never reads

Retail callers pass arguments that the callee does not read. When every retail
caller passes the argument, the definition now names it as an unused parameter:

| Callee | Callee reads | Callers pass | Extra argument |
|---|---|---|---|
| 8011A5CC `carBindAiSlot` (`car_parts_spin.c`) | 2 | 3 | carBindAiByType passes the owner id in `a2` |
| 8014D3F4 `lightAddSources` (`engine_fileio_debris.c`) | 3 | 4 | `renderObjectMatrices` in `a3` |
| 800D9238 `fxTrailUpdate` (`ai_route_nodes.c`) | 1 | 2 | the record index (6 calls) |
| 80114C5C `lightFlickerStart` (`arena_setup.c`) | 2 | 3 | a colour pointer (4 units) |
| 800EFCBC `wheelSectionUpdate` (`ai_link_node_pick.c`) | 4 | 5 | 800EFAD0..800EFC08 store it at `16(sp)` |
| 800E6CF8 `wheelSpinOutSettle` (`misc_one_liners.c`) | 1 | 2 | the motion block (`move a1,s2`) |
| 80124718 `carSetDamageStage` (`car_switch_registry.c`) | 2 | 3 | a third value (3 units) |
| 80121C64 `screenLoadImageFile` (`screen_basic.c`) | 1 | 2 | always 0 |
| 80121D38 `screenDisplayToDisplay` (`screen_transfer.c`) | 1 | 2 | the other buffer, (1, 0) or (0, 1) |
| 8012CE04 `passwordEncode` (`password_codec.c`) | 3 | 4 | 0 |
| 80109D58 `weaponDropsUpdate` (`fx_sprite_spawn.c`) | 0 | 2 | the draw context and view |
| 8012DE7C `shellAttractDemo` (`shell_attract_demo.c`) | 0 | 2 | (0, 0), as for a screen handler |
| empty stubs in `nullsubs.c` (`carBindHumanSlotStub`, `carBindAiSlotStub`, `worldLoadStub688`, `worldCollapseReleaseStub`, `worldLoadStub3503`, `weaponHitStub3503`) | 0 | 1 or 2 | the record / object and id |

The C++ constructor at 80123B64 (`TM2FourWay`) reads `this`, the target and three
values; its fourth `int` parameter was never read and is gone.

`cdAudioInit` (8015E8EC) passes its `a0` on to the empty `cdPlayerBaseInit`, but its
only caller, shellInitMachine (80128B74), passes no argument. The call keeps a
function-pointer cast with a comment.

### Arguments passed through in registers

Some retail functions call another one while `a0`..`a3` still hold their own
arguments. Our C used to call these with no arguments; it now passes them
explicitly (same code): aiPickWeapon -> aiPickWeaponFromTable9/10(ai);
sub_8011E114(id) -> humanCarFindById(id); carDmdFileName/carTmsFileName(settings,
car) -> carFileCode; weaponFirePowerMissile -> weaponSpawnProjectile (four
arguments); renderSetWorldShift(level) -> hdpSetDifficulty(level);
taskMgrCheckList(m) -> taskMgrListError(m) -> taskMgrErrorStub(m).

### Width differences between caller and callee

In these places retail narrows an argument or a result that the callee treats as
32 bits (or the other way round). The prototype is the definition's; the call keeps
an explicit cast with a comment:

| Caller (unit) | Call | Retail code |
|---|---|---|
| 8012BAFC sessionLoadArenaAndCars (`shell_main.c`) | `carTmsFileName`/`carDmdFileName(settings, (s8)(u8)car)` | `lbu`, `sll 24`, `sra 24`; the callees never narrow |
| `tournament_pick_opponents.c` | `settingsSetOpponentCar(.., (u8)index, (u8)value)` | both narrowed |
| `screen_single.c` | `passwordDecode((u16)code, ..)` | `andi 0xFFFF` |
| `car_contacts.c` | `carPairDirection((s16)other, (s16)id)` | both sign-extended |
| `car_input.c` | `padCheckAction((u8)action, (s16)player)` | both narrowed |
| `car_records.c` | `(u8)padComboHeld(1, player)` | the result is tested as a byte |
| `menu_object_helpers.c`, `screen_options_audio.c` | `menuSliderSetValue(.., (u16)value)` | `andi 0xFFFF` |
| `weapon_special_missiles.c` | `carApplyDamage(.., (s16)attacker)` | sign-extended |
| `scene_init.c` | `(u16)passwordEncode((u16)level, (u16)car, ..)` | arguments and result narrowed |
| `screen_title_level_select.c` | `(u8)codeDetectorGetLastCode(..)` | result narrowed |
| `screen_resource_records.c` | `(u16)sceneObjGetId(..)` | result narrowed |

Callees that narrow a parameter on entry while their callers pass an `int` are
written as old-style (K&R) definitions, so the prototype has the promoted type:
carSpinOut, carApplyDamage, padCheckAction, weaponDropWrite,
weaponFireEnergyAttack, weaponFireSelected.

Two units keep their own declaration, with a comment, because retail compiled
them with a different return width: `hud_main.c` (`int gameGetLives(int)`; the
definition returns `u8`) and seven units that re-sign-extend
`timerGetFrameTicks()` (`s16`; the definition returns `int`).

### Results taken from void functions

These functions return nothing (`v0` is left over from their last call), but some
retail callers return or use that `v0`. Those units keep an int-returning
declaration with a comment: animSetStep (`car_freeze_dispatch.c`), bitmapMarkRect
(`bitmap_init.c`), carSetViewMode (`view_watch_ai.c`), carStatsDerive
(`small_game_helpers.cpp`), renderDrawModel (`menu_password_digits_draw.c`,
`menu_slider_draw.cpp`), sceneObjSetChoiceState (`menu_render_dispatch.c`),
sceneObjSetPosition / sceneObjSetRotation (`motion_commands.c`), the soundMgr*
functions (`command_wrappers.cpp`), and the PsyQ AddPrim (`packet_digits.c`) and
DrawOTag (`screen_ending.cpp`). In the C++ units a void call in tail position
would compile to a jump, which retail does not have.

Definitions declared `int` without a `return` are now `void` (same code):
groundFiresUpdate, hudInitPanels, mathCopyWords, mathMulMatVec, mathMulMatVec2,
mathMulTransVec, mathMulTransVecShort, weaponTick. screenHistory (8013629C) now
returns the next screen handler from sceneRun, like the other screens.

### Open: a call that could match

80150A74 (`engine_anim_hit_volumes.c`) calls `csGetRoot(a, b)`, but csGetRoot
reads no arguments and retail passes none. Dropping the two arguments makes
80150A74 byte-match retail (7 words differ now). It is kept, with a cast and a
comment, so that the image hash stays the same; the change is a one-line
follow-up.
