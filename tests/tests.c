// You can format this file with the following one-liner:
// $ perl -i -pe 's{assert\((.*?), (.*), ".*"\);}{($a,$b)=($1,$2); (($c=$2) =~ s/([\\"])/\\\1/g); "assert($a, $b, \"$c\");"}ge' tests/tests.c
//
// line comment

/*
 *  This is a block comment
 */
int printf();
int exit();

int g1, g2[4];
int testno;

int assert(int want, int got, char *code) {
    testno = testno + 1;
    if (want == got) {
        printf("%s => %d\n", code, got);
    } else {
        printf("%d: %s => want %d, got %d\n", testno, code, want, got);
        exit(1);
    }
    return 0;
}
char int_to_char(int x) {return x;}
int *g1ptr() { return &g1; }
int add_as_char(char a, char b) {return a+b;}
int div_long(long a, long b) {return a/b;}
_Bool bool_incl(_Bool x) { return ++x; }
_Bool bool_decl(_Bool x) { return --x; }

static int static_fn() { return 3; }

int ret3() {
    return 3;
    return 5;
}
int ret5() { return 5; }

int add(int x, int y) { return x + y; }
int sub(int x, int y) { return x - y; }

int add6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}

int addx(int *x, int y) { return *x + y; }

int fibo(int n) {
    if (n <= 1) {
        return 1;
    }
    return fibo(n - 2) + fibo(n - 1);
}
int sub_char(char a, char b, char c) { return a - b - c; }
int sub_short(short a, short b, short c) { return a - b - c; }
int sub_long(long a, long b, long c) { return a - b - c; }

int main() {
    testno = 0;
    assert(0, 0, "0");
    assert(3, ({ int x = 3; x; }), "({ int x = 3; x; })");
    assert(2, ({ int x = 2; { int x = 3; } x; }), "({ int x = 2; { int x = 3; } x; })");
    assert(2, ({ int x = 2; { int x = 3; } int y = 4; x; }), "({ int x = 2; { int x = 3; } int y = 4; x; })");
    assert(3, ({ int x = 2; { x = 3; } x; }), "({ int x = 2; { x = 3; } x; })");
    assert(42, 42, "42");
    assert(42, (50 - 10 + 2), "(50 - 10 + 2)");
    assert(42, ((2 + 4) * 7), "((2 + 4) * 7)");
    assert(5, (300 / 60), "(300 / 60)");
    assert(15, (-+-5 * - ( -3 )), "(-+-5 * - ( -3 ))");
    assert(1, (3 + 2 == 5), "(3 + 2 == 5)");
    assert(0, (3 + 2 != 5 * - ( -1 )), "(3 + 2 != 5 * - ( -1 ))");
    assert(1, (1 < 2), "(1 < 2)");
    assert(0, (1 >= 2), "(1 >= 2)");
    assert(1, (1 <= 2), "(1 <= 2)");
    assert(0, (1 > 2), "(1 > 2)");
    assert(3, ({ 1; 2; 3; }), "({ 1; 2; 3; })");
    assert(1, ({ int a = 1; a; }), "({ int a = 1; a; })");
    assert(9, ({ int a = 1; int z = 8; a + z; }),
           "{ int a=1;int z=8; a+z; }");
    assert(42, ({ int a = 6; int b = (3 + 4); a *b; }),
           "{ int a = 6; int b = (3+4);  a*b; }");
    assert(16, ({ int a, c; a = c = 4; a *c; }),
           "{ int a, c; a=c=4;  a*c; }");
    assert(3, ({ int foo = 3; foo; }),
           "{ int foo=3;  foo; }");
    assert(8, ({ int foo, bar; foo = 3; bar = 21; (foo + bar) / foo; }),
           "{ int foo,bar; foo=3; bar=21;  (foo+bar)/foo; }");
    assert(8, ({ int foo123 = 3; int bar = 21; (foo123 + bar) / foo123; }),
           "{ int foo123=3; int bar=21;  (foo123+bar)/foo123; }");
    assert(8, ({ int x; if (8 == 8) x=8; else x=5; x; }),
           "{ if (8==8)  8; else  5; }");
    assert(5, ({int x; if (8 != 8) x=8; else x=5; x; }),
           "{ if (8!=8)  8; else  5; }");
    assert(5, ({ int x; int a = 5; int b = 3; if (a == 5) if (b == 3) x=a; else x=5; x; }),
           "{ int a=5; int b=3; if (a==5) if (b==3)  a; else  5; }");
    assert(55, ({
               int sum = 0;
               int i;
               for (i = 1; i <= 10; i = i + 1)
                   sum = sum + i;
               sum;
           }),
           "{ int sum=0; int i; for (i=1;i<=10;i=i+1) sum = sum+i;  sum; }");
    assert(45, ({
               int sum = 0;
               int i = 5;
               while (sum < 45)
                   sum = sum + i;
               sum;
           }),
           "{ int sum=0; int i=5; while (sum < 45) sum = sum+i;  sum; }");
    assert(45, ({
               int sum = 0;
               int i = 0;
               while (i < 10) {
                   sum = i + sum;
                   i = i + 1;
               }
               sum;
           }),
           "{ int sum=0; int i=0; while (i<10) {sum=i+sum; i=i+1;}  sum;}");
    assert(3, ({
               int x = 3;
               *&x;
           }),
           "{ int x=3;  *&x;  }");
    assert(3, ({
               int x = 3;
               int *y = &x;
               int **z = &y;
               **z;
           }),
           "{ int x=3; int *y=&x; int **z=&y;  **z;  }");
    assert(5, ({
               int x = 3;
               int *y = 5;
               *(&x + 1);
           }),
           "{ int x=3; int *y=5;  *(&x+1);  }");
    assert(5, ({
               int x = 3;
               int *y = &x;
               *y = 5;
               x;
           }),
           "{ int x=3; int *y=&x; *y=5;  x;  }");
    assert(8, ({
               int x = 29;
               int *y = &x;
               sizeof(&y);
           }),
           "{ int x=29; int *y=&x;  sizeof(&y); }");
    assert(16, ({int x[4]; sizeof(x);}), "({int x[4]; sizeof(x);})");
    assert(3, (ret3()), "(ret3())");
    assert(5, (ret5()), "(ret5())");
    assert(8, (add(3, 5)), "(add(3, 5))");
    assert(21, (add6(1, 2, 3, 4, 5, 6)), "(add6(1, 2, 3, 4, 5, 6))");
    assert(55, (fibo(9)), "(fibo(9))");
    assert(3, ({
               int x[3];
               *x = 3;
               *(x + 1) = 4;
               *(x + 2) = 5;
               *(x);
           }),
           " {int x[3]; *x=3; *(x+1)=4; *(x+2)=5;  *(x);}");
    assert(4, ({ int x[3]; *x = 3; *(x + 1) = 4; *(x + 2) = 5; *(x + 1); }),
           " {int x[3]; *x=3; *(x+1)=4; *(x+2)=5;  *(x+1);}");
    assert(5, ({ int x[3]; *x = 3; *(x + 1) = 4; *(x + 2) = 5; *(x + 2); }),
           " {int x[3]; *x=3; *(x+1)=4; *(x+2)=5;  *(x+2);}");
    assert(3, ({ int x[2][3]; **x = 3; *(*(x + 2) + 1) = 5; **x; }),
           " {int x[2][3]; **x=3; *(*(x+2)+1) = 5;  **x;}");
    assert(5, ({ int x[2][3]; **x = 3; *(*(x + 2) + 1) = 5; *(*(x + 2) + 1); }), "({ int x[2][3]; **x = 3; *(*(x + 2) + 1) = 5; *(*(x + 2) + 1); })");
    assert(3, ({ int x[3]; x[0] = 3; x[1] = 4; x[2] = 5; x[0]; }),
           " {int x[3]; x[0] = 3; x[1]=4; x[2]=5;  x[0];}");
    assert(4, ({ int x[3]; x[0] = 3; x[1] = 4; x[2] = 5; x[1]; }),
           " {int x[3]; x[0] = 3; x[1]=4; x[2]=5;  x[1];}");
    assert(5, ({ int x[3]; x[0] = 3; x[1] = 4; x[2] = 5; x[2]; }),
           " {int x[3]; x[0] = 3; x[1]=4; x[2]=5;  x[2];}");
    assert(5, ({ int x[3]; x[0] = 3; x[1] = 4; 2 [x] = 5; *(x + 2); }),
           " {int x[3]; x[0] = 3; x[1]=4; 2[x]=5;  *(x+2);}");
    assert(0, ({ int x[2][3]; x[0][0] = 0; x[0][1] = 1; x[0][2] = 2; x[1][0] = 3; x[1][1] = 4; x[1][2] = 5; x[0][0]; }),
           " {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5;  x[0][0];}");
    assert(1, ({ int x[2][3]; x[0][0] = 0; x[0][1] = 1; x[0][2] = 2; x[1][0] = 3; x[1][1] = 4; x[1][2] = 5; x[0][1]; }),
           " {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5;  x[0][1];}");
    assert(2, ({ int x[2][3]; x[0][0] = 0; x[0][1] = 1; x[0][2] = 2; x[1][0] = 3; x[1][1] = 4; x[1][2] = 5; x[0][2]; }),
           " {int x[2][3];  x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5;   x[0][2];}");
    assert(3, ({ int x[2][3]; x[0][0] = 0; x[0][1] = 1; x[0][2] = 2; x[1][0] = 3; x[1][1] = 4; x[1][2] = 5; x[1][0]; }),
           " {int x[2][3];  x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5;   x[1][0];}");
    assert(4, ({ int x[2][3]; x[0][0] = 0; x[0][1] = 1; x[0][2] = 2; x[1][0] = 3; x[1][1] = 4; x[1][2] = 5; x[1][1]; }),
           " {int x[2][3];  x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5;   x[1][1];}");
    assert(5, ({ int x[2][3]; x[0][0] = 0; x[0][1] = 1; x[0][2] = 2; x[1][0] = 3; x[1][1] = 4; x[1][2] = 5; x[1][2]; }),
           " {int x[2][3];  x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5;   x[1][2];}");
    assert(6, ({ int x[2][3]; int *y = x; y[6] = 6; x[2][0]; }), "({ int x[2][3]; int *y = x; y[6] = 6; x[2][0]; })");
    assert(1, ({ char x; sizeof(x); }), "({ char x; sizeof(x); })");
    assert(1, ({ char x = 1; x; }), "({ char x = 1; x; })");
    assert(1, ({ char x = 1; char y = 2; x; }), "({ char x = 1; char y = 2; x; })");
    assert(2, ({ char x = 1; char y = 2; y; }), "({ char x = 1; char y = 2; y; })");
    assert(1, ({ char x; sizeof(x); }), "({ char x; sizeof(x); })");
    assert(10, ({ char x[10]; sizeof(x); }), "({ char x[10]; sizeof(x); })");
    assert(1, (sub_char(7, 3, 3)), "(sub_char(7, 3, 3))");
    assert(97, ("abc"[0]), "(\"abc\"[0])");
    assert(98, ("abc"[1]), "(\"abc\"[1])");
    assert(99, ("abc"[2]), "(\"abc\"[2])");
    assert(0, ("abc"[3]), "(\"abc\"[3])");
    assert(4, (sizeof("abc")), "(sizeof(\"abc\"))");
    assert(7, ("\a"[0]), "(\"\\a\"[0])");
    assert(8, ("\b"[0]), "(\"\\b\"[0])");
    assert(9, ("\t"[0]), "(\"\\t\"[0])");
    assert(10, ("\n"[0]), "(\"\\n\"[0])");
    assert(11, ("\v"[0]), "(\"\\v\"[0])");
    assert(12, ("\f"[0]), "(\"\\f\"[0])");
    assert(13, ("\r"[0]), "(\"\\r\"[0])");
    assert(27, ("\e"[0]), "(\"\\e\"[0])");
    assert(106, ("\j"[0]), "(\"\\j\"[0])");
    assert(107, ("\k"[0]), "(\"\\k\"[0])");
    assert(108, ("\l"[0]), "(\"\\l\"[0])");
    assert(0, ("\0"[0]), "(\"\\0\"[0])");
    assert(16, ("\20"[0]), "(\"\\20\"[0])");
    assert(72, ("\110"[0]), "(\"\\110\"[0])");
    assert(48, ("\1100"[1]), "(\"\\1100\"[1])");
    assert(0, ("\x0"[0]), "(\"\\x0\"[0])");
    assert(16, ("\x10"[0]), "(\"\\x10\"[0])");
    assert(10, ("\x0a"[0]), "(\"\\x0a\"[0])");
    assert(106, ("\xaj"[1]), "(\"\\xaj\"[1])");

    assert(35, ({struct {int a; char b;} x; x.a = 35; x.b = 4; x.a;}), "({struct {int a; char b;} x; x.a = 35; x.b = 4; x.a;})");
    assert(4, ({struct {int a; char b;} x; x.a = 35; x.b = 4; x.b;}), "({struct {int a; char b;} x; x.a = 35; x.b = 4; x.b;})");

    assert(4, sizeof(g1), "sizeof(g1)");
    assert(2, ({ int x[5]; int *y=x+2; y-x; }), "({ int x[5]; int *y=x+2; y-x; })");

    assert(1, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.a; }), "({ struct {int a; int b;} x; x.a=1; x.b=2; x.a; })");
    assert(2, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.b; }), "({ struct {int a; int b;} x; x.a=1; x.b=2; x.b; })");
    assert(1, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.a; }), "({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.a; })");
    assert(2, ({ struct {char a; int b; char c;} x; x.b=1; x.b=2; x.c=3; x.b; }), "({ struct {char a; int b; char c;} x; x.b=1; x.b=2; x.c=3; x.b; })");
    assert(3, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.c; }), "({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.c; })");

    assert(0, ({ struct {int a; int b;} x[3]; int *p=x; p[0]=0; x[0].a; }), "({ struct {int a; int b;} x[3]; int *p=x; p[0]=0; x[0].a; })");
    assert(1, ({ struct {int a; int b;} x[3]; int *p=x; p[1]=1; x[0].b; }), "({ struct {int a; int b;} x[3]; int *p=x; p[1]=1; x[0].b; })");
    assert(2, ({ struct {int a; int b;} x[3]; int *p=x; p[2]=2; x[1].a; }), "({ struct {int a; int b;} x[3]; int *p=x; p[2]=2; x[1].a; })");
    assert(3, ({ struct {int a; int b;} x[3]; int *p=x; p[3]=3; x[1].b; }), "({ struct {int a; int b;} x[3]; int *p=x; p[3]=3; x[1].b; })");

    assert(6, ({ struct {int a[3]; int b[5];} x; int *p=&x; x.a[0]=6; p[0]; }), "({ struct {int a[3]; int b[5];} x; int *p=&x; x.a[0]=6; p[0]; })");
    assert(7, ({ struct {int a[3]; int b[5];} x; int *p=&x; x.b[0]=7; p[3]; }), "({ struct {int a[3]; int b[5];} x; int *p=&x; x.b[0]=7; p[3]; })");

    assert(6, ({ struct { struct { int b; } a; } x; x.a.b=6; x.a.b; }), "({ struct { struct { int b; } a; } x; x.a.b=6; x.a.b; })");

    assert(4, ({ struct {int a;} x; sizeof(x); }), "({ struct {int a;} x; sizeof(x); })");
    assert(8, ({ struct {int a; int b;} x; sizeof(x); }), "({ struct {int a; int b;} x; sizeof(x); })");
    assert(8, ({ struct {int a, b;} x; sizeof(x); }), "({ struct {int a, b;} x; sizeof(x); })");
    assert(12, ({ struct {int a[3];} x; sizeof(x); }), "({ struct {int a[3];} x; sizeof(x); })");
    assert(16, ({ struct {int a;} x[4]; sizeof(x); }), "({ struct {int a;} x[4]; sizeof(x); })");
    assert(24, ({ struct {int a[3];} x[2]; sizeof(x); }), "({ struct {int a[3];} x[2]; sizeof(x); })");
    assert(2, ({ struct {char a; char b;} x; sizeof(x); }), "({ struct {char a; char b;} x; sizeof(x); })");
    assert(8, ({ struct {char a; int b;} x; sizeof(x); }), "({ struct {char a; int b;} x; sizeof(x); })");

    assert(7, ({ int x; int y; char z; char *a=&y; char *b=&z; b-a;  }), "({ int x; int y; char z; char *a=&y; char *b=&z; b-a;  })");
    assert(1, ({ int x; char y; int z; char *a=&y; char *b=&z; b-a;  }), "({ int x; char y; int z; char *a=&y; char *b=&z; b-a;  })");

    assert(8, ({struct t {int a, b;} x; struct t y; sizeof(y);}), "({struct t {int a, b;} x; struct t y; sizeof(y);})");
    assert(2, ({struct t {char x[2];}; {struct t {char x[4];};} struct t y; sizeof(y);}), "({struct t {char x[2];}; {struct t {char x[4];};} struct t y; sizeof(y);})");
    assert(3, ({struct t {char a;} x; x.a=2; int t = 1; t+x.a;}), "({struct t {char a;} x; x.a=2; int t = 1; t+x.a;})");

    assert(3, ({struct t {char a;} x; struct t *y = &x; x.a = 3; y->a;}), "({struct t {char a;} x; struct t *y = &x; x.a = 3; y->a;})");
    assert(3, ({struct t {char a;} x; struct t *y = &x; y->a = 3; x.a;}), "({struct t {char a;} x; struct t *y = &x; y->a = 3; x.a;})");
    assert(8, ({ union { int a; char b[6]; } x; sizeof(x); }), "({ union { int a; char b[6]; } x; sizeof(x); })");
    // 515 = 0x00_00_02_03
    assert(3, ({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; })");
    assert(2, ({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; })");
    assert(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; })");
    assert(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; }), "({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; })");

    assert(3, ({struct t {char a, b;} x; struct t y; x.a = 2; x.b=3; y = x; y.b;}), "({struct t {char a, b;} x; struct t y; x.a = 2; x.b=3; y = x; y.b;})");

    assert(2, ({short x; sizeof(x);}), "({short x; sizeof(x);})");
    assert(4, ({struct {short a; char b;}x; sizeof(x);}), "({struct {short a; char b;}x; sizeof(x);})");

    assert(8, ({long x; sizeof(x);}), "({long x; sizeof(x);})");
    assert(16, ({struct {long a; int b;}x; sizeof(x);}), "({struct {long a; int b;}x; sizeof(x);})");

    assert(1, (sub_short(7, 3, 3)), "(sub_short(7, 3, 3))");
    assert(1, (sub_long(7, 3, 3)), "(sub_long(7, 3, 3))");

    assert(24, ({int *x[3]; sizeof(x);}), "({int *x[3]; sizeof(x);})");
    assert(8, ({int (*x)[3]; sizeof(x);}), "({int (*x)[3]; sizeof(x);})");
    assert(3, ({int *x[3]; int y; x[0]=&y; y=3; x[0][0];}), "({int *x[3]; int y; x[0]=&y; y=3; x[0][0];})");
    assert(4, ({int x[3]; int (*y)[3]=x; y[0][0]=4; y[0][0];}), "({int x[3]; int (*y)[3]=x; y[0][0]=4; y[0][0];})");
    assert(4, ({int x[3]; int (*y)[3]=x; y[0][0]=4; x[0];}), "({int x[3]; int (*y)[3]=x; y[0][0]=4; x[0];})");

    assert(1, ({char x; sizeof(x);}), "({char x; sizeof(x);})");
    assert(2, ({short int x; sizeof(x);}), "({short int x; sizeof(x);})");
    assert(2, ({int short x; sizeof(x);}), "({int short x; sizeof(x);})");
    assert(4, ({int x; sizeof(x);}), "({int x; sizeof(x);})");
    assert(8, ({long int x; sizeof(x);}), "({long int x; sizeof(x);})");
    assert(8, ({int long x; sizeof(x);}), "({int long x; sizeof(x);})");
    assert(8, ({long long int x; sizeof(x);}), "({long long int x; sizeof(x);})");
    assert(8, ({long int long x; sizeof(x);}), "({long int long x; sizeof(x);})");
    assert(8, ({int long long x; sizeof(x);}), "({int long long x; sizeof(x);})");
    assert(8, ({long long x; sizeof(x);}), "({long long x; sizeof(x);})");

    { void *x; }

typedef long int TypeX, *TypeY[4], (*TypeZ)[2];
    assert(4, ({typedef int T; T x; sizeof(x);}), "({typedef int T; T x; sizeof(x);})");
    assert(2, ({short typedef int T; T x; sizeof(x);}), "({short typedef int T; T x; sizeof(x);})");
    assert(8, ({TypeX TypeX; sizeof(TypeX);}), "({TypeX TypeX; sizeof(TypeX);})");
    assert(32, ({TypeY TypeY; sizeof(TypeY);}), "({TypeY TypeY; sizeof(TypeY);})");
    assert(8, ({TypeZ TypeZ; sizeof(TypeZ);}), "({TypeZ TypeZ; sizeof(TypeZ);})");
    assert(1, ({typedef int t; t x=1; x;}), "({typedef int t; t x=1; x;})");
    assert(1, ({typedef char t; { typedef short t; t y; if (sizeof(y) !=2) {printf("NG");}} t x=1; sizeof(x);}), "({typedef char t; { typedef short t; t y; if (sizeof(y) !=2) {printf(\"NG\");}} t x=1; sizeof(x);})");
    assert(1, ({typedef struct t { int a;} t; t x; x.a=1; x.a;}), "({typedef struct t { int a;} t; t x; x.a=1; x.a;})");
    assert(4, ({typedef X; X x; sizeof(x);}), "({typedef X; X x; sizeof(x);})");
    assert(55, ({ int j=0; for (int i=0; i<=10; i=i+1) j=j+i; j;  }), "({ int j=0; for (int i=0; i<=10; i=i+1) j=j+i; j;  })");
    assert(3, ({ int i=3; int j=0; for (int i=0; i<=10; i=i+1) j=j+i; i;  }), "({ int i=3; int j=0; for (int i=0; i<=10; i=i+1) j=j+i; i;  })");

    assert(5, ({int i=3; i+=2; i;}), "({int i=3; i+=2; i;})");
    assert(1, ({int i=3; i-=2; i;}), "({int i=3; i-=2; i;})");
    assert(6, ({int i=3; i*=2; i;}), "({int i=3; i*=2; i;})");
    assert(3, ({int i=6; i/=2; i;}), "({int i=6; i/=2; i;})");

    assert(42, ({int x[6]; x[0] = 3; x[2] = 42; int *y = x; y+=2; *y;}), "({int x[6]; x[0] = 3; x[2] = 42; int *y = x; y+=2; *y;})");

    assert(2, ({int i=1; ++i;}), "({int i=1; ++i;})");
    assert(0, ({int i=1; --i;}), "({int i=1; --i;})");

    assert(2, ({int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p = a+1; ++*p;}), "({int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p = a+1; ++*p;})");
    assert(0, ({int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p = a+1; --*p;}), "({int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p = a+1; --*p;})");

    assert(3, ({1,2,3;}), "({1,2,3;})");

    assert(2, ({int i=1; i++; i;}), "({int i=1; i++; i;})");
    assert(0, ({int i=1; i--; i;}), "({int i=1; i--; i;})");
    assert(1, ({int i=1; i++;}), "({int i=1; i++;})");
    assert(1, ({int i=1; i--;}), "({int i=1; i--;})");

    assert(1, 2&&3, "2&&3");
    assert(0, 0&&3, "0&&3");
    assert(0, 2&&0, "2&&0");
    assert(0, 0&&0, "0&&0");

    assert(1, 2||3, "2||3");
    assert(1, 0||4, "0||4");
    assert(1, 2||0, "2||0");
    assert(0, 0||0, "0||0");

    assert(4, sizeof(int), "sizeof(int)");
    assert(2, sizeof(short int), "sizeof(short int)");
    assert(8, sizeof(int*), "sizeof(int*)");
    assert(24, sizeof(int*[3]), "sizeof(int*[3])");
    assert(8, sizeof(int(*)[3]), "sizeof(int(*)[3])");
    assert(8, sizeof(struct {int a, b;}), "sizeof(struct {int a, b;})");
    assert(131585, (int)8590066177, "(int)8590066177");
    assert(513, (short)8590066177, "(short)8590066177");
    assert(1, (char)8590066177, "(char)8590066177");
    assert(1, (long)1, "(long)1");
    assert(0, (long)&*(int *)0, "(long)&*(int *)0");
    assert(513, ({ int x=512; *(char *)&x=1; x;  }), "({ int x=512; *(char *)&x=1; x;  })");
    assert(5, ({ int x=5; long y=(long)&x; *(int*)y;  }), "({ int x=5; long y=(long)&x; *(int*)y;  })");

    (void)1;

    assert(131585, ({ int x; x=8590066177; x;} ), "({ int x; x=8590066177; x;} )");
    assert(513, ({ short x; x=8590066177; x;} ), "({ short x; x=8590066177; x;} )");
    assert(1, ({ char x; x = 8590066177; x;} ), "({ char x; x = 8590066177; x;} )");

    assert(4, sizeof(-10+5), "sizeof(-10+5)");
    assert(4, sizeof(-10-5), "sizeof(-10-5)");
    assert(4, sizeof(-10*5), "sizeof(-10*5)");
    assert(4, sizeof(-10/5), "sizeof(-10/5)");

    assert(8, sizeof(-10+(long)5), "sizeof(-10+(long)5)");
    assert(8, sizeof(-10-(long)5), "sizeof(-10-(long)5)");
    assert(8, sizeof(-10*(long)5), "sizeof(-10*(long)5)");
    assert(8, sizeof(-10/(long)5), "sizeof(-10/(long)5)");
    assert(8, sizeof((long)-10+5), "sizeof((long)-10+5)");
    assert(8, sizeof((long)-10-5), "sizeof((long)-10-5)");
    assert(8, sizeof((long)-10*5), "sizeof((long)-10*5)");
    assert(8, sizeof((long)-10/5), "sizeof((long)-10/5)");

    assert((long)-5, -10 + (long)5, "-10 + (long)5");
    assert((long)-15, -10 - (long)5, "-10 - (long)5");
    assert((long)-50, -10 * (long)5, "-10 * (long)5");
    assert((long)-2, -10 / (long)5, "-10 / (long)5");
    assert((int)-5, -10 + 5, "-10 + 5");

    assert(0, 2147483647 + 2147483647 + 2, "2147483647 + 2147483647 + 2");
    assert(1, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[0];  }), "({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[0];  })");
    assert(0, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[-1];  }), "({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[-1];  })");

    assert(5, int_to_char(261), "int_to_char(261)");
    assert(3, ({g1 = 3; *g1ptr(); }), "({g1 = 3; *g1ptr(); })");

    assert(10, add_as_char(261, 261), "add_as_char(261, 261)");
    assert(-5, div_long(-10, 2), "div_long(-10, 2)");

    assert(0, ({_Bool x=0; x;}), "({_Bool x=0; x;})");
    assert(1, ({_Bool x=1; x;}), "({_Bool x=1; x;})");
    assert(1, ({_Bool x=100; x;}), "({_Bool x=100; x;})");
    assert(0, (_Bool)0, "(_Bool)0");
    assert(1, (_Bool)1, "(_Bool)1");
    assert(1, (_Bool)10, "(_Bool)10");

    assert(1, bool_incl(0), "bool_incl(0)");
    assert(1, bool_incl(1), "bool_incl(1)");
    assert(1, bool_incl(2), "bool_incl(2)");
    assert(1, bool_decl(0), "bool_decl(0)");
    assert(0, bool_decl(1), "bool_decl(1)");
    assert(0, bool_decl(2), "bool_decl(2)");

    assert(3, static_fn(), "static_fn()");

    assert(97, 'a', "'a'");
    assert(10, '\n', "'\\n'");
    assert(4, sizeof('\n'), "sizeof('\\n')");

    assert(0, ({enum {zero, one, two}; zero;}), "({enum {zero, one, two}; zero;})");
    assert(1, ({enum {zero, one, two}; one;}), "({enum {zero, one, two}; one;})");
    assert(2, ({enum {zero, one, two}; two;}), "({enum {zero, one, two}; two;})");
    assert(10, ({enum {ten=10, eleven, twelve}; ten;}), "({enum {ten=10, eleven, twelve}; ten;})");
    assert(11, ({enum {ten=10, eleven, twelve}; eleven;}), "({enum {ten=10, eleven, twelve}; eleven;})");
    assert(12, ({enum {ten=10, eleven, twelve}; twelve;}), "({enum {ten=10, eleven, twelve}; twelve;})");
    assert(0, ({enum {zero, one, two, five=5, three=3, four}; zero;}), "({enum {zero, one, two, five=5, three=3, four}; zero;})");
    assert(5, ({enum {zero, one, two, five=5, three=3, four}; five;}), "({enum {zero, one, two, five=5, three=3, four}; five;})");
    assert(4, ({enum {zero, one, two, five=5, three=3, four}; four;}), "({enum {zero, one, two, five=5, three=3, four}; four;})");
    assert(4, ({enum {zero, one, two}; sizeof two;}), "({enum {zero, one, two}; sizeof two;})");
    assert(4, ({enum T {zero, one, two}; enum T y; sizeof(y);}), "({enum T {zero, one, two}; enum T y; sizeof(y);})");

    assert(1, 10%3, "10%3");
    assert(4, sizeof( 10%3 ), "sizeof( 10%3 )");
    assert(1, ({int x=10; x%=3; x;}), "({int x=10; x%=3; x;})");
    assert(4, ({int x=10; x%=3; sizeof x;}), "({int x=10; x%=3; sizeof x;})");
    assert(1, ({long x=10; x%=3; x;}), "({long x=10; x%=3; x;})");
    assert(8, ({long x=10; x%=3; sizeof x;}), "({long x=10; x%=3; sizeof x;})");
    assert(2, ({short int x=10; x%=3; sizeof x;}), "({short int x=10; x%=3; sizeof x;})");
    assert(1, ({char x=10; x%=3; sizeof x;}), "({char x=10; x%=3; sizeof x;})");

    assert(3, 7&3, "7&3");
    assert(7, 7|3, "7|3");
    assert(4, 7^3, "7^3");
    assert(4, sizeof(7&3), "sizeof(7&3)");
    assert(4, sizeof(7|3), "sizeof(7|3)");
    assert(4, sizeof(7^3), "sizeof(7^3)");
    assert(8, sizeof(7&(long)3), "sizeof(7&(long)3)");
    assert(8, sizeof(7|(long)3), "sizeof(7|(long)3)");
    assert(8, sizeof(7^(long)3), "sizeof(7^(long)3)");
    assert(2, ({int x=6; x&=3; x;}), "({int x=6; x&=3; x;})");
    assert(7, ({int x=6; x|=3; x;}), "({int x=6; x|=3; x;})");
    assert(5, ({int x=6; x^=3; x;}), "({int x=6; x^=3; x;})");

    assert(1, !0, "!0");
    assert(0, !1, "!1");
    assert(0, !2, "!2");
    assert(4, sizeof(!0), "sizeof(!0)");
    assert(4, sizeof(!(char)0), "sizeof(!(char)0)");
    assert(4, sizeof(!(long)0), "sizeof(!(long)0)");
    assert(-1, ~0, "~0");
    assert(-2, ~1, "~1");
    assert(0, ~-1, "~-1");

    printf("OK\n");
    return 0;
}
