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
| No rows recorded |  |  |  |  |  |  |

### Length 100, max edit distance k = 1

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 100, max edit distance k = 2

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 100, max edit distance k = 3

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 100, max edit distance k = 4

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 100, max edit distance k = 5

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 200, max edit distance k = 0

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 200, max edit distance k = 1

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 200, max edit distance k = 2

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 200, max edit distance k = 3

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 200, max edit distance k = 4

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 200, max edit distance k = 5

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 500, max edit distance k = 0

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 500, max edit distance k = 1

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 500, max edit distance k = 2

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 500, max edit distance k = 3

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 500, max edit distance k = 4

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| No rows recorded |  |  |  |  |  |  |

### Length 500, max edit distance k = 5

| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |
|---|---|---|---:|---:|---:|---|
| leap_avx2 | diff | run_only | 80 | 297.700 | 27151 | LEAP pass-derived sinks updated outside run-only timing |
| leap_avx512 | diff | run_only | 80 | 298.175 | 31216 | LEAP pass-derived sinks updated outside run-only timing |
| lv89 | diff | full_workflow | 80 | 273.700 | 28103 |  |
| miniwfa | diff | full_workflow | 80 | 3246.125 | 239650 |  |
| wfa2_reuse | diff | full_workflow | 80 | 555.575 | 48753 |  |
