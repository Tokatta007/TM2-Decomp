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

struct TM2Position {
    u8 pad_0[0x20];
    s32 x;
    s32 y;
    s32 z;
};

struct TM2ActorView {
    u8 pad_0[0xA8];
    s32 x;
    s32 y;
    s32 z;
    u8 pad_B4[0x64];
    s32 identity;
};

#include "semantic/port_cars.h"   /* pool limits (players.h), TM2_POOL_LIVE */
extern struct TM2SlotRecord weaponArcs[TM2_MAX_ARCS];
void *weaponTaserVictim __attribute__((section(".tm2_80180F10"))) = 0;

extern s32 weaponArcSlotFind(s32 id);
extern struct TM2ActorView *humanCarObject(s32 index);
extern struct TM2ActorView *aiCarObject(s32 index);
extern s32 humanCarCount(void);
extern s32 aiCarCount(void);

#define ABS(x) __builtin_abs(x)
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define DISTANCE(l, r) (MAX(ABS((l)->x - (r)->x), MAX(ABS((l)->y - (r)->y), ABS((l)->z - (r)->z))) >> 3)
#define SLOT weaponArcs[slot_index]
#include "semantic/car_numbers.h"   /* request 73: TM2_SPECIAL_NUM (a mod car's taser reach and arc) */
/* the firer's car slot, for its numbers (evaluated only with TM2_PORT_SPECIAL_PROFILE) */
#define TASER_SLOT (human != 0 ? HREC(car)->slot : AI(car)->slot)
#include "semantic/port_team_locks.h"   /* request 123 item 3: no teammate in a team game */

/* @801075C8. Allocate the requested five-entry weapon slot and associate it
 * with the closest actor from either active actor collection. car is a human
 * record when human is set, else an AI record. */
s32 __attribute__((section(".text_801075C8")))
weaponTaserTarget(void *car, s32 human, s32 id)
{
    struct TM2Position *origin;
    struct TM2ActorView *self;
    struct TM2ActorView *actor;
    s32 slot_index;
    s32 self_identity;
    s32 best_identity;
    s32 best_distance;
    s32 group_count;
    s32 other_count;
    s32 distance;
    s32 index;
    s32 selected_group;
    void *attachment = 0;

    if (human != 0) {
        origin = (struct TM2Position *)&HREC(car)->motion;
        self = humanCarObject(HREC(car)->player);
    } else {
        origin = (struct TM2Position *)&AI(car)->motion;
        self = aiCarObject(AI(car)->index);
    }

    slot_index = weaponArcSlotFind(id);
    if (slot_index < 0)
        return 0;

    best_identity = -1;
    group_count = humanCarCount();
    other_count = aiCarCount();
    best_distance = TM2_SPECIAL_NUM(TASER_SLOT, TM2_SPN_RANGE, 80);
    index = 0;
    selected_group = 0;
    self_identity = self->identity;

    for (; index < group_count; index++) {
        actor = humanCarObject(index);
        distance = DISTANCE(origin, actor);
        if (actor->identity != self_identity && distance < TM2_SPECIAL_NUM(TASER_SLOT, TM2_SPN_RANGE, 80) && distance < best_distance
            && TM2_TEAM_FOE(TASER_SLOT, humanCarGet(index)->slot)) {
            best_distance = distance;
            best_identity = actor->identity;
            attachment = humanCarGet(index)->motion.pos;
            weaponTaserVictim = humanCarGet(index);
            selected_group = 1;
        }
    }

    for (index = 0; index < other_count; index++) {
        if (aiCarGet(index)->stats.hp > 0) {
            actor = aiCarObject(index);
            distance = DISTANCE(origin, actor);
            if (actor->identity != self_identity && distance < TM2_SPECIAL_NUM(TASER_SLOT, TM2_SPN_RANGE, 80) && distance < best_distance
                && TM2_TEAM_FOE(TASER_SLOT, aiCarGet(index)->slot)) {
                best_distance = distance;
                best_identity = actor->identity;
                attachment = aiCarGet(index)->motion.pos;
                weaponTaserVictim = aiCarGet(index);
                selected_group = 0;
            }
        }
    }

    SLOT.count = TM2_SPECIAL_NUM(TASER_SLOT, TM2_SPN_DURATION, 40);
    if (best_identity < 0) {
        SLOT.id = -1;
        SLOT.owner = -1;
        SLOT.target_id = -1;
        SLOT.attachment = 0;
    } else {
        SLOT.id = (u16)id;
        if (human != 0) {
            SLOT.owner = *(u16 *)&HREC(car)->slot;
            carSpinOut(weaponTaserVictim, selected_group, 6, 0);
        } else {
            SLOT.owner = *(u16 *)&AI(car)->slot;
            carSpinOut(weaponTaserVictim, selected_group, 6, 0);
        }
        SLOT.target_id = (s16)best_identity;
        SLOT.attachment = attachment;
    }
    SLOT.field_6 = 0;
    SLOT.origin = &origin->x;
    return 1;
}
