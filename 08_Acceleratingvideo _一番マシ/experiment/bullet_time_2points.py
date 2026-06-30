#!/usr/bin/env python3
"""
2点方式バレットタイムドライバ。

注視点(gaze-u, gaze-v)と補助点(ref-u, ref-v)の2点から初期回転 R_0 を決定し、
全フレームにわたって注視点が中心に固定された注視画像を生成する。

使い方:
  python3 bullet_time_2points.py \
    --frame0   /path/to/frames/frame_0000.jpg \
    --gaze-u 2477 --gaze-v 1504 \
    --ref-u  2600 --ref-v  1504 \
    --frames-dir /path/to/frames \
    --out-dir    /path/to/out \
    --sigma 3.0
"""
import os
import sys
import time
import argparse
from glob import glob
from PIL import Image, ImageDraw
import omnigaze


def make_identity_R():
    return [1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0]


def draw_centerlines(img_path):
    """画像に中心線を描いた _centerline 版を保存する。"""
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
    p.add_argument('--frame0',     required=True,  help='基準フレーム(第0フレーム)のパス')
    p.add_argument('--gaze-u',     type=int, required=True,  help='注視点の画像 u 座標')
    p.add_argument('--gaze-v',     type=int, required=True,  help='注視点の画像 v 座標')
    p.add_argument('--ref-u',      type=int, required=True,  help='補助点の画像 u 座標')
    p.add_argument('--ref-v',      type=int, required=True,  help='補助点の画像 v 座標')
    p.add_argument('--frames-dir', required=True,  help='全フレームが入ったディレクトリ')
    p.add_argument('--out-dir',    required=True,  help='出力ディレクトリ')
    p.add_argument('--sigma',      type=float, default=3.0, help='LM 用ガウシアン σ')
    args = p.parse_args()

    print('=====================================')
    print('  全天球バレットタイム処理プログラム')
    print('=====================================\n')

    os.makedirs(args.out_dir, exist_ok=True)
    print(f'処理パラメータ:')
    print(f'  基準フレーム    : {args.frame0}')
    print(f'  フレームDir     : {args.frames_dir}')
    print(f'  出力Dir         : {args.out_dir}')
    print(f'  注視点          : ({args.gaze_u}, {args.gaze_v})')
    print(f'  参照点          : ({args.ref_u}, {args.ref_v})')
    print(f'  LM σ           : {args.sigma}\n')

    frame0_abs = os.path.abspath(args.frame0)

    # ── 初期回転 R_0 を2点方式で計算 ──────────────────────────────
    print('[初期回転 R_0] 2点方式で計算中 ...')
    R_0 = omnigaze.calc_R_2points(
        frame0_abs,
        args.gaze_u, args.gaze_v,
        args.ref_u,  args.ref_v,
    )
    print(f'  R_0 (row-major 9要素) = {[f"{v:.5f}" for v in R_0]}\n')

    # ── 基準画像 Ib を生成 (R_cum = I) ────────────────────────────
    Ib_path = os.path.join(args.out_dir, 'Ib.jpg')
    print(f'[Ib 生成] 第0フレームを R_0 で回転した全天球基準画像')
    print(f'  → {Ib_path}')
    omnigaze.generate_gaze_full_R0(frame0_abs, Ib_path, R_0, make_identity_R())
    draw_centerlines(Ib_path)
    print('  → Ib_centerline.jpg も生成 (中心に注視点が来ているか確認用)\n')

    # ── フレームリストを構築(frame0 を先頭に) ──────────────────────
    frame_paths = sorted([
        f for f in glob(os.path.join(args.frames_dir, '*'))
        if os.path.isfile(f)
    ])
    frame_paths = [f for f in frame_paths if os.path.abspath(f) != frame0_abs]
    frame_paths.insert(0, frame0_abs)
    total = len(frame_paths)
    print(f'フレーム一覧: {total} フレーム (frame0 先頭固定)\n')

    base_path = Ib_path   # LM 推定の基準画像(固定)
    R_prev = make_identity_R()
    start_all = time.perf_counter()

    for i, fp in enumerate(frame_paths):
        fname = os.path.basename(fp)
        outp  = os.path.join(args.out_dir, fname)
        t_start = time.perf_counter()

        if i == 0:
            # frame0: LM 不要、R_cum = I で Ib と同じ見かけのクリップを生成
            print(f'[フレーム  0/{total-1}] frame0 (LM不要・R_cum = 単位行列)')
            omnigaze.generate_gaze_frame_R0(fp, outp, R_0, make_identity_R())
            draw_centerlines(outp)
            R_prev = make_identity_R()
            t_ms = (time.perf_counter() - t_start) * 1000
            print(f'✓ {fname}  [{t_ms:.0f}ms]\n')
            continue

        # frame 1 以降: Ib を基準として LM 推定
        print(f'[フレーム {i:2d}/{total-1}] LM最適化中 (基準画像=Ib, 参照=現フレーム)')
        sys.stdout.flush()  # [LM] ログ(C側)より先に表示させる
        try:
            R = omnigaze.lm_estimate_frame(base_path, fp, R_prev, sigma=args.sigma)
            # ↑ C側から "[LM] iters=... E_init=... E_final=... dE=... converged=..." が出力される
            print(f'  R_cum[:3] (推定累積回転行列の先頭3成分): {[f"{v:.5f}" for v in R[:3]]}')
        except Exception as e:
            print(f'  LM失敗 ({e})、前フレームの R_cum を流用')
            R = R_prev

        # 推定した R_cum を用いて注視画像を生成
        try:
            omnigaze.generate_gaze_frame_R0(fp, outp, R_0, R)
        except Exception as e:
            print(f'  注視画像生成失敗 ({e})')

        draw_centerlines(outp)
        R_prev = R

        t_ms = (time.perf_counter() - t_start) * 1000
        print(f'✓ {fname}  [{t_ms:.0f}ms]\n')

    total_ms = (time.perf_counter() - start_all) * 1000
    print(f'✓ 全処理完了  総時間: {total_ms:.0f}ms')


if __name__ == '__main__':
    main()
