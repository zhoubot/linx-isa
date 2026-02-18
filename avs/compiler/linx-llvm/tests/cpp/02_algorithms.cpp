#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>

template <typename T, std::size_t N>
static T dot(const std::array<T, N> &a, const std::array<T, N> &b) {
  return std::inner_product(a.begin(), a.end(), b.begin(), static_cast<T>(0));
}

static inline std::uint32_t rotl32(std::uint32_t v, unsigned s) {
  s &= 31U;
  return (v << s) | (v >> ((32U - s) & 31U));
}

extern "C" int cpp_gate_case02(void) {
  constexpr std::array<int, 6> lhs{{2, 4, 6, 8, 10, 12}};
  constexpr std::array<int, 6> rhs{{1, 3, 5, 7, 9, 11}};
  const int d = dot(lhs, rhs);
  if (d != 322)
    return 1;

  std::uint32_t rolling = 1;
  for (int v : lhs)
    rolling = rotl32(rolling ^ static_cast<std::uint32_t>(v), 3);

  return (rolling == 0U) ? 2 : 0;
}
