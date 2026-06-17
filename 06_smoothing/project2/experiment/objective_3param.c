/* objective_3param.c
 * 3パラメータ回転の目的関数・微分計算
 *
 * 池内論文 式(3.2) ~ (3.8), (3.17) の実装
 */

#include "objective_3param.h"
#include "../include/coord_transform.h"
#include "../include/rotation.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ∂X'/∂ωk を計算する（k=0,1,2 → ω₁,ω₂,ω₃）
 *
 * 池内論文 式(3.8):
 *   ∂X'/∂ω₁ = [  0    0    0  ] X
 *              [-R₃₁ -R₃₂ -R₃₃]
 *              [ R₂₁  R₂₂  R₂₃]
 *
 *   ∂X'/∂ω₂ = [ R₃₁  R₃₂  R₃₃] X
 *              [  0    0    0  ]
 *              [-R₁₁ -R₁₂ -R₁₃]
 *
 *   ∂X'/∂ω₃ = [-R₂₁ -R₂₂ -R₂₃] X
 *              [ R₁₁  R₁₂  R₁₃]
 *              [  0    0    0  ]
 *
 * 注意: Rij は行列Rの i行j列成分（1-indexed）
 *        C言語では R.m[i-1][j-1]
 */
static Vector3D dXprime_domega(Matrix3x3 R, Vector3D X, int k) {
    Vector3D result;
    double Xx = X.x, Xy = X.y, Xz = X.z;

    /* Rの行ベクトルを取り出す（式の Rij に対応） */
    double R1x = R.m[0][0], R1y = R.m[0][1], R1z = R.m[0][2];  /* 1行目 */
    double R2x = R.m[1][0], R2y = R.m[1][1], R2z = R.m[1][2];  /* 2行目 */
    double R3x = R.m[2][0], R3y = R.m[2][1], R3z = R.m[2][2];  /* 3行目 */

    if (k == 0) {
        /* ∂X'/∂ω₁ */
        result.x = 0.0;
        result.y = -(R3x*Xx + R3y*Xy + R3z*Xz);
        result.z =   R2x*Xx + R2y*Xy + R2z*Xz;
    } else if (k == 1) {
        /* ∂X'/∂ω₂ */
        result.x =   R3x*Xx + R3y*Xy + R3z*Xz;
        result.y = 0.0;
        result.z = -(R1x*Xx + R1y*Xy + R1z*Xz);
    } else {
        /* ∂X'/∂ω₃ */
        result.x = -(R2x*Xx + R2y*Xy + R2z*Xz);
        result.y =   R1x*Xx + R1y*Xy + R1z*Xz;
        result.z = 0.0;
    }
    return result;
}

/* ===========================
 * 目的関数
 * =========================== */

double compute_objective_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max)
{
    int W = base->width;
    int H = base->height;
    double sum = 0.0;
    int count = 0;

    for (int v = v_min; v <= v_max; v++) {
        for (int u = u_min; u <= u_max; u++) {

            /* 基準画像の点を世界座標へ */
            Vector3D X = image_to_world(u, v, W, H);

            /* 回転: X' = R * X */
            Vector3D Xp = matrix_vector_multiply(R, X);

            /* 参照画像の座標へ変換 */
            double u_ref, v_ref;
            world_to_image(Xp, W, H, &u_ref, &v_ref);

            /* 輝度値を取得 */
            uint8_t rgb_b[3];
            get_pixel(base, u, v, rgb_b);
            double Sb = (rgb_b[0] + rgb_b[1] + rgb_b[2]) / 3.0;
            double Sr = image_gray_bilinear(ref, u_ref, v_ref);

            double diff = Sr - Sb;
            sum += diff * diff;
            count++;
        }
    }

    /* 式(3.2): E = 1/(2N) Σ(Sr - Sb)² */
    return (count > 0) ? sum / (2.0 * count) : 0.0;
}

/* ===========================
 * 1階微分（勾配）
 * =========================== */

void compute_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double grad[3])
{
    int W = base->width;
    int H = base->height;
    double sum[3] = {0.0, 0.0, 0.0};
    int count = 0;

    for (int v = v_min; v <= v_max; v++) {
        for (int u = u_min; u <= u_max; u++) {

            /* 基準画像の点を世界座標へ */
            Vector3D X = image_to_world(u, v, W, H);

            /* 回転: X' = R * X */
            Vector3D Xp = matrix_vector_multiply(R, X);

            /* 参照画像の座標へ変換 */
            double u_ref, v_ref;
            world_to_image(Xp, W, H, &u_ref, &v_ref);

            /* 輝度差 (Sr - Sb) */
            uint8_t rgb_b[3];
            get_pixel(base, u, v, rgb_b);
            double Sb = (rgb_b[0] + rgb_b[1] + rgb_b[2]) / 3.0;
            double Sr = image_gray_bilinear(ref, u_ref, v_ref);
            double diff = Sr - Sb;

            /* ∂S/∂θ, ∂S/∂φ（参照画像の微分） */
            double dS_dtheta, dS_dphi;
            image_derivative_theta_phi_central(ref, u_ref, v_ref, &dS_dtheta, &dS_dphi);

            /* ∂θ/∂X', ∂φ/∂X' など（式3.13） */
            double theta_p, phi_p;
            world_to_angle(Xp, &theta_p, &phi_p);
            double dth_dX, dth_dY, dth_dZ;
            double dph_dX, dph_dY, dph_dZ;
            angle_jacobian_xyz(theta_p, phi_p,
                           &dth_dX, &dth_dY, &dth_dZ,
                           &dph_dX, &dph_dY, &dph_dZ);

            /* ∂Sr/∂X', ∂Sr/∂Y', ∂Sr/∂Z'（連鎖律） */
            double dSr_dXp = dS_dtheta * dth_dX + dS_dphi * dph_dX;
            double dSr_dYp = dS_dtheta * dth_dY + dS_dphi * dph_dY;
            double dSr_dZp = dS_dtheta * dth_dZ + dS_dphi * dph_dZ;

            /* 各 ωk についての勾配を計算（式3.3 + 式3.8） */
            for (int k = 0; k < 3; k++) {
                /* ∂X'/∂ωk（式3.8） */
                Vector3D dXp_domk = dXprime_domega(R, X, k);

                /* 連鎖律: ∂Sr/∂ωk = ∂Sr/∂X' * ∂X'/∂ωk + ... */
                double chain = dSr_dXp * dXp_domk.x
                             + dSr_dYp * dXp_domk.y
                             + dSr_dZp * dXp_domk.z;

                /* 式(3.3): ∂E/∂ωk = (1/N) Σ (Sr - Sb) * chain */
                sum[k] += diff * chain;
            }
            count++;
        }
    }

    for (int k = 0; k < 3; k++) {
        grad[k] = (count > 0) ? sum[k] / (double)count : 0.0;
    }
}

/* ===========================
 * 2階微分（ヘッセ行列）
 * =========================== */

void compute_hessian_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double hessian[3][3])
{
    int W = base->width;
    int H = base->height;
    double sum[3][3];
    int count = 0;

    /* 初期化 */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            sum[i][j] = 0.0;

    for (int v = v_min; v <= v_max; v++) {
        for (int u = u_min; u <= u_max; u++) {

            Vector3D X = image_to_world(u, v, W, H);
            Vector3D Xp = matrix_vector_multiply(R, X);
            double u_ref, v_ref;
            world_to_image(Xp, W, H, &u_ref, &v_ref);

            /* ∂S/∂θ, ∂S/∂φ */
            double dS_dtheta, dS_dphi;
            image_derivative_theta_phi_central(ref, u_ref, v_ref, &dS_dtheta, &dS_dphi);

            /* ∂θ/∂X', ∂φ/∂X' など */
            double theta_p, phi_p;
            world_to_angle(Xp, &theta_p, &phi_p);
            double dth_dX, dth_dY, dth_dZ;
            double dph_dX, dph_dY, dph_dZ;
            angle_jacobian_xyz(theta_p, phi_p,
                           &dth_dX, &dth_dY, &dth_dZ,
                           &dph_dX, &dph_dY, &dph_dZ);

            double dSr_dXp = dS_dtheta * dth_dX + dS_dphi * dph_dX;
            double dSr_dYp = dS_dtheta * dth_dY + dS_dphi * dph_dY;
            double dSr_dZp = dS_dtheta * dth_dZ + dS_dphi * dph_dZ;

            /* j_k = ∂Sr/∂ωk（連鎖律の結果） */
            double j[3];
            for (int k = 0; k < 3; k++) {
                Vector3D dXp = dXprime_domega(R, X, k);
                j[k] = dSr_dXp * dXp.x
                      + dSr_dYp * dXp.y
                      + dSr_dZp * dXp.z;
            }

            /* 式(3.4): ∂²E/∂ωk∂ωl = (1/N) Σ j_k * j_l */
            for (int k = 0; k < 3; k++)
                for (int l = 0; l < 3; l++)
                    sum[k][l] += j[k] * j[l];

            count++;
        }
    }

    for (int k = 0; k < 3; k++)
        for (int l = 0; l < 3; l++)
            hessian[k][l] = (count > 0) ? sum[k][l] / (double)count : 0.0;
}

/* ===========================
 * 数値微分（検証用）
 * =========================== */

double compute_numerical_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int param_idx, double delta,
    int u_min, int v_min, int u_max, int v_max)
{
    /* 中央差分: dE/dωk ≈ (E(R+) - E(R-)) / (2*delta)
     *
     * 前進差分より精度が高い（誤差 O(delta^2) vs O(delta)）
     */

    /* R+ = rodrigues(+delta * ek) * R */
    double omega_plus[3] = {0.0, 0.0, 0.0};
    omega_plus[param_idx] = +delta;
    Matrix3x3 dR_plus  = rodrigues(omega_plus);
    Matrix3x3 R_plus   = matrix_multiply(dR_plus, R);

    /* R- = rodrigues(-delta * ek) * R */
    double omega_minus[3] = {0.0, 0.0, 0.0};
    omega_minus[param_idx] = -delta;
    Matrix3x3 dR_minus = rodrigues(omega_minus);
    Matrix3x3 R_minus  = matrix_multiply(dR_minus, R);

    double Ep = compute_objective_3param(base, ref, R_plus,  u_min, v_min, u_max, v_max);
    double Em = compute_objective_3param(base, ref, R_minus, u_min, v_min, u_max, v_max);

    return (Ep - Em) / (2.0 * delta);
}

/* ===========================
 * ロドリゲス公式
 * =========================== */

Matrix3x3 rodrigues(double omega[3]) {
    /* 回転角 θ = ||ω|| */
    double theta = sqrt(omega[0]*omega[0] + omega[1]*omega[1] + omega[2]*omega[2]);

    /* ωがほぼゼロ → 単位行列 */
    if (theta < 1e-12) {
        return matrix_identity();
    }

    /* 回転軸 n = ω / θ */
    double n1 = omega[0] / theta;
    double n2 = omega[1] / theta;
    double n3 = omega[2] / theta;

    double c = cos(theta);
    double s = sin(theta);
    double mc = 1.0 - c;  /* 1 - cos(θ) */

    /* 池内論文 式(3.17):
     *
     * ΔR = [n₁²(1-c)+c    n₁n₂(1-c)-n₃s  n₁n₃(1-c)+n₂s]
     *      [n₂n₁(1-c)+n₃s n₂²(1-c)+c     n₂n₃(1-c)-n₁s]
     *      [n₃n₁(1-c)-n₂s n₃n₂(1-c)+n₁s  n₃²(1-c)+c   ]
     */
    Matrix3x3 R;
    R.m[0][0] = n1*n1*mc + c;      R.m[0][1] = n1*n2*mc - n3*s;  R.m[0][2] = n1*n3*mc + n2*s;
    R.m[1][0] = n2*n1*mc + n3*s;   R.m[1][1] = n2*n2*mc + c;     R.m[1][2] = n2*n3*mc - n1*s;
    R.m[2][0] = n3*n1*mc - n2*s;   R.m[2][1] = n3*n2*mc + n1*s;  R.m[2][2] = n3*n3*mc + c;

    return R;
}

/* ===========================
 * 3×3 連立方程式（ガウスの消去法）
 * =========================== */

int solve_linear_3x3(double A[3][3], double b[3], double x[3]) {
    /* 拡大係数行列を作る */
    double aug[3][4];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            aug[i][j] = A[i][j];
        aug[i][3] = b[i];
    }

    /* 前進消去 */
    for (int col = 0; col < 3; col++) {
        /* ピボット選択（絶対値最大の行を選ぶ） */
        int pivot = col;
        for (int row = col + 1; row < 3; row++) {
            if (fabs(aug[row][col]) > fabs(aug[pivot][col]))
                pivot = row;
        }

        /* 行の入れ替え */
        if (pivot != col) {
            for (int j = 0; j < 4; j++) {
                double tmp = aug[col][j];
                aug[col][j] = aug[pivot][j];
                aug[pivot][j] = tmp;
            }
        }

        /* ピボットがほぼゼロ → 特異行列 */
        if (fabs(aug[col][col]) < 1e-12) {
            fprintf(stderr, "警告: 特異行列のため連立方程式を解けません\n");
            x[0] = x[1] = x[2] = 0.0;
            return -1;
        }

        /* 消去 */
        for (int row = col + 1; row < 3; row++) {
            double factor = aug[row][col] / aug[col][col];
            for (int j = col; j < 4; j++)
                aug[row][j] -= factor * aug[col][j];
        }
    }

    /* 後退代入 */
    for (int i = 2; i >= 0; i--) {
        x[i] = aug[i][3];
        for (int j = i + 1; j < 3; j++)
            x[i] -= aug[i][j] * x[j];
        x[i] /= aug[i][i];
    }

    return 0;
}