/* Car wheel contacts (800EC994): per-frame wheel step for one car record,
 * (car, human) like the motion helpers (car_motion.c). Blocks by human /
 * AI: motion +0x6DC / +0x71C, stats +0x14C / +0x4FC, state +0x9C / +0x6C,
 * wheels +0x5A4 / +0x5E4, contact +0x234 / +0xD8, controls +0xD0 / +0x44.
 *
 *   1. probe each of the four wheels (carWheelProbe) and update it
 *      (carWheelUpdate);
 *   2. state f0B = any wheel touching, f0C = all four touching (then f1B
 *      and the largest front height difference in the wheels header f02);
 *      any wheel touching calls carBodySpinClear on the motion block;
 *   3. pick the ground surface under the car: the first wheel that is on
 *      a surface and off the ground (or more than 10 above its contact),
 *      later wheels only replace a lower one. Wheel 0 compares wheel 1's
 *      height (retail quirk, kept). Landing / taking off sets the contact
 *      sound (14 / 39, take-off also plays carShakeStart);
 *   4. wheels on a moving surface (f08, e.g. a lift): carry the car with
 *      the surface (trackVehicleGetMove offset, trackVehicleGetTurn turn) and nudge it
 *      towards the surface centre (worldTrackVehiclePos), 1 per frame (2 with
 *      controls f13), outward when not all four wheels are on it;
 *   5. humans in the air turning hard lose speed in proportion;
 *   6. carGroundEffects. */
#include "semantic/cars.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#include "semantic/port_balance.h"   /* request 101 (BALANCE.md 22) */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

typedef struct {
    u8 on;                  /* +0x00 over a surface */
    u8 touch;               /* +0x01 on the ground */
    u8 pad_02;
    u8 f03;                 /* +0x03 */
    u8 pad_04;
    u8 f05;                 /* +0x05 */
    u8 pad_06[2];
    u8 moving;              /* +0x08 surface moves */
    u8 pad_09;
    s16 surface;            /* +0x0A */
    s16 next_surface;       /* +0x0C found by the probe */
    s16 object;             /* +0x0E moving surface id */
    s16 kind;               /* +0x10 ground kind (class in worldSectionGroundClass) */
    s16 f12;                /* +0x12 */
    s16 fall;               /* +0x14 */
    s16 level;              /* +0x16 surface height / sink offset */
    u8 pad_18[4];
    s16 f1C;                /* +0x1C */
    u8 pad_1E[6];
    s16 h;                  /* +0x24 */
    s16 f26;                /* +0x26 */
    s16 depth;              /* +0x28 */
    s16 land_z;             /* +0x2A */
    s16 f2C;                /* +0x2C bounce */
    u8 pad_2E[0x0E];
    s32 pos[3];             /* +0x3C */
} TM2Wheel;

typedef struct {
    s16 surface;            /* +0x00 */
    s16 tilt;               /* +0x02 */
    s32 level;              /* +0x04 */
    s32 step;               /* +0x08 highest climbable step */
    s32 move[3];            /* +0x0C car movement this frame */
    TM2Wheel w[4];          /* +0x18 */
} TM2Wheels;

typedef struct {
    s16 f00;                /* +0x00 */
    u8 pad_02[8];
    s16 speed;              /* +0x0A */
    u8 pad_0C[8];
    s16 f14;                /* +0x14 */
    u8 pad_16[0x0A];
    s32 pos[3];             /* +0x20 */
    u8 pad_2C[4];
    s32 turn;               /* +0x30 */
    s32 rot;                /* +0x34 */
} TM2WheelMotion;

typedef struct {
    u8 pad_00[0x0A];
    u8 on_surface;          /* +0x0A */
    u8 any_touch;           /* +0x0B */
    u8 all_touch;           /* +0x0C */
    u8 f0D;                 /* +0x0D */
    u8 f0E;                 /* +0x0E */
    u8 pad_0F[0x0B];
    u8 f1A;                 /* +0x1A */
    u8 f1B;                 /* +0x1B */
    u8 pad_1C[3];
    u8 f1F;                 /* +0x1F */
    u8 pad_20;
    u8 in_water;            /* +0x21 */
    u8 in_lava;             /* +0x22 */
    u8 in_fire;             /* +0x23 level 5 */
    u8 sinking;             /* +0x24 */
    u8 riding;              /* +0x25 */
} TM2WheelState;

typedef struct {
    u8 pad_00[6];
    u8 burn;                /* +0x06 lava clock */
    u8 pad_07[0x1B];
    s16 f22;                /* +0x22 */
    u8 pad_24[0x3C];
    s16 f60;                /* +0x60 */
    u8 pad_62[0x42];
    s32 hp;                 /* +0xA4 */
    u8 pad_A8[0x10];
    s32 fB8;                /* +0xB8 */
    s32 fBC;                /* +0xBC */
} TM2WheelStats;

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Matrix;

/* Splash / sink effect object (fxGetFreeWakeObject). */
typedef struct {
    u8 pad_00[8];
    s32 on;                 /* +0x08 */
    u8 pad_0C[0x74];
    TM2Matrix m;            /* +0x80 */
    s16 rot[3];             /* +0xA0 */
    u8 pad_A6[2];
    s32 pos[3];             /* +0xA8 */
} TM2WheelFx;

/* Ground surface (worldSurfaces, 28 bytes): type 2 surfaces are round (height
 * by distance from the centre, worldRoundSurfaceHeightAt), others are planes in their
 * own axes (worldSurfaceHeightAt). */
typedef struct {
    u8 type;                /* +0x00 0 rectangle, 1 triangle, 2 round */
    u8 f01;                 /* +0x01 */
    u8 pad_02[2];
    s16 w, l;               /* +0x04 size in its own axes */
    s16 sn;                 /* +0x08 */
    s16 cs;                 /* +0x0A */
    s16 h[4];               /* +0x0C corner heights */
    s32 x, y;               /* +0x14 origin */
} TM2Surface;

extern TM2Surface worldSurfaces[];

extern u8 worldSectionGroundClass[];             /* ground kind -> class */

u8 carWheelProbe(TM2Wheels *, TM2Wheel *);
void carWheelUpdate(s32 speed, s32 full, TM2Wheel *, s32 found);
extern void carBodySpinClear(void *motion);
extern s32 *trackVehicleGetMove(s32 object);
extern s32 trackVehicleGetTurn(s32 object);
#include "semantic/world.h"
void carGroundEffects();

/* @800EC994 */
void __attribute__((section(".text_800EC994"))) carWheelsUpdate(car, human)
    void *car;          /* human record when human is set, else AI */
    u8 human;
{
    TM2WheelMotion *m;
    TM2WheelStats *st;
    TM2WheelState *s;
    TM2Wheels *c;
    struct TM2CarContact *ct;
    struct TM2CarInput *ctl;
    s32 step;
    s32 *p;
    u8 any, all, found;

    if (human) {
        m = (TM2WheelMotion *)&HREC(car)->motion;
        st = (TM2WheelStats *)&HREC(car)->stats;
        s = (TM2WheelState *)&HREC(car)->state;
        c = (TM2Wheels *)&HREC(car)->wheels;
        ct = &HREC(car)->contact;
        ctl = &HREC(car)->input;
    } else {
        m = (TM2WheelMotion *)&AI(car)->motion;
        st = (TM2WheelStats *)&AI(car)->stats;
        s = (TM2WheelState *)&AI(car)->state;
        c = (TM2Wheels *)&AI(car)->wheels;
        ct = &AI(car)->contact;
        ctl = &AI(car)->input;
    }
    carWheelUpdate(m->speed, st->fB8, &c->w[0], carWheelProbe(c, &c->w[0]));
    carWheelUpdate(m->speed, st->fB8, &c->w[1], carWheelProbe(c, &c->w[1]));
    carWheelUpdate(m->speed, st->fB8, &c->w[2], carWheelProbe(c, &c->w[2]));
    carWheelUpdate(m->speed, st->fB8, &c->w[3], carWheelProbe(c, &c->w[3]));
    s->any_touch = c->w[0].touch || c->w[1].touch || c->w[2].touch || c->w[3].touch;
    s->all_touch = c->w[0].touch && c->w[1].touch && c->w[2].touch && c->w[3].touch;
    if (s->all_touch) {
        s->f1B = 1;
        {
            s32 t;
            t = c->w[0].pos[2] - c->w[1].pos[2];
            t = __builtin_abs(t);
            if (c->tilt < t)
                c->tilt = t;
        }
    }
    if (s->any_touch)
        carBodySpinClear(m);
    c->surface = -1;

    found = 0;
    if (c->w[0].on) {
        found = 1;
        if (!c->w[0].touch || c->w[1].h + 10 < c->w[0].pos[2]) {
            c->surface = c->w[0].surface;
            c->level = c->w[0].level;
        }
    }
    if (c->w[1].on) {
        found = 1;
        if (!c->w[1].touch || c->w[1].h + 10 < c->w[1].pos[2]) {
            if (c->surface < 0 || c->w[1].level > c->level) {
                c->surface = c->w[1].surface;
                c->level = c->w[1].level;
            }
        }
    }
    if (c->w[2].on) {
        found = 1;
        if (!c->w[2].touch || c->w[2].h + 10 < c->w[2].pos[2]) {
            if (c->surface < 0 || c->w[2].level > c->level) {
                c->surface = c->w[2].surface;
                c->level = c->w[2].level;
            }
        }
    }
    if (c->w[3].on) {
        found = 1;
        if (!c->w[3].touch || c->w[3].h + 10 < c->w[3].pos[2]) {
            if (c->surface < 0 || c->w[3].level > c->level) {
                c->surface = c->w[3].surface;
                c->level = c->w[3].level;
            }
        }
    }
    if (found) {
        if (!s->on_surface)
            ct->f0E = 14;
    } else if (s->on_surface) {
        carShakeStart(car, 3, 2, 1, human);
        ct->f0E = 39;
    }
    s->on_surface = found;

    any = c->w[0].moving || c->w[1].moving || c->w[2].moving || c->w[3].moving;
    all = c->w[0].moving && c->w[1].moving && c->w[2].moving && c->w[3].moving;
    if (any) {
        if (!s->any_touch && !s->riding) {
            m->speed = 0;
            s->riding = 1;
        } else if (s->riding) {
            p = trackVehicleGetMove(c->w[0].object);
            m->pos[0] += p[0];
            m->pos[1] += p[1];
            m->pos[2] += p[2];
            m->rot += trackVehicleGetTurn(c->w[0].object);
            if (ctl->brake)
                step = all ? 2 : -2;
            else
                step = all ? 1 : -1;
            p = worldTrackVehiclePos(c->w[0].object);
            if (p[0] - m->pos[0] > 0)
                m->pos[0] += step;
            else
                m->pos[0] -= step;
            if (p[1] - m->pos[1] > 0)
                m->pos[1] += step;
            else
                m->pos[1] -= step;
        }
    } else
        s->riding = 0;
    if (human && __builtin_abs(m->turn) >= 57 && !s->any_touch)
        m->f14 -= st->f60 * m->turn / 341 * m->speed / st->fB8;
    carGroundEffects(car, human);
}

extern struct TM2Object *humanCarObject(s32 index);
extern struct TM2Object *aiCarObject(s32 index);
extern int gameGetLevel(void);
extern void fxSplashAt(s32 *pos, s32 speed);
extern void fxLavaSplashAt(s32 *pos);
extern TM2WheelFx *fxGetFreeWakeObject(void);
extern s32 lightGetEnv(s32);
extern void carWheelsSpawnFx(TM2Wheels *, s32);
extern int rand(void);
extern void *RotMatrixYXZ(s16 *, TM2Matrix *);
extern void carBodyObjectShow(s32 type);
#include "semantic/car_groups.h"
/* request 32 (CAR_GROUPS.md): the car's key */
#ifdef TM2_PORT_CAR_GROUPS
#define WHEELS_KEY TM2_CAR_KEY(human ? HREC(car)->slot : AI(car)->slot, type)
#else
#define WHEELS_KEY type
#endif

/* @800ED128: ground effects when all four wheels are on the same kind of
 * ground: lava (class 2) burns hp every 61 ticks, water (3) and deep
 * water (4) splash; deep water sinks the car (wheel levels drop, speed and
 * hp drain). On level 5, kind 7 under a front or back wheel lights the car
 * (lightGetEnv 2). */
void __attribute__((section(".text_800ED128"))) carGroundEffects(car, human)
    void *car;          /* human record when human is set, else AI */
    u8 human;
{
    TM2WheelMotion *m;
    TM2WheelStats *st;
    TM2WheelState *s;
    TM2Wheels *c;
    TM2WheelFx *fx;
    struct TM2CarWeapons *blk;
    struct TM2Object *obj;
    s32 type;
    u8 burns, wet;
    s32 kind;
    struct TM2FireEffect *p;

    if (human) {
        type = HREC(car)->type;
        m = (TM2WheelMotion *)&HREC(car)->motion;
        st = (TM2WheelStats *)&HREC(car)->stats;
        burns = HREC(car)->stats.damageable;
        s = (TM2WheelState *)&HREC(car)->state;
        c = (TM2Wheels *)&HREC(car)->wheels;
        blk = &HREC(car)->weapons;
        obj = humanCarObject(HREC(car)->player);
        fx = (TM2WheelFx *)HREC(car)->fx;
    } else {
        type = AI(car)->type;
        m = (TM2WheelMotion *)&AI(car)->motion;
        st = (TM2WheelStats *)&AI(car)->stats;
        s = (TM2WheelState *)&AI(car)->state;
        blk = &AI(car)->weapons;
        obj = aiCarObject(AI(car)->index);
        c = (TM2Wheels *)&AI(car)->wheels;
        if (type == 0xE0)
            burns = 0;
        else
            burns = AI(car)->f03A;
        fx = (TM2WheelFx *)AI(car)->state.x2C.fx;
    }
    if (gameGetLevel() == 5) {
        if (c->w[0].kind == 7 || c->w[3].kind == 7) {
            if (!s->in_fire) {
                s->in_fire = 1;
                fxSplashAt(m->pos, m->speed);
            }
            obj->light = lightGetEnv(2);
            st->f22 = 100;
        } else if (s->in_fire) {
            s->in_fire = 0;
            obj->light = lightGetEnv(0);
        }
    }
    if (!s->on_surface && c->w[0].kind == c->w[1].kind && c->w[0].kind == c->w[2].kind
        && c->w[0].kind == c->w[3].kind) {
        kind = worldSectionGroundClass[c->w[0].kind];
        if (kind == 4) {
            if (worldSectionGroundClass[c->w[1].kind] != kind)
                kind = worldSectionGroundClass[c->w[1].kind];
            else if (worldSectionGroundClass[c->w[2].kind] != kind)
                kind = worldSectionGroundClass[c->w[2].kind];
            else if (worldSectionGroundClass[c->w[3].kind] != kind)
                kind = worldSectionGroundClass[c->w[3].kind];
        }
        if (kind != 3 && kind != 4) {
            s->in_water = 0;
            if (fx) {
                fx->on = 0;
                if (human)
                    HREC(car)->fx = 0;
                else
                    AI(car)->state.x2C.fx = 0;
                fx = 0;
            }
        }
        if (kind != 4)
            s->sinking = 0;
        if (kind != 2)
            s->in_lava = 0;
        if (kind == 2) {
            if (c->w[0].depth >= 8) {
                if (!s->in_lava) {
                    if (!s->all_touch) {
                        fxLavaSplashAt(m->pos);
                        st->burn = 0;
                    }
                    s->in_lava = 1;
                    st->f22 = 0;
                } else if (!s->f0E) {
                    if (__builtin_abs(m->speed) < st->fBC)
                        carWheelsSpawnFx(c, 1);
                    else
                        carWheelsSpawnFx(c, 2);
                }
                st->burn += timerGetFrameTicks();
                if ((signed char)st->burn > 60) {
                    st->burn = 0;
#ifdef TM2_PORT_HAZARDS
                    if (burns && !s->f0E && !portArenaOff(TM2_ARENA_LAVA))   /* request 38 G2: no HP lost */
#else
                    if (burns && !s->f0E)
#endif
                        st->hp -= 3;
                }
            } else
                st->burn = 0;
        } else {
            if (kind == 3 || kind == 4) {
                if (!s->in_water) {
                    if (!s->all_touch) {
                        fxSplashAt(m->pos, m->speed);
                        s->in_water = 1;
                        st->burn = 0;
                        fx = fxGetFreeWakeObject();
                        if (human) {
                            if (fx)
                                HREC(car)->fx = (s32 *)fx;
                        } else
                            AI(car)->state.x2C.fx = (s32 *)fx;
                    }
                } else if (fx) {
                    fx->pos[0] = m->pos[0];
                    fx->pos[1] = m->pos[1];
                    fx->pos[2] = m->pos[2] - 16;
                    fx->rot[0] = 0;
                    fx->rot[1] = 0;
                    fx->rot[2] = m->rot;
                    RotMatrixYXZ(fx->rot, &fx->m);
                    fx->on = 1;
                    if (st->hp <= 0) {
                        if (human) {
                            fx->on = 0;
                            HREC(car)->fx = 0;
                        } else {
                            fx->on = 0;
                            AI(car)->state.x2C.fx = 0;
                        }
                    }
                }
                wet = s->f1A || st->f22 > 0;
                if (wet && c->w[0].depth > 0 && c->w[3].depth > 0) {
                    s->f1A = 0;
                    p = blk->fire_effect;
                    if (p)
                        p->active = 0;
                }
                if (kind == 4 && !s->sinking) {
                    s->sinking = 1;
                    s->f1F = 1;
                    if (rand() & 1) {
                        c->w[0].level = -1;
                        c->w[1].level = 0;
                    } else {
                        c->w[0].level = 0;
                        c->w[1].level = -1;
                    }
                    c->w[2].level = 0;
                    c->w[3].level = 0;
                    carBodyObjectHide(WHEELS_KEY);
                }
                if (s->sinking) {
                    s->f0D = 1;
                    st->hp -= (s16)timerGetFrameTicks();
                    m->speed = m->speed / 2;
                    m->f00 = m->f00 / 2;
                    c->w[0].level -= 4;
                    c->w[1].level -= 4;
                    c->w[2].level -= 2;
                    c->w[3].level -= 2;
                    if (c->w[0].level < c->w[1].level)
                        c->w[0].level -= 1;
                    else
                        c->w[1].level -= 1;
                    if (rand() & 1)
                        carWheelsSpawnFx(c, 2);
                    else
                        carWheelsSpawnFx(c, 0);
                }
            }
        }
    } else {
        if (s->sinking)
            carBodyObjectShow(WHEELS_KEY);
        s->in_water = 0;
        s->sinking = 0;
        s->in_lava = 0;
        if (fx) {
            fx->on = 0;
            if (human)
                HREC(car)->fx = 0;
            else
                AI(car)->state.x2C.fx = 0;
        }
    }
}

extern void *mathJitterPos(s32 *pos, s32, s32, s32);
extern void fxSmokeC12(void *, s32);
extern void fxSmokeC6(void *, u8 flag);
extern void fxSmokeC24(void *, s32), fxSmokeC48(void *, s32);

/* @800ED920: one time in five, an effect at each wheel: mode 0
 * fxSmokeC6, 1 fxSmokeC12, 2 fxSmokeC24, else fxSmokeC48. */
void __attribute__((section(".text_800ED920"))) carWheelsSpawnFx(TM2Wheels *c, s32 mode)
{
    if (rand() % 5 == 0) {
        if (mode == 0) {
            fxSmokeC6(mathJitterPos(c->w[0].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC6(mathJitterPos(c->w[1].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC6(mathJitterPos(c->w[2].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC6(mathJitterPos(c->w[3].pos, 0x555, 0x555, 0x555), 0);
        } else if (mode == 1) {
            fxSmokeC12(mathJitterPos(c->w[0].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC12(mathJitterPos(c->w[1].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC12(mathJitterPos(c->w[2].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC12(mathJitterPos(c->w[3].pos, 0x555, 0x555, 0x555), 0);
        } else if (mode == 2) {
            fxSmokeC24(mathJitterPos(c->w[0].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC24(mathJitterPos(c->w[1].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC24(mathJitterPos(c->w[2].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC24(mathJitterPos(c->w[3].pos, 0x555, 0x555, 0x555), 0);
        } else {
            fxSmokeC48(mathJitterPos(c->w[0].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC48(mathJitterPos(c->w[1].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC48(mathJitterPos(c->w[2].pos, 0x555, 0x555, 0x555), 0);
            fxSmokeC48(mathJitterPos(c->w[3].pos, 0x555, 0x555, 0x555), 0);
        }
    }
}

extern u8 worldSectionFirstSurface[];             /* ground kind -> first surface */
extern u8 worldSectionLastSurface[];             /* ground kind -> last surface */
extern s32 terrainSurfaceCount;              /* surface count (moving surfaces last) */
#pragma tm2_absolute terrainSurfaceCount
u8 carWheelOverSurface(TM2Wheels *, TM2Wheel *, s32 surface);
extern s32 worldMovingSurfaceCount(s32 kind);
u8 carWheelLandsOnSurface(TM2Wheel *, s32 surface);
void carWheelLandingFall(TM2Wheels *, TM2Wheel *, s32 surface);

/* @800EDBA8: probe the surfaces of the wheel's ground kind, then the
 * kind's moving surfaces (the last worldMovingSurfaceCount of the terrainSurfaceCount); update
 * on / touch / surface and land the wheel when it reaches the ground.
 * Returns whether any surface is under the wheel. Reusing last (not a
 * separate end) for the second loop gives retail's registers. */
u8 __attribute__((section(".text_800EDBA8"))) carWheelProbe(TM2Wheels *c, TM2Wheel *w)
{
    u8 hit;
    s32 surface, i, n, last;
    u8 was;

    surface = w->surface;
    was = w->on != 0;
    w->f05 = 0;
    hit = 0;
    i = w->kind;
    /* (s16): loads the byte straight into a full register (a plain
     * u8 -> s32 assignment costs an andi here). */
    n = (s16)worldSectionFirstSurface[i];
    last = worldSectionLastSurface[i];
    for (i = n; i <= last; i++)
        if (carWheelOverSurface(c, w, i))
            hit = 1;
    n = worldMovingSurfaceCount(w->kind);
    w->moving = 0;
    if (n > 0) {
        {
            s32 t = terrainSurfaceCount;
            n = t - n;
            last = t;
            i = n;
        }
        for (; i < last; i++) {
            if (carWheelOverSurface(c, w, i)) {
                hit = 1;
                w->moving = 1;
                w->object = terrainSurfaceCount - i;
                w->object -= 1;
            }
        }
    }
    if (!hit) {
        if (w->f03)
            hit = 1;
        w->h = w->f26;
    }
    if (hit && !was) {
        w->surface = w->next_surface;
        w->on = 1;
        if (w->h < w->pos[2]) {
            if (!w->touch) {
                w->land_z = w->pos[2];
                w->touch = 1;
                w->f12 = 0;
            }
            w->level = w->pos[2];
            w->fall = -32 / (s16)timerGetFrameTicks();
        }
    } else if (!was)
        goto done;
    else if (!hit) {
        if (!w->touch && w->h < w->f1C + 10) {
            w->touch = 1;
            w->f12 = 0;
            w->land_z = w->pos[2];
            carWheelLandingFall(c, w, surface);
        }
    } else {
        if (surface == w->next_surface)
            goto done;
        w->surface = w->next_surface;
        if (!w->touch && carWheelLandsOnSurface(w, surface)) {
            w->touch = 1;
            w->f12 = 0;
            w->land_z = w->pos[2];
            carWheelLandingFall(c, w, surface);
        }
    }
    w->depth = 0;
done:
    return hit;
}

extern s32 worldSurfaceHeightAt(TM2Surface *, s32 *v);
s32 worldRoundSurfaceHeightAt(TM2Surface *, s32 dist, s32 angle);
extern s32 sub_8013FC44(s32);
extern int ratan2(int, int);
void carWheelLandingFallRound(TM2Wheels *c, TM2Wheel *w, s32 to);

/* @800EDED0: whether moving from the wheel's surface onto surface `to`
 * lands the wheel: not between two flagged round surfaces, not between
 * level planes of the same height. */
u8 __attribute__((section(".text_800EDED0"))) carWheelLandsOnSurface(TM2Wheel *w, s32 to)
{
    u8 r = 1;
    s32 from = w->surface;

    if (worldSurfaces[to].type == 2 && worldSurfaces[from].type == worldSurfaces[to].type
        && worldSurfaces[to].f01)
        r = !worldSurfaces[from].f01;
    if (r && worldSurfaces[to].h[0] == worldSurfaces[to].h[1]
        && worldSurfaces[to].h[0] == worldSurfaces[to].h[2]
        && worldSurfaces[to].h[0] == worldSurfaces[to].h[3]
        && worldSurfaces[to].h[0] == worldSurfaces[from].h[0]
        && worldSurfaces[to].h[0] == worldSurfaces[from].h[1]
        && worldSurfaces[to].h[0] == worldSurfaces[from].h[2]
        && worldSurfaces[to].h[0] == worldSurfaces[from].h[3])
        r = 0;
    return r;
}

/* @800EE038: fall speed of a wheel landing on surface `to`: the surface
 * height under the wheel now minus where it was before this frame's move,
 * per tick (x32). */
void __attribute__((section(".text_800EE038"))) carWheelLandingFall(TM2Wheels *c, TM2Wheel *w, s32 to)
{
    TM2Surface *p = &worldSurfaces[to];
    s32 v[3];
    s32 dx, dy, a, b;

    if (p->type == 2) {
        carWheelLandingFallRound(c, w, to);
        return;
    }
    dx = w->pos[0] - p->x;
    dy = w->pos[1] - p->y;
    v[0] = (dx * p->cs - dy * p->sn) / 4096;
    v[1] = (dy * p->cs + dx * p->sn) / 4096;
    a = worldSurfaceHeightAt(p, v);
    dx -= c->move[0];
    dy -= c->move[1];
    v[0] = (dx * p->cs - dy * p->sn) / 4096;
    v[1] = (dy * p->cs + dx * p->sn) / 4096;
    b = worldSurfaceHeightAt(p, v);
    w->fall = (a - b) * 32 / (s16)timerGetFrameTicks();
}

/* @800EE244: carWheelLandingFall for a round surface. */
void __attribute__((section(".text_800EE244"))) carWheelLandingFallRound(TM2Wheels *c, TM2Wheel *w, s32 to)
{
    TM2Surface *p = &worldSurfaces[to];
    s32 dx, dy, a, b, ang;

    dx = w->pos[0] - p->x;
    dy = w->pos[1] - p->y;
    ang = ratan2(dx, dy);
    a = worldRoundSurfaceHeightAt(p, sub_8013FC44(dx * dx + dy * dy), ang);
    dx -= c->move[0];
    dy -= c->move[1];
    ang = ratan2(dx, dy);
    b = worldRoundSurfaceHeightAt(p, sub_8013FC44(dx * dx + dy * dy), ang);
    w->fall = (a - b) * 32 / (s16)timerGetFrameTicks();
}

void carWheelVerticalMotion(TM2Wheel *, s32 found);
u8 carWheelOverRoundSurface(TM2Wheels *, TM2Wheel *, s32 surface);

/* @800EE3A4: per-wheel update after the probe: limit the fall speed by the
 * car speed; on a surface or the ground carWheelVerticalMotion, damp the fall at low
 * speed in the air and track the highest landing point; in free fall,
 * fall at 32 per tick and count the air time (+0x28, up to 600). */
void __attribute__((section(".text_800EE3A4"))) carWheelUpdate(s32 speed, s32 full, TM2Wheel *w, s32 found)
{
    s32 v, f;

    v = __builtin_abs(speed);
    if (!w->f03) {
        f = __builtin_abs(v / 64) * 32;
        f = f / (s16)timerGetFrameTicks();
        if (f < w->fall)
            w->fall = f;
        else if (w->fall < -f)
            w->fall = -f;
    }
    if (w->on || w->touch) {
        carWheelVerticalMotion(w, (u8)found);
        if (!w->touch) {
            if (v < full / 2)
                w->fall = w->fall * v / (full / 2);
        } else if (w->land_z < w->pos[2])
            w->land_z = w->pos[2];
    } else {
        w->fall = -32 / (s16)timerGetFrameTicks();
        w->f12 = 0;
        if (w->depth < 600)
            w->depth += timerGetFrameTicks();
    }
}

/* The wheel gets onto surface s (carWheelOverSurface / carWheelOverRoundSurface). Retail
 * repeats this block in each of the three accepting branches (instead of
 * one goto target): the extra references to w, z and s are what give them
 * retail's register allocation (no do-while: that adds code). Use inside
 * braces only. */
#define WHEEL_TAKE()     w->f05 = 1;     w->h = z;     w->next_surface = s;     w->on = 1;     r = 1

/* @800EE5F0: is the wheel over surface `s` (rectangle / triangle in the
 * surface axes; type 2 round, carWheelOverRoundSurface) and can it get onto it: not
 * below the best surface found so far, and either at most a step above
 * the wheel now or before this frame's move, or the car's own surface.
 * Records the surface as the wheel's next one. */
u8 __attribute__((section(".text_800EE5F0"))) carWheelOverSurface(TM2Wheels *c, TM2Wheel *w, s32 s)
{
    TM2Surface *p = &worldSurfaces[s];
    u8 r = 0;
    u8 in;
    s32 v[3];
    s32 dx, dy, z, step;

    if (p->type == 2)
        r = carWheelOverRoundSurface(c, w, s);
    else {
        dx = w->pos[0] - p->x;
        dy = w->pos[1] - p->y;
        v[0] = (dx * p->cs - dy * p->sn) / 4096;
        v[1] = (dy * p->cs + dx * p->sn) / 4096;
        in = 0;
        if (v[0] >= 0 && v[1] >= 0 && v[0] <= p->w && v[1] <= p->l) {
            if (p->type == 1)
                in = !(v[0] * p->l / p->w < v[1]);
            else
                in = 1;
        }
        if (in) {
            z = worldSurfaceHeightAt(p, v);
            if (w->f05)
                in = !(z < w->h);
            else
                in = 1;
            if (in) {
                step = c->step;
                if (z - w->pos[2] < step) {
                    WHEEL_TAKE();
                } else {
                    dx -= c->move[0];
                    dy -= c->move[1];
                    v[0] = (dx * p->cs - dy * p->sn) / 4096;
                    v[1] = (dy * p->cs + dx * p->sn) / 4096;
                    if (worldSurfaceHeightAt(p, v) - (w->pos[2] - c->move[2]) < step) {
                        WHEEL_TAKE();
                    } else if (c->surface == s) {
                        WHEEL_TAKE();
                    }
                }
            }
        }
    }
    return r;
}

/* @800EEBC0: carWheelOverSurface for a round surface: inside the angle range
 * (sn..cs, wrapped past 2048) and the radius range (w..l) around the
 * origin. */
u8 __attribute__((section(".text_800EEBC0"))) carWheelOverRoundSurface(TM2Wheels *c, TM2Wheel *w, s32 s)
{
    TM2Surface *p = &worldSurfaces[s];
    s32 dx, dy, ang, d, z, step;
    u8 r, ok;

    dx = w->pos[0] - p->x;
    dy = w->pos[1] - p->y;
    ang = ratan2(dx, dy);
    r = 0;
    if (p->cs > 2048 && ang < 0)
        ang += 4096;
    if (ang >= p->sn && ang <= p->cs) {
        d = sub_8013FC44(dx * dx + dy * dy);
        if (d >= p->w && d <= p->l) {
            z = worldRoundSurfaceHeightAt(p, d, ang);
            if (w->f05)
                ok = !(z < w->h);
            else
                ok = 1;
            if (ok) {
                step = c->step;
                if (z - w->pos[2] < step) {
                    WHEEL_TAKE();
                } else {
                    dx -= c->move[0];
                    dy -= c->move[1];
                    ang = ratan2(dx, dy);
                    if (worldRoundSurfaceHeightAt(p, sub_8013FC44(dx * dx + dy * dy), ang) - (w->pos[2] - c->move[2]) < step) {
                        WHEEL_TAKE();
                    } else if (c->surface == s) {
                        WHEEL_TAKE();
                    }
                }
            }
        }
    }
    return r;
}

/* @800EEE28: height of a round surface at radius d and angle ang: the
 * corner heights h[0..1] (inner edge) and h[2..3] (outer edge) run along
 * the angle range sn..cs; flat-across surfaces (f01 clear) slope either
 * across the radius (h[2] - h[0]) or along the angle (h[1] - h[0]). */
s32 __attribute__((section(".text_800EEE28"))) worldRoundSurfaceHeightAt(TM2Surface *p, s32 d, s32 ang)
{
    s32 a, span, dr, width, h0, h2, v, t;

    a = ang - p->sn;
    while (a < 0)
        a += 4096;
    while (a > 180)
        a -= 4096;
    span = p->cs - p->sn;
    dr = d - p->w;
    width = p->l - p->w;
    if (p->f01) {
        h0 = p->h[0] + (p->h[1] - p->h[0]) * a / span;
        h2 = p->h[2] + (p->h[3] - p->h[2]) * a / span;
        t = h2 - h0;
        return h0 + t * dr / width;
    }
    t = p->h[2] - p->h[0];
    if (!t) {
        t = p->h[1] - p->h[0];
        v = t * a / span;
    }
    else
        v = t * dr / width;
    return p->h[0] + v;
}

/* @800EF0B0: vertical motion of a wheel over its surface. On the ground:
 * move by the fall speed plus the bounce (f2C, from the bounce counter f12
 * which grows while not dropping fast); once below the surface height h
 * with enough bounce, or more than 10 below, the wheel settles: back onto
 * the surface when one was found, otherwise off it. In the air: fall speed
 * from the height left to drop, count the air time, settle. */
void __attribute__((section(".text_800EF0B0"))) carWheelVerticalMotion(TM2Wheel *w, s32 found)
{
    s32 d;

    if (w->touch) {
        d = w->fall * (s16)timerGetFrameTicks() / 32;
        if (w->f12 > 0) {
            w->f2C = (s16)timerGetFrameTicks() * -(w->f12 * 2) * w->f12 / 16;
            d -= 1;
        } else
            w->f2C = 0;
        d += w->f2C / 32;
        if (-((s16)timerGetFrameTicks() * 32) < d)
            w->f12 += timerGetFrameTicks();
        w->level += d;
        if (w->level < w->h && (w->f12 >= 9 || w->level + 10 < w->h)) {
            w->touch = 0;
            w->f12 = 0;
            if (!(u8)found) {
                w->on = 0;
                w->level = 0;
            } else
                w->level = w->h;
        }
        w->depth = 0;
    } else {
        w->fall = (w->h - w->level) * 32 / (s16)timerGetFrameTicks();
        w->depth += timerGetFrameTicks();
        w->level = w->h;
        w->touch = 0;
        w->f12 = 0;
        if (!(u8)found) {
            w->on = 0;
            w->level = 0;
        }
    }
}

typedef struct { s32 z; u8 pad_04[8]; } TM2KindInfo;

extern TM2KindInfo worldSectionFloorHeights[];    /* ground kind: base height */
extern u8 worldSectionHasFloor[];             /* ground kind: no lower floor */
extern void wheelBlockReset(TM2Wheels *);
extern void *memcpy(void *, void *, unsigned int);

/* @800EF340: ground height under a point: probe with a scratch wheel from
 * the point's height down in steps of 40 until a surface is found (or the
 * ground kind has no lower floor, or z goes below 0). */
s32 __attribute__((section(".text_800EF340"))) worldGroundHeightAt(s32 *pos)
{
    TM2Wheels c;
    s32 z, kind, i, last, z0;
    u8 found = 0;

    z = pos[2];
    wheelBlockReset(&c);
    do {
        kind = worldSectionAt(pos[0], pos[1], z);
        c.step = 120;
        c.move[0] = 0;
        c.move[1] = 0;
        c.move[2] = 0;
        z0 = worldSectionFloorHeights[kind].z;    /* via a local: retail keeps the lw */
        c.w[0].f05 = 0;
        c.w[0].f26 = z0;
        c.w[0].h = z0;
        memcpy(c.w[0].pos, pos, 12);
        c.w[0].on = 0;
        i = (s16)worldSectionFirstSurface[kind];
        last = worldSectionLastSurface[kind];
        for (; i <= last; i++)
            if (carWheelOverSurface(&c, &c.w[0], i))
                found = 1;
        if (worldSectionHasFloor[kind])
            found = 1;
        else {
            z -= 40;
            if (z < 0)
                found = 1;
        }
    } while (!found);
    return c.w[0].h;
}

/* Route points (aiRouteNodes, 24 bytes; see ai_steps.c). */
typedef struct { s32 x, y; s16 z; u8 pad[0x0D]; s8 route; } TM2RoutePoint;

extern TM2RoutePoint aiRouteNodes[];
extern s32 aiRouteNodeCount;              /* route point count */
#pragma tm2_absolute aiRouteNodeCount

/* @800EF48C: index of the route point of the car's route (human +0xC8, AI
 * +0x46A; level 8 maps AI routes 4..7 to 24) closest to the car (sum of
 * the axis distances). car is a human record when human is set, else an AI
 * record. */
s32 __attribute__((section(".text_800EF48C"))) carNearestRoutePoint(void *car, u8 human)
{
    TM2WheelMotion *m;
    s32 route, i, best, d, bi, dx, dy, dz;

    if (human) {
        m = (TM2WheelMotion *)&HREC(car)->motion;
        route = HREC(car)->state.x2C.route;
    } else {
        m = (TM2WheelMotion *)&AI(car)->motion;
        route = DRV(car)->route;
        if (gameGetLevel() == 8 && (u32)(route - 4) < 4)
            route = 24;
    }
    best = 0x7FFFFFFF;
    bi = 0;
    for (i = 0; i < aiRouteNodeCount; i++) {
        if (aiRouteNodes[i].route == route) {
            dx = m->pos[0] - aiRouteNodes[i].x;
            dy = m->pos[1] - aiRouteNodes[i].y;
            dz = m->pos[2] - aiRouteNodes[i].z;
            d = __builtin_abs(dx) + __builtin_abs(dy) + __builtin_abs(dz);
            if (d < best) {
                bi = i;
                best = d;
            }
        }
    }
#ifdef TM2_PORT_BALANCE
    if (best == 0x7FFFFFFF)     /* request 101 (bit 29): no node of the car's section: the nearest of any, not 0 */
        bi = portBalanceRouteNearest(m->pos, bi);
#endif
    return bi;
}
