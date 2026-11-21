union TM2HalfWord { int word; short half[2]; };
extern union TM2HalfWord audioCdVolume;
extern int cdAudioPlayCount, cdAudioSector, cdAudioEndSector, cdAudioStartSector;
extern int cdAudioSync(int, int);
extern int CdControlB(unsigned char com, const void *, void *);
extern void (*VSyncCallback(void (*)(void)))(void);
extern void cdAudioVSyncPoll(void);
extern void cdAudioRestart(int);
extern int SsSetSerialAttr(int, int, int);
extern void SsSetSerialVol(char s_num, short voll, short volr);
#pragma tm2_absolute audioCdVolume

/* @8015EBA0. The first incoming argument is unused in retail. */
void __attribute__((section(".text_8015EBA0"))) soundXARepeat(int unused, int arg1, int arg2)
{
    unsigned char mode;
    short volume;
    cdAudioStartSector = arg1;
    cdAudioSector = arg1;
    cdAudioEndSector = arg2;
    cdAudioPlayCount = 0;
    mode = 0xC0;
    cdAudioSync(0, 0);
    CdControlB(0xE, &mode, 0);
    VSyncCallback(cdAudioVSyncPoll);
    cdAudioRestart(6);
    SsSetSerialAttr(0, 0, 1);
    volume = audioCdVolume.half[0];
    SsSetSerialVol(0, volume, volume);
}
