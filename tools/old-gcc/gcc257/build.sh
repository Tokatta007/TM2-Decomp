set -e
cd "$(dirname "$0")"
gcc -O2 -c win32shim.c -o gcc-2.5.7/win32shim.o
cd gcc-2.5.7
make cc1 CC=gcc CLIB=win32shim.o CFLAGS='-std=gnu89 -m32 -static -Dbsd4_4 -Dmips -march=i686 -DHAVE_STRERROR -w -fcommon'
