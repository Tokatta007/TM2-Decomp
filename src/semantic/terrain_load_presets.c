/* Small game helpers (per-track presets 801007A0..801018F4). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/*
 * Per-track terrain loaders.  Each track loader in arena_load_register.c
 * (arenaLoadLosAngeles..) calls a triple: the route loader (ai_route_loaders.c),
 * one TERRAIN\<track>.GRP loader and one TERRAIN\<track>.RMP loader below.
 * The file is read into the shared scratch buffer fileioWorkBuffer and copied out.
 *
 * .RMP: ground surfaces, 6300 bytes = 225 x 28-byte TM2Surface records into
 *       worldSurfaces; sets the surface count terrainSurfaceCount and D_80180AD0.
 *       (FWY/DISH/ICE.RMP are in terrain_load.c.)
 * .GRP: 6036 bytes into worldSectionGrpFile; sets the count terrainSectionCount.
 */

extern u8 *fileioWorkBuffer;              /* file scratch buffer */
#pragma tm2_absolute fileioWorkBuffer
extern s32 terrainSurfaceCount;              /* RMP surface count */
extern s32 D_80180AD0;
extern s32 terrainSectionCount;              /* GRP record count */
extern u8 worldSurfaces[];             /* RMP surfaces (6300 bytes) */
extern u8 worldSectionGrpFile[];             /* GRP data (6036 bytes) */
extern s32 fileioLoadFileIntoRam(void *, const char *);

extern char strTerrainLavaRmp[];           /* "TERRAIN\LAVA.RMP" */
extern char strTerrainParisRmp[];           /* "TERRAIN\PARIS.RMP" */
extern char strTerrainHkongRmp[];           /* "TERRAIN\HKONG.RMP" */
extern char strTerrainRoofRmp[];           /* "TERRAIN\ROOF.RMP" */
extern char strTerrainDenRmp[];           /* "TERRAIN\DEN.RMP" */
extern char strTerrainSwampRmp[];           /* "TERRAIN\SWAMP.RMP" */
extern char strTerrainCybRmp[];           /* "TERRAIN\CYB.RMP" */
extern char strTerrainSroofRmp[];           /* "TERRAIN\SROOF.RMP" */
extern char strTerrainFwyGrp[];           /* "TERRAIN\FWY.GRP" */
extern char strTerrainDishGrp[];           /* "TERRAIN\DISH.GRP" */
extern char strTerrainIceGrp[];           /* "TERRAIN\ICE.GRP" */
extern char strTerrainLavaGrp[];           /* "TERRAIN\LAVA.GRP" */
extern char strTerrainParisGrp[];           /* "TERRAIN\PARIS.GRP" */
extern char strTerrainHkongGrp[];           /* "TERRAIN\HKONG.GRP" */
extern char strTerrainRoofGrp[];           /* "TERRAIN\ROOF.GRP" */
extern char strTerrainDenGrp[];           /* "TERRAIN\DEN.GRP" */
extern char strTerrainSwampGrp[];           /* "TERRAIN\SWAMP.GRP" */
extern char strTerrainCybGrp[];           /* "TERRAIN\CYB.GRP" */
extern char strTerrainSroofGrp[];           /* "TERRAIN\SROOF.GRP" */

#define LOAD_RMP(count, extra, name)                           \
    terrainSurfaceCount = (count);                                      \
    D_80180AD0 = (extra);                                      \
    fileioLoadFileIntoRam(fileioWorkBuffer, name);                   \
    __builtin_memcpy(worldSurfaces, fileioWorkBuffer, 6300)

#define LOAD_GRP(count, name)                                  \
    terrainSectionCount = (count);                                      \
    fileioLoadFileIntoRam(fileioWorkBuffer, name);                   \
    __builtin_memcpy(worldSectionGrpFile, fileioWorkBuffer, 6036)

/* @801007A0: LAVA.RMP, 206 surfaces (track loader arenaLoadAmazonia) */
void __attribute__((section(".text_801007A0"))) terrainLoadRmpLava(void) { LOAD_RMP(206, 0, strTerrainLavaRmp); }

/* @801008A8: PARIS.RMP, 67 surfaces (arenaLoadParis) */
void __attribute__((section(".text_801008A8"))) terrainLoadRmpParis(void) { LOAD_RMP(67, 0, strTerrainParisRmp); }

/* @801009B0: HKONG.RMP, 200 surfaces, D_80180AD0 = 2 (arenaLoadHongKong) */
void __attribute__((section(".text_801009B0"))) terrainLoadRmpHkong(void) { LOAD_RMP(200, 2, strTerrainHkongRmp); }

/* @80100ABC: ROOF.RMP, 26 surfaces (arenaLoadNewYork) */
void __attribute__((section(".text_80100ABC"))) terrainLoadRmpRoof(void) { LOAD_RMP(26, 0, strTerrainRoofRmp); }

/* @80100BC4: DEN.RMP, 8 surfaces (arenaLoadHolland in view_watch_ai.c) */
void __attribute__((section(".text_80100BC4"))) terrainLoadRmpDen(void) { LOAD_RMP(8, 0, strTerrainDenRmp); }

/* @80100CCC: SWAMP.RMP, 50 surfaces (arenaLoadSuicideSwamp) */
void __attribute__((section(".text_80100CCC"))) terrainLoadRmpSwamp(void) { LOAD_RMP(50, 0, strTerrainSwampRmp); }

/* @80100DD4: CYB.RMP, 46 surfaces (arenaLoadCyburbia) */
void __attribute__((section(".text_80100DD4"))) terrainLoadRmpCyb(void) { LOAD_RMP(46, 0, strTerrainCybRmp); }

/* @80100EDC: SROOF.RMP, 10 surfaces (arenaLoadRoofTops) */
void __attribute__((section(".text_80100EDC"))) terrainLoadRmpSroof(void) { LOAD_RMP(10, 0, strTerrainSroofRmp); }

/* @80100FE4: FWY.GRP, count 19 (arenaLoadLosAngeles) */
void __attribute__((section(".text_80100FE4"))) terrainLoadGrpFwy(void) { LOAD_GRP(19, strTerrainFwyGrp); }

/* @801010CC: DISH.GRP, count 12 (arenaLoadMoscow) */
void __attribute__((section(".text_801010CC"))) terrainLoadGrpDish(void) { LOAD_GRP(12, strTerrainDishGrp); }

/* @801011B4: ICE.GRP, count 35 (arenaLoadAntarctica) */
void __attribute__((section(".text_801011B4"))) terrainLoadGrpIce(void) { LOAD_GRP(35, strTerrainIceGrp); }

/* @8010129C: LAVA.GRP, count 23 (arenaLoadAmazonia) */
void __attribute__((section(".text_8010129C"))) terrainLoadGrpLava(void) { LOAD_GRP(23, strTerrainLavaGrp); }

/* @80101384: PARIS.GRP, count 22 (arenaLoadParis) */
void __attribute__((section(".text_80101384"))) terrainLoadGrpParis(void) { LOAD_GRP(22, strTerrainParisGrp); }

/* @8010146C: HKONG.GRP, count 25 (arenaLoadHongKong) */
void __attribute__((section(".text_8010146C"))) terrainLoadGrpHkong(void) { LOAD_GRP(25, strTerrainHkongGrp); }

/* @80101554: ROOF.GRP, count 24 (arenaLoadNewYork) */
void __attribute__((section(".text_80101554"))) terrainLoadGrpRoof(void) { LOAD_GRP(24, strTerrainRoofGrp); }

/* @8010163C: DEN.GRP, count 7 (arenaLoadHolland in view_watch_ai.c) */
void __attribute__((section(".text_8010163C"))) terrainLoadGrpDen(void) { LOAD_GRP(7, strTerrainDenGrp); }

/* @80101724: SWAMP.GRP, count 2 (arenaLoadSuicideSwamp) */
void __attribute__((section(".text_80101724"))) terrainLoadGrpSwamp(void) { LOAD_GRP(2, strTerrainSwampGrp); }

/* @8010180C: CYB.GRP, count 2 (arenaLoadCyburbia) */
void __attribute__((section(".text_8010180C"))) terrainLoadGrpCyb(void) { LOAD_GRP(2, strTerrainCybGrp); }

/* @801018F4: SROOF.GRP, count 14 (arenaLoadRoofTops) */
void __attribute__((section(".text_801018F4"))) terrainLoadGrpSroof(void) { LOAD_GRP(14, strTerrainSroofGrp); }
