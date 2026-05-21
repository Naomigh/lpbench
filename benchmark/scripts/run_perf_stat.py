#!/usr/bin/env python3
import argparse
import csv
import pathlib
import subprocess
import tempfile


EVENTS = [
    "cycles",
    "instructions",
    "branches",
    "branch-misses",
    "cache-references",
    "cache-misses",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "LLC-loads",
    "LLC-load-misses",
    "dTLB-loads",
    "dTLB-load-misses",
]


def parse_perf(path):
    values = {}
    with open(path, newline="") as fh:
        for row in csv.reader(fh):
            if len(row) < 3:
                continue
            raw_value, _, event = row[:3]
            try:
                values[event] = int(float(raw_value))
            except ValueError:
                values[event] = 0
    return values


def rate(num, den):
    return 0.0 if den == 0 else num / den


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", default="benchmark/build/threshold_bench")
    parser.add_argument("--dataset", default="benchmark/data/generated/pairs_len100_ed0_15.tsv")
    parser.add_argument("--length", type=int, default=100)
    parser.add_argument("--ks", nargs="+", type=int, default=[1, 2, 3, 4, 5])
    parser.add_argument("--methods", nargs="+", required=True)
    parser.add_argument("--cache-mode", default="warm")
    parser.add_argument("--core", type=int, default=2)
    parser.add_argument("--out-dir", default="benchmark/results")
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()

    out = pathlib.Path(args.out_dir) / "raw" / "perf_summary.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    write_header = not out.exists()
    with out.open("a", newline="") as fh:
      writer = csv.writer(fh, delimiter="\t")
      if write_header:
        writer.writerow([
            "method", "backend", "mode", "cache_mode", "length", "k", "pair_count",
            "cycles", "instructions", "ipc", "branches", "branch_misses",
            "branch_miss_rate", "cache_references", "cache_misses",
            "cache_miss_rate", "l1d_loads", "l1d_load_misses", "l1d_miss_rate",
            "llc_loads", "llc_load_misses", "llc_miss_rate", "dtlb_loads",
            "dtlb_load_misses", "dtlb_miss_rate"
        ])
      for method in args.methods:
        for k in args.ks:
          with tempfile.NamedTemporaryFile() as perf_out:
            cmd = [
                "perf", "stat", "-x,", "-o", perf_out.name, "-e", ",".join(EVENTS),
                "--", "taskset", "-c", str(args.core), args.binary,
                "--dataset", args.dataset,
                "--lengths", str(args.length),
                "--ks", str(k),
                "--methods", method,
                "--cache-mode", args.cache_mode,
                "--seed", str(args.seed),
                "--out-dir", args.out_dir,
            ]
            subprocess.run(cmd, check=True)
            vals = parse_perf(perf_out.name)
          backend = {"leap_avx2": "avx2", "leap_avx512": "avx512", "lv89": "lv89",
                     "miniwfa": "miniwfa", "wfa2_fresh": "wfa2",
                     "wfa2_reuse": "wfa2"}.get(method, method)
          mode = {"wfa2_fresh": "fresh", "wfa2_reuse": "reuse"}.get(method, "default")
          instructions = vals.get("instructions", 0)
          cycles = vals.get("cycles", 0)
          branches = vals.get("branches", 0)
          branch_misses = vals.get("branch-misses", 0)
          cache_refs = vals.get("cache-references", 0)
          cache_misses = vals.get("cache-misses", 0)
          l1d = vals.get("L1-dcache-loads", 0)
          l1d_miss = vals.get("L1-dcache-load-misses", 0)
          llc = vals.get("LLC-loads", 0)
          llc_miss = vals.get("LLC-load-misses", 0)
          dtlb = vals.get("dTLB-loads", 0)
          dtlb_miss = vals.get("dTLB-load-misses", 0)
          writer.writerow([
              method, backend, mode, args.cache_mode, args.length, k, "",
              cycles, instructions, rate(instructions, cycles), branches, branch_misses,
              rate(branch_misses, branches), cache_refs, cache_misses,
              rate(cache_misses, cache_refs), l1d, l1d_miss, rate(l1d_miss, l1d),
              llc, llc_miss, rate(llc_miss, llc), dtlb, dtlb_miss,
              rate(dtlb_miss, dtlb)
          ])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

