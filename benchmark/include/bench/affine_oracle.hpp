#ifndef BENCH_AFFINE_ORACLE_HPP
#define BENCH_AFFINE_ORACLE_HPP

#include <string>

namespace bench {

int affine_gap_score(const std::string& reference,
                     const std::string& read,
                     int mismatch,
                     int gap_open,
                     int gap_extend);

}  // namespace bench

#endif

