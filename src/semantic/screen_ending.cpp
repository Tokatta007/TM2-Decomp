/* Ending screen: frame rendering and the frame loop.
 * C++ (PsyQ 3.6 CC1PLPSX, -fvolatile).
 *
 * The scene double-buffers two frame pairs (base0/frame, base1/other); each
 * frame is 128 bytes: ordering table, draw environment, display environment.
 * Render objects are g++ 2.x objects with their vtable pointer at +48. */
typedef signed int s32;
struct TM2Frame {
    s32 *ot;
    s32 x4, x8;
    char pad12[4];
    char draw[24];
    char x40;
    char pad41[67];
    char disp[20];
};
/* Render objects: draw() is vtable entry 1. */
struct TM2Render { char pad[48]; virtual void draw(TM2Frame *frame); };
struct TM2Scene {
    s32 base0;
    TM2Frame *frame;
    s32 base1;
    TM2Frame *other;
    char x16[12];
    char x28[12];
    s32 x40;
    s32 done;
    char pad48[8];
    void *x56;
    char pad60[36];
    s32 count0;
    TM2Render *list0[16];
    s32 count1;
    TM2Render *list1[16];
    char pad232[3432 - 232];
    s32 x3432;
    s32 limit;
    s32 result;
};
extern "C" {
extern s32 ClearOTagR(void *, s32);
extern s32 DrawSync(s32);
extern s32 VSync(s32);
extern s32 PutDrawEnv(void *);
/* DrawOTag is void in PsyQ (libgpu.h), but retail returns its v0 here (a void call in
 * tail position would become a jump); this unit keeps an int-returning view. */
extern s32 DrawOTag(void *);
extern s32 PutDispEnv(void *);
extern void sceneDrawBackground(void *, TM2Frame *);
extern void viewSetPosition(void *, s32);
extern void viewSetRotation(void *, s32);
extern s32 scenePostEvent(void *, s32, s32);
#include "semantic/scene.h"
extern void sceneRunCommands(void *);
extern s32 sceneDispatchEvents(void *);
extern s32 GetCurTics(void);
extern s32 timerTicsToFrameRate(s32);
extern void SetFrameStart(void);

/* @80122984: clear both ordering tables, draw every listed object into
 * frame, then present (the other frame first when x3432 is set). */
s32 __attribute__((section(".text_80122984"))) screenCarEnding(TM2Scene *s, TM2Frame *frame)
{
    int i;
    frame->x40 = s->x40;
    frame->x8 = frame->x4;
    s->other->x8 = s->other->x4;
    ClearOTagR(frame->ot, 4096);
    ClearOTagR(s->other->ot, 4096);
    for (i = 0; i < s->count0; i++)
        s->list0[i]->draw(frame);
    for (i = 0; i < s->count1; i++)
        s->list1[i]->draw(frame);
    DrawSync(0);
    VSync(0);
    if (s->x3432) {
        PutDrawEnv(s->other->draw);
        DrawOTag(s->other->ot + 4095);
    }
    PutDrawEnv(frame->draw);
    PutDispEnv(frame->disp);
    if (s->x56)
        sceneDrawBackground(s->x56, frame);
    return DrawOTag(frame->ot + 4095);
}

/* @80122B34: run the scene until done, flipping frames and pacing each
 * frame to the limit. The duplicate GetCurTics() calls are cross-jumped
 * into the loop test in retail. */
s32 __attribute__((section(".text_80122B34"))) sceneRun(TM2Scene *s)
{
    s32 start, elapsed, x, y;
    viewSetPosition(s->x16, 0);
    viewSetRotation(s->x28, 0);
    scenePostEvent(s, 0, 0);
    start = GetCurTics();
    while (!s->done) {
        renderFlipFrame();
        x = s->base0;
        if ((s32)s->frame == s->base0)
            x += 128;
        s->frame = (TM2Frame *)x;
        y = s->base1;
        if ((s32)s->other == s->base1)
            y += 128;
        s->other = (TM2Frame *)y;
        sceneRunCommands(s);
        sceneDispatchEvents(s);
        screenCarEnding(s, s->frame);
        elapsed = timerTicsToFrameRate(GetCurTics() - start);
        if (s->limit < 60 && s->limit < elapsed)
            do VSync(0); while (s->limit < timerTicsToFrameRate(GetCurTics() - start));
        SetFrameStart();
        start = GetCurTics();
    }
    return s->result;
}
}
