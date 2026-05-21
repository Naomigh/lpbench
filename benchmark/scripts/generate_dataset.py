#!/usr/bin/env python3
import argparse
import pathlib
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--length", type=int, default=100)
    parser.add_argument("--true-ed-min", "--min-ed", "--min-edit-distance", dest="true_ed_min", type=int, default=0)
    parser.add_argument("--true-ed-max", "--max-ed", "--max-edit-distance", dest="true_ed_max", type=int, default=15)
    parser.add_argument("--pairs-per-ed", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out", default="benchmark/data/generated/pairs_len100_ed0_15.tsv")
    parser.add_argument("--build-dir", default="benchmark/build")
    args = parser.parse_args()

    binary = pathlib.Path(args.build_dir) / "dataset_generate"
    if not binary.exists():
        subprocess.run(["python3", "benchmark/scripts/build_benchmark.py"], check=True)

    pathlib.Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            str(binary),
            "--length",
            str(args.length),
            "--true-ed-min",
            str(args.true_ed_min),
            "--true-ed-max",
            str(args.true_ed_max),
            "--pairs-per-ed",
            str(args.pairs_per_ed),
            "--seed",
            str(args.seed),
            "--out",
            args.out,
        ],
        check=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

