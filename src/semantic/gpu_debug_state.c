union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
typedef char gpu_environment_size[(sizeof(union TM2GpuEnvironment)==128)?1:-1];
extern int printf(const char *,...);
/* @8017C31C..8017C39C: ResetGraph clears this complete 128-byte object.
 * Existing views include debug +2, limits +4/+6, callback +12,
 * 92-byte draw environment +16, and 20-byte display environment +108. */
union TM2GpuEnvironment gpuSysState __attribute__((section(".tm2_8017C31C"))) = {{0}};
/* @8017C318: writable logging callback, initialized to the real BIOS interface. */
int (*gpuPrintf)(const char *,...) __attribute__((section(".tm2_8017C318"))) = printf;
char strBadRectFmt[12] __attribute__((section(".tm2_800D0A70"))) = "%s:bad RECT";
char strRectCoordsFmt[17] __attribute__((section(".tm2_800D0A7C"))) = "(%d,%d)-(%d,%d)\n";
char strNameColonFmt[4] __attribute__((section(".tm2_800D0A90"))) = "%s:";
