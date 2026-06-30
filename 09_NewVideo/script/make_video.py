#!/usr/bin/env python3
"""make_video.py
注視画像（gaze_frames/gaze_NNNNN.jpg）に緑の十字線を描き、output.mp4 に連結する。

使い方:
    python script/make_video.py [--input-dir gaze_frames] [--output output.mp4]
                                [--fps 30] [--cross-size 40] [--cross-thickness 2]

出力:
    output.mp4（input.mp4 と同じ FPS で書き出す）
"""

import argparse
import os
import sys
import cv2

# 十字線の設定（定数化。ここを変更して調整する）
CROSS_COLOR     = (0, 255, 0)   # 緑（BGR）
CROSS_THICKNESS = 4             # 太さ [px]
CROSS_SIZE      = 40            # 中心から各方向への長さ [px]

# 切り出し領域（画像中心から ±CROP_HALF_W, ±CROP_HALF_V 画素）
# 比較領域に揃える。切り出しサイズは (2*CROP_HALF_W) × (2*CROP_HALF_V)
CROP_HALF_W = 360   # → 幅 720
CROP_HALF_V = 180   # → 高さ 360


def draw_cross(img, cx, cy, color, thickness):
    """画像中心を通る十字線を、画面の端から端まで引く。
    計算に使う画像には呼ばない。"""
    h, w = img.shape[:2]
    cv2.line(img, (0, cy), (w - 1, cy), color, thickness)  # 横線
    cv2.line(img, (cx, 0), (cx, h - 1), color, thickness)  # 縦線


def main():
    parser = argparse.ArgumentParser(description="注視画像に十字線を描いて動画化する")
    parser.add_argument("--input-dir",        default="gaze_frames",  help="入力ディレクトリ")
    parser.add_argument("--output",           default="output.mp4",   help="出力動画ファイル")
    parser.add_argument("--output-frames-dir",default="output_frames",
                        help="十字線付きフレームの保存先ディレクトリ（デフォルト: output_frames）")
    parser.add_argument("--fps",              type=float, default=30.0, help="フレームレート")
    parser.add_argument("--cross-size",       type=int,   default=CROSS_SIZE,
                        help="十字線の片側の長さ [px]")
    parser.add_argument("--cross-thickness",  type=int,   default=CROSS_THICKNESS,
                        help="十字線の太さ [px]")
    args = parser.parse_args()

    # 入力フレームを列挙
    files = sorted(
        f for f in os.listdir(args.input_dir)
        if f.startswith("gaze_") and f.endswith(".jpg")
    )
    if not files:
        print(f"エラー: {args.input_dir} に gaze_*.jpg が見つかりません", file=sys.stderr)
        sys.exit(1)

    # 解像度を1枚目から取得
    first = cv2.imread(os.path.join(args.input_dir, files[0]))
    if first is None:
        print(f"エラー: {files[0]} を読み込めません", file=sys.stderr)
        sys.exit(1)
    h, w = first.shape[:2]
    cx, cy = w // 2, h // 2   # （元画像の中心。以下のWriterサイズは切り出し後に合わせる）
    # 切り出し後のサイズに合わせて Writer を作る
    crop_w = 2 * CROP_HALF_W
    crop_h = 2 * CROP_HALF_V
    w, h = crop_w, crop_h

    os.makedirs(args.output_frames_dir, exist_ok=True)

    print(f"入力  : {args.input_dir}/gaze_NNNNN.jpg  ({len(files)} frames)")
    print(f"解像度: {w} x {h}")
    print(f"FPS   : {args.fps}")
    print(f"十字線: size={args.cross_size}  thickness={args.cross_thickness}  color=green")
    print(f"出力動画  : {args.output}")
    print(f"出力フレーム: {args.output_frames_dir}/output_NNNNN.jpg\n")

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(args.output, fourcc, args.fps, (w, h))
    if not writer.isOpened():
        print(f"エラー: VideoWriter を開けません: {args.output}", file=sys.stderr)
        sys.exit(1)

    for i, fname in enumerate(files, 1):
        img = cv2.imread(os.path.join(args.input_dir, fname))
        if img is None:
            print(f"警告: {fname} を読み込めません。スキップ。", file=sys.stderr)
            continue

        # 注視点付近を切り出す（画像中心 ±CROP_HALF）
        H, Wimg = img.shape[:2]
        ccx, ccy = Wimg // 2, H // 2
        x0 = max(0, ccx - CROP_HALF_W)
        x1 = min(Wimg, ccx + CROP_HALF_W)
        y0 = max(0, ccy - CROP_HALF_V)
        y1 = min(H, ccy + CROP_HALF_V)
        img = img[y0:y1, x0:x1].copy()

        # 切り出し後の中心に十字線を描く（出力用のみ。計算には使わない）
        crop_cx, crop_cy = (x1 - x0) // 2, (y1 - y0) // 2
        draw_cross(img, crop_cx, crop_cy,
                   CROSS_COLOR, args.cross_thickness)

        # 十字線付きフレームをファイルに保存
        out_frame_path = os.path.join(args.output_frames_dir, f"output_{i:05d}.jpg")
        cv2.imwrite(out_frame_path, img, [cv2.IMWRITE_JPEG_QUALITY, 95])

        writer.write(img)

        if i % 50 == 0 or i == 1:
            print(f"  frame {i:5d} / {len(files)}")

    writer.release()
    print(f"\n完了:")
    print(f"  動画          : {args.output}")
    print(f"  フレーム画像  : {args.output_frames_dir}/output_NNNNN.jpg")


if __name__ == "__main__":
    main()
