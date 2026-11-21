# Twisted Metal 2 — Decompilation

A work-in-progress **decompilation of Twisted Metal 2** (PlayStation, 1996,
USA `SCUS-94306`) back into C, function by function, verified byte for byte
against the original game.

The goal is a complete, readable C source of the game. It is the foundation
of [OpenTM2](https://github.com/Tokatta007/OpenTM2), a PC port of the game, which links to this
repository; the [Rogue Trip partial decompilation](https://github.com/Tokatta007/Rogue-Trip-Vacation-2012-Decomp) (same engine family) is a
sister project.

## Progress

Every function below is compiled from C and checked against the retail
executable: **matching** means the compiled bytes are identical to the
original.

<!-- progress:start -->
```
Functions  ████████████████████  99.9 %   1,946 / 1,948 matching
Code       ████████████████████  99.7 %   595.8 KB / 597.6 KB matching
```

| Status | Functions | Code | What it means |
|---|---:|---:|---|
| ✅ **Matching** | 1,946 | 595.8 KB | C compiles to exactly the retail bytes |
| 🟡 **Close** | 2 | 1.8 KB | In C, fits its slot, a few instructions still differ (`sdk_libcd.c`, `sdk_libsnd2.c`) |
| ⬜ **Not yet in C** | 0 | 0.0 KB | Still retail machine code in the build |
<!-- progress:end -->

The totals cover every function in the executable, including Sony's PsyQ SDK
libraries, which do not need to be decompiled for the port; the share of
*game* code done is therefore higher than shown: every SingleTrac function is
byte-identical, and the 2 close ones are Sony SDK functions (source-only builds
can link those from the original PsyQ `.LIB` files).

To refresh this section after a build:
`toolchain\python\python.exe tools\readme_progress.py`.

## How it works

The project builds a **hybrid executable**: each function that has been
reconstructed in C is compiled with the game's original compiler lineage
(GCC 2.7.2, PsyQ 3.30 headers, maspsx in ASPSX 2.34 mode) and placed at its
original address; everything not yet reconstructed stays retail machine code.
The resulting executable runs like the original and grows more "C" with every
function added.

## Building

You need your own copy of the game and a local toolchain. Neither is in this
repository (see `.gitignore`), and no game bytes are: the build reads the retail
executable you supply and checks its result against the hashes in
`BUILD_MANIFEST.json`.

### 1. The game files

| Path | What |
|---|---|
| `bin/SCUS_94306R.EXE` | The retail executable (USA, `SCUS-94306`), copied from your own disc. Its SHA-256 is checked. |
| `cdimage/Twisted Metal 2 (USA) (Track 01).bin` | The data track of your disc image (Redump naming). Only the checks that read level and car files (`tools/check_*.py`) need it, not the build. |

### 2. The toolchain (`toolchain/`, Windows x64)

| Path | What | Where it comes from |
|---|---|---|
| `toolchain/python/` | Python 3.14 (`python.exe`), the embeddable package | python.org, "Windows embeddable package (64-bit)" |
| `toolchain/mips/bin/` | GNU Binutils 2.43 for `mipsel-none-elf` (`as`, `ld`, `objcopy`, `objdump`, `nm`) and its `cpp`, Windows build | the pcsx-redux project's Windows MIPS toolchain, package `g++-mipsel-none-elf-14.2.0` (GCC 14.2.0 with binutils 2.43), installed with its `mips.ps1` script; copy its `bin/` and `libexec/` here |
| `toolchain/gcc-2.7.2-psx/cc1.exe` | GCC 2.7.2 `cc1` for the PlayStation, native Windows | a native Windows GCC 2.7.2 `cc1` built with the decompals `old-gcc` patches (github.com/decompals/old-gcc) |
| `toolchain/psyq330/include/` | PsyQ 3.3 SDK headers | your PsyQ SDK |
| `toolchain/psyq-candidates/psyq3.3/psyq3.3/`, `psyq3.5/psyq3.5/`, `psyq3.6/psyq3.6/` | The PsyQ compilers (`CC1PSX.EXE`, `CC1PLPSX.EXE`, ...) of each version: units that match only with Sony's own compiler are built with them | your PsyQ SDK |
| `toolchain/psyq-candidates/dosbox-x/bin/x64/Release/dosbox-x.exe` | DOSBox-X, which runs those DOS compilers | github.com/joncampbell123/dosbox-x releases |
| `toolchain/gcc-2.5.7-psx/cc1.exe`, `gcc-2.6.3-psx/cc1.exe` | Native `cc1` builds for Sony's libsnd | build them with `tools/old-gcc/` (its README) |
| `toolchain/psyq-sdk/psyq33/`, `psyq35/` | The PsyQ `.LIB` files, only for `--sdk-from-lib` (`docs/SDK_SOURCES.md`) | your PsyQ SDK |

`maspsx` (the assembler front end) is in `tools/maspsx/` and needs nothing more.

### 3. Build

From PowerShell:

```powershell
.\build.cmd
```

Each build recompiles every C unit, links them, and writes
`work/build-<timestamp>/psx/TM2_MAXIMAL_HYBRID.EXE`; `LAST_BUILD.json`
records its path and SHA-256. The build refuses an image whose SHA-256
differs from the one expected in `BUILD_MANIFEST.json`, so every commit is
reproducible. Existing outputs are never overwritten.

```powershell
.\build.cmd --source-only
```

does the same hybrid build, then links the freshly compiled unit objects into
the source-only image (`work/build-<timestamp>/source/TM2_SOURCE.EXE`, the
nonmatching flavour of [docs/SOURCE_BUILD.md](docs/SOURCE_BUILD.md)), checks it
with `tools/verify_image.py` and against `expected_source_only_sha256` in
`BUILD_MANIFEST.json`, and records it in `LAST_SOURCE_BUILD.json`.
`LAST_BUILD.json` always names the hybrid image. `docs/SOURCE_BUILD.md`
describes the relocatable, all-C and port-variant builds and `tools/check_all.py`.

The `split/` folder (retail segments cut from your executable by
`tools/split_retail.py`), `work/` and `out/` are build outputs and stay local.

## Repository layout

| Path | Contents |
|---|---|
| `src/semantic/` | The reconstructed C, one unit per function or module |
| `include/` | Project headers |
| `BUILD_MANIFEST.json` | Every C unit, its compiler settings and the expected image hashes |
| `config/US/` | Function lists (`ida_funcs.tsv`, `extra_funcs.tsv`), IDA names (`ida_names.tsv`), readable names (`names.tsv`), file renames, address layout and pointer census |
| `reference/` | Pure-C versions of the four hand-written asm units (the model emitter, the hit walker, the render walker, the near pass), used by the all-C flavour and the port variant |
| `tools/` | Build, matching (`match_*.py`), source-only/relocatable link, checks, audits, rename tools; maspsx |
| `docs/` | Method, status, the game's systems and data formats, and the designs built for the PC port |

## Where to start reading

- [`docs/CODE_MAP.md`](docs/CODE_MAP.md): which file and function does what, by subsystem.
- [`docs/PORT_RENAMES.md`](docs/PORT_RENAMES.md): old → new names for symbols and files.
- [`docs/RULES.md`](docs/RULES.md): the source rules (pure C, scope).
- [`docs/RETAIL_BUGS.md`](docs/RETAIL_BUGS.md): bugs in the original game.
- [`docs/SOURCE_BUILD.md`](docs/SOURCE_BUILD.md): source-only, relocatable and all-C builds.
- Topic answers for the PC port:
  - [`MAIN_LOOP`](docs/MAIN_LOOP.md)
  - [`MATCH_START`](docs/MATCH_START.md)
  - [`TOURNAMENT_FLOW`](docs/TOURNAMENT_FLOW.md)
  - [`COOP_LIVES`](docs/COOP_LIVES.md)
  - [`GAME_EVENTS`](docs/GAME_EVENTS.md)
  - [`WIDESCREEN`](docs/WIDESCREEN.md)
  - [`SECRET_LEVELS`](docs/SECRET_LEVELS.md)
  - [`CAR_SELECT_CAR`](docs/CAR_SELECT_CAR.md)
  - [`OPTIONS_CONTROLS`](docs/OPTIONS_CONTROLS.md)
  - [`UNLOCKS`](docs/UNLOCKS.md)
  - [`CAR_STATS`](docs/CAR_STATS.md)
  - [`DRAW_DISTANCE`](docs/DRAW_DISTANCE.md)
  - [`RAND_CALLERS`](docs/RAND_CALLERS.md)
  - [`RENDER_ANSWERS`](docs/RENDER_ANSWERS.md)
  - [`ANALOG_STEERING`](docs/ANALOG_STEERING.md)
  - [`TEAM_COLORS`](docs/TEAM_COLORS.md)
  - [`TEXTURE_BANKS`](docs/TEXTURE_BANKS.md)
  - [`WORLD_MARKERS`](docs/WORLD_MARKERS.md)
  - [`FOUR_PLAYERS`](docs/FOUR_PLAYERS.md)
  - [`PICKUP_LOOKS`](docs/PICKUP_LOOKS.md)
  - [`AI_VS_AI`](docs/AI_VS_AI.md)
  - [`AI_LIVES`](docs/AI_LIVES.md)
  - [`SIXTEEN_CARS`](docs/SIXTEEN_CARS.md)
  - [`SANTA`](docs/SANTA.md)
  - [`SIGNATURES`](docs/SIGNATURES.md)
  - [`FX_SPRITE_STACK_POINTER`](docs/FX_SPRITE_STACK_POINTER.md)
  - [`FAR_LOD`](docs/FAR_LOD.md)
  - [`FAR_DISTANCE`](docs/FAR_DISTANCE.md)
  - [`NATIVE_RENDERER`](docs/NATIVE_RENDERER.md) (request 47 item 4: the renderer as host C for the port; research, staged plan)
  - [`CAR_GROUPS`](docs/CAR_GROUPS.md) (plan: [`CAR_GROUPS_PLAN`](docs/CAR_GROUPS_PLAN.md))
  - [`PER_CAR_PLAN`](docs/PER_CAR_PLAN.md)
  - [`SPU_VOICES`](docs/SPU_VOICES.md)
  - [`GUN_GAME_SPECIALS`](docs/GUN_GAME_SPECIALS.md)
  - [`HUMANS16_PLAN`](docs/HUMANS16_PLAN.md)
  - [`DARK_TOOTH_PLAYER`](docs/DARK_TOOTH_PLAYER.md)
  - [`ARENA_SWITCHES`](docs/ARENA_SWITCHES.md)
  - [`BALANCE`](docs/BALANCE.md)
  - [`CUSTOM_TOURNAMENT_ANSWERS`](docs/CUSTOM_TOURNAMENT_ANSWERS.md) (request 42: ordinary Dark Tooth, Sweet Tooth, mod car numbers, car select, arcs)
  - [`SUDDEN_DEATH`](docs/SUDDEN_DEATH.md) (request 43: soldiers on foot and storm clouds, questions answered; not built)
  - [`ONLINE_ROLLBACK`](docs/ONLINE_ROLLBACK.md) (request 44: what rollback netcode needs from the game; questions answered, not built)
  - [`PLAYTEST_ANSWERS`](docs/PLAYTEST_ANSWERS.md) (findings from the port's playtests, answered section by section)
  - [`FILE_FORMATS`](docs/FILE_FORMATS.md) (DMD / TMS / the PC's DPC / TPC, for mod converters)
- [`docs/PERMUTER.md`](docs/PERMUTER.md): the source permuter for close functions.

## Contributing

Work on a branch and open a pull request. Pick a function from
`config/US/ida_funcs.tsv` that is not yet matching, write it in
`src/semantic/`, register it in the build manifest, and iterate with the
`tools/match_*.py` scripts until it is byte-identical. Follow
[`docs/RULES.md`](docs/RULES.md): pure C, with no asm or `volatile`
outside hardware and BIOS access. The Rogue Trip
decompilation's `docs/COMPILER.md` collects matching techniques for the same
engine family.

## Related projects

- **[OpenTM2](https://github.com/Tokatta007/OpenTM2)**, the PC port this decompilation is the base of.
- **[Rogue Trip](https://github.com/Tokatta007/Rogue-Trip-Vacation-2012-Decomp)** partial decompilation (the same engine family); its
  `docs/COMPILER.md` collects matching techniques.

## Rights

A research and preservation project. It contains reconstructed source code of a
commercial game and no game files: you need your own copy of Twisted Metal 2 to
build it. No license to Twisted Metal 2 or to the Sony PsyQ SDK is granted.
Twisted Metal 2 is © its respective owners. Third-party code used here is listed
in [`THIRD_PARTY.md`](THIRD_PARTY.md).
