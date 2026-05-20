#pragma once

#include <string>
#include <vector>

namespace bench {

struct VerifyInput {
    const char* read = nullptr;
    const char* ref = nullptr;
    int length = 0;
    int k = 0;
};

struct VerifyResult {
    bool pass = false;
    int score = -1;
};

using VerifyFn = VerifyResult (*)(const VerifyInput& input);

struct MethodSpec {
    std::string method;
    std::string backend;
    std::string mode;
    VerifyFn verify = nullptr;
};

VerifyResult verify_leap_avx2(const VerifyInput& input);
VerifyResult verify_leap_avx512(const VerifyInput& input);
VerifyResult verify_lv89(const VerifyInput& input);
VerifyResult verify_miniwfa(const VerifyInput& input);
VerifyResult verify_wfa2_fresh(const VerifyInput& input);
VerifyResult verify_wfa2_reuse(const VerifyInput& input);

inline std::vector<MethodSpec> all_methods() {
    return {
        {"leap_avx2", "avx2", "default", verify_leap_avx2},
        {"leap_avx512", "avx512", "default", verify_leap_avx512},
        {"lv89", "lv89", "default", verify_lv89},
        {"miniwfa", "miniwfa", "default", verify_miniwfa},
        {"wfa2_fresh", "wfa2", "fresh", verify_wfa2_fresh},
        {"wfa2_reuse", "wfa2", "reuse", verify_wfa2_reuse},
    };
}

}  // namespace bench
