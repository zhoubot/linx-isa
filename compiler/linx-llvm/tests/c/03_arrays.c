long dot_i32(const int *a, const int *b, int n) {
  long acc = 0;
  for (int i = 0; i < n; ++i)
    acc += (long)a[i] * (long)b[i];
  return acc;
}

void fill_i32(int *a, int n, int v) {
  for (int i = 0; i < n; ++i)
    a[i] = v + i;
}

long local_array_sum(int n) {
  int buf[32];
  if (n > 32)
    n = 32;

  for (int i = 0; i < n; ++i)
    buf[i] = i * 3;

  long s = 0;
  for (int i = 0; i < n; ++i)
    s += buf[i];
  return s;
}
