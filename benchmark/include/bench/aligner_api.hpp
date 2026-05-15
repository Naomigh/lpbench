#ifndef BENCH_ALIGNER_API_HPP
#define BENCH_ALIGNER_API_HPP

#include <memory>
#include <string>
#include <vector>

namespace bench {

enum class Mode {
  Levenshtein,
  Affine,
};

struct AlignParams {
  Mode mode = Mode::Levenshtein;
  int threshold = 0;
  int mismatch = 2;
  int gap_open = 3;
  int gap_extend = 1;
};

struct AlignResult {
  bool supported = true;
  bool pass = false;
  bool score_valid = true;
  int score = 0;
  bool cigar_valid = false;
  bool cigar_replay_ok = true;
  std::string notes;
};

class Aligner {
 public:
  virtual ~Aligner() = default;
  virtual std::string name() const = 0;
  virtual bool supports(Mode mode) const = 0;
  virtual std::string notes() const = 0;
  virtual AlignResult align(const std::string& reference,
                            const std::string& read,
                            const AlignParams& params) = 0;
};

std::unique_ptr<Aligner> make_aligner(const std::string& method);
std::vector<std::string> supported_method_names();
std::string mode_name(Mode mode);
Mode parse_mode(const std::string& mode);

}  // namespace bench

#endif

