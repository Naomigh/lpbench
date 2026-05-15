#ifndef BENCH_EDIT_ORACLE_HPP
#define BENCH_EDIT_ORACLE_HPP

#include <string>

namespace bench {

int levenshtein_distance(const std::string& reference, const std::string& read);

}  // namespace bench

#endif

