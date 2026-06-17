"""
plot_gradient_1d_slice.py
勾配の1次元断面グラフを作成する。

使用データ:
  results/grad_1d_omega1.csv  (omega2=30°固定、omega1スキャン)
  results/grad_1d_omega2.csv  (omega1=15°固定、omega2スキャン)
"""

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

# 日本語フォント設定
fonts = fm.findSystemFonts()
jp_font = next((f for f in fonts if any(
    name in f for name in ['IPAexGothic', 'IPAGothic', 'NotoSansCJK', 'TakaoGothic']
)), None)
if jp_font:
    fm.fontManager.addfont(jp_font)
    plt.rcParams['font.family'] = fm.FontProperties(fname=jp_font).get_name()

# ── データ読み込み ──────────────────────────────────────────────────
df1 = pd.read_csv('results/grad_1d_omega1.csv')
df2 = pd.read_csv('results/grad_1d_omega2.csv')

# ── プロット ────────────────────────────────────────────────────────
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5))

TRUE_W1 = 15.0
TRUE_W2 = 30.0

# 左図: omega2=30°固定、omega1スキャン
ax1.plot(df1['omega1_deg'], np.log10(df1['grad_norm'] + 1e-10),
         color='steelblue', linewidth=1.5, label='log10(|∇E|)')
ax1.axvline(x=TRUE_W1, color='red', linestyle='--', linewidth=1.5,
            label=f'真値 omega1 = {TRUE_W1:.0f}°')
ax1.set_xlabel('omega1 [deg]', fontsize=12)
ax1.set_ylabel('log10(|∇E|)', fontsize=12)
ax1.set_title('omega2 = 30° 固定、omega1 方向の断面', fontsize=13)
ax1.legend(fontsize=11)
ax1.grid(True, alpha=0.3)

# 右図: omega1=15°固定、omega2スキャン
ax2.plot(df2['omega2_deg'], np.log10(df2['grad_norm'] + 1e-10),
         color='darkorange', linewidth=1.5, label='log10(|∇E|)')
ax2.axvline(x=TRUE_W2, color='red', linestyle='--', linewidth=1.5,
            label=f'真値 omega2 = {TRUE_W2:.0f}°')
ax2.set_xlabel('omega2 [deg]', fontsize=12)
ax2.set_ylabel('log10(|∇E|)', fontsize=12)
ax2.set_title('omega1 = 15° 固定、omega2 方向の断面', fontsize=13)
ax2.legend(fontsize=11)
ax2.grid(True, alpha=0.3)

plt.suptitle('勾配の1次元断面（|∇E| のスライス）', fontsize=15, y=1.02)
plt.tight_layout()
plt.savefig('results/gradient_1d_slice.png', dpi=150, bbox_inches='tight')
print('保存完了: results/gradient_1d_slice.png')
