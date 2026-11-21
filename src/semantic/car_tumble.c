/* Game code (AI routes). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_numbers.h"   /* request 70: TM2_CAR_DEFAULT_TOP_SPEED (a mod car's own top speed) */

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

#define U8(p, o) (*(u8 *)((u8 *)(p) + (o)))
#define S16(p, o) (*(s16 *)((u8 *)(p) + (o)))
#define S32(p, o) (*(s32 *)((u8 *)(p) + (o)))

/* Route nodes (24 bytes), as in ai_route_nodes.c. */
typedef struct {
    s32 x, y;
    s16 z;
    s16 next[4];
    s8 lane[4];
    s8 count;
    s8 route;
} TM2RouteNode;

extern TM2RouteNode aiRouteNodes[];
#include "semantic/world.h"
static __inline__ s32 FPS(void) { s16 v = timerGetFrameTicks(); return v; }
extern int rand(void);
extern s32 rsin(s32);
extern void fxImpactDebris(s32 size, s32 *pos);
extern void fxSmokeC24(void *, s32);
#include "semantic/sound.h"
extern s32 carDefaultTopSpeed(s32 object);
extern s32 aiDifficultySpeedScale(void);

#define SPD(n) (32 * ((n) * (s16)timerGetFrameTicks() / 100))

/* @800F2778: car roll (barrel-roll) step: picks the human (+1756) or AI
   (+1820) motion block, derives a roll rate from speed (+10) clamped to at
   least 45*fps, kicks off sparks/sound/spin state on the first frame, advances
   the roll angle (+48) counting half-turns (+32 of the roll block), and either
   ends the roll when slow and nearly level or bounces the wheel heights. */
void __attribute__((section(".text_800F2778"))) carBarrelRollStep(void *car, u8 human)
{
    struct TM2CarMotion *m;
    struct TM2CarStatsBlock *r;
    struct TM2CarWheels *w;
    struct TM2CarState *st;
    s32 lim;
    s32 rate;
    s32 d;

    if (human) {
        m = &HREC(car)->motion;
        r = &HREC(car)->stats;
        w = &HREC(car)->wheels;
        st = &HREC(car)->state;
    } else {
        m = &AI(car)->motion;
        r = &AI(car)->stats;
        w = &AI(car)->wheels;
        st = &AI(car)->state;
        if (AI(car)->stats.mass >= 51)
            AI(car)->state.f12 = 0;
    }
    lim = __builtin_abs(m->speed) < SPD(2850) ? SPD(2850) : __builtin_abs(m->speed);
    rate = m->speed * (FPS() * 91) / lim;
    if (rate < 0) {
        if (-45 * (s16)timerGetFrameTicks() < rate)
            rate = -45 * (s16)timerGetFrameTicks();
    } else {
        if (rate < 45 * (s16)timerGetFrameTicks())
            rate = 45 * (s16)timerGetFrameTicks();
    }
    if (st->wheel_in_air == 0) {
        if (rand() & 1)
            fxImpactDebris(1, m->pos);
        if (!(rand() & 1))
            fxSmokeC24(m->pos, 0);
        if (human || AI(car)->active)
            soundPlay(6, 0, 0, 99);
        carSpinOut(car, human, 4, 1);
    }
    if (m->spin1 > 0) {
        rate += __builtin_abs(rsin(m->rot[1])) * rate / 4096;
        if (m->rot[1] < 1706) {
            m->rot[1] += rate;
            if (m->rot[1] >= 1706)
                r->f20++;
        } else
            m->rot[1] += rate;
    } else {
        rate += __builtin_abs(rsin(m->rot[1])) * rate / 4096;
        if (m->rot[1] >= 1707) {
            m->rot[1] -= rate;
            if (m->rot[1] < 1707)
                r->f20++;
        } else
            m->rot[1] -= rate;
    }
    if (__builtin_abs(m->speed) < SPD(190)
        || (__builtin_abs(m->speed) < SPD(950) && r->f20 > 0)) {
        if (__builtin_abs(m->rot[1]) < __builtin_abs(rate * 3)) {
            st->f12 = 0;
            m->spin1 = 0;
            m->rot[1] = 0;
            r->f20 = 0;
        }
    } else {
        d = rsin(m->rot[1]) * r->track_width / 4096;
        w->wheel[0].pos[2] += d;
        w->wheel[1].pos[2] += d;
        w->wheel[2].pos[2] += d;
        w->wheel[3].pos[2] += d;
        if (m->speed != 0) {
            if (m->speed > 0)
                m->speed -= r->brake_decel / 3;
            else
                m->speed += r->brake_decel / 3;
        }
        m->body_rot[2] = m->f02;
        st->f01 = 1;
        m->rot[2] = mathBlendAngle(m->rot[2], m->f04, 95);
    }
}

/* @800F2DF4: car tumble step (rolling about two axes): like 800F2778 but
   with no crash sound, and it advances the pitch angle (+44) before the roll
   angle (+48), both counting half-turns in +32 of the roll block; ends when
   slow and the pitch is nearly level, else bounces the wheel heights by the
   pitch (+74). */
void __attribute__((section(".text_800F2DF4"))) carTumbleStep(void *car, u8 human)
{
    struct TM2CarMotion *m;
    struct TM2CarStatsBlock *r;
    struct TM2CarWheels *w;
    struct TM2CarState *st;
    s32 lim;
    s32 rate;
    s32 d;

    if (human) {
        m = &HREC(car)->motion;
        r = &HREC(car)->stats;
        w = &HREC(car)->wheels;
        st = &HREC(car)->state;
    } else {
        m = &AI(car)->motion;
        r = &AI(car)->stats;
        w = &AI(car)->wheels;
        st = &AI(car)->state;
        if (AI(car)->stats.mass >= 51)
            AI(car)->state.f20 = 0;
    }
    lim = __builtin_abs(m->speed) < SPD(2850) ? SPD(2850) : __builtin_abs(m->speed);
    rate = m->speed * (FPS() * 91) / lim;
    if (rate < 0) {
        if (-45 * (s16)timerGetFrameTicks() < rate)
            rate = -45 * (s16)timerGetFrameTicks();
    } else {
        if (rate < 45 * (s16)timerGetFrameTicks())
            rate = 45 * (s16)timerGetFrameTicks();
    }
    if (st->wheel_in_air == 0) {
        if (rand() & 1)
            fxImpactDebris(1, m->pos);
        if (!(rand() & 1))
            fxSmokeC24(m->pos, 0);
        carSpinOut(car, human, 4, 1);
    }
    rate += __builtin_abs(rsin(m->rot[0])) * rate / 4096;
    if (m->rot[0] < 1706) {
        m->rot[0] += rate;
        if (m->rot[0] >= 1706)
            r->f20++;
    } else
        m->rot[0] += rate;
    if (m->spin1 > 0) {
        rate += __builtin_abs(rsin(m->rot[1])) * rate / 4096;
        if (m->rot[1] < 1706) {
            m->rot[1] += rate;
            if (m->rot[1] >= 1706)
                r->f20++;
        } else
            m->rot[1] += rate;
    } else {
        rate += __builtin_abs(rsin(m->rot[1])) * rate / 4096;
        if (m->rot[1] >= 1707) {
            m->rot[1] -= rate;
            if (m->rot[1] < 1707)
                r->f20++;
        } else
            m->rot[1] -= rate;
    }
    if (__builtin_abs(m->speed) < SPD(190)
        || (__builtin_abs(m->speed) < SPD(950) && r->f20 > 0)) {
        if (__builtin_abs(m->rot[0]) < __builtin_abs(rate * 3)) {
            st->f20 = 0;
            m->spin1 = 0;
            m->rot[0] = 0;
            r->f20 = 0;
        }
    } else {
        d = rsin(m->rot[0]) * r->wheelbase / 4096;
        w->wheel[0].pos[2] += d;
        w->wheel[1].pos[2] += d;
        w->wheel[2].pos[2] += d;
        w->wheel[3].pos[2] += d;
        if (m->speed != 0) {
            if (m->speed > 0)
                m->speed -= r->brake_decel / 3;
            else
                m->speed += r->brake_decel / 3;
        }
        m->body_rot[2] = m->f02;
        st->f01 = 1;
        m->rot[2] = mathBlendAngle(m->rot[2], m->f04, 95);
    }
}

/* @800F6F6C: set the AI car's target speed (+1156) and cornering speed
   (+1364) for the current route segment: when `mode` is set, from the car
   type's cornering factor (carDefaultTopSpeed), the turn angle (+1164), lane
   (+1148) and line (+1146), clamped to >= 160 and scaled by the difficulty
   (aiDifficultySpeedScale: 75 -> 2/3, 140 -> 6/5); else double base speed. Then floor
   the cornering speed at 2000/+1394, derive the brake distance (+1472), the
   node-to-node delta (+1188/+1192), which axis is nearly aligned
   (+1102/+1103/+1101) and the aim point (+1212/+1216). */
void __attribute__((section(".text_800F6F6C"))) aiSetSegmentSpeed(struct TM2AIRecord *ai, u8 mode)
{
    s32 q;

    if (mode) {
        if (DRV(ai)->sharp_turn) {
            if (DRV(ai)->next_lane <= 0) {
                DRV(ai)->f484 = ai->stats.turn_range;
                ai->stats.cruise_speed = TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type) / 3;
            } else {
                DRV(ai)->f484 = ai->stats.turn_range * 2;
                ai->stats.cruise_speed = TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type) * 2 / 3;
            }
        } else if (__builtin_abs(DRV(ai)->turn) < 455) {
            DRV(ai)->f484 = ai->stats.turn_range / 2;
            ai->stats.cruise_speed = ai->stats.top_speed;
        } else if (__builtin_abs(DRV(ai)->turn) < 1285) {
            ai->stats.cruise_speed = TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type);
            if (DRV(ai)->next_lane < 0) {
                DRV(ai)->f484 = ai->stats.turn_range / 2;
                ai->stats.cruise_speed = ai->stats.cruise_speed / 4;
            } else if (DRV(ai)->lane == 0) {
                DRV(ai)->f484 = ai->stats.turn_range / 2;
                ai->stats.cruise_speed = ai->stats.cruise_speed / 3;
            } else if (DRV(ai)->lane == 1) {
                DRV(ai)->f484 = ai->stats.turn_range * 2 / 3;
                ai->stats.cruise_speed = ai->stats.cruise_speed / 2;
            } else {
                DRV(ai)->f484 = ai->stats.turn_range;
                ai->stats.cruise_speed = ai->stats.cruise_speed * 3 / 4;
            }
        } else {
            ai->stats.cruise_speed = TM2_CAR_DEFAULT_TOP_SPEED(ai->slot, ai->type) / 3;
            DRV(ai)->f484 = ai->stats.turn_range * 3 / 2;
        }
        if (DRV(ai)->f484 < 160)
            DRV(ai)->f484 = 160;
        switch (aiDifficultySpeedScale()) {
        case 120:
            break;
        case 75:
            DRV(ai)->f484 = DRV(ai)->f484 * 2 / 3;
            break;
        case 140:
            DRV(ai)->f484 = DRV(ai)->f484 * 6 / 5;
            break;
        }
    } else {
        DRV(ai)->f484 = ai->stats.turn_range * 2;
        ai->stats.cruise_speed = ai->stats.top_speed;
    }
    q = 2000 / ai->stats.speed_mult;
    if (ai->stats.cruise_speed < q)
        ai->stats.cruise_speed = q;
    ai->stats.cruise_cap = ai->stats.cruise_speed * 19 * FPS() / 100 * 32 * ai->stats.speed_mult / 100;
    DRV(ai)->next_delta[0] = aiRouteNodes[DRV(ai)->node_next].x - aiRouteNodes[DRV(ai)->node].x;
    DRV(ai)->next_delta[1] = aiRouteNodes[DRV(ai)->node_next].y - aiRouteNodes[DRV(ai)->node].y;
    DRV(ai)->next_axis_x = __builtin_abs(DRV(ai)->next_delta[0]) < 40;
    DRV(ai)->next_axis_y = __builtin_abs(DRV(ai)->next_delta[1]) < 40;
    DRV(ai)->next_diagonal = !DRV(ai)->next_axis_x && !DRV(ai)->next_axis_y;
    if (DRV(ai)->next_diagonal) {
        DRV(ai)->next_corner[0] = aiRouteNodes[DRV(ai)->node].x + DRV(ai)->next_delta[0] / 2;
        DRV(ai)->next_corner[1] = aiRouteNodes[DRV(ai)->node].y + DRV(ai)->next_delta[1] / 2;
    } else if (DRV(ai)->next_axis_x)
        DRV(ai)->next_corner[0] = aiRouteNodes[DRV(ai)->node].x;
    else
        DRV(ai)->next_corner[1] = aiRouteNodes[DRV(ai)->node].y;
    DRV(ai)->f486 = DRV(ai)->f484;
}
