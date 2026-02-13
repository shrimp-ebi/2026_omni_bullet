/* rotation.h
 * 回転行列の計算
 * 
 * 注視点Gと補助点Gsから回転行列Rを計算する
 * R = [ex ey ez] (各列が回転後の座標軸)
 */

#ifndef ROTATION_H
#define ROTATION_H

#include "vector_math.h"

/* ===========================
 * 回転行列の計算
 * =========================== */

/* 注視点Gと補助点Gsから回転行列を計算
 * 
 * 入力:
 *   G  - 注視点の世界座標（単位ベクトル）
 *   Gs - 補助点の世界座標（単位ベクトル）
 * 
 * 出力:
 *   R - 回転行列
 * 
 * 計算手順:
 *   1. ez = N[G]           : Z軸（光軸方向）
 *   2. ey = N[G × Gs]      : Y軸（垂直方向）
 *   3. ex = ey × ez        : X軸（水平方向）
 *   4. R = [ex ey ez]      : 回転行列を構成
 */
Matrix3x3 compute_rotation_matrix(Vector3D G, Vector3D Gs);

/* Z軸方向の計算（光軸方向）
 * 
 * 式(4): ez = N[G]
 * 
 * 注視点Gの方向を正規化して、回転後のZ軸とする
 */
Vector3D compute_ez(Vector3D G);

/* Y軸方向の計算（垂直方向）
 * 
 * 式(5): ey = N[G × Gs]
 * 
 * GとGsの外積を正規化して、回転後のY軸とする
 * これにより光軸周りの回転が固定される
 */
Vector3D compute_ey(Vector3D G, Vector3D Gs);

/* X軸方向の計算（水平方向）
 * 
 * 式(6): ex = ey × ez
 * 
 * YとZの外積で、回転後のX軸を計算
 * 右手系座標系を構成
 */
Vector3D compute_ex(Vector3D ey, Vector3D ez);

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