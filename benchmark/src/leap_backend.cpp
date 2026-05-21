#include "leap_backend_api.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "SIMD_ED.h"

#ifndef LEAP_CHUNK_SIZE
#error "LEAP_CHUNK_SIZE must be defined for a LEAP backend"
#endif

namespace {

constexpr size_t kPreparedObjectLimit = 65536;

uint64_t thread_cpu_time_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}

struct PreparedRun {
  std::unique_ptr<SIMD_ED> ed;
};

void prepare_pair_chunks(
    const char* read,
    const char* reference,
    size_t length,
    int k,
    std::vector<PreparedRun>* prepared) {
  for (size_t begin = 0; begin < length; begin += LEAP_CHUNK_SIZE) {
    const size_t chunk_length = std::min<size_t>(LEAP_CHUNK_SIZE, length - begin);
    auto ed = std::make_unique<SIMD_ED>();
    ed->init_levenshtein(k, ED_GLOBAL, false);
    ed->load_reads(
        const_cast<char*>(read + begin),
        const_cast<char*>(reference + begin),
        static_cast<int>(chunk_length));
    ed->calculate_masks();
    ed->reset();
    prepared->push_back({std::move(ed)});
  }
}

void run_prepared(std::vector<PreparedRun>* prepared) {
  for (PreparedRun& one : *prepared) {
    one.ed->run();
  }
}

void consume_prepared(
    std::vector<PreparedRun>* prepared,
    int k,
    leap_backend_result_t* result) {
  for (PreparedRun& one : *prepared) {
    const bool pass = one.ed->check_pass();
    result->pass_sink += pass ? 1 : 0;
    const int score = pass ? 0 : k + 1;
    result->score_sink += static_cast<uint64_t>(std::max(0, score));
  }
  result->run_calls += prepared->size();
}

void run_untimed_pass(
    const char* const* reads,
    const char* const* references,
    const size_t* lengths,
    size_t dataset_count,
    size_t pair_calls,
    int k) {
  std::vector<PreparedRun> prepared;
  prepared.reserve(kPreparedObjectLimit);
  for (size_t pair_index = 0; pair_index < pair_calls;) {
    prepared.clear();
    while (pair_index < pair_calls && prepared.size() < kPreparedObjectLimit) {
      const size_t data_index = pair_index % dataset_count;
      prepare_pair_chunks(
          reads[data_index], references[data_index], lengths[data_index], k, &prepared);
      ++pair_index;
    }
    run_prepared(&prepared);
  }
}

}  // namespace

extern "C" int leap_backend_measure(
    const char* const* reads,
    const char* const* references,
    const size_t* lengths,
    size_t dataset_count,
    size_t pair_calls,
    int k,
    int warmup_passes,
    leap_backend_result_t* result) {
  if (result == nullptr) {
    return 2;
  }
  *result = {};
  if (reads == nullptr || references == nullptr || lengths == nullptr ||
      dataset_count == 0 || pair_calls == 0 || k < 0) {
    result->status = 2;
    std::snprintf(result->message, sizeof(result->message), "invalid LEAP input");
    return result->status;
  }

  for (int pass = 0; pass < warmup_passes; ++pass) {
    run_untimed_pass(reads, references, lengths, dataset_count, pair_calls, k);
  }

  std::vector<PreparedRun> prepared;
  prepared.reserve(kPreparedObjectLimit);
  for (size_t pair_index = 0; pair_index < pair_calls;) {
    prepared.clear();
    while (pair_index < pair_calls && prepared.size() < kPreparedObjectLimit) {
      const size_t data_index = pair_index % dataset_count;
      prepare_pair_chunks(
          reads[data_index], references[data_index], lengths[data_index], k, &prepared);
      ++pair_index;
    }
    const uint64_t begin_ns = thread_cpu_time_ns();
    run_prepared(&prepared);
    result->time_ns += thread_cpu_time_ns() - begin_ns;
    ++result->timing_batches;

    // LEAP extraction stays outside the measured run-only region.
    consume_prepared(&prepared, k, result);
  }

  result->status = 0;
  if (result->timing_batches > 1) {
    std::snprintf(
        result->message,
        sizeof(result->message),
        "run-only time accumulated over %llu prepared-object blocks",
        static_cast<unsigned long long>(result->timing_batches));
  }
  return 0;
}

