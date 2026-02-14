// Cache maintenance operations

// Cache flush (write-back and invalidate)
void cache_flush(void *addr, unsigned size) {
  // Simulate cache flush by touching memory
  volatile char *p = (volatile char *)addr;
  for (unsigned i = 0; i < size; i++) {
    (void)p[i];
  }
}

// Cache invalidate
void cache_invalidate(void *addr, unsigned size) {
  volatile char *p = (volatile char *)addr;
  for (unsigned i = 0; i < size; i++) {
    (void)p[i];
  }
}

// Memory barrier
void memory_barrier(void) {
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

// Synchronization operations
void sync_before_write(void *addr) {
  __atomic_thread_fence(__ATOMIC_RELEASE);
  *(volatile int *)addr = 1;
}

int sync_after_read(void *addr) {
  int val = *(volatile int *)addr;
  __atomic_thread_fence(__ATOMIC_ACQUIRE);
  return val;
}

// Cache line operations
void cache_line_flush(void *addr) {
  // Flush a cache line (typically 64 bytes)
  volatile char *p = (volatile char *)addr;
  for (int i = 0; i < 64; i++) {
    (void)p[i];
  }
}

void cache_line_invalidate(void *addr) {
  volatile char *p = (volatile char *)addr;
  for (int i = 0; i < 64; i++) {
    (void)p[i];
  }
}
