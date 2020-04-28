#!/bin/bash
make lcc
./lcc "$@" > tmp.s
cc -static tmp.s -o tmp
./tmp
echo $?
