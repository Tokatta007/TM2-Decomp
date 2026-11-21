/* PsyQ SDK LIBSPU _SpuSetVoiceAttr (own unit: its two switch jump tables sit at 0x800D12D8,
 * away from libspu's other tables). See docs/SDK_SOURCES.md, THIRD_PARTY.md.
 * Adapted from sotn-decomp src/main/psxsdk/libspu/s_sva.c.
 * SPDX-License-Identifier: MIT
 *
 * TM2's (PsyQ 3.4, Dec 1995) version does not pre-zero vol_left/vol_right.
 * Recipe psyq3.3 cc1 + aspsx 2.21, gp_size 0. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/* Request 34 (docs/SPU_VOICES.md): with --port-define TM2_PORT_SPU48 a voice mask with TM2_SPU_EXT_MASK set
 * selects voices 24..47 (bits 0..23), whose registers are the second block (TM2_SPU_EXT_REGS). */
#include "semantic/spu_voices.h"

#define NUM_SPU_CHANNELS 24

typedef struct {
    s16 left;
    s16 right;
} SpuVolume;

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

union SpuUnion {
    volatile u16 raw[0x100];
};

extern union SpuUnion *D_8017E0DC;     /* _spu_RXX = 0x1F801C00 */
extern u16 D_8017E15C[];               /* _spu_voice_centerNote */

#define _spu_RXX D_8017E0DC
#define _spu_voice_centerNote D_8017E15C

#if TM2_SPU_VOICES > 24
/* the selected block (rxx) and its first voice (ext: 0 or 24, for _spu_voice_centerNote, 48 entries) */
#define RXX rxx
#define CENTER_NOTE(v) _spu_voice_centerNote[(v) + ext]
#else
#define RXX _spu_RXX
#define CENTER_NOTE(v) _spu_voice_centerNote[v]
#endif

u32 sub_80147B30(s32 reg, u32 addr);   /* _spu_FsetRXXa */
s32 sub_8016236C(s32 center_note, u16 center_note_frac, s32 offset_note, u16 offset_note_frac); /* _spu_note2pitch */

/* The two volume-mode jump tables (0x800D12D8, 0x800D12F8). */
#pragma tm2_rdata .tm2_800D12D8

static __inline__ u16 inline_fn(u16 arg0, u16 arg1) { return arg0 & arg1; }

/* _SpuSetVoiceAttr(attr, min, max, nowait): set the attributes in attr->mask (0 = all)
 * for the voices min..max selected in attr->voice; returns 0, or -3 on a bad range. */
s32 __attribute__((section(".text_80161C70"))) sub_80161C70(SpuVoiceAttr *attr, s32 min, s32 max, s32 arg3)
{
    volatile s32 sp10;
    volatile s32 sp14;
    s32 bSetAll;
    s32 var_a1_2;
    s32 var_a1_3;
    s32 var_a1_4;
    u16 volmode_left;
    u16 volmode_right;
    s32 voice;
    u16 temp_a1;
    u16 temp_a3;
    u16 var_a2;
    u16 vol_left;
    u16 vol_right;
    u32 mask;
    s32 pos;
    s32 adsr;
#if TM2_SPU_VOICES > 24
    union SpuUnion *rxx;
    s32 ext;

    rxx = _spu_RXX;
    ext = 0;
    if (attr->voice & TM2_SPU_EXT_MASK) {
        rxx = (union SpuUnion *)TM2_SPU_EXT_REGS;
        ext = 24;
    }
#endif

    mask = attr->mask;
    bSetAll = attr->mask == 0;

    if (min < 0) {
        min = 0;
    }
    if (min >= NUM_SPU_CHANNELS) {
        return -3;
    }
    if (max >= NUM_SPU_CHANNELS) {
        max = NUM_SPU_CHANNELS - 1;
    }
    if (max < 0 || max < min) {
        return -3;
    }
    max++;
    for (voice = min; voice < max; voice++) {
        if (((1 << voice) & attr->voice) == 0) {
            continue;
        }
        pos = voice * 8;

        if (bSetAll || mask & 0x10) {
            RXX->raw[pos + 2] = attr->pitch;
        }
        if (bSetAll || mask & 0x40) {
            CENTER_NOTE(voice) = attr->sample_note;
        }
        if (bSetAll || mask & 0x20) {
            temp_a1 = CENTER_NOTE(voice);
            temp_a3 = attr->note;
            RXX->raw[pos + 2] = sub_8016236C(
                temp_a1 >> 8, temp_a1 & 0xFF, temp_a3 >> 8, temp_a3 & 0xFF);
        }
        if (bSetAll || mask & 0x80) {
            var_a1_3 = pos | 3;
#if TM2_SPU_VOICES > 24
            if (ext) {
                RXX->raw[var_a1_3] = sub_80147B30(-1, attr->addr);  /* _spu_FsetRXXa writes block 1 only */
            } else
#endif
            sub_80147B30(var_a1_3, attr->addr);
        }
        if (bSetAll || mask & 0x800) {
            if (bSetAll || mask & 0x100) {
                switch (attr->a_mode) {
                case 1:
                    var_a1_2 = 0;
                    break;
                case 5:
                    var_a1_2 = 0x80;
                    break;
                default:
                    var_a1_2 = 0;
                    break;
                }
            } else {
                var_a1_2 = 0;
            }
            if (attr->ar > 0x7F) {
                var_a2 = 0x7F;
            } else {
                var_a2 = attr->ar;
            }
            adsr = RXX->raw[pos + 4];
            adsr &= 0x00FF;
            RXX->raw[pos + 4] = adsr | ((var_a2 | var_a1_2) << 8);
        }
        if (bSetAll || mask & 0x1000) {
            if (attr->dr > 0xF) {
                var_a2 = 0xF;
            } else {
                var_a2 = attr->dr;
            }
            adsr = RXX->raw[pos + 4];
            adsr &= 0xFF0F;
            RXX->raw[pos + 4] = adsr | (var_a2 << 4);
        }
        if (bSetAll || mask & 0x8000) {
            if (attr->sl > 0xF) {
                var_a2 = 0xF;
            } else {
                var_a2 = attr->sl;
            }
            adsr = RXX->raw[pos + 4];
            RXX->raw[pos + 4] = (adsr & 0xFFF0) | var_a2;
        }
        if (bSetAll || mask & 0x2000) {
            if (bSetAll || mask & 0x200) {
                switch (attr->s_mode) {
                case 1:
                    var_a1_3 = 0;
                    break;
                case 3:
                    var_a1_3 = 0x100;
                    break;
                case 5:
                    var_a1_3 = 0x200;
                    break;
                case 7:
                    var_a1_3 = 0x300;
                    break;
                default:
                    var_a1_3 = 0x100;
                    break;
                }
            } else {
                var_a1_3 = 0;
            }
            if (attr->sr > 0x7F) {
                var_a2 = 0x7F;
            } else {
                var_a2 = attr->sr;
            }
            adsr = RXX->raw[pos + 5];
            adsr &= 0x003F;
            RXX->raw[pos + 5] = adsr | ((var_a2 | var_a1_3) << 6);
        }
        if (bSetAll || mask & 0x4000) {
            if (bSetAll || mask & 0x400) {
                switch (attr->r_mode) {
                case 3:
                    var_a1_4 = 0;
                    break;
                case 7:
                    var_a1_4 = 0x20;
                    break;
                default:
                    var_a1_4 = 0;
                    break;
                }
            } else {
                var_a1_4 = 0;
            }
            if (attr->rr > 0x1F) {
                var_a2 = 0x1F;
            } else {
                var_a2 = attr->rr;
            }
            adsr = RXX->raw[pos + 5];
            adsr &= 0xFFC0;
            RXX->raw[pos + 5] = adsr | (var_a2 | var_a1_4);
        }
        if (bSetAll || mask & 0x20000) {
            RXX->raw[pos + 4] = attr->adsr1;
        }
        if (bSetAll || mask & 0x40000) {
            RXX->raw[pos + 5] = attr->adsr2;
        }
        if (bSetAll || mask & 0x10000) {
#if TM2_SPU_VOICES > 24
            if (ext) {
                RXX->raw[pos | 7] = sub_80147B30(-1, attr->loop_addr);
            } else
#endif
            sub_80147B30(pos | 7, attr->loop_addr);
        }
        if (bSetAll || mask & 1) {
            if (bSetAll || mask & 4) {
                switch ((s16)attr->volmode.left) {
                case 1:
                    volmode_left = 0x8000;
                    break;
                case 2:
                    volmode_left = 0x9000;
                    break;
                case 3:
                    volmode_left = 0xA000;
                    break;
                case 4:
                    volmode_left = 0xB000;
                    break;
                case 5:
                    volmode_left = 0xC000;
                    break;
                case 6:
                    volmode_left = 0xD000;
                    break;
                case 7:
                    volmode_left = 0xE000;
                    break;
                case 0:
                    vol_left = attr->volume.left;
                    volmode_left = 0;
                    break;
                default:
                    vol_left = attr->volume.left;
                    volmode_left = 0;
                    break;
                }
            } else {
                vol_left = attr->volume.left;
                volmode_left = 0;
            }
            if (volmode_left != 0) {
                if ((s16)attr->volume.left >= 0x80) {
                    vol_left = 0x7F;
                } else if ((s16)attr->volume.left < 0) {
                    vol_left = 0;
                } else {
                    vol_left = attr->volume.left;
                }
            }
            RXX->raw[pos + 0] = inline_fn(vol_left, 0x7FFF) | volmode_left;
        }
        if (bSetAll || mask & 2) {
            if (bSetAll || mask & 8) {
                switch ((s16)attr->volmode.right) {
                case 1:
                    volmode_right = 0x8000;
                    break;
                case 2:
                    volmode_right = 0x9000;
                    break;
                case 3:
                    volmode_right = 0xA000;
                    break;
                case 4:
                    volmode_right = 0xB000;
                    break;
                case 5:
                    volmode_right = 0xC000;
                    break;
                case 6:
                    volmode_right = 0xD000;
                    break;
                case 7:
                    volmode_right = 0xE000;
                    break;
                case 0:
                    vol_right = attr->volume.right;
                    volmode_right = 0;
                    break;
                default:
                    vol_right = attr->volume.right;
                    volmode_right = 0;
                    break;
                }
            } else {
                vol_right = attr->volume.right;
                volmode_right = 0;
            }
            if (volmode_right != 0) {
                if ((s16)attr->volume.right > 0x7F) {
                    vol_right = 0x7F;
                } else if ((s16)attr->volume.right < 0) {
                    vol_right = 0;
                } else {
                    vol_right = attr->volume.right;
                }
            }
            RXX->raw[pos + 1] = inline_fn(vol_right, 0x7FFF) | volmode_right;
        }
    }
    if (arg3 == 0) {
        sp14 = 1;
        for (sp10 = 0; sp10 < 2; sp10++) {
            sp14 *= 0xD;
        }
    }
    return 0;
}
