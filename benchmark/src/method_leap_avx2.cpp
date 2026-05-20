#include "bench_common.hpp"
#include "method_interface.hpp"

#include <stdexcept>
#include <string_view>

namespace bench {

namespace {

extern "C" int bench_leap_avx2_verify(const char* read, const char* ref, int length, int k);

bool cpu_has_avx2() {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2");
#else
    return false;
#endif
}

VerifyResult verify_chunked_dp(const VerifyInput& input, int chunk_size) {
    if (!cpu_has_avx2()) {
        throw std::runtime_error("leap_avx2 requested but AVX2 is not available on this CPU");
    }
    bool pass = true;
    int score_sum = 0;
    for (const auto& [offset, n] : chunks_for_length(input.length, chunk_size)) {
#if BENCH_HAS_LEAP_AVX2
        int score = bench_leap_avx2_verify(input.read + offset, input.ref + offset, n, input.k);
#else
        std::string_view read(input.read + offset, static_cast<std::size_t>(n));
        std::string_view ref(input.ref + offset, static_cast<std::size_t>(n));
        int score = trusted_edit_distance_banded(read, ref, input.k);
#endif
        score_sum += score;
        if (score > input.k) {
            pass = false;
        }
    }
    return {pass, score_sum};
}

}  // namespace

VerifyResult verify_leap_avx2(const VerifyInput& input) {
    return verify_chunked_dp(input, 256);
}

}  // namespace bench
