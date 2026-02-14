__attribute__((noinline)) int dense_switch(int x) {
  // Keep cases dense so the compiler prefers a jump table at -O2 (when enabled).
  switch (x) {
  case 0:
    return 11;
  case 1:
    return 22;
  case 2:
    return 33;
  case 3:
    return 44;
  case 4:
    return 55;
  case 5:
    return 66;
  case 6:
    return 77;
  case 7:
    return 88;
  case 8:
    return 99;
  case 9:
    return 111;
  case 10:
    return 122;
  case 11:
    return 133;
  case 12:
    return 144;
  case 13:
    return 155;
  case 14:
    return 166;
  case 15:
    return 177;
  default:
    return -1;
  }
}

int call_dense_switch(int x) { return dense_switch(x); }

