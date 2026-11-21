typedef signed int s32;typedef unsigned int u32;typedef unsigned short u16;typedef unsigned char u8;
#include "semantic/input.h"
extern void *sub_80123E98(void*, u16 first, u16 second);extern s32 menuPasswordDigitsDraw(u8 *object, s32 context);
u32 menuPasswordDigitsVtbl[4] __attribute__((section(".tm2_800D02EC")))={0,0,0,(u32)menuPasswordDigitsDraw};
/* Movie skip test (YOUWON etc.): polls the pads, returns 1 if Cross (10)
   or Start (8) was pressed on either pad (padCheckAction(button,2)). */
s32 __attribute__((section(".text_8012D024"))) movieSkipCheck(void)
{
 s32 skip;
 padUpdate();
 skip=0;
 if((u8)padCheckAction(10,2)!=0||(u8)padCheckAction(8,2)!=0)skip=1;
 return skip;
}
/* Movie player object constructor: base init (sub_80123E98), vtable
   menuPasswordDigitsVtbl, 6 stream buffers: pointer table +76[6] -> records at +144
   (24 bytes each: type 4, data offset 184*(5-j), cleared fields) and clears
   the u16 array +288[6]. Returns the object. */
typedef struct TM2MovieBuffer { u8 type; u8 pad[3]; u32 offset; u32 start; u32 end; u16 count; u8 flag0; u8 flag1; u32 link; } TM2MovieBuffer;
typedef struct TM2MovieObject { u8 head[76]; TM2MovieBuffer *slot[6]; u8 pad100[44]; TM2MovieBuffer buffer[6]; u16 state[6]; } TM2MovieObject;
void * __attribute__((section(".text_8012D080"))) menuPasswordDigitsCreate(TM2MovieObject *movie,u16 first,u16 second)
{
 s32 j;s32 offset;
 sub_80123E98(movie,first,second);
 *(u32*)(movie->head+48)=(u32)menuPasswordDigitsVtbl;*(u32*)(movie->head+4)=0;*(u32*)(movie->head+20)=0;movie->head[52]=1;movie->head[72]=6;movie->head[73]=4;
 offset=0;
 for(j=5;j>=0;j--){
  movie->slot[j]=&movie->buffer[j];
  movie->buffer[j].type=4;movie->buffer[j].count=0;movie->buffer[j].flag0=0;movie->buffer[j].flag1=0;movie->buffer[j].link=0;
  movie->buffer[j].offset=offset;offset+=184;
  movie->buffer[j].start=0;movie->buffer[j].end=0;
  movie->state[j]=0;
 }
 return movie;
}
