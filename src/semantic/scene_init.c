/* Game helpers (8011..8012 range). Names stay address-based. */
/* The car arrays are declared below through the block views (TM2HumanRecord /
 * TM2AIRecord), unsized as before. */
#define TM2_OWN_CAR_ARRAYS
#include "semantic/cars.h"
#include "semantic/settings.h"
#include "semantic/scene.h"
#ifdef TM2_PORT_HAZARDS
#include "semantic/port_hazards.h"   /* request 38 (ARENA_SWITCHES.md) */
#endif
#ifdef TM2_PORT_BALANCE
#include "semantic/port_balance.h"   /* request 40 (BALANCE.md) */
#endif
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 42 item 1: TM2_DT_BOSS_SLOT (roundStart) */
#include "semantic/port_rollback.h"   /* request 48: SEED (RB_SEED) */

/* Tournament password glyph table (passwordBuildString). */
#pragma tm2_rdata .tm2_800CFB20

typedef struct { s16 x, y, w, h; } TM2Rect;
typedef struct {
    u32 mode;
    s16 px, py;
    u16 pw, ph;
    u32 *pixel;
    s16 cx, cy;
    u16 cw, ch;
    u32 *clut;
} TM2TimInfo;

extern struct TM2AIRecord aiCars[];
extern struct TM2HumanRecord humanCars[];
extern s32 roundWonFlag, roundOverFlag, viewWatchingAi, viewWatchedAiCar;
extern s32 demoRoundsPlayed, demoWatchTimer, demoRespawnTimer, demoFollowPlayerCamera;
extern u8 gameAttractDemoMode;
extern u8 demoSpawnSlotsUsed[];
extern s32 aiCarTotal, aiCarsRemaining, aiBossDefeatedFlag;

extern s32 GetCurTics(void);
extern void srand(u32);
extern s32 rand(void);
extern void carFreezeResetAll(void);
extern void debrisClear(void);
extern s32 gameGetLevel(void);
extern void aiRoutePatchBossLevel(void);
extern void humanCarsRestart(void);
extern void weaponsRoundReset(void);
extern void carSetViewMode(struct TM2HumanRecord *, u32);
extern void demoRespawnAiCars(void);
extern void demoWatchNextAiCar(void);

/* @8011A024: race start: reseed rand, reset race state, init both human
   slots; in a multi-car race pick a random AI leader and activate every AI car. */
void __attribute__((section(".text_8011A024"))) roundStart(void)
{
    s32 i;

#ifdef TM2_PORT_HAZARDS
    portArenaLatch();   /* request 38 (ARENA_SWITCHES.md): the arena items off this round, before the srand */
#endif
#ifdef TM2_PORT_BALANCE
    portBalanceLatch();     /* request 40 (BALANCE.md): the balance rules of this round, before the srand */
#endif
    roundWonFlag = 0;
    roundOverFlag = 0;
    srand(RB_SEED(TM2_RB_SITE_ROUND));   /* request 48 SEED: the port's seed; retail GetCurTics() */
    carFreezeResetAll();
    debrisClear();
#ifdef TM2_PORT_ROLLBACK
    portRoundCleanProjectiles();   /* request 60 ROUND_CLEAN (ONLINE_ROLLBACK.md 21): no record of the round before, before the restarts' projectilesReset */
#endif
    if (gameGetLevel() == 8 && aiCars[0].type == 0xF0 && TM2_DT_BOSS_SLOT(aiCars[0].slot))
        aiRoutePatchBossLevel();
    humanCarsRestart();
    weaponsRoundReset();
    carSetResetWeapons(0, 0);
    carSetResetWeapons(0, 1);
#if TM2_MAX_HUMANS > 2
    for (i = 2; i < TM2_MAX_HUMANS; i++)    /* port variant: players 3-4 too */
        carSetResetWeapons(0, i);
#endif
    if (gameAttractDemoMode) {
        demoRoundsPlayed = 0;
        demoWatchTimer = 0;
        demoRespawnTimer = 0;
        demoFollowPlayerCamera = 0;
        viewWatchingAi = 1;
        viewWatchedAiCar = rand() % aiCarTotal;
        carSetViewMode(humanCars, 7);
        for (i = 0; i < 4; i++)
            demoSpawnSlotsUsed[i] = 0;
        demoRespawnAiCars();
        demoWatchNextAiCar();
        for (i = 0; i < aiCarTotal; i++)
            aiCars[i].f001 = 1;
    }
}

extern struct TM2Object *carSlotObjects[];
extern struct TM2Object *humanCarObjects[];
extern s32 humanCarObjectSlots[];
extern s32 carSlotIndices[];
extern s32 humanCarTypes[];
extern s32 carSlotOwners[];
extern u8 carResetWeapons[];
extern s32 carSlotKinds[];
extern void carBindHumanSlotStub(void *);
extern void csSetDrawEnable(void *, u8 v);

/* @8011A3FC: bind human slot `slot` to car entry `id` (human analogue of
   carBindAiSlot): cross-link the tables, initialise the record from
   carSlotOwners[id], mark the entry taken and tag its object with TM2_HUMAN_ID(slot) (slot + 1 up to 7 humans). */
void __attribute__((section(".text_8011A3FC"))) carBindHumanSlot(s32 slot, s32 id, s32 c)
{
    struct TM2HumanRecord *rec = &humanCars[slot];

    humanCarObjects[slot] = carSlotObjects[id];
    humanCarObjectSlots[slot] = id;
    carSlotIndices[id] = slot;
    humanCars[slot].slot = id;
    humanCarTypes[slot] = c;
    humanCars[slot].player = slot;
    carBindHumanSlotStub(rec);
    carRestart(rec, carSlotOwners[id], carResetWeapons[slot]);
    if (!gameAttractDemoMode)
        carSetViewMode(rec, 2);
    carSlotKinds[id] = 0;
    carSlotObjects[id]->id = TM2_HUMAN_ID(slot);
    humanCars[slot].player = slot;
    csSetDrawEnable(carSlotObjects[id], 1);
}

extern void carStatsDarkToothStage2(struct TM2CarStatsBlock *);
extern void rtScheduleEvent(s32, s32);
#ifdef TM2_PORT_DARK_TOOTH
#include "semantic/car_groups.h"
#include "semantic/car_specials.h"   /* request 33-2e (CAR_GROUPS.md 11) */
#endif

/* @8011C248: count the AI cars still racing (stats.hp > 0) into aiCarsRemaining;
   when none are left in race mode 8 with the lead car in state 0xF0, finish
   the race once (aiBossDefeatedFlag) and clamp the lead car's position. */
#ifdef TM2_PORT_DARK_TOOTH
/* request 33-2e (CAR_GROUPS.md 11): retail's rule (level 8, aiCars[0]) on aiCars[0]'s own stage flag (its key
   selected); with portCarOptions bit 6 first the per-car stage loop of every other Dark Tooth (SURVIVAL.md 1.6:
   a Dark Tooth waiting for its stage 2 counts as remaining) */
void __attribute__((section(".text_8011C248"))) aiCarsCountRemaining(void)
{
    s32 i, special;

    portCarDarkToothParked();   /* request 46 (PLAYTEST_ANSWERS.md 7): a parked Dark Tooth's head hidden with his car */
    aiCarsRemaining = 0;
    for (i = 0; i < aiCarTotal; i++)
        if (aiCars[i].stats.hp > 0)
            aiCarsRemaining++;
    if (TM2_PORT_BOSS_ANYWHERE)
        aiCarsRemaining += portCarDarkToothStages();
    if (aiCarsRemaining > 0)
        return;
    if (gameGetLevel() != 8)
        return;
    if (aiCars[0].type != 0xF0 || TM2_DT_ORDINARY(aiCars[0].slot))   /* request 42 item 1: no stage 2 for him */
        return;
    special = portCarSpecialSelect(TM2_CAR_KEY(aiCars[0].slot, aiCars[0].type));
    if (aiBossDefeatedFlag != 0)
        goto done;
    aiCarsRemaining = 1;
    if (aiCars[0].stats.wreck_timer <= 0) {
        aiBossDefeatedFlag = 1;
        carStatsDarkToothStage2(&aiCars[0].stats);
        carBuildCollisionBox(&aiCars[0], 0);
        aiCars[0].state.dead = 0;
        roundOverFlag = 0;
        rtScheduleEvent(3, 0);
        if (aiCars[0].motion.pos[1] < -13680) {
            aiCars[0].motion.pos[1] = -12800;
            aiCars[0].motion.pos[2] = 480;
        }
        if (aiCars[0].motion.pos[0] > 9600) {
            aiCars[0].motion.pos[0] = 9600;
            aiCars[0].motion.pos[2] = 480;
        } else if (aiCars[0].motion.pos[0] < -7200) {
            aiCars[0].motion.pos[0] = -9600;
            aiCars[0].motion.pos[2] = 480;
        } else if (aiCars[0].motion.pos[0] < -4800) {
            aiCars[0].motion.pos[0] = -4800;
            aiCars[0].motion.pos[2] = 480;
        }
    } else if (aiCars[0].stats.wreck_timer >= 121)
        carSpinOut(&aiCars[0], 0, 12, 0);
done:
    portCarSpecialRestore(special);
}
#else
void __attribute__((section(".text_8011C248"))) aiCarsCountRemaining(void)
{
    s32 i;

    aiCarsRemaining = 0;
    for (i = 0; i < aiCarTotal; i++)
        if (aiCars[i].stats.hp > 0)
            aiCarsRemaining++;
    if (aiCarsRemaining > 0)
        return;
    if (gameGetLevel() != 8)
        return;
    if (aiCars[0].type != 0xF0)
        return;
    if (aiBossDefeatedFlag != 0)
        return;
    aiCarsRemaining = 1;
    if (aiCars[0].stats.wreck_timer <= 0) {
        aiBossDefeatedFlag = 1;
        carStatsDarkToothStage2(&aiCars[0].stats);
        carBuildCollisionBox(&aiCars[0], 0);
        aiCars[0].state.dead = 0;
        roundOverFlag = 0;
        rtScheduleEvent(3, 0);
        if (aiCars[0].motion.pos[1] < -13680) {
            aiCars[0].motion.pos[1] = -12800;
            aiCars[0].motion.pos[2] = 480;
        }
        if (aiCars[0].motion.pos[0] > 9600) {
            aiCars[0].motion.pos[0] = 9600;
            aiCars[0].motion.pos[2] = 480;
        } else if (aiCars[0].motion.pos[0] < -7200) {
            aiCars[0].motion.pos[0] = -9600;
            aiCars[0].motion.pos[2] = 480;
        } else if (aiCars[0].motion.pos[0] < -4800) {
            aiCars[0].motion.pos[0] = -4800;
            aiCars[0].motion.pos[2] = 480;
        }
    } else if (aiCars[0].stats.wreck_timer >= 121)
        carSpinOut(&aiCars[0], 0, 12, 0);
}
#endif /* TM2_PORT_DARK_TOOTH */

typedef struct { s32 p0[3]; s32 x; s32 y; } TM2VramSlot;
extern TM2VramSlot *vramTexSlots;
extern s32 bitmapIsFreeXY(s32, s32);
extern int LoadImage(TM2Rect *, void *);
extern int DrawSync(s32);
#if defined(TM2_PORT_TEXTURE_BANKS) || defined(TM2_PORT_DARK_TOOTH) || defined(TM2_PORT_CAR_CLUTS)
#include "semantic/tex_banks.h"   /* request 32 step 4; car_cluts.h: 33-2e's fallback into rows 240..255, 33-1's bit 9 */
#endif
#ifdef TM2_PORT_CAR_CLUTS
/* request 33-1 follow-up (CAR_GROUPS.md 13.12, RETAIL_BUGS.md 27): with portCarOptions bit 9 latched the spot test
   covers every texel the CLUT takes (portClutSpotFree); bit 9 clear: retail's top-left test */
#define CLUT_SPOT_FREE(x, y, w) portClutSpotFree(x, y, w)
#else
#define CLUT_SPOT_FREE(x, y, w) bitmapIsFreeXY(x, y)
#endif

/* @80120D3C: find a free w x h VRAM area (scanning 256x256 pages, 4 across,
   2 down), reserve it, record it in slot vramTexSlots[idx] and
   upload img there. Returns 1 if found.
   The flags are set after the upload: sched1 hoists `found = 1` above the calls,
   and its later luid puts it after the `&r` argument as in retail. */
s32 __attribute__((section(".text_80120D3C"))) vramUploadClut(s32 w, s32 h, void *img, s32 col, s32 row)
{
    TM2Rect r;
    s32 found = 0;
    s32 result = 0;
    s32 idx = row * TM2_TEX_SLOT_STRIDE + col;   /* 20; 32 with TM2_PORT_CARS (players.h) */
    s32 px, py, x, y;
#ifdef TM2_PORT_TEXTURE_BANKS
    /* request 32 step 4 (TEXTURE_BANKS.md "As built"): a banked group's CLUT goes to its bank (retail's
       scan, whole rectangles tested in the bank); 0 = VRAM, as retail */
    s32 bank = portTexBankOfGroup(row);
#endif

#ifdef TM2_PORT_DARK_TOOTH
    for (py = 0; py < 512 && !found; py += 256) {
        /* request 33-2e's portCarOptions bit 6 (Dark Tooth anywhere; CAR_GROUPS.md 13): when the upper half
           has no room, a spot in rows 240..255 (the allocator they share with 33-1's CLUT copies) before the
           lower half, where only the top-left texel would be tested */
#ifdef TM2_PORT_TEXTURE_BANKS
        if (py == 256 && portClutRowsUpload(w, h, img, idx, bank)) {
#else
        if (py == 256 && portClutRowsUpload(w, h, img, idx, 0)) {
#endif
            result = 1;
            found = 1;
        }
        for (px = 0; px < 1024 && !found; px += 256)
#else
    for (py = 0; py < 512 && !found; py += 256)
        for (px = 0; px < 1024 && !found; px += 256)
#endif
            for (y = py; y < py + 256 && !found; y += h)
                for (x = px; x < px + 256 && !found; x += w)
#ifdef TM2_PORT_TEXTURE_BANKS
                    if (bank ? portTexBankIsFree(bank, x, y, w, h) : CLUT_SPOT_FREE(x, y, w)) {
                        if (bank)
                            portTexBankMark(bank, x, y, w, h);
                        else
                            bitmapMarkRect(x, y, w, h);
#else
                    if (CLUT_SPOT_FREE(x, y, w)) {
                        bitmapMarkRect(x, y, w, h);
#endif
                        vramTexSlots[idx].x = x;
                        vramTexSlots[idx].y = y;
                        r.x = x;
                        r.y = y;
                        r.w = w;
                        r.h = h;
#ifdef TM2_PORT_TEXTURE_BANKS
                        portVramUploadBank = bank;   /* the port's GPU: this upload goes to the bank */
#endif
                        LoadImage(&r, img);
                        DrawSync(0);
#ifdef TM2_PORT_TEXTURE_BANKS
                        portVramUploadBank = 0;
#endif
                        result = 1;
                        found = 1;
                    }
#ifdef TM2_PORT_DARK_TOOTH
    }
#endif
#ifdef TM2_PORT_CAR_CLUTS
    /* bit 9: no whole free spot in either half: rows 240..255 as the last resort (portClutRowsLast; 0 = none
       there either, retail's failure: vramPlaceTexture prints "Relocatable texture not loaded") */
#ifdef TM2_PORT_TEXTURE_BANKS
    if (!found && portClutRowsLast(w, h, img, idx, bank))
#else
    if (!found && portClutRowsLast(w, h, img, idx, 0))
#endif
        result = 1;
#endif
    return result;
}

typedef struct {
    s32 f0;
    void *f4;
    u16 h8;
    u16 hA;
    s16 hC;
    s16 hE;
    s32 f10;
    u8 *f14;
    s32 f18, f1C, f20;
    s32 f24, f28, f2C;
} TM2ViewObj;
extern u8 *csAlloc(void);
extern s32 lightGetEnv(s32);
extern void csSetModel(void *, void *);
extern void *RotMatrixYXZ(void *, void *);
extern void viewProc(s32);
extern s32 abs(s32);
extern void viewSetFixedCamera(s32, s32, s32);
extern void renderDrawModel(s32, void *);

/* @80122124: draw a scene object: create its instance on first use, copy
   position/rotation, build its matrix, then either run the view or set the
   colour/scale and draw it into ot. */
void __attribute__((section(".text_80122124"))) sceneObjDraw(TM2ViewObj *p, s32 ot)
{
    if (p->f4 && p->h8) {
        if (!p->f14) {
            p->f14 = csAlloc();
            *(s32 *)(p->f14 + 4) = lightGetEnv(0);
            csSetModel(p->f14, p->f4);
        }
        *(s32 *)(p->f14 + 0xA8) = p->f18;
        *(s32 *)(p->f14 + 0xAC) = p->f1C;
        *(s32 *)(p->f14 + 0xB0) = p->f20;
        *(s16 *)(p->f14 + 0xA0) = p->f28;
        *(s16 *)(p->f14 + 0xA2) = p->f2C;
        *(s16 *)(p->f14 + 0xA4) = p->f24;
        RotMatrixYXZ(p->f14 + 0xA0, p->f14 + 0x80);
#ifdef TM2_PORT_CAR_NUMBERS
        if (p->f4 == (void *)portCarSel.node[0])
            portCarSelDraw(p->f4);      /* request 42 item 5: the extended carousel (the port loads a mod car) */
#endif
        if (p->hA)
            viewProc(0);
        else {
            if (p->hC != -1)
                *(s32 *)p->f14 = p->hC;
            if (p->f10 < 0)
                viewSetFixedCamera(0, 8 / abs(p->f10), 1);
            else
                viewSetFixedCamera(0, p->f10 << 3, 1);
        }
        renderDrawModel(ot, p->f14);
    }
}

/* struct TM2ShellScene: include/semantic/scene.h. */
extern s32 renderGetView0Frames(void);
extern s32 renderGetView1Frames(void);
extern void viewInit(void);
extern void csPoolInit(void);
extern void viewCreate(s32, s32, s32);
extern void lightSetEnvColor(s32, s32, s16 r, s16 g, s16 b);
extern void lightSetEnvDirection(s32, s32, s16 az, s16 el);
extern void lightSetEnvAmbient(s32, s16 r, s16 g, s16 b);

/* @801222F0: shell scene init: grab the two buffers, reset the camera,
   create the view (mode 7 if n else 1), set three white lights and ambient,
   clear the scene state and install the back/cancel/idle handlers. */
void __attribute__((section(".text_801222F0"))) sceneInit(struct TM2ShellScene *s, void *back, void *cancel, void *idle, s32 n)
{
    s->f0 = renderGetView0Frames();
    s->f4 = s->f0;
    s->f8 = renderGetView1Frames();
    s->fC = s->f8;
    s->cam_pos[0] = 0;
    s->cam_pos[1] = 0;
    s->cam_pos[2] = 0;
    s->cam_rot[0] = 0;
    s->cam_rot[1] = 0;
    s->cam_rot[2] = 0;
    s->f24 = n;
    viewInit();
    csPoolInit();
    if (s->f24)
        viewCreate(s->f0, 7, 0);
    else
        viewCreate(s->f0, 1, 0);
    lightSetEnvColor(0, 0, 255, 255, 255);
    lightSetEnvColor(0, 1, 255, 255, 255);
    lightSetEnvColor(0, 2, 255, 255, 255);
    lightSetEnvDirection(0, 0, 200, 30);
    lightSetEnvDirection(0, 1, 200, 30);
    lightSetEnvDirection(0, 2, 200, 30);
    lightSetEnvAmbient(0, 32, 32, 32);
    s->done = 0;
    s->exit_code = 0;
    s->selection = 0;
    s->background = 0;
    s->f3C = 0;
    s->f60 = 0;
    s->item_count = 0;
    s->f128 = 0;
    s->running = 0;
    s->events = 0;
    s->fD68 = 0;
    s->f28 = 1;
    s->back = back;
    s->cancel = cancel;
    s->idle = idle;
    s->fD74 = 0;
    s->fD78 = 0;
    s->fD6C = 60;
}


extern char textFormatBuffer[];
extern char passwordNumberFmt[];
extern s32 sessionGetLevel(struct TM2Settings *);
extern s32 sessionGetDarkToothStage(struct TM2Settings *);
extern u32 passwordEncode(s32 first, s32 second, u8 flag, s32 arg);
extern s32 sprintf(char *, const char *, ...);

/* @801292A4: build the tournament password into textFormatBuffer: code from the
   settings, base-5 digits as "%06d", then digits 0-4 become glyphs _ < @ > =. */
void __attribute__((section(".text_801292A4"))) passwordBuildString(void)
{
    s32 i;

    sprintf(textFormatBuffer, passwordNumberFmt,
            /* retail narrows the encoder's level/car arguments and its result to 16 bits */
            passwordRebase((u16)passwordEncode((u16)sessionGetLevel(&sessionSettings), (u16)sessionGetHumanCar(&sessionSettings, 0),
                                      sessionGetDarkToothStage(&sessionSettings), 0), 10, 5));
    for (i = 0; i < 7; i++) {
        switch (textFormatBuffer[i]) {
        case '0': textFormatBuffer[i] = '_'; break;
        case '1': textFormatBuffer[i] = '<'; break;
        case '2': textFormatBuffer[i] = '@'; break;
        case '3': textFormatBuffer[i] = '>'; break;
        case '4': textFormatBuffer[i] = '='; break;
        }
    }
}

typedef struct {
    u8 p0[0xE];
    u16 hE;
    u8 p10[6];
    u16 h16;
    u8 p18[4];
    u8 b1C, b1D;
    u8 p1E[0xA];
} TM2CardImg;
extern u8 *fileioWorkBuffer2;
#pragma tm2_absolute fileioWorkBuffer2
extern TM2CardImg screenCardQuads[];
extern char strCardTimFmtSSS[];
extern char screenDirPrefix[], screenPlateSuffix[];
extern s32 sessionGetOpponents(struct TM2Settings *);
extern TM2CarNumber sessionGetOpponentCar(struct TM2Settings *, s32);
extern s32 fileioLoadFileIntoRam(void *, char *);
extern void GsGetTimInfo(void *, TM2TimInfo *);

/* @8012A614: for every card in the current setting, load its TIM file and
   upload the image (and CLUT) into the card's VRAM position. */
void __attribute__((section(".text_8012A614"))) screenLoadCardImages(void)
{
    u8 *buf = fileioWorkBuffer2;
    s16 i;
    TM2Rect r;
    TM2TimInfo tim;
    struct { s32 y, x; } pos;
    s32 tx, ty;
    TM2CardImg *e;

    for (i = 0; i < sessionGetOpponents(&sessionSettings); i++) {
        sprintf(textFormatBuffer, strCardTimFmtSSS, screenDirPrefix,
                carFileCode(&sessionSettings, sessionGetOpponentCar(&sessionSettings, i)), screenPlateSuffix);
        fileioLoadFileIntoRam(buf, textFormatBuffer);
        e = &screenCardQuads[i];
        {   /* block-scoped copies: keeps them local (v0/v1) instead of sharing tx/ty */
            s32 ty2 = (e->h16 << 4) & 0x100;
            s32 tx2 = (e->h16 << 6) & 0x3FF;
            pos.y = ty2;
            pos.x = tx2;
        }
        GsGetTimInfo(buf + 4, &tim);
        r.x = (e->b1C >> 1) + pos.x;
        r.y = e->b1D + pos.y;
        r.w = tim.pw;
        r.h = tim.ph;
        LoadImage(&r, tim.pixel);
        if ((tim.mode >> 3) & 1) {
            tx = (e->hE & 0x3F) << 4;
            ty = e->hE >> 6;
            pos.x = tx;
            pos.y = ty;
            r.x = tx;
            r.y = ty;
            r.w = tim.cw;
            r.h = tim.ch;
            LoadImage(&r, tim.clut);
        }
    }
}

extern u8 *fileioWorkBuffer;
#pragma tm2_absolute fileioWorkBuffer
extern s16 screenCardBufferIndex;
extern char strCardTimFmtSSD[], screenShowOpsName[];
#include "semantic/input.h"
extern s32 screenDisplayImage(s32, void *, s32, s32);
extern void screenDrawCard(s32, s32, s32);
extern void screenCardSettle(s32);

/* @8012A7B8: show the card-table screen: load the background for the
   current setting, then draw every card, highlighting every 11th draw. */
void __attribute__((section(".text_8012A7B8"))) screenDealCards(void)
{
    s32 i, t;

    padUpdate();
    sprintf(textFormatBuffer, strCardTimFmtSSD, screenDirPrefix, screenShowOpsName, sessionGetLevel(&sessionSettings));
    t = 0;
    i = -1;
    fileioLoadFileIntoRam(fileioWorkBuffer, textFormatBuffer);
    screenDisplayImage(screenCardBufferIndex, fileioWorkBuffer, 0, 0);
    while (i < sessionGetOpponents(&sessionSettings) + 1) {
        if (sessionGetOpponents(&sessionSettings) - 1 < i) {
            screenDrawCard(sessionGetOpponents(&sessionSettings), 0, 1);
            i++;
        } else {
            if (t % 11 == 0) {
                screenDrawCard(i, 1, 0);
                screenCardSettle(i);
                i++;
            } else
                screenDrawCard(i, 0, 0);
            t++;
        }
    }
}
