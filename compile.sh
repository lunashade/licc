#!/bin/bash
set -e
make lcc
./lcc "$@" > tmp.s
cc tmp.s -o tmp
./tmp
echo $?
