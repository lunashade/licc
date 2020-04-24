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

//
// Tokenizer
//

static Token *new_token(Token *cur, TokenKind kind, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}
const char *punctuator = "+-()/*<>=!";
const char *operator[] = {"<=", ">=", "==", "!="};

static bool startswith(char *p, char *q) {
  return strncmp(p, q, strlen(q)) == 0;
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

static long number(Token *tok) {
  if (tok->kind != TK_NUM) {
    error_tok(tok, "expected a number token");
  }
  return tok->val;
}
static bool equal(Token *tok, char *s) {
  return strlen(s) == tok->len && !strncmp(tok->loc, s, tok->len);
}
static Token *skip(Token *tok, char *s) {
  if (!equal(tok, s)) {
    error_tok(tok, "expected token %s", s);
  }
  return tok->next;
}

//
// Parser
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM,
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  long val;
};

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binary_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_number_node(long val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// expr = equality
// equality = relational ( "==" relational | "!=" relational )*
// relational = add ( "<" add | ">" add | "<=" add | ">=" add )*
// add = mul ( "+" mul | "-" mul )*
// mul = unary ( "*" unary | "/" unary )*
// unary = ( "+" | "-" ) unary | primary
// primary = "(" expr ")" | num
static Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

// expr = equality
static Node *expr(Token **rest, Token *tok) { return equality(rest, tok); }

// equality = relational ( "==" relational | "!=" relational )*
static Node *equality(Token **rest, Token *tok) {
  Node *node = relational(&tok, tok);
  for (;;) {
    if (equal(tok, "==")) {
      Node *rhs = relational(&tok, tok->next);
      node = new_binary_node(ND_EQ, node, rhs);
      continue;
    }
    if (equal(tok, "!=")) {
      Node *rhs = relational(&tok, tok->next);
      node = new_binary_node(ND_NE, node, rhs);
      continue;
    }
    *rest = tok;
    return node;
  }
}

// relational = add ( "<" add | ">" add | "<=" add | ">=" add )*
static Node *relational(Token **rest, Token *tok) {
  Node *node = add(&tok, tok);
  for (;;) {
    if (equal(tok, "<")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary_node(ND_LT, node, rhs);
      continue;
    }
    if (equal(tok, "<=")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary_node(ND_LE, node, rhs);
      continue;
    }
    if (equal(tok, ">")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary_node(ND_LT, rhs, node);
      continue;
    }
    if (equal(tok, ">=")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary_node(ND_LE, rhs, node);
      continue;
    }
    *rest = tok;
    return node;
  }
}

// add = mul ( "+" mul | "-" mul )*
static Node *add(Token **rest, Token *tok) {
  Node *node = mul(&tok, tok);
  for (;;) {
    if (equal(tok, "+")) {
      Node *rhs = mul(&tok, tok->next);
      node = new_binary_node(ND_ADD, node, rhs);
      continue;
    }
    if (equal(tok, "-")) {
      Node *rhs = mul(&tok, tok->next);
      node = new_binary_node(ND_SUB, node, rhs);
      continue;
    }
    *rest = tok;
    return node;
  }
}

// mul = unary ( "*" unary | "/" unary )*
static Node *mul(Token **rest, Token *tok) {
  Node *node = unary(&tok, tok);
  for (;;) {
    if (equal(tok, "*")) {
      Node *rhs = unary(&tok, tok->next);
      node = new_binary_node(ND_MUL, node, rhs);
      continue;
    }
    if (equal(tok, "/")) {
      Node *rhs = unary(&tok, tok->next);
      node = new_binary_node(ND_DIV, node, rhs);
      continue;
    }
    *rest = tok;
    return node;
  }
}

// unary = ( "+" | "-" ) unary | primary
static Node *unary(Token **rest, Token *tok) {
  if (equal(tok, "+")) {
    return unary(rest, tok->next);
  }
  if (equal(tok, "-")) {
    return new_binary_node(ND_SUB, new_number_node(0), unary(rest, tok->next));
  }
  return primary(rest, tok);
}

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok) {
  if (equal(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }
  Node *node = new_number_node(number(tok));
  *rest = tok->next;
  return node;
}

//
// Codegen
//

static char *reg(int idx) {
  static char *r[] = {"r10", "r11", "r12", "r13", "r14", "r15"};

  if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
    error("registor out of range: %d", idx);
  return r[idx];
}

static int top;

static void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    printf("\tmov %s, %ld\n", reg(top), node->val);
    top++;
    return;
  }
  gen_expr(node->lhs);
  gen_expr(node->rhs);

  // binary node
  char *rd = reg(top - 2);
  char *rs = reg(top - 1);
  top--;  // 2-pop, 1-push

  if (node->kind == ND_ADD) {
    printf("\tadd %s, %s\n", rd, rs);
    return;
  }
  if (node->kind == ND_SUB) {
    printf("\tsub %s, %s\n", rd, rs);
    return;
  }
  if (node->kind == ND_MUL) {
    printf("\timul %s, %s\n", rd, rs);
    return;
  }
  if (node->kind == ND_DIV) {
    printf("\tmov rax, %s\n", rd);
    printf("\tcqo\n");
    printf("\tidiv %s\n", rs);
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_EQ) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsete al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_NE) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsetne al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_LT) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsetl al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_LE) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsetle al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  error("invalid expression");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of argument\n", argv[0]);
  }

  Token *tok = tokenize(argv[1]);
  Node *node = expr(&tok, tok);
  if (tok->kind != TK_EOF) {
    error_tok(tok, "extra token");
  }

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  printf("main:\n");

  // save callee-saved registers
  printf("\tpush r12\n");
  printf("\tpush r13\n");
  printf("\tpush r14\n");
  printf("\tpush r15\n");

  gen_expr(node);
  printf("\tmov rax, %s\n", reg(top - 1));

  // recover callee-saved registers
  printf("\tpop r15\n");
  printf("\tpop r14\n");
  printf("\tpop r13\n");
  printf("\tpop r12\n");
  printf("\tret\n");
  return 0;
}
