/* Car-selectable predicate (docs/UNLOCKS.md) over the match settings struct sessionSettings. */
typedef signed int s32;
typedef signed char s8;
typedef unsigned char u8;
#include "semantic/settings.h"
#include "semantic/car_groups.h"
#ifdef TM2_PORT_DT_PLAYER
#include "semantic/dt_player.h"   /* request 36 */
#endif
extern u8 unlockSweetTooth, unlockMinion;
extern s32 settingsGetHumans(struct TM2Settings *), settingsGetOpponents(struct TM2Settings *);
extern TM2CarNumber settingsGetOpponentCar(struct TM2Settings *, s32);

/* returns 1 if car `value` may be picked: cars 12/13 need their unlock flags (unlockSweetTooth/05),
   and the car must not already be in the player slots (settingsGetHumanCar) or opponent slots (settingsGetOpponentCar) */
s32 __attribute__((section(".text_8012CC10"))) carIsSelectable(s32 value)
{
    s32 i;

    if (value == 12 && unlockSweetTooth == 0)
        return 0;
    if (value == 13 && unlockMinion == 0) {
fail:
        return 0;
    }
#ifdef TM2_PORT_DT_PLAYER
    /* request 36 (DARK_TOOTH_PLAYER.md): Dark Tooth (index 14, reachable only on the extended carousel) needs
       portCarOptions bit 8, read live */
    if (value == TM2_DT_PLAYER_INDEX && !(portCarOptions & TM2_PORT_CAR_OPT_DT_PLAYER))
        goto fail;
#endif

#ifdef TM2_PORT_CAR_NUMBERS
    /* request 42 items 3 and 5: a number that is no car (the extended carousel's empty children 15..0x1F, an unused
       mod; 14 without TM2_PORT_DT_PLAYER) is never picked; the getters below compare numbers (ints) */
    if (!portCarNumberPickable(value))
        goto fail;
#endif
#ifdef TM2_PORT_CAR_GROUPS
    if (portCarOptions & 1)     /* request 32 (CAR_GROUPS.md): the port allows repeated cars */
        return 1;
#endif
    for (i = 0; (u8)i < settingsGetHumans(&sessionSettings); i++)
        if (settingsGetHumanCar(&sessionSettings, (u8)i) == value)
            goto fail;

    for (i = 0; (u8)i < settingsGetOpponents(&sessionSettings); i++)
        if (settingsGetOpponentCar(&sessionSettings, (u8)i) == value)
            goto fail;
    return 1;
}
