#include "bench_common.hpp"
#include "miniwfa.h"

#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
  mwf_opt_t opt;
  mwf_opt_init(&opt);
  opt.flag = MWF_F_NO_KALLOC;
  opt.x = 1;
  opt.o1 = 0;
  opt.e1 = 1;
  opt.o2 = 0;
  opt.e2 = 1;
  opt.step = 0;
  opt.max_s = 1000000;
  opt.max_iter = 0;

  return run_wrapper_main(argc, argv, "miniwfa", "exact",
                          [&opt](const std::string& read, const std::string& reference, int) {
                            mwf_rst_t rst;
                            std::memset(&rst, 0, sizeof(rst));
                            mwf_wfa_exact(nullptr, &opt, static_cast<int32_t>(reference.size()),
                                          reference.c_str(), static_cast<int32_t>(read.size()),
                                          read.c_str(), &rst);
                            std::free(rst.cigar);
                            return static_cast<int>(rst.s);
                          });
}
