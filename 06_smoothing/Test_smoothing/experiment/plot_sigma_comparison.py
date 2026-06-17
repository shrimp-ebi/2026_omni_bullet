#!/usr/bin/env python3
"""
plot_sigma_comparison.py
複数σの収束域ヒートマップを横1行5枚で比較表示する。
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

SIGMA_LIST = [0.3, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 7.0, 10.0]
OUTPUT_PNG = "results/sigma_comparison.png"

TRUE_W1 = 15.0
TRUE_W2 = 30.0


def sigma_to_tag(sigma):
    return f"{int(round(sigma * 100)):03d}"


def input_csv(sigma):
    return f"results/convergence_sigma{sigma_to_tag(sigma)}.csv"


def load_grid(path):
    df = pd.read_csv(path)
    w1_vals = sorted(df["w1_init"].unique())
    w2_vals = sorted(df["w2_init"].unique())

    grid = np.zeros((len(w2_vals), len(w1_vals)))
    w1_idx = {v: i for i, v in enumerate(w1_vals)}
    w2_idx = {v: i for i, v in enumerate(w2_vals)}

    for _, row in df.iterrows():
        i = w2_idx.get(row["w2_init"])
        j = w1_idx.get(row["w1_init"])
        if i is not None and j is not None:
            try:
                grid[i, j] = float(row["success"])
            except (ValueError, TypeError):
                grid[i, j] = 0.0

    n_success = int(df["success"].astype(float).sum())
    n_total   = len(df)
    return grid, w1_vals, w2_vals, n_success, n_total


def main():
    missing = [s for s in SIGMA_LIST if not os.path.exists(input_csv(s))]
    if missing:
        print("エラー: 以下のCSVが見つかりません:")
        for s in missing:
            print(f"  {input_csv(s)}")
        sys.exit(1)

    n = len(SIGMA_LIST)
    fig, axes = plt.subplots(1, n, figsize=(4 * 9, 5), sharey=True)

    cmap = matplotlib.colors.LinearSegmentedColormap.from_list(
        "rg", ["#e74c3c", "#2ecc71"]
    )
    norm = matplotlib.colors.Normalize(vmin=0, vmax=1)

    im_ref = None
    for ax, sigma in zip(axes, SIGMA_LIST):
        grid, w1_vals, w2_vals, n_success, n_total = load_grid(input_csv(sigma))

        step_w1 = w1_vals[1] - w1_vals[0] if len(w1_vals) > 1 else 1.0
        step_w2 = w2_vals[1] - w2_vals[0] if len(w2_vals) > 1 else 1.0
        w1_edges = [v - step_w1 / 2 for v in w1_vals] + [w1_vals[-1] + step_w1 / 2]
        w2_edges = [v - step_w2 / 2 for v in w2_vals] + [w2_vals[-1] + step_w2 / 2]

        im = ax.pcolormesh(w1_edges, w2_edges, grid, cmap=cmap, norm=norm,
                           edgecolors="white", linewidth=0.4)
        im_ref = im

        ax.axvline(TRUE_W1, color="blue",  linestyle="--", linewidth=1.2)
        ax.axhline(TRUE_W2, color="cyan",  linestyle="--", linewidth=1.2)

        ax.set_title(f"σ = {sigma}\n({n_success}/{n_total} 収束)", fontsize=10)
        ax.set_xlabel("ω1 初期値 [deg]", fontsize=9)
        ax.set_xticks(w1_vals[::2] if len(w1_vals) > 6 else w1_vals)
        ax.tick_params(axis="x", labelsize=8)

    axes[0].set_ylabel("ω2 初期値 [deg]", fontsize=9)

    # 共通カラーバー
    fig.subplots_adjust(right=0.88, wspace=0.08)
    cbar_ax = fig.add_axes([0.90, 0.15, 0.02, 0.70])
    cbar = fig.colorbar(im_ref, cax=cbar_ax)
    cbar.set_ticks([0, 1])
    cbar.set_ticklabels(["失敗", "成功"])
    cbar.ax.tick_params(labelsize=9)

    # 凡例（図全体の下）
    patch_ok = mpatches.Patch(color="#2ecc71", label="収束成功")
    patch_ng = mpatches.Patch(color="#e74c3c", label="収束失敗")
    line_w1  = matplotlib.lines.Line2D([], [], color="blue",  linestyle="--", label=f"真値 ω1={TRUE_W1:.0f}°")
    line_w2  = matplotlib.lines.Line2D([], [], color="cyan",  linestyle="--", label=f"真値 ω2={TRUE_W2:.0f}°")
    fig.legend(handles=[patch_ok, patch_ng, line_w1, line_w2],
               loc="lower center", ncol=4, fontsize=9,
               bbox_to_anchor=(0.45, 0.01))

    fig.suptitle("B方式平滑化 LM法 収束域 — σ比較", fontsize=13, y=1.02)

    os.makedirs("results", exist_ok=True)
    plt.savefig(OUTPUT_PNG, dpi=150, bbox_inches="tight")
    print(f"保存完了: {OUTPUT_PNG}")
    plt.close()


if __name__ == "__main__":
    main()
