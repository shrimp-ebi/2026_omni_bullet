#!/usr/bin/env python3
"""
scan_boundary_fine.py
収束域の境界を 0.1° 刻みで精密スキャンする。

4つの境界帯（ω1左・右、ω2下・上）の和集合を格子点として使用する。
各帯はそれぞれ対応する収束域内の範囲のみを対象とする。
"""

import os
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed

BINARY      = "./build/lm_estimate_single"
SIGMA_LIST  = [3.0]
MAX_WORKERS = 16
TIMEOUT     = 7200
STEP        = 0.1

OUTPUT_CSV  = "results/convergence_boundary_fine.csv"

# ω1左境界帯 × ω2収束域内
W1_MIN_L, W1_MAX_L = 4.5, 6.5
W2_MIN_ALL, W2_MAX_ALL = 25.0, 35.0

# ω1右境界帯 × ω2収束域内
W1_MIN_R, W1_MAX_R = 22.0, 24.5

# ω1収束域内 × ω2下境界帯
W1_MIN_ALL, W1_MAX_ALL = 6.0, 23.0
W2_MIN_B, W2_MAX_B = 23.5, 25.5

# ω1収束域内 × ω2上境界帯
W2_MIN_T, W2_MAX_T = 34.5, 36.5


def sigma_to_tag(sigma):
    return f"{int(round(sigma * 100)):03d}"


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
    w1, w2, sigma = args
    try:
        proc = subprocess.run(
            [BINARY, str(w1), str(w2), str(sigma)],
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


def frange(lo, hi, step):
    vals = []
    v = lo
    while v <= hi + 1e-9:
        vals.append(round(v, 6))
        v += step
    return vals


def build_cases():
    """4境界帯の和集合（重複除外）を返す。"""
    w1_L   = frange(W1_MIN_L,   W1_MAX_L,   STEP)
    w1_R   = frange(W1_MIN_R,   W1_MAX_R,   STEP)
    w1_all = frange(W1_MIN_ALL, W1_MAX_ALL, STEP)
    w2_B   = frange(W2_MIN_B,   W2_MAX_B,   STEP)
    w2_T   = frange(W2_MIN_T,   W2_MAX_T,   STEP)
    w2_all = frange(W2_MIN_ALL, W2_MAX_ALL, STEP)

    case_set = set()
    # ω1左境界帯 × ω2収束域内
    for w1 in w1_L:
        for w2 in w2_all:
            case_set.add((w1, w2))
    # ω1右境界帯 × ω2収束域内
    for w1 in w1_R:
        for w2 in w2_all:
            case_set.add((w1, w2))
    # ω1収束域内 × ω2下境界帯
    for w1 in w1_all:
        for w2 in w2_B:
            case_set.add((w1, w2))
    # ω1収束域内 × ω2上境界帯
    for w1 in w1_all:
        for w2 in w2_T:
            case_set.add((w1, w2))

    # ω1, ω2 の昇順でソート
    return sorted(case_set, key=lambda p: (p[0], p[1]))


def scan_sigma(sigma, cases):
    total = len(cases)
    results = {}
    done = 0

    args_list = [(w1, w2, sigma) for w1, w2 in cases]

    with ProcessPoolExecutor(max_workers=MAX_WORKERS) as executor:
        future_to_args = {executor.submit(run_single, a): a for a in args_list}

        for future in as_completed(future_to_args):
            row = future.result()
            results[(row[0], row[1])] = row
            done += 1
            print(f"    [{done:4d}/{total}] ω1={row[0]:+7.2f}°  ω2={row[1]:+7.2f}°  "
                  f"success={row[2]}  iter={row[3]}  frob={row[4]:.6f}")

    return results


def save_csv(path, cases, results):
    os.makedirs("results", exist_ok=True)
    with open(path, "w") as f:
        f.write("w1_init,w2_init,success,iter,frob_error,final_E\n")
        for w1, w2 in cases:
            row = results[(w1, w2)]
            f.write(f"{row[0]:.2f},{row[1]:.2f},{row[2]},{row[3]},{row[4]:.6f},{row[5]:.6f}\n")


def main():
    if not os.path.exists(BINARY):
        build_binary()

    cases = build_cases()
    total = len(cases)

    print(f"境界精密スキャン: 4帯の和集合  格子点数={total}  刻み={STEP}°  並列数={MAX_WORKERS}")
    print(f"  ω1左帯:  {W1_MIN_L}°～{W1_MAX_L}°  ×  ω2収束域内({W2_MIN_ALL}°～{W2_MAX_ALL}°)")
    print(f"  ω1右帯:  {W1_MIN_R}°～{W1_MAX_R}°  ×  ω2収束域内")
    print(f"  ω1収束域内({W1_MIN_ALL}°～{W1_MAX_ALL}°)  ×  ω2下帯: {W2_MIN_B}°～{W2_MAX_B}°")
    print(f"  ω1収束域内  ×  ω2上帯: {W2_MIN_T}°～{W2_MAX_T}°")
    print(f"σリスト: {SIGMA_LIST}\n")

    for sigma in SIGMA_LIST:
        print(f"{'='*60}")
        print(f"σ = {sigma}  →  {OUTPUT_CSV}")
        t_start = time.time()
        print(f"  開始: {time.strftime('%H:%M:%S')}")

        results = scan_sigma(sigma, cases)
        save_csv(OUTPUT_CSV, cases, results)

        elapsed = time.time() - t_start
        n_success = sum(1 for r in results.values() if r[2] == 1)
        print(f"  終了: {time.strftime('%H:%M:%S')}  "
              f"経過: {elapsed:.1f}秒  "
              f"収束: {n_success}/{total}\n")

    print("境界精密スキャン完了")


if __name__ == "__main__":
    main()
