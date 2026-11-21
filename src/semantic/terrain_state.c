/* Initialized retail state. Separate storage keeps the consumers' ordinary
 * small-data declarations independent of linker placement sections. */
int terrainSurfaceCount __attribute__((section(".tm2_80180ACC"))) = 0;
int D_80180AD0 __attribute__((section(".tm2_80180AD0"))) = 0;
unsigned char *fileioWorkBuffer __attribute__((section(".tm2_80180D58"))) = 0;
