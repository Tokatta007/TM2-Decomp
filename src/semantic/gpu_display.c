union TM2GpuEnvironment { int words[32]; short shorts[64]; unsigned char bytes[128]; };
union TM2DispEnv { unsigned int words[5]; short shorts[10]; unsigned char bytes[20]; };
typedef int (*TM2GpuOperation)();
struct TM2GpuDriver { char *revision; TM2GpuOperation operations[15]; };
extern union TM2GpuEnvironment gpuSysState;
extern struct TM2GpuDriver *gpuDriver;
extern int (*gpuPrintf)(const char *,...);
extern int GsGetWorkBase(void);
extern void *memcpy(void *,const void *,unsigned int);
int get_dx(union TM2DispEnv *);
char strPutDispEnvFmt[20] __attribute__((section(".tm2_800D0B20"))) = "PutDispEnv(%08x)...\n";

#define limitRange(x,l,h) ((x)=((x)<(l)?(l):(x)>(h)?(h):(x)))
#define VBYTE(n) (*(volatile unsigned char *)&gpuSysState.bytes[n])

/* @80141BA0. The environment is writable: byte 18 caches video mode.
 * Each dispatch and cache comparison uses the state current at that call. */
union TM2DispEnv * __attribute__((section(".text_80141BA0"))) PutDispEnv(union TM2DispEnv *env)
{
    unsigned int mode=0x08000000u;
    int left,right,top,bottom,small;
    if (VBYTE(2)>=2) gpuPrintf(strPutDispEnvFmt,env);
    gpuDriver->operations[3](0x05000000u|((unsigned int)(gpuSysState.bytes[0]-1)<2u ?
        ((env->shorts[1]&0xFFF)<<12)|(get_dx(env)&0xFFF) :
        ((env->shorts[1]&0x3FF)<<10)|(env->shorts[0]&0x3FF)));
    if (gpuSysState.words[29]!=env->words[2] || gpuSysState.words[30]!=env->words[3]) {
        env->bytes[18]=GsGetWorkBase();
        left=env->shorts[4]*10+608;
        top=env->shorts[5]+(env->bytes[18]?19:16);
        right=left+(env->shorts[6]?env->shorts[6]*10:2560);
        bottom=top+(env->shorts[7]?env->shorts[7]:240);
        limitRange(left,500,3290);
        limitRange(right,left+80,3290);
        limitRange(top,0,env->bytes[18]?310:254);
        limitRange(bottom,top+1,env->bytes[18]?312:256);
        gpuDriver->operations[3](0x06000000u|((right&0xFFF)<<12)|(left&0xFFF));
        gpuDriver->operations[3](0x07000000u|((bottom&0x3FF)<<10)|(top&0x3FF));
    }
    if (gpuSysState.words[31]!=env->words[4] || gpuSysState.words[27]!=env->words[0] ||
        gpuSysState.words[28]!=env->words[1]) {
        env->bytes[18]=GsGetWorkBase();
        if (env->bytes[18]==1) mode|=8;
        if (env->bytes[17]) mode|=16;
        if (env->bytes[16]) mode|=32;
        if (VBYTE(3)) mode|=128;
        if (env->shorts[2]>280) {
            if (env->shorts[2]<=352) mode|=1;
            else if (env->shorts[2]<=400) mode|=64;
            else if (env->shorts[2]<=560) mode|=2;
            else mode|=3;
        }
        small=env->shorts[3]<(env->bytes[18]?289:257);
        if (!small) mode|=36;
        gpuDriver->operations[3](mode);
    }
    memcpy(gpuSysState.bytes+108,env,20);
    return env;
}

/* @80142718. Signed halves and truncation toward zero are significant. */
int __attribute__((section(".text_80142718"))) get_dx(union TM2DispEnv *env)
{
    switch (*(volatile unsigned char *)&gpuSysState.bytes[0]) {
    case 1:
        return gpuSysState.bytes[3] ? 1024-env->shorts[0]-env->shorts[2] : env->shorts[0];
    case 2:
        return gpuSysState.bytes[3] ? 1024-env->shorts[0]-env->shorts[2]/2 : env->shorts[0]/2;
    }
    return env->shorts[0];
}
