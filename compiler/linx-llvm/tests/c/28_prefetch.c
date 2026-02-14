// Prefetch operations

void prefetch_read(void *addr) {
  __builtin_prefetch(addr, 0, 3); // Read, high temporal locality
}

void prefetch_write(void *addr) {
  __builtin_prefetch(addr, 1, 3); // Write, high temporal locality
}

void prefetch_stream(void *addr) {
  __builtin_prefetch(addr, 0, 0); // Read, no temporal locality
}

// Prefetch in loops
void prefetch_loop(int *arr, int n) {
  for (int i = 0; i < n; i++) {
    if (i + 1 < n) {
      __builtin_prefetch(&arr[i + 1], 0, 1);
    }
    arr[i] *= 2;
  }
}

// Multiple prefetches
void prefetch_multiple(void *a, void *b, void *c) {
  __builtin_prefetch(a, 0, 2);
  __builtin_prefetch(b, 0, 2);
  __builtin_prefetch(c, 0, 2);
}

// Prefetch with stride
void prefetch_stride(int *arr, int n, int stride) {
  for (int i = 0; i < n; i += stride) {
    if (i + stride < n) {
      __builtin_prefetch(&arr[i + stride], 0, 1);
    }
    arr[i] = arr[i] * 2 + 1;
  }
}
