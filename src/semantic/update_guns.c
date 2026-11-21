#include "semantic/cars.h"

extern void mathMulTransVec(void *, int *, int *);
extern int rsin(int);
extern int rcos(int);
extern void bulletSpawn(int, s32 kind, int *, int *, int);
extern void fxExplosionQuiet8(int, int);

#define HUMAN(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

/* @80104F70. Fire the machine guns of car: a human record when the low byte
 * of human is set, else an AI record. Vector locals are contiguous triples. */
unsigned short __attribute__((section(".text_80104F70")))
UpdateGuns(void *car, unsigned int human)
{
    struct TM2CarStatsBlock *model;
    struct TM2CarMotion *motion;
    struct TM2CarWeapons *state;
    struct TM2CarState *flags;
    struct TM2CarAim *aim;
    int direction[3], position[3], local[3];
    short owner;
    unsigned int kind;

    if (human & 255) {
        model = &HUMAN(car)->stats;
        motion = &HUMAN(car)->motion;
        state = &HUMAN(car)->weapons;
        flags = &HUMAN(car)->state;
        aim = &HUMAN(car)->aim;
        owner = (short)TM2_HUMAN_ID(HUMAN(car)->player);
        kind = humanCarGet(HUMAN(car)->player)->type;
    } else {
        model = &AI(car)->stats;
        motion = &AI(car)->motion;
        state = &AI(car)->weapons;
        flags = &AI(car)->state;
        aim = &AI(car)->aim;
        owner = (short)(AI(car)->index + 50);
        kind = aiCarGet(AI(car)->index)->type;
    }
    if (state->gun_cooldown > 0 || flags->frozen) {
        state->gun_cooldown = (unsigned short)(state->gun_cooldown - 1);
        return state->gun_cooldown;
    }
    if (state->gun_side) {
        local[0] = -model->gun[0];
        local[1] = model->gun[1];
        local[2] = model->gun[2];
    } else {
        local[0] = model->gun[0];
        local[1] = model->gun[1];
        local[2] = model->gun[2];
    }
    state->gun_side = (unsigned char)(1 - state->gun_side);
    mathMulTransVec(&motion->rot_m,local,position);
    position[0] = (int)((unsigned int)position[0] + motion->pos[0]);
    position[1] = (int)((unsigned int)position[1] + motion->pos[1]);
    position[2] = (int)((unsigned int)position[2] + motion->pos[2]);
    direction[0] = rsin((int)(motion->rot[2] - (unsigned int)aim->yaw));
    direction[1] = rcos((int)(motion->rot[2] - (unsigned int)aim->yaw));
    direction[2] = (int)(0u - (unsigned int)rsin((int)(motion->rot[0] - (unsigned int)aim->pitch)));
    if (state->mega_guns)
        bulletSpawn(owner,4,direction,position,3);
    else
        bulletSpawn(owner,0,direction,position,1);
    fxExplosionQuiet8((int)position,0);
    if (kind == 0xe0 && state->gun_side) {
        local[0] = 0;
        local[1] = (int)(0u - (unsigned int)local[1]);
        mathMulTransVec(&motion->rot_m,local,position);
        position[0] = (int)((unsigned int)position[0] + motion->pos[0]);
        position[1] = (int)((unsigned int)position[1] + motion->pos[1]);
        position[2] = (int)((unsigned int)position[2] + motion->pos[2]);
        direction[0] = (int)(0u - (unsigned int)direction[0]);
        direction[1] = (int)(0u - (unsigned int)direction[1]);
        direction[2] = (int)(0u - (unsigned int)direction[2]);
        bulletSpawn(owner,4,direction,position,3);
        fxExplosionQuiet8((int)position,0);
    }
    /* The cooldown just stored is returned as the residue in v0, as in retail. */
    if (human & 255)
        state->gun_cooldown = 2;
    else
        state->gun_cooldown = 5;
}
