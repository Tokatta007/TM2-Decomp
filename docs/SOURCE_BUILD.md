# Source-only build

The goal is an executable built only from sources and assembled data, rather
than compiled units pasted over a copy of retail (`tools/build_maximal_hybrid.py`).
For now (M1) every segment is still retail bytes, pulled in with `.incbin`.
Later milestones swap segments for compiled C, piece by piece.

```
py -3.14 tools/survey_layout.py          # M0: writes config/US/layout.tsv + exe_header.json (--check to verify)
py -3.14 tools/split_retail.py           # writes split/bin/*.bin + split/SPLIT.json (gitignored retail bytes)
py -3.14 tools/build_source_only.py      # .s per segment -> as -> ld -> elf_to_psx_exe -> sha256 == retail
py -3.14 tools/build_source_only.py --build-dir last          # M2: link every unit object of the last hybrid build
py -3.14 tools/verify_image.py work/source-<timestamp>        # M2: compare with the hybrid and retail, check the map
py -3.14 build.py --source-only          # M4: fresh compile + hybrid, then link + verify -> LAST_SOURCE_BUILD.json
py -3.14 tools/build_source_only.py --build-dir last --relocatable [--shift ADDR:N]   # relocatable link
py -3.14 tools/verify_reloc.py OUT [--e0 E0_OUT]               # static proof of a relocatable link
py -3.14 tools/verify_reloc.py --experiments --build-dir last  # E0, E1, E1b + negative controls
py -3.14 build.py --source-only --relocatable [--shift ADDR:N] # -> LAST_RELOC_BUILD.json
py -3.14 tools/build_source_only.py --build-dir last --sdk-from-lib [--relocatable [--shift ADDR:N]]  # PsyQ modules from the .LIB
py -3.14 build.py --source-only --sdk-from-lib [--relocatable --shift ADDR:N]                       # -> source_sdklib/
py -3.14 tools/lnk2elf.py --exact                                # convert the exact PsyQ 3.3 modules into work/sdk_lib/
py -3.14 tools/function_maps.py LINK_DIR [...]                  # functions.tsv, jumptables.tsv, recompiler inputs of a relocatable link
py -3.14 build.py --source-only --sdk-from-lib --relocatable --port-variant   # the port variant -> out/TM2_SOURCE_PORT.*, LAST_PORT_BUILD.json
```

Each build writes to a new `work/source-<timestamp>/` directory. It holds the
generated `.s` files, the linker script, the ELF, the map, `TM2_SOURCE.EXE`
and `report.json`. If `split/` is missing or stale, the build regenerates it.

## Layout (US, SCUS_94306)

| name          | start    | end      | size    | kind        |
|---------------|----------|----------|---------|-------------|
| model_emitter | 800CDC54 | 800CE910 | 0xCBC   | code        |
| rodata        | 800CE910 | 800D1318 | 0x2A08  | rodata      |
| text          | 800D1318 | 801625B8 | 0x912A0 | code        |
| data          | 801625B8 | 801809D4 | 0x1E41C | data        |
| sdata         | 801809D4 | 80180EE4 | 0x510   | sdata       |
| bss_in_file   | 80180EE4 | 80181454 | 0x570   | bss_in_file |
| bss           | 80181454 | 801FAD48 | 0x798F4 | bss         |

`survey_layout.py` derives each boundary and asserts the evidence below. It
also checks that the segments tile `[t_addr, bss_end)` with no gaps and no
overlaps.

- **800CDC54, start of model_emitter.** This is `t_addr` from the PS-X EXE
  header. The first listed function, `sub_800CDC54`, sits here: 815 words, the
  SingleTrac model emitter.
- **800CE910, start of rodata.** This is where `sub_800CDC54` ends. No listed
  function starts until 800D1318. The first label in the gap is a string,
  `aInternalErrorW`. 700 of its words are addresses inside `text`: these are
  the switch jump tables.
- **800D1318, start of text.** The first function after the gap
  (`rtMainLoop`).
- **801625B8, end of text and start of data.** This is where the last listed
  function, `_spu_pitch2note` (80162460), ends. The function lists are
  `ida_funcs` + `extra_funcs` âˆ’ `not_funcs`; `not_funcs.tsv` drops IDA's false
  positives past this point.
  - Inside `text`, functions run without a break except for 46 small unlisted
    gaps (284 bytes in all, the largest 56). These are inline data and padding,
    for example crt0's stack-size table at 80138838.
- **801809D4, start of sdata.** This is `gp`, set by crt0 at pc0 with `lui/addiu
  $28`. No gp-relative access in the code goes below gp. The code makes 1790
  such accesses, spanning 801809D4..80181164: 871 fall in sdata and 919 in sbss.
- **80180EE4, start of BSS.** crt0's clear loop starts here (the `lui/addiu v0`
  constants). From this point to the end of the file image, every byte is zero.
- **80181454, end of the file image.** This is `t_addr + t_size`, and it equals
  `ALIGN(80180EE4 âˆ’ t_addr, 0x800)`. So `bss_in_file` is just the padding that
  rounds the file up to the 0x800 boundary.
  - The build emits `bss_in_file` as NOLOAD. `elf_to_psx_exe.py` reproduces
    the padding by rounding the end of the last PROGBITS section up to 0x800.
- **801FAD48, end of BSS.** crt0's clear loop ends here (the `lui/addiu v1`
  constants).

## Header (`config/US/exe_header.json`)

Only 77 header bytes are non-zero:

- magic `PS-X EXE`
- pc0 = 8013878C (`start`)
- t_addr = 800CDC54
- t_size = 0xB3800
- s_addr = 801FFFF0
- the region string at 0x4C, "Sony Computer Entertainment Inc. for North
  America area"

gp0, d/b/s sizes and every other header byte are zero. The survey fails if any
other header byte is non-zero.

## M2: the nonmatching flavour

`--build-dir DIR` (or `last`, the build named by `LAST_BUILD.json`) links every
compiled unit object of that hybrid build into the image. Nothing is recompiled:
the objects listed in `DIR/selection.json` are reused as they are, and the
build fails if an object's sha256 differs from the selection. Retail bytes only
fill what no object owns. `tools/gen_link.py` computes the plan; the build
writes `link_plan.json`, `source_only.ld`, `symbols.ld`, the map, the ELF,
`TM2_SOURCE.EXE` and `report.json` to the new `work/source-*` directory.

**Placement.** Sections are placed with the hybrid's rules
(`tools/build_maximal_hybrid.py`), and the rules must agree:

1. A selection function's symbol fixes its section at VA âˆ’ symbol offset.
2. A global whose name resolves through `symbol_map()` fixes its section.
3. `.tm2_<X>` sits at X.

Each input section becomes its own output section at that address
(`.c.<addr>`). Plain `.text`/`.bss` objects are placed at their first
symbol's address (the 40 `effect_bindings/*`, the 4 `gpu_bindings/*`,
`weapon_flame_update.c`, `fx_ground_fires.c`, `hud_state.c`, `rt_buffer_init.c`, plus
the two `.bss` sections of the `func_8010*` units). Every symbol they contain is
checked against its VA. Sections at or above 80180EE4 are NOLOAD, and the
generator checks that their contents are zero. Empty declared-only placeholder
sections (for example `.text_8014A7B8` in `shared_pad.c`) are discarded by
name. `.rel.dyn` is asserted empty.

**Slots and holes.** A code section owns the full retail slots of the
functions it holds (ida_funcs + extra_funcs âˆ’ not_funcs). It must fit inside
them. When it is shorter, nothing is written in the rest of the slot, so the
writer's zero buffer fills it. A data section owns exactly its own bytes. Each
layout segment is cut into `.incbin "split/bin/â€¦", off, len` pieces (`.space`
for BSS) around these holes (`.r.<addr>`). The link uses
`--orphan-handling=error`.

**Symbols (`symbols.ld`).** The script sets `_gp = 0x801809D4`. Each import of
each object is resolved in one of four ways:

- **Defined by another object:** nothing is assigned; an ASSERT checks the
  mapped address.
- **Alias:** `Name = sub_X;`, when the address is a C function's symbol, for
  example `start = sub_8013878C;` (the entry point).
- **Interior:** `D_x = sym + 0xOFF;`, when the address lies inside a placed
  section.
- **Absolute:** `D_x = 0xâ€¦;` for everything else.

Every assignment gets an ASSERT, and so does every selection function. The
generator fails on:

- a name both defined by an object and assigned in the script
- duplicate definitions
- unresolved imports
- an import that points into a zero-filled tail
- a section that exceeds its slots
- a section that crosses a segment boundary
- overlaps

**Result** (objects of build `work/build-20261005-145118-709641`, hybrid
sha `31e82cb604b0â€¦`):

- **Links:** 308 unit objects, 2067 C sections (2017 `.text_<VA>`/`.tm2_<X>`,
  50 plain), 109 retail pieces, 17 zero-filled slot tails.
- **Image:** sha256 `40872a4b8abd8fb781cbdcce4c6e6414c834385c87151bc67987b7b691a47292`,
  t_size 0xB3800, header identical to retail. Rebuilding gives the same sha.
- **Symbols (`symbols.ld`):** 1171 imports are defined by another object, 196
  are aliases, 17 are interior and 1273 are absolute:
  - **Interior:** 14 labels inside the hand-written asm `sub_801526CC` (renderWalkModel) of
    `engine_render_scene.c`, used by that unit itself; `D_80184A78` (weaponShieldRecords)/`D_80184AF0` inside
    `asset_pools.c` tables; `D_80184DD0` (weaponTornadoOwner) inside `round_reset.c`'s
    `D_80184DC0` (weaponTornadoState).
  - **Absolute:** they point into retail data: rodata 247, data 262, sdata 277,
    bss_in_file 144, bss 337. Also 2 point into the unlisted text gaps
    (`D_80138838` is crt0's stack table, plus `D_80140FF4`), 3 are hardware or
    scratchpad addresses, and 1 is the BSS end `D_801FAD48`.
- **M1:** the M1 build (no `--build-dir`) still matches retail.

**Verification (`tools/verify_image.py`).** It writes `verify.json` next to the
image and attributes every differing byte run to a header field, a function
slot, a placed C section or the nearest data label.

- **vs the hybrid:** 829 bytes in 214 runs, all inside zero-filled slot tails.
  16 of the 17 tails differ, because the hybrid keeps retail's tail bytes.
  `SpuMalloc` (801467D0) does not differ: its retail tail is already zero.

  | VA | function | unit | tail bytes |
  |----|----------|------|-----------|
  | 801388B8 | sprintf | lib_r138.c | 748 |
  | 80139C1C | MDEC_vlc | lib_r138.c | 180 |
  | 8015DAF8 | sub_8015DAF8 | sdk_libspu_leftovers.c | 40 |
  | 801424B4, 80142580 | sub_801424B4, sub_80142580 | lib_r140.c | 28 each |
  | 8013ADB4, 80139F50, 8015BD20 | getintr, MDEC_vlc2, SpuVmSetVol | sdk_libcd/libpress/libsnd2 | 16 each |
  | 80141BA0, 80142718 | PutDispEnv, get_dx | gpu_display.c | 12 each |
  | 8013C3A8, 8013DD78, 8013E724, 80139418 | cd_read_retry, StCdInterrupt, dma_execute, memmove | sdk_libcd/libc | 8 each |
  | 80142458, 8014264C | get_mode, get_ofs | sdk_libgpu.c | 4 each |

- **vs retail:** 10821 bytes in 1895 runs:
  - **close functions:** 10816 bytes, inside the slots of exactly the 41
    functions whose hybrid bytes differ from retail (1948 âˆ’ 41 = 1907 match).
  - **placed data:** 5 bytes in the jump table `.tm2_800D0650` (`sdk_libcd.c`).
  - **other:** nothing differs anywhere else.
- **Map:**
  - every selection function symbol is at its VA
  - every `.text_<VA>`/`.tm2_<X>` input section is at its address
  - every symbol of a plain section is at its VA
  - the output sections and zero tails tile 800CDC54â€“801FAD48
  - no output section crosses a layout.tsv boundary
  - every retail piece comes from its own segment's object
  - `_gp` = 801809D4
  - no non-empty code or data input section was discarded

**Problem found: BSS storage claimed twice.** `round_reset.c` defines
`D_80184DC0` (weaponTornadoState) (`.tm2_80184DC0`, 0x50 bytes, to 80184E10). `weapon_leap_homing_init.cpp`
defines `D_80184DD4` (homingLeapState) (`.tm2_80184DD4`, 0x3C bytes), which lies inside it. Both
are zero-initialised BSS. The hybrid never noticed, because it does not write
BSS. The link keeps both sections as NOLOAD and reports the pair as an alias:
`--no-check-sections` is passed to ld only when such an alias exists, and
`elf_to_psx_exe.py` allows exactly the listed NOBITS-in-NOBITS aliases and
rejects every other overlap. It does not change the image, but the sources
should agree on who owns the storage: one unit should define it and the other
should declare it `extern`, or `TM2ShieldState` should stop covering
80184DD4.

## M4: `build.py --source-only`

`build.py --source-only` (or `build.cmd --source-only`) runs the normal build
first: every unit is compiled into a new `work/build-<timestamp>/`, the hybrid
is linked and must match `expected_image_sha256`, and `LAST_BUILD.json` is
written for it as usual. Then, from that same build directory:

1. `tools/build_source_only.py --build-dir work/build-<timestamp> --output
   work/build-<timestamp>/source` links the fresh objects (M2 above);
2. `tools/verify_image.py work/build-<timestamp>/source` must report
   `VERIFY OK`;
3. the image's sha256 must equal `expected_source_only_sha256` in
   `BUILD_MANIFEST.json` (if the key is missing, a warning is printed and the
   image is not checked);
4. `LAST_SOURCE_BUILD.json` records the image, its sha256, the expected value,
   the hybrid it was built with, the link counts and the verify summary.

`LAST_SOURCE_BUILD.json` is separate on purpose: `LAST_BUILD.json` always means
the hybrid image. Without
`--source-only`, `build.py` behaves exactly as before.

When a source change moves `expected_image_sha256`, update
`expected_source_only_sha256` too: run `build.py --source-only` once (it fails
with the new sha), check the verify output, and copy the sha into the manifest.

**Result** (hybrid `dd61030122a9â€¦`):

- **Image:** sha256 `c51f197415aecc128ccbc75d62df3a647234a38569a88b86d9caf979e8554f15`
  (`expected_source_only_sha256`). `--build-dir last` on the same objects gives
  the same sha.
- **Links:** 308 unit objects, 2066 C sections, 109 retail pieces, 3
  zero-filled slot tails.
- **vs the hybrid:** 45 bytes in 15 runs, all in the 3 zero-filled tails
  (`80139F50`, `8015BD20`, `8015DAF8`).
- **vs retail:** 2201 bytes, all inside the slots of the 26 close functions;
  no placed data differs.

## Relocatable build: pointer census

A relocatable build lets functions grow or move and data shift. Every address
stored in the image must then come from a symbol. `tools/pointer_census.py`
finds those addresses. It reads the retail image and the objects of the last
hybrid build; it writes nothing else, and `--check` verifies the outputs.

```
py -3.14 tools/pointer_census.py            # writes config/US/data_relocs.tsv + code_abs_refs.tsv
py -3.14 tools/pointer_census.py --no-sdk   # skip the PsyQ .LIB evidence (1 s instead of 20 s)
```

**What it scans.**
- **Data:** every aligned word of rodata, data, sdata and the 46 inline-data
  gaps of the code segments. A word is a candidate when its value lies in
  800CDC54..801FAD48 (image or BSS).
- **Relocated words:** every word that a compiled object (`R_MIPS_32` in a
  placed data section) or a placed PsyQ module (32-bit data patch) relocates.

**Evidence, strongest first:**
- `obj-reloc` and `lib-patch` are exact.
- `lw+jr` means a retail-code scan shows the function loading the table and
  jumping through it.
- `gcc2 vtable` is the 8-byte zero header, then `{delta/index word, pfn}`
  entries.
- `code-ref` means code builds the table base with lui/addiu.
- Also used: IDA `jpt_`/`off_` names and pointer declarations, pointer-typed
  `D_<addr>` declarations in `src/semantic`, table runs (strides 4..32) and
  the target (function start, string, label).

**Result: `config/US/data_relocs.tsv`, 859 words, none coincidental.**

| segment | jump_table | vtable | code_ptr | string_ptr | data_ptr | total |
|---------|-----------:|-------:|---------:|-----------:|---------:|------:|
| rodata  | 631 | 69 | â€“ | 5 | â€“ | 705 |
| data    | â€“ | â€“ | 21 | 124 | 8 | 153 |
| sdata   | â€“ | â€“ | â€“ | 1 | â€“ | 1 |
| text gaps | â€“ | â€“ | â€“ | â€“ | â€“ | 0 |

- **Confidence:** 667 certain and 192 likely. None is doubtful.
- **Jump tables:** every one of the 631 entries has both an object relocation
  and the lw+jr pattern.
- **vtables:** 49 gcc 2.x vtables with 69 slots. 21 of the slots are
  `__pure_virtual` (80145EC0), and 62 of the slots belong to a vtable that a
  constructor addresses.
- **Data segment, game:** the car-name/message table 80162830 (52), the TIM
  name table 80164868 (14), the movie table 801648C4 (14, stride 12), and
  sdata 80180AC4 â†’ "LIVES".
- **Data segment, SDK:** libcd's command and interrupt names 80176BBC (41) and
  bios.c 80176EA8 (6, one pointing at an odd byte address 80176BB9), libgpu's
  sys.c table 8017C2D4 (18) and font.c 8017CFC8, libetc's intr.c 8017E044 (6),
  and libsnd 8017F03C.
- **Not pointers:**
  - the text gaps hold none: crt0's 80138838 holds four 0x00200000 size words, and
    80140FF4 is exception-hook code that is copied to low RAM;
  - nothing is stored unaligned;
  - no KSEG1 (A0â€¦) form appears;
  - no data holds a hi/lo split address;
  - 229 data words lie in 000CDC54..001FAD48. They are counted as integers:
    no code or table evidence makes them addresses, and libgpu builds its
    masked `& 0xFFFFFF` addresses at run time.
- **Who relocates them:** 659 words are already relocated by compiled
  objects and 8 only by a .LIB module. **192 are still retail `.incbin` bytes
  and must become `.word symbol`:**
  - 43 vtable slots;
  - the 80 game strings;
  - the SDK tables above, unless their 3.4-only modules come from a .LIB.

**Code side: `config/US/code_abs_refs.tsv`.** Code bytes are scanned for
`j`/`jal` with no `R_MIPS_26`, lui/lo pairs with no HI16/LO16, gp-relative
accesses with no GPREL16, image-address words and branches that leave their
function.

- **Retail code:** none is left. Every function slot is owned by an object,
  and the gaps hold no code references.
- **Objects:** the hand-written asm blocks (`engine_render_scene.c`, `engine_math_near_pass.c`,
  `engine_hit_detection.c`, `model_emitter.c`, crt0 in `lib_r138.c`, `__pure_virtual` in
  `engine_misc_accessors.c`) are symbolic: `jal sub_X`, `%hi/%lo(D_X)`.
- **One finding:** `gte_extra.c` 8013FDFC is a `.word 0x10000011`, a `beq
  $0,$0` into `VectorNormalSS+0x14` (8013FE44). It is a pc-relative branch
  across functions, so it breaks if either function moves. Write it as a
  symbolic branch (or keep the two sections adjacent).
- **engine_hit_detection / engine_render_scene / engine_math_near_pass:** these reuse `$28` as a data register,
  so their `$28` operands are not gp-relative.
- **Numeric addresses in C:** they all lie outside the image and stay fixed:
  - the asset load buffers 80010000, 80019C40.. and 80048270 below `t_addr`;
  - the scratchpad 1F80xxxx.
- **Not covered here:** the 1267 absolute `D_x = 0xâ€¦` imports in `symbols.ld`.
  They point into retail pieces, which need labels once those pieces can move
  (including the gap labels `D_80138838` and `D_80140FF4`). The relocatable
  link below turns 1264 of them into labels.
- **Missed by the census, found by the relocatable proof:** 10 stores used
  `lui $1,%hi(D_X)` followed by a *numeric* low half, `sw $r,4284($1)`: crt0
  (`D_801810BC`, the saved ra), `MDEC_vlc` (`D_80164AF4`..`D_80164B04`, 5 stores)
  in `lib_r138.c`, and 4 in `gte_extra.c` (`D_8017E8BC`, `D_8017E8C8`). The HI16
  is relocated but the low half is not, so the store would keep its retail
  address once `D_X` moves. The sources now say `sw $r,%lo(D_X)($1)` (same
  bytes; `tools/match_unit.py` reports "all match" for both units, and the
  hybrid and source-only shas are unchanged). maspsx used to expand any store
  whose operand is `%lo(sym)(reg)` into a 4-instruction `$at` sequence; a TM2
  local patch now leaves it alone, as its load handler already did.

`target_symbol` uses the object's global name where a unit defines one, so it
follows the last hybrid build.

## Relocatable link

The relocatable flavour links the same objects and retail bytes as the M2
nonmatching flavour, but nothing is pinned to a retail address except the
first section at `t_addr`. Functions may outgrow their retail slots and the
image hash is not enforced; what is enforced is a static proof that the
shifted image is the fixed image, moved.

```
py -3.14 tools/build_source_only.py --build-dir last --relocatable --output work/r/E0
py -3.14 tools/build_source_only.py --build-dir last --relocatable --shift 800D1318:16 --output work/r/E1
py -3.14 tools/verify_reloc.py work/r/E0                    # E0 proof
py -3.14 tools/verify_reloc.py work/r/E1 --e0 work/r/E0     # E1 proof
py -3.14 tools/verify_reloc.py --experiments --build-dir last
py -3.14 build.py --source-only --relocatable --shift 800D1318:16
```

**Layout (`tools/gen_link.py`, `reloc_plan`).** Every item is laid out in
retail address order from `. = 0x800CDC54`, each as its own output section
with `ALIGN(n)`:

- **C sections:** their own alignment (code at least 4); each takes its
  compiled size, so a larger function pushes everything after it.
- **Slot tails:** a C function shorter than its slot keeps the rest of the slot
  as zero padding (`.t.<addr>`), so a zero-shift link keeps the retail
  addresses.
- **Retail pieces:** the alignment of their retail start, at most 4.
- **`--shift ADDR:N`:** inserts N zero bytes (`.x.<addr>`, a multiple of 4)
  before the item at retail ADDR. A retail piece is split when ADDR falls
  inside it; inside a C section it is refused. The padding is NOBITS when the
  next item is.
- **BSS:** the BSS pieces follow sdata in retail order as NOLOAD sections.
  Storage claimed twice (`alias_of`) is placed inside its outer section.

**Retail pieces become generated `.s` (`reloc_<segment>.s`).**

- **Labels:** each piece gets a global label at every image or BSS address that
  an object imports. That is 1264 of the 1267 absolute imports. The labels
  include the text-gap labels `D_80138838` and `D_80140FF4`, `D_80180EE4` (BSS
  start) and `D_801809D4` (gp). `D_801FAD48` (BSS end) is a label after the
  last BSS piece.
- **`.incbin`** covers the plain bytes.
- **`.word target + addend`** fills each `data_relocs.tsv` slot that lies in a
  retail piece: 200 words, all of them certain or likely. These are the 192
  `.incbin` words of the census plus the 8 relocated only by a .LIB module.
  - **When the target is in a C section:** the nearest object global at or
    below it, plus an offset.
  - **Otherwise:** a label. It takes the retail IDA name (`aAxel_0`), or
    `D_<addr>` when no IDA name is free.
- **The other 659 slots** live in C data sections, and their objects relocate
  them.
- **`.align 0`** stops gas from auto-aligning `.word`.

**`symbols.ld`.**

- **Kept:** the object aliases (`start = sub_8013878C`) and interior
  references (`D_x = sym + 0xOFF`).
- **Still absolute:** only the 3 out-of-image imports (`D_1F800060` (renderScrDrMode),
  `D_1F80006C` (renderScrDrModeDither), `D_1F800088`, all scratchpad). The 0x80010000-area buffers are
  numbers in C, not imports.
- **`_gp = ADDR(<first sdata item>)`**, with an ASSERT that it equals
  `D_801809D4`, the label that crt0 loads `$28` from.
- **crt0 uses symbols for everything:** it clears `D_80180EE4`..`D_801FAD48`
  and calls `InitHeap(D_801FAD48 + 4)` (the `addi $4,$4,4` after
  `%hi/%lo(D_801FAD48)`). The BSS clear, the heap and the stack-size table
  `D_80138838` all follow the layout.

**The gte_extra branch.** The raw `.word 0x10000011` at 8013FDFC (a `b` into
`VectorNormalSS+0x14`) stays in the source. gas would take `b sym+off` across
sections (R_MIPS_PC16), but the source is left as it is. Instead, each
`code_abs_refs.tsv` branch becomes a linker-script ASSERT: the distance from
the branch to `sub_8013FE30 + 0x14` must stay 0x48. A shift between the two
sections (`--shift 8013FE30:16`) therefore fails the link with "keep its
sections adjacent". So does a size change of `sub_8013FDF0`, whose code is
asm.

**Header.** `elf_to_psx_exe.py --relocated` (or `convert(..., relocated=True)`)
takes `pc0` from the ELF entry and lets `t_size` follow the link:
ALIGN(end of sdata âˆ’ t_addr, 0x800). Every other header byte is checked as
before.

**Outputs next to the EXE** (`tools/reloc_image.py`):

- **`addrmap.tsv`:**
  - **Header lines** `gp_r gp_c bss_end_r bss_end_c entry_r entry_c`, then
    rows `r_start r_end c_start c_end kind owner`.
  - **Code** has one row per function; a slot tail is merged into its function.
    A function is `code_linear` when its un-relocated candidate bytes equal
    retail over the whole slot, otherwise `code_entry`. In E0 that gives 1968
    linear rows and 26 entry rows, the 26 close functions.
  - **The 46 text gaps** are `code_linear` rows owned by `retail text`.
  - **Data items** are `data` (rodata, data), `sdata` or `bss` (bss_in_file,
    bss).
  - **`aux`** marks the shift padding and any alignment gap; these rows have
    `r_start == r_end`.
  - **Coverage:** the rows tile 800CDC54..801FAD48 in retail and the whole
    image in the candidate.
- **`symbols.tsv`:**
  - **Format:** `va code name size`.
  - **Contents:** every global of the linked ELF, including the script-defined
    ones, plus each retail `ida_names.tsv` name that the link does not define,
    translated with `r2c` (size 0). For example, `rtMainLoop`,
    `StCdInterrupt`, `VSync`, `SetFrameStart`, `start` and `_gp` are all
    present.
- **`pointer_slots.tsv`:**
  - **Format:** `r_addr width target_symbol addend kind`.
  - **Contents:** all 859 `data_relocs.tsv` slots. The symbol is one that
    `symbols.tsv` defines.

**Proof (`tools/verify_reloc.py`).** It reads the relocation records of every
linked input section, both unit objects and generated pieces: R_MIPS_32/26,
HI16/LO16 paired as ld pairs them, and GPREL16. There are 26811 records.
Then it checks:

- **relocs:** every field holds what its record says.
- **map:** the rows tile both coordinate spaces. In E0 the map must also be the
  identity.
- **unrelocate (E1):**
  - each candidate row is copied to its retail position;
  - each relocated field is rewritten with `c2r(address)`, where GPREL16 uses
    `c2r(gp)`;
  - the result must equal the E0 image byte for byte;
  - pc0 must map to E0's pc0, and the header must be otherwise equal.
- **sha (E0):** the image must equal `expected_source_only_sha256`.
- **slots:**
  - each `data_relocs` slot must be an R_MIPS_32 site;
  - its word must equal `r2c(retail value)`;
  - its `pointer_slots.tsv` row must resolve to that word through
    `symbols.tsv`.
- **hi16:** no `lui %hi(X)` may be used by a load, store or addiu without
  `%lo(X)`.
- **unrelocated:** no data word outside a relocation site may hold an image
  address.
- **branches:** each `code_abs_refs.tsv` branch must still reach its retail
  target.
- **header:**
  - gp must equal `r2c(sdata start)`;
  - `D_801FAD48` must equal the candidate BSS end;
  - `D_80180EE4` and `D_801809D4` must follow the layout;
  - `start` must map to 8013878C.

**Results** (objects of `work/build-20261005-215825-474525`, hybrid
`dd61030122a9â€¦`):

| run | shift | sha256 | t_size | pc0 | gp | BSS end | proof |
|-----|-------|--------|--------|-----|----|---------|-------|
| E0 | none | `c51f197415aeâ€¦` = `expected_source_only_sha256` | 0xB3800 | 8013878C | 801809D4 | 801FAD48 | all checks pass, identity map |
| E1 | 800D1318:16 | `5c51f5195a2fâ€¦` | 0xB3800 | 8013879C | 801809E4 | 801FAD58 | all checks pass |
| E1b | 801625B8:16 | `ed6377efe8f2â€¦` | 0xB3800 | 8013878C | 801809E4 | 801FAD58 | all checks pass |

- **E0:** byte-for-byte the fixed nonmatching build.
- **E1:** text, data, sdata, BSS and `start` move by 16; rodata stays. The
  631+69 rodata jump-table and vtable words point at the shifted code.
- **E1b:** only data, sdata, gp and BSS move.
- **t_size is unchanged in both:** the 0x570 bytes of `bss_in_file` padding
  absorb the 16 bytes. That is exactly why a shifted image must never be
  compared without its map.

**Negative controls (`--experiments`).**

- **E1 proved with an identity map** fails four checks: unrelocate (the jal
  targets differ), slots, branches and header.
- **E1 built with `--corrupt-slot 80162858`** keeps the retail word in place of
  `.word aAxel_0` (80180A9C, in sdata, which moves). This build still
  *passes* unrelocate, because an unrelocated word has unchanged bytes. It fails
  slots (the word is not a relocation site and does not point at the shifted
  string) and unrelocated. This is why static data needs the typed checks.
- **The hybrid build from before the fix** (`work/build-20261005-213642-727548`)
  fails hi16 with the 10 stores listed above.

**Not relocatable yet / limits.**

- **gte_extra pair:** the `sub_8013FDF0`/`sub_8013FE30` pair must stay adjacent
  and keep its size (see above).
- **Shift points:** a shift must fall at an item boundary, never inside a C
  section.
- **Pieces at unaligned starts:** retail pieces that start at an address not
  aligned to 4 only keep the internal word alignment if the shift above them is
  a multiple of 4.
- **Imports:** the 3 scratchpad imports stay absolute, which is correct for
  them.
- **Static only:** this has not been run. Comparing a shifted image with
  retail needs its addresses translated through `addrmap.tsv`.

**`build.py --source-only --relocatable [--shift ADDR:N]`.** It first runs the
normal build and M4 (`LAST_BUILD.json` and `LAST_SOURCE_BUILD.json` are
unchanged in meaning). Then, in the same build directory:

1. It links `source_reloc_e0/` with no shift. The E0 proof must pass, which
   includes the sha check.
2. With `--shift`, it links `source_reloc/` and proves it against E0.
3. It writes `LAST_RELOC_BUILD.json`. This records the image, its sha, the
   shift, t_size, pc0, gp and BSS end, the paths of `addrmap.tsv`,
   `symbols.tsv` and `pointer_slots.tsv`, the E0 image and sha (with
   `e0_matches_source_only`), the proof results, and the source-only and
   hybrid images it came from. It also sets `relocated: true`.

## SDK from the PsyQ libraries (`--sdk-from-lib`)

The PsyQ SDK is out of scope (`docs/RULES.md`): it is to be linked from Sony's
`.LIB` files, which each contributor supplies in `toolchain/psyq-sdk/`
(gitignored). `--sdk-from-lib` does that for every module that
`config/US/sdk_modules.tsv` marks exact in PsyQ 3.3 (`docs/SDK_SOURCES.md`).
It works in both flavours. The C units are not touched: they stay in `src/`
and in the hybrid build, and the plain source-only link still uses them.

```
py -3.14 tools/build_source_only.py --build-dir last --sdk-from-lib --output work/s/F      # fixed
py -3.14 tools/verify_image.py work/s/F
py -3.14 tools/build_source_only.py --build-dir last --sdk-from-lib --relocatable --output work/s/E0
py -3.14 tools/build_source_only.py --build-dir last --sdk-from-lib --relocatable --shift 800D1318:16 --output work/s/E1
py -3.14 tools/verify_reloc.py work/s/E0 && py -3.14 tools/verify_reloc.py work/s/E1 --e0 work/s/E0
py -3.14 build.py --source-only --sdk-from-lib --relocatable --shift 800D1318:16
```

**Conversion (`tools/lnk2elf.py`).** Each LNK module (read with
`tools/psyq_lib.py`) becomes an ELF32 MIPS relocatable object in
`work/sdk_lib/` (gitignored). The file name carries a hash of the converter,
the module bytes and the options, so it is rebuilt only when one of them
changes.

- **Sections:** `.text`, `.rdata`, `.data`, `.sdata`, `.bss`, `.sbss` become
  `.psyq.<name>`; empty ones are left out. Alignment is 4: every LNK section
  says 8, but the PsyQ linker packed TM2's modules at 4 (LIBC `C14` sits at
  80138858).
- **Symbols:** XDEFs become global symbols and LNK locals local ones. Every
  name a patch uses (XREF, XBSS common, `sectstart`/`sectend`) becomes an
  undefined global. XBSS commons are not allocated: the retail link already
  placed them, so they resolve to their retail address like any other import.
- **Patches â†’ relocations:** word32 â†’ `R_MIPS_32`, jump26 â†’ `R_MIPS_26`,
  hi16/lo16 â†’ `R_MIPS_HI16`/`R_MIPS_LO16`, gprel16 â†’ `R_MIPS_GPREL16`.
  - **Addends:** the LNK linker overwrites the field, so the converter writes
    the constant part of the expression as the in-place addend.
  - **HI16/LO16 pairing:** ld pairs a HI16 with the next LO16 of the same
    symbol in the table. The converter orders the table so that every HI16 is
    followed by a LO16 with the same low half, and fails if there is none.
- **Expressions:** the 102 modules use these forms:
  - constants and symbols;
  - `sectbase` of the module's own sections;
  - sums with a constant;
  - `sectstart(.sbss)`, `sectstart(.sdata)` and `sectend(.bss)` (crt0);
  - one difference.

  Anything else (products, sums of two relocatable terms, kind 30
  section-relative patches) stops the conversion with an error.
- **Finding: the subtraction operands are stored reversed.** `- a b` in the
  prefix stream means b âˆ’ a. libcd `ISO9660` +0x6F8 is
  `sectbase(.bss) + 0x600 âˆ’ 0x24` = 80191AE8, which is the retail value.
  `psyq_lib.py` now reads it that way, and `match` reports `ISO9660` as plainly
  exact (before: "2 relocs unresolved").

**Link (`tools/gen_link.py`, `apply_sdk_from_lib`).**

- **Code:** the module's `.text` sits at its retail start (fixed) or in retail
  order (relocatable).
  - **Extent:** its row covers its functions' slots plus the unlisted bytes
    inside the module: crt0's stack table 80138838 (`2MBYTE`), the pad word
    after each 12-byte BIOS stub, and the exception hook at 80140FF4
    (`PATCHGTE`).
  - **Size:** 39,736 text bytes, for 263 functions of 39,468 bytes.
- **Dropped C sections:**
  - **What goes:** the 263 C function sections of those functions, plus 9 C
    data sections that lie inside a linked module data section (for example
    `cd_control.c` `.tm2_80176B18` inside libcd `SYS` `.data`).
  - **Mixed sections:** a C section that holds both library and other
    functions is an error. There is none.
  - **Filtered objects:** the 28 unit objects that lose sections are linked as
    filtered copies (`<out>/sdk_units/`). In each copy, a dropped section is
    emptied and renamed to a discarded name, and its relocations are cleared.
    Its globals become undefined, so `sub_8014510C` and the like now resolve
    to the module.
- **Data:** 34 data sections are linked at the address `sdk_modules.tsv`
  located (`eq` or `bss`).
  - **Externalized:** a section that is located but not equal to retail, or
    that only partly overlaps a kept C section, is not linked. The module
    refers to it as `D_<retail address>`, and its bytes stay with the retail
    piece or C unit.
  - **libetc `INTR` `.rdata` (800D0D00)** is the only externalized section. Its
    rcsid reads `intr.c v1.71`, while retail has `v1.73` (5 words). So the
    retail string stays, and `INTR`'s code points at it.
  - **The 16-byte CD-register `.data` of `CDROM` and `C_002`â€¦`C_010`** (8
    sections, "9 candidates") are left out. No patch refers to them, so they
    cannot be placed and nothing needs them. The 9 retail copies stay retail
    bytes.
- **Names.** The C objects import `sub_<VA>`, `D_<VA>` and IDA names, while
  the modules define Sony names. `symbols.ld` aliases one to the other
  automatically. For example `sub_8014510C = VSync;`, `D_8017E098 = Vcount;`
  and `GsGetWorkBase = GetVideoMode;` (222 aliases of this kind).
  - **The other direction:** every name a module imports gets the value its
    patches hold in retail, read back from the retail words at the module's
    retail address (hi16 paired with its lo16). A name with two different
    retail values is an error.
  - **Fallback:** a name with no such value falls back to the retail names
    (`ida_names.tsv`, `undefined_syms_*`).
  - **Results:** this gives `printf`, `main`, `InitHeap`, the libcd/libspu
    globals (`CD_cbsync`, `_spu_RXX`, ...), the XBSS commons (`StRingBase`,
    `CChannel`, ...) and `CD_com` at the odd address 80176BB9.
  - **crt0:** `sectstart(.sbss)`, `sectstart(.sdata)` and `sectend(.bss)`
    become the labels `D_80180EE4`, `D_801809D4` and `D_801FAD48`, the same
    ones that crt0 in C used.
  - **The entry point:** `start` aliases `__SN_ENTRY_POINT`/`stup2`
    (8013878C).
- **Collisions** are reported as warnings in `report.json`. A name defined
  both by a module and by a kept object is an error; there is none. Two IDA
  names disagree with the library:
  - `AverageSZ3` and `AverageSZ4` are at 80140754/80140764 in libgte `SMP`.
    IDA puts those names on 80140720/80140730.
  - No object imports these names, so nothing is mis-linked.
- **Relocatable:** each module section is a `c` item in retail order.
  - The `code_abs_refs.tsv` branch at 8013FDFC (`gte_extra.c`) now lies inside
    libgte `MSC02`'s single `.text`, so its adjacency ASSERT is not needed.
  - A shift inside a module is refused, as for any C section.

**Coverage** (objects of `work/build-20261005-220917-125178`, hybrid
`dd61030122a9â€¦`):

| | Modules | Functions | Bytes |
|---|---:|---:|---:|
| From the PsyQ 3.3 `.LIB` | 102 | 263 | 39,468 (39,736 with the in-module gaps) |
| Still from C: 3.4-only library code (no exact 3.3/3.5 module) | 36 | 240 | 60,936 |
| Failed | 0 | 0 | 0 |

- **Data from the `.LIB`:** 34 module data sections are linked, 1 is
  externalized (`INTR` `.rdata`) and 8 are left out (CD-register tables).
- **The 3.4-only modules stay C:**
  - libsnd: 25 modules (the GCC 2.5.7 build);
  - libgpu `SYS` and `PRIM`;
  - libcd `BIOS`, `CDREAD` and `CDREAD2`;
  - libspu `SPU` and `S_INI`;
  - libpress `LIBPRESS`;
  - libc `SPRINTF` and `MEMMOVE`;
  - libapi `COUNTER`.

  See `docs/SDK_SOURCES.md`.

**Result.**

- **Fixed:** sha256 `00f341fa9c31e6ca81f6d342ce8b169ab0b025d2a345e167039ef8fe0fe3264d`,
  recorded as `expected_source_only_sdk_lib_sha256` in `BUILD_MANIFEST.json`.
  The links have 1930 C sections, 79 retail pieces and 2 zero-filled tails.
  - **Why it is not `c51f1974â€¦`:** two of the 263 C functions were close, not
    exact:
    - `MDEC_vlc2` (80139F50, libpress `VLC2`): 716 bytes differ;
    - `dma_execute` (8013E724, libcd `C_011`): 135 bytes differ.

    The modules give the retail bytes there.
  - **Against `c51f1974â€¦`:** the image differs in exactly those two slots
    (851 bytes) and nowhere else, data included.
  - **Against retail:** 1350 bytes differ, all inside the 24 remaining close
    functions.
  - **`verify_image.py` checks this:**
    - against the hybrid, a module section may differ only where it equals
      retail (`sdk_lib_retail`, 842 bytes, plus 36 in the 2 C tails);
    - against retail, a module section must not differ at all.
- **Relocatable:** every proof of `verify_reloc.py` passes. The E0 sha check
  uses `expected_source_only_sdk_lib_sha256` when `reloc_plan.json` says
  `sdk_from_lib`.

  | run | shift | sha256 | pc0 | gp | BSS end |
  |-----|-------|--------|-----|----|---------|
  | E0 | none | `00f341fa9c31â€¦` (= fixed), identity map | 8013878C | 801809D4 | 801FAD48 |
  | E1 | 800D1318:16 | `46216150cc17â€¦` | 8013879C | 801809E4 | 801FAD58 |
  | E1b | 801625B8:16 | `826c824bd879â€¦` | 8013878C | 801809E4 | 801FAD58 |
  | SDK block | 8014510C:16 (before libetc `VSYNC`) | `b2e67d18b8d1â€¦` | 8013878C | 801809E4 | 801FAD58 |
  | crt0 | 80138784:16 (before `2MBYTE`) | `b5b87a14e8ceâ€¦` | 8013879C | 801809E4 | 801FAD58 |

  - **Pointer words:** 192 pointer words are generated (200 without the
    option). The 8 data words that only a `.LIB` module relocated now are
    relocations of the module objects.
  - **Negative control:** E1 checked with an identity map still fails.
- **Without `--sdk-from-lib`:** nothing changes. The plain link of the same
  objects is still `c51f1974â€¦`, fixed and E0.

**`build.py --source-only --sdk-from-lib`** runs the normal M4 step
(`source/`, `expected_source_only_sha256`).

- **The extra link:** it also links `source_sdklib/` with the option. That
  link must pass `verify_image.py` and match
  `expected_source_only_sdk_lib_sha256`.
- **`LAST_SOURCE_BUILD.json`** records the extra link under `sdk_from_lib`:
  the image, sha, coverage, the externalized section and the close functions
  that were replaced. `LAST_BUILD.json` still means the hybrid.
- **With `--relocatable`:** E0 and the shifted link use the `.LIB` modules
  too. E0 must equal the `source_sdklib` image, and `LAST_RELOC_BUILD.json`
  gets `sdk_from_lib: true`.

**Per-module override (port variant only; request 34, `SPU_VOICES.md` 4).**

- **The table:** `config/US/sdk_c_modules.tsv` lists, per port define, exact modules whose functions stay the C
  units' while that define is on. Today: `TM2_PORT_SPU48` → all 23 exact libspu modules and libsnd `VS_MONO` /
  `VS_SRV` (30 functions), because the 48-voice libraries are C.
- **How it works:** `build_source_only.py --port-variant --sdk-from-lib` passes the rows of its `-D` defines to
  `gen_link.plan(sdk_keep_c=...)`. `apply_sdk_from_lib` then leaves those modules out, and their C sections are
  linked (recompiled by the variant where the define changes them).
- **Checks:** a listed module that is not exact fails the link, and so does one with a function without a C
  section.
- **Reported:** `report.json` `sdk_from_lib.c_override` / `coverage.c_override_modules`, and
  `port_variant.sdk_c_override`.
- **The proof covers it:** a module's C sections that the define leaves alone are un-relocated against E0, which
  holds the `.LIB` module's bytes. In the placement check (`TM2_SPU_VOICES` forced to 24) every listed module is
  C and must equal E0.
- **Without such a define nothing changes.** That covers the retail links, E0 and the variant without
  `TM2_PORT_SPU48`.

## All-C flavour (`--all-c`)

SingleTrac's hand-written assembly stays asm in `src/` so that the hybrid and
the fixed source-only builds byte-match (`docs/RULES.md`). The all-C flavour is
a relocatable link in which those four units are replaced by their pure-C
copies in `reference/`. The C is larger than some retail slots, which only the
relocatable layout can place. **Status: static only.** It links and passes the
static proofs; it has not been run (comparing it with retail needs an
address map, and more, see below).

```
py -3.14 tools/build_source_only.py --build-dir last --relocatable --output work/r/E0
py -3.14 tools/build_source_only.py --build-dir last --relocatable --all-c --output work/r/C [--shift ADDR:N] [--sdk-from-lib]
py -3.14 tools/verify_reloc.py work/r/C --e0 work/r/E0      # E0 of the same objects (and the same --sdk-from-lib)
py -3.14 build.py --source-only --relocatable --all-c [--shift ADDR:N] [--sdk-from-lib]   # -> LAST_ALLC_BUILD.json
```

**The units** (`ALL_C_UNITS` in `tools/build_source_only.py`):

| src/ unit (asm kept) | reference/ copy (linked by `--all-c`) | replaced functions |
|---|---|---|
| `model_emitter.c` | `model_emitter_800CDC54.c` | `sub_800CDC54` (the model emitter) |
| `engine_hit_detection.c` | `hdp_walker_engine_hit_detection.c` | `hdpWalk` (with the `hdpKind*` handlers) .. `hdpPassRun`, 10 functions |
| `engine_render_scene.c` | `renderer_walker_engine_render_scene.c` | `renderPushMatrix`, `renderWalkModel`, `renderMulTransMatVec` |
| `engine_math_near_pass.c` | `near_pass_engine_math_near_pass.c` | `renderNearPolys`, `renderNearSplitTri`, `renderNearLinkPrims`, `renderNearSplitQuad` |

**Compile.** Each copy is compiled into `<out>/all_c/<name>/` with the
`BUILD_MANIFEST.json` recipe of the unit it replaces (psyq3.6 cc1 via
DOSBox-X, `-fvolatile`, extern gp). Its object takes the unit's place in a
copy of the selection (`<out>/all_c/selection.json`).

**Which sections are replaced.** A section counts as replaced when its bytes or
its relocations differ from the `src/` unit's object. The build fails unless
these are exactly code sections of retail functions; every other section of a
copy must be the same C, so the rest of the image equals the normal link. The
result is exactly the 18 asm sections. To get there, the copies were brought in
line with `src/`: current names from `config/US/names.tsv`, `hdpPush` as in
`src/`, and `hdpKind6/7/10/13` as separate empty handlers, so that
`hdpInitKindTable` is the `src/` C. A reference copy may still use an older
name of a selection function: the build then takes the global the copy defines
at that VA and reports it (`renamed_selection_symbols`).

**Pure C.** The copies follow `docs/RULES.md`:

- The only asm left is GTE access (`lwc2`/`mtc2`/`ctc2`/`cop2`/`mfc2`/`cfc2`/`swc2`,
  with their hazard `nop`s) through each file's GTE macros and blocks.
- The emitter's two non-GTE asm uses are now C:
  - an empty `asm` that kept the scratchpad base in a register is now `S = SCR`;
  - the raw `div` is now `HWDIV()`, which gives the R3000 results without a
    trap: a zero divisor gives âˆ’1 or +1, and 0x80000000 / âˆ’1 gives 0x80000000.

**Link (`gen_link.plan(..., all_c=)`, `reloc_plan`).**

- **Size:** a replaced section may be longer or shorter than its retail slots,
  and it gets no zero-filled tail. Its item still covers the retail slot
  `[va, slot end)`, while the output section has the compiled size, so
  everything after it moves.
- **Asm labels:** the retail asm had labels inside a replaced function. Imports
  of those labels now resolve to the function's entry. These are
  `renderNodeHandler0..2`, `renderNodeSkipHandler` and 10 `D_80152xxx`, which
  `renderInitNodeHandlers` stores in the node-handler table. The C walker
  dispatches with a `switch` and never uses the table.
- **Recorded:** `reloc_plan.json` `all_c` lists:
  - the replaced sections;
  - `retail_symbols`, the retail address of each global of the asm sections
    and of each label;
  - the labels.

**Proof (`verify_reloc.py OUT --e0 E0`).** An all-C link has to be proved against
the normal relocatable E0 of the same objects; without `--e0` it is refused.

- **The usual checks:** `relocs`, `map`, `slots`, `hi16`, `unrelocated`,
  `branches` and `header` all run unchanged.
- **`unrelocate`:** the check skips the retail slots of the 18 replaced
  sections. `reloc_verify.json` lists them in `all_c_excluded`. Every other
  byte of the image must equal E0 once it is un-relocated.
- **Un-relocating by name:** the relocations inside the replaced C are
  checked (each field holds what its record says) but not un-relocated. A
  relocation elsewhere that points into replaced C is un-relocated by name,
  through `retail_symbols`. Examples are `hdpInitKindTable` â†’ `hdpKind1`, and
  `renderInitNodeHandlers` â†’ `renderNodeHandler0`, which is now the walker's
  entry.
- **In the address map:** the replaced functions are `code_entry` rows (only
  their entry maps), next to the close functions.

**Result** (objects of `work/build-20261006-065011-264073`, hybrid
`0a6911abd57fâ€¦`; E0 `798daa204dceâ€¦`, with `--sdk-from-lib` `9728340a4b60â€¦`):

| link | sha256 | t_size | pc0 | gp | BSS end | code_entry | proof vs E0 |
|------|--------|--------|-----|----|---------|-----------:|-------------|
| all-C | `4c4e73a904caâ€¦` | 0xB4000 | 80138808 | 80180FA0 | 801FB314 | 18 + 18 | all checks pass |
| all-C `--shift 800D1318:16` | `609241ea4268â€¦` | 0xB4000 | 80138818 | 80180FB0 | 801FB324 | 18 + 18 | all checks pass |
| all-C `--sdk-from-lib` | `e7482c47f679â€¦` | 0xB4000 | 80138808 | 80180FA0 | 801FB314 | 16 + 18 | all checks pass |

- **`build.py --source-only --sdk-from-lib --relocatable --shift 800D1318:16 --all-c`**
  (`work/build-20261006-071716-778301`) passes end to end. The all-C link
  (`source_reloc_allc/`, sha `fa0b8ad43581â€¦`, pc0 80138818, gp 80180FB0,
  BSS end 801FB324) is proved against `source_reloc_e0/` (`9728340a4b60â€¦`).
  It is recorded in `LAST_ALLC_BUILD.json`.
- **Growth:** the image grows by 0x5CC bytes. The emitter at `t_addr` already
  pushes everything else by 0x7C, rodata included. t_size grows by one
  0x800 page.
- **Negative controls:**
  - the all-C link proved with an identity map fails `header`;
  - with `--corrupt-slot 80162858` it fails `slots` and `unrelocated`.

**Sizes, C against the retail slot** (bytes):

| function | slot | C | Î” |
|---|---:|---:|---:|
| `sub_800CDC54` (emitter) | 0xCBC | 0xD38 | +124 |
| `hdpWalk` (+ `hdpKind*`) | 0x4DC | 0x7D4 | +760 |
| `hdpWalkPush` | 0x5C | 0x8C | +48 |
| `hdpRotVecSplit` | 0x134 | 0xE8 | âˆ’76 |
| `hdpRotTransSVector` | 0x9C | 0x88 | âˆ’20 |
| `hdpLeafTestPoints` | 0x550 | 0x654 | +260 |
| `hdpApplyParentTransforms` | 0x98 | 0x120 | +136 |
| `hdpLeafTestPointsMark` | 0x178 | 0x170 | âˆ’8 |
| `hdpLeafTestSingleBox` | 0x100 | 0x5C | âˆ’164 |
| `hdpBoxTestModel` (+ `hdpBox`) | 0xE8 | 0x1E8 | +256 |
| `hdpPassRun` | 0x28C | 0x2DC | +80 |
| `renderPushMatrix` | 0x68 | 0x88 | +32 |
| `renderWalkModel` | 0xCC0 | 0xAC0 | âˆ’512 |
| `renderMulTransMatVec` | 0xD4 | 0x128 | +84 |
| `renderNearPolys` | 0x7A0 | 0x8B8 | +280 |
| `renderNearSplitTri` | 0x54 | 0xA8 | +84 |
| `renderNearLinkPrims` | 0x204 | 0x220 | +28 |
| `renderNearSplitQuad` | 0x84 | 0xE0 | +92 |
| **total** | 14352 | 15836 | +1484 |

**ABI.** Each copy's header comment has the details.

- **Entry points from other code:**
  - `sub_800CDC54` and `renderWalkModel` are called by compiled C
    (`renderScene`, `renderDrawModel`);
  - so is `renderNearPolys`;
  - `hdpPassRun` is called by `hdpCollideObject`.

  All of these callers pass standard o32 arguments (the emitter's 5th
  argument goes at 16($sp)), which is also what the asm reads. They also set
  the scratchpad state that the routines read (the emitter's 0x40, 0x4C,
  0x60, 0x6C and 0x88), and the C reads and writes the same fields.
- **Internal subroutines:** the asm gave them private register conventions.
  In the C they are ordinary functions.
- **Differences a comparison with retail will see:**
  - **Scratchpad save areas:** the asm saved registers into the scratchpad,
    and the C keeps them on its stack. The areas are:
    - emitter: 0x00..0x2B and 0x8C..0x9F;
    - near pass: 0x00..0x53, 0x58..0x5B and 0x78..0x8B (request 82's check: besides its saves, the
      emitter's and the renderer's fields there, each set again before it is read);
    - walker: 0x40 and 0x4C..0x7B;
    - hdp: 0x08 and 0x6C..0xA3.
  - **Handler tables in BSS:** `renderNodeHandlers` holds C addresses, here
    the walker entry. `hdpKindHandlers` holds the C handlers, which have no
    retail address.
  - **Caller-saved registers:** the asm preserved t8/t9 and a2/a3, which the
    ABI does not require. The C does not.
- **Behaviour:**
  - the renderer walker and the near pass match the retail asm on random
    inputs;
  - the hdp walker and the emitter have not been run.

**Not done / limits.**

- **No fixed (non-relocatable) all-C image:** the C does not fit the retail
  slots.
- **Comparing it with retail needs more than an address map:**
  - mask the scratchpad save areas above;
  - mask the two handler tables;
  - accept `code_entry` rows whose interior never maps.
- **`LAST_BUILD.json`, `LAST_SOURCE_BUILD.json` and `LAST_RELOC_BUILD.json`
  are unchanged in meaning.** The all-C link is recorded only in
  `LAST_ALLC_BUILD.json`, which has `all_c: true`.

## For the port: stable build and maps

The PC port (`docs/DECOMP_REQUESTS.md` item 25 in the port repo) recompiles the
decomp's source-only build at retail's layout and runs it beside retail. It
needs that image at a fixed path after every build, with hashes, the list of
places where it differs from retail, and function and jump-table lists that its
recompiler (`tools/recomp`) can read without IDA.

**What `build.py --source-only` adds.** After the fixed-layout link and its
verify (M4 above), every `--source-only` build also:

1. links the zero-shift relocatable **E0** of the same objects
   (`source_reloc_e0/`, with the `.LIB` modules when `--sdk-from-lib` is given)
   and proves it with `tools/verify_reloc.py`;
2. checks that E0 is **byte-identical** to the fixed-layout image of the same
   flavour (`source_sdklib/TM2_SOURCE.EXE` with `--sdk-from-lib`,
   `source/TM2_SOURCE.EXE` without). Both are the same file, so it does not
   matter which one the port takes. E0 is the one it ran (build
   `20261006-071716-778301`, sha `9728340aâ€¦`, equal to that build's
   `source_sdklib` image). E0 is published because only the relocatable link
   writes the maps;
3. publishes to `out/` (replaced in place on every build; each file is
   written to a `.tmp` file and renamed, so a reader never sees a partial
   file):

| path | contents |
|---|---|
| `out/TM2_SOURCE_E0.EXE` | the E0 image |
| `out/TM2_SOURCE_E0.json` | hashes, build, git commit, the differences from retail (schema below) |
| `out/TM2_SOURCE_E0/addrmap.tsv`, `symbols.tsv`, `pointer_slots.tsv` | E0's maps (Relocatable link, above). For E0 the address map is the identity |
| `out/TM2_SOURCE_E0/functions.tsv`, `jumptables.tsv` | function and jump-table lists (below) |
| `out/TM2_SOURCE_E0/functions_ida.tsv`, `names_ida.tsv`, `recomp_inventory.json` | the same lists in the port recompiler's input formats (below) |

`LAST_SOURCE_BUILD.json` gets an `e0` entry (image, sha, `identical_to_fixed_layout`,
the published paths, the difference counts).

**`out/` is gitignored and must stay that way.** The source-only image is
not made only from C. Every byte that no C or `.LIB` section owns is a retail
piece `.incbin`-ed from `split/` (the retail rodata, data and sdata, the text
gaps, the header). That is copyrighted retail data. The same holds for the
`work/` copies.

**Every relocatable link writes the maps.**
`tools/build_source_only.py --relocatable` (and so `build.py`'s E0, `--shift`
and `--all-c` links) calls `tools/function_maps.py` after
`tools/reloc_image.py`. Next to `addrmap.tsv` it writes the five files below.
Their summary goes into `report.json` (`maps.function_maps`), and
`LAST_RELOC_BUILD.json` and `LAST_ALLC_BUILD.json` name `functions.tsv` and
`jumptables.tsv`. Run `py -3.14 tools/function_maps.py LINK_DIR` to write them
for an older link directory.

- **`functions.tsv`** has one row per retail function (ida_funcs + extra_funcs
  âˆ’ not_funcs: 1948), in build order. Columns:
  - `build_va`, `size`: in this image. The size includes the zero-filled slot
    tail of a C function shorter than its retail slot.
  - `retail_va`, `retail_size`: the retail slot.
  - `name`: the linked symbol (the C name for C functions, the PsyQ export
    for `.LIB` modules). `ida_name` is the retail IDA name.
  - `source`: `src/...` (or the `reference/` copy in an all-C link), or
    `psyq LIB:MODULE`.
  - `kind`: `game` or `sdk` (in `config/US/sdk_modules.tsv`, a `.LIB`
    module, or an `sdk_*` unit).
  - `form`: `c`, `cpp`, `asm` (a file-scope `__asm__` `.text_<VA>` section in
    `src/`, hand-written SingleTrac or PsyQ asm), `lib` (a PsyQ `.LIB`
    module) or `retail` (retail bytes; none in the current builds).
  - `identical`: 1 when the un-relocated bytes equal retail over the whole
    slot (the `code_linear` rows of `addrmap.tsv`), 0 for close functions and
    replaced (all-C) ones.
- **`jumptables.tsv`** has one row per switch table. Columns:
  - `table`, `site`: the build addresses of the table and of the `jr` that uses
    it.
  - `retail_table`, `retail_site`: their retail addresses through `c2r`. A
    column is empty when there is no retail address, which can happen inside a
    resized function.
  - `entries`; the owning function (`function_va`, `function_retail_va`,
    `function`).
  - `targets`: the case list `index:build_target`.

  Tables are found the way `tools/pointer_census.py` finds them in retail: a
  `lw` from a `lui`/`addiu` base indexed by `addu`, then a `jr` through the
  loaded word. A table extends while its words point into the owning function
  and no other table starts.
- **`functions_ida.tsv`** (`va words name`, the format of
  `config/US/ida_funcs.tsv`) and **`names_ida.tsv`** (`va code name`, the
  format of `ida_names.tsv`: `symbols.tsv` without the retail `jpt_*` names,
  plus one `jpt_<build jr site>` per table at the table's build address).
  These are the `--funcs` and `--names` inputs of the port's
  `tools/recomp/inventory.py`.
- **`recomp_inventory.json`**: the part of the port's
  `docs/recomp_inventory.json` that `tools/recomp/recompile.py` reads:
  - `functions_table`: `addr`, `name`, `size`, plus `retail_addr`, `kind`,
    `form`, `identical`;
  - `jump_tables`: `site`, `function`, `table`, `entries`;
  - `unresolved_jr`: the `jr` sites with no static table, with
    `runtime_table_targets`, the code constants (lui/addiu, from any function)
    inside the owning function.

  `jr $t2` BIOS stubs (`$t2` = A0/B0/C0) are not listed, as in the port's
  inventory.

**The port's recompiler formats** (read from the port repo's `tools/recomp`,
not changed):

- **`recompile.py --exe EXE --inventory JSON`** needs only `functions_table`
  (`addr`, `size`), `jump_tables` (`site`, `table`, `entries`; it reads the
  targets from the EXE) and `unresolved_jr` (`site`, `runtime_table_targets`).
  `recomp_inventory.json` can be passed directly.
- **`inventory.py --exe --funcs --names --json`** builds the full inventory
  (calls, BIOS wrappers, GTE use, determinism) from an IDA-style function list
  and name list. `functions_ida.tsv` and `names_ida.tsv` are those lists for
  the build.
- **Checked on the 2026-10-06 objects:**
  - On E0 and on the `--shift 800D1318:16` link, `inventory.py` with these
    lists finds the same 50 tables (631 entries) and the same 2 run-time-table
    `jr` sites (`hdpWalk`, `renderWalkModel`, 10 targets each) as
    `recomp_inventory.json`.
  - On E0 these are also the retail inventory's tables.
  - On the shifted link the rows map back to E0 exactly through `c2r`.
  - `recompile.py` gives identical C from either inventory, with 0
    untranslated functions.
- **One port-side caveat:** `recompile.py` hard-codes
  `EXTRA_FUNCTIONS = {0x8015FCA0: 0x8015FCE4}`, a retail address. The
  function is already in `functions.tsv` (from `config/US/extra_funcs.tsv`).
  On a shifted build that constant adds a bogus function at the old retail
  address, so the port should drop it when it reads these lists.

**`out/TM2_SOURCE_E0.json` (schema `tm2-source-e0-v1`).**

| key | meaning |
|---|---|
| `executable`, `sha256`, `size` | `out/TM2_SOURCE_E0.EXE` |
| `flavour`, `sdk_from_lib`, `layout` | `nonmatching [+ sdk_from_lib]`; retail layout (identity map) |
| `built_utc`, `build_dir`, `e0_executable`, `e0_proof` | the build that produced it and the `verify_reloc.py` result |
| `fixed_layout_executable`, `fixed_layout_sha256`, `identical_to_fixed_layout` | the fixed-layout image it equals (always true; the build fails otherwise) |
| `expected_sha256`, `matches_expected` | `BUILD_MANIFEST.json` `expected_source_only[_sdk_lib]_sha256` |
| `hybrid_executable`, `hybrid_sha256` | the hybrid of the same build |
| `retail_executable`, `retail_sha256`, `matches_retail` | `bin/SCUS_94306R.EXE` |
| `git` | `commit`, `branch`, `dirty` (tracked files differed from the commit) |
| `maps` | the paths of the eight map files |
| `functions` | `total`, `identical`, `by_form`, `sdk` |
| `jump_tables` | `tables`, `entries`, `unresolved_jr` (site, function, run-time targets) |
| `differences` | totals and the per-owner list (below) |
| `contains_retail_bytes` | true (see above) |

`differences` comes from `tools/verify_image.py`'s `vs_retail` runs of the
fixed-layout image, which has the same bytes as E0. Each run is attributed with
`functions.tsv` or the data rows of `addrmap.tsv`.

- **Totals:**
  - `bytes`: the number of bytes that differ.
  - `runs`: the number of contiguous differing byte runs.
  - `places`: the number of runs left after merging runs less than
    `place_gap` = 16 bytes apart within one owner. This is how the port
    counted "48 places".
  - `owners`: the number of functions and data items that differ.
  - `by_kind`: the byte count per kind.
- **`functions`**: one entry per owner with these fields:
  - `va`, `end`: the owner's retail range (E0 = build);
  - `name`, `ida_name`, `source`;
  - `kind`: `close C`, `asm`, `lib`, `retail`, `data` or `header`;
  - `verify_class`: `close_function` or `placed_data`;
  - `bytes`, `runs`;
  - `places`: a list of `start`, `end`, `bytes`, `runs`.

## Port variant (`--port-variant`)

The PC port's request 25, second bullet: a build in which the two-player limits
are constants in one header, the arrays they size have grown to 4 and live in a
section of their own above 2 MB, and the loops and "other player" tests of
`FOUR_HUMANS.md` section 2 run for N players. **The retail builds do not change**:
the hybrid, the source-only images and E0 keep their hashes. **Status: static
only.** The variant links and passes the static proofs below; it has not been run.

```
py -3.14 build.py --source-only --sdk-from-lib --relocatable --port-variant [--port-base 80200000]
py -3.14 tools/build_source_only.py --build-dir last --relocatable --sdk-from-lib --port-variant --output work/pv/P4
py -3.14 tools/build_source_only.py --build-dir last --relocatable --sdk-from-lib --port-variant --port-limits 2 --output work/pv/P2
py -3.14 tools/verify_reloc.py work/pv/P4 --e0 work/build-<timestamp>/source_reloc_e0    # E0 of the same objects and flags
```

The PC port's own changes and code go in with `--port-define NAME[=VALUE]` and
`--port-unit PATH.c` (both repeatable, on both tools; see "Port code" below).

### The limits (`include/semantic/players.h`)

| constant | retail | port variant | sizes |
|---|---:|---:|---|
| `TM2_MAX_HUMANS` | 2 | 4 | the human records and their per-index arrays, the settings port table |
| `TM2_MAX_VIEWS` | 2 | 4 | the per-view arrays (`FOUR_PLAYERS.md` 4.4) |
| `TM2_MAX_HUDS` | 2 | 4 | the per-HUD arrays (`FOUR_PLAYERS.md` 5.1) |
| `TM2_MAX_VIEW_MODES` | 9 | 16 | the rows of `viewModeTable` (request 26) |
| `TM2_SPU_VOICES` (`spu_voices.h`) | 24 | 24; 48 with `--port-define TM2_PORT_SPU48` | the sound libraries' voice tables and the driver's `soundVoiceInUse` (request 34, `SPU_VOICES.md`) |

`TM2_RETAIL_HUMANS/VIEWS/HUDS` (2) and `TM2_RETAIL_VIEW_MODES` (9) name what the retail storage holds.
`-DTM2_PORT_VARIANT` selects 4 (16 view modes); any of them may also be predefined
(`--port-limits 2` forces all three to 2 and `TM2_MAX_VIEW_MODES` to 9, see the placement check).

Where only a bound changes (`i < 2` â†’ `i < TM2_MAX_VIEWS`, `humanCars[2]` â†’
`humanCars[TM2_MAX_HUMANS]`), the constant expands to the retail C. Where the
code changes shape it is written under `#if TM2_MAX_HUMANS > 2` (or `VIEWS`,
`HUDS`), and the `#else` (or the code after the `#endif`) is the retail C
unchanged. Where it matters at run time, the N-player path is taken only with
more than two humans (`if (humanCarTotal > 2)`, `if (s->humans > 2)`). So with one
or two humans the variant follows retail's game logic, `rand()` calls included.
Two things differ: the array addresses, and the setup loops, which also set the
entries of players, views and HUDs 3-4.

### What moves (`config/US/port_variant_arrays.tsv`)

Every array of `FOUR_HUMANS.md` 1.1 / section 7 and of `FOUR_PLAYERS.md` 4.4 and
5.1 that is indexed by a human, view or HUD, 29 in all, plus the view-mode table
(request 26) and one new table. They
are defined, sized by the limits, in `src/semantic/port_variant_arrays.c`, which only
the variant compiles:

| group | arrays | retail | variant |
|---|---|---|---|
| humans | `humanCars`, `humanCarObjects`, `humanCarObjectSlots`, `padConfigs`, `humanCarTypes`, `hudSmoothedSpeeds`, `carViewHotkeyReleased` | `[2]` | `[4]` |
| views | `viewCameraObjectTable`, `viewFollowObjects`, `viewCameraObjects`, `viewEnvs`, `viewModes`, `viewEyePositions`, `viewMatrices` | `[2]` | `[4]` |
| HUDs | `hudMarkerBlinkCounters`, `hudMarkerBlinkPhases`, `hudOrderTables[2][N][2]`, `hudPlayerRecords`, `hudRadars`, `hudRadarArrow{Up,Down,Left,Right}Prims`, `hudTargetNameModes`, `hudTargetNameSprites`, `hudRadarArrow{Up,Down,Left,Right}Sprites` | `[2]` | `[4]` |
| view modes | `viewModeTable` (96-byte rows) | `[9]` | `[16]`: rows 0-8 retail, 9-15 zero |
| settings | `sessionPortHumanCars` (new) | none | `[TM2_MAX_HUMANS - 2]` |

- **Sections.**
  - `.port_data`: initialised. `carViewHotkeyReleased` (1 per entry, as
    retail's sdata `{1, 1}`) and `viewModeTable` (retail's 9 rows word for word,
    then zero rows; the table holds no pointers) are in it.
  - `.port_bss`: everything else, zero. Retail's sdata arrays that start at zero
    (`viewFollowObjects`, the two blink arrays) go here too.
- **Placement.** The relocatable link puts `.port_data` at the port base
  (`--port-base`, default `0x80200000`; the port has 8 MB) and `.port_bss` after
  it, 16-aligned, then the port's code, `.port_text` and `.port_rodata` ("Port
  code" below). All are outside the PS-X EXE: `elf_to_psx_exe.py` leaves out
  every section at or above the base.
- **What the port must do.**
  - copy `out/TM2_SOURCE_PORT/port_data.bin` to the base before the game runs
    (and `port_rodata.bin` to its address, when it is not empty);
  - zero `.port_bss` (address and size in the json). **crt0 clears only the
    retail BSS** (`D_80180EE4`..`D_801FAD48`).
- **Group `one`.** A row with group `one` is a fixed-size block: one entry of
  `entry_bytes`, whatever the limits (`retail_va` `-`, `retail_entries` 0,
  layout `new`). For the port's own storage, e.g. its 64-byte settings block
  (`portConfig`, defined by the port in `.port_bss`).
- **The retail storage stays** where it was, unused: plain retail bytes (BSS
  zeros, the sdata words) in the link. Only one built unit defined any of it
  (`car_object_lookup.c`, `humanCarObjects`). In the variant it declares the
  array instead.
- **Not moved, room for 4 already:** `carResetWeapons`, `hudRadarVisible`,
  `hudWeaponListVisible` and `padLayout` are four bytes each. Bytes 2-3 of
  `hudRadarVisible` start at 0 (retail sdata `{1, 1, 0, 0}`), so the radars of
  HUDs 2-3 start hidden.
- **Not moved, dead:** `D_80180FD8` / `D_80180FE0` (`FOUR_HUMANS.md` 1.1). The
  `uaInit` loop that clears them stays at 2.
- **Not moved, the port's side:** the frame records, frame slots, OTs, packet
  buffers (rendering buffers for more views), and the pad
  arrays (`padActiveBindings`, `padConfiguredBindings`, `padActionCache`: the
  port reads pads 3-4 itself).

**Absolute addressing.** $gp reaches only 801789D4..801889D3. In retail, five
functions reach a moved array through $gp: `uaInit` (`humanCarTypes`,
`viewCameraObjectTable`), `carTablesClear` (`viewCameraObjectTable`),
`viewSetFollowObj` (`viewFollowObjects`), `viewProc` (`viewCameraObjects`) and
`viewGetDisplaySize` (`a80181120` = `viewModes[0]`). Under `-DTM2_PORT_VARIANT`,
`players.h` emits `#pragma tm2_absolute` for every moved name (and
`a80181120`), so maspsx addresses them with `lui`/`%lo`. 26 of the 28 units
that refer to a moved array include `players.h` (directly, or through `cars.h`
or `settings.h`). The other two, `arena_setup.c` (`humanCars[]`) and
`engine_math_near_pass.c` (`viewModes[]`), declare the array without a size,
which maspsx never addresses through $gp. A gp-relative access to the port base
could not link anyway (`R_MIPS_GPREL16` overflow).

### What the C does with N players

| function | change |
|---|---|
| `uaInit` | records, ids and reset flags for `TM2_MAX_HUMANS`; `PadSetConfig(1, i)` for players 3-4; `viewCameraObjectTable` cleared for every view |
| `carTablesClear` | `humanCars[i].f_810` for every record (the count-down starts at `(N - 1) * 0x814`); `viewCameraObjectTable` for every view |
| `roundStart` | `carSetResetWeapons(0, i)` for players 3-4 too |
| `worldUpdate` | more than two humans: human i on the radar of every other human j (`hudRadarPlotCar(.., j)`, z test of the pair); co-op (`gameMode == 2`): the AI blips on radars 3-4 too. Two or fewer: retail |
| `sessionRun` | every reset-weapons pair also sets players 3..N (`TM2_RESET_WEAPONS_REST`); after a death in a co-op tournament with more than two humans, every dead human's weapons are reset (two or fewer: retail's "player 1 dead, else player 2"); the Minion intro also checks cars 2..N |
| `settingsGetHumanCar`, `sessionGetHumanCar`, `settingsSetHumanCar` | players 3-4 read and write `sessionPortHumanCars` (`TM2_SETTINGS_HUMAN_CAR` in `settings.h`), so `opponent_car[0..1]` are no longer aliased (`FOUR_HUMANS.md` 5.2, "a port field"). Through them `carIsSelectable`, `screenCarSelect` and `screenEnding` work for 4 |
| `settingsClearHumanCars` | also clears the port table to -1 |
| `sessionLoadArenaAndCars`, `sessionBindCars` | the TMS/DMD files and the binding of human i take its car from `TM2_SETTINGS_HUMAN_CAR` |
| `viewInit`, `viewCycleMode`, `viewSetBgColor`, `viewSetFov` | loop over `TM2_MAX_VIEWS` (unset views are skipped, as in retail) |
| `viewCreate` | refuses `i >= TM2_MAX_VIEWS` |
| `hudInitPanels`, `hudRadarInit` | initialise every HUD record and radar (the split-mode layouts of retail; quarter layouts are the port's) |
| `hudRadarArrowsLoad` | parses the arrow image into each HUD's copy of each of the four sprite arrays (retail reaches them as one `[4][2]` block) |
| `hudDrawSingleView`, `hudDrawSplitVertical`, `hudDrawSplitHorizontal` | only the `hudOrderTables[buffer][player]` stride |
| `viewProc`, `viewSetFollowObj`, `viewGetDisplaySize` | only $gp â†’ absolute |

**Not in the variant (the port's side).** This build covers data structures and
loops only:
- **Pads 3-4.** `padCheckAction(.., 2)` is "either pad", and port 3 reads nothing
  (`FOUR_HUMANS.md` 3). No `padSetLayout` for ports 2-3; no layouts or lives in
  settings for players 3-4.
- **Views 3-4.** No view creation, no quarter view modes (`viewModeTable` has
  room for them since request 26, rows 9-15 zero: the port fills them), no
  frame records or OTs for them. Nor any of the rest of the view side:
  - `rtMainLoop` draws two views;
  - `viewChaseCamera` and the HUD switches have no split mode 3;
  - `viewSetFollowObj` makes only the sky copy for view 1;
  - `viewProc`'s mode 4 is views 0 â†’ 1;
  - `carViewHotkeys` still toggles split 1 â†” 2.
- **HUD drawing.** No quarter HUD layout or drawing; `UAdashInitDashboardIcons`
  still sets up records 0-1 only.
- **4-player rules.**
  - the round-won test still reads player 1 only;
  - deathmatch results 3/4 and the win tallies are two-sided;
  - the runner decrements `lives[0]` only;
  - `weaponFindTargetAhead/Behind` aim at player 1.
- **Sound.** The per-player sound loops know sides 0-1 only. A turbo or gun loop
  by player 3-4 leaks one pool object per frame (`FOUR_HUMANS.md` 3); the port
  must route those sides.

### How the build makes it

- **Which units are recompiled.** `tools/build_source_only.py --port-variant`
  (`prepare_port_variant`) preprocesses every unit of the build twice, without
  and with the defines (`semantic_compile.preprocess`, the same cpp flags as
  `compile_source(..., defines=)`). A unit is recompiled with its
  `BUILD_MANIFEST.json` recipe only when the two outputs differ. Today that is
  109 units, because `cars.h` and `settings.h` include `players.h`.
- **Checks on each recompiled object** (against the build's object):
  - **Changed code section:** a code section whose bytes or relocations differ
    is a function the variant changes. It is linked like an all-C section, with
    its compiled size and no slot tail, so everything after it moves.
  - **Lost data section:** allowed only if it holds nothing but moved arrays
    (`humanCarObjects` in `car_object_lookup.c`).
  - **Changed data section:** allowed only if it is a jump table whose words all
    point into a changed function (`sessionRun`'s `.tm2_800CFCA8`). It is
    marked `excluded`.
  - **Anything else** stops the build.
- **The port unit.** `port_variant_arrays.c` is compiled as plain data (GCC
  2.7.2 path, no $gp) and added to the selection
  (`port_variant/selection.json`).
- **`gen_link.plan(port=...)`:**
  - collects `.port_data`/`.port_bss` instead of placing them;
  - checks every array of the table against the object: section, size
    `N Ã— entry_bytes`, and the order of the four sprite arrays when the HUDs
    are 2;
  - resolves the moved names by the linker;
  - redirects any other import inside a moved array's retail storage to the
    new storage (`a80181120 = viewModes`);
  - refuses a unit section that still overlaps that storage.

  `reloc_linker_script` adds `. = base; .port_data . : {..}
  .port_bss ALIGN(16) (NOLOAD) : {..}`, plus ASSERTs: each array at its
  offset, the image below the base, the port area below 0x80800000.
- **Address map** (`reloc_image.py`). Rows outside the tiling, which `c2r`/`r2c`
  look up first:
  - `port`: retail entries â†’ the first entries of the new array;
  - `port_new`: the grown entries, candidate only;
  - `port_relayout`: only the start maps; this is `hudOrderTables`, whose HUD
    count is the middle dimension.

  `r2c` of a retail address in a moved array gives its new home, so the port's
  retail-VA hooks and `symbols.tsv` translate. `sessionPortHumanCars` has no
  retail address and is in `symbols.tsv` and the json only.
- **The stack** (PC port request 89, `ONLINE_ROLLBACK.md` 25). The variant's
  BSS ends higher than retail's, so crt0's sp of 0x80200000 left only about
  6 KB above it. `elf_to_psx_exe.convert(stack=('portStack', bytes))` sets
  crt0's stack word (2MBYTE's table at `__SN_ENTRY_POINT + 0xB0`, retail
  0x00200000) and the header's `s_addr` to the end of `portStack` (64 KB,
  last in .port_bss), and records both in `reloc_plan.json`
  `port_variant.stack`. `verify_reloc` puts retail's word and header back
  before it compares with E0.

### Proofs (static)

1. **The variant against E0** (`verify_reloc.py source_reloc_port --e0
   source_reloc_e0`). All the usual checks run: relocs, map, unrelocate, slots,
   hi16, unrelocated, branches and header.
   - **Excluded from `unrelocate`:** the 25 changed functions and the one jump
     table. They are listed in `reloc_verify.json`, as in the all-C proof.
   - **Moved arrays:** every reference to one, anywhere else in the image,
     un-relocates through the `port` rows to its retail address. The image
     must then equal E0 byte for byte.
   - **The excluded jump table:** its `data_relocs` slots must be `R_MIPS_32`
     relocations into a changed function. They are not compared with r2c of
     the retail value.
2. **Placement check** (`--port-limits 2`, `source_reloc_port2/`). The variant
   with all three limits forced to 2:
   - the arrays move, at their retail sizes; every `#if ... > 2` path is off;
   - the image must un-relocate to E0 everywhere except the functions that
     reached a moved array through $gp;
   - `build.py` requires that set to equal the `gprel16` rows of
     `config/US/retail_relocs.tsv` that target a moved array: `uaInit`,
     `carTablesClear`, `viewGetDisplaySize`, `viewProc`, `viewSetFollowObj`.
     With `--port-define`, the functions the port's defines change (and their
     jump tables) are accepted too; they are listed apart
     (`port_changed_functions`).

   So the placement is sound: no reference to a moved array is missed, and no
   other byte changes. The excluded functions differ from E0 only in the
   absolute addressing (each grows by 8-40 bytes).
3. **Negative control.** Swapping the `port` rows of `humanCarObjects` and
   `humanCarObjectSlots` in the placement check's `addrmap.tsv` fails
   `unrelocate` with 11 differences. The first is `8011A458: un-relocated
   f40f4224, E0 ec0f4224 (relocation R_MIPS_LO16 -> 80180FF4)` in
   `carBindHumanSlot`.

### Outputs

`build.py --port-variant` publishes, beside the E0 files (same atomic writes,
gitignored, retail bytes inside):

| path | contents |
|---|---|
| `out/TM2_SOURCE_PORT.EXE` | the variant image (relocatable link with `--sdk-from-lib` when given) |
| `out/TM2_SOURCE_PORT.json` | schema `tm2-source-port-v1`, below |
| `out/TM2_SOURCE_PORT/addrmap.tsv`, `symbols.tsv`, `pointer_slots.tsv`, `functions.tsv`, `jumptables.tsv`, `functions_ida.tsv`, `names_ida.tsv`, `recomp_inventory.json` | the maps, as for E0 (the address map is not the identity) |
| `out/TM2_SOURCE_PORT/port_data.bin` | the initial bytes of `.port_data` |
| `out/TM2_SOURCE_PORT/port_text.bin`, `port_rodata.bin` | the bytes of `.port_text` / `.port_rodata` ("Port code"); always written, 0 bytes without port code |
| `out/TM2_SOURCE_PORT/port_arrays.tsv` | every port array: build address, size, entries, retail address and size, layout |

`LAST_PORT_BUILD.json` records the same as the json. `LAST_BUILD.json`,
`LAST_SOURCE_BUILD.json` and `LAST_RELOC_BUILD.json` are unchanged in meaning.

**`out/TM2_SOURCE_PORT.json`**

| key | meaning |
|---|---|
| `executable`, `sha256`, `size`, `t_size`, `pc0`, `gp`, `bss_end` | the image |
| `defines`, `limits` | `TM2_PORT_VARIANT`; the limits read back from `players.h` |
| `port_base`, `port_sections` | `.port_data` / `.port_bss` / `.port_text` / `.port_rodata`, always all four in link order: address, size (0: empty, at the address it would start at), NOLOAD or not, and who must fill it |
| `port_ram_end`, `port_end` | the port RAM end of the link (`--port-ram-end`, default 0x80C00000; "Port code") and the end of the port sections (at or below it) |
| `port_defines`, `port_units` | the port's `--port-define` / `--port-unit` (empty without them) |
| `port_functions`, `port_jump_tables` | the functions in `.port_text` (name, address, size, unit, static) and their jump tables |
| `port_arrays` | the arrays (as `port_arrays.tsv`, with offsets in their section) |
| `redirects` | imports redirected into a moved array (`a80181120`) |
| `changed_functions` | the functions that differ from E0 because of the variant: retail and build VA, sizes, unit, `why`, `changed_with_limits_2`, `port_changed` (changed by the port's defines) |
| `changed_jump_tables` | jump tables that changed with their function |
| `proof`, `placement_check` | the two proofs above (E0 sha, excluded ranges, the expected and found $gp functions) |
| `functions`, `jump_tables`, `maps` | as for E0 |
| `e0` | the E0 files; E0's own differences from retail (the close functions) are in `TM2_SOURCE_E0.json` and are in this build too |
| `scope` | what the variant is (data structures and loops) |

### Result

Build `work/build-20261008-173907-098903`
(`build.py --source-only --sdk-from-lib --relocatable --port-variant`). The retail
images are unchanged: hybrid `a270438ec719â€¦`, source-only `e033d7d14ba8â€¦`,
`source_sdklib` = E0 `cab53c296b12â€¦`. `tools/data_audit.py` reports 0
mismatches.

| link | sha256 | t_size | pc0 | gp | BSS end | port sections | proof vs E0 |
|---|---|---|---|---|---|---|---|
| variant (`source_reloc_port`) | `98554bddbedbâ€¦` | 0xB4000 | 80138EDC | 80181160 | 801FB4D4 | `.port_data` 80200000 +0x10, `.port_bss` 80200010 +0x5984 (NOLOAD) | all checks pass; 25 functions + 1 jump table excluded |
| limits 2 (`source_reloc_port2`) | `e478c3cf52e9â€¦` | 0xB3800 | 801387A0 | 80180A24 | 801FAD98 | `.port_data` +0x8, `.port_bss` +0x2CC0 | all checks pass; exactly the 5 $gp functions excluded |

- **Recompiled:** 110 units (109 of the build, whose preprocessed source
  changes with the define, plus `port_variant_arrays.c`).
- **The image:** 1,948 functions, 50 jump tables with 631 entries (the same as
  E0); 1,909 functions identical to retail.
- **Lost section:** one, `car_object_lookup.c` `.tm2_80180FEC`
  (`humanCarObjects`).
- **Redirect:** one, `a80181120` â†’ `viewModes`.
- **`port_data.bin`:** 16 bytes, `carViewHotkeyReleased` = 1, 1, 1, 1.

**The functions that differ from E0** (25; `*` = also changed with the limits
forced to 2, by the $gp â†’ absolute addressing). Build VAs are in the variant
image; everything after `hudRadarArrowsLoad` moves.

| function | retail VA | build VA | retail bytes | variant bytes | unit |
|---|---|---|---:|---:|---|
| `hudDrawSingleView` | 800F8370 | 800F8370 | 3348 | 3348 | hud_main.c |
| `hudDrawSplitVertical` | 800F9084 | 800F9084 | 3268 | 3268 | hud_split.cpp |
| `hudDrawSplitHorizontal` | 800F9D48 | 800F9D48 | 3268 | 3268 | hud_split.cpp |
| `hudInitPanels` | 800FB0B0 | 800FB0B0 | 1240 | 1240 | hud_init.c |
| `hudRadarInit` | 800FC5C4 | 800FC5C4 | 868 | 868 | ai_route_setup.c |
| `hudRadarArrowsLoad` | 800FD00C | 800FD00C | 196 | 212 | weapon_special_missiles.c |
| `uaInit` * | 80119698 | 801196A8 | 412 | 476 | ua_init.c |
| `carTablesClear` * | 80119848 | 80119898 | 276 | 300 | ua_init.c |
| `roundStart` | 8011A024 | 8011A08C | 372 | 400 | scene_init.c |
| `worldUpdate` | 8011A700 | 8011A784 | 4312 | 5008 | world_update.c |
| `sessionLoadArenaAndCars` | 8012BAFC | 8012BE38 | 664 | 720 | shell_main.c |
| `sessionBindCars` | 8012BD94 | 8012C108 | 200 | 236 | screen_card_layout.c |
| `sessionRun` (+ jump table 800CFCA8) | 8012BE5C | 8012C1F4 | 2688 | 3504 | session_runner.cpp |
| `sessionGetHumanCar` | 8012CBD4 | 8012D29C | 24 | 56 | game_setting_accessors.c |
| `settingsGetHumanCar` | 8013788C | 80137F74 | 24 | 56 | game_settings_extra.c |
| `settingsSetHumanCar` | 801378A4 | 80137FAC | 16 | 52 | menu_state_accessors.c |
| `settingsClearHumanCars` | 801378B4 | 80137FE0 | 40 | 76 | game_settings_extra.c |
| `viewGetDisplaySize` * | 80155714 | 80155E64 | 88 | 96 | engine_misc_accessors.c |
| `viewCycleMode` | 8015576C | 80155EC4 | 160 | 160 | view_system.c |
| `viewInit` | 8015580C | 80155F64 | 124 | 124 | view_system.c |
| `viewCreate` | 80155888 | 80155FE0 | 680 | 680 | view_system.c |
| `viewSetFov` | 80155DB8 | 80156510 | 192 | 192 | engine_misc_accessors.c |
| `viewProc` * | 80155E80 | 801565D8 | 1080 | 1120 | view_system.c |
| `viewSetFollowObj` * | 80156344 | 80156AC4 | 92 | 104 | view_system.c |
| `viewSetBgColor` | 8015659C | 80156D28 | 120 | 120 | view_system.c |

**For the port** (static only; runtime not tested):
- **Load:** recompile `TM2_SOURCE_PORT.EXE` like E0, with `recomp_inventory.json`
  or `functions_ida.tsv` / `names_ida.tsv`.
- **Port RAM:**
  - map RAM up to `.port_bss` end (80205994 here);
  - copy `port_data.bin` to `port_base`;
  - zero `.port_bss` before the entry point.
- **Hooks:** they are keyed by retail VA. `addrmap.tsv` `r2c` translates code
  (only the entry of a changed function maps). For a moved array it gives the
  new address.
- **The side-by-side check:**
  - with one or two humans the variant should match retail, apart from the
    addresses of the moved arrays and the spare bytes 2-3 of `carResetWeapons`
    (set by `uaInit`'s loop);
  - the human records hold pointers, as in the shifted builds
    (`RELOCATIONS.md` 4).
- **Players 3-4:** pads, views, HUD drawing, rules and sound, as listed under
  "Not in the variant".

### Port code (`--port-define`, `--port-unit`, `.port_text`)

The PC port's request 27. The port keeps its own changes to the game's C in an
overlay (all under `#ifdef TM2_PORT_HOOKS`) that it applies to a copy of this
tree. Three things let that overlay leave the tools alone and write real
functions:

1. **`--port-define NAME[=VALUE]`** (repeatable; `build_source_only.py` and
   `build.py`). Appended to the variant's `-D` list after `TM2_PORT_VARIANT` and
   the forced limits, for every unit the variant compiles, in both links of
   `build.py` (the variant and the placement check). A unit whose preprocessed
   source changes with the defines is recompiled, as for `TM2_PORT_VARIANT`.
   Recorded in `defines` and `port_defines` (report, `out/TM2_SOURCE_PORT.json`,
   `LAST_PORT_BUILD.json`).
   - **Which functions the port changed.** When a unit's preprocessed source
     also differs without the port's defines, it is compiled once more without
     them (or compared with the build's object, when the decomp's own defines
     leave it alone). The sections that differ are marked `port_changed`
     (`changed_sections`, `changed_functions[].port_changed`, `why`). The
     placement check accepts them besides the $gp functions
     (`port_changed_functions`).
2. **Group `one`** in `config/US/port_variant_arrays.tsv`: one entry of
   `entry_bytes` whatever the limits (above). The port adds its row and its
   definition (`int portConfig[64 / 4] PORT_BSS = {0};`) as before.
3. **`.port_text` and `.port_rodata`**: port functions and their constants.

**Who may put code there.**

| unit | compiled with | how |
|---|---|---|
| a port unit, `--port-unit PATH.c` (repeatable; `.c` or `.cpp` under this tree, e.g. a file the overlay adds) | **the port code recipe**: the game's compiler and flags (PsyQ 3.6 `CC1PSX`, `-fvolatile`, delay slots by the compiler) but **no $gp** (`-G0`, no `extern_gp`): the port storage lies above 2 MB, out of $gp's reach, so every access is absolute (`lui`/`%lo`). Game data is reached absolutely too, which works anywhere | nothing to mark: its `.text`, `.rdata`/`.rodata`, `.data`, `.bss` become `.port_text`, `.port_rodata`, `.port_data`, `.port_bss` (objcopy). Data needs an initializer (no common symbols); global data in `.port_data`/`.port_bss` must have a row in the tsv, as everywhere; statics need none |
| a game unit (under the port's define) | its own recipe (`BUILD_MANIFEST.json`; mostly PsyQ 3.6 with $gp for small game data) | `__attribute__((section(".port_text")))` on the function; reach port storage with `#pragma tm2_absolute NAME` (as the port's `port_hooks.h` does). Anything else such a unit would emit into `.rdata` (a string literal outside the function's body) stops the build: put that code in a port unit |
| `port_variant_arrays.c` | GCC 2.7.2 (`PORT_UNIT_RECIPE`; its `as` runs with `-G0`, so access is absolute) | the same attribute |

In every case a switch's jump table goes to `.port_rodata` (the compile step
moves a `.rdata` block emitted inside a `.port_text` function there; the
tm2_rdata pragma of a game unit then sees only the game's tables). Calls both
ways are plain `jal`s (game code at 0x8001xxxx-0x801Fxxxx and port code at
0x802xxxxx share the 256 MB segment); names resolve as usual (`symbols.ld`,
the linker).

**Sections at the port base**, in this order:

| section | contents | in the EXE | published |
|---|---|---|---|
| `.port_data` | initialised port storage, at the base (unchanged) | no | `port_data.bin`; the port copies it |
| `.port_bss` | zero port storage, 16-aligned, NOLOAD | no | address and size; the port zeroes it |
| `.port_text` | port functions, 16-aligned | no | `port_text.bin`; the port's recompiler translates it (`functions.tsv` kind `port`) |
| `.port_rodata` | their constants and jump tables, 16-aligned | no | `port_rodata.bin`; the port copies it (the code reads its constants and tables there) |

Within a section the objects follow each other at their own alignment (at most
16): `port_variant_arrays.c` first, then the build's units in build order, then
the port units in command-line order. `gen_link.port_plan` fixes every address
before the link, and the linker script asserts each section and each object's
first global. So `.port_data` stays at 0x80200000 with the same layout, and the
code comes after all the storage (its address moves with `.port_bss`, e.g.
with `--port-limits 2`). `port_text.bin` and `port_rodata.bin` are always
written (0 bytes without port code), and the json always lists all four
sections (an empty one with size 0).

**Port RAM end (`--port-ram-end ADDR`, both tools).** The four port sections
must end at or below this address, or the link stops (`gen_link`: "the port
sections end at ..., past the port RAM end"). **The port chooses it:** its own
high RAM must start at or above it. Default `0x80C00000`
(`gen_link.DEFAULT_PORT_RAM_END`), from the port's answer of 2026-10-13: the
emulated RAM grows to 16 MB, the decomp's port memory may use
0x80200000..0x80BFFFFF (12 MB), and the port moves its own high block (until
now 0x80600000..0x807FFFFF) to 0x80C00000..0x80FFFFFF. The old fixed limit
0x80600000 (`PORT_RAM_END` of request 32 step 2) is superseded; the port moves
its block when this lands. The address must be 16-aligned, above the retail
image and at most 0x81000000. Recorded as `port_variant.ram_end` in
`report.json` and as `port_ram_end` (with `port_end`, the end of the port
sections) in `out/TM2_SOURCE_PORT.json` and `LAST_PORT_BUILD.json`; both tools
print the end and the RAM end. GPU draw lists (OTs and packets) must stay below
0x80FFFFFF (their links are 24-bit): the decomp's (request 31's packet buffers,
`portPacketBuffers`) are inside 0x80200000..0x80BFFFFF, so they are fine.
Request 31's packet buffers and the 2 MiB car DMD area keep their sizes (the
port measures first).

**Maps.** `functions.tsv`, `functions_ida.tsv` and `recomp_inventory.json` list
every function of `.port_text`, global or static (kind `port`, form `c`/`cpp`,
no retail address: `retail_va` empty, `retail_size` 0; the size runs to the next
function). `jumptables.tsv`, `names_ida.tsv` (`jpt_<site>`) and the inventory
list their jump tables (no retail address). `symbols.tsv` has the globals. The
address map has no rows for port code (it has no retail counterpart).

**Proofs.** `verify_reloc.py` checks every relocation of every object's part of
the port sections (`relocs`: a jump table word must equal its target; a
corrupted word in a port table fails with `relocation R_MIPS_32 at 80206150
(.port_rodata:...)`) and the port code's `%hi`/`%lo` pairs (`hi16`). Port code
is never un-relocated: only changed functions call it, and those are excluded
anyway. With no port code the outputs are those of before: the same EXE
(`d7dace6fâ€¦` for the variant) and the same maps.

**Test** (temporary, removed after; the same objects as the build of 2026-10-10):
`--port-define TM2_PORT_TEST --port-define TM2_PORT_TEST_VALUE=3 --port-unit
src/port_test/port_test_unit.c`, with
- the port unit: a global `portTestSwitch` (8-case switch: game sdata
  `aiCarTotal`, the port array `viewModes`, a `one` block `portTestBlock` of 64
  bytes with its tsv row, `jal mathEase` into the game, a static helper reading
  a string constant), `.port_bss` block and string in `.port_rodata`;
- `camera.c` under `TM2_PORT_TEST`: a `.port_text` function with an 8-case
  switch (the unit has a `tm2_rdata` pragma), and `nullsub_16` calling both;
- `port_variant_arrays.c` under `TM2_PORT_TEST`: a `.port_text` function with
  a switch reading `sessionPortHumanCars` (2 bytes) absolutely.

Results: `.port_text` 80205FE0 +0x1C4 and `.port_rodata` 802061B0 +0x70 after
`.port_bss` 80200610 +0x59C4; 4 port functions (one static) and 3 jump tables
in the maps, every table word pointing at its case in the linked code; the
variant's proof against E0 passes (27 ranges excluded: the usual 26 and
`nullsub_16`, `port_changed`); the placement check passes with `nullsub_16` as its only
`port_changed_functions` entry; `build.py`'s publishing wrote `port_text.bin`
(316 bytes) and `port_rodata.bin` (80 bytes) with the first two parts.

**For the port** (its `tools/decomp/wc.py` and overlay):

```
py -3.14 tools/build_source_only.py --build-dir <build> --relocatable --sdk-from-lib --port-variant \
    --port-define TM2_PORT_HOOKS [--port-unit src/port/<file>.c ...] --output work/pv/P4
```

(or `build.py --source-only --sdk-from-lib --relocatable --port-variant
--port-define TM2_PORT_HOOKS [--port-unit ...]`). The patches to
`build_source_only.py` (`port_defines`) and `gen_link.py` (`port_entries`) are no
longer needed; `portConfig`'s row (group `one`) and definition stay the
overlay's. Shared logic such as `TM2_PORT_AI_AIM` can become a function in a
port unit. The port loads `port_rodata.bin` beside `port_data.bin` and feeds
`port_text.bin` (functions of kind `port`) to its recompiler.

**Decomp-side port options.** Some answers to the port's requests are port
code too, behind a define of their own that the port adds to its
`--port-define` list:

| define | what | storage the port sets | doc |
|---|---|---|---|
| `TM2_PORT_FAR_LOD` | LOD nodes keep their farthest band past its end while `renderScene` walks (`renderScene`, `worldLoadStartObjects`, `assetInstallImage` change; 11 functions in `.port_text`); request 46 item 6: `portFarLodNearScale` 2..16 puts every LOD band that many times farther while a view is walked (the sharp near bands kept farther; `portFarLodNear` lists every record's retail band at the scan; `tools/check_custom.py` check 5) | `portFarLod` (0 = retail), `portFarLodNearScale` (0 / 1 = retail) | `FAR_LOD.md` 4, 8 |
| `TM2_PORT_FAR_DISTANCE` | far distance up to 32,767 with a GTE range pass for far meshes, a bigger mesh queue and packet buffers, effect culls that follow it (`renderScene`, `rtMainLoop`, `fxSpritesDraw`, `fxImpactParticlesDraw`, `streaksDraw`, `fxDebrisDraw` change; 14 functions in `.port_text`, and `portPacketBuffersApplyView` / `portPacketBufferCursor` since request 47 revised); also the read-only render capacity counter `portRenderStats` (`renderScene` calls `portRenderStatsWalked` after the walks and `portRenderStatsDrawn` after the near pass: per view and the mirror the matrix slots, meshes and packet bytes, high-water marks, caps reached; observation only, not game state; `tools/check_custom.py` check 5: stores only to `portRenderStats`, no `rand()`); request 61 (`FAR_DISTANCE.md` 2): the range pass rounds the scaled matrix, corrects each scaled entry's translation for that rounding (`portFarTranslate`: the far geometry's sub-pixel wiggle, up to 1.2 px at k = 3, now under 0.05 px), draws a mesh with point sprites or beams from a copy with their sizes divided by 2^k (`portFarSpriteMesh`, storage `portFarSprites` 139,280 bytes after `portDmdImages`, every variant; they were 2^k too large) and gives each scaled entry its own key `0x40000000 | k << 28 | index` (the port reads k there); no game function changes for it (`tools/check_custom.py` check 7) | `portFarDistance`, `portMeshListCap`, `portPacketBufferBytes` (up to 390,320: 4 x retail, **a buffer per view** and parity since request 47 revised, `portPacketBuffers[5][2]`: views 1-4 and the mirror; `rtMainLoop` points its six records at theirs and starts view 2's and the mirror's cursor at their own buffer (`portPacketBufferCursor`); the port points its view 3-4 records with `portPacketBuffersApplyView(record, 2 / 3, parity)` or request 47's `portPacketBuffersApplySet(records, 4, 1)`), `portMirrorFar` (request 46 item 9: the rear-view mirror's far distance, retail 500 / 1,000; `tools/check_custom.py` check 5) (0 = retail); `portRenderStats.reset` (the port writes 1 to restart the counter) | `FAR_DISTANCE.md`; `RENDER_ANSWERS.md` R2.4 |
| `TM2_PORT_RENDER_SLOTS=N` | more matrix slots per view (request 47, `render_slots.h`; N = the limit, 201..2,048, the bare define 400; needs the port variant): `renderScene` counts slots up to N (retail 200), transform nodes get their own slot below N - 2 (retail 198), and the five slot arrays (`renderSlotMatrixPtrs`, `renderMatrixSlots`, `renderObjectMatrices`, `renderFrustumMatricesA/B`) move to `.port_bss` with N + 1 slots (tsv group `renderslots+`, limit `TM2_MAX_RENDER_SLOTS` = N + 1, forced to 200 by `--port-limits 2`). The walker is SingleTrac asm with the 198 as an immediate, so the variant links its C form: `engine_render_scene.c` includes the walker of `reference/renderer_walker_engine_render_scene.c` (the all-C build's copy; the same bytes but the immediate). `renderScene`, `renderWalkModel`, `renderPushMatrix`, `renderMulTransMatVec`, `renderInitNodeHandlers` change; the emitter and the near pass do not. `portRenderStats`' cap bits follow the limit; `portFarState` is sized by it | none (build time) | `FAR_DISTANCE.md` 3 "Matrix slots"; `PLAYTEST_ANSWERS.md` 20 |
| `TM2_PORT_C_EMITTER` | the model emitter's C form with precise corners (request 61 addendum, `port_c_emitter.h`; needs the port variant): `model_emitter.c` includes `reference/model_emitter_800CDC54.c` (the all-C build's copy, with two t8 fixes found by check 8: the light matrix's word 3 at a slot change, SXY1 at a dropped beam) in place of `sub_800CDC54`'s asm, so `sub_800CDC54` changes (0xCBC -> its C size, laid out at it); its integer outputs are the asm's (check 8). With `portPreciseEmit` non-zero its beam and point-sprite branches also write each drawn quad's four corners in 1/16 pixel to the port's precise ring (0x80CB0000, 16,380 entries since request 94; `PORT_PRECISE_PUT`'s format): the exact ends (the entry's translation with the split's fraction, 1/256 GTE unit), the direction from their difference in the display's shape, the half sizes `w H / (Z 2^k)` unclamped below (k from the key; a far copy's source sizes through `portFarSpriteSource`), the x ones times the anamorphic factor `portPreciseEmit / 4096`. The helpers `portPrecise*` are static port functions in `.port_text` (`portFarSpriteSource` in `port_variant_arrays.c`, with `TM2_PORT_FAR_DISTANCE`). Request 82 (`PLAYTEST_ANSWERS.md` 27): `engine_math_near_pass.c` also links the near pass's pure C (only that part of `reference/near_pass_engine_math_near_pass.c`, through `TM2_NEAR_PASS_ONLY`); with `portPreciseEmit` the emitter keeps each near entry's corner depths in `portNearDepth` and the near pass reports every vertex word it writes to the precise ring (`portNear*` static port functions); check 8 runs it against the asm | `portPreciseEmit` (0 off; else the x factor in 1/4096, 4096 none): every frame, drawing only | `PLAYTEST_ANSWERS.md` 4.3; `FAR_DISTANCE.md` 2 "Precise corners" |
| `TM2_PORT_FALL_WHEELS` | a falling car's pit death measured from its wheels (request 107): `humanCarTick` and `aiCarDrive` test `motion.pos[2] - stats.ride_height < worldFallDeathZ()`, so Dark Tooth (ride height 116) dies on the same floor as the other cars; check_custom.py check 20 | none: always on with the define | `RETAIL_BUGS.md` 40 |
| `TM2_PORT_WHOLE_MAPS` | Antarctica's first six floes keep their ground in modes 2 and 4 when the load has them (request 111): `worldCollapseClearDone` takes floes 0..5 only when the loaded file has none of them (`GLACIER2.DMD`, retail's), so the whole `GLACIER.DMD` in those modes plays as in the Challenge; check_custom.py check 23 | none: always on with the define | `RETAIL_BUGS.md` 41 |
| `TM2_PORT_FWY_GROUND` (first named `TM2_PORT_FWY_PARAPETS`, which still works) | Los Angeles: ground under the three ground-level curves' end lips and inner kerbs, which the disc draws up to 65 above their ground (request 112): `portFwyGroundAdd` (from `arenaLoadLosAngeles`) inserts 79 round surfaces from `portFwyGroundTable`, each group after its section's last surface; check_custom.py check 24 | none: always on with the define | `RETAIL_BUGS.md` 43 |
| `TM2_PORT_MOD_ARENA` | Mod arenas: the hidden full-heal spot and the computer cars' teleporter exits as data (request 114): `portArenaRules` (`.port_bss`, `port_mod_arena.h`); `humanCarTick` takes the spot from `portArenaHealSpot`, `aiWarpSetExitNode` the exits from `exit[]` when `warps` is set; check_custom.py check 26 | `portArenaRules` all 0 (retail's) | `MOD_ARENAS.md` |
| `TM2_PORT_WRECK_FALL` | A falling wreck hidden with its splash, or exploding at the floor (request 117): `portWreckFall` (from `humanCarTick` and `aiCarDrive`, after the fall test) hides a still-drawn wreck below 240 on Antarctica (-40 on level 8) as `carDestroyed` does, and explodes a falling wreck once when it passes the fall death's test unless `carDestroyed` blew it up there (`portWreckFallBlown`); `portWreckFallMark` (`.port_bss`, per car slot, `port_wreck_fall.h`); check_custom.py check 28 | none: always on with the define | `RETAIL_BUGS.md` 44 |
| `TM2_PORT_GLACIER_FIRE` | Antarctica's fire missile on floe 5 moved out of the hit-only mark volume that hides it from every car (request 120): `portGlacierFire` (from `worldLoadStartObjects`, level 6, every word checked) moves `GLACIER.DMD`'s pickup `80051B40` and its LOD `80051B64` to floe-local (992, 119); check_custom.py check 30 | none: always on with the define | `RETAIL_BUGS.md` 45 |
| `TM2_PORT_CAR_POSE` | `portCarPose(kind, index, x, y, floor_z, yaw)` (request 108; needs `TM2_PORT_CAR_SPECIALS`): a human or computer car standing still and level at (x, y) on a flat floor at `floor_z`, its matrices, wheels, shadow, object and a Dark Tooth's head written as a step leaves them; no probe, physics, sound or `rand()`; a `.port_text` function only the port calls; check_custom.py check 21 | none: the port calls it between steps | `CAR_POSE.md` |
| `TM2_PORT_CAR_GROUPS` | two or more cars of one type (request 32 step 1): per-car keys, the per-type tables of `SURVIVAL.md` 2.2 replaced by per-key tables, binders by load group, pickers may allow repeats (47 functions change; 9 functions in `.port_text`; `tools/check_car_groups.py` proves no changed function still reaches a retail per-type table) | `portCarOptions` (0 = retail); `portCarGroups` (registries) | `CAR_GROUPS.md` |
| `TM2_PORT_CARS=16` | more cars (request 32 step 2): every car limit a macro in `players.h` (AI records 16, loaded cars 32, slots 64), the car arrays and the pools (projectiles, coordinate systems, sound instances, radar blips; arcs, shields, remote bombs, ricochets, flames, claw records, world models, CLUT animations; since request 33 step 4 the sound request pool, 35 -> 64) raised, opponent slots 9.. in `sessionPortOpponentCars`, car files through `portCarLoadGroup` (2 MiB DMD area), texture-slot stride 32, start picker, cars per arena, card screen skipped past 9 opponents (75 functions change, 71 before request 33 step 4; 2 functions in `.port_text`) | `portCarOptions` bits 1-3, `portCarsPerArena`, `portCarSource`, `sessionPortOpponentCars` (0 = retail) | `CAR_GROUPS.md` 7 |
| `TM2_PORT_CAR_SWAP` | the car swap (request 32 step 3; needs `TM2_PORT_CAR_GROUPS`): `portCarSetGroup` moves a record to another loaded car (two-way, or onto a parked or free car), specials and looks of both cars reset, pickups kept, HP scaled or set, references follow the record; `portCarParkAi`; `sessionBindCars` keeps swaps through the per-round rebind (no function beyond step 1's changes; 11 functions in `.port_text`) | `portCarBindGroup` (0 = the settings' groups; written by the swap, cleared at each load) | `CAR_GROUPS.md` 8 |
| `TM2_PORT_TEXTURE_BANKS` | texture banks (request 32 step 4; needs `TM2_PORT_CAR_GROUPS`): with `portCarOptions` bit 5 (latched at each load) every car load group's images and CLUTs go to a VRAM bank of its own (no sharing between cars; the arena's images by key stay in bank 0), the bank in tpage bits 9..15 of the car's primitives, the invisibility fade per car in its bank; `portVramUploadBank` tells the port's GPU the bank of each `LoadImage`/`StoreImage`; per-group no-reuse flag for mod cars (8 functions change; 11 functions in `.port_text`; `tools/check_car_groups.py` checks 6-7) | `portCarOptions` bit 5, `portCarGroupNoReuse` (0 = retail); read: `portTexBanks`, `portVramUploadBank` | `TEXTURE_BANKS.md` 6, `CAR_GROUPS.md` 9 |
| `TM2_PORT_CAR_SPECIALS` | per-car specials (request 33 step 2a; needs `TM2_PORT_CAR_GROUPS`): Mr. Slam's arm and claw, Twister's tornado, Shadow's strike, Axel's shockwave and Grasshopper's leap per car key (the first car of a type keeps the retail global; duplicates in `portCarSpecials`, exchanged into the retail storage for the time of a call); the per-frame updates loop over the type's cars, first car first; the claw-record pool without retail's overflow; retail-equal with no duplicate, no run-time bit (13 functions change; 11 functions in `.port_text`; `tools/check_car_groups.py` check 8) | none (read: `portCarSpecials`) | `CAR_GROUPS.md` 10, `PER_CAR_PLAN.md` C.2 |
| `TM2_PORT_DARK_TOOTH` | Dark Tooth per car (request 33 step 2e; needs `TM2_PORT_CAR_SPECIALS`): every Dark Tooth has its own head, boss timers, swoop state and stage flag (a sixth part of the specials' selection; `aiDarkToothBossUpdate` runs in its car's selection); retail's level-8 / `aiCars[0]` rules unchanged, retail-equal with one Dark Tooth. `portCarOptions` bit 6 adds `SURVIVAL.md` 1.5/1.6: boss weapons and stage 2 on any arena (per-car stage loop, no round end, no Hong Kong clamps), wreck state 4 after the final death, Hong Kong's jump/warp/marker code only on level 8, the missile wobble anywhere, CLUT uploads that find no room in the upper half go to VRAM rows 240..255 (the allocator shared with `TM2_PORT_CAR_CLUTS`; until request 33-1 `bitmapInit` reserved rows 0..239) (11 functions change; 7 port functions, 2 static helpers and the 3 functions of the rows allocator in `.port_text`; `tools/check_car_groups.py` check 8). Request 42 item 1: `portCarOptions` bit 10 makes every computer Dark Tooth ordinary (his first phase on every arena: the boss update, his head, the missile wobble, CLUT rows 240..255; no stage loop or stage 2, no level-8 tail, no Hong Kong route patch, jump zones, warps or markers; wreck state 4 with his head hidden; no damage after a won round and a 120-tick wreck timer as every car) unless the port marks his car slot as the boss (`portCarDarkToothRole`, `portCarDtBossMask`); `roundStart` and `carDestroyed` change too (13 functions), 3 port functions more; `tools/check_custom.py` check 1 proves the bit-clear gate. Request 46 (`PLAYTEST_ANSWERS.md` 7): the head of a Dark Tooth nobody drives (slot kind 2, or an AI record dead in wreck state 4) is hidden each frame (`portCarDarkToothParked` from `aiCarsCountRemaining`; `portCarDarkToothParkHead` in the park functions) and shown again over the truck when a computer record drives the car (`portCarDtParkMask`, 8 bytes; `tools/check_car_groups.py` check 16) | `portCarOptions` bits 6, 10 (0 = retail rules); `portCarDtBossMask` (bit per car slot, 0 = all ordinary); read: `portCarSpecials.dark_tooth`, `portCarDtParkMask`; API `portCarDarkToothStage`, `portCarDarkToothRole(slot, boss)` | `CAR_GROUPS.md` 11, 18, `PER_CAR_PLAN.md` C.2, `DARK_TOOTH_PLAYER.md` 10.1 |
| `TM2_PORT_CAR_SOUND` | per-car engine sound (request 33 step 3; needs `TM2_PORT_CAR_GROUPS`): with `portCarOptions` bit 4 (read live) every car plays its engine, Thumper's flame and (with `TM2_PORT_DARK_TOOTH`) Dark Tooth's swoops as its own sound instance (slot + 1), merged by instance, the lists raised; no cap by default, `portEngineVoices` = N keeps the N engines nearest to a human; engine voices sort after every other sound, so the farthest engines drop first past the 24 SPU voices; bit 4 clear = the retail calls (5 functions change, +1 with `TM2_PORT_DARK_TOOTH`; 8 port functions in `.port_text`; `tools/check_car_groups.py` check 9). Request 33 step 4 adds: the human loops (turbo, guns, burn/lava) of players 2.. as instance slot + 1 instead of a leaked request per frame (any bit 4; players 0/1 retail), with bit 4 Twister's tornado sound per car and room for 4 in each special-fire sound's list (4 more functions: `carThrottleSteer`, `weaponTick`, `weaponFireSpecial`, `carDestroyed`; 4 more port functions) | `portCarOptions` bit 4, `portEngineVoices` (0 = no cap); read: `portCarSound`, `portCarSoundLoops`; API `portCarSoundStopSlot` | `CAR_GROUPS.md` 12, 16, `PER_CAR_PLAN.md` C.3, C.4 |
| `TM2_PORT_CAR_CLUTS` | per-car CLUTs without texture banks (request 33 step 1; needs `TM2_PORT_CAR_GROUPS`): with `portCarOptions` bit 7 (latched at each load; ignored while banks are live) every CLUT a car would share with another car or the arena is copied after the last car TMS (VRAM read-back, bodies first) to VRAM rows 240..255 (one allocator, shared with `TM2_PORT_DARK_TOOTH`'s bit 6), then to whole free spots of VRAM; a copy without room leaves the car shared and flagged; the invisibility fade per car key; per group and image the CLUT id published for team paint. `portCarOptions` bit 9 (latched at each load, independent of bit 7): `vramUploadClut` tests every texel of a CLUT spot instead of retail's top-left one (`RETAIL_BUGS.md` 27), so an original CLUT never lands on used texels, and takes rows 240..255 when neither half has a whole free spot; request 41 item B: `vramPlaceTexture` tests the whole image rectangle the same way in two passes, retail's two passes as the fallback (`portImageSpotFree`, `portImagePasses`) (6 functions change; 10 port functions and step 4's 5 bookkeeping functions in `.port_text`; `tools/check_car_groups.py` checks 10-11) | `portCarOptions` bits 7 and 9, `portCarGroupNoReuse`; read: `portCarCluts`, `portTexBanks.body_clut`; API `portCarClutsCopyGroup` | `CAR_GROUPS.md` 13, `PER_CAR_PLAN.md` C.1 |
| `TM2_PORT_SPU48` | 48 SPU voices (request 34): voices 24..47 in the port SPU's second register block (0x1F802C00); libspu and libsnd from the C with 48 voices (the `.LIB` modules replaced by `config/US/sdk_c_modules.tsv`: 23 libspu + 2 libsnd modules), libspu masks with bit 31 for 24..47, the voice manager's tables (48) moved to `.port_bss`; the game's driver uses voices 0..43 (`soundVoiceLimit` 44), 44..47 are the port's (`TM2_PORT_SOUND_VOICES_RESERVED`, default 4); no pitch modulation or noise for 24..47 (TM2 uses neither) (36 functions change: 5 libspu, 23 libsnd, 8 game; no port functions) | none to set; read: `portSpuExt` (block 2's key, reverb and envelope bits) | `SPU_VOICES.md`, `CAR_GROUPS.md` 12.4 |
| `TM2_PORT_CAR_DROPIN` | online with bots, players dropping in (request 33; needs `TM2_PORT_CAR_GROUPS`, `TM2_PORT_CARS` and `TM2_PORT_CAR_SWAP`, `#error` otherwise; with `TM2_PORT_CAR_SPECIALS` / `TM2_PORT_DARK_TOOTH` a second Twister, Axel, Mr. Slam, Shadow / Dark Tooth may be loaded mid-match, with `TM2_PORT_TEXTURE_BANKS` each live load gets a bank, with `TM2_PORT_CAR_CLUTS` its CLUT copies): `portCarRemove` takes a record and its car out for good (specials, look, sound ended; its projectiles, mines, drops, arcs, shields, ricochets, bullets removed; every reference cleared; an AI record left vacant on a dummy object, a leaving human's index filled by the last human, `portHumanCompact`); `portCarStartHuman` / `portCarStartAi` start a record mid-match on a free car as a respawn (farthest free start, total <= `TM2_PORT_CARS`); `portCarLoadGroupLive` loads a car from port buffers between frames (no CD read; the load's texture bookkeeping kept in port storage by `bitmapSetupStorage`; refused cleanly when it does not fit); `portCarUnloadGroup` frees a group's bank. Retail logic while nothing is called (1 function changes beyond steps 1/2: `bitmapSetupStorage`; `sessionBindCars`, `sessionLoadArenaAndCars` and `portStartNodePick` get hooks; 13 port functions and 16 static helpers in `.port_text`; `tools/check_car_groups.py` check 12) | none to set; read: `portCarDropIn` (vacant records, live groups); API `portCarRemove`, `portCarStartHuman`, `portCarStartAi`, `portCarLoadGroupLive`, `portCarUnloadGroup`, `portCarFreeGroupOfType`, `portCarGroupFree`, `portCarActiveCount`, `portHumanCompact` | `CAR_GROUPS.md` 14, `PER_CAR_PLAN.md` B.6 / B.7 |
| `TM2_PORT_SPECIAL_AS` | any car fires another car's special, for Gun Game (request 35; needs `TM2_PORT_CAR_SPECIALS`, `#error` otherwise): `portCarSpecialAsSet(slot, type, ammo)` makes car slot `slot` fire `type`'s special (Sweet Tooth, Outlaw 2, Warthog, Mr. Grimm, Thumper, Spectre, Roadkill, Axel, Shadow, Minion; Hammerhead, Grasshopper, Twister, Mr. Slam and Dark Tooth refused), ending the old one and setting its ammo; a borrowed Shadow strike / Axel shockwave runs on the slot's own as-entry of the 33-2a selection with its own strike / ring object; the AI's and the crush's type tests follow the borrowed type; `weaponBlockInit` keeps its max / recharge; the car projectile models and the 2324 / 2325 nodes are cleared at each load and the spiral stores test for 0; with `TM2_PORT_CARS` and `portCarOptions` bit 1, `portCarSpecialLoad` adds the missing car files as parked spare groups. Retail logic while every slot is -1 and the mask 0 (13 game functions change, + `sessionLoadArenaAndCars` with `TM2_PORT_CARS`; 12 port functions; `tools/check_car_groups.py` check 13) | `portCarSpecialLoad` (load mask, bit `type >> 4`); API `portCarSpecialAsSet`, `portCarSpecialType`; read: `portCarSpecialAs` | `GUN_GAME_SPECIALS.md` 4, `CAR_GROUPS.md` 15 |
| `TM2_PORT_HUMANS=N` | N human records (online: up to 16, at most 4 local; needs `TM2_PORT_CARS`, `#error` otherwise; `TM2_MAX_HUMANS` = N). Step H0 (now): the object-id macros of `players.h`, `TM2_HUMAN_ID(i)` (record index -> object id: 1..7, then 9, 11.. past 7 humans, skipping the world root 8 and the missile objects' 10; retail's i + 1 up to 7 humans) and `TM2_HUMAN_INDEX_OF_ID(id)`, at every writer and reader of a human's id (`carBindHumanSlot`, `UpdateGuns`, `weaponFireSpecial`, `weaponDropItem`, `weaponFireEnergyAttack`, `weaponDropCheckHit`; port: `portCarBindSlot`, `portDropInId`, `portHumanCompact`); retail and every N <= 7 compile to the same code. Step H1: the local-player map (`port_humans.h`): each human has a local slot 0..3 (its view, camera, HUD, radar, target panel, hotkey targets) or none (remote: no camera, view calls, HUD or hotkeys here); `worldUpdate` runs the camera and sky follow by slot and only for local humans and plots radar blips on the slot's radar (every `rand()` stays on the all-humans loops), `carSetViewMode` touches the view system and the rear view only for a local human, `carViewHotkeys` returns for a remote human, the HUDs show the human of their slot (`portLocalHuman`), `portHumanCompact` moves the slot with the record; the default map (all zero) is retail's identity for humans 0..3 (8 game functions change: `worldUpdate`, `viewCarCameraUpdate`, `carSetViewMode`, `carViewHotkeys`, `hudDrawSingleView`, `hudDrawSplitVertical`, `hudDrawSplitHorizontal`, `hudDrawTargetPanel`; 4 port functions + 2 static helpers in `.port_text`). Step H2: the inputs (`port_humans.h`): a human on local slot 0/1 reads pad 0/1 with retail's calls, every other human (local slots 2-3, remote humans, everyone with `portHumanOptions` bit 0) its 16-bit input word `portHumanPad[h]` (`padButtonsDecoded`'s half layout) through its own binding table (`portHumanBindings[h]`, built from `portHumanLayout[h]` as `padSetLayout` builds a pad's: by `sessionRun` at the match start, `portCarStartHuman` at a drop-in); `humanCarReadInput` / `carReadMoveCodes` (actions), `humanCarTick` (cheat combos) and `carViewHotkeys` (combo 1) read only through `portHumanAction` / `portHumanCombo` / `portHumanAnyLatched`; `portHumanOptions` bit 3 turns the cheat combos and the three cheat codes off, bit 2 aims a bot's aimed shots at its own target (`aiGetTargetCar`), bit 4 draws the AI blips on every local human's radar in every mode (no `rand()`), bit 5 makes `gameGetLives` read `portHumanLives` for every human (past human 3 it always does); the repair-zone ammo test of `humanCarTick` reads the game mode (2 or 4) instead of the split mode (retail-equal, machine-independent); `portHumanSteer[h]` (tilt 0..4096) is storage for the port's analog hook; `portHumanCompact` moves the word, tilt, layout, bindings and lives (9 more game functions change: `humanCarReadInput`, `carReadMoveCodes`, `humanCarTick`, `gameGetLives`, `weaponFindTargetAhead`, `weaponFindTargetBehind`, `sessionRun`, and `worldUpdate` / `carViewHotkeys` again; 9 port functions + 2 static helpers; `portHumanCompact` and `portCarStartHuman` changed). Step H3: sound sides and local feedback (`port_humans.h`): the turbo / gun / burn loops take the human's sound side (its local slot when 0 or 1: `portHumanSide`) instead of its index; a remote human's gun plays in the computer cars' shared gun loop and (with `TM2_PORT_CAR_SOUND`) its turbo / burn loops as its car's instance, positionally; a remote human's HUD messages, jingles, cues and denied buzz are dropped (`portHumanMessage` / `portHumanSound` / `portHumanDenied` at every site, `TM2_HUMAN_*` macros that are retail's calls without the define) and retail's full-volume sounds "for the player" play positionally for a remote human, as for a computer car (`TM2_CAR_FEEDBACK`); the listener (`sndListenerTransform`, view -1) also takes views 2..3 of local humans; the engine-voice cap (`portEngineVoices`) ranks by the distance to the local humans (17 game functions change: `carThrottleSteer`, `weaponTick`, `carSoundUpdate`, `carReadMoveCodes`, `humanCarTick`, `pickupApply`, `targetActorsUpdate`, `weaponFireSelected`, `weaponFireEnergyAttack`, `carDestroyed`, `weaponFireSpecial`, `weaponFlameStart`, `carCrashStart`, `weaponSlamSwingUpdate`, `weaponSlamGrabUpdate`, `carParisSinkZonesUpdate`, `sndListenerTransform`; 7 port functions + 1 static helper; `portCarSoundFrame` changed). Step H4: N is 4..16 (`#error` outside, above `TM2_PORT_CARS` or without it; 16 human records, 16 cars in all in any mix): every per-human array is `[TM2_MAX_HUMANS]` in port storage (the records 0x814 each, the six word arrays, `sessionPortHumanCars` N - 2, the map, input words, layouts, lives and bindings), `carResetWeapons` moves from retail's 4 bytes (80180FA0) to `.port_bss` past 4 humans (tsv group `humans>4`), `carViewHotkeyReleased` starts 1 for every human, `settingsCopy` stops each list at the settings record's own entries past 4 humans (retail's loops wrote past the record from 7 humans on), static asserts in `port_variant_arrays.c` (records, word arrays, map, `portCarBindGroup`'s exactly 16 human entries + one per AI record); the human setup (`uaInit`, `carTablesClear`, `roundStart`, `sessionRun`, `sessionBindCars`, `sessionLoadArenaAndCars`, `carBindHumanSlot`, `portCarStartHuman`) already ran by `TM2_MAX_HUMANS` / the settings' count; with N = 4 every image is H3's. Step H5: the rules (`portHumanOptions` bit 1, `port_humans.h`; clear = retail): with the bit the C ends no round and decides no result on fixed humans 0/1 (`humanCarDamageUpdate`: a death leaves the human down, no round end; `worldUpdate`: no won test on human 0; `sessionRun`: the result goes to `portRoundResults`, no `lives[0]`, no 2-player result screens; `carNearestToHuman`: bots too; the split HUDs: each slot's own lives; `gameGetLives`: `portHumanLives` for every human); the port's rules use `portHumanAlive` / `portHumanDown` / `portHumanAliveCount` / `portAiAliveCount`, end a round with `portRoundEnd(event, delay, winner, flags)` (kept in `portRound`, every variant) and restart a down human on its own car with `portHumanRespawn(h, node, flags)` (a drop-in style start at the port's route node or the farthest start; no `carRestart` in place); `portCarStartHumanAt` gives a drop-in its start node; `portAiAimTarget` returns no target without human 0 (7 game functions change: `humanCarDamageUpdate`, `carNearestToHuman`, and again `worldUpdate`, `sessionRun`, `gameGetLives`, `hudDrawSplitVertical`, `hudDrawSplitHorizontal`; 9 port functions + 3 static helpers). More than 7 records without the define is an error. `tools/check_humans.py` checks the macros (retail, 4, 7, 8, 16 records; checks 4, 6 and 8 also at 16), that no C computes a human id or index another way, every view / camera / HUD / radar index argument against a reviewed list (check 4), no `rand()` below the local-only code or H2's functions (check 5), every pad read's port against a reviewed list with the human index only in the input functions (check 6) and the cheat gates (check 7), every sound-side call, HUD message, denied buzz and full-volume sound against a reviewed list with the human index only in the H3 gates (check 8; check 5 covers H3's `rand()` side), the storage (check 9: every per-human array of the census sized by its group in every declaration, no write past the settings record with 16 humans, the H4 static asserts; with a build, every humans-group array's entries and the port end), and the rules (check 10: every round end on a reviewed list with the fixed-human rules behind the bit-1 gate, no fixed-human read before the gate, none in the H5 port functions, the respawn path without an in-place `carRestart`), and the compaction (check 11: `portHumanCompact` writes every per-human array, each row of a humans group of the tsv, and the settings' car selection; H6: with the define it also moves the human's car selection, `human_car[0..1]` / `sessionPortHumanCars`). `tools/check_all.py` runs every static check that applies to a build ("Static checks of the port options" below). Request 50 (with `TM2_PORT_CAR_SWAP`, `TM2_HUMAN_HANDOVER`; no `TM2_PORT_CAR_DROPIN` needed): a leaving player's car goes, as it is (place, HP, weapons, slot and team paint), to a free AI record as a computer car (`portHumanHandOver`: the car's blocks copied, the AI's own fields as `aiCarStart` sets them without its start and without `rand()`, the AI difficulty and stats of the type, ids of its things in the air moved to the AI), or is parked (`portHumanLeave`); the human record stays at its index, vacant (dead, far away, a dummy object never drawn, remote, never updated, restarted, bound or targeted, not counted); `portHumanReturn` gives a vacant record an AI's car back (rejoin) and `portHumanRejoin` (request 50b) starts a joiner in vacant index h on a free / parked car, in place (no renumbering; with `TM2_PORT_CAR_DROPIN` also through `portCarStartHumanAt(h, ..)`); 5 game functions test the vacancy (`worldUpdate`, `humanCarsRestart`, `humanCarNearest`, `worldCollapseUpdate`, `sessionBindCars`), the other 25 human loops pass a vacant record by its state (`check_humans.py` check 12); storage `portHumanVacant` (0x14C bytes, `.port_bss` last, every variant) | `portHumanLocal` (signed char per human: 0 = default, 1..4 = slot 0..3, < 0 = remote); `portHumanOptions` (bit 0 every human reads its word, bit 1 the port's rules (H5: no round end or result on humans 0/1), bit 2 bots aim at their own target, bit 3 cheats off, bit 4 AI blips on every local radar, bit 5 lives from `portHumanLives`; 0 = retail); per frame `portHumanPad[h]` (buttons) and `portHumanSteer[h]` (0..4096); per match `portHumanLayout[h]` (0..3), `portHumanLives[h]`, optionally single entries of `portHumanBindings[h][action]` (all every variant); API `portHumanOfId(id)` (every port variant: human index of an object id, -1 if not a present human's; the kill feed's id -> player); with the define `portHumanSlot(h)`, `portLocalHuman(L)`, `portLocalHumanCount()`, `portHumanSetLayout(h, layout)`, `portHumanSide(h)`, `portHumanFeedback(h)`; H5 `portHumanRules()`, `portHumanAlive(h)`, `portHumanDown(h)`, `portHumanAliveCount()`, `portAiAliveCount()`, `portRoundEnd(event, delay, winner, flags)` (read back: `portRound`), `portRoundResults(s, result)` (the results hook), `portHumanRespawn(h, node, flags)`; with `TM2_PORT_CAR_DROPIN` `portCarStartHumanAt(index, group, flags, node)`; request 50 (with `TM2_PORT_CAR_SWAP` too) `portHumanHandOver(h, j)`, `portHumanLeave(h)`, `portHumanReturn(h, j)`, `portHumanRejoin(h, group, node)` (50b), `portHumanFreeGroupOfType(type)` (50b), `portHumanVacantCount()`, read `portHumanVacant`; request 66: `portHumanControl[h]` bit 0 (`TM2_HUMAN_CTL_STEER_KEEP`, match state, set from each player's Controls option): the turn keeps its sign in reverse (`carThrottleSteer`, `carRicochetOffObject` through `portHumanSteerKeep`; `check_humans.py` check 13) | `HUMANS16_PLAN.md` 1.4, 4 (H0, H1, H2, H3, H4, H5), 8 (request 50); `ANALOG_STEERING.md` 2.6 (request 66) |
| `TM2_PORT_DT_PLAYER` | a player's Dark Tooth (request 36; needs `TM2_PORT_DARK_TOOTH` and `TM2_PORT_CAR_GROUPS`, `#error` otherwise; forces `TM2_TEX_SLOT_STRIDE` 32): car select offers index 14 (0xF0) with `portCarOptions` bit 8 (a 15-child copy of the carousel node, child 14 an empty node or the port's model; the info picture and the ending movie of index 14 are the port's, no password for him); a human 0xF0 fires the boss's missiles (`weaponFireDarkToothMissile`, 5 damage, alternating sides from the boss's front launch points) with the machine-gun button every `fire_rate` updates (0 = 10), and the special is the boss's head swoop at the nearest car within 4000 (7 damage, sets burning; refunded with no car in range; 360 / 5 unless set); his own head (his key's `darkToothBossObject`) hovers 240 above the truck placed exactly each frame and sways as the boss's; no boss controller, wreck state 4 at death, camera offsets for his size, +25 missile hit radius as the AI's, the Hong Kong boss markers skip humans, optional engine sound; after a won round he takes no damage like every other car, the computer boss keeps retail's exception (request 41 item A, `carApplyDamage`). Request 42: car select's two name labels are extended with the carousel (their 14 children made index 14 walk garbage), the pause menu shows no password for him, his ending through `portCarEnding` (`DARK_TOOTH_PLAYER.md` 10.2). Retail logic without a human Dark Tooth and with bit 8 clear (15 game functions change with `pauseMenu`, + 4 for the stride without `TM2_PORT_CARS`; 7 port functions, 4 static helpers; `portCarDarkToothWreck`, `portCarModelOn` changed; `tools/check_car_groups.py` check 14) | `portCarOptions` bit 8; `portDtPlayer` (fire rate, recharge, max specials, engine sound, bio picture, ending movie, carousel model; 0 = default) | `DARK_TOOTH_PLAYER.md` |
| `TM2_PORT_HAZARDS` | arena hazards and destructibles off (request 38): `portArenaOptions` bit 0 switches off the hazards (Antarctica's ice: `worldCollapseUpdate` skipped; lava: no HP lost; fire volumes 666: no burning; Hong Kong's trains: removed, hidden and parked outside the map with their roof surfaces and targets), bit 1 the destructibles (LA quake, LA target 508, Moscow's bridges and dish, the Eiffel Tower and its AI-count route patch, New York 519 / 509, Holland's barns, the trains as targets 506 / 507: the trigger ignored or the target immune); `portArenaItems` switches single items (per-arena switches, item 16 the props); both latched by `roundStart` into `portArenaLive`. 12 game functions change (11 gates + the latch, and `laEventsUpdate`'s null-object guard for `RETAIL_BUGS.md` 18), 4 port functions, none calls `rand()`; with both words zero retail's logic and `rand()` order (`tools/check_hazards.py`) | `portArenaOptions` (0 = retail), `portArenaItems` (0 = none) | `ARENA_SWITCHES.md` |
| `TM2_PORT_BALANCE` | Vanilla Plus balancing (request 40; replaces request 39's `TM2_PORT_RAM_BALANCE`, which now stops the build with an `#error`): one rule per bit of `portBalance`, latched by `roundStart` into `portBalanceLive`: bit 0 fire missile 10 / homing 7, 1 a shield recast (or a full shield pool) refused at no cost, 2 permanent mines, 5 per driver (the 6th removes the oldest), own drop records, refused at no cost when one cannot be placed (with `TM2_PORT_CARS` the drop records 6 N, mine objects 5 N and coordinate systems 20 N, tsv groups `drops+` / `mines+`), 3 Hammerhead ram x1.2, crush 15 + 5 once on top, recharge 900, 4 Axel and 5 Minion never mount and ram x1.1 (Minion recharge 600), 6 Twister: no mine / remote-bomb placement and no ram damage taken while her tornado runs, recharge 600, 7 Grasshopper: leap 18, no ram damage taken while leaping, touching the target is the hit, 8 Outlaw 2's recharge 900 (request 45: bit 21 Mr. Grimm's 1200, bit 22 Thumper's 1500), 9 reserved, no effect (Sweet Tooth's spiral stays retail: `BALANCE.md` 3.11), 10 Shadow's detonation min(retail, 25) + 3 and a remote bomb's min(retail, 25), 11 a borrowing Hammerhead does not mount (with `TM2_PORT_SPECIAL_AS`), 12 Twister's throw as her tornado ends (option c; humans by their caught flag), 13 Grasshopper's landing (first cut); request 41: Hammerhead's recharge 700 (bit 3), 14 reserved (request 41's "no hit before the hop" removed in round 3), 15 Twister fires nothing but her shield while her tornado runs (pickups, special, guns, the other energy attacks, detonation: denied, no cost; request 46 item 3 lets the shield through, `BALANCE.md` 15), 16 Outlaw 2's taser second chance (a late connect: the connect's lift and one hit of 10), 17 freeze diminishing returns (ice freezes only: half the previous within 20 frames, floor 15 ticks), 18 a ricochet bomb cannot hit its owner before it left his box once (request 45: nor bounce off his car: it passes through until then), 19 Grasshopper's energy jump free; round 3: 20 the machine guns without the 50 % coin flip (every connecting bullet at half its power, the half point carried per target car; one retail `rand()` fewer per connecting bullet); request 67: 23 `pickupRespawnTick`'s one pick among the pickups that are taken and can show (`portBalancePickupSlot`, `BALANCE.md` 16; the same `rand()` count); request 77: 24 Hammerhead's brakes 2 and slide speed 60 (`carStatsLoad` -> `portBalanceCarStats`, read from `portBalance` at the load, `BALANCE.md` 17). 21 game functions change (+ 6 that size the drop / mine tables, with `TM2_PORT_CARS`), 29 port functions and 8 static helpers, none calls `rand()` itself (three reach retail's own calls by design: the +5, the throw, a late taser connect's lift); storage `portBalanceCars` per car slot (8 bytes) and `portBalanceGunObjects`; with the word zero retail's logic and `rand()` order (`tools/check_balance.py`) | `portBalance` (0 = retail, `0x1FFFFFF` Vanilla Plus / online, bits 0-24, bits 9 and 14 reserved); read: `portBalanceLive`, `portBalanceCars`, `portBalanceGunObjects` | `BALANCE.md` |
| `TM2_PORT_VISUAL` | Vanilla Plus visual switches (request 42 items 6 and 7, `port_visual.h`): `portVisual` bit 0 links each taser / lightning arc segment at the OT entry of its nearer end (`portVisualArcOtz`: camera z >> 3 compressed as the level emitter's SZ, 4 entries in front, 1..4094) instead of entry 0 (retail: drawn over everything, through walls); bit 1 starts the taser's arc (kind 0; the lightning keeps retail's) at Outlaw 2's light bar (request 76: `portVisualTaserFrom`, the bar's top on her near model turned by the car's `motion.body_m`) instead of + 30 over the centre (its jitter retail's: request 88 took out requests 42-87's smaller jitter at points 1 / 2; the `rand()` calls are retail's, count and order); bit 2 (request 45, `PLAYTEST_ANSWERS.md` 3.3) walks the view's own car first in `renderScene` (`portVisualOwnCar`), before the level, so a full mesh queue or packet buffer drops other things first; bit 3 (request 46 item 5, `PLAYTEST_ANSWERS.md` 12) sorts every car's shadow one OT entry behind its car's farthest polygon (`portVisualWalked` after each walk, `portVisualShadows` before the emitter: the shadow's draw entries get the emitter's override pair); bit 4 (request 46 item 4, `PLAYTEST_ANSWERS.md` 11) draws a machine-gun bullet's first streak from the shooter's muzzle where it is now (`portVisualBulletSpawn` in `bulletSpawn`, `portVisualBulletTail` in `streaksDraw`); bit 5 (request 63, `FAR_LOD.md` 10) keeps the chase camera's eye inside the level (`viewChaseCamera` calls `portVisualCameraWall`: the look-at point to eye segment against the level model's hit volumes, the eye pulled in front of the first solid; reads only, stores only the eye); bit 6 (request 72) acts only with `TM2_PORT_HIDDEN_AREAS` (its row). Request 79 (`PICKUP_LOOKS.md` 10, no bit): at each level load `worldLoadStartObjects` builds `portPickupSpecial.model`, a pickup look for "a special" (the HUD's SPECIAL globe on a weapon pickup's model, request 83: no blinking label; `worldLoadObject`'s 1260 case notes the HUD object), for the port to point pickups at. Request 81 (`PICKUP_LOOKS.md` 11): `portWeaponIcon` gives the kill feed a mine's and the freeze shot's sprite (page, CLUT, texel box, colour) from the loaded models. Seven game functions change (`weaponArcsDraw`, `renderScene`, `bulletSpawn`, `streaksDraw`, `viewChaseCamera`, `worldLoadObject`, `worldLoadStartObjects`), 20 port functions; read live, drawing state only (`portVisualState`), no `rand()` (`tools/check_custom.py` check 2: the bit-0 gate, the taser's start run in the link, the retail call census of `weaponArcsDraw`, `bulletSpawn` and `streaksDraw` against E0, request 46's callers and walk order) | `portVisual` (0 = retail, bits 0-5; each machine's own, snapshot / hash no since request 63) | `CUSTOM_TOURNAMENT_ANSWERS.md` 6, 7; `PLAYTEST_ANSWERS.md` 3.3, 11, 12; `FAR_LOD.md` 10 |
| `TM2_PORT_CAR_NUMBERS` | 16-bit car numbers for 128 mod drivers (request 42 item 3, `car_numbers.h`): numbers 0..14 retail, 0x20 + m mod m (`portModCars[m]`: used, base car, name, file code, bio picture, ending movie), -1 none; sessionSettings' numbers in `portSettingsHumanCar` / `portSettingsOpponentCar`, its bytes keeping the base car; the getters return an int (the mod's number when its base is the byte, else the byte), setters and clears write both; `carIndexToType` / `carDisplayName` / `carFileCode` take mods (the base's type, the mod's or the base's name and code), `carIsSelectable` compares numbers and refuses numbers that are no car, no password past car 11 (`sessionRun`, `pauseMenu`), `screenEnding` plays a mod's movie and indexes `endingMovieTable` only below 14, `shellAttractDemo` keeps the numbers, with `TM2_PORT_CARS` `portCarLoadGroup` gets the number (kept per group, the base's disc files as fallback). Item 5: car select's carousel and both name labels get 0x20 + `portCarSel.mods` children (the index is the number; `portCarSelSetChild`, `portCarSelDraw` from `sceneObjDraw` for the port's on-demand in-game model); item 2: `portCarOptions` bit 11 offers Sweet Tooth in Challenge's picker (`screenAiSelect`) while `unlockSweetTooth` is set (13-child copies of AICARSEL's carousel and labels). 22 game functions change (with `passwordBuildString` and `screenLoadCardImages`, which take the getters' int) (+ `sessionLoadArenaAndCars` with `TM2_PORT_CARS`); without a mod number every getter returns retail's value (`tools/check_custom.py` check 3) | `portModCars`, `portCarSel.mods`, `portCarOptions` bit 11; read / API: `portSettingsHumanCar`, `portSettingsOpponentCar` (or the getters), `portCarNumberOfGroup(group)`, `portCarSelSetChild(index, model)`, `portCarSelLoad(k, index, dmd, dmd_len, tms, tms_len)` (request 71: a car's files installed as a carousel child, 4 slots; also with `TM2_PORT_DT_PLAYER`) | `CUSTOM_TOURNAMENT_ANSWERS.md` 2, 3, 5, `PER_CAR_PLAN.md` E.5, `CAR_PROFILES.md` 5-7 |
| `TM2_PORT_CAR_PROFILE` | a mod car's own numbers (request 70, `car_numbers.h`; needs `TM2_PORT_CAR_NUMBERS` and `TM2_PORT_CARS`): the record's slot's load group's car number picks `portCarProfile[number - 0x20]`; a valid one replaces stats-block fields in `carStatsLoad` (before the derived ones, so the hit box follows), the step (`carWheelsSetStepHeight`), the camera offsets (`viewCameraOffsetsInit`), the special's recharge, capacity and max (`weaponBlockInit`), the default top speed (`TM2_CAR_DEFAULT_TOP_SPEED` in the AI's cruise speeds and the shield cheat), the radar colour (`worldUpdate` passes the slot in the type's bits 16..23 to `hudRadarPlotCar`) and the target panel's name; the behaviour stays the base's. 11 game functions change; without a valid profile each runs retail's code | `portCarProfile` (the port's, before a load; match state); API `portCarProfileOfSlot(slot)` | `CAR_PROFILES.md` 1-4 |
| `TM2_PORT_HIDDEN_AREAS` | the hidden areas drawn from anywhere (request 72, `port_visual.h`; needs `TM2_PORT_VISUAL`): with `portVisual` bit 6 (`TM2_VIS_HIDDEN_AREAS`) `renderScene` calls `portHiddenAreasBegin(view)` before its walks and `portHiddenAreasEnd()` after them; Begin checks the loaded level's listed nodes at their fixed addresses and patches them for the walks (Paris's church in PARIS and PARIS2: its zone draws the outer walls and the real inside together, the facade with the doorway and the door at every distance; 6 words), End puts every word back (the hit walker sees the file's values). For the port's depth test only. Request 75 (`HIDDEN_AREAS.md` 12): the church also gets a roof behind its towers (the nave's back ceiling tiles walked again through a half-turn node in `portHiddenRoof`: z 832, facing up; nothing copied but two mesh headers). Request 78 (`HIDDEN_AREAS.md` 12.6): with the view's eye above the roof the nave's top vertices (z 920: its ceiling and its walls' tops, 42) are at the roof's height for the view, so the walls no longer stand up through the roof; `portHiddenAreasDrawn` (request 84: after the emitter and the near pass, which read them) puts them back. One game function changes (`renderScene`); with the bit clear or on another level retail's walk (`tools/check_custom.py` check 12) | `portVisual` bit 6; `portHiddenAreas` (drawing only: `drawn[view & 7]`, the words a view patched); `portHiddenRoof` (`built` 2 = the roof walked) | `HIDDEN_AREAS.md` 11, 12 |
| `TM2_PORT_SPECIAL_PROFILE` | a mod car's special (request 73, `car_numbers.h`; needs `TM2_PORT_CAR_PROFILE` and `TM2_PORT_SPECIAL_AS`): `portSpecialProfile[m]` names its template (one of request 35's 10 borrowable specials, or a pickup weapon fired through `weaponLaunchProjectile`, or the ricochet and remote bomb), applied as a borrowed special at each start (`weaponBlockInit`), and its numbers: the projectiles each fire makes take its damage, life, speed and turn (`portSpecialFireBegin` / `End` around `weaponFireSpecial`'s switch), Axel's shockwave, Shadow's strike and blast, Thumper's flame and burn, the taser, the freeze, the boomerang's turn-back theirs (`TM2_SPECIAL_NUM`, `TM2_SPECIAL_NUM_ID` by the shot's owner id); a caught boomerang's charge goes to its thrower; a computer car takes the template's weight tables (`aiCarSetupProfile`). Ten game functions change (and the strike update's per-car body); without a valid profile retail's code (`tools/check_custom.py` check 13) | `portSpecialProfile` (the port's, before a load; match state), `portSpecialState` (the C's) | `SPECIAL_TEMPLATES.md` 6 |
| `TM2_PORT_ROLLBACK` | online, stages 1 to 3 (request 48, complete; `port_rollback.h`; needs the port variant): `portRollbackOptions` bit 0 `FIXED_TICKS` (`SetFrameStart` sets ticks = `portStepTicks` (0: 3 in every game mode, co-op included: 20 steps a second, the user's decision; clamped 3..5), rate 60 / ticks, the delta, `timerFrameCount` + 1, no measurement; `rtMainLoop` fixes the round's first step and paces at 60 / ticks), bit 1 `SEED` (the three `srand(GetCurTics())` of `uaInit`, `arenaSetup`, `roundStart` take `portRollbackSeedFor(site)`: `portRollbackSeed` + 0x9E3779B9 x the reseeds since the load + 0x10001 x site; `sessionLoadArenaAndCars` starts with `srand` of site 0 and `timerFrameCount` = `portRollbackFrameCount`), bit 7 `HOLD` (`portStepHold`, latched at the top of each iteration: the iteration draws only; no event countdown, pad latch, update, `SetFrameStart`, palette step, `streaksDraw` tick or animation step), bit 9 `PAUSE_PORT` (`pauseMenu` returns at once; `portRollbackQuit` schedules event 2 at the top of the next step); stage 2 (drawing independence): bit 2 `ONCE_PER_STEP` (`streaksDraw` only draws in every view; `portRollbackStepEffects` after view 0's `streaksDraw` runs the flash countdown, `weaponDropsUpdate`, `weaponFlameUpdate` and the arcs' tick once per step; `fxSheetDisplay` / `weaponArcsDraw` split by `portRollbackPass` into a tick pass and a drawing pass that writes no state and takes its jitter from the visual stream; `fxSheetStart` no split doubling), bit 3 `ANIM_IN_STEP` (`animUpdateAll` / `animUpdateOne` step nothing; `portRollbackAnimStep` after `renderFlipFrame` steps the registered list and walks the level model, the world models, the follow object and the visible objects without the camera), bit 4 `VISUAL_RAND` (the arcs' jitter and, without bit 3, the four animator sites draw from `portRollbackVisualRand`, retail's formula on `portRollbackVisualSeed`); stage 3 (re-runs): bit 8 `STEP_SPLIT` (`rtMainLoop` runs the whole step at the top of the iteration, `portStepBody` in retail's order with the HUD's radar swap before `worldUpdate`, then only draws; the port re-runs steps with `portRunStep` between iterations; a re-run's round end leaves the loop at its next top), bit 5 `SOUND_GATE` (while `portResimulating`: the 10 sound wrappers, `soundUpdate`, the port's instance merge and the CD track test do nothing; `portSoundReconcile` after a batch stops the projectile sounds whose projectile is gone), bit 6 `CLUT_SHADOW` (the palette animations read and write their VRAM rows through `portClutShadow`, emptied (`portClutShadowReset`, request 59) and captured at the loop's entry, no upload while re-running, `portClutAnimUpload` after a batch or a loaded snapshot); `portDmdMutableList(out, max)` lists the mutable node fields of every model file loaded for the match (request 54: the level file's every root and every file `assetInstallImage` installed, noted in `portDmdImages` by `portDmdNoteImage`; the same list for the whole match, again after a drop-in's load or unload); `portRollbackEndDue()` peeks whether the next step ends the round (request 55: a scheduled event expires in its `rtUpdateScheduledEvents`); bit 10 `LOCAL_VIEWS` (section 18: each machine's split screen and mirror are its own; the step's one game-state reader of the split, `humanCarTick`'s weapon regeneration in builds without `TM2_PORT_HUMANS`, reads `portRollbackViewSplit()`: retail's split for the match's game mode and level; the other reads are presentation, the 11 view settings "hash no"); bit 11 `ROUND_CLEAN` (request 60: `roundStart` clears every projectile record before the round's restarts, `portRoundCleanProjectiles`: type, life, speeds, target, victim, owner, the trail's counters; `weaponProjectileUpdate` drops a target that is no counted car object's position, `portSeekerTargetOk`; retail revives every record for 11 steps at each round start with its old type and target, RETAIL_BUGS 38); `config/US/rollback_regions.tsv` (resolved per build by `tools/rollback_regions.py`, published as `out/TM2_SOURCE_PORT/rollback_regions.tsv`) lists what a snapshot copies and the hash covers (request 54: the drawing's, the interrupt clock's and the sound side's words "hash no", the scene objects' per-view fields as `field` rows), the port's high RAM as "no". 37 game functions change (request 54's `assetInstallImage`, request 60's `weaponProjectileUpdate`; 38 without `TM2_PORT_HUMANS`: `humanCarTick`, section 18) (the 10 sound wrappers, `soundUpdate`, `worldUpdate`, `hudDisplayRadar`, the 6 `clutAnim` start / step functions) with stages 1-2's 16 (`SetFrameStart`, `rtMainLoop`, `uaInit`, `arenaSetup`, `roundStart`, `sessionLoadArenaAndCars`, `pauseMenu`, `streaksDraw`, `animUpdateAll`, `animUpdateOne`, `weaponArcsDraw`, `fxSheetDisplay`, `fxSheetStart`, `animStepRandom`, `animStepWeightedRandom`, `animPickPeriod`), 36 port functions + 9 static helpers, none calls `rand()` itself (the step runners run the game's own code, which does); with the word zero retail's logic and `rand()` order (`tools/check_rollback.py`) | `portRollbackOptions` (0 = retail; 0x283 = stage 1's four bits, 0x29F = stages 1-2, 0x3FF = stages 1-3, 0x7FF = with bit 10, 0xFFF = every bit), `portStepTicks`, `portStepHold`, `portRollbackSeed`, `portRollbackFrameCount`, `portRollbackQuit`, `portResimulating`, `portRollbackVisualSeed` (optional); calls: `portRunStep()`, `portSoundReconcile()`, `portClutAnimUpload()`, `portClutShadowCapture()`, `portDmdMutableList(out, max)`, `portRollbackEndDue()`; read: `portRollbackHolding`, `portRollbackReseeds`, `portRollbackSplitting`, `portRollbackEnded`, `portClutShadow`, `portDmdImages` | `ONLINE_ROLLBACK.md` 10, 11, 12; 13 (request 51: bit 3 is needed online, the level's animation frames hold hit volumes); 14 (requests 52-53: with bit 3 the animators' `rand()` is the game's stream, so the port's own animator seed wrapper must pass through; check 8 also proves every static symbol in a row); 15 (requests 54-55: the node list of every loaded model file, the hash rows without the drawing's state, the follow object, `portRollbackEndDue`); 18 (the split screen and mirror audit: bit 10, the view settings' rows); 20 (request 59: `weaponArcs.to` hash no, the seekers' target analysis, the palette shadow emptied each round); 21 (request 60: bit 11, the projectile records at a round start, the pools' resets) |

Their storage is defined in every variant (`port_variant_arrays.c`, rows in the
tsv), so the variant's layout does not depend on the option.

**Limits of request 32 step 2 in `players.h`.** The car arrays (`aiCars`, `aiCarObjects`,
`aiCarObjectSlots`, `aiCarTargets`, the four slot tables, the five pair tables) and three pools
(`fxRecords`, `csObjectPool`, `soundInstPool`) move to `.port_bss` in every variant, at retail's sizes
without `TM2_PORT_CARS` (the default variant's code is unchanged: its 25 functions; only addresses move).
The weapon and render pools of the port's Q9 (`weaponArcs` .. `clutAnimRunning`) move only when the
define raises them (tsv groups `name+`). `--port-limits 2` forces every new limit to retail's. The tools
learnt: limit expressions in `entry_bytes`, groups `ai`, `slots`, `slotpairs`, `groups`, `ai-9`,
`name+`; a changed jump-table section shared with an unchanged function's table
(`tournament_tables.c`); a changed function in a plain `.text` section (`weapon_flame_update.c`); and
the port sections must end below the port RAM end (0x80600000 then; now `--port-ram-end`, default
0x80C00000, `gen_link.DEFAULT_PORT_RAM_END`: see "Port code").

### Static checks of the port options (`tools/check_all.py`)

One runner for every static check of a build (HUMANS16_PLAN.md H6). It picks the checks from each link's port
defines (`report.json` `port_variant.port_defines`), runs them in parallel and prints one summary line per check
(the full output of each goes to `work/check_all/`); exit status 0 when all pass.

    py -3.14 tools/check_all.py                          # the latest full build (work/build-*)
    py -3.14 tools/check_all.py work/build-<stamp>       # a given full build
    py -3.14 tools/check_all.py work/pv/<tag>_P4 [...]   # port variant links (build_source_only.py --output)
    py -3.14 tools/check_all.py work/pv/<tag>            # <tag>_P4 and <tag>_P2
    options: --e0 DIR, --no-proof, --models, -j N, -v

| check | runs on | tool |
|---|---|---|
| retail images | a full build: hybrid, source-only, `--sdk-from-lib` sha256 against `BUILD_MANIFEST.json` | (built in) |
| proof | every link: un-relocating it gives E0 outside the changed functions (a full build's own `source_reloc_e0`; else `--e0` or the latest full build's) | `verify_reloc.py LINK --e0 E0` |
| placement | a link with the limits forced to 2 (`source_reloc_port2`, `--port-limits 2`): the changed functions are the $gp users of moved arrays plus the port defines' own | `build.py`'s placement rule |
| humans | every limits-4 port variant link (`portHumanOfId`; with `TM2_PORT_HUMANS` checks 3, 5, 9, 10, 11 on the link; with `TM2_PORT_CAR_SWAP` too check 12: the hand-over functions, the gated functions changed, no forbidden callee; check 13 on every link: `carThrottleSteer` on the MIPS interpreter, the turn's sign with `portHumanControl` bit 0, retail's flip without `TM2_PORT_HUMANS`); the source checks 1-12 once (12: every human loop reviewed, gated or inert for a vacant record; the rejoin in place, no `humanCarTotal` write; negative controls: without `TM2_PORT_CAR_SWAP`, a gate removed, three broken rejoin copies) | `check_humans.py` |
| car groups | a car define (`TM2_PORT_CAR_GROUPS`, `_CARS`, `_CAR_SWAP`, `_TEXTURE_BANKS`, `_CAR_SPECIALS`, `_DARK_TOOTH`, `_CAR_SOUND`, `_CAR_CLUTS`, `_CAR_DROPIN`, `_SPECIAL_AS`, `_DT_PLAYER`) | `check_car_groups.py LINK --expect-functions` |
| hazards | `TM2_PORT_HAZARDS` | `check_hazards.py LINK --expect-functions` |
| balance | `TM2_PORT_BALANCE` (request 67: G22, `portBalancePickupSlot` on the MIPS interpreter) | `check_balance.py LINK --expect-functions` |
| rollback | `TM2_PORT_ROLLBACK` (request 48: the port functions in `.port_text` without `rand()`, no new `rand()` / `srand()` against the variant without port defines, the seed sites on `portRollbackSeedFor`, the load seed first, the HOLD / quit / fixed-ticks / pause gates by flow graph, the HOLD latch; stage 2: the per-view tick, split-function and animation gates, the tick and drawing passes, the visual-stream sites, the step runners' calls; stage 3: the split step's order and gate, the sound gate on every wrapper call, the radar swap, the palette rows through the shadow, the split latch; check 8: `rollback_regions.tsv` resolves and holds every static word the step stores to; check 9 (request 54): no static word the drawing stores to, none of the words the port's re-run check found and none of `csObjectPool`'s per-view fields in a "hash yes" row; requests 54-55's model-file notes and round-end peek; check 10 (section 18): no step-reachable read of this machine's split screen, mirror or sound listener reaches a state write or `rand()` (a taint over the flow graph with control dependence) except through `portRollbackViewSplit`, the view and sound side stores only "hash no" words, `arenaSetup` makes views 0 and 1, the 11 view settings out of the hash, and with the bit modelled clear the gated site is found; check 13 (request 60): `portRoundCleanProjectiles` and `portSeekerTargetOk` run on a small MIPS interpreter of the link's code (the clean clears every live record's fire fields with bit 11 and writes nothing without it; the target test passes only a counted car object's position), their calls dominate the round's restarts and the steering, the round's resets reach every pool; with `TM2_PORT_CARS` request 49's placed cars) | `check_rollback.py LINK --expect-functions --e0 E0` (`--negative-control` on a link without the define must fail; `--regions` with a row removed fails check 8, with request 53's list check 9, with request 57's list check 10; a link of request 59's source fails check 13) |
| custom | `TM2_PORT_DARK_TOOTH`, `_DT_PLAYER`, `_VISUAL`, `_CAR_NUMBERS` (request 42), `_FAR_LOD`, `_FAR_DISTANCE` (request 46, check 5), `_RENDER_SLOTS` (request 47, check 6: the C walker and its limit, the slot arrays in `.port_bss`, no stale relocation into their retail storage; a packet buffer per view: the four packet functions, their callers, `rtMainLoop`'s two cursor calls, `RT_PACKET_CURSOR`'s retail form), `_FAR_DISTANCE` (request 61, check 7: the compiled GTE range pass run in a MIPS interpreter on a made-up queue: scaled vertices within 0.1 px of the exact projection, `round(R / 2^k)`, the entry's own key, the sprite and beam mesh copies' sizes, one copy per (mesh, k), stores only to its state and the queue, `portFarSprites` named only by the pass), `_C_EMITTER` (request 61 addendum, check 8: the C emitter against E0's asm on the MIPS interpreter with a GTE model, 8 made-up views of disc and made-up meshes: the same bytes; with `portPreciseEmit` the same integer bytes and four precise corners per drawn beam and sprite, near the exact ones), `_VISUAL` (request 63, check 9: the camera wall on the MIPS interpreter, a made-up level model against a float model and Moscow's from the disc; no static store) | `check_custom.py LINK --expect-functions --e0 E0` (a link of request 60's source, e.g. `work/pv/r60_port_P4`, fails check 7; the C emitter before the addendum's t8 fixes, `work/pv/r61b_ce_P4`, fails check 8) |
| car models | `--models`: the disc's car DMDs and the key / bank / CLUT / unload models (no link) | `check_car_groups.py --dmd --key-model --bank-model --clut-model --unload-model` |

`TM2_PORT_FAR_LOD`, `TM2_PORT_FAR_DISTANCE` and `TM2_PORT_SPU48` have no tool of their own: the proof and the
placement check cover them. The negative controls of each tool (`--negative-control`) are not part of the run.
