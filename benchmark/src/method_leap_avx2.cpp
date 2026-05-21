#define SIMD_ED SIMD_ED_AVX2
#include "SIMD_ED.h"
#undef SIMD_ED

#include "method_interface.hpp"
#include "timing.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

namespace bench {
namespace {

constexpr int kChunk = 256;

struct PreparedPairAVX2 {
  std::vector<std::unique_ptr<SIMD_ED_AVX2>> chunks;
};

void touch_rows_forward_avx2(const std::vector<const PairRecord*>& rows,
                             volatile uint64_t* sink) {
  uint64_t local = 0;
  for (const auto* r : rows) {
    for (char c : r->read) local += uint8_t(c);
    for (char c : r->reference) local += uint8_t(c);
  }
  *sink += local & 1u;
}

void evict_forward_avx2(volatile uint64_t* sink) {
  static std::vector<uint8_t> eviction(256ull * 1024ull * 1024ull, 1);
  uint64_t local = 0;
  for (size_t i = 0; i < eviction.size(); i += 64) local += eviction[i];
  *sink += local & 1u;
}

std::vector<PreparedPairAVX2> prepare_forward_avx2(
    const std::vector<const PairRecord*>& rows, size_t begin, size_t end, int k) {
  std::vector<PreparedPairAVX2> prepared;
  prepared.reserve(end - begin);
  for (size_t row_idx = begin; row_idx < end; ++row_idx) {
    const auto* r = rows[row_idx];
    PreparedPairAVX2 pair;
    for (int offset = 0; offset < r->length; offset += kChunk) {
      const int chunk_len = std::min(kChunk, r->length - offset);
      auto ed = std::make_unique<SIMD_ED_AVX2>();
      ed->init_levenshtein(k, ED_GLOBAL, false);
      ed->load_reads(const_cast<char*>(r->read.data() + offset),
                     const_cast<char*>(r->reference.data() + offset), chunk_len);
      ed->calculate_masks();
      ed->reset();
      pair.chunks.push_back(std::move(ed));
    }
    prepared.push_back(std::move(pair));
  }
  return prepared;
}

}  // namespace

bool method_available_avx2() { return cpu_has_avx2(); }

VerifyResult verify_leap_avx2(const VerifyInput& input) {
  bool pass = true;
  for (int offset = 0; offset < input.length; offset += kChunk) {
    const int chunk_len = std::min(kChunk, input.length - offset);
    SIMD_ED_AVX2 ed;
    ed.init_levenshtein(input.k, ED_GLOBAL, false);
    ed.load_reads(const_cast<char*>(input.read + offset),
                  const_cast<char*>(input.ref + offset), chunk_len);
    ed.calculate_masks();
    ed.reset();
    ed.run();
    if (!ed.check_pass()) {
      pass = false;
      break;
    }
  }
  return {pass, -1};
}

LeapForwardTimingResult run_leap_avx2_forward_timed(
    const std::vector<const PairRecord*>& rows, int k,
    const std::string& cache_mode, volatile uint64_t* sink) {
  if (cache_mode == "warm") {
    touch_rows_forward_avx2(rows, sink);
    for (const auto* r : rows) {
      VerifyInput input{r->read.data(), r->reference.data(), r->length, k};
      const auto result = verify_leap_avx2(input);
      *sink += result.pass ? 1 : 0;
    }
  } else if (cache_mode == "cold-ish") {
    evict_forward_avx2(sink);
  } else {
    throw std::runtime_error("unknown cache mode: " + cache_mode);
  }

  LeapForwardTimingResult stats;
  constexpr size_t kBlockRows = 8192;
  for (size_t begin = 0; begin < rows.size(); begin += kBlockRows) {
    const size_t end_row = std::min(rows.size(), begin + kBlockRows);
    auto prepared = prepare_forward_avx2(rows, begin, end_row, k);
    if (cache_mode == "cold-ish") evict_forward_avx2(sink);

    const uint64_t start = thread_cpu_time_ns();
    for (auto& pair : prepared) {
      for (auto& ed : pair.chunks) {
        ed->run();
        *sink += 1;
      }
    }
    const uint64_t end = thread_cpu_time_ns();
    stats.total_cpu_ns += end - start;

    for (size_t i = 0; i < prepared.size(); ++i) {
      bool pass = true;
      for (auto& ed : prepared[i].chunks) {
        if (!ed->check_pass()) {
          pass = false;
          break;
        }
      }
      stats.pass_count += pass ? 1 : 0;
      if (uint8_t(pass ? 1 : 0) != expected_pass_label(*rows[begin + i], k)) ++stats.mismatch_count;
    }
  }
  stats.sink = *sink;
  return stats;
}

}  // namespace bench
