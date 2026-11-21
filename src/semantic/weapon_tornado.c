#include "semantic/cars.h"
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md 3.7) */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

struct TM2Actor {
    u8 pad_0[8];
    s32 active;
    u8 pad_C[0x74];
    struct TM2CarMatrix matrix;     /* +0x80 */
    s16 rot[3];             /* +0xA0 */
    u8 pad_A6[2];
    s32 position[3];
    u8 pad_B4[0x64];
    s32 identity;
};

struct TM2ShieldState {
    s16 timer;
    s16 field_2;
    s16 angle;
    s16 field_6;
    s32 *anchor;
    struct TM2CarState *flags;
    struct TM2Actor *object;
    u8 field_14;
    u8 field_15;
    s16 field_16;
    s32 rest[13];
};

struct TM2NearRecord {
    s16 field_0;
    s16 field_2;
    s16 active;
    s16 field_6;
    s32 position[3];
};

struct TM2ObjectRecord {
    u8 pad_0[4];
    s16 state;
    u8 pad_6[0x32];
    struct TM2Actor *actor;
};

extern struct TM2ShieldState weaponTornadoState;
#pragma tm2_absolute weaponTornadoOwner
extern struct TM2Actor *weaponTornadoOwner;   /* = weaponTornadoState.object */
extern struct TM2Actor *weaponTornadoObject;
extern struct TM2Actor *weaponTornadoFunnelObject;
struct TM2Triple { s32 v[3]; };
extern struct TM2Triple weaponHitSpinDefault;

void *weaponTornadoFxPos __attribute__((section(".tm2_80180F14"))) = 0;
s32 tornadoPushVector[3] __attribute__((section(".tm2_80183F38"))) = {0, 0, 0};
u8 tornadoRotMatrix[32] __attribute__((section(".tm2_80183F44"))) = {0};
s16 weaponTornadoRotation[4] __attribute__((section(".tm2_80180B44"))) = {0, 0, -341, 0};

extern s32 humanCarCount(void);
extern s32 aiCarCount(void);
extern struct TM2Actor *humanCarObject(s32 index);
extern struct TM2Actor *aiCarObject(s32 index);
extern void *memcpy(void *destination, const void *source, s32 size);
extern s32 mathBlendValue(s32 current, s32 target, s32 rate);
extern s32 mathBlendAngle(s32 current, s32 target, s32 rate);
extern void *RotMatrixYXZ(const void *rotation, void *matrix);
extern s32 rand(void);
extern s32 mathJitterPos(void *position, s32 x, s32 y, s32 z);
extern void fxSmokeC12(s32 object, s32 value);
#include "semantic/fx.h"
#include "semantic/weapon_calls.h"
extern void fxRandomDebrisBurst(void *position);
extern void fxSmokeB12(s32 object, s32 value);
extern void mathMulTransVec(const void *matrix, const s32 *input, s32 *output);
extern int ratan2(s32 y, s32 x);
extern struct TM2NearRecord *targetPoolGet(void);
extern s32 targetDamage(s32 a0, s32 a1, s32 kind);
extern struct TM2ObjectRecord *targetActorsGet(void);
extern void targetActorHit(s32 index, s32 *rest);
extern void carBodyObjectShow(s32 value);
extern void fxSmokeB24(void *position, s32 value);

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)

/* The shield owner is read three ways, as retail does: through its own
 * absolute word (weaponTornadoOwner), as a struct member (reloaded after each store),
 * and through a cast pointer (a volatile read whose address is hoisted). */
#define OWNER weaponTornadoOwner
#define OWNER_P (*(struct TM2Actor **)&weaponTornadoState.object)
static __inline__ s16 tm2_heading(s32 *x, s32 *y)
{
    if (*y == 0)
        *y = 1;
    return ratan2(*x, *y);
}


/* @8010E040. Advance and retire the active shield volume. */
#ifdef TM2_PORT_CAR_SPECIALS
/* request 33-2a (CAR_GROUPS.md 10): retail's body, run by weaponTornadoUpdate below once per Twister */
static void __attribute__((section(".port_text"))) weaponTornadoUpdateOne(void)
#else
void __attribute__((section(".text_8010E040")))
weaponTornadoUpdate(void)
#endif
{
    struct TM2Actor *actor;
    struct TM2NearRecord *near_record;
    struct TM2ObjectRecord *object_record;
    void *aux;              /* human or AI record */
    s32 output[3];
    struct TM2Triple rest;
    s32 actor_count = humanCarCount();
    s32 other_count = aiCarCount();
    s32 index;
    s32 roll;
    struct TM2Actor *shield;

    if (weaponTornadoState.timer <= 0)
        return;

    weaponTornadoState.timer--;
    memcpy(weaponTornadoObject->position, OWNER->position, 12);
    memcpy(weaponTornadoFunnelObject->position, OWNER->position, 12);
    weaponTornadoFunnelObject->position[2] = mathBlendValue(weaponTornadoFunnelObject->position[2],
                                           weaponTornadoFunnelObject->position[2] + 120, 90);
    weaponTornadoFunnelObject->rot[0] =
        mathBlendAngle(weaponTornadoFunnelObject->rot[0], 796, 95);
    weaponTornadoState.angle = (s16)mathBlendAngle(weaponTornadoState.angle, 682, 90);
    weaponTornadoFunnelObject->rot[2] -= weaponTornadoState.angle;
    RotMatrixYXZ(weaponTornadoFunnelObject->rot, &weaponTornadoFunnelObject->matrix);

    roll = rand();
    if (roll == roll / 10 * 10) {
        weaponTornadoFxPos = (void *)mathJitterPos(OWNER->position, 0x333, 0x333, 0x333);
        fxSmokeC12((s32)weaponTornadoFxPos, 0);
        fxSparkOrange((s32)weaponTornadoFxPos, 0);
    }

    for (index = 0; index < actor_count; index++) {
        actor = humanCarObject(index);
        aux = humanCarGet(index);
        if (DISTANCE(OWNER->position, HREC(aux)->motion.pos) < 35 &&
            OWNER_P != actor) {
            HREC(aux)->state.f16 = 1;
            shield = weaponTornadoObject;
            tornadoPushVector[0] = HREC(aux)->motion.pos[0] - shield->position[0];
            tornadoPushVector[1] = HREC(aux)->motion.pos[1] - shield->position[1];
            tornadoPushVector[2] = HREC(aux)->motion.pos[2] - shield->position[2];
            if (HREC(aux)->state.frozen == 0) {
                HREC(aux)->state.frozen = 1;
                HREC(aux)->stats.freeze_timer = 60;
                carSpinOut(aux, 1, 3, 1);
                weaponApplyHit(actor->identity, 0, 0, 4, tornadoPushVector, OWNER->identity);
            }
            fxRandomDebrisBurst(weaponTornadoObject->position);
            weaponTornadoFxPos = (void *)mathJitterPos(OWNER_P->position, 0x333, 0x333, 0x333);
            if (rand() & 1)
                fxSmokeB12((s32)weaponTornadoFxPos, 0);
            RotMatrixYXZ(weaponTornadoRotation, tornadoRotMatrix);
            mathMulTransVec(tornadoRotMatrix, tornadoPushVector, output);
            HREC(aux)->motion.pos[0] = weaponTornadoState.object->position[0] + output[0];
            HREC(aux)->motion.pos[1] = weaponTornadoState.object->position[1] + output[1];
            HREC(aux)->motion.pos[2] = weaponTornadoState.object->position[2] + output[2];
            HREC(aux)->motion.rot[0] -= 113;
            HREC(aux)->motion.rot[1] -= 113;
            HREC(aux)->motion.rot[2] -= 113;
            if (HREC(aux)->stats.freeze_timer == 1) {
#ifdef TM2_PORT_CAR_GROUPS
                carSpinOutById(TM2_CAR_KEY(HREC(aux)->slot, HREC(aux)->type),   /* request 32 */
                              tm2_heading(&tornadoPushVector[0], &tornadoPushVector[1]), 0);
#else
                carSpinOutById(HREC(aux)->type,
                              tm2_heading(&tornadoPushVector[0], &tornadoPushVector[1]), 0);
#endif
            }
        } else
            HREC(aux)->state.f16 = 0;
    }

    for (index = 0; index < other_count; index++) {
        actor = aiCarObject(index);
        aux = aiCarGet(index);
        if (DISTANCE(OWNER->position, AI(aux)->motion.pos) < 35 &&
            OWNER_P != actor) {
            shield = weaponTornadoObject;
            tornadoPushVector[0] = AI(aux)->motion.pos[0] - shield->position[0];
            tornadoPushVector[1] = AI(aux)->motion.pos[1] - shield->position[1];
            tornadoPushVector[2] = AI(aux)->motion.pos[2] - shield->position[2];
            if (AI(aux)->state.frozen == 0) {
                AI(aux)->state.frozen = 1;
                AI(aux)->stats.freeze_timer = 60;
                carSpinOut(aux, 0, 3, 1);
                weaponApplyHit(actor->identity, 0, 0, 4, tornadoPushVector, OWNER->identity);
            }
            fxRandomDebrisBurst(weaponTornadoObject->position);
            weaponTornadoFxPos = (void *)mathJitterPos(OWNER_P->position, 0x333, 0x333, 0x333);
            if (rand() & 1)
                fxSmokeB12((s32)weaponTornadoFxPos, 0);
            RotMatrixYXZ(weaponTornadoRotation, tornadoRotMatrix);
            mathMulTransVec(tornadoRotMatrix, tornadoPushVector, output);
            AI(aux)->motion.pos[0] = weaponTornadoState.object->position[0] + output[0];
            AI(aux)->motion.pos[1] = weaponTornadoState.object->position[1] + output[1];
            AI(aux)->motion.pos[2] = weaponTornadoState.object->position[2] + output[2];
            AI(aux)->motion.rot[0] -= 113;
            AI(aux)->motion.rot[1] -= 113;
            AI(aux)->motion.rot[2] -= 341;
            if (AI(aux)->stats.freeze_timer == 1) {
#ifdef TM2_PORT_CAR_GROUPS
                carSpinOutById(TM2_CAR_KEY(AI(aux)->slot, AI(aux)->type),   /* request 32 */
                              tm2_heading(&tornadoPushVector[0], &tornadoPushVector[1]), 0);
#else
                carSpinOutById(AI(aux)->type,
                              tm2_heading(&tornadoPushVector[0], &tornadoPushVector[1]), 0);
#endif
            }
        }
    }

    if (OWNER->identity < 50) {
        near_record = targetPoolGet();
        for (index = 0; index < 50; index++) {
            if (near_record[index].active > 0 &&
                DISTANCE(OWNER->position, near_record[index].position) < 35)
                targetDamage(near_record[index].field_0, near_record[index].field_2, 4);
        }
    }

    object_record = targetActorsGet();
    for (index = 0; index < 6; index++) {
        if (object_record[index].state != -1) {
            rest = weaponHitSpinDefault;
            if (DISTANCE(OWNER->position, object_record[index].actor->position) < 35)
                targetActorHit(index, rest.v);
        }
    }

    if (weaponTornadoState.timer == 0 || *weaponTornadoState.anchor <= 0 ||
        weaponTornadoState.flags->frozen != 0) {
        weaponTornadoState.timer = 0;
        weaponTornadoObject->active = 0;
        weaponTornadoFunnelObject->active = 0;
        if (*weaponTornadoState.anchor > 0)
            OWNER->active = 1;
        weaponTornadoObject->position[2] += 104;
#ifdef TM2_PORT_CAR_GROUPS
        /* request 32 (CAR_GROUPS.md): the Twister that fired (its object), else retail's 160 */
        index = portCarKeyOfObject(weaponTornadoState.object);
        if (!index)
            index = 160;
        if (*weaponTornadoState.anchor <= 0)
            carBodyObjectHide(index);
        else
            carBodyObjectShow(index);
#else
        if (*weaponTornadoState.anchor <= 0)
            carBodyObjectHide(160);
        else
            carBodyObjectShow(160);
#endif
        fxSmokeB24(weaponTornadoObject->position, 0);
#ifdef TM2_PORT_BALANCE
        /* request 40 bit 12 (BALANCE.md 3.7, option c): the tornado's last act throws out every car still in her
           catch box (retail's throw, which its freeze_timer == 1 tests above never reach: RETAIL_BUGS.md 21) */
        portBalanceTornadoThrow();
#endif
        for (index = 0; index < actor_count; index++)
            HREC(humanCarGet(index))->state.f16 = 0;
    }
}

#ifdef TM2_PORT_CAR_SPECIALS
/* @8010E040 under TM2_PORT_CAR_SPECIALS (PER_CAR_PLAN.md D6): the first Twister's tornado (retail's state,
 * retail's order), then each duplicate Twister's in key order. */
void __attribute__((section(".text_8010E040")))
weaponTornadoUpdate(void)
{
    int d = 0, t;

    weaponTornadoUpdateOne();
    while ((t = portCarSpecialNext(0xA0, &d)) != 0) {
        weaponTornadoUpdateOne();
        portCarSpecialRestore(t);
    }
}
#endif
