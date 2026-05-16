#include "bench/aligner_api.hpp"

#include <cstdlib>

#ifdef BENCH_ENABLE_MINIWFA
extern "C" {
#include "miniwfa.h"
}
#endif

namespace bench {

#ifdef BENCH_ENABLE_MINIWFA
namespace {
class MiniwfaAligner final : public Aligner {
 public:
  std::string name() const override { return "miniwfa"; }
  bool supports(Mode mode) const override { return mode != Mode::LevenshteinExact; }
  std::string notes() const override { return "backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L"; }
  AlignResult align(const std::string& reference,
                    const std::string& read,
                    const AlignParams& params) override {
    mwf_opt_t opt;
    mwf_opt_init(&opt);
    opt.flag = MWF_F_NO_KALLOC;
    opt.max_s = params.threshold;
    if (params.mode == Mode::Levenshtein) {
      opt.x = 1;
      opt.o1 = 0;
      opt.e1 = 1;
      opt.o2 = opt.o1;
      opt.e2 = opt.e1;
    } else {
      opt.x = params.mismatch;
      opt.o1 = params.gap_open;
      opt.e1 = params.gap_extend;
      opt.o2 = opt.o1;
      opt.e2 = opt.e1;
    }
    mwf_rst_t rst{};
    mwf_wfa_exact(nullptr, &opt, static_cast<int32_t>(reference.size()), reference.data(),
                  static_cast<int32_t>(read.size()), read.data(), &rst);
    AlignResult result;
    if (rst.s < 0) {
      result.score_valid = false;
      result.pass = false;
      result.score = -1;
      result.notes = "max_s_exceeded_or_failed";
    } else {
      result.score = rst.s;
      result.pass = rst.s <= params.threshold;
    }
    std::free(rst.cigar);
    return result;
  }
};
}  // namespace
#endif

std::unique_ptr<Aligner> make_native_miniwfa() {
#ifdef BENCH_ENABLE_MINIWFA
  return std::make_unique<MiniwfaAligner>();
#else
  return nullptr;
#endif
}

}  // namespace bench
