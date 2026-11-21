typedef signed int s32;typedef unsigned int u32;typedef signed short s16;typedef unsigned short u16;typedef unsigned char u8;
extern void *csAlloc(void);extern s32 lightGetEnv(s32),abs(s32);extern void csSetModel(void*,void*),viewProc(s32),viewSetFixedCamera(s32,s32,s32);
extern void *RotMatrixYXZ(void*, void*);
/* renderDrawModel is void (v0 at its return is left over from its last call), but retail
 * returns its result here, so this unit keeps the int-returning view it was compiled with. */
extern s32 renderDrawModel(s32,void*);
/* @8012D184: render the six-part movie object. The light block (+20) is
 * re-read through the object at every use, as in retail. An inactive object
 * returns the zero just read (no explicit return). */
#define LIGHT (*(u8 **)(object + 20))
s32 __attribute__((section(".text_8012D184"))) menuPasswordDigitsDraw(u8 *object, s32 context)
{
    s32 i, value, *values;
    if (*(u16 *)(object + 8) != 0) {
        if (LIGHT == 0) {
            LIGHT = csAlloc();
            *(s32 *)(LIGHT + 4) = lightGetEnv(0);
            csSetModel(LIGHT, object + 52);
        }
        values = *(s32 **)(object + 4);
        for (i = 5; i >= 0; i--) {
            *(s32 *)(object + 164 + 24 * i) = values[*(u16 *)(object + 288 + 2 * i) + 6];
            object[162 + 24 * i] = 1;
        }
        *(s32 *)(LIGHT + 168) = *(s32 *)(object + 24);
        *(s32 *)(LIGHT + 172) = *(s32 *)(object + 28);
        *(s32 *)(LIGHT + 176) = *(s32 *)(object + 32);
        *(s16 *)(LIGHT + 160) = *(s32 *)(object + 40);
        *(s16 *)(LIGHT + 162) = *(s32 *)(object + 44);
        *(s16 *)(LIGHT + 164) = *(s32 *)(object + 36);
        RotMatrixYXZ(LIGHT + 160, LIGHT + 128);
        if (*(u16 *)(object + 10) != 0)
            viewProc(0);
        else {
            if (*(s16 *)(object + 12) != -1) {
                value = *(s16 *)(object + 12);
                *(s32 *)LIGHT = value;
            }
            if (*(s32 *)(object + 16) < 0)
                viewSetFixedCamera(0, 8 / abs(*(s32 *)(object + 16)), 1);
            else
                viewSetFixedCamera(0, *(s32 *)(object + 16) * 8, 1);
        }
        return renderDrawModel(context, LIGHT);
    }
}
