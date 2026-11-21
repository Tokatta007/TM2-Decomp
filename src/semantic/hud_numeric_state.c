struct TM2HudNumberTemplate { unsigned char bytes[4]; };
/* @80180A74..80180A84: two initialized counters and two initialized phases. */
unsigned int hudMarkerBlinkCounters[2] __attribute__((section(".tm2_80180A74"))) = {0,0};
unsigned int hudMarkerBlinkPhases[2] __attribute__((section(".tm2_80180A7C"))) = {0,0};
/* @80180AC8..80180ACC: real mutable template, not a folded literal. */
struct TM2HudNumberTemplate hudNumberBlankTemplate __attribute__((section(".tm2_80180AC8"))) = {{32,32,32,0}};
