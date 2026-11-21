extern short SsVabOpen(unsigned char *addr, unsigned char *vab_header);
extern short SsVabTransCompleted(short);

/* @8017F278: the VAB header buffer (512 bytes) filled by SsVabOpen, and
 * @8017F478: the opened VAB id. Both are zero-initialized retail BSS
 * resolved by address; the id is unsized here because retail addresses it
 * absolutely rather than through $gp. */
extern unsigned char sndVabHeaderBuf[];
extern int sndVabIds[];

/* @801283F8: initial attempt plus at most twenty retries. */
void __attribute__((section(".text_801283F8"))) soundVabOpenRetry(unsigned char *addr)
{
    int remaining = 20;
    short result = SsVabOpen(addr, sndVabHeaderBuf);
    sndVabIds[0] = result;
    while (result == -1 && remaining > 0) {
        result = SsVabOpen(addr, sndVabHeaderBuf);
        sndVabIds[0] = result;
        --remaining;
    }
    SsVabTransCompleted(1);
}
