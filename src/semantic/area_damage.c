/* @8010B5C4: area (splash) damage around a point. C (PsyQ 3.6 CC1PSX,
 * -fvolatile). Distances are MAX(|dx|, MAX(|dy|, |dz|)) >> 3 with abs as the
 * compiler builtin and MAX as a macro whose operands are evaluated again. */
#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)

#include "semantic/weapons.h"
#include "semantic/car_numbers.h"   /* request 73 stage 2: TM2_SPECIAL_NUM_ID (a mod car's special numbers) */
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
/* bit 10 (BALANCE.md 3.12): Shadow's detonation (big == 0) min(retail, 25) + 3; remote bombs (big) retail */
#define SPLASH(d) portBalanceSplash((d), big)
#else
#define SPLASH(d) (d)
#endif

extern int humanCarCount(void);
extern int aiCarCount(void);
extern int carObjectId(int, u8 human);
extern void carApplyDamage(int, int, int *, int, int);
extern void fxExplosion48(int *, u8 flag);
extern struct TM2Breakable *targetPoolGet(void);
extern struct TM2Spinner *targetActorsGet(void);
#include "semantic/world.h"
struct TM2Triple { int v[3]; };

struct TM2Breakable {          /* 20 bytes, 50 records */
    short a, b;
    short health;
    short pad6;
    int position[3];
};

struct TM2Spinner {            /* 60 bytes, 6 records */
    char pad0[4];
    short state;               /* -1 = unused */
    char pad6[50];
    struct TM2SceneObject *object;     /* +56 */
};
extern struct TM2Triple weaponHitSpinDefault;

/* @8016292C: retail initialized vector, distinct from the other template. */
int areaDamageUpDir[3] __attribute__((section(".tm2_8016292C"))) = {0, 0, 4095};

#define HUMAN(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

/* request 73 stage 2: the blast's base damage (25, 50 on a surface polygon) of the firer `strength`'s Shadow
 * detonation; a remote bomb's (big 1) is retail's */
#ifdef TM2_PORT_SPECIAL_PROFILE
#define BLAST_BASE(retail) portSpecialBlast(strength, big, (retail))
#else
#define BLAST_BASE(retail) (retail)
#endif
void __attribute__((section(".text_8010B5C4"))) areaDamage(int *center, short strength, short big)
{
    void *actor;            /* human or AI record */
    struct TM2Breakable *breakables;
    struct TM2Spinner *spinners;
    int i, count, distance, radius, impact[3];
    struct TM2Triple spin;

    radius = TM2_SPECIAL_NUM(strength, TM2_SPN_BLAST_RADIUS, 75);   /* request 73: Shadow's (big 0) */
    if (big) radius = 100;
    count = humanCarCount();
    for (i = 0; i < count; i++) {
        actor = humanCarGet(i);
        distance = DISTANCE(HUMAN(actor)->motion.pos, center);
        if (distance < radius && HUMAN(actor)->stats.wreck_state != 4) {
            carSpinOut(actor, 1, 5, 0);
            if (HUMAN(actor)->state.on_surface)
                carApplyDamage(carObjectId(i, 1), SPLASH(BLAST_BASE(50) - (distance >> 2)), areaDamageUpDir, 2, strength);
            else
                carApplyDamage(carObjectId(i, 1), SPLASH(BLAST_BASE(25) - (distance >> 2)), areaDamageUpDir, 2, strength);
            impact[0] = HUMAN(actor)->motion.pos[0];
            impact[1] = HUMAN(actor)->motion.pos[1];
            impact[2] = HUMAN(actor)->motion.pos[2] + 200;
            fxExplosion48(impact, 0);
        }
    }
    count = aiCarCount();
    for (i = 0; i < count; i++) {
        actor = aiCarGet(i);
        distance = DISTANCE(AI(actor)->motion.pos, center);
        if (distance < radius && AI(actor)->stats.wreck_state != 4) {
            carSpinOut(actor, 0, 5, 0);
            if (AI(actor)->state.on_surface)
                carApplyDamage(carObjectId(i, 0), SPLASH(BLAST_BASE(50) - (distance >> 2)), areaDamageUpDir, 2, strength);
            else
                carApplyDamage(carObjectId(i, 0), SPLASH(BLAST_BASE(25) - (distance >> 2)), areaDamageUpDir, 2, strength);
            impact[0] = AI(actor)->motion.pos[0];
            impact[1] = AI(actor)->motion.pos[1];
            impact[2] = AI(actor)->motion.pos[2] + 200;
            fxExplosion48(impact, 0);
        }
    }
    breakables = targetPoolGet();
    for (i = 0; i < 50; i++) {
        if (breakables[i].health > 0 && DISTANCE(center, breakables[i].position) < 50)
            targetDamage(breakables[i].a, breakables[i].b, 25);
    }
    spinners = targetActorsGet();
    for (i = 0; i < 6; i++) {
        if (spinners[i].state != -1) {
            spin = weaponHitSpinDefault;
            if (DISTANCE(center, spinners[i].object->pos) < 50)
                targetActorHit(i, &spin);
        }
    }
}
