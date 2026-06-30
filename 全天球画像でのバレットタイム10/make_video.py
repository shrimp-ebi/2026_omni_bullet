#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import cv2
import argparse

# 十字線設定
CROSS_COLOR = (0, 255, 0)
CROSS_THICKNESS = 4

# 切り出しサイズ
CROP_HALF_W = 360
CROP_HALF_H = 180


def draw_cross(img, cx, cy):
    h, w = img.shape[:2]

    cv2.line(
        img,
        (0, cy),
        (w - 1, cy),
        CROSS_COLOR,
        CROSS_THICKNESS
    )

    cv2.line(
        img,
        (cx, 0),
        (cx, h - 1),
        CROSS_COLOR,
        CROSS_THICKNESS
    )


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--input-dir",
        default="processed_frames",
        help="入力画像ディレクトリ"
    )

    parser.add_argument(
        "--output",
        default="output.mp4",
        help="出力動画"
    )

    parser.add_argument(
        "--output-frames-dir",
        default="output_frames",
        help="十字線付き画像の保存先"
    )

    parser.add_argument(
        "--fps",
        type=float,
        default=29.97
    )

    args = parser.parse_args()

    os.makedirs(args.output_frames_dir, exist_ok=True)

    files = sorted(
        [f for f in os.listdir(args.input_dir) if f.endswith(".png")],
        key=lambda x: int(os.path.splitext(x)[0])
    )

    if len(files) == 0:
        print("画像が見つかりません")
        return

    crop_w = 2 * CROP_HALF_W
    crop_h = 2 * CROP_HALF_H

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")

    writer = cv2.VideoWriter(
        args.output,
        fourcc,
        args.fps,
        (crop_w, crop_h)
    )

    if not writer.isOpened():
        print("VideoWriterの作成に失敗")
        return

    print(f"入力画像数 : {len(files)}")
    print(f"出力動画   : {args.output}")
    print(f"出力画像   : {args.output_frames_dir}")

    for idx, fname in enumerate(files):

        img = cv2.imread(
            os.path.join(args.input_dir, fname)
        )

        if img is None:
            continue

        h, w = img.shape[:2]

        cx = w // 2
        cy = h // 2

        draw_cross(img, cx, cy)

        x0 = max(0, cx - CROP_HALF_W)
        x1 = min(w, cx + CROP_HALF_W)

        y0 = max(0, cy - CROP_HALF_H)
        y1 = min(h, cy + CROP_HALF_H)

        crop = img[y0:y1, x0:x1]

        output_frame = os.path.join(
            args.output_frames_dir,
            f"frame_{idx:05d}.jpg"
        )

        cv2.imwrite(
            output_frame,
            crop,
            [cv2.IMWRITE_JPEG_QUALITY, 95]
        )

        writer.write(crop)

        if idx % 10 == 0:
            print(f"{idx}/{len(files)}")

    writer.release()

    print("\n完了")
    print(f"動画 : {args.output}")
    print(f"画像 : {args.output_frames_dir}")


if __name__ == "__main__":
    main()