"""バレットタイム映像生成の共通部品"""

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
FRAMES_RAW_DIR = os.path.join(PROJECT_ROOT, "Videos", "frames_raw")
FRAMES_GAZE_DIR = os.path.join(PROJECT_ROOT, "Videos", "frames_gaze")
FRAMES_LINED_DIR = os.path.join(PROJECT_ROOT, "Videos", "frames_lined")
GAZE_TXT = os.path.join(PROJECT_ROOT, "Videos", "gaze.txt")


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


def world_to_image(x, y, z, W, H):
    """coord_transform.c と同じ式で世界座標→画像座標"""
    theta = math.atan2(x, z)
    phi = math.acos(max(-1.0, min(1.0, y)))
    u = (theta + math.pi) * W / (2.0 * math.pi)
    v = -(phi - math.pi) * H / math.pi
    return u, v


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
        ctypes.POINTER(ctypes.c_double),
        ctypes.POINTER(ctypes.c_double),
    ]

    lib.get_gaze_rotation.restype = None
    lib.get_gaze_rotation.argtypes = [
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
