#!/usr/bin/env python3
import argparse
import csv
import pathlib


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", default="benchmark/results")
    args = parser.parse_args()
    out_dir = pathlib.Path(args.out_dir)
    raw = out_dir / "raw" / "threshold_raw.tsv"
    summary = out_dir / "summary" / "report.md"
    summary.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    if raw.exists():
        with raw.open() as fh:
            rows = list(csv.DictReader(fh, delimiter="\t"))
    with summary.open("w") as fh:
        fh.write("# Threshold Verification Benchmark Summary\n\n")
        fh.write("Main metric: core CPU time including memory/cache access effects.\n\n")
        fh.write("| method | k | pairs | mismatches | seconds per 10M pairs |\n")
        fh.write("|---|---:|---:|---:|---:|\n")
        for row in rows:
            fh.write(
                f"| {row['method']} | {row['k']} | {row['pair_count']} | "
                f"{row['mismatch_count']} | {float(row['seconds_per_10M_pairs']):.6f} |\n"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

