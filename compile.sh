#!/bin/bash
make lcc
echo "$@" | ./lcc - > tmp.s || exit
cc -static tmp.s -o tmp
./tmp
echo $?
