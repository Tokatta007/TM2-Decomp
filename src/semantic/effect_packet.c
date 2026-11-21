union TM2UvRecord { int words[2]; unsigned short halves[4]; unsigned char bytes[8]; };
struct TM2PacketSource {
    unsigned char field_0;
    unsigned char pad_1[11];
    unsigned char *field_C;
    unsigned int field_10;
};
typedef char packet_source_size[(sizeof(struct TM2PacketSource) == 20) ? 1 : -1];
typedef char uv_record_size[(sizeof(union TM2UvRecord) == 8) ? 1 : -1];

/* @8014DD74. Keep the stores and subsequent reads sequenced for aliases. */
void __attribute__((section(".text_8014DD74"))) grutilsUvBounds(unsigned char *points, unsigned short *x,
                 unsigned short *y, unsigned short *width, unsigned short *height)
{
    int max_x, max_y, i;
    *x = points[0];
    *y = points[1];
    max_x = points[0];
    max_y = points[1];
    for (i = 1; i < 4; ++i) {
        *x = *x < points[i * 4] ? *x : points[i * 4];
        *y = *y < points[i * 4 + 1] ? *y : points[i * 4 + 1];
        max_x = max_x < points[i * 4] ? points[i * 4] : max_x;
        max_y = max_y < points[i * 4 + 1] ? points[i * 4 + 1] : max_y;
    }
    *width = (unsigned short)(max_x - *x);
    *height = (unsigned short)(max_y - *y);
}

/* @8014E008. Input packets and output records must have their retail alignment
 * and enough storage for the selected count. The null/status gates precede
 * all descriptor-field reads. The point pointer is formed inside the call's
 * argument list (from a packet+16 temporary) so it is computed after the
 * stack-passed &bounds[3], as in retail. */
int __attribute__((section(".text_8014E008"))) grutilsParseSpriteRects(void *arg0, unsigned char *output, unsigned int requested)
{
    struct TM2PacketSource *source = (struct TM2PacketSource *)arg0;
    unsigned char *packet;
    unsigned char *record = output;
    unsigned int count = requested;
    int i;
    unsigned short bounds[4];
    if (!source || source->field_0 != 0) return 0;
    packet = source->field_C;
    count = count < source->field_10 ? count : source->field_10;
    for (i = 0; i < (int)count; ++i) {
        unsigned char *data = packet + 16;
        unsigned char *points;
        grutilsUvBounds(points = data + ((packet[15] >> 2) & 0x1C),
                     &bounds[0], &bounds[1], &bounds[2], &bounds[3]);
        record[0] = bounds[0];
        record[1] = bounds[1];
        record[2] = bounds[2];
        record[3] = bounds[3];
        *(unsigned short *)(record + 6) = *(unsigned short *)(points + 2);
        *(unsigned short *)(record + 4) = *(unsigned short *)(points + 6);
        packet += packet[2] * 4;
        record += 8;
    }
    return (int)count;
}
