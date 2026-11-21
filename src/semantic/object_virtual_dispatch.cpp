/* @80123BBC: forward a command to the attached target object through two virtual
 * calls. TM2 game objects are C++ (g++ 2.7.2.SN.1, PsyQ 3.6 CC1PLPSX, -fvolatile):
 * the target's vtable pointer follows 200 bytes of fields, and g++ 2.x vtable
 * entries are {delta, index, function}; `action` is entry 1, `finish` entry 2.
 * extern "C" keeps the retail symbol name; a member function has the same ABI. */
struct TM2Target {
    char fields[200];
    virtual void action(int x, int y, int z, int kind, int arg, int flag);
    virtual void finish();
};

struct TM2Commander {
    char fields[16];
    int x, y, z;
    TM2Target *target;
};

#include "semantic/world.h"

extern "C" void __attribute__((section(".text_80123BBC"))) sub_80123BBC(TM2Commander *self)
{
    if (self->target) {
        self->target->action(self->x, self->y, self->z, 99, -1, 0);
        taskMgrRunFrame(self->target);
        self->target->finish();
    }
}
