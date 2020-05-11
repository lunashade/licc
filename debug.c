#include "lcc.h"

void print_tokens(Token *head) {
    for (Token *tok = head; tok->kind != TK_EOF; tok = tok->next) {
        if (tok->at_bol)
            fprintf(stderr, "\n");
        else
            fprintf(stderr, " ");
        fprintf(stderr, "%.*s", tok->len, tok->loc);
    }
}
