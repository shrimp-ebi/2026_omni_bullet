"""
plot_convergence_smooth.py
平滑化方式の違いによる収束域を2枚並べて比較する
"""

import pandas as pd
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import os

# 日本語フォント設定
font_candidates = [
    "/usr/share/fonts/truetype/fonts-japanese-gothic.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
]
for fp in font_candidates:
    if os.path.exists(fp):
        fm.fontManager.addfont(fp)
        prop = fm.FontProperties(fname=fp)
        plt.rcParams["font.family"] = prop.get_name()
        break

CSV_SMOOTH = "results/convergence_smooth.csv"
CSV_TEO    = "results/convergence_smooth_teo.csv"
OUTPUT_PNG = "results/convergence_smooth_compare.png"


def build_map(df):
    w1_vals = sorted(df["w1_init"].unique())
    w2_vals = sorted(df["w2_init"].unique())
    w1_idx = {v: i for i, v in enumerate(w1_vals)}
    w2_idx = {v: i for i, v in enumerate(w2_vals)}
    Z = np.zeros((len(w2_vals), len(w1_vals)))
    for _, row in df.iterrows():
        i = w2_idx[row["w2_init"]]
        j = w1_idx[row["w1_init"]]
        Z[i, j] = row["success"]
    return Z, w1_vals, w2_vals


def plot_panel(ax, Z, w1_vals, w2_vals, title):
    im = ax.imshow(
        Z, origin="lower", aspect="auto",
        extent=[float(min(w1_vals)), float(max(w1_vals)),
                float(min(w2_vals)), float(max(w2_vals))],
        cmap="RdYlGn", vmin=0, vmax=1
    )
    ax.axvline(x=15.0, color="blue",  linewidth=1.5, linestyle="--", label="真値 ω1=15°")
    ax.axhline(y=30.0, color="cyan",  linewidth=1.5, linestyle="--", label="真値 ω2=30°")
    ax.set_xlabel("ω1 初期値 [deg]")
    ax.set_ylabel("ω2 初期値 [deg]")
    ax.set_title(title)
    ax.legend(loc="upper right", fontsize=8)
    return im


def main():
    for path in [CSV_SMOOTH, CSV_TEO]:
        if not os.path.exists(path):
            print(f"CSVが見つかりません: {path}")
            return

    df_smooth = pd.read_csv(CSV_SMOOTH)
    df_teo    = pd.read_csv(CSV_TEO)

    Z_smooth, w1_s, w2_s = build_map(df_smooth)
    Z_teo,    w1_t, w2_t = build_map(df_teo)

    fig, axes = plt.subplots(1, 2, figsize=(18, 7))

    im1 = plot_panel(axes[0], Z_smooth, w1_s, w2_s,
                     "base+ref 両方平滑化 (lm_estimate_single)")
    im2 = plot_panel(axes[1], Z_teo,    w1_t, w2_t,
                     "ref のみ平滑化 / TEO方式 (lm_estimate_single_teo)")

    fig.colorbar(im1, ax=axes[0], label="収束成功 (1=成功, 0=失敗)")
    fig.colorbar(im2, ax=axes[1], label="収束成功 (1=成功, 0=失敗)")

    success_smooth = df_smooth["success"].sum()
    success_teo    = df_teo["success"].sum()
    total_smooth   = len(df_smooth)
    total_teo      = len(df_teo)
    fig.suptitle(
        f"平滑化方式比較  |  "
        f"両方平滑化: {success_smooth}/{total_smooth} 収束  |  "
        f"TEO方式: {success_teo}/{total_teo} 収束",
        fontsize=13
    )

    plt.tight_layout()
    plt.savefig(OUTPUT_PNG, dpi=150)
    print(f"保存: {OUTPUT_PNG}")
    print(f"  両方平滑化: {success_smooth}/{total_smooth} ({100*success_smooth//total_smooth}%) 収束")
    print(f"  TEO方式:    {success_teo}/{total_teo} ({100*success_teo//total_teo}%) 収束")


if __name__ == "__main__":
    main()
