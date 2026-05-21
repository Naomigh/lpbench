#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bench {

struct PairRecord {
  uint64_t pair_id = 0;
  int length = 0;
  int true_ed = 0;
  std::string read;
  std::string reference;
  int num_sub = 0;
  int num_ins = 0;
  int num_del = 0;
  std::string edit_pattern;
  uint64_t seed = 0;
  uint8_t pass[6] = {};
};

inline uint8_t expected_pass_label(const PairRecord& row, int k) {
  return row.true_ed <= k ? 1 : 0;
}

int trusted_edit_distance(const char* a, int n, const char* b, int m);

inline bool cpu_has_avx512() {
#if defined(__x86_64__) || defined(__i386__)
  __builtin_cpu_init();
  return __builtin_cpu_supports("avx512f") &&
         __builtin_cpu_supports("avx512bw");
#else
  return false;
#endif
}

inline bool cpu_has_avx2() {
#if defined(__x86_64__) || defined(__i386__)
  __builtin_cpu_init();
  return __builtin_cpu_supports("avx2");
#else
  return false;
#endif
}

}  // namespace bench

