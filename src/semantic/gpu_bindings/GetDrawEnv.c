/* Storage-view migration only; original function body retained. */
union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
extern union TM2GpuEnvironment gpuSysState;
#define D_8017C32C (*(char (*)[92])(gpuSysState.bytes+16))
typedef signed char s8; typedef unsigned char u8; typedef signed short s16;typedef unsigned short u16; typedef signed int s32; typedef unsigned int u32;typedef long long s64; typedef unsigned long long u64;
/* cc: fill */
extern int memcpy(int, int, int);

int GetDrawEnv(int arg0) {
    memcpy(arg0, (int)D_8017C32C, 0x5C);
    return arg0;
}
