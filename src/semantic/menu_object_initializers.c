typedef signed int s32;
typedef unsigned int u32;

extern void menuSelectPrevRun(void *cmd);
extern void menuSelectNextRun(void *cmd);
extern int menuLoadPictureRun(void *cmd);
extern void menuHideTargetRun(unsigned char *object);
u32 menuSelectPrevVtbl[4] __attribute__((section(".tm2_800D031C"))) = {0,0,0,(u32)menuSelectPrevRun};
u32 menuSelectNextVtbl[4] __attribute__((section(".tm2_800D032C"))) = {0,0,0,(u32)menuSelectNextRun};
u32 menuLoadPictureVtbl[4] __attribute__((section(".tm2_800D033C"))) = {0,0,0,(u32)menuLoadPictureRun};
u32 menuHideTargetVtbl[4] __attribute__((section(".tm2_800D034C"))) = {0,0,0,(u32)menuHideTargetRun};
extern u32 D_800D037C[],D_800D038C[],D_800D03AC[];
extern char *strcpy(char *,const char *);

u32 __attribute__((section(".text_80137B34"))) *menuSelectPrevInit(u32 *object,u32 owner)
{
    object[2]=(u32)D_800D03AC;object[2]=(u32)D_800D037C;object[2]=(u32)menuSelectPrevVtbl;object[3]=owner;return object;
}

u32 __attribute__((section(".text_80137B8C"))) *menuSelectNextInit(u32 *object,u32 owner)
{
    object[2]=(u32)D_800D03AC;object[2]=(u32)D_800D037C;object[2]=(u32)menuSelectNextVtbl;object[3]=owner;return object;
}

u32 __attribute__((section(".text_80137C04"))) *menuLoadPictureInit(u32 *object,u32 owner,const char *name)
{
    object[2]=(u32)D_800D03AC;object[2]=(u32)D_800D037C;object[2]=(u32)menuLoadPictureVtbl;object[3]=owner;strcpy((char *)object+16,name);return object;
}

u32 __attribute__((section(".text_80137C84"))) *menuHideTargetInit(u32 *object,u32 owner)
{
    object[2]=(u32)D_800D03AC;object[2]=(u32)D_800D038C;object[2]=(u32)menuHideTargetVtbl;object[3]=owner;object[1]=0xffffffff;object[0]=object[1];return object;
}
