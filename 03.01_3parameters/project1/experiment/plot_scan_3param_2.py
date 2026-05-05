"""
plot_scan_3param_2.py
3パラメータ回転スキャンの各グラフを個別に6枚出力する。

Run from project1/experiment/:
  python3 plot_scan_3param_2.py
"""

import os
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

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(SCRIPT_DIR, "..", "results")
GRAPH_DIR = os.path.join(RESULTS_DIR, "graphs_3param")
OUT_DIR = os.path.join(GRAPH_DIR, "scan_3param_2")
os.makedirs(OUT_DIR, exist_ok=True)

PLOTS = [
    {
        "csv": os.path.join(RESULTS_DIR, "scan_3param_omega1.csv"),
        "param_label": r"$\omega_1$",
        "suffix": "omega1",
    },
    {
        "csv": os.path.join(RESULTS_DIR, "scan_3param_omega2.csv"),
        "param_label": r"$\omega_2$",
        "suffix": "omega2",
    },
    {
        "csv": os.path.join(RESULTS_DIR, "scan_3param_omega3.csv"),
        "param_label": r"$\omega_3$",
        "suffix": "omega3",
    },
]


def load_scan_csv(path):
    df = pd.read_csv(path)
    clip = 1e5
    df["grad_analytical"] = df["grad_analytical"].clip(-clip, clip)
    df["grad_numerical"] = df["grad_numerical"].clip(-clip, clip)
    return df


def save_objective_plot(df, param_label, suffix):
    angle = df["angle_deg"].values
    objective = df["objective"].values
    true_angle = df["true_angle_deg"].iloc[0]

    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(angle, objective, color="navy", linewidth=1.8)
    ax.axvline(true_angle, color="darkgreen", linewidth=1.0, linestyle=":",
               label=f"真値 {true_angle:.1f}°")
    ax.axhline(0, color="gray", linewidth=0.5, linestyle="--")
    ax.set_title(f"回転角度と目的関数 E の関係({param_label})", fontsize=12)
    ax.set_xlabel("回転角度 [度]")
    ax.set_ylabel("目的関数 E の値")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=10)
    fig.tight_layout()

    out_path = os.path.join(OUT_DIR, f"scan_3param_objective_{suffix}.png")
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {out_path}")


def save_gradient_plot(df, param_label, suffix):
    angle = df["angle_deg"].values
    grad_analytical = df["grad_analytical"].values
    grad_numerical = df["grad_numerical"].values
    true_angle = df["true_angle_deg"].iloc[0]

    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(angle, grad_analytical, color="red", linewidth=1.8, linestyle="-",
            label="理論微分(提案手法の式を用いて計算)")
    ax.plot(angle, grad_numerical, color="blue", linewidth=1.8, linestyle="--",
            label="数値微分(目的関数の値から計算)")
    ax.axvline(true_angle, color="darkgreen", linewidth=1.0, linestyle=":",
               label=f"真値 {true_angle:.1f}°")
    ax.axhline(0, color="gray", linewidth=0.5, linestyle="--")
    ax.set_title(f"回転角度と微分 dE/d の関係({param_label} )", fontsize=12)
    ax.set_xlabel("回転角度 [度]")
    ax.set_ylabel(f"dE/d{param_label} の値")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=10)
    fig.tight_layout()

    out_path = os.path.join(OUT_DIR, f"scan_3param_gradient_{suffix}.png")
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {out_path}")


def main():
    for plot in PLOTS:
        df = load_scan_csv(plot["csv"])
        save_objective_plot(df, plot["param_label"], plot["suffix"])
        save_gradient_plot(df, plot["param_label"], plot["suffix"])


if __name__ == "__main__":
    main()
