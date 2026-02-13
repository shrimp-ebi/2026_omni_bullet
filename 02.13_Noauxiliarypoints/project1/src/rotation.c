/* rotation.c
 * 回転行列の計算の実装
 */

#include "rotation.h"
#include "vector_math.h"
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