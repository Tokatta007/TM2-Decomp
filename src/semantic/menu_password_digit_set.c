typedef unsigned short u16;
/* @8012D144: store value in slot index % 6 of the six halfwords at object
 * +288 (callers ignore $v0). */
void __attribute__((section(".text_8012D144"))) menuPasswordDigitSet(u16 *object, u16 index, u16 value)
{
    object[144 + index % 6] = value;
}
