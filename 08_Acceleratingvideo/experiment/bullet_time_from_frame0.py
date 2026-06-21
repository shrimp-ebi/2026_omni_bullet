#!/usr/bin/env python3
"""
バレットタイム用ドライバ: frame0 で視線点を選び，その世界方向を使って
全フレームに対して同じ世界方向を中心に出力を作る。

使い方の例:
  python3 bullet_time_from_frame0.py \
    --frame0 /path/to/frames/frame_0000.jpg \
    --gaze-u 640 --gaze-v 360 \
    --frames-dir /path/to/frames \
    --out-dir /path/to/out \
    --ref /path/to/reference.jpg
"""
import os
import sys
import time
import argparse
from glob import glob
from math import sin, cos, pi
import omnigaze


def image_to_angle(u, v, W, H):
    theta = (u - W / 2.0) * (2.0 * pi) / W
    phi = -((v - H) * pi / H)
    return theta, phi


def angle_to_world(theta, phi):
    sin_phi = sin(phi)
    x = sin_phi * sin(theta)
    y = cos(phi)
    z = sin_phi * cos(theta)
    return x, y, z


def compute_G_from_pixel(u, v, W, H):
    theta, phi = image_to_angle(u, v, W, H)
    return angle_to_world(theta, phi)


def make_identity_R():
    # row-major 3x3 identity
    return [1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0]


def draw_centerlines(img_path):
    from PIL import Image, ImageDraw
    try:
        imc = Image.open(img_path)
        Wc, Hc = imc.size
        draw = ImageDraw.Draw(imc)
        cx, cy = Wc // 2, Hc // 2
        draw.line((cx, 0, cx, Hc), fill=(255, 0, 0), width=2)
        draw.line((0, cy, Wc, cy), fill=(255, 0, 0), width=2)
        root, ext = os.path.splitext(img_path)
        imc.save(f"{root}_centerline{ext}")
    except Exception as e:
        print(f'Warning: centerline draw failed for {img_path}: {e}')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--frame0', required=True)
    p.add_argument('--gaze-u', type=int, required=True)
    p.add_argument('--gaze-v', type=int, required=True)
    p.add_argument('--frames-dir', required=True)
    p.add_argument('--out-dir', required=True)
    p.add_argument('--sigma', type=float, default=3.0)
    args = p.parse_args()

    print('=====================================')
    print('  全天球バレットタイム処理プログラム')
    print('=====================================\n')

    os.makedirs(args.out_dir, exist_ok=True)
    print('処理パラメータ:')
    print(f'  基準フレーム    : {args.frame0}')
    print(f'  フレームDir     : {args.frames_dir}')
    print(f'  出力Dir         : {args.out_dir}')
    print(f'  注視点          : ({args.gaze_u}, {args.gaze_v})')
    print(f'  参照点          : 上ベクトル (0,1,0) を自動使用')
    print(f'  LM σ           : {args.sigma}\n')

    from PIL import Image
    im0 = Image.open(args.frame0)
    W, H = im0.size
    im0.close()

    # 注視点の世界座標ベクトル G を計算
    Gx, Gy, Gz = compute_G_from_pixel(args.gaze_u, args.gaze_v, W, H)
    print(f'[注視方向ベクトル G] 画素 ({args.gaze_u},{args.gaze_v}) → G = ({Gx:.5f}, {Gy:.5f}, {Gz:.5f})\n')

    # フレームリスト構築
    frame_paths = sorted([fp for fp in glob(os.path.join(args.frames_dir, '*')) if os.path.isfile(fp)])
    frame0_abs = os.path.abspath(args.frame0)
    frame_paths = [fp for fp in frame_paths if os.path.abspath(fp) != frame0_abs]
    frame_paths.insert(0, frame0_abs)
    total = len(frame_paths)

    # 基準画像 Ib: frame0 を注視点中心にしたフルサイズ画像(固定)
    Ib_path = os.path.join(args.out_dir, 'Ib.jpg')
    print(f'[Ib 生成] 第0フレームを G 方向中心に回転した全天球基準画像')
    print(f'  → {Ib_path}')
    try:
        omnigaze.generate_gaze_full_world(frame0_abs, Ib_path, Gx, Gy, Gz, make_identity_R())
        draw_centerlines(Ib_path)
        print('  → Ib_centerline.jpg も生成 (中心に注視点が来ているか確認用)\n')
    except Exception as e:
        print('  Ib 生成失敗:', e)
        return

    print(f'フレーム一覧: {total} フレーム (frame0 先頭固定)\n')

    base_path = Ib_path
    R_prev = make_identity_R()
    start_all = time.perf_counter()

    for i, fp in enumerate(frame_paths):
        fname = os.path.basename(fp)
        outp = os.path.join(args.out_dir, fname)
        t_start = time.perf_counter()

        if i == 0:
            print(f'[フレーム  0/{total-1}] frame0 (LM不要・R_cum = 単位行列)')
            try:
                omnigaze.generate_gaze_frame_world(fp, outp, Gx, Gy, Gz, make_identity_R())
                draw_centerlines(outp)
            except Exception as e:
                print(f'  生成失敗: {e}')
            R_prev = make_identity_R()
            t_ms = (time.perf_counter() - t_start) * 1000
            print(f'✓ {fname}  [{t_ms:.0f}ms]\n')
            continue

        print(f'[フレーム {i:2d}/{total-1}] LM最適化中 (基準画像=Ib, 参照=現フレーム)')
        sys.stdout.flush()
        try:
            R = omnigaze.lm_estimate_frame(base_path, fp, R_prev, sigma=args.sigma)
            # ↑ C側から "[LM] iters=... E_init=... E_final=... dE=... converged=..." が出力される
            print(f'  R_cum[:3] (推定累積回転行列の先頭3成分): {[f"{v:.5f}" for v in R[:3]]}')
        except Exception as e:
            print(f'  LM失敗 ({e})、前フレームの R_cum を流用')
            R = R_prev

        try:
            omnigaze.generate_gaze_frame_world(fp, outp, Gx, Gy, Gz, R)
            draw_centerlines(outp)
        except Exception as e:
            print(f'  注視画像生成失敗: {e}')

        R_prev = R
        t_ms = (time.perf_counter() - t_start) * 1000
        print(f'✓ {fname}  [{t_ms:.0f}ms]\n')

    total_ms = (time.perf_counter() - start_all) * 1000
    print(f'✓ 全処理完了  総時間: {total_ms:.0f}ms')


if __name__ == '__main__':
    main()
