#!/bin/sh
set -u

OUT="${1:-benchmark/results/summary/environment.txt}"
mkdir -p "$(dirname "$OUT")"
{
  date
  hostname
  uname -a
  lscpu
  gcc --version
  g++ --version
  cmake --version
  perf --version 2>/dev/null || true
  cat /proc/cmdline
  cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null | sort | uniq -c || true
  cat /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
} > "$OUT"

