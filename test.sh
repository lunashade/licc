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
        echo -e "🎉 $input => $want"
    else
        echo -e "$input => want $want, but got $got"
        exit 1
    fi
}

assert 0 '{ return 0; }'
assert 42 '{ return 42; }'
assert 42 '{ return 50-10+2; }'
assert 42 '{ return 50 - 10  + 2; }'
assert 42 '{ return (2+4)*7; }'
assert 5 '{ return 300 / 60; }'
assert 15 '{ return -+ -5 * - -3; }'
assert 1 '{ return 3+2 == 5; }'
assert 0 '{ return 3+2 != 5*--1; }'
assert 1 '{ return 1 < 2; }'
assert 0 '{ return 1 >= 2; }'
assert 1 '{ return 1 <= 2; }'
assert 0 '{ return 1 > 2; }'
assert 3 '{ 1;2; return 3; }'
assert 3 '{ 1; return 3; 2; }'
assert 1 '{ int a=1;return a; }'
assert 9 '{ int a=1;int z=8;return a+z; }'
assert 42 '{ int a = 6; int b = (3+4); return a*b; b; }'
assert 16 '{ int a, c; a=c=4; return a*c; }'
assert 3 '{ int foo=3; return foo; }'
assert 8 '{ int foo,bar; foo=3; bar=21; return (foo+bar)/foo; }'
assert 8 '{ int foo123=3; int bar=21; return (foo123+bar)/foo123; }'
assert 8 '{ if (8==8) return 8; return 5; }'
assert 5 '{ if (8!=8) return 8; return 5; }'
assert 8 '{ if (8==8) return 8; else return 5; }'
assert 5 '{ if (8!=8) return 8; else return 5; }'
assert 5 '{ int a=5; int b=3; if (a==5) if (b==3) return a; else return 5; }'
assert 55 '{ int sum=0; int i; for (i=1;i<=10;i=i+1) sum = sum+i; return sum; }'
assert 45 '{ int sum=0; int i=5; while (sum < 45) sum = sum+i; return sum; }'
assert 45 '{ int sum=0; int i=0; while (i<10) {sum=i+sum; i=i+1;} return sum;}'
assert 3 '{ int x=3; return *&x;  }'
assert 3 '{ int x=3; int *y=&x; int **z=&y; return **z;  }'
assert 5 '{ int x=3; int *y=5; return *(&x+1);  }'
assert 3 '{ int x=3; int *y=5; return *(&y-1);  }'
assert 5 '{ int x=3; int *y=&x; *y=5; return x;  }'
assert 7 '{ int x=3; int *y=5; *(&x+1)=7; return y;  }'
assert 7 '{ int x=3; int *y=5; *(&y-1)=7; return x;  }'
assert 8 '{ return sizeof( 12 ); }'
assert 8 '{ int x=29; int *y=&x; return sizeof(&y); }'

echo OK
