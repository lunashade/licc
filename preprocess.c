#include "lcc.h"

//
static char *pathjoin(char *dir, char *file) {
    char *buf = malloc(strlen(dir) + strlen(file) + 2);
    sprintf(buf, "%s/%s", dir, file);
    return buf;
}

static bool file_exists(char *path) {
    struct stat st;
    return !stat(path, &st);
}

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

// er -> (ee)
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

// read token sequence until "," or ")", copy to actual
static Token *read_actual(Token *tok, Token **actual) {
    Token head = {};
    Token *cur = &head;
    int level = 0;
    for (; level > 0 || (!equal(tok, ",") && !equal(tok, ")"));
         tok = tok->next) {
        cur = cur->next = copy_token(tok);
        if (equal(tok, "("))
            level++;
        if (equal(tok, ")"))
            level--;
    }
    *actual = head.next;
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
typedef struct MacroParams MacroParams;
struct MacroParams {
    MacroParams *next;
    char *name;
    Token *tok;
    Token *actual; // actual token sequence
};

static MacroParams *new_params(Token *tok) {
    MacroParams *fp = calloc(1, sizeof(MacroParams));
    fp->tok = copy_token(tok);
    fp->name = get_ident(fp->tok);
    return fp;
}
typedef struct Macro Macro;
struct Macro {
    Macro *next;
    char *name;
    Token *body;
    bool deleted;
    bool funclike;
    MacroParams *params;
    int n_param;
};
static Macro *macro;
static Macro *push_macro(char *name) {
    Macro *m = calloc(1, sizeof(Macro));
    m->next = macro;
    m->name = name;
    macro = m;
    return m;
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

static bool inhideset(Hideset *hs, char *loc, int len) {
    for (; hs; hs = hs->next) {
        if (strlen(hs->name) == len && !strncmp(hs->name, loc, len))
            return true;
    }
    return false;
}

static Hideset *hsintersect(Hideset *hs1, Hideset *hs2) {
    Hideset head = {};
    Hideset *cur = &head;
    for (; hs1; hs1 = hs1->next) {
        if (inhideset(hs2, hs1->name, strlen(hs1->name)))
            cur = cur->next = new_hideset(hs1->name);
    }
    return head.next;
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

static MacroParams *find_param(Token *tok, MacroParams *fp) {
    for (; fp; fp = fp->next) {
        if (equal(tok, fp->name)) {
            return fp;
        }
    }
    return NULL;
}

static char *quote_string(char *str) {
    int bufsize = 3;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\\' || str[i] == '"')
            bufsize++;
        bufsize++;
    }
    char *buf = malloc(bufsize);
    char *p = buf;
    *p++ = '"';
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\\' || str[i] == '"')
            *p++ = '\\';
        *p++ = str[i];
    }
    *p++ = '"';
    *p++ = '\0';
    return buf;
}

static char *join_token(Token *tok, Token *end) {
    int len = 1;
    for (Token *t = tok; t != end; t = t->next) {
        if (t != tok && t->has_space)
            len++;
        len += t->len;
    }
    char *buf = malloc(len);

    int pos = 0;
    for (Token *t = tok; t != end; t = t->next) {
        if (t != tok && t->has_space) {
            buf[pos++] = ' ';
        }
        strncpy(buf + pos, t->loc, t->len);
        pos += t->len;
    }
    buf[pos] = '\0';
    return buf;
}

static Token *new_string_token(char *buf, Token *tmpl) {
    return tokenize(tmpl->filename, tmpl->fileno, buf);
}
static Token *new_number_token(int val, Token *tmpl) {
    char *buf = malloc(20);
    sprintf(buf, "%d\n", val);
    return tokenize(tmpl->filename, tmpl->fileno, buf);
}

static Token *stringize(Token *tok, Token *actual) {
    char *buf = join_token(actual, NULL);
    buf = quote_string(buf);
    return tokenize(tok->filename, tok->fileno, buf);
}

static Token *glue(Token *cur, Token *tok) {
    char *buf = malloc(cur->len + tok->len + 1);
    sprintf(buf, "%.*s%.*s", cur->len, cur->loc, tok->len, tok->loc);

    Token *tok2 = tokenize(cur->filename, cur->fileno, buf);
    if (tok2->next->kind != TK_EOF) {
        error_tok(cur, "glued token `%s` is invalid", buf);
    }
    return tok2;
}

static Token *subst(Macro *m) {
    Token head = {};
    Token *cur = &head;
    for (Token *tok = m->body; tok->kind != TK_EOF; tok = tok->next) {
        MacroParams *fp = find_param(tok, m->params);
        if (fp) {
            if (!fp->actual && equal(tok->next, "##")) {
                tok = tok->next;
                continue;
            }
            if (!fp->actual) {
                continue;
            }
            for (Token *t = fp->actual; t; t = t->next) {
                cur = cur->next = copy_token(t);
            }
            continue;
        }
        // stringize
        if (equal(tok, "#")) {
            // tok  tok->next
            // #    IDENT(end)
            MacroParams *fp = find_param(tok->next, m->params);
            if (fp) {
                cur = cur->next = stringize(tok, fp->actual);
                tok = tok->next;
                continue;
            }
        }
        // glue
        if (equal(tok, "##")) {
            // tok  tok->next
            // ##   any(end)
            MacroParams *rhs = find_param(tok->next, m->params);
            if (!rhs) {
                // x ## 2 => x2
                *cur = *glue(cur, tok->next);
                tok = tok->next;
                continue;
            }
            // x ## y => x(actual of y)
            if (!rhs->actual) {
                continue;
            }
            *cur = *glue(cur, rhs->actual);
            for (Token *t = rhs->actual->next; t; t = t->next) {
                cur = cur->next = copy_token(t);
            }
            tok = tok->next;
            continue;
        }
        cur = cur->next = copy_token(tok);
        continue;
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
    Token *macro_ident = tok;
    Hideset *tokhs = new_hideset(get_ident(macro_ident));
    Hideset *hs1 = hsunion(tokhs, macro_ident->hideset);
    if (!m->funclike) {
        Token *body = hsadd(hs1, m->body);
        *rest = append(body, tok->next);
        return true;
    }
    if (!equal(tok->next, "("))
        return false;
    tok = skip(tok->next, "(");
    MacroParams *fp = m->params;
    for (int i = 0; i < m->n_param; i++) {
        if (i > 0) {
            tok = skip(tok, ",");
        }
        Token *actual;
        tok = read_actual(tok, &actual);
        fp->actual = actual;
        fp = fp->next;
    }
    Token *rparen = tok;
    Hideset *hs2 = hsintersect(hs1, hsunion(tokhs, rparen->hideset));
    tok = skip(tok, ")");
    Token *body = subst(m);
    body = hsadd(hs2, body);
    *rest = append(body, tok);
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

// tok == "define"
static void read_macro_def(Token **rest, Token *tok) {
    tok = skip(tok, "define");
    Macro *m = push_macro(get_ident(tok));
    tok = tok->next;

    m->funclike = (!tok->has_space && equal(tok, "("));
    if (m->funclike) {
        tok = skip(tok, "(");
        MacroParams head = {};
        MacroParams *cur = &head;
        int cnt = 0;
        while (!equal(tok, ")")) {
            if (cnt++ > 0) {
                tok = skip(tok, ",");
            }
            cur = cur->next = new_params(tok);
            tok = tok->next;
        }
        tok = skip(tok, ")");
        m->params = head.next;
        m->n_param = cnt;
    }
    Token *body;
    tok = read_pp_line(tok, &body);
    m->body = body;
    *rest = tok;
    return;
}
static Token *read_const_expr(Token *tok2, Token **line) {
    tok2 = read_pp_line(tok2, line);
    Token head = {};
    Token *cur = &head;

    Token *tok = *line;
    while (tok->kind != TK_EOF) {
        if (equal(tok, "defined")) {
            Token *defined = tok;
            tok = tok->next;
            bool has_paren = consume(&tok, tok, "(");
            if (tok->kind != TK_IDENT)
                error_tok(tok, "preprocess: macro name must be an identifier");
            bool m = find_macro(tok);
            tok = tok->next;
            if (has_paren) {
                tok = skip(tok, ")");
            }
            cur = cur->next = new_number_token(m ? 1 : 0, defined);
            continue;
        }
        cur = cur->next = tok;
        tok = tok->next;
    }

    cur->next = tok;
    *line = head.next;
    return tok2;
}

static long eval_const_expr(Token **rest, Token *tok) {
    Token *if_line;
    tok = read_const_expr(tok->next, &if_line);
    if_line = preprocess(if_line);
    for (Token *t = if_line; t->kind != TK_EOF; t = t->next) {
        if (t->kind == TK_IDENT) {
            // replace remaining ident -> 0
            Token *next = t->next;
            *t = *new_number_token(0, t);
            t->next = next;
        }
    }
    long val = const_expr(&if_line, if_line);
    if (if_line->kind != TK_EOF)
        error_tok(if_line, "preprocess: extra token after #if const-expr");
    *rest = tok;
    return val;
}

static char *read_include_path(Token **rest, Token *tok) {
    // #include "filepath"
    if (tok->kind == TK_STR) {
        // Do not escape
        char *filename = strndup(tok->loc + 1, tok->len - 2);
        tok = tok->next;
        if (!tok->at_bol)
            warn_tok(tok, "preprocess: extra tokens after include directive");
        *rest = skip_line(tok);
        return filename;
    }
    // #include <foo.h>
    if (equal(tok, "<")) {
        Token *start = tok;
        while (!equal(tok, ">")) {
            if (tok->kind == TK_EOF)
                error_tok(tok, "preprocess: expect `>`");
            tok = tok->next;
        }
        char *filename = join_token(start->next, tok);
        tok = skip(tok, ">");
        *rest = skip_line(tok);
        // TODO: actual include path
        char *path = pathjoin(".", filename);
        if (!file_exists(path))
            error_tok(start, "`%s`: file not found", filename);
        return path;
    }
    // #include MACRO
    if (tok->kind == TK_IDENT) {
        Token *tok2;
        tok = read_pp_line(tok, &tok2);
        tok2 = preprocess(tok2);
        char *path = read_include_path(&tok2, tok2);
        if (tok2->kind != TK_EOF) {
            error_tok(tok, "preprocess: failed include");
        }
        *rest = tok;
        return path;
    }
    error_tok(tok->next, "preprocess: expected include file path");
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
            char *path = read_include_path(&tok, tok->next);
            Token *tok2 = tokenize_file(path);
            if (!tok2)
                error_tok(tok, "%s", strerror(errno));
            tok = append(tok2, tok);
            continue;
        }
        // #define ident replacements
        if (equal(tok, "define")) {
            read_macro_def(&tok, tok);
            continue;
        }
        // #undef ident
        if (equal(tok, "undef")) {
            Macro *m = push_macro(get_ident(tok->next));
            m->deleted = true;
            tok = skip_line(tok);
            continue;
        }
        // #if const-expr
        if (equal(tok, "if")) {
            long val = eval_const_expr(&tok, tok);
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
            long val = eval_const_expr(&tok, tok);
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
