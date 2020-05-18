#include "lcc.h"

static Function *funcdef(Token **rest, Token *tok);

static Type *decl_specifier(Token **rest, Token *tok, DeclContext *ctx);
static Type *enum_spec(Token **rest, Token *tok);
static Type *struct_spec(Token **rest, Token *tok);
static Member *struct_decl(Token **rest, Token *tok);
static Type *pointer(Token **rest, Token *tok, Type *ty);
static Type *typename(Token **rest, Token *tok);
static Type *abstract_declarator(Token **rest, Token *tok, Type *ty);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Type *type_suffix(Token **rest, Token *tok, Type *ty);
static Type *array_dim(Token **rest, Token *tok, Type *ty);
static Type *func_params(Token **rest, Token *tok, Type *ty);

static char *gvar_initializer(Token **rest, Token *tok, Var *var);
static Node *lvar_initializer(Token **rest, Token *tok, Var *var);
static Initializer *initializer(Token **rest, Token *tok, Type *ty);

static Node *compound_stmt(Token **rest, Token *tok);
static Node *declaration(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static long eval(Node *node);
static double eval_double(Node *node);
static long eval2(Node *node, Var **var);
long const_expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *conditional(Token **rest, Token *tok);
static Node *logical_or(Token **rest, Token *tok);
static Node *logical_and(Token **rest, Token *tok);
static Node *bit_or(Token **rest, Token *tok);
static Node *bit_xor(Token **rest, Token *tok);
static Node *bit_and(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *shift(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *cast(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *postfix(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Node *funcall(Token **rest, Token *tok, Node *fn);

//
// Token utility
//
static long get_number(Token *tok) {
    if (tok->kind != TK_NUM) {
        error_tok(tok, "parse: number: expected a number token");
    }
    return tok->val;
}

char *get_ident(Token *tok) {
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
bool consume(Token **rest, Token *tok, char *s) {
    if (equal(tok, s)) {
        *rest = skip(tok, s);
        return true;
    }
    return false;
}

// consume ending "}" or "," "}"
static bool consume_end(Token **rest, Token *tok) {
    if (equal(tok, "}")) {
        *rest = tok->next;
        return true;
    }
    if (equal(tok, ",") && equal(tok->next, "}")) {
        *rest = tok->next->next;
        return true;
    }
    *rest = tok;
    return false;
}
static Token *skip_end(Token *tok) {
    if (equal(tok, "}")) {
        return tok->next;
    }
    if (equal(tok, ",") && equal(tok->next, "}")) {
        return tok->next->next;
    }
    error_tok(tok, "parse: expected token '}'");
}
static bool is_end(Token *tok) {
    return equal(tok, "}") || (equal(tok, ",") && equal(tok->next, "}"));
}
static Token *skip_excess_element(Token *tok) {
    while (!is_end(tok)) {
        tok = skip(tok, ",");
        if (equal(tok, "{")) {
            tok = skip_excess_element(tok->next);
        } else {
            assign(&tok, tok);
        }
    }
    return tok;
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

static Node *new_number(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}
static Node *new_ulong(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    node->ty = ty_ulong;
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
    if (is_numeric(lhs->ty) && is_numeric(rhs->ty))
        return new_binary_node(ND_ADD, lhs, rhs, tok);

    // ptr+ptr -> invalid
    if (is_pointing(lhs->ty) && is_pointing(rhs->ty))
        error_tok(tok, "parse: add: invalid operands: ptr+ptr");

    // canonicalize int + ptr -> ptr + int
    if (!is_pointing(lhs->ty) && is_pointing(rhs->ty)) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + int
    if (!is_integer(rhs->ty))
        error_tok(tok, "parse: add: invalid operands");

    Node *size = new_number(lhs->ty->base->size, tok);
    rhs = new_binary_node(ND_MUL, rhs, size, tok);
    return new_binary_node(ND_ADD, lhs, rhs, tok);
}

static Node *new_sub_node(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);
    // num+num
    if (is_numeric(lhs->ty) && is_numeric(rhs->ty))
        return new_binary_node(ND_SUB, lhs, rhs, tok);

    // ptr-ptr : how many elements are between the two
    if (is_pointing(lhs->ty) && is_pointing(rhs->ty)) {
        Node *size = new_number(lhs->ty->base->size, tok);
        Node *node = new_binary_node(ND_SUB, lhs, rhs, tok);
        return new_binary_node(ND_DIV, node, size, tok);
    }
    // ptr - int
    if (is_pointing(lhs->ty) && is_integer(rhs->ty)) {
        Node *size = new_number(lhs->ty->base->size, tok);
        rhs = new_binary_node(ND_MUL, rhs, size, tok);
        return new_binary_node(ND_SUB, lhs, rhs, tok);
    }

    // num-ptr -> invalid
    error_tok(tok, "parse: sub: invalid operands: num-ptr");
    // won't reach
    return NULL;
}

static Member *get_struct_member(Type *ty, Token *tok) {
    for (Member *m = ty->member; m; m = m->next) {
        if (m->name->len == tok->len &&
            !strncmp(m->name->loc, tok->loc, tok->len))
            return m;
    }
    error_tok(tok, "parse: member: no such member in struct");
    // won't reach
    return NULL;
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

Node *new_cast(Node *expr, Type *ty) {
    add_type(expr);

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_CAST;
    node->lhs = expr;
    node->tok = expr->tok;
    node->ty = copy_type(ty);
    return node;
}

//
// Var utility
//

static Var *globals;
static Var *locals;

static VarScope *var_scope;  // variable scope stack top
static TagScope *tag_scope;  // tag scope stack top
static Node *current_switch; // switch statement AST node currently parsing
static Var *current_fn;      // function currently parsing
static int scope_depth;

static void enter_scope(void) { scope_depth++; }
static void leave_scope(void) {
    scope_depth--;
    while (tag_scope && tag_scope->depth > scope_depth)
        tag_scope = tag_scope->next;
    while (var_scope && var_scope->depth > scope_depth)
        var_scope = var_scope->next;
}

static VarScope *push_scope(char *name) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->depth = scope_depth;
    sc->next = var_scope;
    sc->name = name;
    var_scope = sc;
    return sc;
}

static TagScope *push_tag_scope(Token *tok, Type *ty, TagKind kind) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->depth = scope_depth;
    sc->next = tag_scope;
    sc->kind = kind;
    sc->name = strndup(tok->loc, tok->len);
    sc->ty = ty;
    tag_scope = sc;
    return sc;
}

static VarScope *find_var(Token *tok) {
    for (VarScope *sc = var_scope; sc; sc = sc->next) {
        if (strlen(sc->name) == tok->len &&
            !strncmp(tok->loc, sc->name, tok->len))
            return sc;
    }
    return NULL;
}

static Type *find_typedef(Token *tok) {
    if (tok->kind == TK_IDENT) {
        VarScope *v = find_var(tok);
        if (v)
            return v->type_def;
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
    lvar->align = ty->align;
    lvar->is_local = true;
    push_scope(name)->var = lvar;
    locals = lvar;
    return lvar;
}

static Var *new_gvar(char *name, Type *ty, bool is_static, bool emit) {
    Var *gvar = calloc(1, sizeof(Var));
    gvar->name = name;
    gvar->ty = ty;
    gvar->is_static = is_static;
    gvar->align = ty->align;
    gvar->is_local = false;
    push_scope(name)->var = gvar;
    if (emit) {
        gvar->next = globals;
        globals = gvar;
    }
    return gvar;
}
static char *new_label(void) {
    static int cnt = 0;
    char *buf = malloc(20);
    sprintf(buf, ".LC%d", cnt++);
    return buf;
}

static Var *new_string_literal(char *p, int len) {
    Type *ty = array_of(ty_char, len);
    Var *var = new_gvar(new_label(), ty, true, true);
    var->contents = p;
    var->ascii = true;
    return var;
}

bool is_typename(Token *tok) {
    if (tok->kind == TK_IDENT) {
        return find_typedef(tok);
    }

    char *tn[] = {"int",    "char",     "struct",   "union",  "_Bool",
                  "enum",   "short",    "long",     "void",   "typedef",
                  "static", "extern",   "_Alignas", "signed", "unsigned",
                  "const",  "volatile", "float",    "double"};
    for (int i = 0; i < sizeof(tn) / sizeof(*tn); i++)
        if (equal(tok, tn[i]))
            return true;
    return false;
}

//
// Initializer
//

static Initializer *new_initializer(Type *ty, int len) {
    Initializer *init = calloc(1, sizeof(Initializer));
    init->ty = ty;
    init->len = len;
    if (len) {
        init->children = calloc(len, sizeof(Initializer *));
    }
    return init;
}

// string-initializer = str
static Initializer *string_initializer(Token **rest, Token *tok, Type *ty) {
    if (ty->is_incomplete) {
        ty->size = tok->contents_len;
        ty->array_len = tok->contents_len;
        ty->is_incomplete = false;
    }
    Initializer *init = new_initializer(ty, ty->array_len);
    int len = (ty->array_len < tok->contents_len) ? ty->array_len
                                                  : tok->contents_len; // min;
    for (int i = 0; i < len; i++) {
        init->children[i] = new_initializer(ty->base, 0);
        init->children[i]->expr = new_number(tok->contents[i], tok);
    }
    *rest = tok->next;
    return init;
}

// array-initializer = "{" initializer ("," initializer)* ","? "}"
static Initializer *array_initializer(Token **rest, Token *tok, Type *ty) {
    bool has_paren = consume(&tok, tok, "{");
    if (ty->is_incomplete) {
        int i = 0;
        for (Token *tok2 = tok; (!is_end(tok2)); i++) {
            if (i > 0)
                tok2 = skip(tok2, ",");
            initializer(&tok2, tok2, ty->base);
        }
        ty->array_len = i;
        ty->size = size_of(ty->base) * i;
        ty->is_incomplete = false;
    }
    Initializer *init = new_initializer(ty, ty->array_len);
    for (int i = 0; i < ty->array_len && (!is_end(tok)); i++) {
        if (i > 0)
            tok = skip(tok, ",");
        init->children[i] = initializer(&tok, tok, ty->base);
    }
    if (!has_paren) {
        *rest = tok;
        return init;
    }
    if (!is_end(tok)) {
        warn_tok(tok, "parse: initializer: skip excess element");
        tok = skip_excess_element(tok);
    }
    *rest = skip_end(tok);
    return init;
}

// struct-initializer = "{" initializer ("," initializer)* ","? "}" | assign
static Initializer *struct_initializer(Token **rest, Token *tok, Type *ty) {
    if (!equal(tok, "{")) {
        Token *tok2;
        Node *expr = assign(&tok2, tok);
        add_type(expr);
        if (expr->ty->kind == TY_STRUCT) {
            Initializer *init = new_initializer(ty, 0);
            init->expr = expr;
            *rest = tok2;
            return init;
        }
    }
    bool has_paren = consume(&tok, tok, "{");

    int len = 0;
    for (Member *m = ty->member; m; m = m->next)
        len++;

    Initializer *init = new_initializer(ty, len);
    int i = 0;
    for (Member *m = ty->member; m && !is_end(tok); m = m->next, i++) {
        if (i > 0)
            tok = skip(tok, ",");
        init->children[i] = initializer(&tok, tok, m->ty);
    }
    for (int i = 0; i < ty->array_len && (!is_end(tok)); i++) {
        if (i > 0)
            tok = skip(tok, ",");
        init->children[i] = initializer(&tok, tok, ty->base);
    }

    if (!has_paren) {
        *rest = tok;
        return init;
    }
    if (!is_end(tok)) {
        warn_tok(tok, "parse: initializer: skip excess element");
        tok = skip_excess_element(tok);
    }
    *rest = skip_end(tok);
    return init;
}

static Node *designator_expr(Designator *desg, Token *tok) {
    if (desg->var) {
        return new_var_node(desg->var, tok);
    }
    assert(desg->parent);
    if (desg->member) {
        Node *node =
            new_unary_node(ND_MEMBER, designator_expr(desg->parent, tok), tok);
        node->member = desg->member;
        return node;
    }
    Node *node = designator_expr(desg->parent, tok);
    node = new_add_node(node, new_number(desg->index, tok), tok),
    node = new_unary_node(ND_DEREF, node, tok);
    return node;
}

// scalar: a = 0; => { a = 0; }
// array:
// a[3] = {0, 1, 2};
// => {a[0] = 0; a[1] = 1; a[2] = 2;}
// => {*(a+0) = 0; *(a+1) = 1; *(a+2) = 2;}
// struct:
// struct {int a; int b;} x = {1, 2};
// => {x.a = 1; x.b = 2;}
static Node *new_lvar_initialization(Node *cur, Initializer *init, Type *ty,
                                     Designator *desg, Token *tok) {
    if (ty->kind == TY_ARRAY) {
        for (int i = 0; i < ty->array_len; i++) {
            Designator desg_child = {desg, i};
            Initializer *child = init ? init->children[i] : NULL;
            cur =
                new_lvar_initialization(cur, child, ty->base, &desg_child, tok);
        }
        return cur;
    }
    if (ty->kind == TY_STRUCT && (!init || init->len)) {
        int i = 0;
        for (Member *m = ty->member; m; m = m->next, i++) {
            Designator desg_child = {desg};
            desg_child.member = m;
            Initializer *child = init ? init->children[i] : NULL;
            cur = new_lvar_initialization(cur, child, m->ty, &desg_child, tok);
        }
        return cur;
    }
    Node *lhs = designator_expr(desg, tok);
    Node *rhs = (init && init->expr) ? init->expr : new_number(0, tok);
    lhs = new_binary_node(ND_ASSIGN, lhs, rhs, tok);
    lhs->is_init = true;
    cur->next = new_unary_node(ND_EXPR_STMT, lhs, tok);
    return cur->next;
}

static Node *lvar_initializer(Token **rest, Token *tok, Var *var) {
    Initializer *init = initializer(rest, tok, var->ty);
    Node head = {};
    Designator desg_head = {};
    desg_head.var = var;
    new_lvar_initialization(&head, init, var->ty, &desg_head, tok);
    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
}

static void write_buf(char *buf, unsigned long val, int sz) {
    switch (sz) {
    case 1:
        *(unsigned char *)buf = val;
        return;
    case 2:
        *(unsigned short *)buf = val;
        return;
    case 4:
        *(unsigned int *)buf = val;
        return;
    default:
        assert(sz == 8);
        *(unsigned long *)buf = val;
        return;
    }
}

static Relocation *write_gvar_data(Relocation *cur, char *buf,
                                   Initializer *init, Type *ty, int offset) {
    if (ty->kind == TY_STRUCT) {
        int i = 0;
        for (Member *m = ty->member; m; m = m->next, i++) {
            Initializer *child = init->children[i];
            if (child)
                cur =
                    write_gvar_data(cur, buf, child, m->ty, offset + m->offset);
        }
        return cur;
    }
    if (ty->kind == TY_ARRAY) {
        int sz = size_of(ty->base);
        for (int i = 0; i < ty->array_len; i++) {
            Initializer *child = init->children[i];
            if (child)
                cur =
                    write_gvar_data(cur, buf, child, ty->base, offset + sz * i);
        }
        return cur;
    }
    if (ty->kind == TY_FLOAT) {
        *(float *)(buf + offset) = eval_double(init->expr);
        return cur;
    }
    if (ty->kind == TY_DOUBLE) {
        *(double *)(buf + offset) = eval_double(init->expr);
        return cur;
    }
    Var *var = NULL;
    long val = eval2(init->expr, &var);
    if (var) {
        Relocation *reloc = calloc(1, sizeof(Relocation));
        reloc->offset = offset;
        reloc->label = var->name;
        reloc->addend = val;
        cur->next = reloc;
        return cur->next;
    }
    write_buf(buf + offset, val, size_of(ty));
    return cur;
}

static char *gvar_initializer(Token **rest, Token *tok, Var *var) {
    Initializer *init = initializer(rest, tok, var->ty);
    char *buf = calloc(1, size_of(var->ty));
    Relocation head = {};
    write_gvar_data(&head, buf, init, var->ty, 0);
    var->contents = buf;
    var->reloc = head.next;
    return buf;
}

//
// Parser
//

// program = ( global-var |  funcdef )*
// global-var = decl-specifier init-declarator ("," init-declarator)* ";"
// !! initializer must be evaluated as compile-time constant.
Program *parse(Token *tok) {
    globals = NULL;
    // Add builtin function
    new_gvar("__builtin_va_start", func_type(ty_void), false, false);

    Function head = {};
    Function *cur = &head;

    while (tok->kind != TK_EOF) {
        DeclContext ctx = {};
        Token *start = tok;
        Type *basety = decl_specifier(&tok, tok, &ctx);
        if (consume(&tok, tok, ";"))
            continue;
        Type *ty = declarator(&tok, tok, basety);

        if (ctx.type_def) {
            // typedef int a, *b;
            for (;;) {
                if (!ty->name)
                    error_tok(ty->name_pos,
                              "parse: program: expected typedef name");
                push_scope(get_ident(ty->name))->type_def = ty;
                if (equal(tok, ";")) {
                    tok = skip(tok, ";");
                    break;
                }
                tok = skip(tok, ",");
                ty = declarator(&tok, tok, basety);
            }
            continue;
        }
        if (ty->kind == TY_FUNC) {
            current_fn =
                new_gvar(get_ident(ty->name), ty, ctx.is_static, false);
            if (!consume(&tok, tok, ";")) {
                cur = cur->next = funcdef(&tok, start);
            }
            continue;
        }
        for (;;) {
            if (!ty->name)
                error_tok(ty->name_pos,
                          "parse: program: expected variable name");
            Var *var = new_gvar(get_ident(ty->name), ty, ctx.is_static,
                                !ctx.is_extern);
            if (ctx.align)
                var->align = ctx.align;
            if (equal(tok, "=")) {
                gvar_initializer(&tok, tok->next, var);
            }
            if (consume(&tok, tok, ";")) {
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

static void add_func_ident(char *name) {
    Var *var = new_string_literal(name, strlen(name) + 1);
    push_scope("__func__")->var = var;
}

// funcdef = decl-specifier declarator "{" compound_stmt
static Function *funcdef(Token **rest, Token *tok) {
    locals = NULL;

    DeclContext ctx = {};
    Type *ty = decl_specifier(&tok, tok, &ctx);
    ty = declarator(&tok, tok, ty);
    if (!ty->name)
        error_tok(ty->name_pos, "parse: funcdef: expected function name");

    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(ty->name);
    fn->is_static = ctx.is_static;
    enter_scope();
    for (Type *t = ty->params; t; t = t->next) {
        if (!ty->name)
            error_tok(t->name_pos, "parse: funcdef: expected parameter name");
        new_lvar(get_ident(t->name), t);
    }
    fn->params = locals;

    tok = skip(tok, "{");
    add_func_ident(fn->name);
    fn->node = compound_stmt(&tok, tok)->body;
    fn->locals = locals;
    fn->is_variadic = ty->is_variadic;
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

// declaration = decl-specifier init-declarator ("," init-declarator)* ";"
// init-declarator = declarator ("=" initializer)?
static Node *declaration(Token **rest, Token *tok) {
    DeclContext ctx = {};
    Type *basety = decl_specifier(&tok, tok, &ctx);

    Node head = {};
    Node *cur = &head;
    int cnt = 0;
    while (!equal(tok, ";")) {
        if (cnt++ > 0)
            tok = skip(tok, ",");

        Type *ty = declarator(&tok, tok, basety);
        if (!ty->name)
            error_tok(ty->name_pos, "parse: variable declared void");
        if (ty->kind == TY_VOID) {
            error_tok(tok, "parse: variable declared void");
        }
        if (ctx.type_def) {
            push_scope(get_ident(ty->name))->type_def = ty;
            // tok = tok->next;
            continue;
        }
        if (ctx.is_static) {
            Var *var = new_gvar(new_label(), ty, ctx.is_static, true);
            push_scope(get_ident(ty->name))->var = var;
            if (equal(tok, "="))
                gvar_initializer(&tok, tok->next, var);
            continue;
        }
        Var *var = new_lvar(get_ident(ty->name), ty);
        if (ctx.align)
            var->align = ctx.align;

        // ("=" initializer)?
        if (equal(tok, "=")) {
            cur = cur->next = lvar_initializer(&tok, tok->next, var);
        }
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = skip(tok, ";");
    return node;
}
// initializer = array-initializer
//             | string-initializer
//             | struct-initializer
//             | assign
static Initializer *initializer(Token **rest, Token *tok, Type *ty) {
    if (ty->kind == TY_ARRAY && ty->base->kind == TY_CHAR &&
        tok->kind == TK_STR) {
        return string_initializer(rest, tok, ty);
    }
    if (ty->kind == TY_ARRAY) {
        return array_initializer(rest, tok, ty);
    }
    if (ty->kind == TY_STRUCT) {
        return struct_initializer(rest, tok, ty);
    }
    Initializer *init = new_initializer(ty, 0);
    bool has_paren = consume(&tok, tok, "{");
    init->expr = assign(&tok, tok);
    if (has_paren) {
        tok = skip_end(tok);
    }
    *rest = tok;
    return init;
}

// decl-specifier = (storage-class-specifier | type-specifier | type-qualifier)*
// type-specifier = builtin-type | struct-union-spec | enum-spec | typedef-name
// storage-class-specifier = "typedef" | "extern" | "static"
// type-qualifier = "const" | "volatile"
// alignment-specifier = "_Alignas" "(" typename | const-expr ")"
static Type *decl_specifier(Token **rest, Token *tok, DeclContext *ctx) {
    enum {
        VOID = 1 << 0,
        BOOL = 1 << 2,
        CHAR = 1 << 4,
        SHORT = 1 << 6,
        INT = 1 << 8,
        LONG = 1 << 10,
        FLOAT = 1 << 12,
        DOUBLE = 1 << 14,
        OTHER = 1 << 16,
        SIGNED = 1 << 17,
        UNSIGNED = 1 << 18,
    };
    int cnt = 0;
    Type *spec_ty = ty_int;
    bool is_const = false;

    while (is_typename(tok)) {
        // storage-class
        if (equal(tok, "typedef") || equal(tok, "static") ||
            equal(tok, "extern")) {
            if (!ctx)
                error_tok(tok, "parse: decl-specifier: storage-class-specifier "
                               "not allowed in this context");

            if (equal(tok, "typedef")) {
                if (ctx->type_def)
                    error_tok(tok,
                              "parse: decl-specifier: duplicate `typedef`");
                ctx->type_def = true;
            }
            if (equal(tok, "static")) {
                if (ctx->is_static)
                    error_tok(tok, "parse: decl-specifier: duplicate `static`");
                ctx->is_static = true;
            }
            if (equal(tok, "extern")) {
                ctx->is_extern = true;
            }
            // validation
            if (ctx->is_extern + ctx->is_static + ctx->type_def > 1) {
                error_tok(tok, "parse: decl-specifier: cannot use multiple "
                               "storage class specifier");
            }
            tok = tok->next;
            continue;
        }
        // type qualifier
        if (equal(tok, "volatile") || equal(tok, "const")) {
            if (equal(tok, "const"))
                is_const = true;
            // ignore volatile

            tok = tok->next;
            continue;
        }

        if (equal(tok, "_Alignas")) {
            if (!ctx)
                error_tok(tok, "parse: decl-specifier: alignment-specifier "
                               "not allowed in this context");
            tok = skip(tok->next, "(");
            if (is_typename(tok)) {
                ctx->align = typename(&tok, tok)->align;
            } else {
                ctx->align = const_expr(&tok, tok);
            }
            tok = skip(tok, ")");
            continue;
        }

        // user defined types

        // if IDENT && is_typename, it is typedef-name
        if (tok->kind == TK_IDENT) {
            if (cnt) {
                break;
            }
            Type *ty = find_typedef(tok);
            if (!ty) {
                error_tok(tok, "parse: decl-specifier: internal error");
            }
            tok = tok->next;
            spec_ty = ty;
            cnt += OTHER;
            continue;
        }
        if (equal(tok, "struct") || equal(tok, "union")) {
            if (cnt) {
                break;
            }
            Type *ty = struct_spec(&tok, tok);
            spec_ty = ty;
            cnt += OTHER;
            continue;
        }
        if (equal(tok, "enum")) {
            if (cnt) {
                break;
            }
            Type *ty = enum_spec(&tok, tok);
            spec_ty = ty;
            cnt += OTHER;
            continue;
        }

        // built-in types
        if (consume(&tok, tok, "void")) {
            cnt += VOID;
        } else if (consume(&tok, tok, "_Bool")) {
            cnt += BOOL;
        } else if (consume(&tok, tok, "int")) {
            cnt += INT;
        } else if (consume(&tok, tok, "short")) {
            cnt += SHORT;
        } else if (consume(&tok, tok, "long")) {
            cnt += LONG;
        } else if (consume(&tok, tok, "char")) {
            cnt += CHAR;
        } else if (consume(&tok, tok, "float")) {
            cnt += FLOAT;
        } else if (consume(&tok, tok, "double")) {
            cnt += DOUBLE;
        } else if (consume(&tok, tok, "signed")) {
            cnt |= SIGNED;
        } else if (consume(&tok, tok, "unsigned")) {
            cnt |= UNSIGNED;
        }
        // validation check
        switch (cnt) {
        case VOID:
            spec_ty = ty_void;
            break;
        case BOOL:
            spec_ty = ty_bool;
            break;
        case UNSIGNED + CHAR:
            spec_ty = ty_uchar;
            break;
        case CHAR:
        case SIGNED + CHAR:
            spec_ty = ty_char;
            break;
        case SHORT:
        case SHORT + INT:
        case SIGNED + SHORT + INT:
        case SIGNED + SHORT:
            spec_ty = ty_short;
            break;
        case UNSIGNED + SHORT + INT:
        case UNSIGNED + SHORT:
            spec_ty = ty_ushort;
            break;
        case INT:
        case SIGNED:
        case SIGNED + INT:
            spec_ty = ty_int;
            break;
        case UNSIGNED:
        case UNSIGNED + INT:
            spec_ty = ty_uint;
            break;
        case LONG:
        case LONG + INT:
        case LONG + LONG:
        case LONG + LONG + INT:
        case SIGNED + LONG:
        case SIGNED + LONG + INT:
        case SIGNED + LONG + LONG:
        case SIGNED + LONG + LONG + INT:
            spec_ty = ty_long;
            break;
        case UNSIGNED + LONG:
        case UNSIGNED + LONG + INT:
        case UNSIGNED + LONG + LONG:
        case UNSIGNED + LONG + LONG + INT:
            spec_ty = ty_ulong;
            break;
        case FLOAT:
            spec_ty = ty_float;
            break;
        case DOUBLE:
        case LONG + DOUBLE:
            spec_ty = ty_double;
            break;
        default:
            error_tok(tok, "parse: unsupported type-specifier");
        }
    }
    // epilogue
    *rest = tok;
    if (is_const) {
        spec_ty = copy_type(spec_ty);
        spec_ty->is_const = is_const;
    }
    return spec_ty;
}

// enum-spec = "enum" ident? "{" enum-list | "enum" ident
// enum-list = ident ("=" const-expr)? ("," ident ("=" const-expr)?)* ","? "}"
static Type *enum_spec(Token **rest, Token *tok) {
    Type *ty = enum_type();
    tok = skip(tok, "enum");
    Token *tag = NULL;
    if (tok->kind == TK_IDENT) {
        tag = tok;
        tok = tok->next;
    }
    if (tag && !equal(tok, "{")) {
        TagScope *sc = find_tag(tag);
        if (!sc) {
            error_tok(tag, "parse: enum-spec: unknown enum");
        }
        if (sc->kind != TAG_ENUM) {
            error_tok(tag, "parse: enum-spec: not an enum tag");
        }
        *rest = tok;
        return sc->ty;
    }
    tok = skip(tok, "{");

    // Parse enum-list
    int val = 0;
    int cnt = 0;

    while (!consume_end(&tok, tok)) {
        if (cnt++ > 0)
            tok = skip(tok, ",");
        char *name = get_ident(tok);
        tok = tok->next;
        if (equal(tok, "=")) {
            val = const_expr(&tok, tok->next);
        }

        VarScope *vsc = push_scope(name);
        vsc->enum_ty = ty;
        vsc->enum_val = val++;
    }
    if (tag)
        push_tag_scope(tag, ty, TAG_ENUM);

    *rest = tok;
    return ty;
}

// struct-union-spec = ("struct" | "union") ident? ("{" struct-decl)?
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
        *rest = tok;
        TagScope *sc = find_tag(tag);
        if (sc) {
            if (sc->kind != kind)
                error_tok(tag,
                          "parse: tag kind mismatch from previous declaration");
            return sc->ty;
        }
        Type *ty = struct_type();
        ty->is_incomplete = true;
        push_tag_scope(tag, ty, kind);
        return ty;
    }

    tok = skip(tok, "{");
    Type *ty = struct_type();
    ty->member = struct_decl(rest, tok);

    if (kind == TAG_STRUCT) {
        int offset = 0;
        for (Member *m = ty->member; m; m = m->next) {
            offset = align_to(offset, m->align);
            m->offset = offset;
            offset += m->ty->size;
            if (ty->align < m->align)
                ty->align = m->align;
        }
        ty->size = align_to(offset, ty->align);
    } else {
        for (Member *m = ty->member; m; m = m->next) {
            m->offset = 0;
            if (ty->size < m->ty->size)
                ty->size = m->ty->size;
            if (ty->align < m->align)
                ty->align = m->align;
        }
        ty->size = align_to(ty->size, ty->align);
    }
    if (tag) {
        TagScope *sc = find_tag(tag);
        if (sc && sc->depth == scope_depth) {
            *sc->ty = *ty;
            return sc->ty;
        }
        push_tag_scope(tag, ty, kind);
    }
    return ty;
}

// struct-decl = (decl_spec declarator ("," declarator)* ";")* "}"
static Member *struct_decl(Token **rest, Token *tok) {
    Member head = {};
    Member *cur = &head;

    while (!equal(tok, "}")) {
        DeclContext ctx = {};
        Type *basety = decl_specifier(&tok, tok, &ctx);
        int cnt = 0;

        while (!equal(tok, ";")) {
            if (cnt++ > 0) {
                tok = skip(tok, ",");
            }
            Type *ty = declarator(&tok, tok, basety);
            cur->next = new_member(ty);
            cur = cur->next;
            cur->align = ctx.align ? ctx.align : cur->ty->align;
        }
        tok = skip(tok, ";");
    }

    *rest = skip(tok, "}");
    return head.next;
}

// pointer = ("*" ("volatile" | "const")*)*
static Type *pointer(Token **rest, Token *tok, Type *ty) {
    while (consume(&tok, tok, "*")) {
        ty = pointer_to(ty);
        while (consume(&tok, tok, "const")) {
            ty->is_const = true;
        }
        while (consume(&tok, tok, "volatile")) {
            ;
        }
    }
    *rest = tok;
    return ty;
}

// abstract-declarator = pointer ("(" declarator ")")? type-suffix?
static Type *abstract_declarator(Token **rest, Token *tok, Type *ty) {
    ty = pointer(&tok, tok, ty);
    if (equal(tok, "(")) {
        Type *placeholder = calloc(1, sizeof(Type));
        Type *new_ty = abstract_declarator(&tok, tok->next, placeholder);
        tok = skip(tok, ")");
        *placeholder = *type_suffix(rest, tok, ty);
        return new_ty;
    }
    return type_suffix(rest, tok, ty);
}
// typename = type-specifier abstract-declarator
static Type *typename(Token **rest, Token *tok) {
    Type *ty = decl_specifier(&tok, tok, NULL);
    return abstract_declarator(rest, tok, ty);
}

// declarator = pointer ("(" declarator ")" | ident?) type-suffix?
static Type *declarator(Token **rest, Token *tok, Type *ty) {
    ty = pointer(&tok, tok, ty);
    if (equal(tok, "(")) {
        Type *placeholder = calloc(1, sizeof(Type));
        Type *new_ty = declarator(&tok, tok->next, placeholder);
        tok = skip(tok, ")");
        *placeholder = *type_suffix(rest, tok, ty);
        return new_ty;
    }
    Token *name = NULL;
    Token *name_pos = tok;
    if (tok->kind == TK_IDENT) {
        name = tok;
        tok = tok->next;
    }
    ty = type_suffix(rest, tok, ty);
    ty->name = name;
    ty->name_pos = name_pos;
    return ty;
}

// type-suffix = ("(" func-params | "[" array-dim)?
static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "(")) {
        ty = func_type(ty);
        ty = func_params(rest, tok->next, ty);
        return ty;
    }
    if (equal(tok, "[")) {
        return array_dim(rest, tok->next, ty);
    }
    *rest = tok;
    return ty;
}

// array-dim = const_expr? "]" type-suffix
static Type *array_dim(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "]")) {
        // imcomplete
        ty = type_suffix(rest, tok->next, ty);
        ty = array_of(ty, 0);
        ty->is_incomplete = true;
        return ty;
    }

    int sz = const_expr(&tok, tok);
    tok = skip(tok, "]");
    *rest = tok;
    ty = type_suffix(rest, tok, ty);
    return array_of(ty, sz);
}

// func-params = ("void" | param ("," param)* ("," "...")? )? ")"
// param       = decl-specifier declarator
static Type *func_params(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "void") && equal(tok->next, ")")) {
        *rest = tok->next->next;
        return ty;
    }
    Type head = {};
    Type *cur = &head;
    int cnt = 0;
    bool is_variadic = false;
    while (!equal(tok, ")")) {
        if (cnt++ > 0)
            tok = skip(tok, ",");
        if (equal(tok, "...")) {
            is_variadic = true;
            tok = tok->next;
            skip(tok, ")"); // assertion
            break;
        }
        Type *basety = decl_specifier(&tok, tok, NULL);
        Type *ty = declarator(&tok, tok, basety);
        if (ty->kind == TY_ARRAY) {
            Token *name = ty->name;
            ty = pointer_to(ty->base);
            ty->name = name;
        }
        cur = cur->next = copy_type(ty);
    }
    *rest = skip(tok, ")");
    ty->params = head.next;
    ty->is_variadic = is_variadic;
    return ty;
}

// stmt = expr ";"
//      | ";"
//      | "return" expr? ";"
//      | "if" "(" expr ")" stmt ( "else" stmt )?
//      | "while" "(" expr ")" stmt
//      | "do" stmt "while" "(" expr ")" ";"
//      | "for" "(" expr?; expr?; expr?; ")" stmt
//      | "{" compound_stmt
//      | "goto" ident ";"
//      | ident ":" stmt
//      | "break" ";"
//      | "continue" ";"
//      | "switch" "(" expr ")" stmt
//      | "case" const-expr ":" stmt
//      | "default" ":" stmt
static Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "{")) {
        return compound_stmt(rest, tok->next);
    }
    if (equal(tok, ";")) {
        Node *node = new_node(ND_BLOCK, tok);
        *rest = tok->next;
        return node;
    }
    if (equal(tok, "switch")) {
        Node *node = new_node(ND_SWITCH, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");

        Node *sw = current_switch;
        current_switch = node;
        node->then = stmt(rest, tok);
        current_switch = sw;
        return node;
    }
    if (equal(tok, "case")) {
        if (!current_switch)
            error_tok(tok, "parse: stmt: stray `case`");
        Node *node = new_node(ND_CASE, tok);
        long val = const_expr(&tok, tok->next);
        tok = skip(tok, ":");
        node->then = stmt(rest, tok);
        node->val = val;
        node->case_next = current_switch->case_next;
        current_switch->case_next = node;
        return node;
    }
    if (equal(tok, "default")) {
        if (!current_switch)
            error_tok(tok, "parse: stmt: stray `default`");
        tok = skip(tok->next, ":");
        Node *node = new_node(ND_CASE, tok);
        node->then = stmt(rest, tok);
        current_switch->default_case = node;
        return node;
    }
    if (equal(tok, "break")) {
        Node *node = new_node(ND_BREAK, tok);
        *rest = skip(tok->next, ";");
        return node;
    }
    if (equal(tok, "continue")) {
        Node *node = new_node(ND_CONTINUE, tok);
        *rest = skip(tok->next, ";");
        return node;
    }
    if (equal(tok, "return")) {
        Node *node = new_node(ND_RETURN, tok);
        if (equal(tok->next, ";")) {
            *rest = skip(tok->next, ";");
            return node;
        }
        Node *exp = expr(&tok, tok->next);
        *rest = skip(tok, ";");

        add_type(exp);
        node->lhs = new_cast(exp, current_fn->ty->return_ty);
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
    if (equal(tok, "do")) {
        Node *node = new_node(ND_DO, tok);
        node->then = stmt(&tok, tok->next);
        tok = skip(tok, "while");
        tok = skip(tok, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        tok = skip(tok, ";");

        *rest = tok;
        return node;
    }
    if (equal(tok, "for")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");
        enter_scope();

        if (is_typename(tok)) {
            node->init = declaration(&tok, tok);
        } else if (!equal(tok, ";")) {
            node->init = expr_stmt(&tok, tok);
            tok = skip(tok, ";");
        } else {
            tok = skip(tok, ";");
        }

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
        leave_scope();
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
    if (equal(tok, "goto")) {
        Node *node = new_node(ND_GOTO, tok);
        node->labelname = get_ident(tok->next);
        *rest = skip(tok->next->next, ";");
        return node;
    }
    if (tok->kind == TK_IDENT && equal(tok->next, ":")) {
        Node *node = new_node(ND_LABEL, tok);
        node->labelname = strndup(tok->loc, tok->len);
        node->lhs = stmt(rest, tok->next->next);
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

// expr = assign ("," expr)?
static Node *expr(Token **rest, Token *tok) {
    Node *node = assign(&tok, tok);
    if (equal(tok, ",")) {
        return new_binary_node(ND_COMMA, node, expr(rest, tok->next), tok);
    }
    *rest = tok;
    return node;
}

// evaluate constant expression
static long eval(Node *node) { return eval2(node, NULL); }
// evaluate for initializer evaluation
static long eval2(Node *node, Var **var) {
    add_type(node);
    if (is_flonum(node->ty))
        return eval_double(node);
    switch (node->kind) {
    case ND_NUM:
        return node->val;
    case ND_CAST: {
        long val = eval2(node->lhs, var);
        if (size_of(node->ty) == 8 || !is_integer(node->ty))
            return val;

        switch (size_of(node->ty)) {
        case 1:
            if (node->ty->is_unsigned)
                return (unsigned char)eval(node->lhs);
            return (char)eval(node->lhs);
        case 2:
            if (node->ty->is_unsigned)
                return (unsigned short)eval(node->lhs);
            return (short)eval(node->lhs);
        case 4:
            if (node->ty->is_unsigned)
                return (unsigned int)eval(node->lhs);
            return (int)eval(node->lhs);
        default:
            error_tok(node->tok, "parse: eval: unknown size of type");
        }
    }
    case ND_ADDR:
        if (!var)
            error_tok(node->tok,
                      "parse: const_expr: not a constant expression");
        // int g1; int *g2 = &g1 + 3;
        if (*var || node->lhs->kind != ND_VAR || node->lhs->var->is_local)
            error_tok(node->tok, "parse: invalid initializer");
        *var = node->lhs->var;
        return 0;
    case ND_VAR:
        if (!var)
            error_tok(node->tok,
                      "parse: const_expr: not a constant expression");
        // int g1[3]; int *g2 = g1 + 3;
        if (*var || node->var->ty->kind != TY_ARRAY)
            error_tok(node->tok, "parse: invalid initializer");
        *var = node->var;
        return 0;
    case ND_ADD:
        return eval2(node->lhs, var) + eval(node->rhs);
    case ND_SUB:
        return eval2(node->lhs, var) - eval(node->rhs);
    case ND_MUL:
        return eval(node->lhs) * eval(node->rhs);
    case ND_DIV:
        if (node->lhs->ty->is_unsigned)
            return (unsigned long)eval(node->lhs) / eval(node->rhs);
        return eval(node->lhs) / eval(node->rhs);
    case ND_MOD:
        return eval(node->lhs) % eval(node->rhs);
    case ND_AND:
        return eval(node->lhs) & eval(node->rhs);
    case ND_OR:
        return eval(node->lhs) | eval(node->rhs);
    case ND_XOR:
        return eval(node->lhs) ^ eval(node->rhs);
    case ND_SHL:
        return eval(node->lhs) << eval(node->rhs);
    case ND_SHR:
        if (node->lhs->ty->is_unsigned)
            return (unsigned long)eval(node->lhs) >> eval(node->rhs);
        return eval(node->lhs) >> eval(node->rhs);
    case ND_LOGAND:
        return eval(node->lhs) && eval(node->rhs);
    case ND_LOGOR:
        return eval(node->lhs) || eval(node->rhs);
    case ND_EQ:
        return eval(node->lhs) == eval(node->rhs);
    case ND_NE:
        return eval(node->lhs) != eval(node->rhs);
    case ND_LE:
        if (node->lhs->ty->is_unsigned)
            return (unsigned long)eval(node->lhs) <= eval(node->rhs);
        return eval(node->lhs) <= eval(node->rhs);
    case ND_LT:
        if (node->lhs->ty->is_unsigned)
            return (unsigned long)eval(node->lhs) < eval(node->rhs);
        return eval(node->lhs) < eval(node->rhs);
    case ND_COND:
        return eval(node->cond) ? eval(node->then) : eval(node->els);
    case ND_COMMA:
        return eval(node->rhs);
    case ND_NOT:
        return !eval(node->lhs);
    case ND_BITNOT:
        return ~eval(node->lhs);
    default:
        error_tok(node->tok, "parse: const_expr: not a constant expression");
    }
}

static double eval_double(Node *node) {
    add_type(node);
    if (is_integer(node->ty)) {
        if (node->ty->is_unsigned)
            return (unsigned long)eval(node);
        else
            return eval(node);
    }
    switch (node->kind) {
    case ND_NUM:
        return node->fval;
    case ND_CAST: {
        if (is_flonum(node->lhs->ty))
            return eval_double(node->lhs);
        return eval(node->lhs);
    }
    case ND_ADD:
        return eval_double(node->lhs) + eval_double(node->rhs);
    case ND_SUB:
        return eval_double(node->lhs) - eval_double(node->rhs);
    case ND_MUL:
        return eval_double(node->lhs) * eval_double(node->rhs);
    case ND_DIV:
        return eval_double(node->lhs) / eval_double(node->rhs);
    case ND_COND:
        return eval_double(node->cond) ? eval_double(node->then)
                                       : eval_double(node->els);
    case ND_COMMA:
        return eval_double(node->rhs);
    default:
        error_tok(node->tok, "parse: const_expr: not a constant expression");
    }
}

// const-expr = conditional
long const_expr(Token **rest, Token *tok) {
    Node *node = conditional(rest, tok);
    return eval(node);
}

// assign = conditional ( assign-op assign )*
// assign-op = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "|=" | "&=" | "^=" |
// "<<=" | ">>="
static Node *assign(Token **rest, Token *tok) {
    Node *node = conditional(&tok, tok);
    for (;;) {
        if (equal(tok, "=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node, rhs, op);
        }
        if (equal(tok, "+=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node, new_add_node(node, rhs, op),
                                   op);
        }
        if (equal(tok, "-=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node, new_sub_node(node, rhs, op),
                                   op);
        }
        if (equal(tok, "*=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_MUL, node, rhs, op), op);
        }
        if (equal(tok, "/=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_DIV, node, rhs, op), op);
        }
        if (equal(tok, "%=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_MOD, node, rhs, op), op);
        }
        if (equal(tok, "&=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_AND, node, rhs, op), op);
        }
        if (equal(tok, "^=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_XOR, node, rhs, op), op);
        }
        if (equal(tok, "|=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_OR, node, rhs, op), op);
        }
        if (equal(tok, "<<=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_SHL, node, rhs, op), op);
        }
        if (equal(tok, ">>=")) {
            Token *op = tok;
            Node *rhs = assign(&tok, tok->next);
            node = new_binary_node(ND_ASSIGN, node,
                                   new_binary_node(ND_SHR, node, rhs, op), op);
        }
        *rest = tok;
        return node;
    }
}

// conditional = logical-or ("?" expr ":" conditional)?
static Node *conditional(Token **rest, Token *tok) {
    Node *node = logical_or(&tok, tok);
    if (!equal(tok, "?")) {
        *rest = tok;
        return node;
    }
    Node *cond = new_node(ND_COND, tok);
    cond->cond = node;
    cond->then = expr(&tok, tok->next);
    tok = skip(tok, ":");
    cond->els = conditional(rest, tok);
    return cond;
}

// logical-or = logical-and ("||" logical-and)*
static Node *logical_or(Token **rest, Token *tok) {
    Node *node = logical_and(&tok, tok);
    while (equal(tok, "||")) {
        Token *op = tok;
        node =
            new_binary_node(ND_LOGOR, node, logical_and(&tok, tok->next), op);
    }
    *rest = tok;
    return node;
}

// logical-and = inclusive-or ("&&" inclusive-or)*
static Node *logical_and(Token **rest, Token *tok) {
    Node *node = bit_or(&tok, tok);
    while (equal(tok, "&&")) {
        Token *op = tok;
        node = new_binary_node(ND_LOGAND, node, bit_or(&tok, tok->next), op);
    }
    *rest = tok;
    return node;
}

// or = xor ("|" xor)*
static Node *bit_or(Token **rest, Token *tok) {
    Node *node = bit_xor(&tok, tok);
    while (equal(tok, "|")) {
        Token *op = tok;
        node = new_binary_node(ND_OR, node, bit_xor(&tok, tok->next), op);
    }
    *rest = tok;
    return node;
}
// xor = and ("^" and)*
static Node *bit_xor(Token **rest, Token *tok) {
    Node *node = bit_and(&tok, tok);
    while (equal(tok, "^")) {
        Token *op = tok;
        node = new_binary_node(ND_XOR, node, bit_and(&tok, tok->next), op);
    }
    *rest = tok;
    return node;
}
// and = equality ("&" equality)*
static Node *bit_and(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    while (equal(tok, "&")) {
        Token *op = tok;
        node = new_binary_node(ND_AND, node, equality(&tok, tok->next), op);
    }
    *rest = tok;
    return node;
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

// relational = shift ( "<" shift | ">" shift | "<=" shift | ">=" shift )*
static Node *relational(Token **rest, Token *tok) {
    Node *node = shift(&tok, tok);
    for (;;) {
        if (equal(tok, "<")) {
            Token *op = tok;
            Node *rhs = shift(&tok, tok->next);
            node = new_binary_node(ND_LT, node, rhs, op);
            continue;
        }
        if (equal(tok, "<=")) {
            Token *op = tok;
            Node *rhs = shift(&tok, tok->next);
            node = new_binary_node(ND_LE, node, rhs, op);
            continue;
        }
        if (equal(tok, ">")) {
            Token *op = tok;
            Node *rhs = shift(&tok, tok->next);
            node = new_binary_node(ND_LT, rhs, node, op);
            continue;
        }
        if (equal(tok, ">=")) {
            Token *op = tok;
            Node *rhs = shift(&tok, tok->next);
            node = new_binary_node(ND_LE, rhs, node, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}
// shift = add ("<<" add | ">>" add)*
static Node *shift(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);
    for (;;) {
        Token *op = tok;
        if (equal(tok, "<<")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary_node(ND_SHL, node, rhs, op);
            continue;
        }
        if (equal(tok, ">>")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary_node(ND_SHR, node, rhs, op);
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

// mul = cast ( "*" cast | "/" cast | "%" cast )*
static Node *mul(Token **rest, Token *tok) {
    Node *node = cast(&tok, tok);
    for (;;) {
        if (equal(tok, "*")) {
            Token *op = tok;
            Node *rhs = cast(&tok, tok->next);
            node = new_binary_node(ND_MUL, node, rhs, op);
            continue;
        }
        if (equal(tok, "/")) {
            Token *op = tok;
            Node *rhs = cast(&tok, tok->next);
            node = new_binary_node(ND_DIV, node, rhs, op);
            continue;
        }
        if (equal(tok, "%")) {
            Token *op = tok;
            Node *rhs = cast(&tok, tok->next);
            node = new_binary_node(ND_MOD, node, rhs, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// compound_literal = initializer "}"
static Node *compound_literal(Token **rest, Token *tok, Type *ty,
                              Token *start) {
    if (scope_depth == 0) {
        Var *var = new_gvar(new_label(), ty, true, true);
        gvar_initializer(&tok, tok, var);
        *rest = skip_end(tok);
        return new_var_node(var, start);
    }
    Var *var = new_lvar(new_label(), ty);
    Node *lhs = new_node(ND_STMT_EXPR, start);
    lhs->body = lvar_initializer(&tok, tok, var)->body;
    *rest = skip_end(tok);
    Node *rhs = new_var_node(var, start);
    return new_binary_node(ND_COMMA, lhs, rhs, start);
}
// cast = "(" typename ")" cast | unary
//      | "(" typename ")" "{" compound-literal
static Node *cast(Token **rest, Token *tok) {
    if (equal(tok, "(") && is_typename(tok->next)) {
        Token *start = tok;
        Type *ty = typename(&tok, tok->next);
        tok = skip(tok, ")");
        if (consume(&tok, tok, "{")) {
            return compound_literal(rest, tok, ty, start);
        }
        Node *node = new_unary_node(ND_CAST, cast(rest, tok), tok);
        node->ty = ty;
        add_type(node->lhs);
        return node;
    }
    return unary(rest, tok);
}

// unary =  (unary-op)? cast | postfix
//       | "sizeof" unary
//       | "sizeof" "(" typename ")"
//       | "_Alignof" "(" typename ")"
// unary-op = ( "+" | "-" | "*" | "&" | "++" | "--" | "~" | "!")
static Node *unary(Token **rest, Token *tok) {
    Token *start = tok;
    if (equal(tok, "+")) {
        return cast(rest, tok->next);
    }
    if (equal(tok, "-")) {
        return new_binary_node(ND_SUB, new_number(0, tok),
                               cast(rest, tok->next), start);
    }
    if (equal(tok, "*")) {
        return new_unary_node(ND_DEREF, cast(rest, tok->next), start);
    }
    if (equal(tok, "&")) {
        return new_unary_node(ND_ADDR, cast(rest, tok->next), start);
    }
    if (equal(tok, "!")) {
        return new_unary_node(ND_NOT, cast(rest, tok->next), start);
    }
    if (equal(tok, "~")) {
        return new_unary_node(ND_BITNOT, cast(rest, tok->next), start);
    }
    if (equal(tok, "sizeof") && equal(tok->next, "(") &&
        is_typename(tok->next->next)) {
        Token *op = tok;
        Type *ty = typename(&tok, tok->next->next);
        *rest = skip(tok, ")");
        return new_ulong(size_of(ty), op);
    }
    if (equal(tok, "_Alignof")) {
        tok = skip(tok->next, "(");
        Type *ty = typename(&tok, tok);
        *rest = skip(tok, ")");
        return new_ulong(ty->align, tok);
    }
    if (equal(tok, "sizeof")) {
        Node *node = unary(rest, tok->next);
        add_type(node);
        return new_ulong(size_of(node->ty), tok);
    }
    if (equal(tok, "++")) {
        Node *node = cast(rest, tok->next);
        return new_binary_node(
            ND_ASSIGN, node, new_add_node(node, new_number(1, tok), tok), tok);
    }
    if (equal(tok, "--")) {
        Node *node = cast(rest, tok->next);
        return new_binary_node(
            ND_ASSIGN, node, new_sub_node(node, new_number(1, tok), tok), tok);
    }
    return postfix(rest, tok);
}

// postfix = primary
//         | postfix postfix-op?
//         | ident funcall
// postfix-op = funcall | "[" expr "]" | "." ident | "->" ident | "++" | "--"
static Node *postfix(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);
    for (;;) {
        if (equal(tok, "(")) {
            node = funcall(&tok, tok, node);
            continue;
        }
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
        if (equal(tok, "++")) {
            Node *expr1 = new_binary_node(
                ND_ASSIGN, node, new_add_node(node, new_number(1, tok), tok),
                tok);
            Node *expr2 = new_sub_node(node, new_number(1, tok), tok);
            node = new_binary_node(ND_COMMA, expr1, expr2, tok);
            tok = skip(tok, "++");
            continue;
        }
        if (equal(tok, "--")) {
            Node *expr1 = new_binary_node(
                ND_ASSIGN, node, new_sub_node(node, new_number(1, tok), tok),
                tok);
            Node *expr2 = new_add_node(node, new_number(1, tok), tok);
            node = new_binary_node(ND_COMMA, expr1, expr2, tok);
            tok = skip(tok, "--");
            continue;
        }
        *rest = tok;
        return node;
    }
}

// primary = "(" "{" compound-stmt ")"
//         | "(" expr ")"
//         | num
//         | str
//         | char
//         | ident func-args?
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(") && equal(tok->next, "{")) {
        Node *node = new_node(ND_STMT_EXPR, tok);
        node->body = compound_stmt(&tok, tok->next->next)->body;
        *rest = skip(tok, ")");

        Node *cur = node->body;
        while (cur->next)
            cur = cur->next;
        if (cur->kind != ND_EXPR_STMT)
            error_tok(cur->tok, "parse: stmt-expr: statement expression "
                                "returning void is not supported");
        return node;
    }
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    if (tok->kind == TK_IDENT) {
        // variable or enum constant
        VarScope *sc = find_var(tok);
        *rest = tok->next;
        if (sc) {
            if (sc->var)
                return new_var_node(sc->var, tok);
            if (sc->enum_ty)
                return new_number(sc->enum_val, tok);
        }
        if (equal(tok->next, "(")) {
            warn_tok(tok, "parse: implicit declaration of a function");
            char *name = strndup(tok->loc, tok->len);
            Var *var = new_gvar(name, func_type(ty_int), true, false);
            return new_var_node(var, tok);
        }
        error_tok(tok, "parse: primary: undefined variable");
    }
    if (tok->kind == TK_STR) {
        Var *var = new_string_literal(tok->contents, tok->contents_len);
        *rest = tok->next;
        return new_var_node(var, tok);
    }
    if (tok->kind != TK_NUM)
        error_tok(tok, "parse: primary: expected expression");

    Node *node;
    if (!tok->ty)
        error_tok(tok, "parse: internal error: number token must have type");
    if (is_flonum(tok->ty)) {
        node = new_node(ND_NUM, tok);
        node->fval = tok->fval;
    } else {
        node = new_number(get_number(tok), tok);
    }
    node->ty = tok->ty;
    *rest = tok->next;
    return node;
}

// funcall = "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **rest, Token *tok, Node *fn) {
    add_type(fn);
    if (fn->ty->kind != TY_FUNC &&
        (fn->ty->kind != TY_PTR || fn->ty->base->kind != TY_FUNC)) {
        error_tok(fn->tok, "parse: funcall: not a function");
    }

    // func-args
    tok = skip(tok, "(");
    Node *cur = new_node(ND_NOP_EXPR, tok);

    Var **args = NULL;
    int nargs = 0;
    Type *func_ty = (fn->ty->kind == TY_FUNC) ? fn->ty : fn->ty->base;
    Type *arg_ty = func_ty->params;
    while (!equal(tok, ")")) {
        if (nargs)
            tok = skip(tok, ",");

        Node *arg = assign(&tok, tok);
        add_type(arg);
        if (arg_ty) {
            arg = new_cast(arg, arg_ty);
            arg_ty = arg_ty->next;
        } else if (arg->ty->kind == TY_FLOAT) {
            arg = new_cast(arg, ty_double);
        }
        Var *var = is_pointing(arg->ty)
                       ? new_lvar("", pointer_to(arg->ty->base))
                       : new_lvar("", arg->ty);

        args = realloc(args, sizeof(*args) * (nargs + 1));
        args[nargs] = var;
        nargs++;
        Node *expr =
            new_binary_node(ND_ASSIGN, new_var_node(var, tok), arg, tok);
        cur = new_binary_node(ND_COMMA, cur, expr, tok);
    }
    *rest = skip(tok, ")");

    Node *node = new_unary_node(ND_FUNCALL, fn, tok);
    node->ty = func_ty->return_ty;
    node->args = args;
    node->nargs = nargs;

    return new_binary_node(ND_COMMA, cur, node, tok);
}
