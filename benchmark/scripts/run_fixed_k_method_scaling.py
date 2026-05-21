#!/usr/bin/env python3
import argparse
import csv
import math
import subprocess
import sys
import time
from pathlib import Path

ALL_METHODS = ["leap_avx2", "leap_avx512", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"]
PRESETS = {
    "all": ALL_METHODS,
    "no-avx512": ["leap_avx2", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"],
    "leap": ["leap_avx2", "leap_avx512"],
    "non-leap": ["lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"],
    "wfa": ["miniwfa", "wfa2_fresh", "wfa2_reuse"],
    "wfa2": ["wfa2_fresh", "wfa2_reuse"],
    "baseline": ["lv89", "miniwfa", "wfa2_reuse"],
}
COLORS = {
    "leap_avx2": "#2563eb",
    "leap_avx512": "#dc2626",
    "lv89": "#059669",
    "miniwfa": "#7c3aed",
    "wfa2_fresh": "#ea580c",
    "wfa2_reuse": "#0891b2",
}


def ordered_unique(items):
    seen = set()
    out = []
    for item in items:
        if item not in seen:
            out.append(item)
            seen.add(item)
    return out


def validate_methods(methods):
    bad = [m for m in methods if m not in ALL_METHODS]
    if bad:
        raise SystemExit(f"Unknown method(s): {' '.join(bad)}\nValid methods: {' '.join(ALL_METHODS)}")


def choose_methods(args):
    methods = args.methods if args.methods else PRESETS[args.preset]
    validate_methods(methods)
    if args.exclude:
        validate_methods(args.exclude)
        excluded = set(args.exclude)
        methods = [m for m in methods if m not in excluded]
    methods = ordered_unique(methods)
    if not methods:
        raise SystemExit("No methods selected after applying --exclude")
    return methods


def run(cmd, cwd, dry_run=False):
    print("+ " + " ".join(map(str, cmd)), flush=True)
    if dry_run:
        return
    subprocess.run(list(map(str, cmd)), cwd=cwd, check=True)


def linear_fit(xs, ys):
    n = len(xs)
    mean_x = sum(xs) / n
    mean_y = sum(ys) / n
    sxx = sum((x - mean_x) ** 2 for x in xs)
    if sxx == 0:
        return 0.0, mean_y, 0.0
    sxy = sum((x - mean_x) * (y - mean_y) for x, y in zip(xs, ys))
    slope = sxy / sxx
    intercept = mean_y - slope * mean_x
    ss_tot = sum((y - mean_y) ** 2 for y in ys)
    ss_res = sum((y - (slope * x + intercept)) ** 2 for x, y in zip(xs, ys))
    r2 = 1.0 if ss_tot == 0 else 1.0 - ss_res / ss_tot
    return slope, intercept, r2


def write_tsv(path, rows, fieldnames):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)


def svg_polyline(points):
    return " ".join(f"{x:.2f},{y:.2f}" for x, y in points)


def draw_svg(path, rows, fit_rows, methods, k, y_field, title, y_label):
    w, h = 1120, 640
    ml, mr, mt, mb = 105, 210, 50, 80
    plot_w, plot_h = w - ml - mr, h - mt - mb
    xs = sorted({int(r["pairs_per_ed"]) for r in rows})
    ys = [float(r[y_field]) for r in rows]
    xmin, xmax = min(xs), max(xs)
    ymin, ymax = 0.0, max(ys) * 1.10 if ys and max(ys) > 0 else 1.0

    def sx(x):
        if xmax == xmin:
            return ml + plot_w / 2
        return ml + (x - xmin) / (xmax - xmin) * plot_w

    def sy(y):
        return mt + (ymax - y) / (ymax - ymin) * plot_h

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">')
    svg.append('<rect width="100%" height="100%" fill="white"/>')
    svg.append('<style>text{font-family:Arial,Helvetica,sans-serif;fill:#111}.grid{stroke:#ddd;stroke-width:1}.axis{stroke:#111;stroke-width:1.5}.line{fill:none;stroke-width:2.5}.fit{fill:none;stroke-width:1.8;stroke-dasharray:6 4}.dot{stroke:white;stroke-width:1}</style>')
    svg.append(f'<text x="{w/2}" y="30" text-anchor="middle" font-size="18" font-weight="700">{title}</text>')

    for i in range(6):
        yv = ymin + (ymax - ymin) * i / 5
        y = sy(yv)
        svg.append(f'<line class="grid" x1="{ml}" y1="{y:.2f}" x2="{w-mr}" y2="{y:.2f}"/>')
        svg.append(f'<text x="{ml-10}" y="{y+4:.2f}" text-anchor="end" font-size="12">{yv:.5f}</text>')
    for x in xs:
        px = sx(x)
        svg.append(f'<line class="grid" x1="{px:.2f}" y1="{mt}" x2="{px:.2f}" y2="{h-mb}"/>')
        svg.append(f'<text x="{px:.2f}" y="{h-mb+22}" text-anchor="middle" font-size="12">{x}</text>')

    svg.append(f'<line class="axis" x1="{ml}" y1="{h-mb}" x2="{w-mr}" y2="{h-mb}"/>')
    svg.append(f'<line class="axis" x1="{ml}" y1="{mt}" x2="{ml}" y2="{h-mb}"/>')

    fit_by_method = {r["method"]: r for r in fit_rows}
    legend_y = mt + 10
    for idx, method in enumerate(methods):
        color = COLORS.get(method, "#111827")
        m_rows = [r for r in rows if r["method"] == method]
        m_rows.sort(key=lambda r: int(r["pairs_per_ed"]))
        measured_points = [(sx(int(r["pairs_per_ed"])), sy(float(r[y_field]))) for r in m_rows]
        svg.append(f'<polyline class="line" stroke="{color}" points="{svg_polyline(measured_points)}"/>')
        for px, py in measured_points:
            svg.append(f'<circle class="dot" fill="{color}" cx="{px:.2f}" cy="{py:.2f}" r="3.5"/>')
        # Draw the fitted line after intercept removal. For adjusted plots this is slope*x.
        fit = fit_by_method[method]
        slope = float(fit["slope_seconds_per_pair_count"])
        fit_points = [(sx(x), sy(slope * (x * int(fit["true_ed_groups"])))) for x in (xmin, xmax)]
        svg.append(f'<polyline class="fit" stroke="{color}" points="{svg_polyline(fit_points)}"/>')
        ly = legend_y + idx * 28
        svg.append(f'<line x1="{w-mr+20}" y1="{ly}" x2="{w-mr+52}" y2="{ly}" stroke="{color}" stroke-width="3"/>')
        svg.append(f'<text x="{w-mr+60}" y="{ly+4}" font-size="12">{method}</text>')

    svg.append(f'<text x="{w/2}" y="{h-26}" text-anchor="middle" font-size="14">pairs per true_ed group, k={k}</text>')
    svg.append(f'<text x="20" y="{h/2}" transform="rotate(-90 20 {h/2})" text-anchor="middle" font-size="14">{y_label}</text>')
    svg.append('</svg>')
    path.write_text("\n".join(svg))


def main():
    parser = argparse.ArgumentParser(description="Fixed-k method scaling sweep with intercept-subtracted plots.")
    parser.add_argument("--k", type=int, default=5)
    parser.add_argument("--pairs-start", type=int, default=1000)
    parser.add_argument("--pairs-stop", type=int, default=10000)
    parser.add_argument("--pairs-step", type=int, default=1000)
    parser.add_argument("--preset", choices=sorted(PRESETS), default="all")
    parser.add_argument("--methods", nargs="+")
    parser.add_argument("--exclude", nargs="+", default=[])
    parser.add_argument("--length", type=int, default=100)
    parser.add_argument("--true-ed-min", "--min-ed", "--min-edit-distance", dest="true_ed_min", type=int, default=0)
    parser.add_argument("--true-ed-max", "--max-ed", "--max-edit-distance", dest="true_ed_max", type=int, default=15)
    parser.add_argument("--cache-mode", choices=["warm", "cold-ish"], default="warm")
    parser.add_argument("--core", type=int, default=2)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--benchmark-out-dir", default="benchmark/results")
    parser.add_argument("--build-dir", default="benchmark/build")
    parser.add_argument("--sweep-out-dir", default=None)
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    methods = choose_methods(args)
    pairs_values = list(range(args.pairs_start, args.pairs_stop + 1, args.pairs_step))
    if not pairs_values:
        raise SystemExit("empty pairs sweep")
    true_ed_groups = args.true_ed_max - args.true_ed_min + 1
    sweep_dir = Path(args.sweep_out_dir) if args.sweep_out_dir else root / f"benchmark/results/sweep/fixed_k{k_safe(args.k)}_ed{args.true_ed_min}_{args.true_ed_max}_{method_tag(methods)}"
    if not sweep_dir.is_absolute():
        sweep_dir = root / sweep_dir
    sweep_dir.mkdir(parents=True, exist_ok=True)

    raw_rows = []
    first_build = not args.no_build
    for pairs in pairs_values:
        cmd = [
            "python3", "benchmark/scripts/run_selected_methods.py",
            "--methods", *methods,
            "--pairs-per-ed", str(pairs),
            "--ks", str(args.k),
            "--lengths", str(args.length),
            "--true-ed-min", str(args.true_ed_min),
            "--true-ed-max", str(args.true_ed_max),
            "--cache-mode", args.cache_mode,
            "--core", str(args.core),
            "--seed", str(args.seed),
            "--out-dir", args.benchmark_out_dir,
            "--build-dir", args.build_dir,
        ]
        if not first_build:
            cmd.append("--no-build")
        wall_start = time.perf_counter()
        run(cmd, cwd=root, dry_run=args.dry_run)
        wall_seconds = 0.0 if args.dry_run else time.perf_counter() - wall_start
        first_build = False
        if args.dry_run:
            continue

        threshold_raw = root / args.benchmark_out_dir / "raw" / "threshold_raw.tsv"
        rows = list(csv.DictReader(threshold_raw.open(), delimiter="\t"))
        expected = {m: False for m in methods}
        for row in rows:
            if row["method"] not in methods or int(row["k"]) != args.k:
                continue
            expected[row["method"]] = True
            total_cpu_ns = int(float(row["total_cpu_ns"]))
            pair_count = int(row["pair_count"])
            raw_rows.append({
                "method": row["method"],
                "k": args.k,
                "pairs_per_ed": pairs,
                "true_ed_groups": true_ed_groups,
                "pair_count": pair_count,
                "total_cpu_ns": total_cpu_ns,
                "total_cpu_seconds": total_cpu_ns / 1e9,
                "avg_cpu_ns_per_pair": float(row["avg_cpu_ns_per_pair"]),
                "seconds_per_10M_pairs": float(row["seconds_per_10M_pairs"]),
                "wall_seconds_runner": wall_seconds,
                "mismatch_count": int(row["mismatch_count"]),
            })
        missing = [m for m, seen in expected.items() if not seen]
        if missing:
            raise RuntimeError(f"missing method rows for pairs_per_ed={pairs}: {' '.join(missing)}")

    if args.dry_run:
        return 0

    fit_rows = []
    adjusted_rows = []
    for method in methods:
        m_rows = [r for r in raw_rows if r["method"] == method]
        m_rows.sort(key=lambda r: int(r["pairs_per_ed"]))
        xs = [float(r["pair_count"]) for r in m_rows]
        ys = [float(r["total_cpu_seconds"]) for r in m_rows]
        slope, intercept, r2 = linear_fit(xs, ys)
        fit_rows.append({
            "method": method,
            "k": args.k,
            "true_ed_groups": true_ed_groups,
            "slope_seconds_per_pair_count": slope,
            "slope_ns_per_pair": slope * 1e9,
            "intercept_seconds": intercept,
            "r2": r2,
            "point_count": len(m_rows),
            "mismatch_count_sum": sum(int(r["mismatch_count"]) for r in m_rows),
        })
        for r in m_rows:
            adjusted = float(r["total_cpu_seconds"]) - intercept
            fitted_adjusted = slope * float(r["pair_count"])
            out = dict(r)
            out["fit_intercept_seconds"] = intercept
            out["fit_slope_seconds_per_pair_count"] = slope
            out["intercept_subtracted_seconds"] = adjusted
            out["fitted_intercept_subtracted_seconds"] = fitted_adjusted
            adjusted_rows.append(out)

    raw_tsv = sweep_dir / f"fixed_k{args.k}_method_scaling_raw.tsv"
    fit_tsv = sweep_dir / f"fixed_k{args.k}_method_scaling_fit.tsv"
    adjusted_tsv = sweep_dir / f"fixed_k{args.k}_method_scaling_intercept_subtracted.tsv"
    adjusted_svg = sweep_dir / f"fixed_k{args.k}_method_scaling_intercept_subtracted.svg"

    write_tsv(raw_tsv, raw_rows, list(raw_rows[0].keys()))
    write_tsv(fit_tsv, fit_rows, list(fit_rows[0].keys()))
    write_tsv(adjusted_tsv, adjusted_rows, list(adjusted_rows[0].keys()))
    draw_svg(
        adjusted_svg,
        adjusted_rows,
        fit_rows,
        methods,
        args.k,
        "intercept_subtracted_seconds",
        f"Fixed k={args.k}: total CPU time after subtracting fitted intercept",
        "total_cpu_seconds - fitted intercept",
    )

    print(raw_tsv)
    print(fit_tsv)
    print(adjusted_tsv)
    print(adjusted_svg)
    return 0


def k_safe(k):
    return str(k).replace("-", "m")


def method_tag(methods):
    if methods == ALL_METHODS:
        return "all_methods"
    if len(methods) <= 3:
        return "_".join(methods)
    return f"{len(methods)}_methods"


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"command failed with exit code {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode)
