typedef unsigned int u32;typedef unsigned short u16;
/* Reinterpret the digits of value in input_base as digits in output_base. */
u32 __attribute__((section(".text_8012CF94"))) passwordRebase(u32 value,u16 output_base,u16 input_base)
{
 u32 result=0,place=1;
 while(value>=input_base){result+=(value%input_base)*place;value/=input_base;place*=output_base;}
 return result+(value%input_base)*place;
}
