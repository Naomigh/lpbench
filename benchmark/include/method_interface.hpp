#pragma once

#include "bench_common.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bench {

struct VerifyInput {
  const char* read;
  const char* ref;
  int length;
  int k;
};

struct VerifyResult {
  bool pass;
  int score;
};

using VerifyFn = VerifyResult (*)(const VerifyInput& input);
using MethodHook = bool (*)();
using MethodCleanup = void (*)();

struct LeapForwardTimingResult {
  uint64_t total_cpu_ns = 0;
  uint64_t pass_count = 0;
  uint64_t mismatch_count = 0;
  uint64_t sink = 0;
};


struct MethodSpec {
  std::string method;
  std::string backend;
  std::string mode;
  VerifyFn verify = nullptr;
  MethodHook available = nullptr;
  MethodHook prepare = nullptr;
  MethodCleanup cleanup = nullptr;
};

VerifyResult verify_leap_avx2(const VerifyInput& input);
VerifyResult verify_leap_avx512(const VerifyInput& input);
VerifyResult verify_lv89(const VerifyInput& input);
VerifyResult verify_miniwfa(const VerifyInput& input);
LeapForwardTimingResult run_leap_avx2_forward_timed(
    const std::vector<const PairRecord*>& rows, int k,
    const std::string& cache_mode, volatile uint64_t* sink);
LeapForwardTimingResult run_leap_avx512_forward_timed(
    const std::vector<const PairRecord*>& rows, int k,
    const std::string& cache_mode, volatile uint64_t* sink);
VerifyResult verify_wfa2_fresh(const VerifyInput& input);
VerifyResult verify_wfa2_reuse(const VerifyInput& input);

bool method_available_avx2();
bool method_available_avx512();
bool method_prepare_wfa2_reuse();
void method_cleanup_wfa2_reuse();

}  // namespace bench

