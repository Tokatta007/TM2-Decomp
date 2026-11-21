/* Per-player camera (80102BE8..80103888).
 *
 * The camera lives in the human car record. viewCarCameraUpdate runs once per
 * player and frame: it places the camera object, runs the camera for the
 * car's view (chase views 1..4 and 7, fixed-offset views 5 and 6) and
 * hands the result to the view system (viewSetRotation rotation,
 * viewSetPosition position).
 *
 * Views 7 (chase) and 5 (fixed) follow the car returned by aiGetWatchedCar
 * instead of the player's own car (spectating after death).
 *
 * The chase camera eases its position towards a point behind the car
 * (offset rotated by the car's heading, chosen by view and split-screen
 * mode) at rate cam_ease, keeps it above the car, and eases its rotation
 * towards the look-at point (offset cam_look_offset in car space). Rates are
 * percentages kept per frame: mathEase and mathEaseAngle return
 * (cur * p + target * (100 - p)) / 100, the second for angles (wrapping). */
#include <stddef.h>
#include <libgte.h>

#include "semantic/cars.h"   /* TM2HumanRecord (cam_* fields), TM2AIRecord */
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H1: the local-player map */
#endif
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"   /* request 63: portVisual bit 5, the eye kept inside the level */
#endif

s16 viewCameraLockFrames __attribute__((section(".tm2_80180AE4"))) = 0;            /* frames since the camera was locked */
s32 cameraPrevPos[3] __attribute__((section(".tm2_80183F14"))) = {0, 0, 0}; /* camera position before this frame's update */
extern s32 cameraZeroLookOffset[];    /* look-at offset of views 5 and 6 */
extern s16 viewCameraZeroRotation[];

extern int GetCurTics(void);
extern void viewSetCameraPose(s32 i, s32 *pos, s16 *rot);
extern struct TM2AIRecord *aiGetWatchedCar(void);
extern int viewGetSplitMode(void);
extern void mathMulTransVec(MATRIX *, s32 *, s32 *);
extern long sub_8013FC44(long);
extern void mathBoundSAngle(s16 *);
extern void mathBoundAngle(s32 *);
extern void viewSetRotation(SVECTOR *, int);
extern void viewSetPosition(s32 *, int);

void viewChaseCamera(struct TM2HumanRecord *c);
void viewFixedCameraTrack(struct TM2HumanRecord *c);
int mathEaseAngle(int cur, int target, int p);
int mathEase(int cur, int target, int p);

#pragma tm2_rdata .tm2_800CF0DC

/* @80102BE8 */
void __attribute__((section(".text_80102BE8"))) viewCarCameraUpdate(struct TM2HumanRecord *c, s32 i)
{
    GetCurTics();
    switch (c->view) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 7:
        viewSetCameraPose(i, cameraZeroLookOffset, viewCameraZeroRotation);
        viewChaseCamera(c);
        break;
    case 5:
    case 6:
        viewSetCameraPose(i, cameraZeroLookOffset, viewCameraZeroRotation);
        viewFixedCameraTrack(c);
        break;
    case 8:
        break;
    }
#ifdef TM2_PORT_HUMANS
    /* H1: i is the human's local slot (worldUpdate), not its record index */
    viewSetRotation((SVECTOR *)c->cam_rot, i);
    viewSetPosition(c->cam_pos, i);
#else
    viewSetRotation((SVECTOR *)c->cam_rot, c->player);
    viewSetPosition(c->cam_pos, c->player);
#endif
    GetCurTics();
}

/* @80102CC4: chase camera. */
void __attribute__((section(".text_80102CC4"))) viewChaseCamera(struct TM2HumanRecord *c)
{
    s32 vec[3];
    s16 view[4];
    s32 out[3];
    s16 rot[4];
    s32 pos[3];
    s32 ofs[3];
    MATRIX m, mt;
    s16 yaw, pitch;
    struct TM2AIRecord *tg;
    struct TM2CarState *st;
    u8 locked;
    s32 rate, d;

    locked = 0;
    if (c->view == 7) {
        tg = aiGetWatchedCar();
        rot[0] = tg->motion.rot[0]; rot[1] = tg->motion.rot[1]; rot[2] = tg->motion.rot[2];
        pos[0] = tg->motion.pos[0]; pos[1] = tg->motion.pos[1]; pos[2] = tg->motion.pos[2];
        st = &tg->state;
        locked = tg->state.falling;
    } else {
        rot[0] = c->motion.rot[0]; rot[1] = c->motion.rot[1]; rot[2] = c->motion.rot[2];
        pos[0] = c->motion.pos[0]; pos[1] = c->motion.pos[1]; pos[2] = c->motion.pos[2];
        st = &c->state;
        if (c->state.falling != 0 || c->state.f1D != 0 || c->state.f20 != 0) {
            viewCameraLockFrames = 0;
            c->cam_ease = 98;
            locked = 1;
        } else if (++viewCameraLockFrames >= 41 && c->state.all_in_air == 0 && c->state.frozen != 0)
            locked = 0;     /* already clear: only the flag reads remain, as in retail */
    }
    cameraPrevPos[0] = c->cam_pos[0]; cameraPrevPos[1] = c->cam_pos[1]; cameraPrevPos[2] = c->cam_pos[2];
    if (!locked) {
        /* Follow a third of the car's pitch and all of its heading, no roll. */
        view[0] = rot[0] / 3;
        view[1] = 0;
        view[2] = rot[2];
        RotMatrixYXZ((SVECTOR *)view, &m);
        switch (viewGetSplitMode()) {
        case 0:
            if (c->view == 2 || c->view == 7) {
                ofs[0] = c->cam_chase[0]; ofs[1] = c->cam_chase[1]; ofs[2] = c->cam_chase[2];
                if (c->view == 7) {
                    tg = aiGetWatchedCar();
                    ofs[1] += tg->stats.axle_rear;
                    ofs[2] += tg->stats.ride_height + tg->stats.roof_height;
                }
                mathMulTransVec(&m, ofs, c->cam_goal);
            } else if (c->view == 3)
                mathMulTransVec(&m, c->cam_offset3, c->cam_goal);
            else
                mathMulTransVec(&m, c->cam_offset, c->cam_goal);
            break;
        case 1:
            mathMulTransVec(&m, c->cam_split1, c->cam_goal);
            break;
        case 2:
            mathMulTransVec(&m, c->cam_split2, c->cam_goal);
            break;
        }
        c->cam_goal[0] += pos[0]; c->cam_goal[1] += pos[1]; c->cam_goal[2] += pos[2];
        if (c->view != 7) {
            /* Lead the camera by the car's speed. */
            vec[0] = 0;
            vec[1] = c->motion.speed / 32;
            vec[2] = 0;
            mathMulTransVec((MATRIX *)&c->motion.rot_m, vec, out);
            c->cam_pos[0] += out[0]; c->cam_pos[1] += out[1]; c->cam_pos[2] += out[2];
        }
    }
    if (c->view != 7 && st->all_in_air == 0 && c->cam_pos[2] < c->motion.pos[2])
        c->cam_pos[2] = c->motion.pos[2];
    if (c->view != 4) {
        rate = 40;
        if (c->state.f06 != 0) {
            rate = 70 - (__builtin_abs(c->motion.steer) * 30) / c->stats.steer_limit;
            if (rate >= 99)
                rate = 98;
            else if (rate < 40)
                rate = 40;
        }
        c->cam_ease = mathEase(c->cam_ease, rate, 95);
    } else
        c->cam_ease = 90;
    c->cam_pos[0] = mathEase(c->cam_pos[0], c->cam_goal[0], c->cam_ease);
    c->cam_pos[1] = mathEase(c->cam_pos[1], c->cam_goal[1], c->cam_ease);
    c->cam_pos[2] = mathEase(c->cam_pos[2], c->cam_goal[2], c->cam_ease);
    if (c->view != 7) {
        /* Keep the camera at least 40 units above the car (car space). */
        vec[0] = c->cam_pos[0] - pos[0];
        vec[1] = c->cam_pos[1] - pos[1];
        vec[2] = c->cam_pos[2] - pos[2];
        TransposeMatrix((MATRIX *)&c->motion.rot_m, &mt);
        mathMulTransVec(&mt, vec, out);
        if (out[1] >= -40) {
            vec[0] = out[0];
            vec[1] = -40;
            vec[2] = out[2];
            mathMulTransVec((MATRIX *)&c->motion.rot_m, vec, out);
            c->cam_pos[0] = pos[0] + out[0];
            c->cam_pos[1] = pos[1] + out[1];
            c->cam_pos[2] = pos[2] + out[2];
        }
        c->cam_look_offset[1] = mathEase(c->cam_look_offset[1], 40, 98);
    }
    mathMulTransVec((MATRIX *)&c->motion.rot_m, c->cam_look_offset, c->cam_look);
    c->cam_look[0] += pos[0]; c->cam_look[1] += pos[1]; c->cam_look[2] += pos[2];
#ifdef TM2_PORT_VISUAL
    portVisualCameraWall(c->cam_look, c->cam_pos);     /* request 63 (FAR_LOD.md 10): bit 5 (tested inside) */
#endif
    vec[0] = c->cam_look[0] - c->cam_pos[0];
    vec[1] = c->cam_look[1] - c->cam_pos[1];
    vec[2] = c->cam_look[2] - c->cam_pos[2];
    d = sub_8013FC44(vec[0] * vec[0] + vec[1] * vec[1]);
    if (vec[1] == 0)
        vec[1] = 1;
    if (d == 0)
        d = 1;
    yaw = ratan2(vec[0], vec[1]);
    switch (viewGetSplitMode()) {
    case 0:
        if (c->view == 2 || c->view == 7)
            pitch = -ratan2(vec[2], d) - 159;
        else if (c->view == 3)
            pitch = -ratan2(vec[2], d) - 204;
        else
            pitch = -ratan2(vec[2], d) - 227;
        break;
    case 1:
        pitch = -ratan2(vec[2], d) - 193;
        break;
    case 2:
        pitch = -ratan2(vec[2], d) - 102;
        break;
    }
    mathBoundSAngle(&yaw);
    mathBoundSAngle(&pitch);
    if (locked) {
        if (st->falling)
            c->cam_rot[0] = mathEaseAngle(c->cam_rot[0], pitch, 80);
        else
            c->cam_rot[0] = mathEaseAngle(c->cam_rot[0], pitch, 98);
        c->cam_rot[1] = 0;
        c->cam_rot[2] = mathEaseAngle(c->cam_rot[2], yaw, 90);
    } else {
        c->cam_rot[0] = mathEaseAngle(c->cam_rot[0], pitch, 70);
        c->cam_rot[1] = 0;
        c->cam_rot[2] = mathEaseAngle(c->cam_rot[2], yaw, 40);
    }
}

/* @8010353C: fixed camera (views 5 and 6): stays put, turns to the car. */
void __attribute__((section(".text_8010353C"))) viewFixedCameraTrack(struct TM2HumanRecord *c)
{
    VECTOR v;
    SVECTOR rot;
    VECTOR pos;
    s16 yaw, pitch;
    int locked;
    int d;

    if (c->view == 5) {
        struct TM2AIRecord *tg = aiGetWatchedCar();
        rot.vx = tg->motion.rot[0];
        rot.vy = tg->motion.rot[1];
        rot.vz = tg->motion.rot[2];
        pos.vx = tg->motion.pos[0];
        pos.vy = tg->motion.pos[1];
        pos.vz = tg->motion.pos[2];
        locked = tg->state.falling;
    } else {
        rot.vx = c->motion.rot[0];
        rot.vy = c->motion.rot[1];
        rot.vz = c->motion.rot[2];
        pos.vx = c->motion.pos[0];
        pos.vy = c->motion.pos[1];
        pos.vz = c->motion.pos[2];
        locked = c->state.falling;
    }
    mathMulTransVec((MATRIX *)&c->motion.rot_m, cameraZeroLookOffset, c->cam_look);
    c->cam_look[0] += pos.vx;
    c->cam_look[1] += pos.vy;
    c->cam_look[2] += pos.vz;
    v.vx = c->cam_look[0] - c->cam_pos[0];
    v.vy = c->cam_look[1] - c->cam_pos[1];
    v.vz = c->cam_look[2] - c->cam_pos[2];
    d = sub_8013FC44(v.vx * v.vx + v.vy * v.vy);
    if (!v.vy)
        v.vy = 1;
    if (!d)
        d = 1;
    yaw = ratan2(v.vx, v.vy);
    pitch = -ratan2(v.vz, d);
    mathBoundSAngle(&yaw);
    mathBoundSAngle(&pitch);
    c->cam_rot[1] = 0;
    if (__builtin_abs(c->cam_rot[2] - yaw) > 512) {
        c->cam_rot[0] = pitch;
        c->cam_rot[2] = yaw;
    } else {
        c->cam_rot[0] = mathEaseAngle(c->cam_rot[0], pitch, 80);
        c->cam_rot[2] = mathEaseAngle(c->cam_rot[2], yaw, 80);
    }
}

/* @801037A0 */
void __attribute__((section(".text_801037A0"))) nullsub_16(void)
{
}

/* @801037A8: ease an angle (0..4095, wrapping) towards target. */
int __attribute__((section(".text_801037A8"))) mathEaseAngle(int cur, int target, int p)
{
    s32 r;

    if (cur < -1024 && target > 1024)
        target -= 4096;
    else if (cur > 1024 && target < -1024)
        target += 4096;
    r = (cur * p + target * (100 - p)) / 100;
    mathBoundAngle(&r);
    return r;
}

/* @80103840: ease a value towards target, keeping p percent. */
int __attribute__((section(".text_80103840"))) mathEase(int cur, int target, int p)
{
    int r = (cur * p + target * (100 - p)) / 100;

    if (r == cur)
        r = target;
    return r;
}
