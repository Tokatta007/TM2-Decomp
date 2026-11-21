# Sony PsyQ SDK libraries: what TM2 links and where C exists

About 750 functions from 0x80138000 on are Sony's PsyQ SDK libraries. Of the
399 named ones, 346 are real SDK functions; the other 53 are TM2's own
(`fileio*`, `view*`, `sound*`, ...). This file records which SDK version TM2
links and which public projects already have C for those functions.
Research date: 2026-10-04.

## Version

TM2 links **PsyQ 3.4-era libraries (December 1995)**:

- **Embedded version strings:**
  - libgpu `sys.c v1.116 1995/12/01`
  - libcd `bios.c v1.71 1995/12/01`
  - libetc `intr.c v1.73 1995/11/10`
  - libsnd `ut_f.c v1.3 1995/08/23`
- **Signature matching:** [lab313ru/psx_psyq_signatures](https://github.com/lab313ru/psx_psyq_signatures) gives 3.4 the most whole-object matches (148).
- **Per function:** 243 of 329 identified functions are byte-equal to at least one other version, so C written for 3.3, 3.5 or 3.6 often matches as-is. 86 exist only in 3.4.
- **Compiler:** most libraries match with PsyQ 3.3's `CC1PSX.EXE` (GCC 2.6.0, `--compiler psyq3.3`, which implies aspsx 2.21), `gp_size 0`, no `-fvolatile` and no extern-gp. **libsnd was built with GCC 2.5.7** (`--compiler gcc2.5.7`; real `div` by constants, its own prologue/epilogue). How to build that compiler is in `tools/old-gcc/README.md`.

## Public C for these libraries

| Project | Licence | Coverage | Library version |
|---|---|---|---|
| [sotn-decomp `src/main/psxsdk`](https://github.com/Xeeynamo/sotn-decomp/tree/master/src/main/psxsdk) | **MIT** (per-file SPDX; the rest of that repo is AGPL — use only the psxsdk files) | about 400 functions: libapi, libc, libcard, libcd, libetc, libgpu, libgs, libgte (a few), libsnd, libspu | mixed 1995–96 |
| [brian-oblivion/lsddecomp](https://github.com/brian-oblivion/lsddecomp) `src/psyq` | **CC0** | libcd `bios`, libsnd, libspu, libgs, libcard (about 98 C functions) | December 1995, same era as TM2 |
| [sozud/psy-q-decomp](https://github.com/sozud/psy-q-decomp) | **MIT** | libsnd, libspu, libetc, libcd, libgs, libapi, libc, libpress (about 230 functions) | 3.3 / 3.5 / 3.6 / 4.0 |
| [Xeeynamo/psyz](https://github.com/Xeeynamo/psyz) `decomp/` | **MIT** | about 365 C functions (rest still asm) | 4.0 |
| [juandav/dcb_decomp](https://github.com/juandav/dcb_decomp) | MIT | about 490 functions | 4.7 (low value here) |

Do not use:
- **NFSHS-PSX-decomp:** all rights reserved.
- **Repos with no licence:** for example memories-decomp.
- **Sony's original files:** the `.LIB` / `.OBJ` files (for example sozud/psy-q) are proprietary.

## Estimate for TM2's 346 named SDK functions

- **About 216 likely free matches.** Their bytes equal a version that has C. Main sources: sotn 154, LSD 58, psy-q-decomp 54, PSY-Z 45; many functions have more than one source.
- **About 55 have C for a different version** and need porting. These are mainly libsnd (3.4-only) and libc.
- **About 75 have no C anywhere.** These are libgte (25) and the libapi BIOS stubs (14), which Sony wrote in assembly, plus libpress VLC and some libcd and libc.

| Library | Free | Port | None |
|---|---|---|---|
| libcd | 44 | 3 | 3 |
| libetc | 23 | 0 | 0 |
| libgpu | 59 | 2 | 3 |
| libspu | 31 | 2 | 1 |
| libsnd | 38 | 34 | 5 |
| libpress | 7 | 0 | 7 |
| libapi | 7 | 0 | 14 |
| libgte | 4 | 1 | 25 |
| libc | 2 | 9 | 4 |

Code taken from these projects keeps its licence notice and is listed in
`THIRD_PARTY.md`.

## Linking from the PsyQ libraries

Since the SDK is out of scope (`docs/RULES.md`), its code is to be linked from
Sony's `.LIB` files (supplied locally in `toolchain/psyq-sdk/psyq33` and
`psyq35`, never committed). `tools/psyq_lib.py` reads the LIB/LNK format and
`py -3.14 tools/psyq_lib.py match` places every 3.3 and 3.5 module in the
retail image. The result is `config/US/sdk_modules.tsv` (names, addresses and
sizes only, no library bytes). Research date: 2026-10-05.

The source-only build links these exact modules from the `.LIB` with
`--sdk-from-lib` (`tools/lnk2elf.py` converts LNK to ELF; see
`docs/SOURCE_BUILD.md`, "SDK from the PsyQ libraries").

A module counts as **exact** when its `.text` equals retail with every patch
applied against retail addresses (jump targets, `lui`/`addiu` pairs, jump
tables). Matching is per module, because TM2 links whole modules contiguously.

### Coverage

The SDK block (0x80138000 to the end of `.text`, 0x801625B8) holds 864
functions (173,108 bytes, by `ida_funcs.tsv` + `extra_funcs.tsv`).

| Class | Modules | Functions | Bytes |
|---|---:|---:|---:|
| Exact 3.3 module (all of them; 81 are also exact in 3.5) | 102 | 263 | 39,468 |
| Library code with no equal 3.3/3.5 module (3.4-only) | 36 | 240 | 60,936 |
| Not in any library (SingleTrac code linked among the SDK modules) | – | 361 | 72,704 |

- **Use 3.3.** Every exact module is in 3.3. Only 121 of the 263 functions
  (12,276 bytes) are also exact in 3.5, and no module is exact only in 3.5.
  The 3.3 libraries are dated 1995-10-18, two months before TM2's
  December 1995 libraries.
- **Of the 503 library functions,** 52 % (39 % of the bytes) can be linked
  as-is today.
- **crt0 is `2MBYTE.OBJ`.** `__main` and `start` are exact, and its `.sbss`
  is at 0x801810BC.
- **No library module matches below 0x80138000.** IDA's `SsInitHot` at
  0x8012A910 has the same shape as libsnd's but calls game functions.
- **Data.** 35 data sections of exact modules were located: 29 `.rdata` /
  `.data` equal after relocation and 5 are bss. One differs:
  - **libetc `INTR`:** the code is exact, but its `.rdata` holds the rcsid
    `intr.c v1.71 1995/08/29`, while retail has v1.73 (5 words differ).
    Linking 3.3 `INTR` would change that string, so `--sdk-from-lib` links
    its code but keeps the retail `.rdata` (the code refers to `D_800D0D00`).
  - **Not placed:** the 16-byte CD-register `.data` of libcd `CDROM` and
    `C_002`...`C_010`. Eight modules have 9 identical copies, so they cannot be
    assigned by bytes.

| Library | Exact modules | Exact functions / bytes | 3.4-only modules | 3.4-only functions / retail bytes |
|---|---:|---:|---:|---:|
| libgte | 16 | 105 / 12,992 | 0 | 0 |
| libcd | 13 | 47 / 9,268 | 3 | 19 / 8,796 |
| libspu | 23 | 27 / 7,184 | 2 | 13 / 4,768 |
| libetc | 5 | 28 / 3,316 | 0 | 0 |
| libgpu | 2 | 9 / 3,352 | 2 | 92 / 13,240 |
| libpress | 2 | 4 / 2,388 | 1 | 18 / 1,876 |
| libapi (BIOS stubs) | 20 | 20 / 248 | 1 | 5 / 384 |
| libc (BIOS stubs) | 15 | 15 / 180 | 2 | 2 / 2,992 |
| libsnd | 2 | 3 / 92 | 25 | 91 / 28,880 |
| libgs, libsn, crt0 | 4 | 5 / 448 | 0 | 0 |

### 3.4-only modules

The 3.3/3.5 module named in each row is the closest version. The diff is rough:
instructions are compared with their relocatable fields ignored, and "equal
functions" counts that module's named functions that match retail byte for
byte on their own.

| Module | Retail | Words differing | Equal functions | Note |
|---|---|---:|---:|---|
| libgpu `PRIM` | 80144978 | 2 | 31/32 | nearly exact |
| libapi `COUNTER` | 80155444 | 2 | 3/5 | nearly exact |
| libcd `CDREAD2` | 8013D9B4 | 4 | 0/1 | |
| libgpu `SYS` | 8014102C | 85 | 43/50 | 0x2C24 bytes; most functions equal |
| libspu `SPU`, `S_INI` | 80146A98, 80147C20 | 38, 9 | 6/11, 1/2 | |
| libpress `LIBPRESS` | 80139498 | 56 | 7/10 | |
| libcd `BIOS`, `CDREAD` | 8013ADB4, 8013CE10 | 416, 325 | 4/13, 0/4 | module split differs between versions |
| libc `SPRINTF`, `MEMMOVE` | 801388B8, 80139418 | 1128, 26 | 0/1, 0/1 | retail printf is 0xB40 bytes, 3.5's 0x85C |
| libsnd: 25 modules (`SCSSATTR` … `UT_GVA`, `VMANAGER`) | 80145EF8–80161A0C | 4 to 3266 | 2 of 76 | GCC 2.5.7 build; only `VS_MONO` and `VS_SRV` are exact |

The smallest libsnd differences are 4 words each, in `SCSSATTR`, `VS_VTC`,
`SSQUIT` and `SSEND`. Because no 3.3/3.5 module reproduces these 36 modules,
TM2's own 3.4 libraries (or matching C) are needed for a byte-identical link.

### Corrections to IDA names

- **InitPAD2 / StartPAD2 / StopPAD2** (8014AA70/80/90) are the libapi BIOS
  stubs `InitPAD` / `StartPAD` / `StopPAD` (modules A18–A20).
- **`sub_80145DE8` and `GsGetWorkBase` (80145E00)** are libetc `VMODE`
  (`SetVideoMode` / `GetVideoMode`). This is not libgs.
- **`SsInitHot` at 0x8012A910** is game code; the real one is 0x8015702C.
- **Library code inside units of other names:**
  - libspu `S_IT` covers 801482BC (`engine_clut_anim.c`).
  - libcd `EVENT` covers 8014DC04/2C/54 (`engine_fileio_debris.c`).
  - libcd `SYS` covers 8013A580 (`global_accessors.c`).
  - libsn `PUREV` (`__pure_virtual`) is 80145EC0 (`engine_misc_accessors.c`).

### SDK-looking code in no library

- **`80157CDC` (`sdk_extra_snd.c`):** the size of 3.3's `Snd_nextpause`
  (0x64). In 3.3 that function sits inside `PAUSE`.
- **`80157D40` (`sdk_libsnd_leftovers.c`):** the size of `Snd_play` / `_SsSndPlay`
  (0x2C).
- **`80156E08` (`sdk_libsnd_leftovers.c`) and `8015DAF8` (`sdk_libspu_leftovers.c`):** no
  convincing library counterpart.

All four sit next to libsnd/libspu modules and are probably 3.4 library
statics. Everything else in the SDK block that matches no module is SingleTrac
code. It is linked between library modules, e.g. `engine_clut_anim.c` sits between
libspu `S_IT` and libgte `MTX_00`. That ordering suggests SingleTrac's engine
was itself a library, pulled in on demand.

### Units that become redundant

**Every function is library code** (* in the tool output marks 3.4-only):
- **SDK units:** `sdk_libapi.c`, `sdk_libc.c`, `sdk_libcd.c`, `sdk_libetc.c`,
  `sdk_libgpu.c`, `sdk_libgte.c`, `sdk_libpress.c`, `sdk_libsnd.c`,
  `sdk_libsnd2.c`, `sdk_libspu.c`, `sdk_libspu_sva.c`, `sdk_extra.c`.
- **GTE units:** `gte_extra.c`, `gte_depth.c`, `gte_setters.c`.
- **Library-region units:** `lib_r138.c`, `lib_r140.c`, `sdk_libgpu_font.c`,
  `sdk_libsnd_vs_vh.c`, `sdk_libsnd_ut.c`.
- **BIOS units:** `bios_calls.c`, `bios_extra.c`.
- **CD units:** `cd_control.c`, `cd_toc.c`.
- **GPU units:** `gpu_addprim.c`, `gpu_clear.c`, `gpu_control.c`,
  `gpu_display.c`, `gpu_reset.c`, `gpu_video_mode.c`, `check_rect.c`,
  `DrawOTag.c`, `DrawSyncCallback.c`, `GetDispEnv.c`, `GetDrawEnv.c`.
- **MDEC:** `mdec_transfer.c`.

`py -3.14 tools/psyq_lib.py match` prints the VAs of each.

**Mixed units** (library functions | functions left in no library):

| Unit | Library functions | Left |
|---|---|---:|
| `engine_clut_anim.c` | 801482BC | 44 (80148660 … 8014BF3C) |
| `engine_fileio_debris.c` | 8014DC04 8014DC2C 8014DC54 | 24 (fileio*, Load4BitClut, …) |
| `sdk_libsnd_leftovers.c` | 8015702C* 8015704C* 801583B8 80159A00* 80159A24* 80159FC8* 8015B864* | 80156E08, 80157D40 (see above) |
| `sdk_libspu_leftovers.c` | 8015D5F0 8015D7F8 | 8015DAF8 |
| `sdk_extra_snd.c` | 801583A4 8015BA5C* 8015BAB0* 8015CF20* 8015CF78* 8015CFEC* | 80157CDC |
| `global_accessors.c` | 801395BC* 8013A580 | 42 game accessors |
| `engine_misc_accessors.c` | 80145EC0 (`__pure_virtual`) | 30 |

**No library function at all:** `engine_hit_detection.c`, `engine_anim_hit_volumes.c`, `engine_render_scene.c`,
`engine_math_near_pass.c`, `engine_sound_lists.c`, `engine_sound_object_update.c` and `engine_sound_driver.c` are
SingleTrac code.
