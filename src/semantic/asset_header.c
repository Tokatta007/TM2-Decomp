extern void exit(int) __attribute__((noreturn));
extern void renderSetWorldShift(int);
extern void modelRelocate(void *, unsigned int);
extern void worldLoadObject(int, void *, int *);
extern void dbscanHierarchy(void *, int, void (*)(int, void *, int *));
#include "semantic/port_far_lod.h"
#include "semantic/port_rollback.h"

/* @8018104C, @80181050: full-word game state, zeroed by retail CRT. */
int assetInstallShift __attribute__((section(".tm2_8018104C"))) = 0;
int vramTexSlotGroup __attribute__((section(".tm2_80181050"))) = 0;

/* @80120EF4: install a "PSXD" asset image (version 0x43): sets the difficulty from the
 * header, then relocates each listed object (serialized pointers use a fixed image origin:
 * the relocation delta is the buffer address plus 0x7ffe63c0, modulo 32 bits) and
 * registers it. A zero relocated object stops the walk. Bad magic/version -> exit(-1). */
void __attribute__((section(".text_80120EF4")))
assetInstallImage(unsigned int *base, int arg1)
{
    unsigned int delta = (unsigned int)base + 0x7ffe63c0u;
    unsigned int count, i;
    void *object;
    unsigned int *p = base;   /* cursor copied after delta: keeps the constant ahead of the copy */

    vramTexSlotGroup = arg1;
    if (*p != 0x50535844u)
        exit(-1);
    p++;
    if (*p != 0x43u)
        exit(-1);
    p += 3;
#ifdef TM2_PORT_ROLLBACK
    /* PC port request 54 (ONLINE_ROLLBACK.md 15.1): note the file (portDmdImages), so portDmdMutableList lists every
       model of every file loaded for the match, held by an object or not. Bookkeeping only: no game state. */
    portDmdNoteImage(base, arg1);
#endif
    renderSetWorldShift((int)*p);
    assetInstallShift = (int)*p;
    p += 2;
    count = *p++;
    for (i = 0; i < count; i++) {
        object = (void *)(*p + delta);
        if (object == 0)
            return;
        modelRelocate(object, delta);
#ifdef TM2_PORT_FAR_LOD
        /* PC port request 30 (FAR_LOD.md): this root's LOD records to extend. */
        portFarLodScan(object);
#endif
        dbscanHierarchy(object, assetInstallShift, worldLoadObject);
        p++;
    }
}
