// Floating-point arithmetic operations

float add_f32(float a, float b) { return a + b; }

float sub_f32(float a, float b) { return a - b; }

float mul_f32(float a, float b) { return a * b; }

float div_f32(float a, float b) { return a / b; }

float fma_f32(float a, float b, float c) { return a * b + c; }

double add_f64(double a, double b) { return a + b; }

double sub_f64(double a, double b) { return a - b; }

double mul_f64(double a, double b) { return a * b; }

double div_f64(double a, double b) { return a / b; }

// Floating-point comparisons
int cmp_f32(float a, float b) {
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

int cmp_f64(double a, double b) {
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

// Floating-point conversions
int f32_to_i32(float f) { return (int)f; }

float i32_to_f32(int i) { return (float)i; }

double f32_to_f64(float f) { return (double)f; }

float f64_to_f32(double d) { return (float)d; }
