/* scan_3param.c
 * create_reference_3param.c とユーザー向けの符号規約を揃えた
 * 3パラメータスキャン。
 *
 * 参照画像生成側は rodrigues(omega_user) を逆写像に使う一方で、
 * objective_3param 側は順写像 R = rodrigues(omega_internal) を評価する。
 * この差を吸収するため、スキャン時は
 *   omega_internal = -omega_user
 * として rodrigues に渡す。
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

#define DELTA            0.01   /* 数値微分のステップ（ラジアン、約0.57°） */
#define ANGLE_HALF_RANGE 10.0   /* 真値の前後に何度ずつスキャンするか */
#define ANGLE_STEP        1.0   /* スキャン刻み（度） */

static const char *OUT_FILES[3] = {
    "results/scan_3param_omega1.csv",
    "results/scan_3param_omega2.csv",
    "results/scan_3param_omega3.csv",
};

int main(void)
{
    const char *base_path = "images/base/base.jpg";
    const char *ref_path  = "images/reference/ref_w1_15_w2_30.jpg";
    const double omega_true_deg[3] = {15.0, 30.0, 0.0};

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
    printf("参照画像の真値: (ω1, ω2, ω3) = (%.1f, %.1f, %.1f) [deg]\n",
           omega_true_deg[0], omega_true_deg[1], omega_true_deg[2]);

    /* k = 0,1,2 の各パラメータについてスキャン */
    for (int k = 0; k < 3; k++) {

        FILE *fp = fopen(OUT_FILES[k], "w");
        if (!fp) {
            fprintf(stderr, "エラー: ファイルを開けません: %s\n", OUT_FILES[k]);
            image_free(base);
            image_free(ref);
            return 1;
        }

        fprintf(fp, "angle_deg,objective,grad_analytical,grad_numerical,true_angle_deg\n");
        printf("\nomega%d スキャン中 → %s\n", k + 1, OUT_FILES[k]);

        double angle_min = omega_true_deg[k] - ANGLE_HALF_RANGE;
        double angle_max = omega_true_deg[k] + ANGLE_HALF_RANGE;
        int n_steps = (int)((angle_max - angle_min) / ANGLE_STEP) + 1;
        for (int i = 0; i < n_steps; i++) {
            double angle_deg = angle_min + ANGLE_STEP * i;
            double angle_rad = angle_deg * M_PI / 180.0;

            /* ユーザー向け真値を内部表現に変換して、k番目だけ走査する */
            double omega_user[3] = {
                omega_true_deg[0] * M_PI / 180.0,
                omega_true_deg[1] * M_PI / 180.0,
                omega_true_deg[2] * M_PI / 180.0
            };
            omega_user[k] = angle_rad;

            double omega[3] = {
                -omega_user[0],
                -omega_user[1],
                -omega_user[2]
            };

            /* 回転行列を生成 */
            Matrix3x3 R = rodrigues(omega);

            /* 目的関数 */
            double E = compute_objective_3param(
                base, ref, R, u_min, v_min, u_max, v_max);

            /* 解析微分（k番目の成分） */
            double grad_a[3];
            compute_gradient_3param(
                base, ref, R, u_min, v_min, u_max, v_max, grad_a);

            /* 数値微分（k番目のパラメータ） */
            double grad_n = compute_numerical_gradient_3param(
                base, ref, R, k, DELTA, u_min, v_min, u_max, v_max);

            fprintf(fp, "%.1f,%.6f,%.6f,%.6f,%.1f\n",
                    angle_deg, E, grad_a[k], grad_n, omega_true_deg[k]);

            /* 進捗表示（10°ごと） */
            if (i % 10 == 0) {
                printf("  angle=%+.0f deg  E=%.4f  ga=%.4f  gn=%.4f\n",
                       angle_deg, E, grad_a[k], grad_n);
            }
        }

        fclose(fp);
        printf("  完了: %s\n", OUT_FILES[k]);
    }

    image_free(base);
    image_free(ref);

    printf("\n全スキャン完了。\n");
    return 0;
}
