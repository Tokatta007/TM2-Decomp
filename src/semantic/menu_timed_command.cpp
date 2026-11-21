/* @8012D37C: constructor of the movie camera command. C++ (PsyQ 3.6 CC1PLPSX,
 * -fvolatile). The vtable of each class in the chain is stored in turn; the
 * vtables are retail data (#pragma tm2_symbol). The body records the
 * parameters and reads the start position of the target into +24. */
typedef signed int s32;
typedef unsigned int u32;

struct TM2MenuCommand {                     /* vtable 800D03AC */
    s32 duration;
    s32 remaining;
    virtual void run();
    TM2MenuCommand() {}
};

struct TM2MenuTimedCommand : TM2MenuCommand {   /* vtable 800D037C */
    s32 a;
    virtual void run();
    TM2MenuTimedCommand() {}
};

struct TM2MovieCamera : TM2MenuTimedCommand {   /* vtable 800D02DC */
    void *target;
    s32 c;
    s32 start;
    s32 d;
    virtual void run();
    TM2MovieCamera(s32 first, void *second, s32 third, s32 fourth);
};

#include "semantic/scene.h"

#pragma tm2_symbol _vt.14TM2MenuCommand D_800D03AC
#pragma tm2_symbol _vt.19TM2MenuTimedCommand D_800D037C
#pragma tm2_symbol _vt.14TM2MovieCamera D_800D02DC
#pragma tm2_symbol __14TM2MovieCameraiPvii sub_8012D37C
TM2MovieCamera::TM2MovieCamera(s32 first, void *second, s32 third, s32 fourth)
{
    s32 temporary[2];
    a = first;
    target = second;
    c = third;
    d = fourth;
    sceneObjGetPosition(target, &start, temporary, temporary);
}

extern "C" {
void menuPasswordCursorRun(unsigned char *object);
struct TM2VtableEntry { short delta, index; void (*function)(void); };
TM2VtableEntry D_800D02DC[2] __attribute__((section(".tm2_800D02DC"))) = {{0, 0, 0}, {0, 0, (void (*)(void))menuPasswordCursorRun}};
}
