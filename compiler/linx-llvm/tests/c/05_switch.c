int switch_test(int x) {
  switch ((unsigned)x & 7u) {
  case 0:
    return 11;
  case 1:
    return -3;
  case 2:
    return 0;
  case 3:
    return 5;
  case 4:
    return x << 2;
  case 5:
    return x >> 1;
  case 6:
    return x ^ 0x1234;
  default:
    return x + 7;
  }
}
