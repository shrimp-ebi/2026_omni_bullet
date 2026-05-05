import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
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

# ω₁=15°固定でω₂方向の断面を切り出す
slice_df = df[df['omega1_deg'] == 15.0].sort_values('omega2_deg')
omega2 = slice_df['omega2_deg'].values
E = slice_df['E'].values

# 数値微分（中心差分）
dE = np.gradient(E, omega2)

# 真値
true_omega2 = 30.0

fig, axes = plt.subplots(2, 1, figsize=(10, 8))

# 上段：目的関数E（図6相当）
ax1 = axes[0]
ax1.plot(omega2, E, 'b-', linewidth=1.5, label='目的関数 E(ω₂)')
ax1.axvline(x=true_omega2, color='red', linestyle='--', label=f'真値 ({true_omega2}°)')
min_idx = np.argmin(E)
ax1.scatter([omega2[min_idx]], [E[min_idx]], color='red', s=80, zorder=5,
            label=f'最小値 {omega2[min_idx]:.1f}°')
ax1.set_xlabel('ω₂ [deg]')
ax1.set_ylabel('目的関数の値 E')
ax1.set_title('回転角度と目的関数の関係（ω₁=15°固定）')
ax1.legend()
ax1.grid(True, alpha=0.3)

# 下段：1次微分（図7相当）
ax2 = axes[1]
ax2.plot(omega2, dE, 'b-', linewidth=1.0, label='数値微分（中心差分）')
ax2.axhline(y=0, color='gray', linestyle='-', linewidth=0.8, alpha=0.5)
ax2.axvline(x=true_omega2, color='red', linestyle='--', label=f'期待されるゼロ交差 ({true_omega2}°)')
ax2.set_xlabel('ω₂ [deg]')
ax2.set_ylabel('1階微分の値 dE/dω₂')
ax2.set_title('回転角度と1階微分の関係（ω₁=15°固定）')
ax2.legend()
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('results/slice_1d_omega2.png', dpi=150, bbox_inches='tight')
print('保存完了: results/slice_1d_omega2.png')
