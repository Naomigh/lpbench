#!/usr/bin/env python3
"""Run bounded affine-gap benchmark conditions."""

from __future__ import annotations

import argparse
import csv
import shutil
import subprocess
from pathlib import Path


DEFAULT_METHODS = "leap,miniwfa,wfa2,ksw2"
DEFAULT_THRESHOLDS = "5,10,15,20,30"


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dataset", default="benchmark/data/generated/SIM-SR100-EQLEN-GAP")
    p.add_argument("--thresholds", default=DEFAULT_THRESHOLDS)
    p.add_argument("--methods", default=DEFAULT_METHODS)
    p.add_argument("--mismatch", type=int, default=2)
    p.add_argument("--gap-open", type=int, default=3)
    p.add_argument("--gap-extend", type=int, default=1)
    p.add_argument("--repeat", type=int, default=5)
    p.add_argument("--warmup", type=int, default=1)
    p.add_argument("--bench", default="benchmark/build/bench_align")
    p.add_argument("--raw-dir", default="benchmark/results/raw")
    p.add_argument("--no-taskset", action="store_true")
    return p.parse_args()


def write_failure(path: Path, method: str, dataset_name: str, threshold: int, args: argparse.Namespace, note: str) -> None:
    fields = [
        "method", "mode", "dataset", "threshold", "mismatch", "gap_open", "gap_extend",
        "repeat", "warmup", "num_pairs", "seconds", "pairs_per_second", "ns_per_pair",
        "checksum", "score_mismatches", "pass_mismatches", "cigar_replay_failures", "notes",
    ]
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerow({
            "method": method, "mode": "affine", "dataset": dataset_name, "threshold": threshold,
            "mismatch": args.mismatch, "gap_open": args.gap_open, "gap_extend": args.gap_extend,
            "repeat": 0, "warmup": args.warmup, "num_pairs": 0, "seconds": "NA",
            "pairs_per_second": "NA", "ns_per_pair": "NA", "checksum": "NA",
            "score_mismatches": "NA", "pass_mismatches": "NA", "cigar_replay_failures": "NA",
            "notes": note,
        })


def main() -> None:
    args = parse_args()
    dataset = Path(args.dataset)
    raw_dir = Path(args.raw_dir)
    raw_dir.mkdir(parents=True, exist_ok=True)
    pairs = dataset / "pairs.tsv"
    metadata = dataset / "metadata.tsv"
    methods = [x for x in args.methods.split(",") if x]
    thresholds = [int(x) for x in args.thresholds.split(",") if x]
    use_taskset = shutil.which("taskset") is not None and not args.no_taskset
    for method in methods:
        for threshold in thresholds:
            out = raw_dir / f"{method}_affine_{dataset.name}_T{threshold}.tsv"
            cmd = [
                args.bench, "--method", method, "--mode", "affine", "--threshold", str(threshold),
                "--mismatch", str(args.mismatch), "--gap-open", str(args.gap_open),
                "--gap-extend", str(args.gap_extend), "--input", str(pairs),
                "--metadata", str(metadata), "--repeat", str(args.repeat), "--warmup", str(args.warmup),
                "--output", str(out),
            ]
            if use_taskset:
                cmd = ["taskset", "-c", "0"] + cmd
            try:
                subprocess.run(cmd, check=True)
            except Exception as exc:
                write_failure(out, method, dataset.name, threshold, args, f"run_failed={exc}")
                print(f"FAILED {method} T={threshold}: {exc}")
            else:
                print(f"wrote {out}")


if __name__ == "__main__":
    main()
