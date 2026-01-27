# 座標取得プログラム（2点版・簡潔）

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
    points = []

    cv2.namedWindow("window", cv2.WINDOW_KEEPRATIO)
    cv2.resizeWindow("window", img.shape[1], img.shape[0])
    cv2.setMouseCallback("window", mouse_event)

    while len(points) < 2:
        cv2.imshow("window", img)
        if cv2.waitKey(1) & 0xFF == ord("z"):
            break

    cv2.destroyAllWindows()
    return points


if __name__ == "__main__":
    image_dir = "/home/y233324/ドキュメント/2026_注視画像生成/Code/2026_omni_bullet/01.26/project1"
    image_files = [f for f in os.listdir(image_dir) if f.endswith(('.jpg', '.jpeg', '.png'))]

    print("利用可能な画像:")
    for i, image_file in enumerate(image_files):
        print(f"{i+1}. {image_file}")

    selected_index = int(input("画像番号: ")) - 1
    selected_image_path = os.path.join(image_dir, image_files[selected_index])

    print("\n2点クリックしてください")
    points = run(selected_image_path)

    if len(points) == 2:
        u_g, v_g = points[0]
        u_s, v_s = points[1]
        print(f"\n./main input.jpg output.jpg {u_g} {v_g} {u_s} {v_s}")
    else:
        print("キャンセルされました")
