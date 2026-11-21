# Relocations of the retail EXE (port request 24)

The PC port recompiles the retail EXE and wants to move the two-entry per-player arrays
(human records, views, frames, OTs, HUD) to free RAM above 2 MB by rewriting the
instructions that address them. This page describes three data files for that, and then
answers whether the source-only relocatable build is close to running.

```
py -3.14 tools/build_source_only.py --build-dir last --sdk-from-lib --relocatable --output work/reloc24/E0
py -3.14 tools/export_retail_relocs.py --e0 work/reloc24/E0
```

| File | Rows | What |
|---|---:|---|
| `config/US/retail_relocs.tsv` | 25,933 | every relocation in the retail code: VA, kind, symbol + addend, target, paired VA |
| `config/US/retail_data_ptrs.tsv` | 859 | every data word that holds an image or BSS address, with symbol + addend |
| `config/US/move_hazards.tsv` | 131 | what makes moving the arrays unsafe (12 high, 25 medium, 46 low, 48 info) |

Each file starts with a header comment that explains its columns. The files hold addresses
and values only, no retail code bytes. The tool's `ARRAYS` table lists the arrays of the
request (current `names.tsv` names, sizes from `FOUR_PLAYERS.md` 3/4.4/5.1).

## 1. `retail_relocs.tsv`: every relocation in the code

**Columns:** `va kind symbol addend target pair_va array function source confidence method`.

**Kinds** (counts):

| kind | rows | what it is |
|---|---:|---|
| `hi16` | 6,765 | a `lui`; target = the full address of the pair, `pair_va` = its first lo16 |
| `lo16` | 6,765 | an `addiu`/load/store that uses the lui register; `pair_va` = the lui |
| `gprel16` | 1,790 | an offset from gp 801809D4 (all of them target 801809D4..80181164) |
| `jal26` | 10,613 | `j` and `jal`; target = the jump target |
| `word32` | 0 | no address words sit in code (the 46 text gaps hold none, `SOURCE_BUILD.md`) |

- **The pair rule.** A `lui` can feed more than one lo16. Every lo16 names its own lui, and
  each row carries the full target. To move an array by `d`, rewrite every lo16 whose target
  lies in it, and its lui, so that `hi + sext(lo)` = target + `d`. The carry into `%hi`
  changes when `d` crosses 64 KB, so both halves need rewriting. No lui feeds lo16s that
  target different arrays (the `shared_hi` check found none).
- **Indexed pairs** (`lui at,%hi(A); addu at,at,idx; lw %lo(A)(at)`): the target is the base
  `A`. The access reaches `A + idx`, so the whole indexed table must move as one block.
- **`array`** names the request-24 array that holds the target (`start <= target < end`).
  An address equal to an array's end is the next symbol's start, and it is not marked.

**Sources.**
- **`reloc` (25,124 rows, 1,933 functions).** These are the relocation records of the
  source-only relocatable link at retail's layout (E0, identity map, sha `cab53c29…` =
  `expected_source_only_sdk_lib_sha256`) for every function whose bytes equal retail
  (`addrmap.tsv` `code_linear`). The symbol is the one the record names, given its
  current name.
- **`disasm` (809 rows, 15 functions).** These cover the close functions, the
  `code_entry` rows, whose compiled C differs from retail. Their rows come from the RETAIL
  disassembly:
  - **lui pairs:** a forward data-flow from each `lui` to its `addiu`/`ori`/load/store
    uses, also through `addu`;
  - **gp:** `$28` operands, except in functions that write `$28` first (the hand-written
    asm);
  - **jumps:** `j`/`jal`.

  The symbol is the nearest retail label at or below the target. The 15 functions:
  `weaponProjectileUpdate` 800DB110, `carWreckUpdate` 800E75F4, `worldSectionAt`
  800EFF34, `hudRadarInit` 800FC5C4, `fontInit` 80113F30, `pauseMenu` 80129528,
  `screenDrawCard` 8012A0E0, `moviePlayEnding` 8012B898, `menuPasswordCursorRun`
  8012D3F0, `SetDrawEnv` 801421B4, `fxDebrisDraw` 8014C19C, `hudDrawNumber` 8014E194,
  `hdpTestVolumeGroup` 80150804, `SpuVmSetVol` 8015BD20, `sndObjUpdate` 8015F42C.

**Validation.** The same disassembly method ran on all 1,933 identical functions and was
compared with the records (same VA, kind and target):

| method | disasm rows | agree |
|---|---:|---:|
| adjacent lui/lo | 10,782 | 100% |
| indexed (through `addu`) | 2,456 | 100% |
| gp | 1,668 | 100% |
| jump | 10,218 | 100% |
| **all** | **25,124** | **100%** (and 100% of the 25,124 records were found) |

So every disasm row is `confidence high`. Two fixes during validation brought the scan to
100%:
- an `addu` of a lui register with a pointer register is still a lui register;
- crt0's `lui/addiu $28` (the gp setup) is a lo16 pair, not a gp access.

## 2. `retail_data_ptrs.tsv`: pointers stored in data

This file covers all 859 words of `data_relocs.tsv` (`pointer_census.py`), with symbol +
addend from the E0 `pointer_slots.tsv`, checked against every R_MIPS_32 record of the E0
link's data sections. The census missed nothing, so there are **0 new rows**.

| kind | rows |
|---|---:|
| jump_table | 631 |
| string_ptr | 130 |
| vtable | 69 |
| code_ptr | 21 |
| data_ptr | 8 |

**No data word points into any request-24 array.** They are all BSS, apart from the six
sdata arrays in section 3. BSS holds no initialised words. The pointers to the arrays that
exist at run time are all stored by code, from relocated constants (`stored_pointer`
below). The jump tables, vtables and code pointers move with code, not with the arrays.

## 3. Move hazards (`move_hazards.tsv`)

### The arrays

The `layout` rows give each array's size and what follows it. The counts show how each
array is reached.

| array | VA | bytes | followed by | lo16 | gprel16 | functions | where |
|---|---|---:|---|---:|---:|---:|---|
| `humanCars` | 80187B10 | 0x1028 | `aiCarObjects` | 195 | 0 | 34 | bss |
| `humanCarObjects` | 80180FEC | 8 | `humanCarObjectSlots` | 6 | 0 | 6 | sbss |
| `humanCarObjectSlots` | 80180FF4 | 8 | `padConfigs` | 5 | 0 | 5 | sbss |
| `humanCarTypes` | 80181004 | 8 | `hudSmoothedSpeeds` | 3 | **1** | 4 | sbss |
| `humanCarTotal` | 80181014 | 4 | `arenaLoadedId` | 0 | **16** | 7 | sbss |
| `D_80180FD8` / `D_80180FE0` | 80180FD8 | 8 + 8 | `carWorldObjectCount` | 4 / 3 | 0 | 4 / 3 | sbss |
| `carResetWeapons` | 80180FA0 | 4 | `viewWatchingAi` | 5 | 0 | 5 | sbss |
| `carViewHotkeyReleased` | 80180B9C | 8 | `padLayout` | 10 | 0 | 1 | **sdata** |
| `hudSmoothedSpeeds` | 8018100C | 8 | `humanCarTotal` | 2 | 0 | 2 | sbss |
| `viewFollowObjects` | 80180DDC | 8 | `viewFov` | 3 | **3** | 4 | **sdata** |
| `viewCameraObjectTable` | 80181024 | 8 | `aiBossDefeatedFlag` | 4 | **4** | 6 | sbss |
| `viewCameraObjects` | 80181110 | 8 | `viewEnvs` | 18 | **10** | 17 | sbss |
| `viewEnvs` | 80181118 | 8 | `viewModes` | 3 | 0 | 3 | sbss |
| `viewModes` | 80181120 | 8 | `sndObjNeedVoices` | 14 | **2** | 15 | sbss |
| `viewEyePositions` / `viewMatrices` | 801A51A8 | 24 + 64 | `D_801A5200` | 2 / 2 | 0 | 2 / 2 | bss |
| frame slots `D_80180EE4`, `renderBufferView1`, `renderBufferRearView` | 80180EE4 | 3 × 4 | `worldLoadShift` | 1 / 0 / 0 | **26 / 13 / 11** | 10 / 3 / 3 | sbss |
| frame records `renderView0/View1/MirrorFrames` | 8018118C | 3 × 0x100 | `eiffelTowerDebrisPos` | 15 / 11 / 9 | 0 | 9 / 4 / 4 | bss |
| packet buffers 0 / 1 | 801B600C | 2 × 0x17CF0 | (0x50 spare each) | 2 / 1 | 0 | | bss |
| OTs `renderOtView1Buf0/1`, `renderOtView2Buf0/1` | 801E5A8C | 4 × 0x4000 | `dc_y` | 2 / 1 / 1 / 1 | 0 | | bss |
| `hudOrderingTable` (current OT pointer) | 80180F0C | 4 | `weaponTaserVictim` | 0 | **94** | 3 | sbss |
| `hudOrderTables` | 801828D0 | 0x20 | `hudPlayerRecords` | 3 | 0 | 3 | bss |
| `hudPlayerRecords` | 801828F0 | 0xED8 | `hudRadars` | 172 | 0 | 10 | bss |
| `hudRadars` | 801837C8 | 0x6E0 | `hudRadarArrowUpSprites` | 43 | 0 | 4 | bss |
| `hudRadarVisible`, `hudWeaponListVisible`, `hudMarkerBlinkCounters/Phases` | 80180A6C.. | 4, 4, 8, 8 | `aMinion_0` | 8, 9, 1, 3 | 0 | | **sdata** |
| `hudRadarArrow{Up,Down,Left,Right}Prims`, `hudTargetNameModes/Sprites` | 80182308.. | 4 × 64, 24, 1200 | `hudOrderTables` | 1 each | 0 | 1 | bss |
| `hudRadarArrow{Up,Down,Left,Right}Sprites` | 80183EA8.. | 4 × 16 | `hudSplitDivider` | 7 each | 0 | 2 | bss |

### What makes a move unsafe

**1. gp-relative accesses (12 high).** sdata and sbss lie at gp 801809D4 + 0..0x790. A
signed 16-bit gp offset reaches only 801789D4..801889D3. An array moved above 2 MB
(80200000+) cannot be reached from gp: every `gprel16` row that targets it must become an
absolute address (a lui/addiu pair, or simply a constant in the recompiled code).

These are reached gp-relative:
- `humanCarTypes` (1)
- `humanCarTotal` (16)
- `viewFollowObjects` (3)
- `viewCameraObjectTable` (4)
- `viewCameraObjects` (10)
- `viewModes` (2)
- the three frame slots (26/13/11)
- `hudOrderingTable` (94)

The scalars (`humanCarTotal`, the frame slots, `hudOrderingTable`) do not need to move. The
other sbss/sdata arrays (`humanCarObjects`, `humanCarObjectSlots`, `viewEnvs`, the HUD
flags...) are reached only through hi/lo pairs. Their row says so (`low`).

The sbss arrays are packed back to back, with no free bytes after them. For example,
`humanCarObjects` → `humanCarObjectSlots` → `padConfigs`, and `viewCameraObjects` →
`viewEnvs` → `viewModes` → `sndObjNeedVoices`. None of them can grow in place.

**2. Initialised arrays (6 medium).** `carViewHotkeyReleased`, `viewFollowObjects`,
`hudRadarVisible`, `hudWeaponListVisible`, `hudMarkerBlinkCounters` and
`hudMarkerBlinkPhases` are in sdata (file image), not BSS. Moving one means copying its
initial words. Each row lists them: `carViewHotkeyReleased` = 1, 1; `hudRadarVisible` =
00000101; the rest are zero.

**3. BSS clear (1 high).** crt0 zeroes `D_80180EE4..D_801FAD48` (loop bound `sltu` at
801387A4) and then calls `InitHeap(D_801FAD48 + 4)`. An array moved above 2 MB is not zeroed
at boot, so the port must clear the new area itself. The heap still starts at retail's BSS
end.

**4. Addresses compared with a constant: none for the arrays.** The scan tracked every
register that holds a formed address through each function. It found only these:
- the frame-slot tests `D_80180EE4 == &renderView0Frames`, `renderBufferView1 ==
  &renderView1Frames`, `renderBufferRearView == &renderMirrorFrames` (`rtMainLoop`,
  `rtToggleRearView`, `renderSyncView1Frame`). Both sides are relocated constants, so they
  are safe as long as both use the new address;
- crt0's BSS loop.

The scan found no pointer difference (`subu` of two addresses) and no loop bound written as
an address. All the per-player loops count an index (`i < 2`; `FOUR_PLAYERS.md` 3).

**5. Sizes and end addresses computed by hand.**
- **ClearOTagR:** `ClearOTagR(&hudOrderTables[buf][player], 2)` in `hudDrawSingleView`,
  `hudDrawSplitVertical` and `hudDrawSplitHorizontal` stays inside the array. No
  `memset`/`memcpy`/`memmove`/`ClearOTag` is called on any of the arrays with a constant
  size.
- **`hudOrderTables[2][2][2]`** is `[buffer][player]`. More players change the buffer
  stride, not just the length.
- **End addresses:** the 4 `end_address` rows are symbolic names for the next array, not
  loop bounds:
  - `renderGetView1Frames` forms `renderView0Frames + 0x100`;
  - `hudRadarArrowsLoad` forms `hudRadarArrowUpSprites + 0x10`.
- **Outside the symbol:** the 6 `outside_symbol` rows are the same pattern:
  - `renderGetMirrorFrames` forms `renderView0Frames + 0x200`;
  - `hudRadarArrowsLoad` forms `UpSprites + 0x20/0x30`.

  The decomp's C treats the six frame records as one `[6]` array and the four arrow-sprite
  arrays as one `[4][2]` array. By address, each instruction targets the next array. The
  port should either move each of these groups as one block, or rewrite by target address,
  which is what `retail_relocs.tsv` gives.

**6. Neighbours reached through an index past the end (from the docs, not found by the
scan).**
- **`fxFontUvStorage`:** `func_80112AB0` 80112AC4 asks for 30 records into a 10-record bank
  (`RETAIL_BUGS.md` 6). The overflow would land in the font draw modes and templates.
- **Matrix slots:** the matrix-slot index can reach 200 (`RETAIL_BUGS.md` 13). It writes
  `renderMatrixSlots[200]` = `worldSurfaces` and `renderObjectMatrices[200]` = 801FA3B8.
- **`padActionCache`:** `[action][2]` = `[action+1][0]` for port 2 (`FOUR_PLAYERS.md` 3).
- **Two-entry arrays:** index 2 of any two-entry array lands in the "followed by" column
  above. For example, `humanCars[2]` is `aiCarObjects`, `viewModes[2]` is
  `sndObjNeedVoices`, `hudRadars[2]` is the arrow sprites, and `humanCarObjects[2]` is
  `humanCarObjectSlots`. The static scan cannot see an index's range.

**7. Shared storage (3 rows).**
- **`bitmapRecords`:** while a level loads, `bitmapRecords` (24 bytes each) live in
  `renderOtView1Buf0`. Past 682 records they spill into the following OTs
  (`SIXTEEN_CARS.md` 4). Moving or resizing the OTs separately moves where the spill lands.
- **Packet buffer 0:** it holds the VRAM usage bitmap and `vramTexSlots` while loading.
- **Mirror OTs:** view 1 and the mirror share `renderOtView2Buf0/1`.

**8. Pointers stored at run time (27 low).**
- **`rtDoubleBufferInit`:** stores the OT and packet-buffer addresses into the frame records
  (+0x00/+0x04/+0x80/+0x84).
- **`rtMainLoop` / `renderSelectFrame`:** store the frame-record addresses into the frame
  slots.

Every one of these is a relocated constant, so the stored pointer follows the move.

**9. Jump tables and pointers in data (1 info row).** These are the 631 jump-table words
and the 90 code/vtable pointers. They point into code, not into the arrays.

**Hazard list size:** 131 rows:
- `layout` 49
- `stored_pointer` 27
- `gp_relative` 22
- `address_compare` 6
- `initialised` 6
- `outside_symbol` 6
- `end_address` 4
- `index_past_end` 3
- `shared_storage` 3
- `size_by_hand` 3
- `bss_clear` 1
- `code_pointers_in_data` 1

## 4. Is the source-only relocatable build (`source_reloc`) close to running?

**Done:**
- **E0 at retail's layout runs like retail.** The port ran
  `build-20261006-071716-778301/source_reloc_e0` beside retail for 823 frames, and the car
  record hashes were identical (request 25). Today's objects (build
  `20261007-034623-844435`) give E0 sha `cab53c29…`. That equals
  `expected_source_only_sdk_lib_sha256`, and it differs from retail only inside the 15
  close functions listed in section 1.
- **Shifted links are statically proven.** `verify_reloc.py` checks the relocation fields,
  un-relocation back to E0 byte for byte, the typed pointer slots, the hi16 pairing, that
  no unrelocated word holds an image address, the cross-function branches and the header.
  Two shifts were re-proved for this page with the current objects:
  - `--shift 800D1318:16` (sha `d36236d4…`, pc0 8013879C, gp 801809E4);
  - `--shift 80187B10:4096`: 4 KB inserted before `humanCars`, so it and every later BSS
    item move (sha `ab747412…`, BSS end 801FBD48).

  All checks pass for both. The second one shows that every access to `humanCars` and to
  what follows it is symbolic in the source build.

**What is left before the port could recompile `source_reloc` instead of retail:**
1. **A run of a shifted build.** Nothing shifted has been run yet. Comparing its state
   with retail works only for records that hold no pointers into moved data. Car records
   hold object pointers, so they must be skipped or translated with `addrmap.tsv` (`c2r`).
2. **A comparison that understands an address map:** addresses translated through
   `addrmap.tsv`, relocated pointers compared as pointers, and any shifted image without
   its map refused.
3. **Inputs for the recompiler.** Function starts are the `code_*` rows of `addrmap.tsv`
   (plus `symbols.tsv`). Jump tables are the `jump_table` rows of `pointer_slots.tsv`.
   Both are written next to every relocatable EXE.
4. **The 15 close functions** still differ from retail in bytes. The E0 run shows they
   behave the same in that test.
5. **The all-C flavour** (`--all-c`, the four asm units as pure C) is static only. It also
   needs the scratchpad save areas and the two handler tables left out of a comparison,
   and it must accept `code_entry` rows. The hdp walker and the emitter have not been run.
6. **Request 25 (done since, static):**
   - a published stable E0 at a fixed path with its hash (`out/TM2_SOURCE_E0.*`,
     `SOURCE_BUILD.md` "For the port");
   - the port variant: player limits in one header, and the grown arrays in their own
     section above 2 MB (`out/TM2_SOURCE_PORT.*`, `SOURCE_BUILD.md` "Port variant").

   In the variant, sizes like `humanCars[4]` are C changes. The hazards of section 3 became
   source edits: the moved sdata/sbss arrays are addressed absolutely
   (`#pragma tm2_absolute` in `players.h`); their initial words go to `.port_data`; the
   port zeroes `.port_bss`.

**Verdict.** Close. The layout and relocation machinery is proven statically, and E0 ran
identical to retail in the port. The next step is one run of a shifted build with a
pointer-aware comparison. After that comes the port variant of request 25.
