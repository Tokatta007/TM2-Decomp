/* Storage-view migration only; original function body retained. */
union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
extern union TM2GpuEnvironment gpuSysState;
#define D_8017C31E (gpuSysState.bytes[2])
/* cc: fill */
typedef signed char s8; typedef unsigned char u8; typedef signed short s16;typedef unsigned short u16; typedef signed int s32; typedef unsigned int u32;typedef long long s64; typedef unsigned long long u64;
typedef struct DrawObj { char pad0[8]; void (*f8)(); char pad12[24 - 12]; void *f24; } DrawObj;

extern void (*gpuPrintf)(void *, void *);
extern DrawObj *gpuDriver;
extern char D_800D0AF4[];
void DrawOTag(void *arg0) {
    DrawObj *p;
    if (D_8017C31E >= 2) {
        gpuPrintf(D_800D0AF4, arg0);
    }
    p = gpuDriver;
    ((void (*)(void *, void *, s32, s32))p->f8)(p->f24, arg0, 0, 0);
}
