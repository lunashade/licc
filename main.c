#include "lcc.h"

static char *filename;

static void usage() { fprintf(stderr, "Usage: lcc [-E] <file>\n"); }

int main(int argc, char **argv) {
    filename = NULL;
    bool opt_E;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            usage();
            exit(2);
        }
        if (!strcmp(argv[i], "-E")) {
            opt_E = true;
            continue;
        }

        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            error("unknown option: %s", argv[i]);
        }
        filename = argv[i];
    }
    if (!filename)
        error("no input file");

    Token *tok = tokenize_file(filename);
    tok = preprocess(tok);
    if (opt_E) {
        print_tokens(tok);
        exit(0);
    }

    Program *prog = parse(tok);
    // emit .file directive for assembler
    printf(".file 1 \"%s\"\n", filename);
    codegen(prog);
    return 0;
}
