/* Game code (shell screens). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/scene.h"
#include "semantic/settings.h"

typedef void (*SS3Method)(u8 *, ...);
typedef struct { s16 delta; s16 index; SS3Method fn; } SS3VEntry;   /* vtable slot */

extern struct TM2ShellScene shellScene;             /* the shell scene */

extern u8 *soundManagerObject;              /* sound object */
#pragma tm2_absolute soundManagerObject
extern u8 *musicManagerObject;              /* music object */
#pragma tm2_absolute musicManagerObject
extern s32 audioCdVolume, audioSfxVolume, audioVolumeClampMax, audioVolumeClampMin;
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioSfxVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern char strShellAudioTim[];           /* "SHELLDB2\AUDIO.TIM" */
extern char strShellAudioScene[];           /* "SHELLDB2\AUDIO" */

extern void *screenTitle(void);    /* title screen */
extern void *shellAttractCycle(void *value);
void *screenOptions(void);

extern s32 soundStop(s32);
extern void soundUpdate(void);
extern s32 soundFlush(void);
extern s32 audioGetMusicVolume(void);
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
extern void *menuSliderCreate(void *object, u16 first, u16 second);    /* slider (volume bar) */
extern void *menuSelectNextInit(void *object, void *owner);   /* action: select the next item */
extern void *menuSelectPrevInit(void *object, void *owner);   /* action: select the previous item */
extern void *sub_80123828(void *object, void *target, s32, s32);
extern void *sub_80123870(void *object, void *target, s32);
extern void *sub_80123A20(void *object, void *target, s32, s32, s32, s32);
extern void *sub_80123B64(void *object, void *target, s32, s32, s32);
extern void *menuSliderIncInit(void *object, void *slider);   /* action: slider up */
extern void *menuSliderDecInit(void *object, void *slider);   /* action: slider down */
extern void *menuVolumeApplyInit(void *object, void *slider, s16 which);   /* action: apply slider */
extern void *menuPlaySoundInit(void *object, u8 *actor, s32, s32, s32);
extern void sceneSetBackground(struct TM2ShellScene *scene, void *record);
extern s32 sceneAddItem(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddDecor(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddAction(struct TM2ShellScene *scene, s32 event, void *source, s32 mode, void *action);
extern s32 sceneLoadTmsDmd(struct TM2ShellScene *scene, const char *name);
extern s32 sceneRun(struct TM2ShellScene *scene);
extern void menuSliderSetValue(void *slider, s32 value);
extern s32 soundPlay(s32, s32, s32, s32);
extern void sceneObjSetChoiceState(void *object, s32 value);
extern s32 sceneObjGetChoiceState(void *object);
extern void *sceneGetSelectedItem2(struct TM2ShellScene *scene);
extern u8 settingsGetDifficulty(struct TM2Settings *settings);
extern void settingsSetDifficulty(struct TM2Settings *settings, u8 v);
extern u8 settingsGetWorldMap(struct TM2Settings *settings);
extern void settingsSetWorldMap(struct TM2Settings *settings, u8 v);
extern u8 settingsGetControlLayout(struct TM2Settings *settings, u8 port);
extern void settingsSetControlLayout(struct TM2Settings *settings, u8 port, u8 v);
extern void *screenControllerLayout(void);    /* controller layout (KEYPAD1-4) */
extern void *screenHistory(void);    /* history */
void *screenAudio(void);
extern u8 controlsLayoutPlayer;               /* controller-layout screen: player 0 / 1 */
extern char strShellOptsTim[];           /* "SHELLDB2\OPTS.TIM" */
extern char strShellOptionsScene[];           /* "SHELLDB2\OPTIONS" */


/* Clamp a level into audioSfxVolume (to [audioVolumeClampMin, audioVolumeClampMax]). */
static __inline__ void SS3SetE18(s32 v)
{
    audioSfxVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioSfxVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
}

/* @80133C08: OPTIONS screen (SHELLDB2 OPTS / OPTIONS). Plays sound 27 with the
 * effects level audioSfxVolume temporarily set from audioGetMusicVolume(), then restores it.
 * Six items: 9001 difficulty, 9002 a second toggle, 9003 / 9009 player 1 /
 * player 2 controller layout (-> screenControllerLayout), 9004 audio (-> screenAudio),
 * 9005 history (-> screenHistory). Items 1-4 carry a value label (9006, 9007,
 * 9008, 9010) cycled with left/right (events 9/10). The labels are loaded from
 * settings sessionSettings[36], [37], [38], [39] before the scene runs and written
 * back after it; controlsLayoutPlayer (gp+840) = 0 when the controller-layout entry chosen
 * was player 1's (9003), else 1. */
void *__attribute__((section(".text_80133C08"))) screenOptions(void)
{
    u8 record[48];
    u8 it1[56], it2[56], it3[56], it4[56], it5[56], it6[56];
    u8 lab1[56], lab2[56], lab3[56], lab4[56];
    u8 sel_next[16], sel_prev[16];
    u8 wait[24], wait2[24], set0[24], set1[24], set2[24];
    u8 turn1[32], turn2[32], turn3[32], turn4[32], turn5[32], turn6[32];
    u8 up1[32], down1[32], lup1[32], ldown1[32];
    u8 up2[32], down2[32], lup2[32], ldown2[32];
    u8 up3[32], down3[32], lup3[32], ldown3[32];
    u8 up4[32], down4[32], lup4[32], ldown4[32];
    u8 up5[32], down5[32], up6[32], down6[32];
    u8 snd_move[32], snd_ok[32];
    s32 saved;
    s32 result;

    saved = audioSfxVolume;
    SS3SetE18(audioGetMusicVolume());
    soundPlay(27, 0, 0, 99);
    soundUpdate();
    soundFlush();
    SS3SetE18(saved);
    sceneInit(&shellScene, screenTitle, screenTitle, shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -1200, 0, 0, 0, 0);
    screenImageRecordInit(record, strShellOptsTim, 0x80048270);
    sub_80137F60(it1, 9001, 1, screenTitle);
    sceneObjSetPosition(it1, -1784, 0, 360);
    sceneObjSetUseCamera(it1, 0);
    sub_80123E5C(it1, 0);
    sceneObjSetScale(it1, 1);
    sub_80137F60(it2, 9002, 1, screenTitle);
    sceneObjSetPosition(it2, -584, 0, 176);
    sceneObjSetUseCamera(it2, 0);
    sub_80123E5C(it2, 0);
    sceneObjSetScale(it2, 1);
    sub_80137F60(it3, 9003, 1, screenControllerLayout);
    sceneObjSetPosition(it3, -1784, 0, -8);
    sceneObjSetUseCamera(it3, 0);
    sub_80123E5C(it3, 0);
    sceneObjSetScale(it3, 1);
    sub_80137F60(it4, 9009, 1, screenControllerLayout);
    sceneObjSetPosition(it4, -584, 0, -192);
    sceneObjSetUseCamera(it4, 0);
    sub_80123E5C(it4, 0);
    sceneObjSetScale(it4, 1);
    sub_80137F60(it5, 9004, 1, screenAudio);
    sceneObjSetPosition(it5, -1784, 0, -376);
    sceneObjSetUseCamera(it5, 0);
    sub_80123E5C(it5, 0);
    sceneObjSetScale(it5, 1);
    sub_80137F60(it6, 9005, 1, screenHistory);
    sceneObjSetPosition(it6, -584, 0, -560);
    sceneObjSetUseCamera(it6, 0);
    sub_80123E5C(it6, 0);
    sceneObjSetScale(it6, 1);
    sub_80123E98(lab1, 9006, 1);
    sceneObjSetPosition(lab1, -320, 0, 360);
    sceneObjSetUseCamera(lab1, 0);
    sub_80123E5C(lab1, 0);
    sceneObjSetScale(lab1, 1);
    sub_80123E98(lab2, 9007, 1);
    sceneObjSetPosition(lab2, -320, 0, 176);
    sceneObjSetUseCamera(lab2, 0);
    sub_80123E5C(lab2, 0);
    sceneObjSetScale(lab2, 1);
    sub_80123E98(lab3, 9008, 1);
    sceneObjSetPosition(lab3, -320, 0, -8);
    sceneObjSetUseCamera(lab3, 0);
    sub_80123E5C(lab3, 0);
    sceneObjSetScale(lab3, 1);
    sub_80123E98(lab4, 9010, 1);
    sceneObjSetPosition(lab4, -320, 0, -192);
    sceneObjSetUseCamera(lab4, 0);
    sub_80123E5C(lab4, 0);
    sceneObjSetScale(lab4, 1);
    menuSelectNextInit(sel_next, &shellScene);
    menuSelectPrevInit(sel_prev, &shellScene);
    sub_80123828(wait, &shellScene, 0, 2000);
    sub_80123828(wait2, &shellScene, 2, 2000);
    sub_80123870(set0, &shellScene, 0);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    sub_80123A20(turn1, it1, 40, 0, 0, 15);
    sub_80123A20(turn2, it2, -40, 0, 0, 15);
    sub_80123A20(turn3, it3, 40, 0, 0, 15);
    sub_80123A20(turn4, it4, -40, 0, 0, 15);
    sub_80123A20(turn5, it5, 40, 0, 0, 15);
    sub_80123A20(turn6, it6, -40, 0, 0, 15);
    sub_80123B64(up1, it1, 0, 1, 0);
    sub_80123B64(down1, it1, 0, -1, 0);
    sub_80123B64(lup1, lab1, 0, 1, 0);
    sub_80123B64(ldown1, lab1, 0, -1, 0);
    sub_80123B64(up2, it2, 0, 1, 0);
    sub_80123B64(down2, it2, 0, -1, 0);
    sub_80123B64(lup2, lab2, 0, 1, 0);
    sub_80123B64(ldown2, lab2, 0, -1, 0);
    sub_80123B64(up3, it3, 0, 1, 0);
    sub_80123B64(down3, it3, 0, -1, 0);
    sub_80123B64(lup3, lab3, 0, 1, 0);
    sub_80123B64(ldown3, lab3, 0, -1, 0);
    sub_80123B64(up4, it4, 0, 1, 0);
    sub_80123B64(down4, it4, 0, -1, 0);
    sub_80123B64(lup4, lab4, 0, 1, 0);
    sub_80123B64(ldown4, lab4, 0, -1, 0);
    sub_80123B64(up5, it5, 0, 1, 0);
    sub_80123B64(down5, it5, 0, -1, 0);
    sub_80123B64(up6, it6, 0, 1, 0);
    sub_80123B64(down6, it6, 0, -1, 0);
    menuPlaySoundInit(snd_move, soundManagerObject, 28, 0, 0);
    menuPlaySoundInit(snd_ok, soundManagerObject, 29, 0, 0);
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, it1);
    sceneAddDecor(&shellScene, lab1);
    sceneAddItem(&shellScene, it2);
    sceneAddDecor(&shellScene, lab2);
    sceneAddItem(&shellScene, it3);
    sceneAddDecor(&shellScene, lab3);
    sceneAddItem(&shellScene, it4);
    sceneAddDecor(&shellScene, lab4);
    sceneAddItem(&shellScene, it5);
    sceneAddItem(&shellScene, it6);
    sceneAddAction(&shellScene, 7, 0, 2, snd_move);
    sceneAddAction(&shellScene, 7, 0, 2, sel_prev);
    sceneAddAction(&shellScene, 8, 0, 2, snd_move);
    sceneAddAction(&shellScene, 8, 0, 2, sel_next);
    sceneAddAction(&shellScene, 15, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 15, 0, 1, wait);
    sceneAddAction(&shellScene, 15, 0, 1, wait2);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 16, 0, 1, wait);
    sceneAddAction(&shellScene, 16, 0, 1, wait2);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 0, 0, 0, turn1);
    sceneAddAction(&shellScene, 0, 0, 0, turn2);
    sceneAddAction(&shellScene, 0, 0, 0, turn3);
    sceneAddAction(&shellScene, 0, 0, 0, turn4);
    sceneAddAction(&shellScene, 0, 0, 0, turn5);
    sceneAddAction(&shellScene, 0, 0, 0, turn6);
    sceneAddAction(&shellScene, 2, turn1, 2, snd_move);
    sceneAddAction(&shellScene, 2, turn1, 2, up1);
    sceneAddAction(&shellScene, 2, turn1, 0, wait);
    sceneAddAction(&shellScene, 7, it1, 2, down1);
    sceneAddAction(&shellScene, 7, it1, 2, up6);
    sceneAddAction(&shellScene, 8, it1, 2, down1);
    sceneAddAction(&shellScene, 8, it1, 2, up2);
    sceneAddAction(&shellScene, 9, it1, 2, snd_move);
    sceneAddAction(&shellScene, 9, it1, 2, ldown1);
    sceneAddAction(&shellScene, 10, it1, 2, snd_move);
    sceneAddAction(&shellScene, 10, it1, 2, lup1);
    sceneAddAction(&shellScene, 7, it2, 2, down2);
    sceneAddAction(&shellScene, 7, it2, 2, up1);
    sceneAddAction(&shellScene, 8, it2, 2, down2);
    sceneAddAction(&shellScene, 8, it2, 2, up3);
    sceneAddAction(&shellScene, 9, it2, 2, snd_move);
    sceneAddAction(&shellScene, 9, it2, 2, ldown2);
    sceneAddAction(&shellScene, 10, it2, 2, snd_move);
    sceneAddAction(&shellScene, 10, it2, 2, lup2);
    sceneAddAction(&shellScene, 7, it3, 2, down3);
    sceneAddAction(&shellScene, 7, it3, 2, up2);
    sceneAddAction(&shellScene, 8, it3, 2, down3);
    sceneAddAction(&shellScene, 8, it3, 2, up4);
    sceneAddAction(&shellScene, 8, it3, 1, wait);
    sceneAddAction(&shellScene, 8, it3, 0, wait2);
    sceneAddAction(&shellScene, 9, it3, 2, snd_move);
    sceneAddAction(&shellScene, 9, it3, 2, ldown3);
    sceneAddAction(&shellScene, 10, it3, 2, snd_move);
    sceneAddAction(&shellScene, 10, it3, 2, lup3);
    sceneAddAction(&shellScene, 3, it3, 2, snd_ok);
    sceneAddAction(&shellScene, 3, it3, 1, wait);
    sceneAddAction(&shellScene, 3, it3, 0, set0);
    sceneAddAction(&shellScene, 14, it3, 2, snd_ok);
    sceneAddAction(&shellScene, 14, it3, 1, wait);
    sceneAddAction(&shellScene, 14, it3, 0, set0);
    sceneAddAction(&shellScene, 7, it4, 2, down4);
    sceneAddAction(&shellScene, 7, it4, 2, up3);
    sceneAddAction(&shellScene, 7, it4, 1, wait2);
    sceneAddAction(&shellScene, 7, it4, 0, wait);
    sceneAddAction(&shellScene, 8, it4, 2, down4);
    sceneAddAction(&shellScene, 8, it4, 2, up5);
    sceneAddAction(&shellScene, 8, it4, 1, wait2);
    sceneAddAction(&shellScene, 8, it4, 0, wait);
    sceneAddAction(&shellScene, 9, it4, 2, snd_move);
    sceneAddAction(&shellScene, 9, it4, 2, ldown4);
    sceneAddAction(&shellScene, 10, it4, 2, snd_move);
    sceneAddAction(&shellScene, 10, it4, 2, lup4);
    sceneAddAction(&shellScene, 3, it4, 2, snd_ok);
    sceneAddAction(&shellScene, 3, it4, 1, wait);
    sceneAddAction(&shellScene, 3, it4, 0, set0);
    sceneAddAction(&shellScene, 14, it4, 2, snd_ok);
    sceneAddAction(&shellScene, 14, it4, 1, wait);
    sceneAddAction(&shellScene, 14, it4, 0, set0);
    sceneAddAction(&shellScene, 7, it5, 2, down5);
    sceneAddAction(&shellScene, 7, it5, 2, up4);
    sceneAddAction(&shellScene, 7, it5, 1, wait);
    sceneAddAction(&shellScene, 7, it5, 0, wait2);
    sceneAddAction(&shellScene, 8, it5, 2, down5);
    sceneAddAction(&shellScene, 8, it5, 2, up6);
    sceneAddAction(&shellScene, 3, it5, 2, snd_ok);
    sceneAddAction(&shellScene, 3, it5, 1, wait);
    sceneAddAction(&shellScene, 3, it5, 0, set0);
    sceneAddAction(&shellScene, 14, it5, 2, snd_ok);
    sceneAddAction(&shellScene, 14, it5, 1, wait);
    sceneAddAction(&shellScene, 14, it5, 0, set0);
    sceneAddAction(&shellScene, 7, it6, 2, down6);
    sceneAddAction(&shellScene, 7, it6, 2, up5);
    sceneAddAction(&shellScene, 8, it6, 2, down6);
    sceneAddAction(&shellScene, 8, it6, 2, up1);
    sceneAddAction(&shellScene, 3, it6, 2, snd_ok);
    sceneAddAction(&shellScene, 3, it6, 1, wait);
    sceneAddAction(&shellScene, 3, it6, 0, set0);
    sceneAddAction(&shellScene, 14, it6, 2, snd_ok);
    sceneAddAction(&shellScene, 14, it6, 1, wait);
    sceneAddAction(&shellScene, 14, it6, 0, set0);
    sceneAddAction(&shellScene, 2, wait, 2, set1);
    sceneAddAction(&shellScene, 2, wait2, 2, set1);
    sceneLoadTmsDmd(&shellScene, strShellOptionsScene);
    screenImageRecordLoad(record);
    sceneObjSetChoiceState(lab1, settingsGetDifficulty(&sessionSettings));
    sceneObjSetChoiceState(lab2, settingsGetWorldMap(&sessionSettings));
    sceneObjSetChoiceState(lab3, settingsGetControlLayout(&sessionSettings, 0));
    sceneObjSetChoiceState(lab4, settingsGetControlLayout(&sessionSettings, 1));
    result = sceneRun(&shellScene);
    if (sceneGetSelectedItem2(&shellScene) == it3)
        controlsLayoutPlayer = 0;
    else
        controlsLayoutPlayer = 1;
    settingsSetDifficulty(&sessionSettings, sceneObjGetChoiceState(lab1));
    settingsSetWorldMap(&sessionSettings, sceneObjGetChoiceState(lab2));
    settingsSetControlLayout(&sessionSettings, 0, sceneObjGetChoiceState(lab3));
    settingsSetControlLayout(&sessionSettings, 1, sceneObjGetChoiceState(lab4));
    return (void *)result;
}

/* Clamp a serial (CD audio) volume to [audioVolumeClampMin, audioVolumeClampMax] and apply it. */
static __inline__ void SS3SetSerialVolume(s32 v)
{
    audioCdVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioCdVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
    SsSetSerialVol(0, (s16)audioCdVolume, (s16)audioCdVolume);
}

/* @80135844: AUDIO options screen (SHELLDB2 AUDIO): two rows, music volume and
 * sound-effects volume, each a slider (menuSliderCreate) moved with left/right
 * (events 9/10). The music slider starts at (audioCdVolume + 1) / 8 and the effects
 * slider at audioSfxVolume / 7; the slider apply actions (menuVolumeApplyInit -> 80136FF8,
 * 1 = music: audioCdVolume = slider*8 (cap audioMusicVolumeCap), SsSetSerialVol and
 * audioSetMusicVolume -> musicVolume; 0 = effects: audioSfxVolume = slider*7 (cap audioSfxVolumeCap)
 * plus a preview sound 24). CD audio is started (music object vtable slot 2,
 * arg 11) for the screen and stopped (slot 3) with volume 0 after it.
 * Back/cancel return to screenOptions. */
void *__attribute__((section(".text_80135844"))) screenAudio(void)
{
    u8 record[48];
    u8 music[56], music_label[56], music_bar[536];
    u8 sfx[56], sfx_label[56], sfx_bar[536];
    u8 sel_next[16], sel_prev[16];
    u8 wait[24], set1[24], set2[24];
    u8 turn_a[32], turn_b[32];
    u8 up_a[32], down_a[32], up_b[32], down_b[32];
    u8 music_up[16], music_down[16], sfx_up[16], sfx_down[16];
    u8 music_set[24], sfx_set[24];
    u8 snd_move[32], snd_ok[32];
    SS3VEntry *e;
    s32 result;

    soundStop(27);
    soundUpdate();
    soundFlush();
    sceneInit(&shellScene, screenTitle, screenOptions, shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -1200, 0, 0, 0, 0);
    screenImageRecordInit(record, strShellAudioTim, 0x80048270);
    sub_80137F60(music, 0x2329, 1, screenOptions);
    sceneObjSetPosition(music, -1784, 0, 360);
    sceneObjSetUseCamera(music, 0);
    sub_80123E5C(music, 0);
    sceneObjSetScale(music, 1);
    sub_80123E98(music_label, 0x232D, 1);
    sceneObjSetPosition(music_label, -80, 0, 360);
    sceneObjSetUseCamera(music_label, 0);
    sub_80123E5C(music_label, 0);
    sceneObjSetScale(music_label, 1);
    menuSliderCreate(music_bar, 0x232E, 1);
    sceneObjSetPosition(music_bar, -80, 0, 360);
    sceneObjSetUseCamera(music_bar, 0);
    sub_80123E5C(music_bar, 0);
    sceneObjSetScale(music_bar, 1);
    sub_80137F60(sfx, 0x232A, 1, screenOptions);
    sceneObjSetPosition(sfx, -584, 0, 176);
    sceneObjSetUseCamera(sfx, 0);
    sub_80123E5C(sfx, 0);
    sceneObjSetScale(sfx, 1);
    sub_80123E98(sfx_label, 0x232D, 1);
    sceneObjSetPosition(sfx_label, -80, 0, 176);
    sceneObjSetUseCamera(sfx_label, 0);
    sub_80123E5C(sfx_label, 0);
    sceneObjSetScale(sfx_label, 1);
    menuSliderCreate(sfx_bar, 0x232E, 1);
    sceneObjSetPosition(sfx_bar, -80, 0, 176);
    sceneObjSetUseCamera(sfx_bar, 0);
    sub_80123E5C(sfx_bar, 0);
    sceneObjSetScale(sfx_bar, 1);
    menuSelectNextInit(sel_next, &shellScene);
    menuSelectPrevInit(sel_prev, &shellScene);
    sub_80123828(wait, &shellScene, 0, 2000);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    sub_80123A20(turn_a, music, 40, 0, 0, 15);
    sub_80123A20(turn_b, sfx, -40, 0, 0, 15);
    sub_80123B64(up_a, music, 0, 1, 0);
    sub_80123B64(down_a, music, 0, -1, 0);
    sub_80123B64(up_b, sfx, 0, 1, 0);
    sub_80123B64(down_b, sfx, 0, -1, 0);
    menuSliderIncInit(music_up, music_bar);
    menuSliderDecInit(music_down, music_bar);
    menuSliderIncInit(sfx_up, sfx_bar);
    menuSliderDecInit(sfx_down, sfx_bar);
    menuVolumeApplyInit(music_set, music_bar, 1);
    menuVolumeApplyInit(sfx_set, sfx_bar, 0);
    menuPlaySoundInit(snd_move, soundManagerObject, 28, 0, 0);
    menuPlaySoundInit(snd_ok, soundManagerObject, 29, 0, 0);
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, music);
    sceneAddDecor(&shellScene, music_label);
    sceneAddDecor(&shellScene, music_bar);
    sceneAddItem(&shellScene, sfx);
    sceneAddDecor(&shellScene, sfx_label);
    sceneAddDecor(&shellScene, sfx_bar);
    sceneAddAction(&shellScene, 7, 0, 2, snd_move);
    sceneAddAction(&shellScene, 7, 0, 2, sel_prev);
    sceneAddAction(&shellScene, 8, 0, 2, snd_move);
    sceneAddAction(&shellScene, 8, 0, 2, sel_next);
    sceneAddAction(&shellScene, 14, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 14, 0, 1, wait);
    sceneAddAction(&shellScene, 14, 0, 0, set2);
    sceneAddAction(&shellScene, 15, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 15, 0, 1, wait);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, snd_ok);
    sceneAddAction(&shellScene, 16, 0, 1, wait);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 0, 0, 0, turn_a);
    sceneAddAction(&shellScene, 0, 0, 0, turn_b);
    sceneAddAction(&shellScene, 2, turn_a, 2, snd_move);
    sceneAddAction(&shellScene, 2, turn_a, 2, up_a);
    sceneAddAction(&shellScene, 2, turn_a, 0, wait);
    sceneAddAction(&shellScene, 7, music, 2, down_a);
    sceneAddAction(&shellScene, 7, music, 2, up_b);
    sceneAddAction(&shellScene, 8, music, 2, down_a);
    sceneAddAction(&shellScene, 8, music, 2, up_b);
    sceneAddAction(&shellScene, 9, music, 2, music_down);
    sceneAddAction(&shellScene, 9, music, 2, music_set);
    sceneAddAction(&shellScene, 10, music, 2, music_up);
    sceneAddAction(&shellScene, 10, music, 2, music_set);
    sceneAddAction(&shellScene, 7, sfx, 2, down_b);
    sceneAddAction(&shellScene, 7, sfx, 2, up_a);
    sceneAddAction(&shellScene, 8, sfx, 2, down_b);
    sceneAddAction(&shellScene, 8, sfx, 2, up_a);
    sceneAddAction(&shellScene, 9, sfx, 2, sfx_down);
    sceneAddAction(&shellScene, 9, sfx, 2, sfx_set);
    sceneAddAction(&shellScene, 10, sfx, 2, sfx_up);
    sceneAddAction(&shellScene, 10, sfx, 2, sfx_set);
    sceneAddAction(&shellScene, 2, wait, 2, set2);
    sceneLoadTmsDmd(&shellScene, strShellAudioScene);
    screenImageRecordLoad(record);
    e = *(SS3VEntry **)(musicManagerObject + 8) + 2;
    e->fn(musicManagerObject + e->delta, 11);
    SS3SetSerialVolume(audioGetMusicVolume());
    menuSliderSetValue(music_bar, (u16)((audioCdVolume + 1) / 8));   /* retail narrows to 16 bits */
    menuSliderSetValue(sfx_bar, (u16)(audioSfxVolume / 7));
    result = sceneRun(&shellScene);
    e = *(SS3VEntry **)(musicManagerObject + 8) + 3;
    e->fn(musicManagerObject + e->delta, 11);
    SS3SetSerialVolume(0);
    return (void *)result;
}
