union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
union TM2GpuRect { unsigned int words[2]; short halves[4]; };
extern volatile union TM2GpuEnvironment gpuSysState;
extern unsigned int _param(unsigned int);
extern void _cwc(unsigned int *);

/* @80193910..80193940: one linked-list command workspace, cleared by CRT. */
unsigned int D_80193910[12] __attribute__((section(".tm2_80193910"))) = {0};

#define limitRange(x,l,h) ((x)=((x)<(l)?(l):(x)>(h)?(h):(x)))

/* @801428D4. Mutates the caller's dimensions. Unaligned fills use a tile
 * bracketed by drawing-area/offset commands; aligned fills use GP0(02h). */
int __attribute__((section(".text_801428D4"))) _clr(union TM2GpuRect *rect,
    unsigned int color)
{
    int value, limit, result, limit2;
    unsigned int *tail;
    value=rect->halves[2];
    if (value>=0) {
        if ((limit=gpuSysState.shorts[2]-1)<value) value=limit;
        result=value;
    } else
        result=0;
    rect->halves[2]=result;
    value=rect->halves[3];
    if (value>=0) {
        if ((limit2=gpuSysState.shorts[3]-1)<value) value=limit2;
    } else
        value=0;
    rect->halves[3]=value;
    if ((rect->halves[0]&63) || (rect->halves[2]&63)) {
        tail=&D_80193910[8];
        D_80193910[0]=((unsigned int)tail&0x00FFFFFFu)|0x07000000u;
        D_80193910[1]=0xE3000000u;
        D_80193910[2]=0xE4FFFFFFu;
        D_80193910[3]=0xE5000000u;
        D_80193910[4]=0xE6000000u;
        D_80193910[5]=(color&0x00FFFFFFu)|0x60000000u;
        D_80193910[6]=rect->words[0];
        D_80193910[7]=rect->words[1];
        *tail=0x03FFFFFFu;
        D_80193910[9]=_param(3)|0xE3000000u;
        D_80193910[10]=_param(4)|0xE4000000u;
        D_80193910[11]=_param(5)|0xE5000000u;
    } else {
        D_80193910[0]=0x04FFFFFFu;
        D_80193910[1]=0xE6000000u;
        D_80193910[2]=(color&0x00FFFFFFu)|0x02000000u;
        D_80193910[3]=rect->words[0];
        D_80193910[4]=rect->words[1];
    }
    _cwc(D_80193910);
    return 0;
}
