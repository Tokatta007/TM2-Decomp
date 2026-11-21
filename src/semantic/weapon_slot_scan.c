#include "semantic/cars.h"

#define HREC(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))

struct TM2SlotRecord {
    s16 id;
    s16 owner;
    s16 count;
    s16 field_6;
    s16 target_id;
    s16 field_A;
    void *origin;
    void *attachment;
};

struct TM2ActorView {
    u8 pad_0[0xA8];
    s32 position[3];
    u8 pad_B4[0x64];
    s32 identity;
};

struct TM2RegionRecord {
    u8 pad_0[12];
    s16 region;
};

#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
extern struct TM2SlotRecord weaponArcs[TM2_MAX_ARCS];

void *weaponLightningScanPos __attribute__((section(".tm2_80180B38"))) = 0;
/* Scripted scan point (x, y, z) at 80183F20. The distance test reads each
 * coordinate through its (one-element) array, i.e. through a pointer, so
 * -fvolatile re-reads it on every use; the setup stores are plain. */
extern s32 lightningScanOriginX[3];
extern s32 lightningScanOriginY[1], lightningScanOriginZ[1];
s32 weaponLightningScanOffset[3] __attribute__((section(".tm2_80162938"))) = {0, 0, 1064};

extern struct TM2ActorView *humanCarObject(s32 index);
extern struct TM2ActorView *aiCarObject(s32 index);
extern s32 humanCarCount(void);
extern s32 aiCarCount(void);
extern u32 gameGetLevel(void);
extern s32 weaponArcSlotFind(s32 id);
#include "semantic/world.h"
extern struct TM2RegionRecord *moscowEventGet(s32 index);
extern signed char *fxLightningSpark(void *position, u32 value);
extern void fxExplosion48(void *attachment, u8 value);
extern void fxSheetStart(s32 kind, s32 a1, s32 a2, s32 a3);

/* Distance from the scan point to a position: the largest axis difference
 * >> 3 (builtin abs, MAX re-evaluates its operands, as in area_damage.c). The
 * position is read through a struct pointer, so -fvolatile leaves it
 * non-volatile. */
#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
struct TM2ScanVec { s32 x, y, z; };
#define DISTANCE(px, py, pz, r) (MAX(ABS((px) - (r)->x), MAX(ABS((py) - (r)->y), ABS((pz) - (r)->z))) >> 3)

/* Fill weapon target slot SLOT for an actor (identity ID, position POS). */
#define SCAN_SLOT(slot, id, attach, from, pos)            \
    do {                                                        \
        weaponArcs[slot].count = 40;                            \
        if ((id) < 0) {                                         \
            weaponArcs[slot].target_id = -1;                    \
            weaponArcs[slot].attachment = 0;                    \
        } else {                                                \
            weaponArcs[slot].target_id = (id);                  \
            weaponArcs[slot].attachment = (attach);         \
        }                                                       \
        weaponArcs[slot].origin = (from);                     \
        weaponArcs[slot].field_6 = 1;                           \
        fxLightningSpark((pos), 1);                         \
    } while (0)

static __inline__ void scan_empty_slot(s32 slot, s32 *from)
{
    weaponArcs[slot].count = 40;
    weaponArcs[slot].target_id = -1;
    weaponArcs[slot].origin = from;
    weaponArcs[slot].attachment = 0;
    weaponArcs[slot].field_6 = 1;
}

/* @80107B00. Populate weapon target slots by region (mode 2) or by proximity
 * to the current scripted point (modes 1 and 8). C (PsyQ 3.6 CC1PSX,
 * -fvolatile). The shooter's own identity is read but unused; -fvolatile keeps
 * the load. */
s32 __attribute__((section(".text_80107B00")))
weaponLightningScan(void *car, s32 human)
{
    struct TM2ActorView *self, *actor;
    s32 actor_count, other_count, own_identity, identity;
    s32 i, j, slot_index, region, limit, result;

    if (human != 0)
        self = humanCarObject(HREC(car)->player);
    else
        self = aiCarObject(AI(car)->index);

    actor_count = humanCarCount();
    other_count = aiCarCount();
    own_identity = self->identity;

    switch (gameGetLevel()) {
    case 2:
        for (i = 0; i < actor_count; i++) {
            actor = humanCarObject(i);
            identity = actor->identity;
            weaponLightningScanPos = humanCarGet(i)->motion.pos;
            region = worldSectionAt(actor->position[0], actor->position[1], actor->position[2]);
            for (j = 1; j < 5; j++) {
                if (region == moscowEventGet(j)->region) {
                    slot_index = weaponArcSlotFind(i);
                    if (slot_index >= 0)
                        SCAN_SLOT(slot_index, identity, weaponLightningScanPos, weaponLightningScanOffset, actor->position);
                }
            }
        }
        for (i = 0; i < other_count; i++) {
            if (aiCarGet(i)->stats.hp > 0) {
                actor = aiCarObject(i);
                identity = actor->identity;
                weaponLightningScanPos = aiCarGet(i)->motion.pos;
                region = worldSectionAt(actor->position[0], actor->position[1], actor->position[2]);
                for (j = 1; j < 5; j++) {
                    if (region == moscowEventGet(j)->region) {
                        slot_index = weaponArcSlotFind(i + 100);
                        if (slot_index >= 0) {
                            SCAN_SLOT(slot_index, identity, weaponLightningScanPos, weaponLightningScanOffset, actor->position);
                            carSpinOut(aiCarGet(i), 0, 6, 0);
                            fxExplosion48(weaponLightningScanPos, 0);
                        }
                    }
                }
            }
        }
        slot_index = weaponArcSlotFind(-1);
        if (slot_index >= 0)
            scan_empty_slot(slot_index, weaponLightningScanOffset);
        result = 1;
        break;
    case 1:
    case 8:
        if (gameGetLevel() == 1) {
            limit = 250;
            lightningScanOriginX[0] = -4608;
            lightningScanOriginY[0] = -2192;
            lightningScanOriginZ[0] = 800;
        } else {
            limit = 350;
            lightningScanOriginX[0] = 0;
            lightningScanOriginY[0] = 0;
            lightningScanOriginZ[0] = 2560;
        }
        for (i = 0; i < actor_count; i++) {
            actor = humanCarObject(i);
            identity = actor->identity;
            weaponLightningScanPos = humanCarGet(i)->motion.pos;
            if (DISTANCE(*lightningScanOriginX, *lightningScanOriginY, *lightningScanOriginZ, (struct TM2ScanVec *)weaponLightningScanPos) < limit) {
                slot_index = weaponArcSlotFind(i);
                if (slot_index >= 0) {
                    SCAN_SLOT(slot_index, identity, weaponLightningScanPos, (s32 *)lightningScanOriginX, actor->position);
                    carSpinOut(humanCarGet(i), 1, 6, 0);
                    fxExplosion48(weaponLightningScanPos, 0);
                }
            }
        }
        for (i = 0; i < other_count; i++) {
            if (aiCarGet(i)->stats.hp > 0) {
                actor = aiCarObject(i);
                identity = actor->identity;
                weaponLightningScanPos = aiCarGet(i)->motion.pos;
                if (DISTANCE(*lightningScanOriginX, *lightningScanOriginY, *lightningScanOriginZ, (struct TM2ScanVec *)weaponLightningScanPos) < limit) {
                    slot_index = weaponArcSlotFind(i + 100);
                    if (slot_index >= 0) {
                        SCAN_SLOT(slot_index, identity, weaponLightningScanPos, (s32 *)lightningScanOriginX, actor->position);
                        carSpinOut(aiCarGet(i), 0, 6, 0);
                        fxExplosion48(weaponLightningScanPos, 0);
                    }
                }
            }
        }
        slot_index = weaponArcSlotFind(-1);
        if (slot_index >= 0)
            scan_empty_slot(slot_index, (s32 *)lightningScanOriginX);
        result = 1;
        break;
    default:
        result = 0;
        break;
    }

    if (result != 0)
        fxSheetStart(5, 250, 200, 200);
    return result;
}
