#include "semantic/weapons.h"

typedef struct { int vx, vy, vz, pad; } VECTOR;

/* @80180B4C..B58: the controller's short scalars (each its own $gp word). */
extern s16 aiDarkToothShotDelay;      /* frames until the next shot */
extern s16 aiDarkToothShotTimer;      /* shot counter */
extern s16 aiDarkToothFireSide;      /* which side fires next */
extern s16 aiDarkToothSwoopTimer;      /* swoop timer */
extern s16 aiDarkToothYawStep;      /* yaw step */
extern s16 aiDarkToothPitchStep;      /* pitch step, bounced between +-100 */
extern s16 aiDarkToothSwoopTarget;      /* swoop target car */
/* @80180F18..F24: swoop state. */
extern s16 darkToothSwoopDelay;      /* frames until the next swoop */
extern s16 darkToothSwooping;      /* swooping */
extern s32 darkToothHoverHeight;      /* hover height */
extern VECTOR *darkToothSwoopTarget;     /* swoop target position */
extern struct TM2SceneObject *darkToothBossObject;      /* the controlled object */
extern s32 darkToothSwoopVectors[6];   /* last position, then the swoop push */

extern void mathMulTransVec(void *, void *, void *);
extern void mathMulMatVec(void *, void *, void *);
extern void *RotMatrixYXZ(void *, void *);
extern int rand(void);
extern int rsin(int);
extern int ratan2(int, int);
#include "semantic/sound.h"
#if defined(TM2_PORT_CAR_SOUND) && defined(TM2_PORT_DARK_TOOTH)
#include "semantic/car_groups.h"
#include "semantic/car_sound.h"   /* request 33-3 (CAR_GROUPS.md 12): each Dark Tooth's own swoop sounds */
#define DT_SWOOP_SOUND(id) portCarSoundPlay((id), 0, 0, 99, boss->slot)
#else
#define DT_SWOOP_SOUND(id) soundPlay((id), 0, 0, 99)
#endif
#include "semantic/port_dt_swoop.h"   /* request 121 item 1: the swoop at his own target */
extern void weaponFindTargetAhead(int, s32 **, s32 mode);
extern void weaponFindTargetBehind(int, s32 **, s32 mode);
extern struct TM2SceneObject *aiCarObject(int);
extern struct TM2SceneObject *humanCarObject(int);
extern s32 weaponFireDarkToothMissile(int, s32 *, void *, void *, int);
extern int humanCarNearest(VECTOR *, int);
extern int mathBlendValue(int, int, int);
extern int mathBlendAngle(int, int, int);
extern void fxExplosion48(VECTOR *, u8 flag);
extern void carApplyDamage(int, int, s32 *, int, int);
extern void *fxFlameBurning(VECTOR *, int);
extern void fxSmokeA12(VECTOR *, u8 arg1);
extern void fxExplosion24(VECTOR *, u8 flag);
extern int *hdpQueryPoint(int, int, VECTOR *, int);

#define setVector(v, x, y, z) ((v)->vx = (x), (v)->vy = (y), (v)->vz = (z))
#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)->vx - (r)->vx), MAX(ABS((l)->vy - (r)->vy), ABS((l)->vz - (r)->vz))) >> 3)
#define POS(p) ((VECTOR *)(p)->motion.pos)
#define OBJ_POS ((VECTOR *)darkToothBossObject->pos)
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"
/* request 125 (portBalance2 bit 1, BALANCE.md 26.4): the swoop's world test passes pickups' boxes */
#define SWOOP_WORLD(r) portBalanceSwoopHit(r)
#else
#define SWOOP_WORLD(r) (*(r))
#endif

/* @8010FD44: the boss controller. It fires from alternating sides on a random
 * interval; every so often it swoops at the nearest car and shoves it, and
 * otherwise hovers over the boss vehicle, swaying. */
int __attribute__((section(".text_8010FD44"))) aiDarkToothBossUpdate(struct TM2AIRecord *boss)
{
    VECTOR out, offset, hover, launch;
    s32 *shot;
    int value;

    if (boss->state.frozen)
        darkToothSwoopDelay = 100;
    if (aiDarkToothShotDelay < aiDarkToothShotTimer++ && !boss->state.frozen) {
        aiDarkToothShotTimer = 0;
        aiDarkToothShotDelay = rand() / 500 + 10;
        aiDarkToothFireSide ^= 1;
        if (boss->f0D4 == 1) {
            if (!aiDarkToothFireSide)
                setVector(&offset, 72, -224, 56);
            else
                setVector(&offset, -72, -224, 56);
            mathMulTransVec(&boss->motion.rot_m, &offset, &out);
            out.vx += boss->motion.pos[0];
            out.vy += boss->motion.pos[1];
            out.vz += boss->motion.pos[2];
            weaponFindTargetBehind(boss->index, &shot, 0);
        } else {
            if (!aiDarkToothFireSide)
                setVector(&offset, 120, 272, 104);
            else
                setVector(&offset, -120, 272, 104);
            mathMulTransVec(&boss->motion.rot_m, &offset, &out);
            out.vx += boss->motion.pos[0];
            out.vy += boss->motion.pos[1];
            out.vz += boss->motion.pos[2];
            weaponFindTargetAhead(boss->index, &shot, 0);
        }
        launch.vx = 170;
        launch.vy = 0;
        launch.vz = (boss->motion.rot[2] + boss->target_angle) & 4095;
        weaponFireDarkToothMissile(aiCarObject(boss->index)->id, shot, &launch, &out, 5);
    }
    darkToothSwoopVectors[0] = darkToothBossObject->pos[0];
    darkToothSwoopVectors[1] = darkToothBossObject->pos[1];
    darkToothSwoopVectors[2] = darkToothBossObject->pos[2];
    if (aiBossDefeated() & 255)
        darkToothHoverHeight = -24;
    else
        darkToothHoverHeight = 240;
    if (aiDarkToothSwoopTimer == darkToothSwoopDelay - 30)
        DT_SWOOP_SOUND(61);
    if (darkToothSwoopDelay < aiDarkToothSwoopTimer++ && !boss->state.frozen) {
#ifdef TM2_PORT_DT_SWOOP
        /* request 121 item 1: the car he is after (a computer car), or the nearest player as retail */
        aiDarkToothSwoopTarget = portDtSwoopPick(boss);
        darkToothSwoopTarget = (VECTOR *)portDtSwoopPos(aiDarkToothSwoopTarget);
#else
        aiDarkToothSwoopTarget = humanCarNearest(POS(boss), -1);
        darkToothSwoopTarget = (VECTOR *)humanCarGet(aiDarkToothSwoopTarget)->motion.pos;
#endif
        if (DISTANCE(POS(boss), darkToothSwoopTarget) < 500) {
            aiDarkToothSwoopTimer = 0;
            value = rand();
            darkToothSwooping = 1;
            darkToothSwoopDelay = value / 327 + 100;
            DT_SWOOP_SOUND(66);
        }
    }
#ifdef TM2_PORT_DT_SWOOP
    if (darkToothSwooping && !portDtSwoopLive(aiDarkToothSwoopTarget))
        darkToothSwooping = 0;      /* request 121 item 1: the computer car he swooped at left the match */
#endif
    if (!darkToothSwooping) {
        offset.vx = 0;
        offset.vy = 0;
        offset.vz = darkToothHoverHeight;
        mathMulMatVec(&boss->motion.rot_m, &offset, &hover);
        hover.vx += boss->motion.pos[0];
        hover.vy += boss->motion.pos[1];
        hover.vz += boss->motion.pos[2];
        darkToothBossObject->pos[0] = mathBlendValue(darkToothBossObject->pos[0], hover.vx, 40);
        darkToothBossObject->pos[1] = mathBlendValue(darkToothBossObject->pos[1], hover.vy, 40);
        darkToothBossObject->pos[2] = mathBlendValue(darkToothBossObject->pos[2], hover.vz, 40);
        if (!boss->state.frozen) {
            darkToothBossObject->rot[1] += aiDarkToothPitchStep;
            if (darkToothBossObject->rot[1] >= 683)
                aiDarkToothPitchStep = -100;
            if (darkToothBossObject->rot[1] < -682)
                aiDarkToothPitchStep = 100;
            darkToothBossObject->rot[2] += aiDarkToothYawStep;
            RotMatrixYXZ(darkToothBossObject->rot, &darkToothBossObject->matrix);
        }
    } else {
        darkToothBossObject->pos[0] = mathBlendValue(darkToothBossObject->pos[0], darkToothSwoopTarget->vx, 70);
        darkToothBossObject->pos[1] = mathBlendValue(darkToothBossObject->pos[1], darkToothSwoopTarget->vy, 70);
        darkToothBossObject->pos[2] = mathBlendValue(darkToothBossObject->pos[2], darkToothSwoopTarget->vz, 70);
        darkToothBossObject->pos[2] += rsin(aiDarkToothSwoopTimer * 200) * 160 / 4095;
        offset.vx = darkToothBossObject->pos[0] - darkToothSwoopTarget->vx;
        offset.vy = darkToothBossObject->pos[1] - darkToothSwoopTarget->vy;
        if (offset.vy == 0)
            offset.vy = 1;
        darkToothBossObject->rot[2] = mathBlendAngle(darkToothBossObject->rot[2], ratan2(offset.vx, offset.vy), 60);
        RotMatrixYXZ(darkToothBossObject->rot, &darkToothBossObject->matrix);
        if (DISTANCE(OBJ_POS, darkToothSwoopTarget) < 20) {
            fxExplosion48(OBJ_POS, 0);
            darkToothSwoopVectors[3] = darkToothBossObject->pos[0] - darkToothSwoopVectors[0];
            darkToothSwoopVectors[4] = darkToothBossObject->pos[1] - darkToothSwoopVectors[1];
            darkToothSwoopVectors[5] = 0;
#ifdef TM2_PORT_DT_SWOOP
            portDtSwoopHit(aiDarkToothSwoopTarget, boss->slot);     /* request 121 item 1: on a player or a computer car */
#else
            carApplyDamage(humanCarObject(aiDarkToothSwoopTarget)->id, 7, &darkToothSwoopVectors[3], 0, boss->slot);
            if (!humanCarGet(aiDarkToothSwoopTarget)->state.burning && !humanCarGet(aiDarkToothSwoopTarget)->state.energy_shield
                && !humanCarGet(aiDarkToothSwoopTarget)->state.in_water && !humanCarGet(aiDarkToothSwoopTarget)->state.in_fire) {
                humanCarGet(aiDarkToothSwoopTarget)->state.burning = 1;
                humanCarGet(aiDarkToothSwoopTarget)->stats.burn_time = 0;
                humanCarGet(aiDarkToothSwoopTarget)->weapons.fire_effect = fxFlameBurning(darkToothSwoopTarget, 1);
                fxSmokeA12(darkToothSwoopTarget, 0);
                fxExplosion24(darkToothSwoopTarget, 0);
                soundPlay(6, 0, 0, 99);
            }
#endif
            darkToothSwooping = 0;
        } else if (SWOOP_WORLD(hdpQueryPoint(darkToothBossObject->id, -1, OBJ_POS, 0))) {
            fxExplosion48(OBJ_POS, 0);
            darkToothSwooping = 0;
        }
    }
    if (DISTANCE(POS(boss), darkToothSwoopTarget) >= 500 || aiDarkToothSwoopTimer >= 41)
        darkToothSwooping = 0;
}
