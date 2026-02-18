#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <numeric>
#include <string>
#include <sys/reboot.h>
#include <unistd.h>
#include <vector>

enum : std::uintptr_t {
  LINX_UART_BASE = 0x10000000ul,
};

static void uart_puts(const char *s) {
  while (*s)
    *reinterpret_cast<volatile unsigned char *>(LINX_UART_BASE) =
        static_cast<unsigned char>(*s++);
}

static void emit_marker(const char *s) {
  std::printf("%s\n", s);
  std::fflush(stdout);
  uart_puts(s);
  uart_puts("\n");
}

int main() {
  int cfd = ::open("/dev/console", O_RDWR);
  if (cfd >= 0) {
    (void)::dup2(cfd, STDIN_FILENO);
    (void)::dup2(cfd, STDOUT_FILENO);
    (void)::dup2(cfd, STDERR_FILENO);
    if (cfd > STDERR_FILENO)
      (void)::close(cfd);
  }

  emit_marker("MUSL_CPP17_START");

  std::vector<int> v(256);
  std::iota(v.begin(), v.end(), 1);
  std::transform(v.begin(), v.end(), v.begin(),
                 [](int x) { return x * 3 + 1; });
  const std::int64_t sum = std::accumulate(v.begin(), v.end(), std::int64_t{0});
  if (sum != 98944) {
    emit_marker("MUSL_CPP17_FAIL: vector-accumulate");
    ::sync();
    ::reboot(RB_POWER_OFF);
    return 2;
  }

  std::string s = "linx-musl-cpp17";
  std::reverse(s.begin(), s.end());
  if (s != "71ppc-lsum-xnil") {
    emit_marker("MUSL_CPP17_FAIL: string-reverse");
    ::sync();
    ::reboot(RB_POWER_OFF);
    return 3;
  }

  emit_marker("MUSL_CPP17_PASS");
  ::sync();
  ::reboot(RB_POWER_OFF);
  return 0;
}
