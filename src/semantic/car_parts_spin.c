/* Small game helpers (8011..8012 range). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/settings.h"
#include "semantic/car_groups.h"

typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } TM2Matrix;

/* Jump tables of the four name lookups at the end (800CFB58, FB88, FBB8, FBF8). */
#pragma tm2_rdata .tm2_800CFB58

extern s32 carPartsSpinAngleHammerhead, carPartsSpinAngleMinion, carPartsSpinAngleMrSlam, carPartsSpinAngleDarkTooth;
extern u8 *carPartsHammerhead[];
extern u8 *carPartsMrSlam[];
extern u8 *carPartsMinion[];
extern u8 *carPartsDarkTooth[];
extern s32 carPartsTireTurnAccum[];
extern void *RotMatrixX(s32, TM2Matrix *);
extern void mathCopyWords(void *, TM2Matrix *, s32);
extern void carPartsSpinAxel(s32);
extern void uaswToggleCarTireState(s32 group, s32 direction);

/* Four copies of the carPartsSpinAxel pattern: turn a shared angle by a / 7 and
   rotate a list of objects about X by it. */
#define ROTATE_OBJECTS(angle, objs, count)                      \
    TM2Matrix m;                                                \
    s32 i, j, k;                                                \
    angle -= a / 7;                                             \
    for (i = 0; i < 3; i++) {                                   \
        for (j = 0; j < 3; j++)                                 \
            m.m[i][j] = (i == j) << 12;                         \
        m.t[i] = 0;                                             \
    }                                                           \
    RotMatrixX(-angle, &m);                                     \
    for (k = 0; k < count; k++)                                 \
        if (objs[k])                                            \
            mathCopyWords(objs[k] + 4, &m, 5)

/* @801144F8: angle carPartsSpinAngleHammerhead, 4 objects at carPartsHammerhead. */
void __attribute__((section(".text_801144F8"))) carPartsSpinHammerhead(s32 a)
{
    ROTATE_OBJECTS(carPartsSpinAngleHammerhead, carPartsHammerhead, 4);
}

/* @801145E8: angle carPartsSpinAngleMinion, 6 objects at carPartsMinion. */
void __attribute__((section(".text_801145E8"))) carPartsSpinMinion(s32 a)
{
    ROTATE_OBJECTS(carPartsSpinAngleMinion, carPartsMinion, 6);
}

/* @801146D8: angle carPartsSpinAngleMrSlam, 4 objects at carPartsMrSlam. */
void __attribute__((section(".text_801146D8"))) carPartsSpinMrSlam(s32 a)
{
    ROTATE_OBJECTS(carPartsSpinAngleMrSlam, carPartsMrSlam, 4);
}

/* @801147C8: angle carPartsSpinAngleDarkTooth, 4 objects at carPartsDarkTooth. */
void __attribute__((section(".text_801147C8"))) carPartsSpinDarkTooth(s32 a)
{
    ROTATE_OBJECTS(carPartsSpinAngleDarkTooth, carPartsDarkTooth, 4);
}

/* @80114B70: dispatch a track-object event: kinds 0x20/B0/C0/E0/F0 turn
   their object groups by amount; otherwise kind>>4 (< 15) accumulates |amount|
   in carPartsTireTurnAccum[] and toggles the car tire state once it passes 320. */
#ifdef TM2_PORT_CAR_GROUPS
/* request 32 (CAR_GROUPS.md): kind is a car key; the spinning parts and the tire state of that car */
void __attribute__((section(".text_80114B70"))) carPartsAnimate(s32 kind, s32 amount)
{
    s32 type = portCarKeyType(kind);

    if (type == 0x20 || type == 0xB0 || type == 0xC0 || type == 0xE0 || type == 0xF0)
        portCarPartsSpin(kind, amount);
    else {
        kind >>= 4;
        if ((u32)kind < TM2_CAR_KEYS) {
            portCarGroups.tire_accum[kind] += __builtin_abs(amount);
            if (portCarGroups.tire_accum[kind] > 320) {
                uaswToggleCarTireState(kind, amount);
                portCarGroups.tire_accum[kind] = 0;
            }
        }
    }
}
#else
void __attribute__((section(".text_80114B70"))) carPartsAnimate(s32 kind, s32 amount)
{
    if (kind == 0x20)
        carPartsSpinHammerhead(amount);
    else if (kind == 0xB0)
        carPartsSpinAxel(amount);
    else if (kind == 0xC0)
        carPartsSpinMrSlam(amount);
    else if (kind == 0xE0)
        carPartsSpinMinion(amount);
    else if (kind == 0xF0)
        carPartsSpinDarkTooth(amount);
    else {
        kind >>= 4;
        if (kind < 15) {
            carPartsTireTurnAccum[kind] += __builtin_abs(amount);
            if (carPartsTireTurnAccum[kind] > 320) {
                /* retail passes the group and the signed amount: a negative
                   amount steps the tire choices back */
                uaswToggleCarTireState(kind, amount);
                carPartsTireTurnAccum[kind] = 0;
            }
        }
    }
}
#endif

extern u8 *trackVehicleRecords;
extern void *RotMatrixYXZ(void *, void *);
/* 56-byte per-player records at *trackVehicleRecords (object pointer at +52) */
typedef struct {
    s32 f0;
    u8 pad[48];
    u8 *obj;
} Rec56;
#define REC56(i) (((Rec56 *)trackVehicleRecords)[i])

/* @801179CC: put player 0's (side 0) or player 1's object at its fixed start
   position/rotation, rebuild its matrix and clear the record's first word.
   (Identical once written as 56-byte record array accesses REC56(i).obj;
   raw byte-offset casts schedule the first li differently.) */
void __attribute__((section(".text_801179CC"))) worldTrackVehiclePlace(s32 side)
{
    if (side == 0) {
        if (REC56(0).obj) {
            *(s32 *)(REC56(0).obj + 0xA8) = -11584;
            *(s32 *)(REC56(0).obj + 0xAC) = -3200;
            *(s32 *)(REC56(0).obj + 0xB0) = -64;
            *(s16 *)(REC56(0).obj + 0xA0) = 0;
            *(s16 *)(REC56(0).obj + 0xA2) = 0;
            *(s16 *)(REC56(0).obj + 0xA4) = 0;
            RotMatrixYXZ(REC56(0).obj + 0xA0, REC56(0).obj + 0x80);
            REC56(0).f0 = 0;
        }
    } else {
        if (REC56(1).obj) {
            *(s32 *)(REC56(1).obj + 0xA8) = -11776;
            *(s32 *)(REC56(1).obj + 0xAC) = -1200;
            *(s32 *)(REC56(1).obj + 0xB0) = -64;
            *(s16 *)(REC56(1).obj + 0xA0) = 0;
            *(s16 *)(REC56(1).obj + 0xA2) = 0;
            *(s16 *)(REC56(1).obj + 0xA4) = 0x800;
            RotMatrixYXZ(REC56(1).obj + 0xA0, REC56(1).obj + 0x80);
            REC56(1).f0 = 0;
        }
    }
}

extern s32 fxLavaSplashPoolIndex;
extern u8 *fxLavaSplashObjects[];
extern void objRegistrySetStep(s32, s32, s32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);

/* @8011930C: cycle through the pool fxLavaSplashObjects[1..4]: move the next object to
   pos, play sound 0x2B3 on it and fire the matching pad feedback. */
void __attribute__((section(".text_8011930C"))) fxLavaSplashAt(s32 *pos)
{
    s32 a;

    if (fxLavaSplashPoolIndex >= 5)
        fxLavaSplashPoolIndex = 1;
    /* plain array indexing (re-read because the stores may alias it) gives
       retail's address register order; a pointer local swaps them */
    if (fxLavaSplashObjects[fxLavaSplashPoolIndex]) {
        *(s32 *)(fxLavaSplashObjects[fxLavaSplashPoolIndex] + 4) = pos[0];
        *(s32 *)(fxLavaSplashObjects[fxLavaSplashPoolIndex] + 8) = pos[1];
        *(s32 *)(fxLavaSplashObjects[fxLavaSplashPoolIndex] + 12) = pos[2];
        objRegistrySetStep(0x2B3, fxLavaSplashPoolIndex, 1);
        fxLavaSplashPoolIndex++;
        if (viewGetSplitMode() == 0)
            soundLocateForView0(pos);
        else
            soundLocateForView(pos, -1);
        a = PadChkMtap();
        if ((a < 0 ? -a : a) < 1500) {
            a = PadChkMtap();
            soundPlay(0x11, a, soundGetListenerPan(), 0x63);
        }
    }
}

typedef struct {
    u8 pad_00[4];
    s16 f4;                 /* +4 */
    s16 f6;                 /* +6 */
    u8 pad_08[2132 - 8];
} TM2Slot2132;

extern TM2Slot2132 aiCars[];
extern u8 *carSlotObjects[];
extern u8 *aiCarObjects[];
extern s32 aiCarObjectSlots[];
extern s32 carSlotIndices[];
extern s32 carSlotOwners[];
extern s32 carSlotKinds[];
extern void carBindAiSlotStub(void *);
extern void aiCarStart(void *, s32, u8 reset_weapons);
extern void csSetDrawEnable(void *, u8 v);
extern void aiCarSetupProfile(void *);

/* @8011A5CC: bind slot record `slot` to entry `id`: cross-link the tables,
   initialise the record from carSlotOwners[id], mark the entry active and tag its
   object with slot + 50. */
void __attribute__((section(".text_8011A5CC"))) carBindAiSlot(s32 slot, s32 id, s32 owner /* unused; the retail caller passes it */)
{
    TM2Slot2132 *rec = &aiCars[slot];

    aiCarObjects[slot] = carSlotObjects[id];
    aiCarObjectSlots[slot] = id;
    carSlotIndices[id] = slot;
    aiCars[slot].f6 = id;
    carBindAiSlotStub(rec);
    aiCars[slot].f4 = slot;
    aiCarStart(rec, carSlotOwners[id], 1);
    aiCars[slot].f4 = slot;
    carSlotKinds[id] = 1;
    *(s32 *)(carSlotObjects[id] + 280) = slot + 50;
    csSetDrawEnable(carSlotObjects[id], 1);
    aiCarSetupProfile(rec);
}


extern s32 sessionGetMode(struct TM2Settings *);
extern void padFlush(void);
#include "semantic/input.h"
extern int VSync(s32);

/* @801290CC: wait at least n frames, then until action 9 or 8 is pressed
   (returns 1) or, when limit >= 0, more than limit frames have passed
   (returns 0). Pad port is 0 in settings modes 1/3, else 2. */
s32 __attribute__((section(".text_801290CC"))) padWaitConfirmOrTimeout(s32 n, s32 limit)
{
    s32 i = 0;
    s32 port;
    s16 p;
    s32 hit;

    if (sessionGetMode(&sessionSettings) == 1 || sessionGetMode(&sessionSettings) == 3)
        port = 0;
    else
        port = 2;
    padFlush();
    p = port;
    for (;;) {
        padUpdate();
        if (limit >= 0)
            VSync(0);
        i++;
        hit = 0;
        if (i < n)
            continue;
        if (padCheckAction(9, p) || padCheckAction(8, p))
            hit = 1;
        if (hit)
            return 1;
        if (limit >= 0 && limit < i)
            return 0;
    }
}

typedef struct {
    u32 tag;
    u8 r0, g0, b0, code;
    s16 x0, y0, x1, y1, x2, y2, x3, y3;
} TM2PolyF4;

typedef struct {
    s16 clip[4];
    s16 ofs[2];
    s16 tw[4];
    u16 tpage;
    u8 dtd, dfe, isbg, r0, g0, b0;
    u32 dr_env[16];
} TM2DrawEnv;

extern void *soundManagerObject;
extern int DrawSync(s32);
#include "semantic/scene.h"
extern void taskMgrStopAll(void *);
extern void taskMgrErrorStub(void *);
extern void SetSemiTrans(void *, s32);
extern void *SetDefDrawEnv(void *, s32, s32, s32, s32);
extern u16 GetTPage(s32, s32, s32, s32);
extern void *PutDrawEnv(void *);
extern void DrawPrim(void *);

/* @8012940C (screenLostALife): copy the frame across, draw a semi-transparent
   full-screen black quad over it and show the result. */
void __attribute__((section(".text_8012940C"))) screenLostALife(void)
{
    TM2PolyF4 poly;
    TM2DrawEnv env;

    DrawSync(0);
    VSync(0);
    screenSetEnv();
    taskMgrStopAll(soundManagerObject);
    taskMgrErrorStub(soundManagerObject);
    screenDisplayToDisplay(1, 0);
    ((u8 *)&poly)[3] = 5;
    ((u8 *)&poly)[7] = 0x28;
    SetSemiTrans(&poly, 1);
    poly.x0 = 0;
    poly.y0 = 0;
    poly.x1 = 320;
    poly.y1 = 0;
    poly.x2 = 0;
    poly.y2 = 240;
    poly.x3 = 320;
    poly.y3 = 240;
    poly.r0 = 0;
    poly.g0 = 0;
    poly.b0 = 0;
    SetDefDrawEnv(&env, 0, 0, 320, 240);
    env.tpage = GetTPage(0, 0, 0, 0);
    PutDrawEnv(&env);
    DrawPrim(&poly);
    DrawSync(0);
    VSync(0);
    screenSetEnv();
    screenDisplayToDisplay(0, 1);
}

extern char strLevelStemFreeway[], strLevelStemDish[], strLevelStemGlacier[], strLevelStemHkong[], strLevelStemIslands[],
    strLevelStemParis[], strLevelStemRoof[], strLevelStemDen[], strLevelStemSwamp[], strLevelStemBurb[], strLevelStemSroof[];
extern char strUnknownLevel[];

/* @8012ABD0: level file-name stem for level id 1..12 (1 FREEWAY, 2 DISH,
   3 PARIS, 4 ISLANDS, 5 ROOF, 6 GLACIER, 7 DEN, 8 HKONG, 10 SWAMP, 11 BURB,
   12 SROOF); 9 and out-of-range ids give "UNKNOWN LEVEL". */
char *__attribute__((section(".text_8012ABD0"))) levelFileStem(struct TM2Settings *settings, s32 course)
{
    switch (course) {
    case 1: return strLevelStemFreeway;
    case 2: return strLevelStemDish;
    case 6: return strLevelStemGlacier;
    case 8: return strLevelStemHkong;
    case 4: return strLevelStemIslands;
    case 3: return strLevelStemParis;
    case 5: return strLevelStemRoof;
    case 7: return strLevelStemDen;
    case 10: return strLevelStemSwamp;
    case 11: return strLevelStemBurb;
    case 12: return strLevelStemSroof;
    case 9: default: return strUnknownLevel;
    }
}

extern char strLevelNameLosAngeles[], strLevelNameMoscow[], strLevelNameAntarctica[], strLevelNameHongKong[], strLevelNameAmazonia[],
    strLevelNameParis[], strLevelNameNewYork[], strLevelNameHolland[], strLevelNameFlorida[], strLevelNameCyburbia[], strLevelNameRoofTops[],
    strLevelNameSecret[];

/* @8012ACB8: level display name for level id 1..12 (same ids as 8012ABD0:
   10 Suicide Swamp, 11 Cyburbia, 12 Roof Tops); 9 and out-of-range ids give
   "It's a Secret". */
char *__attribute__((section(".text_8012ACB8"))) levelDisplayName(struct TM2Settings *settings, s32 level)
{
    switch (level) {
    case 1: return strLevelNameLosAngeles;
    case 2: return strLevelNameMoscow;
    case 6: return strLevelNameAntarctica;
    case 8: return strLevelNameHongKong;
    case 4: return strLevelNameAmazonia;
    case 3: return strLevelNameParis;
    case 5: return strLevelNameNewYork;
    case 7: return strLevelNameHolland;
    case 10: return strLevelNameFlorida;
    case 11: return strLevelNameCyburbia;
    case 12: return strLevelNameRoofTops;
    case 9: default: return strLevelNameSecret;
    }
}

extern char strCarCodeHh[], strCarCodeOt[], strCarCodeWh[], strCarCodeMg[], strCarCodePv[],
    strCarCodeTh[], strCarCodeSp[], strCarCodeRk[], strCarCodeIr[], strCarCodeAx[], strCarCodeFl[],
    strCarCodeHs[], strCarCodeMn[], strCarCodeSt[], strCarCodeBb[];
extern char strUnknownCar[];
/* request 42 item 3 (car_numbers.h): a mod's file code / name past retail's 0..14; retail's "unknown" else */
#ifdef TM2_PORT_CAR_NUMBERS
#include "semantic/car_numbers.h"
#define TM2_NUMBER_CODE(id, unknown) ((char *)portCarNumberCode((id), (unknown)))
#define TM2_NUMBER_NAME(id, unknown) ((char *)portCarNumberName((id), (unknown)))
#else
#define TM2_NUMBER_CODE(id, unknown) (unknown)
#define TM2_NUMBER_NAME(id, unknown) (unknown)
#endif

/* @8012ADA0: car file-name code for car id 0..14 (HH, OT, WH, MG, PV, TH, SP,
   RK, IR, AX, FL, HS, 12 ST, 13 MN, BB); default strUnknownCar. */
char *__attribute__((section(".text_8012ADA0"))) carFileCode(struct TM2Settings *settings, u32 id)
{
    switch (id) {
    case 0: return strCarCodeHh;
    case 1: return strCarCodeOt;
    case 2: return strCarCodeWh;
    case 3: return strCarCodeMg;
    case 4: return strCarCodePv;
    case 5: return strCarCodeTh;
    case 6: return strCarCodeSp;
    case 7: return strCarCodeRk;
    case 8: return strCarCodeIr;
    case 9: return strCarCodeAx;
    case 10: return strCarCodeFl;
    case 11: return strCarCodeHs;
    case 13: return strCarCodeMn;
    case 12: return strCarCodeSt;
    case 14: return strCarCodeBb;
    default: return TM2_NUMBER_CODE(id, strUnknownCar);
    }
}

extern char strCarNameHammerhead[], strCarNameOutlaw2[], strCarNameWarthog[], strCarNameMrGrimm[], strCarNameGrasshopper[],
    strCarNameThumper[], strCarNameSpectre[], strCarNameRoadkill[], strCarNameTwister[], strCarNameAxel[], strCarNameMrSlam[],
    strCarNameShadow[], strCarNameMinion[], strCarNameSweetTooth[], strCarNameDarkTooth[];

/* @8012AEC4: car name for car id 0..14 (0 Hammerhead ... 11 Shadow,
   12 Sweet Tooth, 13 Minion, 14 Dark Tooth); default strUnknownCar. */
char *__attribute__((section(".text_8012AEC4"))) carDisplayName(struct TM2Settings *settings, u32 id)
{
    switch (id) {
    case 0: return strCarNameHammerhead;
    case 1: return strCarNameOutlaw2;
    case 2: return strCarNameWarthog;
    case 3: return strCarNameMrGrimm;
    case 4: return strCarNameGrasshopper;
    case 5: return strCarNameThumper;
    case 6: return strCarNameSpectre;
    case 7: return strCarNameRoadkill;
    case 8: return strCarNameTwister;
    case 9: return strCarNameAxel;
    case 10: return strCarNameMrSlam;
    case 11: return strCarNameShadow;
    case 13: return strCarNameMinion;
    case 12: return strCarNameSweetTooth;
    case 14: return strCarNameDarkTooth;
    default: return TM2_NUMBER_NAME(id, strUnknownCar);
    }
}
