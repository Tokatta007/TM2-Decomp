/* Car choice records: one 264-byte record per id, built from the active
 * definition (carFreezeChoiceTemplate) with its left and right option lists. */
typedef unsigned char u8;typedef signed char s8;typedef signed int s32;

typedef struct TM2ChoiceNode {
    u8 type;            /* 1 = group, 3 = option list */
    u8 pad1[10];
    u8 count;           /* +11 */
    u8 pad12[12];
    struct TM2ChoiceNode *child;   /* +24 */
    u8 pad28[4];
    void *items[1];     /* +32 */
} TM2ChoiceNode;

typedef struct TM2ChoiceSide {
    u8 pad0[5];
    s8 last;            /* +5: count - 1 */
    u8 pad6[13];
    u8 side;            /* +19: 1 left, 0 right */
    u8 pad20[8];
    struct TM2Choice *owner;       /* +28 */
    void *items[21];    /* +32 */
} TM2ChoiceSide;

typedef struct TM2Choice {
    u8 pad0[20];
    TM2ChoiceSide *left;           /* +20 */
    void *x18;                     /* +24 */
    TM2ChoiceSide *right;          /* +28 */
    TM2ChoiceSide left_side;       /* +32 */
    TM2ChoiceSide right_side;      /* +148 */
} TM2Choice;
typedef char tm2_choice_size[(sizeof(TM2Choice) == 264) ? 1 : -1];

typedef struct TM2ChoiceDefinition {
    u8 pad0[20];
    TM2ChoiceNode *left;           /* +20 */
    void *x18;                     /* +24 */
    TM2ChoiceNode *right;          /* +28 */
} TM2ChoiceDefinition;

#include "semantic/car_groups.h"
#ifdef TM2_PORT_CAR_GROUPS
/* request 32 (CAR_GROUPS.md): one record per car key, in portCarGroups.freeze */
#define carFreezeObjects ((TM2Choice *)portCarGroups.freeze)
#else
extern TM2Choice carFreezeObjects[16];
#endif
extern TM2ChoiceDefinition *carFreezeChoiceTemplate;
extern void mathCopyWords(void *, void *, s32);

/* Copy one option list into the record's side when the definition holds a
 * group (directly or one level down) whose child is an option list. */
#define BUILD_SIDE(source, field, slot, value) \
    root = (source); \
    if (root->type == 1) { \
        node = root->child; \
        if (node->type == 3) \
            node = root; \
        if (node->type == 1) { \
            child = node->child; \
            if (child->type == 3) { \
                mathCopyWords(&carFreezeObjects[index].field, child, 9); \
                side = &carFreezeObjects[index].field; \
                side->side = (value); \
                side->last = child->count - 1; \
                side->owner = &carFreezeObjects[index]; \
                for (i = 0; i < child->count; i++) \
                    side->items[i] = child->items[i]; \
                carFreezeObjects[index].slot = side; \
            } \
        } \
    }

/* @80124C40 */
void __attribute__((section(".text_80124C40"))) carFreezeBuild(s32 id, void **output)
{
    s32 index = id >> 4, i;
    TM2ChoiceDefinition *definition;
    TM2ChoiceNode *root, *node, *child;
    TM2ChoiceSide *side;
#ifdef TM2_PORT_CAR_GROUPS
    if (portCarLoadKey())
        index = portCarLoadKey() >> 4;
    if (!output || (unsigned int)index >= TM2_CAR_KEYS)
        return;
#else
    if (!output || index >= 16)
        return;
#endif
    if (!carFreezeChoiceTemplate)
        return;
    definition = carFreezeChoiceTemplate;
    mathCopyWords(&carFreezeObjects[index], definition, 5);
    output[8] = &carFreezeObjects[index];
    BUILD_SIDE(definition->left, left_side, left, 1)
    carFreezeObjects[index].x18 = definition->x18;
    BUILD_SIDE(definition->right, right_side, right, 0)
}
