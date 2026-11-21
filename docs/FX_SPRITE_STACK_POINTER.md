# The effect sprite that keeps a stack address

Answer to the PC port's request (port repo `docs/DECOMP_REQUESTS.md` section 4, "An effect sprite
that keeps a stack address"; `docs/HYBRID.md` "Open: 3 players drift").

## Summary

- **The writer.** `fxSprites` record 1's `owner_pos` (+0x2C; fxSprites 80185144 + 0x5C) holds
  `&q` from `carContactRespond` 800E3800. It calls `fxSparkOrange(q, 0)` at 800E4024
  (`car_contacts.c:429`), with `q` at `sp+0x40`. This is the spark when a car hits a wall fast.
  It is the only stack-local caller whose frame chain gives exactly 801FE670 (CONFIRMED by
  frame arithmetic, below).
- **The 8 bytes.** The variant's `worldUpdate` (8011A784 in the variant, retail 8011A700) has a
  0xB8 frame. Retail's is 0xB0. Its `#if TM2_MAX_HUMANS > 2` radar loop adds locals. The frame is
  compiled once, so it applies to every player count. Every frame below `worldUpdate` sits 8
  bytes lower: 801FE670 - 8 = 801FE668, as the hybrid shows. (CONFIRMED from the
  `out/TM2_SOURCE_PORT.EXE` prologues.)
- **The pointer is never read, in retail or in the variant.** `owner_pos` is dereferenced only by
  `fxSpritesDraw` 80110BE4, and only when `follow` (+1, the spawn's `flag` argument) is non-zero.
  This spark is spawned with `flag = 0`. Every `flag = 1` spawn in the executable passes a static
  address: a car record's `motion.pos`, an object's position or `mathJitterResult`. So retail has
  **no dead-stack read here**. The stale pointer is a harmless leftover. It stays in the record
  after the spark expires (`frame = -1`) until the slot is reused.
- **So this difference does not cause the drift.** It is the only difference you can see in game
  data because it is the only place where a stack *address* gets stored in game memory. The drift
  is most likely a read of an **uninitialised local** somewhere below `worldUpdate`. That read now
  sees stack contents shifted by 8 bytes. A 3-player-only code path is the likely place to look,
  since 1, 2 and 4 players stay equal although they have the same 8-byte shift.
- **Recommendation:** keep the variant's `worldUpdate` frame at 0xB0. That fixes this pointer *and*
  any hidden stale-stack read below it. Copying the value into the record does not help, because
  nothing reads it (details at the end).

## 1. Who reads `owner_pos` (+0x2C)

| Address | Function | C file:line | Use of +0x2C |
|---|---|---|---|
| 80110918 | `fxSpriteSpawn` | `fx_sprite_spawn.c:231` | stores `pos`; copies `pos[0..2]` once into +20..+28 (z +120 when kind >= 96); stores `flag` into +1 |
| 80110BE4 | `fxSpritesDraw` | `world_collapse.c:79` | `if (r->follow) r->pos = *r->src` on every draw (asm 80110CEC..80110D18, plain `lw`); otherwise not touched |
| 80110A40 | `fxSpritesUpdate` | `car_restart.c:331` | not read (frame, pos += vel, loops) |
| 80112820 | `fxSpritesReset` | `arena_load_register.c:522` | not touched (sets +10, +16, +7, +12) |
| 80110BA8 | `fxSpriteFindFree` | `misc_one_liners.c:474` | reads +10 only |
| 801134C8 | `fxSpritesGet` | `misc_one_liners.c:79` | returns the base; no `jal` caller in retail |

There are no other references to fxSprites + n (`config/US/retail_relocs.tsv`). Callers that keep
the returned record (`weapons.fire_effect`, etc.) write +12 (`loops`/"active") and +0x28, +2..+5,
and never +0x2C or +1. Same finding as `WORLD_MARKERS.md` section 4.2.

## 2. Every spawn with `follow = 1` (the only ones that dereference `pos`)

This was checked on the binary: every `jal` into `fxSpriteSpawn` or one of its 40 wrappers, with `a1`
(the wrappers' flag) traced. The wrappers pass their `a1` through as the 7th argument.

| Call site | Caller | C file:line | Effect | `pos` | Stack? |
|---|---|---|---|---|---|
| 800D7314, 800D75D4 | `projectileHitTest` 800D6FD8 | `weapon_slam_swing.c:74,107` | `fxFlameBurning` | car record `motion.pos` (+1788 / +1852) | no |
| 800E08C8 | `carContactApplyObject` 800E06CC | `car_object_contact.c:222` | `fxFlameBurning` | object `pos` | no |
| 800E0F48, 800E0FB8 | `carContactResolve` 800E0C28 | `car_contact_resolve.c:263,274` | `fxFlameBurning` | `motion.pos` | no |
| 800E7754 | `carWreckUpdate` 800E75F4 | `car_crash.c:305` | `fxFlame10` | `m->pos` (AI record) | no |
| 80107CB4, 80107DF8, 80108138, 801083E0 | `weaponLightningScan` 80107B00 | `weapon_slot_scan.c:73` (macro) | `fxLightningSpark` | `actor->position` | no |
| 8010AB0C | `weaponFlameUpdate` 8010A7A0 | `weapon_flame_update.c:117` | `fxFlameBurning` | `motion->pos` | no |
| 8010C460, 8010C638 | `groundFiresUpdate` 8010C160 | `fx_ground_fires.c:106,124` | `fxFlameBurning` | `motion.pos` | no |
| 8010DFEC | `weaponInvisibilityUpdate` 8010DE54 | `ai_link_node_pick.c:304` | `fxSparkBlue` | `mathJitterPos` result = static `mathJitterResult` | no |
| 801106E0 | `aiDarkToothBossUpdate` 8010FD44 | `ai_dark_tooth_boss.c:152` | `fxFlameBurning` | `darkToothSwoopTarget` (= a human's `motion.pos`) | no |

`mathJitterPos` 8010BDE8 (`ai_link_node_pick.c:229`) returns the static `mathJitterResult`, never
the stack. A follow sprite on it moves whenever another jitter call overwrites the buffer. That is
deterministic.

## 3. Spawns from a stack local (`follow = 0`, pointer stored but never read)

Found on the binary: `a0 = sp+N` (directly, or through an `s` register set from `sp`) at a call to
an fx wrapper. "Implied entry sp" is the `rtMainLoop` entry sp that would put `&local` at 801FE670
through the listed chain. The real value on the tournament path is **801FE810**:

- `start` sets sp = 0x80200000.
- `main` 8012C8DC has a 0x1740 frame.
- Then `shellScreenLoop` (0x20), `screenScrollText` (0x20) and `sessionRun` (0x70) lead to
  `rtMainLoop`.

| Rank | Caller (call site) | C file:line | Effect wrapper(s) | Local | Chain to `rtMainLoop` (frames) | Implied entry sp |
|---|---|---|---|---|---|---|
| **1 (match)** | `carContactRespond` 800E3800 (800E4024) | `car_contacts.c:429` | `fxSparkOrange` | `q[3]` sp+0x40 | `carsPhysicsUpdate` 0x78 (`car_physics.c:147`, AI cars) or `:130` (humans) <- `worldUpdate` 0xB0 (`world_update.c:423`) | **801FE810** |
| - | `carWreckUpdate` 800E75F4 (800E7ACC..800E7CF0) | `car_crash.c:368-414` | `fxExplosion24/48/96`, `fxSmokeA48` | `v[3]` sp+0x18 | via `humanCarTick` / `aiMovementUpdate` | 801FE7D0 / 801FE7F8 |
| - | `UpdateGuns` 80104F70 (801051B8, 80105270) | `update_guns.c:68,80` | `fxExplosionQuiet8` | sp+0x28 | via `weaponTick` | 801FE800+ |
| - | `carApplyDamage` 800E5628 | `car_damage.c:184,275` | `fxSmokePuffBlue`, `fxSmokeC48`, `fxExplosion192`, `fxSmokeA48` | sp+0x50 | several | 801FE7E8+ |
| - | `areaDamage` 8010B5C4 | `area_damage.c:64,80` | `fxExplosion48` | `impact` sp+0x18 | several | 801FE7D0+ |
| - | `bulletsMove` 800D7E30 | `weapon_bullets.c:273-277` | `fxSparkOrange`, `fxSmokeA6`, `fxBulletImpact` | `last` sp+0x28 | `bulletsUpdate` | 801FE6D8 |
| - | `weaponProjectileUpdate` 800DB110 | `projectile_hit.c:83-259` | puffs, `fxExplosion24`, `fxSmokeA24/A48` | `cur` sp+0x48 | `fxRecordsUpdate` | 801FE6F0 |
| - | `weaponFlameUpdate` 8010A7A0 | `weapon_flame_update.c:97` | `fxFlameStreamParticle`, `fxSmokeA6` | `pos` sp+0x18 | `streaksDraw` | 801FE790 |
| - | `carDestroyed` 8011BE10 | `car_destroyed.c:372` | `fxSmokeA24` | `pos` sp+0x18 | `*CarDamageUpdate` | 801FE790/8 |
| - | `wheelEffectEmit` 800F1FA8 | `ai_car_start.c:111,116` | `fxSparkOrange` | sp+0x10 | via `carSuspensionUpdate` | 801FE868+ |

Only `carContactRespond` under `carsPhysicsUpdate <- worldUpdate <- rtMainLoop` fits exactly. It
also fits the 8-byte shift: `worldUpdate` is the only function on that chain whose variant frame
differs (variant frames: `rtMainLoop` 0x20, `worldUpdate` **0xB8**, `carsPhysicsUpdate` 0x78,
`carContactRespond` 0x98, `sessionRun` 0x70, `screenScrollText` 0x20, `shellScreenLoop` 0x20). This
spark is not specific to 3 players. It fires whenever a car's `motion.vel[1]` exceeds
`ticks*380/100*32` during a contact response, for humans and AI alike. Record 1 only means that
record 0 was busy at the time.

Side note (retail, harmless here): in the `default` / case 5 contact, `q[0] = q[1] = 0`, so that
spark appears at world x = y = 0 (`car_contacts.c:411-413`).

## 4. Retail behaviour and what the port should do

- **Retail does not read garbage here.** `fxSparkOrange` passes `flag = 0`, so `fxSpritesDraw` never
  dereferences +0x2C. The value never affects anything. The spawn reads `q` only once, while
  `carContactRespond`'s frame is still live.
- **For equal RAM, keep the variant's `worldUpdate` frame at 0xB0.** For example:
  - reuse `delta`/`target` for the radar loop's values, or
  - move the extra locals to `.port_bss` statics (only `worldUpdate` uses them, and it is not
    re-entrant).

  Then every frame below it lines up with retail again. That fixes this pointer and any real
  stale-stack read below `worldUpdate` that may be what makes the 3-player run drift. This is the
  only change that makes the variant layout-equal to the play build. The play build is retail
  code and stores retail's stack address whatever the overlay does.
- **If the frame cannot be kept**, have the comparator ignore `fxSprites[i] + 0x2C` whenever
  `fxSprites[i] + 1 == 0`. That word is write-only in that state.
  - Changing `fxSpriteSpawn` in the overlay would not help. Storing `flag ? pos : 0`, or pointing
    `owner_pos` at the record's own `pos`, makes the variant differ from the play build in every
    mode (1-2 players included), because the play build keeps retail's stack value.
  - A "copy into the record" is already what happens: `pos` is copied at spawn, and there is no
    follow.
- **Finding the real drift** (not answered by this question):
  - Diff the stack page (801FD000..80200000) between the two builds at frame ~2790. A local below
    `worldUpdate` whose contents are equal *after* undoing the 8-byte shift, but which a function
    reads before writing, is the culprit. Fixing the frame size (above) makes this unnecessary.
  - Known uninitialised-local hazards with more than two views: `viewChaseCamera` 80102CC4
    (`camera.c:113`) has no `default` in its split-mode switch. It leaves `cam_goal` unwritten,
    and the pitch bias is uninitialised for a split mode other than 0, 1 or 2
    (`FOUR_PLAYERS.md` 4.4). That matters only if a 3-player view ever runs with such a mode.
  - The host's own view-3/4 sprite draw (`vp_RenderScene`) must honour +1 like `fxSpritesDraw`
    does.

## Follow-up: `worldUpdate`'s frame (request 29)

Done. The variant's N-human radar loop is now `worldRadarPlotHumans(i)` in
`world_update.c`, a static function in `.port_text` (0x80205FA0 at the time, 380 bytes;
0x80209FA0 since request 30 grew `.port_bss`; request 27), compiled only when
`TM2_MAX_HUMANS > 2`. `worldUpdate` calls it for each living human when `humanCarTotal > 2`, and keeps retail's 0xB0
frame in the variant, so every frame below it sits where it does in the play
build. The calls, their order and the `rand()` draws are unchanged. The
variant now publishes a non-empty `port_text.bin`, which the port must load
at the `.port_text` address listed in `out/TM2_SOURCE_PORT.json`.
