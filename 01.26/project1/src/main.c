/* main.c
 * 全方位画像からの注視画像生成
 * 
 * 使い方:
 *   ./main input.jpg output.jpg u_g v_g u_s v_s
 * 
 * 例:
 *   ./main input.jpg output.jpg 1000 500 1100 500
 */

#include "coord_transform.h"
#include "rotation.h"
#include "vector_math.h"
#include "image_utils.h"
#include "stb_image.h"
#include "stb_image_write.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


/* 注視画像を生成する関数 */
Image* generate_gaze_image(Image *input, int u_g, int v_g, int u_s, int v_s) {
    printf("\n===== 注視画像生成開始 =====\n\n");
    
    int W = input->width;
    int H = input->height;
    
    printf("【ステップ1】注視点と補助点の設定\n");
    printf("  注視点: (%d, %d)\n", u_g, v_g);
    printf("  補助点: (%d, %d)\n", u_s, v_s);
    
    /* 画像座標を世界座標に変換 */
    printf("\n【ステップ2】世界座標への変換\n");
    Vector3D G = image_to_world(u_g, v_g, W, H);
    Vector3D Gs = image_to_world(u_s, v_s, W, H);
    
    vector_print("  注視点G", G);
    vector_print("  補助点Gs", Gs);
    
    /* 回転行列を計算 */
    printf("\n【ステップ3】回転行列の計算\n");
    Matrix3x3 R = compute_rotation_matrix(G, Gs);
    matrix_print("  回転行列R", R);
    
    /* 回転行列の転置（逆回転用） */
    // Matrix3x3 R_T = matrix_transpose(R);
    
    /* 出力画像を作成 */
    printf("\n【ステップ4】注視画像の生成\n");
    Image *output = image_create_like(input);
    if (!output) {
        fprintf(stderr, "エラー: 出力画像の作成失敗\n");
        return NULL;
    }
    
    printf("  処理中");
    fflush(stdout);
    
    int progress_step = H / 10;  /* 進捗表示用 */
    
    /* 出力画像の全画素について処理 */
    for (int v_out = 0; v_out < H; v_out++) {
        /* 進捗表示 */
        if (v_out % progress_step == 0) {
            printf(".");
            fflush(stdout);
        }
        
        for (int u_out = 0; u_out < W; u_out++) {
            /* 1. 出力画素を世界座標に変換 */
            Vector3D X_prime = image_to_world(u_out, v_out, W, H);

            /* 2. 回転: X = R × X' */
            Vector3D X = matrix_vector_multiply(R, X_prime);
            
            /* 3. 世界座標を画像座標に変換 */
            double u_in, v_in;
            world_to_image(X, W, H, &u_in, &v_in);
            
            /* 4. バイリニア補間で画素値を取得 */
            uint8_t rgb[3];
            get_pixel_bilinear(input, u_in, v_in, rgb);
            
            /* 5. 出力画像に設定 */
            set_pixel(output, u_out, v_out, rgb);
        }
    }
    
    printf(" 完了！\n");
    printf("\n===== 注視画像生成完了 =====\n");
    
    return output;
}


int main(int argc, char *argv[]) {
    printf("===== 全方位画像からの注視画像生成 =====\n\n");
    
    /* コマンドライン引数のチェック */
    if (argc != 7) {
        fprintf(stderr, "使い方: %s <入力画像> <出力画像> <u_g> <v_g> <u_s> <v_s>\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "引数:\n");
        fprintf(stderr, "  入力画像: 全方位画像のファイル名（例: input.jpg）\n");
        fprintf(stderr, "  出力画像: 注視画像のファイル名（例: output.jpg）\n");
        fprintf(stderr, "  u_g, v_g: 注視点の画像座標\n");
        fprintf(stderr, "  u_s, v_s: 補助点の画像座標（注視点の右側）\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "例:\n");
        fprintf(stderr, "  %s input.jpg output.jpg 1000 500 1100 500\n", argv[0]);
        return 1;
    }
    
    /* 引数の読み込み */
    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    int u_g = atoi(argv[3]);
    int v_g = atoi(argv[4]);
    int u_s = atoi(argv[5]);
    int v_s = atoi(argv[6]);
    
    printf("入力ファイル: %s\n", input_filename);
    printf("出力ファイル: %s\n", output_filename);
    printf("\n");
    
    /* 画像の読み込み */
    printf("【画像読み込み】\n");
    Image *input = image_load(input_filename);
    if (!input) {
        fprintf(stderr, "エラー: 入力画像の読み込みに失敗しました\n");
        return 1;
    }
    
    /* 座標の妥当性チェック */
    if (u_g < 0 || u_g >= input->width || v_g < 0 || v_g >= input->height) {
        fprintf(stderr, "エラー: 注視点が画像範囲外です\n");
        fprintf(stderr, "  画像サイズ: %d × %d\n", input->width, input->height);
        fprintf(stderr, "  注視点: (%d, %d)\n", u_g, v_g);
        image_free(input);
        return 1;
    }
    
    if (u_s < 0 || u_s >= input->width || v_s < 0 || v_s >= input->height) {
        fprintf(stderr, "エラー: 補助点が画像範囲外です\n");
        fprintf(stderr, "  画像サイズ: %d × %d\n", input->width, input->height);
        fprintf(stderr, "  補助点: (%d, %d)\n", u_s, v_s);
        image_free(input);
        return 1;
    }
    
    /* 注視画像を生成 */
    Image *output = generate_gaze_image(input, u_g, v_g, u_s, v_s);
    
    if (!output) {
        fprintf(stderr, "エラー: 注視画像の生成に失敗しました\n");
        image_free(input);
        return 1;
    }
    
    /* 結果を保存 */
    printf("\n【画像保存】\n");
    if (!image_save_jpg(output_filename, output, 95)) {
        fprintf(stderr, "エラー: 出力画像の保存に失敗しました\n");
        image_free(input);
        image_free(output);
        return 1;
    }
    
    /* メモリ解放 */
    image_free(input);
    image_free(output);
    
    printf("\n===== 処理完了 =====\n");
    printf("結果を確認してください: %s\n", output_filename);
    
    return 0;
}