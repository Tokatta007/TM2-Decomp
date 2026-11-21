/* Car spin-out (800E5FFC): carSpinOut(car, human, kind, keep_motion)
 * throws car `car` (a human record when human is set, else an AI record)
 * into a spin / wheel bounce. Does nothing while a spin is
 * already running (state +11). `kind` picks the strength:
 *   7: the energy-shield bounce (sets state +39), 448 x 2/3;
 *   otherwise 288 scaled by (100 - mass)%, at least 96, then
 *   1, 8: 96 / 2;  4 (damage): / 3;  5: x 2 (sets state +39);
 *   10: / 2 and a random heading kick of +-(stat +112 .. 2 x stat +112);
 *   11: 96 x 2;  12: 288 x 2;  13: 288 x 3/2.
 * The four wheels (72-byte records at +24 of the wheel block) get the
 * strength as their bounce speed. Kinds 5, 7, 12, 13 lift wheels at random
 * against the current yaw rate; the rest pick one of 8 bounce patterns that
 * also nudge the car's turn (motion +64, unless `keep_motion`) and set the
 * spin rate (+1988 / +2052) from stat +98, scaled again by kind afterwards.
 * Computer cars get no spin rate; in game modes 5 and 6 their motion
 * +8 / +10 is cleared too.
 * The car blocks are those of car_damage.c: wheels human +1444 / AI +1508,
 * state +156 / +108, motion +1756 / +1820, stats +332 / +1276. */
#include "semantic/cars.h"

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

/* x / fps * fps (timerGetFrameTicks, see docs/CAR_STATS.md) */
#define RATE(x) ((x) / (s16)timerGetFrameTicks() * (s16)timerGetFrameTicks())

typedef struct {
    u8 on;                  /* +0 */
    u8 f01;                 /* +1 */
    u8 pad_02;
    u8 f03;                 /* +3 */
    u8 pad_04[6];
    s16 f0A;                /* +10 */
    s16 f0C;                /* +12 */
    u8 pad_0E[4];
    s16 bounce;             /* +18 */
    s16 speed;              /* +20 */
    s16 f16;                /* +22 */
    u8 pad_18[0x12];
    s16 f2A;                /* +42 */
    u8 pad_2C[0x18];
    s32 f44;                /* +68 */
} TM2Wheel;                 /* 72 bytes */

typedef struct {
    u8 pad_00[24];
    TM2Wheel w[4];
} TM2Wheels;

typedef struct {
    u8 pad_00;
    u8 spinning;            /* +1 */
    u8 pad_02[9];
    u8 spin;                /* +11 */
    u8 pad_0C[27];
    u8 shield_bounce;       /* +39 */
    u8 shield_bounced;      /* +40 */
} TM2SpinState;

typedef struct {
    s16 heading;            /* +0 */
    u8 pad_02[6];
    s16 f08;                /* +8 */
    s16 yaw_rate;           /* +10 */
    u8 pad_0C[0x34];
    s32 turn;               /* +64 */
} TM2SpinMotion;

typedef struct {
    u8 pad_00[0x3C];
    s16 shield;             /* +60 */
    u8 pad_3E[0x24];
    s16 spin_rate;          /* +98 */
    u8 pad_64[0x0C];
    s16 kick;               /* +112 */
    u8 pad_72[8];
    s16 mass;               /* +122 */
} TM2SpinStats;

#include "semantic/world.h"
extern s32 GsGetWorkBase_0(void);
extern int rand(void);
extern s32 gameGetLevel(void);

#pragma tm2_rdata .tm2_800CEB40

/* @800E5FFC */
void __attribute__((section(".text_800E5FFC"))) carSpinOut(car, human, kind, keep_motion)
void *car; u8 human; s32 kind; u8 keep_motion;   /* old-style: callers pass ints, the body narrows */
{
    TM2Wheels *wh;
    TM2SpinState *s;
    TM2SpinMotion *m;
    TM2SpinStats *st;
    s32 *rate;
    s32 mass, speed, dir;

    if (human) {
        wh = (TM2Wheels *)&HREC(car)->wheels;
        s = (TM2SpinState *)&HREC(car)->state;
        mass = HREC(car)->stats.mass;
        m = (TM2SpinMotion *)&HREC(car)->motion;
        st = (TM2SpinStats *)&HREC(car)->stats;
        rate = &HREC(car)->motion.fE8;
    } else {
        wh = (TM2Wheels *)&AI(car)->wheels;
        s = (TM2SpinState *)&AI(car)->state;
        mass = AI(car)->stats.mass;
        m = (TM2SpinMotion *)&AI(car)->motion;
        st = (TM2SpinStats *)&AI(car)->stats;
        rate = &AI(car)->motion.fE8;
    }
    if (human && st->shield && kind == 7 && s->shield_bounce && !s->shield_bounced) {
        s->shield_bounced = 1;
        s->spin = 0;
        wh->w[0].bounce = 0;
        wh->w[1].bounce = 0;
        wh->w[2].bounce = 0;
        wh->w[3].bounce = 0;
    }
    if (s->spin)
        return;
    if (kind == 7) {
        speed = RATE(448) * 2 / 3;
        s->shield_bounce = 1;
    } else {
        s->shield_bounce = 0;
        speed = RATE(288) * (100 - mass) / 100;
        if (speed < RATE(96))
            speed = RATE(96);
    }
    switch (kind) {
    case 4:
        speed /= 3;
        break;
    case 1:
    case 8:
        speed = RATE(96) / 2;
        break;
    case 5:
        s->shield_bounce = 1;
        speed *= 2;
        break;
    case 10:
        speed /= 2;
        if (GsGetWorkBase_0() & 1)
            m->heading = st->kick + rand() % st->kick;
        else
            m->heading = -st->kick - rand() % st->kick;
        break;
    case 11:
        speed = RATE(96) * 2;
        break;
    case 13:
        speed = RATE(288) * 3 / 2;
        break;
    case 12:
        speed = RATE(288) * 2;
        break;
    case 0:
    default:
        break;
    }
    wh->w[0].speed = speed;
    wh->w[1].speed = speed;
    wh->w[2].speed = speed;
    wh->w[3].speed = speed;
    wh->w[0].f2A = wh->w[0].f44;
    wh->w[1].f2A = wh->w[1].f44;
    wh->w[2].f2A = wh->w[2].f44;
    wh->w[3].f2A = wh->w[3].f44;
    if (kind == 7 || kind == 5 || kind == 12 || kind == 13) {
        dir = -1;
        if (m->yaw_rate < 0)
            dir = 1;
        wh->w[2].bounce += dir;
        wh->w[3].bounce += dir;
        if (rand() & 1) {
            wh->w[2].bounce += dir;
            if (rand() & 1) {
                wh->w[1].bounce -= dir;
                wh->w[2].bounce += dir;
            }
        } else {
            wh->w[3].bounce += dir;
            if (rand() & 1) {
                wh->w[0].bounce -= dir;
                wh->w[3].bounce += dir;
            }
        }
        *rate = 0;
    } else {
        switch (rand() & 7) {
        case 0:
        default:
            wh->w[0].bounce -= 6;
            if (!keep_motion) {
                m->turn -= 512;
                m->yaw_rate -= m->yaw_rate / 4;
            }
            *rate = -st->spin_rate;
            break;
        case 1:
            wh->w[2].bounce -= 6;
            wh->w[3].bounce -= 5;
            if (!keep_motion) {
                m->turn -= 341;
                m->yaw_rate -= m->yaw_rate / 2;
            }
            *rate = -st->spin_rate / 4;
            break;
        case 2:
            wh->w[1].bounce -= 7;
            if (!keep_motion) {
                m->turn += 512;
                m->yaw_rate -= m->yaw_rate / 4;
            }
            *rate = st->spin_rate;
            break;
        case 3:
            wh->w[1].bounce -= 7;
            wh->w[3].bounce -= 4;
            if (!keep_motion) {
                m->turn += 1024;
                m->yaw_rate -= m->yaw_rate / 2;
            }
            *rate = st->spin_rate * 2;
            break;
        case 4:
            wh->w[3].bounce -= 9;
            if (!keep_motion) {
                m->turn += 1536;
                m->yaw_rate -= m->yaw_rate / 4;
            }
            *rate = st->spin_rate;
            break;
        case 5:
            wh->w[2].bounce -= 7;
            wh->w[3].bounce -= 5;
            if (!keep_motion)
                m->turn += 341;
            *rate = st->spin_rate / 4;
            break;
        case 6:
            wh->w[2].bounce -= 6;
            if (!keep_motion) {
                m->turn -= 1536;
                m->yaw_rate -= m->yaw_rate / 4;
            }
            *rate = -st->spin_rate;
            break;
        }
    }
    wh->w[0].f01 = 1;
    wh->w[1].f01 = 1;
    wh->w[2].f01 = 1;
    wh->w[3].f01 = 1;
    wh->w[0].f03 = 1;
    wh->w[1].f03 = 1;
    wh->w[2].f03 = 1;
    wh->w[3].f03 = 1;
    if (!wh->w[0].on) {
        wh->w[0].f16 = wh->w[0].f44 + 1;
        wh->w[0].f0C = wh->w[0].f0A;
    }
    if (!wh->w[1].on) {
        wh->w[1].f16 = wh->w[1].f44 + 1;
        wh->w[1].f0C = wh->w[1].f0A;
    }
    if (!wh->w[2].on) {
        wh->w[2].f16 = wh->w[2].f44 + 1;
        wh->w[2].f0C = wh->w[2].f0A;
    }
    if (!wh->w[3].on) {
        wh->w[3].f16 = wh->w[3].f44 + 1;
        wh->w[3].f0C = wh->w[3].f0A;
    }
    wh->w[0].on = 1;
    wh->w[1].on = 1;
    wh->w[2].on = 1;
    wh->w[3].on = 1;
    s->spin = 1;
    s->spinning = 1;
    switch (kind) {
    case 4:
        *rate /= 3;
        break;
    case 1:
        *rate /= 2;
        break;
    case 2:
    case 3:
        *rate *= 2;
        break;
    case 5:
    case 7:
    case 11:
    case 12:
    case 13:
        *rate = 0;
        break;
    case 0:
    default:
        break;
    }
    if (!human) {
        *rate = 0;
        s->spinning = 0;
        if (gameGetLevel() == 5 || gameGetLevel() == 6) {
            m->f08 = 0;
            m->yaw_rate = 0;
        }
    }
}
