/* y_rotation.h
 * Y軸回りの回転専用関数
 * 
 * 1パラメータ検証実験用
 */

#ifndef Y_ROTATION_H
#define Y_ROTATION_H

#include "vector_math.h"
#include "image_utils.h"

/* ===========================
 * Y軸回りの回転行列
 * =========================== */

/* Y軸回りの回転行列を生成
 * 
 * 入力:
 *   psi_deg - 回転角度（度数法）
 * 
 * 出力:
 *   R(Y)(ψ) - Y軸回りの回転行列
 * 
 * 式(9):
 *   R(Y)(ψ) = [ cos(ψ)   0  -sin(ψ) ]
 *             [   0      1     0     ]
 *             [ sin(ψ)   0   cos(ψ) ]
 */
Matrix3x3 create_y_rotation_matrix(double psi_deg);


/* ===========================
 * 回転画像の生成
 * =========================== */

/* Y軸回りに回転させた画像を生成
 * 
 * 入力:
 *   input - 入力画像（基準画像）
 *   psi_deg - 回転角度（度数法）
 * 
 * 出力:
 *   回転後の画像
 * 
 * 注意:
 *   呼び出し側でメモリ解放が必要
 */
Image* rotate_image_y_axis(Image *input, double psi_deg);


/* ===========================
 * 目的関数の計算
 * =========================== */

/* 目的関数を計算（式14）
 * 
 * E(ψ) = (1/2N) Σ (Sr(X',Y',Z') - Sb(X,Y,Z))²
 * 
 * 入力:
 *   base - 基準画像 I_b
 *   ref  - 参照画像 I_r
 *   psi_deg - 回転角度（度数法）
 *   u_min, v_min - 比較領域の左上
 *   u_max, v_max - 比較領域の右下
 * 
 * 出力:
 *   目的関数の値
 */
double compute_objective_function(
    Image *base, Image *ref,
    double psi_deg,
    int u_min, int v_min, 
    int u_max, int v_max
);


/* ===========================
 * 微分の計算
 * =========================== */

/* 理論微分を計算（式15）
 * 
 * dE/dψ = (1/N) Σ (Sr - Sb) * (∂Sr/∂X' * dX'/dψ + ... )
 * 
 * 入力:
 *   base - 基準画像 I_b
 *   ref  - 参照画像 I_r
 *   psi_deg - 回転角度（度数法）
 *   比較領域の座標
 * 
 * 出力:
 *   理論微分の値
 */
double compute_analytical_derivative(
    Image *base, Image *ref,
    double psi_deg,
    int u_min, int v_min,
    int u_max, int v_max
);

/* 数値微分を計算
 * 
 * dE/dψ ≈ (E(ψ + Δψ) - E(ψ)) / Δψ
 * 
 * 入力:
 *   base - 基準画像 I_b
 *   ref  - 参照画像 I_r
 *   psi_deg - 回転角度（度数法）
 *   delta_psi - 微小角度（度数法、デフォルト0.1）
 *   比較領域の座標
 * 
 * 出力:
 *   数値微分の値
 */
double compute_numerical_derivative(
    Image *base, Image *ref,
    double psi_deg,
    double delta_psi,
    int u_min, int v_min,
    int u_max, int v_max
);

#endif /* Y_ROTATION_H */