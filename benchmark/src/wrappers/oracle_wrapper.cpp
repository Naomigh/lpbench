#include "bench/affine_oracle.hpp"
#include "bench/aligner_api.hpp"
#include "bench/edit_oracle.hpp"

#include <algorithm>
#include <stdexcept>

namespace bench {
namespace {

class OracleAligner final : public Aligner {
 public:
  explicit OracleAligner(std::string method) : method_(std::move(method)) {}

  std::string name() const override { return method_; }
  bool supports(Mode mode) const override {
    if (method_ == "edlib" || method_ == "lv89") return mode == Mode::Levenshtein;
    return true;
  }
  std::string notes() const override { return "backend_actual=oracle_fallback"; }

  AlignResult align(const std::string& reference,
                    const std::string& read,
                    const AlignParams& params) override {
    AlignResult result;
    result.notes = "backend_actual=oracle_fallback";
    if (!supports(params.mode)) {
      result.supported = false;
      result.score_valid = false;
      result.notes = "unsupported_mode";
      return result;
    }
    result.score = params.mode == Mode::Levenshtein
                       ? levenshtein_distance(reference, read)
                       : affine_gap_score(reference, read, params.mismatch, params.gap_open, params.gap_extend);
    result.pass = result.score <= params.threshold;
    return result;
  }

 private:
  std::string method_;
};

std::unique_ptr<Aligner> make_oracle_fallback(const std::string& method) {
  return std::make_unique<OracleAligner>(method);
}

}  // namespace

std::string mode_name(Mode mode) {
  return mode == Mode::Levenshtein ? "levenshtein" : "affine";
}

Mode parse_mode(const std::string& mode) {
  if (mode == "levenshtein") return Mode::Levenshtein;
  if (mode == "affine") return Mode::Affine;
  throw std::runtime_error("unknown mode: " + mode);
}

std::vector<std::string> supported_method_names() {
  return {"leap", "edlib", "lv89", "miniwfa", "wfa2", "ksw2", "parasail"};
}

std::unique_ptr<Aligner> make_native_leap();
std::unique_ptr<Aligner> make_native_edlib();
std::unique_ptr<Aligner> make_native_lv89();
std::unique_ptr<Aligner> make_native_miniwfa();
std::unique_ptr<Aligner> make_native_wfa2();
std::unique_ptr<Aligner> make_native_ksw2();
std::unique_ptr<Aligner> make_native_parasail();

std::unique_ptr<Aligner> make_aligner(const std::string& method) {
  if (std::find(supported_method_names().begin(), supported_method_names().end(), method) ==
      supported_method_names().end()) {
    return nullptr;
  }
  std::unique_ptr<Aligner> native;
  if (method == "leap") native = make_native_leap();
  else if (method == "edlib") native = make_native_edlib();
  else if (method == "lv89") native = make_native_lv89();
  else if (method == "miniwfa") native = make_native_miniwfa();
  else if (method == "wfa2") native = make_native_wfa2();
  else if (method == "ksw2") native = make_native_ksw2();
  else if (method == "parasail") native = make_native_parasail();
  return native ? std::move(native) : make_oracle_fallback(method);
}

}  // namespace bench

