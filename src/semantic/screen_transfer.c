/* cc: fill */
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;

typedef struct ScreenRect { s16 x, y, w, h; } ScreenRect;
typedef struct TimInfo {
    u32 mode;
    s16 pixel_x, pixel_y, pixel_w, pixel_h;
    const void *pixels;
    s16 clut_x, clut_y, clut_w, clut_h;
    const void *clut;
} TimInfo;

extern TimInfo screenTimInfo;
extern ScreenRect screenImageRect;
extern ScreenRect screenDisplayRect;
extern s32 DrawSync(s32 mode);
extern void GsGetTimInfo(const void *tim, TimInfo *info);
extern s32 LoadImage(const ScreenRect *rect, const void *data);
extern s32 MoveImage(const ScreenRect *rect, s32 x, s32 y);

/* @80121C8C: upload a TIM image (header skipped) to (x, y) in the left or right display buffer. */
s32 __attribute__((section(".text_80121C8C")))
screenDisplayImage(s32 right_buffer, const void *tim, s32 x, s32 y)
{
    DrawSync(0);
    GsGetTimInfo((const char *)tim + 4, &screenTimInfo);
    screenImageRect.x = x;
    screenImageRect.w = screenTimInfo.pixel_w;
    screenImageRect.h = screenTimInfo.pixel_h;
    if (right_buffer != 0) {
        screenImageRect.x = x + 320;
    }
    screenImageRect.y = y;
    LoadImage(&screenImageRect, screenTimInfo.pixels);
    return DrawSync(0);
}

/* @80121D38: copy one 320x240 display buffer onto the other in VRAM
 * (right_buffer != 0: right half -> left half, else left -> right). */
s32 __attribute__((section(".text_80121D38")))
screenDisplayToDisplay(s32 right_buffer, s32 other /* unused; every retail caller passes !right_buffer */)
{
    s32 destination_x, destination_y;
    if (right_buffer == 0) {
        destination_x = 320;
        screenDisplayRect.x = 0;
        destination_y = 0;
        ((ScreenRect *)&screenDisplayRect)->y = 0;
    } else {
        destination_x = 0;
        destination_y = 0;
        screenDisplayRect.x = 320;
        ((ScreenRect *)&screenDisplayRect)->y = 0;   /* per-arm store, cross-jumped into the shared tail */
    }
    ((ScreenRect *)&screenDisplayRect)->w = 320;
    ((ScreenRect *)&screenDisplayRect)->h = 240;
    return MoveImage(&screenDisplayRect, destination_x, destination_y);
}

/* @80121D98: upload a block of pixels to VRAM at (x, y), waiting for the GPU before and after. */
s32 __attribute__((section(".text_80121D98")))
screenRamToVram(s32 x, s32 y, s32 width, s32 height, const void *data)
{
    ScreenRect rect;
    DrawSync(0);
    rect.w = width;
    rect.h = height;
    rect.x = x;
    rect.y = y;
    LoadImage(&rect, data);
    return DrawSync(0);
}
