/* Semantic reconstruction of the bidirectional animation update at 80151ACC. */
typedef signed int s32;
typedef unsigned char u8;
typedef unsigned short u16;

typedef struct AnimationPingPongState {
    u8 pad00[5];
    u8 trigger_frame;
    u16 frame_period;
    u8 first_frame;
    u8 last_frame;
    u8 pad0A[2];
    u8 frame;
    u8 direction;
    u16 elapsed;
    u8 pad10[3];
    u8 child_frame;
    u8 pad14[8];
    void *child;
} AnimationPingPongState;

extern s32 renderFrameTicks;
#include "semantic/world.h"

void __attribute__((section(".text_80151ACC")))
animStepPingPong(AnimationPingPongState *state)
{
    state->elapsed = (u16)(state->elapsed + renderFrameTicks);
    if (state->elapsed <= state->frame_period)
        return;

    if (state->frame == 0)
        animPickPeriod(state, 1);

    state->frame = (u8)(state->frame + state->direction);
    if (state->frame == 0)
        animPickPeriod(state, 0);

    if (state->frame == state->last_frame)
        state->direction = 0xFF;
    else if (state->frame == state->first_frame)
        state->direction = 1;

    if (state->child != 0 && state->frame == state->trigger_frame)
        animControl(state->child, state->child_frame);

    state->elapsed = 0;
}
