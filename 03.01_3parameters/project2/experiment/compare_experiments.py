#!/usr/bin/env python3
"""
compare_experiments.py
4実験の収束域を比較して表を出力する。

実験 | 方向 | 真値 | 参照画像真値  | 収束域幅
C   | ω₁  | 15° | ω₂=30°     | XX°
D   | ω₂  | 30° | ω₁=15°     | XX°
A   | ω₁  | 15° | ω₂=15°     | XX°
B   | ω₂  | 15° | ω₁=15°     | XX°
"""

import csv
import math
import os

FROB_THRESHOLD = 0.005

EXPERIMENTS = [
    {
        'label':    'C',
        'csv':      'results/scan_C_omega1_true15_ref30.csv',
        'dir':      'ω₁',
        'true_deg': 15.0,
        'ref_info': 'ω₂=30°',
        'est_col':  'final_omega1_deg',
    },
    {
        'label':    'D',
        'csv':      'results/scan_D_omega2_true30_ref30.csv',
        'dir':      'ω₂',
        'true_deg': 30.0,
        'ref_info': 'ω₁=15°',
        'est_col':  'final_omega2_deg',
    },
    {
        'label':    'A',
        'csv':      'results/scan_A_omega1_true15_ref15.csv',
        'dir':      'ω₁',
        'true_deg': 15.0,
        'ref_info': 'ω₂=15°',
        'est_col':  'final_omega1_deg',
    },
    {
        'label':    'B',
        'csv':      'results/scan_B_omega2_true15_ref15.csv',
        'dir':      'ω₂',
        'true_deg': 15.0,
        'ref_info': 'ω₁=15°',
        'est_col':  'final_omega2_deg',
    },
]


def load_results(csv_path, est_col, true_deg):
    rows = []
    if not os.path.exists(csv_path):
        return None
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            frob = float(row['frob_err']) if row['frob_err'] != 'nan' else float('nan')
            conv = row['converged'] == 'True'
            conv_true = conv and (not math.isnan(frob)) and frob < FROB_THRESHOLD
            rows.append({
                'initial_deg':  float(row['initial_deg']),
                'converged':    conv,
                'conv_true':    conv_true,
                'frob_err':     frob,
            })
    return sorted(rows, key=lambda r: r['initial_deg'])


def convergence_width(rows):
    degs = [r['initial_deg'] for r in rows if r['conv_true']]
    if not degs:
        return 0.0, None, None
    return round(max(degs) - min(degs), 1), min(degs), max(degs)


def main():
    print("\n" + "=" * 70)
    print("4実験 収束域比較表")
    print("=" * 70)
    header = f"{'実験':<4} {'方向':<6} {'真値':<8} {'参照画像真値':<14} {'収束域':<20} {'幅[°]':<8}"
    print(header)
    print("-" * 70)

    for exp in EXPERIMENTS:
        rows = load_results(exp['csv'], exp['est_col'], exp['true_deg'])
        if rows is None:
            print(f"  {exp['label']}   {exp['dir']:<6} {exp['true_deg']:.0f}°     "
                  f"{exp['ref_info']:<14} {'(CSVなし)':<20} {'--'}")
            continue

        width, lo, hi = convergence_width(rows)
        n_total = len(rows)
        n_true  = sum(1 for r in rows if r['conv_true'])

        if lo is None:
            range_str = "収束なし"
            width_str = "0.0"
        else:
            range_str = f"{lo:.1f}° 〜 {hi:.1f}°"
            width_str = f"{width:.1f}"

        print(f"  {exp['label']}   {exp['dir']:<6} {exp['true_deg']:.0f}°"
              f"     {exp['ref_info']:<14} {range_str:<20} {width_str}  ({n_true}/{n_total}点)")

    print("=" * 70)
    print(f"判定基準: Frobenius誤差 < {FROB_THRESHOLD}")
    print()


if __name__ == '__main__':
    main()
