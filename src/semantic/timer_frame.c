/* Frame timer state (retail forms every address separately, so these are individual absolute globals):
 * timerFrameTicks frame ticks (clamped 3..5), timerFrameRate update rate, timerFrameCount frame counter,
 * timerLastFrameDelta last frame delta, timerFrameStart frame start, timerPrevFrameStart last frame start. */
extern short timerFrameTicks, timerFrameRate;
extern unsigned int timerFrameCount, timerLastFrameDelta, timerFrameStart, timerPrevFrameStart;
#pragma tm2_absolute timerFrameTicks
#pragma tm2_absolute timerFrameRate
#pragma tm2_absolute timerFrameCount
#pragma tm2_absolute timerLastFrameDelta
#pragma tm2_absolute timerFrameStart
#pragma tm2_absolute timerPrevFrameStart
extern unsigned int timerTicsAccum;
extern int timerRootCounterEvent, soundManagerObject;
extern unsigned int timerBaseRate;
extern unsigned char carResetWeapons[];
extern int EnterCriticalSection(void), ExitCriticalSection(void);
extern int OpenEvent(unsigned int, unsigned int, unsigned int, int (*)(void));
extern int EnableEvent(int), DisableEvent(int), CloseEvent(int);
extern int SetRCnt(unsigned int, unsigned int, unsigned int);
extern int StartRCnt(unsigned int), StopRCnt(unsigned int);
extern unsigned int GetRCnt(unsigned int);
extern void taskMgrRunFrame(int);
#include "semantic/port_rollback.h"   /* request 48: FIXED_TICKS */
#pragma tm2_absolute timerTicsAccum
#pragma tm2_absolute timerRootCounterEvent
#pragma tm2_absolute timerBaseRate
#pragma tm2_absolute soundManagerObject

/* @801553A8: defined 32-bit wraparound for the interrupt accumulator. */
int __attribute__((section(".text_801553A8"))) SysClkIntHandler(void)
{
    unsigned int value = timerTicsAccum + 65535u;
    timerTicsAccum = value;
    return (int)value;
}

/* @8015510C */
void __attribute__((section(".text_8015510C"))) InitTimer(void)
{
    timerTicsAccum = 0;
    EnterCriticalSection();
    timerRootCounterEvent = OpenEvent(0xF2000002u, 2, 0x1000, SysClkIntHandler);
    ExitCriticalSection();
    EnableEvent(timerRootCounterEvent);
    SetRCnt(0xF2000002u, 0xFFFF, 0x1000);
    StartRCnt(0xF2000002u);
    timerFrameCount = 0;
    timerBaseRate = 60;
}

/* @801551A4 */
void __attribute__((section(".text_801551A4"))) TermTimer(void)
{
    StopRCnt(0xF2000002u);
    DisableEvent(timerRootCounterEvent);
    CloseEvent(timerRootCounterEvent);
}

/* @801551E8: read the interrupt accumulator AFTER the hardware-counter call. */
unsigned int __attribute__((section(".text_801551E8"))) GetCurTics(void)
{
    unsigned int low = GetRCnt(0xF2000002u);
    return timerTicsAccum + low;
}

/* @80155214. The short truncations precede the signed clamp tests. */
void __attribute__((section(".text_80155214"))) SetFrameStart(void)
{
    unsigned int now = GetCurTics();
    unsigned int delta = now - timerFrameStart;
    unsigned int value = (delta + 0x89EDu) / 70618u;
#ifdef TM2_PORT_ROLLBACK
    /* PC port request 48 (ONLINE_ROLLBACK.md 8.1 bit 0, FIXED_TICKS): the step's ticks are the port's (or the mode's
       rule), the rate 60 / ticks, timerFrameCount + 1; the clock read above is discarded */
    if (portRollbackFixedFrame())
        return;
#endif
    timerPrevFrameStart = now;
    timerFrameStart = now;
    timerLastFrameDelta = delta;
    timerFrameTicks = (short)value;
    if (timerFrameTicks <= 0) timerFrameTicks = 3;
    if (timerFrameTicks >= 6) timerFrameTicks = 5;
    timerFrameRate = (short)(timerBaseRate / (unsigned int)timerFrameTicks);
    if (timerFrameRate <= 0) timerFrameRate = 1;
    timerFrameCount += 1u;
}

/* @8015531C */
int __attribute__((section(".text_8015531C"))) timerGetFrameTicks(void) { return timerFrameTicks; }
/* @8015532C */
int __attribute__((section(".text_8015532C"))) timerGetFrameRate(void) { return timerFrameRate; }
/* @80155370 */
unsigned int __attribute__((section(".text_80155370"))) GsGetWorkBase_0(void) { return timerFrameCount; }
/* @80155380 */
void __attribute__((section(".text_80155380"))) ResetUpdateRate(void) { timerFrameStart = GetCurTics(); }

/* @801553C8 */
unsigned int __attribute__((section(".text_801553C8"))) timerTicsToFrameRate(int arg0)
{
    int divisor = ((unsigned int)arg0 + 0x89EDu) / 70618u;
    int value;
    if (divisor > 0) {
        value = timerBaseRate / divisor;
        if (value > 0) goto out;
    }
    value = 1;
out:
    return value;
}

/* @800D34C0 */
#ifdef TM2_PORT_ROLLBACK
/* request 48 stage 3 SOUND_GATE (ONLINE_ROLLBACK.md 12): no sound frame while the port re-runs steps */
void __attribute__((section(".text_800D34C0"))) soundUpdate(void)
{
    if (!portRollbackSoundOff())
        taskMgrRunFrame(soundManagerObject);
}
#else
void __attribute__((section(".text_800D34C0"))) soundUpdate(void) { taskMgrRunFrame(soundManagerObject); }
#endif
/* @80119834: the proven caller domain is two player slots. */
void __attribute__((section(".text_80119834"))) carSetResetWeapons(signed char value, int index) { carResetWeapons[index] = value; }
