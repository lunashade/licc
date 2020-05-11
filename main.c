#include "lcc.h"

static char *filename;

static void usage() {
    fprintf(stderr, "Usage: lcc <file>\n");
    exit(2);
}

int main(int argc, char **argv) {
    filename = NULL;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help"))
            usage();
        if (argv[i][0] == '-' && argv[i][1] != '\0'){
            error("unknown option: %s", argv[i]);
        }
        filename = argv[i];
    }
    if (!filename)
        error("no input file");

    Token *tok = tokenize_file(filename);
    tok = preprocess(tok);
    Program *prog = parse(tok);
    // emit .file directive for assembler
    printf(".file 1 \"%s\"\n", filename);
    codegen(prog);
    return 0;
}
