# Source rules

Rules for every file in `src/`. They apply to people and to AI agents alike.

## Pure C only

- **No assembly in C sources.** That means no `asm`/`__asm__` statements
  (including empty ones such as `__asm__ volatile("")` or
  `__asm__("" : : "r"(x))`), no `register T x asm("$N")` variables, and no
  top-level `__asm__` blocks that hold whole functions.
- **No `volatile`.** Do not use it to force loads, stores or their order.
- **Exceptions:** code that must talk to the hardware or the BIOS:
  - cop0 (system control) access
  - GTE (cop2) instructions
  - BIOS syscalls
  - memory-mapped I/O registers (GPU, CD, SPU, MDEC, DMA, timers,
    interrupt control; addresses 0x1F80xxxx)

  In these, `volatile` is allowed for the hardware access, and asm only where C
  cannot express the instruction at all.
- **`volatile` is also allowed for:**
  - data written by interrupt handlers (VSync counters, CD interrupt state, GPU
    queue, callbacks)
  - timing and busy-wait loops

## Scope

- **The PsyQ SDK is out of scope.** Sony's library functions (libapi, libc,
  libcd, libetc, libgpu, libgte, libpress, libsnd, libspu, libsn: crt0
  `start`, `__pure_virtual` and so on) are not reconstructed. They will be
  linked from the original PsyQ `.LIB` files.
- **Sony's files are never committed.** Each contributor supplies them locally
  (`toolchain/psyq-sdk/`, ignored by git).
- **The reconstruction target** is SingleTrac's game and engine code.

## Matching

- **Matching is the goal, honesty comes first.** If plain C does not give the
  retail bytes, keep the closest correct C and report the function as close. Do
  not force a match with asm, register variables or `volatile`.
- **Plain C techniques are fine.** Examples: statement order, temporaries,
  types, loop shape, `do { } while (0)`, struct views of records, and the
  compiler recipe in `BUILD_MANIFEST.json`.
- **The game's recipe keeps `-fvolatile`.** Without it, 642 of the 1,376
  compiled game functions match instead of 1,355, and none matches only
  without it. The simplest getters only match retail with the flag, so
  SingleTrac's build used it. It is a compiler flag, not `volatile` in the
  source.
- **Behaviour must equal retail.** Close functions are checked at runtime
  against retail before they are trusted.

## Data

- **Tables and strings must agree with retail**, not just the code.
  `tools/data_audit.py` and the `DATA` lines of `tools/match_status.py` check
  this.

## Hand-written assembly

SingleTrac wrote a few routines in assembly. These stay as asm, so the
decompilation byte-matches:
- the renderer and tree walker (`engine_render_scene.c`)
- the near-polygon pass (`engine_math_near_pass.c`)
- the hdp walker (`engine_hit_detection.c`)
- the model emitter (`model_emitter.c`)

The evidence that they are hand-written:
- they use trapping `add`/`addi` 36 times, which the 1,084 compiled game
  functions never do;
- they keep data in gp/sp/fp;
- they save registers in the scratchpad instead of on the stack;
- they pass arguments in non-ABI registers.

A working C version of each is kept as an alternative in `reference/`. It
behaves the same as retail, but it is larger.

## Open questions (pending review)

1. **Plain-C reload forms:** do `*(T *)&D_X` and the pointer-macro reads in
   `weapon_leap_homing_init.cpp` count as steering? (Type views that change *what* is
   read, such as a 16-bit read from a byte table, are ordinary C.)
