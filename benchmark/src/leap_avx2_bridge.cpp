#include <stdexcept>
#include <string_view>
#include <x86intrin.h>

#include "leap_phase_interface.hpp"
#include "leap_raw_warm_kernel_interface.hpp"
#include "timing.hpp"

#define SIMD_ED SIMD_ED_AVX2
#include "SIMD_ED.h"
#undef SIMD_ED

extern "C" {
volatile uint64_t bench_leap_raw_touch_sink = 0;
}

namespace {

inline void touch_read_ref(const char* read, const char* ref, int len) {
    uint64_t s = 0;
    for (int i = 0; i < len; ++i) {
        s += static_cast<unsigned char>(read[i]);
        s += static_cast<unsigned char>(ref[i]);
    }
    bench_leap_raw_touch_sink += s;
    asm volatile("" ::: "memory");
}

inline uint64_t rdtscp_lfence() {
    unsigned aux = 0;
    _mm_lfence();
    uint64_t t = __rdtscp(&aux);
    _mm_lfence();
    return t;
}

}  // namespace

extern "C" BenchLeapPhaseResult bench_leap_avx2_phase_once(
    const char* read, const char* ref, int length, int k) {
    BenchLeapPhaseResult result;

    uint64_t start = bench::thread_cpu_time_ns();
    SIMD_ED_AVX2 ed;
    ed.init_levenshtein(k, ED_GLOBAL, false);
    uint64_t end = bench::thread_cpu_time_ns();
    result.construct_init_cpu_ns = end - start;

    start = bench::thread_cpu_time_ns();
    ed.load_reads(const_cast<char*>(read), const_cast<char*>(ref), length);
    end = bench::thread_cpu_time_ns();
    result.load_reads_cpu_ns = end - start;

    start = bench::thread_cpu_time_ns();
    ed.calculate_masks();
    end = bench::thread_cpu_time_ns();
    result.calculate_masks_cpu_ns = end - start;

    start = bench::thread_cpu_time_ns();
    ed.reset();
    end = bench::thread_cpu_time_ns();
    result.reset_cpu_ns = end - start;

    start = bench::thread_cpu_time_ns();
    ed.run();
    end = bench::thread_cpu_time_ns();
    result.run_cpu_ns = end - start;

    start = bench::thread_cpu_time_ns();
    result.pass = ed.check_pass() ? 1 : 0;
    result.score = result.pass ? ed.get_ED() : k + 1;
    end = bench::thread_cpu_time_ns();
    result.check_pass_cpu_ns = end - start;
    return result;
}

extern "C" BenchLeapRawWarmResult bench_leap_avx2_raw_warm_kernel_once(
    const char* read, const char* ref, int length, int k, int repeat, int do_warmup) {
    BenchLeapRawWarmResult result;
    result.chunk_count = 1;

    SIMD_ED_AVX2 ed;
    ed.init_levenshtein(k, ED_GLOBAL, false);

    touch_read_ref(read, ref, length);

    if (do_warmup) {
        ed.load_reads(const_cast<char*>(read), const_cast<char*>(ref), length);
        ed.calculate_masks();
        ed.reset();
        ed.run();
    }

    uint64_t start_cycles = rdtscp_lfence();
    uint64_t start = bench::thread_cpu_time_ns();
    for (int r = 0; r < repeat; ++r) {
        ed.load_reads(const_cast<char*>(read), const_cast<char*>(ref), length);
        ed.calculate_masks();
        ed.reset();
        ed.run();
        asm volatile("" ::: "memory");
    }
    uint64_t end = bench::thread_cpu_time_ns();
    uint64_t end_cycles = rdtscp_lfence();

    result.total_cpu_ns = end - start;
    result.total_cycles = end_cycles - start_cycles;
    result.pass = ed.check_pass() ? 1 : 0;
    return result;
}

extern "C" int bench_leap_avx2_verify(const char* read, const char* ref, int length, int k) {
    auto result = bench_leap_avx2_phase_once(read, ref, length, k);
    return result.score;
}
