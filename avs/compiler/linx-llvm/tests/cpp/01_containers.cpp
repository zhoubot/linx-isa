#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

extern "C" int cpp_gate_case01(void) {
  std::vector<int> values{1, 3, 5, 7, 9, 11};
  std::transform(values.begin(), values.end(), values.begin(),
                 [](int x) { return x * 2; });

  const int sum = std::accumulate(values.begin(), values.end(), 0);
  if (sum != 72)
    return 1;

  std::string tag = "linx-cpp17";
  std::reverse(tag.begin(), tag.end());
  if (tag != "71ppc-xnil")
    return 2;

  std::uint64_t mix = 0;
  for (int v : values)
    mix = (mix << 3) ^ static_cast<std::uint64_t>(v);
  return (mix == 0) ? 3 : 0;
}
