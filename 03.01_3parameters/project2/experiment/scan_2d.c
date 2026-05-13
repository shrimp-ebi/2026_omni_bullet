/* scan_2d.c
 * ω₁-ω₂ 2次元スキャン：目的関数 E の値を記録する。
 *
 * 真値は ω₁=15°, ω₂=30°, ω₃=0°（固定）。
 * scan_3param.c と同じ符号規約を使用する：
 *   omega_internal = -omega_user として objective_3param に渡す
 */

#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/rotation.h"
#include "objective_3param.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ANGLE_HALF_RANGE 10.0   /* 真値の前後に何度ずつスキャンするか */
#define ANGLE_STEP        0.1   /* スキャン刻み（度） */

int main(void)
{
    const char *base_path = "images/base/base3.jpg";
    const char *ref_path  = "images/reference/ref_base3_w1_15_w2_30.jpg";
    const char *out_path  = "results/scan_2d.csv";

    /* 真値 */
    const double omega1_true_deg = 15.0;
    const double omega2_true_deg = 30.0;
    const double omega3_true_deg =  0.0;  /* 固定 */

    Image *base = image_load(base_path);
    if (!base) {
        fprintf(stderr, "エラー: 基準画像を読み込めません: %s\n", base_path);
        return 1;
    }
    Image *ref = image_load(ref_path);
    if (!ref) {
        fprintf(stderr, "エラー: 参照画像を読み込めません: %s\n", ref_path);
        image_free(base);
        return 1;
    }

    int W = base->width;
    int H = base->height;
    int u_min = W / 4,  u_max = 3 * W / 4;
    int v_min = H / 4,  v_max = 3 * H / 4;

    printf("画像サイズ: %d x %d\n", W, H);
    printf("比較領域: u=[%d,%d]  v=[%d,%d]\n", u_min, u_max, v_min, v_max);
    printf("真値: (ω1, ω2, ω3) = (%.1f, %.1f, %.1f) [deg]\n",
           omega1_true_deg, omega2_true_deg, omega3_true_deg);

    /* スキャン範囲 */
    double w1_min = omega1_true_deg - ANGLE_HALF_RANGE;
    double w1_max = omega1_true_deg + ANGLE_HALF_RANGE;
    double w2_min = omega2_true_deg - ANGLE_HALF_RANGE;
    double w2_max = omega2_true_deg + ANGLE_HALF_RANGE;

    int n1 = (int)((w1_max - w1_min) / ANGLE_STEP) + 1;
    int n2 = (int)((w2_max - w2_min) / ANGLE_STEP) + 1;

    printf("ω₁ スキャン範囲: %.1f°〜%.1f°  (%d点)\n", w1_min, w1_max, n1);
    printf("ω₂ スキャン範囲: %.1f°〜%.1f°  (%d点)\n", w2_min, w2_max, n2);
    printf("合計 %d 点を計算します\n\n", n1 * n2);

    FILE *fp = fopen(out_path, "w");
    if (!fp) {
        fprintf(stderr, "エラー: 出力ファイルを開けません: %s\n", out_path);
        image_free(base);
        image_free(ref);
        return 1;
    }

    fprintf(fp, "omega1_deg,omega2_deg,E\n");

    int total = n1 * n2;
    int count = 0;

    for (int i = 0; i < n1; i++) {
        double w1_deg = w1_min + ANGLE_STEP * i;

        for (int j = 0; j < n2; j++) {
            double w2_deg = w2_min + ANGLE_STEP * j;

            /* ユーザー向け角度を内部表現に変換（符号反転） */
            double omega[3] = {
                -(w1_deg          * M_PI / 180.0),
                -(w2_deg          * M_PI / 180.0),
                -(omega3_true_deg * M_PI / 180.0)
            };

            /* 回転行列を生成 */
            Matrix3x3 R = rodrigues(omega);

            /* 目的関数を計算 */
            double E = compute_objective_3param(
                base, ref, R, u_min, v_min, u_max, v_max);

            fprintf(fp, "%.1f,%.1f,%.10f\n", w1_deg, w2_deg, E);

            count++;
            if (count % 50 == 0 || count == total) {
                printf("  進捗: %d / %d  (ω1=%.1f°, ω2=%.1f°, E=%.4f)\n",
                       count, total, w1_deg, w2_deg, E);
            }
        }
    }

    fclose(fp);
    image_free(base);
    image_free(ref);

    printf("\n完了: %s に %d 点を書き込みました。\n", out_path, total);
    return 0;
}
