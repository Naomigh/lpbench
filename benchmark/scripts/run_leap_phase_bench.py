#!/usr/bin/env python3
"""Run the LEAP exact phase timing benchmark."""

from __future__ import annotations

import argparse
import csv
import shutil
import subprocess
from pathlib import Path


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dataset", default="benchmark/data/generated/SIM-SR100-EQLEN-BAL")
    p.add_argument("--repeat", type=int, default=3)
    p.add_argument("--warmup", type=int, default=1)
    p.add_argument("--bench", default="benchmark/build/bench_align")
    p.add_argument("--raw-output", default="benchmark/results/raw/leap_exact.tsv")
    p.add_argument("--phase-output", default="benchmark/results/leap_phase_timing.tsv")
    p.add_argument("--markdown-output", default="benchmark/results/leap_phase_timing.md")
    p.add_argument("--build", action="store_true")
    p.add_argument("--no-taskset", action="store_true")
    return p.parse_args()


def fmt_float(value: str) -> str:
    return f"{float(value):.2f}"


def write_markdown(tsv_path: Path, md_path: Path) -> None:
    with tsv_path.open(newline="") as fh:
        rows = list(csv.DictReader(fh, delimiter="\t"))
    columns = [
        "method", "dataset", "pairs", "preprocess ns/pair", "forward ns/pair",
        "traceback ns/pair", "total ns/pair", "preprocess %", "forward %",
        "traceback %", "score mismatches", "cigar failures",
    ]
    md_path.parent.mkdir(parents=True, exist_ok=True)
    with md_path.open("w") as out:
        out.write("| " + " | ".join(columns) + " |\n")
        out.write("| " + " | ".join(["---"] * len(columns)) + " |\n")
        for row in rows:
            values = [
                row["method"],
                row["dataset"],
                row["num_pairs"],
                fmt_float(row["avg_preprocess_ns_per_pair"]),
                fmt_float(row["avg_forward_ns_per_pair"]),
                fmt_float(row["avg_traceback_ns_per_pair"]),
                fmt_float(row["avg_total_ns_per_pair"]),
                fmt_float(row["preprocess_percent"]),
                fmt_float(row["forward_percent"]),
                fmt_float(row["traceback_percent"]),
                row["score_mismatches"],
                row["cigar_replay_failures"],
            ]
            out.write("| " + " | ".join(values) + " |\n")


def main() -> None:
    args = parse_args()
    dataset = Path(args.dataset)
    bench = Path(args.bench)
    raw_output = Path(args.raw_output)
    phase_output = Path(args.phase_output)
    markdown_output = Path(args.markdown_output)

    if args.build:
        subprocess.run(["cmake", "--build", "benchmark/build", "--target", "bench_align"], check=True)
    if not bench.exists():
        raise SystemExit(f"benchmark executable not found: {bench}")

    raw_output.parent.mkdir(parents=True, exist_ok=True)
    phase_output.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(bench),
        "--method", "leap",
        "--mode", "levenshtein_exact",
        "--input", str(dataset / "pairs.tsv"),
        "--metadata", str(dataset / "metadata.tsv"),
        "--repeat", str(args.repeat),
        "--warmup", str(args.warmup),
        "--output", str(raw_output),
        "--phase-output", str(phase_output),
    ]
    if shutil.which("taskset") is not None and not args.no_taskset:
        cmd = ["taskset", "-c", "0"] + cmd
    subprocess.run(cmd, check=True)
    write_markdown(phase_output, markdown_output)
    print(f"wrote {phase_output}")
    print(f"wrote {markdown_output}")


if __name__ == "__main__":
    main()
