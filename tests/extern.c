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

float add_float(float x, float y) { return x + y; }
double add_double(double x, double y) { return x + y; }
int add10(int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8,
          int x9, int x10) {
    return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10;
}
double adddouble10(double x1, double x2, double x3, double x4, double x5,
                   double x6, double x7, double x8, double x9, double x10) {
    return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10;
}
