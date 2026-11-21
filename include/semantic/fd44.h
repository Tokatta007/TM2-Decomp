#ifndef TM2_SEMANTIC_FD44_H
#define TM2_SEMANTIC_FD44_H
typedef unsigned char FD44_u8;
typedef unsigned short FD44_u16;
typedef unsigned int FD44_u32;
struct FD44State {
    short field_0;
    FD44_u16 field_2;
    short field_4;
    FD44_u16 field_6;
    int field_8;
    int *field_C;
};
typedef char fd44_state_size[(sizeof(struct FD44State) == 16) ? 1 : -1];
extern short aiDarkToothShotDelay[7];
extern struct FD44State darkToothSwoopDelay;
extern FD44_u8 *darkToothBossObject;
extern int darkToothSwoopVectors[6];
int aiDarkToothBossUpdate(FD44_u8 *arg0);
#endif
