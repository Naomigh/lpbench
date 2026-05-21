#!/usr/bin/env python3
import csv
import subprocess
import time
from pathlib import Path

pairs_values = list(range(1000, 10001, 1000))
fixed_k = 5
root = Path('/home/novellab/projects/NavigaMer')
out_dir = root / 'benchmark/results/sweep/leap_avx512_pairs_k5'
out_dir.mkdir(parents=True, exist_ok=True)
summary_tsv = out_dir / 'leap_avx512_pair_sweep_k5.tsv'
svg_path = out_dir / 'leap_avx512_pair_sweep_k5.svg'

rows = []
for pairs in pairs_values:
    cmd = [
        'python3', 'benchmark/scripts/run_selected_methods.py',
        '--methods', 'leap_avx512',
        '--pairs-per-ed', str(pairs),
        '--ks', str(fixed_k),
        '--true-ed-min', '0',
        '--true-ed-max', '15',
        '--cache-mode', 'warm',
        '--core', '2',
        '--seed', '1',
    ]
    wall_start = time.perf_counter()
    subprocess.run(cmd, cwd=root, check=True)
    wall_s = time.perf_counter() - wall_start

    raw_path = root / 'benchmark/results/raw/threshold_raw.tsv'
    raw_rows = list(csv.DictReader(raw_path.open(), delimiter='\t'))
    method_rows = [r for r in raw_rows if r['method'] == 'leap_avx512' and int(r['k']) == fixed_k]
    if len(method_rows) != 1:
        raise RuntimeError(f'expected one leap_avx512 k={fixed_k} row, got {len(method_rows)}')
    r = method_rows[0]
    rows.append({
        'method': 'leap_avx512',
        'k': fixed_k,
        'pairs_per_ed': pairs,
        'true_ed_groups': 16,
        'pair_count': int(r['pair_count']),
        'total_cpu_ns': int(float(r['total_cpu_ns'])),
        'total_cpu_seconds': int(float(r['total_cpu_ns'])) / 1e9,
        'avg_cpu_ns_per_pair': float(r['avg_cpu_ns_per_pair']),
        'seconds_per_10M_pairs': float(r['seconds_per_10M_pairs']),
        'wall_seconds_runner': wall_s,
        'mismatch_count': int(r['mismatch_count']),
    })

with summary_tsv.open('w', newline='') as fh:
    fieldnames = list(rows[0].keys())
    writer = csv.DictWriter(fh, fieldnames=fieldnames, delimiter='\t')
    writer.writeheader()
    writer.writerows(rows)

w, h = 920, 520
ml, mr, mt, mb = 90, 40, 45, 75
plot_w, plot_h = w - ml - mr, h - mt - mb
xs = [r['pairs_per_ed'] for r in rows]
ys = [r['total_cpu_seconds'] for r in rows]
xmin, xmax = min(xs), max(xs)
ymin, ymax = 0.0, max(ys) * 1.10 if max(ys) > 0 else 1.0

def sx(x):
    return ml + (x - xmin) / (xmax - xmin) * plot_w

def sy(y):
    return mt + (ymax - y) / (ymax - ymin) * plot_h

points = ' '.join(f'{sx(x):.2f},{sy(y):.2f}' for x, y in zip(xs, ys))
svg = []
svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">')
svg.append('<rect width="100%" height="100%" fill="white"/>')
svg.append('<style>text{font-family:Arial,Helvetica,sans-serif;fill:#111} .grid{stroke:#ddd;stroke-width:1} .axis{stroke:#111;stroke-width:1.5} .line{fill:none;stroke:#2563eb;stroke-width:3} .dot{fill:#2563eb}</style>')
svg.append(f'<text x="{w/2}" y="28" text-anchor="middle" font-size="18" font-weight="700">leap_avx512 total CPU time vs pairs-per-ed (k={fixed_k})</text>')
for i in range(6):
    yv = ymin + (ymax - ymin) * i / 5
    y = sy(yv)
    svg.append(f'<line class="grid" x1="{ml}" y1="{y:.2f}" x2="{w-mr}" y2="{y:.2f}"/>')
    svg.append(f'<text x="{ml-10}" y="{y+4:.2f}" text-anchor="end" font-size="12">{yv:.4f}</text>')
for x in xs:
    px = sx(x)
    svg.append(f'<line class="grid" x1="{px:.2f}" y1="{mt}" x2="{px:.2f}" y2="{h-mb}"/>')
    svg.append(f'<text x="{px:.2f}" y="{h-mb+22}" text-anchor="middle" font-size="12">{x}</text>')
svg.append(f'<line class="axis" x1="{ml}" y1="{h-mb}" x2="{w-mr}" y2="{h-mb}"/>')
svg.append(f'<line class="axis" x1="{ml}" y1="{mt}" x2="{ml}" y2="{h-mb}"/>')
svg.append(f'<polyline class="line" points="{points}"/>')
for x, y in zip(xs, ys):
    px, py = sx(x), sy(y)
    svg.append(f'<circle class="dot" cx="{px:.2f}" cy="{py:.2f}" r="4"/>')
    svg.append(f'<text x="{px:.2f}" y="{py-10:.2f}" text-anchor="middle" font-size="11">{y:.4f}s</text>')
svg.append(f'<text x="{w/2}" y="{h-24}" text-anchor="middle" font-size="14">pairs per true_ed group</text>')
svg.append(f'<text x="20" y="{h/2}" transform="rotate(-90 20 {h/2})" text-anchor="middle" font-size="14">total_cpu_ns for k=5 (seconds)</text>')
svg.append('</svg>')
svg_path.write_text('\n'.join(svg))

print(summary_tsv)
print(svg_path)
