typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;

typedef struct Rect { s16 x, y, w, h; } Rect;
typedef struct TimInfo {
    u32 mode;
    s16 pixel_x, pixel_y, pixel_w, pixel_h;
    const void *pixels;
    s16 clut_x, clut_y, clut_w, clut_h;
    const void *clut;
} TimInfo;

extern void GsGetTimInfo(const void *tim, TimInfo *info);
extern s32 vramPlaceTexture(TimInfo *info, s32 index, s32 flag);
extern int LoadImage(const Rect *rect, const void *data);
#include "semantic/scene.h"
extern s32 bitmapAddRecord(const TimInfo *info, s32 index, s32 flag);

/* Register a TIM image: upload pixels (and CLUT) to VRAM, mark the area used,
   or hand it to vramPlaceTexture when flag is set; then record it (bitmapAddRecord). */
s32 __attribute__((section(".text_8012191C")))
bitmapRegisterTim(const void *tim, s32 flag, s32 index)
{
    Rect rect;
    TimInfo info;

    GsGetTimInfo((const char *)tim + 4, &info);
    if (flag != 0) {
        vramPlaceTexture(&info, index, flag);
    } else {
        rect.x = info.pixel_x;
        rect.y = info.pixel_y;
        rect.w = info.pixel_w;
        rect.h = info.pixel_h;
        LoadImage(&rect, info.pixels);
        bitmapMarkRect(rect.x, rect.y, rect.w, rect.h);
        if ((info.mode >> 3) & 1) {  /* TIM has a CLUT */
            rect.x = info.clut_x;
            rect.y = info.clut_y;
            rect.w = info.clut_w;
            rect.h = info.clut_h;
            LoadImage(&rect, info.clut);
            bitmapMarkRect(rect.x, rect.y, rect.w, rect.h);
        }
    }
    return bitmapAddRecord(&info, index, flag);
}
