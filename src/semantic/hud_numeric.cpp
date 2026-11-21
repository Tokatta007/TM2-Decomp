/* HUD blinking markers and the three-digit number print. C++ (PsyQ 3.6
 * CC1PLPSX, -fvolatile); extern "C" keeps the retail symbol names. Each player
 * has a 1900-byte HUD packet record; the coloured parts are libgpu-style
 * primitives (tag word, then r0 g0 b0 code). */
#define setRGB0(p, r, g, b) ((p)->r0 = (r), (p)->g0 = (g), (p)->b0 = (b))

struct TM2HudPrim {            /* 20 bytes */
    unsigned int tag;
    unsigned char r0, g0, b0, code;
    char rest[12];
};

struct TM2HudDigitPair {
    TM2HudPrim high, low;
};

struct TM2HudPlayer {          /* 1900 bytes */
    char head[0x100];
    TM2HudPrim marker[32];                         /* +0x100 */
    TM2HudDigitPair digits[(1900 - 0x380) / 40];   /* +0x380 */
    char tail[1900 - 0x380 - ((1900 - 0x380) / 40) * 40];
};
typedef char tm2_hud_player_size[(sizeof(TM2HudPlayer) == 1900) ? 1 : -1];

struct TM2HudNumberText {
    unsigned char bytes[4];
};

extern "C" {
#include "semantic/players.h"
extern TM2HudPlayer hudPlayerRecords[TM2_MAX_HUDS];
extern int hudMarkerBlinkCounters[TM2_MAX_HUDS];             /* blink counter */
extern unsigned int hudMarkerBlinkPhases[TM2_MAX_HUDS];    /* blink phase, bit 0 toggles */
extern TM2HudNumberText hudNumberBlankTemplate;   /* "   " template */
#include "semantic/hud.h"
}

/* @800FB588: advance the blink counter (the test uses the old value) and colour
 * one marker by the phase. */
extern "C" void __attribute__((section(".text_800FB588"))) hudBlinkWeaponMarker(int player, int slot)
{
    if (hudMarkerBlinkCounters[player]++ >= 4) {
        hudMarkerBlinkPhases[player] ^= 1;
        hudMarkerBlinkCounters[player] = 0;
    }
    if (hudMarkerBlinkPhases[player]) setRGB0(&hudPlayerRecords[player].marker[slot], 0, 140, 10);
    else setRGB0(&hudPlayerRecords[player].marker[slot], 0, 0, 0);
}

/* @800FB684: colour both digits of a pair by the current phase (no advance). */
extern "C" void __attribute__((section(".text_800FB684"))) hudBlinkWeaponDigits(int player, int slot)
{
    if (hudMarkerBlinkPhases[player]) {
        setRGB0(&hudPlayerRecords[player].digits[slot].high, 0, 140, 10);
        setRGB0(&hudPlayerRecords[player].digits[slot].low, 0, 140, 10);
    } else {
        setRGB0(&hudPlayerRecords[player].digits[slot].high, 0, 0, 0);
        setRGB0(&hudPlayerRecords[player].digits[slot].low, 0, 0, 0);
    }
}

/* @800FB768: print a value as three space-prefilled digits (signed remainder,
 * larger numbers truncated, no sign or clamp). Argument five is unused; six is
 * the draw-mode packet. */
extern "C" void __attribute__((section(".text_800FB768"))) hudPrintNumber3(int value, int x, int y,
    void *packets, void *unused, void *mode, unsigned int *ot)
{
    TM2HudNumberText text = hudNumberBlankTemplate;
    int i = 2;
    do {
        int digit = value % 10;
        value /= 10;
        if (i == 2 || digit != 0 || value != 0) text.bytes[i] = digit + '0';
    } while (--i != -1);
    fontSetColor(4, 36, 150, 36);
    fontSpritePrintXY(4, x, y, mode, packets, text.bytes, ot);
    fontResetColor(4);
}
