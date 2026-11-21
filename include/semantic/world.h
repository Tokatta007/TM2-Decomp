/* world.h: shared prototypes (see docs/SIGNATURES.md). Plain C types only, so units
 * with their own u8/s32 typedefs (and C++ units) can include it. */
#ifndef TM2_SEMANTIC_WORLD_H
#define TM2_SEMANTIC_WORLD_H

/* --- prototypes (shared; definitions in src/semantic) --- */
#ifdef __cplusplus
extern "C" {
#endif

/* area_damage.c */
void areaDamage(int *center, short int strength, short int big);

/* arena_setup.c */
void lightFlickerStart(int key, int val, unsigned char *color);

/* car_records.c */
void viewSetCameraPose(int i, int *pos, short *rot);

/* car_restart.c */
void worldSectionDisable(int id);

/* car_wheel_sway.c */
int worldSectionAt(int x, int y, int z);

/* engine_anim_hit_volumes.c */
void animControl(void *p, int mode);
void animPickPeriod(void *a, int on);
void animSetStep(void *s, unsigned int step);
void renderDrawModel(void *m, void *obj);

/* engine_clut_anim.c */
void *csAlloc(void);
void *csGetRoot(void);
void csListAdd(void *obj);
void csSetDrawEnable(void *obj, unsigned char v);

/* engine_math_near_pass.c */
int mathBlendAngle(int angle, int target, int pct);
int mathBlendValue(int value, int target, int pct);
void mathBoundAngle(int *a);
void mathCopyWords(int *dst, int *src, int n);
void mathMulMatVec2(short int *m, int *input, int *output);

/* engine_render_scene.c */
void lightSetEnvAmbient(int env, short r, short g, short b);
void lightSetEnvColor(int env, int l, short r, short g, short b);
void lightSetEnvDirection(int env, int l, short az, short el);

/* engine_sound_driver.c */
void taskMgrErrorStub(void *m);
void taskMgrRunFrame(void *m);

/* fx_debris_splash.c */
void worldWarpStart(int idx, void *src, unsigned char flag);

/* math_vec.c */
void mathMulMatVec(short int *m, int *input, int *output);
void mathMulTransVec(short int *m, int *input, int *output);

/* object_registry.c */
void objRegistrySetStep(int id, int index, int value);

/* target_actor_events.c */
void targetActorHit(int index, int *direction);

/* target_executor.c */
int targetDamage(int type, int instance, int amount);

/* target_level_object.c */
void *targetGetLevelObject(int index);

/* timer_frame.c */
unsigned int GetCurTics(void);
void SetFrameStart(void);
int timerGetFrameTicks(void);

/* track_vehicle_accessors.c */
int *worldTrackVehiclePos(int index);

/* view_system.c */
void viewSetPosition(int *src, int i);
void viewSetRotation(void *src, int i);

/* weapon_special_missiles.c */
void trackVehicleSurfaceSet(int n, int *pos, int size, int angle);

/* world_object_register.c */
unsigned char trackVehiclesShouldStop(void);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
