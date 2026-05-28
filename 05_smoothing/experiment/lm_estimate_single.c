/* lm_estimate_single.c
 * LM法による3パラメータ回転推定（初期値をコマンドライン引数で受け取る版）
 *
 * 使い方:
 *   ./lm_estimate_single <ω1初期値[deg]> <ω2初期値[deg]>
 *
 * 出力（1行CSV）:
 *   ω1初期値, ω2初期値, 収束フラグ(1=収束/0=失敗), 反復回数, Frobenius誤差, 最終E
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

#define C_INIT      0.0001
#define EPS_OMEGA   1e-8
#define MAX_ITER    200

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "使い方: %s <ω1[deg]> <ω2[deg]>\n", argv[0]);
        return 1;
    }

    double init_w1_deg = atof(argv[1]);
    double init_w2_deg = atof(argv[2]);

    const char *base_path = "images/base/base.jpg";
    const char *ref_path  = "images/reference/ref_w1_15_w2_30.jpg";

    /* 真値 */
    const double omega_true_deg[3] = {15.0, 30.0, 0.0};

    Image *base = image_load(base_path);
    if (!base) { fprintf(stderr, "エラー: %s\n", base_path); return 1; }
    Image *ref  = image_load(ref_path);
    if (!ref)  { fprintf(stderr, "エラー: %s\n", ref_path); image_free(base); return 1; }

    int W = base->width;
    int H = base->height;
    int u_min = W / 4,  u_max = 3 * W / 4;
    int v_min = H / 4,  v_max = 3 * H / 4;

    Image *base_smooth = image_gaussian_blur(base, 5, 2.0);
    Image *ref_smooth  = image_gaussian_blur(ref,  5, 2.0);
    if (!base_smooth || !ref_smooth) {
        fprintf(stderr, "エラー: 平滑化失敗\n");
        image_free(base); image_free(ref);
        return 1;
    }

    /* 真値の回転行列 */
    double omega_true[3] = {
        -omega_true_deg[0] * M_PI / 180.0,
        -omega_true_deg[1] * M_PI / 180.0,
        0.0
    };
    Matrix3x3 Rth = rodrigues(omega_true);

    /* 初期値 */
    double omega_init[3] = {
        -init_w1_deg * M_PI / 180.0,
        -init_w2_deg * M_PI / 180.0,
        0.0
    };
    Matrix3x3 R = rodrigues(omega_init);
    double C = C_INIT;
    double E = compute_objective_3param(
                   base_smooth, ref_smooth, R,
                   u_min, v_min, u_max, v_max);

    int converged = 0;
    int iter = 0;
    for (iter = 0; iter < MAX_ITER; iter++) {
        double grad[3];
        compute_gradient_3param(
            base_smooth, ref_smooth, R,
            u_min, v_min, u_max, v_max, grad);

        double hessian[3][3];
        compute_hessian_3param(
            base_smooth, ref_smooth, R,
            u_min, v_min, u_max, v_max, hessian);

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
            C *= 10.0;
            continue;
        }

        Matrix3x3 dR    = rodrigues(delta_omega);
        Matrix3x3 R_new = matrix_multiply(dR, R);
        double E_new = compute_objective_3param(
                           base_smooth, ref_smooth, R_new,
                           u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                            + delta_omega[1]*delta_omega[1]
                            + delta_omega[2]*delta_omega[2]);

        if (E_new < E) {
            R = R_new;
            E = E_new;
            C /= 10.0;
        } else {
            C *= 10.0;
        }

        if (norm_dw < EPS_OMEGA) {
            converged = 1;
            iter++;
            break;
        }
    }

    /* Frobenius誤差 */
    double frob = 0.0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            double d = R.m[i][j] - Rth.m[i][j];
            frob += d * d;
        }
    frob = sqrt(frob);

    /* 収束とみなす閾値: Frobenius誤差 < 0.01 */
    int success = converged && (frob < 0.01);

    /* 結果出力（CSV1行） */
    printf("%.2f,%.2f,%d,%d,%.6f,%.6f\n",
           init_w1_deg, init_w2_deg,
           success, iter, frob, E);

    image_free(base_smooth);
    image_free(ref_smooth);
    image_free(base);
    image_free(ref);
    return 0;
}
