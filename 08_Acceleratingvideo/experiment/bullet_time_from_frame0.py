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


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--frame0', required=True)
    p.add_argument('--gaze-u', type=int, required=True)
    p.add_argument('--gaze-v', type=int, required=True)
    p.add_argument('--frames-dir', required=True)
    p.add_argument('--out-dir', required=True)
    p.add_argument('--sigma', type=float, default=3.0)
    args = p.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)

    # get image size from first frame
    from PIL import Image, ImageDraw
    im0 = Image.open(args.frame0)
    W, H = im0.size
    im0.close()


    # 初期化: フレーム0→現フレームの回転推定値
    R_prev = make_identity_R()

    # compute world vector from chosen pixel on frame0
    Gx, Gy, Gz = compute_G_from_pixel(args.gaze_u, args.gaze_v, W, H)
    print('G0 =', (Gx, Gy, Gz))

    # process all frames (ensure frame0 is first)
    frame_paths = sorted([p for p in glob(os.path.join(args.frames_dir, '*')) if os.path.isfile(p)])
    frame0_abs = os.path.abspath(args.frame0)
    frame_paths = [p for p in frame_paths if os.path.abspath(p) != frame0_abs]
    frame_paths.insert(0, frame0_abs)

    # 基準画像 Ib: frame0 を注視点中心にしたフルサイズ画像(固定)
    Ib_path = os.path.join(args.out_dir, 'Ib.jpg')
    try:
        omnigaze.generate_gaze_full_world(frame0_abs, Ib_path, Gx, Gy, Gz, make_identity_R())
        print(f'Created reference Ib: {Ib_path}')
    except Exception as e:
        print('Error creating Ib:', e)
        return

    base_path = Ib_path  # LM推定の基準画像(注視点中心フルサイズ、固定)
    print('Found', len(frame_paths), 'frames (frame0 forced first)')

    for i, fp in enumerate(frame_paths):
        fname = os.path.basename(fp)
        outp = os.path.join(args.out_dir, fname)

        if i == 0:
            # frame0: LM を行わず、単位行列の R で生成
            R = make_identity_R()
            try:
                omnigaze.generate_gaze_frame_world(fp, outp, Gx, Gy, Gz, R)
                print(f'Frame {i} (frame0): generated with identity R, R[:3]={R[:3]}')
                # draw centerlines and save separate file
                try:
                    imc = Image.open(outp)
                    Wc, Hc = imc.size
                    draw = ImageDraw.Draw(imc)
                    cx = Wc // 2
                    cy = Hc // 2
                    draw.line((cx, 0, cx, Hc), fill=(255,0,0), width=2)
                    draw.line((0, cy, Wc, cy), fill=(255,0,0), width=2)
                    root, ext = os.path.splitext(outp)
                    out_center = f"{root}_centerline{ext}"
                    imc.save(out_center)
                except Exception as _e:
                    print('Warning: failed to write centerline for frame0', _e)
            except Exception as e:
                print('Error generating gaze for frame0', fp, e)
            R_prev = R
            continue

        # フレーム1以降: base=フレーム0(固定), ref=現フレーム
        try:
            R = omnigaze.lm_estimate_frame(base_path, fp, R_prev, sigma=args.sigma)
            print(f'Frame {i}: LM succeeded, R[:3]={R[:3]}')
        except Exception as e:
            print(f'LM failed on {fp} -> reusing R_prev; error: {e}')
            R = R_prev

        # 推定した R をそのまま使って注視画像を生成 (累積なし)
        try:
            omnigaze.generate_gaze_frame_world(fp, outp, Gx, Gy, Gz, R)
            # draw centerlines into separate file for visual check
            try:
                imc = Image.open(outp)
                Wc, Hc = imc.size
                draw = ImageDraw.Draw(imc)
                cx = Wc // 2
                cy = Hc // 2
                draw.line((cx, 0, cx, Hc), fill=(255,0,0), width=2)
                draw.line((0, cy, Wc, cy), fill=(255,0,0), width=2)
                root, ext = os.path.splitext(outp)
                out_center = f"{root}_centerline{ext}"
                imc.save(out_center)
            except Exception as _e:
                print('Warning: failed to write centerline for', fp, _e)
        except Exception as e:
            print('Error generating gaze for', fp, e)

        R_prev = R

        if (i+1) % 10 == 0:
            print('Processed', i+1, 'frames')

    print('Done')


if __name__ == '__main__':
    main()
