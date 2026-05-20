#pragma once

#include <cstdint>

struct BenchLeapRawWarmResult {
    uint64_t total_cpu_ns = 0;
    uint64_t total_cycles = 0;
    int pass = 0;
    int chunk_count = 0;
};

extern "C" volatile uint64_t bench_leap_raw_touch_sink;

extern "C" BenchLeapRawWarmResult bench_leap_avx2_raw_warm_kernel_once(
    const char* read, const char* ref, int length, int k, int repeat, int do_warmup);

extern "C" BenchLeapRawWarmResult bench_leap_avx512_raw_warm_kernel_once(
    const char* read, const char* ref, int length, int k, int repeat, int do_warmup);
