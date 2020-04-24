#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_RESERVED, // Keywords, punctuators
  TK_NUM,      // numbers
  TK_EOF,      // end-of-file
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  long val;
  char *loc;
  int len;
};

// error report
static char *current_input;
static void error(char *fmt, ...) {
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
static void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->loc, fmt, ap);
}

// Token
static Token *new_token(Token *cur, TokenKind kind, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

static Token *tokenize(char *p) {
  Token head = {};
  current_input = p;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }
    if (*p == '+' || *p == '-') {
      cur = new_token(cur, TK_RESERVED, p++, 1);
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

static long number(Token *tok) {
  if (tok->kind != TK_NUM) {
    error_tok(tok, "expected a number token");
  }
  return tok->val;
}
static bool equal(Token *tok, char *s) {
  return strlen(s) == tok->len && !strncmp(tok->loc, s, tok->len);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of argument\n", argv[0]);
  }

  Token *tok = tokenize(argv[1]);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  printf("main:\n");

  printf("\tmov rax, %ld\n", number(tok));
  tok = tok->next;
  while (tok->kind != TK_EOF) {
    if (equal(tok, "+")) {
      tok = tok->next;
      printf("\tadd rax, %ld\n", number(tok));
      tok = tok->next;
      continue;
    }
    if (equal(tok, "-")) {
      tok = tok->next;
      printf("\tsub rax, %ld\n", number(tok));
      tok = tok->next;
      continue;
    }
    error_tok(tok, "something wrong in tokenize");
  }

  printf("\tret\n");
  return 0;
}
