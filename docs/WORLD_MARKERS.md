# World markers: drawing something at a point in the arena (King of the Hill)

Answers item 21 of the PC port's `docs/DECOMP_REQUESTS.md`. The port spawned smoke puffs at the
hill with `fxSpriteSpawn` and saw nothing. This file explains exactly what an effect sprite needs,
why those puffs were not seen, which existing effect makes a good column or ring, how to colour
it, and how the port can add its own primitives with the game's camera instead.

Read with `RENDER_ANSWERS.md` (R1 for the sprite projection, R3 (a) for the 48-byte record),
`MAIN_LOOP.md` (the loop order) and `WIDESCREEN.md` (the view table and the projection).

How this was checked: static analysis only. Sources were the C in `src/semantic`, the retail
disassembly (`bin/SCUS_94306R.EXE`, `mipsel-none-elf-objdump`, load address 0x800CDC54), and the
level files `DMD\*.DMD` read from the USA disc image (`cdimage/`), for the sprite frame tables.
Nothing was built or run.

Tags: **CONFIRMED** = seen in code or data and understood. **LIKELY** = inferred; check it before
relying on it. Names are the current ones from `config/US/names.tsv`.

## Short answer

- **The call itself was right.** `fxSpriteSpawn(pos, 16, 2, 16, fxSmokeFramesC, depth, follow)`
  is exactly the game's own `fxSmokePuffBlue` (80111AB0), the puff that appears when the energy
  shield puts a fire out. That puff is made to be seen next to the player's own car.
- **Most likely reason nothing was seen (LIKELY; the port's code was not seen here):** the puff
  was drawn, but it was too small, too faint and sorted behind the ground.
  1. **Size.** A smoke cell is 31 texels wide. At scale 16 the sprite is 2 x ((31 · 16 · 228 / z) >> 3) pixels
     wide, with z in position units. That is 28 px at 1000 units, 4 px at 5000, and 2 px at
     10000 or more. Arena points are thousands of units apart. The table in 2.4 has the numbers.
  2. **Blend.** `fxSmokeFramesC` is the **additive** smoke sheet (abr 1, read from every level
     file). Its tint multiplies the texture before it is added to the screen. The tyre smoke
     uses 0x10, almost nothing. The same puff is invisible over snow or a bright wall.
  3. **Depth.** Sprites go into the ordering table at `z >> 3`, which is linear. The level
     geometry uses a compressed depth beyond 1024 (`DRAW_DISTANCE.md` §7). So a sprite farther
     than about 8192 position units from the camera sorts **behind** nearly all the geometry
     around it, ground included. It shows only against the sky (section 2.5).
  4. **Lifetime.** One puff lives 2 x 16 = 32 steps (about 1.6 s) and does not rise unless
     `vel[2]` is set. Every human car restart wipes all 150 sprites (`fxSpritesReset`).
- **Things that are *not* the problem (CONFIRMED):**
  - No list, kind or flag needs registering. A record is live while its frame byte (+10) is
    `>= 0`.
  - `owner_pos` does not need to point at a car. The game code never masks addresses.
  - `fxSpritesDraw` runs for every drawn view, every frame, in every match mode.
  - The 6th argument (the "count" that lands in +6) is only a depth hint (section 2.5).
- **Also check on the port side.** Arguments 5 to 7 are passed **on the stack** (caller sp+16,
  +20, +24). A host-to-guest call shim that sets only a0 to a3 gives a garbage table, depth and
  follow. Read the record back after the spawn (section 5).
- **Best existing effect for a marker:** a "type 1" sprite. This is the mechanism of the ground
  flames (`fxGroundFlame`): one quad stretched from the point up to `zofs` units above it, so a
  single record makes a vertical column.
  - Use `fxSmokeFramesB` (average blend) for colours that read on any background, or the
    flames (additive) for a glow.
  - Spawned directly with `fxSpriteSpawn`, it does **no damage**. Only `groundFireSpawn`
    records ignite cars.
  - Set `loops` high so it stays.
  - A ring is 8 to 12 of these around the hill.
- **Own primitives are easy.** The frame record passed to `fxSpritesDraw(rec, view)` gives the
  OT (+0, 4096 entries, reverse order) and the packet cursor and end (+8, +0xC). The camera is
  `viewGetEyeTrans(view)` and `viewGetViewMatrix(view)`. The projection is
  `x = cx + v.x·H / v.z`, as in section 6. Hook right after `fxSpritesDraw` returns.

## 1. `fxSpriteSpawn` 80110918 exactly (CONFIRMED, C in `fx_sprite_spawn.c`, asm 80110918..80110A3C)

```c
struct TM2FxSprite *fxSpriteSpawn(s32 *pos, s32 scale, s32 rate, s32 cells,
                                  u8 *table, u16 depth, u8 follow);
```

| Arg | Register / stack | Stored | Meaning |
|---|---|---|---|
| `pos` | a0 | `pos` +20..+28 copied **now**; the pointer itself is stored at +44 | position in position units (section 4); read once here |
| `scale` | a1 | u8 +8 | sprite size factor (section 2.4). `scale >= 96` adds 120 to the copied z (signed compare on the full a1; the stored byte is truncated) |
| `rate` | a2 | u8 +9 | steps per animation cell |
| `cells` | a3 | u8 +11 = `rate * cells` | the length of one pass in steps (`frames`); compared as **s8**, so keep `rate * cells <= 127` |
| `table` | **sp+16** | ptr +16 | frame table: 8-byte cells {u, v, w, h, tpage, clut} (section 3) |
| `depth` | **sp+20** (lhu) | s8 +6 | OT hint: 1 = 8 entries nearer, 8 = fixed entry 8, others no effect (section 2.5) |
| `follow` | **sp+24** (lbu) | u8 +1 | non-zero: the draw copies `*pos` into the record every draw (section 4.2) |

Fields it resets:

- frame +10 = 0, loops +12 = 0, type +0 = 0, tinted +2 = 0, r g b +3..+5 = 0, vel +32..+40 = 0.

Fields it does **not** touch:

- zofs +14, which keeps the value of the slot's last user;
- the slot index +7.

It returns the record, or 0 when all 150 slots are busy (`fxSpriteFindFree` 80110BA8 returns the
first record with frame < 0). It plays no sound and touches no cooldown. Only the wrappers
`fxExplosion*` and `fxSmokeBurst*` do that.

Every game wrapper writes after the spawn:

```c
o->vel[2] = rise;            /* +40: units per step, z up */
o->tinted = 1;               /* +2: without it the tint is ignored */
o->r = R; o->g = G; o->b = B;    /* +3..+5 */
```

Examples: `fxSmokeC6` (tyre smoke) is `(pos, 6, 2, 16, fxSmokeFramesC, 1, f)`, rise 1, tint
0x10. `fxSmokeB48` is `(pos, 48, 2, 16, fxSmokeFramesB, 0, f)`, rise 15, tint 0x80.
`fxGroundFlame` 80111F48 is `(pos, 10, 1, 8, fxFlameFrames, 1, f)` with type = 1, loops = 50 and
zofs = 300.

## 2. What a record needs to move and to be drawn

### 2.1 Live, moved and freed: `fxSpritesUpdate` 80110A40 (CONFIRMED, C in `car_restart.c`, asm checked)

It runs once per step (#11 in `MAIN_LOOP.md`, about 20 steps a second). For each of the 150
records with frame (s8 +10) `>= 0`:

1. `frame++` and `pos += vel`.
2. If `frame >= frames` (both compared as s8):
   - `if (loops-- > 0) frame = 0; else frame = -1;` (-1 frees the slot).
   - If `loops` is now 1 and the table is one of the three smoke tables, `frames += 16`, so
     the last pass is longer. With `rate * 16` cells already in use, that pass reads 8
     cells past the end of its table:
     - A then reads B's cells, and B reads C's. They are the same image (section 3), so this is
       harmless.
     - C reads `fxContrailFrames`, so C shows a wrong look for those 8 cells.
     - To end a long-lived smoke record cleanly, write `loops = 0` yourself.

There is no other condition. There is no list, kind, owner or distance check, and no game code
reads sprite positions. `fxSpritesGet` 801134C8 has no callers. Sprites are purely visual.

**Lifetime** = `frames x (loops + 1)` steps. A pass is at most 127 steps. A marker that must
stay sets `loops = 32767`, which is hours, and ends it with `loops = 0`.

**What wipes sprites (CONFIRMED):** `fxSpritesReset` 80112820 frees all 150. It is called:

- from `carRestart` 800FD550 (`car_restart.c` line 198), which runs at each round start
  (`arena_setup.c`), on `restart_pending` (`car_tick.c`), from 8011E114, and from `scene_init.c`;
- from `arenaResetSystems` 80128CE4 and `ua_init.c`.

So the port must check its marker every step: `frame < 0`, or `table` not its own. Respawn it
when that happens.

### 2.2 Drawn: `fxSpritesDraw` 80110BE4 (CONFIRMED, C in `world_collapse.c`, asm 80110BE4..80111310)

The call sites in `rtMainLoop` (`rt_main.c`):

- view 0 every iteration (#12);
- view 1 when split screen is on, after `debrisDraw`;
- the rear-view mirror when it is on (view index 1, mode 4).

That is every frame of every match, the attract demo included. Nothing is drawn during the
blocking `pauseMenu` or in the shell.

For each live record:

| Step | Code | Notes |
|---|---|---|
| skip | `view > 0 && frame == 1 && loops == 0` | only one-pass records, only on their first step |
| follow | `if (follow) pos = *src` | every draw, through the raw pointer (section 4.2) |
| camera | `v = mat · (pos + eye) / 4096` | `eye = viewGetEyeTrans(view)` (`viewEyePositions` 801A51A8), `mat = viewGetViewMatrix(view)` (`viewMatrices` 801A51C0), `mathMulMatVec` 80153FE4 |
| depth cull | drop unless `8 <= v.z <= 32000` | position units; the `< -32` test is redundant |
| size | `half = (table[0].w · scale · H / v.z) >> 3`, clamped 1..512 | H = view mode +0x08 (`viewGetProjScaleX` 801555FC); cell 0's w for every frame |
| centre | `sx = cx + v.x·H / v.z`, `sy = cy + v.y·Hy / v.z` | `cx, cy` = view w/2, h/2 (`viewGetCentre` 80156614); Hy = mode +0x0C (= H) |
| OT index | `z = v.z >> 3`; `depth == 1`: `z -= 8`; `depth == 8`: `z = 8`; `z < 0`: 0 | no upper clamp (32000 >> 3 = 4000 < 4096) |
| screen cull | drop unless `-99 <= sx <= 999` and `-99 <= sy <= 999` | the GPU clips the rest |
| packet | drop if `cursor + 160 >= end`; `cursor += 160` | **160 bytes per sprite**, of which only 40 (a POLY_FT4) are written; `addiu v0,v0,160` at 80110F3C |
| prim | POLY_FT4, code 0x2C, plus semi-transparent (2), plus raw texture (1) | if `tinted`: the raw bit is cleared and RGB = +3..+5 |
| UV | cell `frame / rate` of the table | tpage and clut from the cell |
| type 1 | the quad runs from the point to the projection of `pos + (0, 0, zofs)` | see 2.3 |
| link | `AddPrim(ot + z, prim)` | |

### 2.3 Type 1: the vertical column (CONFIRMED code, LIKELY look)

With +0 = 1, the draw also projects `pos + (0, 0, zofs)`. That point is divided by the **base's**
depth, not its own. The quad is turned so that one edge is at the base and the opposite edge is
at that top point, with a width of about `2·half`. So one record draws an upright strip `zofs`
units tall. `fxGroundFlame` uses `zofs = 300`. The strip faces the camera around its vertical axis,
like a billboard. Seen from straight above it shrinks to a short smear.

### 2.4 Size (CONFIRMED formula; screen numbers are arithmetic)

The half size in pixels is `(w · scale · H / z) >> 3`. Here w = 31 for smoke, sparks and
explosions, and 63 for flames. H = 228 in one-player mode, 138 in vertical split, 228 in
horizontal split and 91 in the mirror. In world terms the sprite is `w · scale / 4` position
units across: scale 16 smoke is 124 units across, scale 255 is about 1980.

| Distance z (position units) | scale 16 | scale 64 | scale 128 | scale 255 |
|---|---|---|---|---|
| 1 000 | 28 px | 112 | 226 | 450 |
| 2 000 | 14 | 56 | 112 | 224 |
| 5 000 | 4 | 22 | 44 | 90 |
| 10 000 | 2 | 10 | 22 | 44 |
| 20 000 | 2 (minimum) | 4 | 10 | 22 |
| > 32 000 | culled | culled | culled | culled |

These are full widths (2 x the integer half size) for 31-texel smoke at H = 228. For scale: arena points are thousands of
units apart. The warp pads in `worldWarpsSetup` 80115310 sit at points such as (3144, 4368, 360)
and (-6968, 9336, 136), and level 6 has x up to 15256.

### 2.5 Depth and the ordering table (CONFIRMED code; the sorting effect is LIKELY)

- The OT is 4096 entries, cleared with `ClearOTagR` and drawn from entry 4095 down to 0. A low
  index is drawn last, so it is in front.
- Sprites use `v.z >> 3`. The level emitter `sub_800CDC54` uses `SZ >> shift` (shift 3 in every
  level). That value is linear below 1024 and compressed above it: `1024 + (z - 1024) / 4`, and
  so on (`DRAW_DISTANCE.md` §7, `reference/model_emitter_800CDC54.c` lines 292..300).
- Below 8192 position units of depth, both agree.
- **Beyond 8192, a sprite at linear index W sorts behind every polygon whose compressed index is
  below W.** That covers nearly all the geometry nearer than about 4·W - 3072, so the ground
  behind and around the sprite is drawn over it.
  - Example: a sprite 16000 units away gets index 2000. A ground quad whose far corner is 20000
    units away gets index 1024 + (2500 - 1024) / 4 = 1393, so the quad is drawn after the
    sprite and hides it.
  - Few meshes use a fixed OT index: none in GLACIER, 3 in HKONG, 173 of 428 in DEN (indices
    4090..4093, drawn first).
  - LIKELY this is retail behaviour for all far sprites and tracers (streaks also use
    `z >> 3`). `fxDebrisDraw` uses `renderCompressDepth` 8014D368 and sorts correctly.
- The `depth` byte (+6, the spawn's 6th argument) changes only this:

| depth | OT index | Use |
|---|---|---|
| 1 | `z/8 - 8` | most smoke and flames: slightly in front |
| 8 | 8 | drawn over almost everything, through walls too. Useful for a marker that must always show |
| anything else | `z/8` | |

**For a marker:** use depth 8 to see it everywhere, through buildings. If the port overrides
`fxSpritesDraw` in C, it can instead map the index with `renderCompressDepth(v.z >> 3)`. That
sorts like the level for all sprites, but it changes retail's look.

### 2.6 Tint and blend (CONFIRMED)

- `tinted` (+2) = 0: raw texture, no modulation. The +3..+5 bytes are ignored.
- `tinted` = 1: the GPU modulates, `texel · tint / 128` per channel, clamped to 255. So 0x80 is
  neutral, 0xFF is about 2x, and 0x10 is 1/8.
- Then the semi-transparency mode (abr) from the cell's tpage applies. It applies only to texels
  whose CLUT colour has the STP bit set. Colour 0x0000 is transparent.
- The abr values read from the disc are in section 3:
  - abr 0: `(bg + f) / 2`
  - abr 1: `bg + f` (additive)
  - abr 2: `bg - f` (subtractive)

## 3. The frame tables (CONFIRMED from every level DMD on the USA disc)

`worldLoadObject` 800D1DB0 fills these at level load from mesh nodes with the given ids
(`grutilsParseSpriteRects` 8014E008). Values for GLACIER and HKONG are listed. The cell sizes and
abr are the same in BURB, DEN, DISH, FREEWAY, GLACIER2, ISLANDS, PARIS, PARIS2, ROOF, ROOF2, SROOF
and SWAMP. Only the VRAM page and the CLUT move.

| Table | VA | Asset id | Cells | Cell w x h | abr | Blend | Game tint |
|---|---|---|---|---|---|---|---|
| `fxSmokeFramesA` | 80186F64 | 660 | 16 | 31 x 31 | **2** | subtractive: darkens (black car-damage smoke) | 0x40 |
| `fxSmokeFramesB` | 80186FE4 | 661 | 16 | 31 x 31 | **0** | average | 0x80 |
| `fxSmokeFramesC` | 80187064 | 662 | 16 | 31 x 31 | **1** | additive (tyre smoke, puffs) | 0x10; blue puff 0,0,100 |
| `fxFlameFrames` | 80187144 | 650 | 8 | 63 x 63 | 1 | additive | raw |
| `fxExplosionFrames` | 80186DE4 | 654 | 16 | 31 x 31 | 1 | additive | raw |
| `fxExplosionFramesB` | 80186E64 | 687 | 16 | 31 x 31 | 0 | average | raw. **Only GLACIER and GLACIER2 load it**; elsewhere the table is zero |
| `fxSparkFrames` | 80186EE4 | 653 | 4 | 31 x 31 | 1 (also forced by `explodeStoreSparkAnimation`) | additive | yes |
| bullet impact | 80186F04 | 656 | 8 | 15 x 15 | 1 | additive | |
| `fxContrailFrames` | 801870E4 | 652 | 8 | 63 x 15 | 1 | additive | |

- **New finding: smoke A, B and C are the same image** (same u, v and clut in each level) with
  three blend modes. Any smoke shade can be had by picking the table.
- DISH, FREEWAY and ISLANDS also carry other meshes with id 650: 1 to 10 cells, and in FREEWAY
  31 x 31 abr 2. Which one fills `fxFlameFrames` last depends on the walk order (not checked).
  Read the table back at run time there.

## 4. Positions, units and the owner pointer

### 4.1 Units (CONFIRMED)

- Sprite positions are in the same units as a car's `motion.pos` and an object's +0xA8 (+168).
  The game spawns smoke straight at `motion.pos` (`car_damage.c` 288, `fxFlameBurning(...motion.pos, 1)`).
- **z is up.** Falling compares `motion.pos[2]` with `worldFallLimitZ` (`car_tick.c`), and
  gravity acts on `vel[2]`.
- AI route nodes store height in their middle field: `motion.pos = {node.x, node.z, node.y +
  ride_height}` (`car_object_contact.c` 363..366). Hill points taken from the route nodes need
  that swap.
- No shift is applied to a position anywhere on the sprite path.
- The renderer's "world units" (the far distance 4800, the OT) are these units `>> 3` (the
  level shift, 3 in every DMD). This is LIKELY, from `DRAW_DISTANCE.md` §1, where a node is
  dropped when `depth > far << shift`.
  - So the sprite limit of 32000 is 4000 world units, a little short of the 4800 model far
    distance.
  - That corrects the GUESS in `DRAW_DISTANCE.md` §10.

### 4.2 `owner_pos` (+44) and follow (CONFIRMED)

- `fxSpriteSpawn` always stores `pos` at +44 and copies `*pos` once.
- `owner_pos` is read again only when `follow` (+1) is non-zero. Then the draw copies `src[0..2]`
  into the record on every draw, through a plain `lw` (asm 80110CEC..80110D18).
- It does **not** need to be a car. The game uses it for `fxFlameBurning` on burning cars and for
  the Dark Tooth swoop target. For a fixed marker, use `follow = 0` and any temporary.
- **Addresses:** the game code never masks a pointer (plain `lw` in the spawn and the draw).
  - With a real 8 MB guest map, 807FF200 is fine.
  - On a 2 MB PS1, 807FF200 mirrors 801FF200. That is 0xDF0 bytes below the initial stack
    pointer 801FFFF0 (EXE header +0x30), inside the live stack.
  - If any of the port's memory paths (an interpreter, a recompiler, a DMA or GTE helper)
    masks addresses with 0x1FFFFF, a follow sprite reads stack garbage on every draw.
  - With `follow = 0` only the spawn reads it.

## 5. Why the port's puffs were not seen: ranked, with checks

| # | Cause | How to tell | Fix |
|---|---|---|---|
| 1 | Too small at arena distance (2.4) | Watch from a car parked within 1000 units of the hill: the puff should be 28 px | scale 96..255 |
| 2 | Hidden by depth sorting beyond about 8192 units (2.5) | It shows only against the sky, or only up close | depth (6th argument) = 8 |
| 3 | Additive sheet C with a dim or white tint, over a bright background (2.6, 3) | Tint +3..+5 below 0x40, or snow behind it | `fxSmokeFramesB` (average) with tint 0x80..0xFF, or `tinted = 0` |
| 4 | Short life, no rise, wiped by `carRestart` (2.1) | `o->frame` reads -1 within 32 steps, or after a respawn | spawn every few steps, or keep one record with `loops` set; respawn when freed |
| 5 | Stack arguments not passed by the host call | read the record back: +16 must be 0x80187064, +6 the depth, +1 the follow flag, +11 = 32 | write the caller's sp+16/+20/+24 before the call |
| 6 | Position frame: route-node y/z swapped, or a PC-version coordinate | read +20..+28 back and compare with a car's `motion.pos` standing on the hill | `{x, y, height}` in `motion.pos` units |
| 7 | `follow = 1` with a pointer the port's memory layer aliases (4.2) | the record's pos changes on its own after the spawn | `follow = 0` |
| 8 | Spawned before the arena loaded its tables, or on a level without the table | `*(u32 *)0x80187064` is 0 | spawn only during the match |

Read-back after the spawn (the port can do this from host code):

```
+0  type      0 (or 1 for a column)     +8  scale  as passed
+1  follow    as passed                 +9  rate   as passed
+2  tinted    1 if you want the tint    +10 frame  0 now; -1 = freed
+3..+5 r,g,b                            +11 frames = rate*cells (<= 127)
+6  depth     as passed                 +12 loops
+16 table     the table's VA            +20..+28 pos (x, y, z-up)
+32..+40 vel                            +44 owner_pos
```

## 6. Correct calls (guest-side pseudo-C)

All of these must run in the update part of a step, once per step, not in 60 fps redraws.
Good places are right after `fxSpritesUpdate`, or from the port's `worldUpdate` hook.
`hill[3]` is any RAM the game can read.

### 6.1 One visible puff (rising smoke column when repeated)

```c
s32 hill[3] = { x, y, z };                   /* motion.pos units, z up, ground height */
struct TM2FxSprite *o = fxSpriteSpawn(hill, 96, 2, 16, fxSmokeFramesB /*80186FE4*/, 8, 0);
if (o) {
    o->vel[2] = 15;                          /* rise per step */
    o->tinted = 1;
    o->r = R; o->g = G; o->b = B;            /* 0x80 = the texture's own grey; 0xFF = brighter */
}
```

- scale 96 lifts the start by 120 (any scale >= 96 does).
- Spawned every 4 steps, 8 puffs are alive at once and the column is about 480 units tall.
- In big fights the 150-slot pool can be full (each skidding car spawns 2 tyre puffs a step),
  so a spawn can return 0.

### 6.2 A persistent column: one record, type 1

```c
o = fxSpriteSpawn(hill, 64, 2, 16, fxSmokeFramesB, 8, 0);   /* or fxFlameFrames, 2, 8 cells */
if (o) {
    o->type   = 1;          /* +0  */
    o->zofs   = 1500;       /* +14: column height in units (spawn does not reset it) */
    o->loops  = 32767;      /* +12: stays; write 0 to end it cleanly */
    o->tinted = 1; o->r = R; o->g = G; o->b = B;
}
keep = o;                   /* each step: if (keep->frame < 0 || keep->table != table) respawn */
```

- On a hill change, rewrite `keep->r/g/b`.
- `pos` can be moved by writing +20..+28. Keep `follow = 0`.
- With `rate * cells = 32` and loops 32767, the record lives for hours.

### 6.3 A ring

```c
for (i = 0; i < 12; i++) {
    a = i * 4096 / 12;
    p[0] = cx + ((radius * rcos(a)) >> 12);
    p[1] = cy + ((radius * rsin(a)) >> 12);
    p[2] = ground_z;
    o = fxSpriteSpawn(p, 24, 1, 8, fxFlameFrames /*80187144*/, 1, 0);
    if (o) { o->type = 1; o->zofs = 300; o->loops = 32767; o->tinted = 1; o->r = R; o->g = G; o->b = B; }
}
```

- That is 12 records out of 150. Use fewer (6 to 8) in split screen, where both views draw
  them from one packet buffer.
- The flames are additive, so a team colour reads as a coloured glow. On white levels
  (GLACIER), use `fxSmokeFramesB` at depth 8 instead.

### 6.4 White, then the team colour

| Look | Table | tinted, r g b |
|---|---|---|
| neutral white-grey cloud on any background | B (average) | 1, 0xC0 0xC0 0xC0 |
| team colour, readable on any background | B (average) | 1, team RGB (raise toward 0xFF for strength) |
| glowing team colour (dark arenas) | C or flames (additive) | 1, team RGB |
| dark or black marker | A (subtractive) | 1, the complement of the colour wanted (subtracting 0, 255, 255 leaves red) |

## 7. The other candidate effects

| Effect | What it is | Damage | Usable as a marker | Colour |
|---|---|---|---|---|
| Smoke `fxSmokeA*`, `B*`, `C*` | fxSprites, tables 660/661/662 | none | **yes** (sections 6.1 and 6.2) | tint + table choice |
| Sparks `fxSparkOrange/Blue/Pink` 80111314.. | fxSprites, scale 5, 4 cells, additive | none | small; fine at scale 32+ | tint |
| Flames `fxGroundFlame` 80111F48, `fxFlame3/10` | fxSprites, type 1, zofs 300 | **none by themselves** | **yes** (section 6.3) | tint (raw by default) |
| Ground fires `groundFireSpawn` 8010C020 | a `groundFires` record (801842C4) + 5 `fxGroundFlame` + `fxSmokeA48` | **yes**: `groundFiresUpdate` 8010C160 ignites cars within `max(|dx|,|dy|,|dz|)>>3 < 25` (35 for AI) after 20 steps; ends at 100 steps (`fx_ground_fires.c`) | not without damage. Calling `fxGroundFlame` directly gives the same look without the record | – |
| Explosions `fxExplosion6..192` | fxSprites, table 654, additive | none (the damage is the weapon's) | yes, but each plays sound 8 and rumbles. Spawn with `fxSpriteSpawn(p, s, 2, 16, fxExplosionFrames, 0, 0)` to avoid that | tint |
| `fxExplosionB*` | table 687 | none | GLACIER/GLACIER2 only | tint |
| `fxLightningSpark` 80112600 | fxSprites, loops 5, tint 255,200,50 | none | short | tint |
| Energy shield glow | a model object: `weaponShieldRecords` 80184A78 (4), pose copied from the car each step, z - 64 (`weaponShieldsUpdate` 8010DD30) | none | only by creating a csList object with the shield model; colour only through its CLUT, shared with real shields | CLUT |
| Turbo flame | per-car-type model objects (`carTurboObjects` 8018F5E4, `objRegistrySetStep(type*16+6)`) | none | no: part of the car's model | – |
| Warp effect | 3 csList objects (`worldWarpObjectsCreate` 80115C08, asset 693, an animation node present in every arena DMD), shown 40 steps by `worldWarpStart` | none | `worldWarps[2]` is used only on level 8, so on other levels its object is free: `obj->visible = 1`, `obj->pos = hill` (LIKELY; matrix and animation not checked) | CLUT only |
| Pickup spots | level-file models, `pickupObjects` 8018F6BC (`PICKUP_LOOKS.md`) | – | not recommended: tied to pickup slots and respawn | CLUT |

**Recommendation:**

- For the hill: a type-1 column from `fxSmokeFramesB` or `fxFlameFrames` (6.2) at depth 8.
- Optionally add a ring of 6 to 8 type-1 flames (6.3) for the hill's edge.
- Everything is coloured through `tinted` and +3..+5.
- If an exact outline is wanted, use own primitives (section 8).

## 8. Own primitives with the game's camera

### 8.1 What is current during the draw (CONFIRMED, `rt_main.c`, `rt_buffers.h`)

| Item | Where |
|---|---|
| Frame record (0x80 bytes) | the first argument of `fxSpritesDraw`, `streaksDraw`, `debrisDraw` and `renderScene`. View 0 = `*(80180EE4)`, split view 1 = `*(80180EE8)`, mirror = `*(80180EEC)`. `renderGetCurrentFrame` 800D1A20 returns view 0's |
| OT | record +0. 4096 entries, `ClearOTagR(ot, 4096)`, `DrawOTag(ot + 4095)`. Index 0 is drawn last (in front) |
| Packet cursor and end | record +8 and +0xC. Allocate with `if (cur + size >= end) skip; cur += size;` as retail does |
| Packet budget | 0x17CF0 bytes **per frame, shared by both views**: view 1 and the mirror start at view 0's cursor. The level emitter stops silently when it runs out, so keep markers small (a 12-line box is 192 bytes; one retail sprite takes 160) |
| Camera | `viewGetEyeTrans(view)` 80156694 = `&viewEyePositions[view]` (801A51A8, 3 x s32). `viewGetViewMatrix(view)` 801566B0 = `&viewMatrices[view]` (801A51C0, MATRIX). The matrix already includes the 0.9 axis scale and the mirror's x flip. Its `t` is `view · eye / 4096` (LIKELY, from `mathMulMatVec2` in `viewProc`) |
| Projection | H = `viewModeTable[viewModes[view]]` +0x08 (`viewGetProjScaleX` 801555FC; 228 / 138 / 91). Hy = +0x0C (`viewGetProjScaleY` 80155634, equal to H). Centre = w/2, h/2 (`viewGetCentre` 80156614). The draw environment adds the view's x, y offset |
| Mode | `viewGetMode(view)` 801562B8: 4 = the mirror (skip it if the marker is not wanted there) |

### 8.2 Projection

Use the game's own software projection, the one sprites, streaks and arcs use (CONFIRMED,
`fxSpritesDraw`, `weaponArcsDraw` 801084D8):

```
w = p + eye;  v = M·w / 4096  (mathMulMatVec 80153FE4, s32 safe)
if v.z < 8 → behind / too near
sx = cx + v.x·H / v.z;  sy = cy + v.y·Hy / v.z
OT index = v.z >> 3 (clamp 1..4095), or 0 to draw on top
```

### 8.3 RotTransPers (LIKELY usable, with care)

- `viewProc(view)` sets the GTE H, OFX and OFY. Nothing else writes them during a match
  (`WIDESCREEN.md`).
- The GTE **rotation and translation** are whatever the last drawn object left: `renderScene`
  loads each object's composed matrix (`ctc2` around 80152C5C), and `fxDebrisDraw` sets its own.
  So reload both before use:

```c
MATRIX *m = viewGetViewMatrix(view);
SetRotMatrix(m);      /* 80140410 */
SetTransMatrix(m);    /* 801404A0; t = view·eye */
SVECTOR s = { x, y, z };          /* positions must fit s16; arenas reach about ±15300 (CONFIRMED max seen) */
otz = RotTransPers(&s, &sxy, &p, &flag);   /* 801404C4; returns SZ/4, so sprite-style index = otz >> 1 */
```

The software path in 8.2 has no s16 limit and no GTE state to restore, so prefer it.

### 8.4 Hook point

- **Right after `fxSpritesDraw(rec, view)` returns** (80110BE4). It is called exactly once per
  drawn view:
  - view 0 (#12);
  - split view 1 (after `debrisDraw`);
  - the mirror.
- It always comes after that view's `viewProc` and before its `renderScene`. Order within the OT
  does not matter except inside one entry.
- `streaksDraw` (800D8230) also works as a hook, but it carries update ticks
  (`RENDER_ANSWERS.md` R3 (d)).
- Split screen: do it per view with that view's record and index. The two views have separate
  OTs but one packet buffer.
- 60 fps: the port already re-runs `fxSpritesDraw` per redraw with a reset cursor, so a hook
  there redraws with the interpolated camera for free.

### 8.5 Sketch: a wire box (LINE_F2) and a translucent cap (POLY_F4)

```c
void kothDraw(struct TM2BufferInitRecord *rec, int view, const s32 c[3], s32 r, s32 h,
              u8 R, u8 G, u8 B, int abr)
{
    u32 *ot = (u32 *)rec->field_0;
    s32 *eye = viewGetEyeTrans(view);
    s16 *M = (s16 *)viewGetViewMatrix(view);
    s32 H = viewGetProjScaleX(view), Hy = viewGetProjScaleY(view), cx, cy, i;
    s32 sx[8], sy[8], sz[8];
    static const u8 E[12][2] = {{0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7}};

    viewGetCentre(view, &cx, &cy);
    for (i = 0; i < 8; i++) {
        s32 w[3], v[3];
        w[0] = c[0] + ((i & 1) ? r : -r) + eye[0];
        w[1] = c[1] + ((i & 2) ? r : -r) + eye[1];
        w[2] = c[2] + ((i & 4) ? h : 0) + eye[2];
        mathMulMatVec(M, w, v);
        sz[i] = v[2] < 8 ? 0 : v[2];                 /* no near clipping: drop such edges */
        if (sz[i]) { sx[i] = cx + v[0] * H / v[2]; sy[i] = cy + v[1] * Hy / v[2]; }
    }
    for (i = 0; i < 12; i++) {
        s32 a = E[i][0], b = E[i][1], z;
        LINE_F2 *l;
        if (!sz[a] || !sz[b]) continue;
        if ((u32)(sx[a] + 1024) >= 2048 || (u32)(sx[b] + 1024) >= 2048) continue;   /* GPU range */
        if ((u32)(sy[a] + 1024) >= 2048 || (u32)(sy[b] + 1024) >= 2048) continue;
        if (rec->field_8 + sizeof(LINE_F2) >= rec->field_C) return;
        l = (LINE_F2 *)rec->field_8; rec->field_8 += sizeof(LINE_F2);
        setLineF2(l); setRGB0(l, R, G, B);
        setXY2(l, sx[a], sy[a], sx[b], sy[b]);
        z = (sz[a] > sz[b] ? sz[a] : sz[b]) >> 3;
        if (z > 4095) z = 4095;
        AddPrim(ot + z, l);                          /* or ot + 0 to show through walls */
    }
    /* translucent top: POLY_F4 + DR_MODE. An untextured semi-transparent primitive uses the
       abr of the last texture page the GPU saw, which is random in OT order, so pin it.
       AddPrim inserts at the head of an entry: add the polygon first and the DR_MODE second,
       so the DR_MODE runs first. */
    if (sz[4] && sz[5] && sz[6] && sz[7]
        && rec->field_8 + sizeof(POLY_F4) + sizeof(DR_MODE) < rec->field_C) {
        POLY_F4 *p = (POLY_F4 *)rec->field_8; DR_MODE *d;
        RECT tw = {0, 0, 0, 0};
        s32 z = sz[4] >> 3; if (z > 4095) z = 4095;
        rec->field_8 += sizeof(POLY_F4);
        d = (DR_MODE *)rec->field_8; rec->field_8 += sizeof(DR_MODE);
        setPolyF4(p); setSemiTrans(p, 1); setRGB0(p, R, G, B);
        setXY4(p, sx[4], sy[4], sx[5], sy[5], sx[6], sy[6], sx[7], sy[7]);
        SetDrawMode(d, rec->field_10.dfe, 0, getTPage(0, abr, 0, 0), &tw);   /* 8014215C */
        AddPrim(ot + z, p);
        AddPrim(ot + z, d);
    }
}
/* call: after fxSpritesDraw(rec, view) returns, for each drawn view */
```

- abr 0 gives a 50 % tint, abr 1 a glow, and abr 2 darkens.
- The DR_MODE's page then stays current for later untextured semi-transparent primitives until
  the next textured one, for example the full-screen flash `fxSheetDisplay`. That is minor.
  Add a second DR_MODE with abr 0 after it if it matters.

## 9. Not checked

- Whether the smoke and flame CLUT colours have the STP bit set on every texel. The blend modes
  above apply only to texels that do.
- The exact look of type-1 quads (texture orientation and the 45° rotation trick at 80111150..).
  The geometry was read from the code, not seen.
- The 2.5 sorting effect in retail is inferred from both depth formulas. It was not observed.
- Which id-650 mesh fills `fxFlameFrames` in DISH, FREEWAY and ISLANDS.
- Borrowing the warp object (`worldWarps[2]`): its rotation matrix, its animation and its
  visibility outside a warp.
- The port's own call code, so the ranking in section 5 is by likelihood, not proof.
