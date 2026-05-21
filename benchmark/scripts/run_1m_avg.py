#!/usr/bin/env python3
import argparse
import csv
import subprocess
import sys
import time
from pathlib import Path

ALL_METHODS = ["leap_avx2", "leap_avx512", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"]
PRESETS = {
    "all": ALL_METHODS,
    "no-avx512": ["leap_avx2", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"],
    "leap": ["leap_avx2", "leap_avx512"],
    "non-leap": ["lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"],
    "wfa": ["miniwfa", "wfa2_fresh", "wfa2_reuse"],
    "wfa2": ["wfa2_fresh", "wfa2_reuse"],
    "baseline": ["lv89", "miniwfa", "wfa2_reuse"],
}


def validate_methods(methods):
    bad = [m for m in methods if m not in ALL_METHODS]
    if bad:
        raise SystemExit(f"Unknown method(s): {' '.join(bad)}\nValid methods: {' '.join(ALL_METHODS)}")


def ordered_unique(items):
    seen = set()
    out = []
    for item in items:
        if item not in seen:
            out.append(item)
            seen.add(item)
    return out


def choose_methods(args):
    methods = args.methods if args.methods else PRESETS[args.preset]
    validate_methods(methods)
    if args.exclude:
        validate_methods(args.exclude)
        excluded = set(args.exclude)
        methods = [m for m in methods if m not in excluded]
    methods = ordered_unique(methods)
    if not methods:
        raise SystemExit("No methods selected after applying --exclude")
    return methods


def run(cmd, cwd, dry_run=False):
    print("+ " + " ".join(map(str, cmd)), flush=True)
    if not dry_run:
        subprocess.run(list(map(str, cmd)), cwd=cwd, check=True)


def dataset_has_expected_rows(path, expected_rows):
    if not path.exists():
        return False
    with path.open() as fh:
        return max(0, sum(1 for _ in fh) - 1) == expected_rows


def write_tsv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description="Run one large fixed-k benchmark and report avg CPU ns per pair.")
    parser.add_argument("--target-pairs", type=int, default=1_000_000, help="total selected pairs across all true_ed groups")
    parser.add_argument("--k", type=int, default=5)
    parser.add_argument("--preset", choices=sorted(PRESETS), default="all")
    parser.add_argument("--methods", nargs="+")
    parser.add_argument("--exclude", nargs="+", default=[])
    parser.add_argument("--length", "--read-reference-length", "--read-length", dest="length", type=int, default=100, help="equal read/reference length in bp")
    parser.add_argument("--reference-length", type=int, default=None, help="optional explicit reference length; must equal --length/read length in this benchmark")
    parser.add_argument("--true-ed-min", "--min-ed", "--min-edit-distance", dest="true_ed_min", type=int, default=0)
    parser.add_argument("--true-ed-max", "--max-ed", "--max-edit-distance", dest="true_ed_max", type=int, default=15)
    parser.add_argument("--cache-mode", choices=["warm", "cold-ish"], default="warm")
    parser.add_argument("--core", type=int, default=2)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out-dir", default=None)
    parser.add_argument("--benchmark-out-dir", default=None)
    parser.add_argument("--build-dir", default="benchmark/build")
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--force-regenerate", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    methods = choose_methods(args)
    if args.reference_length is not None and args.reference_length != args.length:
        raise SystemExit("This benchmark requires equal final read/reference lengths; --reference-length must equal --length/--read-length")
    group_count = args.true_ed_max - args.true_ed_min + 1
    if args.target_pairs % group_count != 0:
        raise SystemExit(
            f"--target-pairs must be divisible by true_ed group count {group_count}; "
            f"got {args.target_pairs}"
        )
    pairs_per_ed = args.target_pairs // group_count
    expected_rows = pairs_per_ed * group_count
    default_large_dir = root / f"benchmark/results/large/{args.target_pairs}_pairs_len{args.length}_k{args.k}_ed{args.true_ed_min}_{args.true_ed_max}"
    out_dir = Path(args.out_dir) if args.out_dir else default_large_dir
    if not out_dir.is_absolute():
        out_dir = root / out_dir
    bench_out_dir = Path(args.benchmark_out_dir) if args.benchmark_out_dir else out_dir / "benchmark"
    if not bench_out_dir.is_absolute():
        bench_out_dir = root / bench_out_dir
    dataset = root / f"benchmark/data/generated/pairs_len{args.length}_ed{args.true_ed_min}_{args.true_ed_max}_{expected_rows}pairs_seed{args.seed}.tsv"

    if not args.no_build:
        run(["python3", "benchmark/scripts/build_benchmark.py", "--build-dir", args.build_dir], cwd=root, dry_run=args.dry_run)

    bench_out_dir.mkdir(parents=True, exist_ok=True)
    run(["sh", "benchmark/scripts/record_environment.sh", bench_out_dir / "summary" / "environment.txt"], cwd=root, dry_run=args.dry_run)

    if args.force_regenerate or not dataset_has_expected_rows(dataset, expected_rows):
        run([
            "python3", "benchmark/scripts/generate_dataset.py",
            "--length", args.length,
            "--true-ed-min", args.true_ed_min,
            "--true-ed-max", args.true_ed_max,
            "--pairs-per-ed", pairs_per_ed,
            "--seed", args.seed,
            "--out", dataset,
            "--build-dir", args.build_dir,
        ], cwd=root, dry_run=args.dry_run)

    cmd = [
        Path(args.build_dir) / "threshold_bench",
        "--dataset", dataset,
        "--lengths", args.length,
        "--ks", args.k,
        "--true-ed-min", args.true_ed_min,
        "--true-ed-max", args.true_ed_max,
        "--methods", *methods,
        "--cache-mode", args.cache_mode,
        "--seed", args.seed,
        "--out-dir", bench_out_dir,
    ]
    if args.core is not None:
        cmd = ["taskset", "-c", args.core, *cmd]

    wall_start = time.perf_counter()
    run(cmd, cwd=root, dry_run=args.dry_run)
    wall_seconds = 0.0 if args.dry_run else time.perf_counter() - wall_start
    if args.dry_run:
        return 0

    threshold_raw = bench_out_dir / "raw" / "threshold_raw.tsv"
    raw_rows = list(csv.DictReader(threshold_raw.open(), delimiter="\t"))
    rows = []
    for row in raw_rows:
        if row["method"] not in methods or int(row["k"]) != args.k:
            continue
        pair_count = int(row["pair_count"])
        total_cpu_ns = int(float(row["total_cpu_ns"]))
        rows.append({
            "method": row["method"],
            "backend": row["backend"],
            "mode": row["mode"],
            "k": args.k,
            "length": args.length,
            "target_pairs": args.target_pairs,
            "pair_count": pair_count,
            "pairs_per_ed": pairs_per_ed,
            "true_ed_groups": group_count,
            "cache_mode": args.cache_mode,
            "total_cpu_ns": total_cpu_ns,
            "total_cpu_seconds": total_cpu_ns / 1e9,
            "avg_cpu_ns_per_pair": float(row["avg_cpu_ns_per_pair"]),
            "pairs_per_second_cpu": 1e9 / float(row["avg_cpu_ns_per_pair"]),
            "seconds_per_10M_pairs": float(row["seconds_per_10M_pairs"]),
            "pass_count": int(row["pass_count"]),
            "fail_count": int(row["fail_count"]),
            "mismatch_count": int(row["mismatch_count"]),
            "wall_seconds_threshold_bench": wall_seconds,
            "dataset": str(dataset),
        })
    seen = {r["method"] for r in rows}
    missing = [m for m in methods if m not in seen]
    if missing:
        raise RuntimeError(f"missing method rows: {' '.join(missing)}")
    rows.sort(key=lambda r: float(r["avg_cpu_ns_per_pair"]))

    out_tsv = out_dir / f"avg_{args.target_pairs}_pairs_len{args.length}_k{args.k}_ed{args.true_ed_min}_{args.true_ed_max}.tsv"
    write_tsv(out_tsv, rows)
    print(out_tsv)
    print("\nAverage CPU ns per pair:")
    for r in rows:
        print(f"{r['method']:12s} {r['avg_cpu_ns_per_pair']:10.3f} ns/pair  mismatches={r['mismatch_count']}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"command failed with exit code {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode)
