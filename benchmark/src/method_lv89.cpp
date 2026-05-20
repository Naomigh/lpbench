#include "bench_common.hpp"
#include "method_interface.hpp"

#include <cstdlib>
#include <string_view>

#if BENCH_HAS_LV89
#include "lv89.h"
#endif

namespace bench {

VerifyResult verify_lv89(const VerifyInput& input) {
#if BENCH_HAS_LV89
    int32_t score = -1;
    int32_t t_endl = 0;
    int32_t q_endl = 0;
    lv_ed_basic(input.length, input.ref, input.length, input.read, 0, input.k,
                &score, &t_endl, &q_endl, nullptr);
    return {score <= input.k, score};
#else
    std::string_view read(input.read, static_cast<std::size_t>(input.length));
    std::string_view ref(input.ref, static_cast<std::size_t>(input.length));
    int score = trusted_edit_distance_banded(read, ref, input.k);
    return {score <= input.k, score};
#endif
}

}  // namespace bench
