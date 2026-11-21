/* Car rollover (800F0944). Names stay address-based. */
#include "semantic/cars.h"

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

#define U8(p, o) (*(u8 *)((u8 *)(p) + (o)))
#define S16(p, o) (*(s16 *)((u8 *)(p) + (o)))
#define S32(p, o) (*(s32 *)((u8 *)(p) + (o)))

typedef struct { s16 m[3][3]; s32 t[3]; } RMatrix;

extern s32 timerGetFrameTicks(void);
extern void *RotMatrixYXZ(s16 *, RMatrix *);
extern void mathMulTransVec(RMatrix *, s32 *, s32 *);
extern s32 sub_8013FC44(s32);
extern int ratan2(int, int);
extern s32 mathBlendAngle(s32 angle, s32 target, s32 pct);
extern void carBarrelRollStep(void *car, u8 human);
extern void carTumbleStep(void *car, u8 human);
extern void carSuspensionUpdate(void *car, int human);

/* @800F0944: per-frame body attitude for one car side (human +1756 / AI +1820
   motion block): sums each wheel's height terms (+92/+164/+236/+308 of the
   wheel block), clamps sagging wheels to the axle limit (flagging them and
   pulling their spring), nudges the body sideways when wheels are clamped,
   steers the roll (+48) and pitch (+44) angles toward the wheel-derived
   attitude (clamped by speed), starts a barrel roll (800F2778) when the roll
   difference is too big at speed, runs barrel roll / tumble (800F2DF4) when
   active, sets the body height (+40) to the mean wheel height and finally
   runs the wheel update (800F15D0). */
void __attribute__((section(".text_800F0944"))) carBodyAttitudeUpdate(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    struct TM2CarMotion *m;
    struct TM2CarStatsBlock *r;
    struct TM2CarWheels *w;
    struct TM2CarState *st;
    s32 unused;
    s32 mx, lim, d, h, a, spd, c, e;
    s16 ang[4];
    s32 vec[4];
    s32 out[4];
    RMatrix mat;

    if (human) {
        m = &HREC(car)->motion;
        r = &HREC(car)->stats;
        w = &HREC(car)->wheels;
        st = &HREC(car)->state;
        unused = HREC(car)->type;
    } else {
        m = &AI(car)->motion;
        r = &AI(car)->stats;
        w = &AI(car)->wheels;
        st = &AI(car)->state;
        unused = AI(car)->f02C;
        unused = AI(car)->type;
    }
    w->wheel[0].pos[2] = w->wheel[0].level + w->wheel[0].f1A + w->wheel[0].sway + w->wheel[0].crush;
    if (w->wheel[0].on == 0)
        w->wheel[0].pos[2] += w->wheel[0].f26;
    w->wheel[1].pos[2] = w->wheel[1].level + w->wheel[1].f1A + w->wheel[1].sway + w->wheel[1].crush;
    if (w->wheel[1].on == 0)
        w->wheel[1].pos[2] += w->wheel[1].f26;
    w->wheel[2].pos[2] = w->wheel[2].level + w->wheel[2].f1A + w->wheel[2].sway + w->wheel[2].crush;
    if (w->wheel[2].on == 0)
        w->wheel[2].pos[2] += w->wheel[2].f26;
    w->wheel[3].pos[2] = w->wheel[3].level + w->wheel[3].f1A + w->wheel[3].sway + w->wheel[3].crush;
    if (w->wheel[3].on == 0)
        w->wheel[3].pos[2] += w->wheel[3].f26;

    if (w->wheel[0].pos[2] < w->wheel[1].pos[2])
        mx = w->wheel[1].pos[2];
    else
        mx = w->wheel[0].pos[2];
    lim = mx - 7 * r->track_width / 8;
    if (w->wheel[0].pos[2] < lim) {
        w->wheel[0].pos[2] = lim;
        w->wheel[0].clamped = 1;
        if (w->wheel[0].touch) {
            w->wheel[0].level = w->wheel[0].pos[2] - (w->wheel[0].crush + (w->wheel[0].f1A + w->wheel[0].sway));
            w->wheel[0].f12 -= timerGetFrameTicks();
        }
    } else if (w->wheel[0].touch == 0)
        w->wheel[0].clamped = 0;
    if (w->wheel[1].pos[2] < lim) {
        w->wheel[1].pos[2] = lim;
        w->wheel[1].clamped = 1;
        if (w->wheel[1].touch) {
            w->wheel[1].level = w->wheel[1].pos[2] - (w->wheel[1].crush + (w->wheel[1].f1A + w->wheel[1].sway));
            w->wheel[1].f12 -= timerGetFrameTicks();
        }
    } else if (w->wheel[1].touch == 0)
        w->wheel[1].clamped = 0;

    c = mx < w->wheel[2].pos[2] ? w->wheel[2].pos[2] : mx;
    c = c < w->wheel[3].pos[2] ? w->wheel[3].pos[2] : c;  /* ?: temp + copy back, as retail */
    lim = c - 7 * r->wheelbase / 8;
    if (w->wheel[2].pos[2] < lim) {
        w->wheel[2].pos[2] = lim;
        w->wheel[2].clamped = 1;
        if (w->wheel[2].touch) {
            w->wheel[2].level = w->wheel[2].pos[2] - (w->wheel[2].crush + (w->wheel[2].f1A + w->wheel[2].sway));
            w->wheel[2].f12 -= timerGetFrameTicks();
        }
    } else if (w->wheel[2].touch == 0)
        w->wheel[2].clamped = 0;
    if (w->wheel[3].pos[2] < lim) {
        w->wheel[3].pos[2] = lim;
        w->wheel[3].clamped = 1;
        if (w->wheel[3].touch) {
            w->wheel[3].level = w->wheel[3].pos[2] - (w->wheel[3].crush + (w->wheel[3].f1A + w->wheel[3].sway));
            w->wheel[3].f12 -= timerGetFrameTicks();
        }
    } else if (w->wheel[3].touch == 0)
        w->wheel[3].clamped = 0;

    if (st->all_in_air == 0) {
        vec[0] = 0;
        vec[1] = 0;
        vec[2] = 0;
        if (w->wheel[0].clamped || w->wheel[2].pos[2] - w->wheel[0].pos[2] > r->wheelbase) {
            vec[0] = -2;
            vec[1] = 2;
        }
        if (w->wheel[1].clamped || w->wheel[3].pos[2] - w->wheel[1].pos[2] > r->wheelbase) {
            vec[0] += 2;
            vec[1] += 2;
        }
        if (w->wheel[2].clamped) {
            vec[0] -= 2;
            vec[1] -= 2;
        }
        if (w->wheel[3].clamped) {
            vec[0] += 2;
            vec[1] -= 2;
        }
        if (vec[0] || vec[1]) {
            ang[0] = 0;
            ang[1] = 0;
            ang[2] = m->rot[2];
            RotMatrixYXZ(ang, &mat);
            mathMulTransVec(&mat, vec, out);
            m->pos[0] += out[0];
            m->pos[1] += out[1];
            m->pos[2] += out[2];
        }
    }

    h = (w->wheel[1].pos[2] - w->wheel[0].pos[2] + (w->wheel[3].pos[2] - w->wheel[2].pos[2])) / 2;
    a = ratan2(h, sub_8013FC44(r->track_width * r->track_width - h * h));
    spd = __builtin_abs(m->speed) < r->speed_cap ? r->speed_cap : __builtin_abs(m->speed);

    mx = 0;  /* mx is reused as the "unstable" flag (retail keeps both in s4) */
    if (st->crush_contact || st->crush_peer || w->wheel[0].depth < 15 || w->wheel[1].depth < 15
        || w->wheel[2].depth < 15 || w->wheel[3].depth < 15
        || r->steer_boost_speed < __builtin_abs(m->speed)
        || __builtin_abs(m->steer) > r->steer_tilt_max)
        mx = 1;

    if (st->f12)
        carBarrelRollStep(car, human);
    else if (st->f20 == 0 && st->dead == 0) {
        if ((u8)mx)
            m->rot[1] = a;
        else if (__builtin_abs(m->yaw_rate) <= r->steer_rate_high
            && __builtin_abs(m->speed) < r->accel_launch)
            m->rot[1] = mathBlendAngle(m->rot[1], a, 90);
        else
            m->rot[1] = mathBlendAngle(m->rot[1], a, 80);
        e = 853 - __builtin_abs(m->speed) * 426 / spd;
        if (e < m->rot[1] || (e = -e, m->rot[1] < e))
            m->rot[1] = e;
        if (r->rollover_cap < __builtin_abs(m->speed) && w->wheel[0].touch == 0 && w->wheel[1].touch == 0) {
            s32 t = w->wheel[0].pos[2] - w->wheel[1].pos[2];
            d = -t;
            if (r->rollover < t) {
                st->f12 = 1;
                m->f02 = m->body_rot[2];
                m->spin1 = -1;
                m->f04 = m->body_rot[2] - 1024;
                r->f20 = 0;
            } else if (r->rollover < d) {
                st->f12 = 1;
                m->f02 = m->body_rot[2];
                m->spin1 = 1;
                m->f04 = m->body_rot[2] + 1024;
                r->f20 = 0;
            }
        }
    }

    if (st->f20)
        carTumbleStep(car, human);
    else if (st->dead && st->on_surface == 0)
        m->rot[0] = mathBlendAngle(m->rot[0], 0, 98);
    else {
        h = (w->wheel[2].pos[2] - w->wheel[0].pos[2] + (w->wheel[3].pos[2] - w->wheel[1].pos[2])) / 2;
        a = ratan2(h, sub_8013FC44(r->wheelbase * r->wheelbase - h * h));
        if ((u8)mx)
            m->rot[0] = a;
        else if (__builtin_abs(m->yaw_rate) <= r->steer_rate_high
            && __builtin_abs(m->speed) < r->accel_launch)
            m->rot[0] = mathBlendAngle(m->rot[0], a, 90);
        else
            m->rot[0] = mathBlendAngle(m->rot[0], a, 80);
        e = 853 - __builtin_abs(m->speed) * 426 / spd;
        if (e / 2 < m->rot[0])
            m->rot[0] = e / 2;
        else if (m->rot[0] < -e)
            m->rot[0] = -e;
    }
    m->pos[2] = (w->wheel[0].pos[2] + w->wheel[1].pos[2] + w->wheel[2].pos[2] + w->wheel[3].pos[2]) / 4;
    carSuspensionUpdate(car, human);
}
