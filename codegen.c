#include "lcc.h"

//
// Codegen
//

// register
static char *argreg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static char *argreg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static char *argreg16[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
static char *argreg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};

static char *reg64[] = {"r10", "r11", "r12", "r13", "r14", "r15"};
static char *reg32[] = {"r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
static char *reg16[] = {"r10w", "r11w", "r12w", "r13w", "r14w", "r15w"};
static char *reg8[] = {"r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};

static char *argreg(int sz, int idx) {
    if (sz == 1) {
        return argreg8[idx];
    }
    if (sz == 2) {
        return argreg16[idx];
    }
    if (sz == 4) {
        return argreg32[idx];
    }
    if (sz == 8) {
        return argreg64[idx];
    }
    error("invalid size of register: %d", sz);
}

static char *reg(int idx) {
    if (idx < 0 || sizeof(reg64) / sizeof(*reg64) <= idx)
        error("registor out of range: %d", idx);
    return reg64[idx];
}

static char *regsz(int sz, int idx) {
    if (idx < 0 || sizeof(reg64) / sizeof(*reg64) <= idx)
        error("registor out of range: %d", idx);
    if (sz == 1) {
        return reg8[idx];
    }
    if (sz == 2) {
        return reg16[idx];
    }
    if (sz == 4) {
        return reg32[idx];
    }
    if (sz == 8) {
        return reg64[idx];
    }
    error("invalid size of register: %d", sz);
}

static char *regx(Type *ty, int idx) {
    if (idx < 0 || sizeof(reg64) / sizeof(*reg64) <= idx)
        error("registor out of range: %d", idx);
    if (is_pointing(ty) || size_of(ty) == 8) {
        return regsz(8, idx);
    } else {
        return regsz(4, idx);
    }
}

static int top;
static char *reg_push() { return reg(top++); }
static char *reg_push_sz(int sz) { return regsz(sz, top++); }
static char *reg_push_x(Type *ty) { return regx(ty, top++); }
static char *reg_pop() { return reg(--top); }
static char *reg_pop_sz(int sz) { return regsz(sz, --top); }
static char *reg_pop_x(Type *ty) { return regx(ty, --top); }

// label
static int labelcnt;
static int next_label() { return ++labelcnt; }

static char *funcname; // currently generating

// address

// load a value from stack top is pointing to.
static void load(Type *ty) {
    if (ty->kind == TY_ARRAY || ty->kind == TY_STRUCT) {
        // do nothing since generally array or struct can't be loaded to a
        // register. As a result, evaluation is converted to pointer to the
        // first element.
        return;
    }
    char *rs = reg_pop();
    // When we load value of size <4, always extend to the size of int,
    // so that we can assume that the lower half of register contains valid value.
    char *rd = reg_push_x(ty);
    int sz = size_of(ty);
    if (sz == 1) {
        printf("\tmovsx %s, byte ptr [%s]\n", rd, rs);
    } else if (sz == 2) {
        printf("\tmovsx %s, word ptr [%s]\n", rd, rs);
    } else if (sz == 4) {
        printf("\tmov %s, dword ptr [%s]\n", rd, rs);
    } else {
        assert(sz == 8);
        printf("\tmov %s, [%s]\n", rd, rs);
    }
    return;
}
// pop address and value, store value into address, push address
static void store(Type *ty) {
    char *rd = reg_pop();
    if (ty->kind == TY_STRUCT) {
        char *rs = reg_pop();
        for (int i = 0; i < size_of(ty); i++) {
            printf("\tmov al, [%s+%d]\n", rs, i);
            printf("\tmov [%s+%d], al\n", rd, i);
        }
        reg_push(); // address to top
        return;
    }
    char *rs = reg_pop_sz(size_of(ty));
    printf("\tmov [%s], %s\n", rd, rs);
    reg_push(); // address to top
    return;
}

// cast a type of stack top
static void cast(Type *from, Type *to) {
    if (to->kind == TY_VOID)
        return;
    if (to->kind == TY_STRUCT)
        return;

    int sz = size_of(to);
    char *rs = reg_pop();
    char *rd = reg_push();
    if (to->kind == TY_BOOL) {
        printf("\tcmp %s, 0\n", rs);
        printf("\tsetne %sb\n", rd);
        printf("\tmovzx %s, %sb\n", rd, rd);
    } else if (size_of(to) == 1) {
        printf("\tmovsx %s, %sb\n", rd, rs);
    } else if (size_of(to) == 2) {
        printf("\tmovsx %s, %sw\n", rd, rs);
    } else if (size_of(to) == 4) {
        printf("\tmov %sd, %sd\n", rd, rs);
    } else if (is_integer(from) && size_of(from) < 8) {
        printf("\tmovsx %s, %sd\n", rd, rs);
    }
}

// code generation
static void gen_addr(Node *node);
static void gen_expr(Node *node);
static void gen_stmt(Node *node);

// code generate address of node
static void gen_addr(Node *node) {
    if (node->kind == ND_VAR) {
        if (node->var->is_local)
            printf("\tlea %s, [rbp-%d]\n", reg_push(), node->var->offset);
        else
            printf("\tmov %s, offset %s\n", reg_push(), node->var->name);
        return;
    }
    if (node->kind == ND_DEREF) {
        gen_expr(node->lhs);
        return;
    }
    if (node->kind == ND_MEMBER) {
        gen_addr(node->lhs);
        printf("\tadd %s, %d\n", reg_pop(), node->member->offset);
        reg_push();
        return;
    }
    error_tok(node->tok, "codegen: gen_addr: not an lvalue");
}

// code generate expression
static void gen_expr(Node *node) {
    printf(".loc 1 %d\n", node->tok->lineno);
    switch (node->kind) {
    case ND_NUM:
        printf("\tmov %s, %ld\n", reg_push(), node->val);
        return;
    case ND_VAR:
    case ND_MEMBER:
        gen_addr(node);
        load(node->ty);
        return;
    case ND_CAST:
        gen_expr(node->lhs);
        cast(node->lhs->ty, node->ty);
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        load(node->ty);
        return;
    case ND_ASSIGN:
        if (node->ty->kind == TY_ARRAY) {
            error_tok(node->tok, "codegen: assign: array is not an lvalue");
        }
        gen_expr(node->rhs);
        gen_addr(node->lhs);
        store(node->ty);
        return;
    case ND_LOGOR: {
        int label = next_label();

        gen_expr(node->lhs);
        char *rs = reg_pop_x(node->lhs->ty);
        printf("\tcmp %s, 0\n", rs);
        printf("\tjne .L.true.%d\n", label);
        gen_expr(node->rhs);
        char *rd = reg_pop_x(node->rhs->ty);
        printf("\tcmp %s, 0\n", rd);
        printf("\tjne .L.true.%d\n", label);

        rd = reg_push();
        printf("\tmov %s, 0\n", rd);
        printf("\tjmp .L.end.%d\n", label);
        printf(".L.true.%d:\n", label);
        printf("\tmov %s, 1\n", rd);
        printf(".L.end.%d:\n", label);
        return;
    }
    case ND_LOGAND: {
        int label = next_label();

        gen_expr(node->lhs);
        char *rs = reg_pop_x(node->lhs->ty);
        printf("\tcmp %s, 0\n", rs);
        printf("\tje .L.false.%d\n", label);
        gen_expr(node->rhs);
        char *rd = reg_pop_x(node->rhs->ty);
        printf("\tcmp %s, 0\n", rd);
        printf("\tje .L.false.%d\n", label);

        rd = reg_push();
        printf("\tmov %s, 1\n", rd);
        printf("\tjmp .L.end.%d\n", label);
        printf(".L.false.%d:\n", label);
        printf("\tmov %s, 0\n", rd);
        printf(".L.end.%d:\n", label);
        return;
    }
    case ND_COMMA:
        gen_expr(node->lhs);
        reg_pop();
        gen_expr(node->rhs);
        return;
    case ND_STMT_EXPR:
        for (Node *n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        reg_push();
        return;
    case ND_FUNCALL: {
        int top_orig = top;
        top = 0;
        printf("\tpush r10\n");
        printf("\tpush r11\n");
        printf("\tpush r12\n");
        printf("\tpush r13\n");
        printf("\tpush r14\n");
        printf("\tpush r15\n");

        // push arguments then pop to register
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            if (nargs >= 6) {
                error_tok(arg->tok, "codegen: funcall: too many arguments: %d",
                          nargs);
            }
            gen_expr(arg);
            printf("\tpush %s\n", reg_pop());
            printf("\tsub rsp, 8\n");
            nargs++;
        }
        for (int i = nargs - 1; i >= 0; i--) {
            printf("\tadd rsp, 8\n");
            printf("\tpop %s\n", argreg64[i]);
        }

        printf("\tmov rax, 0\n");
        printf("\tcall %s\n", node->funcname);

        top = top_orig;
        printf("\tpop r15\n");
        printf("\tpop r14\n");
        printf("\tpop r13\n");
        printf("\tpop r12\n");
        printf("\tpop r11\n");
        printf("\tpop r10\n");

        printf("\tmov %s, rax\n", reg_push());
        return;
    }
    }

    // binary node
    gen_expr(node->lhs);
    gen_expr(node->rhs);

    char *rs = reg_pop_x(node->lhs->ty);
    char *rd = reg_pop_x(node->lhs->ty);
    reg_push();

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
        if (size_of(node->ty) == 8) {
            printf("\tmov rax, %s\n", rd);
            printf("\tcqo\n");
            printf("\tidiv %s\n", rs);
            printf("\tmov %s, rax\n", rd);
        } else {
            printf("\tmov eax, %s\n", rd);
            printf("\tcdq\n");
            printf("\tidiv %s\n", rs);
            printf("\tmov %s, eax\n", rd);
        }
        return;
    }
    if (node->kind == ND_EQ) {
        printf("\tcmp %s, %s\n", rd, rs);
        printf("\tsete al\n");
        printf("\tmovzx %s, al\n", rd);
        return;
    }
    if (node->kind == ND_NE) {
        printf("\tcmp %s, %s\n", rd, rs);
        printf("\tsetne al\n");
        printf("\tmovzx %s, al\n", rd);
        return;
    }
    if (node->kind == ND_LT) {
        printf("\tcmp %s, %s\n", rd, rs);
        printf("\tsetl al\n");
        printf("\tmovzx %s, al\n", rd);
        return;
    }
    if (node->kind == ND_LE) {
        printf("\tcmp %s, %s\n", rd, rs);
        printf("\tsetle al\n");
        printf("\tmovzx %s, al\n", rd);
        return;
    }
    error_tok(node->tok, "codegen: gen_expr: invalid expression");
}

static void gen_stmt(Node *node) {
    printf(".loc 1 %d\n", node->tok->lineno);
    if (node->kind == ND_BLOCK) {
        for (Node *n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        return;
    }
    if (node->kind == ND_RETURN) {
        gen_expr(node->lhs);
        printf("\tmov rax, %s\n", reg_pop());
        printf("\tjmp .L.return.%s\n", funcname);
        return;
    }
    if (node->kind == ND_FOR) {
        int lfor = next_label();
        if (node->init)
            gen_stmt(node->init);
        printf(".L.begin.%d:\n", lfor);
        if (node->cond) {
            gen_expr(node->cond);
            printf("\tcmp %s, 0\n", reg_pop());
            printf("\tje .L.end.%d\n", lfor);
        }
        gen_stmt(node->then);
        if (node->inc)
            gen_stmt(node->inc);
        printf("\tjmp .L.begin.%d\n", lfor);
        printf(".L.end.%d:\n", lfor);
        return;
    }
    if (node->kind == ND_IF) {
        gen_expr(node->cond);
        printf("\tcmp %s, 0\n", reg_pop());
        int lif = next_label();
        if (node->els) {
            printf("\tje .L.els.%d\n", lif);
            gen_stmt(node->then);
            printf("\tjmp .L.end.%d\n", lif);
            printf(".L.els.%d:\n", lif);
            gen_stmt(node->els);
        } else {
            printf("\tje .L.end.%d\n", lif);
            gen_stmt(node->then);
        }
        printf(".L.end.%d:\n", lif);
        return;
    }
    if (node->kind == ND_EXPR_STMT) {
        gen_expr(node->lhs);
        reg_pop();
        return;
    }
    error_tok(node->tok, "codegen: gen_stmt: invalid statement");
}

static void emit_string_literal(char *contents, int len) {
    printf("\t.ascii \"");
    for (int i = 0; i < len; i++) {
        char c = contents[i];
        if (iscntrl(c) || c == '\"' || c == '\\') {
            char d1 = c / 64;
            char d2 = (c % 64) / 8;
            char d3 = (c % 8);
            printf("\\%d%d%d", d1, d2, d3);
        } else {
            printf("%c", c);
        }
    }
    printf("\"\n");
}

static void emit_data(Program *prog) {
    printf(".data\n");

    for (Var *gv = prog->globals; gv; gv = gv->next) {
        printf("%s:\n", gv->name);
        if (gv->contents)
            emit_string_literal(gv->contents, gv->contents_len);
        else
            printf("\t.zero %d\n", size_of(gv->ty));
    }
}

static void emit_text(Program *prog) {
    printf(".text\n");
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        printf(".globl %s\n", fn->name);
        printf("%s:\n", fn->name);
        funcname = fn->name;

        // prologue
        // save stack pointer
        printf("\tpush rbp\n");
        printf("\tmov rbp, rsp\n");
        printf("\tsub rsp, %d\n", fn->stacksize);
        // save callee-saved registers
        printf("\tmov [rbp-8], r12\n");
        printf("\tmov [rbp-16], r13\n");
        printf("\tmov [rbp-24], r14\n");
        printf("\tmov [rbp-32], r15\n");

        int i = 0;
        for (Var *v = fn->params; v; v = v->next) {
            i++;
        }
        for (Var *v = fn->params; v; v = v->next) {
            printf("\tmov [rbp-%d], %s\n", v->offset,
                   argreg(size_of(v->ty), --i));
        }
        for (Node *n = fn->node; n; n = n->next) {
            gen_stmt(n);
            if (top != 0)
                error("top: %d\n", top);
        }

        // Epilogue
        // recover callee-saved registers
        printf(".L.return.%s:\n", funcname);
        printf("\tmov r12, [rbp-8]\n");
        printf("\tmov r13, [rbp-16]\n");
        printf("\tmov r14, [rbp-24]\n");
        printf("\tmov r15, [rbp-32]\n");
        // recover stack pointer
        printf("\tmov rsp, rbp\n");
        printf("\tpop rbp\n");
        printf("\tret\n");
    }
}

void codegen(Program *prog) {
    printf(".intel_syntax noprefix\n");
    emit_data(prog);
    emit_text(prog);
}
