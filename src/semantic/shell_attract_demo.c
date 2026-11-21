typedef signed int s32;typedef unsigned char u8;
#include "semantic/settings.h"
extern s32 attractDemoLevel;
extern void *settingsCopy(void*,void*);extern void settingsSetMode(struct TM2Settings *,s32),settingsSetLevel(struct TM2Settings *,s32),settingsClearOpponentCars(struct TM2Settings *),tournamentPickOpponents(void);
extern s32 sub_8014510C(s32);
extern int sub_801414CC(s32);
extern s32 rand(void),sessionRun(struct TM2Settings *);

/* Attract-mode demo match (docs/MATCH_START.md): saves the match settings
   sessionSettings, sets mode 0 (attract), arena attractDemoLevel, 1 human with a random
   car (rand()%13), 4 random computer opponents, runs the match with
   sessionRun, restores the settings and advances the demo arena
   attractDemoLevel (1..8, wrapping to 1). Returns 0. shellAttractCycle calls it
   with (0, 0) like a screen handler; the arguments are ignored. */
s32 __attribute__((section(".text_8012DE7C"))) shellAttractDemo(s32 unused0, s32 unused1)
{
 u8 saved[48];
 settingsCopy(saved,&sessionSettings);
#ifdef TM2_PORT_CAR_NUMBERS
 portCarNumbersKeep(0);     /* request 42 item 3: settingsCopy keeps the bytes only */
#endif
 settingsSetMode(&sessionSettings,0);
 settingsSetLevel(&sessionSettings,attractDemoLevel);
 settingsSetHumans(&sessionSettings,1);
 settingsSetCarSelectPlayer(&sessionSettings,1);
 settingsSetHumanCar(&sessionSettings,0,rand()%13);
 settingsSetOpponents(&sessionSettings,4);
 settingsClearOpponentCars(&sessionSettings);
 tournamentPickOpponents();
 sub_801414CC(0);
 sub_8014510C(0);
 sessionRun(&sessionSettings);
 settingsCopy(&sessionSettings,saved);
#ifdef TM2_PORT_CAR_NUMBERS
 portCarNumbersKeep(1);
#endif
 if(attractDemoLevel<8)attractDemoLevel++;else attractDemoLevel=1;
 return 0;
}
