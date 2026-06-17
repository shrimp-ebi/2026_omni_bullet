/* create_reference_3param.c
 * 3パラメータ（ω₁, ω₂, ω₃）で指定した回転行列を使って参照画像を生成する
 *
 * create_reference.c（rotate_image_y_axis）と同じ向き：
 *   Rinv = create_y_rotation_matrix(-psi) == rodrigues(omega)
 * をそのまま逆写像に使う。
 *
 * 使い方:
 *   ./build/create_reference_3param <入力画像> <出力画像> <ω₁deg> <ω₂deg> <ω₃deg>
 */

#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/vector_math.h"
#include "../experiment/objective_3param.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(int argc, char *argv[])
{
    printf("===== 3パラメータ参照画像生成プログラム =====\n\n");

    if (argc != 6) {
        fprintf(stderr,
            "使い方: %s <入力画像> <出力画像> <ω₁deg> <ω₂deg> <ω₃deg>\n",
            argv[0]);
        return 1;
    }

    const char *input_path  = argv[1];
    const char *output_path = argv[2];
    double omega1_deg = atof(argv[3]);
    double omega2_deg = atof(argv[4]);
    double omega3_deg = atof(argv[5]);

    /* 度 → ラジアン */
    double omega[3];
    omega[0] = omega1_deg * M_PI / 180.0;
    omega[1] = omega2_deg * M_PI / 180.0;
    omega[2] = omega3_deg * M_PI / 180.0;

    printf("回転パラメータ:\n");
    printf("  ω₁ = %7.3f°\n", omega1_deg);
    printf("  ω₂ = %7.3f°\n", omega2_deg);
    printf("  ω₃ = %7.3f°\n\n", omega3_deg);

    /* create_reference.c と同じ向き:
     * rodrigues(omega) をそのまま逆写像行列として使う
     * (create_y_rotation_matrix(-psi) == rodrigues({0,psi,0}) と等価) */
    Matrix3x3 RT = rodrigues(omega);

    printf("逆写像行列 rodrigues(omega):\n");
    printf("  [ %+.6f  %+.6f  %+.6f ]\n", RT.m[0][0], RT.m[0][1], RT.m[0][2]);
    printf("  [ %+.6f  %+.6f  %+.6f ]\n", RT.m[1][0], RT.m[1][1], RT.m[1][2]);
    printf("  [ %+.6f  %+.6f  %+.6f ]\n\n", RT.m[2][0], RT.m[2][1], RT.m[2][2]);

    /* 入力画像の読み込み */
    printf("【画像読み込み】\n");
    Image *input = image_load(input_path);
    if (!input) {
        fprintf(stderr, "エラー: 入力画像を読み込めませんでした: %s\n", input_path);
        return 1;
    }
    printf("  入力: %s (%d x %d)\n\n", input_path, input->width, input->height);

    int W = input->width;
    int H = input->height;

    /* 出力画像の作成 */
    Image *output = image_create_like(input);
    if (!output) {
        fprintf(stderr, "エラー: 出力画像を作成できませんでした\n");
        image_free(input);
        return 1;
    }

    /* 各画素を処理 */
    printf("【画像生成】\n  処理中");
    int dot_interval = H / 20;

    for (int vp = 0; vp < H; vp++) {
        if (dot_interval > 0 && vp % dot_interval == 0) {
            printf(".");
            fflush(stdout);
        }
        for (int up = 0; up < W; up++) {
            /* 出力画素 → 世界座標 X' */
            Vector3D Xp = image_to_world(up, vp, W, H);

            /* 逆写像: X = R^T * X' */
            Vector3D X = matrix_vector_multiply(RT, Xp);

            /* 世界座標 → 入力画像座標 */
            double u_in, v_in;
            world_to_image(X, W, H, &u_in, &v_in);

            /* バイリニア補間で画素値を取得して書き込む */
            uint8_t rgb[3];
            get_pixel_bilinear(input, u_in, v_in, rgb);
            set_pixel(output, up, vp, rgb);
        }
    }
    printf(" 完了\n\n");

    /* 出力画像を保存 */
    printf("【画像保存】\n");
    if (!image_save_jpg(output_path, output, 95)) {
        fprintf(stderr, "エラー: 出力画像を保存できませんでした: %s\n", output_path);
        image_free(input);
        image_free(output);
        return 1;
    }
    printf("  出力: %s\n\n", output_path);

    image_free(input);
    image_free(output);

    printf("===== 処理完了 =====\n");
    printf("出力先: %s\n", output_path);
    return 0;
}
