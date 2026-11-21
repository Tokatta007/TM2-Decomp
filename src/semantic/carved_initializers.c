/* Nonzero initial values used by the carved C globals. Values are verified
 * against retail data and the post-CRT RAM capture, not inferred defaults. */
int musicVolume __attribute__((section(".tm2_init_801809F8"))) = 104;
int hdpDifficulty __attribute__((section(".tm2_init_80180D60"))) = 3;
/* Scratchpad address: a hardware-space value, not a program-address alias. */
int hdpStackScratch __attribute__((section(".tm2_init_80180D6C"))) = 0x1F800000;
int hdpShift __attribute__((section(".tm2_init_80180D70"))) = 3;
int renderWorldShift __attribute__((section(".tm2_init_80180DB4"))) = 3;
int renderPrevFrameParity __attribute__((section(".tm2_init_80180DBC"))) = 1;
int viewFov __attribute__((section(".tm2_init_80180DE4"))) = 1;
