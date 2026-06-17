import pandas as pd
import numpy as np
import matplotlib
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
E = pivot.values
omega1_vals = pivot.columns.values
omega2_vals = pivot.index.values
h1 = omega1_vals[1] - omega1_vals[0]
h2 = omega2_vals[1] - omega2_vals[0]

# 中心差分で勾配を計算
dE_dw1 = np.gradient(E, h1, axis=1)
dE_dw2 = np.gradient(E, h2, axis=0)

# 勾配のノルム（大きさ）
grad_norm = np.sqrt(dE_dw1**2 + dE_dw2**2)
log_grad = np.log10(grad_norm + 1e-10)

W1, W2 = np.meshgrid(omega1_vals, omega2_vals)

# 3Dグラフ
fig = plt.figure(figsize=(12, 5))

# 左：勾配ノルムの3Dグラフ
ax1 = fig.add_subplot(121, projection='3d')
surf = ax1.plot_surface(W1, W2, log_grad,
                         cmap='viridis', alpha=0.85,
                         linewidth=0, antialiased=True)
fig.colorbar(surf, ax=ax1, label='log10(|∇E|)', shrink=0.5)
ax1.set_xlabel('omega1 [deg]')
ax1.set_ylabel('omega2 [deg]')
ax1.set_zlabel('log10(|∇E|)')
ax1.set_title('勾配の大きさ |∇E|')

# 真値の位置
true_w1, true_w2 = 15.0, 30.0
idx1 = np.argmin(np.abs(omega1_vals - true_w1))
idx2 = np.argmin(np.abs(omega2_vals - true_w2))
true_grad = log_grad[idx2, idx1]
ax1.scatter([true_w1], [true_w2], [true_grad],
            color='red', s=80, zorder=10, label='真値')
ax1.legend()

# 右：勾配ノルムのヒートマップ
ax2 = fig.add_subplot(122)
im = ax2.pcolormesh(omega1_vals, omega2_vals, log_grad,
                     cmap='viridis')
fig.colorbar(im, ax=ax2, label='log10(|∇E|)')
ax2.scatter([true_w1], [true_w2], color='red', marker='x',
            s=200, linewidth=3, label='真値')
ax2.set_xlabel('omega1 [deg]')
ax2.set_ylabel('omega2 [deg]')
ax2.set_title('勾配ヒートマップ')
ax2.legend()

plt.suptitle('目的関数Eの1次微分（勾配）', fontsize=14)
plt.tight_layout()
plt.savefig('results/gradient_2d.png', dpi=150, bbox_inches='tight')
print('保存完了: results/gradient_2d.png')
plt.show()
