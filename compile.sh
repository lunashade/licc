#!/bin/bash
BIN=lcc-stage2
make $BIN
./$BIN "$@" >tmp.s || exit
cc -static tmp.s -o tmp
./tmp
echo $?
