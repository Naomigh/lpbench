#!/usr/bin/env bash
set -u

out_dir="${1:-benchmark/results/summary}"
mkdir -p "$out_dir"
out="$out_dir/environment.txt"

{
  date
  hostname
  uname -a
  lscpu
  gcc --version
  g++ --version
  cmake --version
  perf --version
  cat /proc/cmdline
  cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null | sort | uniq -c
  cat /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
} > "$out"

echo "Wrote $out"
