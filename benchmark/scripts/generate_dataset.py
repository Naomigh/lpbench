#!/usr/bin/env python3
import argparse
import pathlib
import subprocess


def default_dataset_path(length: int, ed_min: int, ed_max: int) -> str:
    return f"benchmark/data/generated/pairs_len{length}_ed{ed_min}_{ed_max}.tsv"


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate verified synthetic DNA benchmark dataset.")
    parser.add_argument("--binary", default="benchmark/build/dataset_generate")
    parser.add_argument("--length", type=int, default=100)
    parser.add_argument("--true-ed-min", type=int, default=0)
    parser.add_argument("--true-ed-max", type=int, default=15)
    parser.add_argument("--pairs-per-ed", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out")
    args = parser.parse_args()

    out = args.out or default_dataset_path(args.length, args.true_ed_min, args.true_ed_max)
    pathlib.Path(out).parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            args.binary,
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
            out,
        ],
        check=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
