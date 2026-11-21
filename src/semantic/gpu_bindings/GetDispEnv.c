/* Storage-view migration only; original function body retained. */
union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
extern union TM2GpuEnvironment gpuSysState;
#define D_8017C388 (*(char (*)[20])(gpuSysState.bytes+108))
/* cc: fill */
typedef signed char s8; typedef unsigned char u8; typedef signed short s16;typedef unsigned short u16; typedef signed int s32; typedef unsigned int u32;typedef long long s64; typedef unsigned long long u64;
extern void memcpy(void*, void*, s32);

void *GetDispEnv(void *arg0) {
    memcpy(arg0, &D_8017C388, 0x14);
    return arg0;
}
