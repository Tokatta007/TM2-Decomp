/* Game code (800E..800F range). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

typedef struct {
    s32 x, z, y;
} TM2Pos12;

/* Track section tables (35 sections), one object at 801A9854. */
typedef struct {
    TM2Pos12 pos[35];      /* 801A9854 */
    s32 w[35];             /* 801A99F8 section length */
    s32 l[35];             /* 801A9A84 section width */
    s16 sn[35];            /* 801A9B10 */
    s16 cs[35];            /* 801A9B56 */
    u8 pad_348[0x11C6 - 0x348];
    u8 rot[105];           /* 801AAA1A section is rotated (sn/cs) */
    u8 shape[0x1784 - 0x122F]; /* 801AAA83 0 box, 1 hexagon, 2 triangle */
    u8 count;              /* 801AAFD8 */
} TM2Track;

extern TM2Track worldSections;

/* @800EFF34: find the track section containing (x, y) at height z: each
 * section is a (possibly rotated) box with optional diagonal/hexagonal
 * clipped shape; returns the first containing section whose floor is
 * below z + 80, else the last containing one (or count - 1).
 * Matches retail. l[i] is indexed directly (no pointer temporary), so the
 * address is formed as i*4 + l in its own block as in retail. Both /4096s
 * of the rotation are written out (+4095 when negative, then >> 12, which
 * is what GCC expands); the dx shift is a separate statement, which gives
 * dx priority over &l[i] in global allocation (retail: dx t0, &l[i] t1).
 * The hexagon's product shares t with the rotation's second sum (both v0);
 * a variable used only by mult/div prefers LO and spills.
 */
s32 __attribute__((section(".text_800EFF34"))) worldSectionAt(s32 x, s32 y, s32 z)
{
    s32 i = 0;
    s32 r = worldSections.count - 1;
    s16 *sn, *cs;
    s32 *l, *w;
    s32 n;
    s32 dx, dz, t, t2, hw, hl;
    u8 flag;
    s32 pad[2]; /* unused: reproduces retail's 16-byte frame */

    if (worldSections.count != 0) {
        l = worldSections.l;
        w = worldSections.w;
        z += 80;
        n = worldSections.count;
        do {
            dx = x - worldSections.pos[i].x;
            dz = y - worldSections.pos[i].z;
            if (worldSections.rot[i]) {
                sn = &worldSections.sn[i];
                cs = &worldSections.cs[i];
                t2 = dx * *cs - dz * *sn;
                if (t2 < 0)
                    t2 += 4095;
                t = dz * *cs + dx * *sn;
                dx = t2;
                dx >>= 12;
                if (t < 0)
                    t += 4095;
                dz = t >> 12;
            }
            flag = 0;
            if (dx >= 0 && dz >= 0 && w[i] >= dx) {
                if (l[i] >= dz) {
                    if (worldSections.shape[i] == 0) {
                        flag = 1;
                    } else if (worldSections.shape[i] == 2) {
                        if (dx * l[i] / w[i] >= dz)
                            flag = 1;
                    } else if (worldSections.shape[i] == 1) {
                        hw = w[i] / 4;
                        hl = l[i] / 2;
                        if (dx < hw) {
                            if (hl < dz) {
                                dz -= hl;
                                flag = dz * hw / hl < dx;
                            } else {
                                flag = (hl - dz) * hw / hl < dx;
                            }
                        } else if (dx < w[i] - hw) {
                            flag = 1;
                        } else {
                            dx -= w[i] * 3 / 4;
                            if (dz < hl) {
                                t = dz * hw;
                            } else {
                                dz -= hl;
                                t = (hl - dz) * hw;
                            }
                            flag = dx < t / hl;
                        }
                    }
                }
            }
            if (flag) {
                if (worldSections.pos[i].y < z) {
                    r = i;
                    break;
                }
                r = i;
            }
            i++;
        } while (i < n);
    }
    return r;
}

extern s32 gameGetLevel(void);
extern u8 parisSinkZoneContains(s32 x, s32 y);
extern void wheelSinkStep(struct TM2CarWheel *wheel);
extern void wheelSinkRecover(struct TM2CarWheel *wheel);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 GsGetWorkBase_0(void);
extern u8 soundRepeatThrottle(s32 time);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);

/* @800F0450: pickup-light feedback (mode 3 only): in sections 17/18 the
 * four lamps light (wheelSinkStep) when the car or the lamp is inside one
 * of the hot squares, otherwise fade (wheelSinkRecover); while any lamp is lit,
 * pad rumble is (re)started and command 0x23 is sent. */
void __attribute__((section(".text_800F0450"))) carParisSinkZonesUpdate(void *car, u8 human)
{
    struct TM2CarWheels *m;
    struct TM2CarMotion *s;
    s32 k;

    if (gameGetLevel() != 3)
        return;
    if (human) {
        m = &HREC(car)->wheels;
        s = &HREC(car)->motion;
    } else {
        m = &AI(car)->wheels;
        s = &AI(car)->motion;
    }
    k = m->wheel[0].kind;
    if ((u32)(k - 17) < 2) {
        if (parisSinkZoneContains(s->pos[0] - worldSections.pos[k].x, s->pos[1] - worldSections.pos[k].z)) {
            wheelSinkStep(&m->wheel[0]);
            wheelSinkStep(&m->wheel[1]);
            wheelSinkStep(&m->wheel[2]);
            wheelSinkStep(&m->wheel[3]);
        } else {
            if (parisSinkZoneContains(m->wheel[0].pos[0] - worldSections.pos[k].x, m->wheel[0].pos[1] - worldSections.pos[k].z))
                wheelSinkStep(&m->wheel[0]);
            else if (m->wheel[0].f09)
                wheelSinkRecover(&m->wheel[0]);
            if (parisSinkZoneContains(m->wheel[1].pos[0] - worldSections.pos[k].x, m->wheel[1].pos[1] - worldSections.pos[k].z))
                wheelSinkStep(&m->wheel[1]);
            else if (m->wheel[1].f09)
                wheelSinkRecover(&m->wheel[1]);
            if (parisSinkZoneContains(m->wheel[2].pos[0] - worldSections.pos[k].x, m->wheel[2].pos[1] - worldSections.pos[k].z))
                wheelSinkStep(&m->wheel[2]);
            else if (m->wheel[2].f09)
                wheelSinkRecover(&m->wheel[2]);
            if (parisSinkZoneContains(m->wheel[3].pos[0] - worldSections.pos[k].x, m->wheel[3].pos[1] - worldSections.pos[k].z))
                wheelSinkStep(&m->wheel[3]);
            else if (m->wheel[3].f09)
                wheelSinkRecover(&m->wheel[3]);
        }
    } else {
        if (m->wheel[0].f09)
            wheelSinkRecover(&m->wheel[0]);
        if (m->wheel[1].f09)
            wheelSinkRecover(&m->wheel[1]);
        if (m->wheel[2].f09)
            wheelSinkRecover(&m->wheel[2]);
        if (m->wheel[3].f09)
            wheelSinkRecover(&m->wheel[3]);
    }
    if (m->wheel[0].f09 || m->wheel[1].f09 || m->wheel[2].f09 || m->wheel[3].f09) {
        if (!viewGetSplitMode())
            soundLocateForView0(s->pos);
        else
            soundLocateForView(s->pos, -1);
        if (soundRepeatThrottle(GsGetWorkBase_0()) && human) {
#ifdef TM2_PORT_HUMANS
            /* H3: a remote human's cue positional; soundRepeatThrottle (rand) is called exactly as retail */
            if (TM2_HUMAN_FEEDBACK(HREC(car)->player)) {
                soundPlay(0x23, 0, 0, 0x63);
            } else {
                s32 a = PadChkMtap();
                soundPlay(0x23, a, soundGetListenerPan(), 0x63);
            }
#else
            soundPlay(0x23, 0, 0, 0x63);
#endif
        } else if (soundRepeatThrottle(GsGetWorkBase_0())) {
            s32 a = PadChkMtap();
            soundPlay(0x23, a, soundGetListenerPan(), 0x63);
        }
    }
}

extern s32 timerGetFrameTicks(void);


/* Ease the sway offset back to zero by the frame step. */
#define DECAY34()                                 if (w->sway > 0) {                                w->sway -= timerGetFrameTicks();                    if (w->sway < 0)                                  w->sway = 0;                          } else if (w->sway < 0) {                         w->sway += timerGetFrameTicks();                    if (w->sway > 0)                                  w->sway = 0;                          }

/* One sway step: move the offset towards the current turn point; at the turn the
 * amplitude halves and the direction flips, or the sway stops. */
static __inline__ void sway(struct TM2CarWheel *w, s16 step)
{
    if (w->sway_up) {
        w->sway += step;
        if (w->sway > w->shake) {
            w->shake /= 2;
            if (w->shake >= 3) {
                w->sway_up = 0;
            } else {
                w->shake = 0;
                w->sway = 0;
            }
        }
    } else {
        w->sway -= step;
        if (w->sway < w->sway_low) {
            w->sway_low /= 2;
            if (w->sway_low < -2) {
                w->sway_up = 1;
            } else {
                w->shake = 0;
                w->sway = 0;
            }
        }
    }
}

/* @800F1BEC: wheel suspension sway: clamps the amplitude (shake/sway_low) to
 * the remaining time/8 (or to `amp`, ramped over 60 frames while armed),
 * otherwise oscillates the offset (sway) between shake and sway_low, halving
 * the amplitude at each turn until it dies out, then eases sway back to 0. */
void __attribute__((section(".text_800F1BEC"))) wheelSwayUpdate(struct TM2CarWheel *w, s16 amp)
{
    s32 d, step;

    d = (w->pos[2] - w->f1C) / (s16)timerGetFrameTicks();
    d /= 8;
    if (w->shake < d) {
        if ((s16)amp < d)
            d = (s16)amp;
        w->shake = d;
        w->sway_low = -w->shake;
        w->sway_up = 0;
        return;
    }
    if (w->touch) {
        if (w->f12 > 60)
            w->shake = amp;
        else
            w->shake = (s16)amp * w->f12 / 60;
        w->sway_low = -w->shake;
        w->sway_up = 0;
        DECAY34();
    } else if (*(u16 *)&w->shake != 0) {
        step = __builtin_abs(w->shake) / 2;
        step = step < 2 ? 2 : step;
        sway(w, step);
    } else if (*(u16 *)&w->sway != 0) {
        DECAY34();
    }
}

extern struct TM2Object *carObjectFromOwner(s32);
extern u8 carFindByObject(struct TM2Object *object, u8 *human, s32 *index);
extern s32 slamArmGetAngle(void);
extern s32 rsin(s32);
extern void mathMulTransVec(void *, s32 *, s32 *);
extern s32 mathBlendValue(s32 value, s32 target, s32 pct);
extern void mathBoundAngle(s32 *);
extern s32 mathBlendAngle(s32 angle, s32 target, s32 pct);

#define S32(p, o) (*(s32 *)((p) + (o)))
#define S16(p, o) (*(s16 *)((p) + (o)))

/* @800F59A4: pull the car bound to object `id` (the slam target) beside the
 * carrying car whose stats and motion blocks are given (offset along its
 * matrix by stats->axle_front + 120, swayed by the sine of the arm angle when
 * `on`), ease its position (pct 40 when on, else 70) and heading (carrier
 * +/- 1024, pct 80), and when `on` set its wheel heights.
 * The timer phase reuses the dead parameter `id` (retail register choice). */
void __attribute__((section(".text_800F59A4"))) weaponSlamCarryTarget(s32 id, struct TM2CarStatsBlock *stats, struct TM2CarMotion *motion, u8 on, u8 left)
{
    s32 vec[3];
    s32 out[3];
    u8 human;
    s32 index;
    s32 diff, target;
    struct TM2CarMotion *tmotion;
    struct TM2CarWheels *twheels;
    s32 h;

    if (!carFindByObject(carObjectFromOwner(id), &human, &index))
        return;
    vec[0] = 0;
    vec[1] = stats->axle_front + 120;
    if (on) {
        id = 910 - slamArmGetAngle();
        if (left)
            vec[1] -= 120 * rsin(id) / 4096;
        else
            vec[1] += 120 * rsin(id) / 4096;
    }
    vec[2] = 0;
    mathMulTransVec(&motion->rot_m, vec, out);
    if (human) {
        tmotion = &humanCarGet(index)->motion;
        twheels = &humanCarGet(index)->wheels;
    } else {
        tmotion = &aiCarGet(index)->motion;
        twheels = &aiCarGet(index)->wheels;
    }
    if (on) {
        if (left) {
            tmotion->pos[0] = mathBlendValue(tmotion->pos[0], motion->pos[0] + out[0], 40);
            tmotion->pos[1] = mathBlendValue(tmotion->pos[1], motion->pos[1] + out[1], 40);
        } else {
            tmotion->pos[0] = mathBlendValue(tmotion->pos[0], motion->pos[0] + out[0], 40);
            tmotion->pos[1] = mathBlendValue(tmotion->pos[1], motion->pos[1] + out[1], 40);
        }
    } else {
        tmotion->pos[0] = mathBlendValue(tmotion->pos[0], motion->pos[0] + out[0], 70);
        tmotion->pos[1] = mathBlendValue(tmotion->pos[1], motion->pos[1] + out[1], 70);
    }
    diff = tmotion->rot[2] - motion->rot[2];
    mathBoundAngle(&diff);
    if (diff < 0)
        target = motion->rot[2] - 1024;
    else
        target = motion->rot[2] + 1024;
    mathBoundAngle(&target);
    tmotion->rot[2] = mathBlendAngle(tmotion->rot[2], target, 80);
    if (on) {
        id = 910 - slamArmGetAngle();
        h = 120 * rsin(id) / 4096;
        if (diff < 0) {
            twheels->wheel[0].f1A = h;
            twheels->wheel[2].f1A = h;
            h = h + stats->track_width / 2 * rsin(id) / 4096;
            twheels->wheel[1].f1A = h;
            twheels->wheel[3].f1A = h;
        } else {
            twheels->wheel[1].f1A = h;
            twheels->wheel[3].f1A = h;
            h = h + stats->track_width / 2 * rsin(id) / 4096;
            twheels->wheel[0].f1A = h;
            twheels->wheel[2].f1A = h;
        }
    }
}

extern void carBodySpinClear(struct TM2CarMotion *motion);

/* @800F5D20: put the car bound to object `id` into a spin-out: reset its
 * flags and lamps, set spin speed and heading from `angle` (reversed and
 * normalised when `back`), and for a forward spin pick the turn direction
 * from the heading difference. */
void __attribute__((section(".text_800F5D20"))) carSpinOutById(s32 id, s16 angle, u8 back)
{
    struct TM2Object *obj;
    void *car, *p;          /* human or AI record */
    u8 human;
    s32 index;
    s32 diff;
    struct TM2CarState *f;
    struct TM2CarWheels *m;
    struct TM2CarMotion *c;
    struct TM2CarContact *g;
    struct TM2CarStatsBlock *h;

    obj = carObjectFromOwner(id);
    if (obj == 0)
        return;
    if (!carFindByObject(obj, &human, &index))
        return;
    if (human) {
        p = humanCarGet(index);
        f = &HREC(p)->state;
        m = &HREC(p)->wheels;
        c = &HREC(p)->motion;
        g = &HREC(p)->contact;
        h = &HREC(p)->stats;
        car = p;
    } else {
        p = aiCarGet(index);
        f = &AI(p)->state;
        m = &AI(p)->wheels;
        c = &AI(p)->motion;
        g = &AI(p)->contact;
        h = &AI(p)->stats;
        car = p;
    }
    f->f1D = 0;
    f->crush_peer = 0;
    if (back) {
        carSpinOut(car, human, 2, 0);
        c->speed = 1330 * (s16)timerGetFrameTicks() / 100 * 32 / 3;
        c->body_rot[2] = angle + 2048;
        mathBoundAngle(&c->body_rot[2]);
        g->f0A = 0;
        g->timer = 180;
        f->f19 = 0;
        f->f01 = 1;
        carBodySpinClear(c);
        carShakeStart(car, 5, 2, 2, human);
    } else {
        carSpinOut(car, human, 1, 0);
        c->speed = 1330 * (s16)timerGetFrameTicks() / 100 * 32;
        c->body_rot[2] = angle;
        g->f0A = 0;
        g->timer = 180;
        f->f19 = 0;
        f->f01 = 1;
        carBodySpinClear(c);
        carShakeStart(car, 5, 2, 2, human);
        f->f12 = 1;
        c->f02 = c->body_rot[2];
        diff = c->body_rot[2] - c->rot[2];
        mathBoundAngle(&diff);
        if (diff < 0) {
            c->spin1 = 1;
            c->f04 = c->body_rot[2] + 1024;
        } else {
            c->spin1 = -1;
            c->f04 = c->body_rot[2] - 1024;
        }
        h->f20 = 0;
    }
    m->wheel[0].f1A = 0;
    m->wheel[1].f1A = 0;
    m->wheel[2].f1A = 0;
    m->wheel[3].f1A = 0;
}
