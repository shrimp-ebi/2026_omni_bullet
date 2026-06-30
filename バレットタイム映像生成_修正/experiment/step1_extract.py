#!/usr/bin/env python3
"""step1: 動画をフレーム分割し、注視点を指定する"""

import os
import sys

import cv2

from common import (
    FRAMES_RAW_DIR,
    GAZE_TXT,
    INPUT_VIDEO,
    PROJECT_ROOT,
    get_gaze_point,
)


def main():
    os.chdir(PROJECT_ROOT)

    if not os.path.isfile(INPUT_VIDEO):
        print(f"エラー: 入力動画が見つかりません: {INPUT_VIDEO}")
        sys.exit(1)

    os.makedirs(FRAMES_RAW_DIR, exist_ok=True)

    cap = cv2.VideoCapture(INPUT_VIDEO)
    if not cap.isOpened():
        print(f"エラー: 動画を開けません: {INPUT_VIDEO}")
        sys.exit(1)

    ret, first_frame = cap.read()
    if not ret:
        print("エラー: フレームを読み込めません")
        sys.exit(1)

    W = first_frame.shape[1]
    H = first_frame.shape[0]

    gaze = get_gaze_point(first_frame)
    if gaze is None:
        print("キャンセルされました")
        sys.exit(0)

    gaze_x, gaze_y, gaze_z = gaze
    print(f"注視点 (世界座標): ({gaze_x:.6f}, {gaze_y:.6f}, {gaze_z:.6f})")

    with open(GAZE_TXT, "w", encoding="utf-8") as f:
        f.write(f"{gaze_x} {gaze_y} {gaze_z}\n")

    frame_idx = 0
    cv2.imwrite(os.path.join(FRAMES_RAW_DIR, f"frame_{frame_idx:04d}.png"), first_frame)
    print(f"保存: frame_{frame_idx:04d}.png")
    frame_idx += 1

    while True:
        ret, frame = cap.read()
        if not ret:
            break
        cv2.imwrite(os.path.join(FRAMES_RAW_DIR, f"frame_{frame_idx:04d}.png"), frame)
        print(f"保存: frame_{frame_idx:04d}.png")
        frame_idx += 1

    cap.release()
    print(f"完了: 総フレーム数 {frame_idx}")


if __name__ == "__main__":
    main()
