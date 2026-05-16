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
