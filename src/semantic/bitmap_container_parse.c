/* cc: fill */
typedef signed int s32;
typedef unsigned int u32;

extern void exit(s32 status) __attribute__((noreturn));
extern void bitmapSetKeyTable(const u32 *keys);
extern s32 bitmapRegisterTim(const void *tim, s32 flag, s32 index);

/* @80121A2C: parse a TIM container ("TXSP", version 0x43): result word,
 * entry count, the key table, then the entries, each preceded by its size. */
s32 __attribute__((section(".text_80121A2C")))
bitmapParseContainer(const u32 *data, s32 flag)
{
    u32 size;
    s32 result;
    s32 count;
    s32 i;

    if (data == 0)
        return 0;
    if (*data != 0x50535854u)
        exit(-1);
    data++;
    if (*data != 0x43u)
        exit(-1);
    data++;
    result = *data++;
    count = *data++;
    bitmapSetKeyTable(data);
    data += count;
    size = *data;
    for (i = 0; i < count; i++) {
        data++;
        bitmapRegisterTim(data, flag, i);
        data += size >> 2;
        size = *data;
    }
    return result;
}
