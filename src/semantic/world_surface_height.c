typedef unsigned int u32;
struct D800EE950_Record {
    unsigned char field_0, field_1;
    short field_2, field_4, field_6;
    unsigned char pad_8[4];
    short field_C, field_E, field_10, field_12;
};
typedef char record_size[(sizeof(struct D800EE950_Record) == 20) ? 1 : -1];
typedef char record_C[((unsigned int)&((struct D800EE950_Record *)0)->field_C == 12) ? 1 : -1];
typedef char record_12[((unsigned int)&((struct D800EE950_Record *)0)->field_12 == 18) ? 1 : -1];

#define TM2_SCALED(difference, coordinate, divisor) ((difference) * (coordinate) / (divisor))

/* @800EE950. Record/position interpolation; field roles inferred from raw
 * arithmetic (32-bit wraparound as in retail). */
int __attribute__((section(".text_800EE950"))) worldSurfaceHeightAt(void *record, int *position)
{
    struct D800EE950_Record *r = (struct D800EE950_Record *)record;
    int a, b, difference;
    if (r->field_6 == 0 || r->field_4 == 0)
        return r->field_C;
    if (r->field_1 != 0) {
        a = r->field_C + TM2_SCALED(r->field_E - r->field_C, position[0], r->field_4);
        b = r->field_10 + TM2_SCALED(r->field_12 - r->field_10, position[0], r->field_4);
        difference = b - a;
        return a + TM2_SCALED(difference, position[1], r->field_6);
    }
    difference = r->field_10 - r->field_C;
    if (difference != 0)
        return r->field_C + TM2_SCALED(difference, position[1], r->field_6);
    difference = r->field_E - r->field_C;
    return r->field_C + TM2_SCALED(difference, position[0], r->field_4);
}
