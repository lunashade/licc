#!/bin/bash

cd $(dirname $0)/..

make bin/licc
bin/licc -c examples/dp_a/main.c -o tmp.o
gcc -o tmp_dp tmp.o

for i in {1..3}; do
    got=$(cat "examples/dp_a/sample-$i.in" | ./tmp_dp)
    want=$(cat "examples/dp_a/sample-$i.out")
    if [[ "$got" == "$want" ]]; then
        echo "PASSED: $i"
    else
        echo "FAIL: $i: want $want, got $got"
    fi
done
