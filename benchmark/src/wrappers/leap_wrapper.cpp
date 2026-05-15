#include "bench/affine_oracle.hpp"
#include "bench/aligner_api.hpp"
#include "bench/edit_oracle.hpp"

#ifdef BENCH_ENABLE_LEAP
#include "SIMD_ED.h"
#endif

namespace bench {

#ifdef BENCH_ENABLE_LEAP
namespace {
class LeapAligner final : public Aligner {
 public:
  std::string name() const override { return "leap"; }
  bool supports(Mode) const override { return true; }
  std::string notes() const override { return "backend_actual=avx512;implementation=../leap/SIMD_ED"; }
  AlignResult align(const std::string& reference,
                    const std::string& read,
                    const AlignParams& params) override {
    AlignResult result;
    if (reference.size() != read.size() || read.size() > 512) {
      result.notes = "backend_actual=oracle_fallback;reason=leap_native_requires_equal_length_le_512";
      result.score = params.mode == Mode::Levenshtein
                         ? levenshtein_distance(reference, read)
                         : affine_gap_score(reference, read, params.mismatch, params.gap_open, params.gap_extend);
      result.pass = result.score <= params.threshold;
      return result;
    }
    std::string ref = reference;
    std::string query = read;
    SIMD_ED aligner;
    if (params.mode == Mode::Levenshtein) {
      aligner.init_levenshtein(params.threshold, ED_GLOBAL, false);
    } else {
      aligner.init_affine(params.threshold, params.threshold, ED_GLOBAL,
                          params.mismatch, params.gap_open, params.gap_extend, false, 0);
    }
    aligner.load_reads(query.data(), ref.data(), static_cast<int>(query.size()));
    aligner.calculate_masks();
    aligner.reset();
    aligner.run();
    result.pass = aligner.check_pass();
    if (result.pass) {
      result.score = aligner.get_ED();
      result.score_valid = false;
      result.notes = "score_semantics=pass_fail_only_until_cigar_replay";
    } else {
      result.score = -1;
      result.score_valid = false;
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
