#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT}/benchmark/build"
DATA_DIR="${ROOT}/benchmark/data"
RESULT_DIR="${ROOT}/benchmark/results"

mkdir -p "${BUILD_DIR}" "${DATA_DIR}" "${RESULT_DIR}/raw"

cmake -S "${ROOT}/benchmark" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j"$(nproc)"

MAX_EDITS="${MAX_EDITS:-0 1 2 3 4 5 6 7 8 9 10}"
LEAP_MAX_ENERGY="${LEAP_MAX_ENERGY:-10}"
READ_LEN="${READ_LEN:-100}"
REF_LEN="${REF_LEN:-${READ_LEN}}"
LEAP_CHUNK_SIZE="${LEAP_CHUNK_SIZE:-0}"

if (( READ_LEN != REF_LEN )); then
  echo "ERROR: this all-method benchmark includes LEAP, whose current wrapper requires READ_LEN == REF_LEN." >&2
  echo "       Set READ_LEN and REF_LEN to the same value for LEAP runs." >&2
  exit 1
fi

for max_edit in ${MAX_EDITS}; do
  len_diff=$(( READ_LEN > REF_LEN ? READ_LEN - REF_LEN : REF_LEN - READ_LEN ))
  if (( len_diff > max_edit )); then
    echo "ERROR: abs(READ_LEN - REF_LEN)=${len_diff} exceeds max_edit=${max_edit}" >&2
    exit 1
  fi
done

for max_edit in ${MAX_EDITS}; do
  python3 "${ROOT}/benchmark/scripts/generate_pairs.py" \
    --num-pairs "${NUM_PAIRS:-10000}" \
    --read-len "${READ_LEN}" \
    --reference-len "${REF_LEN}" \
    --max-edit "${max_edit}" \
    --seed "$((1000 + max_edit))" \
    --edlib-helper "${BUILD_DIR}/edlib_gt" \
    --out "${DATA_DIR}/pairs_ed${max_edit}.tsv"
done

python3 "${ROOT}/benchmark/scripts/run_benchmark.py" \
  --build-dir "${BUILD_DIR}" \
  --data-dir "${DATA_DIR}" \
  --max-edits ${MAX_EDITS} \
  --leap-max-energy "${LEAP_MAX_ENERGY}" \
  --leap-chunk-size "${LEAP_CHUNK_SIZE}" \
  --raw-out "${RESULT_DIR}/raw/benchmark_raw.tsv" \
  --summary-out "${RESULT_DIR}/benchmark_summary.md"

echo "Wrote ${RESULT_DIR}/raw/benchmark_raw.tsv"
echo "Wrote ${RESULT_DIR}/benchmark_summary.md"
