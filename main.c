#include "lcc.h"

static char *readfile(char *path) {
    FILE *fp = stdin;
    if (strcmp(path, "-")) {
        fp = fopen(path, "r");
        if (!fp) {
            error("main: cannot open file %s: %s", path, strerror(errno));
        }
    }
    int buflen = 4096;
    int nread = 0;
    char *buf = malloc(buflen);

    for (;;) {
        int end = buflen - 2;
        int n = fread(buf + nread, 1, end - nread, fp);
        if (n == 0)
            break;
        nread += n;
        if (nread == end) {
            buflen *= 2;
            buf = realloc(buf, buflen);
        }
    }

    if (fp != stdin)
        fclose(fp);

    if (nread == 0 || buf[nread - 1] != '\n')
        buf[nread++] = '\n';
    buf[nread] = '\0';

    // emit .file directive for assembler
    printf(".file 1 \"%s\"\n", path);
    return buf;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s: invalid number of argument\n", argv[0]);
    }

    char *input = readfile(argv[1]);
    Token *tok = tokenize(argv[1], input);
    Program *prog = parse(tok);
    codegen(prog);
    return 0;
}
