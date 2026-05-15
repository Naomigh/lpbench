#ifndef BENCH_TIMER_HPP
#define BENCH_TIMER_HPP

#include <chrono>

namespace bench {

class Timer {
 public:
  void start();
  double seconds() const;

 private:
  std::chrono::steady_clock::time_point start_;
};

}  // namespace bench

#endif

