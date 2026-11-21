/* SPU voices: 24 (the PS1, retail) or 48 (the PC port's SPU; request 34, docs/SPU_VOICES.md).
 *
 * --port-define TM2_PORT_SPU48 (port variant only) gives the sound libraries 48 voices. Voices 24..47 have a
 * second register block at 0x1F802C00 (the PS1's expansion region 2), laid out as the SPU's first 0x200 bytes:
 * voice 24+n's 16 bytes at +0x10*n, key on +0x188/+0x18A, key off +0x18C/+0x18E, reverb on +0x198/+0x19A,
 * ENDX +0x19C/+0x19E (bit n = voice 24+n). Nothing else is there (main volume, reverb settings, SPUCNT,
 * transfers, sound RAM, pitch modulation and noise are the SPU's, voices 0..23 only).
 *
 * libspu's voice masks stay 32-bit words: bits 0..23 are voices 0..23, as retail; a mask with
 * TM2_SPU_EXT_MASK (bit 31) set means its bits 0..23 are voices 24..47 (TM2_SPU_VOICE_BIT). One call
 * addresses voices of one block.
 *
 * Without the define every macro gives retail's value (24 voices, 1 << voice), so retail builds and the
 * variant without it compile as before. Plain C types only (included by SDK units with their own typedefs). */
#ifndef TM2_SEMANTIC_SPU_VOICES_H
#define TM2_SEMANTIC_SPU_VOICES_H

#define TM2_SPU_BLOCK_VOICES 24             /* voices per register block (the PS1 SPU's 24) */

/* TM2_SPU_VOICES is a limit of the port variant (tools/build_source_only.py PORT_LIMIT_MACROS `spuvoices`):
 * the placement check forces it to retail's 24 with -D, as the other limits. */
#ifdef TM2_PORT_SPU48
#ifndef TM2_PORT_VARIANT
#error "TM2_PORT_SPU48 is a port variant define (--port-define TM2_PORT_SPU48)"
#endif
#ifndef TM2_SPU_VOICES
#define TM2_SPU_VOICES 48
#endif
#endif
#ifndef TM2_SPU_VOICES
#define TM2_SPU_VOICES 24
#endif
#if TM2_SPU_VOICES != 24 && TM2_SPU_VOICES != 48
#error "TM2_SPU_VOICES is 24 (retail) or 48 (TM2_PORT_SPU48)"
#endif
#if TM2_SPU_VOICES != 24 && !defined(TM2_PORT_SPU48)
#error "48 voices need TM2_PORT_SPU48 (the build then links libspu/libsnd from the C: config/US/sdk_c_modules.tsv)"
#endif

#if TM2_SPU_VOICES > 24
/* Voices the game's sound driver leaves alone (the highest ones: 44..47 by default), for the port's own menu
 * sounds (docs/SPU_VOICES.md 5). --port-define TM2_PORT_SOUND_VOICES_RESERVED=0 gives the driver all 48. */
#ifndef TM2_PORT_SOUND_VOICES_RESERVED
#define TM2_PORT_SOUND_VOICES_RESERVED 4
#endif
#if TM2_PORT_SOUND_VOICES_RESERVED < 0 || TM2_PORT_SOUND_VOICES_RESERVED > 24
#error "TM2_PORT_SOUND_VOICES_RESERVED is 0..24"
#endif
#else
#undef TM2_PORT_SOUND_VOICES_RESERVED
#define TM2_PORT_SOUND_VOICES_RESERVED 0
#endif

/* The voices the game's sound driver hands out (soundVoiceLimit): 0 .. TM2_SOUND_VOICES - 1. */
#define TM2_SOUND_VOICES (TM2_SPU_VOICES - TM2_PORT_SOUND_VOICES_RESERVED)

#if TM2_SPU_VOICES > TM2_SPU_BLOCK_VOICES
#define TM2_SPU_EXT_REGS 0x1F802C00         /* voices 24..47: their register block */
#define TM2_SPU_EXT_MASK 0x80000000         /* in a libspu voice mask: bits 0..23 are voices 24..47 */
/* the libspu mask of voice v (0..47) */
#define TM2_SPU_VOICE_BIT(v) ((v) < TM2_SPU_BLOCK_VOICES ? 1 << (v) \
                                                         : (int)(TM2_SPU_EXT_MASK | 1 << ((v) - TM2_SPU_BLOCK_VOICES)))

/* portSpuExt (.port_bss, config/US/port_variant_arrays.tsv): the libraries' bit words for voices 24..47
 * (bit n = voice 24+n), beside retail's for 0..23. */
struct TM2PortSpuExt {
    unsigned short okon[2];                 /* +0x00 libsnd _svm_okon1/2: key on at the next flush (+0x188/+0x18A) */
    unsigned short okof[2];                 /* +0x04 libsnd _svm_okof1/2: key off (+0x18C/+0x18E) */
    unsigned short orev[2];                 /* +0x08 libsnd _svm_orev1/2: reverb on (+0x198/+0x19A) */
    unsigned short pad0C[2];
    unsigned int keystat;                   /* +0x10 libspu _spu_keystat */
    unsigned int envx_hist[16];             /* +0x14 libsnd _svm_envx_hist: envelope 0 at that tick */
};
typedef char tm2_port_spu_ext_size[(sizeof(struct TM2PortSpuExt) == 0x54) ? 1 : -1];
#pragma tm2_absolute portSpuExt
extern struct TM2PortSpuExt portSpuExt;
#else
#define TM2_SPU_VOICE_BIT(v) (1 << (v))
#endif

/* Request 87 (SPU_VOICES.md 11): whose sound each voice plays, for the port's own samples (a mod car's engine.wav and
 * special.wav, read at the key-on). portVoiceCar[v] = the car slot + 1 the voice's request sounds for (0 none),
 * portVoiceKind[v] = TM2_VOICE_ENGINE, TM2_VOICE_SPECIAL or 0; set before the voice's key-on is flushed: by the game's
 * driver (soundOn, portVoiceOwner) from its request, to 0 by libsnd's own allocation (SpuVmAlloc). A play of a car's
 * special sets portSoundTag around itself (TM2_VOICE_TAG); sndPoolAlloc copies it into portSndReqTag[the request's
 * index]. Every variant defines the storage (.port_bss); written only with TM2_PORT_SPU48; the sound side (snapshot
 * and hash no). Without the define the macros are empty. */
#define TM2_VOICE_ENGINE 1
#define TM2_VOICE_SPECIAL 2
#define TM2_VOICE_REQS 64                   /* request pool indices tagged (TM2_MAX_SOUND_REQUESTS: at most 64) */
#if TM2_SPU_VOICES > 24
#pragma tm2_absolute portVoiceCar
#pragma tm2_absolute portVoiceKind
#pragma tm2_absolute portSndReqTag
#pragma tm2_absolute portSoundTag
extern unsigned char portVoiceCar[48];
extern unsigned char portVoiceKind[48];
extern unsigned short portSndReqTag[TM2_VOICE_REQS];  /* per request: kind << 8 | car slot + 1, 0 = none */
extern int portSoundTag;                    /* the play under way: the same, 0 = none */
extern void portVoiceOwner(int voice, void *request);
#define TM2_VOICE_TAG(slot, kind) (portSoundTag = (kind) << 8 | (((slot) + 1) & 0xFF))
#define TM2_VOICE_UNTAG() (portSoundTag = 0)
#else
#define TM2_VOICE_TAG(slot, kind) ((void)0)
#define TM2_VOICE_UNTAG() ((void)0)
#endif

#endif
