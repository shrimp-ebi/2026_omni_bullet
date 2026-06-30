#!/usr/bin/env python3
"""バレットタイム映像生成: 動画→注視点指定→LM推定→注視動画出力"""

import ctypes
import math
import os
import sys

import cv2
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
LIB_PATH = os.path.join(PROJECT_ROOT, "build", "libomnigaze.so")
INPUT_VIDEO = os.path.join(PROJECT_ROOT, "Videos", "input1.mp4")
OUTPUT_VIDEO = os.path.join(PROJECT_ROOT, "Videos", "output1.mp4")
FRAMES_LINED_DIR = os.path.join(PROJECT_ROOT, "Videos", "frames_lined")


def image_to_world(u, v, W, H):
    """coord_transform.c と同じ式で画像座標→世界座標"""
    theta = (u - W / 2.0) * (2.0 * math.pi) / W
    phi = -((v - H) * math.pi / H)
    sin_phi = math.sin(phi)
    return (
        sin_phi * math.sin(theta),
        math.cos(phi),
        sin_phi * math.cos(theta),
    )


def get_gaze_point(first_frame_bgr):
    """1フレーム目を表示し、マウスクリックで注視点を1点取得"""
    points = []
    display = first_frame_bgr.copy()

    def on_mouse(event, x, y, _flags, _param):
        if event == cv2.EVENT_LBUTTONDOWN:
            points.clear()
            points.append((x, y))
            print(f"注視点: ({x}, {y})")

    win = "select_gaze"   # 日本語名はQtでハンドル生成に失敗するため英数字にする
    cv2.namedWindow(win, cv2.WINDOW_NORMAL)
    cv2.imshow(win, display)
    cv2.waitKey(1)
    cv2.setMouseCallback(win, on_mouse)

    print("1フレーム目を表示しています。注視点を1点クリックしてください。")
    print("  Enter: 確定 / q: 終了")

    while True:
        img = display.copy()
        if points:
            cv2.drawMarker(
                img, points[0], (0, 0, 255),
                cv2.MARKER_CROSS, 20, 2,
            )
        cv2.imshow(win, img)
        key = cv2.waitKey(30) & 0xFF
        if key == ord("q"):
            break
        if key == 13 and points:
            break

    cv2.destroyAllWindows()

    if not points:
        return None

    u, v = points[0]
    W = first_frame_bgr.shape[1]
    H = first_frame_bgr.shape[0]
    return image_to_world(u, v, W, H)


def draw_center_lines(frame_bgr, color=(0, 255, 0), thickness=2):
    """画像中心を通る縦横のラインを端から端まで描画"""
    h, w = frame_bgr.shape[:2]
    cx, cy = w // 2, h // 2
    cv2.line(frame_bgr, (0, cy), (w, cy), color, thickness)  # 横ライン
    cv2.line(frame_bgr, (cx, 0), (cx, h), color, thickness)  # 縦ライン


def load_lib():
    if not os.path.isfile(LIB_PATH):
        print(f"エラー: {LIB_PATH} が見つかりません。make libomnigaze.so を実行してください。")
        sys.exit(1)

    lib = ctypes.CDLL(LIB_PATH)

    lib.lm_estimate_frame.restype = ctypes.c_int
    lib.lm_estimate_frame.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte),
        ctypes.POINTER(ctypes.c_ubyte),
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_double,
        ctypes.c_double,
        ctypes.c_double,
        ctypes.POINTER(ctypes.c_double),
    ]

    lib.generate_gaze_frame.restype = None
    lib.generate_gaze_frame.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte),
        ctypes.POINTER(ctypes.c_double),
        ctypes.c_int,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_ubyte),
    ]

    lib.generate_gaze_at_gaze_point.restype = None
    lib.generate_gaze_at_gaze_point.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte),
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_double,
        ctypes.c_double,
        ctypes.c_double,
        ctypes.POINTER(ctypes.c_ubyte),
    ]

    return lib


def bgr_to_rgb_contiguous(frame_bgr):
    rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
    return np.ascontiguousarray(rgb, dtype=np.uint8)


def main():
    os.chdir(PROJECT_ROOT)

    if not os.path.isfile(INPUT_VIDEO):
        print(f"エラー: 入力動画が見つかりません: {INPUT_VIDEO}")
        sys.exit(1)

    lib = load_lib()

    cap = cv2.VideoCapture(INPUT_VIDEO)
    if not cap.isOpened():
        print(f"エラー: 動画を開けません: {INPUT_VIDEO}")
        sys.exit(1)

    fps = cap.get(cv2.CAP_PROP_FPS)
    W = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    H = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    ret, first_frame = cap.read()
    if not ret:
        print("エラー: フレームを読み込めません")
        sys.exit(1)

    gaze = get_gaze_point(first_frame)
    if gaze is None:
        print("キャンセルされました")
        sys.exit(0)

    gaze_x, gaze_y, gaze_z = gaze
    print(f"注視点 (世界座標): ({gaze_x:.6f}, {gaze_y:.6f}, {gaze_z:.6f})")

    frames = [first_frame]
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        frames.append(frame)
    cap.release()

    print(f"フレーム数: {len(frames)}, 解像度: {W}x{H}, fps: {fps:.2f}")

    R_out = np.zeros(9, dtype=np.float64)
    out_buf = np.zeros((H, W, 3), dtype=np.uint8)
    output_frames = []
    os.makedirs(FRAMES_LINED_DIR, exist_ok=True)

    for i, frame_bgr in enumerate(frames):
        ref_rgb = bgr_to_rgb_contiguous(frame_bgr)

        if i == 0:
            lib.generate_gaze_at_gaze_point(
                ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                W, H,
                gaze_x, gaze_y, gaze_z,
                out_buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
            )
            n_iter = 0
        else:
            n_iter = lib.lm_estimate_frame(
                base_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                W, H,
                gaze_x, gaze_y, gaze_z,
                R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
            )
            lib.generate_gaze_frame(
                ref_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
                R_out.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                W, H,
                out_buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
            )

        out_bgr = cv2.cvtColor(out_buf, cv2.COLOR_RGB2BGR)
        # ラインは表示・保存用のコピーにだけ引く(計算用 out_buf には引かない)
        lined = out_bgr.copy()
        draw_center_lines(lined)
        output_frames.append(lined)
        # ライン付きフレームを個別保存
        cv2.imwrite(
            os.path.join(FRAMES_LINED_DIR, f"frame_{i:04d}.png"),
            lined,
        )
        base_rgb = np.ascontiguousarray(out_buf.copy(), dtype=np.uint8)

        print(f"フレーム {i + 1}/{len(frames)}: LM反復 {n_iter}回")

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(OUTPUT_VIDEO, fourcc, fps, (W, H))
    for frame in output_frames:
        writer.write(frame)
    writer.release()

    print(f"出力完了: {OUTPUT_VIDEO}")


if __name__ == "__main__":
    main()
