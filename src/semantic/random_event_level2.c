/* Arena-2 scripted event update (800D4D58), called from rtMainLoop when the level is 2. */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

#define S8(p, o) (*(s8 *)((u8 *)(p) + (o)))
#define U8(p, o) (*(u8 *)((u8 *)(p) + (o)))
#define S16(p, o) (*(s16 *)((u8 *)(p) + (o)))
#define S32(p, o) (*(s32 *)((u8 *)(p) + (o)))
#define PTR(p, o) (*(u8 **)((u8 *)(p) + (o)))

extern int rand(void);
extern u8 *moscowEventGet(s32);
extern u8 *mathJitterPos(void *, s32, s32, s32);
extern void fxExplosion24(void *, u8 flag);
extern void fxExplosion48(void *, u8 flag);
extern void fxExplosion96(void *, u8 flag);
extern void debrisBurstAt(s32, void *, void *, void *, void *, s32, s32, s32, s32);
extern s32 viewGetSplitMode(void);
extern void soundLocateForView0(void *);
extern void soundLocateForView(void *, s32);
extern u32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern void fxSheetStart(s32, s32, s32, s32);
#include "semantic/world.h"

/* @800D4D58: per-frame update of random events 1..4 (196-byte records from moscowEventGet) on arena 2:
   tick the timer, spawn random sparks, fire lanes 1..3 at their start times and end each with a
   shake/rumble; once lanes 1..3 are done play the triple burst; lane 4 fires once lanes 1..3 have
   started and ends the same way; when all four lanes are done the event retires 20 frames later. */
void __attribute__((section(".text_800D4D58"))) moscowEventsUpdate(void)
{
    s32 k, j;
    u8 *p;
    u8 *o;
    s16 t;

    for (k = 1; k < 5; k++) {
        p = moscowEventGet(k);
        if (p[1] == 0)
            continue;
        t = ++S16(p, 10);
        if (S8(p, 3)-- < 5)
            S8(p, 3) = 5;
        for (j = 1; j < 4; j++) {
            if (rand() % S8(p, 3) == 0 && p[j + 5] == 0) {
                o = mathJitterPos(p + 56 + j * 12, 0x37, 0x37, 0x147);
                if (rand() & 1)
                    fxExplosion24(o, 0);
                else
                    fxExplosion48(o, 0);
            }
            if (S16(p, 10) == S16(p, 14 + j * 2)) {
                debrisBurstAt(S32(p, 36 + j * 4), p + 56 + j * 12, &p[j * 8 + 156], &p[j * 8 + 116], 0,
                             S16(p, 24 + j * 2), 20, 200, 1);
                U8(PTR(p, 36 + j * 4), 18) = 0;
            }
            if (S16(p, 10) >= S16(p, 14 + j * 2) + S16(p, 24 + j * 2) &&
                S16(p, 10) < S16(p, 14 + j * 2) + S16(p, 24 + j * 2) + 10) {
                if (rand() % 3 == 0)
                    fxExplosion96(mathJitterPos(p + 56 + j * 12, 0x37, 0x37, 0x147), 0);
                if (viewGetSplitMode() == 0)
                    soundLocateForView0(p + 56 + j * 12);
                else
                    soundLocateForView(p + 56 + j * 12, -1);
                soundPlay(4, PadChkMtap() >> 1, soundGetListenerPan(), 99);
                soundPlay(19, PadChkMtap() >> 1, soundGetListenerPan(), 99);
                p[j + 5] = 1;
            }
        }
        if (p[6] && p[7] && p[8] && p[4] == 0) {
            o = mathJitterPos(p + 68, 0x147, 0x147, 0x37);
            S32(o, 8) -= 400;
            fxExplosion96(o, 0);
            o = mathJitterPos(p + 80, 0x147, 0x147, 0x37);
            S32(o, 8) -= 400;
            fxExplosion96(o, 0);
            fxExplosion96(mathJitterPos(p + 104, 0x147, 0x147, 0x37), 0);
            fxSheetStart(3, 255, 200, 200);
            objRegistrySetStep(4000, k, 1);
            p[4] = 1;
        }
        if (rand() % S8(p, 3) == 0 && p[9] == 0) {
            o = mathJitterPos(p + 104, 0x147, 0x147, 0x37);
            if (rand() & 1)
                fxExplosion24(o, 0);
            else
                fxExplosion48(o, 0);
        }
        if (S16(p, 10) >= S16(p, 22) && S16(p, 10) >= S16(p, 18) && S16(p, 10) >= S16(p, 20)) {
            debrisBurstAt(S32(p, 52), p + 104, p + 188, p + 148, 0, S16(p, 32), 20, 200, 0);
            U8(PTR(p, 52), 18) = 0;
        }
        if (S16(p, 10) >= S16(p, 22) + S16(p, 32) && S16(p, 10) < S16(p, 22) + S16(p, 32) + 10) {
            if (rand() % 3 == 0)
                fxExplosion96(mathJitterPos(p + 104, 0x147, 0x147, 0x37), 0);
            p[9] = 1;
            if (viewGetSplitMode() == 0)
                soundLocateForView0(p + 104);
            else
                soundLocateForView(p + 104, -1);
            soundPlay(4, PadChkMtap() >> 1, soundGetListenerPan(), 99);
            soundPlay(19, PadChkMtap() >> 1, soundGetListenerPan(), 99);
        }
        if (p[6] && p[7] && p[8] && p[9]) {
            if (p[2] == 0)
                p[2] = S16(p, 10) + 20;
            if (S16(p, 10) >= S8(p, 2))
                p[1] = 0;
        }
    }
}
