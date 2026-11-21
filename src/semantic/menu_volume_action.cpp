/* @80136FF8: options menu volume slider action. C++ (PsyQ 3.6 CC1PLPSX,
 * -fvolatile). MIN is a macro, so the slider value is read twice when it is the
 * smaller operand. The setters are inline: each stores the upper-clamped value
 * and then overwrites it with the lower-clamped one (retail behaviour). The
 * sound branch plays a confirmation through the global target's virtual calls. */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct TM2Target {
    char fields[200];
    virtual void action(int x, int y, int z, int kind, int arg, int flag);
    virtual void finish();
};

extern "C" {
#include "semantic/scene.h"
extern int audioCdVolume, audioMusicVolumeCap, audioSfxVolume, audioSfxVolumeCap, audioVolumeClampMax, audioVolumeClampMin;
extern TM2Target *soundManagerObject;
void sub_80145F8C(char a, short b, short c);
void audioSetMusicVolume(int);
#include "semantic/world.h"
}

struct TM2VolumeItem {
    char fields[12];
    void *slider;
    unsigned short is_music;
};

static inline void set_music_volume(int v)
{
    audioCdVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioCdVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
}

static inline void set_sound_volume(int v)
{
    audioSfxVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioSfxVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
}

extern "C" void __attribute__((section(".text_80136FF8"))) menuVolumeApply(TM2VolumeItem *item)
{
    if (item->is_music) {
        set_music_volume(MIN(menuSliderGetValue(item->slider) * 8, audioMusicVolumeCap));
        sub_80145F8C(0, audioCdVolume, audioCdVolume);
        audioSetMusicVolume(audioCdVolume);
    } else {
        set_sound_volume(MIN(menuSliderGetValue(item->slider) * 7, audioSfxVolumeCap));
        soundManagerObject->action(24, 0, 0, 99, -1, 0);
        taskMgrRunFrame(soundManagerObject);
        soundManagerObject->finish();
    }
}
