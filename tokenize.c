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
  static char *kw[] = {"return", "if", "else", "for", "while"};

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
    if (ispunct(*p)) {
      cur = new_token(cur, TK_RESERVED, p, 1);
      p++;
      continue;
    }
    if (isalpha(*p)) {
      cur = new_token(cur, TK_IDENT, p, 0);
      char *q = p;
      while (isalnum(*p)) {
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
