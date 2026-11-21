#include "semantic/cars.h"   /* u8..u32, TM2Object, TM2HumanRecord / TM2AIRecord */
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H3: local feedback (retail's calls without the define) */

/* @80190E54: target actor records (60 bytes). */
struct TM2TargetRecord {
    signed char row;        /* start row */
    u8 leg;                 /* which half of the row is the current goal */
    u8 state;               /* 0 roaming, 1 thrown, 2 settling */
    u8 pad3;
    s16 kind;               /* +4: 0x202/0x205 are the moving kinds */
    s16 sub;                /* +6 */
    s16 timer;              /* +8 */
    s16 speed;              /* +10 */
    s32 goal[3];            /* +12 */
    u8 matrix[32];          /* +24: copy of the hitting car's matrix */
    struct TM2Object *object;   /* +56 */
};
typedef char tm2_target_record_size[(sizeof(struct TM2TargetRecord) == 60) ? 1 : -1];
extern struct TM2TargetRecord targetActors[6];

/* @80163DB8: start tables (see target_actor_init.c); the moving kinds walk
 * between the two halves of their 6-word rows. */
struct TM2StartRow { s32 x, y, z; };
struct TM2StartPair { struct TM2StartRow at[2]; };
struct TM2StartTables {
    struct TM2StartPair mode3_special[4];
    struct TM2StartRow mode3[4];
    struct TM2StartPair mode1_special[3];
    struct TM2StartRow mode1[5];
    struct TM2StartPair mode6_special[2];
    struct TM2StartRow mode6[5];
    struct TM2StartRow mode5_single[2];
    struct TM2StartRow mode5[5];
    struct TM2StartPair mode8_special[5];
    struct TM2StartRow mode8[10];
    struct TM2StartRow mode11[8];
};
extern struct TM2StartTables targetStartPositions;

extern s32 targetActorCount;
extern s32 targetActorRunOverCount;
extern s32 worldGroundHeightAt(s32 *);
extern void mathMulTransVec(void *, s32 *, s32 *);
extern s32 ratan2(s32, s32);
extern void *RotMatrixYXZ(void *, void *);
extern s32 gameGetLevel(void);
extern s32 humanCarCount(void);
extern s32 aiCarCount(void);
extern void humanCarGetPos(s32, s32 *);
extern void aiCarGetPos(s32, s32 *);
extern struct TM2Object *humanCarObject(s32);
extern struct TM2Object *aiCarObject(s32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void hudMessagePost(s32, s32);
extern void fxSplashAt(void *, s32);
extern void fxTargetHitAt(void *);
extern void objRegistrySetStep(s32 id, s32 index, s32);
extern void targetActorRespawn(s32);
extern void *memcpy(void *, const void *, unsigned int);

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)
#define REC (*record)
#define POS (REC.object->pos)
#define GOAL(table) \
    REC.goal[0] = (table)[REC.row].at[(signed char)REC.leg].x; \
    REC.goal[1] = (table)[REC.row].at[(signed char)REC.leg].y; \
    REC.goal[2] = (table)[REC.row].at[(signed char)REC.leg].z
#define RUMBLE \
    if (REC.kind != 0x205) { \
        if (viewGetSplitMode() == 0) \
            soundLocateForView0(POS); \
        else \
            soundLocateForView(POS, -1); \
        soundPlay(25, PadChkMtap() * 2, soundGetListenerPan(), 50); \
    }

/* The mode switch's jump table sits at its retail address. */
#pragma tm2_rdata .tm2_800CF5F8

/* @80126464: move the target actors: the moving kinds walk between the two
 * goals of their start row; a car (or AI car) touching an actor throws it
 * along the car's heading; thrown actors settle and are removed later. */
void __attribute__((section(".text_80126464"))) targetActorsUpdate(void)
{
    struct TM2TargetRecord *record;
    s32 actor, i, dx, dy;
    s32 delta[3];
    s32 step[3];

    for (actor = 0; actor < targetActorCount; actor++) {
        record = &targetActors[actor];
        if (POS[2] - worldGroundHeightAt(POS) >= 201)
            objRegistrySetStep(REC.kind, REC.sub, 0);

        if ((REC.kind == 0x202 || REC.kind == 0x205) && REC.state == 0) {
            step[0] = 0;
            step[1] = 7;
            step[2] = 0;
            mathMulTransVec(REC.object->matrix, step, delta);
            POS[0] += delta[0];
            POS[1] += delta[1];
            POS[2] += delta[2];
            dx = REC.goal[0] - POS[0];
            dy = REC.goal[1] - POS[1];
            if (dy == 0)
                dy = 1;
            REC.object->rot[2] = ratan2(dx, dy);
            RotMatrixYXZ(REC.object->rot, REC.object->matrix);
            if (ABS(POS[0] - REC.goal[0]) < 80 && ABS(POS[1] - REC.goal[1]) < 80) {
                REC.leg ^= 1;
                switch (gameGetLevel()) {
                case 3: GOAL(targetStartPositions.mode3_special); break;
                case 1: GOAL(targetStartPositions.mode1_special); break;
                case 6: GOAL(targetStartPositions.mode6_special); break;
                case 8: GOAL(targetStartPositions.mode8_special); break;
                case 2:
                case 4:
                    break;
                default:
                    return;   /* other modes end the whole update, as in retail */
                }
                REC.object->rot[2] += 0x800;
            }
        }

        if (REC.state == 0) {
            for (i = 0; i < humanCarCount(); i++) {
                humanCarGetPos(i, delta);
                if (DISTANCE((s32 *)delta, POS) < 10) {
                    RUMBLE
                    TM2_HUMAN_SOUND(i, 3, 0, 0, 45);   /* H3: human i's own cue */
                    REC.state = 1;
                    REC.timer = 0;
                    REC.speed = humanCarGet(i)->motion.speed / 32;
                    memcpy(REC.matrix, humanCarObject(i)->matrix, 32);
                    objRegistrySetStep(REC.kind, REC.sub, 2);
                    POS[2] += 40;
                    fxTargetHitAt(POS);
                    POS[2] -= 40;
                    if (++targetActorRunOverCount == 10) {
                        struct TM2HumanRecord *vehicle = humanCarGet(i);
                        s32 add = vehicle->weapons.capacity - vehicle->weapons.held;
                        vehicle->weapons.ammo[2] = vehicle->weapons.ammo[2] + (add >= 6 ? 5 : add);
                        vehicle->stats.hp += vehicle->stats.max_hp / 3;
                        vehicle->stats.hp = MIN(vehicle->stats.hp, vehicle->stats.max_hp);
                        TM2_HUMAN_MESSAGE(i, 34, 0);
                        TM2_HUMAN_SOUND(i, 24, 0, 0, 65);
                    }
                }
            }
            for (i = 0; i < aiCarCount(); i++) {
                aiCarGetPos(i, delta);
                if (DISTANCE((s32 *)delta, POS) < 10) {
                    RUMBLE
                    REC.state = 1;
                    REC.timer = 0;
                    REC.speed = aiCarGet(i)->motion.speed / 32;
                    memcpy(REC.matrix, aiCarObject(i)->matrix, 32);
                    objRegistrySetStep(REC.kind, REC.sub, 2);
                    POS[2] += 40;
                    fxTargetHitAt(POS);
                    POS[2] -= 40;
                }
            }
        }

        if ((signed char)REC.state == 1) {
            if (gameGetLevel() == 5 && REC.sub == 1) {
                /* stored through pointers, as in retail (kept in order) */
                ((s32 *)delta)[0] = 0;
                ((s32 *)delta)[1] = 0;
                ((s32 *)delta)[2] = 0;
            } else {
                ((s32 *)step)[0] = 0;
                ((s32 *)step)[1] = REC.speed + 5;
                ((s32 *)step)[2] = 0;
                mathMulTransVec(REC.matrix, step, delta);
            }
            POS[0] += delta[0];
            POS[1] += delta[1];
            POS[2] += delta[2];
            if (gameGetLevel() == 5 && REC.sub == 1 && REC.timer == 10)
                fxSplashAt(&targetStartPositions.mode5_single[1], 0);
            if (REC.timer++ >= 41)
                REC.state = 2;
        }
        if ((signed char)REC.state == 2) {
            if (REC.timer++ >= 301)
                targetActorRespawn(actor);
        }
    }
}
