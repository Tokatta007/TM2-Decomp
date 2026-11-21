/* fx.h: shared prototypes (see docs/SIGNATURES.md). Plain C types only, so units
 * with their own u8/s32 typedefs (and C++ units) can include it. */
#ifndef TM2_SEMANTIC_FX_H
#define TM2_SEMANTIC_FX_H

/* --- prototypes (shared; definitions in src/semantic) --- */
struct TM2FxSprite;
#ifdef __cplusplus
extern "C" {
#endif

/* ai_route_helpers.c */
void fxSmokeC6(int a, unsigned char flag);

/* ai_route_nodes.c */
void fxTrailUpdate(void *t, int index);

/* arena_load_register.c */
void fxRecordFree(short index, int unused);

/* fxSmokeA12.c */
void fxSmokeA12(int arg0, unsigned char arg1);

/* fxSmokeA48.c */
void fxSmokeA48(int arg0, int arg1);

/* fxSmokePuffBlue.c */
int *fxSmokePuffBlue(int arg0, int arg1);

/* fxSparkOrange.c */
void fxSparkOrange(int arg0, unsigned char arg1);

/* fx_debris_splash.c */
void fxSplashAt(int *pos, int d);

/* fx_explosions.c */
void fxExplosion12(int a, unsigned char flag);
void fxExplosion192(int a, unsigned char flag);
void fxExplosion24(int a, unsigned char flag);
void fxExplosion48(int a, unsigned char flag);
void fxExplosion96(int a, unsigned char flag);

/* fx_sprite_spawn.c */
struct TM2FxSprite *fxSpriteSpawn(int *pos, int kind, int b, int c, unsigned char *table, unsigned short count, unsigned char flag);
#ifdef __cplusplus
}
#endif
/* --- end of prototypes --- */

#endif
