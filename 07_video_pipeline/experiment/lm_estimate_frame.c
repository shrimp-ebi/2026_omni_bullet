/* lm_estimate_frame.c
 * 1フレーム分のLM法回転推定（B方式平滑化）
 *
 * 使い方:
 *   ./lm_estimate_frame <base> <ref> \
 *       <r00> <r01> <r02> <r10> <r11> <r12> <r20> <r21> <r22> <sigma>
 *
 * 出力（CSV1行、9要素）:
 *   r00,r01,r02,r10,r11,r12,r20,r21,r22
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

#define MAX_ITER  500
#define C_INIT    0.0001
#define EPS_OMEGA 1e-8

int main(int argc, char *argv[])
{
    /* 引数: prog base ref r00..r22(9個) sigma → argc==13 */
    if (argc != 13) {
        fprintf(stderr,
            "使い方: %s <base> <ref> "
            "<r00> <r01> <r02> <r10> <r11> <r12> <r20> <r21> <r22> <sigma>\n",
            argv[0]);
        return 1;
    }

    const char *base_path = argv[1];
    const char *ref_path  = argv[2];

    Matrix3x3 R_init;
    R_init.m[0][0] = atof(argv[3]);  R_init.m[0][1] = atof(argv[4]);  R_init.m[0][2] = atof(argv[5]);
    R_init.m[1][0] = atof(argv[6]);  R_init.m[1][1] = atof(argv[7]);  R_init.m[1][2] = atof(argv[8]);
    R_init.m[2][0] = atof(argv[9]);  R_init.m[2][1] = atof(argv[10]); R_init.m[2][2] = atof(argv[11]);

    double sigma = atof(argv[12]);

    Image *base = image_load(base_path);
    if (!base) { fprintf(stderr, "エラー: %s\n", base_path); return 1; }
    Image *ref  = image_load(ref_path);
    if (!ref)  { fprintf(stderr, "エラー: %s\n", ref_path); image_free(base); return 1; }

    /* B方式: 参照画像のみをブラーして勾配・ヘッセ計算に使う */
    Image *ref_blur = image_gaussian_blur(ref, 5, sigma);
    if (!ref_blur) {
        fprintf(stderr, "エラー: 参照画像の平滑化失敗\n");
        image_free(base); image_free(ref);
        return 1;
    }

    int W = base->width;
    int H = base->height;
    int u_min = W / 4,  u_max = 3 * W / 4;
    int v_min = H / 4,  v_max = 3 * H / 4;

    Matrix3x3 R = R_init;
    double C = C_INIT;

    /* E評価は元画像 ref を使う */
    double E = compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);

    int converged = 0;
    int iter = 0;
    for (iter = 0; iter < MAX_ITER; iter++) {

        /* 勾配・ヘッセは ref_blur を使う */
        double grad[3];
        compute_gradient_3param(base, ref_blur, R, u_min, v_min, u_max, v_max, grad);

        double hessian[3][3];
        compute_hessian_3param(base, ref_blur, R, u_min, v_min, u_max, v_max, hessian);

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

        /* E評価は元画像 ref を使う */
        double E_new = compute_objective_3param(base, ref, R_new, u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                            + delta_omega[1]*delta_omega[1]
                            + delta_omega[2]*delta_omega[2]);

        #define EPS_E 1e-6
        if (E_new < E || fabs(E_new - E) < EPS_E) {
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

    (void)converged; /* 出力はR要素のみ */

    /* 推定結果を9要素CSVで出力 */
    printf("%.10f,%.10f,%.10f,%.10f,%.10f,%.10f,%.10f,%.10f,%.10f\n",
           R.m[0][0], R.m[0][1], R.m[0][2],
           R.m[1][0], R.m[1][1], R.m[1][2],
           R.m[2][0], R.m[2][1], R.m[2][2]);

    image_free(ref_blur);
    image_free(base);
    image_free(ref);
    return 0;
}
