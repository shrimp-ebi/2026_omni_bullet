#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
plot_results.py (改善版)
Y軸回りの回転検証実験の結果をグラフ化

- 期待角度は --expected-angle で指定可能
- 指定がなければ results/expected_angle.txt を参照
- それもなければ 5.0° を使用
"""

import argparse
import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

try:
    japanese_fonts = [
        f.name for f in fm.fontManager.ttflist
        if 'Gothic' in f.name or 'Mincho' in f.name
    ]
    if japanese_fonts:
        plt.rcParams['font.family'] = japanese_fonts[0]
except Exception:
    pass


def resolve_expected_angle(cli_value):
    if cli_value is not None:
        return cli_value

    metadata_path = 'results/expected_angle.txt'
    if os.path.exists(metadata_path):
        try:
            with open(metadata_path, 'r', encoding='utf-8') as f:
                return float(f.read().strip())
        except ValueError:
            pass

    return 5.0




def compute_centered_xlim(angles, expected_angle, margin=0.5):
    data_min = float(np.min(angles))
    data_max = float(np.max(angles))
    half = max(expected_angle - data_min, data_max - expected_angle) + margin
    x_min = expected_angle - half
    x_max = expected_angle + half
    return x_min, x_max


def plot_objective_function(expected_angle):
    df = pd.read_csv('results/objective_function.csv')

    min_idx = df['objective_function'].idxmin()
    min_angle = df.loc[min_idx, 'angle_deg']
    min_value = df.loc[min_idx, 'objective_function']

    fig, ax = plt.subplots(figsize=(12, 7))
    ax.plot(df['angle_deg'], df['objective_function'], 'b-', linewidth=2.5)
    ax.plot(min_angle, min_value, 'ro', markersize=12,
            label=f'最小値 {min_angle:.1f}°', zorder=5)

    ax.axvline(
        x=expected_angle, color='r', linestyle='--', alpha=0.6, linewidth=2,
        label=f'期待される最小値 ({expected_angle:.2f}°)'
    )

    x_min, x_max = compute_centered_xlim(df['angle_deg'].to_numpy(), expected_angle)
    ax.set_xlim(x_min, x_max)
    ax.set_ylim(0, 2000)
    ax.set_xlabel('視線方向の回転角度(度)', fontsize=16, fontweight='bold')
    ax.set_ylabel('目的関数の値', fontsize=16, fontweight='bold')
    ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    ax.legend(fontsize=13, loc='upper left', framealpha=0.9)
    ax.set_xticks(np.arange(x_min, x_max + 0.001, 0.5))
    ax.set_yticks(np.arange(0, 2001, 200))
    ax.tick_params(axis='both', which='major', labelsize=12)

    for spine in ax.spines.values():
        spine.set_linewidth(1.5)

    os.makedirs('results/graphs', exist_ok=True)
    plt.tight_layout()
    plt.savefig('results/graphs/objective_function.png', dpi=300, bbox_inches='tight')
    print("✓ 目的関数のグラフを保存: results/graphs/objective_function.png")
    plt.close()


def plot_derivatives(expected_angle):
    df = pd.read_csv('results/derivatives.csv')

    fig, ax = plt.subplots(figsize=(12, 7))
    ax.plot(df['angle_deg'], df['numerical_derivative'], 'b-',
            linewidth=2.5, label='目的関数の値から計算した1階微分', alpha=0.8)
    ax.plot(df['angle_deg'], df['analytical_derivative'], 'r--',
            linewidth=2.5, label='提案手法を用いて計算した1階微分')
    ax.axhline(y=0, color='gray', linestyle='-', linewidth=1, alpha=0.5)
    ax.axvline(x=expected_angle, color='g', linestyle='--', alpha=0.6, linewidth=1.5,
               label=f'期待されるゼロ交差 ({expected_angle:.2f}°)')

    x_min, x_max = compute_centered_xlim(df['angle_deg'].to_numpy(), expected_angle)
    ax.set_xlim(x_min, x_max)
    ax.set_ylim(-80000, 80000)
    ax.set_xlabel('視線方向の回転角度(度)', fontsize=16, fontweight='bold')
    ax.set_ylabel('1階微分の値', fontsize=16, fontweight='bold')
    ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    ax.legend(fontsize=12, loc='upper right', framealpha=0.9)
    ax.set_xticks(np.arange(x_min, x_max + 0.001, 0.5))
    ax.set_yticks(np.arange(-80000, 80001, 20000))
    ax.tick_params(axis='both', which='major', labelsize=12)

    for spine in ax.spines.values():
        spine.set_linewidth(1.5)

    plt.tight_layout()
    plt.savefig('results/graphs/derivatives.png', dpi=300, bbox_inches='tight')
    print("✓ 微分のグラフを保存: results/graphs/derivatives.png")
    plt.close()


def print_statistics(expected_angle):
    df_obj = pd.read_csv('results/objective_function.csv')
    min_idx = df_obj['objective_function'].idxmin()
    min_angle = df_obj.loc[min_idx, 'angle_deg']
    min_value = df_obj.loc[min_idx, 'objective_function']

    print("\n===== 統計情報 =====")
    print("目的関数の最小値:")
    print(f"  角度: {min_angle:.2f}° (期待値: {expected_angle:.2f}°)")
    print(f"  誤差: {abs(min_angle - expected_angle):.2f}°")
    print(f"  E(ψ): {min_value:.6f}")

    if abs(min_angle - expected_angle) > 1.0:
        print("\n  ⚠️  警告: 最小値が期待位置から大きくずれています！")
        print(f"       最小値: {min_angle:.1f}°")
        print(f"       期待値: {expected_angle:.1f}°")

    df_der = pd.read_csv('results/derivatives.csv')
    idx_expected = (df_der['angle_deg'] - expected_angle).abs().idxmin()
    analytical_at_expected = df_der.loc[idx_expected, 'analytical_derivative']
    numerical_at_expected = df_der.loc[idx_expected, 'numerical_derivative']

    print(f"\n期待角度({expected_angle:.2f}°)付近での微分値:")
    print(f"  理論微分: {analytical_at_expected:.2f}")
    print(f"  数値微分: {numerical_at_expected:.2f}")
    print(f"  差: {abs(analytical_at_expected - numerical_at_expected):.2f}")

    correlation = df_der['analytical_derivative'].corr(df_der['numerical_derivative'])
    print("\n微分の一致度:")
    print(f"  相関係数: {correlation:.4f}")


def main():
    parser = argparse.ArgumentParser(description='検証結果のグラフ描画')
    parser.add_argument('--expected-angle', type=float, default=None,
                        help='期待される回転角度（度）。未指定時は results/expected_angle.txt または 5.0 を使用')
    args = parser.parse_args()

    expected_angle = resolve_expected_angle(args.expected_angle)

    print("===== グラフ描画プログラム（池内論文フォーマット版）=====\n")
    print(f"期待角度: {expected_angle:.2f}°")

    if not os.path.exists('results/objective_function.csv'):
        print('エラー: results/objective_function.csv が見つかりません')
        return

    if not os.path.exists('results/derivatives.csv'):
        print('エラー: results/derivatives.csv が見つかりません')
        return

    print('グラフを作成中...')
    plot_objective_function(expected_angle)
    plot_derivatives(expected_angle)
    print_statistics(expected_angle)

    print('\n===== 完了 =====')
    print('グラフは results/graphs/ に保存されました')


if __name__ == '__main__':
    main()
