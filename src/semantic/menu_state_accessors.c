typedef signed int s32;
typedef unsigned int u32;
typedef unsigned char u8;

#include "semantic/settings.h"

extern void sceneCmdSetValue(void *, s32);

void __attribute__((section(".text_80137388"))) menuTextSetValue(u8 *p,s32 v){*(s32 *)(p+56)=v;}
void __attribute__((section(".text_801374B0"))) codeDetectorSetPort(void *p,s32 v){sceneCmdSetValue(p,v);}
void __attribute__((section(".text_80137614"))) menuSliderSetValue(u8 *p,s32 v){p[528]=v;}
unsigned short __attribute__((section(".text_80137620"))) menuSliderGetValue(u8 *p){return p[528];}
s32 __attribute__((section(".text_80137744"))) shellSetUnlockFlag(u8 *p){u8 *q=*(u8 **)(p+16);u8 v=p[20];*q=v;return v;}
u8 __attribute__((section(".text_80137798"))) settingsGetControlLayout(struct TM2Settings *settings,u8 player){return settings->control_layout[player];}
void __attribute__((section(".text_801377AC"))) settingsSetControlLayout(struct TM2Settings *settings,u8 player,u8 layout){settings->control_layout[player]=layout;}
u8 __attribute__((section(".text_801377C0"))) settingsGetWorldMap(struct TM2Settings *settings){return settings->world_map;}
void __attribute__((section(".text_801377CC"))) settingsSetWorldMap(struct TM2Settings *settings,u8 v){settings->world_map=v;}
u8 __attribute__((section(".text_801377D8"))) settingsGetDifficulty(struct TM2Settings *settings){return settings->difficulty;}
void __attribute__((section(".text_801377E4"))) settingsSetDifficulty(struct TM2Settings *settings,u8 v){settings->difficulty=v;}
s32 __attribute__((section(".text_801377F0"))) settingsGetDarkToothStage(struct TM2Settings *settings){return settings->dark_tooth_stage;}
void __attribute__((section(".text_801377FC"))) settingsSetDarkToothStage(struct TM2Settings *settings,u8 v){settings->dark_tooth_stage=v;}
s32 __attribute__((section(".text_80137808"))) settingsGetLevel(struct TM2Settings *settings){return settings->level;}
void __attribute__((section(".text_80137814"))) settingsSetLevel(struct TM2Settings *settings,s32 v){settings->level=v;}
#ifdef TM2_PORT_CAR_NUMBERS
/* request 42 item 3 (car_numbers.h): the number and the byte (the mod's base) */
void __attribute__((section(".text_80137854"))) settingsSetOpponentCar(struct TM2Settings *settings,s32 slot,s32 car){portCarNumberSet(settings,1,slot,car);}
#else
void __attribute__((section(".text_80137854"))) settingsSetOpponentCar(struct TM2Settings *settings,s32 slot,s32 car){TM2_SETTINGS_OPPONENT_CAR(settings,slot)=car;}
#endif
#if defined(TM2_PORT_CAR_NUMBERS)
void __attribute__((section(".text_801378A4"))) settingsSetHumanCar(struct TM2Settings *settings,s32 player,s32 car){portCarNumberSet(settings,0,player,car);}
#elif TM2_MAX_HUMANS > 2
/* players 3-4: port table */
void __attribute__((section(".text_801378A4"))) settingsSetHumanCar(struct TM2Settings *settings,s32 player,s32 car){TM2_SETTINGS_HUMAN_CAR(settings,player)=car;}
#else
void __attribute__((section(".text_801378A4"))) settingsSetHumanCar(struct TM2Settings *settings,s32 player,s32 car){settings->human_car[player]=car;}
#endif
s32 __attribute__((section(".text_801378DC"))) settingsGetOpponentsPicked(struct TM2Settings *settings){return settings->opponents_picked;}
void __attribute__((section(".text_801378E8"))) settingsSetOpponentsPicked(struct TM2Settings *settings,s32 v){settings->opponents_picked=v;}
s32 __attribute__((section(".text_801378F4"))) settingsGetCarSelectPlayer(struct TM2Settings *settings){return settings->car_select_player;}
void __attribute__((section(".text_80137900"))) settingsSetCarSelectPlayer(struct TM2Settings *settings,s32 v){settings->car_select_player=v;}
s32 __attribute__((section(".text_8013790C"))) settingsGetOpponents(struct TM2Settings *settings){return settings->opponents;}
void __attribute__((section(".text_80137918"))) settingsSetOpponents(struct TM2Settings *settings,s32 v){settings->opponents=v;}
s32 __attribute__((section(".text_80137924"))) settingsGetHumans(struct TM2Settings *settings){return settings->humans;}
void __attribute__((section(".text_80137930"))) settingsSetHumans(struct TM2Settings *settings,u8 v){settings->humans=v;}
s32 __attribute__((section(".text_8013793C"))) settingsGetMode(struct TM2Settings *settings){return settings->mode;}
void __attribute__((section(".text_80137948"))) settingsSetMode(struct TM2Settings *settings,s32 v){settings->mode=v;}
