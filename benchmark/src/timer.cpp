#include "bench/timer.hpp"

namespace bench {

void Timer::start() { start_ = std::chrono::steady_clock::now(); }

double Timer::seconds() const {
  const auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(end - start_).count();
}

}  // namespace bench

