set -e
cd "$(dirname "$0")"
rm -rf gcc-2.6.3
tar xzf gcc-2.6.3.tar.gz
cd gcc-2.6.3
sed -i 's/include <varargs.h>/include <stdarg.h>/g' *.c
patch -u -p1 obstack.h -i ../patches/obstack-2.7.2.h.patch
patch -u -p1 sdbout.c -i ../patches/sdbout-2.6.3.c.patch
patch -u -p1 cp/g++.c -i ../patches/g++-2.6.3.c.patch
patch -su -p1 < ../patches/psx-2.5.7.patch
chmod u+w config/mips/mips.c; patch -u -p0 < ../patches/win32-mips-tempfile.patch
touch -c cp/parse.y cp/parse.h cp/parse.c; touch insn-config.h
./configure --target=mips-sony-psx --prefix=/opt/cross --with-endian-little --with-gnu-as --host=i386-pc-linux --build=i386-pc-linux 2>&1 | tail -20
