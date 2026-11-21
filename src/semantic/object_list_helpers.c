typedef signed int s32;

/*
 * Fixed-capacity lists inside a shell menu/scene object: each list is a
 * count followed by an inline array.  The append helpers return the new
 * entry's index, or -1 when the list is full.
 */
typedef struct ListObject {
    char pad0[60];
    s32 list8_count;            /* 60 */
    s32 list8[8];               /* 64 */
    s32 list16_count;           /* 96 */
    s32 list16[16];             /* 100 */
    s32 list32_count;           /* 164 */
    s32 list32[32];             /* 168 */
    s32 quad_count;             /* 296 */
    s32 quads[128][4];          /* 300 */
    s32 ids_count;              /* 2348 */
    s32 ids[10];                /* 2352 */
    s32 pair_count;             /* 2392 */
    s32 pairs[128][2];          /* 2396 */
} ListObject;

/* Append value to the 8-entry list. */
s32 __attribute__((section(".text_801224B4"))) sceneAddAuxObj(ListObject *o, s32 v)
{
    if (o->list8_count < 8) {
        o->list8[o->list8_count] = v;
        return o->list8_count++;
    }
    return -1;
}

/* Append value to the 16-entry list. */
s32 __attribute__((section(".text_801224F4"))) sceneAddDecor(ListObject *o, s32 v)
{
    if (o->list16_count < 16) {
        o->list16[o->list16_count] = v;
        return o->list16_count++;
    }
    return -1;
}

/* Append value to the 32-entry list. */
s32 __attribute__((section(".text_80122534"))) sceneAddItem(ListObject *o, s32 v)
{
    if (o->list32_count < 32) {
        o->list32[o->list32_count] = v;
        return o->list32_count++;
    }
    return -1;
}

/* Append a 4-word record (a, b, c, d) to the 128-entry quad list. */
s32 __attribute__((section(".text_80122574"))) sceneAddAction(ListObject *o, s32 a, s32 b, s32 c, s32 d)
{
    if (o->quad_count < 128) {
        o->quads[o->quad_count][0] = a;
        o->quads[o->quad_count][1] = b;
        o->quads[o->quad_count][2] = c;
        o->quads[o->quad_count][3] = d;
        return o->quad_count++;
    }
    return -1;
}

/* Append an id to the 10-entry id list. */
s32 __attribute__((section(".text_801225F0"))) sceneStartCommand(ListObject *o, s32 v)
{
    if (o->ids_count < 10) {
        o->ids[o->ids_count] = v;
        return o->ids_count++;
    }
    return -1;
}

s32 sceneFindCommand(ListObject *o, s32 value);

/* Remove an id from the id list (shifting later entries down); 1 if found. */
s32 __attribute__((section(".text_80122630"))) sceneStopCommand(ListObject *o, s32 value)
{
    s32 i;
    s32 index = sceneFindCommand(o, value);

    if (index == -1)
        return 0;
    for (i = index; i < o->ids_count; i++)
        o->ids[i] = o->ids[i + 1];
    o->ids_count--;
    return 1;
}

/* Index of value in the id list, or -1. */
s32 __attribute__((section(".text_801226B4"))) sceneFindCommand(ListObject *o, s32 value)
{
    s32 i;
    s32 pad[1];

    for (i = 0; i < o->ids_count; i++) {
        if (o->ids[i] == value)
            return i;
    }
    return -1;
}

/* Append an (a, b) pair to the 128-entry pair list. */
s32 __attribute__((section(".text_801226FC"))) scenePostEvent(ListObject *o, s32 a, s32 b)
{
    if (o->pair_count < 128) {
        o->pairs[o->pair_count][0] = a;
        o->pairs[o->pair_count][1] = b;
        return o->pair_count++;
    }
    return -1;
}
