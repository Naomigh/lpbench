# LEAP Phase Timing And Perf Report

Generated: 2026-05-20, Asia/Shanghai

## Executive Summary

LEAP was measured with SHD prefilter disabled:

```text
init_levenshtein(k, ED_GLOBAL, false)
```

For length-100 reads, both AVX2 and AVX512 use one chunk per pair. The main phase-timer result is that LEAP input preparation costs more than the core algorithm traversal:

| Backend | Total CPU ns/pair | load_reads + calculate_masks | run |
|---|---:|---:|---:|
| `leap_avx2` | 978.161 | 317.404 ns/pair, 32.45% | 145.509 ns/pair, 14.88% |
| `leap_avx512` | 971.188 | 309.608 ns/pair, 31.88% | 149.552 ns/pair, 15.40% |

The closest benchmark-visible proxy for “read/reference data being loaded and converted into LEAP’s CPU-cache-visible internal state” is `load_reads + calculate_masks`. The closest proxy for the true LEAP traversal is `run`.

Do not interpret `load_reads` as pure DRAM time. It includes CPU loads through the cache hierarchy, copies into LEAP object buffers, DNA-to-bit-plane conversion, and normal instruction overhead. Actual L1/L2/LLC/DRAM residency is controlled by the CPU.

## What Was Run

Dataset:

- `benchmark/data/generated/pairs_len100_ed0_15.tsv`
- 160000 pairs total
- 10000 pairs per true edit-distance group, `0..15`
- `k=1..5`
- One 100 bp chunk per pair for both AVX2 and AVX512

Phase timing command:

```bash
taskset -c 2 benchmark/build/leap_phase_bench \
  --dataset benchmark/data/generated/pairs_len100_ed0_15.tsv \
  --out-dir benchmark/results \
  --backends leap_avx2 leap_avx512 \
  --ks 1 2 3 4 5 \
  --length 100 \
  --true-ed-min 0 \
  --true-ed-max 15 \
  --seed 1
```

Perf commands:

```bash
perf stat -x, -o benchmark/results/raw/leap_phase_perf_avx2.csv \
  -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,\
L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses,\
dTLB-loads,dTLB-load-misses \
  -- taskset -c 2 benchmark/build/leap_phase_bench ... --backend leap_avx2
```

```bash
perf stat -x, -o benchmark/results/raw/leap_phase_perf_avx512.csv \
  -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,\
L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses,\
dTLB-loads,dTLB-load-misses \
  -- taskset -c 2 benchmark/build/leap_phase_bench ... --backend leap_avx512
```

Raw outputs:

- `benchmark/results/raw/leap_phase_raw.tsv`
- `benchmark/results/raw/leap_phase_raw_under_perf_avx2.tsv`
- `benchmark/results/raw/leap_phase_raw_under_perf_avx512.tsv`
- `benchmark/results/raw/leap_phase_perf_avx2.csv`
- `benchmark/results/raw/leap_phase_perf_avx512.csv`

## How To Read The Two Measurement Types

The phase timer is inside `leap_phase_bench`. It calls `CLOCK_THREAD_CPUTIME_ID` around each LEAP phase:

- `construct_init`: construct `SIMD_ED` and call `init_levenshtein`.
- `load_reads`: call `ed.load_reads(read, ref, length)`. This reads heap sequence bytes and converts/copies them into LEAP internal bit-plane buffers.
- `calculate_masks`: call `ed.calculate_masks()`. This builds lane masks from LEAP’s converted read/reference representation.
- `reset`: call `ed.reset()`. This initializes per-run LEAP traversal state.
- `run`: call `ed.run()`. This is the core LEAP edit-distance threshold traversal.
- `check_pass`: call `ed.check_pass()` and `ed.get_ED()` when needed.
- `sink`: update the anti-optimization volatile sink.

These phase times are the best source for “which stage costs how much CPU time”. They include the overhead of the timing calls themselves, so very small stages such as `check_pass` and `sink` are partly measurement overhead.

`perf stat` is outside the program. It counts hardware events for the whole process it wraps. In this run, perf includes dataset parsing, output writing, the phase loop, and the internal timer calls. It does not split counters by phase. Use perf to understand overall CPU/cache behavior of an AVX2 or AVX512 phase run, not to assign exact L1/LLC misses to `load_reads` or `run`.

Because many events were requested at once, perf multiplexed counters. The CSV shows about 49-50% active time for most events, meaning perf scaled counts from sampled time. That is normal, but it makes the counters approximate.

## Phase Time Without Perf

This is the clean phase-timer run, not wrapped by `perf stat`.

Average CPU ns per pair:

| Backend | Total | construct_init | load_reads | calculate_masks | reset | run | check_pass | sink |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `leap_avx2` | 978.161 | 235.320 | 206.352 | 111.053 | 95.032 | 145.509 | 92.893 | 92.003 |
| `leap_avx512` | 971.188 | 236.801 | 199.118 | 110.490 | 93.706 | 149.552 | 90.828 | 90.695 |

Percent of total CPU time:

| Backend | construct_init | load_reads | calculate_masks | reset | run | check_pass | sink |
|---|---:|---:|---:|---:|---:|---:|---:|
| `leap_avx2` | 24.06% | 21.10% | 11.35% | 9.72% | 14.88% | 9.50% | 9.41% |
| `leap_avx512` | 24.38% | 20.50% | 11.38% | 9.65% | 15.40% | 9.35% | 9.34% |

## Phase Time Under Perf

These are the internal phase timers from runs wrapped by `perf stat`. They are useful as a sanity check, but the clean no-perf timing above should be preferred for phase-time conclusions.

Average CPU ns per pair:

| Backend | Total | construct_init | load_reads | calculate_masks | reset | run | check_pass | sink |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `leap_avx2` | 970.186 | 231.186 | 203.275 | 110.623 | 94.166 | 144.855 | 92.139 | 93.942 |
| `leap_avx512` | 994.763 | 242.209 | 206.203 | 111.882 | 94.796 | 152.586 | 92.311 | 94.777 |

The under-perf phase timings are close to the no-perf timings. AVX512 is a little slower under perf in this run, which is expected noise/perturbation from measurement.

## Perf Counter Summary

Perf counters for the whole backend run:

| Metric | `leap_avx2` | `leap_avx512` |
|---|---:|---:|
| cycles | 7,610,476,740 | 7,704,745,580 |
| instructions | 22,440,007,602 | 23,520,954,863 |
| IPC | 2.949 | 3.053 |
| branches | 4,210,133,538 | 4,162,946,889 |
| branch misses | 24,510,576 | 24,619,808 |
| branch miss rate | 0.582% | 0.591% |
| cache references | 86,622,650 | 97,680,346 |
| cache misses | 4,831,502 | 5,203,297 |
| cache miss rate | 5.578% | 5.327% |
| L1D loads | 10,382,663,761 | 10,583,688,970 |
| L1D load misses | 25,405,680 | 31,152,249 |
| L1D load miss rate | 0.245% | 0.294% |
| LLC loads | not supported | not supported |
| LLC load misses | not supported | not supported |
| dTLB loads | 1,124,656 | 1,362,371 |
| dTLB load misses | 522,689 | 225,435 |
| dTLB miss rate | 46.475% | 16.547% |

Approximate counters per measured pair, using 800000 phase-loop pairs per backend:

| Metric per pair | `leap_avx2` | `leap_avx512` |
|---|---:|---:|
| cycles/pair | 9513.096 | 9630.932 |
| instructions/pair | 28050.010 | 29401.194 |
| L1D loads/pair | 12978.330 | 13229.611 |
| L1D load misses/pair | 31.757 | 38.940 |
| cache references/pair | 108.278 | 122.100 |
| cache misses/pair | 6.039 | 6.504 |
| dTLB misses/pair | 0.653 | 0.282 |

The perf cycles/pair are much larger than the phase timer converted to cycles would suggest because perf wraps the whole process, including TSV parsing, output writing, timer calls, and control overhead. The phase timers isolate the measured LEAP phase calls better than external perf does.

## Per-k Phase Timing Without Perf

Average CPU ns per pair:

| Backend | k | total | construct_init | load_reads | calculate_masks | reset | run | check_pass | sink |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `leap_avx2` | 1 | 928.153 | 189.523 | 218.727 | 106.243 | 99.818 | 117.222 | 98.717 | 97.902 |
| `leap_avx2` | 2 | 897.582 | 197.987 | 200.898 | 102.935 | 92.428 | 122.309 | 90.807 | 90.218 |
| `leap_avx2` | 3 | 964.809 | 234.173 | 203.760 | 111.007 | 93.545 | 140.752 | 91.262 | 90.310 |
| `leap_avx2` | 4 | 1018.264 | 260.305 | 204.251 | 114.865 | 94.309 | 161.941 | 91.902 | 90.691 |
| `leap_avx2` | 5 | 1081.999 | 294.612 | 204.122 | 120.213 | 95.058 | 185.320 | 91.779 | 90.894 |
| `leap_avx512` | 1 | 866.965 | 182.776 | 198.391 | 98.460 | 92.132 | 113.949 | 90.869 | 90.388 |
| `leap_avx512` | 2 | 916.234 | 209.271 | 198.732 | 104.640 | 92.808 | 128.132 | 90.861 | 91.791 |
| `leap_avx512` | 3 | 962.855 | 231.793 | 198.905 | 110.554 | 94.291 | 146.041 | 90.866 | 90.405 |
| `leap_avx512` | 4 | 1021.581 | 262.731 | 199.454 | 116.173 | 94.264 | 168.028 | 90.565 | 90.366 |
| `leap_avx512` | 5 | 1088.306 | 297.433 | 200.109 | 122.621 | 95.034 | 191.609 | 90.977 | 90.523 |

## What The Memory-Related Numbers Mean

`load_reads` is not “time to read from DRAM”. It is the time spent by LEAP’s `load_reads` function. That includes:

- Loading read/reference bytes from already-parsed heap strings.
- Copying bytes into LEAP object buffers.
- DNA character conversion into bit-plane representation.
- CPU cache hits/misses that happen while doing those operations.
- Normal instruction execution overhead.

`calculate_masks` is also memory-facing, but it is not just memory. It reads the converted bit-plane buffers, applies SIMD operations, and writes LEAP hamming/mismatch masks. It mixes loads, vector operations, and stores.

`run` is the closest isolated measurement for the algorithm traversal after inputs and masks are ready. It still includes memory/cache effects from LEAP internal state: lane arrays, masks, start/end buffers, and object fields.

Perf’s L1D misses/cache misses help explain whether the whole run is memory-stressed. In this run:

- L1D miss rates are low: about 0.245% for AVX2 and 0.294% for AVX512.
- Generic cache miss rates are about 5.58% for AVX2 and 5.33% for AVX512.
- LLC load events are not supported on this CPU/perf event set, so there is no reliable LLC-specific split.

The low L1D miss rate suggests the benchmark is not dominated by L1D load misses. But without supported LLC counters, and because external perf covers the whole process, this does not prove exact DRAM time.

## Interpretation

For length 100, `load_reads + calculate_masks` is about 2.1x the `run` phase for both AVX2 and AVX512. The dominant cost is not the LEAP traversal alone; setup, conversion, mask construction, and small-phase overhead matter a lot at this short length.

AVX512 is not dramatically faster than AVX2 here because the input is only 100 bp. Both backends use one chunk, and the wider vector width is underutilized relative to its setup cost.

The most defensible conclusion is:

```text
For 100 bp threshold verification, LEAP spends more CPU time preparing/encoding input and masks than in the measured run() traversal.
```

The less defensible conclusion would be:

```text
Memory time is exactly X ns and compute time is exactly Y ns.
```

The benchmark does not prove that. It provides phase-level CPU time plus whole-run hardware counters.
