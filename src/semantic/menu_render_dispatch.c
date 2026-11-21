typedef signed int s32;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern s32 sceneObjGetChoiceState(s32);
/* sceneObjSetChoiceState is void (v0 at its return is left over from its last call), but retail
 * returns its result here, so this unit keeps the int-returning view it was compiled with. */
extern s32 sceneObjSetChoiceState(s32,s32);
extern s32 sub_801414CC(s32);
#include "semantic/hud.h"
extern u32 menuTextDrawMode[];
extern u8 menuTextSprites[];

/* Returns sceneObjSetChoiceState(object+20 value, sceneObjGetChoiceState(object+16 value)). */
s32 __attribute__((section(".text_8013724C"))) menuCopyChoiceRun(u8 *object)
{
    return sceneObjSetChoiceState(*(s32 *)(object+20),sceneObjGetChoiceState(*(s32 *)(object+16)));
}

/* Menu text-item draw method: when the item is visible (+8 != 0), waits for
   DrawSync and prints its value (+56) at (+62,+64) with font id +52, using
   fontSpritePrintCenteredXY when mode (+60) is set, else fontSpritePrintXY. */
void __attribute__((section(".text_801372C4"))) menuTextDraw(u8 *object,u32 *value)
{
    if(*(u16 *)(object+8)!=0){
        sub_801414CC(0);
        if(object[60]!=0)
            fontSpritePrintCenteredXY(object[52],*(u16 *)(object+62),*(u16 *)(object+64),
                         menuTextDrawMode,menuTextSprites,*(void **)(object+56),(u32 *)*value);
        else
            fontSpritePrintXY(object[52],*(u16 *)(object+62),*(u16 *)(object+64),
                         menuTextDrawMode,menuTextSprites,*(void **)(object+56),(u32 *)*value);
    }
}
