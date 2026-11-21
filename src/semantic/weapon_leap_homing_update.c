typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;

/* @80184DC0: homing projectile state (the shield block's tail fields). */
typedef struct TM2HomingState {
    u8 pad0[0x14];
    s8 active;          /* +0x14 */
    u8 pad15[3];
    u8 *cancel;         /* +0x18: nonzero byte cancels */
    u8 pad1C[4];
    s16 source;         /* +0x20 */
    s16 age;            /* +0x22 */
    s16 owner;          /* +0x24 */
    s16 pad26;
    s32 speed_target;   /* +0x28 */
    s32 *clock;         /* +0x2C */
    s32 start;          /* +0x30 */
    s32 velocity[3];    /* +0x34 (velocity[1] = speed) */
    s32 *projectile;    /* +0x40 */
    s32 *aim;           /* +0x44 */
    s32 *target;        /* +0x48 */
} TM2HomingState;
extern TM2HomingState weaponTornadoState;
#define H weaponTornadoState
#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)

extern s32 sub_8013FC44(s32 squared_distance);
extern s32 ratan2(s32 y,s32 x);
extern s32 mathBlendAngle(s32 current,s32 target,s32 rate);
extern s32 mathBlendValue(s32 current,s32 target,s32 rate);
extern void *RotMatrixYXZ(const s16 *rotation, void *matrix);
extern void mathMulTransVec(const void *matrix,const void *input,s32 *output);
extern void objRegistrySetStep(s32 id, s32 index, s32 value);
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
/* request 32 (CAR_GROUPS.md): the turbo flame of the leaping car (0x66 = Grasshopper 0x60 + 6) */
#ifdef TM2_PORT_CAR_GROUPS
#define LEAP_TURBO(v) portCarTurboSetStep(TM2_CAR_KEY(H.source, 0x60), (v))
#else
#define LEAP_TURBO(v) objRegistrySetStep(0x66, 0, (v))
#endif
extern void fxSmokeC6(void *object, u8 value);
extern void fxExplosion24(void *object, u8 value);
extern void fxSmokeA24(void *object,s32 value);
extern void carApplyDamage(s32 owner, s32 kind, s32 *vector, s32 count, s32 source);
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif

/* @8010F520. Advance the active homing projectile and retire it on impact,
 * timeout or external cancellation. The early-impact vector is intentionally
 * not initialized before carApplyDamage, matching the retail source path. */
#ifdef TM2_PORT_CAR_SPECIALS
/* request 33-2a (CAR_GROUPS.md 10): retail's body, run by weaponHomingLeapUpdate below once per Grasshopper */
static void __attribute__((section(".port_text"))) weaponHomingLeapUpdateOne(void)
#else
void __attribute__((section(".text_8010F520")))
weaponHomingLeapUpdate(void)
#endif
{
    s32 *projectile;
    s32 transformed[3];
    s16 rotation[4];
    u8 matrix[32];
    s32 dx, dy, dz, horizontal;
    s32 yaw, pitch;
#ifdef TM2_PORT_BALANCE
    s32 touched;
#endif

    if (H.active == 0)
        return;
    dx = H.target[0] - H.projectile[0];
    dy = H.target[1] - H.projectile[1];
    dz = H.target[2] - H.projectile[2];
    horizontal = sub_8013FC44(dx * dx + dy * dy);
    if (dy == 0)
        dy = 1;
    if (horizontal == 0)
        horizontal = 1;
    yaw = ratan2(dx, dy);
    pitch = -ratan2(dz, horizontal);
    H.aim[2] = mathBlendAngle(H.aim[2], yaw, 75);

    if (H.age++ >= 8) {
        H.velocity[1] = mathBlendValue(H.velocity[1], H.speed_target, 90);
        rotation[0] = pitch;
        rotation[1] = 0;
        rotation[2] = yaw;
        RotMatrixYXZ(rotation, matrix);
        mathMulTransVec(matrix, H.velocity, transformed);
        H.projectile[0] += transformed[0];
        H.projectile[1] += transformed[1];
        projectile = H.projectile;   /* one load for the last step and the call */
        projectile[2] += transformed[2];
        LEAP_TURBO(1);
        fxSmokeC6(H.projectile, 0);
    }

#ifdef TM2_PORT_BALANCE
    /* request 40 bit 7 (BALANCE.md 3.8): her body touching the target counts as the hit (retail cancels the leap);
       the hit does 18; bit 13: she lands on the ground */
    touched = 0;
    if (DISTANCE(H.projectile, H.target) >= 15 && *H.cancel != 0 && portBalanceLeapTouch())
        touched = 1;
    if (touched || DISTANCE(H.projectile, H.target) < 15) {
        H.active = 0;
        fxExplosion24(H.target, 0);
        fxSmokeA24(H.target, 0);
        carApplyDamage(H.owner, portBalanceLeapDamage(15), transformed, 2, H.source);
        portBalanceLeapPush();      /* request 103 (bit 31): and the push */
        LEAP_TURBO(0);
        portBalanceLeapLand();
    }
#else
    if (DISTANCE(H.projectile, H.target) < 15) {
        H.active = 0;
        fxExplosion24(H.target, 0);
        fxSmokeA24(H.target, 0);
        carApplyDamage(H.owner, 15, transformed, 2, H.source);
        LEAP_TURBO(0);
    }
#endif

    if (H.age >= 101 || *H.cancel != 0 || H.start - *H.clock >= 6) {
        H.active = 0;
        LEAP_TURBO(0);
#ifdef TM2_PORT_BALANCE
        if (*H.cancel != 0 && !touched) {   /* request 40 bit 7: no cancel explosion after a touch hit */
#else
        if (*H.cancel != 0) {
#endif
            fxExplosion24(H.projectile, 0);
            fxSmokeA24(H.projectile, 0);
        }
    }
}

#ifdef TM2_PORT_CAR_SPECIALS
/* @8010F520 under TM2_PORT_CAR_SPECIALS (PER_CAR_PLAN.md D6): the first Grasshopper's leap, then each
 * duplicate Grasshopper's in key order. */
void __attribute__((section(".text_8010F520")))
weaponHomingLeapUpdate(void)
{
    int d = 0, t;

    weaponHomingLeapUpdateOne();
    while ((t = portCarSpecialNext(0x60, &d)) != 0) {
        weaponHomingLeapUpdateOne();
        portCarSpecialRestore(t);
    }
}
#endif
