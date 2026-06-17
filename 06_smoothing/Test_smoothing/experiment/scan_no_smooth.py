#!/usr/bin/env python3
"""
scan_no_smooth.py
平滑化なし（sigma≈0）の収束域スキャン（16並列）
"""

import os
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed

BINARY      = "./build/lm_estimate_single"
OUTPUT_CSV  = "results/convergence_no_smooth.csv"
SIGMA       = 0.0001
MAX_WORKERS = 16
TIMEOUT     = 7200

W1_MIN, W1_MAX, W1_STEP = 5.0, 25.0, 2.0
W2_MIN, W2_MAX, W2_STEP = 20.0, 40.0, 2.0


def build_binary():
    print("lm_estimate_single をビルド中 (make lm_estimate_single)...")
    ret = subprocess.run(["make", "lm_estimate_single"],
                         capture_output=True, text=True)
    if ret.returncode != 0:
        print("ビルド失敗:")
        print(ret.stderr)
        sys.exit(1)
    print("ビルド完了\n")


def run_single(args):
    w1, w2 = args
    try:
        proc = subprocess.run(
            [BINARY, str(w1), str(w2), str(SIGMA)],
            capture_output=True, text=True, timeout=TIMEOUT
        )
        if proc.returncode == 0:
            line = proc.stdout.strip()
            if line:
                parts = line.split(",")
                if len(parts) == 6:
                    return (w1, w2,
                            int(parts[2]), int(parts[3]),
                            float(parts[4]), float(parts[5]))
    except subprocess.TimeoutExpired:
        pass
    except Exception:
        pass
    return (w1, w2, 0, -1, 999.0, 999.0)


def main():
    if not os.path.exists(BINARY):
        build_binary()

    w1_values = []
    v = W1_MIN
    while v <= W1_MAX + 1e-9:
        w1_values.append(round(v, 6))
        v += W1_STEP

    w2_values = []
    v = W2_MIN
    while v <= W2_MAX + 1e-9:
        w2_values.append(round(v, 6))
        v += W2_STEP

    cases = [(w1, w2) for w1 in w1_values for w2 in w2_values]
    total = len(cases)

    print(f"スキャン範囲: ω1 {W1_MIN}°～{W1_MAX}° ({W1_STEP}°刻み), "
          f"ω2 {W2_MIN}°～{W2_MAX}° ({W2_STEP}°刻み)")
    print(f"格子点数: {total}  sigma={SIGMA}  並列数: {MAX_WORKERS}")
    print(f"出力CSV: {OUTPUT_CSV}\n")

    t_start = time.time()
    print(f"開始: {time.strftime('%H:%M:%S')}")

    results = {}
    done = 0

    with ProcessPoolExecutor(max_workers=MAX_WORKERS) as executor:
        future_to_case = {executor.submit(run_single, c): c for c in cases}

        for future in as_completed(future_to_case):
            row = future.result()
            results[(row[0], row[1])] = row
            done += 1
            print(f"  [{done:3d}/{total}] ω1={row[0]:+6.1f}°  ω2={row[1]:+6.1f}°  "
                  f"success={row[2]}  iter={row[3]}  frob={row[4]:.6f}")

    os.makedirs("results", exist_ok=True)
    with open(OUTPUT_CSV, "w") as f:
        f.write("w1_init,w2_init,success,iter,frob_error,final_E\n")
        for w1, w2 in cases:
            row = results[(w1, w2)]
            f.write(f"{row[0]:.2f},{row[1]:.2f},{row[2]},{row[3]},{row[4]:.6f},{row[5]:.6f}\n")

    elapsed = time.time() - t_start
    n_success = sum(1 for r in results.values() if r[2] == 1)
    print(f"\n終了: {time.strftime('%H:%M:%S')}  経過: {elapsed:.1f}秒")
    print(f"収束: {n_success}/{total}")
    print(f"CSV保存: {OUTPUT_CSV}")


if __name__ == "__main__":
    main()
