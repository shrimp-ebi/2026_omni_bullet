#!/usr/bin/env python3
"""extract_frames.py
input.mp4 を 1 フレームずつ JPEG に展開する。

使い方:
    python script/extract_frames.py [--input INPUT_VIDEO] [--output-dir frames]

出力:
    frames/frame_00001.jpg, frame_00002.jpg, ...
"""

import argparse
import os
import sys
import cv2

INPUT_VIDEO = "input.mp4"   # 入力動画パスのデフォルト（1箇所だけ書く）


def main():
    parser = argparse.ArgumentParser(description="全方位動画をフレーム画像に展開する")
    parser.add_argument("--input",      default=INPUT_VIDEO, help="入力動画ファイル")
    parser.add_argument("--output-dir", default="frames",    help="出力ディレクトリ")
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    cap = cv2.VideoCapture(args.input)
    if not cap.isOpened():
        print(f"エラー: 動画を開けません: {args.input}", file=sys.stderr)
        sys.exit(1)

    fps       = cap.get(cv2.CAP_PROP_FPS)
    n_frames  = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    width     = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height    = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    print(f"入力  : {args.input}")
    print(f"解像度: {width} x {height}")
    print(f"FPS   : {fps:.3f}")
    print(f"総フレーム数: {n_frames}")
    print(f"出力先: {args.output_dir}/frame_NNNNN.jpg\n")

    fi = 1
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        path = os.path.join(args.output_dir, f"frame_{fi:05d}.jpg")
        cv2.imwrite(path, frame, [cv2.IMWRITE_JPEG_QUALITY, 95])
        if fi % 50 == 0 or fi == 1:
            print(f"  frame {fi:5d} / {n_frames} -> {path}")
        fi += 1

    cap.release()
    print(f"\n完了: {fi - 1} フレームを展開しました")
    print(f"FPS: {fps:.3f}  (make_video.py に渡す値)")


if __name__ == "__main__":
    main()
