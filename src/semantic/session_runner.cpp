/* Session runner (8012BE5C, IDA "sessionRun"). Sets the
 * split-screen mode and the players' cars, then runs rtMainLoop until the
 * session ends, handling each round's result per session mode (lives and
 * GAME OVER, boss introductions, next-level messages, two-player wins).
 * C++ (PsyQ 3.6 CC1PLPSX, -fvolatile), like game_mode_audio.cpp. */
/* u8..s32 and the car records (humanCarGet, TM2HumanRecord). */
#include "semantic/cars.h"

/* g++ 2.x object: vtable pointer at +8 (see game_mode_audio.cpp). */
struct TM2ModeTarget {
    int f0, f4;
    virtual void f1();
    virtual void set_mode(int mode);
    virtual void f3(int value);
};

/* Session settings: struct TM2Settings (&sessionSettings), include/semantic/settings.h. */
#include "semantic/settings.h"
/* sessionRun's password line (the 4-bit car field holds 0..11): retail none for 12 and 13; request 36 also none for
   Dark Tooth (14, TM2_PORT_DT_PLAYER); request 42 item 3 none for a number past 11 (a mod: TM2_PORT_CAR_NUMBERS) */
#if defined(TM2_PORT_CAR_NUMBERS)
#define TM2_PASSWORD_CAR(s) ((u32)portCarNumberGet((s), 0, 0) < 12)
#elif defined(TM2_PORT_DT_PLAYER)
#define TM2_PASSWORD_CAR(s) ((u8)((s)->human_car[0] - 12) >= 3)
#else
#define TM2_PASSWORD_CAR(s) ((u8)((s)->human_car[0] - 12) >= 2)
#endif

extern "C" {
extern TM2ModeTarget *musicManagerObject;
extern s32 soundManagerObject;
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern s32 audioCdVolume, audioVolumeClampMax, audioVolumeClampMin;
extern char textFormatBuffer[];
extern char strGameOver[], strMinionIntro1[], strMinionIntro2[], strMinionIntro3[], strCongratulations[], strPrepareFor[];
extern char strDarkToothIntro1[], strDarkToothIntro2[], strDarkToothIntro3[], strDarkToothReveal1[], strDarkToothReveal2[], strDarkToothReveal3[];
extern char strMeetCalypso1[], strMeetCalypso2[];

#include "semantic/input.h"
extern void fontInit(void);
extern void SsInitHot(void);
extern int DrawSync(int);
extern int VSync(int);
#include "semantic/scene.h"
extern void passwordBuildString(void);
#include "semantic/hud.h"
extern int gameGetLevel(void);
extern void viewSetSplitMode(int);
extern void padSetLayout(u8 config, short int port);
extern void gameSetMode(s32);
extern void humanCarsSetCount(s32);
extern void aiSetDifficulty(s32);
extern void sessionLoadArenaAndCars(TM2Settings *);
extern void sessionBindCars(TM2Settings *);
extern void audioSetDarkToothMusic(void);
extern void roundStartMusic(s32);
extern void fxSpritesReset(void);
extern void taskMgrStopAll(s32);
#include "semantic/world.h"
#ifdef TM2_PORT_HUMANS
#include "semantic/port_humans.h"   /* HUMANS16_PLAN.md H2: portHumanSetLayouts */
#endif
extern int rtMainLoop(void);
extern void SsSetSerialVol(char s_num, short voll, short volr);
extern void rtClearScheduledEvents(void);
extern void screenShowMessageLines(TM2Settings *, s32, char *, s32, s32, s32, char *, s32, s32, s32, char *, s32, s32);
extern void sessionSetOpponentCar(struct TM2Settings *, s32, u8 car);
extern void sessionSetOpponents(struct TM2Settings *, u8 value);
extern char *levelDisplayName(struct TM2Settings *, s32);
extern void screenChallengeResult(TM2Settings *, u8 won, s16 wins, s16 losses);
extern void screenDeathmatchResult(TM2Settings *, s16 won, s16 wins, s16 losses);
extern void sessionTeardown(void);

/* The mode switch's jump table sits at its retail address. */
#pragma tm2_rdata .tm2_800CFCA8

/* Port variant (players.h): wherever the runner sets the reset-weapons flags of
 * players 1 and 2 as a pair, it sets those of players 3..TM2_MAX_HUMANS too
 * (retail's carResetWeapons storage holds four; past four humans it is port storage, HUMANS16_PLAN.md
 * H4). Expands to nothing in retail. */
#if TM2_MAX_HUMANS > 2
#define TM2_RESET_WEAPONS_REST(v) for (k = 2; k < TM2_MAX_HUMANS; k++) carSetResetWeapons(v, k);
#else
#define TM2_RESET_WEAPONS_REST(v)
#endif

/* @8012BE5C
 * Notes: the two boss_intro = 1 stores (merged by cross-jumping) give the
 * loop-hoisted constant 1 enough uses to outrank wins2 (s6/s7).
 * screenDisplayToDisplay(1, 0) runs unconditionally (retail); the first
 * volume clamp compares against quit (0 here) so cse/jump turn it into the
 * branchless x & (x >> 31); the per-result handling is nested switches. */
int __attribute__((section(".text_8012BE5C"))) sessionRun(TM2Settings *s)
{
    int quit = 0;
    int result = 0;
    short wins2 = 0;
    short wins1b = 0;
    short wins1 = 0;
    int vertical;
    int split;
    s32 value;
#if TM2_MAX_HUMANS > 2
    int k;
#endif

    while (padCheckAction(6, 0))
        padUpdate();
    fontInit();
    if (!s->dark_tooth_stage && (s->mode < 2 || s->mode == 2))
        SsInitHot();
    DrawSync(0);
    VSync(0);
    screenSetEnv();
    if (s->mode == 1 && s->level >= 2 && TM2_PASSWORD_CAR(s) && !s->dark_tooth_stage) {
        passwordBuildString();
        fontSetColor(5, 0xFF, 0xA1, 0);
        fontPrintCenteredXY(5, 0x9F, 0xDE, textFormatBuffer);
        fontResetColor(5);
    }
    screenDisplayToDisplay(1, 0);
    if (s->mode == 4 || s->mode == 2) {
        vertical = gameGetLevel() == 2 || gameGetLevel() == 3 || gameGetLevel() == 8;
        split = vertical ? 1 : 2;
    } else
        split = 0;
    viewSetSplitMode(split);
    padSetLayout(s->control_layout[0], 0);
    padSetLayout(s->control_layout[1], 1);
#ifdef TM2_PORT_HUMANS
    portHumanSetLayouts();  /* HUMANS16_PLAN.md H2: every human's bindings from portHumanLayout (the port's) */
#endif
    gameSetMode(s->mode);
    humanCarsSetCount(s->humans);
    aiSetDifficulty(s->difficulty);
    sessionLoadArenaAndCars(s);
    sessionBindCars(s);
    if (s->dark_tooth_stage && !s->dark_tooth_reveal)
        audioSetDarkToothMusic();

    while (!quit) {
        roundStartMusic(s->level);
        fxSpritesReset();
        taskMgrStopAll(soundManagerObject);
        taskMgrErrorStub(soundManagerObject);
        fontResetColor(6);
        fontResetColor(2);
        fontResetColor(0);
        result = rtMainLoop();
        musicManagerObject->f3(2);
        /* quit is always 0 here: min(E24, 0) then max(0, E28). */
        audioCdVolume = audioVolumeClampMax < quit ? audioVolumeClampMax : 0;
        value = quit < audioVolumeClampMin ? audioVolumeClampMin : quit;
        audioCdVolume = value;
        SsSetSerialVol(0, (s16)value, (s16)value);
        taskMgrStopAll(soundManagerObject);
        taskMgrErrorStub(soundManagerObject);
        rtClearScheduledEvents();
        fontSetColor(6, 0x73, 0x19, 8);
        fontSetColor(2, 0x73, 0x19, 8);
        fontSetColor(0, 0x73, 0x19, 8);
#ifdef TM2_PORT_HUMANS
        /* H5: with portHumanOptions bit 1 the port's rules take the result (no lives[0], no human 0 / 1 test, no
           2-player result screens); -1 = retail's handling below */
        value = portRoundResults(s, result);
        if (value >= 0) {
            quit = value;
            continue;
        }
#endif

        switch (s->mode) {
        case 0:
            carSetResetWeapons(1, 0);
            carSetResetWeapons(1, 1);
            TM2_RESET_WEAPONS_REST(1)
            if (result == 1)
                quit = 1;
            break;
        case 1:
        case 2:
            switch (result) {
            case 2:
                quit = 1;
                carSetResetWeapons(1, 0);
                carSetResetWeapons(1, 1);
                TM2_RESET_WEAPONS_REST(1)
                break;
            case 1:
                if (--s->lives[0] == 0) {
                    screenShowMessageLines(s, 6, strGameOver, 0x9F, 0x50, 0, 0, 0, 0, 0, 0, 0, 0);
                    quit = 1;
                    carSetResetWeapons(1, 0);
                    carSetResetWeapons(1, 1);
                    TM2_RESET_WEAPONS_REST(1)
                }
#if TM2_MAX_HUMANS > 2
                /* port variant, more than two humans: reset every dead human's
                   weapons (two or fewer: retail below) */
                if (s->humans > 2) {
                    for (k = 0; k < s->humans; k++)
                        if (humanCarGet(k)->stats.hp <= 0)
                            carSetResetWeapons(1, k);
                } else
#endif
                if (humanCarGet(0)->stats.hp <= 0)
                    carSetResetWeapons(1, 0);
                else
                    carSetResetWeapons(1, 1);
                break;
            case 3:
            default:
            if (s->level == 4) {
                if (s->human_car[0] == 13)
                    s->boss_intro = 1;
                else if (s->humans == 2 && s->human_car[1] == 13)
                    s->boss_intro = 1;
#if TM2_MAX_HUMANS > 2
                if (s->humans > 2)    /* port variant: any human after the first drives Minion */
                    for (k = 1; k < s->humans; k++)
                        if (TM2_SETTINGS_HUMAN_CAR(s, k) == 13)
                            s->boss_intro = 1;
#endif
                if (!s->boss_intro) {
                    s->boss_intro = 1;
                    screenShowMessageLines(s, 2, strMinionIntro1, 0x9F, 100, 2, strMinionIntro2, 0x9F, 0x78, 2, strMinionIntro3, 0x9F, 0x8C);
                    padWaitConfirmOrTimeout(120, 120);
                    sessionSetOpponentCar(&sessionSettings, 0, 13);
                    sessionSetOpponents(&sessionSettings, 1);
                    carSetResetWeapons(0, 0);
                    carSetResetWeapons(0, 1);
                    TM2_RESET_WEAPONS_REST(0)
                    sessionLoadArenaAndCars(s);
                    sessionBindCars(s);
                } else {
                    if (s->difficulty) {
                        char *next;
                        carSetResetWeapons(0, 0);
                        carSetResetWeapons(0, 1);
                        TM2_RESET_WEAPONS_REST(0)
                        next = levelDisplayName(&sessionSettings, s->level + 1);
                        screenShowMessageLines(s, 6, strCongratulations, 0x9F, 0x50, 2, strPrepareFor, 0x9F, 0x78, 2, next, 0x9F, 0x8C);
                        padWaitConfirmOrTimeout(300, 600);
                    } else {
                        carSetResetWeapons(1, 0);
                        carSetResetWeapons(1, 1);
                        TM2_RESET_WEAPONS_REST(1)
                    }
                    quit = 1;
                }
            } else if (s->level == 8) {
                if (!s->dark_tooth_stage) {
                    s->dark_tooth_stage = 1;
                    screenShowMessageLines(s, 2, strDarkToothIntro1, 0x9F, 100, 2, strDarkToothIntro2, 0x9F, 0x78, 2, strDarkToothIntro3, 0x9F, 0x8C);
                    padWaitConfirmOrTimeout(120, 120);
                    sessionSetOpponentCar(&sessionSettings, 0, 14);
                    sessionSetOpponents(&sessionSettings, 1);
                    carSetResetWeapons(0, 0);
                    carSetResetWeapons(0, 1);
                    TM2_RESET_WEAPONS_REST(0)
                    sessionLoadArenaAndCars(s);
                    sessionBindCars(s);
                    audioSetDarkToothMusic();
                } else if (!s->dark_tooth_reveal) {
                    s->dark_tooth_reveal = 1;
                    screenShowMessageLines(s, 2, strDarkToothReveal1, 0x9F, 100, 2, strDarkToothReveal2, 0x9F, 0x78, 2, strDarkToothReveal3, 0x9F, 0x8C);
                    padWaitConfirmOrTimeout(300, 600);
                } else {
                    screenShowMessageLines(s, 6, strCongratulations, 0x9F, 0x50, 2, strMeetCalypso1, 0x9F, 0x78, 2, strMeetCalypso2, 0x9F, 0x8C);
                    padWaitConfirmOrTimeout(300, 600);
                    quit = 1;
                    carSetResetWeapons(1, 0);
                    carSetResetWeapons(1, 1);
                    TM2_RESET_WEAPONS_REST(1)
                }
            } else {
                char *next;
                next = levelDisplayName(&sessionSettings, s->level + 1);
                quit = 1;
                screenShowMessageLines(s, 6, strCongratulations, 0x9F, 0x50, 2, strPrepareFor, 0x9F, 0x78, 2, next, 0x9F, 0x8C);
                padWaitConfirmOrTimeout(300, 600);
                carSetResetWeapons(0, 0);
                carSetResetWeapons(0, 1);
                TM2_RESET_WEAPONS_REST(0)
            }
                break;
            }
            break;
        case 3:
            sessionBindCars(s);
            switch (result) {
            case 2:
                quit = 1;
                carSetResetWeapons(1, 0);
                carSetResetWeapons(1, 1);
                TM2_RESET_WEAPONS_REST(1)
                break;
            case 1:
                screenChallengeResult(s, 0, wins2, ++wins1);
                padWaitConfirmOrTimeout(120, 120);
                carSetResetWeapons(1, 0);
                break;
            case 3:
                screenChallengeResult(s, 1, ++wins2, wins1);
                padWaitConfirmOrTimeout(120, 120);
                carSetResetWeapons(1, 0);
                break;
            }
            break;
        case 4:
        default:
            sessionBindCars(s);
            switch (result) {
            case 2:
                quit = 1;
                carSetResetWeapons(1, 0);
                carSetResetWeapons(1, 1);
                TM2_RESET_WEAPONS_REST(1)
                break;
            case 4:
                screenDeathmatchResult(s, 2, wins2, ++wins1b);
                padWaitConfirmOrTimeout(120, 120);
                carSetResetWeapons(1, 0);
                carSetResetWeapons(1, 1);
                TM2_RESET_WEAPONS_REST(1)
                break;
            case 3:
                screenDeathmatchResult(s, 1, ++wins2, wins1b);
                padWaitConfirmOrTimeout(120, 120);
                carSetResetWeapons(1, 0);
                carSetResetWeapons(1, 1);
                TM2_RESET_WEAPONS_REST(1)
                break;
            }
            break;
        }
    }
    fontResetColor(6);
    fontResetColor(2);
    fontResetColor(0);
    s->boss_intro = s->dark_tooth_stage = s->dark_tooth_reveal = 0;
    sessionTeardown();
    return result;
}
}
