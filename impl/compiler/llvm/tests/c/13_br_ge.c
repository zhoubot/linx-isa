__attribute__((noinline)) int foo(int x) { return x + 1; }
__attribute__((noinline)) int bar(int x) { return x - 1; }

// Force a signed >= conditional branch (B.GE) via explicit CFG.
__attribute__((noinline)) int ge_branch(int a, int b) {
  if (a >= b)
    goto ge;
  return bar(b);
ge:
  return foo(a);
}

__attribute__((noinline)) unsigned ufoo(unsigned x) { return x + 1u; }
__attribute__((noinline)) unsigned ubar(unsigned x) { return x - 1u; }

// Force an unsigned >= conditional branch (B.GEU) via explicit CFG.
__attribute__((noinline)) unsigned geu_branch(unsigned a, unsigned b) {
  if (a >= b)
    goto ge;
  return ubar(b);
ge:
  return ufoo(a);
}

