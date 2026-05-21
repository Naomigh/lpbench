#pragma once

#include "bench_common.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bench {

struct LeapPhaseRow {
  std::string backend;
  int length = 0;
  int k = 0;
  int true_ed_group = 0;
  uint64_t pair_count = 0;
  int chunk_size = 0;
  int chunk_count_per_pair = 0;
  uint64_t construct_init_cpu_ns = 0;
  uint64_t load_reads_cpu_ns = 0;
  uint64_t calculate_masks_cpu_ns = 0;
  uint64_t reset_cpu_ns = 0;
  uint64_t run_cpu_ns = 0;
  uint64_t check_pass_cpu_ns = 0;
  uint64_t sink_cpu_ns = 0;
  uint64_t seed = 0;
};

std::vector<LeapPhaseRow> run_leap_phase_avx2(
    const std::vector<const PairRecord*>& rows, const std::vector<int>& ks,
    int true_ed_min, int true_ed_max, uint64_t seed);
std::vector<LeapPhaseRow> run_leap_phase_avx512(
    const std::vector<const PairRecord*>& rows, const std::vector<int>& ks,
    int true_ed_min, int true_ed_max, uint64_t seed);

}  // namespace bench

