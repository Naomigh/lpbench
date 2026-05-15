#!/usr/bin/env python3
"""Run bounded Levenshtein benchmark conditions."""

from __future__ import annotations

import argparse
import csv
import os
import shutil
import subprocess
from pathlib import Path


DEFAULT_METHODS = "leap,edlib,lv89,miniwfa,wfa2,ksw2"
DEFAULT_THRESHOLDS = "1,2,3,5,8"


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dataset", default="benchmark/data/generated/SIM-SR100-EQLEN-BAL")
    p.add_argument("--thresholds", default=DEFAULT_THRESHOLDS)
    p.add_argument("--methods", default=DEFAULT_METHODS)
    p.add_argument("--repeat", type=int, default=5)
    p.add_argument("--warmup", type=int, default=1)
    p.add_argument("--bench", default="benchmark/build/bench_align")
    p.add_argument("--raw-dir", default="benchmark/results/raw")
    p.add_argument("--no-taskset", action="store_true")
    return p.parse_args()


def write_failure(path: Path, method: str, dataset_name: str, threshold: int, repeat: int, warmup: int, note: str) -> None:
    fields = [
        "method", "mode", "dataset", "threshold", "mismatch", "gap_open", "gap_extend",
        "repeat", "warmup", "num_pairs", "seconds", "pairs_per_second", "ns_per_pair",
        "checksum", "score_mismatches", "pass_mismatches", "cigar_replay_failures", "notes",
    ]
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerow({
            "method": method, "mode": "levenshtein", "dataset": dataset_name, "threshold": threshold,
            "mismatch": 2, "gap_open": 3, "gap_extend": 1, "repeat": 0, "warmup": warmup,
            "num_pairs": 0, "seconds": "NA", "pairs_per_second": "NA", "ns_per_pair": "NA",
            "checksum": "NA", "score_mismatches": "NA", "pass_mismatches": "NA",
            "cigar_replay_failures": "NA", "notes": note,
        })


def dataset_name(dataset: Path) -> str:
    return dataset.name


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
            out = raw_dir / f"{method}_levenshtein_{dataset.name}_k{threshold}.tsv"
            cmd = [
                args.bench, "--method", method, "--mode", "levenshtein", "--threshold", str(threshold),
                "--input", str(pairs), "--metadata", str(metadata), "--repeat", str(args.repeat),
                "--warmup", str(args.warmup), "--output", str(out),
            ]
            if use_taskset:
                cmd = ["taskset", "-c", "0"] + cmd
            try:
                subprocess.run(cmd, check=True)
            except Exception as exc:
                write_failure(out, method, dataset.name, threshold, args.repeat, args.warmup, f"run_failed={exc}")
                print(f"FAILED {method} k={threshold}: {exc}")
            else:
                print(f"wrote {out}")


if __name__ == "__main__":
    main()
