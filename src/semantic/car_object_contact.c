/* Game code (mixed). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/port_cars.h"   /* request 49: portStartPlacedMark */
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* request 66: portHumanSteerKeep */
#endif
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
#define DRV(p) (&((struct TM2AIDriveView *)(p))->drive)

extern u8 moscowEvents[];
#include "semantic/world.h"
extern void moscowEventStart(s32 kind);

/* |pos[k] - lane point k| for the lane point at rec+o (re-read on every use, as retail does). */
#define G9_ABSD(k, o) __builtin_abs(pos[k] - *(s32 *)(rec + (o) + (k) * 4))
#define G9_MAX(p, q) ((p) < (q) ? (q) : (p))
/* Largest-axis distance to the lane point at rec+o, >> 3, below 100 (800 units). The plain
   nested MAX of three inline abs() expressions is what retail compiled (each re-evaluated). */
#define G9_NEAR(o) ((G9_MAX(G9_ABSD(0, o), G9_MAX(G9_ABSD(1, o), G9_ABSD(2, o))) >> 3) < 100)

/* @800D4504: for each active random event 1..4, call moscowEventStart(i) when pos is on the
   event's track section or within 800 units (largest axis) of any of its four lane
   points. (Fixed 2026-10: the earlier version skipped events on pos's own section;
   retail branches straight to the call in that case.) Identical. */
void __attribute__((section(".text_800D4504"))) moscowEventsTrigger(s32 *pos)
{
    s32 i;
    s32 section;
    u8 *rec;

    section = worldSectionAt(pos[0], pos[1], pos[2]);
    for (i = 1; i < 5; i++) {
        rec = moscowEvents + i * 196;
        if (rec[0]) {
            if (section == *(s16 *)(rec + 12) || G9_NEAR(68) || G9_NEAR(80) ||
                G9_NEAR(92) || G9_NEAR(104))
                moscowEventStart(i);
        }
    }
}

/* Scripted rising/spinning event object at 801AB810 (updated by eiffelTowerUpdate). */
typedef struct {
    s8 active;      /* 810 */
    s8 spark_rate;  /* 811 */
    s8 done;        /* 812 */
    u8 pad_03;
    s16 spin_step;  /* 814 */
    s16 speed;      /* 816 */
    s16 time;       /* 818 */
    s16 start;      /* 81A */
    u8 *obj;        /* 81C */
    s32 emit[3];    /* 820 */
    s32 pos[3];     /* 82C */
    s16 rot[4];     /* 838: SVECTOR rotation, [0]=x, [2]=z */
} TM2RiseEvent;

extern TM2RiseEvent eiffelTowerState;
extern s32 eiffelSparkCenter[], eiffelDebrisOrigin[], eiffelTowerDebrisPos[], eiffelExplosionPoints[], eiffelDebrisBurstOffsets[];
extern s16 eiffelDebrisBurstDirs[], eiffelDebrisBurstAngles[];
extern s32 *eiffelExplosionPos;
extern void eiffelTowerSetFallenSurfaces(void);
extern void aiRoutePatchEiffelTower(void);
extern void eiffelTowerDebrisPoint(void *a, void *out);
extern void fxExplosion192(void *pos, u8 flag);
extern void fxSmokeA48(void *pos, s32);
extern void fxExplosion48(void *pos, u8 flag);
extern void fxExplosion96(void *pos, u8 flag);
extern s32 *mathJitterPos(void *pos, s32, s32, s32);
extern void fxSheetStart(s32 time, s32 r, s32 g, s32 b);
#include "semantic/sound.h"
extern void *RotMatrixYXZ(s16 *rot, void *m);
extern s32 rand(void);
extern void debrisBurstAt(s32, void *, s16 *, s16 *, s32 *, s32, s32, s32, s32);

/* @800D6800: per-frame update of the scripted rising/spinning event object (eiffelTowerState
   state): moves and spins it, finishes with a burst of effects and a screen flash at full
   rotation, randomly spawns sparks while running, and fires its three emitters on cue.
   rot is an SVECTOR (x at [0], z at [2]); indexing one array for both is what makes the
   store/% 5 scheduling match. Reading the fields through the record (not as separate
   scalars) is what makes speed and obj reload after the stores through obj. */
void __attribute__((section(".text_800D6800"))) eiffelTowerUpdate(void)
{
    s32 i;
    s16 t;
    s32 *src;
    s32 *dst;

    if (!eiffelTowerState.active)
        return;
    t = ++eiffelTowerState.time;
    if (--eiffelTowerState.spark_rate < 2)
        eiffelTowerState.spark_rate = 2;
    if (eiffelTowerState.start < t) {
        if (t < eiffelTowerState.start + 12) {
            *(s32 *)(eiffelTowerState.obj + 28) += 5;
            *(s32 *)(eiffelTowerState.obj + 32) += eiffelTowerState.speed;
            eiffelTowerState.speed -= 15;
        } else if (t < eiffelTowerState.start + 16) {
            *(s32 *)(eiffelTowerState.obj + 32) += eiffelTowerState.speed;
            eiffelTowerState.speed += 4;
        }
        eiffelTowerState.rot[2] = mathBlendAngle(eiffelTowerState.rot[2], 91, 90);
        eiffelTowerState.rot[0] += eiffelTowerState.spin_step;
        if (eiffelTowerState.time % 5 == 0)
            eiffelTowerState.spin_step++;
        if (eiffelTowerState.rot[0] > 1092) {
            eiffelTowerState.rot[0] = 1092;
            eiffelTowerState.active = 0;
            eiffelTowerState.done = 1;
            eiffelTowerSetFallenSurfaces();
            aiRoutePatchEiffelTower();
            for (i = 0; i < 8; i++) {
                eiffelTowerDebrisPoint(eiffelDebrisOrigin, eiffelTowerDebrisPos);
                fxExplosion192(eiffelTowerDebrisPos, 0);
                fxSmokeA48(eiffelTowerDebrisPos, 0);
            }
            for (i = 4; i < 9; i++) {
                fxExplosion192(&eiffelExplosionPoints[i * 3], 0);
                fxSmokeA48(&eiffelExplosionPoints[i * 3], 0);
            }
            fxSheetStart(3, 255, 200, 200);
            objRegistrySetStep(3200, 0, 1);
            soundPlay(62, 0, -640, 99);
            soundPlay(2, 0, -640, 99);
            soundPlay(19, 0, 640, 99);
            soundPlay(30, 0, 640, 99);
        }
        RotMatrixYXZ(eiffelTowerState.rot, eiffelTowerState.obj + 4);
    }
    if (rand() % eiffelTowerState.spark_rate == 0 && !eiffelTowerState.done) {
        eiffelExplosionPos = mathJitterPos(eiffelSparkCenter, 41, 81, 81);
        fxExplosion192(eiffelExplosionPos, 0);
        fxSmokeA48(eiffelExplosionPos, 0);
        eiffelExplosionPos = mathJitterPos(eiffelSparkCenter, 41, 81, 81);
        fxExplosion192(eiffelExplosionPos, 0);
        fxSmokeA48(eiffelExplosionPos, 0);
        src = eiffelDebrisOrigin;
        dst = eiffelTowerDebrisPos;
        eiffelTowerDebrisPoint(src, dst);
        if (rand() & 1)
            fxExplosion48(dst, 0);
        else
            fxExplosion96(dst, 0);
        fxSmokeA48(dst, 0);
        if (rand() & 1) {
            eiffelTowerDebrisPoint(src, dst);
            fxExplosion192(dst, 0);
            fxSmokeA48(dst, 0);
        }
    }
    if (eiffelTowerState.time == eiffelTowerState.start) {
        *(s32 *)(eiffelTowerState.obj + 32) += 200;
        for (i = 0; i < 4; i++) {
            fxExplosion192(&eiffelExplosionPoints[i * 3], 0);
            fxSmokeA48(&eiffelExplosionPoints[i * 3], 0);
        }
        for (i = 0; i < 3; i++)
            debrisBurstAt(eiffelTowerState.emit[i], eiffelTowerState.pos, &eiffelDebrisBurstDirs[i * 4], eiffelDebrisBurstAngles,
                         &eiffelDebrisBurstOffsets[i * 3], rand() / 1024, 75, 200, 0);
    }
}
extern u32 humanCarObject(s32 index);
extern u32 aiCarObject(s32 index);
extern void pickupApply(void *car, int human, s32 kind, s32 arg);
extern struct TM2FireEffect *fxFlameBurning(void *src, s32 mode);
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/dt_player.h"   /* request 118: portCarDtPlayerStart */
#endif
#endif

/* @800E06CC: apply/consume entry `slot` of a car's (human/AI) 32-byte pickup list by its
   kind (401-480, 666, 694-697, 500/520, 3002, 3600, turn markers when mode == 240);
   `apply` triggers the effect; returns 1 when the entry was consumed. */
u8 __attribute__((section(".text_800E06CC"))) carContactApplyObject(s32 slot, void *car, u8 human, u8 apply)
{
    struct TM2CarWeapons *s6;
    s32 ret;
    struct TM2CarState *s4;
    struct TM2CarStatsBlock *t;
    struct TM2CarContact *g;
    struct TM2CarMotion *s3;
    s32 mode;
    struct TM2CarInput *a;
    struct TM2ContactProbe *e;
    u8 flag;
    s32 j;
#ifdef TM2_PORT_DARK_TOOTH
    s32 key;
#endif

    ret = 0;

    if (human) {
        s4 = &HREC(car)->state;
        t = &HREC(car)->stats;
        g = &HREC(car)->contact;
        s3 = &HREC(car)->motion;
        s6 = &HREC(car)->weapons;
        humanCarObject(HREC(car)->player);
        mode = HREC(car)->type;
        a = &HREC(car)->input;
    } else {
        s4 = &AI(car)->state;
        g = &AI(car)->contact;
        t = &AI(car)->stats;
        s3 = &AI(car)->motion;
        s6 = &AI(car)->weapons;
        aiCarObject(AI(car)->index);
        mode = AI(car)->type;
        a = &AI(car)->input;
    }
    e = &g->probe[slot];
    if (e->active == 0)
        return ret; /* (a direct return here, not goto out, gives retail's s5/s6 choice) */
    if (e->material == 3002)
        goto consume;
    if (e->material >= 401 && e->material < 481) {
        ret = 1;
        e->active = 0;
        if (apply)
            pickupApply(car, human, e->material, e->surface);
        goto out;
    }
    if (e->material == 666) {
        ret = 1;
        e->active = 0;
#ifdef TM2_PORT_HAZARDS
        /* request 38 G3: fire off = the volume is consumed (as retail) but sets nothing alight */
        if (apply && !portArenaOff(TM2_ARENA_FIRE)
            && !s4->burning && !s4->energy_shield && (!a->turbo || (a->turbo && s6->turbo < 50))) {
#else
        if (apply && !s4->burning && !s4->energy_shield && (!a->turbo || (a->turbo && s6->turbo < 50))) {
#endif
            s4->burning = ret;
            t->burn_time = 0;
            s6->fire_effect = fxFlameBurning(s3->pos, 1);
        }
        goto out;
    }
    if (e->material >= 694 && e->material < 698) {
        ret = 1;
        if (apply) {
            flag = 0;
            if (human) {
                if ((g->ground[0].f02 && g->ground[1].f02 && g->ground[2].f02 && g->ground[3].f02) || mode == 224)
                    flag = 1;
            } else {
                if (g->ground[0].f02 || g->ground[1].f02 || g->ground[2].f02 || g->ground[3].f02)
                    flag = 1;
            }
            if (flag) {
                worldWarpStart(e->material - 694, car, human);
                for (j = 0; j < 4; j++)
                    g->ground[j].f02 = 0;
            }
        }
        goto out;
    }
    if (e->material == 500 || e->material == 520) {
        ret = 1;
        e->active = 0;
        if (apply)
            targetDamage(e->material, e->surface, 1);
        goto out;
    }
    if (e->material == 3600) {
        if (human)
            goto out;
    consume:
        ret = 1;
        e->active = 0;
        goto out;
    }
    if (mode != 240)
        goto out;
#ifdef TM2_PORT_DT_PLAYER
    /* request 36 (DARK_TOOTH_PLAYER.md): the boss markers are the computer's (retail calls carSpinOut with the AI
       layout on a human record here) */
    if (human)
        goto out;
#endif
#ifdef TM2_PORT_DARK_TOOTH
    /* request 33-2e (CAR_GROUPS.md 11): this Dark Tooth's own stage; with portCarOptions bit 6 the boss markers
       (Hong Kong data: warp 2, spin-outs) only on level 8 (SURVIVAL.md 1.5 #5) */
    if (TM2_PORT_BOSS_ANYWHERE && gameGetLevel() != 8)
        goto out;
    /* request 42 item 1: an ordinary Dark Tooth (bit 10) has no boss markers on any arena */
    if (!human && TM2_DT_ORDINARY(AI(car)->slot))
        goto out;
    key = TM2_CAR_KEY(human ? HREC(car)->slot : AI(car)->slot, mode);
#define BOSS_DEFEATED() portCarBossDefeated(key)
#else
#define BOSS_DEFEATED() aiBossDefeated()
#endif
    if (g->probe[0].active && g->probe[1].active && g->probe[2].active && g->probe[3].active) {
        if (!BOSS_DEFEATED())
            worldWarpStart(2, car, 0);
        e->active = 0;
        ret = 1;
    } else if (g->probe[0].active && g->probe[2].active) {
        if (!BOSS_DEFEATED()) {
            carSpinOut(car, 0, 4, 1);
            s3->body_rot[2] = s3->rot[2] + 1024;
            mathBoundAngle(&s3->body_rot[2]);
        }
    } else if (g->probe[1].active && g->probe[3].active) {
        if (!BOSS_DEFEATED()) {
            carSpinOut(car, 0, 4, 1);
            s3->body_rot[2] = s3->rot[2] - 1024;
            mathBoundAngle(&s3->body_rot[2]);
        }
    } else if (slot == 5) {
        ret = 1;
        e->active = 0;
    }
#undef BOSS_DEFEATED
    g->free_frames = 0;
    goto out;
out:
    return ret;
}
/* 24-byte track-node record (aiRouteNodes): position and up to 6 successor nodes. */
typedef struct {
    s32 x;
    s32 z;
    s16 y;
    s16 next[6];
    s8 count;
    s8 pad;
} G9Node;
typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} G9Matrix;
extern G9Node aiRouteNodes[];
extern s8 startNodeCount;
#pragma tm2_absolute startNodeCount
extern G9Matrix mathIdentityMatrix;
extern void carStateFlagsClear(struct TM2CarState *state);
extern s32 gameGetLevel(void);
extern s32 startNodePickRandom(s32);
extern s32 ratan2(s32, s32);
extern s32 carStatsSetSpeedMult(struct TM2CarStatsBlock *stats, s32);
extern void carStatsLoad(struct TM2CarStatsBlock *stats, s32);
extern void carWheelsSetStepHeight(struct TM2CarWheels *wheels, s32);
extern void wheelBlockReset(struct TM2CarWheels *wheels);
extern void wheelBlockSetSection(struct TM2CarWheels *wheels, s32);
extern void wheelBlockSetHeight(struct TM2CarWheels *wheels, s32);
extern void carBodyAttitudeUpdate(void *car, s32);
extern s32 carNearestRoutePoint(void *car, u8 human);

#define G9_SET_MATRIX(d, s)                    \
    {                                          \
        (d)->m[0][0] = (s).m[0][0];            \
        (d)->m[0][1] = (s).m[0][1];            \
        (d)->m[0][2] = (s).m[0][2];            \
        (d)->m[1][0] = (s).m[1][0];            \
        (d)->m[1][1] = (s).m[1][1];            \
        (d)->m[1][2] = (s).m[1][2];            \
        (d)->m[2][0] = (s).m[2][0];            \
        (d)->m[2][1] = (s).m[2][1];            \
        (d)->m[2][2] = (s).m[2][2];            \
        (d)->t[0] = (s).t[0];                  \
        (d)->t[1] = (s).t[1];                  \
        (d)->t[2] = (s).t[2];                  \
    }

/* @800FD6C4: place the human car on its start node (random successor sets heading), reset
   its motion state and orientation matrices, then rebuild its physics/contact state.
   Identical. G9_SET_MATRIX must be a plain { } block: a do { } while (0) wrapper adds
   loop notes that act as scheduling barriers and moved the a1 = 100 load for carStatsSetSpeedMult. */
void __attribute__((section(".text_800FD6C4"))) humanCarPlaceAtStart(struct TM2HumanRecord *human)
{
    s32 n;
    s32 y;
    s32 k;
    s32 m;
    s32 dx;
    s32 dz;
    G9Matrix *id;

    carStateFlagsClear(&human->state);
    human->f094 = 0;
    human->f095 = 0;
    human->motion.vel0 = 0;
    human->motion.speed = 0;
    human->motion.vel2 = 0;
    human->motion.pos[0] = 0;
    human->motion.pos[1] = 0;
    human->motion.rot[0] = 0;
    human->motion.rot[1] = 0;
    human->motion.rot[2] = 0;
    human->stats.probe_on = 1;
    carPushClear(&human->contact);
#ifdef TM2_PORT_CARS
    portStartPlacedMark(human, 1, 0);   /* request 49 (CAR_GROUPS.md 7.4): not counted by its own start pick */
#endif
    n = startNodePickRandom(gameGetLevel() == 3 ? 9 : startNodeCount);
#ifdef TM2_PORT_CARS
    portStartPlacedMark(human, 1, 1);   /* request 49: placed this round */
#endif
    human->motion.pos[0] = aiRouteNodes[n].x;
    human->motion.pos[1] = aiRouteNodes[n].z;
    y = aiRouteNodes[n].y;
    human->motion.pos[2] = human->stats.ride_height + y;
    if (aiRouteNodes[n].count > 0)
        k = rand() % aiRouteNodes[n].count;
    else
        k = 0;
    m = aiRouteNodes[n].next[k];
    dx = aiRouteNodes[m].x - aiRouteNodes[n].x;
    dz = aiRouteNodes[m].z - aiRouteNodes[n].z;
    if (dz == 0)
        dz = 1;
    id = &mathIdentityMatrix; /* identity matrix; taking it here matches retail's lui/addiu hoist */
    human->motion.rot[2] = ratan2(dx, dz);
#ifdef TM2_PORT_CARS
    if (portStartLinkPlace(human->motion.pos, &k, &y, &m)) {
        /* request 85 (SIXTEEN_CARS.md 5.5): a start along a route link: x and y there, on the ground, along it */
        human->motion.rot[2] = k;
        human->motion.pos[2] = human->stats.ride_height + y;
    }
#endif
    human->motion.spin0 = 0;
    human->motion.spin1 = 0;
    human->motion.yaw_rate = 0;
    human->motion.body_spin[0] = 0;
    human->motion.body_spin[1] = 0;
    human->motion.body_spin[2] = 0;
    human->motion.body_rot[0] = 0;
    human->motion.body_rot[1] = 0;
    human->motion.body_rot[2] = 0;
    G9_SET_MATRIX(&human->motion.rot_m, *id);
    G9_SET_MATRIX(&human->motion.body_rot_m, *id);
    G9_SET_MATRIX(&human->motion.fC8_m, *id);
    human->motion.fB0[0] = human->motion.pos[0];
    human->motion.fB0[1] = human->motion.pos[1];
    human->motion.fB0[2] = human->motion.pos[2];
    carStatsSetSpeedMult(&human->stats, 100);
    carStatsLoad(&human->stats, human->type);
#ifdef TM2_PORT_DT_PLAYER
    portCarDtPlayerStart(human);   /* request 118: a Dark Tooth's phase (stage 2's stats before the +20) */
#endif
    human->stats.max_hp += 20;
    human->stats.hp = human->stats.max_hp;
    carWheelsSetStepHeight(&human->wheels, human->type);
    wheelBlockReset(&human->wheels);
    wheelBlockSetSection(&human->wheels, worldSectionAt(human->motion.pos[0], human->motion.pos[1],
                                          human->motion.pos[2]));
    wheelBlockSetHeight(&human->wheels, y);
    carBodyAttitudeUpdate(human, 1);
    carSpinStop(human, 1);
    carBuildCollisionBox(human, 1);
    human->state.route_node = carNearestRoutePoint(human, 1);
    human->cam_pos[0] = human->motion.pos[0];
    human->cam_pos[1] = human->motion.pos[1];
    human->cam_pos[2] = human->motion.pos[2];
    human->cam_rot[0] = 0;
    human->cam_rot[1] = 0;
    human->cam_rot[2] = human->motion.rot[2];
}
extern void carBodySpinClear(struct TM2CarMotion *motion);
extern void carSetCollisionBounce(void *car, u8 human, s32 speed);
extern void mathBoundSAngle(s16 *angle);

/* @800E4754 (carRicochetOffObject): bounce a car off an object hit at relative angle `angle`:
   turn the heading away, pick a deflected steering target, optionally start a spin-out
   (spin != 0, fast enough), and bleed speed according to how head-on the hit was. */
void __attribute__((section(".text_800E4754"))) carRicochetOffObject(void *car, u8 human, u8 spin, s32 angle,
                                                              u32 kind)
{
    struct TM2CarMotion *mo;
    struct TM2CarState *ctl;
    struct TM2CarStatsBlock *phys;
    struct TM2CarContact *ct;
    struct TM2CarInput *in;
    s32 d;
    s32 r;
    s32 b;
    s32 c;
    s32 e;
    u8 ok;
    s32 v;
#ifdef TM2_PORT_HUMANS
    s32 keep;
#endif

    if (human) {
        mo = &HREC(car)->motion;
        ctl = &HREC(car)->state;
        phys = &HREC(car)->stats;
        ct = &HREC(car)->contact;
        in = &HREC(car)->input;
    } else {
        mo = &AI(car)->motion;
        ctl = &AI(car)->state;
        ct = &AI(car)->contact;
        phys = &AI(car)->stats;
        in = &AI(car)->input;
        DRV(car)->f453 = 1;
    }
    mo->body_rot[2] += angle;
    mathBoundAngle(&mo->body_rot[2]);
    ctl->f01 = 1;
    carBodySpinClear(mo);
    carSetCollisionBounce(car, human, angle);
    if (human && ctl->f06 && mo->speed < (s16)timerGetFrameTicks() * 570 / 100 * 32 &&
        (kind < 2 || kind == 4))
        ct->f04 = 1;
    else
        ct->f04 = 0;
    if (kind < 2 || kind == 4 || in->reverse) {
        if (ct->f04 || (e = __builtin_abs(angle)) < 682) {
            mo->body_rot[2] -= angle / 3;
            ct->f08 = mo->body_rot[2];
        } else if (e < 1365) {
            ct->f08 = mo->body_rot[2] - angle / 2;
        } else {
            ct->f08 = mo->body_rot[2] - 2048;
        }
        mathBoundSAngle(&ct->f08);
        ct->f03 = 1;
    }
    d = mo->rot[2] - mo->body_rot[2];
    mathBoundAngle(&d);
    if (ct->f04 && ctl->f06 && ((d < 0 && in->steer_dec) || (d > 0 && in->steer_inc))) {
        ct->f08 = mo->body_rot[2] - 2048;
        mathBoundSAngle(&ct->f08);
    }
#ifdef TM2_PORT_HUMANS
    /* request 66 (ANALOG_STEERING.md 2.6): with the human's portHumanControl bit 0 the held direction deflects the
     * car as going forward when reversing too (retail flips it); 0 for the AI cars and with the bit clear */
    keep = portHumanSteerKeep(car, human);
#define RICOCHET_REVERSING(ctl) ((ctl)->reversing && !keep)
#else
#define RICOCHET_REVERSING(ctl) ((ctl)->reversing)
#endif
    if (human && ctl->f06) {
        if (!RICOCHET_REVERSING(ctl)) {
            if (!in->steer_dec)
                ct->f08 = mo->rot[2] + 512;
            else
                ct->f08 = mo->rot[2] - 512;
        } else {
            if (in->steer_dec)
                ct->f08 = mo->rot[2] + 512;
            else
                ct->f08 = mo->rot[2] - 512;
        }
        mathBoundSAngle(&ct->f08);
        ct->f03 = 1;
    }
    v = 3;
    v = phys->mass * v / 35 + 3;
    if (spin && v < ct->hit.damage) {
        c = __builtin_abs(d);
        if (c >= 1707)
            ok = 0;
        else
            ok = c >= 342;
        if (ok) {
            ctl->f12 = 1;
            c = d;
            mo->f02 = mo->body_rot[2];
            phys->f20 = 0;
            if (c > 0) {
                mo->spin1 = 1;
                mo->f04 = mo->body_rot[2] + 1024;
            } else {
                mo->spin1 = -1;
                mo->f04 = mo->body_rot[2] - 1024;
            }
        }
    }
    r = 0;
    if (__builtin_abs(mo->speed) >= (s16)timerGetFrameTicks() * 760 / 100 * 32) {
        b = __builtin_abs(angle);
        if (b < 512)
            r = mo->speed / 4;
        else if (b < 1024)
            r = (s16)(mo->speed / 3);
        else
            r = mo->speed / 2;
    }
    mo->speed -= r;
    carPushTimerArm(ct);
    if (ct->hit.f01)
        ct->push = 0;
}
