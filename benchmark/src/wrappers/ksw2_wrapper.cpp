#include "bench/aligner_api.hpp"

#include <array>
#include <cstdlib>
#include <vector>

#ifdef BENCH_ENABLE_KSW2
extern "C" {
#include "ksw2.h"
}
#endif

namespace bench {

#ifdef BENCH_ENABLE_KSW2
namespace {
std::vector<uint8_t> encode_dna(const std::string& seq) {
  std::vector<uint8_t> out(seq.size());
  for (std::size_t i = 0; i < seq.size(); ++i) {
    switch (seq[i]) {
      case 'A': out[i] = 0; break;
      case 'C': out[i] = 1; break;
      case 'G': out[i] = 2; break;
      case 'T': out[i] = 3; break;
      default: out[i] = 4; break;
    }
  }
  return out;
}

class Ksw2Aligner final : public Aligner {
 public:
  std::string name() const override { return "ksw2"; }
  bool supports(Mode) const override { return true; }
  std::string notes() const override { return "backend_actual=ksw2;mode=global;score_negated"; }
  AlignResult align(const std::string& reference,
                    const std::string& read,
                    const AlignParams& params) override {
    const auto query = encode_dna(read);
    const auto target = encode_dna(reference);
    std::array<int8_t, 25> mat{};
    const int mismatch = params.mode == Mode::Levenshtein ? 1 : params.mismatch;
    for (int i = 0; i < 5; ++i) {
      for (int j = 0; j < 5; ++j) mat[i * 5 + j] = (i == j && i < 4) ? 0 : static_cast<int8_t>(-mismatch);
    }
    const int gap_open = params.mode == Mode::Levenshtein ? 0 : params.gap_open;
    const int gap_extend = params.mode == Mode::Levenshtein ? 1 : params.gap_extend;
    int m_cigar = 0, n_cigar = 0;
    uint32_t* cigar = nullptr;
    const int raw = ksw_gg2(nullptr, static_cast<int>(query.size()), query.data(),
                            static_cast<int>(target.size()), target.data(),
                            5, mat.data(), static_cast<int8_t>(gap_open),
                            static_cast<int8_t>(gap_extend), -1,
                            &m_cigar, &n_cigar, &cigar);
    std::free(cigar);
    AlignResult result;
    result.score = -raw;
    result.pass = result.score <= params.threshold;
    return result;
  }
};
}  // namespace
#endif

std::unique_ptr<Aligner> make_native_ksw2() {
#ifdef BENCH_ENABLE_KSW2
  return std::make_unique<Ksw2Aligner>();
#else
  return nullptr;
#endif
}

}  // namespace bench

