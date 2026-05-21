#!/usr/bin/env python3
import argparse
import subprocess
import sys

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


def ordered_unique(items):
    seen = set()
    out = []
    for item in items:
        if item not in seen:
            out.append(item)
            seen.add(item)
    return out


def validate_methods(methods):
    bad = [m for m in methods if m not in ALL_METHODS]
    if bad:
        raise SystemExit(f"Unknown method(s): {' '.join(bad)}\nValid methods: {' '.join(ALL_METHODS)}")


def choose_methods(args):
    if args.methods:
        methods = args.methods
    else:
        methods = PRESETS[args.preset]
    validate_methods(methods)
    if args.exclude:
        validate_methods(args.exclude)
        excluded = set(args.exclude)
        methods = [m for m in methods if m not in excluded]
    methods = ordered_unique(methods)
    if not methods:
        raise SystemExit("No methods selected after applying --exclude")
    return methods


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run threshold benchmark with a selectable method set."
    )
    parser.add_argument("--list-methods", action="store_true", help="print valid methods and presets, then exit")
    parser.add_argument("--preset", choices=sorted(PRESETS), default="all", help="method preset used when --methods is not provided")
    parser.add_argument("--methods", nargs="+", help="explicit methods to run")
    parser.add_argument("--exclude", nargs="+", default=[], help="methods to remove from the selected preset/list")
    parser.add_argument("--lengths", nargs="+", type=int, default=[100])
    parser.add_argument("--ks", nargs="+", type=int, default=[1, 2, 3, 4, 5])
    parser.add_argument("--true-ed-min", "--min-ed", "--min-edit-distance", dest="true_ed_min", type=int, default=0)
    parser.add_argument("--true-ed-max", "--max-ed", "--max-edit-distance", dest="true_ed_max", type=int, default=15)
    parser.add_argument("--pairs-per-ed", type=int, default=10000)
    parser.add_argument("--cache-mode", choices=["warm", "cold-ish"], default="warm")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--core", type=int, default=2)
    parser.add_argument("--out-dir", default="benchmark/results")
    parser.add_argument("--build-dir", default="benchmark/build")
    parser.add_argument("--no-build", action="store_true", help="skip CMake build step")
    parser.add_argument("--run-perf", action="store_true")
    parser.add_argument("--run-leap-phase", action="store_true", help="also run separate LEAP phase timing; disabled by default")
    parser.add_argument("--dry-run", action="store_true", help="print command without executing")
    args = parser.parse_args()

    if args.list_methods:
        print("Methods:")
        for method in ALL_METHODS:
            print(f"  {method}")
        print("\nPresets:")
        for name in sorted(PRESETS):
            print(f"  {name}: {' '.join(PRESETS[name])}")
        return 0

    methods = choose_methods(args)
    cmd = [
        "python3", "benchmark/scripts/run_threshold_bench.py",
        "--lengths", *map(str, args.lengths),
        "--ks", *map(str, args.ks),
        "--true-ed-min", str(args.true_ed_min),
        "--true-ed-max", str(args.true_ed_max),
        "--pairs-per-ed", str(args.pairs_per_ed),
        "--cache-mode", args.cache_mode,
        "--core", str(args.core),
        "--seed", str(args.seed),
        "--out-dir", args.out_dir,
        "--build-dir", args.build_dir,
        "--methods", *methods,
    ]
    if not args.no_build:
        cmd.append("--build")
    if args.run_perf:
        cmd.append("--run-perf")
    if args.run_leap_phase:
        cmd.append("--run-leap-phase")

    print("Selected methods: " + " ".join(methods))
    print("+ " + " ".join(cmd), flush=True)
    if args.dry_run:
        return 0
    subprocess.run(cmd, check=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"command failed with exit code {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode)
