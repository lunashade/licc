#include "licc.h"

bool opt_E = false;
bool opt_S;
bool opt_c;
bool opt_fpic = true;
char **include_paths;
char *input_dir;
FILE *tempfile;

static char *input_path;
static char *output_path;
static char *tempfile_path;

static void usage(int code) {
    fprintf(stderr, "Usage: licc [-ESc] [-fpic,-fPIC] [-fno-pic,-fno-PIC] [-o "
                    "<output_path>] [-I<include_dir>] <file>\n");
    exit(code);
}
static void define(char *str) {
    char *eq = strchr(str, '=');
    if (eq)
        define_macro(strndup(str, eq - str), eq + 1);
    else
        define_macro(str, "");
}

static void add_include_path(char *path) {
    static int len = 2;
    include_paths = realloc(include_paths, sizeof(char *) * len);
    include_paths[len - 2] = path;
    include_paths[len - 1] = NULL;
    len++;
}

static void add_default_include_paths(char *argv0) {
#ifndef LICC_LIB_DIR
    // Add ./include
    char *buf = malloc(strlen(argv0) + 10);
    sprintf(buf, "%s/include", dirname(strdup(argv0)));
    add_include_path(buf);
#else
    add_include_path(LICC_LIB_DIR "/include");
#endif

    add_include_path("/usr/local/include");
    add_include_path("/usr/include/x86_64-linux-gnu");
    add_include_path("/usr/include");
}

static char *get_output_filename() {
    char *filename = basename(strdup(input_path));
    int len = strlen(filename);

    if (3 <= len && strcmp(filename + len - 2, ".c") == 0) {
        if (opt_S)
            filename[len - 1] = 's';
        else
            filename[len - 1] = 'o';

        return filename;
    }

    char *buf = malloc(len + 3);
    if (opt_S)
        sprintf(buf, "%s.s", filename);
    else
        sprintf(buf, "%s.o", filename);
    return buf;
}

static void parse_args(int argc, char **argv) {
    input_path = NULL;
    output_path = NULL;
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
        if (!strcmp(argv[i], "-S")) {
            opt_S = true;
            continue;
        }
        if (!strcmp(argv[i], "-c")) {
            opt_c = true;
            continue;
        }
        if (!strcmp(argv[i], "-E")) {
            opt_E = true;
            continue;
        }
        if (!strcmp(argv[i], "-fpic") || !strcmp(argv[i], "-fPIC")) {
            opt_fpic = true;
            continue;
        }
        if (!strcmp(argv[i], "-fno-pic") || !strcmp(argv[i], "-fno-PIC")) {
            opt_fpic = false;
            continue;
        }
        if (!strncmp(argv[i], "-I", 2)) {
            add_include_path(argv[i] + 2);
            continue;
        }
        if (!strncmp(argv[i], "-D", 2)) {
            define(argv[i] + 2);
            continue;
        }
        // Ignored option
        if (!strncmp(argv[i], "-g", 2) || !strncmp(argv[i], "-O", 2) ||
            !strncmp(argv[i], "-W", 2)) {
            continue;
        }
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            error("unknown option: %s", argv[i]);
        }
        input_path = argv[i];
    }
    if (!input_path)
        error("no input file");
    if (!output_path)
        output_path = get_output_filename();
}

static void copy_file(FILE *in, FILE *out) {
    char buf[4096];
    for (;;) {
        int nr = fread(buf, 1, sizeof(buf), in);
        if (nr == 0)
            break;
        fwrite(buf, 1, nr, out);
    }
}

static void cleanup(void) {
    if (tempfile_path)
        unlink(tempfile_path);
}

int main(int argc, char **argv) {
    init_macros();
    parse_args(argc, argv);
    add_default_include_paths(argv[0]);
    atexit(cleanup);

    input_dir = dirname(strdup(input_path));

    tempfile_path = strdup("/tmp/licc-XXXXXX");
    int fd = mkstemp(tempfile_path);
    if (!fd)
        error("cannot create a temporary file: %s: %s", tempfile_path,
              strerror(errno));
    tempfile = fdopen(fd, "w");

    Token *tok = tokenize_file(input_path);
    if (!tok)
        error("%s: %s", input_path, strerror(errno));
    tok = preprocess(tok);
    if (opt_E) {
        print_tokens(tok);
        exit(0);
    }

    Program *prog = parse(tok);
    codegen(prog);

    if (opt_S) {
        // Write assembly to output file
        fseek(tempfile, 0, SEEK_SET);
        FILE *out;
        if (strcmp(output_path, "-") == 0)
            out = stdout;
        else {
            out = fopen(output_path, "w");
            if (!out)
                error("cannot open output file: %s: %s", output_path,
                      strerror(errno));
        }
        copy_file(tempfile, out);
        exit(0);
    }
    // Run assembler
    fclose(tempfile);
    pid_t pid;
    if ((pid = fork()) == 0) {
        // child process, run assembler
        execlp("as", "-c", "-o", output_path, tempfile_path, (char *)0);
        fprintf(stderr, "exec failed: as: %s", strerror(errno));
    }
    // Wait for child process
    for (;;) {
        int status;
        int w = waitpid(pid, &status, 0);
        if (!w) {
            error("waitpid failed: %s", strerror(errno));
            exit(1);
        }
        if (WIFEXITED(status))
            break;
    }
    return 0;
}
