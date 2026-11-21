typedef signed int s32;typedef unsigned char u8;
extern s32 audioSfxVolume;
#pragma tm2_absolute audioSfxVolume
extern s32 audioVolumeClampMax;
#pragma tm2_absolute audioVolumeClampMax
extern s32 audioVolumeClampMin;
#pragma tm2_absolute audioVolumeClampMin
extern u8 strNumberedTimFmt[],strShellCreditStem[];
extern s32 audioGetMusicVolume(void);extern void soundUpdate(void);
#include "semantic/sound.h"
#include "semantic/scene.h"
extern s32 screenDisplayImage(s32, void*, s32, s32);
#include "semantic/input.h"
extern s32 sub_801388B8(char*,void*,void*,s32);

/* Store v into audioSfxVolume clamped first to the maximum audioVolumeClampMax and then
   (from the unclamped v) to the minimum audioVolumeClampMin. */
static __inline__ void credit_set_level(s32 v)
{
 s32 t=v;if(audioVolumeClampMax<v)t=audioVolumeClampMax;audioSfxVolume=t;
 audioSfxVolume=(v<audioVolumeClampMin)?audioVolumeClampMin:v;
}

/* Credits / picture slideshow screen: temporarily sets the clamped level
   audioSfxVolume from audioGetMusicVolume() around a soundPlay(27,0,0,99) call, then
   restores it and shows images 1..7 (name built with sprintf), each until
   padWaitConfirmOrTimeout(0,360) reports a button / timeout. Returns the next screen
   handler (back to screenHistory if called from it, else screenTitle). */
void * __attribute__((section(".text_80136A3C"))) screenCredits(void *caller)
{
 s32 original=audioSfxVolume;s32 index;char name[24];
 credit_set_level(audioGetMusicVolume());
 soundPlay(27,0,0,99);soundUpdate();soundFlush();
 credit_set_level(original);
 for(index=1;index<8;index++){sub_801388B8(name,strNumberedTimFmt,strShellCreditStem,index);screenSetEnv();screenLoadImageFile(name,0);screenDisplayImage(1,(void*)0x80048270,0,0);if(padWaitConfirmOrTimeout(0,360)!=0)break;}
 if(caller==(void*)screenHistory)return (void*)screenHistory;return (void*)screenTitle;
}
