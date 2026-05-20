#!/usr/bin/env python3
import argparse
import csv
import pathlib


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize threshold benchmark TSV outputs.")
    parser.add_argument("--out-dir", default="benchmark/results")
    args = parser.parse_args()

    out_dir = pathlib.Path(args.out_dir)
    raw = out_dir / "raw" / "threshold_raw.tsv"
    summary = out_dir / "summary" / "threshold_summary.md"
    summary.parent.mkdir(parents=True, exist_ok=True)

    rows = []
    if raw.exists():
        with raw.open(encoding="utf-8") as f:
            rows = list(csv.DictReader(f, delimiter="\t"))

    lines = ["# Threshold Results", ""]
    if not rows:
        lines.append("No threshold rows found.")
    else:
        lines.append("| method | k | cache | pairs | avg ns/pair | mismatches |")
        lines.append("|---|---:|---|---:|---:|---:|")
        for row in rows:
            lines.append(
                f"| {row['method']} | {row['k']} | {row['cache_mode']} | {row['pair_count']} | "
                f"{row['avg_cpu_ns_per_pair']} | {row['mismatch_count']} |"
            )
    summary.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
