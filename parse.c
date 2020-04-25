#include "lcc.h"

// stmt = expr ";" | "return" expr ";"
// expr = assign
// assign = equality ("=" assign)
// equality = relational ( "==" relational | "!=" relational )*
// relational = add ( "<" add | ">" add | "<=" add | ">=" add )*
// add = mul ( "+" mul | "-" mul )*
// mul = unary ( "*" unary | "/" unary )*
// unary = ( "+" | "-" ) unary | primary
// primary = "(" expr ")" | num | ident
static Node *stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

//
// Token utility
//
static long get_number(Token *tok) {
  if (tok->kind != TK_NUM) {
    error_tok(tok, "expected a number token");
  }
  return tok->val;
}

bool equal(Token *tok, char *s) {
  return strlen(s) == tok->len && !strncmp(tok->loc, s, tok->len);
}

Token *skip(Token *tok, char *s) {
  if (!equal(tok, s)) {
    error_tok(tok, "expected token %s", s);
  }
  return tok->next;
}

//
// Node utility
//
static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_unary_node(NodeKind kind, Node *lhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
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

static Node *new_var_node(LVar *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

//
// LVar utility
//

LVar *locals;

static LVar *find_lvar(Token *tok) {
  for (LVar *lv = locals; lv; lv = lv->next)
    if (strlen(lv->name) == tok->len && !strncmp(tok->loc, lv->name, tok->len))
      return lv;

  return NULL;
}

static LVar *new_lvar(char *name) {
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->next = locals;
  lvar->name = name;
  locals = lvar;
  return lvar;
}

//
// Parser
//

Function *parse(Token *tok) {
  Node head = {};
  Node *cur = &head;
  while (tok->kind != TK_EOF) {
    cur = cur->next = stmt(&tok, tok);
  }
  Function *prog = calloc(1, sizeof(Function));
  prog->node = head.next;
  prog->locals = locals;
  return prog;
}

// stmt = expr ";" | "return" expr ";" | "if" "(" expr ")" stmt ( "else" stmt )?
static Node *stmt(Token **rest, Token *tok) {
  if (equal(tok, "return")) {
    Node *node = new_unary_node(ND_RETURN, expr(&tok, tok->next));
    *rest = skip(tok, ";");
    return node;
  }
  if (equal(tok, "if")) {
    Node *node = new_node(ND_IF);
    tok = skip(tok->next, "(");
    node->cond = expr(&tok, tok);
    tok = skip(tok, ")");
    node->then = stmt(&tok, tok);
    *rest = tok;
    return node;
  }
  Node *node = new_unary_node(ND_EXPR_STMT, expr(&tok, tok));
  *rest = skip(tok, ";");
  return node;
}

// expr = assign
static Node *expr(Token **rest, Token *tok) { return assign(rest, tok); }

// assign = equality ( "=" assign )*
static Node *assign(Token **rest, Token *tok) {
  Node *node = equality(&tok, tok);
  for (;;) {
    if (equal(tok, "=")) {
      Node *rhs = assign(&tok, tok->next);
      node = new_binary_node(ND_ASSIGN, node, rhs);
    }
    *rest = tok;
    return node;
  }
}

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

// primary = "(" expr ")" | num | ident
static Node *primary(Token **rest, Token *tok) {
  if (equal(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }
  if (tok->kind == TK_IDENT) {
    LVar *lvar = find_lvar(tok);
    if (!lvar) {
      lvar = new_lvar(strndup(tok->loc, tok->len));
    }
    *rest = tok->next;
    return new_var_node(lvar);
  }
  Node *node = new_number_node(get_number(tok));
  *rest = tok->next;
  return node;
}
