# LEAP short-read alignment benchmark

This benchmark framework evaluates the current AVX512 implementation in `../leap`
against local baseline repositories:

- `../edlib`
- `../lv89`
- `../miniwfa`
- `../ksw2`
- `../parasail`
- `../WFA2-lib`

The original LEAP-BV implementation is not part of this benchmark. LEAP means the
updated AVX512 implementation in `./leap` relative to the repository root.

The framework is self-contained under `benchmark/`. Generated datasets and
benchmark outputs are written only under `benchmark/data/generated` and
`benchmark/results`.

## LEAP exact phase benchmark

The LEAP benchmark now measures exact Levenshtein alignment, not threshold
pass/fail verification. The exact path is selected with `--mode
levenshtein_exact` and does not require `--threshold`.

For LEAP, timing is split into three phases:

1. preprocessing: input loading, bit-vector conversion, mismatch-mask
   generation, and state allocation/reset;
2. forward: recurrence evaluation until the bottom-right terminal state is
   reached;
3. traceback: traceback from the final state and CIGAR generation.

Correctness is evaluated only by exact score and CIGAR replay. The expected
score is `dp_levenshtein_distance` from `metadata.tsv`; missing values are an
error in the exact benchmark. The replay check requires the CIGAR to consume the
entire read and reference and to reproduce the exact edit distance. Threshold
pass/fail checks are not part of the new LEAP exact benchmark.

Run the phase benchmark with:

```bash
python3 benchmark/scripts/run_leap_phase_bench.py \
  --dataset benchmark/data/generated/SIM-SR100-EQLEN-BAL \
  --repeat 3 \
  --warmup 1
```

This produces:

- `benchmark/results/raw/leap_exact.tsv`
- `benchmark/results/leap_phase_timing.tsv`
- `benchmark/results/leap_phase_timing.md`

The underlying command is:

```bash
benchmark/build/bench_align \
  --method leap \
  --mode levenshtein_exact \
  --input benchmark/data/generated/SIM-SR100-EQLEN-BAL/pairs.tsv \
  --metadata benchmark/data/generated/SIM-SR100-EQLEN-BAL/metadata.tsv \
  --repeat 1 \
  --warmup 1 \
  --output benchmark/results/raw/leap_exact.tsv \
  --phase-output benchmark/results/leap_phase_timing.tsv
```

## Phase 1-3 status

Implemented now:

1. Folder structure and documentation.
2. Synthetic short-read dataset generation with substitutions, insertions, and
   deletions.
3. Dataset validation with exact Levenshtein and affine-gap DP oracles.

Benchmark harnesses and method wrappers are available under `benchmark/src`.

## Dataset generation

Balanced Levenshtein dataset:

```bash
python3 benchmark/scripts/generate_dataset.py \
  --name SIM-SR100-EQLEN-BAL \
  --model eqlen_balanced \
  --num-pairs 100000 \
  --ref-length 100 \
  --max-distance 15 \
  --seed 42 \
  --output-dir benchmark/data/generated
```

Gap-heavy affine dataset:

```bash
python3 benchmark/scripts/generate_dataset.py \
  --name SIM-SR100-EQLEN-GAP \
  --model eqlen_gap \
  --num-pairs 100000 \
  --ref-length 100 \
  --max-affine-score 40 \
  --mismatch-penalty 2 \
  --gap-open-penalty 3 \
  --gap-extend-penalty 1 \
  --mean-gap-length 3 \
  --max-gap-length 20 \
  --seed 43 \
  --output-dir benchmark/data/generated
```

The generator supports `--model balanced`, `--model gap`, `--model
eqlen_balanced`, and `--model eqlen_gap`. If `--model` is omitted, it infers the
dataset family from `--name`:

- names ending in `-BAL` generate balanced Levenshtein data;
- names ending in `-GAP` generate gap-heavy affine data.
- names containing `-EQLEN-` generate equal-length mixed-edit data.

The first LEAP-AVX512 benchmark uses equal-length mixed-edit datasets. Insertions
and deletions are still present, but total inserted bases equals total deleted
bases per pair, so `len(read) == len(reference)`. This avoids the current LEAP
wrapper fallback on variable-length pairs and measures the native AVX512 path.
These datasets evaluate short-read bounded alignment under balanced indel
conditions, not arbitrary variable-length alignment.

By default, generation does not run exact DP for every pair, because the intended
10M-pair datasets would make that prohibitively slow in Python. For small debug
datasets, add `--dp-validation all` to fill `dp_levenshtein_distance` during
generation. The separate validator recomputes DP oracles on a configurable
sample and writes validation reports.

## Validation

```bash
python3 benchmark/scripts/validate_dataset.py \
  --dataset benchmark/data/generated/SIM-SR100-EQLEN-BAL \
  --mode levenshtein \
  --sample 100000
```

```bash
python3 benchmark/scripts/validate_dataset.py \
  --dataset benchmark/data/generated/SIM-SR100-EQLEN-GAP \
  --mode affine \
  --sample 100000 \
  --mismatch 2 \
  --gap-open 3 \
  --gap-extend 1
```

Validation reports are written to:

- `benchmark/results/processed/<DATASET_NAME>_validation.tsv`
- `benchmark/results/reports/<DATASET_NAME>_validation.md`

## File layout

See `docs/dataset_format.md` and `docs/benchmark_protocol.md` for the detailed
format and protocol.
