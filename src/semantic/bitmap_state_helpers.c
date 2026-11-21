typedef signed int s32;
typedef unsigned int u32;

/* VRAM-usage bitmap (1024 bits per row) and loaded-image table helpers. */
extern s32 bitmapTrackingEnabled;   /* bitmap tracking enabled */
extern u32 *bitmapWords;  /* bitmap words */
extern s32 bitmapKeyTable;
extern s32 *bitmapRecords;  /* image records, 6 words each */
extern s32 bitmapRecordCount;   /* image record count */

/* Disable VRAM-usage tracking. */
void __attribute__((section(".text_801215EC"))) bitmapDisableTracking(void)
{
    bitmapTrackingEnabled = 0;
}

s32 bitmapIsFree(s32 row, s32 column);

/* Is VRAM cell (x, y) free? */
s32 __attribute__((section(".text_801216A8"))) bitmapIsFreeXY(s32 x, s32 y)
{
    return bitmapIsFree(y, x);
}

/* Is the bitmap bit for (row, column) clear? */
s32 __attribute__((section(".text_801216D0"))) bitmapIsFree(s32 row, s32 column)
{
    s32 bit = (row << 10) + column;
    s32 sh = bit % 32;      /* remainder first, then shift the same variable */
    bit >>= 5;
    return (bitmapWords[bit] & (1 << sh)) == 0;
}

/* Mark the bitmap bit for (row, column) as used. */
void __attribute__((section(".text_80121718"))) bitmapMarkUsed(s32 row, s32 column)
{
    s32 bit = (row << 10) + column;
    s32 sh = bit % 32;
    bit >>= 5;
    bitmapWords[bit] |= 1 << sh;
}

/* Set bitmapKeyTable. */
void __attribute__((section(".text_801218C0"))) bitmapSetKeyTable(s32 value)
{
    bitmapKeyTable = value;
}

/* Find the image record whose first word equals value; 0 if none. */
s32 *__attribute__((section(".text_801218CC"))) bitmapFindRecord(s32 value)
{
    s32 i;
    s32 *rec;
    s32 pad[1];

    for (i = 0; i < bitmapRecordCount; i++) {
        rec = &bitmapRecords[i * 6];
        if (*rec == value)
            return rec;
    }
    return 0;
}
