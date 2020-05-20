#include "licc.h"

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void print_tokens(Token *head) {
    for (Token *tok = head; tok && tok->kind != TK_EOF; tok = tok->next) {
        if (tok->at_bol)
            fprintf(stdout, "\n");
        else if (tok->has_space)
            fprintf(stdout, " ");
        fprintf(stdout, "%.*s", tok->len, tok->loc);
    }
}
