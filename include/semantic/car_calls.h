/* car_calls.h: prototypes of the shared car functions (see docs/SIGNATURES.md). Plain C
 * types only, so units with their own u8/s32 typedefs (and C++ units) can include
 * it directly; cars.h / weapons.h include it too. */
#ifndef TM2_SEMANTIC_CAR_CALLS_H
#define TM2_SEMANTIC_CAR_CALLS_H

/* --- prototypes (shared; definitions in src/semantic) --- */
struct TM2AIRecord; struct TM2CarStatsBlock; struct TM2HumanRecord; struct TM2Settings;
#ifdef __cplusplus
extern "C" {
#endif

/* ai_car_start.c */
void aiCarStart(struct TM2AIRecord *ai, int type, unsigned char reset_weapons);
void carSpinOutByObject(int idx, int spin, int other);

/* ai_route_helpers.c */
void carBodyObjectHide(int id);
unsigned char carHasMeleeSpecial(int type);
void carStatsSetTopSpeed(struct TM2CarStatsBlock *stats, int value);

/* car_control.c */

/* car_destroyed.c */
void carDestroyed(void *car, unsigned char human);

/* car_freeze_dispatch.c */
int carFreezeSetState(int id, int value);

/* car_input.c */
void carSkidSmokeUpdate(void *car, unsigned char human);

/* car_is_selectable.c */
int carIsSelectable(int value);

/* car_motion.c */
void CarRotUpdate(void *car, unsigned char human);
void carMoveByVelocity(void *car, unsigned char human);
void carSpinStop(void *car, unsigned char human);
void carUpdateSidewaysFlag(void *car, unsigned char human);

/* car_parts_spin.c */
char *carFileCode(struct TM2Settings *settings, unsigned int id);
void carPartsAnimate(int kind, int amount);

/* car_records.c */
unsigned char aiBossDefeated(void);
void *aiGetTargetCar(int i, unsigned char *human);
unsigned char carFindById(int id, unsigned char *human, int *index);
void *carFromSlot(int i, unsigned char *human);

/* car_restart.c */
void carBuildCollisionBox(void *car, unsigned char human);
void carRestart(struct TM2HumanRecord *human, int type, unsigned char reset_weapons);

/* car_spin.c */
void carSpinOut(void *car, int human, int kind, int keep_motion);

/* car_tumble.c */

/* car_wheel_sway.c */
void carSpinOutById(int id, short angle, unsigned char back);

/* global_accessors.c */
unsigned char gameIsAttractDemo(void);

/* timer_frame.c */
void carSetResetWeapons(signed char value, int index);

/* weapon_fire_missiles.c */
void carShakeStart(void *car, int level, int axes, int time, unsigned char side);

/* weapon_special_missiles.c */
void carDamageByObject(int key, int a1, int a2);

/* world_object_register.c */
void carScrapeSparks(void *car, unsigned char human);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
