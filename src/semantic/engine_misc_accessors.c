/* engine_misc_accessors: functions IDA did not list (config/US/extra_funcs.tsv).
   801374A4 codeDetectorSetLastCode, 8013762C sub_8013762C, 80145EC0 __pure_virtual, 801495B4 sub_801495B4, 8014A4F0 padGetLatched, 8014A530 padOrLatched, 8014AA24 padAnalogAxis, 8014E7D4 hdpSetPlane, 80150DD4 padSetActionCache, 80150E84 padBindActiveAction, 80150F98 padGetConfiguredBinding, 80151120 padSetRepeatDelay, 8015112C padSetRepeatInterval, 80151550 sub_80151550, 8015165C renderSetFarDistance, 801516C0 renderEnableNodeCulling, 80153E7C mathInitBitTables, 8015530C timerSetFrameTicks, 8015533C timerGetFrameUsec, 8015566C viewGetFovH, 801556A4 viewGetFovH2, 801556DC viewGetFovV, 80155714 viewGetDisplaySize, 80155DB8 viewSetFov, 8015644C viewGetRotationPtr, 80156468 viewGetRotMatrix, 801564BC viewGetPositionPtr, 80156578 viewGetParent, 8015697C mathBoundSVector, 8015F134 sndPoolResetAll, 8015FCA0 sndObjMinVoicePriority */

/* Mostly dead accessors and helpers that sit between functions of other
 * units: the menu record accessors (menu_state_accessors.c), the pad latch
 * (shared_pad.c), the hit plane (engine_hit_detection.c), the action bindings
 * (controls.c), the frame timer (timer_frame.c), the view system
 * (view_system.c), angle wrapping (engine_math_near_pass.c) and the sound object pool
 * (engine_sound_lists.c). Types and names follow those units.
 *
 * Recipe: PsyQ 3.6 cc1 (C), -fvolatile, extern_gp, gp_size 8. */

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;

/* ---- menu record accessors (menu_state_accessors.c) ---- */

/* @801374A4: set byte 48 of a menu record (read by codeDetectorGetLastCode). */
void __attribute__((section(".text_801374A4"))) codeDetectorSetLastCode(u8 *rec, u8 v)
{
    rec[48] = v;
}

/* Six 24-byte entries at +148 of a menu record; three words each. */
typedef struct {
    s32 a, b, c;
    u8 pad[12];
} TM2MenuEntry;

typedef struct {
    u8 pad[148];
    TM2MenuEntry entry[6];
} TM2MenuRecord;

/* @8013762C: read the three words of entry (index mod 6). */
void __attribute__((section(".text_8013762C"))) sub_8013762C(TM2MenuRecord *rec, s32 index, s32 *a, s32 *b, s32 *c)
{
    index %= 6;
    *a = rec->entry[index].a;
    *b = rec->entry[index].b;
    *c = rec->entry[index].c;
}

/* ---- C++ runtime ---- */

/* @80145EC0 __pure_virtual: the slot of every abstract method in the C++
 * vtables (800CF490...); calling one traps with a break. */
__asm__(
    ".section .text_80145EC0,\"ax\",@progbits\n"
    ".align 2\n"
    ".globl sub_80145EC0\n"
    ".type sub_80145EC0,@function\n"
    ".ent sub_80145EC0\n"
    "sub_80145EC0:\n"
    ".set\tnoreorder\n"
    ".set noreorder\n"
    "\tbreak 0\n"   /* 80145EC0: 0000000d */
    ".set\treorder\n"
    ".set noreorder\n"
    ".end sub_80145EC0\n"
    ".size sub_80145EC0,.-sub_80145EC0\n");

/* ---- object pool (engine_clut_anim.c) ---- */

extern s32 D_801810D8;

/* @801495B4: set the pool word between the shared object csSharedObject and
 * the allocation count csPoolCount (nothing reads it). */
void __attribute__((section(".text_801495B4"))) sub_801495B4(s32 v)
{
    D_801810D8 = v;
}

/* ---- pad latch (shared_pad.c) ---- */

union TM2PadWord { u32 word; u16 half[2]; };
extern union TM2PadWord padButtonsLatched;

/* BIOS pad buffer 1: [4..7] analog axes. */
extern u8 padRecvBufferPort1[34];

/* @8014A4F0: the latched buttons of a port (2: both). */
u32 __attribute__((section(".text_8014A4F0"))) padGetLatched(short port)
{
    if (port == 1)
        return padButtonsLatched.word & 0xFFFF0000;
    else if (port == 0)
        return padButtonsLatched.half[0];
    else
        return padButtonsLatched.word;
}

/* @8014A530: add buttons to a port's latch (2: replace the whole word). */
void __attribute__((section(".text_8014A530"))) padOrLatched(short port, u32 buttons)
{
    if (port == 1)
        padButtonsLatched.word = buttons | (padButtonsLatched.word & 0xFFFF0000);
    else if (port == 0)
        padButtonsLatched.word = buttons | padButtonsLatched.half[0];
    else
        padButtonsLatched.word = buttons;
}

/* @8014AA24: pad 1 analog axis 15 or 16, centred on 0. (Load, then
 * subtract: the one-expression form puts the byte in another register.) */
s8 __attribute__((section(".text_8014AA24"))) padAnalogAxis(short axis)
{
    s8 v = 0;
    if (axis == 15) {
        v = padRecvBufferPort1[4];
        v -= 128;
    }
    if (axis == 16) {
        v = padRecvBufferPort1[5];
        v -= 128;
    }
    return v;
}

/* ---- hit plane (engine_hit_detection.c) ---- */

typedef struct {
    s16 x, y, z, pad;
    s32 d;
    u8 b0c, b0d;
    u16 h0e;
} HdpPlane;

extern HdpPlane hdpPlane;

/* @8014E7D4: set the global hit plane (normal, distance, responses). */
void __attribute__((section(".text_8014E7D4"))) hdpSetPlane(s16 *normal, s32 d, u8 b0d, u16 h0e)
{
    hdpPlane.x = normal[0];
    hdpPlane.y = normal[1];
    hdpPlane.z = normal[2];
    hdpPlane.d = d;
    hdpPlane.b0c = 1;
    hdpPlane.b0d = b0d;
    hdpPlane.h0e = h0e;
}

/* ---- action bindings (controls.c) ---- */

typedef struct { u8 button[128]; } TM2Bindings;

extern TM2Bindings padActiveBindings[2];     /* active bindings, per port */
extern TM2Bindings padConfiguredBindings[2];     /* configured bindings, per port */
extern u8 padActionCache[128][2];         /* last lookup result, [action][port] */
extern int padRepeatDelay;                /* first repeat delay (tics) */
extern int padRepeatInterval;                /* repeat interval (tics) */

/* @80150DD4: store an action's cached lookup result. */
void __attribute__((section(".text_80150DD4"))) padSetActionCache(u8 action, u8 held, short port)
{
    padActionCache[action][port] = held;
}

/* @80150E84: bind an action to a button in the active bindings. */
void __attribute__((section(".text_80150E84"))) padBindActiveAction(u8 action, u8 button, short port)
{
    (padActiveBindings + port)->button[action] = button;
}

/* @80150F98: button bound to an action (configured bindings). */
u8 __attribute__((section(".text_80150F98"))) padGetConfiguredBinding(u8 action, short port)
{
    return (padConfiguredBindings + port)->button[action];
}

/* @80151120 */
void __attribute__((section(".text_80151120"))) padSetRepeatDelay(int tics)
{
    padRepeatDelay = tics;
}

/* @8015112C */
void __attribute__((section(".text_8015112C"))) padSetRepeatInterval(int tics)
{
    padRepeatInterval = tics;
}

/* ---- engine_anim_hit_volumes.c globals ---- */

extern s32 D_80181160;
extern s32 renderFarDistance, renderFarDistanceDefault;
extern s32 renderNodeCullEnable;

/* @80151550 */
void __attribute__((section(".text_80151550"))) sub_80151550(s32 v)
{
    D_80181160 = v;
}

/* @8015165C: set both renderFarDistance and renderFarDistanceDefault. */
void __attribute__((section(".text_8015165C"))) renderSetFarDistance(s32 v)
{
    renderFarDistanceDefault = v;
    renderFarDistance = v;
}

/* @801516C0 */
void __attribute__((section(".text_801516C0"))) renderEnableNodeCulling(void)
{
    renderNodeCullEnable = 1;
}

/* ---- bit-count tables ---- */

extern u8 mathSignBitTable[256];      /* redundant sign bits of a byte (0: 7) */
extern u8 mathLeadingZeroTable[256];      /* leading zero bits (0: 8) */
extern u8 mathLeadingOneTable[256];      /* leading one bits */

/* @80153E7C: build the three 256-entry leading-bit tables. The leading-zero
 * loop is `while (m) { if (m & 0x80) break; ... }`: the `m && !(m & 0x80)`
 * condition gets its exit test duplicated at the bottom (2 words more). */
void __attribute__((section(".text_80153E7C"))) mathInitBitTables(void)
{
    s32 i, m, sign;

    for (i = 0; i < 256; i++) {
        sign = i & 0x80;
        if (i == 0) {
            mathSignBitTable[i] = 7;
        } else {
            mathSignBitTable[i] = 0;
            for (m = i << 1; m && (m & 0x80) == sign; m <<= 1)
                mathSignBitTable[i]++;
        }
    }
    for (i = 0; i < 256; i++) {
        mathLeadingZeroTable[i] = 0;
        mathLeadingOneTable[i] = mathLeadingZeroTable[i];
        m = i;
        if (i == 0) {
            mathLeadingZeroTable[i] = 8;
        } else {
            while (m) {
                if (m & 0x80)
                    break;
                mathLeadingZeroTable[i]++;
                m <<= 1;
            }
        }
        for (m = i; m && (m & 0x80); m <<= 1)
            mathLeadingOneTable[i]++;
    }
}

/* ---- frame timer (timer_frame.c) ---- */

extern short timerFrameTicks;
extern unsigned int timerLastFrameDelta;
#pragma tm2_absolute timerFrameTicks
#pragma tm2_absolute timerLastFrameDelta

/* @8015530C: set the frame ticks. */
void __attribute__((section(".text_8015530C"))) timerSetFrameTicks(short ticks)
{
    timerFrameTicks = ticks;
}

/* @8015533C: the last frame delta scaled by 236/1000. */
unsigned int __attribute__((section(".text_8015533C"))) timerGetFrameUsec(void)
{
    return timerLastFrameDelta * 236 / 1000;
}

/* ---- view system (view_system.c) ---- */

typedef struct {
    s32 f0;                 /* +0x00 -> viewFov */
    s32 f4;
    s32 scale;              /* +0x08 */
    s32 fC;                 /* +0x0C */
    s32 disp_w;             /* +0x10 */
    s32 disp_h;             /* +0x14 */
    u8 pad_18[0x48];
} TM2ViewMode;

typedef struct TM2ViewObj {
    s32 f0;
    u8 pad_04[0x0C];
    struct TM2ViewObj *parent;      /* +0x10 */
    u8 pad_14[0x6C];
    u8 rotm[0x20];                  /* +0x80 MATRIX */
    u8 rot[8];                      /* +0xA0 SVECTOR */
    s32 pos[3];                     /* +0xA8 */
} TM2ViewObj;

extern TM2ViewMode viewModeTable[];
#include "semantic/players.h"
extern TM2ViewObj *viewCameraObjects[TM2_MAX_VIEWS];
extern s32 viewModes[];
/* viewModes[0] (view 0's mode), which one accessor reads through $gp. */
extern s32 a80181120;
extern s32 viewFov;

extern void viewLoadMode(int mode, int i);

/* @8015566C */
s32 __attribute__((section(".text_8015566C"))) viewGetFovH(int i)
{
    return viewModeTable[viewModes[i]].f0;
}

/* @801556A4 (a second copy) */
s32 __attribute__((section(".text_801556A4"))) viewGetFovH2(int i)
{
    return viewModeTable[viewModes[i]].f0;
}

/* @801556DC */
s32 __attribute__((section(".text_801556DC"))) viewGetFovV(int i)
{
    return viewModeTable[viewModes[i]].f4;
}

/* @80155714: view 0's display size. */
void __attribute__((section(".text_80155714"))) viewGetDisplaySize(s32 *w, s32 *h)
{
    *w = viewModeTable[a80181120].disp_w;
    *h = viewModeTable[a80181120].disp_h;
}

/* @80155DB8: switch every active view to mode (x - 40) / 5. */
void __attribute__((section(".text_80155DB8"))) viewSetFov(s32 x)
{
    int i;
    int mode;

    viewFov = x;
    mode = (x - 40) / 5;
    for (i = 0; i < TM2_MAX_VIEWS; i++) {
        if (viewCameraObjects[i]) {
            viewLoadMode(mode, i);
            viewModes[i] = mode;
        }
    }
    viewFov = viewModeTable[mode].f0;
}

/* @8015644C: view i's rotation (SVECTOR). */
void * __attribute__((section(".text_8015644C"))) viewGetRotationPtr(int i)
{
    return viewCameraObjects[i]->rot;
}

/* @80156468: view i's rotation matrix. */
void * __attribute__((section(".text_80156468"))) viewGetRotMatrix(int i)
{
    return viewCameraObjects[i]->rotm;
}

/* @801564BC: view i's position. */
s32 * __attribute__((section(".text_801564BC"))) viewGetPositionPtr(int i)
{
    return viewCameraObjects[i]->pos;
}

/* @80156578: the object view i follows. */
TM2ViewObj * __attribute__((section(".text_80156578"))) viewGetParent(int i)
{
    return viewCameraObjects[i]->parent;
}

/* ---- angle wrapping (engine_math_near_pass.c) ---- */

/* @8015697C BoundSVector: wrap three halfword angles into [-2048, 2048). */
void __attribute__((section(".text_8015697C"))) mathBoundSVector(s16 *v)
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

/* ---- sound object pool (engine_sound_lists.c) ---- */

typedef struct Voice {
    u8 pad00[2];
    u8 pri;                /* 02 */
    u8 pad03[0x2D];
    struct Voice *next;    /* 30 */
} Voice;

typedef struct SndObj {
    u8 pad00[0x68];
    s32 f68;
    Voice *voices;         /* 6C */
    s32 index;             /* 70 */
    s32 inuse;             /* 74 */
} SndObj;

extern s32 sndPoolFreeCount, sndPoolSearchEnd, sndPoolSearchStart;
extern SndObj *sndObjPool[];

/* @8015F134: reset the sound object pool, also dropping each object's
 * voice list (sndPoolReset clears only the in-use flags). */
void __attribute__((section(".text_8015F134"))) sndPoolResetAll(void)
{
    s32 i;
    sndPoolSearchEnd = TM2_MAX_SOUND_REQUESTS - 1;     /* 35 (players.h; request 33-4) */
    sndPoolSearchStart = 0;
    sndPoolFreeCount = TM2_MAX_SOUND_REQUESTS;
    for (i = 0; i < sndPoolFreeCount; i++) {
        sndObjPool[i]->voices = 0;
        sndObjPool[i]->f68 = 0;
        sndObjPool[i]->inuse = 0;
    }
}

/* @8015FCA0: a sound object method (vtable 800D1234): the lowest voice
 * priority byte, 99 when it has no voices. Retail's loop is not rotated
 * (test at the top, `j` back), so it is a goto loop. */
s32 __attribute__((section(".text_8015FCA0"))) sndObjMinVoicePriority(SndObj *o)
{
    s32 min = 99;
    Voice *v = o->voices;
    s32 pri;
loop:
    if (v) {
        pri = v->pri;
        if (pri < min)
            min = pri;
        v = v->next;
        goto loop;
    }
    return min;
}
