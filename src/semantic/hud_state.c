extern int hudMessageCurrent[5];
extern int aiCarsCountAlive(void);

int hudMessageUpdate(void) __attribute__((section(".text_800FBC44")));
/* @800FBC44. State order is shared with 800FBB6C/800FBBC4; the calls may
 * observe the intervening writes. Preserve the path-dependent return. */
int hudMessageUpdate(void)
{
    int result;
    if (aiCarsCountAlive() == 0) {
        hudMessageCurrent[0] = 0;
        hudMessageCurrent[2] = 0;
        hudMessageCurrent[4] = 100;
        hudMessageCurrent[1] = 0;
    }
    result = (unsigned int)hudMessageCurrent[4] - 1;
    hudMessageCurrent[4] = result;
    if (result <= 0) {
        int pending = hudMessageCurrent[1];
        int priority = hudMessageCurrent[3];
        hudMessageCurrent[4] = 100;
        hudMessageCurrent[0] = pending;
        hudMessageCurrent[2] = priority;
        if (!pending) {
            hudMessageCurrent[0] = aiCarsCountAlive();
            hudMessageCurrent[2] = 50;
        }
        result = aiCarsCountAlive();
        hudMessageCurrent[1] = result;
    }
    return result;
}

void UAdashHealthColor(int level, unsigned char *packet) __attribute__((section(".text_800FBD04")));
/* @800FBD04. Two repeated pairs of RGB corners in a 36-byte packet.
 * IDA's 64-bit return is incidental v0/v1 color temporaries, unused by the
 * reviewed direct callers. The observable contract is the twelve byte stores. */
void UAdashHealthColor(int level, unsigned char *packet)
{
    unsigned int ar,ag,ab,br,bg,bb;
    switch (level) {
    case 0: ar=255; ag=0; ab=0; br=255; bg=0; bb=0; break;
    case 1: ar=255; ag=42; ab=0; br=255; bg=115; bb=0; break;
    case 2: ar=186; ag=155; ab=32; br=221; bg=231; bb=50; break;
    default: ar=0; ag=112; ab=10; br=49; bg=148; bb=41; break;
    }
    packet[4]=ar; packet[5]=ag; packet[6]=ab;
    packet[20]=ar; packet[21]=ag; packet[22]=ab;
    packet[12]=br; packet[13]=bg; packet[14]=bb;
    packet[28]=br; packet[29]=bg; packet[30]=bb;
}

int hudHealthLevel(int percent) __attribute__((section(".text_800FBDD4")));
/* @800FBDD4. Signed thresholds; preserve the existing carved helper's behavior. */
int hudHealthLevel(int percent)
{
    if (percent >= 41) return 3;
    if (percent >= 13) return 2;
    return percent > 0;
}
