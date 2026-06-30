#!/usr/bin/env python3
"""step3 拡大版: 線あり注視画像の中央を切り取って動画を生成する

注視点は画像中心にあるため，中心領域をクロップすると
バレットタイム映像を拡大表示したような出力になる。

使い方:
  python3 experiment/step3_makevideo_expansion.py

環境変数:
  CROP_DIV  元画像に対する切り取り比率の分母（既定: 8 → 1/8 サイズ）
            例: CROP_DIV=4 なら幅・高さとも 1/4 にクロップ
"""

import glob
import os
import sys

import cv2

from common import FRAMES_LINED_DIR, INPUT_VIDEO, PROJECT_ROOT


def center_crop(frame, crop_w, crop_h):
    """画像中心を基準に crop_w x crop_h を切り出す"""
    h, w = frame.shape[:2]
    x0 = max(0, w // 2 - crop_w // 2)
    y0 = max(0, h // 2 - crop_h // 2)
    x1 = min(w, x0 + crop_w)
    y1 = min(h, y0 + crop_h)
    return frame[y0:y1, x0:x1]


def main():
    os.chdir(PROJECT_ROOT)

    crop_div = int(os.environ.get("CROP_DIV", "8"))
    if crop_div < 2:
        print("エラー: CROP_DIV は 2 以上を指定してください")
        sys.exit(1)

    output_video = os.path.join(PROJECT_ROOT, "Videos", "output1_expansion.mp4")

    lined_paths = sorted(glob.glob(os.path.join(FRAMES_LINED_DIR, "frame_*.png")))
    if not lined_paths:
        print(f"エラー: {FRAMES_LINED_DIR} にフレームがありません。step2_process.py を先に実行してください。")
        sys.exit(1)

    first = cv2.imread(lined_paths[0])
    if first is None:
        print(f"エラー: フレームを読み込めません: {lined_paths[0]}")
        sys.exit(1)

    src_h, src_w = first.shape[:2]
    crop_w = max(2, src_w // crop_div)
    crop_h = max(2, src_h // crop_div)
    cropped = center_crop(first, crop_w, crop_h)
    out_h, out_w = cropped.shape[:2]

    cap = cv2.VideoCapture(INPUT_VIDEO)
    fps = cap.get(cv2.CAP_PROP_FPS)
    cap.release()
    if fps <= 0:
        fps = 30.0

    print(f"入力フレーム: {src_w} x {src_h}")
    print(f"クロップ領域: {out_w} x {out_h}  (CROP_DIV={crop_div}, 中心切り取り)")
    print(f"フレーム数: {len(lined_paths)}")
    print(f"出力: {output_video}")

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(output_video, fourcc, fps, (out_w, out_h))
    if not writer.isOpened():
        print(f"エラー: 動画を開けません: {output_video}")
        sys.exit(1)

    for i, path in enumerate(lined_paths):
        frame = cv2.imread(path)
        if frame is None:
            print(f"警告: スキップ {path}")
            continue
        writer.write(center_crop(frame, crop_w, crop_h))
        print(f"書き込み: frame_{i:04d}.png")

    writer.release()
    print(f"出力完了: {output_video}")


if __name__ == "__main__":
    main()
