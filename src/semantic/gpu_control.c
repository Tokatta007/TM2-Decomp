/* @8017C3EC..8017C400: writable hardware register-pointer cells.
 * Volatile applies to real MMIO, not to ordinary C storage or allocator shaping. */
volatile unsigned int *D_8017C3EC __attribute__((section(".tm2_8017C3EC"))) = (volatile unsigned int *)0x1F801810;
volatile unsigned int *D_8017C3F0 __attribute__((section(".tm2_8017C3F0"))) = (volatile unsigned int *)0x1F801814;
volatile unsigned int *D_8017C3F4 __attribute__((section(".tm2_8017C3F4"))) = (volatile unsigned int *)0x1F8010A0;
volatile unsigned int *D_8017C3F8 __attribute__((section(".tm2_8017C3F8"))) = (volatile unsigned int *)0x1F8010A4;
volatile unsigned int *D_8017C3FC __attribute__((section(".tm2_8017C3FC"))) = (volatile unsigned int *)0x1F8010A8;
/* @80193950..80193A50: _reset clears exactly 256 bytes. */
unsigned char D_80193950[256] __attribute__((section(".tm2_80193950"))) = {0};

/* @801427D4. Read the full GPU status word through the current pointer. */
unsigned int __attribute__((section(".text_801427D4"))) _status(void)
{
    return *D_8017C3F0;
}

/* @80142FE4. The reviewed display caller ignores incidental v0; the effects
 * are the hardware write and the one-byte software shadow. */
void __attribute__((section(".text_80142FE4"))) _ctl(unsigned int command)
{
    *D_8017C3F0=command;
    D_80193950[command>>24]=(unsigned char)command;
}

/* @80143010. The caller supplies a valid shadow index (0..255). */
unsigned int __attribute__((section(".text_80143010"))) _getctl(unsigned int index)
{
    return D_80193950[index];
}

/* @80143028. Disable GPU DMA direction even for an empty transfer. */
int __attribute__((section(".text_80143028"))) _cwb(const unsigned int *words,
    unsigned int count)
{
    *D_8017C3F0=0x04000000u;
    while (count--) *D_8017C3EC=*words++;
    return 0;
}

/* @80143078. Queue callers ignore v0; it incidentally held a register address. */
void __attribute__((section(".text_80143078"))) _cwc(unsigned int *head)
{
    *D_8017C3F0=0x04000002u;
    *D_8017C3F4=(unsigned int)head;
    *D_8017C3F8=0;
    *D_8017C3FC=0x01000401u;
}

/* @801430C4. Keep the command selector's upper bits and mask only the result. */
unsigned int __attribute__((section(".text_801430C4"))) _param(unsigned int selector)
{
    *D_8017C3F0=selector|0x10000000u;
    return *D_8017C3EC&0x00FFFFFFu;
}
