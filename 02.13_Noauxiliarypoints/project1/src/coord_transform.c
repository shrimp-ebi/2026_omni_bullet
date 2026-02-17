/* coord_transform.c
 * 座標系変換関数の実装
 */

#include "coord_transform.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===========================
 * 基本的な座標変換
 * =========================== */

/* 画像座標 → 角度座標
 * 
 * 式(1): θ = (u - W/2) * 2π/W
 *        φ = -(v - H) * π/H
 * 
 * 画像中心 (W/2, H/2) → (θ, φ) = (0, π/2)
 */
void image_to_angle(int u, int v, int W, int H, 
                    double *theta, double *phi) {
    *theta = ((double)u - (double)W / 2.0) * (2.0 * M_PI) / (double)W;
    *phi = -((double)v - (double)H) * M_PI / (double)H;
}

/* 角度座標 → 画像座標
 * 
 * 式(2): u = (θ + π) * W/(2π)
 *        v = -(φ - π) * H/π
 */
void angle_to_image(double theta, double phi, int W, int H,
                    int *u, int *v) {
    *u = (int)((theta + M_PI) * (double)W / (2.0 * M_PI));
    *v = (int)(-(phi - M_PI) * (double)H / M_PI);
    
    /* 画像範囲内に収める */
    if (*u < 0) *u = 0;
    if (*u >= W) *u = W - 1;
    if (*v < 0) *v = 0;
    if (*v >= H) *v = H - 1;
}

/* 角度座標 → 世界座標
 * 
 * 式(3): X = sin(φ) * sin(θ)
 *        Y = cos(φ)
 *        Z = sin(φ) * cos(θ)
 * 
 * これは以下の2段階の回転と等価:
 *   1. Y軸上の点 (0, 1, 0) をX軸周りにφ回転 → (0, cos(φ), sin(φ))
 *   2. その結果をY軸周りにθ回転 → 最終結果
 */
Vector3D angle_to_world(double theta, double phi) {
    Vector3D xyz;
    double sin_phi = sin(phi);
    
    xyz.x = sin_phi * sin(theta);
    xyz.y = cos(phi);
    xyz.z = sin_phi * cos(theta);
    
    return xyz;
}

/* 世界座標 → 角度座標
 * 
 * 逆変換:
 *   θ = atan2(X, Z)
 *   φ = acos(Y)
 * 
 * 注意:
 *   - atan2(y, x) は y/x の逆正接を計算（象限を考慮）
 *   - acos(Y) は Y = cos(φ) の逆関数
 */
void world_to_angle(Vector3D xyz, double *theta, double *phi) {
    *theta = atan2(xyz.x, xyz.z);
    *phi = acos(xyz.y);
    
    /* 数値誤差対策: Yが範囲外の場合 */
    if (xyz.y > 1.0) {
        *phi = 0.0;  /* 北極点 */
    } else if (xyz.y < -1.0) {
        *phi = M_PI;  /* 南極点 */
    }
}


/* ===========================
 * 統合変換（便利関数）
 * =========================== */

/* 画像座標 → 世界座標 */
Vector3D image_to_world(int u, int v, int W, int H) {
    double theta, phi;
    image_to_angle(u, v, W, H, &theta, &phi);
    return angle_to_world(theta, phi);
}

/* 世界座標 → 画像座標 */
void world_to_image(Vector3D xyz, int W, int H, double *u, double *v) {
    double theta, phi;
    world_to_angle(xyz, &theta, &phi);
    
    /* angle_to_imageはint型を返すので、doubleに変換 */
    *u = (theta + M_PI) * (double)W / (2.0 * M_PI);
    *v = -(phi - M_PI) * (double)H / M_PI;
}