/* Storage-view migration only; original function body retained. */
union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
extern union TM2GpuEnvironment gpuSysState;
#define D_8017C31E (gpuSysState.bytes[2])
#define D_8017C328 (gpuSysState.words[3])
/* cc: fill */
typedef signed char s8; typedef unsigned char u8; typedef signed short s16;typedef unsigned short u16; typedef signed int s32; typedef unsigned int u32;typedef long long s64; typedef unsigned long long u64;
extern char gpuPrintf[];

extern char aDrawsynccallba[];

s32 DrawSyncCallback(s32 arg0) {
    s32 temp_v0;
    if (*(u8 *)&D_8017C31E >= 2) {
        (*(void (**)(char *, s32)) &gpuPrintf)(aDrawsynccallba, arg0);
    }
    temp_v0 = D_8017C328;
    D_8017C328 = arg0;
    return temp_v0;
}
