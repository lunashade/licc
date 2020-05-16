#include "lcc.h"

static char *entry_filename;
bool opt_E;
char **include_paths;

static void usage() { fprintf(stderr, "Usage: lcc [-E] <file>\n"); }

static void parse_args(int argc, char **argv) {
    include_paths = malloc(sizeof(char *) * argc);
    int npaths = 0;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            usage();
            exit(2);
        }
        if (!strcmp(argv[i], "-E")) {
            opt_E = true;
            continue;
        }
        if (!strncmp(argv[i], "-I", 2)) {
            include_paths[npaths++] = argv[i] + 2;
            continue;
        }

        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            error("unknown option: %s", argv[i]);
        }
        entry_filename = argv[i];
    }
    include_paths[npaths] = NULL;
    if (!entry_filename)
        error("no input file");
}

int main(int argc, char **argv) {
    entry_filename = NULL;
    parse_args(argc, argv);

    current_filename = entry_filename;
    Token *tok = read_file(entry_filename);
    if (opt_E) {
        print_tokens(tok);
        exit(0);
    }

    Program *prog = parse(tok);
    codegen(prog);
    return 0;
}
