import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

# 日本語フォント設定
matplotlib.rcParams['font.family'] = 'IPAexGothic'

df = pd.read_csv('results/scan_2d.csv')

# ピボットテーブルに変換（行=omega2、列=omega1）
pivot = df.pivot(index='omega2_deg', columns='omega1_deg', values='E')
logE = np.log10(pivot.values)

fig, ax = plt.subplots(figsize=(8, 6))
im = ax.pcolormesh(
    pivot.columns,  # omega1
    pivot.index,    # omega2
    logE,
    cmap='coolwarm'
)
plt.colorbar(im, ax=ax, label='log10(E)')

# 真値に赤×印
ax.scatter([15.0], [30.0], color='red', marker='x', s=200, linewidth=3, label='真値 (omega1=15°, omega2=30°)')

ax.set_xlabel('omega1 [deg]')
ax.set_ylabel('omega2 [deg]')
ax.set_title('目的関数 E ヒートマップ（omega3=0°固定）')
ax.legend()
plt.tight_layout()
plt.savefig('results/scan_2d_heatmap.png', dpi=150, bbox_inches='tight')
print('保存完了: results/scan_2d_heatmap.png')
