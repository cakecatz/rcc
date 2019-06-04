#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define tokenValue(p, key) (*(Token *)tokens->data[p]).key

// token values
enum TK {
  TK_NUM = 256, // integer
  TK_EOF,
  TK_EQ,
  TK_NE,
  TK_LTE,
  TK_GTE,
  TK_INC,
  TK_MUL,
  TK_DIV,
  TK_RBR,
  TK_LBR,
  TK_DEC
};

// node values
enum ND {
  ND_NUM = 256,
  ND_LT,  // <
  ND_LTE, // <=
  ND_GT,  // >
  ND_GTE, // >=
  ND_EQ,  // ==
  ND_NE,  // !=
};

// token type
typedef struct Token {
  int ty;      // type
  int val;     // value
  char *input; // token string for error message
} Token;

typedef struct Node {
  int ty;
  struct Node *lhs;
  struct Node *rhs;
  int val;
} Node;

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Node *expr();
Node *mul();
Node *term();
Node *unary();
Node *equality();
Node *relational();
Node *add();
Vector *new_vector();
void runtest();

Vector *new_vector() {
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void *) * 16);
  vec->capacity = 16;
  vec->len = 0;
  return vec;
}

void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }

  vec->data[vec->len++] = elem;
}

Vector *tokens;
int pos = 0;
char *user_input;

void t(int i) { printf("{ty: %d},", (*(Token *)tokens->data[i]).ty); }

void debug_tokens() {
  printf("\n[");
  for (int i = 0; i < tokens->len; i++) {
    t(i);
  }
  printf("]\n");
}

void error_at(char *loc, char *msg) {
  fprintf(stderr, "%s\n", user_input);
  // fprintf(stderr, "%*s", input_pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ %s\n", msg);
  exit(1);
}

int consume(int ty) {
  if (((Token *)tokens->data[pos])->ty != ty) {
    return 0;
  }

  pos++;
  return 1;
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume(TK_EQ)) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume(TK_NE)) {
      node = new_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume(TK_LTE)) {
      node = new_node(ND_LTE, node, add());
    } else if (consume(TK_GTE)) {
      node = new_node(ND_GTE, node, add());
    } else if (consume('<')) {
      node = new_node(ND_LT, node, add());
    } else if (consume('>')) {
      node = new_node(ND_GT, node, add());
    } else {
      return node;
    }
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume(TK_INC)) {
      node = new_node('+', node, mul());
    } else if (consume(TK_DEC)) {
      node = new_node('-', node, mul());
    } else {
      return node;
    }
  }
}

Node *unary() {
  if (consume(TK_INC)) {
    return term();
  }

  if (consume(TK_DEC)) {
    return new_node('-', new_node_num(0), term());
  }

  return term();
}

Node *term() {
  if (consume(TK_RBR)) {
    Node *node = expr();
    if (!consume(TK_LBR)) {
      error_at(((Token *)tokens->data[pos])->input,
               "開きカッコに対応する閉じカッコがありません");
    }
    return node;
  }

  if ((*(Token *)tokens->data[pos]).ty == TK_NUM) {
    return new_node_num(((Token *)tokens->data[pos++])->val);
  }

  error_at(((Token *)tokens->data[pos])->input,
           "数値でも開きカッコでもないトークンです");
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume(TK_MUL)) {
      node = new_node('*', node, unary());
    } else if (consume(TK_DIV)) {
      node = new_node('/', node, unary());
    } else {
      return node;
    }
  }
}

Node *expr() { return equality(); }

void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  imul rdi\n");
    break;
  case '/':
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LTE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GT:
    printf("  cmp rdi, rax\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GTE:
    printf("  cmp rdi, rax\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

void add_token(int ty, char *p) {
  Token *t = malloc(sizeof(Token));
  t->ty = ty;
  t->input = p;
  vec_push(tokens, t);
}

void tokenize(char *p) {
  int i = 0;
  int tk = -1;
  int len = 0;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strncmp(p, "==", 2) == 0) {
      add_token(TK_EQ, p);
      i++;
      p += 2;
      continue;
    }

    if (strncmp(p, "!=", 2) == 0) {
      add_token(TK_NE, p);
      i++;
      p += 2;
      continue;
    }

    if (strncmp(p, "<=", 2) == 0) {
      add_token(TK_LTE, p);
      i++;
      p += 2;
      continue;
    }

    if (strncmp(p, ">=", 2) == 0) {
      add_token(TK_GTE, p);
      i++;
      p += 2;
      continue;
    }

    if (*p == '<') {
      add_token('<', p);
      i++;
      p++;
      continue;
    }

    if (*p == '>') {
      add_token('>', p);
      i++;
      p++;
      continue;
    }

    if (*p == '+') {
      add_token(TK_INC, p);
      i++;
      p++;
      continue;
    }

    if (*p == '-') {
      add_token(TK_DEC, p);
      i++;
      p++;
      continue;
    }

    if (*p == '*') {
      add_token(TK_MUL, p);
      i++;
      p++;
      continue;
    }

    if (*p == '/') {
      add_token(TK_DIV, p);
      i++;
      p++;
      continue;
    }

    if (*p == '(') {
      add_token(TK_RBR, p);
      i++;
      p++;
      continue;
    }

    if (*p == ')') {
      add_token(TK_LBR, p);
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      Token *t = malloc(sizeof(Token));
      t->ty = TK_NUM;
      t->input = p;
      t->val = strtol(p, &p, 10);
      vec_push(tokens, t);
      i++;
      continue;
    }

    error_at(p, "cannot tokenize");
  }

  add_token(TK_EOF, p);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  user_input = argv[1];
  tokens = new_vector();
  tokenize(user_input);

  Node *node = expr();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");

  return 0;
}

// testing

void expect(int line, int expected, int actual) {
  if (expected == actual) {
    return;
  }

  fprintf(stderr, "Line %d: \e[92m%d\e[0m expected, but got \e[91m%d\e[0m\n",
          line, expected, actual);
  exit(1);
}

void runtest() {
  Vector *vec = new_vector();
  expect(__LINE__, 0, vec->len);

  for (int i = 0; i < 100; i++) {
    vec_push(vec, (void *)i);
  }

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, (long)vec->data[0]);
  expect(__LINE__, 50, (long)vec->data[50]);
  expect(__LINE__, 99, (long)vec->data[99]);

  Token t = {.ty = '>'};
  vec_push(vec, &t);
  expect(__LINE__, '>', ((Token *)vec->data[100])->ty);

  printf("OK\n");
}
