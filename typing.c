#include "lcc.h"

int align_to(int n, int align) { return (n + align - 1) & ~(align - 1); }

Type *ty_void = &(Type){TY_VOID, 1, 1};
Type *ty_bool = &(Type){TY_BOOL, 1, 1};
Type *ty_long = &(Type){TY_LONG, 8, 8};
Type *ty_int = &(Type){TY_INT, 4, 4};
Type *ty_short = &(Type){TY_SHORT, 2, 2};
Type *ty_char = &(Type){TY_CHAR, 1, 1};

bool is_integer(Type *ty) {
    return (ty->kind == TY_INT || ty->kind == TY_CHAR || ty->kind == TY_SHORT ||
            ty->kind == TY_LONG || ty->kind == TY_BOOL);
}
bool is_scalar(Type *ty) { return (is_integer(ty) || ty->kind == TY_PTR); }
bool is_pointing(Type *ty) { return ty->base; }

int size_of(Type *ty) {
    if (ty->kind == TY_VOID) {
        error_tok(ty->name, "type: void has no size");
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

Type *enum_type(void) {
    return new_type(TY_ENUM, 4, 4);
}

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

    if (size_of(ty1) == 8 || size_of(ty2) == 8) {
        return ty_long;
    }
    return ty_int;
}

static void usual_arithmetic_conversion(Node **lhs, Node **rhs) {
    Type *ty = common_type((*lhs)->ty, (*rhs)->ty);
    *lhs = new_cast(*lhs, ty);
    *rhs = new_cast(*rhs, ty);
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
    for (Node *n = node->args; n; n = n->next) {
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
        usual_arithmetic_conversion(&node->lhs, &node->rhs);
        node->ty = node->lhs->ty;
        return;
    case ND_ASSIGN:
        if (is_scalar(node->rhs->ty))
            node->rhs = new_cast(node->rhs, node->lhs->ty);
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
    case ND_DEREF:
        if (!is_pointing(node->lhs->ty)) {
            error_tok(node->tok, "type: invalid pointer dereference");
        }
        if (node->lhs->ty->base->kind == TY_VOID) {
            error_tok(node->tok, "type: dereferencing a void pointer");
        }
        node->ty = node->lhs->ty->base;
        return;
    case ND_FUNCALL: {
        node->ty = node->func_ty->return_ty;

        Type *ty = node->func_ty->params;
        Node *arg = node->args;
        Node head = {};
        Node *cur = &head;
        for (; arg; arg = arg->next) {
            if (ty) {
                cur = cur->next = new_cast(arg, ty);
                ty = ty->next;
            } else {
                cur = cur->next = arg;
            }
        }
        node->args = head.next;
        return;
    }
    }
}
