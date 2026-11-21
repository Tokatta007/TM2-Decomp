/* Game code (leftover small functions and round setup). Names stay address-based. */
#pragma tm2_rdata .tm2_800CF2B0
#include "semantic/players.h"
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/* ---- externs ---- */
typedef struct { s32 f0; u8 pad4[56]; } Rec60;
extern Rec60 bulletsLifeField[];

extern s32 carWorldObjectCount;          /* number of entries in carSlotOwners / carSlotObjects */
extern s32 carSlotOwners[];
extern s32 carSlotObjects[];

extern s32 aiCarTotal;          /* number of 2132-byte records at aiCars */
typedef struct { u8 pad0[0x5A0]; s32 hp; u8 pad5A4[0x2B0]; } Rec2132;  /* hp = stats.hp (cars.h) */
extern Rec2132 aiCars[];

typedef struct { s32 val; s32 key; } KeyVal;
extern KeyVal lightFlickerTimers[];

typedef struct {
    u8 b0;          /* +0x00 */
    u8 pad1;
    u8 b2;          /* +0x02 */
    u8 pad3[9];
    u8 *obj;        /* +0x0C */
    u8 pad10[12];
    s32 w1C;        /* +0x1C */
    s32 w20;        /* +0x20 */
    s32 w24;        /* +0x24 */
    s16 rot[3];     /* +0x28 */
} CamState;
extern CamState eiffelTowerState;
extern void objRegistrySetStep(s32, s32, s32);
extern void *RotMatrixYXZ(s16 *, void *);

typedef struct { s16 d; s16 i; void (*f)(u8 *, s32); } VtblEnt;
extern u8 *musicManagerObject;
extern s32 soundManagerObject;
extern s32 audioCdVolume;
#pragma tm2_absolute audioCdVolume
extern s32 audioVolumeClampMax;
#pragma tm2_absolute audioVolumeClampMax
extern s32 audioVolumeClampMin;
#pragma tm2_absolute audioVolumeClampMin
extern void SsSetSerialVol(char s_num, short voll, short volr);
extern void taskMgrStopAll(s32);
extern void audioClearDarkToothMusic(void);
extern void taskMgrErrorStub(s32);

typedef struct { u8 pad0[0x90]; s32 f90; u8 pad94[0x7C]; s32 f110; u8 pad114[0x700]; } Rec2068;
extern Rec2068 humanCars[];
extern u8 carResetWeapons[];
extern s32 humanCarTotal;
extern void carRestart(Rec2068 *, s32, u8 reset_weapons);
extern void hudResetSpeed(s16);
extern s32 viewGetSplitMode(void);
extern void carSetViewMode(Rec2068 *, s32);

extern s32 arenaLoadedId;
extern s32 arenaHollandFlag;
extern s32 aiBossDefeatedFlag;
extern s32 aiCarTargets[];
extern void hdpSetSingleBoxTest(s32);
extern void hudRadarInit(void);
extern void carChoiceRegisterGroup(s32, s32);
#include "semantic/car_groups.h"
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif
#include "semantic/port_rollback.h"   /* request 48: SEED (RB_SEED) */
#include "semantic/port_humans.h"   /* request 50: vacant human records (TM2_HUMAN_SKIP_VACANT) */
extern void rtRearViewOff(void);
extern s32 renderGetView0Frames(void);
extern s32 renderGetView1Frames(void);
extern s32 renderGetMirrorFrames(void);
extern void rtSetRearView(s32);
extern void viewCreateWithCamera(s32, s32, s32);
extern void worldSetLoadedArena(s32);
extern void lightSetEnvAmbient(s32, s16 r, s16 g, s16 b);
extern void lightSetEnvDirection(s32, s32, s16 az, s16 el);
extern void lightSetEnvColor(s32, s32, s16 r, s16 g, s16 b);
extern void arenaLoadLosAngeles(void);
extern void arenaLoadMoscow(void);
extern void arenaLoadAntarctica(void);
extern void arenaLoadAmazonia(void);
extern void arenaLoadParis(void);
extern void arenaLoadHongKong(void);
extern void arenaLoadNewYork(void);
extern int arenaLoadHolland(void);
extern void arenaLoadSuicideSwamp(void);
extern void arenaLoadCyburbia(void);
extern void arenaLoadRoofTops(void);
extern void moscowEventsInitSections(void);
extern void laEventsSetSection(void);
extern void worldWarpsSetup(s32);
extern void startNodesClearUsed(void);
extern s32 GetCurTics(void);
extern void srand(s32);

/* @800D876C: index of the first of the first n 60-byte records @801814C0 whose first word is <= 0, or -1. */
s32 __attribute__((section(".text_800D876C"))) bulletFindFree(s32 n)
{
    s32 tmp[1];     /* unused; gives retail's 8-byte frame */
    s32 i;

    for (i = 0; i < n; i++) {
        if (bulletsLifeField[i].f0 <= 0)
            return i;
    }
    return -1;
}

/* @8011D8F4: index of x in the id list carSlotOwners[0..carWorldObjectCount), or -1. */
s32 __attribute__((section(".text_8011D8F4"))) carFindSlotByOwner(s32 x)
{
    s32 tmp[1];     /* unused; gives retail's 8-byte frame */
    s32 r = -1;
    s32 i;

    for (i = 0; i < carWorldObjectCount; i++) {
        if (carSlotOwners[i] == x) {
            r = i;
            break;
        }
    }
    return r;
}

/* @8011E16C: count of the aiCarTotal 2132-byte records @801890FC whose first word is > 0. */
s32 __attribute__((section(".text_8011E16C"))) aiCarsCountAlive(void)
{
    s32 tmp[1];     /* unused; gives retail's 8-byte frame */
    s32 i;
    s32 n = 0;

    for (i = 0; i < aiCarTotal; i++) {
        if (aiCars[i].hp > 0)
            n++;
    }
    return n;
}

#define H(o) (*(s16 *)(p + (o)))
/* @800FF5BC: fill an object's default tuning halfwords (+0x3A..+0xE4) and copy +0x3A into word +0xA4. */
void __attribute__((section(".text_800FF5BC"))) carStatsDarkToothStage2(u8 *p)
{
    H(66) = 50;
    H(68) = 30;
    H(70) = 60;
    H(72) = 50;
    H(76) = 115;
    H(78) = 40;
    H(80) = -40;
    H(216) = -20;
    H(218) = 20;
    H(220) = 0;
    H(224) = 0;
    H(226) = 0;
    H(228) = 20;
    H(58) = 110;
    H(122) = 50;
    *(s32 *)(p + 164) = H(58);
}
#undef H

/* @800D6E4C: reset the camera/view state @801AB810 (flags, distances, rotation), clear its object's
   +0x20, call objRegistrySetStep(0xC80,0,0) and rebuild the object's matrix from the zero rotation. */
void __attribute__((section(".text_800D6E4C"))) eiffelTowerReset(void)
{
    eiffelTowerState.b0 = 0;
    eiffelTowerState.w20 = -6144;
    eiffelTowerState.b2 = 0;
    eiffelTowerState.w1C = 0;
    eiffelTowerState.w24 = 240;
    eiffelTowerState.rot[0] = 0;
    eiffelTowerState.rot[1] = 0;
    eiffelTowerState.rot[2] = 0;
    *(s32 *)(eiffelTowerState.obj + 32) = 0;
    objRegistrySetStep(0xC80, 0, 0);
    RotMatrixYXZ(eiffelTowerState.rot, eiffelTowerState.obj + 4);
}

/* @80114C5C: in the 10-entry (val,key) table @80187380 set val for key; if key is absent, claim the
   first entry with val == 0. */
void __attribute__((section(".text_80114C5C"))) lightFlickerStart(s32 key, s32 val, u8 *color /* unused; every retail caller passes a colour */)
{
    s32 found = 0;
    s32 i;

    for (i = 0; !found && i < 10; i++) {
        if (lightFlickerTimers[i].key == key) {
            lightFlickerTimers[i].val = val;
            found = 1;
        }
    }
    for (i = 0; !found && i < 10; i++) {
        if (lightFlickerTimers[i].val == 0) {
            found = 1;
            lightFlickerTimers[i].val = val;
            lightFlickerTimers[i].key = key;
        }
    }
}

static __inline__ s32 imin(s32 a, s32 b) { return (a < b) ? a : b; }
static __inline__ s32 imax(s32 a, s32 b) { if (a >= b) return a; return b; }

/* @80128F64: call virtual slot 3 of object musicManagerObject with 2; audioCdVolume = min(0, audioVolumeClampMax), then
   overwritten with max(0, audioVolumeClampMin) and used as the serial (CD) volume; then taskMgrStopAll / audioClearDarkToothMusic / taskMgrErrorStub on soundManagerObject. */
void __attribute__((section(".text_80128F64"))) audioMuteMusic(void)
{
    u8 *o = musicManagerObject;
    VtblEnt *e = (VtblEnt *)(*(u8 **)(o + 8) + 24);
    s32 v;

    e->f(o + e->d, 2);
    v = 0;
    audioCdVolume = imin(v, audioVolumeClampMax);
    audioCdVolume = imax(v, audioVolumeClampMin);
    v = audioCdVolume;
    SsSetSerialVol(0, v, v);
    taskMgrStopAll(soundManagerObject);
    audioClearDarkToothMusic();
    taskMgrErrorStub(soundManagerObject);
}

/* @8011A198: for each of the humanCarTotal 2068-byte records @80187B10: carRestart(rec, +0x90,
   carResetWeapons[i]), hudResetSpeed(i), and unless viewGetSplitMode() also carSetViewMode(rec, +0x110). */
void __attribute__((section(".text_8011A198"))) humanCarsRestart(void)
{
    s32 i;

    for (i = 0; i < humanCarTotal; i++) {
        TM2_HUMAN_SKIP_VACANT(i);   /* request 50: a vacant record is not restarted (nothing with no define) */
        carRestart(&humanCars[i], humanCars[i].f90, carResetWeapons[i]);
        hudResetSpeed(i);
        if (viewGetSplitMode() == 0)
            carSetViewMode(&humanCars[i], humanCars[i].f110);
    }
}

/* @8011995C: round/arena start setup: reset, register ids, set up view mode by player count,
   clear the 9 slots @801879FC; if the arena changed, load it (lighting, per-arena setup, warps),
   else startNodesClearUsed; then reseed rand. */
void __attribute__((section(".text_8011995C"))) arenaSetup(s32 arena)
{
    s32 i;

    hdpSetSingleBoxTest(0);
    hudRadarInit();
    for (i = 0; i < carWorldObjectCount; i++)
#ifdef TM2_PORT_CAR_GROUPS
        carChoiceRegisterGroup(carSlotObjects[i], TM2_CAR_KEY(i, carSlotOwners[i]));   /* request 32: the slot's key */
#else
        carChoiceRegisterGroup(carSlotObjects[i], carSlotOwners[i]);
#endif
    rtRearViewOff();
    if (viewGetSplitMode() == 1) {
        viewCreateWithCamera(renderGetView0Frames(), 5, 0);
        viewCreateWithCamera(renderGetView1Frames(), 6, 1);
    } else if (viewGetSplitMode() == 2) {
        viewCreateWithCamera(renderGetView0Frames(), 2, 0);
        viewCreateWithCamera(renderGetView1Frames(), 3, 1);
    } else {
        viewCreateWithCamera(renderGetView0Frames(), 1, 0);
        rtSetRearView(1);
        viewCreateWithCamera(renderGetMirrorFrames(), 4, 1);
    }
    for (i = 0; i < TM2_MAX_AI_CARS; i++)
        aiCarTargets[i] = -1;
    if (arenaLoadedId != arena) {
        arenaLoadedId = arena;
        worldSetLoadedArena(arena);
        lightSetEnvAmbient(0, 175, 175, 175);
        lightSetEnvDirection(0, 0, 0, 59);
        lightSetEnvDirection(0, 1, 0, 18);
        lightSetEnvDirection(0, 2, 0, 21);
        lightSetEnvColor(0, 0, 75, 75, 75);
        lightSetEnvColor(0, 1, 75, 75, 75);
        lightSetEnvColor(0, 2, 75, 75, 75);
        switch (arena) {
        case 1:
            arenaLoadLosAngeles();
            break;
        case 2:
            arenaLoadMoscow();
            break;
        case 6:
            arenaLoadAntarctica();
            break;
        case 4:
            arenaLoadAmazonia();
            break;
        case 3:
            arenaLoadParis();
            eiffelTowerReset();
            break;
        case 8:
            arenaLoadHongKong();
            break;
        case 5:
            arenaLoadNewYork();
            break;
        case 10:
            arenaLoadSuicideSwamp();
            break;
        case 11:
            arenaLoadCyburbia();
            break;
        case 12:
            arenaLoadRoofTops();
            break;
        default:
            arenaLoadHolland();
            break;
        }
        moscowEventsInitSections();
        laEventsSetSection();
        arenaHollandFlag = 0;
        worldWarpsSetup(arena);
    } else {
        startNodesClearUsed();
    }
    aiBossDefeatedFlag = 0;
#ifdef TM2_PORT_DARK_TOOTH
    portCarDarkToothClearStages();   /* request 33-2e: the duplicates' stage flags too */
#endif
    srand(RB_SEED(TM2_RB_SITE_ARENA));   /* request 48 SEED: the port's seed; retail GetCurTics() */
}
