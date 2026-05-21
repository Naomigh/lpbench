#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint64_t time_ns;
  uint64_t pass_sink;
  uint64_t score_sink;
  uint64_t run_calls;
  uint64_t timing_batches;
  int status;
  char message[256];
} leap_backend_result_t;

typedef int (*leap_backend_measure_fn)(
    const char* const* reads,
    const char* const* references,
    const size_t* lengths,
    size_t dataset_count,
    size_t pair_calls,
    int k,
    int warmup_passes,
    leap_backend_result_t* result);

#ifdef __cplusplus
}
#endif

