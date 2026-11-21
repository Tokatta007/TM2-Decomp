struct TM2TimerState {
    short field_0, field_2;
    unsigned int field_4, field_8, field_C, field_10;
};
typedef char timer_state_size[(sizeof(struct TM2TimerState) == 20) ? 1 : -1];
/* @8017EBF8: two halfword values followed by four state words. */
struct TM2TimerState timerFrameTicks __attribute__((section(".tm2_8017EBF8"))) = {2, 20, 0, 0, 0, 0};
/* @801FA3B8: interrupt accumulator, updated by the timer IRQ callback (SysClkIntHandler). */
unsigned int timerTicsAccum __attribute__((section(".tm2_801FA3B8"))) = 0;
/* @801A5180 / @801A5184 */
int timerRootCounterEvent __attribute__((section(".tm2_801A5180"))) = 0;
unsigned int timerBaseRate __attribute__((section(".tm2_801A5184"))) = 0;
/* @80180BC8 */
int soundManagerObject __attribute__((section(".tm2_80180BC8"))) = 0;
/* @80180FA0: uaInit and the other direct callers use indices zero and one. The port variant's humans 2..3
 * use the 2 bytes of padding after it; with more human records (TM2_PORT_HUMANS 5..16, HUMANS16_PLAN.md H4)
 * it is port_variant_arrays.c's [TM2_MAX_HUMANS] (.port_bss) and this definition goes. */
#include "semantic/players.h"
#if !defined(TM2_PORT_VARIANT) || TM2_MAX_HUMANS <= TM2_VARIANT_HUMANS
unsigned char carResetWeapons[2] __attribute__((section(".tm2_80180FA0"))) = {0, 0};
#endif
