#!/usr/bin/env python3
"""
plot_convergence_smooth.py
B方式平滑化LM法の収束域ヒートマップ描画

INPUT_CSV の結果を読み込み、ω1-ω2 平面上の収束/失敗をヒートマップで表示する。
"""

import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
import pandas as pd

matplotlib.rcParams["font.family"] = "Noto Sans CJK JP"

INPUT_CSV  = "results/convergence_smooth.csv"
OUTPUT_PNG = "results/convergence_smooth.png"

TRUE_W1 = 15.0
TRUE_W2 = 30.0


def main():
    if not os.path.exists(INPUT_CSV):
        print(f"エラー: {INPUT_CSV} が見つかりません。先にスキャンを実行してください。")
        sys.exit(1)

    df = pd.read_csv(INPUT_CSV)

    w1_vals = sorted(df["w1_init"].unique())
    w2_vals = sorted(df["w2_init"].unique())

    # ヒートマップ用配列（行=ω2、列=ω1）
    success_grid = np.zeros((len(w2_vals), len(w1_vals)))
    w1_idx = {v: i for i, v in enumerate(w1_vals)}
    w2_idx = {v: i for i, v in enumerate(w2_vals)}

    for _, row in df.iterrows():
        i = w2_idx.get(row["w2_init"])
        j = w1_idx.get(row["w1_init"])
        if i is not None and j is not None:
            try:
                success_grid[i, j] = float(row["success"])
            except (ValueError, TypeError):
                success_grid[i, j] = 0.0

    n_success = int(df["success"].astype(float).sum())
    n_total   = len(df)

    fig, ax = plt.subplots(figsize=(9, 7))

    # 緑=成功(1)、赤=失敗(0) のカラーマップ
    cmap = matplotlib.colors.ListedColormap(["#e74c3c", "#2ecc71"])
    bounds = [-0.5, 0.5, 1.5]
    norm = matplotlib.colors.BoundaryNorm(bounds, cmap.N)

    w1_edges = [v - 2.5 for v in w1_vals] + [w1_vals[-1] + 2.5]
    w2_edges = [v - 2.5 for v in w2_vals] + [w2_vals[-1] + 2.5]

    ax.pcolormesh(w1_edges, w2_edges, success_grid, cmap=cmap, norm=norm,
                  edgecolors="white", linewidth=0.5)

    # 真値位置の破線
    ax.axvline(TRUE_W1, color="blue",  linestyle="--", linewidth=1.5,
               label=f"真値 ω1 = {TRUE_W1:.0f}°")
    ax.axhline(TRUE_W2, color="cyan",  linestyle="--", linewidth=1.5,
               label=f"真値 ω2 = {TRUE_W2:.0f}°")

    # 凡例パッチ
    patch_ok  = mpatches.Patch(color="#2ecc71", label="収束成功")
    patch_ng  = mpatches.Patch(color="#e74c3c", label="収束失敗")
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles=handles + [patch_ok, patch_ng], fontsize=9, loc="upper right")

    ax.set_xlabel("ω1 初期値 [deg]", fontsize=12)
    ax.set_ylabel("ω2 初期値 [deg]", fontsize=12)
    ax.set_title(
        f"B方式平滑化 LM法 収束域  ({n_success}/{n_total} 収束)",
        fontsize=13
    )
    ax.set_xticks(w1_vals)
    ax.set_yticks(w2_vals)

    plt.tight_layout()
    os.makedirs("results", exist_ok=True)
    plt.savefig(OUTPUT_PNG, dpi=150, bbox_inches="tight")
    print(f"保存完了: {OUTPUT_PNG}  (収束 {n_success}/{n_total})")
    plt.close()


if __name__ == "__main__":
    main()
