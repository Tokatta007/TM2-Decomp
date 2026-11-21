/* Semantic reconstruction of the timed animation-frame advance at 801519DC. */
typedef signed int s32;
typedef unsigned char u8;
typedef unsigned short u16;

typedef struct AnimationFrameState {
    u8 pad00[5];
    u8 trigger_frame;
    u16 frame_period;
    u8 first_frame;
    u8 last_frame;
    u8 pad0A[2];
    u8 frame;
    u8 pad0D;
    u16 elapsed;
    u8 pad10[3];
    u8 child_frame;
    u8 pad14[8];
    void *child;
} AnimationFrameState;

extern s32 renderFrameTicks;
#include "semantic/world.h"

void __attribute__((section(".text_801519DC")))
animStepLoop(AnimationFrameState *state)
{
    state->elapsed = (u16)(state->elapsed + renderFrameTicks);
    if (state->elapsed <= state->frame_period)
        return;

    if (state->frame == 0)
        animPickPeriod(state, 1);

    state->frame++;
    if (state->frame > state->last_frame)
        state->frame = state->first_frame;

    if (state->child != 0 && state->frame == state->trigger_frame)
        animControl(state->child, state->child_frame);

    if (state->frame == 0)
        animPickPeriod(state, 0);
    state->elapsed = 0;
}
