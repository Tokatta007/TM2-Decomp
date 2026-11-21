/* rtDoubleBufferInit @0x800D1A84.
 * Stage 1: typed views of the six 0x80-byte records written by retail.
 * Raw 800D1B3C stores renderPacketBuffer0, not IDA's folded zero, at 80181390.
 * Storage definitions live in rt_buffer_records.c and rt_buffer_slots.c.
 * Packet/ordering storage lives in rt_buffer_workspace.c. Downstream game
 * consumers remain retail dependencies in the current probe image.
 */
/* Retail addresses the six records and the three slot pointers as separate
 * symbols (the slots through $gp), so the header's array views are renamed
 * out of the way here. */
#define renderView0Frames tm2_rt_record_array_view
#define D_80180EE4 tm2_rt_slot_array_view
#include "semantic/rt_buffers.h"
#undef renderView0Frames
#undef D_80180EE4
extern struct TM2BufferInitRecord renderView0Frames, renderView0FrameB, renderView1Frames, renderView1FrameB, renderMirrorFrames, renderMirrorFrameB;
extern struct TM2BufferInitRecord *D_80180EE4, *renderBufferView1, *renderBufferRearView;

struct TM2BufferInitRecord *rtDoubleBufferInit(void)
{
    renderView0Frames.field_4 = (unsigned char *)renderPacketBuffer0;
    renderView0FrameB.field_4 = (unsigned char *)renderPacketBuffer1;
    renderView0Frames.field_0 = renderOtView1Buf0;
    renderView0FrameB.field_0 = renderOtView1Buf1;
    renderView0Frames.field_C = renderView0Frames.field_4 + 0x17CF0;
    renderView0FrameB.field_C = renderView0FrameB.field_4 + 0x17CF0;
    D_80180EE4 = &renderView0Frames;
    renderView1Frames.field_0 = renderOtView2Buf0;
    renderBufferView1 = &renderView1Frames;
    renderView1FrameB.field_0 = renderOtView2Buf1;
    renderView1Frames.field_4 = (unsigned char *)renderPacketBuffer0;
    renderView1FrameB.field_4 = (unsigned char *)renderPacketBuffer1;
    renderView1Frames.field_C = renderView1Frames.field_4 + 0x17CF0;
    renderView1FrameB.field_C = renderView1FrameB.field_4 + 0x17CF0;
    renderMirrorFrames.field_4 = (unsigned char *)renderPacketBuffer0;
    renderMirrorFrameB.field_4 = (unsigned char *)renderPacketBuffer1;
    renderMirrorFrames.field_0 = renderOtView2Buf0;
    renderMirrorFrameB.field_0 = renderOtView2Buf1;
    renderMirrorFrames.field_C = renderMirrorFrames.field_4 + 0x17CF0;
    renderMirrorFrameB.field_C = renderMirrorFrameB.field_4 + 0x17CF0;
    renderBufferRearView = &renderMirrorFrames;
    return &renderMirrorFrames;
}
