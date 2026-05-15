# Dataset format

Each generated dataset is stored under:

```text
benchmark/data/generated/<DATASET_NAME>/
  pairs.tsv
  metadata.tsv
  manifest.json
```

## pairs.tsv

Tab-separated columns:

```text
pair_id
reference
read
```

Sequences use only the configured alphabet, normally `ACGT`.

## metadata.tsv

Tab-separated columns:

```text
pair_id
dataset_name
  ref_length
  read_length
  equal_length
  bucket
num_substitutions
num_inserted_bases
num_deleted_bases
num_insertion_events
num_deletion_events
simulated_levenshtein_distance
  dp_levenshtein_distance
  simulated_affine_score
  gotoh_affine_score
  edit_script
  seed
  random_seed
```

`bucket` is the stratification bucket. For balanced datasets it is the requested
Levenshtein edit count. For gap-heavy datasets it is the simulated affine score.

`edit_script` is JSON. Events are ordered by reference coordinate. The generator
stores the edit events used to create the read; matches are implicit. Files may
also contain explicit `M` records, and validators should tolerate them. Example:

```json
[
  {"op":"M","ref_pos":0,"base":"A"},
  {"op":"X","ref_pos":10,"from":"C","to":"T"},
  {"op":"I","ref_pos":20,"seq":"AG"},
  {"op":"D","ref_pos":31,"seq":"TGA"}
]
```

Operation meanings:

- `M`: optional explicit match, consumes one reference base and emits the same
  read base.
- `X`: substitution, consumes one reference base and emits a different read
  base.
- `I`: insertion, consumes no reference bases and emits one or more read bases.
- `D`: deletion, consumes one or more reference bases and emits no read bases.

## manifest.json

The manifest records:

- dataset name;
- generation timestamp;
- generator version;
- random seed;
- reference length;
- number of pairs;
- alphabet;
- error model;
- scoring parameters;
- bucket statistics;
- aggregate edit counts;
- SHA-256 checksums of `pairs.tsv` and `metadata.tsv`.

## Ground truth

For Levenshtein datasets:

```text
simulated_levenshtein_distance =
  num_substitutions + num_inserted_bases + num_deleted_bases
```

The generator also computes exact DP Levenshtein distance. If simulated and DP
distances differ, benchmark correctness should use `dp_levenshtein_distance`.
For large datasets this field may be blank at generation time; run
`validate_dataset.py` on a representative sample or the full dataset to produce
independent oracle validation reports.

For affine datasets, `simulated_affine_score` is computed from the generated
events under the documented convention:

```text
gap cost for length L = gap_open + gap_extend * L
```

Validation can additionally compute an independent Gotoh DP affine score.

## Equal-Length Models

`eqlen_balanced` and `eqlen_gap` datasets guarantee:

```text
len(read) == len(reference)
num_inserted_bases == num_deleted_bases
equal_length == true
```

They are mixed-edit datasets: substitutions, insertions, and deletions occur in
the edit scripts. They are designed to measure the current LEAP AVX512 native
path without triggering fallback caused by variable read/reference lengths.
