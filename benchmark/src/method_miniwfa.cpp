#include "method_interface.hpp"

#include <cstdlib>
#include <cstring>

extern "C" {
#include "miniwfa.h"
}

namespace bench {

VerifyResult verify_miniwfa(const VerifyInput& input) {
  mwf_opt_t opt;
  mwf_opt_init(&opt);
  opt.flag = 0;
  opt.x = 1;
  opt.o1 = 0;
  opt.e1 = 1;
  opt.o2 = 0;
  opt.e2 = 1;
  opt.max_s = input.k;
  mwf_rst_t rst;
  std::memset(&rst, 0, sizeof(rst));
  mwf_wfa_exact(nullptr, &opt, input.length, input.ref, input.length, input.read,
                &rst);
  if (rst.cigar) std::free(rst.cigar);
  return {rst.s >= 0 && rst.s <= input.k, rst.s};
}

}  // namespace bench

