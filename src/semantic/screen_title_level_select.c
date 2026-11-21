/* Game code (title screen and level select). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/scene.h"
#include "semantic/settings.h"

typedef void (*TM2Method)(u8 *, ...);

extern struct TM2ShellScene shellScene;             /* the shell scene */

#pragma tm2_absolute soundManagerObject
extern s32 soundManagerObject;              /* sound object */
extern u8 *musicManagerObject;              /* music object */
#pragma tm2_absolute musicManagerObject
extern s32 audioCdVolume, audioSfxVolume, audioVolumeClampMax, audioVolumeClampMin;
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioSfxVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern char strShellTitleTim[];           /* "SHELLDB1\TITLE.TIM" */
extern char strShellTitleScene[];           /* "SHELLDB1\TITLE" */

void *screenTitle(void);           /* title screen */
extern void *shellAttractCycle(void *value);
extern void *screenSinglePlayerMenu(void);   /* 8012E908 single-player menu */
extern void *screenMultiPlayer(void);
extern void *screenOptions(void);

extern s32 soundStop(s32);
extern void soundUpdate(void);
extern s32 soundFlush(void);
extern s32 audioGetMusicVolume(void);
extern void audioFadeOutMusic(s32);
extern s32 soundPlay(s32, s32, s32, s32);
extern void SsSetSerialVol(char s_num, short voll, short volr);

extern void sceneInit(struct TM2ShellScene *scene, void *back, void *cancel, void *idle, s32);
extern u16 sceneSetCamera(struct TM2ShellScene *scene, s32 x, s32 y, s32 z, u16, u16, u16);
extern void *screenImageRecordInit(void *record, const char *name, s32 buffer);
extern void screenImageRecordLoad(void *record);
extern void *sub_80137F60(void *item, u16 id, u16 count, void *next);
extern void sceneObjSetPosition(void *object, s32 x, s32 y, s32 z);
extern void sceneObjSetUseCamera(void *object, u16 v);
extern void sub_80123E5C(void *object, u16 v);
extern void sceneObjSetScale(void *object, s32 v);
extern void *sub_80123E98(void *object, u16 first, u16 second);
extern void *menuSelectNextInit(void *object, void *owner);
extern void *menuSelectPrevInit(void *object, void *owner);
extern void *sub_80123828(void *object, void *target, s32, s32);
extern void *sub_80123870(void *object, void *target, s32);
extern void *sub_80123A20(void *object, void *target, s32, s32, s32, s32);
extern void *sub_80123B64(void *object, void *target, s32, s32, s32);
extern void *menuPlaySoundInit(void *object, s32 actor, s32, s32, s32);
extern void sceneSetBackground(struct TM2ShellScene *scene, void *record);
extern s32 sceneAddItem(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddDecor(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddAction(struct TM2ShellScene *scene, s32 event, void *source, s32 mode, void *action);
extern s32 sceneLoadTmsDmd(struct TM2ShellScene *scene, const char *name);
extern s32 sceneRun(struct TM2ShellScene *scene);
extern s32 sceneGetExitCode(struct TM2ShellScene *scene);

/* Clamp a serial (CD audio) volume to [audioVolumeClampMin, audioVolumeClampMax] and apply it. */
static __inline__ void SetSerialVolume(s32 v)
{
    audioCdVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioCdVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
    SsSetSerialVol(0, (s16)audioCdVolume, (s16)audioCdVolume);
}

/* Clamp the second volume (audioSfxVolume) to [audioVolumeClampMin, audioVolumeClampMax]. */
static __inline__ void SetSfxVolume(s32 v)
{
    audioSfxVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioSfxVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
}

/* Call music-object virtual slot `slot` with one argument. */
#define MUSIC_CALL(slot, arg) do { \
    u8 *obj_ = musicManagerObject; u8 *vt_ = *(u8 **)(obj_ + 8); \
    (*(TM2Method *)(vt_ + (slot) * 8 + 4))(obj_ + *(s16 *)(vt_ + (slot) * 8), arg); } while (0)

/* @8012DF84: title screen (SHELLDB1\TITLE): three items (9001 single player
 * -> 8012E908, 9002 -> 8012F5F8, 9003 -> 80133C08) plus a logo, left/right
 * (events 9/10) moves, turn-in and bob animations.  Starts the title music
 * (music slot 2, arg 11) at the CD volume; on exit via event 3 stops it with
 * the current volume, otherwise plays the start jingle (27) at the current
 * level and restores the sfx volume; finally mutes CD audio. */
void *__attribute__((section(".text_8012DF84"))) screenTitle(void)
{
    u8 record[48];
    u8 single[56], multi[56], options[56];
    u8 logo[56];
    u8 sel_next[16], sel_prev[16];
    u8 wait[24], set0[24], set3[24];
    u8 turn1[32], turn2[32], turn3[32];
    u8 bob1_up[32], bob1_down[32], bob2_up[32], bob2_down[32], bob3_up[32], bob3_down[32];
    u8 sound_move[32], sound_back[32];
    struct TM2ShellScene *scene = &shellScene;
    s32 result, saved, vol;
    u8 *obj, *vt;
    s16 delta;

    soundStop(27);
    soundUpdate();
    soundFlush();
    sceneInit(scene, screenTitle, 0, shellAttractCycle, 0);
    sceneSetCamera(scene, 0, -1200, 0, 0, 0, 0);
    screenImageRecordInit(record, strShellTitleTim, 0x80048270);

    sub_80137F60(single, 0x2329, 1, screenSinglePlayerMenu);
    sceneObjSetPosition(single, -1440, 0, -680);
    sceneObjSetUseCamera(single, 0);
    sub_80123E5C(single, 0);
    sceneObjSetScale(single, 1);
    sub_80137F60(multi, 0x232A, 1, screenMultiPlayer);
    sceneObjSetPosition(multi, 0, 0, -1280);
    sceneObjSetUseCamera(multi, 0);
    sub_80123E5C(multi, 0);
    sceneObjSetScale(multi, 1);
    sub_80137F60(options, 0x232B, 1, screenOptions);
    sceneObjSetPosition(options, 1440, 0, -680);
    sceneObjSetUseCamera(options, 0);
    sub_80123E5C(options, 0);
    sceneObjSetScale(options, 1);
    sub_80123E98(logo, 0x232C, 1);
    sceneObjSetPosition(logo, 56, 0, -184);
    sceneObjSetUseCamera(logo, 0);
    sub_80123E5C(logo, 0);
    sceneObjSetScale(logo, 1);

    menuSelectNextInit(sel_next, scene);
    menuSelectPrevInit(sel_prev, scene);
    sub_80123828(wait, scene, 0, 2000);
    sub_80123870(set0, scene, 0);
    sub_80123870(set3, scene, 3);
    sub_80123A20(turn1, single, 40, 0, 0, 15);
    sub_80123A20(turn2, multi, 0, 0, 40, 15);
    sub_80123A20(turn3, options, -40, 0, 0, 15);
    sub_80123B64(bob1_up, single, 0, 1, 0);
    sub_80123B64(bob1_down, single, 0, -1, 0);
    sub_80123B64(bob2_up, multi, 0, 1, 0);
    sub_80123B64(bob2_down, multi, 0, -1, 0);
    sub_80123B64(bob3_up, options, 0, 1, 0);
    sub_80123B64(bob3_down, options, 0, -1, 0);
    menuPlaySoundInit(sound_move, soundManagerObject, 0x1C, 0, 0);
    menuPlaySoundInit(sound_back, soundManagerObject, 0x1D, 0, 0);
    sceneSetBackground(scene, record);

    sceneAddItem(scene, single);
    sceneAddItem(scene, multi);
    sceneAddItem(scene, options);
    sceneAddDecor(scene, logo);

    sceneAddAction(scene, 9, 0, 2, sound_move);
    sceneAddAction(scene, 9, 0, 2, sel_prev);
    sceneAddAction(scene, 10, 0, 2, sound_move);
    sceneAddAction(scene, 10, 0, 2, sel_next);
    sceneAddAction(scene, 3, 0, 2, sound_back);
    sceneAddAction(scene, 3, 0, 1, wait);
    sceneAddAction(scene, 3, 0, 0, set0);
    sceneAddAction(scene, 14, 0, 2, sound_back);
    sceneAddAction(scene, 14, 0, 1, wait);
    sceneAddAction(scene, 14, 0, 0, set0);
    sceneAddAction(scene, 0, 0, 0, turn1);
    sceneAddAction(scene, 0, 0, 0, turn2);
    sceneAddAction(scene, 0, 0, 0, turn3);
    sceneAddAction(scene, 2, turn3, 2, sound_move);
    sceneAddAction(scene, 2, turn3, 2, bob1_up);
    sceneAddAction(scene, 2, turn3, 0, wait);
    sceneAddAction(scene, 9, single, 2, bob1_down);
    sceneAddAction(scene, 9, single, 2, bob3_up);
    sceneAddAction(scene, 10, single, 2, bob1_down);
    sceneAddAction(scene, 10, single, 2, bob2_up);
    sceneAddAction(scene, 9, multi, 2, bob2_down);
    sceneAddAction(scene, 9, multi, 2, bob1_up);
    sceneAddAction(scene, 10, multi, 2, bob2_down);
    sceneAddAction(scene, 10, multi, 2, bob3_up);
    sceneAddAction(scene, 9, options, 2, bob3_down);
    sceneAddAction(scene, 9, options, 2, bob2_up);
    sceneAddAction(scene, 10, options, 2, bob3_down);
    sceneAddAction(scene, 10, options, 2, bob1_up);
    sceneAddAction(scene, 2, wait, 2, set3);

    sceneLoadTmsDmd(scene, strShellTitleScene);
    screenImageRecordLoad(record);
    MUSIC_CALL(2, 11);
    SetSerialVolume(audioGetMusicVolume());
    result = sceneRun(scene);
    if (sceneGetExitCode(scene) == 3) {
        audioFadeOutMusic(audioCdVolume);
        MUSIC_CALL(3, 11);
    } else {
        saved = audioSfxVolume;
        SetSfxVolume(audioGetMusicVolume());
        soundPlay(27, 0, 0, 99);
        soundUpdate();
        soundFlush();
        /* SetSfxVolume(saved) + MUSIC_CALL(3, 11), written out so the final
         * volume store lands after the vtable loads as in retail */
        audioSfxVolume = saved < audioVolumeClampMax ? saved : audioVolumeClampMax;
        vol = saved < audioVolumeClampMin ? audioVolumeClampMin : saved;
        obj = musicManagerObject;
        vt = *(u8 **)(obj + 8);
        delta = *(s16 *)(vt + 24);
        audioSfxVolume = vol;
        (*(TM2Method *)(vt + 28))(obj + delta, 11);
    }
    SetSerialVolume(0);
    return (void *)result;
}

extern s32 secretLevelCodeTable[];            /* secret-level code table (mode 4) */
extern s32 secretLevelCodeTableOther[];            /* secret-level code table (other modes) */
extern char strShellLevelSelTim[];           /* "SHELLDB1\LEVELSEL.TIM" */
extern char strShellLevelSelScene[];           /* "SHELLDB1\LEVELSEL" */
extern char strSecretLevelSelected[];           /* "secret level selected" */
extern u8 strEmpty[];
extern void *screenCarSelect(void);    /* car select */
extern s32 menuLevelNameText(s32 index);
extern s32 settingsGetMode(struct TM2Settings *settings);
extern s32 settingsGetLevel(struct TM2Settings *settings);
extern s32 tournamentOpponentCount(struct TM2Settings *settings, s32 level);
extern s32 settingsGetHumans(struct TM2Settings *settings);
extern void settingsSetOpponents(struct TM2Settings *settings, s32 v);
extern void settingsSetLevel(struct TM2Settings *settings, s32 v);
extern void sceneObjSetRotation(void *object, s32 x, s32 y, s32 z);
extern void *menuTextInit(void *object, s32, void *value, s8 mode, u16 first, u16 second, u16 third);
extern void *sub_8012DB10(void *object, void *scene, s32, void *tab, s32);
extern void *sub_801239CC(void *object, void *target, s32, s32, s32, s32);
extern void menuTextDestroy(void *object, s32);
extern s32 codeDetectorGetLastCode(void *object);
extern void *sub_80136FB8(void *object, void *, void *, void *text_callback);
extern s32 sceneObjGetChoiceState(void *object);
extern int fontPrintCenteredXY(s32, s32, s32, char *);

/* @8012FD30: level select (SHELLDB1\LEVELSEL): a car-select item (9000 ->
 * 801307EC), the level wheel (9001) and two arrows, a level number display,
 * and a code detector (8012DB10) fed with the secretLevelCodeTable table in mode 4,
 * secretLevelCodeTableOther otherwise.  Left/right spin the wheel; back goes to the
 * single-player menu in mode 3, else 8012F5F8.  On exit with result 0 the
 * chosen level (wheel index + 1) is stored (mode 3 also resets the
 * difficulty offset); result 4 (a secret code typed) selects level
 * code index + 10, prints "secret level selected" and goes to car select. */
void *__attribute__((section(".text_8012FD30"))) screenLevelSelect(void)
{
    s32 codes4[12], codes[12];
    u8 record[48];
    u8 select[56], wheel[56], arrow_l[56], arrow_r[56];
    u8 number[72];
    u8 detector[64];
    u8 set0[24], set1[24], set2[24], set4[24];
    u8 sel_r1[32], sel_r2[32], sel_l1[32], sel_l2[32];
    u8 wheel_r1[32], wheel_r2[32], wheel_l1[32], wheel_l2[32];
    u8 arrow_l_move[32], arrow_r_move[32];
    u8 bob_up[32], bob_down[32];
    u8 show[32];
    u8 sound_move[32], sound_back[32];
    void *result;

    __builtin_memcpy(codes4, secretLevelCodeTable, 48);
    __builtin_memcpy(codes, secretLevelCodeTableOther, 48);
    sceneInit(&shellScene, screenTitle,
                 settingsGetMode(&sessionSettings) == 3 ? screenSinglePlayerMenu : screenMultiPlayer,
                 shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -2800, 0, 0, 0, 0);
    screenImageRecordInit(record, strShellLevelSelTim, 0x80048270);
    sub_80137F60(select, 0x2328, 1, screenCarSelect);
    sub_80123E98(wheel, 0x2329, 1);
    sub_80123E98(arrow_l, 0x232C, 1);
    sceneObjSetPosition(arrow_l, -280, -2240, 0);
    sub_80123E98(arrow_r, 0x232C, 1);
    sceneObjSetPosition(arrow_r, 280, -2240, 0);
    sceneObjSetRotation(arrow_r, 0, 0, 2048);
    menuTextInit(number, 2, strEmpty, 1, 160, 205, 1);
    sub_8012DB10(detector, &shellScene, (settingsGetMode(&sessionSettings) != 3) * 2,
                 settingsGetMode(&sessionSettings) == 4 ? codes4 : codes, 2000);
    sub_80123870(set0, &shellScene, 0);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    sub_80123870(set4, &shellScene, 4);
    sub_801239CC(sel_r1, select, 256, 0, 0, 8);
    sub_801239CC(sel_r2, select, 256, 0, 0, 8);
    sub_801239CC(sel_l1, select, -256, 0, 0, 8);
    sub_801239CC(sel_l2, select, -256, 0, 0, 8);
    sub_801239CC(wheel_r1, wheel, 256, 0, 0, 8);
    sub_801239CC(wheel_r2, wheel, 256, 0, 0, 8);
    sub_801239CC(wheel_l1, wheel, -256, 0, 0, 8);
    sub_801239CC(wheel_l2, wheel, -256, 0, 0, 8);
    sub_801239CC(arrow_l_move, arrow_l, 0, 256, 0, 16);
    sub_801239CC(arrow_r_move, arrow_r, 0, 256, 0, 16);
    sub_80123B64(bob_up, wheel, 0, 1, 0);
    sub_80123B64(bob_down, wheel, 0, -1, 0);
    sub_80136FB8(show, number, wheel, menuLevelNameText);
    menuPlaySoundInit(sound_move, soundManagerObject, 0x1C, 0, 0);
    menuPlaySoundInit(sound_back, soundManagerObject, 0x1D, 0, 0);
    sceneSetBackground(&shellScene, record);

    sceneAddItem(&shellScene, select);
    sceneAddDecor(&shellScene, wheel);
    sceneAddDecor(&shellScene, arrow_l);
    sceneAddDecor(&shellScene, arrow_r);
    sceneAddDecor(&shellScene, number);

    sceneAddAction(&shellScene, 3, 0, 1, detector);
    sceneAddAction(&shellScene, 3, 0, 2, sound_back);
    sceneAddAction(&shellScene, 3, 0, 0, set0);
    sceneAddAction(&shellScene, 14, 0, 1, detector);
    sceneAddAction(&shellScene, 14, 0, 2, sound_back);
    sceneAddAction(&shellScene, 14, 0, 0, set0);
    sceneAddAction(&shellScene, 17, 0, 1, detector);
    sceneAddAction(&shellScene, 17, 0, 2, sound_back);
    sceneAddAction(&shellScene, 17, 0, 2, set4);
    sceneAddAction(&shellScene, 18, 0, 1, detector);
    sceneAddAction(&shellScene, 18, 0, 2, sound_back);
    sceneAddAction(&shellScene, 18, 0, 2, set4);
    sceneAddAction(&shellScene, 19, 0, 1, detector);
    sceneAddAction(&shellScene, 19, 0, 2, sound_back);
    sceneAddAction(&shellScene, 19, 0, 2, set4);
    sceneAddAction(&shellScene, 15, 0, 2, sound_back);
    sceneAddAction(&shellScene, 15, 0, 1, detector);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, sound_back);
    sceneAddAction(&shellScene, 16, 0, 1, detector);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 0, 0, 2, show);
    sceneAddAction(&shellScene, 0, 0, 0, detector);
    sceneAddAction(&shellScene, 9, 0, 1, detector);
    sceneAddAction(&shellScene, 9, 0, 2, sound_move);
    sceneAddAction(&shellScene, 9, 0, 0, sel_r1);
    sceneAddAction(&shellScene, 9, 0, 0, wheel_r1);
    sceneAddAction(&shellScene, 9, 0, 0, arrow_l_move);
    sceneAddAction(&shellScene, 2, wheel_r1, 2, bob_down);
    sceneAddAction(&shellScene, 2, wheel_r1, 0, sel_r2);
    sceneAddAction(&shellScene, 2, wheel_r1, 0, wheel_r2);
    sceneAddAction(&shellScene, 2, wheel_r1, 2, show);
    sceneAddAction(&shellScene, 2, wheel_r2, 0, detector);
    sceneAddAction(&shellScene, 10, 0, 1, detector);
    sceneAddAction(&shellScene, 10, 0, 2, sound_move);
    sceneAddAction(&shellScene, 10, 0, 0, sel_l1);
    sceneAddAction(&shellScene, 10, 0, 0, wheel_l1);
    sceneAddAction(&shellScene, 10, 0, 0, arrow_r_move);
    sceneAddAction(&shellScene, 2, wheel_l1, 2, bob_up);
    sceneAddAction(&shellScene, 2, wheel_l1, 0, sel_l2);
    sceneAddAction(&shellScene, 2, wheel_l1, 0, wheel_l2);
    sceneAddAction(&shellScene, 2, wheel_l1, 2, show);
    sceneAddAction(&shellScene, 2, wheel_l2, 0, detector);
    sceneAddAction(&shellScene, 2, detector, 0, set1);

    sceneLoadTmsDmd(&shellScene, strShellLevelSelScene);
    screenImageRecordLoad(record);
    result = (void *)sceneRun(&shellScene);
    if (sceneGetExitCode(&shellScene) == 0) {
        settingsSetLevel(&sessionSettings, sceneObjGetChoiceState(wheel) + 1);
        if (settingsGetMode(&sessionSettings) == 3)
            settingsSetOpponents(&sessionSettings, tournamentOpponentCount(&sessionSettings, settingsGetLevel(&sessionSettings))
                                     - settingsGetHumans(&sessionSettings));
    } else if (sceneGetExitCode(&shellScene) == 4) {
        settingsSetLevel(&sessionSettings, (u8)codeDetectorGetLastCode(detector) + 10);
        fontPrintCenteredXY(2, 159, 149, strSecretLevelSelected);
        result = screenCarSelect;
    }
    menuTextDestroy(number, 2);
    return result;
}
