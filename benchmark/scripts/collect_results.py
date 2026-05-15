#!/usr/bin/env python3
"""Collect raw TSV benchmark results into a median summary."""

from __future__ import annotations

import argparse
import csv
import statistics
from collections import defaultdict
from pathlib import Path


FIELDS = [
    "method", "mode", "dataset", "threshold", "mismatch", "gap_open", "gap_extend", "num_pairs",
    "repeats", "median_seconds", "median_pairs_per_second", "median_ns_per_pair",
    "seconds_per_10M", "score_mismatches", "pass_mismatches", "cigar_replay_failures",
    "checksums", "notes", "source_files",
]


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--raw-dir", default="benchmark/results/raw")
    p.add_argument("--output", default="benchmark/results/processed/summary.tsv")
    p.add_argument("--data-dir", default="benchmark/data/generated")
    p.add_argument("--results-dir", default="benchmark/results")
    return p.parse_args()


def fnum(value: str):
    try:
        return float(value)
    except Exception:
        return None


def key(row: dict) -> tuple:
    return (
        row["method"], row["mode"], row["dataset"], row["threshold"],
        row["mismatch"], row["gap_open"], row["gap_extend"],
    )


def main() -> None:
    args = parse_args()
    groups = defaultdict(list)
    source_files = defaultdict(list)
    for path in sorted(Path(args.raw_dir).glob("*.tsv")):
      with path.open(newline="") as fh:
        reader = csv.DictReader(fh, delimiter="\t")
        for row in reader:
            k = key(row)
            groups[k].append(row)
            source_files[k].append(path.name)

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    summary_rows = []
    with out_path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=FIELDS, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for k, rows in sorted(groups.items()):
            seconds = [fnum(r["seconds"]) for r in rows if fnum(r["seconds"]) is not None]
            pps = [fnum(r["pairs_per_second"]) for r in rows if fnum(r["pairs_per_second"]) is not None]
            ns = [fnum(r["ns_per_pair"]) for r in rows if fnum(r["ns_per_pair"]) is not None]
            num_pairs = max([int(r["num_pairs"]) for r in rows if r["num_pairs"].isdigit()] or [0])
            med_seconds = statistics.median(seconds) if seconds else "NA"
            med_pps = statistics.median(pps) if pps else "NA"
            med_ns = statistics.median(ns) if ns else "NA"
            sec10m = (float(med_seconds) / num_pairs * 10_000_000) if seconds and num_pairs else "NA"
            def isum(name: str) -> str:
                vals = []
                for r in rows:
                    try:
                        vals.append(int(r[name]))
                    except Exception:
                        pass
                return str(max(vals) if vals else "NA")
            notes = sorted({r.get("notes", "") for r in rows if r.get("notes", "")})
            checksums = sorted({r.get("checksum", "") for r in rows if r.get("checksum", "")})
            out_row = {
                "method": k[0], "mode": k[1], "dataset": k[2], "threshold": k[3],
                "mismatch": k[4], "gap_open": k[5], "gap_extend": k[6], "num_pairs": num_pairs,
                "repeats": len(seconds), "median_seconds": med_seconds,
                "median_pairs_per_second": med_pps, "median_ns_per_pair": med_ns,
                "seconds_per_10M": sec10m, "score_mismatches": isum("score_mismatches"),
                "pass_mismatches": isum("pass_mismatches"),
                "cigar_replay_failures": isum("cigar_replay_failures"),
                "checksums": ";".join(checksums), "notes": ";".join(notes),
                "source_files": ";".join(sorted(set(source_files[k]))),
            }
            summary_rows.append(out_row)
            writer.writerow(out_row)
    write_leap_audit(summary_rows, Path(args.data_dir), Path(args.results_dir))
    print(f"wrote {out_path}")


def count_equal_length_pairs(dataset_dir: Path) -> tuple[int, int, int]:
    pairs_path = dataset_dir / "pairs.tsv"
    if not pairs_path.exists():
        return 0, 0, 0
    total = equal = non_equal = 0
    with pairs_path.open(newline="") as fh:
        reader = csv.DictReader(fh, delimiter="\t")
        for row in reader:
            total += 1
            if len(row["reference"]) == len(row["read"]):
                equal += 1
            else:
                non_equal += 1
    return total, equal, non_equal


def write_leap_audit(summary_rows: list[dict], data_dir: Path, results_dir: Path) -> None:
    processed = results_dir / "processed"
    reports = results_dir / "reports"
    processed.mkdir(parents=True, exist_ok=True)
    reports.mkdir(parents=True, exist_ok=True)
    audit_path = processed / "leap_backend_audit.tsv"
    fields = [
        "method", "mode", "dataset", "threshold", "total_pairs", "equal_length_pairs",
        "non_equal_length_pairs", "avx512_pairs", "fallback_pairs", "fallback_reasons",
    ]
    audit_rows = []
    for row in summary_rows:
        if row["method"] != "leap":
            continue
        total_file, equal_file, non_equal_file = count_equal_length_pairs(data_dir / row["dataset"])
        total_pairs = total_file or int(row["num_pairs"] or 0)
        notes = row.get("notes", "")
        fallback = "fallback_used=true" in notes or "oracle_fallback" in notes
        avx512 = "backend_actual=avx512" in notes and not fallback
        fallback_reasons = []
        for part in notes.split(";"):
            if "reason=" in part or "oracle_fallback" in part or "fallback_used=true" in part:
                fallback_reasons.append(part)
        audit_rows.append({
            "method": "leap",
            "mode": row["mode"],
            "dataset": row["dataset"],
            "threshold": row["threshold"],
            "total_pairs": total_pairs,
            "equal_length_pairs": equal_file if total_file else ("NA" if total_pairs == 0 else total_pairs),
            "non_equal_length_pairs": non_equal_file if total_file else "NA",
            "avx512_pairs": total_pairs if avx512 else 0,
            "fallback_pairs": total_pairs if fallback else 0,
            "fallback_reasons": ";".join(sorted(set(fallback_reasons))),
        })
    with audit_path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(audit_rows)
    md_path = reports / "leap_backend_audit.md"
    with md_path.open("w") as fh:
        fh.write("# LEAP backend audit\n\n")
        fh.write("This audit checks whether LEAP benchmark rows used the AVX512 backend or fell back.\n\n")
        if not audit_rows:
            fh.write("No LEAP raw results were found.\n")
        else:
            fh.write("| mode | dataset | threshold | total pairs | equal length | non-equal length | AVX512 pairs | fallback pairs | reasons |\n")
            fh.write("|---|---|---:|---:|---:|---:|---:|---:|---|\n")
            for row in audit_rows:
                fh.write(
                    f"| {row['mode']} | {row['dataset']} | {row['threshold']} | {row['total_pairs']} | "
                    f"{row['equal_length_pairs']} | {row['non_equal_length_pairs']} | {row['avx512_pairs']} | "
                    f"{row['fallback_pairs']} | {row['fallback_reasons']} |\n"
                )
            failures = [r for r in audit_rows if int(r["fallback_pairs"]) > 0]
            fh.write("\n")
            if failures:
                fh.write("Fallback was observed and must be investigated before using these LEAP rows in final speed tables.\n")
            else:
                fh.write("No fallback was observed in the collected LEAP rows.\n")
    print(f"wrote {audit_path}")


if __name__ == "__main__":
    main()
