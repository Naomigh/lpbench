#pragma once

#include <cstdint>

struct BenchLeapPhaseResult {
    uint64_t construct_init_cpu_ns = 0;
    uint64_t load_reads_cpu_ns = 0;
    uint64_t calculate_masks_cpu_ns = 0;
    uint64_t reset_cpu_ns = 0;
    uint64_t run_cpu_ns = 0;
    uint64_t check_pass_cpu_ns = 0;
    int pass = 0;
    int score = -1;
};

extern "C" BenchLeapPhaseResult bench_leap_avx2_phase_once(
    const char* read, const char* ref, int length, int k);
extern "C" BenchLeapPhaseResult bench_leap_avx512_phase_once(
    const char* read, const char* ref, int length, int k);
