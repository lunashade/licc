#!/bin/bash
make lcc
./lcc "$@" > tmp.s
cc tmp.s -o tmp
./tmp
echo $?
