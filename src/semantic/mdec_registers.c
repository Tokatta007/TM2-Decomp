/* @80164AB4: four DMA channel triplets, MDEC data/control and DMA control. */
volatile unsigned int *mdecHwRegs[15] __attribute__((section(".tm2_80164AB4"))) = {
    (volatile unsigned int *)0x1F801080, (volatile unsigned int *)0x1F801084,
    (volatile unsigned int *)0x1F801088, (volatile unsigned int *)0x1F801090,
    (volatile unsigned int *)0x1F801094, (volatile unsigned int *)0x1F801098,
    (volatile unsigned int *)0x1F8010A0, (volatile unsigned int *)0x1F8010A4,
    (volatile unsigned int *)0x1F8010A8, (volatile unsigned int *)0x1F8010B0,
    (volatile unsigned int *)0x1F8010B4, (volatile unsigned int *)0x1F8010B8,
    (volatile unsigned int *)0x1F801820, (volatile unsigned int *)0x1F801824,
    (volatile unsigned int *)0x1F8010F0
};
typedef char mdec_pointer_table_size[(sizeof(mdecHwRegs) == 60) ? 1 : -1];
/* @800D03EC / @800D03FC */
char strMdecInSync[] __attribute__((section(".tm2_800D03EC"))) = "MDEC_in_sync";
char strMdecOutSync[] __attribute__((section(".tm2_800D03FC"))) = "MDEC_out_sync";
