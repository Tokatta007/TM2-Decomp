#include "semantic/cars.h"

/* Motion objects: +128 matrix, +160 rotation, +164 heading, +168 position. */
typedef struct TM2MotionObject {
    u8 pad0[128];
    u8 matrix[32];         /* +128 */
    s16 rotation[3];       /* +160 (rotation[2] = +164) */
    s16 pad166;
    s32 position[3];       /* +168 */
} TM2MotionObject;
typedef struct TM2MotionRecord {
    u8 pad0[12];
    s32 busy;              /* +12 */
    s32 front[3];          /* +16 */
    s32 rear[3];           /* +28 */
    u8 pad40[12];
    TM2MotionObject *object;   /* +52 */
} TM2MotionRecord;
typedef char tm2_motion_record_size[(sizeof(TM2MotionRecord) == 56) ? 1 : -1];
extern TM2MotionRecord *trackVehicleRecords;
extern void mathMulTransVec(void *matrix, s32 *input, s32 *output);
extern s32 worldGroundHeightAt(s32 *position);
extern s32 sub_8013FC44(s32 value);
extern int ratan2(s32 y, s32 x);
extern void *RotMatrixYXZ(void *rotation, void *matrix);
extern s32 humanCarCount(void);
extern void trackVehicleSurfaceSet(s32 side, s32 *position, s32 height, s32 angle);

/* @80118BBC. Rebuild the two endpoints around record[index]'s motion object,
 * orient the object from their terrain heights, then emit one nearby-player
 * interaction at the transformed rear point. Void: $v0 is whatever the loop
 * test left there. */
#define REC trackVehicleRecords[index]
#define OBJ REC.object
void __attribute__((section(".text_80118BBC")))
trackVehicleGround(s32 index)
{
    struct TM2HumanRecord *actor;
    s32 local[3];
    s32 delta[3];
    s32 position[3];
    s32 low_height;
    s32 high_height;
    s32 dx;
    s32 dy;
    s32 count;
    s32 i;
    u8 emitted;

    REC.front[0] = OBJ->position[0];
    REC.front[1] = OBJ->position[1];
    REC.front[2] = OBJ->position[2];
    REC.rear[0] = OBJ->position[0];
    REC.rear[1] = OBJ->position[1];
    REC.rear[2] = OBJ->position[2];

    local[0] = 0;
    local[1] = 192;
    local[2] = 0;
    mathMulTransVec(OBJ->matrix, local, delta);
    REC.front[0] += delta[0];
    REC.front[1] += delta[1];
    REC.front[2] += delta[2];
    REC.rear[0] -= delta[0];
    REC.rear[1] -= delta[1];
    REC.rear[2] -= delta[2];

    low_height = worldGroundHeightAt(REC.front);
    high_height = worldGroundHeightAt(REC.rear);
    dx = high_height - low_height;
    OBJ->position[2] = (low_height + high_height) / 2;
    OBJ->position[2] += 48;
    dy = sub_8013FC44(0x24000 - dx * dx);
    OBJ->rotation[0] = ratan2(dx, dy);

    emitted = 0;
    i = 0;
    RotMatrixYXZ(OBJ->rotation, OBJ->matrix);
    count = humanCarCount();
    for (; i < count && emitted == 0; i++) {
        actor = humanCarGet(i);
        dx = actor->motion.pos[0] - OBJ->position[0];
        dy = actor->motion.pos[1] - OBJ->position[1];
        if (__builtin_abs(dx) + __builtin_abs(dy) < 800) {
            local[0] = -104;
            local[1] = -240;
            local[2] = 0;
            mathMulTransVec(OBJ->matrix, local, delta);
            position[0] = OBJ->position[0] + delta[0];
            position[1] = OBJ->position[1] + delta[1];
            position[2] = OBJ->position[2];
            position[2] = high_height + 152;
            emitted = 1;
            low_height += 152;
            if (REC.busy == 0)
                trackVehicleSurfaceSet(index, position, low_height, OBJ->rotation[2]);
        }
    }
}
