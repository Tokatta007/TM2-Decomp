/* AI steps (800E8604..): sensing, goals and decisions of one AI car
 * (record aiCars[i]; called from the AI driver, ai_driver.c). */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 35: TM2_SPECIAL_TYPE (f006 = the car slot) */
#include "semantic/car_numbers.h"   /* request 70: TM2_CAR_DEFAULT_TOP_SPEED (a mod car's own top speed) */
#include "semantic/port_balance.h"  /* request 91 item 3 (BALANCE.md 20) */

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Matrix;

struct TM2AI {
    u8 active;          /* +0x000 */
    s8 state;          /* +0x001 */
    u8 pad_002[0x2];
    s16 id;          /* +0x004 */
    s16 f006;          /* +0x006 */
    s16 f008;          /* +0x008 */
    u8 pad_00A[0x4];
    s16 f00E;          /* +0x00E */
    s16 state_clock;          /* +0x010 */
    s16 state_time;          /* +0x012 */
    u8 pad_014[0x2];
    s16 range_near;          /* +0x016 */
    s16 range_1;          /* +0x018 */
    s16 range_2;          /* +0x01A */
    s16 range_3;          /* +0x01C */
    s16 range_far;          /* +0x01E */
    s16 f020;          /* +0x020 */
    s16 f022;          /* +0x022 */
    s16 f024;          /* +0x024 */
    s16 dz;          /* +0x026 */
    s32 dist;          /* +0x028 */
    s32 last_dist;          /* +0x02C */
    s32 type;          /* +0x030 */
    u8 in_range;          /* +0x034 */
    u8 level;          /* +0x035 */
    u8 near;          /* +0x036 */
    u8 r1;          /* +0x037 */
    u8 r2;          /* +0x038 */
    u8 r3;          /* +0x039 */
    u8 r4;          /* +0x03A */
    u8 f03B;          /* +0x03B */
    u8 f03C;          /* +0x03C */
    u8 f03D;          /* +0x03D */
    u8 pad_03E[0x1];
    u8 f03F;          /* +0x03F */
    u8 f040;          /* +0x040 */
    u8 f041;          /* +0x041 */
    u8 pad_042[0x2];
    u8 f044;          /* +0x044 */
    u8 f045;          /* +0x045 */
    u8 f046;          /* +0x046 */
    u8 f047;          /* +0x047 */
    u8 pad_048[0xA];
    u8 f052;          /* +0x052 */
    u8 f053;          /* +0x053 */
    u8 f054;          /* +0x054 */
    u8 pad_055[0x1];
    u8 f056;          /* +0x056 */
    u8 f057;          /* +0x057 */
    u8 pad_058[0x6];
    u8 f05E;          /* +0x05E */
    u8 pad_05F[0xD];
    u8 f06C;          /* +0x06C */
    u8 f06D;          /* +0x06D */
    u8 pad_06E[0x4];
    u8 f072;          /* +0x072 */
    u8 pad_073[0x4];
    u8 hold_dist;          /* +0x077 */
    u8 f078;          /* +0x078 */
    u8 f079;          /* +0x079 */
    u8 pad_07A[0x4];
    u8 f07E;          /* +0x07E */
    u8 pad_07F[0x1];
    u8 f080;          /* +0x080 */
    u8 pad_081[0x2];
    u8 stuck;          /* +0x083 */
    u8 pad_084[0x4];
    u8 firing;          /* +0x088 */
    u8 f089;          /* +0x089 */
    u8 pad_08A[0x6];
    s32 f090;          /* +0x090 */
    u8 pad_094[0x40];
    s32 f0D4;          /* +0x0D4 */
    u8 pad_0D8[0x6];
    s16 f0DE;          /* +0x0DE */
    u8 pad_0E0[0x368];
    u8 f448;          /* +0x448 */
    u8 f449;          /* +0x449 */
    u8 pad_44A[0x8];
    u8 f452;          /* +0x452 */
    u8 f453;          /* +0x453 */
    u8 pad_454[0x1];
    u8 f455;          /* +0x455 */
    u8 f456;          /* +0x456 */
    u8 f457;          /* +0x457 */
    u8 f458;          /* +0x458 */
    u8 f459;          /* +0x459 */
    u8 f45A;          /* +0x45A */
    u8 f45B;          /* +0x45B */
    u8 f45C;          /* +0x45C */
    u8 f45D;          /* +0x45D */
    u8 f45E;          /* +0x45E */
    u8 f45F;          /* +0x45F */
    u8 f460;          /* +0x460 */
    u8 pad_461[0x1];
    s16 f462;          /* +0x462 */
    s16 f464;          /* +0x464 */
    u8 pad_466[0x2];
    s16 f468;          /* +0x468 */
    s16 node;          /* +0x46A */
    s16 away;          /* +0x46C */
    s16 closer;          /* +0x46E */
    u8 pad_470[0x2];
    s16 f472;          /* +0x472 */
    s16 f474;          /* +0x474 */
    s16 backup_time;          /* +0x476 */
    u8 pad_478[0x2];
    s16 f47A;          /* +0x47A */
    u8 pad_47C[0x2];
    s16 turned;          /* +0x47E */
    s16 f480;          /* +0x480 */
    s16 f482;          /* +0x482 */
    s16 f484;          /* +0x484 */
    s16 f486;          /* +0x486 */
    u8 pad_488[0x2];
    s16 f48A;          /* +0x48A */
    u8 pad_48C[0x2];
    s16 goal2_angle;          /* +0x48E */
    s16 f490;          /* +0x490 */
    s16 f492;          /* +0x492 */
    s16 goal_angle;          /* +0x494 */
    u8 pad_496[0x2];
    s32 f498;          /* +0x498 */
    u8 pad_49C[0x10];
    s32 look_ahead;          /* +0x4AC */
    s32 f4B0;          /* +0x4B0 */
    u8 pad_4B4[0x10];
    s32 goal_dist;          /* +0x4C4 */
    s32 goal_dist_prev;          /* +0x4C8 */
    s32 goal2_dist;          /* +0x4CC */
    u8 pad_4D0[0x8];
    s32 f4D8;          /* +0x4D8 */
    u8 pad_4DC[0x8];
    s32 goal[2];          /* +0x4E4 */
    s32 goal2[2];          /* +0x4EC */
    s32 aim[2];          /* +0x4F4 */
    u8 pad_4FC[0x5];
    s8 f501;          /* +0x501 */
    u8 pad_502[0x1E];
    s16 f520;          /* +0x520 */
    u8 pad_522[0x2];
    s16 stuck_time;          /* +0x524 */
    u8 pad_526[0x10];
    s16 max_hp;          /* +0x536 */
    u8 pad_538[0x6];
    s16 f53E;          /* +0x53E */
    u8 pad_540[0xE];
    s16 f54E;          /* +0x54E */
    u8 pad_550[0x4];
    s16 f554;          /* +0x554 */
    u8 pad_556[0xA];
    s16 f560;          /* +0x560 */
    u8 pad_562[0xA];
    s16 f56C;          /* +0x56C */
    u8 pad_56E[0x4];
    s16 f572;          /* +0x572 */
    s16 f574;          /* +0x574 */
    s16 f576;          /* +0x576 */
    s16 hunt_full;          /* +0x578 */
    s16 hunt_low;          /* +0x57A */
    s16 attack_full;          /* +0x57C */
    s16 attack_low;          /* +0x57E */
    s16 hunt_chance;          /* +0x580 */
    s16 hunt_time;          /* +0x582 */
    s16 attack_time;          /* +0x584 */
    s16 flee_time;          /* +0x586 */
    s16 f588;          /* +0x588 */
    u8 pad_58A[0x2];
    s16 pass_chance;          /* +0x58C */
    u8 pad_58E[0xE];
    s32 f59C;          /* +0x59C */
    s32 hp;          /* +0x5A0 */
    u8 pad_5A4[0xC];
    s32 f5B0;          /* +0x5B0 */
    s32 f5B4;          /* +0x5B4 */
    s32 f5B8;          /* +0x5B8 */
    u8 pad_5BC[0x4];
    s32 f5C0;          /* +0x5C0 */
    s32 f5C4;          /* +0x5C4 */
    s32 brake;          /* +0x5C8 */
    u8 pad_5CC[0x4];
    s32 spawn;          /* +0x5D0 */
    u8 pad_5D4[0x148];
    s16 m_f00;          /* +0x71C */
    u8 pad_71E[0x8];
    s16 speed;          /* +0x726 */
    u8 pad_728[0x8];
    s16 spin2;          /* +0x730 */
    u8 pad_732[0xA];
    s32 pos[3];          /* +0x73C */
    u8 pad_748[0x8];
    s32 heading;          /* +0x750 */
    u8 pad_754[0x8];
    s32 f75C;          /* +0x75C */
    u8 pad_760[0xC];
    TM2Matrix rot_m;          /* +0x76C */
    u8 pad_78C[0x89];
    u8 special;          /* +0x815 */
    u8 pad_816[0x18];
    s16 f82E;          /* +0x82E */
    u8 pad_830[0xE];
    s16 f83E;          /* +0x83E */
    u8 pad_840[0x14];
};
typedef char tm2ai_size[(sizeof(struct TM2AI) == 0x854) ? 1 : -1];

typedef struct TM2AI TM2AI;

typedef struct {
    u8 pad_00[0x20];
    s32 pos[3];             /* +0x20 */
} TM2AITargetMotion;

extern void *aiGetTargetCar(s32 id, u8 *human);
extern s32 sub_8013FC44(s32);
extern s32 timerGetFrameTicks(void);
extern void mathMulTransVec(TM2Matrix *, s32 *, s32 *);
extern int ratan2(int, int);
extern void mathBoundSAngle(s16 *);

/* @800E8604: distance and height to the target car; closing/opening
 * timers; range flags from the thresholds +0x16..+0x1E. */
void __attribute__((section(".text_800E8604"))) aiUpdateTargetRange(TM2AI *ai)
{
    u8 th;
    void *tgt;              /* human (th) or AI record */
    TM2AITargetMotion *tm;
    s32 unused;
    s32 dx, dy;

    tgt = aiGetTargetCar(ai->id, &th);
    if (th) {
        unused = (u16)HREC(tgt)->state.x2C.route;
        tm = (TM2AITargetMotion *)&HREC(tgt)->motion;
    } else {
        unused = (u16)DRV(tgt)->route;
        tm = (TM2AITargetMotion *)&AI(tgt)->motion;
    }
    dx = tm->pos[0] - ai->pos[0];
    dy = tm->pos[1] - ai->pos[1];
    ai->dist = sub_8013FC44(dx * dx + dy * dy);
    ai->dz = tm->pos[2] - ai->pos[2];
    unused = ai->f464;
    if (!ai->f040) {
        if (ai->dist > ai->last_dist) {
            ai->away += timerGetFrameTicks();
            ai->closer = 0;
        } else if (ai->dist < ai->last_dist) {
            ai->away = 0;
            ai->closer += timerGetFrameTicks();
        } else {
            ai->away = 0;
            ai->closer = 0;
        }
    }
    if (!ai->hold_dist)
        ai->last_dist = ai->dist;
    if (ai->dist < 4320 || (ai->f455 && ai->f468 > 0)) {
        ai->in_range = 1;
        if (ai->dz < 160)
            ai->level = 1;
        else
            ai->level = 0;
        if (ai->dist < ai->range_near)
            ai->near = 1;
        else
            ai->near = 0;
        if (ai->dist < ai->range_1) {
            ai->r1 = 1;
            ai->r2 = 1;
            ai->r3 = 1;
            ai->r4 = 1;
        } else {
            ai->r1 = 0;
            if (ai->dist < ai->range_2) {
                ai->r2 = 1;
                ai->r3 = 1;
                ai->r4 = 1;
            } else {
                ai->r2 = 0;
                if (ai->dist < ai->range_3) {
                    ai->r3 = 1;
                    ai->r4 = 1;
                } else {
                    ai->r3 = 0;
                    if (ai->dist < ai->range_far)
                        ai->r4 = 1;
                    else
                        ai->r4 = 0;
                }
            }
        }
    } else {
        ai->in_range = 0;
        ai->level = 0;
        ai->near = 0;
        ai->r1 = 0;
        ai->r2 = 0;
        ai->r3 = 0;
        ai->r4 = 0;
    }
}

/* @800E88C8: bearing and distance to the current goal point, looking
 * look_ahead units ahead of the car. */
void __attribute__((section(".text_800E88C8"))) aiUpdateGoalBearing(TM2AI *ai)
{
    s32 v[3];
    s32 out[4];
    s32 d[3];

    v[0] = ai->look_ahead;
    v[1] = 0;
    v[2] = 0;
    mathMulTransVec(&ai->rot_m, v, out);
    d[0] = ai->goal[0] + out[0] - ai->pos[0];
    d[1] = ai->goal[1] + out[1] - ai->pos[1];
    ai->goal_dist_prev = ai->goal_dist;
    ai->goal_dist = sub_8013FC44(d[0] * d[0] + d[1] * d[1]);
    if (ai->f03B || ai->f472 < 10) {
        ai->goal_dist_prev = ai->goal_dist;
        ai->f45B = 0;
    }
    if (!d[1])
        d[1] = 1;
    ai->goal_angle = ratan2(d[0], d[1]);
    mathBoundSAngle(&ai->goal_angle);
}

/* @800E89E8: bearing and distance to the second goal point. */
void __attribute__((section(".text_800E89E8"))) aiUpdateGoal2Bearing(TM2AI *ai)
{
    s32 d[3];

    d[0] = ai->goal2[0] - ai->pos[0];
    d[1] = ai->goal2[1] - ai->pos[1];
    ai->goal2_dist = sub_8013FC44(d[0] * d[0] + d[1] * d[1]);
    if (!d[1])
        d[1] = 1;
    ai->goal2_angle = ratan2(d[0], d[1]);
    mathBoundSAngle(&ai->goal2_angle);
}

extern int rand(void);

/* @800E8A84: mood state machine (+0x01: 0 idle, 1 hunt, 2 attack,
 * 3 flee). When a state's time is up, roll the next one; the chances
 * blend between "hurt" and "well" values by the health percentage. */
void __attribute__((section(".text_800E8A84"))) aiUpdateMood(TM2AI *ai)
{
    s32 health, rest, roll, lim;
    s16 t;

    if (ai->state < 2 && ai->f588 < 0) {
        ai->state = 2;
        ai->state_time = ai->attack_time + ai->attack_time * (rand() & 3);
        ai->state_clock = 0;
        ai->f00E = 0;
    }
    ai->state_clock += timerGetFrameTicks();
    if (ai->state_clock > ai->state_time) {
        health = ai->hp * 100 / ai->max_hp;
        if (health > 100)
            health = 100;
        rest = 100 - health;
        ai->hunt_chance = health * ai->hunt_full / 100 + rest * ai->hunt_low / 100;
        roll = rand() % 100;
        if (roll < ai->hunt_chance) {
            ai->state = 1;
            t = ai->hunt_time;
        } else if (roll < health * ai->attack_full / 100 + rest * ai->attack_low / 100 + ai->hunt_chance) {
            ai->state = 2;
            t = ai->attack_time;
        } else {
            ai->state = 3;
            t = ai->flee_time;
        }
        ai->state_time = t;
        ai->state_clock = 0;
        ai->f00E = 0;
    } else if (!ai->state) {
        lim = 0;
        if (ai->dist < 1201) {
            lim = 0x800;
            if (ai->dist > 0)
                lim = 136000 / ai->dist;
        }
        if (__builtin_abs(ai->f024) < lim) {
            ai->state = 2;
            ai->state_time = ai->attack_time;
            ai->f00E = 0;
            ai->state_clock = 0;
        }
    }
}

extern s32 GsGetWorkBase_0(void);
extern void carStatsDerive(void *stats);

/* @800E8DFC: count frames spent stuck against something; after 25 the
 * car loses health now and then, after 7 it is flagged stuck. */
void __attribute__((section(".text_800E8DFC"))) aiUpdateStuck(TM2AI *ai)
{
    ai->stuck = 0;
    if (ai->f474) {
        ++ai->stuck_time;
        if (ai->stuck_time >= 25) {
            if ((GsGetWorkBase_0() & 5) == 1)
                ai->hp -= 1;
        } else if (ai->stuck_time >= 7)
            ai->stuck = 1;
    } else {
        ai->stuck_time = 0;
        ai->f45A = 0;
    }
    ai->f474 = 0;
    if (ai->f03D) {
        ai->f03D = 0;
        ai->f008 = 0;
        ai->f572 = ai->f574;
        carStatsDerive(&AI(ai)->stats);
    }
    ai->away = 0;
    ai->closer = 0;
    if (ai->f472 >= 601)
        ++ai->stuck_time;
    else
        ai->f472 = 0;
}

/* Per-level route tables used by the follow-the-route mode. */
typedef struct { s32 x, y; s16 z; u8 pad[0x0D]; s8 route; } TM2AIRouteInfo;
extern TM2AIRouteInfo aiRouteNodes[];
extern u8 aiRouteLinkTargetTable[][35];

extern void aiClearControls(struct TM2CarInput *ctl);
extern void mathBoundAngle(s32 *);
extern void aiMaybeReplanRoute(TM2AI *), aiStartGiveUp(TM2AI *), aiUpdateGiveUp(TM2AI *), aiUpdateStop(TM2AI *);
extern int gameGetLevel(void);
extern u8 trackVehiclesShouldStop(void);
extern void sub_800E9E20(TM2AI *), aiFreeSteer(TM2AI *), aiFollowRoute(TM2AI *), aiSteerToGoal(TM2AI *);
extern void aiStartRouteFollow(TM2AI *);
extern void aiChooseNextNode(TM2AI *, s32);
extern void aiSetSegmentSpeed(TM2AI *, u8 mode);
extern void aiInputFinalize(TM2AI *);
extern void carUpdateContacts(TM2AI *, s32), carWheelsUpdate(TM2AI *, s32);
extern void carThrottleSteer(TM2AI *, u8 human);
extern void carCrushContactUpdate(TM2AI *, s32);
extern void carSpinOutUpdate(TM2AI *, u8 side);
extern void carParisSinkZonesUpdate(TM2AI *, u8 human);
extern void carBodyAttitudeUpdate(TM2AI *, s32), carWreckUpdate(TM2AI *, s32);
extern void carBodySwayUpdate(TM2AI *, u8 side);
extern s32 gameGetMode(void);

/* @800E8F38: AI movement: turn-around and give-up timers, route or free
 * steering, attack moves, then weapons, collisions and effects. */
void __attribute__((section(".text_800E8F38"))) aiMovementUpdate(TM2AI *ai)
{
    s32 a;
    u8 th;
    s32 lim;
    void *tgt;              /* human (th) or AI record */
    s32 x;

    aiClearControls(&AI(ai)->input);
    if (ai->f458) {
        a = ai->heading - ai->f022;
        mathBoundAngle(&a);
        if (__builtin_abs(a) < 512) {
            ai->f03B = 1;
            ai->f472 += timerGetFrameTicks();
            if (ai->f472 >= 301) {
                ai->f458 = 0;
                aiMaybeReplanRoute(ai);
            }
        }
    }
    if (!ai->f06C && ai->active)
        ai->f472 += timerGetFrameTicks();
    if (ai->f03B || ai->f455)
        lim = 3000;
    else
        lim = 600;
    if (lim < ai->f472)
        aiStartGiveUp(ai);
    if (ai->turned >= 6145) {
        if (ai->f03B)
            aiMaybeReplanRoute(ai);
        else {
            ai->f008 = 180;
            ai->f03D = 1;
        }
    }
    if (ai->f03D)
        aiUpdateGiveUp(ai);
    if (ai->f453)
        aiUpdateStop(ai);
    else if (ai->active && !ai->f06C) {
        if (ai->f03B) {
            ai->goal_dist = ai->dist * 2;
            ai->goal_dist_prev = ai->goal_dist;
            ai->f486 = ai->goal_dist / 2;
            ai->f4D8 = ai->goal_dist / 2;
            ai->f48A = ai->f022;
            ai->goal_angle = ai->f022;
            ai->f457 = 0;
        }
        if (ai->r3) {
            a = ai->heading - ai->f022;
            mathBoundAngle(&a);
            if (__builtin_abs(a) < 56 && (8 - gameGetLevel()) * 60 + 180 < ai->f83E) {
                ai->f82E = 0;
                ai->f05E = 1;
            }
        }
        if (ai->f455 && ai->f468 > 0) {
            tgt = aiGetTargetCar(ai->id, &th);
            if (th)
                x = HREC(tgt)->state.x2C.route;
            else
                x = DRV(tgt)->route;
            if (ai->f468 != (s8)aiRouteLinkTargetTable[aiRouteNodes[ai->f464].route][x] && !ai->f460)
                ai->f455 = 0;
        }
        if (ai->f448) {
            if (ai->f45E && !trackVehiclesShouldStop())
                ai->f45E = 0;
            if (ai->f455 && ai->f468 == ai->f464)
                sub_800E9E20(ai);
            else
                aiFreeSteer(ai);
        } else {
            aiFollowRoute(ai);
            aiSteerToGoal(ai);
            if (ai->level && ai->state < 2 && ai->away > ai->f520 && ai->f0D4 == 1 && ai->f47A > 0) {
                aiStartRouteFollow(ai);
                aiChooseNextNode(ai, ai->f462);
                aiUpdateStuck(ai);
                aiSetSegmentSpeed(ai, 0);
            }
        }
        aiInputFinalize(ai);
        carThrottleSteer(ai, 0);
    }
    carUpdateContacts(ai, 0);
    carWheelsUpdate(ai, 0);
    carSpinOutUpdate(ai, 0);
    if (gameGetMode() != 4 && gameGetMode() != 2)
        carParisSinkZonesUpdate(ai, 0);
    if (ai->f080)
        carCrushContactUpdate(ai, 0);
    carBodySwayUpdate(ai, 0);
    carBodyAttitudeUpdate(ai, 0);
    if (ai->f06C && ai->f501 != 4)
        carWreckUpdate(ai, 0);
}

extern s32 carNearestRoutePoint(TM2AI *, u8 human);
extern void aiPlaceOnRouteNode(TM2AI *);
extern void fxExplosion48(s32 *pos, u8 flag);
extern void fxSmokeA48(s32 *pos, s32);
extern void aiAdvanceRouteNode(TM2AI *);
extern void carBodyFollowCar(void *motion, void *state);
extern void aiTryRouteToTarget(TM2AI *);
void aiReplanRoute(TM2AI *ai);

/* @800E9504 */
void __attribute__((section(".text_800E9504"))) aiMaybeReplanRoute(TM2AI *ai)
{
    if (ai->f03B || (ai->f455 && ai->turned >= 6145)) {
        aiReplanRoute(ai);
        ai->f03B = 0;
        ai->f472 = 0;
        ai->f457 = 0;
    }
}

/* @800E9578: plan a new route (respawning first when stuck). */
void __attribute__((section(".text_800E9578"))) aiReplanRoute(TM2AI *ai)
{
    aiChooseNextNode(ai, carNearestRoutePoint(ai, 0));
    aiUpdateStuck(ai);
    aiSetSegmentSpeed(ai, 0);
    if (ai->stuck) {
        ai->spawn = carNearestRoutePoint(ai, 0);
        aiPlaceOnRouteNode(ai);
        fxExplosion48(ai->pos, 0);
        fxSmokeA48(ai->pos, 0);
    }
    aiAdvanceRouteNode(ai);
    aiUpdateGoalBearing(ai);
    ai->f45B = 1;
    ai->f448 = 0;
    ai->f457 = 0;
    ai->f490 = 0x800;
    ai->f492 = 0x7FFF;
    ai->turned = 0;
    ai->f456 = 0;
    ai->f449 = 0;
    ai->f498 = 0;
    ai->f45F = 0;
    ai->f03B = 0;
    ai->f45E = 0;
    ai->f03F = 0;
    if (!ai->f06D)
        carBodyFollowCar(&AI(ai)->motion, &AI(ai)->state);
}

/* @800E9674: give up after 180 tics, a full turn or being stuck. */
void __attribute__((section(".text_800E9674"))) aiUpdateGiveUp(TM2AI *ai)
{
    ai->f008 += timerGetFrameTicks();
    if (ai->f008 >= 181 || ai->turned >= 6145 || ai->f474 >= 4 || ai->f041) {
        aiReplanRoute(ai);
        ai->f041 = 0;
    }
}

/* @800E971C: start following the route. */
void __attribute__((section(".text_800E971C"))) aiStartRouteFollow(TM2AI *ai)
{
    ai->f448 = 1;
    ai->f449 = 0;
    ai->f490 = 0x800;
    ai->f492 = 0x7FFF;
    ai->turned = 0;
    ai->f456 = 0;
    ai->f457 = 0;
    ai->f498 = 0;
    ai->f45F = 0;
    if (!ai->f455 && ai->state == 1 && ai->f472 < 600)
        aiTryRouteToTarget(ai);
}

extern s8 aiRouteFollowRanges[];             /* per-node follow range / 64 */
extern u8 carHasMeleeSpecial(s32 type);
extern s32 aiGetDifficulty(void);
extern void *RotMatrixYXZ(s16 *, TM2Matrix *);
u8 aiSlowTargetRouteAllowed(TM2AI *ai);
u8 aiRollTakeRoute(TM2AI *ai);
s32 aiBearingBetween(s32 x0, s32 y0, s32 x1, s32 y1);

typedef struct {
    u8 pad_00[0x0A];
    s16 speed;              /* +0x0A */
    u8 pad_0C[0x1C];
    s32 z;                  /* +0x28 */
} TM2AITgtMotion2;

typedef struct {
    u8 pad_00[0xB8];
    s32 fB8;                /* +0xB8 */
    s32 fBC;                /* +0xBC */
    u8 pad_C0[8];
    s32 fC8;                /* +0xC8 */
} TM2AITgtStats;

/* @800E97B0: start following the route towards the target's node when
 * it is close enough along the route. */
void __attribute__((section(".text_800E97B0"))) aiTryRouteToTarget(TM2AI *ai)
{
    u8 th;
    s32 node, tn, lim, next;
    void *tgt;              /* human (th) or AI record */
    TM2AITgtMotion2 *tm;
    TM2AITgtStats *tst;

    node = ai->node;
    tgt = aiGetTargetCar(ai->id, &th);
    if (th) {
        tn = HREC(tgt)->state.x2C.route;
        tm = (TM2AITgtMotion2 *)&HREC(tgt)->motion;
        tst = (TM2AITgtStats *)&HREC(tgt)->stats;
    } else {
        tn = DRV(tgt)->route;
        tm = (TM2AITgtMotion2 *)&AI(tgt)->motion;
        tst = (TM2AITgtStats *)&AI(tgt)->stats;
    }
    lim = 16800;
    if (node == tn)
        lim = aiRouteFollowRanges[tn] * 64;
    next = (s8)aiRouteLinkTargetTable[node][tn];
    if (tst->fBC < tm->speed && ai->away > ai->f520)
        lim /= 3;
    if (ai->dist < lim && next) {
        if (!ai->f455)
            ai->f455 = aiRollTakeRoute(ai);
        if (ai->f455) {
            ai->f468 = next;
            if (next < 0)
                ai->f468 = ai->f464;
            else if (next == 81)
                rand();
        }
    } else
        ai->f455 = 0;
}

/* @800E9950 */
void __attribute__((section(".text_800E9950"))) aiTryRouteToSlowTarget(TM2AI *ai)
{
    u8 th;
    void *tgt;              /* human (th) or AI record */
    TM2AITgtMotion2 *tm;
    TM2AITgtStats *tst;

    if (aiSlowTargetRouteAllowed(ai)) {
        tgt = aiGetTargetCar(ai->id, &th);
        if (th) {
            tm = (TM2AITgtMotion2 *)&HREC(tgt)->motion;
            tst = (TM2AITgtStats *)&HREC(tgt)->stats;
        } else {
            tm = (TM2AITgtMotion2 *)&AI(tgt)->motion;
            tst = (TM2AITgtStats *)&AI(tgt)->stats;
        }
        if (tm->speed < tst->fC8 && __builtin_abs(tm->z - ai->pos[2]) < 80 && !carHasMeleeSpecial(TM2_SPECIAL_TYPE(ai->f006, ai->type))) {
            aiStartRouteFollow(ai);
            ai->f460 = 1;
            ai->f455 = 1;
            ai->f468 = ai->f464;
        }
    } else
        ai->f460 = 0;
}

/* @800E9A40 */
u8 __attribute__((section(".text_800E9A40"))) aiSlowTargetRouteAllowed(TM2AI *ai)
{
    u8 r;

    aiGetDifficulty() > 0 || ai->r1;
    if (gameGetLevel() == 5)
        r = ai->r2 != 0;
    else
        r = 0;
    return r;
}

/* @800E9A98: decide (partly at random, by level) to take the route. */
u8 __attribute__((section(".text_800E9A98"))) aiRollTakeRoute(TM2AI *ai)
{
    u8 r = 0;
    s32 n;

    if (gameGetLevel() == 8)
        r = 1;
    else if (!carHasMeleeSpecial(TM2_SPECIAL_TYPE(ai->f006, ai->type))) {
        if (gameGetLevel() == 5) {
            n = aiGetDifficulty();
            if (n <= 0)
                r = (rand() & 15) == 1;
            else if (n < 2)
                r = (rand() & 7) == 1;
            else
                r = rand() & 1;
        } else if (gameGetLevel() == 3 && ai->pos[2] >= 401 && __builtin_abs(ai->dz) < 400)
            r = rand() & 1;
    }
    return r;
}

/* @800E9BB4: aim point on the route node (ahead by the speed; car 0xF0
 * offsets by its lane f47A). */
void __attribute__((section(".text_800E9BB4"))) aiSetRouteAimPoint(TM2AI *ai)
{
    s16 r[4];
    s32 v[3];
    s32 out[4];
    TM2Matrix m;
    s32 gx, gy;

    ai->f457 = 1;
    gx = aiRouteNodes[ai->f464].x;
    gy = aiRouteNodes[ai->f464].y;
    if (ai->type == 0xF0) {
        r[0] = 0;
        r[1] = 0;
        r[2] = ai->goal_angle;
        RotMatrixYXZ(r, &m);
        v[0] = 0;
        if (ai->f47A == -2)
            v[1] = 0;
        else if (ai->f47A == -1)
            v[1] = -40;
        else if (ai->f47A == 0)
            v[1] = -80;
        else if (ai->f47A == 1)
            v[1] = -120;
        else
            v[1] = -160;
        v[2] = 0;
        mathMulTransVec(&m, v, out);
        gx += out[0];
        gy += out[1];
    }
    r[0] = 0;
    r[1] = 0;
    r[2] = ai->f48A;
    RotMatrixYXZ(r, &m);
    v[0] = 0;
    v[1] = ai->speed * 400 / ai->f5B4 + 200;
    v[2] = 0;
    mathMulTransVec(&m, v, out);
    ai->aim[0] = gx + out[0];
    ai->aim[1] = gy + out[1];
    ai->f482 = ai->f48A - aiBearingBetween(ai->pos[0], ai->pos[1], ai->aim[0], ai->aim[1]);
    mathBoundSAngle(&ai->f482);
    ai->f480 = ai->heading;
    mathBoundSAngle(&ai->f480);
}

/* @800E9DF0: bearing from (x0, y0) to (x1, y1). */
s32 __attribute__((section(".text_800E9DF0"))) aiBearingBetween(s32 x0, s32 y0, s32 x1, s32 y1)
{
    s32 dx, dy;

    dx = x1 - x0;
    dy = y1 - y0;
    if (!dy)
        dy = 1;
    return ratan2(dx, dy);
}

typedef struct {
    u8 pad_00[0x20];
    s32 pos[3];             /* +0x20 */
    s32 rot[3];             /* +0x2C */
} TM2AITgtMotion3;

extern s32 humanCarCount(void);
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H5: portHumanNearestCar */
#endif
extern s32 aiCarNearestAlive(s32 *pos);
extern s32 humanCarNearest(s32 *pos, s32);
extern void aiRouteLateralOffset(TM2AI *, s32 *, u8 which);

/* @800EC890 */
void __attribute__((section(".text_800EC890"))) aiStartGiveUp(TM2AI *ai)
{
    ai->f03D = 1;
}

/* @800EC8A0: the car nearest to human car a (an AI car, or the other
 * human in two-player modes); *human says which. */
void *__attribute__((section(".text_800EC8A0"))) carNearestToHuman(s32 a, u8 *human)
{
    s32 *pos = humanCarGet(a)->motion.pos;

#ifdef TM2_PORT_HUMANS
    if (portHumanRules())   /* H5: the nearest of the live humans and computer cars (presentation only) */
        return portHumanNearestCar(a, human);
#endif
    if (humanCarCount() == 1 || gameGetMode() == 2) {
        *human = 0;
        return aiCarGet(aiCarNearestAlive(pos));
    }
    *human = 1;
    return humanCarGet(humanCarNearest(pos, a));
}

/* @800EC938: clear the AI controls. */
void __attribute__((section(".text_800EC938"))) aiClearControls(struct TM2CarInput *c)
{
    c->f0D = 0;
    c->reverse = 0;
    c->steer_inc = 0;
    c->steer_dec = 0;
    c->brake = 0;
    c->throttle = 0;
    c->slide_inc = 0;
    c->slide_dec = 0;
    c->turbo = 0;
    c->cycle_armed = 1;
    c->cycle_next = 0;
    c->cycle_prev = 0;
    c->fire = 0;
    c->fire_guns = 0;
    c->fire_special = 0;
    c->fire_held = 0;
    c->f04 = 0;
    c->view_modifier = 0;
}

/* @800EC3A4: bearing to the target (+0x22), relative to our heading
 * (+0x20) and to the target's heading (+0x24); +0xD4 = where it is:
 * 1 behind, 3 left, 2 right, 0 ahead. */
void __attribute__((section(".text_800EC3A4"))) aiUpdateTargetBearing(TM2AI *ai)
{
    u8 th;
    TM2AITgtMotion3 *tm;
    void *tgt;              /* human (th) or AI record */
    s32 dx, dy;

    tgt = aiGetTargetCar(ai->id, &th);
    if (th)
        tm = (TM2AITgtMotion3 *)&HREC(tgt)->motion;
    else
        tm = (TM2AITgtMotion3 *)&AI(tgt)->motion;
    if (!ai->hold_dist) {
        dx = tm->pos[0] - ai->pos[0];
        dy = tm->pos[1] - ai->pos[1];
        if (!dy)
            dy = 1;
        ai->f022 = ratan2(dx, dy);
        mathBoundSAngle(&ai->f022);
    }
    ai->f020 = ai->f022 - ai->heading;
    mathBoundSAngle(&ai->f020);
    ai->f024 = ai->f022 - tm->rot[2] - 2048;
    mathBoundSAngle(&ai->f024);
    if (__builtin_abs(ai->f020) >= 1537)
        ai->f0D4 = 1;
    else if (ai->f020 >= 513)
        ai->f0D4 = 3;
    else if (ai->f020 < -512)
        ai->f0D4 = 2;
    else
        ai->f0D4 = 0;
}

/* @800EAAAC: can the car turn by angle a now? */
u8 __attribute__((section(".text_800EAAAC"))) aiCanTurnBy(TM2AI *ai, s32 a)
{
    u8 r = 1;

    if (!(__builtin_abs(a) < __builtin_abs(ai->m_f00) * 2)) {
        if (__builtin_abs(a) >= 1025)
            r = 0;
        else
            r = __builtin_abs(a) < 57;
    }
    if (__builtin_abs(ai->spin2) < ai->f56C)
        r = 0;
    if (__builtin_abs(a) < 170)
        ai->f457 = 0;
    return r;
}

/* @800EB544: turning range scaled down at low speed (at least 160). */
void __attribute__((section(".text_800EB544"))) aiUpdateTurnRange(TM2AI *ai)
{
    ai->f486 = ai->f484;
    if (ai->speed < ai->f5C0) {
        ai->f486 = ai->f484 * ai->speed / ai->f5C0;
        if (ai->f486 < 160)
            ai->f486 = 160;
    }
}

/* @800EB5F4: lane offset (look_ahead) for passing on side mode 1/2. */
void __attribute__((section(".text_800EB5F4"))) aiSetPassLane(TM2AI *ai, s32 mode)
{
    s32 tmp[4];
    s32 d;
    s32 g;

    aiRouteLateralOffset(ai, tmp, 1);
    ai->f4B0 = tmp[0];
    ai->look_ahead = 0;
    ai->f498 = 0;
    ai->f449 = 0;
    ai->f459 = 0;
    if (mode && ai->f47A > 0) {
        if (mode == 2)
            ai->look_ahead = 72;
        else
            ai->look_ahead = -72;
        if (ai->f47A >= 2)
            ai->look_ahead = ai->look_ahead * 2;
        g = ai->goal_dist - 400;
        d = g - ai->f486;
        if (d < 0)
            ai->look_ahead = 0;
        else {
            if (d < 800)
                ai->look_ahead = ai->look_ahead * d / 800;
            ai->f498 = mode;
            ai->f449 = 1;
        }
    }
}

extern s32 carDefaultTopSpeed(s32 type);
extern void objRegistrySetStep(s32, s32, s32);

/* @800EC180: weapon wishes and the fire button. */
void __attribute__((section(".text_800EC180"))) aiInputFinalize(TM2AI *ai)
{
    u8 fire;
    s32 x;

    ai->f054 = ai->f054 && !ai->hold_dist;
    ai->f053 = ai->f053 && !ai->hold_dist;
    ai->f072 = ai->f053 || ai->f054;
    ai->f057 = ai->f057 || ai->f079;
    ai->f056 = ai->f056 && !ai->f079 && !ai->f07E;
    ai->f052 = ai->f052 && !ai->f079 && !ai->f07E;
    if (ai->f089) {
        ai->f056 = 0;
        ai->f047 = 0;
    }
    fire = 1;
    if (!ai->f047) {
        fire = 0;
        if (aiGetDifficulty() > 0)
            fire = ai->f5C4 < ai->speed;
    }
    if (fire) {
        x = TM2_CAR_DEFAULT_TOP_SPEED(ai->f006, ai->type);
        if (ai->f54E * 3 / 4 < x)
            fire = 0;
    }
    if (fire) {
        if (!ai->firing) {
            ai->firing = 1;
#ifdef TM2_PORT_CAR_GROUPS
            portCarTurboSetStep(TM2_CAR_KEY(ai->f006, ai->type), 1);   /* request 32: f006 = slot */
#else
            objRegistrySetStep(ai->type + 6, 0, 1);
#endif
        }
    } else if (ai->firing) {
        ai->firing = 0;
#ifdef TM2_PORT_CAR_GROUPS
        portCarTurboSetStep(TM2_CAR_KEY(ai->f006, ai->type), 0);   /* request 32 */
#else
        objRegistrySetStep(ai->type + 6, 0, 0);
#endif
    }
}

typedef struct {
    u8 pad_00[0x0A];
    s16 speed;              /* +0x0A */
    u8 pad_0C[0x14];
    s32 x, y, z;            /* +0x20 */
} TM2AIPoint;

/* @800EBE74: is p (and the car) inside the drivable area of the level
 * (hard-coded boxes)? */
s32 __attribute__((section(".text_800EBE74"))) aiPointInLevelArea(TM2AI *ai, TM2AIPoint *p)
{
    s32 r = 1;

    if (p->y < -12000)
        r = 0;
    else if (p->y < -9840) {
        if (p->x < -8000)
            r = 0;
        else if (!(p->x < 9601))
            r = 0;
    } else if (p->y >= 801) {
        if (!(ai->node < 4))
            r = 0;
    } else if (!(p->y < -8799) && p->y < -5600) {
        if (p->x >= 4321 && p->y < 5280)
            r = 0;
        else if (!(p->x < -5279) && p->y < -4320)
            r = 0;
    } else if (!(ai->pos[1] < -8799) && ai->pos[1] < -5600) {
        if (ai->pos[0] >= 4321 && ai->pos[1] < 5280)
            r = 0;
        else if (!(ai->pos[0] < -5279) && ai->pos[1] < -4320)
            r = 0;
    }
    return r;
}

typedef struct {
    u8 pad_00[0x0A];
    s16 speed;              /* +0x0A */
    u8 pad_0C[0x28];
    s32 rot2;               /* +0x34 */
} TM2AITgtMotion4;

#define CRUISE() ((s16)timerGetFrameTicks() * 190 / 100 * 32)

/* @800EBFF8: speed to keep: the target's when following it closely,
 * else at least the cruise speed, else +0x5C0. */
s32 __attribute__((section(".text_800EBFF8"))) aiFollowSpeed(TM2AI *ai)
{
    u8 th;
    s32 a;
    TM2AITgtMotion4 *tm;
    void *tgt;              /* human (th) or AI record */

    tgt = aiGetTargetCar(ai->id, &th);
    if (th)
        tm = (TM2AITgtMotion4 *)&HREC(tgt)->motion;
    else
        tm = (TM2AITgtMotion4 *)&AI(tgt)->motion;
    a = ai->heading - tm->rot2;
    mathBoundAngle(&a);
    if (!ai->f03C) {
        if (__builtin_abs(a) < 1024 && tm->speed > 0)
            return tm->speed;
        if (tm->speed >= 0 && __builtin_abs(tm->speed) < CRUISE())
            return CRUISE();
    }
    return ai->f5C0;
}

extern s32 mathBlendAngle(s32 angle, s32 target, s32 pct);
extern void carSpinOut(void *car, s32 human, s32, s32);
void aiSetPassLane(TM2AI *ai, s32 mode);
u8 aiShouldChaseDirect(TM2AI *ai, s32 range);

/* @800EB714: steer towards the goal; ram a weaker car close ahead or
 * start passing it; while passing, steer out to the chosen side. */
void __attribute__((section(".text_800EB714"))) aiSteerToGoal(TM2AI *ai)
{
    s32 tmp[4];
    u8 th;
    s32 a, d;
    void *tgt;              /* human (th) or AI record */
    s32 tw;
    s32 r;
    u8 pass;

    if (ai->f45E)
        return;
    if (ai->f03B) {
        if (aiShouldChaseDirect(ai, ai->goal_dist))
            ai->heading = mathBlendAngle(ai->heading, ai->f022, 90);
        else
            ai->f03B = 0;
        return;
    }
    if (!ai->f498) {
        if (ai->f474)
            ai->heading = mathBlendAngle(ai->heading, ai->goal_angle, 80);
        else
            ai->heading = mathBlendAngle(ai->heading, ai->goal_angle, 90);
        if (__builtin_abs(ai->f024) >= 113)
            return;
        pass = 0;
        if (__builtin_abs(ai->f020) < 227) {
            tgt = aiGetTargetCar(ai->id, &th);
            if (th)
                tw = HREC(tgt)->stats.mass;
            else
                tw = AI(tgt)->stats.mass;
            if (ai->f576 < tw) {
                if (ai->dist < 600) {
                    pass = 0;
                    carSpinOut(ai, th, 7, 1);
                } else if (ai->dist < 1200)
                    pass = 1;
            }
        } else if (__builtin_abs(ai->f020) >= 1821 && !ai->f45A) {
            r = rand() % 100;
            if (r < ai->pass_chance) {
                pass = 1;
                ai->f45A = 1;
            }
        }
        if (pass) {
            aiRouteLateralOffset(ai, tmp, 1);
            if (tmp[0] > 0)
                aiSetPassLane(ai, 1);
            else
                aiSetPassLane(ai, 2);
        }
        return;
    }
    if (ai->f459)
        a = ai->goal_angle;
    else {
        aiRouteLateralOffset(ai, tmp, 1);
        if (__builtin_abs(tmp[0] - ai->f4B0) > __builtin_abs(ai->look_ahead) / 2) {
            ai->f459 = 1;
            a = ai->goal_angle;
        } else {
            if (ai->f498 == 2)
                a = ai->goal_angle + 170;
            else
                a = ai->goal_angle - 170;
            mathBoundAngle(&a);
        }
    }
    d = a - (ai->heading + ai->spin2);
    mathBoundAngle(&d);
    if (__builtin_abs(d) >= 57) {
        if (ai->f498 == 2) {
            if (ai->f459) {
                if (d < 0)
                    ai->f053 = 1;
            } else if (d > 0) {
                ai->f054 = 1;
                ai->f044 = 1;
            }
        } else if (ai->f498 == 1) {
            if (ai->f459) {
                if (d > 0)
                    ai->f054 = 1;
            } else if (d < 0) {
                ai->f053 = 1;
                ai->f045 = 1;
            }
        }
    } else if (ai->f459)
        aiSetPassLane(ai, 0);
}

extern u8 worldSectionHasDrop[];             /* level 5: nodes where the car may leave the route */
extern u8 arenaIsHolland(void), arenaGetShortChaseRange(void);

/* @800EBB74: drive straight at the target (instead of along the route)? */
u8 __attribute__((section(".text_800EBB74"))) aiShouldChaseDirect(TM2AI *ai, s32 range)
{
    u8 th;
    u8 r = 0;
    void *tgt;              /* human (th) or AI record */
    TM2AIPoint *tm;
    TM2AITgtStats *tst;
    s32 tn, node, dz, unused, u2;
    u8 ok;
#ifdef TM2_PORT_BALANCE
    s32 troute0;            /* request 101: the target's section (tn is changed on Holland) */
#endif

    tgt = aiGetTargetCar(ai->id, &th);
    if (th) {
        tn = HREC(tgt)->state.x2C.route;
        tm = (TM2AIPoint *)&HREC(tgt)->motion;
        tst = (TM2AITgtStats *)&HREC(tgt)->stats;
        unused = ai->f090;
    } else {
        tn = DRV(tgt)->route;
        tm = (TM2AIPoint *)&AI(tgt)->motion;
        tst = (TM2AITgtStats *)&AI(tgt)->stats;
        unused = ai->type;
    }
#ifdef TM2_PORT_BALANCE
    troute0 = tn;
#endif
    if (gameGetLevel() == 5) {
        if (carHasMeleeSpecial(TM2_SPECIAL_TYPE(ai->f006, ai->type))) {
            if (tn == ai->node)
                ai->f03B = 1;
        } else
            ai->f03B = 0;
        return ai->f03B;
    }
    /* two separate r = 0 (merged by cross-jumping) give r the extra use
     * that puts it in s5 ahead of tm */
    if (tm->speed > tst->fB8 / 2)
        r = 0;
    else if (ai->f455 && ai->f468 > 0)
        r = 0;
    else if (ai->state == 1) {
        node = ai->node;
        dz = ai->pos[2] - tm->z;
        if (!ai->f474) {
            if (arenaIsHolland()) {
                tn = node;
                range = 3200;
                if (ai->dist >= 601)
                    dz = 0;
            }
            if (node == tn)
                ok = 1;
            else if (gameGetLevel() == 5)
                ok = worldSectionHasDrop[node] == 0;
            else if (dz >= 401) {
                if (__builtin_abs(ai->f024) < 796) {
                    ok = 1;
                    range = 1600;
                    u2 = ai->dist;  /* dead load (separate variable for retail regs) */
                } else
                    ok = 0;
            } else
                ok = ai->f47A > 0;
            if (ok && ai->stuck_time < 6 && dz >= -199) {
                if (!arenaGetShortChaseRange())
                    range *= 2;
                if (range < 320)
                    range = 320;
                else if (range >= 4801)
                    range = 4800;
                if (ai->dist < range)
                    r = 1;
            }
        }
    }
    if (ai->type == 0xF0 && !(u8)aiPointInLevelArea(ai, tm))
        r = 0;
#ifdef TM2_PORT_BALANCE
    if (r && portBalanceJamRound(ai))   /* request 91 item 3 (bit 27): going round a pile, along the route */
        r = 0;
    if (r && portBalanceRouteBlocked(ai->f464, &tm->x, troute0))    /* request 101 (bit 29): out of the graph's reach
                                                                       (f464: the car's node) */
        r = 0;
#endif
    return r;
}

extern s32 mathBlendValue(s32 value, s32 target, s32 pct);
extern u8 aiNearSectionEdge(TM2AI *);

/* @800EC504: stop manoeuvre: brake to a halt (stunned), back out for 30
 * tics, or brake (harder on some route nodes) and re-plan once stopped. */
void __attribute__((section(".text_800EC504"))) aiUpdateStop(TM2AI *ai)
{
    u8 done = 0;
    s32 dec;

    if (ai->f06C) {
        ai->backup_time = 30;
        ai->f041 = 0;
    }
    aiSetPassLane(ai, 0);
    if (ai->f079) {
        if (ai->speed || ai->f06D) {
            if (ai->speed > 0) {
                ai->speed -= ai->brake;
                if (ai->speed >= 0)
                    return;
            } else {
                ai->speed += ai->brake;
                if (ai->speed <= 0)
                    return;
            }
            ai->speed = 0;
            carBodyFollowCar(&AI(ai)->motion, &AI(ai)->state);
        }
        return;
    }
    if (ai->f041) {
        ai->backup_time += timerGetFrameTicks();
        carBodyFollowCar(&AI(ai)->motion, &AI(ai)->state);
        ai->speed = mathBlendValue(ai->speed, -((s16)timerGetFrameTicks() * 760) / 100 * 32, 95);
        aiUpdateGoal2Bearing(ai);
        ai->heading = mathBlendAngle(ai->heading, ai->goal2_angle, 95);
        if (ai->backup_time >= 31) {
            ai->f453 = 0;
            ai->f041 = 0;
            ai->backup_time = 0;
            ai->speed = 0;
        }
        return;
    }
    if (!ai->f59C)
        ai->f0DE -= timerGetFrameTicks();
    ai->heading = mathBlendAngle(ai->heading, ai->goal_angle, 95);
    if ((ai->f0DE >= 31 || worldSectionHasDrop[ai->node]) && ai->speed) {
        if (aiNearSectionEdge(ai))
            dec = ai->brake;
        else
            dec = ai->brake / 3;
        if (ai->speed > 0) {
            ai->speed -= dec;
            if (ai->speed < 0)
                ai->speed = 0;
        } else {
            ai->speed += dec;
            if (ai->speed > 0)
                ai->speed = 0;
        }
    } else
        done = 1;
    if (done) {
        carBodyFollowCar(&AI(ai)->motion, &AI(ai)->state);
        carPushClear(&AI(ai)->contact);
#ifdef TM2_PORT_BALANCE
        if (portBalanceJamStop(ai))     /* request 91 item 3 (bit 27): jammed by cars: back out and go round */
            ai->f03F = 1;
#endif
        if (ai->f03F)
            ai->f041 = 1;
        else {
            ai->f041 = 0;
            ai->f453 = 0;
        }
        aiReplanRoute(ai);
    }
}

typedef struct {
    u8 pad_00[8];
    s8 f08;                 /* +0x08 */
} TM2AITgtStats8;

/* @800E9E20: on the route: turn to the second goal far away; when slow,
 * fire a special (level-dependent random choice) at the target. */
void __attribute__((section(".text_800E9E20"))) sub_800E9E20(TM2AI *ai)
{
    u8 th;
    s32 a, lim;
    void *tgt;              /* human (th) or AI record */
    TM2AITgtStats8 *tst;

    if (!(ai->dist < 5601) || !(ai->f472 < 421)) {
        tgt = aiGetTargetCar(ai->id, &th);
        if (th)
            tst = (TM2AITgtStats8 *)&HREC(tgt)->stats;
        else
            tst = (TM2AITgtStats8 *)&AI(tgt)->stats;
        if (tst->f08 == ai->f006) {
            ai->f472 = 0;
            tst->f08 = -1;
            return;
        }
        a = ai->heading - ai->goal2_angle;
        mathBoundAngle(&a);
        lim = __builtin_abs(ai->m_f00) * 341 / ai->f560;
        if (a < -lim)
            ai->f054 = 1;
        else if (lim < a)
            ai->f053 = 1;
        else {
            ai->f455 = 0;
            ai->f460 = 0;
        }
        if (ai->f472 >= 601) {
            ai->f455 = 0;
            ai->f460 = 0;
        }
        ai->f057 = 1;
        return;
    }
    if (__builtin_abs(ai->speed) < ai->f5B0) {
        ai->f057 = 1;
        if (!ai->f45E) {
            a = ai->heading - ai->f022;
            mathBoundAngle(&a);
            lim = __builtin_abs(ai->m_f00) * 341 / ai->f560;
            if (a < -lim)
                ai->f054 = 1;
            else if (lim < a)
                ai->f053 = 1;
        }
        if (ai->dist < 3201)
            return;
        if ((8 - gameGetLevel()) * 60 + 120 < ai->f83E) {
            if (gameGetLevel() == 8 || TM2_SPECIAL_TYPE(ai->f006, ai->type) == 0x20) {
                if (ai->f45E) {
                    ai->special = 6;
                    if (ai->dist < 4000 && (rand() & 1))
                        ai->special = 2;
                } else if ((rand() & 5) == 1)
                    ai->special = 5;
                else if (rand() & 1)
                    ai->special = 2;
                else
                    ai->special = 1;
            } else
                ai->special = 0;
            ai->f82E = 0;
            ai->f83E = 0;
            ai->f05E = 1;
        } else
            ai->f83E += timerGetFrameTicks();
        return;
    }
    if (ai->goal_dist * ai->brake / ai->speed * 32 < ai->speed)
        ai->f057 = 1;
    else
        ai->f057 = 0;
}

extern void carBodySpinClear(void *motion);
u8 aiCanTurnBy(TM2AI *ai, s32 a);

#define SETTLE(ai) carBodyFollowCar(&AI(ai)->motion, &AI(ai)->state)

/* @800EA270: free steering towards the target (direct chase) or the
 * second goal; give up when stuck or turned around; car 0xF0 spins its
 * body to aim; steer through the aim point when one is set. */
void __attribute__((section(".text_800EA270"))) aiFreeSteer(TM2AI *ai)
{
    u8 th;
    s32 a, b, c, d, e;
    u8 done = 0;
    void *tgt;              /* human (th) or AI record */
    s32 tw, k, g, h;

    aiUpdateGoal2Bearing(ai);
    if (aiShouldChaseDirect(ai, ai->goal2_dist)) {
        a = ai->heading - ai->f022;
        ai->f03B = 1;
        tgt = aiGetTargetCar(ai->id, &th);
        if (th)
            tw = HREC(tgt)->stats.mass;
        else
            tw = AI(tgt)->stats.mass;
        if (TM2_SPECIAL_TYPE(ai->f006, ai->type) == 0x20 || ai->f458)
            ai->f03C = 1;
        if (!(ai->f576 < tw) && (rand() & 3) == 1)
            ai->f03C = 1;
        else
            ai->f03C = 0;
    } else {
        a = ai->heading - ai->goal2_angle;
        aiMaybeReplanRoute(ai);
    }
    mathBoundAngle(&a);
    if (ai->f456) {
        if (ai->f452) {
            if (__builtin_abs(ai->speed) < ai->f5B0 * 2)
                done = 1;
            else if (!ai->f078)
                ai->speed = mathBlendValue(ai->speed, 0, 80);
        } else if (ai->type == 0xF0) {
            b = ai->heading - ai->f75C;
            mathBoundAngle(&b);
            if (b > 0) {
                ai->f75C += (s16)timerGetFrameTicks() * 170;
                b = ai->heading - ai->f75C;
                mathBoundAngle(&b);
                if (b < 0) {
                    SETTLE(ai);
                    done = 1;
                }
            } else {
                ai->f75C -= (s16)timerGetFrameTicks() * 170;
                b = ai->heading - ai->f75C;
                mathBoundAngle(&b);
                if (b > 0) {
                    SETTLE(ai);
                    done = 1;
                }
            }
            ai->m_f00 = ai->m_f00 / 2;
        } else
            done = 1;
        if (!ai->f078) {
            ai->m_f00 = mathBlendAngle(ai->m_f00, 0, 90);
            ai->heading = mathBlendAngle(ai->heading, ai->f48A, 90);
        }
    } else if (ai->f472 >= 601 || ai->turned >= 6145)
        done = 1;
    else if (__builtin_abs(a) < 57) {
        if (ai->f452 || ai->type == 0xF0) {
            ai->f456 = 1;
            ai->f554 = 0;
        } else
            done = 1;
    } else if (__builtin_abs(a) < 342) {
        if (__builtin_abs(ai->f490) < __builtin_abs(a)) {
            if (ai->f452 || ai->type == 0xF0) {
                ai->f456 = 1;
                ai->f554 = 0;
            } else
                done = 1;
        } else
            ai->f490 = a;
    } else if (ai->type == 0xF0 && (ai->goal_dist > ai->goal_dist_prev || ai->goal_dist < 80))
        done = 1;
    if (done) {
        aiAdvanceRouteNode(ai);
        if (ai->f452)
            ai->speed = ai->f5B0;
        if (!ai->f06D)
            SETTLE(ai);
    } else {
        if (!aiCanTurnBy(ai, a)) {
            if (a > 0)
                ai->f053 = 1;
            else
                ai->f054 = 1;
        } else {
            ai->f054 = 0;
            ai->f053 = 0;
        }
        if (ai->f452 || ai->type == 0xF0) {
            carBodySpinClear(&AI(ai)->motion);
            ai->f046 = 1;
            if (ai->f053)
                ai->f045 = 1;
            else if (ai->f054)
                ai->f044 = 1;
        } else if (!ai->f06D)
            SETTLE(ai);
        if (ai->f457 && !ai->hold_dist) {
            c = ai->f48A - aiBearingBetween(ai->pos[0], ai->pos[1], ai->aim[0], ai->aim[1]);
            mathBoundAngle(&c);
            k = 0;
            if (ai->f482)
                k = c * 100 / ai->f482;
            d = ai->f48A - ai->f480;
            mathBoundAngle(&d);
            g = ai->f48A - d * k / 100;
            e = g - ai->heading;
            mathBoundAngle(&e);
            h = __builtin_abs(e);
            if ((s16)timerGetFrameTicks() * 79 < h)
                ai->f457 = 0;
            else {
                ai->m_f00 = e;
                ai->heading = g - e;
                mathBoundAngle(&ai->heading);
                if (ai->m_f00 > ai->f560)
                    ai->m_f00 = ai->f560;
                else if (ai->m_f00 < -ai->f560)
                    ai->m_f00 = -ai->f560;
            }
        } else {
            tw = __builtin_abs(a / 2);
            if (tw < ai->m_f00)
                ai->m_f00 = tw;
            else if (ai->m_f00 < -tw)
                ai->m_f00 = -tw;
        }
    }
    if (!ai->f456 && ai->speed < ai->f5C0 && !ai->f452) {
        ai->f056 = 1;
        ai->f057 = 0;
    }
}

extern u8 gameIsAttractDemo(void);
extern s32 aiRouteSpeedLimit(s32 from, s32 to);
extern void aiDarkToothJumpSelect(TM2AI *);
extern void weaponTick(TM2AI *, s32);

/* @800EAB50: follow the route. Reverse and turn when the goal falls behind;
 * at a node either leave the goal (f4D8) or pick the next node; otherwise
 * keep the speed for the distance (chase or goal) under the route's speed
 * limit. Car 0xF0 jumps between route points 0x70 / 0x75. */
void __attribute__((section(".text_800EAB50"))) aiFollowRoute(TM2AI *ai)
{
    s32 a;
    u8 done = 0;
    s32 d, lim, k, x, t, dx, dy;
    u8 acc, brk;

    if (!ai->f06D)
        SETTLE(ai);
    if (!ai->f4D8)
        aiUpdateTurnRange(ai);
    if (ai->goal_dist > ai->goal_dist_prev && !ai->f452) {
        ai->f45B = 1;
        ai->f03B = 0;
    }
    if (ai->f45B) {
        carBodySpinClear(&AI(ai)->motion);
        ai->f057 = 1;
        ai->f056 = 0;
        if (ai->speed < ai->f5B8) {
            ai->speed = ai->speed / 2;
            ai->goal_dist_prev = ai->goal_dist;
            ai->heading = mathBlendAngle(ai->heading, ai->goal_angle, 95);
            a = ai->heading - ai->goal_angle;
            mathBoundAngle(&a);
            if (__builtin_abs(a) < 56) {
                SETTLE(ai);
                ai->f45B = 0;
            }
        }
    } else if (ai->f486 > ai->goal_dist) {
        if (ai->f4D8) {
            aiChooseNextNode(ai, -1);
            aiUpdateStuck(ai);
            aiSetSegmentSpeed(ai, 1);
            ai->f4D8 = 0;
        } else {
            aiStartRouteFollow(ai);
            if (!ai->f452 && (!gameIsAttractDemo() || ai->f47A < 2))
                aiSetRouteAimPoint(ai);
        }
    } else {
        if (ai->goal_dist < ai->f4D8) {
            aiChooseNextNode(ai, -1);
            aiUpdateStuck(ai);
            aiSetSegmentSpeed(ai, 1);
            ai->f4D8 = 0;
        }
        if (ai->f03B) {
            if (ai->type == 0xC0)
                d = ai->dist - 120;
            else if (ai->f03C) {
                d = ai->dist;
                aiFollowSpeed(ai);
            } else
                d = ai->dist - ai->range_near;
            aiSetPassLane(ai, 0);
        } else {
            d = ai->goal_dist;
            lim = ai->f5C0;
        }
        if (d <= 0) {
            if (__builtin_abs(ai->f020) < 113)
                ai->f057 = 1;
            else {
                ai->f056 = 1;
                ai->f03B = 0;
            }
        } else if (__builtin_abs(ai->speed) < (s16)timerGetFrameTicks() * 380 / 100 * 32 || d > 0x1FFFFF)
            ai->f056 = 1;
        else {
            if (ai->node != aiRouteNodes[ai->f464].route
                && (lim = aiRouteSpeedLimit(aiRouteNodes[ai->f462].route, aiRouteNodes[ai->f464].route)) != 0) {
                done = 1;
                if (ai->speed < lim) {
                    ai->f056 = 1;
                    if (ai->f5B4 < lim)
                        ai->f047 = 1;
                } else if (ai->speed > lim + lim / 4)
                    ai->f057 = 1;
            } else
                lim = ai->f5C0;
            if (!done) {
                k = ai->brake;
                if (aiGetDifficulty() <= 0)
                    k = k / 8;
                else if (aiGetDifficulty() < 2)
                    k = k / 2;
                brk = 0;
                acc = 0;
                t = ai->speed - lim;
                x = d << 5;
                if (ai->speed)
                    x = x / ai->speed * k;
                if (t < 0)
                    acc = 1;
                else if (x < t)
                    brk = 1;
                else if (t < x / 2)
                    acc = 1;
                if (acc)
                    ai->f056 = 1;
                else if (brk)
                    ai->f057 = 1;
            }
        }
    }
    if (ai->type == 0xF0) {
        aiDarkToothJumpSelect(ai);
        if (ai->f45C || ai->f45D) {
            if (ai->goal_dist < 3600 && ai->goal_dist >= 3201) {
                if (ai->f45C) {
                    ai->f45C = 0;
                    carSpinOut(ai, 0, 12, 0);
                    ai->speed = (s16)timerGetFrameTicks() * 1805 / 100 * 32;
                } else {
                    ai->f45D = 0;
                    ai->f45E = 1;
                    carSpinOut(ai, 0, 13, 0);
                    ai->speed = (s16)timerGetFrameTicks() * 570 / 100 * 32;
                    if (ai->f464 == 2 || ai->f464 == 40)
                        ai->f464 = 0x75;
                    else if (ai->f464 == 11 || ai->f464 == 44)
                        ai->f464 = 0x70;
                }
                dx = ai->goal[0] - ai->pos[0];
                dy = ai->goal[1] - ai->pos[1];
                if (!dy)
                    dy = 1;
                ai->f75C = ratan2(dx, dy);
                mathBoundAngle(&ai->f75C);
                ai->heading = ai->f75C;
            }
        } else if (ai->f45E) {
            if (ai->f022 > 0)
                ai->heading = mathBlendAngle(ai->heading, 1024, 90);
            else
                ai->heading = mathBlendAngle(ai->heading, -1024, 90);
            if (!ai->f078) {
                ai->speed = 0;
                ai->f056 = 0;
                ai->f057 = 1;
                ai->f455 = 0;
                ai->f460 = 0;
                if (ai->f464 == 0x70 || ai->f464 == 0x75) {
                    ai->f455 = 1;
                    ai->f468 = ai->f464;
                }
                if (ai->f455) {
                    ai->pos[0] = mathBlendValue(ai->pos[0], aiRouteNodes[ai->f464].x, 90);
                    ai->pos[1] = mathBlendValue(ai->pos[1], aiRouteNodes[ai->f464].y, 90);
                    ai->pos[2] = mathBlendValue(ai->pos[2], aiRouteNodes[ai->f464].z + ai->f53E, 90);
                } else {
                    ai->f45E = 0;
                    ai->f45F = 1;
                }
                if (ai->f83E > 120) {
                    ai->special = 6;
                    if (ai->dist < 4000 && (rand() & 1))
                        ai->special = 2;
                    ai->f82E = 0;
                    ai->f83E = 0;
                    ai->f05E = 1;
                    weaponTick(ai, 0);
                } else
                    ai->f83E += timerGetFrameTicks();
            }
            if (!trackVehiclesShouldStop())
                ai->f45E = 0;
        }
    }
}
