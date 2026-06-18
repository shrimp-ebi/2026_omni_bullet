#!/usr/bin/env python3
import os
import sys
import argparse
import glob
from pathlib import Path

sys.path.append(os.path.dirname(__file__))
import omnigaze


def process_frames(frames_dir, ref_path, out_dir, max_frames=None, verbose=True):
    frames = sorted(glob.glob(os.path.join(frames_dir, '*')))
    if max_frames:
        frames = frames[:max_frames]
    os.makedirs(out_dir, exist_ok=True)

    for i, frame in enumerate(frames, start=1):
        name = os.path.basename(frame)
        out_path = os.path.join(out_dir, name)
        if verbose:
            print(f'[{i}/{len(frames)}] Processing {name}')

        # estimate rotation from this base frame to the reference
        R_init = [1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0]
        try:
            R_out = omnigaze.lm_estimate_frame(frame, ref_path, R_init, sigma=3.0)
        except Exception as e:
            print(f'  lm_estimate_frame failed for {name}:', e)
            continue

        # determine center coords for gaze (use image_utils via C side? just compute from filename using PIL)
        try:
            from PIL import Image
            im = Image.open(frame)
            w, h = im.size
            u_g = w // 2
            v_g = h // 2
            im.close()
        except Exception:
            # fallback to (100,100)
            u_g, v_g = 100, 100

        try:
            omnigaze.generate_gaze_frame(frame, out_path, u_g, v_g, R_out)
            if verbose:
                print('  wrote', out_path)
        except Exception as e:
            print(f'  generate_gaze_frame failed for {name}:', e)


def main():
    p = argparse.ArgumentParser(description='Generate gaze images for frames using libomnigaze')
    p.add_argument('frames_dir', nargs='?', default='/home/y233324/ドキュメント/2026_注視画像生成/Code/2026_omni_bullet/07_video_pipeline/videos/output/frames')
    p.add_argument('--ref', default='/home/y233324/ドキュメント/2026_注視画像生成/Code/2026_omni_bullet/07_video_pipeline/images/reference/reference_30deg.jpg')
    p.add_argument('--out', default=os.path.join(os.path.dirname(__file__), '..', 'build', 'video_gaze_output'))
    p.add_argument('--max', type=int, default=0, help='Maximum number of frames to process (0 = all)')
    args = p.parse_args()

    frames_dir = os.path.normpath(args.frames_dir)
    ref = os.path.normpath(args.ref)
    out_dir = os.path.normpath(args.out)
    max_frames = args.max if args.max > 0 else None

    if not os.path.isdir(frames_dir):
        print('frames_dir not found:', frames_dir)
        sys.exit(2)
    if not os.path.exists(ref):
        print('reference image not found:', ref)
        sys.exit(2)

    process_frames(frames_dir, ref, out_dir, max_frames=max_frames)


if __name__ == '__main__':
    main()
