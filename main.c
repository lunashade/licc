#include "lcc.h"

static char *entry_filename;

static void usage() { fprintf(stderr, "Usage: lcc [-E] <file>\n"); }

int main(int argc, char **argv) {
    entry_filename = NULL;
    bool opt_E = false;

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
        entry_filename = argv[i];
    }
    if (!entry_filename)
        error("no input file");

    current_filename = entry_filename;
    Token *tok = read_file(entry_filename);
    if (opt_E) {
        print_tokens(tok);
        exit(0);
    }

    Program *prog = parse(tok);
    // emit .file directive for assembler
    printf(".file 1 \"%s\"\n", entry_filename);
    codegen(prog);
    return 0;
}
