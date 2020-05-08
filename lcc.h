#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct Type Type;
typedef struct Member Member;
typedef struct Relocation Relocation;

//
// Tokenizer
//
typedef enum {
    TK_RESERVED, // Keywords, punctuators
    TK_STR,      // string literal
    TK_NUM,      // integer literal
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
    char *contents;   // string literal including \0
    int contents_len; // string literal length
    int lineno;
};

Token *tokenize(char *filename, char *p);
bool equal(Token *tok, char *s);
Token *skip(Token *tok, char *s);

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
void warn_tok(Token *tok, char *fmt, ...);

//
// Parser
//

typedef struct DeclContext DeclContext;
struct DeclContext {
    bool type_def;
    bool is_extern;
    bool is_static;
    int align;
};

typedef struct Var Var;
struct Var {
    Var *next;     // next LVar
    char *name;    // name string
    Type *ty;      // type
    int offset;    // offset from rbp
    int align;     // alignment
    bool is_local; // local or not

    char *contents;    // global initialization
    int contents_len;  // length of contents
    Relocation *reloc; // relocation
    bool ascii;        // can be converted ascii string
};

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    char *name;
    int depth;
    Var *var;
    Type *type_def;
    Type *enum_ty;
    int enum_val;
};

typedef enum {
    TAG_STRUCT,
    TAG_UNION,
    TAG_ENUM,
} TagKind;

typedef struct TagScope TagScope;
struct TagScope {
    TagKind kind;
    Type *ty;
    TagScope *next;
    int depth;
    char *name;
};

typedef enum {
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_MOD,       // %
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_BITNOT,    // ~
    ND_NOT,       // !
    ND_OR,        // |
    ND_AND,       // &
    ND_XOR,       // ^
    ND_SHL,       // <<
    ND_SHR,       // >>
    ND_CAST,      // (type-name)
    ND_LOGAND,    // &&
    ND_LOGOR,     // ||
    ND_COMMA,     // ,
    ND_ASSIGN,    // =
    ND_COND,      // cond ? then : else
    ND_MEMBER,    // . (struct member)
    ND_EXPR_STMT, // Expession Statement
    ND_STMT_EXPR, // GNU Statement Expression
    ND_BLOCK,     // block statement
    ND_SWITCH,    // switch
    ND_CASE,      // case
    ND_BREAK,     // break
    ND_CONTINUE,  // continue
    ND_GOTO,      // goto
    ND_LABEL,     // Labeled statement
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

    // ND_FUNCALL
    char *funcname; // function call
    Node *args;     // function args
    Type *func_ty;  // function type info

    Node *cond; // condition
    Node *then; // then
    Node *els;  // else
    Node *init; // for init
    Node *inc;  // for increment

    Node *lhs; // binary node left-hand side
    Node *rhs; // binary node right-hand side

    Var *var;        // ND_VAR, local variable
    long val;        // ND_NUM, value
    Member *member;  // ND_MEMBER, struct member
    char *labelname; // label

    // ND_SWITCH, ND_CASE
    Node *case_next;    // switch-case-list
    Node *default_case; // switch-default
    int case_label;     // codegen label for case node
    int case_end_label; // codegen label for case node

    Token *tok; // Debug info: representative token
};

typedef struct Function Function;
struct Function {
    Function *next; // next function
    char *name;     // function name
    Var *params;

    Node *node;    // body statements
    Var *locals;   // linked list of locals
    int stacksize; // local variable stack size

    bool is_static; // file scope
};

typedef struct Program Program;
struct Program {
    Var *globals;
    Function *fns;
};

typedef struct Designator Designator;
struct Designator {
    Designator *parent;
    int index;
    Member *member;
    Var *var;
};
typedef struct Initializer Initializer;
struct Initializer {
    Type *ty;
    int len;
    Node *expr;
    Initializer **children;
};

struct Relocation {
    Relocation *next;
    int offset;
    char *label;
    long addend;
};

Node *new_cast(Node *node, Type *ty);
Program *parse(Token *tok);

//
// typing.c
//

int align_to(int n, int align);
bool is_typename(Token *tok);

typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
    TY_STRUCT,
    TY_ENUM,
} TypeKind;

struct Type {
    TypeKind kind;
    int size;
    int align;
    Type *base;    // TY_PTR or TY_ARRAY pointer_to
    int array_len; // TY_ARRAY
    // TY_FUNC
    Type *return_ty; // function return type
    Token *name;     // function name
    Type *params;    // params
    Type *next;      // next parameter
    // TY_STRUCT
    Member *member;
    // ARRAY or STRUCT
    bool is_incomplete;
};

struct Member {
    Member *next;
    Type *ty;    // type
    Token *name; // member name
    int offset;  // offset from base
    int size;    // size of this member
    int align;   // alignment
};
Member *new_member(Type *ty);

extern Type *ty_void;
extern Type *ty_bool;

extern Type *ty_int;
extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_long;

bool is_integer(Type *ty);
bool is_pointing(Type *ty);
int size_of(Type *ty);

void add_type(Node *node);

Type *new_type(TypeKind kind, int size, int align);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
Type *func_type(Type *return_ty);
Type *struct_type(void);
Type *enum_type(void);
Type *copy_type(Type *ty);

//
// Codegen
//

void codegen(Program *prog);
