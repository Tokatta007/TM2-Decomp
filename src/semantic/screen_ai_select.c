/* AI opponent select screen (80131EEC; loads SHELLDB1\AICARSEL), the
 * same scene as the car select (screen_car_select.c) with a third item.
 * The scene runs once per enemy slot still free
 * (settingsGetOpponentsPicked(settings) .. settingsGetOpponents(settings)): "select" adds the
 * car under the cursor as the next enemy, "info" shows its biography
 * picture until a button is pressed, "done" ends the choice (it refuses
 * with no enemy chosen). Twelve cars can be enemies. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/settings.h"
#include "semantic/scene.h"
/* Request 42 item 2 (portCarOptions bit 11, car_numbers.h): Sweet Tooth in the picker while he is unlocked. The bio
   wrap goes over the carousel's own count (13 when extended); retail: 12. */
#ifdef TM2_PORT_CAR_NUMBERS
#define TM2_AI_SEL_PREV(choice) ((choice) + portAiSelCount() - 1) % portAiSelCount()
#else
#define TM2_AI_SEL_PREV(choice) ((choice) + 11) % 12
#endif

extern struct TM2ShellScene shellScene;     /* the shell scene */

#pragma tm2_absolute soundManagerObject
extern s32 soundManagerObject;
extern u8 strEmpty[], unlockSweetTooth[], unlockMinion[];
extern char strShellCarSelTim[];           /* "SHELLDB1\CARSEL.TIM" */
extern char *carBioPictureFiles[14];        /* car biography pictures */

extern void *screenTitle(void);    /* title screen */
extern void *shellAttractCycle(void *value);
extern void *screenSinglePlayerMenu(void);    /* single-player menu */
extern void *screenMultiPlayer(void);    /* multi-player menu */
extern void *screenLevelSelect(void);    /* level select */
extern void *screenCarSelect(void);    /* car select */
extern void *screenPickOpponents(void);
extern void *screenWorldGlobe(void);    /* level select globe */
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

extern s32 aiSelectStartCar;              /* carousel start car */
extern char strShellAiCarSelScene[];           /* "SHELLDB1\AICARSEL" */
extern char strNoEnemiesSelected[];           /* "no enemies selected" */
extern void *sub_80123828(void *object, void *target, s32, s32);
extern s32 settingsGetOpponentsPicked(struct TM2Settings *settings);
extern s32 settingsGetOpponents(struct TM2Settings *settings);
extern void settingsSetOpponentsPicked(struct TM2Settings *settings, s32 v);
extern void settingsSetOpponents(struct TM2Settings *settings, s32 v);
extern void padUpdate(void);
extern u8 padAnyPressed(short int port);
extern s32 screenDisplayToDisplay(s32, s32);
extern int fontPrintCenteredXY(s32, s32, s32, char *);

/* (u8)a < b. As an inline helper both calls are made before the byte mask
   (retail: move s0,v0 / jal / andi s0,s0,0xff); the plain expression masks
   the first result before the second call. */
static __inline__ s32 below(s32 a, s32 b) { return (u8)a < b; }

/* @80131EEC */
void *__attribute__((section(".text_80131EEC"))) screenAiSelect(void)
{
    u8 record[48];
    u8 select[56], info[56], done[56];
    u8 arrow_l[56], arrow_r[56], carousel[56];
    u8 label_a[56], label_b[56];
    u8 counter[72];
    u8 sel_next[16], sel_prev[16];
    u8 wait[24], set0[24], set1[24], set2[24];
    u8 o728[32], o760[32], o792[32];                       /* TM2MoveBy select, info, done */
    u8 o824[32], o856[32], o888[32], o920[32], o952[32], o984[32];   /* TM2FourWay items up, down */
    u8 o1016[32], o1048[32], o1080[32];                    /* TM2TurnBy arrows, carousel */
    u8 o1112[32], o1144[32], o1176[32];                    /* TM2MoveTo carousel */
    u8 o1208[32];                                          /* TM2TurnTo carousel */
    u8 o1240[32], o1272[32], o1304[32], o1336[32], o1368[32], o1400[32];   /* TM2MoveBy carousel */
    u8 o1432[32], o1464[32], o1496[32];                    /* TM2FourWay carousel */
    u8 o1528[24], o1552[24];
    u8 o1576[32], o1608[32];
    u8 o1640[16], o1656[16];
    u8 o1672[32], o1704[32];                               /* sounds 0x1C, 0x1D */
    s32 result, slot;

    for (slot = (u8)settingsGetOpponentsPicked(&sessionSettings); slot < TM2_MAX_AI_CARS; slot++)
        settingsSetOpponentCar(&sessionSettings, slot, -1);
    sceneInit(&shellScene, screenTitle, screenCarSelect, shellAttractCycle, 0);
    sceneSetCamera(&shellScene, 0, -1200, 80, 170, 0, 0);
    lightSetEnvColor(0, 0, 200, 200, 200);
    lightSetEnvColor(0, 1, 100, 100, 100);
    lightSetEnvColor(0, 2, 0, 0, 0);
    lightSetEnvDirection(0, 0, 210, 60);
    lightSetEnvDirection(0, 1, 210, 60);
    lightSetEnvDirection(0, 2, 210, 60);
    lightSetEnvAmbient(0, 100, 100, 100);
    screenImageRecordInit(record, strShellCarSelTim, 0x80048270);
    sub_80137F60(select, 9001, 1, screenWorldGlobe);
    sceneObjSetPosition(select, -600, 0, 720);
    sceneObjSetUseCamera(select, 0);
    sub_80123E5C(select, 4094);
    sceneObjSetScale(select, 1);
    sub_80137F60(info, 9002, 1, 0);
    sceneObjSetPosition(info, 600, 0, 536);
    sceneObjSetUseCamera(info, 0);
    sub_80123E5C(info, 4094);
    sceneObjSetScale(info, 1);
    sub_80137F60(done, 9003, 1, screenWorldGlobe);
    sceneObjSetPosition(done, -600, 0, 352);
    sceneObjSetUseCamera(done, 0);
    sub_80123E5C(done, 4094);
    sceneObjSetScale(done, 1);
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
    menuTextInit(counter, 2, strEmpty, 1, 160, 205, 1);
    menuSelectNextInit(sel_next, &shellScene);
    menuSelectPrevInit(sel_prev, &shellScene);
    sub_80123828(wait, &shellScene, 0, 2000);
    sub_80123870(set0, &shellScene, 0);
    sub_80123870(set1, &shellScene, 1);
    sub_80123870(set2, &shellScene, 2);
    sub_80123A20(o728, select, 40, 0, 0, 15);
    sub_80123A20(o760, info, -40, 0, 0, 15);
    sub_80123A20(o792, done, 40, 0, 0, 15);
    sub_80123B64(o824, select, 0, 1, 0);
    sub_80123B64(o856, select, 0, -1, 0);
    sub_80123B64(o888, info, 0, 1, 0);
    sub_80123B64(o920, info, 0, -1, 0);
    sub_80123B64(o952, done, 0, 1, 0);
    sub_80123B64(o984, done, 0, -1, 0);
    sub_801239CC(o1016, arrow_l, 0, 256, 0, 16);
    sub_801239CC(o1048, arrow_r, 0, 256, 0, 16);
    sub_801239CC(o1080, carousel, 20, 0, 0, -1);
    sub_80123AC8(o1112, carousel, -320, -960, -40, -1);
    sub_80123AC8(o1144, carousel, 320, -960, -40, -1);
    sub_80123AC8(o1176, carousel, 0, -640, -120, -1);
    sub_80123A74(o1208, carousel, 2048, 0, 0, -1);
    sub_80123A20(o1240, carousel, -32, 0, 0, 10);
    sub_80123A20(o1272, carousel, -32, 0, 0, 10);
    sub_80123A20(o1304, carousel, 32, 0, 0, 10);
    sub_80123A20(o1336, carousel, 32, 0, 0, 10);
    sub_80123A20(o1368, carousel, 0, -32, 8, 10);
    sub_80123A20(o1400, carousel, 0, 0, -320, 10);
    sub_80123B64(o1432, carousel, 0, 1, carIsSelectable);
    sub_80123B64(o1464, carousel, 0, -1, carIsSelectable);
    sub_80123B64(o1496, carousel, 2, 0, carIsSelectable);
    menuCopyChoiceInit(o1528, label_a, carousel);
    menuCopyChoiceInit(o1552, label_b, carousel);
    sub_80136FB8(o1576, counter, carousel, menuCarNameText);
    sub_80136FB8(o1608, counter, carousel, menuRandomCarText);
    menuHideTargetInit(o1640, label_a);
    menuShowTargetInit(o1656, label_b);
    menuPlaySoundInit(o1672, soundManagerObject, 28, 0, 0);
    menuPlaySoundInit(o1704, soundManagerObject, 29, 0, 0);
    sceneSetBackground(&shellScene, record);
    sceneAddItem(&shellScene, select);
    sceneAddItem(&shellScene, info);
    sceneAddItem(&shellScene, done);
    sceneAddDecor(&shellScene, arrow_l);
    sceneAddDecor(&shellScene, arrow_r);
    sceneAddDecor(&shellScene, carousel);
    sceneAddDecor(&shellScene, label_a);
    sceneAddDecor(&shellScene, label_b);
    sceneAddDecor(&shellScene, counter);
    sceneAddAction(&shellScene, 7, 0, 2, o1672);
    sceneAddAction(&shellScene, 7, 0, 2, sel_prev);
    sceneAddAction(&shellScene, 8, 0, 2, o1672);
    sceneAddAction(&shellScene, 8, 0, 2, sel_next);
    sceneAddAction(&shellScene, 3, 0, 2, o1704);
    sceneAddAction(&shellScene, 3, 0, 1, wait);
    sceneAddAction(&shellScene, 3, 0, 0, set0);
    sceneAddAction(&shellScene, 14, 0, 2, o1704);
    sceneAddAction(&shellScene, 14, 0, 1, wait);
    sceneAddAction(&shellScene, 14, 0, 0, set0);
    sceneAddAction(&shellScene, 15, 0, 2, o1704);
    sceneAddAction(&shellScene, 15, 0, 1, wait);
    sceneAddAction(&shellScene, 15, 0, 0, set1);
    sceneAddAction(&shellScene, 16, 0, 2, o1704);
    sceneAddAction(&shellScene, 16, 0, 1, wait);
    sceneAddAction(&shellScene, 16, 0, 0, set2);
    sceneAddAction(&shellScene, 0, 0, 0, o728);
    sceneAddAction(&shellScene, 0, 0, 0, o760);
    sceneAddAction(&shellScene, 0, 0, 0, o792);
    sceneAddAction(&shellScene, 0, 0, 2, o1432);
    sceneAddAction(&shellScene, 0, 0, 2, o1528);
    sceneAddAction(&shellScene, 0, 0, 2, o1552);
    sceneAddAction(&shellScene, 0, 0, 2, o1576);
    sceneAddAction(&shellScene, 2, o728, 2, o1672);
    sceneAddAction(&shellScene, 2, o728, 2, o824);
    sceneAddAction(&shellScene, 2, o728, 0, o1080);
    sceneAddAction(&shellScene, 2, o728, 0, wait);
    sceneAddAction(&shellScene, 7, select, 2, o856);
    sceneAddAction(&shellScene, 7, select, 2, o952);
    sceneAddAction(&shellScene, 8, select, 2, o856);
    sceneAddAction(&shellScene, 8, select, 2, o888);
    sceneAddAction(&shellScene, 3, select, 2, o1640);
    sceneAddAction(&shellScene, 3, select, 2, o1656);
    sceneAddAction(&shellScene, 14, select, 2, o1640);
    sceneAddAction(&shellScene, 14, select, 2, o1656);
    sceneAddAction(&shellScene, 7, info, 2, o920);
    sceneAddAction(&shellScene, 7, info, 2, o824);
    sceneAddAction(&shellScene, 8, info, 2, o920);
    sceneAddAction(&shellScene, 8, info, 2, o952);
    sceneAddAction(&shellScene, 7, done, 2, o984);
    sceneAddAction(&shellScene, 7, done, 2, o888);
    sceneAddAction(&shellScene, 8, done, 2, o984);
    sceneAddAction(&shellScene, 8, done, 2, o824);
    sceneAddAction(&shellScene, 9, 0, 2, o1672);
    sceneAddAction(&shellScene, 9, 0, 1, wait);
    sceneAddAction(&shellScene, 9, 0, 1, o1080);
    sceneAddAction(&shellScene, 9, 0, 0, o1016);
    sceneAddAction(&shellScene, 9, 0, 0, o1336);
    sceneAddAction(&shellScene, 2, o1336, 2, o1464);
    sceneAddAction(&shellScene, 2, o1336, 2, o1528);
    sceneAddAction(&shellScene, 2, o1336, 2, o1552);
    sceneAddAction(&shellScene, 2, o1336, 2, o1576);
    sceneAddAction(&shellScene, 2, o1336, 2, o1112);
    sceneAddAction(&shellScene, 2, o1336, 0, o1304);
    sceneAddAction(&shellScene, 2, o1304, 0, o1080);
    sceneAddAction(&shellScene, 2, o1304, 0, wait);
    sceneAddAction(&shellScene, 10, 0, 2, o1672);
    sceneAddAction(&shellScene, 10, 0, 1, wait);
    sceneAddAction(&shellScene, 10, 0, 1, o1080);
    sceneAddAction(&shellScene, 10, 0, 0, o1048);
    sceneAddAction(&shellScene, 10, 0, 0, o1272);
    sceneAddAction(&shellScene, 2, o1272, 2, o1432);
    sceneAddAction(&shellScene, 2, o1272, 2, o1528);
    sceneAddAction(&shellScene, 2, o1272, 2, o1552);
    sceneAddAction(&shellScene, 2, o1272, 2, o1576);
    sceneAddAction(&shellScene, 2, o1272, 2, o1144);
    sceneAddAction(&shellScene, 2, o1272, 0, o1240);
    sceneAddAction(&shellScene, 2, o1240, 0, o1080);
    sceneAddAction(&shellScene, 2, o1240, 0, wait);
    sceneAddAction(&shellScene, 11, 0, 2, o1608);
    sceneAddAction(&shellScene, 11, 0, 2, o1672);
    sceneAddAction(&shellScene, 11, 0, 1, wait);
    sceneAddAction(&shellScene, 11, 0, 1, o1080);
    sceneAddAction(&shellScene, 11, 0, 0, o1016);
    sceneAddAction(&shellScene, 11, 0, 0, o1048);
    sceneAddAction(&shellScene, 11, 0, 0, o1400);
    sceneAddAction(&shellScene, 2, o1400, 2, o1496);
    sceneAddAction(&shellScene, 2, o1400, 2, o1528);
    sceneAddAction(&shellScene, 2, o1400, 2, o1552);
    sceneAddAction(&shellScene, 2, o1400, 2, o1176);
    sceneAddAction(&shellScene, 2, o1400, 2, o1208);
    sceneAddAction(&shellScene, 2, o1400, 0, o1368);
    sceneAddAction(&shellScene, 2, o1368, 0, o1080);
    sceneAddAction(&shellScene, 2, o1368, 0, wait);
    sceneAddAction(&shellScene, 2, o1400, 2, o1576);
    sceneAddAction(&shellScene, 2, wait, 0, set1);
    sceneLoadTmsDmd(&shellScene, strShellAiCarSelScene);
#ifdef TM2_PORT_CAR_NUMBERS
    portAiSelExtend(carousel, label_a, label_b);   /* request 42 item 2: Sweet Tooth (bit 11, unlocked) */
#endif
    screenImageRecordLoad(record);

    while (below(settingsGetOpponentsPicked(&sessionSettings), settingsGetOpponents(&sessionSettings))) {
        sceneObjSetChoiceState(carousel, aiSelectStartCar);
        result = sceneRun(&shellScene);
        if (sceneGetExitCode(&shellScene))
            break;
        if (sceneGetSelectedItem2(&shellScene) == select) {
            slot = (u8)settingsGetOpponentsPicked(&sessionSettings);
            settingsSetOpponentCar(&sessionSettings, slot, sceneObjGetChoiceState(carousel));
            settingsSetOpponentsPicked(&sessionSettings, (u8)settingsGetOpponentsPicked(&sessionSettings) + 1);
        } else if (sceneGetSelectedItem2(&shellScene) == info) {
            aiSelectStartCar = TM2_AI_SEL_PREV(sceneObjGetChoiceState(carousel));
            screenSetEnv();
            screenDisplayToDisplay(0, 1);
            screenLoadImageFile(carBioPictureFiles[sceneObjGetChoiceState(carousel)], 0);
            screenDisplayImage(1, 0x80048270, 0, 0);
            do
                padUpdate();
            while (!padAnyPressed(0));
            screenImageRecordLoad(record);
        } else if (sceneGetSelectedItem2(&shellScene) == done) {
            if (!(u8)settingsGetOpponentsPicked(&sessionSettings)) {
                screenSetEnv();
                screenDisplayToDisplay(0, 1);
                fontPrintCenteredXY(2, 159, 149, strNoEnemiesSelected);
                padWaitConfirmOrTimeout(30, 120);
            } else
                settingsSetOpponents(&sessionSettings, (u8)settingsGetOpponentsPicked(&sessionSettings));
        }
        sceneObjSetPosition(select, -600, 0, 720);
        sceneObjSetPosition(info, 600, 0, 536);
        sceneObjSetPosition(done, -600, 0, 352);
        sceneObjSetChoiceState(select, 0);
        sceneObjSetChoiceState(info, 0);
        sceneObjSetChoiceState(done, 0);
        sceneObjSetVisible(label_a, 1);
        sceneObjSetVisible(label_b, 0);
        sceneSetDone(&shellScene, 0);
        sceneSetSelection(&shellScene, 0);
        sceneClearRunning(&shellScene);
        sceneClearEvents(&shellScene);
        padWaitConfirmOrTimeout(30, 30);
    }
    settingsSetOpponentsPicked(&sessionSettings, 0);
    menuTextDestroy(counter, 2);
    return (void *)result;
}
