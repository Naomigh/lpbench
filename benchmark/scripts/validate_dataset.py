#!/usr/bin/env python3
"""Validate generated benchmark datasets."""

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
from typing import Dict, List, Tuple


VALIDATION_FIELDS = [
    "pair_id",
    "checked",
    "length_ok",
    "equal_read_ref_length_ok",
    "metadata_equal_length_ok",
    "indel_balance_ok",
    "alphabet_ok",
    "simulated_levenshtein_ok",
    "dp_levenshtein_metadata",
    "dp_levenshtein_recomputed",
    "dp_levenshtein_ok",
    "simulated_affine_metadata",
    "edit_script_affine_recomputed",
    "edit_script_affine_ok",
    "gotoh_affine_recomputed",
    "gotoh_affine_metadata",
    "gotoh_affine_ok",
    "has_substitution",
    "has_insertion",
    "has_deletion",
    "notes",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", required=True)
    parser.add_argument("--mode", choices=["levenshtein", "affine", "both"], required=True)
    parser.add_argument("--sample", default="all", help="Number of pairs to validate, or all.")
    parser.add_argument("--alphabet", default="ACGT")
    parser.add_argument("--mismatch", type=int, default=2)
    parser.add_argument("--gap-open", type=int, default=3)
    parser.add_argument("--gap-extend", type=int, default=1)
    parser.add_argument("--results-dir", default="benchmark/results")
    return parser.parse_args()


def levenshtein_dp(a: str, b: str) -> int:
    if len(a) < len(b):
        a, b = b, a
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i]
        for j, cb in enumerate(b, 1):
            cur.append(
                min(
                    prev[j] + 1,
                    cur[j - 1] + 1,
                    prev[j - 1] + (0 if ca == cb else 1),
                )
            )
        prev = cur
    return prev[-1]


def affine_gotoh_dp(
    reference: str, read: str, mismatch: int, gap_open: int, gap_extend: int
) -> int:
    # Minimum-cost global alignment. Gap length L costs gap_open + gap_extend * L.
    inf = 10**12
    n = len(reference)
    m = len(read)
    match = [[inf] * (m + 1) for _ in range(n + 1)]
    ins = [[inf] * (m + 1) for _ in range(n + 1)]
    dele = [[inf] * (m + 1) for _ in range(n + 1)]
    match[0][0] = 0
    for j in range(1, m + 1):
        ins[0][j] = min(match[0][j - 1] + gap_open + gap_extend, ins[0][j - 1] + gap_extend)
    for i in range(1, n + 1):
        dele[i][0] = min(match[i - 1][0] + gap_open + gap_extend, dele[i - 1][0] + gap_extend)
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            sub_cost = 0 if reference[i - 1] == read[j - 1] else mismatch
            match[i][j] = min(match[i - 1][j - 1], ins[i - 1][j - 1], dele[i - 1][j - 1]) + sub_cost
            ins[i][j] = min(match[i][j - 1] + gap_open + gap_extend, ins[i][j - 1] + gap_extend)
            dele[i][j] = min(match[i - 1][j] + gap_open + gap_extend, dele[i - 1][j] + gap_extend)
    return min(match[n][m], ins[n][m], dele[n][m])


def affine_from_script(script: List[dict], mismatch: int, gap_open: int, gap_extend: int) -> int:
    score = 0
    for event in script:
        op = event.get("op")
        if op == "X":
            score += mismatch
        elif op == "I":
            score += gap_open + gap_extend * len(event.get("seq", ""))
        elif op == "D":
            score += gap_open + gap_extend * len(event.get("seq", ""))
    return score


def read_tsvs(dataset: Path) -> Tuple[Dict[str, dict], Dict[str, dict]]:
    with (dataset / "pairs.tsv").open(newline="") as fh:
        pairs = {row["pair_id"]: row for row in csv.DictReader(fh, delimiter="\t")}
    with (dataset / "metadata.tsv").open(newline="") as fh:
        metadata = {row["pair_id"]: row for row in csv.DictReader(fh, delimiter="\t")}
    return pairs, metadata


def sample_ids(ids: List[str], sample_arg: str) -> List[str]:
    if sample_arg == "all":
        return ids
    n = int(sample_arg)
    return ids[: max(0, min(n, len(ids)))]


def int_field(row: dict, name: str, default: int = 0) -> int:
    value = row.get(name, "")
    return default if value == "" else int(value)


def validate_pair(
    pair_id: str,
    pair: dict,
    meta: dict,
    args: argparse.Namespace,
) -> dict:
    reference = pair["reference"]
    read = pair["read"]
    alphabet = set(args.alphabet)
    notes = []
    length_ok = (
        int_field(meta, "ref_length") == len(reference)
        and int_field(meta, "read_length") == len(read)
    )
    equal_read_ref_length_ok = len(reference) == len(read)
    metadata_equal_length_ok = meta.get("equal_length", "").lower() in ("true", "1", "yes") if "EQLEN" in meta.get("dataset_name", "") else True
    indel_balance_ok = int_field(meta, "num_inserted_bases") == int_field(meta, "num_deleted_bases")
    alphabet_ok = all(base in alphabet for base in reference) and all(base in alphabet for base in read)
    simulated_lev = int_field(meta, "simulated_levenshtein_distance")
    count_lev = (
        int_field(meta, "num_substitutions")
        + int_field(meta, "num_inserted_bases")
        + int_field(meta, "num_deleted_bases")
    )
    simulated_lev_ok = simulated_lev == count_lev

    dp_recomputed = ""
    dp_ok = ""
    if args.mode in ("levenshtein", "both", "affine"):
        dp_recomputed = levenshtein_dp(reference, read)
        stored = meta.get("dp_levenshtein_distance", "")
        if stored == "":
            dp_ok = "NA"
            notes.append("missing_dp_levenshtein_metadata")
        else:
            dp_ok = int(stored) == dp_recomputed

    try:
        script = json.loads(meta.get("edit_script", "[]"))
    except json.JSONDecodeError:
        script = []
        notes.append("invalid_edit_script_json")

    script_affine = affine_from_script(script, args.mismatch, args.gap_open, args.gap_extend)
    simulated_affine = int_field(meta, "simulated_affine_score")
    script_affine_ok = script_affine == simulated_affine
    gotoh = ""
    gotoh_ok = ""
    if args.mode in ("affine", "both"):
        gotoh = affine_gotoh_dp(reference, read, args.mismatch, args.gap_open, args.gap_extend)
        stored_gotoh = meta.get("gotoh_affine_score", "")
        gotoh_ok = "NA" if stored_gotoh == "" else int(stored_gotoh) == gotoh
    has_sub = int_field(meta, "num_substitutions") > 0
    has_ins = int_field(meta, "num_inserted_bases") > 0
    has_del = int_field(meta, "num_deleted_bases") > 0

    return {
        "pair_id": pair_id,
        "checked": True,
        "length_ok": length_ok,
        "equal_read_ref_length_ok": equal_read_ref_length_ok,
        "metadata_equal_length_ok": metadata_equal_length_ok,
        "indel_balance_ok": indel_balance_ok,
        "alphabet_ok": alphabet_ok,
        "simulated_levenshtein_ok": simulated_lev_ok,
        "dp_levenshtein_metadata": meta.get("dp_levenshtein_distance", ""),
        "dp_levenshtein_recomputed": dp_recomputed,
        "dp_levenshtein_ok": dp_ok,
        "simulated_affine_metadata": simulated_affine,
        "edit_script_affine_recomputed": script_affine,
        "edit_script_affine_ok": script_affine_ok,
        "gotoh_affine_recomputed": gotoh,
        "gotoh_affine_metadata": meta.get("gotoh_affine_score", ""),
        "gotoh_affine_ok": gotoh_ok,
        "has_substitution": has_sub,
        "has_insertion": has_ins,
        "has_deletion": has_del,
        "notes": ",".join(notes),
    }


def write_report(dataset_name: str, rows: List[dict], pair_id_match: bool, args: argparse.Namespace) -> None:
    results_dir = Path(args.results_dir)
    processed_dir = results_dir / "processed"
    reports_dir = results_dir / "reports"
    processed_dir.mkdir(parents=True, exist_ok=True)
    reports_dir.mkdir(parents=True, exist_ok=True)

    tsv_path = processed_dir / f"{dataset_name}_validation.tsv"
    with tsv_path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=VALIDATION_FIELDS, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

    def count_bad(field: str) -> int:
        return sum(1 for row in rows if row[field] not in (True, "True", "NA", ""))

    md_path = reports_dir / f"{dataset_name}_validation.md"
    with md_path.open("w") as fh:
        fh.write(f"# Validation report: {dataset_name}\n\n")
        fh.write(f"- Dataset: `{args.dataset}`\n")
        fh.write(f"- Mode: `{args.mode}`\n")
        fh.write(f"- Checked pairs: {len(rows)}\n")
        fh.write(f"- Pair ID sets match: {pair_id_match}\n")
        fh.write(f"- Length mismatches: {count_bad('length_ok')}\n")
        fh.write(f"- Equal read/reference length mismatches: {count_bad('equal_read_ref_length_ok')}\n")
        fh.write(f"- Metadata equal_length mismatches: {count_bad('metadata_equal_length_ok')}\n")
        fh.write(f"- Per-pair inserted/deleted base imbalance: {count_bad('indel_balance_ok')}\n")
        fh.write(f"- Alphabet mismatches: {count_bad('alphabet_ok')}\n")
        fh.write(f"- Simulated Levenshtein count mismatches: {count_bad('simulated_levenshtein_ok')}\n")
        fh.write(f"- Stored DP Levenshtein mismatches: {count_bad('dp_levenshtein_ok')}\n")
        fh.write(f"- Edit-script affine score mismatches: {count_bad('edit_script_affine_ok')}\n\n")
        total_sub = sum(1 for row in rows if row["has_substitution"] is True)
        total_ins = sum(1 for row in rows if row["has_insertion"] is True)
        total_del = sum(1 for row in rows if row["has_deletion"] is True)
        both_indel = sum(1 for row in rows if row["has_insertion"] is True and row["has_deletion"] is True)
        all_three = sum(
            1
            for row in rows
            if row["has_substitution"] is True and row["has_insertion"] is True and row["has_deletion"] is True
        )
        denom = max(1, len(rows))
        fh.write("## Equal-length mixed-edit checks\n\n")
        fh.write(f"- All checked pairs are equal length: {count_bad('equal_read_ref_length_ok') == 0}\n")
        fh.write(f"- All checked pairs have balanced inserted/deleted bases: {count_bad('indel_balance_ok') == 0}\n")
        fh.write(f"- Pairs with substitutions: {total_sub} ({total_sub / denom:.3f})\n")
        fh.write(f"- Pairs with insertions: {total_ins} ({total_ins / denom:.3f})\n")
        fh.write(f"- Pairs with deletions: {total_del} ({total_del / denom:.3f})\n")
        fh.write(f"- Pairs with both insertion and deletion: {both_indel} ({both_indel / denom:.3f})\n")
        fh.write(f"- Pairs with all three edit types: {all_three} ({all_three / denom:.3f})\n\n")
        if args.mode in ("affine", "both"):
            gotoh_vs_script = sum(
                1
                for row in rows
                if row["gotoh_affine_recomputed"] != ""
                and int(row["gotoh_affine_recomputed"]) != int(row["edit_script_affine_recomputed"])
            )
            fh.write(f"- Gotoh DP vs simulated script affine differences: {gotoh_vs_script}\n\n")
            fh.write(f"- Stored Gotoh affine mismatches: {count_bad('gotoh_affine_ok')}\n\n")
            fh.write(
                "Gotoh DP can be lower than the simulated script score when random edits create an alternate lower-cost alignment. "
                "Use the Gotoh DP score as the authoritative affine oracle when benchmark metadata is extended with that field.\n"
            )


def main() -> None:
    args = parse_args()
    dataset = Path(args.dataset)
    pairs, metadata = read_tsvs(dataset)
    pair_id_match = set(pairs) == set(metadata)
    ids = sample_ids(sorted(set(pairs) & set(metadata)), args.sample)
    rows = [validate_pair(pair_id, pairs[pair_id], metadata[pair_id], args) for pair_id in ids]
    write_report(dataset.name, rows, pair_id_match, args)
    bad_rows = [
        row
        for row in rows
        if row["length_ok"] is not True
        or row["alphabet_ok"] is not True
        or row["equal_read_ref_length_ok"] is not True
        or row["metadata_equal_length_ok"] is not True
        or row["indel_balance_ok"] is not True
        or row["simulated_levenshtein_ok"] is not True
        or row["dp_levenshtein_ok"] is False
        or row["edit_script_affine_ok"] is not True
    ]
    print(
        f"validated {len(rows)} pairs from {dataset.name}; "
        f"pair_ids_match={pair_id_match}; mismatching_rows={len(bad_rows)}"
    )


if __name__ == "__main__":
    main()
