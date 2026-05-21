# Method Timing Benchmark

LEAP numbers are run-only forward traversal time and are not LEAP end-to-end time.

Non-LEAP numbers are warmed full-workflow timing for the selected method.

The N1/N2 difference estimate uses t = (T2 - T1) / (N2 - N1) to reduce fixed benchmark overhead.

LEAP AVX2 and AVX512 timing includes only `ed.run()` over prepared chunk objects. Non-LEAP timing includes the warmed method workflow needed to produce score/pass, with CIGAR and traceback disabled.

Dataset loading, TSV parsing, synthetic generation, ground-truth computation, output writing, report generation, Python orchestration, and build time are outside timed regions.

Timer overhead is reduced with batch timing and optionally with the N1/N2 linear-difference timing mode. The internal timer is `CLOCK_THREAD_CPUTIME_ID`.

The benchmark does not claim exact pure arithmetic time. The benchmark does not claim exact memory time.

## Requested 18 Groups

### Length 100, max edit distance k = 0

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 136.694 | 136693990 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 121.278 | 121278410 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 45.728 | 45728041 |  |
| miniwfa | batch | full_workflow | 1000000 | 441.187 | 441187266 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 69.762 | 69761852 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 571.640 | 571640430 | WFA2 aligner construction and destruction included per pair |

### Length 100, max edit distance k = 1

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 218.775 | 218774873 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 185.821 | 185820668 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 65.439 | 65438795 |  |
| miniwfa | batch | full_workflow | 1000000 | 484.500 | 484499671 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 100.816 | 100816436 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 606.575 | 606574815 | WFA2 aligner construction and destruction included per pair |

### Length 100, max edit distance k = 2

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 322.218 | 322217978 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 294.820 | 294819895 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 83.239 | 83239043 |  |
| miniwfa | batch | full_workflow | 1000000 | 528.709 | 528708594 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 129.940 | 129939693 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 636.007 | 636007198 | WFA2 aligner construction and destruction included per pair |

### Length 100, max edit distance k = 3

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 406.622 | 406622031 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 379.928 | 379927998 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 102.854 | 102854014 |  |
| miniwfa | batch | full_workflow | 1000000 | 582.028 | 582027565 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 158.997 | 158997242 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 666.094 | 666094440 | WFA2 aligner construction and destruction included per pair |

### Length 100, max edit distance k = 4

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 438.650 | 438649907 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 460.947 | 460946690 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 126.763 | 126762697 |  |
| miniwfa | batch | full_workflow | 1000000 | 611.815 | 611814887 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 196.839 | 196838679 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 718.735 | 718734731 | WFA2 aligner construction and destruction included per pair |

### Length 100, max edit distance k = 5

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 603.964 | 603964285 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 600.130 | 600130022 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 146.678 | 146677457 |  |
| miniwfa | batch | full_workflow | 1000000 | 651.094 | 651093792 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 221.026 | 221025961 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 731.697 | 731697281 | WFA2 aligner construction and destruction included per pair |

### Length 200, max edit distance k = 0

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 137.773 | 137772505 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 141.283 | 141282630 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 40.813 | 40813527 |  |
| miniwfa | batch | full_workflow | 1000000 | 557.846 | 557846292 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 80.130 | 80129757 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 566.248 | 566248140 | WFA2 aligner construction and destruction included per pair |

### Length 200, max edit distance k = 1

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 220.923 | 220923547 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 190.387 | 190386938 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 58.588 | 58587696 |  |
| miniwfa | batch | full_workflow | 1000000 | 595.726 | 595726498 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 108.584 | 108584341 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 613.653 | 613653345 | WFA2 aligner construction and destruction included per pair |

### Length 200, max edit distance k = 2

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 325.720 | 325719736 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 324.509 | 324509296 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 77.071 | 77070808 |  |
| miniwfa | batch | full_workflow | 1000000 | 648.161 | 648160570 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 134.885 | 134884473 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 646.144 | 646144355 | WFA2 aligner construction and destruction included per pair |

### Length 200, max edit distance k = 3

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 376.843 | 376842858 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 451.013 | 451013136 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 97.711 | 97711168 |  |
| miniwfa | batch | full_workflow | 1000000 | 685.140 | 685140217 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 170.018 | 170017567 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 667.179 | 667179329 | WFA2 aligner construction and destruction included per pair |

### Length 200, max edit distance k = 4

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 400.408 | 400408029 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 463.718 | 463717779 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 117.950 | 117950375 |  |
| miniwfa | batch | full_workflow | 1000000 | 730.424 | 730424320 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 222.372 | 222372240 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 719.997 | 719996706 | WFA2 aligner construction and destruction included per pair |

### Length 200, max edit distance k = 5

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 528.713 | 528712801 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 620.202 | 620201822 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 140.863 | 140863363 |  |
| miniwfa | batch | full_workflow | 1000000 | 769.912 | 769911949 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 231.453 | 231452717 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 751.193 | 751193314 | WFA2 aligner construction and destruction included per pair |

### Length 500, max edit distance k = 0

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 283.163 | 283163395 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 31 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 180.991 | 180991182 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 123.581 | 123581064 |  |
| miniwfa | batch | full_workflow | 1000000 | 908.322 | 908322393 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 110.947 | 110947228 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 634.966 | 634966093 | WFA2 aligner construction and destruction included per pair |
| leap_avx2 | diff | run_only | 200000 | 327.917 | 53117297 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 7 prepared-object blocks |
| leap_avx512 | diff | run_only | 200000 | 223.269 | 32600164 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 4 prepared-object blocks |
| lv89 | diff | full_workflow | 200000 | 119.482 | 24205589 |  |
| miniwfa | diff | full_workflow | 200000 | 935.036 | 187048290 |  |
| wfa2_reuse | diff | full_workflow | 200000 | 110.727 | 22214852 |  |
| wfa2_fresh | diff | full_workflow | 200000 | 599.198 | 119726369 | WFA2 aligner construction and destruction included per pair |

### Length 500, max edit distance k = 1

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 354.986 | 354986337 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 31 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 231.628 | 231628337 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 130.686 | 130685832 |  |
| miniwfa | batch | full_workflow | 1000000 | 953.327 | 953326450 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 140.909 | 140909292 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 669.658 | 669657655 | WFA2 aligner construction and destruction included per pair |

### Length 500, max edit distance k = 2

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 510.071 | 510070580 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 31 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 349.854 | 349854037 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 143.105 | 143104607 |  |
| miniwfa | batch | full_workflow | 1000000 | 1005.120 | 1005119609 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 173.931 | 173930979 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 710.972 | 710972176 | WFA2 aligner construction and destruction included per pair |

### Length 500, max edit distance k = 3

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 534.392 | 534391660 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 31 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 407.805 | 407804728 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 157.911 | 157910451 |  |
| miniwfa | batch | full_workflow | 1000000 | 1045.831 | 1045831351 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 203.039 | 203038962 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 729.417 | 729416929 | WFA2 aligner construction and destruction included per pair |

### Length 500, max edit distance k = 4

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 595.942 | 595942352 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 31 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 498.244 | 498243722 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 175.341 | 175341455 |  |
| miniwfa | batch | full_workflow | 1000000 | 1088.120 | 1088119953 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 234.013 | 234012927 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 768.390 | 768389867 | WFA2 aligner construction and destruction included per pair |

### Length 500, max edit distance k = 5

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | batch | run_only | 1000000 | 783.870 | 783870442 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 31 prepared-object blocks |
| leap_avx512 | batch | run_only | 1000000 | 644.200 | 644199888 | LEAP pass-derived sinks updated outside run-only timing; run-only time accumulated over 16 prepared-object blocks |
| lv89 | batch | full_workflow | 1000000 | 193.195 | 193195044 |  |
| miniwfa | batch | full_workflow | 1000000 | 1141.883 | 1141883461 |  |
| wfa2_reuse | batch | full_workflow | 1000000 | 275.910 | 275910381 |  |
| wfa2_fresh | batch | full_workflow | 1000000 | 799.175 | 799174894 | WFA2 aligner construction and destruction included per pair |
