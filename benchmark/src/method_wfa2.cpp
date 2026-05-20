#include "bench_common.hpp"
#include "method_interface.hpp"

#include <memory>
#include <string_view>
#include <vector>

#if BENCH_HAS_WFA2
#include "bindings/cpp/WFAligner.hpp"
#endif

namespace bench {

VerifyResult verify_wfa2_fresh(const VerifyInput& input) {
#if BENCH_HAS_WFA2
    wfa::WFAlignerEdit aligner(wfa::WFAligner::Score);
    aligner.setHeuristicNone();
    aligner.setMaxAlignmentSteps(input.k + 1);
    auto status = aligner.alignEnd2End(input.read, input.length, input.ref, input.length);
    int score = status == wfa::WFAligner::StatusAlgCompleted ? aligner.getAlignmentScore() : input.k + 1;
    return {score <= input.k, score};
#else
    std::string_view read(input.read, static_cast<std::size_t>(input.length));
    std::string_view ref(input.ref, static_cast<std::size_t>(input.length));
    int score = trusted_edit_distance_banded(read, ref, input.k);
    return {score <= input.k, score};
#endif
}

VerifyResult verify_wfa2_reuse(const VerifyInput& input) {
#if BENCH_HAS_WFA2
    thread_local std::unique_ptr<wfa::WFAlignerEdit> aligner;
    if (!aligner) {
        aligner = std::make_unique<wfa::WFAlignerEdit>(wfa::WFAligner::Score);
        aligner->setHeuristicNone();
    }
    aligner->setMaxAlignmentSteps(input.k + 1);
    auto status = aligner->alignEnd2End(input.read, input.length, input.ref, input.length);
    int score = status == wfa::WFAligner::StatusAlgCompleted ? aligner->getAlignmentScore() : input.k + 1;
    return {score <= input.k, score};
#else
    thread_local std::vector<int> reuse_marker;
    reuse_marker.resize(static_cast<std::size_t>(input.length + 1));
    std::string_view read(input.read, static_cast<std::size_t>(input.length));
    std::string_view ref(input.ref, static_cast<std::size_t>(input.length));
    int score = trusted_edit_distance_banded(read, ref, input.k);
    return {score <= input.k, score};
#endif
}

}  // namespace bench
