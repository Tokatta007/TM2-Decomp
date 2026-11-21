/* Game code (mixed). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/car_numbers.h"   /* request 73 stage 2: TM2_SPECIAL_NUM_ID (a mod car's special numbers) */
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 95 (bit 28): portBalanceBoomerangCatch */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

typedef struct TM2Car {
    u8 pad0[0x80];
    u8 matrix[0x20];        /* +0x80 */
    s16 rot[4];             /* +0xA0 */
    s32 pos[3];             /* +0xA8 */
    u8 padB4[0x11C - 0xB4];
    s32 color;              /* +0x11C */
} TM2Car;

typedef struct TM2Seeker {
    s8 id;                  /* +0x00 */
    u8 pad1;
    s16 type;               /* +0x02 */
    s16 pad4;
    s16 delay;              /* +0x06 */
    s16 rate;               /* +0x08 */
    s16 count;              /* +0x0A */
    u8 padC[0x28 - 0x0C];
    s32 pos[3];             /* +0x28 */
    TM2Car *car;            /* +0x34 */
    s32 *target;            /* +0x38 */
    TM2Car *victim;         /* +0x3C */
} TM2Seeker;
/* the record's owner id (+0x42, weapons.h struct TM2Projectile) */
#define SEEKER_OWNER(rec) (*(s16 *)((u8 *)(rec) + 0x42))

extern s32 weaponFreezeMissileModel;
extern s32 weaponNapalmModel;
extern s32 ratan2(s32 y, s32 x);
extern s32 rand(void);
extern void missileSetTrailColor(s32, s32, s32, s32);
#include "semantic/fx.h"
extern s32 mathBlendValue(s32 current, s32 target, s32 rate);
extern void *RotMatrixYXZ(s16 *rotation, void *matrix);

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)

/* @800DA5B4: steer a seeking car-effect record toward its target point
 * (yaw/pitch with a per-frame turn rate and random wobble for types 0x29/0x31),
 * flash/retarget when far away, and rebuild the car's rotation matrix. */
void __attribute__((section(".text_800DA5B4"))) weaponProjectileSteer(TM2Seeker *rec)
{
    s32 *t;
    TM2Car *o;
    s32 dx, dy, dz;
    s32 yaw, pitch, cur;
    s16 left, right;


    t = rec->target;
    o = rec->car;
    dx = t[0] - o->pos[0];
    dy = t[1] - o->pos[1];
    dz = t[2] - o->pos[2];
    yaw = (ratan2(dx, dy) + 4096) % 4096;
    dx = MAX(ABS(dx), ABS(dy)) + (MIN(ABS(dx), ABS(dy)) >> 1);
    pitch = (4096 - ratan2(dz, dx)) % 4096;
    if (pitch > 1024 && pitch < 3072)
        pitch = (5120 - pitch) % 4096;

    cur = rec->car->rot[2];
    if (cur - yaw > 2048)
        yaw += 4096;
    if (yaw - cur > 2048)
        yaw -= 4096;
    if (rec->type == 0x29 && ABS(cur - yaw) > 341)
        rec->target = 0;
    if (ABS(cur - yaw) <= rec->rate) {
        left = 0;
        right = 0;
        rec->car->rot[2] = yaw;
    } else if (cur < yaw) {
        left = 0;
        right = 1;
        rec->car->rot[2] += rec->rate;
    } else {
        left = 1;
        right = 0;
        rec->car->rot[2] -= rec->rate;
    }

    if (rec->type == 0x29) {
        rec->car->rot[2] += rand() % (rec->rate * 4) - (rec->rate << 1);
        if (DISTANCE(rec->pos, rec->car->pos) > 300) {
            rec->count -= 1;
            if (rec->count < 2)
                rec->count = 2;
            if (rand() & 1) {
                rec->car->color = weaponNapalmModel;
                missileSetTrailColor(rec->id, 255, 255, 255);
            } else {
                rec->car->color = weaponFreezeMissileModel;
                missileSetTrailColor(rec->id, 0, 0, 255);
            }
        }
    }
    if (rec->type == 0x31)
        rec->car->rot[2] += rand() % (rec->rate * 4) - (rec->rate << 1);
    if (rec->car->rot[2] >= 4096)
        rec->car->rot[2] -= 4096;
    if (rec->car->rot[2] < 0)
        rec->car->rot[2] += 4096;

    cur = rec->car->rot[0];
    if (cur - pitch > 2048)
        pitch += 4096;
    if (pitch - cur > 2048)
        pitch -= 4096;
    if (ABS(cur - pitch) <= rec->rate)
        rec->car->rot[0] = pitch;
    else if (cur < pitch)
        rec->car->rot[0] += rec->rate;
    else
        rec->car->rot[0] -= rec->rate;
    if (rec->type == 0x29 || rec->type == 0x31)
        rec->car->rot[0] += rand() % (rec->rate * 2) - rec->rate;
    if (rec->car->rot[0] >= 4096)
        rec->car->rot[0] -= 4096;
    if (rec->car->rot[0] < 0)
        rec->car->rot[0] += 4096;

    if (rec->type == 0x33) {
        if (DISTANCE(rec->pos, rec->car->pos) > TM2_SPECIAL_NUM_ID(SEEKER_OWNER(rec), TM2_SPN_TURNBACK, 300)) {
            if (rec->victim->pos[0] != rec->target[0]) {
                rec->count <<= 2;
                rec->rate = 30;
                rec->delay = 50;
                missileSetTrailColor(rec->id, 255, 0, 0);
            }
            rec->target = rec->victim->pos;
        }
        if (rec->victim->pos[0] == rec->target[0] && rec->victim->pos[1] == rec->target[1]) {
#ifdef TM2_PORT_BALANCE
            /* request 95 (bit 28, BALANCE.md 21): within the catch distance it is caught (the charge back), not
               taken away */
            if (portBalanceBoomerangCatch(rec, rec->car->pos))
                ;
            else
#endif
            if (DISTANCE(rec->victim->pos, rec->car->pos) < 20)
                fxRecordFree(rec->id, 1);
            rec->car->rot[2] = mathBlendValue(rec->car->rot[2], yaw, 80);
        }
        if (left)
            rec->car->rot[1] = mathBlendValue(rec->car->rot[1], 1024, 70);
        else if (right)
            rec->car->rot[1] = mathBlendValue(rec->car->rot[1], -1024, 70);
        else
            rec->car->rot[1] = mathBlendValue(rec->car->rot[1], 0, 90);
    }
    RotMatrixYXZ(rec->car->rot, rec->car->matrix);
}

/* Retail re-sign-extends the result in this unit (as with an s16 return);
 * timerGetFrameTicks returns int (timer_frame.c), and other units use it so. */
extern s16 timerGetFrameTicks(void);
extern void mathBoundAngle(s32 *);
extern u8 carFindByObject(struct TM2Object *, u8 *, s32 *);
extern void *fxFlameBurning(void *, s32);
extern u8 carCarCollision(void *car, s32 human, void *other, s32 side, s32 steer, s32 index, s32 force);
extern s32 *worldTrackVehiclePos(s32);
extern s32 worldTrackVehicleHeading(s32);
extern s32 rsin(s32);
extern s32 carObjectId(s32, u8 human);
extern void carApplyDamage(s32, s32, s32 *, s32, s32);
extern s32 GsGetWorkBase_0(void);
extern s32 targetDamage(s32, s32, s32);
#include "semantic/weapon_calls.h"
extern void *memcpy(void *, const void *, u32);

/* @800E0C28: resolve contact slot `index` of a car (human or AI layout):
 * bounce off walls (reverse the contact vector, speed-scaled damage), hand
 * car-car contacts to carCarCollision, and handle arena hazard objects
 * (kinds 100/101) with a speed clamp / spin-out / glancing deflection.
 * Returns nonzero when the contact was consumed (its slot is then cleared). */
u8 __attribute__((section(".text_800E0C28"))) carContactResolve(u32 index, void *car, u8 human)
{
    s32 angle;
    s32 rel;
    s32 steer;
    u8 side;
    s32 slot;
    u8 hit;
    s32 limit, minspeed, id, mode;
    s32 *prel;
    struct TM2CarState *state;
    struct TM2CarStatsBlock *stats;
    struct TM2CarContact *block;
    struct TM2CarMotion *motion;
    struct TM2CarWeapons *weapons;
    struct TM2ContactProbe *contact;
    struct TM2ContactRec *resp;
    struct TM2Object *obj;
    struct TM2Object *hobj;
    void *other;            /* TM2HumanRecord when side, else TM2AIRecord */
    s32 *pt;
    s32 base;
    s32 sev;
    s32 d;
    s32 dmg;
    struct TM2FireEffect *fx;

    hit = 0;
    if (human) {
        id = HREC(car)->player;
        mode = HREC(car)->type;
        state = &HREC(car)->state;
        stats = &HREC(car)->stats;
        block = &HREC(car)->contact;
        motion = &HREC(car)->motion;
        weapons = &HREC(car)->weapons;
        limit = 0;
        minspeed = 760 * timerGetFrameTicks() / 100 * 32;
    } else {
        state = &AI(car)->state;
        block = &AI(car)->contact;
        mode = AI(car)->type;
        id = AI(car)->index;
        limit = AI(car)->target_dist;
        stats = &AI(car)->stats;
        motion = &AI(car)->motion;
        weapons = &AI(car)->weapons;
        minspeed = 0;
    }
    contact = &block->probe[index];
    resp = &block->ground[index];    /* 12 records: ground[] runs on into object[] */
    if (contact->pos[1] == 0)
        contact->pos[1] = 1;
    angle = ratan2(contact->pos[0], contact->pos[1]);
    if (contact->pos[2] < 0 && contact->pos[0] * contact->pos[0] + contact->pos[1] * contact->pos[1] < contact->pos[2] * contact->pos[2]) {
        hit = 1;
        angle = motion->fC4;
    }
    if (index == 5)
        angle = 2048 - motion->fC4;
    mathBoundAngle(&angle);
    resp->f0A = angle;
    rel = motion->fC4 - angle;
    if (motion->vel[1] < 0)
        rel += 2048;
    prel = &rel;
    mathBoundAngle(prel);
    steer = 2048 - rel * 2;
    mathBoundAngle(&steer);
    resp->turn = steer;
    resp->side = index;
    memcpy(resp->pos, contact->pos, 12);
    resp->force = contact->force;

    if (contact->obj != 0) {
        obj = contact->obj;
        if (obj->id < 6000 && obj->id != 8) {
            resp->f01 = 0;
            resp->f00 = 0;
            if (carFindByObject(contact->obj, &side, &slot)) {
                if (side) {
                    other = humanCarGet(slot);
                    if (state->burning && stats->hp > 0 && HREC(other)->state.burning == 0
                        && HREC(other)->state.energy_shield == 0) {
                        HREC(other)->state.burning = 1;
                        HREC(other)->stats.burn_time = 0;
                        HREC(other)->weapons.fire_effect = fxFlameBurning(HREC(other)->motion.pos, 1);
                        state->burning = 0;
                        do fx = weapons->fire_effect; while (0);
                        if (fx)
                            fx->active = 0;
                    }
                } else {
                    other = aiCarGet(slot);
                    if (state->burning && stats->hp > 0 && AI(other)->state.burning == 0 && AI(other)->type != 240) {
                        AI(other)->state.burning = 1;
                        AI(other)->stats.burn_time = 0;
                        AI(other)->weapons.fire_effect = fxFlameBurning(AI(other)->motion.pos, 1);
                        state->burning = 0;
                        do fx = weapons->fire_effect; while (0);
                        if (fx)
                            fx->active = 0;
                    }
                }
                hit = carCarCollision(car, human, other, side, steer, index, resp->force);
                goto out;
            }
            hobj = contact->obj;
#ifdef TM2_PORT_HAZARDS
            /* request 38 G6: trains off = a plain object (unreachable while portArenaTrainsPark hides them) */
            if ((hobj->id == 100 || hobj->id == 101) && limit <= 4000 && !portArenaOff(TM2_ARENA_TRAINS)) {
#else
            if ((hobj->id == 100 || hobj->id == 101) && limit <= 4000) {
#endif
            pt = worldTrackVehiclePos(hobj->id - 100);
            base = ratan2(motion->pos[0] - pt[0], motion->pos[1] - pt[1]);
            angle = worldTrackVehicleHeading(contact->obj->id - 100);
            rel = angle - base;
            mathBoundAngle(prel);
            sev = 25;
            if (mode != 240) {
                d = __builtin_abs(rel);
                if (d >= 1707) {
                    resp->damage = 1;
                    sev = 1;
                    if (motion->speed > 32 * (1140 * timerGetFrameTicks() / 100))
                        motion->speed = 32 * (1140 * timerGetFrameTicks() / 100);
                    steer = rand() % 682;
                    if (rel > 0) {
                        motion->body_rot[2] = angle - steer;
                        motion->steer = -stats->steer_limit / 2;
                    } else {
                        motion->body_rot[2] = angle + steer;
                        motion->steer = stats->steer_limit / 2;
                    }
                    state->f01 = 1;
                    hit = 1;
                } else if (d < 341) {
                    resp->damage = 3;
                    motion->speed = 608 * timerGetFrameTicks();
                    sev = 4;
                    steer = rand() % 682;
                    if (rel > 0) {
                        motion->body_rot[2] = angle - steer;
                        motion->steer = -stats->steer_limit / 2;
                    } else {
                        motion->body_rot[2] = angle + steer;
                        motion->steer = stats->steer_limit / 2;
                    }
                    rel = angle - motion->rot[2];
                    mathBoundAngle(&rel);
                    d = __builtin_abs(rel);
                    if (d >= 456) {
                        if (d < 1479) {
                            state->f12 = 1;
                            motion->f02 = motion->body_rot[2];
                            stats->f20 = 0;
                            if (rel < 0) {
                                motion->spin1 = 1;
                                motion->f04 = motion->body_rot[2] + 1024;
                            } else {
                                motion->spin1 = -1;
                                motion->f04 = motion->body_rot[2] - 1024;
                            }
                        }
                    }
                    carSpinOut(car, human, 10, 1);
                    state->f01 = 1;
                    hit = 1;
                } else {
                    resp->damage = 3 * __builtin_abs(rsin(steer / 4) * __builtin_abs(motion->speed)) / stats->speed_cap / 4096;
                    sev = 2;
                    if (steer > 0)
                        motion->rot[2] = motion->rot[2] + 113;
                    else
                        motion->rot[2] = motion->rot[2] - 113;
                    resp->f01 = 1;
                    resp->f00 = 1;
                }
            }
            if (human)
                carApplyDamage(carObjectId(id, human), resp->damage, 0, 2, -1);
            if (mode == 240 || (GsGetWorkBase_0() & 5) == 1) {
                if (contact->obj->id == 100)
                    targetDamage(506, 4, sev);
                else
                    targetDamage(507, 5, sev);
            }
            } else
                hit = 1;
            goto out;
        }
    }
    resp->f01 = 1;
    resp->f00 = 1;
    resp->damage = 0;
    resp->pos[0] = -resp->pos[0];
    resp->pos[1] = -resp->pos[1];
    resp->pos[2] = -resp->pos[2];
    if (minspeed < __builtin_abs(motion->speed)) {
        resp->damage = 6 * __builtin_abs(rsin(steer / 4) * __builtin_abs(motion->speed)) / stats->speed_cap / 4096;
        dmg = resp->damage * stats->mass / 33;
        if (!human) {
            dmg += 4;
            targetDamage(contact->material, contact->surface, dmg);
        }
        if (dmg > 0) {
            weaponApplyHit(contact->obj->id, contact->material, contact->surface, dmg, motion->body_rot, 0);
            if (motion->speed > stats->speed_cap / 4 && contact->material == 500) {
                resp->damage = 0;
                resp->speed = motion->speed / 4;
                resp->angle = motion->body_rot[2];
                resp->f10 = motion->body_rot[2];
                resp->turn = 0;
            }
        }
    }
out:
    if (hit)
        contact->active = 0;
    return hit;
}
