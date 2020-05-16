#include "include1.h"

int printf();
int exit();
int testno;
int ret3(void) {
    return 3;
}

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

int main() {
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
    assert(12, M1 5, "M1");
#define ASSERT assert(
#define END )
#define if 5
#define five "5"
    ASSERT 5, if, five END;
#undef five
#undef if
#undef END
#undef ASSERT
    assert(3, ({int x; if (1) x=3; x; }), "if (1); 3;");

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
    assert(88, M8, "M8()");
#define M9 ()
    assert(3, ret3 M9, "ret3 M9");

#define M10(a, b) a*b
    assert(12, M10(3,4), "M10(3,4)");
    assert(24, M10(3+4,4+5), "M10(3,4)");
#define M11(a, b) (a) * (b)
    assert(63, M11(3+4,4+5), "M11(3,4)");
#define M12(a,b) a b
    assert(9, M12(, 4+5), "M12(, 4+5)");
    assert(20, M10((2+3), 4), "M10((2+3), 4)");
    assert(12, M10((2,3), 4), "M10((2,3), 4)");
    printf("OK\n"); 
    return 0;
}
