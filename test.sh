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

assert 0 'return 0;'
assert 42 'return 42;'
assert 42 'return 50-10+2;'
assert 42 'return 50 - 10  + 2;'
assert 42 'return (2+4)*7;'
assert 5 'return 300 / 60;'
assert 15 'return -+ -5 * - -3;'
assert 1 'return 3+2 == 5;'
assert 0 'return 3+2 != 5*--1;'
assert 1 'return 1 < 2;'
assert 0 'return 1 >= 2;'
assert 1 'return 1 <= 2;'
assert 0 'return 1 > 2;'
assert 3 '1;2; return 3;'
assert 3 '1; return 3; 2;'
assert 42 'a = 6; b = (3+4); return a*b; b;'

echo OK
