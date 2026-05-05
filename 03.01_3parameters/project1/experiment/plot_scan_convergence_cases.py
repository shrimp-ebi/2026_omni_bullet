#!/usr/bin/env python3
"""
plot_scan_convergence_cases.py
scan_convergence.csv から代表3ケースを選び、
LM法の収束曲線グラフを1枚にまとめる。

ケース選択:
  A (真値収束) : converged_true=True のうち真値から最も遠い初期値
  B (局所収束) : converged=True かつ converged_true=False のうち最も遠い初期値
  C (発散)     : converged=False のうち真値に最も近い初期値

使用バイナリ: ./build/lm_convergence_cases
出力: results/convergence_scan_cases.png
"""

import csv
import math
import os
import subprocess
import sys

import matplotlib
import matplotlib.font_manager as fm
import matplotlib.pyplot as plt

# ── 日本語フォント設定 ───────────────────────────────────────────────────────
def find_japanese_font():
    candidates = [
        "Noto Sans CJK JP", "Noto Sans JP",
        "IPAGothic", "IPAPGothic", "IPAexGothic",
        "TakaoGothic", "VL Gothic",
    ]
    available = {f.name for f in fm.fontManager.ttflist}
    for name in candidates:
        if name in available:
            return name
    return None

font_name = find_japanese_font()
if font_name:
    matplotlib.rcParams["font.family"] = font_name

# ── 設定 ────────────────────────────────────────────────────────────────────
TRUE_DEG       = -30.0
FROB_THRESHOLD = 0.005   # 真値収束の判定しきい値（scan_convergence.py と統一）
BINARY      = "./build/lm_convergence_cases"
SCAN_CSV    = "results/scan_convergence.csv"
OUT_PNG     = "results/convergence_scan_cases.png"

CASE_FILES = {
    "A": ("results/conv_caseA.csv", "results/summary_caseA.csv"),
    "B": ("results/conv_caseB.csv", "results/summary_caseB.csv"),
    "C": ("results/conv_caseC.csv", "results/summary_caseC.csv"),
}

COLORS = {
    "A": "#2ca02c",   # 緑: 真値収束
    "B": "#ff7f0e",   # 橙: 局所収束
    "C": "#d62728",   # 赤: 発散
}


# ── ビルド確認 ────────────────────────────────────────────────────────────────
def ensure_binary():
    print("make lm_convergence_cases ...")
    ret = subprocess.run(["make", "lm_convergence_cases"],
                         capture_output=True, text=True)
    if ret.returncode != 0:
        print("ビルド失敗:")
        print(ret.stderr)
        sys.exit(1)
    print("ビルド完了\n")


# ── scan_convergence.csv から3ケースを特定 ───────────────────────────────────
def select_cases():
    rows = list(csv.DictReader(open(SCAN_CSV, encoding="utf-8")))

    # ケースA: converged_true=True のうち真値から最も遠い
    cands_a = [r for r in rows if r["converged_true"] == "True"]
    if not cands_a:
        print("エラー: 真値収束点が見つかりません")
        sys.exit(1)
    case_a = max(cands_a, key=lambda r: abs(float(r["initial_deg"]) - TRUE_DEG))

    # ケースB: converged=True かつ converged_true=False のうち最も遠い
    cands_b = [r for r in rows
               if r["converged"] == "True" and r["converged_true"] == "False"]
    if not cands_b:
        print("エラー: 局所収束点が見つかりません")
        sys.exit(1)
    case_b = max(cands_b, key=lambda r: abs(float(r["initial_deg"]) - TRUE_DEG))

    # ケースC: converged=False のうち真値に最も近い
    cands_c = [r for r in rows if r["converged"] == "False"]
    if not cands_c:
        print("エラー: 発散点が見つかりません")
        sys.exit(1)
    case_c = min(cands_c, key=lambda r: abs(float(r["initial_deg"]) - TRUE_DEG))

    return {
        "A": float(case_a["initial_deg"]),
        "B": float(case_b["initial_deg"]),
        "C": float(case_c["initial_deg"]),
    }


# ── lm_convergence_cases を実行 ──────────────────────────────────────────────
def run_case(label, init_deg):
    """
    バイナリを実行し、実際の収束結果を返す。
    戻り値: dict {converged, converged_true, frob_err, final_E, iters}
    """
    conv_csv, sum_csv = CASE_FILES[label]

    # 既存ファイルを削除（appendモード対策）
    for path in [conv_csv, sum_csv]:
        if os.path.exists(path):
            os.remove(path)

    print(f"  ケース{label}: ./build/lm_convergence_cases {init_deg:.1f} "
          f"{conv_csv} {sum_csv}")
    ret = subprocess.run(
        [BINARY, f"{init_deg:.1f}", conv_csv, sum_csv],
        capture_output=True, text=True, timeout=300
    )
    converged = "収束 (" in ret.stdout

    # サマリCSVから Frobenius誤差と最終E を取得
    frob_err = float("nan")
    final_E  = float("nan")
    iters    = -1
    if os.path.exists(sum_csv) and os.path.getsize(sum_csv) > 0:
        with open(sum_csv, encoding="utf-8") as f:
            for line in f:
                parts = line.strip().split(",")
                if len(parts) >= 4:
                    try:
                        iters    = int(parts[1])
                        final_E  = float(parts[2])
                        frob_err = float(parts[3])
                    except ValueError:
                        pass

    converged_true = converged and (not math.isnan(frob_err)) and frob_err < FROB_THRESHOLD

    if converged_true:
        actual = "真値収束"
    elif converged:
        actual = "局所収束"
    else:
        actual = "未収束"

    print(f"    → {actual}  Frob={frob_err:.5f}  最終E={final_E:.4f}  反復={iters}")
    return {
        "converged":      converged,
        "converged_true": converged_true,
        "frob_err":       frob_err,
        "final_E":        final_E,
        "iters":          iters,
        "actual":         actual,
    }


# ── 収束CSV を読み込んで (iter, E) リストを返す ──────────────────────────────
def load_convergence_csv(label):
    conv_csv = CASE_FILES[label][0]
    iters, Es = [], []
    with open(conv_csv, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("case,"):
                continue
            parts = line.split(",")
            if len(parts) >= 3:
                try:
                    iters.append(int(parts[1]))
                    Es.append(float(parts[2]))
                except ValueError:
                    pass
    return iters, Es


# ── グラフ描画 ───────────────────────────────────────────────────────────────
def plot(cases, run_results):
    """
    cases     : {label: init_deg}
    run_results: {label: run_case() の戻り値 dict}
    """
    # scan分類の説明（凡例用）
    scan_class = {
        "A": "scan: 真値収束",
        "B": "scan: 局所収束",
        "C": "scan: 発散",
    }

    fig, ax = plt.subplots(figsize=(10, 6))

    max_iter_all = 1
    for label in sorted(cases):
        iters, _ = load_convergence_csv(label)
        if iters:
            max_iter_all = max(max_iter_all, iters[-1])

    for label in sorted(cases.keys()):
        init_deg = cases[label]
        res      = run_results[label]
        iters, Es = load_convergence_csv(label)
        if not iters:
            print(f"  警告: ケース{label} のデータが空です")
            continue

        # ラベル: 実際の収束型を表示（scan分類と異なる場合は注記）
        actual   = res["actual"]
        sc       = scan_class[label]
        if actual in sc:
            # scan分類と一致
            legend_label = f"ケース{label}: 初期値{init_deg:+.1f}°  ({actual})"
        else:
            # scan誤判定 → 実際の結果を優先表示
            legend_label = (
                f"ケース{label}: 初期値{init_deg:+.1f}°  "
                f"({sc} → 実際:{actual})"
            )

        linestyle = "--" if actual == "局所収束" else "-"
        ax.plot(iters, Es,
                marker="o", markersize=3,
                label=legend_label,
                color=COLORS[label],
                linewidth=1.8,
                linestyle=linestyle)

        # 最終点に注釈
        ax.annotate(
            f"E={Es[-1]:.2f}",
            xy=(iters[-1], Es[-1]),
            xytext=(iters[-1] + max_iter_all * 0.04, Es[-1] * 1.8),
            fontsize=9,
            color=COLORS[label],
            arrowprops=dict(arrowstyle="->", color=COLORS[label], lw=1.0),
        )

    ax.set_xlabel("反復回数", fontsize=13)
    ax.set_ylabel("目的関数 E", fontsize=13)
    ax.set_title(
        "LM法 収束曲線（初期値スキャンより代表3ケース）\n"
        f"真値: w2 = {TRUE_DEG:.0f} deg  (w1=w3=0)",
        fontsize=13
    )
    ax.legend(fontsize=10, loc="upper right")
    ax.grid(True, alpha=0.4)
    ax.set_yscale("log")
    ax.set_xlim(left=0)

    os.makedirs("results", exist_ok=True)
    plt.tight_layout()
    plt.savefig(OUT_PNG, dpi=150, bbox_inches="tight")
    print(f"\nグラフ保存: {OUT_PNG}")
    plt.close()


# ── メイン ───────────────────────────────────────────────────────────────────
def main():
    ensure_binary()

    print("=== ケース選択 ===")
    cases = select_cases()
    for label, deg in cases.items():
        print(f"  ケース{label}: 初期値 {deg:+.1f}°")
    print()

    print("=== lm_convergence_cases 実行 ===")
    run_results = {}
    for label, deg in cases.items():
        run_results[label] = run_case(label, deg)
    print()

    print("=== グラフ生成 ===")
    plot(cases, run_results)


if __name__ == "__main__":
    main()
