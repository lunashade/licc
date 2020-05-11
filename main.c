#include "lcc.h"

static char *filename;

int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s: invalid number of argument\n", argv[0]);
    }
    filename = argv[1];

    Token *tok = tokenize_file(filename);
    tok = preprocess(tok);
    Program *prog = parse(tok);
    // emit .file directive for assembler
    printf(".file 1 \"%s\"\n", filename);
    codegen(prog);
    return 0;
}
