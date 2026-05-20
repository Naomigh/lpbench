# Threshold Verification Benchmark Report

Generated: 2026-05-20, Asia/Shanghai

## Setup

Repositories cloned into the workspace:

| Repository | Directory | Commit |
|---|---|---|
| Naomigh/leap | `leap/` | `6169540` |
| lh3/lv89 | `lv89/` | `b5447aa` |
| lh3/miniwfa | `miniwfa/` | `66770a3` |
| smarco/WFA2-lib | `WFA2-lib/` | `bcf473a` |

Benchmark command:

```bash
python3 benchmark/scripts/run_threshold_bench.py \
  --lengths 100 \
  --ks 1 2 3 4 5 \
  --true-ed-min 0 \
  --true-ed-max 15 \
  --pairs-per-ed 10000 \
  --methods leap_avx2 leap_avx512 lv89 miniwfa wfa2_fresh wfa2_reuse \
  --cache-mode warm \
  --core 2 \
  --seed 1 \
  --build
```

Dataset:

- `benchmark/data/generated/pairs_len100_ed0_15.tsv`
- 160000 total pairs
- 10000 pairs for each trusted edit-distance group `0..15`
- `k=1..5`
- Equal-length 100 bp read/reference pairs
- Ground truth labels loaded from TSV before timing

Timing:

- Main metric is `CLOCK_THREAD_CPUTIME_ID`.
- Timed region is only the in-memory threshold verification loop.
- File I/O, TSV parsing, dataset generation, output writing, and Python orchestration are excluded.

## Wrapper Configuration

| Method | Integration |
|---|---|
| `leap_avx2` | `SIMD_ED::init_levenshtein(k, ED_GLOBAL, false)`, no SHD prefilter, 256 bp chunking |
| `leap_avx512` | `SIMD_ED::init_levenshtein(k, ED_GLOBAL, false)`, no SHD prefilter, 512 bp chunking |
| `lv89` | `lv_ed_basic(length, ref, length, read, is_ext=0, bw=k, n_cigar=null)` |
| `miniwfa` | `mwf_wfa_exact`, score-only, CIGAR disabled, `x=1`, `o1=0,e1=1`, `o2=0,e2=1`, `max_s=k` |
| `wfa2_fresh` | New `wfa::WFAlignerEdit(Score)` per call, heuristic disabled, `max_alignment_steps=k+1` |
| `wfa2_reuse` | Thread-local reused `wfa::WFAlignerEdit(Score)`, heuristic disabled, `max_alignment_steps=k+1` |

## Correctness

`lv89`, `miniwfa`, `wfa2_fresh`, and `wfa2_reuse` matched all stored ground-truth labels for every `k`.

LEAP was run without SHD prefilter as requested. In this no-SHD configuration, both LEAP backends produced mismatches against the trusted dataset labels. These rows should not be used as valid threshold-verification accuracy results until the LEAP wrapper or LEAP configuration is corrected.

| Method | Total mismatch count across k=1..5 |
|---|---:|
| `leap_avx2` | 87320 |
| `leap_avx512` | 87320 |
| `lv89` | 0 |
| `miniwfa` | 0 |
| `wfa2_fresh` | 0 |
| `wfa2_reuse` | 0 |

## Main Results

Average CPU ns per pair:

| Method | k=1 | k=2 | k=3 | k=4 | k=5 | Mean |
|---|---:|---:|---:|---:|---:|---:|
| `wfa2_reuse` | 91.939 | 122.000 | 149.000 | 174.273 | 199.411 | 147.325 |
| `lv89` | 286.871 | 252.665 | 257.489 | 264.905 | 267.624 | 265.911 |
| `leap_avx512` | 267.352 | 324.991 | 405.305 | 478.217 | 558.719 | 406.917 |
| `leap_avx2` | 276.542 | 349.388 | 422.203 | 489.284 | 566.392 | 420.762 |
| `miniwfa` | 368.392 | 416.582 | 447.630 | 464.364 | 496.260 | 438.646 |
| `wfa2_fresh` | 451.325 | 481.633 | 531.535 | 524.293 | 564.109 | 510.579 |

For methods with zero mismatches, `wfa2_reuse` is fastest on this workload. Its mean time is about 1.8x faster than `lv89`, 3.0x faster than `miniwfa`, and 3.5x faster than `wfa2_fresh`.

`wfa2_fresh` is slower than `wfa2_reuse` because it includes per-pair aligner construction and deletion inside the timed verification call. This is intentional; the two modes should be interpreted separately.

## Edit-Distance Dependence

At `k=5`, average CPU ns per pair by selected true edit-distance groups:

| Method | ed=0 | ed=1 | ed=2 | ed=3 | ed=4 | ed=5 | ed=11 | ed=15 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `lv89` | 32.041 | 59.125 | 84.521 | 110.145 | 144.225 | 173.734 | 376.959 | 530.853 |
| `miniwfa` | 299.932 | 341.743 | 382.003 | 431.715 | 476.897 | 618.907 | 520.837 | 526.105 |
| `wfa2_reuse` | 47.942 | 83.049 | 122.019 | 168.951 | 214.262 | 329.328 | 224.650 | 212.106 |

`lv89` scales strongly with true edit distance, as expected for an `O(nd)` style algorithm. `wfa2_reuse` is very fast for low edit distances and remains low for high edit-distance fail cases because threshold stopping limits work. `miniwfa` is consistently slower on this short-read threshold task.

## Outputs

- Raw main results: `benchmark/results/raw/threshold_raw.tsv`
- By-edit-distance results: `benchmark/results/raw/threshold_by_ed.tsv`
- LEAP phase output: `benchmark/results/raw/leap_phase_raw.tsv`
- Environment record: `benchmark/results/summary/environment.txt`

## Notes

- No perf counter run was requested or executed.
- The LEAP phase binary currently reports the benchmark phase scaffold. The accuracy-relevant LEAP finding is from `threshold_raw.tsv`: no-SHD LEAP wrapper output mismatches trusted labels.
- The benchmark reports core CPU time including memory/cache access effects, not pure compute time.
