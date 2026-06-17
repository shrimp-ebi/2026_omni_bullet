#!/usr/bin/env python3
"""
plot_convergence_cases.py
3ケースLM法収束実験のグラフ作成

グラフ1: 反復回数 vs 目的関数E（収束曲線）
グラフ2: 各ケースの推定結果と真値の比較表
"""

import csv
import math
import os
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np

# ── 日本語フォントを自動検出 ─────────────────────────────────────
def find_japanese_font():
    """システムから日本語対応フォントを探す"""
    candidates = [
        "Noto Sans CJK JP", "Noto Sans JP",
        "IPAGothic", "IPAPGothic", "IPAexGothic",
        "TakaoGothic", "VL Gothic",
        "Hiragino Sans", "Hiragino Kaku Gothic Pro",
        "Yu Gothic", "Meiryo",
    ]
    available = {f.name for f in fm.fontManager.ttflist}
    for name in candidates:
        if name in available:
            return name
    # ファイル名でも探す
    keywords = ["noto", "ipa", "takao", "gothic", "hiragino", "meiryo", "yugothic"]
    for f in fm.fontManager.ttflist:
        for kw in keywords:
            if kw in f.fname.lower():
                return f.name
    return None

font_name = find_japanese_font()
if font_name:
    matplotlib.rcParams["font.family"] = font_name
    print(f"日本語フォント使用: {font_name}")
else:
    print("警告: 日本語フォントが見つかりません。文字化けする可能性があります。")

# ── データ読み込み ────────────────────────────────────────────────
RESULTS_DIR = "results"
CONV_CSV  = os.path.join(RESULTS_DIR, "lm_convergence.csv")
SUM_CSV   = os.path.join(RESULTS_DIR, "lm_summary.csv")
OUT_DIR   = os.path.join(RESULTS_DIR, "graphs_lm_cases")
os.makedirs(OUT_DIR, exist_ok=True)

# 収束CSVの読み込み（複数ケースが混在）
cases_conv = {}   # init_deg -> list of (iter, E)
with open(CONV_CSV, "r", encoding="utf-8") as f:
    current_case = None
    for line in f:
        line = line.strip()
        if not line:
            continue
        if line.startswith("# case="):
            current_case = float(line.split("=")[1])
            cases_conv[current_case] = []
        elif line.startswith("case,iter"):
            continue  # ヘッダー行をスキップ
        else:
            parts = line.split(",")
            if len(parts) >= 3:
                try:
                    c = float(parts[0])
                    it = int(parts[1])
                    e = float(parts[2])
                    if c not in cases_conv:
                        cases_conv[c] = []
                    cases_conv[c].append((it, e))
                except ValueError:
                    pass

# サマリCSVの読み込み
summary = {}  # init_deg -> dict
with open(SUM_CSV, "r", encoding="utf-8") as f:
    reader = csv.DictReader(f)
    for row in reader:
        key = float(row["init_deg"])
        summary[key] = {
            "init_deg": float(row["init_deg"]),
            "iter": int(row["iter"]),
            "final_E": float(row["final_E"]),
            "frob_err": float(row["frob_err"]),
            "R": [[float(row[f"R{i}{j}"]) for j in range(3)] for i in range(3)],
        }

# 理論値 R_Y(-30°)
RTH = [
    [ 0.866025, 0.0, -0.500000],
    [ 0.0,      1.0,  0.0     ],
    [ 0.500000, 0.0,  0.866025],
]

# Y軸回転角 omega2 を推定
# rodrigues({0, omega2, 0}) では R[0][2] = sin(omega2) なので
# omega2 = asin(R[0][2])
def estimate_angle_deg(R):
    val = max(-1.0, min(1.0, R[0][2]))  # clamp
    return math.degrees(math.asin(val))

# ケース設定
case_keys = sorted(cases_conv.keys())
case_labels = {k: f"初期値 R_Y({k:.0f}°)" for k in case_keys}
colors = ["#1f77b4", "#ff7f0e", "#2ca02c"]

# ── グラフ1: 収束曲線 ────────────────────────────────────────────
fig1, ax1 = plt.subplots(figsize=(10, 6))

for idx, key in enumerate(case_keys):
    data = cases_conv[key]
    iters = [d[0] for d in data]
    Es    = [d[1] for d in data]
    ax1.plot(iters, Es, marker="o", markersize=4,
             label=case_labels[key], color=colors[idx], linewidth=1.8)

ax1.set_xlabel("反復回数", fontsize=13)
ax1.set_ylabel("目的関数 E", fontsize=13)
ax1.set_title("LM法 収束曲線\n(真値: R_Y(−30°)、参照画像: reference_30deg.jpg)", fontsize=14)
ax1.legend(fontsize=11)
ax1.grid(True, alpha=0.4)
ax1.set_yscale("log")
ax1.set_xlim(left=0)

# 最終Eを注釈
for idx, key in enumerate(case_keys):
    s = summary[key]
    last_iter = s["iter"]
    last_E    = s["final_E"]
    ax1.annotate(
        f"E={last_E:.2f}",
        xy=(last_iter, last_E),
        xytext=(last_iter + 1, last_E * 1.5),
        fontsize=9,
        color=colors[idx],
        arrowprops=dict(arrowstyle="->", color=colors[idx], lw=1.0),
    )

plt.tight_layout()
out1 = os.path.join(OUT_DIR, "convergence_curve.png")
fig1.savefig(out1, dpi=150, bbox_inches="tight")
print(f"グラフ1保存: {out1}")

# ── グラフ2: 推定結果 vs 真値の比較表 ───────────────────────────
# 表の内容：初期値、収束後E、Frobenius誤差、推定角度、理論角度との差

true_angle = estimate_angle_deg(RTH)
rows = []
for key in case_keys:
    s = summary[key]
    est_angle = estimate_angle_deg(s["R"])
    rows.append({
        "初期値 [°]": f"{key:.0f}",
        "反復回数": s["iter"],
        "最終E": f"{s['final_E']:.4f}",
        "推定角度 [°]": f"{est_angle:.4f}",
        "角度誤差 [°]": f"{abs(est_angle - true_angle):.4f}",
        "Frobenius誤差": f"{s['frob_err']:.6f}",
        "収束成否": "◎ 収束" if s["frob_err"] < 0.01 else "NG 局所最小",
    })

col_names = list(rows[0].keys())
cell_data = [[r[c] for c in col_names] for r in rows]

fig2, ax2 = plt.subplots(figsize=(13, 3.5))
ax2.axis("off")

tbl = ax2.table(
    cellText=cell_data,
    colLabels=col_names,
    loc="center",
    cellLoc="center",
)
tbl.auto_set_font_size(False)
tbl.set_fontsize(10)
tbl.scale(1.0, 2.5)

# ヘッダーを青でハイライト
for j in range(len(col_names)):
    tbl[0, j].set_facecolor("#1f77b4")
    tbl[0, j].set_text_props(color="white", fontweight="bold")

# 収束成功セルを緑、失敗を赤
for i, row in enumerate(rows):
    if row["収束成否"].startswith("◎"):
        tbl[i+1, col_names.index("収束成否")].set_facecolor("#c8e6c9")
    else:
        tbl[i+1, col_names.index("収束成否")].set_facecolor("#ffcdd2")

ax2.set_title(
    f"推定結果と真値の比較\n（真値: R_Y(−30°)、理論角度 = {true_angle:.4f}°）",
    fontsize=13, pad=20
)

plt.tight_layout()
out2 = os.path.join(OUT_DIR, "comparison_table.png")
fig2.savefig(out2, dpi=150, bbox_inches="tight")
print(f"グラフ2保存: {out2}")

# ── 追加グラフ: 推定回転行列の可視化 ────────────────────────────
fig3, axes = plt.subplots(1, len(case_keys) + 1, figsize=(14, 4))
all_cases = list(summary.values()) + [{"init_deg": "真値", "R": RTH, "iter": "-", "final_E": 0.0}]
titles_r   = [case_labels[k] for k in case_keys] + ["真値 R_Y(−30°)"]

vmin, vmax = -1.0, 1.0
for ax, s, title in zip(axes, all_cases, titles_r):
    R_arr = np.array(s["R"])
    im = ax.imshow(R_arr, vmin=vmin, vmax=vmax, cmap="coolwarm", aspect="equal")
    ax.set_title(title, fontsize=10)
    ax.set_xticks([0, 1, 2])
    ax.set_yticks([0, 1, 2])
    ax.set_xticklabels(["列1", "列2", "列3"], fontsize=8)
    ax.set_yticklabels(["行1", "行2", "行3"], fontsize=8)
    for i in range(3):
        for j in range(3):
            ax.text(j, i, f"{R_arr[i, j]:.3f}", ha="center", va="center",
                    fontsize=8, color="black")

fig3.colorbar(im, ax=axes[-1], fraction=0.046, pad=0.04)
fig3.suptitle("推定回転行列の比較（各ケースと真値）", fontsize=13)
plt.tight_layout()
out3 = os.path.join(OUT_DIR, "rotation_matrix_comparison.png")
fig3.savefig(out3, dpi=150, bbox_inches="tight")
print(f"グラフ3保存: {out3}")

plt.close("all")
print("\n=== 実験結果サマリ ===")
print(f"{'初期値':>10}  {'反復数':>6}  {'最終E':>14}  {'推定角度':>10}  {'角度誤差':>10}  {'Frob誤差':>12}  結果")
for key in case_keys:
    s = summary[key]
    est = estimate_angle_deg(s["R"])
    conv = "◎収束" if s["frob_err"] < 0.01 else "NG局所最小"
    print(f"  {key:>7.1f}°  {s['iter']:>6}  {s['final_E']:>14.4f}  "
          f"{est:>10.4f}°  {abs(est-true_angle):>10.4f}°  {s['frob_err']:>12.6f}  {conv}")
print(f"\n真値角度: {true_angle:.4f}°")
