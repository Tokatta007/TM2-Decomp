/* Library code 8015C000..80160000 not yet in C: the PsyQ libspu leftovers
 * and one unoptimised empty function. (The SingleTrac engine functions of
 * this range and the two libsnd ones need other recipes and live in their
 * own units.)
 *
 * SpuSetNoiseClock (8015D5F0) and SpuGetCommonAttr (8015D7F8): adapted from
 * psy-q-decomp src/spu/s_snc.c and src/spu/s_gca.c (MIT, see THIRD_PARTY.md);
 * SpuGetCommonAttr follows retail (modes only zeroed when the volume is not
 * in sweep mode, the < 0x4000 test first).
 * SPDX-License-Identifier: MIT
 *
 * Recipe: psyq3.3 cc1 (GCC 2.6.0) + aspsx 2.21, gp_size 0, no -fvolatile.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* 8015DAF8 is in sdk_unknown_o0.c (compiled without optimisation). */

/* SpuSetNoiseClock / SpuGetCommonAttr (libspu, psyq3.3) */
typedef struct {
    s16 left;
    s16 right;
} SpuVolume;

typedef struct {
    SpuVolume volume;
    s32 reverb;
    s32 mix;
} SpuExtAttr;

typedef struct {
    u32 mask;
    SpuVolume mvol;
    SpuVolume mvolmode;
    SpuVolume mvolx;
    SpuExtAttr cd;
    SpuExtAttr ext;
} SpuCommonAttr;

typedef struct tagSpuControl {
    u8 voice[0x180];
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
} SPU_RXX;

union SpuUnion {
    volatile SPU_RXX rxx;
    volatile u16 raw[0x100];
};
extern union SpuUnion *D_8017E0DC;
#define _spu_RXX D_8017E0DC
#pragma tm2_absolute D_8017E0DC

/* SpuSetNoiseClock */
long __attribute__((section(".text_8015D5F0"))) sub_8015D5F0(long n_clock)
{
    long clamped;
    u16 controlRegister;

    if (n_clock < 0) {
        clamped = 0;
    } else if (n_clock > 0x3F) {
        clamped = 0x3F;
    } else {
        clamped = n_clock;
    }

    controlRegister = _spu_RXX->rxx.spucnt;
    controlRegister &= ~0x3F00;
    controlRegister |= ((clamped & 0x3F) << 8);
    _spu_RXX->rxx.spucnt = controlRegister;

    return clamped;
}

/* SpuGetCommonAttr */
void __attribute__((section(".text_8015D7F8"))) sub_8015D7F8(SpuCommonAttr *attr)
{
    u16 var_a1;
    u16 var_a2;
    u16 var_a3;
    u16 var_t0;

    var_a3 = _spu_RXX->rxx.main_vol.left;
    var_t0 = _spu_RXX->rxx.main_vol.right;


    if ((var_a3 & 0x8000) == 0) {
        var_a2 = 0;
    } else {
        switch (var_a3 & 0xF000) {
        case 0x8000: var_a2 = 1; break;
        case 0x9000: var_a2 = 2; break;
        case 0xA000: var_a2 = 3; break;
        case 0xB000: var_a2 = 4; break;
        case 0xC000: var_a2 = 5; break;
        case 0xD000: var_a2 = 6; break;
        case 0xE000:
        case 0xF000: var_a2 = 7; break;
        }
        var_a3 &= 0xFFF;
    }

    if ((var_t0 & 0x8000) == 0) {
        var_a1 = 0;
    } else {
        switch (var_t0 & 0xF000) {
        case 0x8000: var_a1 = 1; break;
        case 0x9000: var_a1 = 2; break;
        case 0xA000: var_a1 = 3; break;
        case 0xB000: var_a1 = 4; break;
        case 0xC000: var_a1 = 5; break;
        case 0xD000: var_a1 = 6; break;
        case 0xE000:
        case 0xF000: var_a1 = 7; break;
        }
        var_t0 &= 0xFFF;
    }

    {
        u32 x = 0x8000;
        u32 y = var_a3;
        if (y < 0x4000) {
            attr->mvol.left = var_a3;
        } else {
            attr->mvol.left = y - x;
        }
    }
    {
        u32 x = 0x8000;
        u32 y = var_t0;
        if (y < 0x4000) {
            attr->mvol.right = var_t0;
        } else {
            attr->mvol.right = y - x;
        }
    }

    attr->mvolmode.left = var_a2;
    attr->mvolmode.right = var_a1;
    attr->mvolx.left = _spu_RXX->rxx.main_volx.left;
    attr->mvolx.right = _spu_RXX->rxx.main_volx.right;
    attr->cd.volume.left = _spu_RXX->rxx.cd_vol.left;
    attr->cd.volume.right = _spu_RXX->rxx.cd_vol.right;
    if (_spu_RXX->rxx.spucnt & 4) attr->cd.reverb = 1; else attr->cd.reverb = 0;
    if (_spu_RXX->rxx.spucnt & 1) attr->cd.mix = 1; else attr->cd.mix = 0;
    attr->ext.volume.left = _spu_RXX->rxx.ex_vol.left;
    attr->ext.volume.right = _spu_RXX->rxx.ex_vol.right;
    if (_spu_RXX->rxx.spucnt & 8) attr->ext.reverb = 1; else attr->ext.reverb = 0;
    if (_spu_RXX->rxx.spucnt & 2) attr->ext.mix = 1; else attr->ext.mix = 0;
}
