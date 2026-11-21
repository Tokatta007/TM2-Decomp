/* Session setup (80119698..8011995C): uaInit and the car-record clears.
 * Both human records are prepared and get pad configuration 1; the random
 * generator is seeded from the tick counter (srand(GetCurTics())). */

typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

struct TM2HumanInit {
    u8 pad_000[0x2];
    s16 id;                 /* +0x002 */
    u8 pad_004[0x92];
    u8 f_096, f_097, f_098, f_099, f_09A, f_09B;   /* +0x096 */
    u8 pad_09C[0x158];
    s32 f_1F4;              /* +0x1F4 */
    u8 pad_1F8[0x618];
    s32 f_810;              /* +0x810 */
};
typedef char tm2_human_init_size[(sizeof(struct TM2HumanInit) == 0x814) ? 1 : -1];

struct TM2AIInit {
    u8 pad_000[0x850];
    s32 f_850;              /* +0x850 */
};
typedef char tm2_ai_init_size[(sizeof(struct TM2AIInit) == 0x854) ? 1 : -1];

#include "semantic/players.h"

extern struct TM2HumanInit humanCars[TM2_MAX_HUMANS];
extern struct TM2AIInit aiCars[TM2_MAX_AI_CARS];

extern s32 roundOverFlag, D_80180F98, arenaHollandFlag, viewWatchingAi, viewMainPlayer, viewWatchedAiCar, roundWonFlag;
extern u8 viewExtraViewsUnlocked;
extern s32 aiDifficulty;
extern s32 D_80180FD8[2], D_80180FE0[2];
extern s32 viewCameraObjectTable[TM2_MAX_VIEWS];      /* camera objects */
extern s32 humanCarTypes;
extern s32 gameMode;                  /* game type */
extern u8 gameAttractDemoMode;
extern s32 demoRoundsPlayed, demoWatchTimer, demoRespawnTimer, demoFollowPlayerCamera;
extern s32 aiCarTargets[TM2_MAX_AI_CARS];               /* AI targets */
extern s32 carSlotKinds[TM2_MAX_CAR_SLOTS];              /* car object kinds */
extern s32 carSlotIndices[TM2_MAX_CAR_SLOTS];              /* car object indices */
extern s32 aiCarObjectSlots[TM2_MAX_AI_CARS];               /* AI car object slots */
extern s32 aiCarTotal;                  /* AI cars */
extern s32 carWorldObjectCount;                  /* car objects */
extern s32 arenaLoadedId;

extern void PadSetConfig(s32 config, short player);
extern void humanCarsSetCount(s32);
extern void fxSpritesReset(void);
#include "semantic/world.h"
extern void srand(unsigned int);
extern void weaponSlamStatesReset(void);
extern void targetPoolReset(void);
extern int gameGetLevel(void);
extern void worldCollapseReset(void);
extern void worldTrackVehiclesReset(void);

#include "semantic/car_calls.h"
#include "semantic/car_groups.h"
#include "semantic/port_cars.h"
#include "semantic/port_rollback.h"   /* request 48: SEED (RB_SEED) */
/* in timer_frame.c */
void carTablesClear(void);

/* @80119698 */
void __attribute__((section(".text_80119698"))) uaInit(void)
{
    s32 i;
    roundOverFlag = 0;
    D_80180F98 = 1;
    arenaHollandFlag = 0;
    viewWatchingAi = 0;
    viewMainPlayer = 0;
    viewWatchedAiCar = 0;
    roundWonFlag = 0;
    viewExtraViewsUnlocked = 1;
    aiDifficulty = 1;
    roundOverFlag = 0;
    D_80180F98 = 1;
    for (i = 0; i < TM2_MAX_HUMANS; i++) {
        carSetResetWeapons(1, i);
        humanCars[i].f_1F4 = 1;
        humanCars[i].id = i;
        humanCars[i].f_096 = 0;
        humanCars[i].f_097 = 0;
        humanCars[i].f_098 = 0;
        humanCars[i].f_099 = 0;
        humanCars[i].f_09A = 0;
        humanCars[i].f_09B = 0;
    }
    carTablesClear();
    for (i = 0; i < 2; i++) {
        PadSetConfig(1, i);
        D_80180FD8[i] = 0;
        D_80180FE0[i] = 0;
    }
#if TM2_MAX_HUMANS > 2
    /* players 3-4: pad configuration only (D_80180FD8/D_80180FE0 stay two
       entries; FOUR_HUMANS.md 1.1: nothing reads them) */
    for (i = 2; i < TM2_MAX_HUMANS; i++)
        PadSetConfig(1, i);
#endif
#if TM2_MAX_VIEWS > 2
    for (i = 0; i < TM2_MAX_VIEWS; i++)
        viewCameraObjectTable[i] = 0;
#else
    viewCameraObjectTable[0] = 0;
    viewCameraObjectTable[1] = 0;
#endif
    humanCarsSetCount(1);
    humanCarTypes = 0x90;
    fxSpritesReset();
    srand(RB_SEED(TM2_RB_SITE_SHELL));   /* request 48 SEED: the port's seed; retail GetCurTics() */
    gameMode = 1;
    gameAttractDemoMode = 0;
    demoRoundsPlayed = 0;
    demoWatchTimer = 0;
    demoRespawnTimer = 0;
    demoFollowPlayerCamera = 0;
    for (i = 0; i < TM2_MAX_AI_CARS; i++)
        aiCarTargets[i] = -1;
}

/* @80119848: clear the car object tables and records. */
void __attribute__((section(".text_80119848"))) carTablesClear(void)
{
    s32 i;
#ifdef TM2_PORT_CAR_GROUPS
    portCarGroupsReset();   /* request 32 (CAR_GROUPS.md): car keys and per-key registries */
#endif
#ifdef TM2_PORT_CARS
    /* request 32 step 2: the weapon pools' size for this load (portCarOptions bit 3), fixed until the
       next load: the level and car DMDs create the pools' objects after this */
    portCarPoolsLive = TM2_PORT_POOLS_ON;
#endif
    for (i = 0; i < TM2_MAX_CAR_SLOTS; i++) {
        carSlotKinds[i] = 2;
        carSlotIndices[i] = -1;
    }
    for (i = 0; i < TM2_MAX_AI_CARS; i++)
        aiCarObjectSlots[i] = -1;
    for (i = 0; i < TM2_MAX_HUMANS; i++)
        humanCars[i].f_810 = 0;
    for (i = 0; i < TM2_MAX_AI_CARS; i++)
        aiCars[i].f_850 = 0;
    weaponSlamStatesReset();
    aiCarTotal = 0;
    carWorldObjectCount = 0;
#if TM2_MAX_VIEWS > 2
    for (i = 0; i < TM2_MAX_VIEWS; i++)
        viewCameraObjectTable[i] = 0;
#else
    viewCameraObjectTable[0] = 0;
    viewCameraObjectTable[1] = 0;
#endif
    targetPoolReset();
    if (gameGetLevel() == 6)
        worldCollapseReset();
    else if (gameGetLevel() == 8)
        worldTrackVehiclesReset();
    arenaLoadedId = -1;
}
