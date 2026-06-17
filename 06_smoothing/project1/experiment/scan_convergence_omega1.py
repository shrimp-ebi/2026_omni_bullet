#!/usr/bin/env python3
"""
scan_convergence_omega1.py
LM法の収束域スキャン（ω₁方向）

ω₁の初期値を 15° ± 5°（10°〜20°）の範囲で 0.1° 刻みで変化させながら
LM法が収束するかどうかを記録する。

固定設定:
  参照画像 : ref_w1_15_w2_30.jpg
  初期ω₂  : 30°（真値固定）
  初期ω₃  : 0°（固定）
  真値     : ω₁=15°, ω₂=30°, ω₃=0°
  使用バイナリ: ./build/lm_scan_omega1

サマリCSV形式:
  init_deg, iter, final_E, frob_err, omega1_est_deg, R00..R22

判定基準:
  - converged      : "収束 (" が stdout に含まれる（||Δω|| < 1e-8 を達成）
  - converged_true : かつ Frobenius誤差 < FROB_THRESHOLD
"""

import csv
import math
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

# ── 設定 ─────────────────────────────────────────────────────────────────────
TRUE_DEG       = 15.0               # 真値 ω₁ [度]
SCAN_MIN       = TRUE_DEG - 5.0    # 10.0°
SCAN_MAX       = TRUE_DEG + 5.0    # 20.0°
STEP           = 0.1               # スキャン刻み幅 [度]
MAX_WORKERS    = 4                 # 並列実行数
FROB_THRESHOLD = 0.005             # Frobenius誤差の真値収束判定しきい値
TIMEOUT_SEC    = 240               # 1回の実行タイムアウト [秒]

BINARY     = "./build/lm_scan_omega1"
RESULT_CSV = "results/scan_convergence_omega1.csv"
RESULT_PNG = "results/scan_convergence_omega1.png"


# ── バイナリのビルド確認 ──────────────────────────────────────────────────────
def build_binary():
    print("lm_scan_omega1 をビルド中 (make lm_scan_omega1)...")
    ret = subprocess.run(["make", "lm_scan_omega1"],
                         capture_output=True, text=True)
    if ret.returncode != 0:
        print("ビルド失敗:")
        print(ret.stderr)
        sys.exit(1)
    print("ビルド完了\n")


# ── 1点分の LM 実行 ──────────────────────────────────────────────────────────
def run_lm_single(init_deg):
    """
    lm_scan_omega1 を init_deg (ω₁初期値) で実行し結果を返す。

    戻り値: dict {
        initial_deg, converged, converged_true,
        final_omega1_deg, error_deg, frob_err, final_E, iters
    }
    """
    conv_fd, conv_path = tempfile.mkstemp(suffix='.csv', prefix='scan_conv_')
    sum_fd,  sum_path  = tempfile.mkstemp(suffix='.csv', prefix='scan_sum_')
    os.close(conv_fd)
    os.close(sum_fd)

    result = {
        'initial_deg':     init_deg,
        'converged':       False,
        'converged_true':  False,
        'final_omega1_deg': float('nan'),
        'error_deg':       float('nan'),
        'frob_err':        float('nan'),
        'final_E':         float('nan'),
        'iters':           -1,
    }

    try:
        proc = subprocess.run(
            [BINARY, f"{init_deg:.1f}", conv_path, sum_path],
            capture_output=True, text=True, timeout=TIMEOUT_SEC
        )
        stdout = proc.stdout

        if proc.returncode != 0:
            return result

        # 収束判定（||Δω|| < 1e-8 を達成したか）
        result['converged'] = "収束 (" in stdout

        # サマリCSV を解析
        # 形式: init_deg,iter,final_E,frob_err,omega1_est_deg,R00..R22
        if os.path.exists(sum_path) and os.path.getsize(sum_path) > 0:
            with open(sum_path, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith('#'):
                        continue
                    parts = line.split(',')
                    if len(parts) >= 5:
                        try:
                            result['iters']    = int(parts[1])
                            result['final_E']  = float(parts[2])
                            frob_err           = float(parts[3])
                            omega1_est         = float(parts[4])
                            result['frob_err']         = frob_err
                            result['final_omega1_deg'] = omega1_est
                            result['error_deg']        = omega1_est - TRUE_DEG
                            result['converged_true']   = (
                                result['converged'] and frob_err < FROB_THRESHOLD
                            )
                        except (ValueError, IndexError):
                            pass

    except subprocess.TimeoutExpired:
        pass

    finally:
        for p in [conv_path, sum_path]:
            try:
                os.unlink(p)
            except OSError:
                pass

    return result


# ── スキャン実行 ──────────────────────────────────────────────────────────────
def scan(init_values):
    """全初期値について並列実行し、結果リストを返す（初期値順にソート）"""
    total   = len(init_values)
    done    = 0
    results = {}

    print(f"並列実行数: {MAX_WORKERS}  合計: {total} 点")
    print(f"推定所要時間: 約 {total * 18 // MAX_WORKERS // 60} 分\n")

    with ProcessPoolExecutor(max_workers=MAX_WORKERS) as executor:
        future_to_deg = {executor.submit(run_lm_single, deg): deg
                         for deg in init_values}

        for future in as_completed(future_to_deg):
            res = future.result()
            results[res['initial_deg']] = res
            done += 1

            init_d = res['initial_deg']
            if res['converged_true']:
                status = (f"○収束(真)  推定ω₁={res['final_omega1_deg']:+7.3f}°"
                          f"  誤差={res['error_deg']:+7.3f}°  Frob={res['frob_err']:.5f}")
            elif res['converged']:
                status = (f"△収束(局)  推定ω₁={res['final_omega1_deg']:+7.3f}°"
                          f"  Frob={res['frob_err']:.5f}")
            else:
                status = "✗ 発散"
            print(f"  [{done:3d}/{total}] ω₁初期値 = {init_d:+6.1f}°  {status}")

    return [results[d] for d in sorted(results.keys())]


# ── CSV 保存 ──────────────────────────────────────────────────────────────────
def save_csv(results):
    os.makedirs("results", exist_ok=True)
    fieldnames = [
        'initial_deg', 'converged', 'converged_true',
        'final_omega1_deg', 'error_deg', 'frob_err', 'final_E', 'iters'
    ]
    with open(RESULT_CSV, 'w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)
    print(f"\nCSV 保存: {RESULT_CSV}")


# ── グラフ生成 ────────────────────────────────────────────────────────────────
def save_plot(results):
    try:
        import matplotlib
        matplotlib.use('Agg')
        matplotlib.rcParams['font.family'] = 'IPAexGothic'
        import matplotlib.pyplot as plt
        import matplotlib.patches as mpatches
    except ImportError:
        print("警告: matplotlib が利用できません。グラフ生成をスキップします。")
        return

    init_arr  = [r['initial_deg']    for r in results]
    conv_true = [r['converged_true'] for r in results]
    conv_loc  = [r['converged'] and not r['converged_true'] for r in results]
    diverged  = [not r['converged']  for r in results]
    error_arr = [r['error_deg']      for r in results]

    x_true = [init_arr[i] for i, v in enumerate(conv_true) if v]
    x_loc  = [init_arr[i] for i, v in enumerate(conv_loc)  if v]
    x_div  = [init_arr[i] for i, v in enumerate(diverged)  if v]

    err_true = [error_arr[i] for i, v in enumerate(conv_true) if v]
    err_loc  = [error_arr[i] for i, v in enumerate(conv_loc)  if v]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 8), sharex=True)

    # ── 上段: 収束フラグ ──────────────────────────────────────────────
    bar_vals = []
    bar_cols = []
    for i in range(len(results)):
        if conv_true[i]:
            bar_vals.append(1); bar_cols.append('#2ecc71')  # 緑: 真値収束
        elif conv_loc[i]:
            bar_vals.append(1); bar_cols.append('#f39c12')  # 橙: 局所収束
        else:
            bar_vals.append(1); bar_cols.append('#e74c3c')  # 赤: 発散

    ax1.bar(init_arr, bar_vals, width=STEP * 0.9, color=bar_cols, zorder=3)
    ax1.axvline(TRUE_DEG, color='navy', linestyle='--', linewidth=1.5,
                label=f'真値 w1 = {TRUE_DEG:.0f} deg')

    patch_true = mpatches.Patch(color='#2ecc71', label='真値に収束')
    patch_loc  = mpatches.Patch(color='#f39c12', label='局所収束（真値に非収束）')
    patch_div  = mpatches.Patch(color='#e74c3c', label='発散')
    ax1.legend(handles=[patch_true, patch_loc, patch_div], loc='upper right', fontsize=9)
    ax1.set_ylabel('収束状態')
    ax1.set_yticks([])
    ax1.set_title(
        f'LM法 収束域スキャン（w2=30固定, w3=0固定, 真値 w1={TRUE_DEG:.0f}deg,'
        f' Frob閾値={FROB_THRESHOLD}）',
        fontsize=12
    )
    ax1.grid(axis='x', alpha=0.3)

    # ── 下段: 最終誤差 ────────────────────────────────────────────────
    if x_true:
        ax2.scatter(x_true, err_true, color='#2ecc71', s=30, zorder=4,
                    label=f'真値収束 (n={len(x_true)})')
    if x_loc:
        ax2.scatter(x_loc, err_loc, color='#f39c12', s=30, zorder=4, marker='x',
                    label=f'局所収束 (n={len(x_loc)})')

    ax2.axhline(0, color='navy', linestyle='--', linewidth=1.5, label='誤差 = 0')
    ax2.axvline(TRUE_DEG, color='navy', linestyle='--', linewidth=1.5)
    ax2.set_xlabel('w1 初期値 [deg]', fontsize=11)
    ax2.set_ylabel('最終誤差 (推定 w1 - 真値 w1) [deg]', fontsize=10)
    ax2.legend(loc='upper right', fontsize=9)
    ax2.grid(alpha=0.3)

    plt.tight_layout()
    os.makedirs("results", exist_ok=True)
    plt.savefig(RESULT_PNG, dpi=150, bbox_inches='tight')
    print(f"グラフ保存: {RESULT_PNG}")
    plt.close()


# ── 既存CSV から再プロット ────────────────────────────────────────────────────
def load_csv_and_reclassify():
    results = []
    with open(RESULT_CSV, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            frob = float(row['frob_err']) if row['frob_err'] != 'nan' else float('nan')
            conv = row['converged'] == 'True'
            conv_true = conv and (not math.isnan(frob)) and frob < FROB_THRESHOLD
            results.append({
                'initial_deg':     float(row['initial_deg']),
                'converged':       conv,
                'converged_true':  conv_true,
                'final_omega1_deg': float(row['final_omega1_deg']) if row['final_omega1_deg'] != 'nan' else float('nan'),
                'error_deg':       float(row['error_deg']) if row['error_deg'] != 'nan' else float('nan'),
                'frob_err':        frob,
                'final_E':         float(row['final_E']) if row['final_E'] != 'nan' else float('nan'),
                'iters':           int(row['iters']),
            })
    return sorted(results, key=lambda r: r['initial_deg'])


# ── メイン ───────────────────────────────────────────────────────────────────
def main():
    import argparse
    parser = argparse.ArgumentParser(description='LM法 ω₁収束域スキャン')
    parser.add_argument('--replot', action='store_true',
                        help='既存CSVを再利用してグラフのみ再生成する')
    parser.add_argument('--single', type=float, metavar='DEG',
                        help='1点だけ実行して収束判定を確認する（例: --single 14.5）')
    args = parser.parse_args()

    # 再プロットモード
    if args.replot:
        if not os.path.exists(RESULT_CSV):
            print(f"エラー: {RESULT_CSV} が見つかりません。")
            sys.exit(1)
        print(f"既存CSV ({RESULT_CSV}) から再プロット (Frob閾値={FROB_THRESHOLD})")
        results = load_csv_and_reclassify()
        save_plot(results)
        return

    # 1点テストモード
    if args.single is not None:
        if not os.path.exists(BINARY):
            build_binary()
        deg = round(args.single, 1)
        print(f"1点テスト: ω₁初期値 = {deg:+.1f}°  (TIMEOUT_SEC={TIMEOUT_SEC})")
        res = run_lm_single(deg)
        if res['converged_true']:
            status = (f"○真値収束  推定ω₁={res['final_omega1_deg']:+.3f}°"
                      f"  誤差={res['error_deg']:+.3f}°  Frob={res['frob_err']:.5f}")
        elif res['converged']:
            status = f"△局所収束  推定ω₁={res['final_omega1_deg']:+.3f}°  Frob={res['frob_err']:.5f}"
        else:
            status = "✗ 発散（タイムアウトまたは最大反復）"
        print(f"結果: {status}")
        return

    # ビルド確認
    if not os.path.exists(BINARY):
        build_binary()

    # スキャン範囲の生成
    init_values = []
    v = SCAN_MIN
    while v <= SCAN_MAX + 1e-9:
        init_values.append(round(v, 1))
        v += STEP

    print("=" * 60)
    print("LM法 収束域スキャン（ω₁方向）")
    print(f"  真値        : ω₁ = {TRUE_DEG:.1f}°  (ω₂=30°固定, ω₃=0°固定)")
    print(f"  スキャン範囲 : {SCAN_MIN:.1f}° 〜 {SCAN_MAX:.1f}°  ({STEP}° 刻み)")
    print(f"  総スキャン点 : {len(init_values)} 点")
    print(f"  Frob閾値    : {FROB_THRESHOLD}  (真値収束の判定)")
    print("=" * 60 + "\n")

    results = scan(init_values)
    save_csv(results)

    # サマリ表示
    n_total     = len(results)
    n_conv_true = sum(1 for r in results if r['converged_true'])
    n_conv_loc  = sum(1 for r in results if r['converged'] and not r['converged_true'])
    n_div       = sum(1 for r in results if not r['converged'])

    print(f"\n{'='*60}")
    print("スキャン結果サマリ")
    print(f"  総点数        : {n_total}")
    print(f"  真値収束      : {n_conv_true} 点  ({100*n_conv_true/n_total:.1f}%)")
    print(f"  局所収束      : {n_conv_loc} 点  ({100*n_conv_loc/n_total:.1f}%)")
    print(f"  発散          : {n_div} 点  ({100*n_div/n_total:.1f}%)")
    if n_conv_true > 0:
        degs_true = [r['initial_deg'] for r in results if r['converged_true']]
        print(f"  真値収束域    : {min(degs_true):.1f}° 〜 {max(degs_true):.1f}°")
    print(f"{'='*60}\n")

    save_plot(results)


if __name__ == '__main__':
    main()
