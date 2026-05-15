#!/usr/bin/env python3
"""Generate synthetic short-read read-reference pair datasets."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import random
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


GENERATOR_VERSION = "phase-eqlen.1"
METADATA_FIELDS = [
    "pair_id",
    "dataset_name",
    "ref_length",
    "read_length",
    "equal_length",
    "bucket",
    "num_substitutions",
    "num_inserted_bases",
    "num_deleted_bases",
    "num_insertion_events",
    "num_deletion_events",
    "simulated_levenshtein_distance",
    "dp_levenshtein_distance",
    "simulated_affine_score",
    "gotoh_affine_score",
    "edit_script",
    "seed",
    "random_seed",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument(
        "--model",
        choices=["balanced", "gap", "eqlen_balanced", "eqlen_gap"],
        help="Generation model. If omitted, inferred from dataset name suffix.",
    )
    parser.add_argument("--equal-length", action="store_true", help="Alias for equal-length model inference.")
    parser.add_argument("--num-pairs", type=int, required=True)
    parser.add_argument("--ref-length", type=int, required=True)
    parser.add_argument("--max-distance", type=int)
    parser.add_argument("--max-affine-score", type=int)
    parser.add_argument("--mismatch-penalty", type=int, default=2)
    parser.add_argument("--gap-open-penalty", type=int, default=3)
    parser.add_argument("--gap-extend-penalty", type=int, default=1)
    parser.add_argument("--mean-gap-length", type=float, default=3.0)
    parser.add_argument("--max-gap-length", type=int, default=20)
    parser.add_argument("--alphabet", default="ACGT")
    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument(
        "--dp-validation",
        choices=["all", "none"],
        default="none",
        help="Compute exact Levenshtein DP during generation. Use all for small/debug datasets.",
    )
    return parser.parse_args()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def random_seq(rng: random.Random, length: int, alphabet: str) -> str:
    return "".join(rng.choice(alphabet) for _ in range(length))


def different_base(rng: random.Random, base: str, alphabet: str) -> str:
    choices = [x for x in alphabet if x != base]
    return rng.choice(choices)


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


def affine_gotoh_dp(reference: str, read: str, mismatch: int, gap_open: int, gap_extend: int) -> int:
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


def affine_score_from_counts(
    substitutions: int,
    insertions: Sequence[int],
    deletions: Sequence[int],
    mismatch: int,
    gap_open: int,
    gap_extend: int,
) -> int:
    return (
        mismatch * substitutions
        + sum(gap_open + gap_extend * n for n in insertions)
        + sum(gap_open + gap_extend * n for n in deletions)
    )


def bucket_counts(total: int, buckets: Sequence[int]) -> Dict[int, int]:
    base = total // len(buckets)
    rem = total % len(buckets)
    return {bucket: base + (1 if i < rem else 0) for i, bucket in enumerate(buckets)}


def finalize_meta(
    pair_id: str,
    dataset_name: str,
    ref_length: int,
    read: str,
    bucket: int,
    substitutions: Dict[int, str],
    insertions: Dict[int, List[str]],
    deletions: Dict[int, int],
    script: List[dict],
    pair_seed: int,
    do_dp: bool,
    mismatch: int,
    gap_open: int,
    gap_extend: int,
    reference: str = "",
) -> dict:
    insertion_lengths = [len(seq) for seqs in insertions.values() for seq in seqs]
    deletion_lengths = list(deletions.values())
    num_subs = len(substitutions)
    simulated_lev = num_subs + sum(insertion_lengths) + sum(deletion_lengths)
    simulated_affine = affine_score_from_counts(
        num_subs, insertion_lengths, deletion_lengths, mismatch, gap_open, gap_extend
    )
    dp_lev = levenshtein_dp(reference, read) if do_dp and reference else ""
    gotoh = affine_gotoh_dp(reference, read, mismatch, gap_open, gap_extend) if do_dp and reference else ""
    return {
        "pair_id": pair_id,
        "dataset_name": dataset_name,
        "ref_length": ref_length,
        "read_length": len(read),
        "equal_length": str(len(read) == ref_length).lower(),
        "bucket": bucket,
        "num_substitutions": num_subs,
        "num_inserted_bases": sum(insertion_lengths),
        "num_deleted_bases": sum(deletion_lengths),
        "num_insertion_events": len(insertion_lengths),
        "num_deletion_events": len(deletion_lengths),
        "simulated_levenshtein_distance": simulated_lev,
        "dp_levenshtein_distance": dp_lev,
        "simulated_affine_score": simulated_affine,
        "gotoh_affine_score": gotoh,
        "edit_script": json.dumps(script, separators=(",", ":")),
        "seed": pair_seed,
        "random_seed": pair_seed,
    }


def choose_balanced_ops(rng: random.Random, distance: int, pair_index: int) -> List[str]:
    if distance == 0:
        return []
    ops = []
    cycle = ["X", "I", "D"]
    for j in range(distance):
        if rng.random() < 0.75:
            ops.append(cycle[(pair_index + j) % 3])
        else:
            ops.append(rng.choice(cycle))
    rng.shuffle(ops)
    return ops


def build_read_from_events(
    reference: str,
    substitutions: Dict[int, str],
    insertions: Dict[int, List[str]],
    deletions: Dict[int, int],
) -> Tuple[str, List[dict]]:
    read_parts: List[str] = []
    script: List[dict] = []
    ref_len = len(reference)
    pos = 0
    while pos <= ref_len:
        if pos in insertions:
            for seq in insertions[pos]:
                read_parts.append(seq)
                script.append({"op": "I", "ref_pos": pos, "seq": seq})
        if pos == ref_len:
            break
        if pos in deletions:
            length = deletions[pos]
            seq = reference[pos : pos + length]
            script.append({"op": "D", "ref_pos": pos, "seq": seq})
            pos += length
            continue
        if pos in substitutions:
            to_base = substitutions[pos]
            read_parts.append(to_base)
            script.append(
                {"op": "X", "ref_pos": pos, "from": reference[pos], "to": to_base}
            )
        else:
            read_parts.append(reference[pos])
        pos += 1
    return "".join(read_parts), script


def make_balanced_pair(
    pair_id: str,
    dataset_name: str,
    ref_length: int,
    distance: int,
    alphabet: str,
    pair_seed: int,
    pair_index: int,
    do_dp: bool,
    mismatch: int,
    gap_open: int,
    gap_extend: int,
) -> Tuple[Tuple[str, str, str], dict]:
    rng = random.Random(pair_seed)
    reference = random_seq(rng, ref_length, alphabet)
    ops = choose_balanced_ops(rng, distance, pair_index)
    substitutions: Dict[int, str] = {}
    insertions: Dict[int, List[str]] = defaultdict(list)
    deletions: Dict[int, int] = {}
    used_ref_positions = set()

    for op in ops:
        if op in ("X", "D") and len(used_ref_positions) >= ref_length:
            op = "I"
        if op == "X":
            available = [i for i in range(ref_length) if i not in used_ref_positions]
            pos = rng.choice(available)
            used_ref_positions.add(pos)
            substitutions[pos] = different_base(rng, reference[pos], alphabet)
        elif op == "D":
            available = [i for i in range(ref_length) if i not in used_ref_positions]
            pos = rng.choice(available)
            used_ref_positions.add(pos)
            deletions[pos] = 1
        else:
            pos = rng.randrange(ref_length + 1)
            insertions[pos].append(random_seq(rng, 1, alphabet))

    read, script = build_read_from_events(reference, substitutions, insertions, deletions)
    meta = finalize_meta(
        pair_id, dataset_name, ref_length, read, distance, substitutions, insertions,
        deletions, script, pair_seed, do_dp, mismatch, gap_open, gap_extend, reference
    )
    return (pair_id, reference, read), meta


def sample_gap_length(rng: random.Random, mean: float, max_gap: int) -> int:
    # Geometric distribution on 1, 2, ... with E[L] approximately mean.
    p = 1.0 / max(mean, 1.0)
    u = rng.random()
    length = int(math.floor(math.log(1.0 - u) / math.log(1.0 - p))) + 1 if p < 1 else 1
    return max(1, min(max_gap, length))


def choose_eqlen_counts(rng: random.Random, distance: int, pair_index: int) -> Tuple[int, int]:
    """Return substitutions and paired indel bases for an equal-length edit count."""
    if distance <= 0:
        return 0, 0
    candidates = []
    for indel_bases in range(distance // 2 + 1):
        substitutions = distance - 2 * indel_bases
        balance_penalty = abs(substitutions - distance / 3.0) + abs(indel_bases - distance / 3.0)
        if distance >= 2 and indel_bases == 0:
            balance_penalty += 2.0
        candidates.append((balance_penalty + rng.random() * 0.25, substitutions, indel_bases))
    candidates.sort()
    if distance >= 2 and pair_index % 5 == 0:
        # Preserve a controlled minority of substitution-heavy equal-length pairs.
        return distance, 0
    _, substitutions, indel_bases = candidates[0]
    return substitutions, indel_bases


def nonoverlap_deletions(rng: random.Random, ref_length: int, lengths: Sequence[int], blocked=None) -> Dict[int, int]:
    blocked_set = set(blocked or [])
    deletions: Dict[int, int] = {}
    used = set()
    for length in sorted(lengths, reverse=True):
        starts = [
            i
            for i in range(0, ref_length - length + 1)
            if all((i + j) not in used and (i + j) not in blocked_set for j in range(length))
        ]
        if not starts:
            raise ValueError("not enough reference positions for deletion gaps")
        pos = rng.choice(starts)
        deletions[pos] = length
        for j in range(length):
            used.add(pos + j)
    return deletions


def add_random_insertions(
    rng: random.Random,
    ref_length: int,
    lengths: Sequence[int],
    alphabet: str,
    avoid_positions=None,
) -> Dict[int, List[str]]:
    avoid = set(avoid_positions or [])
    insertions: Dict[int, List[str]] = defaultdict(list)
    candidates = [i for i in range(ref_length + 1) if i == ref_length or i not in avoid]
    if not candidates:
        candidates = [ref_length]
    for length in lengths:
        insertions[rng.choice(candidates)].append(random_seq(rng, length, alphabet))
    return insertions


def make_eqlen_balanced_pair(
    pair_id: str,
    dataset_name: str,
    ref_length: int,
    distance: int,
    alphabet: str,
    pair_seed: int,
    pair_index: int,
    do_dp: bool,
    mismatch: int,
    gap_open: int,
    gap_extend: int,
) -> Tuple[Tuple[str, str, str], dict]:
    rng = random.Random(pair_seed)
    reference = random_seq(rng, ref_length, alphabet)
    substitutions_n, indel_bases = choose_eqlen_counts(rng, distance, pair_index)
    deletion_lengths = [1] * indel_bases
    deletions = nonoverlap_deletions(rng, ref_length, deletion_lengths) if indel_bases else {}
    deleted_positions = set(deletions.keys())
    available_subs = [i for i in range(ref_length) if i not in deleted_positions]
    substitutions: Dict[int, str] = {}
    for pos in rng.sample(available_subs, min(substitutions_n, len(available_subs))):
        substitutions[pos] = different_base(rng, reference[pos], alphabet)
    blocked = set(deletions.keys()) | set(substitutions.keys())
    insertions = add_random_insertions(rng, ref_length, [1] * indel_bases, alphabet, blocked)
    read, script = build_read_from_events(reference, substitutions, insertions, deletions)
    if len(read) != ref_length:
        raise AssertionError("equal-length balanced generator produced variable-length read")
    meta = finalize_meta(
        pair_id, dataset_name, ref_length, read, distance, substitutions, insertions,
        deletions, script, pair_seed, do_dp, mismatch, gap_open, gap_extend, reference
    )
    return (pair_id, reference, read), meta


def make_eqlen_gap_pair(
    pair_id: str,
    dataset_name: str,
    ref_length: int,
    target_score: int,
    alphabet: str,
    pair_seed: int,
    mismatch: int,
    gap_open: int,
    gap_extend: int,
    mean_gap: float,
    max_gap: int,
    do_dp: bool,
) -> Tuple[Tuple[str, str, str], dict]:
    rng = random.Random(pair_seed)
    reference = random_seq(rng, ref_length, alphabet)
    score = 0
    gap_lengths: List[int] = []
    min_pair_cost = 2 * (gap_open + gap_extend)
    # Prefer paired gaps once the target score can pay for at least one pair.
    while target_score - score >= min_pair_cost and rng.random() < 0.75:
        length = sample_gap_length(rng, mean_gap, max_gap)
        while length > 1 and score + 2 * (gap_open + gap_extend * length) > target_score:
            length -= 1
        cost = 2 * (gap_open + gap_extend * length)
        if score + cost > target_score:
            break
        gap_lengths.append(length)
        score += cost
    substitutions_n = max(0, (target_score - score) // max(1, mismatch))
    try:
        deletions = nonoverlap_deletions(rng, ref_length, gap_lengths)
    except ValueError:
        deletions = {}
        gap_lengths = []
        score = 0
        substitutions_n = target_score // max(1, mismatch)
    deleted_positions = {pos + j for pos, length in deletions.items() for j in range(length)}
    available_subs = [i for i in range(ref_length) if i not in deleted_positions]
    substitutions: Dict[int, str] = {}
    for pos in rng.sample(available_subs, min(substitutions_n, len(available_subs))):
        substitutions[pos] = different_base(rng, reference[pos], alphabet)
    insertions = add_random_insertions(rng, ref_length, gap_lengths, alphabet, deleted_positions | set(substitutions.keys()))
    read, script = build_read_from_events(reference, substitutions, insertions, deletions)
    if len(read) != ref_length:
        raise AssertionError("equal-length gap generator produced variable-length read")
    insertion_total = sum(len(seq) for seqs in insertions.values() for seq in seqs)
    deletion_total = sum(deletions.values())
    if insertion_total != deletion_total:
        raise AssertionError("equal-length gap generator produced unbalanced indel bases")
    simulated_affine = affine_score_from_counts(
        len(substitutions), [len(seq) for seqs in insertions.values() for seq in seqs],
        list(deletions.values()), mismatch, gap_open, gap_extend
    )
    meta = finalize_meta(
        pair_id, dataset_name, ref_length, read, simulated_affine, substitutions, insertions,
        deletions, script, pair_seed, do_dp, mismatch, gap_open, gap_extend, reference
    )
    return (pair_id, reference, read), meta


def make_gap_pair_for_score(
    pair_id: str,
    dataset_name: str,
    ref_length: int,
    target_score: int,
    alphabet: str,
    pair_seed: int,
    mismatch: int,
    gap_open: int,
    gap_extend: int,
    mean_gap: float,
    max_gap: int,
    do_dp: bool,
) -> Tuple[Tuple[str, str, str], dict]:
    rng = random.Random(pair_seed)
    reference = random_seq(rng, ref_length, alphabet)
    substitutions: Dict[int, str] = {}
    insertions: Dict[int, List[str]] = defaultdict(list)
    deletions: Dict[int, int] = {}
    used_ref_positions = set()
    insertion_anchors = set()
    score = 0
    attempts = 0

    while score < target_score and attempts < 10000:
        attempts += 1
        event = rng.choices(["X", "I", "D"], weights=[0.50, 0.25, 0.25], k=1)[0]
        if event == "X":
            cost = mismatch
            if score + cost > target_score or len(used_ref_positions) >= ref_length:
                continue
            available = [i for i in range(ref_length) if i not in used_ref_positions]
            pos = rng.choice(available)
            used_ref_positions.add(pos)
            substitutions[pos] = different_base(rng, reference[pos], alphabet)
            score += cost
        elif event == "I":
            length = sample_gap_length(rng, mean_gap, max_gap)
            cost = gap_open + gap_extend * length
            if score + cost > target_score:
                length = (target_score - score - gap_open) // gap_extend
                if length < 1:
                    continue
                cost = gap_open + gap_extend * length
            insertion_candidates = [
                i for i in range(ref_length + 1) if i == ref_length or i not in used_ref_positions
            ]
            if not insertion_candidates:
                continue
            pos = rng.choice(insertion_candidates)
            insertions[pos].append(random_seq(rng, length, alphabet))
            insertion_anchors.add(pos)
            score += cost
        else:
            length = sample_gap_length(rng, mean_gap, max_gap)
            cost = gap_open + gap_extend * length
            if score + cost > target_score:
                length = (target_score - score - gap_open) // gap_extend
                if length < 1:
                    continue
                cost = gap_open + gap_extend * length
            starts = [
                i
                for i in range(0, ref_length - length + 1)
                if all((i + j) not in used_ref_positions for j in range(length))
                and all((i + j) not in insertion_anchors for j in range(length))
            ]
            if not starts:
                continue
            pos = rng.choice(starts)
            for j in range(length):
                used_ref_positions.add(pos + j)
            deletions[pos] = length
            score += cost

    read, script = build_read_from_events(reference, substitutions, insertions, deletions)
    simulated_affine = affine_score_from_counts(
        len(substitutions),
        [len(seq) for seqs in insertions.values() for seq in seqs],
        list(deletions.values()),
        mismatch,
        gap_open,
        gap_extend,
    )
    meta = finalize_meta(
        pair_id, dataset_name, ref_length, read, simulated_affine, substitutions, insertions,
        deletions, script, pair_seed, do_dp, mismatch, gap_open, gap_extend, reference
    )
    return (pair_id, reference, read), meta


def write_dataset(args: argparse.Namespace, pairs: Iterable[Tuple[str, str, str]], metas: Iterable[dict], manifest_extra: dict) -> None:
    out_dir = Path(args.output_dir) / args.name
    out_dir.mkdir(parents=True, exist_ok=True)
    pairs_path = out_dir / "pairs.tsv"
    metadata_path = out_dir / "metadata.tsv"

    metas = list(metas)
    pairs = list(pairs)
    with pairs_path.open("w", newline="") as fh:
        writer = csv.writer(fh, delimiter="\t", lineterminator="\n")
        writer.writerow(["pair_id", "reference", "read"])
        writer.writerows(pairs)
    with metadata_path.open("w", newline="") as fh:
        writer = csv.DictWriter(
            fh, fieldnames=METADATA_FIELDS, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        for row in metas:
            writer.writerow(row)

    bucket_stats = Counter(int(row["bucket"]) for row in metas)
    bucket_detail = {}
    for bucket in sorted(bucket_stats):
        rows = [row for row in metas if int(row["bucket"]) == bucket]
        denom = max(1, len(rows))
        ins_events = sum(int(row["num_insertion_events"]) for row in rows)
        del_events = sum(int(row["num_deletion_events"]) for row in rows)
        bucket_detail[str(bucket)] = {
            "pair_count": len(rows),
            "mean_num_substitutions": sum(int(row["num_substitutions"]) for row in rows) / denom,
            "mean_num_inserted_bases": sum(int(row["num_inserted_bases"]) for row in rows) / denom,
            "mean_num_deleted_bases": sum(int(row["num_deleted_bases"]) for row in rows) / denom,
            "mean_dp_levenshtein_distance": (
                sum(int(row["dp_levenshtein_distance"] or 0) for row in rows) / denom
                if all(str(row["dp_levenshtein_distance"]) != "" for row in rows)
                else None
            ),
            "mean_insertion_gap_length": (
                sum(int(row["num_inserted_bases"]) for row in rows) / ins_events if ins_events else 0
            ),
            "mean_deletion_gap_length": (
                sum(int(row["num_deleted_bases"]) for row in rows) / del_events if del_events else 0
            ),
            "mean_simulated_affine_score": sum(int(row["simulated_affine_score"]) for row in rows) / denom,
            "mean_gotoh_affine_score": (
                sum(int(row["gotoh_affine_score"] or 0) for row in rows) / denom
                if all(str(row["gotoh_affine_score"]) != "" for row in rows)
                else None
            ),
        }
    count_totals = {
        "substitutions": sum(int(row["num_substitutions"]) for row in metas),
        "inserted_bases": sum(int(row["num_inserted_bases"]) for row in metas),
        "deleted_bases": sum(int(row["num_deleted_bases"]) for row in metas),
        "insertion_events": sum(int(row["num_insertion_events"]) for row in metas),
        "deletion_events": sum(int(row["num_deletion_events"]) for row in metas),
    }
    manifest = {
        "dataset_name": args.name,
        "generation_timestamp": datetime.now(timezone.utc).isoformat(),
        "generator_version": GENERATOR_VERSION,
        "random_seed": args.seed,
        "reference_length": args.ref_length,
        "number_of_pairs": args.num_pairs,
        "alphabet": args.alphabet,
        "bucket_statistics": dict(sorted(bucket_stats.items())),
        "bucket_detail": bucket_detail,
        "counts": count_totals,
        "total_substitutions": count_totals["substitutions"],
        "total_inserted_bases": count_totals["inserted_bases"],
        "total_deleted_bases": count_totals["deleted_bases"],
        "total_insertion_events": count_totals["insertion_events"],
        "total_deletion_events": count_totals["deletion_events"],
        "pairs_sha256": sha256_file(pairs_path),
        "metadata_sha256": sha256_file(metadata_path),
    }
    manifest.update(manifest_extra)
    with (out_dir / "manifest.json").open("w") as fh:
        json.dump(manifest, fh, indent=2, sort_keys=True)
        fh.write("\n")


def generate_balanced(args: argparse.Namespace) -> None:
    if args.max_distance is None:
        raise SystemExit("--max-distance is required for *-BAL datasets")
    buckets = list(range(args.max_distance + 1))
    counts = bucket_counts(args.num_pairs, buckets)
    master = random.Random(args.seed)
    pairs = []
    metas = []
    pair_index = 0
    for distance in buckets:
        for _ in range(counts[distance]):
            pair_seed = master.randrange(0, 2**63)
            pair_id = f"{args.name}_{pair_index:012d}"
            pair, meta = make_balanced_pair(
                pair_id,
                args.name,
                args.ref_length,
                distance,
                args.alphabet,
                pair_seed,
                pair_index,
                args.dp_validation == "all",
                args.mismatch_penalty,
                args.gap_open_penalty,
                args.gap_extend_penalty,
            )
            pairs.append(pair)
            metas.append(meta)
            pair_index += 1
    write_dataset(
        args,
        pairs,
        metas,
        {
            "error_model": "balanced_levenshtein",
            "scoring_parameters": {
                "mismatch": args.mismatch_penalty,
                "gap_open": args.gap_open_penalty,
                "gap_extend": args.gap_extend_penalty,
                "gap_convention": "gap_open + gap_extend * L",
            },
        },
    )


def generate_gap(args: argparse.Namespace) -> None:
    if args.max_affine_score is None:
        raise SystemExit("--max-affine-score is required for *-GAP datasets")
    buckets = list(range(args.max_affine_score + 1))
    counts = bucket_counts(args.num_pairs, buckets)
    master = random.Random(args.seed)
    pairs = []
    metas = []
    pair_index = 0
    for target in buckets:
        for _ in range(counts[target]):
            pair_seed = master.randrange(0, 2**63)
            pair_id = f"{args.name}_{pair_index:012d}"
            pair, meta = make_gap_pair_for_score(
                pair_id,
                args.name,
                args.ref_length,
                target,
                args.alphabet,
                pair_seed,
                args.mismatch_penalty,
                args.gap_open_penalty,
                args.gap_extend_penalty,
                args.mean_gap_length,
                args.max_gap_length,
                args.dp_validation == "all",
            )
            pairs.append(pair)
            metas.append(meta)
            pair_index += 1
    write_dataset(
        args,
        pairs,
        metas,
        {
            "error_model": "gap_heavy_affine",
            "scoring_parameters": {
                "mismatch": args.mismatch_penalty,
                "gap_open": args.gap_open_penalty,
                "gap_extend": args.gap_extend_penalty,
                "gap_convention": "gap_open + gap_extend * L",
                "mean_gap_length": args.mean_gap_length,
                "max_gap_length": args.max_gap_length,
            },
        },
    )


def generate_eqlen_balanced(args: argparse.Namespace) -> None:
    if args.max_distance is None:
        raise SystemExit("--max-distance is required for eqlen_balanced datasets")
    buckets = list(range(args.max_distance + 1))
    counts = bucket_counts(args.num_pairs, buckets)
    master = random.Random(args.seed)
    pairs = []
    metas = []
    pair_index = 0
    for distance in buckets:
        for _ in range(counts[distance]):
            pair_seed = master.randrange(0, 2**63)
            pair_id = f"{args.name}_{pair_index:012d}"
            pair, meta = make_eqlen_balanced_pair(
                pair_id, args.name, args.ref_length, distance, args.alphabet,
                pair_seed, pair_index, args.dp_validation == "all",
                args.mismatch_penalty, args.gap_open_penalty, args.gap_extend_penalty,
            )
            pairs.append(pair)
            metas.append(meta)
            pair_index += 1
    write_dataset(
        args,
        pairs,
        metas,
        {
            "error_model": "equal_length_balanced_levenshtein",
            "generation_model": "eqlen_balanced",
            "equal_length": True,
            "total_pairs": args.num_pairs,
            "ref_length": args.ref_length,
            "max_distance": args.max_distance,
            "scoring_parameters": {
                "mismatch": args.mismatch_penalty,
                "gap_open": args.gap_open_penalty,
                "gap_extend": args.gap_extend_penalty,
                "gap_convention": "gap_open + gap_extend * L",
            },
            "assertion": {
                "all_read_lengths_equal_reference_length": all(int(m["read_length"]) == args.ref_length for m in metas),
                "total_inserted_bases_equals_total_deleted_bases_per_pair": all(
                    int(m["num_inserted_bases"]) == int(m["num_deleted_bases"]) for m in metas
                ),
            },
        },
    )


def generate_eqlen_gap(args: argparse.Namespace) -> None:
    if args.max_affine_score is None:
        raise SystemExit("--max-affine-score is required for eqlen_gap datasets")
    buckets = list(range(args.max_affine_score + 1))
    counts = bucket_counts(args.num_pairs, buckets)
    master = random.Random(args.seed)
    pairs = []
    metas = []
    pair_index = 0
    for target in buckets:
        for _ in range(counts[target]):
            pair_seed = master.randrange(0, 2**63)
            pair_id = f"{args.name}_{pair_index:012d}"
            pair, meta = make_eqlen_gap_pair(
                pair_id, args.name, args.ref_length, target, args.alphabet, pair_seed,
                args.mismatch_penalty, args.gap_open_penalty, args.gap_extend_penalty,
                args.mean_gap_length, args.max_gap_length, args.dp_validation == "all",
            )
            pairs.append(pair)
            metas.append(meta)
            pair_index += 1
    write_dataset(
        args,
        pairs,
        metas,
        {
            "error_model": "equal_length_gap_heavy_affine",
            "generation_model": "eqlen_gap",
            "equal_length": True,
            "total_pairs": args.num_pairs,
            "ref_length": args.ref_length,
            "max_affine_score": args.max_affine_score,
            "scoring_parameters": {
                "mismatch": args.mismatch_penalty,
                "gap_open": args.gap_open_penalty,
                "gap_extend": args.gap_extend_penalty,
                "gap_convention": "gap_open + gap_extend * L",
                "mean_gap_length": args.mean_gap_length,
                "max_gap_length": args.max_gap_length,
            },
            "assertion": {
                "all_read_lengths_equal_reference_length": all(int(m["read_length"]) == args.ref_length for m in metas),
                "total_inserted_bases_equals_total_deleted_bases_per_pair": all(
                    int(m["num_inserted_bases"]) == int(m["num_deleted_bases"]) for m in metas
                ),
            },
        },
    )


def main() -> None:
    args = parse_args()
    if len(set(args.alphabet)) != len(args.alphabet) or len(args.alphabet) < 2:
        raise SystemExit("--alphabet must contain at least two unique symbols")
    model = args.model
    if model is None:
        if args.equal_length and args.name.endswith("-BAL"):
            model = "eqlen_balanced"
        elif args.equal_length and args.name.endswith("-GAP"):
            model = "eqlen_gap"
        elif "-EQLEN-" in args.name and args.name.endswith("-BAL"):
            model = "eqlen_balanced"
        elif "-EQLEN-" in args.name and args.name.endswith("-GAP"):
            model = "eqlen_gap"
        elif args.name.endswith("-BAL"):
            model = "balanced"
        elif args.name.endswith("-GAP"):
            model = "gap"
    if model == "eqlen_balanced":
        generate_eqlen_balanced(args)
    elif model == "eqlen_gap":
        generate_eqlen_gap(args)
    elif model == "balanced":
        generate_balanced(args)
    elif model == "gap":
        generate_gap(args)
    else:
        raise SystemExit("Cannot infer model. Use --model balanced|gap|eqlen_balanced|eqlen_gap")


if __name__ == "__main__":
    main()
