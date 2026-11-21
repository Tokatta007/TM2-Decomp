union TM2PadWord { unsigned int word; unsigned short half[2]; };
union TM2FlagWord { unsigned int word; unsigned char byte[4]; };
typedef char pad_size[(sizeof(union TM2PadWord) == 4) ? 1 : -1];
typedef char flag_size[(sizeof(union TM2FlagWord) == 4) ? 1 : -1];

/* @80180D34 / @80180D38; halfword labels D36/D3A are interior views. */
union TM2PadWord padButtonsLatched __attribute__((section(".tm2_80180D34"))) = {0};
union TM2PadWord padButtonsDecoded __attribute__((section(".tm2_80180D38"))) = {0};
/* @80180F9C / @80180FA4 */
union TM2FlagWord arenaHollandFlag __attribute__((section(".tm2_80180F9C"))) = {0};
union TM2FlagWord viewWatchingAi __attribute__((section(".tm2_80180FA4"))) = {0};
/* @80180FAC / @8018101C / @80181030 */
int viewWatchedAiCar __attribute__((section(".tm2_80180FAC"))) = 0;
int aiCarTotal __attribute__((section(".tm2_8018101C"))) = 0;
unsigned char arenaShortChaseRange __attribute__((section(".tm2_80181030"))) = 0;
