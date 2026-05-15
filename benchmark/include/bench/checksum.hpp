#ifndef BENCH_CHECKSUM_HPP
#define BENCH_CHECKSUM_HPP

#include <cstdint>
#include <string>

namespace bench {

struct Checksum {
  std::uint64_t value = 1469598103934665603ull;

  void add(std::uint64_t x);
  void add_result(int score, bool score_valid, bool pass, std::size_t read_length);
  std::string hex() const;
};

}  // namespace bench

#endif

