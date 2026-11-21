typedef signed int s32;
typedef unsigned char u8;
#include "semantic/settings.h"
#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
#include "semantic/car_groups.h"
#include "semantic/dt_player.h"   /* request 36 */
#include "semantic/car_numbers.h" /* request 42 item 3: portCarEnding */
#endif
/* the human's car (request 42 item 3: the car number, an int with TM2_PORT_CAR_NUMBERS, no narrowing) */
#ifdef TM2_PORT_CAR_NUMBERS
#define TM2_ENDING_CAR(i) settingsGetHumanCar(&sessionSettings, (i))
#else
#define TM2_ENDING_CAR(i) (signed char)settingsGetHumanCar(&sessionSettings, (i))
#endif
typedef s32 (*TM2Handler)(void);
/* endingMovieTable[14]: per-car ending movie {file, frame count, wide (480x240) flag} */
typedef struct TM2MediaEntry { const char *name; s32 frames; s32 wide; } TM2MediaEntry;


extern TM2MediaEntry endingMovieTable[];
extern char strMovieYouWon[];
extern s32 settingsGetHumans(struct TM2Settings *);
#include "semantic/scene.h"
extern s32 movieSkipCheck(void);
extern int sub_801414CC(s32);
extern s32 sub_8014510C(s32);

/* Winner ending player (docs/UNLOCKS.md): for each human entry in the settings
   list sessionSettings, plays the car's ending movie endingMovieTable[car] (skippable after
   frame 35 via movieSkipCheckDelayed) and then \MOVIES\YOUWON.STR (118 frames) except
   for Grasshopper (car 4); then clears the screen and returns the credits
   screen handler screenCredits. */
void * __attribute__((section(".text_80136D98"))) screenEnding(void)
{
    s32 index;
    for(index=0;index<settingsGetHumans(&sessionSettings);index++){
#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
        /* request 36 (DARK_TOOTH_PLAYER.md): Dark Tooth (index 14) has no entry in endingMovieTable: the port's
           movie (portDtPlayer.ending_movie), none when 0; request 42 item 3: nor has a mod (portModCars[m]); every
           other number past 13 none (retail read past the 14 entries); YOUWON.STR below as for the others */
        if((unsigned int)TM2_ENDING_CAR(index)>=14)
            portCarEnding(TM2_ENDING_CAR(index));
        else
#endif
        moviePlayEnding(&sessionSettings,
                     (char *)endingMovieTable[TM2_ENDING_CAR(index)].name,
                     endingMovieTable[TM2_ENDING_CAR(index)].frames,
                     endingMovieTable[TM2_ENDING_CAR(index)].wide,
                     movieSkipCheckDelayed);
        if(TM2_ENDING_CAR(index)!=4)
            moviePlayEnding(&sessionSettings,strMovieYouWon,0x76,1,movieSkipCheck);
    }
    sub_801414CC(0);
    sub_8014510C(0);
    screenClearScreen(0,0,320,240);
    screenDisplayToDisplay(0,1);
    screenSetEnv();
    return (void *)screenCredits;
}
