#include "include1.h"

int printf();
int exit();
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
    assert(3, ({if (1); 3;}), "if (1); 3;");
    printf("OK\n");
    return 0;
}
