/* Controller input (8014A30C..8014AA24).
 *
 * The BIOS pad buffers (InitPAD2) are decoded once per frame into
 * padButtonsDecoded: player 1 in the low half, player 2 in the high half. Bits are
 * active-high: 0 L2, 1 R2, 2 L1, 3 R1, 4 triangle, 5 circle, 6 cross,
 * 7 square, 8 select, 9 L3, 10 R3, 11 start, 12 up, 13 right, 14 down,
 * 15 left. padButtonsLatched is the latched copy the game reads; port 0 is the
 * low half, port 1 the high half, port 2 either. */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

union TM2PadWord { u32 word; u16 half[2]; };
extern union TM2PadWord padButtonsLatched, padButtonsDecoded;

/* BIOS pad buffers: [0] status, [1] type << 4 | length, [2..3] buttons
 * (active-low), [4..7] analog axes. */
extern u8 padRecvBufferPort1[34];
extern u8 padRecvBufferPort2[34];

union TM2PadBytes { u32 word; u8 byte[4]; };

#define PAD_DIGITAL 4

/* The two switch jump tables sit back to back at their retail address. */
#pragma tm2_rdata .tm2_800D0E80

/* @8014A30C */
void __attribute__((section(".text_8014A30C"))) padClear(void)
{
    padButtonsLatched.word = 0;
    padButtonsDecoded.word = 0;
}

/* @8014A31C: decode both BIOS buffers into padButtonsDecoded. A missing pad 1
 * clears the word; the pad 2 half then still uses pad 1's raw low byte. */
void __attribute__((section(".text_8014A31C"))) padDecode(void)
{
    union TM2PadBytes pad[2];
    pad[0].word = padRecvBufferPort1[3];
    pad[1].word = padRecvBufferPort2[3];
    if ((padRecvBufferPort1[1] >> 4) != PAD_DIGITAL)
        padButtonsDecoded.word = 0;
    else {
        pad[0].byte[1] = padRecvBufferPort1[2];
        pad[0].word ^= 0xFFFF;
        padButtonsDecoded.word = pad[0].word;
    }
    if ((padRecvBufferPort2[1] >> 4) == PAD_DIGITAL) {
        pad[1].byte[1] = padRecvBufferPort2[2];
        pad[1].word ^= 0xFFFF;
        padButtonsDecoded.word = pad[0].word | (pad[1].word << 16);
    }
}

/* @8014A3CC: add one port's (or both ports') current buttons to the latch. */
void __attribute__((section(".text_8014A3CC"))) padLatchPort(short port)
{
    if (port == 1)
        padButtonsLatched.word |= padButtonsDecoded.word & 0xFFFF0000;
    else if (port == 0)
        padButtonsLatched.word |= padButtonsDecoded.half[0];
    else
        padButtonsLatched.word = padButtonsDecoded.word;
}

/* @8014A42C */
void __attribute__((section(".text_8014A42C"))) padLatchAll(void)
{
    padButtonsLatched.word = padButtonsDecoded.word;
}

/* @8014A440: a second copy of the decoder. */
void __attribute__((section(".text_8014A440"))) padDecodeCopy(void)
{
    union TM2PadBytes pad[2];
    pad[0].word = padRecvBufferPort1[3];
    pad[1].word = padRecvBufferPort2[3];
    if ((padRecvBufferPort1[1] >> 4) != PAD_DIGITAL)
        padButtonsDecoded.word = 0;
    else {
        pad[0].byte[1] = padRecvBufferPort1[2];
        pad[0].word ^= 0xFFFF;
        padButtonsDecoded.word = pad[0].word;
    }
    if ((padRecvBufferPort2[1] >> 4) == PAD_DIGITAL) {
        pad[1].byte[1] = padRecvBufferPort2[2];
        pad[1].word ^= 0xFFFF;
        padButtonsDecoded.word = pad[0].word | (pad[1].word << 16);
    }
}

/* @8014A57C: is one button held? Buttons 1..14 are select, start, L1, L2,
 * up, down, left, right, R1, R2, triangle, cross, square, circle. */
u8 __attribute__((section(".text_8014A57C"))) padButtonHeld(short button, short port)
{
    u8 held = 0;
    u32 pad;
    if (port == 2) {
        if (padButtonHeld(button, 1) || padButtonHeld(button, 0))
            held = 1;
    } else {
        pad = padButtonsLatched.word;
        if (port == 1)
            pad >>= 16;
        switch (button) {
        case 2: held = (pad >> 11) & 1; break;
        case 1: held = (pad >> 8) & 1; break;
        case 3: held = (pad >> 2) & 1; break;
        case 4: held = pad & 1; break;
        case 5: held = (pad >> 12) & 1; break;
        case 6: held = (pad >> 14) & 1; break;
        case 7: held = (pad >> 15) & 1; break;
        case 8: held = (pad >> 13) & 1; break;
        case 9: held = (pad >> 3) & 1; break;
        case 10: held = (pad >> 1) & 1; break;
        case 11: held = (pad >> 4) & 1; break;
        case 12: held = (pad >> 6) & 1; break;
        case 13: held = (pad >> 7) & 1; break;
        case 14: held = (pad >> 5) & 1; break;
        }
    }
    return held;
}

/* @8014A6A8: is exactly one of nine button combinations held? Port 0 tests
 * the whole latch word, so a second pad's buttons make it fail. */
int __attribute__((section(".text_8014A6A8"))) padComboHeld(short combo, short port)
{
    int match = 0;
    u32 pad;
    if (port == 0)
        pad = padButtonsLatched.word;
    else
        pad = padButtonsLatched.half[1];
    switch (combo) {
    case 1: if (pad == 0x180B) match = 1; break;
    case 2: if (pad == 0x30C9) match = 1; break;
    case 3: if (pad == 0xC036) match = 1; break;
    case 4: if (pad == 0xC035) match = 1; break;
    case 5: if (pad == 0x30CA) match = 1; break;
    case 6: if (pad == 0x3000) match = 1; break;
    case 7: if (pad == 0x00C0) match = 1; break;
    case 8: if (pad == 0x30C0) match = 1; break;
    case 9: if (pad == 0xC030) match = 1; break;
    }
    return match;
}

/* @8014A750: any button latched on a port. */
int __attribute__((section(".text_8014A750"))) padAnyLatched(int port)
{
    u32 pad;
    if (port == 0)
        pad = padButtonsLatched.half[0];
    else if (port == 1)
        pad = padButtonsLatched.half[1];
    else
        pad = padButtonsLatched.word;
    return pad != 0;
}

/* @8014A784: any button currently down on a port. */
u8 __attribute__((section(".text_8014A784"))) padAnyDown(int port)
{
    u32 pad;
    if (port == 0)
        pad = padButtonsDecoded.half[0];
    else if (port == 1)
        pad = padButtonsDecoded.half[1];
    else
        pad = padButtonsDecoded.word;
    return pad != 0;
}

#define ANY_DIRECTION(name, va, bit)                                         \
    u8 __attribute__((section(".text_" va))) name(int port)                  \
    {                                                                        \
        u32 held;                                                            \
        u32 pad;                                                             \
        if (port == 2) {                                                     \
            held = 0;                                                        \
            if (name(1) || name(0))                                          \
                held = 1;                                                    \
        } else {                                                             \
            pad = padButtonsLatched.word;                                           \
            if (port == 1)                                                   \
                pad >>= 16;                                                  \
            held = pad >> (bit);                                             \
            held &= 1;                                                       \
        }                                                                    \
        return held;                                                         \
    }

/* @8014A7B8 .. @8014A92C */
ANY_DIRECTION(ctlpadAnyUp, "8014A7B8", 12)
ANY_DIRECTION(ctlpadAnyDown, "8014A834", 14)
ANY_DIRECTION(ctlpadAnyLeft, "8014A8B0", 15)
ANY_DIRECTION(ctlpadAnyRight, "8014A92C", 13)

/* @8014A9A8: cross, select or start (0x940) on a port. */
u8 __attribute__((section(".text_8014A9A8"))) ctlpadAnySelect(int port)
{
    u32 held;
    u32 pad;
    if (port == 2) {
        held = 0;
        if (ctlpadAnySelect(1) || ctlpadAnySelect(0))
            held = 1;
    } else {
        pad = padButtonsLatched.word;
        if (port == 1)
            pad >>= 16;
        held = (pad & 0x940) != 0;
    }
    /* The 0x41 mask is in retail (andi v0,s0,0x41). */
    return held & 0x41;
}
