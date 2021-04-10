#!/bin/bash

rm gido.o*
xcrun -sdk iphoneos clang -arch armv7 gido.c -o gido.o -c -fno-stack-protector -nostdlib -Wno-incompatible-library-redeclaration
if [ $? -ne 0 ]; then
echo "compilation failed"
exit 1
fi

jtool -e __TEXT.__text gido.o >/dev/null 2>/dev/null
jtool -e __DATA.__data gido.o >/dev/null 2>/dev/null
jtool -e __TEXT.__cstring gido.o >/dev/null 2>/dev/null
jtool -e __TEXT.__const gido.o >/dev/null 2>/dev/null
jtool -e __TEXT.__literal8 gido.o >/dev/null 2>/dev/null
jtool -e __TEXT.__literal16 gido.o >/dev/null 2>/dev/null
python2 buildexploit.py
