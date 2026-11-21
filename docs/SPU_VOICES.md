# SPU voices: 48 in the port variant

Answer to the PC port's request 34 ("More than 24 sound voices"). With 16 cars every engine and weapon wants a
voice, and the PS1's 24 run out. The port's SPU has 24 more voices (PSY-Z patch 17, `hw.c`). This change gives
the game's sound libraries and its sound driver 48 voices in the port variant. Tags: **CONFIRMED** = read in
code, counted in the data or proved by the build; **GUESS** = inferred, not run.

## Summary

- **The define:** `--port-define TM2_PORT_SPU48` on a port variant build (`SOURCE_BUILD.md` "Port variant",
  "Port code"). It sets `TM2_SPU_VOICES` to 48 (`include/semantic/spu_voices.h`; 24 otherwise). It is a variant
  limit like the others (`spuvoices`): the placement check forces it back to 24. It needs no other port define
  and works with all of them.
- **Without the define nothing changes.** Every change is under `#if TM2_SPU_VOICES > 24`, or is a macro that
  expands to the retail constant (`24`, `1 << voice`). The retail images keep their hashes, and the default
  variant is unchanged in code (section 8).
- **The libraries:** libspu and libsnd address voices 24..47 in the second register block at 0x1F802C00
  (section 1). libspu's 32-bit voice masks gain a block flag (bit 31). libsnd's voice manager runs 48 voices:
  its per-voice tables move to `.port_bss` with 48 entries, and its bit words for 24..47 are in `portSpuExt`.
  Allocation works as in retail.
- **The game's driver** hands out voices 0..43 (`soundVoiceLimit` = 44). Voices 44..47 are left to the port's own
  menu sounds (section 5). `--port-define TM2_PORT_SOUND_VOICES_RESERVED=0` gives the driver all 48.
- **The SDK comes from C under the define.** `--sdk-from-lib` normally links 23 libspu modules and 2 libsnd
  modules from Sony's `.LIB`. With `TM2_PORT_SPU48` they come from our C units instead, through a new per-module
  override (`config/US/sdk_c_modules.tsv`, section 4). The C of all 25 modules is byte-identical to retail. The
  one libsnd function that is not, `SpuVmSetVol`, was already C in every `--sdk-from-lib` build (section 4.3).
- **Pitch modulation and noise are not used by TM2** (CONFIRMED, section 2). So nothing is lost by the second
  block not having them.
- **Proved statically** for four configurations (section 8). **Not runtime-tested** (section 9).

## 1. The registers

Voices 24..47 have a second register block at **0x1F802C00..0x1F802DFF**, the PS1's expansion region 2. The game
does not use that region. The block is laid out as the SPU's first 0x200 bytes (`hw.c`, PSY-Z
`PSYZ_SPU_EXT_VOICES`):

| offset | what | bits |
|---|---|---|
| +0x000 + 0x10·n | voice 24+n: volume L/R, pitch, start address, ADSR 1/2, current volume (ENVX), repeat address, as each voice at 0x1F801C00 | |
| +0x188 / +0x18A | key on | bit n = voice 24+n (n 0..15 / 16..23) |
| +0x18C / +0x18E | key off | the same |
| +0x198 / +0x19A | reverb on | the same |
| +0x19C / +0x19E | ENDX | the same |

Everything else stays the SPU's, and covers voices 0..23 only: main volume, reverb settings, SPUCNT (with the
noise clock), transfers, sound RAM, pitch modulation (0x1F801D90) and noise (0x1F801D94).

`include/semantic/spu_voices.h` names them:

| macro | value | use |
|---|---|---|
| `TM2_SPU_VOICES` | 24, or 48 with `TM2_PORT_SPU48` | the libraries' voices |
| `TM2_SPU_BLOCK_VOICES` | 24 | voices per block |
| `TM2_SPU_EXT_REGS` | 0x1F802C00 | the second block |
| `TM2_SPU_EXT_MASK` | 0x80000000 | the block flag in a libspu voice mask |
| `TM2_SPU_VOICE_BIT(v)` | `1 << v`; with the define `v < 24 ? 1 << v : 0x80000000 \| 1 << (v - 24)` | the libspu mask of voice v |
| `TM2_PORT_SOUND_VOICES_RESERVED` | 0; 4 with the define (settable) | voices the game's driver leaves to the port |
| `TM2_SOUND_VOICES` | `TM2_SPU_VOICES - TM2_PORT_SOUND_VOICES_RESERVED` | `soundVoiceLimit` |

**libspu masks.** libspu's calls take a 32-bit voice mask (`SpuVoiceAttr.voice`, `SpuGetKeyStatus`,
`SpuSetNoiseVoice`), and 48 bits do not fit. So the mask format is:
- bits 0..23 are voices 0..23, as in retail;
- with bit 31 (`TM2_SPU_EXT_MASK`) set, bits 0..23 are voices 24..47.

One call addresses the voices of one block. The game always names one voice, through `TM2_SPU_VOICE_BIT`.
Retail never sets bit 31, so retail calls mean what they meant.

## 2. Pitch modulation and noise (CONFIRMED: TM2 uses neither)

- **Pitch modulation (FM, 0x1F801D90).** TM2 links no libspu function that sets it: no
  `SpuSetPitchLFOVoice` module is in the image (`config/US/sdk_modules.tsv`). libsnd never writes it. The only
  write is `_spu_init`'s cold-start clear (80146B28).
- **Noise (0x1F801D94).** Three paths could turn noise on:
  - **The game's driver:** `soundOn` calls `SpuSetNoiseVoice(1, ...)` when the instance's `noise` field (+0x6C)
    is 1. Only `soundInstAlloc` writes that field, and it writes 0. No other game code writes +0x6C (the object
    update's view of the instance has it as padding).
  - **libsnd, a noise tone:** a tone whose VAG number is 0xFF (`vmNoiseOn` from `SsUtKeyOnV`, `SsUtKeyOn`,
    `SpuVmKeyOn`). TM2 opens one VAB, `SCREEN\UACORE.VAB` (`shellLoadUaCore` → `soundVabOpenRetry`). Its 124
    tones use VAG numbers 1..43, none 0xFF (counted from the disc with `tools/dmd_lod_dump.py`'s `Disc`).
  - **libsnd, `SpuVmNoiseOn` / `SpuVmNoiseOnWithAdsr`:** nothing calls them.
- `soundInit` does set the noise clock (`SpuSetNoiseClock(45)`): SPUCNT bits only, no voice.
- **So the second block's lack of pitch modulation and noise costs nothing.** For safety, the libraries do this
  under the define:
  - `SpuSetNoiseVoice` with a block-2 mask does nothing and returns 0;
  - `vmNoiseOn` / `vmNoiseOn2` on a voice 24..47 return without keying it on (the voice stays silent; the
    flush frees it after its envelope history is empty).

## 3. What changed

### 3.1 libspu (`sdk_libspu.c`, `sdk_libspu_sva.c`; 5 functions + 1 jump table)

| VA | function | change under the define |
|---|---|---|
| 80146B28 | `_spu_init` | block 2: key off all and reverb off, and on a cold start (mode 0) the 24 voices cleared as block 1's (volume 0, pitch 0x3FFF, start 0x200, ADSR 0), key on all, then key off all |
| 80147C20 | `_SpuInit` | `portSpuExt.keystat` (block 2's `_spu_keystat`) cleared |
| 8015D518 | `SpuSetNoiseVoice` | a block-2 mask: no noise there, returns 0 (section 2) |
| 80161C70 | `_SpuSetVoiceAttr` (+ its jump tables 800D12D8) | a block-2 mask writes block 2's registers; `_spu_voice_centerNote[24 + v]`; the start and loop addresses through `_spu_FsetRXXa(-1, addr)` (which aligns the address but would write block 1) |
| 801622D8 | `SpuGetKeyStatus` | a block-2 mask: block 2's ENVX and `portSpuExt.keystat` |

`SpuSetVoiceAttr` (80161C48) still passes voices 0..23 to `_SpuSetVoiceAttr`: the range is per block.
`_SpuSetAnyVoice` (8015D53C) is unchanged; its only caller in TM2 is `SpuSetNoiseVoice`. `SpuSetKey`,
`SpuSetReverbVoice` and `SpuGetVoiceAttr` are not in TM2's image (no module), so they need no change. Note that
`_spu_keystat` is written only by `SpuSetKey`, so in TM2 it is always 0, in both blocks.

### 3.2 libsnd (`sdk_libsnd.c`, `sdk_libsnd2.c`, `sdk_libsnd_ut.c`, `sdk_extra_snd.c`; 23 functions)

| VA | function | change under the define |
|---|---|---|
| 80156EF8 | `_SsInit` | block 2: the voice register template for its 24 voices, the control template's key on / key off / reverb words; `SpuVmInit(TM2_SPU_VOICES)` (was 24) |
| 801583C8 | `SsSetReservedVoice` | accepts 1..48 (was 1..24) |
| 801586A4 | `SpuVmKeyOnNow` | voice 24+: key-on and reverb bits in `portSpuExt.okon` / `orev` (the key-off words cleared of them, as retail does for 0..23) |
| 80158B90 | `SpuVmKeyOffNow` | voice 24+: `portSpuExt.okof` (the key-on words cleared of it) |
| 80158D1C | `SpuVmDoAllocate` | voice 24+: its envelope history bits in `portSpuExt.envx_hist` |
| 80158F54 | `vmNoiseOn` | voice 24+: returns, no noise there (section 2) |
| 80159424 | `vmNoiseOn2` | the same |
| 801597F0 | `SePitchBend` | `voice < TM2_SPU_VOICES` |
| 8015A560 | `SpuVmInit` | the tables cleared for 48; the cap is 48; voices 24+ reset in block 2; `portSpuExt`'s key and reverb words cleared |
| 8015AD64 | `SpuVmFlush` | ENVX of voices 24+ from block 2 into `portSpuExt.envx_hist`; their auto key-off from that history; volume/pan ramps for all 48; dirty shadows of 24+ copied to block 2; block 2's key off, key on and reverb words written after block 1's |
| 8015B118 | `SpuVmKeyOn` | the returned mask has a bit per keyed voice below 32 only (it is an `int`) |
| 8015C6A4 | `SsUtKeyOff` | `voice < TM2_SPU_VOICES` |
| 8015C7D0 | `SsUtKeyOnV` | the same |
| 8015CBC8 | `SsUtKeyOffV` | the same |
| 8015CCD0 | `SsUtChangePitch` | the same |
| 8015CE30 | `SsUtChangeADSR` | the same |
| 8015CF20 | `SsUtGetDetVVol` | the same; voices 24+ read from block 2 |
| 8015CF78 | `SsUtSetDetVVol` | `voice < TM2_SPU_VOICES` |
| 8015CFEC | `SsUtGetVVol` | the same; voices 24+ read from block 2 |
| 8015D09C | `SsUtSetVVol` | `voice < TM2_SPU_VOICES` |
| 8015D134 | `SsUtAutoVol` | the same |
| 8015D188 | `SsUtAutoPan` | the same |
| 8015D1DC | `SsUtAllKeyOff` | voices 24+ reset in block 2 |

Unchanged:
- **`SpuVmAlloc` (80158400):** the allocation itself: the first free silent voice, else the playing voice of
  lowest priority (ties: quietest envelope, then oldest), over `spuVmMaxVoice` (now up to 48). Every other loop
  over `spuVmMaxVoice` (`SpuVmKeyOff`, `SpuVmSeqKeyOff`, `SpuVmSetSeqVol`, `SpuVmSetVol`, `SpuVmPitchBend`,
  `SpuVmNoiseOff`) is also unchanged and now covers 48.
- **The functions that only index the per-voice tables** (`SetAutoVol`, `SetAutoPan`, `SeAutoVol`, `SeAutoPan`,
  `SpuVmPBVoice`, `vmNoiseOff`): they reach the moved tables by relocation only.
- **The voice age start value:** `SpuVmInit` / `SsUtAllKeyOff` still start each voice's age at 0x18 (24). It is
  only a starting value for the oldest-voice tie-break, kept as retail.

**The tables** (moved to `.port_bss` / `.port_data` with 48 entries; tsv group `spuvoices+`, layout `linear`;
voice v < 24 keeps its retail entry's place in the new array, so the address map translates retail addresses):

| table | retail | entry | section | variant (SPU48 alone) |
|---|---|---|---|---|
| `_svm_voice` (`D_801A5398`) | 801A5398, 24 | 0x34 | bss | 805412D8 |
| `_svm_sreg_buf` (`D_801A5200`) | 801A5200, 24 | 0x10 | bss | 80541C98 |
| `_svm_sreg_dirty` (`D_801A5380`) | 801A5380, 24 | 1 | bss | 80541F98 |
| `_spu_voice_centerNote` (`D_8017E15C`) | 8017E15C, 24 | 2 | data (0xC000 each) | 80200610 |
| `soundVoiceInUse` (game) | 8017F570, 24 | 4 | bss (retail .data, zero) | 8054201C |

**`portSpuExt`** (`struct TM2PortSpuExt`, 0x54 bytes, `.port_bss`, tsv group `one:spuvoices+`: defined only with
the define; 80541FC8 with SPU48 alone). It holds the bit words of voices 24..47, with bit n = voice 24+n:

| offset | field | retail counterpart (0..23) |
|---|---|---|
| +0x00 | `okon[2]` | libsnd `_svm_okon1` / `_svm_okon2` |
| +0x04 | `okof[2]` | `_svm_okof1` / `_svm_okof2` |
| +0x08 | `orev[2]` | `_svm_orev1` / `_svm_orev2` |
| +0x10 | `keystat` | libspu `_spu_keystat` |
| +0x14 | `envx_hist[16]` | libsnd `_svm_envx_hist` |

The addresses move with the other defines: read them from the build's `symbols.tsv`.

### 3.3 The game (`engine_sound_driver.c`, `engine_math_near_pass.c`; 8 functions)

| VA | function | change under the define |
|---|---|---|
| 8016069C | `soundDriverInit` | `soundVoiceLimit = TM2_SOUND_VOICES` (44) |
| 801606F8 | `soundDriverReset` | the same; keys off and frees voices 0..43 |
| 80160CCC | `soundInstUpdate` | `sndVoiceAttr.voice = TM2_SPU_VOICE_BIT(voice)` |
| 80160F20 | `soundOn` | the same, three times (`SpuSetVoiceAttr` before and after `SsUtKeyOnV`, `SpuSetNoiseVoice`) |
| 80161148 | `soundInstKeyOff` | the same (`SpuSetNoiseVoice`) |
| 8016120C | `soundVoiceAlloc` | `SpuGetKeyStatus(TM2_SPU_VOICE_BIT(i))` over `soundVoiceLimit` (lowest free voice first, as retail) |
| 80161318 | `soundDriverUpdate` | `SpuGetKeyStatus(TM2_SPU_VOICE_BIT(voice))`; the stealing cap is `soundVoiceLimit` (44) |
| 80156D08 | `soundInit` | `SsSetReservedVoice(TM2_SPU_VOICES)` (48; `soundReservedVoices`) |

`soundVoiceInUse` is declared `[TM2_SPU_VOICES]` and moved (above). `soundSilenceFreeVoices` and
`soundVoiceFree` are unchanged: they loop over `soundVoiceLimit` or index the moved table.

**`soundVoiceInUse` is placed right after `portSpuExt`, on purpose.** Retail keys off an instance whose
`SsUtKeyOnV` failed (voice -1) with `soundVoiceFree(-1)`, a store of 0 at `soundVoiceInUse[-1]`. In retail that
word is the unused one after `soundOverflowInsts[28]`. In the variant it is `portSpuExt.envx_hist[15]`, which
nothing reads (`SpuVmFlush` combines entries 0..14 only). `TM2_SPU_VOICE_BIT(-1)` is 0x80000000 (as retail's
`1 << -1`): a block-2 mask with no voice, so `SpuSetVoiceAttr` does nothing and `SpuGetKeyStatus` returns -1, as
in retail.

**Request 33-3 (per-car engine sound, `CAR_GROUPS.md` 12.4):** its sort (`portCarSoundSortInstances`) and cap
(`portEngineVoices`) work on `soundVoiceLimit`, not on a constant. So with the define the engines drop only past
44 voices; no change to that code was needed.

## 4. libspu and libsnd from C: the per-module override of `--sdk-from-lib`

### 4.1 Why

The port variant is linked with `--sdk-from-lib` (`SOURCE_BUILD.md`, "SDK from the PsyQ libraries"). That links
every module that `config/US/sdk_modules.tsv` marks exact in PsyQ 3.3 from Sony's `.LIB`, in place of the C:
- **libspu:** 23 modules, 27 functions. They include `SpuSetVoiceAttr` (`S_SVA`), `SpuGetKeyStatus` (`S_GKS`)
  and `SpuSetNoiseVoice` (`S_SNV`).
- **libsnd:** 2 modules, 3 functions, among them `SsSetReservedVoice` (`VS_SRV`).

Those `.LIB` modules know 24 voices. The other libspu/libsnd modules (`SPU`, `S_INI` and libsnd's 25 3.4-only
modules) are not exact in any 3.3/3.5 library, so they were always C.

The project rule (`RULES.md`) is that the SDK is linked from the `.LIB` and is not ours to change. This request
is the documented exception: with `TM2_PORT_SPU48`, and only then, both libraries come entirely from our C units,
compiled with the define.

### 4.2 How (`config/US/sdk_c_modules.tsv`, `tools/gen_link.py`)

- **`config/US/sdk_c_modules.tsv`** (hand-maintained) lists, per port define, the exact modules that
  `--sdk-from-lib` leaves to C: `define`, `lib`, `module`, `text`, `note`. Today it has 25 rows for
  `TM2_PORT_SPU48`: every exact libspu module and libsnd's `VS_MONO` and `VS_SRV`.
- **`tools/build_source_only.py`** (port variant with `--sdk-from-lib`) takes the rows whose define is among the
  variant's `-D` defines (`gen_link.sdk_c_override`). It passes them to `gen_link.plan(..., sdk_keep_c=...)`.
- **`apply_sdk_from_lib`** leaves those modules out of the `.LIB` conversion. So their C sections are not
  dropped: they are linked like any other C section, recompiled by the variant where the define changes them.
  The link fails if a listed module is not an exact module, or if one of its functions has no C section. The
  modules' data goes with them:
  - `S_SCA`'s and `S_SVA`'s `.rdata` jump tables are the C units' `#pragma tm2_rdata` sections;
  - `S_N2P`'s `.data` pitch table stays retail bytes (the C declares it `extern`).
- **The reports:**
  - `report.json` `sdk_from_lib.c_override` and `coverage.c_override_modules` / `c_override_functions` list the
    modules left to C (25 / 30);
  - `port_variant.sdk_c_override` lists them for the variant;
  - the link prints "25 modules (30 functions) left to C by sdk_c_modules.tsv".
- **Without the define, or outside the port variant, nothing changes.** The retail `--sdk-from-lib` links, E0,
  the default variant and the placement check all still take the 102 modules from the `.LIB`.

### 4.3 What it implies (CONFIRMED)

- **Every function of the 25 modules is byte-identical in C.** The hybrid image (C) equals retail over each
  module's whole `.text` (`py -3.14` check of `work/build-20261013-165534-786000`, all 25 `EQUAL`).
- **The static proof re-checks this in every build.** Each of these modules is a C section in the variant. The
  ones the define does not change are un-relocated and compared with E0, which has the `.LIB` module bytes at
  the same place, by `verify_reloc.py`. In the placement check the define is forced to 24 voices, so all 25 come
  from C unchanged and must equal E0. They do (section 8).
- **The 3.4-only libspu/libsnd modules were C already.** All their functions are byte-identical except one:
  - **`SpuVmSetVol` (8015BD20, libsnd `VMANAGER`) is close, not identical** (119 of its words differ; same
    behaviour as written). It was already in every `--sdk-from-lib`
    link and in E0 (VMANAGER has no exact `.LIB` module), so the override does not add it. Nothing in TM2
    calls it: `config/US/retail_relocs.tsv` has no `jal` to 8015BD20 from outside the function, and
    `data_relocs.tsv` has no pointer to it.
- **Out of the variant the rule stands.** No retail build and no variant without `TM2_PORT_SPU48` uses the
  overridden C.

## 5. Reserved voices: the port's menu sounds move to 44..47 (recommended, implemented)

- **What the port does today** (its `src/ui/options.c`): its menu sounds take the highest silent voice below 24.
  In practice that is 20..23. The game does not reserve those voices: its driver hands out 0..23, lowest free
  first, and each frame `soundSilenceFreeVoices` sets the volume of every voice it is not using (0..23) to 0. That
  is why the port's closing boom from the title menu gets cut off and has to be played through the driver
  (`boom_queued`).
- **The choice:** with the define the game's driver uses voices **0..43** (`soundVoiceLimit` = 44, allocation,
  silencing and stealing cap). It never touches **44..47**. libsnd still manages 48 (`SsSetReservedVoice(48)`), but
  for TM2 that only means `SpuVmFlush` reads their envelope. It writes no register of a voice that is not dirty,
  key-on/key-off words of 0 have no effect, and only block 2's reverb word is rewritten each flush (so the port's
  sounds on 44..47 have no reverb).
- **Why not keep 20..23 reserved:** the driver would need a hole in its voice range (allocation, silencing and
  the stealing cap). Also, 20..23 are not reserved today, so a sound of the game and a sound of the port can
  meet there.
- **What the port changes:** `free_voice()` / `key_on()` pick from 47 down to 44, with block 2's registers
  (0x1F802C00 + 0x10·(v-24)) and key-on word (0x1F802D8A, bit v-40). On the 24-voice build they keep 20..23.
- **The setting:** `--port-define TM2_PORT_SOUND_VOICES_RESERVED=N` (0..24, default 4) sets how many of the top
  voices the driver leaves alone. With 0 the driver uses all 48, and `soundVoiceLimit` is 48.

## 6. Storage and words for the port

| name | size | SPU48 alone | every port define + SPU48 | |
|---|---|---|---|---|
| `portSpuExt` | 0x54 | 0x80541FC8 | 0x8057E478 | block 2's bit words (3.2) |
| `soundVoiceInUse` | 0xC0 | 0x8054201C | 0x8057E4CC | the driver's voice table (48) |
| `D_801A5398` | 0x9C0 | 0x805412D8 | 0x8057D788 | `_svm_voice` (48) |
| `D_8017E15C` | 0x60 | 0x80200610 | 0x80200610 | `_spu_voice_centerNote` (48, `.port_data`) |

Read the addresses from `out/TM2_SOURCE_PORT/symbols.tsv` (or `port_arrays.tsv`). Retail addresses inside the
moved tables translate through `addrmap.tsv` (`port` rows), as for the other moved arrays.

## 7. Building

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_SPU48 [other port defines] --output work/pv/P4
py -3.14 tools/verify_reloc.py work/pv/P4 --e0 <build>/source_reloc_e0
py -3.14 build.py --source-only --sdk-from-lib --relocatable --port-variant --port-define TM2_PORT_SPU48 [...]
```

`--sdk-from-lib` is what makes the override matter. Without it (plain C link) libspu/libsnd are C anyway.

## 8. Proofs (static) and results

**How it was checked.** Each configuration was built with `tools/build_source_only.py --relocatable
--sdk-from-lib --port-variant` and checked as `build.py` checks it:
- the variant, proved against E0 with `verify_reloc.py`;
- its placement check (`--port-limits 2`, which forces `TM2_SPU_VOICES` to 24 too), proved the same way, with
  `build.py`'s rule: only the $gp functions and the functions the port defines change.

The objects were those of `work/build-20261013-165534-786000`, port RAM end 0x80C00000. All pass:

| configuration | variant sha256 | functions changed vs E0 (of them by the port's defines) | placement check sha256 | port sections end |
|---|---|---|---|---|
| no port define | `451fc414…` (= `LAST_PORT_BUILD.json`, unchanged) | 25 (0) | `b95e6fed…` | 0x80541400 |
| `TM2_PORT_SPU48` | `6756d3ba…` | 61 (36: section 3, + the `_SpuSetVoiceAttr` jump tables) | `b95e6fed…` | 0x80542260 |
| `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS=16`, `TM2_PORT_CAR_SWAP`, `TM2_PORT_CAR_SPECIALS`, `TM2_PORT_DARK_TOOTH`, `TM2_PORT_CAR_SOUND`, `TM2_PORT_CAR_CLUTS`, `TM2_PORT_CAR_DROPIN` + `TM2_PORT_SPU48` | `1872dd4f…` | 184 (165; without SPU48: 151 / 132) | `d4e50383…` | 0x805875A8 |
| every port define (+ `TM2_PORT_TEXTURE_BANKS`, `TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE`) + `TM2_PORT_SPU48` | `5020657e…` | 193 (174; without SPU48: 160 / 141) | `13f0a1b5…` | 0x805889F8 |

- **What SPU48 adds:** with the other defines the define adds exactly 33 functions. `soundDriverInit`, `soundDriverReset`
  and `soundDriverUpdate` were already changed by `TM2_PORT_CARS`.
- **The C SDK equals the `.LIB` (CONFIRMED by the build).** The placement check of `TM2_PORT_SPU48` alone links
  libspu and libsnd from C. The one without any define links them from the `.LIB`. Both images are the same
  (`b95e6fed…`) byte for byte.
- **The SDK functions the define does not change** are un-relocated against E0 in every variant above, and they
  pass.
- **The retail builds keep their hashes:** hybrid `9219f7ba6531…`, source-only `d755c86e…`, `--sdk-from-lib` /
  E0 `f557c1e3…` (the full default build `work/build-20261013-210155-514511`, which also publishes the default variant `451fc414…` again).

## 9. Tests for the port (not run)

1. **Without the define:** the variant must behave as before. `TM2PC_SPU_EXT_TEST=1` (each voice n played on
   voice 24+n by the port) is the test of the port's own extra voices, and it is meant for this 24-voice build.
   Do not combine it with the 48-voice build: there the game writes voices 24..43 itself.
2. **With `TM2_PORT_SPU48`, the sixteen-car battle** (`sixteen` scenario, `TM2_PORT_CARS=16` with
   `TM2_PORT_CAR_SOUND` bit 4, no `portEngineVoices` cap):
   - every engine and every weapon sound should keep its voice up to 44 at once;
   - `TM2PC_SPU_TRACE=1` should show key-ons on block 2 (voices 24..43) once more than 24 sounds play;
   - the game should never key on 44..47.
3. **The menus:** the port's menu sounds on 44..47 should not be cut off by the game's next frame.
4. **Release:** a voice of block 2 that ends should be found free again:
   - `SpuGetKeyStatus` reads block 2's ENVX (+0x0C), so the port's ENVX for the extra voices must fall to 0
     when the release ends, as block 1's does;
   - `SpuVmFlush` reads the same register for the auto key-off.
5. **Reverb:** a tone with reverb (VAB tone mode bit 2) on a voice 24+ sets block 2's reverb word
   (+0x198/+0x19A).
6. **Init:** at boot `_spu_init` keys block 2 on and off and clears its voices. `_SsInit` writes the templates
   (pitch 0x1000, start 0x3000, ADSR1 0x00BF) to block 2's voices.

## 10. Open questions

- **Runtime:** none of this has run. ENVX (+0x0C) of the extra voices must read back as on block 1, or a voice
  never looks free and the driver runs out at 24 again. ENDX is not read by TM2.
- **The port's menu sounds** must move to 44..47 (section 5). Until they do, they compete with the game for
  20..23 as today.
- **`SsUtKeyOnV` failure path (retail behaviour, unchanged):** an instance whose key-on failed keeps voice -1.
  Its voice stays marked in use until `soundDriverReset` (a retail leak). With 44 voices it matters less.

## 11. Request 87 (2026-10-30): whose sound a voice plays

Port request 87 part 1 (the user's: mod characters' own `engine.wav` and `special.wav`, the port's `MODDING.md` 4):
the port plays them from a bank of its own (PSY-Z patch 22), swapping the sample at a voice's key-on, so it must know
there which car the voice sounds for and what kind of sound it is. Asked: two tables outside the match state, set when
the driver gives the voice to a request, before the key-on reaches the SPU, for voices 0..47, the game's driver's and
libsnd's.

### 11.1 Built (`TM2_PORT_SPU48`; `spu_voices.h`)

- **`portVoiceCar[48]`** (u8): the car slot + 1 the voice's request sounds for, 0 = none. **`portVoiceKind[48]`**
  (u8): `TM2_VOICE_ENGINE` 1, `TM2_VOICE_SPECIAL` 2, 0 anything else. `.port_bss`, every variant (written only with
  the define), the sound side: snapshot and hash no. Addresses: the build's `symbols.tsv`.
- **When they are written** (CONFIRMED by reading; the check below):
  - **the game's driver:** `soundOn` (80160F20), after it has picked the voice and before `SsUtKeyOnV`
    (8015C7D0), calls `portVoiceOwner(voice, request)`. The key-on bit is set by `SsUtKeyOnV` and reaches the SPU
    at the flush right after it (`SsUtFlush`, 801619CC: 0x1F801D88 / 0x1F802D88), so the tables are current at
    that write. A retrigger (`soundOffOn`) and a stolen voice go through `soundOn` again.
  - **libsnd's own voices** (a sequence's, `SsUtKeyOn`'s, the noise's): `SpuVmAlloc` (80158400) sets both to 0
    for the voice it chooses. The game's driver does not use it (`SsUtKeyOnV` takes the driver's voice).
  - The port's own voices (44..47) are not touched.
- **Whose sound** (`portVoiceOwner`, from the voice's request, the `SndObj` a `soundInst` sounds for):
  1. **A tag:** a play of a car's special sets `portSoundTag` (kind << 8 | slot + 1) around itself; `sndPoolAlloc`
     copies it into `portSndReqTag[the request's index]` (64), so the request keeps it for every voice it is
     given, until the request is freed and handed out again. The tag counts only when the request's sound id is
     of the tag's kind (below), so a hit or an explosion the fire sets off is not the special's.
  2. **Else by instance:** an engine's sound, the flame (47), the tornado (0x35) and Dark Tooth's swoops (61, 66)
     played as a car's own instance (`portCarOptions` bit 4: instance = slot + 1) carry that car.
  3. **Else** an engine's sound gives kind 1 with car 0 (bit 4 clear: one request for every car of the type),
     anything else 0 / 0.
- **The kinds by sound id:** engines 0x24..0x3C even and 0x41 (`carSoundUpdate`'s switch). The specials' sounds:
  the fire sounds 0x27..0x3D odd (0x27 taser, 0x29 Warthog, 0x2B Mr. Grimm, 0x2D Grasshopper, 0x2F the flame,
  0x31 Spectre, 0x33 the boomerang's flight, 0x35 the tornado, 0x37 Axel, 0x39 Mr. Slam's grab and swing, 0x3B
  Shadow, 0x3D Sweet Tooth and Dark Tooth's first swoop), 66 his second swoop, and for a pickup template (request
  73) the launch and flight sounds 10..15, the ricochet 0x12 and the bomb 0x13; Minion's freeze missile (13) with
  them. Not the hits or the explosions (7, 8, 0x11 the spin-out, ...).
- **The tagged plays:**

| VA | function | unit | what is tagged |
|---|---|---|---|
| 80105A7C | `weaponFireSpecial` | `weapon_basic.c` | the whole type switch with the firer (`otherid`): every special's fire sound, the boomerang's flight, the tornado, Minion's 13, a template's (request 73: the type it borrows is in the switch) and a pickup template's (`portSpecialFirePickup`) |
| 800F4CC0 | `weaponSlamGrabUpdate` | `weapon_arcs_draw.c` | Mr. Slam's grab sound (0x39) |
| 800F4058 | `weaponSlamSwingUpdate` | `weapon_slam_swing.c` | his swing sound (57) |
| (port) | `portCarFlameSound`, `portCarSoundPlay`, `portCarEngineUpdate` | `port_variant_arrays.c` | the flame (47), the swoops, the engine (kind 1); each gives the caller's tag back after it |

  Engines need no tag with bit 4 (their instance is the car), and get one in the retail path too (the first car
  of the type to make the request).
- **Other changes:** `soundOn` and `sndPoolAlloc` (`engine_sound_driver.c`, `engine_sound_lists.c`), `SpuVmAlloc`
  (`sdk_libsnd.c`), all under `#if TM2_SPU_VOICES > 24`. Without the define nothing changes (the tag macros are
  empty).
- **Storage** (`.port_bss`, after `portStepScrubFill`): `portVoiceCar` 48, `portVoiceKind` 48, `portSndReqTag`
  128, `portSoundTag` 4.

### 11.2 Checked

`tools/check_rollback.py` check 1 (with the define): `soundOn` calls `portVoiceOwner` once, before `SsUtKeyOnV`;
`SpuVmAlloc` writes both tables; `sndPoolAlloc` reads the tag and writes the request's; the three game functions
store the tag; `portVoiceOwner` run in LinkMips on 9 made-up requests gives each its car and kind (a tagged
special's sound; a tagged explosion: none; an engine by its instance, and with none; a pickup's launch untagged
(none) and tagged; the flame by its instance; no request; voice 48: no write). **Not run in game.**

### 11.3 For the port

- Read `portVoiceCar[v]` / `portVoiceKind[v]` for each voice bit at the key-on write. Car = slot + 1 (the slot's
  mod: your `portCarProfile` mapping); kind 1: its `engine.wav`, kind 2: its `special.wav`.
- Per-car engines need `portCarOptions` bit 4 (you have it on): without it two cars of a type share one engine
  request, and the voice carries the first one's car.
- A special sound that loops while it flies (the boomerang) keeps its voice, so its car, until it stops.
