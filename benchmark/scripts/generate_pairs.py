#!/usr/bin/env python3
import argparse
import os
import random
import subprocess
import sys
from pathlib import Path


DNA = "ACGT"


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def mutate_to_length(read: str, reference_len: int, max_edit: int, rng: random.Random) -> str:
    min_ed = abs(reference_len - len(read))
    if min_ed > max_edit:
        raise ValueError(
            f"abs(reference_len - read_len)={min_ed} exceeds max_edit={max_edit}; "
            "cannot guarantee edit_distance <= max_edit"
        )
    target_ed = rng.randint(min_ed, max_edit)
    seq = list(read)
    used = 0

    while len(seq) < reference_len:
        seq.insert(rng.randrange(len(seq) + 1), rng.choice(DNA))
        used += 1
    while len(seq) > reference_len:
        del seq[rng.randrange(len(seq))]
        used += 1

    while used < target_ed:
        remaining = target_ed - used
        possible = ["sub"]
        if remaining >= 2 and len(seq) > 0:
            possible.extend(["ins_del", "del_ins"])
        op = rng.choice(possible)
        if op == "sub":
            i = rng.randrange(len(seq))
            old = seq[i]
            choices = [b for b in DNA if b != old]
            seq[i] = rng.choice(choices)
            used += 1
        elif op in ("ins_del", "ins"):
            if remaining < 2:
                continue
            seq.insert(rng.randrange(len(seq) + 1), rng.choice(DNA))
            del seq[rng.randrange(len(seq))]
            used += 2
        else:
            if remaining < 2:
                continue
            del seq[rng.randrange(len(seq))]
            seq.insert(rng.randrange(len(seq) + 1), rng.choice(DNA))
            used += 2
    return "".join(seq)


def edlib_distances(helper: Path, rows):
    payload = "".join(f"{pair_id}\t{read}\t{ref}\n" for pair_id, read, ref in rows)
    proc = subprocess.run(
        [str(helper)],
        input=payload,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"edlib helper failed:\n{proc.stderr}")
    distances = {}
    for line in proc.stdout.splitlines():
        pair_id, ed = line.split("\t")
        distances[pair_id] = int(ed)
    return distances


def generate(args):
    rng = random.Random(args.seed)
    helper = Path(args.edlib_helper)
    if not helper.exists():
        raise FileNotFoundError(f"edlib helper not found: {helper}")
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)

    rows = []
    attempts = 0
    while len(rows) < args.num_pairs:
        batch = []
        while len(batch) < min(args.batch_size, args.num_pairs - len(rows)):
            attempts += 1
            if attempts > args.num_pairs * 100:
                raise RuntimeError("too many rejected candidates; check generation settings")
            read = "".join(rng.choice(DNA) for _ in range(args.read_len))
            reference = mutate_to_length(read, args.reference_len, args.max_edit, rng)
            pair_id = f"ed{args.max_edit}_{len(rows) + len(batch):06d}"
            batch.append((pair_id, read, reference))
        distances = edlib_distances(helper, batch)
        for pair_id, read, reference in batch:
            ed = distances[pair_id]
            if ed <= args.max_edit:
                rows.append((pair_id, read, reference, ed))
                if len(rows) >= args.num_pairs:
                    break
        if attempts > args.num_pairs * 100:
            raise RuntimeError("too many rejected candidates; check generation settings")

    with out.open("w", encoding="utf-8") as fh:
        fh.write("pair_id\tread\treference\tground_truth_ed\tmax_edit\n")
        for pair_id, read, reference, ed in rows:
            fh.write(f"{pair_id}\t{read}\t{reference}\t{ed}\t{args.max_edit}\n")


def main(argv=None):
    root = repo_root()
    parser = argparse.ArgumentParser()
    parser.add_argument("--num-pairs", type=int, default=10000)
    parser.add_argument("--read-len", type=int, default=100)
    parser.add_argument("--reference-len", type=int)
    parser.add_argument("--max-edit", type=int, required=True)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out", required=True)
    parser.add_argument("--edlib-helper", default=str(root / "benchmark" / "build" / "edlib_gt"))
    parser.add_argument("--batch-size", type=int, default=512)
    args = parser.parse_args(argv)
    if args.reference_len is None:
        args.reference_len = args.read_len
    generate(args)


if __name__ == "__main__":
    main()
