#include "bench_common.hpp"
#include "method_interface.hpp"

#include <string_view>

#if BENCH_HAS_MINIWFA
#include "miniwfa.h"
#endif

namespace bench {

VerifyResult verify_miniwfa(const VerifyInput& input) {
#if BENCH_HAS_MINIWFA
    mwf_opt_t opt;
    mwf_rst_t result{};
    mwf_opt_init(&opt);
    opt.flag = 0;
    opt.x = 1;
    opt.o1 = 0;
    opt.e1 = 1;
    opt.o2 = 0;
    opt.e2 = 1;
    opt.max_s = input.k;
    mwf_wfa_exact(nullptr, &opt, input.length, input.ref, input.length, input.read, &result);
    int score = result.s < 0 ? input.k + 1 : result.s;
    return {score <= input.k, score};
#else
    std::string_view read(input.read, static_cast<std::size_t>(input.length));
    std::string_view ref(input.ref, static_cast<std::size_t>(input.length));
    int score = trusted_edit_distance_banded(read, ref, input.k);
    return {score <= input.k, score};
#endif
}

}  // namespace bench
