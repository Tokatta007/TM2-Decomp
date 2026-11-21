/* @80137D38: forward a command to the attached target object through two
 * virtual calls (the same pattern as object_virtual_dispatch.cpp). C++ (PsyQ
 * 3.6 CC1PLPSX, -fvolatile); C linkage. */
struct TM2Target {
    char fields[200];
    virtual void action(int x, int y, int z, int kind, int arg, int flag);
    virtual void finish();
};

struct TM2MenuForward {
    char fields[16];
    int x, y, z;
    TM2Target *target;
};

#include "semantic/world.h"

extern "C" void __attribute__((section(".text_80137D38"))) menuPlaySoundRun(TM2MenuForward *self)
{
    if (self->target) {
        self->target->action(self->x, self->y, self->z, 99, -1, 0);
        taskMgrRunFrame(self->target);
        self->target->finish();
    }
}
