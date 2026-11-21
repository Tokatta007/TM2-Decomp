#include "semantic/rt_buffers.h"

/* @8018118C, 768 bytes. Startup @8013878C clears this complete range.
 * The EXE's zero padding covers only its first 712 bytes; that is not the
 * logical BSS boundary. The next IDA data label is 8018148C. */
struct TM2BufferInitRecord renderView0Frames[6];
