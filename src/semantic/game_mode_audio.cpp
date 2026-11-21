/* Game mode audio: sets the mode on the active mode target and applies the
 * serial (CD) volume. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile). */
typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;
#include "semantic/settings.h"

/* g++ 2.x object: vtable pointer at +8; set_mode() is vtable entry 2. */
struct TM2ModeTarget {
    int f0, f4;
    virtual void f1();
    virtual void set_mode(int mode);
};

extern "C" {

extern TM2ModeTarget *musicManagerObject;
/* Retail addresses the volume settings absolutely, not through $gp. */
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern s32 audioCdVolume, audioVolumeClampMax, audioVolumeClampMin;
extern void roundStart(void);
extern s32 sessionGetMode(struct TM2Settings *);
#include "semantic/sound.h"
extern s32 audioGetMusicVolume(void);
extern void SsSetSerialVol(char s_num, short voll, short volr);

/* @80128DB4. The first volume clamp keeps the unclamped value in a copy
 * taken on both paths; the do/while(0) only weights the register allocator
 * (retail register choice). */
void __attribute__((section(".text_80128DB4"))) roundStartMusic(s32 requested)
{
    s32 raw, value, low;
    roundStart();
    if (sessionGetMode(&sessionSettings) == 0)
        musicManagerObject->set_mode(10);
    else if ((u8)audioGetDarkToothMusic() != 0)
        musicManagerObject->set_mode(11);
    else if (requested == 10)
        musicManagerObject->set_mode(8);
    else if (requested == 11)
        musicManagerObject->set_mode(2);
    else if (requested == 12)
        musicManagerObject->set_mode(6);
    else
        musicManagerObject->set_mode(requested + 1);
    low = audioGetMusicVolume();
    if (audioVolumeClampMax < low) {
        raw = low;
        do low = audioVolumeClampMax; while (0);
    } else
        raw = low;
    audioCdVolume = low;
    value = raw < audioVolumeClampMin ? audioVolumeClampMin : raw;
    audioCdVolume = value;
    SsSetSerialVol(0, (s16)value, (s16)value);
}
}
