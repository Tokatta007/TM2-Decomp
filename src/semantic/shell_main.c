/* Game code (main and shell movie/session helpers). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/settings.h"

typedef struct {
    s16 x, y, w, h;
} TM2Rect;

typedef struct {
    TM2Rect disp;
    TM2Rect screen;
    u8 isinter;
    u8 isrgb24;
    u8 pad0;
    u8 pad1;
} TM2DispEnv;

typedef struct {
    u8 head[24];
    u8 isbg;
    u8 tail[92 - 25];
} TM2DrawEnv;

typedef void (*TM2Method)(u8 *, ...);

/* Shell object (main's stack object, soundManagerObject): vtable at +200. */
typedef struct {
    u8 pad_000[200];
    u8 *vt;
    u8 members[67][20];
    u8 extra[7][20];
    u8 pad_694[4];
} TM2ShellObj;

typedef struct {
    u8 data[16];
} TM2SoundObj;

typedef struct {
    u8 data[120];
} TM2Slot120;

extern u8 *soundManagerObject;
extern u8 *musicManagerObject;
extern s32 audioCdVolume, audioSfxVolume, audioSfxVolumeCap, audioVolumeClampMax, audioVolumeClampMin;
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioSfxVolume
#pragma tm2_absolute audioSfxVolumeCap
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern char strYouWin[];  /* "YOU WIN" */
extern char strLoading[];  /* "LOADING" */
extern char strYouLose[];  /* "YOU LOSE" */
extern char strWinsLossesWide[];  /* "WINS   LOSSES" */
extern char strWinsLossesFmt[];  /* "%d    %d" */
extern char textFormatBuffer[];
extern char strScreenSceaPresTim[], strScreenSisaProdTim[], strScreenDevelopTim[], strMovieSlogo15s[];
extern u8 shellObjBaseVtbl[], soundMgrVtbl[];
extern TM2DispEnv movieDispEnvs[];
extern TM2DrawEnv movieDrawEnvs[];
extern TM2Slot120 *sndObjPool[];


extern void SsSetSerialVol(char s_num, short voll, short volr);
extern void screenLostALife(void);
#include "semantic/hud.h"
extern int sprintf(char *, const char *, ...);
#include "semantic/world.h"
#include "semantic/input.h"
extern int VSync(int);
extern int DrawSync(int);
#include "semantic/scene.h"
extern void *SetDefDispEnv(TM2DispEnv *, s32, s32, s32, s32);
extern void *SetDefDrawEnv(TM2DrawEnv *, s32, s32, s32, s32);
extern void *PutDispEnv(TM2DispEnv *);
extern void *PutDrawEnv(TM2DrawEnv *);
extern int MoveImage(TM2Rect *, s32, s32);
extern s32 moviePlay(char *, void *, u8, u8, s32, s32, s32, s32, s32, s32, s32,
                            TM2DispEnv *, u8 *);
extern s32 bitmapInit(void);
extern void bitmapDisableTracking(void);
extern char *settingsLevelTmsFileName(struct TM2Settings *);
extern char *settingsLevelDmdFileName(struct TM2Settings *);
/* Retail passes the car index converted to a signed char ((s8)(u8)human_car[i]:
 * lbu, sll 24, sra 24) although the callees take a 32-bit index and never narrow
 * it: SingleTrac's prototype here must have said `char`. The calls below keep that
 * conversion as an explicit cast. */
extern char *carTmsFileName(struct TM2Settings *settings, s32 car);
extern char *carDmdFileName(struct TM2Settings *settings, s32 car);
extern s32 fileioLoadFileIntoRam(u8 *, char *);
#include "semantic/port_cars.h"
#ifdef TM2_PORT_CAR_CLUTS
#include "semantic/car_cluts.h"   /* request 33-1 */
#endif
#include "semantic/car_dropin.h"  /* request 33: portCarLoadDone */
#include "semantic/port_rollback.h"   /* request 48: SEED (the load's seed) */
/* The car handed to portCarLoadGroup (TM2_PORT_CARS): retail's signed byte; request 42 item 3: the car number
   (car_numbers.h: portCarLoadGroup keeps it per group and loads a mod's base from the disc when no buffer is set) */
#ifdef TM2_PORT_CAR_NUMBERS
#define TM2_LOAD_HUMAN_CAR(s, i) portCarNumberGet((s), 0, (i))
#define TM2_LOAD_OPPONENT_CAR(s, i) portCarNumberGet((s), 1, (i))
#else
#define TM2_LOAD_HUMAN_CAR(s, i) (s8)(u8)TM2_SETTINGS_HUMAN_CAR(s, i)
#define TM2_LOAD_OPPONENT_CAR(s, i) (s8)(u8)TM2_SETTINGS_OPPONENT_CAR(s, i)
#endif
extern s32 bitmapParseContainer(u8 *, s32);
extern void arenaResetSystems(s32);
extern void assetInstallImage(u8 *, s32);
extern void __main(void);
extern void *sndListCtor(u8 *);
extern void *cdAudioCtor(TM2SoundObj *);
extern void *sndObjCtor(TM2Slot120 *);
extern void shellInitMachine(void);
extern void shellLoadUaCore(void);
extern void settingsInitDefaults(struct TM2Settings *);
extern s32 screenDisplayImage(s32, u8 *, s32, s32);
extern s32 padWaitAnyButtonOrTimeout(s32, s32);
extern void shellScreenLoop(void);

/* Clamp a serial (CD audio) volume to [audioVolumeClampMin, audioVolumeClampMax] (min, then
 * max, as retail stores it) and apply it. */
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

/* Present one frame of the shell object: virtual method 1 (args 28,0,0,99,-1,0),
 * taskMgrRunFrame, then virtual method 2. */
static __inline__ void ShellPresent(void)
{
    u8 *obj;
    u8 *vt;

    obj = soundManagerObject;
    vt = *(u8 **)(obj + 200);
    (*(TM2Method *)(vt + 12))(obj + *(s16 *)(vt + 8), 28, 0, 0, 99, -1, 0);
    taskMgrRunFrame(soundManagerObject);
    {
        u8 *obj2;
        u8 *vt2;

        obj2 = soundManagerObject;
        vt2 = *(u8 **)(obj2 + 200);
        (*(TM2Method *)(vt2 + 20))(obj2 + *(s16 *)(vt2 + 16));
    }
}

/* Two-player match result screen: tell the sound object (method 3, arg 2),
 * mute CD audio, then print "YOU WIN"/"YOU LOSE" and the wins/losses tally. */
void __attribute__((section(".text_8012B0EC"))) screenChallengeResult(s32 unused, u8 won, s16 wins, s16 losses)
{
    u8 *obj;
    u8 *vt;
    s16 x;
    s16 y;
    s16 step;

    obj = musicManagerObject;
    vt = *(u8 **)(obj + 8);
    (*(TM2Method *)(vt + 28))(obj + *(s16 *)(vt + 24), 2);
    step = 20;
    x = 159;
    y = 80;
    SetSerialVolume(0);
    screenLostALife();
    if (won)
        fontPrintCenteredXY(6, x, y, strYouWin);
    else
        fontPrintCenteredXY(6, x, y, strYouLose);
    ShellPresent();
    fontPrintCenteredXY(0, x, y + step * 2, strWinsLossesWide);
    sprintf(textFormatBuffer, strWinsLossesFmt, wins, losses);
    fontPrintCenteredXY(0, x, y + step * 3, textFormatBuffer);
}

/* Show up to three centred text lines; lines 2 and 3 (if their string is
 * non-null) each appear after a 30-frame wait with a fresh frame. */
void __attribute__((section(".text_8012B5FC"))) screenShowMessageLines(s32 unused,
    s32 font1, char *str1, s32 x1, s32 y1,
    s32 font2, char *str2, s32 x2, s32 y2,
    s32 font3, char *str3, s32 x3, s32 y3)
{
    screenLostALife();
    ShellPresent();
    fontPrintCenteredXY(font1, x1, y1, str1);
    if (str2) {
        padWaitConfirmOrTimeout(30, 30);
        ShellPresent();
        fontPrintCenteredXY(font2, x2, y2, str2);
    }
    if (str3) {
        padWaitConfirmOrTimeout(30, 30);
        ShellPresent();
        fontPrintCenteredXY(font3, x3, y3, str3);
    }
}

/* Movie player: play `name` (`frames` long; `wide` = 24-bit 480-wide VRAM
 * layout) through moviePlay with the given skip callback, then, if
 * the movie was not aborted, copy the last frame and restore display/draw
 * environments. Returns moviePlay's result.
 * MoveImage's x and 0 go through the locals mx (s16, so cse does not reuse
 * moviePlay's sign-extended x) and my (set in a do/while(0) so the 0 is not
 * folded); they get a1/a2 before the rect stores, as in retail. */
s32 __attribute__((section(".text_8012B898"))) moviePlayEnding(struct TM2Settings *settings, char *name, s32 frames, s32 wide, void *skip)
{
    u8 done;
    s16 x;
    s32 w;
    s32 h;
    s32 result;

    x = 320;
    done = 1;
    VSync(0);
    w = 320;
    h = 240;
    DrawSync(0);
    screenSetEnv();
    if (wide) {
        x = 480;
        SetDefDispEnv(&movieDispEnvs[0], 0, 0, 320, h);
        SetDefDispEnv(&movieDispEnvs[1], 480, 0, 320, h);
        movieDispEnvs[0].isrgb24 = 0;
        movieDispEnvs[1].isrgb24 = 1;
        PutDispEnv(&movieDispEnvs[1]);
        SetDefDrawEnv(&movieDrawEnvs[0], 0, 0, 480, 256);
        SetDefDrawEnv(&movieDrawEnvs[1], 480, 0, 480, 256);
        movieDrawEnvs[0].isbg = 1;
        movieDrawEnvs[1].isbg = 1;
        PutDrawEnv(&movieDrawEnvs[1]);
    } else {
        SetDefDispEnv(&movieDispEnvs[0], 0, 0, 320, h);
        SetDefDispEnv(&movieDispEnvs[1], 320, 0, 320, h);
    }
    result = moviePlay(name, skip, 0, wide, frames, 0, 0, x, 0, w, h, movieDispEnvs, &done);
    if (!done) {
        TM2Rect rect;
        s16 mx;
        s32 my;

        DrawSync(0);
        VSync(0);
        mx = x;
        do my = 0; while (0);
        rect.x = 0;
        rect.y = 0;
        if (wide)
            rect.w = w / 16 * 24;
        else
            rect.w = w;
        rect.h = h;
        MoveImage(&rect, mx, my);
        DrawSync(0);
        PutDispEnv(&movieDispEnvs[1]);
        movieDrawEnvs[1].isbg = 0;
        PutDrawEnv(&movieDrawEnvs[1]);
    }
    return result;
}

/* Load a race's track data with "LOADING" shown: the main file, then the
 * per-player (+4 count, ids at +8) and per-AI (+5 count, ids at +10) files
 * into 0x80019C40 (handle kept at +0x20), then the second file set packed
 * one after another from 0x80019C40. */
void __attribute__((section(".text_8012BAFC"))) sessionLoadArenaAndCars(struct TM2Settings *settings)
{
#ifdef TM2_PORT_CARS
    s32 i;
    s32 n;
    s32 off;
    s32 size;
    char *msg;
    u8 *base;
    u32 end;
#ifdef TM2_PORT_SPECIAL_AS
    s32 k;
#endif

    /* request 32 step 2 (TM2_PORT_CARS; SIXTEEN_CARS.md 3.5, CAR_GROUPS.md): every car file goes through
       portCarLoadGroup (the disc file, or a buffer the port hands over for that load group: mod cars,
       request 33), and with portCarOptions bit 1 the car DMDs are packed from the start of portCarDmdArea
       (above 2 MB) instead of after the level DMD. The level files and the TMS stay at 0x80019C40
       (assetInstallImage relocates a car anywhere). With no buffer and bit 1 clear this is retail's
       sequence of loads, call for call. */
#ifdef TM2_PORT_ROLLBACK
    portRollbackPreload();   /* request 48 SEED: srand(the load's seed), timerFrameCount = portRollbackFrameCount */
#endif
    screenDisplayToDisplay(1, 0);
    bitmapInit();
    fileioLoadFileIntoRam((u8 *)0x80019C40, settingsLevelTmsFileName(settings));
    msg = strLoading;
    settings->f20 = bitmapParseContainer((u8 *)0x80019C40, 0);
    fontPrintCenteredXY(0, 159, 40, msg);
    off = 0;
    n = 1;
    for (i = 0; i < settings->humans; i++, n++) {
        portCarLoadGroup((u8 *)0x80019C40, n, TM2_LOAD_HUMAN_CAR(settings, i), 0, TM2_PORT_LOW_AREA_END);
        settings->f20 = bitmapParseContainer((u8 *)0x80019C40, n);
    }
    for (i = 0; i < settings->opponents; i++, n++) {
        portCarLoadGroup((u8 *)0x80019C40, n, TM2_LOAD_OPPONENT_CAR(settings, i), 0, TM2_PORT_LOW_AREA_END);
        settings->f20 = bitmapParseContainer((u8 *)0x80019C40, n);
    }
#ifdef TM2_PORT_SPECIAL_AS
    /* request 35 (GUN_GAME_SPECIALS.md "As built"): the car files of portCarSpecialLoad's types that no car of the
       match has (with bit 1), as spare groups after the opponents', parked below; 0 = none (retail) */
    portCarSpecialLoadPlan(settings);
    for (i = 0; (k = portCarSpecialLoadCar(i)) >= 0; i++, n++) {
        portCarLoadGroup((u8 *)0x80019C40, n, k, 0, TM2_PORT_LOW_AREA_END);
        settings->f20 = bitmapParseContainer((u8 *)0x80019C40, n);
    }
#endif
#ifdef TM2_PORT_CAR_CLUTS
    /* request 33-1 (CAR_GROUPS.md 13): with portCarOptions bit 7 every CLUT a car took from another car or the
       arena gets its own copy now, after every original is placed and before the DMDs are remapped */
    portCarClutsCopyAll();
#endif
    screenDisplayToDisplay(0, 1);
    size = fileioLoadFileIntoRam((u8 *)0x80019C40, settingsLevelDmdFileName(settings));
    arenaResetSystems(settings->f20);
    fontPrintCenteredXY(0, 159, 40, msg);
    if (portCarOptions & TM2_PORT_CAR_OPT_HIGH_DMD) {
        base = (u8 *)portCarDmdArea;
        end = (u32)portCarDmdArea + TM2_PORT_CAR_DMD_AREA_BYTES;
        off = 0;
    } else {
        base = (u8 *)0x80019C40;
        end = TM2_PORT_LOW_AREA_END;
        off += size;
    }
    n = 1;
    for (i = 0; i < settings->humans; i++, n++, off += size) {
        size = portCarLoadGroup(base + off, n, TM2_LOAD_HUMAN_CAR(settings, i), 1, end);
        assetInstallImage(base + off, n);
    }
    for (i = 0; i < settings->opponents; i++, n++, off += size) {
        size = portCarLoadGroup(base + off, n, TM2_LOAD_OPPONENT_CAR(settings, i), 1, end);
        assetInstallImage(base + off, n);
    }
#ifdef TM2_PORT_SPECIAL_AS
    for (i = 0; (k = portCarSpecialLoadCar(i)) >= 0; i++, n++, off += size) {   /* request 35: the spares */
        size = portCarLoadGroup(base + off, n, k, 1, end);
        assetInstallImage(base + off, n);
    }
    portCarSpecialLoadDone();
#endif
#ifdef TM2_PORT_CAR_DROPIN
    /* request 33 (CAR_GROUPS.md 14): where a car loaded mid-match goes (the area's free end, the next group) */
    portCarLoadDone(base + off, end, n);
#endif
    bitmapDisableTracking();

#else
    s32 i;
    s32 n;
    s32 off;
    s32 size;
    char *msg;

#ifdef TM2_PORT_ROLLBACK
    portRollbackPreload();   /* request 48 SEED, as above */
#endif
    screenDisplayToDisplay(1, 0);
    bitmapInit();
    fileioLoadFileIntoRam((u8 *)0x80019C40, settingsLevelTmsFileName(settings));
    msg = strLoading;
    settings->f20 = bitmapParseContainer((u8 *)0x80019C40, 0);
    fontPrintCenteredXY(0, 159, 40, msg);
    off = 0;
    n = 1;
    for (i = 0; i < settings->humans; i++, n++) {
#if TM2_MAX_HUMANS > 2
        fileioLoadFileIntoRam((u8 *)0x80019C40, carTmsFileName(settings, (s8)(u8)TM2_SETTINGS_HUMAN_CAR(settings, i)));
#else
        fileioLoadFileIntoRam((u8 *)0x80019C40, carTmsFileName(settings, (s8)(u8)settings->human_car[i]));
#endif
        settings->f20 = bitmapParseContainer((u8 *)0x80019C40, n);
    }
    for (i = 0; i < settings->opponents; i++, n++) {
        fileioLoadFileIntoRam((u8 *)0x80019C40, carTmsFileName(settings, (s8)(u8)settings->opponent_car[i]));
        settings->f20 = bitmapParseContainer((u8 *)0x80019C40, n);
    }
#ifdef TM2_PORT_CAR_CLUTS
    portCarClutsCopyAll();   /* request 33-1 (CAR_GROUPS.md 13), as above */
#endif
    screenDisplayToDisplay(0, 1);
    size = fileioLoadFileIntoRam((u8 *)0x80019C40, settingsLevelDmdFileName(settings));
    arenaResetSystems(settings->f20);
    fontPrintCenteredXY(0, 159, 40, msg);
    off += size;
    n = 1;
    for (i = 0; i < settings->humans; i++, n++, off += size) {
#if TM2_MAX_HUMANS > 2
        size = fileioLoadFileIntoRam((u8 *)0x80019C40 + off, carDmdFileName(settings, (s8)(u8)TM2_SETTINGS_HUMAN_CAR(settings, i)));
#else
        size = fileioLoadFileIntoRam((u8 *)0x80019C40 + off, carDmdFileName(settings, (s8)(u8)settings->human_car[i]));
#endif
        assetInstallImage((u8 *)0x80019C40 + off, n);
    }
    for (i = 0; i < settings->opponents; i++, n++, off += size) {
        size = fileioLoadFileIntoRam((u8 *)0x80019C40 + off, carDmdFileName(settings, (s8)(u8)settings->opponent_car[i]));
        assetInstallImage((u8 *)0x80019C40 + off, n);
    }
    bitmapDisableTracking();
#endif
}

/* This is the game's main() (IDA name "main"), named by address for the
 * matcher. Builds the shell object (soundManagerObject) and sound object
 * (musicManagerObject) on the stack plus the 35 120-byte slots listed in
 * sndObjPool, sets default volumes, shows three logo screens (a skip ends
 * the sequence) and the intro movie, then runs the shell. */
s32 __attribute__((section(".text_8012C8DC"))) sub_8012C8DC(void)
{
    TM2ShellObj shell;
    TM2SoundObj sound;
#if TM2_MAX_SOUND_REQUESTS == TM2_RETAIL_SOUND_REQUESTS
    TM2Slot120 slots[35];
#endif
    s32 k;
    u8 *p;
    TM2Slot120 *s;
    s32 n;
    s32 i;
    TM2ShellObj *self;

    __main();
    self = (TM2ShellObj *)&shell;
    p = self->members[0];
    k = 66;
    self->vt = shellObjBaseVtbl;
    self->vt = soundMgrVtbl;
    do {
        sndListCtor(p);
        k--;
        p += 20;
    } while (k != -1);
    sndListCtor(self->extra[0]);
    sndListCtor(self->extra[1]);
    sndListCtor(self->extra[2]);
    sndListCtor(self->extra[3]);
    sndListCtor(self->extra[4]);
    sndListCtor(self->extra[5]);
    sndListCtor(self->extra[6]);
    soundManagerObject = (u8 *)&shell;
    cdAudioCtor(&sound);
    musicManagerObject = (u8 *)&sound;
#if TM2_MAX_SOUND_REQUESTS == TM2_RETAIL_SOUND_REQUESTS
    s = slots;
    n = 34;
    do {
        sndObjCtor(s);
        n--;
        s++;
    } while (n != -1);
    for (i = 0; i < 35; i++)
        sndObjPool[i] = &slots[i];
#else
    /* request 33-4 (TM2_PORT_CARS): the raised request pool's objects are port storage, not main's stack */
    s = (TM2Slot120 *)portSndObjSlots;
    n = TM2_MAX_SOUND_REQUESTS - 1;
    do {
        sndObjCtor(s);
        n--;
        s++;
    } while (n != -1);
    for (i = 0; i < TM2_MAX_SOUND_REQUESTS; i++)
        sndObjPool[i] = &((TM2Slot120 *)portSndObjSlots)[i];
#endif
    shellInitMachine();
    audioSfxVolumeCap = 112;
    SetSfxVolume(91);
    SetSerialVolume(104);
    shellLoadUaCore();
    settingsInitDefaults(&sessionSettings);
    screenLoadImageFile(strScreenSceaPresTim, 0);
    screenDisplayImage(1, (u8 *)0x80048270, 0, 0);
    if (padWaitAnyButtonOrTimeout(0, 120) == 0) {
        screenLoadImageFile(strScreenSisaProdTim, 0);
        screenDisplayImage(1, (u8 *)0x80048270, 0, 0);
        if (padWaitAnyButtonOrTimeout(0, 120) == 0) {
            screenLoadImageFile(strScreenDevelopTim, 0);
            screenDisplayImage(1, (u8 *)0x80048270, 0, 0);
            if (padWaitAnyButtonOrTimeout(0, 120) == 0)
                moviePlayEnding(&sessionSettings, strMovieSlogo15s, 149, 1, movieSkipCheckDelayed);
        }
    }
    shellScreenLoop();
    return 1;
}
