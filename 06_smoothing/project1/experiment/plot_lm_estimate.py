#!/usr/bin/env python3
"""
plot_lm_estimate.py
lm_estimate_small.txt（または lm_estimate_large.txt）の収束結果をグラフ化

グラフ1: 反復回数 vs 目的関数E（収束曲線、3ケース重ね）
グラフ2: 反復回数 vs ||Δω||（ステップサイズ推移）
グラフ3: 結果サマリ表

使い方:
    cd project1
    python3 experiment/plot_lm_estimate.py [入力txtファイル] [出力ディレクトリ]

    # 例（デフォルト引数）
    python3 experiment/plot_lm_estimate.py \
        results/lm_estimate_small.txt \
        results/graphs_lm_estimate
"""

import re
import sys
import os
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

# ── 日本語フォント ─────────────────────────────────────────────────
def find_japanese_font():
    candidates = [
        "Noto Sans CJK JP", "Noto Sans JP",
        "IPAGothic", "IPAPGothic", "IPAexGothic",
        "TakaoGothic", "VL Gothic",
        "Hiragino Sans", "Yu Gothic", "Meiryo",
    ]
    available = {f.name for f in fm.fontManager.ttflist}
    for name in candidates:
        if name in available:
            return name
    keywords = ["noto", "ipa", "takao", "gothic", "hiragino", "meiryo"]
    for f in fm.fontManager.ttflist:
        for kw in keywords:
            if kw in f.fname.lower():
                return f.name
    return None

font_name = find_japanese_font()
if font_name:
    matplotlib.rcParams["font.family"] = font_name

# ── テキストファイルのパース ────────────────────────────────────────
def parse_lm_txt(path):
    """
    lm_estimate_small.txt / lm_estimate_large.txt を解析して
    ケースごとのデータリストを返す。

    戻り値: list of dict
        {
          "label": str,      # "ケース1: (14.0, 29.0, 0.0)"
          "init": [float x3],
          "init_E": float,
          "iters":  [int],
          "Es":     [float],
          "norms":  [float],
          "accepted": [bool],
          "R":      [[float x3] x3],
          "frob":   float,
          "final_E": float,
          "converged": bool,
        }
    """
    cases = []
    cur = None

    re_case  = re.compile(r"ケース\d+.*?=\s*\(([\d.]+),\s*([\d.]+),\s*([\d.]+)\)")
    re_initE = re.compile(r"初期 E = ([\d.]+)")
    re_iter  = re.compile(r"iter=(\d+)\s+E=([\d.]+)\s+\|Δω\|=([\d.e+\-]+)\s+C=([\d.e+\-]+)\s+(受理|棄却)")
    re_row   = re.compile(r"\[\s*([\d.+\-]+)\s+([\d.+\-]+)\s+([\d.+\-]+)\]")
    re_frob  = re.compile(r"Frobenius誤差:\s*([\d.]+)")
    re_finalE = re.compile(r"最終 E = ([\d.]+)")
    re_conv  = re.compile(r"収束しました")
    re_maxiter = re.compile(r"最大反復回数")

    reading_R = False
    R_rows    = []

    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip()

            m = re_case.search(line)
            if m:
                if cur is not None:
                    cases.append(cur)
                w1, w2, w3 = float(m.group(1)), float(m.group(2)), float(m.group(3))
                cur = {
                    "label": f"ケース{len(cases)+1}: ({w1:.0f}°, {w2:.0f}°, {w3:.0f}°)",
                    "init": [w1, w2, w3],
                    "init_E": None,
                    "iters": [], "Es": [], "norms": [], "accepted": [],
                    "R": None, "frob": None, "final_E": None,
                    "converged": False,
                }
                reading_R = False
                R_rows = []
                continue

            if cur is None:
                continue

            m = re_initE.search(line)
            if m:
                cur["init_E"] = float(m.group(1))
                continue

            m = re_iter.search(line)
            if m:
                cur["iters"].append(int(m.group(1)))
                cur["Es"].append(float(m.group(2)))
                cur["norms"].append(float(m.group(3)))
                cur["accepted"].append(m.group(5) == "受理")
                continue

            if "推定された回転行列" in line:
                reading_R = True
                R_rows = []
                continue

            if reading_R:
                m = re_row.search(line)
                if m:
                    R_rows.append([float(m.group(1)), float(m.group(2)), float(m.group(3))])
                    if len(R_rows) == 3:
                        cur["R"] = R_rows
                        reading_R = False
                continue

            m = re_frob.search(line)
            if m:
                cur["frob"] = float(m.group(1))
                continue

            m = re_finalE.search(line)
            if m:
                cur["final_E"] = float(m.group(1))
                continue

            if re_conv.search(line):
                cur["converged"] = True
                continue

            if re_maxiter.search(line):
                cur["converged"] = False
                continue

    if cur is not None:
        cases.append(cur)
    return cases


# ── メイン ─────────────────────────────────────────────────────────
def main():
    in_txt  = sys.argv[1] if len(sys.argv) > 1 else "results/lm_estimate_small.txt"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "results/graphs_lm_estimate"
    os.makedirs(out_dir, exist_ok=True)

    cases = parse_lm_txt(in_txt)
    if not cases:
        print(f"エラー: データが見つかりません: {in_txt}")
        sys.exit(1)

    colors = ["#1f77b4", "#ff7f0e", "#2ca02c"]
    n = len(cases)

    # ── グラフ1: 収束曲線（反復回数 vs E）───────────────────────────
    fig1, ax1 = plt.subplots(figsize=(9, 5))
    for i, cas in enumerate(cases):
        iters = [-1] + cas["iters"]
        Es    = [cas["init_E"]] + cas["Es"]
        ax1.plot(iters, Es, color=colors[i % len(colors)],
                 linewidth=1.8, marker="o", markersize=3, label=cas["label"])

    ax1.set_xlabel("反復回数", fontsize=12)
    ax1.set_ylabel("目的関数 E", fontsize=12)
    ax1.set_title("LM法 収束曲線（3ケース）", fontsize=13)
    ax1.set_yscale("log")
    ax1.set_xlim(left=-1)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.4)
    plt.tight_layout()
    out1 = os.path.join(out_dir, "convergence_E.png")
    fig1.savefig(out1, dpi=150, bbox_inches="tight")
    print(f"保存: {out1}")

    # ── グラフ2: ステップサイズ推移（反復回数 vs ||Δω||）─────────────
    fig2, ax2 = plt.subplots(figsize=(9, 5))
    for i, cas in enumerate(cases):
        ax2.plot(cas["iters"], cas["norms"], color=colors[i % len(colors)],
                 linewidth=1.5, marker="o", markersize=3, label=cas["label"])

    ax2.axhline(1e-8, color="gray", linestyle="--", linewidth=1.0, label="収束しきい値 (1e-8)")
    ax2.set_xlabel("反復回数", fontsize=12)
    ax2.set_ylabel("||Δω||", fontsize=12)
    ax2.set_title("LM法 ステップサイズ推移", fontsize=13)
    ax2.set_yscale("log")
    ax2.set_xlim(left=0)
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.4)
    plt.tight_layout()
    out2 = os.path.join(out_dir, "convergence_norm.png")
    fig2.savefig(out2, dpi=150, bbox_inches="tight")
    print(f"保存: {out2}")

    # ── グラフ3: サマリ表 ────────────────────────────────────────────
    col_names = ["初期値 [°]", "反復数", "最終 E", "Frobenius誤差", "収束"]
    rows = []
    for cas in cases:
        conv_str = "◎ 収束" if cas["converged"] else ("△ 局所解" if cas["frob"] and cas["frob"] < 0.5 else "✗ 未収束")
        rows.append([
            f"({cas['init'][0]:.0f}, {cas['init'][1]:.0f}, {cas['init'][2]:.0f})",
            str(len(cas["iters"])),
            f"{cas['final_E']:.4f}" if cas["final_E"] else "-",
            f"{cas['frob']:.6f}"    if cas["frob"]    else "-",
            conv_str,
        ])

    fig3, ax3 = plt.subplots(figsize=(10, 1.0 + 0.6 * n))
    ax3.axis("off")
    tbl = ax3.table(cellText=rows, colLabels=col_names,
                    loc="center", cellLoc="center")
    tbl.auto_set_font_size(False)
    tbl.set_fontsize(11)
    tbl.scale(1.0, 2.2)

    for j in range(len(col_names)):
        tbl[0, j].set_facecolor("#1f77b4")
        tbl[0, j].set_text_props(color="white", fontweight="bold")

    conv_col = col_names.index("収束")
    frob_col = col_names.index("Frobenius誤差")
    for i, cas in enumerate(cases):
        if cas["converged"] and cas["frob"] and cas["frob"] < 0.01:
            tbl[i+1, conv_col].set_facecolor("#c8e6c9")
            tbl[i+1, frob_col].set_facecolor("#c8e6c9")
        elif not cas["converged"]:
            tbl[i+1, conv_col].set_facecolor("#ffcdd2")
        else:
            tbl[i+1, conv_col].set_facecolor("#fff9c4")

    ax3.set_title("LM法 推定結果サマリ", fontsize=13, pad=20)
    plt.tight_layout()
    out3 = os.path.join(out_dir, "summary_table.png")
    fig3.savefig(out3, dpi=150, bbox_inches="tight")
    print(f"保存: {out3}")

    plt.close("all")

    # ── テキストサマリ ───────────────────────────────────────────────
    print("\n=== 結果サマリ ===")
    for cas in cases:
        print(f"  {cas['label']}")
        print(f"    反復数: {len(cas['iters'])}  最終E: {cas['final_E']:.4f}"
              f"  Frob誤差: {cas['frob']:.6f}  {'収束' if cas['converged'] else '未収束/局所解'}")


if __name__ == "__main__":
    main()
