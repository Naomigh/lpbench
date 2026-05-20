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


HEADER = [
    "method",
    "backend",
    "mode",
    "cache_mode",
    "length",
    "k",
    "pair_count",
    "cycles",
    "instructions",
    "ipc",
    "branches",
    "branch_misses",
    "branch_miss_rate",
    "cache_references",
    "cache_misses",
    "cache_miss_rate",
    "l1d_loads",
    "l1d_load_misses",
    "l1d_miss_rate",
    "llc_loads",
    "llc_load_misses",
    "llc_miss_rate",
    "dtlb_loads",
    "dtlb_load_misses",
    "dtlb_miss_rate",
]


BACKEND_MODE = {
    "leap_avx2": ("avx2", "default"),
    "leap_avx512": ("avx512", "default"),
    "lv89": ("lv89", "default"),
    "miniwfa": ("miniwfa", "default"),
    "wfa2_fresh": ("wfa2", "fresh"),
    "wfa2_reuse": ("wfa2", "reuse"),
}


def parse_perf(path: pathlib.Path) -> dict[str, int]:
    counters: dict[str, int] = {}
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        for row in reader:
            if len(row) < 3:
                continue
            value = row[0].strip().replace(",", "")
            event = row[2].strip()
            if value == "<not supported>" or value == "<not counted>":
                counters[event] = 0
                continue
            try:
                counters[event] = int(float(value))
            except ValueError:
                counters[event] = 0
    return counters


def rate(num: int, den: int) -> str:
    return "0" if den == 0 else f"{num / den:.6f}"


def main() -> int:
    parser = argparse.ArgumentParser(description="Run perf stat for one threshold benchmark method/k.")
    parser.add_argument("--binary", default="benchmark/build/threshold_bench")
    parser.add_argument("--dataset", required=True)
    parser.add_argument("--out-dir", default="benchmark/results")
    parser.add_argument("--method", required=True)
    parser.add_argument("--length", type=int, required=True)
    parser.add_argument("--k", type=int, required=True)
    parser.add_argument("--true-ed-min", type=int, default=0)
    parser.add_argument("--true-ed-max", type=int, default=15)
    parser.add_argument("--cache-mode", choices=["warm", "cold-ish"], default="warm")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--core", type=int)
    args = parser.parse_args()

    out_dir = pathlib.Path(args.out_dir)
    raw_dir = out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)
    perf_summary = raw_dir / "perf_summary.tsv"

    with tempfile.TemporaryDirectory() as tmp:
        perf_out = pathlib.Path(tmp) / "perf.csv"
        bench_out = pathlib.Path(tmp) / "bench_out"
        bench_cmd = [
            args.binary,
            "--dataset",
            args.dataset,
            "--out-dir",
            str(bench_out),
            "--method",
            args.method,
            "--k",
            str(args.k),
            "--length",
            str(args.length),
            "--true-ed-min",
            str(args.true_ed_min),
            "--true-ed-max",
            str(args.true_ed_max),
            "--cache-mode",
            args.cache_mode,
            "--seed",
            str(args.seed),
        ]
        task_cmd = bench_cmd if args.core is None else ["taskset", "-c", str(args.core), *bench_cmd]
        cmd = ["perf", "stat", "-x,", "-o", str(perf_out), "-e", ",".join(EVENTS), "--", *task_cmd]
        subprocess.run(cmd, check=True)

        counters = parse_perf(perf_out)
        row0 = next(csv.DictReader((bench_out / "raw" / "threshold_raw.tsv").open(encoding="utf-8"), delimiter="\t"))
        backend, mode = BACKEND_MODE.get(args.method, ("unknown", "default"))
        cycles = counters.get("cycles", 0)
        instructions = counters.get("instructions", 0)
        branches = counters.get("branches", 0)
        branch_misses = counters.get("branch-misses", 0)
        cache_refs = counters.get("cache-references", 0)
        cache_misses = counters.get("cache-misses", 0)
        l1_loads = counters.get("L1-dcache-loads", 0)
        l1_misses = counters.get("L1-dcache-load-misses", 0)
        llc_loads = counters.get("LLC-loads", 0)
        llc_misses = counters.get("LLC-load-misses", 0)
        dtlb_loads = counters.get("dTLB-loads", 0)
        dtlb_misses = counters.get("dTLB-load-misses", 0)
        row = [
            args.method,
            backend,
            mode,
            args.cache_mode,
            str(args.length),
            str(args.k),
            row0["pair_count"],
            str(cycles),
            str(instructions),
            "0" if cycles == 0 else f"{instructions / cycles:.6f}",
            str(branches),
            str(branch_misses),
            rate(branch_misses, branches),
            str(cache_refs),
            str(cache_misses),
            rate(cache_misses, cache_refs),
            str(l1_loads),
            str(l1_misses),
            rate(l1_misses, l1_loads),
            str(llc_loads),
            str(llc_misses),
            rate(llc_misses, llc_loads),
            str(dtlb_loads),
            str(dtlb_misses),
            rate(dtlb_misses, dtlb_loads),
        ]

    write_header = not perf_summary.exists()
    with perf_summary.open("a", encoding="utf-8", newline="") as f:
        writer = csv.writer(f, delimiter="\t")
        if write_header:
            writer.writerow(HEADER)
        writer.writerow(row)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
