typedef signed int s32;typedef unsigned char u8;
#include "semantic/settings.h"
extern u8 strMovieTm2In[],strShellTitleTim[],D_80048270[];
#include "semantic/scene.h"
extern s32 screenDisplayImage(s32, void*, s32, s32);
extern void sub_80141BA0(void*);
extern void *sub_80143E64(void*, s32, s32, s32, s32);
extern void *sub_80143DA4(void*, s32, s32, s32, s32);
extern void *sub_80141AA4(void*);
extern s32 movieSkipCheck(void);extern s32 shellAttractDemo(s32, s32);
void * __attribute__((section(".text_8012DD74"))) shellPlayTitleIntro(s32 unused0,s32 unused1)
{
 u8 display[24],draw[96];moviePlayEnding(&sessionSettings,strMovieTm2In,0x53,1,movieSkipCheck);screenLoadImageFile(strShellTitleTim,0);screenDisplayImage(0,(u8 *)0x80048270,0,0);sub_80143E64(display,0,0,320,240);sub_80141BA0(display);sub_80143DA4(draw,0,0,320,240);sub_80141AA4(draw);return (void*)screenTitle;
}
void * __attribute__((section(".text_8012DE3C"))) shellAttractCycle(void *value)
{
 shellAttractDemo(0,0);shellPlayTitleIntro(0,0);return value;
}
