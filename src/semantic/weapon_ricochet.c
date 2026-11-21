/* 8010CC54. Names stay address-based. */
#include "semantic/weapons.h"

#define HUMAN(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

/* Rolling-bomb shot records (weapon special 6, include/semantic/weapons.h),
 * @80184888 inside the weaponRicochetPool effect table owned by asset_pools.c. */
extern struct TM2RicochetTable weaponRicochetPool;

struct G19Hit {
    s32 f0;
    s32 hit;           /* +0x04 */
    s32 normal[3];     /* +0x08 */
    s32 depth;         /* +0x14 */
    s32 f18[2];        /* +0x18 [0] the object whose volume was hit (hdpPassRun: the level root, or a scene
                          object such as a car; request 45) */
    s16 f20;           /* +0x20 */
    u16 kind;          /* +0x22 surface kind */
    u8 rest[0x188 - 0x24];
};
struct G19Triple { s32 v[3]; };
struct G19Object { u8 f0[4]; s16 state; u8 f6[50]; struct TM2SceneObject *actor; };

extern struct G19Triple weaponHitSpinDefault;   /* {0, 0, 4095} (weapon_basic.c) */
extern u8 weaponShakeColor[];

extern s32 humanCarCount(void);
extern s32 aiCarCount(void);
extern struct TM2SceneObject *humanCarObject(s32);
extern struct TM2SceneObject *aiCarObject(s32);
extern void *RotMatrixYXZ(s16 *, void *);
extern void mathMulTransVec(void *, s32 *, s32 *);
extern void *memcpy(void *, const void *, int);
extern void hdpCollideObject(void *, void *, s32, void *, void *);
extern s32 targetDamage(s32, s32, s32);
extern void fxExplosion24(void *, u8 flag);
extern void mathReflectHeading(s32 *, s32 *, void *);
extern void lightFlickerStart(void *, s32, u8 *);
extern s32 worldGroundHeightAt(s32 *);
extern s32 mathBlendValue(s32, s32, s32);
extern void mathBoundSAngle(s16 *);
extern int VectorNormal(s32 *, s32 *);
extern void carKnockBack(void *, u8 human, s32 *, s32);
extern struct G19Object *targetActorsGet(void);
extern void targetActorHit(s32, struct G19Triple *);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlayVariantEx(s32, s32, s32, s32, s32, s32);
extern u8 soundIsActive(s32);
extern s32 soundStop(s32);

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)
#define SHOT weaponRicochetPool.shots[i]
#define POS(p) ((p)->pos)
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"
/* request 41 bit 18 (BALANCE.md 12.6): the owner only once the shot has been outside his box */
#define OWNER_HIT(retail, car_pos) portBalanceRicochetOwner(&SHOT, (retail), DISTANCE(POS(SHOT.obj), (car_pos)))
#else
#define OWNER_HIT(retail, car_pos) (retail)
#endif

/* Detonate: stop the shot, hide its object, lift it and start the blast effect. */
#define EXPLODE \
    SHOT.life = 0; \
    SHOT.obj->visible = 0; \
    SHOT.obj->pos[2] += 80; \
    fxExplosion24(SHOT.obj->pos, 0)

/* @8010CC54. Per-tick update of the ten rolling bombs (weapon special 6):
 * each live shot ages (power grows with age, minimum 7), rolls forward along
 * its heading, bounces off walls (reversing its heading, doubling its power on
 * the first bounce), detonates on breakable scenery (player shots only),
 * follows the ground, explodes on contact with a car or AI car (not its own
 * owner during the first few ticks) and knocks nearby spinner objects. A live
 * shot keeps its looping sound (18, channel i) running; when every shot is
 * dead the sound is stopped. The life and bounce counters are stored through
 * a pointer ((&SHOT)->) as retail does; everything else is plain indexing. */
void __attribute__((section(".text_8010CC54"))) weaponRicochetUpdate(void)
{
    s16 ang[4];
    s32 norm[4];
    struct G19Hit hit;
    s16 turn;
    s32 i, count0, count1;
    u8 idle;
    s32 j, ground, mtap;
    struct G19Object *objs;
    struct TM2SceneObject *lookup;
    void *car;              /* human or AI record */

    idle = 1;
    count0 = humanCarCount();
    count1 = aiCarCount();
    for (i = 0; i < TM2_MAX_RICOCHETS; i++) {
        if (SHOT.life <= 0)
            continue;
        (&SHOT)->life = SHOT.life - 1;
        idle = 0;
        SHOT.power = (400 - SHOT.life) / 20;
        SHOT.power = SHOT.power < 7 ? 7 : SHOT.power;
        ang[0] = 0;
        ang[1] = 0;
        ang[2] = SHOT.obj->rot[2];
        RotMatrixYXZ(ang, &SHOT.obj->matrix);
        mathMulTransVec(&SHOT.obj->matrix, SHOT.offset, SHOT.vel);
        memcpy(SHOT.prev, SHOT.obj->pos, 12);
        SHOT.obj->pos[0] += SHOT.vel[0];
        SHOT.obj->pos[1] += SHOT.vel[1];
        SHOT.obj->pos[2] += SHOT.vel[2];
        hdpCollideObject(SHOT.obj, &hit, 1, &SHOT.obj->matrix, SHOT.prev);
        if ((u32)(hit.kind - 667) < 2 || (u32)(hit.kind - 401) < 40)
            hit.hit = 0;
#ifdef TM2_PORT_BALANCE
        /* request 45 bit 18 (BALANCE.md 14.2): the owner's car box is no wall until the shot has left his box */
        if (portBalanceRicochetPass(&SHOT, (void *)hit.f18[0]))
            hit.hit = 0;
#endif
        if (hit.hit) {
            if (SHOT.owner < 50 && (u32)(hit.kind - 500) < 21) {
                targetDamage((s16)hit.kind, hit.f20, SHOT.power);
                EXPLODE;
            }
            if (hit.hit) {
                mathReflectHeading(SHOT.vel, hit.normal, &SHOT.obj->rot[2]);
                (&SHOT)->bounces = SHOT.bounces + 1;
                if (SHOT.bounces == 1) {
                    SHOT.power = (u8)SHOT.power << 1;
                    lightFlickerStart(SHOT.obj, 100, weaponShakeColor);
                }
                hit.depth += 20;
                SHOT.obj->pos[0] += hit.depth * -hit.normal[0] / 4096;
                SHOT.obj->pos[1] += hit.depth * -hit.normal[1] / 4096;
                SHOT.obj->pos[2] += hit.depth * -hit.normal[2] / 4096;
                hdpCollideObject(SHOT.obj, &hit, 1, &SHOT.obj->matrix, &SHOT.prev);
                if ((u32)(hit.kind - 667) < 2)
                    hit.hit = 0;
#ifdef TM2_PORT_BALANCE
                if (portBalanceRicochetPass(&SHOT, (void *)hit.f18[0]))
                    hit.hit = 0;
#endif
                if (hit.hit) {
                    SHOT.obj->pos[0] -= SHOT.vel[0];
                    SHOT.obj->pos[1] -= SHOT.vel[1];
                    SHOT.obj->pos[2] -= SHOT.vel[2];
                    SHOT.obj->rot[2] += 2048;
                }
            }
        }
        ground = worldGroundHeightAt(POS(SHOT.obj)) + 16;
        if (SHOT.obj->pos[2] < ground)
            SHOT.obj->pos[2] = ground;
        else
            SHOT.obj->pos[2] = mathBlendValue(SHOT.obj->pos[2], ground, 70);

        for (j = 0; j < count0; j++) {
            lookup = humanCarObject(j);
            car = humanCarGet(j);
            if (HUMAN(car)->stats.hp > 0 && (lookup->id != SHOT.owner || OWNER_HIT(SHOT.life < 398, HUMAN(car)->motion.pos))
                && DISTANCE(POS(SHOT.obj), HUMAN(car)->motion.pos) < 20) {
                EXPLODE;
                weaponApplyHit(lookup->id, 0, 0, SHOT.power, SHOT.vel, SHOT.owner);
                carSpinOut(car, 1, 4, 1);
                turn = SHOT.obj->rot[2] - HUMAN(car)->motion.rot[2];
                mathBoundSAngle(&turn);
                if ((u16)turn > 512 && (u16)turn < 1536)
                    HUMAN(car)->state.hit_from_side = 1;
                VectorNormal(SHOT.vel, norm);
                carKnockBack(car, 1, norm, 800);
            }
        }
        for (j = 0; j < count1; j++) {
            lookup = aiCarObject(j);
            car = aiCarGet(j);
            if (AI(car)->stats.hp > 0 && (lookup->id != SHOT.owner || OWNER_HIT(SHOT.life < 396, AI(car)->motion.pos))
                && DISTANCE(POS(SHOT.obj), AI(car)->motion.pos) < 20) {
                EXPLODE;
                weaponApplyHit(lookup->id, 0, 0, SHOT.power, SHOT.vel, SHOT.owner);
                carSpinOut(car, 0, 4, 1);
                turn = SHOT.obj->rot[2] - AI(car)->motion.rot[2];
                mathBoundSAngle(&turn);
                if (ABS(turn) > 512 && ABS(turn) < 1536)
                    AI(car)->state.hit_from_side = 1;
                VectorNormal(SHOT.vel, norm);
                carKnockBack(car, 0, norm, 800);
            }
        }
        objs = targetActorsGet();
        for (j = 0; j < 6; j++) {
            if (objs[j].state != -1) {
                struct G19Triple spin;
                spin = weaponHitSpinDefault;
                if (DISTANCE(POS(SHOT.obj), POS(objs[j].actor)) < 10)
                    targetActorHit(j, &spin);
            }
        }
        if (SHOT.life == 100)
            lightFlickerStart(SHOT.obj, 100, weaponShakeColor);
        if (SHOT.life == 0) {
            if (SHOT.obj->visible) {
                SHOT.obj->visible = 0;
                SHOT.obj->pos[2] += 80;
                fxExplosion24(SHOT.obj->pos, 0);
            }
        } else {
            if (viewGetSplitMode() == 0)
                soundLocateForView0(SHOT.obj->pos);
            else
                soundLocateForView(SHOT.obj->pos, -1);
            mtap = PadChkMtap();
            soundPlayVariantEx(18, mtap, soundGetListenerPan(), 99, i, 0);
        }
    }
    if (idle && soundIsActive(18))
        soundStop(18);
}
