/* sound.h: shared prototypes (see docs/SIGNATURES.md). Plain C types only, so units
 * with their own u8/s32 typedefs (and C++ units) can include it. */
#ifndef TM2_SEMANTIC_SOUND_H
#define TM2_SEMANTIC_SOUND_H

/* --- prototypes (shared; definitions in src/semantic) --- */
#ifdef __cplusplus
extern "C" {
#endif

/* command_wrappers.cpp */
int soundFlush(void);
int soundPlay(int a, int b, int c, int d);
int soundPlayVariant(int a, int b, int c, int d, int e);
int soundPlayVariantEx(int a, int b, int c, int d, int e, int f);
int soundStartPlayerLoop(int a, int b, int c, int d, int e);
int soundStop(int a);
int soundStopInstance(int a, int b);
int soundStopPlayerLoop(int a, int b);
int soundStopSharedLoop(int a, int b);

/* global_accessors.c */
unsigned char audioGetDarkToothMusic(void);
void soundSetDeniedPending(unsigned char value);

/* misc_one_liners.c */
void soundLocateForView(int a, int b);
void soundLocateForView0(int a);

/* weapon_fire_specials.c */
unsigned char soundIsActive(int id);
void soundSetActive(int id, unsigned char value);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
