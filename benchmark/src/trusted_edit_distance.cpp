#include "bench_common.hpp"

#include <algorithm>
#include <vector>

namespace bench {

int trusted_edit_distance(const char* a, int n, const char* b, int m) {
  std::vector<int> prev(m + 1), curr(m + 1);
  for (int j = 0; j <= m; ++j) prev[j] = j;
  for (int i = 1; i <= n; ++i) {
    curr[0] = i;
    for (int j = 1; j <= m; ++j) {
      const int sub = prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
      const int del = prev[j] + 1;
      const int ins = curr[j - 1] + 1;
      curr[j] = std::min({sub, del, ins});
    }
    prev.swap(curr);
  }
  return prev[m];
}

}  // namespace bench

