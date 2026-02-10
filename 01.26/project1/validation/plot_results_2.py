#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Y軸回転検証実験の結果可視化スクリプト

菅谷先生の資料に準拠：
- 横軸：回転角度（度）
- 縦軸：目的関数の値 / 1階微分の値
- 目的関数が5°で最小になることを確認
- 数値微分と解析的微分が似た形状を示すことを確認
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import matplotlib.font_manager as fm
import csv
import os

# 日本語フォント設定（plot_results.pyと同じ方式）
# 利用可能な日本語フォントを検索
japanese_fonts = [f.name for f in fm.fontManager.ttflist 
                  if 'Gothic' in f.name or 'Mincho' in f.name or 'CJK' in f.name]

if japanese_fonts:
    # 日本語フォントが見つかった場合、最初のものを使用
    matplotlib.rcParams['font.family'] = 'IPAGothic'
    print(f"✓ 日本語フォント '{japanese_fonts[0]}' を使用します")
else:
    # 見つからない場合はDejaVu Sans（英語）
    matplotlib.rcParams['font.family'] = 'DejaVu Sans'
    print("⚠ 日本語フォントが見つかりません。英語で表示します。")

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42
matplotlib.rcParams['axes.unicode_minus'] = False  # マイナス記号の文字化け対策

def load_csv_data(filepath):
    """
    CSVファイルからデータを読み込む
    
    Args:
        filepath: CSVファイルのパス
        
    Returns:
        numpy配列（2次元）
    """
    data = []
    with open(filepath, 'r') as f:
        reader = csv.reader(f)
        for i, row in enumerate(reader):
            # ヘッダー行をスキップ
            if i == 0:
                # 1行目が数値でない場合はヘッダーとしてスキップ
                try:
                    float(row[0])
                except ValueError:
                    continue  # ← ヘッダー行をスキップ
            data.append([float(x) for x in row])
    return np.array(data)


def plot_objective_function(angles, values, output_path):
    """
    目的関数のグラフを作成
    
    Args:
        angles: 回転角度の配列（度）
        values: 目的関数の値の配列
        output_path: 出力ファイルパス
    """
    plt.figure(figsize=(12, 6))
    
    # 目的関数のプロット
    plt.plot(angles, values, 'b-', linewidth=2, label='目的関数 $E(\psi)$')
    
    # 最小値を探す
    min_idx = np.argmin(values)
    min_angle = angles[min_idx]
    min_value = values[min_idx]
    
    # 最小値の位置をマーク
    plt.plot(min_angle, min_value, 'ro', markersize=10, 
             label=f'最小値 {min_angle:.1f}°')
    
    # 期待される最小値の位置（5°）を破線で表示
    plt.axvline(x=5.0, color='r', linestyle='--', linewidth=1.5, 
                label='期待される最小値（5°）')
    
    # グラフの装飾
    plt.xlabel('視線方向の回転角度（度）', fontsize=14)
    plt.ylabel('目的関数の値', fontsize=14)
    plt.title('回転角度と目的関数の関係', fontsize=16, fontweight='bold')
    plt.legend(fontsize=12, loc='upper right')
    plt.grid(True, alpha=0.3)
    
    # 軸の範囲を設定
    plt.xlim(angles[0], angles[-1])
    
    # レイアウト調整
    plt.tight_layout()
    
    # 保存
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"目的関数のグラフを保存: {output_path}")
    
    plt.close()


def plot_derivatives(angles, numerical, analytical, output_path):
    """
    1階微分のグラフを作成（数値微分と解析的微分を重ねて表示）
    
    Args:
        angles: 回転角度の配列（度）
        numerical: 数値微分の配列
        analytical: 解析的微分の配列
        output_path: 出力ファイルパス
    """
    plt.figure(figsize=(12, 6))
    
    # 数値微分のプロット（青実線）
    plt.plot(angles, numerical, 'b-', linewidth=2, 
             label='数値微分（目的関数の値から計算した1階微分）')
    
    # 解析的微分のプロット（赤破線）
    plt.plot(angles, analytical, 'r--', linewidth=2, 
             label='解析的微分（提案手法の式を用いて計算した1階微分）')
    
    # ゼロ交差の位置（5°）を縦線で表示
    plt.axvline(x=5.0, color='g', linestyle='--', linewidth=1.5, 
                label='期待されるゼロ交差（5°）', alpha=0.7)
    
    # ゼロ線
    plt.axhline(y=0, color='k', linestyle='-', linewidth=0.5, alpha=0.5)
    
    # グラフの装飾
    plt.xlabel('視線方向の回転角度（度）', fontsize=14)
    plt.ylabel('1階微分の値', fontsize=14)
    plt.title('回転角度と1階微分の関係', fontsize=16, fontweight='bold')
    plt.legend(fontsize=12, loc='upper right')
    plt.grid(True, alpha=0.3)
    
    # 軸の範囲を設定
    plt.xlim(angles[0], angles[-1])
    
    # レイアウト調整
    plt.tight_layout()
    
    # 保存
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"微分のグラフを保存: {output_path}")
    
    plt.close()


def print_statistics(angles, obj_values, num_deriv, ana_deriv):
    """
    統計情報を表示
    
    Args:
        angles: 回転角度の配列
        obj_values: 目的関数の値の配列
        num_deriv: 数値微分の配列
        ana_deriv: 解析的微分の配列
    """
    print("\n" + "="*60)
    print("【統計情報】")
    print("="*60)
    
    # 目的関数の最小値
    min_idx = np.argmin(obj_values)
    min_angle = angles[min_idx]
    min_value = obj_values[min_idx]
    
    print(f"\n■ 目的関数")
    print(f"  最小値の位置: {min_angle:.2f}° （期待値: 5.00°）")
    print(f"  最小値: {min_value:.6f}")
    print(f"  誤差: {abs(min_angle - 5.0):.2f}°")
    
    # 5°付近での微分値
    idx_5deg = np.argmin(np.abs(angles - 5.0))
    
    print(f"\n■ 1階微分（5°付近）")
    print(f"  数値微分: {num_deriv[idx_5deg]:.6f}")
    print(f"  解析的微分: {ana_deriv[idx_5deg]:.6f}")
    print(f"  差: {abs(num_deriv[idx_5deg] - ana_deriv[idx_5deg]):.6f}")
    
    # 微分の相関係数
    correlation = np.corrcoef(num_deriv, ana_deriv)[0, 1]
    print(f"\n■ 微分の一致度")
    print(f"  相関係数: {correlation:.6f}")
    
    # RMSEを計算
    rmse = np.sqrt(np.mean((num_deriv - ana_deriv)**2))
    print(f"  RMSE: {rmse:.6f}")
    
    print("\n" + "="*60)
    
    # 検証結果の判定
    print("\n【検証結果】")
    print("="*60)
    
    success = True
    
    if abs(min_angle - 5.0) < 0.2:
        print("✓ 目的関数の最小値が5°付近にある")
    else:
        print("✗ 目的関数の最小値が5°からずれている")
        success = False
    
    if correlation > 0.95:
        print("✓ 数値微分と解析的微分が高い相関を示す")
    else:
        print("✗ 数値微分と解析的微分の相関が低い")
        success = False
    
    if abs(num_deriv[idx_5deg]) < 1000:  # 閾値は調整可能
        print("✓ 5°付近で微分がゼロに近い")
    else:
        print("✗ 5°付近で微分がゼロから離れている")
        success = False
    
    print("="*60)
    
    if success:
        print("\n🎉 すべての検証項目をクリア！実装が正しいことが確認されました。")
    else:
        print("\n⚠️  一部の検証項目で問題が見つかりました。")
    
    print()


def main():
    """
    メイン処理
    """
    print("\n" + "="*60)
    print("Y軸回転検証実験 - 結果可視化")
    print("="*60 + "\n")
    
    # ファイルパスの設定
    obj_csv = 'results/objective_function.csv'
    deriv_csv = 'results/derivatives.csv'
    
    output_dir = 'results/graphs_2'
    obj_png = os.path.join(output_dir, 'objective_function.png')
    deriv_png = os.path.join(output_dir, 'derivatives.png')
    
    # 出力ディレクトリの作成
    os.makedirs(output_dir, exist_ok=True)
    
    # データの読み込み
    print("データを読み込み中...")
    
    if not os.path.exists(obj_csv):
        print(f"エラー: {obj_csv} が見つかりません")
        return
    
    if not os.path.exists(deriv_csv):
        print(f"エラー: {deriv_csv} が見つかりません")
        return
    
    # 目的関数のデータ
    obj_data = load_csv_data(obj_csv)
    angles_obj = obj_data[:, 0]
    values_obj = obj_data[:, 1]
    
    # 微分のデータ
    deriv_data = load_csv_data(deriv_csv)
    angles_deriv = deriv_data[:, 0]
    analytical_deriv = deriv_data[:, 1]  # 解析的微分（提案手法）
    numerical_deriv = deriv_data[:, 2]   # 数値微分（差分近似）
    
    print(f"  目的関数: {len(angles_obj)}点")
    print(f"  微分: {len(angles_deriv)}点")
    
    # グラフの作成
    print("\nグラフを作成中...")
    
    # 1. 目的関数のグラフ
    plot_objective_function(angles_obj, values_obj, obj_png)
    
    # 2. 微分のグラフ
    plot_derivatives(angles_deriv, numerical_deriv, analytical_deriv, deriv_png)
    
    # 統計情報の表示
    print_statistics(angles_obj, values_obj, numerical_deriv, analytical_deriv)
    
    print("\n✓ 処理完了\n")


if __name__ == '__main__':
    main()