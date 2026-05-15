# Method notes

This file is a placeholder for wrapper-specific notes added in later phases.

Required notes for the final benchmark:

- whether each method returns exact score, threshold pass/fail only, CIGAR, or
  multiple of these;
- whether the wrapper uses a native threshold API or exact scoring mode;
- scoring convention conversions for affine-gap methods;
- unsupported method/task combinations;
- confirmation that LEAP uses the AVX512 backend from `../leap`;
- build flags and any dependency patches, if required.

## Phase 4-5 wrapper status

The benchmark harness provides native wrappers for local source trees when they
are enabled at CMake configure time:

- LEAP: `../leap/SIMD_ED.cc`, built with AVX512 flags.
- edlib: `../edlib/edlib/src/edlib.cpp`.
- lv89: `../lv89/lv89.c`.
- miniWFA: `../miniwfa/miniwfa.c`.
- KSW2: `../ksw2/ksw2_gg2.c`.
- WFA2: `../WFA2-lib`, via its CMake `wfa2cpp_static` target.

Parasail is currently represented by an oracle fallback wrapper. Enabling a true
parasail wrapper is deferred because the local parasail tree is a full generated
library with many translation units and should be linked through its own build
target rather than partially compiled.

If a native wrapper is disabled or unavailable, `bench_align` still emits raw TSV
rows using an exact oracle fallback and records `backend_actual=oracle_fallback`
in `notes`. Such rows are useful for harness smoke tests but must not be used as
method performance numbers in final tables.

The LEAP native wrapper records `backend_actual=avx512`. For pairs whose read
and reference lengths differ, the current wrapper falls back to the oracle and
records `reason=leap_native_requires_equal_length_le_512`; this is deliberately
visible in raw results so it cannot be mistaken for a LEAP timing.

The main equal-length datasets are intended to produce `fallback_used=false` for
LEAP. `collect_results.py` writes `processed/leap_backend_audit.tsv` and
`reports/leap_backend_audit.md`; any fallback in an EQLEN dataset is a benchmark
readiness failure.

The WFA2 affine wrapper converts benchmark scoring convention
`gap_open + gap_extend * L` to WFA2's constructor convention by passing
`gapOpening = gap_open + gap_extend`. The Phase 4-5 wrapper uses exact WFA2
scoring rather than an early-exit threshold because WFA2's step limit is not the
same unit as weighted affine score. The current affine wrapper marks exact score
as unavailable in raw correctness accounting until Phase 6 validates score
normalization against CIGAR replay / Gotoh DP across thresholds.
