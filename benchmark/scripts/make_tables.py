#!/usr/bin/env python3
"""Generate Markdown/LaTeX tables and report drafts from summary.tsv."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


LEV_METHODS = [("leap", "LEAP-AVX512"), ("edlib", "edlib"), ("lv89", "lv89"), ("miniwfa", "miniWFA"), ("wfa2", "WFA2"), ("ksw2", "KSW2")]
AFF_METHODS = [("leap", "LEAP-AVX512"), ("miniwfa", "miniWFA"), ("wfa2", "WFA2-affine"), ("ksw2", "KSW2")]


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--summary", default="benchmark/results/processed/summary.tsv")
    p.add_argument("--results-dir", default="benchmark/results")
    return p.parse_args()


def read_summary(path: Path) -> list[dict]:
    if not path.exists():
        return []
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh, delimiter="\t"))


def usable(row: dict) -> bool:
    notes = row.get("notes", "")
    if "oracle_fallback" in notes or "fallback_used=true" in notes:
        return False
    try:
        return (
            int(row["score_mismatches"]) == 0
            and int(row["pass_mismatches"]) == 0
            and int(row["cigar_replay_failures"]) == 0
        )
    except Exception:
        return False


def fmt_speed(row: dict | None) -> str:
    if row is None or not usable(row):
        return "N/A"
    try:
        return f"{float(row['seconds_per_10M']):.3f}"
    except Exception:
        return "N/A"


def find_row(rows: list[dict], mode: str, method: str, threshold: str, dataset_contains: str) -> dict | None:
    candidates = [
        r for r in rows
        if r["mode"] == mode and r["method"] == method and r["threshold"] == threshold and dataset_contains in r["dataset"]
    ]
    return candidates[0] if candidates else None


def speed_table(rows: list[dict], mode: str, methods: list[tuple[str, str]], dataset_contains: str) -> tuple[list[str], list[list[str]]]:
    thresholds = sorted({int(r["threshold"]) for r in rows if r["mode"] == mode and dataset_contains in r["dataset"]})
    label = "k" if mode == "levenshtein" else "T"
    header = [label] + [name for _, name in methods] + ["Best non-LEAP", "Speedup vs best non-LEAP"]
    body = []
    for t in thresholds:
        values = {}
        row = [str(t)]
        for method, _ in methods:
            r = find_row(rows, mode, method, str(t), dataset_contains)
            cell = fmt_speed(r)
            row.append(cell)
            if cell != "N/A":
                values[method] = float(cell)
        non_leap = {m: v for m, v in values.items() if m != "leap"}
        best = min(non_leap.values()) if non_leap else None
        row.append("N/A" if best is None else f"{best:.3f}")
        if best is not None and "leap" in values and values["leap"] > 0:
            row.append(f"{best / values['leap']:.2f}x")
        else:
            row.append("N/A")
        body.append(row)
    return header, body


def write_md(path: Path, header: list[str], body: list[list[str]], title: str, unit_note: str = "") -> None:
    with path.open("w") as fh:
        fh.write(f"# {title}\n\n")
        if unit_note:
            fh.write(unit_note + "\n\n")
        fh.write("| " + " | ".join(header) + " |\n")
        fh.write("|" + "|".join(["---"] * len(header)) + "|\n")
        for row in body:
            fh.write("| " + " | ".join(row) + " |\n")


def write_tex(path: Path, header: list[str], body: list[list[str]]) -> None:
    with path.open("w") as fh:
        fh.write("\\begin{tabular}{" + "l" * len(header) + "}\n\\hline\n")
        fh.write(" & ".join(header).replace("_", "\\_") + " \\\\\n\\hline\n")
        for row in body:
            fh.write(" & ".join(row).replace("_", "\\_") + " \\\\\n")
        fh.write("\\hline\n\\end{tabular}\n")


def correctness_tables(rows: list[dict]) -> tuple[list[str], list[list[str]]]:
    header = ["mode", "dataset", "method", "threshold", "pairs", "score mismatches", "pass/fail mismatches", "CIGAR replay failures", "notes"]
    body = []
    for r in rows:
        body.append([
            r["mode"], r["dataset"], r["method"], r["threshold"], r["num_pairs"],
            r["score_mismatches"], r["pass_mismatches"], r["cigar_replay_failures"], r["notes"],
        ])
    return header, body


def write_reports(results_dir: Path) -> None:
    reports = results_dir / "reports"
    tables = results_dir / "tables"
    reports.mkdir(parents=True, exist_ok=True)
    lev = (tables / "table_levenshtein_speed.md").read_text() if (tables / "table_levenshtein_speed.md").exists() else ""
    aff = (tables / "table_affine_speed.md").read_text() if (tables / "table_affine_speed.md").exists() else ""
    correctness = (tables / "table_correctness.md").read_text() if (tables / "table_correctness.md").exists() else ""
    (reports / "benchmark_report.md").write_text(
        "# Benchmark report\n\n"
        "LEAP means the current AVX512 implementation in `./leap`; original LEAP-BV is not included.\n\n"
        "The first LEAP-AVX512 benchmark uses equal-length mixed-edit short-read datasets. "
        "Insertions and deletions are present, but total inserted bases equals total deleted bases per pair. "
        "This avoids the current LEAP wrapper fallback on variable-length pairs and measures the native AVX512 path. "
        "These datasets do not cover arbitrary variable-length alignment cases.\n\n"
        "## Levenshtein speed\n\n" + lev + "\n\n"
        "## Affine speed\n\n" + aff + "\n\n"
        "## Correctness\n\n" + correctness + "\n\n"
        "Unsupported methods and fallback rows are reported as N/A in speed tables. Correctness mismatches must be resolved before using a row as a final performance number.\n"
    )
    (reports / "paper_result_section_draft.md").write_text(
        "# Paper-style result section draft\n\n"
        "We evaluated LEAP-AVX512 as a short-read bounded alignment kernel on simulated equal-length mixed-edit read-reference pairs. "
        "The datasets include substitutions, insertions, and deletions; insertion and deletion base counts are balanced per pair so the final read length equals the reference length. "
        "This design measures the current AVX512 path and should not be interpreted as coverage of all variable-length alignment cases.\n\n"
        "Levenshtein and affine-gap conditions are reported separately. Unsupported or fallback method/task combinations are marked N/A, and correctness mismatches are reported alongside speed results.\n"
    )


def main() -> None:
    args = parse_args()
    rows = read_summary(Path(args.summary))
    results_dir = Path(args.results_dir)
    tables = results_dir / "tables"
    tables.mkdir(parents=True, exist_ok=True)
    header, body = speed_table(rows, "levenshtein", LEV_METHODS, "EQLEN-BAL")
    write_md(
        tables / "table_levenshtein_speed.md", header, body, "Levenshtein speed table",
        "Unit: seconds per 10 million pairs. `N/A` means unsupported, failed, fallback, or unavailable."
    )
    write_tex(tables / "table_levenshtein_speed.tex", header, body)
    header, body = speed_table(rows, "affine", AFF_METHODS, "EQLEN-GAP")
    write_md(
        tables / "table_affine_speed.md", header, body, "Affine gap speed table",
        "Unit: seconds per 10 million pairs. `N/A` means unsupported, failed, fallback, or unavailable."
    )
    write_tex(tables / "table_affine_speed.tex", header, body)
    header, body = correctness_tables(rows)
    write_md(
        tables / "table_correctness.md", header, body, "Correctness table",
        "Rows with nonzero mismatches are not eligible for final speed claims."
    )
    write_tex(tables / "table_correctness.tex", header, body)
    write_reports(results_dir)
    print(f"wrote tables under {tables}")


if __name__ == "__main__":
    main()
