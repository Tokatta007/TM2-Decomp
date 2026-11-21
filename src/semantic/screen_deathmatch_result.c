/* Game code (8011..8012 range). Names stay address-based. */
#include "semantic/cars.h"
#include "semantic/settings.h"

typedef void (*TM2Method)(u8 *, ...);
typedef struct { s16 delta; s16 index; TM2Method fn; } TM2VEntry;  /* vtable slot */

/* ---- car-to-car relative vectors ---- */
extern s32 carWorldObjectCount;              /* number of cars */
extern s32 carSlotKinds[];            /* car kind: 0 human, 1 AI */
extern s32 carSlotIndices[];            /* index in its array */
extern s32 carPairVectors[][TM2_MAX_CAR_SLOTS][3];     /* vector from car i to car j */
extern s32 carPairDirections[][TM2_MAX_CAR_SLOTS][3];     /* normalised vector */
extern s16 carPairHeadings[][TM2_MAX_CAR_SLOTS];        /* heading from car i to car j */
extern s32 carPairHeadingDeltas[][TM2_MAX_CAR_SLOTS];        /* heading change since last frame */
extern s32 carPairDistances[][TM2_MAX_CAR_SLOTS];        /* distance from car i to car j */
extern int VectorNormal(s32 *, s32 *);
extern int ratan2(int, int);
#include "semantic/world.h"
extern s32 sub_8013FC44(s32);

static __inline__ s32 *CarPos(s32 i)
{
    s32 idx = carSlotIndices[i];
    if (carSlotKinds[i] == 0)
        return humanCars[idx].pos;
    return aiCars[idx].pos;
}

/* @8011E7D8: for every ordered pair of cars (i, j), store the vector from i
   to j, its normalised form, its heading (and the heading change since the
   previous call, wrapped by mathBoundAngle) and the distance. */
void __attribute__((section(".text_8011E7D8"))) carPairsUpdate(void)
{
    s32 i, j;
    s32 *me, *other;
    s32 old, a;

    for (i = 0; i < carWorldObjectCount; i++) {
        me = CarPos(i);
        for (j = 0; j < carWorldObjectCount; j++) {
            if (i != j) {
                other = CarPos(j);
                carPairVectors[i][j][0] = other[0] - me[0];
                carPairVectors[i][j][1] = other[1] - me[1];
                carPairVectors[i][j][2] = other[2] - me[2];
                VectorNormal(carPairVectors[i][j], carPairDirections[i][j]);
                old = carPairHeadings[i][j];
                a = ratan2(carPairDirections[i][j][0], carPairDirections[i][j][1]);
                carPairHeadings[i][j] = a;
                carPairHeadingDeltas[i][j] = a - old;
                mathBoundAngle(&carPairHeadingDeltas[i][j]);
                carPairDistances[i][j] = sub_8013FC44(carPairVectors[i][j][0] * carPairVectors[i][j][0]
                    + carPairVectors[i][j][1] * carPairVectors[i][j][1]
                    + carPairVectors[i][j][2] * carPairVectors[i][j][2]);
            }
        }
    }
}

/* ---- texture upload ---- */
typedef struct { s16 x, y, w, h; } TM2Rect;
typedef struct {
    u32 mode;           /* bits 0-2 colour mode, bit 3 has CLUT */
    s32 f4;
    u16 w, h;
    void *pixels;
    s32 f10, f14;
    void *clut;
} TM2TexImg;
typedef struct { s32 tpage, x, y, clutx, cluty; } TM2TexSlot;
extern TM2TexSlot *vramTexSlots;
extern s32 *bitmapKeyTable;
extern s32 vramTexSlotGroup;
extern char strRelocTexNotLoaded[];
extern s32 *bitmapFindRecord(s32);
extern s32 bitmapIsFreeXY(s32, s32);
#include "semantic/scene.h"
extern s32 vramUploadClut(s32, s32, void *, s32, s32);
extern int LoadImage(TM2Rect *, void *);
extern int DrawSync(s32);
extern u16 GetTPage(s32, s32, s32, s32);
extern int printf(const char *, ...);
#if defined(TM2_PORT_TEXTURE_BANKS) || defined(TM2_PORT_CAR_CLUTS)
#include "semantic/tex_banks.h"   /* request 32 step 4; request 33-1 (car_cluts.h) */
#endif
#ifdef TM2_PORT_CAR_CLUTS
/* request 41 item B (CAR_GROUPS.md 13.12, RETAIL_BUGS.md 27): with portCarOptions bit 9 latched the first two
   passes test the whole rectangle (portImageSpotFree), then retail's two passes run as the fallback; bit 9
   clear: retail's two passes with its top-left test */
#define IMAGE_PASSES portImagePasses()
#define IMAGE_SPOT_FREE(x, y, w, h) portImageSpotFree(x, y, w, h, pass)
#define IMAGE_PASS_START (pass & 1) == 0 ? 0x3C0 : 0
#else
#define IMAGE_PASSES 2
#define IMAGE_SPOT_FREE(x, y, w, h) bitmapIsFreeXY(x, y)
#define IMAGE_PASS_START pass == 0 ? 0x3C0 : 0
#endif

/* @801209E8: give texture (col,row) a VRAM home: reuse a known placement from
   bitmapFindRecord if there is one, else find a free w x h area (first pass only
   the last page column, second pass all), upload the pixels (and the 16/256
   colour CLUT via vramUploadClut), and record tpage/x/y in slot row*20+col.
   Returns 1 on success, else prints a message and returns 0. */
s32 __attribute__((section(".text_801209E8"))) vramPlaceTexture(TM2TexImg *img, s32 col, s32 row)
{
    TM2Rect r;
    s32 *known;
    s32 found = 0;
    s32 result = 0;
    s32 idx;
    s32 w, pass, h;
    s32 px, py, x, y;
#ifdef TM2_PORT_TEX_SHARED
    s32 bank;
#endif

    vramTexSlotGroup = row;
    idx = row * TM2_TEX_SLOT_STRIDE + col;   /* 20; 32 with TM2_PORT_CARS (players.h) */
#ifdef TM2_PORT_TEX_SHARED
    /* request 32 step 4 (TEXTURE_BANKS.md "As built"): the group's bank (0 = VRAM, retail; allocated at its
       first image while portCarOptions bit 5 is latched). A banked group takes only the arena's images by
       key (they keep bank 0), a group with portCarGroupNoReuse set none; with neither, bitmapFindRecord's
       first record with the key, as retail (request 33-1 without banks: always the last case, bank 0) */
    bank = portTexPlaceBegin(row);
    known = portTexFindReuse(bitmapKeyTable[col], row, bank);
#else
    known = bitmapFindRecord(bitmapKeyTable[col]);
#endif
    if (known) {
        vramTexSlots[idx].tpage = known[1];
        vramTexSlots[idx].x = known[2];
        vramTexSlots[idx].y = known[3];
        vramTexSlots[idx].clutx = known[4];
        vramTexSlots[idx].cluty = known[5];
#ifdef TM2_PORT_TEX_SHARED
        portTexNoteSlot(row, col);
#endif
#ifdef TM2_PORT_CAR_CLUTS
        /* request 33-1 (CAR_GROUPS.md 13): with portCarOptions bit 7 a CLUT taken from another car or the arena
           is noted here and copied after the last car TMS (portCarClutsCopyAll) */
        portCarClutNote(row, col, img->mode, known);
#endif
        return 1;
    }
    pass = 0;
    w = img->w;
    h = img->h;
    do {
        s32 start = IMAGE_PASS_START;
        pass++;
        for (py = 256; py < 512 && !found; py += 256)
            for (px = start; px < 1024 && !found; px += 256)
                for (x = px; x < px + 256 && !found; x += w)
                    for (y = py; y < py + 256 && !found; y += h)
#ifdef TM2_PORT_TEXTURE_BANKS
                        /* a banked group: retail's scan, whole rectangles tested in its bank */
                        if (bank ? portTexBankIsFree(bank, x, y, w, h) : IMAGE_SPOT_FREE(x, y, w, h)) {
                            if (bank)
                                portTexBankMark(bank, x, y, w, h);
                            else
                                bitmapMarkRect(x, y, w, h);
#else
                        if (IMAGE_SPOT_FREE(x, y, w, h)) {
                            bitmapMarkRect(x, y, w, h);
#endif
                            r.x = x;
                            r.y = y;
                            r.w = w;
                            r.h = h;
#ifdef TM2_PORT_TEXTURE_BANKS
                            portVramUploadBank = bank;   /* the port's GPU: this upload goes to the bank */
#endif
                            LoadImage(&r, img->pixels);
                            DrawSync(0);
#ifdef TM2_PORT_TEXTURE_BANKS
                            portVramUploadBank = 0;
#endif
                            result = 1;
                            if ((img->mode >> 3) & 1) {
                                if ((img->mode & 7) == 0)
                                    result = vramUploadClut(16, 1, img->clut, col, row);
                                else if ((img->mode & 7) == 1)
                                    result = vramUploadClut(256, 1, img->clut, col, row);
                            }
#ifdef TM2_PORT_TEXTURE_BANKS
                            /* the slot records the bank in tpage bits 9..15 (modelRemapTextures) */
                            vramTexSlots[idx].tpage = (GetTPage(img->mode, 0, x, y) & 0x1F) | TM2_TEX_BANK_BITS(bank);
#else
                            vramTexSlots[idx].tpage = GetTPage(img->mode, 0, x, y) & 0x1F;
#endif
                            vramTexSlots[idx].x = x;
                            vramTexSlots[idx].y = y;
#ifdef TM2_PORT_TEX_SHARED
                            portTexNoteSlot(row, col);
#endif
#ifdef TM2_PORT_CAR_CLUTS
                            portCarClutNote(row, col, img->mode, 0);   /* its own CLUT (request 33-1) */
#endif
                            found = 1;
                        }
    } while (pass < IMAGE_PASSES && !result);
    if (!result)
        printf(strRelocTexNotLoaded, row, col, w, h);
    return result;
}

/* ---- card sprites ---- */
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

extern u8 screenCardDispEnvs[];             /* DISPENV[2] */
extern u8 screenCardDrawEnvs[];             /* DRAWENV[2] */
extern u8 screenCardDrawEnv0Isbg, screenCardDrawEnv0Dtd, screenCardDrawEnv1Isbg, screenCardDrawEnv1Dtd;
#pragma tm2_absolute screenCardDrawEnv0Isbg
#pragma tm2_absolute screenCardDrawEnv0Dtd
#pragma tm2_absolute screenCardDrawEnv1Isbg
#pragma tm2_absolute screenCardDrawEnv1Dtd
extern s32 screenCardLevel;
extern TM2PolyFT4 screenCardQuads[];
extern s32 sessionGetLevel(struct TM2Settings *);
extern s32 sessionGetOpponents(struct TM2Settings *);
extern void screenCardHalfSize(s16 *, s16 *);
extern void *SetDefDispEnv(void *, s32, s32, s32, s32);
extern void *SetDefDrawEnv(void *, s32, s32, s32, s32);
extern void SetSemiTrans(void *, s32);

/* setPolyFT4 + semi-transparency off + neutral grey (0x7F) colour. */
#define SetupQuad(p) \
    ((p)->len = 9, (p)->code = 0x2C, SetSemiTrans((p), 0), \
     (p)->r0 = 0x7F, (p)->g0 = 0x7F, (p)->b0 = 0x7F)

/* @80129D34 (IDA: screenInitCardSprites): set up the two 320x240 display/draw
   buffers and one semi-transparent textured quad per card (four cards per
   texture page, 64 lines each; CLUT row 128+i), plus the template quad 9. */
void __attribute__((section(".text_80129D34"))) screenInitCardSprites(void)
{
    s16 w, h;
    s16 i;
    s32 v = 0;
    s32 page = 9;
    s32 top;

    screenCardLevel = sessionGetLevel(&sessionSettings);
    SetDefDispEnv(screenCardDispEnvs, 320, 0, 320, 240);
    SetDefDrawEnv(screenCardDrawEnvs, 0, 0, 320, 240);
    screenCardDrawEnv0Isbg = 1;
    screenCardDrawEnv0Dtd = 0;
    SetDefDispEnv(screenCardDispEnvs + 20, 0, 0, 320, 240);
    SetDefDrawEnv(screenCardDrawEnvs + 92, 320, 0, 320, 240);
    screenCardDrawEnv1Isbg = 1;
    screenCardDrawEnv1Dtd = 0;
    screenCardHalfSize(&w, &h);
    for (i = 0; i < sessionGetOpponents(&sessionSettings); i++) {
        SetupQuad(&screenCardQuads[i]);
        screenCardQuads[i].x0 = w;
        screenCardQuads[i].y0 = -h;
        screenCardQuads[i].x1 = w;
        screenCardQuads[i].y1 = h;
        screenCardQuads[i].x2 = -w;
        screenCardQuads[i].y2 = -h;
        screenCardQuads[i].x3 = -w;
        screenCardQuads[i].y3 = h;
        if ((i & 3) == 0) {
            page++;
            v = 0;
        }
        top = v;
        screenCardQuads[i].v0 = v;
        v += 64;
        screenCardQuads[i].u0 = 0x7F;
        screenCardQuads[i].u1 = 0x7F;
        screenCardQuads[i].v1 = top + 63;
        screenCardQuads[i].u2 = 0;
        screenCardQuads[i].u3 = 0;
        screenCardQuads[i].v2 = top;
        screenCardQuads[i].v3 = top + 63;
        screenCardQuads[i].tpage = page + 128;
        screenCardQuads[i].clut = (((i + 128) & 0x3FF) << 6) | 0x30;
    }
    SetupQuad(&screenCardQuads[9]);
}

/* ---- two-player result screen ---- */
extern u8 *soundManagerObject;
extern u8 *musicManagerObject;
extern s32 audioCdVolume, audioVolumeClampMax, audioVolumeClampMin;
#pragma tm2_absolute audioCdVolume
#pragma tm2_absolute audioVolumeClampMax
#pragma tm2_absolute audioVolumeClampMin
extern char strYouWin[];  /* "YOU WIN" */
extern char strYouLose[];  /* "YOU LOSE" */
extern char strWinsLosses[];  /* "WINS LOSSES" (one space; strWinsLossesWide is the three-space one) */
extern char strWinsLossesFmt[];  /* "%d    %d" */
extern char textFormatBuffer[];
extern s32 viewGetSplitMode(void);
extern void SsSetSerialVol(char s_num, short voll, short volr);
extern void screenLostALife(void);
#include "semantic/hud.h"
extern int sprintf(char *, const char *, ...);

/* Clamp a serial (CD audio) volume to [audioVolumeClampMin, audioVolumeClampMax] and apply it. */
static __inline__ void SetSerialVolume(s32 v)
{
    audioCdVolume = v < audioVolumeClampMax ? v : audioVolumeClampMax;
    audioCdVolume = v < audioVolumeClampMin ? audioVolumeClampMin : v;
    SsSetSerialVol(0, (s16)audioCdVolume, (s16)audioCdVolume);
}

/* @8012B2D4: split-screen two-player result: tell the sound object (method
   3, arg 2), mute CD audio, print "YOU WIN" on the winning half (player 1
   if won == 1) and "YOU LOSE" on the other, present a frame, then each
   half's wins/losses tally (mod 100) from its own side. Layout is
   side-by-side or stacked depending on viewGetSplitMode. */
void __attribute__((section(".text_8012B2D4"))) screenDeathmatchResult(s32 unused, s16 won, s16 wins, s16 losses)
{
    TM2VEntry *e;
    s16 x1, y1, x2, y2;
    s16 step = 20;
    s16 w, l;

    if (viewGetSplitMode() == 1) {
        x1 = 79;
        y1 = 110;
        x2 = 239;
        y2 = 110;
    } else {
        x1 = 159;
        y1 = 40;
        x2 = 159;
        y2 = 160;
    }
    e = *(TM2VEntry **)(musicManagerObject + 8) + 3;
    e->fn(musicManagerObject + e->delta, 2);
    SetSerialVolume(0);
    screenLostALife();
    if (won == 1) {
        fontPrintCenteredXY(6, x1, y1, strYouWin);
        fontPrintCenteredXY(6, x2, y2, strYouLose);
    } else {
        fontPrintCenteredXY(6, x2, y2, strYouWin);
        fontPrintCenteredXY(6, x1, y1, strYouLose);
    }
    e = *(TM2VEntry **)(soundManagerObject + 200) + 1;
    e->fn(soundManagerObject + e->delta, 28, 0, 0, 99, -1, 0);
    taskMgrRunFrame(soundManagerObject);
    e = *(TM2VEntry **)(soundManagerObject + 200) + 2;
    e->fn(soundManagerObject + e->delta);
    fontPrintCenteredXY(0, x1, y1 + step * 2, strWinsLosses);
    w = wins % 100;
    l = losses % 100;
    sprintf(textFormatBuffer, strWinsLossesFmt, w, l);
    fontPrintCenteredXY(0, x1, y1 + step * 3, textFormatBuffer);
    fontPrintCenteredXY(0, x2, y2 + step * 2, strWinsLosses);
    sprintf(textFormatBuffer, strWinsLossesFmt, l, w);
    fontPrintCenteredXY(0, x2, y2 + step * 3, textFormatBuffer);
}
