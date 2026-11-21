/* Genuine PSX GTE control-register interfaces. These are not general-ASM
 * reconstructions: C loads/scales the values; ctc2 is the hardware boundary.
 * Consecutive transfers target distinct registers, as in retail. The return
 * and its delay slot separate the final transfer from any caller GTE command. */
#ifndef __mips__
#error These interfaces require the PSX GTE target.
#endif

#define CTC5(a,b,c,d,e,r0,r1,r2,r3,r4) \
    __asm__ volatile ("ctc2 %0,$" #r0 "\n\tctc2 %1,$" #r1 \
                      "\n\tctc2 %2,$" #r2 "\n\tctc2 %3,$" #r3 \
                      "\n\tctc2 %4,$" #r4 : : "r"(a), "r"(b), "r"(c), "r"(d), "r"(e))
#define CTC3(a,b,c,r0,r1,r2) \
    __asm__ volatile ("ctc2 %0,$" #r0 "\n\tctc2 %1,$" #r1 \
                      "\n\tctc2 %2,$" #r2 : : "r"(a), "r"(b), "r"(c))

/* @80140410: five packed words; GTE sign-extends the last word's low half. */
void __attribute__((section(".text_80140410"))) SetRotMatrix(const unsigned int *matrix)
{
    register unsigned int a __asm__("$8")=matrix[0], b __asm__("$9")=matrix[1], c __asm__("$10")=matrix[2],
        d __asm__("$11")=matrix[3], e __asm__("$12")=matrix[4];
    CTC5(a,b,c,d,e,0,1,2,3,4);
}

/* @80140440. */
void __attribute__((section(".text_80140440"))) SetLightMatrix(const unsigned int *matrix)
{
    register unsigned int a __asm__("$8")=matrix[0], b __asm__("$9")=matrix[1], c __asm__("$10")=matrix[2],
        d __asm__("$11")=matrix[3], e __asm__("$12")=matrix[4];
    CTC5(a,b,c,d,e,8,9,10,11,12);
}

/* @80140470. */
void __attribute__((section(".text_80140470"))) SetColorMatrix(const unsigned int *matrix)
{
    register unsigned int a __asm__("$8")=matrix[0], b __asm__("$9")=matrix[1], c __asm__("$10")=matrix[2],
        d __asm__("$11")=matrix[3], e __asm__("$12")=matrix[4];
    CTC5(a,b,c,d,e,16,17,18,19,20);
}

/* @801404A0: translation starts at byte +20, not the packed matrix base. */
void __attribute__((section(".text_801404A0"))) SetTransMatrix(const unsigned int *matrix)
{
    register unsigned int x __asm__("$8")=matrix[5], y __asm__("$9")=matrix[6], z __asm__("$10")=matrix[7];
    CTC3(x,y,z,5,6,7);
}

/* @8014A1A8: modulo-32-bit scaling, without a byte clamp. */
void __attribute__((section(".text_8014A1A8"))) SetBackColor(unsigned int r,unsigned int g,unsigned int b)
{
    CTC3(r<<4,g<<4,b<<4,13,14,15);
}

/* @8014A25C. */
void __attribute__((section(".text_8014A25C"))) SetGeomOffset(unsigned int x,unsigned int y)
{
    __asm__ volatile ("ctc2 %0,$24\n\tctc2 %1,$25" : : "r"(x<<16), "r"(y<<16));
}
