# Function signatures for the port

`tools/export_signatures.py` writes three tab-separated files for the PC port, which hooks retail
functions by VA:

| file | contents |
|------|----------|
| `config/US/signatures.tsv` | one row per function of `config/semantic/selection-template.json` |
| `config/US/signature_types.tsv` | the type definitions those signatures name (transitively) |
| `config/US/signature_conflicts.tsv` | functions declared with different prototypes in different places |

```sh
py -3 tools/export_signatures.py          # about 10 s; scratch output in work/signatures/
```

Re-run it after changing sources, headers or the selection template. It changes nothing under `src/`.
Lines starting with `#` are comments; the first other line is the header. Files are UTF-8 with `\n`.

## How prototypes are obtained

- **C units**: preprocessed with the flags of `tools/semantic_compile.py`, then compiled by the native
  GCC 2.7.2 cc1 (`toolchain/gcc-2.7.2-psx`) with `-aux-info`. The compiler lists every declaration and
  definition the unit sees, with typedef names as written and parameter names for definitions.
- **C++ units** (`.cpp`, PsyQ CC1PLPSX, which has no `-aux-info`): a small parser over the preprocessed
  text. Constructors and methods named by `#pragma tm2_symbol` get an explicit first parameter
  `Class *this`. A constructor returns `this` (GCC 2.x does this).
- **Functions in file-scope `__asm__` blocks** (`kind` = `asm`): the prototype comes from, in order,
  the pure-C reference version (`reference/*.c`, the units `tools/build_source_only.py --all-c` swaps
  in), the callers' declarations in other units (the most common prototyped form), or the PsyQ 3.3
  headers under an alias name (`extra_funcs.tsv`, `ida_names.tsv`, the `names.tsv` old name, for
  example `sub_8013FCC8` = `SquareRoot12`).

## signatures.tsv

| column | meaning |
|--------|---------|
| `va` | retail address (hex, no `0x`) |
| `name` | the template symbol; the tool finds its definition by this name in `source` (else `check` says so) |
| `return_type` | as written in the source (`?` = unknown) |
| `params` | `type name` items separated by `, `. Split only at commas outside parentheses, because function-pointer parameters contain commas (`s32 (*fn)(s32)`). Empty = no parameters. `?` = unknown or unprototyped. Prototypes taken from declarations have types without names |
| `varargs` | `yes` if the list ends in `...` (not repeated in `params`) |
| `source` | the defining unit |
| `kind` | `game`, `sdk` (inside a PsyQ library module of `config/US/sdk_modules.tsv`) or `asm` (defined in an asm block, game or SDK) |
| `sdk_module` | `LIB/MODULE` from `sdk_modules.tsv` when the VA is in a library module |
| `style` | `ansi`; `knr` (old-style definition; parameters shown with promoted types); `cpp`, `cpp-ctor`, `cpp-method`; for asm functions `asm-` plus the style of the declaration used, or `asm-unknown` |
| `linkage` | `global` or `static` |
| `proto_from` | `file:line` of the definition, or where an asm function's prototype came from (`reference C ...`, `decl file:line (N decls)`, `psyq header:line as Alias`) |
| `knr_declared` | for K&R definitions with narrow parameters: `name:declared_type` (for example `human:u8`). The caller passes the promoted `int`; the callee narrows it |
| `types` | non-scalar types named by the signature, as `Name=file:line` of the definition this unit sees. Scalar typedefs (`s32`, `u8`, ...) are left out; they are in `signature_types.tsv` |
| `check` | `ok` (the `.text_<VA>` section attribute matches the VA), `ok (no section attribute)` (placed by `#pragma tm2_symbol`, binding files, ...), `section .text_X` (attribute names another VA) or `not defined in source` |

All registers are 32 bits on the R3000: pointers, `int`, `long` and `unsigned` arguments are passed the
same way. The first four argument words go in `a0`..`a3`, the rest on the stack. Structs passed by value
are copied into argument words.

## signature_types.tsv

`name` (a typedef name, or `struct X` / `union X` / `enum X`), `kind`, `defined_at` (`file:line`, plus
`(+N more)` when the same text is defined in several units; headers in `include/` come first) and
`definition` (the definition on one line, attributes removed). Units often declare their own view of a
record, so one name can have several rows with different definitions; the `types` column of
`signatures.tsv` says which one a function's unit uses.

## signature_conflicts.tsv

A function appears here when its declarations and definition disagree after typedefs are expanded,
`const`/`volatile` removed and `long`/`int` spellings unified. Each distinct prototype is one row:

| column | meaning |
|--------|---------|
| `name`, `va` | function (`va` empty when it is not a template function, for example an SDK call that is a macro or not linked) |
| `severity` | the worst difference for the function (same on all its rows, sorted in this order): |
| | `count`: different number of parameters or varargs |
| | `abi`: a parameter or return value differs in register class (32-bit vs 16-bit vs 8-bit vs struct by value vs float) |
| | `sign`: only the signedness of a narrow type differs |
| | `return-void`: some places say `void`, others return a value |
| | `type`: same classes, different C types (`void *` vs `struct X *`, `s32` vs `u32`, ...) |
| | `implicit` / `unprototyped`: some caller has no declaration or an old-style `f()` declaration |
| `variant` | 1 = the prototype used at most places |
| `prototype` | as written at the first site |
| `canonical` | the compared form |
| `sites` | number of places |
| `where` | the places; `(def)` marks the definition |

Static functions are compared only within their unit. Declarations in shared headers count once. These
are worth fixing, but the build is byte-identical with them: a prototype change can change the code GCC
generates (argument narrowing, delay-slot filling), so check the image hash after each fix.

## Shared prototypes

Functions that several units call have one prototype in `include/semantic/`, taken from the
definition (plain C types, so units with their own `u8`/`s32` typedefs and C++ units can include
it): `car_calls.h` (included by `cars.h`), `weapon_calls.h` (included by `weapons.h`), `scene.h`,
`settings.h`, `fx.h`, `hud.h`, `input.h`, `sound.h` and `world.h`. A unit keeps a local
declaration only where its pointer types differ from the definition's, or where retail was
compiled with a different prototype; those cases have a comment and are listed in
docs/RETAIL_BUGS.md ("Decomp notes").
