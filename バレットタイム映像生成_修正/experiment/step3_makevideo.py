#!/usr/bin/env python3
"""step3: 線あり注視画像から動画を生成する"""

import glob
import os
import sys

import cv2

from common import FRAMES_LINED_DIR, INPUT_VIDEO, OUTPUT_VIDEO, PROJECT_ROOT


def main():
    os.chdir(PROJECT_ROOT)

    lined_paths = sorted(glob.glob(os.path.join(FRAMES_LINED_DIR, "frame_*.png")))
    if not lined_paths:
        print(f"エラー: {FRAMES_LINED_DIR} にフレームがありません。step2_process.py を先に実行してください。")
        sys.exit(1)

    first = cv2.imread(lined_paths[0])
    H, W = first.shape[:2]

    cap = cv2.VideoCapture(INPUT_VIDEO)
    fps = cap.get(cv2.CAP_PROP_FPS)
    cap.release()

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(OUTPUT_VIDEO, fourcc, fps, (W, H))

    for i, path in enumerate(lined_paths):
        frame = cv2.imread(path)
        writer.write(frame)
        print(f"書き込み: frame_{i:04d}.png")

    writer.release()
    print(f"出力完了: {OUTPUT_VIDEO}")


if __name__ == "__main__":
    main()
