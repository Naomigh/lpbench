#pragma once

namespace bench {

inline constexpr const char* kThresholdRawHeader =
    "method\tbackend\tmode\tcache_mode\tlength\tk\tpair_count\tpass_count\t"
    "fail_count\tmismatch_count\ttotal_cpu_ns\tavg_cpu_ns_per_pair\t"
    "seconds_per_10M_pairs\tresult_sink\tseed\n";

inline constexpr const char* kThresholdByEdHeader =
    "method\tbackend\tmode\tcache_mode\tlength\tk\ttrue_ed_group\tpair_count\t"
    "pass_count\tfail_count\tmismatch_count\ttotal_cpu_ns\t"
    "avg_cpu_ns_per_pair\tseed\n";

}  // namespace bench

