/* Small game helpers: constant returns, empty stubs, record addresses,
 * table clears and one-line wrappers. Names stay address-based. */
#define TM2_OWN_CAR_ARRAYS /* aiCars declared below with its own view */
#include "semantic/cars.h"
#include "semantic/settings.h"
#include "semantic/car_groups.h"
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H2: portHumanLives */
#endif

typedef struct {
    s32 f00;
    u8 pad_04[56];
} TM2Rec60;

typedef struct {
    s32 f00;
    u8 pad_04[32];
} TM2Rec36;

typedef struct {
    u8 f00;
    u8 pad_01[2131];
} TM2Rec2132;

extern s32 terrainSurfaceCount;
extern u8 *darkToothBossObject;
extern s32 *fxTargetHitObject;
extern u8 *trackVehicleRecords;
extern s32 gameMode;
extern u8 gameAttractDemoMode;
extern u8 fxSprites[];
extern u8 targetActors[];
extern u8 worldFlameMarkers[];
extern u8 moscowEvents[];
extern u8 laEvents[];
extern TM2Rec60 bulletsLifeField[40];
extern TM2Rec36 fxImpactParticlesLifeField[20];
extern TM2Rec2132 aiCars[TM2_MAX_AI_CARS];

extern void taskMgrStopAll(s32);
extern void sndListenerTransform(s32, s32);
extern void worldCollapseReset(void);

/* @8010B594 */
s32 __attribute__((section(".text_8010B594"))) sub_8010B594(void)
{
    return 15;
}

/* @8010B59C */
s32 __attribute__((section(".text_8010B59C"))) carCrushDamageAmount(void)
{
    return 15;
}

/* @8010B5A4 .. 8010B5BC: empty stubs (IDA: soundSeVibOnStub, soundSetVibStub, soundUtVibrateOnStub/Off) */
void __attribute__((section(".text_8010B5A4"))) soundSeVibOnStub(void)
{
}

void __attribute__((section(".text_8010B5AC"))) soundSetVibStub(void)
{
}

void __attribute__((section(".text_8010B5B4"))) soundUtVibrateOnStub(void)
{
}

void __attribute__((section(".text_8010B5BC"))) soundUtVibrateOffStub(void)
{
}

/* @80100478 */
s32 __attribute__((section(".text_80100478"))) trackVehicleSurfaceIndex(s32 index)
{
    s32 n = index + 1;

    return terrainSurfaceCount - n;
}

/* @801134C8 */
u8 *__attribute__((section(".text_801134C8"))) fxSpritesGet(void)
{
    return fxSprites;
}

/* @80126454 */
u8 *__attribute__((section(".text_80126454"))) targetActorsGet(void)
{
    return targetActors;
}

/* @801108F8 */
s32 __attribute__((section(".text_801108F8"))) aiDarkToothBossModel(void)
{
    return *(s32 *)(darkToothBossObject + 284);
}

/* @80119584 */
void __attribute__((section(".text_80119584"))) fxSetTargetHitObject(s32 *p)
{
    fxTargetHitObject = p;
    p[2] = 1;
}

/* @800E4D54 */
void __attribute__((section(".text_800E4D54"))) carPushClear(struct TM2CarContact *c)
{
    c->in_contact = 0;
    c->f03 = 0;
    c->f04 = 0;
    c->timer = 0;
}

/* @800D667C: 20-byte record `index`. */
u8 *__attribute__((section(".text_800D667C"))) worldFlameMarkerGet(s32 index)
{
    return worldFlameMarkers + index * 20;
}

/* @800E6CF8 */
void __attribute__((section(".text_800E6CF8"))) wheelSpinOutSettle(u8 *p, void *motion /* unused; the retail caller passes it */)
{
    if (!p[1])
        p[3] = 0;
}

/* @80118EFC: field +40 of 56-byte record `index`. */
u8 *__attribute__((section(".text_80118EFC"))) trackVehicleGetMove(s32 index)
{
    return trackVehicleRecords + index * 56 + 40;
}

/* 800D3530 is the body of soundStopAll (game_misc_accessors.c), not a function. */

/* @800D3550 */
void __attribute__((section(".text_800D3550"))) soundLocateForView0(s32 a)
{
    sndListenerTransform(a, 0);
}

/* @800D3570 */
void __attribute__((section(".text_800D3570"))) soundLocateForView(s32 a, s32 b)
{
    sndListenerTransform(a, b);
}

/* @800E4D6C */
void __attribute__((section(".text_800E4D6C"))) carPushTimerArm(struct TM2CarContact *c)
{
    if (c->timer <= 0)
        c->timer = 180;
}

/* @8011A248 */
void __attribute__((section(".text_8011A248"))) worldTeardown(void)
{
    worldCollapseReset();
}

/* @800D44E0: 196-byte record `index`. */
u8 *__attribute__((section(".text_800D44E0"))) moscowEventGet(s32 index)
{
    return moscowEvents + index * 196;
}

/* @800D5560: 196-byte record `index`. */
u8 *__attribute__((section(".text_800D5560"))) laEventGet(s32 index)
{
    return laEvents + index * 196;
}

/* @800D7A28 */
void __attribute__((section(".text_800D7A28"))) bulletsClear(void)
{
    s32 i;

    for (i = 39 * sizeof(TM2Rec60); i >= 0; i -= sizeof(TM2Rec60))
        *(s32 *)((u8 *)bulletsLifeField + i) = 0;
}

/* @80112C30 */
void __attribute__((section(".text_80112C30"))) fxImpactParticlesClear(void)
{
    s32 i;

    for (i = 19 * sizeof(TM2Rec36); i >= 0; i -= sizeof(TM2Rec36))
        *(s32 *)((u8 *)fxImpactParticlesLifeField + i) = 0;
}

/* @8011A3A0 */
void __attribute__((section(".text_8011A3A0"))) aiCarsSetActive(u8 value)
{
    s32 i;

    for (i = (TM2_MAX_AI_CARS - 1) * sizeof(TM2Rec2132); i >= 0; i -= sizeof(TM2Rec2132))
        *((u8 *)aiCars + i) = value;
}

/* @8011ED88 */
void __attribute__((section(".text_8011ED88"))) gameSetMode(s32 value)
{
    gameMode = value;
    if (!value)
        gameAttractDemoMode = 1;
    else
        gameAttractDemoMode = 0;
}

extern u8 hudRadarVisible[];
extern u8 hudWeaponListVisible[];

extern s32 carPairDistances[][TM2_MAX_CAR_SLOTS];
extern s16 carPairHeadings[][TM2_MAX_CAR_SLOTS];
extern u8 carPairDirections[][TM2_MAX_CAR_SLOTS][12];
extern s32 carTypeNames[];
extern s32 carTypeIdTable[];
extern s32 *fxPlatformSoundObject;
extern u8 worldSectionFirstSurface[35], worldSectionLastSurface[35];  /* per track section */
extern u8 *carBodyObjects[];
extern u8 *carWreckModels[];
extern s32 bulletsFrameParity;
extern s32 *carPlayerOneObject;
extern u8 *slamArmObject;
extern s32 slamArmAngle;

/* Moscow dish emitter (5 x 16 bytes): a flag word and the emitter position. */
typedef struct {
    s32 flag;
    s32 pos[3];
} TM2DishEmitter;
extern TM2DishEmitter moscowDishEmitters[5];

typedef struct {
    s16 vx, vy, vz, pad;
} TM2SVector;

extern void lightFlickerUpdateStub(void);
extern void lightFlickerTick(void);
extern void audioClearDarkToothMusic(void);
extern s32 sessionGetLevel(struct TM2Settings *);
extern s32 settingsLevelDmdFileName(struct TM2Settings *);
extern s32 settingsLevelTmsFileName(struct TM2Settings *);
extern s32 sessionGetLives(struct TM2Settings *, s32);
extern s32 levelFileStem(struct TM2Settings *, s32);
extern s32 gameGetLevel(void);
extern void screenInitCardSprites(void);
extern void screenLoadCardImages(void);
extern void screenDealCards(void);
extern s32 cdAudioHasEnded(void);
extern void cdAudioClearEnded(void);
extern s32 carTypeToIndex(s32);
#include "semantic/world.h"
extern void bulletsMove(void);
extern s32 *humanCarObject(s32);
extern void *RotMatrixYXZ(TM2SVector *, u8 *);

/* @800FCFA4: toggle flag `index`. */
void __attribute__((section(".text_800FCFA4"))) hudToggleRadar(s32 index)
{
    hudRadarVisible[index] = 1 - hudRadarVisible[index];
}

/* @800FCFE4: toggle flag `index`. */
void __attribute__((section(".text_800FCFE4"))) hudToggleWeaponList(s32 index)
{
    hudWeaponListVisible[index] = 1 - hudWeaponListVisible[index];
}

/* @80114B48 */
void __attribute__((section(".text_80114B48"))) lightFlickerUpdate(void)
{
    lightFlickerUpdateStub();
    lightFlickerTick();
}

/* @80128F3C */
void __attribute__((section(".text_80128F3C"))) sessionTeardown(void)
{
    audioClearDarkToothMusic();
    worldTeardown();
}

/* @80129AF8: the level / arena (settings +24). */
s32 __attribute__((section(".text_80129AF8"))) gameGetLevel(void)
{
    return sessionGetLevel(&sessionSettings);
}

/* @80129B20 */
u8 __attribute__((section(".text_80129B20"))) gameGetLives(s32 index)
{
#ifdef TM2_PORT_HUMANS
    /* H2 (request 37 answer 11): the settings hold humans 0..3 (retail); past them, or for every human with
       portHumanOptions bit 5, the lives the port keeps in portHumanLives */
    if ((u32)index >= 4 || (portHumanOptions & (TM2_HUMAN_OPT_LIVES | TM2_HUMAN_OPT_RULES)))   /* H5: bit 1 too */
        return (u32)index < TM2_MAX_HUMANS ? portHumanLives[index] : 0;
#endif
    return sessionGetLives(&sessionSettings, index);
}

/* @80129B4C */
s32 __attribute__((section(".text_80129B4C"))) gameLevelDmdFileName(void)
{
    return settingsLevelDmdFileName(&sessionSettings);
}

/* @80129B74 */
s32 __attribute__((section(".text_80129B74"))) gameLevelTmsFileName(void)
{
    return settingsLevelTmsFileName(&sessionSettings);
}

/* @80129B9C */
s32 __attribute__((section(".text_80129B9C"))) gameLevelFileStem(s32 index)
{
    return levelFileStem(&sessionSettings, index);
}

/* @8011EB78 */
u8 *__attribute__((section(".text_8011EB78"))) carPairDirection(s32 i, s32 j)
{
    return carPairDirections[i][j];
}

/* @8011EBAC */
s32 __attribute__((section(".text_8011EBAC"))) carPairHeading(s32 i, s32 j)
{
    return carPairHeadings[i][j];
}

/* @8011EBE0 */
s32 __attribute__((section(".text_8011EBE0"))) carPairDistance(s32 i, s32 j)
{
    return carPairDistances[i][j];
}

/* @800F600C */
void __attribute__((section(".text_800F600C"))) eiffelTowerSetFallenSurfaces(void)
{
    worldSectionFirstSurface[13] = 54;
    worldSectionLastSurface[13] = 66;
    worldSectionFirstSurface[15] = 54;
    worldSectionLastSurface[15] = 66;
}

/* @801002E8 */
void __attribute__((section(".text_801002E8"))) aiRangeFlagsClear(u8 *p)
{
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
    p[4] = 0;
    p[5] = 0;
    p[6] = 0;
    p[7] = 0;
    p[8] = 0;
    p[9] = 0;
}

/* @8011ECC4 */
s32 __attribute__((section(".text_8011ECC4"))) worldFallDeathZ(void)
{
    /* reusing the call result variable keeps it in v0 (matches retail) */
    s32 r = gameGetLevel();
    if (r == 8)
        r = -800;
    else
        r = 80;
    return r;
}

/* @8012A910 */
void __attribute__((section(".text_8012A910"))) sub_8012A910(void)
{
#ifdef TM2_PORT_CARS
    /* request 32 step 2 (CAR_GROUPS.md): the card screen holds 9 opponent cards (screenCardQuads,
       the slot layouts); with more opponents it is skipped (the port draws its own line-up) */
    if (sessionSettings.opponents > TM2_RETAIL_OPPONENT_CARDS)
        return;
#endif
    screenInitCardSprites();
    screenLoadCardImages();
    screenDealCards();
}

/* @800D3A10 */
u8 __attribute__((section(".text_800D3A10"))) audioTakeTrackEnded(void)
{
    if (!cdAudioHasEnded())
        return 0;
    cdAudioClearEnded();
    return 1;
}

/* @800FB878 */
s32 __attribute__((section(".text_800FB878"))) carTypeName(s32 type)
{
    return carTypeNames[carTypeToIndex(type)];
}

/* @801190F8 */
void __attribute__((section(".text_801190F8"))) fxPlatformSoundStop(void)
{
    if (fxPlatformSoundObject) {
        fxPlatformSoundObject[2] = 0;
        objRegistrySetStep(0xC1F, 0, 0);
    }
}

/* @8011E1C4: index of `value` in the 14-entry table (14 if absent). */
s32 __attribute__((section(".text_8011E1C4"))) carTypeToIndex(s32 value)
{
    s32 i;

    for (i = 0; i < 14; i++)
        if (value == carTypeIdTable[i])
            break;
    return i;
}

/* @80114DD0 */
void __attribute__((section(".text_80114DD0"))) sub_80114DD0(s32 a, s32 b, s32 mode)
{
    if (mode == 0)
        mode = 1;
    else if (mode == 1)
        mode = 2;
    else
        return;
    objRegistrySetStep(a, b, mode);
}

/* @80114EEC */
void __attribute__((section(".text_80114EEC"))) carBodyObjectRegister(s32 id, u8 *object)
{
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): into the slot of the load group's key; retail's bound on the
       type stays (Dark Tooth's 243 is not registered, as retail) */
    s32 k = portCarLoadKey();

    if (!k)
        k = id;
    id >>= 4;
    k >>= 4;
    if (*object == 5 && id < 15 && k < TM2_CAR_KEYS)
        portCarGroups.body[k] = object;
#else
    id >>= 4;
    if (*object == 5 && id < 15)
        carBodyObjects[id] = object;
#endif
}

/* @801152A8 */
void __attribute__((section(".text_801152A8"))) carWreckModelRegister(s32 id, u8 *object)
{
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): as carBodyObjectRegister */
    s32 k = portCarLoadKey();

    if (!k)
        k = id;
    id >>= 4;
    k >>= 4;
    if (*object == 1 && id < 15 && k < TM2_CAR_KEYS)
        portCarGroups.wreck[k] = object;
#else
    id >>= 4;
    if (*object == 1 && id < 15)
        carWreckModels[id] = object;
#endif
}

/* @80122078 */
void __attribute__((section(".text_80122078"))) sceneObjSetChoiceState(u8 **p, s32 value)
{
    u8 *q = p[1];

    if (q && *q == 9)
        p[1][7] = value;
}

/* @800D5484 */
void __attribute__((section(".text_800D5484"))) laEventsSetSection(void)
{
    s32 i;
    struct { u8 on; u8 pad[11]; s16 f0C; u8 pad2[182]; } *rec = (void *)laEvents;

    for (i = 0; i < 5; i++)
        if (rec[i].on)
            rec[i].f0C = 6;
}

/* @800D65FC */
void __attribute__((section(".text_800D65FC"))) moscowDishSetEmitterPos(s32 *v, s32 index)
{
    moscowDishEmitters[index].pos[0] = v[0];
    moscowDishEmitters[index].pos[1] = v[1];
    moscowDishEmitters[index].pos[2] = v[2];
}

/* @800D87B4 */
void __attribute__((section(".text_800D87B4"))) bulletsUpdate(void)
{
    bulletsFrameParity = 1 - bulletsFrameParity;
    bulletsMove();
    carPlayerOneObject = humanCarObject(0);
}

/* @80110BA8: first 48-byte record whose +10 is negative, else 0. */
u8 *__attribute__((section(".text_80110BA8"))) fxSpriteFindFree(void)
{
    s32 i;
    u8 *rec;

    for (i = 0, rec = fxSprites; i < TM2_MAX_FX_SPRITES; i++, rec += 48)
        if ((s8)rec[10] < 0)
            return rec;
    return 0;
}

/* @80112C54: first free 36-byte slot, else -1. */
s32 __attribute__((section(".text_80112C54"))) fxImpactParticleFindFree(void)
{
    s32 i;

    for (i = 0; i < 20; i++)
        if (fxImpactParticlesLifeField[i].f00 <= 0)
            return i;
    return -1;
}

/* @80115230 */
void __attribute__((section(".text_80115230"))) slamArmSetAngle(s32 angle)
{
    TM2SVector v;

    if (slamArmObject) {
        slamArmAngle = angle;
        v.vx = angle;
        v.vy = 0;
        v.vz = 0;
        RotMatrixYXZ(&v, slamArmObject + 4);
    }
}
