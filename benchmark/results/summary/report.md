# Threshold Verification Benchmark Summary

Main metric: core CPU time including memory/cache access effects.

| method | k | pairs | mismatches | seconds per 10M pairs |
|---|---:|---:|---:|---:|
| leap_avx2 | 4 | 160000 | 840 | 4.777760 |
| leap_avx512 | 4 | 160000 | 840 | 4.745070 |
| lv89 | 4 | 160000 | 0 | 2.694010 |
| miniwfa | 4 | 160000 | 0 | 4.746650 |
| wfa2_fresh | 4 | 160000 | 0 | 7.381990 |
| wfa2_reuse | 4 | 160000 | 0 | 4.171430 |
