#include "lcc.h"

// error report
static char *current_input;
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}
static void verror_at(char *loc, char *fmt, va_list ap) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}
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

static bool is_keyword(Token *tok) {
    static char *kw[] = {"return", "if",     "else", "for",
                         "while",  "sizeof", "int",  "char"};

    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (equal(tok, kw[i]))
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

static char *read_escape_char(char *ret, char *p) {
    switch (*p) {
    case 'a':
        *ret = '\a';
        return p++;
    case 'b':
        *ret = '\b';
        return p++;
    case 't':
        *ret = '\t';
        return p++;
    case 'r':
        *ret = '\r';
        return p++;
    case 'n':
        *ret = '\n';
        return p++;
    case 'v':
        *ret = '\v';
        return p++;
    case 'f':
        *ret = '\f';
        return p++;
    case 'e':
        *ret = 27;
        return p++;
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
            error_at(p, "expected hexadecimal sequence");
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
        return p++;
    }
}

static Token *read_string_literal(Token *cur, char *start) {
    char *p = start + 1;
    char *end = p;
    for (; *end != '"'; end++) {
        if (*end == '\0')
            error_at(start, "string literal not closed.");
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

Token *tokenize(char *p) {
    Token head = {};
    current_input = p;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (startswith(p, "==") || startswith(p, ">=") || startswith(p, "<=") ||
            startswith(p, "!=")) {
            cur = new_token(cur, TK_RESERVED, p, 2);
            p += 2;
            continue;
        }
        if (*p == '"') {
            cur = read_string_literal(cur, p);
            p += cur->len;
            continue;
        }
        if (ispunct(*p)) {
            cur = new_token(cur, TK_RESERVED, p, 1);
            p++;
            continue;
        }
        if (isalpha(*p)) {
            cur = new_token(cur, TK_IDENT, p, 0);
            char *q = p;
            while (isalnum(*p) || *p == '_') {
                p++;
            }
            cur->len = p - q;
            continue;
        }
        if (isdigit(*p)) {
            cur = new_token(cur, TK_NUM, p, 0);
            char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }
        error_at(p, "invalid token character");
    }
    new_token(cur, TK_EOF, p, 0);
    convert_keywords(head.next);
    return head.next;
}
