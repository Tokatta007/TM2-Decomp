# Old GCC compilers for the Sony SDK libraries

Most of TM2 builds with the PsyQ compilers in `toolchain/psyq-candidates`.
Sony's **libsnd**, however, was built with **GCC 2.5.7**, which no PsyQ
package ships. These scripts build native Windows `cc1.exe` binaries for it
(and for GCC 2.6.3, kept for comparison) under MSYS2/MinGW. Nothing is
installed system-wide.

The results go to `toolchain/gcc-2.5.7-psx/cc1.exe` and
`toolchain/gcc-2.6.3-psx/cc1.exe`. `tools/match_unit.py` uses them as
`--compiler gcc2.5.7` / `gcc2.6.3`, and so does `build.py` through the
manifest's `compiler` key.

## Steps

1. **Get MSYS2.** Extract `msys2-base-x86_64-latest.sfx.exe`
   (https://github.com/msys2/msys2-installer) to `toolchain/msys64`. In its
   shell, run
   `pacman -S mingw-w64-i686-gcc make patch bison flex diffutils`.
2. **Get the sources.** Put the GCC source next to the scripts in
   `toolchain/build-gcc257/`:
   - 2.5.7: http://www.nic.funet.fi/index/gnu/funet/historical-funet-gnu-area-from-early-1990s/gcc-2.5.7.tar.gz
   - 2.6.3: https://mirrors.slackware.com/slackware/slackware-2.2.0/source/d/gcc/gcc-2.6.3.tar.gz
3. **Get the patches.** Put the decompals/old-gcc patches
   (https://github.com/decompals/old-gcc, `patches/`) in `patches/`. `prep.sh`
   lists exactly which ones are needed. Then add the `win32-*.patch` files
   from this folder.
4. **Build.** In an MSYS2 MINGW32 shell, run `prep.sh`, then `build.sh`.
   They follow the decompals Dockerfiles. Windows needs these additions:
   - `win32shim.c`: `index`, `rindex`, `bcmp`, `bcopy`, `bzero`
   - a MIPS backend temp-file fix (use `%TEMP%`, delete on close)
   - for 2.5.7, `times()` replaced by `clock()`
   - for 2.5.7, `-fcommon`
5. **Install.** Copy the stripped `cc1.exe` to `toolchain/gcc-2.5.7-psx/`.
   It needs only `KERNEL32.dll` and `msvcrt.dll`.

Check the result with `cc1 -quiet -O2 little_endian.c`, which must emit
`lbu $2,0($4)`. The test file is from decompals.
