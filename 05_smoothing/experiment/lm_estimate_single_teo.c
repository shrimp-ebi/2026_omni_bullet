/* lm_estimate_single_teo.c
 * LM法による3パラメータ回転推定
 * 05_TEOと同じ平滑化処理: 参照画像をGaussianBlurしたあとSob elで微分を計算する
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

    Image *base = image_load(base_path);
    if (!base) { fprintf(stderr, "エラー: %s\n", base_path); return 1; }
    Image *ref  = image_load(ref_path);
    if (!ref)  { fprintf(stderr, "エラー: %s\n", ref_path); image_free(base); return 1; }

    /* 05_TEO方式: 参照画像の微分算出のためにGaussianBlurを適用 */
    Image *ref_blur = image_gaussian_blur(ref, 5, 2.0);
    if (!ref_blur) {
        fprintf(stderr, "エラー: 参照画像の平滑化失敗\n");
        image_free(base);
        image_free(ref);
        return 1;
    }

    int W = base->width;
    int H = base->height;
    int u_min = W / 4,  u_max = 3 * W / 4;
    int v_min = H / 4,  v_max = 3 * H / 4;

    /* 初期値 */
    double omega_init[3] = {
        -init_w1_deg * M_PI / 180.0,
        -init_w2_deg * M_PI / 180.0,
        0.0
    };
    Matrix3x3 R = rodrigues(omega_init);
    double C = C_INIT;
    double E = compute_objective_3param(
                   base, ref, R,
                   u_min, v_min, u_max, v_max);

    int converged = 0;
    int iter = 0;
    for (iter = 0; iter < MAX_ITER; iter++) {
        double grad[3];
        compute_gradient_3param(
            base, ref_blur, R,
            u_min, v_min, u_max, v_max, grad);

        double hessian[3][3];
        compute_hessian_3param(
            base, ref_blur, R,
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
                           base, ref, R_new,
                           u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                            + delta_omega[1]*delta_omega[1]
                            + delta_omega[2]*delta_omega[2]);

        if (E_new < E) {
            R = R_new;
            E = E_new;
            C *= 0.1;
        } else {
            C *= 10.0;
        }

        if (norm_dw < EPS_OMEGA) {
            converged = 1;
            break;
        }
    }

    printf("結果: ω1=%f, ω2=%f, ω3=%f, 収束=%d, iter=%d, E=%e\n",
           R.m[0][0], R.m[1][1], R.m[2][2], converged, iter, E);

    image_free(base);
    image_free(ref);
    image_free(ref_blur);

    return 0;
}
