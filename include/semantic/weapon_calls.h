/* weapon_calls.h: prototypes of the shared weapon functions (see docs/SIGNATURES.md). Plain C
 * types only, so units with their own u8/s32 typedefs (and C++ units) can include
 * it directly; cars.h / weapons.h include it too. */
#ifndef TM2_SEMANTIC_WEAPON_CALLS_H
#define TM2_SEMANTIC_WEAPON_CALLS_H

/* --- prototypes (shared; definitions in src/semantic) --- */
struct TM2CarMotion; struct TM2CarStatsBlock; struct TM2CarWeapons;
#ifdef __cplusplus
extern "C" {
#endif

/* ai_route_setup.c */
void weaponBlockInit(void *car, struct TM2CarWeapons *rec, unsigned char alt);

/* arena_load_register.c */
void slamClawSetAngle(int angle);

/* car_switch_registry.c */
void pickupSetVisible(int id, int index, int value);

/* car_wheel_sway.c */
void weaponSlamCarryTarget(int id, struct TM2CarStatsBlock *stats, struct TM2CarMotion *motion, unsigned char on, unsigned char left);

/* misc_one_liners.c */
void slamArmSetAngle(int angle);

/* screen_card_draw.c */
unsigned char weaponSlamFindTarget(void *car, unsigned char human);

/* weapon_fire_missiles.c */
short weaponFireColoredMissile(int a0, int a1, int *pos, int *vel, int a4, short *rgb);

/* weapon_fire_specials.c */
short weaponSpawnProjectile(int kind, int pos, int dir, int a3);

/* weapon_helpers.c */
void weaponCycleSelection(struct TM2CarWeapons *weap, int step, int count);

/* weapon_missile_trails.c */
int weaponApplyHit(int type, int a, int b, int strength, int *dir, int id);

/* weapon_slam_swing.c */
void weaponSlamSwingUpdate(void *car, unsigned char side);

/* weapon_tick.c */
void weaponTick(void *car, unsigned int human);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
