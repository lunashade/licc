#!/bin/bash
set -e

TMP=$1
CC=$2
OUT=$3

rm -rf $TMP
mkdir -p $TMP

lcc() {
    src=$1
    obj=$(basename "${src%.c}.o")
    $CC -c -o $TMP/${obj} $src
}

for f in src/*.c; do
    lcc "$f"
done

(cd $TMP; gcc -o ../$OUT *.o)
