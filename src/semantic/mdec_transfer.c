/* MMIO pointees are volatile hardware; the pointer table is ordinary data. */
extern volatile unsigned int *mdecHwRegs[15];
extern char strMdecInSync[], strMdecOutSync[];
extern void timeout(char *);

/* @80139918 */
int __attribute__((section(".text_80139918"))) sub_80139918(void)
{
    volatile int remaining = 0x100000;
    while (*mdecHwRegs[13] & 0x20000000u) {
        if (--remaining == -1) {
            timeout(strMdecInSync);
            return -1;
        }
    }
    return 0;
}

/* @801399B0 */
int __attribute__((section(".text_801399B0"))) sub_801399B0(void)
{
    volatile int remaining = 0x100000;
    while (*mdecHwRegs[5] & 0x01000000u) {
        if (--remaining == -1) {
            timeout(strMdecOutSync);
            return -1;
        }
    }
    return 0;
}

/* @801397F4. Retail proceeds even if the synchronization call times out. */
void __attribute__((section(".text_801397F4"))) MDEC_in(unsigned int *input, unsigned int count)
{
    sub_80139918();
    *mdecHwRegs[14] |= 0x88;
    *mdecHwRegs[0] = (unsigned int)(input + 1);
    *mdecHwRegs[1] = ((count >> 5) << 16) | 0x20;
    *mdecHwRegs[12] = *input;
    *mdecHwRegs[2] = 0x01000201;
}

/* @80139888 */
void __attribute__((section(".text_80139888"))) MDEC_out(void *output, unsigned int count)
{
    sub_801399B0();
    *mdecHwRegs[14] |= 0x88;
    *mdecHwRegs[5] = 0;
    *mdecHwRegs[3] = (unsigned int)output;
    *mdecHwRegs[4] = ((count >> 5) << 16) | 0x20;
    *mdecHwRegs[5] = 0x01000200;
}
