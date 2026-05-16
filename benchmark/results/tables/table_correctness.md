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
