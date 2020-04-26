#include "lcc.h"

static Node *compound_stmt(Token **rest, Token *tok);
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

static Node *new_var_node(LVar *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

//
// LVar utility
//

LVar *locals;

static LVar *find_lvar(Token *tok) {
    for (LVar *lv = locals; lv; lv = lv->next)
        if (strlen(lv->name) == tok->len &&
            !strncmp(tok->loc, lv->name, tok->len))
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
    tok = skip(tok, "{");
    Function *prog = calloc(1, sizeof(Function));
    prog->node = compound_stmt(&tok, tok)->body;
    prog->locals = locals;
    assert(tok->kind == TK_EOF);
    return prog;
}

// compound_stmt = stmt* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")) {
        cur = cur->next = stmt(&tok, tok);
    }
    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = skip(tok, "}");
    return node;
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
        if (equal(tok, "+")) {
            Token *op = tok;
            Node *rhs = mul(&tok, tok->next);
            node = new_binary_node(ND_ADD, node, rhs, op);
            continue;
        }
        if (equal(tok, "-")) {
            Token *op = tok;
            Node *rhs = mul(&tok, tok->next);
            node = new_binary_node(ND_SUB, node, rhs, op);
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

// unary = ( "+" | "-" | "*" | "&" ) unary | primary
static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }
    if (equal(tok, "-")) {
        Token *op = tok;
        return new_binary_node(ND_SUB, new_number_node(0, NULL),
                               unary(rest, tok->next), op);
    }
    if (equal(tok, "*")) {
        Token *op = tok;
        return new_unary_node(ND_DEREF, unary(rest, tok->next), op);
    }
    if (equal(tok, "&")) {
        Token *op = tok;
        return new_unary_node(ND_ADDR, unary(rest, tok->next), op);
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
        return new_var_node(lvar, tok);
    }
    Node *node = new_number_node(get_number(tok), tok);
    *rest = tok->next;
    return node;
}
