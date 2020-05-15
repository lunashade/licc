#include "lcc.h"

// Token
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

// er -> (ee) -> er->next
static Token *append(Token *er, Token *ee) {
    if (!er || er->kind == TK_EOF)
        return ee;

    Token head = {};
    Token *cur = &head;

    for (; er && er->kind != TK_EOF; er = er->next) {
        cur = cur->next = copy_token(er);
    }
    cur->next = ee;
    return head.next;
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

// #define macro
typedef struct Macro Macro;
struct Macro {
    Macro *next;
    char *name;
    Token *body;
    bool deleted;
};
static Macro *macro;
static void push_macro(char *name) {
    Macro *m = calloc(1, sizeof(Macro));
    m->next = macro;
    m->name = name;
    macro = m;
}

static bool ishidden(Token *tok) {
    for (Hideset *hs = tok->hideset; hs; hs = hs->next) {
        if (equal(tok, hs->name)) {
            return true;
        }
    }
    return false;
}

static Hideset *new_hideset(char *name) {
    Hideset *hs = calloc(1, sizeof(Hideset));
    hs->name = name;
    return hs;
}

static Hideset *hsunion(Hideset *hs1, Hideset *hs2) {
    Hideset head = {};
    Hideset *cur = &head;
    for (; hs1; hs1 = hs1->next) {
        cur = cur->next = new_hideset(hs1->name);
    }
    cur->next = hs2;
    return head.next;
}

static Token *hsadd(Hideset *hs, Token *tok) {
    Token head = {};
    Token *cur = &head;
    for (; tok; tok = tok->next) {
        Token *t = copy_token(tok);
        t->hideset = hsunion(tok->hideset, hs);
        cur = cur->next = t;
    }
    return head.next;
}

static Macro *find_macro(Token *tok) {
    if (tok->kind != TK_IDENT) {
        return NULL;
    }
    for (Macro *m = macro; m; m = m->next) {
        if (equal(tok, m->name)) {
            return m->deleted ? NULL : m;
        }
    }
    return NULL;
}

static bool expand_macro(Token **rest, Token *tok) {
    if (ishidden(tok)) {
        return false;
    }
    Macro *m = find_macro(tok);
    if (!m) {
        return false;
    }
    Hideset *hs = hsunion(new_hideset(get_ident(tok)), tok->hideset);
    Token *body = hsadd(hs, m->body);
    *rest = append(body, tok->next);
    return true;
}

// #if stack
typedef struct PPCond PPCond;
struct PPCond {
    PPCond *next;
    Token *tok;
    long val;
    enum { IN_THEN, IN_ELSE } ctx;
};
static PPCond *current_if; // stack top

static void push_if(Token *tok, long val) {
    PPCond *ppif = calloc(1, sizeof(PPCond));
    ppif->tok = tok;
    ppif->next = current_if;
    ppif->val = val;
    ppif->ctx = IN_THEN;
    current_if = ppif;
}

static bool is_hash(Token *tok) { return tok->at_bol && equal(tok, "#"); }
static bool is_directive(Token *tok, char *s) {
    return is_hash(tok) && equal(tok->next, s);
}

static Token *skip_to_endif(Token *tok) {
    while (tok->kind != TK_EOF) {
        if (is_directive(tok, "endif")) {
            break;
        }
        if (is_directive(tok, "if")) {
            tok = skip_to_endif(tok->next->next);
        }
        tok = tok->next;
    }
    return tok;
}

static Token *skip_to_cond_directive(Token *tok) {
    while (tok->kind != TK_EOF) {
        if (is_directive(tok, "endif")) {
            break;
        }
        if (is_directive(tok, "elif")) {
            break;
        }
        if (is_directive(tok, "else")) {
            break;
        }
        if (is_directive(tok, "if") || is_directive(tok, "ifdef") ||
            is_directive(tok, "ifndef")) {
            tok = skip_to_endif(tok->next->next);
        }
        tok = tok->next;
    }
    return tok;
}

Token *preprocess(Token *tok) {
    Token head = {};
    Token *cur = &head;

    while (tok->kind != TK_EOF) {
        if (expand_macro(&tok, tok))
            continue;
        if (!is_hash(tok)) {
            cur = cur->next = tok;
            tok = tok->next;
            continue;
        }
        // preprocessor directive
        Token *start = tok;
        tok = tok->next;
        // #include TK_STR
        if (equal(tok, "include")) {
            tok = tok->next;
            if (tok->kind != TK_STR) {
                error_tok(tok->next, "preprocess: expected include file path");
            }
            Token *tok2 = tokenize_file(tok->contents);
            if (!tok2)
                error_tok(tok, "%s", strerror(errno));
            tok = tok->next;
            if (!tok->at_bol)
                warn_tok(tok,
                         "preprocess: extra tokens after include directive");
            tok = skip_line(tok);
            tok = append(tok2, tok);
            continue;
        }
        // #define ident replacements
        if (equal(tok, "define")) {
            push_macro(get_ident(tok->next));
            Token *body;
            tok = read_pp_line(tok->next->next, &body);
            macro->body = body;
            continue;
        }
        // #undef ident
        if (equal(tok, "undef")) {
            push_macro(get_ident(tok->next));
            macro->deleted = true;
            tok = skip_line(tok);
            continue;
        }
        // #if const-expr
        if (equal(tok, "if")) {
            Token *if_line;
            tok = read_pp_line(tok->next, &if_line);
            if_line = preprocess(if_line);
            long val = const_expr(&if_line, if_line);
            if (if_line->kind != TK_EOF)
                error_tok(if_line,
                          "preprocess: extra token after #if const-expr");
            push_if(tok, val);

            if (!val) {
                tok = skip_to_cond_directive(tok);
            }
            continue;
        }
        // #ifndef ident
        if (equal(tok, "ifndef")) {
            bool defined = !find_macro(tok->next);
            push_if(tok->next, defined);
            tok = skip_line(tok->next->next);
            if (!defined) {
                tok = skip_to_cond_directive(tok);
            }
            continue;
        }
        // #ifdef ident
        if (equal(tok, "ifdef")) {
            bool defined = find_macro(tok->next);
            push_if(tok->next, defined);
            tok = skip_line(tok->next->next);
            if (!defined) {
                tok = skip_to_cond_directive(tok);
            }
            continue;
        }
        // #elif const-expr
        if (equal(tok, "elif")) {
            if (!current_if)
                error_tok(tok, "preprocess: stray #elif");
            if (current_if->ctx == IN_ELSE)
                error_tok(tok, "preprocess: #elif after #else");
            Token *if_line;
            tok = read_pp_line(tok->next, &if_line);
            if_line = preprocess(if_line);
            long val = const_expr(&if_line, if_line);
            if (if_line->kind != TK_EOF)
                error_tok(if_line,
                          "preprocess: extra token after #elif const-expr");
            // if current_if->val is true, don't update to skip elif or else
            if (current_if->val) {
                tok = skip_to_endif(tok);
                continue;
            }
            // if current_if->val is false
            current_if->val = val;
            if (!val) {
                tok = skip_to_cond_directive(tok);
            }
            continue;
        }
        // #else
        if (equal(tok, "else")) {
            if (!current_if)
                error_tok(tok, "preprocess: stray #else");
            if (current_if->ctx == IN_ELSE)
                error_tok(tok, "preprocess: #else after #else");

            current_if->ctx = IN_ELSE;
            tok = tok->next;
            if (current_if->val) {
                tok = skip_to_endif(tok);
            }
            continue;
        }
        // #endif
        if (equal(tok, "endif")) {
            if (!current_if)
                error_tok(tok, "preprocess: stray #endif");
            current_if = current_if->next;
            tok = tok->next;
            if (!tok->at_bol)
                warn_tok(tok, "preprocess: extra tokens after endif directive");
            tok = skip_line(tok);
            continue;
        }
        // # (null directive)
        if (tok->at_bol)
            continue;

        error_tok(tok, "preprocess: invalid preprocessor directive");
    }
    cur->next = tok;
    return head.next;
}

Token *preprocess_file(char *filename) {
    Token *tok = tokenize_file(filename);
    return preprocess(tok);
}

Token *read_file(char *filename) {
    Token *tok = preprocess_file(filename);
    concat_string_literals(tok);
    convert_keywords(tok);
    return tok;
}
