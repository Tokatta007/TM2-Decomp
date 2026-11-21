/* Script command objects: constructors of the command classes and small
 * forwarding helpers. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile).
 *
 * g++ 2.x layout: TM2Command holds duration (+0) and remaining (+4), then its
 * vtable pointer (+8); target-based commands add the target (+12) and their
 * parameters (+16...). Each constructor stores the vtable of every class in
 * the chain in turn (base, intermediate, final), then runs its body. The
 * vtables are retail data; their compiler symbols are mapped to the retail
 * addresses (#pragma tm2_symbol), and the final classes' tables stay defined
 * here at their retail addresses. */
typedef signed int s32;
typedef unsigned int u32;
typedef unsigned char u8;

struct TM2Target;

struct TM2Command {
    int duration;
    int remaining;
    virtual int run();
    TM2Command() {}
};

struct TM2TargetCommand : TM2Command {          /* vtable 800CF484 */
    TM2Target *target;
    virtual int run();
    TM2TargetCommand() {}
};

struct TM2VectorCommand : TM2Command {          /* vtable 800CF494 */
    TM2Target *target;
    int x, y, z;
    virtual int run();
    TM2VectorCommand() {}
};

struct TM2FourCommand : TM2Command {            /* vtable 800CF4A4 */
    TM2Target *target;
    int a, b, c;
    virtual int run();
    TM2FourCommand() {}
};

/* @80123828 */
struct TM2SetValue : TM2TargetCommand {
    int value;
    virtual int run();
    TM2SetValue(TM2Target *t, int v, int frames);
};

/* @80123870 */
struct TM2SetValueNow : TM2TargetCommand {
    int value;
    virtual int run();
    TM2SetValueNow(TM2Target *t, int v);
};

/* @801239CC, @80123A20, @80123A74, @80123AC8 */
struct TM2TurnBy : TM2VectorCommand { virtual int run(); TM2TurnBy(TM2Target *t, int x, int y, int z, int frames); };
struct TM2MoveBy : TM2VectorCommand { virtual int run(); TM2MoveBy(TM2Target *t, int x, int y, int z, int frames); };
struct TM2TurnTo : TM2VectorCommand { virtual int run(); TM2TurnTo(TM2Target *t, int x, int y, int z, int frames); };
struct TM2MoveTo : TM2VectorCommand { virtual int run(); TM2MoveTo(TM2Target *t, int x, int y, int z, int frames); };

/* @80123B64 */
struct TM2FourWay : TM2FourCommand { virtual int run(); TM2FourWay(TM2Target *t, int a, int b, int c); };

#pragma tm2_symbol _vt.10TM2Command D_800CF4B4
#pragma tm2_symbol _vt.16TM2TargetCommand D_800CF484
#pragma tm2_symbol _vt.16TM2VectorCommand D_800CF494
#pragma tm2_symbol _vt.14TM2FourCommand D_800CF4A4
#pragma tm2_symbol _vt.11TM2SetValue D_800CF394
#pragma tm2_symbol _vt.14TM2SetValueNow D_800CF3A4
#pragma tm2_symbol _vt.9TM2TurnBy D_800CF404
#pragma tm2_symbol _vt.9TM2MoveBy D_800CF414
#pragma tm2_symbol _vt.9TM2TurnTo D_800CF424
#pragma tm2_symbol _vt.9TM2MoveTo D_800CF434
#pragma tm2_symbol _vt.10TM2FourWay D_800CF464

#pragma tm2_symbol __11TM2SetValueP9TM2Targetii sub_80123828
TM2SetValue::TM2SetValue(TM2Target *t, int v, int frames)
{
    target = t;
    value = v;
    remaining = frames;
    duration = remaining;
}

#pragma tm2_symbol __14TM2SetValueNowP9TM2Targeti sub_80123870
TM2SetValueNow::TM2SetValueNow(TM2Target *t, int v)
{
    target = t;
    value = v;
}

#define VECTOR_CTOR(cls)                                        \
cls::cls(TM2Target *t, int x0, int y0, int z0, int frames)       \
{                                                               \
    target = t;                                                 \
    remaining = frames;                                         \
    duration = remaining;                                       \
    x = x0;                                                     \
    y = y0;                                                     \
    z = z0;                                                     \
}

#pragma tm2_symbol __9TM2TurnByP9TM2Targetiiii sub_801239CC
VECTOR_CTOR(TM2TurnBy)
#pragma tm2_symbol __9TM2MoveByP9TM2Targetiiii sub_80123A20
VECTOR_CTOR(TM2MoveBy)
#pragma tm2_symbol __9TM2TurnToP9TM2Targetiiii sub_80123A74
VECTOR_CTOR(TM2TurnTo)
#pragma tm2_symbol __9TM2MoveToP9TM2Targetiiii sub_80123AC8
VECTOR_CTOR(TM2MoveTo)

#pragma tm2_symbol __10TM2FourWayP9TM2Targetiii sub_80123B64
TM2FourWay::TM2FourWay(TM2Target *t, int a0, int b0, int c0)
{
    target = t;
    a = a0;
    b = b0;
    c = c0;
    remaining = -1;
    duration = remaining;
}

extern "C" {
/* g++ 2.x vtables of the final classes: entry 0 empty, entry 1 = run(). */
struct TM2VtableEntry { short delta, index; void (*function)(void); };
/* The run() functions, entered through the vtables with their own prototypes. */
s32 sceneCmdPadInputRun(void *cmd);
void sceneCmdExitRun(void *cmd), sceneCmdChoiceStepRun(void *cmd);
int sceneCmdTurnByRun(void *cmd), sceneCmdMoveByRun(void *cmd), sceneCmdTurnToRun(void *cmd), sceneCmdMoveToRun(void *cmd);
TM2VtableEntry D_800CF394[2] __attribute__((section(".tm2_800CF394"))) = {{0, 0, 0}, {0, 0, (void (*)(void))sceneCmdPadInputRun}};
TM2VtableEntry D_800CF3A4[2] __attribute__((section(".tm2_800CF3A4"))) = {{0, 0, 0}, {0, 0, (void (*)(void))sceneCmdExitRun}};
TM2VtableEntry D_800CF404[2] __attribute__((section(".tm2_800CF404"))) = {{0, 0, 0}, {0, 0, (void (*)(void))sceneCmdTurnByRun}};
TM2VtableEntry D_800CF414[2] __attribute__((section(".tm2_800CF414"))) = {{0, 0, 0}, {0, 0, (void (*)(void))sceneCmdMoveByRun}};
TM2VtableEntry D_800CF424[2] __attribute__((section(".tm2_800CF424"))) = {{0, 0, 0}, {0, 0, (void (*)(void))sceneCmdTurnToRun}};
TM2VtableEntry D_800CF434[2] __attribute__((section(".tm2_800CF434"))) = {{0, 0, 0}, {0, 0, (void (*)(void))sceneCmdMoveToRun}};
TM2VtableEntry D_800CF464[2] __attribute__((section(".tm2_800CF464"))) = {{0, 0, 0}, {0, 0, (void (*)(void))sceneCmdChoiceStepRun}};

#include "semantic/scene.h"
extern "C" void sceneObjSetVisible(unsigned char *p, unsigned short v);
extern void sceneSetCursor(void *, s32);
extern void sceneCursorPrev(void *);
extern void sceneCursorNext(void *);
extern s32 sceneGetBackground(void *), screenImageRecordGetAddress(void *), fileioLoadFileIntoRam(s32, const char *);

void __attribute__((section(".text_8012381C"))) sceneCmdSetValue(u8 *p, s32 v) { *(s32 *)(p + 16) = v; }
void __attribute__((section(".text_801238A8"))) sceneCmdSetCursorRun(u8 *p) { sceneSetCursor(*(void **)(p + 12), *(s32 *)(p + 16)); }
void __attribute__((section(".text_801238D4"))) sceneCmdCursorPrevRun(u8 *p) { sceneCursorPrev(*(void **)(p + 12)); }
void __attribute__((section(".text_801238F8"))) sceneCmdCursorNextRun(u8 *p) { sceneCursorNext(*(void **)(p + 12)); }
s32 __attribute__((section(".text_8012391C"))) sceneCmdLoadBackgroundRun(u8 *p)
{
    s32 x = sceneGetBackground(*(void **)(p + 12));
    return fileioLoadFileIntoRam(screenImageRecordGetAddress((void *)x), (char *)p + 16);
}
void __attribute__((section(".text_80123B1C"))) sceneCmdHideRun(u8 *p) { sceneObjSetVisible(*(void **)(p + 12), 0); }
void __attribute__((section(".text_80123B40"))) sceneCmdShowRun(u8 *p) { sceneObjSetVisible(*(void **)(p + 12), 1); }
}
