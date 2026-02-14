struct S {
  int a;
  int b;
  long c;
  signed char d;
};

long struct_sum(const struct S *p) {
  return (long)p->a + (long)p->b + p->c + (long)p->d;
}

void struct_write(struct S *p, int x) {
  p->a = x;
  p->b = x + 1;
  p->c = (long)x * 1000L;
  p->d = (signed char)(x ^ 0x5A);
}
