# Execution Paths

## Dataset And File Path Common To All Methods

1. Dataset TSV resides on disk / filesystem storage.
2. Before timing, benchmark opens and reads the TSV.
3. OS page cache may serve the file if it was previously read.
4. The benchmark parses each row into in-memory arrays/vectors.
5. read/reference sequences are stored in heap memory, preferably contiguous buffers.
6. ground-truth pass/fail labels are stored in memory.
7. File I/O and TSV parsing happen before timing and are excluded.
8. During timing, methods read read/reference bytes from heap memory.
9. The CPU accesses these bytes through cache hierarchy: memory / LLC / L2 / L1D -> registers.
10. The exact cache level depends on warm/cold mode, previous accesses, hardware prefetching, and replacement policy.
11. Results are written to a volatile sink and a preallocated result array.
12. Stores first update cache lines, then are eventually written back to memory by hardware.

The benchmark does not manually "put data into cache". It touches/warmups data to influence cache residency. Actual cache placement and eviction are controlled by CPU hardware.

## LEAP AVX2 / AVX512 Execution Path

Input:

- read/reference bytes are already in heap memory.
- timed code receives char pointers.

Main threshold benchmark timing path:

1. Before timing, CPU loads read/reference bytes from memory/cache as required by preparation.
2. Before timing, LEAP constructs `SIMD_ED` object.
3. Before timing, `init_levenshtein` initializes threshold/max energy state.
4. Before timing, `load_reads` copies/loads sequence data into LEAP internal representation.
5. Before timing, DNA chars are converted into LEAP bit-plane / bit-vector representation.
6. Before timing, `calculate_masks` builds mismatch/match masks.
7. Before timing, `reset` initializes LEAP state for traversal.
8. Timed region: `run` executes the LEAP bit-vector lane/state transition.
9. After timing, `check_pass` reads final state and returns threshold pass/fail.
10. After timing, pass/fail is compared against stored labels for mismatch counting.

The main `threshold_raw.tsv` rows for `leap_avx2` and `leap_avx512` therefore use `mode=forward` and measure only LEAP forward/run time. The separate `leap_phase_bench` still reports per-phase timings for construct/init, load, mask, reset, run, check, and sink.

Chunking:

- AVX2 uses 256 bp chunks.
- AVX512 uses 512 bp chunks.
- Pair passes only if every chunk passes.

Memory/cache note:

- read/reference input bytes are loaded through cache hierarchy.
- LEAP internal bit-vectors/masks are stored in stack or heap/object memory depending on implementation.
- SIMD operations use vector registers after data is loaded.
- Stores to result variables and internal state update cache lines first; write-back to main memory is hardware-managed.

## lv89 Execution Path

Input:

- read/reference bytes are already in heap memory.
- timed code passes char pointers and length to lv89.

Timed path:

1. CPU loads read/reference bytes from memory/cache when lv89 accesses them.
2. lv89 runs its edit-distance/banded algorithm.
3. The benchmark passes `k` as the bandwidth/threshold argument.
4. The wrapper extracts the score and compares `score <= k`.
5. Return pass/fail.
6. Store pass/fail to sink/result array.

Memory/cache note:

- lv89 internal DP/bit-vector state lives in its own stack/heap buffers depending on the implementation.
- The benchmark does not count file I/O.
- It counts memory/cache effects during lv89 execution.

## miniwfa Execution Path

Input:

- read/reference bytes are already in heap memory.

Timed path:

1. CPU loads read/reference bytes from memory/cache during miniwfa execution.
2. Initialize miniwfa options for edit-distance-like scoring.
3. Disable CIGAR/traceback.
4. Set `max_s = k`.
5. Run `mwf_wfa_exact`.
6. Extract score.
7. Free temporary result/cigar if allocated.
8. Return `score <= k`.
9. Store pass/fail to sink/result array.

Memory/cache note:

- miniwfa may allocate or touch temporary wavefront/state memory.
- These accesses are part of timed CPU time.

## WFA2-lib Execution Path

`wfa2_fresh`:

1. Create wavefront aligner inside timed verify call.
2. Run `wavefront_align` in edit-distance score-only mode.
3. Extract score.
4. Delete aligner.
5. Return `score <= k`.

`wfa2_reuse`:

1. Create aligner before timed loop.
2. Inside timed loop, reap/reset aligner state.
3. Run `wavefront_align`.
4. Extract score.
5. Return `score <= k`.
6. Delete aligner after timing.

Memory/cache note:

- `wfa2_fresh` includes per-pair allocation/init/delete overhead.
- `wfa2_reuse` excludes aligner creation but includes per-pair reset/reap.
- These are reported as separate methods and never merged.

