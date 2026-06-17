import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.font_manager as fm

# 日本語フォント設定
fonts = fm.findSystemFonts()
jp_font = next((f for f in fonts if any(
    name in f for name in ['IPAexGothic','IPAGothic','NotoSansCJK','TakaoGothic']
)), None)
if jp_font:
    fm.fontManager.addfont(jp_font)
    plt.rcParams['font.family'] = fm.FontProperties(fname=jp_font).get_name()

df = pd.read_csv('results/scan_2d.csv')
pivot = df.pivot(index='omega2_deg', columns='omega1_deg', values='E')
logE = np.log10(pivot.values)
W1, W2 = np.meshgrid(pivot.columns, pivot.index)

fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection='3d')

surf = ax.plot_surface(W1, W2, logE, cmap='coolwarm',
                        alpha=0.85, linewidth=0, antialiased=True)
fig.colorbar(surf, ax=ax, label='log10(E)', shrink=0.5)

# 真値に赤い点と縦線
true_w1, true_w2 = 15.0, 30.0
true_E = df[(df['omega1_deg']==true_w1) & (df['omega2_deg']==true_w2)]['E'].values[0]
true_logE = np.log10(true_E)
z_floor = np.nanmin(logE)
ax.plot([true_w1, true_w1], [true_w2, true_w2],
        [z_floor, true_logE], color='red', linewidth=2.5)
ax.scatter([true_w1], [true_w2], [true_logE],
           color='red', s=80, zorder=10,
           label=f'真値 (ω1={true_w1}°, ω2={true_w2}°)')

ax.set_xlabel('omega1 [deg]')
ax.set_ylabel('omega2 [deg]')
ax.set_zlabel('log10(E)')
ax.set_title('目的関数 E（omega1-omega2平面、omega3=0°固定）')
ax.legend()
ax.view_init(elev=30, azim=-60)

plt.tight_layout()
plt.show()
