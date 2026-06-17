#!/usr/bin/env python3
"""
scan_convergence.py
LM法の収束域スキャン

ω₂の初期値を -30° ± 5° の範囲で 0.1° 刻みで変化させながら
LM法が収束するかどうかを記録する。

真値: ω₁=0, ω₂=-30°, ω₃=0
使用バイナリ: ./build/lm_convergence_cases
  引数: <初期角度_度> <収束CSV> <サマリCSV>
  出力: 収束時は "収束 (" を含む行を stdout に出力
        サマリCSV: init_deg,iter,final_E,frob_err,R00..R22

判定基準:
  - converged:       "収束 (" が stdout に含まれる（||Δω|| < 1e-8 を達成）
  - converged_true:  かつ Frobenius誤差 < FROB_THRESHOLD（真の最小値に収束）
"""

import csv
import math
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed

# ── 設定 ───────────────────────────────────────────────────────────────────
TRUE_DEG       = -30.0    # 真値 ω₂ [度]
SCAN_MIN       = TRUE_DEG - 5.0   # -35.0°
SCAN_MAX       = TRUE_DEG + 5.0   # -25.0°
STEP           = 0.1              # スキャン刻み幅 [度]
MAX_WORKERS    = 4                # 並列実行数（並列数を減らして1プロセスの時間を確保）
FROB_THRESHOLD = 0.005            # Frobenius誤差の収束判定しきい値（真値収束: Frob≈0）
TIMEOUT_SEC    = 240              # 1回の実行タイムアウト [秒]（120→240: タイムアウト誤判定を防ぐ）

BINARY     = "./build/lm_convergence_cases"
RESULT_CSV = "results/scan_convergence.csv"
RESULT_PNG = "results/scan_convergence.png"


# ── バイナリのビルド確認 ────────────────────────────────────────────────────
def build_binary():
    print("lm_convergence_cases をビルド中 (make lm_convergence_cases)...")
    ret = subprocess.run(["make", "lm_convergence_cases"],
                         capture_output=True, text=True)
    if ret.returncode != 0:
        print("ビルド失敗:")
        print(ret.stderr)
        sys.exit(1)
    print("ビルド完了\n")


# ── ω₂ 抽出 (回転行列の第1行・第3行から) ────────────────────────────────
def extract_omega2_deg(r00, r02):
    """
    Y軸回転 R_Y(θ) の慣例:
      R = [[cosθ, 0, sinθ], [0, 1, 0], [-sinθ, 0, cosθ]]
    θ = atan2(R[0][2], R[0][0])
    """
    return math.atan2(r02, r00) * 180.0 / math.pi


# ── 1点分の LM 実行 ─────────────────────────────────────────────────────────
def run_lm_single(init_deg):
    """
    lm_convergence_cases を init_deg で実行し結果を返す。
    並列実行のため ProcessPoolExecutor から呼ばれる。

    戻り値: dict {
        initial_deg, converged, converged_true,
        final_omega2_deg, error_deg, frob_err, final_E, iters
    }
    """
    # 一意の一時ファイルを生成
    conv_fd, conv_path = tempfile.mkstemp(suffix='.csv', prefix='scan_conv_')
    sum_fd,  sum_path  = tempfile.mkstemp(suffix='.csv', prefix='scan_sum_')
    os.close(conv_fd)
    os.close(sum_fd)

    result = {
        'initial_deg':    init_deg,
        'converged':      False,
        'converged_true': False,
        'final_omega2_deg': float('nan'),
        'error_deg':      float('nan'),
        'frob_err':       float('nan'),
        'final_E':        float('nan'),
        'iters':          -1,
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

        # サマリ CSV を解析
        # 形式: init_deg,iter,final_E,frob_err,R00,R01,R02,R10,R11,R12,R20,R21,R22
        if os.path.exists(sum_path) and os.path.getsize(sum_path) > 0:
            with open(sum_path, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith('#'):
                        continue
                    parts = line.split(',')
                    if len(parts) >= 13:
                        try:
                            result['iters']   = int(parts[1])
                            result['final_E'] = float(parts[2])
                            frob_err          = float(parts[3])
                            r00               = float(parts[4])
                            r02               = float(parts[6])
                            result['frob_err'] = frob_err
                            final_omega2_deg   = extract_omega2_deg(r00, r02)
                            result['final_omega2_deg'] = final_omega2_deg
                            result['error_deg']        = final_omega2_deg - TRUE_DEG
                            # 真の最小値への収束判定
                            result['converged_true'] = (
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


# ── スキャン実行 ────────────────────────────────────────────────────────────
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

            # 進捗表示
            init_d = res['initial_deg']
            if res['converged_true']:
                status = f"○収束(真)  最終={res['final_omega2_deg']:+7.3f}°  誤差={res['error_deg']:+7.3f}°  Frob={res['frob_err']:.4f}"
            elif res['converged']:
                status = f"△収束(局)  最終={res['final_omega2_deg']:+7.3f}°  Frob={res['frob_err']:.4f}"
            else:
                status = "✗ 発散"
            print(f"  [{done:3d}/{total}] ω₂初期値 = {init_d:+6.1f}°  {status}")

    # 初期値の昇順にソート
    return [results[d] for d in sorted(results.keys())]


# ── CSV 保存 ────────────────────────────────────────────────────────────────
def save_csv(results):
    os.makedirs("results", exist_ok=True)
    fieldnames = [
        'initial_deg', 'converged', 'converged_true',
        'final_omega2_deg', 'error_deg', 'frob_err', 'final_E', 'iters'
    ]
    with open(RESULT_CSV, 'w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)
    print(f"\nCSV 保存: {RESULT_CSV}")


# ── グラフ生成 ──────────────────────────────────────────────────────────────
def save_plot(results):
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        import matplotlib.patches as mpatches
        from matplotlib import rcParams
        rcParams['font.family'] = 'Noto Sans CJK JP'
    except ImportError:
        print("警告: matplotlib が利用できません。グラフ生成をスキップします。")
        return

    init_arr  = [r['initial_deg']    for r in results]
    conv_true = [r['converged_true'] for r in results]
    conv_loc  = [r['converged'] and not r['converged_true'] for r in results]
    diverged  = [not r['converged']  for r in results]
    error_arr = [r['error_deg']      for r in results]

    # 収束/局所収束/発散 それぞれの点
    x_true = [init_arr[i] for i, v in enumerate(conv_true) if v]
    x_loc  = [init_arr[i] for i, v in enumerate(conv_loc)  if v]
    x_div  = [init_arr[i] for i, v in enumerate(diverged)  if v]

    err_true = [error_arr[i] for i, v in enumerate(conv_true) if v]
    err_loc  = [error_arr[i] for i, v in enumerate(conv_loc)  if v]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 8), sharex=True)

    # ── 上段: 収束フラグ ─────────────────────────────────────────────
    # 3種類の帯を積み上げ表示
    bar_vals = []
    bar_cols = []
    for i in range(len(results)):
        if conv_true[i]:
            bar_vals.append(1)
            bar_cols.append('#2ecc71')  # 緑: 真の最小値
        elif conv_loc[i]:
            bar_vals.append(1)
            bar_cols.append('#f39c12')  # 橙: 局所収束
        else:
            bar_vals.append(1)
            bar_cols.append('#e74c3c')  # 赤: 発散

    ax1.bar(init_arr, bar_vals, width=STEP * 0.9, color=bar_cols, zorder=3)
    ax1.axvline(TRUE_DEG, color='navy', linestyle='--', linewidth=1.5,
                label=f'真値 w2 = {TRUE_DEG:.0f} deg')

    patch_true = mpatches.Patch(color='#2ecc71', label='真値に収束')
    patch_loc  = mpatches.Patch(color='#f39c12', label='局所収束（真値に非収束）')
    patch_div  = mpatches.Patch(color='#e74c3c', label='発散')
    ax1.legend(handles=[patch_true, patch_loc, patch_div], loc='upper right', fontsize=9)
    ax1.set_ylabel('収束状態')
    ax1.set_yticks([])
    ax1.set_title(
        f'LM法 収束域スキャン  (w1=w3=0固定, 真値 w2={TRUE_DEG:.0f}deg, Frob閾値={FROB_THRESHOLD})',
        fontsize=12
    )
    ax1.grid(axis='x', alpha=0.3)

    # ── 下段: 最終誤差 ───────────────────────────────────────────────
    if x_true:
        ax2.scatter(x_true, err_true, color='#2ecc71', s=30, zorder=4,
                    label=f'真値収束 (n={len(x_true)})')
    if x_loc:
        ax2.scatter(x_loc, err_loc, color='#f39c12', s=30, zorder=4, marker='x',
                    label=f'局所収束 (n={len(x_loc)})')

    ax2.axhline(0, color='navy', linestyle='--', linewidth=1.5, label='誤差 = 0')
    ax2.axvline(TRUE_DEG, color='navy', linestyle='--', linewidth=1.5)
    ax2.set_xlabel('w2 初期値 [deg]', fontsize=11)
    ax2.set_ylabel('最終誤差 (推定値 - 真値) [deg]', fontsize=10)
    ax2.legend(loc='upper right', fontsize=9)
    ax2.grid(alpha=0.3)

    plt.tight_layout()
    os.makedirs("results", exist_ok=True)
    plt.savefig(RESULT_PNG, dpi=150, bbox_inches='tight')
    print(f"グラフ保存: {RESULT_PNG}")
    plt.close()


# ── メイン ──────────────────────────────────────────────────────────────────
def load_csv_and_reclassify():
    """既存 CSV を読み込み、現在の FROB_THRESHOLD で再分類して返す"""
    results = []
    with open(RESULT_CSV, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            frob = float(row['frob_err']) if row['frob_err'] != 'nan' else float('nan')
            conv = row['converged'] == 'True'
            conv_true = conv and (not math.isnan(frob)) and frob < FROB_THRESHOLD
            results.append({
                'initial_deg':      float(row['initial_deg']),
                'converged':        conv,
                'converged_true':   conv_true,
                'final_omega2_deg': float(row['final_omega2_deg']) if row['final_omega2_deg'] != 'nan' else float('nan'),
                'error_deg':        float(row['error_deg']) if row['error_deg'] != 'nan' else float('nan'),
                'frob_err':         frob,
                'final_E':          float(row['final_E']) if row['final_E'] != 'nan' else float('nan'),
                'iters':            int(row['iters']),
            })
    return sorted(results, key=lambda r: r['initial_deg'])


def main():
    import argparse
    parser = argparse.ArgumentParser(description='LM法収束域スキャン')
    parser.add_argument('--replot', action='store_true',
                        help='既存CSVを再利用してグラフのみ再生成する')
    parser.add_argument('--single', type=float, metavar='DEG',
                        help='1点だけ実行して収束判定を確認する（例: --single -29.2）')
    args = parser.parse_args()

    # ── 再プロットモード ─────────────────────────────────────────────────
    if args.replot:
        if not os.path.exists(RESULT_CSV):
            print(f"エラー: {RESULT_CSV} が見つかりません。先にスキャンを実行してください。")
            sys.exit(1)
        print(f"既存CSV ({RESULT_CSV}) から再プロット (Frob閾値={FROB_THRESHOLD})")
        results = load_csv_and_reclassify()
        save_plot(results)
        return

    # ── 1点テストモード ──────────────────────────────────────────────────
    if args.single is not None:
        if not os.path.exists(BINARY):
            build_binary()
        deg = round(args.single, 1)
        print(f"1点テスト: ω₂初期値 = {deg:+.1f}°  (TIMEOUT_SEC={TIMEOUT_SEC})")
        res = run_lm_single(deg)
        if res['converged_true']:
            status = f"○真値収束  最終値={res['final_omega2_deg']:+.3f}°  誤差={res['error_deg']:+.3f}°  Frob={res['frob_err']:.5f}"
        elif res['converged']:
            status = f"△局所収束  最終値={res['final_omega2_deg']:+.3f}°  Frob={res['frob_err']:.5f}"
        else:
            status = "✗ 発散（タイムアウトまたは最大反復）"
        print(f"結果: {status}")
        return

    # ── ビルド確認 ─────────────────────────────────────────────────────
    if not os.path.exists(BINARY):
        build_binary()

    # スキャン範囲の生成
    init_values = []
    v = SCAN_MIN
    while v <= SCAN_MAX + 1e-9:
        init_values.append(round(v, 1))
        v += STEP

    print("=" * 60)
    print(f"LM法 収束域スキャン")
    print(f"  真値       : ω₂ = {TRUE_DEG:.1f}°  (ω₁=ω₃=0)")
    print(f"  スキャン範囲: {SCAN_MIN:.1f}° 〜 {SCAN_MAX:.1f}°  ({STEP}° 刻み)")
    print(f"  総スキャン点: {len(init_values)} 点")
    print(f"  Frob閾値   : {FROB_THRESHOLD}  (真値収束の判定)")
    print("=" * 60 + "\n")

    # スキャン実行
    results = scan(init_values)

    # CSV 保存
    save_csv(results)

    # サマリ表示
    n_total     = len(results)
    n_conv_true = sum(1 for r in results if r['converged_true'])
    n_conv_loc  = sum(1 for r in results if r['converged'] and not r['converged_true'])
    n_div       = sum(1 for r in results if not r['converged'])

    print(f"\n{'='*60}")
    print(f"スキャン結果サマリ")
    print(f"  総点数        : {n_total}")
    print(f"  真値収束      : {n_conv_true} 点  ({100*n_conv_true/n_total:.1f}%)")
    print(f"  局所収束      : {n_conv_loc} 点  ({100*n_conv_loc/n_total:.1f}%)")
    print(f"  発散          : {n_div} 点  ({100*n_div/n_total:.1f}%)")
    if n_conv_true > 0:
        degs_true = [r['initial_deg'] for r in results if r['converged_true']]
        print(f"  真値収束域    : {min(degs_true):.1f}° 〜 {max(degs_true):.1f}°")
    print(f"{'='*60}\n")

    # グラフ生成
    save_plot(results)


if __name__ == '__main__':
    main()
