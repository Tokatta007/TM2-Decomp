/* Per-track .RMP terrain loaders for Freeway, Dish and Ice (siblings are in terrain_load_presets.c).
 * Each sets the surface count, loads the file into the scratch buffer fileioWorkBuffer and copies
 * the 6300 bytes (225 x 28-byte surface records) into worldSurfaces. */
typedef unsigned char u8;
typedef signed int s32;

extern u8 *fileioWorkBuffer;              /* file scratch buffer */
#pragma tm2_absolute fileioWorkBuffer
extern s32 terrainSurfaceCount;              /* RMP surface count */
extern s32 D_80180AD0;
extern u8 worldSurfaces[];             /* RMP surfaces (6300 bytes) */
extern s32 fileioLoadFileIntoRam(void *, const char *);

extern char strTerrainFwyRmp[];           /* "TERRAIN\FWY.RMP" */
extern char strTerrainDishRmp[];           /* "TERRAIN\DISH.RMP" */
extern char strTerrainIceRmp[];           /* "TERRAIN\ICE.RMP" */

#define LOAD_RMP(count, extra, name)                           \
    terrainSurfaceCount = (count);                                      \
    D_80180AD0 = (extra);                                      \
    fileioLoadFileIntoRam(fileioWorkBuffer, name);                   \
    __builtin_memcpy(worldSurfaces, fileioWorkBuffer, 6300)

/* @80100488: FWY.RMP, 128 surfaces */
void __attribute__((section(".text_80100488"))) terrainLoadRmpFwy(void) { LOAD_RMP(128, 0, strTerrainFwyRmp); }

/* @80100590: DISH.RMP, 56 surfaces */
void __attribute__((section(".text_80100590"))) terrainLoadRmpDish(void) { LOAD_RMP(56, 0, strTerrainDishRmp); }

/* @80100698: ICE.RMP, 141 surfaces */
void __attribute__((section(".text_80100698"))) terrainLoadRmpIce(void) { LOAD_RMP(141, 0, strTerrainIceRmp); }
