/* Game code (mixed). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_numbers.h"   /* request 73 stage 2: TM2_SPECIAL_NUM_ID (a mod car's special numbers) */
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 41 bit 17 (BALANCE.md 12.5) */
#endif
#include "semantic/car_groups.h"
#include "semantic/spu_voices.h"   /* request 87: the swing's sound carries the car (TM2_VOICE_TAG) */
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)

#define S32(p, n) (*(s32 *)((u8 *)(p) + (n)))
#define S16(p, n) (*(s16 *)((u8 *)(p) + (n)))
#define S8(p, n) (*(s8 *)((u8 *)(p) + (n)))
#define U8(p, n) (*((u8 *)(p) + (n)))

extern s32 humanCarCount(void);
extern s32 aiCarCount(void);
extern struct TM2Object *humanCarObject(s32);
extern struct TM2Object *aiCarObject(s32);
extern struct TM2FireEffect *fxFlameBurning(void *, s32);

struct G16Breakable {          /* 20 bytes, 50 records */
    s16 a, b;
    s16 health;
    s16 pad6;
    s32 position[3];
};

struct G16Spinner {            /* 60 bytes, 6 records */
    u8 pad0[2];
    s8 kind;                   /* +2 */
    u8 pad3;
    s16 state;                 /* +4, -1 = unused */
    u8 pad6[50];
    struct TM2Object *object;  /* +56 */
};
extern struct G16Breakable *targetPoolGet(void);
extern struct G16Spinner *targetActorsGet(void);

/* @800D6FD8: projectile hit test. Returns the owner id (+280) of the first
 * player car / enemy car within (radius+1)*8 of pos that is not `owner`
 * (applying weapon-type side effects: 14 freeze, 15/47 fire, 20 hit), or
 * 500+i for breakable i, 6000+i for spinner i, else 0. */
/* request 73 stage 2: the freeze of the shot's owner (retail 300 ticks) */
#define FREEZE_TICKS TM2_SPECIAL_NUM_ID(owner, TM2_SPN_FREEZE, 300)
s32 __attribute__((section(".text_800D6FD8"))) projectileHitTest(s32 *pos, s32 owner, s32 radius, s32 type)
{
    s32 n1, n2, i, extra;
    struct TM2Object *obj;
    struct TM2AIRecord *e;
    struct G16Breakable *b;
    struct G16Spinner *s;

    n1 = humanCarCount();
    n2 = aiCarCount();
    for (i = 0; i < n1; i++) {
        obj = humanCarObject(i);
        if (obj->visible == 0 && humanCarGet(i)->state.dead != 0)
            continue;
#ifdef TM2_PORT_DT_PLAYER
        /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth is as big a target as the AI's (+25 below) */
        extra = humanCarGet(i)->type == 0xF0 ? 25 : 0;
        if (DISTANCE(pos, obj->pos) < radius + 1 + extra) {
#else
        if (DISTANCE(pos, obj->pos) < radius + 1) {
#endif
            if (obj->id != owner) {
                if (type == 14) {
                    if (humanCarGet(i)->state.frozen == 0 && humanCarGet(i)->state.energy_shield == 0) {
                        humanCarGet(i)->state.frozen = 1;
#ifdef TM2_PORT_BALANCE
                        /* request 41 bit 17 (BALANCE.md 12.5): diminishing returns */
                        humanCarGet(i)->stats.freeze_timer = portBalanceFreeze(humanCarGet(i), 1, FREEZE_TICKS);
#else
                        humanCarGet(i)->stats.freeze_timer = FREEZE_TICKS;
#endif
                        humanCarGet(i)->stats.frozen_by_missile = 1;
#ifdef TM2_PORT_CAR_GROUPS
                        carFreezeSetState(TM2_CAR_KEY(humanCarGet(i)->slot, humanCarGet(i)->type), 1);   /* request 32 */
#else
                        carFreezeSetState(humanCarGet(i)->type, 1);
#endif
                    } else {
#ifdef TM2_PORT_BALANCE
                        humanCarGet(i)->stats.freeze_timer = portBalanceFreeze(humanCarGet(i), 1, FREEZE_TICKS);
#else
                        humanCarGet(i)->stats.freeze_timer = FREEZE_TICKS;
#endif
                    }
                }
                if (type == 15 || type == 47) {
                    if (humanCarGet(i)->state.burning == 0 && humanCarGet(i)->state.energy_shield == 0
                        && humanCarGet(i)->state.in_water == 0 && humanCarGet(i)->state.in_fire == 0) {
                        humanCarGet(i)->state.burning = 1;
                        humanCarGet(i)->weapons.fire_effect = fxFlameBurning(humanCarGet(i)->motion.pos, 1);
                    }
                    humanCarGet(i)->stats.burn_time = TM2_SPECIAL_BURN_START(owner);
                }
                if (type == 20)
                    carSpinOut(humanCarGet(i), 1, 0, 1);
                return obj->id;
            }
            if (type == 51)
                return obj->id;
        }
    }
    for (i = 0; i < n2; i++) {
        obj = aiCarObject(i);
        e = aiCarGet(i);
        if (obj->visible == 0 && e->state.dead != 0)
            continue;
        extra = e->type == 0xF0 ? 25 : 0;
        if (obj->id != owner && DISTANCE(pos, obj->pos) < radius + 1 + extra) {
            if (type == 14) {
                if (e->state.frozen == 0) {
                    e->state.frozen = 1;
#ifdef TM2_PORT_BALANCE
                    e->stats.freeze_timer = portBalanceFreeze(e, 0, FREEZE_TICKS);   /* request 41 bit 17 */
#else
                    e->stats.freeze_timer = FREEZE_TICKS;
#endif
                    e->stats.frozen_by_missile = 1;
                    e->stats.f1E = 0;
#ifdef TM2_PORT_CAR_GROUPS
                    carFreezeSetState(TM2_CAR_KEY(e->slot, e->type), 1);   /* request 32 */
#else
                    carFreezeSetState(e->type, 1);
#endif
                } else {
#ifdef TM2_PORT_BALANCE
                    e->stats.freeze_timer = portBalanceFreeze(e, 0, FREEZE_TICKS);
#else
                    e->stats.freeze_timer = FREEZE_TICKS;
#endif
                }
            }
            if (type == 15 || type == 47) {
                if (e->state.burning == 0 && e->state.in_water == 0 && e->state.in_fire == 0) {
                    e->state.burning = 1;
                    e->weapons.fire_effect = fxFlameBurning(e->motion.pos, 1);
                }
                e->stats.burn_time = TM2_SPECIAL_BURN_START(owner);
            }
            if (type == 20)
                carSpinOut(e, 0, 0, 1);
            return obj->id;
        }
    }
    if (owner < 50) {
        b = targetPoolGet();
        for (i = 0; i < 50; i++) {
            if (b[i].health > 0 && b[i].a != -owner && DISTANCE(pos, b[i].position) < radius + 1)
                return i + 500;
        }
    }
    s = targetActorsGet();
    for (i = 0; i < 6; i++) {
        if (s[i].state != -1 && s[i].kind != 2
            && DISTANCE(pos, s[i].object->pos) < radius + 1)
            return i + 6000;
    }
    return 0;
}

extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern s32 slamClawGetAngle(void);
extern s32 slamArmGetAngle(void);
/* Retail re-sign-extends the result in this unit (as with an s16 return);
 * timerGetFrameTicks returns int (timer_frame.c), and other units use it so. */
extern s16 timerGetFrameTicks(void);
extern void mathBoundAngle(s32 *);
#include "semantic/weapon_calls.h"
extern void mathMulTransVec(void *, s32 *, s32 *);
extern s32 targetDamage(s32, s32, s32);
extern void targetActorHit(s32, void *);
extern void weaponSlamRelease(void *, u8);
struct G16Triple { s32 v[3]; };
extern struct G16Triple weaponSlamTargetHitVec;

/* Pad feedback command; the pad is only polled for the side-0 player. */
static __inline__ void g16_feedback(struct TM2CarMotion *motion, s32 command, u8 side)
{
    s32 a, b;

    if (side) {
        soundPlay(command, 0, 0, 99);
    } else {
        if (viewGetSplitMode() == 0)
            soundLocateForView0(motion->pos);
        else
            soundLocateForView(motion->pos, -1);
        a = PadChkMtap();
        b = soundGetListenerPan();
        soundPlay(command, a, b, 99);
    }
}

/* @800F4058: per-frame update of one side's staged swing/special sequence
 * (side != 0: state at +0x810, else +0x850). Drives the two arm angles
 * (getters 80115218/80115224, setters 80115230/8011526C); when the swing
 * reaches 911 it smashes breakables and kicks spinners within 15*8 of the
 * strike point. */
void __attribute__((section(".text_800F4058"))) weaponSlamSwingUpdate(void *car, u8 side)
{
    struct TM2CarSlam *state;
    struct TM2CarStatsBlock *geometry;
    struct TM2CarMotion *motion;
    struct TM2CarInput *input;
    struct TM2CarState *flags;
    s32 id;
    s32 i;
    s32 local[3];
    s32 pos[3];
    s32 angle;
    struct G16Breakable *b;
    struct G16Spinner *s;

    if (side) {
        state = HREC(car)->weapons.slam;
        geometry = &HREC(car)->stats;
        motion = &HREC(car)->motion;
        input = &HREC(car)->input;
        flags = &HREC(car)->state;
        id = (s16)*(u16 *)&HREC(car)->slot;
    } else {
        state = AI(car)->weapons.slam;
        geometry = &AI(car)->stats;
        motion = &AI(car)->motion;
        input = &AI(car)->input;
        flags = &AI(car)->state;
        id = (s16)*(u16 *)&AI(car)->slot;
    }
    if (state->swing == 0)
        return;
    if (flags->frozen != 0) {
        if (state->spin_pending != 0)
            weaponSlamCarryTarget(state->spin_target, geometry, motion, 0, 0);
        return;
    }
    if (state->f03 != 0) {
        TM2_VOICE_TAG(id, TM2_VOICE_SPECIAL);    /* request 87: Mr. Slam's special */
        g16_feedback(motion, 57, TM2_CAR_FEEDBACK(side, HREC(car)->player));   /* H3 */
        TM2_VOICE_UNTAG();
        angle = slamClawGetAngle();
        if (angle >= -681) {
            angle -= 34 * timerGetFrameTicks();
            mathBoundAngle(&angle);
            if (angle < -682)
                angle = -682;
            slamClawSetAngle(angle);
            angle = slamArmGetAngle();
            angle -= 5 * timerGetFrameTicks();
            mathBoundAngle(&angle);
            slamArmSetAngle(angle);
            return;
        }
        state->f03 = 0;
        state->f05 = 1;
        return;
    }
    if (state->f05 != 0) {
        if (input->fire_held == 0 || (S16(state, 16) += timerGetFrameTicks(), S16(state, 16) >= 601)) {
            state->f05 = 0;
            state->f04 = 1;
        }
        return;
    }
    if (state->f04 != 0) {
        angle = slamArmGetAngle();
        angle += 79 * timerGetFrameTicks();
        mathBoundAngle(&angle);
        if (angle >= 911) {
            state->f04 = 0;
            angle = 910;
            if (state->spin_pending != 0) {
                carSpinOutByObject(state->spin_target, 1, 1);
                if (state->half_damage != 0)
                    carDamageByObject(state->spin_target, id, 6);
                else
                    carDamageByObject(state->spin_target, id, 12);
#ifdef TM2_PORT_CAR_GROUPS
                carSpinOutByObject(TM2_CAR_KEY(id, 192), 1, 1);   /* request 32: the slammer's key */
#else
                carSpinOutByObject(192, 1, 1);
#endif
            } else {
                g16_feedback(motion, 5, TM2_CAR_FEEDBACK(side, HREC(car)->player));
#ifdef TM2_PORT_CAR_GROUPS
                carSpinOutByObject(TM2_CAR_KEY(id, 192), 1, 0);   /* request 32 */
#else
                carSpinOutByObject(192, 1, 0);
#endif
            }
            local[0] = 0;
            local[1] = geometry->axle_front + 120;
            local[2] = 0;
            mathMulTransVec(&motion->rot_m, local, pos);
            pos[0] += motion->pos[0];
            pos[1] += motion->pos[1];
            pos[2] += motion->pos[2];

            b = targetPoolGet();
            for (i = 0; i < 50; i++) {
                if (b[i].health > 0 && DISTANCE((s32 *)pos, b[i].position) < 15)
                    targetDamage(b[i].a, b[i].b, 25);
            }
            s = targetActorsGet();
            for (i = 0; i < 6; i++) {
                if (s[i].state != -1 && DISTANCE((s32 *)pos, s[i].object->pos) < 15) {
                    struct G16Triple rest;

                    rest = weaponSlamTargetHitVec;
                    targetActorHit(i, &rest);
                }
            }
        } else if (angle >= 569) {
            if (state->spin_pending != 0)
                weaponSlamCarryTarget(state->spin_target, geometry, motion, 0, 0);
            else if (weaponSlamFindTarget(car, side)) {
                state->f04 = 0;
                carSpinOutByObject(state->spin_target, 1, 1);
                if (state->half_damage != 0)
                    carDamageByObject(state->spin_target, id, 12);
                else
                    carDamageByObject(state->spin_target, id, 25);
#ifdef TM2_PORT_CAR_GROUPS
                carSpinOutByObject(TM2_CAR_KEY(id, 192), 1, 1);   /* request 32: the slammer's key */
#else
                carSpinOutByObject(192, 1, 1);
#endif
            }
        }
        if (state->spin_pending != 0)
            carSpinOutByObject(state->spin_target, 0, 1);
        slamArmSetAngle(angle);
        return;
    }
    angle = slamClawGetAngle();
    if (angle < 739) {
        angle += 45 * timerGetFrameTicks();
        mathBoundAngle(&angle);
    }
    slamClawSetAngle(angle);
    angle = slamArmGetAngle();
    if (angle >= -112) {
        angle -= 34 * timerGetFrameTicks();
        mathBoundAngle(&angle);
    } else if (angle >= 569) {          /* unreachable (angle < -112); kept for codegen */
        if (state->spin_pending != 0)
            carSpinOutByObject(state->spin_target, 0, 1);
    } else if (state->spin_pending == 0 && weaponSlamFindTarget(car, side)) {
        carSpinOutByObject(state->spin_target, 0, 1);
    }
    slamArmSetAngle(angle);
    if (slamClawGetAngle() >= 739 && slamArmGetAngle() < -112) {
        angle = 739;
        slamClawSetAngle(angle);
        angle = -113;
        slamArmSetAngle(angle);
        weaponSlamRelease(car, side);
    }
}
