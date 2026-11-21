# Third-party code

Some reconstructed Sony PsyQ SDK library functions in `src/semantic/sdk_*.c` and `src/semantic/lib_r*.c`
are adapted from these open-source projects. Each adapted file says in its
header comment which project it came from.

## sotn-decomp — `src/main/psxsdk` (MIT)

https://github.com/Xeeynamo/sotn-decomp

The psxsdk files carry `SPDX-License-Identifier: MIT`. Only those files are used;
the rest of sotn-decomp is AGPL-3.0 and is not used here.

> MIT License
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

## Other sources used (same licence terms apply)

- psy-q-decomp, https://github.com/sozud/psy-q-decomp — MIT (but `src/press/libpress.c` is AGPL-3.0-or-later and is NOT used)
- PSY-Z, https://github.com/Xeeynamo/psyz — MIT
- lsddecomp (LSD), https://github.com/brian-oblivion/lsddecomp — CC0-1.0, no conditions

Copyright lines for the MIT sources used:

- sotn-decomp psxsdk: Copyright (c) github.com/xeeynamo/sotn-decomp contributors (per-file SPDX MIT)
- psy-q-decomp: Copyright (c) 2024 the psy-q-decomp contributors
- PSY-Z: Copyright (c) 2024 github.com/psyqz members; Copyright (c) 2025 github.com/xeeynamo/sotn-decomp contributors

Each `src/semantic/sdk_*.c` and `src/semantic/lib_r*.c` file names the exact source files it adapts in its header comment.
