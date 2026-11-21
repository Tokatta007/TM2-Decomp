#include "semantic/rt_buffers.h"

/* @800D19E0: the first record pair, not an offset from the "%s" string. */
struct TM2BufferInitRecord *__attribute__((section(".text_800D19E0"))) renderGetView0Frames(void)
{
    return &renderView0Frames[0];
}

/* @800D1B74 */
struct TM2BufferInitRecord *__attribute__((section(".text_800D1B74"))) renderGetView1Frames(void)
{
    return &renderView0Frames[2];
}

/* @800D1B84 */
struct TM2BufferInitRecord *__attribute__((section(".text_800D1B84"))) renderGetMirrorFrames(void)
{
    return &renderView0Frames[4];
}
