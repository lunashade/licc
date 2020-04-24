#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//
typedef enum {
  TK_RESERVED, // Keywords, punctuators
  TK_NUM,      // numbers
  TK_IDENT,    // identifier
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

Token *new_token(Token *cur, TokenKind kind, char *str, int len);
Token *tokenize(char *p);
Token *skip(Token *tok, char *s);

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);

//
// Parser
//

typedef struct LVar LVar;
struct LVar {
  LVar *next; // next LVar
  char *name; // name string
  int offset; // offset from rbp
};

typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_EXPR_STMT, // Expession Statement
  ND_RETURN,    // return statement
  ND_NUM,       // Integer
  ND_VAR,       // variable
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *next; // next statement
  Node *lhs;  // binary node left-hand side
  Node *rhs;  // binary node right-hand side
  LVar *var;  // ND_VAR, local variable
  long val;   // ND_NUM, value
};

typedef struct Function Function;
struct Function {
  Node *node;
  LVar *locals;  // linked list of locals
  int stacksize; // local variable stack size
};

Function *parse(Token *tok);

//
// Codegen
//

void codegen(Function *prog);
