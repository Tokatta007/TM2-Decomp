/* Small game helpers (shell/session 8012 range). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
#include "semantic/settings.h"

#pragma tm2_rdata .tm2_800CFB38

/* POLY_FT4 (40 bytes): one per card at screenCardQuads, template at [9]. */
typedef struct {
    u8 addr[3], len;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad1;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad2;
} TM2PolyFT4;
typedef struct { u16 a[9]; u16 b[9]; } TM2OpponentLayout;


extern char fileNameExtFmt[], fileExtDmd[], fileExtTms[];
extern char strDmdDir[], strFileNameFmtDirStemExt[], strTmsDir[], strLevelAltFileFmt[];
extern char textFormatBuffer[];
extern TM2OpponentLayout screenCardSlotLayouts[];
extern TM2PolyFT4 screenCardQuads[];
extern s16 screenCardPosX, screenCardPosY, screenCardAngle;

extern int DrawSync(s32);
extern s32 VSync(s32);
extern int sprintf(char *, const char *, ...);
extern s32 fileioLoadFileIntoRam(void *, const char *);
extern s32 bitmapParseContainer(void *, s32);
extern void worldResetLevelModelFlag(void);
extern void worldLoadStartObjects(s32);
extern s32 sessionGetOpponents(struct TM2Settings *);
extern s32 sessionGetMode(struct TM2Settings *);
extern void padFlush(void);
#include "semantic/input.h"
extern s32 levelFileStem(struct TM2Settings *, s32);
extern void arenaSetup(s32);
extern s32 carIndexToType(struct TM2Settings *, s32);
#ifdef TM2_PORT_CAR_GROUPS
/* request 32 (CAR_GROUPS.md): the binders also take the car's load group */
extern void carBindHumanByType(s32, s32, s32);
extern void carBindAiByType(s32, s32, s32);
#ifdef TM2_PORT_CAR_SWAP
#include "semantic/car_groups.h"   /* request 32 step 3: portCarBindGroupOf, portCarBindTypeOf */
#include "semantic/car_dropin.h"   /* request 33: portCarBindCount, portCarBindSkip */
#include "semantic/port_humans.h"  /* request 50: vacant human records (TM2_HUMAN_VACANT, TM2_HUMAN_BIND_AI_COUNT) */
#endif
#else
extern void carBindHumanByType(s32, s32);
extern void carBindAiByType(s32, s32);
#endif

/* @80129010: load "<name>.TMS" and "<name>.DMD" through the 0x80019C40 buffer and install them. */
void __attribute__((section(".text_80129010"))) shellLoadTmsDmd(const char *name)
{
    char dmd[40];
    char tms[40];
    s32 handle;

    DrawSync(0);
    sprintf(dmd, fileNameExtFmt, name, fileExtDmd);
    sprintf(tms, fileNameExtFmt, name, fileExtTms);
    fileioLoadFileIntoRam((void *)0x80019C40, tms);
    handle = bitmapParseContainer((void *)0x80019C40, 0);
    fileioLoadFileIntoRam((void *)0x80019C40, dmd);
    worldResetLevelModelFlag();
    worldLoadStartObjects(handle);
}

/* @801291CC: wait at least `frames` frames, then until the pad confirms; a non-negative
   `timeout` vsyncs each frame and gives up (returns 0) after that many frames. */
s32 __attribute__((section(".text_801291CC"))) padWaitAnyButtonOrTimeout(s32 frames, s32 timeout)
{
    s32 count = 0;
    s16 pad;

    pad = (sessionGetMode(&sessionSettings) == 1 || sessionGetMode(&sessionSettings) == 3) ? 0 : 2;
    padFlush();
    for (;;) {
        padUpdate();
        if (timeout >= 0)
            VSync(0);
        count++;
        if (count < frames)
            continue;
        if (padAnyPressed(pad))
            return 1;
        if (timeout >= 0 && timeout < count)
            return 0;
    }
}

/* @80129BC8: split-screen viewport half-sizes for the current opponent count. */
void __attribute__((section(".text_80129BC8"))) screenCardHalfSize(s16 *width, s16 *height)
{
    s16 value;

    switch (sessionGetOpponents(&sessionSettings)) {
    case 7:
    case 8:
    case 9:
        *width = 38;
        value = 19;
        break;
    case 5:
    case 6:
        *width = 46;
        value = 23;
        break;
    case 4:
        *width = 50;
        value = 25;
        break;
    case 3:
        *width = 60;
        value = 30;
        break;
    case 2:
    default:
        *width = 80;
        value = 40;
        break;
    }
    *height = value;
}

/* @80129C7C: per-opponent-count screen layout entry `index` (two u16 tables). */
void __attribute__((section(".text_80129C7C"))) screenCardSlotPos(s32 index, u16 *a, u16 *b)
{
    *a = screenCardSlotLayouts[sessionGetOpponents(&sessionSettings) - 1].a[index];
    *b = screenCardSlotLayouts[sessionGetOpponents(&sessionSettings) - 1].b[index];
}

/* @8012A014: reset the 160x320 screen globals and copy the template quad's
   xy pairs (record 9) into record `index`. */
void __attribute__((section(".text_8012A014"))) screenCardSettle(s32 index)
{
    screenCardPosX = 160;
    screenCardPosY = 320;
    screenCardAngle = 0;
    if (index >= 0) {
        screenCardQuads[index].x0 = screenCardQuads[9].x0;
        screenCardQuads[index].y0 = screenCardQuads[9].y0;
        screenCardQuads[index].x1 = screenCardQuads[9].x1;
        screenCardQuads[index].y1 = screenCardQuads[9].y1;
        screenCardQuads[index].x2 = screenCardQuads[9].x2;
        screenCardQuads[index].y2 = screenCardQuads[9].y2;
        screenCardQuads[index].x3 = screenCardQuads[9].x3;
        screenCardQuads[index].y3 = screenCardQuads[9].y3;
    }
}

/* @8012A998: build the ".DMD" file name for a session (alternate format for modes 2/4 with
   course 3, 5 or 6) into textFormatBuffer. */
char *__attribute__((section(".text_8012A998"))) settingsLevelDmdFileName(struct TM2Settings *settings)
{
    if ((settings->mode == 4 || settings->mode == 2) &&
        (settings->level == 3 || (u32)(settings->level - 5) < 2))
        sprintf(textFormatBuffer, strLevelAltFileFmt, strDmdDir, levelFileStem(settings, settings->level), fileExtDmd);
    else
        sprintf(textFormatBuffer, strFileNameFmtDirStemExt, strDmdDir, levelFileStem(settings, settings->level), fileExtDmd);
    return textFormatBuffer;
}

/* @8012AA5C: as settingsLevelDmdFileName but for the ".TMS" file name. */
char *__attribute__((section(".text_8012AA5C"))) settingsLevelTmsFileName(struct TM2Settings *settings)
{
    if ((settings->mode == 4 || settings->mode == 2) &&
        (settings->level == 3 || (u32)(settings->level - 5) < 2))
        sprintf(textFormatBuffer, strLevelAltFileFmt, strTmsDir, levelFileStem(settings, settings->level), fileExtTms);
    else
        sprintf(textFormatBuffer, strFileNameFmtDirStemExt, strTmsDir, levelFileStem(settings, settings->level), fileExtTms);
    return textFormatBuffer;
}

/* @8012BD94: apply a session's level, then bind its human and opponent cars
   (car-select indices human_car[] / opponent_car[]). */
void __attribute__((section(".text_8012BD94"))) sessionBindCars(struct TM2Settings *settings)
{
    s32 i;

    arenaSetup(settings->level);
#ifdef TM2_PORT_CAR_GROUPS
    /* request 32 (CAR_GROUPS.md): each car binds to its own load group, in the order of
       sessionLoadArenaAndCars (human i = group i + 1, opponent i = group humans + i + 1) */
#ifdef TM2_PORT_CAR_SWAP
    /* request 32 step 3 (CAR_GROUPS.md 8): a record that portCarSetGroup moved to another car binds to
       that car's group and type (portCarBindGroup); with no swap both functions return their argument */
#ifdef TM2_PORT_CAR_DROPIN
    /* request 33 (CAR_GROUPS.md 14): once a car was removed, started or loaded in this load, the records there
       are, each on its own group, and no vacant AI record; before that the settings' counts (retail) */
    for (i = 0; i < portCarBindCount(0, settings->humans); i++)
#else
    for (i = 0; i < settings->humans; i++)
#endif
        if (!TM2_HUMAN_VACANT(i))   /* request 50: a vacant human record stays unbound (always true with no define) */
#if TM2_MAX_HUMANS > 2
        carBindHumanByType(i, portCarBindTypeOf(0, i, carIndexToType(settings, TM2_SETTINGS_HUMAN_CAR(settings, i))),
                           portCarBindGroupOf(0, i, i + 1));
#else
        carBindHumanByType(i, portCarBindTypeOf(0, i, carIndexToType(settings, settings->human_car[i])),
                           portCarBindGroupOf(0, i, i + 1));
#endif
#ifdef TM2_PORT_CAR_DROPIN
    for (i = 0; i < portCarBindCount(1, settings->opponents); i++)
        if (!portCarBindSkip(1, i))
#else
    for (i = 0; i < TM2_HUMAN_BIND_AI_COUNT(settings->opponents); i++)   /* request 50: + hand-overs' records */
#endif
        carBindAiByType(i, portCarBindTypeOf(1, i, carIndexToType(settings, TM2_SETTINGS_OPPONENT_CAR(settings, i))),
                        portCarBindGroupOf(1, i, settings->humans + i + 1));
#else
    for (i = 0; i < settings->humans; i++)
#if TM2_MAX_HUMANS > 2
        carBindHumanByType(i, carIndexToType(settings, TM2_SETTINGS_HUMAN_CAR(settings, i)), i + 1);
#else
        carBindHumanByType(i, carIndexToType(settings, settings->human_car[i]), i + 1);
#endif
    for (i = 0; i < settings->opponents; i++)
        carBindAiByType(i, carIndexToType(settings, TM2_SETTINGS_OPPONENT_CAR(settings, i)), settings->humans + i + 1);
#endif
#else
    for (i = 0; i < settings->humans; i++)
#if TM2_MAX_HUMANS > 2
        carBindHumanByType(i, carIndexToType(settings, TM2_SETTINGS_HUMAN_CAR(settings, i)));    /* 3-4: port table */
#else
        carBindHumanByType(i, carIndexToType(settings, settings->human_car[i]));
#endif
    for (i = 0; i < settings->opponents; i++)
        carBindAiByType(i, carIndexToType(settings, TM2_SETTINGS_OPPONENT_CAR(settings, i)));
#endif
}
