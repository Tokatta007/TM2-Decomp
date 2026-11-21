typedef unsigned char u8;typedef signed short s16;typedef signed int s32;typedef void(*VirtualInit)(void*);
extern void *soundManagerObject;extern void *musicManagerObject;
#include "semantic/scene.h"
extern int sub_80145DE8(s32);extern void SetDispMask(s32);extern int ResetCallback(void);extern void shellInitGraphicsSystem(void);extern void *renderGetPacketBuffer(void);extern void *renderGetOrderingTable(void);extern void fileioInit(void*,void*);extern void soundDriverInit(void);extern void sndPoolReset(void);extern void soundInit(void);extern void sndListenerInitDefaults(void);extern void cdAudioInit(void *player);extern void soundMgrInitLists(void*);extern void fontInit(void);extern s32 screenInit(void);extern void *rtDoubleBufferInit(void);
/* One-time machine bring-up: display off, ResetCallback, graphics system,
   subsystem inits (incl. soundInit), shell object soundManagerObject, a virtual init
   call on musicManagerObject, screen init with both buffers cleared, display on and
   double buffering. */
void __attribute__((section(".text_80128B74"))) shellInitMachine(void){u8*object;u8*vtable;void*a;void*b;VirtualInit init;s16 delta;sub_80145DE8(0);SetDispMask(0);ResetCallback();shellInitGraphicsSystem();a=renderGetPacketBuffer();b=renderGetOrderingTable();fileioInit(a,b);soundDriverInit();sndPoolReset();soundInit();sndListenerInitDefaults();/* retail passes no player: cdAudioInit's a0 only reaches the empty cdPlayerBaseInit */((void (*)(void))cdAudioInit)();soundMgrInitLists(soundManagerObject);object=musicManagerObject;vtable=*(u8**)(object+8);delta=*(s16*)(vtable+8);init=*(VirtualInit*)(vtable+12);init(object+delta);fontInit();screenInit();screenClearScreen(0,0,320,240);screenClearScreen(320,0,320,240);screenSetEnv();SetDispMask(1);rtDoubleBufferInit();}
