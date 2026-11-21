/* Car-car collision (800E184C), called from the car contact pass.
 * Names stay address-based. */
#include "semantic/cars.h"
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Matrix;

extern s32 aiGetDifficulty(void);
extern u8 carCrashStart();
extern void weaponSlamStateInit(struct TM2CarWeapons *weapons);
extern void mathBoundAngle(s32 *);
extern void mathBoundSAngle(s16 *);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(s32 *pos);
extern void soundLocateForView(s32 *pos, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern s32 *carPairDirection(s32, s32);
extern int ratan2(int, int);
extern s32 sub_8013FC44(s32);
extern s32 timerGetFrameTicks(void);
extern void *RotMatrixYXZ(s16 *, TM2Matrix *);
extern void mathMulTransVec(TM2Matrix *, s32 *, s32 *);

#define SCRAPE_SOUND(id, vol) { \
    s32 x = PadChkMtap(); \
    s32 y = soundGetListenerPan(); \
    soundPlay(id, x, y, vol); \
}

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AREC(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

#define MIN_HIT() ((s16)timerGetFrameTicks() * 190 / 100 * 32)

/* @800E184C: car-to-car collision between `car` and `other` (side = which
 * side of the car was hit). A hit that starts a crash (carCrashStart, given a
 * fifth `side` arg it ignores) returns 1 at once. Otherwise it plays the
 * crash sounds by closing speed and fills a free object-contact record on
 * each car (else the last one): a shared push direction, hit speeds split by
 * weight and clamped to each car's max speed, a turn kick for the car hit
 * side-on, and a spin push from the push vector's offset to each car's
 * center; returns 0. Repeating the last hit between the same two cars
 * instead resets both records to the cars' own heading/speed and returns 1. `angle` is unused; `strength` is clamped but never used
 * afterwards (retail keeps the dead clamp). */
u8 __attribute__((section(".text_800E184C"))) carCarCollision(car, human, other, other_human, angle, side, strength)
void *car;              /* human record when human is set, else AI */
u8 human;
void *other;            /* human record when other_human is set, else AI */
u8 other_human;
s32 angle;
u32 side;
s16 strength;
{
    s16 r[4];
    s32 v[3];
    s32 out[4];
    TM2Matrix mat;
    s32 diff;
    s32 tmp;
    u8 result;
    u8 flag;
    s32 id, oid, id2, oid2;
    s32 d1, d2;
    u8 *ai, *ai2;       /* AI +0x034 (active); a TM2AIRecord pointer changes code */
    struct TM2CarStatsBlock *st, *st2;
    struct TM2CarMotion *m, *m2;
    struct TM2CarContact *c, *c2;
    struct TM2CarState *state, *state2;
    struct TM2CarWheels *w, *w2;
    s32 r2;
    u8 front, back;
    struct TM2CarWeapons *obj;
    struct TM2ContactRec *hit, *hit2;
    s32 mode;
    s32 i;
    s32 a, b, speed, f;
    s32 *p1, *p2;
    s32 ang1, ang2;
    s32 wa, wb;
    s32 x, y, x1, y1;
    s32 e;
    s32 ang;

    result = 0;
    if (human) {
        state = &HREC(car)->state;
        c = &HREC(car)->contact;
        id = HREC(car)->player;
        mode = HREC(car)->type;
        w = &HREC(car)->wheels;
        id2 = HREC(car)->slot;
        st = &HREC(car)->stats;
        m = &HREC(car)->motion;
        obj = &HREC(car)->weapons;
        ai = 0;
    } else {
        state = &AREC(car)->state;
        st = &AREC(car)->stats;
        c = &AREC(car)->contact;
        id = AREC(car)->index;
        mode = AREC(car)->type;
        m = &AREC(car)->motion;
        w = &AREC(car)->wheels;
        ai = &AREC(car)->active;
        id2 = AREC(car)->slot;
        obj = 0;
    }
    if (other_human) {
        state2 = &HREC(other)->state;
        st2 = &HREC(other)->stats;
        c2 = &HREC(other)->contact;
        m2 = &HREC(other)->motion;
        oid = HREC(other)->player;
        w2 = &HREC(other)->wheels;
        oid2 = HREC(other)->slot;
        ai2 = 0;
    } else {
        state2 = &AREC(other)->state;
        st2 = &AREC(other)->stats;
        c2 = &AREC(other)->contact;
        m2 = &AREC(other)->motion;
        oid = AREC(other)->index;
        w2 = &AREC(other)->wheels;
        oid2 = AREC(other)->slot;
        ai2 = &AREC(other)->active;
    }

    hit = &c->object[5];
    for (i = 0; i < 6; i++) {
        if (!c->object[i].f00) {
            hit = &c->object[i];
            break;
        }
    }
    hit2 = &c2->object[5];
    for (i = 0; i < 6; i++) {
        if (!c2->object[i].f00) {
            hit2 = &c2->object[i];
            break;
        }
    }

    if (st->mounts_cars
#ifdef TM2_PORT_BALANCE
        && !portBalanceNoMount(id2, mode)   /* request 40 (BALANCE.md 3.4-3.6): Axel, Minion (bits 4, 5) ram like a
                                               normal car; a borrowing Hammerhead (bit 11) too */
#endif
        ) {
        if (state->crush_contact) {
            result = 1;
            goto checked;
        }
        if (side < 2 || side == 4)
            front = 1;
        else
            front = 0;
        back = side - 2 < 2;
        if ((front && m->vel[1] > 0) || (back && m->vel[1] < 0)) {
            if (carCrashStart(car, human, other, other_human, side))
                result = 1;
        }
    } else if (state2->crush_peer)
        result = 1;
checked:
    if (aiGetDifficulty() <= 0 && mode == 192 && (side < 2 || side == 4)
        && m->vel[1] > 0 && obj && obj->slam && !obj->slam->active) {
        weaponSlamStateInit(obj);
        obj->slam->half_damage = 1;
        obj->slam->active = 1;
        if (!human)
            DRV(car)->f458 = 1;
    }
    if (result)
        goto out;

    hit->turn = 0;
    hit2->turn = 0;
    if (__builtin_abs(m->vel[1]) > st->accel_launch)
        a = m->fC4;
    else
        a = m->rot[2];
    if (__builtin_abs(m2->vel[1]) > st2->accel_launch)
        b = m2->fC4;
    else
        b = m2->rot[2];
    diff = a - b;
    mathBoundAngle(&diff);

    speed = __builtin_abs(m->vel[0] + m2->vel[0]) + __builtin_abs(m->vel[1] + m2->vel[1])
          + __builtin_abs(m->vel[2] + m2->vel[2]);
    if (!viewGetSplitMode())
        soundLocateForView0(m->pos);
    else
        soundLocateForView(m->pos, -1);
    if ((unsigned)(speed - 1000) < 1000) {
        SCRAPE_SOUND(5, speed * 48 / 2000 + 51);
    } else {
        SCRAPE_SOUND(5, 99);
    }
    if (speed < 1000) {
        SCRAPE_SOUND(3, speed * 49 / 1000 + 50);
    } else {
        SCRAPE_SOUND(3, 99);
    }

    p1 = carPairDirection(id2, oid2);
    p2 = carPairDirection(oid2, id2);
    if (!p1[1])
        p1[1] = 1;
    ang1 = ratan2(p1[0], p1[1]);
    ang2 = ratan2(p2[0], p2[1]);
    hit->f0A = ang1;
    hit2->f0A = ang2;
    hit2->other = id2;
    hit->other = oid2;

    if (strength >= 9) {
        e = __builtin_abs(m->vel[1]);
        if (e < __builtin_abs(m2->vel[1]))
            e = __builtin_abs(m2->vel[1]);
        if (e / 32 < strength)
            strength = e / 32;
        /* Dead test: keeps the clamp alive; its branch is removed after
         * reload, leaving retail's lone reload of strength. */
        if (strength > 0)
            e = 0;
    }

    wa = 100 * st->mass / (st->mass + st2->mass);
    d2 = b - ang2;
    d1 = a - ang1;
    wb = 100 - wa;
    flag = __builtin_abs(d1) < __builtin_abs(d2);
    if (wb < wa) {
        wb = wb * 100 / wa;
        wa = 100;
    } else {
        wa = wa * 100 / wb;
        wb = 100;
    }

    x1 = w->move[0] * wa;
    y1 = w->move[1] * wa;
    x = (x1 + w2->move[0] * wb) / 200;
    y = (y1 + w2->move[1] * wb) / 200;
    ang = ratan2(x, y);
    hit->angle = ang;
    hit2->angle = ang;
    f = sub_8013FC44(x * x + y * y) * 32;
    if (f < MIN_HIT())
        f = MIN_HIT();
    hit->speed = f;
    hit2->speed = f;
    if (wb + 10 < wa) {
        hit2->speed = f * wa / wb;
        if (f * 2 < hit2->speed)
            hit2->speed = f * 2;
    } else if (wa + 10 < wb) {
        hit->speed = f * wb / wa;
        if (f * 2 < hit->speed)
            hit->speed = f * 2;
    }

    if (st->speed_cap < st2->speed_cap)
        e = st2->speed_cap;
    else
        e = st->speed_cap;
    hit->push = f * 120 / e + 40;
    hit2->push = hit->push;

    if (hit->speed > st->speed_cap)
        hit->speed = st->speed_cap;
    else if (hit->speed < -st->speed_cap)
        hit->speed = -st->speed_cap;
    if (hit2->speed > st2->speed_cap)
        hit2->speed = st2->speed_cap;
    else if (hit2->speed < -st2->speed_cap)
        hit2->speed = -st2->speed_cap;

    hit->f10 = 0;
    hit2->f10 = 0;
    if (flag) {
        hit2->f10 = hit2->angle - m2->rot[2];
        mathBoundSAngle(&hit2->f10);
        if (d1 * hit2->f10 > 0) {
            if (__builtin_abs(hit2->f10) < 512) {
                if (hit2->f10 < 0)
                    hit2->f10 = hit2->f10 + 1024;
                else
                    hit2->f10 = hit2->f10 - 1024;
            } else
                hit2->f10 = hit2->f10 + 2048;
            mathBoundSAngle(&hit2->f10);
        }
        e = __builtin_abs(m->speed) * 2 * wa / st->speed_cap;
        if (e > 100) {
            e = 100;
            hit2->push *= 2;
        }
        hit2->f10 = hit2->f10 * e / 100;
        if (__builtin_abs(d1) < 227) {
            hit2->f10 = hit2->f10 * __builtin_abs(d1) / 227;
            hit2->speed += hit2->speed / 20;
        } else {
            hit2->speed -= hit2->speed / 20;
            hit->speed += hit->speed / 20;
        }
    } else {
        hit->f10 = hit->angle - m->rot[2];
        mathBoundSAngle(&hit->f10);
        if (d2 * hit->f10 > 0) {
            if (__builtin_abs(hit->f10) < 512) {
                if (hit->f10 < 0)
                    hit->f10 = hit->f10 + 1024;
                else
                    hit->f10 = hit->f10 - 1024;
            } else
                hit->f10 = hit->f10 + 2048;
            mathBoundSAngle(&hit->f10);
        }
        e = __builtin_abs(m2->speed) * 2 * wb / st2->speed_cap;
        if (e > 100) {
            e = 100;
            hit->push *= 2;
        }
        hit->f10 = hit->f10 * e / 100;
        if (__builtin_abs(d2) < 227) {
            hit->f10 = hit->f10 * __builtin_abs(d2) / 227;
            hit->speed += hit->speed / 20;
        } else {
            hit->speed -= hit->speed / 20;
            hit2->speed += hit2->speed / 20;
        }
    }

    if (c->last == oid && c2->last == id && c->f0A < 10) {
        result = 1;
        if (c->push < 80) {
            c->push = 80;
            c2->push = 80;
        }
    }

    if (!result) {
        r[0] = 0;
        r[1] = 0;
        r[2] = hit->angle;
        RotMatrixYXZ(r, &mat);
        v[0] = 0;
        v[1] = hit->speed / 32;
        v[2] = 0;
        mathMulTransVec(&mat, v, out);
        x = w->move[0] - out[0];
        y = w->move[1] - out[1];
        f = sub_8013FC44(x * x + y * y);
        f *= 32;
        hit->damage = f * 12 / st2->speed_cap;
        r[0] = 0;
        r[1] = 0;
        r[2] = hit2->angle;
        RotMatrixYXZ(r, &mat);
        v[0] = 0;
        v[1] = hit2->speed / 32;
        v[2] = 0;
        mathMulTransVec(&mat, v, out);
        x = w2->move[0] - out[0];
        y = w2->move[1] - out[1];
        f = sub_8013FC44(x * x + y * y);
        f *= 32;
        hit2->damage = f * 12 / st->speed_cap;
        if (flag) {
            hit->damage /= 2;
            hit2->damage *= 2;
        } else {
            hit2->damage /= 2;
            hit->damage *= 2;
        }
        d1 = __builtin_abs(d1);
        if (d1 < 1024)
            hit->damage = hit->damage * d1 / 1024;
        d2 = __builtin_abs(d2);
        if (d2 < 1024)
            hit2->damage = hit2->damage * d2 / 1024;
        c->last = oid;
        c2->last = id;
    }

    if (ai) {
        tmp = m->body_rot[2] - hit->angle;
        mathBoundAngle(&tmp);
        if (__builtin_abs(tmp) > 512)
            ai[11] = 1;     /* AI +0x03F spun */
    }
    if (ai2) {
        tmp = m2->body_rot[2] - hit2->angle;
        mathBoundAngle(&tmp);
        if (__builtin_abs(tmp) > 512)
            ai2[11] = 1;    /* AI +0x03F spun */
    }

    if (result) {
        hit->f00 = 0;
        hit2->f00 = 0;
        hit->turn = 0;
        hit2->turn = 0;
        hit->angle = m->body_rot[2];
        hit2->angle = m2->body_rot[2];
        hit->speed = m->speed;
        hit2->speed = m2->speed;
    } else {
        state->f19 = 0;
        state2->f19 = 0;
        hit->f00 = 1;
        hit2->f00 = 1;
        hit->f01 = 0;
        hit2->f01 = 0;
        hit->side = -1;
        hit2->side = -1;
    }
out:
    return result;
}
