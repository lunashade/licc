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

static char *freg64[] = {"xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13"};

static char *freg(int idx) {
    if (idx < 0 || sizeof(freg64) / sizeof(*freg64) <= idx)
        error("registor out of range: %d", idx);
    return freg64[idx];
}

static char *argreg(int sz, int idx) {
    if (idx < 0 || sizeof(argreg64) / sizeof(*argreg64) <= idx)
        error("registor out of range: %d", idx);
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

static char *argregx(Type *ty, int idx) {
    int sz = size_of(ty);
    if (sz == 8)
        return argreg(8, idx);
    else
        return argreg(4, idx);
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
static int breaklabel;
static int continuelabel;

static Function *current_fn;

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
    // so that we can assume that the lower half of register contains valid
    // value.
    char *rd = reg_push_x(ty);
    int sz = size_of(ty);
    char *insn = ty->is_unsigned ? "movzx" : "movsx";
    if (sz == 1) {
        printf("\t%s %s, byte ptr [%s]\n", insn, rd, rs);
    } else if (sz == 2) {
        printf("\t%s %s, word ptr [%s]\n", insn, rd, rs);
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

// compare stack-top value to zero, push 1 or 0 to top
static void cmpzero(Type *ty) {
    if (ty->kind == TY_FLOAT) {
        printf("\txorpd xmm0, xmm0\n");
        printf("\tucomiss xmm0, %s\n", freg(--top));
    } else if (ty->kind == TY_DOUBLE) {
        printf("\txorpd xmm0, xmm0\n");
        printf("\tucomisd xmm0, %s\n", freg(--top));
    } else {
        printf("\tcmp %s, 0\n", regx(ty, --top));
    }
}

// cast a type of stack top
static void cast(Type *from, Type *to) {
    if (to->kind == TY_VOID)
        return;

    int sz = size_of(to);
    char *r = reg(top - 1);
    char *fr = freg(top - 1);

    if (to->kind == TY_BOOL) {
        cmpzero(from);
        printf("\tsetne %sb\n", r);
        printf("\tmovzx %s, %sb\n", r, r);
        top++;
        return;
    }

    // from FLONUM
    if (from->kind == TY_FLOAT) {
        if (to->kind == TY_FLOAT)
            return;
        if (to->kind == TY_DOUBLE)
            printf("\tcvtss2sd %s, %s\n", fr, fr);
        else
            printf("\tcvttss2si %s, %s\n", r, fr);
        return;
    }
    if (from->kind == TY_DOUBLE) {
        if (to->kind == TY_DOUBLE)
            return;
        if (to->kind == TY_FLOAT)
            printf("\tcvtsd2ss %s, %s\n", fr, fr);
        else
            printf("\tcvttsd2si %s, %s\n", r, fr);
        return;
    }
    // INTEGER -> FLONUM
    if (to->kind == TY_FLOAT) {
        printf("\tcvtsi2ss %s, %s\n", fr, r);
        return;
    }
    if (to->kind == TY_DOUBLE) {
        printf("\tcvtsi2sd %s, %s\n", fr, r);
        return;
    }

    // INTEGER -> INTEGER
    char *insn = to->is_unsigned ? "movzx" : "movsx";

    if (size_of(to) == 1) {
        printf("\t%s %s, %sb\n", insn, r, r);
    } else if (size_of(to) == 2) {
        printf("\t%s %s, %sw\n", insn, r, r);
    } else if (size_of(to) == 4) {
        printf("\tmov %sd, %sd\n", r, r);
    } else if (is_integer(from) && size_of(from) < 8 && !from->is_unsigned) {
        printf("\tmovsx %s, %sd\n", r, r);
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
    if (node->kind == ND_COMMA) {
        gen_expr(node->lhs);
        reg_pop();
        gen_addr(node->rhs);
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

static void builtin_va_start(Node *node) {
    int n = 0;
    for (Var *var = current_fn->params; var; var = var->next)
        n++;

    printf("\tmov rax, [rbp-%d]\n", node->args[0]->offset);
    printf("\tmov dword ptr [rax], %d\n", n * 8);
    printf("\tmov [rax+16], rbp\n");
    printf("\tsub qword ptr [rax+16], 80\n");
    top++;
}

// code generate expression
static void gen_expr(Node *node) {
    printf(".loc 1 %d\n", node->tok->lineno);
    switch (node->kind) {
    case ND_NOP_EXPR:
        top++;
        return;
    case ND_NUM:
        switch (node->ty->kind) {
        case TY_FLOAT: {
            float fval = node->fval;
            printf("\tmov rax, %u\n", *(int *)&fval);
            printf("\tpush rax\n");
            printf("\tmovss %s, [rsp]\n", freg(top++));
            printf("\tadd rsp, 8\n");
            return;
        }
        case TY_DOUBLE: {
            printf("\tmov rax, %lu\n", *(long *)&node->fval);
            printf("\tpush rax\n");
            printf("\tmovsd %s, [rsp]\n", freg(top++));
            printf("\tadd rsp, 8\n");
            return;
        }
        case TY_LONG:
            printf("\tmovabs %s, %lu\n", reg_push(), node->val);
            return;
        default:
            printf("\tmov %s, %lu\n", reg_push(), node->val);
            return;
        }
    case ND_VAR:
    case ND_MEMBER:
        gen_addr(node);
        load(node->ty);
        return;
    case ND_COND: {
        int label = next_label();
        gen_expr(node->cond);
        cmpzero(node->cond->ty);
        printf("\tje .L.else.%d\n", label);
        gen_expr(node->then);
        top--;
        printf("\tjmp .L.end.%d\n", label);
        printf(".L.else.%d:\n", label);
        gen_expr(node->els);
        printf(".L.end.%d:\n", label);
        return;
    }
    case ND_CAST:
        gen_expr(node->lhs);
        if (node->lhs->ty == NULL) {
            error_tok(node->lhs->tok, "wtf");
        }
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
    case ND_BITNOT:
        gen_expr(node->lhs);
        printf("\tnot %s\n", reg_pop());
        reg_push();
        return;
    case ND_NOT: {
        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        char *rd = reg_push();
        printf("\tsete %sb\n", rd);
        printf("\tmovzx %s, %sb\n", rd, rd);
        return;
    }
    case ND_LOGOR: {
        int label = next_label();

        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        printf("\tjne .L.true.%d\n", label);
        gen_expr(node->rhs);
        cmpzero(node->rhs->ty);
        printf("\tjne .L.true.%d\n", label);

        char *rd = reg_push();
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
        cmpzero(node->lhs->ty);
        printf("\tje .L.false.%d\n", label);
        gen_expr(node->rhs);
        cmpzero(node->rhs->ty);
        printf("\tje .L.false.%d\n", label);

        char *rd = reg_push();
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
        if (!strcmp(node->funcname, "__builtin_va_start")) {
            builtin_va_start(node);
            return;
        }

        printf("\tpush r10\n");
        printf("\tpush r11\n");

        // push arguments then pop to register
        for (int i = 0; i < node->nargs; i++) {
            Var *arg = node->args[i];
            int sz = size_of(arg->ty);
            char *insn = (arg->ty->is_unsigned) ? "movzx" : "movsx";
            switch (sz) {
            case 1:
                printf("\t%s %s, byte ptr [rbp-%d]\n", insn,
                       argregx(arg->ty, i), arg->offset);
                break;
            case 2:
                printf("\t%s %s, word ptr [rbp-%d]\n", insn,
                       argregx(arg->ty, i), arg->offset);
                break;
            case 4:
                printf("\tmov %s, dword ptr [rbp-%d]\n", argregx(arg->ty, i),
                       arg->offset);
                break;
            default:
                assert(sz == 8);
                printf("\tmov %s, [rbp-%d]\n", argregx(arg->ty, i),
                       arg->offset);
                break;
            }
        }

        printf("\tmov rax, 0\n");
        printf("\tcall %s\n", node->funcname);
        if (node->ty->kind == TY_BOOL)
            printf("\tmovzx eax, al\n");

        printf("\tpop r11\n");
        printf("\tpop r10\n");

        printf("\tmov %s, rax\n", reg_push());
        return;
    }
    }

    // binary node
    gen_expr(node->lhs);
    gen_expr(node->rhs);

    char *rs = regx(node->rhs->ty, top - 1);
    char *rd = regx(node->lhs->ty, top - 2);
    char *fs = freg(top - 1);
    char *fd = freg(top - 2);
    top--;

    if (node->kind == ND_SHL) {
        printf("\tmov rcx, %s\n", reg(top)); // rs with 8-bit register
        printf("\tshl %s, cl\n", rd);
        return;
    }
    if (node->kind == ND_SHR) {
        printf("\tmov rcx, %s\n", reg(top)); // rs with 8-bit register
        if (node->ty->is_unsigned)
            printf("\tshr %s, cl\n", rd);
        else
            printf("\tsar %s, cl\n", rd);

        return;
    }
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
            if (node->ty->is_unsigned) {
                printf("\tmov rdx, 0\n");
                printf("\tdiv %s\n", rs);
            } else {
                printf("\tcqo\n");
                printf("\tidiv %s\n", rs);
            }

            printf("\tmov %s, rax\n", rd);
        } else {
            printf("\tmov eax, %s\n", rd);
            if (node->ty->is_unsigned) {
                printf("\tmov edx, 0\n");
                printf("\tdiv %s\n", rs);
            } else {
                printf("\tcdq\n");
                printf("\tidiv %s\n", rs);
            }
            printf("\tmov %s, eax\n", rd);
        }
        return;
    }
    if (node->kind == ND_MOD) {
        if (size_of(node->ty) == 8) {
            printf("\tmov rax, %s\n", rd);
            if (node->ty->is_unsigned) {
                printf("\tmov rdx, 0\n");
                printf("\tdiv %s\n", rs);
            } else {
                printf("\tcqo\n");
                printf("\tidiv %s\n", rs);
            }
            printf("\tmov %s, rdx\n", rd);
        } else {
            printf("\tmov eax, %s\n", rd);
            if (node->ty->is_unsigned) {
                printf("\tmov edx, 0\n");
                printf("\tdiv %s\n", rs);
            } else {
                printf("\tcdq\n");
                printf("\tidiv %s\n", rs);
            }
            printf("\tmov %s, edx\n", rd);
        }
        return;
    }
    if (node->kind == ND_OR) {
        printf("\tor %s, %s\n", rd, rs);
        return;
    }
    if (node->kind == ND_AND) {
        printf("\tand %s, %s\n", rd, rs);
        return;
    }
    if (node->kind == ND_XOR) {
        printf("\txor %s, %s\n", rd, rs);
        return;
    }
    if (node->kind == ND_EQ) {
        if (node->lhs->ty->kind == TY_FLOAT)
            printf("\tucomiss %s, %s\n", fd, fs);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            printf("\tucomisd %s, %s\n", fd, fs);
        else
            printf("\tcmp %s, %s\n", rd, rs);

        printf("\tsete al\n");
        printf("\tmovzx %s, al\n", rd);
        return;
    }
    if (node->kind == ND_NE) {
        if (node->lhs->ty->kind == TY_FLOAT)
            printf("\tucomiss %s, %s\n", fd, fs);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            printf("\tucomisd %s, %s\n", fd, fs);
        else
            printf("\tcmp %s, %s\n", rd, rs);
        printf("\tsetne al\n");
        printf("\tmovzx %s, al\n", rd);
        return;
    }
    if (node->kind == ND_LT) {
        if (node->lhs->ty->kind == TY_FLOAT) {
            printf("\tucomiss %s, %s\n", fd, fs);
            printf("\tsetb al\n");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            printf("\tucomisd %s, %s\n", fd, fs);
            printf("\tsetb al\n");
        } else {
            printf("\tcmp %s, %s\n", rd, rs);
            if (node->lhs->ty->is_unsigned) {
                printf("\tsetb al\n");
            } else {
                printf("\tsetl al\n");
            }
        }
        printf("\tmovzx %s, al\n", rd);
        return;
    }
    if (node->kind == ND_LE) {
        if (node->lhs->ty->kind == TY_FLOAT) {
            printf("\tucomiss %s, %s\n", fd, fs);
            printf("\tsetbe al\n");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            printf("\tucomisd %s, %s\n", fd, fs);
            printf("\tsetbe al\n");
        } else {
            printf("\tcmp %s, %s\n", rd, rs);
            if (node->lhs->ty->is_unsigned) {
                printf("\tsetbe al\n");
            } else {
                printf("\tsetle al\n");
            }
        }
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
    if (node->kind == ND_SWITCH) {
        int label = next_label();
        int brk = breaklabel;
        breaklabel = label;

        node->case_label = label;

        gen_expr(node->cond);
        char *rd = reg_pop();

        for (Node *n = node->case_next; n; n = n->case_next) {
            n->case_label = next_label();
            n->case_end_label = label;
            printf("\tcmp %s, %ld\n", rd, n->val);
            printf("\tje .L.case.%d\n", n->case_label);
        }
        if (node->default_case) {
            node->default_case->case_label = next_label();
            node->default_case->case_end_label = label;
            printf("\tjmp .L.case.%d\n", node->default_case->case_label);
        }

        printf("\tjmp .L.break.%d\n", label);
        gen_stmt(node->then);
        printf("\t.L.break.%d:\n", label);

        breaklabel = brk;
        return;
    }
    if (node->kind == ND_CASE) {
        printf(".L.case.%d:\n", node->case_label);
        gen_stmt(node->then);
        return;
    }
    if (node->kind == ND_RETURN) {
        if (node->lhs) {
            gen_expr(node->lhs);
            printf("\tmov rax, %s\n", reg_pop());
        }
        printf("\tjmp .L.return.%s\n", current_fn->name);
        return;
    }
    if (node->kind == ND_DO) {
        int label = next_label();
        int brk = breaklabel;
        int cnt = continuelabel;
        continuelabel = breaklabel = label;

        printf(".L.begin.%d:\n", label);
        gen_stmt(node->then);
        printf(".L.continue.%d:\n", label);
        gen_expr(node->cond);
        printf("\tcmp %s, 0\n", reg_pop());
        printf("\tjne .L.begin.%d\n", label);
        printf(".L.break.%d:\n", label);

        continuelabel = cnt;
        breaklabel = brk;
        return;
    }
    if (node->kind == ND_FOR) {
        int lfor = next_label();
        int pastbrk = breaklabel;
        int pastcnt = continuelabel;
        continuelabel = breaklabel = lfor;

        if (node->init)
            gen_stmt(node->init);
        printf(".L.begin.%d:\n", lfor);
        if (node->cond) {
            gen_expr(node->cond);
            printf("\tcmp %s, 0\n", reg_pop());
            printf("\tje .L.break.%d\n", lfor);
        }
        gen_stmt(node->then);
        printf(".L.continue.%d:\n", lfor);
        if (node->inc)
            gen_stmt(node->inc);
        printf("\tjmp .L.begin.%d\n", lfor);
        printf(".L.break.%d:\n", lfor);

        continuelabel = pastcnt;
        breaklabel = pastbrk;
        return;
    }
    if (node->kind == ND_CONTINUE) {
        if (continuelabel == 0) {
            error_tok(node->tok, "codegen: stray continue");
        }
        printf("\tjmp .L.continue.%d\n", continuelabel);
        return;
    }
    if (node->kind == ND_BREAK) {
        if (breaklabel == 0) {
            error_tok(node->tok, "codegen: stray break");
        }
        printf("\tjmp .L.break.%d\n", breaklabel);
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
    if (node->kind == ND_GOTO) {
        printf("\tjmp .L.label.%s.%s\n", current_fn->name, node->labelname);
        return;
    }
    if (node->kind == ND_LABEL) {
        printf("\t.L.label.%s.%s:\n", current_fn->name, node->labelname);
        gen_stmt(node->lhs);
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

static void emit_bss(Program *prog) {
    printf(".bss\n");
    for (Var *gv = prog->globals; gv; gv = gv->next) {
        if (gv->contents)
            continue;
        printf(".align %d\n", gv->align);
        if (!gv->is_static)
            printf(".globl %s\n", gv->name);
        printf("%s:\n", gv->name);
        printf("\t.zero %d\n", size_of(gv->ty));
    }
}

static void emit_init_data(Var *var) {
    Relocation *reloc = var->reloc;
    int pos = 0;
    while (pos < size_of(var->ty)) {
        if (reloc && reloc->offset == pos) {
            printf("\t.quad %s+%ld\n", reloc->label, reloc->addend);
            reloc = reloc->next;
            pos += 8;
        } else {
            printf("\t.byte %d\n", var->contents[pos++]);
        }
    }
}

static void emit_data(Program *prog) {
    printf(".data\n");

    for (Var *gv = prog->globals; gv; gv = gv->next) {
        if (!gv->contents)
            continue;
        printf(".align %d\n", gv->align);
        if (!gv->is_static)
            printf(".globl %s\n", gv->name);
        printf("%s:\n", gv->name);
        if (gv->ascii)
            emit_string_literal(gv->contents, size_of(gv->ty));
        else
            emit_init_data(gv);
    }
}

static void emit_text(Program *prog) {
    printf(".text\n");
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        if (!fn->is_static)
            printf(".globl %s\n", fn->name);
        printf("%s:\n", fn->name);
        current_fn = fn;

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

        if (fn->is_variadic) {
            printf("\tmov [rbp-80], rdi\n");
            printf("\tmov [rbp-72], rsi\n");
            printf("\tmov [rbp-64], rdx\n");
            printf("\tmov [rbp-56], rcx\n");
            printf("\tmov [rbp-48], r8\n");
            printf("\tmov [rbp-40], r9\n");
        }
        // push arguments to the stack
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
        printf(".L.return.%s:\n", current_fn->name);
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
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        // calle-saved registers take 32 bytes
        // and variable-argument save area takes 48 bytes.
        int offset = fn->is_variadic ? 80 : 32;
        for (Var *v = fn->locals; v; v = v->next) {
            offset = align_to(offset, v->align);
            offset += v->ty->size;
            v->offset = offset;
        }
        fn->stacksize = align_to(offset, 16);
    }
    printf(".intel_syntax noprefix\n");
    emit_bss(prog);
    emit_data(prog);
    emit_text(prog);
}
