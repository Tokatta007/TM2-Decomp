/* @8010A7A0. M2C/IDA shape reconciled with the complete retail MIPS body.
 * Caller @800D82D4 ignores v0. The two local vectors are contiguous 12-byte
 * objects; byte/halfword actor fields must not become generic int stores.
 * External game/SDK callees remain dependencies until their own validation.
 */
#include "semantic/cars.h"
#include "semantic/car_numbers.h"   /* request 73 stage 2: TM2_SPECIAL_NUM_ID (a mod car's special numbers) */

struct TM2Emitter_A7A0 {
    short field_0;
    u16 field_2;
    u8 field_4[24];
    struct TM2Object *field_1C; /* car object the flame comes from */
};
struct TM2Point_A7A0 {
    s32 field_0;
    s32 field_4;
    s32 field_8[3];
};
typedef char tm2_emitter_size[(sizeof(struct TM2Emitter_A7A0) == 32) ? 1 : -1];
typedef char tm2_point_size[(sizeof(struct TM2Point_A7A0) == 20) ? 1 : -1];

/* @80184774..801847B4, within the retail CRT's zero-initialized BSS. */
#if TM2_MAX_FLAME_EMITTERS > TM2_RETAIL_FLAME_EMITTERS
extern struct TM2Emitter_A7A0 weaponFlameEmitters[TM2_MAX_FLAME_EMITTERS];   /* TM2_PORT_CARS: port_variant_arrays.c */
#else
struct TM2Emitter_A7A0 weaponFlameEmitters[2];
#endif
extern void mathMulTransVec(const void *, const s32 *, s32 *);
extern int rand(void);
#include "semantic/sound.h"
#ifdef TM2_PORT_CAR_SOUND
#include "semantic/car_sound.h"   /* request 33-3 (CAR_GROUPS.md 12) */
#endif
extern struct TM2Point_A7A0 *worldFlameMarkerGet(int);
extern int projectileHitTest(s32 *, int, int, int);
#include "semantic/weapon_calls.h"
extern void fxExplosion24(s32 *, u8 flag);
extern void fxExplosion48(s32 *, u8 flag);
extern void *fxFlameBurning(s32 *, int);
extern void fxFlamePair(s32 *, int);
extern void fxFlameStreamParticle(s32 *);
extern void fxSmokeA6(s32 *, int);
extern void fxSmokeA12(s32 *, u8 arg1);
extern int gameGetLevel(void);
extern void clutAnimStartSweep(int, int, int);

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)[0] - (r)[0]), MAX(ABS((l)[1] - (r)[1]), ABS((l)[2] - (r)[2]))) >> 3)
#define EMITTER weaponFlameEmitters[e]

/* @8010A7A0. Advance the two flame emitters: step a particle trail along the
 * emitter's facing, ignite the first car it touches and light any of the
 * three markers (worldFlameMarkerGet) it passes. Caller @800D82D4 ignores v0. */
void weaponFlameUpdate(void)
{
    s32 pos[3];
    s32 dir[3];
    u8 side;
    int actor_index;
    int e;
    s32 *origin;
    int active = 0;
    int step, k, hit_id;
    u8 count, hit;
    struct TM2Object *actor;
    struct TM2HumanRecord *human;
    struct TM2AIRecord *ai;
    struct TM2CarMotion *motion;
    struct TM2CarStatsBlock *stats;
    struct TM2CarState *state;
    struct TM2CarWeapons *weap;
    struct TM2CarInput *input;

    for (e = 0; e < TM2_MAX_FLAME_EMITTERS; ++e) {
        if (EMITTER.field_0 <= 0)
            continue;
        pos[0] = 0;
        pos[1] = 32;
        pos[2] = 0;
        EMITTER.field_0--;
        active++;
        if (EMITTER.field_0 <= 0) {
            active--;
#ifdef TM2_PORT_CAR_SOUND
            portCarFlameSoundEnd(EMITTER.field_1C);   /* request 33-3: this car's flame sound (bit 4) */
#endif
        }
        mathMulTransVec(EMITTER.field_1C->matrix, pos, dir);
        actor = EMITTER.field_1C;
        pos[0] = actor->pos[0] + dir[0] * 2;
        pos[1] = actor->pos[1] + dir[1] * 2;
        pos[2] = actor->pos[2] + dir[2];
        origin = actor->pos;
        hit = 0;
        count = EMITTER.field_2++;
        if (count >= 16)
            count = 15;

        for (step = 0; step < count; ++step) {
            pos[0] += dir[0];
            pos[1] += dir[1];
            pos[2] += 1 + dir[2];
            if (pos[2] < 8 && origin[2] > 0)
                pos[2] = 7;
            fxFlameStreamParticle(pos);
            if (rand() % 50 == 0)
                fxSmokeA6(pos, 0);
            if ((step & 2) && !hit) {
                hit_id = projectileHitTest(pos, EMITTER.field_1C->id, 20, -1);
                if (hit_id > 0) {
                    weaponApplyHit(hit_id, 0, 0, TM2_SPECIAL_NUM_ID(EMITTER.field_1C->id, TM2_SPN_DAMAGE, 1), dir,
                                   EMITTER.field_1C->id);
                    hit = 1;
                    if (carFindById(hit_id, &side, &actor_index) & 255) {
                        if (side) {
                            human = humanCarGet(actor_index);
                            motion = &human->motion; stats = &human->stats; state = &human->state;
                            weap = &human->weapons; input = &human->input;
                        } else {
                            ai = aiCarGet(actor_index);
                            motion = &ai->motion; stats = &ai->stats; state = &ai->state;
                            weap = &ai->weapons; input = &ai->input;
                        }
                        if (!state->burning && !state->energy_shield && !state->in_water && !state->in_fire
                            && (!input->turbo || (input->turbo && weap->turbo < 50))) {
                            state->burning = 1;
                            stats->burn_time = TM2_SPECIAL_BURN_START(EMITTER.field_1C->id);
                            weap->fire_effect = fxFlameBurning(motion->pos, 1);
                            fxSmokeA12(motion->pos, 0);
                            fxExplosion24(motion->pos, 0);
                        }
                    }
                }
            }

            for (k = 1; k < 4; ++k) {
                if (DISTANCE(pos, worldFlameMarkerGet(k)->field_8) < 35 && worldFlameMarkerGet(k)->field_0 == 0) {
                    worldFlameMarkerGet(k)->field_0 = 1;
                    fxFlamePair(worldFlameMarkerGet(k)->field_8, 0);
                    fxExplosion48(worldFlameMarkerGet(k)->field_8, 0);
                    if (gameGetLevel() == 3)
                        clutAnimStartSweep(5100, k, 0);
                    else
                        clutAnimStartSweep(5100, k, 2);
                }
            }
        }
    }
#ifdef TM2_PORT_CAR_SOUND
    portCarFlameSoundIdle(active);      /* request 33-3: bit 4 clear = the retail stop below */
#else
    if (active == 0 && (soundIsActive(47) & 255))
        soundStop(47);
#endif
}
