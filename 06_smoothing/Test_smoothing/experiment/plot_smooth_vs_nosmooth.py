#!/usr/bin/env python3
"""
plot_smooth_vs_nosmooth.py
平滑化なし vs 平滑化あり（σ=1.0）の収束域を2枚並べて比較する。
"""

import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import matplotlib.patches as mpatches
import numpy as np
import pandas as pd

matplotlib.rcParams["font.family"] = "Noto Sans CJK JP"

PANELS = [
    ("results/convergence_no_smooth.csv", "平滑化なし"),
    ("results/convergence_sigma100.csv",  "平滑化あり (σ=1.0)"),
]
OUTPUT_PNG = "results/smooth_vs_nosmooth.png"

TRUE_W1 = 15.0
TRUE_W2 = 30.0


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
    missing = [p for p, _ in PANELS if not os.path.exists(p)]
    if missing:
        print("エラー: 以下のCSVが見つかりません:")
        for p in missing:
            print(f"  {p}")
        sys.exit(1)

    cmap = matplotlib.colors.LinearSegmentedColormap.from_list(
        "rg", ["#e74c3c", "#2ecc71"]
    )
    norm = matplotlib.colors.Normalize(vmin=0, vmax=1)

    fig, axes = plt.subplots(1, 2, figsize=(10, 5), sharey=True)

    im_ref = None
    for ax, (csv_path, label) in zip(axes, PANELS):
        grid, w1_vals, w2_vals, n_success, n_total = load_grid(csv_path)

        step_w1 = w1_vals[1] - w1_vals[0] if len(w1_vals) > 1 else 1.0
        step_w2 = w2_vals[1] - w2_vals[0] if len(w2_vals) > 1 else 1.0
        w1_edges = [v - step_w1 / 2 for v in w1_vals] + [w1_vals[-1] + step_w1 / 2]
        w2_edges = [v - step_w2 / 2 for v in w2_vals] + [w2_vals[-1] + step_w2 / 2]

        im = ax.pcolormesh(w1_edges, w2_edges, grid, cmap=cmap, norm=norm,
                           edgecolors="white", linewidth=0.5)
        im_ref = im

        ax.axvline(TRUE_W1, color="blue", linestyle="--", linewidth=1.5)
        ax.axhline(TRUE_W2, color="cyan", linestyle="--", linewidth=1.5)

        ax.set_title(f"{label}\n({n_success}/{n_total} 収束)", fontsize=11)
        ax.set_xlabel("ω1 初期値 [deg]", fontsize=10)
        ax.set_xticks(w1_vals)
        ax.tick_params(axis="x", labelsize=9)
        ax.tick_params(axis="y", labelsize=9)

    axes[0].set_ylabel("ω2 初期値 [deg]", fontsize=10)

    # 共通カラーバー
    fig.subplots_adjust(right=0.86, wspace=0.08)
    cbar_ax = fig.add_axes([0.88, 0.15, 0.025, 0.70])
    cbar = fig.colorbar(im_ref, cax=cbar_ax)
    cbar.set_ticks([0, 1])
    cbar.set_ticklabels(["失敗", "成功"])
    cbar.ax.tick_params(labelsize=9)

    # 凡例
    patch_ok = mpatches.Patch(color="#2ecc71", label="収束成功")
    patch_ng = mpatches.Patch(color="#e74c3c", label="収束失敗")
    line_w1  = mlines.Line2D([], [], color="blue", linestyle="--", label=f"真値 ω1={TRUE_W1:.0f}°")
    line_w2  = mlines.Line2D([], [], color="cyan", linestyle="--", label=f"真値 ω2={TRUE_W2:.0f}°")
    fig.legend(handles=[patch_ok, patch_ng, line_w1, line_w2],
               loc="lower center", ncol=4, fontsize=9,
               bbox_to_anchor=(0.43, 0.01))

    fig.suptitle("平滑化あり vs 平滑化なし 収束域比較", fontsize=13, y=1.02)

    os.makedirs("results", exist_ok=True)
    plt.savefig(OUTPUT_PNG, dpi=150, bbox_inches="tight")
    print(f"保存完了: {OUTPUT_PNG}")
    plt.close()


if __name__ == "__main__":
    main()
