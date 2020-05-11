#include "lcc.h"

int align_to(int n, int align) { return (n + align - 1) & ~(align - 1); }

Type *ty_void = &(Type){TY_VOID, 1, 1};
Type *ty_bool = &(Type){TY_BOOL, 1, 1};
Type *ty_long = &(Type){TY_LONG, 8, 8};
Type *ty_ulong = &(Type){TY_LONG, 8, 8, true};
Type *ty_int = &(Type){TY_INT, 4, 4};
Type *ty_uint = &(Type){TY_INT, 4, 4, true};
Type *ty_short = &(Type){TY_SHORT, 2, 2};
Type *ty_ushort = &(Type){TY_SHORT, 2, 2, true};
Type *ty_char = &(Type){TY_CHAR, 1, 1};
Type *ty_uchar = &(Type){TY_CHAR, 1, 1, true};

Type *ty_float = &(Type){TY_FLOAT, 4, 4};
Type *ty_double = &(Type){TY_DOUBLE, 8, 8};

bool is_integer(Type *ty) {
    return (ty->kind == TY_INT || ty->kind == TY_CHAR || ty->kind == TY_SHORT ||
            ty->kind == TY_LONG || ty->kind == TY_BOOL || ty->kind == TY_ENUM);
}
bool is_flonum(Type *ty) {
    return (ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE);
}
bool is_numeric(Type *ty) {
    return is_integer(ty) || is_flonum(ty);
}
bool is_scalar(Type *ty) {
    return (is_integer(ty) || is_flonum(ty) || ty->base);
}
bool is_pointing(Type *ty) { return ty->base; }

int size_of(Type *ty) {
    if (ty->kind == TY_VOID) {
        error_tok(ty->name, "type: void has no size");
    }
    if (ty->is_incomplete) {
        error_tok(ty->name, "type: incomplete type");
    }
    return ty->size;
}

Type *new_type(TypeKind kind, int size, int align) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = kind;
    ty->size = size;
    ty->align = align;
    return ty;
}

Type *pointer_to(Type *base) {
    Type *ty = new_type(TY_PTR, 8, 8);
    ty->base = base;
    return ty;
}

Type *array_of(Type *base, int size) {
    Type *ty = new_type(TY_ARRAY, size * (base->size), base->align);
    ty->base = base;
    ty->array_len = size;
    return ty;
}

Type *copy_type(Type *ty) {
    Type *ret = malloc(sizeof(Type));
    *ret = *ty;
    return ret;
}

Type *func_type(Type *return_ty) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_FUNC;
    ty->return_ty = return_ty;
    return ty;
}

Type *enum_type(void) { return new_type(TY_ENUM, 4, 4); }
Type *struct_type(void) { return new_type(TY_STRUCT, 0, 1); }

Member *new_member(Type *ty) {
    Member *mem = calloc(1, sizeof(Member));
    mem->ty = ty;
    mem->name = ty->name;
    mem->size = ty->size;
    return mem;
}

static Type *common_type(Type *ty1, Type *ty2) {
    if (ty1->base)
        return pointer_to(ty1->base);

    if (ty1->kind == TY_DOUBLE || ty2->kind == TY_DOUBLE)
        return ty_double;
    if (ty1->kind == TY_FLOAT || ty2->kind == TY_FLOAT)
        return ty_float;

    if (size_of(ty1) < 4)
        ty1 = ty_int;
    if (size_of(ty2) < 4)
        ty1 = ty_int;

    if (size_of(ty1) != size_of(ty2)) {
        return (size_of(ty1) < size_of(ty2)) ? ty2 : ty1;
    }
    if (ty2->is_unsigned)
        return ty2;
    return ty1;
}

static void usual_arithmetic_conversion(Node **lhs, Node **rhs) {
    Type *ty = common_type((*lhs)->ty, (*rhs)->ty);
    *lhs = new_cast(*lhs, ty);
    *rhs = new_cast(*rhs, ty);
}

static void assert_same_type(Type *lhs, Type *rhs, Token *tok) {
    if (is_pointing(lhs)) {
        if (!is_pointing(rhs))
            error_tok(tok, "type: must have same scalar type");
        return;
    }
    if (is_scalar(lhs)) {
        if (lhs->kind != rhs->kind)
            error_tok(tok, "type: must have same scalar type");
        return;
    }
    if (lhs != rhs)
        error_tok(tok, "type: must have same type");
    return;
}

void add_type(Node *node) {
    if (!node || node->ty)
        return;

    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->init);
    add_type(node->inc);
    add_type(node->then);
    add_type(node->els);
    for (Node *n = node->body; n; n = n->next) {
        add_type(n);
    }

    switch (node->kind) {
    case ND_NUM:
        node->ty = (node->val == (int)node->val) ? ty_int : ty_long;
        return;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_OR:
    case ND_XOR:
    case ND_AND:
        usual_arithmetic_conversion(&node->lhs, &node->rhs);
        node->ty = node->lhs->ty;
        return;
    case ND_ASSIGN:
        if (is_scalar(node->rhs->ty))
            node->rhs = new_cast(node->rhs, node->lhs->ty);
        if (node->lhs->ty->is_const && !node->is_init)
            error_tok(node->tok, "assignment to constant");
        assert_same_type(node->lhs->ty, node->rhs->ty, node->tok);
        node->ty = node->lhs->ty;
        return;
    case ND_COMMA:
        node->ty = node->rhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_LOGAND:
    case ND_LOGOR:
        usual_arithmetic_conversion(&node->lhs, &node->rhs);
        assert_same_type(node->lhs->ty, node->rhs->ty, node->tok);
        node->ty = ty_int;
        return;
    case ND_VAR:
        node->ty = node->var->ty;
        return;
    case ND_MEMBER:
        node->ty = node->member->ty;
        return;
    case ND_ADDR:
        if (node->lhs->ty->kind == TY_ARRAY) {
            node->ty = pointer_to(node->lhs->ty->base);
        } else {
            node->ty = pointer_to(node->lhs->ty);
        }
        return;
    case ND_BITNOT:
    case ND_SHL:
    case ND_SHR:
        node->ty = node->lhs->ty;
        return;
    case ND_COND: {
        if (node->then->ty->kind == TY_VOID || node->els->ty->kind == TY_VOID) {
            node->ty = ty_void;
            return;
        }
        if (is_scalar(node->then->ty) || is_scalar(node->els->ty)) {
            usual_arithmetic_conversion(&node->then, &node->els);
        }
        assert_same_type(node->then->ty, node->els->ty, node->tok);
        node->ty = node->then->ty;
        return;
    }
    case ND_NOT:
        node->ty = ty_int;
        return;
    case ND_DEREF:
        if (node->lhs->ty->kind == TY_FUNC) {
            *node = *node->lhs;
            return;
        }
        if (!is_pointing(node->lhs->ty)) {
            error_tok(node->tok, "type: invalid pointer dereference");
        }
        if (node->lhs->ty->base->kind == TY_VOID) {
            error_tok(node->tok, "type: dereferencing a void pointer");
        }
        node->ty = node->lhs->ty->base;
        return;
    case ND_STMT_EXPR: {
        Node *stmt = node->body;
        while (stmt->next)
            stmt = stmt->next;
        node->ty = stmt->lhs->ty;
        return;
    }
    }
}
