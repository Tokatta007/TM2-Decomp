/* Main loop and render-state helpers (800D1318..800D1DAC).
 *
 * rtMainLoop runs frames until a scheduled event fires (rtUpdateScheduledEvents) and
 * returns that event's value. Each frame: flip the view-0 buffer, poll the
 * pads, update the world, draw view 0, then the second split-screen view or
 * the rear-view mirror, wait for the frame time (15 or 20 tics), and present.
 *
 * Buffers: renderView0Frames[0..1] view 0, [2..3] view 1, [4..5] rear view;
 * D_80180EE4/EE8/EEC point at the current ones (see rt_buffers.h). */

/* Retail addresses the six records and the three slot pointers as separate
 * symbols (the slots through $gp), as in rt_buffer_init.c. */
#define renderView0Frames tm2_rt_record_array_view
#define D_80180EE4 tm2_rt_slot_array_view
#include "semantic/rt_buffers.h"
#undef renderView0Frames
#undef D_80180EE4
#include "semantic/port_far_lod.h"
#include "semantic/port_far_distance.h"
#include "semantic/port_rollback.h"   /* request 48 (ONLINE_ROLLBACK.md 8.1): FIXED_TICKS, HOLD, PAUSE_PORT's quit */
#ifdef TM2_PORT_VISUAL
#include "semantic/port_visual.h"   /* request 79 (PICKUP_LOOKS.md 10): portPickupSpecialBuild */
#endif
extern struct TM2BufferInitRecord renderView0Frames, renderView1Frames, renderMirrorFrames, renderMirrorFrameB;
extern struct TM2BufferInitRecord *D_80180EE4, *renderBufferView1, *renderBufferRearView;

typedef struct TM2BufferInitRecord Buffer;

/* PC port request 47 revised (FAR_DISTANCE.md 3): view 2's and the mirror's packet cursor. Retail goes
 * on from view 1's (one buffer a frame); with TM2_PORT_FAR_DISTANCE a view whose records have a
 * buffer of their own (portPacketBuffersApply) starts at its start, one that shares view 1's goes on. */
#ifdef TM2_PORT_FAR_DISTANCE
#define RT_PACKET_CURSOR(record, before) \
    portPacketBufferCursor((unsigned char *)(record), (unsigned char *)(before))
#else
#define RT_PACKET_CURSOR(record, before) ((before)->field_8)
#endif

/* One scheduled event: when time drops below zero, rtMainLoop returns value. */
struct TM2Timer {
    int time;
    int value;
};

extern int D_801809D4;              /* scheduled events */
extern int rtFiredEventValue;              /* value of the event that fired */
extern int rtRearViewActive;              /* rear-view mirror shown */
extern int rtRearViewAllowed;              /* rear-view mirror allowed */
extern int viewSplitScreenMode;              /* split screen: 0 off, 1 and 2 two layouts */
extern int rtFrameTime;              /* frame time in tics */
extern int rtFieldCounter;              /* field counter */
extern struct TM2Timer rtScheduledEvents[4];
extern int worldLoadShift;
extern int worldLoadPickupIndex, worldLoadTargetIndex;


#include "semantic/input.h"
#include "semantic/world.h"
extern int VSync(int mode);
extern void worldUpdate(void);
extern void animEnable(void);
extern void animDisable(void);
extern int gameGetMode(void);
#include "semantic/car_calls.h"
extern void targetActorsUpdate(void);
extern void fxRecordsUpdate(void);
extern void debrisUpdate(void);
extern void lightFlickerUpdate(void);
extern void viewProc(int view);
extern void streaksDraw(Buffer *, int view);
extern void bulletsUpdate(void);
extern int gameGetLevel(void);
extern void moscowEventsUpdate(void);
extern void moscowDishUpdate(void);
extern void laEventsUpdate(void);
extern void targetTimedEventUpdate(void);
extern void eiffelTowerUpdate(void);
extern void targetLevelEventUpdate(void);
extern void targetPlayerEventsUpdate(void);
extern void fxSpritesUpdate(void);
extern void fxSpritesDraw(Buffer *, int view);
extern void debrisDraw(Buffer *);
#include "semantic/scene.h"
extern void renderScene(Buffer *, int view);
extern int timerTicsToFrameRate(int tics);
extern void clutAnimUpdate(void);
extern void hudDrawSingleView(int view, int first);
extern void hudDrawSplitVertical(int view, int first);
extern void hudDrawSplitHorizontal(int view, int first);
extern void pauseMenu(void);
extern void renderSetWorldShift(int);
extern int printf(const char *, ...);
extern char D_800CE910[];
extern void dbscanHierarchy(int, int, void (*)());
extern void worldLoadObject();
extern void csSetModel(void *, void *);

void nullsub_1(void);
int rtUpdateScheduledEvents(void);
void rtUpdateFieldCounter(int tics);

/* @800D1318
 * Matching notes: the prologue stores go through their own local (init) so
 * the prologue's &renderView0Frames is not reused for `first`; `first` is set just
 * before the loop and the loop is a goto loop (so start, not first, gets s0);
 * the frame time is an if/else whose two stores retail merges into one. */
int __attribute__((section(".text_800D1318"))) rtMainLoop(void)
{
    int start;
    Buffer *first;
    Buffer *buffer;
    Buffer *init;

#ifdef TM2_PORT_ROLLBACK
    portRollbackLoopEnter();    /* request 48 FIXED_TICKS: the round's first step (and worldUpdate below) at fixed ticks */
#endif
    padSetRepeatMode(1);
    nullsub_1();
    start = GetCurTics();
    init = &renderView0Frames;
    D_80180EE4 = init;
    if (viewSplitScreenMode) {
        init = &renderView1Frames;
        renderBufferView1 = init;
    }
    worldUpdate();
    if (rtRearViewActive) {
        init = &renderMirrorFrames;
        renderBufferRearView = init;
    }
    animEnable();
    if (gameGetMode() == 2)
        rtFrameTime = 15;
    else
        rtFrameTime = 20;
#ifdef TM2_PORT_ROLLBACK
    portRollbackFrameTime();    /* request 48 FIXED_TICKS: the pacing at 60 / ticks */
#endif
    first = &renderView0Frames;

loop:
    /* request 48 (ONLINE_ROLLBACK.md 6, 8.1 bits 7 and 9): with TM2_PORT_ROLLBACK the top of the iteration latches
     * HOLD (portRollbackTop; a quit from portRollbackQuit is scheduled first) and RB_STEP skips every update of a
     * held iteration, which then only draws; without the define both are retail's calls.
     * Stage 3 STEP_SPLIT (ONLINE_ROLLBACK.md 12): portRollbackTop also latches the split; then portRollbackLoopStep
     * runs the whole step here (portStepBody, below), RB_STEP skips the iteration's own update calls, and the rest
     * of the iteration only draws (view 0 is drawn after the world step) */
    if (!RB_EVENTS()) {
        GetCurTics();
#ifdef TM2_PORT_FAR_DISTANCE
        /* PC port request 31 (FAR_DISTANCE.md): the six frame records (view 0, view 1,
         * mirror; two parities each) use the port's packet buffers when it sized them,
         * a buffer per view since request 47 revised. */
        portPacketBuffersApply((unsigned char *)&renderView0Frames, 6);
#endif
        buffer = &renderView0Frames;
        if (D_80180EE4 == buffer)
            buffer++;
        buffer->field_8 = buffer->field_4;
        D_80180EE4 = buffer;
        ClearOTagR(buffer->field_0, 4096);
        RB_STEP(padUpdate());
        RB_STEP(targetActorsUpdate());
        RB_STEP(fxRecordsUpdate());
        RB_STEP(debrisUpdate());
        RB_STEP(lightFlickerUpdate());
        viewProc(0);
        streaksDraw(D_80180EE4, 0);
#ifdef TM2_PORT_ROLLBACK
        /* request 48 stage 2 (ONLINE_ROLLBACK.md 11, ONCE_PER_STEP): streaksDraw's four ticks once per step, here
           (after view 0's drawing, where retail ran them first), whatever the views drawn */
        RB_STEP(portRollbackStepEffects());
#endif
        RB_STEP(bulletsUpdate());
        if (gameGetLevel() == 2) {
            RB_STEP(moscowEventsUpdate());
            RB_STEP(moscowDishUpdate());
        }
        if (gameGetLevel() == 1) {
            RB_STEP(laEventsUpdate());
            RB_STEP(targetTimedEventUpdate());
        }
        if (gameGetLevel() == 3)
            RB_STEP(eiffelTowerUpdate());
        if (gameGetLevel() == 5)
            RB_STEP(targetLevelEventUpdate());
        if (gameGetLevel() == 7)
            RB_STEP(targetPlayerEventsUpdate());
        RB_STEP(fxSpritesUpdate());
        fxSpritesDraw(D_80180EE4, 0);
        debrisDraw(D_80180EE4);
        RB_STEP(renderFlipFrame());
#ifdef TM2_PORT_ROLLBACK
        /* request 48 stage 2 (ONLINE_ROLLBACK.md 11, ANIM_IN_STEP): the animations step here, not in renderScene */
        RB_STEP(portRollbackAnimStep());
#endif
        renderScene(D_80180EE4, 0);
        RB_STEP(worldUpdate());

        if (viewSplitScreenMode) {
            buffer = &renderView1Frames;
            if (renderBufferView1 == buffer)
                buffer++;
            renderBufferView1 = buffer;
            ClearOTagR(buffer->field_0, 4096);
            viewProc(1);
            renderBufferView1->field_8 = RT_PACKET_CURSOR(renderBufferView1, D_80180EE4);
            streaksDraw(renderBufferView1, 1);
            debrisDraw(renderBufferView1);
            fxSpritesDraw(renderBufferView1, 1);
            renderScene(renderBufferView1, 1);
        } else if (rtRearViewActive) {
            buffer = &renderMirrorFrames;
            if (renderBufferRearView == buffer)
                buffer++;
            renderBufferRearView = buffer;
            ClearOTagR(buffer->field_0, 4096);
            viewProc(1);
            renderBufferRearView->field_8 = RT_PACKET_CURSOR(renderBufferRearView, D_80180EE4);
            streaksDraw(renderBufferRearView, 1);
            fxSpritesDraw(renderBufferRearView, 1);
            debrisDraw(renderBufferRearView);
            renderScene(renderBufferRearView, 1);
        }

        DrawSync(0);
        do
            VSync(0);
        while (rtFrameTime < timerTicsToFrameRate(GetCurTics() - start));
        RB_STEP(rtUpdateFieldCounter((short)timerGetFrameTicks()));
        RB_STEP(SetFrameStart());
        start = GetCurTics();
        RB_STEP(clutAnimUpdate());
        PutDrawEnv(&D_80180EE4->field_10);
        PutDispEnv(&D_80180EE4->field_6C);
        DrawOTag(D_80180EE4->field_0 + 4095);
        if (!gameIsAttractDemo()) {
            switch (viewSplitScreenMode) {
            case 0:
                hudDrawSingleView(0, D_80180EE4 == first);
                break;
            case 1:
                hudDrawSplitVertical(0, D_80180EE4 == first);
                PutDrawEnv(&renderBufferView1->field_10);
                DrawOTag(renderBufferView1->field_0 + 4095);
                hudDrawSplitVertical(1, D_80180EE4 == first);
                break;
            case 2:
                hudDrawSplitHorizontal(0, D_80180EE4 == first);
                PutDrawEnv(&renderBufferView1->field_10);
                DrawOTag(renderBufferView1->field_0 + 4095);
                hudDrawSplitHorizontal(1, D_80180EE4 == first);
                break;
            }
        }
        if (rtRearViewActive) {
            PutDrawEnv(&renderBufferRearView->field_10);
            DrawOTag(renderBufferRearView->field_0 + 4095);
        }
        pauseMenu();
        goto loop;
    }
    animDisable();
    padSetRepeatMode(0);
    return rtFiredEventValue;
}

#ifdef TM2_PORT_ROLLBACK
/* ---- request 48 stage 3 STEP_SPLIT (ONLINE_ROLLBACK.md 8.2, 12): the step without the drawing ----
 * portStepBody is the update half of rtMainLoop's iteration in retail's order (the calls RB_STEP guards there, and
 * stage 2's two step runners at their places), with the HUD's radar swap (portRollbackHudStep) before worldUpdate.
 * Without the drawing in between: viewProc(0) and renderScene(0) ran before worldUpdate in retail; here the step sees
 * the last frame's camera (its readers are the sound listener and debrisStartBurst's capture: presentation,
 * ONLINE_ROLLBACK.md 1.3, 11.6 (b)). With portResimulating set (a re-run) padUpdate is not called: the inputs come from
 * the port's words (HUMANS16 H2 INPUT_ALL), and the pad latch stays the live step's. In .port_text. */
#define RB_PORT_TEXT __attribute__((section(".port_text")))
extern void portStepStackScrub(int bytes);
int RB_PORT_TEXT portStepBody(void)
{
    if (portStepScrub > 0)
        portStepStackScrub(portStepScrub);  /* request 86: the stack below, the same on every machine */
    if (rtUpdateScheduledEvents())
        return 1;   /* an event fired: the round is over (rtFiredEventValue) */
    if (!portResimulating)
        padUpdate();
    targetActorsUpdate();
    fxRecordsUpdate();
    debrisUpdate();
    lightFlickerUpdate();
    portRollbackStepEffects();      /* ONCE_PER_STEP's ticks (bit 2), where view 0's streaksDraw stood */
    bulletsUpdate();
    if (gameGetLevel() == 2) {
        moscowEventsUpdate();
        moscowDishUpdate();
    }
    if (gameGetLevel() == 1) {
        laEventsUpdate();
        targetTimedEventUpdate();
    }
    if (gameGetLevel() == 3)
        eiffelTowerUpdate();
    if (gameGetLevel() == 5)
        targetLevelEventUpdate();
    if (gameGetLevel() == 7)
        targetPlayerEventsUpdate();
    fxSpritesUpdate();
    renderFlipFrame();
    portRollbackAnimStep();         /* ANIM_IN_STEP (bit 3) */
    portRollbackHudStep();          /* the radar swap, before worldUpdate's plots */
    worldUpdate();
    rtUpdateFieldCounter((short)timerGetFrameTicks());
    SetFrameStart();
    clutAnimUpdate();               /* CLUT_SHADOW (bit 6): no upload while re-running */
    return 0;
}

/* The port's re-run (its rollback engine's "advance"), from its frame hook between iterations of rtMainLoop (the top
 * of the iteration or the pacing wait) with portResimulating set around the batch: one step whatever this iteration's
 * HOLD latch says. 1 = an event fired (the round is over): stop the batch; rtMainLoop leaves at its next top. */
int RB_PORT_TEXT portRunStep(void)
{
    int held = portRollbackHolding;
    int r;

    portRollbackHolding = 0;
    r = portStepBody();
    portRollbackHolding = held;
    if (r)
        portRollbackEnded = 1;
    return r;
}

/* rtMainLoop's top when portRollbackTop returned 1: an end a re-run left (1: leave with rtFiredEventValue), a held
 * iteration (0: draw only), else STEP_SPLIT's live step (1 = an event fired). */
int RB_PORT_TEXT portRollbackLoopStep(void)
{
    if (portRollbackEnded) {
        portRollbackEnded = 0;
        return 1;
    }
    if (portRollbackHolding || !portRollbackSplitting)
        return 0;
    return portStepBody();
}

/* Request 55 (ONLINE_ROLLBACK.md 15.4): 1 when the next portStepBody (live or re-run) returns 1 at once, the round
 * over: its first call, rtUpdateScheduledEvents, counts every scheduled event down by this step's ticks
 * ((short)timerGetFrameTicks(): the last SetFrameStart's, FIXED_TICKS' constant) and fires one whose time drops below
 * zero. The same test, without counting down or consuming anything (a peek: no store). Every round end goes through
 * a scheduled event (rtMainLoop leaves no other way), and an event fires no earlier than the step after the one that
 * scheduled it, before that step reads an input: so the answer depends on the state at the end of the steps run so
 * far alone, never on the next step's inputs. Between iterations of rtMainLoop (the port's hook), any time. */
int RB_PORT_TEXT portRollbackEndDue(void)
{
    int i;
    int ticks = (short)timerGetFrameTicks();

    for (i = 0; i < D_801809D4 && i < 4; i++)
        if (rtScheduledEvents[i].time - ticks < 0)
            return 1;
    return 0;
}
#endif /* TM2_PORT_ROLLBACK */

/* @800D181C: schedule an event (at most four). */
void __attribute__((section(".text_800D181C"))) rtScheduleEvent(int value, int time)
{
    if (D_801809D4 < 4) {
        rtScheduledEvents[D_801809D4].value = value;
        rtScheduledEvents[D_801809D4].time = time;
        D_801809D4++;
    }
}

/* @800D1858: clear the events. Retail clears the slot at the current count
 * four times rather than slots 0..3. */
void __attribute__((section(".text_800D1858"))) rtClearScheduledEvents(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        rtScheduledEvents[D_801809D4].value = 0;
        rtScheduledEvents[D_801809D4].time = -1;
    }
    D_801809D4 = 0;
    rtFiredEventValue = 0;
}

/* @800D18A0: count the events down by this frame's tics; when one expires,
 * store its value in rtFiredEventValue, remove it and return 1. */
int __attribute__((section(".text_800D18A0"))) rtUpdateScheduledEvents(void)
{
    int fired;
    int i;
    if (!D_801809D4)
        return 0;
    fired = -1;
    for (i = 0; i < D_801809D4; i++) {
        rtScheduledEvents[i].time -= (short)timerGetFrameTicks();
        if (rtScheduledEvents[i].time < 0)
            fired = i;
    }
    if (fired == -1)
        return 0;
    rtFiredEventValue = rtScheduledEvents[fired].value;
    for (i = fired + 1; i < D_801809D4; i++) {
        rtScheduledEvents[i - 1].value = rtScheduledEvents[i].value;
        rtScheduledEvents[i - 1].time = rtScheduledEvents[i].time;
    }
    D_801809D4--;
    return 1;
}

/* @800D19B0 */
void __attribute__((section(".text_800D19B0"))) rtUpdateFieldCounter(int tics)
{
    rtFieldCounter += tics;
}

/* @800D19D4 */
int __attribute__((section(".text_800D19D4"))) rtGetFieldCounter(void)
{
    return rtFieldCounter;
}

/* @800D1A20 */
Buffer *__attribute__((section(".text_800D1A20"))) renderGetCurrentFrame(void)
{
    return D_80180EE4;
}

/* @800D1A64 */
unsigned long *__attribute__((section(".text_800D1A64"))) renderGetPacketBuffer(void)
{
    return renderPacketBuffer0;
}

/* @800D1A74 */
unsigned long *__attribute__((section(".text_800D1A74"))) renderGetOrderingTable(void)
{
    return renderOtView1Buf0;
}

/* @800D1B94 */
void __attribute__((section(".text_800D1B94"))) rtSetRearView(int allowed)
{
    if (allowed == 1)
        rtRearViewAllowed = allowed;
    else
        rtRearViewAllowed = 0;
}

/* @800D1BB8 */
void __attribute__((section(".text_800D1BB8"))) rtToggleRearView(void)
{
    Buffer *buffer;
    if (rtRearViewAllowed) {
        rtRearViewActive = !rtRearViewActive;
        buffer = &renderMirrorFrames;
        if (D_80180EE4 == &renderView0Frames)
            buffer++;
        renderBufferRearView = buffer;
    }
}

/* @800D1C00 */
int __attribute__((section(".text_800D1C00"))) rtGetRearViewActive(void)
{
    return rtRearViewActive;
}

/* @800D1C0C */
void __attribute__((section(".text_800D1C0C"))) rtRearViewOff(void)
{
    rtRearViewActive = 0;
}

/* @800D1C18: split-screen mode. */
int __attribute__((section(".text_800D1C18"))) viewGetSplitMode(void)
{
    return viewSplitScreenMode;
}

/* @800D1C24 */
void __attribute__((section(".text_800D1C24"))) viewSetSplitMode(int mode)
{
    viewSplitScreenMode = mode;
}

/* @800D1C30 */
void __attribute__((section(".text_800D1C30"))) nullsub_1(void)
{
}

/* @800D1C70: apply the start parameters from the block at 80019C40 (filled
 * before the game starts): difficulty, then load each listed item through
 * dbscanHierarchy. Retail walks the block with one pointer variable reloaded
 * with each field's address; the fields at +0..+C are read and discarded
 * (+8 only when flag is set). */
#ifdef TM2_PORT_GLACIER_FIRE
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_GLACIER_FIRE needs the port variant (build_source_only.py --port-variant)"
#endif
extern int portGlacierFire(void);   /* port_variant_arrays.c (request 120) */
#endif
void __attribute__((section(".text_800D1C70"))) worldLoadStartObjects(int flag)
{
    unsigned int count;
    unsigned int i;
    int *p;
    int unused;
    p = (int *)0x80019C40;
    unused = *p;
    p = (int *)0x80019C44;
    unused = *p;
    p = (int *)0x80019C48;
    if (flag)
        unused = *p;
    p = (int *)0x80019C4C;
    unused = *p;
    p = (int *)0x80019C50;
    renderSetWorldShift(*p);
    worldLoadShift = *p;
    p = (int *)0x80019C58;
    count = *p;
    worldLoadPickupIndex = 0;
    worldLoadTargetIndex = 0;
#ifdef TM2_PORT_GLACIER_FIRE
    portGlacierFire();   /* PC port request 120: Antarctica's fire missile out of the mark that hides it */
#endif
    p = (int *)0x80019C5C;
    for (i = 0; i < count; i++) {
        if (!*p) {
            printf(D_800CE910);
            break;
        }
        dbscanHierarchy(*p, worldLoadShift, worldLoadObject);
        p++;
    }
#ifdef TM2_PORT_FAR_LOD
    /* PC port request 30 (FAR_LOD.md): the level's LOD records to extend. */
    portFarLodScanLevel();
#endif
#ifdef TM2_PORT_VISUAL
    /* PC port request 79 (PICKUP_LOOKS.md 10): the special's pickup look, for the port to point pickups at. */
    portPickupSpecialBuild();
#endif
}

/* @800D1D64 */
void *__attribute__((section(".text_800D1D64"))) dbInitCsForAModel(void *model)
{
    void *cs = csAlloc();
    csListAdd(cs);
    csSetModel(cs, model);
    return cs;
}
