#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
plot_results.py (改善版)
Y軸回りの回転検証実験の結果をグラフ化
池内さんの論文のフォーマットに合わせる
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import os
import numpy as np

# 日本語フォント設定
plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

# 日本語ラベル用の設定
try:
    # 日本語フォントの検索（利用可能な場合）
    japanese_fonts = [f.name for f in fm.fontManager.ttflist if 'Gothic' in f.name or 'Mincho' in f.name]
    if japanese_fonts:
        plt.rcParams['font.family'] = japanese_fonts[0]
except:
    pass

def plot_objective_function():
    """目的関数 E(ψ) のグラフを作成（池内論文 図4.4に対応）
    
    池内論文の仕様:
    - 横軸: -5から10まで (0.5刻み表示)
    - 縦軸: 0から2000まで (200刻み)
    - 日本語ラベル
    """
    
    # CSVファイルを読み込み
    df = pd.read_csv('results/objective_function.csv')
    
    # 最小値の位置を探す
    min_idx = df['objective_function'].idxmin()
    min_angle = df.loc[min_idx, 'angle_deg']
    min_value = df.loc[min_idx, 'objective_function']
    
    # グラフ作成（池内論文のサイズに合わせる）
    fig, ax = plt.subplots(figsize=(12, 7))
    
    # メインプロット（太めの青線）
    ax.plot(df['angle_deg'], df['objective_function'], 'b-', linewidth=2.5)
    
    # 最小値をプロット（赤丸）
    ax.plot(min_angle, min_value, 'ro', markersize=12, 
             label=f'最小値 {min_angle:.1f}°', zorder=5)
    
    # 期待される最小値の位置（5度）- 赤破線
    ax.axvline(x=5.0, color='r', linestyle='--', alpha=0.6, linewidth=2,
                label='期待される最小値 (5°)')
    
    # 軸の範囲設定（池内論文に厳密に合わせる）
    ax.set_xlim(-5, 10)
    ax.set_ylim(0, 2000)
    
    # 軸ラベル（日本語）
    ax.set_xlabel('視線方向の回転角度(度)', fontsize=16, fontweight='bold')
    ax.set_ylabel('目的関数の値', fontsize=16, fontweight='bold')
    
    # グリッド（薄めに）
    ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    
    # 凡例
    ax.legend(fontsize=13, loc='upper left', framealpha=0.9)
    
    # 軸の目盛り設定（池内論文に合わせる）
    ax.set_xticks(np.arange(-5, 10.5, 0.5))  # -5から10まで0.5刻み
    ax.set_yticks(np.arange(0, 2001, 200))   # 0から2000まで200刻み
    
    # 目盛りラベルのフォントサイズ
    ax.tick_params(axis='both', which='major', labelsize=12)
    
    # 枠線を太くする
    for spine in ax.spines.values():
        spine.set_linewidth(1.5)
    
    # 保存
    os.makedirs('results/graphs', exist_ok=True)
    plt.tight_layout()
    plt.savefig('results/graphs/objective_function.png', dpi=300, bbox_inches='tight')
    print("✓ 目的関数のグラフを保存: results/graphs/objective_function.png")
    
    plt.close()

def plot_derivatives():
    """微分のグラフを作成（池内論文 図4.5に対応）
    
    池内論文の仕様:
    - 横軸: -5から10まで (0.5刻み表示)
    - 縦軸: -80000から80000まで (20000刻み)
    - 日本語ラベル
    - 青線: 数値微分
    - 赤破線: 理論微分
    """
    
    # CSVファイルを読み込み
    df = pd.read_csv('results/derivatives.csv')
    
    # グラフ作成
    fig, ax = plt.subplots(figsize=(12, 7))
    
    # 数値微分（青実線）
    ax.plot(df['angle_deg'], df['numerical_derivative'], 'b-', 
             linewidth=2.5, label='目的関数の値から計算した1階微分', alpha=0.8)
    
    # 理論微分（赤破線）
    ax.plot(df['angle_deg'], df['analytical_derivative'], 'r--', 
             linewidth=2.5, label='提案手法を用いて計算した1階微分')
    
    # ゼロ線（グレー細線）
    ax.axhline(y=0, color='gray', linestyle='-', linewidth=1, alpha=0.5)
    
    # 5度の位置に縦線（緑破線）
    ax.axvline(x=5.0, color='g', linestyle='--', alpha=0.6, linewidth=1.5)
    
    # 軸の範囲設定（池内論文に厳密に合わせる）
    ax.set_xlim(-5, 10)
    ax.set_ylim(-80000, 80000)
    
    # 軸ラベル（日本語）
    ax.set_xlabel('視線方向の回転角度(度)', fontsize=16, fontweight='bold')
    ax.set_ylabel('1階微分の値', fontsize=16, fontweight='bold')
    
    # グリッド（薄めに）
    ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    
    # 凡例
    ax.legend(fontsize=12, loc='upper right', framealpha=0.9)
    
    # 軸の目盛り設定（池内論文に合わせる）
    ax.set_xticks(np.arange(-5, 10.5, 0.5))  # -5から10まで0.5刻み
    ax.set_yticks(np.arange(-80000, 80001, 20000))  # -80000から80000まで20000刻み
    
    # 目盛りラベルのフォントサイズ
    ax.tick_params(axis='both', which='major', labelsize=12)
    
    # 枠線を太くする
    for spine in ax.spines.values():
        spine.set_linewidth(1.5)
    
    # 保存
    plt.tight_layout()
    plt.savefig('results/graphs/derivatives.png', dpi=300, bbox_inches='tight')
    print("✓ 微分のグラフを保存: results/graphs/derivatives.png")
    
    plt.close()

def print_statistics():
    """統計情報を表示"""
    
    # 目的関数
    df_obj = pd.read_csv('results/objective_function.csv')
    min_idx = df_obj['objective_function'].idxmin()
    min_angle = df_obj.loc[min_idx, 'angle_deg']
    min_value = df_obj.loc[min_idx, 'objective_function']
    
    print("\n===== 統計情報 =====")
    print(f"目的関数の最小値:")
    print(f"  角度: {min_angle:.2f}° (期待値: 5.00°)")
    print(f"  誤差: {abs(min_angle - 5.0):.2f}°")
    print(f"  E(ψ): {min_value:.6f}")
    
    # ⚠️ 警告表示
    if abs(min_angle - 5.0) > 1.0:
        print(f"\n  ⚠️  警告: 最小値が期待位置から大きくずれています！")
        print(f"       最小値: {min_angle:.1f}°")
        print(f"       期待値: 5.0°")
        print(f"       → 回転方向の符号が逆の可能性があります")
    
    # 微分
    df_der = pd.read_csv('results/derivatives.csv')
    
    # 5度付近の微分値を取得
    idx_5deg = (df_der['angle_deg'] - 5.0).abs().idxmin()
    analytical_at_5 = df_der.loc[idx_5deg, 'analytical_derivative']
    numerical_at_5 = df_der.loc[idx_5deg, 'numerical_derivative']
    
    print(f"\n5度での微分値:")
    print(f"  理論微分: {analytical_at_5:.2f}")
    print(f"  数値微分: {numerical_at_5:.2f}")
    print(f"  差: {abs(analytical_at_5 - numerical_at_5):.2f}")
    
    # 微分の一致度（全体）
    # 理論微分の値が大きすぎるため、相関係数で評価
    correlation = df_der['analytical_derivative'].corr(df_der['numerical_derivative'])
    
    print(f"\n微分の一致度:")
    print(f"  相関係数: {correlation:.4f}")
    
    if correlation > 0.5:
        print("  ⚠️  理論微分と数値微分の傾向は類似していますが、")
        print("      スケールが大きく異なります（理論微分の値が大きすぎます）")
    else:
        print("  ⚠️  理論微分と数値微分が一致していません")

def main():
    print("===== グラフ描画プログラム（池内論文フォーマット版）=====\n")
    
    # ファイルの存在確認
    if not os.path.exists('results/objective_function.csv'):
        print("エラー: results/objective_function.csv が見つかりません")
        return
    
    if not os.path.exists('results/derivatives.csv'):
        print("エラー: results/derivatives.csv が見つかりません")
        return
    
    # グラフ作成
    print("グラフを作成中...")
    plot_objective_function()
    plot_derivatives()
    
    # 統計情報を表示
    print_statistics()
    
    print("\n===== 完了 =====")
    print("グラフは results/graphs/ に保存されました")

if __name__ == "__main__":
    main()