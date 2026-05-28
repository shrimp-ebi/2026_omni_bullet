"""
scan_convergence_smooth.py
平滑化ありLM法の収束域スキャン

ω1, ω2 の初期値を格子状に変えて lm_estimate_single を実行し、
結果をCSVに保存する。
"""

import subprocess
import csv
import os
import sys

# スキャン範囲（度）
W1_MIN = -5.0
W1_MAX = 35.0
W1_STEP = 1.0

W2_MIN = 0.0
W2_MAX = 60.0
W2_STEP = 1.0

BINARY = "./build/lm_estimate_single"
OUTPUT_CSV = "results/convergence_smooth.csv"

def main():
    os.makedirs("results", exist_ok=True)

    w1_values = []
    v = W1_MIN
    while v <= W1_MAX + 1e-9:
        w1_values.append(round(v, 2))
        v += W1_STEP

    w2_values = []
    v = W2_MIN
    while v <= W2_MAX + 1e-9:
        w2_values.append(round(v, 2))
        v += W2_STEP

    total = len(w1_values) * len(w2_values)
    print(f"スキャン総数: {total} ケース")
    print(f"ω1: {W1_MIN}° ~ {W1_MAX}° ({W1_STEP}°刻み)")
    print(f"ω2: {W2_MIN}° ~ {W2_MAX}° ({W2_STEP}°刻み)")
    print(f"出力: {OUTPUT_CSV}")
    print()

    done = 0
    with open(OUTPUT_CSV, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["w1_init", "w2_init", "success",
                          "iter", "frob_error", "final_E"])

        for w1 in w1_values:
            for w2 in w2_values:
                try:
                    result = subprocess.run(
                        [BINARY, str(w1), str(w2)],
                        capture_output=True, text=True, timeout=60
                    )
                    line = result.stdout.strip()
                    if line:
                        writer.writerow(line.split(","))
                    else:
                        writer.writerow([w1, w2, 0, 0, 999.0, 999.0])
                except subprocess.TimeoutExpired:
                    writer.writerow([w1, w2, 0, -1, 999.0, 999.0])
                except Exception as e:
                    writer.writerow([w1, w2, 0, -2, 999.0, 999.0])

                done += 1
                if done % 50 == 0 or done == total:
                    print(f"  進捗: {done}/{total} ({100*done//total}%)")

    print(f"\n完了！ → {OUTPUT_CSV}")

if __name__ == "__main__":
    main()
