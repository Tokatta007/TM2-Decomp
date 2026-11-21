/* cc: fill */
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

typedef struct ScreenRect { s16 x, y, w, h; } ScreenRect;
typedef struct ClearPrimitive {
    s32 tag;
    s8 r, g, b, code;
    ScreenRect rect;
} ClearPrimitive;

extern unsigned char screenDrawEnv[];
extern unsigned char screenDispEnv[];
extern unsigned char D_80048270[];
extern s32 SetDefDrawEnv(void *, s32, s32, s32, s32);
extern s32 PutDrawEnv(void *);
extern s32 SetDefDispEnv(void *, s32, s32, s32, s32);
extern s32 PutDispEnv(void *);
extern void sub_80144E78(ClearPrimitive *);
extern void DrawPrim(ClearPrimitive *);
extern s32 DrawSync(s32);
extern s32 fileioLoadFileIntoRam(void *, const char *);
#pragma tm2_absolute screenDrawEnv
#pragma tm2_absolute screenDispEnv

s32 __attribute__((section(".text_80121B30"))) screenInit(void)
{
    SetDefDrawEnv(screenDrawEnv, 320, 0, 320, 240);
    PutDrawEnv(screenDrawEnv);
    SetDefDispEnv(screenDispEnv, 320, 0, 320, 240);
    return PutDispEnv(screenDispEnv);
}

s32 __attribute__((section(".text_80121BAC"))) screenSetEnv(void)
{
    PutDispEnv(screenDispEnv);
    return PutDrawEnv(screenDrawEnv);
}

s32 __attribute__((section(".text_80121BE4")))
screenClearScreen(s16 x, s16 y, s16 width, s16 height)
{
    ClearPrimitive primitive;
    sub_80144E78(&primitive);
    {
        /* Stores through a pointer are volatile under -fvolatile, so the last
           one stays out of DrawPrim's delay slot as in retail. */
        ClearPrimitive *p = &primitive;
        p->r = 0;
        p->g = 0;
        p->b = 0;
        p->rect.x = x;
        p->rect.y = y;
        p->rect.w = width;
        p->rect.h = height;
    }
    DrawPrim(&primitive);
    return DrawSync(0);
}

s32 __attribute__((section(".text_80121C64"))) screenLoadImageFile(const char *name, s32 arg /* unused; every retail caller passes 0 */)
{
    return fileioLoadFileIntoRam((void *)0x80048270, name);
}
