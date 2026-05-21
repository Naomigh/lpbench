#!/usr/bin/env python3
import argparse
import csv
import html
import math
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path


ALL_METHODS = ["leap_avx2", "leap_avx512", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"]
COLORS = {
    "leap_avx2": "#2563eb",
    "leap_avx512": "#dc2626",
    "lv89": "#059669",
    "miniwfa": "#7c3aed",
    "wfa2_fresh": "#ea580c",
    "wfa2_reuse": "#0891b2",
}


def run(cmd, cwd, dry_run=False):
    print("+ " + " ".join(map(str, cmd)), flush=True)
    if not dry_run:
        subprocess.run(list(map(str, cmd)), cwd=cwd, check=True)


def dataset_has_expected_rows(path, expected_rows):
    if not path.exists():
        return False
    with path.open() as fh:
        return max(0, sum(1 for _ in fh) - 1) == expected_rows


def write_tsv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)


def fmt_float(value, digits=3):
    return f"{float(value):.{digits}f}"


def make_markdown_table(headers, rows):
    out = ["| " + " | ".join(headers) + " |"]
    out.append("| " + " | ".join(["---"] * len(headers)) + " |")
    for row in rows:
        out.append("| " + " | ".join(str(v) for v in row) + " |")
    return "\n".join(out)


def svg_polyline(points):
    return " ".join(f"{x:.2f},{y:.2f}" for x, y in points)


def nice_ticks(ymax):
    if ymax <= 0:
        return [0, 1]
    raw = ymax / 4
    power = 10 ** math.floor(math.log10(raw))
    step = math.ceil(raw / power) * power
    return [i * step for i in range(0, int(math.ceil(ymax / step)) + 1)]


def draw_svg(path, rows, lengths, max_eds, methods):
    w, h = 1280, 760
    ml, mr, mt, mb = 72, 190, 58, 70
    gap = 42
    panel_w = (w - ml - mr - gap * (len(lengths) - 1)) / len(lengths)
    panel_h = h - mt - mb
    ymax = max(float(r["avg_cpu_ns_per_pair"]) for r in rows) * 1.08
    ticks = nice_ticks(ymax)
    ymax = ticks[-1]

    def sy(y):
        return mt + (ymax - y) / ymax * panel_h

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">')
    svg.append('<rect width="100%" height="100%" fill="white"/>')
    svg.append(
        '<style>'
        'text{font-family:Arial,Helvetica,sans-serif;fill:#111}'
        '.grid{stroke:#e5e7eb;stroke-width:1}.axis{stroke:#111;stroke-width:1.3}'
        '.line{fill:none;stroke-width:2.4}.dot{stroke:white;stroke-width:1}'
        '</style>'
    )
    svg.append('<text x="640" y="30" text-anchor="middle" font-size="19" font-weight="700">Large matrix average CPU time per pair</text>')

    row_by_key = {(int(r["length"]), int(r["max_edit_distance"]), r["method"]): r for r in rows}
    xmin, xmax = min(max_eds), max(max_eds)

    for idx, length in enumerate(lengths):
        x0 = ml + idx * (panel_w + gap)
        x1 = x0 + panel_w
        svg.append(f'<text x="{(x0+x1)/2:.2f}" y="{mt-18}" text-anchor="middle" font-size="15" font-weight="700">{length} bp</text>')
        for tick in ticks:
            y = sy(tick)
            svg.append(f'<line class="grid" x1="{x0:.2f}" y1="{y:.2f}" x2="{x1:.2f}" y2="{y:.2f}"/>')
            if idx == 0:
                svg.append(f'<text x="{x0-10:.2f}" y="{y+4:.2f}" text-anchor="end" font-size="11">{tick:.0f}</text>')
        for ed in max_eds:
            px = x0 + (ed - xmin) / (xmax - xmin) * panel_w if xmax != xmin else (x0 + x1) / 2
            svg.append(f'<line class="grid" x1="{px:.2f}" y1="{mt}" x2="{px:.2f}" y2="{mt+panel_h}"/>')
            svg.append(f'<text x="{px:.2f}" y="{mt+panel_h+22}" text-anchor="middle" font-size="11">{ed}</text>')
        svg.append(f'<line class="axis" x1="{x0:.2f}" y1="{mt+panel_h:.2f}" x2="{x1:.2f}" y2="{mt+panel_h:.2f}"/>')
        svg.append(f'<line class="axis" x1="{x0:.2f}" y1="{mt:.2f}" x2="{x0:.2f}" y2="{mt+panel_h:.2f}"/>')
        for method in methods:
            pts = []
            for ed in max_eds:
                r = row_by_key[(length, ed, method)]
                px = x0 + (ed - xmin) / (xmax - xmin) * panel_w if xmax != xmin else (x0 + x1) / 2
                pts.append((px, sy(float(r["avg_cpu_ns_per_pair"]))))
            color = COLORS.get(method, "#111827")
            svg.append(f'<polyline class="line" stroke="{color}" points="{svg_polyline(pts)}"/>')
            for px, py in pts:
                svg.append(f'<circle class="dot" fill="{color}" cx="{px:.2f}" cy="{py:.2f}" r="3.4"/>')

    legend_x = w - mr + 22
    for idx, method in enumerate(methods):
        y = mt + 24 + idx * 28
        color = COLORS.get(method, "#111827")
        svg.append(f'<line x1="{legend_x}" y1="{y}" x2="{legend_x+34}" y2="{y}" stroke="{color}" stroke-width="3"/>')
        svg.append(f'<text x="{legend_x+44}" y="{y+4}" font-size="12">{html.escape(method)}</text>')
    svg.append(f'<text x="{(w-mr+ml)/2:.2f}" y="{h-24}" text-anchor="middle" font-size="14">maximum edit distance; k = maximum edit distance</text>')
    svg.append(f'<text x="20" y="{h/2}" transform="rotate(-90 20 {h/2})" text-anchor="middle" font-size="14">avg CPU ns / pair</text>')
    svg.append('</svg>')
    path.write_text("\n".join(svg))


def write_report(path, rows, lengths, max_eds, methods, args, svg_path):
    pivot = {(int(r["length"]), int(r["max_edit_distance"]), r["method"]): r for r in rows}
    headers = ["length_bp", "max_edit_distance", "k", "pair_count", "pairs_per_ed"] + methods
    table_rows = []
    for length in lengths:
        for max_ed in max_eds:
            first = pivot[(length, max_ed, methods[0])]
            table_rows.append(
                [
                    length,
                    max_ed,
                    max_ed,
                    first["pair_count"],
                    first["pairs_per_ed"],
                    *[fmt_float(pivot[(length, max_ed, method)]["avg_cpu_ns_per_pair"]) for method in methods],
                ]
            )

    lines = []
    lines.append("# Large Matrix Benchmark")
    lines.append("")
    lines.append(f"- Generated at: {datetime.now().isoformat(timespec='seconds')}")
    lines.append(f"- Lengths: {', '.join(map(str, lengths))} bp")
    lines.append(f"- Maximum edit distances: {', '.join(map(str, max_eds))}")
    lines.append("- k: equal to maximum edit distance for each group")
    lines.append(f"- Methods: {', '.join(methods)}")
    lines.append(f"- Cache mode: {args.cache_mode}")
    if args.pairs_per_ed is None:
        lines.append(f"- Target pairs per group: at least {args.target_pairs}; actual count is rounded up to whole true_ed groups")
    else:
        lines.append(f"- Pairs per true_ed group: {args.pairs_per_ed}")
    lines.append("")
    lines.append("LEAP timing note: `leap_avx2` and `leap_avx512` rows come from `mode=forward`; setup (`init_levenshtein`, `load_reads`, `calculate_masks`, `reset`) is outside the timed block, and the timed block wraps only `ed.run()`.")
    lines.append("")
    lines.append("## Average CPU Time")
    lines.append("")
    lines.append("Values are average CPU nanoseconds per pair.")
    lines.append("")
    lines.append(make_markdown_table(headers, table_rows))
    lines.append("")
    lines.append("## Figure")
    lines.append("")
    lines.append(f"![Large matrix average CPU time]({svg_path.name})")
    path.write_text("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser(description="Run the 18-case large benchmark matrix and write one Markdown summary.")
    parser.add_argument("--lengths", nargs="+", type=int, default=[100, 200, 500])
    parser.add_argument("--max-edit-distances", "--max-eds", nargs="+", type=int, default=[0, 1, 2, 3, 4, 5])
    parser.add_argument("--target-pairs", type=int, default=1_000_000, help="minimum selected pairs per length/max-ed case; rounded up by true_ed group count")
    parser.add_argument("--pairs-per-ed", type=int, default=None, help="override --target-pairs with a fixed count per true_ed group")
    parser.add_argument("--methods", nargs="+", default=ALL_METHODS)
    parser.add_argument("--cache-mode", choices=["warm", "cold-ish"], default="warm")
    parser.add_argument("--core", type=int, default=2)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out-dir", default="benchmark/results/large_matrix")
    parser.add_argument("--build-dir", default="benchmark/build")
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--force-regenerate", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = root / out_dir
    out_dir.mkdir(parents=True, exist_ok=True)
    summary_dir = out_dir / "summary"
    summary_dir.mkdir(parents=True, exist_ok=True)

    unknown = [m for m in args.methods if m not in ALL_METHODS]
    if unknown:
        raise SystemExit(f"Unknown method(s): {' '.join(unknown)}")

    if not args.no_build:
        run(["python3", "benchmark/scripts/build_benchmark.py", "--build-dir", args.build_dir], cwd=root, dry_run=args.dry_run)
    run(["sh", "benchmark/scripts/record_environment.sh", summary_dir / "environment.txt"], cwd=root, dry_run=args.dry_run)

    all_rows = []
    for length in args.lengths:
        for max_ed in args.max_edit_distances:
            if max_ed < 0:
                raise SystemExit("maximum edit distance must be non-negative")
            true_ed_groups = max_ed + 1
            pairs_per_ed = args.pairs_per_ed
            if pairs_per_ed is None:
                pairs_per_ed = math.ceil(args.target_pairs / true_ed_groups)
            expected_rows = pairs_per_ed * true_ed_groups
            case_tag = f"len{length}_k{max_ed}_ed0_{max_ed}_{expected_rows}pairs"
            case_out_dir = out_dir / case_tag
            bench_out_dir = case_out_dir / "benchmark"
            bench_out_dir.mkdir(parents=True, exist_ok=True)
            dataset = root / f"benchmark/data/generated/pairs_len{length}_ed0_{max_ed}_{expected_rows}pairs_seed{args.seed}.tsv"

            if args.force_regenerate or not dataset_has_expected_rows(dataset, expected_rows):
                run(
                    [
                        "python3",
                        "benchmark/scripts/generate_dataset.py",
                        "--length",
                        length,
                        "--true-ed-min",
                        0,
                        "--true-ed-max",
                        max_ed,
                        "--pairs-per-ed",
                        pairs_per_ed,
                        "--seed",
                        args.seed,
                        "--out",
                        dataset,
                        "--build-dir",
                        args.build_dir,
                    ],
                    cwd=root,
                    dry_run=args.dry_run,
                )

            cmd = [
                Path(args.build_dir) / "threshold_bench",
                "--dataset",
                dataset,
                "--lengths",
                length,
                "--ks",
                max_ed,
                "--true-ed-min",
                0,
                "--true-ed-max",
                max_ed,
                "--methods",
                *args.methods,
                "--cache-mode",
                args.cache_mode,
                "--seed",
                args.seed,
                "--out-dir",
                bench_out_dir,
            ]
            if args.core is not None:
                cmd = ["taskset", "-c", args.core, *cmd]
            wall_start = time.perf_counter()
            run(cmd, cwd=root, dry_run=args.dry_run)
            wall_seconds = 0.0 if args.dry_run else time.perf_counter() - wall_start
            if args.dry_run:
                continue

            raw_rows = list(csv.DictReader((bench_out_dir / "raw" / "threshold_raw.tsv").open(), delimiter="\t"))
            seen = set()
            for row in raw_rows:
                if row["method"] not in args.methods or int(row["k"]) != max_ed:
                    continue
                if row["method"].startswith("leap_") and row["mode"] != "forward":
                    raise RuntimeError(f"{row['method']} did not report forward mode")
                seen.add(row["method"])
                total_cpu_ns = int(float(row["total_cpu_ns"]))
                pair_count = int(row["pair_count"])
                avg_cpu_ns = float(row["avg_cpu_ns_per_pair"])
                all_rows.append(
                    {
                        "length": length,
                        "max_edit_distance": max_ed,
                        "k": max_ed,
                        "method": row["method"],
                        "backend": row["backend"],
                        "mode": row["mode"],
                        "cache_mode": args.cache_mode,
                        "pair_count": pair_count,
                        "pairs_per_ed": pairs_per_ed,
                        "true_ed_groups": true_ed_groups,
                        "total_cpu_ns": total_cpu_ns,
                        "total_cpu_seconds": total_cpu_ns / 1e9,
                        "avg_cpu_ns_per_pair": avg_cpu_ns,
                        "pairs_per_second_cpu": 1e9 / avg_cpu_ns if avg_cpu_ns > 0 else 0.0,
                        "seconds_per_10M_pairs": float(row["seconds_per_10M_pairs"]),
                        "pass_count": int(row["pass_count"]),
                        "fail_count": int(row["fail_count"]),
                        "mismatch_count": int(row["mismatch_count"]),
                        "wall_seconds_threshold_bench": wall_seconds,
                        "dataset": str(dataset),
                    }
                )
            missing = [m for m in args.methods if m not in seen]
            if missing:
                raise RuntimeError(f"missing method rows for length={length}, max_ed={max_ed}: {' '.join(missing)}")

    if args.dry_run:
        return 0

    all_rows.sort(key=lambda r: (int(r["length"]), int(r["max_edit_distance"]), args.methods.index(r["method"])))
    raw_tsv = out_dir / "large_matrix_raw.tsv"
    svg_path = out_dir / "large_matrix_avg_cpu_ns.svg"
    report_path = out_dir / "large_matrix_report.md"
    write_tsv(raw_tsv, all_rows)
    draw_svg(svg_path, all_rows, args.lengths, args.max_edit_distances, args.methods)
    write_report(report_path, all_rows, args.lengths, args.max_edit_distances, args.methods, args, svg_path)

    print(raw_tsv)
    print(report_path)
    print(svg_path)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"command failed with exit code {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode)
