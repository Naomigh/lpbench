# Threshold Verification Benchmark

This directory benchmarks in-memory threshold verification:

```text
pass = edit_distance(read, reference) <= k
```

The main metric is CPU time from `CLOCK_THREAD_CPUTIME_ID`: core CPU time including memory/cache access effects.

## What Is Measured

- Already-loaded read/reference bytes in heap memory.
- Method computation and method-internal temporary memory accesses.
- Score extraction or threshold comparison.
- Boolean result handling through a volatile sink and a preallocated result array.
- Loop overhead inside the timed verification loop.

## What Is Not Measured

- Dataset generation.
- Trusted ground-truth computation.
- File I/O and TSV parsing.
- Output writing.
- Python orchestration.
- CMake build time.
- Wall-clock time as the main metric.

## Dataset

The default dataset is synthetic DNA:

- length: 100 bp
- thresholds: `k = 1, 2, 3, 4, 5`
- true edit distance groups: `0..15`
- default count: 10000 pairs per true edit distance group
- final read/reference lengths are equal
- generated edits include substitutions, insertions, and deletions across the dataset

The TSV stores `true_ed` and legacy `pass_k1..pass_k5` columns. Correctness labels are computed as `true_ed <= k`, so `k=0` and `k>5` are supported without adding more pass columns.

## Timing Boundary

The benchmark loads and parses the TSV before timing. Timing starts immediately before the verification loop and ends immediately after that loop. Correctness comparison runs after timing against the stored `pass_kX` labels.

## Methods

- `leap_avx2`
- `leap_avx512`
- `lv89`
- `miniwfa`
- `wfa2_fresh`
- `wfa2_reuse`

Edlib is not benchmarked.

## LEAP Chunking

- AVX2 uses 256 bp chunks.
- AVX512 uses 512 bp chunks.
- A pair passes only when every chunk passes.

The default 100 bp benchmark uses one chunk for both backends. AVX512 is not emulated or replaced by AVX2; if AVX512F+AVX512BW are unavailable and `leap_avx512` is requested, the benchmark fails clearly.

## Cache Modes

- `warm`: touch the selected read/reference bytes and run one untimed warmup verification pass before timing.
- `cold-ish`: touch a large eviction buffer before timing.

The benchmark does not manually put data into cache. It influences cache residency; actual placement and eviction are controlled by CPU hardware.

## Perf Counters

`benchmark/scripts/run_perf_stat.py` runs `perf stat` per method/k/cache mode. Perf counters help analyze cache and memory behavior; they do not directly split exact memory time from compute time.

## How To Run

Recommended machine setup:

```bash
sudo cpupower frequency-set -g performance
sudo sh -c 'echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo'
```

Build, generate the default dataset if missing, run all methods, and write outputs:

```bash
python3 benchmark/scripts/run_threshold_bench.py \
  --build \
  --lengths 100 \
  --ks 1 2 3 4 5 \
  --true-ed-min 0 \
  --true-ed-max 15 \
  --pairs-per-ed 10000 \
  --cache-mode warm \
  --core 2 \
  --seed 1
```

Optional perf run:

```bash
python3 benchmark/scripts/run_threshold_bench.py \
  --build \
  --lengths 100 \
  --ks 1 2 3 4 5 \
  --true-ed-min 0 \
  --true-ed-max 15 \
  --pairs-per-ed 10000 \
  --cache-mode warm \
  --core 2 \
  --seed 1 \
  --run-perf
```

Main outputs:

- `benchmark/results/raw/threshold_raw.tsv`
- `benchmark/results/raw/threshold_by_ed.tsv`
- `benchmark/results/raw/leap_phase_raw.tsv` only when `--run-leap-phase` is used
- `benchmark/results/raw/perf_summary.tsv` when `--run-perf` is used
- `benchmark/results/summary/environment.txt`
- `benchmark/results/summary/report.md`


## Select Methods Conveniently

Use `benchmark/scripts/run_selected_methods.py` when you want a shorter entry point for choosing method groups.

List available methods and presets:

```bash
python3 benchmark/scripts/run_selected_methods.py --list-methods
```

Run a preset without AVX512:

```bash
python3 benchmark/scripts/run_selected_methods.py \
  --preset no-avx512 \
  --core 2
```

Run explicit methods only:

```bash
python3 benchmark/scripts/run_selected_methods.py \
  --methods leap_avx2 lv89 miniwfa \
  --pairs-per-ed 10000 \
  --core 2
```

Preview the command without running it:

```bash
python3 benchmark/scripts/run_selected_methods.py \
  --preset wfa2 \
  --dry-run
```


## Edit Distance Range

All experiment entry points accept the maximum generated edit distance:

- `--true-ed-max N`
- `--max-ed N`
- `--max-edit-distance N`

These are aliases. `--true-ed-min`, `--min-ed`, and `--min-edit-distance` control the lower bound.

Examples:

```bash
python3 benchmark/scripts/run_selected_methods.py \
  --methods lv89 miniwfa \
  --ks 3 \
  --max-ed 8 \
  --pairs-per-ed 10000
```

```bash
python3 benchmark/scripts/run_fixed_k_method_scaling.py \
  --k 3 \
  --max-ed 8 \
  --preset all
```

```bash
python3 benchmark/scripts/run_1m_avg.py \
  --target-pairs 900000 \
  --k 3 \
  --max-ed 8 \
  --preset all
```

For large runs, `--target-pairs` must be divisible by the number of edit-distance groups. For example, `ed=0..8` has 9 groups, so `900000` works and gives `100000` pairs per edit-distance group.


## LEAP Forward-Only Timing

In `threshold_raw.tsv` and scripts built on it, `leap_avx2` and `leap_avx512` report only the LEAP forward/run phase as their main timing. The default experiment runners do not call `leap_phase_bench`; use `--run-leap-phase` only when you explicitly want the separate phase table:

- outside timing: `SIMD_ED` construction, `init_levenshtein`, `load_reads`, `calculate_masks`, and `reset`
- timed: `ed.run()` over the selected in-memory pairs
- after timing: `check_pass()` and mismatch counting

The output `mode` column is `forward` for both LEAP methods. Other methods still report their wrapper verification time as before.


## Large Experiment Length

`run_1m_avg.py` supports changing the equal read/reference length:

```bash
python3 benchmark/scripts/run_1m_avg.py \
  --target-pairs 1000000 \
  --k 5 \
  --length 150 \
  --max-ed 15 \
  --preset all
```

Aliases:

- `--length N`
- `--read-length N`
- `--read-reference-length N`

You may also pass `--reference-length N`, but this benchmark requires equal final read/reference lengths, so it must match `--length` / `--read-length`.

Length is included in large-run output paths, for example:

```text
benchmark/results/large/1000000_pairs_len150_k5_ed0_15/
```
