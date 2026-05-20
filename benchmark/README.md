# Threshold Verification Benchmark

This directory implements an in-memory threshold-verification benchmark:

```text
pass = edit_distance(read, reference) <= k
```

The main timer uses `CLOCK_THREAD_CPUTIME_ID`, so reported time is core CPU time including memory/cache access effects inside the verification loop.

## What Is Measured

The timed region starts immediately before the method verification loop and ends immediately after it. It includes reading already-loaded read/reference bytes, CPU loads through the cache hierarchy, method computation, method-internal temporary memory accesses, final `score <= k` comparison, result storage to a preallocated array, sink updates, and loop overhead.

## What Is Not Measured

Dataset generation, trusted edit-distance computation, TSV file reading, TSV parsing, output writing, Python orchestration, CMake build time, and wall time are outside the main timing boundary.

## Dataset

The default dataset is synthetic DNA with length 100 bp, true edit-distance groups `0..15`, thresholds `k=1..5`, and alphabet `A/C/G/T`. Final read and reference lengths are equal. The TSV stores trusted `true_ed` and precomputed `pass_k1..pass_k5` labels before any benchmark timing begins.

Default path:

```bash
benchmark/data/generated/pairs_len100_ed0_15.tsv
```

The generator verifies every accepted pair with an internal DP implementation and includes substitutions, balanced insertions/deletions, and mixed edits across the dataset.

## Methods

Main benchmark method names:

```text
leap_avx2
leap_avx512
lv89
miniwfa
wfa2_fresh
wfa2_reuse
```

The benchmark layer exposes the requested unified method interface and calls the cloned method implementations when their top-level directories are present. `edlib` is not benchmarked.

## LEAP Chunking

LEAP AVX2 uses fixed 256 bp chunks. LEAP AVX512 uses fixed 512 bp chunks. For lengths larger than the chunk size, the pair is split into consecutive chunks and passes only if every chunk passes. The default 100 bp dataset uses one chunk for both backends.

The AVX wrappers check CPU feature availability and fail clearly if the requested backend is unavailable.

## Cache Modes

`warm` touches all selected read/reference bytes and performs one untimed warmup verification pass before timing.

`cold-ish` touches a large eviction buffer before timing. This approximates colder cache behavior, but it is not an exact memory-time measurement.

## Perf Counters

`benchmark/scripts/run_perf_stat.py` records counters such as cycles, instructions, branches, cache misses, L1D misses, LLC misses, and dTLB misses. These counters help analyze cache and memory behavior; they do not directly split exact memory time from compute time.

## Build

```bash
python3 benchmark/scripts/build_benchmark.py
```

Equivalent CMake commands:

```bash
cmake -S benchmark -B benchmark/build -DCMAKE_BUILD_TYPE=Release
cmake --build benchmark/build --target threshold_verify_bench -j
```

Default compiler flags include `-O3`, `-DNDEBUG`, and `-march=native`.

## Run

Required default run:

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

On bare metal, record the environment and prefer stable CPU settings:

```bash
bash benchmark/scripts/record_environment.sh
sudo cpupower frequency-set -g performance
sudo sh -c 'echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo'
taskset -c 2 ./benchmark/build/threshold_bench ...
```

If turbo cannot be disabled, record that fact in `benchmark/results/summary/environment.txt`.

## Outputs

```text
benchmark/results/raw/threshold_raw.tsv
benchmark/results/raw/threshold_by_ed.tsv
benchmark/results/raw/leap_phase_raw.tsv
benchmark/results/raw/perf_summary.tsv
benchmark/results/summary/summary.md
benchmark/results/summary/environment.txt
```

`threshold_raw.tsv` reports whole selected-group timing per method and `k`. `threshold_by_ed.tsv` reports timing grouped by true edit distance. `leap_phase_raw.tsv` reports LEAP-style phase timings for AVX2 and AVX512 backends.
