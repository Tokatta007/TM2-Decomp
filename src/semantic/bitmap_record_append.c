#include "semantic/players.h"   /* TM2_TEX_SLOT_STRIDE */
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;

typedef struct BitmapImage {
    u32 mode;
    s16 x;
    s16 y;
    s32 unused08;
    s32 unused0C;
    s16 width;
    s16 height;
} BitmapImage;

typedef struct BitmapRecord {
    s32 key;
    s32 page;
    s32 x;
    s32 y;
    s32 width;
    s32 height;
} BitmapRecord;

typedef struct BitmapPreset {
    s32 page;
    s32 x;
    s32 y;
    s32 width;
    s32 height;
} BitmapPreset;

extern s32 bitmapTrackingEnabled;
extern BitmapPreset *vramTexSlots;
extern BitmapRecord *bitmapRecords;
extern s32 *bitmapKeyTable;
extern s32 bitmapRecordCount;
extern s32 vramTexSlotGroup;
extern unsigned short GetTPage(u32 mode, s32 abr, s32 x, s32 y);

/* @80121764: append a bitmap record {key, page, x, y, width, height}, taken
 * from the preset table or computed from the TIM image. Falls off the end:
 * $v0 is left holding the new record count, or vramTexSlotGroup when appending is
 * disabled. */
s32 __attribute__((section(".text_80121764")))
bitmapAddRecord(BitmapImage *image, s32 index, s32 use_preset)
{
    BitmapRecord *record;
    s32 slot = vramTexSlotGroup * TM2_TEX_SLOT_STRIDE + index;   /* 20; 32 with TM2_PORT_CARS (players.h) */

    if (bitmapTrackingEnabled != 0) {
        record = &bitmapRecords[bitmapRecordCount];
        record->key = bitmapKeyTable[index];
        if (use_preset != 0) {
            record->page = vramTexSlots[slot].page;
            record->x = vramTexSlots[slot].x;
            record->y = vramTexSlots[slot].y;
            record->width = vramTexSlots[slot].width;
            record->height = vramTexSlots[slot].height;
        } else {
            record->page = GetTPage(image->mode, 0, image->x, image->y) & 0x1f;
            record->x = image->x;
            record->y = image->y;
            record->width = image->width;
            record->height = image->height;
        }
        bitmapRecordCount++;
    }
}
