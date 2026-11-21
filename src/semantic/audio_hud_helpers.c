union TM2HalfWord { int word; short half[2]; };
union TM2IconRecord { unsigned int words[2]; unsigned char bytes[8]; };
extern int sndListenerDist, sndListenerPan, audioMasterVolume;
extern union TM2HalfWord audioCdVolume;
extern union TM2IconRecord explodeArmorIconRect;
#pragma tm2_absolute sndListenerDist
#pragma tm2_absolute sndListenerPan
#pragma tm2_absolute audioMasterVolume
#pragma tm2_absolute audioCdVolume
extern int hudMessageCurrent[5];
extern int aiCarsCountAlive(void);
#include "semantic/hud.h"

/* @800D3590 */
int __attribute__((section(".text_800D3590"))) PadChkMtap(void) { return sndListenerDist; }
/* @800D35A0 */
int __attribute__((section(".text_800D35A0"))) soundGetListenerPan(void) { return sndListenerPan; }
/* @800D35B0 */
int __attribute__((section(".text_800D35B0"))) audioGetMasterVolume(void) { return audioMasterVolume; }
/* @800D35C0 */
int __attribute__((section(".text_800D35C0"))) audioGetCdVolume(void) { return audioCdVolume.word; }
/* @8011334C */
unsigned char *__attribute__((section(".text_8011334C"))) explodeGetArmorIconRect(void) { return explodeArmorIconRect.bytes; }

/* @800FBB6C. The callback may observe the intervening state writes. */
void __attribute__((section(".text_800FBB6C"))) hudMessageReset(void)
{
    hudMessageCurrent[0] = aiCarsCountAlive();
    hudMessageCurrent[2] = 50;
    hudMessageCurrent[4] = 50;
    hudMessageCurrent[1] = aiCarsCountAlive();
    hudMessageCurrent[3] = 50;
}

/* @800FBBC4 */
void __attribute__((section(".text_800FBBC4"))) hudMessagePost(int arg0, int arg1)
{
    if (arg1 < hudMessageCurrent[2]) {
        hudMessageCurrent[0] = arg0;
        hudMessageCurrent[2] = arg1;
        hudMessageCurrent[4] = 50;
        hudMessageCurrent[1] = aiCarsCountAlive();
        hudMessageCurrent[3] = 50;
    } else {
        hudMessageCurrent[1] = arg0;
        hudMessageCurrent[3] = arg1;
    }
}

/* @80112B5C. The callee writes an eight-byte record, not two byte globals. */
void __attribute__((section(".text_80112B5C"))) explodeStoreArmorIcon(void *arg0)
{
    grutilsParseSpriteRects(arg0, explodeArmorIconRect.bytes, 1);
    explodeArmorIconRect.bytes[0] += 6;
    explodeArmorIconRect.bytes[1] += 2;
}
