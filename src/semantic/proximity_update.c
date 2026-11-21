#include "semantic/weapons.h"
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif

#define HUMAN(p) ((struct TM2HumanRecord *)(p))
#define AI(p) ((struct TM2AIRecord *)(p))
/* target holds the crush target's scene object, then the peer's stats block */
#define TARGET_OBJ ((struct TM2SceneObject *)target)
#define TARGET_STATS ((struct TM2CarStatsBlock *)target)
extern u8 *carObjectFromOwner(s32 index);
extern s32 sub_8013FC44(s32 value);
extern u8 carFindByObject(void *target, u8 *side, s32 *index);
extern s32 rand(void);
extern void fxExplosion12(void *position, u8 value);
extern s32 GsGetWorkBase_0(void);

/* One contact point: distance from the target to wheel i; inside the crush
 * range it sets the contact flag and an intensity that falls from the peak
 * to half of it at the range edge. Range and peak are re-read at each use,
 * as in retail. */
#define CONTACT(i) \
    dx = TARGET_OBJ->pos[0] - state->wheel[i].pos[0]; \
    dy = TARGET_OBJ->pos[1] - state->wheel[i].pos[1]; \
    distance = sub_8013FC44(dx * dx + dy * dy); \
    if (distance < geometry->crush_range) { \
        flags->crush_contact = 1; \
        state->wheel[i].crush = geometry->crush_peak - geometry->crush_peak * distance / geometry->crush_range / 2; \
        if (distance < nearest) \
            nearest = distance; \
    } else \
        state->wheel[i].crush = 0

/* @800F39D0. Update four proximity/contact intensities for one car side.
 * The peer geometry reuses the target pointer. The outer clear sits in a
 * do/while(0) block, which doubles its weight in the register allocator's
 * reference count; that gives target s0 ahead of geometry, as in retail. */
void __attribute__((section(".text_800F39D0")))
carCrushContactUpdate(void *car,s32 side)
{
    struct TM2CarState *flags, *peer_flags;
    struct TM2CarMotion *motion;
    struct TM2CarWheels *state;
    struct TM2CarStatsBlock *geometry;
    u8 *target;             /* crush target's scene object, then the peer's stats */
    void *peer;             /* human (peer_side) or AI record */
    s32 nearest, distance, index, dx, dy;
    u8 peer_side;
    if (side & 0xFF) { flags = &HUMAN(car)->state; motion = &HUMAN(car)->motion; state = &HUMAN(car)->wheels; geometry = &HUMAN(car)->stats; }
    else { flags = &AI(car)->state; motion = &AI(car)->motion; state = &AI(car)->wheels; geometry = &AI(car)->stats; }
    flags->crush_contact = 0;
    target = carObjectFromOwner(geometry->crush_owner);
    nearest = 0x7FFF;
    if (target) {
        CONTACT(0);
        CONTACT(1);
        CONTACT(2);
        CONTACT(3);
    }
    if (target) {
        if (carFindByObject(target, &peer_side, &index) & 0xFF) {
            if (peer_side) { peer = humanCarGet(index); peer_flags = &HUMAN(peer)->state; target = (u8 *)&HUMAN(peer)->stats; }
            else { peer = aiCarGet(index); peer_flags = &AI(peer)->state; target = (u8 *)&AI(peer)->stats; }
        } else {
            peer_flags = 0;
            target = 0;
        }
    } else {
        peer_flags = 0;
        do target = 0; while (0);
    }
    if (flags->crush_contact && nearest < geometry->crush_range / 2) {
        carScrapeSparks(car, side & 0xFF);
        if ((rand() & 15) == 1)
            fxExplosion12(motion->pos, 0);
        if (peer_flags && TARGET_STATS->damageable != 0 && !(GsGetWorkBase_0() & 3) && TARGET_STATS->hp > 0)
            TARGET_STATS->hp -= 1;
#ifdef TM2_PORT_BALANCE
        if (peer_flags)
            portBalanceCrushTop(car, side & 0xFF, peer_side, index);   /* request 40 bit 3: Hammerhead's +5, once */
#endif
    }
    if (flags->crush_contact == 0) {
        state->wheel[0].crush = 0; state->wheel[1].crush = 0; state->wheel[2].crush = 0; state->wheel[3].crush = 0;
        if (peer_flags)
            peer_flags->crush_peer = 0;
#ifdef TM2_PORT_BALANCE
        portBalanceCrushEnd(car, side & 0xFF);   /* request 40 bit 3: the ride ended, nothing owed */
#endif
    }
}
