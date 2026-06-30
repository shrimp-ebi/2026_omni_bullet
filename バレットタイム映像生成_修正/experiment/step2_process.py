#!/usr/bin/env python3
"""step2: LM推定 + 注視画像生成（栖原ロール）"""

import ctypes
import glob
import os
import sys
import time

import cv2
import numpy as np

from common import (
    FRAMES_GAZE_DIR,
    FRAMES_LINED_DIR,
    FRAMES_RAW_DIR,
    GAZE_TXT,
    PROJECT_ROOT,
    bgr_to_rgb_contiguous,
    draw_center_lines,
    load_lib,
)


def log(msg):
    """stdoutバッファリング対策: 即座に表示する"""
    print(msg, flush=True)


def main():
    os.chdir(PROJECT_ROOT)
    t_start = time.time()

    if not os.path.isfile(GAZE_TXT):
        log(f"エラー: {GAZE_TXT} が見つかりません。step1_extract.py を先に実行してください。")
        sys.exit(1)

    raw_paths = sorted(glob.glob(os.path.join(FRAMES_RAW_DIR, "frame_*.png")))
    if not raw_paths:
        log(f"エラー: {FRAMES_RAW_DIR} にフレームがありません。step1_extract.py を先に実行してください。")
        sys.exit(1)

    frame_limit = os.environ.get("FRAME_LIMIT")
    if frame_limit is not None:
        raw_paths = raw_paths[: int(frame_limit)]
        log(f"[DEBUG] FRAME_LIMIT={frame_limit} → {len(raw_paths)} フレームのみ処理")

    with open(GAZE_TXT, encoding="utf-8") as f:
        parts = f.read().strip().split()
    gaze_x, gaze_y, gaze_z = float(parts[0]), float(parts[1]), float(parts[2])

    os.makedirs(FRAMES_GAZE_DIR, exist_ok=True)
    os.makedirs(FRAMES_LINED_DIR, exist_ok=True)

    log("[開始] ライブラリ読み込み...")
    lib = load_lib()
    log(f"  libomnigaze.so 読み込み完了 ({time.time() - t_start:.1f}s)")

    log("[開始] 1枚目のフレーム読み込み...")
    t_read = time.time()
    first_bgr = cv2.imread(raw_paths[0])
    H, W = first_bgr.shape[:2]
    cw = max(32, int(W * 380.0 / 6080.0 / 2.0))
    ch = max(16, int(H * 190.0 / 3040.0 / 2.0))
    total = len(raw_paths)

    log(f"  解像度: {W} x {H} ({W * H:,} 画素/フレーム)")
    log(f"  LM比較領域: 中心付近 約 {cw} x {ch} = {cw * ch:,} 画素")
    log(f"  総フレーム数: {total}")
    log(f"  注視点: ({gaze_x:.6f}, {gaze_y:.6f}, {gaze_z:.6f})")
    log(f"  1枚目読み込み完了 ({time.time() - t_read:.1f}s)")

    R_out = np.zeros(9, dtype=np.float64)
    out_buf = np.zeros((H, W, 3), dtype=np.uint8)
    log("[準備] バッファ確保完了。フレーム処理を開始します。")
    log("  ※ frame0: R0 のみ（Ib=Ir だと LM が単位行列に収束するため）")
    log("  ※ frame1以降: Ib=前フレーム注視画像, Ir=生フレーム, LM初期値=前フレームR")
    log("  ※ 5760x2880 等の高解像度では1フレームあたり数分〜数十分かかる場合があります。")

    for i, raw_path in enumerate(raw_paths):
        t_frame = time.time()
        log(f"\n[フレーム {i + 1}/{total}] {os.path.basename(raw_path)}")

        t_load = time.time()
        frame_bgr = cv2.imread(raw_path)
        ref_rgb = bgr_to_rgb_contiguous(frame_bgr)
        log(f"  読み込み+RGB変換 完了 ({time.time() - t_load:.1f}s)")

        if i == 0:
            t_gen = time.time()
            log("  frame0: R0 のみで注視画像生成（LMスキップ）...")
            lib.get_gaze_rotation(
                gaze_x, gaze_y, gaze_z,
                R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
            )
            lib.generate_gaze_at_gaze_point(
                ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                W, H,
                gaze_x, gaze_y, gaze_z,
                out_buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
            )
            n_iter = 0
            log(f"  注視画像生成 完了 ({time.time() - t_gen:.1f}s)")
        else:
            t_lm = time.time()
            log("  LM推定 開始 (Ib=前フレーム注視画像, Ir=生フレーム)...")
            R_prev = np.ascontiguousarray(R_out.copy(), dtype=np.float64)
            n_iter = lib.lm_estimate_frame(
                base_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                W, H,
                R_prev.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
            )
            log(f"  LM推定 完了: {n_iter}反復 ({time.time() - t_lm:.1f}s)")

            t_gen = time.time()
            log("  注視画像生成 開始 warp(Ir, R)...")
            lib.generate_gaze_frame(
                ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                W, H,
                out_buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
            )
            log(f"  注視画像生成 完了 ({time.time() - t_gen:.1f}s)")

        t_save = time.time()
        out_bgr = cv2.cvtColor(out_buf, cv2.COLOR_RGB2BGR)
        cv2.imwrite(os.path.join(FRAMES_GAZE_DIR, f"frame_{i:04d}.png"), out_bgr)
        lined = out_bgr.copy()
        draw_center_lines(lined)
        cv2.imwrite(os.path.join(FRAMES_LINED_DIR, f"frame_{i:04d}.png"), lined)
        log(f"  保存 完了 ({time.time() - t_save:.1f}s)")

        base_rgb = np.ascontiguousarray(out_buf.copy(), dtype=np.uint8)

        elapsed = time.time() - t_frame
        avg = (time.time() - t_start) / (i + 1)
        eta = avg * (total - i - 1)
        log(f"  フレーム合計: {elapsed:.1f}s | 平均: {avg:.1f}s/枚 | 残り見込み: {eta / 60:.1f}分")

    log(f"\n完了: {total} フレームを処理しました (総時間: {(time.time() - t_start) / 60:.1f}分)")


if __name__ == "__main__":
    main()
