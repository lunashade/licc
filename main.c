#include "lcc.h"
static int align_to(int n, int align) {
    return (n + align - 1) & ~(align - 1);
} // align must 2-power

int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s: invalid number of argument\n", argv[0]);
    }

    Token *tok = tokenize(argv[1]);
    Program *prog = parse(tok);
    for (Function *fn = prog->fns; fn; fn=fn->next) {
        int offset = 32; // for callee-saved registers
        for (Var *v = fn->locals; v; v = v->next) {
            offset += v->ty->size;
            v->offset = offset;
        }
        fn->stacksize = align_to(offset, 16);
    }

    codegen(prog);
    return 0;
}
