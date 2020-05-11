#include "lcc.h"

static void convert_keywords(Token *tok) {
    for (Token *t = tok; t->kind != TK_EOF; t = t->next) {
        if (t->kind == TK_IDENT && is_keyword(t)) {
            t->kind = TK_RESERVED;
        }
    }
}
static void concat_string_literals(Token *tok) {
    for (Token *t = tok; t->kind != TK_EOF; t = t->next) {
        if (t->kind != TK_STR) {
            continue;
        }
        while (t->next->kind == TK_STR) {
            long len = t->contents_len - 1 + t->next->contents_len;
            t->contents = realloc(t->contents, len);
            strncat(t->contents, t->next->contents, t->next->contents_len);
            t->len = t->len + t->next->len;
            t->contents_len = len;
            t->next = t->next->next;
        }
    }
}

static bool is_hash(Token *tok) { return tok->at_bol && equal(tok, "#"); }

static Token *preprocess(Token *tok) {
    Token head = {};
    Token *cur = &head;

    while (tok->kind != TK_EOF) {
        if (!is_hash(tok)) {
            cur = cur->next = tok;
            tok = tok->next;
            continue;
        }
        // preprocessor directive
        tok = tok->next;
        if (tok->at_bol)
            // null directive
            continue;

        error_tok(tok, "preprocess: invalid preprocessor directive");
    }
    cur->next = tok;
    return head.next;
}

Token *read_file(char *filename) {
    Token *tok = tokenize_file(filename);
    tok = preprocess(tok);
    concat_string_literals(tok);
    convert_keywords(tok);
    return tok;
}
