union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
typedef int (*TM2GpuOperation)();
struct TM2GpuDriver { char *revision; TM2GpuOperation operations[15]; };
extern union TM2GpuEnvironment gpuSysState;
extern struct TM2GpuDriver gpuDriverJtb,*gpuDriver;
extern unsigned int gpuVramWidths[5],gpuVramHeights[5];
extern char strResetGraphJtbFmt[30],strResetGraphModeFmt[19];
extern int (*gpuPrintf)(const char *,...);
extern int printf(const char *,...);
extern int ResetCallback(void),_reset(int);
extern void GPU_cw(unsigned int);
void sub_80143C24(unsigned char *,int,unsigned int);

/* @8014102C. Keep direct and dispatched reset paths distinct. Hardware
 * returns a type whose low byte selects one of the five limit entries. */
int __attribute__((section(".text_8014102C"))) ResetGraph(int mode)
{
    if ((mode&7)==0) {
        printf(strResetGraphJtbFmt,(unsigned int)&gpuDriverJtb,(unsigned int)&gpuSysState);
        sub_80143C24(gpuSysState.bytes,0,128);
        ResetCallback();
        GPU_cw((unsigned int)gpuDriver&0x00FFFFFFu);
        gpuSysState.bytes[0]=_reset(mode);
        gpuSysState.bytes[1]=1;
        gpuSysState.shorts[2]=(unsigned short)gpuVramWidths[gpuSysState.bytes[0]];
        gpuSysState.shorts[3]=(unsigned short)gpuVramHeights[gpuSysState.bytes[0]];
        sub_80143C24(gpuSysState.bytes+16,-1,92);
        sub_80143C24(gpuSysState.bytes+108,-1,20);
        return gpuSysState.bytes[0];
    }
    if (gpuSysState.bytes[2]>=2) gpuPrintf(strResetGraphModeFmt,mode);
    return gpuDriver->operations[12](1);
}

/* @80143C24. The caller provides a writable byte range; no word padding is
 * touched and zero length does not access the pointer. The raw v0 is unused. */
void __attribute__((section(".text_80143C24"))) sub_80143C24(unsigned char *dst,
    int value,unsigned int count)
{
    while (count--) *dst++=(unsigned char)value;
}
