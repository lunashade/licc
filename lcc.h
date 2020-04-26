#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

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

Token *tokenize(char *p);
bool equal(Token *tok, char *s);
Token *skip(Token *tok, char *s);

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);

//
// Parser
//

typedef struct Var Var;
struct Var {
    Var *next;  // next LVar
    char *name; // name string
    Type *ty;   // type
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
    ND_BLOCK,     // block statement
    ND_FUNCALL,   // function call
    ND_RETURN,    // return statement
    ND_IF,        // if statement
    ND_FOR,       // for statement
    ND_NUM,       // Integer
    ND_VAR,       // variable
    ND_ADDR,      // &
    ND_DEREF,     // unary *
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Type *ty; // Type

    Node *next; // next statement
    Node *body; // block body

    char *funcname; // function call
    Node *args; // function args

    Node *cond; // condition
    Node *then; // then
    Node *els;  // else
    Node *init; // for init
    Node *inc;  // for increment

    Node *lhs; // binary node left-hand side
    Node *rhs; // binary node right-hand side

    Var *var;  // ND_VAR, local variable
    long val;  // ND_NUM, value

    Token *tok; // Debug info: representative token
};

typedef struct Function Function;
struct Function {
    Node *node;
    Var *locals;   // linked list of locals
    int stacksize; // local variable stack size
};

Function *parse(Token *tok);

//
// typing.c
//

typedef enum {
    TY_INT,
    TY_PTR,
} TypeKind;

struct Type {
    TypeKind kind;
    int size;
    Type *base;
    Token *name;
};

extern Type *ty_int;
bool is_integer(Type *ty);
bool is_pointing(Type *ty);
void add_type(Node *node);
Type *pointer_to(Type *base);

//
// Codegen
//

void codegen(Function *prog);
