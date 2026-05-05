"""plot_scan_2d.py
ω₁-ω₂ 2次元スキャン結果を3次元曲面グラフおよびヒートマップとして描画する。

入力 : results/scan_2d.csv  (omega1_deg, omega2_deg, E)
出力 : results/scan_2d_surface.png
       results/scan_2d_heatmap.png
"""

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
matplotlib.rcParams['font.family'] = 'IPAexGothic'
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401
import seaborn as sns

# ── データ読み込み ───────────────────────────────────────────
df = pd.read_csv("results/scan_2d.csv")

omega1_vals = np.sort(df["omega1_deg"].unique())
omega2_vals = np.sort(df["omega2_deg"].unique())

W2, W1 = np.meshgrid(omega2_vals, omega1_vals, indexing="ij")
# W2.shape = W1.shape = (n_omega2, n_omega1)
# axis 0 → omega2 方向（plot_surface の Y 軸・奥行き）
# axis 1 → omega1 方向（plot_surface の X 軸・横）

E_grid = np.full(W1.shape, np.nan)
for _, row in df.iterrows():
    i_w1 = np.searchsorted(omega1_vals, row["omega1_deg"])  # axis 1 方向
    i_w2 = np.searchsorted(omega2_vals, row["omega2_deg"])  # axis 0 方向
    E_grid[i_w2, i_w1] = row["E"]

# 対数スケール（log10）
logE_grid = np.log10(E_grid)

# ── 真値の位置 ───────────────────────────────────────────────
TRUE_W1 = 15.0
TRUE_W2 = 30.0
true_logE = np.log10(df.loc[
    (df["omega1_deg"] == TRUE_W1) & (df["omega2_deg"] == TRUE_W2), "E"
].values[0])

# ── 描画 ────────────────────────────────────────────────────
fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection="3d")

surf = ax.plot_surface(
    W1, W2, logE_grid,
    cmap="coolwarm",
    alpha=0.85,
    linewidth=0,
    antialiased=True,
)

# 真値に赤い縦線（最小値床から曲面上まで）
z_floor = np.nanmin(logE_grid)
ax.plot(
    [TRUE_W1, TRUE_W1],
    [TRUE_W2, TRUE_W2],
    [z_floor, true_logE],
    color="red",
    linewidth=2.5,
    zorder=10,
    label=f"真値 (omega1={TRUE_W1}°, omega2={TRUE_W2}°)",
)
ax.scatter(
    [TRUE_W1], [TRUE_W2], [true_logE],
    color="red",
    s=80,
    zorder=11,
)

# カラーバー
cbar = fig.colorbar(surf, ax=ax, shrink=0.5, aspect=12, pad=0.1)
cbar.set_label("log10(E)", fontsize=11)

# ラベル・タイトル
ax.set_xlabel("omega1 [deg]", fontsize=12, labelpad=8)
ax.set_ylabel("omega2 [deg]", fontsize=12, labelpad=8)
ax.set_zlabel("log10(E)", fontsize=12, labelpad=8)
ax.set_title("目的関数 E（omega1-omega2平面、omega3=0°固定）", fontsize=13, pad=14)

ax.legend(loc="upper left", fontsize=10)
ax.view_init(elev=30, azim=-60)

# ── 保存 ────────────────────────────────────────────────────
out_path = "results/scan_2d_surface.png"
plt.tight_layout()
plt.savefig(out_path, dpi=150, bbox_inches="tight")
plt.close()
print(f"保存完了: {out_path}")

# ── 2D ヒートマップ ──────────────────────────────────────────
# seaborn.heatmap 用に DataFrame へ変換
# 行 = omega2（上が大きい値になるよう index を降順に並べる）
# 列 = omega1
heat_df = pd.DataFrame(
    logE_grid,                          # shape (n_omega2, n_omega1)
    index=omega2_vals,
    columns=omega1_vals,
)
heat_df = heat_df.iloc[::-1]           # omega2 を上から大きい順に表示

fig2, ax2 = plt.subplots(figsize=(8, 6))

sns.heatmap(
    heat_df,
    ax=ax2,
    cmap="coolwarm",
    cbar_kws={"label": "log10(E)"},
)

# 真値に赤い×印
# heatmap の座標系はセル番号（0始まり）なので変換する
true_col = np.searchsorted(omega1_vals, TRUE_W1) + 0.5   # omega1 軸方向
true_row = (len(omega2_vals) - 1 - np.searchsorted(omega2_vals, TRUE_W2)) + 0.5  # 反転後の行位置
ax2.scatter(
    true_col, true_row,
    marker="x", s=200, linewidths=3, color="red",
    zorder=5, label=f"真値 (omega1={TRUE_W1}°, omega2={TRUE_W2}°)",
)
ax2.legend(loc="upper right", fontsize=10)

ax2.set_title("目的関数 E ヒートマップ（omega3=0°固定）", fontsize=13)
ax2.set_xlabel("omega1 [deg]", fontsize=12)
ax2.set_ylabel("omega2 [deg]", fontsize=12)

# 軸目盛りをセル番号から角度に変換（5刻みで表示）
step = 5
w1_ticks = np.arange(0, len(omega1_vals), step)
w2_ticks = np.arange(0, len(omega2_vals), step)
ax2.set_xticks(w1_ticks + 0.5)
ax2.set_xticklabels([f"{omega1_vals[t]:.0f}" for t in w1_ticks], rotation=0)
ax2.set_yticks(w2_ticks + 0.5)
ax2.set_yticklabels([f"{omega2_vals[len(omega2_vals) - 1 - t]:.0f}" for t in w2_ticks])

out_heatmap = "results/scan_2d_heatmap.png"
plt.tight_layout()
plt.savefig(out_heatmap, dpi=150, bbox_inches="tight")
plt.close()
print(f"保存完了: {out_heatmap}")
