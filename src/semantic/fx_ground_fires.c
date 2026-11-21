/* @8010C160. IDA/M2C structure checked against retail load/store widths.
 * In particular, expiration writes a HALFWORD at effect+0xC, not an int.
 * The old draft truncated actor pointers where the raw truncates timer values.
 */
#include "semantic/cars.h"   /* u8..u32, TM2Object, TM2HumanRecord / TM2AIRecord */

/* Burning-effect record (fxFlameBurning); writing 0 to the halfword +0xC
 * ends it. */
struct TM2FireFx_C160 {
    u8 pad_00[12];
    short life;             /* +0x0C */
};

struct TM2Area_C160 {
    signed char field_0;
    u8 field_1[3];
    u16 field_4;
    u8 field_6[2];
    s32 field_8[3];
    struct TM2FireFx_C160 *field_14[5];
};
struct TM2Object_C160 {
    u8 field_0[4];
    short field_4;
    u8 field_6[50];
    struct TM2Object *field_38;
};
struct TM2Point_C160 { s32 field_0, field_4, field_8[3]; };
typedef char tm2_area_size[(sizeof(struct TM2Area_C160) == 40) ? 1 : -1];
typedef char tm2_object_size[(sizeof(struct TM2Object_C160) == 60) ? 1 : -1];

/* @801842C4..80184774; next object is the A7A0 emitter pool. */
struct TM2Area_C160 groundFires[30];

extern int rand(void);
extern struct TM2Point_C160 *worldFlameMarkerGet(int);
extern void fxExplosion48(s32 *, u8 flag);
extern void *fxFlameBurning(s32 *, int);
extern void fxFlamePair(s32 *, int);
extern struct TM2Object *humanCarObject(int);
extern struct TM2Object *aiCarObject(int);
extern int humanCarCount(void);
extern int aiCarCount(void);
extern struct TM2Object_C160 *targetActorsGet(void);
#include "semantic/world.h"
extern int gameGetLevel(void);
extern void clutAnimStartSweep(int, int, int);

#define HUMAN_OF(p) ((struct TM2HumanRecord *)(p))
#define AI_OF(p) ((struct TM2AIRecord *)(p))

struct TM2Triple { s32 v[3]; };
extern struct TM2Triple weaponHitSpinDefault;   /* {0, 0, 4095} */

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)
#define AREA groundFires[i]

/* @8010C160. Age the 30 burning areas: each live area ignites cars, AI cars,
 * spinners and markers within range; burning cars and AI cars then cool down
 * (and slowly lose health). The return value is unset, as in retail. Each
 * inner loop works on its own copy of the area-centre pointer. */
void groundFiresUpdate(void)
{
    int count0 = humanCarCount();
    int count1 = aiCarCount();
    int i, j;
    struct TM2Object *object;
    void *actor;            /* human or AI record */
    struct TM2FireFx_C160 *effect;
    struct TM2Triple spin;
    struct TM2Object_C160 *objects;
    s32 *center;   /* &groundFires[i].field_8, stepped with i */

    for (i = 0, center = groundFires[0].field_8; i < 30;
         center = (s32 *)((u8 *)center + sizeof(struct TM2Area_C160)), ++i) {
        if (!AREA.field_0)
            continue;
        if ((short)AREA.field_4++ >= 100) {
            AREA.field_0 = 0;
            for (j = 0; j < 5; ++j) {
                if (AREA.field_14[j])
                    AREA.field_14[j]->life = 0;
            }
            continue;
        }
        if ((short)AREA.field_4 < 21)
            continue;

        {
            s32 *c;
            j = 0;
            if (j < count0) {
                c = center;
                do {
                    object = humanCarObject(j);
                    actor = humanCarGet(j);
                    if (DISTANCE(c, object->pos) < 25
                        && !HUMAN_OF(actor)->state.burning && !HUMAN_OF(actor)->state.energy_shield
                        && (!HUMAN_OF(actor)->input.turbo
                            || (HUMAN_OF(actor)->input.turbo && HUMAN_OF(actor)->weapons.turbo < 50))
                        && !HUMAN_OF(actor)->state.in_water && !HUMAN_OF(actor)->state.in_fire) {
                        HUMAN_OF(actor)->state.burning = 1;
                        HUMAN_OF(actor)->stats.burn_time = 0;
                        HUMAN_OF(actor)->weapons.fire_effect = fxFlameBurning(HUMAN_OF(actor)->motion.pos, 1);
                    }
                } while (++j < count0);
            }
        }
        {
            s32 *c;
            j = 0;
            if (j < count1) {
                c = center;
                do {
                    object = aiCarObject(j);
                    actor = aiCarGet(j);
                    if (DISTANCE(c, object->pos) < 35
                        && !AI_OF(actor)->state.burning && !AI_OF(actor)->state.in_water
                        && !AI_OF(actor)->state.in_fire) {
                        AI_OF(actor)->state.burning = 1;
                        AI_OF(actor)->stats.burn_time = 0;
                        AI_OF(actor)->weapons.fire_effect = fxFlameBurning(AI_OF(actor)->motion.pos, 1);
                    }
                } while (++j < count1);
            }
        }
        objects = targetActorsGet();
        {
            s32 *c;
            j = 0;
            c = center;
            for (; j < 6; ++j) {
                if (objects[j].field_4 == -1)
                    continue;
                spin = weaponHitSpinDefault;
                if (DISTANCE(c, objects[j].field_38->pos) < 35)
                    targetActorHit(j, spin.v);
            }
        }
        {
            s32 *c;
            j = 1;
            c = center;
            for (; j < 4; ++j) {
                if (DISTANCE(c, worldFlameMarkerGet(j)->field_8) < 35 && !worldFlameMarkerGet(j)->field_0) {
                    worldFlameMarkerGet(j)->field_0 = 1;
                    fxFlamePair(worldFlameMarkerGet(j)->field_8, 0);
                    fxExplosion48(worldFlameMarkerGet(j)->field_8, 0);
                    if (gameGetLevel() == 3)
                        clutAnimStartSweep(5100, j, 0);
                    else
                        clutAnimStartSweep(5100, j, 2);
                }
            }
        }
    }
    for (j = 0; j < count0; ++j) {
        humanCarObject(j);
        actor = humanCarGet(j);
        if (HUMAN_OF(actor)->state.burning) {
            if (HUMAN_OF(actor)->stats.damageable == 1 && !(rand() & 3) && HUMAN_OF(actor)->stats.hp >= 6)
                HUMAN_OF(actor)->stats.hp--;
            HUMAN_OF(actor)->stats.burn_time++;
            if (HUMAN_OF(actor)->stats.burn_time >= 100) {
                HUMAN_OF(actor)->state.burning = 0;
                effect = (struct TM2FireFx_C160 *)HUMAN_OF(actor)->weapons.fire_effect;
                if (effect)
                    effect->life = 0;
            }
        }
    }
    for (j = 0; j < count1; ++j) {
        aiCarObject(j);
        actor = aiCarGet(j);
        if (AI_OF(actor)->state.burning) {
            if (!(rand() & 3) && AI_OF(actor)->stats.hp >= 6 && AI_OF(actor)->active)
                AI_OF(actor)->stats.hp--;
            AI_OF(actor)->stats.burn_time++;
            if (AI_OF(actor)->stats.burn_time >= 100) {
                AI_OF(actor)->state.burning = 0;
                effect = (struct TM2FireFx_C160 *)AI_OF(actor)->weapons.fire_effect;
                if (effect)
                    effect->life = 0;
            }
        }
    }
}
