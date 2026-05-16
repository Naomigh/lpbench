# LEAP backend audit

This audit checks whether LEAP benchmark rows used the AVX512 backend or fell back.

| mode | dataset | threshold | total pairs | equal length | non-equal length | AVX512 pairs | fallback pairs | reasons |
|---|---|---:|---:|---:|---:|---:|---:|---|
| affine | SIM-SR100-EQLEN-GAP | 10 | 10000 | 10000 | 0 | 0 | 10000 | backend_actual=oracle_fallback;fallback_used=true;reason=leap_native_requires_equal_length_le_512 |
| affine | SIM-SR100-EQLEN-GAP | 15 | 10000 | 10000 | 0 | 0 | 10000 | backend_actual=oracle_fallback;fallback_used=true;reason=leap_native_requires_equal_length_le_512 |
| affine | SIM-SR100-EQLEN-GAP | 20 | 10000 | 10000 | 0 | 0 | 10000 | backend_actual=oracle_fallback;fallback_used=true;reason=leap_native_requires_equal_length_le_512 |
| affine | SIM-SR100-EQLEN-GAP | 30 | 10000 | 10000 | 0 | 0 | 10000 | backend_actual=oracle_fallback;fallback_used=true;reason=leap_native_requires_equal_length_le_512 |
| affine | SIM-SR100-EQLEN-GAP | 5 | 10000 | 10000 | 0 | 0 | 10000 | backend_actual=oracle_fallback;fallback_used=true;reason=leap_native_requires_equal_length_le_512 |
| levenshtein | SIM-SR100-EQLEN-BAL | 1 | 10000 | 10000 | 0 | 10000 | 0 |  |
| levenshtein | SIM-SR100-EQLEN-BAL | 2 | 10000 | 10000 | 0 | 10000 | 0 |  |
| levenshtein | SIM-SR100-EQLEN-BAL | 3 | 10000 | 10000 | 0 | 10000 | 0 |  |
| levenshtein | SIM-SR100-EQLEN-BAL | 5 | 10000 | 10000 | 0 | 10000 | 0 |  |
| levenshtein | SIM-SR100-EQLEN-BAL | 8 | 10000 | 10000 | 0 | 10000 | 0 |  |
| levenshtein_exact | SIM-SR100-EQLEN-BAL | exact | 10000 | 10000 | 0 | 10000 | 0 |  |

Fallback was observed and must be investigated before using these LEAP rows in final speed tables.
