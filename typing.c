#include "lcc.h"

int align_to(int n, int align) {
    return (n + align - 1) & ~(align - 1);
} // align must 2-power

Type *ty_int = &(Type){TY_INT, 4, 4};
Type *ty_char = &(Type){TY_CHAR, 1, 1};

bool is_integer(Type *ty) { return ty->kind == TY_INT || ty->kind == TY_CHAR; }
bool is_pointing(Type *ty) { return ty->base; }

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
    Type *ty = new_type(TY_ARRAY, size*(base->size), base->align);
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

Member *new_member(Type *ty) {
    Member *mem = calloc(1, sizeof(Member));
    mem->ty = ty;
    mem->name = ty->name;
    mem->size = ty->size;
    return mem;
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
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_ASSIGN:
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NUM:
    case ND_FUNCALL:
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
        node->ty = node->lhs->ty->base;
        return;
    }
}
