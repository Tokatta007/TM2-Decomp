/* Genuine GTE register interfaces; C owns pointer arguments and return ABI. */
#ifndef __mips__
#error These interfaces require the PSX GTE.
#endif

/* @8014A274: H control register; the GTE handles low-half sign extension. */
void __attribute__((section(".text_8014A274"))) sub_8014A274(unsigned int value)
{
    __asm__ volatile ("ctc2 %0,$26" : : "r"(value));
}

/* @8014D600: SZ1, SZ2, SZ3, in order even when destinations alias. */
void __attribute__((section(".text_8014D600"))) sub_8014D600(unsigned int *a,
    unsigned int *b,unsigned int *c)
{
    __asm__ volatile ("swc2 $17,0(%0)\n\tswc2 $18,0(%1)\n\tswc2 $19,0(%2)"
                      : : "r"(a),"r"(b),"r"(c) : "memory");
}

/* @8014D614: SZ0 through SZ3. Upper halfwords are zero on GTE reads. */
void __attribute__((section(".text_8014D614"))) sub_8014D614(unsigned int *a,
    unsigned int *b,unsigned int *c,unsigned int *d)
{
    __asm__ volatile ("swc2 $16,0(%0)\n\tswc2 $17,0(%1)\n\tswc2 $18,0(%2)\n\tswc2 $19,0(%3)"
                      : : "r"(a),"r"(b),"r"(c),"r"(d) : "memory");
}
