#include "semantic/rt_buffers.h"

/* @801B600C..801F5A8C, contiguous BSS. Raw pointer bases and ClearOTagR's
 * 4096-word length agree with these IDA array extents. Word arrays preserve
 * alignment for packet views; the record's byte pointers retain byte units. */
unsigned long renderPacketBuffer0[24400];
unsigned long renderPacketBuffer1[24400];
unsigned long renderOtView1Buf0[4096];
unsigned long renderOtView1Buf1[4096];
unsigned long renderOtView2Buf0[4096];
unsigned long renderOtView2Buf1[4096];
