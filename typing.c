#include "lcc.h"
static Type *ty_int = &(Type){TY_INT, 8};

bool is_integer(Type *ty) { return ty->kind == TY_INT; }
bool is_pointing(Type *ty) { return ty->base; }

static Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    ty->size = 8;
    return ty;
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
    case ND_VAR:
        node->ty = ty_int;
        return;
    case ND_ADDR:
        node->ty = pointer_to(node->lhs->ty);
        return;
    case ND_DEREF:
        if (node->lhs->ty->kind == TY_PTR) {
            node->ty = node->lhs->ty->base;
        } else {
            node->ty = ty_int;
        }
        return;
    default:
        return;
    }
}
