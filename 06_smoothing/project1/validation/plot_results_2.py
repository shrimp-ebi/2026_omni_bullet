#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Y軸回転検証実験の結果可視化スクリプト

- 期待角度は --expected-angle で指定可能
- 未指定時は results/expected_angle.txt または 5.0° を使用
"""

import argparse
import csv
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import matplotlib.font_manager as fm

japanese_fonts = [f.name for f in fm.fontManager.ttflist
                  if 'Gothic' in f.name or 'Mincho' in f.name or 'CJK' in f.name]

if japanese_fonts:
    matplotlib.rcParams['font.family'] = 'IPAGothic'
    print(f"✓ 日本語フォント '{japanese_fonts[0]}' を使用します")
else:
    matplotlib.rcParams['font.family'] = 'DejaVu Sans'
    print('⚠ 日本語フォントが見つかりません。英語で表示します。')

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42
matplotlib.rcParams['axes.unicode_minus'] = False


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


def load_csv_data(filepath):
    data = []
    with open(filepath, 'r', encoding='utf-8') as f:
        reader = csv.reader(f)
        for i, row in enumerate(reader):
            if i == 0:
                try:
                    float(row[0])
                except ValueError:
                    continue
            data.append([float(x) for x in row])
    return np.array(data)


def plot_objective_function(angles, values, output_path, expected_angle):
    plt.figure(figsize=(12, 6))
    plt.plot(angles, values, 'b-', linewidth=2, label='目的関数 $E(\psi)$')

    min_idx = np.argmin(values)
    min_angle = angles[min_idx]
    min_value = values[min_idx]
    plt.plot(min_angle, min_value, 'ro', markersize=10,
             label=f'最小値 {min_angle:.1f}°')

    plt.axvline(x=expected_angle, color='r', linestyle='--', linewidth=1.5,
                label=f'期待される最小値（{expected_angle:.2f}°）')

    plt.xlabel('視線方向の回転角度（度）', fontsize=14)
    plt.ylabel('目的関数の値', fontsize=14)
    plt.title('回転角度と目的関数の関係', fontsize=16, fontweight='bold')
    plt.legend(fontsize=12, loc='upper right')
    plt.grid(True, alpha=0.3)
    x_min, x_max = compute_centered_xlim(angles, expected_angle)
    plt.xlim(x_min, x_max)
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f'目的関数のグラフを保存: {output_path}')
    plt.close()


def plot_derivatives(angles, numerical, analytical, output_path, expected_angle):
    plt.figure(figsize=(12, 6))
    plt.plot(angles, numerical, 'b-', linewidth=2,
             label='数値微分（目的関数の値から計算した1階微分）')
    plt.plot(angles, analytical, 'r--', linewidth=2,
             label='解析的微分（提案手法の式を用いて計算した1階微分）')

    plt.axvline(x=expected_angle, color='g', linestyle='--', linewidth=1.5,
                label=f'期待されるゼロ交差（{expected_angle:.2f}°）', alpha=0.7)
    plt.axhline(y=0, color='k', linestyle='-', linewidth=0.5, alpha=0.5)

    plt.xlabel('視線方向の回転角度（度）', fontsize=14)
    plt.ylabel('1階微分の値', fontsize=14)
    plt.title('回転角度と1階微分の関係', fontsize=16, fontweight='bold')
    plt.legend(fontsize=12, loc='upper right')
    plt.grid(True, alpha=0.3)
    x_min, x_max = compute_centered_xlim(angles, expected_angle)
    plt.xlim(x_min, x_max)
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f'微分のグラフを保存: {output_path}')
    plt.close()


def print_statistics(angles, obj_values, num_deriv, ana_deriv, expected_angle):
    print('\n' + '=' * 60)
    print('【統計情報】')
    print('=' * 60)

    min_idx = np.argmin(obj_values)
    min_angle = angles[min_idx]
    min_value = obj_values[min_idx]

    print('\n■ 目的関数')
    print(f'  最小値の位置: {min_angle:.2f}° （期待値: {expected_angle:.2f}°）')
    print(f'  最小値: {min_value:.6f}')
    print(f'  誤差: {abs(min_angle - expected_angle):.2f}°')

    idx_expected = np.argmin(np.abs(angles - expected_angle))
    print(f'\n■ 1階微分（{expected_angle:.2f}°付近）')
    print(f'  数値微分: {num_deriv[idx_expected]:.6f}')
    print(f'  解析的微分: {ana_deriv[idx_expected]:.6f}')
    print(f'  差: {abs(num_deriv[idx_expected] - ana_deriv[idx_expected]):.6f}')

    correlation = np.corrcoef(num_deriv, ana_deriv)[0, 1]
    print('\n■ 微分の一致度')
    print(f'  相関係数: {correlation:.6f}')


def main():
    parser = argparse.ArgumentParser(description='検証結果の可視化（派生版）')
    parser.add_argument('--expected-angle', type=float, default=None,
                        help='期待される回転角度（度）。未指定時は results/expected_angle.txt または 5.0 を使用')
    args = parser.parse_args()

    expected_angle = resolve_expected_angle(args.expected_angle)

    print('\n' + '=' * 60)
    print('Y軸回転検証実験 - 結果可視化')
    print('=' * 60)
    print(f'期待角度: {expected_angle:.2f}°\n')

    obj_csv = 'results/objective_function.csv'
    deriv_csv = 'results/derivatives.csv'

    output_dir = 'results/graphs_2'
    obj_png = os.path.join(output_dir, 'objective_function.png')
    deriv_png = os.path.join(output_dir, 'derivatives.png')

    os.makedirs(output_dir, exist_ok=True)

    print('データを読み込み中...')
    if not os.path.exists(obj_csv):
        print(f'エラー: {obj_csv} が見つかりません')
        return
    if not os.path.exists(deriv_csv):
        print(f'エラー: {deriv_csv} が見つかりません')
        return

    obj_data = load_csv_data(obj_csv)
    angles_obj = obj_data[:, 0]
    values_obj = obj_data[:, 1]

    deriv_data = load_csv_data(deriv_csv)
    angles_deriv = deriv_data[:, 0]
    analytical_deriv = deriv_data[:, 1]
    numerical_deriv = deriv_data[:, 2]

    print(f'  目的関数: {len(angles_obj)}点')
    print(f'  微分: {len(angles_deriv)}点')

    print('\nグラフを作成中...')
    plot_objective_function(angles_obj, values_obj, obj_png, expected_angle)
    plot_derivatives(angles_deriv, numerical_deriv, analytical_deriv, deriv_png, expected_angle)
    print_statistics(angles_obj, values_obj, numerical_deriv, analytical_deriv, expected_angle)

    print('\n✓ 処理完了\n')


if __name__ == '__main__':
    main()
