#include "lcc.h"

void print_tokens(Token *head) {
    for (Token *tok = head; tok && tok->kind != TK_EOF; tok = tok->next) {
        if (tok->at_bol)
            fprintf(stdout, "\n");
        else if (tok->has_space)
            fprintf(stdout, " ");
        fprintf(stdout, "%.*s", tok->len, tok->loc);
    }
}
