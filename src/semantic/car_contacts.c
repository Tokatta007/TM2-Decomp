/* Car contact resolution (800E2DD0), called per car from carsPhysicsUpdate.
 *
 * The contact block (struct TM2CarContact, human +0x234, AI +0xD8) holds
 * 36-byte contact records (struct TM2ContactRec): ground contacts at
 * +0x19C, car-to-object contacts at +0x274 and the combined record at
 * +0x34C. This function merges the frame's ground
 * contacts into the combined record (averaging, with angle wrap), plays
 * the scrape sounds by impact speed, then merges object contacts: a hard
 * side hit by a heavier car spins this car out (carSpinOut). It finally
 * keeps the "in contact" flag (+0x02) and the frames-without-contact
 * counter (+0x10).
 *
 * Retail reads the impact speed at car +0x6F4 (the human record's saved
 * velocity) for AI cars too. */
#include "semantic/cars.h"
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif

void carContactPassToOther(struct TM2ContactRec *from, struct TM2ContactRec *rec);

typedef struct {
    u8 pad[0xB4];
    s32 points;             /* +0xB4 contact points */
} TM2CarType;

extern TM2CarType *humanCarObject(s32 player);
extern TM2CarType *aiCarObject(s32 id);
extern void *memcpy(void *, const void *, int);
extern int viewGetSplitMode(void);
extern void soundLocateForView0(s32 *pos);
extern void soundLocateForView(s32 *pos, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void mathBoundAngle(s32 *);
extern void carBodySpinClear(struct TM2CarMotion *);

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

/* Play scrape sound `id` at the listener position with volume `vol`.
 * The block-local x/y keep these temporaries out of the global register
 * allocator (retail's s0/s1 choice depends on it). */
#define SCRAPE_SOUND(id, vol) { \
    s32 x = PadChkMtap(); \
    s32 y = soundGetListenerPan(); \
    soundPlay(id, x, y, vol); \
}

/* Merge test: when the combined record has f01 clear, it is passed to the
 * car it hit and replaced instead of merged. (Written as an inline to get
 * retail's flag register.) */
static __inline__ u8 can_merge(struct TM2CarContact *c, s32 i)
{
    if (!c->hit.f01) {
        carContactPassToOther(&c->hit, &c->ground[i]);
        return 0;
    }
    return 1;
}

/* @800E2DD0 */
void __attribute__((section(".text_800E2DD0"))) carContactsCombine(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    struct TM2CarContact *c;
    struct TM2CarStatsBlock *st;
    struct TM2CarMotion *m;
    struct TM2CarState *state;
    TM2CarType *type;
    void *other;            /* human (other_human) or AI record */
    struct TM2CarStatsBlock *ost;
    struct TM2CarMotion *om;
    u8 other_human;
    s32 a, i, speed, unused;

    if (human) {
        c = &HREC(car)->contact;
        st = &HREC(car)->stats;
        m = &HREC(car)->motion;
        state = &HREC(car)->state;
        type = humanCarObject(HREC(car)->player);
    } else {
        c = &AI(car)->contact;
        st = &AI(car)->stats;
        m = &AI(car)->motion;
        state = &AI(car)->state;
        type = aiCarObject(AI(car)->index);
    }

    c->hit.f00 = 0;
    for (i = 0; i < type->points; i++) {
        if (!c->probe[i].active || c->ground[i].f02 || !c->ground[i].f01)
            continue;
        if (!c->hit.f00 || c->hit.f02) {
            c->hit = c->ground[i];
            c->hit.f00 = 1;
            continue;
        }
        if (!can_merge(c, i)) {
            c->hit = c->ground[i];
            continue;
        }
        if (c->hit.side < c->ground[i].side)
            c->hit.side = c->ground[i].side;
        else
            c->hit.side = c->hit.side;
        c->hit.damage = (c->ground[i].damage + c->hit.damage) / 2;
        c->hit.speed = (c->ground[i].speed + c->hit.speed) / 2;
        a = c->hit.angle - c->ground[i].angle;
        if (__builtin_abs(a) > 2048)
            c->hit.angle = -a / 2;
        else
            c->hit.angle = (c->hit.angle + c->ground[i].angle) / 2;
        a = c->hit.turn - c->ground[i].turn;
        if (__builtin_abs(a) > 2048)
            c->hit.turn = -a / 2;
        else
            c->hit.turn = (c->hit.turn + c->ground[i].turn) / 2;
        if (c->hit.force < c->ground[i].force)
            c->hit.force = c->ground[i].force;
        c->hit.pos[0] = (c->hit.pos[0] + c->ground[i].pos[0]) / 2;
        c->hit.pos[1] = (c->hit.pos[1] + c->ground[i].pos[1]) / 2;
        c->hit.pos[2] = (c->hit.pos[2] + c->ground[i].pos[2]) / 2;
    }

    if (c->hit.f00) {
        /* Reads with no use (kept: retail performs them). */
        if (human) {
            unused = c->hit.f0A;
            unused = HREC(car)->motion.fC4;
        } else {
            unused = c->hit.f0A;
            unused = AI(car)->motion.fC4;
        }
        /* human record's motion.vel, for AI cars too (retail) */
        speed = __builtin_abs(HREC(car)->motion.vel[0]) + __builtin_abs(HREC(car)->motion.vel[1])
              + __builtin_abs(HREC(car)->motion.vel[2]);
        if (!viewGetSplitMode())
            soundLocateForView0(m->pos);
        else
            soundLocateForView(m->pos, -1);
        if (!state->dead) {
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
        }
    }

    if (!c->hit.f00 || c->hit.f02) {
        for (i = 0; i < type->points; i++) {
            if (!c->object[i].f00)
                break;
            if (!c->hit.f00 || c->hit.f02) {
                c->hit = c->object[i];
                c->push = c->hit.push;
                c->other = c->hit.other;
                other = carFromSlot(c->hit.other, &other_human);
                if (!other)
                    continue;
                if (other_human) {
                    ost = &HREC(other)->stats;
                    om = &HREC(other)->motion;
                } else {
                    ost = &AI(other)->stats;
                    om = &AI(other)->motion;
                }
                if (st->reverse_cap < m->speed && ost->mass > st->mass) {
                    a = m->rot[2] - om->rot[2];
                    mathBoundAngle(&a);
                    if (__builtin_abs(a) >= 1480) {
                        state->f20 = 1;
                        state->f01 = 1;
                        carBodySpinClear(m);
                        if (c->hit.turn > 0) {
                            c->hit.angle = m->body_rot[2] + 170;
                            c->hit.turn = 170;
                        } else {
                            c->hit.angle = m->body_rot[2] - 170;
                            c->hit.turn = -170;
                        }
                        c->hit.speed = st->reverse_cap;
                        c->hit.push = 0;
                        m->f02 = m->body_rot[2];
                        st->f20 = 0;
                        if (a > 0) {
                            m->spin1 = 1;
                            m->f04 = m->body_rot[2] + 1024;
                        } else {
                            m->spin1 = -1;
                            m->f04 = m->body_rot[2] - 1024;
                        }
                        carSpinOut(car, human, 10, 1);
                    }
                }
            } else if (c->hit.f01)
                carContactPassToOther(&c->object[i], &c->hit);
            else if (c->object[i].force > c->hit.force)
                c->hit = c->object[i];
        }
    }

    c->in_contact = c->hit.f00;
    if (c->in_contact)
        c->free_frames = 0;
    else
        ++c->free_frames;
}

#define HIT_SPEED() ((s16)timerGetFrameTicks() * 760 / 100 * 32)

extern s32 timerGetFrameTicks(void);

/* @800E35F4: pass a contact to the car it hit (rec->other): pick the side
 * of that car (0 back, 1 front, 2 and 3 the sides) from the angles, store
 * the record there with at least the minimum hit speed. */
void __attribute__((section(".text_800E35F4"))) carContactPassToOther(struct TM2ContactRec *from, struct TM2ContactRec *rec)
{
    u8 oh;
    s32 a, b, c;
    struct TM2CarContact *oc;
    struct TM2CarMotion *om;
    void *other;            /* human (oh) or AI record */
    s32 side;

    other = carFromSlot(from->other, &oh);
    if (!other)
        return;
    if (oh) {
        oc = &HREC(other)->contact;
        om = &HREC(other)->motion;
    } else {
        oc = &AI(other)->contact;
        om = &AI(other)->motion;
    }
    a = om->fC4 - rec->f0A;
    if (om->vel[1] < 0)
        a += 2048;
    mathBoundAngle(&a);
    b = 2048 - a * 2;
    mathBoundAngle(&b);
    c = om->rot[2] - b;
    mathBoundAngle(&c);
    if (c > 1024)
        side = 1;
    else if (c < -1024)
        side = 0;
    else if (c > 0)
        side = 2;
    else
        side = 3;
    oc->ground[side] = *rec;
    oc->ground[side].f01 = 1;
    oc->ground[side].side = side;
    oc->ground[side].speed = HIT_SPEED() < om->speed ? om->speed : HIT_SPEED();
    oc->ground[side].turn = b;
}

extern void *carObjectId(s32 id, u8 human);
extern void carApplyDamage(void *obj, s32 strength, s32 *pos, s32, s32);
extern void aiWallBumpCount(struct TM2AIRecord *ai);
extern void carRicochetOffObject(void *car, u8 human, u8 mode, s32 turn, u32 side);
extern void carContactSetTargetHeading(void *car, u8 human);
extern void *RotMatrixYXZ(s16 *, struct TM2CarMatrix *);
extern void mathMulTransVec(struct TM2CarMatrix *, s32 *, s32 *);
extern void fxSparkOrange(s32 *pos, u8 arg1);

#pragma tm2_rdata .tm2_800CEB10

/* @800E3800: respond to the combined contact: ricochet, push the car out
 * of the obstacle and spark above a speed threshold. */
void __attribute__((section(".text_800E3800"))) carContactRespond(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    s16 r[4];
    s32 v[3];
    s32 out[4];
    s32 q[3];
    struct TM2CarMatrix mat;
    struct TM2CarContact *c;
    struct TM2CarState *state;
    s32 id;
    struct TM2CarStatsBlock *st;
    struct TM2CarMotion *m;
    struct TM2CarWheels *w;
    s32 k;

    if (human) {
        c = &HREC(car)->contact;
        state = &HREC(car)->state;
        id = HREC(car)->player;
        st = &HREC(car)->stats;
        m = &HREC(car)->motion;
        w = &HREC(car)->wheels;
    } else {
        c = &AI(car)->contact;
        state = &AI(car)->state;
        id = AI(car)->index;
        st = &AI(car)->stats;
        m = &AI(car)->motion;
        w = &AI(car)->wheels;
    }
    if (c->hit.f00 && c->hit.f02)
        c->hit.f00 = 0;
    if (!c->hit.f00)
        return;
    if (c->hit.f01) {
        c->f05 = 0;
        if (c->hit.damage > 0) {
            if (c->hit.damage > 0) {
                if (!human)
                    goto bump;
                carApplyDamage(carObjectId(id, human), c->hit.damage, c->hit.pos, 0, -1);
            }
            if (human)
                goto ricochet;
        } else if (human)
            goto ricochet;
bump:
        aiWallBumpCount(AI(car));
        if (c->hit.turn >= 1707)
            c->hit.turn = 1706;
        else if (c->hit.turn < -1706)
            c->hit.turn = -1706;
ricochet:
        carRicochetOffObject(car, human, 0, c->hit.turn, c->hit.side);
        r[0] = 0;
    } else {
        c->f05 = 1;
        m->body_rot[2] = c->hit.angle;
        m->speed = c->hit.speed;
        carRicochetOffObject(car, human, 1, c->hit.turn, -1);
#ifdef TM2_PORT_BALANCE
        /* request 40 (BALANCE.md 3.4-3.8): a car-to-car ram (f01 clear: only carCarCollision's records): none taken by
         * a Twister whose tornado runs (bit 6) or a leaping Grasshopper (bit 7); x1.2 / x1.1 / x1.1 when Hammerhead /
         * Axel / Minion dealt it (bits 3-5). A copy: the repeat guard below keeps the raw value. */
        carApplyDamage(carObjectId(id, human), portBalanceRam(car, human, c->hit.other, c->hit.damage), c->hit.pos, 0,
                       c->hit.other);
#else
        carApplyDamage(carObjectId(id, human), c->hit.damage, c->hit.pos, 0, c->hit.other);
#endif
        carContactSetTargetHeading(car, human);
        c->f0A = 30 - c->hit.damage * 30 / 12;
        r[0] = 0;
    }
    r[1] = 0;
    r[2] = c->hit.f0A;
    RotMatrixYXZ(r, &mat);
    v[0] = 0;
    k = (m->vel[1] * 4) / st->speed_cap;
    k += 2;
    v[1] = -(c->hit.force + k);
    v[2] = 0;
    mathMulTransVec(&mat, v, out);
    m->pos[0] += out[0];
    m->pos[1] += out[1];
    m->pos[2] += out[2];
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    switch (c->hit.side) {
    case 0:
        if (w->wheel[0].clamped)
            v[0] = -k;
        else
            v[0] = k;
        q[0] = w->wheel[0].pos[0];
        q[1] = w->wheel[0].pos[1];
        q[2] = w->wheel[0].pos[2] + st->ride_height;
        break;
    case 1:
        if (w->wheel[0].clamped)
            v[0] = k;
        else
            v[0] = -k;
        q[0] = w->wheel[1].pos[0];
        q[1] = w->wheel[1].pos[1];
        q[2] = w->wheel[1].pos[2] + st->ride_height;
        break;
    case 2:
        if (state->reversing)
            v[1] = -(v[1] * 2);
        if (w->wheel[0].clamped)
            v[0] = k;
        else
            v[0] = -k;
        q[0] = w->wheel[3].pos[0];
        q[1] = w->wheel[3].pos[1];
        q[2] = w->wheel[3].pos[2] + st->ride_height;
        break;
    case 3:
        if (state->reversing)
            v[1] = -(v[1] * 2);
        if (w->wheel[0].clamped)
            v[0] = -k;
        else
            v[0] = k;
        q[0] = w->wheel[2].pos[0];
        q[1] = w->wheel[2].pos[1];
        q[2] = w->wheel[2].pos[2] + st->ride_height;
        break;
    case 4:
        v[1] = v[1] * 2;
        q[0] = (w->wheel[0].pos[0] + w->wheel[1].pos[0]) / 2;
        q[1] = (w->wheel[0].pos[1] + w->wheel[1].pos[1]) / 2;
        q[2] = (w->wheel[0].pos[2] + w->wheel[1].pos[2]) / 2 + st->ride_height;
        break;
    case 5:
    default:
        v[1] = -(k * 2);
        q[0] = 0;
        q[1] = 0;
        q[2] = m->pos[2] + st->roof_height;
        if (state->wheel_in_air) {
            w->wheel[0].fall = -640 / (s16)timerGetFrameTicks();
            w->wheel[1].fall = -640 / (s16)timerGetFrameTicks();
            w->wheel[2].fall = -640 / (s16)timerGetFrameTicks();
            w->wheel[3].fall = -640 / (s16)timerGetFrameTicks();
            m->speed = m->speed / 2;
        }
        break;
    }
    c->f0E = __builtin_abs(m->speed) * 99
                 / (__builtin_abs(m->speed) < st->speed_cap ? st->speed_cap : __builtin_abs(m->speed));
    mathMulTransVec(&m->rot_m, v, out);
    m->pos[0] += out[0];
    m->pos[1] += out[1];
    m->pos[2] += out[2];
    if (m->vel[1] > (s16)timerGetFrameTicks() * 380 / 100 * 32)
        fxSparkOrange(q, 0);
}

extern s32 *carPairDirection(s32 other, s32 id);
extern s32 mathBlendAngle(s32 angle, s32 target, s32 pct);
extern s32 worldGroundHeightAt(s32 *pos);
extern int ratan2(int, int);

/* @800E405C: while the contact timer runs, push the car along the contact
 * direction (a hard push spins it), keep it on the ground, and turn its
 * heading towards the contact direction; end the contact when the timer
 * or the speed runs out. */
void __attribute__((section(".text_800E405C"))) carContactPushUpdate(car, human)
void *car;              /* human record when human is set, else AI */
u8 human;
{
    s32 q[3];
    s32 p[3];
    s32 a;
    struct TM2CarMotion *m;
    struct TM2CarState *state;
    struct TM2CarStatsBlock *st;
    struct TM2CarContact *c;
    u16 id;
    struct TM2CarWheels *w;
    s32 f, lim;
    s32 *dir;
    u8 flag;

    if (human) {
        m = &HREC(car)->motion;
        state = &HREC(car)->state;
        st = &HREC(car)->stats;
        c = &HREC(car)->contact;
        id = HREC(car)->slot;
        w = &HREC(car)->wheels;
    } else {
        m = &AI(car)->motion;
        state = &AI(car)->state;
        c = &AI(car)->contact;
        st = &AI(car)->stats;
        id = AI(car)->slot;
        w = &AI(car)->wheels;
    }
    if (!c->timer)
        goto tail;
    {
        /* t * 7 spelled as a shift: a plain "* 7" gets reassociated onto
         * f12 by the compiler; retail multiplies the frame step by 7. */
        s16 t = timerGetFrameTicks();
        f = c->push * ((t << 3) - t) / 100;
    }
    if (!f) {
        if (c->push || (c->f05 && c->f0A < 10))
            f = 2;
    }
    if (f) {
        if (c->f05)
            dir = carPairDirection((s16)c->other, (s16)id);   /* retail narrows both */
        else
            dir = c->hit.pos;
        p[0] = f * dir[0] / 4096;
        p[1] = f * dir[1] / 4096;
        q[0] = p[0] + w->move[0];
        q[1] = p[1] + w->move[1];
        lim = (s16)timerGetFrameTicks() * 2660 / 100;
        if (lim < q[0]) {
            p[0] = lim - w->move[0];
            f = lim;
        } else if (q[0] < -lim) {
            p[0] = -lim - w->move[0];
            f = lim;
        }
        if (lim < q[1]) {
            f = lim;
            p[1] = f - w->move[1];
        } else if (q[1] < -lim) {
            f = lim;
            p[1] = -lim - w->move[1];
        }
        m->pos[0] += p[0];
        m->pos[1] += p[1];
        c->push -= f;
        if (f > (s16)timerGetFrameTicks() * 760 / 100) {
            state->f01 = 1;
            carBodySpinClear(m);
            if (!q[1])
                q[1] = 1;
            m->body_rot[2] = ratan2(q[0], q[1]);
        } else if (!state->f01)
            c->push = 0;
        q[0] = m->pos[0];
        q[1] = m->pos[1];
        q[2] = m->pos[2] + f / 2;
        f = worldGroundHeightAt(q) - m->pos[2];
        if (f > 0) {
            m->pos[2] += f;
            w->wheel[0].pos[2] += f;
            w->wheel[1].pos[2] += f;
            w->wheel[2].pos[2] += f;
            w->wheel[3].pos[2] += f;
        }
    }
    if (human && state->f06) {
        if (c->timer < 120)
            carPushClear(c);
        else
            m->yaw_rate = 0;
    }
    if (c->f04) {
        flag = 1;
        m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 80);
    } else if (c->f03) {
        a = m->body_rot[2] - m->rot[2];
        mathBoundAngle(&a);
        flag = 1;
        if (c->f05) {
            if (__builtin_abs(a) < 1650) {
                if (__builtin_abs(a) >= 1252)
                    m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 95);
                else
                    m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 90);
            } else
                m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 98);
        } else {
            if (__builtin_abs(a) < 341)
                m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 80);
            else if (__builtin_abs(a) < 568)
                m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 90);
            else if (__builtin_abs(a) < 910)
                m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 95);
            else
                m->rot[2] = mathBlendAngle(m->rot[2], c->f08, 98);
        }
    } else
        flag = 1;
    if (flag) {
        c->timer -= timerGetFrameTicks();
        if (c->timer > 0) {
            if (c->f05 || c->push)
                goto tail;
            if (__builtin_abs(m->speed) >= st->accel_launch
                && __builtin_abs(c->f08 - m->rot[2]) >= 56)
                goto tail;
        }
    } else if (c->timer < 121 || c->push >= 8)
        goto tail;
    carPushClear(c);
tail:
    if (c->f0A < 32001)
        c->f0A += timerGetFrameTicks();
}
