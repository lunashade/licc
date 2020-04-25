#include "lcc.h"

//
// Codegen
//

// register
static char *reg(int idx) {
  static char *r[] = {"r10", "r11", "r12", "r13", "r14", "r15"};

  if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
    error("registor out of range: %d", idx);
  return r[idx];
}

static int top;
static char *reg_push() { return reg(top++); }
static char *reg_pop() { return reg(--top); }

// label
static int labelcnt;
static int next_label() { return ++labelcnt; }

// address
static void gen_addr(Node *node) {
  if (node->kind == ND_VAR) {
    printf("\tlea %s, [rbp-%d]\n", reg_push(), node->var->offset);
    return;
  }
  error("not an lvalue");
}

static void load(void) {
  char *rd = reg_pop();
  printf("\tmov %s, [%s]\n", reg_push(), rd);
  return;
}
static void store(void) {
  char *addr = reg_pop();
  char *val = reg_pop();
  printf("\tmov [%s], %s\n", addr, val);
  reg_push(); // address to top
  return;
}

// code generation
static void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    printf("\tmov %s, %ld\n", reg_push(), node->val);
    return;
  }
  if (node->kind == ND_VAR) {
    gen_addr(node);
    load();
    return;
  }
  if (node->kind == ND_ASSIGN) {
    gen_expr(node->rhs);
    gen_addr(node->lhs);
    store();
    return;
  }

  // binary node
  gen_expr(node->lhs);
  gen_expr(node->rhs);

  char *rs = reg_pop();
  char *rd = reg_pop();
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
    printf("\tmov rax, %s\n", rd);
    printf("\tcqo\n");
    printf("\tidiv %s\n", rs);
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_EQ) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsete al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_NE) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsetne al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_LT) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsetl al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  if (node->kind == ND_LE) {
    printf("\tcmp %s, %s\n", rd, rs);
    printf("\tsetle al\n");
    printf("\tmovzb rax, al\n");
    printf("\tmov %s, rax\n", rd);
    return;
  }
  error("invalid expression");
}

static void gen_stmt(Node *node) {
  if (node->kind == ND_RETURN) {
    gen_expr(node->lhs);
    printf("\tmov rax, %s\n", reg_pop());
    printf("\tjmp .L.return\n");
    return;
  }
  if (node->kind == ND_IF) {
    gen_expr(node->cond);
    printf("\tcmp %s, 0\n", reg_pop());
    int l = next_label();
    printf("\tje .L.end.%d\n", l);
    gen_stmt(node->then);
    printf(".L.end.%d:\n", l);
    return;
  }
  if (node->kind == ND_EXPR_STMT) {
    gen_expr(node->lhs);
    reg_pop();
    return;
  }
  error("invalid statement");
}

void codegen(Function *prog) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  printf("main:\n");

  // prologue
  // save stack pointer
  printf("\tpush rbp\n");
  printf("\tmov rbp, rsp\n");
  printf("\tsub rsp, %d\n", prog->stacksize);
  // save callee-saved registers
  printf("\tmov [rbp-8], r12\n");
  printf("\tmov [rbp-16], r13\n");
  printf("\tmov [rbp-24], r14\n");
  printf("\tmov [rbp-32], r15\n");

  for (Node *n = prog->node; n; n = n->next) {
    gen_stmt(n);
    assert(top == 0);
  }

  // Epilogue
  // recover callee-saved registers
  printf(".L.return:\n");
  printf("\tmov r12, [rbp-8]\n");
  printf("\tmov r13, [rbp-16]\n");
  printf("\tmov r14, [rbp-24]\n");
  printf("\tmov r15, [rbp-32]\n");
  // recover stack pointer
  printf("\tmov rsp, rbp\n");
  printf("\tpop rbp\n");
  printf("\tret\n");
}
