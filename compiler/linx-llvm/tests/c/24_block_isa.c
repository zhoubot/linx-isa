// Block ISA operations - testing BSTART/BSTOP/SETC patterns

// Simple block with conditional execution
int block_conditional(int x, int y) {
  int result;
  if (x < y) {
    result = x + y;
  } else {
    result = x - y;
  }
  return result;
}

// Nested blocks
int nested_blocks(int a, int b, int c) {
  int result = 0;
  if (a > 0) {
    if (b > 0) {
      result = a + b;
    } else {
      result = a - b;
    }
    if (c > 0) {
      result += c;
    }
  }
  return result;
}

// Loop with blocks
int loop_with_blocks(int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    if (i % 2 == 0) {
      sum += i;
    } else {
      sum -= i;
    }
  }
  return sum;
}

// Function calls (should use BSTART.CALL)
int helper(int x) { return x * 2; }

int call_test(int x) {
  int a = helper(x);
  int b = helper(x + 1);
  return a + b;
}

// Switch statement (multiple blocks)
int switch_test(int x) {
  switch (x) {
  case 0:
    return 10;
  case 1:
    return 20;
  case 2:
    return 30;
  default:
    return 0;
  }
}

// Early return (block termination)
int early_return(int x) {
  if (x < 0)
    return -1;
  if (x == 0)
    return 0;
  return x * 2;
}

// Complex control flow
int complex_control(int a, int b, int c) {
  int result = 0;
  while (a > 0) {
    if (b > c) {
      result += a;
      if (a % 2 == 0) {
        break;
      }
    } else {
      result -= a;
    }
    a--;
  }
  return result;
}
