/* debug_grad.c
 * ω₂（Y軸のみ）ψ=5° における compute_gradient_3param() の内部値を
 * 画像中心の1画素だけに絞って表示するデバッグプログラム
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
    printf("画像サイズ: %d x %d\n", W, H);

    /* ψ = 5° で ω₂ のみ設定 */
    double psi_deg = 5.0;
    double psi_rad = psi_deg * M_PI / 180.0;
    double omega[3] = {0.0, psi_rad, 0.0};
    Matrix3x3 R = rodrigues(omega);

    printf("\n=== 回転行列 R (ω₂=%.1f°) ===\n", psi_deg);
    for (int i = 0; i < 3; i++)
        printf("  R[%d] = (%f, %f, %f)\n", i, R.m[i][0], R.m[i][1], R.m[i][2]);

    /* 画像中心の1点 */
    int u = W / 2;
    int v = H / 2;
    printf("\n=== 対象画素: u=%d, v=%d ===\n", u, v);

    /* ── 座標変換 ────────────────────────────────────── */
    Vector3D X = image_to_world(u, v, W, H);
    printf("\n[座標変換]\n");
    printf("X = (%f, %f, %f)\n", X.x, X.y, X.z);

    Vector3D Xp = matrix_vector_multiply(R, X);
    printf("Xp = R*X = (%f, %f, %f)\n", Xp.x, Xp.y, Xp.z);

    double u_ref, v_ref;
    world_to_image(Xp, W, H, &u_ref, &v_ref);
    printf("u_ref=%f, v_ref=%f\n", u_ref, v_ref);

    /* ── 輝度差 ───────────────────────────────────────── */
    printf("\n[輝度値]\n");
    uint8_t rgb_b[3];
    get_pixel(base, u, v, rgb_b);
    double Sb = (rgb_b[0] + rgb_b[1] + rgb_b[2]) / 3.0;
    double Sr = image_gray_bilinear(ref, u_ref, v_ref);
    double diff = Sr - Sb;
    printf("Sb = %f\n", Sb);
    printf("Sr = %f\n", Sr);
    printf("diff = Sr - Sb = %f\n", diff);

    /* ── 画像微分 ─────────────────────────────────────── */
    printf("\n[画像微分]\n");
    double dS_dtheta, dS_dphi;
    image_derivative_theta_phi(ref, u_ref, v_ref, &dS_dtheta, &dS_dphi);
    printf("dS_dtheta = %f\n", dS_dtheta);
    printf("dS_dphi   = %f\n", dS_dphi);

    /* ── 角度ヤコビアン ────────────────────────────────── */
    printf("\n[角度ヤコビアン]\n");
    double theta_p, phi_p;
    world_to_angle(Xp, &theta_p, &phi_p);
    printf("theta_p = %f rad (%f deg)\n", theta_p, theta_p * 180.0 / M_PI);
    printf("phi_p   = %f rad (%f deg)\n", phi_p,   phi_p   * 180.0 / M_PI);

    double dth_dX, dth_dY, dth_dZ;
    double dph_dX, dph_dY, dph_dZ;
    angle_jacobian_xyz(theta_p, phi_p,
                       &dth_dX, &dth_dY, &dth_dZ,
                       &dph_dX, &dph_dY, &dph_dZ);
    printf("dth: dX=%f  dY=%f  dZ=%f\n", dth_dX, dth_dY, dth_dZ);
    printf("dph: dX=%f  dY=%f  dZ=%f\n", dph_dX, dph_dY, dph_dZ);

    /* ── 連鎖律1段目 ──────────────────────────────────── */
    printf("\n[連鎖律1段目: dSr/dX']\n");
    double dSr_dXp = dS_dtheta * dth_dX + dS_dphi * dph_dX;
    double dSr_dYp = dS_dtheta * dth_dY + dS_dphi * dph_dY;
    double dSr_dZp = dS_dtheta * dth_dZ + dS_dphi * dph_dZ;
    printf("dSr: dXp=%f  dYp=%f  dZp=%f\n", dSr_dXp, dSr_dYp, dSr_dZp);

    /* ── k=1（ω₂）の ∂X'/∂ω₂ ──────────────────────────── */
    printf("\n[dXprime_domega (k=1, ω₂)]\n");
    double R1x = R.m[0][0], R1y = R.m[0][1], R1z = R.m[0][2];
    double R3x = R.m[2][0], R3y = R.m[2][1], R3z = R.m[2][2];
    double Xx = X.x, Xy = X.y, Xz = X.z;

    double dXp_x =   R3x*Xx + R3y*Xy + R3z*Xz;   /* R₃行 · X */
    double dXp_y = 0.0;
    double dXp_z = -(R1x*Xx + R1y*Xy + R1z*Xz);  /* -(R₁行 · X) */
    printf("dXp_dom2 = (%f, %f, %f)\n", dXp_x, dXp_y, dXp_z);
    printf("  R₃·X = %f\n", R3x*Xx + R3y*Xy + R3z*Xz);
    printf("  R₁·X = %f\n", R1x*Xx + R1y*Xy + R1z*Xz);

    /* ── 最終的な chain と寄与 ─────────────────────────── */
    printf("\n[最終計算 (k=1)]\n");
    double chain = dSr_dXp * dXp_x + dSr_dYp * dXp_y + dSr_dZp * dXp_z;
    printf("chain(k=1) = %f\n", chain);
    printf("diff * chain = %f * %f = %f\n", diff, chain, diff * chain);

    /* ── 参考: 数値微分でのこの画素の寄与 ───────────────── */
    printf("\n[参考: 数値微分での比較]\n");
    double delta = 1e-4;
    double omega_p[3] = {0.0,  psi_rad + delta, 0.0};
    double omega_m[3] = {0.0,  psi_rad - delta, 0.0};
    Matrix3x3 R_plus  = rodrigues(omega_p);
    Matrix3x3 R_minus = rodrigues(omega_m);

    Vector3D Xp_plus  = matrix_vector_multiply(R_plus,  X);
    Vector3D Xp_minus = matrix_vector_multiply(R_minus, X);
    double up, vp, um, vm;
    world_to_image(Xp_plus,  W, H, &up, &vp);
    world_to_image(Xp_minus, W, H, &um, &vm);
    double Sr_plus  = image_gray_bilinear(ref, up, vp);
    double Sr_minus = image_gray_bilinear(ref, um, vm);
    printf("Sr(ω₂+δ) = %f  at (u=%f, v=%f)\n", Sr_plus,  up, vp);
    printf("Sr(ω₂-δ) = %f  at (u=%f, v=%f)\n", Sr_minus, um, vm);
    double dSr_dω2_num = (Sr_plus - Sr_minus) / (2.0 * delta);
    printf("数値: dSr/dω₂ = %f\n", dSr_dω2_num);
    printf("解析: dSr/dω₂ = chain = %f\n", chain);

    /* ── 数値微分の摂動方向の確認 ───────────────────────── */
    printf("\n[数値微分: 左乗算 vs ω直接摂動の比較]\n");
    /* 現在のコードの数値微分: 左乗算 exp(δ*e2^×)*R */
    double omega_ek[3] = {0.0, delta, 0.0};
    Matrix3x3 dR = rodrigues(omega_ek);
    Matrix3x3 R_left_plus  = matrix_multiply(dR, R);

    double omega_ek_m[3] = {0.0, -delta, 0.0};
    Matrix3x3 dRm = rodrigues(omega_ek_m);
    Matrix3x3 R_left_minus = matrix_multiply(dRm, R);

    Vector3D Xp_lp = matrix_vector_multiply(R_left_plus,  X);
    Vector3D Xp_lm = matrix_vector_multiply(R_left_minus, X);
    double ulp, vlp, ulm, vlm;
    world_to_image(Xp_lp, W, H, &ulp, &vlp);
    world_to_image(Xp_lm, W, H, &ulm, &vlm);
    double Sr_lp = image_gray_bilinear(ref, ulp, vlp);
    double Sr_lm = image_gray_bilinear(ref, ulm, vlm);
    printf("左乗算 Sr(+) = %f  at (u=%f, v=%f)\n", Sr_lp, ulp, vlp);
    printf("左乗算 Sr(-) = %f  at (u=%f, v=%f)\n", Sr_lm, ulm, vlm);
    double dSr_left = (Sr_lp - Sr_lm) / (2.0 * delta);
    printf("左乗算 dSr/dω₂ = %f\n", dSr_left);
    printf("ω直接 dSr/dω₂ = %f\n", dSr_dω2_num);

    image_free(base);
    image_free(ref);
    return 0;
}
