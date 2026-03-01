/* scan_omega2.c
 * omega2（Y軸回転）方向に角度を変化させて
 * 目的関数と微分をスキャンするプログラム
 *
 * Y軸版の validate_y_rotation.c と同じ形式の CSV を出力する。
 *
 * 出力:
 *   results/objective_function_3p.csv  ... angle_deg, objective_function
 *   results/derivatives_3p.csv        ... angle_deg, analytical_derivative, numerical_derivative
 *
 * 使い方:
 *   ./build/scan_omega2 <基準画像> <参照画像> [期待角度(度)]
 *
 * 例:
 *   ./build/scan_omega2 images/base/base.jpg images/reference/reference_5deg.jpg 5.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../include/image_utils.h"
#include "../include/vector_math.h"
#include "../include/coord_transform.h"
#include "objective_3param.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 比較領域（validate_y_rotation.c と同じ値） */
#define REGION_U_MIN 2850
#define REGION_V_MIN 1425
#define REGION_U_MAX 3229
#define REGION_V_MAX 1614

/* スキャン設定（validate_y_rotation.c と同じ値） */
#define ANGLE_HALF_RANGE 10.0
#define ANGLE_STEP        0.1

int main(int argc, char *argv[])
{
    printf("===== omega2方向スキャン（Y軸版と同形式） =====\n\n");

    if (argc < 3) {
        fprintf(stderr, "使い方: %s <基準画像> <参照画像> [期待角度(度)]\n", argv[0]);
        fprintf(stderr, "例: %s images/base/base.jpg images/reference/reference_5deg.jpg 5.0\n",
                argv[0]);
        return 1;
    }

    const char *base_file = argv[1];
    const char *ref_file  = argv[2];
    double expected_deg   = (argc >= 4) ? atof(argv[3]) : 5.0;

    double angle_min = expected_deg - ANGLE_HALF_RANGE;
    double angle_max = expected_deg + ANGLE_HALF_RANGE;

    printf("基準画像: %s\n", base_file);
    printf("参照画像: %s\n", ref_file);
    printf("期待角度: %.2f deg\n", expected_deg);
    printf("スキャン範囲: %.1f deg ~ %.1f deg (刻み %.1f deg)\n\n",
           angle_min, angle_max, ANGLE_STEP);

    /* 画像読み込み */
    Image *base = image_load(base_file);
    if (!base) { fprintf(stderr, "エラー: 基準画像を読み込めません\n"); return 1; }
    Image *ref  = image_load(ref_file);
    if (!ref)  {
        fprintf(stderr, "エラー: 参照画像を読み込めません\n");
        image_free(base);
        return 1;
    }

    /* 出力ファイル */
    FILE *fp_obj   = fopen("results/objective_function_3p.csv", "w");
    FILE *fp_deriv = fopen("results/derivatives_3p.csv",        "w");
    if (!fp_obj || !fp_deriv) {
        fprintf(stderr, "エラー: results/ フォルダを作成してください\n");
        fprintf(stderr, "  mkdir -p results\n");
        image_free(base);
        image_free(ref);
        return 1;
    }

    /* Y軸版 validate_y_rotation.c と全く同じヘッダー */
    fprintf(fp_obj,   "angle_deg,objective_function\n");
    fprintf(fp_deriv, "angle_deg,analytical_derivative,numerical_derivative\n");

    int total = (int)((angle_max - angle_min) / ANGLE_STEP) + 1;
    int step  = total / 10;
    if (step < 1) step = 1;

    printf("計算点数: %d点\n処理中", total);
    fflush(stdout);

    for (double deg = angle_min; deg <= angle_max + 1e-9; deg += ANGLE_STEP) {

        int idx = (int)((deg - angle_min) / ANGLE_STEP);
        if (idx % step == 0) { printf("."); fflush(stdout); }

        /* omega2 = deg をラジアンに変換して回転行列を作る
         *
         * Y軸方向（omega2）だけに回転量を与える:
         *   omega = (0, deg_in_rad, 0)
         *   R = rodrigues(omega)
         *
         * これは validate_y_rotation.c の create_y_rotation_matrix(deg) と等価
         */
        double rad = deg * M_PI / 180.0;
        double omega[3] = {0.0, -rad, 0.0};
        Matrix3x3 R = rodrigues(omega);

        /* 目的関数 E(R) */
        double E = compute_objective_3param(
            base, ref, R,
            REGION_U_MIN, REGION_V_MIN, REGION_U_MAX, REGION_V_MAX);

        /* 解析微分: grad[1] = dE/domega2（Y軸成分）*/
        double grad[3];
        compute_gradient_3param(
            base, ref, R,
            REGION_U_MIN, REGION_V_MIN, REGION_U_MAX, REGION_V_MAX,
            grad);
        double ana = grad[1];

        /* 数値微分: omega2方向（k=1）の中央差分 */
        double delta = 1e-3;  /* ラジアン */
        double num = compute_numerical_gradient_3param(
            base, ref, R, 1, delta,
            REGION_U_MIN, REGION_V_MIN, REGION_U_MAX, REGION_V_MAX);

        fprintf(fp_obj,   "%.2f,%.6f\n",      deg, E);
        fprintf(fp_deriv, "%.2f,%.6f,%.6f\n", deg, ana, num);
    }

    printf(" 完了！\n");

    fclose(fp_obj);
    fclose(fp_deriv);

    /* 期待角度を保存（plot_results_3p.py から参照） */
    FILE *fp_exp = fopen("results/expected_angle_3p.txt", "w");
    if (fp_exp) {
        fprintf(fp_exp, "%.6f\n", expected_deg);
        fclose(fp_exp);
    }

    image_free(base);
    image_free(ref);

    printf("\n結果ファイル:\n");
    printf("  results/objective_function_3p.csv\n");
    printf("  results/derivatives_3p.csv\n");
    printf("\nグラフ描画:\n");
    printf("  python3 experiment/plot_results_3p.py --expected-angle %.2f\n", expected_deg);

    return 0;
}