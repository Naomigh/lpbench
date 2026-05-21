#!/usr/bin/env python3
"""Build, generate synthetic DNA pairs, run method timing, and render reports."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import os
import pathlib
import platform
import random
import subprocess
import sys
from typing import Iterable


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
BENCHMARK_ROOT = REPO_ROOT / "benchmark"
BUILD_DIR = BENCHMARK_ROOT / "build"
BINARY = BUILD_DIR / "bench_methods"
DNA = "ACGT"
METHODS = [
    "leap_avx2",
    "leap_avx512",
    "lv89",
    "miniwfa",
    "wfa2_reuse",
    "wfa2_fresh",
]
RAW_COLUMNS = [
    "method",
    "length",
    "k",
    "max_edit_distance",
    "pair_count",
    "timing_mode",
    "n1",
    "n2",
    "time_n1_ns",
    "time_n2_ns",
    "total_time_ns",
    "avg_ns_per_pair_batch",
    "avg_ns_per_pair_diff",
    "warmup_passes",
    "measured_boundary",
    "included",
    "excluded",
    "result_sink",
    "score_sink",
    "seed",
    "notes",
]
DATA_COLUMNS = [
    "pair_id",
    "length",
    "true_ed",
    "read",
    "reference",
    "num_sub",
    "num_ins",
    "num_del",
    "edit_pattern",
    "seed",
    "pass_k",
]
PERF_COLUMNS = [
    "method",
    "length",
    "k",
    "max_edit_distance",
    "timing_mode",
    "event",
    "value",
    "unit",
    "note",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", action="store_true")
    parser.add_argument("--length", type=int, default=100)
    parser.add_argument("--max-edit-distance", type=int, default=15)
    parser.add_argument("--k", type=int, default=5)
    parser.add_argument("--pairs", type=int, default=100000)
    parser.add_argument(
        "--methods",
        nargs="+",
        choices=METHODS,
        default=["leap_avx2", "leap_avx512", "lv89", "miniwfa", "wfa2_reuse"],
    )
    parser.add_argument("--timing-mode", choices=["batch", "diff"], default="diff")
    parser.add_argument("--n1", type=int, default=50000)
    parser.add_argument("--n2", type=int, default=100000)
    parser.add_argument("--warmup-passes", type=int, default=1)
    parser.add_argument("--core", type=int)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--run-perf", action="store_true")
    parser.add_argument("--out-dir", type=pathlib.Path, default=pathlib.Path("benchmark/results"))
    return parser.parse_args()


def run_checked(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=REPO_ROOT,
        check=True,
        text=True,
        **kwargs,
    )


def build_benchmark() -> None:
    run_checked(["cmake", "-S", str(BENCHMARK_ROOT), "-B", str(BUILD_DIR), "-DCMAKE_BUILD_TYPE=Release"])
    run_checked(["cmake", "--build", str(BUILD_DIR), "--target", "bench_methods", "-j"])


def random_dna(rng: random.Random, length: int) -> str:
    return "".join(rng.choice(DNA) for _ in range(length))


def different_base(rng: random.Random, base: str) -> str:
    alternatives = [one for one in DNA if one != base]
    return rng.choice(alternatives)


def myers_edit_distance(pattern: str, text: str) -> int:
    """Compute exact Levenshtein distance with Python integer bit vectors."""
    if not pattern:
        return len(text)
    masks: dict[str, int] = {}
    for index, char in enumerate(pattern):
        masks[char] = masks.get(char, 0) | (1 << index)
    full_mask = (1 << len(pattern)) - 1
    high_bit = 1 << (len(pattern) - 1)
    vp = full_mask
    vn = 0
    score = len(pattern)
    for char in text:
        eq = masks.get(char, 0)
        xv = eq | vn
        xh = (((eq & vp) + vp) ^ vp) | eq
        ph = vn | ~(xh | vp)
        mh = vp & xh
        if ph & high_bit:
            score += 1
        if mh & high_bit:
            score -= 1
        ph = ((ph << 1) | 1) & full_mask
        mh = (mh << 1) & full_mask
        vp = (mh | ~(xv | ph)) & full_mask
        vn = ph & xv
    return score


def mutate_equal_length(
    rng: random.Random, reference: str, requested_ed: int, pair_index: int
) -> tuple[str, int, int, int, str]:
    if requested_ed == 0:
        return reference, 0, 0, 0, "exact"

    chars = list(reference)
    use_indel_pair = requested_ed >= 2 and pair_index % 3 != 0
    num_ins = 0
    num_del = 0
    substitutions = requested_ed
    pattern = "substitutions"

    if use_indel_pair:
        num_ins = 1
        num_del = 1
        substitutions -= 2
        delete_at = rng.randrange(len(chars))
        del chars[delete_at]
        insert_at = rng.randrange(len(chars) + 1)
        chars.insert(insert_at, rng.choice(DNA))
        pattern = "insertion_deletion" if substitutions == 0 else "mixed"

    positions = rng.sample(range(len(chars)), substitutions)
    for position in positions:
        chars[position] = different_base(rng, chars[position])
    return "".join(chars), substitutions, num_ins, num_del, pattern


def dataset_path(length: int, max_edit_distance: int, pairs: int) -> pathlib.Path:
    return (
        BENCHMARK_ROOT
        / "data"
        / "generated"
        / f"pairs_len{length}_ed{max_edit_distance}_n{pairs}.tsv"
    )


def generate_dataset(path: pathlib.Path, args: argparse.Namespace) -> None:
    if args.length <= 0 or args.max_edit_distance < 0 or args.pairs <= 0:
        raise SystemExit("dataset parameters must have positive length/pairs and non-negative edit distance")
    path.parent.mkdir(parents=True, exist_ok=True)
    rng = random.Random(args.seed)
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=DATA_COLUMNS, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for pair_index in range(args.pairs):
            target_ed = pair_index % (args.max_edit_distance + 1)
            for _ in range(10000):
                reference = random_dna(rng, args.length)
                read, num_sub, num_ins, num_del, pattern = mutate_equal_length(
                    rng, reference, target_ed, pair_index
                )
                if target_ed == 2 and sum(a != b for a, b in zip(read, reference)) <= 1:
                    continue
                # Equal-length accepted edits at distance <= 2 cannot be cheaper:
                # ED 0 is equality, ED 1 changes one base, and ED 2 differs
                # by more than one substitution. Larger mixed edits are verified.
                true_ed = target_ed if target_ed <= 2 else myers_edit_distance(read, reference)
                if true_ed == target_ed:
                    break
            else:
                raise RuntimeError(f"could not synthesize exact edit distance {target_ed}")
            writer.writerow(
                {
                    "pair_id": f"pair_{pair_index}",
                    "length": args.length,
                    "true_ed": true_ed,
                    "read": read,
                    "reference": reference,
                    "num_sub": num_sub,
                    "num_ins": num_ins,
                    "num_del": num_del,
                    "edit_pattern": pattern,
                    "seed": args.seed,
                    "pass_k": int(true_ed <= args.k),
                }
            )


def dataset_has_rows(path: pathlib.Path, expected_pairs: int) -> bool:
    if not path.exists():
        return False
    with path.open() as handle:
        return max(0, sum(1 for _ in handle) - 1) == expected_pairs


def ensure_dataset(args: argparse.Namespace) -> pathlib.Path:
    path = dataset_path(args.length, args.max_edit_distance, args.pairs)
    if not dataset_has_rows(path, args.pairs):
        print(f"generating {path.relative_to(REPO_ROOT)}", file=sys.stderr)
        generate_dataset(path, args)
    return path


def ensure_tsv(path: pathlib.Path, columns: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        with path.open("w", newline="") as handle:
            csv.DictWriter(handle, fieldnames=columns, delimiter="\t", lineterminator="\n").writeheader()


def append_rows(path: pathlib.Path, columns: list[str], rows: Iterable[dict[str, object]]) -> None:
    ensure_tsv(path, columns)
    with path.open("a", newline="") as handle:
        writer = csv.DictWriter(
            handle, fieldnames=columns, delimiter="\t", lineterminator="\n", extrasaction="ignore"
        )
        for row in rows:
            writer.writerow(row)


def base_binary_command(args: argparse.Namespace, method: str, data_path: pathlib.Path) -> list[str]:
    command = [
        str(BINARY),
        "--dataset",
        str(data_path),
        "--method",
        method,
        "--length",
        str(args.length),
        "--k",
        str(args.k),
        "--pairs",
        str(args.pairs),
        "--timing-mode",
        args.timing_mode,
        "--n1",
        str(args.n1),
        "--n2",
        str(args.n2),
        "--warmup-passes",
        str(args.warmup_passes),
        "--seed",
        str(args.seed),
    ]
    if args.core is not None:
        return ["taskset", "-c", str(args.core), *command]
    return command


def parse_bench_stdout(stdout: str, args: argparse.Namespace) -> dict[str, str]:
    rows = list(csv.DictReader(stdout.splitlines(), delimiter="\t"))
    if len(rows) != 1:
        raise RuntimeError(f"bench_methods returned {len(rows)} data rows\n{stdout}")
    row = rows[0]
    row["max_edit_distance"] = str(args.max_edit_distance)
    return row


def parse_perf(stderr: str, args: argparse.Namespace, method: str) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for line in stderr.splitlines():
        fields = line.split("\t")
        if len(fields) < 3 or not fields[2].strip():
            continue
        value = fields[0].strip()
        event = fields[2].strip()
        if value.startswith("<"):
            note = value
        else:
            note = ""
        rows.append(
            {
                "method": method,
                "length": str(args.length),
                "k": str(args.k),
                "max_edit_distance": str(args.max_edit_distance),
                "timing_mode": args.timing_mode,
                "event": event,
                "value": value,
                "unit": fields[1].strip(),
                "note": note,
            }
        )
    return rows


def run_method(args: argparse.Namespace, method: str, data_path: pathlib.Path) -> tuple[dict[str, str], list[dict[str, str]]]:
    command = base_binary_command(args, method, data_path)
    perf_rows: list[dict[str, str]] = []
    if args.run_perf:
        command = [
            "perf",
            "stat",
            "-x",
            "\t",
            "-e",
            "cycles,instructions,branches,branch-misses,cache-references,cache-misses",
            "--",
            *command,
        ]
    completed = subprocess.run(command, cwd=REPO_ROOT, text=True, capture_output=True)
    if completed.returncode:
        raise RuntimeError(
            f"{method} failed with exit {completed.returncode}\n"
            f"command: {' '.join(command)}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    if args.run_perf:
        perf_rows = parse_perf(completed.stderr, args, method)
    return parse_bench_stdout(completed.stdout, args), perf_rows


def read_raw_rows(path: pathlib.Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def chosen_average(row: dict[str, str]) -> str:
    column = "avg_ns_per_pair_diff" if row["timing_mode"] == "diff" else "avg_ns_per_pair_batch"
    try:
        return f"{float(row[column]):.3f}"
    except (KeyError, ValueError):
        return ""


def render_table(rows: list[dict[str, str]]) -> list[str]:
    lines = [
        "| Method | Mode | Boundary | Pairs | ns/pair | total thread CPU ns | Notes |",
        "|---|---|---|---:|---:|---:|---|",
    ]
    for row in rows:
        lines.append(
            "| {method} | {timing_mode} | {measured_boundary} | {pair_count} | {average} | "
            "{total_time_ns} | {notes} |".format(average=chosen_average(row), **row)
        )
    if not rows:
        lines.append("| No rows recorded |  |  |  |  |  |  |")
    return lines


def write_report(raw_path: pathlib.Path, report_path: pathlib.Path) -> None:
    rows = read_raw_rows(raw_path)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Method Timing Benchmark",
        "",
        "LEAP numbers are run-only forward traversal time and are not LEAP end-to-end time.",
        "",
        "Non-LEAP numbers are warmed full-workflow timing for the selected method.",
        "",
        "The N1/N2 difference estimate uses t = (T2 - T1) / (N2 - N1) to reduce fixed benchmark overhead.",
        "",
        "LEAP AVX2 and AVX512 timing includes only `ed.run()` over prepared chunk objects. Non-LEAP timing "
        "includes the warmed method workflow needed to produce score/pass, with CIGAR and traceback disabled.",
        "",
        "Dataset loading, TSV parsing, synthetic generation, ground-truth computation, output writing, report "
        "generation, Python orchestration, and build time are outside timed regions.",
        "",
        "Timer overhead is reduced with batch timing and optionally with the N1/N2 linear-difference timing mode. "
        "The internal timer is `CLOCK_THREAD_CPUTIME_ID`.",
        "",
        "The benchmark does not claim exact pure arithmetic time. The benchmark does not claim exact memory time.",
        "",
        "## Requested 18 Groups",
        "",
    ]
    for length in (100, 200, 500):
        for k in range(6):
            group = [
                row
                for row in rows
                if row.get("length") == str(length)
                and row.get("k") == str(k)
                and row.get("max_edit_distance") == str(k)
            ]
            lines.append(f"### Length {length}, max edit distance k = {k}")
            lines.append("")
            lines.extend(render_table(group))
            lines.append("")
    extra = [
        row
        for row in rows
        if not (
            row.get("length") in {"100", "200", "500"}
            and row.get("k") in {str(k) for k in range(6)}
            and row.get("max_edit_distance") == row.get("k")
        )
    ]
    if extra:
        lines.extend(["## Additional Runs", ""])
        lines.extend(render_table(extra))
        lines.append("")
    report_path.write_text("\n".join(lines), encoding="utf-8")


def command_output(command: list[str]) -> str:
    completed = subprocess.run(command, cwd=REPO_ROOT, text=True, capture_output=True)
    if completed.returncode:
        return f"$ {' '.join(command)}\n{completed.stderr.strip()}\n"
    return f"$ {' '.join(command)}\n{completed.stdout.strip()}\n"


def write_environment(path: pathlib.Path, args: argparse.Namespace) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    text = [
        f"generated_utc: {dt.datetime.now(dt.timezone.utc).isoformat()}",
        f"repo_root: {REPO_ROOT}",
        f"platform: {platform.platform()}",
        f"python: {sys.version.replace(os.linesep, ' ')}",
        f"runner_args: {' '.join(sys.argv[1:])}",
        "",
        command_output(["uname", "-a"]),
        command_output(["lscpu"]),
    ]
    path.write_text("\n".join(text), encoding="utf-8")


def main() -> int:
    args = parse_args()
    args.out_dir = (REPO_ROOT / args.out_dir).resolve() if not args.out_dir.is_absolute() else args.out_dir
    if args.build:
        build_benchmark()
    if not BINARY.exists():
        raise SystemExit(f"missing {BINARY}; run with --build")
    data_path = ensure_dataset(args)
    raw_path = args.out_dir / "raw" / "method_timing_raw.tsv"
    perf_path = args.out_dir / "raw" / "perf_summary.tsv"
    report_path = args.out_dir / "summary" / "report_method_timing.md"
    env_path = args.out_dir / "summary" / "environment.txt"

    all_perf: list[dict[str, str]] = []
    for method in args.methods:
        print(f"running {method}", file=sys.stderr)
        row, perf_rows = run_method(args, method, data_path)
        append_rows(raw_path, RAW_COLUMNS, [row])
        all_perf.extend(perf_rows)
    append_rows(perf_path, PERF_COLUMNS, all_perf)
    write_report(raw_path, report_path)
    write_environment(env_path, args)
    print(raw_path.relative_to(REPO_ROOT))
    print(report_path.relative_to(REPO_ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

