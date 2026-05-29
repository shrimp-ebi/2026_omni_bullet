/* rotation.c
 * 回転行列の計算の実装
 */

#include "rotation.h"
#include "vector_math.h"
#include "image_utils.h"
#include "coord_transform.h"
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===========================
 * 回転行列の計算
 * =========================== */

/* Z軸方向の計算（光軸方向）
 * 
 * 式(4): ez = N[G]
 */
Vector3D compute_ez(Vector3D G) {
    return vector_normalize(G);
}

/* X軸方向の計算（水平方向）
 * 
 * 式: ex = N[up × ez]
 */
Vector3D compute_ex(Vector3D ez) {
    Vector3D up = vector_create(0.0, 1.0, 0.0);
    Vector3D cross = vector_cross(up, ez);

    /* 光軸がupと平行に近い場合は代替の基準ベクトルを使う */
    if (vector_norm(cross) < 1e-10) {
        Vector3D alt_up = vector_create(1.0, 0.0, 0.0);
        cross = vector_cross(alt_up, ez);
    }

    return vector_normalize(cross);
}

/* Y軸方向の計算（垂直方向）
 * 
 * 式: ey = ez × ex
 */
Vector3D compute_ey(Vector3D ez, Vector3D ex) {
    Vector3D cross = vector_cross(ez, ex);

    /* 理論上は既に単位ベクトルだが、数値誤差対策で正規化 */
    return vector_normalize(cross);
}

/* 注視点Gから回転行列を計算
 * 
 * 式(7): R = [ex ey ez]^T（転置版）
 * 
 * この定義により、逆変換は X = R^T X' で表現される
 */
Matrix3x3 compute_rotation_matrix(Vector3D G) {
    Matrix3x3 R;
    
    /* 1. Z軸（光軸方向）を計算 */
    Vector3D ez = compute_ez(G);
    printf("\n【デバッグ】回転行列計算\n");
    vector_print("  ez (光軸)", ez);
    
    /* 2. X軸（水平方向）を計算 */
    Vector3D ex = compute_ex(ez);
    vector_print("  ex (水平)", ex);

    /* 3. Y軸（垂直方向）を計算 */
    Vector3D ey = compute_ey(ez, ex);
    vector_print("  ey (垂直)", ey);

    /* 4. 回転行列を構成: R = [ex ey ez]^T（転置版） */
    /* 各行ベクトルとして並べる: R = [ex^T; ey^T; ez^T] */
    R.m[0][0] = ex.x;  R.m[0][1] = ex.y;  R.m[0][2] = ex.z;
    R.m[1][0] = ey.x;  R.m[1][1] = ey.y;  R.m[1][2] = ey.z;
    R.m[2][0] = ez.x;  R.m[2][1] = ez.y;  R.m[2][2] = ez.z;
    
    printf("\n各軸の確認（転置版: 各行がベクトル）:\n");
    printf("  ex = (%.6f, %.6f, %.6f)\n", ex.x, ex.y, ex.z);
    printf("  ey = (%.6f, %.6f, %.6f)\n", ey.x, ey.y, ey.z);
    printf("  ez = (%.6f, %.6f, %.6f)\n", ez.x, ez.y, ez.z);
    
    /* 各行ベクトルを取り出す（R = [ex; ey; ez]） */
    Vector3D R_row0 = {R.m[0][0], R.m[0][1], R.m[0][2]};
    Vector3D R_row1 = {R.m[1][0], R.m[1][1], R.m[1][2]};
    Vector3D R_row2 = {R.m[2][0], R.m[2][1], R.m[2][2]};
    
    vector_print("  第1行（ex）", R_row0);
    vector_print("  第2行（ey）", R_row1);
    vector_print("  第3行（ez）", R_row2);
    
    return R;
}


/* ===========================
 * 検証・デバッグ用
 * =========================== */

/* 回転行列が正しく構成されているかチェック */
int rotation_matrix_verify(Matrix3x3 R) {
    int ok = 1;
    double eps = 1e-6;
    
    printf("\n【回転行列の検証】\n");
    
    /* 各行ベクトルを取り出す（R = [ex; ey; ez]） */
    Vector3D ex = {R.m[0][0], R.m[0][1], R.m[0][2]};
    Vector3D ey = {R.m[1][0], R.m[1][1], R.m[1][2]};
    Vector3D ez = {R.m[2][0], R.m[2][1], R.m[2][2]};
    
    /* 1. 各軸が単位ベクトルか */
    double norm_ex = vector_norm(ex);
    double norm_ey = vector_norm(ey);
    double norm_ez = vector_norm(ez);
    
    printf("(1) 単位ベクトルチェック:\n");
    printf("    ||ex|| = %.10f ", norm_ex);
    if (fabs(norm_ex - 1.0) < eps) {
        printf("✓\n");
    } else {
        printf("✗ (1.0であるべき)\n");
        ok = 0;
    }
    
    printf("    ||ey|| = %.10f ", norm_ey);
    if (fabs(norm_ey - 1.0) < eps) {
        printf("✓\n");
    } else {
        printf("✗ (1.0であるべき)\n");
        ok = 0;
    }
    
    printf("    ||ez|| = %.10f ", norm_ez);
    if (fabs(norm_ez - 1.0) < eps) {
        printf("✓\n");
    } else {
        printf("✗ (1.0であるべき)\n");
        ok = 0;
    }
    
    /* 2. 各軸が直交しているか */
    double dot_xy = vector_dot(ex, ey);
    double dot_yz = vector_dot(ey, ez);
    double dot_zx = vector_dot(ez, ex);
    
    printf("(2) 直交性チェック:\n");
    printf("    ex·ey = %.10f ", dot_xy);
    if (fabs(dot_xy) < eps) {
        printf("✓\n");
    } else {
        printf("✗ (0.0であるべき)\n");
        ok = 0;
    }
    
    printf("    ey·ez = %.10f ", dot_yz);
    if (fabs(dot_yz) < eps) {
        printf("✓\n");
    } else {
        printf("✗ (0.0であるべき)\n");
        ok = 0;
    }
    
    printf("    ez·ex = %.10f ", dot_zx);
    if (fabs(dot_zx) < eps) {
        printf("✓\n");
    } else {
        printf("✗ (0.0であるべき)\n");
        ok = 0;
    }
    
    /* 3. 右手系かチェック（ex × ey = ez） */
    Vector3D cross = vector_cross(ex, ey);
    double diff_x = fabs(cross.x - ez.x);
    double diff_y = fabs(cross.y - ez.y);
    double diff_z = fabs(cross.z - ez.z);
    
    printf("(3) 右手系チェック (ex × ey = ez):\n");
    printf("    誤差: (%.2e, %.2e, %.2e) ", diff_x, diff_y, diff_z);
    if (diff_x < eps && diff_y < eps && diff_z < eps) {
        printf("✓\n");
    } else {
        printf("✗\n");
        ok = 0;
    }
    
    if (ok) {
        printf("\n結果: ✓ 回転行列は正しく構成されています\n");
    } else {
        printf("\n結果: ✗ 回転行列に問題があります\n");
    }
    
    return ok;
}

/* ===========================
 * 共通画像・角度微分ユーティリティ
 * =========================== */

/* グレースケール値をdouble精度でバイリニア補間して取得
 * get_pixel_bilinear は uint8_t で返すため整数丸めが生じる。
 * 微小座標変化（0.2px程度）を輝度値に反映するためdouble精度で直接計算する。
 */
double image_gray_bilinear(Image *img, double u, double v) {
    int u0 = (int)floor(u);
    int v0 = (int)floor(v);
    int u1 = u0 + 1;
    int v1 = v0 + 1;
    double fu = u - u0;
    double fv = v - v0;

    uint8_t rgb00[3], rgb10[3], rgb01[3], rgb11[3];
    get_pixel(img, u0, v0, rgb00);
    get_pixel(img, u1, v0, rgb10);
    get_pixel(img, u0, v1, rgb01);
    get_pixel(img, u1, v1, rgb11);

    double g00 = (rgb00[0] + rgb00[1] + rgb00[2]) / 3.0;
    double g10 = (rgb10[0] + rgb10[1] + rgb10[2]) / 3.0;
    double g01 = (rgb01[0] + rgb01[1] + rgb01[2]) / 3.0;
    double g11 = (rgb11[0] + rgb11[1] + rgb11[2]) / 3.0;

    return g00 * (1.0 - fu) * (1.0 - fv)
         + g10 * fu         * (1.0 - fv)
         + g01 * (1.0 - fu) * fv
         + g11 * fu         * fv;
}

/* ===========================
 * 微分関数（2種類）
 * =========================== */

/* 【方式1】中央差分による微分（ガウシアン重み付け） */
void image_derivative_theta_phi_central(Image *img, double u, double v,
                                        double *dS_dtheta, double *dS_dphi) {
    int W = img->width;
    int H = img->height;
    const double dtheta = 2.0 * M_PI / (double)W;
    const double dphi   = M_PI / (double)H;
    const double sigma  = 1.0;
    const int    radius = 3;

    /* ガウシアン重みを計算（奇対称差分用） */
    double weight[4]; /* weight[0]未使用, weight[1..3]が±1,±2,±3に対応 */
    double wsum = 0.0;
    for (int i = 1; i <= radius; i++) {
        weight[i] = (double)i * exp(-(double)(i*i) / (2.0 * sigma * sigma));
        wsum += 2.0 * weight[i];
    }
    /* 正規化 */
    for (int i = 1; i <= radius; i++)
        weight[i] /= wsum;

    /* θ方向（u方向）の加重差分 */
    double sum_theta = 0.0;
    for (int i = 1; i <= radius; i++) {
        double Sp = image_gray_bilinear(img, u + (double)i, v);
        double Sm = image_gray_bilinear(img, u - (double)i, v);
        sum_theta += weight[i] * (Sp - Sm);
    }
    *dS_dtheta = sum_theta / dtheta;

    /* φ方向（v方向）の加重差分
     * phi = (H - v)*pi/H なので dv/dphi = -H/pi = -1/dphi */
    double sum_phi = 0.0;
    for (int i = 1; i <= radius; i++) {
        double Sp = image_gray_bilinear(img, u, v + (double)i);
        double Sm = image_gray_bilinear(img, u, v - (double)i);
        sum_phi += weight[i] * (Sp - Sm);
    }
    double dS_dv = sum_phi;
    *dS_dphi = dS_dv * (-1.0 / dphi);
}

/* 【方式2】5×5 Sobelフィルタ（GaussianBlur後の微分） */
static double image_sobel5_dx(Image *img, double u, double v)
{
    static const double deriv[5] = { 1.0, -8.0, 0.0, 8.0, -1.0 };
    static const double smooth[5] = { 1.0, 4.0, 6.0, 4.0, 1.0 };
    double sum = 0.0;

    for (int dy = -2; dy <= 2; dy++) {
        double wy = smooth[dy + 2];
        for (int dx = -2; dx <= 2; dx++) {
            double wx = deriv[dx + 2];
            sum += wy * wx * image_gray_bilinear(img, u + (double)dx, v + (double)dy);
        }
    }

    return sum / 192.0;  /* 16*12 = 192 で正規化 */
}

static double image_sobel5_dy(Image *img, double u, double v)
{
    static const double deriv[5] = { 1.0, -8.0, 0.0, 8.0, -1.0 };
    static const double smooth[5] = { 1.0, 4.0, 6.0, 4.0, 1.0 };
    double sum = 0.0;

    for (int dy = -2; dy <= 2; dy++) {
        double wy = deriv[dy + 2];
        for (int dx = -2; dx <= 2; dx++) {
            double wx = smooth[dx + 2];
            sum += wy * wx * image_gray_bilinear(img, u + (double)dx, v + (double)dy);
        }
    }

    return sum / 192.0;
}

/* 参照画像上での ∂S/∂θ, ∂S/∂φ を計算（GaussianBlur + Sobel 5x5） */
void image_derivative_theta_phi(Image *img, double u, double v,
                                double *dS_dtheta, double *dS_dphi)
{
    *dS_dtheta = image_sobel5_dx(img, u, v);
    *dS_dphi   = image_sobel5_dy(img, u, v);
}

/* Sobel版の明示的エイリアス */
void image_derivative_theta_phi_sobel(Image *img, double u, double v,
                                      double *dS_dtheta, double *dS_dphi)
{
    image_derivative_theta_phi(img, u, v, dS_dtheta, dS_dphi);
}

/* ∂θ/∂X, ∂θ/∂Y, ∂θ/∂Z, ∂φ/∂X, ∂φ/∂Y, ∂φ/∂Z を計算 */
void angle_jacobian_xyz(double theta, double phi,
                        double *dth_dX, double *dth_dY, double *dth_dZ,
                        double *dph_dX, double *dph_dY, double *dph_dZ) {
    double sinphi = sin(phi);
    double costh  = cos(theta);
    double sinth  = sin(theta);
    double cosphi = cos(phi);

    /* 極（sinphi≈0）で発散するので安全対策 */
    if (fabs(sinphi) < 1e-8)
        sinphi = (sinphi >= 0 ? 1e-8 : -1e-8);

    *dth_dX = costh  / sinphi;
    *dth_dY = 0.0;
    *dth_dZ = -sinth / sinphi;

    *dph_dX = cosphi * sinth;
    *dph_dY = -sinphi;
    *dph_dZ = cosphi * costh;
}

/* 回転行列の詳細情報を表示 */
void rotation_matrix_info(Matrix3x3 R) {
    printf("\n【回転行列の詳細情報】\n");
    
    matrix_print("R", R);
    
    /* 各行ベクトルを取り出す（R = [ex; ey; ez]） */
    Vector3D ex = {R.m[0][0], R.m[0][1], R.m[0][2]};
    Vector3D ey = {R.m[1][0], R.m[1][1], R.m[1][2]};
    Vector3D ez = {R.m[2][0], R.m[2][1], R.m[2][2]};
    
    printf("\n各軸ベクトル:\n");
    vector_print("  ex (X軸/右方向)", ex);
    vector_print("  ey (Y軸/上方向)", ey);
    vector_print("  ez (Z軸/光軸)", ez);
}