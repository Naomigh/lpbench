#pragma once

#include <cstdint>
#include <ctime>

namespace bench {

inline uint64_t thread_cpu_time_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}

}  // namespace bench

