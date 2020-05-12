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

//
// Preprocessor directive
//

// #if stack
typedef struct PPIf PPIf;
struct PPIf {
    PPIf *next;
    Token *tok;
};
static PPIf *current_if; // stack top

static void push_if(Token *tok) {
    PPIf *ppif = calloc(1, sizeof(PPIf));
    ppif->tok = tok;
    ppif->next = current_if;
    current_if = ppif;
}

static bool is_hash(Token *tok) { return tok->at_bol && equal(tok, "#"); }
static bool is_directive(Token *tok, char *s) {
    return is_hash(tok) && equal(tok->next, s);
}

static Token *skip_to_endif(Token *tok) {
    while (tok->kind != TK_EOF) {
        if (is_directive(tok, "endif")) {
            return tok;
        }
        if (is_directive(tok, "if")) {
            tok = skip_to_endif(tok->next->next);
        }
        tok = tok->next;
    }
    return tok;
}

static Token *copy_token(Token *tok) {
    Token *tok2 = malloc(sizeof(Token));
    *tok2 = *tok;
    tok2->next = NULL;
    return tok2;
}

static Token *new_eof(Token *tok) {
    Token *eof = copy_token(tok);
    eof->kind = TK_EOF;
    eof->len = 0;
    return eof;
}

static Token *skip_line(Token *tok) {
    while (tok->kind != TK_EOF && !tok->at_bol) {
        tok = tok->next;
    }
    return tok;
}

// read the token sequence until eol, copy the sequence to pp_line
static Token *read_pp_line(Token *tok, Token **pp_line) {
    Token head = {};
    Token *cur = &head;
    for (; !tok->at_bol; tok = tok->next) {
        cur = cur->next = copy_token(tok);
    }
    cur->next = new_eof(tok);
    *pp_line = head.next;
    return tok;
}

static Token *preprocess_file(char *);
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
        // "#" "include" TK_STR
        if (equal(tok, "include")) {
            if (tok->next->kind != TK_STR) {
                error_tok(tok->next, "preprocess: expected include file path");
            }
            tok = tok->next;
            Token *tok2 = preprocess_file(tok->contents);
            cur = cur->next = tok2;
            while (cur->next->kind != TK_EOF) {
                cur = cur->next;
            }
            tok = tok->next;
            if (!tok->at_bol)
                warn_tok(tok,
                         "preprocess: extra tokens after include directive");
            tok = skip_line(tok);
            continue;
        }
        // "#" "if" const-expr
        if (equal(tok, "if")) {

            Token *if_line;
            tok = read_pp_line(tok->next, &if_line);
            long val = const_expr(&if_line, if_line);
            if (if_line->kind != TK_EOF)
                error_tok(if_line,
                          "preprocess: extra token after #if const-expr");
            push_if(tok);

            if (!val) {
                tok = skip_to_endif(tok);
            }
            continue;
        }
        // "#" "endif"
        if (equal(tok, "endif")) {
            if (!current_if)
                error_tok(tok, "stray endif");
            current_if = current_if->next;
            tok = tok->next;
            if (!tok->at_bol)
                warn_tok(tok, "preprocess: extra tokens after endif directive");
            tok = skip_line(tok);
            continue;
        }
        if (tok->at_bol)
            // null directive
            continue;

        error_tok(tok, "preprocess: invalid preprocessor directive");
    }
    cur->next = tok;
    return head.next;
}

static Token *preprocess_file(char *filename) {
    Token *tok = tokenize_file(filename);
    return preprocess(tok);
}

Token *read_file(char *filename) {
    Token *tok = preprocess_file(filename);
    concat_string_literals(tok);
    convert_keywords(tok);
    return tok;
}
