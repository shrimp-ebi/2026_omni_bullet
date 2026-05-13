/* lm_scan_omega2.c
 * ω₂方向の収束域スキャン用 LM法実行バイナリ
 *
 * 使い方:
 *   ./build/lm_scan_omega2 <init_omega2_deg> <収束CSV> <サマリCSV> <ref_path>
 *
 * 固定設定:
 *   基準画像 : images/base/base.jpg
 *   初期ω₁  : 15°（TRUE_W1_DEG固定）
 *   初期ω₃  : 0°（固定）
 *   真値     : (ω₁, ω₂, ω₃) = (15°, 15°, 0°)
 *
 * 符号規約（scan_3param.c / lm_scan_omega1.c と同じ）:
 *   omega_internal = -omega_user として rodrigues に渡す
 *
 * サマリCSV形式（1行）:
 *   init_deg, iter, final_E, frob_err, omega2_est_deg, R00..R22
 */

#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/rotation.h"
#include "../include/vector_math.h"
#include "objective_3param.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* LM法パラメータ */
#define C_INIT    0.0001
#define EPS_OMEGA 1e-8
#define MAX_ITER  500
#define C_MAX     1e10

static const char *BASE_PATH = "images/base/base.jpg";

static const double TRUE_W1_DEG = 15.0;
static const double TRUE_W2_DEG = 15.0;
static const double TRUE_W3_DEG =  0.0;

/* Frobenius 誤差 ||R - Rth|| */
static double matrix_frob_error(Matrix3x3 R, Matrix3x3 Rth)
{
    double sum = 0.0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            double d = R.m[i][j] - Rth.m[i][j];
            sum += d * d;
        }
    return sqrt(sum);
}

/*
 * 回転行列 R = rodrigues(omega_internal) から
 * ω₂（ユーザー向け, 度）を復元する。
 *
 * ロドリゲスの逆変換:
 *   θ = arccos((trace(R)-1)/2)
 *   omega_internal_y = θ/(2 sinθ) * (R[0][2] - R[2][0])
 *   omega_user_y = -omega_internal_y
 */
static double extract_omega2_user_deg(Matrix3x3 R)
{
    double tr = R.m[0][0] + R.m[1][1] + R.m[2][2];
    double cos_th = (tr - 1.0) / 2.0;
    if (cos_th >  1.0) cos_th =  1.0;
    if (cos_th < -1.0) cos_th = -1.0;
    double theta = acos(cos_th);
    if (fabs(sin(theta)) < 1e-10)
        return 0.0;
    double factor = theta / (2.0 * sin(theta));
    double omega_int1 = factor * (R.m[0][2] - R.m[2][0]);  /* y成分 */
    return -omega_int1 * 180.0 / M_PI;  /* omega_user = -omega_internal */
}

int main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr,
            "使い方: %s <init_omega2_deg> <収束CSV> <サマリCSV> <ref_path>\n"
            "例: %s 14.0 results/conv.csv results/sum.csv"
            " images/reference/ref_w1_15_w2_15.jpg\n",
            argv[0], argv[0]);
        return 1;
    }

    double init_deg        = atof(argv[1]);
    const char *conv_csv   = argv[2];
    const char *sum_csv    = argv[3];
    const char *ref_path   = argv[4];

    /* 画像読み込み */
    Image *base = image_load(BASE_PATH);
    if (!base) {
        fprintf(stderr, "エラー: 基準画像を読み込めません: %s\n", BASE_PATH);
        return 1;
    }
    Image *ref = image_load(ref_path);
    if (!ref) {
        fprintf(stderr, "エラー: 参照画像を読み込めません: %s\n", ref_path);
        image_free(base);
        return 1;
    }

    int W = base->width, H = base->height;
    int u_min = W / 4, u_max = 3 * W / 4;
    int v_min = H / 4, v_max = 3 * H / 4;

    printf("=== LM収束実験(ω₂スキャン): 初期値 ω₂=%.1f° ===\n", init_deg);

    /* 真の回転行列: rodrigues(-omega_true_user) */
    double omega_true_int[3] = {
        -TRUE_W1_DEG * M_PI / 180.0,
        -TRUE_W2_DEG * M_PI / 180.0,
        -TRUE_W3_DEG * M_PI / 180.0
    };
    Matrix3x3 Rth = rodrigues(omega_true_int);

    /* 初期回転行列: ω₁=TRUE_W1_DEG固定, ω₂=init_deg スキャン */
    double omega_init[3] = {
        -TRUE_W1_DEG * M_PI / 180.0,
        -init_deg    * M_PI / 180.0,
        -TRUE_W3_DEG * M_PI / 180.0
    };
    Matrix3x3 R = rodrigues(omega_init);
    double C = C_INIT;

    double E = compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);
    printf("初期 E = %.8f\n\n", E);

    /* 出力ファイルを開く（追記モード） */
    FILE *fp_conv = fopen(conv_csv, "a");
    if (!fp_conv) {
        fprintf(stderr, "エラー: %s を開けません\n", conv_csv);
        image_free(base); image_free(ref);
        return 1;
    }
    FILE *fp_sum = fopen(sum_csv, "a");
    if (!fp_sum) {
        fprintf(stderr, "エラー: %s を開けません\n", sum_csv);
        fclose(fp_conv);
        image_free(base); image_free(ref);
        return 1;
    }

    fprintf(fp_conv, "# case=%.1f\n", init_deg);
    fprintf(fp_conv, "case,iter,E,norm_dw,C\n");
    fprintf(fp_conv, "%.1f,0,%.10f,0.0,%.6e\n", init_deg, E, C);

    /* ── LM 反復 ──────────────────────────────────────────── */
    int iter;
    for (iter = 0; iter < MAX_ITER; iter++) {

        double grad[3];
        compute_gradient_3param(base, ref, R, u_min, v_min, u_max, v_max, grad);

        double hessian[3][3];
        compute_hessian_3param(base, ref, R, u_min, v_min, u_max, v_max, hessian);

        double A[3][3], b[3];
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

        Matrix3x3 dR    = rodrigues(delta_omega);
        Matrix3x3 R_new = matrix_multiply(dR, R);

        double E_new = compute_objective_3param(base, ref, R_new, u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                            + delta_omega[1]*delta_omega[1]
                            + delta_omega[2]*delta_omega[2]);

        int accepted;
        if (E_new < E) {
            R = R_new;
            E = E_new;
            C = C / 10.0;
            accepted = 1;
        } else {
            C = C * 10.0;
            if (C > C_MAX) { printf("Cが上限超過。終了。\n"); break; }
            accepted = 0;
        }

        printf("iter=%3d  E=%.8f  |Δω|=%.2e  C=%.2e  %s\n",
               iter+1, E, norm_dw, C, accepted ? "受理" : "棄却");
        fprintf(fp_conv, "%.1f,%d,%.10f,%.8e,%.6e\n",
                init_deg, iter+1, E, norm_dw, C);

        if (norm_dw < EPS_OMEGA) {
            printf("\n収束 (||Δω|| = %.2e < %.2e)\n", norm_dw, EPS_OMEGA);
            iter++;
            break;
        }
    }

    if (iter == MAX_ITER)
        printf("\n最大反復回数 %d 回に達しました。\n", MAX_ITER);

    /* ── 推定結果 ────────────────────────────────────────── */
    double frob_err   = matrix_frob_error(R, Rth);
    double omega2_est = extract_omega2_user_deg(R);

    printf("\n=== 推定結果 ===\n");
    printf("推定 ω₂ (ユーザー向け): %.4f °\n", omega2_est);
    printf("Frobenius誤差 ||R - Rth|| = %.8f\n", frob_err);
    printf("反復回数: %d\n", iter);
    printf("最終 E = %.8f\n", E);

    /* サマリCSV: init_deg,iter,final_E,frob_err,omega2_est_deg,R00..R22 */
    fprintf(fp_sum, "%.1f,%d,%.10f,%.8f,%.6f",
            init_deg, iter, E, frob_err, omega2_est);
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
