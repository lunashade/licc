#ifndef __STDARG_H
#define __STDARG_H

typedef struct {
      unsigned int gp_offset;
        unsigned int fp_offset;
          void *overflow_arg_area;
            void *reg_save_area;
} va_list[1];

#define va_start(ap, last) __builtin_va_start(ap)
#define va_end(ap) 0

#define __GNUC_VA_LIST 1
typedef va_list __gnuc_va_list;

#endif
