#!/bin/bash
make lcc
./lcc "$@" >tmp.s || exit
cc -static tmp.s -o tmp
./tmp
echo $?
