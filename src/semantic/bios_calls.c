/* Genuine PSX BIOS jump-table interfaces (the PsyQ libapi/libc call stubs).
 *
 * Each stub is `li t2,TABLE; jr t2; li t1,FUNCTION`: a tail jump into the BIOS
 * A0h/B0h/C0h dispatcher with the function number in t1. The caller's
 * arguments, stack and RA are untouched, so the BIOS function returns straight
 * to the caller with its result in v0.
 *
 * Expressed in C for cc1:
 *  - `goto *bios_table` (labels-as-values indirect jump) ends the body with a
 *    barrier, so no `jr ra` epilogue and no prologue are emitted;
 *  - bios_fn is a global register variable on $9 (t1): stores to it are never
 *    dead, and reorg moves the store into the jump's delay slot;
 *  - adding the $0 register variable makes cc1 emit `addu reg,$0,N`
 *    (assembled as addiu, as in retail) instead of `li` (assembled as ori).
 * Nothing else in this file may use t1. */
#ifndef __mips__
#error These interfaces require the real PSX BIOS.
#endif

register int bios_fn asm("$9");

/* Function body only (the section attribute stays on each definition). */
#define BIOS_TAIL(table, number) \
    register int bios_zero asm("$0"); \
    register void *bios_table asm("$10"); \
    bios_table = (void *)(bios_zero + (table)); \
    bios_fn = bios_zero + (number); \
    goto *bios_table

/* @80138848: BIOS B0h function 38h */
void __attribute__((section(".text_80138848"))) exit(int status)
{
    BIOS_TAIL(0xB0, 0x38);
}

/* @80138858: BIOS A0h function 0Eh */
int __attribute__((section(".text_80138858"))) abs(int value)
{
    BIOS_TAIL(0xA0, 0x0E);
}

/* @80138868: BIOS A0h function 19h */
char * __attribute__((section(".text_80138868"))) strcpy(char *dst, const char *src)
{
    BIOS_TAIL(0xA0, 0x19);
}

/* @80138878: BIOS A0h function 2Ah */
void * __attribute__((section(".text_80138878"))) memcpy(void *dst, const void *src, int count)
{
    BIOS_TAIL(0xA0, 0x2A);
}

/* @80138888: BIOS A0h function 2Fh */
int __attribute__((section(".text_80138888"))) rand(void)
{
    BIOS_TAIL(0xA0, 0x2F);
}

/* @80138898: BIOS A0h function 30h */
void __attribute__((section(".text_80138898"))) srand(unsigned int seed)
{
    BIOS_TAIL(0xA0, 0x30);
}

/* @801388A8: BIOS A0h function 3Fh. Callers see `int printf(char *, ...)`;
 * the definition omits the `...` because a variadic cc1 body spills a0-a3
 * to the caller's frame before the jump. The stub never reads its arguments,
 * so the BIOS still receives all of them unchanged in a0-a3 and on the stack. */
int __attribute__((section(".text_801388A8"))) printf(char *format)
{
    BIOS_TAIL(0xA0, 0x3F);
}

/* @801393F8: BIOS A0h function 1Bh */
int __attribute__((section(".text_801393F8"))) strlen(const char *text)
{
    BIOS_TAIL(0xA0, 0x1B);
}

/* @80139408: BIOS A0h function 2Eh */
void * __attribute__((section(".text_80139408"))) memchr(const void *text, unsigned char value, int count)
{
    BIOS_TAIL(0xA0, 0x2E);
}

/* @8013D994: BIOS A0h function 17h */
int __attribute__((section(".text_8013D994"))) strcmp(const char *left, const char *right)
{
    BIOS_TAIL(0xA0, 0x17);
}

/* @8013D9A4: BIOS A0h function 18h */
int __attribute__((section(".text_8013D9A4"))) strncmp(const char *left, const char *right, int count)
{
    BIOS_TAIL(0xA0, 0x18);
}

/* @80144968: BIOS A0h function 2Bh */
void * __attribute__((section(".text_80144968"))) memset(void *dst, unsigned char value, int count)
{
    BIOS_TAIL(0xA0, 0x2B);
}

/* @80145A04: BIOS A0h function 13h */
int __attribute__((section(".text_80145A04"))) setjmp(int context[12])
{
    BIOS_TAIL(0xA0, 0x13);
}

/* @80145E10: BIOS A0h function 39h */
void __attribute__((section(".text_80145E10"))) InitHeap(unsigned long *base, unsigned long size)
{
    BIOS_TAIL(0xA0, 0x39);
}

/* @80145E20: BIOS A0h function 44h */
void __attribute__((section(".text_80145E20"))) FlushCache(void)
{
    BIOS_TAIL(0xA0, 0x44);
}

/* @80145E30: BIOS A0h function 49h */
void __attribute__((section(".text_80145E30"))) GPU_cw(unsigned int value)
{
    BIOS_TAIL(0xA0, 0x49);
}

/* @80145E50: BIOS B0h function 17h */
void __attribute__((section(".text_80145E50"))) ReturnFromException(void)
{
    BIOS_TAIL(0xB0, 0x17);
}

/* @80145E60: BIOS B0h function 18h */
void __attribute__((section(".text_80145E60"))) ResetEntryInt(void)
{
    BIOS_TAIL(0xB0, 0x18);
}

/* @80145E70: BIOS B0h function 19h */
void __attribute__((section(".text_80145E70"))) HookEntryInt(int context[12])
{
    BIOS_TAIL(0xB0, 0x19);
}

/* @80145EA0: BIOS B0h function 5Bh */
int __attribute__((section(".text_80145EA0"))) ChangeClearPAD(int mode)
{
    BIOS_TAIL(0xB0, 0x5B);
}

/* @80145EB0: BIOS C0h function 0Ah */
int __attribute__((section(".text_80145EB0"))) ChangeClearRCnt(int counter, int mode)
{
    BIOS_TAIL(0xC0, 0x0A);
}

/* @80145EE8: BIOS A0h function 34h */
void __attribute__((section(".text_80145EE8"))) free(void *ptr)
{
    BIOS_TAIL(0xA0, 0x34);
}

/* @80147C10: BIOS B0h function 07h */
void __attribute__((section(".text_80147C10"))) DeliverEvent(unsigned int event, unsigned int spec)
{
    BIOS_TAIL(0xB0, 0x07);
}

/* @80147D48: BIOS B0h function 08h */
int __attribute__((section(".text_80147D48"))) OpenEvent(unsigned int event, unsigned int spec, unsigned int mode, int (*handler)(void))
{
    BIOS_TAIL(0xB0, 0x08);
}

/* @80147D58: BIOS B0h function 0Ch */
int __attribute__((section(".text_80147D58"))) EnableEvent(int event)
{
    BIOS_TAIL(0xB0, 0x0C);
}

/* @80148280: BIOS B0h function 0Bh */
int __attribute__((section(".text_80148280"))) TestEvent(int event)
{
    BIOS_TAIL(0xB0, 0x0B);
}

/* @8014AA70: BIOS B0h function 12h */
int __attribute__((section(".text_8014AA70"))) InitPAD2(unsigned char *port0, int size0, unsigned char *port1, int size1)
{
    BIOS_TAIL(0xB0, 0x12);
}

/* @8014AA80: BIOS B0h function 13h */
void __attribute__((section(".text_8014AA80"))) StartPAD2(void)
{
    BIOS_TAIL(0xB0, 0x13);
}

/* @8014AA90: BIOS B0h function 14h */
void __attribute__((section(".text_8014AA90"))) StopPAD2(void)
{
    BIOS_TAIL(0xB0, 0x14);
}

/* @80155424: BIOS B0h function 09h */
int __attribute__((section(".text_80155424"))) CloseEvent(int event)
{
    BIOS_TAIL(0xB0, 0x09);
}

/* @80155434: BIOS B0h function 0Dh */
int __attribute__((section(".text_80155434"))) DisableEvent(int event)
{
    BIOS_TAIL(0xB0, 0x0D);
}

#undef BIOS_TAIL
