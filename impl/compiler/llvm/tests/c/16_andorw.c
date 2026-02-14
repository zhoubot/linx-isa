// Force i32 reg-reg bitwise ops (*W forms).

__attribute__((noinline, optnone)) int andw_force(int a, int b) { return a & b; }

__attribute__((noinline, optnone)) int orw_force(int a, int b) { return a | b; }

