# Source permuter (`tools/permuter.py`)

`tools/permuter.py` searches for plain-C rewrites of one close function. It
changes the function body at random with transformations that keep the
behaviour, compiles every candidate with the unit's real recipe, and keeps the
candidates whose bytes are closer to retail. It was written for this project
from scratch. It does not use the decomp-permuter project.

It never writes to `src/`. A candidate that reaches 0 words is only a
**candidate**: a person must check it for behaviour equivalence and against
[RULES.md](RULES.md) before it goes into `src/`.

## Usage

```
py -3.14 tools/permuter.py --source src/semantic/font_init.c --va 80113F30 \
    --out <dir> --jobs 3 --minutes 60 [--resume] [--seed f.c ...] \
    [--focus FROM TO] [--alias LOCAL=OBJECT ...] [--weights swap=40,type=0] \
    [--size-penalty 100] [--temperature 1.5] [--stop-after 3]

py -3.14 tools/permuter.py --source ... --va ... --out <dir> --eval variant.c
py -3.14 tools/permuter.py --source ... --va ... --out <dir> --mutate-test 20
```

| Option | Meaning |
|---|---|
| `--source`, `--va` | The manifest unit and the target function. The function is the one placed in `.text_<VA>`. |
| `--out` | The output directory (see below). |
| `--jobs` | The number of parallel compile workers. Each worker has its own work directory and its own DOSBox. |
| `--minutes` | The time budget of this run. |
| `--resume` | Continue from `<out>/<func>.state.json`: the pool, the matches found so far, the elapsed time and the iteration count. |
| `--seed FILE` | A full replacement definition of the function, used as an extra starting point. A seed may contain helpers before the definition, such as a `static __inline__` function. They are placed in front of the function. |
| `--focus FROM TO` | Only accept mutations whose change lies between the line that contains `FROM` and the line that contains `TO`. Use it for large functions with a local difference. |
| `--alias LOCAL=OBJECT` | Tells the dependence analysis that pointer local `LOCAL` only points into `OBJECT`, for example `gt=fontGlyphTables`. Without an alias, a store through a pointer local may alias any memory. |
| `--weights` | Override the mutation weights (names as in the table below, without `m_`). A weight of 0 turns a mutation off. |
| `--size-penalty` | Added to the score when the size differs from the retail slot. The default is 100. |
| `--temperature` | Simulated-annealing temperature. A worse candidate is accepted with probability `exp(-delta/T)`. |
| `--eval FILE` | Score one replacement definition and print the instruction differences. |
| `--mutate-test N` | Print N random mutations as diffs, without compiling. Use it to check the mutations on a new function. |

## Score

The candidate unit is preprocessed (`semantic_compile.preprocess`) and
compiled by `match_unit.compile_candidate`. That is the same PsyQ CC1PSX
(DOSBox-X), maspsx and GNU as path, with the unit's manifest policy. Then it is
linked at the retail addresses, as in `match_unit.py`, and the target symbol's
bytes are compared with the retail slot:

- same size: the number of differing words, the same count as `match_unit.py`;
- different size: the number of non-equal words in a word-level alignment
  (difflib), plus `--size-penalty`;
- compile, assemble or link failure: 1,000,000.

Before the PsyQ compile, every candidate is checked with the native GCC 2.7.2
front end (`toolchain/gcc-2.7.2-psx/cc1.exe -fsyntax-only`). PsyQ's CC1PSX
in DOSBox still writes OUT.S after front-end errors (such as an undeclared
identifier), and its messages are not captured. Without this check, invalid
candidates would get real scores.

Each worker uses `<out>/<func>.work/w<N>/`. Nothing goes through
`work/match`, and no file is shared between workers, so parallel runs are
safe. (`match_unit.disasm` uses a fixed `%TEMP%` file name, so the permuter
has its own disassembler.) Scores are cached by the hash of the candidate
text.

## Search

Each worker does a random walk. It applies 1 to 4 mutations to its current
candidate, then scores the result. It keeps the result when the score is lower
or equal (plateau moves). A worse result is kept with the annealing
probability. After `--restart` steps without an improvement, the worker
restarts from one of the 5 best entries of a shared pool. The pool holds the
12 best distinct candidates.

## Mutations

All mutations apply only inside the target function body, after a small C
tokenizer, statement parser and expression parser. The statement parser knows
blocks, if/else, for, while, do, switch, jumps, labels, declarations and
preprocessor lines. The expression parser uses precedence climbing, with
casts, calls, members, indices and the ternary operator.

| Name | Transformation | Safety condition |
|---|---|---|
| `swap` | Swap two adjacent statements, or two declarations. | Independent (see below). |
| `move` | Move a statement up or down past several statements. | Independent of every statement it passes. |
| `barrier` / `unwrap` | Insert or remove an empty `do ; while (0);`. | Always. |
| `wrap` | Wrap one or two statements in `do { } while (0);`. | No free `break`/`continue`, no labels. |
| `temp` | `{ __typeof__(e) t = e; S[e := t] }` for a subexpression `e`. | `e` has no side effects and is not in a conditional operand (`&&`, `\|\|`, `?:`). The statement has at most one assignment, at the top, and at most one call, which contains `e` in its arguments. |
| `untemp` | Inline a block temporary that is used once. | The initializer has no calls and no writes. |
| `decl_split` | `T a = e;` to `T a; a = e;` and back. Split `T a, b;`. | Only the last declaration of a block, and only scalar declarations. |
| `type` | Change a local's base type to an equivalent spelling (`int`/`s32`/`long`). | Always. Mutations marked `RISKY` change the width or signedness and need review. |
| `dup_if` | Move the statement after an if/else into the end of both arms (the cross-jump trick), or the statement before it into the head of both arms. Hoist a common tail out of the arms. | The arms do not end in a jump. For the head form, the statement must be independent of the condition. |
| `commute` | Swap the operands of `+ * & \| ^ == !=`, or flip `<`/`>`. | Both operands have no side effects. |
| `compound` | `a op= b` to `a = a op (b)`, and back. | `a` has no side effects. |
| `shift` | `x << n` to `x * 2^n`, and back. | Constant `n`. The two forms are the same in two's complement. |
| `cond` | Swap the if/else arms with a negated condition. `!x` to `x == 0`. `i < N` to `i <= N-1`. | Integer constants. |
| `loop` | `for` to `while`, `for`/`while` to `if (c) do ... while (c)`. | The body has no `continue`. |
| `decl_move` | Move an uninitialised declaration into the innermost block that holds all its uses. | The block's first use is a plain top-level assignment, and the variable's address is never taken. |
| `block_copy` | `{ __typeof__(v) c = v; ...v -> c... }` for a local that the block only reads. | `v` is not written in the block and its address is never taken. |
| `expand_macro` | Replace `MACRO(args);` (a unit macro that expands to a comma list) with one statement per element. | Comma sequencing becomes statement sequencing. |

### Dependence analysis

Two statements are independent when all of these hold:

- neither has a call, jump, label or preprocessor line;
- neither writes a local that the other reads or writes;
- no memory write of one may overlap a memory access of the other.

Macros defined in the unit and its headers are expanded before the analysis.
Memory is classified by its base object:

- a global or local array or struct, with `#k` for a constant first index and
  `@member` for the first member (union members excepted);
- an `--alias` target;
- `ANY`, for a store through any other pointer.

Different array elements, or different struct members, do not conflict. A
cast in the access path drops the member detail. Under `-fvolatile` the
order of volatile pointer accesses to separate objects does not change
behaviour.

## Output

| File | Content |
|---|---|
| `<func>.log` | One entry per improvement: time, iteration, score, the mutation chain, a unified diff against the starting function and the first instruction differences. Also seed scores and `=== MATCH` lines. |
| `<func>.best.c` | The best function text. |
| `<func>.pool<k>.c` | The pool, for `--resume`. |
| `<func>.match_<n>.c` | Each 0-word candidate. **Review these by hand.** |
| `<func>.state.json` | Statistics: iterations, compiles, cache hits, errors, the score histogram, and per-mutation counts (`[used, improved, failed to compile]`). |

## Reviewing a match

1. Read the diff in the log and replay the mutation chain. `RISKY` type
   changes need a proof that the value range fits. A reorder needs a check
   that the two statements touch different objects (aliasing through
   pointers is the usual trap).
2. Replace each `__typeof__(...)` temporary with its concrete type.
   Re-check the result with `py -3.14 tools/match_unit.py --source <unit>`.
3. Check the result against [RULES.md](RULES.md): pure C, no `volatile`, no
   new `*(T *)&D_X` casts.
4. Only then edit `src/`. Keep the file's line endings.
