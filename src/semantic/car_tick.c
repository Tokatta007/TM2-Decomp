#include "semantic/cars.h"
#ifdef TM2_PORT_MOD_ARENA
#include "semantic/port_mod_arena.h"   /* request 114 */
#endif
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 41 bit 17 (BALANCE.md 12.5) */
#endif
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
#include "semantic/car_numbers.h"   /* request 70: TM2_CAR_DEFAULT_TOP_SPEED (a mod car's own top speed) */
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/dt_player.h"   /* request 36 */
#endif
#ifdef TM2_PORT_WRECK_FALL
#include "semantic/port_wreck_fall.h"   /* request 117 */
#endif

extern void carStatsDerive(struct TM2CarStatsBlock *stats);
extern s32 timerGetFrameRate(void);
#include "semantic/world.h"
extern void humanCarSlideUpdate(void *car);
extern void carUpdateContacts(void *car, s32 side);
extern void carWheelsUpdate(void *car, s32 side);
extern void carSpinOutUpdate(void *car, u8 side);
extern s32 gameGetMode(void);
extern void carParisSinkZonesUpdate(void *car, u8 side);
extern void carCrushContactUpdate(void *car, s32 side);
extern void carBodySwayUpdate(void *car, u8 side);
#include "semantic/weapon_calls.h"
extern void weaponSlamGrabUpdate(void *car, s32 side);
extern void carBodyAttitudeUpdate(void *car, s32 side);
extern void carWreckUpdate(void *car, s32 side);
extern void carBodyFollowCar(void *motion, void *flags);
extern s32 worldFallLimitZ(void *car, s32 side);
extern s32 worldFallDeathZ(void);
#ifdef TM2_PORT_FALL_WHEELS
/* request 107 (RETAIL_BUGS 40): the pit's death floor measured from the wheels (the body's height less its ride
   height), so every car dies at the same floor; retail measures the body, and Dark Tooth's (ride height 116) stays
   above 80 on a pit floor where every other car dies */
#define FALL_Z(car) ((car)->motion.pos[2] - (car)->stats.ride_height)
#else
#define FALL_Z(car) ((car)->motion.pos[2])
#endif
extern void carBodyPoseUpdate(void *car, s32 side);
extern s32 viewGetSplitMode(void);
extern void hudMessagePost(s32 sound, s32 value);
extern s32 gameGetLevel(void);
#include "semantic/input.h"
extern void carThrottleSteer(void *car, unsigned char human);
extern s32 carDefaultTopSpeed(s32 object);
#include "semantic/port_humans.h"   /* H3: TM2_HUMAN_MESSAGE (retail's hudMessagePost without the define) */
#ifdef TM2_PORT_HUMANS
/* HUMANS16_PLAN.md H2: the cheat combos read human `player`'s pad 0/1 by its local slot (retail's calls) or its
 * input word */
#define CHEAT_COMBO(n, player) portHumanCombo(n, player)
#define CHEAT_ANY_LATCHED(player) portHumanAnyLatched(player)
/* H1 review: retail's split mode is non-zero exactly in game modes 2 and 4 (sessionRun sets it from the mode; the
 * hotkey only toggles 1 <-> 2), so the simulation tests the mode: a split mode the port sets per machine from its
 * local players cannot desync two machines */
#define REGEN_SPLIT() (gameGetMode() == 2 || gameGetMode() == 4)
#else
#define CHEAT_COMBO(n, player) padComboHeld(n, player)
#define CHEAT_ANY_LATCHED(player) padAnyLatched(player)
#ifdef TM2_PORT_ROLLBACK
/* online, ONLINE_ROLLBACK.md 18 (bit 10 LOCAL_VIEWS): the match's split mode, not this machine's (the one game-state
 * reader of the split screen in the step); without the bit portRollbackViewSplit returns viewGetSplitMode() */
#include "semantic/port_rollback.h"
#define REGEN_SPLIT() (portRollbackViewSplit() != 0)
#else
#define REGEN_SPLIT() (viewGetSplitMode() != 0)
#endif
#endif


/* The start-area switch's jump table sits at its retail address. */
#pragma tm2_rdata .tm2_800CEAE0

/* @800DC88C. Per-frame update for the first car/controller side. */
void __attribute__((section(".text_800DC88C")))
humanCarTick(struct TM2HumanRecord *car)
{
    s32 scale;
    s32 old_speed;
    s32 mode;
    s32 z;
    s32 x;
    s32 x_end;
    s32 z_end;
    s32 changed;

    carStatsDerive(&car->stats);
    scale = timerGetFrameRate();
    if (car->stats.fps != scale) {
        car->motion.speed = (car->motion.speed * car->stats.fps) / scale;
        car->motion.steer = (car->motion.steer * car->stats.fps) / scale;
        car->motion.yaw_rate = (car->motion.yaw_rate * car->stats.fps) / scale;
        car->stats.fps = scale;
    }
    car->contact.f0E = 0;
    car->state.weapon_fired = 0;
    if (car->state.frozen != 0) {
#ifdef TM2_PORT_BALANCE
        portBalanceFrozenTick(car, 1);   /* request 41 bit 17 (BALANCE.md 12.5): the freeze chain's clock */
#endif
        car->stats.freeze_timer -= timerGetFrameTicks();
        if (car->stats.freeze_timer <= 0) {
            car->state.frozen = 0;
            if (car->stats.frozen_by_missile == 1) {
#ifdef TM2_PORT_CAR_GROUPS
                carFreezeSetState(TM2_CAR_KEY(car->slot, car->type), 3);   /* request 32 */
#else
                carFreezeSetState(car->type, 3);
#endif
                car->stats.frozen_by_missile = 0;
            }
        }
    }

    old_speed = car->motion.speed;
    carThrottleSteer(car,1);
    humanCarSlideUpdate(car);
    car->speed_change = car->motion.speed - old_speed;
    GetCurTics(); carUpdateContacts(car,1);
    GetCurTics(); GetCurTics(); carWheelsUpdate(car,1);
    GetCurTics(); carSpinOutUpdate(car,1);
    if (gameGetMode() != 4 && gameGetMode() != 2)
        carParisSinkZonesUpdate(car,1);
    if (car->state.crush_contact != 0)
        carCrushContactUpdate(car,1);
    carBodySwayUpdate(car,1);

#define RECORD (car->weapons.slam)
    if (RECORD != 0) {
        if (RECORD->active != 0) {
#ifdef TM2_PORT_CAR_SPECIALS
            /* request 33-2a (CAR_GROUPS.md 10): this Mr. Slam's own arm and claw (its car key; the first
               Mr. Slam: retail's); weaponSlamCarryTarget lifts the held car by that arm */
            s32 special = portCarSpecialSelect(TM2_CAR_KEY(car->slot, car->type));
#endif
            if (RECORD->swing != 0)
                weaponSlamSwingUpdate(car,1);
            if (RECORD->grab != 0)
                weaponSlamGrabUpdate(car,1);
#ifdef TM2_PORT_CAR_SPECIALS
            portCarSpecialRestore(special);
#endif
        } else if (RECORD->spin_pending != 0) {
            carSpinOutById(RECORD->spin_target,car->motion.rot[2],1);
            RECORD->spin_target = -1;
            RECORD->spin_pending = 0;
        }
    }
    carBodyAttitudeUpdate(car,1);
    if (car->state.dead != 0)
        carWreckUpdate(car,1);
#ifdef TM2_PORT_CAR_GROUPS
    carPartsAnimate(TM2_CAR_KEY(car->slot, car->type), car->motion.speed);   /* request 32 */
#else
    carPartsAnimate(car->type,car->motion.speed);
#endif
    if (car->state.f01 == 0)
        carBodyFollowCar(&car->motion,&car->state);
    CarRotUpdate(car,1);
    carMoveByVelocity(car,1);

    if (car->state.falling == 0) {
        if (car->motion.pos[2] < worldFallLimitZ(car,1))
            car->state.falling = 1;
    } else if (FALL_Z(car) < worldFallDeathZ()) {
        car->stats.hp = -1;
    } else if (worldFallLimitZ(car,1) < car->motion.pos[2]) {
        car->state.falling = 0;
    }
#ifdef TM2_PORT_WRECK_FALL
    portWreckFall(car, 1);   /* request 117: a falling wreck hidden with its splash, or exploding at the floor */
#endif
    carBodyPoseUpdate(car,1);
    weaponTick(car,1);
#ifdef TM2_PORT_DT_PLAYER
    if (car->type == 0xF0)
        portCarDtPlayerHead(car);   /* request 36 (DARK_TOOTH_PLAYER.md): his head over the truck, the swoop */
#endif
    carUpdateSidewaysFlag(car,1);
    carSkidSmokeUpdate(car,1);
    if (car->stats.restart_pending != 0) {
        carRestart(car,car->type,1);
        car->stats.restart_pending = 0;
    }

    if (car->state.riding != 0) {
        if (car->stats.regen_clock < 0)
            car->stats.regen_clock = 0;
        car->stats.regen_clock += timerGetFrameTicks();
        if (car->stats.regen_clock >= 0x169) {
            car->stats.regen_steps++;
            if (car->stats.regen_steps < 5) {
                changed = 0;
                if (car->weapons.infinite == 0 &&
                    (car->weapons.capacity-car->weapons.held > 0 || REGEN_SPLIT())) {
                    car->weapons.ammo[car->weapons.selected]++;
                    car->state.hud_notice = 1;
                    TM2_HUMAN_MESSAGE(car->player, 0x23, 0);   /* H3: the player's own message */
                    changed = 1;
                }
                if (car->stats.damageable != 0 && car->stats.hp < car->stats.max_hp) {
                    car->stats.hp += 5;
                    if (car->stats.max_hp < car->stats.hp) {
                        car->stats.hp = car->stats.max_hp;
                        if (changed == 0) {
                            TM2_HUMAN_MESSAGE(car->player, 0x23, 0);
                            car->state.hud_notice = 1;
                        }
                    }
                }
            }
            car->stats.regen_clock = 0;
        }
    } else {
        if (car->stats.regen_clock > 0)
            car->stats.regen_clock = 0;
        car->stats.regen_clock -= timerGetFrameTicks();
        if (car->stats.regen_clock < -0xE10) {
            car->stats.regen_clock = 0;
            car->stats.regen_steps = 0;
        }
    }

#ifdef TM2_PORT_MOD_ARENA
    /* request 114 (MOD_ARENAS.md): a mod arena's spot, or none, from portArenaRules.heal; 0 is retail's */
    if (car->stats.heal_spot_used == 0 && car->motion.speed == 0 &&
        (mode = portArenaHealSpot(&x, &z)) != TM2_MOD_HEAL_NONE) {
        if (mode == TM2_MOD_HEAL_RETAIL)
#else
    if (car->stats.heal_spot_used == 0 && car->motion.speed == 0) {
#endif
        {
        mode = gameGetLevel();
        switch (mode) {
        case 1: x = 0x1FA8; z = 0xD88; break;
        case 2: x = -0x910; z = -0x3D8; break;
        case 3: x = -0xAD0; z = -0xBC0; break;
        case 4: x = 0xA8; z = 0x2DF0; break;
        case 5: x = 0xC0; z = 0x320; break;
        case 6: x = 0x39E8; z = -0xA90; break;
        case 7: x = -0x490; z = -0x1838; break;
        case 8: x = -0xAA0; z = 0xA60; break;
        case 10: x = 0x320; z = -0xA70; break;
        case 11: x = 0x5860; z = 0x2D40; break;
        case 12:
        default: x = 0x2A30; z = 0x7D0; break;
        }
        }
        x_end = x + 0x40;
        z_end = z + 0x40;
        if (car->motion.pos[0] >= x && car->motion.pos[0] <= x_end &&
            car->motion.pos[1] >= z && car->motion.pos[1] <= z_end) {
            car->stats.hp = car->stats.max_hp;
            car->stats.heal_spot_used = 1;
        }
    }

#ifdef TM2_PORT_HUMANS
    if (portHumanCheatsOff())
        return;     /* H2 (request 37 answer 13): portHumanOptions bit 3, the cheat combos off */
#endif
    if (car->stats.cheat_a == 0 && car->stats.cheat_b == 0) {
        if (CHEAT_COMBO(2,car->player) & 0xFF) {
            car->stats.cheat_a = 1;
            car->stats.cheat_released = 0;
            return;
        }
        if (CHEAT_COMBO(4,car->player) & 0xFF) {
            car->stats.cheat_b = 1;
            car->stats.cheat_released = 0;
        }
    } else {
        if (car->stats.cheat_released == 0) {
            if (CHEAT_ANY_LATCHED(car->player) & 0xFF) {
                car->stats.cheat_timer = 0;
                return;
            }
            car->stats.cheat_released = 1;
            return;
        }
        if (car->stats.cheat_a != 0) {
            if (CHEAT_COMBO(3,car->player) & 0xFF) {
                if (car->stats.shield != 0) {
                    car->stats.shield = 0;
                    car->stats.mass = 0x19;
                    carStatsSetTopSpeed(&car->stats,TM2_CAR_DEFAULT_TOP_SPEED(car->slot, car->type));
                } else {
                    car->stats.shield = 1;
                    car->stats.mass = 0x64;
                    car->stats.top_speed += 0x3C;
                    carStatsSetTopSpeed(&car->stats,car->stats.top_speed);
                }
                car->stats.cheat_timer = 10;
            } else if (CHEAT_COMBO(8,car->player) & 0xFF) {
                if (car->stats.hp < car->stats.max_hp)
                    car->stats.hp += 0x32;
                car->stats.cheat_timer = 10;
            } else if (CHEAT_COMBO(9,car->player) & 0xFF) {
                if (car->weapons.turbo < 0x2EE)
                    car->weapons.turbo += 0xFA;
                car->stats.cheat_timer = 10;
            }
        } else if (car->stats.cheat_b != 0 &&
                   (CHEAT_COMBO(5,car->player) & 0xFF)) {
            if (car->weapons.infinite != 0) {
                car->weapons.infinite = 0;
                car->stats.damageable = 1;
            } else {
                car->weapons.infinite = 1;
                car->stats.damageable = 0;
            }
            car->stats.cheat_timer = 10;
        }
        car->stats.cheat_timer++;
        if (car->stats.cheat_timer >= 11) {
            car->stats.cheat_a = 0;
            car->stats.cheat_b = 0;
        }
    }
}
