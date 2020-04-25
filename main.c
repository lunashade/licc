#include "lcc.h"
static int align_to(int n, int align) {
    return (n + align - 1) & ~(align - 1);
} // align must 2-power

int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s: invalid number of argument\n", argv[0]);
    }

    Token *tok = tokenize(argv[1]);
    Function *prog = parse(tok);
    int offset = 32; // for callee-saved registers
    for (LVar *v = prog->locals; v; v = v->next) {
        offset += 8;
        v->offset = offset;
    }
    prog->stacksize = align_to(offset, 16);

    codegen(prog);
    return 0;
}
