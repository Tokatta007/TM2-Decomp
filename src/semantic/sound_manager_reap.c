typedef unsigned char u8;
typedef signed int s32;
typedef struct { s32 state; u8 pad[16]; } TM2ShellSeq;
extern TM2ShellSeq sndEffectTable[];
#include "semantic/sound.h"
extern void sndListStopById(void *,void *,s32);
extern void sndListPurgeFinished(void *);extern void sndListClear(void *);
extern int ResetGraph(s32);extern int SetGraphDebug(s32);extern void InitGeom(void);extern void InitTimer(void);extern int padInit(void);
extern s32 fileioLoadFileIntoRam(void *, void *);extern char aScreenUacoreVa[];extern void soundVabOpenRetry(void *);extern void lightInit(void);extern void csPoolInit(void);extern void rtSetRearView(s32);extern void viewInit(void);extern void renderInit(void);extern void uaInit(void);extern void bitmapDisableTracking(void);
extern void carTablesClear(void);extern void carPartsReset(void);extern void objRegistriesReset(void);extern void debrisInit(void);extern void renderClearModelLists(void);extern void clutAnimInit(void);extern void pickupSlotsReset(void);extern void targetPoolReset(void);extern void targetActorsReset(void);extern void worldResetLevelModelFlag(void);extern void arenaResetStub(void);extern void weaponObjectsDetach(void);extern void levelEventsReset(void);extern void worldLoadStartObjects(s32);extern void bulletsClear(void);extern void fxSpritesReset(void);extern void missilesCreateObjects(void);extern void missileTrailsInit(void);
#define S32(p,o) (*(s32 *)((u8 *)(p)+(o)))
void __attribute__((section(".text_801289B0"))) soundMgrStop(u8*owner,s32 type,s32 arg){switch(sndEffectTable[type].state){case 0:break;case 1:sndListStopById(owner+(type*20+0xcc),owner,arg);soundSetActive(type,0);break;}}
void __attribute__((section(".text_80128A1C"))) soundMgrReapFinished(u8*owner){s32 i;for(i=0;i<67;i++)sndListPurgeFinished(owner+0xcc+i*20);sndListPurgeFinished(owner+0x608);sndListPurgeFinished(owner+0x61c);sndListPurgeFinished(owner+0x630);sndListPurgeFinished(owner+0x644);sndListPurgeFinished(owner+0x658);sndListPurgeFinished(owner+0x680);sndListPurgeFinished(owner+0x66c);}
void __attribute__((section(".text_80128AA8"))) soundMgrClearAll(u8*owner){s32 i;for(i=0;i<67;i++)sndListClear(owner+0xcc+i*20);sndListClear(owner+0x608);sndListClear(owner+0x61c);sndListClear(owner+0x630);sndListClear(owner+0x644);sndListClear(owner+0x658);sndListClear(owner+0x66c);sndListClear(owner+0x680);}
void __attribute__((section(".text_80128B34"))) shellInitGraphicsSystem(void){ResetGraph(0);SetGraphDebug(0);InitGeom();InitTimer();padInit();}
void __attribute__((section(".text_80128C7C"))) shellLoadUaCore(void){fileioLoadFileIntoRam((void*)0x80010000,aScreenUacoreVa);soundVabOpenRetry((void*)0x80010000);lightInit();csPoolInit();rtSetRearView(0);viewInit();renderInit();uaInit();bitmapDisableTracking();}
void __attribute__((section(".text_80128CE4"))) arenaResetSystems(s32 mode){carTablesClear();carPartsReset();objRegistriesReset();csPoolInit();debrisInit();renderClearModelLists();rtSetRearView(0);viewInit();clutAnimInit();pickupSlotsReset();targetPoolReset();targetActorsReset();lightInit();worldResetLevelModelFlag();arenaResetStub();weaponObjectsDetach();levelEventsReset();worldLoadStartObjects(mode);bulletsClear();fxSpritesReset();missilesCreateObjects();missileTrailsInit();}
