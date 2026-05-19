# Threshold Verification Benchmark

## Timing Methodology

This benchmark measures CPU time, not wall time. The measured timer is `CLOCK_THREAD_CPUTIME_ID` via `clock_gettime`, exposed in the benchmark harness as `thread_cpu_time_ns()`.

The timed task is threshold verification: determine whether `edit_distance(read, reference) <= k`. It is not an end-to-end mapper benchmark. The benchmark does not compute CIGAR strings, traceback, or backtracking.

File I/O, dataset generation, ground-truth computation, result writing, report generation, correctness summary generation, and stdout printing are excluded from the measured region. All synthetic read/reference pairs are generated and preloaded in memory before timing. Ground truth is computed by a trusted dynamic-programming edit-distance implementation outside timing and stored with each generated pair.

Before measured timing, every byte of every read/reference string is touched using a volatile sink. Each method also receives one full untimed dry run over every pair in each dataset group. The measured region then runs the algorithm work for each pair and records pass/fail results for later correctness checking.

Optional software prefetching is enabled with `--prefetch-distance`; this run used distance `16`. If the distance is greater than zero, the measured loop prefetches pair `i + N` when that pair is in bounds.

Results are normalized to seconds per 10 million pairs, matching the LEAP paper style, using `seconds_per_10M_pairs = avg_cpu_ns_per_pair / 100.0`.

LEAP runs as a threshold verifier with `max_energy = k`. LEAP AVX2 and AVX512 use the backend MAX_LENGTH/effective length. If the target length is at most MAX_LENGTH, a pair is processed as one chunk. If the target length exceeds MAX_LENGTH, read and reference are split into corresponding chunks, each chunk is processed with `max_energy = k`, and the wrapper reports pass only if all chunks pass. Chunk overhead is included in the measured wrapper time.

Methods that do not expose a max-distance cutoff may compute the full score first and then apply the threshold.

## Summary

| method | length | k | pairs | pass_count | fail_count | avg_cpu_ns_per_pair | seconds_per_10M_pairs |
| --- | --- | --- | --- | --- | --- | --- | --- |
| leap_avx2 | 100 | 1 | 10000 | 10000 | 0 | 567.821 | 5.678210 |
| leap_avx2 | 100 | 2 | 10000 | 10000 | 0 | 656.842 | 6.568420 |
| leap_avx2 | 100 | 3 | 10000 | 10000 | 0 | 771.030 | 7.710300 |
| leap_avx2 | 100 | 4 | 10000 | 10000 | 0 | 868.090 | 8.680890 |
| leap_avx2 | 100 | 5 | 10000 | 10000 | 0 | 989.037 | 9.890370 |
| leap_avx2 | 200 | 1 | 10000 | 10000 | 0 | 555.029 | 5.550290 |
| leap_avx2 | 200 | 2 | 10000 | 9996 | 4 | 652.344 | 6.523440 |
| leap_avx2 | 200 | 3 | 10000 | 9992 | 8 | 783.032 | 7.830320 |
| leap_avx2 | 200 | 4 | 10000 | 9989 | 11 | 879.811 | 8.798110 |
| leap_avx2 | 200 | 5 | 10000 | 9991 | 9 | 987.546 | 9.875460 |
| leap_avx2 | 500 | 1 | 10000 | 10000 | 0 | 1132.160 | 11.321600 |
| leap_avx2 | 500 | 2 | 10000 | 9999 | 1 | 1343.150 | 13.431500 |
| leap_avx2 | 500 | 3 | 10000 | 9999 | 1 | 1552.710 | 15.527100 |
| leap_avx2 | 500 | 4 | 10000 | 9999 | 1 | 1735.940 | 17.359400 |
| leap_avx2 | 500 | 5 | 10000 | 9996 | 4 | 2011.290 | 20.112900 |
| leap_avx512 | 100 | 1 | 10000 | 10000 | 0 | 476.588 | 4.765880 |
| leap_avx512 | 100 | 2 | 10000 | 10000 | 0 | 576.856 | 5.768560 |
| leap_avx512 | 100 | 3 | 10000 | 10000 | 0 | 682.669 | 6.826690 |
| leap_avx512 | 100 | 4 | 10000 | 10000 | 0 | 788.065 | 7.880650 |
| leap_avx512 | 100 | 5 | 10000 | 10000 | 0 | 904.362 | 9.043620 |
| leap_avx512 | 200 | 1 | 10000 | 10000 | 0 | 479.330 | 4.793300 |
| leap_avx512 | 200 | 2 | 10000 | 10000 | 0 | 581.973 | 5.819730 |
| leap_avx512 | 200 | 3 | 10000 | 10000 | 0 | 697.740 | 6.977400 |
| leap_avx512 | 200 | 4 | 10000 | 10000 | 0 | 817.272 | 8.172720 |
| leap_avx512 | 200 | 5 | 10000 | 10000 | 0 | 921.258 | 9.212580 |
| leap_avx512 | 500 | 1 | 10000 | 10000 | 0 | 510.242 | 5.102420 |
| leap_avx512 | 500 | 2 | 10000 | 10000 | 0 | 644.188 | 6.441880 |
| leap_avx512 | 500 | 3 | 10000 | 10000 | 0 | 792.423 | 7.924230 |
| leap_avx512 | 500 | 4 | 10000 | 10000 | 0 | 927.617 | 9.276170 |
| leap_avx512 | 500 | 5 | 10000 | 10000 | 0 | 1041.340 | 10.413400 |
| miniwfa | 100 | 1 | 10000 | 10000 | 0 | 463.375 | 4.633750 |
| miniwfa | 100 | 2 | 10000 | 10000 | 0 | 495.474 | 4.954740 |
| miniwfa | 100 | 3 | 10000 | 10000 | 0 | 540.673 | 5.406730 |
| miniwfa | 100 | 4 | 10000 | 10000 | 0 | 578.228 | 5.782280 |
| miniwfa | 100 | 5 | 10000 | 10000 | 0 | 610.317 | 6.103170 |
| miniwfa | 200 | 1 | 10000 | 10000 | 0 | 576.688 | 5.766880 |
| miniwfa | 200 | 2 | 10000 | 10000 | 0 | 613.850 | 6.138500 |
| miniwfa | 200 | 3 | 10000 | 10000 | 0 | 651.374 | 6.513740 |
| miniwfa | 200 | 4 | 10000 | 10000 | 0 | 689.703 | 6.897030 |
| miniwfa | 200 | 5 | 10000 | 10000 | 0 | 725.973 | 7.259730 |
| miniwfa | 500 | 1 | 10000 | 10000 | 0 | 924.957 | 9.249570 |
| miniwfa | 500 | 2 | 10000 | 10000 | 0 | 964.084 | 9.640840 |
| miniwfa | 500 | 3 | 10000 | 10000 | 0 | 1006.560 | 10.065600 |
| miniwfa | 500 | 4 | 10000 | 10000 | 0 | 1047.140 | 10.471400 |
| miniwfa | 500 | 5 | 10000 | 10000 | 0 | 1090.400 | 10.904000 |
| lv89 | 100 | 1 | 10000 | 10000 | 0 | 59.475 | 0.594752 |
| lv89 | 100 | 2 | 10000 | 10000 | 0 | 75.319 | 0.753190 |
| lv89 | 100 | 3 | 10000 | 10000 | 0 | 94.953 | 0.949525 |
| lv89 | 100 | 4 | 10000 | 10000 | 0 | 113.638 | 1.136380 |
| lv89 | 100 | 5 | 10000 | 10000 | 0 | 134.739 | 1.347390 |
| lv89 | 200 | 1 | 10000 | 10000 | 0 | 61.913 | 0.619129 |
| lv89 | 200 | 2 | 10000 | 10000 | 0 | 90.006 | 0.900061 |
| lv89 | 200 | 3 | 10000 | 10000 | 0 | 107.574 | 1.075740 |
| lv89 | 200 | 4 | 10000 | 10000 | 0 | 129.368 | 1.293680 |
| lv89 | 200 | 5 | 10000 | 10000 | 0 | 139.641 | 1.396410 |
| lv89 | 500 | 1 | 10000 | 10000 | 0 | 104.282 | 1.042820 |
| lv89 | 500 | 2 | 10000 | 10000 | 0 | 123.022 | 1.230220 |
| lv89 | 500 | 3 | 10000 | 10000 | 0 | 144.532 | 1.445320 |
| lv89 | 500 | 4 | 10000 | 10000 | 0 | 169.066 | 1.690660 |
| lv89 | 500 | 5 | 10000 | 10000 | 0 | 197.494 | 1.974940 |
| wfa2-lib | 100 | 1 | 10000 | 10000 | 0 | 618.422 | 6.184220 |
| wfa2-lib | 100 | 2 | 10000 | 10000 | 0 | 650.473 | 6.504730 |
| wfa2-lib | 100 | 3 | 10000 | 10000 | 0 | 685.347 | 6.853470 |
| wfa2-lib | 100 | 4 | 10000 | 10000 | 0 | 720.832 | 7.208320 |
| wfa2-lib | 100 | 5 | 10000 | 10000 | 0 | 750.984 | 7.509840 |
| wfa2-lib | 200 | 1 | 10000 | 10000 | 0 | 632.530 | 6.325300 |
| wfa2-lib | 200 | 2 | 10000 | 10000 | 0 | 665.868 | 6.658680 |
| wfa2-lib | 200 | 3 | 10000 | 10000 | 0 | 705.712 | 7.057120 |
| wfa2-lib | 200 | 4 | 10000 | 10000 | 0 | 738.865 | 7.388650 |
| wfa2-lib | 200 | 5 | 10000 | 10000 | 0 | 795.977 | 7.959770 |
| wfa2-lib | 500 | 1 | 10000 | 10000 | 0 | 690.116 | 6.901160 |
| wfa2-lib | 500 | 2 | 10000 | 10000 | 0 | 754.399 | 7.543990 |
| wfa2-lib | 500 | 3 | 10000 | 10000 | 0 | 780.812 | 7.808110 |
| wfa2-lib | 500 | 4 | 10000 | 10000 | 0 | 855.699 | 8.556990 |
| wfa2-lib | 500 | 5 | 10000 | 10000 | 0 | 857.723 | 8.577230 |

## Method Notes

| method | cutoff behavior | chunk behavior | timed components |
| --- | --- | --- | --- |
| leap_avx2 | Native threshold cutoff via max_energy = k. | Uses LEAP AVX2 MAX_LENGTH/effective length 256; lengths above 256 are split into ceil(length / 256) chunks and all chunks must pass. | load_reads, calculate_masks, reset, run, check_pass for each chunk. |
| leap_avx512 | Native threshold cutoff via max_energy = k. | Uses LEAP AVX512 MAX_LENGTH/effective length 512; lengths above 512 are split into ceil(length / 512) chunks and all chunks must pass. | load_reads, calculate_masks, reset, run, check_pass for each chunk. |
| miniwfa | Native threshold cutoff via opt.max_s = k; returns failure when score exceeds the cutoff. | No external chunking. | mwf_wfa_exact score-only call with per-pair options and result extraction. |
| lv89 | Uses lv89 native band width bw = k, then thresholds the computed score; no separate max-score cutoff is exposed. | No external chunking. | lv_ed score-only call with bw = k and final score extraction. |
| wfa2-lib | Computes score-only edit distance and thresholds the score; no max-score cutoff is used. | No external chunking. | wavefront_aligner_new, wavefront_align with compute_score/edit, score extraction, delete. |

## Correctness

| method | length | k | mismatch_count |
| --- | --- | --- | --- |
| leap_avx2 | 100 | 1 | 0 |
| leap_avx2 | 100 | 2 | 0 |
| leap_avx2 | 100 | 3 | 0 |
| leap_avx2 | 100 | 4 | 0 |
| leap_avx2 | 100 | 5 | 0 |
| leap_avx2 | 200 | 1 | 0 |
| leap_avx2 | 200 | 2 | 4 |
| leap_avx2 | 200 | 3 | 8 |
| leap_avx2 | 200 | 4 | 11 |
| leap_avx2 | 200 | 5 | 9 |
| leap_avx2 | 500 | 1 | 0 |
| leap_avx2 | 500 | 2 | 1 |
| leap_avx2 | 500 | 3 | 1 |
| leap_avx2 | 500 | 4 | 1 |
| leap_avx2 | 500 | 5 | 4 |
| leap_avx512 | 100 | 1 | 0 |
| leap_avx512 | 100 | 2 | 0 |
| leap_avx512 | 100 | 3 | 0 |
| leap_avx512 | 100 | 4 | 0 |
| leap_avx512 | 100 | 5 | 0 |
| leap_avx512 | 200 | 1 | 0 |
| leap_avx512 | 200 | 2 | 0 |
| leap_avx512 | 200 | 3 | 0 |
| leap_avx512 | 200 | 4 | 0 |
| leap_avx512 | 200 | 5 | 0 |
| leap_avx512 | 500 | 1 | 0 |
| leap_avx512 | 500 | 2 | 0 |
| leap_avx512 | 500 | 3 | 0 |
| leap_avx512 | 500 | 4 | 0 |
| leap_avx512 | 500 | 5 | 0 |
| miniwfa | 100 | 1 | 0 |
| miniwfa | 100 | 2 | 0 |
| miniwfa | 100 | 3 | 0 |
| miniwfa | 100 | 4 | 0 |
| miniwfa | 100 | 5 | 0 |
| miniwfa | 200 | 1 | 0 |
| miniwfa | 200 | 2 | 0 |
| miniwfa | 200 | 3 | 0 |
| miniwfa | 200 | 4 | 0 |
| miniwfa | 200 | 5 | 0 |
| miniwfa | 500 | 1 | 0 |
| miniwfa | 500 | 2 | 0 |
| miniwfa | 500 | 3 | 0 |
| miniwfa | 500 | 4 | 0 |
| miniwfa | 500 | 5 | 0 |
| lv89 | 100 | 1 | 0 |
| lv89 | 100 | 2 | 0 |
| lv89 | 100 | 3 | 0 |
| lv89 | 100 | 4 | 0 |
| lv89 | 100 | 5 | 0 |
| lv89 | 200 | 1 | 0 |
| lv89 | 200 | 2 | 0 |
| lv89 | 200 | 3 | 0 |
| lv89 | 200 | 4 | 0 |
| lv89 | 200 | 5 | 0 |
| lv89 | 500 | 1 | 0 |
| lv89 | 500 | 2 | 0 |
| lv89 | 500 | 3 | 0 |
| lv89 | 500 | 4 | 0 |
| lv89 | 500 | 5 | 0 |
| wfa2-lib | 100 | 1 | 0 |
| wfa2-lib | 100 | 2 | 0 |
| wfa2-lib | 100 | 3 | 0 |
| wfa2-lib | 100 | 4 | 0 |
| wfa2-lib | 100 | 5 | 0 |
| wfa2-lib | 200 | 1 | 0 |
| wfa2-lib | 200 | 2 | 0 |
| wfa2-lib | 200 | 3 | 0 |
| wfa2-lib | 200 | 4 | 0 |
| wfa2-lib | 200 | 5 | 0 |
| wfa2-lib | 500 | 1 | 0 |
| wfa2-lib | 500 | 2 | 0 |
| wfa2-lib | 500 | 3 | 0 |
| wfa2-lib | 500 | 4 | 0 |
| wfa2-lib | 500 | 5 | 0 |

Raw TSV: `benchmark/results/raw/threshold_verify_raw.tsv`
Summary TSV: `benchmark/results/summary/threshold_verify_summary.tsv`
Mismatch debug TSV: `benchmark/results/raw/threshold_verify_mismatches.tsv`

## Build And Run

```bash
make threshold_verify_bench
python3 benchmark/scripts/run_threshold_verify_bench.py
```
