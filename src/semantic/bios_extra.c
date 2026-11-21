/* Genuine BIOS interfaces. These preserve the caller frame and forward the
 * BIOS result; no game algorithm or compiler-shaped instruction sequence is
 * replaced here. Section placement keeps each syscall's real exception PC. */
#ifndef __mips__
#error These interfaces require the PSX BIOS.
#endif

extern int sub_8013CF7C(const char *text);
extern int sub_80145E40(void);
extern int EnterCriticalSection(void);
extern void ExitCriticalSection(void);

#define BIOS_TABLE_ENTRY(name,va,table,number) \
    __asm__(".section .text_" #va ",\"ax\",@progbits\n.align 2\n" \
            ".globl " #name "\n.type " #name ",@function\n" \
            #name ":\n.set noreorder\n" \
            "addiu $10,$0," #table "\njr $10\naddiu $9,$0," #number "\n" \
            ".set reorder\n.size " #name ",.-" #name "\n")
#define BIOS_SYSCALL_ENTRY(name,va,selector) \
    __asm__(".section .text_" #va ",\"ax\",@progbits\n.align 2\n" \
            ".globl " #name "\n.type " #name ",@function\n" \
            #name ":\n.set noreorder\n" \
            "addiu $4,$0," #selector "\nsyscall\njr $31\nnop\n" \
            ".set reorder\n.size " #name ",.-" #name "\n")

/* @8013CF7C: raw callers supply their diagnostic string in a0. */
BIOS_TABLE_ENTRY(sub_8013CF7C,8013CF7C,0xB0,0x3F);
/* @80145E40: no incoming argument consumed by the reviewed caller. */
BIOS_TABLE_ENTRY(sub_80145E40,80145E40,0xA0,0x72);
/* @80145E80 / @80145E90: actual kernel traps, not simulated interrupt flags. */
BIOS_SYSCALL_ENTRY(EnterCriticalSection,80145E80,1);
BIOS_SYSCALL_ENTRY(ExitCriticalSection,80145E90,2);

#undef BIOS_TABLE_ENTRY
#undef BIOS_SYSCALL_ENTRY
