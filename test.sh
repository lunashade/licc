#!/bin/bash
BIN=lcc

function assert {
    want="$1"
    input="$2"

    ./${BIN} "${input}" > tmp.s
    gcc tmp.s -o tmp
    ./tmp
    got="$?"

    if [[ "$got" == "$want" ]]; then
        echo "$input => $want"
    else
        echo "$input => want $want, but got $got"
        exit 1
    fi
}

assert 0 0
assert 42 42
assert 42 50-10+2
echo OK
