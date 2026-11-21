/* Car body pose (80114F24): orient and place a car's body object from its
 * four wheel contacts. Takes (car record, human) like the motion helpers
 * (car_motion.c): wheels at human +0x5A4 / AI +0x5E4, motion block at
 * human +0x6DC / AI +0x71C.
 *
 * Pitch and roll come from the wheel position differences turned into car
 * space, heading from the car; the height is the average wheel height
 * (cars 0x40, 0x60, 0xA0, 0xB0, 0xC0 sit 3 higher). The body objects are
 * carBodyObjects[id >> 4]. */
#include "semantic/cars.h"
#include "semantic/car_groups.h"

typedef struct TM2CarMatrix TM2Matrix;

typedef struct {
    s32 f0;
    TM2Matrix m;            /* +0x04 */
} TM2BodyObject;

#ifdef TM2_PORT_CAR_GROUPS
/* request 32 (CAR_GROUPS.md): the body object of the car's key */
#define carBodyObjects ((TM2BodyObject **)portCarGroups.body)
#else
extern TM2BodyObject *carBodyObjects[];
#endif

extern void mathMulMatVec(TM2Matrix *, s32 *, s32 *);
extern int ratan2(int, int);
extern void *RotMatrixYXZ(s16 *, TM2Matrix *);

/* @80114F24 */
void __attribute__((section(".text_80114F24"))) carBodyPoseUpdate(void *car, s32 human)
{
    s16 r[4];
    s32 a[3], b[3];
    s32 ca[3], cb[3];
    struct TM2CarWheels *c;
    struct TM2CarMotion *m;
    s32 id, slot, h, pitch;

    if (human) {
        c = &((struct TM2HumanRecord *)car)->wheels;
        m = &((struct TM2HumanRecord *)car)->motion;
#ifdef TM2_PORT_CAR_GROUPS
        slot = TM2_CAR_KEY(((struct TM2HumanRecord *)car)->slot, ((struct TM2HumanRecord *)car)->type) >> 4;
#else
        slot = ((struct TM2HumanRecord *)car)->type >> 4;
#endif
        id = ((struct TM2HumanRecord *)car)->type;
    } else {
        c = &((struct TM2AIRecord *)car)->wheels;
        m = &((struct TM2AIRecord *)car)->motion;
#ifdef TM2_PORT_CAR_GROUPS
        slot = TM2_CAR_KEY(((struct TM2AIRecord *)car)->slot, ((struct TM2AIRecord *)car)->type) >> 4;
#else
        slot = ((struct TM2AIRecord *)car)->type >> 4;
#endif
        id = ((struct TM2AIRecord *)car)->type;
    }
    if (!carBodyObjects[slot])
        return;
    h = (((c->wheel[0].h + c->wheel[2].h) >> 1) + ((c->wheel[1].h + c->wheel[3].h) >> 1)) >> 1;
    a[0] = c->wheel[3].pos[0] - c->wheel[2].pos[0];
    a[1] = c->wheel[3].pos[1] - c->wheel[2].pos[1];
    a[2] = (c->wheel[3].pos[2] - c->wheel[3].h) - (c->wheel[2].pos[2] - c->wheel[2].h);
    b[0] = c->wheel[0].pos[0] - c->wheel[2].pos[0];
    b[1] = c->wheel[0].pos[1] - c->wheel[2].pos[1];
    b[2] = (c->wheel[0].pos[2] - c->wheel[0].h) - (c->wheel[2].pos[2] - c->wheel[2].h);
    mathMulMatVec(&m->rot_m, a, ca);
    mathMulMatVec(&m->rot_m, b, cb);
    pitch = -ratan2(ca[2], ca[0]);
    r[0] = ratan2(cb[2], cb[1]);
    r[1] = pitch;
    r[2] = m->rot[2];
    RotMatrixYXZ(r, &carBodyObjects[slot]->m);
    carBodyObjects[slot]->m.t[0] = m->pos[0];
    carBodyObjects[slot]->m.t[1] = m->pos[1];
    if (id == 0xA0 || id == 0xB0 || id == 0x40 || id == 0xC0 || id == 0x60)
        carBodyObjects[slot]->m.t[2] = h + 3;
    else
        carBodyObjects[slot]->m.t[2] = h;
}
