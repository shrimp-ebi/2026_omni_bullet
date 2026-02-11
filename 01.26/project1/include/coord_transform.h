/* coord_transform.h
 * 座標系変換関数
 * 
 * 座標系の種類:
 *   - 画像座標系 (u, v): 左上原点、右がu軸、下がv軸
 *   - 角度座標系 (θ, φ): 球面上の角度表現
 *   - 世界座標系 (X, Y, Z): 単位球面上の3次元座標
 */

#ifndef COORD_TRANSFORM_H
#define COORD_TRANSFORM_H

#include "vector_math.h"

/* ===========================
 * 基本的な座標変換
 * =========================== */

/* 画像座標 → 角度座標
 * 
 * 入力: (u, v) - 画像座標、W, H - 画像サイズ
 * 出力: (θ, φ) - 角度座標（ラジアン）
 * 
 * 式(1): θ  = (u - W/2) * 2π/W
 *        φ = -(v - H) * π/H
 */
void image_to_angle(int u, int v, int W, int H, 
                    double *theta, double *phi);

/* 角度座標 → 画像座標
 * 
 * 入力: (θ, φ) - 角度座標（ラジアン）、W, H - 画像サイズ
 * 出力: (u, v) - 画像座標
 * 
 * 式(2): u = (θ + π) * W/(2π)
 *        v = -(φ - π) * H/π
 */
void angle_to_image(double theta, double phi, int W, int H,
                    int *u, int *v);

/* 角度座標 → 世界座標
 * 
 * 入力: (θ, φ) - 角度座標（ラジアン）
 * 出力: (X, Y, Z) - 単位球面上の3次元座標
 * 
 * 式(3): X = sin(φ) * sin(θ)
 *        Y = cos(φ)
 *        Z = sin(φ) * cos(θ)
 */
Vector3D angle_to_world(double theta, double phi);

/* 世界座標 → 角度座標
 * 
 * 入力: (X, Y, Z) - 単位球面上の3次元座標
 * 出力: (θ, φ) - 角度座標（ラジアン）
 * 
 * 逆変換:
 *   θ = atan2(X, Z)
 *   φ = acos(Y)
 */
void world_to_angle(Vector3D xyz, double *theta, double *phi);


/* ===========================
 * 統合変換（便利関数）
 * =========================== */

/* 画像座標 → 世界座標
 * 
 * image_to_angle() と angle_to_world() を連続実行
 */
Vector3D image_to_world(int u, int v, int W, int H);

/* 世界座標 → 画像座標
 * 
 * world_to_angle() と angle_to_image() を連続実行
 */
void world_to_image(Vector3D xyz, int W, int H, double *u, double *v);


/* ===========================
 * デバッグ・確認用
 * =========================== */

/* 座標変換の動作確認 */
void coord_transform_test(void);

#endif /* COORD_TRANSFORM_H */