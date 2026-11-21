/* Tournament opponent picker over the match settings struct sessionSettings (docs/MATCH_START.md). */
typedef signed int s32;
typedef unsigned char u8;
#include "semantic/settings.h"
extern u8 unlockSweetTooth;
#include "semantic/car_calls.h"
extern s32 settingsGetDarkToothStage(struct TM2Settings *), settingsGetOpponents(struct TM2Settings *), rand(void);

/* if settings mode flag (settingsGetDarkToothStage) is set, put car 14 in slot 0; otherwise fill every opponent
   slot with a random car 0..12 accepted by carIsSelectable (car 12 temporarily allowed) */
void __attribute__((section(".text_8012CD08"))) tournamentPickOpponents(void)
{
    s32 index;
    u8 saved;

    if ((u8)settingsGetDarkToothStage(&sessionSettings) != 0) {
        settingsSetOpponentCar(&sessionSettings, 0, 14);
        return;
    }
    saved = unlockSweetTooth;
    unlockSweetTooth = 1;
    for (index = 0; (u8)index < settingsGetOpponents(&sessionSettings); index++) {
        s32 value, r;
        do {
            r = rand();
            value = r - (r / 13) * 13; /* r % 13, spelled out for register allocation */
#ifdef TM2_PORT_CARS
            /* request 32 step 2: past the 13 - humans distinct cars a repeat is taken (retail would loop
               forever; never reached with retail's 9 opponents at most) */
        } while (carIsSelectable(value & 0xff) == 0 && (u8)index < 13 - sessionSettings.humans);
#else
        } while (carIsSelectable(value & 0xff) == 0);
#endif
        settingsSetOpponentCar(&sessionSettings, (u8)index, (u8)value);   /* retail narrows both here */
    }
    unlockSweetTooth = saved;
}
