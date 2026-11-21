union TM2HalfWord { int word; short half[2]; };
union TM2IconRecord { unsigned int words[2]; unsigned char bytes[8]; };
typedef char half_word_size[(sizeof(union TM2HalfWord) == 4) ? 1 : -1];
typedef char icon_record_size[(sizeof(union TM2IconRecord) == 8) ? 1 : -1];
/* @80180E84 / @80180E74 / @80180E2C / @80180E0C */
int sndListenerDist __attribute__((section(".tm2_80180E84"))) = 0;
int sndListenerPan __attribute__((section(".tm2_80180E74"))) = 0;
int audioMasterVolume __attribute__((section(".tm2_80180E2C"))) = 0;
union TM2HalfWord audioCdVolume __attribute__((section(".tm2_80180E0C"))) = {0};
/* @80180F48: raw 8014E008 writes bytes 0..3 and halfwords 4/6. */
union TM2IconRecord explodeArmorIconRect __attribute__((section(".tm2_80180F48"))) = {{0, 0}};
/* @80183F00: five contiguous state words used by 800FBB6C/800FBBC4. */
int hudMessageCurrent[5] __attribute__((section(".tm2_80183F00"))) = {0};
/* @80180E8C / @80180E90 / @80180E94 / @80180E98 */
int cdAudioPlayCount __attribute__((section(".tm2_80180E8C"))) = 0;
int cdAudioSector __attribute__((section(".tm2_80180E90"))) = 0;
int cdAudioEndSector __attribute__((section(".tm2_80180E94"))) = 0;
int cdAudioStartSector __attribute__((section(".tm2_80180E98"))) = 0;
