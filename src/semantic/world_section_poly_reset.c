/* @800F220C: reset one 28-byte terrain record (indices 0..224) in the storage that
 * terrain_load.c owns as a word/halfword union; this unit sees it as a record array. */
struct TM2TerrainRecord { unsigned short h0, h2, h4, h6; unsigned int w8; unsigned short h12, h14, h16, h18; unsigned int w20, w24; };
extern struct TM2TerrainRecord worldSurfaces[225];
typedef char terrain_record_size[(sizeof(struct TM2TerrainRecord) * 225 == 6300) ? 1 : -1];

unsigned int __attribute__((section(".text_800F220C"))) worldSectionPolyReset(int index)
{
    worldSurfaces[index].w20 = 32000;
    worldSurfaces[index].w24 = 32000;
    worldSurfaces[index].h12 = 32000;
    worldSurfaces[index].h14 = 32000;
    worldSurfaces[index].h16 = 32000;
    worldSurfaces[index].h18 = 32000;
    worldSurfaces[index].h6 = 0;
    worldSurfaces[index].h4 = 0;
    return index * 28;
}
