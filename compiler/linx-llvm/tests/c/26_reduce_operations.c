// Reduce operations with register accumulation

// Sum reduction
long sum_reduce(long *arr, int n) {
  long sum = 0;
  for (int i = 0; i < n; i++) {
    sum += arr[i];
  }
  return sum;
}

// Product reduction
long product_reduce(long *arr, int n) {
  long prod = 1;
  for (int i = 0; i < n; i++) {
    prod *= arr[i];
  }
  return prod;
}

// Max reduction
long max_reduce(long *arr, int n) {
  long max_val = arr[0];
  for (int i = 1; i < n; i++) {
    if (arr[i] > max_val) {
      max_val = arr[i];
    }
  }
  return max_val;
}

// Min reduction
long min_reduce(long *arr, int n) {
  long min_val = arr[0];
  for (int i = 1; i < n; i++) {
    if (arr[i] < min_val) {
      min_val = arr[i];
    }
  }
  return min_val;
}

// And reduction
unsigned and_reduce(unsigned *arr, int n) {
  unsigned result = ~0u;
  for (int i = 0; i < n; i++) {
    result &= arr[i];
  }
  return result;
}

// Or reduction
unsigned or_reduce(unsigned *arr, int n) {
  unsigned result = 0;
  for (int i = 0; i < n; i++) {
    result |= arr[i];
  }
  return result;
}

// Xor reduction
unsigned xor_reduce(unsigned *arr, int n) {
  unsigned result = 0;
  for (int i = 0; i < n; i++) {
    result ^= arr[i];
  }
  return result;
}

// Dot product (sum of products)
long dot_product(long *a, long *b, int n) {
  long sum = 0;
  for (int i = 0; i < n; i++) {
    sum += a[i] * b[i];
  }
  return sum;
}
