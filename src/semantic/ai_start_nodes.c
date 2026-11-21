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
extern union TM2ShortList startNodeList;
extern int rand(void);
#include "semantic/port_cars.h"

/* @800F636C. Valid count is 1..15; BIOS rand supplies 0..32767. */
int __attribute__((section(".text_800F636C"))) startNodePickRandom(int count)
{
    unsigned char found = 0;
    int result = -1;
    int i, start;
#ifdef TM2_PORT_CARS
    if (portCarOptions & TM2_PORT_CAR_OPT_FAR_STARTS)   /* request 32 step 2 (CAR_GROUPS.md) */
        return portStartNodePick(count);
#endif
    start = rand() % count;
    for (i = start; i < count && !found; ++i) {
        if (startNodeList.view.field_1E[i] == 0) {
            result = startNodeList.view.field_0[i];
            startNodeList.view.field_1E[i] = 1;
            found = 1;
        }
    }
    for (i = 0; i < start && !found; ++i) {
        if (startNodeList.view.field_1E[i] == 0) {
            result = startNodeList.view.field_0[i];
            startNodeList.view.field_1E[i] = 1;
            found = 1;
        }
    }
    if (result < 0) {
        start = rand() % count;
        result = startNodeList.view.field_0[start];
    }
    return result;
}

/* @801019DC: clear the fifteen bytes only, preserving IDs and count. */
void __attribute__((section(".text_801019DC"))) startNodesClearUsed(void)
{
    int i;
    for (i = 0; i < 15; ++i) startNodeList.view.field_1E[i] = 0;
#ifdef TM2_PORT_CARS
    portStartPlacedClear();     /* request 49 (CAR_GROUPS.md 7.4): no car placed this round yet */
#endif
}

/* @800F37A4 */
short *__attribute__((section(".text_800F37A4"))) startNodesGet(void) { return startNodeList.view.field_0; }

/* @80101A04. Retail copies the following halfword even for the last active
 * element. At count 15 that is the first two flag bytes; the union backing
 * view preserves this access without indexing beyond the ID array. */
void __attribute__((section(".text_80101A04"))) startNodesRemove(int value)
{
    int i, j;
    for (i = 0; i < startNodeList.view.field_2D; ++i) {
        if ((short)startNodeList.half[i] == value) {
            for (j = i; j < startNodeList.view.field_2D; ++j)
                startNodeList.half[j] = startNodeList.half[j + 1];
            startNodeList.view.field_2D -= 1;
        }
    }
}

/* @80101AB4 */
void __attribute__((section(".text_80101AB4"))) startNodesFwy(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 10;
    startNodeList.view.field_0[0] = 6;
    startNodeList.view.field_0[1] = 10;
    startNodeList.view.field_0[2] = 15;
    startNodeList.view.field_0[3] = 21;
    startNodeList.view.field_0[4] = 24;
    startNodeList.view.field_0[5] = 28;
    startNodeList.view.field_0[6] = 39;
    startNodeList.view.field_0[7] = 50;
    startNodeList.view.field_0[8] = 55;
    startNodeList.view.field_0[9] = 64;
}

/* @80101C38 */
void __attribute__((section(".text_80101C38"))) startNodesDish(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 10;
    startNodeList.view.field_0[0] = 5;
    startNodeList.view.field_0[1] = 6;
    startNodeList.view.field_0[2] = 7;
    startNodeList.view.field_0[3] = 12;
    startNodeList.view.field_0[4] = 17;
    startNodeList.view.field_0[5] = 19;
    startNodeList.view.field_0[6] = 21;
    startNodeList.view.field_0[7] = 23;
    startNodeList.view.field_0[8] = 25;
    startNodeList.view.field_0[9] = 27;
}

/* @80101DBC */
void __attribute__((section(".text_80101DBC"))) startNodesIce(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 15;
    startNodeList.view.field_0[0] = 65;
    startNodeList.view.field_0[1] = 63;
    startNodeList.view.field_0[2] = 61;
    startNodeList.view.field_0[3] = 59;
    startNodeList.view.field_0[4] = 57;
    startNodeList.view.field_0[5] = 54;
    startNodeList.view.field_0[6] = 52;
    startNodeList.view.field_0[7] = 51;
    startNodeList.view.field_0[8] = 49;
    startNodeList.view.field_0[9] = 40;
    startNodeList.view.field_0[10] = 36;
    startNodeList.view.field_0[11] = 31;
    startNodeList.view.field_0[12] = 19;
    startNodeList.view.field_0[13] = 14;
    startNodeList.view.field_0[14] = 8;
}

/* @80101F7C */
void __attribute__((section(".text_80101F7C"))) startNodesLava(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 11;
    startNodeList.view.field_0[0] = 2;
    startNodeList.view.field_0[1] = 4;
    startNodeList.view.field_0[2] = 6;
    startNodeList.view.field_0[3] = 9;
    startNodeList.view.field_0[4] = 11;
    startNodeList.view.field_0[5] = 14;
    startNodeList.view.field_0[6] = 37;
    startNodeList.view.field_0[7] = 44;
    startNodeList.view.field_0[8] = 55;
    startNodeList.view.field_0[9] = 57;
    startNodeList.view.field_0[10] = 82;
}

/* @8010210C */
void __attribute__((section(".text_8010210C"))) startNodesParis(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 11;
    startNodeList.view.field_0[1] = 13;
    startNodeList.view.field_0[2] = 15;
    startNodeList.view.field_0[3] = 31;
    startNodeList.view.field_0[4] = 36;
    startNodeList.view.field_0[5] = 44;
    startNodeList.view.field_0[6] = 58;
    startNodeList.view.field_0[7] = 62;
    startNodeList.view.field_0[8] = 72;
    startNodeList.view.field_0[9] = 91;
    startNodeList.view.field_0[0] = 0;
    startNodeList.view.field_0[10] = 96;
}

/* @80102298 */
void __attribute__((section(".text_80102298"))) startNodesHkong(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 14;
    startNodeList.view.field_0[0] = 2;
    startNodeList.view.field_0[1] = 4;
    startNodeList.view.field_0[2] = 9;
    startNodeList.view.field_0[3] = 11;
    startNodeList.view.field_0[4] = 14;
    startNodeList.view.field_0[5] = 20;
    startNodeList.view.field_0[6] = 22;
    startNodeList.view.field_0[7] = 28;
    startNodeList.view.field_0[8] = 36;
    startNodeList.view.field_0[9] = 49;
    startNodeList.view.field_0[10] = 53;
    startNodeList.view.field_0[11] = 13;
    startNodeList.view.field_0[12] = 23;
    startNodeList.view.field_0[13] = 48;
}

/* @8010244C */
void __attribute__((section(".text_8010244C"))) startNodesRoof(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 10;
    startNodeList.view.field_0[0] = 1;
    startNodeList.view.field_0[1] = 17;
    startNodeList.view.field_0[2] = 27;
    startNodeList.view.field_0[3] = 37;
    startNodeList.view.field_0[4] = 45;
    startNodeList.view.field_0[5] = 55;
    startNodeList.view.field_0[6] = 63;
    startNodeList.view.field_0[7] = 67;
    startNodeList.view.field_0[8] = 72;
    startNodeList.view.field_0[9] = 78;
}

/* @801025D0 */
void __attribute__((section(".text_801025D0"))) startNodesDen(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 15;
    startNodeList.view.field_0[1] = 2;
    startNodeList.view.field_0[2] = 13;
    startNodeList.view.field_0[3] = 15;
    startNodeList.view.field_0[4] = 17;
    startNodeList.view.field_0[5] = 19;
    startNodeList.view.field_0[6] = 21;
    startNodeList.view.field_0[7] = 25;
    startNodeList.view.field_0[8] = 27;
    startNodeList.view.field_0[9] = 30;
    startNodeList.view.field_0[10] = 34;
    startNodeList.view.field_0[11] = 43;
    startNodeList.view.field_0[12] = 45;
    startNodeList.view.field_0[13] = 46;
    startNodeList.view.field_0[0] = 0;
    startNodeList.view.field_0[14] = 47;
}

/* @8010278C */
void __attribute__((section(".text_8010278C"))) startNodesSwamp(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 8;
    startNodeList.view.field_0[1] = 1;
    startNodeList.view.field_0[2] = 2;
    startNodeList.view.field_0[3] = 3;
    startNodeList.view.field_0[4] = 4;
    startNodeList.view.field_0[5] = 5;
    startNodeList.view.field_0[6] = 6;
    startNodeList.view.field_0[0] = 0;
    startNodeList.view.field_0[7] = 7;
}

/* @801028F4 */
void __attribute__((section(".text_801028F4"))) startNodesCyb(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 10;
    startNodeList.view.field_0[1] = 1;
    startNodeList.view.field_0[2] = 2;
    startNodeList.view.field_0[3] = 3;
    startNodeList.view.field_0[4] = 4;
    startNodeList.view.field_0[5] = 5;
    startNodeList.view.field_0[6] = 6;
    startNodeList.view.field_0[7] = 7;
    startNodeList.view.field_0[8] = 8;
    startNodeList.view.field_0[0] = 0;
    startNodeList.view.field_0[9] = 9;
}

/* @80102A74 */
void __attribute__((section(".text_80102A74"))) startNodesSroof(void)
{
    startNodesClearUsed();
    startNodeList.view.field_2D = 9;
    startNodeList.view.field_0[1] = 1;
    startNodeList.view.field_0[2] = 2;
    startNodeList.view.field_0[3] = 3;
    startNodeList.view.field_0[4] = 4;
    startNodeList.view.field_0[5] = 5;
    startNodeList.view.field_0[6] = 6;
    startNodeList.view.field_0[7] = 7;
    startNodeList.view.field_0[0] = 0;
    startNodeList.view.field_0[8] = 8;
}
