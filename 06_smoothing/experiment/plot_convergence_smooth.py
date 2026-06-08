"""
plot_convergence_smooth.py
平滑化あり（ref画像のみ / TEO方式）の収束域を表示する
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

CSV_INPUT  = "results/convergence_smooth.csv"
OUTPUT_PNG = "results/convergence_smooth.png"
TITLE      = "平滑化あり（ref画像のみ）収束域"


def main():
    if not os.path.exists(CSV_INPUT):
        print(f"CSVが見つかりません: {CSV_INPUT}")
        return

    df = pd.read_csv(CSV_INPUT)

    w1_vals = sorted(df["w1_init"].unique())
    w2_vals = sorted(df["w2_init"].unique())
    w1_idx = {v: i for i, v in enumerate(w1_vals)}
    w2_idx = {v: i for i, v in enumerate(w2_vals)}
    Z = np.zeros((len(w2_vals), len(w1_vals)))
    for _, row in df.iterrows():
        i = w2_idx[row["w2_init"]]
        j = w1_idx[row["w1_init"]]
        Z[i, j] = row["success"]

    fig, ax = plt.subplots(figsize=(9, 7))

    im = ax.imshow(
        Z, origin="lower", aspect="auto",
        extent=[float(min(w1_vals)), float(max(w1_vals)),
                float(min(w2_vals)), float(max(w2_vals))],
        cmap="RdYlGn", vmin=0, vmax=1
    )
    ax.axvline(x=15.0, color="blue", linewidth=1.5, linestyle="--", label="真値 ω1=15°")
    ax.axhline(y=30.0, color="cyan", linewidth=1.5, linestyle="--", label="真値 ω2=30°")
    ax.set_xlabel("ω1 初期値 [deg]")
    ax.set_ylabel("ω2 初期値 [deg]")
    ax.set_title(TITLE)
    ax.legend(loc="upper right", fontsize=9)
    fig.colorbar(im, ax=ax, label="収束成功 (1=成功, 0=失敗)")

    success = df["success"].sum()
    total   = len(df)
    fig.suptitle(
        f"{TITLE}  |  {success}/{total} 収束 ({100 * success // total}%)",
        fontsize=12
    )

    plt.tight_layout()
    plt.savefig(OUTPUT_PNG, dpi=150)
    print(f"保存: {OUTPUT_PNG}")
    print(f"  収束: {success}/{total} ({100 * success // total}%)")


if __name__ == "__main__":
    main()
