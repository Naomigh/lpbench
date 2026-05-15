#include "bench/aligner_api.hpp"

#include <cstdlib>

#ifdef BENCH_ENABLE_LV89
extern "C" {
#include "lv89.h"
}
#endif

namespace bench {

#ifdef BENCH_ENABLE_LV89
namespace {
class Lv89Aligner final : public Aligner {
 public:
  std::string name() const override { return "lv89"; }
  bool supports(Mode mode) const override { return mode == Mode::Levenshtein; }
  std::string notes() const override { return "backend_actual=lv89;mode=global;bandwidth=threshold"; }
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
    int32_t score = 0, t_end = 0, q_end = 0, n_cigar = 0;
    uint32_t* cigar = lv_ed_basic(static_cast<int32_t>(reference.size()), reference.data(),
                                  static_cast<int32_t>(read.size()), read.data(),
                                  0, params.threshold, &score, &t_end, &q_end, &n_cigar);
    std::free(cigar);
    result.score = score;
    result.pass = score <= params.threshold;
    return result;
  }
};
}  // namespace
#endif

std::unique_ptr<Aligner> make_native_lv89() {
#ifdef BENCH_ENABLE_LV89
  return std::make_unique<Lv89Aligner>();
#else
  return nullptr;
#endif
}

}  // namespace bench

