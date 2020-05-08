#include "lcc.h"

static char *KEYWORDS[] = {
    "return",  "if",     "else",     "for",      "while",  "sizeof",
    "int",     "char",   "struct",   "union",    "short",  "long",
    "void",    "signed", "unsigned", "typedef",  "_Bool",  "static",
    "enum",    "goto",   "break",    "continue", "switch", "case",
    "default", "extern", "_Alignof", "_Alignas"};
static char *MULTIPUNCT[] = { // must be length descending order
    "<<=", ">>=", "<=", "==", ">=", "!=", "->", "+=", "-=", "*=", "/=",
    "%=",  "&=",  "|=", "^=", "++", "--", "&&", "||", "<<", ">>"};
// error report
static char *current_filename;
static char *current_input;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}
static void verror_at(int lineno, char *kind, char *loc, char *fmt,
                      va_list ap) {
    char *line = loc;
    while (current_input < line && line[-1] != '\n')
        line--;
    char *lineend = loc;
    while (*lineend != '\n')
        lineend++;

    int indent = fprintf(stderr, "%s:%d:%s: ", current_filename, lineno, kind);
    fprintf(stderr, "%.*s\n", (int)(lineend - line), line);
    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}
void error_at(char *loc, char *fmt, ...) {
    int lineno = 1;
    for (char *p = current_input; p < loc; p++)
        if (*p == '\n')
            lineno++;

    va_list ap;
    va_start(ap, fmt);
    verror_at(lineno, "error", loc, fmt, ap);
    exit(1);
}

void warn_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->lineno, "warning", tok->loc, fmt, ap);
}
void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->lineno, "error", tok->loc, fmt, ap);
    exit(1);
}

// utility
static bool startswith(char *p, char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

// Tokenizer
static Token *new_token(Token *cur, TokenKind kind, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

static int is_multipunct(char *p) {
    for (int i = 0; i < sizeof(MULTIPUNCT) / sizeof(*MULTIPUNCT); i++) {
        if (startswith(p, MULTIPUNCT[i])) {
            return strlen(MULTIPUNCT[i]);
        }
    }
    return 0;
}

static bool is_keyword(Token *tok) {
    for (int i = 0; i < sizeof(KEYWORDS) / sizeof(*KEYWORDS); i++) {
        if (equal(tok, KEYWORDS[i]))
            return true;
    }
    return false;
}

static void convert_keywords(Token *tok) {
    for (Token *t = tok; t->kind != TK_EOF; t = t->next) {
        if (t->kind == TK_IDENT && is_keyword(t)) {
            t->kind = TK_RESERVED;
        }
    }
}
static void add_lineno(Token *tok) {
    char *p = current_input;
    int lineno = 1;
    for (Token *t = tok; t->kind != TK_EOF; t = t->next) {
        for (; p < t->loc; p++) {
            if (*p == '\n') {
                lineno++;
            }
        }
        t->lineno = lineno;
    }
}

static char *read_escape_char(char *ret, char *p) {
    switch (*p) {
    case 'a':
        *ret = 7;
        return p + 1;
    case 'b':
        *ret = 8;
        return p + 1;
    case 't':
        *ret = 9;
        return p + 1;
    case 'n':
        *ret = 10;
        return p + 1;
    case 'v':
        *ret = 11;
        return p + 1;
    case 'f':
        *ret = 12;
        return p + 1;
    case 'r':
        *ret = 13;
        return p + 1;
    case 'e':
        *ret = 27;
        return p + 1;
    case 'x': {
        int r;
        p++;
        if ('0' <= *p && *p <= '9') {
            r = *p++ - '0';
        } else if ('a' <= *p && *p <= 'f') {
            r = 10 + (*p++ - 'a');
        } else if ('A' <= *p && *p <= 'F') {
            r = 10 + (*p++ - 'A');
        } else {
            error_at(p, "tokenize: expected hexadecimal sequence");
        }
        if ('0' <= *p && *p <= '9') {
            r = (r << 4) | (*p++ - '0');
        } else if ('a' <= *p && *p <= 'f') {
            r = (r << 4) | (10 + (*p++ - 'a'));
        } else if ('A' <= *p && *p <= 'F') {
            r = (r << 4) | (10 + (*p++ - 'A'));
        }
        *ret = r;
        return p;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': {
        int r = *p++ - '0';
        if ('0' <= *p && *p <= '7') {
            r = (r << 3) | (*p++ - '0');
        }
        if ('0' <= *p && *p <= '7') {
            r = (r << 3) | (*p++ - '0');
        }
        *ret = r;
        return p;
    }
    default:
        *ret = *p;
        return p + 1;
    }
}

static Token *read_char_literal(Token *cur, char *start) {
    char *p = start + 1;
    if (*p == '\0') {
        error_at(start, "tokenize: unclosed char literal.");
    }

    char c;
    if (*p == '\\') {
        p = read_escape_char(&c, p + 1);
    } else {
        c = *p;
        p++;
    }
    if (*p != '\'') {
        error_at(start, "tokenize: char literal too long.");
    }

    cur = new_token(cur, TK_NUM, start, p - start + 1);
    cur->val = c;
    return cur;
}
static Token *read_int_literal(Token *cur, char *start) {
    char *p = start;
    int base = 10;
    if (!strncasecmp(p, "0x", 2) && isalnum(p[2])) {
        p += 2;
        base = 16;
    } else if (!strncasecmp(p, "0b", 2) && isalnum(p[2])) {
        p += 2;
        base = 2;
    } else if (*p == '0') {
        base = 8;
    }
    long val = strtoul(p, &p, base);
    if (isalnum(*p) || *p == '_')
        error_at(p, "tokenize: invalid digit");

    cur = new_token(cur, TK_NUM, start, p - start);
    cur->val = val;
    return cur;
}

static Token *read_string_literal(Token *cur, char *start) {
    char *p = start + 1;
    char *end = p;
    for (; *end != '"'; end++) {
        if (*end == '\0')
            error_at(start, "tokenize: string literal not closed.");
        if (*end == '\\')
            end++; // skip escaped char
    }
    char *buf = malloc(end - p + 1);
    int len = 0;

    for (; (end - p) > 0;) {
        if (*p == '\\') {
            char c;
            p = read_escape_char(&c, p + 1);
            buf[len++] = c;
        } else {
            buf[len++] = *p;
            p++;
        }
    }

    buf[len++] = '\0';

    cur = new_token(cur, TK_STR, start, end - start + 1);
    cur->contents = buf;
    cur->contents_len = len;
    return cur;
}

Token *tokenize(char *filename, char *p) {
    current_filename = filename;
    current_input = p;

    Token head = {};
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (startswith(p, "//")) {
            p += 2;
            while (*p != '\n')
                p++;
            continue;
        }
        if (startswith(p, "/*")) {
            char *q = strstr(p + 2, "*/");
            if (!q)
                error_at(p, "tokenize: unclosed block comment");
            p = q + 2;
            continue;
        }
        if (is_multipunct(p)) {
            int l = is_multipunct(p);
            cur = new_token(cur, TK_RESERVED, p, l);
            p += l;
            continue;
        }
        if (*p == '"') {
            cur = read_string_literal(cur, p);
            p += cur->len;
            continue;
        }
        if (*p == '\'') {
            cur = read_char_literal(cur, p);
            p += cur->len;
            continue;
        }
        if (isalpha(*p) || *p == '_') {
            cur = new_token(cur, TK_IDENT, p, 0);
            char *q = p;
            while (isalnum(*p) || *p == '_') {
                p++;
            }
            cur->len = p - q;
            continue;
        }
        if (ispunct(*p)) {
            cur = new_token(cur, TK_RESERVED, p, 1);
            p++;
            continue;
        }
        if (isdigit(*p)) {
            cur = read_int_literal(cur, p);
            p += cur->len;
            continue;
        }
        error_at(p, "tokenize: invalid token character");
    }
    new_token(cur, TK_EOF, p, 0);
    add_lineno(head.next);
    convert_keywords(head.next);
    return head.next;
}
