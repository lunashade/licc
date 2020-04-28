// line comment

/*
 *  This is a block comment
 */

int g1, g2[4];

int assert(int want, int got, char *code) {
    if (want == got) {
        printf("%s => %d\n", code, got);
    } else {
        printf("%s => want %d, got %d\n", code, want, got);
        exit(1);
    }
    return 0;
}

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

int main() {
    assert(0, 0, "0;");
    assert(3, ({ int x = 3; x; }), "{ ({int x=3; x;});}");
    assert(2, ({ int x = 2; { int x = 3; } x; }), "{int x=2; {int x=3;}  x;}");
    assert(2, ({ int x = 2; { int x = 3; } int y = 4; x; }), "{int x=2; {int x=3;} int y=4;  x;}");
    assert(3, ({ int x = 2; { x = 3; } x; }), "{int x=2; {x=3;}  x;}");
    assert(42, 42, "42");
    assert(42, (50 - 10 + 2), "{  50-10+2; }");
    assert(42, ((2 + 4) * 7), "{  (2+4)*7; }");
    assert(5, (300 / 60), "{  300 / 60; }");
    assert(15, (-+-5 * - -3), "{  -+ -5 * - -3; }");
    assert(1, (3 + 2 == 5), "{  3+2 == 5; }");
    assert(0, (3 + 2 != 5 * - -1), "{  3+2 != 5*--1; }");
    assert(1, (1 < 2), "{  1 < 2; }");
    assert(0, (1 >= 2), "{  1 >= 2; }");
    assert(1, (1 <= 2), "{  1 <= 2; }");
    assert(0, (1 > 2), "{  1 > 2; }");
    assert(3, ({ 1; 2; 3; }), "{ 1;2;  3; }");
    assert(1, ({ int a = 1; a; }), "{ int a=1; a; }");
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
    assert(3, ({
               int x = 3;
               int *y = 5;
               *(&y - 1);
           }),
           "{ int x=3; int *y=5;  *(&y-1);  }");
    assert(5, ({
               int x = 3;
               int *y = &x;
               *y = 5;
               x;
           }),
           "{ int x=3; int *y=&x; *y=5;  x;  }");
    assert(7, ({
               int x = 3;
               int *y = 5;
               *(&x + 1) = 7;
               y;
           }),
           "{ int x=3; int *y=5; *(&x+1)=7;  y;  }");
    assert(7, ({
               int x = 3;
               int *y = 5;
               *(&y - 1) = 7;
               x;
           }),
           "{ int x=3; int *y=5; *(&y-1)=7;  x;  }");
    assert(8, ({ sizeof(12); }), "{  sizeof( 12 ); }");
    assert(8, ({
               int x = 29;
               int *y = &x;
               sizeof(&y);
           }),
           "{ int x=29; int *y=&x;  sizeof(&y); }");
    assert(3, (ret3()), "{  ret3();  }");
    assert(5, (ret5()), "{  ret5();  }");
    assert(8, (add(3, 5)), "{  add(3,5); }");
    assert(21, (add6(1, 2, 3, 4, 5, 6)), "{  add6(1,2,3,4,5,6); }");
    assert(55, (fibo(9)), " { fibo(9);} ");
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
    assert(5, ({ int x[2][3]; **x = 3; *(*(x + 2) + 1) = 5; *(*(x + 2) + 1); }), " {int x[2][3]; **x=3; *(*(x+2)+1) = 5;  *(*(x+2)+1);}");
    assert(3, ({ int x[2]; x[0] = 3; x[1] = 4; x[2] = 5; x[0]; }),
           " {int x[2]; x[0] = 3; x[1]=4; x[2]=5;  x[0];}");
    assert(4, ({ int x[2]; x[0] = 3; x[1] = 4; x[2] = 5; x[1]; }),
           " {int x[2]; x[0] = 3; x[1]=4; x[2]=5;  x[1];}");
    assert(5, ({ int x[2]; x[0] = 3; x[1] = 4; x[2] = 5; x[2]; }),
           " {int x[2]; x[0] = 3; x[1]=4; x[2]=5;  x[2];}");
    assert(5, ({ int x[2]; x[0] = 3; x[1] = 4; 2 [x] = 5; *(x + 2); }),
           " {int x[2]; x[0] = 3; x[1]=4; 2[x]=5;  *(x+2);}");
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
    assert(6, ({ int x[2][3]; int *y = x; y[6] = 6; x[2][0]; }),
           " { int x[2][3]; int *y=x; y[6]=6;  x[2][0];  }");
    assert(1, ({ char x; sizeof(x); }), " {char x;  sizeof(x);}");
    assert(1, ({ char x = 1; x; }), " { char x=1;  x;  }");
    assert(1, ({ char x = 1; char y = 2; x; }), " { char x=1; char y=2;  x;  }");
    assert(2, ({ char x = 1; char y = 2; y; }), " { char x=1; char y=2;  y;  }");
    assert(1, ({ char x; sizeof(x); }), " { char x;  sizeof(x);  }");
    assert(10, ({ char x[10]; sizeof(x); }), "{ char x[10];  sizeof(x); }");
    assert(1, (sub_char(7, 3, 3)), "sub_char(7, 3, 3);");
    assert(97, ("abc"[0]), " { \"abc\"[0];}");
    assert(98, ("abc"[1]), " { \"abc\"[1];}");
    assert(99, ("abc"[2]), " { \"abc\"[2];}");
    assert(0, ("abc"[3]), " { \"abc\"[3];}");
    assert(4, (sizeof("abc")), " { sizeof(\"abc\");}");
    assert(7, ("\a"[0]), " {  \"\\a\"[0];  }");
    assert(8, ("\b"[0]), " {  \"\\b\"[0];  }");
    assert(9, ("\t"[0]), " {  \"\\t\"[0];  }");
    assert(10, ("\n"[0]), " {  \"\\n\"[0];  }");
    assert(11, ("\v"[0]), " {  \"\\v\"[0];  }");
    assert(12, ("\f"[0]), " {  \"\\f\"[0];  }");
    assert(13, ("\r"[0]), " {  \"\\r\"[0];  }");
    assert(27, ("\e"[0]), " {  \"\\e\"[0];  }");
    assert(106, ("\j"[0]), " {  \"\\j\"[0];  }");
    assert(107, ("\k"[0]), " {  \"\\k\"[0];  }");
    assert(108, ("\l"[0]), " {  \"\\l\"[0];  }");
    assert(0, ("\0"[0]), " { \"\\0\"[0];}");
    assert(16, ("\20"[0]), " { \"\\20\"[0];}");
    assert(72, ("\110"[0]), " { \"\\110\"[0];}");
    assert(48, ("\1100"[1]), " { \"\\1100\"[1];}");
    assert(0, ("\x0"[0]), " { \"\\x0\"[0];}");
    assert(16, ("\x10"[0]), " { \"\\x10\"[0];}");
    assert(10, ("\x0a"[0]), " { \"\\x0a\"[0];}");
    assert(106, ("\xaj"[1]), " { \"\xaj\"[1];}");

    printf("OK\n");
    return 0;
}
