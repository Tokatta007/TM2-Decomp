/* Game code (shell screens). Names stay address-based.
 *
 * Built like every shell screen (see screen_single.c): stack objects are
 * registered with the shared scene shellScene, transition rows
 * (sceneAddAction(scene, event, source, mode, action)) are added, the scene
 * runs (sceneRun) and its result is the next screen handler. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/scene.h"
#include "semantic/settings.h"

typedef void (*TM2Method)(u8 *, ...);
typedef struct { s16 delta; s16 index; TM2Method fn; } TM2VEntry;  /* vtable slot */

extern struct TM2ShellScene shellScene;             /* the shell scene */

extern u8 *soundManagerObject;              /* sound object */
#pragma tm2_absolute soundManagerObject
extern s32 audioSfxVolume, audioVolumeClampMax, audioVolumeClampMin;
#pragma tm2_absolute audioSfxVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern u8 controlsLayoutPlayer;               /* player whose keypad is shown */
extern s16 levelGlobeAngles[][2];         /* per level: globe angle, globe tilt */
extern char strShellMultiTim[];           /* multi-player menu TIM */
extern char strShellMultiScene[];           /* multi-player menu scene */
extern char strShellWorldTim[];           /* "SHELLDB1\WORLD.TIM" */
extern char strShellSphere2Scene[];           /* "SHELLDB1\SPHERE2" */
extern char strShellKeypad1Tim[];           /* "SHELLDB2\KEYPAD1.TIM" */
extern char strShellKeypad2Tim[];           /* "SHELLDB2\KEYPAD2.TIM" */
extern char strShellKeypad3Tim[];           /* "SHELLDB2\KEYPAD3.TIM" */
extern char strShellKeypad4Tim[];           /* "SHELLDB2\KEYPAD4.TIM" */
extern char strShellArrowsScene[];           /* keypad (player select) scene */

extern void *screenTitle(void);    /* title screen */
extern void *shellAttractCycle(void *value);
extern void *screenLevelSelect(void);    /* level select */
extern void *screenCarSelect(void);    /* car select */
extern void *screenOptions(void);
extern void *screenScrollText(void);

extern void sceneInit(struct TM2ShellScene *scene, void *back, void *cancel, void *idle, s32);
extern u16 sceneSetCamera(struct TM2ShellScene *scene, s32 x, s32 y, s32 z, u16, u16, u16);
extern void *screenImageRecordInit(void *record, const char *name, s32 buffer);
extern void screenImageRecordLoad(void *record);
extern void *sub_80137F60(void *item, u16 id, u16 count, void *(*next)(void));
extern void sceneObjSetPosition(void *object, s32 x, s32 y, s32 z);
extern void sceneObjSetRotation(void *object, s32 x, s32 y, s32 z);
extern void sceneObjSetUseCamera(void *object, u16 v);
extern void sub_80123E5C(void *object, u16 v);
extern void sceneObjSetScale(void *object, s32 v);
extern void *sub_80123E98(void *object, u16 first, u16 second);
extern void *menuTextInit(void *object, s32, void *value, s8 mode, u16 first, u16 second, u16 third);
extern void menuTextDestroy(void *object, s32);
extern void *globeMarkerInit(void *object, u16 id);
extern void *menuLoadPictureInit(void *object, void *owner, const char *name);
extern void *menuSelectNextInit(void *object, void *owner);   /* action: select the next item */
extern void *menuSelectPrevInit(void *object, void *owner);   /* action: select the previous item */
extern void *sub_80123828(void *object, void *target, s32, s32);
extern void *sub_801237D4(void *object, void *target, s32, s32);
extern void *sub_80123870(void *object, void *target, s32);
extern void *sub_80123A20(void *object, void *target, s32, s32, s32, s32);   /* TM2TurnBy */
extern void *sub_801239CC(void *object, void *target, s32, s32, s32, s32);   /* TM2MoveBy */
extern void *sub_80123B64(void *object, void *target, s32, s32, void *);     /* TM2FourWay */
extern void *menuPlaySoundInit(void *object, u8 *actor, s32, s32, s32);
extern void sceneSetBackground(struct TM2ShellScene *scene, void *record);
extern s32 sceneAddItem(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddDecor(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddAuxObj(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddAction(struct TM2ShellScene *scene, s32 event, void *source, s32 mode, void *action);
extern s32 sceneLoadTmsDmd(struct TM2ShellScene *scene, const char *name);
extern s32 sceneRun(struct TM2ShellScene *scene);
extern s32 sceneGetExitCode(struct TM2ShellScene *scene);
extern void *sceneGetSelectedItem2(struct TM2ShellScene *scene);
extern void sceneSetSelection(struct TM2ShellScene *scene, s32);
extern void sceneObjSetChoiceState(void *object, s32 value);
extern s32 settingsGetLevel(struct TM2Settings *settings);
extern u8 settingsGetWorldMap(struct TM2Settings *settings);
extern s32 settingsGetMode(struct TM2Settings *settings);
extern void *levelDisplayName(struct TM2Settings *settings, s32 level);
extern u8 settingsGetControlLayout(struct TM2Settings *settings, u8 player);
extern void settingsSetControlLayout(struct TM2Settings *settings, u8 player, u8 v);
extern void settingsSetHumans(struct TM2Settings *settings, u8 v);
extern void settingsClearHumanCars(struct TM2Settings *settings);
extern void settingsSetCarSelectPlayer(struct TM2Settings *settings, s32 v);
extern void settingsSetOpponentsPicked(struct TM2Settings *settings, s32 v);
extern void settingsClearOpponentCars(struct TM2Settings *settings);
extern void settingsSetMode(struct TM2Settings *settings, s32 v);
extern void settingsSetLevel(struct TM2Settings *settings, s32 v);
extern s32 tournamentOpponentCount(struct TM2Settings *settings, s32 level);
extern s32 settingsGetHumans(struct TM2Settings *settings);
extern void settingsSetOpponents(struct TM2Settings *settings, s32 v);
extern void settingsSetLives(struct TM2Settings *settings, s32 index, s32 value);
extern s32 audioGetMusicVolume(void);
extern void taskMgrRunFrame(u8 *);
extern int abs(int);

/* Clamp the effects volume (audioSfxVolume) to [audioVolumeClampMin, audioVolumeClampMax]. */
static __inline__ void SetSfxVolume(s32 v)
{
    audioSfxVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioSfxVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
}

/* Store min(v, audioVolumeClampMax) to the effects volume and return the
 * max(v, audioVolumeClampMin) value; the caller stores it (retail schedules that
 * second store after the next vtable load). */
static __inline__ s32 ClampSfx(s32 v)
{
    audioSfxVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    return v < audioVolumeClampMin ? audioVolumeClampMin : v;
}

/* @8012F5F8: multi-player menu (two items). Deathmatch (9001 -> level select
 * screenLevelSelect) sets mode 4 with 0 opponents; the other item (9002 -> car
 * select screenCarSelect) sets mode 2 with the level's opponent count. */
void *__attribute__((section(".text_8012F5F8"))) screenMultiPlayer(void)
{
    u8 record[48];
    u8 item_a[56], item_b[56];
    u8 sel_next[16], sel_prev[16];
    u8 wait[24], set0[24], set1[24], set2[24];
    u8 turn_a[32], turn_b[32];
    u8 up_a[32], down_a[32], up_b[32], down_b[32];
    u8 snd_move[32], snd_ok[32];
    s32 result, value;

    sceneInit(&shellScene, screenTitle, screenTitle, shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -1200, 0, 0, 0, 0);
    screenImageRecordInit(record, strShellMultiTim, 0x80048270);
    sub_80137F60(item_a, 9001, 1, screenLevelSelect);
    sceneObjSetPosition(item_a, -1784, 0, 360);
    sceneObjSetUseCamera(item_a, 0);
    sub_80123E5C(item_a, 0);
    sceneObjSetScale(item_a, 1);
    sub_80137F60(item_b, 9002, 1, screenCarSelect);
    sceneObjSetPosition(item_b, -584, 0, 176);
    sceneObjSetUseCamera(item_b, 0);
    sub_80123E5C(item_b, 0);
    sceneObjSetScale(item_b, 1);
    menuSelectNextInit(sel_next, &shellScene);
    menuSelectPrevInit(sel_prev, &shellScene);
    sub_80123828(wait, &shellScene, 0, 2000);
    sub_80123870(set0, &shellScene, 0);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    sub_80123A20(turn_a, item_a, 40, 0, 0, 15);
    sub_80123A20(turn_b, item_b, -40, 0, 0, 15);
    sub_80123B64(up_a, item_a, 0, 1, 0);
    sub_80123B64(down_a, item_a, 0, -1, 0);
    sub_80123B64(up_b, item_b, 0, 1, 0);
    sub_80123B64(down_b, item_b, 0, -1, 0);
    menuPlaySoundInit(snd_move, soundManagerObject, 28, 0, 0);
    menuPlaySoundInit(snd_ok, soundManagerObject, 29, 0, 0);
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, item_a);
    sceneAddItem(&shellScene, item_b);
    sceneAddAction(&shellScene, 7, 0, 2, snd_move);
    sceneAddAction(&shellScene, 7, 0, 2, sel_prev);
    sceneAddAction(&shellScene, 8, 0, 2, snd_move);
    sceneAddAction(&shellScene, 8, 0, 2, sel_next);
    sceneAddAction(&shellScene, 3, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 3, 0, 1, wait);
    sceneAddAction(&shellScene, 3, 0, 0, set0);
    sceneAddAction(&shellScene, 14, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 14, 0, 1, wait);
    sceneAddAction(&shellScene, 14, 0, 0, set0);
    sceneAddAction(&shellScene, 15, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 15, 0, 1, wait);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 16, 0, 1, wait);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 0, 0, 0, turn_a);
    sceneAddAction(&shellScene, 0, 0, 0, turn_b);
    sceneAddAction(&shellScene, 2, turn_b, 2, snd_move);
    sceneAddAction(&shellScene, 2, turn_b, 2, up_a);
    sceneAddAction(&shellScene, 2, turn_b, 0, wait);
    sceneAddAction(&shellScene, 7, item_a, 2, down_a);
    sceneAddAction(&shellScene, 7, item_a, 2, up_b);
    sceneAddAction(&shellScene, 8, item_a, 2, down_a);
    sceneAddAction(&shellScene, 8, item_a, 2, up_b);
    sceneAddAction(&shellScene, 7, item_b, 2, down_b);
    sceneAddAction(&shellScene, 7, item_b, 2, up_a);
    sceneAddAction(&shellScene, 8, item_b, 2, down_b);
    sceneAddAction(&shellScene, 8, item_b, 2, up_a);
    sceneAddAction(&shellScene, 2, wait, 2, set1);
    sceneLoadTmsDmd(&shellScene, strShellMultiScene);
    screenImageRecordLoad(record);
    result = sceneRun(&shellScene);
    if (sceneGetExitCode(&shellScene))
        return (void *)result;

    settingsSetHumans(&sessionSettings, 2);
    settingsSetCarSelectPlayer(&sessionSettings, 0);
    settingsClearHumanCars(&sessionSettings);
    settingsClearOpponentCars(&sessionSettings);
    if (sceneGetSelectedItem2(&shellScene) == item_a) {
        settingsSetMode(&sessionSettings, 4);
        settingsSetOpponents(&sessionSettings, 0);
        settingsSetLives(&sessionSettings, 0, 1);
        settingsSetLives(&sessionSettings, 1, 1);
    } else {
        settingsSetMode(&sessionSettings, 2);
        settingsSetLevel(&sessionSettings, 1);
        value = tournamentOpponentCount(&sessionSettings, settingsGetLevel(&sessionSettings));
        settingsSetOpponents(&sessionSettings, value - settingsGetHumans(&sessionSettings));
        settingsSetOpponentsPicked(&sessionSettings, 0);
        settingsSetLives(&sessionSettings, 0, 6);
        settingsSetLives(&sessionSettings, 1, 6);
    }
    return (void *)result;
}

/* @801334E8: world globe screen (SHELLDB1\WORLD.TIM / SPHERE2) shown between
 * levels: the globe starts at the previous level's angle and turns to the
 * current one, the level counter shows the level name, the passed level
 * markers are lit (single/multi-player modes), and the "level" sound plays.
 * Skipped (straight to screenScrollText) when the flag is off or level >= 9. */
void *__attribute__((section(".text_801334E8"))) screenWorldGlobe(void)
{
    u8 *marks[8];
    u8 record[48];
    u8 globe[56], ring[56], tilt[56];
    u8 counter[72];
    u8 m0[8], m1[8], m2[8], m3[8], m4[8], m5[8], m6[8], m7[8];
    u8 fade[24], set0[24];
    u8 spin[32], snd_ok[32];
    s32 prev, cur, skip, flag, i, delta, saved, result;
    u8 **p;

    prev = settingsGetLevel(&sessionSettings) - 1;
    cur = settingsGetLevel(&sessionSettings);
    skip = 0;
    if (!settingsGetWorldMap(&sessionSettings) || settingsGetLevel(&sessionSettings) >= 9)
        skip = 1;
    if (skip)
        return (void *)screenScrollText;

    sceneInit(&shellScene, screenTitle, screenTitle, shellAttractCycle, 0);
    screenImageRecordInit(record, strShellWorldTim, 0x80048270);
    sub_80137F60(globe, 9000, 1, screenScrollText);
    sceneObjSetPosition(globe, 0, 32000, 0);
    sceneObjSetRotation(globe, (levelGlobeAngles[prev][0] << 12) / 360, 0, 0);
    sub_80123E98(ring, 9001, 1);
    sceneObjSetPosition(ring, 0, 32000, 0);
    sub_80123E98(tilt, 9002, 1);
    sceneObjSetPosition(tilt, 0, 0, (s16)(levelGlobeAngles[cur][1] / 7) << 3);
    sceneObjSetUseCamera(tilt, 0);
    sub_80123E5C(tilt, 0);
    sceneObjSetScale(tilt, 1);
    menuTextInit(counter, 2, levelDisplayName(&sessionSettings, settingsGetLevel(&sessionSettings)), 1, 160, 225, 1);
    globeMarkerInit(m0, 9003);
    globeMarkerInit(m1, 9006);
    globeMarkerInit(m2, 9005);
    globeMarkerInit(m3, 9004);
    globeMarkerInit(m4, 9010);
    globeMarkerInit(m5, 9007);
    globeMarkerInit(m6, 9009);
    globeMarkerInit(m7, 9008);
    marks[0] = m0;
    marks[1] = m1;
    marks[2] = m2;
    marks[3] = m3;
    marks[4] = m4;
    marks[5] = m5;
    marks[6] = m6;
    marks[7] = m7;
    sub_801237D4(fade, &shellScene,
                 settingsGetMode(&sessionSettings) == 4 || settingsGetMode(&sessionSettings) == 2 ? 2 : 0, -1);
    sub_80123870(set0, &shellScene, 0);
    delta = levelGlobeAngles[cur][0] - levelGlobeAngles[prev][0];
    sub_801239CC(spin, globe, delta > 0 ? 11 : -11, 0, 0, abs(delta));
    menuPlaySoundInit(snd_ok, soundManagerObject, 29, 0, 0);
    flag = 0;
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, globe);
    sceneAddDecor(&shellScene, ring);
    sceneAddDecor(&shellScene, tilt);
    sceneAddDecor(&shellScene, counter);
    sceneAddAuxObj(&shellScene, m0);
    sceneAddAuxObj(&shellScene, m1);
    sceneAddAuxObj(&shellScene, m2);
    sceneAddAuxObj(&shellScene, m3);
    sceneAddAuxObj(&shellScene, m4);
    sceneAddAuxObj(&shellScene, m5);
    sceneAddAuxObj(&shellScene, m6);
    sceneAddAuxObj(&shellScene, m7);
    sceneAddAction(&shellScene, 0, 0, 0, fade);
    sceneAddAction(&shellScene, 0, 0, 0, spin);
    sceneAddAction(&shellScene, 3, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 3, 0, 1, fade);
    sceneAddAction(&shellScene, 3, 0, 0, set0);
    sceneAddAction(&shellScene, 2, spin, 0, set0);
    sceneLoadTmsDmd(&shellScene, strShellSphere2Scene);
    screenImageRecordLoad(record);
    if (settingsGetMode(&sessionSettings) == 1 || settingsGetMode(&sessionSettings) == 2)
        flag = 1;
    i = 0;
    if (flag) {
        p = marks;
        while (i < settingsGetLevel(&sessionSettings) - 1) {
            sceneObjSetChoiceState(*p, 1);
            p++;
            i++;
        }
    }
    saved = audioSfxVolume;
    {
        TM2VEntry *e;
        s32 t;

        /* Same as SetSfxVolume(audioGetMusicVolume()), split for scheduling. */
        t = ClampSfx(audioGetMusicVolume());
        e = *(TM2VEntry **)(soundManagerObject + 200) + 1;
        audioSfxVolume = t;
        e->fn(soundManagerObject + e->delta, 27, 0, 0, 99, -1, 0);
        taskMgrRunFrame(soundManagerObject);
        e = *(TM2VEntry **)(soundManagerObject + 200) + 2;
        e->fn(soundManagerObject + e->delta);
    }
    SetSfxVolume(saved);
    result = sceneRun(&shellScene);
    menuTextDestroy(counter, 2);
    return (void *)result;
}

/* @80135088: controller keypad layout screen (SHELLDB2\KEYPAD1..4.TIM, by
 * player controlsLayoutPlayer's current layout; returns to screenOptions): left/right cycle the four keypad
 * pictures; the picked item is stored back into the settings for that player. */
void *__attribute__((section(".text_80135088"))) screenControllerLayout(void)
{
    u8 record[48];
    u8 item0[56], item1[56], item2[56], item3[56];
    u8 arrow_l[56], arrow_r[56];
    u8 pic0[56], pic1[56], pic2[56], pic3[56];
    u8 sel_next[16], sel_prev[16];
    u8 wait[24], set0[24], set1[24], set2[24];
    u8 move_l[32], move_r[32];
    u8 snd_move[32], snd_ok[32];
    char *name;
    s32 result;

    sceneInit(&shellScene, screenTitle, screenOptions, shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -1200, 0, 0, 0, 0);
    switch (settingsGetControlLayout(&sessionSettings, controlsLayoutPlayer)) {
    case 0:
        name = strShellKeypad1Tim;
        break;
    case 1:
        name = strShellKeypad2Tim;
        break;
    case 2:
        name = strShellKeypad3Tim;
        break;
    case 3:
        name = strShellKeypad4Tim;
        break;
    }
    screenImageRecordInit(record, name, 0x80048270);
    sub_80137F60(item0, 0, 0, screenOptions);
    sub_80137F60(item1, 0, 0, screenOptions);
    sub_80137F60(item2, 0, 0, screenOptions);
    sub_80137F60(item3, 0, 0, screenOptions);
    sub_80123E98(arrow_l, 9004, 1);
    sceneObjSetPosition(arrow_l, -416, -400, 40);
    sub_80123E98(arrow_r, 9004, 1);
    sceneObjSetPosition(arrow_r, 416, -400, 40);
    sceneObjSetRotation(arrow_r, 0, 0, 2048);
    menuLoadPictureInit(pic0, &shellScene, strShellKeypad1Tim);
    menuLoadPictureInit(pic1, &shellScene, strShellKeypad2Tim);
    menuLoadPictureInit(pic2, &shellScene, strShellKeypad3Tim);
    menuLoadPictureInit(pic3, &shellScene, strShellKeypad4Tim);
    menuSelectNextInit(sel_next, &shellScene);
    menuSelectPrevInit(sel_prev, &shellScene);
    sub_80123828(wait, &shellScene, controlsLayoutPlayer ? 2 : 0, 2000);
    sub_80123870(set0, &shellScene, 0);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    sub_801239CC(move_l, arrow_l, 0, 512, 0, 8);
    sub_801239CC(move_r, arrow_r, 0, 512, 0, 8);
    menuPlaySoundInit(snd_move, soundManagerObject, 28, 0, 0);
    menuPlaySoundInit(snd_ok, soundManagerObject, 29, 0, 0);
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, item0);
    sceneAddItem(&shellScene, item1);
    sceneAddItem(&shellScene, item2);
    sceneAddItem(&shellScene, item3);
    sceneAddDecor(&shellScene, arrow_l);
    sceneAddDecor(&shellScene, arrow_r);
    sceneAddAction(&shellScene, 0, 0, 0, wait);
    sceneAddAction(&shellScene, 9, 0, 2, snd_move);
    sceneAddAction(&shellScene, 9, 0, 0, move_l);
    sceneAddAction(&shellScene, 9, 0, 2, sel_prev);
    sceneAddAction(&shellScene, 9, item0, 2, pic3);
    sceneAddAction(&shellScene, 9, item1, 2, pic0);
    sceneAddAction(&shellScene, 9, item2, 2, pic1);
    sceneAddAction(&shellScene, 9, item3, 2, pic2);
    sceneAddAction(&shellScene, 10, 0, 2, snd_move);
    sceneAddAction(&shellScene, 10, 0, 0, move_r);
    sceneAddAction(&shellScene, 10, 0, 2, sel_next);
    sceneAddAction(&shellScene, 10, item0, 2, pic1);
    sceneAddAction(&shellScene, 10, item1, 2, pic2);
    sceneAddAction(&shellScene, 10, item2, 2, pic3);
    sceneAddAction(&shellScene, 10, item3, 2, pic0);
    sceneAddAction(&shellScene, 3, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 3, 0, 1, wait);
    sceneAddAction(&shellScene, 3, 0, 0, set0);
    sceneAddAction(&shellScene, 14, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 14, 0, 1, wait);
    sceneAddAction(&shellScene, 14, 0, 0, set0);
    sceneAddAction(&shellScene, 15, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 15, 0, 1, wait);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 16, 0, 1, wait);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 2, wait, 2, set1);
    sceneLoadTmsDmd(&shellScene, strShellArrowsScene);
    screenImageRecordLoad(record);
    sceneSetSelection(&shellScene, settingsGetControlLayout(&sessionSettings, controlsLayoutPlayer));
    result = sceneRun(&shellScene);
    if (sceneGetSelectedItem2(&shellScene) == item0)
        settingsSetControlLayout(&sessionSettings, controlsLayoutPlayer, 0);
    if (sceneGetSelectedItem2(&shellScene) == item1)
        settingsSetControlLayout(&sessionSettings, controlsLayoutPlayer, 1);
    if (sceneGetSelectedItem2(&shellScene) == item2)
        settingsSetControlLayout(&sessionSettings, controlsLayoutPlayer, 2);
    if (sceneGetSelectedItem2(&shellScene) == item3)
        settingsSetControlLayout(&sessionSettings, controlsLayoutPlayer, 3);
    return (void *)result;
}
