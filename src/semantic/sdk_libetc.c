/* PsyQ SDK libetc (interrupts, VSync, callbacks), reconstructed C.
 * Based on the MIT-licensed psxsdk sources of sotn-decomp (see THIRD_PARTY.md):
 * src/main/psxsdk/libetc/{intr.c,intr_vb.c,intr_dma.c,vsync.c}
 * SPDX-License-Identifier: MIT
 * Library version: intr.c v1.73 (1995/11/10 suzu).
 *
 * The library's static data lives at its retail addresses and is declared
 * extern by address (D_XXXXXXXX); the I/O register pointers are initialised
 * data in retail (e.g. D_8017E06C = 0x1F801074).
 *
 * Recipe: all 23 functions are byte-identical with cc1 psyq3.3 (aspsx 2.21),
 * or psyq3.5 cc1 with --aspsx 2.21; with psyq3.5 + aspsx 2.34 (the manifest
 * default) stopIntr and trapIntrVSync miss load-delay nops and do not fit. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef void (*Callback)();

struct Callbacks {
    const char *rcsid;
    Callback (*DMACallback)(int dma, Callback f);
    Callback (*InterruptCallback)(int irq, Callback f);
    void *(*ResetCallback)(void);
    void *(*StopCallback)(void);
    Callback (*VSyncCallbacks)(int ch, Callback f);
    void *(*RestartCallback)(void);
    void *intrEnv;
};

typedef struct {
    u16 interruptsInitialized;      /* +0x00 */
    u16 inInterrupt;                /* +0x02 */
    Callback handlers[11];          /* +0x04 */
    u16 enabledInterruptsMask;      /* +0x30 */
    u16 savedMask;                  /* +0x32 */
    s32 savedPcr;                   /* +0x34 */
    s32 buf[12];                    /* +0x38 jmp_buf */
    s32 stack[1024];                /* +0x68 */
} intrEnv_t;

#define JB_SP 1

/* ---- library data (retail addresses) ---- */
extern volatile s32 *D_8017CFCC;        /* vsync.c: 0x1F801814 (GPUSTAT) */
extern volatile s32 *D_8017CFD0;        /* vsync.c: 0x1F801110 (root counter 1) */
extern volatile s32 D_8017CFD4;         /* Hcount */
extern volatile s32 D_8017CFD8;         /* prevVcount */
extern intrEnv_t D_8017CFDC;            /* intrEnv */
extern struct Callbacks *D_8017E064;    /* pCallbacks */
extern volatile u16 *D_8017E068;        /* i_stat = 0x1F801070 */
extern volatile u16 *D_8017E06C;        /* i_mask = 0x1F801074 */
extern volatile s32 *D_8017E070;        /* d_pcr  = 0x1F8010F0 */
extern s32 D_8017E074;                  /* trapMissedCount */
extern Callback D_8017E078[8];          /* VSync callbacks */
extern volatile s32 D_8017E098;         /* Vcount */
extern volatile u32 *D_8017E09C;        /* 0x1F801114 (timer 1 mode) */
extern volatile u32 *D_8017E0A0;        /* 0x1F8010F4 (DICR) */
extern Callback D_8017E0A4[8];          /* DMA callbacks */
extern u32 *D_8017E0C4;                 /* 0x1F801080 (MADR) */

extern char D_800D0CF0[];               /* "VSync: timeout\n" */
extern char D_800D0D34[];               /* "unexpected interrupt(%04x)\n" */
extern char D_800D0D50[];               /* "intr timeout(%04x:%04x)\n" */
extern char D_800D0D6C[];               /* "DMA bus error: code=%08x\n" */
extern char D_800D0D88[];               /* "MADR[%d]=%08x\n" */

/* ---- other libraries ---- */
extern int printf(const char *, ...);
extern int sub_8013CF7C(const char *text);  /* puts */
extern int setjmp(int context[12]);
extern void HookEntryInt(int context[12]);
extern void ResetEntryInt(void);
extern void ReturnFromException(void);
extern int EnterCriticalSection(void);
extern int ExitCriticalSection(void);
extern int ChangeClearPAD(int mode);
extern int ChangeClearRCnt(int counter, int mode);
extern void sub_80145E40(void);             /* _96_remove */
extern void sub_801459D8(void *p, int n);   /* intr.c memclr */
extern void sub_80145B14(void *p, int n);   /* intr_vb.c memclr */
extern void sub_80145DBC(void *p, int n);   /* intr_dma.c memclr */

/* forward declarations within this file */
void sub_80145254(s32 count, s32 max);
Callback sub_80145320(int irq, Callback f);
void sub_80145564(void);
Callback (*sub_80145A14(void))(int, Callback);
void sub_80145A6C(void);
Callback sub_80145AE8(int index, Callback callback);
Callback (*sub_80145B40(void))(int, Callback);
void sub_80145B90(void);
Callback sub_80145D14(int index, Callback callback);

/* VSync(mode): wait for vertical blank / query counters. */
s32 __attribute__((section(".text_8014510C"))) sub_8014510C(s32 mode)
{
    s32 temp_s0;
    s32 ret;
    s32 max;
    s32 waitCount;

    temp_s0 = *D_8017CFCC;

    ret = (*D_8017CFD0 - D_8017CFD4) & 0xFFFF;
    if (mode < 0) {
        return D_8017E098;
    }
    if (mode == 1) {
        return ret;
    }
    if (mode > 0) {
        waitCount = 1;
        waitCount = (D_8017CFD8 - waitCount) + mode;
    } else {
        waitCount = D_8017CFD8;
    }
    max = 0;
    if (mode > 0) {
        max = mode - 1;
    }
    sub_80145254(waitCount, max);
    temp_s0 = *D_8017CFCC;
    sub_80145254(D_8017E098 + 1, 1);
    if (temp_s0 & 0x80000) {
        while (!((temp_s0 ^ *D_8017CFCC) & 0x80000000)) {
        }
    }
    D_8017CFD8 = D_8017E098;
    D_8017CFD4 = *D_8017CFD0;
    return ret;
}

/* v_wait(count, max): spin until Vcount reaches count, with timeout. */
void __attribute__((section(".text_80145254"))) sub_80145254(s32 count, s32 max)
{
    volatile s32 timeout[2];

    timeout[0] = max << 0xF;
    while (D_8017E098 < count) {
        if (timeout[0]-- == 0) {
            sub_8013CF7C(D_800D0CF0);
            ChangeClearPAD(0);
            ChangeClearRCnt(3, 0);
            return;
        }
    }
}

/* ResetCallback(): initialise the interrupt environment. */
void *__attribute__((section(".text_801452F0"))) sub_801452F0(void)
{
    return D_8017E064->ResetCallback();
}

/* InterruptCallback(irq, f): install an interrupt handler. */
Callback __attribute__((section(".text_80145320"))) sub_80145320(int irq, Callback f)
{
    return D_8017E064->InterruptCallback(irq, f);
}

/* DMACallback(dma, f): install a DMA-complete handler. */
Callback __attribute__((section(".text_80145350"))) sub_80145350(int dma, Callback f)
{
    return D_8017E064->DMACallback(dma, f);
}

/* VSyncCallback(f): install vblank callback slot 0. */
void __attribute__((section(".text_80145380"))) sub_80145380(Callback f)
{
    D_8017E064->VSyncCallbacks(0, f);
}

/* VSyncCallbacks(ch, f): install vblank callback slot ch. */
Callback __attribute__((section(".text_801453B4"))) sub_801453B4(int ch, Callback f)
{
    return D_8017E064->VSyncCallbacks(ch, f);
}

/* StopCallback(): stop interrupt handling. */
void __attribute__((section(".text_801453E4"))) sub_801453E4(void)
{
    D_8017E064->StopCallback();
}

/* RestartCallback(): resume interrupt handling. */
long __attribute__((section(".text_80145414"))) sub_80145414(void)
{
    return (long)D_8017E064->RestartCallback();
}

/* CheckCallback(): nonzero while inside an interrupt handler. */
int __attribute__((section(".text_80145444"))) sub_80145444(void)
{
    return D_8017CFDC.inInterrupt;
}

/* GetIntrMask(): read the interrupt mask register. */
u16 __attribute__((section(".text_80145454"))) sub_80145454(void)
{
    return *D_8017E06C;
}

/* SetIntrMask(mask): write the interrupt mask register, return old. */
u16 __attribute__((section(".text_8014546C"))) sub_8014546C(u16 arg0)
{
    u16 mask;

    mask = *D_8017E06C;
    *D_8017E06C = arg0;
    return mask;
}

/* startIntr(): set up intrEnv, hook the BIOS interrupt entry. */
intrEnv_t *__attribute__((section(".text_80145488"))) sub_80145488(void)
{
    if (D_8017CFDC.interruptsInitialized) {
        return 0;
    }
    *D_8017E068 = *D_8017E06C = 0;
    *D_8017E070 = 0x33333333;
    sub_801459D8(&D_8017CFDC, sizeof(D_8017CFDC) / sizeof(s32));
    if (setjmp(D_8017CFDC.buf) != 0) {
        sub_80145564();
    }
    D_8017CFDC.buf[JB_SP] = (s32)&D_8017CFDC.stack[1004];
    HookEntryInt(D_8017CFDC.buf);
    D_8017CFDC.interruptsInitialized = 1;
    D_8017E064->VSyncCallbacks = sub_80145A14();
    /* called through a long long (v0:v1) prototype, as psy-q-decomp does:
     * keeps v1 live so pCallbacks is reloaded into a0 as in retail */
    D_8017E064->DMACallback = (Callback (*)(int, Callback))((long long (*)(void))sub_80145B40)();
    sub_80145E40();
    ExitCriticalSection();
    return &D_8017CFDC;
}

/* trapIntr(): interrupt dispatcher (runs on the intrEnv stack). */
void __attribute__((section(".text_80145564"))) sub_80145564(void)
{
    int i;
    u16 mask;

    if (!D_8017CFDC.interruptsInitialized) {
        printf(D_800D0D34, *D_8017E068);
        ReturnFromException();
    }
    D_8017CFDC.inInterrupt = 1;
    mask = (D_8017CFDC.enabledInterruptsMask & *D_8017E068) & *D_8017E06C;
    while (mask != 0) {
        for (i = 0; mask && i < 11; ++i, mask >>= 1) {
            if (mask & 1) {
                *D_8017E068 = ~(1 << i);
                if (D_8017CFDC.handlers[i] != 0) {
                    D_8017CFDC.handlers[i]();
                }
            }
        }
        mask = (D_8017CFDC.enabledInterruptsMask & *D_8017E068) & *D_8017E06C;
    }
    if (*D_8017E068 & *D_8017E06C) {
        if (D_8017E074++ > 0x800) {
            printf(D_800D0D50, *D_8017E068, *D_8017E06C);
            D_8017E074 = 0;
            *D_8017E068 = 0;
        }
    } else {
        D_8017E074 = 0;
    }
    D_8017CFDC.inInterrupt = 0;
    ReturnFromException();
}

/* setIntr(irq, handler): set/clear an interrupt handler and its mask bit. */
Callback __attribute__((section(".text_8014574C"))) sub_8014574C(int irq, Callback handler)
{
    Callback prevHandler;
    int mask;

    prevHandler = D_8017CFDC.handlers[irq];
    if (handler != prevHandler && D_8017CFDC.interruptsInitialized) {
        mask = *D_8017E06C;
        *D_8017E06C = 0;
        if (handler != 0) {
            D_8017CFDC.handlers[irq] = handler;
            mask = mask | (1 << irq);
            D_8017CFDC.enabledInterruptsMask |= 1 << irq;
        } else {
            D_8017CFDC.handlers[irq] = 0;
            mask = mask & ~(1 << irq);
            D_8017CFDC.enabledInterruptsMask &= ~(1 << irq);
        }
        if (irq == 0) {
            ChangeClearPAD(handler == 0);
            ChangeClearRCnt(3, handler == 0);
        }
        if (irq == 4) {
            ChangeClearRCnt(0, handler == 0);
        }
        if (irq == 5) {
            ChangeClearRCnt(1, handler == 0);
        }
        if (irq == 6) {
            ChangeClearRCnt(2, handler == 0);
        }
        *D_8017E06C = mask;
    }
    return prevHandler;
}

/* stopIntr(): save mask/DPCR and unhook the interrupt entry. */
intrEnv_t *__attribute__((section(".text_801458A0"))) sub_801458A0(void)
{
    if (!D_8017CFDC.interruptsInitialized) {
        return 0;
    }
    EnterCriticalSection();
    D_8017CFDC.savedMask = *D_8017E06C;
    D_8017CFDC.savedPcr = *D_8017E070;
    *D_8017E068 = *D_8017E06C = 0;
    *D_8017E070 &= 0x77777777;
    ResetEntryInt();
    D_8017CFDC.interruptsInitialized = 0;
    return &D_8017CFDC;
}

/* restartIntr(): re-hook the interrupt entry, restore mask/DPCR. */
intrEnv_t *__attribute__((section(".text_80145950"))) sub_80145950(void)
{
    if (D_8017CFDC.interruptsInitialized) {
        return 0;
    }

    HookEntryInt(D_8017CFDC.buf);
    D_8017CFDC.interruptsInitialized = 1;
    *D_8017E06C = D_8017CFDC.savedMask;
    *D_8017E070 = D_8017CFDC.savedPcr;
    ExitCriticalSection();
    return &D_8017CFDC;
}

/* startIntrVSync(): reset Vcount/callbacks, hook IRQ0; returns setIntrVSync. */
Callback (*__attribute__((section(".text_80145A14"))) sub_80145A14(void))(int, Callback)
{
    *D_8017E09C = 0x107;
    D_8017E098 = 0;
    sub_80145B14(D_8017E078, 8);
    sub_80145320(0, sub_80145A6C);
    return sub_80145AE8;
}

/* trapIntrVSync(): vblank IRQ: bump Vcount, run the 8 vblank callbacks. */
void __attribute__((section(".text_80145A6C"))) sub_80145A6C(void)
{
    int i;

    D_8017E098 += 1;
    (void)D_8017E098;

    for (i = 0; i < 8; i++) {
        if (D_8017E078[i] != 0) {
            D_8017E078[i]();
        }
    }
}

/* setIntrVSync(index, callback): set a vblank callback, return the old one. */
Callback __attribute__((section(".text_80145AE8"))) sub_80145AE8(int index, Callback callback)
{
    Callback ret = D_8017E078[index];
    if (callback != ret) {
        D_8017E078[index] = callback;
    }
    return ret;
}

/* startIntrDMA(): clear DMA callbacks/DICR, hook IRQ3; returns setIntrDMA. */
Callback (*__attribute__((section(".text_80145B40"))) sub_80145B40(void))(int, Callback)
{
    sub_80145DBC(D_8017E0A4, 8);
    *D_8017E0A0 = 0;
    sub_80145320(3, sub_80145B90);
    return sub_80145D14;
}

/* trapIntrDMA(): DMA IRQ: acknowledge channels and run their callbacks. */
void __attribute__((section(".text_80145B90"))) sub_80145B90(void)
{
    u32 mask;
    int i;

    while ((mask = (*D_8017E0A0 >> 24) & 0x7f) != 0) {
        for (i = 0; mask != 0 && i < 7; i++, mask >>= 1) {
            if (mask & 1) {
                *D_8017E0A0 &= (0xffffff | (1 << (i + 24)));
                if (D_8017E0A4[i] != 0) {
                    D_8017E0A4[i]();
                }
            }
        }
    }

    if ((*D_8017E0A0 & 0xFF000000) == 0x80000000 || *D_8017E0A0 & 0x8000) {
        printf(D_800D0D6C, *D_8017E0A0);
        for (i = 0; i < 7; i++) {
            printf(D_800D0D88, i, D_8017E0C4[4 * i]);
        }
    }
}

/* setIntrDMA(index, callback): set a DMA callback and its DICR enable bit. */
Callback __attribute__((section(".text_80145D14"))) sub_80145D14(int index, Callback callback)
{
    Callback prev = D_8017E0A4[index];
    if (callback != prev) {
        if (callback != 0) {
            D_8017E0A4[index] = callback;
            *D_8017E0A0 =
                (*D_8017E0A0 & 0xFFFFFF) | 0x800000 | (1 << (index + 16));
        } else {
            D_8017E0A4[index] = 0;
            *D_8017E0A0 =
                ((*D_8017E0A0 & 0xFFFFFF) | 0x800000) & ~(1 << (index + 16));
        }
    }
    return prev;
}
