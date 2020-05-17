#include "lcc.h"

bool opt_E;
char **include_paths;
char *input_path;
char *output_path;
FILE *output_file;

static void usage(int code) {
    fprintf(stderr, "Usage: lcc [-E] [-o <output_path>] <file>\n");
    exit(code);
}
static char *get_output_filename() {
    char *filename = basename(strdup(input_path));
    int len = strlen(filename);

    if (3 <= len && strcmp(filename + len - 2, ".c") == 0) {
        filename[len - 1] = 's';
        return filename;
    }

    char *buf = malloc(len + 3);
    sprintf(buf, "%s.s", filename);
    return buf;
}

static void parse_args(int argc, char **argv) {
    input_path = NULL;
    output_path = NULL;
    include_paths = malloc(sizeof(char *) * argc);
    int npaths = 0;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            usage(0);
        }
        if (!strcmp(argv[i], "-o")) {
            if (!argv[++i]) {
                usage(1);
            }
            output_path = argv[i];
            continue;
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
        input_path = argv[i];
    }
    include_paths[npaths] = NULL;
    if (!input_path)
        error("no input file");
    if (!output_path)
        output_path = get_output_filename();
}

static void set_output_file() {
    if (strcmp(output_path, "-") == 0) {
        output_file = stdout;
        return;
    }
    output_file = fopen(output_path, "w");
    if (!output_file)
        error("cannot open output file: %s: %s", output_path, strerror(errno));
}

int main(int argc, char **argv) {
    parse_args(argc, argv);
    set_output_file();

    current_filename = input_path;
    Token *tok = read_file(input_path);
    if (opt_E) {
        print_tokens(tok);
        exit(0);
    }

    Program *prog = parse(tok);
    codegen(prog);
    return 0;
}
