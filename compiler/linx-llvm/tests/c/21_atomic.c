// Atomic operations
//
// Note: C11 <stdatomic.h> defines function-like macros such as `atomic_load`
// and `atomic_store`. Avoid naming test functions after these macros.

#include <stdatomic.h>

int atomic_load_fn(atomic_int *ptr) {
  return atomic_load_explicit(ptr, memory_order_acquire);
}

void atomic_store_fn(atomic_int *ptr, int val) {
  atomic_store_explicit(ptr, val, memory_order_release);
}

int atomic_exchange_fn(atomic_int *ptr, int val) {
  return atomic_exchange_explicit(ptr, val, memory_order_acq_rel);
}

int atomic_compare_exchange_fn(atomic_int *ptr, int expected, int desired) {
  atomic_compare_exchange_strong_explicit(ptr, &expected, desired,
                                           memory_order_acq_rel,
                                           memory_order_acquire);
  return expected;
}

int atomic_fetch_add_fn(atomic_int *ptr, int val) {
  return atomic_fetch_add_explicit(ptr, val, memory_order_acq_rel);
}

int atomic_fetch_sub_fn(atomic_int *ptr, int val) {
  return atomic_fetch_sub_explicit(ptr, val, memory_order_acq_rel);
}

int atomic_fetch_and_fn(atomic_int *ptr, int val) {
  return atomic_fetch_and_explicit(ptr, val, memory_order_acq_rel);
}

int atomic_fetch_or_fn(atomic_int *ptr, int val) {
  return atomic_fetch_or_explicit(ptr, val, memory_order_acq_rel);
}

int atomic_fetch_xor_fn(atomic_int *ptr, int val) {
  return atomic_fetch_xor_explicit(ptr, val, memory_order_acq_rel);
}

void atomic_fence_fn(void) {
  atomic_thread_fence(memory_order_seq_cst);
}
