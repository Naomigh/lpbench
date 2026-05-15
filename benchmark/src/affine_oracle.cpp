#include "bench/affine_oracle.hpp"

#include <algorithm>
#include <vector>

namespace bench {

int affine_gap_score(const std::string& reference,
                     const std::string& read,
                     int mismatch,
                     int gap_open,
                     int gap_extend) {
  const int inf = 1000000000;
  const int n = static_cast<int>(reference.size());
  const int m = static_cast<int>(read.size());
  std::vector<int> M((n + 1) * (m + 1), inf);
  std::vector<int> I((n + 1) * (m + 1), inf);
  std::vector<int> D((n + 1) * (m + 1), inf);
  auto at = [m](int i, int j) { return i * (m + 1) + j; };
  M[at(0, 0)] = 0;
  for (int j = 1; j <= m; ++j) {
    I[at(0, j)] = std::min(M[at(0, j - 1)] + gap_open + gap_extend,
                           I[at(0, j - 1)] + gap_extend);
  }
  for (int i = 1; i <= n; ++i) {
    D[at(i, 0)] = std::min(M[at(i - 1, 0)] + gap_open + gap_extend,
                           D[at(i - 1, 0)] + gap_extend);
  }
  for (int i = 1; i <= n; ++i) {
    for (int j = 1; j <= m; ++j) {
      const int sub = reference[i - 1] == read[j - 1] ? 0 : mismatch;
      M[at(i, j)] = std::min({M[at(i - 1, j - 1)], I[at(i - 1, j - 1)], D[at(i - 1, j - 1)]}) + sub;
      I[at(i, j)] = std::min(M[at(i, j - 1)] + gap_open + gap_extend,
                             I[at(i, j - 1)] + gap_extend);
      D[at(i, j)] = std::min(M[at(i - 1, j)] + gap_open + gap_extend,
                             D[at(i - 1, j)] + gap_extend);
    }
  }
  return std::min({M[at(n, m)], I[at(n, m)], D[at(n, m)]});
}

}  // namespace bench

