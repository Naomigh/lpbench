#pragma once

#include <cstdint>
#include <stdexcept>
#include <time.h>

namespace bench {

inline uint64_t thread_cpu_time_ns() {
    struct timespec ts {};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) {
        throw std::runtime_error("clock_gettime(CLOCK_THREAD_CPUTIME_ID) failed");
    }
    return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}

}  // namespace bench
