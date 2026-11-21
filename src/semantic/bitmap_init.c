typedef signed int s32;

extern s32 *bitmapWords;
extern s32 bitmapTrackingEnabled;
extern s32 bitmapRecordCount;
extern void bitmapSetupStorage(void);
/* bitmapMarkRect is void (v0 at its return is left over from its last call), but retail
 * returns the last call's result here, so this unit keeps the int-returning view it was compiled with. */
extern s32 bitmapMarkRect(s32 x, s32 y, s32 width, s32 height);
#if defined(TM2_PORT_TEXTURE_BANKS) || defined(TM2_PORT_CAR_CLUTS) || defined(TM2_PORT_DARK_TOOTH)
#include "semantic/tex_banks.h"   /* request 32 step 4; car_cluts.h: request 33-1 and 33-2e's rows 240..255 */
#endif

s32 __attribute__((section(".text_80121550"))) bitmapInit(void)
{
    s32 i;
    s32 result;

#ifdef TM2_PORT_TEX_SHARED
    /* request 32 step 4 (TEXTURE_BANKS.md "As built"): every load starts here; latch portCarOptions bit 5
       for this load and free every bank (with TM2_PORT_CAR_CLUTS also the image bookkeeping of 33-1) */
    portTexBanksReset();
#endif
#ifdef TM2_PORT_CLUT_ROWS
    /* request 33-1 / 33-2e (CAR_GROUPS.md 13): latch bits 7 (CLUT copies) and 6 (Dark Tooth anywhere) for this
       load; both take VRAM rows 240..255 from one allocator, which the usage bitmap below keeps reserved */
    portCarClutsReset();
#endif
    bitmapSetupStorage();
    for (i = 0; i < 0x4000; i++) {
        bitmapWords[i] = 0;
    }
    bitmapTrackingEnabled = 1;
    bitmapMarkRect(0, 0, 640, 256);
    bitmapMarkRect(960, 256, 64, 32);
    bitmapMarkRect(960, 288, 64, 128);
    result = bitmapMarkRect(960, 416, 32, 96);
    bitmapRecordCount = 0;
    return result;
}
