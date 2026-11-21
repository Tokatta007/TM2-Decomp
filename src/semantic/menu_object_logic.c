typedef signed int s32;
typedef unsigned int u32;
typedef unsigned char u8;
#include "semantic/scene.h"

extern void menuTextDraw(u8 *object, u32 *context);
u32 menuTextVtbl[4] __attribute__((section(".tm2_800D028C")))={0,0,0,(u32)menuTextDraw};
#include "semantic/hud.h"
extern s32 func_80145EC4(void *);

void __attribute__((section(".text_80137394"))) menuTextDestroy(u8 *object,s32 mode)
{
    *(u32 *)(object+48)=(u32)menuTextVtbl;
    fontResetColor(object[52]);
    if(mode&1)func_80145EC4(object);
}

/* @80137E78: step a wrapping index backwards. Returns nothing: v0 merely keeps
   the intermediate sum (verified by byte match, PsyQ 3.6 + -fvolatile). */
void __attribute__((section(".text_80137E78"))) sceneSelectPrev(struct TM2ShellScene *scene)
{
    scene->selection=(scene->selection+scene->item_count-1)%scene->item_count;
}
