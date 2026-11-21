typedef signed int s32;
typedef unsigned char u8;

extern s32 *modelRelocateStack;
extern s32 *bitmapWords;
extern s32 *vramTexSlots;
extern s32 *bitmapRecords;
extern s32 renderGetSharedBuffer(void);
extern s32 *renderGetPacketBuffer(void);
extern s32 *renderGetOrderingTable(void);
#ifdef TM2_PORT_CAR_DROPIN
#include "semantic/car_dropin.h"
#endif

/* @801214FC: the load's texture bookkeeping: the VRAM usage bitmap in packet buffer 0, the texture-slot table
   after it, the image records in an ordering table (the first frames overwrite them); the store through
   modelRelocateStack is retail's (the pointer is 0: RETAIL_BUGS.md). With TM2_PORT_CAR_DROPIN (request 33,
   CAR_GROUPS.md 14) the three live in port storage instead (portCarLiveStorage), so a car can be loaded mid-match. */
void __attribute__((section(".text_801214FC"))) bitmapSetupStorage(void)
{
#ifdef TM2_PORT_CAR_DROPIN
    *modelRelocateStack = renderGetSharedBuffer();
    portCarLiveStorage();
#else
    s32 *bitmap;

    *modelRelocateStack = renderGetSharedBuffer();
    bitmap = renderGetPacketBuffer();
    bitmapWords = bitmap;
    vramTexSlots = (s32 *)((u8 *)bitmap + 0x10004);
    bitmapRecords = renderGetOrderingTable();
#endif
}
