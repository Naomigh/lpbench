#!/usr/bin/env python3
"""Generate simple publication-style benchmark plots."""

from __future__ import annotations

import argparse
import base64
import csv
import json
from collections import Counter
from pathlib import Path


BLANK_PNG = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+/p9sAAAAASUVORK5CYII="
)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--summary", default="benchmark/results/processed/summary.tsv")
    p.add_argument("--data-dir", default="benchmark/data/generated")
    p.add_argument("--figures-dir", default="benchmark/results/figures")
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
    if row.get("median_seconds") in ("", "NA", None):
        return False
    return True


def save_blank(path: Path) -> None:
    path.write_bytes(BLANK_PNG)


def plot_speed(rows: list[dict], mode: str, path: Path, speedup: bool = False) -> None:
    try:
        import matplotlib.pyplot as plt
    except Exception:
        save_blank(path)
        return
    data = [r for r in rows if r["mode"] == mode and usable(r)]
    if not data:
        save_blank(path)
        return
    thresholds = sorted({int(r["threshold"]) for r in data})
    methods = sorted({r["method"] for r in data})
    plt.figure(figsize=(6, 4))
    for method in methods:
        xs, ys = [], []
        for t in thresholds:
            vals = [float(r["seconds_per_10M"]) for r in data if r["method"] == method and int(r["threshold"]) == t]
            if not vals:
                continue
            xs.append(t)
            if speedup:
                non_leap = [
                    float(r["seconds_per_10M"]) for r in data
                    if r["method"] != "leap" and int(r["threshold"]) == t
                ]
                ys.append((min(non_leap) / vals[0]) if non_leap and vals[0] else 0)
            else:
                ys.append(vals[0])
        if xs:
            plt.plot(xs, ys, marker="o", label=method)
    plt.xlabel("k" if mode == "levenshtein" else "T")
    plt.ylabel("speedup vs best non-LEAP" if speedup else "seconds per 10M pairs")
    plt.legend(frameon=False)
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()


def plot_distribution(data_dir: Path, pattern: str, out: Path, gap_lengths: bool = False) -> None:
    try:
        import matplotlib.pyplot as plt
    except Exception:
        save_blank(out)
        return
    counts = Counter()
    for meta in data_dir.glob(pattern + "/metadata.tsv"):
        with meta.open(newline="") as fh:
            for row in csv.DictReader(fh, delimiter="\t"):
                if gap_lengths:
                    try:
                        script = json.loads(row["edit_script"])
                    except Exception:
                        continue
                    for ev in script:
                        if ev.get("op") in ("I", "D"):
                            counts[f"{ev['op']}{len(ev.get('seq', ''))}"] += 1
                else:
                    counts["substitution"] += int(row["num_substitutions"])
                    counts["inserted bases"] += int(row["num_inserted_bases"])
                    counts["deleted bases"] += int(row["num_deleted_bases"])
    if not counts:
        save_blank(out)
        return
    labels, values = zip(*counts.most_common(20))
    plt.figure(figsize=(7, 4))
    plt.bar(labels, values)
    plt.xticks(rotation=45, ha="right")
    plt.tight_layout()
    plt.savefig(out, dpi=200)
    plt.close()


def main() -> None:
    args = parse_args()
    rows = read_summary(Path(args.summary))
    figures = Path(args.figures_dir)
    figures.mkdir(parents=True, exist_ok=True)
    plot_speed(rows, "levenshtein", figures / "levenshtein_speed_vs_k.png")
    plot_speed(rows, "levenshtein", figures / "levenshtein_speedup_vs_k.png", speedup=True)
    plot_speed(rows, "affine", figures / "affine_speed_vs_T.png")
    plot_speed(rows, "affine", figures / "affine_speedup_vs_T.png", speedup=True)
    plot_distribution(Path(args.data_dir), "*EQLEN-BAL", figures / "eqlen_edit_type_distribution.png")
    plot_distribution(Path(args.data_dir), "*EQLEN-GAP", figures / "eqlen_gap_length_distribution.png", gap_lengths=True)
    print(f"wrote figures under {figures}")


if __name__ == "__main__":
    main()
