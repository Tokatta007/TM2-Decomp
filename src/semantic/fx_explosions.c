/* Small game helpers (8011 range). Names stay address-based. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

/* AI car record (9 x 0x854 bytes at aiCars; TM2AIRecord in include/semantic/cars.h):
 * the fields aiCarShouldUpdate reads. */
typedef struct {
    u8 pad000[0xC];
    s16 f00C;           /* +0x00C */
    u8 pad00E[0x16];
    s16 f024;           /* +0x024 */
    u8 pad026[2];
    s32 target_dist;    /* +0x028 */
    u8 pad02C[0xD];
    u8 f039;            /* +0x039 */
    u8 pad03A[0x4C7];
    s8 wreck_state;     /* +0x501 stats.wreck_state: 3/4 wrecked */
    u8 pad502[0x352];
} TM2AICarUpdate;
typedef char tm2_ai_car_update_size[(sizeof(TM2AICarUpdate) == 0x854) ? 1 : -1];

typedef struct {
    s32 count;
    u8 *obj;
} TM2Timer8;

/* Collapsing track section (35 x 36 bytes at worldCollapseSections, see
 * world_collapse.c): only the six edge/link slots. */
typedef struct {
    u8 pad_00[0x1C];
    s8 slot[6];         /* +0x1C */
    u8 pad_22[2];
} TM2CollapseLinks;

extern s32 fxExplosionCooldown;
extern s32 fxBlastPoolIndex;
extern u8 gameAttractDemoMode;
extern u8 fxExplosionFrames[];
extern TM2Timer8 lightFlickerTimers[];
extern TM2CollapseLinks worldCollapseSections[];
extern u8 *fxBlastObjects[];
extern TM2AICarUpdate aiCars[];

extern s32 viewGetSplitMode(void);
#include "semantic/sound.h"
extern u32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern u8 *fxSpriteSpawn(s32, s32, s32, s32, u8 *, u16 count, u8 flag);
extern s32 rand(void);
extern s32 lightGetEnv(s32);
extern s32 objPoolGetStep(s32);
#include "semantic/world.h"
#include "semantic/port_balance.h"

/* request 102 item 3 (portBalance bit 30, BALANCE.md 23): an explosion far from the one that last sounded sounds
   too, inside the 6-step cooldown */
#ifdef TM2_PORT_BALANCE
#define BOOM_READY(pos) portBalanceBoomReady((int *)(pos), fxExplosionCooldown >= 6)
#else
#define BOOM_READY(pos) (fxExplosionCooldown >= 6)
#endif

/* @80111464: rumble kick (cmd 8, strength 75) when the cooldown has run, then spawn effect 6 */
void __attribute__((section(".text_80111464"))) fxExplosion6(s32 a, u8 flag)
{
    if (BOOM_READY(a)) {
        if (viewGetSplitMode() == 0) {
            soundLocateForView0(a);
            soundPlay(8, PadChkMtap(), soundGetListenerPan(), 75);
        } else {
            soundLocateForView(a, -1);
            soundPlay(8, PadChkMtap(), 0, 75);
        }
        fxExplosionCooldown = 0;
    }
    fxSpriteSpawn(a, 6, 2, 16, fxExplosionFrames, 1, flag);
}

/* @80111538: rumble kick (cmd 8, strength 75), then spawn effect 12 */
void __attribute__((section(".text_80111538"))) fxExplosion12(s32 a, u8 flag)
{
    if (BOOM_READY(a)) {
        if (viewGetSplitMode() == 0) {
            soundLocateForView0(a);
            soundPlay(8, PadChkMtap(), soundGetListenerPan(), 75);
        } else {
            soundLocateForView(a, -1);
            soundPlay(8, PadChkMtap(), 0, 75);
        }
        fxExplosionCooldown = 0;
    }
    fxSpriteSpawn(a, 12, 2, 16, fxExplosionFrames, 1, flag);
}

/* @8011160C: rumble kick (cmd 8, strength 99), then spawn effect 24 */
void __attribute__((section(".text_8011160C"))) fxExplosion24(s32 a, u8 flag)
{
    if (BOOM_READY(a)) {
        if (viewGetSplitMode() == 0) {
            soundLocateForView0(a);
            soundPlay(8, PadChkMtap(), soundGetListenerPan(), 99);
        } else {
            soundLocateForView(a, -1);
            soundPlay(8, PadChkMtap(), 0, 99);
        }
        fxExplosionCooldown = 0;
    }
    fxSpriteSpawn(a, 24, 2, 16, fxExplosionFrames, 1, flag);
}

/* @801116E0: rumble kick (cmd 7, strength 75), then spawn effect 48 */
void __attribute__((section(".text_801116E0"))) fxExplosion48(s32 a, u8 flag)
{
    if (BOOM_READY(a)) {
        if (viewGetSplitMode() == 0) {
            soundLocateForView0(a);
            soundPlay(7, PadChkMtap(), soundGetListenerPan(), 75);
        } else {
            soundLocateForView(a, -1);
            soundPlay(7, PadChkMtap(), 0, 75);
        }
        fxExplosionCooldown = 0;
    }
    fxSpriteSpawn(a, 48, 2, 16, fxExplosionFrames, 1, flag);
}

/* @801117B4: rumble kick (cmd 7, half port, strength 99), then spawn effect 96 */
void __attribute__((section(".text_801117B4"))) fxExplosion96(s32 a, u8 flag)
{
    if (BOOM_READY(a)) {
        if (viewGetSplitMode() == 0) {
            soundLocateForView0(a);
            soundPlay(7, PadChkMtap() >> 1, soundGetListenerPan(), 99);
        } else {
            soundLocateForView(a, -1);
            soundPlay(7, PadChkMtap() >> 1, 0, 99);
        }
        fxExplosionCooldown = 0;
    }
    fxSpriteSpawn(a, 96, 1, 16, fxExplosionFrames, 1, flag);
}

/* @80111888: rumble kick (cmd 7, half port, strength 99), then spawn effect 192 */
void __attribute__((section(".text_80111888"))) fxExplosion192(s32 a, u8 flag)
{
    if (BOOM_READY(a)) {
        if (viewGetSplitMode() == 0) {
            soundLocateForView0(a);
            soundPlay(7, PadChkMtap() >> 1, soundGetListenerPan(), 99);
        } else {
            soundLocateForView(a, -1);
            soundPlay(7, PadChkMtap() >> 1, 0, 99);
        }
        fxExplosionCooldown = 0;
    }
    fxSpriteSpawn(a, 192, 1, 16, fxExplosionFrames, 8, flag);
}

/* @80114D08: tick 10 light-flicker timers; while running, pick a random light env (1 in 3) */
void __attribute__((section(".text_80114D08"))) lightFlickerTick(void)
{
    s32 i;
    TM2Timer8 *t;
    s32 on;

    for (i = 0; i < 10; i++) {
        t = &lightFlickerTimers[i];
        if (t->count > 0) {
            --t->count;
            if (t->count && rand() % 3 == 0)
                on = 1;
            else
                on = 0;
            *(s32 *)(t->obj + 4) = lightGetEnv(on);
        }
    }
}

/* @80116F30: clear every reference to id from the 6 slots of all 35 records */
void __attribute__((section(".text_80116F30"))) worldCollapseUnlink(s32 id)
{
    s32 i;

    for (i = 0; i < 35; i++) {
        if (worldCollapseSections[i].slot[0] == id)
            worldCollapseSections[i].slot[0] = -1;
        if (worldCollapseSections[i].slot[1] == id)
            worldCollapseSections[i].slot[1] = -1;
        if (worldCollapseSections[i].slot[2] == id)
            worldCollapseSections[i].slot[2] = -1;
        if (worldCollapseSections[i].slot[3] == id)
            worldCollapseSections[i].slot[3] = -1;
        if (worldCollapseSections[i].slot[4] == id)
            worldCollapseSections[i].slot[4] = -1;
        if (worldCollapseSections[i].slot[5] == id)
            worldCollapseSections[i].slot[5] = -1;
    }
}

/* @80119468: place the next pooled object (slots 1..4, round-robin) at pos and play sound 0x2AE.
 * Plain array indexing (re-read because the stores may alias it) gives retail's
 * address register order; a pointer local swaps them. */
u8 __attribute__((section(".text_80119468"))) fxSpawnPooledBlast(s32 *pos)
{
    s32 r = 0;

    if (objPoolGetStep(fxBlastPoolIndex) == 0) {
        if (fxBlastObjects[fxBlastPoolIndex] != 0) {
            *(s32 *)(fxBlastObjects[fxBlastPoolIndex] + 4) = pos[0];
            *(s32 *)(fxBlastObjects[fxBlastPoolIndex] + 8) = pos[1];
            *(s32 *)(fxBlastObjects[fxBlastPoolIndex] + 12) = pos[2];
            objRegistrySetStep(0x2AE, fxBlastPoolIndex, 1);
            if (++fxBlastPoolIndex >= 5)
                fxBlastPoolIndex = 1;
            r = 1;
        }
    }
    return r;
}

/* @8011B7D8: car i is eligible (always in battle display mode; else alive, active, slow and not turning hard) */
u8 __attribute__((section(".text_8011B7D8"))) aiCarShouldUpdate(s32 i)
{
    u8 r = 1;
    s32 v;

    if (gameAttractDemoMode == 0) {
        if (aiCars[i].wreck_state == 4)
            goto no;
        if (aiCars[i].f00C != 0 && aiCars[i].f039 == 0) {
            if (aiCars[i].target_dist > 4800)
                goto no;
            v = aiCars[i].f024;
            if (v < 0)
                v = -v;
            if (v > 1024)
            no:
                r = 0;
        }
    }
    return r;
}
