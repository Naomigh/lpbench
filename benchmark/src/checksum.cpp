#include "bench/checksum.hpp"

#include <iomanip>
#include <sstream>

namespace bench {

void Checksum::add(std::uint64_t x) {
  value ^= x;
  value *= 1099511628211ull;
}

void Checksum::add_result(int score, bool score_valid, bool pass, std::size_t read_length) {
  add(score_valid ? static_cast<std::uint64_t>(score + 0x9e3779b9) : 0xabcdefu);
  add(pass ? 1000003u : 17u);
  add(static_cast<std::uint64_t>(read_length));
}

std::string Checksum::hex() const {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

}  // namespace bench

