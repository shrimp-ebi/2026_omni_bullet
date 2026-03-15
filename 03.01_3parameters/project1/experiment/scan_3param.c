/* scan_3param.c
 * 各回転パラメータ（ω₁, ω₂, ω₃）を単独で動かして
 * 目的関数と微分（解析・数値）をスキャンし、CSVに出力する。
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

#define DELTA      0.01   /* 数値微分のステップ（ラジアン、約0.57°） */
#define ANGLE_MIN  -45.0  /* スキャン開始角度（度） */
#define ANGLE_MAX   45.0  /* スキャン終了角度（度） */
#define ANGLE_STEP   1.0  /* スキャン刻み（度） */

static const char *OUT_FILES[3] = {
    "results/scan_3param_omega1.csv",
    "results/scan_3param_omega2.csv",
    "results/scan_3param_omega3.csv",
};

int main(void)
{
    const char *base_path = "images/base/base.jpg";
    const char *ref_path  = "images/reference/reference_30deg.jpg";

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

    /* k = 0,1,2 の各パラメータについてスキャン */
    for (int k = 0; k < 3; k++) {

        FILE *fp = fopen(OUT_FILES[k], "w");
        if (!fp) {
            fprintf(stderr, "エラー: ファイルを開けません: %s\n", OUT_FILES[k]);
            image_free(base);
            image_free(ref);
            return 1;
        }

        fprintf(fp, "angle_deg,objective,grad_analytical,grad_numerical\n");
        printf("\nomega%d スキャン中 → %s\n", k + 1, OUT_FILES[k]);

        int n_steps = (int)((ANGLE_MAX - ANGLE_MIN) / ANGLE_STEP) + 1;
        for (int i = 0; i < n_steps; i++) {
            double angle_deg = ANGLE_MIN + ANGLE_STEP * i;
            double angle_rad = angle_deg * M_PI / 180.0;

            /* omega[k] だけ設定、他は 0 */
            double omega[3] = {0.0, 0.0, 0.0};
            omega[k] = angle_rad;

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

            fprintf(fp, "%.1f,%.6f,%.6f,%.6f\n",
                    angle_deg, E, grad_a[k], grad_n);

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
