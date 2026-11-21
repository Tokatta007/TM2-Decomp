/* Game code (car contacts and tracking). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 35: TM2_SPECIAL_TYPE */
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

extern s32 rand(void);
extern s32 timerGetFrameTicks(void);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(s32 *pos);
extern void soundLocateForView(s32 *pos, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void carApplyDamage(s32, s32, s32 *, s32, s32);
extern s32 *carPairDirection(s32 other, s32 id);
extern struct TM2Object *humanCarObject(s32 player);
extern struct TM2Object *aiCarObject(s32 id);
extern s32 carCrushDamageAmount(void);

#define RSHAKE(p, i) \
    (p)->wheel[i].shake = rand() % ost->ride_height * (s16)timerGetFrameTicks();

/* @800E6D14: start a crash between car and other: mark the car's crash
 * state, spin the other car (unless the crash kind is 240), set up the
 * crash stats and random shake of both cars' four contact slots, and play
 * the crash sound.  Returns 0 when the other car was spun (or the human
 * crash budget ran out), else 1. */
u8 __attribute__((section(".text_800E6D14"))) carCrashStart(car, human, other, other_human)
void *car;              /* human record when human is set, else AI */
u8 human;
void *other;            /* human record when other_human is set, else AI */
u8 other_human;
{
    u8 ret;
    s32 oid, id, oid2, okind;
    struct TM2CarWheels *ow;
    struct TM2CarState *state;
    struct TM2CarStatsBlock *st;
    struct TM2CarMotion *m;
    struct TM2CarWheels *w;
    struct TM2CarWeapons *q;
    s32 kind;
    struct TM2CarState *ostate;
    struct TM2CarStatsBlock *ost;
    s32 *dir;
    struct TM2Object *type;
    s32 x;

    ret = 1;
    if (human) {
        state = &HREC(car)->state;
        st = &HREC(car)->stats;
        kind = HREC(car)->type;
        id = HREC(car)->slot;
        m = &HREC(car)->motion;
        w = &HREC(car)->wheels;
        q = &HREC(car)->weapons;
    } else {
        state = &AI(car)->state;
        st = &AI(car)->stats;
        kind = AI(car)->type;
        id = AI(car)->slot;
        m = &AI(car)->motion;
        w = &AI(car)->wheels;
        q = &AI(car)->weapons;
    }
    if (other_human) {
        oid = HREC(other)->player;
        ostate = &HREC(other)->state;
        ost = &HREC(other)->stats;
        okind = HREC(other)->type;
        oid2 = HREC(other)->slot;
        ow = &HREC(other)->wheels;
    } else {
        oid = AI(other)->index;
        ostate = &AI(other)->state;
        ost = &AI(other)->stats;
        okind = AI(other)->type;
        oid2 = AI(other)->slot;
        ow = &AI(other)->wheels;
    }
    if (state->crush_contact)
        return 1;
    if (human && TM2_SPECIAL_TYPE(id, kind) == 32) {   /* request 35: a borrowing Hammerhead pays no charge */
        if (q->ammo[0] <= 0)
            return 0;
        q->ammo[0] -= 1;
    }
    if (TM2_SPECIAL_TYPE(id, kind) == 32) {
        if (TM2_CAR_FEEDBACK(human, HREC(car)->player) || TM2_CAR_FEEDBACK(other_human, oid)) {   /* H3 */
            soundPlay(37, 0, 0, 99);
        } else {
            if (!viewGetSplitMode())
                soundLocateForView0(m->pos);
            else
                soundLocateForView(m->pos, -1);
            x = PadChkMtap();
            soundPlay(37, x, soundGetListenerPan(), 99);
        }
    }
    if (kind != 240) {
        ostate->frozen = 1;
        ost->freeze_timer = 30;
        carSpinOut(car, human, 8, 0);
        ret = 0;
        ostate->crush_peer = 1;
    }
    state->crush_contact = 1;
#ifdef TM2_PORT_CAR_GROUPS
    st->crush_owner = TM2_CAR_KEY(oid2, okind);   /* request 32: the ridden car's key */
#else
    st->crush_owner = okind;
#endif
    if (ost->axle_rear < ost->axle_front)
        st->crush_range = ost->axle_front;
    else
        st->crush_range = ost->axle_rear;
    st->crush_range += st->wheelbase / 2;
    st->crush_peak = ost->ride_height + ost->roof_height + 24;
    if (st->crush_peak >= 105)
        st->crush_peak = 104;
    RSHAKE(w, 0)
    RSHAKE(w, 1)
    RSHAKE(w, 2)
    RSHAKE(w, 3)
    RSHAKE(ow, 0)
    RSHAKE(ow, 1)
    RSHAKE(ow, 2)
    RSHAKE(ow, 3)
#ifdef TM2_PORT_BALANCE
    portBalanceCrushStart(id, kind);   /* request 40 bit 3 (BALANCE.md 3.4): Hammerhead is owed +5 once on top */
#endif
    dir = carPairDirection(id, oid2);
    if (other_human) {
        type = humanCarObject(oid);
        carApplyDamage(type->id, carCrushDamageAmount(), dir, 0, id);
    } else {
        type = aiCarObject(oid);
        carApplyDamage(type->id, carCrushDamageAmount(), dir, 0, id);
    }
    return ret;
}

extern void mathMulTransVec(void *, s32 *, s32 *);
extern s32 worldSectionAt(s32 x, s32 y, s32 z);
extern void wheelSectionUpdate(struct TM2CarWheel *rec, s32 section, s32 lead, u8 shared, u8 moving);
extern void carSetViewMode(struct TM2HumanRecord *human, s32 mode);
extern u8 worldSectionGroundClass[];

#define CORNER(i, hx, ry)     v[0] = hx;     v[1] = st->ry;     v[2] = -st->ride_height;     mathMulTransVec(&m->rot_m, v, w->wheel[i].corner);     w->wheel[i].pos[0] = m->pos[0] + w->wheel[i].corner[0];     w->wheel[i].pos[1] = m->pos[1] + w->wheel[i].corner[1];     w->wheel[i].pos[2] = m->pos[2] + w->wheel[i].corner[2];

#define SHIFT(i)     w->wheel[i].pos[0] += w->move[0];     w->wheel[i].pos[1] += w->move[1];     w->wheel[i].pos[2] += w->move[2];     w->wheel[i].f1C = w->wheel[i].pos[2];

/* @800EF5D0: place the car's four corner contact points from its half
 * width/length and height (plus the shake offset), find the track section
 * under each corner and update the four contact slots; the human car
 * records its section (and runs the section-kind 1 event in modes 3/4),
 * the AI car records it at +0x46A. */
void __attribute__((section(".text_800EF5D0"))) carUpdateContacts(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    struct TM2CarMotion *m;
    struct TM2CarStatsBlock *st;
    struct TM2CarWheels *w;
    s32 v[3];
    s32 s0, s1, s2, s3;
    u8 moving, f;

    if (human) {
        m = &HREC(car)->motion;
        st = &HREC(car)->stats;
        w = &HREC(car)->wheels;
    } else {
        m = &AI(car)->motion;
        st = &AI(car)->stats;
        w = &AI(car)->wheels;
    }
    CORNER(0, -st->track_width / 2, axle_front)
    CORNER(1, st->track_width / 2, axle_front)
    CORNER(2, -st->track_width / 2, axle_rear)
    CORNER(3, st->track_width / 2, axle_rear)
    v[0] = m->vel0 / 32;
    v[1] = m->speed / 32;
    v[2] = m->vel2 / 32;
    mathMulTransVec(&m->body_rot_m, v, w->move);
    SHIFT(0)
    SHIFT(1)
    SHIFT(2)
    SHIFT(3)
    s0 = worldSectionAt(w->wheel[0].pos[0], w->wheel[0].pos[1], m->pos[2]);
    s1 = worldSectionAt(w->wheel[1].pos[0], w->wheel[1].pos[1], m->pos[2]);
    s2 = worldSectionAt(w->wheel[2].pos[0], w->wheel[2].pos[1], m->pos[2]);
    s3 = worldSectionAt(w->wheel[3].pos[0], w->wheel[3].pos[1], m->pos[2]);
    moving = m->speed != 0;
    if (s0 == w->wheel[1].kind || s0 == w->wheel[2].kind || s0 == w->wheel[3].kind)
        f = 1;
    else
        f = 0;
    wheelSectionUpdate(&w->wheel[0], s0, st->rollover, f, moving);
    if (s1 == w->wheel[0].kind || s1 == w->wheel[2].kind || s1 == w->wheel[3].kind)
        f = 1;
    else
        f = 0;
    wheelSectionUpdate(&w->wheel[1], s1, st->rollover, f, moving);
    if (s2 == w->wheel[0].kind || s2 == w->wheel[1].kind || s2 == w->wheel[3].kind)
        f = 1;
    else
        f = 0;
    wheelSectionUpdate(&w->wheel[2], s2, st->rollover, f, moving);
    if (s3 == w->wheel[0].kind || s3 == w->wheel[1].kind || s3 == w->wheel[2].kind)
        f = 1;
    else
        f = 0;
    wheelSectionUpdate(&w->wheel[3], s3, st->rollover, f, moving);
    if (human) {
        if (s0 == s1 && s0 == s2 && s0 == s3)
            HREC(car)->state.x2C.route = s0;
        if (worldSectionGroundClass[s0] == 1) {
            if (HREC(car)->view == 3 || HREC(car)->view == 4)
                carSetViewMode(car, 2);
        }
    } else {
        DRV(car)->route = s0;
    }
}

extern s32 sub_8013FC44(s32);
extern struct TM2FireEffect *fxFlame10(s32 *pos, u8 arg1);
extern s32 *mathJitterPos(void *, s32, s32, s32);
extern void fxSmokeA12(void *, u8 arg1);
extern void fxSmokeA24(void *, s32);
extern u32 GsGetWorkBase_0(void);
extern void fxExplosion24(void *, u8 flag);
extern void fxExplosion48(void *, u8 flag);
extern void fxExplosion96(void *, u8 flag);
extern void fxSmokeA48(void *, s32);
extern struct TM2Object *carObjectFromOwner(s32);
extern s32 mathBlendAngle(s32 angle, s32 target, s32 pct);

#define TURN(t) m->rot[1] = mathBlendAngle(m->rot[1], t, 98)

/* @800E75F4: per-frame wreck handling of a car: (AI) recompute the
 * distance to its own track node and the near flag at +52, start the smoke
 * emitter, spawn random sparks, count down the wreck timer at +38 while
 * spinning the car and emitting debris, and once the timer runs out either
 * blow the car up (sound, fire, smoke off) or settle its heading toward the
 * nearest quarter turn.
 * Register note: in the f11 spin branch the turn k1 and the new spin1 value w
 * must be variables also used in other blocks (k1 = the other branch's turn,
 * w = the wreck timer's new value).  Block-local temps there let local-alloc
 * tie k1 to the /1024 quotient (v0) instead of the base turn's a0. */
void __attribute__((section(".text_800E75F4"))) carWreckUpdate(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    struct TM2CarStatsBlock *a;
    struct TM2CarMotion *m;
    struct TM2CarState *f;
    struct TM2CarWeapons *q;
    struct TM2AIRecord *c;
    struct TM2CarMotion *node;
    s32 kind;
    u8 ok;
    s32 v[3];
    s32 d[3];
    u8 near[8];
    s32 *p;
    s32 dx, dy;
    s32 k1, k2;
    s16 w;
    s32 n, div;
    void *t;                /* target: human (near[0]) or AI record */
    struct TM2Object *o;

    if (human) {
        a = &HREC(car)->stats;
        m = &HREC(car)->motion;
        f = &HREC(car)->state;
        kind = HREC(car)->type;
        q = &HREC(car)->weapons;
        ok = 1;
    } else {
        kind = AI(car)->type;
        a = &AI(car)->stats;
        m = &AI(car)->motion;
        t = aiGetTargetCar(AI(car)->index, near);
        f = &AI(car)->state;
        q = &AI(car)->weapons;
        c = AI(car);
        if (near[0])
            node = &HREC(t)->motion;
        else
            node = &AI(t)->motion;
        dx = node->pos[0] - m->pos[0];
        dy = node->pos[1] - m->pos[1];
        c->target_dist = sub_8013FC44(dx * dx + dy * dy);
        if (c->target_dist < 4320)
            c->active = 1;
        else
            c->active = 0;
        ok = c->active;
    }
    if (!human) {
        if (!f->burning && !f->wheel_in_air && !f->in_water) {
            f->burning = 1;
            v[0] = m->pos[0];
            v[1] = m->pos[1];
            v[2] = m->pos[2];
            q->fire_effect = fxFlame10(m->pos, 1);
            q->fire_effect->active = 500;
        }
        a->burn_time = 0;
    }
    if (rand() < 2048 && f->burning) {
        p = mathJitterPos(m->pos, 819, 819, 819);
        p[2] += 120;
        if (rand() < 16384)
            fxSmokeA24(p, 0);
        else
            fxSmokeA12(p, 0);
    }
    w = a->wreck_timer - timerGetFrameTicks();
    a->wreck_timer = w;
    if (a->wreck_timer > 0) {
        if (f->f11) {
            if (m->rot[1] < -1023)
                goto zero;
            k2 = a->steer_rate_high;
            k1 = k2 - a->steer_rate_low * m->rot[1] / 1024;
            w = m->spin1 - k1;
            m->spin1 = w;
        } else {
            if (m->rot[1] >= 1024)
                goto zero;
            k1 = a->steer_rate_high + a->steer_rate_low * m->rot[1] / 1024;
            m->spin1 += k1;
        }
        m->speed /= 2;
        goto next;
    zero:
        m->spin1 = 0;
    next:
        if (f->f10) {
            if (m->speed < a->accel_launch)
                f->f10 = 0;
        } else {
            n = 3;
            if (!f->burning && !f->wheel_in_air && !f->in_water) {
                f->burning = 1;
                m->pos[2] -= 40;
                q->fire_effect = fxFlame10(m->pos, 0);
                m->pos[2] += 40;
                q->fire_effect->active = 500;
            }
            a->burn_time = 0;
            if (a->f16 < n) {
                div = a->f16 < n ? 4 : 12;
                if (GsGetWorkBase_0() % div == 0 && (rand() & 1)) {
                    (a->f16)++;
                    f->hud_notice = 1;
                    if (n == 7) {
                        d[0] = (rand() & 63) - 32;
                        d[1] = (rand() & 63) - 32;
                        d[2] = (rand() & 15) - 8;
                        v[0] = m->pos[0];
                        v[1] = m->pos[1];
                        v[2] = m->pos[2];
                        v[0] += d[0];
                        v[1] += d[1];
                        v[2] += 120 + d[2];
                        if (!(rand() & 1))
                            fxExplosion96(v, 0);
                        else
                            fxExplosion48(v, 0);
                    } else {
                        d[0] = (rand() & 127) - 64;
                        d[1] = (rand() & 127) - 64;
                        d[2] = (rand() & 32) - 16;
                        v[0] = m->pos[0];
                        v[1] = m->pos[1];
                        v[2] = m->pos[2];
                        v[0] += d[0];
                        v[1] += d[1];
                        v[2] += d[2];
                        v[0] = m->pos[0];
                        v[1] = m->pos[1];
                        v[2] = m->pos[2] + 120;
                        if (rand() & 1)
                            fxExplosion24(v, 0);
                        else
                            fxExplosion48(v, 0);
                    }
                    fxSmokeA48(v, 0);
                }
            }
        }
    } else if (!ok || a->wreck_timer < -1200) {
        if (!human) {
            AI(car)->active = 1;
#ifdef TM2_PORT_CAR_GROUPS
            o = carObjectFromOwner(TM2_CAR_KEY(AI(car)->slot, kind));   /* request 32 */
#else
            o = carObjectFromOwner(kind);
#endif
            carApplyDamage(o->id, 15, 0, 2, 0);
            v[0] = m->pos[0];
            v[1] = m->pos[1];
            v[2] = m->pos[2] + 120;
            fxExplosion48(v, 0);
            v[2] += a->ride_height;
            fxSmokeA48(v, 0);
            q->fire_effect->active = 0;
            f->burning = 0;
        }
    } else if (a->wreck_timer < -960) {
        if (!human && !f->f26) {
            v[0] = m->pos[0];
            v[1] = m->pos[1];
            v[2] = m->pos[2] + 24;
            fxExplosion24(v, 0);
            v[2] += a->ride_height;
            fxSmokeA48(v, 0);
            f->f26 = 1;
        }
    } else {
        m->spin0 = 0;
        m->spin1 = 0;
        m->yaw_rate = 0;
        m->vel0 = 0;
        m->speed -= m->speed / 16;
        m->vel2 = 0;
        if (m->rot[1] >= 1138) {
            if (m->rot[1] >= 1934)
                return;
            if (m->rot[1] < 1536)
                TURN(1024);
            else
                TURN(2036);
        } else if (m->rot[1] >= 114) {
            if (m->rot[1] >= 910)
                return;
            if (m->rot[1] < 512)
                TURN(0);
            else
                TURN(1024);
        } else if (m->rot[1] < -113) {
            if (m->rot[1] < -909)
                return;
            if (m->rot[1] >= -511)
                TURN(0);
            else
                TURN(1024);
        } else {
            if (m->rot[1] >= -1137)
                return;
            if (m->rot[1] < -1933)
                return;
            if (m->rot[1] >= -1535)
                TURN(-1536);
            else
                TURN(-2036);
        }
    }
}
