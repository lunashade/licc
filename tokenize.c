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
Token *new_token(Token *cur, TokenKind kind, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = str;
  tok->len = len;
  cur->next = tok;
  return tok;
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
    if (startswith(p, "return")) {
      cur = new_token(cur, TK_RESERVED, p, 6);
      p += 6;
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
  return head.next;
}
