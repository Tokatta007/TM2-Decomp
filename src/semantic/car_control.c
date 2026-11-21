/* Car control (800DD304). */
#include "semantic/cars.h"

/* The human and AI records keep the same blocks at different offsets (see
 * include/semantic/cars.h); this step works on block pointers. */
#define HUMAN(car) ((struct TM2HumanRecord *)(car))
#define AI(car) ((struct TM2AIRecord *)(car))

/* Retail re-sign-extends the result in this unit (as with an s16 return);
 * timerGetFrameTicks returns int (timer_frame.c), and other units use it so. */
extern s16 timerGetFrameTicks(void);
extern s32 mathBlendAngle(s32, s32, s32);
#include "semantic/sound.h"
extern u8 soundGetTurboLoopP1(void);
extern u8 soundGetTurboLoopP2(void);
extern s32 aiGetDifficulty(void);
extern s32 rand(void);
extern int *fxSmokePuffBlue(void *, s32);
extern void fxSmokeC48(void *, s32);

/* Pad 0/1 feedback refresh for the human car (player number at car+0). */
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: TM2_HUMAN_SOUND (retail's soundPlay without the define) */
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"
#endif
#if defined(TM2_PORT_CAR_SOUND) || defined(TM2_PORT_HUMANS)
/* request 33-4 (CAR_GROUPS.md 16): the turbo loop of players 2.. per car (retail's calls for players 0 and 1);
 * H3: by the human's sound side (its local slot 0/1) */
#define PAD_REFRESH(car)                                         \
    portHumanLoopStop(0, HUMAN(car)->player, HUMAN(car)->slot);
#else
#define PAD_REFRESH(car)                                         \
    if (HUMAN(car)->player == 0 && soundGetTurboLoopP1())                      \
        soundStopPlayerLoop(0, HUMAN(car)->player);                            \
    if (HUMAN(car)->player == 1 && soundGetTurboLoopP2())                      \
        soundStopPlayerLoop(0, HUMAN(car)->player);
#endif

/* @800DD304: car throttle/steer step shared by human (human=1) and AI cars.
 * Blocks: in = input flags (human +0xD0 / AI +0x44), fl = state flags
 * (+0x9C / +0x6C), geo = handling params (+0x14C / +0x4FC), mot = motion
 * (+0x6DC / +0x71C: +0 steer, +0xA speed, +0x14 yaw rate), st = boost state
 * (+0x7C8 / +0x808: +0x32 boost fuel, +0x44 effect object).
 * Steers mot+0 towards the steering limit, derives the yaw rate, handles
 * the boost (in+3) with its pad rumble, then brakes (in+0x13), accelerates
 * (in+0x12), coasts down, and finally applies the slope term (mot+0x38). */
void __attribute__((section(".text_800DD304"))) carThrottleSteer(void *car, u8 human)
{
    struct TM2CarInput *in;
    struct TM2CarState *fl;
    struct TM2CarStatsBlock *geo;
    struct TM2CarMotion *mot;
    struct TM2CarWeapons *st;
    struct TM2FireEffect *obj;
    s32 speed, maxsp, accel, lim, step, t, n;
    u8 flag;
#ifdef TM2_PORT_HUMANS
    s32 keep;
#endif

    if (human) {
        in = &HUMAN(car)->input;
        fl = &HUMAN(car)->state;
        geo = &HUMAN(car)->stats;
        mot = &HUMAN(car)->motion;
        st = &HUMAN(car)->weapons;
    } else {
        in = &AI(car)->input;
        fl = &AI(car)->state;
        geo = &AI(car)->stats;
        mot = &AI(car)->motion;
        st = &AI(car)->weapons;
    }
    speed = __builtin_abs(mot->speed);
    maxsp = speed < geo->speed_cap ? geo->speed_cap : speed;
    accel = geo->accel_launch;

    /* steering limit */
    if (in->power_slide || in->slide) {
        lim = geo->steer_limit * geo->handbrake_lock;
        if (fl->wheel_in_air)
            lim /= 4;
        else if (in->slide && in->power_slide)
            lim += lim * (speed << 1) / maxsp;
    } else {
        lim = geo->steer_limit;
        if (fl->wheel_in_air)
            lim /= 2;
    }
    if (fl->frozen) {
        in->steer_dec = 0;
        in->steer_inc = 0;
    }
    if (in->steer_inc || in->steer_dec) {
        if (timerGetFrameTicks() * 1520 / 100 * 32 < speed)
            step = geo->steer_rate_high;
        else
            step = geo->steer_rate_low - speed * (geo->steer_rate_low - geo->steer_rate_high) / (timerGetFrameTicks() * 1520 / 100 * 32);
        if (!fl->f01 && ((in->steer_inc && mot->steer <= 0) || (in->steer_dec && mot->steer >= 0)))
            step *= 2;
        if (geo->steer_boost_speed < speed && (in->power_slide || in->slide)) {
            step *= geo->handbrake_lock;
            if (in->slide && in->power_slide)
                step += geo->steer_rate_high;
        }
        if (in->steer_inc) {
            mot->steer += step;
            if (lim < mot->steer)
                mot->steer = lim;
        } else {
            mot->steer -= step;
            if (mot->steer < -lim)
                mot->steer = -lim;
        }
    } else if (mot->steer != 0) {
        if (__builtin_abs(mot->steer) < geo->steer_rate_low)
            mot->steer = 0;
        else if (fl->wheel_in_air)
            mot->steer -= mot->steer / 16;
        else
            mot->steer = mot->steer / timerGetFrameTicks();
    }

    /* yaw rate */
#ifdef TM2_PORT_HUMANS
    /* request 66 (ANALOG_STEERING.md 2.6): with the human's portHumanControl bit 0 the rolling turn takes the speed's
     * size, not its sign, so it keeps the sign of the turn in place below accel_launch (retail flips it in reverse);
     * 0 for the AI cars and with the bit clear (retail) */
    keep = portHumanSteerKeep(car, human);
#define YAW_SPEED (keep ? speed : mot->speed)
#else
#define YAW_SPEED (mot->speed)
#endif
    if (geo->accel_launch < speed) {
        if (speed / 32 < geo->turn_speed)
            mot->yaw_rate = YAW_SPEED / 32 * mot->steer / geo->turn_speed;
        else {
            mot->yaw_rate = mot->steer;
            if (YAW_SPEED < 0)
                mot->yaw_rate *= -1;
        }
    } else if (fl->f06)
        mot->yaw_rate = mathBlendAngle(mot->yaw_rate, mot->steer / 2, 0x5F);
    else
        mot->yaw_rate = 0;

    if (fl->frozen) {
        in->brake = 1;
        in->throttle = 0;
    }
    if (fl->burning) {
        if (human) {
            if (in->turbo && st->turbo >= 51)
                st->turbo -= 50;
            else {
                if (in->turbo)
                    TM2_HUMAN_SOUND(HUMAN(car)->player, 0x3F, 0, 0, 99);   /* H3: the player's own cue */
                goto check;
            }
        } else {
            n = aiGetDifficulty();
            if (n <= 0)
                t = 120;
            else if (n < 2)
                t = 60;
            else
                t = 30;
            if (rand() & 1)
                t *= 2;
            if (t >= geo->burn_time)
                goto check;
        }
        fl->burning = 0;
    check:
        if (!fl->burning) {
            obj = st->fire_effect;
            if (obj)
                obj->active = 0;
            if (human) {
                fxSmokePuffBlue(mot->pos, 0);
                fxSmokeC48(mot->pos, 0);
            }
        }
    }
    if (fl->energy_shield && fl->burning) {
        fl->burning = 0;
        obj = st->fire_effect;
        if (obj)
            obj->active = 0;
    }

    /* boost */
    if (in->turbo) {
        in->throttle = 0x7F;
        if (st->turbo > 0) {
            maxsp = geo->speed_cap * 4 / 3;
            if (maxsp / 32 * 100 / (timerGetFrameTicks() * 19) < 140)
                maxsp = timerGetFrameTicks() * 2660 / 100 * 32;
            if (maxsp / 32 * 100 / (timerGetFrameTicks() * 19) >= 181)
                maxsp = timerGetFrameTicks() * 3420 / 100 * 32;
            accel = geo->accel_launch * 4;
            st->turbo -= 1;
            if (human) {
#if defined(TM2_PORT_CAR_SOUND) || defined(TM2_PORT_HUMANS)
                portHumanLoopPlay(0, 0, 0, 99, HUMAN(car)->player, HUMAN(car)->slot);   /* request 33-4, H3 */
#else
                soundStartPlayerLoop(0, 0, 0, 99, HUMAN(car)->player);
#endif
                if ((!in->reverse && mot->speed <= 0) || (in->reverse && mot->speed >= 0)) {
                    if (!fl->in_lava && !fl->in_water && !fl->wheel_in_air)
                        TM2_HUMAN_SOUND(HUMAN(car)->player, 1, 0, 0, 99);
                }
            }
        } else {
            maxsp = geo->speed_cap;
            accel = geo->accel_launch;
            if (mot->speed > geo->speed_cap)
                in->throttle = 0;
            if (human) {
                PAD_REFRESH(car)
                TM2_HUMAN_SOUND(HUMAN(car)->player, 0x3F, 0, 0, 99);
            }
        }
    } else {
        if (mot->speed > geo->speed_cap)
            in->throttle = 0;
        if (human) {
            PAD_REFRESH(car)
        }
    }

    if (in->brake) {
        /* brake */
        if (mot->speed != 0 && !fl->all_in_air) {
            if (mot->speed > 0) {
                step = geo->brake_decel;
                if (in->forward)
                    step /= 2;
                mot->speed -= step;
                if (mot->speed < 0)
                    mot->speed = 0;
            } else {
                step = geo->brake_decel;
                if (in->reverse)
                    step /= 2;
                mot->speed += step;
                if (mot->speed > 0)
                    mot->speed = 0;
            }
        }
        fl->accelerating = 0;
        fl->f08 = 0;
        if (mot->speed < 0)
            fl->reversing = 1;
        else
            fl->reversing = 0;
    } else if (in->throttle) {
        /* throttle (reverse when in+0xE is held at/below zero speed) */
        fl->accelerating = 1;
        fl->f08 = 0;
        if (in->reverse && mot->speed <= 0)
            fl->reversing = 1;
        else
            fl->reversing = 0;
        if (((fl->reversing && mot->speed > -geo->reverse_cap) || (!fl->reversing && mot->speed < maxsp)) && !fl->f01) {
            step = accel;
            if ((fl->reversing && mot->speed < 0) || (!fl->reversing && mot->speed > 0))
                step -= (accel - geo->accel_top) * speed / maxsp;
            if (fl->reversing)
                mot->speed -= step;
            else
                mot->speed += step;
        }
    } else
        fl->accelerating = 0;

    /* coast down */
    if (mot->speed != 0 && !fl->accelerating && !fl->f08 && !fl->wheel_in_air) {
        step = (geo->brake_decel - geo->accel_top) * speed / maxsp;
        step /= 2;
        if (step < geo->accel_top)
            step = geo->accel_top;
        if (mot->speed > 0) {
            mot->speed -= step;
            if (mot->speed < 0)
                mot->speed = 0;
        } else {
            mot->speed += step;
            if (mot->speed > 0)
                mot->speed = 0;
        }
    }
    if (in->reverse) {
        if (mot->speed <= 0) {
            fl->reversing = 1;
            if (mot->speed > -geo->reverse_cap) {
                fl->accelerating = 0;
                fl->f08 = 0;
            }
        }
    } else
        fl->reversing = 0;

    if (human) {
        if (fl->f12)
            return;
        if (!fl->wheel_in_air)
            goto slope;
        return;
    } else if (fl->f12)
        return;
slope:
    /* slope */
    flag = 0;
    if (!fl->riding) {
        if (human) {
            if (speed >= geo->steer_boost_speed || !in->drive)
                flag = 1;
        } else
            flag = fl->dead != 0;
    }
    if (flag) {
        s32 d, m, e;
        d = timerGetFrameTicks() * 10 * geo->accel / 5;
        m = timerGetFrameTicks() * 7 * geo->accel / 5;
        e = d * mot->body_rot[0] / 170;
        if (m < e)
            e = m;
        else if (e < -m)
            e = -m;
        mot->speed += e;
    }
}
