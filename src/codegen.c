#include "lcc.h"

//
// Codegen
//

// register
static char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8w", "%r9w"};
static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};

static char *reg64[] = {"%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
static char *reg32[] = {"%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d"};
static char *reg16[] = {"%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w"};
static char *reg8[] = {"%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b"};

static char *freg64[] = {"%xmm8",  "%xmm9",  "%xmm10",
                         "%xmm11", "%xmm12", "%xmm13"};

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
    if (ty->kind == TY_ARRAY || ty->kind == TY_STRUCT || ty->kind == TY_FUNC) {
        // do nothing since generally array or struct can't be loaded to a
        // register. As a result, evaluation is converted to pointer to the
        // first element.
        return;
    }
    if (ty->kind == TY_DOUBLE) {
        fprintf(output_file, "\tmovsd (%s), %s\n", reg(top - 1), freg(top - 1));
        return;
    }
    if (ty->kind == TY_FLOAT) {
        fprintf(output_file, "\tmovss (%s), %s\n", reg(top - 1), freg(top - 1));
        return;
    }
    char *rs = reg(top - 1);
    // When we load value of size <4, always extend to the size of int,
    // so that we can assume that the lower half of register contains valid
    // value.
    char *rd = regx(ty, top - 1);
    int sz = size_of(ty);
    char *insn = ty->is_unsigned ? "movz" : "movs";
    if (sz == 1) {
        fprintf(output_file, "\t%sbl (%s), %s\n", insn, rs, rd);
    } else if (sz == 2) {
        fprintf(output_file, "\t%swl (%s), %s\n", insn, rs, rd);
    } else if (sz == 4) {
        fprintf(output_file, "\tmov (%s), %s\n", rs, rd);
    } else {
        assert(sz == 8);
        fprintf(output_file, "\tmov (%s), %s\n", rs, rd);
    }
    return;
}
// pop address and value, store value into address, push address
static void store(Type *ty) {
    char *rd = reg_pop();
    if (ty->kind == TY_STRUCT) {
        char *rs = reg_pop();
        for (int i = 0; i < size_of(ty); i++) {
            fprintf(output_file, "\tmov %d(%s), %%al\n", i, rs);
            fprintf(output_file, "\tmov %%al, %d(%s)\n", i, rd);
        }
        reg_push(); // address to top
        return;
    }
    if (is_flonum(ty)) {
        char *insn = (ty->kind == TY_FLOAT) ? "movss" : "movsd";
        char *rs = freg(--top);
        fprintf(output_file, "\t%s %s, (%s)\n", insn, rs, rd);
        reg_push();
        return;
    }

    char *rs = reg_pop_sz(size_of(ty));
    fprintf(output_file, "\tmov %s, (%s)\n", rs, rd);
    reg_push(); // address to top
    return;
}

// compare stack-top value to zero, push 1 or 0 to top
static void cmpzero(Type *ty) {
    if (ty->kind == TY_FLOAT) {
        fprintf(output_file, "\txorpd %%xmm0, %%xmm0\n");
        fprintf(output_file, "\tucomiss %s, %%xmm0\n", freg(--top));
    } else if (ty->kind == TY_DOUBLE) {
        fprintf(output_file, "\txorpd %%xmm0, %%xmm0\n");
        fprintf(output_file, "\tucomisd %s, %%xmm0\n", freg(--top));
    } else {
        fprintf(output_file, "\tcmp $0, %s\n", regx(ty, --top));
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
        fprintf(output_file, "\tsetne %sb\n", r);
        fprintf(output_file, "\tmovzx %sb, %s\n", r, r);
        top++;
        return;
    }

    // from FLONUM
    if (from->kind == TY_FLOAT) {
        if (to->kind == TY_FLOAT)
            return;
        if (to->kind == TY_DOUBLE)
            fprintf(output_file, "\tcvtss2sd %s, %s\n", fr, fr);
        else
            fprintf(output_file, "\tcvttss2si %s, %s\n", fr, r);
        return;
    }
    if (from->kind == TY_DOUBLE) {
        if (to->kind == TY_DOUBLE)
            return;
        if (to->kind == TY_FLOAT)
            fprintf(output_file, "\tcvtsd2ss %s, %s\n", fr, fr);
        else
            fprintf(output_file, "\tcvttsd2si %s, %s\n", fr, r);
        return;
    }
    // INTEGER -> FLONUM
    if (to->kind == TY_FLOAT) {
        fprintf(output_file, "\tcvtsi2ss %s, %s\n", r, fr);
        return;
    }
    if (to->kind == TY_DOUBLE) {
        fprintf(output_file, "\tcvtsi2sd %s, %s\n", r, fr);
        return;
    }

    // INTEGER -> INTEGER
    char *insn = to->is_unsigned ? "movzx" : "movsx";

    if (size_of(to) == 1) {
        fprintf(output_file, "\t%s %sb, %s\n", insn, r, r);
    } else if (size_of(to) == 2) {
        fprintf(output_file, "\t%s %sw, %s\n", insn, r, r);
    } else if (size_of(to) == 4) {
        fprintf(output_file, "\tmov %sd, %sd\n", r, r);
    } else if (is_integer(from) && size_of(from) < 8 && !from->is_unsigned) {
        fprintf(output_file, "\tmovsx %sd, %s\n", r, r);
    }
}

// code generation
static void gen_addr(Node *node);
static void gen_expr(Node *node);
static void gen_stmt(Node *node);

// code generate address of node
static void gen_addr(Node *node) {
    if (node->kind == ND_VAR) {
        if (node->var->is_local) {
            fprintf(output_file, "\tlea -%d(%%rbp), %s\n", node->var->offset,
                    reg_push());
            return;
        }
        if (!opt_fpic) {
            fprintf(output_file, "\tmov $%s, %s\n", node->var->name,
                    reg_push());
        } else if (node->var->is_static) {
            fprintf(output_file, "\tlea %s(%%rip), %s\n", node->var->name,
                    reg_push());
        } else {
            fprintf(output_file, "\tmov %s@GOTPCREL(%%rip), %s\n",
                    node->var->name, reg_push());
        }
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
        fprintf(output_file, "\tadd $%d, %s\n", node->member->offset,
                reg_pop());
        reg_push();
        return;
    }
    error_tok(node->tok, "codegen: gen_addr: not an lvalue");
}

static void builtin_va_start(Node *node) {
    int gp = 0, fp = 0;
    for (Var *var = current_fn->params; var; var = var->next)
        if (is_flonum(var->ty))
            fp++;
        else
            gp++;

    fprintf(output_file, "\tmov -%d(%%rbp), %%rax\n", node->args[0]->offset);
    fprintf(output_file, "\tmovl $%d, (%%rax)\n", gp * 8);
    fprintf(output_file, "\tmovl $%d, 4(%%rax)\n", 48 + fp * 8);
    fprintf(output_file, "\tmov %%rbp, 16(%%rax)\n");
    fprintf(output_file, "\tsubq $128, 16(%%rax)\n");
    top++;
}

// code generate expression
static void gen_expr(Node *node) {
    fprintf(output_file, ".loc %d %d\n", node->tok->fileno, node->tok->lineno);
    switch (node->kind) {
    case ND_NOP_EXPR:
        top++;
        return;
    case ND_NUM:
        switch (node->ty->kind) {
        case TY_FLOAT: {
            float fval = node->fval;
            fprintf(output_file, "\tmov $%u, %%rax\n", *(int *)&fval);
            fprintf(output_file, "\tpush %%rax\n");
            fprintf(output_file, "\tmovss (%%rsp), %s\n", freg(top++));
            fprintf(output_file, "\tadd $8, %%rsp\n");
            return;
        }
        case TY_DOUBLE: {
            fprintf(output_file, "\tmov $%lu, %%rax\n", *(long *)&node->fval);
            fprintf(output_file, "\tpush %%rax\n");
            fprintf(output_file, "\tmovsd (%%rsp), %s\n", freg(top++));
            fprintf(output_file, "\tadd $8, %%rsp\n");
            return;
        }
        case TY_LONG:
            fprintf(output_file, "\tmovabs $%lu, %s\n", node->val, reg_push());
            return;
        default:
            fprintf(output_file, "\tmov $%lu, %s\n", node->val, reg_push());
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
        fprintf(output_file, "\tje .L.else.%d\n", label);
        gen_expr(node->then);
        top--;
        fprintf(output_file, "\tjmp .L.end.%d\n", label);
        fprintf(output_file, ".L.else.%d:\n", label);
        gen_expr(node->els);
        fprintf(output_file, ".L.end.%d:\n", label);
        return;
    }
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
    case ND_BITNOT:
        gen_expr(node->lhs);
        fprintf(output_file, "\tnot %s\n", reg_pop());
        reg_push();
        return;
    case ND_NOT: {
        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        char *rd = reg_push();
        fprintf(output_file, "\tsete %sb\n", rd);
        fprintf(output_file, "\tmovzx %sb, %s\n", rd, rd);
        return;
    }
    case ND_LOGOR: {
        int label = next_label();

        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        fprintf(output_file, "\tjne .L.true.%d\n", label);
        gen_expr(node->rhs);
        cmpzero(node->rhs->ty);
        fprintf(output_file, "\tjne .L.true.%d\n", label);

        char *rd = reg_push();
        fprintf(output_file, "\tmov $0, %s\n", rd);
        fprintf(output_file, "\tjmp .L.end.%d\n", label);
        fprintf(output_file, ".L.true.%d:\n", label);
        fprintf(output_file, "\tmov $1, %s\n", rd);
        fprintf(output_file, ".L.end.%d:\n", label);
        return;
    }
    case ND_LOGAND: {
        int label = next_label();

        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        fprintf(output_file, "\tje .L.false.%d\n", label);
        gen_expr(node->rhs);
        cmpzero(node->rhs->ty);
        fprintf(output_file, "\tje .L.false.%d\n", label);

        char *rd = reg_push();
        fprintf(output_file, "\tmov $1, %s\n", rd);
        fprintf(output_file, "\tjmp .L.end.%d\n", label);
        fprintf(output_file, ".L.false.%d:\n", label);
        fprintf(output_file, "\tmov $0, %s\n", rd);
        fprintf(output_file, ".L.end.%d:\n", label);
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
        if (node->lhs->kind == ND_VAR &&
            !strcmp(node->lhs->var->name, "__builtin_va_start")) {
            builtin_va_start(node);
            return;
        }

        fprintf(output_file, "\tsub $64, %%rsp\n");
        fprintf(output_file, "\tmov %%r10, (%%rsp)\n");
        fprintf(output_file, "\tmov %%r11, 8(%%rsp)\n");
        fprintf(output_file, "\tmovsd %%xmm8, 16(%%rsp)\n");
        fprintf(output_file, "\tmovsd %%xmm9, 24(%%rsp)\n");
        fprintf(output_file, "\tmovsd %%xmm10, 32(%%rsp)\n");
        fprintf(output_file, "\tmovsd %%xmm11, 40(%%rsp)\n");
        fprintf(output_file, "\tmovsd %%xmm12, 48(%%rsp)\n");
        fprintf(output_file, "\tmovsd %%xmm13, 56(%%rsp)\n");
        gen_expr(node->lhs);

        // push arguments then pop to register
        int gp = 0, fp = 0;
        for (int i = 0; i < node->nargs; i++) {
            Var *arg = node->args[i];
            if (is_flonum(arg->ty)) {
                if (arg->ty->kind == TY_FLOAT) {
                    fprintf(output_file, "\tmovss -%d(%%rbp), %%xmm%d\n",
                            arg->offset, fp++);
                } else {
                    fprintf(output_file, "\tmovsd -%d(%%rbp), %%xmm%d\n",
                            arg->offset, fp++);
                }
                continue;
            }
            int sz = size_of(arg->ty);
            char *insn = (arg->ty->is_unsigned) ? "movz" : "movs";
            switch (sz) {
            case 1:
                fprintf(output_file, "\t%sbl -%d(%%rbp), %s\n", insn,
                        arg->offset, argregx(arg->ty, i));
                break;
            case 2:
                fprintf(output_file, "\t%swl -%d(%%rbp), %s\n", insn,
                        arg->offset, argregx(arg->ty, i));
                break;
            case 4:
                fprintf(output_file, "\tmovl -%d(%%rbp), %s\n", arg->offset,
                        argregx(arg->ty, i));
                break;
            default:
                assert(sz == 8);
                fprintf(output_file, "\tmov -%d(%%rbp), %s\n", arg->offset,
                        argregx(arg->ty, i));
                break;
            }
        }

        fprintf(output_file, "\tmov $%d, %%rax\n", fp);
        fprintf(output_file, "\tcall *%s\n", reg(--top));
        if (node->ty->kind == TY_BOOL)
            fprintf(output_file, "\tmovzx %%al, %%eax\n");

        fprintf(output_file, "\tmov (%%rsp), %%r10\n");
        fprintf(output_file, "\tmov 8(%%rsp), %%r11\n");
        fprintf(output_file, "\tmovsd 16(%%rsp), %%xmm8\n");
        fprintf(output_file, "\tmovsd 24(%%rsp), %%xmm9\n");
        fprintf(output_file, "\tmovsd 32(%%rsp), %%xmm10\n");
        fprintf(output_file, "\tmovsd 40(%%rsp), %%xmm11\n");
        fprintf(output_file, "\tmovsd 48(%%rsp), %%xmm12\n");
        fprintf(output_file, "\tmovsd 56(%%rsp), %%xmm13\n");
        fprintf(output_file, "\tadd $64, %%rsp\n");

        if (node->ty->kind == TY_FLOAT) {
            fprintf(output_file, "\tmovss %%xmm0, %s\n", freg(top++));
        } else if (node->ty->kind == TY_DOUBLE) {
            fprintf(output_file, "\tmovsd %%xmm0, %s\n", freg(top++));
        } else {
            fprintf(output_file, "\tmov %%rax, %s\n", reg(top++));
        }
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
        fprintf(output_file, "\tmov %s, %%rcx\n",
                reg(top)); // rs with 8-bit register
        fprintf(output_file, "\tshl %%cl, %s\n", rd);
        return;
    }
    if (node->kind == ND_SHR) {
        fprintf(output_file, "\tmov %s, %%rcx\n",
                reg(top)); // rs with 8-bit register
        if (node->ty->is_unsigned)
            fprintf(output_file, "\tshr %%cl, %s\n", rd);
        else
            fprintf(output_file, "\tsar %%cl, %s\n", rd);
        return;
    }
    if (node->kind == ND_ADD) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "addss" : "addsd";
            fprintf(output_file, "\t%s %s, %s\n", insn, fs, fd);
            return;
        }
        fprintf(output_file, "\tadd %s, %s\n", rs, rd);
        return;
    }
    if (node->kind == ND_SUB) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "subss" : "subsd";
            fprintf(output_file, "\t%s %s, %s\n", insn, fs, fd);
            return;
        }
        fprintf(output_file, "\tsub %s, %s\n", rs, rd);
        return;
    }
    if (node->kind == ND_MUL) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "mulss" : "mulsd";
            fprintf(output_file, "\t%s %s, %s\n", insn, fs, fd);
            return;
        }
        fprintf(output_file, "\timul %s, %s\n", rs, rd);
        return;
    }
    if (node->kind == ND_DIV) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "divss" : "divsd";
            fprintf(output_file, "\t%s %s, %s\n", insn, fs, fd);
            return;
        }
        if (size_of(node->ty) == 8) {
            fprintf(output_file, "\tmov %s, %%rax\n", rd);
            if (node->ty->is_unsigned) {
                fprintf(output_file, "\tmov $0, %%rdx\n");
                fprintf(output_file, "\tdiv %s\n", rs);
            } else {
                fprintf(output_file, "\tcqo\n");
                fprintf(output_file, "\tidiv %s\n", rs);
            }
            fprintf(output_file, "\tmov %%rax, %s\n", rd);
        } else {
            fprintf(output_file, "\tmov %s, %%eax\n", rd);
            if (node->ty->is_unsigned) {
                fprintf(output_file, "\tmov $0, %%edx\n");
                fprintf(output_file, "\tdiv %s\n", rs);
            } else {
                fprintf(output_file, "\tcdq\n");
                fprintf(output_file, "\tidiv %s\n", rs);
            }
            fprintf(output_file, "\tmov %%eax, %s\n", rd);
        }
        return;
    }
    if (node->kind == ND_MOD) {
        if (size_of(node->ty) == 8) {
            fprintf(output_file, "\tmov %s, %%rax\n", rd);
            if (node->ty->is_unsigned) {
                fprintf(output_file, "\tmov $0, %%rdx\n");
                fprintf(output_file, "\tdiv %s\n", rs);
            } else {
                fprintf(output_file, "\tcqo\n");
                fprintf(output_file, "\tidiv %s\n", rs);
            }
            fprintf(output_file, "\tmov %%rdx, %s\n", rd);
        } else {
            fprintf(output_file, "\tmov %s, %%eax\n", rd);
            if (node->ty->is_unsigned) {
                fprintf(output_file, "\tmov $0, %%edx\n");
                fprintf(output_file, "\tdiv %s\n", rs);
            } else {
                fprintf(output_file, "\tcdq\n");
                fprintf(output_file, "\tidiv %s\n", rs);
            }
            fprintf(output_file, "\tmov %%edx, %s\n", rd);
        }
        return;
    }
    if (node->kind == ND_OR) {
        fprintf(output_file, "\tor %s, %s\n", rs, rd);
        return;
    }
    if (node->kind == ND_AND) {
        fprintf(output_file, "\tand %s, %s\n", rs, rd);
        return;
    }
    if (node->kind == ND_XOR) {
        fprintf(output_file, "\txor %s, %s\n", rs, rd);
        return;
    }
    if (node->kind == ND_EQ) {
        if (node->lhs->ty->kind == TY_FLOAT)
            fprintf(output_file, "\tucomiss %s, %s\n", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            fprintf(output_file, "\tucomisd %s, %s\n", fs, fd);
        else
            fprintf(output_file, "\tcmp %s, %s\n", rs, rd);

        fprintf(output_file, "\tsete %%al\n");
        fprintf(output_file, "\tmovzx %%al, %s\n", rd);
        return;
    }
    if (node->kind == ND_NE) {
        if (node->lhs->ty->kind == TY_FLOAT)
            fprintf(output_file, "\tucomiss %s, %s\n", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            fprintf(output_file, "\tucomisd %s, %s\n", fs, fd);
        else
            fprintf(output_file, "\tcmp %s, %s\n", rs, rd);
        fprintf(output_file, "\tsetne %%al\n");
        fprintf(output_file, "\tmovzx %%al, %s\n", rd);
        return;
    }
    if (node->kind == ND_LT) {
        if (node->lhs->ty->kind == TY_FLOAT) {
            fprintf(output_file, "\tucomiss %s, %s\n", fs, fd);
            fprintf(output_file, "\tsetb %%al\n");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            fprintf(output_file, "\tucomisd %s, %s\n", fs, fd);
            fprintf(output_file, "\tsetb %%al\n");
        } else {
            fprintf(output_file, "\tcmp %s, %s\n", rs, rd);
            if (node->lhs->ty->is_unsigned) {
                fprintf(output_file, "\tsetb %%al\n");
            } else {
                fprintf(output_file, "\tsetl %%al\n");
            }
        }
        fprintf(output_file, "\tmovzx %%al, %s\n", rd);
        return;
    }
    if (node->kind == ND_LE) {
        if (node->lhs->ty->kind == TY_FLOAT) {
            fprintf(output_file, "\tucomiss %s, %s\n", fs, fd);
            fprintf(output_file, "\tsetbe %%al\n");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            fprintf(output_file, "\tucomisd %s, %s\n", fs, fd);
            fprintf(output_file, "\tsetbe %%al\n");
        } else {
            fprintf(output_file, "\tcmp %s, %s\n", rs, rd);
            if (node->lhs->ty->is_unsigned) {
                fprintf(output_file, "\tsetbe %%al\n");
            } else {
                fprintf(output_file, "\tsetle %%al\n");
            }
        }
        fprintf(output_file, "\tmovzx %%al, %s\n", rd);
        return;
    }
    error_tok(node->tok, "codegen: gen_expr: invalid expression");
}

static void gen_stmt(Node *node) {
    fprintf(output_file, ".loc %d %d\n", node->tok->fileno, node->tok->lineno);
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
            fprintf(output_file, "\tcmp $%ld, %s\n", n->val, rd);
            fprintf(output_file, "\tje .L.case.%d\n", n->case_label);
        }
        if (node->default_case) {
            node->default_case->case_label = next_label();
            node->default_case->case_end_label = label;
            fprintf(output_file, "\tjmp .L.case.%d\n",
                    node->default_case->case_label);
        }

        fprintf(output_file, "\tjmp .L.break.%d\n", label);
        gen_stmt(node->then);
        fprintf(output_file, "\t.L.break.%d:\n", label);

        breaklabel = brk;
        return;
    }
    if (node->kind == ND_CASE) {
        fprintf(output_file, ".L.case.%d:\n", node->case_label);
        gen_stmt(node->then);
        return;
    }
    if (node->kind == ND_RETURN) {
        if (node->lhs) {
            gen_expr(node->lhs);
            if (is_flonum(node->lhs->ty)) {
                fprintf(output_file, "\tmovsd %s, %%xmm0\n", freg(--top));
            } else {
                fprintf(output_file, "\tmov %s, %%rax\n", reg(--top));
            }
        }
        fprintf(output_file, "\tjmp .L.return.%s\n", current_fn->name);
        return;
    }
    if (node->kind == ND_DO) {
        int label = next_label();
        int brk = breaklabel;
        int cnt = continuelabel;
        continuelabel = breaklabel = label;

        fprintf(output_file, ".L.begin.%d:\n", label);
        gen_stmt(node->then);
        fprintf(output_file, ".L.continue.%d:\n", label);
        gen_expr(node->cond);
        cmpzero(node->cond->ty);
        fprintf(output_file, "\tjne .L.begin.%d\n", label);
        fprintf(output_file, ".L.break.%d:\n", label);

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
        fprintf(output_file, ".L.begin.%d:\n", lfor);
        if (node->cond) {
            gen_expr(node->cond);
            cmpzero(node->cond->ty);
            fprintf(output_file, "\tje .L.break.%d\n", lfor);
        }
        gen_stmt(node->then);
        fprintf(output_file, ".L.continue.%d:\n", lfor);
        if (node->inc)
            gen_stmt(node->inc);
        fprintf(output_file, "\tjmp .L.begin.%d\n", lfor);
        fprintf(output_file, ".L.break.%d:\n", lfor);

        continuelabel = pastcnt;
        breaklabel = pastbrk;
        return;
    }
    if (node->kind == ND_CONTINUE) {
        if (continuelabel == 0) {
            error_tok(node->tok, "codegen: stray continue");
        }
        fprintf(output_file, "\tjmp .L.continue.%d\n", continuelabel);
        return;
    }
    if (node->kind == ND_BREAK) {
        if (breaklabel == 0) {
            error_tok(node->tok, "codegen: stray break");
        }
        fprintf(output_file, "\tjmp .L.break.%d\n", breaklabel);
        return;
    }
    if (node->kind == ND_IF) {
        gen_expr(node->cond);
        cmpzero(node->cond->ty);
        int lif = next_label();
        if (node->els) {
            fprintf(output_file, "\tje .L.els.%d\n", lif);
            gen_stmt(node->then);
            fprintf(output_file, "\tjmp .L.end.%d\n", lif);
            fprintf(output_file, ".L.els.%d:\n", lif);
            gen_stmt(node->els);
        } else {
            fprintf(output_file, "\tje .L.end.%d\n", lif);
            gen_stmt(node->then);
        }
        fprintf(output_file, ".L.end.%d:\n", lif);
        return;
    }
    if (node->kind == ND_GOTO) {
        fprintf(output_file, "\tjmp .L.label.%s.%s\n", current_fn->name,
                node->labelname);
        return;
    }
    if (node->kind == ND_LABEL) {
        fprintf(output_file, "\t.L.label.%s.%s:\n", current_fn->name,
                node->labelname);
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
    fprintf(output_file, "\t.ascii \"");
    for (int i = 0; i < len; i++) {
        char c = contents[i];
        if (iscntrl(c) || c == '\"' || c == '\\') {
            char d1 = c / 64;
            char d2 = (c % 64) / 8;
            char d3 = (c % 8);
            fprintf(output_file, "\\%d%d%d", d1, d2, d3);
        } else {
            fprintf(output_file, "%c", c);
        }
    }
    fprintf(output_file, "\"\n");
}

static void emit_bss(Program *prog) {
    fprintf(output_file, ".bss\n");
    for (Var *gv = prog->globals; gv; gv = gv->next) {
        if (gv->contents)
            continue;
        fprintf(output_file, ".align %d\n", gv->align);
        if (!gv->is_static)
            fprintf(output_file, ".globl %s\n", gv->name);
        fprintf(output_file, "%s:\n", gv->name);
        fprintf(output_file, "\t.zero %d\n", size_of(gv->ty));
    }
}

static void emit_init_data(Var *var) {
    Relocation *reloc = var->reloc;
    int pos = 0;
    while (pos < size_of(var->ty)) {
        if (reloc && reloc->offset == pos) {
            fprintf(output_file, "\t.quad %s+%ld\n", reloc->label,
                    reloc->addend);
            reloc = reloc->next;
            pos += 8;
        } else {
            fprintf(output_file, "\t.byte %d\n", var->contents[pos++]);
        }
    }
}

static void emit_data(Program *prog) {
    fprintf(output_file, ".data\n");

    for (Var *gv = prog->globals; gv; gv = gv->next) {
        if (!gv->contents)
            continue;
        fprintf(output_file, ".align %d\n", gv->align);
        if (!gv->is_static)
            fprintf(output_file, ".globl %s\n", gv->name);
        fprintf(output_file, "%s:\n", gv->name);
        if (gv->ascii)
            emit_string_literal(gv->contents, size_of(gv->ty));
        else
            emit_init_data(gv);
    }
}

static void emit_text(Program *prog) {
    fprintf(output_file, ".text\n");
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        if (!fn->is_static)
            fprintf(output_file, ".globl %s\n", fn->name);
        fprintf(output_file, "%s:\n", fn->name);
        current_fn = fn;

        // prologue
        // save stack pointer
        fprintf(output_file, "\tpush %%rbp\n");
        fprintf(output_file, "\tmov %%rsp, %%rbp\n");
        fprintf(output_file, "\tsub $%d, %%rsp\n", fn->stacksize);
        // save callee-saved registers
        fprintf(output_file, "\tmov %%r12, -8(%%rbp)\n");
        fprintf(output_file, "\tmov %%r13, -16(%%rbp)\n");
        fprintf(output_file, "\tmov %%r14, -24(%%rbp)\n");
        fprintf(output_file, "\tmov %%r15, -32(%%rbp)\n");

        if (fn->is_variadic) {
            fprintf(output_file, "\tmov %%rdi, -128(%%rbp)\n");
            fprintf(output_file, "\tmov %%rsi, -120(%%rbp)\n");
            fprintf(output_file, "\tmov %%rdx, -112(%%rbp)\n");
            fprintf(output_file, "\tmov %%rcx, -104(%%rbp)\n");
            fprintf(output_file, "\tmov %%r8, -96(%%rbp)\n");
            fprintf(output_file, "\tmov %%r9, -88(%%rbp)\n");

            fprintf(output_file, "\tmovsd %%xmm0, -80(%%rbp)\n");
            fprintf(output_file, "\tmovsd %%xmm1, -72(%%rbp)\n");
            fprintf(output_file, "\tmovsd %%xmm2, -64(%%rbp)\n");
            fprintf(output_file, "\tmovsd %%xmm3, -56(%%rbp)\n");
            fprintf(output_file, "\tmovsd %%xmm4, -48(%%rbp)\n");
            fprintf(output_file, "\tmovsd %%xmm5, -40(%%rbp)\n");
        }
        // push arguments to the stack
        int gp = 0, fp = 0;
        for (Var *v = fn->params; v; v = v->next) {
            if (is_flonum(v->ty))
                fp++;
            else
                gp++;
        }
        for (Var *v = fn->params; v; v = v->next) {
            if (v->ty->kind == TY_FLOAT) {
                fprintf(output_file, "\tmovss %%xmm%d, -%d(%%rbp)\n", --fp,
                        v->offset);
            } else if (v->ty->kind == TY_DOUBLE) {
                fprintf(output_file, "\tmovsd %%xmm%d, -%d(%%rbp)\n", --fp,
                        v->offset);
            } else {
                fprintf(output_file, "\tmov %s, -%d(%%rbp)\n",
                        argreg(size_of(v->ty), --gp), v->offset);
            }
        }
        for (Node *n = fn->node; n; n = n->next) {
            gen_stmt(n);
            if (top != 0)
                error("top: %d\n", top);
        }

        // Epilogue
        // recover callee-saved registers
        fprintf(output_file, ".L.return.%s:\n", current_fn->name);
        fprintf(output_file, "\tmov -8(%%rbp), %%r12\n");
        fprintf(output_file, "\tmov -16(%%rbp), %%r13\n");
        fprintf(output_file, "\tmov -24(%%rbp), %%r14\n");
        fprintf(output_file, "\tmov -32(%%rbp), %%r15\n");
        // recover stack pointer
        fprintf(output_file, "\tmov %%rbp, %%rsp\n");
        fprintf(output_file, "\tpop %%rbp\n");
        fprintf(output_file, "\tret\n");
    }
}

void codegen(Program *prog) {
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        // calle-saved registers take 32 bytes
        // and variable-argument save area takes 8 * 6 * 2 = 96 bytes.
        int offset = fn->is_variadic ? 128 : 32;
        for (Var *v = fn->locals; v; v = v->next) {
            offset = align_to(offset, v->align);
            offset += v->ty->size;
            v->offset = offset;
        }
        fn->stacksize = align_to(offset, 16);
    }
    emit_bss(prog);
    emit_data(prog);
    emit_text(prog);
}
