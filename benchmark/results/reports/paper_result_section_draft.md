# Paper-style result section draft

We evaluated LEAP-AVX512 as a short-read bounded alignment kernel on simulated equal-length mixed-edit read-reference pairs. The datasets include substitutions, insertions, and deletions; insertion and deletion base counts are balanced per pair so the final read length equals the reference length. This design measures the current AVX512 path and should not be interpreted as coverage of all variable-length alignment cases.

Levenshtein and affine-gap conditions are reported separately. Unsupported or fallback method/task combinations are marked N/A, and correctness mismatches are reported alongside speed results.
