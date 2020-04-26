#!/bin/bash
BIN=lcc


function assert {
    want="$1"
    input="$2"

    ./${BIN} "${input}" > tmp.s

    gcc -static -o tmp tmp.s tmp2.o
    ./tmp
    got="$?"

    if [[ "$got" == "$want" ]]; then
        echo -e "🎉 $input => $want"
    else
        echo -e "$input => want $want, but got $got"
        exit 1
    fi
}

function ready {
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3;  }
int ret5() { return 5;  }
int add(int x, int y) { return x+y;  }
int sub(int x, int y) { return x-y;  }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF
}

ready
assert 0 'int main(){ return 0; }'
assert 42 'int main(){ return 42; }'
assert 42 'int main(){ return 50-10+2; }'
assert 42 'int main(){ return 50 - 10  + 2; }'
assert 42 'int main(){ return (2+4)*7; }'
assert 5 'int main(){ return 300 / 60; }'
assert 15 'int main(){ return -+ -5 * - -3; }'
assert 1 'int main(){ return 3+2 == 5; }'
assert 0 'int main(){ return 3+2 != 5*--1; }'
assert 1 'int main(){ return 1 < 2; }'
assert 0 'int main(){ return 1 >= 2; }'
assert 1 'int main(){ return 1 <= 2; }'
assert 0 'int main(){ return 1 > 2; }'
assert 3 'int main(){ 1;2; return 3; }'
assert 3 'int main(){ 1; return 3; 2; }'
assert 1 'int main(){ int a=1;return a; }'
assert 9 'int main(){ int a=1;int z=8;return a+z; }'
assert 42 'int main(){ int a = 6; int b = (3+4); return a*b; b; }'
assert 16 'int main(){ int a, c; a=c=4; return a*c; }'
assert 3 'int main(){ int foo=3; return foo; }'
assert 8 'int main(){ int foo,bar; foo=3; bar=21; return (foo+bar)/foo; }'
assert 8 'int main(){ int foo123=3; int bar=21; return (foo123+bar)/foo123; }'
assert 8 'int main(){ if (8==8) return 8; return 5; }'
assert 5 'int main(){ if (8!=8) return 8; return 5; }'
assert 8 'int main(){ if (8==8) return 8; else return 5; }'
assert 5 'int main(){ if (8!=8) return 8; else return 5; }'
assert 5 'int main(){ int a=5; int b=3; if (a==5) if (b==3) return a; else return 5; }'
assert 55 'int main(){ int sum=0; int i; for (i=1;i<=10;i=i+1) sum = sum+i; return sum; }'
assert 45 'int main(){ int sum=0; int i=5; while (sum < 45) sum = sum+i; return sum; }'
assert 45 'int main(){ int sum=0; int i=0; while (i<10) {sum=i+sum; i=i+1;} return sum;}'
assert 3 'int main(){ int x=3; return *&x;  }'
assert 3 'int main(){ int x=3; int *y=&x; int **z=&y; return **z;  }'
assert 5 'int main(){ int x=3; int *y=5; return *(&x+1);  }'
assert 3 'int main(){ int x=3; int *y=5; return *(&y-1);  }'
assert 5 'int main(){ int x=3; int *y=&x; *y=5; return x;  }'
assert 7 'int main(){ int x=3; int *y=5; *(&x+1)=7; return y;  }'
assert 7 'int main(){ int x=3; int *y=5; *(&y-1)=7; return x;  }'
assert 8 'int main(){ return sizeof( 12 ); }'
assert 8 'int main(){ int x=29; int *y=&x; return sizeof(&y); }'
assert 3 'int main(){ return ret3();  }'
assert 5 'int main(){ return ret5();  }'
assert 8 'int main(){ return add(3,5); }'
assert 21 'int main(){ return add6(1,2,3,4,5,6); }'
assert 4 'int main(){return ret4();} int ret4(){return 4;}'

echo OK
