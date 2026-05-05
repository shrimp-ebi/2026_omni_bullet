"""
plot_scan_3param.py
3-parameter rotation scan: objective function and gradient comparison plots.

Run from project1/experiment/:
  python3 plot_scan_3param.py
"""

import os
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

matplotlib.rcParams["font.family"] = "sans-serif"
matplotlib.rcParams["font.sans-serif"] = [
    "Noto Sans CJK JP",
    "IPAexGothic",
    "IPAGothic",
    "TakaoGothic",
    "VL Gothic",
    "DejaVu Sans",
]
matplotlib.rcParams["axes.unicode_minus"] = False

# ── paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(SCRIPT_DIR, "..", "results")
GRAPH_DIR   = os.path.join(RESULTS_DIR, "graphs_3param")
os.makedirs(GRAPH_DIR, exist_ok=True)

CSV_FILES = [
    os.path.join(RESULTS_DIR, "scan_3param_omega1.csv"),
    os.path.join(RESULTS_DIR, "scan_3param_omega2.csv"),
    os.path.join(RESULTS_DIR, "scan_3param_omega3.csv"),
]
PARAM_LABELS = [r"$\omega_1$", r"$\omega_2$", r"$\omega_3$"]

# ── load data ─────────────────────────────────────────────────────────────────
dfs = []
for f in CSV_FILES:
    df = pd.read_csv(f)
    # clip extreme analytical-gradient values (pole singularity near ±90°)
    clip = 1e5
    df["grad_analytical"] = df["grad_analytical"].clip(-clip, clip)
    df["grad_numerical"]  = df["grad_numerical"].clip(-clip, clip)
    dfs.append(df)

# ── figure ────────────────────────────────────────────────────────────────────
fig, axes = plt.subplots(3, 2, figsize=(12, 12))
fig.suptitle("3パラメータ回転スキャン: 目的関数と微分", fontsize=14, y=1.01)

for row, (df, plabel) in enumerate(zip(dfs, PARAM_LABELS)):
    angle = df["angle_deg"].values
    E     = df["objective"].values
    ga    = df["grad_analytical"].values
    gn    = df["grad_numerical"].values
    true_angle = df["true_angle_deg"].iloc[0]

    # ── left: objective function ───────────────────────────────────────────────
    ax_E = axes[row, 0]
    ax_E.plot(angle, E, color="navy", linewidth=1.5)
    ax_E.axvline(true_angle, color="darkgreen", linewidth=1.0, linestyle=":",
                 label=f"真値 {true_angle:.1f}°")
    ax_E.axhline(0, color="gray", linewidth=0.5, linestyle="--")
    ax_E.set_title(f"目的関数 E ({plabel} を走査)", fontsize=11)
    ax_E.set_xlabel("回転角 [deg]")
    ax_E.set_ylabel("目的関数 E")
    ax_E.legend(fontsize=9)
    ax_E.grid(True, alpha=0.3)

    # ── right: gradient comparison ─────────────────────────────────────────────
    ax_G = axes[row, 1]
    ax_G.plot(angle, ga, color="red",  linewidth=1.5, linestyle="-",
              label="解析微分")
    ax_G.plot(angle, gn, color="blue", linewidth=1.5, linestyle="--",
              label="数値微分")
    ax_G.axvline(true_angle, color="darkgreen", linewidth=1.0, linestyle=":",
                 label=f"真値 {true_angle:.1f}°")
    ax_G.axhline(0, color="gray", linewidth=0.5, linestyle="--")
    ax_G.set_title(f"微分 dE/d{plabel} ({plabel} を走査)", fontsize=11)
    ax_G.set_xlabel("回転角 [deg]")
    ax_G.set_ylabel(f"dE/d{plabel}")
    ax_G.legend(fontsize=9)
    ax_G.grid(True, alpha=0.3)

plt.tight_layout()
out_path = os.path.join(GRAPH_DIR, "scan_3param.png")
plt.savefig(out_path, dpi=150, bbox_inches="tight")
print(f"Saved: {out_path}")
