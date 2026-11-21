typedef void (*TM2CdCallback)(int,unsigned char *);
extern int CD_shell(void);
extern int CD_cw(unsigned int,unsigned char *,unsigned char *,int);
extern int CD_sync(int,unsigned char *);

/* @80176B18..80176B98. Four-byte flags, bounded by the following callback
 * cell. Commands 3,6,21,22,27 issue Setloc first when parameters are present. */
int D_80176B18[32] __attribute__((section(".tm2_80176B18"))) = {
    0,0,0,1,0,0,1,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,0, 0,0,0,1,0,0,0,0
};
/* @80176B98: sync callback, read/called by the CD completion machinery. */
TM2CdCallback D_80176B98 __attribute__((section(".tm2_80176B98"))) = 0;
/* @80176BA8: full status word written by the IRQ handler; these wrappers
 * inspect only its low byte. Adjacent SDK globals are separate objects. */
unsigned int D_80176BA8 __attribute__((section(".tm2_80176BA8"))) = 0;

/* Shared retry loop: up to four attempts; Setloc (2) first for the
 * parameter-bearing commands. 0 on success, -1 after four failures. */
static __inline__ int cd_command(unsigned char com,unsigned char *parameters,
    unsigned char *result,int fast)
{
    TM2CdCallback saved=D_80176B98;
    int cnt=4;
    while (cnt--) {
        if (com==16 && !(*(unsigned char *)&D_80176BA8&32)) {
            CD_shell();
            continue;
        }
        if (com!=1) {
            D_80176B98=0;
            CD_shell();
            if (parameters && D_80176B18[com] && CD_cw(2,parameters,result,0)!=0)
                continue;
            D_80176B98=saved;
        }
        if (CD_cw(com,parameters,result,fast)==0) return 0;
    }
    D_80176B98=saved;
    return -1;
}

/* @8013A72C. 1 when the command was accepted, 0 after four failures. */
int __attribute__((section(".text_8013A72C"))) CdControl(unsigned char com,
    unsigned char *parameters,unsigned char *result)
{
    return cd_command(com,parameters,result,0)+1;
}

/* @8013A874. Preliminary Setloc is synchronous; only the primary command
 * receives the fast flag. Neither call receives a result buffer. */
int __attribute__((section(".text_8013A874"))) CdControlF(unsigned char com,
    unsigned char *parameters)
{
    return cd_command(com,parameters,0,1)+1;
}

/* @8013A9B0. Sync only after command acceptance, with the result pointer.
 * Completion code 2 alone maps to success. */
int __attribute__((section(".text_8013A9B0"))) CdControlB(unsigned char com,
    unsigned char *parameters,unsigned char *result)
{
    if (cd_command(com,parameters,result,0)!=0) return 0;
    return CD_sync(0,result)==2;
}

/* @8013A6E4. Existing index label retained; this swaps the CD callback. */
TM2CdCallback __attribute__((section(".text_8013A6E4"))) PadStopCom_1(TM2CdCallback callback)
{
    TM2CdCallback previous=D_80176B98;
    D_80176B98=callback;
    return previous;
}

/* @8013A570. Byte access is intentional despite full-word shared storage. */
unsigned int __attribute__((section(".text_8013A570"))) sub_8013A570(void)
{
    return *(unsigned char *)&D_80176BA8;
}
