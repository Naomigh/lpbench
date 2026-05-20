# Execution Paths

This document describes the logical execution path requested by the benchmark and the likely CPU/cache/memory behavior. It does not claim exact cache residency for every byte.

## Dataset and File Path Common to All Methods

1. Dataset TSV resides on disk / filesystem storage.
2. Before timing, benchmark opens and reads the TSV.
3. OS page cache may serve the file if it was previously read.
4. The benchmark parses each row into in-memory arrays/vectors.
5. Read/reference sequences are stored in heap memory as `std::string` payloads inside dataset records.
6. Ground-truth pass/fail labels are stored in memory.
7. File I/O and TSV parsing happen before timing and are excluded.
8. During timing, methods read read/reference bytes from heap memory.
9. The CPU accesses these bytes through cache hierarchy: memory / LLC / L2 / L1D -> registers.
10. The exact cache level depends on warm/cold mode, previous accesses, hardware prefetching, and replacement policy.
11. Results are written to a volatile sink and a preallocated result array.
12. Stores first update cache lines, then are eventually written back to memory by hardware.

The benchmark does not manually "put data into cache". It touches/warmups data to influence cache residency. Actual cache placement and eviction are controlled by CPU hardware.

## LEAP AVX2 / AVX512 Execution Path

Input:

- Read/reference bytes are already in heap memory.
- Timed code receives char pointers.

Timed path:

1. CPU loads read/reference bytes from memory/cache as required.
2. LEAP constructs SIMD_ED object.
3. `init_levenshtein` initializes threshold/max energy state.
4. `load_reads` copies/loads sequence data into LEAP internal representation.
5. DNA chars are converted into LEAP bit-plane / bit-vector representation.
6. `calculate_masks` builds mismatch/match masks.
7. `reset` initializes LEAP state for traversal.
8. `run` executes the LEAP bit-vector lane/state transition.
9. `check_pass` reads final state and returns threshold pass/fail.
10. Boolean result is written to sink / result array.

Chunking:

- AVX2 uses 256 bp chunks.
- AVX512 uses 512 bp chunks.
- Pair passes only if every chunk passes.

Memory/cache note:

- Read/reference input bytes are loaded through cache hierarchy.
- LEAP internal bit-vectors/masks are stored in stack or heap/object memory depending on implementation.
- SIMD operations use vector registers after data is loaded.
- Stores to result variables and internal state update cache lines first; write-back to main memory is hardware-managed.

The LEAP wrappers call the cloned `SIMD_ED` implementation with backend-specific compilation flags. SHD prefiltering is disabled in the threshold benchmark wrapper.

## lv89 Execution Path

Input:

- Read/reference bytes are already in heap memory.
- Timed code passes char pointers and length to lv89.

Timed path:

1. CPU loads read/reference bytes from memory/cache when lv89 accesses them.
2. lv89 runs its edit-distance/banded algorithm.
3. If lv89 supports a band/threshold parameter, pass `k` as the band/threshold.
4. If it does not stop exactly at threshold, compute score and then compare `score <= k`.
5. Extract score and endpoint information if needed for global alignment.
6. Return pass/fail.
7. Store pass/fail to sink/result array.

Memory/cache note:

- lv89 internal DP/bit-vector state lives in its own stack/heap buffers depending on the implementation.
- The benchmark does not count file I/O.
- It counts memory/cache effects during lv89 execution.

The lv89 wrapper calls `lv_ed_basic` in score-only mode with `bw=k`.

## miniwfa Execution Path

Input:

- Read/reference bytes are already in heap memory.

Timed path:

1. CPU loads read/reference bytes from memory/cache during miniwfa execution.
2. Initialize miniwfa options for edit-distance-like scoring.
3. Disable CIGAR/traceback.
4. Set `max_s = k` if supported.
5. Run `mwf_wfa_exact` or the appropriate miniwfa score function.
6. Extract score.
7. Free temporary result/cigar if allocated.
8. Return `score <= k`.
9. Store pass/fail to sink/result array.

Memory/cache note:

- miniwfa may allocate or touch temporary wavefront/state memory.
- These accesses are part of timed CPU time.

The miniwfa wrapper calls `mwf_wfa_exact` in score-only mode with CIGAR disabled and `max_s=k`.

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
- Report these as separate methods and never merge them.

The WFA2 wrappers use `wfa::WFAlignerEdit` in score-only mode, with separate fresh and reused aligner paths.
