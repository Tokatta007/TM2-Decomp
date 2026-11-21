typedef signed int s32;typedef unsigned char u8;
#include "semantic/settings.h"
extern s32 sceneCmdPadInputRun(void*);extern s32 sub_8012DB94(void*,s32),carDisplayName(struct TM2Settings *,s32),levelDisplayName(struct TM2Settings *,s32);extern u8 aGettingRandomC[];
s32 __attribute__((section(".text_8012DCA4"))) menuCodeDetectorRun(u8 *object)
{
 s32 result;sceneCmdPadInputRun(object);if(*(s32*)(object+52)>0)--*(s32*)(object+52);result=*(s32*)(object+20);if(result>=0)result=sub_8012DB94(object,*(s32*)(object+20));return result;
}
s32 __attribute__((section(".text_8012DD0C"))) menuCarNameText(s32 value){return carDisplayName(&sessionSettings,value);}
void * __attribute__((section(".text_8012DD38"))) menuRandomCarText(void){return aGettingRandomC;}
s32 __attribute__((section(".text_8012DD48"))) menuLevelNameText(s32 value){return levelDisplayName(&sessionSettings,value+1);}
