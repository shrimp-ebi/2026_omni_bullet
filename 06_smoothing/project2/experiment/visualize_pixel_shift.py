#!/usr/bin/env python3
"""
visualize_pixel_shift.py
「1° のズレが画像上で何画素に相当するか」を目視確認する図を生成する。

画像情報:
  base.jpg  6080 × 3040 px
  1° = 6080 / 360 ≈ 16.89 px（水平方向）
"""

import os
import sys

import cv2
import matplotlib
import matplotlib.font_manager as fm
import matplotlib.pyplot as plt
import numpy as np

matplotlib.use("Agg")

# ── 日本語フォント自動検出 ───────────────────────────────────────────────────
def find_japanese_font():
    candidates = [
        "Noto Sans CJK JP", "Noto Sans JP",
        "IPAGothic", "IPAPGothic", "IPAexGothic",
        "TakaoGothic", "VL Gothic",
    ]
    available = {f.name for f in fm.fontManager.ttflist}
    for name in candidates:
        if name in available:
            return name
    return None

font_name = find_japanese_font()
if font_name:
    matplotlib.rcParams["font.family"] = font_name

# ── 設定 ────────────────────────────────────────────────────────────────────
IMG_PATH   = "images/base/base.jpg"
OUT_PATH   = "results/pixel_shift_1deg.png"
CROP_SIZE  = 200          # 切り出し領域の一辺 [px]
SHIFT_PX   = 17           # ≈ 1°（6080 / 360 ≈ 16.89）
ARROW_COLOR = (255, 0, 0) # 赤（BGR）

# ── 画像読み込み ─────────────────────────────────────────────────────────────
img_bgr = cv2.imread(IMG_PATH)
if img_bgr is None:
    print(f"エラー: 画像を読み込めません: {IMG_PATH}")
    sys.exit(1)

H, W = img_bgr.shape[:2]
shift_deg_approx = W / 360.0   # 実際の px/deg

print(f"画像サイズ: {W} x {H} px")
print(f"1° = {W}/360 = {shift_deg_approx:.2f} px（水平方向）")
print(f"使用シフト量: {SHIFT_PX} px")

# BGR → RGB（matplotlib 用）
img = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2RGB)

# ── 中央から切り出し ──────────────────────────────────────────────────────────
cx = W // 2
cy = H // 2
half = CROP_SIZE // 2

# 元画像の切り出し（中央 200×200）
orig = img[cy - half : cy + half, cx - half : cx + half].copy()

# 1° ≈ 17px 右にずらした切り出し
shifted = img[cy - half : cy + half,
              cx - half + SHIFT_PX : cx + half + SHIFT_PX].copy()

# ── 差分画像 ─────────────────────────────────────────────────────────────────
diff_bgr = cv2.absdiff(
    cv2.cvtColor(orig,    cv2.COLOR_RGB2BGR),
    cv2.cvtColor(shifted, cv2.COLOR_RGB2BGR),
)
# 視認性向上のため明るさを3倍に増幅
diff_bright = np.clip(diff_bgr.astype(np.int32) * 3, 0, 255).astype(np.uint8)
diff_rgb = cv2.cvtColor(diff_bright, cv2.COLOR_BGR2RGB)

# ── 矢印付き画像（ズレの大きさを図示）────────────────────────────────────────
arrow_img = orig.copy()
arrow_bgr = cv2.cvtColor(arrow_img, cv2.COLOR_RGB2BGR)

# 画像中央行に赤い矢印を描画
mid_y = CROP_SIZE // 2
start_x = CROP_SIZE // 2 - SHIFT_PX // 2
end_x   = start_x + SHIFT_PX

cv2.arrowedLine(
    arrow_bgr,
    pt1=(start_x, mid_y),
    pt2=(end_x,   mid_y),
    color=ARROW_COLOR,
    thickness=2,
    tipLength=0.4,
)
# ラベルテキスト
cv2.putText(
    arrow_bgr,
    f"{SHIFT_PX}px",
    (start_x, mid_y - 8),
    cv2.FONT_HERSHEY_SIMPLEX,
    0.5, ARROW_COLOR, 1, cv2.LINE_AA,
)
arrow_rgb = cv2.cvtColor(arrow_bgr, cv2.COLOR_BGR2RGB)

# ── 2×2 グリッド描画 ─────────────────────────────────────────────────────────
fig, axes = plt.subplots(2, 2, figsize=(10, 10))

panels = [
    (axes[0, 0], orig,      "元画像",              False),
    (axes[0, 1], shifted,   f"1°ずらし ≈{SHIFT_PX}px", False),
    (axes[1, 0], diff_rgb,  "差分（×3 増幅）",      True),
    (axes[1, 1], arrow_rgb, "ズレの大きさ",          False),
]

for ax, panel_img, label, is_diff in panels:
    ax.imshow(panel_img, vmin=0, vmax=255)
    ax.set_title(label, fontsize=14, pad=6)
    ax.axis("off")
    if is_diff:
        # 差分画像にカラーバー用の参照を仕込む
        im_ref = ax.imshow(panel_img, vmin=0, vmax=255)

# 全体タイトル
fig.suptitle(
    f"1° shift ≈ {shift_deg_approx:.1f} pixels  "
    f"（画像サイズ {W}×{H}、切り出し {CROP_SIZE}×{CROP_SIZE}）",
    fontsize=14,
    y=0.98,
)

plt.tight_layout(rect=[0, 0, 1, 0.96])

# ── 保存 ─────────────────────────────────────────────────────────────────────
os.makedirs("results", exist_ok=True)
plt.savefig(OUT_PATH, dpi=150, bbox_inches="tight")
print(f"\n保存完了: {OUT_PATH}")
plt.close()
