typedef signed int s32;
typedef unsigned char u8;
#include "semantic/settings.h"
extern u8 strShellStopTim[],strShellCoWinTim[];
extern void settingsSetLevel(struct TM2Settings *,s32);
extern s32 sub_8014510C(s32);
extern int sub_801414CC(s32);
#include "semantic/scene.h"
extern s32 sessionRun(struct TM2Settings *),settingsGetMode(struct TM2Settings *),settingsGetLevel(struct TM2Settings *),tournamentOpponentCount(struct TM2Settings *,s32),settingsGetHumans(struct TM2Settings *);
extern s32 screenDisplayImage(s32, void*, s32, s32);
#include "semantic/input.h"
extern s32 screenPickOpponents(void),screenEnding(void);

/* @80136BB0: the scrolling text screen's frame handler; returns the next
 * screen handler. */
void * __attribute__((section(".text_80136BB0"))) screenScrollText(void)
{
    s32 option, valid, stop, value;
    sub_801414CC(0);
    sub_8014510C(0);
    option = sessionRun(&sessionSettings);
    valid = 0;
    if (settingsGetMode(&sessionSettings) == 1 || settingsGetMode(&sessionSettings) == 2)
        valid = 1;
    if (valid) {
        switch (option) {
        case 1:
            settingsSetLevel(&sessionSettings, 1);
            return (void *)screenTitle;
        case 2:
            settingsSetLevel(&sessionSettings, 1);
            return (void *)screenTitle;
        case 3:   /* no-op in retail; shapes the case tree */
            break;
        }
        stop = 0;
        if (settingsGetLevel(&sessionSettings) == 4)
            stop = (u8)settingsGetDifficulty(&sessionSettings) == 0;
        if (stop) {
            sub_801414CC(0);
            screenSetEnv();
            screenLoadImageFile(strShellStopTim, 0);
        } else if (settingsGetLevel(&sessionSettings) < 8) {
            settingsSetLevel(&sessionSettings, settingsGetLevel(&sessionSettings) + 1);
            value = tournamentOpponentCount(&sessionSettings, settingsGetLevel(&sessionSettings));
            settingsSetOpponents(&sessionSettings, value - settingsGetHumans(&sessionSettings));
            return (void *)screenPickOpponents;
        } else {
            if (settingsGetMode(&sessionSettings) == 1)
                return (void *)screenEnding;
            sub_801414CC(0);
            screenSetEnv();
            screenLoadImageFile(strShellCoWinTim, 0);
        }
        screenDisplayImage(1, (u8 *)0x80048270, 0, 0);
        padWaitConfirmOrTimeout(300, 600);
    }
    return (void *)screenTitle;
}
