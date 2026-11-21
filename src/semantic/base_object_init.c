typedef unsigned int u32;typedef unsigned short u16;typedef unsigned char u8;
extern void *sceneObjBaseInit(void*,u16);extern void sceneObjDraw(void *obj, int context);
u32 sceneObjVtbl[4] __attribute__((section(".tm2_800CF4C4")))={0,0,0,(u32)sceneObjDraw};
void * __attribute__((section(".text_80123E98"))) sub_80123E98(u8 *object,u16 first,u16 second)
{
 sceneObjBaseInit(object,first);*(u32*)(object+48)=(u32)sceneObjVtbl;*(u16*)(object+8)=second;*(u16*)(object+10)=1;*(short*)(object+12)=-1;*(u32*)(object+16)=1;*(u32*)(object+20)=0;*(u32*)(object+32)=0;*(u32*)(object+28)=*(u32*)(object+32);*(u32*)(object+24)=*(u32*)(object+28);*(u32*)(object+44)=0;*(u32*)(object+40)=*(u32*)(object+44);*(u32*)(object+36)=*(u32*)(object+40);return object;
}
