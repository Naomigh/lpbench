# Benchmark report

LEAP means the current AVX512 implementation in `./leap`; original LEAP-BV is not included.

The first LEAP-AVX512 benchmark uses equal-length mixed-edit short-read datasets. Insertions and deletions are present, but total inserted bases equals total deleted bases per pair. This avoids the current LEAP wrapper fallback on variable-length pairs and measures the native AVX512 path. These datasets do not cover arbitrary variable-length alignment cases.

## Levenshtein speed

# Levenshtein speed table

Unit: seconds per 10 million pairs. `N/A` means unsupported, failed, fallback, or unavailable.

| k | LEAP-AVX512 | edlib | lv89 | miniWFA | WFA2 | KSW2 | Best non-LEAP | Speedup vs best non-LEAP |
|---|---|---|---|---|---|---|---|---|
| 1 | N/A | 13.728 | N/A | 5.609 | 12.607 | 505.397 | 5.609 | N/A |
| 2 | N/A | 15.316 | N/A | 6.068 | 12.131 | 507.006 | 6.068 | N/A |
| 3 | N/A | 16.017 | N/A | 6.517 | 12.167 | 503.430 | 6.517 | N/A |
| 5 | N/A | 17.436 | 10.337 | 7.544 | 14.050 | 505.603 | 7.544 | N/A |
| 8 | N/A | 19.200 | 10.360 | 8.915 | 12.120 | 503.450 | 8.915 | N/A |


## Affine speed

# Affine gap speed table

Unit: seconds per 10 million pairs. `N/A` means unsupported, failed, fallback, or unavailable.

| T | LEAP-AVX512 | miniWFA | WFA2-affine | KSW2 | Best non-LEAP | Speedup vs best non-LEAP |
|---|---|---|---|---|---|---|
| 5 | 156.856 | 127.548 | N/A | 628.279 | 127.548 | 0.81x |
| 10 | N/A | 130.822 | N/A | 625.067 | 130.822 | N/A |
| 15 | N/A | 133.198 | N/A | 632.662 | 133.198 | N/A |
| 20 | N/A | 135.739 | N/A | 625.342 | 135.739 | N/A |
| 30 | N/A | 148.477 | N/A | 625.432 | 148.477 | N/A |


## Correctness

# Correctness table

Rows with nonzero mismatches are not eligible for final speed claims.

| mode | dataset | method | threshold | pairs | score mismatches | pass/fail mismatches | CIGAR replay failures | notes |
|---|---|---|---|---|---|---|---|---|
| levenshtein | SIM-SR100-EQLEN-BAL | edlib | 1 | 10000 | 0 | 0 | 0 | backend_actual=edlib;score_semantics=exact_if_within_threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | edlib | 2 | 10000 | 0 | 0 | 0 | backend_actual=edlib;score_semantics=exact_if_within_threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | edlib | 3 | 10000 | 0 | 0 | 0 | backend_actual=edlib;score_semantics=exact_if_within_threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | edlib | 5 | 10000 | 0 | 0 | 0 | backend_actual=edlib;score_semantics=exact_if_within_threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | edlib | 8 | 10000 | 0 | 0 | 0 | backend_actual=edlib;score_semantics=exact_if_within_threshold;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | ksw2 | 10 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | ksw2 | 15 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | ksw2 | 20 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | ksw2 | 30 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | ksw2 | 5 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | ksw2 | 1 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | ksw2 | 2 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | ksw2 | 3 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | ksw2 | 5 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | ksw2 | 8 | 10000 | 0 | 0 | 0 | backend_actual=ksw2;mode=global;score_negated;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | leap | 10 | 10000 | 0 | 394 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| affine | SIM-SR100-EQLEN-GAP | leap | 15 | 10000 | 0 | 498 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| affine | SIM-SR100-EQLEN-GAP | leap | 20 | 10000 | 0 | 728 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| affine | SIM-SR100-EQLEN-GAP | leap | 30 | 10000 | 0 | 778 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| affine | SIM-SR100-EQLEN-GAP | leap | 5 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 1 | 10000 | 0 | 8401 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 2 | 10000 | 0 | 1339 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 3 | 10000 | 0 | 7094 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 5 | 10000 | 0 | 5975 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 8 | 10000 | 0 | 3707 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=pass_fail_only_until_cigar_replay;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 1 | 10000 | 1574 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 2 | 10000 | 357 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 3 | 10000 | 45 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 5 | 10000 | 0 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 8 | 10000 | 0 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | miniwfa | 10 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | miniwfa | 15 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | miniwfa | 20 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | miniwfa | 30 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | miniwfa | 5 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | miniwfa | 1 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | miniwfa | 2 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | miniwfa | 3 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | miniwfa | 5 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | miniwfa | 8 | 10000 | 0 | 0 | 0 | backend_actual=miniwfa;gap_convention=gap_open+gap_extend*L;max_s_exceeded_or_failed;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 10 | 10000 | 0 | 6968 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 15 | 10000 | 0 | 5923 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 20 | 10000 | 0 | 4430 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 30 | 10000 | 0 | 1836 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 5 | 10000 | 0 | 8490 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | wfa2 | 1 | 10000 | 0 | 0 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | wfa2 | 2 | 10000 | 0 | 0 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | wfa2 | 3 | 10000 | 0 | 0 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | wfa2 | 5 | 10000 | 0 | 0 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | wfa2 | 8 | 10000 | 0 | 0 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;dataset_equal_length=true |


Unsupported methods and fallback rows are reported as N/A in speed tables. Correctness mismatches must be resolved before using a row as a final performance number.
