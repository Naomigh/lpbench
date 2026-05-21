#include "method_interface.hpp"

extern "C" {
#include "lv89.h"
}

namespace bench {

VerifyResult verify_lv89(const VerifyInput& input) {
  int32_t score = input.k + 1;
  int32_t t_end = 0;
  int32_t q_end = 0;
  lv_ed_basic(input.length, input.ref, input.length, input.read, 0, input.k,
              &score, &t_end, &q_end, nullptr);
  return {score <= input.k, score};
}

}  // namespace bench

