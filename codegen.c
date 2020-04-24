#include "lcc.h"

//
// Codegen
//

static char *reg(int idx) {
  static char *r[] = {"r10", "r11", "r12", "r13", "r14", "r15"};

  if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
    error("registor out of range: %d", idx);
  return r[idx];
}

static int top;

static void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    printf("\tmov %s, %ld\n", reg(top), node->val);
    top++;
    return;
  }
  gen_expr(node->lhs);
  gen_expr(node->rhs);

  // binary node
  char *rd = reg(top - 2);
  char *rs = reg(top - 1);
  top--;  // 2-pop, 1-push

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

void codegen(Node *node) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  printf("main:\n");

  // save callee-saved registers
  printf("\tpush r12\n");
  printf("\tpush r13\n");
  printf("\tpush r14\n");
  printf("\tpush r15\n");

  gen_expr(node);
  printf("\tmov rax, %s\n", reg(top - 1));

  // recover callee-saved registers
  printf("\tpop r15\n");
  printf("\tpop r14\n");
  printf("\tpop r13\n");
  printf("\tpop r12\n");
  printf("\tret\n");
}
