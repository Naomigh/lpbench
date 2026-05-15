#include "bench/aligner_api.hpp"

#ifdef BENCH_ENABLE_EDLIB
#include "edlib.h"
#endif

namespace bench {

#ifdef BENCH_ENABLE_EDLIB
namespace {
class EdlibAligner final : public Aligner {
 public:
  std::string name() const override { return "edlib"; }
  bool supports(Mode mode) const override { return mode == Mode::Levenshtein; }
  std::string notes() const override { return "backend_actual=edlib;score_semantics=exact_if_within_threshold"; }
  AlignResult align(const std::string& reference,
                    const std::string& read,
                    const AlignParams& params) override {
    AlignResult result;
    if (!supports(params.mode)) {
      result.supported = false;
      result.score_valid = false;
      result.notes = "unsupported_mode";
      return result;
    }
    EdlibAlignConfig config = edlibNewAlignConfig(
        params.threshold, EDLIB_MODE_NW, EDLIB_TASK_DISTANCE, nullptr, 0);
    EdlibAlignResult r = edlibAlign(read.data(), static_cast<int>(read.size()),
                                    reference.data(), static_cast<int>(reference.size()),
                                    config);
    if (r.status != EDLIB_STATUS_OK) {
      result.supported = false;
      result.score_valid = false;
      result.notes = "edlib_status_error";
    } else if (r.editDistance < 0) {
      result.pass = false;
      result.score_valid = false;
      result.score = -1;
    } else {
      result.score = r.editDistance;
      result.pass = r.editDistance <= params.threshold;
    }
    edlibFreeAlignResult(r);
    return result;
  }
};
}  // namespace
#endif

std::unique_ptr<Aligner> make_native_edlib() {
#ifdef BENCH_ENABLE_EDLIB
  return std::make_unique<EdlibAligner>();
#else
  return nullptr;
#endif
}

}  // namespace bench

