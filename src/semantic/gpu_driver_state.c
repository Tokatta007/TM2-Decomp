typedef int (*TM2GpuOperation)();
struct TM2GpuDriver { char *revision; TM2GpuOperation operations[15]; };
extern int _addque(),_addque2(),_clr(),_ctl(),_cwb(),_cwc(),_drs(),_dws();
extern int _exeque(),_getctl(),_otc(),_param(),_reset(),_status(),_sync();
typedef char gpu_driver_size[(sizeof(struct TM2GpuDriver)==64)?1:-1];

/* @800D096C: original revision text referenced by the dispatch header. */
char strGpuSysRcsId[50] __attribute__((section(".tm2_800D096C"))) = "$Id: sys.c,v 1.116 1995/12/01 07:03:58 suzu Exp $";
/* @8017C2D4: one string pointer followed by fifteen operation pointers.
 * The array stores interfaces; each consumer supplies its actual call ABI. */
struct TM2GpuDriver gpuDriverJtb __attribute__((section(".tm2_8017C2D4"))) = {
    strGpuSysRcsId, {_addque,_addque2,_clr,_ctl,_cwb,_cwc,_drs,_dws,
                 _exeque,_getctl,_otc,_param,_reset,_status,_sync}
};
/* @8017C314: mutable, not a folded address constant. */
struct TM2GpuDriver *gpuDriver __attribute__((section(".tm2_8017C314"))) = &gpuDriverJtb;
/* @8017C39C / @8017C3B0: five four-byte entries; users store low halves. */
unsigned int gpuVramWidths[5] __attribute__((section(".tm2_8017C39C"))) = {1024,1024,1024,1024,1024};
unsigned int gpuVramHeights[5] __attribute__((section(".tm2_8017C3B0"))) = {512,1024,1024,512,1024};
char strResetGraphJtbFmt[30] __attribute__((section(".tm2_800D09A0"))) = "ResetGraph:jtb=%08x,env=%08x\n";
char strResetGraphModeFmt[19] __attribute__((section(".tm2_800D09C0"))) = "ResetGraph(%d)...\n";
