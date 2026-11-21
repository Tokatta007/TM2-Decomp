#include "semantic/settings.h"
/* @801368D8: intro screen setup. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile). Returns the
 * next state handler (screenHistory). The level setter is an inline function: it
 * stores the upper-clamped value and then overwrites it with the lower-clamped
 * value of the same input, exactly as retail does (two stores, upper clamp lost). */
extern "C" {
extern unsigned char strMovieIntro[];
extern int audioSfxVolume, audioVolumeClampMax, audioVolumeClampMin;
#include "semantic/sound.h"
void soundUpdate(void); 
#include "semantic/scene.h"
int sub_801414CC(int); int sub_8014510C(int); 
int audioGetMusicVolume(void); int movieSkipCheck(void); 
}
static inline void set_level(int v)
{
    audioSfxVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioSfxVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
}
extern "C" int (* __attribute__((section(".text_801368D8"))) screenHistoryMovie(void))(void)
{
    soundStop(27); soundUpdate(); soundFlush();
    moviePlayEnding(&sessionSettings, strMovieIntro, 0x4e5, 1, movieSkipCheck);
    sub_801414CC(0); sub_8014510C(0); screenClearScreen(0, 0, 320, 240); screenDisplayToDisplay(0, 1); screenSetEnv();
    int original = audioSfxVolume;
    set_level(audioGetMusicVolume());
    soundPlay(27, 0, 0, 99); soundUpdate(); soundFlush();
    set_level(original);
    return (int (*)(void))screenHistory;
}
