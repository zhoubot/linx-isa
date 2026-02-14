// Minimal soft-float stubs for linx32 compiler bring-up.
//
// The linx32 backend is still missing legalization/isel coverage for many 64-bit
// integer operations used by the full soft-fp runtime. The compiler compile-only
// tests only need these symbols to link so relocations can be resolved before
// extracting raw `.bin` files.
//
// These implementations are intentionally simplistic and are *not* suitable for
// running real code.

float __addsf3(float a, float b) {
  (void)b;
  return a;
}

float __subsf3(float a, float b) {
  (void)b;
  return a;
}

float __mulsf3(float a, float b) {
  (void)b;
  return a;
}

float __divsf3(float a, float b) {
  (void)b;
  return a;
}

double __adddf3(double a, double b) {
  (void)b;
  return a;
}

double __subdf3(double a, double b) {
  (void)b;
  return a;
}

double __muldf3(double a, double b) {
  (void)b;
  return a;
}

double __divdf3(double a, double b) {
  (void)b;
  return a;
}

int __ltsf2(float a, float b) {
  (void)a;
  (void)b;
  return 0;
}

int __gtsf2(float a, float b) {
  (void)a;
  (void)b;
  return 0;
}

int __lesf2(float a, float b) {
  (void)a;
  (void)b;
  return 0;
}

int __gesf2(float a, float b) {
  (void)a;
  (void)b;
  return 0;
}

int __eqsf2(float a, float b) {
  (void)a;
  (void)b;
  return 0;
}

int __nesf2(float a, float b) {
  (void)a;
  (void)b;
  return 0;
}

int __ltdf2(double a, double b) {
  (void)a;
  (void)b;
  return 0;
}

int __gtdf2(double a, double b) {
  (void)a;
  (void)b;
  return 0;
}

int __ledf2(double a, double b) {
  (void)a;
  (void)b;
  return 0;
}

int __gedf2(double a, double b) {
  (void)a;
  (void)b;
  return 0;
}

int __eqdf2(double a, double b) {
  (void)a;
  (void)b;
  return 0;
}

int __nedf2(double a, double b) {
  (void)a;
  (void)b;
  return 0;
}

int __fixsfsi(float a) {
  (void)a;
  return 0;
}

float __floatsisf(int a) {
  (void)a;
  return 0.0f;
}

double __extendsfdf2(float a) {
  (void)a;
  return 0.0;
}

float __truncdfsf2(double a) {
  (void)a;
  return 0.0f;
}

