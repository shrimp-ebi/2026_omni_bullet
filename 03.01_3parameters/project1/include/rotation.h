/* rotation.h
 * 回転行列の計算
 * 
 * 注視点Gから回転行列Rを計算する
 * R = [ex ey ez] (各列が回転後の座標軸)
 */

#ifndef ROTATION_H
#define ROTATION_H

#include "vector_math.h"
#include "image_utils.h"

/* ===========================
 * 回転行列の計算
 * =========================== */

/* 注視点Gから回転行列を計算
 * 
 * 入力:
 *   G  - 注視点の世界座標（単位ベクトル）
 * 
 * 出力:
 *   R - 回転行列
 * 
 * 計算手順:
 *   1. ez = N[G]           : Z軸（光軸方向）
 *   2. ex = N[up × ez]     : X軸（水平方向）
 *   3. ey = ez × ex        : Y軸（垂直方向）
 *   4. R = [ex ey ez]      : 回転行列を構成
 */
Matrix3x3 compute_rotation_matrix(Vector3D G);

/* Z軸方向の計算（光軸方向）
 * 
 * 式(4): ez = N[G]
 * 
 * 注視点Gの方向を正規化して、回転後のZ軸とする
 */
Vector3D compute_ez(Vector3D G);

/* X軸方向の計算（水平方向）
 * 
 * 式: ex = N[up × ez]
 * 
 * 基準上方向ベクトルupとezの外積を正規化して、回転後のX軸とする
 */
Vector3D compute_ex(Vector3D ez);

/* Y軸方向の計算（垂直方向）
 * 
 * 式: ey = ez × ex
 * 
 * ZとXの外積で、回転後のY軸を計算
 * 右手系座標系を構成
 */
Vector3D compute_ey(Vector3D ez, Vector3D ex);

/* ===========================
 * 共通画像・角度微分ユーティリティ
 * =========================== */

/* グレースケール値をバイリニア補間で取得 */
double image_gray_bilinear(Image *img, double u, double v);

/* 参照画像上での ∂S/∂θ, ∂S/∂φ を計算（中央差分） */
void image_derivative_theta_phi(Image *img, double u, double v,
                                double *dS_dtheta, double *dS_dphi);

/* ∂θ/∂X, ∂θ/∂Y, ∂θ/∂Z, ∂φ/∂X, ∂φ/∂Y, ∂φ/∂Z を計算 */
void angle_jacobian_xyz(double theta, double phi,
                        double *dth_dX, double *dth_dY, double *dth_dZ,
                        double *dph_dX, double *dph_dY, double *dph_dZ);

/* ===========================
 * 検証・デバッグ用
 * =========================== */

/* 回転行列が正しく構成されているかチェック
 * 
 * チェック項目:
 *   - 各軸が単位ベクトル
 *   - 各軸が直交している
 *   - 右手系である（det(R) = 1）
 * 
 * 戻り値:
 *   1: OK
 *   0: NG
 */
int rotation_matrix_verify(Matrix3x3 R);

/* 回転行列の詳細情報を表示 */
void rotation_matrix_info(Matrix3x3 R);

#endif /* ROTATION_H */
