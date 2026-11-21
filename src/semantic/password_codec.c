typedef signed int s32;typedef unsigned int u32;typedef unsigned short u16;typedef unsigned char u8;
u32 __attribute__((section(".text_8012CE04"))) passwordEncode(s32 first,s32 second,u8 flag,s32 arg /* unused; the retail caller passes 0 */)
{
 s32 product=((u16)second+1)*((u16)first-1);
 return ((product%13<<10)+(flag<<9)+16*(second+4)+2*(first-1))&0xfffe;
}
s32 __attribute__((section(".text_8012CE7C"))) passwordDecode(code,first,second,flag,extra)
u16 code; u16 *first; u16 *second; u8 *flag; u8 *extra;
{
 if(code&1)return 0;
 *first=(((u16)code>>1)&7)+1;
 if((u32)(*first-2)>=7)return 0;
 *second=(((u16)code>>4)&15)-4;
 if(*second>=12)return 0;
 *flag=((u16)code>>9)&1;
 *extra=((u16)code>>8)&1;
 if(*flag==1&&*first<8)return 0;
 if(*extra==0)return ((code>>10)&15)==(*second+1)*(*first-1)%13;
 return 0;
}
