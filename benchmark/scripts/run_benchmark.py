#!/usr/bin/env python3
import argparse
import csv
import subprocess
import sys
from pathlib import Path


FIELDS = [
    "method",
    "backend",
    "dataset",
    "max_edit",
    "num_pairs",
    "total_time_ns",
    "avg_time_ns_per_pair",
    "correct_count",
    "mismatch_count",
]


METHODS = [
    ("leap", "avx2", "leap_avx2_bench", True),
    ("leap", "avx512", "leap_avx512_bench", True),
    ("lv89", "default", "lv89_bench", False),
    ("miniwfa", "exact", "miniwfa_bench", False),
    ("wfa2", "edit", "wfa2_bench", False),
]


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def run_one(binary: Path, dataset_path: Path, dataset_name: str, extra_args=None):
    cmd = [str(binary), "--input", str(dataset_path), "--dataset", dataset_name]
    if extra_args:
        cmd.extend(extra_args)
    proc = subprocess.run(
        cmd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"{binary.name} failed on {dataset_name}\nSTDERR:\n{proc.stderr}")
    lines = [line for line in proc.stdout.splitlines() if line.strip()]
    if len(lines) < 2:
        raise RuntimeError(f"{binary.name} produced malformed output:\n{proc.stdout}")
    row = dict(zip(lines[0].split("\t"), lines[1].split("\t")))
    mismatches = []
    err_lines = [line for line in proc.stderr.splitlines() if line.strip()]
    if err_lines:
        if err_lines[0].startswith("pair_id\t"):
            for line in err_lines[1:]:
                parts = line.split("\t")
                if len(parts) == 7:
                    mismatches.append(parts)
        else:
            sys.stderr.write(proc.stderr)
    return row, mismatches


def write_summary(rows, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as fh:
        fh.write("# Benchmark Summary\n\n")
        for max_edit in sorted({int(r["max_edit"]) for r in rows}):
            fh.write(f"## max_edit = {max_edit}\n\n")
            fh.write("| max_edit | method | backend | avg_time_ns_per_pair | correct_count | mismatch_count |\n")
            fh.write("|---:|---|---|---:|---:|---:|\n")
            for row in rows:
                if int(row["max_edit"]) != max_edit:
                    continue
                fh.write(
                    f"| {row['max_edit']} | {row['method']} | {row['backend']} | "
                    f"{float(row['avg_time_ns_per_pair']):.2f} | {row['correct_count']} | "
                    f"{row['mismatch_count']} |\n"
                )
            fh.write("\n")


def main(argv=None):
    root = repo_root()
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default=str(root / "benchmark" / "build"))
    parser.add_argument("--data-dir", default=str(root / "benchmark" / "data"))
    parser.add_argument("--raw-out", default=str(root / "benchmark" / "results" / "raw" / "benchmark_raw.tsv"))
    parser.add_argument("--summary-out", default=str(root / "benchmark" / "results" / "benchmark_summary.md"))
    parser.add_argument("--max-edits", nargs="+", type=int, default=[5, 10, 15])
    parser.add_argument("--leap-max-energy", type=int, default=15)
    parser.add_argument("--leap-chunk-size", type=int, default=0)
    args = parser.parse_args(argv)

    build_dir = Path(args.build_dir)
    data_dir = Path(args.data_dir)
    rows = []
    mismatches = []
    for max_edit in args.max_edits:
        dataset_path = data_dir / f"pairs_ed{max_edit}.tsv"
        dataset_name = f"pairs_ed{max_edit}"
        if not dataset_path.exists():
            raise FileNotFoundError(dataset_path)
        for method, backend, exe, is_leap in METHODS:
            extra_args = (
                ["--max-energy", str(args.leap_max_energy), "--chunk-size", str(args.leap_chunk_size)]
                if is_leap
                else None
            )
            row, bad = run_one(build_dir / exe, dataset_path, dataset_name, extra_args)
            rows.append(row)
            mismatches.extend(bad)

    raw_out = Path(args.raw_out)
    raw_out.parent.mkdir(parents=True, exist_ok=True)
    mismatch_out = raw_out.parent / "mismatch_details.tsv"
    if mismatch_out.exists():
        mismatch_out.unlink()
    with raw_out.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=FIELDS, delimiter="\t")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: row[field] for field in FIELDS})

    if mismatches:
        with mismatch_out.open("w", encoding="utf-8") as fh:
            fh.write("pair_id\tmethod\tbackend\tread\treference\tground_truth\tmethod_result\n")
            for parts in mismatches:
                fh.write("\t".join(parts) + "\n")

    write_summary(rows, Path(args.summary_out))


if __name__ == "__main__":
    main()
