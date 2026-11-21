/* LIBSND vs_vh.c: SsVabOpenHeadSticky / SsVabFakeHead.
 * Adapted from sotn-decomp src/main/psxsdk/libsnd/vs_vh.c (SPDX-License-Identifier: MIT).
 * Recipe: --compiler gcc2.5.7 (libsnd recipe), gp_size 0. Both byte-identical. */
typedef unsigned char u8;
typedef unsigned int u32;
typedef signed short s16;

extern s16 sub_801462AC(u8 *addr, s16 vabid, s16 mode, u32 sbaddr); /* SsVabOpenHeadWithMode */

/* SsVabOpenHeadSticky: open a VAB header at a fixed SPU address. */
s16 __attribute__((section(".text_8014624C"))) sub_8014624C(u8 *addr, s16 vabid, u32 sbaddr)
{
    return sub_801462AC(addr, vabid, 1, sbaddr);
}

/* SsVabFakeHead: same as SsVabOpenHeadSticky in this build. */
s16 __attribute__((section(".text_8014627C"))) sub_8014627C(u8 *addr, s16 vabid, u32 sbaddr)
{
    return sub_801462AC(addr, vabid, 1, sbaddr);
}
