typedef void (*TM2CdCallback)(int,unsigned char *);
typedef struct TM2CdLocation {
    unsigned char minute,second,sector,track;
} TM2CdLocation;
extern TM2CdCallback PadStopCom_1(TM2CdCallback);
extern int CdControlB(unsigned char com, unsigned char *, unsigned char *);
extern int printf(const char *,...);
int CdGetToc2(int,TM2CdLocation *);

/* @80176BA4: mutable signed SDK debug level. */
int D_80176BA4 __attribute__((section(".tm2_80176BA4"))) = 0;
char D_800D11C0[13] __attribute__((section(".tm2_800D11C0"))) = "track=%d,%d\n";
char strCdGetToc2Fmt[25] __attribute__((section(".tm2_800D11D0"))) = "CdGetToc2: %02x:%02x:00\n";
char strCdGetToc2Error[18] __attribute__((section(".tm2_800D11EC"))) = "CdGetToc2: error\n";

/* @8013A61C. Return the old full signed level. */
int __attribute__((section(".text_8013A61C"))) sub_8013A61C(int level)
{
    int previous=D_80176BA4;
    D_80176BA4=level;
    return previous;
}

/* @8015EE44. Preserve the otherwise-unused leading argument slot. */
int __attribute__((section(".text_8015EE44"))) CdGetToc(TM2CdLocation *toc)
{
    return CdGetToc2(1,toc);
}

#define btoi(b) ((b)/16*10+(b)%16)

/* @8015EE68. The first argument is unused in retail. Record zero is leadout;
 * track records follow at four-byte strides. The fourth byte is not written. */
int __attribute__((section(".text_8015EE68"))) CdGetToc2(int mode,TM2CdLocation *toc)
{
    unsigned char parameter[8],response[8];
    TM2CdCallback saved;
    int first,last,track,count,tracks;
    (void)mode;
    parameter[0]=1;
    saved=PadStopCom_1(0);
    if (!CdControlB(19,0,response)) goto error;
    first=btoi(response[1]);
    last=btoi(response[2]);
    if (D_80176BA4>=2) printf(D_800D11C0,first,last);
    parameter[0]=0;
    if (!CdControlB(20,parameter,response)) goto error;
    toc[0].minute=response[1];
    toc[0].second=response[2];
    toc[0].sector=0;
    count=1;
    for (track=first;track<=last;track++) {
        parameter[0]=(unsigned char)((track/10)*16+track%10);
        if (!CdControlB(20,parameter,response)) goto error;
        toc[count].minute=response[1];
        toc[count].second=response[2];
        toc[count].sector=0;
        count++;
    }
    tracks=count-1;
    if (D_80176BA4>=2) {
        for (count=0;count<=tracks;count++)
            printf(strCdGetToc2Fmt,toc[count].minute,toc[count].second);
    }
    PadStopCom_1(saved);
    return tracks;
error:
    if (D_80176BA4) printf(strCdGetToc2Error);
    PadStopCom_1(saved);
    return 0;
}
