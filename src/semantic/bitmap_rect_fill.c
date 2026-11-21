typedef signed int s32;

extern s32 bitmapTrackingEnabled;  /* VRAM-usage tracking enabled */
extern void bitmapMarkUsed(s32 row, s32 column);

/* Mark a width x height VRAM rectangle at (x, y) as used in the usage bitmap. */
void __attribute__((section(".text_801215F8"))) bitmapMarkRect(s32 x, s32 y, s32 width, s32 height)
{
    s32 row, column;

    if (!bitmapTrackingEnabled)
        return;
    for (row = y; row < y + height; row++)
        for (column = x; column < x + width; column++)
            bitmapMarkUsed(row, column);
}
