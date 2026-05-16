#include "bench/affine_oracle.hpp"
#include "bench/aligner_api.hpp"
#include "bench/edit_oracle.hpp"

#include <chrono>

#ifdef BENCH_ENABLE_LEAP
#include "SIMD_ED.h"
#endif

namespace bench {

#ifdef BENCH_ENABLE_LEAP
namespace {
std::string normalize_exact_cigar(std::string cigar) {
  for (char& c : cigar) {
    if (c == 'I') c = 'D';
    else if (c == 'D') c = 'I';
  }
  return cigar;
}

class LeapAligner final : public Aligner {
 public:
  std::string name() const override { return "leap"; }
  bool supports(Mode) const override { return true; }
  std::string notes() const override { return "backend_actual=avx512;implementation=../leap/SIMD_ED"; }
  AlignResult align(const std::string& reference,
                    const std::string& read,
                    const AlignParams& params) override {
    AlignResult result;
    if (params.mode == Mode::LevenshteinExact) {
      if (reference.size() != read.size() || read.size() > 512) {
        result.supported = false;
        result.score_valid = false;
        result.cigar_valid = false;
        result.notes = "unsupported_exact_input;reason=leap_exact_requires_equal_length_le_512";
        return result;
      }
      SIMD_ED aligner;
      auto start = std::chrono::steady_clock::now();
      aligner.init_levenshtein_exact(ED_GLOBAL);
      aligner.preprocess_levenshtein_exact(read.data(), static_cast<int>(read.size()),
                                           reference.data(), static_cast<int>(reference.size()));
      auto stop = std::chrono::steady_clock::now();
      result.preprocess_ns = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count());

      start = std::chrono::steady_clock::now();
      aligner.forward_levenshtein_until_bottom_right();
      stop = std::chrono::steady_clock::now();
      result.forward_ns = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count());

      start = std::chrono::steady_clock::now();
      aligner.traceback_levenshtein_exact();
      result.cigar = normalize_exact_cigar(aligner.get_exact_CIGAR());
      stop = std::chrono::steady_clock::now();
      result.traceback_ns = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count());

      result.total_ns = result.preprocess_ns + result.forward_ns + result.traceback_ns;
      result.score = aligner.get_score();
      result.score_valid = true;
      result.cigar_valid = !result.cigar.empty() || read.empty();
      result.notes = "score_semantics=exact_score_and_cigar";
      return result;
    }
    if (params.mode == Mode::Levenshtein && reference.size() == read.size() && read.size() <= 512) {
      SIMD_ED aligner;
      aligner.init_levenshtein_exact(ED_GLOBAL);
      aligner.preprocess_levenshtein_exact(read.data(), static_cast<int>(read.size()),
                                           reference.data(), static_cast<int>(reference.size()));
      aligner.forward_levenshtein_until_bottom_right();
      result.score = aligner.get_score();
      result.pass = result.score <= params.threshold;
      result.score_valid = true;
      result.notes = "score_semantics=exact_score";
      return result;
    }
    if (reference.size() != read.size() || read.size() > 512 || params.mode == Mode::Affine) {
      result.notes = "backend_actual=oracle_fallback;reason=leap_native_requires_equal_length_le_512";
      result.score = params.mode == Mode::Levenshtein
                         ? levenshtein_distance(reference, read)
                         : affine_gap_score(reference, read, params.mismatch, params.gap_open, params.gap_extend);
      result.pass = result.score <= params.threshold;
      return result;
    }
    return result;
  }
};
}  // namespace
#endif

std::unique_ptr<Aligner> make_native_leap() {
#ifdef BENCH_ENABLE_LEAP
  return std::make_unique<LeapAligner>();
#else
  return nullptr;
#endif
}

}  // namespace bench
