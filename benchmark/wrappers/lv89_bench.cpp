#include "bench_common.hpp"
#include "lv89.h"

#include <cstdlib>

int main(int argc, char** argv) {
  return run_wrapper_main(argc, argv, "lv89", "default",
                          [](const std::string& read, const std::string& reference, int max_edit) {
                            int32_t score = -1, t_end = -1, q_end = -1, n_cigar = 0;
                            uint32_t* cigar = lv_ed_basic(
                                static_cast<int32_t>(reference.size()), reference.c_str(),
                                static_cast<int32_t>(read.size()), read.c_str(), 0, max_edit,
                                &score, &t_end, &q_end, &n_cigar);
                            std::free(cigar);
                            return static_cast<int>(score);
                          });
}
