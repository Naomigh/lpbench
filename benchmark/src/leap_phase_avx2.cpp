#define SIMD_ED SIMD_ED_AVX2
#include "SIMD_ED.h"
#undef SIMD_ED

#include "leap_phase_api.hpp"
#include "timing.hpp"

#include <algorithm>

namespace bench {
namespace {

volatile uint64_t g_phase_sink_avx2 = 0;

LeapPhaseRow run_group(const std::vector<const PairRecord*>& rows, int k, int ed) {
  LeapPhaseRow row;
  row.backend = "leap_avx2";
  row.length = rows.empty() ? 0 : rows.front()->length;
  row.k = k;
  row.true_ed_group = ed;
  row.pair_count = rows.size();
  row.chunk_size = 256;
  row.chunk_count_per_pair = (row.length + row.chunk_size - 1) / row.chunk_size;

  for (const auto* rec : rows) {
    bool pair_pass = true;
    for (int offset = 0; offset < rec->length; offset += row.chunk_size) {
      const int chunk_len = std::min(row.chunk_size, rec->length - offset);
      uint64_t t0 = thread_cpu_time_ns();
      SIMD_ED_AVX2 ed_obj;
      ed_obj.init_levenshtein(k, ED_GLOBAL, false);
      uint64_t t1 = thread_cpu_time_ns();
      ed_obj.load_reads(const_cast<char*>(rec->read.data() + offset),
                        const_cast<char*>(rec->reference.data() + offset),
                        chunk_len);
      uint64_t t2 = thread_cpu_time_ns();
      ed_obj.calculate_masks();
      uint64_t t3 = thread_cpu_time_ns();
      ed_obj.reset();
      uint64_t t4 = thread_cpu_time_ns();
      ed_obj.run();
      uint64_t t5 = thread_cpu_time_ns();
      const bool pass = ed_obj.check_pass();
      uint64_t t6 = thread_cpu_time_ns();
      g_phase_sink_avx2 += pass ? 1 : 0;
      uint64_t t7 = thread_cpu_time_ns();
      row.construct_init_cpu_ns += t1 - t0;
      row.load_reads_cpu_ns += t2 - t1;
      row.calculate_masks_cpu_ns += t3 - t2;
      row.reset_cpu_ns += t4 - t3;
      row.run_cpu_ns += t5 - t4;
      row.check_pass_cpu_ns += t6 - t5;
      row.sink_cpu_ns += t7 - t6;
      pair_pass = pair_pass && pass;
    }
    g_phase_sink_avx2 += pair_pass ? 1 : 0;
  }
  return row;
}

}  // namespace

std::vector<LeapPhaseRow> run_leap_phase_avx2(
    const std::vector<const PairRecord*>& rows, const std::vector<int>& ks,
    int true_ed_min, int true_ed_max, uint64_t seed) {
  std::vector<LeapPhaseRow> out;
  for (int k : ks) {
    for (int ed = true_ed_min; ed <= true_ed_max; ++ed) {
      std::vector<const PairRecord*> group;
      for (const auto* row : rows)
        if (row->true_ed == ed) group.push_back(row);
      if (group.empty()) continue;
      auto r = run_group(group, k, ed);
      r.seed = seed;
      out.push_back(r);
    }
  }
  return out;
}

}  // namespace bench

