/* libgpu primitive tag: a 24-bit link address and an 8-bit word count. */
typedef struct { unsigned addr:24; unsigned len:8; unsigned char r0, g0, b0, code; } P_TAG;
#define setaddr(p, _addr) (((P_TAG *)(p))->addr = (unsigned long)(_addr))
#define getaddr(p) (unsigned long)(((P_TAG *)(p))->addr)
#define addPrim(ot, p) setaddr(p, getaddr(ot)), setaddr(ot, p)

/* @80144BA0. Link a primitive in front of the ordering-table entry (libgpu). */
void __attribute__((section(".text_80144BA0"))) AddPrim(void *ot, void *p)
{
    addPrim(ot, p);
}
