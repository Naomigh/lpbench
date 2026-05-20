#pragma once

namespace bench {

constexpr const char* kThresholdRawHeader =
    "method\tbackend\tmode\tcache_mode\tlength\tk\tpair_count\tpass_count\tfail_count\t"
    "mismatch_count\ttotal_cpu_ns\tavg_cpu_ns_per_pair\tseconds_per_10M_pairs\tresult_sink\tseed\n";

constexpr const char* kThresholdByEdHeader =
    "method\tbackend\tmode\tcache_mode\tlength\tk\ttrue_ed_group\tpair_count\tpass_count\t"
    "fail_count\tmismatch_count\ttotal_cpu_ns\tavg_cpu_ns_per_pair\tseed\n";

constexpr const char* kLeapPhaseHeader =
    "backend\tlength\tk\ttrue_ed_group\tpair_count\tchunk_size\tchunk_count_per_pair\t"
    "construct_init_cpu_ns\tload_reads_cpu_ns\tcalculate_masks_cpu_ns\treset_cpu_ns\t"
    "run_cpu_ns\tcheck_pass_cpu_ns\tsink_cpu_ns\ttotal_cpu_ns\tconstruct_init_pct\t"
    "load_reads_pct\tcalculate_masks_pct\treset_pct\trun_pct\tcheck_pass_pct\tsink_pct\t"
    "avg_total_cpu_ns_per_pair\tavg_run_cpu_ns_per_pair\tseed\n";

}  // namespace bench
