#!/usr/bin/env python3
import csv
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "benchmark" / "build"
RAW_DIR = ROOT / "benchmark" / "results" / "raw"
SUMMARY_DIR = ROOT / "benchmark" / "results" / "summary"
RAW_OUT = RAW_DIR / "threshold_verify_raw.tsv"
SUMMARY_OUT = SUMMARY_DIR / "threshold_verify_summary.tsv"
MISMATCH_OUT = RAW_DIR / "threshold_verify_mismatches.tsv"
REPORT_OUT = ROOT / "benchmark" / "results" / "report.md"

METHOD_BINS = [
    ("leap_avx2", "threshold_verify_leap_avx2"),
    ("leap_avx512", "threshold_verify_leap_avx512"),
    ("miniwfa", "threshold_verify_miniwfa"),
    ("lv89", "threshold_verify_lv89"),
    ("wfa2-lib", "threshold_verify_wfa2"),
]

METHOD_NOTES = {
    "leap_avx2": (
        "Native threshold cutoff via max_energy = k.",
        "Uses LEAP AVX2 MAX_LENGTH/effective length 256; lengths above 256 are split into ceil(length / 256) chunks and all chunks must pass.",
        "load_reads, calculate_masks, reset, run, check_pass for each chunk.",
    ),
    "leap_avx512": (
        "Native threshold cutoff via max_energy = k.",
        "Uses LEAP AVX512 MAX_LENGTH/effective length 512; lengths above 512 are split into ceil(length / 512) chunks and all chunks must pass.",
        "load_reads, calculate_masks, reset, run, check_pass for each chunk.",
    ),
    "miniwfa": (
        "Native threshold cutoff via opt.max_s = k; returns failure when score exceeds the cutoff.",
        "No external chunking.",
        "mwf_wfa_exact score-only call with per-pair options and result extraction.",
    ),
    "lv89": (
        "Uses lv89 native band width bw = k, then thresholds the computed score; no separate max-score cutoff is exposed.",
        "No external chunking.",
        "lv_ed score-only call with bw = k and final score extraction.",
    ),
    "wfa2-lib": (
        "Computes score-only edit distance and thresholds the score; no max-score cutoff is used.",
        "No external chunking.",
        "wavefront_aligner_new, wavefront_align with compute_score/edit, score extraction, delete.",
    ),
}


def parse_args():
    pairs = 10000
    prefetch = 16
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == "--pairs-per-group" and i + 1 < len(sys.argv):
            pairs = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == "--prefetch-distance" and i + 1 < len(sys.argv):
            prefetch = int(sys.argv[i + 1])
            i += 2
        else:
            raise SystemExit(f"unknown or incomplete argument: {sys.argv[i]}")
    return pairs, prefetch


def read_rows(path):
    with path.open(newline="") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def write_tsv(path, rows, fields):
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, delimiter="\t", fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def run_methods(pairs, prefetch):
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    SUMMARY_DIR.mkdir(parents=True, exist_ok=True)
    if RAW_OUT.exists():
        RAW_OUT.unlink()
    if MISMATCH_OUT.exists():
        MISMATCH_OUT.unlink()

    first = True
    skipped = []
    for method, binary in METHOD_BINS:
        exe = BUILD / binary
        if not exe.exists():
            raise SystemExit(f"missing {exe}; run: make threshold_verify_bench")
        cmd = [
            str(exe),
            "--pairs-per-group",
            str(pairs),
            "--prefetch-distance",
            str(prefetch),
            "--raw-out",
            str(RAW_OUT),
            "--mismatch-out",
            str(MISMATCH_OUT),
            "--append",
            "0" if first else "1",
        ]
        result = subprocess.run(cmd, cwd=ROOT)
        if result.returncode == 77:
            skipped.append(method)
            continue
        result.check_returncode()
        first = False
    return skipped


def build_summary():
    rows = read_rows(RAW_OUT)
    fields = [
        "method",
        "length",
        "k",
        "pair_count",
        "pass_count",
        "fail_count",
        "total_cpu_ns",
        "avg_cpu_ns_per_pair",
        "seconds_per_10M_pairs",
        "mismatch_count",
        "prefetch_distance",
    ]
    write_tsv(SUMMARY_OUT, rows, fields)
    return rows


def md_table(headers, rows):
    out = []
    out.append("| " + " | ".join(headers) + " |")
    out.append("| " + " | ".join(["---"] * len(headers)) + " |")
    for row in rows:
        out.append("| " + " | ".join(str(x) for x in row) + " |")
    return "\n".join(out)


def write_report(rows, skipped, prefetch):
    summary_rows = []
    for r in rows:
        summary_rows.append([
            r["method"],
            r["length"],
            r["k"],
            r["pair_count"],
            r["pass_count"],
            r["fail_count"],
            f"{float(r['avg_cpu_ns_per_pair']):.3f}",
            f"{float(r['seconds_per_10M_pairs']):.6f}",
        ])

    note_rows = []
    for method, notes in METHOD_NOTES.items():
        if method in skipped:
            note_rows.append([method, "Skipped: CPU support unavailable.", notes[1], notes[2]])
        else:
            note_rows.append([method, notes[0], notes[1], notes[2]])

    mismatch_rows = [
        [r["method"], r["length"], r["k"], r["mismatch_count"]]
        for r in rows
    ]

    lines = [
        "# Threshold Verification Benchmark",
        "",
        "## Timing Methodology",
        "",
        "This benchmark measures CPU time, not wall time. The measured timer is `CLOCK_THREAD_CPUTIME_ID` via `clock_gettime`, exposed in the benchmark harness as `thread_cpu_time_ns()`.",
        "",
        "The timed task is threshold verification: determine whether `edit_distance(read, reference) <= k`. It is not an end-to-end mapper benchmark. The benchmark does not compute CIGAR strings, traceback, or backtracking.",
        "",
        "File I/O, dataset generation, ground-truth computation, result writing, report generation, correctness summary generation, and stdout printing are excluded from the measured region. All synthetic read/reference pairs are generated and preloaded in memory before timing. Ground truth is computed by a trusted dynamic-programming edit-distance implementation outside timing and stored with each generated pair.",
        "",
        "Before measured timing, every byte of every read/reference string is touched using a volatile sink. Each method also receives one full untimed dry run over every pair in each dataset group. The measured region then runs the algorithm work for each pair and records pass/fail results for later correctness checking.",
        "",
        f"Optional software prefetching is enabled with `--prefetch-distance`; this run used distance `{prefetch}`. If the distance is greater than zero, the measured loop prefetches pair `i + N` when that pair is in bounds.",
        "",
        "Results are normalized to seconds per 10 million pairs, matching the LEAP paper style, using `seconds_per_10M_pairs = avg_cpu_ns_per_pair / 100.0`.",
        "",
        "LEAP runs as a threshold verifier with `max_energy = k`. LEAP AVX2 and AVX512 use the backend MAX_LENGTH/effective length. If the target length is at most MAX_LENGTH, a pair is processed as one chunk. If the target length exceeds MAX_LENGTH, read and reference are split into corresponding chunks, each chunk is processed with `max_energy = k`, and the wrapper reports pass only if all chunks pass. Chunk overhead is included in the measured wrapper time.",
        "",
        "Methods that do not expose a max-distance cutoff may compute the full score first and then apply the threshold.",
        "",
    ]
    if skipped:
        lines += ["Skipped methods: " + ", ".join(skipped), ""]

    lines += [
        "## Summary",
        "",
        md_table(
            ["method", "length", "k", "pairs", "pass_count", "fail_count", "avg_cpu_ns_per_pair", "seconds_per_10M_pairs"],
            summary_rows,
        ),
        "",
        "## Method Notes",
        "",
        md_table(["method", "cutoff behavior", "chunk behavior", "timed components"], note_rows),
        "",
        "## Correctness",
        "",
        md_table(["method", "length", "k", "mismatch_count"], mismatch_rows),
        "",
        f"Raw TSV: `{RAW_OUT.relative_to(ROOT)}`",
        f"Summary TSV: `{SUMMARY_OUT.relative_to(ROOT)}`",
        f"Mismatch debug TSV: `{MISMATCH_OUT.relative_to(ROOT)}`",
        "",
        "## Build And Run",
        "",
        "```bash",
        "make threshold_verify_bench",
        "python3 benchmark/scripts/run_threshold_verify_bench.py",
        "```",
    ]
    REPORT_OUT.write_text("\n".join(lines) + "\n")


def main():
    pairs, prefetch = parse_args()
    skipped = run_methods(pairs, prefetch)
    rows = build_summary()
    write_report(rows, skipped, prefetch)


if __name__ == "__main__":
    main()
