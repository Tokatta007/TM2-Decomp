/* Single-player mode menu (8012E908; IDA calls it "screenSinglePlayerMenu",
 * but it loads SHELLDB1\SINGLE: the car select is screenCarSelect).
 *
 * Like every shell screen it builds a scene on the shared scene object
 * shellScene from stack objects, adds its transition rows
 * (sceneAddAction(scene, event, source, mode, action)), runs it
 * (sceneRun) and returns the next screen handler.
 *
 * Three menu items: tournament (-> car select screenCarSelect), challenge
 * (-> level select screenLevelSelect) and password (-> keypad screenPickOpponents,
 * then checked here). Events used: 0 start, 2 action done, 3/14/15/16
 * (back and other buttons), 7/8 up/down, 10 select. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/scene.h"
#include "semantic/settings.h"

extern struct TM2ShellScene shellScene;             /* the shell scene */

#pragma tm2_absolute soundManagerObject
extern s32 soundManagerObject;
extern char strShellSingleTim[];           /* "SHELLDB1\SINGLE.TIM" */
extern char strShellSingleScene[];           /* "SHELLDB1\SINGLE" */
extern char strDifficultySetMedium[];           /* "difficulty set to medium for this level" */
extern char strInvalidPassword[];           /* "invalid password" */

/* title screen */
extern void *screenCarSelect(void);    /* car select */
extern void *screenLevelSelect(void);    /* level select */
extern void *screenPickOpponents(void);    /* password keypad */
void *screenSinglePlayerMenu(void);

extern void sceneInit(struct TM2ShellScene *scene, void *back, void *cancel, void *idle, s32);
extern u16 sceneSetCamera(struct TM2ShellScene *scene, s32 x, s32 y, s32 z, u16, u16, u16);
extern void *screenImageRecordInit(void *record, const char *name, s32 buffer);
extern void *sub_80137F60(void *item, u16 id, u16 count, void *(*next)(void));
extern void sceneObjSetPosition(void *object, s32 x, s32 y, s32 z);
extern void sceneObjSetUseCamera(void *object, u16 v);
extern void sub_80123E5C(void *object, u16 v);
extern void sceneObjSetScale(void *object, s32 v);
extern void *menuPasswordDigitsCreate(void *object, u16 first, u16 second);
extern void *sub_80123E98(void *object, u16 first, u16 second);
extern void *menuSelectNextInit(void *object, void *owner);   /* action: select the next item */
extern void *menuSelectPrevInit(void *object, void *owner);   /* action: select the previous item */
extern void *menuSelectIndexInit(void *object, void *owner, s32 value);   /* action: select item value */
extern void *sub_80123828(void *object, void *target, s32, s32);
extern void *sub_80123870(void *object, void *target, s32);
extern void *sub_80123A20(void *object, void *target, s32, s32, s32, s32);
extern void *sub_80123B64(void *object, void *target, s32, s32, s32);
extern void *menuPlaySoundInit(void *object, s32 actor, s32, s32, s32);
extern void *sub_8012D37C(void *object, void *scene, void *, void *, void *);
extern void sceneSetBackground(struct TM2ShellScene *scene, void *record);
extern s32 sceneAddItem(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddDecor(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddAction(struct TM2ShellScene *scene, s32 event, void *source, s32 mode, void *action);
extern s32 sceneLoadTmsDmd(struct TM2ShellScene *scene, const char *name);
extern s32 sceneRun(struct TM2ShellScene *scene);
extern s32 sceneGetExitCode(struct TM2ShellScene *scene);
extern void *sceneGetSelectedItem2(struct TM2ShellScene *scene);
extern void settingsClearHumanCars(struct TM2Settings *settings);
extern void settingsClearOpponentCars(struct TM2Settings *settings);
extern void settingsSetMode(struct TM2Settings *settings, s32 v);
extern void settingsSetLevel(struct TM2Settings *settings, s32 v);
extern s32 settingsGetLevel(struct TM2Settings *settings);
extern s32 tournamentOpponentCount(struct TM2Settings *settings, s32 level);
extern s32 settingsGetHumans(struct TM2Settings *settings);
extern void settingsSetLives(struct TM2Settings *settings, s32 index, s32 value);
extern void settingsSetDarkToothStage(struct TM2Settings *settings, u8 v);
extern void settingsSetDifficulty(struct TM2Settings *settings, u8 v);
extern u32 menuKeypadGetValue(void *keypad);
extern s32 passwordDecode(s32 code, u16 *level, u16 *car, u8 *flag, u8 *extra);
extern int DrawSync(int);
extern int VSync(int);
#include "semantic/hud.h"
#include "semantic/input.h"


/* @8012E908 */
void *__attribute__((section(".text_8012E908"))) screenSinglePlayerMenu(void)
{
    u8 record[48];
    u8 tournament[56], challenge[56], password[56];
    u8 keypad[304];
    u8 title1[56], title2[56];
    u8 sel_next[16], sel_prev[16], sel_first[24], sel_last[24];
    u8 wait[24], set0[24], set1[24], set2[24];
    u8 turn1[32], turn2[32], turn3[32];
    u8 bob1_up[32], bob1_down[32], bob2_up[32], bob2_down[32], bob3_up[32], bob3_down[32];
    u8 sound_move[32], sound_back[32];
    u8 camera[32];
    u16 level, car;
    u8 flag, extra;
    struct TM2ShellScene *scene = &shellScene;
    s32 result, value, value2, stop;   /* separate value locals: retail gives them different registers */

    sceneInit(scene, screenTitle, screenTitle, shellAttractCycle, 0);
    sceneSetCamera(scene, 0, -1200, 0, 0, 0, 0);
    screenImageRecordInit(record, strShellSingleTim, 0x80048270);

    sub_80137F60(tournament, 0x2329, 1, screenCarSelect);
    sceneObjSetPosition(tournament, -1784, 0, 360);
    sceneObjSetUseCamera(tournament, 0);
    sub_80123E5C(tournament, 0);
    sceneObjSetScale(tournament, 1);
    sub_80137F60(challenge, 0x232A, 1, screenLevelSelect);
    sceneObjSetPosition(challenge, -584, 0, 176);
    sceneObjSetUseCamera(challenge, 0);
    sub_80123E5C(challenge, 0);
    sceneObjSetScale(challenge, 1);
    sub_80137F60(password, 0x232B, 1, screenPickOpponents);
    sceneObjSetPosition(password, -1784, 0, -8);
    sceneObjSetUseCamera(password, 0);
    sub_80123E5C(password, 0);
    sceneObjSetScale(password, 1);

    menuPasswordDigitsCreate(keypad, 0x233C, 1);
    sceneObjSetPosition(keypad, -160, 0, -8);
    sceneObjSetUseCamera(keypad, 0);
    sub_80123E5C(keypad, 0);
    sceneObjSetScale(keypad, 1);
    sub_80123E98(title1, 0x2334, 0);
    sceneObjSetPosition(title1, -160, 0, -8);
    sceneObjSetUseCamera(title1, 0);
    sub_80123E5C(title1, 10);
    sceneObjSetScale(title1, 1);
    sub_80123E98(title2, 0x2335, 1);
    sceneObjSetPosition(title2, -160, 0, -8);
    sceneObjSetUseCamera(title2, 0);
    sub_80123E5C(title2, 20);
    sceneObjSetScale(title2, 1);

    menuSelectNextInit(sel_next, scene);
    menuSelectPrevInit(sel_prev, scene);
    menuSelectIndexInit(sel_first, scene, 0);
    menuSelectIndexInit(sel_last, scene, 2);
    sub_80123828(wait, scene, 0, 2000);
    sub_80123870(set0, scene, 0);
    sub_80123870(set1, scene, 1);
    sub_80123870(set2, scene, 2);
    sub_80123A20(turn1, tournament, 40, 0, 0, 15);
    sub_80123A20(turn2, challenge, -40, 0, 0, 15);
    sub_80123A20(turn3, password, 40, 0, 0, 15);
    sub_80123B64(bob1_up, tournament, 0, 1, 0);
    sub_80123B64(bob1_down, tournament, 0, -1, 0);
    sub_80123B64(bob2_up, challenge, 0, 1, 0);
    sub_80123B64(bob2_down, challenge, 0, -1, 0);
    sub_80123B64(bob3_up, password, 0, 1, 0);
    sub_80123B64(bob3_down, password, 0, -1, 0);
    menuPlaySoundInit(sound_move, soundManagerObject, 0x1C, 0, 0);
    menuPlaySoundInit(sound_back, soundManagerObject, 0x1D, 0, 0);
    sub_8012D37C(camera, scene, keypad, title1, sound_move);
    sceneSetBackground(scene, record);

    sceneAddItem(scene, tournament);
    sceneAddItem(scene, challenge);
    sceneAddItem(scene, password);
    sceneAddDecor(scene, keypad);
    sceneAddDecor(scene, title1);
    sceneAddDecor(scene, title2);

    sceneAddAction(scene, 7, 0, 2, sound_move);
    sceneAddAction(scene, 8, 0, 2, sound_move);
    sceneAddAction(scene, 3, 0, 2, sound_back);
    sceneAddAction(scene, 3, 0, 1, wait);
    sceneAddAction(scene, 3, 0, 0, set0);
    sceneAddAction(scene, 14, 0, 2, sound_back);
    sceneAddAction(scene, 14, 0, 1, wait);
    sceneAddAction(scene, 14, 0, 0, set0);
    sceneAddAction(scene, 15, 0, 2, sound_back);
    sceneAddAction(scene, 15, 0, 1, wait);
    sceneAddAction(scene, 15, 0, 0, set1);
    sceneAddAction(scene, 16, 0, 2, sound_back);
    sceneAddAction(scene, 16, 0, 1, wait);
    sceneAddAction(scene, 16, 0, 0, set2);
    sceneAddAction(scene, 0, 0, 0, turn1);
    sceneAddAction(scene, 0, 0, 0, turn2);
    sceneAddAction(scene, 0, 0, 0, turn3);
    sceneAddAction(scene, 2, turn1, 2, sound_move);
    sceneAddAction(scene, 2, turn1, 2, bob1_up);
    sceneAddAction(scene, 2, turn1, 0, wait);
    sceneAddAction(scene, 7, tournament, 2, sel_last);
    sceneAddAction(scene, 7, tournament, 2, bob1_down);
    sceneAddAction(scene, 7, tournament, 2, bob3_up);
    sceneAddAction(scene, 8, tournament, 2, sel_next);
    sceneAddAction(scene, 8, tournament, 2, bob1_down);
    sceneAddAction(scene, 8, tournament, 2, bob2_up);
    sceneAddAction(scene, 7, challenge, 2, sel_prev);
    sceneAddAction(scene, 7, challenge, 2, bob2_down);
    sceneAddAction(scene, 7, challenge, 2, bob1_up);
    sceneAddAction(scene, 8, challenge, 2, sel_next);
    sceneAddAction(scene, 8, challenge, 2, bob2_down);
    sceneAddAction(scene, 8, challenge, 2, bob3_up);
    sceneAddAction(scene, 7, password, 2, sel_prev);
    sceneAddAction(scene, 7, password, 2, bob3_down);
    sceneAddAction(scene, 7, password, 2, bob2_up);
    sceneAddAction(scene, 8, password, 2, sel_first);
    sceneAddAction(scene, 8, password, 2, bob3_down);
    sceneAddAction(scene, 8, password, 2, bob1_up);
    sceneAddAction(scene, 10, password, 2, sound_move);
    sceneAddAction(scene, 10, password, 2, bob3_down);
    sceneAddAction(scene, 10, password, 1, wait);
    sceneAddAction(scene, 10, password, 0, camera);
    sceneAddAction(scene, 2, camera, 2, sound_move);
    sceneAddAction(scene, 2, camera, 2, bob3_up);
    sceneAddAction(scene, 2, camera, 0, wait);
    sceneAddAction(scene, 2, wait, 2, set1);

    sceneLoadTmsDmd(scene, strShellSingleScene);
    screenImageRecordLoad(record);
    result = sceneRun(scene);
    if (sceneGetExitCode(scene))
        return (void *)result;

    settingsSetHumans(&sessionSettings, 1);
    settingsClearHumanCars(&sessionSettings);
    settingsSetCarSelectPlayer(&sessionSettings, 0);
    settingsSetOpponentsPicked(&sessionSettings, 0);
    settingsClearOpponentCars(&sessionSettings);
    if (sceneGetSelectedItem2(scene) == tournament) {
        settingsSetMode(&sessionSettings, 1);
        settingsSetLevel(&sessionSettings, 1);
        value = tournamentOpponentCount(&sessionSettings, settingsGetLevel(&sessionSettings));
        settingsSetOpponents(&sessionSettings, value - settingsGetHumans(&sessionSettings));
        settingsSetLives(&sessionSettings, 0, 3);
    } else if (sceneGetSelectedItem2(scene) == challenge) {
        settingsSetMode(&sessionSettings, 3);
        settingsSetLives(&sessionSettings, 0, 1);
    } else if (sceneGetSelectedItem2(scene) == password) {
        if (passwordDecode((u16)passwordRebase(menuKeypadGetValue(keypad), 5, 10), &level, &car, &flag, &extra)) {
            if (level == 8)
                settingsSetDarkToothStage(&sessionSettings, flag);
            settingsSetLevel(&sessionSettings, level);
            settingsSetHumanCar(&sessionSettings, 0, car);
            if (settingsGetLevel(&sessionSettings) == 8 && flag == 1)
                settingsSetOpponents(&sessionSettings, 1);
            else {
                value2 = tournamentOpponentCount(&sessionSettings, settingsGetLevel(&sessionSettings));
                settingsSetOpponents(&sessionSettings, value2 - settingsGetHumans(&sessionSettings));
            }
            stop = 0;
            settingsSetLives(&sessionSettings, 0, 3);
            if (settingsGetLevel(&sessionSettings) >= 5)
                stop = settingsGetDifficulty(&sessionSettings) == 0;
            if (stop) {
                screenSetEnv();
                screenDisplayToDisplay(0, 1);
                fontPrintCenteredXY(2, 159, 149, strDifficultySetMedium);
                padWaitConfirmOrTimeout(30, 120);
                settingsSetDifficulty(&sessionSettings, 1);
            }
            settingsSetMode(&sessionSettings, 1);
        } else {
            DrawSync(0);
            VSync(0);
            screenSetEnv();
            screenDisplayToDisplay(0, 1);
            fontPrintCenteredXY(2, 159, 149, strInvalidPassword);
            padWaitConfirmOrTimeout(30, 120);
            settingsSetLevel(&sessionSettings, 1);
            return (void *)screenSinglePlayerMenu;
        }
    }
    return (void *)result;
}
