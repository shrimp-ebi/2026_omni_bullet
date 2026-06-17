#!/usr/bin/env python3
"""
video_pipeline.py
全方位動画から注視画像を生成するパイプライン。

処理の流れ:
  1. 動画からフレームを取り出して保存
  2. 1フレーム目をウィンドウ表示、マウスクリックで注視点指定
  3. 注視点から R_gaze を計算
  4. フレームごとに lm_estimate_frame で R を推定し、注視画像を生成
  5. gaze_frames から output.mp4 を生成
  6. 推定ログを results/estimation_log.csv に保存
"""

import argparse
import csv
import math
import os
import subprocess
import sys
import tempfile
import time

import cv2
import numpy as np

BINARY     = "./build/lm_estimate_frame"
INPUT_MP4  = "videos/input/input.mp4"
FRAMES_DIR = "videos/output/frames"
GAZE_DIR   = "videos/output/gaze_frames"
OUTPUT_MP4 = "videos/output/output.mp4"
LOG_CSV    = "results/estimation_log.csv"

GAZE_W = 1440
GAZE_H = 720


# ── 座標変換 ─────────────────────────────────────────────────────────────────

def image_to_world(u, v, W, H):
    """画像座標 → 単位球面上の3D点（NumPy配列）"""
    theta = (u - W / 2.0) * 2.0 * math.pi / W
    phi   = -(v - H) * math.pi / H
    sp = math.sin(phi)
    return np.array([sp * math.sin(theta), math.cos(phi), sp * math.cos(theta)])


def image_to_world_grid(W, H):
    """全画素の world 座標を (H, W, 3) 配列で一括生成"""
    u_idx = np.arange(W, dtype=np.float64)
    v_idx = np.arange(H, dtype=np.float64)
    theta = (u_idx - W / 2.0) * 2.0 * math.pi / W          # (W,)
    phi   = -(v_idx - H) * math.pi / H                       # (H,)

    sp = np.sin(phi)                                          # (H,)
    X  = np.outer(sp, np.sin(theta))                         # (H, W)
    Y  = np.tile(np.cos(phi)[:, None], (1, W))               # (H, W)
    Z  = np.outer(sp, np.cos(theta))                         # (H, W)
    return np.stack([X, Y, Z], axis=2)                       # (H, W, 3)


def world_to_image_coords(xyz, W, H):
    """
    xyz: (H, W, 3) 配列
    戻り値: u_f, v_f それぞれ (H, W) の float 配列
    """
    X, Y, Z = xyz[..., 0], xyz[..., 1], xyz[..., 2]
    Y_clipped = np.clip(Y, -1.0, 1.0)
    theta = np.arctan2(X, Z)
    phi   = np.arccos(Y_clipped)

    u_f = (theta + math.pi) * W / (2.0 * math.pi)
    v_f = -(phi - math.pi) * H / math.pi
    return u_f, v_f


# ── 注視回転行列 ─────────────────────────────────────────────────────────────

def compute_R_gaze(theta_g, phi_g):
    """
    注視点(theta_g, phi_g)方向に Z 軸を向ける回転行列を返す。
    R_gaze の各行が [ex, ey, ez]。
    """
    sp = math.sin(phi_g)
    ez = np.array([sp * math.sin(theta_g),
                   math.cos(phi_g),
                   sp * math.cos(theta_g)])

    up = np.array([0.0, 1.0, 0.0])
    ex = np.cross(up, ez)
    norm_ex = np.linalg.norm(ex)
    if norm_ex < 1e-8:
        up = np.array([1.0, 0.0, 0.0])
        ex = np.cross(up, ez)
        norm_ex = np.linalg.norm(ex)
    ex /= norm_ex

    ey = np.cross(ez, ex)
    ey /= np.linalg.norm(ey)

    R = np.stack([ex, ey, ez], axis=0)   # (3, 3) 各行が ex, ey, ez
    return R


# ── フレーム処理 ──────────────────────────────────────────────────────────────

def apply_rotation_to_frame(frame_bgr, R_final):
    """
    R_final: (3, 3) NumPy 配列
    frame_bgr: (H, W, 3) カラー画像
    戻り値: (H, W, 3) 回転後のカラー画像
    """
    H, W = frame_bgr.shape[:2]

    # 出力画素 → ワールド座標
    xyz_out = image_to_world_grid(W, H)                      # (H, W, 3)

    # R_final^T で逆変換（出力→入力）
    RT = R_final.T
    xyz_src = (xyz_out @ RT.T)                                  # (H, W, 3)

    # ワールド座標 → 入力画像座標
    u_src, v_src = world_to_image_coords(xyz_src, W, H)

    # 水平方向の周期境界
    u_src = u_src % W

    # バイリニア補間（cv2.remap）
    map_x = u_src.astype(np.float32)
    map_y = v_src.astype(np.float32)
    out = cv2.remap(frame_bgr, map_x, map_y,
                    interpolation=cv2.INTER_LINEAR,
                    borderMode=cv2.BORDER_REPLICATE)
    return out


def crop_center(img, W, H):
    """中央領域 (W/4:3W/4, H/4:3H/4) を切り出して元解像度で返す"""
    h, w = img.shape[:2]
    x0, x1 = w // 4, 3 * w // 4
    y0, y1 = h // 4, 3 * h // 4
    cropped = img[y0:y1, x0:x1]
    return cv2.resize(cropped, (W, H), interpolation=cv2.INTER_LINEAR)


# ── LM推定（Cバイナリ呼び出し） ───────────────────────────────────────────────

def run_lm_estimate_frame(base_gray_path, ref_gray_path, R_init, sigma,
                          timeout=300, frame_idx=None):
    """
    lm_estimate_frame を呼び出して推定済み R (3×3 NumPy 配列) を返す。
    失敗した場合は R_init をそのまま返す。
    """
    r_args = [f"{R_init[i, j]:.10f}"
              for i in range(3) for j in range(3)]
    cmd = [BINARY, base_gray_path, ref_gray_path] + r_args + [str(sigma)]

    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        if proc.returncode == 0:
            line = proc.stdout.strip()
            vals = [float(x) for x in line.split(",")]
            if len(vals) == 9:
                return np.array(vals).reshape(3, 3)
    except subprocess.TimeoutExpired:
        print(f"  警告: frame{frame_idx}でタイムアウト → 前フレームのRを使用",
              file=sys.stderr)
    except Exception as e:
        print(f"  警告: {e}", file=sys.stderr)

    return R_init.copy()


# ── ステップ1: フレーム抽出 ────────────────────────────────────────────────────

def extract_frames():
    os.makedirs(FRAMES_DIR, exist_ok=True)
    cap = cv2.VideoCapture(INPUT_MP4)
    if not cap.isOpened():
        print(f"エラー: {INPUT_MP4} を開けません")
        sys.exit(1)

    fps   = cap.get(cv2.CAP_PROP_FPS)
    total = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    print(f"動画: {INPUT_MP4}  fps={fps:.1f}  総フレーム数={total}")

    frame_paths = []
    idx = 0
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        path = os.path.join(FRAMES_DIR, f"frame_{idx:04d}.jpg")
        cv2.imwrite(path, frame)
        frame_paths.append(path)
        idx += 1
    cap.release()
    print(f"フレーム保存完了: {idx} 枚 → {FRAMES_DIR}/")
    return frame_paths, fps


# ── ステップ2: 注視点指定 ─────────────────────────────────────────────────────

def select_gaze_point(frame_path):
    frame = cv2.imread(frame_path)
    H, W  = frame.shape[:2]
    clicked = {}

    def on_mouse(event, x, y, _flags, _param):
        if event == cv2.EVENT_LBUTTONDOWN:
            clicked["u"] = x
            clicked["v"] = y
            print(f"クリック: u={x}, v={y}")

    win = "window"
    cv2.namedWindow(win, cv2.WINDOW_KEEPRATIO)
    cv2.resizeWindow(win, frame.shape[1], frame.shape[0])
    cv2.setMouseCallback(win, on_mouse)
    cv2.imshow(win, frame)

    print("1フレーム目を表示中。注視点をクリックし Enter を押してください。")
    while True:
        key = cv2.waitKey(30) & 0xFF
        if key == 13 and "u" in clicked:   # Enter
            break
        if key == 27:                       # Esc → 中央
            clicked["u"] = W // 2
            clicked["v"] = H // 2
            break
    cv2.destroyWindow(win)

    u_g, v_g = clicked.get("u", W // 2), clicked.get("v", H // 2)

    theta_g = (u_g - W / 2.0) * 2.0 * math.pi / W
    phi_g   = -(v_g - H) * math.pi / H

    print(f"注視点: u={u_g}, v={v_g}  →  θ={math.degrees(theta_g):.2f}°, φ={math.degrees(phi_g):.2f}°")
    return theta_g, phi_g, W, H


# ── ステップ4–5: フレーム処理と動画生成 ──────────────────────────────────────

def process_frames(frame_paths, R_gaze, W, H, sigma, output_fps):
    os.makedirs(GAZE_DIR, exist_ok=True)
    os.makedirs("results", exist_ok=True)

    # 池内方式: LM法がRそのものを推定する
    # フレーム0の初期値はR_gaze（注視点が中央に来る回転行列）
    # フレームNの初期値は前フレームで推定したR（池内論文 アルゴリズム 手順1）
    R_prev = R_gaze.copy()
    prev_gray_path = None

    log_rows = []

    for idx, fpath in enumerate(frame_paths):
        t0 = time.time()
        frame_bgr = cv2.imread(fpath)

        # グレー変換して一時保存
        gray = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2GRAY)
        cur_gray_path = os.path.join(tempfile.gettempdir(), f"lm_gray_{idx:04d}.jpg")
        cv2.imwrite(cur_gray_path, gray)

        if idx == 0:
            # フレーム0: LM法は使わずR_gazeをそのまま使用
            R = R_gaze.copy()
        else:
            # フレームN: 前フレームのRを初期値としてLM法を実行
            # base=前フレームのグレー画像, ref=現フレームのグレー画像
            R = run_lm_estimate_frame(prev_gray_path, cur_gray_path, R_prev, sigma,
                                      frame_idx=idx)

        # 前フレームのグレー画像を削除（使い終わった）
        if prev_gray_path is not None:
            try:
                os.remove(prev_gray_path)
            except OSError:
                pass

        # 現フレームのグレー画像を次回のbaseに、現フレームのRを次回の初期値に
        prev_gray_path = cur_gray_path
        R_prev = R.copy()

        # 池内方式: LM法が返したRで直接注視画像を生成（R_gazeとの合成は不要）
        R_final = R

        # カラーフレームを回転
        rotated = apply_rotation_to_frame(frame_bgr, R_final)

        # 中央領域を元解像度に
        gaze_img = crop_center(rotated, W, H)

        # 保存（元解像度）
        gaze_path = os.path.join(GAZE_DIR, f"gaze_{idx:04d}.jpg")
        cv2.imwrite(gaze_path, gaze_img)

        # 表示用: 1440×720 に縮小して十字線描画
        disp = cv2.resize(gaze_img, (GAZE_W, GAZE_H), interpolation=cv2.INTER_LINEAR)
        cx, cy = GAZE_W // 2, GAZE_H // 2
        cv2.line(disp, (cx, 0), (cx, GAZE_H), (0, 0, 255), 2)
        cv2.line(disp, (0, cy), (GAZE_W, cy), (0, 0, 255), 2)
        disp_path = os.path.join(GAZE_DIR, f"disp_{idx:04d}.jpg")
        cv2.imwrite(disp_path, disp)

        elapsed = time.time() - t0
        R_flat = R.flatten().tolist()
        log_rows.append([idx] + R_flat + [f"{elapsed:.3f}"])
        print(f"  frame {idx:4d}/{len(frame_paths)}  elapsed={elapsed:.2f}s")

    # 最後のグレー画像を削除
    if prev_gray_path is not None:
        try:
            os.remove(prev_gray_path)
        except OSError:
            pass

    # CSV保存
    with open(LOG_CSV, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["frame",
                          "r00","r01","r02","r10","r11","r12","r20","r21","r22",
                          "elapsed_sec"])
        writer.writerows(log_rows)
    print(f"推定ログ保存: {LOG_CSV}")

    # output.mp4 生成
    make_video(GAZE_DIR, output_fps)


def make_video(frames_dir, fps):
    files = sorted([
        f for f in os.listdir(frames_dir)
        if f.startswith("disp_") and f.endswith(".jpg")
    ])
    if not files:
        print("エラー: 動画フレームが見つかりません")
        return

    first = cv2.imread(os.path.join(frames_dir, files[0]))
    h, w = first.shape[:2]

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    out = cv2.VideoWriter(OUTPUT_MP4, fourcc, fps, (w, h))

    for fname in files:
        img = cv2.imread(os.path.join(frames_dir, fname))
        if img is not None:
            out.write(img)

    out.release()
    print(f"動画生成完了: {OUTPUT_MP4}")


# ── メイン ───────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="全方位動画 注視画像生成パイプライン")
    parser.add_argument("--sigma",      type=float, default=3.0)
    parser.add_argument("--output_fps", type=float, default=30.0)
    parser.add_argument("--gaze_u",     type=int,   default=None)
    parser.add_argument("--gaze_v",     type=int,   default=None)
    args = parser.parse_args()

    if not os.path.exists(BINARY):
        print(f"エラー: {BINARY} が見つかりません。make lm_estimate_frame を実行してください。")
        sys.exit(1)

    # ステップ1: フレーム抽出
    frame_paths, video_fps = extract_frames()
    if not frame_paths:
        print("エラー: フレームを取り出せませんでした")
        sys.exit(1)

    # ステップ2: 注視点指定
    if args.gaze_u is not None and args.gaze_v is not None:
        first = cv2.imread(frame_paths[0])
        H, W = first.shape[:2]
        theta_g = (args.gaze_u - W / 2.0) * 2.0 * math.pi / W
        phi_g   = -(args.gaze_v - H) * math.pi / H
        print(f"注視点（引数指定）: u={args.gaze_u}, v={args.gaze_v}  "
              f"→  θ={math.degrees(theta_g):.2f}°, φ={math.degrees(phi_g):.2f}°")
    else:
        theta_g, phi_g, W, H = select_gaze_point(frame_paths[0])

    # ステップ3: R_gaze を計算
    R_gaze = compute_R_gaze(theta_g, phi_g)
    print(f"R_gaze:\n{R_gaze}")

    # ステップ4–5: フレーム処理と動画生成
    fps_out = args.output_fps if args.output_fps > 0 else video_fps
    process_frames(frame_paths, R_gaze, W, H, args.sigma, fps_out)

    print("\nパイプライン完了")


if __name__ == "__main__":
    main()