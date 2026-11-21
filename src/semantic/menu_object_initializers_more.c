typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

extern void menuVolumeApply(void *item);
extern void menuSliderIncRun(unsigned int *object);
extern void menuSliderDecRun(unsigned int *object);
extern void menuShowTargetRun(unsigned char *object);
extern void menuPlaySoundRun(void *object);
extern int menuCopyChoiceRun(unsigned char *object);
u32 menuVolumeApplyVtbl[4] __attribute__((section(".tm2_800D026C")))={0,0,0,(u32)menuVolumeApply};
u32 menuCopyChoiceVtbl[4] __attribute__((section(".tm2_800D027C")))={0,0,0,(u32)menuCopyChoiceRun};
u32 menuSliderDecVtbl[4] __attribute__((section(".tm2_800D02AC")))={0,0,0,(u32)menuSliderDecRun};
u32 menuSliderIncVtbl[4] __attribute__((section(".tm2_800D02BC")))={0,0,0,(u32)menuSliderIncRun};
u32 menuShowTargetVtbl[4] __attribute__((section(".tm2_800D035C")))={0,0,0,(u32)menuShowTargetRun};
u32 menuPlaySoundVtbl[4] __attribute__((section(".tm2_800D036C")))={0,0,0,(u32)menuPlaySoundRun};
extern u32 D_800D038C[],menuSoundActionBaseVtbl[],D_800D03AC[];

u32 __attribute__((section(".text_80137200"))) *menuVolumeApplyInit(u32 *o,u32 owner,s16 value){o[2]=(u32)D_800D03AC;o[2]=(u32)D_800D038C;o[2]=(u32)menuVolumeApplyVtbl;o[3]=owner;*(u16 *)(o+4)=(u16)value;o[1]=0xffffffff;o[0]=o[1];return o;}
u32 __attribute__((section(".text_80137288"))) *menuCopyChoiceInit(u32 *o,u32 owner,u32 value){o[2]=(u32)D_800D03AC;o[2]=(u32)D_800D038C;o[2]=(u32)menuCopyChoiceVtbl;o[3]=owner;o[4]=value;o[5]=owner;return o;}
u32 __attribute__((section(".text_80137528"))) *menuSliderDecInit(u32 *o,u32 owner){o[2]=(u32)D_800D03AC;o[2]=(u32)D_800D038C;o[2]=(u32)menuSliderDecVtbl;o[3]=owner;o[1]=0xffffffff;o[0]=o[1];return o;}
u32 __attribute__((section(".text_801375CC"))) *menuSliderIncInit(u32 *o,u32 owner){o[2]=(u32)D_800D03AC;o[2]=(u32)D_800D038C;o[2]=(u32)menuSliderIncVtbl;o[3]=owner;o[1]=0xffffffff;o[0]=o[1];return o;}
u32 __attribute__((section(".text_80137CF0"))) *menuShowTargetInit(u32 *o,u32 owner){o[2]=(u32)D_800D03AC;o[2]=(u32)D_800D038C;o[2]=(u32)menuShowTargetVtbl;o[3]=owner;o[1]=0xffffffff;o[0]=o[1];return o;}
u32 __attribute__((section(".text_80137DE8"))) *menuPlaySoundInit(u32 *o,u32 actor,u32 a,u32 b,u32 c){o[2]=(u32)D_800D03AC;o[2]=(u32)menuSoundActionBaseVtbl;o[2]=(u32)menuPlaySoundVtbl;o[7]=actor;o[4]=a;o[5]=b;o[6]=c;return o;}
