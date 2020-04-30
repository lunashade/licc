#include "lcc.h"

static Function *funcdef(Token **rest, Token *tok);

static Type *typespec(Token **rest, Token *tok);
static Type *struct_spec(Token **rest, Token *tok);
static Member *struct_decl(Token **rest, Token *tok);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Type *type_suffix(Token **rest, Token *tok, Type *ty);
static Type *func_params(Token **rest, Token *tok, Type *ty);

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
static Node *postfix(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Node *func_args(Token **rest, Token *tok);

static bool is_typename(Token *tok);

//
// Token utility
//
static long get_number(Token *tok) {
    if (tok->kind != TK_NUM) {
        error_tok(tok, "parse: number: expected a number token");
    }
    return tok->val;
}

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "parse: ident: expected a identifier token");
    }
    return strndup(tok->loc, tok->len);
}

bool equal(Token *tok, char *s) {
    return strlen(s) == tok->len && !strncmp(tok->loc, s, tok->len);
}

Token *skip(Token *tok, char *s) {
    if (!equal(tok, s)) {
        error_tok(tok, "parse: expected token %s", s);
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
        error_tok(tok, "parse: add: invalid operands: ptr+ptr");

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
    error_tok(tok, "parse: sub: invalid operands: num-ptr");
}

static Member *get_struct_member(Type *ty, Token *tok) {
    for (Member *m = ty->member; m; m = m->next) {
        if (m->name->len == tok->len &&
            !strncmp(m->name->loc, tok->loc, tok->len))
            return m;
    }
    error_tok(tok, "parse: member: no such member in struct");
}

static Node *struct_ref(Node *lhs, Token *tok) {
    add_type(lhs);
    if (lhs->ty->kind != TY_STRUCT) {
        error_tok(lhs->tok, "parse: member: member access for non-struct");
    }
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "parse: member: member should be a identifier token");
    }
    Node *node = new_unary_node(ND_MEMBER, lhs, tok);
    node->member = get_struct_member(lhs->ty, tok);
    return node;
}

//
// Var utility
//

static Var *globals;
static Var *locals;

static VarScope *var_scope;
static TagScope *tag_scope;
static int scope_depth;

static void enter_scope(void) { scope_depth++; }
static void leave_scope(void) {
    scope_depth--;
    while (tag_scope && tag_scope->depth > scope_depth)
        tag_scope = tag_scope->next;
    while (var_scope && var_scope->depth > scope_depth)
        var_scope = var_scope->next;
}

static void push_scope(Var *var) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->depth = scope_depth;
    sc->next = var_scope;
    sc->var = var;
    sc->name = var->name;
    var_scope = sc;
}
static void push_tag_scope(Token *tok, Type *ty, TagKind kind) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->depth = scope_depth;
    sc->next = tag_scope;
    sc->kind = kind;
    sc->name = strndup(tok->loc, tok->len);
    sc->ty = ty;
    tag_scope = sc;
}

static Var *find_var(Token *tok) {
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (strlen(sc->name) == tok->len &&
            !strncmp(tok->loc, sc->name, tok->len))
            return sc->var;
    }
    return NULL;
}
static TagScope *find_tag(Token *tok) {
    for (TagScope *sc = tag_scope; sc; sc = sc->next) {
        if (strlen(sc->name) == tok->len &&
            !strncmp(tok->loc, sc->name, tok->len))
            return sc;
    }
    return NULL;
}

static Var *new_lvar(char *name, Type *ty) {
    Var *lvar = calloc(1, sizeof(Var));
    lvar->next = locals;
    lvar->name = name;
    lvar->ty = ty;
    lvar->is_local = true;
    push_scope(lvar);
    locals = lvar;
    return lvar;
}

static Var *new_gvar(char *name, Type *ty) {
    Var *gvar = calloc(1, sizeof(Var));
    gvar->next = globals;
    gvar->name = name;
    gvar->ty = ty;
    gvar->is_local = false;
    push_scope(gvar);
    globals = gvar;
    return gvar;
}
static char *new_label(void) {
    static int cnt = 0;
    char *buf = malloc(20);
    sprintf(buf, ".L.data.%d", cnt++);
    return buf;
}

static Var *new_string_literal(char *p, int len) {
    Type *ty = array_of(ty_char, len);
    Var *var = new_gvar(new_label(), ty);
    var->contents = p;
    var->contents_len = len;
    return var;
}

//
// Parser
//

// program = ( global-var |  funcdef )*
Program *parse(Token *tok) {
    globals = NULL;
    Function head = {};
    Function *cur = &head;

    while (tok->kind != TK_EOF) {
        Token *start = tok;
        Type *basety = typespec(&tok, tok);
        Type *ty = declarator(&tok, tok, basety);

        if (ty->kind == TY_FUNC) {
            cur = cur->next = funcdef(&tok, start);
            continue;
        }
        for (;;) {
            new_gvar(get_ident(ty->name), ty);
            if (equal(tok, ";")) {
                tok = skip(tok, ";");
                break;
            }
            tok = skip(tok, ",");
            ty = declarator(&tok, tok, basety);
        }
    }

    Program *prog = calloc(1, sizeof(Program));
    prog->globals = globals;
    prog->fns = head.next;
    return prog;
}

// funcdef = typespec declarator "{" compound_stmt
static Function *funcdef(Token **rest, Token *tok) {
    locals = NULL;

    Type *ty = typespec(&tok, tok);
    ty = declarator(&tok, tok, ty);

    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(ty->name);
    enter_scope();
    for (Type *t = ty->params; t; t = t->next) {
        new_lvar(get_ident(t->name), t);
    }
    fn->params = locals;

    tok = skip(tok, "{");
    fn->node = compound_stmt(&tok, tok)->body;
    fn->locals = locals;
    *rest = tok;
    leave_scope();
    return fn;
}

// compound_stmt = ( declaration | stmt )* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
    Node head = {};
    Node *cur = &head;
    enter_scope();
    while (!equal(tok, "}")) {
        if (is_typename(tok)) {
            cur = cur->next = declaration(&tok, tok);
        } else {
            cur = cur->next = stmt(&tok, tok);
        }
    }
    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    add_type(node);
    leave_scope();
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

static bool is_typename(Token *tok) {
    return equal(tok, "int") || equal(tok, "char") || equal(tok, "struct") ||
           equal(tok, "union");
}

// typespec = "int" | "char" | struct-union-spec
static Type *typespec(Token **rest, Token *tok) {
    if (equal(tok, "struct") || equal(tok, "union")) {
        Type *ty = struct_spec(&tok, tok);
        *rest = tok;
        return ty;
    }
    if (equal(tok, "char")) {
        *rest = skip(tok, "char");
        return ty_char;
    }
    *rest = skip(tok, "int");
    return ty_int;
}

// struct-union-spec = ("struct" | "union") (ident)? "{" struct-decl
static Type *struct_spec(Token **rest, Token *tok) {
    TagKind kind;
    if (equal(tok, "struct")) {
        kind = TAG_STRUCT;
        tok = skip(tok, "struct");
    } else {
        // tok == "union"
        kind = TAG_UNION;
        tok = skip(tok, "union");
    }

    Token *tag = NULL;
    if (tok->kind == TK_IDENT) {
        tag = tok;
        tok = tok->next;
    }
    if (tag && !equal(tok, "{")) {
        TagScope *sc = find_tag(tag);
        if (!sc) {
            error_tok(tag, "parse: struct-spec: unknown struct type");
        }
        if (sc->kind != kind)
            error_tok(tag, "parse: tag kind mismatch from previous declaration");
        *rest = tok;
        return sc->ty;
    }

    tok = skip(tok, "{");
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->member = struct_decl(rest, tok);

    if (kind == TAG_STRUCT) {
        int offset = 0;
        for (Member *m = ty->member; m; m = m->next) {
            offset = align_to(offset, m->ty->align);
            m->offset = offset;
            offset += m->ty->size;
            if (ty->align < m->ty->align)
                ty->align = m->ty->align;
        }
        ty->size = align_to(offset, ty->align);
    } else {
        for (Member *m = ty->member; m; m = m->next) {
            m->offset = 0;
            if (ty->size < m->ty->size)
                ty->size = m->ty->size;
            if (ty->align < m->ty->align)
                ty->align = m->ty->align;
        }
        ty->size = align_to(ty->size, ty->align);
    }

    if (tag)
        push_tag_scope(tag, ty, kind);
    return ty;
}

// struct-decl = (typespec declarator ("," declarator)* ";")* "}"
static Member *struct_decl(Token **rest, Token *tok) {
    Member head = {};
    Member *cur = &head;

    while (!equal(tok, "}")) {
        Type *basety = typespec(&tok, tok);
        int cnt = 0;

        while (!equal(tok, ";")) {
            if (cnt++ > 0) {
                tok = skip(tok, ",");
            }
            Type *ty = declarator(&tok, tok, basety);
            cur->next = new_member(ty);
            cur = cur->next;
        }
        tok = skip(tok, ";");
    }

    *rest = skip(tok, "}");
    return head.next;
}

// declarator = ("*")* ident type-suffix?
static Type *declarator(Token **rest, Token *tok, Type *ty) {
    for (Token *t = tok; equal(t, "*"); t = t->next) {
        ty = pointer_to(ty);
        tok = t->next;
    }
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "parse: declarator: expected variable name");
    }
    ty = type_suffix(rest, tok->next, ty);
    ty->name = tok;
    return ty;
}

// type-suffix = ("(" func-params)?
//             | "[" num "]" type-suffix
//             | e
static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "(")) {
        ty = func_type(ty);
        ty = func_params(rest, tok->next, ty);
        return ty;
    }
    if (equal(tok, "[")) {
        int sz = get_number(tok->next);
        tok = skip(tok->next->next, "]");
        *rest = tok;
        ty = type_suffix(rest, tok, ty);
        return array_of(ty, sz);
    }
    *rest = tok;
    return ty;
}

// func-params = param ("," param)* ")"
// param       = typespec declarator
static Type *func_params(Token **rest, Token *tok, Type *ty) {
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
    *rest = skip(tok, ")");
    ty->params = head.next;
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

// unary = ( "+" | "-" | "*" | "&" | "sizeof" ) unary | postfix
static Node *unary(Token **rest, Token *tok) {
    Token *start = tok;
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }
    if (equal(tok, "-")) {
        return new_binary_node(ND_SUB, new_number_node(0, tok),
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
    return postfix(rest, tok);
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident)*
static Node *postfix(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);
    for (;;) {
        if (equal(tok, "[")) {
            Token *op = tok;
            Node *ex = expr(&tok, tok->next);
            node = new_add_node(node, ex, op);
            node = new_unary_node(ND_DEREF, node, op);
            tok = skip(tok, "]");
            continue;
        }
        if (equal(tok, ".")) {
            Token *op = tok;
            node = struct_ref(node, tok->next);
            tok = tok->next->next;
            continue;
        }
        if (equal(tok, "->")) {
            Token *op = tok;
            node = new_unary_node(ND_DEREF, node, op);
            node = struct_ref(node, tok->next);
            tok = tok->next->next;
            continue;
        }
        *rest = tok;
        return node;
    }
}

// primary = "(" "{" compound-stmt ")"
//         | "(" expr ")"
//         | num | str | ident func-args?
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(") && equal(tok->next, "{")) {
        Node *node = new_node(ND_STMT_EXPR, tok);
        node->body = compound_stmt(&tok, tok->next->next)->body;
        *rest = skip(tok, ")");

        Node *cur = node->body;
        while (cur->next)
            cur = cur->next;
        if (cur->kind != ND_EXPR_STMT)
            error_tok(cur->tok,
                      "parse: stmt-expr: statement expression returning void is not supported");
        return node;
    }
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
        Var *lvar = find_var(tok);
        if (!lvar) {
            error_tok(tok, "parse: var: undeclared variable: %s", get_ident(tok));
        }
        *rest = tok->next;
        return new_var_node(lvar, tok);
    }
    if (tok->kind == TK_STR) {
        Var *var = new_string_literal(tok->contents, tok->contents_len);
        *rest = tok->next;
        return new_var_node(var, tok);
    }
    if (tok->kind != TK_NUM)
        error_tok(tok, "parse: primary: expected expression");
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
