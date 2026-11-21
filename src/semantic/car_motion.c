/* Car motion helpers (800DF3EC..): shared by human and AI cars.
 *
 * Most take (car record, human) and pick the matching blocks, which sit at
 * different offsets in the two record layouts (see cars.h):
 *   motion  human +0x6DC, AI +0x71C  (velocity, position, angles, matrices)
 *   state   human +0x09C, AI +0x06C  (flags; see camera.c TM2CarState)
 *   tilt    human +0x114, AI +0x09C  (suspension/bump angles)
 *   stats   human +0x14C, AI +0x4FC  (handling constants)
 * A shared car layout in the port can drop the selection. */
#include "semantic/cars.h"

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Matrix;

typedef struct {
    s16 f00;                /* +0x00 angle */
    u8 pad_02[6];
    s16 vel[3];             /* +0x08 velocity in car space; vel[1] is the speed */
    u8 pad_0E[2];
    s16 spin[3];            /* +0x10 angular velocity */
    u8 pad_16[10];
    s32 pos[3];             /* +0x20 */
    s32 rot[3];             /* +0x2C */
    s32 body_rot[3];        /* +0x38 visible body angles */
    s32 body_spin[3];       /* +0x44 */
    TM2Matrix rot_m;        /* +0x50 from rot */
    TM2Matrix body_m;       /* +0x70 from rot + tilt */
    TM2Matrix body_rot_m;   /* +0x90 from body_rot */
} TM2Motion;

typedef struct {
    u8 f00;
    u8 f01;                 /* +0x01 */
    u8 f02;                 /* +0x02 */
    u8 pad_03b;
    u8 f04;                 /* +0x04 */
    u8 pad_05[6];
    u8 f0B;                 /* +0x0B */
    u8 f0C;                 /* +0x0C */
    u8 pad_0D[12];
    u8 f19;                 /* +0x19 */
    u8 pad_1A[2];
    u8 f1C;                 /* +0x1C */
} TM2MotionState;

typedef struct {
    u8 pad_00[0x16];
    s16 a[2];               /* +0x16 */
    u8 pad_1A[2];
    s16 b[2];               /* +0x1C */
    u8 pad_20[0x10];
    s16 c[2];               /* +0x30 */
} TM2Tilt;

typedef struct {
    u8 pad_00[0x42];
    s16 lift_flat;          /* +0x42 */
    u8 pad_44[2];
    s16 lift_steep;         /* +0x46 */
    s16 lift_mid;           /* +0x48 */
    u8 pad_4A[0x1A];
    s16 grip;               /* +0x64 */
    u8 pad_66[8];
    s16 grip_scale;         /* +0x6E */
    s16 grip_base;          /* +0x70 */
    u8 pad_72[8];
    s16 turn_damp;          /* +0x7A percent/2 */
    u8 pad_7C[0x38];
    s32 spin_speed;         /* +0xB4 */
    s32 min_speed;          /* +0xB8 */
    u8 pad_BC[4];
    s32 slide_speed;        /* +0xC0 */
    u8 pad_C4[8];
    s32 slow;               /* +0xCC */
} TM2Stats;

/* Human car record view (0x814). */
typedef struct {
    u8 pad_000[0x9C];
    TM2MotionState st;      /* +0x09C (0x1D bytes) */
    u8 pad_0B9[0x17];
    u8 f0D0, f0D1;          /* +0x0D0 */
    u8 pad_0D2[4];
    u8 f0D6;                /* +0x0D6 */
    u8 pad_0D7[0xDB];
    s16 f1B2;               /* +0x1B2 */
    u8 pad_1B4[0x58];
    s32 f20C;               /* +0x20C */
    u8 pad_210[0x29];
    u8 f239;                /* +0x239 */
    u8 pad_23A[0x383];
    u8 f5BD;                /* +0x5BD */
    u8 pad_5BE[0x47];
    u8 f605;                /* +0x605 */
} TM2MotionHuman;

#define MOTION(car, human) ((TM2Motion *)((u8 *)(car) + ((human) ? 0x6DC : 0x71C)))

extern void mathBoundSAngle(s16 *);
extern void mathBoundAngle(s32 *);
extern void mathBoundVector(s32 *);
extern void *RotMatrixYXZ(s16 *, TM2Matrix *);
extern void mathMulTransVec(TM2Matrix *, s32 *, s32 *);
extern s32 timerGetFrameTicks(void);
extern s32 GsGetWorkBase_0(void);
extern u8 carContactApplyObject();
extern void carContactResolve();

void carSlideEndIfSlow(void *car, u8 human);
void carGripSlowdown(void *car, u8 human);
void humanCarSlideStep(TM2MotionHuman *car);
void carBodySpinClear(TM2Motion *m);
void carBodyRealign();
void carSlideDrag(void *car, u8 human);
void humanCarSlideStart(TM2MotionHuman *car);
void carBodyFollowCar(TM2Motion *m, TM2MotionState *st);

/* @800DF3EC: integrate the angles and rebuild the three matrices. */
void __attribute__((section(".text_800DF3EC"))) CarRotUpdate(void *car, u8 human)
{
    TM2Motion *m;
    TM2Tilt *t;
    s16 r[4];

    if (human) {
        m = (TM2Motion *)&HREC(car)->motion;
        t = (TM2Tilt *)&HREC(car)->aim;
    } else {
        m = (TM2Motion *)&AI(car)->motion;
        t = (TM2Tilt *)&AI(car)->aim;
    }
    m->rot[0] += m->spin[0];
    m->rot[1] += m->spin[1];
    m->rot[2] += m->spin[2];
    m->body_rot[0] += m->body_spin[0];
    m->body_rot[1] += m->body_spin[1];
    m->body_rot[2] += m->body_spin[2];
    mathBoundSAngle(&m->f00);
    mathBoundVector(m->rot);
    mathBoundVector(m->body_rot);
    r[0] = m->body_rot[0];
    r[1] = m->body_rot[1];
    r[2] = m->body_rot[2];
    RotMatrixYXZ(r, &m->body_rot_m);
    r[0] = m->rot[0];
    r[1] = m->rot[1];
    r[2] = m->rot[2];
    RotMatrixYXZ(r, &m->rot_m);
    r[0] += t->c[0];
    r[0] += t->b[0];
    r[0] += t->a[0];
    r[1] += t->c[1];
    r[1] += t->b[1];
    r[1] += t->a[1];
    RotMatrixYXZ(r, &m->body_m);
}

/* @800DF594: move by the velocity; height follows the pitch. */
void __attribute__((section(".text_800DF594"))) carMoveByVelocity(void *car, u8 human)
{
    TM2Motion *m;
    TM2Stats *st;
    s32 k, ang, d;
    s32 v[3];
    s32 out[4];

    if (human) {
        m = (TM2Motion *)&HREC(car)->motion;
        st = (TM2Stats *)&HREC(car)->stats;
    } else {
        m = (TM2Motion *)&AI(car)->motion;
        st = (TM2Stats *)&AI(car)->stats;
    }
    v[0] = m->vel[0] / 32;
    v[1] = m->vel[1] / 32;
    v[2] = m->vel[2] / 32;
    mathMulTransVec(&m->body_rot_m, v, out);
    m->pos[0] += out[0];
    m->pos[1] += out[1];
    k = st->lift_mid / 2 + 16;
    ang = __builtin_abs(m->rot[1]);
    mathBoundAngle(&ang);
    if (ang < 1024)
        d = k * ang / 1024 + (st->lift_flat - st->lift_flat * ang / 1024);
    else {
        ang -= 1024;
        d = k - k * ang / 1024;
        d += st->lift_steep * ang / 1024;
    }
    m->pos[2] += d;
}

/* @800DF770 */
void __attribute__((section(".text_800DF770"))) humanCarSlideUpdate(TM2MotionHuman *car)
{
    if (car->st.f01) {
        if (car->f239 && !car->st.f19)
            carSlideEndIfSlow((u8 *)car, 1);
        else
            humanCarSlideStep(car);
    } else
        humanCarSlideStart(car);
}

/* @800DF7E0 */
void __attribute__((section(".text_800DF7E0"))) carSlideEndIfSlow(void *car, u8 human)
{
    TM2MotionState *s;
    TM2Motion *m;
    TM2Stats *st;

    if (human) {
        s = (TM2MotionState *)&HREC(car)->state;
        m = (TM2Motion *)&HREC(car)->motion;
        st = (TM2Stats *)&HREC(car)->stats;
    } else {
        s = (TM2MotionState *)&AI(car)->state;
        m = (TM2Motion *)&AI(car)->motion;
        st = (TM2Stats *)&AI(car)->stats;
    }
    if (__builtin_abs(m->vel[1]) < st->spin_speed)
        carBodyFollowCar(m, s);
}

/* @800DF84C: stop the body spin and align the body with the car. */
void __attribute__((section(".text_800DF84C"))) carSpinStop(void *car, u8 human)
{
    TM2MotionState *s;
    TM2Motion *m;

    if (human) {
        s = (TM2MotionState *)&HREC(car)->state;
        m = (TM2Motion *)&HREC(car)->motion;
    } else {
        s = (TM2MotionState *)&AI(car)->state;
        m = (TM2Motion *)&AI(car)->motion;
    }
    s->f01 = 0;
    m->spin[2] = 0;
    m->body_spin[2] = 0;
    m->body_rot[0] = m->rot[0];
    m->body_rot[1] = m->rot[1];
    m->body_rot[2] = m->rot[2];
}

/* @800DF8A0 */
void __attribute__((section(".text_800DF8A0"))) humanCarSlideStart(TM2MotionHuman *car)
{
    if (!((car->f0D6 && car->f20C < __builtin_abs(MOTION(car, 1)->vel[1]))
          || (car->f20C < __builtin_abs(MOTION(car, 1)->vel[1]) && __builtin_abs(MOTION(car, 1)->spin[2]) > car->f1B2)
          || car->f0D6))
        carBodyFollowCar(MOTION(car, 1), &car->st);
    else {
        car->st.f01 = 1;
        carBodySpinClear(MOTION(car, 1));
    }
    carGripSlowdown((u8 *)car, 1);
}

/* @800DF990: let the body keep the car's spin (unless state f0C). */
void __attribute__((section(".text_800DF990"))) carBodyFollowCar(TM2Motion *m, TM2MotionState *s)
{
    s->f01 = 0;
    s->f19 = 1;
    if (s->f0C)
        m->body_spin[2] = 0;
    else {
        m->body_spin[0] = m->spin[0];
        m->body_spin[1] = m->spin[1];
        m->body_spin[2] = m->spin[2];
        m->body_rot[0] = m->rot[0];
        m->body_rot[1] = m->rot[1];
        m->body_rot[2] = m->rot[2];
    }
}

/* @800DFA20: state f02 = fast and the body turned more than 45 degrees
 * away from the car. */
void __attribute__((section(".text_800DFA20"))) carUpdateSidewaysFlag(void *car, u8 human)
{
    TM2Motion *m;
    TM2MotionState *s;
    u8 flag;
    s32 a;

    if (human) {
        m = (TM2Motion *)&HREC(car)->motion;
        s = (TM2MotionState *)&HREC(car)->state;
        flag = HREC(car)->state.f01;
    } else {
        m = (TM2Motion *)&AI(car)->motion;
        s = (TM2MotionState *)&AI(car)->state;
        flag = DRV(car)->sharp_turn;
    }
    if (flag && m->vel[1] > (s16)timerGetFrameTicks() * 760 / 100 * 32) {
        a = m->body_rot[2] - m->rot[2];
        mathBoundAngle(&a);
        if (__builtin_abs(a) > 512) {
            s->f02 = 1;
            return;
        }
    }
    s->f02 = 0;
}

/* @800DFB28: slow the car down when its angle f00 exceeds the grip
 * (human cars use the plain grip, AI cars twice it, unless boosted). */
void __attribute__((section(".text_800DFB28"))) carGripSlowdown(void *car, u8 human)
{
    TM2Stats *st;
    TM2Motion *m;
    s32 lim, grip, d;

    if (human) {
        st = (TM2Stats *)&HREC(car)->stats;
        m = (TM2Motion *)&HREC(car)->motion;
        if (HREC(car)->input.slide_inc || HREC(car)->input.slide_dec)
            grip = st->grip * st->grip_scale;
        else
            grip = st->grip;
    } else {
        st = (TM2Stats *)&AI(car)->stats;
        m = (TM2Motion *)&AI(car)->motion;
        if (AI(car)->input.slide_inc || AI(car)->input.slide_dec)
            grip = st->grip * st->grip_scale;
        else
            grip = st->grip * 2;
    }
    if (m->vel[1] < st->min_speed)
        lim = st->min_speed;
    else
        lim = m->vel[1];
    if (st->slide_speed < __builtin_abs(m->vel[1])) {
        d = (grip - st->grip_base) * __builtin_abs(m->vel[1]) / lim;
        grip -= d;
        if (grip < __builtin_abs(m->f00)) {
            if (m->vel[1] > 0)
                m->vel[1] = m->vel[1] - st->slow;
            else if (m->vel[1] < 0)
                m->vel[1] = m->vel[1] + st->slow;
        }
    }
}

/* @800DFD14 */
void __attribute__((section(".text_800DFD14"))) humanCarSlideStep(TM2MotionHuman *car)
{
    u8 ok = 1;

    /* Both flags: never; one flag: half the time. */
    if (car->f5BD) {
        if (car->f605) {
            ok = 0;
            goto test;
        }
    } else if (!car->f605)
        goto test;
    if (GsGetWorkBase_0() & 1)
        ok = 0;
test:
    if (ok) {
        if (car->f0D6) {
            car->st.f01 = 1;
            carBodySpinClear(MOTION(car, 1));
        }
        if (((u8 *)car)[0xB8] || !((u8 *)car)[0xA7])
            carBodyRealign((u8 *)car, 1);
        carSlideDrag((u8 *)car, 1);
    }
}

/* @800DFDF0 */
void __attribute__((section(".text_800DFDF0"))) carBodySpinClear(TM2Motion *m)
{
    m->body_spin[2] = 0;
}

/* Driver controls (human +0xD0, AI +0x44). */
typedef struct {
    u8 pad_00[3];
    u8 f03;                 /* +0x03 */
    u8 pad_04[11];
    u8 f0F;                 /* +0x0F */
    u8 f10;                 /* +0x10 */
} TM2MotionCtl;

/* @800DFDFC: turn the body back towards the direction of travel while
 * the car is fast enough; once aligned the body follows the car again. */
void __attribute__((section(".text_800DFDFC"))) carBodyRealign(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    TM2MotionState *s;
    TM2Motion *m;
    TM2Stats *st;
    TM2MotionCtl *ctl;
    struct TM2CarContact *x;
    s32 a, l, r, turn;

    if (human) {
        st = (TM2Stats *)&HREC(car)->stats;
        m = (TM2Motion *)&HREC(car)->motion;
        s = (TM2MotionState *)&HREC(car)->state;
        ctl = (TM2MotionCtl *)&HREC(car)->input;
        x = &HREC(car)->contact;
    } else {
        st = (TM2Stats *)&AI(car)->stats;
        m = (TM2Motion *)&AI(car)->motion;
        s = (TM2MotionState *)&AI(car)->state;
        ctl = (TM2MotionCtl *)&AI(car)->input;
        x = &AI(car)->contact;
    }
    if (__builtin_abs(m->vel[1]) < st->spin_speed) {
        carBodyFollowCar(m, s);
        return;
    }
    a = m->rot[2] - m->body_rot[2];
    mathBoundAngle(&a);
    if (__builtin_abs(a) >= 1423 && m->vel[1] > (s16)timerGetFrameTicks() * 380 / 100 * 32)
        return;
    if (a > 0) {
        if (ctl->f10)
            l = 0;
        else if (ctl->f0F)
            l = (s16)timerGetFrameTicks() * 5;
        else
            l = (s16)timerGetFrameTicks() * 2;
    } else {
        if (ctl->f10)
            l = (s16)timerGetFrameTicks() * 5;
        else if (ctl->f0F)
            l = 0;
        else
            l = (s16)timerGetFrameTicks() * 2;
    }
    if (l > 0) {
        if (st->slide_speed < __builtin_abs(m->vel[1]))
            r = 0;
        else if (__builtin_abs(m->vel[1]) > st->slide_speed / 2)
            r = (s16)timerGetFrameTicks() * 2;
        else
            r = (s16)timerGetFrameTicks() * 5;
    } else {
        if (ctl->f03) {
            timerGetFrameTicks();
            r = 0;
        } else
            r = 0;
    }
    if (x->f03) {
        l /= 2;
        r /= 2;
    }
    turn = ((r + l) / 2 << 12) / 360;
    if (ctl->f03)
        turn *= 3;
    else if (s->f04)
        turn *= 2;
    if (human && s->f1C && s->f0B)
        turn = (s16)timerGetFrameTicks() * 5;
    turn -= turn * st->turn_damp / 200;
    if (a > 0) {
        m->body_rot[2] += turn;
        mathBoundAngle(&m->body_rot[2]);
        a = m->rot[2] - m->body_rot[2];
        mathBoundAngle(&a);
        if (a < 0)
            carBodyFollowCar(m, s);
    } else {
        m->body_rot[2] -= turn;
        mathBoundAngle(&m->body_rot[2]);
        a = m->rot[2] - m->body_rot[2];
        mathBoundAngle(&a);
        if (a > 0)
            carBodyFollowCar(m, s);
    }
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Contact block (human +0x5A4, AI +0x5E4): four wheel entries of 0x48. */
typedef struct {
    u8 pad_00[0x2C];
    s16 drag;               /* +0x2C */
    u8 pad_2E[0x12];
    struct { s16 f00; u8 pad[0x46]; } wheel[4];   /* +0x40 */
} TM2Contacts;

/* @800E0208: slow the car; more when the body is turned against the
 * direction of travel, plus drag from the wheels. */
void __attribute__((section(".text_800E0208"))) carSlideDrag(void *car, u8 human)
{
    TM2MotionState *s;
    TM2Stats *st;
    TM2Motion *m;
    TM2Contacts *c;
    s32 a, aa, slow, q, d;
    /* n holds first the TM2MotionCtl pointer (32-bit on the PSX), later the
     * wheel minimum: one variable, as retail (both in s0; the shared
     * variable's priority puts st in s1). A union changes the code. */
    s32 n;

    if (human) {
        s = (TM2MotionState *)&HREC(car)->state;
        st = (TM2Stats *)&HREC(car)->stats;
        m = (TM2Motion *)&HREC(car)->motion;
        n = (s32)(&HREC(car)->input);
        c = (TM2Contacts *)&HREC(car)->wheels;
    } else {
        s = (TM2MotionState *)&AI(car)->state;
        st = (TM2Stats *)&AI(car)->stats;
        m = (TM2Motion *)&AI(car)->motion;
        n = (s32)(&AI(car)->input);
        c = (TM2Contacts *)&AI(car)->wheels;
    }
    a = m->rot[2] - m->body_rot[2];
    mathBoundAngle(&a);
    aa = __builtin_abs(a);
    if (aa < 682)
        slow = st->slow * 2 / 3;
    else if (aa < 1024)
        slow = st->slow * 3 / 4;
    else if (aa < 1422)
        slow = st->slow;
    else if (((TM2MotionCtl *)n)->f03)
        slow = st->slow * 3;
    else if (s->f04)
        slow = st->slow * 2;
    else
        slow = st->slow * 3 / 2;
    if (m->vel[1] && !s->f0B) {
        d = c->drag * (s16)timerGetFrameTicks() / 32;
        d *= 32;
        if (d > 0) {
            q = st->slow * (d << 1) / __builtin_abs(m->vel[1]);
            if (slow < q)
                q = slow;
            else if (q < -slow)
                q = -slow;
            n = MIN(c->wheel[0].f00, c->wheel[1].f00);
            n = MIN(n, c->wheel[2].f00);
            n = MIN(n, c->wheel[3].f00);
            n /= (s16)timerGetFrameTicks();
            if ((unsigned)(n - 1) < 4)
                q = q * 5 / n;
            slow += q;
        }
    }
    if (m->vel[1] > 0) {
        m->vel[1] -= slow;
        if (m->vel[1] < 0)
            carBodyFollowCar(m, s);
    } else {
        m->vel[1] += slow;
        if (m->vel[1] > 0)
            carBodyFollowCar(m, s);
    }
}

/* @800E05F0: test the first six contact probes (contact block, human
 * +0x234, AI +0xD8); a probe consumed by carContactApplyObject flags its
 * contact record (f02 set, f01 cleared), the rest go to carContactResolve. */
void __attribute__((section(".text_800E05F0"))) carTestContactPoints(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    struct TM2CarContact *w;
    u8 first = 1;
    s32 i;

    w = human ? &HREC(car)->contact : &AI(car)->contact;
    for (i = 0; i < 6; i++) {
        if (w->probe[i].active) {
            if (carContactApplyObject(i, car, human, first)) {
                first = 0;
                w->ground[i].f02 = 1;
                w->ground[i].f01 = 0;
            } else {
                w->ground[i].f02 = 0;
                carContactResolve(i, car, human);
            }
        }
    }
}
