union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
extern union TM2GpuEnvironment gpuSysState;
extern int (*gpuPrintf)(const char *,...);
extern char strBadRectFmt[12],strRectCoordsFmt[17],strNameColonFmt[4];

/* @80141538. This diagnoses rectangles; it does not clip or mutate them.
 * Level zero/other levels do not read the rectangle at all. Each formatter
 * call uses the current callback, and coordinates are read after call one. */
void __attribute__((section(".text_80141538"))) checkRECT(const char *name,const short *rect)
{
    switch (gpuSysState.bytes[2]) {
    case 1:
        if (rect[2]>gpuSysState.shorts[2] ||
            rect[2]+rect[0]>gpuSysState.shorts[2] ||
            rect[1]>gpuSysState.shorts[3] ||
            rect[1]+rect[3]>gpuSysState.shorts[3] ||
            rect[2]<=0 || rect[0]<0 || rect[1]<0 || rect[3]<=0) {
            gpuPrintf(strBadRectFmt,name);
            gpuPrintf(strRectCoordsFmt,rect[0],rect[1],rect[2],rect[3]);
        }
        break;
    case 2:
        gpuPrintf(strNameColonFmt,name);
        gpuPrintf(strRectCoordsFmt,rect[0],rect[1],rect[2],rect[3]);
        break;
    }
}
