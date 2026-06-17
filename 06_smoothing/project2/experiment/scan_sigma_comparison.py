#!/usr/bin/env python3
"""
scan_sigma_comparison.py
σ値比較用収束域スキャン

σ値（LM_SIGMA環境変数）を 0.5〜4.0 の 8 パターンで変化させながら
ω₁方向・ω₂方向それぞれの収束域を測定し比較する。

固定設定:
  参照画像 : images/reference/ref_w1_15_w2_30.jpg
  真値     : ω₁=15°, ω₂=30°, ω₃=0°

結果:
  results/sigma_comparison.csv
  results/sigma_comparison_omega1.png
  results/sigma_comparison_omega2.png
"""

import argparse
import csv
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

# ── 設定 ─────────────────────────────────────────────────────────────────────
SIGMA_VALUES   = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0]
TRUE_OMEGA1    = 15.0
TRUE_OMEGA2    = 15.0
SCAN_HALF      = 5.0     # スキャン範囲 ± [度]
N_POINTS       = 101
STEP           = 0.1
MAX_WORKERS    = 4
FROB_THRESHOLD = 0.005
TIMEOUT_SEC    = 240

BINARY_OMEGA1 = "./build/lm_scan_omega1"
BINARY_OMEGA2 = "./build/lm_scan_omega2"
REF_PATH      = "images/reference/ref_w1_15_w2_15.jpg"

RESULT_CSV    = "results/sigma_comparison.csv"
RESULT_PNG_O1 = "results/sigma_comparison_omega1.png"
RESULT_PNG_O2 = "results/sigma_comparison_omega2.png"


# ── ユーティリティ ────────────────────────────────────────────────────────────
def make_init_values(true_deg):
    """true_deg ± SCAN_HALF を STEP 刻みで N_POINTS 点生成"""
    scan_min = true_deg - SCAN_HALF
    values = []
    v = scan_min
    while len(values) < N_POINTS:
        values.append(round(v, 1))
        v += STEP
    return values


# ── LM 実行（モジュールレベル関数 = pickle 可能） ────────────────────────────
def run_lm_omega1_single(args):
    """lm_scan_omega1 を 1点実行（引数: (init_deg, sigma)）"""
    init_deg, sigma = args
    conv_fd, conv_path = tempfile.mkstemp(suffix='.csv', prefix='sc1_')
    sum_fd,  sum_path  = tempfile.mkstemp(suffix='.csv', prefix='ss1_')
    os.close(conv_fd)
    os.close(sum_fd)

    result = {
        'initial_deg':    init_deg,
        'converged':      False,
        'converged_true': False,
        'frob_err':       float('nan'),
    }

    env = os.environ.copy()
    env['LM_SIGMA'] = str(sigma)

    try:
        proc = subprocess.run(
            [BINARY_OMEGA1, f"{init_deg:.1f}", conv_path, sum_path],
            capture_output=True, text=True, timeout=TIMEOUT_SEC, env=env
        )
        if proc.returncode == 0:
            result['converged'] = "収束 (" in proc.stdout
            _parse_summary(sum_path, result)
    except subprocess.TimeoutExpired:
        pass
    finally:
        _cleanup(conv_path, sum_path)

    return result


def run_lm_omega2_single(args):
    """lm_scan_omega2 を 1点実行（引数: (init_deg, sigma)）"""
    init_deg, sigma = args
    conv_fd, conv_path = tempfile.mkstemp(suffix='.csv', prefix='sc2_')
    sum_fd,  sum_path  = tempfile.mkstemp(suffix='.csv', prefix='ss2_')
    os.close(conv_fd)
    os.close(sum_fd)

    result = {
        'initial_deg':    init_deg,
        'converged':      False,
        'converged_true': False,
        'frob_err':       float('nan'),
    }

    env = os.environ.copy()
    env['LM_SIGMA'] = str(sigma)

    try:
        proc = subprocess.run(
            [BINARY_OMEGA2, f"{init_deg:.1f}", conv_path, sum_path, REF_PATH],
            capture_output=True, text=True, timeout=TIMEOUT_SEC, env=env
        )
        if proc.returncode == 0:
            result['converged'] = "収束 (" in proc.stdout
            _parse_summary(sum_path, result)
    except subprocess.TimeoutExpired:
        pass
    finally:
        _cleanup(conv_path, sum_path)

    return result


def _parse_summary(sum_path, result):
    """サマリCSV（列3 = frob_err）を解析して result を更新"""
    if not (os.path.exists(sum_path) and os.path.getsize(sum_path) > 0):
        return
    with open(sum_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split(',')
            if len(parts) >= 4:
                try:
                    frob_err = float(parts[3])
                    result['frob_err'] = frob_err
                    result['converged_true'] = (
                        result['converged'] and frob_err < FROB_THRESHOLD
                    )
                except (ValueError, IndexError):
                    pass


def _cleanup(*paths):
    for p in paths:
        try:
            os.unlink(p)
        except OSError:
            pass


# ── 1方向スキャン ─────────────────────────────────────────────────────────────
def scan_direction(sigma, init_values, run_func, dir_label):
    """
    1方向のスキャンを並列実行し収束統計を返す。

    戻り値: dict {converged_true, converged_local, diverged, width_deg}
    """
    total = len(init_values)
    done = 0
    results_map = {}
    args_list = [(deg, sigma) for deg in init_values]

    with ProcessPoolExecutor(max_workers=MAX_WORKERS) as executor:
        futures = {executor.submit(run_func, a): a[0] for a in args_list}
        for future in as_completed(futures):
            res = future.result()
            results_map[res['initial_deg']] = res
            done += 1
            print(f"\r[σ={sigma}] {dir_label}方向スキャン中... ({done}/{total})",
                  end='', flush=True)

    print()  # 改行

    results = [results_map[d] for d in sorted(results_map.keys())]

    n_true  = sum(1 for r in results if r['converged_true'])
    n_local = sum(1 for r in results if r['converged'] and not r['converged_true'])
    n_div   = sum(1 for r in results if not r['converged'])

    conv_true_degs = [r['initial_deg'] for r in results if r['converged_true']]
    if len(conv_true_degs) >= 2:
        width = max(conv_true_degs) - min(conv_true_degs)
    else:
        width = 0.0

    print(f"[σ={sigma}] {dir_label}方向："
          f"真値収束={n_true} 局所収束={n_local} 発散={n_div} 収束域={width:.1f}°")

    return {
        'converged_true':  n_true,
        'converged_local': n_local,
        'diverged':        n_div,
        'width_deg':       width,
    }


# ── CSV 保存 ──────────────────────────────────────────────────────────────────
def save_csv(all_results):
    """all_results: list of (sigma, direction, converged_true, converged_local, diverged, width_deg)"""
    os.makedirs('results', exist_ok=True)
    with open(RESULT_CSV, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['sigma', 'direction',
                         'converged_true', 'converged_local', 'diverged', 'width_deg'])
        for row in all_results:
            writer.writerow(row)
    print(f"\nCSV 保存: {RESULT_CSV}")


# ── グラフ保存 ────────────────────────────────────────────────────────────────
def save_plot_direction(all_results, direction_key, png_path, true_deg, dir_label):
    """
    指定方向の積み上げ棒グラフを生成・保存。
    all_results: list of (sigma, direction, converged_true, converged_local, diverged, width_deg)
    """
    try:
        import matplotlib
        matplotlib.use('Agg')
        matplotlib.rcParams['font.family'] = 'IPAexGothic'
        import matplotlib.pyplot as plt
        import matplotlib.patches as mpatches
    except ImportError:
        print("警告: matplotlib が利用できません。グラフ生成をスキップします。")
        return

    rows = [r for r in all_results if r[1] == direction_key]
    if not rows:
        print(f"警告: {direction_key} のデータがありません。")
        return

    sigmas    = [r[0]  for r in rows]
    n_true    = [r[2]  for r in rows]
    n_local   = [r[3]  for r in rows]
    n_div     = [r[4]  for r in rows]
    widths    = [r[5]  for r in rows]

    x     = list(range(len(sigmas)))
    bw    = 0.6
    bot_local = n_div
    bot_true  = [n_div[i] + n_local[i] for i in range(len(rows))]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8),
                                   gridspec_kw={'height_ratios': [3, 1]})

    # ── 上段: 積み上げ棒グラフ ────────────────────────────────────────
    ax1.bar(x, n_div,   bw, color='#e74c3c', label='発散')
    ax1.bar(x, n_local, bw, bottom=bot_local, color='#f1c40f', label='局所収束')
    ax1.bar(x, n_true,  bw, bottom=bot_true,  color='#2ecc71', label='真値収束')

    ax1.set_xticks(x)
    ax1.set_xticklabels([str(s) for s in sigmas], fontsize=11)
    ax1.set_ylabel('点数 (0〜101)', fontsize=11)
    ax1.set_ylim(0, N_POINTS + 8)
    ax1.set_title(
        f'σ値比較 収束域スキャン（{dir_label}方向）'
        f'  真値 ω={true_deg:.0f}°, ±{SCAN_HALF:.0f}°, {N_POINTS}点\n'
        f'  固定条件: ω₁=15°, ω₂=30°, ω₃=0°  Frob閾値={FROB_THRESHOLD}',
        fontsize=11
    )

    patch_true  = mpatches.Patch(color='#2ecc71', label='真値収束')
    patch_local = mpatches.Patch(color='#f1c40f', label='局所収束')
    patch_div   = mpatches.Patch(color='#e74c3c', label='発散')
    ax1.legend(handles=[patch_true, patch_local, patch_div],
               loc='lower right', fontsize=10)
    ax1.grid(axis='y', alpha=0.3)

    # ── 下段: 収束域の幅 ─────────────────────────────────────────────
    ax2.bar(x, widths, bw, color='#3498db')
    ax2.set_xticks(x)
    ax2.set_xticklabels([str(s) for s in sigmas], fontsize=11)
    ax2.set_xlabel('σ 値', fontsize=12)
    ax2.set_ylabel('収束域 [°]', fontsize=10)
    ax2.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    os.makedirs('results', exist_ok=True)
    plt.savefig(png_path, dpi=150, bbox_inches='tight')
    print(f"グラフ保存: {png_path}")
    plt.close()


# ── メイン ───────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description='σ値比較用収束域スキャン（ω₁・ω₂方向, LM_SIGMA環境変数使用）',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
実行例:
  python experiment/scan_sigma_comparison.py          # 全σスキャン実行
  python experiment/scan_sigma_comparison.py --replot # 既存CSVから再プロットのみ
""")
    parser.add_argument('--replot', action='store_true',
                        help='既存CSVから再プロットのみ実行（スキャンはスキップ）')
    args = parser.parse_args()

    # ── 再プロットモード ─────────────────────────────────────────────
    if args.replot:
        if not os.path.exists(RESULT_CSV):
            print(f"エラー: {RESULT_CSV} が見つかりません。")
            sys.exit(1)
        all_results = []
        with open(RESULT_CSV, 'r', encoding='utf-8') as f:
            reader = csv.reader(f)
            next(reader)
            for row in reader:
                all_results.append((
                    float(row[0]), row[1],
                    int(row[2]), int(row[3]), int(row[4]), float(row[5])
                ))
        print(f"既存CSV ({RESULT_CSV}) から再プロット中...")
        save_plot_direction(all_results, 'omega1', RESULT_PNG_O1, TRUE_OMEGA1, 'ω₁')
        save_plot_direction(all_results, 'omega2', RESULT_PNG_O2, TRUE_OMEGA2, 'ω₂')
        return

    # ── バイナリ・参照画像の確認 ─────────────────────────────────────
    for path, label in [(BINARY_OMEGA1, 'lm_scan_omega1'),
                        (BINARY_OMEGA2, 'lm_scan_omega2'),
                        (REF_PATH, '参照画像')]:
        if not os.path.exists(path):
            print(f"エラー: {label} が見つかりません: {path}")
            sys.exit(1)

    # ── 推定時間表示 ─────────────────────────────────────────────────
    n_sigma    = len(SIGMA_VALUES)
    n_dirs     = 2
    sec_per_pt = 18
    est_sec    = n_sigma * n_dirs * N_POINTS * sec_per_pt // MAX_WORKERS
    print(f"推定所要時間：約{est_sec // 60}分"
          f"（σ{n_sigma}パターン × {n_dirs}方向 × {N_POINTS}点）")
    print(f"並列実行数: {MAX_WORKERS}  Frob閾値: {FROB_THRESHOLD}")
    print()

    init_omega1 = make_init_values(TRUE_OMEGA1)
    init_omega2 = make_init_values(TRUE_OMEGA2)

    # all_results: list of (sigma, direction_key, n_true, n_local, n_div, width_deg)
    all_results = []

    for sigma in SIGMA_VALUES:
        print(f"\n{'='*55}")
        print(f"  σ = {sigma}")
        print(f"{'='*55}")

        r1 = scan_direction(sigma, init_omega1, run_lm_omega1_single, 'ω₁')
        all_results.append((sigma, 'omega1',
                             r1['converged_true'], r1['converged_local'],
                             r1['diverged'], r1['width_deg']))

        r2 = scan_direction(sigma, init_omega2, run_lm_omega2_single, 'ω₂')
        all_results.append((sigma, 'omega2',
                             r2['converged_true'], r2['converged_local'],
                             r2['diverged'], r2['width_deg']))

    save_csv(all_results)
    save_plot_direction(all_results, 'omega1', RESULT_PNG_O1, TRUE_OMEGA1, 'ω₁')
    save_plot_direction(all_results, 'omega2', RESULT_PNG_O2, TRUE_OMEGA2, 'ω₂')

    print("\n全σ値スキャン完了。")


if __name__ == '__main__':
    main()
