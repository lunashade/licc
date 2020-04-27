#include "lcc.h"

static Function *funcdef(Token **rest, Token *tok);

static Type *typespec(Token **rest, Token *tok);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Type *type_suffix(Token **rest, Token *tok, Type *ty);

static Node *compound_stmt(Token **rest, Token *tok);
static Node *declaration(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Node *func_args(Token **rest, Token *tok);

//
// Token utility
//
static long get_number(Token *tok) {
    if (tok->kind != TK_NUM) {
        error_tok(tok, "expected a number token");
    }
    return tok->val;
}

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expected a identifier token");
    }
    return strndup(tok->loc, tok->len);
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
static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_unary_node(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

static Node *new_binary_node(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_number_node(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_var_node(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

// pointer arithmetics support
static Node *new_add_node(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);
    // num+num
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary_node(ND_ADD, lhs, rhs, tok);

    // ptr+ptr -> invalid
    if (is_pointing(lhs->ty) && is_pointing(rhs->ty))
        error_tok(tok, "invalid operands: ptr+ptr");

    // canonicalize num+ptr -> ptr + num
    if (!is_pointing(lhs->ty) && is_pointing(rhs->ty)) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    Node *size = new_number_node(lhs->ty->base->size, tok);
    rhs = new_binary_node(ND_MUL, rhs, size, tok);
    return new_binary_node(ND_ADD, lhs, rhs, tok);
}
static Node *new_sub_node(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);
    // num+num
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary_node(ND_SUB, lhs, rhs, tok);

    // ptr-ptr : how many elements are between the two
    if (is_pointing(lhs->ty) && is_pointing(rhs->ty)) {
        Node *size = new_number_node(lhs->ty->base->size, tok);
        Node *node = new_binary_node(ND_SUB, lhs, rhs, tok);
        return new_binary_node(ND_DIV, node, size, tok);
    }
    // ptr - num
    if (is_pointing(lhs->ty) && is_integer(rhs->ty)) {
        Node *size = new_number_node(lhs->ty->base->size, tok);
        rhs = new_binary_node(ND_MUL, rhs, size, tok);
        return new_binary_node(ND_SUB, lhs, rhs, tok);
    }

    // num-ptr -> invalid
    error_tok(tok, "invalid operands: num-ptr");
}

//
// Var utility
//

Var *locals;

static Var *find_lvar(Token *tok) {
    for (Var *lv = locals; lv; lv = lv->next)
        if (strlen(lv->name) == tok->len &&
            !strncmp(tok->loc, lv->name, tok->len))
            return lv;

    return NULL;
}

static Var *new_lvar(char *name, Type *ty) {
    Var *lvar = calloc(1, sizeof(Var));
    lvar->next = locals;
    lvar->name = name;
    lvar->ty = ty;
    locals = lvar;
    return lvar;
}

//
// Parser
//

// program = funcdef*
Function *parse(Token *tok) {
    Function head = {};
    Function *cur = &head;
    while (tok->kind != TK_EOF) {
        cur = cur->next = funcdef(&tok, tok);
    }
    return head.next;
}

// funcdef = typespec declarator "{" compound_stmt
static Function *funcdef(Token **rest, Token *tok) {
    locals = NULL;

    Type *ty = typespec(&tok, tok);
    ty = declarator(&tok, tok, ty);

    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(ty->name);
    for (Type *t = ty->params; t; t = t->next) {
        new_lvar(get_ident(t->name), t);
    }
    fn->params = locals;

    tok = skip(tok, "{");
    fn->node = compound_stmt(&tok, tok)->body;
    fn->locals = locals;
    *rest = tok;
    return fn;
}

// compound_stmt = ( declaration | stmt )* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")) {
        if (equal(tok, "int")) {
            cur = cur->next = declaration(&tok, tok);
        } else {
            cur = cur->next = stmt(&tok, tok);
        }
    }
    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    add_type(node);
    *rest = skip(tok, "}");
    return node;
}

// declaration = typespec declarator ("=" expr)? ("," declarator ("=" expr)?)*
// ";"
static Node *declaration(Token **rest, Token *tok) {
    Type *basety = typespec(&tok, tok);

    Node head = {};
    Node *cur = &head;
    int cnt = 0;
    while (!equal(tok, ";")) {
        if (cnt++ > 0)
            tok = skip(tok, ",");

        Type *ty = declarator(&tok, tok, basety);
        Var *var = new_lvar(get_ident(ty->name), ty);

        // ("=" expr)?
        if (!equal(tok, "="))
            continue;

        Node *lhs = new_var_node(var, ty->name);
        Node *rhs = expr(&tok, tok->next);
        Node *node = new_binary_node(ND_ASSIGN, lhs, rhs, tok);
        cur = cur->next = new_unary_node(ND_EXPR_STMT, node, tok);
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = skip(tok, ";");
    return node;
}

// typespec = "int"
static Type *typespec(Token **rest, Token *tok) {
    *rest = skip(tok, "int");
    return ty_int;
}

// declarator = ("*")* ident type-suffix?
static Type *declarator(Token **rest, Token *tok, Type *ty) {
    for (Token *t = tok; equal(t, "*"); t = t->next) {
        ty = pointer_to(ty);
        tok = t->next;
    }
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expected variable name");
    }
    ty = type_suffix(rest, tok->next, ty);
    ty->name = tok;
    return ty;
}

// type-suffix = ("(" func-params)?
// func-params = param ("," param)* ")"
// param       = typespec declarator
static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "(")) {
        tok = skip(tok, "(");

        Type head = {};
        Type *cur = &head;
        int cnt = 0;
        while (!equal(tok, ")")) {
            if (cnt++ > 0)
                tok = skip(tok, ",");
            Type *basety = typespec(&tok, tok);
            Type *ty = declarator(&tok, tok, basety);
            cur = cur->next = copy_type(ty);
        }
        ty = func_type(ty);
        ty->params = head.next;
        *rest = skip(tok, ")");
        return ty;
    }
    *rest = tok;
    return ty;
}

// stmt = expr ";"
//      | "return" expr ";"
//      | "if" "(" expr ")" stmt ( "else" stmt )?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr?; expr?; expr?; ")" stmt
//      | "{" compound_stmt
static Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "{")) {
        return compound_stmt(rest, tok->next);
    }
    if (equal(tok, "return")) {
        Node *node = new_node(ND_RETURN, tok);
        node->lhs = expr(&tok, tok->next);
        *rest = skip(tok, ";");
        return node;
    }
    if (equal(tok, "while")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);

        *rest = tok;
        return node;
    }
    if (equal(tok, "for")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");

        if (!equal(tok, ";")) {
            node->init = expr_stmt(&tok, tok);
        }
        tok = skip(tok, ";");

        if (!equal(tok, ";")) {
            node->cond = expr(&tok, tok);
        }
        tok = skip(tok, ";");

        if (!equal(tok, ")")) {
            node->inc = expr_stmt(&tok, tok);
        }
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);

        *rest = tok;
        return node;
    }
    if (equal(tok, "if")) {
        Node *node = new_node(ND_IF, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else")) {
            node->els = stmt(&tok, tok->next);
        }
        *rest = tok;
        return node;
    }
    Node *node = expr_stmt(&tok, tok);
    *rest = skip(tok, ";");
    return node;
}

// expr-stmt = expr
static Node *expr_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_EXPR_STMT, tok);
    node->lhs = expr(rest, tok);
    return node;
}

// expr = assign
static Node *expr(Token **rest, Token *tok) { return assign(rest, tok); }

// assign = equality ( "=" assign )*
static Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    for (;;) {
        if (equal(tok, "=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node, rhs, op);
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
            Token *op = tok;
            Node *rhs = relational(&tok, tok->next);
            node = new_binary_node(ND_EQ, node, rhs, op);
            continue;
        }
        if (equal(tok, "!=")) {
            Token *op = tok;
            Node *rhs = relational(&tok, tok->next);
            node = new_binary_node(ND_NE, node, rhs, op);
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
            Token *op = tok;
            Node *rhs = add(&tok, tok->next);
            node = new_binary_node(ND_LT, node, rhs, op);
            continue;
        }
        if (equal(tok, "<=")) {
            Token *op = tok;
            Node *rhs = add(&tok, tok->next);
            node = new_binary_node(ND_LE, node, rhs, op);
            continue;
        }
        if (equal(tok, ">")) {
            Token *op = tok;
            Node *rhs = add(&tok, tok->next);
            node = new_binary_node(ND_LT, rhs, node, op);
            continue;
        }
        if (equal(tok, ">=")) {
            Token *op = tok;
            Node *rhs = add(&tok, tok->next);
            node = new_binary_node(ND_LE, rhs, node, op);
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
        Token *op = tok;
        if (equal(tok, "+")) {
            Node *rhs = mul(&tok, tok->next);
            node = new_add_node(node, rhs, op);
            continue;
        }
        if (equal(tok, "-")) {
            Node *rhs = mul(&tok, tok->next);
            node = new_sub_node(node, rhs, op);
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
            Token *op = tok;
            Node *rhs = unary(&tok, tok->next);
            node = new_binary_node(ND_MUL, node, rhs, op);
            continue;
        }
        if (equal(tok, "/")) {
            Token *op = tok;
            Node *rhs = unary(&tok, tok->next);
            node = new_binary_node(ND_DIV, node, rhs, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// unary = ( "+" | "-" | "*" | "&" | "sizeof" ) unary | primary
static Node *unary(Token **rest, Token *tok) {
    Token *start = tok;
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }
    if (equal(tok, "-")) {
        return new_binary_node(ND_SUB, new_number_node(0, NULL),
                               unary(rest, tok->next), start);
    }
    if (equal(tok, "*")) {
        return new_unary_node(ND_DEREF, unary(rest, tok->next), start);
    }
    if (equal(tok, "&")) {
        return new_unary_node(ND_ADDR, unary(rest, tok->next), start);
    }
    if (equal(tok, "sizeof")) {
        Node *node = unary(rest, tok->next);
        add_type(node);
        return new_number_node(node->ty->size, start);
    }
    return primary(rest, tok);
}

// primary = "(" expr ")" | num | ident func-args?
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    if (tok->kind == TK_IDENT) {
        if (equal(tok->next, "(")) {
            Node *node = new_node(ND_FUNCALL, tok);
            node->funcname = get_ident(tok);
            node->args = func_args(rest, tok->next);
            return node;
        }
        Var *lvar = find_lvar(tok);
        if (!lvar) {
            error_tok(tok, "undeclared variable: %s", get_ident(tok));
        }
        *rest = tok->next;
        return new_var_node(lvar, tok);
    }
    Node *node = new_number_node(get_number(tok), tok);
    *rest = tok->next;
    return node;
}

// func-args = "(" (assign ("," assign)*)? ")"
static Node *func_args(Token **rest, Token *tok) {
    Node head = {};
    Node *cur = &head;

    tok = skip(tok, "(");
    int cnt = 0;
    while (!equal(tok, ")")) {
        if (cnt++ > 0)
            tok = skip(tok, ",");
        cur = cur->next = assign(&tok, tok);
    }
    *rest = skip(tok, ")");
    return head.next;
}
