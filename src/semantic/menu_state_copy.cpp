/* Menu state helpers. C++ (PsyQ 3.6 CC1PLPSX, -fvolatile); extern "C" keeps the
 * retail symbol names used by the C callers. */

/* @80137690: six decimal digits (least significant first, at +288) to a value;
 * the terms are summed from the most significant digit down. */
struct TM2DigitBlock {
    char fields[288];
    unsigned short digit[6];
};

extern "C" int __attribute__((section(".text_80137690"))) menuKeypadGetValue(TM2DigitBlock *block)
{
    return block->digit[5] * 100000 + block->digit[4] * 10000 + block->digit[3] * 1000
         + block->digit[2] * 100 + block->digit[1] * 10 + block->digit[0];
}

/* @80137954: copy a 42-byte menu selection record. The arrays copy only the
 * entries in use (`count` or `count2`, already copied into dest), so stale tail
 * bytes in dest are kept. */
struct TM2MenuSelection {
    int word0;
    unsigned char count, count2, byte6, byte7;
    unsigned char list8[2];
    unsigned char list10[10];
    unsigned char list20[4];
    int word24;
    unsigned char byte28, byte29, byte30;
    int word32;
    unsigned char byte36, byte37;
    unsigned char list38[2];
    unsigned char byte40, byte41;
};

/* Port variant with more than 4 human records (TM2_PORT_HUMANS 5..16, HUMANS16_PLAN.md 1.6 / H4): retail's loops
 * run to `count` (humans) / `count2` (opponents) past the end of each list. Inside the record that only copies
 * the same offsets again, but list38 (+0x26) with 7 or more humans writes past the 0x2C bytes sessionSettings owns
 * (into screenCardDrawEnvs, 80164788) and past the attract demo's 48-byte copy on its stack. There each loop also
 * stops at its list's own length: the record holds human_car[2], lives[4] and control_layout[2] (the humans past them
 * keep theirs in sessionPortHumanCars, portHumanLives and portHumanLayout, which this copy does not touch) and
 * opponent_car[10] (slots 9.. in sessionPortOpponentCars). Retail and up to 4 humans: retail's loops. */
#include "semantic/players.h"
#if defined(TM2_PORT_VARIANT) && TM2_MAX_HUMANS > TM2_VARIANT_HUMANS
#define TM2_COPY_ROOM(i, list) && (unsigned int)(i) < sizeof(list)
#else
#define TM2_COPY_ROOM(i, list)
#endif

extern "C" TM2MenuSelection * __attribute__((section(".text_80137954")))
settingsCopy(TM2MenuSelection *dest, const TM2MenuSelection *source)
{
    int i;

    dest->word0 = source->word0;
    dest->count = source->count;
    dest->count2 = source->count2;
    dest->byte6 = source->byte6;
    dest->byte7 = source->byte7;
    for (i = 0; i < dest->count TM2_COPY_ROOM(i, dest->list8); i++) dest->list8[i] = source->list8[i];
    for (i = 0; i < dest->count2 TM2_COPY_ROOM(i, dest->list10); i++) dest->list10[i] = source->list10[i];
    for (i = 0; i < dest->count TM2_COPY_ROOM(i, dest->list20); i++) dest->list20[i] = source->list20[i];
    dest->word24 = source->word24;
    dest->byte28 = source->byte28;
    dest->byte29 = source->byte29;
    dest->byte30 = source->byte30;
    dest->word32 = source->word32;
    dest->byte36 = source->byte36;
    dest->byte37 = source->byte37;
    for (i = 0; i < dest->count TM2_COPY_ROOM(i, dest->list38); i++) dest->list38[i] = source->list38[i];
    dest->byte40 = source->byte40;
    dest->byte41 = source->byte41;
    return dest;
}
