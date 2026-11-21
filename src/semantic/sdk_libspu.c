/* PsyQ SDK libspu (SPU low level, transfer, memory allocator, voices), reconstructed C.
 * Adapted from the MIT-licensed psxsdk sources of sotn-decomp, src/main/psxsdk/libspu/
 * {spu.c, s_m_m.c, s_m_f.c, s_m_init.c, s_m_util.c, s_ini.c, s_i.c, s_q.c, s_it.c,
 *  s_itc.c, s_w.c, s_stsa.c, s_stm.c, s_sca.c, s_sr.c, s_sva.c, s_n2p.c, s_dcb.c, s_snv.c}
 * and psy-q-decomp src/spu/{s_sav.c, s_gks.c} (_SpuSetAnyVoice, SpuGetKeyStatus),
 * see THIRD_PARTY.md.
 * SPDX-License-Identifier: MIT
 *
 * TM2's libspu is the PsyQ 3.4 (Dec 1995) release: _SpuInit has no centerNote
 * reset loop, and SpuSetCommonAttr/_SpuSetVoiceAttr do not pre-zero the volumes.
 * Recipe psyq3.3 cc1 + aspsx 2.21: 33/33 byte-identical. (The nop after the
 * dead key_on/key_off[0] reads in _spu_init is aspsx 2.21's load/overwrite
 * hazard nop, emitted by tools/maspsx for --aspsx-version=2.21.) _SpuSetVoiceAttr (80161C70) lives in its own unit,
 * sdk_libspu_sva.c, because its jump tables sit away from this file's.
 *
 * The library's static data lives at its retail addresses and is declared
 * extern by address (D_XXXXXXXX); the SDK names are #defined onto them. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* Request 34 (docs/SPU_VOICES.md): with --port-define TM2_PORT_SPU48 voices 24..47 have their registers in a
 * second block (TM2_SPU_EXT_REGS) and a libspu voice mask with TM2_SPU_EXT_MASK set names them. Without it
 * nothing here changes. */
#include "semantic/spu_voices.h"

typedef char *va_list;
#define __va_rounded_size(TYPE) \
    (((sizeof(TYPE) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))
#define va_start(AP, LASTARG) \
    (AP = ((char *)&(LASTARG) + __va_rounded_size(LASTARG)))
#define va_arg(AP, TYPE) \
    (AP += __va_rounded_size(TYPE), \
     *((TYPE *)(AP - __va_rounded_size(TYPE))))

#define NULL 0
#define NUM_SPU_CHANNELS 24

typedef struct {
    s16 left;
    s16 right;
} SpuVolume;

typedef struct {
    u32 mask;
    s32 mode;
    SpuVolume depth;
    s32 delay;
    s32 feedback;
} SpuReverbAttr;

typedef struct tagSpuMalloc {
    u32 addr;
    u32 size;
} SPU_MALLOC;

typedef struct tagSpuVoiceRegister {
    SpuVolume volume; /* 0 */
    u16 pitch;        /* 4 */
    u16 addr;         /* 6 */
    u16 adsr[2];      /* 8 */
    u16 volumex;      /* C */
    u16 loop_addr;    /* E */
} SPU_VOICE_REG;

typedef struct tagSpuControl {
    SPU_VOICE_REG voice[NUM_SPU_CHANNELS];
    SpuVolume main_vol;  /* 180 */
    SpuVolume rev_vol;   /* 184 */
    u16 key_on[2];       /* 188 */
    u16 key_off[2];      /* 18C */
    u16 chan_fm[2];      /* 190 */
    u16 noise_mode[2];   /* 194 */
    u16 rev_mode[2];     /* 198 */
    u32 chan_on;         /* 19C */
    u16 unk;             /* 1A0 */
    u16 rev_work_addr;   /* 1A2 */
    u16 irq_addr;        /* 1A4 */
    u16 trans_addr;      /* 1A6 */
    u16 trans_fifo;      /* 1A8 */
    u16 spucnt;          /* 1AA */
    u16 data_trans;      /* 1AC */
    u16 spustat;         /* 1AE */
    SpuVolume cd_vol;    /* 1B0 */
    SpuVolume ex_vol;    /* 1B4 */
    SpuVolume main_volx; /* 1B8 */
    SpuVolume unk_vol;   /* 1BC */
    u16 rev[32];         /* 1C0 reverb registers */
} SPU_RXX;

union SpuUnion {
    volatile SPU_RXX rxx;
    volatile u16 raw[0x100];
};

/* ---- library data (retail addresses) ---- */
extern s32 D_8017E0D4;                 /* wait counter */
extern u16 D_8017E0D8;                 /* _spu_tsa */
extern union SpuUnion *D_8017E0DC;     /* _spu_RXX = 0x1F801C00 */
extern s32 *D_8017E0E0;                /* DMA4 MADR */
extern s32 *D_8017E0E4;                /* DMA4 BCR */
extern s32 *D_8017E0E8;                /* DMA4 CHCR */
extern s32 *D_8017E0EC;                /* DPCR */
extern s32 *D_8017E0F0;                /* SPU delay/size register */
extern s32 D_8017E0F4;                 /* _spu_transMode */
extern s32 D_8017E0F8;                 /* _spu_addrMode */
extern s32 D_8017E0FC;                 /* _spu_mem_mode */
extern s32 D_8017E100;                 /* _spu_mem_mode_plus */
extern s32 D_8017E104;                 /* _spu_mem_mode_unit */
extern s32 D_8017E108;                 /* _spu_mem_mode_unitM */
extern s32 spuInTransfer;                 /* _spu_inTransfer */
extern void (*volatile D_8017E110)();  /* _spu_transferCallback */
extern void (*volatile D_8017E114)();  /* _spu_IRQCallback */
extern u16 D_8017E118[];               /* zero block for SPU 0x1000 */
extern s32 D_8017E128;                 /* DMA direction (1 = read) */
extern s32 D_8017E12C;                 /* DMA address */
extern s32 D_8017E130;                 /* DMA block count */
extern s32 D_8017E134;
extern s32 D_8017E138;                 /* _spu_trans_mode */
extern s32 D_8017E13C;                 /* _spu_rev_flag */
extern s32 D_8017E140;                 /* _spu_rev_reserve_wa */
extern SpuReverbAttr D_8017E144;       /* _spu_rev_attr */
extern s32 D_8017E158;                 /* _spu_rev_offsetaddr */
extern s32 D_8017E18C;                 /* _spu_keystat */
extern s32 D_8017E190;                 /* _spu_EVdma */
extern s32 D_8017E594;                 /* _spu_isCalled */
extern s32 D_8017E598;                 /* _spu_AllocBlockNum */
extern s32 D_8017E59C;                 /* _spu_AllocLastNum */
extern SPU_MALLOC *D_8017E5A0;         /* _spu_memList */
extern s32 D_8017E5A4[];               /* _spu_rev_startaddr */
extern u16 D_80180850[];               /* note -> pitch table (12 x 16 steps) */

extern char D_800D0D98[];              /* "SPU:T/O [%s]\n" */
extern char D_800D0DA8[];              /* "wait (reset)" */
extern char D_800D0DB8[];              /* "wait (wrdy H -> L)" */
extern char D_800D0DCC[];              /* "wait (dmaf clear/W)" */

#define _spu_tsa D_8017E0D8
#define _spu_RXX D_8017E0DC
#define _spu_transMode D_8017E0F4
#define _spu_addrMode D_8017E0F8
#define _spu_mem_mode D_8017E0FC
#define _spu_mem_mode_plus D_8017E100
#define _spu_mem_mode_unit D_8017E104
#define _spu_mem_mode_unitM D_8017E108
#define _spu_inTransfer spuInTransfer
#define _spu_transferCallback D_8017E110
#define _spu_IRQCallback D_8017E114
#define _spu_trans_mode D_8017E138
#define _spu_rev_flag D_8017E13C
#define _spu_rev_reserve_wa D_8017E140
#define _spu_rev_attr D_8017E144
#define _spu_rev_offsetaddr D_8017E158
#define _spu_keystat D_8017E18C
#define _spu_EVdma D_8017E190
#define _spu_isCalled D_8017E594
#define _spu_AllocBlockNum D_8017E598
#define _spu_AllocLastNum D_8017E59C
#define _spu_memList D_8017E5A0
#define _spu_rev_startaddr D_8017E5A4

#if TM2_SPU_VOICES > 24
/* voices 24..47: their register block (the same layout; only the voices and the key / reverb / ENDX words) */
#define _spu_RXX2 ((union SpuUnion *)TM2_SPU_EXT_REGS)
#endif

/* ---- other libraries ---- */
typedef void (*Callback)();
extern int printf(const char *, ...);
extern int EnterCriticalSection(void);
extern int ExitCriticalSection(void);
extern int OpenEvent(unsigned int, unsigned int, unsigned int, int (*)(void));
extern int EnableEvent(int);
extern int TestEvent(int);
extern void DeliverEvent(unsigned int, unsigned int);
extern int CloseEvent(int);
extern int DisableEvent(int);
extern void *sub_801452F0(void);                     /* ResetCallback */
extern Callback sub_80145350(int dma, Callback f);  /* DMACallback */

/* ---- this library ---- */
s32 sub_80146B28(s32 mode);
s32 sub_80147070(u8 *addr, u32 size);
void sub_801473B4(void);
s32 sub_80147750(s32 mode, ...);
void sub_80147AE8(s32 reg, u32 value, s32 shift);
u32 sub_80147B30(s32 reg, u32 addr);
void sub_80147CC4(void);
void sub_80147D68(void (*func)());
void sub_80147D8C(void);
s32 sub_801479F8(u8 *addr, u32 size);
s32 sub_8015D53C(s32 on_off, u32 bits, s32 addr1, s32 addr2);
s32 sub_8015D780(u32 addr);

/* SpuSetCommonAttr's two volume-mode jump tables (0x800D0DE0, 0x800D0E00). */
#pragma tm2_rdata .tm2_800D0DE0

#define WASTE_TIME()                     \
    sp4 = 0xD;                           \
    for (sp0 = 0; sp0 < 0xF0; sp0++) {   \
        sp4 *= 3;                        \
    }

#define DELAY_W() (*D_8017E0F0 = (*D_8017E0F0 & 0xF0FFFFFF) | 0x20000000)
#define DELAY_R() (*D_8017E0F0 = (*D_8017E0F0 & 0xF0FFFFFF) | 0x22000000)

/* SpuMalloc(size): allocate SPU RAM from the block list. */
long __attribute__((section(".text_801467D0"))) sub_801467D0(long size)
{
    s32 var_s2;
    s32 var_s3;
    s32 i;

    i = 0;
    var_s2 = -1;

    if (_spu_rev_reserve_wa == 0) {
        var_s3 = 0;
    } else {
        var_s3 = (0x10000 - _spu_rev_offsetaddr) << _spu_mem_mode_plus;
    }

    size += (size & ~_spu_mem_mode_unitM) ? _spu_mem_mode_unitM : 0;
    size >>= _spu_mem_mode_plus;
    size <<= _spu_mem_mode_plus;

    if (_spu_memList[0].addr & 0x40000000) {
        var_s2 = 0;
    } else {
        sub_80147D8C();

        for (; i < _spu_AllocBlockNum; i++) {
            if (_spu_memList[i].addr & 0x40000000 ||
                (_spu_memList[i].addr & 0x80000000 &&
                 _spu_memList[i].size >= size)) {
                var_s2 = i;
                break;
            }
        }
    }

    if (var_s2 == -1)
        return -1;

    if (_spu_memList[var_s2].addr & 0x40000000) {
        if (var_s2 < _spu_AllocBlockNum && _spu_memList[var_s2].size - var_s3 >= size) {
            s32 next = var_s2 + 1;

            _spu_memList[next].addr =
                (*(volatile int *)&_spu_memList[var_s2].addr & 0x0FFFFFFF) + size | 0x40000000;
            _spu_memList[next].size = _spu_memList[var_s2].size - size;

            _spu_memList[var_s2].addr &= 0x0FFFFFFF;
            _spu_memList[var_s2].size = size;

            _spu_AllocLastNum = next;
            sub_80147D8C();

            return _spu_memList[var_s2].addr;
        }
    } else {
        if (size < _spu_memList[var_s2].size && _spu_AllocLastNum < _spu_AllocBlockNum) {
            u32 _addr = _spu_memList[var_s2].addr + size;
            u32 _size = _spu_memList[var_s2].size - size;

            u32 swapAddr = _spu_memList[_spu_AllocLastNum].addr;
            u32 swapSize = _spu_memList[_spu_AllocLastNum].size;

            _spu_memList[_spu_AllocLastNum].addr = _addr | 0x80000000;
            _spu_memList[_spu_AllocLastNum].size = _size;

            _spu_memList[_spu_AllocLastNum + 1].addr = swapAddr;
            _spu_memList[_spu_AllocLastNum + 1].size = swapSize;

            _spu_AllocLastNum++;
        }

        _spu_memList[var_s2].size = size;
        _spu_memList[var_s2].addr &= 0x0FFFFFFF;
        sub_80147D8C();

        return _spu_memList[var_s2].addr;
    }
    return -1;
}

/* _spu_reset(): pulse the SPU control register. */
s32 __attribute__((section(".text_80146A98"))) sub_80146A98(void)
{
    volatile s32 sp0;
    volatile s32 sp4;
    u16 cnt;

    cnt = _spu_RXX->rxx.spucnt;
    _spu_RXX->rxx.spucnt = cnt & 0x7FCF;
    WASTE_TIME();
    cnt &= 0xFFCF;
    _spu_RXX->rxx.spucnt = cnt;
    return 0;
}

/* _spu_init(mode): reset SPU registers (mode 0 = cold: clear voices and SPU RAM head). */
s32 __attribute__((section(".text_80146B28"))) sub_80146B28(s32 mode)
{
    volatile s32 sp0;
    volatile s32 sp4;
    s32 channel;
    u16 cnt;

    *D_8017E0EC |= 0xB0000;
    _spu_RXX->rxx.main_vol.left = 0;
    _spu_RXX->rxx.main_vol.right = 0;
    _spu_RXX->rxx.spucnt = 0;
    _spu_transMode = 0;
    _spu_addrMode = 0;
    _spu_tsa = 0;
    WASTE_TIME();
    _spu_RXX->rxx.main_vol.left = 0;
    _spu_RXX->rxx.main_vol.right = 0;
    D_8017E0D4 = 0;
    while (_spu_RXX->rxx.spustat & 0x7FF) {
        if (++D_8017E0D4 > 5000) {
            printf(D_800D0D98, D_800D0DA8);
            break;
        }
    }
    _spu_mem_mode = 2;
    _spu_mem_mode_plus = 3;
    _spu_mem_mode_unit = 8;
    _spu_mem_mode_unitM = 7;
    _spu_RXX->rxx.data_trans = 4;
    _spu_RXX->rxx.rev_vol.left = 0;
    _spu_RXX->rxx.rev_vol.right = 0;
    _spu_RXX->rxx.key_off[0] = 0xFFFF;
    _spu_RXX->rxx.key_off[1] = 0xFFFF;
    _spu_RXX->rxx.rev_mode[0] = 0;
    _spu_RXX->rxx.rev_mode[1] = 0;
#if TM2_SPU_VOICES > 24
    _spu_RXX2->rxx.key_off[0] = 0xFFFF;
    _spu_RXX2->rxx.key_off[1] = 0xFFFF;
    _spu_RXX2->rxx.rev_mode[0] = 0;
    _spu_RXX2->rxx.rev_mode[1] = 0;
#endif
    if (mode == 0) {
        _spu_RXX->rxx.chan_fm[0] = 0;
        _spu_RXX->rxx.chan_fm[1] = 0;
        _spu_RXX->rxx.noise_mode[0] = 0;
        _spu_RXX->rxx.noise_mode[1] = 0;
        _spu_RXX->rxx.cd_vol.left = 0;
        _spu_RXX->rxx.cd_vol.right = 0;
        _spu_RXX->rxx.ex_vol.left = 0;
        _spu_RXX->rxx.ex_vol.right = 0;
        _spu_tsa = 0x200;
        sub_80147070((u8 *)D_8017E118, 0x10);
        for (channel = 0; channel < NUM_SPU_CHANNELS; channel++) {
            _spu_RXX->rxx.voice[channel].volume.left = 0;
            _spu_RXX->rxx.voice[channel].volume.right = 0;
            _spu_RXX->rxx.voice[channel].pitch = 0x3fff;
            _spu_RXX->rxx.voice[channel].addr = 0x200;
            _spu_RXX->rxx.voice[channel].adsr[0] = 0;
            _spu_RXX->rxx.voice[channel].adsr[1] = 0;
        }
#if TM2_SPU_VOICES > 24
        for (channel = 0; channel < TM2_SPU_VOICES - 24; channel++) {
            _spu_RXX2->rxx.voice[channel].volume.left = 0;
            _spu_RXX2->rxx.voice[channel].volume.right = 0;
            _spu_RXX2->rxx.voice[channel].pitch = 0x3fff;
            _spu_RXX2->rxx.voice[channel].addr = 0x200;
            _spu_RXX2->rxx.voice[channel].adsr[0] = 0;
            _spu_RXX2->rxx.voice[channel].adsr[1] = 0;
        }
#endif
        cnt = _spu_RXX->rxx.key_on[0];
        _spu_RXX->rxx.key_on[0] = 0xFFFF;
        _spu_RXX->rxx.key_on[1] |= 0xFF;
#if TM2_SPU_VOICES > 24
        _spu_RXX2->rxx.key_on[0] = 0xFFFF;
        _spu_RXX2->rxx.key_on[1] = 0xFF;
#endif
        WASTE_TIME();
        WASTE_TIME();
        WASTE_TIME();
        WASTE_TIME();
        cnt = _spu_RXX->rxx.key_off[0];
        _spu_RXX->rxx.key_off[0] = 0xFFFF;
        _spu_RXX->rxx.key_off[1] |= 0xFF;
#if TM2_SPU_VOICES > 24
        _spu_RXX2->rxx.key_off[0] = 0xFFFF;
        _spu_RXX2->rxx.key_off[1] = 0xFF;
#endif
        WASTE_TIME();
        WASTE_TIME();
        WASTE_TIME();
        WASTE_TIME();
    }
    _spu_inTransfer = 1;
    _spu_RXX->rxx.spucnt = 0xC000;
    _spu_transferCallback = NULL;
    _spu_IRQCallback = NULL;
    return 0;
}

/* _spu_writeByIO(addr, size): write SPU RAM through the data FIFO (manual transfer). */
s32 __attribute__((section(".text_80147070"))) sub_80147070(u8 *addr, u32 size)
{
    volatile s32 sp0;
    volatile s32 sp4;
    u16 spustat;
    s32 num_to_trans;
    u16 *cur_pos;
    s32 spustat_cur;
    s32 i;
    u16 cnt;

    cur_pos = (u16 *)addr;
    spustat = _spu_RXX->rxx.spustat & 0x7FF;
    _spu_RXX->rxx.trans_addr = _spu_tsa;
    WASTE_TIME();
    while (size != 0) {
        num_to_trans = (size > 0x40) ? 0x40 : size;
        for (i = 0; i < num_to_trans; i += 2) {
            _spu_RXX->rxx.trans_fifo = *cur_pos++;
        }
        cnt = _spu_RXX->rxx.spucnt;
        cnt &= ~0x30;
        cnt |= 0x10;
        _spu_RXX->rxx.spucnt = cnt;
        WASTE_TIME();
        D_8017E0D4 = 0;
        while (_spu_RXX->rxx.spustat & 0x400) {
            D_8017E0D4++;
            if (D_8017E0D4 > 5000) {
                printf(D_800D0D98, D_800D0DB8);
                break;
            }
        }
        WASTE_TIME();
        WASTE_TIME();
        size -= num_to_trans;
    }
    cnt = _spu_RXX->rxx.spucnt;
    cnt &= ~0x30;
    _spu_RXX->rxx.spucnt = cnt;
    D_8017E0D4 = 0;
    spustat_cur = _spu_RXX->rxx.spustat & 0x7FF;
    while (spustat_cur != spustat) {
        D_8017E0D4++;
        if (D_8017E0D4 > 5000) {
            printf(D_800D0D98, D_800D0DCC);
            break;
        }
        spustat_cur = _spu_RXX->rxx.spustat & 0x7FF;
    }
}

/* _spu_FiDMA(): SPU DMA completion handler; stops the transfer and signals. */
void __attribute__((section(".text_801473B4"))) sub_801473B4(void)
{
    volatile s32 sp0;
    volatile s32 sp4;
    s32 i;

    if (D_8017E128 == 0) {
        WASTE_TIME();
        WASTE_TIME();
        WASTE_TIME();
    }

    _spu_RXX->rxx.spucnt &= 0xFFCF;

    i = 0;
    while (_spu_RXX->rxx.spucnt & 0x30) {
        if (++i > 0xF00) {
            break;
        }
    }

    if (_spu_transferCallback) {
        _spu_transferCallback();
        return;
    }
    DeliverEvent(0xF0000009, 0x20);
}

/* _spu_r_(addr, spuaddr, blocks): start an SPU RAM -> main RAM DMA read. */
void __attribute__((section(".text_80147560"))) sub_80147560(s32 addr, u16 spuaddr, s32 blocks)
{
    volatile s32 sp0;
    volatile s32 sp4;

    _spu_RXX->rxx.trans_addr = spuaddr;

    WASTE_TIME();
    WASTE_TIME();

    _spu_RXX->rxx.spucnt |= 0x30;

    WASTE_TIME();
    WASTE_TIME();

    DELAY_R();
    *D_8017E0E0 = addr;
    *D_8017E0E4 = (blocks << 0x10) | 0x10;
    D_8017E128 = 1;
    *D_8017E0E8 = 0x01000200;
}

/* _spu_t(mode, ...): DMA transfer state machine (2 = set addr, 1 = write, 0 = read, 3 = start). */
s32 __attribute__((section(".text_80147750"))) sub_80147750(s32 mode, ...)
{
    s32 var_a2;
    s32 i;
    va_list args;
    u32 count;
    u16 ck2;
    u16 cnt;

    va_start(args, mode);

    switch (mode) {
    case 2:
        count = va_arg(args, u32);
        _spu_tsa = count >> _spu_mem_mode_plus;
        _spu_RXX->rxx.trans_addr = _spu_tsa;
        break;

    case 1:
        D_8017E128 = 0;
        i = 0;
        while (_spu_RXX->rxx.trans_addr != _spu_tsa) {
            if (++i > 0xF00) {
                return -2;
            }
        }

        cnt = _spu_RXX->rxx.spucnt;
        cnt &= ~0x30;
        cnt |= 0x20;
        _spu_RXX->rxx.spucnt = cnt;
        break;

    case 0:
        D_8017E128 = 1;

        i = 0;
        while (_spu_RXX->rxx.trans_addr != _spu_tsa) {
            if (++i > 0xF00) {
                return -2;
            }
        }

        cnt = _spu_RXX->rxx.spucnt;
        cnt &= ~0x30;
        cnt |= 0x30;
        _spu_RXX->rxx.spucnt = cnt;
        break;

    case 3:
        if (D_8017E128 == 1) {
            ck2 = 0x30;
        } else {
            ck2 = 0x20;
        }

        i = 0;
        while ((_spu_RXX->rxx.spucnt & 0x30) != ck2) {
            if (++i > 0xF00) {
                return -2;
            }
        }

        if (D_8017E128 == 1) {
            DELAY_R();
        } else {
            DELAY_W();
        }

        count = va_arg(args, u32);
        D_8017E12C = count;
        count = va_arg(args, u32);
        D_8017E130 = (count / 64);
        D_8017E130 += ((count % 64) ? 1 : 0);
        *D_8017E0E0 = D_8017E12C;
        *D_8017E0E4 = (D_8017E130 << 0x10) | 0x10;
        if (D_8017E128 == 1) {
            var_a2 = 0x01000200;
        } else {
            var_a2 = 0x01000201;
        }
        *D_8017E0E8 = var_a2;
        break;
    }
    return 0;
}

/* _spu_write(addr, size): write main RAM to SPU RAM at _spu_tsa (DMA or I/O). */
s32 __attribute__((section(".text_801479F8"))) sub_801479F8(u8 *addr, u32 size)
{
    if (_spu_transMode != 0) {
        sub_80147070(addr, size);
        return size;
    }
    sub_80147750(2, _spu_tsa << _spu_mem_mode_plus);
    sub_80147750(1);
    sub_80147750(3, addr, size);
    return size;
}

/* _spu_read(addr, size): DMA SPU RAM at _spu_tsa into main RAM. */
s32 __attribute__((section(".text_80147A80"))) sub_80147A80(s32 addr, u32 size)
{
    sub_80147750(2, _spu_tsa << _spu_mem_mode_plus);
    sub_80147750(0);
    sub_80147750(3, addr, size);
    return size;
}

/* _spu_FsetRXX(reg, value, shift): write an SPU register (optionally as an SPU address). */
void __attribute__((section(".text_80147AE8"))) sub_80147AE8(s32 reg, u32 value, s32 shift)
{
    if (shift == 0) {
        _spu_RXX->raw[reg] = value;
        return;
    }
    _spu_RXX->raw[reg] = value >> _spu_mem_mode_plus;
}

/* _spu_FsetRXXa(reg, addr): align an SPU address and write it (-1: return reg value, -2: return addr). */
u32 __attribute__((section(".text_80147B30"))) sub_80147B30(s32 reg, u32 addr)
{
    u32 temp_a3;
    u32 var_a1;

    var_a1 = addr;
    if ((_spu_mem_mode != 0) && ((var_a1 % _spu_mem_mode_unit) != 0)) {
        var_a1 += _spu_mem_mode_unit;
        var_a1 &= ~_spu_mem_mode_unitM;
    }
    temp_a3 = var_a1 >> _spu_mem_mode_plus;

    switch (reg) {
    case -1:
        return temp_a3 & 0xFFFF;
    case -2:
        return var_a1;
    default:
        _spu_RXX->raw[reg] = temp_a3;
        return var_a1;
    }
}

/* _SpuInit(mode): initialise libspu state and the SPU. */
void __attribute__((section(".text_80147C20"))) sub_80147C20(s32 mode)
{
    sub_801452F0();
    sub_80146B28(mode);
    sub_80147CC4();
    _spu_rev_flag = 0;
    _spu_rev_reserve_wa = 0;
    _spu_rev_attr.mode = 0;
    _spu_rev_attr.depth.left = 0;
    _spu_rev_attr.depth.right = 0;
    _spu_rev_attr.delay = 0;
    _spu_rev_attr.feedback = 0;
    _spu_rev_offsetaddr = _spu_rev_startaddr[0];
    sub_80147AE8(0xD1, _spu_rev_startaddr[0], 0);
    _spu_trans_mode = 0;
    _spu_transMode = 0;
    _spu_keystat = 0;
#if TM2_SPU_VOICES > 24
    portSpuExt.keystat = 0;
#endif
}

/* SpuStart(): one-time hookup of the SPU DMA callback and completion event. */
void __attribute__((section(".text_80147CC4"))) sub_80147CC4(void)
{
    if (_spu_isCalled == 0) {
        _spu_isCalled = 1;
        EnterCriticalSection();
        D_8017E134 = 0;
        sub_80147D68(sub_801473B4);
        _spu_EVdma = OpenEvent(0xF0000009, 0x20, 0x2000, NULL);
        EnableEvent(_spu_EVdma);
        ExitCriticalSection();
    }
}

/* _SpuDataCallback(func): install the SPU (DMA channel 4) completion callback. */
void __attribute__((section(".text_80147D68"))) sub_80147D68(void (*func)())
{
    sub_80145350(4, func);
}

/* _spu_gcSPU(): garbage-collect the SPU allocation list (merge, sort, trim free blocks). */
void __attribute__((section(".text_80147D8C"))) sub_80147D8C(void)
{
    s32 i, j;

    for (i = 0; i <= _spu_AllocLastNum;) {
        if (_spu_memList[i].addr & 0x80000000) {
            for (j = i + 1; 1; j++) {
                if (_spu_memList[j].addr != 0x2FFFFFFF) {
                    break;
                }
            }
            if ((_spu_memList[j].addr & 0x80000000) &&
                ((_spu_memList[j].addr & 0x0FFFFFFF) ==
                 ((_spu_memList[i].addr & 0x0FFFFFFF) + _spu_memList[i].size))) {
                _spu_memList[j].addr = 0x2FFFFFFF;
                _spu_memList[i].size += _spu_memList[j].size;
                continue;
            }
        }
        i++;
    }
    for (i = 0; i <= _spu_AllocLastNum; i++) {
        if (_spu_memList[i].size == 0) {
            _spu_memList[i].addr = 0x2FFFFFFF;
        }
    }
    for (i = 0; i <= _spu_AllocLastNum; i++) {
        if (_spu_memList[i].addr & 0x40000000) {
            break;
        }
        for (j = i + 1; j <= _spu_AllocLastNum; j++) {
            if (_spu_memList[j].addr & 0x40000000) {
                break;
            }
            if ((_spu_memList[j].addr & 0x0FFFFFFF) < (_spu_memList[i].addr & 0x0FFFFFFF)) {
                u32 swapAddr = _spu_memList[i].addr;
                u32 swapSize = _spu_memList[i].size;
                _spu_memList[i].addr = _spu_memList[j].addr;
                _spu_memList[i].size = _spu_memList[j].size;
                _spu_memList[j].addr = swapAddr;
                _spu_memList[j].size = swapSize;
            }
        }
    }
    for (i = 0; i <= _spu_AllocLastNum; i++) {
        if (_spu_memList[i].addr & 0x40000000) {
            break;
        }
        if (_spu_memList[i].addr == 0x2FFFFFFF) {
            _spu_memList[i].addr = _spu_memList[_spu_AllocLastNum].addr;
            _spu_memList[i].size = _spu_memList[_spu_AllocLastNum].size;
            _spu_AllocLastNum = i;
            break;
        }
    }
    for (i = _spu_AllocLastNum - 1; i >= 0; i--) {
        if (!(_spu_memList[i].addr & 0x80000000)) {
            break;
        }
        _spu_memList[i].addr &= 0x0FFFFFFF;
        _spu_memList[i].addr |= 0x40000000;
        _spu_memList[i].size += _spu_memList[_spu_AllocLastNum].size;
        _spu_AllocLastNum = i;
    }
}

/* SpuFree(addr): mark the block starting at addr free, then collect. */
void __attribute__((section(".text_8014808C"))) sub_8014808C(unsigned long addr)
{
    s32 i;

    for (i = 0; i < _spu_AllocBlockNum; i++) {
        if (_spu_memList[i].addr & 0x40000000) {
            break;
        }
        if (_spu_memList[i].addr == addr) {
            _spu_memList[i].addr |= 0x80000000;
            break;
        }
    }
    sub_80147D8C();
}

/* SpuWrite(addr, size): transfer main RAM to SPU RAM (max 0x7F000 bytes). */
unsigned long __attribute__((section(".text_80148108"))) sub_80148108(unsigned char *addr, unsigned long size)
{
    if (size > 0x7F000) {
        size = 0x7F000;
    }
    sub_801479F8(addr, size);
    if (_spu_transferCallback == NULL) {
        _spu_inTransfer = 0;
    }
    return size;
}

/* SpuSetTransferStartAddr(addr): set the SPU RAM transfer address. */
s32 __attribute__((section(".text_80148168"))) sub_80148168(s32 addr)
{
    _spu_tsa = sub_80147B30(-1, addr);
    return addr;
}

/* SpuSetTransferMode(mode): 0 = DMA, 1 = I/O. */
long __attribute__((section(".text_801481A4"))) sub_801481A4(long mode)
{
    s32 transMode;

    switch (mode) {
    case 0:
        transMode = 0;
        break;
    case 1:
        transMode = 1;
        break;
    default:
        transMode = 0;
    }
    _spu_trans_mode = mode;
    _spu_transMode = transMode;
    return transMode;
}

/* SpuIsTransferCompleted(flag): poll (0) or wait (1) for the SPU DMA to finish. */
s32 __attribute__((section(".text_801481D8"))) sub_801481D8(s32 flag)
{
    s32 in_transfer;

    if ((_spu_trans_mode == 1) || (_spu_inTransfer == 1)) {
        return 1;
    }
    in_transfer = TestEvent(_spu_EVdma);
    if (flag == 1) {
        if (in_transfer == 0) {
            do {
            } while (TestEvent(_spu_EVdma) == 0);
        }
        in_transfer = 1;
        _spu_inTransfer = in_transfer;
        return in_transfer;
    }
    if (in_transfer == 1) {
        _spu_inTransfer = in_transfer;
    }
    return in_transfer;
}

/* _spu_setInTransfer(flag): 1 = transfer in progress. */
void __attribute__((section(".text_80148290"))) sub_80148290(s32 flag)
{
    if (flag == 1) {
        _spu_inTransfer = 0;
    } else {
        _spu_inTransfer = 1;
    }
}

typedef struct {
    SpuVolume volume;
    s32 reverb;
    s32 mix;
} SpuExtAttr;

typedef struct {
    u32 mask;           /* 00 */
    SpuVolume mvol;     /* 04 */
    SpuVolume mvolmode; /* 08 */
    SpuVolume mvolx;    /* 0C */
    SpuExtAttr cd;      /* 10 */
    SpuExtAttr ext;     /* 1C */
} SpuCommonAttr;

/* SpuSetCommonAttr(attr): master / CD / external volumes and mixing. */
void __attribute__((section(".text_801482CC"))) sub_801482CC(SpuCommonAttr *attr)
{
    u16 mvol_mode_left;
    u16 mvol_mode_right;
    u16 vol_total_left;
    u16 vol_total_right;
    u32 mask;
    s32 bSetAll;
    u16 cnt;

    mask = attr->mask;
    bSetAll = attr->mask == 0;

    if (bSetAll || mask & 1) {
        if (bSetAll || mask & 4) {
            switch ((s16)attr->mvolmode.left) {
            case 1: mvol_mode_left = 0x8000; break;
            case 2: mvol_mode_left = 0x9000; break;
            case 3: mvol_mode_left = 0xA000; break;
            case 4: mvol_mode_left = 0xB000; break;
            case 5: mvol_mode_left = 0xC000; break;
            case 6: mvol_mode_left = 0xD000; break;
            case 7: mvol_mode_left = 0xE000; break;
            case 0:
                vol_total_left = attr->mvol.left;
                mvol_mode_left = 0;
                break;
            default:
                vol_total_left = attr->mvol.left;
                mvol_mode_left = 0;
                break;
            }
        } else {
            vol_total_left = attr->mvol.left;
            mvol_mode_left = 0;
        }

        if (mvol_mode_left != 0) {
            if ((s16)attr->mvol.left >= 0x80) {
                vol_total_left = 0x7F;
            } else if ((s16)attr->mvol.left < 0) {
                vol_total_left = 0;
            } else {
                vol_total_left = attr->mvol.left;
            }
        }
        vol_total_left &= 0x7FFF;
        _spu_RXX->rxx.main_vol.left = vol_total_left | mvol_mode_left;
    }

    if (bSetAll || mask & 2) {
        if (bSetAll || mask & 8) {
            switch ((s16)attr->mvolmode.right) {
            case 1: mvol_mode_right = 0x8000; break;
            case 2: mvol_mode_right = 0x9000; break;
            case 3: mvol_mode_right = 0xA000; break;
            case 4: mvol_mode_right = 0xB000; break;
            case 5: mvol_mode_right = 0xC000; break;
            case 6: mvol_mode_right = 0xD000; break;
            case 7: mvol_mode_right = 0xE000; break;
            case 0:
                vol_total_right = attr->mvol.right;
                mvol_mode_right = 0;
                break;
            default:
                vol_total_right = attr->mvol.right;
                mvol_mode_right = 0;
                break;
            }
        } else {
            vol_total_right = attr->mvol.right;
            mvol_mode_right = 0;
        }

        if (mvol_mode_right != 0) {
            if ((s16)attr->mvol.right >= 0x80) {
                vol_total_right = 0x7F;
            } else if ((s16)attr->mvol.right < 0) {
                vol_total_right = 0;
            } else {
                vol_total_right = attr->mvol.right;
            }
        }
        vol_total_right &= 0x7FFF;
        _spu_RXX->rxx.main_vol.right = vol_total_right | mvol_mode_right;
    }

    if (bSetAll || mask & 0x40) {
        _spu_RXX->rxx.cd_vol.left = attr->cd.volume.left;
    }
    if (bSetAll || mask & 0x80) {
        _spu_RXX->rxx.cd_vol.right = attr->cd.volume.right;
    }
    if (bSetAll || mask & 0x100) {
        if (attr->cd.reverb == 0) {
            cnt = _spu_RXX->rxx.spucnt;
            cnt &= ~4;
            _spu_RXX->rxx.spucnt = cnt;
        } else {
            cnt = _spu_RXX->rxx.spucnt;
            cnt |= 4;
            _spu_RXX->rxx.spucnt = cnt;
        }
    }
    if (bSetAll || mask & 0x200) {
        if (attr->cd.mix == 0) {
            cnt = _spu_RXX->rxx.spucnt;
            cnt &= ~1;
            _spu_RXX->rxx.spucnt = cnt;
        } else {
            cnt = _spu_RXX->rxx.spucnt;
            cnt |= 1;
            _spu_RXX->rxx.spucnt = cnt;
        }
    }
    if (bSetAll || mask & 0x400) {
        _spu_RXX->rxx.ex_vol.left = attr->ext.volume.left;
    }
    if (bSetAll || mask & 0x800) {
        _spu_RXX->rxx.ex_vol.right = attr->ext.volume.right;
    }
    if (bSetAll || mask & 0x1000) {
        if (attr->ext.reverb == 0) {
            cnt = _spu_RXX->rxx.spucnt;
            cnt &= ~8;
            _spu_RXX->rxx.spucnt = cnt;
        } else {
            cnt = _spu_RXX->rxx.spucnt;
            cnt |= 8;
            _spu_RXX->rxx.spucnt = cnt;
        }
    }
    if (bSetAll || mask & 0x2000) {
        if (attr->ext.mix == 0) {
            cnt = _spu_RXX->rxx.spucnt;
            cnt &= ~2;
            _spu_RXX->rxx.spucnt = cnt;
        } else {
            cnt = _spu_RXX->rxx.spucnt;
            cnt |= 2;
            _spu_RXX->rxx.spucnt = cnt;
        }
    }
}

/* SpuInit(): cold-initialise libspu. */
void __attribute__((section(".text_8015D408"))) sub_8015D408(void)
{
    sub_80147C20(0);
}

/* SpuQuit(): release the SPU DMA callback and event. */
void __attribute__((section(".text_8015D428"))) sub_8015D428(void)
{
    if (_spu_isCalled == 1) {
        _spu_isCalled = 0;
        EnterCriticalSection();
        _spu_transferCallback = NULL;
        _spu_IRQCallback = NULL;
        sub_80147D68(0);
        CloseEvent(_spu_EVdma);
        DisableEvent(_spu_EVdma);
        ExitCriticalSection();
    }
}

/* SpuInitHot(): warm-initialise libspu (voices left alone). */
void __attribute__((section(".text_8015D4A4"))) sub_8015D4A4(void)
{
    sub_80147C20(1);
}

/* SpuInitMalloc(num, top): set up the SPU RAM allocation table. */
s32 __attribute__((section(".text_8015D4C4"))) sub_8015D4C4(s32 num, SPU_MALLOC *top)
{
    if (num > 0) {
        _spu_memList = top;
        _spu_memList[0].addr = 0x40001010;
        _spu_memList[0].size = (0x10000 << _spu_mem_mode_plus) - 0x1010;
        _spu_AllocLastNum = 0;
        _spu_AllocBlockNum = num;
        return num;
    }
    return 0;
}

/* SpuSetNoiseVoice(on_off, voice_bit): switch voices to the noise generator. */
s32 __attribute__((section(".text_8015D518"))) sub_8015D518(s32 on_off, u32 voice_bit)
{
#if TM2_SPU_VOICES > 24
    if (voice_bit & TM2_SPU_EXT_MASK) {
        return 0;  /* voices 24..47 have no noise (docs/SPU_VOICES.md 2): nothing to switch, none on */
    }
#endif
    return sub_8015D53C(on_off, voice_bit, 0xCA, 0xCB);
}

/* _SpuSetAnyVoice(on_off, bits, reg_lo, reg_hi): set/clear voice bits in a 24-bit register pair. */
s32 __attribute__((section(".text_8015D53C"))) sub_8015D53C(s32 on_off, u32 bits, s32 addr1, s32 addr2)
{
    s32 var_t0;
    u16 var_v1;
    u16 *temp_a2;
    u16 *temp_a3;

    temp_a3 = (u16 *)&_spu_RXX->raw[addr2];
    temp_a2 = (u16 *)&_spu_RXX->raw[addr1];
    var_t0 = *temp_a2 | ((_spu_RXX->raw[addr2] & 0xff) << 0x10);
    if (on_off != 0) {
        if (on_off == 1) {
            var_t0 |= bits & 0xFFFFFF;
            *temp_a2 |= bits;
            var_v1 = *temp_a3 | ((bits >> 0x10) & 0xFF);
            *temp_a3 = var_v1;
        }
    } else {
        var_t0 &= ~(bits & 0xFFFFFF);
        *temp_a2 &= ~bits;
        var_v1 = *temp_a3 & ~((bits >> 0x10) & 0xFF);
        *temp_a3 = var_v1;
    }
    return var_t0 & 0xFFFFFF;
}

/* SpuSetReverb(on_off): enable reverb unless its work area overlaps allocated SPU RAM. */
s32 __attribute__((section(".text_8015D640"))) sub_8015D640(s32 on_off)
{
    u16 cnt;

    switch (on_off) {
    case 0:
        _spu_rev_flag = 0;
        cnt = _spu_RXX->rxx.spucnt;
        cnt &= ~0x80;
        _spu_RXX->rxx.spucnt = cnt;
        break;

    case 1:
        if ((_spu_rev_reserve_wa != on_off) && sub_8015D780(_spu_rev_offsetaddr)) {
            _spu_rev_flag = 0;
            cnt = _spu_RXX->rxx.spucnt;
            cnt &= ~0x80;
            _spu_RXX->rxx.spucnt = cnt;
        } else {
            _spu_rev_flag = on_off;
            cnt = _spu_RXX->rxx.spucnt;
            cnt |= 0x80;
            _spu_RXX->rxx.spucnt = cnt;
        }
        break;
    }
    return _spu_rev_flag;
}

/* _SpuIsInAllocateArea_(addr): 1 if SPU address (in 8-byte units) lies in allocated RAM. */
s32 __attribute__((section(".text_8015D780"))) sub_8015D780(u32 addr)
{
    s32 i;

    addr <<= _spu_mem_mode_plus;
    for (i = 0; 1; i++) {
        if (_spu_memList[i].addr & 0x80000000) {
            continue;
        }
        if (_spu_memList[i].addr & 0x40000000) {
            break;
        }
        if (addr <= (_spu_memList[i].addr & 0x0FFFFFFF)) {
            return 1;
        }
        if (addr < (_spu_memList[i].addr & 0x0FFFFFFF) + _spu_memList[i].size) {
            return 1;
        }
    }
    return 0;
}

typedef struct {
    u32 voice;          /* 00 */
    u32 mask;           /* 04 */
    SpuVolume volume;   /* 08 */
    SpuVolume volmode;  /* 0C */
    SpuVolume volumex;  /* 10 */
    u16 pitch;          /* 14 */
    u16 note;           /* 16 */
    u16 sample_note;    /* 18 */
    s16 envx;           /* 1A */
    u32 addr;           /* 1C */
    u32 loop_addr;      /* 20 */
    s32 a_mode;         /* 24 */
    s32 s_mode;         /* 28 */
    s32 r_mode;         /* 2C */
    u16 ar;             /* 30 */
    u16 dr;             /* 32 */
    u16 sr;             /* 34 */
    u16 rr;             /* 36 */
    u16 sl;             /* 38 */
    u16 adsr1;          /* 3A */
    u16 adsr2;          /* 3C */
} SpuVoiceAttr;

s32 sub_80161C70(SpuVoiceAttr *attr, s32 min, s32 max, s32 arg3);
s32 sub_8016236C(s32 center_note, u16 center_note_frac, s32 offset_note, u16 offset_note_frac);

/* SpuSetVoiceAttr(attr): set attributes of all voices selected in attr->voice. */
void __attribute__((section(".text_80161C48"))) sub_80161C48(SpuVoiceAttr *attr)
{
    sub_80161C70(attr, 0, NUM_SPU_CHANNELS - 1, 0);
}

/* SpuGetKeyStatus(voice_bit): key state of the lowest voice in voice_bit
 * (1 = on, 3 = on but envelope 0, 2 = off with sound, 0 = off; -1 = none). */
long __attribute__((section(".text_801622D8"))) sub_801622D8(unsigned long voice_bit)
{
    s32 var_a1;
    s32 var_v1;
    s32 bit;
    u16 temp_a0;

#if TM2_SPU_VOICES > 24
    if (voice_bit & TM2_SPU_EXT_MASK) {  /* voices 24..47: the second block and its key-state word */
        for (var_v1 = 0; var_v1 < TM2_SPU_VOICES - 24; var_v1++) {
            if (voice_bit & (1 << var_v1)) {
                temp_a0 = _spu_RXX2->raw[var_v1 * 8 + 6];
                if (portSpuExt.keystat & (1 << var_v1)) {
                    return temp_a0 != 0 ? 1 : 3;
                }
                return (temp_a0 != 0) * 2;
            }
        }
        return -1;
    }
#endif
    var_a1 = -1;
    for (var_v1 = 0; var_v1 < 24; var_v1++) {
        if (voice_bit & (1 << var_v1)) {
            var_a1 = var_v1;
            break;
        }
    }

    if (var_a1 == -1) {
        return -1;
    }

    temp_a0 = _spu_RXX->raw[var_a1 * 8 + 6];
    bit = 1 << var_a1;
    if (_spu_keystat & bit) {
        if (temp_a0 != 0) {
            return 1;
        } else {
            return 3;
        }
    } else {
        return (temp_a0 != 0) * 2;
    }
}

/* _spu_note2pitch(center, center_frac, note, note_frac): note -> SPU pitch value. */
s32 __attribute__((section(".text_8016236C"))) sub_8016236C(s32 center_note, u16 center_note_frac, s32 offset_note, u16 offset_note_frac)
{
    s16 octave;
    s16 var_a1;
    s16 var_v1_2;
    u16 sample_rate;
    s32 pos;

    var_a1 = (offset_note_frac + center_note_frac) / 8;
    for (var_v1_2 = 0; var_a1 >= 16; var_v1_2++) {
        var_a1 -= 16;
    }
    if (var_a1 < 0) {
        var_a1 = 0;
    }
    var_v1_2 += offset_note - (center_note - 60);
    pos = ((var_v1_2 % 12) * 16) + var_a1;
    sample_rate = D_80180850[pos];
    octave = (var_v1_2 / 12) - 5;
    if (octave > 0) {
        sample_rate <<= octave;
    } else if (octave < 0) {
        sample_rate >>= -octave;
    }
    return sample_rate;
}
