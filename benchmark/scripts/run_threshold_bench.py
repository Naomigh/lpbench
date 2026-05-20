#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys


DEFAULT_METHODS = ["leap_avx2", "leap_avx512", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"]


def dataset_path(length: int, ed_min: int, ed_max: int) -> pathlib.Path:
    return pathlib.Path(f"benchmark/data/generated/pairs_len{length}_ed{ed_min}_{ed_max}.tsv")


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True)


def maybe_taskset(core: int | None, cmd: list[str]) -> list[str]:
    if core is None:
        return cmd
    return ["taskset", "-c", str(core), *cmd]


def write_summary(out_dir: pathlib.Path, run_perf: bool) -> None:
    summary_dir = out_dir / "summary"
    summary_dir.mkdir(parents=True, exist_ok=True)
    perf_line = "- `raw/perf_summary.tsv`" if run_perf else "- `raw/perf_summary.tsv` was not generated because `--run-perf` was not used"
    (summary_dir / "summary.md").write_text(
        "\n".join(
            [
                "# Threshold Benchmark Summary",
                "",
                "Measured: CPU time for in-memory threshold verification, `pass = edit_distance(read, reference) <= k`.",
                "",
                "Not measured: dataset generation, trusted ground truth computation, file I/O, output writing, Python orchestration, or wall time.",
                "",
                "Main outputs:",
                "- `raw/threshold_raw.tsv`",
                "- `raw/threshold_by_ed.tsv`",
                "- `raw/leap_phase_raw.tsv`",
                perf_line,
                "",
            ]
        ),
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Build, generate, and run threshold verification benchmark.")
    parser.add_argument("--lengths", nargs="+", type=int, default=[100])
    parser.add_argument("--ks", nargs="+", type=int, default=[1, 2, 3, 4, 5])
    parser.add_argument("--true-ed-min", type=int, default=0)
    parser.add_argument("--true-ed-max", type=int, default=15)
    parser.add_argument("--pairs-per-ed", type=int, default=10000)
    parser.add_argument("--methods", nargs="+", default=DEFAULT_METHODS)
    parser.add_argument("--cache-mode", choices=["warm", "cold-ish"], default="warm")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--core", type=int)
    parser.add_argument("--build", action="store_true")
    parser.add_argument("--run-perf", action="store_true")
    parser.add_argument("--out-dir", default="benchmark/results")
    parser.add_argument("--dataset", help="Use an existing dataset path. Intended for single-length validation runs.")
    args = parser.parse_args()

    out_dir = pathlib.Path(args.out_dir)
    (out_dir / "raw").mkdir(parents=True, exist_ok=True)
    (out_dir / "summary").mkdir(parents=True, exist_ok=True)

    if args.build:
        run([sys.executable, "benchmark/scripts/build_benchmark.py"])

    for length in args.lengths:
        data = pathlib.Path(args.dataset) if args.dataset else dataset_path(length, args.true_ed_min, args.true_ed_max)
        if args.dataset and len(args.lengths) != 1:
            raise SystemExit("--dataset can only be used with one --lengths value")
        if not data.exists():
            if args.dataset:
                raise SystemExit(f"dataset does not exist: {data}")
            run(
                [
                    sys.executable,
                    "benchmark/scripts/generate_dataset.py",
                    "--length",
                    str(length),
                    "--true-ed-min",
                    str(args.true_ed_min),
                    "--true-ed-max",
                    str(args.true_ed_max),
                    "--pairs-per-ed",
                    str(args.pairs_per_ed),
                    "--seed",
                    str(args.seed),
                    "--out",
                    str(data),
                ]
            )

        bench_cmd = [
            "benchmark/build/threshold_bench",
            "--dataset",
            str(data),
            "--out-dir",
            str(out_dir),
            "--methods",
            *args.methods,
            "--ks",
            *[str(k) for k in args.ks],
            "--length",
            str(length),
            "--true-ed-min",
            str(args.true_ed_min),
            "--true-ed-max",
            str(args.true_ed_max),
            "--cache-mode",
            args.cache_mode,
            "--seed",
            str(args.seed),
        ]
        run(maybe_taskset(args.core, bench_cmd))

        leap_backends = [m for m in args.methods if m in {"leap_avx2", "leap_avx512"}]
        if leap_backends:
            phase_cmd = [
                "benchmark/build/leap_phase_bench",
                "--dataset",
                str(data),
                "--out-dir",
                str(out_dir),
                "--backends",
                *leap_backends,
                "--ks",
                *[str(k) for k in args.ks],
                "--length",
                str(length),
                "--true-ed-min",
                str(args.true_ed_min),
                "--true-ed-max",
                str(args.true_ed_max),
                "--seed",
                str(args.seed),
            ]
            run(maybe_taskset(args.core, phase_cmd))

        if args.run_perf:
            for method in args.methods:
                for k in args.ks:
                    perf_cmd = [
                        sys.executable,
                        "benchmark/scripts/run_perf_stat.py",
                        "--binary",
                        "benchmark/build/threshold_bench",
                        "--dataset",
                        str(data),
                        "--out-dir",
                        str(out_dir),
                        "--method",
                        method,
                        "--length",
                        str(length),
                        "--k",
                        str(k),
                        "--true-ed-min",
                        str(args.true_ed_min),
                        "--true-ed-max",
                        str(args.true_ed_max),
                        "--cache-mode",
                        args.cache_mode,
                        "--seed",
                        str(args.seed),
                    ]
                    if args.core is not None:
                        perf_cmd.extend(["--core", str(args.core)])
                    run(perf_cmd)

    write_summary(out_dir, args.run_perf)
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
- benchmark/results/raw/leap_phase_raw.tsv
- benchmark/results/raw/perf_summary.tsv, if --run-perf was used"""
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
