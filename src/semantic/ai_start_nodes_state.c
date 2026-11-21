union TM2ShortList {
    unsigned short half[23];
    unsigned char bytes[46];
    struct {
        short field_0[15];
        unsigned char field_1E[15];
        signed char field_2D;
    } view;
};
typedef char short_list_size[(sizeof(union TM2ShortList) == 46) ? 1 : -1];
/* @801B2B28: 15 halfwords, 15 flag bytes and one signed count byte. */
union TM2ShortList startNodeList __attribute__((section(".tm2_801B2B28"))) = {{0}};
