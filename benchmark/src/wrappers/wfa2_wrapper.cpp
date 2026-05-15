#include "bench/aligner_api.hpp"

#ifdef BENCH_ENABLE_WFA2
#include "bindings/cpp/WFAligner.hpp"
#endif

namespace bench {

#ifdef BENCH_ENABLE_WFA2
namespace {
class Wfa2Aligner final : public Aligner {
 public:
  std::string name() const override { return "wfa2"; }
  bool supports(Mode) const override { return true; }
  std::string notes() const override { return "backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score"; }
  AlignResult align(const std::string& reference,
                    const std::string& read,
                    const AlignParams& params) override {
    AlignResult result;
    if (params.mode == Mode::Levenshtein) {
      wfa::WFAlignerEdit aligner(wfa::WFAligner::Score, wfa::WFAligner::MemoryHigh);
      const auto status = aligner.alignEnd2End(read, reference);
      if (status < 0) {
        result.score_valid = false;
        result.pass = false;
        result.notes = "max_steps_or_failure";
      } else {
        result.score = aligner.getAlignmentScore();
        result.pass = result.score <= params.threshold;
      }
    } else {
      wfa::WFAlignerGapAffine aligner(params.mismatch, params.gap_open + params.gap_extend, params.gap_extend,
                                      wfa::WFAligner::Score, wfa::WFAligner::MemoryHigh);
      const auto status = aligner.alignEnd2End(read, reference);
      if (status < 0) {
        result.score_valid = false;
        result.pass = false;
        result.notes = "max_steps_or_failure";
      } else {
        result.score = aligner.getAlignmentScore();
        result.pass = result.score <= params.threshold;
        result.score_valid = false;
        result.notes = "score_semantics=pass_fail_only_affine_normalization_pending";
      }
    }
    return result;
  }
};
}  // namespace
#endif

std::unique_ptr<Aligner> make_native_wfa2() {
#ifdef BENCH_ENABLE_WFA2
  return std::make_unique<Wfa2Aligner>();
#else
  return nullptr;
#endif
}

}  // namespace bench
