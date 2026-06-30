/* generate_gaze_frame.c
 * 1フレーム分の注視画像を生成する
 *
 * 使い方:
 *   ./generate_gaze_frame <入力画像> <出力画像> <u_g> <v_g> \
 *       <r00> <r01> <r02> <r10> <r11> <r12> <r20> <r21> <r22>
 *
 * 出力サイズ: 入力のW/2 × H/2（中央領域を切り出し、リサイズなし）
 */

#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/rotation.h"
#include "../include/vector_math.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* フレーム毎呼び出し向けに compute_rotation_matrix と同等の計算を
 * デバッグ出力なしで行う。行列の各行が ex, ey, ez（回転.cと同一規約）。 */
static Matrix3x3 build_R_gaze(Vector3D G)
{
    Vector3D ez = vector_normalize(G);

    Vector3D up = vector_create(0.0, 1.0, 0.0);
    Vector3D ex_raw = vector_cross(up, ez);
    if (vector_norm(ex_raw) < 1e-10) {
        Vector3D alt = vector_create(1.0, 0.0, 0.0);
        ex_raw = vector_cross(alt, ez);
    }
    Vector3D ex = vector_normalize(ex_raw);
    Vector3D ey = vector_normalize(vector_cross(ez, ex));

    Matrix3x3 R;
    R.m[0][0] = ex.x;  R.m[0][1] = ex.y;  R.m[0][2] = ex.z;
    R.m[1][0] = ey.x;  R.m[1][1] = ey.y;  R.m[1][2] = ey.z;
    R.m[2][0] = ez.x;  R.m[2][1] = ez.y;  R.m[2][2] = ez.z;
    return R;
}

int main(int argc, char *argv[])
{
    /* 引数: prog input output u_g v_g r00..r22 → argc==14 */
    if (argc != 14) {
        fprintf(stderr,
            "使い方: %s <入力画像> <出力画像> <u_g> <v_g> "
            "<r00> <r01> <r02> <r10> <r11> <r12> <r20> <r21> <r22>\n",
            argv[0]);
        return 1;
    }

    const char *in_path  = argv[1];
    const char *out_path = argv[2];
    int u_g = atoi(argv[3]);
    int v_g = atoi(argv[4]);

    /* R_cumulative を構築 */
    Matrix3x3 R_cumulative;
    R_cumulative.m[0][0] = atof(argv[5]);
    R_cumulative.m[0][1] = atof(argv[6]);
    R_cumulative.m[0][2] = atof(argv[7]);
    R_cumulative.m[1][0] = atof(argv[8]);
    R_cumulative.m[1][1] = atof(argv[9]);
    R_cumulative.m[1][2] = atof(argv[10]);
    R_cumulative.m[2][0] = atof(argv[11]);
    R_cumulative.m[2][1] = atof(argv[12]);
    R_cumulative.m[2][2] = atof(argv[13]);

    /* 入力画像を読み込む */
    Image *img = image_load(in_path);
    if (!img) {
        fprintf(stderr, "エラー: %s を読み込めません\n", in_path);
        return 1;
    }

    int W = img->width;
    int H = img->height;

    /* 注視点 (u_g, v_g) → 世界座標 G → R_gaze */
    Vector3D G    = image_to_world(u_g, v_g, W, H);
    Matrix3x3 R_gaze = build_R_gaze(G);

    /* R_total = R_gaze × R_cumulative */
    Matrix3x3 R_total = matrix_multiply(R_gaze, R_cumulative);

    /* R_T = transpose(R_total)（逆マッピング用） */
    Matrix3x3 R_T = matrix_transpose(R_total);

    /* 切り出し範囲（フル画像座標） */
    int u_min = W / 4;
    int u_max = 3 * W / 4;
    int v_min = H / 4;
    int v_max = 3 * H / 4;
    int W_out = u_max - u_min;   /* W/2 */
    int H_out = v_max - v_min;   /* H/2 */

    /* 出力画像を確保 */
    Image *out = image_create(W_out, H_out, img->channels);
    if (!out) {
        fprintf(stderr, "エラー: 出力画像を確保できません\n");
        image_free(img);
        return 1;
    }

    /* 各出力画素に対して逆マッピング */
    for (int v_out = v_min; v_out < v_max; v_out++) {
        for (int u_out = u_min; u_out < u_max; u_out++) {
            /* 出力方向をフル画像座標として世界座標へ変換 */
            Vector3D Xp = image_to_world(u_out, v_out, W, H);

            /* R_T で入力画像上の対応方向を得る */
            Vector3D X = matrix_vector_multiply(R_T, Xp);

            /* 入力画像座標へ変換 */
            double u_in, v_in;
            world_to_image(X, W, H, &u_in, &v_in);

            /* バイリニア補間でサンプリング */
            uint8_t rgb[3];
            get_pixel_bilinear(img, u_in, v_in, rgb);

            /* 出力画像に書き込む */
            set_pixel(out, u_out - u_min, v_out - v_min, rgb);
        }
    }

    /* JPEG quality=95 で保存（stb 規約: 非0=成功） */
    if (!image_save_jpg(out_path, out, 95)) {
        fprintf(stderr, "エラー: %s に保存できません\n", out_path);
        image_free(out);
        image_free(img);
        return 1;
    }

    image_free(out);
    image_free(img);
    return 0;
}
