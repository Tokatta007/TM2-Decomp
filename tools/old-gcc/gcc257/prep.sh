set -e
cd "$(dirname "$0")"
rm -rf gcc-2.5.7
tar xzf gcc-2.5.7.tar.gz
cd gcc-2.5.7
sed -i 's/include <varargs.h>/include <stdarg.h>/g' *.c
chmod -R u+w .
patch -u -p1 gvarargs.h -i ../patches/gvarargs-2.5.7.h.patch
patch -u -p1 sdbout.c -i ../patches/sdbout-2.6.0.c.patch
patch -u -p1 obstack.h -i ../patches/obstack-2.5.7.h.patch
patch -u -p1 collect2.c -i ../patches/collect2-2.6.0.c.patch
patch -u -p1 cccp.c -i ../patches/cccp-2.5.7.c.patch
patch -u -p1 gcc.c -i ../patches/gcc-2.5.7.c.patch
patch -u -p1 g++.c -i ../patches/g++-2.5.7.c.patch
patch -u -p1 config/mips/mips.h -i ../patches/mips-2.5.7.h.patch
patch -su -p1 < ../patches/psx-2.5.7.patch
patch -u -p0 < ../patches/win32-mips-tempfile-2.5.7.patch
patch -u -p0 < ../patches/win32-toplev-2.5.7.patch
touch insn-config.h
./configure --target=mips-sony-psx --prefix=/opt/cross --with-endian-little --with-gnu-as --host=i386-pc-linux --build=i386-pc-linux 2>&1 | tail -20
