typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef signed int s32;

typedef signed char s8;
/* Two 44-byte target slots. */
typedef struct TM2TargetSlot {
    s8 active, done;
    s16 id;        /* +2 */
    s16 timer;     /* +4 */
    s16 rate;      /* +6: effect chance divisor */
    s16 trigger;   /* +8 */
    u8 pad10[2];
    void *model;   /* +12 */
    u8 position[12];   /* +16 */
    u8 x28[8];     /* +28 */
    u8 x36[8];     /* +36 */
} TM2TargetSlot;
typedef char tm2_target_slot_size[(sizeof(TM2TargetSlot) == 44) ? 1 : -1];
extern TM2TargetSlot targetPlayerEvents[2];
extern s32 gameGetMode(void);
extern s32 rand(void);
extern void fxSheetStart(s32, s32, s32, s32);
extern void fxExplosion192(void *, u8 flag);
extern void fxSmokeA128(void *, s32);
#include "semantic/world.h"
extern void soundLocateForView(void *, s32);
extern void soundLocateForView0(void *);
extern s32 PadChkMtap(void);
extern s32 soundGetListenerPan(void);
extern s32 soundPlay(s32, s32, s32, s32);
extern s32 viewGetSplitMode(void);
extern void *mathJitterPos(void *, s32, s32, s32);
extern void fxExplosion48(void *, u8 flag);
extern void fxExplosion96(void *, u8 flag);
extern void fxSmokeA48(void *, s32);
extern void debrisBurstAt(void *, void *, void *, void *, s32, s32, s32, s32, s32);

#define SLOT targetPlayerEvents[i]

/* @80127CE0: per-frame update of the two target slots. */
void __attribute__((section(".text_80127CE0"))) targetPlayerEventsUpdate(void)
{
    s32 i;
    void *effect;
    if (gameGetMode() == 2 || gameGetMode() == 4) {
        for (i = 0; i < 2; i++) {
            if (SLOT.active != 0 && SLOT.done == 0) {
                fxSheetStart(5, 255, 200, 200);
                fxExplosion192(SLOT.position, 0);
                fxSmokeA128(SLOT.position, 0);
                objRegistrySetStep(510, SLOT.id, 1);
                soundLocateForView(SLOT.position, -1);
                soundPlay(4, PadChkMtap(), soundGetListenerPan(), 99);
                soundPlay(19, PadChkMtap(), soundGetListenerPan(), 99);
                SLOT.active = 0;
                SLOT.done = 1;
            }
        }
        return;
    }
    for (i = 0; i < 2; i++) {
        if (SLOT.active == 0 || SLOT.done != 0)
            continue;
        SLOT.timer++;
        if (SLOT.timer == 1) {
            fxExplosion192(SLOT.position, 0);
            fxSmokeA128(SLOT.position, 0);
        }
        if (SLOT.rate-- < 5)
            SLOT.rate = 5;
        if (rand() % SLOT.rate == 0) {
            effect = mathJitterPos(SLOT.position, 82, 82, 48);
            if (rand() & 1) {
                fxExplosion48(effect, 0);
                fxSmokeA48(effect, 0);
            } else {
                fxExplosion96(effect, 0);
                fxSmokeA128(effect, 0);
            }
        }
        if (SLOT.timer == SLOT.trigger) {
            fxSheetStart(5, 255, 200, 200);
            fxExplosion192(SLOT.position, 0);
            fxSmokeA128(SLOT.position, 0);
            debrisBurstAt(SLOT.model, SLOT.position, SLOT.x28, SLOT.x36, 0, 0, 40, 120, 0);
            objRegistrySetStep(510, SLOT.id, 1);
        }
        if (SLOT.timer == SLOT.trigger + 30) {
            if (viewGetSplitMode() == 0)
                soundLocateForView0(SLOT.position);
            else
                soundLocateForView(SLOT.position, -1);
            soundPlay(4, PadChkMtap(), soundGetListenerPan(), 99);
            soundPlay(19, PadChkMtap(), soundGetListenerPan(), 99);
            SLOT.active = 0;
            SLOT.done = 1;
        }
    }
}
