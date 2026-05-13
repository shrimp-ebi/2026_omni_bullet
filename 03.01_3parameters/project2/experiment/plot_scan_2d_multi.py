"""plot_scan_2d_multi.py
base2 / base3 の scan_2d.csv からグラフ6枚を生成する。

Usage:
    python3 experiment/plot_scan_2d_multi.py base2
    python3 experiment/plot_scan_2d_multi.py base3
"""

import sys
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
matplotlib.rcParams['font.family'] = 'IPAexGothic'
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401

TRUE_W1 = 15.0
TRUE_W2 = 30.0

def load_data(csv_path):
    df = pd.read_csv(csv_path)
    omega1_vals = np.sort(df["omega1_deg"].unique())
    omega2_vals = np.sort(df["omega2_deg"].unique())
    W2, W1 = np.meshgrid(omega2_vals, omega1_vals, indexing="ij")
    E_grid = np.full(W1.shape, np.nan)
    for _, row in df.iterrows():
        i_w1 = np.searchsorted(omega1_vals, row["omega1_deg"])
        i_w2 = np.searchsorted(omega2_vals, row["omega2_deg"])
        E_grid[i_w2, i_w1] = row["E"]
    return df, omega1_vals, omega2_vals, W1, W2, E_grid


def plot_surface(df, omega1_vals, omega2_vals, W1, W2, E_grid, out_path, label):
    logE_grid = np.log10(E_grid)
    true_logE = np.log10(df.loc[
        (df["omega1_deg"] == TRUE_W1) & (df["omega2_deg"] == TRUE_W2), "E"
    ].values[0])

    fig = plt.figure(figsize=(10, 7))
    ax = fig.add_subplot(111, projection="3d")
    surf = ax.plot_surface(W1, W2, logE_grid, cmap="coolwarm",
                           alpha=0.85, linewidth=0, antialiased=True)
    z_floor = np.nanmin(logE_grid)
    ax.plot([TRUE_W1, TRUE_W1], [TRUE_W2, TRUE_W2], [z_floor, true_logE],
            color="red", linewidth=2.5, zorder=10,
            label=f"真値 (omega1={TRUE_W1}°, omega2={TRUE_W2}°)")
    ax.scatter([TRUE_W1], [TRUE_W2], [true_logE], color="red", s=80, zorder=11)
    cbar = fig.colorbar(surf, ax=ax, shrink=0.5, aspect=12, pad=0.1)
    cbar.set_label("log10(E)", fontsize=11)
    ax.set_xlabel("omega1 [deg]", fontsize=12, labelpad=8)
    ax.set_ylabel("omega2 [deg]", fontsize=12, labelpad=8)
    ax.set_zlabel("log10(E)", fontsize=12, labelpad=8)
    ax.set_title(f"目的関数 E（{label}、omega3=0°固定）", fontsize=13, pad=14)
    ax.legend(loc="upper left", fontsize=10)
    ax.view_init(elev=30, azim=-60)
    plt.tight_layout()
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"保存完了: {out_path}")


def plot_heatmap(df, omega1_vals, omega2_vals, W1, W2, E_grid, out_path, label):
    logE_grid = np.log10(E_grid)
    pivot = df.pivot(index="omega2_deg", columns="omega1_deg", values="E")
    logE = np.log10(pivot.values)

    fig, ax = plt.subplots(figsize=(8, 6))
    im = ax.pcolormesh(pivot.columns, pivot.index, logE, cmap="coolwarm")
    plt.colorbar(im, ax=ax, label="log10(E)")
    ax.scatter([TRUE_W1], [TRUE_W2], color="red", marker="x",
               s=200, linewidth=3,
               label=f"真値 (omega1={TRUE_W1}°, omega2={TRUE_W2}°)")
    ax.set_xlabel("omega1 [deg]", fontsize=12)
    ax.set_ylabel("omega2 [deg]", fontsize=12)
    ax.set_title(f"目的関数 E ヒートマップ（{label}、omega3=0°固定）", fontsize=13)
    ax.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"保存完了: {out_path}")


def plot_gradient(df, omega1_vals, omega2_vals, out_path, label):
    pivot = df.pivot(index="omega2_deg", columns="omega1_deg", values="E")
    E = pivot.values
    h1 = omega1_vals[1] - omega1_vals[0]
    h2 = omega2_vals[1] - omega2_vals[0]
    dE_dw1 = np.gradient(E, h1, axis=1)
    dE_dw2 = np.gradient(E, h2, axis=0)
    grad_norm = np.sqrt(dE_dw1**2 + dE_dw2**2)
    log_grad = np.log10(grad_norm + 1e-10)
    W1g, W2g = np.meshgrid(omega1_vals, omega2_vals)

    fig = plt.figure(figsize=(12, 5))

    ax1 = fig.add_subplot(121, projection="3d")
    surf = ax1.plot_surface(W1g, W2g, log_grad, cmap="viridis",
                             alpha=0.85, linewidth=0, antialiased=True)
    fig.colorbar(surf, ax=ax1, label="log10(|∇E|)", shrink=0.5)
    ax1.set_xlabel("omega1 [deg]")
    ax1.set_ylabel("omega2 [deg]")
    ax1.set_zlabel("log10(|∇E|)")
    ax1.set_title(f"勾配の大きさ |∇E|（{label}）")
    idx1 = np.argmin(np.abs(omega1_vals - TRUE_W1))
    idx2 = np.argmin(np.abs(omega2_vals - TRUE_W2))
    ax1.scatter([TRUE_W1], [TRUE_W2], [log_grad[idx2, idx1]],
                color="red", s=80, zorder=10, label="真値")
    ax1.legend()

    ax2 = fig.add_subplot(122)
    im = ax2.pcolormesh(omega1_vals, omega2_vals, log_grad, cmap="viridis")
    fig.colorbar(im, ax=ax2, label="log10(|∇E|)")
    ax2.scatter([TRUE_W1], [TRUE_W2], color="red", marker="x",
                s=200, linewidth=3, label="真値")
    ax2.set_xlabel("omega1 [deg]")
    ax2.set_ylabel("omega2 [deg]")
    ax2.set_title(f"勾配ヒートマップ（{label}）")
    ax2.legend()

    plt.suptitle(f"目的関数Eの1次微分（勾配）— {label}", fontsize=14)
    plt.tight_layout()
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"保存完了: {out_path}")


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("base2", "base3"):
        print("Usage: python3 experiment/plot_scan_2d_multi.py base2|base3")
        sys.exit(1)

    tag = sys.argv[1]
    csv_path = f"results/scan_2d_{tag}.csv"

    df, omega1_vals, omega2_vals, W1, W2, E_grid = load_data(csv_path)

    plot_surface(df, omega1_vals, omega2_vals, W1, W2, E_grid,
                 f"results/scan_2d_surface_{tag}.png", tag)
    plot_heatmap(df, omega1_vals, omega2_vals, W1, W2, E_grid,
                 f"results/scan_2d_heatmap_{tag}.png", tag)
    plot_gradient(df, omega1_vals, omega2_vals,
                  f"results/gradient_2d_{tag}.png", tag)


if __name__ == "__main__":
    main()
