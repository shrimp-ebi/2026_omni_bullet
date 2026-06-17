/* lm_convergence_cases.c
 * 初期値を変えた3ケースのLM法収束実験
 *
 * 使い方:
 *   ./build/lm_convergence_cases <初期角度_度> <収束CSV出力ファイル> <サマリCSV出力ファイル>
 *
 * 例:
 *   ./build/lm_convergence_cases -29.0 results/lm_case1.csv results/lm_summary.csv
 *
 * 出力（収束CSV）: iter,E,norm_dw,C
 * 出力（サマリCSV）: init_deg,iter,final_E,est_deg,error_deg,R00,R01,...,R22
 */

#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/rotation.h"
#include "../include/vector_math.h"
#include "objective_3param.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* LM法パラメータ */
#define C_INIT      0.0001
#define EPS_OMEGA   1e-8
#define MAX_ITER    150
#define C_MAX       1e10

/* 参照画像（真値: ω₁=15°, ω₂=30°） */
static const char *BASE_PATH = "images/base/base.jpg";
static const char *REF_PATH  = "images/reference/ref_w1_15_w2_30.jpg";

/* 理論値: rodrigues({-15°*pi/180, -30°*pi/180, 0}) = R(ω₁=15°, ω₂=30°) */
static const double RTH[3][3] = {
    { 0.866792,  0.066604, -0.494201 },
    { 0.066604,  0.966698,  0.247101 },
    { 0.494201, -0.247101,  0.833490 }
};

/* Frobenius ノルムで誤差を計算 */
static double matrix_error(Matrix3x3 R)
{
    double sum = 0.0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            double d = R.m[i][j] - RTH[i][j];
            sum += d * d;
        }
    return sqrt(sum);
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr,
            "使い方: %s <初期角度_度> <収束CSV> <サマリCSV>\n"
            "例: %s -29.0 results/lm_case1.csv results/lm_summary.csv\n",
            argv[0], argv[0]);
        return 1;
    }

    double init_deg = atof(argv[1]);
    const char *conv_csv  = argv[2];
    const char *sum_csv   = argv[3];

    /* 画像読み込み */
    Image *base = image_load(BASE_PATH);
    if (!base) {
        fprintf(stderr, "エラー: 基準画像を読み込めません: %s\n", BASE_PATH);
        return 1;
    }
    Image *ref = image_load(REF_PATH);
    if (!ref) {
        fprintf(stderr, "エラー: 参照画像を読み込めません: %s\n", REF_PATH);
        image_free(base);
        return 1;
    }

    int W = base->width;
    int H = base->height;
    int u_min = W / 4, u_max = 3 * W / 4;
    int v_min = H / 4, v_max = 3 * H / 4;

    printf("=== LM収束実験: 初期値 R_Y(%.1f°) ===\n", init_deg);
    printf("画像サイズ: %d x %d\n", W, H);
    printf("比較領域: u=[%d,%d]  v=[%d,%d]\n\n", u_min, u_max, v_min, v_max);

    /* 出力ファイルを開く */
    FILE *fp_conv = fopen(conv_csv, "a");   /* 追記モード（複数ケース連結） */
    if (!fp_conv) {
        fprintf(stderr, "エラー: %s を開けません\n", conv_csv);
        image_free(base); image_free(ref);
        return 1;
    }

    FILE *fp_sum = fopen(sum_csv, "a");     /* 追記モード */
    if (!fp_sum) {
        fprintf(stderr, "エラー: %s を開けません\n", sum_csv);
        fclose(fp_conv);
        image_free(base); image_free(ref);
        return 1;
    }

    /* 初期回転行列: ω₁方向（X軸=垂直チルト）でスキャン。omega_internal = -omega_user */
    double omega_init[3] = {-init_deg * M_PI / 180.0, -30.0 * M_PI / 180.0, 0.0};
    Matrix3x3 R = rodrigues(omega_init);
    double C = C_INIT;

    double E = compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);
    printf("初期 E = %.8f\n\n", E);

    /* 収束CSVヘッダー (ケースごとに書く) */
    fprintf(fp_conv, "# case=%.1f\n", init_deg);
    fprintf(fp_conv, "case,iter,E,norm_dw,C\n");
    fprintf(fp_conv, "%.1f,0,%.10f,0.0,%.6e\n", init_deg, E, C);

    int iter;
    for (iter = 0; iter < MAX_ITER; iter++) {

        /* (a) 勾配 */
        double grad[3];
        compute_gradient_3param(base, ref, R, u_min, v_min, u_max, v_max, grad);

        /* (b) ヘッセ行列 */
        double hessian[3][3];
        compute_hessian_3param(base, ref, R, u_min, v_min, u_max, v_max, hessian);

        /* (c) LM連立方程式 */
        double A[3][3];
        double b[3];
        for (int k = 0; k < 3; k++) {
            for (int l = 0; l < 3; l++)
                A[k][l] = hessian[k][l];
            b[k] = -grad[k];
        }
        A[0][0] *= (1.0 + C);
        A[1][1] *= (1.0 + C);
        A[2][2] *= (1.0 + C);

        double delta_omega[3];
        if (solve_linear_3x3(A, b, delta_omega) != 0) {
            printf("警告: iter=%d 特異行列。Cを増加。\n", iter);
            C *= 10.0;
            if (C > C_MAX) { printf("Cが上限超過。終了。\n"); break; }
            continue;
        }

        /* (d)(e) ΔR・R */
        Matrix3x3 dR    = rodrigues(delta_omega);
        Matrix3x3 R_new = matrix_multiply(dR, R);

        /* (f) 目的関数 */
        double E_new = compute_objective_3param(base, ref, R_new, u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                            + delta_omega[1]*delta_omega[1]
                            + delta_omega[2]*delta_omega[2]);

        /* (g) 受理判定 */
        int accepted;
        if (E_new < E) {
            R = R_new;
            E = E_new;
            C = C / 10.0;
            accepted = 1;
        } else {
            C = C * 10.0;
            if (C > C_MAX) {
                printf("Cが上限超過。終了。\n");
                break;
            }
            accepted = 0;
        }

        printf("iter=%3d  E=%.8f  |Δω|=%.2e  C=%.2e  %s\n",
               iter+1, E, norm_dw, C, accepted ? "受理" : "棄却");

        fprintf(fp_conv, "%.1f,%d,%.10f,%.8e,%.6e\n",
                init_deg, iter+1, E, norm_dw, C);

        /* (h) 収束判定 */
        if (norm_dw < EPS_OMEGA) {
            printf("\n収束 (||Δω|| = %.2e < %.2e)\n", norm_dw, EPS_OMEGA);
            iter++;
            break;
        }
    }

    if (iter == MAX_ITER)
        printf("\n最大反復回数 %d 回に達しました。\n", MAX_ITER);

    /* ── 結果表示 ─────────────────────────────────────── */
    printf("\n=== 推定結果 ===\n");
    printf("推定された回転行列 R:\n");
    for (int i = 0; i < 3; i++)
        printf("  [%+9.6f  %+9.6f  %+9.6f]\n",
               R.m[i][0], R.m[i][1], R.m[i][2]);

    printf("\n理論値 R_Y(-30°):\n");
    for (int i = 0; i < 3; i++)
        printf("  [%+9.6f  %+9.6f  %+9.6f]\n",
               RTH[i][0], RTH[i][1], RTH[i][2]);

    double frob_err = matrix_error(R);
    printf("\nFrobenius誤差 ||R - Rth|| = %.8f\n", frob_err);
    printf("反復回数: %d\n", iter);
    printf("最終 E = %.8f\n", E);

    /* サマリCSVに追記 */
    /* format: init_deg,iter,final_E,frob_err,R00..R22 */
    fprintf(fp_sum, "%.1f,%d,%.10f,%.8f",
            init_deg, iter, E, frob_err);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            fprintf(fp_sum, ",%.8f", R.m[i][j]);
    fprintf(fp_sum, "\n");

    fclose(fp_conv);
    fclose(fp_sum);
    image_free(base);
    image_free(ref);
    return 0;
}
