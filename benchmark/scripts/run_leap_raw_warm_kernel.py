#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import tempfile


PERF_EVENTS = [
    "cycles",
    "instructions",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "cache-references",
    "cache-misses",
]


def add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--dataset", default="benchmark/data/generated/pairs_len100_ed0_15.tsv")
    parser.add_argument("--backends", nargs="+", default=["leap_avx2", "leap_avx512"])
    parser.add_argument("--ks", nargs="+", type=int, default=[1, 2, 3, 4, 5])
    parser.add_argument("--length", type=int, default=100)
    parser.add_argument("--true-ed-min", type=int, default=0)
    parser.add_argument("--true-ed-max", type=int, default=15)
    parser.add_argument("--repeat", type=int, default=64)
    parser.add_argument("--core", type=int, default=2)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out-dir", default="benchmark/results")


def bench_cmd(args: argparse.Namespace, out_dir: pathlib.Path, backends: list[str]) -> list[str]:
    cmd = [
        "benchmark/build/leap_raw_warm_kernel_bench",
        "--dataset",
        args.dataset,
        "--out-dir",
        str(out_dir),
        "--backends",
        *backends,
        "--ks",
        *[str(k) for k in args.ks],
        "--length",
        str(args.length),
        "--true-ed-min",
        str(args.true_ed_min),
        "--true-ed-max",
        str(args.true_ed_max),
        "--repeat",
        str(args.repeat),
        "--seed",
        str(args.seed),
    ]
    if args.core is not None:
        return ["taskset", "-c", str(args.core), *cmd]
    return cmd


def write_report(args: argparse.Namespace, out_dir: pathlib.Path) -> None:
    summary_dir = out_dir / "summary"
    summary_dir.mkdir(parents=True, exist_ok=True)
    report = summary_dir / "leap_raw_warm_kernel_report.md"
    report.write_text(
        "\n".join(
            [
                "# LEAP raw_warm Kernel Benchmark",
                "",
                "`raw_warm` measures the repeated LEAP kernel sequence after the read/reference bytes are explicitly touched outside timing.",
                "",
                "Timed region:",
                "",
                "```cpp",
                "ed.load_reads(read, ref, len);",
                "ed.calculate_masks();",
                "ed.reset();",
                "ed.run();",
                "```",
                "",
                "The benchmark excludes `SIMD_ED` construction, `init_levenshtein`, explicit input touching, optional warmup, `check_pass`, pass/fail comparison, file I/O, TSV parsing, ground-truth computation, and output writing from the timed CPU interval.",
                "",
                "The reported time is warm-cache-hierarchy timing for input loading, DNA-to-bit-vector conversion, mask generation, reset, and forward traversal. CPU cache residency is hardware-managed, so this benchmark does not prove exact L1 residency or isolate exact memory-transfer time.",
                "",
                "Backends use consecutive chunks when the sequence is longer than one backend chunk: `leap_avx2` uses 256 bp chunks and `leap_avx512` uses 512 bp chunks. A pair passes only when every chunk passes `check_pass`; mismatch counts compare that result against the dataset `pass_kX` column outside timing.",
                "",
                f"Raw TSV: `{out_dir / 'raw' / 'leap_raw_warm_kernel_raw.tsv'}`",
                f"Dataset: `{args.dataset}`",
                f"Repeat: `{args.repeat}`",
                f"Seed: `{args.seed}`",
                "",
            ]
        ),
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Run LEAP raw_warm kernel benchmark.")
    add_common_args(parser)
    parser.add_argument("--build", action="store_true")
    parser.add_argument("--run-perf", action="store_true")
    args = parser.parse_args()

    out_dir = pathlib.Path(args.out_dir)
    raw_dir = out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)

    if args.build:
        subprocess.run(["python3", "benchmark/scripts/build_benchmark.py"], check=True)

    subprocess.run(bench_cmd(args, out_dir, args.backends), check=True)

    if args.run_perf:
        for backend in args.backends:
            with tempfile.TemporaryDirectory(prefix=f"leap_raw_warm_{backend}_") as tmp:
                tmp_out = pathlib.Path(tmp) / "results"
                perf_out = raw_dir / f"leap_raw_warm_kernel_perf_{backend}.csv"
                cmd = [
                    "perf",
                    "stat",
                    "-x,",
                    "-o",
                    str(perf_out),
                    "-e",
                    ",".join(PERF_EVENTS),
                    "--",
                    *bench_cmd(args, tmp_out, [backend]),
                ]
                subprocess.run(cmd, check=True)

    write_report(args, out_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
