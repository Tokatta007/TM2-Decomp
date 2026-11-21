typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

extern s32 shellSetUnlockFlag(u8 *object);
extern void menuSelectIndexRun(void *cmd);
u32 menuSelectIndexVtbl[4] __attribute__((section(".tm2_800D030C")))={0,0,0,(u32)menuSelectIndexRun};
u32 shellSetUnlockFlagVtbl[4] __attribute__((section(".tm2_800D02FC")))={0,0,0,(u32)shellSetUnlockFlag};
extern u32 D_800D037C[],D_800D03AC[];
#include "semantic/scene.h"
extern void menuSliderSetValue(unsigned char *p, int v);

u32 __attribute__((section(".text_80137AD8"))) *menuSelectIndexInit(u32 *o,u32 owner,u32 value){o[2]=(u32)D_800D03AC;o[2]=(u32)D_800D037C;o[2]=(u32)menuSelectIndexVtbl;o[3]=owner;o[4]=value;return o;}
u32 __attribute__((section(".text_8013775C"))) *shellSetUnlockFlagInit(u32 *o,u32 owner,u32 value,s32 flag){o[2]=(u32)D_800D03AC;o[2]=(u32)D_800D037C;o[2]=(u32)shellSetUnlockFlagVtbl;o[3]=owner;o[4]=value;*(u8 *)(o+5)=(u8)flag;return o;}
void __attribute__((section(".text_801374D0"))) menuSliderDecRun(u32 *o){if(menuSliderGetValue((void *)o[3])!=0)menuSliderSetValue((void *)o[3],(u16)(menuSliderGetValue((void *)o[3])-1));}
void __attribute__((section(".text_80137570"))) menuSliderIncRun(u32 *o){if(menuSliderGetValue((void *)o[3])<16)menuSliderSetValue((void *)o[3],(u16)(menuSliderGetValue((void *)o[3])+1));}
