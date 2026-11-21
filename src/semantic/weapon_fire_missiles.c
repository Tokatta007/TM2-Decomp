/* Game helpers (800D..800E range). Names stay address-based. */
#include "semantic/weapons.h"

#pragma tm2_rdata .tm2_800CEB28

/* Effect record (980 bytes each, table at fxRecords). */
typedef struct {
    s8 f00;
    u8 pad_01;
    s16 f02, f04, f06, f08, f0A;
    u8 pad_0C[0x28 - 0x0C];
    s32 f28, f2C, f30;
    struct TM2SceneObject *f34;
    s32 f38;
    s32 f3C;
    u8 f40, f41;
    s16 f42;
    u8 f44, f45;
    s16 f46;
    u8 pad_48[980 - 0x48];
} TM2Effect;

extern TM2Effect fxRecords[];

/* Moscow dish event (random event 0 at moscowDishState; moscowDishUpdate in car_destroyed.c). */
typedef struct {
    u8 active;              /* +0x00 */
    u8 f01;                 /* +0x01 */
    u8 retire_time;         /* +0x02 timer value at which the event ends */
    u8 spark_rate;          /* +0x03 1 in spark_rate chance of a spark per frame */
    u8 final_blast_done;    /* +0x04 */
    u8 destroyed;           /* +0x05 */
    s16 timer;              /* +0x06 frame counter */
    s16 burst_time;         /* +0x08 timer value of the debris burst */
    s16 burst_duration;     /* +0x0A */
    s32 object;             /* +0x0C dish object (moscowDishRegister) */
    s32 pos[3];             /* +0x10 */
    s16 spin[3];            /* +0x1C random spin passed to debrisBurstAt */
    u8 pad_22[2];
    s32 f24;                /* +0x24 */
    s32 f28;                /* +0x28 */
    s32 f2C;                /* +0x2C */
    s16 dir[3];             /* +0x30 debrisBurstAt direction */
} TM2DishEvent;

extern TM2DishEvent moscowDishState[];

extern s32 weaponHomingMissileModel, weaponNapalmModel, weaponColoredMissileModel;

extern int rand(void);
extern void *RotMatrixYXZ(s16 *, u8 *);
#include "semantic/world.h"
extern s32 fxRecordFindFree(s32);
extern void missileSetTrailColor(s32, s32, s32, s32);
extern void missileTrailStart(s32);
extern void missileTrailSetUVs(TM2Effect *);
#include "semantic/fx.h"
extern void carShakeRandomize(void *car, u8 side);
extern void wheelSpinOutSettle(struct TM2CarWheel *, struct TM2CarMotion *);
extern void carBodyFollowCar(struct TM2CarMotion *, struct TM2CarState *);

/* Start the random event at moscowDishState (if armed): randomise timers, offsets and spin. */
void __attribute__((section(".text_800D5F94"))) moscowDishStartDestroy(void)
{
    TM2DishEvent *p = moscowDishState;

    if (moscowDishState[0].object == 0)
        return;
    p->active = 1;
    moscowDishState[0].timer = 0;
    moscowDishState[0].spark_rate = 30;
    moscowDishState[0].retire_time = 0;
    moscowDishState[0].final_blast_done = 0;
    moscowDishState[0].burst_duration = (rand() >> 9) + 20;
    moscowDishState[0].dir[0] = rand() / 8;
    moscowDishState[0].dir[1] = rand() / 8;
    moscowDishState[0].dir[2] = 0xFFF;
    moscowDishState[0].burst_time = (rand() >> 9) + 100;
    moscowDishState[0].destroyed = 0;
    if (rand() & 1)
        moscowDishState[0].spin[0] = rand() / 6553;
    else
        moscowDishState[0].spin[0] = -rand() / 6553;
    if (rand() & 1)
        p->spin[1] = rand() / 6553;
    else
        p->spin[1] = -rand() / 6553;
    p->spin[2] = 0;
    p->f24 = 0;
    p->f28 = 0;
    p->f2C = -6;
}

/* Initialise an effect record object: position (yaw-offset for types 15/14), velocity, matrix, colour. */
void __attribute__((section(".text_800D96F8"))) fxRecordInitObject(TM2Effect *rec, s32 *pos, s32 *vel)
{
    struct TM2SceneObject *obj = rec->f34;

    rec->f45 = 1;
    rec->f40 = 0;
    rec->f41 = 1;
    rec->f38 = 0;
    rec->f44 = 6;
    if (rec->f02 == 15)
        obj->rot[0] = pos[0] - 512;
    else if (rec->f02 == 14)
        obj->rot[0] = pos[0] - 170;
    else
        obj->rot[0] = pos[0];
    obj->rot[1] = pos[1];
    obj->rot[2] = pos[2];
    obj->pos[0] = vel[0];
    obj->pos[1] = vel[1];
    obj->pos[2] = vel[2];
    rec->f28 = vel[0];
    rec->f2C = vel[1];
    rec->f30 = vel[2];
    if (obj->model == 0)
        obj->visible = 0;
    else
        obj->visible = 1;
    RotMatrixYXZ(obj->rot, (u8 *)&obj->matrix);
    missileSetTrailColor(rec->f00, 0x80, 0x80, 0x80);
    rec->f46 = 0;
}

/* Spawn effect type 11 (purple, 50/150 timers); returns record index or -1. */
s16 __attribute__((section(".text_800DA044"))) weaponFireHomingMissile(s32 a0, s32 a1, s32 *pos, s32 *vel, s32 a4)
{
    TM2Effect *rec;
    struct TM2SceneObject *obj;
    s32 i = fxRecordFindFree(0);

    if (i < 0)
        return -1;
    rec = &fxRecords[i];
    obj = rec->f34;
    rec->f02 = 11;
    fxRecordInitObject(rec, pos, vel);
    rec->f04 = 50;
    rec->f06 = 150;
    rec->f38 = a1;
    rec->f0A = a4;
    rec->f42 = a0;
    rec->f08 = 50;
    obj->model = weaponHomingMissileModel;
    missileSetTrailColor(i, 0x40, 0, 0x40);
    missileTrailStart(i);
    missileTrailSetUVs(rec);
    fxTrailUpdate(rec, i);
    if (obj->model != 0)
        obj->visible = 1;
    else
        obj->visible = 0;
    return i;
}

/* Spawn effect type 15 (orange, 150/30 timers, rate 7); returns record index or -1. */
s16 __attribute__((section(".text_800DA190"))) weaponFireNapalm(s32 a0, s32 a1, s32 *pos, s32 *vel, s32 a4)
{
    TM2Effect *rec;
    struct TM2SceneObject *obj;
    s32 i = fxRecordFindFree(0);

    if (i < 0)
        return -1;
    rec = &fxRecords[i];
    obj = rec->f34;
    rec->f02 = 15;
    fxRecordInitObject(rec, pos, vel);
    rec->f04 = 150;
    rec->f06 = 30;
    rec->f38 = a1;
    rec->f0A = a4;
    rec->f42 = a0;
    rec->f08 = 7;
    rec->f46 = 100;
    obj->model = weaponNapalmModel;
    missileSetTrailColor(i, 0xFF, 0x80, 0x20);
    missileTrailStart(i);
    missileTrailSetUVs(rec);
    fxTrailUpdate(rec, i);
    if (obj->model != 0)
        obj->visible = 1;
    else
        obj->visible = 0;
    return i;
}

/* Spawn effect type 15 (orange, 150/75 timers, rate 50); returns record index or -1. */
s16 __attribute__((section(".text_800DA2EC"))) weaponFireMegaNapalm(s32 a0, s32 a1, s32 *pos, s32 *vel, s32 a4)
{
    TM2Effect *rec;
    struct TM2SceneObject *obj;
    s32 i = fxRecordFindFree(0);

    if (i < 0)
        return -1;
    rec = &fxRecords[i];
    obj = rec->f34;
    rec->f02 = 15;
    fxRecordInitObject(rec, pos, vel);
    rec->f04 = 150;
    rec->f06 = 75;
    rec->f38 = a1;
    rec->f0A = a4;
    rec->f42 = a0;
    rec->f08 = 50;
    rec->f46 = 100;
    obj->model = weaponNapalmModel;
    missileSetTrailColor(i, 0xFF, 0x80, 0x20);
    missileTrailStart(i);
    missileTrailSetUVs(rec);
    fxTrailUpdate(rec, i);
    if (obj->model != 0)
        obj->visible = 1;
    else
        obj->visible = 0;
    return i;
}

/* Spawn effect type 0x29 with caller colour rgb[3]; returns record index or -1. */
s16 __attribute__((section(".text_800DA448"))) weaponFireColoredMissile(s32 a0, s32 a1, s32 *pos, s32 *vel, s32 a4, s16 *rgb)
{
    TM2Effect *rec;
    struct TM2SceneObject *obj;
    s32 i = fxRecordFindFree(0);

    if (i < 0)
        return -1;
    rec = &fxRecords[i];
    obj = rec->f34;
    rec->f02 = 0x29;
    fxRecordInitObject(rec, pos, vel);
    rec->f04 = 100;
    rec->f06 = 150;
    rec->f38 = a1;
    rec->f08 = 0x23;
    rec->f0A = a4;
    rec->f42 = a0;
    obj->model = weaponColoredMissileModel;
    missileSetTrailColor(i, rgb[0], rgb[1], rgb[2]);
    missileTrailStart(i);
    fxTrailUpdate(rec, i);
    if (obj->model != 0)
        obj->visible = 1;
    else
        obj->visible = 0;
    return i;
}

/* Car shake setup: channel (side ? +0x114 : +0x9C), strength by level, random sign per enabled axis. */
void __attribute__((section(".text_800E4D8C"))) carShakeStart(void *car, s32 level, s32 axes, s32 time, u8 side)
{
    struct TM2CarAim *p;
    s32 s;

    if (side)
        p = &((struct TM2HumanRecord *)car)->aim;
    else
        p = &((struct TM2AIRecord *)car)->aim;
    p->shake = level;
    p->shake_count = time;
    if (p->shake_count <= 0) {
        p->shake_level = level;
        p->shake_axes = axes;
        p->shake_count = ~rand() & 3;
    }
    p->shake_phase = rand() % 11;
    switch (level) {
    case 5:
        s = 113;
        break;
    case 4:
        s = 68;
        break;
    case 3:
        s = 45;
        break;
    case 2:
        s = 22;
        break;
    case 1:
    default:
        s = 5;
        break;
    }
    if (axes == 0 || axes == 2) {
        if (rand() & 1)
            s = -s;
        p->shake_x = s;
    } else
        p->shake_x = 0;
    if (axes == 1 || axes == 2) {
        if (rand() & 1)
            s = -s;
        p->shake_y = s;
    } else
        p->shake_y = 0;
    p->f12 = 0x155;
    carShakeRandomize(car, side);
}

/* AI side update: process 4 contact slots, accumulate heading and settle motion if aligned. */
void __attribute__((section(".text_800E6B78"))) carSpinOutUpdate(void *car, u8 side)
{
    struct TM2CarWheels *slots;
    struct TM2CarMotion *motion;
    struct TM2CarState *state;
    s32 *rate;
    u8 prev;
    s32 a;

    if (side) {
        slots = &((struct TM2HumanRecord *)car)->wheels;
        motion = &((struct TM2HumanRecord *)car)->motion;
        state = &((struct TM2HumanRecord *)car)->state;
        rate = &((struct TM2HumanRecord *)car)->motion.fE8;
    } else {
        slots = &((struct TM2AIRecord *)car)->wheels;
        motion = &((struct TM2AIRecord *)car)->motion;
        state = &((struct TM2AIRecord *)car)->state;
        rate = &((struct TM2AIRecord *)car)->motion.fE8;
    }
    prev = state->f0F;
    state->f0F = 0;
    if (slots->wheel[0].f03) {
        wheelSpinOutSettle(&slots->wheel[0], motion);
        state->f0F = 1;
    }
    if (slots->wheel[1].f03) {
        wheelSpinOutSettle(&slots->wheel[1], motion);
        state->f0F = 1;
    }
    if (slots->wheel[2].f03) {
        wheelSpinOutSettle(&slots->wheel[2], motion);
        state->f0F = 1;
    }
    if (slots->wheel[3].f03) {
        wheelSpinOutSettle(&slots->wheel[3], motion);
        state->f0F = 1;
    }
    if (state->f0F) {
        motion->yaw_rate += *rate;
        a = motion->rot[2] - motion->body_rot[2];
        mathBoundAngle(&a);
        if ((a < 0 ? -a : a) < 227 && prev && state->f27)
            carBodyFollowCar(motion, state);
    } else {
        state->f28 = 0;
        state->f27 = 0;
    }
}
