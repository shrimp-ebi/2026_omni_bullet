/* lm_estimate.c
 * LM法による3パラメータ回転推定
 *
 * 基準画像と参照画像を使って、LM法で回転行列Rを推定する。
 * 理論資料 6.2節に基づく実装。
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

#define C_INIT      0.0001   /* LM法の初期定数 */
#define EPS_OMEGA   1e-8     /* 収束判定のしきい値 */
#define MAX_ITER    100      /* 最大反復回数 */

int main(int argc, char **argv)
{
    int ksize = 5;
    double sigma = 2.0;
    if (argc >= 2) {
        int v = atoi(argv[1]);
        if (v > 0) ksize = v;
    }
    if (argc >= 3) {
        double s = atof(argv[2]);
        if (s > 0.0) sigma = s;
    }
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

    Image *base_smooth = image_gaussian_blur(base, ksize, sigma);
    if (!base_smooth) {
        fprintf(stderr, "エラー: 基準画像の平滑化に失敗しました\n");
        image_free(ref);
        image_free(base);
        return 1;
    }
    Image *ref_smooth = image_gaussian_blur(ref, ksize, sigma);
    if (!ref_smooth) {
        fprintf(stderr, "エラー: 参照画像の平滑化に失敗しました\n");
        image_free(base_smooth);
        image_free(ref);
        image_free(base);
        return 1;
    }

    printf("使用する平滑化パラメータ: ksize=%d sigma=%.3f\n", ksize, sigma);

    /* 差分画像を保存 */
    Image *base_diff = image_difference(base, base_smooth);
    if (base_diff) {
        image_save_png("base_diff.png", base_diff);
        image_free(base_diff);
    }
    Image *ref_diff = image_difference(ref, ref_smooth);
    if (ref_diff) {
        image_save_png("ref_diff.png", ref_diff);
        image_free(ref_diff);
    }

    printf("画像サイズ: %d x %d\n", W, H);
    printf("比較領域: u=[%d,%d]  v=[%d,%d]\n\n", u_min, u_max, v_min, v_max);

    /* 真値: objective_3param は順写像で評価するため omega の符号を反転して渡す */
    double omega_true_user[3] = {
        omega_true_deg[0] * M_PI / 180.0,
        omega_true_deg[1] * M_PI / 180.0,
        omega_true_deg[2] * M_PI / 180.0
    };
    double omega_true[3] = {
        -omega_true_user[0],
        -omega_true_user[1],
        -omega_true_user[2]
    };
    Matrix3x3 Rth = rodrigues(omega_true);

    printf("=== 理論値 rodrigues({%.1f°, %.1f°, %.1f°}) ===\n",
           omega_true_deg[0], omega_true_deg[1], omega_true_deg[2]);
    for (int i = 0; i < 3; i++)
        printf("  [%8.5f  %8.5f  %8.5f]\n",
               Rth.m[i][0], Rth.m[i][1], Rth.m[i][2]);
    printf("\n");

    /* 3ケースの初期値 */
    const double cases[3][3] = {
        {14.0, 29.0, 0.0},   /* ケース1: 真値から約1° */
        {12.0, 27.0, 0.0},   /* ケース2: 真値から約3° */
        {10.0, 25.0, 0.0},   /* ケース3: 真値から約5° */
    };

    for (int cas = 0; cas < 3; cas++) {

        printf("=========================================\n");
        printf("ケース%d: 初期値 (ω1, ω2, ω3) = (%.1f, %.1f, %.1f) [deg]\n",
               cas + 1, cases[cas][0], cases[cas][1], cases[cas][2]);
        printf("=========================================\n");

        /* 初期値: objective_3param は順写像で評価するため omega の符号を反転して渡す */
        double omega_user[3] = {
            cases[cas][0] * M_PI / 180.0,
            cases[cas][1] * M_PI / 180.0,
            cases[cas][2] * M_PI / 180.0
        };
        double omega_init[3] = {-omega_user[0], -omega_user[1], -omega_user[2]};
        Matrix3x3 R = rodrigues(omega_init);
        double C = C_INIT;

        double E = compute_objective_3param(base_smooth, ref_smooth, R, u_min, v_min, u_max, v_max);
        printf("初期 E = %f\n\n", E);

        /* ── LM 反復 ──────────────────────────────────────── */
        int iter;
        for (iter = 0; iter < MAX_ITER; iter++) {

            /* (a) 勾配 */
            double grad[3];
            compute_gradient_3param(base_smooth, ref_smooth, R, u_min, v_min, u_max, v_max, grad);

            /* (b) ヘッセ行列 */
            double hessian[3][3];
            compute_hessian_3param(base_smooth, ref_smooth, R, u_min, v_min, u_max, v_max, hessian);

            /* (c) LM 連立方程式  (H + C*diag(H)) Δω = -g */
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
                printf("警告: iter=%d で連立方程式が解けません（特異行列）。Cを増加。\n", iter);
                C *= 10.0;
                continue;
            }

            /* (d)(e) ΔR・R */
            Matrix3x3 dR    = rodrigues(delta_omega);
            Matrix3x3 R_new = matrix_multiply(dR, R);

            /* (f) 目的関数 */
            double E_new = compute_objective_3param(base_smooth, ref_smooth, R_new, u_min, v_min, u_max, v_max);

            /* ||Δω|| */
            double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                                + delta_omega[1]*delta_omega[1]
                                + delta_omega[2]*delta_omega[2]);

            /* (g) 更新の受理判定 */
            int accepted;
            if (E_new < E) {
                R = R_new;
                E = E_new;
                C = C / 10.0;
                accepted = 1;
            } else {
                C = C * 10.0;
                accepted = 0;
            }
            printf("iter=%d  E=%.6f  |Δω|=%.2e  C=%.2e  %s\n",
                   iter, E, norm_dw, C, accepted ? "受理" : "棄却");

            /* (h) 収束判定 */
            if (norm_dw < EPS_OMEGA) {
                printf("\n収束しました（||Δω|| = %.2e < %.2e）\n", norm_dw, EPS_OMEGA);
                break;
            }
        }

        if (iter == MAX_ITER)
            printf("\n最大反復回数 %d 回に達しました。\n", MAX_ITER);

        /* ── 推定結果 ─────────────────────────────────────── */
        printf("\n=== 推定された回転行列 R ===\n");
        for (int i = 0; i < 3; i++)
            printf("  [%8.5f  %8.5f  %8.5f]\n",
                   R.m[i][0], R.m[i][1], R.m[i][2]);

        printf("\n=== 誤差（推定 - 理論） ===\n");
        for (int i = 0; i < 3; i++) {
            printf("  [%+8.5f  %+8.5f  %+8.5f]\n",
                   R.m[i][0] - Rth.m[i][0],
                   R.m[i][1] - Rth.m[i][1],
                   R.m[i][2] - Rth.m[i][2]);
        }

        double frob = 0.0;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                frob += (R.m[i][j] - Rth.m[i][j]) * (R.m[i][j] - Rth.m[i][j]);
        printf("Frobenius誤差: %.6f\n", sqrt(frob));
        printf("最終 E = %f\n\n", E);
    }

    image_free(base_smooth);
    image_free(ref_smooth);
    image_free(base);
    image_free(ref);
    return 0;
}
