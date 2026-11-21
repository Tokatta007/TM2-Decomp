/* Library code 80154000..8015C000: SingleTrac engine part (math helpers next to
 * mathMulTransVec, the near-polygon renderer pass and its subroutines, the
 * view-mode accessors, angle wrapping/blending, sound start/stop wrappers).
 * Sources/licences: docs/SDK_SOURCES.md, THIRD_PARTY.md. All code here was
 * written against the retail disassembly; no third-party source was used.
 *
 * Recipe: like the game (psyq3.6 cc1, -fvolatile, extern_gp, gp_size 8).
 * The libsnd functions of this range (GCC 2.5.7 recipe) are in a separate
 * unit. renderNearPolys/80154E30/80154E84/80155088 are SingleTrac hand-written
 * assembly (registers saved in the scratchpad, internal jal subroutines),
 * kept as the original instruction sequence in top-level asm blocks. */

typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

/* View mode table entry (8017EC44, 96 bytes; see view_system.c). */
typedef struct {
    s32 f0;
    s32 f4;
    s32 scale;              /* +0x08 */
    s32 fC;
    u8 pad_10[0x50];
} TM2ViewMode;

extern TM2ViewMode viewModeTable[];
extern s32 viewModes[];
extern s32 viewRenderSplitMode;

/* sound settings (gp block 80180DEC..80180E48) */
extern s32 D_80180DEC, soundFineMax, soundFineStepsPerNote, soundInstPitchScale, D_80180DFC, soundNoteMax;
extern s32 D_80180E04, D_80180E08, audioCdVolume, D_80180E10, audioMusicVolumeCap, audioSfxVolume;
extern s32 D_80180E1C, audioSfxVolumeCap, audioVolumeClampMax, audioVolumeClampMin, audioMasterVolume, soundNoiseClock;
extern s32 soundReservedVoices, D_80180E3C, D_80180E44, soundMVolResultPtr;
#include "semantic/spu_voices.h"

extern s32 sub_8013FC44(s32);              /* square root */
extern s32 timerGetFrameRate(void);             /* ticks of the last frame */
extern void sub_8015700C(void);            /* SsInit */
extern char sub_801583C8(char a);             /* SsSetReservedVoice */
extern s32 sub_8015D640(s32);             /* SpuSetReverb */
extern void sub_80157DD4(s32);             /* SsSetTickMode */
extern void sub_80157380(void);            /* SsStart */
extern void sub_80156EA8(s16, s16);        /* SsSetMVol */
extern void sub_80156E08();                /* SsGetMVol (SndVolume *) */
extern void sub_801583B8(void);            /* SsSetStereo */
extern int sub_8015D5F0(s32);
extern void sub_80157F48(void);            /* SsEnd */
extern void sub_8015704C(void);            /* SsQuit */

#include "semantic/world.h"

/* One output component: three halfword-matrix by word-vector products,
 * summed and scaled down by 4096 (as math_vec.c). */
#define MAT_ROW(m, a, b, c, input) (((input)[0] * (m)[a] + (input)[1] * (m)[b] + (input)[2] * (m)[c]) / 4096)

/* @8015424C: 3x3 halfword matrix times word vector (row-major). */
void __attribute__((section(".text_8015424C"))) mathMulMatVec2(short *m, int *input, int *output)
{
    output[0] = MAT_ROW(m, 0, 1, 2, input);
    output[1] = MAT_ROW(m, 3, 4, 5, input);
    output[2] = MAT_ROW(m, 6, 7, 8, input);
}

/* @80154380 mathMulTransVecShort: transposed matrix times halfword vector. */
void __attribute__((section(".text_80154380"))) mathMulTransVecShort(short *m, short *input, int *output)
{
    output[0] = MAT_ROW(m, 0, 3, 6, input);
    output[1] = MAT_ROW(m, 1, 4, 7, input);
    output[2] = MAT_ROW(m, 2, 5, 8, input);
}

/* @801544FC mathNormalizeVec: out = in scaled to length 4096 (zero vector
 * when the length is not positive). */
void __attribute__((section(".text_801544FC"))) mathNormalizeVec(s32 *in, s32 *out)
{
    s32 len;

    len = sub_8013FC44(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
    if (len > 0) {
        out[0] = (in[0] << 12) / len;
        out[1] = (in[1] << 12) / len;
        out[2] = (in[2] << 12) / len;
    } else {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
    }
}

/* @8015465C: copy n words. */
void __attribute__((section(".text_8015465C"))) mathCopyWords(s32 *dst, s32 *src, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++)
        *dst++ = *src++;
}

/* @801555C4: projection scale of view i's mode. */
s32 __attribute__((section(".text_801555C4"))) viewGetProjH(int i)
{
    return viewModeTable[viewModes[i]].scale;
}

/* @801555FC: the same (second entry point). */
s32 __attribute__((section(".text_801555FC"))) viewGetProjScaleX(int i)
{
    return viewModeTable[viewModes[i]].scale;
}

/* @80156860 */
s32 __attribute__((section(".text_80156860"))) viewGetRenderSplitMode(void)
{
    return viewRenderSplitMode;
}

/* @8015686C mathBoundVector: wrap three angles into [-2048, 2048). */
void __attribute__((section(".text_8015686C"))) mathBoundVector(s32 *v)
{
    while (v[0] >= 2048)
        v[0] -= 4096;
    while (v[0] < -2048)
        v[0] += 4096;
    while (v[1] >= 2048)
        v[1] -= 4096;
    while (v[1] < -2048)
        v[1] += 4096;
    while (v[2] >= 2048)
        v[2] -= 4096;
    while (v[2] < -2048)
        v[2] += 4096;
}

/* @80156ABC mathBoundAngle: wrap an angle into [-2048, 2048). */
void __attribute__((section(".text_80156ABC"))) mathBoundAngle(s32 *a)
{
    while (*a >= 2048)
        *a -= 4096;
    while (*a < -2048)
        *a += 4096;
}

/* @80156B1C mathBoundSAngle: mathBoundAngle for a halfword angle. */
void __attribute__((section(".text_80156B1C"))) mathBoundSAngle(s16 *a)
{
    while (*a >= 2048)
        *a -= 4096;
    while (*a < -2048)
        *a += 4096;
}

/* @80156B8C: move angle towards target (pct% of the old angle per 30 ticks,
 * along the shorter way round), wrapped. */
s32 __attribute__((section(".text_80156B8C"))) mathBlendAngle(s32 angle, s32 target, s32 pct)
{
    s32 r;

    if (angle < -1024 && target > 1024)
        target -= 4096;
    else if (angle > 1024 && target < -1024)
        target += 4096;
    r = (angle * pct + target * (100 - pct)) / 100;
    r = angle + (r - angle) * timerGetFrameRate() / 30;
    mathBoundAngle(&r);
    return r;
}

/* @80156C70: the same blend for a plain value. */
s32 __attribute__((section(".text_80156C70"))) mathBlendValue(s32 value, s32 target, s32 pct)
{
    s32 r;

    r = (value * pct + target * (100 - pct)) / 100;
    return value + (r - value) * timerGetFrameRate() / 30;
}

/* @80156D08 soundInit: default sound settings, then start libsnd. */
void __attribute__((section(".text_80156D08"))) soundInit(void)
{
    audioMasterVolume = 127;
    D_80180E44 = 24;
    soundReservedVoices = TM2_SPU_VOICES;  /* libsnd's voices: 24, or 48 with TM2_PORT_SPU48 (SPU_VOICES.md) */
    soundNoiseClock = 45;
    D_80180E08 = 99;
    audioVolumeClampMin = 0;
    audioSfxVolumeCap = 127;
    D_80180E1C = 0;
    audioMusicVolumeCap = 127;
    D_80180E10 = 0;
    soundNoteMax = 127;
    D_80180DFC = 0;
    soundFineMax = 127;
    D_80180DEC = 0;
    D_80180E3C = 0;
    D_80180E04 = 0;
    soundInstPitchScale = 128;
    soundFineStepsPerNote = 128;
    audioVolumeClampMax = 127;
    audioSfxVolume = 127;
    audioCdVolume = 127;
    sub_8015700C();
    sub_801583C8((u8)soundReservedVoices);
    sub_8015D640(0);
    sub_80157DD4(1);
    sub_80157380();
    sub_80156EA8(audioMasterVolume, audioMasterVolume);
    sub_80156E08(soundMVolResultPtr);
    sub_801583B8();
    sub_8015D5F0(soundNoiseClock);
}

/* @80156DE0: stop libsnd. */
void __attribute__((section(".text_80156DE0"))) soundQuit(void)
{
    sub_80157F48();
    sub_8015704C();
}

#ifdef TM2_PORT_C_EMITTER
/* PC port request 82 (port_c_emitter.h, PLAYTEST_ANSWERS.md 27): the port variant with the C emitter also links the near
 * pass's pure-C form (the copy in reference/ that the all-C build proves; tools/check_custom.py check 8 runs it against
 * this asm), whose packets and OT links are the asm's, and which reports its split vertices to the precise ring. Only
 * the near pass is taken from there: this unit's other functions are its own. */
#define TM2_NEAR_PASS_ONLY
#include "../../reference/near_pass_engine_math_near_pass.c"
#else
/* ---- hand-written assembly (kept as the original instruction sequence) ---- */

__asm__(
    ".section .text_80154690,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl renderNearPolys\n"
    ".type renderNearPolys,@function\n"
    "renderNearPolys:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\tlui $8,0x1f80\n"   /* 80154690: 3c081f80 */
    "\tsw $16,0($8)\n"   /* 80154694: ad100000 */
    "\tsw $17,4($8)\n"   /* 80154698: ad110004 */
    "\tsw $18,8($8)\n"   /* 8015469C: ad120008 */
    "\tsw $19,12($8)\n"   /* 801546A0: ad13000c */
    "\tsw $20,16($8)\n"   /* 801546A4: ad140010 */
    "\tsw $21,20($8)\n"   /* 801546A8: ad150014 */
    "\tsw $22,24($8)\n"   /* 801546AC: ad160018 */
    "\tsw $23,28($8)\n"   /* 801546B0: ad17001c */
    "\tsw $24,32($8)\n"   /* 801546B4: ad180020 */
    "\tsw $25,36($8)\n"   /* 801546B8: ad190024 */
    "\tsw $31,40($8)\n"   /* 801546BC: ad1f0028 */
    "\tsw $30,44($8)\n"   /* 801546C0: ad1e002c */
    "\tsw $28,48($8)\n"   /* 801546C4: ad1c0030 */
    "\tsw $29,52($8)\n"   /* 801546C8: ad1d0034 */
    "\tsw $4,64($8)\n"   /* 801546CC: ad040040 */
    "\tsw $5,76($8)\n"   /* 801546D0: ad05004c */
    "\tlw $17,12($4)\n"   /* 801546D4: 8c91000c */
    "\tlui $9,0x1f80\n"   /* 801546D8: 3c091f80 */
    "\tsw $17,60($8)\n"   /* 801546DC: ad11003c */
    "\tori $9,$9,0xa4\n"   /* 801546E0: 352900a4 */
    "\tsw $0,72($8)\n"   /* 801546E4: ad000048 */
    ".L801546E8:\n"
    "\tblez $6,.L80154DF4\n"   /* 801546E8: 18c001c2 */
    "\tsw $6,80($8)\n"   /* 801546EC: ad060050 */
    "\tlw $16,0($9)\n"   /* 801546F0: 8d300000 */
    "\tlw $21,4($9)\n"   /* 801546F4: 8d350004 */
    "\tlw $17,12($16)\n"   /* 801546F8: 8e11000c */
    "\tli $18,0x3\n"   /* 801546FC: 34120003 */
    "\tsw $17,68($8)\n"   /* 80154700: ad110044 */
    "\tsrl $19,$17,0x16\n"   /* 80154704: 00119d82 */
    "\tsrl $20,$17,0x10\n"   /* 80154708: 0011a402 */
    "\tandi $20,$20,0x7\n"   /* 8015470C: 32940007 */
    "\tandi $11,$19,0x3\n"   /* 80154710: 326b0003 */
    "\tsll $11,$11,0x2\n"   /* 80154714: 000b5880 */
    "\tlbu $10,3($16)\n"   /* 80154718: 920a0003 */
    "\taddiu $12,$21,8\n"   /* 8015471C: 26ac0008 */
    "\taddu $13,$12,$11\n"   /* 80154720: 018b6821 */
    "\tlw $17,64($8)\n"   /* 80154724: 8d110040 */
    "\taddu $14,$13,$11\n"   /* 80154728: 01ab7021 */
    "\tlw $24,8($17)\n"   /* 8015472C: 8e380008 */
    "\taddiu $10,$10,1\n"   /* 80154730: 254a0001 */
    "\tsll $22,$10,0x2\n"   /* 80154734: 000ab080 */
    "\taddu $25,$24,$22\n"   /* 80154738: 0316c821 */
    "\taddu $7,$25,$22\n"   /* 8015473C: 03363821 */
    "\tlw $17,60($8)\n"   /* 80154740: 8d11003c */
    "\taddiu $19,$7,160\n"   /* 80154744: 24f300a0 */
    "\tsltu $19,$17,$19\n"   /* 80154748: 0233982b */
    "\tbnez $19,.L80154DF4\n"   /* 8015474C: 166001a9 */
    "\tnop\n"   /* 80154750: 00000000 */
    "\tbne $18,$20,.L801549A4\n"   /* 80154754: 16540093 */
    "\tnop\n"   /* 80154758: 00000000 */
    "\tli $21,0xc\n"   /* 8015475C: 3415000c */
    "\tsw $21,88($8)\n"   /* 80154760: ad150058 */
    "\tlw $21,0($12)\n"   /* 80154764: 8d950000 */
    "\tlw $22,0($13)\n"   /* 80154768: 8db60000 */
    "\tlw $23,0($14)\n"   /* 8015476C: 8dd70000 */
    "\tsrl $4,$21,0x10\n"   /* 80154770: 00152402 */
    "\tandi $17,$21,0xffff\n"   /* 80154774: 32b1ffff */
    "\tsrl $19,$22,0x10\n"   /* 80154778: 00169c02 */
    "\tandi $2,$22,0xffff\n"   /* 8015477C: 32c2ffff */
    "\tsrl $20,$23,0x10\n"   /* 80154780: 0017a402 */
    "\tandi $3,$23,0xffff\n"   /* 80154784: 32e3ffff */
    "\taddu $5,$17,$2\n"   /* 80154788: 02222821 */
    "\taddu $18,$4,$19\n"   /* 8015478C: 00939021 */
    "\tsra $5,$5,0x1\n"   /* 80154790: 00052843 */
    "\tsra $18,$18,0x1\n"   /* 80154794: 00129043 */
    "\tsll $18,$18,0x10\n"   /* 80154798: 00129400 */
    "\tor $18,$18,$5\n"   /* 8015479C: 02459025 */
    "\taddu $2,$2,$3\n"   /* 801547A0: 00431021 */
    "\taddu $19,$19,$20\n"   /* 801547A4: 02749821 */
    "\tsra $2,$2,0x1\n"   /* 801547A8: 00021043 */
    "\tsra $19,$19,0x1\n"   /* 801547AC: 00139843 */
    "\tsll $19,$19,0x10\n"   /* 801547B0: 00139c00 */
    "\tor $19,$19,$2\n"   /* 801547B4: 02629825 */
    "\taddu $17,$3,$17\n"   /* 801547B8: 00718821 */
    "\taddu $4,$20,$4\n"   /* 801547BC: 02842021 */
    "\tsra $17,$17,0x1\n"   /* 801547C0: 00118843 */
    "\tsra $4,$4,0x1\n"   /* 801547C4: 00042043 */
    "\tsll $4,$4,0x10\n"   /* 801547C8: 00042400 */
    "\tor $20,$4,$17\n"   /* 801547CC: 0091a025 */
    "\tjal renderNearSplitTri\n"   /* 801547D0: 0c05538c */
    "\tli $2,0x8\n"   /* 801547D4: 34020008 */
    "\tlw $17,0($16)\n"   /* 801547D8: 8e110000 */
    "\tnop\n"   /* 801547DC: 00000000 */
    "\tandi $17,$17,0x7f\n"   /* 801547E0: 3231007f */
    "\tbeqz $17,.L80154894\n"   /* 801547E4: 1220002b */
    "\tnop\n"   /* 801547E8: 00000000 */
    "\taddiu $12,$12,4\n"   /* 801547EC: 258c0004 */
    "\taddiu $13,$13,4\n"   /* 801547F0: 25ad0004 */
    "\taddiu $14,$14,4\n"   /* 801547F4: 25ce0004 */
    "\tlw $21,0($12)\n"   /* 801547F8: 8d950000 */
    "\tlw $22,0($13)\n"   /* 801547FC: 8db60000 */
    "\tlw $23,0($14)\n"   /* 80154800: 8dd70000 */
    "\tandi $17,$21,0xff\n"   /* 80154804: 32b100ff */
    "\tandi $2,$22,0xff\n"   /* 80154808: 32c200ff */
    "\tandi $3,$23,0xff\n"   /* 8015480C: 32e300ff */
    "\tandi $4,$21,0xffff\n"   /* 80154810: 32a4ffff */
    "\tandi $19,$22,0xffff\n"   /* 80154814: 32d3ffff */
    "\tandi $20,$23,0xffff\n"   /* 80154818: 32f4ffff */
    "\tsrl $4,$4,0x8\n"   /* 8015481C: 00042202 */
    "\tsrl $19,$19,0x8\n"   /* 80154820: 00139a02 */
    "\tsrl $20,$20,0x8\n"   /* 80154824: 0014a202 */
    "\taddu $5,$17,$2\n"   /* 80154828: 02222821 */
    "\taddu $18,$4,$19\n"   /* 8015482C: 00939021 */
    "\tsra $5,$5,0x1\n"   /* 80154830: 00052843 */
    "\tsra $18,$18,0x1\n"   /* 80154834: 00129043 */
    "\tsll $18,$18,0x8\n"   /* 80154838: 00129200 */
    "\tor $18,$18,$5\n"   /* 8015483C: 02459025 */
    "\taddu $2,$2,$3\n"   /* 80154840: 00431021 */
    "\taddu $19,$19,$20\n"   /* 80154844: 02749821 */
    "\tsra $2,$2,0x1\n"   /* 80154848: 00021043 */
    "\tsra $19,$19,0x1\n"   /* 8015484C: 00139843 */
    "\tsll $19,$19,0x8\n"   /* 80154850: 00139a00 */
    "\tor $19,$19,$2\n"   /* 80154854: 02629825 */
    "\taddu $17,$3,$17\n"   /* 80154858: 00718821 */
    "\taddu $4,$20,$4\n"   /* 8015485C: 02842021 */
    "\tsra $17,$17,0x1\n"   /* 80154860: 00118843 */
    "\tsra $4,$4,0x1\n"   /* 80154864: 00042043 */
    "\tsll $4,$4,0x8\n"   /* 80154868: 00042200 */
    "\tor $20,$4,$17\n"   /* 8015486C: 0091a025 */
    "\tlh $17,2($12)\n"   /* 80154870: 85910002 */
    "\tlh $3,2($13)\n"   /* 80154874: 85a30002 */
    "\tsll $17,$17,0x10\n"   /* 80154878: 00118c00 */
    "\tsw $3,84($8)\n"   /* 8015487C: ad030054 */
    "\tsll $3,$3,0x10\n"   /* 80154880: 00031c00 */
    "\tor $19,$19,$17\n"   /* 80154884: 02719825 */
    "\tor $20,$20,$3\n"   /* 80154888: 0283a025 */
    "\tjal renderNearSplitTri\n"   /* 8015488C: 0c05538c */
    "\tli $2,0xc\n"   /* 80154890: 3402000c */
    ".L80154894:\n"
    "\tlw $17,4($9)\n"   /* 80154894: 8d310004 */
    "\tlw $18,68($8)\n"   /* 80154898: 8d120044 */
    "\tlbu $20,1($16)\n"   /* 8015489C: 92140001 */
    "\tsrl $18,$18,0x13\n"   /* 801548A0: 001294c2 */
    "\taddiu $12,$17,4\n"   /* 801548A4: 262c0004 */
    "\tsltiu $21,$20,2\n"   /* 801548A8: 2e950002 */
    "\taddu $13,$12,$11\n"   /* 801548AC: 018b6821 */
    "\tbeqz $21,.L801548C4\n"   /* 801548B0: 12a00004 */
    "\tandi $18,$18,0x7\n"   /* 801548B4: 32520007 */
    "\tsltiu $18,$18,2\n"   /* 801548B8: 2e520002 */
    "\tbnez $18,.L80154DB0\n"   /* 801548BC: 1640013c */
    "\tnop\n"   /* 801548C0: 00000000 */
    ".L801548C4:\n"
    "\taddu $14,$13,$11\n"   /* 801548C4: 01ab7021 */
    "\tlw $21,0($12)\n"   /* 801548C8: 8d950000 */
    "\tlw $22,0($13)\n"   /* 801548CC: 8db60000 */
    "\tlw $23,0($14)\n"   /* 801548D0: 8dd70000 */
    "\tandi $4,$21,0xff\n"   /* 801548D4: 32a400ff */
    "\tsrl $17,$21,0x8\n"   /* 801548D8: 00158a02 */
    "\tandi $17,$17,0xff\n"   /* 801548DC: 323100ff */
    "\tsrl $2,$21,0x10\n"   /* 801548E0: 00151402 */
    "\tandi $2,$2,0xff\n"   /* 801548E4: 304200ff */
    "\tandi $19,$22,0xff\n"   /* 801548E8: 32d300ff */
    "\tsrl $3,$22,0x8\n"   /* 801548EC: 00161a02 */
    "\tandi $3,$3,0xff\n"   /* 801548F0: 306300ff */
    "\tsrl $6,$22,0x10\n"   /* 801548F4: 00163402 */
    "\tandi $6,$6,0xff\n"   /* 801548F8: 30c600ff */
    "\tandi $20,$23,0xff\n"   /* 801548FC: 32f400ff */
    "\tsrl $30,$23,0x8\n"   /* 80154900: 0017f202 */
    "\tandi $30,$30,0xff\n"   /* 80154904: 33de00ff */
    "\tsrl $29,$23,0x10\n"   /* 80154908: 0017ec02 */
    "\tandi $29,$29,0xff\n"   /* 8015490C: 33bd00ff */
    "\taddu $18,$4,$19\n"   /* 80154910: 00939021 */
    "\taddu $5,$17,$3\n"   /* 80154914: 02232821 */
    "\tsra $18,$18,0x1\n"   /* 80154918: 00129043 */
    "\tsra $5,$5,0x1\n"   /* 8015491C: 00052843 */
    "\tsll $5,$5,0x8\n"   /* 80154920: 00052a00 */
    "\tor $18,$18,$5\n"   /* 80154924: 02459025 */
    "\taddu $5,$2,$6\n"   /* 80154928: 00462821 */
    "\tsra $5,$5,0x1\n"   /* 8015492C: 00052843 */
    "\tsll $5,$5,0x10\n"   /* 80154930: 00052c00 */
    "\tor $18,$18,$5\n"   /* 80154934: 02459025 */
    "\taddu $19,$19,$20\n"   /* 80154938: 02749821 */
    "\taddu $5,$3,$30\n"   /* 8015493C: 007e2821 */
    "\tsra $19,$19,0x1\n"   /* 80154940: 00139843 */
    "\tsra $5,$5,0x1\n"   /* 80154944: 00052843 */
    "\tsll $5,$5,0x8\n"   /* 80154948: 00052a00 */
    "\tor $19,$19,$5\n"   /* 8015494C: 02659825 */
    "\taddu $5,$6,$29\n"   /* 80154950: 00dd2821 */
    "\tsra $5,$5,0x1\n"   /* 80154954: 00052843 */
    "\tsll $5,$5,0x10\n"   /* 80154958: 00052c00 */
    "\tor $19,$19,$5\n"   /* 8015495C: 02659825 */
    "\tsrl $5,$23,0x18\n"   /* 80154960: 00172e02 */
    "\tsll $5,$5,0x18\n"   /* 80154964: 00052e00 */
    "\tor $19,$19,$5\n"   /* 80154968: 02659825 */
    "\taddu $20,$20,$4\n"   /* 8015496C: 0284a021 */
    "\taddu $5,$30,$17\n"   /* 80154970: 03d12821 */
    "\tsra $20,$20,0x1\n"   /* 80154974: 0014a043 */
    "\tsra $5,$5,0x1\n"   /* 80154978: 00052843 */
    "\tsll $5,$5,0x8\n"   /* 8015497C: 00052a00 */
    "\tor $20,$20,$5\n"   /* 80154980: 0285a025 */
    "\taddu $5,$29,$2\n"   /* 80154984: 03a22821 */
    "\tsra $5,$5,0x1\n"   /* 80154988: 00052843 */
    "\tsll $5,$5,0x10\n"   /* 8015498C: 00052c00 */
    "\tor $20,$20,$5\n"   /* 80154990: 0285a025 */
    "\tjal renderNearSplitTri\n"   /* 80154994: 0c05538c */
    "\tli $2,0x4\n"   /* 80154998: 34020004 */
    "\tj .L80154DC4\n"   /* 8015499C: 08055371 */
    "\tnop\n"   /* 801549A0: 00000000 */
    ".L801549A4:\n"
    "\taddu $15,$14,$11\n"   /* 801549A4: 01cb7821 */
    "\tli $21,0x10\n"   /* 801549A8: 34150010 */
    "\tsw $21,88($8)\n"   /* 801549AC: ad150058 */
    "\tlw $17,0($12)\n"   /* 801549B0: 8d910000 */
    "\tlw $18,0($13)\n"   /* 801549B4: 8db20000 */
    "\tlw $19,0($14)\n"   /* 801549B8: 8dd30000 */
    "\tlw $20,0($15)\n"   /* 801549BC: 8df40000 */
    "\tsrl $22,$17,0x10\n"   /* 801549C0: 0011b402 */
    "\tandi $21,$17,0xffff\n"   /* 801549C4: 3235ffff */
    "\tsrl $2,$18,0x10\n"   /* 801549C8: 00121402 */
    "\tandi $23,$18,0xffff\n"   /* 801549CC: 3257ffff */
    "\tsrl $4,$19,0x10\n"   /* 801549D0: 00132402 */
    "\tandi $3,$19,0xffff\n"   /* 801549D4: 3263ffff */
    "\tsrl $6,$20,0x10\n"   /* 801549D8: 00143402 */
    "\tandi $5,$20,0xffff\n"   /* 801549DC: 3285ffff */
    "\taddu $29,$21,$3\n"   /* 801549E0: 02a3e821 */
    "\taddu $30,$22,$4\n"   /* 801549E4: 02c4f021 */
    "\tsra $29,$29,0x1\n"   /* 801549E8: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 801549EC: 001ef043 */
    "\tsll $30,$30,0x10\n"   /* 801549F0: 001ef400 */
    "\tor $30,$29,$30\n"   /* 801549F4: 03bef025 */
    "\tsw $30,120($8)\n"   /* 801549F8: ad1e0078 */
    "\taddu $29,$23,$21\n"   /* 801549FC: 02f5e821 */
    "\taddu $30,$2,$22\n"   /* 80154A00: 0056f021 */
    "\tsra $29,$29,0x1\n"   /* 80154A04: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154A08: 001ef043 */
    "\tsll $30,$30,0x10\n"   /* 80154A0C: 001ef400 */
    "\tor $30,$29,$30\n"   /* 80154A10: 03bef025 */
    "\tsw $30,124($8)\n"   /* 80154A14: ad1e007c */
    "\taddu $29,$23,$5\n"   /* 80154A18: 02e5e821 */
    "\taddu $30,$2,$6\n"   /* 80154A1C: 0046f021 */
    "\tsra $29,$29,0x1\n"   /* 80154A20: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154A24: 001ef043 */
    "\tsll $30,$30,0x10\n"   /* 80154A28: 001ef400 */
    "\tor $30,$29,$30\n"   /* 80154A2C: 03bef025 */
    "\tsw $30,128($8)\n"   /* 80154A30: ad1e0080 */
    "\taddu $29,$5,$3\n"   /* 80154A34: 00a3e821 */
    "\taddu $30,$6,$4\n"   /* 80154A38: 00c4f021 */
    "\tsra $29,$29,0x1\n"   /* 80154A3C: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154A40: 001ef043 */
    "\tsll $30,$30,0x10\n"   /* 80154A44: 001ef400 */
    "\tor $30,$29,$30\n"   /* 80154A48: 03bef025 */
    "\tsw $30,132($8)\n"   /* 80154A4C: ad1e0084 */
    "\tlw $21,120($8)\n"   /* 80154A50: 8d150078 */
    "\tlw $22,128($8)\n"   /* 80154A54: 8d160080 */
    "\tsrl $23,$21,0x10\n"   /* 80154A58: 0015bc02 */
    "\tandi $2,$21,0xffff\n"   /* 80154A5C: 32a2ffff */
    "\tsrl $3,$22,0x10\n"   /* 80154A60: 00161c02 */
    "\tandi $4,$22,0xffff\n"   /* 80154A64: 32c4ffff */
    "\taddu $5,$2,$4\n"   /* 80154A68: 00442821 */
    "\taddu $6,$23,$3\n"   /* 80154A6C: 02e33021 */
    "\tsra $5,$5,0x1\n"   /* 80154A70: 00052843 */
    "\tsra $6,$6,0x1\n"   /* 80154A74: 00063043 */
    "\tsll $6,$6,0x10\n"   /* 80154A78: 00063400 */
    "\tor $6,$5,$6\n"   /* 80154A7C: 00a63025 */
    "\tsw $6,136($8)\n"   /* 80154A80: ad060088 */
    "\tjal renderNearSplitQuad\n"   /* 80154A84: 0c055422 */
    "\tli $2,0x8\n"   /* 80154A88: 34020008 */
    "\tlw $17,0($16)\n"   /* 80154A8C: 8e110000 */
    "\tnop\n"   /* 80154A90: 00000000 */
    "\tandi $17,$17,0x7f\n"   /* 80154A94: 3231007f */
    "\tbeqz $17,.L80154BD4\n"   /* 80154A98: 1220004e */
    "\tnop\n"   /* 80154A9C: 00000000 */
    "\taddiu $12,$12,4\n"   /* 80154AA0: 258c0004 */
    "\taddiu $13,$13,4\n"   /* 80154AA4: 25ad0004 */
    "\taddiu $14,$14,4\n"   /* 80154AA8: 25ce0004 */
    "\taddiu $15,$15,4\n"   /* 80154AAC: 25ef0004 */
    "\tlw $17,0($12)\n"   /* 80154AB0: 8d910000 */
    "\tlw $18,0($13)\n"   /* 80154AB4: 8db20000 */
    "\tlw $19,0($14)\n"   /* 80154AB8: 8dd30000 */
    "\tlw $20,0($15)\n"   /* 80154ABC: 8df40000 */
    "\tandi $21,$17,0xff\n"   /* 80154AC0: 323500ff */
    "\tandi $23,$18,0xff\n"   /* 80154AC4: 325700ff */
    "\tandi $3,$19,0xff\n"   /* 80154AC8: 326300ff */
    "\tandi $5,$20,0xff\n"   /* 80154ACC: 328500ff */
    "\tandi $22,$17,0xffff\n"   /* 80154AD0: 3236ffff */
    "\tandi $2,$18,0xffff\n"   /* 80154AD4: 3242ffff */
    "\tandi $4,$19,0xffff\n"   /* 80154AD8: 3264ffff */
    "\tandi $6,$20,0xffff\n"   /* 80154ADC: 3286ffff */
    "\tsrl $22,$22,0x8\n"   /* 80154AE0: 0016b202 */
    "\tsrl $2,$2,0x8\n"   /* 80154AE4: 00021202 */
    "\tsrl $4,$4,0x8\n"   /* 80154AE8: 00042202 */
    "\tsrl $6,$6,0x8\n"   /* 80154AEC: 00063202 */
    "\taddu $29,$21,$3\n"   /* 80154AF0: 02a3e821 */
    "\taddu $30,$22,$4\n"   /* 80154AF4: 02c4f021 */
    "\tsra $29,$29,0x1\n"   /* 80154AF8: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154AFC: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154B00: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154B04: 03bef025 */
    "\tsw $30,120($8)\n"   /* 80154B08: ad1e0078 */
    "\taddu $29,$23,$21\n"   /* 80154B0C: 02f5e821 */
    "\taddu $30,$2,$22\n"   /* 80154B10: 0056f021 */
    "\tsra $29,$29,0x1\n"   /* 80154B14: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154B18: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154B1C: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154B20: 03bef025 */
    "\tsw $30,124($8)\n"   /* 80154B24: ad1e007c */
    "\taddu $29,$23,$5\n"   /* 80154B28: 02e5e821 */
    "\taddu $30,$2,$6\n"   /* 80154B2C: 0046f021 */
    "\tsra $29,$29,0x1\n"   /* 80154B30: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154B34: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154B38: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154B3C: 03bef025 */
    "\tsw $30,128($8)\n"   /* 80154B40: ad1e0080 */
    "\taddu $29,$5,$3\n"   /* 80154B44: 00a3e821 */
    "\taddu $30,$6,$4\n"   /* 80154B48: 00c4f021 */
    "\tsra $29,$29,0x1\n"   /* 80154B4C: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154B50: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154B54: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154B58: 03bef025 */
    "\tsw $30,132($8)\n"   /* 80154B5C: ad1e0084 */
    "\tlw $29,120($8)\n"   /* 80154B60: 8d1d0078 */
    "\tlw $30,128($8)\n"   /* 80154B64: 8d1e0080 */
    "\tandi $21,$29,0xff\n"   /* 80154B68: 33b500ff */
    "\tandi $22,$30,0xff\n"   /* 80154B6C: 33d600ff */
    "\tandi $23,$29,0xffff\n"   /* 80154B70: 33b7ffff */
    "\tandi $2,$30,0xffff\n"   /* 80154B74: 33c2ffff */
    "\tsrl $23,$23,0x8\n"   /* 80154B78: 0017ba02 */
    "\tsrl $2,$2,0x8\n"   /* 80154B7C: 00021202 */
    "\taddu $29,$21,$22\n"   /* 80154B80: 02b6e821 */
    "\taddu $30,$23,$2\n"   /* 80154B84: 02e2f021 */
    "\tsra $29,$29,0x1\n"   /* 80154B88: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154B8C: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154B90: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154B94: 03bef025 */
    "\tsw $30,136($8)\n"   /* 80154B98: ad1e0088 */
    "\tlh $22,2($13)\n"   /* 80154B9C: 85b60002 */
    "\tlh $21,2($12)\n"   /* 80154BA0: 85950002 */
    "\tsw $22,84($8)\n"   /* 80154BA4: ad160054 */
    "\tsh $21,122($8)\n"   /* 80154BA8: a515007a */
    "\tsh $22,138($8)\n"   /* 80154BAC: a516008a */
    "\tsh $22,126($8)\n"   /* 80154BB0: a516007e */
    "\tsh $22,130($8)\n"   /* 80154BB4: a5160082 */
    "\tjal renderNearSplitQuad\n"   /* 80154BB8: 0c055422 */
    "\tli $2,0xc\n"   /* 80154BBC: 3402000c */
    "\tlh $21,2($12)\n"   /* 80154BC0: 85950002 */
    "\taddiu $5,$25,12\n"   /* 80154BC4: 2725000c */
    "\tsh $21,2($5)\n"   /* 80154BC8: a4b50002 */
    "\taddiu $5,$7,12\n"   /* 80154BCC: 24e5000c */
    "\tsh $21,2($5)\n"   /* 80154BD0: a4b50002 */
    ".L80154BD4:\n"
    "\tlw $17,4($9)\n"   /* 80154BD4: 8d310004 */
    "\tlw $18,68($8)\n"   /* 80154BD8: 8d120044 */
    "\tlbu $20,1($16)\n"   /* 80154BDC: 92140001 */
    "\tsrl $18,$18,0x13\n"   /* 80154BE0: 001294c2 */
    "\taddiu $12,$17,4\n"   /* 80154BE4: 262c0004 */
    "\tsltiu $21,$20,2\n"   /* 80154BE8: 2e950002 */
    "\taddu $13,$12,$11\n"   /* 80154BEC: 018b6821 */
    "\tbeqz $21,.L80154C04\n"   /* 80154BF0: 12a00004 */
    "\tandi $18,$18,0x7\n"   /* 80154BF4: 32520007 */
    "\tsltiu $18,$18,2\n"   /* 80154BF8: 2e520002 */
    "\tbnez $18,.L80154DB0\n"   /* 80154BFC: 1640006c */
    "\tnop\n"   /* 80154C00: 00000000 */
    ".L80154C04:\n"
    "\taddu $14,$13,$11\n"   /* 80154C04: 01ab7021 */
    "\taddu $15,$14,$11\n"   /* 80154C08: 01cb7821 */
    "\tlw $17,0($12)\n"   /* 80154C0C: 8d910000 */
    "\tlw $18,0($13)\n"   /* 80154C10: 8db20000 */
    "\tlw $19,0($14)\n"   /* 80154C14: 8dd30000 */
    "\tandi $21,$17,0xff\n"   /* 80154C18: 323500ff */
    "\tsrl $22,$17,0x8\n"   /* 80154C1C: 0011b202 */
    "\tandi $22,$22,0xff\n"   /* 80154C20: 32d600ff */
    "\tsrl $28,$17,0x10\n"   /* 80154C24: 0011e402 */
    "\tandi $28,$28,0xff\n"   /* 80154C28: 339c00ff */
    "\tandi $2,$18,0xff\n"   /* 80154C2C: 324200ff */
    "\tsrl $3,$18,0x8\n"   /* 80154C30: 00121a02 */
    "\tandi $3,$3,0xff\n"   /* 80154C34: 306300ff */
    "\tsrl $4,$18,0x10\n"   /* 80154C38: 00122402 */
    "\tandi $4,$4,0xff\n"   /* 80154C3C: 308400ff */
    "\tandi $5,$19,0xff\n"   /* 80154C40: 326500ff */
    "\tsrl $6,$19,0x8\n"   /* 80154C44: 00133202 */
    "\tandi $6,$6,0xff\n"   /* 80154C48: 30c600ff */
    "\tsrl $23,$19,0x10\n"   /* 80154C4C: 0013bc02 */
    "\tandi $23,$23,0xff\n"   /* 80154C50: 32f700ff */
    "\taddu $29,$21,$5\n"   /* 80154C54: 02a5e821 */
    "\taddu $30,$22,$6\n"   /* 80154C58: 02c6f021 */
    "\tsra $29,$29,0x1\n"   /* 80154C5C: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154C60: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154C64: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154C68: 03bef025 */
    "\taddu $29,$28,$23\n"   /* 80154C6C: 0397e821 */
    "\tsra $29,$29,0x1\n"   /* 80154C70: 001de843 */
    "\tsll $29,$29,0x10\n"   /* 80154C74: 001dec00 */
    "\tor $29,$30,$29\n"   /* 80154C78: 03dde825 */
    "\tsrl $30,$17,0x18\n"   /* 80154C7C: 0011f602 */
    "\tsll $30,$30,0x18\n"   /* 80154C80: 001ef600 */
    "\tor $29,$30,$29\n"   /* 80154C84: 03dde825 */
    "\tsw $29,120($8)\n"   /* 80154C88: ad1d0078 */
    "\taddu $29,$2,$21\n"   /* 80154C8C: 0055e821 */
    "\taddu $30,$3,$22\n"   /* 80154C90: 0076f021 */
    "\tsra $29,$29,0x1\n"   /* 80154C94: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154C98: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154C9C: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154CA0: 03bef025 */
    "\taddu $29,$4,$28\n"   /* 80154CA4: 009ce821 */
    "\tsra $29,$29,0x1\n"   /* 80154CA8: 001de843 */
    "\tsll $29,$29,0x10\n"   /* 80154CAC: 001dec00 */
    "\tor $29,$30,$29\n"   /* 80154CB0: 03dde825 */
    "\tsrl $30,$17,0x18\n"   /* 80154CB4: 0011f602 */
    "\tsll $30,$30,0x18\n"   /* 80154CB8: 001ef600 */
    "\tor $29,$30,$29\n"   /* 80154CBC: 03dde825 */
    "\tsw $29,124($8)\n"   /* 80154CC0: ad1d007c */
    "\tlw $20,0($15)\n"   /* 80154CC4: 8df40000 */
    "\tnop\n"   /* 80154CC8: 00000000 */
    "\tandi $21,$20,0xff\n"   /* 80154CCC: 329500ff */
    "\tsrl $22,$20,0x8\n"   /* 80154CD0: 0014b202 */
    "\tandi $22,$22,0xff\n"   /* 80154CD4: 32d600ff */
    "\tsrl $28,$20,0x10\n"   /* 80154CD8: 0014e402 */
    "\tandi $28,$28,0xff\n"   /* 80154CDC: 339c00ff */
    "\taddu $29,$2,$21\n"   /* 80154CE0: 0055e821 */
    "\taddu $30,$3,$22\n"   /* 80154CE4: 0076f021 */
    "\tsra $29,$29,0x1\n"   /* 80154CE8: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154CEC: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154CF0: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154CF4: 03bef025 */
    "\taddu $29,$4,$23\n"   /* 80154CF8: 0097e821 */
    "\tsra $29,$29,0x1\n"   /* 80154CFC: 001de843 */
    "\tsll $29,$29,0x10\n"   /* 80154D00: 001dec00 */
    "\tor $29,$30,$29\n"   /* 80154D04: 03dde825 */
    "\tsw $29,128($8)\n"   /* 80154D08: ad1d0080 */
    "\taddu $29,$21,$5\n"   /* 80154D0C: 02a5e821 */
    "\taddu $30,$22,$6\n"   /* 80154D10: 02c6f021 */
    "\tsra $29,$29,0x1\n"   /* 80154D14: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154D18: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154D1C: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154D20: 03bef025 */
    "\taddu $29,$28,$4\n"   /* 80154D24: 0384e821 */
    "\tsra $29,$29,0x1\n"   /* 80154D28: 001de843 */
    "\tsll $29,$29,0x10\n"   /* 80154D2C: 001dec00 */
    "\tor $29,$30,$29\n"   /* 80154D30: 03dde825 */
    "\tsw $29,132($8)\n"   /* 80154D34: ad1d0084 */
    "\tlw $21,120($8)\n"   /* 80154D38: 8d150078 */
    "\tlw $22,128($8)\n"   /* 80154D3C: 8d160080 */
    "\tandi $23,$21,0xff\n"   /* 80154D40: 32b700ff */
    "\tsrl $2,$21,0x8\n"   /* 80154D44: 00151202 */
    "\tandi $2,$2,0xff\n"   /* 80154D48: 304200ff */
    "\tsrl $3,$21,0x10\n"   /* 80154D4C: 00151c02 */
    "\tandi $3,$3,0xff\n"   /* 80154D50: 306300ff */
    "\tandi $4,$22,0xff\n"   /* 80154D54: 32c400ff */
    "\tsrl $5,$22,0x8\n"   /* 80154D58: 00162a02 */
    "\tandi $5,$5,0xff\n"   /* 80154D5C: 30a500ff */
    "\tsrl $6,$22,0x10\n"   /* 80154D60: 00163402 */
    "\tandi $6,$6,0xff\n"   /* 80154D64: 30c600ff */
    "\taddu $29,$23,$4\n"   /* 80154D68: 02e4e821 */
    "\taddu $30,$2,$5\n"   /* 80154D6C: 0045f021 */
    "\tsra $29,$29,0x1\n"   /* 80154D70: 001de843 */
    "\tsra $30,$30,0x1\n"   /* 80154D74: 001ef043 */
    "\tsll $30,$30,0x8\n"   /* 80154D78: 001ef200 */
    "\tor $30,$29,$30\n"   /* 80154D7C: 03bef025 */
    "\taddu $29,$3,$6\n"   /* 80154D80: 0066e821 */
    "\tsra $29,$29,0x1\n"   /* 80154D84: 001de843 */
    "\tsll $29,$29,0x10\n"   /* 80154D88: 001dec00 */
    "\tor $29,$30,$29\n"   /* 80154D8C: 03dde825 */
    "\tsrl $30,$17,0x18\n"   /* 80154D90: 0011f602 */
    "\tsll $30,$30,0x18\n"   /* 80154D94: 001ef600 */
    "\tor $29,$30,$29\n"   /* 80154D98: 03dde825 */
    "\tsw $29,136($8)\n"   /* 80154D9C: ad1d0088 */
    "\tjal renderNearSplitQuad\n"   /* 80154DA0: 0c055422 */
    "\tli $2,0x4\n"   /* 80154DA4: 34020004 */
    "\tj .L80154DC4\n"   /* 80154DA8: 08055371 */
    "\tnop\n"   /* 80154DAC: 00000000 */
    ".L80154DB0:\n"
    "\tlw $21,0($12)\n"   /* 80154DB0: 8d950000 */
    "\tnop\n"   /* 80154DB4: 00000000 */
    "\tsw $21,4($24)\n"   /* 80154DB8: af150004 */
    "\tsw $21,4($25)\n"   /* 80154DBC: af350004 */
    "\tsw $21,4($7)\n"   /* 80154DC0: acf50004 */
    ".L80154DC4:\n"
    "\tjal renderNearLinkPrims\n"   /* 80154DC4: 0c0553a1 */
    "\tnop\n"   /* 80154DC8: 00000000 */
    "\tlw $17,72($8)\n"   /* 80154DCC: 8d110048 */
    "\tlw $6,80($8)\n"   /* 80154DD0: 8d060050 */
    "\tli $18,0x46\n"   /* 80154DD4: 34120046 */
    "\tbne $17,$18,.L80154DE4\n"   /* 80154DD8: 16320002 */
    "\taddiu $9,$9,12\n"   /* 80154DDC: 2529000c */
    "\tlw $9,76($8)\n"   /* 80154DE0: 8d09004c */
    ".L80154DE4:\n"
    "\taddiu $17,$17,1\n"   /* 80154DE4: 26310001 */
    "\tsw $17,72($8)\n"   /* 80154DE8: ad110048 */
    "\tj .L801546E8\n"   /* 80154DEC: 080551ba */
    "\taddiu $6,$6,-1\n"   /* 80154DF0: 24c6ffff */
    ".L80154DF4:\n"
    "\tlw $16,0($8)\n"   /* 80154DF4: 8d100000 */
    "\tlw $17,4($8)\n"   /* 80154DF8: 8d110004 */
    "\tlw $18,8($8)\n"   /* 80154DFC: 8d120008 */
    "\tlw $19,12($8)\n"   /* 80154E00: 8d13000c */
    "\tlw $20,16($8)\n"   /* 80154E04: 8d140010 */
    "\tlw $21,20($8)\n"   /* 80154E08: 8d150014 */
    "\tlw $22,24($8)\n"   /* 80154E0C: 8d160018 */
    "\tlw $23,28($8)\n"   /* 80154E10: 8d17001c */
    "\tlw $24,32($8)\n"   /* 80154E14: 8d180020 */
    "\tlw $25,36($8)\n"   /* 80154E18: 8d190024 */
    "\tlw $31,40($8)\n"   /* 80154E1C: 8d1f0028 */
    "\tlw $30,44($8)\n"   /* 80154E20: 8d1e002c */
    "\tlw $28,48($8)\n"   /* 80154E24: 8d1c0030 */
    "\tjr $31\n"   /* 80154E28: 03e00008 */
    "\tlw $29,52($8)\n"   /* 80154E2C: 8d1d0034 */
    ".set\treorder\n"
    ".set noreorder\n"
    ".size renderNearPolys,.-renderNearPolys\n");

__asm__(
    ".section .text_80154E30,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl renderNearSplitTri\n"
    ".type renderNearSplitTri,@function\n"
    "renderNearSplitTri:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\taddu $17,$24,$2\n"   /* 80154E30: 03028821 */
    "\tsw $19,0($17)\n"   /* 80154E34: ae330000 */
    "\taddu $17,$17,$11\n"   /* 80154E38: 022b8821 */
    "\tsw $22,0($17)\n"   /* 80154E3C: ae360000 */
    "\taddu $17,$17,$11\n"   /* 80154E40: 022b8821 */
    "\tsw $18,0($17)\n"   /* 80154E44: ae320000 */
    "\taddu $17,$25,$2\n"   /* 80154E48: 03228821 */
    "\tsw $19,0($17)\n"   /* 80154E4C: ae330000 */
    "\taddu $17,$17,$11\n"   /* 80154E50: 022b8821 */
    "\tsw $20,0($17)\n"   /* 80154E54: ae340000 */
    "\taddu $17,$17,$11\n"   /* 80154E58: 022b8821 */
    "\tsw $23,0($17)\n"   /* 80154E5C: ae370000 */
    "\taddu $17,$7,$2\n"   /* 80154E60: 00e28821 */
    "\tsw $19,0($17)\n"   /* 80154E64: ae330000 */
    "\taddu $17,$17,$11\n"   /* 80154E68: 022b8821 */
    "\tsw $20,0($17)\n"   /* 80154E6C: ae340000 */
    "\taddu $17,$17,$11\n"   /* 80154E70: 022b8821 */
    "\tsw $18,0($17)\n"   /* 80154E74: ae320000 */
    "\tsw $20,0($13)\n"   /* 80154E78: adb40000 */
    "\tjr $31\n"   /* 80154E7C: 03e00008 */
    "\tsw $18,0($14)\n"   /* 80154E80: add20000 */
    ".set\treorder\n"
    ".set noreorder\n"
    ".size renderNearSplitTri,.-renderNearSplitTri\n");

__asm__(
    ".section .text_80154E84,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl renderNearLinkPrims\n"
    ".type renderNearLinkPrims,@function\n"
    "renderNearLinkPrims:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\tlb $19,0($16)\n"   /* 80154E84: 82130000 */
    "\tlh $17,10($9)\n"   /* 80154E88: 8531000a */
    "\tnop\n"   /* 80154E8C: 00000000 */
    "\tmove $18,$17\n"   /* 80154E90: 02209021 */
    "\tbltz $19,.L80154EA4\n"   /* 80154E94: 06600003 */
    "\tnop\n"   /* 80154E98: 00000000 */
    "\tbgez $17,.L80154FFC\n"   /* 80154E9C: 06210057 */
    "\tnop\n"   /* 80154EA0: 00000000 */
    ".L80154EA4:\n"
    "\tbgez $17,.L80154EB0\n"   /* 80154EA4: 06210002 */
    "\tnop\n"   /* 80154EA8: 00000000 */
    "\tnegu $18,$17\n"   /* 80154EAC: 00119023 */
    ".L80154EB0:\n"
    "\tsll $21,$10,0x2\n"   /* 80154EB0: 000aa880 */
    "\taddu $20,$7,$21\n"   /* 80154EB4: 00f5a021 */
    "\tlw $2,64($8)\n"   /* 80154EB8: 8d020040 */
    "\tlw $21,96($8)\n"   /* 80154EBC: 8d150060 */
    "\tlw $22,100($8)\n"   /* 80154EC0: 8d160064 */
    "\tlw $23,104($8)\n"   /* 80154EC4: 8d170068 */
    "\tsw $21,0($20)\n"   /* 80154EC8: ae950000 */
    "\tsw $22,4($20)\n"   /* 80154ECC: ae960004 */
    "\tsw $23,8($20)\n"   /* 80154ED0: ae970008 */
    "\tlw $3,0($2)\n"   /* 80154ED4: 8c430000 */
    "\tsll $18,$18,0x2\n"   /* 80154ED8: 00129080 */
    "\taddu $3,$3,$18\n"   /* 80154EDC: 00721821 */
    "\tli $4,0x2\n"   /* 80154EE0: 34040002 */
    "\tlw $5,0($3)\n"   /* 80154EE4: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80154EE8: 80660003 */
    "\tsw $5,0($20)\n"   /* 80154EEC: ae850000 */
    "\tsb $4,3($20)\n"   /* 80154EF0: a2840003 */
    "\tsw $20,0($3)\n"   /* 80154EF4: ac740000 */
    "\tsb $6,3($3)\n"   /* 80154EF8: a0660003 */
    "\taddiu $20,$20,12\n"   /* 80154EFC: 2694000c */
    "\tlw $4,4($9)\n"   /* 80154F00: 8d240004 */
    "\taddiu $10,$10,-1\n"   /* 80154F04: 254affff */
    "\tlw $5,0($3)\n"   /* 80154F08: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80154F0C: 80660003 */
    "\tsw $5,0($4)\n"   /* 80154F10: ac850000 */
    "\tsb $10,3($4)\n"   /* 80154F14: a08a0003 */
    "\tsw $4,0($3)\n"   /* 80154F18: ac640000 */
    "\tsb $6,3($3)\n"   /* 80154F1C: a0660003 */
    "\tlw $5,0($3)\n"   /* 80154F20: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80154F24: 80660003 */
    "\tsw $5,0($24)\n"   /* 80154F28: af050000 */
    "\tsb $10,3($24)\n"   /* 80154F2C: a30a0003 */
    "\tsw $24,0($3)\n"   /* 80154F30: ac780000 */
    "\tsb $6,3($3)\n"   /* 80154F34: a0660003 */
    "\tlw $5,0($3)\n"   /* 80154F38: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80154F3C: 80660003 */
    "\tsw $5,0($25)\n"   /* 80154F40: af250000 */
    "\tsb $10,3($25)\n"   /* 80154F44: a32a0003 */
    "\tsw $25,0($3)\n"   /* 80154F48: ac790000 */
    "\tsb $6,3($3)\n"   /* 80154F4C: a0660003 */
    "\tlw $5,0($3)\n"   /* 80154F50: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80154F54: 80660003 */
    "\tsw $5,0($7)\n"   /* 80154F58: ace50000 */
    "\tsb $10,3($7)\n"   /* 80154F5C: a0ea0003 */
    "\tsw $7,0($3)\n"   /* 80154F60: ac670000 */
    "\tsb $6,3($3)\n"   /* 80154F64: a0660003 */
    "\tbgez $17,.L80154F80\n"   /* 80154F68: 06210005 */
    "\tnop\n"   /* 80154F6C: 00000000 */
    "\tlw $21,108($8)\n"   /* 80154F70: 8d15006c */
    "\tlw $22,112($8)\n"   /* 80154F74: 8d160070 */
    "\tj .L80154F8C\n"   /* 80154F78: 080553e3 */
    "\tlw $23,116($8)\n"   /* 80154F7C: 8d170074 */
    ".L80154F80:\n"
    "\tlw $21,96($8)\n"   /* 80154F80: 8d150060 */
    "\tlw $22,100($8)\n"   /* 80154F84: 8d160064 */
    "\tlw $23,104($8)\n"   /* 80154F88: 8d170068 */
    ".L80154F8C:\n"
    "\tsw $21,0($20)\n"   /* 80154F8C: ae950000 */
    "\tsw $22,4($20)\n"   /* 80154F90: ae960004 */
    "\tsw $23,8($20)\n"   /* 80154F94: ae970008 */
    "\tbgez $19,.L80154FD0\n"   /* 80154F98: 0661000d */
    "\tnop\n"   /* 80154F9C: 00000000 */
    "\tlw $22,68($8)\n"   /* 80154FA0: 8d160044 */
    "\taddiu $21,$16,16\n"   /* 80154FA4: 26150010 */
    "\tlw $23,88($8)\n"   /* 80154FA8: 8d170058 */
    "\tsrl $4,$22,0x1c\n"   /* 80154FAC: 00162702 */
    "\tandi $4,$4,0x7\n"   /* 80154FB0: 30840007 */
    "\tsll $4,$4,0x2\n"   /* 80154FB4: 00042080 */
    "\taddu $21,$21,$4\n"   /* 80154FB8: 02a4a821 */
    "\taddu $21,$21,$23\n"   /* 80154FBC: 02b7a821 */
    "\tlw $22,0($21)\n"   /* 80154FC0: 8eb60000 */
    "\tlw $23,84($8)\n"   /* 80154FC4: 8d170054 */
    "\tsw $22,8($20)\n"   /* 80154FC8: ae960008 */
    "\tsb $23,4($20)\n"   /* 80154FCC: a2970004 */
    ".L80154FD0:\n"
    "\tli $4,0x2\n"   /* 80154FD0: 34040002 */
    "\tlw $5,0($3)\n"   /* 80154FD4: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80154FD8: 80660003 */
    "\tsw $5,0($20)\n"   /* 80154FDC: ae850000 */
    "\tsb $4,3($20)\n"   /* 80154FE0: a2840003 */
    "\tsw $20,0($3)\n"   /* 80154FE4: ac740000 */
    "\tsb $6,3($3)\n"   /* 80154FE8: a0660003 */
    "\tlw $2,64($8)\n"   /* 80154FEC: 8d020040 */
    "\taddiu $20,$20,12\n"   /* 80154FF0: 2694000c */
    "\tj .L80155080\n"   /* 80154FF4: 08055420 */
    "\tsw $20,8($2)\n"   /* 80154FF8: ac540008 */
    ".L80154FFC:\n"
    "\tlw $2,64($8)\n"   /* 80154FFC: 8d020040 */
    "\tsll $21,$10,0x2\n"   /* 80155000: 000aa880 */
    "\taddu $20,$7,$21\n"   /* 80155004: 00f5a021 */
    "\tsw $20,8($2)\n"   /* 80155008: ac540008 */
    "\tlw $4,4($9)\n"   /* 8015500C: 8d240004 */
    "\taddiu $10,$10,-1\n"   /* 80155010: 254affff */
    "\tlw $3,0($2)\n"   /* 80155014: 8c430000 */
    "\tsll $17,$17,0x2\n"   /* 80155018: 00118880 */
    "\taddu $3,$3,$17\n"   /* 8015501C: 00711821 */
    "\tlw $5,0($3)\n"   /* 80155020: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80155024: 80660003 */
    "\tsw $5,0($4)\n"   /* 80155028: ac850000 */
    "\tsb $10,3($4)\n"   /* 8015502C: a08a0003 */
    "\tsw $4,0($3)\n"   /* 80155030: ac640000 */
    "\tsb $6,3($3)\n"   /* 80155034: a0660003 */
    "\tlw $5,0($3)\n"   /* 80155038: 8c650000 */
    "\tlb $6,3($3)\n"   /* 8015503C: 80660003 */
    "\tsw $5,0($24)\n"   /* 80155040: af050000 */
    "\tsb $10,3($24)\n"   /* 80155044: a30a0003 */
    "\tsw $24,0($3)\n"   /* 80155048: ac780000 */
    "\tsb $6,3($3)\n"   /* 8015504C: a0660003 */
    "\tlw $5,0($3)\n"   /* 80155050: 8c650000 */
    "\tlb $6,3($3)\n"   /* 80155054: 80660003 */
    "\tsw $5,0($25)\n"   /* 80155058: af250000 */
    "\tsb $10,3($25)\n"   /* 8015505C: a32a0003 */
    "\tsw $25,0($3)\n"   /* 80155060: ac790000 */
    "\tsb $6,3($3)\n"   /* 80155064: a0660003 */
    "\tlw $5,0($3)\n"   /* 80155068: 8c650000 */
    "\tlb $6,3($3)\n"   /* 8015506C: 80660003 */
    "\tsw $5,0($7)\n"   /* 80155070: ace50000 */
    "\tsb $10,3($7)\n"   /* 80155074: a0ea0003 */
    "\tsw $7,0($3)\n"   /* 80155078: ac670000 */
    "\tsb $6,3($3)\n"   /* 8015507C: a0660003 */
    ".L80155080:\n"
    "\tjr $31\n"   /* 80155080: 03e00008 */
    "\taddiu $10,$10,1\n"   /* 80155084: 254a0001 */
    ".set\treorder\n"
    ".set noreorder\n"
    ".size renderNearLinkPrims,.-renderNearLinkPrims\n");

__asm__(
    ".section .text_80155088,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl renderNearSplitQuad\n"
    ".type renderNearSplitQuad,@function\n"
    "renderNearSplitQuad:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\tlw $21,120($8)\n"   /* 80155088: 8d150078 */
    "\tlw $22,124($8)\n"   /* 8015508C: 8d16007c */
    "\tlw $23,128($8)\n"   /* 80155090: 8d170080 */
    "\tlw $3,132($8)\n"   /* 80155094: 8d030084 */
    "\tlw $4,136($8)\n"   /* 80155098: 8d040088 */
    "\taddu $5,$24,$2\n"   /* 8015509C: 03022821 */
    "\tsw $21,0($5)\n"   /* 801550A0: acb50000 */
    "\taddu $5,$5,$11\n"   /* 801550A4: 00ab2821 */
    "\tsw $4,0($5)\n"   /* 801550A8: aca40000 */
    "\taddu $5,$5,$11\n"   /* 801550AC: 00ab2821 */
    "\tsw $19,0($5)\n"   /* 801550B0: acb30000 */
    "\taddu $5,$5,$11\n"   /* 801550B4: 00ab2821 */
    "\tsw $3,0($5)\n"   /* 801550B8: aca30000 */
    "\taddu $5,$25,$2\n"   /* 801550BC: 03222821 */
    "\tsw $22,0($5)\n"   /* 801550C0: acb60000 */
    "\taddu $5,$5,$11\n"   /* 801550C4: 00ab2821 */
    "\tsw $18,0($5)\n"   /* 801550C8: acb20000 */
    "\taddu $5,$5,$11\n"   /* 801550CC: 00ab2821 */
    "\tsw $4,0($5)\n"   /* 801550D0: aca40000 */
    "\taddu $5,$5,$11\n"   /* 801550D4: 00ab2821 */
    "\tsw $23,0($5)\n"   /* 801550D8: acb70000 */
    "\taddu $5,$7,$2\n"   /* 801550DC: 00e22821 */
    "\tsw $4,0($5)\n"   /* 801550E0: aca40000 */
    "\taddu $5,$5,$11\n"   /* 801550E4: 00ab2821 */
    "\tsw $23,0($5)\n"   /* 801550E8: acb70000 */
    "\taddu $5,$5,$11\n"   /* 801550EC: 00ab2821 */
    "\tsw $3,0($5)\n"   /* 801550F0: aca30000 */
    "\taddu $5,$5,$11\n"   /* 801550F4: 00ab2821 */
    "\tsw $20,0($5)\n"   /* 801550F8: acb40000 */
    "\tsw $22,0($13)\n"   /* 801550FC: adb60000 */
    "\tsw $21,0($14)\n"   /* 80155100: add50000 */
    "\tjr $31\n"   /* 80155104: 03e00008 */
    "\tsw $4,0($15)\n"   /* 80155108: ade40000 */
    ".set\treorder\n"
    ".set noreorder\n"
    ".size renderNearSplitQuad,.-renderNearSplitQuad\n");

#endif /* TM2_PORT_C_EMITTER */
