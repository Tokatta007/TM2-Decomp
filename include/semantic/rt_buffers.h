#ifndef TM2_SEMANTIC_RT_BUFFERS_H
#define TM2_SEMANTIC_RT_BUFFERS_H

#include <stddef.h>
#include <libgte.h>
#include <libgpu.h>

/* Reconstructed record view, not an original type/name claim. The six
 * records start at 8018118C, stride 80. rtMainLoop passes +10 to
 * PutDrawEnv and +6C to PutDispEnv; +8 is the primitive write cursor. */
struct TM2BufferInitRecord {
    unsigned long *field_0;
    unsigned char *field_4;
    unsigned char *field_8;
    unsigned char *field_C;
    DRAWENV field_10;
    DISPENV field_6C;
};

typedef char tm2_buffer_record_size[(sizeof(struct TM2BufferInitRecord) == 0x80) ? 1 : -1];
typedef char tm2_buffer_field_C_offset[((unsigned int)&((struct TM2BufferInitRecord *)0)->field_C == 12) ? 1 : -1];
typedef char tm2_buffer_drawenv_offset[((unsigned int)&((struct TM2BufferInitRecord *)0)->field_10 == 0x10) ? 1 : -1];
typedef char tm2_buffer_dispenv_offset[((unsigned int)&((struct TM2BufferInitRecord *)0)->field_6C == 0x6C) ? 1 : -1];

extern struct TM2BufferInitRecord renderView0Frames[6];
/* Three contiguous pointer slots at 80180EE4, 80180EE8 and 80180EEC. */
extern struct TM2BufferInitRecord *D_80180EE4[3];

/* IDA extents agree with adjacent bases. Raw rtMainLoop ClearOTagR calls
 * pass 4096 entries; initializer limits leave 80 bytes of each packet bank. */
extern unsigned long renderPacketBuffer0[24400];
extern unsigned long renderPacketBuffer1[24400];
extern unsigned long renderOtView1Buf0[4096];
extern unsigned long renderOtView1Buf1[4096];
extern unsigned long renderOtView2Buf0[4096];
extern unsigned long renderOtView2Buf1[4096];

struct TM2BufferInitRecord *rtDoubleBufferInit(void);

#endif
