#include <stdarg.h>
int static_fn() { return 5; }
int ext1;
int *ext2;
int ext3 = 5;

int true_fn() { return 0x301; }
int false_fn() { return 0x300; }

int add_all1(int x, ...) {
    va_list ap;
    va_start(ap, x);

    for (;;) {
        int y = va_arg(ap, int);
        if (y == 0)
            return x;
        x += y;
    }
}

int add_all3(int x, int y, int z, ...) {
    va_list ap;
    va_start(ap, z);
    x = x + y + z;

    for (;;) {
        int y = va_arg(ap, int);
        if (y == 0)
            return x;
        x += y;
    }
}
