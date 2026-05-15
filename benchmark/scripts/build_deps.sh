#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BENCH_DIR="${ROOT}/benchmark"
BUILD_DIR="${BENCH_DIR}/build"
LOG="${BENCH_DIR}/results/reports/build_log.txt"

mkdir -p "${BUILD_DIR}" "$(dirname "${LOG}")"

{
  echo "Build started: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "Root: ${ROOT}"
  echo "Compiler:"
  ${CXX:-c++} --version || true
  echo
  cmake -S "${BENCH_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBENCH_USE_LEAP=ON \
    -DBENCH_USE_EDLIB=ON \
    -DBENCH_USE_LV89=ON \
    -DBENCH_USE_MINIWFA=ON \
    -DBENCH_USE_KSW2=ON \
    -DBENCH_USE_WFA2=ON
  cmake --build "${BUILD_DIR}" --parallel "$(nproc 2>/dev/null || echo 1)"
  echo
  echo "Build finished: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "Executable: ${BUILD_DIR}/bench_align"
} 2>&1 | tee "${LOG}"
