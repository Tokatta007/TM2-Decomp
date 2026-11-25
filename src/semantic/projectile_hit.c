/* Projectile hit (800DB110). Names stay address-based.
 * Byte-identical to retail. (The `mflo; lw; nop; mult` hazard nops are
 * emitted by tools/maspsx for aspsx 2.34.) */
#include "semantic/cars.h"
#include "semantic/car_numbers.h"   /* request 73 stage 2: TM2_SPECIAL_NUM_ID (a mod car's special numbers) */

/* 3-component position (prev/current sample points of the hit test). */
typedef struct { s32 x, y, z; } TM2V3c;

typedef struct TM2ProjCar {
    u8 pad0[0x80];
    u8 matrix[0x20];        /* +0x80 */
    s16 rot[4];             /* +0xA0 */
    s32 pos[3];             /* +0xA8 */
    u8 padB4[0x118 - 0xB4];
    s32 owner;              /* +0x118 */
} TM2ProjCar;

typedef struct TM2HitInfo {
    u8 pad0[0x0A];
    s16 kind;               /* +0x0A */
    s16 sub;                /* +0x0C */
} TM2HitInfo;

typedef struct TM2Proj {
    s8 id;                  /* +0x00 */
    u8 pad1;
    s16 type;               /* +0x02 */
    s16 life;               /* +0x04 */
    s16 speed;              /* +0x06 */
    s16 rate;               /* +0x08 */
    s16 damage;             /* +0x0A */
    s32 hit;                /* +0x0C */
    u8 pad10[0x28 - 0x10];
    s32 pos[3];             /* +0x28 */
    TM2ProjCar *car;        /* +0x34 */
    s32 *target;            /* +0x38 */
    TM2ProjCar *victim;     /* +0x3C */
    u8 f40, f41;
    s16 owner;              /* +0x42 */
    u8 f44, f45;
    s16 drop;               /* +0x46 */
} TM2Proj;

extern void mathBoundSAngle(s16 *);
extern void *RotMatrixYXZ(s16 *, void *);
extern s32 rsin(s32);
extern void mathMulTransVec(void *, void *, void *);
extern s32 gameGetLevel(void);
extern TM2ProjCar *carObjectFromOwner(s32);
extern void *mathJitterPos(void *, s32, s32, s32);
extern void fxSparkPink(void *, s32);
extern void weaponProjectileSteer(TM2Proj *);
extern void missileSetTrailColor(s32, s32, s32, s32);
extern void fxTrailUpdate(TM2Proj *, s32);
extern s32 projectileHitTest(void *, s32, s32, s32);
extern void fxRecordFree(s16 index, s32);
extern s32 humanCarCount(void);
extern u32 humanCarObject(s32);
extern TM2HitInfo *hdpQueryPoint(s32, s32, void *, void *);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern int *fxSmokePuffBlue(void *, s32);
extern void fxSmokeC48(void *, s32);
extern void *fxSmokeBurstPink(void *, u8 flag);
extern void fxExplosion24(void *, u8 flag);
extern void fxSmokeA24(void *, s32);
extern void fxExplosion96(void *, u8 flag);
extern void fxSmokeA48(void *, s32);
extern s32 worldGroundHeightAt(void *);
extern void groundFireSpawn(void *);
#include "semantic/weapon_calls.h"
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/dt_player.h"   /* request 36 */
#endif
extern s32 soundStopInstance(s32, s32);
#ifdef TM2_PORT_ROLLBACK
#include "semantic/port_rollback.h"   /* request 60: ROUND_CLEAN (portSeekerTargetOk) */
#endif
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 95 (bit 28): portBalanceBoomerangCatch */
#endif

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)

/* Impact sound for type 14 (panned from the hit point) plus its effects. */
#define HIT_SOUND(pt)                                               do {                                                                if (viewGetSplitMode() == 0) {                                          soundLocateForView0(pt);                                                soundPlay(8, PadChkMtap(), soundGetListenerPan(), 75);          } else {                                                            soundLocateForView(pt, -1);                                             soundPlay(8, PadChkMtap(), 0, 75);                       }                                                               fxSmokePuffBlue(pt, 0);                                            fxSmokeC48(pt, 0);                                        } while (0)

/* Impact effect for type 61: one burst plus three small sparks. */
#define HIT_SPARKS(pt)                                          \
    do {                                                        \
        s32 k_;                                                 \
        fxSmokeBurstPink(pt, 0);                                    \
        for (k_ = 0; k_ < 3; k_++)                              \
            fxSparkPink(mathJitterPos(pt, 164, 164, 164), 0);   \
    } while (0)

/* @800DB110: per-frame update of one weapon projectile: steer/spin by type
 * (15 rolling, 61 wobble, 51 homing re-target), move along its heading,
 * test two half-steps for car/world hits, spawn impact effects, apply the
 * hit through weaponApplyHit, and expire it when its life runs out.
 * Matches retail byte for byte. */
void __attribute__((section(".text_800DB110"))) weaponProjectileUpdate(TM2Proj *p)
{
    s32 vel[3];
    s32 step[4];
    TM2V3c prev;
    TM2V3c cur;
    TM2HitInfo *obj;
    TM2ProjCar *c;
    s32 id;
    s32 i, j, n, flag, hx;
    struct TM2HumanRecord *h;

    id = p->id;
    obj = 0;
#ifdef TM2_PORT_ROLLBACK
    /* request 60 ROUND_CLEAN (ONLINE_ROLLBACK.md 21): a target that is no counted car object's position (a pointer of
       an earlier level, RETAIL_BUGS 37's 0xA8) stops the steering, before type 15's untargeted test sees it (asked
       for every record, 0 included: the test dominates the steering) */
    if (!portSeekerTargetOk(p->target))
        p->target = 0;
#endif
    if (p->type == 15) {
        if (p->owner == 3503)
            p->car->rot[0] += 12;
        else
            p->car->rot[0] += 32;
        mathBoundSAngle(&p->car->rot[0]);
        c = p->car;
        if (c->rot[0] > 1024)
            c->rot[0] = 1024;
        if (p->target == 0)
            RotMatrixYXZ(p->car->rot, p->car->matrix);
    }
    if (p->type == 61) {
        p->car->rot[1] = 0;
        if (p->rate == 99) {
            p->car->rot[0] = -284 * rsin(p->life * 300) / 4095;
            p->car->rot[2] += -170 * rsin(p->life * 300) / 4095;
        } else {
            p->car->rot[0] = 284 * rsin(p->life * 300) / 4095;
#ifdef TM2_PORT_DARK_TOOTH
            /* request 33-2e: any Dark Tooth's missile (retail: the first's); with bit 6 on every arena (SURVIVAL.md
               1.5 #7) */
#ifdef TM2_PORT_DT_PLAYER
            /* request 36 (DARK_TOOTH_PLAYER.md): a player's Dark Tooth's missiles wobble as the boss's everywhere */
            if (((gameGetLevel() == 8 || TM2_PORT_BOSS_ANYWHERE || TM2_PORT_DT_ORDINARY_ON) && portCarDarkToothShooter(p->owner))
                || portCarDtPlayerShooter(p->owner))
#else
            if ((gameGetLevel() == 8 || TM2_PORT_BOSS_ANYWHERE || TM2_PORT_DT_ORDINARY_ON) && portCarDarkToothShooter(p->owner))
#endif
#else
            if (gameGetLevel() == 8 && carObjectFromOwner(240) != 0 && p->owner == carObjectFromOwner(240)->owner)
#endif
                p->car->rot[2] += 170 * rsin(p->life * 300) / 4095;
        }
        RotMatrixYXZ(p->car->rot, p->car->matrix);
        fxSparkPink(mathJitterPos(p->car->pos, 0x555, 0x555, 0x555), 0);
        fxSparkPink(mathJitterPos(p->car->pos, 0x555, 0x555, 0x555), 0);
    }
    if (p->target != 0) {
        weaponProjectileSteer(p);
    } else if (p->type == 51) {
        if (DISTANCE(p->pos, p->car->pos) > TM2_SPECIAL_NUM_ID(p->owner, TM2_SPN_TURNBACK, 300)) {
            p->target = p->victim->pos;
            p->damage <<= 2;
            p->rate = 30;
            p->speed = 50;
            missileSetTrailColor(p->id, 255, 0, 0);
        }
    }

    if (p->life <= 0) {
        --p->life;
        goto out;
    }

    vel[2] = 0;
    vel[0] = 0;
    vel[1] = p->speed;
    if (p->speed < 150)
        p->speed = p->speed + 5;
    else
        p->speed = 150;
    mathMulTransVec(p->car->matrix, vel, step);
    cur.x = p->car->pos[0];
    cur.y = p->car->pos[1];
    cur.z = p->car->pos[2];
    p->car->pos[0] += step[0];
    p->car->pos[1] += step[1];
    p->car->pos[2] += step[2];
    p->life = p->life - 1;
    if (p->type == 61 && p->speed > 75)
        p->speed = 75;
    if (p->drop >= 16) {
        p->car->pos[2] -= 16;
        p->drop -= 16;
    } else if (p->drop > 0) {
        p->car->pos[2] -= p->drop;
        p->drop = 0;
    }
    flag = 0;

    for (i = 1; i >= 0; i--) {
        hx = step[0] * i;
        /* Field-by-field copy, stored through a pointer: the volatile
         * stores keep the copy ahead of the p->car reads, and the separate
         * loads give retail's v1/a0/v0 (a struct assignment becomes one
         * block-move insn with v0/v1/a0 scratches). */
        {
            TM2V3c *d = &prev;
            d->x = cur.x;
            d->y = cur.y;
            d->z = cur.z;
        }
        cur.x = p->car->pos[0] - (hx >> 1);
        cur.y = p->car->pos[1] - ((step[1] * i) >> 1);
        cur.z = p->car->pos[2] - ((step[2] * i) >> 1);
        p->hit = p->owner < 50 ? projectileHitTest(&cur, p->owner, 15, p->type)
                               : projectileHitTest(&cur, p->owner, 8, p->type);
#ifdef TM2_PORT_BALANCE
        /* request 95 (bit 28, BALANCE.md 21): back within the catch distance of its thrower (any thrower), caught */
        if (portBalanceBoomerangCatch(p, &cur.x)) {
            p->hit = 0;
            break;
        }
#endif
        if (p->hit == p->owner && p->type == 51 &&
            p->victim->pos[0] == p->target[0] && p->victim->pos[1] == p->target[1]) {
            fxRecordFree(id, 1);
#ifdef TM2_PORT_SPECIAL_PROFILE
            /* request 73: the thrower's charge back (retail: Roadkill's); request 95: once, as retail's flag (both
               sub-steps may catch it) */
            if (flag == 0) {
                flag = 1;
                portSpecialBoomerangCaught(p->owner);
            }
#else
            n = humanCarCount();
            for (j = 0; j < n; j++) {
                humanCarObject(j);
                if (humanCarGet(j)->type == 144 && flag == 0) {
                    h = humanCarGet(j);
                    flag = 1;
                    h->weapons.ammo[0] = h->weapons.ammo[0] + 1;
                }
            }
#endif
        }
        if (p->hit == p->owner)
            p->hit = 0;
        if (p->hit < 0)
            p->hit = 0;
        if ((i & 1) && p->hit == 0) {
            obj = hdpQueryPoint(p->owner, p->car->owner, &cur, &p->hit);
            if (p->hit == 8 && obj->kind == -p->owner)
                p->hit = 0;
            if (obj->kind == 3301 || obj->kind == 2 || obj->kind == 667 || obj->kind == 668 ||
                (obj->kind >= 401 && obj->kind < 441) || (obj->kind >= 3500 && obj->kind < 3510))
                p->hit = 0;
#ifdef TM2_PORT_BALANCE
            if (portBalancePickupPass(obj->kind))       /* request 124 (portBalance2 bit 0): 441..480 too */
                p->hit = 0;
#endif
            if (p->type == 49 && (obj->kind < 500 || obj->kind >= 511))
                p->hit = 0;
            if (p->hit == 8 && p->type == 51 && p->victim->pos[0] == p->target[0])
                p->hit = 0;
        }
        if (p->hit > 0) {
            cur.z += 24;
            if (p->hit != 8) {
                if (p->damage >= 0) {
                    if (p->type == 14)
                        HIT_SOUND(&cur);
                    else if (p->type == 61)
                        HIT_SPARKS(&cur);
                    else {
                        fxExplosion24(&cur, 0);
                        fxSmokeA24(&cur, 0);
                    }
                    if (p->type == 15) {
                        p->car->pos[2] += 120;
                        fxExplosion96(p->car->pos, 0);
                        fxSmokeA48(&cur, 0);
                        p->car->pos[2] -= 120;
                        if (p->hit >= 500 && p->hit < 6000)
                            p->car->pos[2] = worldGroundHeightAt(p->car->pos);
                        groundFireSpawn(p->car->pos);
                    }
                }
            } else {
                if (p->type == 14)
                    HIT_SOUND(&cur);
                else if (p->type == 61)
                    HIT_SPARKS(&cur);
                else
                    fxExplosion24(&cur, 0);
                if (p->type == 15) {
                    fxExplosion96(p->car->pos, 0);
                    fxSmokeA48(p->car->pos, 0);
                    if (obj->kind >= 500 && obj->kind < 521)
                        p->car->pos[2] = worldGroundHeightAt(p->car->pos);
                    if (obj->kind != 3005)
                        groundFireSpawn(p->car->pos);
                }
            }
            fxRecordFree(id, 1);
            i = -1;
            if (obj == 0)
                weaponApplyHit(p->hit, 0, 0, p->damage, step, p->owner);
            else
                weaponApplyHit(p->hit, obj->kind, obj->sub, p->damage, step, p->owner);
            if (p->type == 12 || p->type == 11 || p->type == 15 || p->type == 10)
                soundStopInstance(p->type, p->id + 1);
        }
    }

    if (p->life == 0) {
        c = p->car;
        if (c->pos[2] <= 0)
            c->pos[2] = 0;
        if (p->type == 14)
            HIT_SOUND(&cur);
        else if (p->type == 61)
            HIT_SPARKS(&cur);
        else
            fxExplosion24(p->car->pos, 0);
        if (p->type == 15) {
            p->car->pos[2] += 120;
            fxExplosion96(p->car->pos, 0);
            p->car->pos[2] -= 120;
            groundFireSpawn(p->car->pos);
        }
        fxRecordFree(id, 0);
        if ((p->type == 12 || p->type == 11 || p->type == 10) && p->type != 15)
            soundStopInstance(p->type, p->id + 1);
    }
out:
    fxTrailUpdate(p, id);
}
