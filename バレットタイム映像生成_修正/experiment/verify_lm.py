#!/usr/bin/env python3
"""LM推定 + 注視画像の2フレーム検証

step2 と同じ栖原ロール式で frame 0 → frame 1 を処理し、
注視方向 G が中心（緑十字）に来るか確認する。

処理:
  frame 0: R0 のみで注視画像生成（LMスキップ）
  frame 1: base=注視画像0(線なし), ref=生フレーム1 → LM(初期値I) → R=R_lm×R_prev → 注視画像1

使い方:
  python3 -u experiment/verify_lm.py
"""

import ctypes
import os
import sys
import time

import cv2
import numpy as np

from common import (
    FRAMES_RAW_DIR,
    GAZE_TXT,
    PROJECT_ROOT,
    bgr_to_rgb_contiguous,
    draw_center_lines,
    load_lib,
    world_to_image,
)

VERIFY_DIR = os.path.join(PROJECT_ROOT, "Videos", "verify")


def log(msg):
    print(msg, flush=True)


def frame_path(n):
    return os.path.join(FRAMES_RAW_DIR, f"frame_{n:04d}.png")


def mark_gaze_on_input(frame_bgr, gaze_x, gaze_y, gaze_z):
    marked = frame_bgr.copy()
    H, W = marked.shape[:2]
    u, v = world_to_image(gaze_x, gaze_y, gaze_z, W, H)
    ui, vi = int(round(u)), int(round(v))
    cv2.drawMarker(marked, (ui, vi), (0, 0, 255), cv2.MARKER_CROSS, 30, 2)
    cv2.putText(
        marked, f"G ({ui}, {vi})", (ui + 10, vi - 10),
        cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 0, 255), 2,
    )
    return marked, ui, vi


def process_frame(lib, base_rgb, ref_bgr, R_prev, W, H, R_out, out_buf):
    """step2 と同じ LM + 注視画像生成"""
    ref_rgb = bgr_to_rgb_contiguous(ref_bgr)

    n_iter = lib.lm_estimate_frame(
        base_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        W, H,
        R_prev.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
        R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
    )

    lib.generate_gaze_frame(
        ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
        W, H,
        out_buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
    )

    return n_iter, ref_rgb


def main():
    os.chdir(PROJECT_ROOT)
    os.makedirs(VERIFY_DIR, exist_ok=True)

    for path, label in [
        (GAZE_TXT, "gaze.txt"),
        (frame_path(0), "frame_0000.png"),
        (frame_path(1), "frame_0001.png"),
    ]:
        if not os.path.isfile(path):
            log(f"エラー: {label} が見つかりません: {path}")
            log("  step1_extract.py を先に実行してください。")
            sys.exit(1)

    with open(GAZE_TXT, encoding="utf-8") as f:
        parts = f.read().strip().split()
    gaze_x, gaze_y, gaze_z = float(parts[0]), float(parts[1]), float(parts[2])

    frame0_bgr = cv2.imread(frame_path(0))
    frame1_bgr = cv2.imread(frame_path(1))
    H, W = frame0_bgr.shape[:2]

    log(f"解像度: {W} x {H}")
    log(f"注視点 G: ({gaze_x:.6f}, {gaze_y:.6f}, {gaze_z:.6f})")
    log("")

    lib = load_lib()
    R_out = np.zeros(9, dtype=np.float64)
    out_buf = np.zeros((H, W, 3), dtype=np.uint8)

    # --- frame 0: R0 のみ（base==ref では LM が R→I に収束するため） ---
    log("[frame 0] R0のみで注視画像生成（LMスキップ）")
    ref0_rgb = bgr_to_rgb_contiguous(frame0_bgr)

    t0 = time.time()
    lib.get_gaze_rotation(
        gaze_x, gaze_y, gaze_z,
        R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
    )
    lib.generate_gaze_at_gaze_point(
        ref0_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        W, H,
        gaze_x, gaze_y, gaze_z,
        out_buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
    )
    n_iter0 = 0
    log(f"  完了 ({time.time() - t0:.1f}s)")

    out0_bgr = cv2.cvtColor(out_buf, cv2.COLOR_RGB2BGR)
    lined0 = out0_bgr.copy()
    draw_center_lines(lined0)
    path0 = os.path.join(VERIFY_DIR, "verify_lm_frame_0000_output.png")
    cv2.imwrite(path0, lined0)
    log(f"  保存: {path0}")

    base_rgb = np.ascontiguousarray(out_buf.copy(), dtype=np.uint8)
    log("")
    log("[frame 1] base=注視画像0(線なし), ref=生フレーム1")
    ui, vi = mark_gaze_on_input(frame1_bgr, gaze_x, gaze_y, gaze_z)[1:]
    log(f"  frame1 上の G の位置: ({ui}, {vi})")

    t1 = time.time()
    log("  LM推定 開始...")
    R_prev = np.ascontiguousarray(R_out.copy(), dtype=np.float64)
    n_iter1, _ = process_frame(
        lib, base_rgb, frame1_bgr, R_prev, W, H, R_out, out_buf,
    )
    log(f"  LM推定 完了: {n_iter1}反復 ({time.time() - t1:.1f}s)")

    input_marked, _, _ = mark_gaze_on_input(frame1_bgr, gaze_x, gaze_y, gaze_z)
    out1_bgr = cv2.cvtColor(out_buf, cv2.COLOR_RGB2BGR)
    lined1 = out1_bgr.copy()
    draw_center_lines(lined1)

    path_in = os.path.join(VERIFY_DIR, "verify_lm_frame_0001_input.png")
    path_out = os.path.join(VERIFY_DIR, "verify_lm_frame_0001_output.png")
    cv2.imwrite(path_in, input_marked)
    cv2.imwrite(path_out, lined1)

    log("")
    log("=== 確認方法 ===")
    log(f"frame0 LM結果 : {path0}")
    log(f"frame1 入力   : {path_in}  （赤十字=G の位置）")
    log(f"frame1 出力   : {path_out}  （緑十字=中心）")
    log("  → frame1 出力で、赤十字の物体が緑十字（中心）に来ていれば LM+栖原ロールは成功です。")
    log("")
    log(f"LM反復回数: frame0={n_iter0}, frame1={n_iter1}")


if __name__ == "__main__":
    main()
