#include "bench_common.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace bench {

int trusted_edit_distance(std::string_view a, std::string_view b) {
    const int n = static_cast<int>(a.size());
    const int m = static_cast<int>(b.size());
    std::vector<int> prev(static_cast<std::size_t>(m + 1));
    std::vector<int> curr(static_cast<std::size_t>(m + 1));
    for (int j = 0; j <= m; ++j) {
        prev[static_cast<std::size_t>(j)] = j;
    }
    for (int i = 1; i <= n; ++i) {
        curr[0] = i;
        for (int j = 1; j <= m; ++j) {
            int sub = prev[static_cast<std::size_t>(j - 1)] + (a[static_cast<std::size_t>(i - 1)] == b[static_cast<std::size_t>(j - 1)] ? 0 : 1);
            int del = prev[static_cast<std::size_t>(j)] + 1;
            int ins = curr[static_cast<std::size_t>(j - 1)] + 1;
            curr[static_cast<std::size_t>(j)] = std::min({sub, del, ins});
        }
        prev.swap(curr);
    }
    return prev[static_cast<std::size_t>(m)];
}

int trusted_edit_distance_banded(std::string_view a, std::string_view b, int max_distance) {
    if (max_distance < 0) {
        return trusted_edit_distance(a, b);
    }
    const int n = static_cast<int>(a.size());
    const int m = static_cast<int>(b.size());
    if (std::abs(n - m) > max_distance) {
        return max_distance + 1;
    }

    const int inf = std::numeric_limits<int>::max() / 4;
    std::vector<int> prev(static_cast<std::size_t>(m + 1), inf);
    std::vector<int> curr(static_cast<std::size_t>(m + 1), inf);

    const int init_hi = std::min(m, max_distance);
    for (int j = 0; j <= init_hi; ++j) {
        prev[static_cast<std::size_t>(j)] = j;
    }

    for (int i = 1; i <= n; ++i) {
        std::fill(curr.begin(), curr.end(), inf);
        const int lo = std::max(1, i - max_distance);
        const int hi = std::min(m, i + max_distance);
        if (i <= max_distance) {
            curr[0] = i;
        }
        int row_min = curr[0];
        for (int j = lo; j <= hi; ++j) {
            int sub = prev[static_cast<std::size_t>(j - 1)] + (a[static_cast<std::size_t>(i - 1)] == b[static_cast<std::size_t>(j - 1)] ? 0 : 1);
            int del = prev[static_cast<std::size_t>(j)] + 1;
            int ins = curr[static_cast<std::size_t>(j - 1)] + 1;
            int best = std::min({sub, del, ins});
            curr[static_cast<std::size_t>(j)] = best;
            row_min = std::min(row_min, best);
        }
        if (row_min > max_distance) {
            return max_distance + 1;
        }
        prev.swap(curr);
    }

    int score = prev[static_cast<std::size_t>(m)];
    return score > max_distance ? max_distance + 1 : score;
}

}  // namespace bench
