/* create_reference.c
 * 基準画像をY軸周りに5度回転させた参照画像を生成
 * 
 * 使い方:
 *   ./create_reference <基準画像> <参照画像> [回転角度]
 * 
 * 例:
 *   ./create_reference images/base/base.jpg images/reference/reference_5deg.jpg 5.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "../include/y_rotation.h"
#include "../include/image_utils.h"

int main(int argc, char *argv[]) {
    printf("===== 参照画像生成プログラム =====\n\n");
    
    /* コマンドライン引数のチェック */
    if (argc < 3) {
        fprintf(stderr, "使い方: %s <基準画像> <参照画像> [回転角度]\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "引数:\n");
        fprintf(stderr, "  基準画像: 注視点が中心にある画像（例: images/base/base.jpg）\n");
        fprintf(stderr, "  参照画像: 出力ファイル名（例: images/reference/reference_5deg.jpg）\n");
        fprintf(stderr, "  回転角度: Y軸周りの回転角度（度数法、デフォルト: 5.0）\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "例:\n");
        fprintf(stderr, "  %s images/base/base.jpg images/reference/reference_5deg.jpg 5.0\n", argv[0]);
        return 1;
    }
    
    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    double rotation_deg = 5.0;  /* デフォルト値 */
    
    if (argc >= 4) {
        rotation_deg = atof(argv[3]);
    }
    
    printf("入力ファイル: %s\n", input_filename);
    printf("出力ファイル: %s\n", output_filename);
    printf("回転角度: %.2f度\n", rotation_deg);
    printf("\n");
    
    /* 基準画像の読み込み */
    printf("【画像読み込み】\n");
    Image *base_image = image_load(input_filename);
    if (!base_image) {
        fprintf(stderr, "エラー: 基準画像の読み込みに失敗しました\n");
        return 1;
    }
    
    /* Y軸周りに回転 */
    printf("\n【画像回転】\n");
    Image *reference_image = rotate_image_y_axis(base_image, rotation_deg);
    
    if (!reference_image) {
        fprintf(stderr, "エラー: 参照画像の生成に失敗しました\n");
        image_free(base_image);
        return 1;
    }
    
    /* 結果を保存 */
    printf("\n【画像保存】\n");
    if (!image_save_jpg(output_filename, reference_image, 95)) {
        fprintf(stderr, "エラー: 参照画像の保存に失敗しました\n");
        image_free(base_image);
        image_free(reference_image);
        return 1;
    }
    
    /* メモリ解放 */
    image_free(base_image);
    image_free(reference_image);
    
    printf("\n===== 処理完了 =====\n");
    printf("参照画像が生成されました: %s\n", output_filename);
    printf("この画像は基準画像をY軸周りに%.2f度回転させたものです\n", rotation_deg);
    
    return 0;
}