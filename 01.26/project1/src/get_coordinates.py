# 座標取得プログラム（入力・出力フォルダ完全分離版）

import os
import cv2

def run(image_path):
    def mouse_event(event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONUP:
            points.append([x, y])
            print(f"点{len(points)}: ({x}, {y})")
            cv2.circle(img, (x, y), 10, (0, 0, 255), -1)
            if len(points) == 2:
                cv2.line(img, tuple(points[0]), tuple(points[1]), (0, 255, 0), 2)

    img = cv2.imread(image_path)
    if img is None:
        print(f"エラー: 画像を読み込めませんでした: {image_path}")
        return []
    
    points = []

    cv2.namedWindow("window", cv2.WINDOW_KEEPRATIO)
    cv2.resizeWindow("window", img.shape[1], img.shape[0])
    cv2.setMouseCallback("window", mouse_event)

    print("\n操作方法:")
    print("  - 左クリック: 点を指定（1点目=注視点、2点目=補助点）")
    print("  - 'z'キー: キャンセル")
    
    while len(points) < 2:
        cv2.imshow("window", img)
        if cv2.waitKey(1) & 0xFF == ord("z"):
            break

    cv2.destroyAllWindows()
    return points


if __name__ == "__main__":
    # プロジェクトのルートディレクトリ
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    # 入力・出力フォルダ
    input_dir = os.path.join(project_root, "images", "input")
    output_dir = os.path.join(project_root, "images", "base")
    
    # フォルダ確認
    if not os.path.exists(input_dir):
        print(f"エラー: 入力フォルダが存在しません: {input_dir}")
        exit(1)
    
    # 出力フォルダが無ければ作成
    os.makedirs(output_dir, exist_ok=True)
    
    # 入力画像一覧を取得
    image_files = [f for f in os.listdir(input_dir) 
                   if f.endswith(('.jpg', '.jpeg', '.png', '.JPG', '.JPEG', '.PNG'))]
    
    if not image_files:
        print(f"エラー: {input_dir} に画像がありません")
        exit(1)
    
    # 画像を選択
    print("=" * 60)
    print("入力画像を選択してください")
    print("=" * 60)
    for i, image_file in enumerate(sorted(image_files)):
        print(f"  {i+1}. {image_file}")
    
    try:
        selected_index = int(input("\n画像番号を入力: ")) - 1
        if selected_index < 0 or selected_index >= len(image_files):
            print("無効な番号です")
            exit(1)
    except ValueError:
        print("数値を入力してください")
        exit(1)
    
    selected_image_name = sorted(image_files)[selected_index]
    selected_image_path = os.path.join(input_dir, selected_image_name)
    
    # 出力画像名を設定
    print(f"\n入力画像: {selected_image_name}")
    output_image_name = input(f"出力画像名 (デフォルト: output.jpg): ").strip()
    if not output_image_name:
        output_image_name = "output.jpg"
    
    # 拡張子確認
    if not output_image_name.endswith(('.jpg', '.jpeg', '.png')):
        output_image_name += ".jpg"
    
    output_image_path = os.path.join(output_dir, output_image_name)
    
    # 座標を取得
    print("\n" + "=" * 60)
    print("画像上で2点をクリックしてください")
    print("  1点目: 注視点（画像中心に配置したい点）")
    print("  2点目: 補助点（画像の上下方向を決める点）")
    print("=" * 60)
    
    points = run(selected_image_path)
    
    if len(points) == 2:
        u_g, v_g = points[0]
        u_s, v_s = points[1]
        
        print("\n" + "=" * 60)
        print("取得した座標:")
        print(f"  注視点: ({u_g}, {v_g})")
        print(f"  補助点: ({u_s}, {v_s})")
        print("=" * 60)
        
        # 相対パス
        input_rel = os.path.relpath(selected_image_path, project_root)
        output_rel = os.path.relpath(output_image_path, project_root)
        
        print("\n実行コマンド:")
        print(f"./build/main {input_rel} {output_rel} {u_g} {v_g} {u_s} {v_s}")
        
        print(f"\n出力先: {output_rel}")
    else:
        print("\nキャンセルされました")