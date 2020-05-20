#!/bin/bash
set -e

TMP=$1
CC=$2
OUT=$3

rm -rf $TMP
mkdir -p $TMP

licc() {
    src="$1"
    obj="$TMP/$(basename $src .c).o"
    $CC -c -o "$obj" "$src"
}

for f in src/*.c; do
    licc "$f"
done

(cd $TMP; gcc -o ../$OUT *.o)
