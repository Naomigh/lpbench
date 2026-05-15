# Benchmark protocol

This framework benchmarks short-read pairwise alignment kernels on identical
read-reference pairs.

## Scope

- LEAP means the current AVX512 implementation in `../leap`.
- The original LEAP-BV implementation is excluded.
- Datasets contain simulated short-read read-reference pairs.
- Reads are generated from their paired reference using substitutions,
  insertions, and deletions.
- Levenshtein and affine-gap tasks are reported separately.
- Unsupported method/task combinations must be marked `N/A`, not forced into an
  incompatible comparison.

The first paper-style LEAP-AVX512 speed table uses equal-length mixed-edit
datasets (`SIM-SR100-EQLEN-BAL` and `SIM-SR100-EQLEN-GAP`). Insertions and
deletions are present, but total inserted bases equals total deleted bases per
pair. This avoids the current LEAP wrapper fallback on variable-length pairs and
does not claim coverage of arbitrary variable-length alignment.

## Dataset fairness

Every method is run on the same `pairs.tsv` for a given dataset and threshold.
Dataset generation, input parsing, result collection, and table generation are
outside the measured alignment loop.

## Levenshtein task

The bounded Levenshtein task uses threshold `k`. Ground-truth pass/fail is:

```text
dp_levenshtein_distance <= k
```

The simulated edit count is stored separately. Because random edits can create
accidental equivalences, `dp_levenshtein_distance` is the authoritative
correctness field when present.

## Affine-gap task

The bounded affine task uses threshold `T` and the default scoring convention:

```text
match = 0
mismatch = 2
gap_open = 3
gap_extend = 1
gap cost for length L = gap_open + gap_extend * L
```

The simulated affine score is computed directly from the generated edit events:

```text
mismatch_penalty * substitutions
+ sum insertion events (gap_open + gap_extend * inserted_length)
+ sum deletion events (gap_open + gap_extend * deleted_length)
```

Validation recomputes an independent Gotoh-style affine DP score. The DP score
is the authoritative oracle for benchmark correctness once available.

If a library uses the alternate gap convention
`gap_open + gap_extend * (L - 1)`, wrappers must convert parameters or document
the difference in the raw result `notes` field.

## Timing rules for later phases

1. Read all pairs into memory before timing.
2. Do not include dataset generation, input parsing, or output writing in the
   measured alignment loop.
3. Run one or more warmup iterations before measured repeats.
4. Use at least five measured repeats for the main tables.
5. Report median seconds and median seconds per 10 million pairs.
6. Use a checksum that depends on returned scores, pass/fail values, and input
   lengths.
7. Run single-threaded for the main table.
8. Record CPU model, compiler, flags, operating system, and AVX512 availability.
9. Report correctness mismatches prominently.

## Raw result requirements for later phases

Raw benchmark output is TSV with one row per repeat and includes:

```text
method
mode
dataset
threshold
mismatch
gap_open
gap_extend
repeat
warmup
num_pairs
seconds
pairs_per_second
ns_per_pair
checksum
score_mismatches
pass_mismatches
cigar_replay_failures
notes
```
