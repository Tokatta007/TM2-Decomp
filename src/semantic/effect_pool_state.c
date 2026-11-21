union TM2UvRecord { int words[2]; unsigned short halves[4]; unsigned char bytes[8]; };
union TM2UvPool4 { union TM2UvRecord records[4]; int words[8]; unsigned short halves[16]; unsigned char bytes[32]; };
typedef char uv_pool_4_size[(sizeof(union TM2UvPool4) == 32) ? 1 : -1];
union TM2UvPool8 { union TM2UvRecord records[8]; int words[16]; unsigned short halves[32]; unsigned char bytes[64]; };
typedef char uv_pool_8_size[(sizeof(union TM2UvPool8) == 64) ? 1 : -1];
union TM2UvPool10 { union TM2UvRecord records[10]; int words[20]; unsigned short halves[40]; unsigned char bytes[80]; };
typedef char uv_pool_10_size[(sizeof(union TM2UvPool10) == 80) ? 1 : -1];
union TM2UvPool16 { union TM2UvRecord records[16]; int words[32]; unsigned short halves[64]; unsigned char bytes[128]; };
typedef char uv_pool_16_size[(sizeof(union TM2UvPool16) == 128) ? 1 : -1];

/* @80186DE4..80187194: eleven adjacent banks; final boundary excluded. */
union TM2UvPool16 fxExplosionFrames __attribute__((section(".tm2_80186DE4"))) = {0};
union TM2UvPool16 fxExplosionFramesB __attribute__((section(".tm2_80186E64"))) = {0};
union TM2UvPool4 fxSparkFrames __attribute__((section(".tm2_80186EE4"))) = {0};
union TM2UvPool8 fxBulletImpactFrames __attribute__((section(".tm2_80186F04"))) = {0};
union TM2UvPool4 D_80186F44 __attribute__((section(".tm2_80186F44"))) = {0};
union TM2UvPool16 fxSmokeFramesA __attribute__((section(".tm2_80186F64"))) = {0};
union TM2UvPool16 fxSmokeFramesB __attribute__((section(".tm2_80186FE4"))) = {0};
union TM2UvPool16 fxSmokeFramesC __attribute__((section(".tm2_80187064"))) = {0};
union TM2UvPool8 fxContrailFrames __attribute__((section(".tm2_801870E4"))) = {0};
union TM2UvPool4 D_80187124 __attribute__((section(".tm2_80187124"))) = {0};
union TM2UvPool10 fxFlameFrames __attribute__((section(".tm2_80187144"))) = {0};
