/* AI car driver (800E7EDC): the per-frame update of one AI car record
 * (aiCars[i], struct TM2AIRecord in cars.h). Order:
 *   respawn request (stats.f03) -> pick a spawn point; rescale velocity and
 *   spin when the frame time changes; a countdown (stats.freeze_timer) that ends an
 *   effect; the AI proper (aiUpdateTargetRange.. aiWeaponsUpdate) while the car is
 *   active; steering/movement (aiMovementUpdate); car type 0xF0 locks onto
 *   the car it targets; carPartsAnimate; the attached special (weapons.slam);
 *   angles and position (CarRotUpdate, carMoveByVelocity) or falling; turn
 *   total; out-of-world checks against the level heights
 *   (worldFallLimitZ / worldFallDeathZ); body pose; deferred respawn;
 *   carUpdateSidewaysFlag / carSkidSmokeUpdate with human = 0. */
#include "semantic/cars.h"
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 41 bit 17 (BALANCE.md 12.5) */
#endif
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
#ifdef TM2_PORT_WRECK_FALL
#include "semantic/port_wreck_fall.h"   /* request 117 */
#endif

extern s32 carNearestRoutePoint(struct TM2AIRecord *, u8 human);
extern void aiPlaceOnRouteNode(struct TM2AIRecord *);
extern void carStatsDerive(struct TM2CarStatsBlock *stats);
extern s32 timerGetFrameRate(void);
#include "semantic/world.h"
extern void aiUpdateTargetRange(struct TM2AIRecord *), aiUpdateGoalBearing(struct TM2AIRecord *), aiUpdateTargetBearing(struct TM2AIRecord *);
extern void aiTryRouteToSlowTarget(struct TM2AIRecord *), aiUpdateMood(struct TM2AIRecord *), aiWeaponsUpdate(struct TM2AIRecord *);
extern void aiMovementUpdate(struct TM2AIRecord *);
extern void weaponSlamGrabUpdate(struct TM2AIRecord *, s32);
#include "semantic/weapon_calls.h"
extern s32 worldFallLimitZ(void *car, s32 human);
extern s32 worldFallDeathZ(void);
#ifdef TM2_PORT_FALL_WHEELS
/* request 107 (RETAIL_BUGS 40): the pit's death floor measured from the wheels (the body's height less its ride
   height), so every car dies at the same floor; retail measures the body, and Dark Tooth's (ride height 116) stays
   above 80 on a pit floor where every other car dies */
#define FALL_Z(car) ((car)->motion.pos[2] - (car)->stats.ride_height)
#else
#define FALL_Z(car) ((car)->motion.pos[2])
#endif
extern void carBodyPoseUpdate(void *car, s32 human);

/* @800E7EDC */
void __attribute__((section(".text_800E7EDC"))) aiCarDrive(struct TM2AIRecord *car)
{
    u8 th;
    s32 heading, t, k;
    struct TM2CarMotion *tm;
    void *tgt;      /* TM2HumanRecord when th, else TM2AIRecord */

    if (car->stats.f03) {
        car->stats.spawn_node = carNearestRoutePoint(car, 0);
        aiPlaceOnRouteNode(car);
        car->stats.f03 = 0;
    }
    heading = car->motion.rot[2];
    carStatsDerive(&car->stats);
    t = timerGetFrameRate();
    if (car->stats.fps != t) {
        car->motion.speed = car->motion.speed * car->stats.fps / t;
        car->motion.steer = car->motion.steer * car->stats.fps / t;
        car->motion.yaw_rate = car->motion.yaw_rate * car->stats.fps / t;
        car->stats.fps = t;
        /* k assigned inside the expression: keeps fold from reassociating
         * the *19 and puts the cruise_speed load first, as retail does. */
        car->stats.cruise_cap = car->stats.cruise_speed * (k = (s16)timerGetFrameTicks() * 19) / 100 * (car->stats.speed_mult << 5) / 100;
    }
    if (car->state.frozen) {
#ifdef TM2_PORT_BALANCE
        portBalanceFrozenTick(car, 0);   /* request 41 bit 17 (BALANCE.md 12.5): the freeze chain's clock */
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
    car->contact.f0E = 0;
    car->state.weapon_fired = 0;
    if (car->f000 && !car->state.dead) {
        aiUpdateTargetRange(car);
        aiUpdateGoalBearing(car);
        aiUpdateTargetBearing(car);
        if (!car->drive.link_pending)
            aiTryRouteToSlowTarget(car);
        aiUpdateMood(car);
        aiWeaponsUpdate(car);
    } else if (car->state.dead)
        car->target_dist = 0;
    aiMovementUpdate(car);
    if (car->type == 0xF0) {
        tgt = aiGetTargetCar(car->index, &th);
        if (th)
            tm = &((struct TM2HumanRecord *)tgt)->motion;
        else
            tm = &((struct TM2AIRecord *)tgt)->motion;
        if (car->f039 && tm->speed < car->stats.steer_boost_speed) {
            carStatsSetTopSpeed(&car->stats, 25);
            car->state.f05 = 1;
            if (car->drive.f474 <= 0
                && car->motion.speed / 32 * 100 / ((s16)timerGetFrameTicks() * 19) < 30) {
                if (!(car->wheels.wheel[0].depth < 16 || car->wheels.wheel[1].depth < 16 || car->wheels.wheel[2].depth < 16 || car->wheels.wheel[3].depth < 16))
                    carSpinOut(car, 0, 11, 1);
                else if (tm->pos[1] >= -9759)
                    car->motion.rot[2] = mathBlendAngle(car->motion.rot[2], car->f022, 95);
            }
        } else if (car->state.f05)
            car->state.f05 = 0;
    }
#ifdef TM2_PORT_CAR_GROUPS
    carPartsAnimate(TM2_CAR_KEY(car->slot, car->type), car->motion.speed);   /* request 32 */
#else
    carPartsAnimate(car->type, car->motion.speed);
#endif
    if (car->weapons.slam) {
        if (car->weapons.slam->active) {
            if (!car->weapons.slam->grab && !car->weapons.slam->swing)
                car->weapons.slam->active = 0;
#ifdef TM2_PORT_CAR_SPECIALS
            else {
                /* request 33-2a (CAR_GROUPS.md 10): this Mr. Slam's own arm and claw (its car key) */
                s32 special = portCarSpecialSelect(TM2_CAR_KEY(car->slot, car->type));

                if (car->weapons.slam->swing)
                    weaponSlamSwingUpdate(car, 0);
                else
                    weaponSlamGrabUpdate(car, 0);
                portCarSpecialRestore(special);
            }
#else
            else if (car->weapons.slam->swing)
                weaponSlamSwingUpdate(car, 0);
            else
                weaponSlamGrabUpdate(car, 0);
#endif
        } else if (car->weapons.slam->spin_pending) {
            carSpinOutById(car->weapons.slam->spin_target, car->motion.rot[2], 1);
            car->weapons.slam->spin_target = -1;
            car->weapons.slam->spin_pending = 0;
        }
    }
    if (car->f000) {
        CarRotUpdate(car, 0);
        carMoveByVelocity(car, 0);
    } else
        car->motion.pos[2] += car->stats.ride_height;
    car->drive.turned += __builtin_abs(heading - car->motion.rot[2]);
    if (!car->state.falling) {
        if (car->motion.pos[2] < worldFallLimitZ(car, 0))
            car->state.falling = 1;
    } else if (FALL_Z(car) < worldFallDeathZ())
        car->stats.hp = -1;
    else if (worldFallLimitZ(car, 0) < car->motion.pos[2])
        car->state.falling = 0;
#ifdef TM2_PORT_WRECK_FALL
    portWreckFall(car, 0);   /* request 117: a falling wreck hidden with its splash, or exploding at the floor */
#endif
    carBodyPoseUpdate(car, 0);
    if (car->stats.restart_pending) {
        aiCarStart(car, car->type, 1);
        car->stats.restart_pending = 0;
    }
    carUpdateSidewaysFlag(car, 0);
    carSkidSmokeUpdate(car, 0);
}
