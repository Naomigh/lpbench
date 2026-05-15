#include "bench/edit_oracle.hpp"

#include <algorithm>
#include <numeric>
#include <vector>

namespace bench {

int levenshtein_distance(const std::string& reference, const std::string& read) {
  const std::string* a = &reference;
  const std::string* b = &read;
  if (a->size() < b->size()) std::swap(a, b);
  std::vector<int> prev(b->size() + 1), cur(b->size() + 1);
  std::iota(prev.begin(), prev.end(), 0);
  for (std::size_t i = 1; i <= a->size(); ++i) {
    cur[0] = static_cast<int>(i);
    for (std::size_t j = 1; j <= b->size(); ++j) {
      const int sub = prev[j - 1] + ((*a)[i - 1] == (*b)[j - 1] ? 0 : 1);
      cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, sub});
    }
    std::swap(prev, cur);
  }
  return prev.back();
}

}  // namespace bench

