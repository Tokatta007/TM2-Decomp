# A car posed for a cutscene (`TM2_PORT_CAR_POSE`, PC port request 108)

The port's Versus Tournament podium stands the top three cars on blocks the level does not have, while the
simulation is held. Moving `motion.pos` or an object's matrix moves the body only: the wheels, the shadow and Dark
Tooth's head follow other records, which the step writes. `portCarPose` writes all of them, as a step would leave
them for a car standing still.

```c
int portCarPose(int kind, int index, int x, int y, int floor_z, int yaw);
```

- `kind` 0: human record `index` (`humanCars`, below `TM2_MAX_HUMANS`); 1: computer record `index` (`aiCars`,
  below `TM2_MAX_AI_CARS`). Returns 1, or 0 (nothing written) for anything else.
- The car at (x, y), facing `yaw` (`motion.rot[2]`, any value: `CarRotUpdate` bounds it), level, its wheels on a
  flat floor at `floor_z` (given, not probed; -32,768 to 32,767: the wheels keep it in 16 bits).
- Build with `--port-define TM2_PORT_CAR_POSE` (needs `TM2_PORT_CAR_SPECIALS`, for a Dark Tooth's head by his car
  key). A `.port_text` function in `port_variant_arrays.c`; nothing calls it but the port.
- Call it between steps: during a hold, or in the port's own frame with the RAM put back after. Calling it every
  frame writes the same words each time.

## What it writes

| Part | Words | As |
|---|---|---|
| motion | `steer`, `vel0`/`speed`/`vel2`, the spins, `vel[3]`, `body_spin`, `fE8` 0; `rot` = `body_rot` = (0, 0, yaw); `pos` = (x, y, floor_z + `ride_height`) | still, level |
| human only | `speed_change` (+0x08C) 0 | `carBodySwayUpdate`'s pitch lean |
| aim | the shake (`shake_level`, `shake`, `shake_count`, `shake_phase`, `shake_amp_x/y`), the leans (`lean_*`, `wheel_*`, `pitch`, `f32`) 0 | `CarRotUpdate` adds the leans to `body_m` |
| matrices | `CarRotUpdate(car, human)`: `rot_m`, `body_m`, `body_rot_m` (all three the same, level) | the link's own function |
| wheels | `move` 0; each wheel's `corner` = `rot_m` (transposed) times (±`track_width`/2, `axle_front` or `axle_rear`, -`ride_height`), `pos` = `motion.pos` + `corner` (its z = floor_z); `h`, `level`, `f1C`, `land_z`, `f26` = floor_z; sway, shake, crush, fall and bounce words 0; `on` 1, `touch` 0, `sway_up`, `clamped` 0 | `carUpdateContacts`' corners on a flat floor, settled |
| state | `wheel_in_air`, `all_in_air`, `falling`, `sliding`, `f01`, `f02` 0; `input.turbo` 0 | no skid smoke, on the ground |
| turbo flame | when `state.f1C` is set: cleared and `portCarTurboSetStep(key, 0)` | `carThrottleSteer` / the AI's turbo off |
| shadow | `carBodyPoseUpdate(car, human)`: the key's shadow object level at (x, y, floor_z), +3 for types 0x40, 0x60, 0xA0, 0xB0, 0xC0 | the link's own function |
| car object | `humanCarObjects[index]` / `aiCarObjects[index]`: `rot` (s16), `pos`, `matrix` = `body_m` | `humanCarUpdate` / `aiCarUpdate` |
| Dark Tooth's head | type 0xF0, in his key's selection: `pos` = `motion.pos` + `rot_m` (0, 0, H), H = 240 (-24 for a computer boss beaten: `aiBossDefeated()`); its turn and visibility left as they are | where the boss's 40 % blend (and request 36's exact hover) settles |

It runs no ground probe, physics, AI, weapon, damage, sound or `rand()` (check 21).

## What else the draw reads from the step, and how it stays still

- **Spinning parts and tyres** (`carPartsAnimate`: Hammerhead's, Axel's, Mr. Slam's, Minion's and Dark Tooth's
  turning parts; the tyre texture switch `uaswToggleCarTireState` of the others) move only by `motion.speed`. With
  the speed 0 they keep their last angle and texture, even if a step runs.
- **The wheels have no objects of their own**: they are part of the car's mesh. Nothing draws a steer angle.
- **The engine's wobble** is the body's shake and leans in `aim`, which the pose clears.
- **Dark Tooth's head**: his boss update (an AI record) and request 36's player head (`portCarDtPlayerHead`)
  rock its pitch and turn it every update. During a hold neither runs, so the head stays as posed. If steps run,
  they move it.
- **Smoke, flames and sparks already spawned** are free effects. They stay in the world and fade out by themselves.
- **The chase camera** is updated in `worldUpdate`, not in the car's step. The pose does not touch it.

## Not done

- Visibility: the car, the shadow and the head are shown or hidden as they were. A dead or parked car stays as it
  was (set the object's `visible` word yourself if you pose one).
- A car in the middle of a barrel roll or a crash keeps those counters. The pose writes the angles and wheels, but
  if a step ran it would continue the roll. During a hold nothing does.

## Checked

`check_custom.py` check 21: `portCarPose` in the MIPS interpreter (the matrix products as Python stubs, the
link's `RotMatrixYXZ`, `CarRotUpdate` and `carBodyPoseUpdate`). The runs use human record 1 and computer record 3
filled with junk: a plain car, a raised-shadow car, Dark Tooth (beaten and not), turbo on and off, four headings.
Checked: everything in the table above, the turbo flame turned off exactly once, no `rand()` or `soundPlay`, and an
index out of range writes nothing.

## Each car's special sound (request 108's addendum)

```c
int portCarSpecialSound(int car);
int portCarSpecialSoundHeld(int code);
```

Built in the same block, with the same define, as `portCarPose`. `portCarSpecialSound` returns `soundPlay`'s first
argument for the special of car number `car` as its firing plays it, or -1 for no car. It returns the sound only:
nothing is fired, played or written. The port plays it with `soundPlay(code, 0, 0, 99)`.

| Car | Number | Code | Where the game plays it |
|---|---|---|---|
| Hammerhead | 0 | 0x25 | `carCrashStart`, as the ram mounts a car (the button only gives the charge back) |
| Outlaw 2 | 1 | 0x27 | `weaponFireSpecial`, when the taser hits |
| Warthog | 2 | 0x29 | `weaponFireSpecial` |
| Mr. Grimm | 3 | 0x2B | `weaponFireSpecial` |
| Grasshopper | 4 | 0x2D | `weaponFireSpecial`, the leap |
| Thumper | 5 | 0x2F (**held**) | `weaponFlameStart`; stopped as the flame ends |
| Spectre | 6 | 0x31 | `weaponFireSpecial` |
| Roadkill | 7 | 0x33 (**held**) | `weaponFireSpecial` (as variant `slot`); stopped as the boomerang ends |
| Twister | 8 | 0x35 | `weaponFireSpecial` (per car with `TM2_PORT_CAR_SOUND` bit 4) |
| Axel | 9 | 0x37 | `weaponFireSpecial` |
| Mr. Slam | 10 | 0x39 | `weaponSlamGrabUpdate` / `weaponSlamSwingUpdate`, as the swing starts (5 on a miss) |
| Shadow | 11 | 0x3B | `weaponFireSpecial`, the launch (the detonation is an explosion's sound) |
| Sweet Tooth | 12 | 0x3D | `weaponFireSpecial` (the same id as the boss's swoop warning) |
| Minion | 13 | 0x0D | `weaponFireSpecial` |
| Dark Tooth | 14 | 0x42 | the swoop: the boss (`aiDarkToothBossUpdate`) and a player's (`portCarDtPlayerSwoop`) |

**Mod cars** (`TM2_PORT_CAR_NUMBERS`, 0x20 + m):
- an unused mod returns -1;
- a used mod returns its base's sound;
- with `TM2_PORT_SPECIAL_PROFILE` and a valid `portCarProfile[m]`, it returns the sound of the template it fires
  (`portSpecialTemplate`'s rule):
  - a borrowable car type: that car's sound;
  - a pickup weapon: its launch sound. Fire 10, homing 11, power 12, napalm and Mega 15, and freeze 14
    (`weaponLaunchProjectile`); remote bomb 0x13 (the placement); ricochet 0x12 (**held**).

  Any other template is refused, and the car keeps its base's sound.

A car's borrowed special (request 35, `portCarSpecialAsSet`) belongs to its slot, not to its number. Ask for that
type's car number.

**Held sounds.** `portCarSpecialSoundHeld(code)` returns 1 for 0x2F, 0x33 and 0x12 (`sndEffectTable` kind 3 or 8, the
same as the turbo, gun and burn loops), and 0 for everything else. A held sound plays until `soundStop(code)`; the game
stops it as the flame, boomerang or bomb ends. Every other special sound is a one-shot (kind 12 or 13, 0.5-1.2 s). To
play a held one in the podium scene, stop it yourself after a second or so.

Checked by `check_custom.py` check 22, in the MIPS interpreter:
- every car's code, and -1 out of range;
- with mods: used and unused mods, profiles valid and not, every pickup template, every borrowable type and
  refused templates;
- nothing written and no sound played;
- `portCarSpecialSoundHeld` against the link's own `sndEffectTable` kinds for ids 0..66.
