typedef signed int s32;
typedef unsigned int u32;

/* Model record tables. Each lookup reads the entry once more than it uses it
 * (a dead read through the record pointer, kept under -fvolatile). */
#include "semantic/players.h"
#ifdef TM2_PORT_VARIANT
extern u32 humanCarObjects[TM2_MAX_HUMANS];    /* port_variant_arrays.c (.port_bss) */
#else
u32 humanCarObjects[2] __attribute__((section(".tm2_80180FEC"))) = {0,0};
#endif
#ifdef TM2_PORT_VARIANT
extern u32 aiCarObjects[TM2_MAX_AI_CARS];    /* port_variant_arrays.c (.port_bss) */
#else
u32 aiCarObjects[9] __attribute__((section(".tm2_80188B38"))) = {0};
#endif
extern s32 aiCarTotal;   /* number of AI model records */

/* @8011CB50: lookup the player model record. */
u32 __attribute__((section(".text_8011CB50"))) humanCarObject(s32 index)
{
    u32 *p = humanCarObjects;
    u32 dead = p[index];
    return p[index];
}

/* @8011CB70: lookup the AI model record; the count check only adds a dead read. */
u32 __attribute__((section(".text_8011CB70"))) aiCarObject(s32 index)
{
    u32 dead;
    if (index < aiCarTotal) {
        u32 *p = aiCarObjects;
        dead = p[index];
    }
    return aiCarObjects[index];
}
