/* @801B0418, 225 records of 28 bytes. Both typed views share the same
 * 6300-byte C object; zero initialization is supplied by the retail CRT. */
union TM2TerrainStorage { unsigned int words[1575]; unsigned short halves[3150]; };
typedef char terrain_storage_size[(sizeof(union TM2TerrainStorage) == 6300) ? 1 : -1];
union TM2TerrainStorage worldSurfaces __attribute__((section(".tm2_801B0418"))) = {{0}};
