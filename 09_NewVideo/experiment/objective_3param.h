/* objective_3param.h
 * 3パラメータ回転の目的関数・微分計算
 *
 * 池内論文 式(3.3), (3.4), (3.8) に基づく実装
 */

#ifndef OBJECTIVE_3PARAM_H
#define OBJECTIVE_3PARAM_H

#include "../include/image_utils.h"
#include "../include/vector_math.h"

/* ===========================
 * 目的関数
 * =========================== */

/* 目的関数 E(R) を計算する
 *
 * 池内論文 式(3.2):
 *   E = (1/2N) Σ (Sr - Sb)²
 *
 * 引数:
 *   base   : 基準画像 Ib
 *   ref    : 参照画像 Ir
 *   R      : 現在の回転行列
 *   u_min, v_min, u_max, v_max : 比較領域
 */
double compute_objective_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max
);

/* ===========================
 * 1階微分（勾配）
 * =========================== */

/* ∂E/∂ω₁, ∂E/∂ω₂, ∂E/∂ω₃ を計算する
 *
 * 池内論文 式(3.3) + 式(3.8) の組み合わせ:
 *   ∂E/∂ωk = (1/N) Σ (Sr - Sb) * (∂Sr/∂X' * ∂X'/∂ωk + ...)
 *
 * 結果は grad[0], grad[1], grad[2] に書き込む
 */
void compute_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double grad[3]
);

/* ===========================
 * 2階微分（ヘッセ行列）
 * =========================== */

/* ∂²E/∂ωk∂ωl （ガウス・ニュートン近似）を計算する
 *
 * 池内論文 式(3.4):
 *   ∂²E/∂ωk∂ωl = (1/N) Σ (jk) * (jl)
 *   ここで jk = ∂Sr/∂X' * ∂X'/∂ωk + ...
 *
 * 結果は hessian[3][3] に書き込む
 */
void compute_hessian_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double hessian[3][3]
);

/* ===========================
 * deriv 方式用（sigma/radius 可変版）
 * =========================== */

/* ∂E/∂ω₁, ∂E/∂ω₂, ∂E/∂ω₃ を計算する（sigma 可変・差分カーネル版）
 *
 * compute_gradient_3param と同じ計算。
 * image_derivative_theta_phi の代わりに image_derivative_theta_phi_sigma を使う。
 * deriv 方式で --sigma 引数を反映するために使う。
 */
void compute_gradient_3param_sigma(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double grad[3],
    double sigma, int radius
);

/* ∂²E/∂ωk∂ωl を計算する（sigma 可変・差分カーネル版） */
void compute_hessian_3param_sigma(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double hessian[3][3],
    double sigma, int radius
);

/* ===========================
 * 数値微分（検証用）
 * =========================== */

/* ω₁方向の数値微分
 *
 * dE/dω₁ ≈ (E(ω₁+δ) - E(ω₁)) / δ
 * ここで ω₁ だけ変化させた回転行列を使う
 */
double compute_numerical_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int param_idx,   /* 0=ω₁, 1=ω₂, 2=ω₃ */
    double delta,    /* 微小変化量（ラジアン）*/
    int u_min, int v_min, int u_max, int v_max
);

/* ===========================
 * ロドリゲス公式
 * =========================== */

/* ω = (ω₁, ω₂, ω₃) から微小回転行列 ΔR を計算
 *
 * 池内論文 式(3.17):
 *   θ = ||ω||
 *   n = ω / θ
 *   ΔR = ロドリゲスの回転公式
 */
Matrix3x3 rodrigues(double omega[3]);

/* 3×3連立方程式を解く（LM法の更新量計算）
 *
 * (H + C*diag(H)) * ω = -g を解く
 * ガウスの消去法で実装
 *
 * 戻り値: 0=成功, -1=特異行列（解けない）
 */
int solve_linear_3x3(double A[3][3], double b[3], double x[3]);

#endif /* OBJECTIVE_3PARAM_H */