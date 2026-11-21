#include "semantic/cars.h"   /* u8..u32, TM2HumanRecord / TM2AIRecord */
/* Raw accesses kept where a field access changes the code (see the uses). */
#define U8(p,o) (*(u8 *)((u8 *)(p)+(o)))
#define U16(p,o) (*(u16 *)((u8 *)(p)+(o)))

s16 viewCameraLockFrames __attribute__((section(".tm2_80180AE4")))=0;
s32 cameraPrevPos[3] __attribute__((section(".tm2_80183F14")))={0,0,0};
extern struct TM2AIRecord *aiGetWatchedCar(void);
extern void RotMatrixYXZ(const s16 *,void *);
extern void TransposeMatrix(const void *,void *);
extern void mathMulTransVec(const void *,const s32 *,s32 *);
extern s32 viewGetSplitMode(void);
extern s32 mathEase(s32,s32,s32);
extern s32 mathEaseAngle(s32,s32,s32);
extern s32 sub_8013FC44(s32);
extern s32 ratan2(s32,s32);
extern void mathBoundSAngle(s16 *);

#define TYPE car->view

/* @80102CC4. Update the current aim point, transformed offsets and bounded
 * pitch/yaw controls for one car record. */
void __attribute__((section(".text_80102CC4"))) viewChaseCamera(struct TM2HumanRecord *car)
{
    s32 vec[3];
    s16 view[4];
    s32 out[3];
    s16 rotation[4];
    s32 origin[3];
    s32 offset[3];
    u8 matrix[32], transpose[32];
    s16 yaw, pitch;
    struct TM2AIRecord *aux;
    struct TM2CarState *flags;
    u8 special;
    s32 rate, horizontal;

    special = 0;
    if (TYPE == 7) {
        aux = aiGetWatchedCar();
        rotation[0] = aux->motion.rot[0]; rotation[1] = aux->motion.rot[1]; rotation[2] = aux->motion.rot[2];
        origin[0] = aux->motion.pos[0]; origin[1] = aux->motion.pos[1]; origin[2] = aux->motion.pos[2];
        flags = &aux->state;
        special = aux->state.falling;
    } else {
        rotation[0] = car->motion.rot[0]; rotation[1] = car->motion.rot[1]; rotation[2] = car->motion.rot[2];
        origin[0] = car->motion.pos[0]; origin[1] = car->motion.pos[1]; origin[2] = car->motion.pos[2];
        flags = &car->state;
        /* state +0x1F falling, +0x1D, +0x20: raw byte reads; as fields gcc merges them into one word test */
        if (U8(car,0xBB) != 0 || U8(car,0xB9) != 0 || U8(car,0xBC) != 0) {
            viewCameraLockFrames = 0;
            car->cam_ease = 0x62;
            special = 1;
        } else if (++viewCameraLockFrames >= 41 && car->state.all_in_air == 0 && car->state.frozen != 0)
            special = 0;   /* already clear: only the flag reads remain, as in retail */
    }
    cameraPrevPos[0] = car->cam_pos[0]; cameraPrevPos[1] = car->cam_pos[1]; cameraPrevPos[2] = car->cam_pos[2];
    if (!special) {
        view[0] = rotation[0] / 3;
        view[1] = 0;
        view[2] = rotation[2];
        RotMatrixYXZ(view, matrix);
        switch (viewGetSplitMode()) {
        case 0:
            if (TYPE == 2 || TYPE == 7) {
                offset[0] = car->cam_chase[0]; offset[1] = car->cam_chase[1]; offset[2] = car->cam_chase[2];
                if (TYPE == 7) {
                    aux = aiGetWatchedCar();
                    offset[1] += aux->stats.axle_rear;
                    offset[2] += aux->stats.ride_height + aux->stats.roof_height;
                }
                mathMulTransVec(matrix, offset, car->cam_goal);
            } else if (TYPE == 3)
                mathMulTransVec(matrix, car->cam_offset3, car->cam_goal);
            else
                mathMulTransVec(matrix, car->cam_offset, car->cam_goal);
            break;
        case 1:
            mathMulTransVec(matrix, car->cam_split1, car->cam_goal);
            break;
        case 2:
            mathMulTransVec(matrix, car->cam_split2, car->cam_goal);
            break;
        }
        car->cam_goal[0] += origin[0]; car->cam_goal[1] += origin[1]; car->cam_goal[2] += origin[2];
        if (TYPE != 7) {
            vec[0] = 0;
            vec[1] = car->motion.speed / 32;
            vec[2] = 0;
            mathMulTransVec(&car->motion.rot_m, vec, out);
            car->cam_pos[0] += out[0]; car->cam_pos[1] += out[1]; car->cam_pos[2] += out[2];
        }
    }
    if (TYPE != 7 && flags->all_in_air == 0 && car->cam_pos[2] < car->motion.pos[2])
        car->cam_pos[2] = car->motion.pos[2];
    if (TYPE != 4) {
        rate = 0x28;
        if (car->state.f06 != 0) {
            rate = 0x46 - (__builtin_abs(car->motion.steer) * 30) / car->stats.steer_limit;
            if (rate >= 0x63)
                rate = 0x62;
            else if (rate < 0x28)
                rate = 0x28;
        }
        car->cam_ease = mathEase(car->cam_ease, rate, 0x5F);
    } else
        car->cam_ease = 0x5A;
    car->cam_pos[0] = mathEase(car->cam_pos[0], car->cam_goal[0], car->cam_ease);
    car->cam_pos[1] = mathEase(car->cam_pos[1], car->cam_goal[1], car->cam_ease);
    car->cam_pos[2] = mathEase(car->cam_pos[2], car->cam_goal[2], car->cam_ease);
    if (TYPE != 7) {
        vec[0] = car->cam_pos[0] - origin[0];
        vec[1] = car->cam_pos[1] - origin[1];
        vec[2] = car->cam_pos[2] - origin[2];
        TransposeMatrix(&car->motion.rot_m, transpose);
        mathMulTransVec(transpose, vec, out);
        if (out[1] >= -40) {
            vec[0] = out[0];
            vec[1] = -40;
            vec[2] = out[2];
            mathMulTransVec(&car->motion.rot_m, vec, out);
            car->cam_pos[0] = origin[0] + out[0];
            car->cam_pos[1] = origin[1] + out[1];
            car->cam_pos[2] = origin[2] + out[2];
        }
        car->cam_look_offset[1] = mathEase(car->cam_look_offset[1], 0x28, 0x62);
    }
    mathMulTransVec(&car->motion.rot_m, car->cam_look_offset, car->cam_look);
    car->cam_look[0] += origin[0]; car->cam_look[1] += origin[1]; car->cam_look[2] += origin[2];
    vec[0] = car->cam_look[0] - car->cam_pos[0];
    vec[1] = car->cam_look[1] - car->cam_pos[1];
    vec[2] = car->cam_look[2] - car->cam_pos[2];
    horizontal = sub_8013FC44(vec[0] * vec[0] + vec[1] * vec[1]);
    if (vec[1] == 0)
        vec[1] = 1;
    if (horizontal == 0)
        horizontal = 1;
    yaw = ratan2(vec[0], vec[1]);
    switch (viewGetSplitMode()) {
    case 0:
        if (TYPE == 2 || TYPE == 7)
            pitch = -ratan2(vec[2], horizontal) - 0x9F;
        else if (TYPE == 3)
            pitch = -ratan2(vec[2], horizontal) - 0xCC;
        else
            pitch = -ratan2(vec[2], horizontal) - 0xE3;
        break;
    case 1:
        pitch = -ratan2(vec[2], horizontal) - 0xC1;
        break;
    case 2:
        pitch = -ratan2(vec[2], horizontal) - 0x66;
        break;
    }
    mathBoundSAngle(&yaw);
    mathBoundSAngle(&pitch);
    if (special) {
        if (flags->falling)
            car->cam_rot[0] = mathEaseAngle(car->cam_rot[0], pitch, 0x50);
        else
            car->cam_rot[0] = mathEaseAngle(car->cam_rot[0], pitch, 0x62);
        U16(car,0xE) = 0;   /* raw stores (cam_rot[1]): a field store is scheduled differently */
        car->cam_rot[2] = mathEaseAngle(car->cam_rot[2], yaw, 0x5A);
    } else {
        U16(car,0xC) = mathEaseAngle(car->cam_rot[0], pitch, 0x46);   /* raw store, as above */
        U16(car,0xE) = 0;   /* raw store, as above */
        car->cam_rot[2] = mathEaseAngle(car->cam_rot[2], yaw, 0x28);
    }
}
