/* PsyQ SDK LIBC LIBGS, reconstructed C (see docs/SDK_SOURCES.md, THIRD_PARTY.md).
 *
 * memmove: file-scope assembly reproducing the retail instruction sequence
 * (no public source used; this libc build was not compiled by cc1: t6-t9
 * temporaries, explicit $at, `b` branches).
 * GsGetTimInfo: written by hand from the retail disassembly.
 *
 * Not here: sprintf @801388B8 (assembly in lib_r138.c). */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef signed int s32;

typedef struct {
    u32 pmode;          /* +0x00 TIM flags word */
    s16 px, py;         /* +0x04 pixel data VRAM position */
    u16 pw, ph;         /* +0x08 pixel data size (16-bit units) */
    u32 *pixel;         /* +0x0C pixel data */
    s16 cx, cy;         /* +0x10 CLUT VRAM position */
    u16 cw, ch;         /* +0x14 CLUT size */
    u32 *clut;          /* +0x18 CLUT data */
} GsIMAGE;

/* @80139418 memmove: overlap-safe byte copy. Retail is not GCC output (it uses
 * $t6/$t7 and an `sltu $at` branch macro), so it is file-scope assembly
 * reproducing the retail instruction sequence. Equivalent C:
 *   if (dest >= src) while (count-- > 0) dest[count] = src[count];
 *   else while (count-- > 0) *dest++ = *src++;
 *   return dest;   (dest advanced when copying forwards) */
void *sub_80139418(u8 *dest, u8 *src, s32 count);
__asm__(
    ".section .text_80139418,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80139418\n"
    ".type sub_80139418,@function\n"
    "sub_80139418:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"   /* (first thing in the file: maspsx eats the TAB form) */
    ".set noat\n"
    "\tsltu $1,$4,$5\n"   /* 80139418: 0085082b */
    "\tbne $1,$0,.L80139460\n"   /* 8013941C: 14200010 */
    "\taddu $2,$6,$0\n"   /* 80139420: 00c01021 */
    "\taddu $2,$6,$0\n"   /* 80139424: 00c01021 */
    "\tblez $2,.L80139484\n"   /* 80139428: 18400016 */
    "\taddiu $6,$6,-1\n"   /* 8013942C: 24c6ffff */
    "\taddu $3,$4,$6\n"   /* 80139430: 00861821 */
    "\taddu $7,$5,$6\n"   /* 80139434: 00a63821 */
    ".L80139438:\n"
    "\tlbu $14,0($7)\n"   /* 80139438: 90ee0000 */
    "\taddu $2,$6,$0\n"   /* 8013943C: 00c01021 */
    "\taddiu $6,$6,-1\n"   /* 80139440: 24c6ffff */
    "\taddiu $3,$3,-1\n"   /* 80139444: 2463ffff */
    "\taddiu $7,$7,-1\n"   /* 80139448: 24e7ffff */
    "\tbgtz $2,.L80139438\n"   /* 8013944C: 1c40fffa */
    "\tsb $14,1($3)\n"   /* 80139450: a06e0001 */
    "\tbeq $0,$0,.L80139484\n"   /* 80139454: 1000000b */
    "\tsll $0,$0,0x0\n"   /* 80139458: 00000000 */
    "\taddu $2,$6,$0\n"   /* 8013945C: 00c01021 */
    ".L80139460:\n"
    "\tblez $2,.L80139484\n"   /* 80139460: 18400008 */
    "\taddiu $6,$6,-1\n"   /* 80139464: 24c6ffff */
    ".L80139468:\n"
    "\tlbu $15,0($5)\n"   /* 80139468: 90af0000 */
    "\taddu $2,$6,$0\n"   /* 8013946C: 00c01021 */
    "\taddiu $6,$6,-1\n"   /* 80139470: 24c6ffff */
    "\taddiu $5,$5,1\n"   /* 80139474: 24a50001 */
    "\taddiu $4,$4,1\n"   /* 80139478: 24840001 */
    "\tbgtz $2,.L80139468\n"   /* 8013947C: 1c40fffa */
    "\tsb $15,-1($4)\n"   /* 80139480: a08fffff */
    ".L80139484:\n"
    "\tjr $31\n"   /* 80139484: 03e00008 */
    "\taddu $2,$4,$0\n"   /* 80139488: 00801021 */
    ".set at\n"
    ".set\treorder\n"
    ".set noreorder\n"
    ".size sub_80139418,.-sub_80139418\n");

/* @8013E8B0 GsGetTimInfo: parse a TIM (pointer just past the 0x10 id word)
 * into a GsIMAGE: optional CLUT block (flag bit 3), then the pixel block. */
void __attribute__((section(".text_8013E8B0"))) sub_8013E8B0(u32 *im, GsIMAGE *tim)
{
    u32 *next;

    if ((tim->pmode = *im) >> 3 & 1) {
        im++;
        next = im + (*im >> 2);
        im++;
        tim->cx = ((u16 *)im)[0];
        tim->cy = ((u16 *)im)[1];
        im++;
        tim->cw = ((u16 *)im)[0];
        tim->ch = ((u16 *)im)[1];
        im++;
        tim->clut = im;
        next++;
        tim->px = ((u16 *)next)[0];
        tim->py = ((u16 *)next)[1];
        next++;
        tim->pw = ((u16 *)next)[0];
        tim->ph = ((u16 *)next)[1];
        next++;
        tim->pixel = next;
    } else {
        im += 2;
        tim->px = ((u16 *)im)[0];
        tim->py = ((u16 *)im)[1];
        im++;
        tim->pw = ((u16 *)im)[0];
        tim->ph = ((u16 *)im)[1];
        im++;
        tim->pixel = im;
    }
}
