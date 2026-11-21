typedef signed int s32;
typedef unsigned int u32;

u32 objRegTargetObjects[50] __attribute__((section(".tm2_8018F784"))) = {0};

/* @80124EDC: retrieve an object record by its signed table index. */
void *__attribute__((section(".text_80124EDC"))) targetGetLevelObject(s32 index)
{
    return (void *)objRegTargetObjects[index];
}
