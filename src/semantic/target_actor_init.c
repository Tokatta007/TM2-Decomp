typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;

extern s32 gameGetLevel(void);
extern s32 rand(void);

/* @80163DB8: start tables per game mode. Rows hold a position (x, y, z) and,
 * for the special actor kinds, a velocity (vx, vy, vz). */
struct TM2StartRow { s32 x, y, z; };
struct TM2StartRowVel { s32 x, y, z, vx, vy, vz; };
struct TM2StartTables {
    struct TM2StartRowVel mode3_special[4];
    struct TM2StartRow mode3[4];
    struct TM2StartRowVel mode1_special[3];
    struct TM2StartRow mode1[5];
    struct TM2StartRowVel mode6_special[2];
    struct TM2StartRow mode6[5];            /* four used */
    struct TM2StartRow mode5_single[2];     /* one used */
    struct TM2StartRow mode5[5];
    struct TM2StartRowVel mode8_special[5];
    struct TM2StartRow mode8[10];
    struct TM2StartRow mode11[8];
};
typedef char tm2_start_tables_size[(sizeof(struct TM2StartTables) == 201 * 4) ? 1 : -1];
extern struct TM2StartTables targetStartPositions;

#define S16(p, o) (*(s16 *)((u8 *)(p) + (o)))
#define S32(p, o) (*(s32 *)((u8 *)(p) + (o)))
#define OBJECT (*(u8 **)(record + 56))
#define SPECIAL (S16(record, 4) == 0x202 || S16(record, 4) == 0x205)

/* Place the object at the chosen row, jittered by up to 80 units in x/y. */
#define PLACE_RANDOM(rows) \
    S32(OBJECT, 168) = (rows)[index].x + rand() / 409; \
    S32(OBJECT, 172) = (rows)[index].y + rand() / 409; \
    S32(OBJECT, 176) = (rows)[index].z
#define PLACE(rows) \
    S32(OBJECT, 168) = (rows)[index].x; \
    S32(OBJECT, 172) = (rows)[index].y; \
    S32(OBJECT, 176) = (rows)[index].z
#define FACE_FORWARD \
    S16(OBJECT, 160) = 0; \
    S16(OBJECT, 162) = 0; \
    S16(OBJECT, 164) = 0
#define VELOCITY(rows) \
    S32(record, 12) = (rows)[index].vx; \
    S32(record, 16) = (rows)[index].vy; \
    S32(record, 20) = (rows)[index].vz

/* The mode switch's jump table sits at its retail address. */
#pragma tm2_rdata .tm2_800CF5C8

/* @80125A18: place a target actor at a random start row for the current game
 * mode (modes 7, 9, 10 and unknown modes leave it untouched). */
void __attribute__((section(".text_80125A18"))) targetActorPlace(u8 *record)
{
    s32 index = 0;

    switch (gameGetLevel()) {
    case 3:
        if (SPECIAL) {
            index = rand() % 4;
            PLACE_RANDOM(targetStartPositions.mode3_special);
            FACE_FORWARD;
            VELOCITY(targetStartPositions.mode3_special);
        } else {
            index = rand() % 4;
            PLACE_RANDOM(targetStartPositions.mode3);
            FACE_FORWARD;
        }
        break;
    case 1:
        if (SPECIAL) {
            index = rand() % 3;
            PLACE(targetStartPositions.mode1_special);
            FACE_FORWARD;
            VELOCITY(targetStartPositions.mode1_special);
        } else {
            index = rand() % 5;
            PLACE_RANDOM(targetStartPositions.mode1);
            FACE_FORWARD;
        }
        break;
    case 6:
        if (SPECIAL) {
            index = rand() & 1;
            PLACE(targetStartPositions.mode6_special);
            FACE_FORWARD;
            VELOCITY(targetStartPositions.mode6_special);
        } else {
            index = rand() % 4;
            PLACE_RANDOM(targetStartPositions.mode6);
            FACE_FORWARD;
        }
        break;
    case 5:
        if (S16(record, 6) == 1) {
            S32(OBJECT, 168) = targetStartPositions.mode5_single[0].x;
            S32(OBJECT, 172) = targetStartPositions.mode5_single[0].y;
            S32(OBJECT, 176) = targetStartPositions.mode5_single[0].z;
            FACE_FORWARD;
        } else {
            index = rand() % 5;
            PLACE_RANDOM(targetStartPositions.mode5);
            FACE_FORWARD;
        }
        break;
    case 8:
        if (SPECIAL) {
            index = rand() % 5;
            PLACE_RANDOM(targetStartPositions.mode8_special);
            FACE_FORWARD;
            VELOCITY(targetStartPositions.mode8_special);
        } else {
            index = rand() % 10;
            PLACE_RANDOM(targetStartPositions.mode8);
            FACE_FORWARD;
        }
        break;
    case 11:
        index = rand() % 8;
        PLACE_RANDOM(targetStartPositions.mode11);
        FACE_FORWARD;
        break;
    case 2:
    case 4:
        break;
    default:
        return;
    }
    record[0] = index;
    record[1] = 1;
    S16(record, 8) = 0;
    record[2] = 0;
}
