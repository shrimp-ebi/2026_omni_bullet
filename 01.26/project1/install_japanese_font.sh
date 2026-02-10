#!/bin/bash

# 日本語フォントインストールスクリプト
# Ubuntu/Debian用

echo "============================================"
echo "日本語フォントのインストール"
echo "============================================"
echo ""

# Noto Sans CJK（推奨）
echo "[1/3] Noto Sans CJK フォントをインストール中..."
sudo apt-get update
sudo apt-get install -y fonts-noto-cjk

# IPAフォント（バックアップ）
echo "[2/3] IPAフォントをインストール中..."
sudo apt-get install -y fonts-ipafont-gothic fonts-ipafont-mincho

# matplotlibのフォントキャッシュをクリア
echo "[3/3] matplotlibのフォントキャッシュをクリア中..."
rm -rf ~/.cache/matplotlib

echo ""
echo "✓ インストール完了！"
echo ""
echo "以下のコマンドでインストールされたフォントを確認できます:"
echo "  fc-list :lang=ja"
echo ""
echo "グラフを再生成してください:"
echo "  python3 validation/plot_results.py"
echo ""
