#!/bin/bash
BIN=lcc


function assert {
    want="$1"
    input="$2"

    echo "${input}" | ./${BIN} - > tmp.s || exit

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

assert 2 'int main() {int x=2; {int x=3;} return x;}'
assert 2 'int main() {int x=2; {int x=3;} {int y=4; return x; }}'
assert 3 'int main() {int x=2; {x=3;} return x;}'
assert 2 'int main(){/* return 1; */ return 2;}'
assert 2 'int main(){// return 1;
return 2;}'
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
assert 55 'int main() {return fibo(9);} int fibo(int n) {if (n<=1)return 1; return fibo(n-2)+fibo(n-1);}'

assert 3 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x);}'
assert 4 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1);}'
assert 5 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2);}'

assert 3 'int main() {int x[2][3]; **x=3; *(*(x+2)+1) = 5; return **x;}'
assert 5 'int main() {int x[2][3]; **x=3; *(*(x+2)+1) = 5; return *(*(x+2)+1);}'

assert 3 'int main() {int x[2]; x[0] = 3; x[1]=4; x[2]=5; return x[0];}'
assert 4 'int main() {int x[2]; x[0] = 3; x[1]=4; x[2]=5; return x[1];}'
assert 5 'int main() {int x[2]; x[0] = 3; x[1]=4; x[2]=5; return x[2];}'
assert 5 'int main() {int x[2]; x[0] = 3; x[1]=4; 2[x]=5; return *(x+2);}'

assert 0 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[0][0];}'
assert 1 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[0][1];}'
assert 2 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[0][2];}'
assert 3 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[1][0];}'
assert 4 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[1][1];}'
assert 5 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[1][2];}'
assert 6 'int main() { int x[2][3]; int *y=x; y[6]=6; return x[2][0];  }'

assert 0 'int x; int main() {return x;}'
assert 5 'int x; int main() {x=5; return x;}'
assert 7 'int x,y; int main() {x=3; y=4; return x+y;}'
assert 8 'int x; int main() {return sizeof x;}'
assert 32 'int x[4]; int main() {return sizeof(x);}'

assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0];  }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1];  }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2];  }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3];  }'

assert 1 'int main() {char x; return sizeof(x);}'
assert 1 'int main() { char x=1; return x;  }'
assert 1 'int main() { char x=1; char y=2; return x;  }'
assert 2 'int main() { char x=1; char y=2; return y;  }'

assert 1 'int main() { char x; return sizeof(x);  }'
assert 10 'int main() { char x[10]; return sizeof(x);  }'
assert 1 'int main() { return sub_char(7, 3, 3);  } int sub_char(char a, char b, char c) { return a-b-c;  }'

assert 97 'int main() {return "abc"[0];}'
assert 98 'int main() {return "abc"[1];}'
assert 99 'int main() {return "abc"[2];}'
assert 0 'int main() {return "abc"[3];}'
assert 4 'int main() {return sizeof("abc");}'

assert 7 'int main() { return "\a"[0];  }'
assert 8 'int main() { return "\b"[0];  }'
assert 9 'int main() { return "\t"[0];  }'
assert 10 'int main() { return "\n"[0];  }'
assert 11 'int main() { return "\v"[0];  }'
assert 12 'int main() { return "\f"[0];  }'
assert 13 'int main() { return "\r"[0];  }'
assert 27 'int main() { return "\e"[0];  }'

assert 106 'int main() { return "\j"[0];  }'
assert 107 'int main() { return "\k"[0];  }'
assert 108 'int main() { return "\l"[0];  }'

assert 0 'int main() {return "\0"[0];}'
assert 16 'int main() {return "\20"[0];}'
assert 72 'int main() {return "\110"[0];}'
assert 48 'int main() {return "\1100"[1];}'

assert 0 'int main() {return "\x0"[0];}'
assert 16 'int main() {return "\x10"[0];}'
assert 171 'int main() {return "\xab"[0];}'
assert 106 'int main() {return "\xaj"[1];}'

echo OK
