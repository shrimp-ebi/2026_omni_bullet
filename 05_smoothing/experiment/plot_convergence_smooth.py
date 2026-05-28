"""
plot_convergence_smooth.py
scan_convergence_smooth.py の結果を2Dヒートマップで可視化する
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

CSV_PATH = "results/convergence_smooth.csv"
OUTPUT_PNG = "results/convergence_smooth_map.png"

def main():
    df = pd.read_csv(CSV_PATH)

    w1_vals = sorted(df["w1_init"].unique())
    w2_vals = sorted(df["w2_init"].unique())

    # 収束マップ（success = 1 なら白、0 なら黒）
    Z = np.zeros((len(w2_vals), len(w1_vals)))
    w1_idx = {v: i for i, v in enumerate(w1_vals)}
    w2_idx = {v: i for i, v in enumerate(w2_vals)}

    for _, row in df.iterrows():
        i = w2_idx[row["w2_init"]]
        j = w1_idx[row["w1_init"]]
        Z[i, j] = row["success"]

    fig, ax = plt.subplots(figsize=(10, 8))
    im = ax.imshow(
        Z, origin="lower", aspect="auto",
        extent=[min(w1_vals), max(w1_vals),
                min(w2_vals), max(w2_vals)],
        cmap="RdYlGn", vmin=0, vmax=1
    )

    # 真値の位置に十字マーク
    ax.axvline(x=15.0, color="blue", linewidth=1.5, linestyle="--", label="真値 ω1=15°")
    ax.axhline(y=30.0, color="cyan", linewidth=1.5, linestyle="--", label="真値 ω2=30°")

    plt.colorbar(im, ax=ax, label="収束成功 (1=成功, 0=失敗)")
    ax.set_xlabel("ω1 初期値 [deg]")
    ax.set_ylabel("ω2 初期値 [deg]")
    ax.set_title("平滑化あり LM法 収束域マップ (真値: ω1=15°, ω2=30°)")
    ax.legend(loc="upper right")

    plt.tight_layout()
    plt.savefig(OUTPUT_PNG, dpi=150)
    print(f"保存: {OUTPUT_PNG}")

if __name__ == "__main__":
    main()
