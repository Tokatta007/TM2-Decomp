typedef signed int s32;
typedef unsigned int u32;
typedef unsigned char u8;

u32 fxWakeObjects[5] __attribute__((section(".tm2_801879E8"))) = {0};
u32 objRegBlastAnims[5] __attribute__((section(".tm2_8018F6A8"))) = {0};

/* @80119598: install an object and mark its activity word clear. */
u32 __attribute__((section(".text_80119598"))) fxInstallWakeObject(u8 *object,s32 slot)
{
    u32 *table=fxWakeObjects;
    u8 *installed;
    table[slot]=(u32)object;
    installed=(u8 *)table[slot];   /* retail reads the table entry back */
    *(u32 *)(installed+8)=0;
    return (u32)installed;
}

/* @80124684: byte-seven state of an indexed object. */
u32 __attribute__((section(".text_80124684"))) objPoolGetStep(s32 slot)
{
    return *(u8 *)(objRegBlastAnims[slot]+7);
}
