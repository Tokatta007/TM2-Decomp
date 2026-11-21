/* Car select screen (801307EC; loads SHELLDB1\CARSEL).
 *
 * Built like every shell screen (see screen_single.c): stack objects
 * registered with the scene shellScene, transition rows
 * (sceneAddAction(&shellScene, event, source, mode, action)), then the scene runs.
 * The car carousel is object 9005 ("carousel"); sceneObjGetChoiceState(carousel) is
 * the car under the cursor and carSelectStartCar the car it starts on.
 *
 * The scene runs once per human player still to choose
 * (settingsGetCarSelectPlayer(settings) .. settingsGetHumans(settings)): "select" stores the
 * car for that player and moves to the next one; "info" shows the
 * biography picture of the car (carBioPictureFiles[car]) and returns to the
 * carousel on the previous car. Events: 0 start, 2 action done, 3/14 back,
 * 7/8 up/down, 9/10 left/right, 11 (third carousel move), 15/16 other
 * buttons, 17/18 toggles. The next screen after "select" depends on the
 * game mode. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/scene.h"
#include "semantic/settings.h"
#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
#include "semantic/car_groups.h"
#include "semantic/dt_player.h"   /* request 36 */
#include "semantic/car_numbers.h" /* request 42 items 3 and 5: the extended carousel and labels, car numbers */
#endif
/* request 42 item 5: the car number of the carousel index the player picked (the same value; -1 for no car) */
#ifdef TM2_PORT_CAR_NUMBERS
#define TM2_CARSEL_NUMBER(index) portCarSelNumber(index)
#else
#define TM2_CARSEL_NUMBER(index) (index)
#endif

typedef struct { s32 v[12]; } TM2CameraTab;

extern struct TM2ShellScene shellScene;             /* the shell scene */

#pragma tm2_absolute soundManagerObject
extern s32 soundManagerObject;
extern s32 carSelectStartCar;              /* carousel start car */
extern u8 strEmpty[], unlockSweetTooth[], unlockMinion[];
extern TM2CameraTab carSelectCodeTable;     /* button codes (menu events): Sweet Tooth, Minion, dummy; see docs/UNLOCKS.md */
extern char strShellCarSelTim[];           /* "SHELLDB1\CARSEL.TIM" */
extern char strShellCarSelScene[];           /* "SHELLDB1\CARSEL" */
extern char *carBioPictureFiles[14];        /* car biography pictures */

extern void *screenTitle(void);    /* title screen */
extern void *shellAttractCycle(void *value);
extern void *screenSinglePlayerMenu(void);    /* single-player menu */
extern void *screenMultiPlayer(void);    /* multi-player menu */
extern void *screenLevelSelect(void);    /* level select */
extern void *screenAiSelect(void);    /* AI car select */
extern void *screenPickOpponents(void);
extern void *screenWorldGlobe(void);
extern void *menuRandomCarText(void);
extern s32 menuCarNameText(s32 index);
#include "semantic/car_calls.h"

extern void sceneInit(struct TM2ShellScene *scene, void *back, void *cancel, void *idle, s32);
extern u16 sceneSetCamera(struct TM2ShellScene *scene, s32 x, s32 y, s32 z, u16, u16, u16);
extern void lightSetEnvColor(s32, s32, s16 r, s16 g, s16 b);
extern void lightSetEnvDirection(s32, s32, s16 az, s16 el);
extern void lightSetEnvAmbient(s32, s16 r, s16 g, s16 b);
extern void *screenImageRecordInit(void *record, const char *name, s32 buffer);
extern void screenImageRecordLoad(void *record);
extern void *sub_80137F60(void *item, u16 id, u16 count, void *next);
extern void sceneObjSetPosition(void *object, s32 x, s32 y, s32 z);
extern void sceneObjSetRotation(void *object, s32 x, s32 y, s32 z);
extern void sceneObjSetUseCamera(void *object, u16 v);
extern void sub_80123E5C(void *object, u16 v);
extern void sceneObjSetScale(void *object, s32 v);
extern void sceneObjSetVisible(void *object, u16 v);
extern void *sub_80123E98(void *object, u16 first, u16 second);
extern void *menuTextInit(void *object, s32, void *value, signed char mode, u16 first, u16 second, u16 third);
extern void *menuSelectNextInit(void *object, void *owner);   /* action: select the next item */
extern void *menuSelectPrevInit(void *object, void *owner);   /* action: select the previous item */
extern void *sub_8012DB10(void *object, void *scene, s32, void *tab, s32);
extern void *sub_80123870(void *object, void *target, s32);
extern void *shellSetUnlockFlagInit(void *object, void *owner, void *value, s32 flag);
extern void *sub_80123A20(void *object, void *target, s32, s32, s32, s32);   /* TM2MoveBy */
extern void *sub_80123A74(void *object, void *target, s32, s32, s32, s32);   /* TM2TurnTo */
extern void *sub_801239CC(void *object, void *target, s32, s32, s32, s32);   /* TM2TurnBy */
extern void *sub_80123AC8(void *object, void *target, s32, s32, s32, s32);   /* TM2MoveTo */
extern void *sub_80123B64(void *object, void *target, s32, s32, void *);     /* TM2FourWay */
extern void *menuCopyChoiceInit(void *object, void *target, void *);
extern void *sub_80136FB8(void *object, void *, void *, void *text_callback);
extern void *menuHideTargetInit(void *object, void *target);
extern void *menuShowTargetInit(void *object, void *target);
extern void *menuPlaySoundInit(void *object, s32 actor, s32, s32, s32);
extern void sceneSetBackground(struct TM2ShellScene *scene, void *record);
extern s32 sceneAddItem(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddDecor(struct TM2ShellScene *scene, void *object);
extern s32 sceneAddAction(struct TM2ShellScene *scene, s32 event, void *source, s32 mode, void *action);
extern s32 sceneLoadTmsDmd(struct TM2ShellScene *scene, const char *name);
extern s32 sceneRun(struct TM2ShellScene *scene);
extern s32 sceneGetExitCode(struct TM2ShellScene *scene);
extern void *sceneGetSelectedItem2(struct TM2ShellScene *scene);
extern void sceneSetDone(struct TM2ShellScene *scene, s32);
extern void sceneSetSelection(struct TM2ShellScene *scene, s32);
extern void sceneClearRunning(struct TM2ShellScene *scene);
extern void sceneClearEvents(struct TM2ShellScene *scene);
extern void sceneObjSetChoiceState(void *object, s32 value);
extern s32 sceneObjGetChoiceState(void *object);
extern void codeDetectorSetPort(void *object, s32 flag);
extern void menuTextDestroy(void *object, s32);
extern s32 settingsGetCarSelectPlayer(struct TM2Settings *settings);
extern s32 settingsGetHumans(struct TM2Settings *settings);
extern s32 settingsGetMode(struct TM2Settings *settings);
extern void settingsSetHumanCar(struct TM2Settings *settings, s32 i, s32 v);
extern void settingsSetOpponentCar(struct TM2Settings *settings, s32 i, s32 v);
extern void settingsSetCarSelectPlayer(struct TM2Settings *settings, s32 v);
extern s32 screenSetEnv(void);
extern s32 screenLoadImageFile(const char *name, s32);
extern s32 screenDisplayImage(s32, s32, s32, s32);
extern s32 padWaitAnyButtonOrTimeout(s32, s32);
extern s32 padWaitConfirmOrTimeout(s32, s32);

/* The game-mode switch's jump table sits at its retail address. */
#pragma tm2_rdata .tm2_800D03BC

/* (u8)a < b. As an inline helper both calls are made before the byte mask
   (retail: move s0,v0 / jal / andi s0,s0,0xff); the plain expression masks
   the first result before the second call. */
static __inline__ s32 below(s32 a, s32 b) { return (u8)a < b; }

/* @801307EC */
void *__attribute__((section(".text_801307EC"))) screenCarSelect(void)
{
    TM2CameraTab camera_tab;
    u8 record[48];
    u8 select[56], info[56];
    u8 arrow_l[56], arrow_r[56], carousel[56];
    u8 label_a[56], label_b[56], label_c[56], label_d[56];
    u8 counter[72];
    u8 sel_next[16], sel_prev[16];
    u8 camera[64];
    u8 set0[24], set1[24], set2[24], toggle_a[24], toggle_b[24];
    u8 o920[32], o952[32];                                  /* TM2MoveBy select, info */
    u8 o984[32], o1016[32], o1048[32], o1080[32];          /* TM2FourWay select/info up, down */
    u8 o1112[32], o1144[32], o1176[32];                    /* TM2TurnBy arrows, carousel */
    u8 o1208[32], o1240[32], o1272[32];                    /* TM2MoveTo carousel */
    u8 o1304[32];                                          /* TM2TurnTo carousel */
    u8 o1336[32], o1368[32], o1400[32], o1432[32], o1464[32], o1496[32];   /* TM2MoveBy carousel */
    u8 o1528[32], o1560[32], o1592[32];                    /* TM2FourWay carousel */
    u8 o1624[24], o1648[24];
    u8 o1672[32], o1704[32];
    u8 o1736[16], o1752[16];
    u8 o1768[32], o1800[32];                               /* sounds 0x1C, 0x1D */
    void *(*cancel)(void);
    void *(*next)(void);
    s32 result, player, flag;

    cancel = 0;
    camera_tab = carSelectCodeTable;
    for (player = (u8)settingsGetCarSelectPlayer(&sessionSettings); player < settingsGetHumans(&sessionSettings); player++)
        settingsSetHumanCar(&sessionSettings, player, -1);
    for (player = 0; player < 10; player++)
        settingsSetOpponentCar(&sessionSettings, player, -1);
    switch (settingsGetMode(&sessionSettings)) {
    case 1:
        cancel = screenSinglePlayerMenu;
        break;
    case 3:
    case 4:
        cancel = screenLevelSelect;
        break;
    case 2:
        cancel = screenMultiPlayer;
        break;
    }
    sceneInit(&shellScene, screenTitle, cancel, shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -1200, 80, 170, 0, 0);
    lightSetEnvColor(0, 0, 200, 200, 200);
    lightSetEnvColor(0, 1, 100, 100, 100);
    lightSetEnvColor(0, 2, 0, 0, 0);
    lightSetEnvDirection(0, 0, 210, 60);
    lightSetEnvDirection(0, 1, 210, 60);
    lightSetEnvDirection(0, 2, 210, 60);
    lightSetEnvAmbient(0, 100, 100, 100);
    screenImageRecordInit(record, strShellCarSelTim, 0x80048270);
    switch (settingsGetMode(&sessionSettings)) {
    case 3:
        next = screenAiSelect;
        break;
    case 4:
        next = screenWorldGlobe;
        break;
    case 1:
    case 2:
        next = screenPickOpponents;
        break;
    case 0:
    default:
        next = screenTitle;
        break;
    }
    sub_80137F60(select, 9001, 1, next);
    sceneObjSetPosition(select, -600, 0, 720);
    sceneObjSetUseCamera(select, 0);
    sub_80123E5C(select, 4094);
    sceneObjSetScale(select, 1);
    sub_80137F60(info, 9002, 1, 0);
    sceneObjSetPosition(info, 600, 0, 536);
    sceneObjSetUseCamera(info, 0);
    sub_80123E5C(info, 4094);
    sceneObjSetScale(info, 1);
    sub_80123E98(arrow_l, 9004, 1);
    sceneObjSetPosition(arrow_l, -256, -640, -240);
    sceneObjSetRotation(arrow_l, 0, 170, 0);
    sub_80123E98(arrow_r, 9004, 1);
    sceneObjSetPosition(arrow_r, 256, -640, -240);
    sceneObjSetRotation(arrow_r, 0, 170, 2048);
    sub_80123E98(carousel, 9005, 1);
    sceneObjSetPosition(carousel, 0, -960, -40);
    sub_80123E98(label_a, 9006, 1);
    sceneObjSetPosition(label_a, -800, 0, 408);
    sceneObjSetUseCamera(label_a, 0);
    sub_80123E5C(label_a, 4094);
    sceneObjSetScale(label_a, 1);
    sub_80123E98(label_b, 9007, 0);
    sceneObjSetPosition(label_b, -800, 0, 408);
    sceneObjSetUseCamera(label_b, 0);
    sub_80123E5C(label_b, 4094);
    sceneObjSetScale(label_b, 1);
    sub_80123E98(label_c, 9008, 1);
    sceneObjSetPosition(label_c, 960, 0, 568);
    sceneObjSetUseCamera(label_c, 0);
    sub_80123E5C(label_c, 4094);
    sceneObjSetScale(label_c, 1);
    sub_80123E98(label_d, 9009, 0);
    sceneObjSetPosition(label_d, -960, 0, 568);
    sceneObjSetUseCamera(label_d, 0);
    sub_80123E5C(label_d, 4094);
    sceneObjSetScale(label_d, 1);
    menuTextInit(counter, 2, strEmpty, 1, 160, 205, 1);
    menuSelectNextInit(sel_next, &shellScene);
    menuSelectPrevInit(sel_prev, &shellScene);
    sub_8012DB10(camera, &shellScene, 0, &camera_tab, 2000);
    sub_80123870(set0, &shellScene, 0);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    shellSetUnlockFlagInit(toggle_a, &shellScene, unlockSweetTooth, 1);
    shellSetUnlockFlagInit(toggle_b, &shellScene, unlockMinion, 1);
    sub_80123A20(o920, select, 40, 0, 0, 15);
    sub_80123A20(o952, info, -40, 0, 0, 15);
    sub_80123B64(o984, select, 0, 1, 0);
    sub_80123B64(o1016, select, 0, -1, 0);
    sub_80123B64(o1048, info, 0, 1, 0);
    sub_80123B64(o1080, info, 0, -1, 0);
    sub_801239CC(o1112, arrow_l, 0, 256, 0, 16);
    sub_801239CC(o1144, arrow_r, 0, 256, 0, 16);
    sub_801239CC(o1176, carousel, 20, 0, 0, -1);
    sub_80123AC8(o1208, carousel, -320, -960, -40, -1);
    sub_80123AC8(o1240, carousel, 320, -960, -40, -1);
    sub_80123AC8(o1272, carousel, 0, -640, -120, -1);
    sub_80123A74(o1304, carousel, 2048, 0, 0, -1);
    sub_80123A20(o1336, carousel, -32, 0, 0, 10);
    sub_80123A20(o1368, carousel, -32, 0, 0, 10);
    sub_80123A20(o1400, carousel, 32, 0, 0, 10);
    sub_80123A20(o1432, carousel, 32, 0, 0, 10);
    sub_80123A20(o1464, carousel, 0, -32, 8, 10);
    sub_80123A20(o1496, carousel, 0, 0, -320, 10);
    sub_80123B64(o1528, carousel, 0, 1, carIsSelectable);
    sub_80123B64(o1560, carousel, 0, -1, carIsSelectable);
    sub_80123B64(o1592, carousel, 2, 0, carIsSelectable);
    menuCopyChoiceInit(o1624, label_a, carousel);
    menuCopyChoiceInit(o1648, label_b, carousel);
    sub_80136FB8(o1672, counter, carousel, menuCarNameText);
    sub_80136FB8(o1704, counter, carousel, menuRandomCarText);
    menuHideTargetInit(o1736, label_a);
    menuShowTargetInit(o1752, label_b);
    menuPlaySoundInit(o1768, soundManagerObject, 28, 0, 0);
    menuPlaySoundInit(o1800, soundManagerObject, 29, 0, 0);
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, select);
    sceneAddItem(&shellScene, info);
    sceneAddDecor(&shellScene, arrow_l);
    sceneAddDecor(&shellScene, arrow_r);
    sceneAddDecor(&shellScene, carousel);
    sceneAddDecor(&shellScene, label_a);
    sceneAddDecor(&shellScene, label_b);
    sceneAddDecor(&shellScene, counter);
    sceneAddDecor(&shellScene, label_c);
    sceneAddDecor(&shellScene, label_d);
    sceneAddAction(&shellScene, 7, 0, 2, o1768);
    sceneAddAction(&shellScene, 7, 0, 2, sel_prev);
    sceneAddAction(&shellScene, 8, 0, 2, o1768);
    sceneAddAction(&shellScene, 8, 0, 2, sel_next);
    sceneAddAction(&shellScene, 3, 0, 2, o1800);
    sceneAddAction(&shellScene, 3, 0, 1, camera);
    sceneAddAction(&shellScene, 3, 0, 0, set0);
    sceneAddAction(&shellScene, 14, 0, 2, o1800);
    sceneAddAction(&shellScene, 14, 0, 1, camera);
    sceneAddAction(&shellScene, 14, 0, 0, set0);
    sceneAddAction(&shellScene, 17, 0, 2, o1800);
    sceneAddAction(&shellScene, 17, 0, 2, toggle_a);
    sceneAddAction(&shellScene, 18, 0, 2, o1800);
    sceneAddAction(&shellScene, 18, 0, 2, toggle_b);
    sceneAddAction(&shellScene, 15, 0, 2, o1800);
    sceneAddAction(&shellScene, 15, 0, 1, camera);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, o1800);
    sceneAddAction(&shellScene, 16, 0, 1, camera);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 0, 0, 0, o920);
    sceneAddAction(&shellScene, 0, 0, 0, o952);
    sceneAddAction(&shellScene, 0, 0, 2, o1528);
    sceneAddAction(&shellScene, 0, 0, 2, o1624);
    sceneAddAction(&shellScene, 0, 0, 2, o1648);
    sceneAddAction(&shellScene, 0, 0, 2, o1672);
    sceneAddAction(&shellScene, 2, o920, 2, o1768);
    sceneAddAction(&shellScene, 2, o920, 2, o984);
    sceneAddAction(&shellScene, 2, o920, 0, o1176);
    sceneAddAction(&shellScene, 2, o920, 0, camera);
    sceneAddAction(&shellScene, 7, select, 2, o1016);
    sceneAddAction(&shellScene, 7, select, 2, o1048);
    sceneAddAction(&shellScene, 8, select, 2, o1016);
    sceneAddAction(&shellScene, 8, select, 2, o1048);
    sceneAddAction(&shellScene, 3, select, 2, o1736);
    sceneAddAction(&shellScene, 3, select, 2, o1752);
    sceneAddAction(&shellScene, 14, select, 2, o1736);
    sceneAddAction(&shellScene, 14, select, 2, o1752);
    sceneAddAction(&shellScene, 7, info, 2, o1080);
    sceneAddAction(&shellScene, 7, info, 2, o984);
    sceneAddAction(&shellScene, 8, info, 2, o1080);
    sceneAddAction(&shellScene, 8, info, 2, o984);
    sceneAddAction(&shellScene, 9, 0, 2, o1768);
    sceneAddAction(&shellScene, 9, 0, 1, camera);
    sceneAddAction(&shellScene, 9, 0, 1, o1176);
    sceneAddAction(&shellScene, 9, 0, 0, o1112);
    sceneAddAction(&shellScene, 9, 0, 0, o1432);
    sceneAddAction(&shellScene, 2, o1432, 2, o1560);
    sceneAddAction(&shellScene, 2, o1432, 2, o1624);
    sceneAddAction(&shellScene, 2, o1432, 2, o1648);
    sceneAddAction(&shellScene, 2, o1432, 2, o1672);
    sceneAddAction(&shellScene, 2, o1432, 2, o1208);
    sceneAddAction(&shellScene, 2, o1432, 0, o1400);
    sceneAddAction(&shellScene, 2, o1400, 0, o1176);
    sceneAddAction(&shellScene, 2, o1400, 0, camera);
    sceneAddAction(&shellScene, 10, 0, 2, o1768);
    sceneAddAction(&shellScene, 10, 0, 1, camera);
    sceneAddAction(&shellScene, 10, 0, 1, o1176);
    sceneAddAction(&shellScene, 10, 0, 0, o1144);
    sceneAddAction(&shellScene, 10, 0, 0, o1368);
    sceneAddAction(&shellScene, 2, o1368, 2, o1528);
    sceneAddAction(&shellScene, 2, o1368, 2, o1624);
    sceneAddAction(&shellScene, 2, o1368, 2, o1648);
    sceneAddAction(&shellScene, 2, o1368, 2, o1672);
    sceneAddAction(&shellScene, 2, o1368, 2, o1240);
    sceneAddAction(&shellScene, 2, o1368, 0, o1336);
    sceneAddAction(&shellScene, 2, o1336, 0, o1176);
    sceneAddAction(&shellScene, 2, o1336, 0, camera);
    sceneAddAction(&shellScene, 11, 0, 2, o1704);
    sceneAddAction(&shellScene, 11, 0, 2, o1768);
    sceneAddAction(&shellScene, 11, 0, 1, camera);
    sceneAddAction(&shellScene, 11, 0, 1, o1176);
    sceneAddAction(&shellScene, 11, 0, 0, o1112);
    sceneAddAction(&shellScene, 11, 0, 0, o1144);
    sceneAddAction(&shellScene, 11, 0, 0, o1496);
    sceneAddAction(&shellScene, 2, o1496, 2, o1592);
    sceneAddAction(&shellScene, 2, o1496, 2, o1624);
    sceneAddAction(&shellScene, 2, o1496, 2, o1648);
    sceneAddAction(&shellScene, 2, o1496, 2, o1272);
    sceneAddAction(&shellScene, 2, o1496, 2, o1304);
    sceneAddAction(&shellScene, 2, o1496, 0, o1464);
    sceneAddAction(&shellScene, 2, o1464, 0, o1176);
    sceneAddAction(&shellScene, 2, o1464, 0, camera);
    sceneAddAction(&shellScene, 2, o1464, 2, o1672);
    sceneAddAction(&shellScene, 2, camera, 0, set1);
    sceneLoadTmsDmd(&shellScene, strShellCarSelScene);
#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
    /* request 36: with portCarOptions bit 8 a 15-child carousel (index 14); request 42: the name labels extended
       with it (retail's 14-child labels walked garbage for index 14), and 0x20 + portCarSel.mods children for mods */
    portCarSelExtend(carousel, label_a, label_b);
#endif
    screenImageRecordLoad(record);

    while (below(settingsGetCarSelectPlayer(&sessionSettings), settingsGetHumans(&sessionSettings))) {
        flag = 0;
        if (settingsGetMode(&sessionSettings) == 4 || settingsGetMode(&sessionSettings) == 2) {
            if (settingsGetCarSelectPlayer(&sessionSettings) & 1)
                flag = 1;
        }
        codeDetectorSetPort(camera, flag);
        sceneObjSetChoiceState(carousel, carSelectStartCar);
        result = sceneRun(&shellScene);
        if (!sceneGetExitCode(&shellScene)) {
        if (sceneGetSelectedItem2(&shellScene) == select) {
            flag = (u8)settingsGetCarSelectPlayer(&sessionSettings);
            settingsSetHumanCar(&sessionSettings, flag, TM2_CARSEL_NUMBER(sceneObjGetChoiceState(carousel)));
            settingsSetCarSelectPlayer(&sessionSettings, (u8)settingsGetCarSelectPlayer(&sessionSettings) + 1);
            sceneObjSetPosition(label_a, 800, 0, 408);
            sceneObjSetPosition(label_b, 800, 0, 408);
            sceneObjSetVisible(label_c, 0);
            sceneObjSetVisible(label_d, 1);
        }
        if (sceneGetSelectedItem2(&shellScene) == info) {
#if defined(TM2_PORT_DT_PLAYER) || defined(TM2_PORT_CAR_NUMBERS)
            /* request 36 (DARK_TOOTH_PLAYER.md): the carousel's own count (15 when extended); index 14's
               picture is the port's (portDtPlayer.bio_picture; none: back at once); request 42: a mod's too
               (portModCars[m].bio_picture) */
            carSelectStartCar = (sceneObjGetChoiceState(carousel) + portCarSelCount() - 1) % portCarSelCount();
            screenSetEnv();
            if (portCarBioFile(sceneObjGetChoiceState(carousel)) != 0) {
                screenLoadImageFile(portCarBioFile(sceneObjGetChoiceState(carousel)), 0);
                screenDisplayImage(1, 0x80048270, 0, 0);
                padWaitAnyButtonOrTimeout(0, 1800);
            }
#else
            carSelectStartCar = (sceneObjGetChoiceState(carousel) + 13) % 14;
            screenSetEnv();
            screenLoadImageFile(carBioPictureFiles[sceneObjGetChoiceState(carousel)], 0);
            screenDisplayImage(1, 0x80048270, 0, 0);
            padWaitAnyButtonOrTimeout(0, 1800);
#endif
            screenImageRecordLoad(record);
        }
        sceneObjSetPosition(select, -600, 0, 720);
        sceneObjSetPosition(info, 600, 0, 536);
        sceneObjSetChoiceState(select, 0);
        sceneObjSetChoiceState(info, 0);
        sceneObjSetVisible(label_a, 1);
        sceneObjSetVisible(label_b, 0);
        sceneSetDone(&shellScene, 0);
        sceneSetSelection(&shellScene, 0);
        sceneClearRunning(&shellScene);
        sceneClearEvents(&shellScene);
        } else if (sceneGetExitCode(&shellScene) == 2)
            break;
        else
            goto done;
        padWaitConfirmOrTimeout(30, 30);
    }
    settingsSetCarSelectPlayer(&sessionSettings, 0);
done:
    menuTextDestroy(counter, 2);
    return (void *)result;
}
