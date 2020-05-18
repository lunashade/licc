char *main_fn = __FILE__;
int main_ln = __LINE__;
// You can format this file with the following one-liner:
// $ perl -i -pe 's{assert\((.*?), (.*), ".*"\);}{($a,$b)=($1,$2); (($c=$2) =~ s/([\\"])/\\\1/g); "assert($a, $b, \"$c\");"}ge' tests/tests.c
//
// line comment

/*
 *  This is a block comment
 */
#include "include1.h"
#include "include3.h"
#include "include4.h"
int printf();
int exit();
typedef struct {
    int gp_offset;
    int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];
int;
struct {char a; int b;};
int あ = 3;
const int 🍣 = 42;
typedef struct {char a; int b;} Ty1;
int add_all1(int x, ...);
int add_all3(int x, int y, int z,...);
int strcmp(char *p, char *q);
int memcmp(char *, char *);
int sprintf(char *buf, char *fmt, ...);
int vsprintf(char *buf, char *fmt, va_list ap);
char *fmt(char *buf, char *fmt, ...) {
    va_list ap;
    __builtin_va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
}

char *func_fn(void) {
    return __func__;
}

int _Alignas(512) g_aligned1;
int _Alignas(512) g_aligned2;

int g1, g2[4];

int g3 = 3;
char g4 = 4;
int g5[5] = {1,2,3,4,5};
Ty1 g6 = {'a', 98};
short g7 = 7;
long g8 = 8;

extern int ext1;
extern int *ext2;
static int ext3 = 3;
float add_float(float x, float y);
double add_double(double x, double y);

float add_float3(float x, float y, float z) {
    return x + y + z;
}
double sub_double(double x, float y) {
    return x - y;
}
int M13(int x) {
    return x*x;
}

int add2(int x, int y) {
    return x+y;
}
struct {int a[2];} g9[2] = {{{9, 10}}};

_Bool true_fn();
_Bool false_fn();

char g10[] = "foobar";
char g11[10] = "foobar";
char g12[3] = "foobar";

char *g13 = g10 + 0;
char *g14 = g10 + 3;
char *g15 = &g10 - 3;
char *g16[] = {g10+0, g10+3, g10-3};
int g17 = 3;
int *g18 = &g17;
int g19[3] = {1,2,3};
int *g20 = g19 + 1;

struct {int a[2];} g30[2] = {{1, 2}, 3, 4};
struct {int a[2];} g31[2] = {1, 2, 3, 4};
char g32[][4] = {'f', 'o', 'o', 0, 'b', 'a', 'r', 0};

char *g34 = {"foo"};

float g35 = 1.5;
double g36 = 0.0 ? 55 : (0, 1+1 * 5.0/2 * (double)2  *(int)2.0);

typedef struct Tree {
    int val;
    struct Tree *lhs;
    struct Tree *rhs;
} Tree;

Tree *tree = &(Tree){
    1,
    &(Tree){
        2,
        &(Tree){3,0,0},
        &(Tree){4,0,0},
    },
    0,
};

int counter(void) {
    static int i;
    return ++i;
}

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

int ret3(void) {
    return 3;
    return 5;
}
int ret5(void) { return 5; }

int (*fnptr(void))(int) {
    return ret3;
}

void ret_none() {return;}

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

    assert(3, 0b11, "0b11");
    assert(17, 0x11, "0x11");
    assert(171, 0xab, "0xab");
    assert(9, 011, "011");

    assert(3, ({int i=0; goto a; a: i++; b: i++; c: i++; i;}), "({int i=0; goto a; a: i++; b: i++; c: i++; i;})");
    assert(2, ({int i=0; goto e; d: i++; e: i++; f: i++; i;}), "({int i=0; goto e; d: i++; e: i++; f: i++; i;})");
    assert(1, ({int i=0; goto i; g: i++; h: i++; i: i++; i;}), "({int i=0; goto i; g: i++; h: i++; i: i++; i;})");

    assert(10, ({int i=0; while(1) {if (i==10) break; i++;} i;}), "({int i=0; while(1) {if (i==10) break; i++;} i;})");
    assert(10, ({int i; for(i=0; i>=0; i++) {if (i==10) break;} i;}), "({int i; for(i=0; i>=0; i++) {if (i==10) break;} i;})");

    assert(10, ({int i=0; int j=0; for(;i<10;i++){if (i>5)continue; j++;} i;}), "({int i=0; int j=0; for(;i<10;i++){if (i>5)continue; j++;} i;})");
    assert(6, ({int i=0; int j=0; for(;i<10;i++){if (i>5)continue; j++;} j;}), "({int i=0; int j=0; for(;i<10;i++){if (i>5)continue; j++;} j;})");

    assert(3, ({ int i=0; for(;i<10;i++) { if (i == 3) break;  } i;  }), "({ int i=0; for(;i<10;i++) { if (i == 3) break;  } i;  })");
    assert(4, ({ int i=0; while (1) { if (i++ == 3) break;  } i;  }), "({ int i=0; while (1) { if (i++ == 3) break;  } i;  })");
    assert(3, ({ int i=0; for(;i<10;i++) { for (;;) break; if (i == 3) break;  } i;  }), "({ int i=0; for(;i<10;i++) { for (;;) break; if (i == 3) break;  } i;  })");
    assert(4, ({ int i=0; while (1) { while(1) break; if (i++ == 3) break;  } i;  }), "({ int i=0; while (1) { while(1) break; if (i++ == 3) break;  } i;  })");

    assert(10, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++;  } i;  }), "({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++;  } i;  })");
    assert(6, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++;  } j;  }), "({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++;  } j;  })");
    assert(10, ({ int i=0; int j=0; for(;!i;) { for (;j!=10;j++) continue; break;  } j;  }), "({ int i=0; int j=0; for(;!i;) { for (;j!=10;j++) continue; break;  } j;  })");
    assert(11, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++;  } i;  }), "({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++;  } i;  })");
    assert(5, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++;  } j;  }), "({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++;  } j;  })");
    assert(11, ({ int i=0; int j=0; while(!i) { while (j++!=10) continue; break;  } j;  }), "({ int i=0; int j=0; while(!i) { while (j++!=10) continue; break;  } j;  })");

    assert(8, 1<<3, "1<<3");
    assert(1, 8>>3, "8>>3");
    assert(-1, -8>>3, "-8>>3");
    assert(4, sizeof( -8>>3 ), "sizeof( -8>>3 )");
    assert(4, sizeof( -8>>(long)3 ), "sizeof( -8>>(long)3 )");
    assert(4, sizeof( -8>>(char)3 ), "sizeof( -8>>(char)3 )");
    assert(8, sizeof( (long)-8>>3 ), "sizeof( (long)-8>>3 )");
    assert(1, sizeof( (char)-8>>3 ), "sizeof( (char)-8>>3 )");

    assert(-8, ({int i=-1; i<<=3; i;}), "({int i=-1; i<<=3; i;})");
    assert(-1, ({int i=-8; i>>=3; i;}), "({int i=-8; i>>=3; i;})");

    assert(5, ({int i=0; int j; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;}), "({int i=0; int j; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;})");
    assert(6, ({int i=1; int j; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;}), "({int i=1; int j; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;})");
    assert(7, ({int i=2; int j; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;}), "({int i=2; int j; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;})");
    assert(0, ({int i=3; int j=0; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;}), "({int i=3; int j=0; switch(i){case 0:j=5;break;case 1:j=6;break;case 2:j=7;break;} j;})");
    assert(7, ({int i=3; int j; switch(i){case 0:j=5;break;case 1:j=6;break;default:j=7;break;} j;}), "({int i=3; int j; switch(i){case 0:j=5;break;case 1:j=6;break;default:j=7;break;} j;})");

    assert(2, 0?1:2, "0?1:2");
    assert(1, 1?1:2, "1?1:2");
    assert(-1, 0?-2:-1, "0?-2:-1");
    assert(-2, 1?-2:-1, "1?-2:-1");
    assert(4, sizeof(0?1:2), "sizeof(0?1:2)");
    assert(8, sizeof(0?(long)1:(long)2), "sizeof(0?(long)1:(long)2)");
    assert(-1, 0?(long)-2:-1, "0?(long)-2:-1");
    assert(-1, 0?-2:(long)-1, "0?-2:(long)-1");
    assert(-2, 1?(long)-2:-1, "1?(long)-2:-1");
    assert(-2, 1?-2:(long)-1, "1?-2:(long)-1");

    1 ? -2 : (void)-1;

    assert(8, sizeof(int(*)[10]), "sizeof(int(*)[10])");
    assert(8, sizeof(int(*)[]), "sizeof(int(*)[])");

    assert(8, ({struct foo *bar; sizeof(bar);}), "({struct foo *bar; sizeof(bar);})");
    assert(4, ({struct T *bar; struct T {int x;}; sizeof(struct T);}), "({struct T *bar; struct T {int x;}; sizeof(struct T);})");
    assert(4, ({typedef struct T T; struct T {int x;}; sizeof(T);}), "({typedef struct T T; struct T {int x;}; sizeof(T);})");
    assert(1, ({struct T {struct T *next; int x;} a; struct T b; b.x = 1; a.next=&b; a.next->x;}), "({struct T {struct T *next; int x;} a; struct T b; b.x = 1; a.next=&b; a.next->x;})");

    assert(10, ({ enum { ten=1+2+3+4  }; ten;  }), "({ enum { ten=1+2+3+4  }; ten;  })");
    assert(1, ({ int i=0; switch(3) { case 5-2+0*3: i++;  } i;  }), "({ int i=0; switch(3) { case 5-2+0*3: i++;  } i;  })");
    assert(8, ({ int x[1+1]; sizeof(x);  }), "({ int x[1+1]; sizeof(x);  })");
    assert(6, ({ char x[8-2]; sizeof(x);  }), "({ char x[8-2]; sizeof(x);  })");
    assert(6, ({ char x[2*3]; sizeof(x);  }), "({ char x[2*3]; sizeof(x);  })");
    assert(3, ({ char x[12/4]; sizeof(x);  }), "({ char x[12/4]; sizeof(x);  })");
    assert(0b100, ({ char x[0b110&0b101]; sizeof(x);  }), "({ char x[0b110&0b101]; sizeof(x);  })");
    assert(0b111, ({ char x[0b110|0b101]; sizeof(x);  }), "({ char x[0b110|0b101]; sizeof(x);  })");
    assert(0b110, ({ char x[0b111^0b001]; sizeof(x);  }), "({ char x[0b111^0b001]; sizeof(x);  })");
    assert(4, ({ char x[1<<2]; sizeof(x);  }), "({ char x[1<<2]; sizeof(x);  })");
    assert(2, ({ char x[4>>1]; sizeof(x);  }), "({ char x[4>>1]; sizeof(x);  })");
    assert(2, ({ char x[(1==1)+1]; sizeof(x);  }), "({ char x[(1==1)+1]; sizeof(x);  })");
    assert(1, ({ char x[(1!=1)+1]; sizeof(x);  }), "({ char x[(1!=1)+1]; sizeof(x);  })");
    assert(1, ({ char x[(1<1)+1]; sizeof(x);  }), "({ char x[(1<1)+1]; sizeof(x);  })");
    assert(2, ({ char x[(1<=1)+1]; sizeof(x);  }), "({ char x[(1<=1)+1]; sizeof(x);  })");
    assert(2, ({ char x[1?2:3]; sizeof(x);  }), "({ char x[1?2:3]; sizeof(x);  })");
    assert(3, ({ char x[0?2:3]; sizeof(x);  }), "({ char x[0?2:3]; sizeof(x);  })");
    assert(3, ({ char x[(1,3)]; sizeof(x);  }), "({ char x[(1,3)]; sizeof(x);  })");
    assert(2, ({ char x[!0+1]; sizeof(x);  }), "({ char x[!0+1]; sizeof(x);  })");
    assert(1, ({ char x[!1+1]; sizeof(x);  }), "({ char x[!1+1]; sizeof(x);  })");
    assert(2, ({ char x[~-3]; sizeof(x);  }), "({ char x[~-3]; sizeof(x);  })");
    assert(2, ({ char x[(5||6)+1]; sizeof(x);  }), "({ char x[(5||6)+1]; sizeof(x);  })");
    assert(1, ({ char x[(0||0)+1]; sizeof(x);  }), "({ char x[(0||0)+1]; sizeof(x);  })");
    assert(2, ({ char x[(1&&1)+1]; sizeof(x);  }), "({ char x[(1&&1)+1]; sizeof(x);  })");
    assert(1, ({ char x[(1&&0)+1]; sizeof(x);  }), "({ char x[(1&&0)+1]; sizeof(x);  })");
    assert(3, ({ char x[(int)3]; sizeof(x);  }), "({ char x[(int)3]; sizeof(x);  })");
    assert(15, ({ char x[(char)0xffffff0f]; sizeof(x);  }), "({ char x[(char)0xffffff0f]; sizeof(x);  })");
    assert(0x10f, ({ char x[(short)0xffff010f]; sizeof(x);  }), "({ char x[(short)0xffff010f]; sizeof(x);  })");
    assert(4, ({ char x[(int)0xfffffffffff+5]; sizeof(x);  }), "({ char x[(int)0xfffffffffff+5]; sizeof(x);  })");
    assert(8, ({ char x[(int*)0+2]; sizeof(x);  }), "({ char x[(int*)0+2]; sizeof(x);  })");
    assert(12, ({ char x[(int*)16-1]; sizeof(x);  }), "({ char x[(int*)16-1]; sizeof(x);  })");
    assert(3, ({ char x[(int*)16-(int*)4]; sizeof(x);  }), "({ char x[(int*)16-(int*)4]; sizeof(x);  })");

    ret_none();
    ;

    assert(0, ({int a[3] = {0,1,2}; a[0];}), "({int a[3] = {0,1,2}; a[0];})");
    assert(1, ({int a[3] = {0,1,2}; a[1];}), "({int a[3] = {0,1,2}; a[1];})");
    assert(2, ({int a[3] = {0,1,2}; a[2];}), "({int a[3] = {0,1,2}; a[2];})");

    assert(0, ({int b[2][2] = {{0,1}, {2,3}}; b[0][0];}), "({int b[2][2] = {{0,1}, {2,3}}; b[0][0];})");
    assert(1, ({int b[2][2] = {{0,1}, {2,3}}; b[0][1];}), "({int b[2][2] = {{0,1}, {2,3}}; b[0][1];})");
    assert(2, ({int b[2][2] = {{0,1}, {2,3}}; b[1][0];}), "({int b[2][2] = {{0,1}, {2,3}}; b[1][0];})");
    assert(3, ({int b[2][2] = {{0,1}, {2,3}}; b[1][1];}), "({int b[2][2] = {{0,1}, {2,3}}; b[1][1];})");

    assert(0, ({int a[3] = {}; a[0];}), "({int a[3] = {}; a[0];})");
    assert(0, ({int a[3] = {0, }; a[2];}), "({int a[3] = {0, }; a[2];})");
    assert(0, ({int b[2][3] = {{0,1}, {2,3}}; b[0][0];}), "({int b[2][3] = {{0,1}, {2,3}}; b[0][0];})");
    assert(1, ({int b[2][3] = {{0,1}, {2,3}}; b[0][1];}), "({int b[2][3] = {{0,1}, {2,3}}; b[0][1];})");
    assert(0, ({int b[2][3] = {{0,1}, {2,3}}; b[0][2];}), "({int b[2][3] = {{0,1}, {2,3}}; b[0][2];})");
    assert(2, ({int b[2][3] = {{0,1}, {2,3}}; b[1][0];}), "({int b[2][3] = {{0,1}, {2,3}}; b[1][0];})");
    assert(3, ({int b[2][3] = {{0,1}, {2,3}}; b[1][1];}), "({int b[2][3] = {{0,1}, {2,3}}; b[1][1];})");
    assert(0, ({int b[2][3] = {{0,1}, {2,3}}; b[1][2];}), "({int b[2][3] = {{0,1}, {2,3}}; b[1][2];})");
    assert(0, ({int b[2][3] = {}; b[1][2];}), "({int b[2][3] = {}; b[1][2];})");

    assert(2, ({int a[3] = {0,1,2,3}; a[2];}), "({int a[3] = {0,1,2,3}; a[2];})");
    assert(0, ({int b[2][2] = {{0,1,4}, {2,3,7}}; b[0][0];}), "({int b[2][2] = {{0,1,4}, {2,3,7}}; b[0][0];})");
    assert(3, ({int b[2][2] = {{0,1,4}, {2,3,7}}; b[1][1];}), "({int b[2][2] = {{0,1,4}, {2,3,7}}; b[1][1];})");

    assert(97, ({char a[4] = "abc"; a[0];}), "({char a[4] = \"abc\"; a[0];})");
    assert(98, ({char a[4] = "abc"; a[1];}), "({char a[4] = \"abc\"; a[1];})");
    assert(99, ({char a[4] = "abc"; a[2];}), "({char a[4] = \"abc\"; a[2];})");
    assert(0, ({char a[4] = "abc"; a[3];}), "({char a[4] = \"abc\"; a[3];})");

    assert(97, ({char a[4][4] = {"abc", "def"}; a[0][0];}), "({char a[4][4] = {\"abc\", \"def\"}; a[0][0];})");
    assert(98, ({char a[4][4] = {"abc", "def"}; a[0][1];}), "({char a[4][4] = {\"abc\", \"def\"}; a[0][1];})");
    assert(102, ({char a[4][4] = {"abc", "def"}; a[1][2];}), "({char a[4][4] = {\"abc\", \"def\"}; a[1][2];})");
    assert(0, ({char a[4][4] = {"abc", "def"}; a[0][3];}), "({char a[4][4] = {\"abc\", \"def\"}; a[0][3];})");

    assert(97, ({struct {char a; int b,c;} x = {'a', 42}; x.a;}), "({struct {char a; int b,c;} x = {'a', 42}; x.a;})");
    assert(42, ({struct {char a; int b,c;} x = {'a', 42}; x.b;}), "({struct {char a; int b,c;} x = {'a', 42}; x.b;})");
    assert(0, ({struct {char a; int b,c;} x = {'a', 42}; x.c;}), "({struct {char a; int b,c;} x = {'a', 42}; x.c;})");

    assert(1, ({ struct {int a; int b; int c;} x={1,2,3}; x.a;  }), "({ struct {int a; int b; int c;} x={1,2,3}; x.a;  })");
    assert(2, ({ struct {int a; int b; int c;} x={1,2,3}; x.b;  }), "({ struct {int a; int b; int c;} x={1,2,3}; x.b;  })");
    assert(3, ({ struct {int a; int b; int c;} x={1,2,3}; x.c;  }), "({ struct {int a; int b; int c;} x={1,2,3}; x.c;  })");
    assert(1, ({ struct {int a; int b; int c;} x={1}; x.a;  }), "({ struct {int a; int b; int c;} x={1}; x.a;  })");
    assert(0, ({ struct {int a; int b; int c;} x={1}; x.b;  }), "({ struct {int a; int b; int c;} x={1}; x.b;  })");
    assert(0, ({ struct {int a; int b; int c;} x={1}; x.c;  }), "({ struct {int a; int b; int c;} x={1}; x.c;  })");

    assert(1, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].a;  }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].a;  })");
    assert(2, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].b;  }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].b;  })");
    assert(3, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].a;  }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].a;  })");
    assert(4, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].b;  }), "({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].b;  })");

    assert(0, ({ struct {int a; int b;} x[2]={{1,2}}; x[1].b;  }), "({ struct {int a; int b;} x[2]={{1,2}}; x[1].b;  })");

    assert(0, ({ struct {int a; int b;} x={}; x.a;  }), "({ struct {int a; int b;} x={}; x.a;  })");
    assert(0, ({ struct {int a; int b;} x={}; x.b;  }), "({ struct {int a; int b;} x={}; x.b;  })");

    assert(1, ({ typedef struct {int a,b;} T; T x={1,2}; T y=x; y.a;  }), "({ typedef struct {int a,b;} T; T x={1,2}; T y=x; y.a;  })");
    assert(5, ({ typedef struct {int a,b,c,d,e,f;} T; T x={1,2,3,4,5,6}; T y; y=x; y.e;  }), "({ typedef struct {int a,b,c,d,e,f;} T; T x={1,2,3,4,5,6}; T y; y=x; y.e;  })");
    assert(2, ({ typedef struct {int a,b;} T; T x={1,2}; T y, z; z=y=x; z.b;  }), "({ typedef struct {int a,b;} T; T x={1,2}; T y, z; z=y=x; z.b;  })");


    assert(4, ({int x[] = {1,2,3,4}; x[3];}), "({int x[] = {1,2,3,4}; x[3];})");
    assert(16, ({int x[] = {1,2,3,4}; sizeof(x);}), "({int x[] = {1,2,3,4}; sizeof(x);})");
    assert(4, ({char x[] = "foo"; sizeof(x);}), "({char x[] = \"foo\"; sizeof(x);})");

    assert(3, g3, "g3");
    assert(4, g4, "g4");
    assert(5, g5[4], "g5[4]");
    assert(97, g6.a, "g6.a");
    assert(98, g6.b, "g6.b");
    assert(7, g7, "g7");
    assert(8, g8, "g8");
    assert(8, g8, "g8");

    assert(9, g9[0].a[0], "g9[0].a[0]");
    assert(10, g9[0].a[1], "g9[0].a[1]");
    assert(0, g9[1].a[0], "g9[1].a[0]");
    assert(0, g9[1].a[1], "g9[1].a[1]");

    assert(7, sizeof(g10), "sizeof(g10)");
    assert(10, sizeof(g11), "sizeof(g11)");
    assert(3, sizeof(g12), "sizeof(g12)");

    assert(0, memcmp(g10, "foobar", 7), "memcmp(g10, \"foobar\", 7)");
    assert(0, memcmp(g11, "foobar\0\0\0", 10), "memcmp(g11, \"foobar\\0\\0\\0\", 10)");
    assert(0, memcmp(g12, "foo", 3), "memcmp(g12, \"foo\", 3)");

    assert(0, strcmp(g13, "foobar"), "strcmp(g13, \"foobar\")");
    assert(0, strcmp(g14, "bar"), "strcmp(g14, \"bar\")");
    assert(0, strcmp(g15+3, "foobar"), "strcmp(g15+3, \"foobar\")");

    assert(0, strcmp(g16[0], "foobar"), "strcmp(g16[0], \"foobar\")");
    assert(0, strcmp(g16[1], "bar"), "strcmp(g16[1], \"bar\")");
    assert(0, strcmp(g16[2]+3, "foobar"), "strcmp(g16[2]+3, \"foobar\")");

    assert(3, g17, "g17");
    assert(3, *g18, "*g18");
    assert(2, *g20, "*g20");

    assert(1, g30[0].a[0], "g30[0].a[0]");
    assert(2, g30[0].a[1], "g30[0].a[1]");
    assert(3, g30[1].a[0], "g30[1].a[0]");
    assert(4, g30[1].a[1], "g30[1].a[1]");

    assert(1, g31[0].a[0], "g31[0].a[0]");
    assert(2, g31[0].a[1], "g31[0].a[1]");
    assert(3, g31[1].a[0], "g31[1].a[0]");
    assert(4, g31[1].a[1], "g31[1].a[1]");

    assert(0, ({ int x[2][3]={0,1,2,3,4,5,}; x[0][0];  }), "({ int x[2][3]={0,1,2,3,4,5,}; x[0][0];  })");
    assert(3, ({ int x[2][3]={0,1,2,3,4,5,}; x[1][0];  }), "({ int x[2][3]={0,1,2,3,4,5,}; x[1][0];  })");

    assert(0, ({ struct {int a; int b;} x[2]={0,1,2,3}; x[0].a;  }), "({ struct {int a; int b;} x[2]={0,1,2,3}; x[0].a;  })");
    assert(2, ({ struct {int a; int b;} x[2]={0,1,2,3}; x[1].a;  }), "({ struct {int a; int b;} x[2]={0,1,2,3}; x[1].a;  })");

    assert(0, strcmp(g32[0], "foo"), "strcmp(g32[0], \"foo\")");
    assert(0, strcmp(g32[1], "bar"), "strcmp(g32[1], \"bar\")");
    assert(0, strcmp(g34, "foo"), "strcmp(g34, \"foo\")");
    ext1 = 5;
    ext2 = &ext1;
    assert(5, ext1, "ext1");
    assert(5, *ext2, "*ext2");
    assert(1, _Alignof(char), "_Alignof(char)");
    assert(2, _Alignof(short), "_Alignof(short)");
    assert(4, _Alignof(int), "_Alignof(int)");
    assert(8, _Alignof(long), "_Alignof(long)");
    assert(8, _Alignof(long long), "_Alignof(long long)");
    assert(1, _Alignof(char[3]), "_Alignof(char[3])");
    assert(4, _Alignof(int[3]), "_Alignof(int[3])");
    assert(1, _Alignof(struct {char a; char b;}[2]), "_Alignof(struct {char a; char b;}[2])");
    assert(8, _Alignof(struct {char a; long b;}[2]), "_Alignof(struct {char a; long b;}[2])");

    assert(0, (long)(char *)&g_aligned1 % 512, "(long)(char *)&g_aligned1 % 512");
    assert(0, (long)(char *)&g_aligned2 % 512, "(long)(char *)&g_aligned2 % 512");

    assert(1, ({ _Alignas(char) char x, y; &y-&x;  }), "({ _Alignas(char) char x, y; &y-&x;  })");
    assert(8, ({ _Alignas(long) char x, y; &y-&x;  }), "({ _Alignas(long) char x, y; &y-&x;  })");
    assert(32, ({ _Alignas(32) char x, y; &y-&x;  }), "({ _Alignas(32) char x, y; &y-&x;  })");
    assert(32, ({ _Alignas(32) int *x, *y; ((char *)&y)-((char *)&x);  }), "({ _Alignas(32) int *x, *y; ((char *)&y)-((char *)&x);  })");
    assert(16, ({ struct { _Alignas(16) char x, y;  } a; &a.y-&a.x;  }), "({ struct { _Alignas(16) char x, y;  } a; &a.y-&a.x;  })");
    assert(8, ({ struct T { _Alignas(8) char a;  }; _Alignof(struct T);  }), "({ struct T { _Alignas(8) char a;  }; _Alignof(struct T);  })");

    assert(1, tree->val, "tree->val");
    assert(2, tree->lhs->val, "tree->lhs->val");
    assert(3, tree->lhs->lhs->val, "tree->lhs->lhs->val");
    assert(4, tree->lhs->rhs->val, "tree->lhs->rhs->val");

    (int){3} = 5;
    assert(1, (int){1}, "(int){1}");
    assert(2, ((int[]){0,1,2})[2], "((int[]){0,1,2})[2]");
    assert('a', ((struct {char a; int b;}){'a', 3}).a, "((struct {char a; int b;}){'a', 3}).a");
    assert(3, ({ int x=3; (int){x};  }), "({ int x=3; (int){x};  })");

    assert(1, counter(), "counter()");
    assert(2, counter(), "counter()");
    assert(3, counter(), "counter()");

    assert(3, ext3, "ext3");

    assert(1, true_fn(), "true_fn()");
    assert(0, false_fn(), "false_fn()");

    assert(10, add_all1(1,2,3,4,0), "add_all1(1,2,3,4,0)");
    assert(9, add_all1(1,2,3,4,-1,0), "add_all1(1,2,3,4,-1,0)");
    assert(10, add_all3(1,2,3,4,0), "add_all3(1,2,3,4,0)");
    assert(9, add_all3(1,2,3,4,-1,0), "add_all3(1,2,3,4,-1,0)");

    assert(0, ({char connected[] = "foo" "bar"; strcmp(connected, "foobar");}), "({char connected[] = \"foo\" \"bar\"; strcmp(connected, \"foobar\");})");

    assert(0, ({ char buf[100]; sprintf(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf);  }), "({ char buf[100]; sprintf(buf, \"%d %d %s\", 1, 2, \"foo\"); strcmp(\"1 2 foo\", buf);  })");

    assert(0, ({ char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf);  }), "({ char buf[100]; fmt(buf, \"%d %d %s\", 1, 2, \"foo\"); strcmp(\"1 2 foo\", buf);  })");

    assert(-1, ({signed char x = -1; x;}), "({signed char x = -1; x;})");
    assert(255, ({unsigned char x = -1; (int)x;}), "({unsigned char x = -1; (int)x;})");
    assert(1, sizeof(char), "sizeof(char)");
    assert(1, sizeof(signed char), "sizeof(signed char)");
    assert(1, sizeof(signed char signed), "sizeof(signed char signed)");
    assert(1, sizeof(unsigned char), "sizeof(unsigned char)");
    assert(1, sizeof(unsigned char unsigned), "sizeof(unsigned char unsigned)");

    assert(2, sizeof(short), "sizeof(short)");
    assert(2, sizeof(int short), "sizeof(int short)");
    assert(2, sizeof(short int), "sizeof(short int)");
    assert(2, sizeof(signed short), "sizeof(signed short)");
    assert(2, sizeof(int short signed), "sizeof(int short signed)");
    assert(2, sizeof(unsigned short), "sizeof(unsigned short)");
    assert(2, sizeof(int short unsigned), "sizeof(int short unsigned)");

    assert(4, sizeof(int), "sizeof(int)");
    assert(4, sizeof(signed int), "sizeof(signed int)");
    assert(4, sizeof(signed), "sizeof(signed)");
    assert(4, sizeof(signed signed), "sizeof(signed signed)");
    assert(4, sizeof(unsigned int), "sizeof(unsigned int)");
    assert(4, sizeof(unsigned), "sizeof(unsigned)");
    assert(4, sizeof(unsigned unsigned), "sizeof(unsigned unsigned)");

    assert(8, sizeof(long), "sizeof(long)");
    assert(8, sizeof(signed long), "sizeof(signed long)");
    assert(8, sizeof(signed long int), "sizeof(signed long int)");
    assert(8, sizeof(unsigned long), "sizeof(unsigned long)");
    assert(8, sizeof(unsigned long int), "sizeof(unsigned long int)");

    assert(8, sizeof(long long), "sizeof(long long)");
    assert(8, sizeof(signed long long), "sizeof(signed long long)");
    assert(8, sizeof(signed long long int), "sizeof(signed long long int)");
    assert(8, sizeof(unsigned long long), "sizeof(unsigned long long)");
    assert(8, sizeof(unsigned long long int), "sizeof(unsigned long long int)");

    assert(1, sizeof((char)1), "sizeof((char)1)");
    assert(2, sizeof((short)1), "sizeof((short)1)");
    assert(4, sizeof((int)1), "sizeof((int)1)");
    assert(8, sizeof((long)1), "sizeof((long)1)");
    assert(-1, (char)255, "(char)255");
    assert(-1, (signed char)255, "(signed char)255");
    assert(255, (unsigned char)255, "(unsigned char)255");
    assert(-1, (short)65535, "(short)65535");
    assert(65535, (unsigned short)65535, "(unsigned short)65535");
    assert(-1, (int)0xffffffff, "(int)0xffffffff");
    assert(0xffffffff, (unsigned)0xffffffff, "(unsigned)0xffffffff");

    assert(8, sizeof(sizeof(char)), "sizeof(sizeof(char))");
    assert(8, sizeof(_Alignof(char)), "sizeof(_Alignof(char))");
    assert(4, sizeof(0), "sizeof(0)");
    assert(8, sizeof(0L), "sizeof(0L)");
    assert(8, sizeof(0LU), "sizeof(0LU)");
    assert(8, sizeof(0UL), "sizeof(0UL)");
    assert(8, sizeof(0LL), "sizeof(0LL)");
    assert(8, sizeof(0LLU), "sizeof(0LLU)");
    assert(8, sizeof(0Ull), "sizeof(0Ull)");
    assert(8, sizeof(0l), "sizeof(0l)");
    assert(8, sizeof(0ll), "sizeof(0ll)");
    assert(8, sizeof(0x0L), "sizeof(0x0L)");
    assert(8, sizeof(0b0L), "sizeof(0b0L)");
    assert(4, sizeof(2147483647), "sizeof(2147483647)");
    assert(8, sizeof(2147483648), "sizeof(2147483648)");
    assert(-1, 0xffffffffffffffff, "0xffffffffffffffff");
    assert(8, sizeof(0xffffffffffffffff), "sizeof(0xffffffffffffffff)");
    assert(4, sizeof(4294967295U), "sizeof(4294967295U)");
    assert(8, sizeof(4294967296U), "sizeof(4294967296U)");

    assert(3, -1U>>30, "-1U>>30");
    assert(3, -1Ul>>62, "-1Ul>>62");
    assert(3, -1ull>>62, "-1ull>>62");

    assert(1, 0xffffffffffffffffl>>63, "0xffffffffffffffffl>>63");
    assert(1, 0xffffffffffffffffll>>63, "0xffffffffffffffffll>>63");

    assert(-1, 18446744073709551615, "18446744073709551615");
    assert(8, sizeof(18446744073709551615), "sizeof(18446744073709551615)");
    assert(-1, 18446744073709551615>>63, "18446744073709551615>>63");

    assert(-1, 0xffffffffffffffff, "0xffffffffffffffff");
    assert(8, sizeof(0xffffffffffffffff), "sizeof(0xffffffffffffffff)");
    assert(1, 0xffffffffffffffff>>63, "0xffffffffffffffff>>63");

    assert(-1, 01777777777777777777777, "01777777777777777777777");
    assert(8, sizeof(01777777777777777777777), "sizeof(01777777777777777777777)");
    assert(1, 01777777777777777777777>>63, "01777777777777777777777>>63");

    assert(-1, 0b1111111111111111111111111111111111111111111111111111111111111111, "0b1111111111111111111111111111111111111111111111111111111111111111");
    assert(8, sizeof(0b1111111111111111111111111111111111111111111111111111111111111111), "sizeof(0b1111111111111111111111111111111111111111111111111111111111111111)");
    assert(1, 0b1111111111111111111111111111111111111111111111111111111111111111>>63, "0b1111111111111111111111111111111111111111111111111111111111111111>>63");

    assert(8, sizeof(2147483648), "sizeof(2147483648)");
    assert(4, sizeof(2147483647), "sizeof(2147483647)");

    assert(8, sizeof(0x1ffffffff), "sizeof(0x1ffffffff)");
    assert(4, sizeof(0xffffffff), "sizeof(0xffffffff)");
    assert(1, 0xffffffff>>31, "0xffffffff>>31");

    assert(8, sizeof(040000000000), "sizeof(040000000000)");
    assert(4, sizeof(037777777777), "sizeof(037777777777)");
    assert(1, 037777777777>>31, "037777777777>>31");

    assert(8, sizeof(0b111111111111111111111111111111111), "sizeof(0b111111111111111111111111111111111)");
    assert(4, sizeof(0b11111111111111111111111111111111), "sizeof(0b11111111111111111111111111111111)");
    assert(1, 0b11111111111111111111111111111111>>31, "0b11111111111111111111111111111111>>31");

    assert(-1, 1 << 31 >> 31, "1 << 31 >> 31");
    assert(-1, 01 << 31 >> 31, "01 << 31 >> 31");
    assert(-1, 0x1 << 31 >> 31, "0x1 << 31 >> 31");
    assert(-1, 0b1 << 31 >> 31, "0b1 << 31 >> 31");

    assert(4, ({char x[(-1>>31)+5]; sizeof(x);}), "({char x[(-1>>31)+5]; sizeof(x);})");
    assert(255, ({char x[(unsigned char)0xffffffff]; sizeof(x);}), "({char x[(unsigned char)0xffffffff]; sizeof(x);})");
    assert(0xffff, ({char x[(unsigned short)0xffffffff]; sizeof(x);}), "({char x[(unsigned short)0xffffffff]; sizeof(x);})");
    assert(1, ({char x[(unsigned int)0xffffffffffffff>>31]; sizeof(x);}), "({char x[(unsigned int)0xffffffffffffff>>31]; sizeof(x);})");
    assert(1, ({char x[(unsigned)1< -1]; sizeof(x);}), "({char x[(unsigned)1< -1]; sizeof(x);})");
    assert(1, ({char x[(unsigned)1<=-1]; sizeof(x);}), "({char x[(unsigned)1<=-1]; sizeof(x);})");
    {const x;}
    {volatile x;}
    {int const x;}
    {const int x;}
    {const int volatile const const x;}

    assert(5, ({const int x = 5; x;}), "({const int x = 5; x;})");
    assert(5, ({const int x = 5; int *const y=&x; *y;}), "({const int x = 5; int *const y=&x; *y;})");
    assert(5, ({const int x = 5; *(const volatile * const)&x;}), "({const int x = 5; *(const volatile * const)&x;})");

    assert(3, あ, "あ");
    assert(42, 🍣, "🍣");
    assert(3, ({int β=3; β;}), "({int β=3; β;})");

    assert(10, ({int i=0; do i++; while(i<10); i;}), "({int i=0; do i++; while(i<10); i;})");
    assert(1, ({int i=0; do i++; while(i<0); i;}), "({int i=0; do i++; while(i<0); i;})");
    assert(4, ({ int i=0; int j=0; int k=0; do { if (++j > 3) break; continue; k++;  } while (1); j;  }), "({ int i=0; int j=0; int k=0; do { if (++j > 3) break; continue; k++;  } while (1); j;  })");

    assert(1, (int)1.0, "(int)1.0");
    assert(1.0, (float)1, "(float)1");

    assert(1, 2e3==2e3, "2e3==2e3");
    assert(1, 2e3==2000, "2e3==2000");
    assert(0, 2e3!=2e3, "2e3!=2e3");
    assert(0, 2e3!=2000, "2e3!=2000");
    assert(0, 2e5==2e3, "2e5==2e3");
    assert(0, 2e3==200, "2e3==200");
    assert(1, 2.0<2.1, "2.0<2.1");
    assert(1, 2.0<=2.1, "2.0<=2.1");
    assert(0, 2.0>2.1, "2.0>2.1");
    assert(0, 2.0>=2.1, "2.0>=2.1");
    assert(1, 2.0f<2.1, "2.0f<2.1");
    assert(1, 2<=2.1L, "2<=2.1L");
    assert(0, 2.0l>2.1f, "2.0l>2.1f");
    assert(0, 2ul>2.1f, "2ul>2.1f");

    assert(1, ({float x=1.0; x==1.0;}), "({float x=1.0; x==1.0;})");
    assert(1, ({ float x=2e3f; x==2e3; }), "({ float x=2e3f; x==2e3; })");
    assert(0, ({ float x=2e3f; x==2e5; }), "({ float x=2e3f; x==2e5; })");
    assert(0, ({ float x=5.1f; x<5; }), "({ float x=5.1f; x<5; })");
    assert(0, ({ float x=5.0f; x<5; }), "({ float x=5.0f; x<5; })");
    assert(1, ({ float x=4.9f; x<5; }), "({ float x=4.9f; x<5; })");
    assert(0, ({ float x=5.1f; x<=5; }), "({ float x=5.1f; x<=5; })");
    assert(1, ({ float x=5.0f; x<=5; }), "({ float x=5.0f; x<=5; })");
    assert(1, ({ float x=4.9f; x<=5; }), "({ float x=4.9f; x<=5; })");

    assert(1, ({ double x=2e3f; x==2e3; }), "({ double x=2e3f; x==2e3; })");
    assert(0, ({ double x=2e3f; x==2e5; }), "({ double x=2e3f; x==2e5; })");
    assert(0, ({ double x=5.1f; x<5; }), "({ double x=5.1f; x<5; })");
    assert(0, ({ double x=5.0f; x<5; }), "({ double x=5.0f; x<5; })");
    assert(1, ({ double x=4.9f; x<5; }), "({ double x=4.9f; x<5; })");
    assert(0, ({ double x=5.1f; x<=5; }), "({ double x=5.1f; x<=5; })");
    assert(1, ({ double x=5.0f; x<=5; }), "({ double x=5.0f; x<=5; })");
    assert(1, ({ double x=4.9f; x<=5; }), "({ double x=4.9f; x<=5; })");

    assert(0, (_Bool)0.0, "(_Bool)0.0");
    assert(1, (_Bool)0.1, "(_Bool)0.1");
    assert(3, (char)3.0, "(char)3.0");
    assert(1000, (short)1000.3, "(short)1000.3");
    assert(3, (int)3.99, "(int)3.99");
    assert(2000000000000000, (long)2e15, "(long)2e15");
    assert(3, (float)3.5, "(float)3.5");
    assert(5, (double)(float)5.5, "(double)(float)5.5");
    assert(3, (float)3, "(float)3");
    assert(3, (double)3, "(double)3");
    assert(3, (float)3L, "(float)3L");
    assert(3, (double)3L, "(double)3L");

    assert(4, sizeof(float), "sizeof(float)");
    assert(8, sizeof(double), "sizeof(double)");
    assert(8, sizeof(long double), "sizeof(long double)");

    assert(6, 2.3+3.8, "2.3+3.8");
    assert(-1, 2.3-3.8, "2.3-3.8");
    assert(8, 2.3*3.8, "2.3*3.8");
    assert(2, 5.0/2, "5.0/2");

    assert(6, 2.3f+3.8f, "2.3f+3.8f");
    assert(-1, 2.3f-3.8f, "2.3f-3.8f");
    assert(8, 2.3f*3.8f, "2.3f*3.8f");
    assert(2, 5.0f/2, "5.0f/2");
    assert(2, 5.f/2, "5.f/2");
    assert(2, .5f*4, ".5f*4");

    assert(4, sizeof(1.f+2), "sizeof(1.f+2)");
    assert(4, sizeof(1.f-2), "sizeof(1.f-2)");
    assert(4, sizeof(1.f*2), "sizeof(1.f*2)");
    assert(4, sizeof(1.f/2), "sizeof(1.f/2)");
    assert(8, sizeof(1.0+2), "sizeof(1.0+2)");
    assert(8, sizeof(1.0-2), "sizeof(1.0-2)");
    assert(8, sizeof(1.0*2), "sizeof(1.0*2)");
    assert(8, sizeof(1.0/2), "sizeof(1.0/2)");

    assert(0, !3., "!3.");
    assert(1, !0., "!0.");
    assert(0, 0.0 && 0.0, "0.0 && 0.0");
    assert(0, 0.0 && 0.1, "0.0 && 0.1");
    assert(0, 0.3 && 0.0, "0.3 && 0.0");
    assert(1, 0.3 && 0.5, "0.3 && 0.5");
    assert(0, 0.0 || 0.0, "0.0 || 0.0");
    assert(1, 0.0 || 0.1, "0.0 || 0.1");
    assert(1, 0.3 || 0.0, "0.3 || 0.0");
    assert(1, 0.3 || 0.5, "0.3 || 0.5");
    assert(5, 0.0 ? 3 : 5, "0.0 ? 3 : 5");
    assert(3, 1.2 ? 3 : 5, "1.2 ? 3 : 5");
    assert(5, ({ int x; if (0.0) x=3; else x=5; x; }), "({ int x; if (0.0) x=3; else x=5; x; })");
    assert(3, ({ int x; if (0.1) x=3; else x=5; x; }), "({ int x; if (0.1) x=3; else x=5; x; })");
    assert(5, ({ int x=5; if (0.0) x=3; x; }), "({ int x=5; if (0.0) x=3; x; })");
    assert(3, ({ int x=5; if (0.1) x=3; x; }), "({ int x=5; if (0.1) x=3; x; })");
    assert(10, ({ double i=10.0; int j=0; for (; i; i--, j++); j; }), "({ double i=10.0; int j=0; for (; i; i--, j++); j; })");
    assert(10, ({ double i=10.0; int j=0; do j++; while(--i); j; }), "({ double i=10.0; int j=0; do j++; while(--i); j; })");

    assert(0, add_float(3.8f, -3.8f), "add_float(3.8f, -3.8f)");
    assert(0, add_double(3.8, -3.8), "add_double(3.8, -3.8)");

    assert(4, add_float3(1.4f, 1.3f, 1.4f), "add_float3(1.4f, 1.3f, 1.4f)");
    assert(2, sub_double(1.4l, -0.7f), "sub_double(1.4l, -0.7f)");

    assert(1, g35==1.5, "g35==1.5");
    assert(1, g36==11, "g36==11");

    assert(3, fnptr()(0), "fnptr()(0)");
    assert(3, (*****ret3)(), "(*****ret3)()");

#
    assert(10, include1, "include1");
    assert(200, include2, "include2");
#if 0
    assert_do_nothing;
#if nested
#endif
#endif

    assert(3,
           1
#if 1 + 1
               + 2,
#endif
           "1+2");

    assert(6,
           1
#if 1
               + 2
#if 10
               + 3
#if 0
            hahaha
#endif
#endif
           ,
           "1+2+3"
#endif
    );

    assert(3,
           1
#if 1
#if 0
#if 1
                  foo bar
#endif
#endif
               + 2
#endif
           ,
           "1+2");

    assert(3,
#if 1 - 1
#if 1
#endif
#if 1
#else
#endif
#if 0
#else
#endif
           2,
#else
#if 0
#elif 1
           3,
#endif
#endif
           "3");

    assert(2,
#if 1
           2,
#else
           3,
#endif
           "2");

    assert(3,
#if 0
                  1,
#elif 0
           2,
#elif 1 + 3
           3,
#elif 0
           4,
#else
           5,
#endif
           "3");

#define M1 3
    assert(3, M1, "M1");
#define M1 4
    assert(4, M1, "M1");
#define M1 3 + 4 +
    assert(12, M1 5, "M1 5");
#define ASSERT assert(
#define END )
#define if 5
#define five "5"
    ASSERT 5, if, five END;
#undef five
#undef if
#undef END
#undef ASSERT
    assert(3, ({int x; if (1) x=3; x; }), "({int x; if (1) x=3; x; })");

#define M2 5
#define M3 0
    assert(5, 
#if M2
            5
#elif M3
            6
#else
            7
#endif
            , "5");
#undef M2
#undef M3
    int M2 = 6;
#define M2 M2 + 3
    assert(9, M2, "M2");
#define M3 M2 + 3
    assert(12, M3, "M3");
    int M4 = 3;
#define M4 M5 * 5
#define M5 M4 + 2
    assert(13, M4, "M4");
#undef M2
#undef M3
#undef M4
#undef M5

#define IFDEF 5
#define IFNDEF 6
#define M6 1
    assert(5,
#ifdef M6
            IFDEF
#else
            IFNDEF
#endif
           ,"IFDEF");
    assert(5,
#ifdef M7
            IFNDEF
#else
            IFDEF
#endif
           ,"IFDEF");
    assert(6,
#ifndef M6
            IFDEF
#else
            IFNDEF
#endif
           ,"IFNDEF");
    assert(6,
#ifndef M7
            IFNDEF
#else
            IFDEF
#endif
           ,"IFNDEF");
    
#undef M7
#undef M6
#define M8() 8
    int M8 = 88;
    assert(8, M8(), "M8()");
    assert(88, M8, "M8");
#define M9 ()
    assert(3, ret3 M9, "ret3 M9");

#define M10(a, b) a*b
    assert(12, M10(3,4), "M10(3,4)");
    assert(24, M10(3+4,4+5), "M10(3+4,4+5)");
#define M11(a, b) (a) * (b)
    assert(63, M11(3+4,4+5), "M11(3+4,4+5)");
#define M12(a,b) a b
    assert(9, M12(, 4+5), "M12(, 4+5)");
    assert(20, M10((2+3), 4), "M10((2+3), 4)");
    assert(12, M10((2,3), 4), "M10((2,3), 4)");
#define M13(x) M14(x) * x
#define M14(x) M13(x) + 3
    assert(10, M13(2), "M13(2)");
    printf("OK\n"); 
#undef M11
#define M11(x) #x
    assert('a', M11( a!b  `""c)[0], "M11( a!b  `\"\"c)[0]");
    assert('!', M11( a!b  `""c)[1], "M11( a!b  `\"\"c)[1]");
    assert('b', M11( a!b  `""c)[2], "M11( a!b  `\"\"c)[2]");
    assert(' ', M11( a!b  `""c)[3], "M11( a!b  `\"\"c)[3]");
    assert('`', M11( a!b  `""c)[4], "M11( a!b  `\"\"c)[4]");
    assert('"', M11( a!b  `""c)[5], "M11( a!b  `\"\"c)[5]");
    assert('"', M11( a!b  `""c)[6], "M11( a!b  `\"\"c)[6]");
    assert('c', M11( a!b  `""c)[7], "M11( a!b  `\"\"c)[7]");
    assert(0, M11( a!b  `""c)[8], "M11( a!b  `\"\"c)[8]");
#define paste(x,y) x##y
    assert(15, paste(1,5), "paste(1,5)");
    assert(255, paste(0,xff), "paste(0,xff)");
    assert(3, ({ int foobar=3; paste(foo,bar);  }), "({ int foobar=3; paste(foo,bar);  })");
    assert(5, paste(5,), "paste(5,)");
    assert(5, paste(,5), "paste(,5)");

#define paste2(x) x##2
    assert(12, paste2(1), "paste2(1)");

#define paste3(x) 2##x
    assert(21, paste3(1), "paste3(1)");
#define M15
    assert(3,
#if defined(M15)
           3
#else
           4
#endif
          , "3");
    assert(3,
#if defined M15
           3
#else
           4
#endif
          , "3");
    assert(4,
#if defined(M15) - 1
           3
#else
           4
#endif
          , "4");
    assert(4,
#if defined(NO_SUCH_MACRO)
           3
#else
           4
#endif
          , "4");
    assert(4,
#if NO_SUCH_MACRO
           3
#else
           4
#endif
          , "4");

    asse\
rt(
        4,4,"\
4");
    assert(4,
            size\
of(int),"sizeof(int)");
    assert(3, INCLUDE3, "INCLUDE3");
    assert(4, INC4, "INC4");

    assert(0, strcmp("tests/tests.c", main_fn), "strcmp(\"tests/tests.c\", main_fn)");
    assert(2, main_ln, "main_ln");
    assert(0, strcmp("tests/include1.h", include1_fn), "strcmp(\"tests/include1.h\", include1_fn)");
    assert(5, include1_ln, "include1_ln");

    assert(5, sizeof(__func__), "sizeof(__func__)");
    assert(0, strcmp("main", __func__), "strcmp(\"main\", __func__)");
    assert(0, strcmp("func_fn", func_fn()), "strcmp(\"func_fn\", func_fn())");
#undef M14
#define M14(...) 3
    assert(3, M14(), "M14()");

#define M14(...) __VA_ARGS__
    assert(5, M14(5), "M14(5)");

#define M14(...) add2(__VA_ARGS__)
    assert(8, M14(2, 6), "M14(2, 6)");

#define M14(...) add6(1,2,__VA_ARGS__,6)
    assert(21, M14(3,4,5), "M14(3,4,5)");

#define M14(x, ...) add6(1,2,x,__VA_ARGS__,6)
    assert(21, M14(3,4,5), "M14(3,4,5)");
    printf("OK\n");
    return 0;
}
