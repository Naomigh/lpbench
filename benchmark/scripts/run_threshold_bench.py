#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys


DEFAULT_METHODS = ["leap_avx2", "leap_avx512", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"]


def run(cmd):
    print("+ " + " ".join(map(str, cmd)), flush=True)
    subprocess.run(list(map(str, cmd)), check=True)


def dataset_has_expected_rows(path, expected_rows):
    if not path.exists():
        return False
    with path.open() as fh:
        row_count = max(0, sum(1 for _ in fh) - 1)
    return row_count == expected_rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--lengths", nargs="+", type=int, default=[100])
    parser.add_argument("--ks", nargs="+", type=int, default=[1, 2, 3, 4, 5])
    parser.add_argument("--true-ed-min", "--min-ed", "--min-edit-distance", dest="true_ed_min", type=int, default=0)
    parser.add_argument("--true-ed-max", "--max-ed", "--max-edit-distance", dest="true_ed_max", type=int, default=15)
    parser.add_argument("--pairs-per-ed", type=int, default=10000)
    parser.add_argument("--methods", nargs="+", default=DEFAULT_METHODS)
    parser.add_argument("--cache-mode", default="warm", choices=["warm", "cold-ish"])
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--core", type=int)
    parser.add_argument("--build", action="store_true")
    parser.add_argument("--run-perf", action="store_true")
    parser.add_argument("--run-leap-phase", action="store_true", help="also run separate LEAP phase timing; disabled by default so LEAP main experiments remain forward-only")
    parser.add_argument("--out-dir", default="benchmark/results")
    parser.add_argument("--build-dir", default="benchmark/build")
    args = parser.parse_args()

    if args.build:
        run(["python3", "benchmark/scripts/build_benchmark.py", "--build-dir", args.build_dir])

    out_dir = pathlib.Path(args.out_dir)
    (out_dir / "raw").mkdir(parents=True, exist_ok=True)
    (out_dir / "summary").mkdir(parents=True, exist_ok=True)

    run(["sh", "benchmark/scripts/record_environment.sh", str(out_dir / "summary" / "environment.txt")])

    for length in args.lengths:
        dataset = pathlib.Path(f"benchmark/data/generated/pairs_len{length}_ed{args.true_ed_min}_{args.true_ed_max}.tsv")
        expected_rows = args.pairs_per_ed * (args.true_ed_max - args.true_ed_min + 1)
        if not dataset_has_expected_rows(dataset, expected_rows):
            run(
                [
                    "python3",
                    "benchmark/scripts/generate_dataset.py",
                    "--length",
                    length,
                    "--true-ed-min",
                    args.true_ed_min,
                    "--true-ed-max",
                    args.true_ed_max,
                    "--pairs-per-ed",
                    args.pairs_per_ed,
                    "--seed",
                    args.seed,
                    "--out",
                    dataset,
                    "--build-dir",
                    args.build_dir,
                ]
            )

        bench_cmd = [
            pathlib.Path(args.build_dir) / "threshold_bench",
            "--dataset",
            dataset,
            "--lengths",
            length,
            "--ks",
            *args.ks,
            "--true-ed-min",
            args.true_ed_min,
            "--true-ed-max",
            args.true_ed_max,
            "--methods",
            *args.methods,
            "--cache-mode",
            args.cache_mode,
            "--seed",
            args.seed,
            "--out-dir",
            out_dir,
        ]
        if args.core is not None:
            bench_cmd = ["taskset", "-c", args.core, *bench_cmd]
        run(bench_cmd)

        phase_backends = [m for m in args.methods if m in ("leap_avx2", "leap_avx512")]
        if args.run_leap_phase and phase_backends:
            phase_cmd = [
                pathlib.Path(args.build_dir) / "leap_phase_bench",
                "--dataset",
                dataset,
                "--lengths",
                length,
                "--ks",
                *args.ks,
                "--true-ed-min",
                args.true_ed_min,
                "--true-ed-max",
                args.true_ed_max,
                "--backends",
                *phase_backends,
                "--seed",
                args.seed,
                "--out-dir",
                out_dir,
            ]
            if args.core is not None:
                phase_cmd = ["taskset", "-c", args.core, *phase_cmd]
            run(phase_cmd)

        if args.run_perf:
            run(
                [
                    "python3",
                    "benchmark/scripts/run_perf_stat.py",
                    "--binary",
                    pathlib.Path(args.build_dir) / "threshold_bench",
                    "--dataset",
                    dataset,
                    "--length",
                    length,
                    "--ks",
                    *args.ks,
                    "--methods",
                    *args.methods,
                    "--cache-mode",
                    args.cache_mode,
                    "--core",
                    args.core if args.core is not None else 0,
                    "--out-dir",
                    out_dir,
                    "--seed",
                    args.seed,
                ]
            )

    run(["python3", "benchmark/scripts/summarize_results.py", "--out-dir", out_dir])

    print(
        """Benchmark completed.

Measured:
- CPU time for in-memory threshold verification:
  pass = edit_distance(read, reference) <= k

Not measured:
- dataset generation
- ground truth computation
- file I/O
- output writing
- Python orchestration
- wall time

Main outputs:
- benchmark/results/raw/threshold_raw.tsv
- benchmark/results/raw/threshold_by_ed.tsv
- benchmark/results/raw/leap_phase_raw.tsv, only if --run-leap-phase was used
- benchmark/results/raw/perf_summary.tsv, if --run-perf was used
- benchmark/results/summary/environment.txt

There should also be several tables, just like Table1 in LEAP original paper, recording the total time normalized to 10 million pairs."""
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"command failed with exit code {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode)

