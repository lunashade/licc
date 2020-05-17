#include "lcc.h"

void print_tokens(Token *head) {
    for (Token *tok = head; tok && tok->kind != TK_EOF; tok = tok->next) {
        if (tok->at_bol)
            fprintf(stderr, "\n");
        else if (tok->has_space)
            fprintf(stderr, " ");
        fprintf(stderr, "%.*s", tok->len, tok->loc);
    }
}
