# Benchmark report

LEAP means the current AVX512 implementation in `./leap`; original LEAP-BV is not included.

Speed tables below report measured time even when a row has correctness mismatches. Correctness issues are listed separately and are not used to hide timing numbers.

The first LEAP-AVX512 benchmark uses equal-length mixed-edit short-read datasets. Insertions and deletions are present, but total inserted bases equals total deleted bases per pair. This avoids the current LEAP wrapper fallback on variable-length pairs and measures the native AVX512 path. These datasets do not cover arbitrary variable-length alignment cases.

## LEAP exact phase timing

| method | dataset | pairs | preprocess ns/pair | forward ns/pair | traceback ns/pair | total ns/pair | preprocess % | forward % | traceback % | score mismatches | cigar failures |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| leap | SIM-SR100-EQLEN-BAL | 10000 | 1040050.00 | 1575.04 | 1359.39 | 1042990.00 | 99.72 | 0.15 | 0.13 | 0 | 0 |


## Levenshtein speed

# Levenshtein speed table

Unit: seconds per 10 million pairs. `N/A` means unsupported, failed, fallback, or unavailable.

| k | LEAP-AVX512 | edlib | lv89 | miniWFA | WFA2 | KSW2 | Best non-LEAP | Speedup vs best non-LEAP |
|---|---|---|---|---|---|---|---|---|
| 1 | 12031.600 | 13.906 | 12.215 | 5.579 | 12.431 | 506.933 | 5.579 | 0.00x |
| 2 | 12097.700 | 15.252 | 10.204 | 6.191 | 12.364 | 507.047 | 6.191 | 0.00x |
| 3 | 11914.400 | 16.184 | 9.883 | 6.556 | 12.797 | 506.858 | 6.556 | 0.00x |
| 5 | 12055.600 | 17.771 | 10.248 | 7.462 | 12.407 | 505.965 | 7.462 | 0.00x |
| 8 | 11994.300 | 19.358 | 10.341 | 8.775 | 12.411 | 505.728 | 8.775 | 0.00x |


## Affine speed

# Affine gap speed table

Unit: seconds per 10 million pairs. `N/A` means unsupported, failed, fallback, or unavailable.

| T | LEAP-AVX512 | miniWFA | WFA2-affine | KSW2 | Best non-LEAP | Speedup vs best non-LEAP |
|---|---|---|---|---|---|---|
| 5 | N/A | 126.831 | 162.756 | 628.572 | 126.831 | N/A |
| 10 | N/A | 129.332 | 166.621 | 627.772 | 129.332 | N/A |
| 15 | N/A | 132.848 | 164.572 | 628.319 | 132.848 | N/A |
| 20 | N/A | 138.462 | 163.925 | 628.936 | 138.462 | N/A |
| 30 | N/A | 145.123 | 165.959 | 624.669 | 145.123 | N/A |


## Correctness issues

# Correctness issues

These rows have nonzero or unavailable correctness counters. Timings are still shown in the speed tables.

| mode | dataset | method | threshold | pairs | score mismatches | pass/fail mismatches | CIGAR replay failures | notes |
|---|---|---|---|---|---|---|---|---|
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 1 | 10000 | 1574 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 2 | 10000 | 357 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| levenshtein | SIM-SR100-EQLEN-BAL | lv89 | 3 | 10000 | 45 | 0 | 0 | backend_actual=lv89;mode=global;bandwidth=threshold;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 10 | 10000 | 0 | 6968 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 15 | 10000 | 0 | 5923 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 20 | 10000 | 0 | 4430 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 30 | 10000 | 0 | 1836 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |
| affine | SIM-SR100-EQLEN-GAP | wfa2 | 5 | 10000 | 0 | 8490 | 0 | backend_actual=wfa2;memory=high;scope=score;threshold_mode=exact_score;score_semantics=pass_fail_only_affine_normalization_pending;dataset_equal_length=true |


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
| affine | SIM-SR100-EQLEN-GAP | leap | 10 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;backend_actual=oracle_fallback;reason=leap_native_requires_equal_length_le_512;dataset_equal_length=true;fallback_used=true |
| affine | SIM-SR100-EQLEN-GAP | leap | 15 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;backend_actual=oracle_fallback;reason=leap_native_requires_equal_length_le_512;dataset_equal_length=true;fallback_used=true |
| affine | SIM-SR100-EQLEN-GAP | leap | 20 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;backend_actual=oracle_fallback;reason=leap_native_requires_equal_length_le_512;dataset_equal_length=true;fallback_used=true |
| affine | SIM-SR100-EQLEN-GAP | leap | 30 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;backend_actual=oracle_fallback;reason=leap_native_requires_equal_length_le_512;dataset_equal_length=true;fallback_used=true |
| affine | SIM-SR100-EQLEN-GAP | leap | 5 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;backend_actual=oracle_fallback;reason=leap_native_requires_equal_length_le_512;dataset_equal_length=true;fallback_used=true |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 1 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=exact_score;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 2 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=exact_score;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 3 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=exact_score;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 5 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=exact_score;dataset_equal_length=true;fallback_used=false |
| levenshtein | SIM-SR100-EQLEN-BAL | leap | 8 | 10000 | 0 | 0 | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=exact_score;dataset_equal_length=true;fallback_used=false |
| levenshtein_exact | SIM-SR100-EQLEN-BAL | leap | exact | 10000 | 0 | NA | 0 | backend_actual=avx512;implementation=../leap/SIMD_ED;score_semantics=exact_score_and_cigar;dataset_equal_length=true;fallback_used=false |
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


Unsupported, failed, and fallback rows are reported as N/A in speed tables.
