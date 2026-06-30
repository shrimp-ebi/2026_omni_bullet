#!/usr/bin/env python3
"""注視画像の単体検証（LMなし）

注視点GからR0だけで注視画像を1枚生成し、
注視方向が画像中心（緑の十字）に来るか確認する。

使い方:
  python3 -u experiment/verify_gaze.py              # frame_0000
  python3 -u experiment/verify_gaze.py --frame 1    # frame_0001
  python3 -u experiment/verify_gaze.py --click      # gaze.txt を使わず再クリック
"""

import argparse
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
    get_gaze_point,
    load_lib,
    world_to_image,
)

VERIFY_DIR = os.path.join(PROJECT_ROOT, "Videos", "verify")


def log(msg):
    print(msg, flush=True)


def frame_path(n):
    return os.path.join(FRAMES_RAW_DIR, f"frame_{n:04d}.png")


def mark_gaze_on_input(frame_bgr, gaze_x, gaze_y, gaze_z):
    """入力画像に注視方向Gの位置（赤十字）を描画"""
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


def main():
    parser = argparse.ArgumentParser(description="注視画像の単体検証（LMなし）")
    parser.add_argument(
        "--frame", type=int, default=0,
        help="frames_raw のフレーム番号 (default: 0)",
    )
    parser.add_argument(
        "--input", default=None,
        help="入力画像パス (--frame より優先)",
    )
    parser.add_argument(
        "--click", action="store_true",
        help="gaze.txt を使わず、このフレーム上で注視点を再クリックする",
    )
    args = parser.parse_args()

    os.chdir(PROJECT_ROOT)
    os.makedirs(VERIFY_DIR, exist_ok=True)

    input_path = args.input if args.input else frame_path(args.frame)
    frame_id = args.frame if args.input is None else None

    if not os.path.isfile(input_path):
        log(f"エラー: 入力画像が見つかりません: {input_path}")
        log("  step1_extract.py を先に実行するか、--frame / --input を指定してください。")
        sys.exit(1)

    frame_bgr = cv2.imread(input_path)
    if frame_bgr is None:
        log(f"エラー: 画像を読み込めません: {input_path}")
        sys.exit(1)

    H, W = frame_bgr.shape[:2]
    if frame_id is not None:
        log(f"フレーム: {frame_id}  入力: {input_path} ({W} x {H})")
    else:
        log(f"入力: {input_path} ({W} x {H})")

    if args.click:
        gaze = get_gaze_point(frame_bgr)
        if gaze is None:
            log("キャンセルされました")
            sys.exit(0)
        gaze_x, gaze_y, gaze_z = gaze
    else:
        if not os.path.isfile(GAZE_TXT):
            log(f"エラー: {GAZE_TXT} が見つかりません。")
            log("  step1_extract.py を実行するか、--click で注視点を指定してください。")
            sys.exit(1)
        with open(GAZE_TXT, encoding="utf-8") as f:
            parts = f.read().strip().split()
        gaze_x, gaze_y, gaze_z = float(parts[0]), float(parts[1]), float(parts[2])

    log(f"注視点 G (世界座標, frame0で指定): ({gaze_x:.6f}, {gaze_y:.6f}, {gaze_z:.6f})")

    input_marked, ui, vi = mark_gaze_on_input(frame_bgr, gaze_x, gaze_y, gaze_z)
    cx, cy = W // 2, H // 2
    log(f"このフレーム上の G の位置: ({ui}, {vi})  画像中心: ({cx}, {cy})")

    if frame_id is not None and frame_id > 0:
        log("  ※ frame0 以外では、カメラが動いていれば G の画素位置は frame0 のクリック位置とずれます。")
        log("  ※ 確認するのは「赤十字の物体が出力の緑十字（中心）に来るか」です。")

    lib = load_lib()
    in_rgb = bgr_to_rgb_contiguous(frame_bgr)
    out_buf = np.zeros((H, W, 3), dtype=np.uint8)

    log("注視画像生成中（LMなし、R0のみ）...")
    log("  ※ 高解像度では数分かかることがあります。")
    t0 = time.time()
    lib.generate_gaze_at_gaze_point(
        in_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        W, H,
        gaze_x, gaze_y, gaze_z,
        out_buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
    )
    log(f"  完了 ({time.time() - t0:.1f}s)")

    out_bgr = cv2.cvtColor(out_buf, cv2.COLOR_RGB2BGR)
    draw_center_lines(out_bgr)

    suffix = f"frame_{frame_id:04d}" if frame_id is not None else "custom"
    path_input = os.path.join(VERIFY_DIR, f"verify_{suffix}_input.png")
    path_output = os.path.join(VERIFY_DIR, f"verify_{suffix}_output.png")
    cv2.imwrite(path_input, input_marked)
    cv2.imwrite(path_output, out_bgr)

    log("")
    log("=== 確認方法 ===")
    log(f"1. 入力（赤十字=このフレーム上の注視方向G）: {path_input}")
    log(f"2. 出力（緑十字=画像中心）                : {path_output}")
    log("   → 赤十字の物体が、出力の緑十字（中心）に来ていれば成功です。")
    log("")
    log("LM推定は使っていません。")


if __name__ == "__main__":
    main()
