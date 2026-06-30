#!/usr/bin/env python3
"""
バレットタイム用ドライバ（動画入力・インタラクティブ注視点選択・逐次追跡型）

手順:
  1. 動画からフレームを展開
  2. 1フレーム目を表示 → クリックで注視点を選択
  3. 逐次追跡型 LM で各フレームの注視画像（フルサイズ）を生成
  4. 中心線描画 → 中央切り取り → フレーム保存 + 動画出力

使い方:
  python3 bullet_time_video.py --video /path/to/input.mp4 --out-dir /path/to/out
"""
import argparse
import math
import os
import sys
import time

import cv2
import omnigaze


# ----------------------------------------------------------------
# 出力設定（参照スクリプト make_video.py に準拠）
# ----------------------------------------------------------------
CROSS_COLOR     = (0, 255, 0)   # 緑
CROSS_THICKNESS = 4
CROP_HALF_W     = 360           # 切り取り半幅 → 出力幅 720
CROP_HALF_H     = 180           # 切り取り半高 → 出力高 360


# ----------------------------------------------------------------
# 座標変換
# ----------------------------------------------------------------
def image_to_world(u, v, W, H):
    """画像座標 (u, v) → 単位球面上の世界座標 (X, Y, Z)"""
    theta   = (u - W / 2.0) * (2.0 * math.pi) / W
    phi     = -((v - H) * math.pi / H)
    sin_phi = math.sin(phi)
    return sin_phi * math.sin(theta), math.cos(phi), sin_phi * math.cos(theta)


# ----------------------------------------------------------------
# 描画・切り取り
# ----------------------------------------------------------------
def draw_cross(img, cx, cy):
    """画像中心に十字線を描画（参照スクリプトと同一）"""
    h, w = img.shape[:2]
    cv2.line(img, (0, cy),   (w - 1, cy), CROSS_COLOR, CROSS_THICKNESS)
    cv2.line(img, (cx, 0),   (cx, h - 1), CROSS_COLOR, CROSS_THICKNESS)


def crop_center(img, half_w, half_h):
    """画像中心を 2*half_w × 2*half_h で切り取る"""
    h, w = img.shape[:2]
    cx, cy = w // 2, h // 2
    x0 = max(0, cx - half_w);  x1 = min(w, cx + half_w)
    y0 = max(0, cy - half_h);  y1 = min(h, cy + half_h)
    return img[y0:y1, x0:x1]


# ----------------------------------------------------------------
# インタラクティブ注視点選択
# ----------------------------------------------------------------
def select_gaze_point(frame_bgr):
    """最初のフレームを OpenCV ウィンドウで表示し、
    マウスクリックで注視点ピクセル座標を1点取得する。
    Enter で確定、q でキャンセル。"""
    points = []

    def on_mouse(event, x, y, _flags, _param):
        if event == cv2.EVENT_LBUTTONDOWN:
            points.clear()
            points.append((x, y))
            print(f'  クリック: ({x}, {y})')

    win = 'select_gaze'
    cv2.namedWindow(win, cv2.WINDOW_NORMAL)
    cv2.setMouseCallback(win, on_mouse)
    print('【注視点選択】 1フレーム目を表示中')
    print('  ・注視させたい点をクリック（赤十字で確認）')
    print('  ・Enter: 確定  /  q: キャンセル')

    while True:
        disp = frame_bgr.copy()
        if points:
            cv2.drawMarker(disp, points[0], (0, 0, 255),
                           cv2.MARKER_CROSS, 30, 3)
        cv2.imshow(win, disp)
        key = cv2.waitKey(30) & 0xFF
        if key == ord('q'):
            cv2.destroyAllWindows()
            return None
        if key == 13 and points:   # Enter
            break

    cv2.destroyAllWindows()
    return points[0]


# ----------------------------------------------------------------
# メイン
# ----------------------------------------------------------------
def main():
    p = argparse.ArgumentParser(
        description='動画からバレットタイム映像を生成（逐次追跡型）')
    p.add_argument('--video',        required=True,  help='入力動画ファイル')
    p.add_argument('--out-dir',      required=True,  help='出力ディレクトリ')
    p.add_argument('--frames-dir',   default=None,
                   help='フレーム展開先 (省略時は out-dir/frames)')
    p.add_argument('--sigma',        type=float, default=3.0,
                   help='LM用 Gaussian ブラー σ (デフォルト 3.0)')
    p.add_argument('--crop-half-w',  type=int, default=CROP_HALF_W,
                   help=f'切り取り半幅px (デフォルト {CROP_HALF_W} → 出力幅{CROP_HALF_W*2})')
    p.add_argument('--crop-half-h',  type=int, default=CROP_HALF_H,
                   help=f'切り取り半高px (デフォルト {CROP_HALF_H} → 出力高{CROP_HALF_H*2})')
    args = p.parse_args()

    frames_dir  = args.frames_dir or os.path.join(args.out_dir, 'frames')
    gaze_dir    = os.path.join(args.out_dir, 'gaze_frames')    # フルサイズ注視画像
    output_dir  = os.path.join(args.out_dir, 'output_frames')  # 中心線+切り取り済みフレーム
    video_out   = os.path.join(args.out_dir, 'output.mp4')
    for d in [frames_dir, gaze_dir, output_dir]:
        os.makedirs(d, exist_ok=True)

    crop_hw = args.crop_half_w
    crop_hh = args.crop_half_h
    out_w   = crop_hw * 2
    out_h   = crop_hh * 2

    print('=====================================')
    print('  全天球バレットタイム処理プログラム')
    print('  （動画入力・逐次追跡型）')
    print('=====================================\n')

    # ── 動画情報取得 ──────────────────────────────────────────────
    cap = cv2.VideoCapture(args.video)
    if not cap.isOpened():
        print(f'エラー: 動画を開けません: {args.video}')
        sys.exit(1)
    fps     = cap.get(cv2.CAP_PROP_FPS)
    W       = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    H       = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    n_total = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    print(f'入力動画    : {args.video}')
    print(f'解像度      : {W}×{H} px')
    print(f'fps         : {fps:.2f}')
    print(f'フレーム数  : {n_total}')
    print(f'出力切り取り: {out_w}×{out_h} px (中央 {crop_hw}px×{crop_hh}px 半幅)\n')

    # ── フレーム展開 ──────────────────────────────────────────────
    existing = sorted(f for f in os.listdir(frames_dir) if f.lower().endswith('.jpg'))
    if existing:
        print(f'フレーム展開済み ({len(existing)}枚): {frames_dir}\n')
        frame_paths = [os.path.join(frames_dir, f) for f in existing]
        cap.release()
    else:
        print('フレーム展開中...')
        frame_paths = []
        idx = 0
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            out_p = os.path.join(frames_dir, f'frame_{idx:04d}.jpg')
            cv2.imwrite(out_p, frame, [cv2.IMWRITE_JPEG_QUALITY, 95])
            frame_paths.append(out_p)
            idx += 1
        cap.release()
        print(f'展開完了: {len(frame_paths)}枚 → {frames_dir}\n')

    if not frame_paths:
        print('エラー: フレームが取得できませんでした')
        sys.exit(1)

    # ── 注視点インタラクティブ選択 ──────────────────────────────
    first_frame = cv2.imread(frame_paths[0])
    if first_frame is None:
        print(f'エラー: 最初のフレームを読み込めません: {frame_paths[0]}')
        sys.exit(1)

    gaze_px = select_gaze_point(first_frame)
    if gaze_px is None:
        print('キャンセルされました')
        sys.exit(0)

    gaze_u, gaze_v = gaze_px
    Gx, Gy, Gz = image_to_world(gaze_u, gaze_v, W, H)
    print(f'\n注視点 画素    : ({gaze_u}, {gaze_v})')
    print(f'注視点 世界座標: ({Gx:.5f}, {Gy:.5f}, {Gz:.5f})\n')

    # ── 初期回転行列 ─────────────────────────────────────────────
    R_0 = omnigaze.get_initial_R(Gx, Gy, Gz)

    # ── 動画ライター（切り取りサイズで初期化） ───────────────────
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    writer = cv2.VideoWriter(video_out, fourcc, fps, (out_w, out_h))
    if not writer.isOpened():
        print('エラー: VideoWriter の作成に失敗しました')
        sys.exit(1)

    # ── 逐次追跡 + 中心線 + 切り取りループ ───────────────────────
    total_frames = len(frame_paths)
    base_path    = None
    R_prev       = R_0
    start_all    = time.perf_counter()

    for i, fp in enumerate(frame_paths):
        t_start = time.perf_counter()
        fname   = os.path.basename(fp)
        gaze_p  = os.path.join(gaze_dir, fname)        # フルサイズ注視画像
        out_p   = os.path.join(output_dir, f'frame_{i:05d}.jpg')  # 切り取り済み

        # ── 注視画像生成 ──
        if i == 0:
            print(f'[フレーム   0/{total_frames-1}] R_0 で直接生成 (LM不要)')
            omnigaze.generate_gaze_sequential(fp, gaze_p, R_0)
            base_path = gaze_p
            R_prev    = R_0
        else:
            print(f'[フレーム {i:3d}/{total_frames-1}] LM最適化中...', end=' ', flush=True)
            try:
                R = omnigaze.lm_estimate_frame(base_path, fp, R_prev, sigma=args.sigma)
            except Exception as e:
                print(f'失敗({e})→前R流用  ', end='')
                R = R_prev
            omnigaze.generate_gaze_sequential(fp, gaze_p, R)
            base_path = gaze_p
            R_prev    = R

        # ── 中心線描画 → 中央切り取り → 保存 + 動画書き込み ──
        gaze_img = cv2.imread(gaze_p)
        if gaze_img is not None:
            cx, cy = gaze_img.shape[1] // 2, gaze_img.shape[0] // 2
            draw_cross(gaze_img, cx, cy)
            cropped = crop_center(gaze_img, crop_hw, crop_hh)
            cv2.imwrite(out_p, cropped, [cv2.IMWRITE_JPEG_QUALITY, 95])
            writer.write(cropped)

        t_ms = (time.perf_counter() - t_start) * 1000
        print(f'✓ {fname}  [{t_ms:.0f}ms]')

    writer.release()

    total_s = time.perf_counter() - start_all
    print(f'\n✓ 全処理完了  総時間: {total_s:.1f}s')
    print(f'注視フレーム (フル) : {gaze_dir}')
    print(f'出力フレーム (切取) : {output_dir}')
    print(f'出力動画            : {video_out}')


if __name__ == '__main__':
    main()
