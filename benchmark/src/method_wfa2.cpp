#include "method_interface.hpp"

extern "C" {
#include "wavefront/wavefront_align.h"
}

namespace bench {
namespace {

wavefront_aligner_attr_t make_attrs() {
  wavefront_aligner_attr_t attributes = wavefront_aligner_attr_default;
  attributes.distance_metric = edit;
  attributes.alignment_scope = compute_score;
  attributes.alignment_form.span = alignment_end2end;
  attributes.memory_mode = wavefront_memory_high;
  attributes.system.max_num_threads = 1;
  attributes.system.verbose = 0;
  return attributes;
}

wavefront_aligner_t* g_reuse_aligner = nullptr;

int run_wfa2(wavefront_aligner_t* aligner, const VerifyInput& input,
             bool reap_first) {
  if (reap_first) wavefront_aligner_reap(aligner);
  wavefront_align(aligner, const_cast<char*>(input.read), input.length,
                  const_cast<char*>(input.ref), input.length);
  return aligner->cigar->score;
}

}  // namespace

VerifyResult verify_wfa2_fresh(const VerifyInput& input) {
  auto attributes = make_attrs();
  wavefront_aligner_t* aligner = wavefront_aligner_new(&attributes);
  const int score = run_wfa2(aligner, input, false);
  wavefront_aligner_delete(aligner);
  return {score <= input.k, score};
}

bool method_prepare_wfa2_reuse() {
  if (!g_reuse_aligner) {
    auto attributes = make_attrs();
    g_reuse_aligner = wavefront_aligner_new(&attributes);
  }
  return g_reuse_aligner != nullptr;
}

void method_cleanup_wfa2_reuse() {
  if (g_reuse_aligner) {
    wavefront_aligner_delete(g_reuse_aligner);
    g_reuse_aligner = nullptr;
  }
}

VerifyResult verify_wfa2_reuse(const VerifyInput& input) {
  if (!g_reuse_aligner) method_prepare_wfa2_reuse();
  const int score = run_wfa2(g_reuse_aligner, input, true);
  return {score <= input.k, score};
}

}  // namespace bench

