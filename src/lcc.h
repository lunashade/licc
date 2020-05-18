#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern bool opt_E;
extern bool opt_fpic;
extern char **include_paths;
extern char *input_path;
extern FILE *output_file;

typedef struct Type Type;
typedef struct Member Member;
typedef struct Relocation Relocation;

//
// Tokenizer
//
// hideset for preprocess
typedef struct Hideset Hideset;
struct Hideset {
    Hideset *next;
    char *name;
};
extern char *current_filename;

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
    long val;    // TK_NUM
    double fval; // TK_NUM
    Type *ty;    // TK_NUM

    char *loc;
    int len;

    char *contents;   // string literal including \0
    int contents_len; // string literal length

    int lineno;     // line number
    int fileno;     // file number
    char *filename; // filename
    bool at_bol;    // beginning of line
    bool has_space; // has space before this
    Hideset *hideset;
};

Token *tokenize(char *filename, int fileno, char *p);
Token *tokenize_file(char *filename);
bool is_keyword(Token *tok);
bool equal(Token *tok, char *s);
Token *skip(Token *tok, char *s);
bool consume(Token **rest, Token *tok, char *s);
char *get_ident(Token *tok);

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
void warn_tok(Token *tok, char *fmt, ...);

//
// Preprocessor
//
void init_macros(void);
Token *read_file(char *);
Token *preprocess(Token *);
Token *preprocess_file(char *);

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
    Var *next;      // next LVar
    char *name;     // name string
    Type *ty;       // type
    int offset;     // offset from rbp
    int align;      // alignment
    bool is_local;  // local or not
    bool is_static; // file-scope

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
    ND_NOP_EXPR,  // nop
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
    ND_DO,        // do statement
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
    Var **args;     // function args
    int nargs;      // number of args
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
    double fval;     // ND_NUM, value
    Member *member;  // ND_MEMBER, struct member
    char *labelname; // label

    // ND_SWITCH, ND_CASE
    Node *case_next;    // switch-case-list
    Node *default_case; // switch-default
    int case_label;     // codegen label for case node
    int case_end_label; // codegen label for case node
    // ND_ASSIGN
    bool is_init;

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

    bool is_static;   // file scope
    bool is_variadic; // variadic
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
long const_expr(Token **rest, Token *tok);
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
    TY_ENUM,
    TY_FLOAT,
    TY_DOUBLE,
    TY_FUNC,
    TY_ARRAY,
    TY_STRUCT,
} TypeKind;

struct Type {
    TypeKind kind;
    int size;
    int align;
    // NUMBER
    bool is_unsigned;
    // TY_PTR / TY_ARRAY
    Type *base;    // pointer_to
    int array_len; // TY_ARRAY
    // TY_FUNC
    Type *return_ty;  // function return type
    Token *name;      // var name (nullable)
    Token *name_pos;  // var name should be here
    Type *params;     // params
    Type *next;       // next parameter
    bool is_variadic; // variadic function
    // TY_STRUCT
    Member *member;
    // ARRAY or STRUCT
    bool is_incomplete;
    // qualifier
    bool is_const;
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
extern Type *ty_uint;
extern Type *ty_char;
extern Type *ty_uchar;
extern Type *ty_short;
extern Type *ty_ushort;
extern Type *ty_long;
extern Type *ty_ulong;

extern Type *ty_float;
extern Type *ty_double;

bool is_integer(Type *ty);
bool is_flonum(Type *ty);
bool is_numeric(Type *ty);
bool is_scalar(Type *ty);
bool is_pointing(Type *ty);
int size_of(Type *ty);

void add_type(Node *node);

Type *new_type(TypeKind kind, int size, int align);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
Type *func_type(Type *return_ty);
Type *struct_type(void);
Type *enum_type(void);

//
// Codegen
//

void codegen(Program *prog);

//
// Debug
//
void print_tokens(Token *);
