#!/bin/bash

cd $(dirname $0)/..

make lcc
./lcc examples/dp_a/main.c > tmp.s || exit
cc -static -o tmp_dp tmp.s

for i in {1..3}; do
    got=$(cat "examples/dp_a/sample-$i.in" | ./tmp_dp)
    want=$(cat "examples/dp_a/sample-$i.out")
    if [[ "$got" == "$want" ]]; then
        echo "PASSED: $i"
    else
        echo "FAIL: $i: want $want, got $got"
    fi
done
