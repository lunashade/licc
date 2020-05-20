#include "lcc.h"

// code generation
static void gen_addr(Node *node);
static void gen_expr(Node *node);
static void gen_stmt(Node *node);
//
// Codegen
//
static void emitf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(tempfile, fmt, ap);
}
static void emitfln(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(tempfile, fmt, ap);
    fprintf(tempfile, "\n");
}

// register
static char *argfreg64[] = {"%xmm0", "%xmm1", "%xmm2", "%xmm3",
                            "%xmm4", "%xmm5", "%xmm6", "%xmm7"};
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
        error("freg: registor out of range: %d", idx);
    return freg64[idx];
}

static char *argfreg(int idx) {
    if (idx < 0 || sizeof(argfreg64) / sizeof(*argfreg64) <= idx)
        error("freg: registor out of range: %d", idx);
    return argfreg64[idx];
}

static char *argreg(int sz, int idx) {
    if (idx < 0 || sizeof(argreg64) / sizeof(*argreg64) <= idx)
        error("argreg: registor out of range: %d", idx);
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
        error("reg: registor out of range: %d", idx);
    return reg64[idx];
}

static char *regsz(int sz, int idx) {
    if (idx < 0 || sizeof(reg64) / sizeof(*reg64) <= idx)
        error("regsz: registor out of range: %d", idx);
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
        error("regx: registor out of range: %d", idx);
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
        emitfln("\tmovsd (%s), %s", reg(top - 1), freg(top - 1));
        return;
    }
    if (ty->kind == TY_FLOAT) {
        emitfln("\tmovss (%s), %s", reg(top - 1), freg(top - 1));
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
        emitfln("\t%sbl (%s), %s", insn, rs, rd);
    } else if (sz == 2) {
        emitfln("\t%swl (%s), %s", insn, rs, rd);
    } else if (sz == 4) {
        emitfln("\tmov (%s), %s", rs, rd);
    } else {
        assert(sz == 8);
        emitfln("\tmov (%s), %s", rs, rd);
    }
    return;
}
// pop address and value, store value into address, push address
static void store(Type *ty) {
    char *rd = reg_pop();
    if (ty->kind == TY_STRUCT) {
        char *rs = reg_pop();
        for (int i = 0; i < size_of(ty); i++) {
            emitfln("\tmov %d(%s), %%al", i, rs);
            emitfln("\tmov %%al, %d(%s)", i, rd);
        }
        reg_push(); // address to top
        return;
    }
    if (is_flonum(ty)) {
        char *insn = (ty->kind == TY_FLOAT) ? "movss" : "movsd";
        char *rs = freg(--top);
        emitfln("\t%s %s, (%s)", insn, rs, rd);
        reg_push();
        return;
    }

    char *rs = reg_pop_sz(size_of(ty));
    emitfln("\tmov %s, (%s)", rs, rd);
    reg_push(); // address to top
    return;
}

// compare stack-top value to zero, push 1 or 0 to top
static void cmpzero(Type *ty) {
    if (ty->kind == TY_FLOAT) {
        emitfln("\txorpd %%xmm0, %%xmm0");
        emitfln("\tucomiss %s, %%xmm0", freg(--top));
    } else if (ty->kind == TY_DOUBLE) {
        emitfln("\txorpd %%xmm0, %%xmm0");
        emitfln("\tucomisd %s, %%xmm0", freg(--top));
    } else {
        emitfln("\tcmp $0, %s", regx(ty, --top));
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
        emitfln("\tsetne %sb", r);
        emitfln("\tmovzx %sb, %s", r, r);
        top++;
        return;
    }

    // from FLONUM
    if (from->kind == TY_FLOAT) {
        if (to->kind == TY_FLOAT)
            return;
        if (to->kind == TY_DOUBLE)
            emitfln("\tcvtss2sd %s, %s", fr, fr);
        else
            emitfln("\tcvttss2si %s, %s", fr, r);
        return;
    }
    if (from->kind == TY_DOUBLE) {
        if (to->kind == TY_DOUBLE)
            return;
        if (to->kind == TY_FLOAT)
            emitfln("\tcvtsd2ss %s, %s", fr, fr);
        else
            emitfln("\tcvttsd2si %s, %s", fr, r);
        return;
    }
    // INTEGER -> FLONUM
    if (to->kind == TY_FLOAT) {
        emitfln("\tcvtsi2ss %s, %s", r, fr);
        return;
    }
    if (to->kind == TY_DOUBLE) {
        emitfln("\tcvtsi2sd %s, %s", r, fr);
        return;
    }

    // INTEGER -> INTEGER
    char *insn = to->is_unsigned ? "movzx" : "movsx";

    if (size_of(to) == 1) {
        emitfln("\t%s %sb, %s", insn, r, r);
    } else if (size_of(to) == 2) {
        emitfln("\t%s %sw, %s", insn, r, r);
    } else if (size_of(to) == 4) {
        emitfln("\tmov %sd, %sd", r, r);
    } else if (is_integer(from) && size_of(from) < 8 && !from->is_unsigned) {
        emitfln("\tmovsx %sd, %s", r, r);
    }
}

static void builtin_va_start(Node *node) {
    int gp = 0, fp = 0;
    for (Var *var = current_fn->params; var; var = var->next)
        if (is_flonum(var->ty))
            fp++;
        else
            gp++;

    emitfln("\tmov -%d(%%rbp), %%rax", node->args[0]->offset);
    emitfln("\tmovl $%d, (%%rax)", gp * 8);
    emitfln("\tmovl $%d, 4(%%rax)", 48 + fp * 8);
    emitfln("\tmov %%rbp, 16(%%rax)");
    emitfln("\tsubq $128, 16(%%rax)");
    top++;
}

static void load_fp_arg(Type *ty, int offset, int fp) {
    // load FP to register
    if (ty->kind == TY_FLOAT) {
        emitfln("\tmovss -%d(%%rbp), %%xmm%d", offset, fp);
    } else {
        emitfln("\tmovsd -%d(%%rbp), %%xmm%d", offset, fp);
    }
}

static void load_gp_arg(Type *ty, int offset, int i) {
    int sz = size_of(ty);
    char *insn = (ty->is_unsigned) ? "movz" : "movs";
    switch (sz) {
    case 1:
        emitfln("\t%sbl -%d(%%rbp), %s", insn, offset, argregx(ty, i));
        break;
    case 2:
        emitfln("\t%swl -%d(%%rbp), %s", insn, offset, argregx(ty, i));
        break;
    case 4:
        emitfln("\tmovl -%d(%%rbp), %s", offset, argregx(ty, i));
        break;
    default:
        assert(sz == 8);
        emitfln("\tmov -%d(%%rbp), %s", offset, argregx(ty, i));
        break;
    }
}

static void push_arg(Type *ty, int offset) {
    if (is_flonum(ty)) {
        if (ty->kind == TY_FLOAT) {
            emitfln("\tmov -%d(%%rbp), %%eax", offset);
        } else {
            emitfln("\tmov -%d(%%rbp), %%rax", offset);
        }
    } else {
        int sz = size_of(ty);
        char *insn = (ty->is_unsigned) ? "movz" : "movs";
        switch (sz) {
        case 1:
            emitfln("\t%sbl -%d(%%rbp), %%eax", insn, offset);
            break;
        case 2:
            emitfln("\t%swl -%d(%%rbp), %%eax", insn, offset);
            break;
        case 4:
            emitfln("\tmovl -%d(%%rbp), %%eax", offset);
            break;
        default:
            assert(sz == 8);
            emitfln("\tmov -%d(%%rbp), %%rax", offset);
            break;
        }
    }

    emitfln("\tpush %%rax");
}

// Load function call arguments.
// x86-64 ABI summary by rui314:
// - Up to 6 arguments of integral type are passed using:
//   RDI, RSI, RDX, RCX, R8 and R9
// - Up to 8 arguments of floating-point type are passed using:
//   XMM0 - XMM7
// - If all registers of an appropriate type are already used,
//   push an argument to the stack in the right-to-left order.
// - Each argument passed on stack takes 8 bytes, and the end of
//   the argument area must be aligned to a 16 byte boundary.
// - If function is variadic, set the number of FP type arguments to RSP.
static int load_args(Node *node) {
    int gp = 0;
    int fp = 0;
    int stacksize = 0;
    bool *pass_stack = calloc(node->nargs, sizeof(bool));

    for (int i = 0; i < node->nargs; i++) {
        Var *arg = node->args[i];

        // pass by register
        if (is_flonum(arg->ty)) {
            if (fp < 8) {
                load_fp_arg(arg->ty, arg->offset, fp++);
                continue;
            }
        } else {
            if (gp < 6) {
                load_gp_arg(arg->ty, arg->offset, gp++);
                continue;
            }
        }
        // else, prepare the stack
        pass_stack[i] = true;
        stacksize += 8;
    }

    // align
    if (stacksize && stacksize % 16) {
        emitfln("\tsub $8, %%rsp");
        stacksize += 8;
    }
    // push argument in right-to-left order
    for (int i = node->nargs - 1; i >= 0; i--) {
        if (!pass_stack[i])
            continue;
        Var *arg = node->args[i];
        push_arg(arg->ty, arg->offset);
    }

    // always store the number of FP arguments to RAX.
    int n = 0;
    for (int i = 0; i < node->nargs; i++) {
        if (is_flonum(node->args[i]->ty))
            n++;
    }
    emitfln("\tmov $%d, %%rax", n);

    return stacksize;
}

static void gen_funcall(Node *node) {
    if (node->lhs->kind == ND_VAR &&
        !strcmp(node->lhs->var->name, "__builtin_va_start")) {
        builtin_va_start(node);
        return;
    }

    emitfln("\tsub $64, %%rsp");
    emitfln("\tmov %%r10, (%%rsp)");
    emitfln("\tmov %%r11, 8(%%rsp)");
    emitfln("\tmovsd %%xmm8, 16(%%rsp)");
    emitfln("\tmovsd %%xmm9, 24(%%rsp)");
    emitfln("\tmovsd %%xmm10, 32(%%rsp)");
    emitfln("\tmovsd %%xmm11, 40(%%rsp)");
    emitfln("\tmovsd %%xmm12, 48(%%rsp)");
    emitfln("\tmovsd %%xmm13, 56(%%rsp)");

    gen_expr(node->lhs);
    int memarg_size = load_args(node);

    emitfln("\tcall *%s", reg(--top));
    if (memarg_size)
        emitfln("\tsub $%d, %%rsp", memarg_size);

    if (node->ty->kind == TY_BOOL)
        emitfln("\tmovzx %%al, %%eax");

    emitfln("\tmov (%%rsp), %%r10");
    emitfln("\tmov 8(%%rsp), %%r11");
    emitfln("\tmovsd 16(%%rsp), %%xmm8");
    emitfln("\tmovsd 24(%%rsp), %%xmm9");
    emitfln("\tmovsd 32(%%rsp), %%xmm10");
    emitfln("\tmovsd 40(%%rsp), %%xmm11");
    emitfln("\tmovsd 48(%%rsp), %%xmm12");
    emitfln("\tmovsd 56(%%rsp), %%xmm13");
    emitfln("\tadd $64, %%rsp");

    if (node->ty->kind == TY_FLOAT) {
        emitfln("\tmovss %%xmm0, %s", freg(top++));
    } else if (node->ty->kind == TY_DOUBLE) {
        emitfln("\tmovsd %%xmm0, %s", freg(top++));
    } else {
        emitfln("\tmov %%rax, %s", reg(top++));
    }
    return;
}

// code generate address of node
static void gen_addr(Node *node) {
    if (node->kind == ND_VAR) {
        if (node->var->is_local) {
            emitfln("\tlea %d(%%rbp), %s", -node->var->offset, reg_push());
            return;
        }
        if (!opt_fpic) {
            emitfln("\tmov $%s, %s", node->var->name, reg_push());
        } else if (node->var->is_static) {
            emitfln("\tlea %s(%%rip), %s", node->var->name, reg_push());
        } else {
            emitfln("\tmov %s@GOTPCREL(%%rip), %s", node->var->name,
                    reg_push());
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
        emitfln("\tadd $%d, %s", node->member->offset, reg_pop());
        reg_push();
        return;
    }
    error_tok(node->tok, "codegen: gen_addr: not an lvalue");
}

// code generate expression
static void gen_expr(Node *node) {
    emitfln(".loc %d %d", node->tok->fileno, node->tok->lineno);
    switch (node->kind) {
    case ND_NOP_EXPR:
        top++;
        return;
    case ND_NUM:
        switch (node->ty->kind) {
        case TY_FLOAT: {
            float fval = node->fval;
            emitfln("\tmov $%u, %%rax", *(int *)&fval);
            emitfln("\tpush %%rax");
            emitfln("\tmovss (%%rsp), %s", freg(top++));
            emitfln("\tadd $8, %%rsp");
            return;
        }
        case TY_DOUBLE: {
            emitfln("\tmov $%lu, %%rax", *(long *)&node->fval);
            emitfln("\tpush %%rax");
            emitfln("\tmovsd (%%rsp), %s", freg(top++));
            emitfln("\tadd $8, %%rsp");
            return;
        }
        case TY_LONG:
            emitfln("\tmovabs $%lu, %s", node->val, reg_push());
            return;
        default:
            emitfln("\tmov $%lu, %s", node->val, reg_push());
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
        emitfln("\tje .L.else.%d", label);
        gen_expr(node->then);
        top--;
        emitfln("\tjmp .L.end.%d", label);
        emitfln(".L.else.%d:", label);
        gen_expr(node->els);
        emitfln(".L.end.%d:", label);
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
        emitfln("\tnot %s", reg_pop());
        reg_push();
        return;
    case ND_NOT: {
        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        char *rd = reg_push();
        emitfln("\tsete %sb", rd);
        emitfln("\tmovzx %sb, %s", rd, rd);
        return;
    }
    case ND_LOGOR: {
        int label = next_label();

        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        emitfln("\tjne .L.true.%d", label);
        gen_expr(node->rhs);
        cmpzero(node->rhs->ty);
        emitfln("\tjne .L.true.%d", label);

        char *rd = reg_push();
        emitfln("\tmov $0, %s", rd);
        emitfln("\tjmp .L.end.%d", label);
        emitfln(".L.true.%d:", label);
        emitfln("\tmov $1, %s", rd);
        emitfln(".L.end.%d:", label);
        return;
    }
    case ND_LOGAND: {
        int label = next_label();

        gen_expr(node->lhs);
        cmpzero(node->lhs->ty);
        emitfln("\tje .L.false.%d", label);
        gen_expr(node->rhs);
        cmpzero(node->rhs->ty);
        emitfln("\tje .L.false.%d", label);

        char *rd = reg_push();
        emitfln("\tmov $1, %s", rd);
        emitfln("\tjmp .L.end.%d", label);
        emitfln(".L.false.%d:", label);
        emitfln("\tmov $0, %s", rd);
        emitfln(".L.end.%d:", label);
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
    case ND_FUNCALL:
        gen_funcall(node);
        return;
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
        emitfln("\tmov %s, %%rcx",
                reg(top)); // rs with 8-bit register
        emitfln("\tshl %%cl, %s", rd);
        return;
    }
    if (node->kind == ND_SHR) {
        emitfln("\tmov %s, %%rcx",
                reg(top)); // rs with 8-bit register
        if (node->ty->is_unsigned)
            emitfln("\tshr %%cl, %s", rd);
        else
            emitfln("\tsar %%cl, %s", rd);
        return;
    }
    if (node->kind == ND_ADD) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "addss" : "addsd";
            emitfln("\t%s %s, %s", insn, fs, fd);
            return;
        }
        emitfln("\tadd %s, %s", rs, rd);
        return;
    }
    if (node->kind == ND_SUB) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "subss" : "subsd";
            emitfln("\t%s %s, %s", insn, fs, fd);
            return;
        }
        emitfln("\tsub %s, %s", rs, rd);
        return;
    }
    if (node->kind == ND_MUL) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "mulss" : "mulsd";
            emitfln("\t%s %s, %s", insn, fs, fd);
            return;
        }
        emitfln("\timul %s, %s", rs, rd);
        return;
    }
    if (node->kind == ND_DIV) {
        if (is_flonum(node->ty)) {
            char *insn = (node->ty->kind == TY_FLOAT) ? "divss" : "divsd";
            emitfln("\t%s %s, %s", insn, fs, fd);
            return;
        }
        if (size_of(node->ty) == 8) {
            emitfln("\tmov %s, %%rax", rd);
            if (node->ty->is_unsigned) {
                emitfln("\tmov $0, %%rdx");
                emitfln("\tdiv %s", rs);
            } else {
                emitfln("\tcqo");
                emitfln("\tidiv %s", rs);
            }
            emitfln("\tmov %%rax, %s", rd);
        } else {
            emitfln("\tmov %s, %%eax", rd);
            if (node->ty->is_unsigned) {
                emitfln("\tmov $0, %%edx");
                emitfln("\tdiv %s", rs);
            } else {
                emitfln("\tcdq");
                emitfln("\tidiv %s", rs);
            }
            emitfln("\tmov %%eax, %s", rd);
        }
        return;
    }
    if (node->kind == ND_MOD) {
        if (size_of(node->ty) == 8) {
            emitfln("\tmov %s, %%rax", rd);
            if (node->ty->is_unsigned) {
                emitfln("\tmov $0, %%rdx");
                emitfln("\tdiv %s", rs);
            } else {
                emitfln("\tcqo");
                emitfln("\tidiv %s", rs);
            }
            emitfln("\tmov %%rdx, %s", rd);
        } else {
            emitfln("\tmov %s, %%eax", rd);
            if (node->ty->is_unsigned) {
                emitfln("\tmov $0, %%edx");
                emitfln("\tdiv %s", rs);
            } else {
                emitfln("\tcdq");
                emitfln("\tidiv %s", rs);
            }
            emitfln("\tmov %%edx, %s", rd);
        }
        return;
    }
    if (node->kind == ND_OR) {
        emitfln("\tor %s, %s", rs, rd);
        return;
    }
    if (node->kind == ND_AND) {
        emitfln("\tand %s, %s", rs, rd);
        return;
    }
    if (node->kind == ND_XOR) {
        emitfln("\txor %s, %s", rs, rd);
        return;
    }
    if (node->kind == ND_EQ) {
        if (node->lhs->ty->kind == TY_FLOAT)
            emitfln("\tucomiss %s, %s", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            emitfln("\tucomisd %s, %s", fs, fd);
        else
            emitfln("\tcmp %s, %s", rs, rd);

        emitfln("\tsete %%al");
        emitfln("\tmovzx %%al, %s", rd);
        return;
    }
    if (node->kind == ND_NE) {
        if (node->lhs->ty->kind == TY_FLOAT)
            emitfln("\tucomiss %s, %s", fs, fd);
        else if (node->lhs->ty->kind == TY_DOUBLE)
            emitfln("\tucomisd %s, %s", fs, fd);
        else
            emitfln("\tcmp %s, %s", rs, rd);
        emitfln("\tsetne %%al");
        emitfln("\tmovzx %%al, %s", rd);
        return;
    }
    if (node->kind == ND_LT) {
        if (node->lhs->ty->kind == TY_FLOAT) {
            emitfln("\tucomiss %s, %s", fs, fd);
            emitfln("\tsetb %%al");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            emitfln("\tucomisd %s, %s", fs, fd);
            emitfln("\tsetb %%al");
        } else {
            emitfln("\tcmp %s, %s", rs, rd);
            if (node->lhs->ty->is_unsigned) {
                emitfln("\tsetb %%al");
            } else {
                emitfln("\tsetl %%al");
            }
        }
        emitfln("\tmovzx %%al, %s", rd);
        return;
    }
    if (node->kind == ND_LE) {
        if (node->lhs->ty->kind == TY_FLOAT) {
            emitfln("\tucomiss %s, %s", fs, fd);
            emitfln("\tsetbe %%al");
        } else if (node->lhs->ty->kind == TY_DOUBLE) {
            emitfln("\tucomisd %s, %s", fs, fd);
            emitfln("\tsetbe %%al");
        } else {
            emitfln("\tcmp %s, %s", rs, rd);
            if (node->lhs->ty->is_unsigned) {
                emitfln("\tsetbe %%al");
            } else {
                emitfln("\tsetle %%al");
            }
        }
        emitfln("\tmovzx %%al, %s", rd);
        return;
    }
    error_tok(node->tok, "codegen: gen_expr: invalid expression");
}

static void gen_stmt(Node *node) {
    emitfln(".loc %d %d", node->tok->fileno, node->tok->lineno);
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
            emitfln("\tcmp $%ld, %s", n->val, rd);
            emitfln("\tje .L.case.%d", n->case_label);
        }
        if (node->default_case) {
            node->default_case->case_label = next_label();
            node->default_case->case_end_label = label;
            emitfln("\tjmp .L.case.%d", node->default_case->case_label);
        }

        emitfln("\tjmp .L.break.%d", label);
        gen_stmt(node->then);
        emitfln("\t.L.break.%d:", label);

        breaklabel = brk;
        return;
    }
    if (node->kind == ND_CASE) {
        emitfln(".L.case.%d:", node->case_label);
        gen_stmt(node->then);
        return;
    }
    if (node->kind == ND_RETURN) {
        if (node->lhs) {
            gen_expr(node->lhs);
            if (is_flonum(node->lhs->ty)) {
                emitfln("\tmovsd %s, %%xmm0", freg(--top));
            } else {
                emitfln("\tmov %s, %%rax", reg(--top));
            }
        }
        emitfln("\tjmp .L.return.%s", current_fn->name);
        return;
    }
    if (node->kind == ND_DO) {
        int label = next_label();
        int brk = breaklabel;
        int cnt = continuelabel;
        continuelabel = breaklabel = label;

        emitfln(".L.begin.%d:", label);
        gen_stmt(node->then);
        emitfln(".L.continue.%d:", label);
        gen_expr(node->cond);
        cmpzero(node->cond->ty);
        emitfln("\tjne .L.begin.%d", label);
        emitfln(".L.break.%d:", label);

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
        emitfln(".L.begin.%d:", lfor);
        if (node->cond) {
            gen_expr(node->cond);
            cmpzero(node->cond->ty);
            emitfln("\tje .L.break.%d", lfor);
        }
        gen_stmt(node->then);
        emitfln(".L.continue.%d:", lfor);
        if (node->inc)
            gen_stmt(node->inc);
        emitfln("\tjmp .L.begin.%d", lfor);
        emitfln(".L.break.%d:", lfor);

        continuelabel = pastcnt;
        breaklabel = pastbrk;
        return;
    }
    if (node->kind == ND_CONTINUE) {
        if (continuelabel == 0) {
            error_tok(node->tok, "codegen: stray continue");
        }
        emitfln("\tjmp .L.continue.%d", continuelabel);
        return;
    }
    if (node->kind == ND_BREAK) {
        if (breaklabel == 0) {
            error_tok(node->tok, "codegen: stray break");
        }
        emitfln("\tjmp .L.break.%d", breaklabel);
        return;
    }
    if (node->kind == ND_IF) {
        gen_expr(node->cond);
        cmpzero(node->cond->ty);
        int lif = next_label();
        if (node->els) {
            emitfln("\tje .L.els.%d", lif);
            gen_stmt(node->then);
            emitfln("\tjmp .L.end.%d", lif);
            emitfln(".L.els.%d:", lif);
            gen_stmt(node->els);
        } else {
            emitfln("\tje .L.end.%d", lif);
            gen_stmt(node->then);
        }
        emitfln(".L.end.%d:", lif);
        return;
    }
    if (node->kind == ND_GOTO) {
        emitfln("\tjmp .L.label.%s.%s", current_fn->name, node->labelname);
        return;
    }
    if (node->kind == ND_LABEL) {
        emitfln("\t.L.label.%s.%s:", current_fn->name, node->labelname);
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
    emitf("\t.ascii \"");
    for (int i = 0; i < len; i++) {
        char c = contents[i];
        if (iscntrl(c) || c == '\"' || c == '\\') {
            char d1 = c / 64;
            char d2 = (c % 64) / 8;
            char d3 = (c % 8);
            emitf("\\%d%d%d", d1, d2, d3);
        } else {
            emitf("%c", c);
        }
    }
    emitf("\"\n");
}

static void emit_bss(Program *prog) {
    emitfln(".bss");
    for (Var *gv = prog->globals; gv; gv = gv->next) {
        if (gv->contents)
            continue;
        emitfln(".align %d", gv->align);
        if (!gv->is_static)
            emitfln(".globl %s", gv->name);
        emitfln("%s:", gv->name);
        emitfln("\t.zero %d", size_of(gv->ty));
    }
}

static void emit_init_data(Var *var) {
    Relocation *reloc = var->reloc;
    int pos = 0;
    while (pos < size_of(var->ty)) {
        if (reloc && reloc->offset == pos) {
            emitfln("\t.quad %s+%ld", reloc->label, reloc->addend);
            reloc = reloc->next;
            pos += 8;
        } else {
            emitfln("\t.byte %d", var->contents[pos++]);
        }
    }
}

static void emit_data(Program *prog) {
    emitfln(".data");

    for (Var *gv = prog->globals; gv; gv = gv->next) {
        if (!gv->contents)
            continue;
        emitfln(".align %d", gv->align);
        if (!gv->is_static)
            emitfln(".globl %s", gv->name);
        emitfln("%s:", gv->name);
        if (gv->ascii)
            emit_string_literal(gv->contents, size_of(gv->ty));
        else
            emit_init_data(gv);
    }
}

static void emit_text(Program *prog) {
    emitfln(".text");
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        if (!fn->is_static)
            emitfln(".globl %s", fn->name);
        emitfln("%s:", fn->name);
        current_fn = fn;

        // prologue
        // save stack pointer
        emitfln("\tpush %%rbp");
        emitfln("\tmov %%rsp, %%rbp");
        emitfln("\tsub $%d, %%rsp", fn->stacksize);
        // save callee-saved registers
        emitfln("\tmov %%r12, -8(%%rbp)");
        emitfln("\tmov %%r13, -16(%%rbp)");
        emitfln("\tmov %%r14, -24(%%rbp)");
        emitfln("\tmov %%r15, -32(%%rbp)");

        if (fn->is_variadic) {
            emitfln("\tmov %%rdi, -128(%%rbp)");
            emitfln("\tmov %%rsi, -120(%%rbp)");
            emitfln("\tmov %%rdx, -112(%%rbp)");
            emitfln("\tmov %%rcx, -104(%%rbp)");
            emitfln("\tmov %%r8, -96(%%rbp)");
            emitfln("\tmov %%r9, -88(%%rbp)");

            emitfln("\tmovsd %%xmm0, -80(%%rbp)");
            emitfln("\tmovsd %%xmm1, -72(%%rbp)");
            emitfln("\tmovsd %%xmm2, -64(%%rbp)");
            emitfln("\tmovsd %%xmm3, -56(%%rbp)");
            emitfln("\tmovsd %%xmm4, -48(%%rbp)");
            emitfln("\tmovsd %%xmm5, -40(%%rbp)");
        }
        // push arguments to the stack
        int gp = 0, fp = 0;
        int memgp = 0, memfp = 0; // on-memory arguments
        for (Var *v = fn->params; v; v = v->next) {
            if (is_flonum(v->ty)) {
                if (fp < 8)
                    fp++;
                else
                    memfp++;
            } else {
                if (gp < 6)
                    gp++;
                else
                    memgp++;
            }
        }

        for (Var *v = fn->params; v; v = v->next) {
            if (v->reg) {
                // load SSE from register
                if (v->ty->kind == TY_FLOAT) {
                    emitfln("\tmovss %s, -%d(%%rbp)", v->reg, v->offset);
                } else if (v->ty->kind == TY_DOUBLE) {
                    emitfln("\tmovsd %s, -%d(%%rbp)", v->reg, v->offset);
                } else {
                    emitfln("\tmov %s, -%d(%%rbp)", v->reg, v->offset);
                }
            }
        }
        for (Node *n = fn->node; n; n = n->next) {
            gen_stmt(n);
            if (top != 0)
                error("top: %d", top);
        }

        /* C11 spec says in 5.1.2.2.3:
         * "reaching the } that terminates the main function
         * returns a value of 0." */
        if (!strcmp(fn->name, "main"))
            emitfln("\tmov $0, %%rax");
        // Epilogue
        // recover callee-saved registers
        emitfln(".L.return.%s:", current_fn->name);
        emitfln("\tmov -8(%%rbp), %%r12");
        emitfln("\tmov -16(%%rbp), %%r13");
        emitfln("\tmov -24(%%rbp), %%r14");
        emitfln("\tmov -32(%%rbp), %%r15");
        // recover stack pointer
        emitfln("\tmov %%rbp, %%rsp");
        emitfln("\tpop %%rbp");
        emitfln("\tret");
    }
}

void calc_lvar_offset(Function *fn) {
    // push arguments to the stack
    int gp = 0, fp = 0;
    int memgp = 0, memfp = 0; // on-memory arguments
    for (Var *v = fn->params; v; v = v->next) {
        if (is_flonum(v->ty)) {
            if (fp < 8)
                fp++;
            else
                memfp++;
        } else {
            if (gp < 6)
                gp++;
            else
                memgp++;
        }
    }

    for (Var *v = fn->params; v; v = v->next) {
        if (is_flonum(v->ty)) {
            if (memfp) {
                // load memarg
                v->offset = -(16 + 8 * (--memfp + memgp));
                continue;
            }
            v->reg = argfreg(--fp);
            continue;
        }
        // INTEGER type
        if (memgp) {
            // load memarg
            v->offset = -(16 + 8 * (--memgp + memfp));
            continue;
        }
        v->reg = argreg(size_of(v->ty), --gp);
        continue;
    }

    int offset = fn->is_variadic ? 128 : 32;

    for (Var *v = fn->locals; v; v = v->next) {
        if (v->offset != 0)
            continue;
        offset = align_to(offset, v->align);
        offset += v->ty->size;
        v->offset = offset;
    }
    fn->stacksize = align_to(offset, 16);
}

void codegen(Program *prog) {
    char **paths = get_input_files();
    for (int i = 0; paths[i]; i++)
        emitfln("\t.file %d \"%s\"", i + 1, paths[i]);

    for (Function *fn = prog->fns; fn; fn = fn->next) {
        calc_lvar_offset(fn);
    }
    emit_bss(prog);
    emit_data(prog);
    emit_text(prog);
}
